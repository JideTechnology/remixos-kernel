/*
 * arch/arm/mach-tegra/board-macallan-sensors.c
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
#include <media/imx091.h>
#include <media/ov9772.h>
#include <media/as364x.h>
#include <media/ad5816.h>
#include <generated/mach-types.h>
#include <linux/power/sbs-battery.h>

#include "gpio-names.h"
#include "board.h"
#include "board-common.h"
#include "board-macallan.h"
#include "cpu-tegra.h"
#include "devices.h"
#include "tegra-board-id.h"
#include "dvfs.h"

#ifdef CONFIG_VIDEO_T8EV5
#include <media/t8ev5.h>
#endif

#if defined(CONFIG_INPUT_KIONIX_ACCEL)
#include <linux/input/kionix_accel.h>
#endif

#if defined(CONFIG_VIDEO_T8EV5) || defined(CONFIG_VIDEO_MT9P111)
static struct regulator *macallan_1v8_cam1;
static struct regulator *macallan_2v8_cam1;
#endif

static struct board_info board_info;

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

static int __init macallan_throttle_init(void)
{
	if (machine_is_macallan())
		balanced_throttle_register(&tj_throttle, "tegra-balanced");
	return 0;
}
module_init(macallan_throttle_init);

static struct nct1008_platform_data macallan_nct1008_pdata = {
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

static struct i2c_board_info macallan_i2c4_nct1008_board_info[] = {
	{
		I2C_BOARD_INFO("nct1008", 0x4C),
		.platform_data = &macallan_nct1008_pdata,
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
 * As a workaround, macallan_vcmvdd need to be allocated to activate the
 * sensor devices. This is due to the focuser device(AD5816) will hook up
 * the i2c bus if it is not powered up.
*/

#if defined(CONFIG_VIDEO_T8EV5) || defined(CONFIG_VIDEO_MT9P111)
static struct regulator *macallan_vcmvdd;

static int macallan_get_vcmvdd(void)
{
	if (!macallan_vcmvdd) {
		macallan_vcmvdd = regulator_get(NULL, "vdd_af_cam1");
		if (unlikely(WARN_ON(IS_ERR(macallan_vcmvdd)))) {
			pr_err("%s: can't get regulator vcmvdd: %ld\n",
				__func__, PTR_ERR(macallan_vcmvdd));
			macallan_vcmvdd = NULL;
			return -ENODEV;
		}
	}
	return 0;
}
#endif

