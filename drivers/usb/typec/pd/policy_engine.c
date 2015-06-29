/*
 * policy_engine.c: Intel USB Power Delivery Policy Engine Driver
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
#include <linux/usb_typec_phy.h>
#include <linux/random.h>
#include "policy_engine.h"

static LIST_HEAD(pe_list);
static DEFINE_SPINLOCK(pe_lock);

static void pe_dump_header(struct pd_pkt_header *header);
static void pe_dump_data_msg(struct pd_packet *pkt);

static inline
struct policy *pe_get_active_src_or_snk_policy(struct list_head *head)
{
	struct policy *p = NULL;

	list_for_each_entry(p, head, list) {
		if (p && ((p->type == POLICY_TYPE_SINK)
			|| (p->type == POLICY_TYPE_SOURCE))) {
			if (p->state == POLICY_STATE_ONLINE)
				return p;
		}
	}

	return NULL;
}

static inline
struct policy *pe_get_running_policy(struct list_head *head)
{
	struct policy *p = NULL;

	list_for_each_entry(p, head, list) {
		if (p && (p->state == POLICY_STATE_ONLINE)
			&& (p->status == POLICY_STATUS_RUNNING))
				return p;
	}

	return NULL;
}

static inline bool pe_is_policy_active(struct policy *p)
{
	return (p->state == POLICY_STATE_ONLINE) ? true : false;
}

static inline bool pe_is_policy_running(struct policy *p)
{
	return (p->status == POLICY_STATUS_RUNNING) ? true : false;
}

static struct policy *pe_get_policy(struct policy_engine *pe,
					enum policy_type type)
{
	struct policy *p = NULL;

	list_for_each_entry(p, &pe->policy_list, list) {
		if (p && (p->type == type))
			return p;
	}
	return NULL;
}

static int policy_engine_process_data_msg(struct policy_engine *pe,
				enum pe_event evt, struct pd_packet *pkt)
{
	struct policy *p = NULL;
	int ret = 0;

	switch (evt) {
	case PE_EVT_RCVD_SRC_CAP:
	case PE_EVT_RCVD_REQUEST:
	case PE_EVT_RCVD_BIST:
	case PE_EVT_RCVD_SNK_CAP:
		p = pe_get_active_src_or_snk_policy(&pe->policy_list);
		break;
	case PE_EVT_RCVD_VDM:
		p = pe_get_policy(pe, POLICY_TYPE_DISPLAY);
		if (!p) {
			pr_err("PE: No display pe to forward VDM msgs\n");
			break;
		}
		if (!pe_is_policy_active(p)) {
			pr_err("PE: DispPE not active to forward VDM msgs\n");
			p = NULL;
		}
		break;
	default:
		pr_warn("PE:%s: Invalid data msg, event=%d\n", __func__, evt);
		pe_dump_data_msg(pkt);
	}

	if (p && p->rcv_pkt)
		ret = p->rcv_pkt(p, pkt, evt);
	else
		ret = -ENODEV;
	return ret;
}

static int policy_engine_process_ctrl_msg(struct policy_engine *pe,
				enum pe_event evt, struct pd_packet *pkt)
{
	struct policy *p = NULL;
	int ret = 0;

	switch (evt) {
	case PE_EVT_RCVD_GOODCRC:
		p = pe_get_running_policy(&pe->policy_list);
		if (!p)
			pr_err("PE:No running policy to forward GCRC msgs\n");
		break;
	case PE_EVT_RCVD_GOTOMIN:
	case PE_EVT_RCVD_ACCEPT:
	case PE_EVT_RCVD_REJECT:
	case PE_EVT_RCVD_PING:
	case PE_EVT_RCVD_PS_RDY:
	case PE_EVT_RCVD_GET_SRC_CAP:
	case PE_EVT_RCVD_GET_SINK_CAP:
	case PE_EVT_RCVD_DR_SWAP:
	case PE_EVT_RCVD_PR_SWAP:
	case PE_EVT_RCVD_VCONN_SWAP:
	case PE_EVT_RCVD_WAIT:
		pr_debug("PE:%s: Ctrl msg received\n", __func__);
		p = pe_get_active_src_or_snk_policy(&pe->policy_list);
		if (!p)
			pr_err("PE:No active policy to forward Ctrl msgs\n");
		break;
	default:
		pr_warn("PE:%s:Not a valid ctrl msg to process, event=%d\n",
				__func__, evt);
		pe_dump_header(&pkt->header);
	}
	if (p && p->rcv_pkt)
		ret = p->rcv_pkt(p, pkt, evt);
	else
		ret = -ENODEV;
	return ret;
}

static void pe_dump_header(struct pd_pkt_header *header)
{
#ifdef DBG
	if (!header) {
		pr_err("PE: No Header information available...\n");
		return;
	}
	pr_info("========== POLICY ENGINE: HEADER INFO ==========\n");
	pr_info("PE: Message Type - 0x%x\n", header->msg_type);
	pr_info("PE: Reserved B4 - 0x%x\n", header->rsvd_a);
	pr_info("PE: Port Data Role - 0x%x\n", header->data_role);
	pr_info("PE: Specification Revision - 0x%x\n", header->rev_id);
	pr_info("PE: Port Power Role - 0x%x\n", header->pwr_role);
	pr_info("PE: Message ID - 0x%x\n", header->msg_id);
	pr_info("PE: Number of Data Objects - 0x%x\n", header->num_data_obj);
	pr_info("PE: Reserved B15 - 0x%x\n", header->rsvd_b);
	pr_info("=============================================");
#endif /* DBG */
}

