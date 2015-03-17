
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
#include <media/v4l2-ctrls.h>

#include "vfe_os.h"
struct isp_platform_data
{
	unsigned int isp_sel;
}; 

struct isp_yuv_size_addr_info
{
	unsigned int isp_byte_size;
	unsigned int line_stride_y;
	unsigned int line_stride_c;
	unsigned int buf_height_y;
	unsigned int buf_height_cb;
	unsigned int buf_height_cr;

	unsigned int valid_height_y;
	unsigned int valid_height_cb;
	unsigned int valid_height_cr;
	struct isp_yuv_channel_addr yuv_addr;
};


struct sunxi_isp_ctrls {
	struct v4l2_ctrl_handler handler;

	struct v4l2_ctrl *hflip;
	struct v4l2_ctrl *vflip;
	struct v4l2_ctrl *rotate;
};

struct isp_dev
{
	unsigned int  isp_sel;
	int use_cnt;
	struct v4l2_subdev subdev;
	struct platform_device  *pdev;
	struct sunxi_isp_ctrls ctrls;
	int vflip;
	int hflip;
	int rotate;
	unsigned int id;
	spinlock_t slock;
	struct mutex subdev_lock;
	//int irq;  
	wait_queue_head_t   wait;
	void __iomem      *base;
	struct resource *ioarea;
	struct vfe_mm isp_load_reg_mm;
	struct vfe_mm isp_save_reg_mm;
	int rotation_en;
	enum enable_flag flip_en_glb[ISP_MAX_CH_NUM];
	int plannar_uv_exchange_flag[ISP_MAX_CH_NUM];
	struct isp_yuv_size_addr_info isp_yuv_size_addr[ISP_MAX_CH_NUM];
};
void sunxi_isp_set_fmt(enum bus_pixeltype type, enum pixel_fmt *fmt);
void sunxi_isp_set_flip(enum isp_channel ch, enum enable_flag on_off);
void sunxi_isp_set_mirror(enum isp_channel ch, enum enable_flag on_off);

unsigned int sunxi_isp_set_size(enum pixel_fmt *fmt, struct isp_size_settings *size_settings);
void sunxi_isp_set_output_addr(unsigned long buf_base_addr);



int sunxi_isp_get_subdev(struct v4l2_subdev **sd, int sel);
int sunxi_isp_put_subdev(struct v4l2_subdev **sd, int sel);
int sunxi_isp_register_subdev(struct v4l2_device *v4l2_dev, struct v4l2_subdev *sd);
void sunxi_isp_unregister_subdev(struct v4l2_subdev *sd);
int  sunxi_isp_platform_register(void);
void  sunxi_isp_platform_unregister(void);

#endif /*_SUNXI_ISP_H_*/

