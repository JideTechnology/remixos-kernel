
/* 
 ***************************************************************************************
 * 
 * sunxi_isp.h
 * 
 * Hawkview ISP - sunxi_isp.h module
 * 
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 * 
 * Version		  Author         Date		    Description
 * 
 *   3.0		  Yang Feng   	2014/12/11	ISP Tuning Tools Support
 * 
 ****************************************************************************************
 */


#ifndef _SUNXI_ISP_H_
#define _SUNXI_ISP_H_

#include "vfe_os.h"
struct isp_platform_data
{
	unsigned int isp_sel;
}; 
struct isp_dev
{
	unsigned int  isp_sel;
	int use_cnt;
	struct v4l2_subdev subdev;
	struct platform_device  *pdev;
	unsigned int id;
	spinlock_t slock;
	//int irq;  
	wait_queue_head_t   wait;
	void __iomem      *base;
	struct resource *ioarea;
	struct vfe_mm isp_load_reg_mm;
	struct vfe_mm isp_save_reg_mm;
};
int sunxi_isp_get_subdev(struct v4l2_subdev **sd, int sel);
int sunxi_isp_put_subdev(struct v4l2_subdev **sd, int sel);
int sunxi_isp_register_subdev(struct v4l2_device *v4l2_dev, struct v4l2_subdev *sd);
void sunxi_isp_unregister_subdev(struct v4l2_subdev *sd);
int  sunxi_isp_platform_register(void);
void  sunxi_isp_platform_unregister(void);

#endif /*_SUNXI_ISP_H_*/

