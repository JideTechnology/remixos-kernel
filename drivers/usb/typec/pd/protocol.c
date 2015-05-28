/*
 * protocol.c: PD Protocol framework for handling PD messages
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
 * Author: Kannappan, R <r.kannappan@intel.com>
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/extcon.h>
#include <linux/usb_typec_phy.h>
#include <linux/errno.h>
#include "message.h"
#include "protocol.h"
#include "devpolicy_mgr.h"

static LIST_HEAD(protocol_list);
static DEFINE_SPINLOCK(protocol_lock);

static int pd_extcon_ufp_ntf(struct notifier_block *nb,
				unsigned long event, void *param)
{
	struct pd_prot *prot =
		container_of(nb, struct pd_prot, ufp_nb);
	struct extcon_dev *edev = param;
	bool cable_state;

	if (!edev)
		return NOTIFY_DONE;

	prot->init_data_role = PD_DATA_ROLE_UFP;
	prot->new_data_role = PD_DATA_ROLE_UFP;
	prot->init_pwr_role = PD_POWER_ROLE_CONSUMER;
	prot->new_pwr_role = PD_POWER_ROLE_CONSUMER;

	cable_state = extcon_get_cable_state(edev, CABLE_CONSUMER);
	if (cable_state)
		queue_work(system_nrt_wq, &prot->cable_event_work);

	return NOTIFY_OK;
}

static int pd_extcon_dfp_ntf(struct notifier_block *nb,
				unsigned long event, void *param)
{
	struct pd_prot *prot =
		container_of(nb, struct pd_prot, dfp_nb);
	struct extcon_dev *edev = param;
	bool cable_state;

	if (!edev)
		return NOTIFY_DONE;

	prot->init_data_role = PD_DATA_ROLE_DFP;
	prot->new_data_role = PD_DATA_ROLE_DFP;
	prot->init_pwr_role = PD_POWER_ROLE_PROVIDER;
	prot->new_pwr_role = PD_POWER_ROLE_PROVIDER;

	cable_state = extcon_get_cable_state(edev, CABLE_PROVIDER);
	if (cable_state)
		queue_work(system_nrt_wq, &prot->cable_event_work);

	return NOTIFY_OK;
}

static void pd_reset_counters(struct pd_prot *pd)
{
	mutex_lock(&pd->tx_lock);
	pd->event = PROT_EVENT_NONE;
	pd->tx_msg_id = 0;
	pd->retry_counter = 0;
	mutex_unlock(&pd->tx_lock);
}

static int do_tx_msg_flush(struct pd_prot *pd)
{
	pd_prot_flush_fifo(pd, FIFO_TYPE_TX);
	pd->retry_counter = 0;
	return 0;
}

static int pd_prot_handle_reset(struct pd_prot *pd, enum typec_phy_evts evt)
{
	pd_prot_flush_fifo(pd, FIFO_TYPE_TX);
	pd_prot_flush_fifo(pd, FIFO_TYPE_RX);
	pd->event = PROT_PHY_EVENT_RESET;
	complete(&pd->tx_complete);
	pd_reset_counters(pd);
	/*TODO: check if the the work is completed */
	pd->event = PROT_PHY_EVENT_NONE;

	return 0;
}

static int pd_tx_fsm_state(struct pd_prot *pd, int tx_state)
{
	switch (tx_state) {
	case PROT_TX_PHY_LAYER_RESET:
		do_tx_msg_flush(pd);
		/* pd_prot_reset_phy(pd); */
		break;
	default:
		return -EINVAL;
	}
	pd->cur_tx_state = tx_state;
	return 0;
}

static void pd_prot_tx_work(struct pd_prot *prot)
{
	int len;

	len = PD_MSG_LEN(&prot->tx_buf.header) + PD_MSG_HEADER_SIZE;
	pd_prot_send_phy_packet(prot, &prot->tx_buf, len);

}

static int pd_prot_rcv_pkt_from_policy(struct pd_prot *prot, u8 msg_type,
						void *buf, int len)
{
	struct pd_packet *pkt;

	if (msg_type == PD_CMD_HARD_RESET)
		return pd_prot_reset_phy(prot);
	else if (msg_type == PD_CMD_PROTOCOL_RESET)
		return pd_tx_fsm_state(prot, PROT_TX_PHY_LAYER_RESET);

	pkt = &prot->tx_buf;
	pkt->header.msg_type = msg_type;
	pkt->header.data_role = prot->new_data_role;
	if (prot->pd_version == 2)
		pkt->header.rev_id = 1;
	else
		pkt->header.rev_id = 0;
	pkt->header.pwr_role = prot->new_pwr_role;
	pkt->header.msg_id = prot->tx_msg_id;
	pkt->header.num_data_obj = len / 4;
	memcpy((u8 *)pkt + sizeof(struct pd_pkt_header), buf, len);

	pd_prot_tx_work(prot);

	return 0;
}

