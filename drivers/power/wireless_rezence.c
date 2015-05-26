/*
 * wireless_rezence.c - REZENCE state machine
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author: K K, Vineesh <vineesh.k.k@intel.com>
 * Author: Griffoul, Sebastien <sebastien.griffoul@intel.com>
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/usb/otg.h>
#include <linux/version.h>

#include "wireless_rezence.h"
#include "power_supply_charger.h"

#define REZENCE_INIT_POWER	5000
#define REZENCE_INIT_VOLT	5000
#define REZENCE_INIT_CURRENT (REZENCE_INIT_POWER * 1000 / REZENCE_INIT_VOLT)

struct pru_dynamic_param {
	u8 optional_fields;
	u16 vrect;
	u16 irect;
	u16 vout;
	u16 iout;
	u8 temperature;
	u16 vrect_min_dyn;
	u16 vrect_set_dyn;
	u16 vrect_high_dyn;
	u8 pru_alert;
	u16 rfu1;
	u8 rfu2;
} __packed;

struct pru_static_param {
	u8 optional_fields;
	u8 protocol_rev;
	u8 rfu1;
	u8 pru_category;
	u8 pru_info;
	u8 hw_rev;
	u8 fw_rev;
	u8 prect_max;
	u16 vrect_min_stat;
	u16 vrect_high_stat;
	u16 vrect_set;
	u16 delta_r1_val;
	u32 rfu2;
} __packed;

struct wpr_state_attributes {
	unsigned long status;
	bool present;
};

enum wpr_event {
	WIRELESS_EVENT_UNKNOWN,
	WIRELESS_DRIVER_INIT,
	WIRELESS_DETACHED,
	WIRELESS_ATTACH,
	WIRELESS_SELECTED,
	WIRELESS_DESELECTED,
	WIRELESS_LINK_CONNECTED,
	WIRELESS_LINK_DISCONNECTED,
	WIRELESS_CHARGE_COMPLETE,
	WIRELESS_CHARGE_MAINTENANCE,
	WIRELESS_ERROR,
	WIRELESS_ERROR_RECOVER,
	WIRELESS_CHARGER_BATT_ERROR,
	WIRELESS_CHARGER_BATT_ERROR_RECOVERY,
	WIRELESS_PTU_STATIC_UPDATED,
};

struct pru_dynamic_param pru_dyn = { 0 };

struct wpr_state_attributes wpr_state[] = {
	{ POWER_SUPPLY_STATUS_PRU_NULL, 0 },
	{ POWER_SUPPLY_STATUS_PRU_NULL, 0 },
	{ POWER_SUPPLY_STATUS_PRU_NULL, 0 },
	{ POWER_SUPPLY_STATUS_PRU_NULL, 1},
	{ POWER_SUPPLY_STATUS_PRU_BOOT, 1 },
	{ POWER_SUPPLY_STATUS_PRU_BOOT, 0 },
	{ POWER_SUPPLY_STATUS_PRU_ON, 1 },
	{ POWER_SUPPLY_STATUS_PRU_NULL, 1 },
	{ POWER_SUPPLY_STATUS_PRU_ALERT, 1 },
	{ POWER_SUPPLY_STATUS_PRU_ALERT, 0 },
	{ POWER_SUPPLY_STATUS_PRU_ERROR, 1 },
	{ POWER_SUPPLY_STATUS_PRU_ON, 1 },
	{ POWER_SUPPLY_STATUS_PRU_ERROR, 1 },
	{ POWER_SUPPLY_STATUS_PRU_NULL, 1 },
	{ POWER_SUPPLY_STATUS_PRU_BOOT, 1 },
};

struct event_node {
	struct list_head node;
	unsigned long event;
	struct power_supply_cable_props cap;
	struct power_supply *psy;
};

static inline int set_ps_str_property(struct power_supply *psy,
					enum power_supply_property psp,
					char *strval)
{
	union power_supply_propval val;

	val.strval = strval;

	return psy->set_property(psy, psp, &val);
}

static inline int get_ps_str_property(struct power_supply *psy,
					enum power_supply_property psp,
					char *strval)
{
	union power_supply_propval val;

	psy->get_property(psy, psp, &val);
	memcpy(strval, val.strval, PSY_MAX_STR_PROP_LEN);

	return 0;
}

#define IS_WPR(psy) (psy->type == POWER_SUPPLY_TYPE_WIRELESS)

#define PRESENT(psy) \
		get_ps_int_property(psy, POWER_SUPPLY_PROP_PRESENT)

#define PTU_POWER(psy)\
		get_ps_int_property(psy, POWER_SUPPLY_PROP_PTU_POWER)

#define PTU_SET_POWER(psy, power)\
		set_ps_int_property(psy, POWER_SUPPLY_PROP_PTU_POWER, power)

#define PTU_SET_CLASS(psy, class)\
		set_ps_int_property(psy, POWER_SUPPLY_PROP_PTU_CLASS, class)

#define PRU_SET_DYNAMIC_PARAM(psy, str) \
		set_ps_str_property(psy,\
			POWER_SUPPLY_PROP_PRU_DYNAMIC_PARAM, str)

#define PRU_SET_STATIC_PARAM(psy, str) \
		set_ps_str_property(psy,\
			POWER_SUPPLY_PROP_PRU_STATIC_PARAM, str)

#define PTU_GET_STATIC_PARAM(psy, str) \
		get_ps_str_property(psy,\
			POWER_SUPPLY_PROP_PTU_STATIC_PARAM, str)

#define PRU_SET_DCEN(psy, val)\
	set_ps_int_property(psy, POWER_SUPPLY_PROP_PRU_DCEN, val)

#define PRU_SET_VOLT(psy, volt)\
		set_ps_int_property(psy, POWER_SUPPLY_PROP_VOLTAGE_MAX, volt)

#define PRU_SET_CURRENT(psy, cur)\
		set_ps_int_property(psy, POWER_SUPPLY_PROP_CURRENT_MAX, cur)

/* 250 ms delay */
#define DYNAMIC_PARAM_DELAY (HZ/4)

