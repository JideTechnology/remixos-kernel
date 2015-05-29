#ifndef __POLICY_ENGINE_H__
#define __POLICY_ENGINE_H__

#include "message.h"
#include "devpolicy_mgr.h"
#include "protocol.h"

enum pe_event {

	/* Control Messages (0 - 13) */
	PE_EVT_SEND_NONE,
	PE_EVT_SEND_GOODCRC,
	PE_EVT_SEND_GOTOMIN,
	PE_EVT_SEND_ACCEPT,
	PE_EVT_SEND_REJECT,
	PE_EVT_SEND_PING,
	PE_EVT_SEND_PS_RDY,
	PE_EVT_SEND_GET_SRC_CAP,
	PE_EVT_SEND_GET_SINK_CAP,
	PE_EVT_SEND_DR_SWAP,
	PE_EVT_SEND_PR_SWAP,
	PE_EVT_SEND_VCONN_SWAP,
	PE_EVT_SEND_WAIT,
	PE_EVT_SEND_SOFT_RESET,

	/* Data Messages (14 - 18) */
	PE_EVT_SEND_SRC_CAP,
	PE_EVT_SEND_REQUEST,
	PE_EVT_SEND_BIST,
	PE_EVT_SEND_SNK_CAP,
	PE_EVT_SEND_VDM,

	/* Control Messages (19 - 32) */
	PE_EVT_RCVD_NONE,
	PE_EVT_RCVD_GOODCRC,
	PE_EVT_RCVD_GOTOMIN,
	PE_EVT_RCVD_ACCEPT,
	PE_EVT_RCVD_REJECT,
	PE_EVT_RCVD_PING,
	PE_EVT_RCVD_PS_RDY,
	PE_EVT_RCVD_GET_SRC_CAP,
	PE_EVT_RCVD_GET_SINK_CAP,
	PE_EVT_RCVD_DR_SWAP,
	PE_EVT_RCVD_PR_SWAP,
	PE_EVT_RCVD_VCONN_SWAP,
	PE_EVT_RCVD_WAIT,
	PE_EVT_RCVD_SOFT_RESET,

	/* Data Messages (33 - 37) */
	PE_EVT_RCVD_SRC_CAP,
	PE_EVT_RCVD_REQUEST,
	PE_EVT_RCVD_BIST,
	PE_EVT_RCVD_SNK_CAP,
	PE_EVT_RCVD_VDM,

	/* Other Messages (38 - 41) */
	PE_EVT_SEND_HARD_RESET,
	PE_EVT_SEND_PROTOCOL_RESET,
	PE_EVT_RCVD_HARD_RESET,
	PE_EVT_RCVD_HARD_RESET_COMPLETE,

};

enum pe_states {

	PE_STATE_STARTUP,

	/* Source Port (1 - 12 ) */
	PE_SRC_STARTUP,
	PE_SRC_DISCOVERY,
	PE_SRC_SEND_CAPABILITIES,
	PE_SRC_NEGOTIATE_CAPABILITY,
	PE_SRC_TRANSITION_SUPPLY,
	PE_SRC_READY,
	PE_SRC_DISABLED,
	PE_SRC_CAPABILITY_RESPONSE,
	PE_SRC_HARD_RESET,
	PE_SRC_TRANSITION_TO_DEFAULT,
	PE_SRC_GIVE_SOURCE_CAP,
	PE_SRC_GET_SINK_CAP,

	/* Sink Port (13 - 23) */
	PE_SNK_STARTUP,
	PE_SNK_DISCOVERY,
	PE_SNK_WAIT_FOR_CAPABILITIES,
	PE_SNK_EVALUATE_CAPABILITY,
	PE_SNK_SELECT_CAPABILITY,
	PE_SNK_TRANSITION_SINK,
	PE_SNK_READY,
	PE_SNK_HARD_RESET,
	PE_SNK_TRANSITION_TO_DEFAULT,
	PE_SNK_GIVE_SINK_CAP,
	PE_SNK_GET_SOURCE_CAP,

	/* Source Port Soft Reset (24, 25) */
	PE_SRC_SEND_SOFT_RESET,
	PE_SRC_SOFT_RESET,

	/* Sink Port Soft Reset (26, 27) */
	PE_SNK_SEND_SOFT_RESET,
	PE_SNK_SOFT_RESET,

	/* Source Port Ping (28)*/
	PE_SRC_PING,

