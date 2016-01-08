/*
 * Copyright (C) 2010 MEMSIC, Inc.
 *
 * Initial Code:
 *	Robbie Cao
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/*
 * Definitions for mxc400x accelorometer sensor chip.
 */
#ifndef __MXC400X_H__
#define __MXC400X_H__


#include	<linux/ioctl.h>	/* For IOCTL macros */
#include	<linux/input.h>

#define	MXC400X_ACC_IOCTL_BASE 'a'
/** The following define the IOCTL command values via the ioctl macros */
#define	MXC400X_ACC_IOCTL_SET_DELAY		     _IOW(MXC400X_ACC_IOCTL_BASE, 0x00, int)
#define	MXC400X_ACC_IOCTL_GET_DELAY		     _IOR(MXC400X_ACC_IOCTL_BASE, 0x01, int)
#define	MXC400X_ACC_IOCTL_SET_ENABLE		 _IOW(MXC400X_ACC_IOCTL_BASE, 0x02, int)
#define	MXC400X_ACC_IOCTL_GET_ENABLE		 _IOR(MXC400X_ACC_IOCTL_BASE, 0x03, int)
#define	MXC400X_ACC_IOCTL_GET_TEMP		     _IOR(MXC400X_ACC_IOCTL_BASE, 0x04, int)
#define	MXC400X_ACC_IOCTL_GET_COOR_XYZ       _IOW(MXC400X_ACC_IOCTL_BASE, 0x22, int[3])
#define	MXC400X_ACC_IOCTL_GET_CHIP_ID        _IOR(MXC400X_ACC_IOCTL_BASE, 0x25, char[32])
#define MXC400X_ACC_IOCTL_SET_APARMS		_IOW(MXC400X_ACC_IOCTL_BASE, 0x20, int)

/************************************************/
/* 	Accelerometer defines section	 	*/
/************************************************/
#define MXC400X_ACC_DEV_NAME		"mxc400x"
#define MXC400X_ACC_INPUT_NAME		"mxc400x_accel" 
#define MXC400X_ACC_I2C_ADDR     	0x15
#define MXC400X_ACC_I2C_NAME        MXC400X_ACC_DEV_NAME
/*CHIP ID is 0x01*/
#define WHOAMI_MXC400X_ACC	0x02	
/*	chip id register	*/
#define WHO_AM_I		0x0E

/* MXC400X register address */
#define MXC400X_REG_FAC         0x0E
#define MXC400X_REG_FSRE        0x19
#define MXC400X_REG_CTRL		0x0D
#define MXC400X_REG_TEMP        0x09

#define MXC400X_PASSWORD        0x93
#define MXC400X_RANGE_8G        0x5B    /*full scale range +/-8g*/
#define MXC400X_CTRL_PWRON		0x40	/* power on */
#define MXC400X_CTRL_PWRDN		0x01	/* power donw */

#define MXC400X_RANGE_2G 		0x00

// x 0304 y  0506 z 0708
#define MXC400X_REG_X		0x03
#define MXC400X_REG_Y		0x05
#define MXC400X_REG_Z		0x07

#define MXC400X_DATA_LENGTH       6

/* #define I2C_BUS_NUM_STATIC_ALLOC */
#define I2C_STATIC_BUS_NUM        (3)	// Need to be modified according to actual setting
 
#ifdef	__KERNEL__
struct mxc400x_acc_platform_data {
	int poll_interval;
	int min_interval;

	u8 g_range;
	
	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;

	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);

	int gpio_int1;
	int gpio_int2;

};
#endif	/* __KERNEL__ */

#endif /* __MXC400X_H__ */
