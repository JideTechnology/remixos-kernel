/*
* Copyright (C) 2014 Dyna Image.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef __DI_SENSOR_H_
#define __DI_SENSOR_H_

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/ioctl.h>
#endif

#define DI3220_I2C_ADDR		(0x1E)
#define DI3132_I2C_ADDR		(0x4C)
#define DI3133_I2C_ADDR		(0x4C)

struct sensor_platform_data 
{
	int poll_interval;

	s8 orientation[9];
};

#endif	/* __DI_SENSOR_H_ */
