/*
 * Driver for sunxi SD/MMC host controllers
 * (C) Copyright 2007-2011 Reuuimlla Technology Co., Ltd.
 * (C) Copyright 2007-2011 Aaron Maoye <leafy.myeh@reuuimllatech.com>
 * (C) Copyright 2013-2014 O2S GmbH <www.o2s.ch>
 * (C) Copyright 2013-2014 David Lanzend�rfer <david.lanzendoerfer@o2s.ch>
 * (C) Copyright 2013-2014 Hans de Goede <hdegoede@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>

#include <linux/clk.h>
#include <linux/clk/sunxi.h>

#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/reset.h>

#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>


#include <linux/mmc/host.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/slot-gpio.h>

#include "sunxi-mmc.h"
#include "sunxi-mmc-sun50iw1p1-2.h"
#include "sunxi-mmc-sun50iw1p1-0_1.h"
#include "sunxi-mmc-debug.h"
#include "sunxi-mmc-export.h"

static int sunxi_mmc_reset_host(struct sunxi_mmc_host *host)
{
	unsigned long expire = jiffies + msecs_to_jiffies(250);
	u32 rval;

	mmc_writel(host, REG_GCTRL, SDXC_HARDWARE_RESET);
	do {
		rval = mmc_readl(host, REG_GCTRL);
	} while (time_before(jiffies, expire) && (rval & SDXC_HARDWARE_RESET));

	if (rval & SDXC_HARDWARE_RESET) {
		dev_err(mmc_dev(host->mmc), "fatal err reset timeout\n");
		return -EIO;
	}

	return 0;
}

static int sunxi_mmc_init_host(struct mmc_host *mmc)
{
	u32 rval;
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	if (sunxi_mmc_reset_host(host))
		return -EIO;

	mmc_writel(host, REG_FTRGL, host->dma_tl?host->dma_tl:0x20070008);
	dev_dbg(mmc_dev(host->mmc), "REG_FTRGL %x\n",mmc_readl(host,REG_FTRGL));
	mmc_writel(host, REG_TMOUT, 0xffffffff);
	mmc_writel(host, REG_IMASK, host->sdio_imask);
	mmc_writel(host, REG_RINTR, 0xffffffff);
	mmc_writel(host, REG_DBGC, 0xdeb);
	mmc_writel(host, REG_FUNS, SDXC_CEATA_ON);
	mmc_writel(host, REG_DLBA, host->sg_dma);

	rval = mmc_readl(host, REG_GCTRL);
	rval |= SDXC_INTERRUPT_ENABLE_BIT;
	rval &= ~SDXC_ACCESS_DONE_DIRECT;
	mmc_writel(host, REG_GCTRL, rval);

	return 0;
}

static void sunxi_mmc_init_idma_des(struct sunxi_mmc_host *host,
				    struct mmc_data *data)
{
	struct sunxi_idma_des *pdes = (struct sunxi_idma_des *)host->sg_cpu;
	struct sunxi_idma_des *pdes_pa = (struct sunxi_idma_des *)host->sg_dma;
	int i, max_len = (1 << host->idma_des_size_bits);

	for (i = 0; i < data->sg_len; i++) {
		pdes[i].config = SDXC_IDMAC_DES0_CH | SDXC_IDMAC_DES0_OWN |
				 SDXC_IDMAC_DES0_DIC;

		if (data->sg[i].length == max_len)
			pdes[i].buf_size = 0; /* 0 == max_len */
		else
			pdes[i].buf_size = data->sg[i].length;

		pdes[i].buf_addr_ptr1 = sg_dma_address(&data->sg[i]);
		pdes[i].buf_addr_ptr2 = (u32)&pdes_pa[i + 1];
	}

	pdes[0].config |= SDXC_IDMAC_DES0_FD;
	pdes[i - 1].config |= SDXC_IDMAC_DES0_LD;
	pdes[i - 1].config &= ~SDXC_IDMAC_DES0_DIC;

	/*
	 * Avoid the io-store starting the idmac hitting io-mem before the
	 * descriptors hit the main-mem.
	 */
	wmb();
}

static enum dma_data_direction sunxi_mmc_get_dma_dir(struct mmc_data *data)
{
	if (data->flags & MMC_DATA_WRITE)
		return DMA_TO_DEVICE;
	else
		return DMA_FROM_DEVICE;
}

static int sunxi_mmc_map_dma(struct sunxi_mmc_host *host,
			     struct mmc_data *data)
{
	u32 i, dma_len;
	struct scatterlist *sg;

	dma_len = dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
			     sunxi_mmc_get_dma_dir(data));
	if (dma_len == 0) {
		dev_err(mmc_dev(host->mmc), "dma_map_sg failed\n");
		return -ENOMEM;
	}

	for_each_sg(data->sg, sg, data->sg_len, i) {
		if (sg->offset & 3 || sg->length & 3) {
			dev_err(mmc_dev(host->mmc),
				"unaligned scatterlist: os %x length %d\n",
				sg->offset, sg->length);
			return -EINVAL;
		}
	}

	return 0;
}

static void sunxi_mmc_start_dma(struct sunxi_mmc_host *host,
				struct mmc_data *data)
{
	u32 rval;

	sunxi_mmc_init_idma_des(host, data);

	rval = mmc_readl(host, REG_GCTRL);
	rval |= SDXC_DMA_ENABLE_BIT;
	mmc_writel(host, REG_GCTRL, rval);
	rval |= SDXC_DMA_RESET;
	mmc_writel(host, REG_GCTRL, rval);

	mmc_writel(host, REG_DMAC, SDXC_IDMAC_SOFT_RESET);

	if (!(data->flags & MMC_DATA_WRITE))
		mmc_writel(host, REG_IDIE, SDXC_IDMAC_RECEIVE_INTERRUPT);

	mmc_writel(host, REG_DMAC,
		   SDXC_IDMAC_FIX_BURST | SDXC_IDMAC_IDMA_ON);
}

