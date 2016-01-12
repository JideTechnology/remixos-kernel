/*
 * Support for VM149C VCM.
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

#ifndef __VM149C_H__
#define __VM149C_H__

#include <linux/atomisp_platform.h>
#include <linux/types.h>


#define VM149C_VCM_ADDR	0x0c

#define VM149C_REG_RESET		0x01
#define VM149C_REG_MODE			0x02
#define VM149C_REG_VCM_MOVE_TIME	0x03
#define VM149C_REG_VCM_CODE_MSB		0x04
#define VM149C_REG_VCM_CODE_LSB		0x05
#define VM149C_REG_VCM_THRESHOLD_MSB	0x06
#define VM149C_REG_VCM_THRESHOLD_LSB	0x07

#define VM149C_REG_LENGTH		0x1

#define VM149C_RING_CTRL_ENABLE		0x04
#define VM149C_RING_CTRL_DISABLE	0x00

#define VM149C_RESONANCE_PERIOD		100000
#define VM149C_RESONANCE_COEF		512
#define VM149C_HIGH_FREQ_RANGE		0x80

#define VCM_CODE_MSB_MASK		0xfc

#define VM149C_DEF_TIME		0x60
#define VM149C_DEF_THRESHOLD	0xA0


enum vm149c_tok_type {
	VM149C_8BIT  = 0x1,
	VM149C_16BIT = 0x2,
};

enum vm149c_vcm_mode {
	VM149C_ARC_RES0 = 0x0,	/* Actuator response control RES1 */
	VM149C_ARC_RES1 = 0x1,	/* Actuator response control RES0.5 */
	VM149C_ARC_RES2 = 0x2,	/* Actuator response control RES2 */
	VM149C_ESRC = 0x3,	/* Enhanced slew rate control */
	VM149C_DIRECT = 0x4,	/* Direct control */
};

/* vm149c device structure */
struct vm149c_device {
	struct timespec timestamp_t_focus_abs;
	enum vm149c_vcm_mode vcm_mode;
	s16 number_of_steps;
	bool initialized;		/* true if vm149c is detected */
	s32 focus;			/* Current focus value */
	struct timespec focus_time;	/* Time when focus was last time set */
	__u8 buffer[4];			/* Used for i2c transactions */
	const struct camera_af_platform_data *platform_data;
};

#define VM149C_INVALID_CONFIG	0xffffffff
#define VM149C_MAX_FOCUS_POS	1023


#define DELAY_PER_STEP_NS	1000000
#define DELAY_MAX_PER_STEP_NS	(1000000 * 1023)

int vm149c_vcm_power_up(struct v4l2_subdev *sd);
int vm149c_vcm_power_down(struct v4l2_subdev *sd);
int vm149c_vcm_init(struct v4l2_subdev *sd);

int vm149c_t_focus_vcm(struct v4l2_subdev *sd, u16 val);
int vm149c_t_focus_abs(struct v4l2_subdev *sd, s32 value);
int vm149c_t_focus_rel(struct v4l2_subdev *sd, s32 value);
int vm149c_q_focus_status(struct v4l2_subdev *sd, s32 *value);
int vm149c_q_focus_abs(struct v4l2_subdev *sd, s32 *value);

#endif
