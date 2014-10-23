/*
 * arch/arm/mach-tegra/board-curacao-sensors.c
 *
 * Copyright (c) 2011-2013, NVIDIA CORPORATION, All rights reserved.
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
#include <linux/delay.h>
#include <mach/gpio.h>
#include <media/ov5650.h>
#include <media/ov14810.h>
#include <media/ov2710.h>
#include "gpio-names.h"
#include "board.h"
#include <mach/gpio.h>

#include "gpio-names.h"
#include "board-curacao.h"
#include "cpu-tegra.h"

static struct board_info board_info;

static int curacao_camera_init(void)
{
	return 0;
}

static int curacao_left_ov5650_power_on(struct device *dev)
{
	return 0;
}

static int curacao_left_ov5650_power_off(struct device *dev)
{
	return 0;
}

struct ov5650_platform_data curacao_ov5650_data = {
	.power_on = curacao_left_ov5650_power_on,
	.power_off = curacao_left_ov5650_power_off,
};

#ifdef CONFIG_VIDEO_OV14810
static int curacao_ov14810_power_on(struct device *dev)
{
	return 0;
}

static int curacao_ov14810_power_off(struct device *dev)
{
	return 0;
}

struct ov14810_platform_data curacao_ov14810_data = {
	.power_on = curacao_ov14810_power_on,
	.power_off = curacao_ov14810_power_off,
};

struct ov14810_platform_data curacao_ov14810uC_data = {
	.power_on = NULL,
	.power_off = NULL,
};

struct ov14810_platform_data curacao_ov14810SlaveDev_data = {
	.power_on = NULL,
	.power_off = NULL,
};

static struct i2c_board_info curacao_i2c_board_info_e1214[] = {
	{
		I2C_BOARD_INFO("ov14810", 0x36),
		.platform_data = &curacao_ov14810_data,
	},
	{
		I2C_BOARD_INFO("ov14810uC", 0x67),
		.platform_data = &curacao_ov14810uC_data,
	},
	{
		I2C_BOARD_INFO("ov14810SlaveDev", 0x69),
		.platform_data = &curacao_ov14810SlaveDev_data,
	}
};
#endif

static int curacao_right_ov5650_power_on(struct device *dev)
{
	return 0;
}

static int curacao_right_ov5650_power_off(struct device *dev)
{
	return 0;
}

static void curacao_ov5650_synchronize_sensors(void)
{
	pr_err("%s: UnSupported BoardId\n", __func__);
}

struct ov5650_platform_data curacao_right_ov5650_data = {
	.power_on = curacao_right_ov5650_power_on,
	.power_off = curacao_right_ov5650_power_off,
	.synchronize_sensors = curacao_ov5650_synchronize_sensors,
};

static int curacao_ov2710_power_on(struct device *dev)
{
	return 0;
}

static int curacao_ov2710_power_off(struct device *dev)
{
	return 0;
}

struct ov2710_platform_data curacao_ov2710_data = {
	.power_on = curacao_ov2710_power_on,
	.power_off = curacao_ov2710_power_off,
};

static struct i2c_board_info curacao_i2c2_board_info[] = {
	{
		I2C_BOARD_INFO("ov5650R", 0x36),
		.platform_data = &curacao_ov5650_data,
	},
};

int __init curacao_sensors_init(void)
{
	tegra_get_board_info(&board_info);

	curacao_camera_init();

	i2c_register_board_info(3, curacao_i2c2_board_info,
		ARRAY_SIZE(curacao_i2c2_board_info));

#ifdef CONFIG_VIDEO_OV14810
	i2c_register_board_info(3, curacao_i2c_board_info_e1214,
		ARRAY_SIZE(curacao_i2c_board_info_e1214));
#endif

	return 0;
}
