/*
 * Copyright (c) 2013-2015 liming@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/mfd/axp-mfd.h>
#ifdef CONFIG_SUNXI_ARISC
#include <linux/arisc/arisc.h>
#endif
#include "axp-cfg.h"

static DEFINE_SPINLOCK(axp_list_lock);
static LIST_HEAD(mfd_list);

#ifdef CONFIG_SUNXI_ARISC
static s32 axp_mfd_irq_cb(void *arg)
{
	struct axp_dev *dev;
	unsigned long irqflags;

	spin_lock_irqsave(&axp_list_lock, irqflags);
	list_for_each_entry(dev, &mfd_list, list) {
		(void)schedule_work(&dev->irq_work);
	}
	spin_unlock_irqrestore(&axp_list_lock, irqflags);

	return 0;
}
#endif

struct axp_dev *axp_dev_lookup(s32 type)
{
	struct axp_dev *dev = NULL;
	unsigned long irqflags;

	spin_lock_irqsave(&axp_list_lock, irqflags);
	list_for_each_entry(dev, &mfd_list, list) {
		if (type == dev->type)
			goto out;
	}
	spin_unlock_irqrestore(&axp_list_lock, irqflags);
	return NULL;
out:
	spin_unlock_irqrestore(&axp_list_lock, irqflags);
	return dev;
}
EXPORT_SYMBOL_GPL(axp_dev_lookup);

static void axp_power_off(void)
{
#if defined (CONFIG_AW_AXP81X)
	axp81x_power_off();
#endif
}

static s32 axp_mfd_create_attrs(struct axp_dev *dev)
{
	s32 j,ret = 0;

	for (j = 0; j < dev->attrs_number; j++) {
		ret = device_create_file(dev->dev, (dev->attrs+j));
		if (ret)
			goto sysfs_failed;
	}
	return ret;

sysfs_failed:
	while (j--)
		device_remove_file(dev->dev, (dev->attrs+j));
	return ret;
}

static s32 __remove_subdev(struct device *dev, void *unused)
{
	platform_device_unregister(to_platform_device(dev));
	return 0;
}

static s32 axp_mfd_remove_subdevs(struct axp_dev *dev)
{
	return device_for_each_child(dev->dev, NULL, __remove_subdev);
}

static s32 axp_mfd_add_subdevs(struct axp_dev *dev)
{
	struct axp_funcdev_info *regl_dev;
	struct axp_funcdev_info *sply_dev;
	struct platform_device *pdev;
	s32 i, ret = 0;

	/* register for regultors */
	for (i = 0; i < dev->pdata->num_regl_devs; i++) {
		regl_dev = &dev->pdata->regl_devs[i];
		pdev = platform_device_alloc(regl_dev->name, regl_dev->id);
		pdev->dev.parent = dev->dev;
		pdev->dev.platform_data = regl_dev->platform_data;
		ret = platform_device_add(pdev);
		if (ret)
			goto failed;
	}

	/* register for power supply */
	for (i = 0; i < dev->pdata->num_sply_devs; i++) {
		sply_dev = &dev->pdata->sply_devs[i];
		pdev = platform_device_alloc(sply_dev->name, sply_dev->id);
		pdev->dev.parent = dev->dev;
		pdev->dev.platform_data = sply_dev->platform_data;
		ret = platform_device_add(pdev);
		if (ret)
			goto failed;
	}

	return 0;

failed:
	axp_mfd_remove_subdevs(dev);
	return ret;
}

s32 axp_register_mfd(struct axp_dev *dev)
{
	s32 ret = -1;
	unsigned long irqflags;

	spin_lock_init(&dev->spinlock);
	mutex_init(&dev->lock);
	BLOCKING_INIT_NOTIFIER_HEAD(&dev->notifier_list);

	ret = dev->ops->init_chip(dev);
	if (ret)
		goto out_free_dev;

	spin_lock_irqsave(&axp_list_lock, irqflags);
	list_add(&dev->list, &mfd_list);
	spin_unlock_irqrestore(&axp_list_lock, irqflags);

	ret = axp_mfd_add_subdevs(dev);
	if (ret)
		goto out_free_dev;

	ret = axp_mfd_create_attrs(dev);
	if(ret)
		goto out_free_dev;

	return 0;
out_free_dev:
	return ret;
}
EXPORT_SYMBOL(axp_register_mfd);

void axp_unregister_mfd(struct axp_dev *dev)
{
	unsigned long irqflags;

	if (dev == NULL)
		return;

	axp_mfd_remove_subdevs(dev);

	spin_lock_irqsave(&axp_list_lock, irqflags);
	list_del(&dev->list);
	spin_unlock_irqrestore(&axp_list_lock, irqflags);

	return;
}
EXPORT_SYMBOL_GPL(axp_unregister_mfd);

static s32 __init axp_mfd_init(void)
{
	s32 ret = 0;

	/* PM hookup */
	if(!pm_power_off)
		pm_power_off = axp_power_off;

#ifdef CONFIG_SUNXI_ARISC
	ret = arisc_nmi_cb_register(NMI_INT_TYPE_PMU, axp_mfd_irq_cb, NULL);
	if (ret) {
		printk("axp failed to reg irq cb\n");
	}
#else
#endif
	return ret;
}

static void __exit axp_mfd_exit(void)
{
	pm_power_off = NULL;

#ifdef CONFIG_SUNXI_ARISC
	arisc_nmi_cb_unregister(NMI_INT_TYPE_PMU, axp_mfd_irq_cb);
#else
#endif

}

subsys_initcall(axp_mfd_init);
module_exit(axp_mfd_exit);

MODULE_DESCRIPTION("PMIC MFD Driver for AXP");
MODULE_AUTHOR("LiMing X-POWERS");
MODULE_LICENSE("GPL");

