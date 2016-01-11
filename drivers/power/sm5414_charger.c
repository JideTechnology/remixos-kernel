/*
        oonst struct acpi_device_id *acpi_id;
 * sm5414_charger.c - Charger driver for Siliconmitus
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
#include "sm5414_charger.h"
//#include "o2_adapter.h"
#include "power_supply.h"

#define DEBUG_LOG   		1

#define	RETRY_CNT	5

static char *sm5414_chrg_supplied_to[] = {
        "dollar_cove_battery"
};

static struct sm5414_platform_data  sm5414_pdata_default = {
        .max_charger_currentmA = 2500, //FASTCHG
        .max_charger_voltagemV = 4200, //BATREG
        .topoff_currentmA = 420,       //Toppoff 
		.vbus_limit_currentmA = 1800,  //VBUSLIMIT 
		.supplied_to = sm5414_chrg_supplied_to
};

static enum power_supply_property sm5414_charger_props[] = {
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

static struct sm5414_charger_info *charger_info;

void sm5414_get_setting_data(void);
void sm5414_start_ac_charge(void);
void sm5414_start_usb_charge(void);
void sm5414_stop_charge(void);

int32_t sm5414_read_vbus_voltage(void);
extern  int sm5414_init(void);;
extern int sm5414_get_charging_status(struct sm5414_charger_info *charger);

/*****************************************************************************
 * Description:
 *		sm5414_read_byte 
 * Parameters:
 *		index:	register index to be read
 *		*dat:	buffer to store data read back
 * Return:
 *		I2C_STATUS_OK if read success, else inidcate error
 *****************************************************************************/
int32_t sm5414_read_byte(uint8_t index, uint8_t *dat)
{
	int32_t ret = 0;
	int i;

	for(i = 0; i < RETRY_CNT; i++){
		ret = i2c_smbus_read_byte_data(charger_info->client, index);
		if(ret >= 0) break;
	}
	if(i >= RETRY_CNT)
	{
		pr_err("%s: read fail!\n", __func__);
		return ret;
	} 
	*dat = (uint8_t)ret;

	return ret;
}


/*****************************************************************************
 * Description:
 *		sm5414_write_byte 
 * Parameters:
 *		index:	register index to be write
 *		dat:		write data
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int32_t sm5414_write_byte(uint8_t index, uint8_t dat)
{
	int32_t ret;
	int i;
	
	for(i = 0; i < RETRY_CNT; i++){
		ret = i2c_smbus_write_byte_data(charger_info->client, index,dat);
		if(ret >= 0) break;
	}
	if(i >= RETRY_CNT)
	{
		return ret;
	}

	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5414_assign_bits 
 * Parameters:
 *		reg: register index to be write
 *		dat:		write data
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int32_t sm5414_assign_bits(uint8_t reg, uint8_t mask, uint8_t shift, uint8_t data)
{
	int8_t value;
	uint32_t ret;

	ret = sm5414_read_byte(reg, &value);

    if(ret < 0)
        return ret;

	value &= ~(mask << shift);
	value |= (data << shift);
    
    ret = sm5414_write_byte(reg, value);

	return ret;
}

static void sm5414_dump_read(void)
{
	uint8_t data;
	int i;

    pr_info("%s: dump start\n", __func__);

	for (i = REG_INTMSK1; i <= REG_CHGCTRL5; i++) {
		sm5414_read_byte(i, &data);
		pr_info("0x%0x = 0x%02x, ", i, data);
	}
    
	pr_info("%s: dump end\n", __func__);
}


/*****************************************************************************
 * Description:
 *		sm5414_set_chg_volt 
 * Parameters:
 *		chgvolt_mv:	charge voltage to be written
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
//BATREG
static int sm5414_set_chg_volt(int chgvolt_mv)
{
	int ret = 0;
	u8 chg_volt = 0; 
    
	if (chgvolt_mv < BATREG_MIN_MV)
		chgvolt_mv = BATREG_MIN_MV;		    //limit to 4.1V
	else if (chgvolt_mv > BATREG_MAX_MV)
		chgvolt_mv = BATREG_MAX_MV;		    //limit to 4.475V
		
	chg_volt = (chgvolt_mv - 4100) / BATREG_STEP_MV;	//step is 25mV
	pr_info("%s: chg_mv=%d, chg_volt set = 0x%x\n", __func__, chgvolt_mv, chg_volt);
    
	ret = sm5414_assign_bits(REG_CHGCTRL3, BATREG_MASK, BATREG_SHIFT, chg_volt);	
        
	return ret;	
}

/*****************************************************************************
 * Description:
 *		sm5414_get_chg_volt 
 * Return:
 *		BATREG voltage in mV
 *****************************************************************************/
