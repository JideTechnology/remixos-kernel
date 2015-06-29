/*
 * src_port_pe.c: Intel USB Power Delivery Source Port Policy Engine
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
 * Author: Venkataramana Kotakonda <venkataramana.kotakonda@intel.com>
 */

#include <linux/slab.h>
#include <linux/export.h>
#include "message.h"
#include "policy_engine.h"

#define LOG_TAG "src_pe"
#define log_info(format, ...) \
	pr_info(LOG_TAG":%s:"format"\n", __func__, ##__VA_ARGS__)
#define log_dbg(format, ...) \
	pr_debug(LOG_TAG":%s:"format"\n", __func__, ##__VA_ARGS__)
#define log_err(format, ...) \
	pr_err(LOG_TAG":%s:"format"\n", __func__, ##__VA_ARGS__)

#define MAX_CMD_RETRY	10
#define CMD_NORESPONCE_TIME	1 /* 4 Sec */

#define VOLT_TO_SRC_CAP_DATA_OBJ(x)	(x / 50)
#define CURRENT_TO_SRC_CAP_DATA_OBJ(x)	(x / 10)

struct src_port_pe {
	struct mutex pe_lock;
	struct policy p;
	int state;
	struct power_cap pcap;
	struct delayed_work start_comm;
	int cmd_retry;
};

/* Source policy engine states */
enum src_pe_state {
	SRC_PE_STATE_UNKNOWN = -1,
	SRC_PE_STATE_NONE,
	SRC_PE_STATE_SRCCAP_SENT,
	SRC_PE_STATE_SRCCAP_GCRC,
	SRC_PE_STATE_ACCEPT_SENT,
	SRC_PE_STATE_PS_RDY_SENT,
	SRC_PE_STATE_PD_CONFIGURED,
	SRC_PE_STATE_PD_FAILED,
};

static int src_pe_get_default_power_cap(struct src_port_pe *src_pe,
					struct power_cap *pcap)
{
	/* By default 5V/500mA is supported in provider mode */
	pcap->mv = VBUS_5V;
	pcap->ma = IBUS_0P5A;
	return 0;
}

static int src_pe_get_power_cap(struct src_port_pe *src_pe,
				struct power_cap *pcap)
{
	/* The power capabilities should be read from dev policy manager.
	 * Currently using default capabilities.
	 */
	return src_pe_get_default_power_cap(src_pe, pcap);
}

static void src_pe_reset_policy_engine(struct src_port_pe *src_pe)
{
	src_pe->state = SRC_PE_STATE_NONE;
	src_pe->pcap.mv = 0;
	src_pe->pcap.ma = 0;
}

static int src_pe_send_srccap_cmd(struct src_port_pe *src_pe)
{
	int ret;
	struct pd_fixed_supply_pdo pdo;
	struct power_cap pcap;

	log_dbg("Sending PD_CMD_HARD_RESET");
	policy_send_packet(&src_pe->p, NULL, 0, PD_CMD_HARD_RESET,
				PE_EVT_SEND_HARD_RESET);
	log_dbg("Sending SrcCap");
	ret = src_pe_get_power_cap(src_pe, &pcap);
	if (ret) {
		log_err("Error in getting power capabilities\n");
		return ret;
	}
	memset(&pdo, 0, sizeof(struct pd_fixed_supply_pdo));
	pdo.max_cur = CURRENT_TO_SRC_CAP_DATA_OBJ(pcap.ma); /* In 10mA units */
	pdo.volt = VOLT_TO_SRC_CAP_DATA_OBJ(pcap.mv); /* In 50mV units */
	pdo.peak_cur = 0; /* No peek current supported */
	pdo.dual_role_pwr = 1; /* Dual pwr role supported */

	ret = policy_send_packet(&src_pe->p, &pdo, 4,
				PD_DATA_MSG_SRC_CAP, PE_EVT_SEND_SRC_CAP);
	return ret;
}

static inline int src_pe_send_accept_cmd(struct src_port_pe *src_pe)
{

	return policy_send_packet(&src_pe->p, NULL, 0,
				PD_CTRL_MSG_ACCEPT, PE_EVT_SEND_ACCEPT);
}

static inline int src_pe_send_psrdy_cmd(struct src_port_pe *src_pe)
{

	return policy_send_packet(&src_pe->p, NULL, 0,
				PD_CTRL_MSG_PS_RDY, PE_EVT_SEND_PS_RDY);
}

static int
src_pe_handle_gcrc(struct src_port_pe *src_pe, struct pd_packet *pkt)
{
	int ret = 0;

	switch (src_pe->state) {
	case SRC_PE_STATE_SRCCAP_SENT:
		mutex_lock(&src_pe->pe_lock);
		src_pe->state = SRC_PE_STATE_SRCCAP_GCRC;
		mutex_unlock(&src_pe->pe_lock);
		log_dbg("SRC_PE_STATE_SRCCAP_SENT -> SRC_PE_STATE_SRCCAP_GCRC");
		break;
	case SRC_PE_STATE_ACCEPT_SENT:
		/* TODO: Enable the 5V  and send PS_DRY */
		ret = src_pe_send_psrdy_cmd(src_pe);
		mutex_lock(&src_pe->pe_lock);
		src_pe->state = SRC_PE_STATE_PS_RDY_SENT;
		mutex_unlock(&src_pe->pe_lock);
		log_dbg("SRC_PE_STATE_ACCEPT_SENT -> SRC_PE_STATE_PS_RDY_SENT");
		break;
	case SRC_PE_STATE_PS_RDY_SENT:
		mutex_lock(&src_pe->pe_lock);
		src_pe->state = SRC_PE_STATE_PD_CONFIGURED;
		src_pe->p.status = POLICY_STATUS_SUCCESS;
		mutex_unlock(&src_pe->pe_lock);
		log_info("SRC_PE_STATE_PS_RDY_SENT -> SRC_PE_STATE_PD_CONFIGURED");
		pe_notify_policy_status_changed(&src_pe->p,
				POLICY_TYPE_SOURCE, src_pe->p.status);
		break;
	default:
		ret = -EINVAL;
		log_info("GCRC received in wrong state=%d\n", src_pe->state);
	}

	return ret;
}

static int src_pe_handle_request_cmd(struct src_port_pe *src_pe)
{
	if ((src_pe->state == SRC_PE_STATE_SRCCAP_SENT)
		|| (src_pe->state == SRC_PE_STATE_SRCCAP_GCRC)) {
		/* Send accept for request */
		src_pe_send_accept_cmd(src_pe);
		mutex_lock(&src_pe->pe_lock);
		src_pe->state = SRC_PE_STATE_ACCEPT_SENT;
		mutex_unlock(&src_pe->pe_lock);
		log_dbg(" STATE -> SRC_PE_STATE_ACCEPT_SENT\n");
		return 0;
	}
	log_err(" REQUEST MSG received in wrong state!!!\n");
	return -EINVAL;
}

static int
src_pe_rcv_pkt(struct policy *srcp, struct pd_packet *pkt, enum pe_event evt)
{
	struct src_port_pe *src_pe = container_of(srcp,
					struct src_port_pe, p);
	int ret = 0;

	switch (evt) {
	case PE_EVT_RCVD_GOODCRC:
		ret = src_pe_handle_gcrc(src_pe, pkt);
		break;
	case PE_EVT_RCVD_REQUEST:
		ret = src_pe_handle_request_cmd(src_pe);
		break;
	default:
		ret = -EINVAL;
		log_info("Not proccessing the event=%d\n", evt);
	}
	return ret;
}

int src_pe_rcv_cmd(struct policy *srcp, enum pe_event evt)
{
	struct src_port_pe *src_pe = container_of(srcp,
					struct src_port_pe, p);
	int ret = 0;

	switch (evt) {
	case PE_EVT_RCVD_HARD_RESET:
	case PE_EVT_RCVD_HARD_RESET_COMPLETE:
		src_pe_reset_policy_engine(src_pe);
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void src_pe_start_comm(struct work_struct *work)
{
	struct src_port_pe *src_pe = container_of(work,
					struct src_port_pe,
					start_comm.work);

	if ((src_pe->state == SRC_PE_STATE_PD_FAILED)
		|| (src_pe->state == SRC_PE_STATE_PD_CONFIGURED)
		|| (src_pe->p.state == POLICY_STATE_OFFLINE)) {
		log_info("Not required to send srccap in this state=%d\n",
				src_pe->state);
		return;
	}

	src_pe_send_srccap_cmd(src_pe);
	mutex_lock(&src_pe->pe_lock);
	src_pe->state = SRC_PE_STATE_SRCCAP_SENT;
	mutex_unlock(&src_pe->pe_lock);

	src_pe->cmd_retry++;
	if (src_pe->cmd_retry < MAX_CMD_RETRY) {
		log_dbg("Re-scheduling the start_comm after %dSec\n",
				CMD_NORESPONCE_TIME);
		schedule_delayed_work(&src_pe->start_comm,
					HZ * CMD_NORESPONCE_TIME);
	} else {
		mutex_lock(&src_pe->pe_lock);
		src_pe->state = SRC_PE_STATE_PD_FAILED;
		src_pe->p.status = POLICY_STATUS_FAIL;
		mutex_unlock(&src_pe->pe_lock);
		log_dbg("Not sending srccap as max re-try reached\n");
		pe_notify_policy_status_changed(&src_pe->p,
				POLICY_TYPE_SOURCE, src_pe->p.status);
	}
}

static int src_pe_start_policy_engine(struct policy *p)
{
	struct src_port_pe *src_pe = container_of(p,
					struct src_port_pe, p);

	log_info("IN");
	mutex_lock(&src_pe->pe_lock);
	p->state = POLICY_STATE_ONLINE;
	p->status = POLICY_STATUS_RUNNING;
	policy_set_pd_state(p, true);
	schedule_delayed_work(&src_pe->start_comm, 0);
	mutex_unlock(&src_pe->pe_lock);
	return 0;
}

static int src_pe_stop_policy_engine(struct policy *p)
{
	struct src_port_pe *src_pe = container_of(p,
					struct src_port_pe, p);

	log_info("IN");
	mutex_lock(&src_pe->pe_lock);
	p->state = POLICY_STATE_OFFLINE;
	p->status = POLICY_STATUS_UNKNOWN;
	src_pe_reset_policy_engine(src_pe);
	cancel_delayed_work(&src_pe->start_comm);
	policy_set_pd_state(p, false);
	src_pe->cmd_retry = 0;
	mutex_unlock(&src_pe->pe_lock);
	return 0;
}

static void src_pe_exit(struct policy *p)
{
	struct src_port_pe *src_pe = container_of(p,
					struct src_port_pe, p);

	kfree(src_pe);
}

/* Init function to initialize the source policy engine */
struct policy *src_pe_init(struct policy_engine *pe)
{
	struct src_port_pe *src_pe;
	struct policy *p;

	src_pe = kzalloc(sizeof(struct src_port_pe),
						GFP_KERNEL);
	if (!src_pe) {
		log_err("mem alloc failed\n");
		return ERR_PTR(-ENOMEM);
	}

	mutex_init(&src_pe->pe_lock);
	INIT_DELAYED_WORK(&src_pe->start_comm, src_pe_start_comm);

	p = &src_pe->p;
	p->type = POLICY_TYPE_SOURCE;
	p->state = POLICY_STATE_OFFLINE;
	p->status = POLICY_STATUS_UNKNOWN;

	p->pe = pe;
	p->rcv_pkt = src_pe_rcv_pkt;
	p->rcv_cmd = src_pe_rcv_cmd;
	p->start = src_pe_start_policy_engine;
	p->stop = src_pe_stop_policy_engine;
	p->exit = src_pe_exit;

	log_info("Source pe initialized successfuly");

	return p;
}
EXPORT_SYMBOL(src_pe_init);
