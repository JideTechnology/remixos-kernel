/*
 * arch/arm/mach-tegra/board-keenhi_t40-sensors.c
 *
 * Copyright (c) 2013-2014 NVIDIA CORPORATION, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of NVIDIA CORPORATION nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mpu.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/therm_est.h>
#include <linux/nct1008.h>
#include <linux/cm3217.h>
#include <mach/edp.h>
#include <linux/edp.h>
#include <mach/gpio-tegra.h>
#include <mach/pinmux-t11.h>
#include <mach/pinmux.h>
#include <generated/mach-types.h>
#include <linux/power/sbs-battery.h>
#ifdef CONFIG_VIDEO_IMX091
#include <media/imx091.h>
#endif
#ifdef CONFIG_VIDEO_OV9772
#include <media/ov9772.h>
#endif
#ifdef CONFIG_VIDEO_T8EV5
#include <media/t8ev5.h>
#endif

#ifdef CONFIG_VIDEO_AD5816
#include <media/ad5816.h>
#endif
#ifdef CONFIG_TORCH_AS364X
#include <media/as364x.h>
#endif

#ifdef CONFIG_LIGHTSENSOR_EPL6814
#include <linux/input/elan_interface.h>
#endif

#ifdef CONFIG_GPIO_SWITCH_SERAIL
#include <linux/gpio_switch.h>
#endif

#include "gpio-names.h"
#include "board.h"
#include "board-common.h"
#include "board-keenhi-t40.h"
#include "cpu-tegra.h"
#include "devices.h"
#include "tegra-board-id.h"
#include "dvfs.h"


static struct regulator *macallan_1v8_cam1;
static struct regulator *macallan_2v8_cam1;

static struct throttle_table tj_throttle_table[] = {
	/* CPU_THROT_LOW cannot be used by other than CPU */
	/*    CPU,   C2BUS,   C3BUS,   SCLK,    EMC */
	{ { 1810500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1785000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1759500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1734000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1708500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1683000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1657500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1632000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1606500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1581000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1555500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1530000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1504500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1479000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1453500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1428000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1402500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1377000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1351500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1326000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1300500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1275000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1249500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1224000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1198500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1173000, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1147500, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1122000, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1096500, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1071000, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1045500, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1020000, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  994500, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  969000, 600000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  943500, 600000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  918000, 600000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  892500, 600000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  867000, 600000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  841500, 564000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  816000, 564000, NO_CAP, NO_CAP, 792000 } },
	{ {  790500, 564000, NO_CAP, 372000, 792000 } },
	{ {  765000, 564000, 468000, 372000, 792000 } },
	{ {  739500, 528000, 468000, 372000, 792000 } },
	{ {  714000, 528000, 468000, 336000, 792000 } },
	{ {  688500, 528000, 420000, 336000, 792000 } },
	{ {  663000, 492000, 420000, 336000, 792000 } },
	{ {  637500, 492000, 420000, 336000, 408000 } },
	{ {  612000, 492000, 420000, 300000, 408000 } },
	{ {  586500, 492000, 360000, 336000, 408000 } },
	{ {  561000, 420000, 420000, 300000, 408000 } },
	{ {  535500, 420000, 360000, 228000, 408000 } },
	{ {  510000, 420000, 288000, 228000, 408000 } },
	{ {  484500, 324000, 288000, 228000, 408000 } },
	{ {  459000, 324000, 288000, 228000, 408000 } },
	{ {  433500, 324000, 288000, 228000, 408000 } },
	{ {  408000, 324000, 288000, 228000, 408000 } },
};

static struct balanced_throttle tj_throttle = {
	.throt_tab_size = ARRAY_SIZE(tj_throttle_table),
	.throt_tab = tj_throttle_table,
};

static int __init keenhi_t40_throttle_init(void)
{
		balanced_throttle_register(&tj_throttle, "tegra-balanced");
	return 0;
}
module_init(keenhi_t40_throttle_init);

static struct nct1008_platform_data keenhi_t40_nct1008_pdata = {
	.supported_hwrev = true,
	.ext_range = true,
	.conv_rate = 0x06, /* 4Hz conversion rate */
	.shutdown_ext_limit = 105, /* C */
	.shutdown_local_limit = 120, /* C */

	.num_trips = 1,
	.trips = {
		{
			.cdev_type = "suspend_soctherm",
			.trip_temp = 50000,
			.trip_type = THERMAL_TRIP_ACTIVE,
			.upper = 1,
			.lower = 1,
			.hysteresis = 5000,
		},
	},
};

static struct i2c_board_info keenhi_t40_i2c4_nct1008_board_info[] = {
	{
		I2C_BOARD_INFO("nct1008", 0x4C),
		.platform_data = &keenhi_t40_nct1008_pdata,
		.irq = -1,
	}
};

#define VI_PINMUX(_pingroup, _mux, _pupd, _tri, _io, _lock, _ioreset) \
	{							\
		.pingroup	= TEGRA_PINGROUP_##_pingroup,	\
		.func		= TEGRA_MUX_##_mux,		\
		.pupd		= TEGRA_PUPD_##_pupd,		\
		.tristate	= TEGRA_TRI_##_tri,		\
		.io		= TEGRA_PIN_##_io,		\
		.lock		= TEGRA_PIN_LOCK_##_lock,	\
		.od		= TEGRA_PIN_OD_DEFAULT,		\
		.ioreset	= TEGRA_PIN_IO_RESET_##_ioreset	\
}

