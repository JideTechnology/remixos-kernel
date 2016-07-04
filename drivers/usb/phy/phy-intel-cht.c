/*
 * Intel CherryTrail USB OTG transceiver driver
 *
 * Copyright (C) 2014, Intel Corporation.
 *
 * Author: Wu, Hao
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program;
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/usb.h>
#include <linux/usb/gadget.h>
#include <linux/usb/hcd.h>
#include <linux/extcon.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include "../host/xhci.h"
#include "../host/xhci-intel-cap.h"
#include <linux/wakelock.h>
#include <asm/intel_em_config.h>
#include "phy-intel-cht.h"

#define DRIVER_VERSION "Rev. 0.5"
#define DRIVER_AUTHOR "Hao Wu"
#define DRIVER_DESC "Intel CherryTrail USB OTG Transceiver Driver"
#define DRIVER_INFO DRIVER_DESC " " DRIVER_VERSION

#define FPO0_USB_COMP_OFFSET	0x01

#define USB_VBUS_DRAW_HIGH	500
#define USB_VBUS_DRAW_SUPER	900


static const char driver_name[] = "intel-cht-otg";

static struct cht_otg *cht_otg_dev;
struct wake_lock otg_wakelock;

static int edev_state_to_id(struct extcon_dev *evdev)
{
	int state;

	state = extcon_get_cable_state(evdev, "USB-Host");

	return !evdev->state;
}

/* If this watchdog timeout and executed, that means DCP is connected.
 * Just notify USB_EVENT_NONE, to make sure USB disconnect and stay in
 * low power state */
static void cht_otg_device_watchdog(struct work_struct *work)
{
	if (!cht_otg_dev)
		return;

	dev_info(cht_otg_dev->phy.dev, "not enumerated, so DCP connected?\n");
	atomic_notifier_call_chain(&cht_otg_dev->phy.notifier,
				USB_EVENT_NONE, NULL);
}

static void cht_otg_device_start_watchdog(struct cht_otg *otg_dev)
{
	if (schedule_delayed_work(&otg_dev->watchdog, msecs_to_jiffies(300000)))
		dev_info(otg_dev->phy.dev, "watchdog started\n");
	else
		dev_info(otg_dev->phy.dev, "watchdog already started\n");
}

static void cht_otg_device_stop_watchdog(struct cht_otg *otg_dev)
{
	if (cancel_delayed_work(&cht_otg_dev->watchdog))
		dev_info(otg_dev->phy.dev, "watchdog stopped\n");
}

static int cht_otg_set_id_mux(struct cht_otg *otg_dev, int id)
{
	struct usb_bus *host = otg_dev->phy.otg->host;
	struct usb_gadget *gadget = otg_dev->phy.otg->gadget;
	struct usb_hcd *hcd;
	struct xhci_hcd *xhci;

	if (!host || !gadget || !gadget->dev.parent)
		return -ENODEV;

	hcd = bus_to_hcd(host);
	xhci = hcd_to_xhci(hcd);
	/* HACK: PHY used in Cherrytrail is shared by both host and device
	 * controller, it requires both host and device controller to be D0
	 * for any action related to PHY transition */
	pm_runtime_get_sync(host->controller);
	pm_runtime_get_sync(gadget->dev.parent);

	xhci_intel_phy_mux_switch(xhci, id);

	pm_runtime_put(gadget->dev.parent);
	pm_runtime_put(host->controller);

	return 0;
}

static int cht_otg_set_vbus_valid(struct cht_otg *otg_dev, int vbus_valid)
{
	struct usb_bus *host = otg_dev->phy.otg->host;
	struct usb_gadget *gadget = otg_dev->phy.otg->gadget;
	struct usb_hcd *hcd;
	struct xhci_hcd *xhci;

	if (!host || !gadget || !gadget->dev.parent)
		return -ENODEV;

	hcd = bus_to_hcd(host);
	xhci = hcd_to_xhci(hcd);

	/* HACK: PHY used in Cherrytrail is shared by both host and device
	 * controller, it requires both host and device controller to be D0
	 * for any action related to PHY transition */
	pm_runtime_get_sync(host->controller);
	pm_runtime_get_sync(gadget->dev.parent);

	xhci_intel_phy_vbus_valid(xhci, vbus_valid);

	pm_runtime_put(gadget->dev.parent);
	pm_runtime_put(host->controller);

	return 0;
}