	/* Type-A/B Dual-Role (initially Source Port) Ping (29) */
	PE_PRS_SRC_SNK_PING,

	/* Type-A/B Dual-Role (initially Sink Port) Ping (30) */
	PE_PRS_SNK_SRC_PING,

	/* Type-A/B Hard Reset of P/C in Sink Role (31, 32) */
	PE_PC_SNK_HARD_RESET,
	PE_PC_SNK_SWAP_RECOVERY,

	/* Type-A/B Hard Reset of C/P in Source Role (33, 34) */
	PE_CP_SRC_HARD_RESET,
	PE_CP_SRC_TRANSITION_TO_OFF,

	/* Type-A/B C/P Dead Battery/Power Loss (35 - 41) */
	PE_DB_CP_CHECK_FOR_VBUS,
	PE_DB_CP_POWER_VBUS_DB,
	PE_DB_CP_WAIT_FOR_BIT_STREAM,
	PE_DB_CP_POWER_VBUS_5V,
	PE_DB_CP_WAIT_BIT_STREAM_STOP,
	PE_DB_CP_UNPOWER_VBUS,
	PE_DB_CP_PS_DISCHARGE,

	/* Type-A/B P/C Dead Battery/Power Loss (42 - 46) */
	PE_DB_PC_UNPOWERED,
	PE_DB_PC_CHECK_POWER,
	PE_DB_PC_SEND_BIT_STREAM,
	PE_DB_PC_WAIT_TO_DETECT,
	PE_DB_PC_WAIT_TO_START,

	/* Type-C DFP to UFP Data Role Swap (47 - 51) */
	PE_DRS_DFP_UFP_EVALUATE_DR_SWAP,
	PE_DRS_DFP_UFP_ACCEPT_DR_SWAP,
	PE_DRS_DFP_UFP_CHANGE_TO_UFP,
	PE_DRS_DFP_UFP_SEND_DR_SWAP,
	PE_DRS_DFP_UFP_REJECT_DR_SWAP,

	/* Type-C UFP to DFP Data Role Swap (52 - 56) */
	PE_DRS_UFP_DFP_EVALUATE_DR_SWAP,
	PE_DRS_UFP_DFP_ACCEPT_DR_SWAP,
	PE_DRS_UFP_DFP_CHANGE_TO_DFP,
	PE_DRS_UFP_DFP_SEND_DR_SWAP,
	PE_DRS_UFP_DFP_REJECT_DR_SWAP,

	/* Source to Sink Power Role Swap (57 - 63) */
	PE_PRS_SRC_SNK_EVALUATE_PR_SWAP,
	PE_PRS_SRC_SNK_ACCEPT_PR_SWAP,
	PE_PRS_SRC_SNK_TRANSITION_TO_OFF,
	PE_PRS_SRC_SNK_ASSERT_RD,
	PE_PRS_SRC_SNK_SOURCE_OFF,
	PE_PRS_SRC_SNK_SEND_PR_SWAP,
	PE_PRS_SRC_SNK_REJECT_PR_SWAP,

	/* Sink to Source Power Role Swap (64 - 70) */
	PE_PRS_SNK_SRC_EVALUATE_PR_SWAP,
	PE_PRS_SNK_SRC_ACCEPT_PR_SWAP,
	PE_PRS_SNK_SRC_TRANSITION_TO_OFF,
	PE_PRS_SNK_SRC_ASSERT_RP,
	PE_PRS_SNK_SRC_SOURCE_ON,
	PE_PRS_SNK_SRC_SEND_PR_SWAP,
	PE_PRS_SNK_SRC_REJECT_PR_SWAP,

	/* Dual-Role Source Port Get Source Capabilities (71) */
	PE_DR_SRC_GET_SOURCE_CAP,

	/* Dual-Role Source Port Give Sink Capabilities (72) */
	PE_DR_SRC_GIVE_SINK_CAP,

	/* Dual-Role Sink Port Get Sink Capabilities (73) */
	PE_DR_SNK_GET_SINK_CAP,

	/* Dual-Role Sink Port Give Source Capabilities (74) */
	PE_DR_SNK_GIVE_SOURCE_CAP,