static int handle_psy_notifier(struct notifier_block *nb,
				   unsigned long event, void *data)
{
	struct wireless_pru *rez = container_of(nb,
				struct wireless_pru, psy_rez_nb);
	struct event_node *evt;

	if (data == NULL)
		return NOTIFY_DONE;

	evt = kzalloc(sizeof(*evt), GFP_ATOMIC);
	if (evt == NULL)
		return NOTIFY_DONE;

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
	spin_lock(&rez->evt_queue_lock);
	list_add_tail(&evt->node, &rez->evt_queue);
	spin_unlock(&rez->evt_queue_lock);
	queue_work(rez->notifier_wq, &rez->notifier_work);

	return NOTIFY_OK;
}

static void decode_ptu_stat_param(char *ptu_stat_str,
			struct ptu_static_param *ptu_stat)
{
	int ret;

	ret = sscanf(ptu_stat_str,
		"%hhX %hhX %hhX %hhX %hX %hhX %hhX %hhX %hhX %hhX %X %hX",
			&ptu_stat->optional_fields, &ptu_stat->ptu_power,
			&ptu_stat->ptu_max_src_impedance,
			&ptu_stat->ptu_max_load_res,
			&ptu_stat->rfu1, &ptu_stat->ptu_class,
			&ptu_stat->hw_rev, &ptu_stat->fw_rev,
			&ptu_stat->proto_rev, &ptu_stat->num_devs,
			&ptu_stat->rfu2, &ptu_stat->rfu3);
}

static void process_status(struct wireless_pru *rez,
			struct rezence_status *stat,
				struct rezence_status *prev_stat)
{
	enum wpr_event evt = WIRELESS_EVENT_UNKNOWN;

