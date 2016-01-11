/*
 * bluewhale_charger.c - Charger driver for O2Micro OZ1C115C
 *
 * Copyright (C) 2011 Intel Corporation
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author: Liu, Xiang <xiang.liu@intel.com>
 */

//#define TEST_DEBUG		1	//test debug, open sysfs port and add polling timer
//#define DEBUG_ILMT		1	//detect ILMT from high to low, test only 


#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <linux/gpio.h>
#include <linux/power_supply.h>
#include "parameter.h"
#include "bluewhale_charger.h"
#include "o2_adapter.h"
#include "o2_config.h"
#include "../power_supply.h"

#define VERSION		   "2015.7.02/6.00.01"	

#define	RETRY_CNT	5

//#define BATT_TEMP_SIMULATION 


#define bluewhale_dbg(fmt, args...) pr_err("[bluewhale]:"pr_fmt(fmt)"\n", ## args)

// if you update vbus voltage more than 12v,you must make VBUS_HIGH 1200


static char *bluewhale_chrg_supplied_to[] = {
        "battery"
};


static struct bluewhale_platform_data  bluewhale_pdata_default = {
        .max_charger_currentmA = 3000,
        .max_charger_voltagemV = 4200,
        .termination_currentmA = 80,
		.T34_charger_voltagemV = 4200,    //dangous
		.T45_charger_voltagemV = 4200,    //dangous
		.wakeup_voltagemV = 2500,
		.wakeup_currentmA = 250,
		.recharge_voltagemV = 100,
		.min_vsys_voltagemV = 3600,
		.vbus_limit_currentmA = 2000,
		.max_cc_charge_time = 0,
		.max_wakeup_charge_time = 0,
		.rthm_10k = 0,			//1 for 10K, 0 for 100K thermal resistor
		.inpedance = 80,
		.supplied_to = bluewhale_chrg_supplied_to
};

#ifdef O2_CHARGER_POWER_SUPPOLY_SUPPORT
static enum power_supply_property bluewhale_charger_props[] = {
        POWER_SUPPLY_PROP_PRESENT,
        POWER_SUPPLY_PROP_ONLINE,
        POWER_SUPPLY_PROP_TYPE,
        POWER_SUPPLY_PROP_HEALTH,
        POWER_SUPPLY_PROP_MAX_CHARGE_CURRENT,
        POWER_SUPPLY_PROP_MAX_CHARGE_VOLTAGE,
        POWER_SUPPLY_PROP_CHARGE_CURRENT,
        POWER_SUPPLY_PROP_CHARGE_VOLTAGE,
        POWER_SUPPLY_PROP_INLMT,
        POWER_SUPPLY_PROP_ENABLE_CHARGING,
        POWER_SUPPLY_PROP_ENABLE_CHARGER,
        POWER_SUPPLY_PROP_CHARGE_TERM_CUR,
        POWER_SUPPLY_PROP_CABLE_TYPE,
        POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
        POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
        POWER_SUPPLY_PROP_MAX_TEMP,
        POWER_SUPPLY_PROP_MIN_TEMP,
};
#endif

static struct bluewhale_charger_info *charger_info;
static DEFINE_MUTEX(bluewhale_mutex);
static uint8_t chg_adapter_type = QUICK_CHG_ADAPTER_NULL;
static struct power_supply *battery_psy = NULL;
static uint8_t start_adjust_voltage = 0;



static void o2_start_usb_charge(struct bluewhale_charger_info *charger);
static void o2_stop_charge(struct bluewhale_charger_info *charger);
/*****************************************************************************
 * Description:
 *		bluewhale_read_byte 
 * Parameters:
 *		charger:	charger data
 *		index:	register index to be read
 *		*dat:	buffer to store data read back
 * Return:
 *      negative errno else a data byte received from the device.
 *****************************************************************************/
static int32_t bluewhale_read_byte(struct bluewhale_charger_info *charger, uint8_t index, uint8_t *dat)
{
	int32_t ret;
	uint8_t i;
	struct i2c_client *client = charger->client;

	for(i = 0; i < RETRY_CNT; i++){
		ret = i2c_smbus_read_byte_data(client, index);

		if(ret >= 0) break;
		else
			dev_err(&client->dev, "%s: err %d, %d times\n", __func__, ret, i);
	}
	if(i >= RETRY_CNT)
	{
		return ret;
	} 
	*dat = (uint8_t)ret;

	return ret;
}


/*****************************************************************************
 * Description:
 *		bluewhale_write_byte 
 * Parameters:
 *		charger:	charger data
 *		index:	register index to be write
 *		dat:		write data
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int32_t bluewhale_write_byte(struct bluewhale_charger_info *charger, uint8_t index, uint8_t dat)
{
	int32_t ret;
	uint8_t i;
	struct i2c_client *client = charger->client;
	
	for(i = 0; i < RETRY_CNT; i++){
		ret = i2c_smbus_write_byte_data(client, index, dat);
		if(ret >= 0) break;
		else
			dev_err(&client->dev, "%s: err %d, %d times\n", __func__, ret, i);
	}
	if(i >= RETRY_CNT)
	{
		return ret;
	}

	return ret;
}

static int32_t bluewhale_update_bits(struct bluewhale_charger_info *charger, uint8_t reg, uint8_t mask, uint8_t data)
{
	int32_t ret;
	uint8_t tmp;

	ret = bluewhale_read_byte(charger, reg, &tmp);
	if (ret < 0)
		return ret;

	if ((tmp & mask) != data)
	{
		tmp &= ~mask;
		tmp |= data & mask;
		return bluewhale_write_byte(charger, reg, tmp);

	}
	else
		return 0;
}

/*****************************************************************************/

