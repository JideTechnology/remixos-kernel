/*
 * devpolicy_mgr.c: Intel USB Power Delivery Device Manager Policy Driver
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
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/err.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/extcon.h>
#include <linux/usb_typec_phy.h>
#include "devpolicy_mgr.h"

static struct power_cap spcaps[] = {
	{
		.mv = VIN_5V,
		.ma = ICHRG_P5A,
	},
	{
		.mv = VIN_5V,
		.ma = ICHRG_1P5A,
	},
	{
		.mv = VIN_5V,
		.ma = ICHRG_3A,
	},
	{
		.mv = VIN_9V,
		.ma = ICHRG_1P5A,
	},
	{
		.mv = VIN_9V,
		.ma = ICHRG_3A,
	},
	{
		.mv = VIN_12V,
		.ma = ICHRG_1A,
	},
	{
		.mv = VIN_12V,
		.ma = ICHRG_3A,
	},
};

ATOMIC_NOTIFIER_HEAD(devpolicy_mgr_notifier);
EXPORT_SYMBOL_GPL(devpolicy_mgr_notifier);

static inline struct power_supply *dpm_get_psy(enum psy_type type)
{
	struct class_dev_iter iter;
	struct device *dev;
	static struct power_supply *psy;

	class_dev_iter_init(&iter, power_supply_class, NULL, NULL);
	while ((dev = class_dev_iter_next(&iter))) {
		psy = (struct power_supply *)dev_get_drvdata(dev);
		if ((type == PSY_TYPE_BATTERY && IS_BATTERY(psy)) ||
			(type == PSY_TYPE_CHARGER && IS_CHARGER(psy))) {
			class_dev_iter_exit(&iter);
			return psy;
		}
	}
	class_dev_iter_exit(&iter);

	return NULL;
}

/* Reading the state of charge value of the battery */
static inline int dpm_read_soc(int *soc)
{
	struct power_supply *psy;
	union power_supply_propval val;
	int ret;

	psy = dpm_get_psy(PSY_TYPE_BATTERY);
	if (!psy)
		return -EINVAL;

	ret = psy->get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &val);
	if (!ret)
		*soc = val.intval;

	return ret;
}

static int dpm_get_max_srcpwr_cap(struct devpolicy_mgr *dpm,
				struct power_cap *cap)
{
	cap->mv = VBUS_5V;
	cap->ma = IBUS_1A;
	return 0;
}

static int dpm_get_max_snkpwr_cap(struct devpolicy_mgr *dpm,
					struct power_cap *cap)
{
	cap->mv = VIN_12V;
	cap->ma = ICHRG_3A;
	return 0;
}

static int dpm_get_sink_power_cap(struct devpolicy_mgr *dpm,
					struct power_cap *cap)
{
	int soc;
	/* if the battery capacity is > 80% of cap, 5V, 3A
	 * <=80% of cpacity 12V, 3A.
	 */

	if (dpm_read_soc(&soc)) {
		pr_err("DPM: Error in getting soc\n");
		return -ENODATA;
	} else {
		pr_debug("DPM: capacity = %d\n", soc);
	}

	if (!IS_BATT_SOC_GOOD(soc))
		cap->mv = VIN_12V;
	else
		cap->mv = VIN_5V;

	cap->ma = ICHRG_3A;

	return 0;
}

static int dpm_get_sink_power_caps(struct devpolicy_mgr *dpm,
					struct power_caps *caps)
{

	if (spcaps == NULL)
		return -ENODATA;

	caps->pcap = spcaps;
	caps->n_cap = ARRAY_SIZE(spcaps);
	return 0;
}

static enum data_role dpm_get_data_role(struct devpolicy_mgr *dpm)
{
	return dpm->drole;
}

static enum pwr_role dpm_get_power_role(struct devpolicy_mgr *dpm)
{
	return dpm->prole;
}

static void dpm_set_data_role(struct devpolicy_mgr *dpm, enum data_role role)
{
	mutex_lock(&dpm->role_lock);
	if (dpm->drole != role)
		dpm->drole = role;
	mutex_unlock(&dpm->role_lock);
}

static void dpm_set_power_role(struct devpolicy_mgr *dpm, enum pwr_role role)
{
	mutex_lock(&dpm->role_lock);
	if (dpm->prole != role)
		dpm->prole = role;
	mutex_unlock(&dpm->role_lock);
}

