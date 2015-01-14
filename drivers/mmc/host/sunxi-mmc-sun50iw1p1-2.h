#ifndef SUNXI_MMC_SUN50IW1P1_2_H
#define SUNXI_MMC_SUN50IW1P1_2_H

//reg
#define SDXC_REG_EDSD	(0x010C)    /*SMHC eMMC4.5 DDR Start Bit Detection Control Register*/
#define SDXC_REG_CSDC	(0x0054)    /*SMHC CRC Status Detect Control Register*/
#define SDXC_REG_THLD	(0x0100)    /*SMHC Card Threshold Control Register*/
//bit
#define SDXC_HS400_MD_EN				(1U<<31)
#define SDXC_CARD_WR_THLD_ENB		(1U<<2)
#define SDXC_CARD_RD_THLD_ENB		(1U)

//mask
#define SDXC_CRC_DET_PARA_MASK		(0xf)
#define SDXC_CARD_RD_THLD_MASK		(0x0FFF0000)
#define SDXC_TX_TL_MASK				(0xff)
#define SDXC_RX_TL_MASK				(0x00FF0000)

//value
#define SDXC_CRC_DET_PARA_HS400		(6)
#define SDXC_CRC_DET_PARA_OTHER		(3)
#define SDXC_FIFO_DETH					(1024>>2)

//size
#define SDXC_CARD_RD_THLD_SIZE		(0x00000FFF)

//shit
#define SDXC_CARD_RD_THLD_SIZE_SHIFT		(16)

extern int sunxi_mmc_clk_set_rate_for_sdmmc2(struct sunxi_mmc_host *host,
				  struct mmc_ios *ios);
extern void sunxi_mmc_thld_ctl_for_sdmmc2(struct sunxi_mmc_host *host,
			  struct mmc_ios *ios, struct mmc_data *data)	;
#endif
