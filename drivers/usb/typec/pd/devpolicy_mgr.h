#ifndef __PD_DEVMGR_POLICY_H__
#define __PD_DEVMGR_POLICY_H__

#include <linux/extcon.h>
#include <linux/usb_typec_phy.h>

#define CABLE_CONSUMER	"USB_TYPEC_UFP"
#define CABLE_PROVIDER	"USB_TYPEC_DFP"

/* Assume soc is greater than 80% battery is good */
#define IS_BATT_SOC_GOOD(x)	((x > 80) ? 1 : 0)

enum cable_state {
	CABLE_DETACHED,
	CABLE_ATTACHED,
};

enum cable_type {
	CABLE_TYPE_UNKNOWN,
	CABLE_TYPE_CONSUMER,
	CABLE_TYPE_PROVIDER,
	CABLE_TYPE_DP_SOURCE,
	CABLE_TYPE_DP_SINK,
};

enum charger_mode {
	CHARGER_MODE_UNKNOWN,
	CHARGER_MODE_SET_HZ,
	CHARGER_MODE_ENABLE,
};

enum psy_type {
	PSY_TYPE_UNKNOWN,
	PSY_TYPE_BATTERY,
	PSY_TYPE_CHARGER,
};

#define IS_BATTERY(psy) (psy->type == POWER_SUPPLY_TYPE_BATTERY)
#define IS_CHARGER(psy) (psy->type == POWER_SUPPLY_TYPE_USB ||\
			psy->type == POWER_SUPPLY_TYPE_USB_CDP || \
			psy->type == POWER_SUPPLY_TYPE_USB_DCP || \
			psy->type == POWER_SUPPLY_TYPE_USB_ACA || \
			psy->type == POWER_SUPPLY_TYPE_USB_TYPEC)

/* host mode: max of 5V, 1A */
#define VBUS_5V		5000
#define IBUS_1A		1000
#define IBUS_0P5A	500

/* device mode: max of 12, 3A */
#define VIN_12V		12000
#define VIN_9V		9000
#define VIN_5V		5000

#define ICHRG_3A	3000
#define ICHRG_2A	2000
#define ICHRG_1P5A	1500
#define ICHRG_1A	1000
#define ICHRG_P5A	500

enum devpolicy_mgr_events {
	DEVMGR_EVENT_NONE,
	DEVMGR_EVENT_VBUS_CONNECTED,
	DEVMGR_EVENT_VBUS_DISCONNECTED,
	DEVMGR_EVENT_DFP_CONNECTED,
	DEVMGR_EVENT_DFP_DISCONNECTED,
	DEVMGR_EVENT_UFP_CONNECTED,
	DEVMGR_EVENT_UFP_DISCONNECTED,
};

enum policy_type {
	POLICY_TYPE_UNDEFINED,
	POLICY_TYPE_SOURCE,
	POLICY_TYPE_SINK,
	POLICY_TYPE_DISPLAY,
};

enum pwr_role {
	POWER_ROLE_NONE,
	POWER_ROLE_SINK,
	POWER_ROLE_SOURCE,
};

enum data_role {
	DATA_ROLE_NONE,
	DATA_ROLE_UFP,
	DATA_ROLE_DFP,
};

struct power_cap {
	int mv;
	int ma;
};

struct power_caps {
	struct power_cap *pcap;
	int n_cap;
};

struct cable_event {
	struct list_head node;
	bool is_consumer_state;
	bool is_provider_state;
};

struct pd_policy {
	enum policy_type *policies;
	size_t num_policies;
};

struct devpolicy_mgr;

struct dpm_interface {
	int (*get_max_srcpwr_cap)(struct devpolicy_mgr *dpm,
					struct power_cap *cap);
	int (*get_max_snkpwr_cap)(struct devpolicy_mgr *dpm,
					struct power_cap *cap);
	int (*get_sink_power_cap)(struct devpolicy_mgr *dpm,
					struct power_cap *cap);
	int (*get_sink_power_caps)(struct devpolicy_mgr *dpm,
					struct power_caps *caps);

	/* methods to get/set the sink/source port states */
	int (*set_provider_state)(struct devpolicy_mgr *dpm,
					enum cable_state state);
	int (*set_consumer_state)(struct devpolicy_mgr *dpm,
					enum cable_state state);
	enum cable_state (*get_provider_state)(struct devpolicy_mgr *dpm);
	enum cable_state (*get_consumer_state)(struct devpolicy_mgr *dpm);

	/* methods to get/set data/power roles */
	enum data_role (*get_data_role)(struct devpolicy_mgr *dpm);
	enum pwr_role (*get_pwr_role)(struct devpolicy_mgr *dpm);
	void (*set_data_role)(struct devpolicy_mgr *dpm, enum data_role role);
	void (*set_pwr_role)(struct devpolicy_mgr *dpm, enum pwr_role role);
	int (*set_charger_mode)(struct devpolicy_mgr *dpm,
					enum charger_mode mode);
	int (*update_current_lim)(struct devpolicy_mgr *dpm,
					int ilim);
	int (*get_min_current)(struct devpolicy_mgr *dpm,
					int *ma);
	int (*set_display_port_state)(struct devpolicy_mgr *dpm,
					enum cable_state state,
					enum typec_dp_cable_type type);
};

struct devpolicy_mgr {
	struct pd_policy *policy;
	struct extcon_specific_cable_nb provider_cable_nb;
	struct extcon_specific_cable_nb consumer_cable_nb;
	struct typec_phy *phy;
	struct notifier_block nb;
	struct list_head cable_event_queue;
	struct work_struct cable_event_work;
	struct mutex cable_event_lock;
	struct mutex role_lock;
	struct mutex charger_lock;
	struct dpm_interface *interface;
	spinlock_t cable_event_queue_lock;
	enum cable_state consumer_state;    /* cosumer cable state */
	enum cable_state provider_state;    /* provider cable state */
	enum cable_state dp_state;    /* display cable state */
	enum cable_type prev_cable_evt;
	enum pwr_role prole;
	enum data_role drole;
};

