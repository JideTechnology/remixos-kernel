/*
 *       oonst struct acpi_device_id *acpi_id;
 * sm5702_charger.c - Charger driver for Siliconmitus
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
#include <linux/power/charger/sm5702_charger.h>
#include <linux/power/charger/sm5702_charger_oper.h>
#include <linux/mfd/sm5702.h>
#include <linux/mfd/sm5702_irq.h>
#include "power_supply.h"


#define DEBUG_LOG   1

#define	RETRY_CNT	5

static char *sm5702_chrg_supplied_to[] = {
        "sm5702-fuelgauge"
};

struct sm5702_chg_irq_handler {
	char *name;
	int irq_index;
	irqreturn_t (*handler)(int irq, void *data);
};

static struct sm5702_platform_data  sm5702_pdata_default = {
        .max_charger_currentmA = 2500, //FASTCHG
        .max_charger_voltagemV = 4200, //BATREG
        .topoff_currentmA = 420,       //Toppoff 
		.vbus_limit_currentmA = 1800,  //VBUSLIMIT 
		.supplied_to = sm5702_chrg_supplied_to,
};

static enum power_supply_property sm5702_charger_props[] = {
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

static struct sm5702_charger_info *charger_info;

void sm5702_get_setting_data(void);
void sm5702_start_ac_charge(void);
void sm5702_start_usb_charge(void);
void sm5702_stop_charge(void);

int32_t sm5702_read_vbus_voltage(void);
extern  int sm5702_init(void);;
extern int sm5702_get_charging_status(struct sm5702_charger_info *charger);

/*****************************************************************************
 * Description:
 *		sm5702_set_chg_volt 
 * Parameters:
 *		chgvolt_mv:	charge voltage to be written
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
//BATREG
static int sm5702_set_chg_volt(int chgvolt_mv)
{
	int ret = 0;
	u8 chg_volt = 0; 
    
	if (chgvolt_mv < BATREG_MIN_MV)
		chgvolt_mv = BATREG_MIN_MV;		    //limit to 3.7V
	else if (chgvolt_mv > BATREG_MAX_MV)
		chgvolt_mv = BATREG_MAX_MV;		    //limit to 4.475V
		
	chg_volt = (chgvolt_mv - BATREG_MIN_MV) / BATREG_STEP_MV;	//step is 25mV
	pr_err("%s: chg_mv=%d, chg_volt set = 0x%x\n", __func__, chgvolt_mv, chg_volt);
    
	ret = sm5702_assign_bits(charger_info->client, SM5702_CHGCNTL3, SM5702_BATREG_MASK, chg_volt << SM5702_BATREG_SHIFT);	
        
	return ret;	
}

/*****************************************************************************
 * Description:
 *		sm5702_get_chg_volt 
 * Return:
 *		BATREG voltage in mV
 *****************************************************************************/
static int sm5702_get_chg_volt(void)
{
	u8 data = 0;
	u8 chg_volt = 0;     		
    int chgvolt_mv = 0;
    
	data = sm5702_reg_read(charger_info->client, SM5702_CHGCNTL3);
    if (data < 0)
        return data;
    
    chg_volt = ((data & SM5702_BATREG_MASK) >> SM5702_BATREG_SHIFT);

    chgvolt_mv = BATREG_MIN_MV + (chg_volt * BATREG_STEP_MV);

	if (chgvolt_mv < BATREG_MIN_MV)
		chgvolt_mv = BATREG_MIN_MV;		    //limit to 4.1V
	else if (chgvolt_mv > BATREG_MAX_MV)
		chgvolt_mv = BATREG_MAX_MV;		    //limit to 4.475V

	pr_err("%s: chg_mv=%d, chg_volt set = 0x%x\n", __func__, chgvolt_mv, chg_volt);
            
	return chgvolt_mv;	
}