	/* Type-C DFP VCONN Swap (75 - 79) */
	PE_VCS_DFP_SEND_SWAP,
	PE_VCS_DFP_WAIT_FOR_UFP_VCONN,
	PE_VCS_DFP_TURN_OFF_VCONN,
	PE_VCS_DFP_TURN_ON_VCONN,
	PE_VCS_DFP_SEND_PS_Rdy,

	/* Type-C UFP VCONN Swap (80 - 86) */
	PE_VCS_UFP_EVALUATE_SWAP,
	PE_VCS_UFP_ACCEPT_SWAP,
	PE_VCS_UFP_REJECT_SWAP,
	PE_VCS_UFP_WAIT_FOR_DFP_VCONN,
	PE_VCS_UFP_TURN_OFF_VCONN,
	PE_VCS_UFP_TURN_ON_VCONN,
	PE_VCS_UFP_SEND_PS_RDY,

	/* UFP VDM (87 - 97) */
	PE_UFP_VDM_GET_IDENTITY,
	PE_UFP_VDM_SEND_IDENTITY,
	PE_UFP_VDM_GET_SVIDS,
	PE_UFP_VDM_SEND_SVIDS,
	PE_UFP_VDM_GET_MODES,
	PE_UFP_VDM_SEND_MODES,
	PE_UFP_VDM_EVALUATE_MODE_ENTRY,
	PE_UFP_VDM_MODE_ENTRY_ACK,
	PE_UFP_VDM_MODE_ENTRY_NAK,
	PE_UFP_VDM_MODE_EXIT,
	PE_UFP_VDM_MODE_EXIT_ACK,

	/* UFP VDM Attention (98) */
	PE_UFP_VDM_ATTENTION_REQUEST,

	/* DFP VDM Discover Identity (99 - 101) */
	PE_DFP_VDM_IDENTITY_REQUEST,
	PE_DFP_VDM_IDENTITY_ACKED,
	PE_DFP_VDM_IDENTITY_NAKED,

	/* DFP VDM Discover SVIDs (102 - 104) */
	PE_DFP_VDM_SVIDS_REQUEST,
	PE_DFP_VDM_SVIDS_ACKED,
	PE_DFP_VDM_SVIDS_NAKED,

	/* DFP VDM Discover Modes (105 - 107) */
	PE_DFP_VDM_MODES_REQUEST,
	PE_DFP_VDM_MODES_ACKED,
	PE_DFP_VDM_MODES_NAKED,

	/* DFP VDM Mode Entry (108 - 110) */
	PE_DFP_VDM_MODES_ENTRY_REQUEST,
	PE_DFP_VDM_MODES_ENTRY_ACKED,
	PE_DFP_VDM_MODES_ENTRY_NAKED,

	/* DFP VDM Mode Exit (111, 112) */
	PE_DFP_VDM_MODE_EXIT_REQUEST,
	PE_DFP_VDM_MODE_EXIT_ACKED,

	/* Source Startup VDM Discover Identity (113 - 115) */
	PE_SRC_VDM_IDENTITY_REQUEST,
	PE_SRC_VDM_IDENTITY_ACKED,
	PE_SRC_VDM_IDENTITY_NAKED,

	/* DFP VDM Attention (116) */
	PE_DFP_VDM_ATTENTION_REQUEST,

	/* USB to USB Cable (117 - 128) */
	PE_CBL_READY,
	PE_CBL_GET_IDENTITY,
	PE_CBL_SEND_IDENTITY,
	PE_CBL_GEG_SVIDS,
	PE_CBL_SEND_SVIDS,
	PE_CBL_GEG_MODES,
	PE_CBL_SEND_MODES,
	PE_CBL_EVALUATE_MODE_ENTRY,
	PE_CBL_MODE_ENTRY_ACK,
	PE_CBL_MODE_ENTRY_NAK,
	PE_CBL_MODE_EXUIT,
	PE_CBL_MODE_EXIT_ACK,

	/* Cable Soft Reset (129) */
	PE_CBL_SOFT_RESET,

	/* Cable Hard Reset (130) */
	PE_CBL_HARD_RESET,

	/* BIST Receive Mode (131, 132) */
	PE_BIST_RECEIVE_MODE,
	PE_BIST_FRAME_RECEIVED,

	/* BIST Transmit Mode (133, 134) */
	PE_BIST_TRANSMIT_MODE,
	PE_BIST_SEND_FRAME,