	if (stat->is_init != true) {
		evt =  WIRELESS_DRIVER_INIT;
		stat->is_init = true;
	} else if (stat->ptu_stat != prev_stat->ptu_stat) {
		switch (stat->ptu_stat) {
		case POWER_SUPPLY_CHARGER_EVENT_ATTACH:
			evt = WIRELESS_ATTACH;
			break;
		case POWER_SUPPLY_CHARGER_EVENT_DETACH:
			evt = WIRELESS_DETACHED;
			break;
		case POWER_SUPPLY_CHARGER_EVENT_SELECTED:
			evt = WIRELESS_SELECTED;
			break;
		case POWER_SUPPLY_CHARGER_EVENT_DESELECTED:
			evt = WIRELESS_DESELECTED;
			break;
		case POWER_SUPPLY_CHARGER_EVENT_LINK_DISCONNECT:
			evt = WIRELESS_LINK_DISCONNECTED;
			break;
		}
	} else if (stat->pru_stat != prev_stat->pru_stat) {
		switch (stat->pru_stat) {
		case POWER_SUPPLY_STATUS_PRU_ON:
			evt = WIRELESS_LINK_CONNECTED;
			break;
		}
	} else if (stat->battery_stat != prev_stat->battery_stat) {
		switch (stat->battery_stat) {
		case POWER_SUPPLY_STATUS_FULL:
			if (!stat->is_deselected) {
				/* Do not process any charge complete or
				* maintenance mode if A4WP is not
				* selected as a source for charging
				 */
				if (stat->is_charging_enabled)
					evt = WIRELESS_CHARGE_MAINTENANCE;
				else
					evt = WIRELESS_CHARGE_COMPLETE;
			}
			break;
		case POWER_SUPPLY_STATUS_NOT_CHARGING:
			if (stat->battery_health ==
				POWER_SUPPLY_HEALTH_OVERVOLTAGE) {
				evt = WIRELESS_CHARGER_BATT_ERROR;
			} else if (stat->charger_health !=
				POWER_SUPPLY_HEALTH_GOOD) {
				evt = WIRELESS_CHARGER_BATT_ERROR;
			}
			break;
		default:
			rez->is_charge_complete = false;
		}
	} else if (strncmp(stat->ptu_static_param, prev_stat->ptu_static_param,
				PSY_MAX_STR_PROP_LEN)) {
		evt = WIRELESS_PTU_STATIC_UPDATED;
	} else if (stat->is_deselected && !prev_stat->is_deselected) {
		evt = WIRELESS_DESELECTED;
	} else if (!stat->is_wired_mode && prev_stat->is_wired_mode) {
		/* No more in wire mode */
		rez->hide_present(rez, false, false);
		evt = WIRELESS_EVENT_UNKNOWN;
	}

	if (evt == WIRELESS_EVENT_UNKNOWN)
		return;

	if (STATUS(rez->psy_rez) != wpr_state[evt].status) {
		set_ps_int_property(rez->psy_rez, POWER_SUPPLY_PROP_STATUS,
			wpr_state[evt].status);
	}

	if (PRESENT(rez->psy_rez) != wpr_state[evt].present)
		set_ps_int_property(rez->psy_rez, POWER_SUPPLY_PROP_PRESENT,
			wpr_state[evt].present);

	switch (evt) {
	case WIRELESS_DRIVER_INIT:
		PRU_SET_STATIC_PARAM(rez->psy_rez, rez->pru_static_str);
		break;
	case WIRELESS_CHARGE_COMPLETE:
		PRU_SET_DCEN(rez->psy_rez, 0);
		rez->is_charge_complete = true;
		rez->hide_present(rez, true, false);
		break;
	case WIRELESS_CHARGE_MAINTENANCE:
		rez->is_charge_complete = false;
		rez->hide_present(rez, false, true);
		break;
	case WIRELESS_ATTACH:
		schedule_delayed_work(&rez->dynamic_param_work, 0);
		break;
	case WIRELESS_SELECTED:
		break;
	case WIRELESS_DESELECTED:
		PRU_SET_DCEN(rez->psy_rez, 0);
		/* If we are in wired mode, we should ensure that
		* present is force to 0 even if the PRU is on a PTU
		*/
		if (stat->is_wired_mode)
			rez->hide_present(rez, true, false);
		break;
	case WIRELESS_DETACHED:
		cancel_delayed_work_sync(&rez->dynamic_param_work);
		break;
	case WIRELESS_LINK_CONNECTED:
		PRU_SET_VOLT(rez->psy_rez, REZENCE_INIT_VOLT);

		PTU_SET_CLASS(rez->psy_rez, rez->ptu_stat.ptu_class);
		PTU_SET_POWER(rez->psy_rez, rez->ptu_stat.ptu_power);
		if (PTU_POWER(rez->psy_rez))
			PRU_SET_CURRENT(rez->psy_rez,
				PTU_POWER(rez->psy_rez)
					* 1000/REZENCE_INIT_VOLT);
		PRU_SET_DCEN(rez->psy_rez, 1);
		break;
	case WIRELESS_PTU_STATIC_UPDATED:
		PRU_SET_VOLT(rez->psy_rez, REZENCE_INIT_VOLT);
		PRU_SET_CURRENT(rez->psy_rez, REZENCE_INIT_CURRENT);

		PTU_GET_STATIC_PARAM(rez->psy_rez,
					stat->ptu_static_param);
		decode_ptu_stat_param(stat->ptu_static_param, &rez->ptu_stat);
		PTU_SET_POWER(rez->psy_rez, rez->ptu_stat.ptu_power);
		break;
	default:
		break;
	}
}

