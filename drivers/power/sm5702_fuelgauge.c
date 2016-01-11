/* drivers/battery/sm5702_fuelgauge.c
 * SM5702 Voltage Tracking Fuelgauge Driver
 *
 * Copyright (C) 2015 SiliconMitus
 * Author: 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */ 

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/acpi.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/power_supply.h>
//#include <linux/power/battery_id.h>
#include <linux/slab.h>
#include <linux/power/sm5702_fuelgauge.h>
#include <linux/power/sm5702_fuelgauge_impl.h>
#include <linux/power/sm5702_fuelgauge_init_data.h>

#include <linux/wakelock.h>

#include <linux/interrupt.h>
#include "power_supply.h"

#define BAT_LOW_INTERRUPT 0

#define SM5702FG_SLAVE_ADDR 0x71
static inline int sm5702_fg_read_device(struct i2c_client *client,
					  int reg, int bytes, void *dest)
{
	int ret;

	if (bytes > 1)
		ret = i2c_smbus_read_i2c_block_data(client, reg, bytes, dest);
	else {
		ret = i2c_smbus_read_byte_data(client, reg);
		if (ret < 0)
			return ret;
		*(unsigned char *)dest = (unsigned char)ret;
	}
	return ret;
}

static int32_t sm5702_fg_i2c_read_word(struct i2c_client *client,
						uint8_t reg_addr)
{
	uint16_t data = 0;
	int ret;
	ret = sm5702_fg_read_device(client, reg_addr, 2, &data);
	/*pr_err("%s: ret = %d, addr = 0x%x, data = 0x%x\n",
	__func__, ret, reg_addr, data);*/

	if (ret < 0)
		return ret;
	else
		return data;
}


static int32_t sm5702_fg_i2c_write_word(struct i2c_client *client,
uint8_t reg_addr, uint16_t data)
{
	int ret;

	/* not use big endian*/
	/*data = cpu_to_be16(data);*/
	ret = i2c_smbus_write_i2c_block_data(client, reg_addr, 2,
	(uint8_t *)&data);

	/*pr_err("%s: ret = %d, addr = 0x%x, data = 0x%x\n",
	__func__, ret, reg_addr, data);*/
	return ret;
}

static unsigned int fg_get_vbat(struct i2c_client *client);
static unsigned int fg_get_ocv(struct i2c_client *client);
static int fg_get_curr(struct i2c_client *client);
static int fg_get_temp(struct i2c_client *client);
static bool sm5702_fg_init(struct i2c_client *client);

static void sm5702_pr_ver_info(struct i2c_client *client)
{
	pr_err("sm5702 Fuel-Gauge Ver %s\n", FG_DRIVER_VER);
}

bool sm5702_hal_fg_reset(struct i2c_client *client)
{
	pr_err("%s: sm5702_hal_fg_reset\n", __func__);

	/* SW reset code*/
	sm5702_fg_i2c_write_word(client, 0x90, 0x0008);
	/* delay 200ms*/
	msleep(200);
	/* init code*/
	sm5702_fg_init(client);

	return true;
}

void fg_vbatocv_check(struct i2c_client *client, int ta_exist)
{
	int ret;
	struct sm5702_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);

	/*iocv error case cover start*/
	/*50mA under*/
	if ((ta_exist)
		&& (abs(fuelgauge->info.batt_current) < 50)
		&& (abs(fuelgauge->info.p_batt_current
			-fuelgauge->info.batt_current) < 10)) {
		/*30mV over*/
		if (abs(fuelgauge->info.batt_ocv
			-fuelgauge->info.batt_voltage) > 30)
			fuelgauge->info.iocv_error_count++;

		/*prevent to overflow*/
		if (fuelgauge->info.iocv_error_count > 5)
			fuelgauge->info.iocv_error_count = 6;
	} else {
		fuelgauge->info.iocv_error_count = 0;
	}

	pr_err("%s: iocv_error_count (%d)\n",
		__func__, fuelgauge->info.iocv_error_count);

	if (fuelgauge->info.iocv_error_count > 5) {
		pr_err("%s: p_v - v = (%d)\n",
			__func__, fuelgauge->info.p_batt_voltage
			- fuelgauge->info.batt_voltage);
		/*15mV over*/
		if (abs(fuelgauge->info.p_batt_voltage
			- fuelgauge->info.batt_voltage) > 15) {
			fuelgauge->info.iocv_error_count = 0;
		} else {
			/*mode change to mix RS manual mode*/
			pr_err("%s: mode change to mix RS manual mode\n",
			__func__);
			/*RS manual value write*/
			sm5702_fg_i2c_write_word(client,
				sm5702_REG_RS_MAN, fuelgauge->info.rs_value[0]);
			/*run update*/
			sm5702_fg_i2c_write_word(client,
				sm5702_REG_PARAM_RUN_UPDATE, 0);
			sm5702_fg_i2c_write_word(client,
				sm5702_REG_PARAM_RUN_UPDATE, 1);
			/*mode change*/
			ret = sm5702_fg_i2c_read_word(client, sm5702_REG_CNTL);
			/*+RS_MAN_MODE*/
			ret = (ret | ENABLE_MIX_MODE) | ENABLE_RS_MAN_MODE;
			sm5702_fg_i2c_write_word(client, sm5702_REG_CNTL, ret);
		}
	} else {
		if ((fuelgauge->info.p_batt_voltage < 3400)
			&& (fuelgauge->info.batt_voltage < 3400)
			&& (!ta_exist)) {
			/*mode change*/
			ret = sm5702_fg_i2c_read_word(client, sm5702_REG_CNTL);
			/*+RS_MAN_MODE*/
			ret = (ret | ENABLE_MIX_MODE) | ENABLE_RS_MAN_MODE;
			sm5702_fg_i2c_write_word(client, sm5702_REG_CNTL, ret);
		} else {
			pr_err("%s: mode change to mix RS auto mode\n",
				__func__);

			/*mode change to mix RS auto mode*/
			ret = sm5702_fg_i2c_read_word(client, sm5702_REG_CNTL);
			/*-RS_MAN_MODE*/
			ret = (ret | ENABLE_MIX_MODE) & ~ENABLE_RS_MAN_MODE;
			sm5702_fg_i2c_write_word(client, sm5702_REG_CNTL, ret);
		}

	}
	fuelgauge->info.p_batt_voltage = fuelgauge->info.batt_voltage;
	fuelgauge->info.p_batt_current = fuelgauge->info.batt_current;
	/*iocv error case cover end*/
}

