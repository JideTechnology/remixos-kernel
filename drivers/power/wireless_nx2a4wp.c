/*
 * wireless_nx2a4wp.c - NX2A4WP rezence chip driver
 *
 * Copyright (C) 2015 Intel Corporation
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author: Jenny TC <jenny.tc@intel.com>
 * Author: Vineesh k k <vineesh.k.k@intel.com>
 */

#include <linux/acpi.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/usb/otg.h>
#include <linux/version.h>
#include <linux/acpi.h>
#include <linux/jiffies.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>

#include "wireless_rezence.h"

#define DRV_NAME "INTA4321"
#define DEV_NAME "INTA4321"

#define NX2A4WP_STATL_ADDR	0x00
#define NX2A4WP_STATU_ADDR	0x01
#define NX2A4WP_SFCLRL_ADDR	0x02
#define NX2A4WP_SFCLRU_ADDR	0x03
#define NX2A4WP_ICON_ADDR	0x04
#define NX2A4WP_SYSCONL_ADDR	0x05
#define NX2A4WP_SYSCONU_ADDR	0x06
#define NX2A4WP_ICONL_ADDR	0x07
#define NX2A4WP_ADCEL_ADDR	0x09
#define NX2A4WP_ADCEU_ADDR	0x0A

#define NX2A4WP_ADC0DU_ADDR	0x0B
#define NX2A4WP_ADC1DU_ADDR	0x0C
#define NX2A4WP_ADC2DU_ADDR	0x0D
#define NX2A4WP_ADC3DU_ADDR	0x0E
#define NX2A4WP_ADC4DU_ADDR	0x0F
#define NX2A4WP_ADC5DU_ADDR	0x10
#define NX2A4WP_ADC6DU_ADDR	0x11
#define NX2A4WP_ADC1_ADC0_DL_ADDR 0x12
#define NX2A4WP_ADC3_ADC2_DL_ADDR 0x13
#define NX2A4WP_ADC5_ADC4_DL_ADDR 0x14
#define NX2A4WP_ADC6_DL_ADDR	0x15
#define NX2A4WP_DCSEL_ADDR	0x19
#define NX2A4WP_NTCSEL_ADDR	0x1A
#define NX2A4WP_DEV_ID_ADDR	0x1F

#define NX2A4WP_OTP_MASK	0x06
#define NX2A4WP_IDC_MASK	0x80
#define NX2A4WP_OVP_MASK	0x10
#define NX2A4WP_TAH_MASK	0x01
#define NX2A4WP_WDT_MASK	0xE0

#define NX2A4WP_ADC_SWEEP_EN	0x80
#define NX2A4WP_VRECT_EN	0x01
#define NX2A4WP_IRECT_EN	0x02
#define NX2A4WP_VDCOUT_EN	0x04
#define NX2A4WP_IDCOUT_EN	0x08
#define NX2A4WP_VNTC_EN		0x10

#define NX2A4WP_DCSEL_VOLT_MASK	0xF8
#define NX2A4WP_DCSEL_CUR_MASK	0x07
#define NX2A4WP_SYSCONU_DCEN_MASK	0x01

#define NX2A4WP_SYSCONL_DCTOE_EN	0x01
#define NX2A4WP_SYSCONL_GPNTE_EN	0x04
#define NX2A4WP_SYSCONL_DCNTE_EN	0x08
#define NX2A4WP_SYSCONL_PRECT_EN	0x10
#define NX2A4WP_SYSCONL_HRECT_EN	0x20
#define NX2A4WP_SYSCONL_DCOVE_EN	0x40
#define NX2A4WP_SYSCONL_INTCL_EN	0x80

#define MIN_DCSEL_VOLT	4200
#define MAX_DCSEL_VOLT	6200
#define DCSEL_VOLT_OFFSET	200

#define MIN_DCSEL_CUR	500
#define MAX_DCSEL_CUR	1200
#define DCSEL_CUR_OFFSET	100


#define NX2A4WP_ID 0x28

#define VDCOUT_TABLE_SIZE	12
#define IDCOUT_TABLE_SIZE	13
#define PRU_TEMP_TABLE_SIZE	28
#define PTU_POWER_TABLE_SIZE	5
#define PSY_MAX_STR_PROP_LEN	33
#define A4WP_INT_N_GPIO_BASE	414
#define A4WP_INT_N_GPIO_OFF	77

#define A4WP_LONG_BEACON_ON_PERIOD	(25 * HZ/1000)
#define A4WP_LONG_BEACON_MAX_PERIOD	(3 * HZ)
#define A4WP_LONG_BEACON_TIME	(100 * HZ/1000)

#define PRU_MAX_MA	900

#define IRECT_BIT_RESOLUTION_INV	1715
#define IRECT_ADC12_TO_MA(__value)  \
		((((int)(__value)) * 1000) / IRECT_BIT_RESOLUTION_INV)

#define VRECT_BIT_RESOLUTION	8
#define VRECT_ADC12_TO_MV(__value)  (((int)(__value)) * VRECT_BIT_RESOLUTION)

/* Filter short beacon:
 *  0: do not filter short beacon but immediately send uevent
 *  1: filter out short beacon and only sent uevent when we are on long beacon
 */
#define FILTER_SHORT_BEACON	0

