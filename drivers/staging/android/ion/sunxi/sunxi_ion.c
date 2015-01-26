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
#include <linux/of.h>

struct ion_device;
static struct ion_device *ion_device;
long sunxi_ion_ioctl(struct ion_client *client, unsigned int cmd, unsigned long arg);

struct ion_client *sunxi_ion_client_create(const char *name)
{
	return ion_client_create(ion_device , name);
}
EXPORT_SYMBOL(sunxi_ion_client_create);


int sunxi_ion_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ion_platform_heap heaps_desc;
	struct device_node *heap_node = NULL;	

	ion_device = ion_device_create(sunxi_ion_ioctl);
	if(IS_ERR_OR_NULL(ion_device)) 
	{
		return PTR_ERR(ion_device);
	}

	do{
		u32 type = -1;
		struct ion_heap *pheap = NULL;
		
		/*loop all the child node */
		heap_node = of_get_next_child(np , heap_node);
		if(!heap_node)
			break;
		memset( &heaps_desc , 0 , sizeof(heaps_desc) );
		
		/* get the properties "name","type" for common ion heap	*/
		if(of_property_read_u32(heap_node , "type" , &type) )
		{	
			pr_err("You need config the heap node 'type'\n");
			continue;
		}
		heaps_desc.type = type;
		heaps_desc.id = type;

		if(of_property_read_string(heap_node , "name" , &heaps_desc.name) )
		{ 
			pr_err("You need config the heap node 'name'\n");
			continue;
		}

		/*for specail heaps , need extra argument to config */
		if( ION_HEAP_TYPE_CARVEOUT == heaps_desc.type )
		{
			u32 base = 0 , size = 0;
			if( of_property_read_u32( heap_node , "base" , &base) )
				pr_err("You need config the carvout 'base'\n");
			heaps_desc.base = base;
			if( of_property_read_u32( heap_node , "size" , &size) )
				pr_err("You need config the carvout 'size'\n"); 
			heaps_desc.size = size;
		}else if( ION_HEAP_TYPE_DMA == heaps_desc.type )
		{
			heaps_desc.priv = &(pdev->dev);
		}

		/* now we can create a heap & add it to the ion device*/
		pheap = ion_heap_create(&heaps_desc);
		if(IS_ERR_OR_NULL(pheap)) 
		{
			pr_err("ion_heap_create '%s' failured!\n" , heaps_desc.name );
			continue;
		}

		ion_device_add_heap(ion_device , pheap );		
	}while(1);

	return 0;
}

static const struct of_device_id sunxi_ion_dt_ids[] = {
	{ .compatible = "allwinner,sunxi-ion" },
	{ /* sentinel */ }
};

static struct platform_driver sunxi_ion_driver = {
	.driver = {
		.name = "sunxi-ion",
		.of_match_table = sunxi_ion_dt_ids,
	},
	.probe = sunxi_ion_probe,
};
module_platform_driver(sunxi_ion_driver);