static void sm5703_fg_test_read(struct i2c_client *client)
{
	int ret1, ret2, ret3, ret4;

	ret1 = sm5702_fg_i2c_read_word(client, 0xAC);
	ret2 = sm5702_fg_i2c_read_word(client, 0xAD);
	ret3 = sm5702_fg_i2c_read_word(client, 0xAE);
	ret4 = sm5702_fg_i2c_read_word(client, 0xAF);

	pr_err("0xAC=0x%04x, 0xAD=0x%04x, 0xAE=0x%04x, 0xAF=0x%04x\n",
		ret1, ret2, ret3, ret4);
	ret1 = sm5702_fg_i2c_read_word(client, 0xBC);
	ret2 = sm5702_fg_i2c_read_word(client, 0xBD);
	ret3 = sm5702_fg_i2c_read_word(client, 0xBE);
	ret4 = sm5702_fg_i2c_read_word(client, 0xBF);

	pr_err("0xBC=0x%04x, 0xBD=0x%04x, 0xBE=0x%04x, 0xBF=0x%04x\n",
		ret1, ret2, ret3, ret4);
	ret1 = sm5702_fg_i2c_read_word(client, 0x85);
	ret2 = sm5702_fg_i2c_read_word(client, 0x86);
	ret3 = sm5702_fg_i2c_read_word(client, 0x87);
	ret4 = sm5702_fg_i2c_read_word(client, 0x01);

	pr_err("0x85=0x%04x, 0x86=0x%04x, 0x87=0x%04x, 0x01=0x%04x\n",
		ret1, ret2, ret3, ret4);
}

unsigned int fg_get_soc(struct i2c_client *client)
{
	int ret = 0;
	unsigned int soc;
	int ta_exist;
	int curr_cal;
	int temp_cal_fact;
	struct sm5702_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);

	ta_exist = (fuelgauge->info.batt_current > 9) ? true : false
	| fuelgauge->is_charging;
	pr_err("%s: is_charging = %d, ta_exist = %d\n",
		__func__, fuelgauge->is_charging, ta_exist);

	fg_vbatocv_check(fuelgauge->client, ta_exist);

	if (ta_exist)
		curr_cal = fuelgauge->info.curr_cal
		+(fuelgauge->info.charge_offset_cal << 8);
	else
		curr_cal = fuelgauge->info.curr_cal;
	pr_err("%s: curr_cal = 0x%x\n", __func__, curr_cal);

	temp_cal_fact = fuelgauge->info.temp_std
		- (fuelgauge->info.temperature / 10);
	temp_cal_fact = temp_cal_fact / fuelgauge->info.temp_offset;
	temp_cal_fact = temp_cal_fact * fuelgauge->info.temp_offset_cal;
	curr_cal = curr_cal + (temp_cal_fact << 8);
	pr_err("%s: temp_std = %d, temperature = %d, temp_offset = %d, temp_offset_cal = 0x%x, curr_cal = 0x%x\n",
		__func__, fuelgauge->info.temp_std, fuelgauge->info.temperature,
		fuelgauge->info.temp_offset,
		fuelgauge->info.temp_offset_cal, curr_cal);

	sm5702_fg_i2c_write_word(client, sm5702_REG_CURR_CAL, curr_cal);

	ret = sm5702_fg_i2c_read_word(client, sm5702_REG_SOC);
	if (ret < 0) {
		pr_err("%s: read soc reg fail\n", __func__);
		soc = 500;
	} else {
		/*integer bit;*/
		soc = ((ret&0xff00)>>8) * 10;
		/* integer + fractional bit*/
		soc = soc + (((ret&0x00ff)*10)/256);
	}

	/*update power_supply_changed*/ 
	if(soc/10 != fuelgauge->info.batt_soc/10) { 
	    power_supply_changed(&fuelgauge->psy_fg); 
	} 

	fuelgauge->info.batt_soc = soc;
	pr_err("%s: read = 0x%x, soc = %d\n", __func__, ret, soc);

	/* abnormal case.... SW reset*/
	ret = sm5702_fg_i2c_read_word(client, sm5702_REG_CNTL);
	pr_err("%s : sm5702_REG_CNTL VALUE = 0x%x\n", __func__, ret);
	if (ret == ENABLE_RE_INIT) {
		/* SW reset code*/
		pr_err("%s : sm5702 FG abnormal case... SW reset\n", __func__);
		sm5702_hal_fg_reset(client);
	}

	/*for Test Log*/
	sm5703_fg_test_read(client);

	return soc;
}