static int sm5414_get_chg_volt(void)
{
	int ret = 0;
	u8 data = 0;
	u8 chg_volt = 0;     		
    int chgvolt_mv = 0;
    
	ret = sm5414_read_byte(REG_CHGCTRL3, &data);
    if (ret < 0)
        return ret;
    
    chg_volt = (data >> BATREG_SHIFT) & BATREG_MASK;

    chgvolt_mv = BATREG_MIN_MV + (chg_volt * BATREG_STEP_MV);

	if (chgvolt_mv < BATREG_MIN_MV)
		chgvolt_mv = BATREG_MIN_MV;		    //limit to 4.1V
	else if (chgvolt_mv > BATREG_MAX_MV)
		chgvolt_mv = BATREG_MAX_MV;		    //limit to 4.475V

	pr_info("%s: chg_mv=%d, chg_volt set = 0x%x\n", __func__, chgvolt_mv, chg_volt);
            
	return chgvolt_mv;	
}

/*****************************************************************************
 * Description:
 *		sm5414_set_topoff_current 
 * Parameters:
 *		topoff_ma:	set end of charge current
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5414_set_topoff_current(int topoff_ma)
{
	int ret = 0;
	u8 topoff_curr = 0;
    
	if (topoff_ma <= TOPPOFF_MIN_MA)
		topoff_ma = TOPPOFF_MIN_MA;		//min value is 100mA
	else if (topoff_ma > TOPPOFF_MAX_MA)
		topoff_ma = TOPPOFF_MAX_MA;		    //limit to 650mA		

	topoff_curr = (topoff_ma - 100) / TOPPOFF_STEP_MA;	//step is 50mA
	pr_info("%s: topoff_ma=%d, topoff_curr set = 0x%x\n", __func__, topoff_ma, topoff_curr);

    ret = sm5414_assign_bits(REG_CHGCTRL4, TOPOFF_MASK, TOPOFF_SHIFT ,topoff_curr);
    
	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5414_get_topoff_current 
 * Return:
 *		Topoff current in mA
 *****************************************************************************/
static int sm5414_get_topoff_current(void)
{
	int ret = 0;
	u8 data = 0;
	u8 topoff_curr = 0;     		
    int topoff_ma = 0;
    
	ret = sm5414_read_byte(REG_CHGCTRL4, &data);
    if (ret < 0)
        return ret;
    
    topoff_curr = (data >> TOPOFF_SHIFT) & TOPOFF_MASK;

    topoff_ma = TOPPOFF_MIN_MA + (topoff_curr * TOPPOFF_STEP_MA);

	if (topoff_ma <= TOPPOFF_MIN_MA)
		topoff_ma = TOPPOFF_MIN_MA;		//min value is 100mA
	else if (topoff_ma > TOPPOFF_MAX_MA)
		topoff_ma = TOPPOFF_MAX_MA;		    //limit to 650mA		

	pr_info("%s: topoff_ma=%d, topoff_curr set = 0x%x\n", __func__, topoff_ma, topoff_curr);
            
	return topoff_ma;	
}

/*****************************************************************************
 * Description:
 *		sm5414_set_vbus_limit_current 
 * Parameters:
 *		ilmt_ma:	set input current limit
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5414_set_vbus_limit_current(int ilmt_ma)
{
	int ret = 0;
	u8 ilmt_curr = 0;
    
	if (ilmt_ma <= VBUSLIMIT_MIN_MA)
		ilmt_ma = VBUSLIMIT_MIN_MA;
	else if (ilmt_ma > VBUSLIMIT_MAX_MA)
		ilmt_ma = VBUSLIMIT_MAX_MA;

	ilmt_curr = (ilmt_ma - VBUSLIMIT_MIN_MA) / VBUSLIMIT_STEP_MA;
	pr_info("%s: ilmt_ma=%d, ilmt_curr set = 0x%x\n", __func__, ilmt_ma, ilmt_curr);

    ret = sm5414_assign_bits(REG_VBUSCTRL, VBUSLIMIT_MASK, VBUSLIMIT_SHIFT ,ilmt_curr);
    
	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5414_get_vbus_limit_current 
 * Return:
 *		Vbus input current in mA
 *****************************************************************************/
