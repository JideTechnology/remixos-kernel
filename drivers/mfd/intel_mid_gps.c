/*
 * intel_mid_gps.c: Intel interface for gps devices
 *
 * (C) Copyright 2013 Intel Corporation
 * Author: Kuppuswamy, Sathyanarayanan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/acpi.h>
//#include <linux/acpi_gpio.h>
#include <linux/efi.h>
#include <linux/wakelock.h>
#include <linux/pm_runtime.h>
#include <asm/intel-mid.h>
//#include <asm/intel_mid_hsu.h>
#include <linux/intel_mid_gps.h>

#define DRIVER_NAME "intel_mid_gps"

#define ACPI_DEVICE_ID_BCM4752 "BCM4752"

struct device *tty_dev = NULL;
struct wake_lock hostwake_lock;

/*********************************************************************
 *		Driver GPIO toggling functions
 *********************************************************************/

static int intel_mid_gps_reset(struct device *dev, const char *buf)
{
	struct intel_mid_gps_platform_data *gps = dev_get_drvdata(dev);

	if (!buf)
		return -EINVAL;

	if (!gps)
		return -EINVAL;

	if (sscanf(buf, "%i", &gps->reset) != 1)
		return -EINVAL;

	if (gpio_is_valid(gps->gpio_reset))
		gpio_set_value(gps->gpio_reset,
				gps->reset ? RESET_ON : RESET_OFF);
	else
		return -EINVAL;

	return 0;
}

static int intel_mid_gps_enable(struct device *dev, const char *buf)
{
	struct intel_mid_gps_platform_data *gps = dev_get_drvdata(dev);

	if (!buf)
		return -EINVAL;

	if (!gps)
		return -EINVAL;

	if (sscanf(buf, "%i", &gps->enable) != 1)
		return -EINVAL;

	if (gpio_is_valid(gps->gpio_enable))
		gpio_set_value(gps->gpio_enable,
				gps->enable ? ENABLE_ON : ENABLE_OFF);
	else
		return -EINVAL;

	return 0;
}

/*********************************************************************
 *		Driver sysfs attribute functions
 *********************************************************************/

static ssize_t intel_mid_gps_reset_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct intel_mid_gps_platform_data *gps = dev_get_drvdata(dev);

	if (!gps)
		return -EINVAL;

	return sprintf(buf, "%d\n", gps->reset);
}

static ssize_t intel_mid_gps_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;

	ret = intel_mid_gps_reset(dev, buf);

	return !ret ? size : ret;
}

static ssize_t intel_mid_gps_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct intel_mid_gps_platform_data *gps = dev_get_drvdata(dev);

	if (!gps)
		return -EINVAL;

	return sprintf(buf, "%d\n", gps->enable);
}

static ssize_t intel_mid_gps_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;

	ret = intel_mid_gps_enable(dev, buf);

	return !ret ? size : ret;
}

static DEVICE_ATTR(reset, S_IRUGO|S_IWUSR, intel_mid_gps_reset_show,
				intel_mid_gps_reset_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR, intel_mid_gps_enable_show,
				intel_mid_gps_enable_store);

static struct attribute *intel_mid_gps_attrs[] = {
	&dev_attr_reset.attr,
	&dev_attr_enable.attr,
	NULL,
};

static struct attribute_group intel_mid_gps_attr_group = {
	.name = DRIVER_NAME,
	.attrs = intel_mid_gps_attrs,
};

/*********************************************************************
 *		Driver GPIO probe/remove functions
 *********************************************************************/

/*static irqreturn_t intel_mid_gps_hostwake_isr(int irq, void *dev)
{
	struct intel_mid_gps_platform_data *pdata = dev_get_drvdata(dev);
	int hostwake;

	hostwake = gpio_get_value(pdata->gpio_hostwake);

	tty_dev = intel_mid_hsu_set_wake_peer(pdata->hsu_port, NULL);
	if (!tty_dev) {
		pr_err("%s: unable to get the HSU tty device \n", __func__);
	}

	irq_set_irq_type(irq, hostwake ? IRQF_TRIGGER_FALLING :
			 IRQF_TRIGGER_RISING);

	if (hostwake) {
		wake_lock(&hostwake_lock);
		if (tty_dev)
			pm_runtime_get(tty_dev);
	}
	else {
		if (tty_dev)
			pm_runtime_put(tty_dev);
		wake_unlock(&hostwake_lock);
	}

	return IRQ_HANDLED;
}*/

