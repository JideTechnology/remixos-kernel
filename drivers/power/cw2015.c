/*
 *  cw2015_battery.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved
 *  Chandler Zhang <chazhang@nvidia.com>
 *  Syed Rafiuddin <srafiuddin@nvidia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <asm/unaligned.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/cw2015_battery.h>
#include <linux/mfd/bq2419x.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/gpio.h>
extern int nct1008_temp_to_battery(void);

#define REG_VERSION     0x0
#define REG_VCELL       0x2
#define REG_SOC         0x4
#define REG_RRT_ALERT   0x6
#define REG_CONFIG      0x8
#define REG_MODE        0xA
#define REG_BATINFO     0x10

#define REG_TEMP_H     0x0C
#define REG_TEMP_l     0x0D



#define MODE_SLEEP_MASK (0x3<<6)
#define MODE_SLEEP      (0x3<<6)
#define MODE_NORMAL     (0x0<<6)
#define MODE_QUICK_START (0x3<<4)
#define MODE_RESTART    (0xf<<0)

#define CONFIG_UPDATE_FLG (0x1<<1)
#define ATHD (0x00<<3)   //ATHD =10%->0%

/*----- config part for battery information -----*/

#define FORCE_WAKEUP_CHIP 1

// #define BATT_MAX_VOL_VALUE		  4200				//满电时的电池电压	 
// #define BATT_ZERO_VOL_VALUE 	  3500				//关机时的电池电压


#define BATTERY_UP_MAX_CHANGE   420             // the max time allow battery change quantity
#define BATTERY_DOWN_CHANGE   60                // the max time allow battery change quantity
#define BATTERY_DOWN_MIN_CHANGE_RUN 30          // the min time allow battery change quantity when run
#define BATTERY_DOWN_MIN_CHANGE_SLEEP 1800      // the min time allow battery change quantity when run 30min
#define BATTERY_DOWN_MAX_CHANGE_RUN_AC_ONLINE 3600

#define CW2015_DELAY		1000
#define CW2015_BATTERY_FULL	100
#define CW2015_BATTERY_LOW	15
#define CW2015_VERSION_NO	0x11

//static int g_sitemp =0xFF00;

#define TCL_BATTERY_PR_4160131 0x00000001
#define TCL_BATTERY_PR_4770108 0x00000002

#define DTR_BATTERY_TYPE TCL_BATTERY_PR_4160131

#define BATTERY_PRODUCT 

/* battery info: two 3600 battery 7200mah add by xhc for 3300 */
static char cw2015_bat_config_info[SIZE_BATINFO] = {
#if (DTR_BATTERY_TYPE==TCL_BATTERY_PR_4160131)
	0x15,0x7E,0x5E,0x5E,0x5C,0x57,0x52,0x4F,0x4D, 
	0x4C,0x47,0x45,0x40,0x41,0x38,0x2C,0x25,0x19, 
	0x14,0x0D,0x1D,0x3D,0x51,0x44,0x1C,0x51,0x0B, 
	0x85,0x1F,0x3F,0x47,0x5B,0x65,0x62,0x56,0x5D, 
	0x3E,0x1D,0x72,0x3E,0x11,0x4A,0x52,0x87,0x8F, 
	0x91,0x94,0x52,0x82,0x8C,0x92,0x96,0x7F,0xAE, 
	0xEB,0xCB,0x2F,0x7D,0x72,0xA5,0xB5,0xC1,0x2B,
	0x71

#else
	0x15,	0x60,	0x59,	0x5A,	0x56,
	0x69,	0x56,	0x4C,	0x4D,	0x4A,	
	0x46,	0x43,	0x3C,	0x38,	0x34,	
	0x2C,	0x22,	0x1F,	0x1A,	0x16,	
	0x14,	0x2B,	0x48,	0x48,	0x34,	
	0x32,	0x0B,	0x85,	0x16,	0x2C,	
	0x44,	0x67,	0x66,	0x5D,	0x58,	
	0x5A,	0x3F,	0x1A,	0x39,	0x4D,	
	0x02,	0x2B,	0x3B,	0x6C,	0x8B,	
	0x92,	0x93,	0x18,	0x58,	0x82,	
	0x94,	0xA5,	0x3C,	0xB6,	0xF9,	
	0xCB,	0x2F,	0x7D,	0x72,	0xA5,	
	0xB5,	0xC1,	0x2B,	0x0B
#endif
};



struct cw2015_chip {
	struct i2c_client		*client;
	struct delayed_work		work;
	struct power_supply		battery;
	struct power_supply		ac;
	struct power_supply		usb;
	struct cw2015_platform_data *pdata;
	struct input_dev *input;

	uint8_t *data_table;

	/* State Of Connect */
	int ac_online;
	int usb_online;

	
	/* battery voltage */
	int vcell;
	/* battery capacity */
	int soc;

	int rrt;
	/* State Of Charge */
	int status;
	/* battery health */
	int health;
	/* battery capacity */
	int capacity_level;

	int lasttime_vcell;
	int lasttime_soc;
	int lasttime_status;
	int lasttime_rrt;
	int use_usb;
	int use_ac;
	int charger_mode;
	int shutdown_complete;

	long sleep_time_capacity;      // the sleep time from capacity change to present, it will set 0 when capacity change 
       long run_time_capacity;
   
       long sleep_time_ac_online;      // the sleep time from insert ac to present, it will set 0 when insert ac
       long run_time_ac_online;
	
	struct mutex mutex;
	void (*cw2015_power_off)(void);
	void (*cw2015_irq_displayed)(void);
#if  WARK_UP_FLAG
	struct work_struct athd_work;
#endif
};
struct cw2015_chip *cw2015_data;

