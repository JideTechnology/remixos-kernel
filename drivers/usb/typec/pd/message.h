/*
 * message.h: usb type-c Power Delivery message header file
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
 * Author: Albin, B <albin.bala.krishnan@intel.com>
 */

#ifndef __PD_MESSAGE_H__
#define __PD_MESSAGE_H__

#define PD_REV_ID_1		0
#define PD_REV_ID_2		1
#define PD_REV_ID_RESERVED	3

/* PD Message Header Details */
#define PD_MSG_HEAD_RSV15		(1 << 15)
#define PD_MSG_HEAD_NUM_DATA_OBJS	(7 << 12)
#define PD_MSG_HEAD_MSG_ID		(7 << 9)
#define PD_MSG_HEAD_PORT_PWR_ROLE	(1 << 8)
#define PD_MSG_HEAD_SPEC_REV		(3 << 6)
#define PD_MSG_HEAD_PORT_DATA_ROLE	(1 << 5)
#define PD_MSG_HEAD_RSV4		(1 << 4)
#define PD_MSG_HEAD_MSG_TYPE		(0xf << 0)

/* PD Control Message Types */
#define PD_CTRL_MSG_RESERVED_0		0
#define PD_CTRL_MSG_GOODCRC		1
#define PD_CTRL_MSG_GOTOMIN		2
#define PD_CTRL_MSG_ACCEPT		3
#define PD_CTRL_MSG_REJECT		4
#define PD_CTRL_MSG_PING		5
#define PD_CTRL_MSG_PS_RDY		6
#define PD_CTRL_MSG_GET_SRC_CAP		7
#define PD_CTRL_MSG_GET_SINK_CAP	8
#define PD_CTRL_MSG_DR_SWAP		9
#define PD_CTRL_MSG_PR_SWAP		10
#define PD_CTRL_MSG_VCONN_SWAP		11
#define PD_CTRL_MSG_WAIT		12
#define PD_CTRL_MSG_SOFT_RESET		13

#define PD_DATA_MSG_RESERVED_0		0
#define PD_DATA_MSG_SRC_CAP		1
#define PD_DATA_MSG_REQUEST		2
#define PD_DATA_MSG_BIST		3
#define PD_DATA_MSG_SINK_CAP		4
#define PD_DATA_MSG_RESERVED_5		5
#define PD_DATA_MSG_RESERVED_6		6
#define PD_DATA_MSG_RESERVED_7		7
#define PD_DATA_MSG_RESERVED_8		8
#define PD_DATA_MSG_RESERVED_9		9
#define PD_DATA_MSG_RESERVED_10		10
#define PD_DATA_MSG_RESERVED_11		11
#define PD_DATA_MSG_RESERVED_12		12
#define PD_DATA_MSG_RESERVED_13		13
#define PD_DATA_MSG_RESERVED_14		14
#define PD_DATA_MSG_VENDOR_DEF		15

#define PD_CMD_HARD_RESET		16
#define PD_CMD_PROTOCOL_RESET		17
#define PD_CMD_HARD_RESET_COMPLETE	18

#define SUPPLY_TYPE_FIXED		0
#define SUPPLY_TYPE_BATTERY		1
#define SUPPLY_TYPE_VARIABLE		2
#define SUPPLY_TYPE_UNKNOWN		3

#define PD_MSG_HEADER_SIZE		2
#define PD_MSG_SIZE_PER_DATA_OBJ	4
#define	MAX_NUM_DATA_OBJ		7

#define PD_MSG_HEADER_ROLE_BITS_MASK	0x1
#define PD_MSG_HEADER_REVID_BITS_MASK	0x3
#define PD_MSG_HEADER_MSGID_BITS_MASK	0x7
#define PD_MSG_HEADER_N_DOBJ_BITS_MASK	0x7

enum vdm_cmd_type {
	INITIATOR,
	REP_ACK,
	REP_NACK,
	REP_BUSY,
};

enum vdm_cmd {
	RESVD,
	DISCOVER_IDENTITY,
	DISCOVER_SVID,
	DISCOVER_MODE,
	ENTER_MODE,
	EXIT_MODE,
	ATTENTION,
	DP_STATUS_UPDATE = 0x10,
	DP_CONFIGURE,
};

struct pd_pkt_header {
	u8 msg_type:4;
	u8 rsvd_a:1;
	u8 data_role:1;
	u8 rev_id:2;
	u8 pwr_role:1;
	u8 msg_id:3;
	u8 num_data_obj:3;
	u8 rsvd_b:1;
} __packed;

struct vdm_msg_header {
	u8 msg_type:4;
	u8 rsvd_a:2;
	u8 spec_rev:2;
	u8 pwr_role:1;
	u8 msg_id:3;
	u8 num_data_obj:3;
	u8 rsvd_b:1;
} __packed;

struct vdm_header {
	u8 cmd:5;
	u8 rsvd_a:1;
	u8 cmd_type:2;
	u8 obj_pos:3;
	u8 rsvd_b:2;
	u8 str_vdm_version:2;
	u8 vdm_type:1;
	u16 svid;
} __packed;

struct id_header {
	u16 vendor_id;
	u16 rsvd:10;
	u16 modal_op_supported:1;
	u16 product_type:3;
	u16 is_usb_dev_capable:1;
	u16 is_usb_host_capable:1;
} __packed;

struct cert_stat_vdo {
	u32 tid:20;
	u32 rsvd:12;
} __packed;