enum power_supply_property nx2a4wp_usb_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MAX_TEMP,
	POWER_SUPPLY_PROP_MIN_TEMP,
	POWER_SUPPLY_PROP_VRECT,
	POWER_SUPPLY_PROP_IRECT,
	POWER_SUPPLY_PROP_VDCOUT,
	POWER_SUPPLY_PROP_IDCOUT,
	POWER_SUPPLY_PROP_PRU_TEMP,
	POWER_SUPPLY_PROP_PRU_DCEN,
	POWER_SUPPLY_PROP_PRU_DYNAMIC_PARAM,
	POWER_SUPPLY_PROP_PRU_STATIC_PARAM,
	POWER_SUPPLY_PROP_PTU_STATIC_PARAM,
	POWER_SUPPLY_PROP_PTU_POWER,
	POWER_SUPPLY_PROP_PTU_CLASS,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CURRENT_MAX,
};

enum {
	A4WP_FLAGS_PRESENT_BIT,
	A4WP_FLAGS_MAINTENANCE_BIT,
	A4WP_FLAGS_ATTACH_BIT,
	A4WP_FLAGS_PRU_DCEN_BIT,
};

struct nx2a4wp_context {
	unsigned long flags;
	unsigned long health;

	int temperature;
	int temp_max;
	int temp_min;
	int volt_max;
	int current_max;
	int vrect;
	int irect;
	int vdcout;
	int idcout;
	int pru_temp;
	unsigned long ptu_class;
	unsigned long ptu_power;
	unsigned long status;
	char pru_dyn_param[PSY_MAX_STR_PROP_LEN];
	char pru_stat_param[PSY_MAX_STR_PROP_LEN];
	char ptu_stat_param[PSY_MAX_STR_PROP_LEN];
	struct power_supply psy_wc;
	struct power_supply_cable_props cable;
	struct wireless_pru rez;
	struct i2c_client *client;
	struct gpio_desc *irq_gpio;
	struct mutex stat_lock;
	struct work_struct  pruon_wrkr;
	struct delayed_work detach_wrkr;
	struct delayed_work attach_wrkr;

	/* Timestamp used for performance measurments */
	unsigned long irq_timestamp;
	unsigned long present_timestamp;
	unsigned long present2adv_time;
};

static u32 vdcout_adc_code[VDCOUT_TABLE_SIZE][2] = {
	{0x00, 0},
	{0x61B, 4200},
	{0x666, 4400},
	{0x6B1, 4600},
	{0x6FC, 4800},
	{0x745, 5000},
	{0x791, 5200},
	{0x7DB, 5400},
	{0x826, 5600},
	{0x870, 5800},
	{0x8BA, 6000},
	{0x905, 6200},
};

static u32 idcout_adc_code[IDCOUT_TABLE_SIZE][2] = {
	{0x00, 0},
	{0x38, 100},
	{0x3D, 200},
	{0x43, 300},
	{0x47, 400},
	{0x4C, 500},
	{0x50, 600},
	{0x55, 700},
	{0x59, 800},
	{0x5E, 900},
	{0x62, 1000},
	{0x66, 1100},
	{0x6B, 1200},
};

static u32 pru_temp_adc_code[PRU_TEMP_TABLE_SIZE][2] = {
	{0x2FA, 95},
	{0x34A, 90},
	{0x3A3, 85},
	{0x405, 80},
	{0x470, 75},
	{0x4E3, 70},
	{0x561, 65},
	{0x5E9, 60},
	{0x679, 55},
	{0x711, 50},
	{0x7AF, 45},
	{0x853, 40},
	{0x8F9, 35},
	{0x99D, 30},
	{0xA3C, 25},
	{0xADB, 20},
	{0xB6F, 15},
	{0xBF9, 10},
	{0xC78, 5},
	{0xCEA, 0},
	{0xD4F, -5},
	{0xDA6, -10},
	{0xDF0, -15},
	{0xE2E, -20},
	{0xE62, -25},
	{0xE8B, -30},
};

static u32 ptu_power_code[PTU_POWER_TABLE_SIZE][2] = {
	{0x00, 0},
	{0x28, 6000},
	{0x48, 8400},
	{0x54, 10200},
	{0x69, 33000},
};


static int find_adc_code(u32 val, int table_size, u32 adc_code[][2])
{
	int left = 0;
	int right = table_size;
	int mid;

	while (left <= right) {
		mid = (left + right)/2;
		if (val == adc_code[mid][0] ||
			(mid > 0 &&
			val < adc_code[mid][0] &&
			val > adc_code[mid-1][0]))
			return mid;
		else if (val < adc_code[mid][0])
			right = mid - 1;
		else if (val > adc_code[mid][0])
			left = mid + 1;
	}

	return -EINVAL;
}

static int adc_lookup(u32 adc_val, int table_size, u32 adc_code[][2], int *val)
{
	int x0, x1, y0, y1;
	int nr, dr;		/* Numerator & Denominator */
	int indx;
	int x = adc_val;

	indx = find_adc_code(adc_val, table_size, adc_code);
	if (indx < 0)
		return -EINVAL;

	if (indx == 0 || adc_code[indx][0] == adc_val) {
		*val = adc_code[indx][1];
		return 0;
	}

	/*
	 * The ADC code is in between two values directly defined in the
	 * table. So, do linear interpolation to calculate the temperature.
	 */
	x0 = adc_code[indx][0];
	x1 = adc_code[indx - 1][0];
	y0 = adc_code[indx][1];
	y1 = adc_code[indx - 1][1];

	/*
	 * Find y:
	 * Of course, we can avoid these variables, but keep them
	 * for readability and maintainability.
	 */
	nr = (x - x0) * y1 + (x1 - x) * y0;
	dr =  x1 - x0;

	if (!dr)
		return -EINVAL;

	*val = nr / dr;

	return 0;
}

