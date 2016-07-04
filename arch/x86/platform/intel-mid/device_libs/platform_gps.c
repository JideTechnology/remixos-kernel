/*
 * platform_gps.c: gps platform data initialization file
 *
 * (C) Copyright 2013 Intel Corporation
 * Author:
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <linux/gpio.h>
#include <linux/intel_mid_gps.h>
#include <linux/sfi.h>
#include <asm/intel-mid.h>
#include "platform_gps.h"

static struct intel_mid_gps_platform_data gps_data = {
	.gpio_reset  = -EINVAL,
	.gpio_enable = 365,//GPIO_SUS2 D15
	.gpio_hostwake = -EINVAL,
	.reset  = RESET_ON,
	.enable = ENABLE_OFF,
	.hsu_port = -EINVAL,
};

static struct platform_device intel_mid_gps_device = {
	.name			= "intel_mid_gps",
	.id			= -1,
	.dev			= {
		.platform_data	= &gps_data,
	},
};

int  __init intel_mid_gps_device_init(void)
{
	int ret = 0;

	gps_data.hsu_port = 1;

	ret = platform_device_register(&intel_mid_gps_device);

	if (ret < 0)
		pr_err("platform_device_register failed for intel_mid_gps\n");

	return ret;
}

device_initcall(intel_mid_gps_device_init);
