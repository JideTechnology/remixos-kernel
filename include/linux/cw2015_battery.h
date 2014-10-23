/*
 * Copyright (C) 2009 Samsung Electronics
 * Copyright (C) 2012 Nvidia Cooperation
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CW2015_BATTERY_H_
#define __CW2015_BATTERY_H_

#define SIZE_BATINFO    64 
#define WARK_UP_FLAG    1
struct cw2015_platform_data {
	int use_ac;
	int use_usb;
	int dc_det_pin;
	int batt_low_pin;
	int dc_det_level;
	uint8_t *data_tbl;
	int enable_screen_on_gpio;
	unsigned int active_state:1;
};

void cw2015_battery_status(int status, int chrg_type);
int cw2015_check_battery(void);
#endif
