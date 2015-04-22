/*
 * BLE gpio driver
 * Copyright (c) 2014, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * Author: Vineesh K K <vineesh.k.k@intel.com>
 * Author: Pavan Kumar S <pavan.kumar.s@intel.com>
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/interrupt.h>
#include <linux/acpi.h>
#include "power_supply.h"
#include "power_supply_charger.h"

#define DRV_NAME "INTA4322"
#define DEV_NAME "INTA4322"

#define BLE_DISABLE		1
#define BLE_ENABLE		0

#define BLE_CHRG_COMPL		1
#define BLE_CHRG_RENBL		0

#define TRANS_DELAY		(100 * HZ/1000)
#define HOSTON_LEN		7


struct ble_chip {
	char ble_mode[HOSTON_LEN];
	int present;
	int ishoston;
	struct gpio_desc *ble_dis_desc;
	struct gpio_desc *chrg_cmplt_desc;
	struct gpio_desc *mode_trans_desc;
	struct device *dev;
	struct mutex lock;
	struct notifier_block ble_psy_nb;
	struct list_head evt_queue;
	struct workqueue_struct *notifier_wq;
	struct work_struct notifier_work;
	struct delayed_work ble_dis;
	struct delayed_work mode_trans;
	spinlock_t evt_queue_lock;
};

struct event_node {
	struct list_head node;
	unsigned long event;
	struct power_supply_cable_props cap;
	struct power_supply *psy;
};

static void update_gpio(struct ble_chip *chip,
		struct gpio_desc *gpio_desc, int value)
{
	int ret;
	ret = gpiod_direction_output(gpio_desc, value);
	if (ret)
		dev_err(chip->dev,
			"%s: Error (%d) writing value %d to ble gpio\n",
			__func__, ret, value);
}

static void ble_disable_worker(struct work_struct *work)
{
	struct ble_chip *chip = container_of(work, struct ble_chip,
					ble_dis);

	update_gpio(chip, chip->ble_dis_desc, 1);
	dev_dbg(chip->dev,
		"BLE disabled\n");
}

static void mode_trans_worker(struct work_struct *work)
{
	struct ble_chip *chip = container_of(work, struct ble_chip,
					mode_trans);

	update_gpio(chip, chip->mode_trans_desc, 0);
	dev_dbg(chip->dev,
		"Mode transition initiated\n");
	/* Disable BLE after 100ms */
	schedule_delayed_work(&chip->ble_dis,
				TRANS_DELAY);
}

static ssize_t ble_get_status(struct device *dev,
			    struct device_attribute *attr, char *buf)
{

	struct ble_chip *chip = dev_get_drvdata(dev);

	int ret;

	ret = sprintf(buf, "%s\n", chip->ble_mode);

	return ret;
}