static int cht_otg_start_host(struct otg_fsm *fsm, int on)
{
	struct usb_otg *otg = fsm->otg;
	struct cht_otg *otg_dev = container_of(otg->phy, struct cht_otg, phy);
	int retval;

	dev_dbg(otg->phy->dev, "%s --->\n", __func__);

	if (!otg->host)
		return -ENODEV;

	/* Just switch the mux to host path */
	retval = cht_otg_set_id_mux(otg_dev, !on);

	dev_dbg(otg->phy->dev, "%s <---\n", __func__);

	return retval;
}

static int cht_otg_start_gadget(struct otg_fsm *fsm, int on)
{
	struct usb_otg *otg = fsm->otg;
	struct cht_otg *otg_dev = container_of(otg->phy, struct cht_otg, phy);
	int retval;

	dev_dbg(otg->phy->dev, "%s --->\n", __func__);

	if (!otg->gadget)
		return -ENODEV;

	/* Just switch the mux to host path */
	retval = cht_otg_set_id_mux(otg_dev, on);

	if (on)
		cht_otg_device_start_watchdog(otg_dev);
	else
		cht_otg_device_stop_watchdog(otg_dev);

	retval = cht_otg_set_vbus_valid(otg_dev, on);

	dev_dbg(otg->phy->dev, "%s <---\n", __func__);

	return retval;
}

/* SRP / HNP / ADP are not supported, only simple dual role function
 * start gadget function is not implemented as controller will take
 * care itself per VBUS event */
static struct otg_fsm_ops cht_otg_ops = {
	.start_host = cht_otg_start_host,
	.start_gadget = cht_otg_start_gadget,
};

static int cht_otg_set_power(struct usb_phy *phy, unsigned mA)
{
	struct usb_gadget *gadget = cht_otg_dev->fsm.otg->gadget;

	dev_dbg(phy->dev, "%s --->\n", __func__);

	if (!cht_otg_dev)
		return -ENODEV;

	/* VBUS drop event from EM part may come firstly to phy-intel-cht and
	 * suspend event from device stack may come later than the VBUS drop
	 * event. suspend event will trigger usb_gadget_vbus_draw 2mA when
	 * b_sess_vld is already clear to 0. It will cause conflict events
	 * received by other components, (e.g otg-wakelock) */
	if (!cht_otg_dev->fsm.b_sess_vld)
		return -ENODEV;

	if (phy->state != OTG_STATE_B_PERIPHERAL)
		dev_err(phy->dev, "ERR: Draw %d mA in state %s\n",
			mA, usb_otg_state_string(cht_otg_dev->phy.state));

	/* If it is not staying at suspended state (<=2.5mA), then it is not
	 * a DCP. stop the watchdog.
	 */
	if (mA >= 3)
		cht_otg_device_stop_watchdog(cht_otg_dev);

	/* For none-compliance mode, ignore the given value
	 * and always draw maxium. */
	if (!cht_otg_dev->compliance) {
		if (gadget->speed == USB_SPEED_SUPER)
			mA = USB_VBUS_DRAW_SUPER;
		else
			mA = USB_VBUS_DRAW_HIGH;
	}
	mA = 1500;

	/* Notify other drivers that device enumerated or not.
	 * e.g It is needed by some charger driver, to set
	 * charging current for SDP case */
	atomic_notifier_call_chain(&cht_otg_dev->phy.notifier,
					USB_EVENT_ENUMERATED, &mA);
	dev_info(phy->dev, "Draw %d mA\n", mA);

	dev_dbg(phy->dev, "%s <---\n", __func__);

	return 0;
}

/*
 * Called by initialization code of host driver. Register host
 * controller to the OTG.
 */
