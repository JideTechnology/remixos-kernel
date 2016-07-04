/*
 * Support for gc0310 Camera Sensor.
 *
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
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
 */

#ifndef __GC0310_H__
#define __GC0310_H__

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/spinlock.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-device.h>
#ifndef CONFIG_GMIN_INTEL_MID
#include <media/v4l2-chip-ident.h>
#endif
#include <linux/v4l2-mediabus.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/media-entity.h>
#include <linux/atomisp_platform.h>
#include <linux/atomisp.h>

#define GC0310_NAME	"gc0310"
#define V4L2_IDENT_GC0310 1111
#define	LAST_REG_SETING	{0xffff, 0xff}

#define GC0310_XVCLK	1920
#define GC0310_EXPOSURE_DEFAULT_VAL 33 /* 33ms*/

#define GC0310_FINE_INTG_TIME_MIN 0
#define GC0310_FINE_INTG_TIME_MAX_MARGIN 0
#define GC0310_COARSE_INTG_TIME_MIN 1
#define GC0310_COARSE_INTG_TIME_MAX_MARGIN 6

#define GC0310_FOCAL_LENGTH_NUM	270	/*2.70mm*/
#define GC0310_FOCAL_LENGTH_DEM	100
#define GC0310_F_NUMBER_DEFAULT_NUM	26
#define GC0310_F_NUMBER_DEM	10
#define GC0310_F_NUMBER_DEFAULT 0x16000a

/* #defines for register writes and register array processing */

enum gc0310_tok_type {
	GC0310_8BIT  = 0x0001,
	GC0310_TOK_TERM   = 0xf000,	/* terminating token for reg list */
	GC0310_TOK_DELAY  = 0xfe00,	/* delay token for reg list */
	GC0310_TOK_MASK = 0xfff0
};

#define I2C_RETRY_COUNT		5
#define MSG_LEN_OFFSET		2


#define GC0310_IMG_ORIENTATION	0x17
#define GC0310_FOCAL_LENGTH_NUM	208	/*2.08mm*/
#define GC0310_FOCAL_LENGTH_DEM	100
#define GC0310_F_NUMBER_DEFAULT_NUM	24
#define GC0310_F_NUMBER_DEM	10
#define GC0310_REG_EXPO_COARSE	0x03
#define GC0310_REG_EXPO_DIV     0x25
#define GC0310_REG_MAX_AEC	0x3c
#define GC0310_REG_COLOR_EFFECT	0x83
#define GC0310_REG_BLOCK_ENABLE	0x42
#define GC0310_REG_COL_CODE	0x48
#define GC0310_REG_EXPOSURE_AUTO 0x4f

/*register */
#define GC0310_REG_V_START                     0x0a    //  06
#define GC0310_REG_H_START                     0x0c   //  08
#define GC0310_REG_WIDTH_H                      0x0f //  0b
#define GC0310_REG_WIDTH_L                      0x10   //0c
#define GC0310_REG_HEIGHT_H                      0x0d   //  09
#define GC0310_REG_HEIGHT_L                      0x0e   // 0a
#define GC0310_REG_SH_DELAY                     0x11    //12 
#define GC0310_REG_DUMMY_H                     0x05    //  0f
#define GC0310_REG_H_DUMMY_L                     0x06   // 01
#define GC0310_REG_V_DUMMY_H                     0x07
#define GC0310_REG_V_DUMMY_L                     0x08  //  02 
#define GC0310_REG_EXPO_COARSE                 0x03   //  03
#define GC0310_REG_GLOBAL_GAIN                 0x70   // 50 
#define GC0310_REG_GAIN                        0x71   //51


/*value */
#define GC0310_FRAME_START	0x01
#define GC0310_FRAME_STOP	0x00
#define GC0310_AWB_GAIN_AUTO	0
#define GC0310_AWB_GAIN_MANUAL	1
#define GC0310_EXPOSURE_MANUAL_MASK 0x01

#define GC0310_VALUE_V_START                     0
#define GC0310_VALUE_H_START                     0
#define GC0310_VALUE_V_END                       480
#define GC0310_VALUE_H_END                       640
#define GC0310_VALUE_V_OUTPUT                       480
#define GC0310_VALUE_H_OUTPUT                       640
#define GC0310_VALUE_PIXEL_CLK                   96

#define MIN_SYSCLK		10
#define MIN_VTS			8
#define MIN_HTS			8
#define MIN_SHUTTER		0
#define MIN_GAIN		0

/* GC0310_DEVICE_ID */
#define GC0310_MOD_ID		0xa310

#define GC0310_RES_VGA_SIZE_H		640
#define GC0310_RES_VGA_SIZE_V		480

/*
 * struct gc0310_reg - MI sensor  register format
 * @length: length of the register
 * @reg: 16-bit offset to register
 * @val: 8/16/32-bit register value
 * Define a structure for sensor register initialization values
 */
struct gc0310_reg {
	u16 length;
	u16 reg;
	u32 val;	/* value or for read/mod/write */
};

struct regval_list {
	u16 reg_num;
	u8 value;
};

struct gc0310_device {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt format;
	struct mutex input_lock;
	struct firmware *firmware;

	struct camera_sensor_platform_data *platform_data;
	int run_mode;
	int focus_mode;
	int night_mode;
	int color_effect;
	bool streaming;
	bool preview_ag_ae;
	u16 sensor_id;
	u8 sensor_revision;
	u8 hot_pixel;
	unsigned int ae_high;
	unsigned int ae_low;
	unsigned int preview_shutter;
	unsigned int preview_gain16;
	unsigned int average;
	unsigned int preview_sysclk;
	unsigned int preview_hts;
	unsigned int preview_vts;
	unsigned int fmt_idx;
	unsigned int ae_lock;
	unsigned int wb_mode;
	struct v4l2_ctrl_handler ctrl_handler;
	/* Test pattern control */
	struct v4l2_ctrl *tp_mode;
};

struct gc0310_priv_data {
	u32 port;
	u32 num_of_lane;
	u32 input_format;
	u32 raw_bayer_order;
};

struct gc0310_format_struct {
	u8 *desc;
	u32 pixelformat;
	struct regval_list *regs;
};

struct gc0310_res_struct {
	u8 *desc;
	int res;
	int width;
	int height;
	u16 pixels_per_line;
	u16 lines_per_frame;
	int fps;
	int pix_clk;
	int skip_frames;
	int lanes;
	u8 bin_mode;
	u8 bin_factor_x;
	u8 bin_factor_y;
	bool used;
	struct regval_list *regs;
};

#define GC0310_MAX_WRITE_BUF_SIZE	32
struct gc0310_write_buffer {
	u16 addr;
	u8 data[GC0310_MAX_WRITE_BUF_SIZE];
};

struct gc0310_write_ctrl {
	int index;
	struct gc0310_write_buffer buffer;
};

struct gc0310_control {
	struct v4l2_queryctrl qc;
	int (*query)(struct v4l2_subdev *sd, s32 *value);
	int (*tweak)(struct v4l2_subdev *sd, int value);
};

/* Supported resolutions */
enum {
	GC0310_RES_XCIF,
	GC0310_RES_SVGA,
	GC0310_RES_1M3,
	GC0310_RES_2M,
};

static struct gc0310_res_struct gc0310_res[] = {
	{
	.desc	= "VGA",
	.res	= GC0310_RES_XCIF,
	.width	= 640,
	.height	= 480,
	.pixels_per_line = 1190,
	.lines_per_frame = 605,
	.fps	= 30,
	.pix_clk = 14.4,
	.used	= 0,
	.regs	= NULL,
	.skip_frames = 3,
	.lanes = 1,
	.bin_mode = 1,
	.bin_factor_x = 1,
	},
};
#define N_RES (ARRAY_SIZE(gc0310_res))