static inline int nx2a4wp_read_reg(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

static inline int nx2a4wp_write_reg(struct i2c_client *client, u8 reg, u8 data)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, data);
	if (ret < 0)
		dev_err(&client->dev, "Error(%d) in writing %d to reg %d\n",
			ret, data, reg);
	return ret;
}

static inline int nx2a4wp_read_modify_reg(struct i2c_client *client, u8 reg,
					  u8 mask, u8 val)
{
	int ret;

	ret = nx2a4wp_read_reg(client, reg);
	if (ret < 0)
		return ret;
	ret = (ret & ~mask) | (mask & val);

	return nx2a4wp_write_reg(client, reg, ret);
}

static void nx2a4wp_reg_dump(struct i2c_client *client)
{
	int reg = 0;

	for (reg = 0; reg < 0x15; reg++) {
		dev_info(&client->dev,
			"%s reg(%x)=%x\n", __func__, reg,
				nx2a4wp_read_reg(client, reg));
	}
	dev_info(&client->dev,
		"%s DCSEL=%x\n", __func__,
			nx2a4wp_read_reg(client, 0x19));
}

static int nx2a4wp_get_wpr_irect(struct i2c_client *client, int *irect_val)
{
	int ret;
	u16 irect_raw;

	ret = nx2a4wp_write_reg(client, NX2A4WP_ADCEL_ADDR,
			(NX2A4WP_ADC_SWEEP_EN | NX2A4WP_IRECT_EN));

	if (ret < 0)
		return -EINVAL;

	ret = nx2a4wp_read_reg(client, NX2A4WP_ADC1DU_ADDR);
	if (ret < 0)
		return -EINVAL;

	irect_raw = ret << 4;

	ret = nx2a4wp_read_reg(client, NX2A4WP_ADC1_ADC0_DL_ADDR);

	if (ret < 0)
		return -EINVAL;

	irect_raw |= ((ret >> 4) & 0xF);

	*irect_val = IRECT_ADC12_TO_MA(irect_raw);

	return 0;
}


static int nx2a4wp_get_wpr_idcout(struct i2c_client *client, int *idcval)
{
	int ret;
	int idcout;
	u8 idcout_raw;

	ret = nx2a4wp_write_reg(client, NX2A4WP_ADCEL_ADDR,
			(NX2A4WP_ADC_SWEEP_EN | NX2A4WP_IDCOUT_EN));
	if (ret < 0)
		return -EINVAL;

	idcout_raw = nx2a4wp_read_reg(client, NX2A4WP_ADC3DU_ADDR);
	if (idcout_raw < 0)
		return -EINVAL;

	ret = adc_lookup(idcout_raw, IDCOUT_TABLE_SIZE,
				idcout_adc_code, &idcout);
	if (ret < 0)
		return ret;

	*idcval = idcout;

	return 0;
}

static int nx2a4wp_get_wpr_temp(struct i2c_client *client)
{
	int ret;
	int temp;
	u16 temp_raw;

	ret = nx2a4wp_write_reg(client, NX2A4WP_ADCEL_ADDR,
			(NX2A4WP_ADC_SWEEP_EN | NX2A4WP_VNTC_EN));
	if (ret < 0)
		return -EINVAL;
	ret = nx2a4wp_read_reg(client, NX2A4WP_ADC4DU_ADDR);
	if (ret < 0)
		return -EINVAL;

	temp_raw = ret << 4;

	ret = nx2a4wp_read_reg(client, NX2A4WP_ADC5_ADC4_DL_ADDR);
	if (ret < 0)
		return -EINVAL;

	temp_raw |= (ret & 0xF);
	ret = adc_lookup(temp_raw, PRU_TEMP_TABLE_SIZE, pru_temp_adc_code,
								&temp);

	if (ret < 0)
		return ret;

	return temp;
}

static int nx2a4wp_get_wpr_vrect(struct i2c_client *client, int *vrect_val)
{
	int ret;
	u16 vrect_raw;

	ret = nx2a4wp_write_reg(client, NX2A4WP_ADCEL_ADDR,
			(NX2A4WP_ADC_SWEEP_EN | NX2A4WP_VRECT_EN));
	if (ret < 0)
		return -EINVAL;

	ret = nx2a4wp_read_reg(client, NX2A4WP_ADC0DU_ADDR);
	if (ret < 0)
		return -EINVAL;

	vrect_raw = ret << 4;

	ret = nx2a4wp_read_reg(client, NX2A4WP_ADC1_ADC0_DL_ADDR);

	if (ret < 0)
		return -EINVAL;

	vrect_raw |= (ret & 0xF);

	*vrect_val = VRECT_ADC12_TO_MV(vrect_raw);
	return 0;
}