static void pe_dump_data_msg(struct pd_packet *pkt)
{
#ifdef DBG
	int num_data_objs = PD_MSG_NUM_DATA_OBJS(&pkt->header);
	unsigned int data_buf[num_data_objs];
	int i;

	memset(data_buf, 0, num_data_objs);
	memcpy(data_buf, &pkt->data_obj, PD_MSG_LEN(&pkt->header));

	for (i = 0; i < num_data_objs; i++) {
		pr_info("PE: Data Message - data[%d]: 0x%08x\n",
					i+1, data_buf[i]);
	}
#endif /* DBG */
}

static int pe_fwdcmd_to_policy(struct policy_engine *pe, enum pe_event evt)
{
	struct policy *p;
	int ret = 0;

	p = pe_get_active_src_or_snk_policy(&pe->policy_list);
	if (!p) {
		pr_err("PE: No Active policy!\n");
		return -EINVAL;
	}

	if (p && p->rcv_cmd) {
		p->rcv_cmd(p, evt);
	} else {
		pr_err("PE: Unable to find send cmd\n");
		ret = -ENODEV;
	}

	return ret;
}

static int policy_engine_process_cmd(struct policy_engine *pe,
				enum pe_event evt)
{
	int ret = 0;

	pr_debug("PE: %s - cmd %d\n", __func__, evt);
	switch (evt) {
	case PE_EVT_RCVD_HARD_RESET:
	case PE_EVT_RCVD_HARD_RESET_COMPLETE:
		ret = pe_fwdcmd_to_policy(pe, evt);
		if (ret < 0)
			pr_err("PE: Error in handling cmd\n");
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void pe_dpm_worker(struct work_struct *work)
{
	struct policy_engine *pe =
		container_of(work, struct policy_engine, dpm_work);
	struct pe_dpm_evt *evt, *tmp;
	unsigned long flags;
	struct list_head new_list;

	if (list_empty(&pe->dpm_queue))
		return;

	spin_lock_irqsave(&pe->dpm_queue_lock, flags);
	list_replace_init(&pe->dpm_queue, &new_list);
	spin_unlock_irqrestore(&pe->dpm_queue_lock, flags);

	list_for_each_entry_safe(evt, tmp, &new_list, node) {
		mutex_lock(&pe->dpmwk_lock);
		if (evt != NULL) {
			/* FIXME: Send msg to protocol to advertise the the
			 * source cap if evt = DEVMGR_EVENT_ADVERTISE_SRC_CAP */
		}
		mutex_unlock(&pe->dpmwk_lock);
		kfree(evt);
	}
}

static int pe_dpm_notification(struct notifier_block *nb,
				unsigned long event, void *param)
{
	struct policy_engine *pe =
		container_of(nb, struct policy_engine, dpm_nb);
	struct pe_dpm_evt *evt;

	if (!param || event != DEVMGR_EVENT_DFP_CONNECTED)
		return NOTIFY_DONE;

	evt = kzalloc(sizeof(*evt), GFP_ATOMIC);
	if (!evt) {
		pr_err("PE: failed to allocate memory for dpm event\n");
		return NOTIFY_DONE;
	}

	memcpy(&evt->caps, param, sizeof(struct pe_dpm_evt));
	INIT_LIST_HEAD(&evt->node);
	list_add_tail(&evt->node, &pe->dpm_queue);
	queue_work(system_nrt_wq, &pe->dpm_work);

	return NOTIFY_OK;
}

static inline int pe_register_dpm_notifications(struct policy_engine *pe)
{
	int retval;

	INIT_LIST_HEAD(&pe->dpm_queue);
	INIT_WORK(&pe->dpm_work, pe_dpm_worker);
	mutex_init(&pe->dpmwk_lock);
	spin_lock_init(&pe->dpm_queue_lock);

	pe->dpm_nb.notifier_call = pe_dpm_notification;
	retval = devpolicy_mgr_reg_notifier(&pe->dpm_nb);
	if (retval < 0) {
		pr_err("PE: failed to register dpm notifier\n");
		return -EIO;
	}

	return 0;
}

static int pe_get_snkpwr_cap(struct policy_engine *pe,
					struct power_cap *cap)
{
	if (pe && pe->dpm)
		return devpolicy_get_snkpwr_cap(pe->dpm, cap);

	return -ENODEV;
}

static int pe_get_snkpwr_caps(struct policy_engine *pe,
					struct power_caps *caps)
{
	if (pe && pe->dpm)
		return devpolicy_get_snkpwr_caps(pe->dpm, caps);

	return -ENODEV;
}

static int pe_get_max_snkpwr_cap(struct policy_engine *pe,
					struct power_cap *cap)
{
	if (pe && pe->dpm)
		return devpolicy_get_max_snkpwr_cap(pe->dpm, cap);

	return -ENODEV;
}

static enum data_role pe_get_data_role(struct policy_engine *pe)
{
	if (pe && pe->dpm)
		return devpolicy_get_data_role(pe->dpm);

	return DATA_ROLE_NONE;
}

static enum pwr_role pe_get_power_role(struct policy_engine *pe)
{
	if (pe && pe->dpm)
		return devpolicy_get_power_role(pe->dpm);

	return POWER_ROLE_NONE;
}

static void  pe_set_data_role(struct policy_engine *pe, enum data_role role)
{
	if (pe && pe->dpm)
		devpolicy_set_data_role(pe->dpm, role);
}

static void pe_set_power_role(struct policy_engine *pe, enum pwr_role role)
{
	if (pe && pe->dpm)
		devpolicy_set_power_role(pe->dpm, role);
}

static int pe_set_charger_mode(struct policy_engine *pe, enum charger_mode mode)
{
	if (pe && pe->dpm)
		return devpolicy_set_charger_mode(pe->dpm, mode);

	return -ENODEV;
}

static int pe_update_charger_ilim(struct policy_engine *pe, int ilim)
{
	if (pe && pe->dpm)
		return devpolicy_update_current_limit(pe->dpm, ilim);

	return -ENODEV;
}

static int pe_get_min_snk_current(struct policy_engine *pe, int *ma)
{
	if (pe && pe->dpm)
		return devpolicy_get_min_snk_current(pe->dpm, ma);

	return -ENODEV;
}

static enum cable_state pe_get_vbus_state(struct policy_engine *pe)
{
	if (pe && pe->dpm)
		return devpolicy_get_consumer_state(pe->dpm);

	return -ENODEV;
}

static bool pe_get_pd_state(struct policy_engine *pe)
{
	return pe->is_pd_connected;
}

static int pe_set_pd_state(struct policy_engine *pe, bool state)
{
	mutex_lock(&pe->pe_lock);
	pe->is_pd_connected = state;
	mutex_unlock(&pe->pe_lock);
	return 0;
}

static int pe_send_packet(struct policy_engine *pe, void *data, int len,
				u8 msg_type, enum pe_event evt)
{
	int ret = 0;

	if (!pe_get_pd_state(pe)) {
		ret = -EINVAL;
		goto snd_pkt_err;
	}

	switch (evt) {
	case PE_EVT_SEND_GOTOMIN:
	case PE_EVT_SEND_ACCEPT:
	case PE_EVT_SEND_REJECT:
	case PE_EVT_SEND_PING:
	case PE_EVT_SEND_PS_RDY:
	case PE_EVT_SEND_GET_SRC_CAP:
	case PE_EVT_SEND_GET_SINK_CAP:
	case PE_EVT_SEND_DR_SWAP:
	case PE_EVT_SEND_PR_SWAP:
	case PE_EVT_SEND_VCONN_SWAP:
	case PE_EVT_SEND_WAIT:
	case PE_EVT_SEND_SRC_CAP:
	case PE_EVT_SEND_REQUEST:
	case PE_EVT_SEND_BIST:
	case PE_EVT_SEND_SNK_CAP:
	case PE_EVT_SEND_VDM:
	case PE_EVT_SEND_HARD_RESET:
	case PE_EVT_SEND_PROTOCOL_RESET:
	case PE_EVT_SEND_SOFT_RESET:
		break;
	default:
		ret = -EINVAL;
		goto snd_pkt_err;
	}

	/* Send the pd_packet to protocol directly to request
	 * sink power cap */
	if (pe && pe->prot && pe->prot->policy_fwd_pkt)
		pe->prot->policy_fwd_pkt(pe->prot, msg_type, data, len);

snd_pkt_err:
	return ret;
}

static struct policy *__pe_find_policy(struct list_head *list,
						enum policy_type type)
{
	struct policy  *p = NULL;

	list_for_each_entry(p, list, list) {
		if (p && p->type != type)
			continue;
		return p;
	}

	return ERR_PTR(-ENODEV);
}

static void pe_policy_status_changed(struct policy_engine *pe, int policy_type,
				int status)
{
	struct policy *p;

	if (!pe)
		return;
	/* Handle the source policy status change */
	if ((policy_type == POLICY_TYPE_SOURCE)
		&& ((status == POLICY_STATUS_SUCCESS)
		|| (status == POLICY_STATUS_FAIL))) {
		p = pe_get_policy(pe, POLICY_TYPE_DISPLAY);
		/* Start the display policy */
		if (!p) {
			pr_err("PE:%s:No Display policy found\n", __func__);
			return;
		}
		if (p->start) {
			pr_info("PE:%s:Stating disp policy\n", __func__);
			p->start(p);
		}
	}
}

static void pe_init_policy(struct work_struct *work)
{
	struct policy_engine *pe = container_of(work, struct policy_engine,
							policy_init_work);

	struct pd_policy *supported_policy = pe->supported_policies;
	struct policy *policy;
	int i;

	for (i = 0; i < supported_policy->num_policies; i++) {
		switch (supported_policy->policies[i]) {
		case POLICY_TYPE_SINK:
			policy = sink_port_policy_init(pe);
			if (IS_ERR_OR_NULL(policy)) {
				pr_err("%s: unable to init SINK_POLICY\n",
								__func__);
				continue;
			}
			list_add_tail(&policy->list, &pe->policy_list);
			break;
		case POLICY_TYPE_SOURCE:
			policy = src_pe_init(pe);
			if (IS_ERR_OR_NULL(policy)) {
				pr_err("%s: unable to init SOURCE_POLICY\n",
								__func__);
				continue;
			}
			list_add_tail(&policy->list, &pe->policy_list);
			pr_debug("%s:Successfuly init source pe\n", __func__);
			break;
		case POLICY_TYPE_DISPLAY:
			policy = disp_pe_init(pe);
			if (IS_ERR_OR_NULL(policy)) {
				pr_err("%s: unable to init DOSPLAY_POLICY\n",
								__func__);
				continue;
			}
			list_add_tail(&policy->list, &pe->policy_list);
			pr_debug("%s:Successfuly init display pe\n", __func__);
			break;
		default:
			/* invalid, dont add it to policy */
			pr_err("PE: Unknown policy type %d\n",
				supported_policy->policies[i]);
			break;
		}
	}

	return;
}

static void pe_policy_work(struct work_struct *work)
{
	struct policy_engine *pe = container_of(work, struct policy_engine,
						policy_work);
	struct policy *p;

	switch (pe->cbl_type) {
	case CABLE_TYPE_CONSUMER:
		/* Start sink policy */
		p = pe_get_policy(pe, POLICY_TYPE_SINK);
		if (!p) {
			pr_err("PE: No SINK policy to start on UFP connect\n");
			break;
		}
		if (p->state != POLICY_STATE_ONLINE)
			p->start(p);
		else
			pr_warn("PE: SINK policy is already active!!!\n");
		break;
	case CABLE_TYPE_PROVIDER:
		/* Start source policy.
		 * Display pe should be started after source pe complete.
		 */
		p = pe_get_policy(pe, POLICY_TYPE_SOURCE);
		if (!p) {
			pr_err("PE: No SOURCE policy to start on DFP connect\n");
			break;
		}
		if (p->state != POLICY_STATE_ONLINE)
			p->start(p);
		else
			pr_warn("PE: SOURCE policy is already active!!!\n");
		break;
	case CABLE_TYPE_UNKNOWN:
		/* Stop all active policies */
		list_for_each_entry(p, &pe->policy_list, list) {
			if (p && (p->state == POLICY_STATE_ONLINE))
				p->stop(p);
		}
		break;
	default:
		pr_err("%s: Unknown cable_type=%d\n",
			__func__, pe->cbl_type);
	}
}

static int sink_port_event(struct notifier_block *nb, unsigned long event,
				void *param)
{
	struct policy_engine *pe = container_of(nb, struct policy_engine,
						sink_nb);
	struct extcon_dev *edev = (struct extcon_dev *)param;
	int cable_state;

	cable_state = extcon_get_cable_state(edev, "USB_TYPEC_UFP");
	pr_info("%s:USB_TYPEC_UFP event with cable_state=%d\n",
			__func__, cable_state);
	pe->cbl_type = cable_state ? CABLE_TYPE_CONSUMER : CABLE_TYPE_UNKNOWN;
	schedule_work(&pe->policy_work);
	return 0;
}

static int source_port_event(struct notifier_block *nb, unsigned long event,
				void *param)
{
	struct policy_engine *pe = container_of(nb, struct policy_engine,
						source_nb);
	struct extcon_dev *edev = (struct extcon_dev *)param;
	int cable_state;

	cable_state = extcon_get_cable_state(edev, "USB_TYPEC_DFP");
	pr_info("%s:USB_TYPEC_DFP event with cable_state=%d\n",
			__func__, cable_state);
	pe->cbl_type = cable_state ? CABLE_TYPE_PROVIDER : CABLE_TYPE_UNKNOWN;
	schedule_work(&pe->policy_work);
	return  0;
}

static struct pe_operations ops = {
	.get_snkpwr_cap = pe_get_snkpwr_cap,
	.get_snkpwr_caps = pe_get_snkpwr_caps,
	.get_max_snkpwr_cap = pe_get_max_snkpwr_cap,
	.get_data_role = pe_get_data_role,
	.get_power_role = pe_get_power_role,
	.set_data_role = pe_set_data_role,
	.set_power_role = pe_set_power_role,
	.set_charger_mode = pe_set_charger_mode,
	.update_charger_ilim = pe_update_charger_ilim,
	.get_min_snk_current = pe_get_min_snk_current,
	.send_packet = pe_send_packet,
	.get_vbus_state = pe_get_vbus_state,
	.get_pd_state = pe_get_pd_state,
	.set_pd_state = pe_set_pd_state,
	.process_data_msg = policy_engine_process_data_msg,
	.process_ctrl_msg = policy_engine_process_ctrl_msg,
	.process_cmd = policy_engine_process_cmd,
	.policy_status_changed = pe_policy_status_changed,
};

int policy_engine_bind_dpm(struct devpolicy_mgr *dpm)
{
	int retval;
	struct policy_engine *pe;

	if (!dpm)
		return -EINVAL;

	pe = devm_kzalloc(dpm->phy->dev, sizeof(struct policy_engine),
				GFP_KERNEL);
	if (!pe)
		return -ENOMEM;

	pe->ops = &ops;
	pe->dpm = dpm;
	pe->supported_policies = dpm->policy;
	retval = pe_register_dpm_notifications(pe);
	if (retval) {
		pr_err("PE: failed to register dpm policy notifier\n");
		retval = -EINVAL;
		goto error0;
	}
	INIT_LIST_HEAD(&pe->list);

	retval = protocol_bind_pe(pe);
	if (retval) {
		pr_err("PE: failed to bind pe to protocol\n");
		retval = -EINVAL;
		goto error1;
	}

	pe->sink_nb.notifier_call = sink_port_event;
	pe->source_nb.notifier_call = source_port_event;

	retval = extcon_register_interest(&pe->sink_cable_nb,
						NULL, "USB_TYPEC_UFP",
						&pe->sink_nb);
	if (retval < 0)
		goto error2;

	retval = extcon_register_interest(&pe->source_cable_nb,
						NULL, "USB_TYPEC_DFP",
						&pe->source_nb);
	if (retval < 0)
		goto error3;

	INIT_WORK(&pe->policy_init_work, pe_init_policy);
	INIT_WORK(&pe->policy_work, pe_policy_work);
	mutex_init(&pe->pe_lock);
	INIT_LIST_HEAD(&pe->policy_list);
	list_add_tail(&pe->list, &pe_list);

	schedule_work(&pe->policy_init_work);

	return 0;

error3:
	extcon_unregister_interest(&pe->sink_cable_nb);
error2:
	protocol_unbind_pe(pe);
error1:
	devpolicy_mgr_unreg_notifier(&pe->dpm_nb);
error0:
	kfree(pe);
	return retval;
}
EXPORT_SYMBOL_GPL(policy_engine_bind_dpm);

static void remove_pe(struct policy_engine *pe)
{
	struct pd_policy *supported_policy = pe->supported_policies;
	struct policy *p;
	int i;

	if (!pe)
		return;

	for (i = 0; i < supported_policy->num_policies; i++) {
		p = __pe_find_policy(&pe->policy_list,
					supported_policy->policies[i]);
		p->exit(p);
	}
	extcon_unregister_interest(&pe->source_cable_nb);
	extcon_unregister_interest(&pe->sink_cable_nb);
	protocol_unbind_pe(pe);
	devpolicy_mgr_unreg_notifier(&pe->dpm_nb);
	kfree(pe);
}

void policy_engine_unbind_dpm(struct devpolicy_mgr *dpm)
{
	struct policy_engine *pe, *temp;

	if (list_empty(&pe_list))
		return;

	spin_lock(&pe_lock);
	list_for_each_entry_safe(pe, temp, &pe_list, list) {
		if (pe->dpm == dpm) {
			list_del(&pe->list);
			remove_pe(pe);
			break;
		}
	}
	spin_unlock(&pe_lock);
}
EXPORT_SYMBOL_GPL(policy_engine_unbind_dpm);

MODULE_AUTHOR("Albin B <albin.bala.krishnan@intel.com>");
MODULE_DESCRIPTION("PD Policy Engine Core");
MODULE_LICENSE("GPL v2");