static const struct i2c_device_id gc0310_id[] = {
	{"gc0310", 0},
	{}
};

static struct gc0310_reg const gc0310_stream_on[] = {
	{GC0310_8BIT, 0xfe, 0x30},	
	{GC0310_8BIT, 0xfe, 0x03},	
	{GC0310_8BIT, 0x10, 0x94},
	{GC0310_8BIT, 0xfe, 0x00},	
	{GC0310_TOK_TERM, 0, 0},
};

static struct gc0310_reg const gc0310_stream_off[] = {
	{GC0310_8BIT, 0xfe, 0x03},	
	{GC0310_8BIT, 0x10, 0x84},	
	{GC0310_8BIT, 0xfe, 0x00},	
	{GC0310_TOK_TERM, 0, 0},
};

/* GC0310 scene mode settings */
static struct gc0310_reg gc0310_night_mode_on_table[] = {
	{GC0310_8BIT, 0xfe, 0x01},
	{GC0310_8BIT, GC0310_REG_MAX_AEC, 0x60},
	{GC0310_8BIT, 0x13, 0x45},
//	{GC0310_8BIT, 0xD5, 0x20},
	{GC0310_TOK_TERM, 0, 0},
};

/* Normal scene mode */
static struct gc0310_reg gc0310_night_mode_off_table[] = {
	{GC0310_8BIT, 0xfe, 0x01},
	{GC0310_8BIT, GC0310_REG_MAX_AEC, 0x20},		
	{GC0310_8BIT, 0x13, 0x88},
//	{GC0310_8BIT, 0xD5, 0x00},
	{GC0310_TOK_TERM, 0, 0},
};


static struct gc0310_reg gc0310_frequency_50hz_table[] = {
	{ GC0310_8BIT, 0xfe, 0x00 }, 

	{ GC0310_8BIT, 0x05, 0x01 }, 
	{ GC0310_8BIT, 0x06, 0xe2 }, 
	{ GC0310_8BIT, 0x07, 0x00 }, 
	{ GC0310_8BIT, 0x08, 0x6b }, 

	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0x25, 0x00 }, 
	{ GC0310_8BIT, 0x26, 0x79 }, 


	{ GC0310_8BIT, 0x27, 0x02 }, 
	{ GC0310_8BIT, 0x28, 0x5d }, 
	{ GC0310_8BIT, 0x29, 0x03 }, 
	{ GC0310_8BIT, 0x2a, 0xc8 }, 
	{ GC0310_8BIT, 0x2b, 0x05 }, 
	{ GC0310_8BIT, 0x2c, 0xac }, 
	{ GC0310_8BIT, 0x2d, 0x07 }, 
	{ GC0310_8BIT, 0x2e, 0x90 }, 
	{ GC0310_8BIT, 0x3c, 0x20 }, 

	{ GC0310_8BIT, 0xfe, 0x00 }, 
	{ GC0310_TOK_TERM, 0, 0},
};


static struct gc0310_reg gc0310_frequency_60hz_table[] = {
	{ GC0310_8BIT, 0xfe, 0x00 }, 

	{ GC0310_8BIT, 0x05, 0x01 }, 
	{ GC0310_8BIT, 0x06, 0xec }, 
	{ GC0310_8BIT, 0x07, 0x00 }, 
	{ GC0310_8BIT, 0x08, 0x66 }, 

	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0x25, 0x00 }, 
	{ GC0310_8BIT, 0x26, 0x64 }, 


	{ GC0310_8BIT, 0x27, 0x02 }, 
	{ GC0310_8BIT, 0x28, 0x58 }, 
	{ GC0310_8BIT, 0x29, 0x03 }, 
	{ GC0310_8BIT, 0x2a, 0xe8 }, 
	{ GC0310_8BIT, 0x2b, 0x04 }, 
	{ GC0310_8BIT, 0x2c, 0xb0 }, 
	{ GC0310_8BIT, 0x2d, 0x06 }, 
	{ GC0310_8BIT, 0x2e, 0x40 }, 
	{ GC0310_8BIT, 0x3c, 0x20 }, 

	{ GC0310_8BIT, 0xfe, 0x00 }, 	
	{ GC0310_TOK_TERM, 0, 0},
};

static struct gc0310_reg gc0310_frequency_auto_table[] = {
	{ GC0310_8BIT, 0xfe, 0x00 }, 

	{ GC0310_8BIT, 0x05, 0x01 }, 
	{ GC0310_8BIT, 0x06, 0xe2 }, 
	{ GC0310_8BIT, 0x07, 0x00 }, 
	{ GC0310_8BIT, 0x08, 0x6b }, 

	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0x25, 0x00 }, 
	{ GC0310_8BIT, 0x26, 0x79 }, 


	{ GC0310_8BIT, 0x27, 0x02 }, 
	{ GC0310_8BIT, 0x28, 0x5d }, 
	{ GC0310_8BIT, 0x29, 0x03 }, 
	{ GC0310_8BIT, 0x2a, 0xc8 }, 
	{ GC0310_8BIT, 0x2b, 0x05 }, 
	{ GC0310_8BIT, 0x2c, 0xac }, 
	{ GC0310_8BIT, 0x2d, 0x07 }, 
	{ GC0310_8BIT, 0x2e, 0x90 }, 
	{ GC0310_8BIT, 0x3c, 0x20 }, 

	{ GC0310_8BIT, 0xfe, 0x00 }, 
	{ GC0310_TOK_TERM, 0, 0},
};

static struct gc0310_reg gc0310_frequency_disabled_table[] = {
	{ GC0310_8BIT, 0xfe, 0x00 }, 

	{ GC0310_8BIT, 0x05, 0x01 }, 
	{ GC0310_8BIT, 0x06, 0xe2 }, 
	{ GC0310_8BIT, 0x07, 0x00 }, 
	{ GC0310_8BIT, 0x08, 0x6b }, 

	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0x25, 0x00 }, 
	{ GC0310_8BIT, 0x26, 0x79 }, 


	{ GC0310_8BIT, 0x27, 0x02 }, 
	{ GC0310_8BIT, 0x28, 0xd6 }, 
	{ GC0310_8BIT, 0x29, 0x03 }, 
	{ GC0310_8BIT, 0x2a, 0x4f }, 
	{ GC0310_8BIT, 0x2b, 0x04 }, 
	{ GC0310_8BIT, 0x2c, 0xba }, 
	{ GC0310_8BIT, 0x2d, 0x06 }, 
	{ GC0310_8BIT, 0x2e, 0x9e }, 
	{ GC0310_8BIT, 0x3c, 0x20 }, 

	{ GC0310_8BIT, 0xfe, 0x00 }, 
	{ GC0310_TOK_TERM, 0, 0},
};


static struct gc0310_reg const gc0310_AWB_manual[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x42 , 0xcd},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};

static struct gc0310_reg const gc0310_AWB_auto[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x42 , 0xcf},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};

static struct gc0310_reg const gc0310_AWB_sunny[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x77 , 0x74},
	{GC0310_8BIT, 0x78 , 0x52},
	{GC0310_8BIT, 0x79 , 0x40},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};

static struct gc0310_reg const gc0310_AWB_incandescent[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x77 , 0x48},
	{GC0310_8BIT, 0x78 , 0x40},
	{GC0310_8BIT, 0x79 , 0x5c},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};

static struct gc0310_reg const gc0310_AWB_fluorescent[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x77 , 0x40},
	{GC0310_8BIT, 0x78 , 0x42},
	{GC0310_8BIT, 0x79 , 0x50},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};