static unsigned int fg_get_ocv(struct i2c_client *client)
{
	int ret;
	unsigned int ocv;/* = 3500; 3500 means 3500mV*/
	struct sm5702_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);

	ret = sm5702_fg_i2c_read_word(client, sm5702_REG_OCV);
	if (ret < 0) {
		pr_err("%s: read ocv reg fail\n", __func__);
		ocv = 4000;
	} else {
		/*integer bit;*/
		ocv = ((ret&0x0700)>>8) * 1000;
		/* integer + fractional bit*/
		ocv = ocv + (((ret&0x00ff)*1000)/256);
	}

	fuelgauge->info.batt_ocv = ocv;
	pr_err("%s: read = 0x%x, ocv = %d\n", __func__, ret, ocv);


	return ocv;
}

static unsigned int fg_get_vbat(struct i2c_client *client)
{
	int ret;
	struct sm5702_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);

	unsigned int vbat;/* = 3500; 3500 means 3500mV*/
	ret = sm5702_fg_i2c_read_word(client, sm5702_REG_VOLTAGE);
	if (ret < 0) {
		pr_err("%s: read vbat reg fail", __func__);
		vbat = 4000;
	} else {
		/*integer bit*/
		vbat = ((ret&0x0700)>>8) * 1000;
		/* integer + fractional bit*/
		vbat = vbat + (((ret&0x00ff)*1000)/256);
	}
	fuelgauge->info.batt_voltage = vbat;

	/*cal avgvoltage*/
	fuelgauge->info.batt_avgvoltage =
	(((fuelgauge->info.batt_avgvoltage)*4) + vbat)/5;

	pr_err("%s: read = 0x%x, vbat = %d\n", __func__, ret, vbat);
	pr_err("%s: batt_avgvoltage = %d\n",
		__func__, fuelgauge->info.batt_avgvoltage);

	return vbat;
}

static int fg_get_curr(struct i2c_client *client)
{
	int ret;
	struct sm5702_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);

	int curr;/* = 1000; 1000 means 1000mA*/
	ret = sm5702_fg_i2c_read_word(client, sm5702_REG_CURRENT);
	if (ret < 0) {
		pr_err("%s: read curr reg fail", __func__);
		curr = 0;
	} else {
		/*integer bit*/
		curr = ((ret&0x0700)>>8) * 1000;
		/* integer + fractional bit*/
		curr = curr + (((ret&0x00ff)*1000)/256);
		if (ret&0x8000)
			curr *= -1;
	}
	fuelgauge->info.batt_current = curr;

	/*cal avgcurr*/
	fuelgauge->info.batt_avgcurrent =
	(((fuelgauge->info.batt_avgcurrent)*4) + curr)/5;

	pr_err("%s: read = 0x%x, curr = %d\n", __func__, ret, curr);
	pr_err("%s: batt_avgcurrent = %d\n",
		__func__, fuelgauge->info.batt_avgcurrent);

	return curr;
}

static int fg_get_temp(struct i2c_client *client)
{
	int ret;
	struct sm5702_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);

	int temp;/* = 250; 250 means 25.0oC*/
	ret = sm5702_fg_i2c_read_word(client, sm5702_REG_TEMPERATURE);
	if (ret < 0) {
		pr_err("%s: read temp reg fail", __func__);
		temp = 0;
	} else {
		 /*integer bit*/
		temp = ((ret&0x7F00)>>8) * 10;
		 /* integer + fractional bit*/
		temp = temp + (((ret&0x00ff)*10)/256);
		if (ret&0x8000)
			temp *= -1;
	}
	fuelgauge->info.temperature = temp;

	pr_err("%s: read = 0x%x, temperature = %d\n", __func__, ret, temp);

	return temp;
}



static unsigned int fg_get_device_id(struct i2c_client *client)
{
	int ret;
	ret = sm5702_fg_i2c_read_word(client, sm5702_REG_DEVICE_ID);
	/*ret &= 0x00ff;*/
	pr_err("%s: device_id = 0x%x\n", __func__, ret);

	return ret;
}




static void fg_get_online(struct i2c_client *client)
{
	/*Unfixed*/
	 struct sm5702_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);

	if ((fuelgauge->info.batt_current > 10) ? true : false)
		fuelgauge->info.online = true;
	else
		fuelgauge->info.online = false;
	pr_err("%s: online = 0x%x\n", __func__, fuelgauge->info.online);
}

