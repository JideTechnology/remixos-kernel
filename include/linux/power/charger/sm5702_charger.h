/* ========================================================================== */
/*                                                                            */
/*   sm5702_charger.h                                                      */
/*   (c) 2015 Author                                                          */
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

#ifndef	__SILICONMITUS_CHARGER_H
#define	__SILICONMITUS_CHARGER_H __FILE__

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

#define SM5702_CHGLED12OP		    0x03
#define SM5702_CHGLED12OP_MASK		0xC0
#define SM5702_CHGLED12OP_SHIFT		0x06

#define SM5702_CHGLED12MODE		    0x01
#define SM5702_CHGLED12MODE_MASK	0x20
#define SM5702_CHGLED12MODE_SHIFT	0x05

#define SM5702_AUTOSET              0x01
#define SM5702_AUTOSET_MASK         0x10
#define SM5702_AUTOSET_SHIFT        0x04

#define SM5702_RESET                0x01
#define SM5702_RESET_MASK           0x08
#define SM5702_RESET_SHIFT          0x03

#define SM5702_OPERATION_MODE				        0x07
#define SM5702_OPERATION_MODE_MASK                  0x07
#define SM5702_OPERATION_MODE_SHIFT			        0X00

#define SM5702_OPERATION_MODE_SUSPEND               0x00
#define SM5702_OPERATION_MODE_CHARGING_OFF          0x04//100
#define SM5702_OPERATION_MODE_CHARGING_ON           0x05//101
#define SM5702_OPERATION_MODE_FLASH_BOOST_MODE      0x06//110
#define SM5702_OPERATION_MODE_USB_OTG_MODE          0x07//111

#define SM5702_VBUSLIMIT            0x3F
#define SM5702_VBUSLIMIT_MASK       0x3F
#define SM5702_VBUSLIMIT_SHIFT      0x00

#define SM5702_ENCOMPARATOR         0x01
#define SM5702_ENCOMPARATOR_MASK    0x80
#define SM5702_ENCOMPARATOR_SHIFT   0x07

#define SM5702_AICLTH                 0x07
#define SM5702_AICLTH_MASK            0x70
#define SM5702_AICLTH_SHIFT           0x04

#define SM5702_AUTOSTOP             0x01
#define SM5702_AUTOSTOP_MASK        0x08
#define SM5702_AUTOSTOP_SHIFT       0x03

#define SM5702_AICLEN               0x01
#define SM5702_AICLEN_MASK          0x04
#define SM5702_AICLEN_SHIFT         0x02

#define SM5702_PRECHG               0x01
#define SM5702_PRECHG_MASK          0x03
#define SM5702_PRECHG_SHIFT         0x00

#define SM5702_FASTCHG               0x3F
#define SM5702_FASTCHG_MASK          0x3F
#define SM5702_FASTCHG_SHIFT         0x00

#define SM5702_BATREG               0x1F
#define SM5702_BATREG_MASK          0x1F
#define SM5702_BATREG_SHIFT         0x00

#define SM5702_TOPOFF               0x0F
#define SM5702_TOPOFF_MASK          0x78
#define SM5702_TOPOFF_SHIFT         0x03

#define SM5702_DISLIMIT             0x07
#define SM5702_DISLIMIT_MASK        0x07
#define SM5702_DISLIMIT_SHIFT       0x00

#define SM5702_BSTOUT       		0x0F
#define SM5702_BSTOUT_MASK       	0x0F
#define SM5702_BSTOUT_SHIFT			0x00

/* VBUSLIMIT */
#define VBUSLIMIT_MIN_MA         100
#define VBUSLIMIT_STEP_MA        50
#define VBUSLIMIT_MAX_MA         2050

/* PRECHG */
#define PRECHG_MIN_MA            150
#define PRECHG_STEP_MA           100
#define PRECHG_MAX_MA            450

/* FASTCHG */
#define FASTCHG_MIN_MA           100
#define FASTCHG_STEP_MA          50
#define FASTCHG_MAX_MA           2000

/* BATREG  */
#define BATREG_MIN_MV            3700
#define BATREG_STEP_MV           25
#define BATREG_MAX_MV            4475

/* TOPPOFF  */
#define TOPPOFF_MIN_MA           100
#define TOPPOFF_STEP_MA          25
#define TOPPOFF_MAX_MA           475

/* BSTOUT */
#define BSTOUT_MIN_MV              4000
#define BSTOUT_STEP_MV             100
#define BSTOUT_MAX_MV              5100

struct sm5702_platform_data {
	int		max_charger_currentmA;
	int		max_charger_voltagemV;
	int		topoff_currentmA;
	int		vbus_limit_currentmA;
	char **supplied_to; 
};


struct sm5702_charger_info {
	struct device			*dev;
	struct i2c_client				*client;
	struct power_supply				usb;
	struct sm5702_platform_data 	*pdata;
	struct delayed_work 			delay_work;
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
	bool					chg_full;

	int32_t        chg_volt;
	int32_t        eoc_current;
	int32_t        vbus_current;
	int32_t        charger_current;
	bool present;
	bool online;
	bool is_charging_enabled;
	bool is_charger_enabled;
	int irq_base;
	enum power_supply_charger_cable_type cable_type; 
};

#define sm5702_charger_info_t \
		struct sm5702_charger_info

typedef enum {
	QUICK_CHG_ADAPTER_NULL = 0,
	QUICK_CHG_ADAPTER_PC,
	QUICK_CHG_ADAPTER_NORMAL,
	QUICK_CHG_ADAPTER_EC,
	QUICK_CHG_ADAPTER_MTK,
	QUICK_CHG_ADAPTER_QC,
} SM5702_CHARGER_TYPE;


int sm5702_set_vbus_limit_current(int ilmt_ma);
int sm5702_get_vbus_current(void);
int sm5702_set_fastchg_current(int chg_ma);
extern  int sm5702_init(void);;
extern int sm5702_get_charging_status(struct sm5702_charger_info *charger);
extern void sm5702_stop_charge(void);
int sm5702_set_topoff_current(int topoff_ma);

#endif

