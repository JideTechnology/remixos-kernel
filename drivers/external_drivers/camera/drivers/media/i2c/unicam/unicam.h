/*
 *
 * Copyright (c) 2014 Intel Corporation. All Rights Reserved.
 *
*/

#ifndef __UNICAM_H__
#define __UNICAM_H__


#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/spinlock.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-device.h>
//#include <media/v4l2-chip-ident.h>
#include <linux/v4l2-mediabus.h>
#include <media/media-entity.h>
#include <media/v4l2-ctrls.h>

#include <linux/atomisp_platform.h>

#define BUILDID 52
#include "unicam_ext_linux.h"
#include "unicam_ext.h"

#define MATCH_NAME_LEN 30
#define RATIO_SHIFT_BITS		13
#define LARGEST_ALLOWED_RATIO_MISMATCH	320


struct match_data {
		u8 name[MATCH_NAME_LEN];
		S_UNI_DEVICE * uni;
};

//UNI_NO_EXT for else include
#ifdef UNI_NO_EXT
111111
//ext start

// debug for .h
#include "unicam_hm8131.h"
#include "unicam_hm2056.h"
#include "unicam_m1040.h"
#include "unicam_ar0543.h"
#include "unicam_ov2722.h"
#include "unicam_imx175s.h"
#include "unicam_ov5693.h"
#include "unicam_gc2235.h"
#include "unicam_gc2355.h"
#include "unicam_ov5648.h"

enum SENSOR_CAM_TYPE {
	sensor_ppr_cam_type,
	sensor_spr_cam_type,
};

//#define SENSOR_PPR_CAM "unicam"

#define SENSOR_PPR_CAM "ppr_cam"
#define SENSOR_SPR_CAM "spr_cam"


static struct match_data m_data[]={
	{"ar0543",&ar0543_unidev},//aptina ar0543 5M
	{"hm8131",&hm8131_unidev},//himax hm8131 8M
	{"ov5693",&ov5693_unidev},//ov ov5693 5M
	{"imx175s",&imx175s_unidev},//sony IMX175 8M
	{"gc2235",&gc2235_unidev},//galaxycore gc2235 2M
	{"ov5648",&ov5648_unidev},//ov ov5648 5M

	{},
};

static struct match_data m_data_front[]={
//	{"m1040",&m1040_unidev},//aptina m1040 1.2M
//	{"hm2056",&hm2056_unidev},//himax hm2056 2M
//	{"hm2051",&hm2051_unidev},//himax hm2051 2M
//  {"ov2722",&ov2722_unidev},//ov ov2722 2M	
	{"gc2355",&gc2355_unidev},//galaxycore gc2355 2M

	{},
};

const struct i2c_device_id uni_id_ppr_cam[] = {
	{"ppr_cam", sensor_ppr_cam_type},
	{"spr_cam", sensor_spr_cam_type},
	{}
};

#endif

//ext end
struct unicam_control {
	struct v4l2_queryctrl qc;
	int (*query)(struct v4l2_subdev *sd, s32 *value);
	int (*tweak)(struct v4l2_subdev *sd, s32 value);
};

struct sensor_data {
	struct v4l2_subdev sd;
	struct media_pad pad;

	struct mutex input_lock; // lock i2c access, and fmt_idx group

	struct camera_sensor_platform_data *platform_data;

	struct unicam_vcm *vcm_driver;

	struct v4l2_ctrl_handler ctrl_handler;

	struct v4l2_ctrl *run_mode;

	struct v4l2_mbus_framefmt format;

	// lock protected
	int fmt_idx;
	int mode;
	int parm_mode;
	S_UNI_RESOLUTION *cur_res ;
	int n_res;
	//

	S_UNI_DEVICE * uni;
};

#define to_sensor_data(x) container_of(x, struct sensor_data, sd)

#endif