static struct gc0310_reg const gc0310_AWB_cloudy[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x77 , 0x8c},
	{GC0310_8BIT, 0x78 , 0x50},
	{GC0310_8BIT, 0x79 , 0x40},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};


static struct gc0310_reg const gc0310_exposure_neg3[] = {
	{ GC0310_8BIT, 0xfe, 0x01 },
	{ GC0310_8BIT, 0x13, 0x45 },
	{ GC0310_8BIT, 0xfe, 0x00 },
	{GC0310_TOK_TERM, 0, 0},
};

static struct gc0310_reg const gc0310_exposure_neg2[] = {
	{ GC0310_8BIT, 0xfe, 0x01 },
	{ GC0310_8BIT, 0x13, 0x55 },
	{ GC0310_8BIT, 0xfe, 0x00 },
	{GC0310_TOK_TERM, 0, 0},
};

static struct gc0310_reg const gc0310_exposure_neg1[] = {
	{ GC0310_8BIT, 0xfe, 0x01 },
	{ GC0310_8BIT, 0x13, 0x65 },
	{ GC0310_8BIT, 0xfe, 0x00 },
	{GC0310_TOK_TERM, 0, 0},
};

static struct gc0310_reg const gc0310_exposure_zero[] = {
	{ GC0310_8BIT, 0xfe, 0x01 },
	{ GC0310_8BIT, 0x13, 0x88 },//75
	{ GC0310_8BIT, 0xfe, 0x00 },
	{GC0310_TOK_TERM, 0, 0},
};

static struct gc0310_reg const gc0310_exposure_pos1[] = {
	{ GC0310_8BIT, 0xfe, 0x01 },
	{ GC0310_8BIT, 0x13, 0x88 },
	{ GC0310_8BIT, 0xfe, 0x00 },
	{GC0310_TOK_TERM, 0, 0},
};

static struct gc0310_reg const gc0310_exposure_pos2[] = {
	{ GC0310_8BIT, 0xfe, 0x01 },
	{ GC0310_8BIT, 0x13, 0x98 },
	{ GC0310_8BIT, 0xfe, 0x00 },
	{GC0310_TOK_TERM, 0, 0},
};

static struct gc0310_reg const gc0310_exposure_pos3[] = {
	{ GC0310_8BIT, 0xfe, 0x01 },
	{ GC0310_8BIT, 0x13, 0xa8 },
	{ GC0310_8BIT, 0xfe, 0x00 },
	{GC0310_TOK_TERM, 0, 0},
};

static struct gc0310_reg const gc0310_normal_effect[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x43 , 0x00},
	{GC0310_8BIT, 0xda , 0x00},
	{GC0310_8BIT, 0xdb , 0x00},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};

static struct gc0310_reg const gc0310_sepia_effect[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x43 , 0x02},
	{GC0310_8BIT, 0xda , 0xd0},
	{GC0310_8BIT, 0xdb , 0x28},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};

static struct gc0310_reg const gc0310_negative_effect[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x43 , 0x01},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};

static struct gc0310_reg const gc0310_bw_effect[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x43 , 0x02},
	{GC0310_8BIT, 0xda , 0x00},
	{GC0310_8BIT, 0xdb , 0x00},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};

static struct gc0310_reg const gc0310_blue_effect[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x43 , 0x02},
	{GC0310_8BIT, 0xda , 0x50},
	{GC0310_8BIT, 0xdb , 0xe0},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};

static struct gc0310_reg const gc0310_green_effect[] = {
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_8BIT, 0x43 , 0x02},
	{GC0310_8BIT, 0xda , 0xc0},
	{GC0310_8BIT, 0xdb , 0xc0},
	{GC0310_8BIT, 0xfe , 0x00},
	{GC0310_TOK_TERM, 0, 0}
};

#if 1

/* resolution: 640 x 480, ratios: 4:3, 1 lanes, 30fps */
static struct gc0310_reg const gc0310_VGA_init[] = {


	{ GC0310_8BIT, 0xfe, 0xf0 }, 
	{ GC0310_8BIT, 0xfe, 0xf0 }, 
	{ GC0310_8BIT, 0xfe, 0x00 }, 
	{ GC0310_8BIT, 0xfc, 0x0e }, 
	{ GC0310_8BIT, 0xfc, 0x0e }, 
	{ GC0310_8BIT, 0xf2, 0x80 }, 
	{ GC0310_8BIT, 0xf3, 0x00 }, 

	{ GC0310_8BIT, 0xf7, 0x1b }, 
	{ GC0310_8BIT, 0xf8, 0x05 }, 

	{ GC0310_8BIT, 0xf9, 0x8e }, 
	{ GC0310_8BIT, 0xfa, 0x11 }, 
	{ GC0310_8BIT, 0xfe, 0x03 }, 
	{ GC0310_8BIT, 0x40, 0x08 }, 
	{ GC0310_8BIT, 0x42, 0x00 }, 
	{ GC0310_8BIT, 0x43, 0x00 }, 
	{ GC0310_8BIT, 0x01, 0x03 }, 
	{ GC0310_8BIT, 0x10, 0x84 }, 

	{ GC0310_8BIT, 0x01, 0x03 }, 
	{ GC0310_8BIT, 0x02, 0x22 }, 
	{ GC0310_8BIT, 0x03, 0x94 }, 
	{ GC0310_8BIT, 0x04, 0x01 }, 
	{ GC0310_8BIT, 0x05, 0x00 }, 
	{ GC0310_8BIT, 0x06, 0x80 }, 
	{ GC0310_8BIT, 0x11, 0x1e }, 
	{ GC0310_8BIT, 0x12, 0x00 }, 
	{ GC0310_8BIT, 0x13, 0x05 }, 
	{ GC0310_8BIT, 0x15, 0x12 }, 
	{ GC0310_8BIT, 0x17, 0xf0 }, 

	{ GC0310_8BIT, 0x21, 0x02 }, 
	{ GC0310_8BIT, 0x22, 0x02 }, 
	{ GC0310_8BIT, 0x23, 0x01 }, 
	{ GC0310_8BIT, 0x29, 0x02 }, 
	{ GC0310_8BIT, 0x2a, 0x02 }, 

	{ GC0310_8BIT, 0xfe, 0x00 }, 


	{ GC0310_8BIT, 0x00, 0x2f }, 
	{ GC0310_8BIT, 0x01, 0x0f }, 
	{ GC0310_8BIT, 0x02, 0x04 }, 
	{ GC0310_8BIT, 0x03, 0x02 }, 
	{ GC0310_8BIT, 0x04, 0xee }, 
	{ GC0310_8BIT, 0x09, 0x00 }, 
	{ GC0310_8BIT, 0x0a, 0x00 }, 
	{ GC0310_8BIT, 0x0b, 0x00 }, 
	{ GC0310_8BIT, 0x0c, 0x04 }, 
	{ GC0310_8BIT, 0x0d, 0x01 }, 
	{ GC0310_8BIT, 0x0e, 0xf2 }, //e8
	{ GC0310_8BIT, 0x0f, 0x02 }, 
	{ GC0310_8BIT, 0x10, 0x96 }, //88
	{ GC0310_8BIT, 0x16, 0x00 }, 
	{ GC0310_8BIT, 0x17, 0x14 }, 
	{ GC0310_8BIT, 0x18, 0x1a }, 
	{ GC0310_8BIT, 0x19, 0x14 }, 
	{ GC0310_8BIT, 0x1b, 0x48 }, 
	{ GC0310_8BIT, 0x1e, 0x6b }, 
	{ GC0310_8BIT, 0x1f, 0x28 }, 
	{ GC0310_8BIT, 0x20, 0x8b }, 
	{ GC0310_8BIT, 0x21, 0x49 }, 
	{ GC0310_8BIT, 0x22, 0xb0 }, 
	{ GC0310_8BIT, 0x23, 0x04 }, 
	{ GC0310_8BIT, 0x24, 0x16 }, 
	{ GC0310_8BIT, 0x34, 0x20 }, 