static int Battery_status;

bool Irq_power_off = false;

static int cw2015_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct cw2015_chip *chip = container_of(psy,
				struct cw2015_chip, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = chip->status;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chip->vcell;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = chip->soc;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		val->intval = chip->rrt;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = chip->health;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		val->intval = chip->capacity_level;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = nct1008_temp_to_battery();
		break;
	default:
	return -EINVAL;
	}
	return 0;
}

static int cw2015_ac_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct cw2015_chip *chip = container_of(psy,
				struct cw2015_chip, ac);

	if (psp == POWER_SUPPLY_PROP_ONLINE){
		val->intval = chip->ac_online;
	}else
		return -EINVAL;

	return 0;
}

static int cw2015_usb_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct cw2015_chip *chip = container_of(psy,
					struct cw2015_chip, usb);

	if (psp == POWER_SUPPLY_PROP_ONLINE){
		val->intval = chip->usb_online;
	}else
		return -EINVAL;

	return 0;
}

static int cw2015_verify_update_battery_info(struct cw2015_chip *chip)
{
	int ret = 0;
	int i;
	char value = 0;
	char buffer[SIZE_BATINFO*2];
	char reg_mode_value = 0;

	struct i2c_client *client = chip->client;
	printk("cw2015_verify_update_battery_info:==========>\n");
	//pr_warn("cw2015_verify_update_battery_info:==========>\n");
	/* make sure not in sleep mode */
	ret = i2c_smbus_read_byte_data(client, REG_MODE);
	if(ret < 0) {
		dev_err(&client->dev, "Error read mode\n");
		return ret;
	}

	value = ret;
	reg_mode_value = value; /* save MODE value for later use */
	if((value & MODE_SLEEP_MASK) == MODE_SLEEP) {
		dev_err(&client->dev, "Error, device in sleep mode, cannot update battery info\n");
		return -1;
	}

	/* update new battery info */
	for(i=0; i<SIZE_BATINFO; i++) {
		pr_warn("cw2015_verify:table[%d]=%x\n",i,*(chip->data_table+i));
		ret = i2c_smbus_write_byte_data(client, REG_BATINFO+i, *(chip->data_table+i));
		if(ret < 0) {
			dev_err(&client->dev, "Error update battery info @ offset %d, ret = 0x%x\n", i, ret);
			return ret;
		}
	}

	/* readback & check */
	for(i=0; i<SIZE_BATINFO; i++) {
		ret = i2c_smbus_read_byte_data(client, REG_BATINFO+i);
		if(ret < 0) {
			dev_err(&client->dev, "Error read origin battery info @ offset %d, ret = 0x%x\n", i, ret);
			return ret;
		}

		buffer[i] = ret;
	}

	if(0 != memcmp(buffer, chip->data_table, SIZE_BATINFO)) {
		dev_info(&client->dev, "battery info NOT matched, after readback.\n");
		return -1;
	} else {
		dev_info(&client->dev, "battery info matched, after readback.\n");
	}

	/* set 2015 to use new battery info */
	ret = i2c_smbus_read_byte_data(client, REG_CONFIG);
	if(ret < 0) {
		dev_err(&client->dev, "Error to read CONFIG\n");
		return ret;
	}
	value = ret;
	  
	value |= CONFIG_UPDATE_FLG;/* set UPDATE_FLAG */
	value &= 0x7;  /* clear ATHD */
	value |= ATHD; /* set ATHD */
	
	ret = i2c_smbus_write_byte_data(client, REG_CONFIG, value);
	if(ret < 0) {
		dev_err(&client->dev, "Error to update flag for new battery info\n");
		return ret;
	}
	
	/* check 2015 for ATHD&update_flag */
	ret = i2c_smbus_read_byte_data(client, REG_CONFIG);
	if(ret < 0) {
		dev_err(&client->dev, "Error to read CONFIG\n");
		return ret;
	}
	value = ret;
	
	if (!(value & CONFIG_UPDATE_FLG)) {
	  dev_info(&client->dev, "update flag for new battery info have not set\n");
	}
	if ((value & 0xf8) != ATHD) {
	  dev_info(&client->dev, "the new ATHD have not set\n");
	}	  

	reg_mode_value &= ~(MODE_RESTART);  /* RSTART */
	ret = i2c_smbus_write_byte_data(client, REG_MODE, reg_mode_value|MODE_RESTART);
	if(ret < 0) {
		dev_err(&client->dev, "Error to restart battery info1\n");
		return ret;
	}
	ret = i2c_smbus_write_byte_data(client, REG_MODE, reg_mode_value|0);
	if(ret < 0) {
		dev_err(&client->dev, "Error to restart battery info2\n");
		return ret;
	}
	return 0;
}