int sm5414_get_vbus_limit_current(void)
{
	int ret = 0;
	u8 data = 0;
	u8 ilmt_curr = 0;     		
    int ilmt_ma = 0;
    
	ret = sm5414_read_byte(REG_VBUSCTRL, &data);
    if (ret < 0)
        return ret;
    
    ilmt_curr = (data >> VBUSLIMIT_SHIFT) & VBUSLIMIT_MASK;

    ilmt_ma = VBUSLIMIT_MIN_MA + (ilmt_curr * VBUSLIMIT_STEP_MA);

	if (ilmt_ma <= VBUSLIMIT_MIN_MA)
		ilmt_ma = VBUSLIMIT_MIN_MA;
	else if (ilmt_ma > VBUSLIMIT_MAX_MA)
		ilmt_ma = VBUSLIMIT_MAX_MA;		

	pr_info("%s: ilmt_ma=%d, ilmt_curr set = 0x%x\n", __func__, ilmt_ma, ilmt_curr);
            
	return ilmt_ma;	

}

/*****************************************************************************
 * Description:
 *		sm5414_set_fastchg_current
 * Parameters:
 *		chg_ma:	set charger current
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5414_set_fastchg_current(int chg_ma)
{
	int ret = 0;
	u8 chg_curr = 0;
    
	if (chg_ma <= FASTCHG_MIN_MA)
		chg_ma = FASTCHG_MIN_MA;
	else if (chg_ma > FASTCHG_MAX_MA)
		chg_ma = FASTCHG_MAX_MA;

	chg_curr = (chg_ma - FASTCHG_MIN_MA) / FASTCHG_STEP_MA;
	pr_info("%s: chg_ma=%d, chg_curr set = 0x%x\n", __func__, chg_ma, chg_curr);

    ret = sm5414_assign_bits(REG_CHGCTRL2, FASTCHG_MASK, FASTCHG_SHIFT ,chg_curr);
    
	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5414_get_fastchg_current 
 * Return:
 *		Fast charging current in mA
 *****************************************************************************/
int sm5414_get_fastchg_current(void)
{
	int ret = 0;
	u8 data = 0;
	u8 chg_curr = 0;     		
    int chg_ma = 0;
    
	ret = sm5414_read_byte(REG_CHGCTRL2, &data);
    if (ret < 0)
        return ret;
    
    chg_curr = (data >> FASTCHG_SHIFT) & FASTCHG_MASK;

    chg_ma = FASTCHG_MIN_MA + (chg_curr * FASTCHG_STEP_MA);

	if (chg_ma <= FASTCHG_MIN_MA)
		chg_ma = FASTCHG_MIN_MA;
	else if (chg_ma > FASTCHG_MAX_MA)
		chg_ma = FASTCHG_MAX_MA;		

	pr_info("%s: chg_ma=%d, chg_curr set = 0x%x\n", __func__, chg_ma, chg_curr);
            
	return chg_ma;	
}