static int cht_otg_set_host(struct usb_otg *otg, struct usb_bus *host)
{
	struct cht_otg *otg_dev;

	if (!otg || !host)
		return -ENODEV;

	otg_dev = container_of(otg->phy, struct cht_otg, phy);
	if (otg_dev != cht_otg_dev)
		return -EINVAL;

	otg->host = host;

	/* once host registered, then kick statemachine to move
	 * to A_HOST if id is grounded */
	otg_dev->fsm.a_bus_drop = 0;
	otg_dev->fsm.a_bus_req = 0;

	/* check default ID value when host registered */
	if (otg_dev->cable_nb.edev)
		otg_dev->fsm.id = edev_state_to_id(otg_dev->cable_nb.edev);

	/* if gadget is registered already then kick the state machine.
	 * Only trigger the mode switch once both host and device are
	 * registered */
	if (otg_dev->phy.otg->gadget)
		otg_statemachine(&otg_dev->fsm);

	return 0;
}

/*
 * Called by initialization code of udc. Register udc to OTG.
 */
static int cht_otg_set_peripheral(struct usb_otg *otg,
					struct usb_gadget *gadget)
{
	struct cht_otg *otg_dev;

	if (!otg || !gadget)
		return -ENODEV;

	otg_dev = container_of(otg->phy, struct cht_otg, phy);
	if (otg_dev != cht_otg_dev)
		return -EINVAL;

	otg->gadget = gadget;
	otg->gadget->is_otg = 1;

	otg_dev->fsm.b_bus_req = 1;

	/* if host is registered already then kick the state machine.
	 * Only trigger the mode switch once both host and device are
	 * registered */
	if (otg_dev->phy.otg->host)
		otg_statemachine(&otg_dev->fsm);

	return 0;
}

static int cht_otg_start(struct platform_device *pdev)
{
	struct cht_otg *otg_dev;
	struct usb_phy *phy_dev;
	struct otg_fsm *fsm;

	phy_dev = usb_get_phy(USB_PHY_TYPE_USB2);
	if (!phy_dev)
		return -ENODEV;

	otg_dev = container_of(phy_dev, struct cht_otg, phy);
	fsm = &otg_dev->fsm;

	/* Initialize the state machine structure with default values */
	phy_dev->state = OTG_STATE_UNDEFINED;
	fsm->otg = otg_dev->phy.otg;
	mutex_init(&fsm->lock);

	/* Initialize the id value to default 1 */
	fsm->id = 1;
	otg_statemachine(fsm);

	dev_dbg(&pdev->dev, "initial ID pin set to %d\n", fsm->id);

	return 0;
}

static void cht_otg_stop(struct platform_device *pdev)
{
	if (!cht_otg_dev)
		return;

	if (cht_otg_dev->regs)
		iounmap(cht_otg_dev->regs);
}

static int cht_host_gadget_ready(struct cht_otg *otg_dev)
{
	if (otg_dev && otg_dev->phy.otg->host && otg_dev->phy.otg->gadget)
		return 1;

	return 0;
}

static int cht_otg_handle_notification(struct notifier_block *nb,
				unsigned long event, void *data)
{
	int state;

	if (!cht_otg_dev)
		return NOTIFY_BAD;

	switch (event) {
	/* USB_EVENT_VBUS: vbus valid event */
	case USB_EVENT_VBUS:
		dev_info(cht_otg_dev->phy.dev, "USB_EVENT_VBUS vbus valid\n");
		if (cht_otg_dev->fsm.id)
			cht_otg_dev->fsm.b_sess_vld = 1;
		/* don't kick the state machine if host or device controller
		 * is not registered. Just wait to kick it when set_host or
		 * set_peripheral.*/
		if (cht_host_gadget_ready(cht_otg_dev))
			schedule_work(&cht_otg_dev->fsm_work);
		state = NOTIFY_OK;
		break;
	/* USB_EVENT_ID: id was grounded */
	case USB_EVENT_ID:
		dev_info(cht_otg_dev->phy.dev, "USB_EVENT_ID id ground\n");
		cht_otg_dev->fsm.id = 0;
		/* Same as above */
		if (cht_host_gadget_ready(cht_otg_dev))
			schedule_work(&cht_otg_dev->fsm_work);
		state = NOTIFY_OK;
		break;
	/* USB_EVENT_NONE: no events or cable disconnected */
	case USB_EVENT_NONE:
		dev_info(cht_otg_dev->phy.dev,
					"USB_EVENT_NONE cable disconnected\n");
		if (cht_otg_dev->fsm.id == 0)
			cht_otg_dev->fsm.id = 1;
		else if (cht_otg_dev->fsm.b_sess_vld)
			cht_otg_dev->fsm.b_sess_vld = 0;
		else
			dev_err(cht_otg_dev->phy.dev, "why USB_EVENT_NONE?\n");
		/* Same as above */
		if (cht_host_gadget_ready(cht_otg_dev))
			schedule_work(&cht_otg_dev->fsm_work);
		state = NOTIFY_OK;
		break;
	default:
		dev_info(cht_otg_dev->phy.dev, "unknown notification\n");
		state = NOTIFY_DONE;
		break;
	}

	return state;
}

