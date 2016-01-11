/* ========================================================================== */
/*                                                                            */
/*   sm5414_charger.h                                                      */
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

/* Control Register */
#define REG_INT1        0x00
#define REG_INT2        0x01
#define REG_INT3        0x02
#define REG_INTMSK1     0x03
#define REG_INTMSK2     0x04
#define REG_INTMSK3     0x05
#define REG_STATUS      0x06
#define REG_CTRL        0x07
#define REG_VBUSCTRL    0x08
#define REG_CHGCTRL1    0x09
#define REG_CHGCTRL2    0x0a
#define REG_CHGCTRL3    0x0b
#define REG_CHGCTRL4    0x0c
#define REG_CHGCTRL5    0x0d

/* REG_INT1 and REG_INT1_MASK */
#define VBUSOVP_INT1_SHIFT       7
#define VBUSUVLO_INT1_SHIFT      6
#define VBUSINOK_INT1_SHIFT      5
#define AICL_INT1_SHIFT          4
#define VBUSLIMIT_INT1_SHIFT     3
#define BATOVP_INT1_SHIFT        2
#define THEMSHDN_INT1_SHIFT      1
#define THEMREG_INT1_SHIFT       0
#define INT1_MASK_ALL            0xff

/* REG_INT2 and REG_INT2_MASK */
#define FASTTMROFF_INT2_SHIFT    7
#define NOBAT_INT2_SHIFT         6
#define WEAKBAT_INT2_SHIFT       5
#define OTGFAIL_INT2_SHIFT       4
#define PRETMROFF_INT2_SHIFT     3
#define CHGRSTF_INT2_SHIFT       2
#define DONE_INT2_SHIFT          1
#define TOPOFF_INT2_SHIFT        0
#define INT2_MASK_ALL            0xff

/* REG_INT3 and REG_INT3_MASK */
#define VSYSOK_INT3_SHIFT        3
#define VSYSNG_INT3_SHIFT        2
#define VSYSOLP_INT3_SHIFT       1
#define DISLIMI_INT3_SHIFT       0
#define INT3_MASK_ALL            0xff

/* REG_STATUS */
#define S_VBUSOVP_SHIFT          7
#define S_VBUSUVLO_SHIFT         6
#define S_TOPOFF_SHIFT           5
#define S_VSYSOLP_SHIFT          4
#define S_DISLIMIT_SHIFT         3
#define S_THEMSHDN_SHIFT         2
#define S_BATDET_SHIFT           1
#define S_SUSPEND_SHIFT          0
#define S_ST_MASK_ALL            0xff

/* REG_CTRL */
#define ENCOMPARATOR_SHIFT       6
#define ENCOMPARATOR_MASK        0x1

#define RESET_SHIFT              3
#define RESET_MASK               0x1

#define SUSPEN_SHIFT             2
#define SUSPEN_MASK              0x1

#define CHGEN_SHIFT              1
#define CHGEN_MASK               0x1

#define ENBOOST_SHIFT            0
#define ENBOOST_MASK             0x1

/* REG_VBUSCTRL */
#define VBUSLIMIT_SHIFT          0
#define VBUSLIMIT_MASK           0x3F

/* REG_CHGCTRL1 */
#define AICLTH_SHIFT             4
#define AICLTH_MASK              0x7

#define AUTOSTOP_SHIFT           3
#define AUTOSTOP_MASK            0x1

#define AICLEN_SHIFT             2
#define AICLEN_MASK              0x1

#define PRECHG_SHIFT             0
#define PRECHG_MASK              0x3

/* REG_CHGCTRL2 */
#define FASTCHG_SHIFT            0
#define FASTCHG_MASK             0x3F

/* REG_CHGCTRL3 */
#define BATREG_SHIFT             4
#define BATREG_MASK              0xF

#define WEAKBAT_SHIFT            0
#define WEAKBAT_MASK             0xF

/* REG_CHGCTRL4 */
#define TOPOFF_SHIFT             3
#define TOPOFF_MASK              0xF

#define DISLIMIT_SHIFT           0
#define DISLIMIT_MASK            0x7

/* REG_CHGCTRL5 */
#define VOTG_SHIFT               4
#define VOTG_MASK                0x3

#define FASTTIMER_SHIFT          2
#define FASTTIMER_MASK           0x3

#define TOPOFFTIMER_SHIFT        0
#define TOPOFFTIMER_MASK         0x3

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
#define FASTCHG_MAX_MA           2500

/* BATREG  */
#define BATREG_MIN_MV            4100
#define BATREG_STEP_MV           25
#define BATREG_MAX_MV            4475

/* TOPPOFF  */
#define TOPPOFF_MIN_MA           100
#define TOPPOFF_STEP_MA          50
#define TOPPOFF_MAX_MA           650

/* VOTG */
#define VOTG_MIN_MV              5100
#define VOTG_STEP_MV             100
#define VOTG_MAX_MV              5200

struct sm5414_platform_data {
	int		max_charger_currentmA;
	int		max_charger_voltagemV;
	int		topoff_currentmA;
	int		vbus_limit_currentmA;
	char **supplied_to; 
};


struct sm5414_charger_info {
	struct i2c_client				*client;
	struct power_supply				usb;
	struct sm5414_platform_data 	*pdata;
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
	enum power_supply_charger_cable_type cable_type; 
};

#define sm5414_charger_info_t \
		struct sm5414_charger_info

typedef enum {
	QUICK_CHG_ADAPTER_NULL = 0,
	QUICK_CHG_ADAPTER_PC,
	QUICK_CHG_ADAPTER_NORMAL,
	QUICK_CHG_ADAPTER_EC,
	QUICK_CHG_ADAPTER_MTK,
	QUICK_CHG_ADAPTER_QC,
} SM5414_CHARGER_TYPE;


int32_t sm5414_read_byte(uint8_t index, uint8_t *dat);
int32_t sm5414_write_byte(uint8_t index, uint8_t dat);
int sm5414_set_vbus_limit_current(int ilmt_ma);
int sm5414_get_vbus_current(void);
int sm5414_set_fastchg_current(int chg_ma);
extern  int sm5414_init(void);;
extern int sm5414_get_charging_status(struct sm5414_charger_info *charger);
extern void sm5414_stop_charge(void);
int sm5414_set_topoff_current(int topoff_ma);

#endif

