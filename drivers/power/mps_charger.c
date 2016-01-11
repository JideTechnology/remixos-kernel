/*
 * MPS2617 Charger driver
 *
 * Copyright (C) 2013 Intel Corporation
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author: Xiang, Liu <xiang.liu@intel.com>
 */


#include <linux/device.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>

struct mps_charger_platform_data {
        const char *name;
        enum power_supply_type type;

        char **supplied_to;
        size_t num_supplicants;
};



static char *mps_chrg_supplied_to[] = {
        "dollar_cove_battery"
};

static struct mps_charger_platform_data mps_charger_pdata = {
        .name = "usb",
        .type = POWER_SUPPLY_TYPE_USB,
        .supplied_to = mps_chrg_supplied_to,
        .num_supplicants = ARRAY_SIZE(mps_chrg_supplied_to),
};


struct mps_charger {
	const struct mps_charger_platform_data *pdata;
	bool wakeup_enabled;

	struct power_supply charger;
        int vbus_state;
        int chrg_health;
        int chrg_status;
        int bat_health;
        int cc;
        int cv;
        int inlmt;
        int max_cc;
        int max_cv;
        int iterm;
        int cable_type;
        int cntl_state;
        int cntl_state_max;
        int max_temp;
        int min_temp;
        bool online;
        bool present;
        bool is_charging_enabled;
        bool is_charger_enabled;
        bool is_hw_chrg_term;
};

static inline struct mps_charger *psy_to_mps_charger(struct power_supply *psy)
{
	return container_of(psy, struct mps_charger, charger);
}


static int mps_charger_set_property(struct power_supply *psy,
                                    enum power_supply_property psp,
                                    const union power_supply_propval *val)
{
	struct mps_charger *mps_charger = psy_to_mps_charger(psy);
	const struct mps_charger_platform_data *pdata = mps_charger->pdata;
	pr_err("SSSSS-%s: psp=%d, val_to_set=%d\n", __func__, (int)psp, val->intval);
        switch (psp) {
        case POWER_SUPPLY_PROP_PRESENT:
		mps_charger->present = val->intval;
		if(!mps_charger->present) {
			mps_charger->online = val->intval;
			mps_charger->chrg_health = 0;
		}
                break;
        case POWER_SUPPLY_PROP_ONLINE:
                mps_charger->online = val->intval;
                break;
	case POWER_SUPPLY_PROP_ENABLE_CHARGER:
		mps_charger->is_charger_enabled = val->intval;
		mps_charger->is_charging_enabled = val->intval;
		if (val->intval) {
			mps_charger->online = val->intval;
			mps_charger->chrg_health = 1;
		}
		break;
	case POWER_SUPPLY_PROP_ENABLE_CHARGING:
		if (mps_charger->is_charger_enabled && mps_charger->chrg_health && !(val->intval))
			return 0;
		mps_charger->is_charging_enabled = val->intval;
		if (val->intval) {
			mps_charger->online = val->intval;
			mps_charger->chrg_health = val->intval;
		}
		break;	
	case POWER_SUPPLY_PROP_CABLE_TYPE:
		mps_charger->cable_type = val->intval;
		break;
	case POWER_SUPPLY_PROP_INLMT:
		mps_charger->inlmt = val->intval;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mps_charger_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	struct mps_charger *mps_charger = psy_to_mps_charger(psy);
	const struct mps_charger_platform_data *pdata = mps_charger->pdata;
	
	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
                val->intval = mps_charger->present;
                break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = mps_charger->online;
		break;
	case POWER_SUPPLY_PROP_MAX_CHARGE_CURRENT:
                val->intval = mps_charger->max_cc;
                break;
	case POWER_SUPPLY_PROP_MAX_CHARGE_VOLTAGE:
                val->intval = mps_charger->max_cv;
                break;
        case POWER_SUPPLY_PROP_ENABLE_CHARGER:
		val->intval = mps_charger->is_charger_enabled;
                break;
        case POWER_SUPPLY_PROP_ENABLE_CHARGING:
		val->intval = mps_charger->is_charging_enabled;
                break;
	case POWER_SUPPLY_PROP_CABLE_TYPE:
		val->intval = mps_charger->cable_type;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = mps_charger->chrg_health;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		val->intval = 2;
		break;
	case POWER_SUPPLY_PROP_INLMT:
		val->intval = mps_charger->inlmt;
		break;
	default:
		break;
	}
	pr_err("GGGGG-%s: psp=%d, val=%d\n", __func__, (int)psp, val->intval);

	return 0;
}

static enum power_supply_property mps_charger_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int mps_charger_probe(struct platform_device *pdev)
{
	const struct mps_charger_platform_data *pdata = &mps_charger_pdata;
	struct mps_charger *mps_charger;
	struct power_supply *charger;
	int ret;
	pr_err("%s: Enter!!!\n", __func__);

	mps_charger = devm_kzalloc(&pdev->dev, sizeof(*mps_charger),
					GFP_KERNEL);
	if (!mps_charger) {
		dev_err(&pdev->dev, "Failed to alloc driver structure\n");
		return -ENOMEM;
	}

	charger = &mps_charger->charger;

	charger->name = pdata->name ? pdata->name : "mps-charger";
	charger->type = pdata->type;
	charger->properties = mps_charger_properties;
	charger->num_properties = ARRAY_SIZE(mps_charger_properties);
	charger->get_property = mps_charger_get_property;
	charger->set_property = mps_charger_set_property;
	charger->supplied_to = pdata->supplied_to;
	charger->num_supplicants = pdata->num_supplicants;
	charger->supported_cables = POWER_SUPPLY_CHARGER_TYPE_USB;

	mps_charger->pdata = pdata;
	mps_charger->max_cc = 3000;
	mps_charger->max_cv = 4200;
	ret = power_supply_register(&pdev->dev, charger);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register power supply: %d\n",
			ret);
		goto err_free;
	}

	platform_set_drvdata(pdev, mps_charger);

	device_init_wakeup(&pdev->dev, 1);

	return 0;

err_free:
	return ret;
}

static int mps_charger_remove(struct platform_device *pdev)
{
	struct mps_charger *mps_charger = platform_get_drvdata(pdev);

	power_supply_unregister(&mps_charger->charger);

	return 0;
}


static struct platform_driver mps_charger_driver = {
	.probe = mps_charger_probe,
	.remove = mps_charger_remove,
	.driver = {
		.name = "dummy_charger",
		.owner = THIS_MODULE,
	},
};

static int __init mps_chrg_init(void)
{
        return platform_driver_register(&mps_charger_driver);

}
device_initcall(mps_chrg_init);

static void __exit mps_chrg_exit(void)
{
        platform_driver_unregister(&mps_charger_driver);
}
module_exit(mps_chrg_exit);

MODULE_AUTHOR("Liu, Xiang <xiang.liu@intel.com>");
MODULE_DESCRIPTION("A dummy charger driver for MPS2617 charger");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:mps-charger");