static struct tegra_pingroup_config mclk_disable =
	VI_PINMUX(CAM_MCLK, VI, NORMAL, NORMAL, OUTPUT, DEFAULT, DEFAULT);

static struct tegra_pingroup_config mclk_enable =
	VI_PINMUX(CAM_MCLK, VI_ALT3, NORMAL, NORMAL, OUTPUT, DEFAULT, DEFAULT);

static struct tegra_pingroup_config pbb0_disable =
	VI_PINMUX(GPIO_PBB0, VI, NORMAL, NORMAL, OUTPUT, DEFAULT, DEFAULT);

static struct tegra_pingroup_config pbb0_enable =
	VI_PINMUX(GPIO_PBB0, VI_ALT3, NORMAL, NORMAL, OUTPUT, DEFAULT, DEFAULT);

/*
 * As a workaround, keenhi_t40_vcmvdd need to be allocated to activate the
 * sensor devices. This is due to the focuser device(AD5816) will hook up
 * the i2c bus if it is not powered up.
*/
static struct regulator *keenhi_t40_vcmvdd;

static int keenhi_t40_get_vcmvdd(void)
{
	if (!keenhi_t40_vcmvdd) {
		keenhi_t40_vcmvdd = regulator_get(NULL, "vdd_af_cam1");
		if (unlikely(WARN_ON(IS_ERR(keenhi_t40_vcmvdd)))) {
			pr_err("%s: can't get regulator vcmvdd: %ld\n",
				__func__, PTR_ERR(keenhi_t40_vcmvdd));
			keenhi_t40_vcmvdd = NULL;
			return -ENODEV;
		}
	}
	return 0;
}

#ifdef CONFIG_VIDEO_IMX091
static struct nvc_gpio_pdata imx091_gpio_pdata[] = {
	{IMX091_GPIO_RESET, CAM_RSTN, true, false},
	{IMX091_GPIO_PWDN, CAM1_POWER_DWN_GPIO, true, false},
	{IMX091_GPIO_GP1, CAM_GPIO1, true, false}
};

static int keenhi_t40_imx091_power_on(struct nvc_regulator *vreg)
{
	int err;

	if (unlikely(WARN_ON(!vreg)))
		return -EFAULT;

	if (keenhi_t40_get_vcmvdd())
		goto imx091_poweron_fail;

	gpio_set_value(CAM1_POWER_DWN_GPIO, 0);
	usleep_range(10, 20);

	err = regulator_enable(vreg[IMX091_VREG_AVDD].vreg);
	if (err)
		goto imx091_avdd_fail;

	err = regulator_enable(vreg[IMX091_VREG_DVDD].vreg);
	if (err)
		goto imx091_dvdd_fail;

	err = regulator_enable(vreg[IMX091_VREG_IOVDD].vreg);
	if (err)
		goto imx091_iovdd_fail;

	usleep_range(1, 2);
	gpio_set_value(CAM1_POWER_DWN_GPIO, 1);

	err = regulator_enable(keenhi_t40_vcmvdd);
	if (unlikely(err))
		goto imx091_vcmvdd_fail;

	tegra_pinmux_config_table(&mclk_enable, 1);
	usleep_range(300, 310);

	return 1;

imx091_vcmvdd_fail:
	regulator_disable(vreg[IMX091_VREG_IOVDD].vreg);

imx091_iovdd_fail:
	regulator_disable(vreg[IMX091_VREG_DVDD].vreg);

imx091_dvdd_fail:
	regulator_disable(vreg[IMX091_VREG_AVDD].vreg);

imx091_avdd_fail:
	gpio_set_value(CAM1_POWER_DWN_GPIO, 0);

imx091_poweron_fail:
	pr_err("%s FAILED\n", __func__);
	return -ENODEV;
}

static int keenhi_t40_imx091_power_off(struct nvc_regulator *vreg)
{
	if (unlikely(WARN_ON(!vreg)))
		return -EFAULT;

	usleep_range(1, 2);
	tegra_pinmux_config_table(&mclk_disable, 1);
	gpio_set_value(CAM1_POWER_DWN_GPIO, 0);
	usleep_range(1, 2);

	regulator_disable(keenhi_t40_vcmvdd);
	regulator_disable(vreg[IMX091_VREG_IOVDD].vreg);
	regulator_disable(vreg[IMX091_VREG_DVDD].vreg);
	regulator_disable(vreg[IMX091_VREG_AVDD].vreg);

	return 1;
}