/*****************************************************************************
 * Description:
 *		sm5414_enable_otg 
 * Parameters:
 *		enable:	enable otg function
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5414_enable_otg(int enable)
{
	int ret = 0;

	pr_info("%s: OTG = %d\n", __func__, enable);

    //Before turing on OTG function, you must turn off charging function.
    sm5414_stop_charge();   

	ret = sm5414_assign_bits(REG_CTRL, ENBOOST_MASK, ENBOOST_SHIFT ,enable);
    
	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5414_enable_chgen 
 * Parameters:
 *		enable:	enable otg function
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5414_enable_chgen(int enable)
{
	int ret = 0;

	pr_info("%s: CHGEN = %d\n", __func__, enable);

    //SM : After setting nCHGEN GPIO, plz change way to control charging function to GPIO(nCHGNE)
	ret = sm5414_assign_bits(REG_CTRL, CHGEN_MASK, CHGEN_SHIFT ,enable);
    
	return ret;
}


void sm5414_get_setting_data(void)
{	
	if (!charger_info->client)
		return;

	charger_info->chg_volt = sm5414_get_chg_volt();

	charger_info->charger_current = sm5414_get_fastchg_current();
	
	charger_info->vbus_current = sm5414_get_vbus_limit_current();

	charger_info->eoc_current = sm5414_get_topoff_current();

	pr_info("sm5414_get_setting_data \n");
	
	pr_info( "## sm5414 charger information ##\n"
		"chg_volt: %d, vbus_current: %d, charge_current: %d\n"
		"eoc_current: %d, vbus_ovp: %d, vbus_uvp: %d\n"
		"vbus_ok: %d, chg_full: %d\n",
		charger_info->chg_volt, charger_info->vbus_current,
		charger_info->charger_current,charger_info->eoc_current,
		charger_info->vbus_ovp, charger_info->vbus_uvp,
		charger_info->vbus_ok, charger_info->chg_full);
		
		pr_info("## All data register ##\n");
		sm5414_dump_read();
}

static int sm5414_get_charger_status(struct sm5414_charger_info *charger)
{
	int ret = 0;
	u8 data_status = 0;
	if (!charger)
		return -EINVAL;

	/* Check VBUS */
	ret = sm5414_read_byte(REG_STATUS, &data_status);
	pr_info("%s: status=0x%x\n", __func__, data_status);

    if( (ret < 0) || (data_status & (0x1 << S_VBUSOVP_SHIFT)) || (data_status & (0x1 << S_VBUSUVLO_SHIFT)) ) {
        charger->vbus_ok = 0;
        pr_info("VBUS not OK\n");                
        return ret;
    }

    charger->vbus_ok = (!(data_status & (0x1 << S_VBUSOVP_SHIFT)) && !(data_status & (0x1 << S_VBUSUVLO_SHIFT))) ? 1 : 0;
    charger->vbus_ovp = (data_status & (0x1 << S_VBUSOVP_SHIFT)) ? 1 : 0;
    charger->vbus_uvp = (data_status & (0x1 << S_VBUSUVLO_SHIFT)) ? 1 : 0;
    if (charger->vbus_ok) {
        charger->charger_changed = 1;
        pr_info("Set charger_changed!, charger->vbus_ok=%d, data_status=0x%x\n", charger->vbus_ok, data_status);
	}

    /* Check charger status */
    charger->chg_full = (data_status & (0x1 << S_TOPOFF_SHIFT)) ? 1 : 0;
    
	return ret;	
}

