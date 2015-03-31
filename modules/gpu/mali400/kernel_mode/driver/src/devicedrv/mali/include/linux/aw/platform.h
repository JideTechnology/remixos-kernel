/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright (C) 2015 Allwinner Technology Co., Ltd.
 *
 * Author: Xiangyun Yu <yuxyun@allwinnertech.com>
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <linux/dma-mapping.h>
#include <linux/mali/mali_utgard.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/stat.h>
#include <linux/workqueue.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_OF
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#else
#include <linux/clk/sunxi_name.h>
#include <mach/irqs.h>
#include <mach/sys_config.h>
#include <mach/platform.h>
#endif /* CONFIG_OF */
#include "mali_kernel_common.h"

struct aw_clk_data
{
	const char   *clk_name;    /* Clock name */
#ifndef CONFIG_OF
	const char   *clk_id;      /* Clock ID, which is in the system configuration head file */
#endif
	char clk_parent_num; /* The number is the number in clk_data array, -1 is just for pll */

	struct clk   *clk_handle;

	/* The meaning of the values are as follows:
	*  0: don't need to set frequency.
	*  1: use the public frequency
	*  >1: use the private frequency, the value is for frequency.
	*/
	u32 freq;
};

struct aw_vf_table
{
	u32 vol;
	u32 max_freq;
};

/* The struct is for temperature-level table */
struct aw_tl_table
{
	u8 temp;
	u8 level;
};

struct aw_dvfs_data
{
	u8   max_level;
	bool dvfs_status;
	struct mutex lock;
};

struct aw_tempctrl_data
{
	bool temp_ctrl_status;
	char temp_ctrl_flag;
	u8 level;
	u8 count;     /* The data in tl_table to use */
};

struct aw_private_data
{
	bool   clk_status;
	u8     normal_level;
	u8     sensor_num;
#ifdef CONFIG_MALI_DT
	struct device_node *np_gpu;
#endif
	struct regulator *regulator;
	char   *regulator_id;
	struct aw_dvfs_data dvfs_data;
	struct aw_tempctrl_data tempctrl_data;
};

#if defined CONFIG_ARCH_SUN8IW3P1
#include "sun8i/sun8iw3p1.h"
#elif defined CONFIG_ARCH_SUN8IW5P1
#include "sun8i/sun8iw5p1.h"
#elif defined CONFIG_ARCH_SUN8IW7P1
#include "sun8i/sun8iw7p1.h"
#elif defined CONFIG_ARCH_SUN50IW1P1
#include "sun50i/sun50iw1p1.h"
#else
#error "please select a platform\n"
#endif

#endif
