/* ========================================================================== */
/*                                                                            */
/*   bluewhale_charger.h                                                      */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/* This program is free software; you can redistribute it and/or modify it    */
/* under the terms of the GNU General Public License version 2 as published   */
/* by the Free Software Foundation.											  */
/*																			  */
/* This program is distributed in the hope that it will be useful, but        */
/* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY */
/* or FITNESS FOR A PARTICULAR PURPOSE.										  */
/* See the GNU General Public License for more details.						  */
/*																			  */
/* You should have received a copy of the GNU General Public License along	  */
/* with this program.  If not, see <http://www.gnu.org/licenses/>.			  */
/* ========================================================================== */

#ifndef	__BLUEWHALE_CHARGER_H
#define	__BLUEWHALE_CHARGER_H __FILE__

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/power_supply.h>
#include <linux/workqueue.h>
//#include "table.h"
#include <linux/wakelock.h>

/* Voltage Registers (R/W) */
#define		REG_CHARGER_VOLTAGE			0x00
#define		REG_T34_CHG_VOLTAGE			0x01
#define		REG_T45_CHG_VOLTAGE			0x02
#define			CHG_VOLT_STEP					25	//step 25mV
#define		REG_WAKEUP_VOLTAGE			0x03
#define			WK_VOLT_STEP					100	//step 100mV
#define		REG_RECHARGE_HYSTERESIS		0x04
#define			RECHG_VOLT_STEP					50	//step 50mV
#define		REG_MIN_VSYS_VOLTAGE		0x05
#define			VSYS_VOLT_STEP					200	//step 200mV

/* Current Registers (R/W) */
#define		REG_CHARGE_CURRENT			0x10
#define			CHG_CURRT_STEP					100	//step 100mA
#define		REG_WAKEUP_CURRENT			0x11
#define			WK_CURRT_STEP					10	//step 10mA
#define		REG_END_CHARGE_CURRENT		0x12
#define			EOC_CURRT_STEP					10	//step 10mA
#define		REG_VBUS_LIMIT_CURRENT		0x13
#define			VBUS_ILMT_STEP					100	//step 100mA

/* Protection Register (R/W) */
#define		REG_SAFETY_TIMER			0x20
#define			WAKEUP_TIMER_MASK				0x0F
#define			WK_TIMER_15MIN					0x01	//15min wakeup timer
#define			WK_TIMER_30MIN					0x02	//30min wakeup timer
#define			WK_TIMER_45MIN					0x03	//45min wakeup timer
#define			WK_TIMER_60MIN					0x04	//60min wakeup timer
#define			WK_TIMER_75MIN					0x05	//60min wakeup timer
#define			WK_TIMER_90MIN					0x06	//60min wakeup timer
#define			WK_TIMER_105MIN					0x07	//60min wakeup timer
#define			CC_TIMER_MASK					0xF0
#define			CC_TIMER_120MIN					0x10	//120min CC charge timer
#define			CC_TIMER_180MIN					0x20	//180min CC charge timer
#define			CC_TIMER_240MIN					0x30	//240min CC charge timer
#define			CC_TIMER_300MIN					0x40	//300min CC charge timer
#define			CC_TIMER_390MIN					0x50	//390min CC charge timer
#define			CC_TIMER_480MIN					0x60	//480min CC charge timer
#define			CC_TIMER_570MIN					0x70	//570min CC charge timer

/* Charger Control Register (R/W) */
#define		REG_CHARGER_CONTROL			0x30
#define			RTHM_SELECT						0x01	//0:100K, 1:10K

/* Status Registers (R) */
#define		REG_VBUS_STATUS				0x40
#define			VSYS_OVP_FLAG					0x01	//VSYS OVP event flag
#define			VBUS_UVP_FLAG					0x10	//VBUS UVP event flag
#define			VBUS_OK_FLAG					0x20	//VBUS OK flag
#define			VBUS_OVP_FLAG					0x40	//VBUS OVP event flag
#define			VDC_PR_FLAG                     0x80    // 1 when VDC < VDC threshold for system priority

#define		REG_CHARGER_STATUS			0x41
#define			CHARGER_INIT					0x01	//Before init flag
#define			IN_WAKEUP_STATE					0x02	//In Wakeup State
#define			IN_CC_STATE						0x04	//In CC Charge State
#define			IN_CV_STATE						0x08	//In CV Charge State
#define			CHARGE_FULL_STATE				0x10	//Charge Full State
#define			WK_TIMER_FLAG					0x20	//WK CHG Timer Overflow
#define			CC_TIMER_FLAG					0x40	//CC CHG Timer Overflow 

