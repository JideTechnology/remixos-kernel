/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 * Copyright 2010, 2011 David Jander <david@protonic.nl>
 *
 * Copyright 2010-2011 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/spinlock.h>
#include <asm/mach-types.h>
#include <linux/gpio_switch.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/syscalls.h>
#include	<linux/errno.h>
#include	<linux/fs.h>
#include	<linux/uaccess.h>




static int swtich_strtok(const char *buf, size_t count, char **token, const int token_nr)
{
	char *buf2 = (char *)kzalloc((count + 1) * sizeof(char), GFP_KERNEL);
	char **token2 = token;
	unsigned int num_ptr = 0, num_nr = 0, num_neg = 0;
	int i = 0, start = 0, end = (int)count;

	strcpy(buf2, buf);

	/* We need to breakup the string into separate chunks in order for kstrtoint
	 * or strict_strtol to parse them without returning an error. Stop when the end of
	 * the string is reached or when enough value is read from the string */
	while((start < end) && (i < token_nr)) {
		/* We found a negative sign */
		if(*(buf2 + start) == '-') {
			/* Previous char(s) are numeric, so we store their value first before proceed */
			if(num_nr > 0) {
				/* If there is a pending negative sign, we adjust the variables to account for it */
				if(num_neg) {
					num_ptr--;
					num_nr++;
				}
				*token2 = (char *)kzalloc((num_nr + 2) * sizeof(char), GFP_KERNEL);
				strncpy(*token2, (const char *)(buf2 + num_ptr), (size_t) num_nr);
				*(*token2+num_nr) = '\n';
				i++;
				token2++;
				/* Reset */
				num_ptr = num_nr = 0;
			}
			/* This indicates that there is a pending negative sign in the string */
			num_neg = 1;
		}
		/* We found a numeric */
		else if((*(buf2 + start) >= '0') && (*(buf2 + start) <= '9')) {
			/* If the previous char(s) are not numeric, set num_ptr to current char */
			if(num_nr < 1)
				num_ptr = start;
			num_nr++;
		}
		/* We found an unwanted character */
		else {
			/* Previous char(s) are numeric, so we store their value first before proceed */
			if(num_nr > 0) {
				if(num_neg) {
					num_ptr--;
					num_nr++;
				}
				*token2 = (char *)kzalloc((num_nr + 2) * sizeof(char), GFP_KERNEL);
				strncpy(*token2, (const char *)(buf2 + num_ptr), (size_t) num_nr);
				*(*token2+num_nr) = '\n';
				i++;
				token2++;
			}
			/* Reset all the variables to start afresh */
			num_ptr = num_nr = num_neg = 0;
		}
		start++;
	}

	kfree(buf2);

	return (i == token_nr) ? token_nr : -1;
}
static ssize_t gpio_show_switches(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);		
	struct gpio_switch_drvdata *ddata = platform_get_drvdata(pdev);	
									
	return sprintf(buf, "%d\n", gpio_get_value(ddata->data->gpio));		
}
static ssize_t gpio_store_switches(struct device *dev,
				     struct device_attribute *attr,
				     char *buf,size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);		
	struct gpio_switch_drvdata *ddata = platform_get_drvdata(pdev);	
	char *buf2;
	const int direct_count = 1;
	unsigned long direction;
	int err = 0;

	/* Lock the device to prevent races with open/close (and itself) */
	mutex_lock(&ddata->mutex);

	if(swtich_strtok(buf, count, &buf2, direct_count) < 0) {
		printk("%s: No direction data being read. " 
				"No direction data will be updated.\n", __func__);
	}

	else {
		/* Removes any leading negative sign */
		while(*buf2 == '-')
			buf2++;
		#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35))
		err = kstrtouint((const char *)buf2, 10, (unsigned int *)&direction);
		if (err < 0) {
			printk("%s: kstrtouint returned err = %d\n", __func__, err);
			goto exit;
		}
		#else
		err = strict_strtoul((const char *)buf2, 10, &direction);
		if (err < 0) {
			printk("%s: strict_strtoul returned err = %d\n", __func__, err);
			goto exit;
		}
		#endif
		gpio_direction_output(ddata->data->gpio,direction);					
	}

exit:
	mutex_unlock(&ddata->mutex);
	
	return sprintf(buf, "%d\n", gpio_get_value(ddata->data->gpio));		
}

static DEVICE_ATTR(switches, S_IRUGO, gpio_show_switches, gpio_store_switches);

static struct attribute *gpio_keys_attrs[] = {
	&dev_attr_switches.attr,
	NULL,
};

static struct attribute_group gpio_keys_attr_group = {
	.attrs = gpio_keys_attrs,
};

static int __devinit gpio_switch_probe(struct platform_device *pdev)
{
	const struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_switch_drvdata *ddata;
	struct device *dev = &pdev->dev;

	int error,wakeup = 0;

	ddata = kzalloc(sizeof(struct gpio_switch_drvdata) +
			 sizeof(struct gpio_switch_drvdata),
			GFP_KERNEL);

	if (!ddata) {
		dev_err(dev, "failed to allocate state\n");
		error = -ENOMEM;
		goto fail1;
	}
	ddata->data = pdata;
	platform_set_drvdata(pdev, ddata);
	mutex_init(&ddata->mutex);
	if (gpio_is_valid(ddata->data->gpio)) {

		error = gpio_request(ddata->data->gpio, "gpio_switch_otg");
		if (error < 0) {
			dev_err(dev, "Failed to request GPIO %d, error %d\n",
				ddata->data->gpio, error);
			return error;
		}

		error = gpio_direction_output(ddata->data->gpio,0);
		if (error < 0) {
			dev_err(dev,
				"Failed to configure direction for GPIO %d, error %d\n",
				ddata->data->gpio, error);
			goto fail2;
		}
	} 
	error = sysfs_create_group(&pdev->dev.kobj, &gpio_keys_attr_group);
	if (error) {
		dev_err(dev, "Unable to export keys/switches, error: %d\n",
			error);
		goto fail2;
	}
	return 0;

 fail3:
	sysfs_remove_group(&pdev->dev.kobj, &gpio_keys_attr_group);
 fail2:
	

	platform_set_drvdata(pdev, NULL);
 fail1:
	
	kfree(ddata);
	return error;
}

static int __devexit gpio_switch_remove(struct platform_device *pdev)
{
	struct gpio_switch_drvdata *ddata = platform_get_drvdata(pdev);
	int i;

	sysfs_remove_group(&pdev->dev.kobj, &gpio_keys_attr_group);
	kfree(ddata);

	return 0;
}

static struct platform_driver gpio_switch_device_driver = {
	.probe		= gpio_switch_probe,
	.remove		= __devexit_p(gpio_switch_remove),
	.driver		= {
		.name	= "gpio_switch",
		.owner	= THIS_MODULE,
	}
};

static int __init gpio_switch_init(void)
{
	return platform_driver_register(&gpio_switch_device_driver);
}

static void __exit gpio_switch_exit(void)
{
	platform_driver_unregister(&gpio_switch_device_driver);
}

late_initcall(gpio_switch_init);
module_exit(gpio_switch_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phil Blundell <pb@handhelds.org>");
MODULE_DESCRIPTION("Keyboard driver for GPIOs");
MODULE_ALIAS("platform:gpio-keys");