/*****************************************************************************
 * Description:
 *		bluewhale_set_min_vsys 
 * Parameters:
 *		charger:	charger data
 *		min_vsys_mv:	min sys voltage to be written
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_min_vsys(struct bluewhale_charger_info *charger, int min_vsys_mv)
{
	int ret = 0;
	u8 vsys_val = 0; 

	if (min_vsys_mv < 1800)
		min_vsys_mv = 1800;		//limit to 1.8V
	else if (min_vsys_mv > 3600)
		min_vsys_mv = 3600;		//limit to 3.6V

	vsys_val = min_vsys_mv / VSYS_VOLT_STEP;	//step is 200mV

	ret = bluewhale_update_bits(charger, REG_MIN_VSYS_VOLTAGE, 0x1f, vsys_val);

	return ret;
}
/*****************************************************************************
 * Description:
 *		bluewhale_set_chg_volt 
 * Parameters:
 *		charger:	charger data
 *		chgvolt_mv:	charge voltage to be written
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_chg_volt(struct bluewhale_charger_info *charger, int chgvolt_mv)
{
	int ret = 0;
	u8 chg_volt = 0; 

	if (chgvolt_mv < 4000)
		chgvolt_mv = 4000;		//limit to 4.0V
	else if (chgvolt_mv > 4600)
		chgvolt_mv = 4600;		//limit to 4.6V

	chg_volt = (chgvolt_mv - 4000) / CHG_VOLT_STEP;	//step is 25mV

	ret = bluewhale_update_bits(charger, REG_CHARGER_VOLTAGE, 0x1f, chg_volt);

	return ret;
}

int bluewhale_get_chg_volt(struct bluewhale_charger_info *charger)
{
	uint8_t data;
	int ret = 0;

	ret = bluewhale_read_byte(charger, REG_CHARGER_VOLTAGE, &data);
	if (ret < 0)
		return ret;
	return (data * CHG_VOLT_STEP);	//step is 25mA
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_chg_volt_extern 
 * Parameters:
 *		chgvolt_mv:	charge voltage to be written
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_chg_volt_extern(int chgvolt_mv)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_set_chg_volt(charger_info, chgvolt_mv);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_t34_cv
 * Parameters:
 *		charger:	charger data
 *		chgvolt_mv:	charge voltage to be written at t34
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_t34_cv(struct bluewhale_charger_info *charger, int chgvolt_mv)
{
	int ret = 0;
	u8 chg_volt = 0; 

	if (chgvolt_mv < 4000)
		chgvolt_mv = 4000;		//limit to 4.0V
	else if (chgvolt_mv > 4600)
		chgvolt_mv = 4600;		//limit to 4.6V

	chg_volt = (chgvolt_mv - 4000) / CHG_VOLT_STEP;	//step is 25mV

	ret = bluewhale_update_bits(charger, REG_T34_CHG_VOLTAGE, 0x1f, chg_volt);

	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_t45_cv 
 * Parameters:
 *		charger:	charger data
 *		chgvolt_mv:	charge voltage to be written at t45
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_t45_cv(struct bluewhale_charger_info *charger, int chgvolt_mv)
{
	int ret = 0;
	u8 chg_volt = 0; 

	if (chgvolt_mv < 4000)
		chgvolt_mv = 4000;		//limit to 4.0V
	else if (chgvolt_mv > 4600)
		chgvolt_mv = 4600;		//limit to 4.6V

	chg_volt = (chgvolt_mv - 4000) / CHG_VOLT_STEP;	//step is 25mV

	ret = bluewhale_update_bits(charger, REG_T45_CHG_VOLTAGE, 0x1f, chg_volt);

	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_wakeup_volt 
 * Parameters:
 *		charger:	charger data
 *		wakeup_mv:	set wake up voltage
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_wakeup_volt(struct bluewhale_charger_info *charger, int wakeup_mv)
{
	int ret = 0;
	u8 data = 0;
	u8 wak_volt = 0; 

	if (wakeup_mv < 1500)
		wakeup_mv = 1500;		//limit to 1.5V
	else if (wakeup_mv > 3000)
		wakeup_mv = 3000;		//limit to 3.0V

	wak_volt = wakeup_mv / WK_VOLT_STEP;	//step is 100mV

	ret = bluewhale_read_byte(charger, REG_WAKEUP_VOLTAGE, &data);
	if (ret < 0)
		return ret;
	if (data != wak_volt) 
		ret = bluewhale_write_byte(charger, REG_WAKEUP_VOLTAGE, wak_volt);	
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_eoc_current 
 * Parameters:
 *		charger:	charger data
 *		eoc_ma:	set end of charge current
 *		Only 0mA, 20mA, 40mA, ... , 320mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_eoc_current(struct bluewhale_charger_info *charger, int eoc_ma)
{
	int ret = 0;
	u8 data = 0;
	u8 eoc_curr = 0; 

	//Notice: with 00h, end of charge function function is disabled
	if (eoc_ma <= 0)
		eoc_ma = 0;		//min value is 0mA
	else if (eoc_ma > 320)
		eoc_ma = 320;		//limit to 320mA

	eoc_curr = eoc_ma / EOC_CURRT_STEP;	//step is 10mA

	ret = bluewhale_read_byte(charger, REG_END_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	if (data != eoc_curr) 
		ret = bluewhale_write_byte(charger, REG_END_CHARGE_CURRENT, eoc_curr);	
	return ret;
}

int bluewhale_get_eoc_current(struct bluewhale_charger_info *charger)
{
	uint8_t data;
	int ret = 0;

	ret = bluewhale_read_byte(charger, REG_END_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	return (data * EOC_CURRT_STEP);	//step is 10mA
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_eoc_current_extern 
 * Parameters:
 *		eoc_ma:	set end of charge current
 *		Only 0mA, 20mA, 40mA, ... , 320mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_eoc_current_extern(int eoc_ma)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_set_eoc_current(charger_info, eoc_ma);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_vbus_current
 * Parameters:
 *		charger:	charger data
 *		ilmt_ma:	set input current limit
 *		Only 100mA, 500mA, 700mA, 900mA, 1000mA, 1200mA, 1400mA, 1500mA, 1700mA, 1900mA, 2000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_vbus_current(struct bluewhale_charger_info *charger, int ilmt_ma)
{
	int ret = 0;
	u8 data = 0;
	u8 input_curr = 0;

	if(ilmt_ma < 300)
		input_curr = 0x01;		//100mA
	else if(ilmt_ma >= 300 && ilmt_ma < 600)
		input_curr = 0x05;		//500mA
	else if(ilmt_ma >= 600 && ilmt_ma < 800)
		input_curr = 0x0c;	//700mA
	else if(ilmt_ma >= 800 && ilmt_ma < 950)
		input_curr = 0x09;	//900mA
	else if(ilmt_ma >= 950 && ilmt_ma < 1100)
		input_curr = 0x10;	//1000mA
	else if(ilmt_ma >= 1100 && ilmt_ma < 1300)
		input_curr = 0x12;	//1200mA
	else if(ilmt_ma >= 1300 && ilmt_ma < 1450)
		input_curr = 0x0e;	//1400mA
	else if(ilmt_ma >= 1450 && ilmt_ma < 1600)
		input_curr = 0x0f;	//1500mA
	else if(ilmt_ma >= 1600 && ilmt_ma < 1800)
		input_curr = 0x11;	//1700mA
	else if(ilmt_ma >= 1800 && ilmt_ma < 1950)
		input_curr = 0x13;	//1900mA
	else if(ilmt_ma >= 1950)
		input_curr = 0x14;	//2000mA

	ret = bluewhale_read_byte(charger, REG_VBUS_LIMIT_CURRENT, &data);
	if (ret < 0)
		return ret;
		
	if (data != input_curr) 
		ret = bluewhale_write_byte(charger, REG_VBUS_LIMIT_CURRENT, input_curr);
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_vbus_current_extern
 * Parameters:
 *		ilmt_ma:	set input current limit
 *		Only 100mA, 500mA, 700mA, 900mA, 1000mA, 1200mA, 1400mA, 1500mA, 1700mA, 1900mA, 2000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_vbus_current_extern(int ilmt_ma)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_set_vbus_current(charger_info, ilmt_ma);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}


/*****************************************************************************
 * Description:
 *		bluewhale_get_vbus_current
 * Parameters:
 *		charger:	charger data
 * Return:
 *		Vbus input current in mA
 *****************************************************************************/
int bluewhale_get_vbus_current(struct bluewhale_charger_info *charger)
{
	int ret = 0;
	u8 data = 0;

	ret = bluewhale_read_byte(charger, REG_VBUS_LIMIT_CURRENT, &data);
	if (ret < 0)
		return ret;

	switch(data)
	{
		case 0x00: return 0;
		case 0x01: ;
		case 0x02: ;
		case 0x03: return 100;
		case 0x04: ;
		case 0x05: ;
		case 0x06: ;
		case 0x07: return 500;
		case 0x08: ;
		case 0x09: ;
		case 0x0a: ;
		case 0x0b: return 900;
		case 0x0c: ;
		case 0x0d: return 700;
		case 0x0e: return 1400;
		case 0x0f: return 1500;
		case 0x10: return 1000;
		case 0x11: return 1700;
		case 0x12: return 1200;
		case 0x13: return 1900;
		case 0x14: return 2000;
		default:
				   return -1;
	}
}