static void sunxi_mmc_send_manual_stop(struct sunxi_mmc_host *host,
				       struct mmc_request *req)
{
	u32 arg, cmd_val, ri;
	unsigned long expire = jiffies + msecs_to_jiffies(1000);

	cmd_val = SDXC_START | SDXC_RESP_EXPIRE |
		  SDXC_STOP_ABORT_CMD | SDXC_CHECK_RESPONSE_CRC;

	if (req->cmd->opcode == SD_IO_RW_EXTENDED) {
		cmd_val |= SD_IO_RW_DIRECT;
		arg = (1 << 31) | (0 << 28) | (SDIO_CCCR_ABORT << 9) |
		      ((req->cmd->arg >> 28) & 0x7);
	} else {
		cmd_val |= MMC_STOP_TRANSMISSION;
		arg = 0;
	}

	mmc_writel(host, REG_CARG, arg);
	mmc_writel(host, REG_CMDR, cmd_val);

	do {
		ri = mmc_readl(host, REG_RINTR);
	} while (!(ri & (SDXC_COMMAND_DONE | SDXC_INTERRUPT_ERROR_BIT)) &&
		 time_before(jiffies, expire));

	if (!(ri & SDXC_COMMAND_DONE) || (ri & SDXC_INTERRUPT_ERROR_BIT)) {
		dev_err(mmc_dev(host->mmc), "send stop command failed\n");
		if (req->stop)
			req->stop->resp[0] = -ETIMEDOUT;
	} else {
		if (req->stop)
			req->stop->resp[0] = mmc_readl(host, REG_RESP0);
	}

	mmc_writel(host, REG_RINTR, 0xffff);
}

static void sunxi_mmc_dump_errinfo(struct sunxi_mmc_host *host)
{
	struct mmc_command *cmd = host->mrq->cmd;
	struct mmc_data *data = host->mrq->data;

	/* For some cmds timeout is normal with sd/mmc cards */
	if ((host->int_sum & SDXC_INTERRUPT_ERROR_BIT) ==
		SDXC_RESP_TIMEOUT && (cmd->opcode == SD_IO_SEND_OP_COND ||
				      cmd->opcode == SD_IO_RW_DIRECT))
		return;

	dev_err(mmc_dev(host->mmc),
		"smc %d err, cmd %d,%s%s%s%s%s%s%s%s%s%s !!\n",
		host->mmc->index, cmd->opcode,
		data ? (data->flags & MMC_DATA_WRITE ? " WR" : " RD") : "",
		host->int_sum & SDXC_RESP_ERROR     ? " RE"     : "",
		host->int_sum & SDXC_RESP_CRC_ERROR  ? " RCE"    : "",
		host->int_sum & SDXC_DATA_CRC_ERROR  ? " DCE"    : "",
		host->int_sum & SDXC_RESP_TIMEOUT ? " RTO"    : "",
		host->int_sum & SDXC_DATA_TIMEOUT ? " DTO"    : "",
		host->int_sum & SDXC_FIFO_RUN_ERROR  ? " FE"     : "",
		host->int_sum & SDXC_HARD_WARE_LOCKED ? " HL"     : "",
		host->int_sum & SDXC_START_BIT_ERROR ? " SBE"    : "",
		host->int_sum & SDXC_END_BIT_ERROR   ? " EBE"    : ""
		);
	//sunxi_mmc_dumphex32(host,"sunxi mmc",host->reg_base,0x180);
	//sunxi_mmc_dump_des(host,host->sg_cpu,PAGE_SIZE);
}

/* Called in interrupt context! */
static irqreturn_t sunxi_mmc_finalize_request(struct sunxi_mmc_host *host)
{
	struct mmc_request *mrq = host->mrq;
	struct mmc_data *data = mrq->data;
	u32 rval;

	mmc_writel(host, REG_IMASK, host->sdio_imask);
	mmc_writel(host, REG_IDIE, 0);

	if (host->int_sum & SDXC_INTERRUPT_ERROR_BIT) {
		sunxi_mmc_dump_errinfo(host);
		mrq->cmd->error = -ETIMEDOUT;

		if (data) {
			data->error = -ETIMEDOUT;
			host->manual_stop_mrq = mrq;
		}

		if (mrq->stop)
			mrq->stop->error = -ETIMEDOUT;
	} else {
		if (mrq->cmd->flags & MMC_RSP_136) {
			mrq->cmd->resp[0] = mmc_readl(host, REG_RESP3);
			mrq->cmd->resp[1] = mmc_readl(host, REG_RESP2);
			mrq->cmd->resp[2] = mmc_readl(host, REG_RESP1);
			mrq->cmd->resp[3] = mmc_readl(host, REG_RESP0);
		} else {
			mrq->cmd->resp[0] = mmc_readl(host, REG_RESP0);
		}

		if (data)
			data->bytes_xfered = data->blocks * data->blksz;
	}

	if (data) {
		mmc_writel(host, REG_IDST, 0x337);
		mmc_writel(host, REG_DMAC, 0);
		rval = mmc_readl(host, REG_GCTRL);
		rval |= SDXC_DMA_RESET;
		mmc_writel(host, REG_GCTRL, rval);
		rval &= ~SDXC_DMA_ENABLE_BIT;
		mmc_writel(host, REG_GCTRL, rval);
		rval |= SDXC_FIFO_RESET;
		mmc_writel(host, REG_GCTRL, rval);
		dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
				     sunxi_mmc_get_dma_dir(data));
	}

	mmc_writel(host, REG_RINTR, 0xffff);

	host->mrq = NULL;
	host->int_sum = 0;
	host->wait_dma = false;

	return host->manual_stop_mrq ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static irqreturn_t sunxi_mmc_irq(int irq, void *dev_id)
{
	struct sunxi_mmc_host *host = dev_id;
	struct mmc_request *mrq;
	u32 msk_int, idma_int;
	bool finalize = false;
	bool sdio_int = false;
	irqreturn_t ret = IRQ_HANDLED;

	spin_lock(&host->lock);

	idma_int  = mmc_readl(host, REG_IDST);
	msk_int   = mmc_readl(host, REG_MISTA);

	dev_dbg(mmc_dev(host->mmc), "irq: rq %p mi %08x idi %08x\n",
		host->mrq, msk_int, idma_int);

	mrq = host->mrq;
	if (mrq) {
		if (idma_int & SDXC_IDMAC_RECEIVE_INTERRUPT)
			host->wait_dma = false;

		host->int_sum |= msk_int;

		/* Wait for COMMAND_DONE on RESPONSE_TIMEOUT before finalize */
		if ((host->int_sum & SDXC_RESP_TIMEOUT) &&
				!(host->int_sum & SDXC_COMMAND_DONE))
			mmc_writel(host, REG_IMASK,
				   host->sdio_imask | SDXC_COMMAND_DONE);
		/* Don't wait for dma on error */
		else if (host->int_sum & SDXC_INTERRUPT_ERROR_BIT)
			finalize = true;
		else if ((host->int_sum & SDXC_INTERRUPT_DONE_BIT) &&
				!host->wait_dma)
			finalize = true;
	}

	if (msk_int & SDXC_SDIO_INTERRUPT)
		sdio_int = true;

	mmc_writel(host, REG_RINTR, msk_int);
	mmc_writel(host, REG_IDST, idma_int);

	if (finalize)
		ret = sunxi_mmc_finalize_request(host);

	spin_unlock(&host->lock);

	if (finalize && ret == IRQ_HANDLED)
		mmc_request_done(host->mmc, mrq);

	if (sdio_int)
		mmc_signal_sdio_irq(host->mmc);

	return ret;
}