/*****************************************************************************
 * Description:
 *		sm5702_set_topoff_current 
 * Parameters:
 *		topoff_ma:	set end of charge current
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5702_set_topoff_current(int topoff_ma)
{
	int ret = 0;
	u8 topoff_curr = 0;
    
	if (topoff_ma <= TOPPOFF_MIN_MA)
		topoff_ma = TOPPOFF_MIN_MA;		//min value is 100mA
	else if (topoff_ma > TOPPOFF_MAX_MA)
		topoff_ma = TOPPOFF_MAX_MA;		    //limit to 475mA		

	topoff_curr = (topoff_ma - TOPPOFF_MIN_MA) / TOPPOFF_STEP_MA;	//step is 25mA
	pr_err("%s: topoff_ma=%d, topoff_curr set = 0x%x\n", __func__, topoff_ma, topoff_curr);

    ret = sm5702_assign_bits(charger_info->client, SM5702_CHGCNTL4, SM5702_TOPOFF_MASK, topoff_curr << SM5702_TOPOFF_SHIFT);
    
	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5702_get_topoff_current 
 * Return:
 *		Topoff current in mA
 *****************************************************************************/
static int sm5702_get_topoff_current(void)
{
	u8 data = 0;
	u8 topoff_curr = 0;     		
    int topoff_ma = 0;
    
	data = sm5702_reg_read(charger_info->client, SM5702_CHGCNTL4);
    if (data < 0)
        return data;
    
    topoff_curr = ((data & SM5702_TOPOFF_MASK) >> SM5702_TOPOFF_SHIFT);

    topoff_ma = TOPPOFF_MIN_MA + (topoff_curr * TOPPOFF_STEP_MA);

	if (topoff_ma <= TOPPOFF_MIN_MA)
		topoff_ma = TOPPOFF_MIN_MA;		//min value is 100mA
	else if (topoff_ma > TOPPOFF_MAX_MA)
		topoff_ma = TOPPOFF_MAX_MA;		    //limit to 650mA		

	pr_err("%s: topoff_ma=%d, topoff_curr set = 0x%x\n", __func__, topoff_ma, topoff_curr);
            
	return topoff_ma;	
}

/*****************************************************************************
 * Description:
 *		sm5702_set_vbus_limit_current 
 * Parameters:
 *		ilmt_ma:	set input current limit
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5702_set_vbus_limit_current(int ilmt_ma)
{
	int ret = 0;
	u8 ilmt_curr = 0;
    
	if (ilmt_ma <= VBUSLIMIT_MIN_MA)
		ilmt_ma = VBUSLIMIT_MIN_MA;
	else if (ilmt_ma > VBUSLIMIT_MAX_MA)
		ilmt_ma = VBUSLIMIT_MAX_MA;

	ilmt_curr = (ilmt_ma - VBUSLIMIT_MIN_MA) / VBUSLIMIT_STEP_MA;
	pr_err("%s: ilmt_ma=%d, ilmt_curr set = 0x%x\n", __func__, ilmt_ma, ilmt_curr);

    ret = sm5702_assign_bits(charger_info->client, SM5702_VBUSCNTL, SM5702_VBUSLIMIT_MASK, ilmt_curr << SM5702_VBUSLIMIT_SHIFT);
    
	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5702_get_vbus_limit_current 
 * Return:
 *		Vbus input current in mA
 *****************************************************************************/
int sm5702_get_vbus_limit_current(void)
{
	u8 data = 0;
	u8 ilmt_curr = 0;     		
    int ilmt_ma = 0;
    
	data = sm5702_reg_read(charger_info->client, SM5702_VBUSCNTL);
    if (data < 0)
        return data;
    
    ilmt_curr = ((data & SM5702_VBUSLIMIT_MASK) >> SM5702_VBUSLIMIT_SHIFT);

    ilmt_ma = VBUSLIMIT_MIN_MA + (ilmt_curr * VBUSLIMIT_STEP_MA);

	if (ilmt_ma <= VBUSLIMIT_MIN_MA)
		ilmt_ma = VBUSLIMIT_MIN_MA;
	else if (ilmt_ma > VBUSLIMIT_MAX_MA)
		ilmt_ma = VBUSLIMIT_MAX_MA;		

	pr_err("%s: ilmt_ma=%d, ilmt_curr set = 0x%x\n", __func__, ilmt_ma, ilmt_curr);
            
	return ilmt_ma;	

}

