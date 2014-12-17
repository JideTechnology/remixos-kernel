
/* 
 ***************************************************************************************
 * 
 * sunxi_isp.c
 * 
 * Hawkview ISP - sunxi_isp.c module
 * 
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 * 
 * Version		  Author         Date		    Description
 * 
 *   3.0		  Yang Feng   	2014/12/11	ISP Tuning Tools Support
 * 
 ****************************************************************************************
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include "vfe_os.h"
#include "platform_cfg.h"
#include "lib/bsp_isp.h"
#include "sunxi_isp.h"
#include "vfe.h"

#define ISP_MODULE_NAME "sunxi_isp"
struct isp_dev *isp_gbl;

static int sunxi_isp_subdev_s_power(struct v4l2_subdev *sd, int enable)
{
	return 0;
}
static int sunxi_isp_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int sunxi_isp_enum_mbus_code(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		      struct v4l2_subdev_mbus_code_enum *code)
{
	return 0;
}

static int sunxi_isp_subdev_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
				   struct v4l2_subdev_format *fmt)
{
	return 0;
}

static int sunxi_isp_subdev_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
				   struct v4l2_subdev_format *fmt)
{
	return 0;
}

int sunxi_isp_subdev_init(struct v4l2_subdev *sd, u32 val)
{
	bsp_isp_set_dma_load_addr((unsigned long)isp_gbl->isp_load_reg_mm.dma_addr);
	bsp_isp_set_dma_saved_addr((unsigned long)isp_gbl->isp_save_reg_mm.dma_addr);
	return 0;
}
static int sunxi_isp_subdev_cropcap(struct v4l2_subdev *sd,
				       struct v4l2_cropcap *a)
{
	return 0;
}

static const struct v4l2_subdev_core_ops sunxi_isp_core_ops = {
	.s_power = sunxi_isp_subdev_s_power,
	.init = sunxi_isp_subdev_init,
};

static const struct v4l2_subdev_video_ops sunxi_isp_subdev_video_ops = {
	.s_stream = sunxi_isp_subdev_s_stream,
	.cropcap	= sunxi_isp_subdev_cropcap,

};

static const struct v4l2_subdev_pad_ops sunxi_isp_subdev_pad_ops = {
	.enum_mbus_code = sunxi_isp_enum_mbus_code,
	.get_fmt = sunxi_isp_subdev_get_fmt,
	.set_fmt = sunxi_isp_subdev_set_fmt,
};


static struct v4l2_subdev_ops sunxi_isp_subdev_ops = {
	.core = &sunxi_isp_core_ops,
	.video = &sunxi_isp_subdev_video_ops,
	.pad = &sunxi_isp_subdev_pad_ops,
};
/*
static int sunxi_isp_subdev_registered(struct v4l2_subdev *sd)
{
	struct vfe_dev *vfe = v4l2_get_subdevdata(sd);
	int ret = 0;
	return ret;
}

static void sunxi_isp_subdev_unregistered(struct v4l2_subdev *sd)
{
	struct vfe_dev *vfe = v4l2_get_subdevdata(sd);
	return;
}

static const struct v4l2_subdev_internal_ops sunxi_isp_sd_internal_ops = {
	.registered = sunxi_isp_subdev_registered,
	.unregistered = sunxi_isp_subdev_unregistered,
};
*/
int sunxi_isp_init_subdev(struct isp_dev *isp)
{
    struct v4l2_subdev *sd = &isp->subdev;

    v4l2_subdev_init(sd, &sunxi_isp_subdev_ops);
    sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
    snprintf(sd->name, sizeof(sd->name), "sunxi_isp.%d", isp->isp_sel);
    //sd->internal_ops = &sunxi_isp_sd_internal_ops;
    v4l2_set_subdevdata(sd, isp);
    return 0;
}
    

static void isp_release(struct device *dev)
{
	vfe_print("isp_device_release\n");
};

static int isp_resource_alloc(struct isp_dev *isp)
{
	int ret = 0; 
	isp->isp_load_reg_mm.size = ISP_LOAD_REG_SIZE;
	isp->isp_save_reg_mm.size = ISP_SAVED_REG_SIZE;

	os_mem_alloc(&isp->isp_load_reg_mm);
	os_mem_alloc(&isp->isp_save_reg_mm);

	if(isp->isp_load_reg_mm.phy_addr != NULL) {
		if(!isp->isp_load_reg_mm.vir_addr)
		{
			vfe_err("isp load regs va requset failed!\n");
			return -ENOMEM;
		}
	} else {
		vfe_err("isp load regs pa requset failed!\n");
		return -ENOMEM;
	}
  
	if(isp->isp_save_reg_mm.phy_addr != NULL) {
		if(!isp->isp_save_reg_mm.vir_addr)
		{
			vfe_err("isp save regs va requset failed!\n");
			return -ENOMEM;
		}
	} else {
		vfe_err("isp save regs pa requset failed!\n");
		return -ENOMEM;
	}
	return ret;

}
static void isp_resource_release(struct isp_dev *isp)
{
	os_mem_free(&isp->isp_load_reg_mm);
	os_mem_free(&isp->isp_save_reg_mm);
}

