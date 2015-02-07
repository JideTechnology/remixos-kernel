#ifndef __SUNXI_MMC_SUN50IW1P1_0_1_H__
#define __SUNXI_MMC_SUN50IW1P1_0_1_H__

//reg
#define SDXC_REG_EDSD		(0x010C)    /*SMHC eMMC4.5 DDR Start Bit Detection Control Register*/
#define SDXC_REG_CSDC		(0x0054)    /*SMHC CRC Status Detect Control Register*/
#define SDXC_REG_THLD		(0x0100)    /*SMHC Card Threshold Control Register*/
#define SDXC_REG_DRV_DL	 	(0x0140)    /*SMHC Drive Delay Control Register*/
#define SDXC_REG_SAMP_DL	(0x0144)	/*SMHC Sample Delay Control Register*/
#define SDXC_REG_DS_DL		(0x0148)	/*SMHC Data Strobe Delay Control Register*/
#define SDXC_REG_SD_NTSR	(0x005C)	/*SMHC NewTiming Set Register*/




//bit
#define SDXC_HS400_MD_EN				(1U<<31)
#define SDXC_CARD_WR_THLD_ENB		(1U<<2)
#define SDXC_CARD_RD_THLD_ENB		(1U)

#define SDXC_DAT_DRV_PH_SEL			(1U<<17)
#define SDXC_CMD_DRV_PH_SEL			(1U<<16)
#define SDXC_SAMP_DL_SW_EN			(1u<<7)
#define SDXC_DS_DL_SW_EN			(1u<<7)


#define	SDXC_2X_TIMING_MODE			(1U<<31)


//mask
#define SDXC_CRC_DET_PARA_MASK		(0xf)
#define SDXC_CARD_RD_THLD_MASK		(0x0FFF0000)
#define SDXC_TX_TL_MASK				(0xff)
#define SDXC_RX_TL_MASK				(0x00FF0000)

#define SDXC_SAMP_DL_SW_MASK		(0x0000003F)
#define SDXC_DS_DL_SW_MASK			(0x0000003F)

#define SDXC_SAM_TIMING_PH_MASK		(0x00000030)

//value
#define SDXC_CRC_DET_PARA_HS400		(6)
#define SDXC_CRC_DET_PARA_OTHER		(3)
#define SDXC_FIFO_DETH					(1024>>2)

//size
#define SDXC_CARD_RD_THLD_SIZE		(0x00000FFF)

//shit
#define SDXC_CARD_RD_THLD_SIZE_SHIFT		(16)

#define SDXC_SAM_TIMING_PH_SHIFT			(4)

extern int sunxi_mmc_clk_set_rate_for_sdmmc_01(struct sunxi_mmc_host *host,
				  struct mmc_ios *ios);
extern void sunxi_mmc_thld_ctl_for_sdmmc_01(struct sunxi_mmc_host *host,
			  struct mmc_ios *ios, struct mmc_data *data);

void sunxi_mmc_save_spec_reg_01(struct sunxi_mmc_host *host);
void sunxi_mmc_restore_spec_reg_01(struct sunxi_mmc_host *host);
#endif
