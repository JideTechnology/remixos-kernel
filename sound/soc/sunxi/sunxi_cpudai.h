#ifndef __SUNXI_I2S_H__
#define __SUNXI_I2S_H__
#include "sunxi_dma.h"
#include "sunxi_rw_func.h"
#if defined(CONFIG_ARCH_SUN8IW10)
#include "sun8iw10_codec.h"
#elif defined(CONFIG_ARCH_SUN8IW11)
#include "sun8iw11_codec.h"
#endif

struct sunxi_cpudai {
	struct clk *pllclk;
	struct clk *moduleclk;
	struct sunxi_dma_params play_dma_param;
	struct sunxi_dma_params capture_dma_param;
	struct snd_soc_dai_driver dai;
};
#endif