	/* BIST Carrier Mode and Eye Pattern (135 - 139) */
	PE_BIST_EYE_PATTERN_MODE,
	PE_BIST_CARRIER_MODE_0,
	PE_BIST_CARRIER_MODE_1,
	PE_BIST_CARRIER_MODE_2,
	PE_BIST_CARRIER_MODE_3,

	/* Type-C referenced states (140) */
	ERROR_RECOVERY,
};

enum policy_state {
	POLICY_STATE_UNKNOWN,
	POLICY_STATE_OFFLINE,
	POLICY_STATE_ONLINE,
};

enum policy_status {
	POLICY_STATUS_UNKNOWN,
	POLICY_STATUS_STOPPED,
	POLICY_STATUS_STARTED,
};

struct policy {
	enum policy_type type;
	struct policy_engine *pe;
	struct list_head list;
	enum policy_status status;
	void *priv;
	int (*start)(struct policy *p);
	int (*stop)(struct policy *p);
	int (*rcv_pkt)(struct policy *p, struct pd_packet *pkt,
				enum pe_event evt);
	int (*rcv_cmd)(struct policy *p, enum pe_event evt);
};

struct policy_engine {
	struct pd_prot *prot;
	struct notifier_block proto_nb;
	struct notifier_block dpm_nb;
	struct mutex protowk_lock;
	struct mutex dpmwk_lock;
	struct mutex pe_lock;
	struct list_head proto_queue;
	struct list_head dpm_queue;
	struct work_struct proto_work;
	struct work_struct dpm_work;
	struct devpolicy_mgr *dpm;
	struct pe_operations *ops;
	struct pd_packet pkt;
	struct list_head list;

	enum pe_event prev_evt;
	spinlock_t proto_queue_lock;
	spinlock_t dpm_queue_lock;

	struct pd_policy *supported_policies;
	struct notifier_block sink_nb;
	struct notifier_block source_nb;
	struct extcon_specific_cable_nb sink_cable_nb;
	struct extcon_specific_cable_nb source_cable_nb;
	struct work_struct policy_work;
	struct work_struct policy_init_work;

	struct list_head policy_list;
	enum policy_type policy_in_use;
	enum policy_state state;
	bool is_pd_connected;
};

struct pe_operations {
	int (*get_snkpwr_cap)(struct policy_engine *pe,
					struct power_cap *cap);
	int (*get_snkpwr_caps)(struct policy_engine *pe,
					struct power_caps *caps);
	int (*get_max_snkpwr_cap)(struct policy_engine *pe,
					struct power_cap *cap);
	enum data_role (*get_data_role)(struct policy_engine *pe);
	enum pwr_role (*get_power_role)(struct policy_engine *pe);
	void (*set_data_role)(struct policy_engine *pe, enum data_role role);
	void (*set_power_role)(struct policy_engine *pe, enum pwr_role role);
	int (*set_charger_mode)(struct policy_engine *pe,
					enum charger_mode mode);
	int (*update_charger_ilim)(struct policy_engine *pe,
					int ilim);
	int (*get_min_snk_current)(struct policy_engine *pe,
					int *ma);
	int (*send_packet)(struct policy_engine *pe, void *data,
				int len, u8 msg_type, enum pe_event evt);
	enum cable_state (*get_vbus_state)(struct policy_engine *pe);
	int (*set_pd_state)(struct policy_engine *pe, bool state);
	bool (*get_pd_state)(struct policy_engine *pe);
	int (*process_msg)(struct policy_engine *pe, struct pd_packet *data);
	int (*process_cmd)(struct policy_engine *pe, int cmd);
	void (*update_policy_engine)(struct policy_engine *pe, int policy_type,
					int state);
};

struct pe_proto_evt {
	struct list_head node;
	struct pd_packet pkt;
};

struct pe_dpm_evt {
	struct list_head node;
	struct power_cap caps;
};

#define pe_get_phy(x)	((x) ?  x->dpm->phy : NULL)

static inline const char *policy_port_type_string(enum policy_type ptype)
{
	switch (ptype) {
	case POLICY_TYPE_SOURCE:
	return "SOURCE PORT PE";
	case POLICY_TYPE_SINK:
		return "SINK PORT PE";
	case POLICY_TYPE_DISPLAY:
		return "DISPLAY PORT PE";
	default:
		return "UNKNOWN PE TYPE";
	}
}