static bool sm5702_fg_check_reg_init_need(struct i2c_client *client)
{

	int ret;

	ret = sm5702_fg_i2c_read_word(client, sm5702_REG_CNTL);

	if (ret == ENABLE_RE_INIT) {
		pr_err("%s: return 1\n", __func__);
		return 1;
	} else {
		pr_err("%s: return 0\n", __func__);
		return 0;
	}

	return 1;
}

static bool sm5702_fg_reg_init(struct i2c_client *client)
{
	int i, j, value, ret;
	uint8_t table_reg;
	struct sm5702_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);

	pr_err("%s: sm5702_fg_reg_init START!!\n", __func__);

	/*start first param_ctrl unlock*/
	sm5702_fg_i2c_write_word(client,
		sm5702_REG_PARAM_CTRL, sm5702_FG_PARAM_UNLOCK_CODE);

	/* RCE write*/
	for (i = 0; i < 3; i++)	{
		sm5702_fg_i2c_write_word(client,
			sm5702_REG_RCE0+i, fuelgauge->info.rce_value[i]);
		pr_err("%s: RCE write RCE%d = 0x%x : 0x%x\n",
			__func__,  i, sm5702_REG_RCE0+i,
			fuelgauge->info.rce_value[i]);
	}

	/* DTCD write*/
	sm5702_fg_i2c_write_word(client,
		sm5702_REG_DTCD, fuelgauge->info.dtcd_value);
	pr_err("%s: DTCD write DTCD = 0x%x : 0x%x\n",
		__func__, sm5702_REG_DTCD, fuelgauge->info.dtcd_value);

	/* RS write*/
	sm5702_fg_i2c_write_word(client,
		sm5702_REG_RS, fuelgauge->info.rs_value[0]);
	pr_err("%s: RS write RS = 0x%x : 0x%x\n",
		__func__, sm5702_REG_RS, fuelgauge->info.rs_value[0]);


	/*/ VIT_PERIOD write*/
	sm5702_fg_i2c_write_word(client,
		sm5702_REG_VIT_PERIOD, fuelgauge->info.vit_period);
	pr_err("%s: VIT_PERIOD write VIT_PERIOD = 0x%x : 0x%x\n",
		__func__, sm5702_REG_VIT_PERIOD, fuelgauge->info.vit_period);

	/*/ TABLE_LEN write & pram unlock*/
	sm5702_fg_i2c_write_word(client,
		sm5702_REG_PARAM_CTRL,
		sm5702_FG_PARAM_UNLOCK_CODE |
		sm5702_FG_TABLE_LEN);

	for (i = 0; i < 2; i++) {
		table_reg = sm5702_REG_TABLE_START + (i<<4);
		for (j = 0; j < sm5702_FG_TABLE_LEN; j++) {
			sm5702_fg_i2c_write_word(client,
				(table_reg + j),
				fuelgauge->info.battery_table[i][j]);
		}
	}

	/* MIX_MODE write*/
	sm5702_fg_i2c_write_word(client,
		sm5702_REG_RS_MIX_FACTOR, fuelgauge->info.rs_value[1]);
	sm5702_fg_i2c_write_word(client,
		sm5702_REG_RS_MAX, fuelgauge->info.rs_value[2]);
	sm5702_fg_i2c_write_word(client,
		sm5702_REG_RS_MIN, fuelgauge->info.rs_value[3]);
	sm5702_fg_i2c_write_word(client,
		sm5702_REG_MIX_RATE, fuelgauge->info.mix_value[0]);
	sm5702_fg_i2c_write_word(client,
		sm5702_REG_MIX_INIT_BLANK, fuelgauge->info.mix_value[1]);

	pr_err("%s: RS_MIX_FACTOR = 0x%x, RS_MAX = 0x%x, RS_MIN = 0x%x, MIX_RATE = 0x%x, MIX_INIT_BLANK = 0x%x\n",
		__func__,
		fuelgauge->info.rs_value[1], fuelgauge->info.rs_value[2],
		fuelgauge->info.rs_value[3], fuelgauge->info.mix_value[0],
		fuelgauge->info.mix_value[1]);


	/* CAL write*/
	sm5702_fg_i2c_write_word(client,
		sm5702_REG_CURR_CAL, fuelgauge->info.curr_cal);

	pr_err("%s: VOLT_CAL = 0x%x, CURR_CAL = 0x%x\n",
		__func__, fuelgauge->info.volt_cal, fuelgauge->info.curr_cal);
	value = sm5702_FG_PARAM_LOCK_CODE | sm5702_FG_TABLE_LEN;
	sm5702_fg_i2c_write_word(client, sm5702_REG_PARAM_CTRL, value);
	pr_err("%s: LAST PARAM CTRL VALUE = 0x%x : 0x%x\n",
		__func__, sm5702_REG_PARAM_CTRL, value);

	/*INIT_last -  control register set*/
	value = sm5702_fg_i2c_read_word(client, sm5702_REG_CNTL);
	pr_err("%s: REG_CNTL value = 0x%x, ENABLE_V_ALARM = 0x%x, ENABLE_MIX_MODE = 0x%x, ENABLE_TEMP_MEASURE = 0x%x\n",__func__, value,ENABLE_V_ALARM, ENABLE_MIX_MODE, ENABLE_TEMP_MEASURE);
	value |= ENABLE_V_ALARM | ENABLE_MIX_MODE | ENABLE_TEMP_MEASURE;
	pr_err("%s: value |= ENABLE_MIX_MODE | ENABLE_TEMP_MEASURE = 0x%x\n",__func__, value);

	ret = sm5702_fg_i2c_write_word(client, sm5702_REG_CNTL, value);

    // remove all mask
    sm5702_fg_i2c_write_word(client,sm5702_REG_INTFG_MASK,0x00FE);
    
    /* enable volt, soc alert irq; clear volt and soc alert status via i2c */
    //ret = ENABLE_L_VOL_INT;
    //sm5702_fg_i2c_write_word(client,SM5702_REG_INTFG_MASK,ret);

    /* set volt and alert threshold */
    ret = 0;
    ret |= (fuelgauge->info.alarm_vol_mv / 1000) * 256;
    ret |= ((fuelgauge->info.alarm_vol_mv - (ret * 1000))*256/1000 );
        
    //ret = 0x0300;//3000mV
    sm5702_fg_i2c_write_word(client, sm5702_REG_V_ALARM, ret);
    sm5702_fg_i2c_write_word(client, sm5702_REG_PARAM_RUN_UPDATE, 0);
    sm5702_fg_i2c_write_word(client, sm5702_REG_PARAM_RUN_UPDATE, 1);


    pr_err("%s: sm5702_REG_CNTL reg : 0x%x\n",
		__func__, value);

	/* Pram LOCK*/

	return 1;
}