static int cw2015_gasgauge_get_mvolts(struct i2c_client *client)
{
	int ret = 0;
	short ustemp =0,ustemp1 =0,ustemp2 =0,ustemp3 =0;
	int voltage;
	
	//printk("%s\n", __FUNCTION__);

	if(!client) return -EINVAL;

	struct cw2015_chip *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->mutex);
	if (chip && chip->shutdown_complete) {
		mutex_unlock(&chip->mutex);
		return -ENODEV;
	}


	ret = i2c_smbus_read_word_data(client, REG_VCELL);
	if(ret < 0) {
		dev_err(&client->dev, "Error read VCELL\n");
		goto fail;
	}
	ustemp = ret;
	ustemp = cpu_to_be16(ustemp);
	
	ret = i2c_smbus_read_word_data(client, REG_VCELL);
	if(ret < 0) {
		dev_err(&client->dev, "Error read VCELL\n");
		goto fail;
	}
	ustemp1 = ret;
	ustemp1 = cpu_to_be16(ustemp1);
	
	ret = i2c_smbus_read_word_data(client, REG_VCELL);
	if(ret < 0) {
		dev_err(&client->dev, "Error read VCELL\n");
		goto fail;
	}
	ustemp2 = ret;
	ustemp2 = cpu_to_be16(ustemp2);
	
	if(ustemp >ustemp1)
	{	 
	   ustemp3 =ustemp;
		  ustemp =ustemp1;
		 ustemp1 =ustemp3;
        }
	if(ustemp1 >ustemp2)
	{
	   ustemp3 =ustemp1;
		 ustemp1 =ustemp2;
		 ustemp2 =ustemp3;
	}	
	if(ustemp >ustemp1)
	{	 
	   ustemp3 =ustemp;
		  ustemp =ustemp1;
		 ustemp1 =ustemp3;
        }			

	/* 1 voltage LSB is 305uV, ~312/1024mV */
	// voltage = value16 * 305 / 1000;
	voltage = ustemp1 * 312 / 1024; 
	mutex_unlock(&chip->mutex);
	return voltage;
fail:
	mutex_unlock(&chip->mutex);
	return ret;
}
#if 0
static int cw2015_gasgauge_get_capacity(struct i2c_client *client)
{
	int ret = 0;
	short value16 = 0;
	int soc;

	if(!client) return -EINVAL;
	struct cw2015_chip *chip = i2c_get_clientdata(client);
	
	mutex_lock(&chip->mutex);
	if (chip && chip->shutdown_complete) {
		mutex_unlock(&chip->mutex);
		return -ENODEV;
	}


	ret = i2c_smbus_read_word_data(client, REG_SOC);
	if(ret < 0) {
		dev_err(&client->dev, "Error read SOC\n");
		goto fail;
	}
	value16 = ret;
	value16 = cpu_to_be16(value16);
	
	if (g_sitemp >=0xFF00) {
    	g_sitemp =value16;
	} else {
	    if ((abs(g_sitemp -value16) <=0xFF) && (value16 >0xFF) && (value16 <0x6300)) {
			value16 =g_sitemp; 
	    } 
	    else 
	    {
	        g_sitemp =value16;
	        
	        if ((value16 <= 0xFF)&&(value16 >0)) {
	            g_sitemp =0;
	            value16 =0x0100;
	        }
	        
	        if (value16 >= 0x6300) {
				g_sitemp =0x6380;
	        }

	    }
     }
 
  	soc =  value16 >> 8; 
	//dev_info(&client->dev, "read SOC %d%%. value16 0x%x\n", soc, value16);	
	mutex_unlock(&chip->mutex);
	return soc;
fail:
	mutex_unlock(&chip->mutex);
	return ret;
}
#endif

static int cw_write(struct i2c_client *client, u8 reg, u8 const buf[])
{
        int ret;
		
	 ret = i2c_smbus_write_byte_data(client, reg, buf[0]);
        return ret;
}
static int cw_read_word(struct i2c_client *client, u8 reg, u8 buf[])
{
        int ret;
	 short value16 = 0;
	 ret = i2c_smbus_read_word_data(client, reg);
	 if(ret>=0){
	 	value16 = ret;
	 	value16 = cpu_to_be16(value16);
		buf[0] = (u8) (value16 >> 8);
		buf[1] = (u8) (value16 & 0xff);
		//printk("----------------the cw201x buf[0] = %d,buf[1] = %d funciton: %s, line: %d\n", buf[0], buf[1],  __func__, __LINE__);
	 }
        return ret;
}
static void cw_update_time_member_ac_online(struct cw2015_chip *cw_bat)
{
        struct timespec ts;
        int new_run_time;
        int new_sleep_time;

        ktime_get_ts(&ts);
        new_run_time = ts.tv_sec;

        get_monotonic_boottime(&ts);
        new_sleep_time = ts.tv_sec - new_run_time;

        cw_bat->run_time_ac_online = new_run_time;
        cw_bat->sleep_time_ac_online = new_sleep_time; 
}

static void cw_update_time_member_capacity(struct cw2015_chip *cw_bat)
{
        struct timespec ts;
        int new_run_time;
        int new_sleep_time;

        ktime_get_ts(&ts);
        new_run_time = ts.tv_sec;

        get_monotonic_boottime(&ts);
        new_sleep_time = ts.tv_sec - new_run_time;

        cw_bat->run_time_capacity = new_run_time;
        cw_bat->sleep_time_capacity = new_sleep_time; 
}
static int cw_quickstart(struct cw2015_chip *chip)
{
        int ret = 0;
        u8 reg_val = MODE_QUICK_START;

        printk("---------function: %s\n", __func__);
        ret = cw_write(chip->client, REG_MODE, &reg_val);     //(MODE_QUICK_START | MODE_NORMAL));  // 0x30
        if(ret < 0) {
                printk("Error quick start1\n");
                return ret;
        }
        
        reg_val = MODE_NORMAL;
        ret = cw_write(chip->client, REG_MODE, &reg_val);
        if(ret < 0) {
                printk("Error quick start2\n");
                return ret;
        }
        //printk("---------function: %s, cw_get_capacity = %d\n", __func__, cw_get_capacity(cw_bat));
        return 1;
}