/*****************************************************************************
 * Description:
 *		sm5702_set_fastchg_current
 * Parameters:
 *		chg_ma:	set charger current
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5702_set_fastchg_current(int chg_ma)
{
	int ret = 0;
	u8 chg_curr = 0;
    
	if (chg_ma <= FASTCHG_MIN_MA)
		chg_ma = FASTCHG_MIN_MA;
	else if (chg_ma > FASTCHG_MAX_MA)
		chg_ma = FASTCHG_MAX_MA;

	chg_curr = (chg_ma - FASTCHG_MIN_MA) / FASTCHG_STEP_MA;
	pr_err("%s: chg_ma=%d, chg_curr set = 0x%x\n", __func__, chg_ma, chg_curr);

    ret = sm5702_assign_bits(charger_info->client, SM5702_CHGCNTL2, SM5702_FASTCHG_MASK, chg_curr << SM5702_FASTCHG_SHIFT);
    
	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5702_get_fastchg_current 
 * Return:
 *		Fast charging current in mA
 *****************************************************************************/
int sm5702_get_fastchg_current(void)
{
	u8 data = 0;
	u8 chg_curr = 0;     		
    int chg_ma = 0;
    
	data = sm5702_reg_read(charger_info->client, SM5702_CHGCNTL2);
    if (data < 0)
        return data;
    
    chg_curr = ((data & SM5702_FASTCHG_MASK) >> SM5702_FASTCHG_SHIFT);

    chg_ma = FASTCHG_MIN_MA + (chg_curr * FASTCHG_STEP_MA);

	if (chg_ma <= FASTCHG_MIN_MA)
		chg_ma = FASTCHG_MIN_MA;
	else if (chg_ma > FASTCHG_MAX_MA)
		chg_ma = FASTCHG_MAX_MA;		

	pr_err("%s: chg_ma=%d, chg_curr set = 0x%x\n", __func__, chg_ma, chg_curr);
            
	return chg_ma;	
}

/*****************************************************************************
 * Description:
 *		sm5702_enable_otg 
 * Parameters:
 *		enable:	enable otg function
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5702_enable_otg(int enable)
{
	int ret = 0;

	pr_err("%s: OTG = %d\n", __func__, enable);   

	sm5702_charger_oper_push_event(SM5702_CHARGER_OP_MODE_USB_OTG, enable);
    
	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5702_enable_chgen 
 * Parameters:
 *		enable:	enable otg function
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5702_enable_chgen(int enable)
{
	int ret = 0;

	pr_err("%s: CHGEN = %d\n", __func__, enable);

    //SM : After setting nCHGEN GPIO, plz change way to control charging function to GPIO(nCHGNE)
	sm5702_charger_oper_push_event(SM5702_CHARGER_OP_MODE_CHG_ON, enable);
    
	return ret;
}


void sm5702_get_setting_data(void)
{	
	if (!charger_info->client)
		return;

	charger_info->chg_volt = sm5702_get_chg_volt();

	charger_info->charger_current = sm5702_get_fastchg_current();
	
	charger_info->vbus_current = sm5702_get_vbus_limit_current();

	charger_info->eoc_current = sm5702_get_topoff_current();

	pr_err("sm5702_get_setting_data \n");
	
	pr_err( "## sm5702 charger information ##\n"
		"chg_volt: %d, vbus_current: %d, charge_current: %d\n"
		"eoc_current: %d, vbus_ovp: %d, vbus_uvp: %d\n"
		"vbus_ok: %d, chg_full: %d\n",
		charger_info->chg_volt, charger_info->vbus_current,
		charger_info->charger_current,charger_info->eoc_current,
		charger_info->vbus_ovp, charger_info->vbus_uvp,
		charger_info->vbus_ok, charger_info->chg_full);
		
		pr_err("## All data register ##\n");
		sm5702_cf_dump_read(charger_info->client);
}

static int sm5702_get_charger_status(struct sm5702_charger_info *charger)
{
	int ret = 0;
	u8 data_status[4] = {0,};
	if (!charger)
		return -EINVAL;

	/* Check VBUS */
	data_status[0] = sm5702_reg_read(charger_info->client, SM5702_STATUS1);
	data_status[1] = sm5702_reg_read(charger_info->client, SM5702_STATUS2);
	data_status[2] = sm5702_reg_read(charger_info->client, SM5702_STATUS3);
	data_status[3] = sm5702_reg_read(charger_info->client, SM5702_STATUS4);
    
	pr_err("%s: status1=0x%x, status2=0x%x, status3=0x%x, status4=0x%x\n", __func__, data_status[0],data_status[1],data_status[2],data_status[3]);

    if(!(data_status[0] & (0x1 << SM5702_STATUS1_VBUSPOK_SHIFT))) {
        charger->vbus_ok = 0;
        pr_err("VBUS not OK\n");                
        return ret;
    }

    charger->vbus_ok = (data_status[0] & (0x1 << SM5702_STATUS1_VBUSPOK_SHIFT)) ? 1 : 0;
    charger->vbus_ovp = (data_status[0] & (0x1 << SM5702_STATUS1_VBUSOVP_SHIFT)) ? 1 : 0;
    charger->vbus_uvp = (data_status[0] & (0x1 << SM5702_STATUS1_VBUSUVLO_SHIFT)) ? 1 : 0;
    if (charger->vbus_ok) {
        charger->charger_changed = 1;
        pr_err("Set charger_changed!, charger->vbus_ok=%d, data_status[0]=0x%x\n", charger->vbus_ok, data_status[0]);
	}

    /* Check charger status */
    charger->chg_full = (data_status[1] & (0x1 << SM5702_STATUS2_TOPOFF_SHIFT)) ? 1 : 0;
    
	return ret;	
}