static irqreturn_t sunxi_mmc_handle_manual_stop(int irq, void *dev_id)
{
	struct sunxi_mmc_host *host = dev_id;
	struct mmc_request *mrq;
	unsigned long iflags;

	spin_lock_irqsave(&host->lock, iflags);
	mrq = host->manual_stop_mrq;
	spin_unlock_irqrestore(&host->lock, iflags);

	if (!mrq) {
		dev_err(mmc_dev(host->mmc), "no request for manual stop\n");
		return IRQ_HANDLED;
	}

	dev_err(mmc_dev(host->mmc), "data error, sending stop command\n");

	/*
	 * We will never have more than one outstanding request,
	 * and we do not complete the request until after
	 * we've cleared host->manual_stop_mrq so we do not need to
	 * spin lock this function.
	 * Additionally we have wait states within this function
	 * so having it in a lock is a very bad idea.
	 */
	sunxi_mmc_send_manual_stop(host, mrq);

	spin_lock_irqsave(&host->lock, iflags);
	host->manual_stop_mrq = NULL;
	spin_unlock_irqrestore(&host->lock, iflags);

	mmc_request_done(host->mmc, mrq);

	return IRQ_HANDLED;
}

#if 0
static int sunxi_mmc_oclk_onoff(struct sunxi_mmc_host *host, u32 oclk_en)
{
	unsigned long expire = jiffies + msecs_to_jiffies(250);
	u32 rval;

	rval = mmc_readl(host, REG_CLKCR);
	rval &= ~(SDXC_CARD_CLOCK_ON | SDXC_LOW_POWER_ON);

	if (oclk_en)
		rval |= SDXC_CARD_CLOCK_ON;

	mmc_writel(host, REG_CLKCR, rval);

	rval = SDXC_START | SDXC_UPCLK_ONLY | SDXC_WAIT_PRE_OVER;
	mmc_writel(host, REG_CMDR, rval);

	do {
		rval = mmc_readl(host, REG_CMDR);
	} while (time_before(jiffies, expire) && (rval & SDXC_START));

	/* clear irq status bits set by the command */
	mmc_writel(host, REG_RINTR,
		   mmc_readl(host, REG_RINTR) & ~SDXC_SDIO_INTERRUPT);

	if (rval & SDXC_START) {
		dev_err(mmc_dev(host->mmc), "fatal err update clk timeout\n");
		return -EIO;
	}

	return 0;
}


static int sunxi_mmc_clk_set_rate(struct sunxi_mmc_host *host,
				  struct mmc_ios *ios)
{
	u32 rate, oclk_dly, rval, sclk_dly;
	int ret;

	rate = clk_round_rate(host->clk_mmc, ios->clock);
	dev_dbg(mmc_dev(host->mmc), "setting clk to %d, rounded %d\n",
		ios->clock, rate);

	/* setting clock rate */
	ret = clk_set_rate(host->clk_mmc, rate);
	if (ret) {
		dev_err(mmc_dev(host->mmc), "error setting clk to %d: %d\n",
			rate, ret);
		return ret;
	}

	ret = sunxi_mmc_oclk_onoff(host, 0);
	if (ret)
		return ret;

	/* clear internal divider */
	rval = mmc_readl(host, REG_CLKCR);
	rval &= ~0xff;
	mmc_writel(host, REG_CLKCR, rval);

	/* determine delays */
	if (rate <= 400000) {
		oclk_dly = 0;
		sclk_dly = 7;
	} else if (rate <= 25000000) {
		oclk_dly = 0;
		sclk_dly = 5;
	} else if (rate <= 50000000) {
		if (ios->timing == MMC_TIMING_UHS_DDR50) {
			oclk_dly = 2;
			sclk_dly = 4;
		} else {
			oclk_dly = 3;
			sclk_dly = 5;
		}
	} else {
		/* rate > 50000000 */
		oclk_dly = 2;
		sclk_dly = 4;
	}

	clk_sunxi_mmc_phase_control(host->clk_mmc, sclk_dly, oclk_dly);

	return sunxi_mmc_oclk_onoff(host, 1);
}
#endif