static inline int notify_pru_alert(struct wireless_pru *rez)
{
	int ret = 0;

	if (rez->is_charge_complete)
		ret |= PRU_ALERT_CHARGE_COMPLETE;
	if (rez->stat.is_wired_mode)
		ret |= PRU_ALERT_WIRECHARGER_DET;
	return ret;
}

static void dynamic_param_worker(struct work_struct *work)
{
	struct wireless_pru *rez = container_of(to_delayed_work(work),
						struct wireless_pru,
							dynamic_param_work);
	int ret;

	ret = rez->do_measurements(rez);
	if (ret < 0)
		goto err;

	ret = get_ps_int_property(rez->psy_rez,
				POWER_SUPPLY_PROP_VRECT);
	if (ret < 0)
		goto err;
	pru_dyn.vrect = (u16) ret;

	ret = get_ps_int_property(rez->psy_rez,
				POWER_SUPPLY_PROP_IRECT);
	if (ret < 0)
		goto err;
	pru_dyn.irect = (u16)ret;

	ret = get_ps_int_property(rez->psy_rez,
				POWER_SUPPLY_PROP_VDCOUT);
	if (ret < 0)
		goto err;
	pru_dyn.vout = (u16)ret;

	ret = get_ps_int_property(rez->psy_rez,
				POWER_SUPPLY_PROP_IDCOUT);
	if (ret < 0)
		goto err;
	pru_dyn.iout = (u16)ret;

	ret = get_ps_int_property(rez->psy_rez,
				POWER_SUPPLY_PROP_PRU_TEMP);
	if (ret < 0)
		goto err;
	pru_dyn.temperature = (u8)ret;

	pru_dyn.pru_alert = (u8)notify_pru_alert(rez);

	snprintf(rez->pru_dynamic_str, PSY_MAX_STR_PROP_LEN,
			"%X %X %X %X %X %X %X %X %X %X %X %X",
			pru_dyn.optional_fields, pru_dyn.vrect, pru_dyn.irect,
			pru_dyn.vout, pru_dyn.iout, pru_dyn.temperature,
			pru_dyn.vrect_min_dyn, pru_dyn.vrect_set_dyn,
			pru_dyn.vrect_high_dyn, pru_dyn.pru_alert,
			pru_dyn.rfu1, pru_dyn.rfu2);
	PRU_SET_DYNAMIC_PARAM(rez->psy_rez, rez->pru_dynamic_str);

err:
	schedule_delayed_work(&rez->dynamic_param_work, DYNAMIC_PARAM_DELAY);
}

