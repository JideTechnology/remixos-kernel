/*
 * drivers/battery/sm5702_fuelgauge_init_data.h
 *
 * Header of Richtek sm5702 Fuelgauge Driver Implementation
 *
 * Copyright (C) 2013 SiliconMitus
 * Author: SW Jung
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef sm5702_FUELGAUGE_INIT_DATA_H
#define sm5702_FUELGAUGE_INIT_DATA_H

#include <linux/power/sm5702_fuelgauge.h>

static int battery_table_para[][15] = {\
		{0x2800, 0x34CC, 0x3889, 0x3A56, 0x3AE0, 0x3B09, 0x3B8B, 0x3C37, 0x3CD0, 0x3D70, 0x3E1F, 0x3E92, 0x3FFB, 0x422B, 0x4666},\
		{0x0, 0x9E, 0x212, 0x3CA, 0x582, 0x8F3, 0xC63, 0x1344, 0x2107, 0x27E8, 0x2B58, 0x2EC9, 0x391A, 0x436C, 0x4407},\
};

static struct sm5702_fg_info sm5702_fuelgauge_data = {
	.id = 0,		
	.battery_type = 4200,		/* 4.200V 4.350V 4.400V */	
	//.battery_table = battery_table_para,
	.temp_std = 25,	   
	.temp_offset = 10,	   
	.temp_offset_cal = 0x01,	
	.charge_offset_cal = 0x00,		 
	.rce_value[0] = 0x1406,		/*rce value*/
	.rce_value[1] = 0x4e8,
	.rce_value[2] = 0x79,
	.dtcd_value = 0x4,		 
	.rs_value[0] = 0x6b8,		/*rs mix_factor max min*/  
	.rs_value[1] = 0x0300,
	.rs_value[2] = 0xFFFF,
	.rs_value[3] = 0x0300, 
	.vit_period = 0xD406,
	.mix_value[0] = 0x0043,		/*mix_rate init_blank*/	  
	.mix_value[1] = 0x0008,
	.curr_cal = 0x80,
	.volt_cal = 0x0,
	.alarm_vol_mv = 3400,
};
#endif /* sm5702_FUELGAUGE_INIT_DATA_H */