int prot_rx_send_goodcrc(struct pd_prot *pd, u8 msg_id)
{
	mutex_lock(&pd->tx_data_lock);
	pd_ctrl_msg(pd, PD_CTRL_MSG_GOODCRC, msg_id);
	pd_prot_send_phy_packet(pd, &pd->tx_buf, PD_MSG_HEADER_SIZE);
	mutex_unlock(&pd->tx_data_lock);
	return 0;
}

static void pd_tx_discard_msg(struct pd_prot *pd)
{
	mutex_lock(&pd->tx_lock);
	pd->event = PROT_EVENT_DISCARD;
	pd->tx_msg_id = (pd->tx_msg_id + 1) % PD_MAX_MSG_ID;
	mutex_unlock(&pd->tx_lock);
	complete(&pd->tx_complete);
	pd_tx_fsm_state(pd, PROT_TX_PHY_LAYER_RESET);
}

static void pd_prot_rx_work(struct pd_prot *pd)
{
	u8 msg_id;
	struct pd_packet *buf;
	u8 msg_type;

	/* wait for goodcrc sent */
	if (!pd->phy->support_auto_goodcrc)
		wait_for_completion(&pd->tx_complete);

	if (pd->event == PROT_PHY_EVENT_RESET)
		goto end;

	buf = &pd->cached_rx_buf;

	msg_id = PD_MSG_ID(&buf->header);
	msg_type = PD_MSG_TYPE(&buf->header);

	if (pd->rx_msg_id != msg_id) {
		pd_tx_discard_msg(pd);
		pd->rx_msg_id = msg_id;

	}
end:
	if (!pd->phy->support_auto_goodcrc)
		reinit_completion(&pd->tx_complete);
}

static inline void prot_rx_reset(struct pd_prot *pd)
{
	pd_reset_counters(pd);
	pd_prot_reset_phy(pd); /* flush both the fifo */
}

static void pd_prot_phy_rcv(struct pd_prot *pd)
{
	struct pd_packet rcv_buf;
	int len, event, send_good_crc, msg_type, msg_id;

	mutex_lock(&pd->rx_data_lock);

	len = pd_prot_recv_phy_packet(pd, &rcv_buf);
	if (len == 0)
		goto end;

	msg_type = PD_MSG_TYPE(&rcv_buf.header);
	msg_id = PD_MSG_ID(&rcv_buf.header);
	send_good_crc = 1;
	switch (msg_type) {
	case PD_CTRL_MSG_SOFT_RESET:
		prot_rx_reset(pd);
		break;
	case PD_CTRL_MSG_PING:
		break;
	case PD_CTRL_MSG_GOODCRC:
		if (!IS_CTRL_MSG(&rcv_buf.header))
			/* data message (source capability :)) */
			break;
		send_good_crc = 0;
		if (msg_id == pd->tx_msg_id) {
			pd->tx_msg_id = (pd->tx_msg_id + 1) % PD_MAX_MSG_ID;
			event = PROT_EVENT_NONE;
			/* notify policy */
		} else {
			event = PROT_EVENT_MSGID_MISMATCH;
		}
		mutex_lock(&pd->tx_lock);
		pd->event = event;
		mutex_unlock(&pd->tx_lock);
		complete(&pd->tx_complete);
		break;
	default:
		/*  process all other messages */
		dev_dbg(pd->phy->dev, "PROT: %s msg_type: %d\n",
				__func__, msg_type);
		break;
	}

	if (send_good_crc) {
		if (!pd->phy->support_auto_goodcrc) {
			reinit_completion(&pd->tx_complete);
			prot_rx_send_goodcrc(pd, msg_id);
		}
		memcpy(&pd->cached_rx_buf, &rcv_buf, len);
		pd_prot_rx_work(pd);
	}
end:
	mutex_unlock(&pd->rx_data_lock);
}

static int pd_handle_phy_ntf(struct notifier_block *nb,
			unsigned long event, void *data)
{
	return NOTIFY_OK;
}

