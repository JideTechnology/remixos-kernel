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

#define LOG_TAG "disp_pe"
#define log_info(format, ...) \
	pr_info(LOG_TAG":%s:"format, __func__, ##__VA_ARGS__)
#define log_dbg(format, ...) \
	pr_debug(LOG_TAG":%s:"format, __func__, ##__VA_ARGS__)
#define log_err(format, ...) \
	pr_err(LOG_TAG":%s:"format, __func__, ##__VA_ARGS__)
#define log_warn(format, ...) \
	pr_warn(LOG_TAG":%s:"format, __func__, ##__VA_ARGS__)

#define MAX_CMD_RETRY	10
#define CMD_NORESPONCE_TIME	1 /* 1 Sec */

#define PD_SID          0xff00
#define VESA_SVID       0xff01
#define STRUCTURED_VDM	1

/* Display port pin assignments */
#define DISP_PORT_PIN_ASSIGN_A	(1 << 0)
#define DISP_PORT_PIN_ASSIGN_B	(1 << 1)
#define DISP_PORT_PIN_ASSIGN_C	(1 << 2)
#define DISP_PORT_PIN_ASSIGN_D	(1 << 3)
#define DISP_PORT_PIN_ASSIGN_E	(1 << 4)
#define DISP_PORT_PIN_ASSIGN_F	(1 << 5)

/* Display configuration */
#define DISP_CONFIG_USB		0
#define DISP_CONFIG_UFPU_AS_DFP_D	1
#define DISP_CONFIG_UFPU_AS_UFP_D	2
#define DISP_CONFIG_RESERVED		3

/* Display port signaling for transport */
#define DISP_PORT_SIGNAL_UNSPEC		0
#define DISP_PORT_SIGNAL_DP_1P3		1
#define DISP_PORT_SIGNAL_GEN2		2


struct disp_port_pe {
	struct mutex pe_lock;
	struct policy p;
	int state;
	struct delayed_work start_comm;
	int cmd_retry;
	int dp_mode;
	bool hpd_state;
};

/* Source policy engine states */
enum disp_pe_state {
	DISP_PE_STATE_UNKNOWN = -1,
	DISP_PE_STATE_NONE,
	DISP_PE_STATE_DI_SENT,
	DISP_PE_STATE_DI_GCRC,
	DISP_PE_STATE_SVID_SENT,
	DISP_PE_STATE_SVID_GCRC,
	DISP_PE_STATE_DMODE_SENT,
	DISP_PE_STATE_DMODE_GCRC,
	DISP_PE_STATE_EMODE_SENT,
	DISP_PE_STATE_EMODE_GCRC,
	DISP_PE_STATE_EMODE_SUCCESS,
	DISP_PE_STATE_DISPLAY_CONFIGURED,
	DISP_PE_STATE_ALT_MODE_FAIL,
};

static void disp_pe_reset_policy_engine(struct disp_port_pe *disp_pe)
{
	disp_pe->state = DISP_PE_STATE_NONE;
}

static void disp_pe_do_protocol_reset(struct disp_port_pe *disp_pe)
{
	policy_send_packet(&disp_pe->p, NULL, 0, PD_CMD_PROTOCOL_RESET,
				PE_EVT_SEND_PROTOCOL_RESET);
}

static int
disp_pe_handle_gcrc(struct disp_port_pe *disp_pe, struct pd_packet *pkt)
{
	int ret = 0;

	mutex_lock(&disp_pe->pe_lock);
	switch (disp_pe->state) {
	case DISP_PE_STATE_DI_SENT:
		disp_pe->state = DISP_PE_STATE_DI_GCRC;
		log_dbg("DISP_PE_STATE_DI_SENT -> DISP_PE_STATE_DI_GCRC\n");
		break;
	case DISP_PE_STATE_SVID_SENT:
		disp_pe->state = DISP_PE_STATE_SVID_GCRC;
		log_dbg("DISP_PE_STATE_SVID_SENT -> DISP_PE_STATE_SVID_GCRC\n");
		break;
	case DISP_PE_STATE_DMODE_SENT:
		disp_pe->state = DISP_PE_STATE_DMODE_GCRC;
		log_dbg("DISP_PE_STATE_DMODE_SENT -> DISP_PE_STATE_DMODE_GCRC\n");
		break;
	case DISP_PE_STATE_EMODE_SENT:
		disp_pe->state = DISP_PE_STATE_EMODE_GCRC;
		log_dbg("DISP_PE_STATE_EMODE_SENT -> DISP_PE_STATE_EMODE_GCRC\n");
		break;
	case DISP_PE_STATE_EMODE_SUCCESS:
		log_dbg("State ->DISP_PE_STATE_EMODE_SUCCESS\n");
		break;
	default:
		ret = -EINVAL;
		log_dbg("GCRC received in wrong state=%d\n", disp_pe->state);
	}
	mutex_unlock(&disp_pe->pe_lock);
	return ret;
}


static void disp_pe_prepare_vdm_header(struct pd_packet *pkt, enum vdm_cmd cmd,
					int svid, int obj_pos)
{
	struct vdm_header *v_hdr = (struct vdm_header *) &pkt->data_obj[0];

	memset(pkt, 0, sizeof(struct pd_packet));
	v_hdr->cmd = cmd;
	v_hdr->cmd_type = INITIATOR;
	v_hdr->obj_pos = obj_pos;
	v_hdr->str_vdm_version = 0x0; /* 0 = version 1.0 */
	v_hdr->vdm_type = STRUCTURED_VDM; /* Structured VDM */
	v_hdr->svid = svid;

}

static int disp_pe_send_discover_identity(struct disp_port_pe *disp_pe)
{
	struct pd_packet pkt;
	int ret;

	disp_pe_prepare_vdm_header(&pkt, DISCOVER_IDENTITY,
						PD_SID, 0);
	ret = policy_send_packet(&disp_pe->p, &pkt.data_obj[0], 4,
				PD_DATA_MSG_VENDOR_DEF, PE_EVT_SEND_VDM);

	return ret;
}

static int disp_pe_send_discover_svid(struct disp_port_pe *disp_pe)
{
	struct pd_packet pkt;
	int ret;

	disp_pe_prepare_vdm_header(&pkt, DISCOVER_SVID,
						PD_SID, 0);
	ret = policy_send_packet(&disp_pe->p, &pkt.data_obj[0], 4,
				PD_DATA_MSG_VENDOR_DEF, PE_EVT_SEND_VDM);

	return ret;
}

static int disp_pe_send_discover_mode(struct disp_port_pe *disp_pe, int svid)
{
	struct pd_packet pkt;
	int ret;

	disp_pe_prepare_vdm_header(&pkt, DISCOVER_MODE,
						svid, 0);
	ret = policy_send_packet(&disp_pe->p, &pkt.data_obj[0], 4,
				PD_DATA_MSG_VENDOR_DEF, PE_EVT_SEND_VDM);

	return ret;
}

static int disp_pe_send_enter_mode(struct disp_port_pe *disp_pe, int index)
{
	struct pd_packet pkt;
	int ret;

	disp_pe_prepare_vdm_header(&pkt, ENTER_MODE,
						VESA_SVID, index);
	ret = policy_send_packet(&disp_pe->p, &pkt.data_obj[0], 4,
				PD_DATA_MSG_VENDOR_DEF, PE_EVT_SEND_VDM);

	return ret;
}

static int disp_pe_send_display_configure(struct disp_port_pe *disp_pe)
{
	struct pd_packet pkt;
	struct disp_config *dconf;
	int ret;

	disp_pe_prepare_vdm_header(&pkt, DP_CONFIGURE,
						VESA_SVID, 0);
	dconf = (struct disp_config *)&pkt.data_obj[1];
	dconf->conf_sel = DISP_CONFIG_UFPU_AS_UFP_D;
	dconf->trans_sig = DISP_PORT_SIGNAL_DP_1P3;
	dconf->ufp_pin = DISP_PORT_PIN_ASSIGN_E;

	ret = policy_send_packet(&disp_pe->p, &pkt.data_obj[0], 8,
				PD_DATA_MSG_VENDOR_DEF, PE_EVT_SEND_VDM);

	return ret;
}

static int disp_pe_handle_discover_identity(struct disp_port_pe *disp_pe,
							struct pd_packet *pkt)
{
	struct vdm_header *vdm_hdr = (struct vdm_header *)&pkt->data_obj[0];

	switch (vdm_hdr->cmd_type) {
	case INITIATOR:
		log_warn("UFP alternate mode not supported\n");
		break;
	case REP_ACK:
		if ((disp_pe->state != DISP_PE_STATE_DI_SENT)
			&& (disp_pe->state != DISP_PE_STATE_DI_GCRC)) {
			log_warn("DI RACK received in wrong state,state=%d\n",
					disp_pe->state);
			break;
		}
		disp_pe_send_discover_svid(disp_pe);
		mutex_lock(&disp_pe->pe_lock);
		disp_pe->state = DISP_PE_STATE_SVID_SENT;
		mutex_unlock(&disp_pe->pe_lock);
		log_dbg(" State -> DISP_PE_STATE_SVID_SENT\n");
		break;
	case REP_NACK:
		mutex_lock(&disp_pe->pe_lock);
		cancel_delayed_work(&disp_pe->start_comm);
		disp_pe->state = DISP_PE_STATE_ALT_MODE_FAIL;
		disp_pe->p.status = POLICY_STATUS_FAIL;
		mutex_unlock(&disp_pe->pe_lock);
		log_err("Responder doesn't support alternate mode\n");
		break;
	case REP_BUSY:
		break;
	}
	return 0;
}


static int disp_pe_handle_discover_svid(struct disp_port_pe *disp_pe,
			struct pd_packet *pkt)
{
	struct dis_svid_response_pkt *svid_pkt;
	int num_modes = 0;
	int i, mode = 0;

	svid_pkt = (struct dis_svid_response_pkt *)pkt;

	switch (svid_pkt->vdm_hdr.cmd_type) {
	case INITIATOR:
		log_warn("UFP alternate mode not supported\n");
		break;
	case REP_ACK:
		if ((disp_pe->state != DISP_PE_STATE_SVID_SENT)
			&& (disp_pe->state != DISP_PE_STATE_SVID_GCRC)) {
			log_warn("SVID RACK received in wrong state=%d\n",
					disp_pe->state);
			break;
		}
		/* 2 modes per VDO*/
		num_modes = (svid_pkt->msg_hdr.num_data_obj - 1) * 2;
		log_dbg("SVID_ACK-> This Display supports %d modes\n",
				num_modes);
		for (i = 0; i < num_modes; i++) {
			log_dbg("vdo[%d].svid0=0x%x, svid1=0x%x\n",
				i, svid_pkt->vdo[i].svid0,
				svid_pkt->vdo[i].svid1);
			if ((svid_pkt->vdo[i].svid0 == VESA_SVID)
				|| (svid_pkt->vdo[i].svid1 == VESA_SVID)) {
				mode = VESA_SVID;
				break;
			}
		}
		/* Currently we support only VESA */
		if (mode == VESA_SVID) {
			log_dbg("This Display supports VESA\n");
			disp_pe_send_discover_mode(disp_pe, VESA_SVID);
			mutex_lock(&disp_pe->pe_lock);
			disp_pe->state = DISP_PE_STATE_DMODE_SENT;
			mutex_unlock(&disp_pe->pe_lock);
			log_dbg("State-> DISP_PE_STATE_DMODE_SENT\n");
			break;
		} else
			log_err("This Display doesn't supports VESA\n");
		/* Stop the display detection process */
	case REP_NACK:
		mutex_lock(&disp_pe->pe_lock);
		cancel_delayed_work(&disp_pe->start_comm);
		disp_pe->state = DISP_PE_STATE_ALT_MODE_FAIL;
		disp_pe->p.status = POLICY_STATUS_FAIL;
		mutex_unlock(&disp_pe->pe_lock);
		log_warn("Responder doesn't support alternate mode\n");
		break;
	case REP_BUSY:
		log_info("Responder BUSY!!. Retry Discover SVID\n");
		/*TODO: Retry the Discover SVID */
		break;
	}
	return 0;
}



static int disp_pe_handle_discover_mode(struct disp_port_pe *disp_pe,
			struct pd_packet *pkt)
{
	struct dis_mode_response_pkt *dmode_pkt;
	int i;
	int e_index = 0;
	int d_index = 0;

	dmode_pkt = (struct dis_mode_response_pkt *)pkt;

	switch (dmode_pkt->vdm_hdr.cmd_type) {
	case INITIATOR:
		log_warn("UFP alternate mode not supported\n");
		break;
	case REP_ACK:
		if ((disp_pe->state != DISP_PE_STATE_DMODE_SENT)
			&& (disp_pe->state != DISP_PE_STATE_DMODE_GCRC)) {
			log_warn("DiscMode RACK received in wrong state=%d\n",
					disp_pe->state);
			break;
		}

		for (i = 0; i < dmode_pkt->msg_hdr.num_data_obj - 1; i++) {
			if (dmode_pkt->mode[i].dfp_pin
				& DISP_PORT_PIN_ASSIGN_E) {
				/* Mode intex starts from 1 */
				e_index = i + 1;
				break;
			}
		}
		for (i = 0; i < dmode_pkt->msg_hdr.num_data_obj - 1; i++) {
			if (dmode_pkt->mode[i].dfp_pin
				& DISP_PORT_PIN_ASSIGN_D) {
				d_index = i + 1;
				break;
			}
		}
		/* First check for 2X, Mode E */
		log_dbg("e_index=%d, d_index=%d\n", e_index, d_index);
		if (e_index) {
			disp_pe_send_enter_mode(disp_pe, e_index);
			mutex_lock(&disp_pe->pe_lock);
			disp_pe->state = DISP_PE_STATE_EMODE_SENT;
			disp_pe->dp_mode = TYPEC_DP_TYPE_4X;
			mutex_unlock(&disp_pe->pe_lock);
			log_dbg("State -> DISP_PE_STATE_EMODE_SENT\n");
			break;
		} else if (d_index) {
			disp_pe_send_enter_mode(disp_pe, d_index);
			mutex_lock(&disp_pe->pe_lock);
			disp_pe->state = DISP_PE_STATE_EMODE_SENT;
			disp_pe->dp_mode = TYPEC_DP_TYPE_2X;
			mutex_unlock(&disp_pe->pe_lock);
			log_dbg("State -> DISP_PE_STATE_EMODE_SENT\n");
			break;
		}
		log_warn("This Display doesn't supports neither 2X nor 4X\n");
		/* Stop the display detection process */

	case REP_NACK:
		mutex_lock(&disp_pe->pe_lock);
		cancel_delayed_work(&disp_pe->start_comm);
		disp_pe->state = DISP_PE_STATE_ALT_MODE_FAIL;
		disp_pe->p.status = POLICY_STATUS_FAIL;
		mutex_unlock(&disp_pe->pe_lock);
		log_warn("Responder doesn't support alternate mode\n");
		break;
	case REP_BUSY:
		log_warn("Responder BUSY!!. Retry Discover SVID\n");
		/*TODO: Retry the Discover SVID */
		break;
	}
	return 0;
}


static int disp_pe_handle_enter_mode(struct disp_port_pe *disp_pe,
			struct pd_packet *pkt)
{
	struct vdm_header *vdm_hdr = (struct vdm_header *)&pkt->data_obj[0];

	switch (vdm_hdr->cmd_type) {
	case INITIATOR:
		log_warn("UFP alternate mode not supported\n");
		break;
	case REP_ACK:
		if ((disp_pe->state != DISP_PE_STATE_EMODE_SENT)
			&& (disp_pe->state != DISP_PE_STATE_EMODE_GCRC)) {
			log_warn("EnterMode ACK received in wrong state=%d\n",
					disp_pe->state);
			break;
		}
		mutex_lock(&disp_pe->pe_lock);
		disp_pe->state = DISP_PE_STATE_EMODE_SUCCESS;
		mutex_unlock(&disp_pe->pe_lock);
		log_dbg("State -> DISP_PE_STATE_EMODE_SUCCESS, dp_mode=%d\n",
				disp_pe->dp_mode);
		disp_pe_send_display_configure(disp_pe);
		break;
	case REP_NACK:
		log_warn("Display falied to enter dp mode %d\n",
			disp_pe->dp_mode);
		cancel_delayed_work(&disp_pe->start_comm);
		mutex_lock(&disp_pe->pe_lock);
		disp_pe->state = DISP_PE_STATE_ALT_MODE_FAIL;
		disp_pe->p.status = POLICY_STATUS_FAIL;
		mutex_unlock(&disp_pe->pe_lock);
	}
	return 0;
}

static int disp_pe_handle_display_configure(struct disp_port_pe *disp_pe,
			struct pd_packet *pkt)
{
	struct vdm_header *vdm_hdr = (struct vdm_header *)&pkt->data_obj[0];

	switch (vdm_hdr->cmd_type) {
	case INITIATOR:
		log_warn("UFP alternate mode not supported\n");
		break;
	case REP_ACK:
		if (disp_pe->state != DISP_PE_STATE_EMODE_SUCCESS) {
			log_warn("Config ACK received in wrong state=%d\n",
					disp_pe->state);
			break;
		}
		mutex_lock(&disp_pe->pe_lock);
		disp_pe->state = DISP_PE_STATE_DISPLAY_CONFIGURED;
		disp_pe->p.status = POLICY_STATUS_SUCCESS;
		mutex_unlock(&disp_pe->pe_lock);
		log_info("DISP_PE_STATE_DISPLAY_CONFIGURED,dp_mode=%d\n",
				disp_pe->dp_mode);
		disp_pe->hpd_state = true;
		policy_set_dp_state(&disp_pe->p, CABLE_ATTACHED,
					disp_pe->dp_mode);
		break;
	case REP_NACK:
		log_warn("NAK for display config cmd %d\n", disp_pe->dp_mode);
		mutex_lock(&disp_pe->pe_lock);
		cancel_delayed_work(&disp_pe->start_comm);
		disp_pe->state = DISP_PE_STATE_ALT_MODE_FAIL;
		disp_pe->p.status = POLICY_STATUS_FAIL;
		mutex_unlock(&disp_pe->pe_lock);
	}
	return 0;
}

static int disp_pe_handle_display_attention(struct disp_port_pe *disp_pe,
		struct pd_packet *pkt)
{
	struct dis_port_status *dstat;
	bool hpd;

	dstat = (struct dis_port_status *) &pkt->data_obj[1];
	log_info("hpd_status=%d\n", dstat->hpd_state);
	hpd = dstat->hpd_state;

	/* Some dp cable which doesnt suport status update cmd
	 * which is expected after EnterMode. Due to this the hpd
	 * status cannot be known after EnterMode. To fix this by
	 * default hpd will be triggered after EnterMode.
	 * So, if previous hpd is true then send diconnect and connect.
	 */
	if (hpd && disp_pe->hpd_state) {
		policy_set_dp_state(&disp_pe->p, CABLE_DETACHED,
					TYPEC_DP_TYPE_NONE);
	}
	disp_pe->hpd_state = hpd;
	if (hpd)
		policy_set_dp_state(&disp_pe->p, CABLE_ATTACHED,
					disp_pe->dp_mode);
	else
		policy_set_dp_state(&disp_pe->p, CABLE_DETACHED,
					TYPEC_DP_TYPE_NONE);

	return 0;
}

static int disp_pe_handle_vendor_msg(struct disp_port_pe *disp_pe,
		struct pd_packet *pkt)
{
	struct vdm_header *vdm_hdr = (struct vdm_header *)&pkt->data_obj[0];
	int ret = 0;

	switch (vdm_hdr->cmd) {
	case DISCOVER_IDENTITY:
		ret = disp_pe_handle_discover_identity(disp_pe, pkt);
		break;
	case DISCOVER_SVID:
		ret = disp_pe_handle_discover_svid(disp_pe, pkt);
		break;
	case DISCOVER_MODE:
		ret = disp_pe_handle_discover_mode(disp_pe, pkt);
		break;
	case ENTER_MODE:
		ret = disp_pe_handle_enter_mode(disp_pe, pkt);
		break;
	case EXIT_MODE:
		log_dbg("EXIT DP mode request received\n");
		/* TODO: Handle the exit mode */
		break;
	case DP_CONFIGURE:
		ret = disp_pe_handle_display_configure(disp_pe, pkt);
		break;
	/* DP_STATUS_UPDATE and ATTENTION has same status vdo*/
	case DP_STATUS_UPDATE:
	case ATTENTION:
		ret = disp_pe_handle_display_attention(disp_pe, pkt);
		break;
	default:
		ret = -EINVAL;
		log_err("Not a valid vendor msg to handle\n");
	}
	return ret;
}


static int
disp_pe_rcv_pkt(struct policy *p, struct pd_packet *pkt, enum pe_event evt)
{
	struct disp_port_pe *disp_pe = container_of(p,
					struct disp_port_pe, p);
	int ret = 0;

	switch (evt) {
	case PE_EVT_RCVD_GOODCRC:
		disp_pe_handle_gcrc(disp_pe, pkt);
		break;
	case PE_EVT_RCVD_VDM:
		disp_pe_handle_vendor_msg(disp_pe, pkt);
		break;
	default:
		ret = -EINVAL;
		log_warn("Not proccessing the event=%d\n", evt);
	}
	return ret;
}

int disp_pe_rcv_cmd(struct policy *p, enum pe_event evt)
{
	struct disp_port_pe *disp_pe = container_of(p,
					struct disp_port_pe, p);
	int ret = 0;

	log_dbg("Received command, event=%d\n", evt);
	switch (evt) {
	case PE_EVT_RCVD_HARD_RESET:
	case PE_EVT_RCVD_HARD_RESET_COMPLETE:
		disp_pe_reset_policy_engine(disp_pe);
	default:
		ret = EINVAL;
		break;
	}

	return ret;
}


static void disp_pe_start_comm(struct work_struct *work)
{
	struct disp_port_pe *disp_pe = container_of(work,
					struct disp_port_pe,
					start_comm.work);

	if ((disp_pe->state == DISP_PE_STATE_ALT_MODE_FAIL)
		|| (disp_pe->state == DISP_PE_STATE_DISPLAY_CONFIGURED)) {
		log_dbg("Not required to send DI in this state=%d\n",
				disp_pe->state);
		return;
	}

	if (disp_pe->cmd_retry > 0)
		disp_pe_do_protocol_reset(disp_pe);

	log_info(" Sending DI\n");
	mutex_lock(&disp_pe->pe_lock);
	disp_pe->state = DISP_PE_STATE_DI_SENT;
	mutex_unlock(&disp_pe->pe_lock);
	disp_pe_send_discover_identity(disp_pe);

	disp_pe->cmd_retry++;
	if (disp_pe->cmd_retry < MAX_CMD_RETRY) {
		log_dbg("Re-scheduling the start_comm after %dSec\n",
				CMD_NORESPONCE_TIME);
		/* Retry display identity if dp command sequence
		 * failed/no responce with in CMD_NORESPONCE_TIME.
		 */
		schedule_delayed_work(&disp_pe->start_comm,
					HZ * CMD_NORESPONCE_TIME);
	} else {
		mutex_lock(&disp_pe->pe_lock);
		disp_pe->state = DISP_PE_STATE_ALT_MODE_FAIL;
		disp_pe->p.status = POLICY_STATUS_FAIL;
		mutex_unlock(&disp_pe->pe_lock);
		log_warn("Not scheduling srccap as max re-try reached\n");
	}
}

static int disp_pe_start_policy_engine(struct policy *p)
{
	struct disp_port_pe *disp_pe = container_of(p,
					struct disp_port_pe, p);

	log_dbg("IN");
	mutex_lock(&disp_pe->pe_lock);
	p->state = POLICY_STATE_ONLINE;
	p->status = POLICY_STATUS_RUNNING;
	disp_pe->cmd_retry = 0;
	schedule_delayed_work(&disp_pe->start_comm, 0);
	mutex_unlock(&disp_pe->pe_lock);
	return 0;
}

static int disp_pe_stop_policy_engine(struct policy *p)
{
	struct disp_port_pe *disp_pe = container_of(p,
					struct disp_port_pe, p);

	log_dbg("IN");
	mutex_lock(&disp_pe->pe_lock);
	p->state = POLICY_STATE_OFFLINE;
	p->status = POLICY_STATUS_UNKNOWN;
	cancel_delayed_work(&disp_pe->start_comm);
	disp_pe_reset_policy_engine(disp_pe);
	disp_pe->cmd_retry = 0;
	disp_pe->dp_mode = TYPEC_DP_TYPE_NONE;
	if (disp_pe->hpd_state) {
		policy_set_dp_state(&disp_pe->p, CABLE_DETACHED,
					TYPEC_DP_TYPE_NONE);
		disp_pe->hpd_state = false;
	}
	mutex_unlock(&disp_pe->pe_lock);
	return 0;
}

static void disp_pe_exit(struct policy *p)
{
	struct disp_port_pe *disp_pe = container_of(p,
					struct disp_port_pe, p);

	kfree(disp_pe);
}

/* Init function to initialize the source policy engine */
struct policy *disp_pe_init(struct policy_engine *pe)
{
	struct disp_port_pe *disp_pe;
	struct policy *p;

	disp_pe = kzalloc(sizeof(struct disp_port_pe),
						GFP_KERNEL);
	if (!disp_pe) {
		log_err("mem alloc failed\n");
		return ERR_PTR(-ENOMEM);
	}

	mutex_init(&disp_pe->pe_lock);
	INIT_DELAYED_WORK(&disp_pe->start_comm, disp_pe_start_comm);

	p = &disp_pe->p;
	p->type = POLICY_TYPE_DISPLAY;
	p->state = POLICY_STATE_OFFLINE;
	p->status = POLICY_STATUS_UNKNOWN;
	p->pe = pe;

	p->rcv_pkt = disp_pe_rcv_pkt;
	p->rcv_cmd = disp_pe_rcv_cmd;
	p->start  = disp_pe_start_policy_engine;
	p->stop = disp_pe_stop_policy_engine;
	p->exit = disp_pe_exit;

	log_info("Display pe initialized successfuly");
	return p;
}
EXPORT_SYMBOL_GPL(disp_pe_init);