s32 sunxi_mmc_update_clk(struct sunxi_mmc_host* host)
{
  	u32 rval;
	unsigned long expire = jiffies + msecs_to_jiffies(1000);	//1000ms timeout
  	s32 ret = 0;

	rval = SDXC_START | SDXC_UPCLK_ONLY | SDXC_WAIT_PRE_OVER;
	//if (smc_host->voltage_switching)
	//	rval |= SDXC_VolSwitch;
	mmc_writel(host, REG_CMDR, rval);

	do {
		rval = mmc_readl(host, REG_CMDR);
	} while (time_before(jiffies, expire) && (rval & SDXC_START));

	if (rval & SDXC_START) {
		dev_err(mmc_dev(host->mmc), "update clock timeout, fatal error!!!\n");
		ret = -EIO;
	}

	return ret;
}


static void sunxi_mmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	u32 rval;

	/* Set the power state */
	switch (ios->power_mode) {
	case MMC_POWER_ON:
		break;

	case MMC_POWER_UP:
		if(host->power_on){
			break;
		}
		if (!IS_ERR(mmc->supply.vmmc))
			mmc_regulator_set_ocr(mmc, mmc->supply.vmmc, ios->vdd);

		if (!IS_ERR(mmc->supply.vqmmc)) {
			rval = regulator_enable(mmc->supply.vqmmc);
			if (rval < 0)
				dev_err(mmc_dev(mmc),
					"failed to enable vqmmc regulator\n");
		}


		rval = pinctrl_select_state(host->pinctrl, host->pins_default);
		if (rval)
			dev_warn(mmc_dev(mmc), "could not set default pins\n");


#ifndef USE_OLD_SYS_CLK_INTERFACE
		rval = clk_prepare_enable(host->clk_ahb);
		if (rval) {
			dev_err(mmc_dev(mmc), "Enable ahb clk err %d\n", rval);
			return;
		}
#endif
		rval = clk_prepare_enable(host->clk_mmc);
		if (rval) {
			dev_err(mmc_dev(mmc), "Enable mmc clk err %d\n", rval);
			return;
		}
#ifndef USE_OLD_SYS_CLK_INTERFACE
		if (!IS_ERR(host->reset)) {
			rval = reset_control_deassert(host->reset);
			if (rval) {
				dev_err(mmc_dev(mmc), "reset err %d\n", rval);
				return;
			}
		}
#endif

		host->ferror = sunxi_mmc_init_host(mmc);
		if (host->ferror)
			return;

		enable_irq(host->irq);

		host->power_on = 1;
		dev_dbg(mmc_dev(mmc), "power on!\n");
		break;

	case MMC_POWER_OFF:
		if(!host->power_on){
			break;
		}
		sunxi_mmc_reset_host(host);

#ifndef USE_OLD_SYS_CLK_INTERFACE
		if (!IS_ERR(host->reset))
			reset_control_assert(host->reset);
#endif
		clk_disable_unprepare(host->clk_mmc);
#ifndef USE_OLD_SYS_CLK_INTERFACE
		clk_disable_unprepare(host->clk_ahb);
#endif

		rval = pinctrl_select_state(host->pinctrl, host->pins_sleep);
		if (rval)
			dev_warn(mmc_dev(mmc), "could not set sleep pins\n");

		if (!IS_ERR(mmc->supply.vqmmc))
			regulator_disable(mmc->supply.vqmmc);

		if (!IS_ERR(mmc->supply.vmmc))
			mmc_regulator_set_ocr(mmc, mmc->supply.vmmc, 0);

		disable_irq(host->irq);

		host->power_on = 0;
		dev_dbg(mmc_dev(mmc), "power off!\n");
		break;
	}

	/* set bus width */
	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_1:
		mmc_writel(host, REG_WIDTH, SDXC_WIDTH1);
		break;
	case MMC_BUS_WIDTH_4:
		mmc_writel(host, REG_WIDTH, SDXC_WIDTH4);
		break;
	case MMC_BUS_WIDTH_8:
		mmc_writel(host, REG_WIDTH, SDXC_WIDTH8);
		break;
	}

	dev_dbg(mmc_dev(host->mmc), "REG_WIDTH: 0x%08x \n", mmc_readl(host, REG_WIDTH));

	/* set ddr mode */
	rval = mmc_readl(host, REG_GCTRL);
	if (ios->timing == MMC_TIMING_UHS_DDR50)
		rval |= SDXC_DDR_MODE;
	else
		rval &= ~SDXC_DDR_MODE;
	mmc_writel(host, REG_GCTRL, rval);
	dev_dbg(mmc_dev(host->mmc), "REG_GCTRL: 0x%08x \n", mmc_readl(host, REG_GCTRL));

	/* set up clock */
	if (ios->clock && ios->power_mode&& host->sunxi_mmc_clk_set_rate) {
		host->ferror = host->sunxi_mmc_clk_set_rate(host, ios);
		/* Android code had a usleep_range(50000, 55000); here */
	}
}

static void sunxi_mmc_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	unsigned long flags;
	u32 imask;

	spin_lock_irqsave(&host->lock, flags);

	imask = mmc_readl(host, REG_IMASK);
	if (enable) {
		host->sdio_imask = SDXC_SDIO_INTERRUPT;
		imask |= SDXC_SDIO_INTERRUPT;
	} else {
		host->sdio_imask = 0;
		imask &= ~SDXC_SDIO_INTERRUPT;
	}
	mmc_writel(host, REG_IMASK, imask);
	spin_unlock_irqrestore(&host->lock, flags);
}

static void sunxi_mmc_hw_reset(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	mmc_writel(host, REG_HWRST, 0);
	udelay(10);
	mmc_writel(host, REG_HWRST, 1);
	udelay(300);
}