static inline int devpolicy_get_max_srcpwr_cap(struct devpolicy_mgr *dpm,
					struct power_cap *caps)
{
	if (dpm && dpm->interface && dpm->interface->get_max_srcpwr_cap)
		return dpm->interface->get_max_srcpwr_cap(dpm, caps);
	else
		return -ENODEV;
}

static inline int devpolicy_get_max_snkpwr_cap(struct devpolicy_mgr *dpm,
					struct power_cap *caps)
{
	if (dpm && dpm->interface && dpm->interface->get_max_snkpwr_cap)
		return dpm->interface->get_max_snkpwr_cap(dpm, caps);
	else
		return -ENODEV;
}

static inline int devpolicy_get_snkpwr_cap(struct devpolicy_mgr *dpm,
					struct power_cap *cap)
{
	if (dpm && dpm->interface && dpm->interface->get_sink_power_cap)
		return dpm->interface->get_sink_power_cap(dpm, cap);
	else
		return -ENODEV;
}

static inline int devpolicy_get_snkpwr_caps(struct devpolicy_mgr *dpm,
					struct power_caps *caps)
{
	if (dpm && dpm->interface && dpm->interface->get_sink_power_caps)
		return dpm->interface->get_sink_power_caps(dpm, caps);
	else
		return -ENODEV;
}

static inline enum data_role devpolicy_get_data_role(struct devpolicy_mgr *dpm)
{
	if (dpm && dpm->interface && dpm->interface->get_data_role)
		return dpm->interface->get_data_role(dpm);
	else
		return -ENODEV;
}

static inline enum pwr_role devpolicy_get_power_role(struct devpolicy_mgr *dpm)
{
	if (dpm && dpm->interface && dpm->interface->get_pwr_role)
		return dpm->interface->get_pwr_role(dpm);
	else
		return -ENODEV;
}

static inline void devpolicy_set_data_role(struct devpolicy_mgr *dpm,
					enum data_role role)
{
	if (dpm && dpm->interface && dpm->interface->set_data_role)
		dpm->interface->set_data_role(dpm, role);
}

static inline void devpolicy_set_power_role(struct devpolicy_mgr *dpm,
					enum pwr_role role)
{
	if (dpm && dpm->interface && dpm->interface->set_pwr_role)
		dpm->interface->set_pwr_role(dpm, role);
}

static inline int devpolicy_set_charger_mode(struct devpolicy_mgr *dpm,
					enum charger_mode mode)
{
	if (dpm && dpm->interface && dpm->interface->set_charger_mode)
		return dpm->interface->set_charger_mode(dpm, mode);
	else
		return -ENODEV;
}

static inline int devpolicy_update_current_limit(struct devpolicy_mgr *dpm,
							int ilim)
{
	if (dpm && dpm->interface && dpm->interface->update_current_lim)
		return dpm->interface->update_current_lim(dpm, ilim);
	else
		return -ENODEV;
}

static inline int devpolicy_get_min_snk_current(struct devpolicy_mgr *dpm,
							int *ma)
{
	if (dpm && dpm->interface && dpm->interface->get_min_current)
		return dpm->interface->get_min_current(dpm, ma);
	else
		return -ENODEV;
}

static inline int devpolicy_set_provider_state(struct devpolicy_mgr *dpm,
						enum cable_state state)
{
	if (dpm && dpm->interface && dpm->interface->set_provider_state)
		return dpm->interface->set_provider_state(dpm, state);
	else
		return -ENODEV;
}

static inline int devpolicy_set_consumer_state(struct devpolicy_mgr *dpm,
					enum cable_state state)
{
	if (dpm && dpm->interface && dpm->interface->set_consumer_state)
		return dpm->interface->set_consumer_state(dpm, state);
	else
		return -ENODEV;
}

static inline enum cable_state devpolicy_get_provider_state(
					struct devpolicy_mgr *dpm)
{
	if (dpm && dpm->interface && dpm->interface->get_provider_state)
		return dpm->interface->get_provider_state(dpm);
	else
		return -ENODEV;
}

static inline enum cable_state devpolicy_get_consumer_state(
					struct devpolicy_mgr *dpm)
{
	if (dpm && dpm->interface && dpm->interface->get_consumer_state)
		return dpm->interface->get_consumer_state(dpm);
	else
		return -ENODEV;
}

void typec_notify_cable_state(struct typec_phy *phy, char *type, bool state);

/* methods to register/unregister device manager policy notifier */
extern int devpolicy_mgr_reg_notifier(struct notifier_block *nb);
extern void devpolicy_mgr_unreg_notifier(struct notifier_block *nb);

#if defined(CONFIG_USBC_PD) && defined(CONFIG_USBC_PD_POLICY)
extern struct devpolicy_mgr *dpm_register_syspolicy(struct typec_phy *phy,
				struct pd_policy *policy);
extern void dpm_unregister_syspolicy(struct devpolicy_mgr *dpm);
#else /* CONFIG_USBC_PD && CONFIG_USBC_PD_POLICY */
static inline
struct devpolicy_mgr *dpm_register_syspolicy(struct typec_phy *phy,
				struct pd_policy *policy)
{
	return ERR_PTR(-ENOTSUPP);
}
static inline void dpm_unregister_syspolicy(struct devpolicy_mgr *dpm)
{ }
#endif /* CONFIG_USBC_PD && CONFIG_USBC_PD_POLICY */

#endif /* __PD_DEVMGR_POLICY_H__ */
