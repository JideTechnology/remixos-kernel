/*************************************************************************/ /*!
@File           sun50iw1p1.h
@Title          SUN50IW1P1 Header
@Copyright      Copyright (c) Allwinnertech Co., Ltd. All Rights Reserved
@Description    Define Detailed data about GPU for Allwinnertech platforms
@License        GPLv2

The contents of this file is used under the terms of the GNU General Public 
License Version 2 ("GPL") in which case the provisions of GPL are applicable
instead of those above.
*/ /**************************************************************************/

#ifndef _MALI_SUN50I_W1P1_H_
#define _MALI_SUN50I_W1P1_H_

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
};

static struct aw_private_data private_data =
{
	.dvfs_flag         = 0,
	.sensor_num        = 0,
	.status            =
	{
		.clk_enable  = 0,
		.dvfs_enable = 1,
	},
};

static struct aw_clk_data clk_data[] =
{
	{
		.clk_name            = "pll",
		.clk_parent_id       = NULL,
		.clk_handle          = NULL,
		.expected_freq       = 1,
	},
	{
		.clk_name            = "mali",
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