static struct nvc_imager_cap imx091_cap = {
	.identifier		= "IMX091",
	.sensor_nvc_interface	= 3,
	.pixel_types[0]		= 0x100,
	.orientation		= 0,
	.direction		= 0,
	.initial_clock_rate_khz	= 6000,
	.clock_profiles[0] = {
		.external_clock_khz	= 24000,
		.clock_multiplier	= 850000, /* value / 1,000,000 */
	},
	.clock_profiles[1] = {
		.external_clock_khz	= 0,
		.clock_multiplier	= 0,
	},
	.h_sync_edge		= 0,
	.v_sync_edge		= 0,
	.mclk_on_vgp0		= 0,
	.csi_port		= 0,
	.data_lanes		= 4,
	.virtual_channel_id	= 0,
	.discontinuous_clk_mode	= 1,
	.cil_threshold_settle	= 0x0,
	.min_blank_time_width	= 16,
	.min_blank_time_height	= 16,
	.preferred_mode_index	= 0,
	.focuser_guid		= NVC_FOCUS_GUID(0),
	.torch_guid		= NVC_TORCH_GUID(0),
	.cap_version		= NVC_IMAGER_CAPABILITIES_VERSION2,
};

static unsigned imx091_estates[] = { 876, 656, 220, 0 };

static struct imx091_platform_data imx091_pdata = {
	.num			= 0,
	.sync			= 0,
	.dev_name		= "camera",
	.gpio_count		= ARRAY_SIZE(imx091_gpio_pdata),
	.gpio			= imx091_gpio_pdata,
	.flash_cap		= {
		.sdo_trigger_enabled = 1,
		.adjustable_flash_timing = 1,
	},
	.cap			= &imx091_cap,
	.edpc_config	= {
		.states = imx091_estates,
		.num_states = ARRAY_SIZE(imx091_estates),
		.e0_index = ARRAY_SIZE(imx091_estates) - 1,
		.priority = EDP_MAX_PRIO + 1,
	},
	.power_on		= keenhi_t40_imx091_power_on,
	.power_off		= keenhi_t40_imx091_power_off,
};
#endif

#ifdef CONFIG_VIDEO_OV9772
static int keenhi_t40_ov9772_power_on(struct ov9772_power_rail *pw)
{
	int err;

	if (unlikely(!pw || !pw->avdd || !pw->dovdd))
		return -EFAULT;

	if (keenhi_t40_get_vcmvdd())
		goto ov9772_get_vcmvdd_fail;

	gpio_set_value(CAM2_POWER_DWN_GPIO, 0);
	gpio_set_value(CAM_RSTN, 0);

	err = regulator_enable(pw->avdd);
	if (unlikely(err))
		goto ov9772_avdd_fail;

	err = regulator_enable(pw->dvdd);
	if (unlikely(err))
		goto ov9772_dvdd_fail;

	err = regulator_enable(pw->dovdd);
	if (unlikely(err))
		goto ov9772_dovdd_fail;

	gpio_set_value(CAM_RSTN, 1);
	gpio_set_value(CAM2_POWER_DWN_GPIO, 1);

	err = regulator_enable(keenhi_t40_vcmvdd);
	if (unlikely(err))
		goto ov9772_vcmvdd_fail;

	tegra_pinmux_config_table(&pbb0_enable, 1);
	usleep_range(340, 380);

	/* return 1 to skip the in-driver power_on sequence */
	return 1;

ov9772_vcmvdd_fail:
	regulator_disable(pw->dovdd);

ov9772_dovdd_fail:
	regulator_disable(pw->dvdd);

ov9772_dvdd_fail:
	regulator_disable(pw->avdd);

ov9772_avdd_fail:
	gpio_set_value(CAM_RSTN, 0);
	gpio_set_value(CAM2_POWER_DWN_GPIO, 0);

ov9772_get_vcmvdd_fail:
	pr_err("%s FAILED\n", __func__);
	return -ENODEV;
}

static int keenhi_t40_ov9772_power_off(struct ov9772_power_rail *pw)
{
	if (unlikely(!pw || !keenhi_t40_vcmvdd || !pw->avdd || !pw->dovdd))
		return -EFAULT;

	usleep_range(21, 25);
	tegra_pinmux_config_table(&pbb0_disable, 1);

	gpio_set_value(CAM2_POWER_DWN_GPIO, 0);
	gpio_set_value(CAM_RSTN, 0);

	regulator_disable(keenhi_t40_vcmvdd);
	regulator_disable(pw->dovdd);
	regulator_disable(pw->dvdd);
	regulator_disable(pw->avdd);

	/* return 1 to skip the in-driver power_off sequence */
	return 1;
}

static struct nvc_gpio_pdata ov9772_gpio_pdata[] = {
	{ OV9772_GPIO_TYPE_SHTDN, CAM2_POWER_DWN_GPIO, true, 0, },
	{ OV9772_GPIO_TYPE_PWRDN, CAM_RSTN, true, 0, },
};

static struct ov9772_platform_data keenhi_t40_ov9772_pdata = {
	.num		= 1,
	.dev_name	= "camera",
	.gpio_count	= ARRAY_SIZE(ov9772_gpio_pdata),
	.gpio		= ov9772_gpio_pdata,
	.power_on	= keenhi_t40_ov9772_power_on,
	.power_off	= keenhi_t40_ov9772_power_off,
};
#endif

#ifdef CONFIG_TORCH_AS364X
static int keenhi_t40_as3648_power_on(struct as364x_power_rail *pw)
{
	int err = keenhi_t40_get_vcmvdd();

	if (err)
		return err;

	return regulator_enable(keenhi_t40_vcmvdd);
}

