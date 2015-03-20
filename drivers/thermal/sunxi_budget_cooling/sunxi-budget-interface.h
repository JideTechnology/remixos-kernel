#ifndef SUNXI_BUDGET_INTERFACE_H
#define SUNXI_BUDGET_INTERFACE_H

int sunxi_cpufreq_update_state(struct sunxi_budget_cooling_device *cooling_device, u32 cluster);
struct sunxi_budget_cpufreq *
sunxi_cpufreq_cooling_register(struct sunxi_budget_cooling_device *bcd);
void sunxi_cpufreq_cooling_unregister(struct sunxi_budget_cooling_device *bcd);

#ifdef CONFIG_BUDGET_HOTPLUG_CPU
extern int sunxi_hotplug_update_state(struct sunxi_budget_cooling_device *cooling_device, u32 cluster);
extern struct sunxi_budget_hotplug *
sunxi_hotplug_cooling_register(struct sunxi_budget_cooling_device *bcd);
extern void sunxi_hotplug_cooling_unregister(struct sunxi_budget_cooling_device *bcd);
#else
static inline int sunxi_hotplug_update_state(struct sunxi_budget_cooling_device *cooling_device, u32 cluster){return 0;}
static inline struct sunxi_budget_hotplug *
sunxi_hotplug_cooling_register(struct sunxi_budget_cooling_device *bcd){return NULL;}
static inline void sunxi_hotplug_cooling_unregister(struct sunxi_budget_cooling_device *bcd){}
#endif /* CONFIG_HOTPLUG_CPU */

#endif