/*****************************************************************************
 * Description:
 *		sm5414_get_charging_status 
 * Parameters:
 *		charger:	charger data
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5414_get_charging_status(struct sm5414_charger_info *charger)
{
	int ret = 0;
	u8 data_status = 0;

	if (!charger)
		return -EINVAL;

	/* Check VBUS */
	ret = sm5414_read_byte(REG_STATUS, &data_status);
	pr_info("%s: status=0x%x\n", __func__, data_status);

    if( (ret < 0) || (data_status & (0x1 << S_VBUSOVP_SHIFT)) || (data_status & (0x1 << S_VBUSUVLO_SHIFT)) ) {
        charger->vbus_ok = 0;
        pr_info("VBUS not OK\n");                
        return ret;
    }
	
	charger->vbus_ok = (!(data_status & (0x1 << S_VBUSOVP_SHIFT)) && !(data_status & (0x1 << S_VBUSUVLO_SHIFT))) ? 1 : 0;
    charger->vbus_ovp = (data_status & (0x1 << S_VBUSOVP_SHIFT)) ? 1 : 0;
    charger->vbus_uvp = (data_status & (0x1 << S_VBUSUVLO_SHIFT)) ? 1 : 0;
    if (charger->vbus_ok) {
        charger->charger_changed = 1;
        pr_info("Set charger_changed!, charger->vbus_ok=%d, data_status=0x%x\n", charger->vbus_ok, data_status);
	}

    /* Check charger status */
    charger->chg_full = (data_status & (0x1 << S_TOPOFF_SHIFT)) ? 1 : 0;

	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5414_init
 * Parameters:
 *		None
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5414_init(void)
{
	struct sm5414_platform_data *pdata = NULL;

	int ret = 0;

  	pr_err("SiliconMitus sm5414 Driver sm5414_init Start\n");
	if (!charger_info->client)
		return -EINVAL;
	
	pdata = &sm5414_pdata_default;
	
	// BATREG
	ret = sm5414_set_chg_volt(pdata->max_charger_voltagemV);
    if (ret < 0)
        pr_err("%s : sm5414_set_chg_volt init ERROR\n",__func__);
    // VBUSLIMIT Current
	ret = sm5414_set_vbus_limit_current(1000);
    if (ret < 0)
        pr_err("%s : sm5414_set_vbus_limit_current init ERROR\n",__func__);
    // FASTCHG Current
	ret = sm5414_set_fastchg_current(1200);
    if (ret < 0)
        pr_err("%s : sm5414_set_fastchg_current init ERROR\n",__func__);       
    // Topoff Current
	ret = sm5414_set_topoff_current(pdata->topoff_currentmA);
    if (ret < 0)
        pr_err("%s : sm5414_set_topoff_current init ERROR\n",__func__);       
    // Encomparator
    ret = sm5414_assign_bits(REG_CTRL, ENCOMPARATOR_MASK, ENCOMPARATOR_SHIFT, 0x1);
    if (ret < 0)
        pr_err("%s : ENCOMPARATOR init ERROR\n",__func__);       
    // Autostop 
    ret = sm5414_assign_bits(REG_CHGCTRL1, AUTOSTOP_MASK, AUTOSTOP_SHIFT, 0x0);
    if (ret < 0)
        pr_err("%s : Autostop init ERROR\n",__func__);       
    // INTMSK1
    ret = sm5414_write_byte(REG_INTMSK1, 0xff);
    if (ret < 0)
        pr_err("%s : REG_INTMSK1 init ERROR\n",__func__);   
    // INTMSK2
    ret = sm5414_write_byte(REG_INTMSK2, 0xff);
    if (ret < 0)
        pr_err("%s : REG_INTMSK2 init ERROR\n",__func__);   
    // INTMSK3
    ret = sm5414_write_byte(REG_INTMSK3, 0xff);
    if (ret < 0)
        pr_err("%s : REG_INTMSK3 init ERROR\n",__func__);   


	/* Max CHARGE VOLTAGE:4.2V */
	/* T34 CHARGE VOLTAGE:4.15V */
	/* T45 CHARGE VOLTAGE:4.1V */
	/* WAKEUP VOLTAGE:2.5V */
	/* WAKEUP CURRENT:0.1A */
	/* Max CHARGE VOLTAGE:4.2V */
	/* EOC CHARGE:0.05A */
	//sm5414_get_charging_status(charger_info);
	
	
	//sm5414_set_vbus_limit_current(1500);
	//sm5414_set_fastchg_current(3000);

    // SiliconMitus : If system can idenfy cable type, plz make seperate setting code below line.
	sm5414_start_ac_charge();

	pr_err("SiliconMitus sm5414 Driver sm5414_init Done\n");

	
	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5414_charger_work
 * Parameters:
 *		work
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5414_charger_work(struct work_struct *work)
{
    struct sm5414_charger_info *charger = 
			container_of(work, struct sm5414_charger_info, delay_work.work);
	
	struct sm5414_platform_data *pdata = NULL;
	
	pdata = charger->pdata;

	if (!charger->client)
		return -EINVAL;

	sm5414_get_charging_status(charger_info);
	
	#if DEBUG_LOG
		pr_info("Vbus status:%d\n", charger_info->vbus_ok);
	#endif
		
	if (charger->charger_changed) {
		power_supply_changed(&charger->usb);	//trigger PM
		charger->charger_changed = 0;
	}
	/* TODO: Feed the watch dog timer */	
	
	schedule_delayed_work(&charger->delay_work, charger->polling_invl);

	/*
	if(charger->vbus_ok)
		schedule_delayed_work(&charger->delay_work, charger->polling_invl);
	else
		schedule_delayed_work(&charger->delay_work, msecs_to_jiffies(1* 1000));
	*/
	return 0;
}

enum vbus_states {
    VBUS_ENABLE,
    VBUS_DISABLE,
    MAX_VBUSCTRL_STATES,
};


/*****************************************************************************
 * Description:
 *		sm5414_charger_set_property
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5414_charger_set_property(struct power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
	struct sm5414_charger_info *charger = 
			container_of(psy, struct sm5414_charger_info, usb);
	struct sm5414_platform_data *pdata = NULL;
	int ret = 0;
	mutex_lock(&charger->lock);
	pdata = charger->pdata;
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
    			sm5414_start_usb_charge();
    		else if ((!val->intval) && (charger->is_charging_enabled != val->intval))
    			sm5414_stop_charge();	
    		charger->is_charging_enabled = val->intval;
            break;
        case POWER_SUPPLY_PROP_ENABLE_CHARGER:
            charger->is_charger_enabled = val->intval;
            break;
	    case POWER_SUPPLY_PROP_CHARGE_CURRENT:
    		charger->charger_current = val->intval;
    		sm5414_set_fastchg_current(val->intval);
            break;
        case POWER_SUPPLY_PROP_CHARGE_VOLTAGE:
    		charger->chg_volt = val->intval;
		sm5414_set_chg_volt(val->intval);
    		break;

        case POWER_SUPPLY_PROP_MAX_CHARGE_VOLTAGE:
            pdata->max_charger_voltagemV = val->intval;
    		break;
        case POWER_SUPPLY_PROP_CHARGE_TERM_CUR:
        	charger->eoc_current = val->intval;
            sm5414_set_topoff_current(val->intval);
    		break;
        case POWER_SUPPLY_PROP_CABLE_TYPE:
        	charger->usb.type = get_power_supply_type(val->intval);
    		charger->cable_type = charger->usb.type;
            break;
        case POWER_SUPPLY_PROP_INLMT:
    		charger->vbus_current = val->intval;
    		sm5414_set_vbus_limit_current(val->intval);	
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
 *		sm5414_charger_get_property
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5414_charger_get_property(struct power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
	struct sm5414_charger_info *charger = 
			container_of(psy, struct sm5414_charger_info, usb);
	struct sm5414_platform_data *pdata = NULL;
	int ret = 0;
	mutex_lock(&charger->lock);
	pdata = charger->pdata;

	switch (psp) {
		case POWER_SUPPLY_PROP_PRESENT:
        	val->intval = charger->present;
        	break;
		case POWER_SUPPLY_PROP_ONLINE:
			sm5414_get_charger_status(charger);
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
		    sm5414_get_charger_status(charger);
			if (charger->vbus_ovp)
				val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
			else if (charger->vbus_ok)
				val->intval = POWER_SUPPLY_HEALTH_GOOD;			
			else
				val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;				
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
 *		sm5414_start_ac_charge
 * Parameters:
 *		None
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
void sm5414_start_ac_charge(void)
{
	struct sm5414_platform_data *pdata = NULL;
	int ret = 0;
	int32_t vbus_current;
	
	pdata = &sm5414_pdata_default;
	pr_info("start ac charge\n");

	//BATREG
	ret = sm5414_set_chg_volt(pdata->max_charger_voltagemV);
    	if (ret < 0)
        	pr_err("%s : sm5414_set_chg_volt ERROR\n",__func__);
    	//TOPOFF
   	 ret = sm5414_set_topoff_current(pdata->topoff_currentmA);	
    	if (ret < 0)
        	pr_err("%s : sm5414_set_topoff_current ERROR\n",__func__);

	pr_info("normal charge adpter\n");
    	//Vbuslimit
    	vbus_current = 1500;
	ret = sm5414_set_vbus_limit_current(vbus_current);
    	if (ret < 0)
        	pr_err("%s : sm5414_set_vbus_limit_current ERROR\n",__func__);
	pr_info("%s : vbus value: %d\n", __func__,vbus_current);
    	//FASTCHG
    	ret = sm5414_set_fastchg_current(2100);
    	if (ret < 0)
        	pr_err("%s : sm5414_set_fastchg_currnet ERROR\n",__func__);

    sm5414_enable_chgen(1);

}


/*****************************************************************************
 * Description:
 *		sm5414_start_ac_charge
 * Parameters:
 *		None
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
void sm5414_start_usb_charge(void)
{
	struct sm5414_platform_data *pdata = NULL;

	int ret = 0;
	
	pr_info("start usb charge\n");
	pdata = &sm5414_pdata_default;
	
	//BATREG	
	ret = sm5414_set_chg_volt(pdata->max_charger_voltagemV);
    	if (ret < 0)
        	pr_err("%s : sm5414_set_chg_volt ERROR\n",__func__);
    	//TOPOFF
    	ret = sm5414_set_topoff_current(pdata->topoff_currentmA);
    	if (ret < 0)
        	pr_err("%s : sm5414_set_topoff_current ERROR\n",__func__);
   	 //Vbuslimit    
	ret  = sm5414_set_vbus_limit_current(500);
    	if (ret < 0)
        	pr_err("%s : sm5414_set_vbus_limit_current ERROR\n",__func__);
	//FASTCHG
	ret = sm5414_set_fastchg_current(1000);
    	if (ret < 0)
        	pr_err("%s : sm5414_set_fastchg_currnet ERROR\n",__func__);
    
    sm5414_enable_chgen(1);
}

/*****************************************************************************
 * Description:
 *		sm5414_stop_charge
 * Parameters:
 *		None
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
void sm5414_stop_charge(void)
{
	int ret = 0;
    
	pr_info("%s !!\n", __func__);

	ret = sm5414_set_fastchg_current(1000);
	ret = sm5414_set_vbus_limit_current(500);

    sm5414_enable_chgen(0);
}

static void sm5414_irq_worker(struct work_struct *work)
{
    //int reg_status, reg_fault, ret;
	//struct sm5414_charger_info *charger = container_of(work,
    //                                                struct sm5414_charger_info,
    //                                                irq_work.work);
	pr_info("%s: Enter!\n", __func__);
	
}

/* IRQ handler for charger Interrupts configured to GPIO pin */
static irqreturn_t sm5414_irq_isr(int irq, void *devid)
{
	//struct sm5414_charger_info *charger = (struct sm5414_charger_info *)devid;
        /**TODO: This hanlder will be used for charger Interrupts */
    return IRQ_WAKE_THREAD;
}

/* IRQ handler for charger Interrupts configured to GPIO pin */
static irqreturn_t sm5414_irq_thread(int irq, void *devid)
{
	struct sm5414_charger_info *charger = (struct sm5414_charger_info *)devid;
    
    schedule_delayed_work(&charger->irq_work, 0);
    
    return IRQ_HANDLED;
}

/*****************************************************************************
 * Description:
 *		sm5414_charger_probe
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5414_charger_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct sm5414_charger_info *charger;
	//struct sm5414_platform_data *pdata = NULL;
	int ret = 0;
	int irq;
#ifdef CONFIG_ACPI    
	struct gpio_desc *gpio;
#endif
	struct device *dev;
	
	pr_err("siliconMitus sm5414 Driver Loading\n");
	charger = devm_kzalloc(&client->dev, sizeof(*charger), GFP_KERNEL);
	if (!charger) {
		pr_err("Can't alloc charger struct\n");
		return -ENOMEM;
	}
	//for public use
	charger_info = charger;
	mutex_init(&charger->lock);	
	charger->pdata = &sm5414_pdata_default;
	
	charger->usb.name = "sm5414-charger";
	charger->usb.type = POWER_SUPPLY_TYPE_USB;
	charger->usb.properties = sm5414_charger_props;
	charger->usb.num_properties = ARRAY_SIZE(sm5414_charger_props);
	charger->usb.get_property = sm5414_charger_get_property;
	charger->usb.set_property = sm5414_charger_set_property;
	charger->usb.supplied_to = charger->pdata->supplied_to;
	charger->usb.num_supplicants = ARRAY_SIZE(sm5414_chrg_supplied_to);
	charger->usb.supported_cables = POWER_SUPPLY_CHARGER_TYPE_USB;
	charger->polling_invl = msecs_to_jiffies(4* 1000);
	
	i2c_set_clientdata(client, charger);
	charger->client = client;	
	charger->client->addr = 0x49;
	pr_err("%s: i2c_client name=%s, addr=0x%x\n", __func__, charger->client->name, charger->client->addr);

	pr_err("SiliconMitus sm5414 Driver Loading 1\n");	
	//sm5414_get_charging_status(charger);
	sm5414_init();
	dev = &client->dev;
	ret = power_supply_register(&client->dev, &charger->usb);

	if (ret < 0) {
		goto err_free;
	}
	INIT_DELAYED_WORK(&charger->delay_work, sm5414_charger_work);
	schedule_delayed_work(&charger->delay_work, charger->polling_invl);
	INIT_DELAYED_WORK(&charger->irq_work, sm5414_irq_worker);
	/* Initialize the wakelock */
    wake_lock_init(&charger->wakelock, WAKE_LOCK_SUSPEND,
                                                "sm_charger_wakelock");

#ifdef CONFIG_ACPI
        gpio = devm_gpiod_get_index(dev, "chrg_int", 0);
        pr_err("%s: gpio=%d\n", __func__, gpio);
        if (IS_ERR(gpio)) {
                pr_err("acpi gpio get index failed\n");
                i2c_set_clientdata(client, NULL);
                return PTR_ERR(gpio);
        } else {
                ret = gpiod_direction_input(gpio);
                if (ret < 0)
                        pr_err("SM5414 CHRG set direction failed\n");

                irq = gpiod_to_irq(gpio);
                pr_err("%s: irq=%d\n", __func__, irq);
        }
#endif

        ret = request_threaded_irq(irq,
                                sm5414_irq_isr, sm5414_irq_thread,
                                IRQF_TRIGGER_FALLING, "SM5414 IRQ", charger);

	pr_info("SiliconMitus sm5414 Driver Loading Done\n");
	return 0;

err_free:

	cancel_delayed_work(&charger->delay_work);
	
	//free_irq(pdata->gpio_stat, NULL);
	kfree(charger);
	return ret;
}


