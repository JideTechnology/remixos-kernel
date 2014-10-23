/*
 * bq2419x.h -- BQ2419X driver
 *
 * Interface for mfd/regualtor/battery charging driver for BQ2419X VBUS.
 *
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.

 * Author: Laxman Dewangan <ldewangan@nvidia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA	02110-1301, USA.
 *
 */

#ifndef __LINUX_MFD_BQ2419X_H
#define __LINUX_MFD_BQ2419X_H

struct regmap;

#define BQ2419X_INPUT_SRC_REG   0x00
#define BQ2419X_PWR_ON_REG      0x01
#define BQ2419X_CHRG_CTRL_REG   0x02
#define BQ2419X_CHRG_TERM_REG   0x03
#define BQ2419X_VOLT_CTRL_REG   0x04
#define BQ2419X_TIME_CTRL_REG   0x05
#define BQ2419X_THERM_REG       0x06
#define BQ2419X_MISC_OPER_REG   0x07
#define BQ2419X_SYS_STAT_REG    0x08
#define BQ2419X_FAULT_REG       0x09
#define BQ2419X_REVISION_REG    0x0a

#define BQ24190_IC_VER          0x40
#define BQ24192_IC_VER          0x28
#define BQ24192i_IC_VER         0x18
#define ENABLE_CHARGE_MASK      0x30
#define ENABLE_CHARGE           0x10
#define DISABLE_CHARGE          0x00

#define BQ2419X_REG0			0x0
#define BQ2419X_EN_HIZ			BIT(7)

#define BQ2419X_OTG			0x1
#define BQ2419X_OTG_ENABLE_MASK		0x30
#define BQ2419X_OTG_ENABLE		0x20

#define BQ2419X_WD			0x5
#define BQ2419X_WD_MASK			0x30
#define BQ2419X_WD_DISABLE		0x0

#define BQ2419x_VBUS_STAT		0xc0
#define BQ2419x_VBUS_UNKNOWN		0x00
#define BQ2419x_VBUS_USB		0x40
#define BQ2419x_VBUS_AC			0x80

#define BQ2419x_CONFIG_MASK		0x7
#define BQ2419X_MAX_REGS		(BQ2419X_REVISION_REG + 1)

enum bq2419_charging_states {
	bq2419_idle,
	bq2419_progress,
	bq2419_completed=4,
	bq2419_stopped,
};

enum bq2419_charger_type {
	BQ2419_NONE,
	BQ2419_AC,
	BQ2419_USB,
};



/* bq2419x chip information */
struct bq2419x_chip {
	struct device *dev;
	struct regmap *regmap;
};

/*
 * struct bq2419x_regulator_platform_data - bq2419x regulator platform data.
 *
 * @reg_init_data: The regulator init data.
 * @gpio_otg_iusb: Gpio number for OTG/IUSB
 * @power_off_on_suspend: shutdown upon suspend
 */
struct bq2419x_regulator_platform_data {
	struct regulator_init_data *reg_init_data;
	int gpio_otg_iusb;
	bool power_off_on_suspend;
};

struct bq2419x_charger_platform_data {
	int gpio_interrupt;
	int gpio_status;
	unsigned use_mains:1;
	unsigned use_usb:1;
	void (*update_status)(int, int);
	int (*battery_check)(void);

	int regulator_id;
	int max_charge_volt_mV;
	int max_charge_current_mA;
	int charging_term_current_mA;
	int num_consumer_supplies;
	struct regulator_consumer_supply *consumer_supplies;
};
struct bq2419x_platform_data {
	struct bq2419x_regulator_platform_data *reg_pdata;
	struct bq2419x_charger_platform_data *bcharger_pdata;
	bool disable_watchdog;
};

#endif /* __LINUX_MFD_BQ2419X_H */
