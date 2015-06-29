
#ifndef __USB_PD_PROT_H__
#define __USB_PD_PROT_H__

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/extcon.h>
#include <linux/usb_typec_phy.h>
#include "message.h"
#include "policy_engine.h"

#define PD_MAX_MSG_ID	8

enum {
	PROT_EVENT_NONE,
	PROT_EVENT_COLLISION,
	PROT_EVENT_DISCARD,
	PROT_EVENT_MSGID_MISMATCH,
	PROT_EVENT_TX_FAIL,
	PROT_EVENT_TX_COMPLETED,
};

enum prot_tx_fsm {
	PROT_TX_PHY_LAYER_RESET,
	PROT_TX_MSG_RCV,
	PROT_TX_MSG_SENT,
	PROT_TX_MSG_ERROR,
};

struct pd_prot {
	struct typec_phy *phy;
	struct policy_engine *pe;
	u32 retry_count;
	u8 pd_version;

	u8 init_data_role;
	u8 new_data_role;
	u8 assumed_data_role;
	u8 init_pwr_role;
	u8 new_pwr_role;
	u8 assumed_pwr_role;
	u8 event;
	u8 tx_msg_id;
	s32 rx_msg_id;
	u8 retry_counter;

	u8 cur_tx_state;
	struct pd_packet tx_buf;
	struct pd_packet cached_rx_buf;

	struct completion tx_complete;
	struct completion goodcrc_sent;

	struct mutex rx_data_lock;
	struct mutex tx_data_lock;
	struct mutex tx_lock;

	struct list_head list;
	struct work_struct cable_event_work;

	struct extcon_specific_cable_nb cable_ufp;
	struct extcon_specific_cable_nb cable_dfp;
	struct notifier_block ufp_nb;
	struct notifier_block dfp_nb;
	struct notifier_block phy_nb;
	int (*policy_fwd_pkt)(struct pd_prot *prot, u8 msg_type,
					void *data, int len);
};

static inline int pd_prot_send_phy_packet(struct pd_prot *pd, void *buf,
						int len)
{
	if (pd->phy->send_packet)
		return pd->phy->send_packet(pd->phy, buf, len);

	return -ENOTSUPP;
}

static inline int pd_prot_recv_phy_packet(struct pd_prot *pd, void *buf)
{
	if (pd->phy->recv_packet)
		return pd->phy->recv_packet(pd->phy, buf);

	return -ENOTSUPP;
}

static inline int pd_prot_reset_phy(struct pd_prot *pd)
{
	if (pd->phy->phy_reset)
		return pd->phy->phy_reset(pd->phy);

	return -ENOTSUPP;
}

static inline int pd_prot_flush_fifo(struct pd_prot *pd, int type)
{
	if (pd->phy->flush_fifo)
		return pd->phy->flush_fifo(pd->phy, type);

	return -ENOTSUPP;
}

static inline int pd_prot_setup_role(struct pd_prot *pd,
			int data_role, int power_role)
{
	if (pd->phy->setup_role)
		return pd->phy->setup_role(pd->phy,
				data_role, power_role);

	return -ENOTSUPP;
}

#if defined(CONFIG_USBC_PD) && defined(CONFIG_USBC_PD_POLICY)
extern int protocol_bind_dpm(struct typec_phy *phy);
extern void protocol_unbind_dpm(struct typec_phy *phy);
#else /* CONFIG_USBC_PD && CONFIG_USBC_PD_POLICY */
static inline int protocol_bind_dpm(struct typec_phy *phy)
{
	return 0;
}
static inline void protocol_unbind_dpm(struct typec_phy *phy)
{ }
#endif /* CONFIG_USBC_PD && CONFIG_USBC_PD_POLICY */

extern int pd_ctrl_msg(struct pd_prot *pd, u8 msg_type, u8 msg_id);
extern int pd_data_msg(struct pd_prot *pd, int len, u8 msg_type);
#endif /* __USB_PD_PROT_H__ */