static int nx2a4wp_get_wpr_vdcout(struct i2c_client *client, int *val)
{
	int ret;
	int vdcout;
	u16 vdcout_raw;
	ret = nx2a4wp_write_reg(client, NX2A4WP_ADCEL_ADDR,
			(NX2A4WP_ADC_SWEEP_EN | NX2A4WP_VDCOUT_EN));
	if (ret < 0)
		return -EINVAL;

	ret = nx2a4wp_read_reg(client, NX2A4WP_ADC2DU_ADDR);
	if (ret < 0)
		return -EINVAL;

	vdcout_raw = ret << 4;

	ret = nx2a4wp_read_reg(client, NX2A4WP_ADC3_ADC2_DL_ADDR);
	if (ret < 0)
		return -EINVAL;

	vdcout_raw |= (ret & 0xF);

	ret = adc_lookup(vdcout_raw, VDCOUT_TABLE_SIZE, vdcout_adc_code,
				&vdcout);

	if (ret < 0)
		return ret;

	*val = vdcout;

	return 0;
}

static int nx2a4wp_set_dcsel_volt(struct i2c_client *client, int volt)
{
	u8 regval;

	if (volt < MIN_DCSEL_VOLT)
		return -EINVAL;

	volt = clamp_t(int, volt, MIN_DCSEL_VOLT, MAX_DCSEL_VOLT);

	regval = (u8)(((volt - MIN_DCSEL_VOLT)/DCSEL_VOLT_OFFSET) << 3);

	return nx2a4wp_read_modify_reg(client, NX2A4WP_DCSEL_ADDR,
		NX2A4WP_DCSEL_VOLT_MASK, regval);
}

static int nx2a4wp_set_wpr_dcen(struct i2c_client *client, bool enable)
{
	if (enable)
		return nx2a4wp_read_modify_reg(client, NX2A4WP_SYSCONU_ADDR,
			NX2A4WP_SYSCONU_DCEN_MASK, 1);
	else
		return nx2a4wp_read_modify_reg(client, NX2A4WP_SYSCONU_ADDR,
			NX2A4WP_SYSCONU_DCEN_MASK, 0);
}

static int nx2a4wp_set_dcsel_cur(struct i2c_client *client, int cur)
{
	u8 regval;

	if (cur < MIN_DCSEL_CUR)
		return -EINVAL;

	cur = clamp_t(int, cur,  MIN_DCSEL_CUR, MAX_DCSEL_CUR);

	regval = (cur - MIN_DCSEL_CUR)/DCSEL_CUR_OFFSET;

	return nx2a4wp_read_modify_reg(client, NX2A4WP_DCSEL_ADDR,
		NX2A4WP_DCSEL_CUR_MASK, regval);
}

static int nx2a4wp_init(struct nx2a4wp_context *chip)
{
	int ret;

	ret = nx2a4wp_read_reg(chip->client, NX2A4WP_DEV_ID_ADDR);
	if (ret < 0) {
		dev_err(&chip->client->dev,
			"Error (%d) in reading NX2A4WP_DEV_ID_ADDR\n", ret);
		return ret;
	}
	if (ret != NX2A4WP_ID) {
		dev_err(&chip->client->dev,
			"Invalid Vendor/Rev number in NX2A4WP_DEV_ID_ADDR %d",
			ret);
		return -ENODEV;
	}

	/* Write syscon registers */
	ret = nx2a4wp_write_reg(chip->client, NX2A4WP_SYSCONL_ADDR,
			NX2A4WP_SYSCONL_DCTOE_EN
			| NX2A4WP_SYSCONL_GPNTE_EN
			| NX2A4WP_SYSCONL_DCNTE_EN
			| NX2A4WP_SYSCONL_DCOVE_EN);
	if (ret >= 0)
		ret = nx2a4wp_read_modify_reg(chip->client,
				NX2A4WP_SYSCONU_ADDR, NX2A4WP_WDT_MASK, 0);
	if (ret < 0)
		dev_err(&chip->client->dev,
			"Error in disabling WDT TIMER %d", ret);

	if (ret < 0)
		dev_err(&chip->client->dev,
			"Error when writting NX2A4WP_SYSCONL/U registers\n");

	/* Interrupt is inactive - clear all status registers */
	ret = nx2a4wp_read_modify_reg(chip->client,
			NX2A4WP_SFCLRL_ADDR, 0xFF, 0xFF);
	if (ret >= 0)
		ret = nx2a4wp_read_modify_reg(chip->client,
				NX2A4WP_SFCLRU_ADDR, 0xFF, 0xFF);

	if (ret < 0)
		dev_err(&chip->client->dev, "error in clearing irq\n");

	/* Initialize health parameter */
	chip->health = POWER_SUPPLY_HEALTH_GOOD;

	ret = nx2a4wp_write_reg(chip->client, NX2A4WP_ICON_ADDR, 0xFF);
	if (ret < 0)
		dev_err(&chip->client->dev, "Error in enabling the IRQ\n");

	return ret;
}

/* handle the present event.
 * stat_lock mutex has to be hold before calling this function
 */
static void nx2a4wp_evt_present_lock(struct nx2a4wp_context *chip)
{
	dev_dbg(&chip->client->dev, "nx2a4wp_evt_present_lock\n");

	set_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags);
	chip->health = POWER_SUPPLY_HEALTH_GOOD;
	chip->status = POWER_SUPPLY_STATUS_PRU_NULL;

	cancel_delayed_work_sync(&chip->detach_wrkr);

	if (!test_bit(A4WP_FLAGS_ATTACH_BIT, &chip->flags))
		schedule_delayed_work(&chip->attach_wrkr,
				A4WP_LONG_BEACON_ON_PERIOD);
