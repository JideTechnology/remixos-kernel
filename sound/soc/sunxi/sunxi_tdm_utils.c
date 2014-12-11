/*
 * sound\soc\sunxi\sunxi_tdm_utils.c
 * (C) Copyright 2014-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>
#include <linux/pinctrl/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <asm/dma.h>
#include <linux/gpio.h>
#include "sunxi_dma.h"
#include "sunxi_tdm_utils.h"

int txctrl_tdm(int on,int hub_en,struct sunxi_tdm_info *sunxi_tdm)
{
	u32 reg_val;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	/*flush TX FIFO*/
	reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
	reg_val |= SUNXI_DAUDIOFCTL_FTX;
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);
	/*clear TX counter*/
	writel(0, tdm->regs + SUNXI_DAUDIOTXCNT);

	if (on) {
		/* enable DMA DRQ mode for play */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOINT);
		reg_val |= SUNXI_DAUDIOINT_TXDRQEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOINT);
	} else {
		/* DISBALE dma DRQ mode */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOINT);
		reg_val &= ~SUNXI_DAUDIOINT_TXDRQEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOINT);
	}
	if (hub_en) {
		reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
		reg_val |= SUNXI_DAUDIOFCTL_HUBEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);
	} else {
		reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
		reg_val &= ~SUNXI_DAUDIOFCTL_HUBEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);
	}

#ifdef CONFIG_SUNXI_AUDIO_DEBUG
	for(reg_val = 0; reg_val < 0x5c; reg_val=reg_val+4)
		pr_debug("%s,line:%d,0x%x:%x\n",__func__,__LINE__,reg_val,readl(tdm->regs + reg_val));
#endif
	return 0;
}
EXPORT_SYMBOL(txctrl_tdm);
int  rxctrl_tdm(int on,struct sunxi_tdm_info *sunxi_tdm)
{
	u32 reg_val;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	/*flush RX FIFO*/
	reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
	reg_val |= SUNXI_DAUDIOFCTL_FRX;
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);
	/*clear RX counter*/
	writel(0, tdm->regs + SUNXI_DAUDIORXCNT);

	if (on) {
		/* enable DMA DRQ mode for record */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOINT);
		reg_val |= SUNXI_DAUDIOINT_RXDRQEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOINT);
	} else {
		/* DISBALE dma DRQ mode */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOINT);
		reg_val &= ~SUNXI_DAUDIOINT_RXDRQEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOINT);
	}
#ifdef CONFIG_SUNXI_AUDIO_DEBUG
	for(reg_val = 0; reg_val < 0x58; reg_val=reg_val+4)
		pr_debug("%s,line:%d,0x%x:%x\n",__func__,__LINE__,reg_val,readl(tdm->regs + reg_val));
#endif
	return 0;
}
EXPORT_SYMBOL(rxctrl_tdm);
int tdm_set_fmt(unsigned int fmt,struct sunxi_tdm_info *sunxi_tdm)
{
	u32 reg_val = 0;
	u32 reg_val1 = 0;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	/* master or slave selection */
	reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK){
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master, ap is slave*/
			reg_val &= ~SUNXI_DAUDIOCTL_LRCKOUT;
			reg_val &= ~SUNXI_DAUDIOCTL_BCLKOUT;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave,ap is master*/
			reg_val |= SUNXI_DAUDIOCTL_LRCKOUT;
			reg_val |= SUNXI_DAUDIOCTL_BCLKOUT;
			break;
		default:
			pr_err("unknwon master/slave format\n");
			return -EINVAL;
	}
	writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
	/* pcm or tdm mode selection */
	reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
	reg_val1 = readl(tdm->regs + SUNXI_DAUDIOTX0CHSEL);
	reg_val &= ~SUNXI_DAUDIOCTL_MODESEL;
	reg_val1 &= ~SUNXI_DAUDIOTXn_OFFSET;
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:        /* i2s mode */
			reg_val  |= (1<<4);
			reg_val1 |= (1<<12);
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val  |= (2<<4);
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val  |= (1<<4);
			reg_val1  |= (0<<12);
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L data msb after FRM LRC */
			reg_val  |= (0<<4);
			reg_val1  |= (1<<12);
			break;
		case SND_SOC_DAIFMT_DSP_B:      /* L data msb during FRM LRC */
			reg_val  |= (0<<4);
			reg_val1  |= (0<<12);
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
	writel(reg_val1, tdm->regs + SUNXI_DAUDIOTX0CHSEL);
	/* DAI signal inversions */
	reg_val1 = readl(tdm->regs + SUNXI_DAUDIOFAT0);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + frame */
			reg_val1 &= ~SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			reg_val1 &= ~SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val1 |= SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			reg_val1 &= ~SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val1 &= ~SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			reg_val1 |= SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + frm */
			reg_val1 |= SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			reg_val1 |= SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			break;
	}
	writel(reg_val1, tdm->regs + SUNXI_DAUDIOFAT0);
	return 0;
}
EXPORT_SYMBOL(tdm_set_fmt);