static void cht_otg_fsm_work(struct work_struct *work)
{
	if (!cht_otg_dev)
		return;

	otg_statemachine(&cht_otg_dev->fsm);
}

static int fsm_show(struct seq_file *s, void *unused)
{
	struct cht_otg *otg_dev = s->private;
	struct otg_fsm *fsm = &otg_dev->fsm;

	mutex_lock(&fsm->lock);

	seq_printf(s, "OTG state: %s\n\n",
			usb_otg_state_string(otg_dev->phy.state));

	seq_printf(s, "a_bus_req: %d\n"
			"b_bus_req: %d\n"
			"a_bus_resume: %d\n"
			"a_bus_suspend: %d\n"
			"a_conn: %d\n"
			"a_sess_vld: %d\n"
			"a_srp_det: %d\n"
			"a_vbus_vld: %d\n"
			"b_bus_resume: %d\n"
			"b_bus_suspend: %d\n"
			"b_conn: %d\n"
			"b_se0_srp: %d\n"
			"b_ssend_srp: %d\n"
			"b_sess_vld: %d\n"
			"id: %d\n",
			fsm->a_bus_req,
			fsm->b_bus_req,
			fsm->a_bus_resume,
			fsm->a_bus_suspend,
			fsm->a_conn,
			fsm->a_sess_vld,
			fsm->a_srp_det,
			fsm->a_vbus_vld,
			fsm->b_bus_resume,
			fsm->b_bus_suspend,
			fsm->b_conn,
			fsm->b_se0_srp,
			fsm->b_ssend_srp,
			fsm->b_sess_vld,
			fsm->id);

	mutex_unlock(&fsm->lock);

	return 0;
}

static int fsm_open(struct inode *inode, struct file *file)
{
	return single_open(file, fsm_show, inode->i_private);
}

static const struct file_operations fsm_fops = {
	.open			= fsm_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};


static int vbus_evt_show(struct seq_file *s, void *unused)
{
	struct otg_fsm *fsm;
	struct cht_otg *otg_dev = s->private;

	if (!otg_dev)
		return -EINVAL;

	fsm = &otg_dev->fsm;

	if (otg_dev->fsm.b_sess_vld)
		seq_puts(s, "VBUS high\n");
	else
		seq_puts(s, "VBUS low\n");

	return 0;
}

static int vbus_evt_open(struct inode *inode, struct file *file)
{
	return single_open(file, vbus_evt_show, inode->i_private);
}

static ssize_t vbus_evt_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct cht_otg		*otg_dev = s->private;
	char			buf;

	if (count != 2)
		return -EINVAL;

	/* only need the first character */
	if (copy_from_user(&buf, ubuf, 1))
		return -EFAULT;

	switch (buf) {
	case '1':
		dev_info(otg_dev->phy.dev, "VBUS = 1\n");
		atomic_notifier_call_chain(&otg_dev->phy.notifier,
			USB_EVENT_VBUS, NULL);
		return count;
	case '0':
		dev_info(otg_dev->phy.dev, "VBUS = 0\n");
		atomic_notifier_call_chain(&otg_dev->phy.notifier,
			USB_EVENT_NONE, NULL);
		return count;
	default:
		return -EINVAL;
	}
	return count;
}

