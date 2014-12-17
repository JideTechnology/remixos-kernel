
/* 
 ***************************************************************************************
 * 
 * cci_platform_drv.h
 * 
 * Hawkview ISP - csi_platform_drv.h module
 * 
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 * 
 * Version		  Author         Date		    Description
 * 
 *   2.0		  Yang Feng   	2014/06/23	      Second Version
 * 
 ****************************************************************************************
 */
#ifndef _SUNXI_CSI_H_
#define _SUNXI_CSI_H_

#include "../platform_cfg.h"
struct csi_platform_data
{
	unsigned int csi_sel;
}; 
struct csi_dev
{
	unsigned int  csi_sel;
	struct platform_device  *pdev;
	unsigned int id;
	spinlock_t slock;
	int irq;  
	wait_queue_head_t   wait;
	void __iomem      *base;
    struct resource *ioarea;
};

int sunxi_csi_platform_register(void);
void sunxi_csi_platform_unregister(void);

#endif /*_SUNXI_CSI_H_*/