static int	sunxi_mmc_signal_voltage_switch(struct mmc_host *mmc, struct mmc_ios *ios)
{
#ifdef CONFIG_REGULATOR
	int ret	=	0;
	struct regulator *vqmmc = mmc->supply.vqmmc;

	switch (ios->signal_voltage) {
	case MMC_SIGNAL_VOLTAGE_330:
		if (!IS_ERR(vqmmc)) {
			ret = regulator_set_voltage(vqmmc, 2700000, 3600000);
			if (ret) {
				dev_err(mmc_dev(mmc),"Switching to 3.3V signalling voltage "
						" failed\n");
				return -EIO;
			}
		}else{
			dev_err(mmc_dev(mmc),"no vqmmc\n");
			return -ENOSYS;
		}
		/* Wait for 5ms */
		//usleep_range(5000, 5500);
		return 0;
	case MMC_SIGNAL_VOLTAGE_180:
		if (!IS_ERR(vqmmc)) {
			ret = regulator_set_voltage(vqmmc,
					1700000, 1950000);
			if (ret) {
				dev_err(mmc_dev(mmc),"Switching to 1.8V signalling voltage "
						" failed\n");
				return -EIO;
			}
		}else{
				dev_err(mmc_dev(mmc),"no vqmmc\n");
				return -ENOSYS;
		}

		/* Wait for 5ms */
		//usleep_range(5000, 5500);
		return 0;
	case MMC_SIGNAL_VOLTAGE_120:
		if (!IS_ERR(vqmmc)) {
			ret = regulator_set_voltage(vqmmc, 1100000, 1300000);
			if (ret) {
				dev_err(mmc_dev(mmc),"Switching to 1.2V signalling voltage "
						" failed\n");
				return -EIO;
			}
		}else{
			dev_err(mmc_dev(mmc),"no vqmmc\n");
			return -ENOSYS;
		}

		return 0;
	default:
		/* No signal voltage switch required */
		dev_err(mmc_dev(mmc),"unknow signal voltage switch request %x\n", ios->signal_voltage);
		return -ENOSYS;
	}
#else
	return 0;
#endif
}

static int	sunxi_mmc_card_busy(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	return mmc_readl(host,REG_STAS) & SDXC_CARD_DATA_BUSY;
}


static void sunxi_mmc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data *data = mrq->data;
	unsigned long iflags;
	u32 imask = SDXC_INTERRUPT_ERROR_BIT;
	u32 cmd_val = SDXC_START | (cmd->opcode & 0x3f);
	bool wait_dma = host->wait_dma;
	int ret;

	/* Check for set_ios errors (should never happen) */
	if (host->ferror) {
		mrq->cmd->error = host->ferror;
		mmc_request_done(mmc, mrq);
		return;
	}

	if (data) {
		ret = sunxi_mmc_map_dma(host, data);
		if (ret < 0) {
			dev_err(mmc_dev(mmc), "map DMA failed\n");
			cmd->error = ret;
			data->error = ret;
			mmc_request_done(mmc, mrq);
			return;
		}
	}

	if (cmd->opcode == MMC_GO_IDLE_STATE) {
		cmd_val |= SDXC_SEND_INIT_SEQUENCE;
		imask |= SDXC_COMMAND_DONE;
	}

	if (cmd->flags & MMC_RSP_PRESENT) {
		cmd_val |= SDXC_RESP_EXPIRE;
		if (cmd->flags & MMC_RSP_136)
			cmd_val |= SDXC_LONG_RESPONSE;
		if (cmd->flags & MMC_RSP_CRC)
			cmd_val |= SDXC_CHECK_RESPONSE_CRC;

		if ((cmd->flags & MMC_CMD_MASK) == MMC_CMD_ADTC) {
			cmd_val |= SDXC_DATA_EXPIRE | SDXC_WAIT_PRE_OVER;
			if (cmd->data->flags & MMC_DATA_STREAM) {
				imask |= SDXC_AUTO_COMMAND_DONE;
				cmd_val |= SDXC_SEQUENCE_MODE |
					   SDXC_SEND_AUTO_STOP;
			}

			if (cmd->data->stop) {
				imask |= SDXC_AUTO_COMMAND_DONE;
				cmd_val |= SDXC_SEND_AUTO_STOP;
			} else {
				imask |= SDXC_DATA_OVER;
			}

			if (cmd->data->flags & MMC_DATA_WRITE)
				cmd_val |= SDXC_WRITE;
			else
				wait_dma = true;
		} else {
			imask |= SDXC_COMMAND_DONE;
		}
	} else {
		imask |= SDXC_COMMAND_DONE;
	}

	dev_dbg(mmc_dev(mmc), "cmd %d(%08x) arg %x ie 0x%08x len %d\n",
		cmd_val & 0x3f, cmd_val, cmd->arg, imask,
		mrq->data ? mrq->data->blksz * mrq->data->blocks : 0);

	spin_lock_irqsave(&host->lock, iflags);

	if (host->mrq || host->manual_stop_mrq) {
		spin_unlock_irqrestore(&host->lock, iflags);

		if (data)
			dma_unmap_sg(mmc_dev(mmc), data->sg, data->sg_len,
				     sunxi_mmc_get_dma_dir(data));

		dev_err(mmc_dev(mmc), "request already pending\n");
		mrq->cmd->error = -EBUSY;
		mmc_request_done(mmc, mrq);
		return;
	}

	if (data) {
		mmc_writel(host, REG_BLKSZ, data->blksz);
		mmc_writel(host, REG_BCNTR, data->blksz * data->blocks);
		if(host->sunxi_mmc_thld_ctl){
			host->sunxi_mmc_thld_ctl(host,&mmc->ios,data);
		}
		sunxi_mmc_start_dma(host, data);
	}

	host->mrq = mrq;
	host->wait_dma = wait_dma;
	mmc_writel(host, REG_IMASK, host->sdio_imask | imask);
	mmc_writel(host, REG_CARG, cmd->arg);
	mmc_writel(host, REG_CMDR, cmd_val);

	spin_unlock_irqrestore(&host->lock, iflags);
}