/*****************************************************************************
 * Description:
 *		sm5702_get_charging_status 
 * Parameters:
 *		charger:	charger data
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5702_get_charging_status(struct sm5702_charger_info *charger)
{
    int ret = 0;
    u8 data_status[4] = {0,};
    if (!charger)
        return -EINVAL;

    /* Check VBUS */
    data_status[0] = sm5702_reg_read(charger_info->client, SM5702_STATUS1);
    data_status[1] = sm5702_reg_read(charger_info->client, SM5702_STATUS2);
    data_status[2] = sm5702_reg_read(charger_info->client, SM5702_STATUS3);
    data_status[3] = sm5702_reg_read(charger_info->client, SM5702_STATUS4);
    
    pr_err("%s: status1=0x%x, status2=0x%x, status3=0x%x, status4=0x%x\n", __func__, data_status[0],data_status[1],data_status[2],data_status[3]);

    if(!(data_status[0] & (0x1 << SM5702_STATUS1_VBUSPOK_SHIFT))) {
        charger->vbus_ok = 0;
        pr_err("VBUS not OK\n");                
        return ret;
    }

    charger->vbus_ok = (data_status[0] & (0x1 << SM5702_STATUS1_VBUSPOK_SHIFT)) ? 1 : 0;
    charger->vbus_ovp = (data_status[0] & (0x1 << SM5702_STATUS1_VBUSOVP_SHIFT)) ? 1 : 0;
    charger->vbus_uvp = (data_status[0] & (0x1 << SM5702_STATUS1_VBUSUVLO_SHIFT)) ? 1 : 0;
    if (charger->vbus_ok) {
        charger->charger_changed = 1;
        pr_err("Set charger_changed!, charger->vbus_ok=%d, data_status[0]=0x%x\n", charger->vbus_ok, data_status[0]);
    }

    /* Check charger status */
    charger->chg_full = (data_status[1] & (0x1 << SM5702_STATUS2_TOPOFF_SHIFT)) ? 1 : 0;
    
    return ret; 
}


