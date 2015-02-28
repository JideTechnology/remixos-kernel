
/* 
 ***************************************************************************************
 * 
 * sunxi_mipi.h
 * 
 * Hawkview ISP - sunxi_mipi.h module
 * 
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 * 
 * Version		  Author         Date		    Description
 * 
 *   3.0		  Yang Feng   	2015/02/27	ISP Tuning Tools Support
 * 
 ****************************************************************************************
 */
#ifndef _SUNXI_MIPI__H_
#define _SUNXI_MIPI__H_

#include "../platform_cfg.h"
struct mipi_platform_data
{
	unsigned int mipi_sel;
}; 
struct mipi_dev
{
	unsigned int  mipi_sel;
	struct platform_device  *pdev;
	unsigned int id;
	spinlock_t slock;
//	int irq;  
	wait_queue_head_t   wait;
	void __iomem      *base;
};

int sunxi_mipi_platform_register(void);
void sunxi_mipi_platform_unregister(void);

#endif /*_SUNXI_MIPI__H_*/