static bool sm5702_fg_init(struct i2c_client *client)
{
	int ret = 0;
	int ta_exist;
	//union power_supply_propval value;
	struct sm5702_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);

	/*sm5702 i2c read check*/
	ret = fg_get_device_id(client);
	if (ret < 0) {
		pr_err("%s: fail to do i2c read(%d)\n", __func__, ret);
		/*return false;*/
	}

	if (sm5702_fg_check_reg_init_need(client))
		sm5702_fg_reg_init(client);

	ta_exist = (fuelgauge->info.batt_current > 9) ? true : false |
				fuelgauge->is_charging;
	pr_err("%s: is_charging = %d, ta_exist = %d\n",
		__func__, fuelgauge->is_charging, ta_exist);

	/* get first voltage measure to avgvoltage*/
	fuelgauge->info.batt_avgvoltage = fg_get_vbat(client);

	/* get first temperature*/
	fuelgauge->info.temperature = fg_get_temp(client);

	return true;
}
/*
bool sm5702_fg_full_charged(struct sm5702_fuelgauge_info *fuelgauge)
{
	fuelgauge->info.flag_full_charge = 1;

	pr_err("%s: full_charged\n", __func__);

	return true;
}
*/
/*
static int get_battery_id(struct sm5702_fuelgauge_info *fuelgauge)
{
	// sm5702fg does not support this function
	return 0;
}
*/
#define PROPERTY_NAME_SIZE 128