#if (FILTER_SHORT_BEACON == 0)
	power_supply_changed(&chip->psy_wc);
#endif
}

/* handle the nopresent event.
 * stat_lock mutex has to be hold before calling this function
 */
static void nx2a4wp_evt_nopresent_lock(struct nx2a4wp_context *chip)
{
	dev_dbg(&chip->client->dev, "nx2a4wp_evt_nopresent_lock\n");

	clear_bit(A4WP_FLAGS_PRESENT_BIT,  &chip->flags);
	clear_bit(A4WP_FLAGS_PRU_DCEN_BIT, &chip->flags);

	chip->health = POWER_SUPPLY_HEALTH_UNKNOWN;
	chip->status = POWER_SUPPLY_STATUS_PRU_NULL;
	chip->irect = 0;
	chip->vrect = 0;
	chip->vdcout = 0;
	chip->idcout = 0;

	cancel_delayed_work_sync(&chip->attach_wrkr);

	if (!test_bit(A4WP_FLAGS_MAINTENANCE_BIT, &chip->flags)
			&& test_bit(A4WP_FLAGS_ATTACH_BIT, &chip->flags))
		schedule_delayed_work(&chip->detach_wrkr,
				A4WP_LONG_BEACON_MAX_PERIOD);

	power_supply_changed(&chip->psy_wc);
}

static int nx2a4wp_hide_present(struct wireless_pru *pru,
		bool hide, bool maintenance)
{
	int ret = 0;
	struct nx2a4wp_context *chip = container_of(pru,
					struct nx2a4wp_context, rez);

	mutex_lock(&chip->stat_lock);

	if (maintenance)
		set_bit(A4WP_FLAGS_MAINTENANCE_BIT, &chip->flags);
	else
		clear_bit(A4WP_FLAGS_MAINTENANCE_BIT, &chip->flags);

	if (hide) {
		dev_dbg(&chip->client->dev, "nx2a4wp_hide_present - Hide present\n");
		if (chip->client->irq)
			disable_irq_nosync(chip->client->irq);
		if (test_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags))
			nx2a4wp_evt_nopresent_lock(chip);
	} else {
		dev_dbg(&chip->client->dev, "nx2a4wp_hide_present - show present\n");
		if (chip->client->irq)
			enable_irq(chip->client->irq);

	}

	mutex_unlock(&chip->stat_lock);

	return ret;
}

static int nx2a4wp_do_measurements(struct wireless_pru *pru)
{
	int ret;
	struct nx2a4wp_context *chip = container_of(pru,
					struct nx2a4wp_context, rez);

	mutex_lock(&chip->stat_lock);

	if (test_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags)) {

		ret = nx2a4wp_get_wpr_irect(chip->client, &chip->irect);
		if (ret < 0)
			goto err;

		ret = nx2a4wp_get_wpr_vrect(chip->client, &chip->vrect);
		if (ret < 0)
			goto err;

		ret = nx2a4wp_get_wpr_vdcout(chip->client, &chip->vdcout);
		if (ret < 0)
			goto err;

		ret = nx2a4wp_get_wpr_idcout(chip->client, &chip->idcout);
		if (ret < 0)
			goto err;

		chip->pru_temp = nx2a4wp_get_wpr_temp(chip->client);
	}

	mutex_unlock(&chip->stat_lock);
	return 0;

err:
	dev_err(&chip->client->dev, "Fail to do measurement. Lost present?\n");
	nx2a4wp_evt_nopresent_lock(chip);

	mutex_unlock(&chip->stat_lock);
	return ret;
}

static int nx2a4wp_usb_set_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    const union power_supply_propval *val)
{
	struct nx2a4wp_context *chip = container_of(psy, struct nx2a4wp_context,
						    psy_wc);
	bool notify = false;
	int ret = 0, temp = 0;

	mutex_lock(&chip->stat_lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (val->intval <= POWER_SUPPLY_STATUS_PRU_ERROR &&
				val->intval >=  POWER_SUPPLY_STATUS_PRU_NULL) {
			if (test_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags)) {
				chip->status = val->intval;
				notify = true;
			}
		} else
			ret = -EINVAL;
		break;
	case POWER_SUPPLY_PROP_PRU_DYNAMIC_PARAM:
		memcpy(chip->pru_dyn_param, val->strval,
					PSY_MAX_STR_PROP_LEN);
		break;
	case POWER_SUPPLY_PROP_PRU_STATIC_PARAM:
		memcpy(chip->pru_stat_param, val->strval,
			PSY_MAX_STR_PROP_LEN);
		break;
	case POWER_SUPPLY_PROP_PTU_STATIC_PARAM:
		memcpy(chip->ptu_stat_param, val->strval,
			PSY_MAX_STR_PROP_LEN);
		/* written by user space. Notify power supply subsystem
		 * so that rezence driver can decode the params
		 */
		notify = true;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		if (!test_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags)) {
			ret = -EAGAIN;
		} else {
			nx2a4wp_set_dcsel_volt(chip->client, val->intval);
			chip->volt_max = val->intval;
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		if (!test_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags)) {
			ret = -EAGAIN;
		} else {
			nx2a4wp_set_dcsel_cur(chip->client, val->intval);
			chip->current_max = val->intval;
		}
		break;
	case POWER_SUPPLY_PROP_PTU_POWER:
		temp = find_adc_code(val->intval,
			PTU_POWER_TABLE_SIZE, ptu_power_code);
		chip->ptu_power = ptu_power_code[temp][1];
		break;
	case POWER_SUPPLY_PROP_PTU_CLASS:
		chip->ptu_class = val->intval;
		break;
	case POWER_SUPPLY_PROP_PRU_DCEN:
		if (!test_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags)) {
			ret = -EAGAIN;
		} else {
			ret = nx2a4wp_set_wpr_dcen(chip->client, val->intval);
			if (ret >= 0) {
				if (val->intval) {
					set_bit(A4WP_FLAGS_PRU_DCEN_BIT,
							&chip->flags);
					schedule_work(&chip->pruon_wrkr);
				} else {
					clear_bit(A4WP_FLAGS_PRU_DCEN_BIT,
							&chip->flags);
				}
			}
		}
		break;
	default:
		ret = -ENODATA;
	}

	mutex_unlock(&chip->stat_lock);

	if (notify)
		power_supply_changed(&chip->psy_wc);

	return ret;
}