static int keenhi_t40_as3648_power_off(struct as364x_power_rail *pw)
{
	if (!keenhi_t40_vcmvdd)
		return -ENODEV;

	return regulator_disable(keenhi_t40_vcmvdd);
}

/* estate values under 1000/200/0/0mA, 3.5V input */
static unsigned as364x_estates[] = {3500, 2375, 560, 456, 0};
static struct as364x_platform_data keenhi_t40_as3648_pdata = {
	.config		= {
		.led_mask = 3,
		.max_total_current_mA = 1000,
		.max_peak_current_mA = 600,
		.max_torch_current_mA = 150,
		.vin_low_v_run_mV = 3070,
		.strobe_type = 1,
		},
	.pinstate	= {
		.mask	= 1 << (CAM_FLASH_STROBE - TEGRA_GPIO_PBB0),
		.values	= 1 << (CAM_FLASH_STROBE - TEGRA_GPIO_PBB0)
		},
	.dev_name	= "torch",
	.type		= AS3648,
	.gpio_strobe	= CAM_FLASH_STROBE,
	.edpc_config	= {
		.states		= as364x_estates,
		.num_states	= ARRAY_SIZE(as364x_estates),
		.e0_index	= ARRAY_SIZE(as364x_estates) - 1,
		.priority	= EDP_MAX_PRIO + 2,
		},

	.power_on_callback = keenhi_t40_as3648_power_on,
	.power_off_callback = keenhi_t40_as3648_power_off,
};
#endif

#ifdef CONFIG_VIDEO_AD5816

static struct ad5816_platform_data keenhi_t40_ad5816_pdata = {
	.cfg = 0,
	.num = 0,
	.sync = 0,
	.dev_name = "focuser",
	.power_on = keenhi_t40_focuser_power_on,
	.power_off = keenhi_t40_focuser_power_off,
};
#endif

#if defined(CONFIG_VIDEO_MT9P111)||defined(CONFIG_VIDEO_T8EV5)
#define MT9P111_POWER_RST_PIN TEGRA_GPIO_PBB7
#define MT9P111_POWER_DWN_PIN TEGRA_GPIO_PBB5
static int kai_mt9p111_power_on(void)
{
	int err;
	pr_err("%s:==========>EXE\n",__func__);
	if (keenhi_t40_get_vcmvdd())
		goto mt9p111_poweron_fail;
	err = regulator_enable(keenhi_t40_vcmvdd);
	if (unlikely(err))
		goto mt9p111_poweron_fail;

	if (macallan_1v8_cam1 == NULL) {
		macallan_1v8_cam1 = regulator_get(NULL, "vdd_cam_1v8");//1.8v
		if (WARN_ON(IS_ERR(macallan_1v8_cam1))) {
			pr_err("%s: couldn't get regulator vdd_1v8_cam1 %d\n",	__func__, (int)PTR_ERR(macallan_1v8_cam1));
			goto reg_get_vdd_1v8_cam3_fail;
			}	
	}
	regulator_enable(macallan_1v8_cam1);
	
	if (macallan_2v8_cam1 == NULL) {
		macallan_2v8_cam1 = regulator_get(NULL, "avdd_cam2");//2.8v
		if (WARN_ON(IS_ERR(macallan_2v8_cam1))) {
			pr_err("%s: couldn't get regulator vdd_cam1: %d\n",	__func__, (int)PTR_ERR(macallan_2v8_cam1));
			goto reg_get_vdd_cam3_fail;		
		}	
	}	
	regulator_enable(macallan_2v8_cam1);
	
	msleep(100);
	
	tegra_pinmux_config_table(&mclk_enable, 1);
	msleep(5);
	
	gpio_set_value(MT9P111_POWER_DWN_PIN, 0);
       msleep(5);
       gpio_set_value(MT9P111_POWER_RST_PIN, 1); 
       msleep(10);
	
	return 0;

reg_get_vdd_1v8_cam3_fail:
	regulator_put(macallan_1v8_cam1);
	macallan_1v8_cam1 = NULL;
reg_get_vdd_cam3_fail:
	regulator_put(macallan_2v8_cam1);
	macallan_2v8_cam1 = NULL;
mt9p111_poweron_fail:
	pr_err("%s FAILED\n", __func__);
	return -ENODEV;
}

static int kai_mt9p111_power_off(void)
{
	pr_err("%s:==========>EXE\n",__func__);	

	gpio_set_value(MT9P111_POWER_RST_PIN, 0);       
	msleep(5);
	
	if (keenhi_t40_vcmvdd){
		regulator_disable(keenhi_t40_vcmvdd);
		regulator_put(keenhi_t40_vcmvdd);
		keenhi_t40_vcmvdd =NULL;
	}

	if (macallan_2v8_cam1){
		regulator_disable(macallan_2v8_cam1);
		regulator_put(macallan_2v8_cam1);
		macallan_2v8_cam1 =NULL;
	}	

	tegra_pinmux_config_table(&mclk_disable, 1);
	
	if (macallan_1v8_cam1){
		regulator_disable(macallan_1v8_cam1);
		regulator_put(macallan_1v8_cam1);
		macallan_1v8_cam1 =NULL;
	}
	gpio_set_value(MT9P111_POWER_DWN_PIN, 1);        
	return 0;
}