static int intel_mid_gps_init(struct platform_device *pdev)
{
	int ret;
	struct intel_mid_gps_platform_data *pdata = dev_get_drvdata(&pdev->dev);
	struct device *parent, *grand_parent;

	/* we need to rename the sysfs entry to match the one created with SFI,
	   and we are sure that there is always one GPS per platform */
	/*if (ACPI_HANDLE(&pdev->dev)) {
		ret = sysfs_rename_dir(&pdev->dev.kobj, DRIVER_NAME);
		if (ret)
			pr_err("%s: failed to rename sysfs entry\n", __func__);
	}*/

	ret = sysfs_create_group(&pdev->dev.kobj, &intel_mid_gps_attr_group);
	if (ret)
		dev_err(&pdev->dev,
			"Failed to create intel_mid_gps sysfs interface\n");

	/* With ACPI device tree, GPS is UART child, we need to create symlink at
	   /sys/devices/platform/ level (user libgps is expecting this).
	   To be removed when GPIO sysfs path will not be used from user space */
	/*if (ACPI_HANDLE(&pdev->dev)) {
		parent = pdev->dev.parent;
		grand_parent = (parent == NULL ? NULL : parent->parent);
		if (parent != NULL && grand_parent != NULL) {
			ret = sysfs_create_link(&grand_parent->kobj, &pdev->dev.kobj, DRIVER_NAME);
			if (ret)
				pr_err("%s: failed to create symlink\n", __func__);
		}
	}*/
#if 0
	/* Handle reset GPIO */
	if (gpio_is_valid(pdata->gpio_reset)) {

		/* Request gpio */
		ret = gpio_request(pdata->gpio_reset, "intel_mid_gps_reset");
		if (ret < 0) {
			pr_err("%s: Unable to request GPIO:%d, err:%d\n",
					__func__, pdata->gpio_reset, ret);
			goto error_gpio_reset_request;
		}

		/* set gpio direction */
		ret = gpio_direction_output(pdata->gpio_reset, pdata->reset);
		if (ret < 0) {
			pr_err("%s: Unable to set GPIO:%d direction, err:%d\n",
					__func__, pdata->gpio_reset, ret);
			goto error_gpio_reset_direction;
		}
	}
#endif
	/* Handle enable GPIO */
	if (gpio_is_valid(pdata->gpio_enable)) {

		/* Request gpio */
		ret = gpio_request(pdata->gpio_enable, "intel_mid_gps_enable");
		if (ret < 0) {
			pr_err("%s: Unable to request GPIO:%d, err:%d\n",
					__func__, pdata->gpio_enable, ret);
			goto error_gpio_enable_request;
		}

		/* set gpio direction */
		ret = gpio_direction_output(pdata->gpio_enable, pdata->enable);
		if (ret < 0) {
			pr_err("%s: Unable to set GPIO:%d direction, err:%d\n",
					__func__, pdata->gpio_enable, ret);
			goto error_gpio_enable_direction;
		}
	}
#if 0
	/* Handle hostwake GPIO */
	if (gpio_is_valid(pdata->gpio_hostwake)) {
		int irq_id = -EINVAL;

		/* Request gpio */
		ret = gpio_request(pdata->gpio_hostwake, "intel_mid_gps_hostwake");
		if (ret < 0) {
			pr_err("%s: Unable to request GPIO:%d, err:%d\n",
					__func__, pdata->gpio_hostwake, ret);
			goto error_gpio_hostwake_request;
		}

		/* set gpio direction */
		ret = gpio_direction_input(pdata->gpio_hostwake);
		if (ret < 0) {
			pr_err("%s: Unable to set GPIO:%d direction, err:%d\n",
					__func__, pdata->gpio_hostwake, ret);
			goto error_gpio_hostwake_direction;
		}

		/* configure irq handling */
		irq_id = gpio_to_irq(pdata->gpio_hostwake);
		irq_set_irq_wake(irq_id, 1);

		ret = request_irq(irq_id, intel_mid_gps_hostwake_isr,
				  IRQF_TRIGGER_RISING,
				  "gps_hostwake", &pdev->dev);
		if (ret) {
			pr_err("%s: unable to request irq %d \n", __func__, irq_id);
			goto error_gpio_hostwake_direction;
		}

		wake_lock_init(&hostwake_lock, WAKE_LOCK_SUSPEND, "gps_hostwake_lock");
	}
#endif
	return 0;

error_gpio_hostwake_direction:
	gpio_free(pdata->gpio_hostwake);
error_gpio_hostwake_request:
error_gpio_enable_direction:
	gpio_free(pdata->gpio_enable);
error_gpio_enable_request:
error_gpio_reset_direction:
	gpio_free(pdata->gpio_reset);
error_gpio_reset_request:
	sysfs_remove_group(&pdev->dev.kobj, &intel_mid_gps_attr_group);

	return ret;
}

static void intel_mid_gps_deinit(struct platform_device *pdev)
{
	struct intel_mid_gps_platform_data *pdata = dev_get_drvdata(&pdev->dev);

	if (gpio_is_valid(pdata->gpio_enable))
		gpio_free(pdata->gpio_enable);

	if (gpio_is_valid(pdata->gpio_reset))
		gpio_free(pdata->gpio_reset);

	if (gpio_is_valid(pdata->gpio_hostwake)) {
		free_irq(gpio_to_irq(pdata->gpio_hostwake), &pdev->dev);
		gpio_free(pdata->gpio_hostwake);
		wake_lock_destroy(&hostwake_lock);
	}

	sysfs_remove_group(&pdev->dev.kobj, &intel_mid_gps_attr_group);
}