/*****************************************************************************
 * Description:
 *		sm5702_init
 * Parameters:
 *		None
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
int sm5702_chg_init(void)
{
	struct sm5702_platform_data *pdata = NULL;

	int ret = 0;

  	pr_err("SiliconMitus sm5702 Driver sm5702_init Start\n");
	if (!charger_info->client)
		return -EINVAL;
	
	pdata = &sm5702_pdata_default;
	
	// BATREG
	ret = sm5702_set_chg_volt(pdata->max_charger_voltagemV);
    if (ret < 0)
        pr_err("%s : sm5702_set_chg_volt init ERROR\n",__func__);
    // VBUSLIMIT Current
	//ret = sm5702_set_vbus_limit_current(1000);
    //if (ret < 0)
    //    pr_err("%s : sm5702_set_vbus_limit_current init ERROR\n",__func__);
    // FASTCHG Current
	//ret = sm5702_set_fastchg_current(1200);
    //if (ret < 0)
    //    pr_err("%s : sm5702_set_fastchg_current init ERROR\n",__func__);       
    // Topoff Current
	ret = sm5702_set_topoff_current(pdata->topoff_currentmA);
    if (ret < 0)
        pr_err("%s : sm5702_set_topoff_current init ERROR\n",__func__);       
    // Encomparator
    ret = sm5702_assign_bits(charger_info->client, SM5702_CHGCNTL1, SM5702_ENCOMPARATOR_MASK, 0x1 << SM5702_ENCOMPARATOR_SHIFT);
    if (ret < 0)
        pr_err("%s : ENCOMPARATOR init ERROR\n",__func__);       
    // Autostop
    ret = sm5702_assign_bits(charger_info->client, SM5702_CHGCNTL1, SM5702_AUTOSTOP_MASK, 0x0 << SM5702_AUTOSTOP_SHIFT);
    if (ret < 0)
        pr_err("%s : Autostop init ERROR\n",__func__);           
    // INTMSK1
    ret = sm5702_reg_write(charger_info->client, SM5702_INTMSK1, 0xff);
    if (ret < 0)
        pr_err("%s : REG_INTMSK1 init ERROR\n",__func__);   
    // INTMSK2
    ret = sm5702_reg_write(charger_info->client, SM5702_INTMSK2, 0xff);
    if (ret < 0)
        pr_err("%s : REG_INTMSK2 init ERROR\n",__func__);   
    // INTMSK3
    ret = sm5702_reg_write(charger_info->client, SM5702_INTMSK3, 0xff);
    if (ret < 0)
        pr_err("%s : REG_INTMSK3 init ERROR\n",__func__);   
    // INTMSK4
    ret = sm5702_reg_write(charger_info->client, SM5702_INTMSK4, 0xff);
    if (ret < 0)
        pr_err("%s : REG_INTMSK4 init ERROR\n",__func__);  

	/* Max CHARGE VOLTAGE:4.2V */
	/* T34 CHARGE VOLTAGE:4.15V */
	/* T45 CHARGE VOLTAGE:4.1V */
	/* WAKEUP VOLTAGE:2.5V */
	/* WAKEUP CURRENT:0.1A */
	/* Max CHARGE VOLTAGE:4.2V */
	/* EOC CHARGE:0.05A */
	//sm5702_get_charging_status(charger_info);
	
	
	//sm5702_set_vbus_limit_current(1500);
	//sm5702_set_fastchg_current(3000);

    // SiliconMitus : If system can idenfy cable type, plz make seperate setting code below line.
	sm5702_start_ac_charge();

	pr_err("SiliconMitus sm5702 Driver sm5702_init Done\n");

	
	return ret;
}

