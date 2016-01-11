/*****************************************************************************
* Copyright(c) O2Micro, 2013. All rights reserved.
*	
* O2Micro OZ8503 USB Express Charger Interface
* File: o2_ec_adapter.c

* Author: Eason.yuan
* $Source: /data/code/CVS
* $Revision:  $
*
* This program is free software and can be edistributed and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*	
* This Source Code Reference Design for O2MICRO OZ8503 access (\u201cReference Design\u201d) 
* is sole for the use of PRODUCT INTEGRATION REFERENCE ONLY, and contains confidential 
* and privileged information of O2Micro International Limited. O2Micro shall have no 
* liability to any PARTY FOR THE RELIABILITY, SERVICEABILITY FOR THE RESULT OF PRODUCT 
* INTEGRATION, or results from: (i) any modification or attempted modification of the 
* Reference Design by any party, or (ii) the combination, operation or use of the 
* Reference Design with non-O2Micro Reference Design.
*****************************************************************************/
#include <linux/kernel.h>

#include <linux/jiffies.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/platform_device.h>

#include "o2_mtk_adapter.h"
#include "parameter.h"

#include <asm/gpio.h>
#include <asm/irq.h>
#include <asm/io.h>

//#include <mach/sys_config.h>
#include <linux/gpio.h>
#include "bluewhale_charger.h"
#include "o2_adapter.h"


#define	RETRY_CNT	10





#define  USB_VBUS_D0    0
#define  USB_VBUS_D1   	1

//#define VBUS_HIGH   1200
#define VBUS_HIGH   900


/*****************************************************************************
Rpull=76k \uff0cRdown=3.6k 
VBUS(V)		4.4v	5v		6v		7v		8v		9v		10V		11v		12v		13v		14v
BI/TS(V)	0.23	0.256	0.301	0.346	0.392	0.437	0.482	0.528	0.574	0.618	0.663
*****************************************************************************/


uint8_t mtk_adapter = 0;

//////////////////////////////////////////////////////////////////////////////////////



/******************************************************************/
static void logic_low(void)
{
	bluewhale_set_vbus_current(100);
}

static void logic_high(void)
{
	bluewhale_set_vbus_current(VBUS_HIGH);
}



void increase(void)
{
	u8 temp = 0;
	
	bluewhale_read_byte(REG_VBUS_LIMIT_CURRENT, &temp);

	logic_low();
	msleep(150);

	logic_high();
	msleep(100);
	logic_low();
	msleep(100);
	logic_high();
	msleep(100);
	logic_low();
	msleep(100);
	logic_high();
	msleep(300);
	logic_low();
	msleep(100);
	logic_high();
	msleep(300);
	logic_low();
	msleep(100);
	logic_high();
	msleep(300);
	logic_low();
	msleep(100);
	logic_high();
	msleep(500);
	
	logic_low();
	msleep(100);

	bluewhale_set_vbus_current(VBUS_HIGH);
	
}

void decrease(void)
{
	u8 temp = 0;
	
	//bluewhale_read_byte(REG_VBUS_LIMIT_CURRENT, &temp);

	logic_low();
	msleep(150);

	logic_high();
	msleep(300);
	logic_low();
	msleep(100);
	logic_high();
	msleep(300);
	logic_low();
	msleep(100);
	logic_high();
	msleep(300);
	logic_low();
	msleep(100);
	logic_high();
	msleep(100);
	logic_low();
	msleep(100);
	logic_high();
	msleep(100);
	logic_low();
	msleep(100);
	logic_high();
	msleep(500);

	logic_low();
	msleep(50);

	bluewhale_set_vbus_current(VBUS_HIGH);
}




int bluewhale_increase_vbus_volt(int vbus_step)
{
	int i;

	for (i = 0; i < vbus_step; i ++)
	{
		increase();
	}

	
}

int bluewhale_decrease_vbus_volt(int vbus_step)
{
	
	int i = 0;
	
	for (i = 0; i < vbus_step; i ++)
		decrease();

}



/*****************************************************************************
 * Description:
 *		charger_init
 * Parameters:
 *		None
 * Return:
 *		see I2C_STATUS_XXX define
 *****************************************************************************/
 void o2_adapter_mtk_init(void)
{
	int ret;

	int i;
	int err;

	printk("O2Micro MTK adpater Driver init\n");
	

}



