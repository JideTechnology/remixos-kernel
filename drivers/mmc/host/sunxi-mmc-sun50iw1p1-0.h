#ifndef __SUNXI_MMC_SUN50IW1P1_0_H__
#define __SUNXI_MMC_SUN50IW1P1_0_H__


extern int sunxi_mmc_clk_set_rate_for_sdmmc0(struct sunxi_mmc_host *host,
				  struct mmc_ios *ios);
extern void sunxi_mmc_thld_ctl_for_sdmmc0(struct sunxi_mmc_host *host,
			  struct mmc_ios *ios, struct mmc_data *data);

void sunxi_mmc_save_spec_reg0(struct sunxi_mmc_host *host);
void sunxi_mmc_restore_spec_reg0(struct sunxi_mmc_host *host);
#endif