static const struct of_device_id sunxi_mmc_of_match[] = {
	{ .compatible = "allwinner,sun4i-a10-mmc", },
	{ .compatible = "allwinner,sun5i-a13-mmc", },
	{ .compatible = "allwinner,sun50i-sdmmc2", },
	{ .compatible = "allwinner,sun50i-sdmmc1", },
	{ .compatible = "allwinner,sun50i-sdmmc0", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_mmc_of_match);

static struct mmc_host_ops sunxi_mmc_ops = {
	.request	 = sunxi_mmc_request,
	.set_ios	 = sunxi_mmc_set_ios,
	.get_ro		 = mmc_gpio_get_ro,
	.get_cd		 = mmc_gpio_get_cd,
	.enable_sdio_irq = sunxi_mmc_enable_sdio_irq,
	.hw_reset	 = sunxi_mmc_hw_reset,
	.start_signal_voltage_switch = sunxi_mmc_signal_voltage_switch,
	.card_busy = sunxi_mmc_card_busy,
};

static int sunxi_mmc_resource_request(struct sunxi_mmc_host *host,
				      struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret;

	if (of_device_is_compatible(np, "allwinner,sun4i-a10-mmc")\
		||of_device_is_compatible(np, "allwinner,sun50i-sdmmc2")\
		)
		host->idma_des_size_bits = 13;
	else
		host->idma_des_size_bits = 16;


	if(of_device_is_compatible(np, "allwinner,sun50i-sdmmc2")){
 		host->sunxi_mmc_clk_set_rate = sunxi_mmc_clk_set_rate_for_sdmmc2;
		host->dma_tl = (0x3<<28)|(15<<16)|240;
		host->sunxi_mmc_thld_ctl = sunxi_mmc_thld_ctl_for_sdmmc2;
		host->sunxi_mmc_save_spec_reg = sunxi_mmc_save_spec_reg_2;
		host->sunxi_mmc_restore_spec_reg = sunxi_mmc_save_spec_reg_2;
		sunxi_mmc_reg_ex_res_inter(host,2);
 	}else if(of_device_is_compatible(np, "allwinner,sun50i-sdmmc0")){
  		host->sunxi_mmc_clk_set_rate = sunxi_mmc_clk_set_rate_for_sdmmc_01;
		//host->dma_tl = (0x2<<28)|(15<<16)|240;
		host->dma_tl = (0x2<<28)|(7<<16)|248;
		host->sunxi_mmc_thld_ctl = sunxi_mmc_thld_ctl_for_sdmmc_01;
		host->sunxi_mmc_save_spec_reg = sunxi_mmc_save_spec_reg_01;
		host->sunxi_mmc_restore_spec_reg = sunxi_mmc_save_spec_reg_01;		
		sunxi_mmc_reg_ex_res_inter(host,0);
 	}else if(of_device_is_compatible(np, "allwinner,sun50i-sdmmc1")){
 		host->sunxi_mmc_clk_set_rate = sunxi_mmc_clk_set_rate_for_sdmmc_01;
		host->dma_tl = (0x3<<28)|(15<<16)|240;
		host->sunxi_mmc_thld_ctl = sunxi_mmc_thld_ctl_for_sdmmc_01;
		host->sunxi_mmc_save_spec_reg = sunxi_mmc_save_spec_reg_01;
		host->sunxi_mmc_restore_spec_reg = sunxi_mmc_save_spec_reg_01;		
		sunxi_mmc_reg_ex_res_inter(host,1);
 	}else{
 		host->sunxi_mmc_clk_set_rate = NULL;
		host->dma_tl = NULL;
		host->sunxi_mmc_thld_ctl = NULL;
		host->sunxi_mmc_save_spec_reg = NULL;
		host->sunxi_mmc_restore_spec_reg = NULL;
 	}


	ret = mmc_regulator_get_supply(host->mmc);
	if (ret) {
		if(ret == -EPROBE_DEFER){
			dev_err(&pdev->dev, "Power supply is not ready\n");
			return ret;
		}else{
			dev_err(&pdev->dev, "Could not get power supply,used default ocr\n");
		}		
	}
	if (!host->mmc->ocr_avail)
			host->mmc->ocr_avail = MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 \
									| MMC_VDD_31_32| MMC_VDD_32_33 | MMC_VDD_33_34;

	//enable card detect pin power
	if (!IS_ERR(host->mmc->supply.vdmmc)) {
		ret = regulator_enable(host->mmc->supply.vdmmc);
		if (ret < 0){
			dev_err(mmc_dev(host->mmc),
				"failed to enable vdmmc regulator\n");
			return ret;
		}
	}


	host->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(host->pinctrl)) {
		ret = PTR_ERR(host->pinctrl);
		goto error_disable_vdmmc;
	}

	host->pins_default = pinctrl_lookup_state(host->pinctrl,
			PINCTRL_STATE_DEFAULT);
	if (IS_ERR(host->pins_default)) {
		dev_err(&pdev->dev, "could not get default pinstate\n");
		ret = PTR_ERR(host->pins_default);
		goto error_disable_vdmmc;
	}

	host->pins_sleep = pinctrl_lookup_state(host->pinctrl,
			PINCTRL_STATE_SLEEP);
	if (IS_ERR(host->pins_sleep)) {
		dev_err(&pdev->dev, "could not get sleep pinstate\n");
		ret = PTR_ERR(host->pins_sleep);
		goto error_disable_vdmmc;
	}


	host->reg_base = devm_ioremap_resource(&pdev->dev,
			      platform_get_resource(pdev, IORESOURCE_MEM, 0));
	if (IS_ERR(host->reg_base))
		return PTR_ERR(host->reg_base);

#ifndef USE_OLD_SYS_CLK_INTERFACE
	host->clk_ahb = devm_clk_get(&pdev->dev, "ahb");
	if (IS_ERR(host->clk_ahb)) {
		dev_err(&pdev->dev, "Could not get ahb clock\n");
		return PTR_ERR(host->clk_ahb);
	}
#endif

	host->clk_mmc = devm_clk_get(&pdev->dev, "mmc");
	if (IS_ERR(host->clk_mmc)) {
		dev_err(&pdev->dev, "Could not get mmc clock\n");
		return PTR_ERR(host->clk_mmc);
	}

#ifndef USE_OLD_SYS_CLK_INTERFACE
	host->reset = devm_reset_control_get(&pdev->dev, "ahb");

	ret = clk_prepare_enable(host->clk_ahb);
	if (ret) {
		dev_err(&pdev->dev, "Enable ahb clk err %d\n", ret);
		return ret;
	}
#endif

	ret = clk_prepare_enable(host->clk_mmc);
	if (ret) {
		dev_err(&pdev->dev, "Enable mmc clk err %d\n", ret);
		goto error_disable_clk_ahb;
	}

#ifndef USE_OLD_SYS_CLK_INTERFACE
	if (!IS_ERR(host->reset)) {
		ret = reset_control_deassert(host->reset);
		if (ret) {
			dev_err(&pdev->dev, "reset err %d\n", ret);
			goto error_disable_clk_mmc;
		}
	}
#endif

	/*
	 * Sometimes the controller asserts the irq on boot for some reason,
	 * make sure the controller is in a sane state before enabling irqs.
	 */
	ret = sunxi_mmc_reset_host(host);
	if (ret)
		goto error_assert_reset;

	host->irq = platform_get_irq(pdev, 0);
	ret = devm_request_threaded_irq(&pdev->dev, host->irq, sunxi_mmc_irq,
			sunxi_mmc_handle_manual_stop, 0, "sunxi-mmc", host);
	if(ret){
		dev_err(&pdev->dev, "faild to request irq %d\n", ret);
		goto error_assert_reset;
	}

	disable_irq(host->irq);

#ifndef USE_OLD_SYS_CLK_INTERFACE
	if (!IS_ERR(host->reset))
		reset_control_assert(host->reset);
#endif
	clk_disable_unprepare(host->clk_mmc);
#ifndef USE_OLD_SYS_CLK_INTERFACE
	clk_disable_unprepare(host->clk_ahb);
#endif
	ret = mmc_create_sys_fs(host,pdev);
	if(ret)
		goto error_disable_vdmmc;

	return ret;

error_assert_reset:
#ifndef USE_OLD_SYS_CLK_INTERFACE
	if (!IS_ERR(host->reset))
		reset_control_assert(host->reset);
#endif
error_disable_clk_mmc:
	clk_disable_unprepare(host->clk_mmc);
error_disable_clk_ahb:
	clk_disable_unprepare(host->clk_ahb);
error_disable_vdmmc:
	if (!IS_ERR(host->mmc->supply.vdmmc))
			regulator_disable(host->mmc->supply.vdmmc);
	return ret;
}