#endif

#if defined(CONFIG_VIDEO_T8EV5)
#define T8EV5_POWER_RST_PIN TEGRA_GPIO_PBB7
#define T8EV5_POWER_DWN_PIN TEGRA_GPIO_PBB5

#define T8EV5_POWER_FRONT_RST_PIN TEGRA_GPIO_PBB3
#define T8EV5_POWER_FRONT_DWN_PIN TEGRA_GPIO_PBB6
static int kai_t8ev5_power_on(u8 flag)
{
	int err,rst_gpio = T8EV5_POWER_RST_PIN,dwn_gpio = T8EV5_POWER_DWN_PIN;
	pr_err("%s:==========>EXE\n",__func__);
	if(flag&T8EV5_FRONT_ID){
		rst_gpio = T8EV5_POWER_FRONT_RST_PIN;
		dwn_gpio = T8EV5_POWER_FRONT_DWN_PIN;
	}else{
		if (keenhi_t40_get_vcmvdd())
			goto t8ev5_poweron_fail;
	}
	
	if (macallan_2v8_cam1 == NULL) {
		macallan_2v8_cam1 = regulator_get(NULL, "avdd_cam2");//2.8v
		if (WARN_ON(IS_ERR(macallan_2v8_cam1))) {
			pr_err("%s: couldn't get regulator vdd_cam1: %d\n",	__func__, (int)PTR_ERR(macallan_2v8_cam1));
			goto reg_get_vdd_cam3_fail;		
		}	
	}	
	err = regulator_enable(macallan_2v8_cam1);
	if (unlikely(err))
		goto reg_get_vdd_cam3_fail;
	udelay(100);
	if (macallan_1v8_cam1 == NULL) {
		macallan_1v8_cam1 = regulator_get(NULL, "vdd_cam_1v8");//1.8v
		if (WARN_ON(IS_ERR(macallan_1v8_cam1))) {
			pr_err("%s: couldn't get regulator vdd_1v8_cam1 %d\n",	__func__, (int)PTR_ERR(macallan_1v8_cam1));
			goto reg_get_vdd_1v8_cam3_fail;
			}	
	}
	err = regulator_enable(macallan_1v8_cam1);
	if (unlikely(err))
		goto reg_get_vdd_cam3_fail;
	mdelay(1);

	gpio_direction_output(dwn_gpio, 1);
	mdelay(10);
	gpio_direction_output(rst_gpio, 1);
	if(!(flag&T8EV5_FRONT_ID)){
		err = regulator_enable(keenhi_t40_vcmvdd);
		if (unlikely(err))
			goto reg_get_vdd_1v8_cam3_fail;
		tegra_pinmux_config_table(&mclk_enable, 1);
	}else
		tegra_pinmux_config_table(&pbb0_enable, 1);
	usleep_range(200, 220);
	return 0;

reg_get_vdd_1v8_cam3_fail:
	regulator_put(macallan_1v8_cam1);
	macallan_1v8_cam1 = NULL;
reg_get_vdd_cam3_fail:
	regulator_put(macallan_2v8_cam1);
	macallan_2v8_cam1 = NULL;
t8ev5_poweron_fail:
	pr_err("%s FAILED\n", __func__);
	return -ENODEV;
}

static int kai_t8ev5_power_off(u8 flag)
{
	int rst_gpio = T8EV5_POWER_RST_PIN,dwn_gpio = T8EV5_POWER_DWN_PIN;
	pr_err("%s:==========>EXE\n",__func__);	
	if(flag&T8EV5_FRONT_ID){
		rst_gpio = T8EV5_POWER_FRONT_RST_PIN;
		dwn_gpio = T8EV5_POWER_FRONT_DWN_PIN;
	}
	usleep_range(100, 120);
	if(flag&T8EV5_FRONT_ID){
		tegra_pinmux_config_table(&pbb0_disable, 1);
	}else
		tegra_pinmux_config_table(&mclk_disable, 1);
	
	usleep_range(100, 120);
	
	gpio_direction_output(dwn_gpio, 0);//old 1
	gpio_direction_output(rst_gpio, 0);
	
	if (keenhi_t40_vcmvdd)
	regulator_disable(keenhi_t40_vcmvdd);
	
	if (macallan_1v8_cam1){
		regulator_disable(macallan_1v8_cam1);
	}
	if (macallan_2v8_cam1){
		regulator_disable(macallan_2v8_cam1);
	}
	

	return 0;
}

static int kai_sensor_power_on(u8 flag)
{
	int ret = 0;
	if(flag&T8EV5_TYPE_ID)
		ret = kai_t8ev5_power_on(flag&0x01);
	else if(flag&MT9P111_TYPE_ID)
		ret = kai_mt9p111_power_on();
	return ret;
}
static int kai_sensor_power_off(u8 flag)
{
	int ret = 0;
	if(flag&T8EV5_TYPE_ID)
		ret = kai_t8ev5_power_off(flag&0x01);
	else if(flag&MT9P111_TYPE_ID)
		ret = kai_mt9p111_power_off();
	return ret;
}
struct yuv_t8ev5_sensor_platform_data kai_t8ev5_data = {
	.led_en= TEGRA_GPIO_PBB4,
	.flash_en= TEGRA_GPIO_PCC1,
	.flash_delay = 3,
	.power_on = kai_sensor_power_on,
	.power_off = kai_sensor_power_off,
};