/* methods to initialize/destroy the policy manager */
static inline int policy_get_snkpwr_cap(struct policy *p, struct power_cap *cap)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->get_snkpwr_cap)
		return p->pe->ops->get_snkpwr_cap(p->pe, cap);

	return -ENOTSUPP;
}

static inline int policy_get_snkpwr_caps(struct policy *p,
						struct power_caps *caps)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->get_snkpwr_caps)
		return p->pe->ops->get_snkpwr_caps(p->pe, caps);

	return -ENOTSUPP;
}

static inline int policy_get_max_snkpwr_cap(struct policy *p,
						struct power_cap *cap)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->get_max_snkpwr_cap)
		return p->pe->ops->get_max_snkpwr_cap(p->pe, cap);

	return -ENOTSUPP;
}

static inline enum data_role policy_get_data_role(struct policy *p)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->get_data_role)
		return p->pe->ops->get_data_role(p->pe);

	return DATA_ROLE_NONE;
}

static inline enum pwr_role policy_get_power_role(struct policy *p)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->get_power_role)
		return p->pe->ops->get_power_role(p->pe);

	return POWER_ROLE_NONE;
}

static inline void policy_set_data_role(struct policy *p, int role)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->set_data_role)
		p->pe->ops->set_data_role(p->pe, role);
}

static inline void policy_set_power_role(struct policy *p, int role)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->set_power_role)
		p->pe->ops->set_power_role(p->pe, role);
}

static inline int policy_set_charger_mode(struct policy *p,
						enum charger_mode mode)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->set_charger_mode)
		return p->pe->ops->set_charger_mode(p->pe, mode);

	return -ENOTSUPP;
}

static inline int policy_update_charger_ilim(struct policy *p,
						int ilim)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->update_charger_ilim)
		return p->pe->ops->update_charger_ilim(p->pe, ilim);

	return -ENOTSUPP;
}

static inline int policy_get_min_current(struct policy *p,
						int *ma)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->get_min_snk_current)
		return p->pe->ops->get_min_snk_current(p->pe, ma);

	return -ENOTSUPP;
}

static inline int policy_send_packet(struct policy *p, void *data, int len,
					u8 msg_type, enum pe_event evt)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->send_packet)
		return p->pe->ops->send_packet(p->pe, data, len, msg_type, evt);

	return -ENOTSUPP;
}

static inline enum cable_state policy_get_vbus_state(struct policy *p)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->get_vbus_state)
		return p->pe->ops->get_vbus_state(p->pe);

	return -ENOTSUPP;
}

static inline int policy_set_pd_state(struct policy *p, bool state)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->set_pd_state)
		return p->pe->ops->set_pd_state(p->pe, state);

	return -ENOTSUPP;
}

static inline bool policy_get_pd_state(struct policy *p)
{
	if (p && p->pe && p->pe->ops && p->pe->ops->get_pd_state)
		return p->pe->ops->get_pd_state(p->pe);

	return -ENOTSUPP;
}

#if defined(CONFIG_USBC_PD) && defined(CONFIG_USBC_PD_POLICY)
extern int policy_engine_bind_dpm(struct devpolicy_mgr *dpm);
extern void policy_engine_unbind_dpm(struct devpolicy_mgr *dpm);
#else /* CONFIG_USBC_PD && CONFIG_USBC_PD_POLICY */
static inline int policy_engine_bind_dpm(struct devpolicy_mgr *dpm)
{
	return 0;
}
static inline void policy_engine_unbind_dpm(struct devpolicy_mgr *dpm)
{ }
#endif /* CONFIG_USBC_PD && CONFIG_USBC_PD_POLICY */

static inline int pe_send_cmd(struct policy_engine *pe, int cmd)
{
	if (pe && pe->ops && pe->ops->process_cmd)
		return pe->ops->process_cmd(pe, cmd);

	return -ENOTSUPP;
}

static inline int pe_send_msg(struct policy_engine *pe, struct pd_packet *pkt)
{
	if (pe && pe->ops && pe->ops->process_msg)
		return pe->ops->process_msg(pe, pkt);

	return -ENOTSUPP;
}

extern struct policy *sink_port_policy_init(struct policy_engine *pe);
extern int dpm_register_pe(struct policy_engine *x, int port);
extern void dpm_unregister_pe(struct policy_engine *x);
extern int protocol_bind_pe(struct policy_engine *pe);
extern void protocol_unbind_pe(struct policy_engine *pe);

#endif /* __POLICY_ENGINE_H__ */