	{ GC0310_8BIT, 0x26, 0x23 }, 
	{ GC0310_8BIT, 0x28, 0xff }, 
	{ GC0310_8BIT, 0x29, 0x00 }, 
	{ GC0310_8BIT, 0x33, 0x10 }, 
	{ GC0310_8BIT, 0x37, 0x20 }, 
	{ GC0310_8BIT, 0x38, 0x10 }, 
	{ GC0310_8BIT, 0x47, 0x80 }, 
	{ GC0310_8BIT, 0x4e, 0x66 }, 
	{ GC0310_8BIT, 0xa8, 0x02 }, 
	{ GC0310_8BIT, 0xa9, 0x80 }, 

	{ GC0310_8BIT, 0x40, 0xff }, 
	{ GC0310_8BIT, 0x41, 0x25 }, 
	{ GC0310_8BIT, 0x42, 0xcf }, 
	{ GC0310_8BIT, 0x44, 0x00 }, 
	{ GC0310_8BIT, 0x45, 0xaf }, 
	{ GC0310_8BIT, 0x46, 0x02 }, 
	{ GC0310_8BIT, 0x4a, 0x11 }, 
	{ GC0310_8BIT, 0x4b, 0x01 }, 
	{ GC0310_8BIT, 0x4c, 0x20 }, 
	{ GC0310_8BIT, 0x4d, 0x05 }, 
	{ GC0310_8BIT, 0x4f, 0x01 }, 
	{ GC0310_8BIT, 0x50, 0x01 }, 
	{ GC0310_8BIT, 0x55, 0x01 }, 
	{ GC0310_8BIT, 0x56, 0xe0 }, //e0
	{ GC0310_8BIT, 0x57, 0x02 }, 
	{ GC0310_8BIT, 0x58, 0x80 }, //80

	{ GC0310_8BIT, 0x70, 0x70 }, 
	{ GC0310_8BIT, 0x5a, 0x84 }, 
	{ GC0310_8BIT, 0x5b, 0xc9 }, 
	{ GC0310_8BIT, 0x5c, 0xed }, 
	{ GC0310_8BIT, 0x77, 0x71 }, 
	{ GC0310_8BIT, 0x78, 0x4c }, 
	{ GC0310_8BIT, 0x79, 0x50 }, 

	{ GC0310_8BIT, 0x82, 0x14 }, 
	{ GC0310_8BIT, 0x83, 0x0b }, 
	{ GC0310_8BIT, 0x89, 0xf0 }, 

	{ GC0310_8BIT, 0x8f, 0xaa }, 
	{ GC0310_8BIT, 0x90, 0x8c }, 
	{ GC0310_8BIT, 0x91, 0x90 }, 
	{ GC0310_8BIT, 0x92, 0x03 }, 
	{ GC0310_8BIT, 0x93, 0x03 }, 
	{ GC0310_8BIT, 0x94, 0x05 }, 
	{ GC0310_8BIT, 0x95, 0x65 }, 
	{ GC0310_8BIT, 0x96, 0xf0 }, 

	{ GC0310_8BIT, 0xfe, 0x00 }, 
	{ GC0310_8BIT, 0x9a, 0x20 }, 
	{ GC0310_8BIT, 0x9b, 0xa0 },//0x80 
	{ GC0310_8BIT, 0x9c, 0x40 }, 
	{ GC0310_8BIT, 0x9d, 0x80 }, 
	{ GC0310_8BIT, 0xa1, 0x30 }, 
	{ GC0310_8BIT, 0xa2, 0x32 }, 
	{ GC0310_8BIT, 0xa4, 0x30 }, 
	{ GC0310_8BIT, 0xa5, 0x30 }, 
	{ GC0310_8BIT, 0xaa, 0x10 }, 
	{ GC0310_8BIT, 0xac, 0x66 },//22 
	 
	{ GC0310_8BIT, 0xfe, 0x00 }, 
	{ GC0310_8BIT, 0xbf, 0x0b }, 
	{ GC0310_8BIT, 0xc0, 0x17 }, 
	{ GC0310_8BIT, 0xc1, 0x2a }, 
	{ GC0310_8BIT, 0xc2, 0x41 }, 
	{ GC0310_8BIT, 0xc3, 0x54 }, 
	{ GC0310_8BIT, 0xc4, 0x66 }, 
	{ GC0310_8BIT, 0xc5, 0x74 }, 
	{ GC0310_8BIT, 0xc6, 0x8c }, 
	{ GC0310_8BIT, 0xc7, 0xa3 }, 
	{ GC0310_8BIT, 0xc8, 0xb5 }, 
	{ GC0310_8BIT, 0xc9, 0xc4 }, 
	{ GC0310_8BIT, 0xca, 0xd0 }, 
	{ GC0310_8BIT, 0xcb, 0xdb }, 
	{ GC0310_8BIT, 0xcc, 0xe5 }, 
	{ GC0310_8BIT, 0xcd, 0xf0 }, 
	{ GC0310_8BIT, 0xce, 0xf7 }, 
	{ GC0310_8BIT, 0xcf, 0xff }, 

	{ GC0310_8BIT, 0xd0, 0x40 }, 
	{ GC0310_8BIT, 0xd1, 0x27 }, 
	{ GC0310_8BIT, 0xd2, 0x27 }, 
	{ GC0310_8BIT, 0xd3, 0x40 }, 
	{ GC0310_8BIT, 0xd6, 0xf2 }, 
	{ GC0310_8BIT, 0xd7, 0x1b }, 
	{ GC0310_8BIT, 0xd8, 0x18 }, 
	{ GC0310_8BIT, 0xdd, 0x03 }, 