/*****************************************************************************
 * Description:
 *		sm5414_charger_remove
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5414_charger_shutdown(struct i2c_client *client)
{
	struct sm5414_charger_info *charger = i2c_get_clientdata(client);
	
	cancel_delayed_work_sync(&charger->delay_work);
	power_supply_unregister(&charger->usb);
	pr_info("%s: sm5414 Charger Driver removed\n", __func__);
	kfree(charger);
    
	return 0;
}


/*****************************************************************************
 * Description:
 *		sm5414_charger_suspend
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5414_charger_suspend(struct device *dev)
{
	//struct platform_device * const pdev = to_platform_device(dev);
	//struct sm5414_charger_info * const charger = platform_get_drvdata(pdev);

	return 0;
}


/*****************************************************************************
 * Description:
 *		sm5414_charger_resume
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5414_charger_resume(struct device *dev)
{
	//struct platform_device * const pdev = to_platform_device(dev);
	//struct sm5414_charger_info * const charger = platform_get_drvdata(pdev);
	return 0;
}

static const struct i2c_device_id sm5414_i2c_ids[] = {
		{"EXCG0000", 0},
		{}
}; 

MODULE_DEVICE_TABLE(i2c, sm5414_i2c_ids);


#ifdef CONFIG_ACPI
static struct acpi_device_id extchrg_acpi_match[] = {
        {"EXCG0000", 0 },
        {}
};
MODULE_DEVICE_TABLE(acpi, extchrg_acpi_match);
#endif


static const struct dev_pm_ops pm_ops = {
        .suspend        = sm5414_charger_suspend,
        .resume			= sm5414_charger_resume,
};

static struct i2c_driver sm5414_charger_driver = {
		.driver		= {
			.name 		= "sm5414-charger",
			.pm			= &pm_ops,
#ifdef CONFIG_ACPI
			.acpi_match_table = ACPI_PTR(extchrg_acpi_match),
#endif
		},
		.probe		= sm5414_charger_probe,
		.shutdown	= sm5414_charger_shutdown,
		.id_table	= sm5414_i2c_ids,
};

static int __init sm5414_charger_init(void)
{
	pr_info("sm5414 driver init\n");
	return i2c_add_driver(&sm5414_charger_driver);
}

static void __exit sm5414_charger_exit(void)
{
	pr_info("sm5414 driver exit\n");
	
	i2c_del_driver(&sm5414_charger_driver);
}

device_initcall(sm5414_charger_init);
module_exit(sm5414_charger_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SiliconMitus SM5414 Charger Driver");