/*****************************************************************************
 * Description:
 *		bluewhale_get_vbus_current_extern
 * Return:
 *		Vbus input current in mA
 *****************************************************************************/
int bluewhale_get_vbus_current_extern(void)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_get_vbus_current(charger_info);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_rechg_hystersis
 * Parameters:
 *		charger:	charger data
 *		hyst_mv:	set Recharge hysteresis Register
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_rechg_hystersis(struct bluewhale_charger_info *charger, int hyst_mv)
{
	int ret = 0;
	u8 data = 0;
	u8 rechg = 0; 
		
	if (hyst_mv > 200) {
		hyst_mv = 200;			//limit to 200mV
	}
	//Notice: with 00h, recharge function is disabled

	rechg = hyst_mv / RECHG_VOLT_STEP;	//step is 50mV
	
	ret = bluewhale_read_byte(charger, REG_RECHARGE_HYSTERESIS, &data);
	if (ret < 0)
		return ret;
	if (data != rechg) 
		ret = bluewhale_write_byte(charger, REG_RECHARGE_HYSTERESIS, rechg);	
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_charger_current
 * Parameters:
 *		charger:	charger data
 *		chg_ma:	set charger current
 *		Only 600mA, 800mA, 1000mA, ... , 3800mA, 4000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_charger_current(struct bluewhale_charger_info *charger, int chg_ma)
{
	int ret = 0;
	u8 data = 0;
	u8 chg_curr = 0; 

	if (chg_ma > 4000)
		chg_ma = 4000;		//limit to 4A		

	chg_curr = chg_ma / CHG_CURRT_STEP;	//step is 100mA

	//notice: chg_curr value less than 06h, charger will be disabled.
	//charger can power system in this case.

	ret = bluewhale_read_byte(charger, REG_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	if (data != chg_curr) 
		ret = bluewhale_write_byte(charger, REG_CHARGE_CURRENT, chg_curr);	
	return ret;
}

int bluewhale_get_charger_current(struct bluewhale_charger_info *charger)
{
	uint8_t data;
	int ret = 0;

	ret = bluewhale_read_byte(charger, REG_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	return (data * CHG_CURRT_STEP);	//step is 100mA
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_charger_current_extern
 * Parameters:
 *		chg_ma:	set charger current
 *		Only 600mA, 800mA, 1000mA, ... , 3800mA, 4000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_charger_current_extern(int chg_ma)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_set_charger_current(charger_info, chg_ma);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
/*****************************************************************************
 * Description:
 *		bluewhale_set_wakeup_current 
 * Parameters:
 *		charger:	charger data
 *		wak_ma:	set wakeup current
 *		Only 100mA, 120mA, ... , 400mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_wakeup_current(struct bluewhale_charger_info *charger, int wak_ma)
{
	int ret = 0;
	u8 data = 0;
	u8 wak_curr = 0; 
	if (wak_ma < 100)
		wak_ma = 100;		//limit to 100mA
	if (wak_ma > 400)
		wak_ma = 400;		//limit to 400mA
	wak_curr = wak_ma / WK_CURRT_STEP;	//step is 10mA

	ret = bluewhale_read_byte(charger, REG_WAKEUP_CURRENT, &data);
	if (ret < 0)
		return ret;
	if (data != wak_curr) 
		ret = bluewhale_write_byte(charger, REG_WAKEUP_CURRENT, wak_curr);
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_safety_cc_timer
 * Parameters:
 *		charger:	charger data
 *		tmin:	set safety cc charge time
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_safety_cc_timer(struct bluewhale_charger_info *charger, int tmin)
{
	int ret = 0;
	u8 data = 0;
	u8 tval = 0; 

	//Notice: with 0xh, saftety timer function is disabled
	if (tmin == 0) {	//disable
		tval = 0;
	}
	else if (tmin == 120) {	//120min
		tval = CC_TIMER_120MIN;
	}
	else if (tmin == 180) {	//180min
		tval = CC_TIMER_180MIN;
	}
	else if (tmin == 240) {	//240min
		tval = CC_TIMER_240MIN;
	}
	else if (tmin == 300) {	//300min
		tval = CC_TIMER_300MIN;
	}
	else if (tmin == 390) {	//300min
		tval = CC_TIMER_390MIN;
	}
	else if (tmin == 480) {	//300min
		tval = CC_TIMER_480MIN;
	}
	else if (tmin == 570) {	//300min
		tval = CC_TIMER_570MIN;
	}
	else {				//invalid values
		pr_err("%s: invalide value, set default value 180 minutes\n", __func__);
		tval = CC_TIMER_180MIN;	//default value
	}

	ret = bluewhale_read_byte(charger, REG_SAFETY_TIMER, &data);
	if (ret < 0)
		return ret;

	if ((data & CC_TIMER_MASK) != tval) 
	{
		data &= WAKEUP_TIMER_MASK;	//keep wk timer
		data |= tval;				//update new cc timer
		ret = bluewhale_write_byte(charger, REG_SAFETY_TIMER, data);	
	}
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_safety_wk_timer 
 * Parameters:
 *		charger:	charger data
 *		tmin:	set safety wakeup time
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_safety_wk_timer(struct bluewhale_charger_info *charger, int tmin)
{
	int ret = 0;
	u8 data = 0;
	u8 tval = 0; 

	//Notice: with x0h, saftety timer function is disabled
	if (tmin == 0) {	//disable
		tval = 0;
	}
	else if (tmin == 15) {	//15min
		tval = WK_TIMER_15MIN;
	}
	else if (tmin == 30) {	//30min
		tval = WK_TIMER_30MIN;
	}
	else if (tmin == 45) {	//45min
		tval = WK_TIMER_45MIN;
	}
	else if (tmin == 60) {	//60min
		tval = WK_TIMER_60MIN;
	}
	else if (tmin == 75) {	//60min
		tval = WK_TIMER_75MIN;
	}
	else if (tmin == 90) {	//60min
		tval = WK_TIMER_90MIN;
	}
	else if (tmin == 105) {	//60min
		tval = WK_TIMER_105MIN;
	}
	else {				//invalid values
		pr_err("%s: invalide value, set default value 30 minutes\n", __func__);
		tval = WK_TIMER_30MIN;	//default value
	}

	ret = bluewhale_read_byte(charger, REG_SAFETY_TIMER, &data);
	if (ret < 0)
		return ret;

	if ((data & WAKEUP_TIMER_MASK) != tval) 
	{
		data &= CC_TIMER_MASK;		//keep cc timer
		data |= tval;				//update new wk timer
		ret = bluewhale_write_byte(charger, REG_SAFETY_TIMER, data);		
	} 
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_enable_otg
 * Parameters:
 *		charger:	charger data
 *		enable:	enable/disable OTG
 *		1, 0
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_enable_otg(struct bluewhale_charger_info *charger, int enable)
{
	uint8_t data;
	uint8_t mask = 0;

    data = (enable == 0) ? 0 : 1;

	if (data) //enable otg, disable suspend
		mask = 0x06;
	else //disable otg
		mask = 0x04;

	return bluewhale_update_bits(charger, REG_CHARGER_CONTROL, mask, data << 2);
}


/*****************************************************************************
 * Description:
 *		bluewhale_enable_otg_extern
 * Parameters:
 *		enable:	enable/disable OTG
 *		1, 0
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_enable_otg_extern(int enable)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_enable_otg(charger_info, enable);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}


int32_t o2_read_vbus_voltage(void)
{

	int32_t vbus_voltage = 0;
#ifdef VBUS_COLLECT_BY_O2
	if(oz8806_get_init_status())
	{
		vbus_voltage = oz8806_vbus_voltage();
	}
#elif defined VBUS_COLLECT_BY_MTK
	vbus_voltage = BMT_status.charger_vol;
#endif
	pr_info("vbus voltage:%d\n", vbus_voltage);
	return vbus_voltage;
}
EXPORT_SYMBOL(o2_read_vbus_voltage);
int get_adjust_voltage_status(void)
{
	return start_adjust_voltage;
}
EXPORT_SYMBOL(get_adjust_voltage_status);
static int get_battery_voltage(void)
{
	union power_supply_propval val;
	if (!battery_psy)
		battery_psy = power_supply_get_by_name ("battery"); 
	if (battery_psy 
		&& !battery_psy->get_property (battery_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val))
	{
		return (val.intval / 1000);
	}
	else
	{
		pr_err("can't get POWER_SUPPLY_PROP_VOLTAGE_NOW\n");
		return -1;
	}
}
static int get_battery_current(void)
{
	union power_supply_propval val;
	if (!battery_psy)
		battery_psy = power_supply_get_by_name ("battery"); 
	if (battery_psy 
		&& !battery_psy->get_property (battery_psy, POWER_SUPPLY_PROP_CURRENT_NOW, &val))
	{
		return (val.intval / 1000);
	}
	else
	{
		pr_err("can't get POWER_SUPPLY_PROP_CURRENT_NOW\n");
		return -1;
	}
}

#ifdef BATT_TEMP_SIMULATION 
static ssize_t batt_temp_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int val;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	batt_temp_simulation = val;

	bluewhale_dbg("batt_temp_simulation: %d ", batt_temp_simulation);

	return count;
}

ssize_t batt_temp_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", batt_temp_simulation);
}
#endif

static int otg_enable = 0;
static ssize_t bluewhale_otg_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int val;
	struct bluewhale_charger_info *charger;


	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	if (charger_info->dev == dev)
	    charger = dev_get_drvdata(dev);
	else //this device is the private device of power supply 
	    charger = dev_get_drvdata(dev->parent);

	if (val == 1 || val == 0)
	{
		bluewhale_enable_otg(charger, val);
		bluewhale_dbg("%s: %s OTG", __func__, val?"enable":"disable");
		otg_enable = val;

		return count;
	}
	else
		bluewhale_dbg("%s: wrong parameters", __func__);

	return -EINVAL;
}

ssize_t bluewhale_otg_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", otg_enable);
}

ssize_t bluewhale_show_vbus_volt(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", o2_read_vbus_voltage());
}

static int reg_enable = 0;

#define BLUEWHALE_PARAMS_ATTR(x)  \
static ssize_t bluewhale_write_##x(struct device *dev, struct device_attribute *attr, \
		const char *buf, size_t count) \
{ \
	int data; \
	struct bluewhale_charger_info *charger;\
	if (charger_info->dev == dev)\
	    charger = dev_get_drvdata(dev);\
	else\
	    charger = dev_get_drvdata(dev->parent);\
	if( kstrtoint(buf, 10, &data) == 0) { \
		if(reg_enable ==1)\
		    bluewhale_set_##x(charger, data);\
		return count; \
	}else { \
		pr_err("bluewhale reg %s set failed\n",#x); \
		return 0; \
	} \
	return 0;\
}\
static ssize_t bluewhale_read_##x(struct device *dev, struct device_attribute *attr, \
               char *buf) \
{ \
	struct bluewhale_charger_info *charger;\
	if (charger_info->dev == dev)\
	    charger = dev_get_drvdata(dev);\
	else\
	    charger = dev_get_drvdata(dev->parent);\
	charger->x = bluewhale_get_##x(charger);\
    return sprintf(buf,"%d\n", charger->x);\
}

BLUEWHALE_PARAMS_ATTR(vbus_current) //input current limit
BLUEWHALE_PARAMS_ATTR(charger_current) //charge current
BLUEWHALE_PARAMS_ATTR(chg_volt) //charge voltage

static ssize_t bluewhale_registers_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	int result = 0;
	u8 i = 0;
	u8 data = 0;
	struct bluewhale_charger_info *charger;

	if (charger_info->dev == dev)
	    charger = dev_get_drvdata(dev);
	else //this device is the private device of power supply 
	    charger = dev_get_drvdata(dev->parent);

    for (i=REG_CHARGER_VOLTAGE; i<=REG_MIN_VSYS_VOLTAGE; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", i, data);
    }
    for (i=REG_CHARGE_CURRENT; i<=REG_VBUS_LIMIT_CURRENT; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", i, data);
	}

	bluewhale_read_byte(charger, REG_SAFETY_TIMER, &data);
	result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", REG_SAFETY_TIMER, data);

	bluewhale_read_byte(charger, REG_CHARGER_CONTROL, &data);
	result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", REG_CHARGER_CONTROL, data);

    for (i=REG_VBUS_STATUS; i<=REG_THM_STATUS; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", i, data);
    }
	
	return result;
}

#ifdef BATT_TEMP_SIMULATION 
static DEVICE_ATTR(batt_temp, S_IRUGO | S_IWUGO, batt_temp_show, batt_temp_store);
#endif
static DEVICE_ATTR(registers, S_IRUGO, bluewhale_registers_show, NULL);

static DEVICE_ATTR(vbus_current, S_IWUGO|S_IRUGO, bluewhale_read_vbus_current, bluewhale_write_vbus_current);
static DEVICE_ATTR(charger_current, S_IWUGO|S_IRUGO, bluewhale_read_charger_current, bluewhale_write_charger_current);
static DEVICE_ATTR(chg_volt, S_IWUGO|S_IRUGO, bluewhale_read_chg_volt, bluewhale_write_chg_volt);
static DEVICE_ATTR(vbus_volt, S_IRUGO, bluewhale_show_vbus_volt, NULL);
static DEVICE_ATTR(enable_otg, S_IRUGO | S_IWUGO, bluewhale_otg_show, bluewhale_otg_store);

static struct attribute *bluewhale_attributes[] = {
	&dev_attr_registers.attr,
	&dev_attr_vbus_current.attr, //input current limit
	&dev_attr_charger_current.attr, //charge current
	&dev_attr_chg_volt.attr, //charge voltage
	&dev_attr_vbus_volt.attr,     //vbus voltage
	&dev_attr_enable_otg.attr, //enable/disable OTG(boost)
#ifdef BATT_TEMP_SIMULATION 
	&dev_attr_batt_temp.attr,
#endif
	NULL,
};

static struct attribute_group bluewhale_attribute_group = {
	.attrs = bluewhale_attributes,
};
static int bluewhale_create_sys(struct device *dev)
{
	int err;

	bluewhale_dbg("bluewhale_create_sysfs");
	
	if(NULL == dev){
		pr_err("creat bluewhale sysfs fail: NULL dev\n");
		return -EINVAL;
	}

	err = sysfs_create_group(&(dev->kobj), &bluewhale_attribute_group);

	if (err) 
	{
		pr_err("creat bluewhale sysfs group fail\n");
		return -EIO;
	}

	pr_err("creat bluewhale sysfs group ok\n");	
	return err;
}

static int charger_adjust_ilimit(struct bluewhale_charger_info *charger)
{
	int vbus_current;
	int ilimit[4] = {2000, 1500, 1000, 500};
	int i = 0;
	int ret;
	for (i = 0; i < 4; i++)
	{
		pr_info("set vbus_current:%d\n", ilimit[i]);
		ret = bluewhale_set_vbus_current(charger, ilimit[i]);
		if (ret < 0)
			pr_info("%s: fail to set vbus current, err %d\n", __func__, ret);
		msleep(200);
		vbus_current = bluewhale_get_vbus_current(charger);
		pr_info("get vbus_current:%d\n", vbus_current);
		if (ilimit[i] == vbus_current)
			break;
	}
	pr_info("finale vbus_current:%d\n", vbus_current);
	if (ret < 0)
		return ret;
	return vbus_current;
}

static void print_all_charger_info(struct bluewhale_charger_info *charger)
{
	pr_err("## bluewhale charger information ##\n"
		"chg_volt: %d, vbus_current: %d, charger_current: %d, eoc_current: %d\n"
		"vdc_pr: %d, vbus_ovp: %d, vbus_ok: %d, vbus_uvp: %d, vsys_ovp: %d,\n"
		"cc_timeout: %d, wak_timeout: %d, chg_full: %d, in_cc_state: %d, in_cv_state: %d\n"
		"initial_state: %d, in_wakeup_state: %d, thermal_status: %d\n"
		"start_adjust_voltage: %d, chg_adapter_type: %d\n",
		charger->chg_volt, charger->vbus_current,
		charger->charger_current,charger->eoc_current,
		charger->vdc_pr, charger->vbus_ovp, charger->vbus_ok,
		charger->vbus_uvp, charger->vsys_ovp,
		charger->cc_timeout, charger->wak_timeout, charger->chg_full,
		charger->in_cc_state, charger->in_cv_state,
		charger->initial_state,charger->in_wakeup_state,
		charger->thermal_status,
		start_adjust_voltage, chg_adapter_type);
}

static void print_all_register(struct bluewhale_charger_info *charger)
{
	u8 i = 0;
	u8 data = 0;

    pr_err("[bluewhale charger]:\n");

    for (i=REG_CHARGER_VOLTAGE; i<=REG_MIN_VSYS_VOLTAGE; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		pr_err("[0x%02x]=0x%02x ", i, data);        
    }
    for (i=REG_CHARGE_CURRENT; i<=REG_VBUS_LIMIT_CURRENT; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		pr_err("[0x%02x]=0x%02x ", i, data);        
	}

	bluewhale_read_byte(charger, REG_SAFETY_TIMER, &data);
	pr_err("[0x%02x]=0x%02x ", REG_SAFETY_TIMER, data);        

	bluewhale_read_byte(charger, REG_CHARGER_CONTROL, &data);
	pr_err("[0x%02x]=0x%02x ", REG_CHARGER_CONTROL, data);        

    for (i=REG_VBUS_STATUS; i<=REG_THM_STATUS; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		pr_err("[0x%02x]=0x%02x ", i, data);        
    }
    pr_err("\n");
}

static void bluewhale_get_setting_data(struct bluewhale_charger_info *charger)
{
	if (!charger)
		return;

	charger->chg_volt = bluewhale_get_chg_volt(charger);

	charger->charger_current = bluewhale_get_charger_current(charger);

	charger->vbus_current = bluewhale_get_vbus_current(charger);

	charger->eoc_current = bluewhale_get_eoc_current(charger);

	//bluewhale_dbg("chg_volt:%d, vbus_current: %d, charger_current: %d, eoc_current: %d", 
			//charger->chg_volt, charger->vbus_current, charger->charger_current, charger->eoc_current);

}

static int bluewhale_check_charger_status(struct bluewhale_charger_info *charger)
{
	u8 data_status;
	u8 data_thm = 0;
	u8 i = 0;
	int ret;
	
	/* Check charger status */
	ret = bluewhale_read_byte(charger, REG_CHARGER_STATUS, &data_status);
	if (ret < 0) {
		pr_err("%s: fail to get Charger status\n", __func__);
		return ret; 
	}

	charger->initial_state = (data_status & CHARGER_INIT) ? 1 : 0;
	charger->in_wakeup_state = (data_status & IN_WAKEUP_STATE) ? 1 : 0;
	charger->in_cc_state = (data_status & IN_CC_STATE) ? 1 : 0;
	charger->in_cv_state = (data_status & IN_CV_STATE) ? 1 : 0;
	charger->chg_full = (data_status & CHARGE_FULL_STATE) ? 1 : 0;
	charger->cc_timeout = (data_status & CC_TIMER_FLAG) ? 1 : 0;
	charger->wak_timeout = (data_status & WK_TIMER_FLAG) ? 1 : 0;
 
	/* Check thermal status*/
	ret = bluewhale_read_byte(charger, REG_THM_STATUS, &data_thm);
	if (ret < 0) {
		pr_err("%s: fail to get Thermal status\n", __func__);
		return ret; 
	}
	if (!data_thm)
		charger->thermal_status = THM_DISABLE;
	else {
		for (i = 0; i < 7; i ++) {
			if (data_thm & (1 << i))
				charger->thermal_status = i + 1;
		}
	}

	return ret;
}

static int bluewhale_check_vbus_status(struct bluewhale_charger_info *charger)
{
	int ret = 0;
	u8 data_status = 0;

	/* Check VBUS and VSYS */
	ret = bluewhale_read_byte(charger, REG_VBUS_STATUS, &data_status);

	pr_info("%s: status=0x%x\n", __func__, data_status);

	if(ret < 0) {
#if 0
		start_adjust_voltage = 0;
		chg_adapter_type = QUICK_CHG_ADAPTER_NULL;
		overheat_stop_charge = 0;
#endif
		pr_err("fail to get Vbus status\n");
	}

	charger->vdc_pr = (data_status & VDC_PR_FLAG) ? 1 : 0;
	charger->vsys_ovp = (data_status & VSYS_OVP_FLAG) ? 1 : 0;
	charger->vbus_ovp = (data_status & VBUS_OVP_FLAG) ? 1 : 0; //it's reserved for oz1c115
	charger->vbus_uvp = (data_status & VBUS_UVP_FLAG) ? 1 : 0;
	charger->vbus_ok = (data_status & VBUS_OK_FLAG) ? 1 : 0;

	if (!charger->vbus_ok)
		pr_err("invalid adapter or no adapter\n");

	if (charger->vdc_pr)
		bluewhale_dbg("vdc < vdc threshold of system priority");

	return ret > 0 ? charger->vbus_ok : ret;
}

static int bluewhale_get_charging_status(struct bluewhale_charger_info *charger)
{
	int ret = 0;
	int err = 0;
	static int get_status_count;

	bool last_vbus_ok;

	if (!charger)
		return -EINVAL;

	last_vbus_ok = charger->vbus_ok;

	ret = bluewhale_check_vbus_status(charger);

	if (last_vbus_ok != charger->vbus_ok)
	{
		charger->charger_changed = 1;
		pr_info("Set charger_changed!, charger->vbus_ok=%d\n",
				charger->vbus_ok);
	}
	ret = bluewhale_check_charger_status(charger);

	bluewhale_get_setting_data(charger);

	//print_all_register(charger);
	//print_all_charger_info(charger);

	return ret;
}

static int ir_compensation(struct bluewhale_charger_info *charger)
{
#if IR_COMPENSATION	
	int32_t ir;
	int32_t charge_volt;
	if(charger->vbus_ok && oz8806_get_init_status())
	{
		ir = get_battery_current() * charger->pdata->inpedance /1000;
		charge_volt = charger->pdata->max_charger_voltagemV + ir;

		charge_volt = charge_volt / 25 * 25;

		pr_err(KERN_WARNING"calculate IR volt  is %d\n", charge_volt);

		if(charge_volt < charger->pdata->max_charger_voltagemV)
			charge_volt = charger->pdata->max_charger_voltagemV;

		if(charge_volt > IR_COMPENSATION_MAX_VOLTAGE)
			charge_volt = IR_COMPENSATION_MAX_VOLTAGE;

		// when stop this ?

		if(charger->chg_volt != charge_volt)
			bluewhale_set_chg_volt(charger, charge_volt);
	}
#endif
	return 0;
}

/*****************************************************************************
 * Description:
 *		bluewhale_get_charging_status_extern 
 * Return:
 *		negative errno if error happens
 *****************************************************************************/
int bluewhale_get_charging_status_extern(void)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_get_charging_status(charger_info);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	if (ret < 0 || charger_info->vbus_ok == 0)
		return -1;
	return ret;
}

void bluewhale_dump_register(void)
{
	mutex_lock(&bluewhale_mutex);

	if (!charger_info) goto end;

	print_all_register(charger_info);
 
end:
	mutex_unlock(&bluewhale_mutex);
}


/*****************************************************************************
 * Description:
 *		bluewhale_init
 * Parameters:
 *		charger:	charger data
 * Return:
 *		negative errno if error happens
 *****************************************************************************/
int bluewhale_init(struct bluewhale_charger_info *charger)
{
	struct bluewhale_platform_data *pdata = NULL;

	int ret = 0;

  	pr_err("O2Micro bluewhale Driver bluewhale_init\n");

	if (!charger) return -EINVAL;
	
	pdata = charger->pdata;

	//*************************************************************************************
	//note: you must test usb type and set vbus limit current.
	//for wall charger you can set current more than 500mA
	// but for pc charger you may set current not more than 500 mA for protect pc usb port
	//************************************************************************************/

	// write rthm  100k
	ret = bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x1, pdata->rthm_10k);

	//set chager PWM TON time
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x0 << 3);

	/* Min VSYS:3.6V */
    ret = bluewhale_set_min_vsys(charger, pdata->min_vsys_voltagemV);	
	
	//CV voltage
	ret = bluewhale_set_chg_volt(charger, pdata->max_charger_voltagemV);

	// useless,we will make this later
	//set ilmit
	bluewhale_set_vbus_current(charger, pdata->vbus_limit_currentmA);
	//set charging current, CC
	bluewhale_set_charger_current(charger, pdata->max_charger_currentmA);

	bluewhale_set_eoc_current(charger, pdata->termination_currentmA);

	bluewhale_set_safety_cc_timer(charger, pdata->max_cc_charge_time);

	bluewhale_set_rechg_hystersis(charger, pdata->recharge_voltagemV);
	bluewhale_set_t34_cv(charger, pdata->T34_charger_voltagemV);
	bluewhale_set_t45_cv(charger, pdata->T45_charger_voltagemV);


	/* Max CHARGE VOLTAGE:4.2V */
	/* T34 CHARGE VOLTAGE:4.15V */
	/* T45 CHARGE VOLTAGE:4.1V */
	/* WAKEUP VOLTAGE:2.5V */
	/* WAKEUP CURRENT:0.1A */
	/* Max CHARGE VOLTAGE:4.2V */
	/* EOC CHARGE:0.05A */
	/* RECHG HYSTERESIS:0.1V */
	/* MAX CC CHARGE TIME:180min */
	/* MAX WAKEUP CHARGE TIME:30min */
	/* RTHM 10K/100K:0 (100K) */
	//bluewhale_get_charging_status(charger_info);
	
	
	//bluewhale_set_vbus_current(1500);
	//bluewhale_set_charger_current(3000);
#ifdef O2_EC_ADAPTER_SUPPORT
	o2_adapter_ec_init();
#endif
	
#ifdef MTK_ADAPTER_SUPPORT
	o2_adapter_mtk_init();
#endif
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_init_extern
 * Parameters:
 *		None
 * Return:
 *		negative errno if error happens
 *****************************************************************************/
int bluewhale_init_extern(void)
{
	int ret = 0;
	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_init(charger_info);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}

enum vbus_states {
        VBUS_ENABLE,
        VBUS_DISABLE,
        MAX_VBUSCTRL_STATES,
};


/*****************************************************************************
 * Description:
 *		bluewhale_charger_set_property
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int bluewhale_charger_set_property(struct power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
	struct bluewhale_charger_info *charger = 
			container_of(psy, struct bluewhale_charger_info, usb);
	int ret = 0;

	mutex_lock(&charger->lock);
	pr_info("%s: psp=%d, value_to_set=%d\n", __func__, psp, val->intval);

	switch (psp) {
		case POWER_SUPPLY_PROP_PRESENT:
			charger->present = val->intval;
			break;
		case POWER_SUPPLY_PROP_ONLINE:
			charger->online = val->intval;
			break;
		case POWER_SUPPLY_PROP_ENABLE_CHARGING:
			if (val->intval && (charger->is_charging_enabled != val->intval))
				o2_start_usb_charge(charger);
			else if ((!val->intval) && (charger->is_charging_enabled != val->intval))
				o2_stop_charge(charger);	
			charger->is_charging_enabled = val->intval;
			break;
		case POWER_SUPPLY_PROP_ENABLE_CHARGER:
                charger->is_charger_enabled = val->intval;
                break;
		case POWER_SUPPLY_PROP_CHARGE_CURRENT:
				charger->charger_current = val->intval;
				bluewhale_set_charger_current(charger, val->intval);
				break;
		case POWER_SUPPLY_PROP_CHARGE_VOLTAGE:
				charger->chg_volt = val->intval;
				bluewhale_set_chg_volt(charger, val->intval);
				break;
		case POWER_SUPPLY_PROP_CHARGE_TERM_CUR:
				charger->eoc_current = val->intval;
				bluewhale_set_eoc_current(charger, val->intval);
				break;
		case POWER_SUPPLY_PROP_CABLE_TYPE:
				charger->usb.type = get_power_supply_type(val->intval);
				charger->cable_type = charger->usb.type;
				break;
		case POWER_SUPPLY_PROP_INLMT:
				charger->vbus_current = val->intval;
				bluewhale_set_vbus_current(charger, val->intval);	
				break;
		case POWER_SUPPLY_PROP_MAX_TEMP:
				break;
		case POWER_SUPPLY_PROP_MIN_TEMP:
				break;
		case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
				break;
		case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
                break;
        default:
                ret = -EINVAL;
	}
	mutex_unlock(&charger->lock);
	return ret;
}

	
/*****************************************************************************
 * Description:
 *		bluewhale_charger_get_property
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int bluewhale_charger_get_property(struct power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
	struct bluewhale_charger_info *charger = 
			container_of(psy, struct bluewhale_charger_info, usb);
	struct bluewhale_platform_data *pdata = NULL;
	int ret = 0;

	mutex_lock(&charger->lock);
	pdata = charger->pdata;

	switch (psp) {
		case POWER_SUPPLY_PROP_PRESENT:
                	val->intval = charger->present;
                	break;
		case POWER_SUPPLY_PROP_ONLINE:
			bluewhale_get_charging_status(charger);
			val->intval = charger->vbus_ok ? 1 : 0;	
               		break;
        	case POWER_SUPPLY_PROP_STATUS:
#if 0	/* Does STATUS needed? */
    			if (charger->chg_full) {
    				val->intval = POWER_SUPPLY_STATUS_FULL;
    			}
    			else if ((charger->in_cc_state) || (charger->in_cv_state)
    					|| (charger->in_wakeup_state)) {
    				val->intval = POWER_SUPPLY_STATUS_CHARGING; 
				}
			else
				val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
#endif
	   		break;
		case POWER_SUPPLY_PROP_HEALTH:
			bluewhale_get_charging_status(charger);
    			if ((charger->vbus_ovp) || (charger->vsys_ovp))	//ov check
    				val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
    			else if (charger->thermal_status == 0)			//thm disable
    				val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
    			else if (charger->thermal_status == THM_UNDER_T1)	//cold
    				val->intval = POWER_SUPPLY_HEALTH_COLD;
    			else if ((charger->thermal_status == THM_OVER_T5)	//heat
    					|| (charger->thermal_status == THM_ITOT))
    				val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
    			else
    				val->intval = POWER_SUPPLY_HEALTH_GOOD;
					
        		break;
		case POWER_SUPPLY_PROP_MAX_CHARGE_CURRENT:
			val->intval = pdata->max_charger_currentmA;
			break;
		case POWER_SUPPLY_PROP_MAX_CHARGE_VOLTAGE:
			val->intval = pdata->max_charger_voltagemV;
			break;
		case POWER_SUPPLY_PROP_CHARGE_CURRENT:
			val->intval = charger->charger_current;
			break;
		case POWER_SUPPLY_PROP_CHARGE_VOLTAGE:
			val->intval = charger->chg_volt;
			break;
		case POWER_SUPPLY_PROP_INLMT:
			val->intval = charger->vbus_current;
			break;
		case POWER_SUPPLY_PROP_CHARGE_TERM_CUR:
			val->intval = charger->eoc_current;
			break;
		case POWER_SUPPLY_PROP_CABLE_TYPE:
			val->intval = charger->cable_type;
			break;
		case POWER_SUPPLY_PROP_ENABLE_CHARGING:
			val->intval = charger->is_charging_enabled;
			break;
		case POWER_SUPPLY_PROP_ENABLE_CHARGER:
			val->intval = charger->is_charger_enabled;
			break;
		case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
			val->intval = MAX_VBUSCTRL_STATES;
			break;
		case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
			val->intval = VBUS_ENABLE;
			break;
		case POWER_SUPPLY_PROP_MAX_TEMP:
			val->intval = 45;
			break;
		case POWER_SUPPLY_PROP_MIN_TEMP:
			val->intval = 0;
			break;

		default:
               	ret = -EINVAL;
	}
	pr_info("%s: psp=%d, value=%d\n", __func__, psp, val->intval);
	mutex_unlock(&charger->lock);
	return ret;
}

