/*
 * drivers/battery/sm5702_fuelgauge.h
 *
 * Header of Richtek sm5702 Fuelgauge Driver
 *
 * Copyright (C) 2013 SiliconMitus
 * Author: SW Jung
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef sm5702_FUELGAUGE_H
#define sm5702_FUELGAUGE_H

#include <linux/i2c.h>
/*#include <linux/mfd/sm5702.h>*/
//#include <linux/idi/idi_interface.h>
//#include <linux/idi/idi_controller.h>
//#include <linux/idi/idi_ids.h>
//#include <linux/idi/idi_bus.h>
//#include <linux/idi/idi_device_pm.h>

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif /* #ifdef CONFIG_DEBUG_FS */

#define FG_DRIVER_VER "0.0.0.1"
#define SM5702_DELAY		10000
#define MINVAL(a, b) ((a <= b) ? a : b)

enum battery_table_type {
	DISCHARGE_TABLE = 0,
	Q_TABLE,
	TABLE_MAX,
};


struct battery_data_t {
	const int battery_type; /* 3000 or 4350 or 4400*/
	const int battery_table[2][15];
	const int rce_value[3];
	const int dtcd_value;
};

struct sm5702_fg_info {
	/* State Of Connect */
	int online;
	/* battery SOC (capacity) */
	int batt_soc;
	/* battery voltage */
	int batt_voltage;
	/* battery AvgVoltage */
	int batt_avgvoltage;
	/* battery OCV */
	int batt_ocv;
	/* Current */
	int batt_current;
	/* battery Avg Current */
	int batt_avgcurrent;

	struct battery_data_t *comp_pdata;

	struct mutex param_lock;
	/* copy from platform data /
	 * DTS or update by shell script */

	struct mutex io_lock;
	struct device *dev;
	int32_t temperature;; /* 0.1 deg C*/
	/* register programming */
	int reg_addr;
	u8 reg_data[2];

	int battery_table[2][15];
	int rce_value[3];
	int dtcd_value;
	int rs_value[4]; /*rs mix_factor max min*/
	int vit_period;
	int mix_value[2]; /*mix_rate init_blank*/

	int enable_topoff_soc;
	int topoff_soc;

	int volt_cal;
	int curr_cal;

	int temp_std;
	int temp_offset;
	int temp_offset_cal;
	int charge_offset_cal;

	int battery_type; /* 4200 or 4350 or 4400*/
	uint32_t soc_alert_flag:1;/* 0 : nu-occur, 1: occur */
	uint32_t volt_alert_flag:1;/* 0 : nu-occur, 1: occur */
	uint32_t flag_full_charge:1;/* 0 : no , 1 : yes*/
	uint32_t flag_chg_status:1;/* 0 : discharging, 1: charging*/

	int id;
	int p_batt_current;
	int iocv_error_count;
	int p_batt_voltage;
	int32_t irq_ctrl;

    /*low voltage alarm mV level*/
    int alarm_vol_mv;
};

struct sm5702_fuelgauge_info {
	struct device 			*dev;
	struct i2c_client		*client;
	/*sm5702_fg_platform_data_t *pdata;*/
	struct power_supply		psy_fg;
	struct delayed_work isr_work;

	int cable_type;
	bool is_charging;

	/* HW-dedicated fuel guage info structure
	 * used in individual fuel gauge file only
	 * (ex. dummy_fuelgauge.c)
	 */
	struct sm5702_fg_info	info;

	bool is_fuel_alerted;
	/*struct wake_lock fuel_alert_wake_lock;*/

	unsigned int capacity_old;	/* only for atomic calculation */
	unsigned int capacity_max;	/* only for dynamic calculation */

	bool initial_update_of_soc;
	struct mutex fg_lock;

	/* register programming */
	int reg_addr;
	u8 reg_data[2];
};


bool sm5702_fg_full_charged(struct i2c_client *client);

#endif /* sm5702_FUELGAUGE_H*/