	{ GC0310_8BIT, 0xde, 0xe5 }, 
	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0x1e, 0x41 }, 	
	{ GC0310_8BIT, 0x58, 0x04 }, 
	{ GC0310_8BIT, 0x05, 0x30 }, 
	{ GC0310_8BIT, 0x06, 0x75 }, 
	{ GC0310_8BIT, 0x07, 0x40 }, 
	{ GC0310_8BIT, 0x08, 0xb0 }, 
	{ GC0310_8BIT, 0x0a, 0x85 }, //c5
	{ GC0310_8BIT, 0x0b, 0x11 }, 
	{ GC0310_8BIT, 0x0c, 0x00 }, 

	{ GC0310_8BIT, 0x12, 0x52 }, 
	{ GC0310_8BIT, 0x13, 0x88 }, //90
	{ GC0310_8BIT, 0x18, 0x95 }, 
	{ GC0310_8BIT, 0x19, 0x96 }, 
	{ GC0310_8BIT, 0x1f, 0x30 }, 
	{ GC0310_8BIT, 0x20, 0xc0 }, 
	{ GC0310_8BIT, 0x3e, 0x40 }, 
	{ GC0310_8BIT, 0x3f, 0x57 }, 
	{ GC0310_8BIT, 0x40, 0x7d }, 
	{ GC0310_8BIT, 0x03, 0x60 }, 
	{ GC0310_8BIT, 0x44, 0x02 }, 

	{ GC0310_8BIT, 0x1c, 0x91 }, 
	{ GC0310_8BIT, 0x21, 0x15 }, 
	{ GC0310_8BIT, 0x50, 0x80 }, 
	{ GC0310_8BIT, 0x56, 0x00 },//04 
	{ GC0310_8BIT, 0x59, 0x08 }, 
	{ GC0310_8BIT, 0x5b, 0x04 },//02 
	{ GC0310_8BIT, 0x61, 0x8d }, 
	{ GC0310_8BIT, 0x62, 0xa7 }, 
	{ GC0310_8BIT, 0x63, 0xd0 }, 
	{ GC0310_8BIT, 0x65, 0x06 }, 
	{ GC0310_8BIT, 0x66, 0x06 }, 
	{ GC0310_8BIT, 0x67, 0x84 }, 
	{ GC0310_8BIT, 0x69, 0x08 }, 
	{ GC0310_8BIT, 0x6a, 0x25 }, 
	{ GC0310_8BIT, 0x6b, 0x01 }, 
	{ GC0310_8BIT, 0x6c, 0x00 }, 
	{ GC0310_8BIT, 0x6d, 0x02 }, 
	{ GC0310_8BIT, 0x6e, 0xf0 }, //f0 //e0
	{ GC0310_8BIT, 0x6f, 0x40 }, 
	{ GC0310_8BIT, 0x76, 0x80 }, 
	{ GC0310_8BIT, 0x78, 0xdf }, 
	{ GC0310_8BIT, 0x79, 0x75 }, 
	{ GC0310_8BIT, 0x7a, 0x40 }, 
	{ GC0310_8BIT, 0x7b, 0x70 }, 
	{ GC0310_8BIT, 0x7c, 0x0c }, 

	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0x90, 0x00 }, 
	{ GC0310_8BIT, 0x91, 0x00 }, 
	{ GC0310_8BIT, 0x92, 0xf5 }, 
	{ GC0310_8BIT, 0x93, 0xde }, 
	{ GC0310_8BIT, 0x95, 0x0b }, 
	{ GC0310_8BIT, 0x96, 0xf5 }, 
	{ GC0310_8BIT, 0x97, 0x44 }, 
	{ GC0310_8BIT, 0x98, 0x1f }, 
	{ GC0310_8BIT, 0x9a, 0x4b }, 
	{ GC0310_8BIT, 0x9b, 0x1f }, 
	{ GC0310_8BIT, 0x9c, 0x7e }, 
	{ GC0310_8BIT, 0x9d, 0x4b }, 
	{ GC0310_8BIT, 0x9f, 0x8e }, 
	{ GC0310_8BIT, 0xa0, 0x7e }, 
	{ GC0310_8BIT, 0xa1, 0x00 }, 
	{ GC0310_8BIT, 0xa2, 0x00 }, 
	{ GC0310_8BIT, 0x86, 0x00 }, 
	{ GC0310_8BIT, 0x87, 0x00 }, 
	{ GC0310_8BIT, 0x88, 0x00 }, 
	{ GC0310_8BIT, 0x89, 0x00 }, 
	{ GC0310_8BIT, 0xa4, 0x00 }, 
	{ GC0310_8BIT, 0xa5, 0x00 }, 
	{ GC0310_8BIT, 0xa6, 0xc4 }, 
	{ GC0310_8BIT, 0xa7, 0x99 }, 
	{ GC0310_8BIT, 0xa9, 0xc6 }, 
	{ GC0310_8BIT, 0xaa, 0x9a }, 
	{ GC0310_8BIT, 0xab, 0xab }, 
	{ GC0310_8BIT, 0xac, 0x94 }, 
	{ GC0310_8BIT, 0xae, 0xc7 }, 
	{ GC0310_8BIT, 0xaf, 0xa8 }, 
	{ GC0310_8BIT, 0xb0, 0xc6 }, 
	{ GC0310_8BIT, 0xb1, 0xab }, 
	{ GC0310_8BIT, 0xb3, 0xca }, 
	{ GC0310_8BIT, 0xb4, 0xac }, 
	{ GC0310_8BIT, 0xb5, 0x00 }, 
	{ GC0310_8BIT, 0xb6, 0x00 }, 
	{ GC0310_8BIT, 0x8b, 0x00 }, 
	{ GC0310_8BIT, 0x8c, 0x00 }, 
	{ GC0310_8BIT, 0x8d, 0x00 }, 
	{ GC0310_8BIT, 0x8e, 0x00 }, 
	{ GC0310_8BIT, 0x94, 0x50 }, 
	{ GC0310_8BIT, 0x99, 0xa6 }, 
	{ GC0310_8BIT, 0x9e, 0xaa }, 
	{ GC0310_8BIT, 0xa3, 0x0a }, 
	{ GC0310_8BIT, 0x8a, 0x00 }, 
	{ GC0310_8BIT, 0xa8, 0x50 }, 
	{ GC0310_8BIT, 0xad, 0x55 }, 
	{ GC0310_8BIT, 0xb2, 0x55 }, 
	{ GC0310_8BIT, 0xb7, 0x05 }, 
	{ GC0310_8BIT, 0x8f, 0x00 }, 
	{ GC0310_8BIT, 0xb8, 0xd5 }, 
	{ GC0310_8BIT, 0xb9, 0x8d }, 

	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0xd0, 0x3d }, 
	{ GC0310_8BIT, 0xd1, 0xf0 }, //fc
	{ GC0310_8BIT, 0xd2, 0x02 }, 
	{ GC0310_8BIT, 0xd3, 0x02 }, 
	{ GC0310_8BIT, 0xd4, 0x58 }, 
	{ GC0310_8BIT, 0xd5, 0xf0 }, //ee
	{ GC0310_8BIT, 0xd6, 0x44 }, 
	{ GC0310_8BIT, 0xd7, 0xf0 }, 
	{ GC0310_8BIT, 0xd8, 0xf8 }, 
	{ GC0310_8BIT, 0xd9, 0xf8 }, 
	{ GC0310_8BIT, 0xda, 0x46 }, 
	{ GC0310_8BIT, 0xdb, 0xe4 }, 

	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0xc1, 0x3c }, 
	{ GC0310_8BIT, 0xc2, 0x50 }, 
	{ GC0310_8BIT, 0xc3, 0x00 }, 
	{ GC0310_8BIT, 0xc4, 0x40 },//0x58 
	{ GC0310_8BIT, 0xc5, 0x20 },//0x30 
	{ GC0310_8BIT, 0xc6, 0x1b },//0x30
	{ GC0310_8BIT, 0xc7, 0x10 }, 
	{ GC0310_8BIT, 0xc8, 0x00 }, 
	{ GC0310_8BIT, 0xc9, 0x00 }, 
	{ GC0310_8BIT, 0xdc, 0x20 }, 
	{ GC0310_8BIT, 0xdd, 0x10 }, 
	{ GC0310_8BIT, 0xdf, 0x00 }, 
	{ GC0310_8BIT, 0xde, 0x00 }, 

	{ GC0310_8BIT, 0x01, 0x10 }, 
	{ GC0310_8BIT, 0x0b, 0x31 }, 
	{ GC0310_8BIT, 0x0e, 0x50 }, 
	{ GC0310_8BIT, 0x0f, 0x0f }, 
	{ GC0310_8BIT, 0x10, 0x6e }, 
	{ GC0310_8BIT, 0x12, 0xa0 }, 
	{ GC0310_8BIT, 0x15, 0x60 }, 
	{ GC0310_8BIT, 0x16, 0x60 }, 
	{ GC0310_8BIT, 0x17, 0xe0 }, 

	{ GC0310_8BIT, 0xcc, 0x0c }, 
	{ GC0310_8BIT, 0xcd, 0x10 }, 
	{ GC0310_8BIT, 0xce, 0xa0 }, 
	{ GC0310_8BIT, 0xcf, 0xe6 }, 

	{ GC0310_8BIT, 0x45, 0xf7 }, 
	{ GC0310_8BIT, 0x46, 0xff }, 
	{ GC0310_8BIT, 0x47, 0x15 }, 
	{ GC0310_8BIT, 0x48, 0x03 }, 
	{ GC0310_8BIT, 0x4f, 0x60 }, 


	{ GC0310_8BIT, 0xfe, 0x00 }, 

	{ GC0310_8BIT, 0x05, 0x01 }, 
	{ GC0310_8BIT, 0x06, 0xe2 }, 
	{ GC0310_8BIT, 0x07, 0x00 }, 
	{ GC0310_8BIT, 0x08, 0x6b }, 


	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0x25, 0x00 }, 
	{ GC0310_8BIT, 0x26, 0x79 }, 


	{ GC0310_8BIT, 0x27, 0x02 }, 
	{ GC0310_8BIT, 0x28, 0x5d }, 
	{ GC0310_8BIT, 0x29, 0x03 }, 
	{ GC0310_8BIT, 0x2a, 0xc8}, 
	{ GC0310_8BIT, 0x2b, 0x05 }, //05
	{ GC0310_8BIT, 0x2c, 0xac }, //dc
	{ GC0310_8BIT, 0x2d, 0x07 }, 
	{ GC0310_8BIT, 0x2e, 0x90 }, 
	{ GC0310_8BIT, 0x3c, 0x20 }, 
	{ GC0310_8BIT, 0xfe, 0x00 }, 

	{ GC0310_8BIT, 0xfe, 0x00 }, 

};

