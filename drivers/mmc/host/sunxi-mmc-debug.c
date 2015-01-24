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


#include "sunxi-mmc.h"
#include "sunxi-mmc-debug.h"

void sunxi_mmc_dumphex32(struct sunxi_mmc_host* host, char* name, char* base, int len)
{
	u32 i;
	printk("dump %s registers:", name);
	for (i=0; i<len; i+=4) {
		if (!(i&0xf))
			printk("\n0x%p : ", base + i);
		printk("0x%08x ",__raw_readl(host->reg_base+i));
	}
	printk("\n");
}

void sunxi_mmc_dump_des(struct sunxi_mmc_host* host, char* base, int len)
{
	u32 i;
	printk("dump des mem\n");
	for (i=0; i<len; i+=4) {
		if (!(i&0xf))
			printk("\n0x%p : ", base + i);
		printk("0x%08x ",*(u32 *)(base+i));
	}
	printk("\n");
}
