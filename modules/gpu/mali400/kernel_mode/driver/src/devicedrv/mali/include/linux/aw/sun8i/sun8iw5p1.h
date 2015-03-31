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

#ifndef _MALI_SUN8I_W5P1_H_
#define _MALI_SUN8I_W5P1_H_

#define GPU_PBASE           SUNXI_GPU_PBASE
#define IRQ_GPU_GP          SUNXI_IRQ_GPUGP
#define IRQ_GPU_GPMMU       SUNXI_IRQ_GPUGPMMU
#define IRQ_GPU_PP0         SUNXI_IRQ_GPUPP0
#define IRQ_GPU_PPMMU0      SUNXI_IRQ_GPUPPMMU0
#define IRQ_GPU_PP1         SUNXI_IRQ_GPUPP1
#define IRQ_GPU_PPMMU1      SUNXI_IRQ_GPUPPMMU1

static struct aw_vf_table vf_table[] =
{
	{
		.vol      = 0,   /* mV, zero means the power isn't independent, and it's controlled by system */
		.max_freq = 144, /* MHz */
	},
	{
		.vol      = 0,   /* mV, zero means the power isn't independent, and it's controlled by system */
		.max_freq = 240, /* MHz */
	},
	{
		.vol      = 0,   /* mV, zero means the power isn't independent, and it's controlled by system */
		.max_freq = 384, /* MHz */
	},
};

static struct aw_private_data private_data =
{
	.clk_status       = 0,
	.normal_level     = 2,
	.sensor_num       = 0,
	.regulator        = NULL,
	.regulator_id     = "vdd-sys",
	.dvfs_data        =
	{
		.max_level   = 2,
		.dvfs_status = 0,
	},
	.tempctrl_data =
	{
		.temp_ctrl_status = 1,
		.temp_ctrl_flag   = 0,
		.count            = 1,
	},
};

static struct aw_clk_data clk_data[] =
{
	{
		.clk_name            = "pll",
		.clk_id              = PLL_GPU_CLK,
		.clk_handle          = NULL,
	},
	{
		.clk_name            = "mali",
		.clk_id              = GPU_CLK,
		.clk_handle          = NULL,
	},
};

/* This data is for sensor, but the data of gpu may be about 5 degress Centigrade higher */
static struct aw_tl_table tl_table[] =
{
	{
		.temp  = 70,
		.level = 1,
	},
};

#endif