#define PINFO(format, args...) \
	pr_info("%s() line-%d: " format, \
		__func__, __LINE__, ## args)

static int sm5702_fg_parse(struct sm5702_fuelgauge_info *fuelgauge)
{
	struct device *dev = &fuelgauge->client->dev;

	int i, j;

	BUG_ON(dev == 0);

	/* get battery_table*/ 
	
	for (i = DISCHARGE_TABLE; i < TABLE_MAX; i++) {
		for (j = 0; j < sm5702_FG_TABLE_LEN; j++) {
			fuelgauge->info.battery_table[i][j] = battery_table_para[i][j];
			pr_err("<table[%d][%d] 0x%x>\n", i, j, fuelgauge->info.battery_table[i][j]);
		}
	}
	/* get rce*/
	fuelgauge->info.rce_value[0] = sm5702_fuelgauge_data.rce_value[0];
	fuelgauge->info.rce_value[1] = sm5702_fuelgauge_data.rce_value[1];
	fuelgauge->info.rce_value[2] = sm5702_fuelgauge_data.rce_value[2];
	pr_err("rce_value = <0x%x 0x%x 0x%x>\n",
		fuelgauge->info.rce_value[0], fuelgauge->info.rce_value[1],
		fuelgauge->info.rce_value[2]);

	/* get dtcd_value*/
	fuelgauge->info.dtcd_value = sm5702_fuelgauge_data.dtcd_value;
	pr_err("dtcd_value = <0x%x>\n", fuelgauge->info.dtcd_value);

	/* get rs_value*/
	fuelgauge->info.rs_value[0] = sm5702_fuelgauge_data.rs_value[0];
	fuelgauge->info.rs_value[1] = sm5702_fuelgauge_data.rs_value[1];
	fuelgauge->info.rs_value[2] = sm5702_fuelgauge_data.rs_value[2];
	fuelgauge->info.rs_value[3] = sm5702_fuelgauge_data.rs_value[3];
	pr_err("rs_value = <0x%x 0x%x 0x%x 0x%x>\n",
		fuelgauge->info.rs_value[0], fuelgauge->info.rs_value[1],
		fuelgauge->info.rs_value[2], fuelgauge->info.rs_value[3]);

	/* get vit_period*/
	fuelgauge->info.vit_period = sm5702_fuelgauge_data.vit_period;
	pr_err("vit_period = <0x%x>\n", fuelgauge->info.vit_period);

	/* get mix_value*/
	fuelgauge->info.mix_value[0] = sm5702_fuelgauge_data.mix_value[0];
	fuelgauge->info.mix_value[1] = sm5702_fuelgauge_data.mix_value[1];
	pr_err("mix_value = <0x%x 0x%x>\n",
		fuelgauge->info.mix_value[0], fuelgauge->info.mix_value[1]);

	/* battery_type*/
	fuelgauge->info.battery_type = sm5702_fuelgauge_data.battery_type;
	pr_err("battery_type = <0x%x>\n", fuelgauge->info.battery_type);

	/* VOL & CURR CAL*/
	fuelgauge->info.volt_cal = sm5702_fuelgauge_data.volt_cal;
	fuelgauge->info.curr_cal = sm5702_fuelgauge_data.curr_cal;
	pr_err("volt_cal = <0x%x>\n", fuelgauge->info.volt_cal);
	pr_err("curr_cal = <0x%x>\n", fuelgauge->info.curr_cal);

	/* temp_std*/
	fuelgauge->info.temp_std = sm5702_fuelgauge_data.temp_std;
	pr_err("temp_std = <0x%x>\n", fuelgauge->info.temp_std);

	/* temp_offset*/
	fuelgauge->info.temp_offset = sm5702_fuelgauge_data.temp_offset;
	pr_err("temp_offset = <0x%x>\n", fuelgauge->info.temp_offset);

	/* temp_offset_cal*/
	fuelgauge->info.temp_offset_cal = sm5702_fuelgauge_data.temp_offset_cal;
	pr_err("temp_offset_cal = <0x%x>\n", fuelgauge->info.temp_offset_cal);

	/* charge_offset_cal*/
	fuelgauge->info.charge_offset_cal = sm5702_fuelgauge_data.charge_offset_cal;
	pr_err("charge_offset_cal = <0x%x>\n", fuelgauge->info.charge_offset_cal);	

	/*alarm_vol_mv*/
	fuelgauge->info.alarm_vol_mv = sm5702_fuelgauge_data.alarm_vol_mv;
	pr_err("alarm_vol_mv = <0x%x>\n", fuelgauge->info.charge_offset_cal);	

	return 0;
}

bool sm5702_hal_fg_init(struct i2c_client *client)
{
	struct sm5702_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);

	pr_err("sm5702 sm5702_hal_fg_init...\n");
	mutex_init(&fuelgauge->info.param_lock);
	mutex_lock(&fuelgauge->info.param_lock);
	
	/* Load battery data from header File */
	sm5702_fg_parse(fuelgauge);

	sm5702_fg_init(client);
	sm5702_pr_ver_info(client);
	fuelgauge->info.temperature = 250;

	mutex_unlock(&fuelgauge->info.param_lock);
	pr_err("sm5702 hal fg init OK\n");
	return true;
}

static void sm5702_work(struct work_struct *work)
{
	struct sm5702_fuelgauge_info *fuelgauge;

	fuelgauge = container_of(work,
		struct sm5702_fuelgauge_info, isr_work.work);
	fg_get_vbat(fuelgauge->client);
	fg_get_ocv(fuelgauge->client);
	fg_get_curr(fuelgauge->client);
	fg_get_temp(fuelgauge->client);
	fg_get_soc(fuelgauge->client);
	fg_get_online(fuelgauge->client);

	pr_err("sm5702_work for FG");

	schedule_delayed_work(&fuelgauge->isr_work, SM5702_DELAY);
}

static enum power_supply_property sm5702_fuelgauge_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TEMP_AMBIENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_STATUS,
};

static int sm5702_fg_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sm5702_fuelgauge_info *fuelgauge = container_of(psy,
			struct sm5702_fuelgauge_info, psy_fg);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = fuelgauge->info.online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = fuelgauge->info.batt_voltage;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		val->intval = fuelgauge->info.batt_avgvoltage;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = fuelgauge->info.batt_current * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = fuelgauge->info.batt_avgcurrent * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = (fuelgauge->info.batt_soc >= 1000) ? true : false;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = fuelgauge->info.batt_soc / 10;
		break;
		/* Battery Temperature */
	case POWER_SUPPLY_PROP_TEMP:
		/* Target Temperature */
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		val->intval = fuelgauge->info.temperature;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = fuelgauge->info.batt_voltage <= 0 ? 0 : 1;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		if (fuelgauge->info.batt_soc >= 1000)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else if (fuelgauge->info.batt_current > 9)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	default:
		return -EINVAL;
	}
	pr_err("%s: psp=%d, value_get=%d\n", __func__, psp, val->intval);
	return 0;
}

