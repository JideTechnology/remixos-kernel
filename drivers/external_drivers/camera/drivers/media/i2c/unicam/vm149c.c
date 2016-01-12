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
//#include <media/v4l2-chip-ident.h>
#include <media/v4l2-device.h>

#include "vm149c.h"

static struct vm149c_device vm149c_dev;



static int vm149c_i2c_write(struct i2c_client *client, u16 val)
{
	struct i2c_msg msg;
	u8 buf[2] = {(char)(val >> 4) , (char)(((val & 0xF) << 4))};
	
	msg.addr = VM149C_VCM_ADDR;
	msg.flags = 0;
	msg.len = VM149C_REG_LENGTH + VM149C_8BIT;
	msg.buf = &buf[0];

	if (i2c_transfer(client->adapter, &msg, 1) != 1)
		return -EIO;
	return 0;
}

int vm149c_vcm_power_up(struct v4l2_subdev *sd)
{
	int ret = 0;

	/* Enable power */
	if (vm149c_dev.platform_data) {
		ret = vm149c_dev.platform_data->power_ctrl(sd, 1);
		if (ret)
			return ret;
	}
	/*
	 * waiting time requested by VM149C(vcm)
	 */
	usleep_range(1000, 2000);

	/*
	 * Set vcm ringing control mode.
	 */
	if (vm149c_dev.vcm_mode != VM149C_DIRECT) {

		/* set the VCM_TIME */

	} else {
			return ret;
	}

	return ret;
}

int vm149c_vcm_power_down(struct v4l2_subdev *sd)
{
	int ret = -ENODEV;

	if (vm149c_dev.platform_data)
		ret = vm149c_dev.platform_data->power_ctrl(sd, 0);

	return ret;
}


int vm149c_t_focus_vcm(struct v4l2_subdev *sd, u16 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = -EINVAL;

	ret = vm149c_i2c_write(client, val);

	return ret;
}

int vm149c_t_focus_abs(struct v4l2_subdev *sd, s32 value)
{
	int ret;

	value = min(value, VM149C_MAX_FOCUS_POS);
	ret = vm149c_t_focus_vcm(sd,  value);
	//printk("%s, val: %d\n",__func__,value);
	if (ret == 0) {
		vm149c_dev.number_of_steps = value - vm149c_dev.focus;
		vm149c_dev.focus = value;
		ktime_get_ts(&vm149c_dev.timestamp_t_focus_abs);
	}

	return ret;
}

int vm149c_t_focus_rel(struct v4l2_subdev *sd, s32 value)
{
	return vm149c_t_focus_abs(sd, vm149c_dev.focus + value);
}

int vm149c_q_focus_status(struct v4l2_subdev *sd, s32 *value)
{
	u32 status = 0;
	struct timespec temptime;
	const struct timespec timedelay = {
		0,
		min_t(u32, abs(vm149c_dev.number_of_steps)*DELAY_PER_STEP_NS,
			DELAY_MAX_PER_STEP_NS),
	};

	ktime_get_ts(&temptime);

	temptime = timespec_sub(temptime, (vm149c_dev.timestamp_t_focus_abs));

	if (timespec_compare(&temptime, &timedelay) <= 0)
		status = ATOMISP_FOCUS_STATUS_MOVING
			| ATOMISP_FOCUS_HP_IN_PROGRESS;
	else
		status = ATOMISP_FOCUS_STATUS_ACCEPTS_NEW_MOVE
			| ATOMISP_FOCUS_HP_COMPLETE;

	*value = status;

	return 0;
}

int vm149c_q_focus_abs(struct v4l2_subdev *sd, s32 *value)
{
	s32 val;

	vm149c_q_focus_status(sd, &val);

	if (val & ATOMISP_FOCUS_STATUS_MOVING)
		*value  = vm149c_dev.focus - vm149c_dev.number_of_steps;
	else
		*value  = vm149c_dev.focus ;

	return 0;
}

int vm149c_t_vcm_slew(struct v4l2_subdev *sd, s32 value)
{
	return 0;
}

int vm149c_t_vcm_timing(struct v4l2_subdev *sd, s32 value)
{
	return 0;
}

int vm149c_vcm_init(struct v4l2_subdev *sd)
{
	/* set vcm mode to ARC RES0.5 */
	vm149c_dev.vcm_mode = VM149C_ARC_RES1;
	vm149c_dev.platform_data = camera_get_af_platform_data();
	return vm149c_dev.platform_data ? 0 : -ENODEV;
}

