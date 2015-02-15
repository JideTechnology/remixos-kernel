/*************************************************************************/ /*!
@File           platform.h
@Title          Platform Header
@Copyright      Copyright (c) Allwinnertech Co., Ltd. All Rights Reserved
@Description    Define struct type about GPU for Allwinnertech platform
@License        GPLv2

The contents of this file is used under the terms of the GNU General Public 
License Version 2 ("GPL") in which case the provisions of GPL are applicable
instead of those above.
*/ /**************************************************************************/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <linux/mali/mali_utgard.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/stat.h>
#include <linux/workqueue.h>

#if !defined(CONFIG_MALI_DT)
#include <linux/clk/sunxi_name.h>
#include <mach/irqs.h>
#include <mach/sys_config.h>
#include <mach/platform.h>
#else
#include <linux/of_device.h>
#endif

#include "mali_kernel_common.h"

struct aw_clk_data
{
	const char   *clk_name;    /* Clock name */
	const char   *clk_id;      /* Clock ID, which is in the system configuration head file */
	
	const char   *clk_parent_id;
	
	struct clk   *clk_handle;  /* Clock handle, we can set the frequency value via it */
	
	/* If the value is 1, the reset API while handle it to reset corresponding module */   
	int          need_reset;
	
	/* The meaning of the values are as follows:
	*  0: don't need to set frequency.
	*  1: use the public frequency
	*  >1: use the private frequency, the value is for frequency.
	*/
	unsigned int expected_freq;
};

struct aw_vf_table
{
	unsigned int vol;
	unsigned int max_freq;
};

struct reg_data
{
	unsigned int bit;
	void *addr;
};

struct aw_status
{
	bool clk_enable;
	bool dvfs_enable;
};

struct aw_private_data
{
	char *regulator_id;
	struct regulator *regulator;
	int max_level;
	int dvfs_flag;
	int sensor_num;
	struct mutex dvfs_lock;
	struct reg_data poweroff_gate;
	struct aw_status status;
};

struct aw_thermal_ctrl_data
{
	unsigned int temp;
	int          max_level;
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