/*****************************************************************************
 * Description:
 *		o2_start_ac_charge
 *****************************************************************************/
static void o2_start_ac_charge(struct bluewhale_charger_info *charger)
{
	struct bluewhale_platform_data *pdata = NULL;
	int ret = 0;
	
	if (!charger) return;

	pdata = charger->pdata;
	pr_err("start ac charge\n");

    //disable suspend mode
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0 << 1);
    //enable system over-power protection
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x80, 0x1 << 7);

	//init charging setting which may be changed 
	ret = bluewhale_set_chg_volt(charger, pdata->max_charger_voltagemV);
    ret = bluewhale_set_vbus_current(charger, pdata->vbus_limit_currentmA);
	ret = bluewhale_set_charger_current(charger, pdata->max_charger_currentmA);
	ret = bluewhale_set_eoc_current(charger, pdata->termination_currentmA);

	pr_err("normal charge adpter\n");
	chg_adapter_type = QUICK_CHG_ADAPTER_NORMAL;
	bluewhale_set_vbus_current(charger, TARGET_NORMAL_CHG_ILIMIT);
	bluewhale_set_charger_current(charger, TARGET_NORMAL_CHG_CURRENT);
}

/*****************************************************************************
 * Description:
 *		o2_start_ac_charge_extern
 * Parameters:
 *		None
 * Return:
 *      negative errno if any error
 *****************************************************************************/
