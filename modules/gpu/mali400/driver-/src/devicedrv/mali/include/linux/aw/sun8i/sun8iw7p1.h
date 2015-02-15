/*************************************************************************/ /*!
@File           sun8iw7p1.h
@Title          SUN8IW7P1 Header
@Copyright      Copyright (c) Allwinnertech Co., Ltd. All Rights Reserved
@Description    Define Detailed data about GPU for Allwinnertech platforms
@License        GPLv2

The contents of this file is used under the terms of the GNU General Public 
License Version 2 ("GPL") in which case the provisions of GPL are applicable
instead of those above.
*/ /**************************************************************************/

#ifndef _MALI_SUN8I_W7P1_H_
#define _MALI_SUN8I_W7P1_H_

#define GPU_PBASE           SUNXI_GPU_PBASE
#define IRQ_GPU_GP          SUNXI_IRQ_GPU_GP
#define IRQ_GPU_GPMMU       SUNXI_IRQ_GPU_GPMMU
#define IRQ_GPU_PP0         SUNXI_IRQ_GPU_PP0
#define IRQ_GPU_PPMMU0      SUNXI_IRQ_GPU_PPMMU0
#define IRQ_GPU_PP1         SUNXI_IRQ_GPU_PP1
#define IRQ_GPU_PPMMU1      SUNXI_IRQ_GPU_PPMMU1

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
		.max_freq = 312, /* MHz */
	},
	{
		.vol      = 0,   /* mV, zero means the power isn't independent, and it's controlled by system */
		.max_freq = 432, /* MHz */
	},
	{
		.vol      = 0,   /* mV, zero means the power isn't independent, and it's controlled by system */
		.max_freq = 576, /* MHz */
	},
};

static struct aw_private_data private_data =
{
	.clk_enable_status = 0,
	.dvfs_status       = 1,
	.dvfs_flag         = 0,
	.sensor_num        = 0,
};

static struct aw_clk_data clk_data[] =
{
	{
		.clk_name            = "pll",
		.clk_id              = PLL_GPU_CLK,
		.clk_parent_id       = NULL,
		.clk_handle          = NULL,
		.expected_freq       = 1,
	},
	{
		.clk_name            = "mali",
		.clk_id              = GPU_CLK,
		.clk_parent_id       = NULL,
		.clk_handle          = NULL,
		.expected_freq       = 1,
	},
};

/* This data is for sensor, but the data of gpu may be about 5 degress Centigrade higher */
static struct aw_thermal_ctrl_data thermal_ctrl_data[] =
{
	{
		.temp      = 70,
		.max_level = 3,
	},
	{
		.temp      = 80,
		.max_level = 2,
	},
	{
		.temp      = 90,
		.max_level = 0,
	},
};

#endif