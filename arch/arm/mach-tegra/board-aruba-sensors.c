/*
 * arch/arm/mach-tegra/board-aruba-sensors.c
 *
 * Copyright (c) 2010-2013, NVIDIA CORPORATION, All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/i2c.h>
#include <mach/gpio.h>

#include "gpio-names.h"

#if 0 	// !!!FIXME!!! IMPLEMENT ME

#define ISL29018_IRQ_GPIO	TEGRA_GPIO_PZ2
#define AKM8975_IRQ_GPIO	TEGRA_GPIO_PN5

static void aruba_isl29018_init(void)
{
	gpio_request(ISL29018_IRQ_GPIO, "isl29018");
	gpio_direction_input(ISL29018_IRQ_GPIO);
}

static void aruba_akm8975_init(void)
{
	gpio_request(AKM8975_IRQ_GPIO, "akm8975");
	gpio_direction_input(AKM8975_IRQ_GPIO);
}

struct nct1008_platform_data aruba_nct1008_pdata = {
	.conv_rate = 5,
	.config = NCT1008_CONFIG_ALERT_DISABLE,
	.thermal_threshold = 110,
};

static const struct i2c_board_info aruba_i2c0_board_info[] = {
	{
		I2C_BOARD_INFO("isl29018", 0x44),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PZ2),
	},
};

static const struct i2c_board_info aruba_i2c2_board_info[] = {
	{
		I2C_BOARD_INFO("bq20z75-battery", 0x0B),
	},
};

static struct i2c_board_info aruba_i2c4_board_info[] = {
	{
		I2C_BOARD_INFO("nct1008", 0x4C),
		.platform_data = &aruba_nct1008_pdata,
	},
	{
		I2C_BOARD_INFO("akm8975", 0x0C),
		.irq = TEGRA_GPIO_TO_IRQ(AKM8975_IRQ_GPIO),
	}
};

int __init aruba_sensors_init(void)
{
	aruba_isl29018_init();
	aruba_akm8975_init();

	i2c_register_board_info(0, aruba_i2c0_board_info,
		ARRAY_SIZE(aruba_i2c0_board_info));

	i2c_register_board_info(2, aruba_i2c2_board_info,
		ARRAY_SIZE(aruba_i2c2_board_info));

	i2c_register_board_info(4, aruba_i2c4_board_info,
		ARRAY_SIZE(aruba_i2c4_board_info));

	return 0;
}
#else
int __init aruba_sensors_init(void)
{
	return 0;
}
#endif