static int dpm_set_charger_state(struct power_supply *psy, bool state)
{
	union power_supply_propval val;
	int ret;

	/* setting charger state enable/disable */
	val.intval = state;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ENABLE_CHARGER, &val);
	if (ret < 0)
		return ret;

	power_supply_changed(psy);
	return 0;
}

static int dpm_set_charger_mode(struct devpolicy_mgr *dpm,
					enum charger_mode mode)
{
	int ret = 0;
	struct power_supply *psy;

	mutex_lock(&dpm->charger_lock);

	psy = dpm_get_psy(PSY_TYPE_CHARGER);
	if (!psy) {
		mutex_unlock(&dpm->charger_lock);
		return -EINVAL;
	}

	switch (mode) {
	case CHARGER_MODE_SET_HZ:
		ret = dpm_set_charger_state(psy, false);
		break;
	case CHARGER_MODE_ENABLE:
		ret = dpm_set_charger_state(psy, true);
		break;
	default:
		break;
	}

	mutex_unlock(&dpm->charger_lock);

	return ret;
}

static int dpm_update_current_lim(struct devpolicy_mgr *dpm,
					int ilim)
{
	int ret = 0;
	struct power_supply *psy;
	union power_supply_propval val;

	mutex_lock(&dpm->charger_lock);

	psy = dpm_get_psy(PSY_TYPE_CHARGER);
	if (!psy) {
		mutex_unlock(&dpm->charger_lock);
		return -EINVAL;
	}

	/* reading current inlimit value */
	ret = psy->get_property(psy, POWER_SUPPLY_PROP_INLMT, &val);
	if (ret < 0) {
		pr_err("DPM: Unable to get the current limit (%d)\n", ret);
		goto error;
	}

	if (val.intval != ilim) {
		val.intval = ilim;
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_INLMT, &val);
		if (ret < 0) {
			pr_err("DPM: Unable to set the current limit (%d)\n",
					ret);
			goto error;
		}
		power_supply_changed(psy);
	}

error:
	mutex_unlock(&dpm->charger_lock);
	return ret;

}

static int dpm_get_min_current(struct devpolicy_mgr *dpm,
					int *ma)
{
	/* FIXME: this can be store it from and taken back when needed */
	*ma = ICHRG_P5A;

	return 0;
}

static enum cable_state dpm_get_cable_state(struct devpolicy_mgr *dpm,
					enum cable_type type)
{
	if (type == CABLE_TYPE_PROVIDER)
		return dpm->provider_state;
	else
		return dpm->consumer_state;
}

static void dpm_set_cable_state(struct devpolicy_mgr *dpm,
				enum cable_type type, enum cable_state state)
{
	mutex_lock(&dpm->cable_event_lock);
	if (type == CABLE_TYPE_PROVIDER) {
		if (dpm->provider_state != state) {
			dpm->provider_state = state;
			typec_notify_cable_state(dpm->phy, "USB-Host",
							(bool)state);
		}
	} else {
		if (dpm->consumer_state != state) {
			dpm->consumer_state = state;
			typec_notify_cable_state(dpm->phy, "USB", (bool)state);
		}
	}
	mutex_unlock(&dpm->cable_event_lock);
	return;
}

static int dpm_set_provider_state(struct devpolicy_mgr *dpm,
					enum cable_state state)
{
	dpm_set_cable_state(dpm, CABLE_TYPE_PROVIDER, state);
	return 0;
}

static int dpm_set_consumer_state(struct devpolicy_mgr *dpm,
					enum cable_state state)
{
	dpm_set_cable_state(dpm, CABLE_TYPE_CONSUMER, state);
	return 0;
}

static enum cable_state dpm_get_provider_state(struct devpolicy_mgr *dpm)
{
	return dpm_get_cable_state(dpm, CABLE_TYPE_PROVIDER);
}

static enum cable_state dpm_get_consumer_state(struct devpolicy_mgr *dpm)
{
	return dpm_get_cable_state(dpm, CABLE_TYPE_CONSUMER);
}