static int nx2a4wp_usb_get_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	struct nx2a4wp_context *chip = container_of(psy, struct nx2a4wp_context,
						    psy_wc);
	int ret = 0;

	mutex_lock(&chip->stat_lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = chip->health;
		break;
	case POWER_SUPPLY_PROP_PRU_DYNAMIC_PARAM:
		val->strval = chip->pru_dyn_param;
		break;
	case POWER_SUPPLY_PROP_PRU_STATIC_PARAM:
		val->strval = chip->pru_stat_param;
		break;
	case POWER_SUPPLY_PROP_PTU_STATIC_PARAM:
		val->strval = chip->ptu_stat_param;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		if (test_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags))
			val->intval = 1;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = chip->status;
		break;
	case POWER_SUPPLY_PROP_VRECT:
		val->intval = chip->vrect;
		break;
	case POWER_SUPPLY_PROP_IRECT:
		val->intval = chip->irect;
		break;
	case POWER_SUPPLY_PROP_VDCOUT:
		val->intval = chip->vdcout;
		break;
	case POWER_SUPPLY_PROP_IDCOUT:
		val->intval = chip->idcout;
		break;
	case POWER_SUPPLY_PROP_PRU_TEMP:
		val->intval = chip->pru_temp;
		break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = "nx2a4wp";
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = "NXP";
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = chip->volt_max;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = chip->current_max;
		break;
	case POWER_SUPPLY_PROP_PTU_POWER:
		val->intval = chip->ptu_power;
		break;
	case POWER_SUPPLY_PROP_PTU_CLASS:
		val->intval = chip->ptu_class;
		break;
	case POWER_SUPPLY_PROP_PRU_DCEN:
		if (test_bit(A4WP_FLAGS_PRU_DCEN_BIT, &chip->flags))
			val->intval = 1;
		else
			val->intval = 0;
		break;
	default:
		ret = -ENODATA;
		break;
	}

	mutex_unlock(&chip->stat_lock);

	return ret;
}

static int nx2a4wp_usb_property_is_writeable(struct power_supply *psy,
					     enum power_supply_property psp)
{
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_PTU_STATIC_PARAM:
	case POWER_SUPPLY_PROP_STATUS:
		ret = 1;
		break;
	default:
		ret = 0;
	}

	return ret;
}

static irqreturn_t nx2a4wp_irq_primary_handler(int id, void *data)
{
	struct nx2a4wp_context *chip = (struct nx2a4wp_context *) data;

	chip->irq_timestamp = jiffies;

	return IRQ_WAKE_THREAD;
}

static irqreturn_t nx2a4wp_thread_handler(int id, void *data)
{
	struct nx2a4wp_context *chip = (struct nx2a4wp_context *) data;

	mutex_lock(&chip->stat_lock);

	/* Rising Edge */
	if (gpiod_get_value(chip->irq_gpio)) {
		/* A4WP Present=1 event*/
		if (!test_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags)) {
			chip->present_timestamp = chip->irq_timestamp;
			nx2a4wp_evt_present_lock(chip);
		}
		/* Health event */
		else {
			int ret;

			/* No more Interrupt */
			ret = nx2a4wp_read_modify_reg(chip->client,
					NX2A4WP_SFCLRL_ADDR, 0xFF, 0xFF);
			if (ret >= 0)
				ret = nx2a4wp_read_modify_reg(chip->client,
						NX2A4WP_SFCLRU_ADDR,
						0xFF, 0xFF);

			if (ret < 0) {
				dev_err(&chip->client->dev, "ERROR IN CLEARING IRQ\n");
				goto err;
			}
			if (chip->health != POWER_SUPPLY_HEALTH_GOOD) {
				chip->health = POWER_SUPPLY_HEALTH_GOOD;
				power_supply_changed(&chip->psy_wc);
			}
		}
	}

	/* Falling Edge */
	else {
		int ret1, ret2;

		ret1 = nx2a4wp_read_reg(chip->client, NX2A4WP_STATL_ADDR);
		if (ret1 >= 0)
			ret2 = nx2a4wp_read_reg(chip->client,
					NX2A4WP_STATU_ADDR);

		/* A4WP present=0 event */
		if ((ret1 < 0) || (ret2 < 0)) {
			if (test_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags))
				nx2a4wp_evt_nopresent_lock(chip);
		}

		/* Health event */
		else {
			unsigned int health;
			if (ret2 & NX2A4WP_OVP_MASK)
				health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
			else if (ret1 & NX2A4WP_OTP_MASK)
				health = POWER_SUPPLY_HEALTH_OVERHEAT;
			else if (ret1 & NX2A4WP_IDC_MASK)
				health = POWER_SUPPLY_HEALTH_OVERCURRENT;
			else
				health = POWER_SUPPLY_HEALTH_GOOD;

			if (health != chip->health) {
				chip->health = health;
				power_supply_changed(&chip->psy_wc);
			}
		}
	}
	mutex_unlock(&chip->stat_lock);

	return IRQ_HANDLED;