#if defined(CONFIG_VIDEO_MT9P111)||defined(CONFIG_VIDEO_T8EV5)
#define MT9P111_POWER_RST_PIN TEGRA_GPIO_PBB7
#define MT9P111_POWER_DWN_PIN TEGRA_GPIO_PBB5
static int kai_mt9p111_power_on(void)
{
	int err;
	pr_err("%s:==========>EXE\n",__func__);
	if (macallan_get_vcmvdd())
		goto mt9p111_poweron_fail;
	err = regulator_enable(macallan_vcmvdd);
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
	
	if (macallan_vcmvdd){
		regulator_disable(macallan_vcmvdd);
		regulator_put(macallan_vcmvdd);
		macallan_vcmvdd =NULL;
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


#if defined(CONFIG_VIDEO_MT9P111)
struct yuv_sensor_platform_data kai_mt9p111_data = {
	.power_on = kai_mt9p111_power_on,
	.power_off = kai_mt9p111_power_off,
};
#endif
#endif

#if defined(CONFIG_VIDEO_T8EV5)
#define T8EV5_POWER_RST_PIN TEGRA_GPIO_PBB7
#define T8EV5_POWER_DWN_PIN TEGRA_GPIO_PBB5

#define T8EV5_POWER_FRONT_RST_PIN TEGRA_GPIO_PBB3
#define T8EV5_POWER_FRONT_DWN_PIN TEGRA_GPIO_PBB6
static int kai_t8ev5_power_on(u8 flag)
{
	int err; 
	int rst_gpio = T8EV5_POWER_RST_PIN;
	int dwn_gpio = T8EV5_POWER_DWN_PIN;

	pr_err("%s:==========>EXE\n",__func__);

	if(flag & T8EV5_FRONT_ID){
		rst_gpio = T8EV5_POWER_FRONT_RST_PIN;
		dwn_gpio = T8EV5_POWER_FRONT_DWN_PIN;
	}else{
		if (macallan_get_vcmvdd())
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
		err = regulator_enable(macallan_vcmvdd);
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
	int rst_gpio = T8EV5_POWER_RST_PIN;
	int dwn_gpio = T8EV5_POWER_DWN_PIN;

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
	if(!(flag&T8EV5_FRONT_ID)){
		if (macallan_vcmvdd) {
			regulator_disable(macallan_vcmvdd);
			regulator_put(macallan_vcmvdd);
			macallan_vcmvdd =NULL;
		}
	}
	
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
	.led_en= 0,
	.flash_en= 0,
	.flash_delay = 3,
	.power_on = kai_sensor_power_on,
	.power_off = kai_sensor_power_off,
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
#ifdef CONFIG_VIDEO_MT9P111
	ret = gpio_request(MT9P111_POWER_DWN_PIN, "mt9p111_pwdn");
	if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s\n",
			__func__, "MT9P111_PWDN_GPIO");
	}
	gpio_direction_output(MT9P111_POWER_DWN_PIN, 1);//old 1

	//tegra_gpio_enable(T8EV5_POWER_RST_PIN);
	ret = gpio_request(MT9P111_POWER_RST_PIN, "mt9p111_reset");
	if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s\n",
			__func__, "MT9P111_RESET_GPIO");
	}
	gpio_direction_output(MT9P111_POWER_RST_PIN, 0);
#endif
}

static struct i2c_board_info macallan_camera_i2c_board_info[] = {
#ifdef CONFIG_VIDEO_T8EV5
	{
		I2C_BOARD_INFO("t8ev5", 0x3c),
		.platform_data = &kai_t8ev5_data,
	},
#endif
#ifdef CONFIG_VIDEO_MT9P111
	{
		I2C_BOARD_INFO("mt9p111", 0x3c),
		.platform_data = &kai_mt9p111_data,
	},
#endif
};
static int macallan_camera_init(void)
{
	tegra_pinmux_config_table(&mclk_disable, 1);
	tegra_pinmux_config_table(&pbb0_disable, 1);

	camera_gpio_init();
	kai_t8ev5_data.flash_delay = 0;
	kai_t8ev5_data.flash_en= 0;
	kai_t8ev5_data.led_en= 0;
	i2c_register_board_info(2, macallan_camera_i2c_board_info,
		ARRAY_SIZE(macallan_camera_i2c_board_info));

	return 0;
}

#define TEGRA_CAMERA_GPIO(_gpio, _label, _value)		\
	{							\
		.gpio = _gpio,					\
		.label = _label,				\
		.value = _value,				\
	}

static struct cm3217_platform_data macallan_cm3217_pdata = {
	.levels = {10, 160, 225, 320, 640, 1280, 2600, 5800, 8000, 10240},
	.golden_adc = 0,
	.power = 0,
};

static struct i2c_board_info macallan_i2c0_board_info_cm3217[] = {
	{
		I2C_BOARD_INFO("cm3217", 0x10),
		.platform_data = &macallan_cm3217_pdata,
	},
};

#ifdef CONFIG_INPUT_KIONIX_ACCEL
struct kionix_accel_platform_data kionix_accel_pdata = {
	.min_interval = 5,
	.poll_interval = 200,
	.accel_direction = 4,
	.accel_irq_use_drdy = 0,
	.accel_res = KIONIX_ACCEL_RES_12BIT,
	.accel_g_range = KIONIX_ACCEL_G_2G,
};