static int cw2015_gasgauge_get_capacity(struct i2c_client *client)
{
        int cw_capacity;
        int ret;
        u8 reg_val[2];

        struct timespec ts;
        long new_run_time;
        long new_sleep_time;
        long capacity_or_aconline_time;
        int allow_change;
        int allow_capacity;
        static int if_quickstart = 0;
        static int jump_flag =0;
        static int reset_loop =0;
        u8 reset_val;		
	if(!client) return -EINVAL;
	
	struct cw2015_chip *chip = i2c_get_clientdata(client);
	
	mutex_lock(&chip->mutex);
	if (chip && chip->shutdown_complete) {
		mutex_unlock(&chip->mutex);
		return -ENODEV;
	}
	
        ret = cw_read_word(client, REG_SOC, &reg_val);
        if(ret < 0) {
		dev_err(&client->dev, "Error read SOC\n");
		goto fail;
	}
        
        cw_capacity = reg_val[0];
        if ((cw_capacity < 0) || (cw_capacity > 100)) {
                dev_info(&client->dev, "get cw_capacity error; cw_capacity = %d\n", cw_capacity);
                reset_loop++;
                
            if (reset_loop >5){ 
            	
				ret = cw_quickstart(chip);
				if (ret < 0) 
				  return ret;				
                dev_info(&chip->client->dev, "report battery capacity error");                              
                ret = cw2015_verify_update_battery_info(chip);
                   if (ret < 0) 
                     return ret;
                reset_loop =0;  
                             
            }
            goto out;
        }else {
        	reset_loop =0;
        }				
				//goto out;

        //if (cw_capacity == 0) 
                //dev_info(&client->dev,"the cw201x capacity is 0 !!!!!!!, funciton: %s, line: %d\n", __func__, __LINE__);
       // else 
               // dev_info(&client->dev,"the cw201x capacity is %d, funciton: %s\n", cw_capacity, __func__);

        //ret = cw_read(chip->client, REG_SOC + 1, &reg_val);

        ktime_get_ts(&ts);
        new_run_time = ts.tv_sec;

        get_monotonic_boottime(&ts);
        new_sleep_time = ts.tv_sec - new_run_time;
        
        if (((chip->charger_mode > 0) && (cw_capacity <= (chip->soc - 1)) && (cw_capacity > (chip->soc - 9)))
                        || ((chip->charger_mode == 0) && (cw_capacity == (chip->soc + 1)))) {             // modify battery level swing

                if (!(cw_capacity == 0 && chip->soc == 1)) {			
		                  cw_capacity = chip->soc;
		            }
        }

        if ((chip->charger_mode > 0) && (cw_capacity >= 95) && (cw_capacity <= chip->soc)) {     // avoid no charge full

                capacity_or_aconline_time = (chip->sleep_time_capacity > chip->sleep_time_ac_online) ? chip->sleep_time_capacity : chip->sleep_time_ac_online;
                capacity_or_aconline_time += (chip->run_time_capacity > chip->run_time_ac_online) ? chip->run_time_capacity : chip->run_time_ac_online;
                allow_change = (new_sleep_time + new_run_time - capacity_or_aconline_time) / BATTERY_UP_MAX_CHANGE;
   //              printk("new_sleep_time =%d,new_run_time=%d,capacity_or_aconline_time=%d,allow_change=%d\n"
                    //      ,new_sleep_time,new_run_time,capacity_or_aconline_time,allow_change);				
                if (allow_change > 0) {
		//			printk("111111111\n");
                        allow_capacity = chip->soc + allow_change; 
                        cw_capacity = (allow_capacity <= 100) ? allow_capacity : 100;
                        jump_flag =1;
                }
                else if (cw_capacity <= chip->soc) {
			//		printk("22222222222222\n");
                        cw_capacity = chip->soc; 
                }
        } 
        else if ((chip->charger_mode == 0) && (cw_capacity <= chip->soc ) && (cw_capacity >= 90) && (jump_flag == 1)) {     // avoid battery level jump to chip
                capacity_or_aconline_time = (chip->sleep_time_capacity > chip->sleep_time_ac_online) ? chip->sleep_time_capacity : chip->sleep_time_ac_online;
                capacity_or_aconline_time += (chip->run_time_capacity > chip->run_time_ac_online) ? chip->run_time_capacity : chip->run_time_ac_online;
                allow_change = (new_sleep_time + new_run_time - capacity_or_aconline_time) / BATTERY_DOWN_CHANGE;
                if (allow_change > 0) {
                        allow_capacity = chip->soc - allow_change; 
                        if (cw_capacity >= allow_capacity){
                        	jump_flag =0;
                        }
                        else{
                        cw_capacity = (allow_capacity <= 100) ? allow_capacity : 100;
                      }
                }
                else if (cw_capacity <= chip->soc) {
                        cw_capacity = chip->soc;
                }
        } 
        if ((cw_capacity == 0) && (chip->soc > 1)) {              // avoid battery level jump to 0% at a moment from more than 2%
                allow_change = ((new_run_time - chip->run_time_capacity) / BATTERY_DOWN_MIN_CHANGE_RUN);
                allow_change += ((new_sleep_time - chip->sleep_time_capacity) / BATTERY_DOWN_MIN_CHANGE_SLEEP);

                allow_capacity = chip->soc - allow_change;
                cw_capacity = (allow_capacity >= cw_capacity) ? allow_capacity: cw_capacity;
                reg_val[0] = MODE_NORMAL;
                ret = cw_write(chip->client, REG_MODE, &reg_val);
                 if(ret < 0) {
			              dev_err(&client->dev, "Error write mode\n");
			              goto fail;
		    }
        }
 
#if 1	
	if((chip->charger_mode > 0) &&(cw_capacity == 0))
	{		  
                if (((new_sleep_time + new_run_time - chip->sleep_time_ac_online - chip->run_time_ac_online) > BATTERY_DOWN_MAX_CHANGE_RUN_AC_ONLINE) && (if_quickstart == 0)) {
        		cw_quickstart(chip);      // if the cw_capacity = 0 the cw2015 will qstrt
                        if_quickstart = 1;
                }
	} else if ((if_quickstart == 1)&&(chip->charger_mode == 0)) {
                if_quickstart = 0;
        }
#endif
#if 0
        if (chip->pdata->chg_ok_pin != INVALID_GPIO){ 
            if(cw_capacity >= 100)&&(chip->charger_mode > 0)
               &&(gpio_get_value(chip->pdata->chg_ok_pin) != chip->pdata->chg_ok_level))
               cw_capacity = 99;
        }
