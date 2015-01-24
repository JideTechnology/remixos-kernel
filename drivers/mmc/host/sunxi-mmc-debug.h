#ifndef SUNXI_MMC_DEBUG_H
#define SUNXI_MMC_DEBUG_H
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

void sunxi_mmc_dumphex32(struct sunxi_mmc_host* host, char* name, char* base, int len);
void sunxi_mmc_dump_des(struct sunxi_mmc_host* host, char* base, int len);

#endif
