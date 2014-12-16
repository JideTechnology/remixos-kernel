/*
 * drivers/gpu/ion/sunxi/sunxi_ion.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sunxi ion heap realization
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/slab.h>
#include "../ion_priv.h"

#define DEV_NAME	"ion-sunxi"

struct ion_device;
static int num_heaps;
struct ion_heap **pheap;
struct ion_device *ion_device;
EXPORT_SYMBOL(ion_device);
long sunxi_ion_ioctl(struct ion_client *client, unsigned int cmd, unsigned long arg);


int sunxi_ion_probe(struct platform_device *pdev)
{
	struct ion_platform_data *pdata = pdev->dev.platform_data;
	struct ion_platform_heap *heaps_desc;
	int i, ret = 0;

	pheap = kzalloc(sizeof(struct ion_heap *) * pdata->nr, GFP_KERNEL);
	ion_device = ion_device_create(sunxi_ion_ioctl);
	if(IS_ERR_OR_NULL(ion_device)) {
		kfree(pheap);
		return PTR_ERR(ion_device);
	}

	for(i = 0; i < pdata->nr; i++) {
		heaps_desc = &pdata->heaps[i];
		
		pheap[i] = ion_heap_create(heaps_desc);
		if(IS_ERR_OR_NULL(pheap[i])) {
			ret = PTR_ERR(pheap[i]);
			goto err;
		}

		ion_device_add_heap(ion_device, pheap[i]);

	}

	num_heaps = i;
	platform_set_drvdata(pdev, ion_device);
	return 0;
err:
	while(i--)
		ion_heap_destroy(pheap[i]);
	ion_device_destroy(ion_device);
	kfree(pheap);
	return ret;
}

int sunxi_ion_remove(struct platform_device *pdev)
{
	struct ion_device *dev = platform_get_drvdata(pdev);

	while(num_heaps--)
		ion_heap_destroy(pheap[num_heaps]);
	kfree(pheap);
	ion_device_destroy(dev);
	return 0;
}

static struct ion_platform_data ion_data ;

static struct platform_device ion_dev = {
	.name = DEV_NAME,
	.dev = {
		.platform_data = &ion_data,
	}
};

static struct platform_driver ion_driver = {
	.probe = sunxi_ion_probe,
	.remove = sunxi_ion_remove,
	.driver = {.name = DEV_NAME}
};

struct ion_platform_heap sunxi_ion_heaps[] = 
{
	[0] = {
		.type = ION_HEAP_TYPE_SYSTEM,
		.id = (u32)ION_HEAP_TYPE_SYSTEM,
		.name = "system",
	},
	[1] = {
		.type = ION_HEAP_TYPE_SYSTEM_CONTIG,
		.id = (u32)ION_HEAP_TYPE_SYSTEM_CONTIG,
		.name = "system_contig",
	},
	[2] = {
		.type = ION_HEAP_TYPE_DMA,
		.id = (u32)ION_HEAP_TYPE_DMA,
		.name = "cma",
		.priv = &(ion_dev.dev) ,
	}
};

static int __init sunxi_ion_init(void)
{
	int ret;

	ion_data.nr = ARRAY_SIZE(sunxi_ion_heaps);
	ion_data.heaps = sunxi_ion_heaps;
	
	ret = platform_device_register(&ion_dev);
	if(ret)
		return ret;
	return platform_driver_register(&ion_driver);
}

static void __exit sunxi_ion_exit(void)
{
	platform_driver_unregister(&ion_driver);
	platform_device_unregister(&ion_dev);
}

subsys_initcall(sunxi_ion_init);
module_exit(sunxi_ion_exit);