err:
	chip->health = POWER_SUPPLY_HEALTH_UNKNOWN;
	mutex_unlock(&chip->stat_lock);
	return IRQ_NONE;
}

static void attach_worker(struct work_struct *work)
{
	struct nx2a4wp_context *chip = container_of(work,
					struct nx2a4wp_context,
					attach_wrkr.work);
	int ret;

	dev_dbg(&chip->client->dev, "attach_worker\n");

#if (FILTER_SHORT_BEACON == 1)
	/* The function has to be called prior calling the
	 * atomic_notifier_call_chain and nx2a4wp_init as
	 * these later are time consuming.
	 */
	power_supply_changed(&chip->psy_wc);
#endif

	ret = nx2a4wp_init(chip);
	if (ret < 0) {
		dev_err(&chip->client->dev,
				"Fail to initialize! ret=%d\n", ret);
	} else {
		set_bit(A4WP_FLAGS_ATTACH_BIT, &chip->flags);
		chip->cable.chrg_evt = POWER_SUPPLY_CHARGER_EVENT_ATTACH;
		atomic_notifier_call_chain(&power_supply_notifier,
				PSY_CABLE_EVENT, &chip->cable);
	}
}

static void detach_worker(struct work_struct *work)
{
	struct nx2a4wp_context *chip = container_of(work,
					struct nx2a4wp_context,
					detach_wrkr.work);
	dev_dbg(&chip->client->dev, "dettach_worker\n");

	clear_bit(A4WP_FLAGS_ATTACH_BIT, &chip->flags);
	chip->cable.chrg_evt = POWER_SUPPLY_CHARGER_EVENT_DETACH;
	atomic_notifier_call_chain(&power_supply_notifier,
			PSY_CABLE_EVENT, &chip->cable);
}

/* this worker is WA for h/w bug: There is no connection exist between
PMIC VDCIN to WCHRG_IN. This code is part of PMIC driver */
static void a4wp_pruon_worker(struct work_struct *work)
{
	struct nx2a4wp_context *chip = container_of(work,
					struct nx2a4wp_context,
					pruon_wrkr);
	if (chip->cable.chrg_evt != POWER_SUPPLY_CHARGER_EVENT_CONNECT) {
		chip->cable.chrg_evt =
			POWER_SUPPLY_CHARGER_EVENT_CONNECT;
		atomic_notifier_call_chain(&power_supply_notifier,
		PSY_CABLE_EVENT, &chip->cable);
	}
}

#ifdef CONFIG_DEBUG_FS
static int a4wp_debugfs_timing_show(struct seq_file *seq, void *unused)
{
	struct nx2a4wp_context *chip = (struct nx2a4wp_context *)seq->private;
	unsigned long delay = (chip->present2adv_time * 1000) / HZ;

	seq_printf(seq, "present -> advertising:\t%lums\n", delay);
	return 0;
}

static int a4wp_debugfs_timing_open(struct inode *inode, struct file *file)
{
	return single_open(file, a4wp_debugfs_timing_show, inode->i_private);
}