#else

/* resolution: 640 x 480, ratios: 4:3, 1 lanes, 30fps */
static struct gc0310_reg const gc0310_VGA_init[] = {

	{ GC0310_8BIT, 0xfe, 0xf0 }, 
	{ GC0310_8BIT, 0xfe, 0xf0 }, 
	{ GC0310_8BIT, 0xfe, 0x00 }, 
	{ GC0310_8BIT, 0xfc, 0x0e }, 
	{ GC0310_8BIT, 0xfc, 0x0e }, 
	{ GC0310_8BIT, 0xf2, 0x80 }, 
	{ GC0310_8BIT, 0xf3, 0x00 }, 

	{ GC0310_8BIT, 0xf7, 0x1b }, 
	{ GC0310_8BIT, 0xf8, 0x03 }, 

	{ GC0310_8BIT, 0xf9, 0x8e }, 
	{ GC0310_8BIT, 0xfa, 0x11 }, 
	{ GC0310_8BIT, 0xfe, 0x03 }, 
	{ GC0310_8BIT, 0x40, 0x08 }, 
	{ GC0310_8BIT, 0x42, 0x00 }, 
	{ GC0310_8BIT, 0x43, 0x00 }, 
	{ GC0310_8BIT, 0x01, 0x03 }, 
	{ GC0310_8BIT, 0x10, 0x84 }, 
	
	{ GC0310_8BIT, 0x01, 0x03 }, 
	{ GC0310_8BIT, 0x02, 0x22 }, 
	{ GC0310_8BIT, 0x03, 0x94 }, 
	{ GC0310_8BIT, 0x04, 0x01 }, 
	{ GC0310_8BIT, 0x05, 0x00 }, 
	{ GC0310_8BIT, 0x06, 0x80 }, 
	{ GC0310_8BIT, 0x11, 0x1e }, 
	{ GC0310_8BIT, 0x12, 0x00 }, 
	{ GC0310_8BIT, 0x13, 0x05 }, 
	{ GC0310_8BIT, 0x15, 0x12 }, 
	{ GC0310_8BIT, 0x17, 0xf0 }, 

	{ GC0310_8BIT, 0x21, 0x02 }, 
	{ GC0310_8BIT, 0x22, 0x02 }, 
	{ GC0310_8BIT, 0x23, 0x01 }, 
	{ GC0310_8BIT, 0x29, 0x02 }, 
	{ GC0310_8BIT, 0x2a, 0x02 }, 

	{ GC0310_8BIT, 0xfe, 0x00 }, 

	
	{ GC0310_8BIT, 0x00, 0x2f }, 
	{ GC0310_8BIT, 0x01, 0x0f }, 
	{ GC0310_8BIT, 0x02, 0x04 }, 
	{ GC0310_8BIT, 0x03, 0x02 }, 
	{ GC0310_8BIT, 0x04, 0xee }, 
	{ GC0310_8BIT, 0x09, 0x00 }, 
	{ GC0310_8BIT, 0x0a, 0x00 }, //0x00 
	{ GC0310_8BIT, 0x0b, 0x00 }, 
	{ GC0310_8BIT, 0x0c, 0x04 }, //0x04 
	{ GC0310_8BIT, 0x0d, 0x01 }, 
	{ GC0310_8BIT, 0x0e, 0xf2 }, //e8
	{ GC0310_8BIT, 0x0f, 0x02 }, 
	{ GC0310_8BIT, 0x10, 0x96 }, //88
	{ GC0310_8BIT, 0x16, 0x00 }, 
	{ GC0310_8BIT, 0x17, 0x14 }, 
	{ GC0310_8BIT, 0x18, 0x1a }, 
	{ GC0310_8BIT, 0x19, 0x14 }, 
	{ GC0310_8BIT, 0x1b, 0x48 }, 
	{ GC0310_8BIT, 0x1e, 0x6b }, 
	{ GC0310_8BIT, 0x1f, 0x28 }, 
	{ GC0310_8BIT, 0x20, 0x8b }, 
	{ GC0310_8BIT, 0x21, 0x49 }, 
	{ GC0310_8BIT, 0x22, 0xb0 }, 
	{ GC0310_8BIT, 0x23, 0x04 }, 
	{ GC0310_8BIT, 0x24, 0x16 }, 
	{ GC0310_8BIT, 0x34, 0x20 }, 
	
	{ GC0310_8BIT, 0x26, 0x23 }, 
	{ GC0310_8BIT, 0x28, 0xff }, 
	{ GC0310_8BIT, 0x29, 0x00 }, 
	{ GC0310_8BIT, 0x33, 0x10 }, 
	{ GC0310_8BIT, 0x37, 0x20 }, 
	{ GC0310_8BIT, 0x38, 0x10 }, 
	{ GC0310_8BIT, 0x47, 0x80 }, 
	{ GC0310_8BIT, 0x4e, 0x66 }, 
	{ GC0310_8BIT, 0xa8, 0x02 }, 
	{ GC0310_8BIT, 0xa9, 0x80 }, 
	
	{ GC0310_8BIT, 0x40, 0xff }, 
	{ GC0310_8BIT, 0x41, 0x25 }, 
	{ GC0310_8BIT, 0x42, 0xcf }, 
	{ GC0310_8BIT, 0x44, 0x00 }, 
	{ GC0310_8BIT, 0x45, 0xa8 }, 
	{ GC0310_8BIT, 0x46, 0x02 }, 
	{ GC0310_8BIT, 0x4a, 0x11 }, 
	{ GC0310_8BIT, 0x4b, 0x01 }, 
	{ GC0310_8BIT, 0x4c, 0x20 }, 
	{ GC0310_8BIT, 0x4d, 0x05 }, 
	{ GC0310_8BIT, 0x4f, 0x01 }, 
	{ GC0310_8BIT, 0x50, 0x01 }, 
	{ GC0310_8BIT, 0x55, 0x01 }, 
	{ GC0310_8BIT, 0x56, 0xe0 }, //e0
	{ GC0310_8BIT, 0x57, 0x02 }, 
	{ GC0310_8BIT, 0x58, 0x80 }, //80
	