/*****************************************************************************
 * Description:
 *		sm5702_charger_work
 * Parameters:
 *		work
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5702_charger_work(struct work_struct *work)
{
    struct sm5702_charger_info *charger = 
			container_of(work, struct sm5702_charger_info, delay_work.work);
	
	struct sm5702_platform_data *pdata = NULL;
	
	pdata = charger->pdata;

	if (!charger->client)
		return -EINVAL;

	sm5702_get_charging_status(charger_info);
	
	#if DEBUG_LOG
		pr_err("Vbus status:%d\n", charger_info->vbus_ok);
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
 *		sm5702_charger_set_property
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5702_charger_set_property(struct power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
	struct sm5702_charger_info *charger = 
			container_of(psy, struct sm5702_charger_info, usb);
	struct sm5702_platform_data *pdata = NULL;
	int ret = 0;
	mutex_lock(&charger->lock);
	pdata = charger->pdata;
	pr_err("%s: psp=%d, value_to_set=%d\n", __func__, psp, val->intval);

	switch (psp) {
        case POWER_SUPPLY_PROP_PRESENT:
            charger->present = val->intval;
            break;
        case POWER_SUPPLY_PROP_ONLINE:
            charger->online = val->intval;
            break;
        case POWER_SUPPLY_PROP_ENABLE_CHARGING:
    		if (val->intval && (charger->is_charging_enabled != val->intval))
    			sm5702_start_usb_charge();
    		else if ((!val->intval) && (charger->is_charging_enabled != val->intval))
    			sm5702_stop_charge();	
    		charger->is_charging_enabled = val->intval;
            break;
        case POWER_SUPPLY_PROP_ENABLE_CHARGER:
            charger->is_charger_enabled = val->intval;
            break;
	    case POWER_SUPPLY_PROP_CHARGE_CURRENT:
    		charger->charger_current = val->intval;
    		sm5702_set_fastchg_current(val->intval);
            break;
        case POWER_SUPPLY_PROP_CHARGE_VOLTAGE:
    		charger->chg_volt = val->intval;
            	sm5702_set_chg_volt(val->intval);
    		break;
	case POWER_SUPPLY_PROP_CHARGE_TERM_CUR:
		charger->eoc_current = val->intval;
		sm5702_set_topoff_current(val->intval);
		break;
        case POWER_SUPPLY_PROP_CABLE_TYPE:
        	charger->usb.type = get_power_supply_type(val->intval);
    		charger->cable_type = charger->usb.type;
            break;
        case POWER_SUPPLY_PROP_INLMT:
    		charger->vbus_current = val->intval;
    		sm5702_set_vbus_limit_current(val->intval);	
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
 *		sm5702_charger_get_property
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5702_charger_get_property(struct power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
	struct sm5702_charger_info *charger = 
			container_of(psy, struct sm5702_charger_info, usb);
	struct sm5702_platform_data *pdata = NULL;
	int ret = 0;
	mutex_lock(&charger->lock);
	pdata = charger->pdata;


	switch (psp) {
		case POWER_SUPPLY_PROP_PRESENT:
        	val->intval = charger->present;
        	break;
		case POWER_SUPPLY_PROP_ONLINE:
			sm5702_get_charger_status(charger);
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
		    sm5702_get_charger_status(charger);
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
	pr_err("%s: psp=%d, value=%d\n", __func__, psp, val->intval);
	mutex_unlock(&charger->lock);
	return ret;
}


/*****************************************************************************
 * Description:
 *		sm5702_start_ac_charge
 * Parameters:
 *		None
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
void sm5702_start_ac_charge(void)
{
	struct sm5702_platform_data *pdata = NULL;
	int ret = 0;
	int32_t vbus_current;
	
	pdata = &sm5702_pdata_default;
	pr_err("start ac charge\n");

	//BATREG
	ret = sm5702_set_chg_volt(pdata->max_charger_voltagemV);
    //TOPOFF
    ret = sm5702_set_topoff_current(pdata->topoff_currentmA);	

	pr_err("normal charge adpter\n");
    //Vbuslimit
    vbus_current = 1500;
	ret = sm5702_set_vbus_limit_current(vbus_current);
	pr_err("%s : vbus value: %d\n", __func__,vbus_current);
    //FASTCHG
    ret = sm5702_set_fastchg_current(2000);

    sm5702_enable_chgen(1);

}


/*****************************************************************************
 * Description:
 *		sm5702_start_ac_charge
 * Parameters:
 *		None
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
void sm5702_start_usb_charge(void)
{
	struct sm5702_platform_data *pdata = NULL;

	int ret = 0;
	
	pr_err("start usb charge\n");
	pdata = &sm5702_pdata_default;
	
	//BATREG	
	ret = sm5702_set_chg_volt(pdata->max_charger_voltagemV);
    //TOPOFF
    ret = sm5702_set_topoff_current(pdata->topoff_currentmA);
    //Vbuslimit    
	ret  = sm5702_set_vbus_limit_current(500);
	//FASTCHG
	ret = sm5702_set_fastchg_current(1000);
    
    sm5702_enable_chgen(1);
}

/*****************************************************************************
 * Description:
 *		sm5702_stop_charge
 * Parameters:
 *		None
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
void sm5702_stop_charge(void)
{
	int ret = 0;
    
	pr_err("%s !!\n", __func__);

	ret = sm5702_set_fastchg_current(1000);
	ret = sm5702_set_vbus_limit_current(500);

    sm5702_enable_chgen(0);
}

static irqreturn_t sm5702_chg_vbusuvlo_irq_handler(int irq, void *data)
{
    //struct sm5702_charger_info *info = data;
    //struct i2c_client *iic = info->client;

    /* set full charged flag
     * until TA/USB unplug event / stop charging by PSY
     */

     pr_err("%s : VBUSUVLO\n", __func__);
     
    return IRQ_HANDLED;
}

