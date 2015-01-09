#include <linux/clk.h>
#include <linux/clk-private.h>
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

#include <linux/mmc/host.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/slot-gpio.h>


#include "sunxi-mmc.h"
#include "sunxi-mmc-sun50iw1p1-2.h"


//???? do not use lower power mode at all
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
		   mmc_readl(host, REG_RINTR) & ~SDXC_SDIO_INTERRUPT);//????

	if (rval & SDXC_START) {
		dev_err(mmc_dev(host->mmc), "fatal err update clk timeout\n");
		return -EIO;
	}

	return 0;
}

int sunxi_mmc_clk_set_rate_for_sdmmc2(struct sunxi_mmc_host *host,
				  struct mmc_ios *ios)
{
	u32 mod_clk = 0;
	u32 src_clk = 0;
	u32 rval 		= 0;
	s32 err 		= 0;
	u32 rate		= 0;
	char *sclk_name = NULL; 
	struct clk *mclk = host->clk_mmc;
	struct clk *sclk = NULL;
	struct device *dev = mmc_dev(host->mmc);
	int div = 0;

	if((ios->bus_width == MMC_BUS_WIDTH_8)\
		&&(ios->timing == MMC_TIMING_UHS_DDR50)\
	){
		mod_clk = ios->clock<<2;
		div = 1;
	}else{
		mod_clk = ios->clock<<1;
		div = 0;
	}
	
	if (ios->clock<= 400000) {
		//sclk = of_clk_get(np, 0);
		sclk = clk_get(dev,"osc24m");
		sclk_name = "osc24m";
	} else {
		//sclk = clk_get(np, 1);
		sclk = clk_get(dev,"pll_periph0");
		sclk_name = "pll_periph0";
	}
	if (IS_ERR(sclk)) {
		//SMC_ERR(smc_host, "Error to get source clock %s %d\n", sclk_name, (int)sclk);
		dev_err(mmc_dev(host->mmc), "Error to get source clock %s %d\n",sclk_name , (int)sclk);
		return -1;
	}

	sunxi_mmc_oclk_onoff(host, 0);
	
	err = clk_set_parent(mclk, sclk);
	if(err){
		dev_err(mmc_dev(host->mmc), "set parent failed\n");
		clk_put(sclk);
		return -1;		
	}

	rate = clk_round_rate(mclk, mod_clk);

	//SMC_DBG(smc_host, "sdc%d before set round clock %d\n", smc_host->host_id, rate);
	dev_err(mmc_dev(host->mmc),"get round rate %d\n", rate);

	err = clk_set_rate(mclk, rate);
	if (err) {
		//SMC_ERR(smc_host, "sdc%d set mclk rate error, rate %dHz\n",
		//				smc_host->host_id, rate);
		dev_err(mmc_dev(host->mmc),"set mclk rate error, rate %dHz\n",rate);				
		clk_put(sclk);
		return -1;
	}

	src_clk = clk_get_rate(sclk);
	clk_put(sclk);

	//SMC_DBG(smc_host, "sdc%d set round clock %d, soure clk is %d\n", smc_host->host_id, rate, src_clk);
	dev_err(mmc_dev(host->mmc),"set round clock %d, soure clk is %d\n", rate, src_clk);

#ifdef MMC_FPGA
	if((ios->bus_width == MMC_BUS_WIDTH_8)\
		&&(ios->timing == MMC_TIMING_UHS_DDR50)\
	){
		/* clear internal divider */
		rval = mmc_readl(host, REG_CLKCR);
		rval &= ~0xff;
		rval |= 1;		
	}else{
		/* support internal divide clock under fpga environment  */
		rval = mmc_readl(host, REG_CLKCR);
		rval &= ~0xff;
		rval |= 24000000 / mod_clk / 2; // =24M/400K/2=0x1E
	}
	mmc_writel(host, REG_CLKCR, rval);
	dev_err(mmc_dev(host->mmc), "--FPGA REG_CLKCR: 0x%08x \n", mmc_readl(host, REG_CLKCR));
#else
	/* clear internal divider */
	rval = mmc_readl(host, REG_CLKCR);
	rval &= ~0xff;
	rval |= div;
	mmc_writel(host, REG_CLKCR, rval);
#endif

	return sunxi_mmc_oclk_onoff(host, 1);
}