static const struct file_operations vbus_evt_fops = {
	.open			= vbus_evt_open,
	.write			= vbus_evt_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int otg_id_show(struct seq_file *s, void *unused)
{
	struct cht_otg		*otg_dev = s->private;

	if (otg_dev->fsm.id)
		seq_puts(s, "ID float\n");
	else
		seq_puts(s, "ID gnd\n");

	return 0;
}

static int otg_id_open(struct inode *inode, struct file *file)
{
	return single_open(file, otg_id_show, inode->i_private);
}

static ssize_t otg_id_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct cht_otg		*otg_dev = s->private;
	char			buf;

	if (count != 2)
		return -EINVAL;

	/* only need the first character */
	if (copy_from_user(&buf, ubuf, 1))
		return -EFAULT;

	switch (buf) {
	case '0':
	case 'a':
	case 'A':
		dev_info(otg_dev->phy.dev, "ID = 0\n");
		atomic_notifier_call_chain(&otg_dev->phy.notifier,
			USB_EVENT_ID, NULL);
		return count;
	case '1':
	case 'b':
	case 'B':
		dev_info(otg_dev->phy.dev, "ID = 1\n");
		atomic_notifier_call_chain(&otg_dev->phy.notifier,
			USB_EVENT_NONE, NULL);
		return count;
	default:
		return -EINVAL;
	}

	return count;
}