	{ GC0310_8BIT, 0x70, 0x70 }, 
	{ GC0310_8BIT, 0x5a, 0x84 }, 
	{ GC0310_8BIT, 0x5b, 0xc9 }, 
	{ GC0310_8BIT, 0x5c, 0xed }, 
	{ GC0310_8BIT, 0x77, 0x71 }, 
	{ GC0310_8BIT, 0x78, 0x4c }, 
	{ GC0310_8BIT, 0x79, 0x50 }, 
	
	{ GC0310_8BIT, 0x82, 0x14 }, 
	{ GC0310_8BIT, 0x83, 0x0b }, 
	{ GC0310_8BIT, 0x89, 0xf0 }, 
	
	{ GC0310_8BIT, 0x8f, 0xaa }, 
	{ GC0310_8BIT, 0x90, 0x8c }, 
	{ GC0310_8BIT, 0x91, 0x90 }, 
	{ GC0310_8BIT, 0x92, 0x03 }, 
	{ GC0310_8BIT, 0x93, 0x03 }, 
	{ GC0310_8BIT, 0x94, 0x05 }, 
	{ GC0310_8BIT, 0x95, 0x65 }, 
	{ GC0310_8BIT, 0x96, 0xf0 }, 
	
	{ GC0310_8BIT, 0xfe, 0x00 }, 
	{ GC0310_8BIT, 0x9a, 0x20 }, 
	{ GC0310_8BIT, 0x9b, 0xa0 },//0x80 
	{ GC0310_8BIT, 0x9c, 0x40 }, 
	{ GC0310_8BIT, 0x9d, 0x80 }, 
	{ GC0310_8BIT, 0xa1, 0x30 }, 
	{ GC0310_8BIT, 0xa2, 0x32 }, 
	{ GC0310_8BIT, 0xa4, 0x30 }, 
	{ GC0310_8BIT, 0xa5, 0x30 }, 
	{ GC0310_8BIT, 0xaa, 0x10 }, 
	{ GC0310_8BIT, 0xac, 0x66 },//22 
	 
	{ GC0310_8BIT, 0xfe, 0x00 }, 
	{ GC0310_8BIT, 0xbf, 0x03 }, 
	{ GC0310_8BIT, 0xc0, 0x08 }, 
	{ GC0310_8BIT, 0xc1, 0x2e }, 
	{ GC0310_8BIT, 0xc2, 0x47 }, 
	{ GC0310_8BIT, 0xc3, 0x66 }, 
	{ GC0310_8BIT, 0xc4, 0x7f }, 
	{ GC0310_8BIT, 0xc5, 0x95 }, 
	{ GC0310_8BIT, 0xc6, 0xb5 }, 
	{ GC0310_8BIT, 0xc7, 0xcb }, 
	{ GC0310_8BIT, 0xc8, 0xd7 }, 
	{ GC0310_8BIT, 0xc9, 0xe1 }, 
	{ GC0310_8BIT, 0xca, 0xea }, 
	{ GC0310_8BIT, 0xcb, 0xf0 }, 
	{ GC0310_8BIT, 0xcc, 0xf6 }, 
	{ GC0310_8BIT, 0xcd, 0xfc }, 
	{ GC0310_8BIT, 0xce, 0xfd }, 
	{ GC0310_8BIT, 0xcf, 0xff }, 
	
	{ GC0310_8BIT, 0xd0, 0x40 }, 
	{ GC0310_8BIT, 0xd1, 0x2b }, 
	{ GC0310_8BIT, 0xd2, 0x2b }, 
	{ GC0310_8BIT, 0xd3, 0x40 }, 
	{ GC0310_8BIT, 0xd6, 0xf2 }, 
	{ GC0310_8BIT, 0xd7, 0x1b }, 
	{ GC0310_8BIT, 0xd8, 0x18 }, 
	{ GC0310_8BIT, 0xdd, 0x03 }, 
	