struct cable_vdo {
	u32 superspeed_signal_support:3;
	u32 sop_controller_present:1;
	u32 vbus_through_cable:1;
	u32 vbus_current_cap:2;
	u32 ssrx2_support:1;
	u32 ssrx1_support:1;
	u32 sstx2_support:1;
	u32 sstx1_support:1;
	u32 cable_term_type:2;
	u32 cable_latency:4;
	u32 typec_to_plug:1;
	u32 typec_typeabc:2;
	u32 rsvd:4;
	u32 cable_fw_ver:4;
	u32 cable_hw_ver:4;
} __packed;

struct product_vdo {
	u16 bcd_dev;
	u16 product_id;
} __packed;

struct vdm_req_pkt {
	struct pd_pkt_header msg_hdr;
	struct vdm_header vdm_hdr;
} __packed;

struct dis_id_response_cable_pkt {
	struct pd_pkt_header msg_hdr;
	struct vdm_header vdm_hdr;
	struct id_header id_hdr;
	struct cert_stat_vdo vdo1;
	struct cable_vdo vdo2;
} __packed;

struct dis_id_response_hub_pkt {
	struct pd_pkt_header msg_hdr;
	struct vdm_header vdm_hdr;
	struct id_header id_hdr;
	struct cert_stat_vdo vdo1;
	struct product_vdo vdo2;
} __packed;

struct dp_vdo {
	u16 svid0;
	u16 svid1;
} __packed;

struct dis_svid_response_pkt {
	struct pd_pkt_header msg_hdr;
	struct vdm_header vdm_hdr;
	struct dp_vdo vdo[6];
} __packed;

struct disp_mode {
	u32 port_cap:2;
	u32 dp_sig:4;
	u32 r_ind:1;
	u32 usb_sig:1;
	u32 dfp_pin:8;
	u32 ufp_pin:8;
	u32 rsrvd:8;
} __packed;

struct disp_config {
	u32 conf_sel:2;
	u32 trans_sig:4;
	u32 rsrvd:2;
	u32 dfp_pin:8;
	u32 ufp_pin:8;
	u32 rsrvd1:8;
} __packed;

struct dis_mode_response_pkt {
	struct pd_pkt_header msg_hdr;
	struct vdm_header vdm_hdr;
	struct disp_mode mode[6];
} __packed;

struct dis_port_status {
	u32 dev_connected:2;
	u32 pwr_low:1;
	u32 enabled:1;
	u32 multi_func:1;
	u32 usb_conf:1;
	u32 exit_mode:1;
	u32 hpd_state:1;
	u32 irq_hpd:1;
	u32 rsrvd:7;
	u32 rsrvd1:16;
} __packed;

struct pd_sink_fixed_pdo {
	u32 max_cur:10;
	u32 volt:10;
	u32 rsvd:5;
	u32 data_role_swap:1;
	u32 usb_comm:1;
	u32 ext_powered:1;
	u32 higher_cap:1;
	u32 dual_role_pwr:1;
	u32 supply_type:2;
} __packed;

struct pd_fixed_supply_pdo {
	u32 max_cur:10;
	u32 volt:10;
	u32 peak_cur:2;
	u32 rsvd:3;
	u32 data_role_swap:1;
	u32 usb_comm:1;
	u32 ext_powered:1;
	u32 usb_suspend:1;
	u32 dual_role_pwr:1;
	u32 fixed_supply:2;
} __packed;

struct pd_src_variable_pdo {
	u32 max_cur:10;
	u32 min_volt:10;
	u32 max_volt:10;
	u32 supply_type:2;
} __packed;

struct pd_src_battery_pdo {
	u32 max_power:10;
	u32 min_volt:10;
	u32 max_volt:10;
	u32 supply_type:2;
} __packed;


struct pd_fixed_var_rdo {
	u32 max_cur:10;
	u32 op_cur:10;
	u32 rsvd1:4;
	u32 no_usb_susp:1;
	u32 usb_comm_capable:1;
	u32 cap_mismatch:1;
	u32 give_back:1;
	u32 obj_pos:3;
	u32 rsvd2:1;
} __packed;

struct pd_packet {
	struct pd_pkt_header header;
	u32 data_obj[MAX_NUM_DATA_OBJ+1]; /* +1 is for CRC */
} __packed;

struct pd_buffer {
	struct pd_packet pkt;
	struct list_head node;
};

#define PD_MSG_NUM_DATA_OBJS(header)	((header)->num_data_obj)
#define IS_CTRL_MSG(header)		!PD_MSG_NUM_DATA_OBJS(header)
#define IS_DATA_MSG(header)		(PD_MSG_NUM_DATA_OBJS(header) != 0)
#define PD_MSG_TYPE(header)		((header)->msg_type)
#define PD_MSG_ID(header)		((header)->msg_id)
#define PD_MSG_LEN(header)		(PD_MSG_NUM_DATA_OBJS(header)	\
						* PD_MSG_SIZE_PER_DATA_OBJ)
#define IS_DATA_VDM(header)	((PD_MSG_TYPE(header) ==	\
					PD_DATA_MSG_VENDOR_DEF) ? 1 : 0)

static inline int to_pd_revision(int rev)
{
	if (rev == 1)
		return PD_REV_ID_1;
	else if (rev == 2)
		return PD_REV_ID_2;
	else
		return PD_REV_ID_RESERVED;
}

static inline u32 pd_fixed_pdo_to_current(u32 pdo)
{
	return (pdo & 0x3ff) * 10;
}

static inline u32 pd_cur_to_fixed_pdo(u32 cur)
{
	return cur / 10; /* 10s of mA */
}

static inline u32 pd_fixed_pdo_to_volt(u32 pdo)
{
	return (pdo & 0x3ff) * 50;
}

struct pd_prot;
extern int pd_ctrl_msg(struct pd_prot *pd, u8 msg_type, u8 msg_id);
extern int pd_data_msg(struct pd_prot *pd, int len, u8 msg_type);
#endif /* __PD_MESSAGE_H__ */