#endif
#ifdef SYSTEM_SHUTDOWN_VOLTAGE
        if ((chip->charger_mode == 0) && (cw_capacity <= 10) && (chip->voltage <= SYSTEM_SHUTDOWN_VOLTAGE)){      	     
                if (if_quickstart == 10){       	      	
                        cw_quickstart(chip);
                        if_quickstart = 12;
                        cw_capacity = 0;
                } else if (if_quickstart <= 10)
                        if_quickstart =if_quickstart+2;
                dev_info(&client->dev, "the cw201x voltage is less than SYSTEM_SHUTDOWN_VOLTAGE !!!!!!!, funciton: %s, line: %d\n", __func__, __LINE__);
        } else if ((chip->charger_mode > 0)&& (if_quickstart <= 12)) {
                if_quickstart = 0;
        }
#endif
out:
	mutex_unlock(&chip->mutex);
	return cw_capacity;
fail:
	pr_err("%s:===========>fail!!",__func__);
	mutex_unlock(&chip->mutex);
	return ret;
}
static int cw2015_gasgauge_get_rrt(struct i2c_client *client)
{
	int ret = 0;
	short value16 = 0;
	int rrt;
	struct cw2015_chip *chip = NULL;
	if(!client) return -EINVAL;
	chip = i2c_get_clientdata(client);
		
	mutex_lock(&chip->mutex);
	if (chip && chip->shutdown_complete) {
		mutex_unlock(&chip->mutex);
		return -ENODEV;
	}

	ret = i2c_smbus_read_word_data(client, REG_RRT_ALERT);
	if(ret < 0) {
		dev_err(&client->dev, "Error read RRT\n");
		goto fail;
	}
	value16 = ret;
	value16 = cpu_to_be16(value16);
	rrt = value16 & 0x1fff;	
	//dev_info(&client->dev, "read RRT %d%%. value16 0x%x\n", rrt, value16);

	mutex_unlock(&chip->mutex);
	return rrt;
fail:
	mutex_unlock(&chip->mutex);
	return ret;

}


static void cw2015_get_vcell(struct i2c_client *client)
{
	int vcell;
	struct cw2015_chip *chip = i2c_get_clientdata(client);
	

	vcell = cw2015_gasgauge_get_mvolts(client);
	if (vcell < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, vcell);
	else
		chip->vcell = vcell*1000;
}

static void cw2015_get_soc(struct i2c_client *client)
{
	int soc;
	struct cw2015_chip *chip = i2c_get_clientdata(client);
	

	soc = cw2015_gasgauge_get_capacity(client);
	if (soc < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, soc);
	else
		chip->soc = soc;
	if (soc == 0)
		{
			printk("chip----->soc=%d\n",soc);
		}
	if (chip->soc >= CW2015_BATTERY_FULL) {
		chip->soc = CW2015_BATTERY_FULL;
		if(Battery_status==4||Battery_status==1)
			{
		chip->status = POWER_SUPPLY_STATUS_FULL;
			}
		chip->capacity_level = POWER_SUPPLY_CAPACITY_LEVEL_FULL;
		chip->health = POWER_SUPPLY_HEALTH_GOOD;
	}
	else if (chip->soc < CW2015_BATTERY_LOW) {
		chip->health = POWER_SUPPLY_HEALTH_DEAD;
		chip->capacity_level = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
	} else {
		chip->health = POWER_SUPPLY_HEALTH_GOOD;
		chip->capacity_level = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
	}
}

static void cw2015_get_rrt(struct i2c_client *client)
{
	int rrt;
	struct cw2015_chip *chip = i2c_get_clientdata(client);


	rrt = cw2015_gasgauge_get_rrt(client);
	if (rrt < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, rrt);
	else
		chip->rrt = rrt;
}

#ifdef WARK_UP_FLAG
static int cw_get_alt()
{
	int ret = 0;
	u8 value8 = 0;
	int alrt;
	ret = i2c_smbus_read_byte_data(cw2015_data->client, REG_RRT_ALERT);
	if (ret < 0)
		return ret;
	value8 = ret;
	alrt = value8 >>7;
	printk(&cw2015_data->client->dev, "read RRT %d%%. value16 0x%x\n", alrt, ret);
	value8 = value8&0x7f;
	ret = value8;
	ret = i2c_smbus_write_byte_data(cw2015_data->client, REG_RRT_ALERT, &ret);
	if(ret < 0) {
		dev_err(&cw2015_data->client->dev, "Error clear ALRT\n");
		return ret;
	}
	return alrt;
}

#endif