int o2_start_ac_charge_extern(void)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		o2_start_ac_charge(charger_info);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}

/*****************************************************************************
 * Description:
 *		o2_start_usb_charge
 * Parameters:
 *		None
 *      negative errno if any error
 *****************************************************************************/
static void o2_start_usb_charge(struct bluewhale_charger_info *charger)
{
	struct bluewhale_platform_data *pdata = NULL;

	int ret = 0;
	
	pr_err("start usb charge\n");

	if (!charger) return;

	pdata = charger->pdata;

    //disable suspend mode
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0 << 1);

    //enable system over-power protection
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x80, 0x1 << 7);

	ret = bluewhale_set_chg_volt(charger, pdata->max_charger_voltagemV);
	
	ret  = bluewhale_set_vbus_current(charger, 500);
	bluewhale_dbg("Platform data ILMT value: %d", 500);
	
	/* the minimal data is 600 ma, we use vubs to limit it at 500ma*/
	ret = bluewhale_set_charger_current(charger, 1000);
	
	/* CHARGER CURRENT: 1A */
	//ret = seaelf_set_eoc_current(pdata->termination_currentmA);
	ret = bluewhale_set_eoc_current(charger, 80);
	bluewhale_set_safety_cc_timer(charger, pdata->max_cc_charge_time);

	chg_adapter_type = QUICK_CHG_ADAPTER_PC;
}

