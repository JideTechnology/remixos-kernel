#ifndef __SUNXI_I2S_H__
#define __SUNXI_I2S_H__
#include "sunxi_dma.h"
struct sun8iw10_cpudai {
	struct clk *pllclk;
	struct clk *moduleclk;
	struct sunxi_dma_params play_dma_param;
	struct sunxi_dma_params capture_dma_param;
	struct snd_soc_dai_driver dai;
};
#endif