#ifdef WARK_UP_FLAG
static void athd_irq_work_func(struct work_struct *work)
{

	struct cw2015_chip *chip =
		container_of(work, struct cw2015_chip, work);
	struct input_dev *input = chip->input;
	int displayed = 0;
	cw_get_alt();// clear ALRT
	if(cw2015_data->status == POWER_SUPPLY_STATUS_DISCHARGING)
	{
		if(chip&&chip->pdata&&chip->pdata->enable_screen_on_gpio>0){
			displayed = (gpio_get_value_cansleep(chip->pdata->enable_screen_on_gpio) ? 0 : 1)^chip->pdata->active_state;
			printk("displayed = %d\n",displayed);			
			if(!displayed){
				input_event(input, EV_KEY, KEY_POWER, 1);
				input_event(input, EV_KEY, KEY_POWER, 0);
				input_sync(input);
			}
		}
	}

}
#endif

static void cw2015_work(struct work_struct *work)
{
	struct cw2015_chip *chip;

	chip = container_of(work, struct cw2015_chip, work.work);

	cw2015_get_vcell(chip->client);
	cw2015_get_soc(chip->client);
	cw2015_get_rrt(chip->client);
	
	if (chip->vcell != chip->lasttime_vcell ||
		chip->soc != chip->lasttime_soc ||
		chip->status != chip->lasttime_status ||
		chip->rrt != chip->lasttime_rrt) {
		chip->lasttime_vcell = chip->vcell;
		chip->lasttime_status = chip->status;
		chip->lasttime_rrt = chip->rrt;		
		power_supply_changed(&chip->battery);
	}
	if(chip->soc != chip->lasttime_soc)
			{
				cw_update_time_member_capacity(chip);
				chip->lasttime_soc = chip->soc;
				power_supply_changed(&chip->battery);
				//printk("chip->lasttime_soc = chip->soc\n");
			}


	schedule_delayed_work(&chip->work, CW2015_DELAY);
}
int g_charge_status = 0;
void cw2015_battery_status(int status,
				int chrg_type)
{
	printk("charge_status======%d\n",status);
	g_charge_status = status;
	if (!cw2015_data)
		return;

	cw2015_data->ac_online = 0;
	cw2015_data->usb_online = 0;
	cw2015_data->charger_mode = 0;
	Battery_status =status;
	//dev_info(&cw2015_data->client->dev,"%s: status= %d chrg_type = %d \n",__func__,status,chrg_type);
	if (status == bq2419_progress||status == bq2419_completed) {
		cw2015_data->status = POWER_SUPPLY_STATUS_CHARGING;
		if(cw2015_data->soc >=CW2015_BATTERY_FULL)
				cw2015_data->status = POWER_SUPPLY_STATUS_FULL;	
		if (chrg_type == BQ2419x_VBUS_AC){
			cw2015_data->ac_online = 1;
			cw2015_data->charger_mode = 1;
		}else if (chrg_type == BQ2419x_VBUS_USB){
			cw2015_data->usb_online = 1;
			cw2015_data->charger_mode = 1;
		}
	}
	else{
	
		//dev_info(&cw2015_data->client->dev,"%s: status= %d chrg_type = %d \n","else",status,chrg_type);
//	if(cw2015_data->status != POWER_SUPPLY_STATUS_FULL&&cw2015_data->status != POWER_SUPPLY_STATUS_DISCHARGING)
		cw2015_data->status = POWER_SUPPLY_STATUS_DISCHARGING;
	}
	cw_update_time_member_ac_online(cw2015_data);
	//power_supply_changed(&cw2015_data->battery);
	//if (cw2015_data->use_usb)
	//	power_supply_changed(&cw2015_data->usb);
	//if (cw2015_data->use_ac)
	//	power_supply_changed(&cw2015_data->ac);
}
EXPORT_SYMBOL_GPL(cw2015_battery_status);
int get_current_charge_status(void)
{
	/*printk("=======%s = %d\n",__func__,g_charge_status);*/
	return g_charge_status;
}
EXPORT_S_GPL(get_current_charge_status);
static enum power_supply_property cw2015_battery_props[] = {
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	//POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property cw2015_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property cw2015_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int cw2015_initialize(struct cw2015_chip *chip)
{
    int cnt = 0;
    int i = 0;
	int ret = 0;
	char value = 0;
	char buffer[SIZE_BATINFO*2];
	short value16 = 0;

	struct i2c_client *client = chip->client;
		
	mutex_lock(&chip->mutex);
	if (chip && chip->shutdown_complete) {
		mutex_unlock(&chip->mutex);
		return -ENODEV;
	}

#if FORCE_WAKEUP_CHIP
	value = MODE_SLEEP;
#else

	/* check if sleep mode, bring up */
	ret = i2c_smbus_read_byte_data(client, REG_MODE);
	if(ret < 0) {
		dev_err(&client->dev, "read mode\n");
		goto fail;
	}

	value = ret;
#endif

	if((value & MODE_SLEEP_MASK) == MODE_SLEEP) {
                /* do wakeup cw2015 */
                ret = i2c_smbus_write_byte_data(client, REG_MODE, MODE_NORMAL);
                if(ret < 0) {
                        dev_err(&client->dev, "Error update mode\n");
                        goto fail;
                }
                /* check 2015 if not set ATHD */
                ret = i2c_smbus_read_byte_data(client, REG_CONFIG);
                if(ret < 0) {
                        dev_err(&client->dev, "Error to read CONFIG\n");
                        goto fail;
                }
                value = ret;
                if ((value & 0xf8) != ATHD) {
                        dev_info(&client->dev, "the new ATHD have not set\n");
                        value &= 0x7;  /* clear ATHD */
                        value |= ATHD; 
                        /* set ATHD */
                        ret = i2c_smbus_write_byte_data(client, REG_CONFIG, value);
                        if(ret < 0) {
                                dev_err(&client->dev, "Error to set new ATHD\n");
                                goto fail;
                        }
                }
            

                /* check 2015 for update_flag */
                ret = i2c_smbus_read_byte_data(client, REG_CONFIG);
                if(ret < 0) {
                        dev_err(&client->dev, "Error to read CONFIG\n");
                        goto fail;
                }
                value = ret;  	    	 
                /* not set UPDATE_FLAG,do update_battery_info  */
                if (!(value & CONFIG_UPDATE_FLG)) {
                        dev_info(&client->dev, "update flag for new battery info have not set\n");
                        cw2015_verify_update_battery_info(chip);
                }

  	        /* read origin info */
                for(i=0; i<SIZE_BATINFO; i++) {
                        ret = i2c_smbus_read_byte_data(client, REG_BATINFO+i);
                        if(ret < 0) {
                                dev_err(&client->dev, "Error read origin battery info @ offset %d, ret = 0x%x\n", i, ret);
                               goto fail;
                        }
                        buffer[i] = ret;
                }

                if(0 != memcmp(buffer, chip->data_table, SIZE_BATINFO)) {
                        //dev_info(&client->dev, "battery info NOT matched.\n");
                        /* battery info not matched,do update_battery_info  */
                        cw2015_verify_update_battery_info(chip);
                } else {
                        //dev_info(&client->dev, "battery info matched.\n");
                }
                
		/* do wait valide SOC, if the first time wakeup after poweron */
		ret = i2c_smbus_read_word_data(client, REG_SOC);
		if(ret < 0) {
			dev_err(&client->dev, "Error read init SOC\n");
			goto fail;
		}   
		value16 = ret;		    
		
		while ((value16 == 0xff)&&(cnt < 10000)) {     //SOC value is not valide or time is not over 3 seconds
                        ret = i2c_smbus_read_word_data(client, REG_SOC);		  
                        if(ret < 0) {
                                dev_err(&client->dev, "Error read init SOC\n");
                                goto fail;
                        }   
                        value16 = ret;
                        cnt++;
                }
                
        }
		 for (i = 0; i < 30; i++) {
                ret = i2c_smbus_read_byte_data(client, REG_SOC);
				
                if (ret < 0)
                        return ret;
				value16=ret;
                 if (value16 <= 0x64) 
                        break;
                
                msleep(100);
                if (i > 25)
                        dev_err(&client->dev, "cw2015/cw2013 input unvalid power error\n");

        }
        if (i >=30){
        	   value = MODE_SLEEP;
             ret = i2c_smbus_write_byte_data(client, REG_MODE, value);
            // dev_info(&client->dev, "report battery capacity error");
             return -1;
        } 
	mutex_unlock(&chip->mutex);
	return 0;
fail:
	mutex_unlock(&chip->mutex);
	return ret;
}

static uint16_t cw2015_get_version(const struct i2c_client *client){
	int ret;
	uint16_t version;
	short value16;
	ret = i2c_smbus_read_word_data(client, REG_VERSION);
	if(ret < 0) {
		dev_err(&client->dev, "Error read RRT\n");
		return -1;
	}
	value16 = ret;
	value16 = cpu_to_be16(value16);
	version = value16;	
	return version;
}




int cw2015_check_battery()
{

	uint16_t version;

	if (!cw2015_data)
		return -ENODEV;

	version = cw2015_get_version(cw2015_data->client);
	dev_info(&cw2015_data->client->dev,
"%s:version=0x%x\n",__func__,version);
#if 0	
	if (version != MAX17048_VERSION_NO) {
		return -ENODEV;
	}
#endif 

	return 0;
}
EXPORT_SYMBOL_GPL(cw2015_check_battery);



#if  WARK_UP_FLAG
static irqreturn_t cw2015_irq_handler(int irq, void *dev_id)
{

	struct cw2015_chip *chip = dev_id;
	printk("cw2015_irq_handler11111111111\n");
	schedule_work(&chip->athd_work);
		return IRQ_HANDLED;
}

#endif


static int __devinit cw2015_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct cw2015_chip *chip;
	//struct input_dev *input;
	int ret;
	int loop = 0;
	
	
	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;
	chip->client = client;
	chip->pdata = client->dev.platform_data;
	chip->ac_online = 0;
	chip->usb_online = 0;