static int sm5702_fg_set_property(struct power_supply *psy,
				 enum power_supply_property psp,
				 const union power_supply_propval *val)
{
	//struct sm5702_fuelgauge_info *fuelgauge = container_of(psy,
	//			struct sm5702_fuelgauge_info, psy_fg);
	/*struct sm5702_fuelgauge_info *fuelgauge =
		dev_get_drvdata(psy->dev);*/
	pr_err("%s, psp=%d, value_to_set=%d\n", __func__, psp, val->intval);
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		/*if (val->intval == POWER_SUPPLY_STATUS_FULL)
			sm5702_fg_full_charged(fuelgauge);*/
		break;
		/* Battery Temperature */
	case POWER_SUPPLY_PROP_TEMP:
				break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
				break;
/*
	case POWER_SUPPLY_PROP_ONLINE:
		fuelgauge->cable_type = val->intval;
		if (val->intval == POWER_SUPPLY_TYPE_BATTERY) {
			fuelgauge->is_charging = false;
			fuelgauge->info.online = false;
		} else {
			fuelgauge->is_charging = true;
			fuelgauge->info.online = true;
		}
				break;
*/
	default:
		return -EINVAL;
	}
	return 0;
}

static irqreturn_t bat_low_detect_irq_handler(int irq, void *data)
{
	struct sm5702_fuelgauge_info *fuelgauge = data;
    int32_t int_val = 0, status_val = 0;    

	int_val = sm5702_fg_i2c_read_word(fuelgauge->client, sm5702_REG_INTFG);
    status_val = sm5702_fg_i2c_read_word(fuelgauge->client, sm5702_REG_STATUS);

    pr_err("%s: Low Battery, INT = 0x%x, STATUS = 0x%x\n", __func__, int_val, status_val);
    pr_err("%s: Low Battery\n", __func__);

    return IRQ_HANDLED;
}

static int sm5702_fuelgauge_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct sm5702_fuelgauge_info *fuelgauge;
	struct gpio_desc *gpio;
	int ret, irq;

	pr_err("%s: Probe Start\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;
		
	fuelgauge = devm_kzalloc(&client->dev, sizeof(*fuelgauge), GFP_KERNEL);
	if (!fuelgauge)
		return -ENOMEM;

	fuelgauge->client = client;
    fuelgauge->client->addr = SM5702FG_SLAVE_ADDR;
	i2c_set_clientdata(client, fuelgauge);


	/*fuelgauge->pdata = client->dev.platform_data;*/
	/*dev_set_drvdata(&client->dev, fuelgauge);*/

	/*INIT*/
	if (!sm5702_hal_fg_init(fuelgauge->client))
		pr_err("%s: Failed to Initialize Fuelgauge\n", __func__);


	fuelgauge->psy_fg.name		= "sm5702-fuelgauge";
	fuelgauge->psy_fg.type		= POWER_SUPPLY_TYPE_BATTERY;
	fuelgauge->psy_fg.get_property	= sm5702_fg_get_property;
	fuelgauge->psy_fg.set_property	= sm5702_fg_set_property;
	fuelgauge->psy_fg.properties	= sm5702_fuelgauge_props;
	fuelgauge->psy_fg.num_properties = ARRAY_SIZE(sm5702_fuelgauge_props);

	/* Register the battery with the power supply class. */
	ret = power_supply_register(&client->dev, &fuelgauge->psy_fg);
	if (ret) {
		pr_err("failed: power supply register\n");
		power_supply_unregister(&fuelgauge->psy_fg);
		kfree(fuelgauge);
		return ret;
	}

	fuelgauge->is_fuel_alerted = false;

	/*
	if (fuelgauge->pdata->fuel_alert_soc >= 0) {
		if (sm5702_hal_fg_fuelalert_init(fuelgauge->client,
			fuelgauge->pdata->fuel_alert_soc))
			wake_lock_init(&fuelgauge->fuel_alert_wake_lock,
				WAKE_LOCK_SUSPEND, "fuel_alerted");
		else {
			pr_err("%s: Failed to Initialize Fuel-alert\n",
				__func__);
			if (fuelgauge->fg_irq > 0)
				free_irq(fuelgauge->fg_irq, fuelgauge);
			wake_lock_destroy(&fuelgauge->fuel_alert_wake_lock);
			return EINTR;
		}
	}

	if (fuelgauge->pdata->fg_irq > 0) {
		INIT_DELAYED_WORK(
			&fuelgauge->isr_work, sm5702_fg_isr_work);

		fuelgauge->fg_irq = gpio_to_irq(fuelgauge->pdata->fg_irq);
		dev_info(&client->dev,
			"%s: fg_irq = %d\n", __func__, fuelgauge->fg_irq);
		if (fuelgauge->fg_irq > 0) {
			ret = request_threaded_irq(fuelgauge->fg_irq,
					NULL, fg_irq_thread,
					IRQF_TRIGGER_FALLING |
					IRQF_TRIGGER_RISING |
					IRQF_ONESHOT,
					"fuelgauge-irq", fuelgauge);
			if (ret) {
				dev_err(&client->dev,
				"%s: Failed to Reqeust IRQ\n", __func__);
				goto err_supply_unreg;
			}

			ret = enable_irq_wake(fuelgauge->fg_irq);
			if (ret < 0)
				dev_err(&client->dev,
				"%s: Failed to Enable Wakeup Source(%d)\n",
					__func__, ret);
		} else {
			dev_err(&client->dev, "%s: Failed gpio_to_irq(%d)\n",
				__func__, fuelgauge->fg_irq);
			goto err_supply_unreg;
		}
	}
	*/
#ifdef CONFIG_ACPI
	gpio = devm_gpiod_get_index(&client->dev, "sm5702_fg_int", 0);
	pr_err("%s: gpio=%d\n", __func__);
	if (IS_ERR(gpio)) {
		dev_err(&client->dev, "acpi gpio get index failed\n");
		i2c_set_clientdata(client, NULL);
		return PTR_ERR(gpio);
	} else {
		ret = gpiod_direction_input(gpio);
		if (ret < 0)
			dev_err(&client->dev, "BQ CHRG set direction failed\n");

		irq = gpiod_to_irq(gpio);
		pr_err("%s: irq=%d\n", __func__);
	}

	fuelgauge->initial_update_of_soc = true;

    ret = request_threaded_irq(irq, NULL, bat_low_detect_irq_handler,
                   IRQF_ONESHOT | IRQF_TRIGGER_FALLING |
                   IRQF_NO_SUSPEND, "bat_low_detect", fuelgauge);

	if (ret < 0) {
		pr_err("%s : register_irq error\n",__func__);
    	goto err_reg_irq;
	}
	enable_irq_wake(irq);
#endif

	pr_err("%s: SM5702 Fuelgauge Driver Loaded\n", __func__);

	INIT_DEFERRABLE_WORK(&fuelgauge->isr_work, sm5702_work);
	schedule_delayed_work(&fuelgauge->isr_work, SM5702_DELAY);

    return 0;
    
err_reg_irq:
    power_supply_unregister(&fuelgauge->psy_fg);
	
	kfree(fuelgauge);
    
	return ret;    
}

