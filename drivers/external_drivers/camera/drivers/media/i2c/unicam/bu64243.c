#include <asm/intel-mid.h>
#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <media/v4l2-device.h>

#include "bu64243.h"

static struct bu64243_device bu64243_dev;
static int bu64243_i2c_write(struct i2c_client *client, int reg, u8 val)
{
	struct i2c_msg msg;
	u8 buf[4];

	memset(&msg, 0 , sizeof(msg));
	msg.addr = BU64243_I2C_ADDR;
	msg.len = 2;
	msg.buf = &buf[0];
	msg.buf[0] = reg;
	msg.buf[1] = val;

	return i2c_transfer(client->adapter, &msg, 1);
}

static int bu64243_i2c_read(struct i2c_client *client, int reg, u8 *val)
{
	u8 buf[4];
	struct i2c_msg msg[2];
	int r;

	memset(msg, 0 , sizeof(msg));
	msg[0].addr = BU64243_I2C_ADDR;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &buf[0];
	msg[0].buf[0] = reg;

	msg[1].addr = BU64243_I2C_ADDR;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = val;

	r = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (r != ARRAY_SIZE(msg))
		return -EIO;

	return 0;
}

static int bu64243_cmd(struct v4l2_subdev *sd, s32 reg, s32 val)
{
	int cmd = 0;

	cmd = BU64243_PS(1) | BU64243_EN(BU64243_DEFAULT_OUTPUT_STATUS) |
			BU64243_W0_W2(reg) | BU64243_M(BU64243_DEFAULT_ISRC_MODE) |
			BU64243_D_HI(val);

	return cmd;
}

int bu64243_vcm_power_up(struct v4l2_subdev *sd)
{
	int ret = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int r, value;

	/* Enable power */
	if (bu64243_dev.platform_data) {
		ret = bu64243_dev.platform_data->power_ctrl(sd, 1);
		if (ret){
			return ret;
		}
	}

	r = bu64243_i2c_write(client, bu64243_cmd(sd, BU64243_VCM_CURRENT, BU64243_DEFAULT_POINT_C), BU64243_D_LO(BU64243_DEFAULT_POINT_C));
	if (r < 0){
		return r;
	}
	value = (BU64243_RFEQ(BU64243_DEFAULT_VCM_FREQ)) | BU64243_SRATE((BU64243_DEFAULT_SLEW_RATE));
	r = bu64243_i2c_write(client, bu64243_cmd(sd, BU64243_PARAM_1, value), BU64243_D_LO(value));
	if (r < 0)
		return r;

	r = bu64243_i2c_write(client, bu64243_cmd(sd, BU64243_PARAM_2, BU64243_DEFAULT_POINT_A), BU64243_D_LO(BU64243_DEFAULT_POINT_A));
	if (r < 0)
		return r;

	r = bu64243_i2c_write(client, bu64243_cmd(sd, BU64243_PARAM_3, BU64243_DEFAULT_POINT_B), BU64243_D_LO(BU64243_DEFAULT_POINT_B));
	if (r < 0)
		return r;

	value = (BU64243_STIME(BU64243_DEFAULT_STEP_TIME_SETTING)) | (BU64243_SRES(BU64243_DEFAULT_RES_SETTING));
	r = bu64243_i2c_write(client, bu64243_cmd(sd, BU64243_PARAM_4, value), BU64243_D_LO(value));
	if (r < 0)
		return r;
	v4l2_info(client, "detected bu64243\n");

	return 0;
}

int bu64243_vcm_power_down(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int focus_far =0;
	u8 data[10];
	int r,i;
	s32 current_value,average,focus_value;
	int ret = -ENODEV;

	memset(data, 0 , sizeof(data));
	focus_far = VCM_ORIENTATION_OFFSET + INTEL_FOCUS_OFFSET +1 + POINT_AB_OFFSET;
	r = bu64243_i2c_write(client, bu64243_cmd(sd, BU64243_VCM_CURRENT, focus_far), BU64243_D_LO(focus_far));
	if (r < 0)
		return r;
	bu64243_i2c_read(client, bu64243_cmd(sd, BU64243_VCM_CURRENT, 0), data);
	current_value = ((data[0] &0x03)<<8) +data[1];
	average = current_value/10;
	for (i= 1; i < 11; i ++){
		focus_value =current_value -average*i;
		r = bu64243_i2c_write(client, bu64243_cmd(sd, BU64243_VCM_CURRENT, focus_value), BU64243_D_LO(focus_value));
		if (r < 0)
		      return r;
             mdelay(5);
		}

	if (bu64243_dev.platform_data)
		ret = bu64243_dev.platform_data->power_ctrl(sd, 0);	

	return 0;
}

int bu64243_t_focus_vcm(struct v4l2_subdev *sd, u16 val)
{
	return 0;
}

int bu64243_t_focus_abs(struct v4l2_subdev *sd, s32 value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int r;

	if (!bu64243_dev.platform_data)
		return -ENODEV;
	value = clamp(value, 0, BU64243_MAX_FOCUS_POS);

	r = bu64243_i2c_write(client, bu64243_cmd(sd, BU64243_VCM_CURRENT,value), BU64243_D_LO(value));
	if (r < 0){
		return r;
	}

	bu64243_dev.focus = value;
	ktime_get_ts(&bu64243_dev.timestamp_t_focus_abs);

	return 0;
}

int bu64243_t_focus_rel(struct v4l2_subdev *sd, s32 value)
{
	return bu64243_t_focus_abs(sd, bu64243_dev.focus + value);
}

int bu64243_q_focus_status(struct v4l2_subdev *sd, s32 *value)
{
	u32 status = 0;
	struct timespec temptime;
	const struct timespec timedelay = {
		0,
		min_t(u32, abs(bu64243_dev.number_of_steps)*DELAY_PER_STEP_NS,
			DELAY_MAX_PER_STEP_NS),
	};

	ktime_get_ts(&temptime);

	temptime = timespec_sub(temptime, (bu64243_dev.timestamp_t_focus_abs));

	if (timespec_compare(&temptime, &timedelay) <= 0)
		status = ATOMISP_FOCUS_STATUS_MOVING
			| ATOMISP_FOCUS_HP_IN_PROGRESS;
	else
		status = ATOMISP_FOCUS_STATUS_ACCEPTS_NEW_MOVE
			| ATOMISP_FOCUS_HP_COMPLETE;

	*value = status;

	return 0;
}

int bu64243_q_focus_abs(struct v4l2_subdev *sd, s32 *value)
{
	s32 val;

	bu64243_q_focus_status(sd, &val);

	if (val & ATOMISP_FOCUS_STATUS_MOVING)
		*value	= bu64243_dev.focus - bu64243_dev.number_of_steps;
	else
		*value	= bu64243_dev.focus ;

	return 0;
}

int bu64243_t_vcm_slew(struct v4l2_subdev *sd, s32 value)
{
	return 0;
}

int bu64243_t_vcm_timing(struct v4l2_subdev *sd, s32 value)
{
	return 0;
}

int bu64243_vcm_init(struct v4l2_subdev *sd)
{
	bu64243_dev.platform_data = camera_get_af_platform_data();
	return bu64243_dev.platform_data ? 0 : -ENODEV;
	return 0;
}