	if(chip->pdata->data_tbl){
		pr_warn("cw2015:use pdata tabl\n");
		chip->data_table = chip->pdata->data_tbl;
	}else
		chip->data_table = cw2015_bat_config_info;
	
	cw2015_data = chip;
	mutex_init(&chip->mutex);
	chip->shutdown_complete = 0;
	i2c_set_clientdata(client, chip);
	
	cw2015_check_battery();
	ret = cw2015_initialize(chip);
	if (ret < 0) {
		while ((loop++ < 200) && (ret != 0)) {
            ret = cw2015_initialize(chip);
        }
	}
  if (ret){ 
		dev_err(&client->dev, "Error: Initializing fuel-gauge\n");
		goto error2;
	}

	chip->battery.name		= "battery";
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->battery.get_property	= cw2015_get_property;
	chip->battery.properties	= cw2015_battery_props;
	chip->battery.num_properties	= ARRAY_SIZE(cw2015_battery_props);
	chip->status			= POWER_SUPPLY_STATUS_DISCHARGING;

	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
		goto error2;
	}

	if (chip->pdata&&chip->pdata->use_ac) {
		chip->ac.name		= "maxim-ac";
		chip->ac.type		= POWER_SUPPLY_TYPE_MAINS;
		chip->ac.get_property	= cw2015_ac_get_property;
		chip->ac.properties	= cw2015_ac_props;
		chip->ac.num_properties	= ARRAY_SIZE(cw2015_ac_props);

		ret = power_supply_register(&client->dev, &chip->ac);
		if (ret) {
			dev_err(&client->dev, "failed: power supply register\n");
			goto error1;
		}
	}

	if (chip->pdata&&chip->pdata->use_usb) {
		chip->usb.name		= "maxim-usb";
		chip->usb.type		= POWER_SUPPLY_TYPE_USB;
		chip->usb.get_property	= cw2015_usb_get_property;
		chip->usb.properties	= cw2015_usb_props;
		chip->usb.num_properties = ARRAY_SIZE(cw2015_usb_props);

		ret = power_supply_register(&client->dev, &chip->usb);
		if (ret) {
			dev_err(&client->dev, "failed: power supply register\n");
			goto error;
		}
	}

	cw_update_time_member_capacity(chip);        
	cw_update_time_member_ac_online(chip);

	INIT_DELAYED_WORK_DEFERRABLE(&chip->work, cw2015_work);

	schedule_delayed_work(&chip->work, CW2015_DELAY);
	