static irqreturn_t sm5702_chg_vbuspok_irq_handler(int irq, void *data)
{
    //struct sm5702_charger_info *info = data;
    //struct i2c_client *iic = info->client;

    /* set full charged flag
     * until TA/USB unplug event / stop charging by PSY
     */

     pr_err("%s : VBUSPOK\n", __func__);
     
    return IRQ_HANDLED;
}
    
static irqreturn_t sm5702_chg_topoff_irq_handler(int irq, void *data)
{
    //struct sm5702_charger_info *info = data;
    //struct i2c_client *iic = info->client;

    /* set full charged flag
     * until TA/USB unplug event / stop charging by PSY
     */

     pr_err("%s : Full charged(topoff)\n", __func__);
     
    return IRQ_HANDLED;
}

const struct sm5702_chg_irq_handler sm5702_chg_irq_handlers[] = {
/*
    {
        .name = "VBUSPOK",
        .handler = sm5702_chg_topoff_irq_handler,
        .irq_index = SM5702_VBUSPOK_IRQ,
    },
    {
        .name = "VBUSUVLO",
        .handler = sm5702_chg_vbusuvlo_irq_handler,
        .irq_index = SM5702_VBUSUVLO_IRQ,
    },    
    {
        .name = "TOPOFF",
        .handler = sm5702_chg_topoff_irq_handler,
        .irq_index = SM5702_TOPOFF_IRQ,
    },
*/
};

static int register_irq(struct platform_device *pdev,
        struct sm5702_charger_info *info)
{
    int irq;
    int i, j;
    int ret;
    const struct sm5702_chg_irq_handler *irq_handler = sm5702_chg_irq_handlers;
    const char *irq_name;
    for (i = 0; i < ARRAY_SIZE(sm5702_chg_irq_handlers); i++) {
        irq_name = sm5702_get_irq_name_by_index(irq_handler[i].irq_index);
        irq = platform_get_irq_byname(pdev, irq_name);
        ret = request_threaded_irq(irq, NULL, irq_handler[i].handler,
                       IRQF_ONESHOT | IRQF_TRIGGER_FALLING |
                       IRQF_NO_SUSPEND, irq_name, info);
        if (ret < 0) {
            pr_err("%s : Failed to request IRQ (%s): #%d: %d\n",
                    __func__, irq_name, irq, ret);
            goto err_irq;
        }

        pr_err("%s : Register IRQ%d(%s) successfully\n",
                __func__, irq, irq_name);
    }

    return 0;
err_irq:
    for (j = 0; j < i; j++) {
        irq_name = sm5702_get_irq_name_by_index(irq_handler[j].irq_index);
        irq = platform_get_irq_byname(pdev, irq_name);
        free_irq(irq, info);
    }

    return ret;
}

static void unregister_irq(struct platform_device *pdev,
        struct sm5702_charger_info *info)
{
    int irq;
    int i;
    const char *irq_name;
    const struct sm5702_chg_irq_handler *irq_handler = sm5702_chg_irq_handlers;

    for (i = 0; i < ARRAY_SIZE(sm5702_chg_irq_handlers); i++) {
        irq_name = sm5702_get_irq_name_by_index(irq_handler[i].irq_index);
        irq = platform_get_irq_byname(pdev, irq_name);
        free_irq(irq, info);
    }
}

