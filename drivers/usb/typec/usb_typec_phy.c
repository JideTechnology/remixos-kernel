#include <linux/slab.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/usb_typec_phy.h>

static LIST_HEAD(typec_phy_list);

struct typec_phy *typec_get_phy(int type)
{
	struct typec_phy *phy;
	list_for_each_entry(phy, &typec_phy_list, list) {
		if (phy->type != type)
			continue;
		return phy;
	}
	return ERR_PTR(-ENODEV);
}
EXPORT_SYMBOL_GPL(typec_get_phy);

int typec_get_cc_orientation(void)
{
	struct typec_phy *phy;
	phy = typec_get_phy(USB_TYPE_C);
	return phy->valid_cc;
}
EXPORT_SYMBOL_GPL(typec_get_cc_orientation);

int typec_add_phy(struct typec_phy *phy)
{
	if (phy) {
		spin_lock_init(&phy->irq_lock);
		phy->type = USB_TYPE_C;
		phy->state = TYPEC_STATE_UNKNOWN;
		ATOMIC_INIT_NOTIFIER_HEAD(&phy->notifier);
		ATOMIC_INIT_NOTIFIER_HEAD(&phy->prot_notifier);
		/* TODO: Need to enable later */
		/* pd_init_protocol(phy); */
		list_add_tail(&phy->list, &typec_phy_list);
	}
	return 0;
}

int typec_remove_phy(struct typec_phy *x)
{
	struct typec_phy *phy;
	unsigned long flags;

	if (x) {
		spin_lock_irqsave(&phy->irq_lock, flags);
		list_for_each_entry(phy, &typec_phy_list, list) {
			if (!strcmp(phy->label, x->label)) {
				list_del(&phy->list);
				kfree(phy);
			}
		}
		spin_unlock_irqrestore(&phy->irq_lock, flags);
	}
	return 0;
}