static int intel_mid_gps_probe(struct platform_device *pdev)
{
	struct intel_mid_gps_platform_data *pdata = NULL;
	int ret = 0;

	pr_info("%s probe called\n", dev_name(&pdev->dev));
#if 0
	if (ACPI_HANDLE(&pdev->dev)) {
		/* create a new platform data that will be
		   populated with gpio data from ACPI table */
		struct acpi_gpio_info info;
		pdata = kzalloc(sizeof(struct intel_mid_gps_platform_data),
			GFP_KERNEL);

		if (!pdata)
			return -ENOMEM;

#ifndef CONFIG_GPS_CSRGSD5T
		if (!strncmp(dev_name(&pdev->dev),
				ACPI_DEVICE_ID_BCM4752,
				strlen(ACPI_DEVICE_ID_BCM4752))) {
			pdata->has_reset = 0;  /* Unavailable */
			pdata->has_enable = 1; /* Available */
		}

#else
                 /* GPS GSD5T to enable both*/
               pdata->has_reset = 1;  /* Available */
               pdata->has_enable = 1; /* Available */
#endif
#ifndef CONFIG_GPS_CSRGSD5T

		/* Check below if EDK Bios (EFI RS enabled) or FDK Ifwi
		 as we have to maintain both ACPI tables for now */
		if (efi_enabled(EFI_RUNTIME_SERVICES))
			pdata->gpio_enable = pdata->has_enable ?
				acpi_get_gpio_by_index(&pdev->dev, 0, &info)
				: -EINVAL;
		else {
			pdata->gpio_reset = pdata->has_reset ?
				acpi_get_gpio_by_index(&pdev->dev, 0, &info)
				: -EINVAL;
			pdata->gpio_enable = pdata->has_enable ?
				acpi_get_gpio_by_index(&pdev->dev, 1, &info)
				: -EINVAL;
		}
		pdata->gpio_hostwake = -EINVAL;
#else
               pdata->gpio_reset  = 139;/*GPIO_SUS9,NFC_FW_RESET not used*/
               pdata->gpio_enable = 152;/*GPIO_SUS22, GPS_WAKEUP*/
#endif
		pr_info("%s enable: %d, reset: %d\n", __func__, pdata->gpio_enable, pdata->gpio_reset);
		platform_set_drvdata(pdev, pdata);
	} else 
#endif
	{
		platform_set_drvdata(pdev, pdev->dev.platform_data);
	}

	ret = intel_mid_gps_init(pdev);

	if (ret) {
		dev_err(&pdev->dev, "Failed to initalize %s\n",
			dev_name(&pdev->dev));

		if (ACPI_HANDLE(&pdev->dev))
			kfree(pdata);
	}
	return ret;
}

static int intel_mid_gps_remove(struct platform_device *pdev)
{
	intel_mid_gps_deinit(pdev);

	if (ACPI_HANDLE(&pdev->dev))
		kfree(dev_get_drvdata(&pdev->dev));

	return 0;
}

static void intel_mid_gps_shutdown(struct platform_device *pdev)
{
	struct intel_mid_gps_platform_data *pdata = dev_get_drvdata(&pdev->dev);

	pr_info("%s shutdown called\n", dev_name(&pdev->dev));

	/* Turn gps off if not done already */
	if (gpio_is_valid(pdata->gpio_enable))
		gpio_set_value(pdata->gpio_enable, ENABLE_OFF);
}

/*********************************************************************
 *		Driver initialisation and finalization
 *********************************************************************/

#if 0 //def CONFIG_ACPI
static struct acpi_device_id acpi_gps_id_table[] = {
	/* ACPI IDs here */
#ifndef CONFIG_GPS_CSRGSD5T
        {ACPI_DEVICE_ID_BCM4752},
#else
        /* Fix me: use this to rigister gps device, if FW supported BCM4752 correctly, remove it, here
           BT cannot be used */
        {"OBDA8723"},
#endif
	{ }
};

MODULE_DEVICE_TABLE(acpi, acpi_gps_id_table);
#endif

static struct platform_device_id bcm4752_ids[] = {
	{
		.name = DRIVER_NAME,
	},
	{}
};


static struct platform_driver intel_mid_gps_driver = {
	.probe		= intel_mid_gps_probe,
	.remove		= intel_mid_gps_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
#if 0//def CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(acpi_gps_id_table),
#endif
	},
	//.id_table = bcm4752_ids;
	.shutdown	= intel_mid_gps_shutdown,
};

static int __init intel_mid_gps_driver_init(void)
{
	return platform_driver_register(&intel_mid_gps_driver);
}

static void __exit intel_mid_gps_driver_exit(void)
{
	platform_driver_unregister(&intel_mid_gps_driver);
}

module_init(intel_mid_gps_driver_init);
module_exit(intel_mid_gps_driver_exit);

MODULE_AUTHOR("Kuppuswamy, Sathyanarayanan");
MODULE_DESCRIPTION("Intel MID GPS driver");
MODULE_LICENSE("GPL");