static const struct file_operations otg_id_fops = {
	.open			= otg_id_open,
	.write			= otg_id_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int cht_handle_extcon_otg_event(struct notifier_block *nb,
					unsigned long event, void *param)
{
	struct extcon_dev *edev = param;
	int id = edev_state_to_id(edev);
	struct usb_bus *host;
	struct usb_gadget *gadget;

	if (!cht_otg_dev)
		return NOTIFY_DONE;

	host = cht_otg_dev->phy.otg->host;
	gadget = cht_otg_dev->phy.otg->gadget;

	dev_info(cht_otg_dev->phy.dev, "[extcon notification]: USB-Host: %s\n",
			id ? "Disconnected" : "connected");
	
    /*hold a wakelock when insert otg*/
    if(id==0){
        if(!wake_lock_active(&otg_wakelock))
			wake_lock(&otg_wakelock);
    }
    else{
        if(wake_lock_active(&otg_wakelock))
			wake_unlock(&otg_wakelock);
    }

	/* update id value and schedule fsm work to start/stop host per id */
	cht_otg_dev->fsm.id = id;

	/* don't kick the state machine if host or device controller
	 * is not registered. Just wait to kick it when set_host or
	 * set_peripheral.*/
	if (host && gadget)
		schedule_work(&cht_otg_dev->fsm_work);

	return NOTIFY_OK;
}

static int cht_otg_probe(struct platform_device *pdev)
{
	struct cht_otg *cht_otg;
	struct em_config_oem1_data em_config;
	unsigned compliance_bit = 0;
	struct dentry *root;
	struct dentry *file;
	int status;

	cht_otg = kzalloc(sizeof(struct cht_otg), GFP_KERNEL);
	if (!cht_otg) {
		dev_err(&pdev->dev, "Failed to alloc cht_otg structure\n");
		return -ENOMEM;
	}

	cht_otg->phy.otg = kzalloc(sizeof(struct usb_otg), GFP_KERNEL);
	if (!cht_otg->phy.otg) {
		kfree(cht_otg);
		return -ENOMEM;
	}
    wake_lock_init(&otg_wakelock,0,"otg_wake_lock");

	/* Set OTG state machine operations */
	cht_otg->fsm.ops = &cht_otg_ops;

	/* initialize the otg and phy structure */
	cht_otg->phy.label = DRIVER_DESC;
	cht_otg->phy.dev = &pdev->dev;
	cht_otg->phy.set_power = cht_otg_set_power;

	cht_otg->phy.otg->phy = &cht_otg->phy;
	cht_otg->phy.otg->set_host = cht_otg_set_host;
	cht_otg->phy.otg->set_peripheral = cht_otg_set_peripheral;

	/* No support for ADP, HNP and SRP */
	cht_otg->phy.otg->start_hnp = NULL;
	cht_otg->phy.otg->start_srp = NULL;


	INIT_WORK(&cht_otg->fsm_work, cht_otg_fsm_work);
	INIT_DELAYED_WORK(&cht_otg->watchdog, cht_otg_device_watchdog);

	cht_otg_dev = cht_otg;

	status = usb_add_phy(&cht_otg_dev->phy, USB_PHY_TYPE_USB2);
	if (status) {
		dev_err(&pdev->dev, "failed to add cht otg usb phy\n");
		goto err1;
	}

	cht_otg_dev->nb.notifier_call = cht_otg_handle_notification;
	usb_register_notifier(&cht_otg_dev->phy, &cht_otg_dev->nb);

	/* Register on extcon for OTG event too */
	cht_otg_dev->id_nb.notifier_call = cht_handle_extcon_otg_event;
	status = extcon_register_interest(&cht_otg_dev->cable_nb, NULL,
					"USB-Host", &cht_otg_dev->id_nb);
	if (status)
		dev_warn(&pdev->dev, "failed to register extcon notifier\n");

#ifdef CONFIG_ACPI
	status = em_config_get_oem1_data(&em_config);
	if (!status)
		pr_warn("%s: failed to fetch OEM1 table %d\n",
			__func__, status);
	else
		/* 0 - usb compliance, 1 - no usb compliance */
		compliance_bit = em_config.fpo_0 & FPO0_USB_COMP_OFFSET;
	pr_info("%s : usb %s mode selected\n", __func__,
		compliance_bit ? "none_compliance" : "compliance");
	cht_otg->compliance = !compliance_bit;
#endif

	/* init otg-fsm */
	status = cht_otg_start(pdev);
	if (status) {
		dev_err(&pdev->dev, "failed to add cht otg usb phy\n");
		goto err2;
	}

	root = debugfs_create_dir(dev_name(&pdev->dev), NULL);
	if (!root) {
		status = -ENOMEM;
		goto err2;
	}

	cht_otg_dev->root = root;

	file = debugfs_create_file("fsm", S_IRUGO, root,
					cht_otg_dev, &fsm_fops);
	if (!file) {
		status = -ENOMEM;
		goto err3;
	}

	file = debugfs_create_file("otg_id", S_IRUGO | S_IWUSR | S_IWGRP, root,
					cht_otg_dev, &otg_id_fops);
	if (!file) {
		status = -ENOMEM;
		goto err3;
	}

	file = debugfs_create_file("vbus_evt", S_IRUGO | S_IWUSR | S_IWGRP,
					root, cht_otg_dev, &vbus_evt_fops);
	if (!file) {
		status = -ENOMEM;
		goto err3;
	}

	return 0;

err3:
	debugfs_remove_recursive(root);
err2:
	cht_otg_stop(pdev);
	extcon_unregister_interest(&cht_otg_dev->cable_nb);
	usb_remove_phy(&cht_otg_dev->phy);
err1:
	kfree(cht_otg->phy.otg);
	kfree(cht_otg);
	return status;
}

static int cht_otg_remove(struct platform_device *pdev)
{
	wake_lock_destroy(&otg_wakelock);
    debugfs_remove_recursive(cht_otg_dev->root);

	cht_otg_stop(pdev);
	extcon_unregister_interest(&cht_otg_dev->cable_nb);
	usb_remove_phy(&cht_otg_dev->phy);

	kfree(cht_otg_dev->phy.otg);
	kfree(cht_otg_dev);

	return 0;
}

struct platform_driver intel_cht_otg_driver = {
	.probe = cht_otg_probe,
	.remove = cht_otg_remove,
	.driver = {
		.name = driver_name,
		.owner = THIS_MODULE,
	},
};

static int __init cht_otg_phy_init(void)
{
	return platform_driver_register(&intel_cht_otg_driver);
}
subsys_initcall(cht_otg_phy_init);

static void __exit cht_otg_phy_exit(void)
{
	platform_driver_unregister(&intel_cht_otg_driver);
}
module_exit(cht_otg_phy_exit);

MODULE_DESCRIPTION(DRIVER_INFO);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