int tdm_hw_params(struct snd_pcm_hw_params *params,struct sunxi_tdm_info *sunxi_tdm)
{
	u32 reg_val = 0;
	//u32 sample_resolution = 0;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;

	switch (params_format(params))
	{
	case SNDRV_PCM_FORMAT_S16_LE:
		tdm->samp_res = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		tdm->samp_res = 24;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		tdm->samp_res = 24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		tdm->samp_res = 24;
		break;
	default:
		return -EINVAL;
	}
	reg_val = readl(tdm->regs + SUNXI_DAUDIOFAT0);
	reg_val &= ~SUNXI_DAUDIOFAT0_SR;
	if(tdm->samp_res == 16)
		reg_val |= (3<<4);
	else if(tdm->samp_res == 20)
		reg_val |= (4<<4);
	else
		reg_val |= (5<<4);
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFAT0);

	if (tdm->samp_res == 24) {
		reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
		reg_val &= ~SUNXI_DAUDIOFCTL_TXIM;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);
	} else {
		reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
		reg_val |= SUNXI_DAUDIOFCTL_TXIM;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);
	}
	return 0;
}
EXPORT_SYMBOL(tdm_hw_params);
int tdm_trigger(struct snd_pcm_substream *substream,int cmd, struct sunxi_tdm_info *sunxi_tdm)
{
	s32 ret = 0;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				rxctrl_tdm(1,tdm);
			} else {
				txctrl_tdm(1,0,tdm);
			}
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				rxctrl_tdm(0,tdm);
			} else {
			  	txctrl_tdm(0,0,tdm);
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}
EXPORT_SYMBOL(tdm_trigger);
int tdm_set_sysclk(unsigned int freq,struct sunxi_tdm_info *sunxi_tdm)
{
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	if (clk_set_rate(tdm->tdm_pllclk, freq)) {
		pr_err("try to set the tdm_pll2clk failed!\n");
	}
	return 0;
}
EXPORT_SYMBOL(tdm_set_sysclk);