static void dpm_set_cur_data_role(struct devpolicy_mgr *dpm)
{
	enum cable_state state;

	state = dpm_get_cable_state(dpm, CABLE_TYPE_PROVIDER);
	if (state == CABLE_ATTACHED) {
		dpm_set_data_role(dpm, DATA_ROLE_DFP);
		return;
	}

	state = dpm_get_cable_state(dpm, CABLE_TYPE_CONSUMER);
	if (state == CABLE_ATTACHED)
		dpm_set_data_role(dpm, DATA_ROLE_UFP);
	else
		dpm_set_data_role(dpm, DATA_ROLE_NONE);
}

static void dpm_set_cur_power_role(struct devpolicy_mgr *dpm)
{
	enum cable_state state;

	state = dpm_get_cable_state(dpm, CABLE_TYPE_PROVIDER);
	if (state == CABLE_ATTACHED) {
		dpm_set_power_role(dpm, POWER_ROLE_SOURCE);
		return;
	}

	state = dpm_get_cable_state(dpm, CABLE_TYPE_CONSUMER);
	if (state == CABLE_ATTACHED)
		dpm_set_power_role(dpm, POWER_ROLE_SINK);
	else
		dpm_set_power_role(dpm, POWER_ROLE_NONE);
}

static void dpm_cable_worker(struct work_struct *work)
{
	struct devpolicy_mgr *dpm =
		container_of(work, struct devpolicy_mgr, cable_event_work);
	struct cable_event *evt, *tmp;
	unsigned long flags;
	enum cable_state provider_state;
	struct list_head new_list;

	if (list_empty(&dpm->cable_event_queue))
		return;

	INIT_LIST_HEAD(&new_list);
	spin_lock_irqsave(&dpm->cable_event_queue_lock, flags);
	list_replace_init(&dpm->cable_event_queue, &new_list);
	spin_unlock_irqrestore(&dpm->cable_event_queue_lock, flags);

	list_for_each_entry_safe(evt, tmp, &new_list, node) {
		pr_debug("DPM: %s Consumer - %s Provider - %s\n", __func__,
			evt->is_consumer_state ? "Connected" : "Disconnected",
			evt->is_provider_state ? "Connected" : "Disconnected");

		if (evt->is_consumer_state == CABLE_ATTACHED ||
			dpm->prev_cable_evt == CABLE_TYPE_CONSUMER) {
			dpm_set_cable_state(dpm, CABLE_TYPE_CONSUMER,
						evt->is_consumer_state);
			if  (dpm->prev_cable_evt == CABLE_TYPE_CONSUMER)
				dpm->prev_cable_evt = CABLE_TYPE_UNKNOWN;
			else
				 dpm->prev_cable_evt = CABLE_TYPE_CONSUMER;
		} else if (evt->is_provider_state == CABLE_ATTACHED ||
				dpm->prev_cable_evt == CABLE_TYPE_PROVIDER) {
			dpm_set_cable_state(dpm, CABLE_TYPE_PROVIDER,
						evt->is_provider_state);
			if  (dpm->prev_cable_evt == CABLE_TYPE_CONSUMER)
				dpm->prev_cable_evt = CABLE_TYPE_UNKNOWN;
			else
				dpm->prev_cable_evt = CABLE_TYPE_PROVIDER;
		}

		dpm_set_cur_power_role(dpm);
		dpm_set_cur_data_role(dpm);

		provider_state = dpm_get_cable_state(dpm, CABLE_TYPE_PROVIDER);
		if (provider_state == CABLE_ATTACHED) {
			/* FIXME: Notify Policy Engine to Advertise
			 * Source Capabilities, in case of provider mode
			 */
		}
		list_del(&evt->node);
		kfree(evt);
	}

	return;
}

static int dpm_cable_event(struct notifier_block *nblock,
						unsigned long event,
						void *param)
{
	struct devpolicy_mgr *dpm = container_of(nblock,
						struct devpolicy_mgr,
						nb);
	struct extcon_dev *edev = param;
	struct cable_event *evt;

	if (!edev)
		return NOTIFY_DONE;

	evt = kzalloc(sizeof(*evt), GFP_ATOMIC);
	if (!evt) {
		pr_err("DPM: failed to allocate memory for cable event\n");
		return NOTIFY_DONE;
	}

	evt->is_consumer_state = extcon_get_cable_state(edev, CABLE_CONSUMER);
	evt->is_provider_state = extcon_get_cable_state(edev, CABLE_PROVIDER);
	pr_debug("DPM: extcon notification evt Consumer - %s Provider - %s\n",
			evt->is_consumer_state ? "Connected" : "Disconnected",
			evt->is_provider_state ? "Connected" : "Disconnected");

	INIT_LIST_HEAD(&evt->node);
	spin_lock(&dpm->cable_event_queue_lock);
	list_add_tail(&evt->node, &dpm->cable_event_queue);
	spin_unlock(&dpm->cable_event_queue_lock);

	queue_work(system_nrt_wq, &dpm->cable_event_work);
	return NOTIFY_OK;
}

