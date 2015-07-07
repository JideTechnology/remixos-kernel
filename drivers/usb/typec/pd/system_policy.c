/*
 * system_policy.c: Intel USBC Power Delivery System Policy
 *
 * Copyright (C) 2015 Intel Corporation
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. Seee the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author: Albin B <albin.bala.krishnan@intel.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/err.h>
#include "system_policy.h"

/* exported for the USBC Power Delivery polices */
struct class *power_delivery_class;
EXPORT_SYMBOL_GPL(power_delivery_class);

/* it is a singleton class for a system */
static struct system_policy *spolicy;

static enum policy_type policies[] = {
	POLICY_TYPE_SINK,
};

/* FIXME: now supports only one port */
static struct pd_policy pdpolicy = {
	.policies = policies,
	.num_policies = ARRAY_SIZE(policies),
};

/**
 * syspolicy_register_typec_phy - register typec phy with system policy
 * @x: the typec_phy to be used; or NULL
 *
 * This call is exclusively for use by typec phy drivers, which
 * coordinate the activities of drivers for power delivery support.
 */
int syspolicy_register_typec_phy(struct typec_phy *x)
{
	int			ret = 0;
	unsigned long		flags;
	struct device_list	*devlist, *tmp;
	struct devpolicy_mgr	*dpm;
	bool			is_pd_supported;

	if (!x || !x->dev) {
		pr_err("SYS_POLICY: Invalid Phy!\n");
		return -EINVAL;
	}

	if (x->type != USB_TYPE_C) {
		pr_err("SYS_POLICY: not accepting PHY %s\n",
				x->label);
		return -EINVAL;
	}

	spin_lock_irqsave(&spolicy->lock, flags);
	list_for_each_entry_safe(devlist, tmp, &spolicy->list, list) {
		if (devlist->phy == x) {
			pr_err("SYS_POLICY: Phy is already exists\n");
			spin_unlock_irqrestore(&spolicy->lock, flags);
			return -EBUSY;
		}
	}
	spin_unlock_irqrestore(&spolicy->lock, flags);

	devlist = kzalloc(sizeof(struct device_list), GFP_KERNEL);
	if (!devlist) {
		dev_err(x->dev, "SYS_POLICY: kzalloc failed\n");
		ret = -ENOMEM;
		goto fail;
	}
	INIT_LIST_HEAD(&devlist->list);
	devlist->phy = x;

	is_pd_supported = devlist->phy->is_pd_capable(devlist->phy);
	if (!is_pd_supported) {
		dev_err(x->dev, "SYS_POLICY: PD protocol not supported!\n");
		ret = -EPROTONOSUPPORT;
		goto fail;
	}

	dpm = dpm_register_syspolicy(devlist->phy, &pdpolicy);
	if (IS_ERR_OR_NULL(dpm)) {
		dev_err(x->dev, "SYS_POLICY: Unable to register dpm!\n");
		ret = -ENODEV;
		goto fail;
	}

	devlist->dpm = dpm;
	list_add_tail(&devlist->list, &spolicy->list);
	return 0;

fail:
	kfree(devlist);
	return ret;
}
EXPORT_SYMBOL_GPL(syspolicy_register_typec_phy);

/**
 * syspolicy_unregister_typec_phy - unregister the usb typec PHY
 * @x: the USB Typec-C phy to be removed;
 *
 * This reverts the effects of syspolicy_register_typec_phy
 */
void syspolicy_unregister_typec_phy(struct typec_phy *x)
{
	unsigned long	flags;
	struct device_list *devlist, *tmp;

	if (list_empty(&spolicy->list))
		return;

	spin_lock_irqsave(&spolicy->lock, flags);
	if (x) {
		list_for_each_entry_safe(devlist, tmp, &spolicy->list, list) {
			if (devlist->phy == x) {
				list_del(&devlist->list);
				dpm_unregister_syspolicy(devlist->dpm);
				kfree(devlist);
				break;
			}
		}
	}
	spin_unlock_irqrestore(&spolicy->lock, flags);
}
EXPORT_SYMBOL_GPL(syspolicy_unregister_typec_phy);

static int __init system_policy_init(void)
{
	int ret;

	power_delivery_class = class_create(THIS_MODULE, "power_delivery");

	if (IS_ERR(power_delivery_class))
		return PTR_ERR(power_delivery_class);

	spolicy = kzalloc(sizeof(struct system_policy), GFP_KERNEL);
	if (!spolicy) {
		pr_err("SYS_POLICY: %s kzalloc failed\n", __func__);
		ret = -ENOMEM;
		goto exit;
	}
	INIT_LIST_HEAD(&spolicy->list);
	spin_lock_init(&spolicy->lock);

	return 0;
exit:
	return ret;
}
subsys_initcall(system_policy_init);

static void __exit system_policy_exit(void)
{
	class_destroy(power_delivery_class);
	if (spolicy)
		kfree((void *)spolicy);
}
module_exit(system_policy_exit);

MODULE_AUTHOR("Albin B <albin.bala.krishnan@intel.com>");
MODULE_DESCRIPTION("Intel PD System Policy");
MODULE_LICENSE("GPL v2");