static int sunxi_mmc_probe(struct platform_device *pdev)
{
	struct sunxi_mmc_host *host;
	struct mmc_host *mmc;
	int ret;

	mmc = mmc_alloc_host(sizeof(struct sunxi_mmc_host), &pdev->dev);
	if (!mmc) {
		dev_err(&pdev->dev, "mmc alloc host failed\n");
		return -ENOMEM;
	}

	host = mmc_priv(mmc);
	host->mmc = mmc;
	spin_lock_init(&host->lock);

	ret = sunxi_mmc_resource_request(host, pdev);
	if (ret)
		goto error_free_host;

	host->sg_cpu = dma_alloc_coherent(&pdev->dev, PAGE_SIZE,
					  &host->sg_dma, GFP_KERNEL);
	if (!host->sg_cpu) {
		dev_err(&pdev->dev, "Failed to allocate DMA descriptor mem\n");
		ret = -ENOMEM;
		goto error_free_host;
	}

	mmc->ops		= &sunxi_mmc_ops;
	mmc->max_blk_count	= 8192;
	mmc->max_blk_size	= 4096;
	mmc->max_segs		= PAGE_SIZE / sizeof(struct sunxi_idma_des);
	mmc->max_seg_size	= (1 << host->idma_des_size_bits);
	mmc->max_req_size	= mmc->max_seg_size * mmc->max_segs;
	/* 400kHz ~ 50MHz */
	mmc->f_min		=   400000;
	mmc->f_max		= 50000000;
	mmc->caps	       |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED |
				  MMC_CAP_ERASE;
	//mmc->caps2	  |= MMC_CAP2_HS400_1_8V;


#ifndef CONFIG_REGULATOR
	//Because fpga has no regulator,so we add it manully
	mmc->ocr_avail = MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32
				| MMC_VDD_32_33 | MMC_VDD_33_34;
	dev_info(&pdev->dev,"*******************set host ocr**************************\n");

#endif

	mmc_of_parse(mmc);

	ret = mmc_add_host(mmc);
	if (ret)
		goto error_free_dma;

	dev_info(&pdev->dev, "base:0x%p irq:%u\n", host->reg_base, host->irq);
	platform_set_drvdata(pdev, mmc);
	return 0;

error_free_dma:
	dma_free_coherent(&pdev->dev, PAGE_SIZE, host->sg_cpu, host->sg_dma);
error_free_host:
	mmc_free_host(mmc);
	return ret;
}

static int sunxi_mmc_remove(struct platform_device *pdev)
{
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	mmc_remove_host(mmc);
	disable_irq(host->irq);
	sunxi_mmc_reset_host(host);

	mmc_remove_sys_fs(host,pdev);

	if (!IS_ERR(mmc->supply.vdmmc))
			regulator_disable(mmc->supply.vdmmc);

	dma_free_coherent(&pdev->dev, PAGE_SIZE, host->sg_cpu, host->sg_dma);
	mmc_free_host(mmc);

	return 0;
}