static struct i2c_board_info i2c0_board_info_kionix[] = {
	{
		I2C_BOARD_INFO("kionix_accel", 0x0e),
		.platform_data = &kionix_accel_pdata,
	},
	{
		I2C_BOARD_INFO("kionix_accel", 0x0f),
		.platform_data = &kionix_accel_pdata,
	},
};
#endif

#ifdef CONFIG_SENSORS_AKM8963
static struct i2c_board_info i2c0_board_info_akm8963[] = {
	{
		I2C_BOARD_INFO("akm8975", 0x0d),
	},
};
#endif



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

static int macallan_nct1008_init(void)
{
	int nct1008_port;
	int ret = 0;

	nct1008_port = TEGRA_GPIO_PO4;

	tegra_add_cdev_trips(macallan_nct1008_pdata.trips,
				&macallan_nct1008_pdata.num_trips);

	macallan_i2c4_nct1008_board_info[0].irq = gpio_to_irq(nct1008_port);
	pr_info("%s: macallan nct1008 irq %d",
			__func__, macallan_i2c4_nct1008_board_info[0].irq);

	ret = gpio_request(nct1008_port, "temp_alert");
	if (ret < 0)
		return ret;

	ret = gpio_direction_input(nct1008_port);
	if (ret < 0) {
		pr_info("%s: calling gpio_free(nct1008_port)", __func__);
		gpio_free(nct1008_port);
	}

	/* macallan has thermal sensor on GEN1-I2C i.e. instance 0 */
	i2c_register_board_info(0, macallan_i2c4_nct1008_board_info,
		ARRAY_SIZE(macallan_i2c4_nct1008_board_info));

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
			-1, 0, 1, 0,
			0, -1, 0, -1,
			0, -1, 0, -1,
			0, -1, -1, -1,
			-1, -1, -3, -13
		},
	},
	{
		.dev_data = "Tboard",
		.coeffs = {
			39, 6, 4, 7,
			2, 5, 6, 2,
			4, 3, 6, 3,
			3, 6, 7, 1,
			1, 2, 0, 3
		},
	},
};

static struct therm_est_data skin_data = {
	.num_trips = ARRAY_SIZE(skin_trips),
	.trips = skin_trips,
	.toffset = 2724,
	.polling_period = 1100,
	.passive_delay = 15000,
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

static int __init macallan_skin_init(void)
{
	if (machine_is_macallan()) {
		balanced_throttle_register(&skin_throttle, "skin-balanced");
		tegra_skin_therm_est_device.dev.platform_data = &skin_data;
		platform_device_register(&tegra_skin_therm_est_device);
	}

	return 0;
}
late_initcall(macallan_skin_init);
#endif

int __init macallan_sensors_init(void)
{
//	int err;

	tegra_get_board_info(&board_info);

	/* NCT218 thermal sensor is only mounted on E1546 */
	
	macallan_nct1008_init();
	macallan_camera_init();
#ifdef CONFIG_MPU_SENSORS_MPU3050	
	mpuirq_init();
#endif

#ifdef CONFIG_INPUT_KIONIX_ACCEL
	i2c0_board_info_kionix[0].irq = gpio_to_irq(TEGRA_GPIO_PO5);
	i2c_register_board_info(0, i2c0_board_info_kionix,
			ARRAY_SIZE(i2c0_board_info_kionix));
#endif

#ifdef CONFIG_SENSORS_AKM8963
	i2c0_board_info_akm8963[0].irq = gpio_to_irq(TEGRA_GPIO_PX1);
	i2c_register_board_info(0, i2c0_board_info_akm8963,
			ARRAY_SIZE(i2c0_board_info_akm8963));
#endif

	i2c_register_board_info(0, macallan_i2c0_board_info_cm3217,
				ARRAY_SIZE(macallan_i2c0_board_info_cm3217));

	return 0;
}