static ssize_t ble_set_status(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{

	struct ble_chip *chip = dev_get_drvdata(dev);

	if (!strncmp(buf, "onhost", strlen("onhost"))) {

		if (chip->present) {
			update_gpio(chip, chip->mode_trans_desc, 1);
			/* 100ms width pulse for mode transition */
			schedule_delayed_work(&chip->mode_trans,
				TRANS_DELAY);
		}
		chip->ishoston = 1;
		strncpy(chip->ble_mode, buf, HOSTON_LEN);
	} else {
		update_gpio(chip, chip->mode_trans_desc, 0);
		update_gpio(chip, chip->ble_dis_desc, 0);
		chip->ishoston = 0;
	}

	return count;
}

static DEVICE_ATTR(ble_mode, S_IWUSR | S_IWGRP | S_IRUGO,
		ble_get_status, ble_set_status);

static void notifier_event_worker(struct work_struct *work)
{
	struct ble_chip *chip = container_of(work, struct ble_chip,
					notifier_work);
	struct event_node *evt, *tmp;

	spin_lock(&chip->evt_queue_lock);
	list_for_each_entry_safe(evt, tmp, &chip->evt_queue, node) {
		list_del(&evt->node);
		spin_unlock(&chip->evt_queue_lock);
		switch (evt->event) {
		case PSY_CABLE_EVENT:
		break;
		case PSY_EVENT_PROP_CHANGED:
			if (IS_BATTERY(evt->psy)) {
				if (STATUS(evt->psy) ==
					POWER_SUPPLY_STATUS_FULL) {
					dev_dbg(chip->dev,
					"%s: Batt.status FULL, Update nordic",
					__func__);
					update_gpio(chip,
						chip->chrg_cmplt_desc,
						BLE_CHRG_COMPL);
				} else {
					update_gpio(chip,
						chip->chrg_cmplt_desc,
						BLE_CHRG_RENBL);
				}
			} else if (IS_CHARGER(evt->psy)) {
				/* evt->cap.chrg_type is UNKNOWN initially */
				if (!strncmp(evt->psy->name, "nx2a4wp_wpr",
						strlen("nx2a4wp_wpr")))
					chip->present =
						IS_PRESENT(evt->psy);
				/* ble_dis set to 0 for detach. BLE Disabled
				and no mode transition required when attach
				comes after onchipon, */
				if (chip->present &&
						chip->ishoston) {
					update_gpio(chip,
						chip->ble_dis_desc, 1);
				} else
					update_gpio(chip,
						chip->ble_dis_desc, 0);

			}
		}

		spin_lock(&chip->evt_queue_lock);
		kfree(evt);
	}
	spin_unlock(&chip->evt_queue_lock);
}

static int handle_psy_notification(struct notifier_block *nb,
				   unsigned long event, void *data)
{
	struct ble_chip *chip = container_of(nb, struct ble_chip,
					ble_psy_nb);
	struct event_node *evt;

	evt = kzalloc(sizeof(*evt), GFP_ATOMIC);
	if (evt == NULL)
		return -ENOMEM;
	if (data == NULL) {
		kfree(evt);
		return NOTIFY_DONE;
	}
	evt->event = event;
	switch (event) {
	case PSY_CABLE_EVENT:
		evt->cap = *(struct power_supply_cable_props *)data;
		break;
	case PSY_EVENT_PROP_CHANGED:
		evt->psy = data;
		break;
	case PSY_BATTERY_EVENT:
	default:
		kfree(evt);
		return NOTIFY_DONE;
	}

	INIT_LIST_HEAD(&evt->node);
	spin_lock(&chip->evt_queue_lock);
	list_add_tail(&evt->node, &chip->evt_queue);
	spin_unlock(&chip->evt_queue_lock);
	queue_work(chip->notifier_wq, &chip->notifier_work);

	return NOTIFY_OK;
}

static int ble_gpio_probe(struct platform_device *pdev)
{
	struct ble_chip *chip;
	int ret = 0;

	dev_info(&pdev->dev, "%s\n", __func__);

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		dev_err(&pdev->dev, "mem alloc for chip failed:%d\n", ret);
		return -ENOMEM;
	}

	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);
	mutex_init(&chip->lock);

	chip->notifier_wq = create_singlethread_workqueue("psy_ble");
	if (!chip->notifier_wq) {
		kfree(chip);
		return -EINVAL;
	}
	INIT_WORK(&chip->notifier_work, notifier_event_worker);
	INIT_LIST_HEAD(&chip->evt_queue);
	spin_lock_init(&chip->evt_queue_lock);

	chip->ble_psy_nb.notifier_call = handle_psy_notification;
	ret = power_supply_reg_notifier(&chip->ble_psy_nb);
	if (ret) {
		dev_err(&pdev->dev,
			"failed:%d to register power_supply notifier\n",
				ret);
		goto err_free;
	}
	if (ACPI_HANDLE(chip->dev)) {

		dev_dbg(chip->dev, "%s ACPI Handle\n", __func__);
		/* BLE disable  */
		chip->ble_dis_desc = devm_gpiod_get_index(chip->dev,
			"ble_disable", 0);
		if (IS_ERR_OR_NULL(chip->ble_dis_desc))
			dev_err(chip->dev,
				"Failed to get the BLE DISABLE Pin Index\n");
		else {
			ret = gpiod_direction_output(
				chip->ble_dis_desc, 0);
			if (ret < 0)
				dev_err(chip->dev,
					"BLE GPIO set direction failed\n");
			gpiod_export(chip->ble_dis_desc, 1);
		}

		/* charge Complete */
		chip->chrg_cmplt_desc = devm_gpiod_get_index(chip->dev,
			"chrg_complete", 2);
		if (IS_ERR_OR_NULL(chip->chrg_cmplt_desc))
			dev_err(chip->dev,
				"Failed to get the CHARGE CMPLT Pin Index\n");
		else {
			ret = gpiod_direction_output
				(chip->chrg_cmplt_desc, 0);
			if (ret < 0)
				dev_err(chip->dev,
					"BLE GPIO set direction failed\n");
			gpiod_export(chip->chrg_cmplt_desc, 1);
		}

		/* Mode transition */
		chip->mode_trans_desc = devm_gpiod_get_index(chip->dev,
			"mode_trans", 1);
		if (IS_ERR_OR_NULL(chip->mode_trans_desc))
			dev_err(chip->dev,
				"Failed to get the MODE TRANS Pin Index\n");
		else {
			ret = gpiod_direction_output(
				chip->mode_trans_desc, 0);
			if (ret < 0)
				dev_err(chip->dev,
					"BLE GPIO set direction failed\n");
			gpiod_export(chip->mode_trans_desc, 1);
		}
	}

	ret = device_create_file(&pdev->dev,
			&dev_attr_ble_mode);
	if (ret) {
		dev_err(&pdev->dev,
				"Failed to create sysfs: ble_mode\n");
	}

	dev_dbg(&pdev->dev, "%s completed successful\n", __func__);
	INIT_DELAYED_WORK(&chip->ble_dis, ble_disable_worker);
	INIT_DELAYED_WORK(&chip->mode_trans, mode_trans_worker);
	return 0;

err_free:
	kfree(chip);

	return ret;
}

static int ble_gpio_remove(struct platform_device *pdev)
{
	struct ble_chip *chip = platform_get_drvdata(pdev);

	device_remove_file(&pdev->dev, &dev_attr_ble_mode);
	power_supply_unreg_notifier(&chip->ble_psy_nb);
	kfree(chip);
	return 0;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id gpio_ble_acpi_ids[] = {
	{DEV_NAME},
	{},
};
MODULE_DEVICE_TABLE(acpi, gpio_ble_acpi_ids);
#endif
static const struct platform_device_id ble_gpio_id[] = {
	{DEV_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(platform, ble_gpio_id);

static struct platform_driver ble_gpio_driver = {
	.probe = ble_gpio_probe,
	.remove = ble_gpio_remove,
	.driver = {
		.name = DRV_NAME,
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(gpio_ble_acpi_ids),
#endif
	},
};
/*module_platform_driver(ble_gpio_driver);*/

static int __init ble_gpio_init(void)
{
	return platform_driver_register(&ble_gpio_driver);
}
module_init(ble_gpio_init);

static void __exit ble_gpio_exit(void)
{
	platform_driver_unregister(&ble_gpio_driver);
}
module_exit(ble_gpio_exit);

MODULE_AUTHOR("Pavan Kumar S <pavan.kumar.s@intel.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("BLE GPIO driver");

