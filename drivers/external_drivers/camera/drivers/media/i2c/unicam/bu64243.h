/*
 * Support for BU64243 VCM.
 *
 * Copyright (c) 2013 Intel Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef __BU64243_H__
#define __BU64243_H__

#include <linux/atomisp_platform.h>
#include <linux/types.h>


#define BU64243_VCM_ADDR	0x0c

enum bu64243_tok_type {
	BU64243_8BIT  = 0x1,
	BU64243_16BIT = 0x2,
};

enum bu64243_vcm_mode {
	BU64243_ARC_RES0 = 0x0,	/* Actuator response control RES1 */
	BU64243_ARC_RES1 = 0x1,	/* Actuator response control RES0.5 */
	BU64243_ARC_RES2 = 0x2,	/* Actuator response control RES2 */
	BU64243_ESRC = 0x3,	/* Enhanced slew rate control */
	BU64243_DIRECT = 0x4,	/* Direct control */
};

#define to_bu64243_device(x) container_of(x, struct bu64243_device, sd)

#define BU64243_W0_W2(a) ((a & 0x07) << 3)
#define BU64243_PS(a) (a << 7)
#define BU64243_EN(a) (a << 6)
#define BU64243_M(a) (a << 2)
#define BU64243_D_HI(a) ((a >> 8) & 0x03)
#define BU64243_D_LO(a) (a & 0xff)
#define BU64243_RFEQ(a) ((a & 0x1f) << 3)
#define BU64243_SRATE(a) (a & 0x03)
#define BU64243_STIME(a) (a & 0x1f)
#define BU64243_SRES(a) ((a & 0x07) << 5)
#define BU64243_I2C_ADDR				0x0C
#define BU64243_VCM_CURRENT			0	/* point C */
#define BU64243_PARAM_1				1	/* freq and slew rate */
#define BU64243_PARAM_2				2	/* point A */
#define BU64243_PARAM_3				3   /* point B */
#define BU64243_PARAM_4				4	/*step res and step time*/



#define BU64243_DEFAULT_POINT_A				0x2a	/* 42	*/
#define BU64243_DEFAULT_POINT_B				0x52	/* 82 */
#define BU64243_DEFAULT_POINT_C				0xCB	/* 203*/
#define BU64243_DEFAULT_VCM_FREQ			0x08	/* 85HZ */
#define BU64243_DEFAULT_SLEW_RATE			0x03	/* fastest */
#define BU64243_DEFAULT_RES_SETTING			0x04	/* 4LSB */
#define BU64243_DEFAULT_STEP_TIME_SETTING	0x04	/* 200us */
#define BU64243_DEFAULT_OUTPUT_STATUS		0x01	/* 0 um	*/
#define BU64243_DEFAULT_ISRC_MODE			0x01	/* 0 um	*/

#define INTEL_FOCUS_OFFSET 20
#define VCM_ORIENTATION_OFFSET 100
#define POINT_AB_OFFSET 40

#define BU64243_MAX_FOCUS_POS 1023


/* bu64243 device structure */
struct bu64243_device {
	struct timespec timestamp_t_focus_abs;
	enum bu64243_vcm_mode vcm_mode;
	s16 number_of_steps;
	bool initialized;		/* true if bu64243 is detected */
	s32 focus;			/* Current focus value */
	struct timespec focus_time;	/* Time when focus was last time set */
	__u8 buffer[4];			/* Used for i2c transactions */
	const struct camera_af_platform_data *platform_data;
};

#define BU64243_INVALID_CONFIG	0xffffffff
#define BU64243_MAX_FOCUS_POS	1023


#define DELAY_PER_STEP_NS	1000000
#define DELAY_MAX_PER_STEP_NS	(1000000 * 1023)

int bu64243_vcm_power_up(struct v4l2_subdev *sd);
int bu64243_vcm_power_down(struct v4l2_subdev *sd);
int bu64243_vcm_init(struct v4l2_subdev *sd);

int bu64243_t_focus_vcm(struct v4l2_subdev *sd, u16 val);
int bu64243_t_focus_abs(struct v4l2_subdev *sd, s32 value);
int bu64243_t_focus_rel(struct v4l2_subdev *sd, s32 value);
int bu64243_q_focus_status(struct v4l2_subdev *sd, s32 *value);
int bu64243_q_focus_abs(struct v4l2_subdev *sd, s32 *value);

#endif
