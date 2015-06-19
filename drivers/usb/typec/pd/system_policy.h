#ifndef __PD_SYSTEM_POLICY_H__
#define __PD_SYSTEM_POLICY_H__

#include <linux/usb_typec_phy.h>

struct device_list {
	struct list_head list;
	struct typec_phy *phy;
};

struct system_policy {
	struct list_head list;
	spinlock_t lock;
};

#if defined(CONFIG_USBC_PD) && defined(CONFIG_USBC_SYSTEM_POLICY)
extern int syspolicy_register_typec_phy(struct typec_phy *x);
extern void syspolicy_unregister_typec_phy(struct typec_phy *x);
#else /* CONFIG_USBC_PD && CONFIG_USBC_SYSTEM_POLICY */
static inline int syspolicy_register_typec_phy(struct typec_phy *x)
{
	return 0;
}
static inline void syspolicy_unregister_typec_phy(struct typec_phy *x)
{}
#endif /* CONFIG_USBC_PD && CONFIG_USBC_SYSTEM_POLICY */

#endif /* __PD_SYSTEM_POLICY_H__ */