static int isp_probe(struct platform_device *pdev)
{
	struct isp_dev *isp = NULL;
	struct resource *res = NULL;
	struct isp_platform_data *pdata = NULL;
	int ret = 0;
	if(pdev->dev.platform_data == NULL)
	{
		return -ENODEV;
	}
	pdata = pdev->dev.platform_data;
	vfe_print("isp probe start isp_sel = %d!\n",pdata->isp_sel);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		return -ENODEV;
	}
	isp = kzalloc(sizeof(struct isp_dev), GFP_KERNEL);
	if (!isp) {
		ret = -ENOMEM;
		goto ekzalloc;
	}
	isp_gbl = isp;
	isp->use_cnt = 0;
	sunxi_isp_init_subdev(isp);
	isp->ioarea = request_mem_region(res->start, resource_size(res), res->name);
	if (!isp->ioarea) {
		goto eremap;
	}
	isp->isp_sel = pdata->isp_sel;
	
	spin_lock_init(&isp->slock);
	init_waitqueue_head(&isp->wait);
	
	isp->base = ioremap(res->start, resource_size(res));
	if (!isp->base) {
		ret = -EIO;
		goto out_map;
	}
	if(isp_resource_alloc(isp) < 0)
	{
		ret = -EIO;
		goto ereqirq;
	}
	bsp_isp_init_platform(SUNXI_PLATFORM_ID);
	bsp_isp_set_base_addr((unsigned long)isp->base);
	bsp_isp_set_map_load_addr((unsigned long)isp->isp_load_reg_mm.vir_addr);
	bsp_isp_set_map_saved_addr((unsigned long)isp->isp_save_reg_mm.vir_addr);
	memset((unsigned int*)isp->isp_load_reg_mm.vir_addr,0,ISP_LOAD_REG_SIZE);
	memset((unsigned int*)isp->isp_save_reg_mm.vir_addr,0,ISP_SAVED_REG_SIZE);
	platform_set_drvdata(pdev, isp);
	vfe_print("isp probe end isp_sel = %d!\n",pdata->isp_sel);
	return 0;

ereqirq:
	iounmap(isp->base);
out_map:
	release_resource(isp->ioarea);
	kfree(isp->ioarea);
eremap:
	kfree(isp);
ekzalloc:
	vfe_print("isp probe err!\n");
	return ret;
}


static int isp_remove(struct platform_device *pdev)
{
	struct isp_dev *isp = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, NULL);
	isp_resource_release(isp) ;
	if(isp->ioarea) {
        release_resource(isp->ioarea);
        kfree(isp->ioarea);
	}
	if(isp->base)
		iounmap(isp->base);
	kfree(isp);
	return 0;
}

static struct resource isp0_resource[] = 
{
	[0] = {
		.name  = "isp",
		.start  = ISP_REGS_BASE,
		.end    = ISP_REGS_BASE + ISP_REG_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct isp_platform_data isp0_pdata[] = {
	{
		.isp_sel  = 0,
	},
};

static struct platform_device isp_device[] = {
	[0] = {
		.name  = ISP_MODULE_NAME,
		.id = 0,
		.num_resources = ARRAY_SIZE(isp0_resource),
		.resource = isp0_resource,
		.dev = {
			.platform_data  = isp0_pdata,
			.release        = isp_release,
		},
	},
};

static struct platform_driver isp_platform_driver = {
	.probe    = isp_probe,
	.remove   = isp_remove,
	.driver = {
		.name   = ISP_MODULE_NAME,
		.owner  = THIS_MODULE,
	}
};

int sunxi_isp_register_subdev(struct v4l2_device *v4l2_dev, struct v4l2_subdev *sd)
{
	return v4l2_device_register_subdev(v4l2_dev, sd);
}

void sunxi_isp_unregister_subdev(struct v4l2_subdev *sd)
{
	v4l2_device_unregister_subdev(sd);
	v4l2_set_subdevdata(sd, NULL);
}

int sunxi_isp_get_subdev(struct v4l2_subdev **sd, int sel)
{
	*sd = &isp_gbl->subdev;
	return (isp_gbl->use_cnt++);
}
int sunxi_isp_put_subdev(struct v4l2_subdev **sd, int sel)
{
	*sd = NULL;
	return (isp_gbl->use_cnt--);
}

int sunxi_isp_platform_register(void)
{
	int ret,i;
	for(i=0; i<ARRAY_SIZE(isp_device); i++) 
	{
		ret = platform_device_register(&isp_device[i]);
		if (ret)
			vfe_err("isp device %d register failed\n",i);
	}
	ret = platform_driver_register(&isp_platform_driver);
	if (ret) {
		vfe_err("platform driver register failed\n");
		return ret;
	}
	vfe_print("sunxi_isp_platform_register end\n");
	return 0;
}

void  sunxi_isp_platform_unregister(void)
{
	int i;
	vfe_print("sunxi_isp_platform_unregister start\n");
	for(i=0; i<ARRAY_SIZE(isp_device); i++)
	{
		platform_device_unregister(&isp_device[i]);
	}
	platform_driver_unregister(&isp_platform_driver);
	vfe_print("sunxi_isp_platform_unregister end\n");
}