#if  WARK_UP_FLAG	
	chip->input = input_allocate_device();
	if (!chip->input) {
		dev_err(&client->dev, "error for allocate device\n");
		ret = -ENOMEM;
		goto error2;
	}

	chip->input->name ="cw2015_wake_up";
	chip->input->id.bustype = BUS_I2C;
	chip->input->dev.parent = &client->dev;
	chip->input->evbit[0] = BIT_MASK(EV_KEY);

	input_set_capability(chip->input, EV_KEY, KEY_POWER);

	ret = input_register_device(chip->input);
	if (ret) {
		dev_err(&client->dev, "error register device\n");
		ret = -EINVAL;
		goto error2;
	}
	

	INIT_WORK(&chip->athd_work, athd_irq_work_func);


if(client->irq)
	{
		ret  = request_threaded_irq(client->irq, NULL,cw2015_irq_handler ,IRQ_TYPE_EDGE_FALLING,
								client->name, chip);
		if (ret< 0) {
			printk(KERN_WARNING "cw2015: unable to get irq %d\n",client->irq);
			goto error2;
		}
		enable_irq_wake(client->irq);
	}
#endif	
	dev_info(&client->dev, "%s:========>register success!\n", __func__);
	return 0;
error:
	if (chip->pdata&&chip->pdata->use_ac)
	power_supply_unregister(&chip->ac);
error1:
	power_supply_unregister(&chip->battery);
error2:
#if WARK_UP_FLAG
	if (client->irq)
		free_irq(client->irq, chip);
#endif
	mutex_destroy(&chip->mutex);
	kfree(chip);

	dev_info(&client->dev, "%s:========>register fail!\n", __func__);
	return ret;
}

static int __devexit cw2015_remove(struct i2c_client *client)
{
	struct cw2015_chip *chip = i2c_get_clientdata(client);

	power_supply_unregister(&chip->battery);
	
	if (chip->pdata&&chip->pdata->use_usb)
	power_supply_unregister(&chip->usb);
	
	if (chip->pdata&&chip->pdata->use_ac)
	power_supply_unregister(&chip->ac);
#if WARK_UP_FLAG
	if (client->irq)
		free_irq(client->irq, chip);
	cancel_work_sync(&chip->athd_work);
	input_unregister_device(chip->input);
#endif	
	cancel_delayed_work_sync(&chip->work);
	mutex_destroy(&chip->mutex);
	kfree(chip);

	return 0;
}

static void cw2015_shutdown(struct i2c_client *client)
{
	struct cw2015_chip *chip = i2c_get_clientdata(client);

	cancel_delayed_work_sync(&chip->work);
#if WARK_UP_FLAG	
	cancel_work_sync(&chip->athd_work);
#endif
	mutex_lock(&chip->mutex);
	chip->shutdown_complete = 1;
	mutex_unlock(&chip->mutex);

}

#ifdef CONFIG_PM

static int cw2015_suspend(struct i2c_client *client,
		pm_message_t state)
{
	struct cw2015_chip *chip = i2c_get_clientdata(client);
#if WARK_UP_FLAG
	if(client->irq)
		enable_irq_wake(client->irq);
#endif	
	cancel_delayed_work_sync(&chip->work);
	return 0;
}

static int cw2015_resume(struct i2c_client *client)
{
	struct cw2015_chip *chip = i2c_get_clientdata(client);
#if WARK_UP_FLAG
		if(client->irq)
			disable_irq_wake(client->irq);
#endif

	schedule_delayed_work(&chip->work, CW2015_DELAY);

	return 0;
}

#else

#define cw2015_suspend NULL
#define cw2015_resume NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id cw2015_id[] = {
	{ "cw2015", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cw2015_id);

static struct i2c_driver cw2015_i2c_driver = {
	.driver	= {
		.name	= "cw2015",
	},
	.probe		= cw2015_probe,
	.remove		= __devexit_p(cw2015_remove),
	.suspend	= cw2015_suspend,
	.resume		= cw2015_resume,
	.id_table	= cw2015_id,
	.shutdown	= cw2015_shutdown,
};

static int __init cw2015_init(void)
{
	return i2c_add_driver(&cw2015_i2c_driver);
}
subsys_initcall(cw2015_init);

static void __exit cw2015_exit(void)
{
	i2c_del_driver(&cw2015_i2c_driver);
}
module_exit(cw2015_exit);

MODULE_AUTHOR("Chandler Zhang <chazhang@nvidia.com>");
MODULE_DESCRIPTION("MAX17048 Fuel Gauge");
MODULE_LICENSE("GPL");
