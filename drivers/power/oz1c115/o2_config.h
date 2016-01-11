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

#ifndef	__O2_CONFIG_H
#define	__O2_CONFIG_H __FILE__


#define CHARGER_I2C_NUM    2
#define OZ88106_I2C_NUM    2

#define	EXT_THERMAL_READ   1
#define ANDROID_5_XX	 
		

#define IR_COMPENSATION		0
#define IR_COMPENSATION_MAX_VOLTAGE     4550

#define O2_CONFIG_CV_VOLTAGE  		4200
#define O2_CONFIG_CAPACITY  		5158
#define O2_CONFIG_RSENSE  			10
#define O2_CONFIG_EOC  				200
#define O2_CONFIG_EOD 				3400
#define O2_CONFIG_BOARD_OFFSET		20


#define TARGET_QUICK_CHG_ILIMIT	    1500
#define TARGET_QUICK_CHG_CURRENT	2000

#define TARGET_NORMAL_CHG_ILIMIT	1500
#define TARGET_NORMAL_CHG_CURRENT	2000

#define STOP_CHG_ILIMIT				1000
#define STOP_CHG_CURRENT			1000



#define O2_CHARGER_EOC				80

#define O2_CHARGER_INPEDANCE		80

#define BATTERY_WORK_INTERVAL		10

#define RPULL (76000)
#define RDOWN (5100)


//#define TEMPERATURE_CHECK


#ifdef  TEMPERATURE_CHECK
#define BAT_HOT_STOP_CC (60)
#define BAT_COLD_STOP_CC (0)
#define BAT_HOT_RECOVERY (BAT_HOT_STOP_CC - 3)
#define BAT_COLD_RECOVERY (BAT_COLD_STOP_CC + 3)

#define BAT_WARM_LOWER_CC_STEP (2)
#define BAT_WARM_LOWER_CC (45)
#define BAT_COOL_LOWER_CC (10)
#define BAT_COOL_CC_CURRENT (600)

#define BAT_WARM_LOWER_CV (50)
#define BAT_COOL_LOWER_CV (-10)

#define BAT_WARM_CV_VOLTAGE (O2_CONFIG_CV_VOLTAGE - 100)
#define BAT_COOL_CV_VOLTAGE (O2_CONFIG_CV_VOLTAGE - 100)

#endif


          


//#define MTK_MACH_MTKADAPTER_O2GAUGE_O2CHARGER
//#define MTK_MACH_O2CHARGER_FIX_ADAPTER
//#define TQ210_MACH_MTK_ADAPTER_O2GAUGE_O2CHARGER
//#define QUALCOMM_MACH_SUPPORT
//#define MTK_MACH_O2CHARGER_MTKADAPTER
#define INTEL_MACH






// ****************************TQ210 TEST*************************


// ****************************MTK MACH + MTK ADAPTER + O2 CHARGER + O2 GAUGE*************************
#if defined MTK_MACH_MTKADAPTER_O2GAUGE_O2CHARGER
	#define MTK_MACH_SUPPORT
//	#define MTK_ADAPTER_SUPPORT
	#define O2_GAUGE_SUPPORT
	#define O2_CHARGER_SUPPORT
	#define VBUS_COLLECT_BY_O2
	#define ENABLE_10MIN_END_CHARGE_FUN
	
	//#define VBUS_COLLECT_BY_MTK


#elif defined MTK_MACH_FIXADAPTER_O2GAUGE_O2CHARGER

	#define MTK_MACH_SUPPORT
	#define O2_GAUGE_SUPPORT
	#define O2_CHARGER_SUPPORT
	#define FIX_ADPATER_ILIMIT
	


#elif defined INTEL_MACH
	#define O2_GAUGE_SUPPORT
	#define O2_CHARGER_SUPPORT
	#define VBUS_COLLECT_BY_O2
	#define O2_GAUGE_WORK_SUPPORT
	#define O2_GAUGE_POWER_SUPPOLY_SUPPORT
	#define O2_CHARGER_POWER_SUPPOLY_SUPPORT

#elif defined TQ210_MACH_MTK_ADAPTER_O2GAUGE_O2CHARGER

	#define TQ210_TEST
//	#define MTK_ADAPTER_SUPPORT
	//#define EC_ADAPTER_SUPPORT
	//#define QC_ADAPTER_SUPPORT
	#define O2_GAUGE_SUPPORT
	#define O2_CHARGER_SUPPORT
	#define VBUS_COLLECT_BY_O2
	#define O2_GAUGE_WORK_SUPPORT
	#define O2_GAUGE_POWER_SUPPOLY_SUPPORT
	#define O2_CHARGER_POWER_SUPPOLY_SUPPORT

	//#define TRPULLEMPERATURE_CV_CHECK
  
	//#define FIX_ADPATER_ILIMIT
	
	
// ****************************MTK MACH  + O2 GAUGE*************************
#elif defined MTK_MACH_O2GAUGE
	#define MTK_MACH_SUPPORT
	#define O2_GAUGE_SUPPORT

// ****************************MTK MACH  + O2 CHARGER + MTK Adapter*************************
#elif defined MTK_MACH_O2CHARGER_MTKADAPTER
	#define MTK_MACH_SUPPORT
//	#define MTK_ADAPTER_SUPPORT
	#define O2_CHARGER_SUPPORT
	#define VBUS_COLLECT_BY_MTK


// ****************************MTK MACH + O2 CHARGER + fix adapter *************************
#elif defined MTK_MACH_O2CHARGER_FIX_ADAPTER
	#define MTK_MACH_SUPPORT
	
	#define O2_CHARGER_SUPPORT
	#define FIX_ADPATER_ILIMIT
	//#define VBUS_COLLECT_BY_O2




#elif defined OTHER_MACH_MTK_ADAPTER_O2GAUGE_O2CHARGER




#elif defined OTHER_MACH_O2GAUGE_O2CHARGER



#elif defined OTHER_MACH_CHARGER

	
// ****************************default*************************
#else
	#define TQ210_TEST
//	#define MTK_ADAPTER_SUPPORT
	#define O2_GAUGE_SUPPORT
	#define O2_CHARGER_SUPPORT
	#define VBUS_COLLECT_BY_O2

#endif





#ifdef TQ210_TEST
	#define CHG_ADJUST_RETRY_TIMES		10
	#define CHARGER_CHECK_ADAPTER_INTERVAL		5
	#define START_QUICK_CHARGE_VOLTAGE     3500
#else
	#define CHG_ADJUST_RETRY_TIMES		10
	#define CHARGER_CHECK_ADAPTER_INTERVAL		40
	#define START_QUICK_CHARGE_VOLTAGE     3700
#endif


#ifdef MTK_MACH_SUPPORT
#include <mach/battery_common.h>
#endif

#ifdef MTK_ADAPTER_SUPPORT
#include "o2_mtk_adapter.h"
#endif

#ifdef EC_ADAPTER_SUPPORT
#include "o2_ec_adapter.h"
#endif

#ifdef O2_GAUGE_SUPPORT
#include "parameter.h"
#endif

#ifdef O2_CHARGER_SUPPORT
#include "bluewhale_charger.h"
#endif

#endif