/*****************************************************************************
 * Description:
 *		o2_adapter_set_voltage
 * Parameters:
 *		None
 * Return:
 *		
 *****************************************************************************/
 bool o2_set_mtk_voltage(int voltage)
{
	
	int32_t voltage_ref, retry_count = 5;
	
	int32_t vbus_current;
	uint8_t i;
	int32_t vubs_voltage;

	
	
	if(voltage == ADAPTER_5V)
		voltage_ref = VBUS_BI_5V;
	else if(voltage == ADAPTER_7V)
		voltage_ref = VBUS_BI_7V;
	else if(voltage == ADAPTER_9V)
		voltage_ref = VBUS_BI_9V;
	else if(voltage == ADAPTER_12V)
		voltage_ref = VBUS_BI_12V;
	else
		return false;

	

	printk("START TO make MTK voltage\n");
	bluewhale_set_vbus_current(VBUS_HIGH);
	//bluewhale_set_charger_current(3500);

	bluewhale_set_charger_current(2000);

	msleep(500);
	vbus_current = bluewhale_get_vbus_current();

	if(vbus_current != VBUS_HIGH)
	{	
		for(i = 0;i < RETRY_CNT;i++)
		{

			vbus_current = bluewhale_get_vbus_current();
			if(vbus_current == VBUS_HIGH)
				break;
			bluewhale_set_vbus_current(VBUS_HIGH);
			msleep(200);
		}
		if(i >= RETRY_CNT)
		{
			printk("fail to make vbus current \n");
		
			bluewhale_set_vbus_current(1000);
			return false;

		}
	}


	for(i = 0;i < RETRY_CNT;i++)
	{

		msleep(3000);
		if(bmu_init_ok)
	 		oz8806_temp_read(&vubs_voltage);

		printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!! times to update %d \n",i);
		printk("vubs_voltage: %d\n",vubs_voltage);
		
		if(vubs_voltage >= voltage_ref)
		{
			printk("success to update voltage \n");
			bluewhale_set_vbus_current(1400);
			bluewhale_set_charger_current(3500);
			//bluewhale_set_charger_current(2000);
			return true;

		}


		bluewhale_increase_vbus_volt(1);
	
	}
	
	if(i >= RETRY_CNT)
	{
		printk("fail to make vbus voltage \n");
		
		bluewhale_set_vbus_current(1000);
	}
		
	
	
	return false;
}







#if 0
/*****************************************************************************
Rpull=76k \uff0cRdown=3.6k 
VBUS(V)		4.4v	5v		6v		7v		8v		9v		10V		11v		12v		13v		14v
BI/TS(V)	0.23	0.256	0.301	0.346	0.392	0.437	0.482	0.528	0.574	0.618	0.663
*****************************************************************************/
void start_update_voltage_fun(void)
{
	int32_t vbus_current;
	uint8_t i;

	if((0 == start_adjust_voltage) || (0 == charger_info->vbus_ok))
		return;

	vbus_current = bluewhale_get_vbus_current();
	if(vbus_current != VBUS_HIGH)
	{	
		for(i = 0;i <= 5;i++)
		{

			vbus_current = bluewhale_get_vbus_current();
			if(vbus_current == VBUS_HIGH)
				break;
			bluewhale_set_vbus_current(VBUS_HIGH);
			msleep(200);
		}
		if(i >= 5)
		{
			printk("fail to make vbus high \n");
			start_adjust_voltage = 0;
			update_voltage_times = 0;
			mtk_adapter = 0;
			bluewhale_set_vbus_current(1000);
			return;

		}
	}
	
	if(bmu_init_ok)
 		oz8806_temp_read(&vubs_voltage);
	
	
		
	if(update_voltage_times > 8)
	{
		printk("fail to update voltage \n");
		start_adjust_voltage = 0;
		update_voltage_times = 0;
		return;
	}
	
	if(vubs_voltage >= VBUS_BI_SUCCESS)
	{
		printk("success to update voltage \n");
		start_adjust_voltage = 0;
		update_voltage_times = 0;
		mtk_adapter = 1;
		return;

	}

	printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!! times to update %d \n",update_voltage_times);
	
	bluewhale_increase_vbus_volt(1);
	update_voltage_times++;
}

#endif



