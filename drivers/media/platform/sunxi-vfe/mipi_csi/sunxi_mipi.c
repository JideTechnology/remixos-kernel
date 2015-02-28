
/* 
 ***************************************************************************************
 * 
 * sunxi_mipi.c
 * 
 * Hawkview ISP - sunxi_mipi.c module
 * 
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 * 
 * Version		  Author         Date		    Description
 * 
 *   3.0		  Yang Feng   	2015/02/27	ISP Tuning Tools Support
 * 
 ****************************************************************************************
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "bsp_mipi_csi.h"
#include "sunxi_mipi.h"
#include "../vfe_os.h"
#include "../platform_cfg.h"
#define MIPI_MODULE_NAME "vfe_mipi"

static void mipi_release(struct device *dev)
{
	vfe_print("mipi_device_release\n");
};

static int mipi_probe(struct platform_device *pdev)
{
	struct mipi_dev *mipi = NULL;
	struct resource *res = NULL;
	struct mipi_platform_data *pdata = NULL;
	int ret = 0;
	if(pdev->dev.platform_data == NULL)
	{
		return -ENODEV;
	}
	pdata = pdev->dev.platform_data;
	vfe_print("mipi probe start mipi_sel = %d!\n",pdata->mipi_sel);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!request_mem_region(res->start, resource_size(res), res->name)) {
		return -ENOMEM;
	}
	mipi = kzalloc(sizeof(struct mipi_dev), GFP_KERNEL);
	if (!mipi) {
		ret = -ENOMEM;
		goto ekzalloc;
	}
	mipi->mipi_sel = pdata->mipi_sel;
	
	spin_lock_init(&mipi->slock);
	init_waitqueue_head(&mipi->wait);
	mipi->base = ioremap(res->start, resource_size(res));
	if (!mipi->base) {
		ret = -EIO;
		goto freedev;
	}

#if defined(CONFIG_ARCH_SUN8IW7P1)

#else
	if(mipi->mipi_sel == 0) {
#if defined (CONFIG_ARCH_SUN8IW6P1)
		bsp_mipi_csi_set_base_addr(mipi->mipi_sel, (unsigned long)mipi->base);
		bsp_mipi_dphy_set_base_addr(mipi->mipi_sel, (unsigned long)mipi->base + 0x1000);
#else
		ret = bsp_mipi_csi_set_base_addr(mipi->mipi_sel, 0);
		if(ret < 0)
			goto ehwinit;
		ret = bsp_mipi_dphy_set_base_addr(mipi->mipi_sel, 0);
		if(ret < 0)
			goto ehwinit;
#endif
	}
#endif

	platform_set_drvdata(pdev, mipi);
	vfe_print("mipi probe end mipi_sel = %d!\n",pdata->mipi_sel);
	return 0;

ehwinit:
	iounmap(mipi->base);
freedev:
	kfree(mipi);
ekzalloc:
	vfe_print("mipi probe err!\n");
	return ret;
}


static int mipi_remove(struct platform_device *pdev)
{
	struct mipi_dev *mipi = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, NULL);
	if(mipi->base)
		iounmap(mipi->base);
	kfree(mipi);
	return 0;
}

static struct resource mipi0_resource[] = 
{
	[0] = {
		.name	= "mipi",
		.start  = MIPI_CSI0_REGS_BASE,
		.end    = MIPI_CSI0_REGS_BASE + MIPI_CSI_REG_SIZE -1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct mipi_platform_data mipi0_pdata[] = {
	{
		.mipi_sel  = 0,
	},
};

static struct platform_device mipi_device[] = {
	[0] = {
		.name  = MIPI_MODULE_NAME,
		.id = 0,
		.num_resources = ARRAY_SIZE(mipi0_resource),
		.resource = mipi0_resource,
		.dev = {
			.platform_data  = mipi0_pdata,
			.release        = mipi_release,
		},
	},
};

static struct platform_driver mipi_platform_driver = {
	.probe    = mipi_probe,
	.remove   = mipi_remove,
	.driver = {
		.name   = MIPI_MODULE_NAME,
		.owner  = THIS_MODULE,
	}
};

int sunxi_mipi_platform_register(void)
{
	int ret,i;
	for(i=0; i<ARRAY_SIZE(mipi_device); i++) 
	{
		ret = platform_device_register(&mipi_device[i]);
		if (ret)
			vfe_err("mipi device %d register failed\n",i);
	}
	ret = platform_driver_register(&mipi_platform_driver);
	if (ret) {
		vfe_err("platform driver register failed\n");
		return ret;
	}
	vfe_print("mipi_init end\n");
	return 0;
}

void sunxi_mipi_platform_unregister(void)
{
	int i;
	vfe_print("mipi_exit start\n");
	for(i=0; i<ARRAY_SIZE(mipi_device); i++)
	{
		platform_device_unregister(&mipi_device[i]);
	}
	platform_driver_unregister(&mipi_platform_driver);
	vfe_print("mipi_exit end\n");
}