/*****************************************************************************
 * Description:
 *		sm5702_charger_probe
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5702_charger_probe(struct platform_device *pdev)
{
	sm5702_mfd_chip_t *chip = dev_get_drvdata(pdev->dev.parent);
	//struct sm5702_mfd_platform_data *mfd_pdata = dev_get_platdata(chip->dev);  
	struct sm5702_charger_info *charger;
	//struct sm5702_platform_data *pdata = NULL;
	int ret = 0;
	
	pr_err("siliconMitus sm5702 Driver Loading\n");
	charger = devm_kzalloc(&pdev->dev, sizeof(*charger), GFP_KERNEL);
	if (!charger) {
		pr_err("Can't alloc charger struct\n");
		return -ENOMEM;
	}
	//for public use
	charger_info = charger;
	mutex_init(&charger->lock);	
	charger->pdata = &sm5702_pdata_default;
	charger->dev = &pdev->dev;	    
    charger->irq_base = chip->irq_base;
	charger->usb.name = "sm5702-charger";
	charger->usb.type = POWER_SUPPLY_TYPE_USB;
	charger->usb.properties = sm5702_charger_props;
	charger->usb.num_properties = ARRAY_SIZE(sm5702_charger_props);
	charger->usb.get_property = sm5702_charger_get_property;
	charger->usb.set_property = sm5702_charger_set_property;
	charger->usb.supplied_to = charger->pdata->supplied_to;
	charger->usb.num_supplicants = ARRAY_SIZE(sm5702_chrg_supplied_to);
	charger->usb.supported_cables = POWER_SUPPLY_CHARGER_TYPE_USB;
	charger->polling_invl = msecs_to_jiffies(4* 1000);
	
	charger->client = chip->i2c_client;	
	pr_err("%s: i2c_client name=%s, addr=0x%x\n", __func__, charger->client->name, charger->client->addr);

	pr_err("SiliconMitus sm5702 Driver Loading 1\n");	
	//sm5702_get_charging_status(charger);
	sm5702_chg_init();
	platform_set_drvdata(pdev, charger);
    
	ret = power_supply_register(&pdev->dev, &charger->usb);
	if (ret < 0) {
		pr_err("%s : power_supply_register error\n",__func__);
		goto err_free;
	}

    //For Interrupt
	ret = register_irq(pdev, charger);
	if (ret < 0)
	{
		pr_err("%s : register_irq error\n",__func__);
        	goto err_reg_irq;
    }

	INIT_DELAYED_WORK(&charger->delay_work, sm5702_charger_work);
	schedule_delayed_work(&charger->delay_work, charger->polling_invl);
	/* Initialize the wakelock */
    wake_lock_init(&charger->wakelock, WAKE_LOCK_SUSPEND,
                                                "sm_charger_wakelock");
    
	pr_err("SiliconMitus sm5702 Driver Loading Done\n");
	return 0;
    
err_reg_irq:
    power_supply_unregister(&charger->usb);
err_free:
	cancel_delayed_work(&charger->delay_work);
	
	//free_irq(pdata->gpio_stat, NULL);
	kfree(charger);
	return ret;
}


/*****************************************************************************
 * Description:
 *		sm5702_charger_remove
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5702_charger_remove(struct platform_device *pdev)
{
	struct sm5702_charger_info *charger = platform_get_drvdata(pdev);	

	unregister_irq(pdev, charger);
	cancel_delayed_work_sync(&charger->delay_work);
	power_supply_unregister(&charger->usb);
	pr_err("%s: sm5702 Charger Driver removed\n", __func__);
	kfree(charger);

    return 0;
}


/*****************************************************************************
 * Description:
 *		sm5702_charger_suspend
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5702_charger_suspend(struct device *dev)
{
	//struct platform_device * const pdev = to_platform_device(dev);
	//struct sm5702_charger_info * const charger = platform_get_drvdata(pdev);

	return 0;
}


/*****************************************************************************
 * Description:
 *		sm5702_charger_resume
 * Parameters:
 *		
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
static int sm5702_charger_resume(struct device *dev)
{
	//struct platform_device * const pdev = to_platform_device(dev);
	//struct sm5702_charger_info * const charger = platform_get_drvdata(pdev);
	return 0;
}

static const struct dev_pm_ops pm_ops = {
        .suspend        = sm5702_charger_suspend,
        .resume			= sm5702_charger_resume,
};

static struct platform_driver sm5702_charger_driver = {
		.driver		= {
			.name 		= "sm5702-charger",                
            .owner      = THIS_MODULE,
			.pm			= &pm_ops,			
		},
		.probe		= sm5702_charger_probe,
		.remove	    = sm5702_charger_remove,
};

static int __init sm5702_charger_init(void)
{
	int ret = 0;

	pr_err("%s \n", __func__);
	ret = platform_driver_register(&sm5702_charger_driver);

	return ret;
}
module_init(sm5702_charger_init);

static void __exit sm5702_charger_exit(void)
{
	platform_driver_unregister(&sm5702_charger_driver);
}
module_exit(sm5702_charger_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SiliconMitus SM5702 Charger Driver");

