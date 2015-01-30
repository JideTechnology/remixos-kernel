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
#include <linux/stat.h>


#include <linux/mmc/host.h>
#include "sunxi-mmc.h"
#include "sunxi-mmc-debug.h"
#include "sunxi-mmc-export.h"
#include "sunxi-mmc-sun50iw1p1-2.h"

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



static ssize_t maual_insert_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "Usage: \"echo 1 > insert\" to scan card\n");
	return ret;
}

static ssize_t maual_insert_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int ret;
	char *end;
	unsigned long insert = simple_strtoul(buf, &end, 0);
	struct platfrom_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	if (end == buf) {
		ret = -EINVAL;
		return ret;
	}

	dev_info(dev, "insert %ld\n", insert);
	if(insert)
		mmc_detect_change(mmc,0);
	else
		dev_info(dev, "no detect change\n");
	//sunxi_mmc_rescan_card(0);
	
	ret = count;
	return ret;
}

static ssize_t dump_register_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	char *p = buf;
	int i = 0;
	struct platfrom_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	p += sprintf(p, "Dump sdmmc regs:\n");
	for (i=0; i<0x180; i+=4) {
		if (!(i&0xf))
			p += sprintf(p, "\n0x%08x : ", (u32)(host->reg_base + i));
		p += sprintf(p, "%08x ", readl(host->reg_base + i));
	}
	p += sprintf(p, "\n");


	return p-buf;
}


static ssize_t dump_clk_dly_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	char *p = buf;
	struct platfrom_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	if(host->sunxi_mmc_dump_dly_table){
		host->sunxi_mmc_dump_dly_table(host);
	}else{
		dev_warn(mmc_dev(mmc),"not found the dump dly table\n");
	}

	return p-buf;
}



int mmc_create_sys_fs(struct sunxi_mmc_host* host,struct platform_device *pdev)
{
	int ret;

	host->maual_insert.show = maual_insert_show;
	host->maual_insert.store = maual_insert_store;
	sysfs_attr_init(host->maual_insert.attr);
	host->maual_insert.attr.name = "sunxi_insert";
	host->maual_insert.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(&pdev->dev, &host->maual_insert);
	if(ret)
		return ret;
	
	host->dump_register.show = dump_register_show;
	sysfs_attr_init(host->dump_register.attr);
	host->dump_register.attr.name = "sunxi_dump_regster";
	host->dump_register.attr.mode = S_IRUGO;
	ret = device_create_file(&pdev->dev, &host->dump_register);
	if(ret)
		return ret;


	host->dump_clk_dly.show = dump_clk_dly_show;
	sysfs_attr_init(host->dump_clk_dly.attr);
	host->dump_clk_dly.attr.name = "sunxi_dump_clk_dly";
	host->dump_clk_dly.attr.mode = S_IRUGO;
	ret = device_create_file(&pdev->dev, &host->dump_clk_dly);
	if(ret)
		return ret;

	
	return ret;
}

void mmc_remove_sys_fs(struct sunxi_mmc_host* host,struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &host->maual_insert);
	device_remove_file(&pdev->dev, &host->dump_register);
	device_remove_file(&pdev->dev, &host->dump_clk_dly);
}