static void process_event(struct wireless_pru *rez, struct event_node *evt)
{
	switch (evt->event) {
	case PSY_CABLE_EVENT:
		if (evt->cap.chrg_type ==
			POWER_SUPPLY_CHARGER_TYPE_WIRELESS) {
				rez->stat.ptu_stat = evt->cap.chrg_evt;
		} else {
			if (evt->cap.chrg_evt ==
					POWER_SUPPLY_CHARGER_EVENT_SELECTED) {
				/* An other cable is selected */
				rez->stat.is_deselected = true;
				rez->stat.is_wired_mode = true;
			}
		}
		break;
	case PSY_EVENT_PROP_CHANGED:
		if (IS_WPR(evt->psy)) {
			rez->stat.pru_health = HEALTH(evt->psy);
			rez->stat.pru_stat = STATUS(evt->psy);
			if (rez->stat.pru_stat >= POWER_SUPPLY_STATUS_PRU_BOOT)
				PTU_GET_STATIC_PARAM(rez->psy_rez,
						rez->stat.ptu_static_param);
			rez->stat.is_init = true;
		} else if (!rez->to_charger && IS_CHARGER(evt->psy)) {
			/* Not yet selected by any charger */
			if (CABLE_TYPE(evt->psy) ==
				POWER_SUPPLY_CHARGER_TYPE_WIRELESS) {
				/* This charger is now using us */
				rez->to_charger = evt->psy;
				rez->stat.charger_health = HEALTH(evt->psy);
				rez->stat.is_charging_enabled =
						IS_CHARGING_ENABLED(evt->psy);
				rez->stat.is_deselected = false;
				rez->stat.is_wired_mode = false;
			}
		} else if (rez->to_charger == evt->psy) {
			/* Event comes from the charger to which we are plug */
			int type = CABLE_TYPE(evt->psy);
			if (type ==
				POWER_SUPPLY_CHARGER_TYPE_WIRELESS) {
				/* Wireless Rez is the currently in used */
				rez->stat.charger_health = HEALTH(evt->psy);
				rez->stat.is_charging_enabled =
						IS_CHARGING_ENABLED(evt->psy);
				rez->stat.is_wired_mode = false;
				rez->stat.is_deselected = false;
			} else if (type == POWER_SUPPLY_CHARGER_TYPE_NONE) {
				/* Wireless-Rezence is no more use */
				rez->stat.is_deselected = true;
				rez->stat.is_wired_mode = false;
			} else {
				/* An other cable is used instead of us */
				rez->stat.is_wired_mode = true;
				rez->stat.is_deselected = true;
			}
		} else if (IS_BATTERY(evt->psy)) {
			rez->stat.battery_health = HEALTH(evt->psy);
			rez->stat.battery_stat = STATUS(evt->psy);
		}
		break;
	default:
		break;
	}

	process_status(rez, &rez->stat, &rez->prev_stat);
	rez->prev_stat = rez->stat;
}

static void notifier_event_worker(struct work_struct *work)
{
	struct wireless_pru *rez = container_of(work,
					struct wireless_pru, notifier_work);
	struct event_node *evt, *tmp;

	spin_lock(&rez->evt_queue_lock);
	list_for_each_entry_safe(evt, tmp, &rez->evt_queue, node) {
		list_del(&evt->node);
		spin_unlock(&rez->evt_queue_lock);

		process_event(rez, evt);

		kfree(evt);
		spin_lock(&rez->evt_queue_lock);
	}
	spin_unlock(&rez->evt_queue_lock);
}

static void init_pru_static(struct wireless_pru *pru)
{
	struct pru_static_param pru_stat = { 0 };

	pru_stat.protocol_rev = 1;
	pru_stat.pru_category = 3;
	pru_stat.hw_rev = 16;
	pru_stat.fw_rev = 16;
	pru_stat.prect_max = 90;
	pru_stat.vrect_min_stat = 6700;
	pru_stat.vrect_high_stat = 15000;
	pru_stat.vrect_set = 10000;
	pru_stat.pru_info = 232;

	snprintf(pru->pru_static_str, PSY_MAX_STR_PROP_LEN,
			"%X %X %X %X %X %X %X %X %X %X %X %X %X",
			pru_stat.optional_fields, pru_stat.protocol_rev,
			pru_stat.rfu1, pru_stat.pru_category, pru_stat.pru_info,
			pru_stat.hw_rev, pru_stat.fw_rev, pru_stat.prect_max,
			pru_stat.vrect_min_stat, pru_stat.vrect_high_stat,
			pru_stat.vrect_set, pru_stat.delta_r1_val,
			pru_stat.rfu2);
}

int wireless_rezence_register(struct wireless_pru *rez)
{
	/* Initialize rez structure */
	memset(&rez->stat, 0, sizeof(struct rezence_status));
	memset(&rez->prev_stat, 0, sizeof(struct rezence_status));

	rez->to_charger = NULL;
	rez->stat.is_deselected = true;
	rez->prev_stat.is_deselected = true;

	rez->notifier_wq = create_singlethread_workqueue("psy_rez");
	if (!rez->notifier_wq)
		return -EINVAL;

	INIT_WORK(&rez->notifier_work, notifier_event_worker);
	INIT_LIST_HEAD(&rez->evt_queue);
	INIT_DELAYED_WORK(&rez->dynamic_param_work, dynamic_param_worker);
	spin_lock_init(&rez->evt_queue_lock);

	init_pru_static(rez);

	rez->psy_rez_nb.notifier_call = handle_psy_notifier;
	power_supply_reg_notifier(&rez->psy_rez_nb);

	return 0;
}

int wireless_rezence_unregister(struct wireless_pru *pru)
{
	return 0;
}
