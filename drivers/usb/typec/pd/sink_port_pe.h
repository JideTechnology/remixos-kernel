#ifndef __SINK_PORT_PE__H__
#define __SINK_PORT_PE__H__

#include "policy_engine.h"

#define SNK_FSPDO_VOLT_SHIFT		10

#define SNK_FSPDO_FIXED_SUPPLY		(3 << 30)
#define SNK_FSPDO_DUAL_ROLE_PWR		(1 << 29)
#define SNK_FSPDO_HIGHTER_CAPABILITY	(1 << 28)
#define SNK_FSPDO_EXT_POWERED		(1 << 27)
#define SNK_FSPDO_USB_COMM_CAPABLE	(1 << 26)
#define SNK_FSPDO_DATA_ROLE_SWAP	(1 << 25)
#define SNK_FSPDO_RESERVED		(3 << 20)
#define SNK_FSPDO_VOLTAGE		(0x3FF << SNK_FSPDO_VOLT_SHIFT)
#define SNK_FSPDO_MAX_CURRENT		(0x3FF << 0)

#define IS_DUAL_ROLE_POWER(x)	(x & SNK_FSPDO_DUAL_ROLE_PWR)
#define IS_USB_SUSPEND_SUPP(x)	(x & SNK_FSPDO_HIGHTER_CAPABILITY)
#define IS_EXT_POWERED(x)	(x & SNK_FSPDO_EXT_POWERED)
#define IS_USB_COMM_CAP(x)	(x & SNK_FSPDO_USB_COMM_CAPABLE)
#define IS_DATA_ROLE_SWAP(x)	(x & SNK_FSPDO_DATA_ROLE_SWAP)

/* returns in mV */
#define DATA_OBJ_TO_VOLT(x)	(((x & SNK_FSPDO_VOLTAGE) >>	\
					SNK_FSPDO_VOLT_SHIFT) * 50)
/* returns in mA */
#define DATA_OBJ_TO_CURRENT(x)	((x & SNK_FSPDO_MAX_CURRENT) * 10)

#define VOLT_TO_DATA_OBJ(x)	(((x / 50) << SNK_FSPDO_VOLT_SHIFT) &	\
					SNK_FSPDO_VOLTAGE)
#define CURRENT_TO_DATA_OBJ(x)	((x / 10) & SNK_FSPDO_MAX_CURRENT)

#define REQ_DOBJ_OBJ_POS_SHIFT		28
#define REQ_DOBJ_GB_FLAG_SHIFT		27
#define REQ_DOBJ_CAP_MISMATCH_SHIFT	26
#define REQ_DOBJ_USB_COMM_CAPABLE_SHIFT	25
#define REQ_DOBJ_NO_USB_SUSPEND_SHIFT	24
#define REQ_DOBJ_OPERATING_CUR_SHIFT	10
#define REQ_DOBJ_MAX_OP_CUR_SHIFT	0

#define REQ_DOBJ_OBJ_POSITION		(7 << REQ_DOBJ_OBJ_POS_SHIFT)
#define REQ_DOBJ_GIVEBACK_FLAG		(1 << REQ_DOBJ_GB_FLAG_SHIFT)
#define REQ_DOBJ_CAP_MISMATCH		(1 << REQ_DOBJ_CAP_MISMATCH_SHIFT)
#define REQ_DOBJ_USB_COMM_CAPABLE	(1 << REQ_DOBJ_USB_COMM_CAPABLE_SHIFT)
#define REQ_DOBJ_NO_USB_SUSPEND		(1 << REQ_DOBJ_NO_USB_SUSPEND_SHIFT)
#define REQ_DOBJ_OPERATING_CUR		(0x3FF << REQ_DOBJ_OPERATING_CUR_SHIFT)
#define REQ_DOBJ_MAX_OPERATING_CUR	(0x3FF << REQ_DOBJ_MAX_OP_CUR_SHIFT)

#define TYPEC_SENDER_RESPONSE_TIMER	30 /* min: 24mSec; max: 30mSec */
#define TYPEC_SINK_WAIT_CAP_TIMER	2500 /* min 2.1Sec; max: 2.5Sec */
#define TYPEC_NO_RESPONSE_TIMER		5500 /* min 4.5Sec; max: 5.5Sec */
#define TYPEC_PS_TRANSITION_TIMER	550 /* min 450mSec; max: 550mSec */
#define TYPEC_SINK_ACTIVITY_TIMER	150 /* min 120mSec; max: 150mSec */
#define TYPEC_SINK_REQUEST_TIMER	100 /* min 100mSec; max: ? */
#define HARD_RESET_COUNT_N		2

enum {
	CHRGR_UNKNOWN,
	CHRGR_SET_HZ,
	CHRGR_ENABLE,
};

struct snk_cable_event {
	struct list_head node;
	bool vbus_state;
};

enum snkpe_timeout {
	UNKNOWN_TIMER,
	SINK_WAIT_CAP_TIMER,
	SINK_ACTIVITY_TIMER,
	SINK_REQUEST_TIMER,
	PS_TRANSITION_TIMER,
	NO_RESPONSE_TIMER,
	SENDER_RESPONSE_TIMER,
};

struct req_cap {
	u8 obj_pos;
	bool cap_mismatch;
	u32 ma;
	u32 mv;
};

struct sink_port_pe {
	struct policy p;
	struct pd_packet prev_pkt;
	struct completion wct_complete; /* wait cap timer */
	struct completion srt_complete; /* sender response timer */
	struct completion nrt_complete; /* no response timer */
	struct completion pstt_complete; /* PS Transition timer */
	struct completion sat_complete; /* Sink Activity timer */
	struct completion srqt_complete; /* Sink Request timer */
	struct completion pktwt_complete; /* fifo write complete */
	struct kfifo pkt_fifo;
	struct req_cap rcap;
	int ilim;
	enum pe_event pevt;
	enum pe_states cur_state;
	enum pe_states prev_state;
	struct mutex snkpe_state_lock;
	enum snkpe_timeout timeout;
	struct work_struct timer_work;
	u8 hard_reset_count;
	bool is_vbus_connected;
};

static int snkpe_start(struct sink_port_pe *sink);
static int snkpe_handle_snk_ready_state(struct sink_port_pe *sink,
						enum pe_event evt);
static int snkpe_handle_select_capability_state(struct sink_port_pe *sink,
							struct pd_packet *pkt);
static int snkpe_handle_transition_sink_state(struct sink_port_pe *sink);
static int snkpe_handle_give_snk_cap_state(struct sink_port_pe *sink);
static int snkpe_handle_evaluate_capability(struct sink_port_pe *sink);
static int snkpe_handle_transition_to_default(struct sink_port_pe *sink);
static int snkpe_vbus_attached(struct sink_port_pe *sink);
static void sink_port_policy_exit(struct policy *p);

static inline bool snkpe_is_cur_state(struct sink_port_pe *sink,
					enum pe_states state)
{
	return sink->cur_state == state;
}

static inline bool snkpe_is_prev_state(struct sink_port_pe *sink,
					enum pe_states state)
{
	return sink->prev_state == state;
}

static inline bool snkpe_is_prev_evt(struct sink_port_pe *sink,
					enum pe_event evt)
{
	return sink->pevt == evt;
}

#endif /* __SINK_PORT_PE__H__ */