#ifdef CONFIG_PM

void sunxi_mmc_regs_save(struct sunxi_mmc_host* host)
{
	struct sunxi_mmc_ctrl_regs* bak_regs = &host->bak_regs;

	/*save public register*/
	bak_regs->gctrl		= mmc_readl(host, REG_GCTRL);
	bak_regs->clkc		= mmc_readl(host, REG_CLKCR);
	bak_regs->timeout	= mmc_readl(host, REG_TMOUT);
	bak_regs->buswid	= mmc_readl(host, REG_WIDTH);
	bak_regs->waterlvl	= mmc_readl(host, REG_FTRGL);
	bak_regs->funcsel	= mmc_readl(host, REG_FUNS);
	bak_regs->debugc	= mmc_readl(host, REG_DBGC);
	bak_regs->idmacc	= mmc_readl(host, REG_DMAC);
	bak_regs->dlba		= mmc_readl(host, REG_DLBA);
	bak_regs->imask		= mmc_readl(host, REG_IMASK);

	//to do SMHC_A12A

	if(host->sunxi_mmc_save_spec_reg)
		host->sunxi_mmc_save_spec_reg(host);
}

void sunxi_mmc_regs_restore(struct sunxi_mmc_host* host)
{
	struct sunxi_mmc_ctrl_regs* bak_regs = &host->bak_regs;

	/*restore public register*/
	mmc_writel(host, REG_GCTRL, bak_regs->gctrl   );
	mmc_writel(host, REG_CLKCR, bak_regs->clkc    );
	mmc_writel(host, REG_TMOUT, bak_regs->timeout );
	mmc_writel(host, REG_WIDTH, bak_regs->buswid  );
	mmc_writel(host, REG_FTRGL, bak_regs->waterlvl);
	mmc_writel(host, REG_FUNS , bak_regs->funcsel );
	mmc_writel(host, REG_DBGC , bak_regs->debugc  );
	mmc_writel(host, REG_DMAC , bak_regs->idmacc  );
	mmc_writel(host, REG_DLBA , bak_regs->dlba	);
	mmc_writel(host, REG_IMASK , bak_regs->dlba	);

	//to do SMHC_A12A

	if(host->sunxi_mmc_restore_spec_reg)
		host->sunxi_mmc_restore_spec_reg(host);
}



static int sunxi_mmc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	int ret = 0;

	if (mmc) {
		ret = mmc_suspend_host(mmc);
		if(!ret){
			if (!IS_ERR(mmc->supply.vdmmc))
					regulator_disable(mmc->supply.vdmmc);

			if(mmc_card_keep_power(mmc)){
				struct sunxi_mmc_host *host = mmc_priv(mmc);
				sunxi_mmc_regs_save(host);
#ifndef USE_OLD_SYS_CLK_INTERFACE
				if (!IS_ERR(host->reset))
					reset_control_assert(host->reset);
#endif
				clk_disable_unprepare(host->clk_mmc);
#ifndef USE_OLD_SYS_CLK_INTERFACE
				clk_disable_unprepare(host->clk_ahb);
#endif
				rval = pinctrl_select_state(host->pinctrl, host->pins_sleep);
				if (rval)
					dev_warn(mmc_dev(mmc), "could not set sleep pins\n");				
			}
		}
      }

	return ret;
}


static int sunxi_mmc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	int ret = 0;

	if (mmc) {
		if (mmc_card_keep_power(mmc)){
			struct sunxi_mmc_host *host = mmc_priv(mmc);
			rval = pinctrl_select_state(host->pinctrl, host->pins_default);
			if (rval)
				dev_warn(mmc_dev(mmc), "could not set default pins\n");
#ifndef USE_OLD_SYS_CLK_INTERFACE
			rval = clk_prepare_enable(host->clk_ahb);
			if (rval) {
				dev_err(mmc_dev(mmc), "Enable ahb clk err %d\n", rval);
				return;
			}
#endif
			rval = clk_prepare_enable(host->clk_mmc);
			if (rval) {
				dev_err(mmc_dev(mmc), "Enable mmc clk err %d\n", rval);
				return;
			}
#ifndef USE_OLD_SYS_CLK_INTERFACE
			if (!IS_ERR(host->reset)) {
				rval = reset_control_deassert(host->reset);
				if (rval) {
					dev_err(mmc_dev(mmc), "reset err %d\n", rval);
					return;
				}
			}			
#endif
			sunxi_mmc_regs_restore(host);
			host->ferror = sunxi_mmc_update_clk(host);
		}

		//enable card detect pin power
		if (!IS_ERR(mmc->supply.vdmmc)) {
			ret = regulator_enable(mmc->supply.vdmmc);
			if (ret < 0)
				dev_err(mmc_dev(mmc),
					"failed to enable vdmmc regulator\n");
			return ret;
		}
		ret = mmc_resume_host(mmc);
	}

	return ret;
}


static const struct dev_pm_ops sunxi_mmc_pm = {
	.suspend	= sunxi_mci_suspend,
	.resume		= sunxi_mci_resume,
};
#define sunxi_mmc_pm_ops &sunxi_mmc_pm

#else /* CONFIG_PM */

#define sunxi_mmc_pm_ops NULL

#endif /* CONFIG_PM */



static struct platform_driver sunxi_mmc_driver = {
	.driver = {
		.name	= "sunxi-mmc",
		.of_match_table = of_match_ptr(sunxi_mmc_of_match),
		.pm = sunxi_mmc_pm_ops,
	},
	.probe		= sunxi_mmc_probe,
	.remove		= sunxi_mmc_remove,
};
module_platform_driver(sunxi_mmc_driver);

MODULE_DESCRIPTION("Allwinner's SD/MMC Card Controller Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("David Lanzend�rfer <david.lanzendoerfer@o2s.ch>");
MODULE_ALIAS("platform:sunxi-mmc");