int devpolicy_mgr_reg_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&devpolicy_mgr_notifier, nb);
}
EXPORT_SYMBOL_GPL(devpolicy_mgr_reg_notifier);

void devpolicy_mgr_unreg_notifier(struct notifier_block *nb)
{
	atomic_notifier_chain_unregister(&devpolicy_mgr_notifier, nb);
}
EXPORT_SYMBOL_GPL(devpolicy_mgr_unreg_notifier);

static struct dpm_interface interface = {
	.get_max_srcpwr_cap = dpm_get_max_srcpwr_cap,
	.get_max_snkpwr_cap = dpm_get_max_snkpwr_cap,
	.get_sink_power_cap = dpm_get_sink_power_cap,
	.get_sink_power_caps = dpm_get_sink_power_caps,
	.set_provider_state = dpm_set_provider_state,
	.set_consumer_state = dpm_set_consumer_state,
	.get_provider_state = dpm_get_provider_state,
	.get_consumer_state = dpm_get_consumer_state,
	.get_data_role = dpm_get_data_role,
	.get_pwr_role = dpm_get_power_role,
	.set_data_role = dpm_set_data_role,
	.set_pwr_role = dpm_set_power_role,
	.set_charger_mode = dpm_set_charger_mode,
	.update_current_lim = dpm_update_current_lim,
	.get_min_current = dpm_get_min_current,
};

struct devpolicy_mgr *dpm_register_syspolicy(struct typec_phy *phy,
				struct pd_policy *policy)
{
	int ret;
	struct devpolicy_mgr *dpm;

	if (!phy) {
		pr_err("DPM: No typec phy!\n");
		return ERR_PTR(-ENODEV);
	}

	dpm = kzalloc(sizeof(struct devpolicy_mgr), GFP_KERNEL);
	if (!dpm) {
		pr_err("DPM: mem alloc failed\n");
		return ERR_PTR(-ENOMEM);
	}

	dpm->phy = phy;
	dpm->policy = policy;
	dpm->interface = &interface;
	INIT_LIST_HEAD(&dpm->cable_event_queue);
	INIT_WORK(&dpm->cable_event_work, dpm_cable_worker);
	spin_lock_init(&dpm->cable_event_queue_lock);
	mutex_init(&dpm->cable_event_lock);
	mutex_init(&dpm->role_lock);
	mutex_init(&dpm->charger_lock);
	dpm->prev_cable_evt = CABLE_TYPE_UNKNOWN;

	/* register for extcon notifier */
	dpm->nb.notifier_call = dpm_cable_event;
	ret = extcon_register_interest(&dpm->consumer_cable_nb,
						NULL,
						CABLE_CONSUMER,
						&dpm->nb);
	if (ret < 0) {
		pr_err("DPM: failed to register notifier for Consumer (%d)\n",
						ret);
		goto error0;
	}

	ret = extcon_register_interest(&dpm->provider_cable_nb,
						NULL,
						CABLE_PROVIDER,
						&dpm->nb);
	if (ret < 0) {
		pr_err("DPM: failed to register notifier for Provider\n");
		goto error1;
	}

	return dpm;

error1:
	extcon_unregister_interest(&dpm->consumer_cable_nb);
error0:
	kfree(dpm);
	return NULL;
}
EXPORT_SYMBOL(dpm_register_syspolicy);

void dpm_unregister_syspolicy(struct devpolicy_mgr *dpm)
{
	if (dpm) {
		extcon_unregister_interest(&dpm->provider_cable_nb);
		extcon_unregister_interest(&dpm->consumer_cable_nb);
		kfree(dpm);
	}
}
EXPORT_SYMBOL(dpm_unregister_syspolicy);

MODULE_AUTHOR("Albin B <albin.bala.krishnan@intel.com>");
MODULE_DESCRIPTION("PD Device Policy Manager");
MODULE_LICENSE("GPL v2");