	{ GC0310_8BIT, 0xde, 0xe5 }, 
	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0x1e, 0x41 }, 	
	{ GC0310_8BIT, 0x58, 0x04 }, 
	{ GC0310_8BIT, 0x05, 0x30 }, 
	{ GC0310_8BIT, 0x06, 0x75 }, 
	{ GC0310_8BIT, 0x07, 0x40 }, 
	{ GC0310_8BIT, 0x08, 0xb0 }, 
	{ GC0310_8BIT, 0x0a, 0x85 }, //c5
	{ GC0310_8BIT, 0x0b, 0x11 }, 
	{ GC0310_8BIT, 0x0c, 0x00 }, 
	{ GC0310_8BIT, 0x12, 0x52 }, 
	{ GC0310_8BIT, 0x13, 0x88 }, //90
	{ GC0310_8BIT, 0x18, 0x95 }, 
	{ GC0310_8BIT, 0x19, 0x96 }, 
	{ GC0310_8BIT, 0x1f, 0x30 }, 
	{ GC0310_8BIT, 0x20, 0xc0 }, 
	{ GC0310_8BIT, 0x3e, 0x40 }, 
	{ GC0310_8BIT, 0x3f, 0x57 }, 
	{ GC0310_8BIT, 0x40, 0x7d }, 
	{ GC0310_8BIT, 0x03, 0x60 }, 
	{ GC0310_8BIT, 0x44, 0x02 }, 
	
	{ GC0310_8BIT, 0x1c, 0x91 }, 
	{ GC0310_8BIT, 0x21, 0x15 }, 
	{ GC0310_8BIT, 0x50, 0x80 }, 
	{ GC0310_8BIT, 0x56, 0x04 }, 
	{ GC0310_8BIT, 0x59, 0x08 }, 
	{ GC0310_8BIT, 0x5b, 0x02 }, 
	{ GC0310_8BIT, 0x61, 0x8d }, 
	{ GC0310_8BIT, 0x62, 0xa7 }, 
	{ GC0310_8BIT, 0x63, 0xd0 }, 
	{ GC0310_8BIT, 0x65, 0x06 }, 
	{ GC0310_8BIT, 0x66, 0x06 }, 
	{ GC0310_8BIT, 0x67, 0x84 }, 
	{ GC0310_8BIT, 0x69, 0x08 }, 
	{ GC0310_8BIT, 0x6a, 0x25 }, 
	{ GC0310_8BIT, 0x6b, 0x01 }, 
	{ GC0310_8BIT, 0x6c, 0x00 }, 
	{ GC0310_8BIT, 0x6d, 0x02 }, 
	{ GC0310_8BIT, 0x6e, 0xe0 }, //f0
	{ GC0310_8BIT, 0x6f, 0x40 }, 
	{ GC0310_8BIT, 0x76, 0x80 }, 
	{ GC0310_8BIT, 0x78, 0xaf }, 
	{ GC0310_8BIT, 0x79, 0x75 }, 
	{ GC0310_8BIT, 0x7a, 0x40 }, 
	{ GC0310_8BIT, 0x7b, 0x70 }, 
	{ GC0310_8BIT, 0x7c, 0x0c }, 

	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0x90, 0x00 }, 
	{ GC0310_8BIT, 0x91, 0x00 }, 
	{ GC0310_8BIT, 0x92, 0xff }, 
	{ GC0310_8BIT, 0x93, 0xe3 }, 
	{ GC0310_8BIT, 0x95, 0x18 }, 
	{ GC0310_8BIT, 0x96, 0x00 }, 
	{ GC0310_8BIT, 0x97, 0x48 }, 
	{ GC0310_8BIT, 0x98, 0x18 }, 
	{ GC0310_8BIT, 0x9a, 0x48 }, 
	{ GC0310_8BIT, 0x9b, 0x18 }, 
	{ GC0310_8BIT, 0x9c, 0x88 }, 
	{ GC0310_8BIT, 0x9d, 0x48 }, 
	{ GC0310_8BIT, 0x9f, 0x00 }, 
	{ GC0310_8BIT, 0xa0, 0x00 }, 
	{ GC0310_8BIT, 0xa1, 0x00 }, 
	{ GC0310_8BIT, 0xa2, 0x00 }, 
	{ GC0310_8BIT, 0x86, 0x00 }, 
	{ GC0310_8BIT, 0x87, 0x00 }, 
	{ GC0310_8BIT, 0x88, 0x00 }, 
	{ GC0310_8BIT, 0x89, 0x00 }, 
	{ GC0310_8BIT, 0xa4, 0x00 }, 
	{ GC0310_8BIT, 0xa5, 0x00 }, 
	{ GC0310_8BIT, 0xa6, 0xcb }, 
	{ GC0310_8BIT, 0xa7, 0xa2 }, 
	{ GC0310_8BIT, 0xa9, 0xcb }, 
	{ GC0310_8BIT, 0xaa, 0x9b }, 
	{ GC0310_8BIT, 0xab, 0xb1 }, 
	{ GC0310_8BIT, 0xac, 0x8e }, 
	{ GC0310_8BIT, 0xae, 0xcb }, 
	{ GC0310_8BIT, 0xaf, 0xb1 }, 
	{ GC0310_8BIT, 0xb0, 0xcb }, 
	{ GC0310_8BIT, 0xb1, 0x9e }, 
	{ GC0310_8BIT, 0xb3, 0x00 }, 
	{ GC0310_8BIT, 0xb4, 0x00 }, 
	{ GC0310_8BIT, 0xb5, 0x00 }, 
	{ GC0310_8BIT, 0xb6, 0x00 }, 
	{ GC0310_8BIT, 0x8b, 0x00 }, 
	{ GC0310_8BIT, 0x8c, 0x00 }, 
	{ GC0310_8BIT, 0x8d, 0x00 }, 
	{ GC0310_8BIT, 0x8e, 0x00 }, 
	{ GC0310_8BIT, 0x94, 0x50 }, 
	{ GC0310_8BIT, 0x99, 0xaa }, 
	{ GC0310_8BIT, 0x9e, 0xaa }, 
	{ GC0310_8BIT, 0xa3, 0x00 }, 
	{ GC0310_8BIT, 0x8a, 0x00 }, 
	{ GC0310_8BIT, 0xa8, 0x50 }, 
	{ GC0310_8BIT, 0xad, 0x55 }, 
	{ GC0310_8BIT, 0xb2, 0x55 }, 
	{ GC0310_8BIT, 0xb7, 0x00 }, 
	{ GC0310_8BIT, 0x8f, 0x00 }, 
	{ GC0310_8BIT, 0xb8, 0xcc }, 
	{ GC0310_8BIT, 0xb9, 0x9a }, 

	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0xd0, 0x40 }, 
	{ GC0310_8BIT, 0xd1, 0xf9 }, //fc
	{ GC0310_8BIT, 0xd2, 0x08 }, 
	{ GC0310_8BIT, 0xd3, 0x02 }, 
	{ GC0310_8BIT, 0xd4, 0x3c }, 
	{ GC0310_8BIT, 0xd5, 0xf0 }, //ee
	{ GC0310_8BIT, 0xd6, 0x30 }, 
	{ GC0310_8BIT, 0xd7, 0x00 }, 
	{ GC0310_8BIT, 0xd8, 0x0a }, 
	{ GC0310_8BIT, 0xd9, 0x16 }, 
	{ GC0310_8BIT, 0xda, 0x39 }, 
	{ GC0310_8BIT, 0xdb, 0xf8 }, 
	
	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0xc1, 0x3c }, 
	{ GC0310_8BIT, 0xc2, 0x50 }, 
	{ GC0310_8BIT, 0xc3, 0x00 }, 
	{ GC0310_8BIT, 0xc4, 0x58 }, 
	{ GC0310_8BIT, 0xc5, 0x30 }, 
	{ GC0310_8BIT, 0xc6, 0x24 }, 
	{ GC0310_8BIT, 0xc7, 0x18 }, 
	{ GC0310_8BIT, 0xc8, 0x00 }, 
	{ GC0310_8BIT, 0xc9, 0x18 }, 
	{ GC0310_8BIT, 0xdc, 0x20 }, 
	{ GC0310_8BIT, 0xdd, 0x10 }, 
	{ GC0310_8BIT, 0xdf, 0x00 }, 
	{ GC0310_8BIT, 0xde, 0x00 }, 
	
	{ GC0310_8BIT, 0x01, 0x10 }, 
	{ GC0310_8BIT, 0x0b, 0x31 }, 
	{ GC0310_8BIT, 0x0e, 0x50 }, 
	{ GC0310_8BIT, 0x0f, 0x0f }, 
	{ GC0310_8BIT, 0x10, 0x6e }, 
	{ GC0310_8BIT, 0x12, 0xa0 }, 
	{ GC0310_8BIT, 0x15, 0x60 }, 
	{ GC0310_8BIT, 0x16, 0x60 }, 
	{ GC0310_8BIT, 0x17, 0xe0 }, 
	
	{ GC0310_8BIT, 0xcc, 0x0c }, 
	{ GC0310_8BIT, 0xcd, 0x10 }, 
	{ GC0310_8BIT, 0xce, 0xa0 }, 
	{ GC0310_8BIT, 0xcf, 0xe6 }, 
	
	{ GC0310_8BIT, 0x45, 0xf7 }, 
	{ GC0310_8BIT, 0x46, 0xff }, 
	{ GC0310_8BIT, 0x47, 0x15 }, 
	{ GC0310_8BIT, 0x48, 0x03 }, 
	{ GC0310_8BIT, 0x4f, 0x60 }, 
	

	{ GC0310_8BIT, 0xfe, 0x00 }, 

	{ GC0310_8BIT, 0x05, 0x01 }, 
	{ GC0310_8BIT, 0x06, 0xe2 }, 
	{ GC0310_8BIT, 0x07, 0x00 }, 
	{ GC0310_8BIT, 0x08, 0x6b }, 


	{ GC0310_8BIT, 0xfe, 0x01 }, 
	{ GC0310_8BIT, 0x25, 0x00 }, 
	{ GC0310_8BIT, 0x26, 0x79 }, 


	{ GC0310_8BIT, 0x27, 0x02 }, 
	{ GC0310_8BIT, 0x28, 0x5d }, 
	{ GC0310_8BIT, 0x29, 0x03 }, 
	{ GC0310_8BIT, 0x2a, 0xc8}, 
	{ GC0310_8BIT, 0x2b, 0x05 }, //05
	{ GC0310_8BIT, 0x2c, 0xac }, //dc
	{ GC0310_8BIT, 0x2d, 0x07 }, 
	{ GC0310_8BIT, 0x2e, 0x90 }, 
	{ GC0310_8BIT, 0x3c, 0x20 }, 
	{ GC0310_8BIT, 0xfe, 0x00 }, 

	{ GC0310_8BIT, 0xfe, 0x00 }, 
	{ GC0310_TOK_TERM, 0, 0}

};

#endif

static struct gc0310_reg const gc0310_common[] = {
	 {GC0310_TOK_TERM, 0, 0}
};

static struct gc0310_reg const gc0310_iq[] = {
	{GC0310_TOK_TERM, 0, 0}
};

#endif