static int sm5702_fuelgauge_suspend(struct device *dev)
{
	struct sm5702_fuelgauge_info *fuelgauge =
			i2c_get_clientdata(to_i2c_client(dev));
	pr_err("sm5702_fuelgauge_resume\n");
	cancel_delayed_work(&fuelgauge->isr_work);
	return 0;
}

static int sm5702_fuelgauge_resume(struct device *dev)
{
	struct sm5702_fuelgauge_info *fuelgauge =
			i2c_get_clientdata(to_i2c_client(dev));
	pr_err("sm5702_fuelgauge_resume\n");
	schedule_delayed_work(&fuelgauge->isr_work, SM5702_DELAY);
	return 0;
}

static int sm5702_fuelgauge_remove(struct i2c_client *client)
{
	struct sm5702_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);
	pr_err("sm5702_fuelgauge_remove\n");    
	power_supply_unregister(&fuelgauge->psy_fg);
	cancel_delayed_work(&fuelgauge->isr_work);
	kfree(fuelgauge);
	return 0;
}

static const struct dev_pm_ops sm5702_fuelgauge_pm_ops = {
	.suspend = sm5702_fuelgauge_suspend,
	.resume	 = sm5702_fuelgauge_resume,
};

#ifdef CONFIG_ACPI	
static const struct i2c_device_id sm5702_fuelgauge_id[] = {
	{ "EXFG0000", 0 },
	/*{"sm5702-fuelgauge", 0},
	{}*/
};
#else
static const struct i2c_device_id sm5702_fuelgauge_id[] = {
	{"sm5702-fuelgauge", 0},
	{}
};
#endif    

#ifdef CONFIG_ACPI
static struct acpi_device_id extfg_acpi_match[] = {
	{"EXFG0000", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, extfg_acpi_match);
#endif
static struct i2c_driver sm5702_fuelgauge_driver = {
	.probe			= sm5702_fuelgauge_probe,
	.remove			= sm5702_fuelgauge_remove,
#ifdef CONFIG_ACPI	
	.id_table		= sm5702_fuelgauge_id,
#else	
    .id_table   = sm5702_fuelgauge_id,			
#endif    
	.driver = {
		.name	= "sm5702-fuelgauge",
		.owner	= THIS_MODULE,
		.pm = &sm5702_fuelgauge_pm_ops,
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(extfg_acpi_match),
#endif	
	},
};

static int __init sm5702_fuelgauge_init(void)
{
	pr_err("%s: Enter!**************\n", __func__);
	return i2c_add_driver(&sm5702_fuelgauge_driver);   
}

static void __exit sm5702_fuelgauge_exit(void)
{
    i2c_del_driver(&sm5702_fuelgauge_driver);
}

subsys_initcall(sm5702_fuelgauge_init);
module_exit(sm5702_fuelgauge_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sm5702 Fuel Gauge Driver");