#define		REG_THM_STATUS				0x42
#define			THM_T1_STATE					0x01	//T1 > THM state 
#define			THM_T12_STATE					0x02	//THM in T12 state 
#define			THM_T23_STATE					0x04	//THM in T23 state 
#define			THM_T34_STATE					0x08	//THM in T34 state 
#define			THM_T45_STATE					0x10	//THM in T45 state 
#define			THM_T5_STATE					0x20	//THM > T5 state 
#define			INT_OTP_FLAG					0x40	//Internal OTP event



enum bluewhale_thermal {
		THM_DISABLE = 0,
        THM_UNDER_T1 = 1,
        THM_RANGE_T12,
        THM_RANGE_T23,
        THM_RANGE_T34,
        THM_RANGE_T45,
        THM_OVER_T5,
		THM_ITOT
};

#define bluewhale_thermal_t \
        enum bluewhale_thermal


struct bluewhale_platform_data {
	int		max_charger_currentmA;
	int		max_charger_voltagemV;
	int		termination_currentmA;
	int		T34_charger_voltagemV;
	int		T45_charger_voltagemV;
	int		wakeup_voltagemV;
	int		wakeup_currentmA;
	int		recharge_voltagemV;
	int		min_vsys_voltagemV;
	int		vbus_limit_currentmA;
	int		max_cc_charge_time;
	int		max_wakeup_charge_time;
	bool	rthm_10k;	//1 for 10K, 0 for 100K thermal resistor
	int     inpedance;
	char **supplied_to; 
};

struct bluewhale_charger_info {
	struct device                	*dev;
	struct i2c_client				*client;
	struct power_supply				usb;
	struct bluewhale_platform_data 	*pdata;
	struct delayed_work 			delay_work;
    struct delayed_work 			adjust_ilimit_work;
	struct delayed_work 			irq_work;
	struct wake_lock wakelock; 
	struct mutex            lock;
	bool					ilmt_found;
	bool					charger_changed;
	unsigned int			polling_invl;
	bool					vbus_ovp;
	bool					vbus_ok;
	bool					vbus_uvp;
	bool					vsys_ovp;
	bool					vdc_pr;
	bool					cc_timeout;
	bool					wak_timeout;
	bool					chg_full;
	bool					in_cc_state;
	bool					in_cv_state;
	bool					in_wakeup_state;
	bool					initial_state;
	bluewhale_thermal_t		thermal_status;

	int32_t        chg_volt;
	int32_t        t34_cv;
	int32_t        t45_cv;
	int32_t        wakeup_volt;
	int32_t        eoc_current;
	int32_t        vbus_current;
	int32_t        rechg_hystersis;
	int32_t        charger_current;
	int32_t        wakeup_current;
	int32_t        safety_cc_timer;
	int32_t        safety_wk_timer;	
	bool present;
	bool online;
	bool is_charging_enabled;
	bool is_charger_enabled;
	enum power_supply_charger_cable_type cable_type;		
};

#define bluewhale_charger_info_t \
		struct bluewhale_charger_info




/*****************************************************************************
Rpull=76k \uff0cRdown=3.6k 
VBUS(V)		4.4v	5v		6v		7v		8v		9v		10V		11v		12v		13v		14v
BI/TS(V)	0.23	0.256	0.301	0.346	0.392	0.437	0.482	0.528	0.574	0.618	0.663
*****************************************************************************/
#define VBUS_BI_5V		5500
#define VBUS_BI_7V		7000
#define VBUS_BI_9V		7900
#define VBUS_BI_12V		11000

typedef enum {
	QUICK_CHG_ADAPTER_NULL = 0,
	QUICK_CHG_ADAPTER_PC,
	QUICK_CHG_ADAPTER_NORMAL,
	QUICK_CHG_ADAPTER_EC,
	QUICK_CHG_ADAPTER_MTK,
	QUICK_CHG_ADAPTER_QC,
	QUICK_CHG_ADAPTER_FIX,
} O2_CHARGER_TYPE;


int bluewhale_get_charging_status_extern(void);
void bluewhale_dump_register(void);

int bluewhale_set_vbus_current_extern(int ilmt_ma);
int bluewhale_get_vbus_current_extern(void);
int bluewhale_set_charger_current_extern(int chg_ma);
int bluewhale_set_eoc_current_extern(int eoc_ma);
int bluewhale_set_chg_volt_extern(int chgvolt_mv);

int bluewhale_init_extern(void);
int o2_stop_charge_extern(void);
int o2_start_ac_charge_extern(void);
int get_adjust_voltage_status(void);
int bluewhale_enable_otg_extern(int enable);



#endif