#endif

static struct i2c_board_info keenhi_t40_camera_i2c_board_info[] = {
#ifdef CONFIG_VIDEO_IMX091	
	{
		I2C_BOARD_INFO("imx091", 0x36),
		.platform_data = &imx091_pdata,
	},
#endif
#ifdef CONFIG_VIDEO_OV9772
	{
		I2C_BOARD_INFO("ov9772", 0x10),
		.platform_data = &keenhi_t40_ov9772_pdata,
	},
#endif
#ifdef CONFIG_TORCH_AS364X	
	{
		I2C_BOARD_INFO("as3648", 0x30),
		.platform_data = &keenhi_t40_as3648_pdata,
	},
#endif	
#ifdef CONFIG_VIDEO_AD5816
	{
		I2C_BOARD_INFO("ad5816", 0x0E),
		.platform_data = &keenhi_t40_ad5816_pdata,
	},
#endif	
#ifdef CONFIG_VIDEO_T8EV5
	{
		I2C_BOARD_INFO("t8ev5", 0x3c),
		.platform_data = &kai_t8ev5_data,
	},
#endif
};

#ifdef  CONFIG_LIGHTSENSOR_EPL6814
static struct i2c_board_info __initdata i2c0_board_info_lightsensor[] = {
		{
		        I2C_BOARD_INFO(ELAN_LS_6814, 0x92 >> 1),
    		},
};
#endif

static void camera_gpio_init(void)
{
	int ret;
#ifdef CONFIG_VIDEO_T8EV5
	ret = gpio_request(T8EV5_POWER_DWN_PIN, "t8ev5_pwdn");
	if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s\n",
			__func__, "T8EV5_PWDN_GPIO");
	}
	gpio_direction_output(T8EV5_POWER_DWN_PIN, 0);//old 1

	//tegra_gpio_enable(T8EV5_POWER_RST_PIN);
	ret = gpio_request(T8EV5_POWER_RST_PIN, "t8ev5_reset");
	if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s\n",
			__func__, "T8EV5_RESET_GPIO");
	}
	gpio_direction_output(T8EV5_POWER_RST_PIN, 0);
#endif

}

static void keenhi_lightsensor_init(void)
{
#ifdef  CONFIG_LIGHTSENSOR_EPL6814
	i2c_register_board_info(KEENHI_LIGHTSENSOR_I2C_BUS, i2c0_board_info_lightsensor,
		ARRAY_SIZE(i2c0_board_info_lightsensor));
#endif

}
static int keenhi_t40_camera_init(void)
{
	tegra_pinmux_config_table(&mclk_disable, 1);
	tegra_pinmux_config_table(&pbb0_disable, 1);
	camera_gpio_init();
	kai_t8ev5_data.flash_delay = 0;
	kai_t8ev5_data.flash_en= 0;
	kai_t8ev5_data.led_en= 0;

	i2c_register_board_info(2, keenhi_t40_camera_i2c_board_info,
		ARRAY_SIZE(keenhi_t40_camera_i2c_board_info));
	return 0;
}

#define TEGRA_CAMERA_GPIO(_gpio, _label, _value)		\
	{							\
		.gpio = _gpio,					\
		.label = _label,				\
		.value = _value,				\
	}

static struct cm3217_platform_data keenhi_t40_cm3217_pdata = {
	.levels = {10, 160, 225, 320, 640, 1280, 2600, 5800, 8000, 10240},
	.golden_adc = 0,
	.power = 0,
};

static struct i2c_board_info keenhi_t40_i2c0_board_info_cm3217[] = {
	{
		I2C_BOARD_INFO("cm3217", 0x10),
		.platform_data = &keenhi_t40_cm3217_pdata,
	},
};

/* MPU board file definition	*/
#ifdef CONFIG_MPU_SENSORS_MPU3050
static struct mpu_platform_data mpu3050_gyro_data = {
	.int_config	= 0x10,
	.level_shifter	= 0,
	.orientation	= MPU_GYRO_ORIENTATION,	/* Located in board_[platformname].h	*/
#if 0
	.sec_slave_type	= SECONDARY_SLAVE_TYPE_ACCEL,
	.sec_slave_id	= ACCEL_ID_KXTF9,
	.secondary_i2c_addr	= MPU3050_ACCEL_ADDR,
	.secondary_read_reg	= 0x06,
	.secondary_orientation	= MPU3050_ACCEL_ORIENTATION,
	.key		= {0x4E, 0xCC, 0x7E, 0xEB, 0xF6, 0x1E, 0x35, 0x22,
			   0x00, 0x34, 0x0D, 0x65, 0x32, 0xE9, 0x94, 0x89},
#endif
};

