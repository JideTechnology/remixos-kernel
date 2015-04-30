/*
 * arch/arm/mach-tegra/board-macallan-kbc.c
 * Keys configuration for Nvidia t114 macallan platform.
 *
 * Copyright (C) 2013 NVIDIA, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <mach/io.h>
#include <linux/io.h>
#include <mach/iomap.h>
#include <mach/kbc.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/mfd/palmas.h>
#include "wakeups-t11x.h"

#include "tegra-board-id.h"
#include "board.h"
#include "board-macallan.h"
#include "devices.h"

#define GPIO_KEY(_id, _gpio, _iswake)           \
	{                                       \
		.code = _id,                    \
		.gpio = TEGRA_GPIO_##_gpio,     \
		.active_low = 1,                \
		.desc = #_id,                   \
		.type = EV_KEY,                 \
		.wakeup = _iswake,              \
		.debounce_interval = 10,        \
	}

static struct gpio_keys_button macallan_e1545_keys[] = {
	[0] = GPIO_KEY(KEY_POWER, PQ0, 1),
	[1] = GPIO_KEY(KEY_VOLUMEUP, PR2, 0),
	[2] = GPIO_KEY(KEY_VOLUMEDOWN, PR1, 0),
};

#ifdef NV_WAKEUP_KEY
static int macallan_wakeup_key(void)
{
	int wakeup_key;
	u64 status = readl(IO_ADDRESS(TEGRA_PMC_BASE) + PMC_WAKE_STATUS)
		| (u64)readl(IO_ADDRESS(TEGRA_PMC_BASE)
		+ PMC_WAKE2_STATUS) << 32;
	if (status & ((u64)1 << TEGRA_WAKE_GPIO_PQ0))
		wakeup_key = KEY_POWER;
	else if (status & ((u64)1 << TEGRA_WAKE_GPIO_PS0))
		wakeup_key = SW_LID;
	else
		wakeup_key = KEY_RESERVED;

	return wakeup_key;
}
#endif

static struct gpio_keys_platform_data macallan_e1545_keys_pdata = {
	.buttons	= macallan_e1545_keys,
	.nbuttons	= ARRAY_SIZE(macallan_e1545_keys),
#ifdef NV_WAKEUP_KEY
	.wakeup_key	= macallan_wakeup_key,
#endif
};

static struct platform_device macallan_e1545_keys_device = {
	.name	= "gpio-keys",
	.id	= 0,
	.dev	= {
		.platform_data  = &macallan_e1545_keys_pdata,
	},
};
#define SW_KEY(_id, _gpio, _iswake)           \
	{                                       \
		.code = _id,                    \
		.gpio = TEGRA_GPIO_##_gpio,     \
		.active_low = 1,                \
		.desc = #_id,                   \
		.type = EV_SW,                 \
		.wakeup = _iswake,              \
		.debounce_interval = 10,        \
	}
#ifdef CONFIG_KEYBOARD_SW
static struct gpio_keys_button macallan_sw_keys[] = {
	[0] = SW_KEY(SW_LID, PK6, 0),

};
static struct gpio_keys_platform_data macallan_sw_keys_pdata = {
	.buttons	= macallan_sw_keys,
	.nbuttons	= ARRAY_SIZE(macallan_sw_keys),
};
static struct platform_device macallan_sw_keys_device = {
	.name	= "sw-key",
	.id	= 0,
	.dev	= {
		.platform_data  = &macallan_sw_keys_pdata,
	},
};
#endif
int __init macallan_kbc_init(void)
{
	platform_device_register(&macallan_e1545_keys_device);
#ifdef CONFIG_KEYBOARD_SW
	platform_device_register(&macallan_sw_keys_device);
#endif
	return 0;
}