static void pd_notify_protocol(struct typec_phy *phy, unsigned long event)
{
	struct pd_prot *pd = phy->proto;

	switch (event) {
	case PROT_PHY_EVENT_TX_SENT:
		mutex_lock(&pd->tx_lock);
		pd->event = PROT_EVENT_TX_COMPLETED;
		mutex_unlock(&pd->tx_lock);
		complete(&pd->tx_complete);
		break;
	case PROT_PHY_EVENT_COLLISION:
		mutex_lock(&pd->tx_lock);
		pd->event = PROT_EVENT_COLLISION;
		mutex_unlock(&pd->tx_lock);
		complete(&pd->tx_complete);
		break;
	case PROT_PHY_EVENT_MSG_RCV:
		pd_prot_phy_rcv(pd);
		break;
	case PROT_PHY_EVENT_GOODCRC_SENT:
		dev_dbg(phy->dev, "%s: PROT_PHY_EVENT_GOODCRC_SENT\n",
				__func__);
		break;
	case PROT_PHY_EVENT_HARD_RST: /* recv HRD_RST */
	case PROT_PHY_EVENT_SOFT_RST:
		pd_prot_handle_reset(pd, event);
		/* wait for activity complete */
		break;
	case PROT_PHY_EVENT_TX_FAIL:
		mutex_lock(&pd->tx_lock);
		pd->event = PROT_EVENT_TX_FAIL;
		mutex_unlock(&pd->tx_lock);
		complete(&pd->tx_complete);
		break;
	case PROT_PHY_EVENT_SOFT_RST_FAIL:
		break;
	case PROT_PHY_EVENT_TX_HARD_RST: /* sent HRD_RST */
		break;
	default:
		break;
	}
}

static void prot_cable_worker(struct work_struct *work)
{
	struct pd_prot *prot =
		container_of(work, struct pd_prot, cable_event_work);

	pd_prot_setup_role(prot, prot->new_data_role, prot->new_pwr_role);
}

int protocol_bind_dpm(struct typec_phy *phy)
{
	int ret;
	struct pd_prot *prot;

	prot = devm_kzalloc(phy->dev, sizeof(struct pd_prot), GFP_KERNEL);
	if (!prot)
		return -ENOMEM;

	prot->phy = phy;
	prot->pd_version = phy->get_pd_version(phy);

	if (prot->pd_version == 0) {
		kfree(prot);
		return -EINVAL;
	}

	phy->proto = prot;
	prot->phy->notify_protocol = pd_notify_protocol;
	prot->phy_nb.notifier_call = pd_handle_phy_ntf;

	ret = typec_register_prot_notifier(phy, &prot->phy_nb);
	if (ret < 0) {
		dev_err(phy->dev, "%s: unable to register notifier", __func__);
		kfree(prot);
		return -EIO;
	}

	INIT_LIST_HEAD(&prot->list);

	mutex_init(&prot->tx_lock);
	mutex_init(&prot->tx_data_lock);
	mutex_init(&prot->rx_data_lock);
	init_completion(&prot->tx_complete);

	prot->init_data_role = PD_DATA_ROLE_UFP;
	prot->new_data_role = PD_DATA_ROLE_UFP;
	prot->init_pwr_role = PD_POWER_ROLE_CONSUMER;
	prot->new_pwr_role = PD_POWER_ROLE_CONSUMER;

	prot->rx_msg_id = -1; /* no message is stored */
	pd_reset_counters(prot);
	pd_tx_fsm_state(prot, PROT_TX_PHY_LAYER_RESET);
	INIT_WORK(&prot->cable_event_work, prot_cable_worker);

	prot->ufp_nb.notifier_call = pd_extcon_ufp_ntf;
	extcon_register_interest(&prot->cable_ufp, "usb-typec",
					"USB_TYPEC_UFP", &prot->ufp_nb);
	prot->dfp_nb.notifier_call = pd_extcon_dfp_ntf;
	extcon_register_interest(&prot->cable_dfp, "usb-typec",
					"USB_TYPEC_DFP", &prot->dfp_nb);
	prot->policy_fwd_pkt = pd_prot_rcv_pkt_from_policy;
	list_add_tail(&prot->list, &protocol_list);

	return 0;
}
EXPORT_SYMBOL_GPL(protocol_bind_dpm);

static void remove_protocol(struct pd_prot *prot)
{
	if (!prot)
		return;

	extcon_unregister_interest(&prot->cable_dfp);
	extcon_unregister_interest(&prot->cable_ufp);
	typec_unregister_prot_notifier(prot->phy, &prot->phy_nb);
	kfree(prot);
}

void protocol_unbind_dpm(struct typec_phy *phy)
{
	struct pd_prot *prot, *temp;

	if (list_empty(&protocol_list))
		return;

	spin_lock(&protocol_lock);
	list_for_each_entry_safe(prot, temp, &protocol_list, list) {
		if (prot->phy == phy) {
			list_del(&prot->list);
			remove_protocol(prot);
			break;
		}
	}
	spin_unlock(&protocol_lock);
}
EXPORT_SYMBOL_GPL(protocol_unbind_dpm);

MODULE_AUTHOR("Kannappan, R <r.kannappan@intel.com>");
MODULE_DESCRIPTION("PD Protocol Layer");
MODULE_LICENSE("GPL v2");
