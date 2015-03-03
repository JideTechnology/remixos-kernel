
/* 
 ***************************************************************************************
 * 
 * sunxi_csi.c
 * 
 * Hawkview ISP - sunxi_csi.c module
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

#include "bsp_csi.h"
#include "sunxi_csi.h"
#include "../vfe_os.h"
#include "../platform_cfg.h"
#define CSI_MODULE_NAME "vfe_csi"

static int csi_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
	struct csi_dev *csi = NULL;
	struct csi_platform_data *pdata = NULL;
	int ret = 0;

	if (np == NULL) {
		vfe_err("CSI failed to get of node\n");
		return -ENODEV;
	}
	csi = kzalloc(sizeof(struct csi_dev), GFP_KERNEL);
	if (!csi) {
		ret = -ENOMEM;
		goto ekzalloc;
	}
	pdata = kzalloc(sizeof(struct csi_platform_data), GFP_KERNEL);
	if (pdata == NULL) {
        ret = -ENOMEM;
        goto freedev;
	}
	pdev->dev.platform_data = pdata;

	pdev->id = of_alias_get_id(np, "csi_res");
	if (pdev->id < 0) {
		vfe_err("CSI failed to get alias id\n");
		ret = -EINVAL;
		goto freepdata;
	}
	pdata->csi_sel = pdev->id;

 	csi->base = of_iomap(np, 0);
	if (!csi->base) {
		ret = -EIO;
		goto freepdata;
	}
	csi->csi_sel = pdata->csi_sel;	
	spin_lock_init(&csi->slock);
	init_waitqueue_head(&csi->wait);
	
	ret = bsp_csi_set_base_addr(csi->csi_sel, (unsigned long)csi->base);
	if(ret < 0)
		goto ehwinit;

	platform_set_drvdata(pdev, csi);
	vfe_print("csi probe end csi_sel = %d!\n",pdata->csi_sel);

	return 0;

ehwinit:
	iounmap(csi->base);
freepdata:
    kfree(pdata);
freedev:
	kfree(csi);
ekzalloc:
	vfe_print("csi probe err!\n");
	return ret;
}


static int csi_remove(struct platform_device *pdev)
{
	struct csi_dev *csi = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, NULL);
	free_irq(csi->irq, csi);
	if(csi->base)
		iounmap(csi->base);
	kfree(csi);
	kfree(pdev->dev.platform_data);
	return 0;
}

static const struct of_device_id sunxi_csi_match[] = {
	{ .compatible = "allwinner,sunxi-csi", },
	{},
};

MODULE_DEVICE_TABLE(of, sunxi_csi_match);

static struct platform_driver csi_platform_driver = {
	.probe    = csi_probe,
	.remove   = csi_remove,
	.driver = {
		.name   = CSI_MODULE_NAME,
		.owner  = THIS_MODULE,
        .of_match_table = sunxi_csi_match,
	}    
};

int sunxi_csi_platform_register(void)
{
	int ret;

	ret = platform_driver_register(&csi_platform_driver);
	if (ret) {
		vfe_err("platform driver register failed\n");
		return ret;
	}
	vfe_print("csi_init end\n");
	return 0;
}

void sunxi_csi_platform_unregister(void)
{
	vfe_print("csi_exit start\n");
	platform_driver_unregister(&csi_platform_driver);
	vfe_print("csi_exit end\n");
}