/*****************************************************************************
 * Description:
 *		o2_start_usb_charge_extern
 * Parameters:
 *		None
 * Return:
 *      negative errno if any error
 *****************************************************************************/
int o2_start_usb_charge_extern(void)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		o2_start_usb_charge(charger_info);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
/*****************************************************************************
 * Description:
 *		o2_stop_charge, this is used when plug out adapter cable
 * Parameters:
 *		None
 *****************************************************************************/
static void o2_stop_charge(struct bluewhale_charger_info *charger)
{
	int ret = 0;

	pr_err("%s !!\n", __func__);

	ret = bluewhale_set_charger_current(charger, STOP_CHG_CURRENT);
	ret = bluewhale_set_vbus_current(charger, STOP_CHG_ILIMIT);

	//disable system over-power protection
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x80, 0x0 << 7);

    //enable suspend mode
	//bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0x1 << 1);

	//disable ki2c
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x0 << 3);

	chg_adapter_type = QUICK_CHG_ADAPTER_NULL;

	pr_err("%s return !!\n", __func__);
}

/*
 * stop charging when adapter is in
 */
static void bluewhale_stop_charge(struct bluewhale_charger_info *charger)
{
	int ret = 0;

	pr_err("%s\n", __func__);

	//disable charging, and charger only powers system
	ret = bluewhale_set_charger_current(charger, 0);

	//ret = bluewhale_set_vbus_current(charger, STOP_CHG_ILIMIT);
}
/*****************************************************************************
 * Description:
 *		o2_stop_charge_extern
 * Parameters:
 *		None
 * Return:
 *      negative errno if any error
 *****************************************************************************/