static struct ext_slave_platform_data mpu3050_accel_data =
{
	.address	= MPU3050_ACCEL_ADDR,
	.irq		= 0,
	.adapt_num	= MPU3050_ACCEL_BUS_NUM,
	.bus		= EXT_SLAVE_BUS_SECONDARY,
	.orientation	= MPU3050_ACCEL_ORIENTATION,
};

static struct ext_slave_platform_data mpu3050_compass_data =
{
	.address	= MPU_COMPASS_ADDR,
	.irq		= 0,
	.adapt_num	= MPU_COMPASS_BUS_NUM,
	.bus		= EXT_SLAVE_BUS_PRIMARY,
	.orientation	= MPU_COMPASS_ORIENTATION,
};

static struct i2c_board_info __initdata inv_mpu3050_i2c2_board_info[] = {
	{
		I2C_BOARD_INFO(MPU_GYRO_NAME, MPU_GYRO_ADDR),
		.platform_data = &mpu3050_gyro_data,
	},
	{
		I2C_BOARD_INFO(MPU3050_ACCEL_NAME, MPU3050_ACCEL_ADDR),
		.platform_data = &mpu3050_accel_data,
	},
	{
		I2C_BOARD_INFO(MPU_COMPASS_NAME, MPU_COMPASS_ADDR),
		.platform_data = &mpu3050_compass_data,
	},
};

static void mpuirq_init(void)
{
	int ret = 0,i = 0;
	pr_info("*** MPU START *** mpuirq_init...\n");
#if	MPU_GYRO_IRQ_GPIO
	ret = gpio_request(MPU_GYRO_IRQ_GPIO, MPU_GYRO_NAME);
	if (ret < 0) {
		pr_err("%s: gpio_request failed %d\n", __func__, ret);
		return;
	}
	ret = gpio_direction_input(MPU_GYRO_IRQ_GPIO);
	if (ret < 0) {
		pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
		gpio_free(MPU_GYRO_IRQ_GPIO);
		return;
	}	
#endif	
#if	MPU3050_ACCEL_IRQ_GPIO
		ret = gpio_request(MPU3050_ACCEL_IRQ_GPIO, MPU3050_ACCEL_NAME);
		if (ret < 0) {
			pr_err("%s: gpio_request failed %d\n", __func__, ret);
			return;
		}
		ret = gpio_direction_input(MPU3050_ACCEL_IRQ_GPIO);
		if (ret < 0) {
			pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
			gpio_free(MPU3050_ACCEL_IRQ_GPIO);
			return;
		}	
#endif
#if	MPU_COMPASS_IRQ_GPIO
	ret = gpio_request(MPU_COMPASS_IRQ_GPIO, MPU_COMPASS_NAME);
	if (ret < 0) {
		pr_err("%s: gpio_request failed %d\n", __func__, ret);
		return;
	}
	ret = gpio_direction_input(MPU_COMPASS_IRQ_GPIO);
	if (ret < 0) {
		pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
		gpio_free(MPU_COMPASS_IRQ_GPIO);
		return;
	}
#endif	
	pr_info("*** MPU END *** mpuirq_init...\n");

#if	MPU_GYRO_IRQ_GPIO
  inv_mpu3050_i2c2_board_info[i].irq = gpio_to_irq(MPU_GYRO_IRQ_GPIO);
#endif
  i++;	

#if MPU3050_ACCEL_IRQ_GPIO
	inv_mpu3050_i2c2_board_info[i].irq = gpio_to_irq(MPU3050_ACCEL_IRQ_GPIO);
#endif
	i++;
#if MPU_COMPASS_IRQ_GPIO
	inv_mpu3050_i2c2_board_info[i++].irq = gpio_to_irq(MPU_COMPASS_IRQ_GPIO);
#endif
	i2c_register_board_info(MPU_GYRO_BUS_NUM, inv_mpu3050_i2c2_board_info,
		ARRAY_SIZE(inv_mpu3050_i2c2_board_info));
}
#endif

static int keenhi_t40_nct1008_init(void)
{
	int nct1008_port;
	int ret = 0;

	nct1008_port = TEGRA_GPIO_PO4;

	tegra_add_cdev_trips(keenhi_t40_nct1008_pdata.trips,
				&keenhi_t40_nct1008_pdata.num_trips);

	keenhi_t40_i2c4_nct1008_board_info[0].irq = gpio_to_irq(nct1008_port);
	pr_info("%s: keenhi_t40 nct1008 irq %d",
			__func__, keenhi_t40_i2c4_nct1008_board_info[0].irq);

	ret = gpio_request(nct1008_port, "temp_alert");
	if (ret < 0)
		return ret;

	ret = gpio_direction_input(nct1008_port);
	if (ret < 0) {
		pr_info("%s: calling gpio_free(nct1008_port)", __func__);
		gpio_free(nct1008_port);
	}

	/* macallan has thermal sensor on GEN1-I2C i.e. instance 0 */
	i2c_register_board_info(0, keenhi_t40_i2c4_nct1008_board_info,
		ARRAY_SIZE(keenhi_t40_i2c4_nct1008_board_info));

	return ret;
}