int tdm_set_clkdiv(int sample_rate,struct sunxi_tdm_info *sunxi_tdm)
{
	u32 reg_val = 0;
	u32 mclk_div = 0;
	u32 bclk_div = 0;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	/*enable mclk*/
	reg_val = readl(tdm->regs + SUNXI_DAUDIOCLKD);
	reg_val |= SUNXI_DAUDIOCLKD_MCLKOEN;

	/*i2s mode*/
	if (tdm->tdm_config) {
		switch (sample_rate) {
			case 192000:
			case 96000:
			case 48000:
			case 32000:
			case 24000:
			case 12000:
			case 16000:
			case 8000:
				bclk_div = ((24576000/sample_rate)/(2*tdm->pcm_lrck_period));
				mclk_div = 1;
			break;
			default:
				bclk_div = ((22579200/sample_rate)/(2*tdm->pcm_lrck_period));
				mclk_div = 1;
			break;
		}
	} else {/*pcm mode*/
		bclk_div = ((24576000/sample_rate)/(tdm->pcm_lrck_period));
		mclk_div = 1;//((24576000/sample_rate)/mclk_fs);
	}

	switch(mclk_div)
	{
		case 1: mclk_div = 1;
			break;
		case 2: mclk_div = 2;
			break;
		case 4: mclk_div = 3;
			break;
		case 6: mclk_div = 4;
			break;
		case 8: mclk_div = 5;
			break;
		case 12: mclk_div = 6;
			 break;
		case 16: mclk_div = 7;
			 break;
		case 24: mclk_div = 8;
			 break;
		case 32: mclk_div = 9;
			 break;
		case 48: mclk_div = 10;
			 break;
		case 64: mclk_div = 11;
			 break;
		case 96: mclk_div = 12;
			 break;
		case 128: mclk_div = 13;
			 break;
		case 176: mclk_div = 14;
			 break;
		case 192: mclk_div = 15;
			 break;
	}

	reg_val &= ~(0xf<<0);
	reg_val |= mclk_div<<0;
	switch(bclk_div)
	{
		case 1: bclk_div = 1;
			break;
		case 2: bclk_div = 2;
			break;
		case 4: bclk_div = 3;
			break;
		case 6: bclk_div = 4;
			break;
		case 8: bclk_div = 5;
			break;
		case 12: bclk_div = 6;
			break;
		case 16: bclk_div = 7;
			break;
		case 24: bclk_div = 8;
			break;
		case 32: bclk_div = 9;
			break;
		case 48: bclk_div = 10;
			break;
		case 64: bclk_div = 11;
			break;
		case 96: bclk_div = 12;
			break;
		case 128: bclk_div = 13;
			break;
		case 176: bclk_div = 14;
			break;
		case 192:bclk_div = 15;
	}
	reg_val &= ~(0xf<<4);
	reg_val |= bclk_div<<4;
	writel(reg_val, tdm->regs + SUNXI_DAUDIOCLKD);

	reg_val = readl(tdm->regs + SUNXI_DAUDIOFAT0);
	reg_val &= ~(0x3ff<<20);
	reg_val &= ~(0x3ff<<8);
	reg_val |= (tdm->pcm_lrck_period-1)<<8;
	reg_val |= (tdm->pcm_lrckr_period-1)<<20;
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFAT0);

	reg_val = readl(tdm->regs + SUNXI_DAUDIOFAT0);
	reg_val &= ~SUNXI_DAUDIOFAT0_SW;
	if(tdm->slot_width_select == 16)
		reg_val |= (3<<0);
	else if(tdm->slot_width_select == 20)
		reg_val |= (4<<0);
	else if(tdm->slot_width_select == 24)
		reg_val |= (5<<0);
	else if(tdm->slot_width_select == 32)
		reg_val |= (7<<0);
	else
		reg_val |= (1<<0);
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFAT0);


	reg_val = readl(tdm->regs + SUNXI_DAUDIOFAT1);
	reg_val |= tdm->pcm_lsb_first<<7;
	reg_val |= tdm->pcm_lsb_first<<6;
	/*linear or u/a-law*/
	reg_val &= ~(0xf<<0);
	reg_val |= (tdm->tx_data_mode)<<2;
	reg_val |= (tdm->rx_data_mode)<<0;
	if(tdm->frametype)/*pcm mode*/
		reg_val |= SUNXI_DAUDIOFAT0_LRCK_WIDTH;
	else
		reg_val &= ~SUNXI_DAUDIOFAT0_LRCK_WIDTH;
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFAT1);

	return 0;
}
EXPORT_SYMBOL(tdm_set_clkdiv);
int tdm_perpare(struct snd_pcm_substream *substream,
					struct sunxi_tdm_info *sunxi_tdm)
{
	u32 reg_val;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	/* play or record */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg_val = readl(tdm->regs + SUNXI_TXCHCFG);
		if (substream->runtime->channels == 1) {
			reg_val &= ~(0x7<<0);
			reg_val |= (0x0)<<0;
		} else {
			reg_val &= ~(0x7<<0);
			reg_val |= (0x1)<<0;
		}
		writel(reg_val, tdm->regs + SUNXI_TXCHCFG);
		reg_val = readl(tdm->regs + SUNXI_DAUDIOTX0CHSEL);
		reg_val |= (0x1<<12);
		reg_val &= ~(0xff<<4);
		reg_val &= ~(0x7<<0);
		if (substream->runtime->channels == 1) {
			reg_val |= (0x3<<4);
			reg_val	|= (0x1<<0);
		} else {
			reg_val |= (0x3<<4);
			reg_val	|= (0x1<<0);
		}
		writel(reg_val, tdm->regs + SUNXI_DAUDIOTX0CHSEL);
		reg_val = readl(tdm->regs + SUNXI_DAUDIOTX0CHMAP);
		reg_val = 0;
		if(substream->runtime->channels == 1) {
			reg_val = 0x0;
		} else {
			reg_val = 0x10;
		}
		writel(reg_val, tdm->regs + SUNXI_DAUDIOTX0CHMAP);

		reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
		reg_val &= ~SUNXI_DAUDIOCTL_SDO3EN;
		reg_val &= ~SUNXI_DAUDIOCTL_SDO2EN;
		reg_val &= ~SUNXI_DAUDIOCTL_SDO1EN;
		reg_val &= ~SUNXI_DAUDIOCTL_SDO0EN;
		switch (substream->runtime->channels) {
			case 1:
			case 2:
				reg_val |= SUNXI_DAUDIOCTL_SDO0EN;
				break;
			case 3:
			case 4:
				reg_val |= SUNXI_DAUDIOCTL_SDO0EN | SUNXI_DAUDIOCTL_SDO1EN;
				break;
			case 5:
			case 6:
				reg_val |= SUNXI_DAUDIOCTL_SDO0EN | SUNXI_DAUDIOCTL_SDO1EN | SUNXI_DAUDIOCTL_SDO2EN;
				break;
			case 7:
			case 8:
				reg_val |= SUNXI_DAUDIOCTL_SDO0EN | SUNXI_DAUDIOCTL_SDO1EN | SUNXI_DAUDIOCTL_SDO2EN | SUNXI_DAUDIOCTL_SDO3EN;
				break;
			default:
				reg_val |= SUNXI_DAUDIOCTL_SDO0EN;
				break;
		}
		writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
		/*clear TX counter*/
		writel(0, tdm->regs + SUNXI_DAUDIOTXCNT);

		/* DAUDIO TX ENABLE */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
		reg_val |= SUNXI_DAUDIOCTL_TXEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
	} else {
		reg_val = readl(tdm->regs + SUNXI_TXCHCFG);
		if (substream->runtime->channels == 1) {
			reg_val &= ~(0x7<<4);
			reg_val |= (0x0)<<4;
		} else {
			reg_val &= ~(0x7<<4);
			reg_val |= (0x1)<<4;
		}
		writel(reg_val, tdm->regs + SUNXI_TXCHCFG);

		reg_val = readl(tdm->regs + SUNXI_DAUDIORXCHSEL);
		reg_val |= (0x1<<12);
		if(substream->runtime->channels == 1) {
			reg_val	&= ~(0x1<<0);
		} else {
			reg_val	|= (0x1<<0);
		}
		writel(reg_val, tdm->regs + SUNXI_DAUDIORXCHSEL);

		reg_val = readl(tdm->regs + SUNXI_DAUDIORXCHMAP);
		reg_val = 0;
		if (substream->runtime->channels == 1) {
			reg_val = 0x00;
		} else {
			reg_val = 0x10;
		}
		writel(reg_val, tdm->regs + SUNXI_DAUDIORXCHMAP);
		reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
		reg_val |= SUNXI_DAUDIOFCTL_RXOM;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);

		/*clear RX counter*/
		writel(0, tdm->regs + SUNXI_DAUDIORXCNT);

		/* DAUDIO RX ENABLE */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
		reg_val |= SUNXI_DAUDIOCTL_RXEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
	}
	return 0;
}
EXPORT_SYMBOL(tdm_perpare);

int tdm_global_enable(struct sunxi_tdm_info *sunxi_tdm,bool on)
{
	u32 reg_val;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
	if (!on)
		reg_val &= ~SUNXI_DAUDIOCTL_GEN;
	else
		reg_val |= SUNXI_DAUDIOCTL_GEN;

	writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
	return 0;
}
EXPORT_SYMBOL(tdm_global_enable);
MODULE_LICENSE("GPL");