int o2_stop_charge_extern(void)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		o2_stop_charge(charger_info);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}

static void oz_irq_worker(struct work_struct *work)
{
        int reg_status, reg_fault, ret;
	struct bluewhale_charger_info *charger = container_of(work,
                                                    struct bluewhale_charger_info,
                                                    irq_work.work);
	pr_info("%s: Enter!\n", __func__);
	
}

/* IRQ handler for charger Interrupts configured to GPIO pin */
static irqreturn_t oz_irq_isr(int irq, void *devid)
{
	struct bluewhale_charger_info *charger = (struct bluewhale_charger_info *)devid;
        /**TODO: This hanlder will be used for charger Interrupts */
        return IRQ_WAKE_THREAD;
}

/* IRQ handler for charger Interrupts configured to GPIO pin */
static irqreturn_t oz_irq_thread(int irq, void *devid)
{
	struct bluewhale_charger_info *charger = (struct bluewhale_charger_info *)devid;
        schedule_delayed_work(&charger->irq_work, 0);
        return IRQ_HANDLED;
}

/*****************************************************************************
 * Description:
 *		bluewhale_charger_probe
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int bluewhale_charger_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct bluewhale_charger_info *charger;
	int ret = 0;
	int irq;
	struct gpio_desc *gpio;
	
	pr_err("O2Micro bluewhale Driver Loading\n");
	charger = devm_kzalloc(&client->dev, sizeof(*charger), GFP_KERNEL);
	if (!charger) {
		pr_err("Can't alloc charger struct\n");
		return -ENOMEM;
	}
	//for public use
	charger_info = charger;
	mutex_init(&charger->lock);	
	charger->pdata = &bluewhale_pdata_default;
	charger->dev = &client->dev;
	
#ifdef O2_CHARGER_POWER_SUPPOLY_SUPPORT
	charger->usb.name = "usb";
	charger->usb.type = POWER_SUPPLY_TYPE_USB;
	charger->usb.properties = bluewhale_charger_props;
	charger->usb.num_properties = ARRAY_SIZE(bluewhale_charger_props);
	charger->usb.get_property = bluewhale_charger_get_property;
	charger->usb.set_property = bluewhale_charger_set_property;
	charger->usb.supplied_to = charger->pdata->supplied_to;
	charger->usb.num_supplicants = ARRAY_SIZE(bluewhale_chrg_supplied_to);
	charger->usb.supported_cables = POWER_SUPPLY_CHARGER_TYPE_USB;
#endif
	charger->polling_invl = msecs_to_jiffies(5000);
	
	i2c_set_clientdata(client, charger);
	charger->client = client;	
	charger->client->addr = 0x10;
	pr_err("%s: i2c_client name=%s, addr=0x%x\n", __func__, charger->client->name, charger->client->addr);

	pr_err("O2Micro bluewhale Driver Loading 1\n");	
#ifdef O2_CHARGER_POWER_SUPPOLY_SUPPORT
	ret = power_supply_register(charger->dev, &charger->usb);

	if (ret < 0) {
		goto err_free;
	}
	ret = bluewhale_create_sys(charger->usb.dev);
#else
	//sys/class/i2c-dev/i2c-2/device/2-0010/
	ret = bluewhale_create_sys(&client->dev);
#endif
	if(ret){
		pr_err("Err failed to creat charger attributes\n");
		goto sys_failed;
	}

	bluewhale_init(charger);

	//INIT_DELAYED_WORK(&charger->delay_work, bluewhale_charger_work);
	//schedule_delayed_work(&charger->delay_work, charger->polling_invl);

	INIT_DELAYED_WORK(&charger->irq_work, oz_irq_worker);
	/* Initialize the wakelock */
	wake_lock_init(&charger->wakelock, WAKE_LOCK_SUSPEND,
			"OZ_charger_wakelock");