#ifdef CONFIG_TEGRA_SKIN_THROTTLE
static struct thermal_trip_info skin_trips[] = {
	{
		.cdev_type = "skin-balanced",
		.trip_temp = 45000,
		.trip_type = THERMAL_TRIP_PASSIVE,
		.upper = THERMAL_NO_LIMIT,
		.lower = THERMAL_NO_LIMIT,
		.hysteresis = 0,
	},
};

static struct therm_est_subdevice skin_devs[] = {
	{
		.dev_data = "Tdiode",
		.coeffs = {
			2, 1, 1, 1,
			1, 1, 1, 1,
			1, 1, 1, 0,
			1, 1, 0, 0,
			0, 0, -1, -7
		},
	},
	{
		.dev_data = "Tboard",
		.coeffs = {
			-11, -7, -5, -3,
			-3, -2, -1, 0,
			0, 0, 1, 1,
			1, 2, 2, 3,
			4, 6, 11, 18
		},
	},
};

static struct therm_est_data skin_data = {
	.num_trips = ARRAY_SIZE(skin_trips),
	.trips = skin_trips,
	.toffset = 9793,
	.polling_period = 1100,
	.passive_delay = 10000,
	.tc1 = 10,
	.tc2 = 1,
	.ndevs = ARRAY_SIZE(skin_devs),
	.devs = skin_devs,
};

static struct throttle_table skin_throttle_table[] = {
	/* CPU_THROT_LOW cannot be used by other than CPU */
	/*    CPU,   C2BUS,   C3BUS,   SCLK,    EMC    */
	{ { 1810500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1785000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1759500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1734000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1708500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1683000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1657500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1632000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1606500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1581000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1555500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1530000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1504500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1479000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1453500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1428000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1402500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1377000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1351500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1326000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1300500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1275000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1249500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1224000, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1198500, NO_CAP, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1173000, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1147500, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1122000, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1096500, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1071000, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1045500, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ { 1020000, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  994500, 636000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  969000, 600000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  943500, 600000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  918000, 600000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  892500, 600000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  867000, 600000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  841500, 564000, NO_CAP, NO_CAP, NO_CAP } },
	{ {  816000, 564000, NO_CAP, NO_CAP, 792000 } },
	{ {  790500, 564000, NO_CAP, 372000, 792000 } },
	{ {  765000, 564000, 468000, 372000, 792000 } },
	{ {  739500, 528000, 468000, 372000, 792000 } },
	{ {  714000, 528000, 468000, 336000, 792000 } },
	{ {  688500, 528000, 420000, 336000, 792000 } },
	{ {  663000, 492000, 420000, 336000, 792000 } },
	{ {  637500, 492000, 420000, 336000, 408000 } },
	{ {  612000, 492000, 420000, 300000, 408000 } },
	{ {  586500, 492000, 360000, 336000, 408000 } },
	{ {  561000, 420000, 420000, 300000, 408000 } },
	{ {  535500, 420000, 360000, 228000, 408000 } },
	{ {  510000, 420000, 288000, 228000, 408000 } },
	{ {  484500, 324000, 288000, 228000, 408000 } },
	{ {  459000, 324000, 288000, 228000, 408000 } },
	{ {  433500, 324000, 288000, 228000, 408000 } },
	{ {  408000, 324000, 288000, 228000, 408000 } },
};

static struct balanced_throttle skin_throttle = {
        .throt_tab_size = ARRAY_SIZE(skin_throttle_table),
        .throt_tab = skin_throttle_table,
};

static int __init keenhi_t40_skin_init(void)
{
		balanced_throttle_register(&skin_throttle, "skin-balanced");
		tegra_skin_therm_est_device.dev.platform_data = &skin_data;
		platform_device_register(&tegra_skin_therm_est_device);
	return 0;
}
late_initcall(keenhi_t40_skin_init);
#endif
#ifdef CONFIG_GPIO_SWITCH_SERAIL

static struct gpio_switch_platform_data gpio_switch_pdata = {
     .gpio = TEGRA_GPIO_SERIAL_ENABLE,
       
};

static struct platform_device gpio_switch_serial = {
               .name   = "gpio_switch",
               .id     = 0,
               .dev    = {
                       .platform_data  = &gpio_switch_pdata,
               },
};

static void keenhi_gpio_switch(void)
{
       platform_device_register(&gpio_switch_serial);

}
#endif
int __init keenhi_t40_sensors_init(void)
{
	int err;

//	tegra_get_board_info(&board_info);

	/* NCT218 thermal sensor is only mounted on E1546 */
//	if (board_info.board_id == BOARD_E1546)
	{
		err = keenhi_t40_nct1008_init();
		if (err) {
			pr_err("%s: nct1008 register failed.\n", __func__);
			return err;
		}
	}
	keenhi_lightsensor_init();
	keenhi_t40_camera_init();
#ifdef CONFIG_GPIO_SWITCH_SERAIL
	keenhi_gpio_switch();
#endif
#ifdef CONFIG_MPU_SENSORS_MPU3050	
	mpuirq_init();
#endif

	i2c_register_board_info(0, keenhi_t40_i2c0_board_info_cm3217,
				ARRAY_SIZE(keenhi_t40_i2c0_board_info_cm3217));

	return 0;
}