static const struct file_operations a4wp_debugfs_timing_fops = {
	.open = a4wp_debugfs_timing_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static void a4wp_debugfs_init(struct nx2a4wp_context *chip)
{
	struct dentry *fentry;
	struct dentry *a4wp_dbg_dir;

	/* Creating a directory under debug fs for charger */
	a4wp_dbg_dir = debugfs_create_dir(DEV_NAME , NULL);
	if (a4wp_dbg_dir == NULL)
		goto debugfs_root_exit;

	fentry = debugfs_create_file("timing",
				S_IRUGO,
				a4wp_dbg_dir,
				chip,
				&a4wp_debugfs_timing_fops);

	if (fentry == NULL)
		goto debugfs_err_exit;

	dev_info(&chip->client->dev, "Debugfs created successfully!!");
	return;

debugfs_err_exit:
	debugfs_remove_recursive(a4wp_dbg_dir);
debugfs_root_exit:
	dev_err(&chip->client->dev, "Error creating debugfs entry!!");
	return;
}
#endif

static int nx2a4wp_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct nx2a4wp_context *chip;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev,
			"I2C adapter %s doesn't support BYTE DATA transfer\n",
			adapter->name);
		return -EIO;
	}

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;


	if (ACPI_HANDLE(&client->dev)) {
		chip->irq_gpio = devm_gpiod_get_index(&client->dev,
				"a4wp_irq", 0);
		if (!IS_ERR(chip->irq_gpio)) {
			ret = gpiod_direction_input(chip->irq_gpio);
			if (ret < 0)
				return ret;
			client->irq = gpiod_to_irq(chip->irq_gpio);
			dev_info(&client->dev, "a4wp_irq=%d\n", client->irq);
		}
	}

	i2c_set_clientdata(client, chip);
	chip->client = client;
	chip->flags = 0;

	chip->psy_wc.name = DEV_NAME;
	chip->psy_wc.type = POWER_SUPPLY_TYPE_WIRELESS;
	chip->psy_wc.supported_cables = POWER_SUPPLY_TYPE_WIRELESS;
	chip->psy_wc.properties = nx2a4wp_usb_props;
	chip->psy_wc.num_properties = ARRAY_SIZE(nx2a4wp_usb_props);
	chip->psy_wc.get_property = nx2a4wp_usb_get_property;
	chip->psy_wc.set_property = nx2a4wp_usb_set_property;
	chip->psy_wc.property_is_writeable = nx2a4wp_usb_property_is_writeable;
	chip->cable.chrg_type = POWER_SUPPLY_CHARGER_TYPE_WIRELESS;
	chip->cable.chrg_evt = POWER_SUPPLY_CHARGER_EVENT_DETACH;
	chip->cable.ma = PRU_MAX_MA;
	chip->status = POWER_SUPPLY_STATUS_PRU_NULL;
	chip->health = POWER_SUPPLY_HEALTH_UNKNOWN;
	chip->vrect = 0;
	chip->irect = 0;

	/* Check current interruot state */
	if (chip->irq_gpio && gpiod_get_value(chip->irq_gpio))
		set_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags);

	INIT_WORK(&chip->pruon_wrkr, a4wp_pruon_worker);
	INIT_DELAYED_WORK(&chip->detach_wrkr, detach_worker);
	INIT_DELAYED_WORK(&chip->attach_wrkr, attach_worker);

	mutex_init(&chip->stat_lock);

	ret = power_supply_register(&client->dev, &chip->psy_wc);
	if (ret) {
		dev_err(&client->dev, "Failed: power supply register (%d)\n",
			ret);
		return ret;
	}

	if (client->irq) {
		ret = devm_request_threaded_irq(&client->dev, client->irq,
				nx2a4wp_irq_primary_handler,
				nx2a4wp_thread_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				DEV_NAME, chip);
		if (ret) {
			dev_err(&client->dev, "Failed: request_irq (%d)\n",
				ret);
			power_supply_unregister(&chip->psy_wc);
			return ret;
		}
	}

	chip->rez.do_measurements = nx2a4wp_do_measurements;
	chip->rez.hide_present = nx2a4wp_hide_present;
	chip->rez.psy_rez = &chip->psy_wc;

	ret = wireless_rezence_register(&chip->rez);
	if (ret) {
		dev_err(&client->dev,
			"Failed: wireless rezence register (%d)\n",
			ret);
		return ret;
	}

	/* Create debugfs */
#ifdef CONFIG_DEBUG_FS
	a4wp_debugfs_init(chip);
#endif

	/* Check whether the NX2A4WP chis is currently powered */
	if (test_bit(A4WP_FLAGS_PRESENT_BIT, &chip->flags)) {
		schedule_delayed_work(&chip->attach_wrkr,
				A4WP_LONG_BEACON_ON_PERIOD);
		power_supply_changed(&chip->psy_wc);
	}

	return 0;
}

static int nx2a4wp_remove(struct i2c_client *client)
{
	struct nx2a4wp_context *chip = i2c_get_clientdata(client);

	if (client->irq)
		devm_free_irq(&client->dev, client->irq, chip);

	power_supply_unregister(&chip->psy_wc);

	return 0;
}

static int nx2a4wp_suspend(struct device *dev)
{
	return 0;
}

static int nx2a4wp_resume(struct device *dev)
{
	return 0;
}

static int nx2a4wp_runtime_suspend(struct device *dev)
{
	return 0;
}

static int nx2a4wp_runtime_resume(struct device *dev)
{
	return 0;
}

static int nx2a4wp_runtime_idle(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops nx2a4wp_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(nx2a4wp_suspend,
			nx2a4wp_resume)
	SET_RUNTIME_PM_OPS(nx2a4wp_runtime_suspend,
			nx2a4wp_runtime_resume,
			nx2a4wp_runtime_idle)
};

#ifdef CONFIG_ACPI
static struct acpi_device_id nx2a4wp_acpi_match[] = {
	{DEV_NAME, 0},
};
MODULE_DEVICE_TABLE(acpi, nx2a4wp_acpi_match);
#endif

static const struct i2c_device_id nx2a4wp_id[] = {
	{DEV_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, nx2a4wp_id);

static struct i2c_driver nx2a4wp_i2c_driver = {
	.driver = {
		   .name		= DRV_NAME,
#if defined(CONFIG_ACPI)
		   .acpi_match_table	= ACPI_PTR(nx2a4wp_acpi_match),
#endif
		   .pm			= &nx2a4wp_pm_ops,
	},
	.probe = nx2a4wp_probe,
	.remove = nx2a4wp_remove,
	.id_table = nx2a4wp_id,
};
module_i2c_driver(nx2a4wp_i2c_driver);

MODULE_AUTHOR("Jenny TC <jenny.tc@intel.com>");
MODULE_DESCRIPTION("NX2A4WP WPR Driver");
MODULE_LICENSE("GPL");