#ifdef CONFIG_ACPI
	gpio = devm_gpiod_get_index(charger->dev, "chrg_int", 0);

	pr_err("%s: gpio=%d\n", __func__, desc_to_gpio(gpio));

        if (IS_ERR(gpio)) {
                pr_err("acpi gpio get index failed\n");
                i2c_set_clientdata(client, NULL);
                return PTR_ERR(gpio);
        } else {
                ret = gpiod_direction_input(gpio);
                if (ret < 0)
                        pr_err("OZ1C115C CHRG set direction failed\n");

                irq = gpiod_to_irq(gpio);
                pr_err("%s: irq=%d\n", __func__, irq);
        }
#endif

        ret = request_threaded_irq(irq,
                                oz_irq_isr, oz_irq_thread,
                                IRQF_TRIGGER_FALLING, "OZ1C115 IRQ", charger);

	pr_info("O2Micro bluewhale Driver Loading 3\n");
	return 0;

sys_failed:
#ifdef O2_CHARGER_POWER_SUPPOLY_SUPPORT
	power_supply_unregister(&charger->usb);
#endif
err_free:

//	cancel_delayed_work(&charger->delay_work);
	
	return ret;
}


/*****************************************************************************
 * Description:
 *		bluewhale_charger_shutdown
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static void bluewhale_charger_shutdown(struct i2c_client *client)
{
	struct bluewhale_charger_info *charger = i2c_get_clientdata(client);
	
	pr_err("%s\n", __func__);
#ifdef O2_CHARGER_POWER_SUPPOLY_SUPPORT
	power_supply_unregister(&charger->usb);
#endif
}

static int bluewhale_charger_remove(struct i2c_client *client)
{
	struct bluewhale_charger_info *charger = i2c_get_clientdata(client);
	
	pr_err("%s\n", __func__);
#ifdef O2_CHARGER_POWER_SUPPOLY_SUPPORT
	power_supply_unregister(&charger->usb);
#endif
	return 0;
}

/*****************************************************************************
 * Description:
 *		bluewhale_charger_suspend
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int bluewhale_charger_suspend(struct device *dev)
{
	//struct platform_device * const pdev = to_platform_device(dev);
	//struct bluewhale_charger_info * const charger = platform_get_drvdata(pdev);

	return 0;
}


/*****************************************************************************
 * Description:
 *		bluewhale_charger_resume
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int bluewhale_charger_resume(struct device *dev)
{
	//struct platform_device * const pdev = to_platform_device(dev);
	//struct bluewhale_charger_info * const charger = platform_get_drvdata(pdev);
	return 0;
}

static const struct i2c_device_id bluewhale_i2c_ids[] = {
		{"EXCG0000", 0},
		{}
}; 

MODULE_DEVICE_TABLE(i2c, bluewhale_i2c_ids);


#ifdef CONFIG_ACPI
static struct acpi_device_id extchrg_acpi_match[] = {
        {"EXCG0000", 0 },
        {}
};
MODULE_DEVICE_TABLE(acpi, extchrg_acpi_match);
#endif


static const struct dev_pm_ops pm_ops = {
        .suspend        = bluewhale_charger_suspend,
        .resume			= bluewhale_charger_resume,
};

static struct i2c_driver bluewhale_charger_driver = {
		.driver		= {
			.name 		= "bluewhale-charger",
			.pm			= &pm_ops,
#ifdef CONFIG_ACPI
			.acpi_match_table = ACPI_PTR(extchrg_acpi_match),
#endif
		},
		.probe		= bluewhale_charger_probe,
		.shutdown	= bluewhale_charger_shutdown,
		.remove     = bluewhale_charger_remove,
		.id_table	= bluewhale_i2c_ids,
};



static int __init bluewhale_charger_init(void)
{
	pr_err("bluewhale driver init\n");
	return i2c_add_driver(&bluewhale_charger_driver);
}

static void __exit bluewhale_charger_exit(void)
{
	pr_err("bluewhale driver exit\n");
	
	i2c_del_driver(&bluewhale_charger_driver);
}

device_initcall(bluewhale_charger_init);
module_exit(bluewhale_charger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("O2Micro");
MODULE_DESCRIPTION("O2Micro BlueWhale Charger Driver");
