
#ifndef	__LIGHT_H__
#define	__LIGHT_H__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/string.h>
#include <linux/irq.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/ktime.h>
#include <asm/div64.h>
#include <linux/wakelock.h>

#include "di_misc.h"

#define CAL_GAIN_FP_SHIFT	(16)	//to avoid data overflow in calibration process, this value should not be more than 16
#define MAX_CAL_GAIN		(10)

struct light_data {
	struct i2c_client *client;
	struct mutex lock;
	struct delayed_work light_work;
	struct input_dev *input_dev;

	/*create attribute at specific path*/
	struct class* class;
	struct device* dev;

	u16 poll_interval;

	atomic_t enabled;
	u32 low_thres;
	u32 high_thres;
	u8 gain;
	u8 ext_gain;
	//u32 range;

	u32 cal_gain;

	int irq;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

/* Hardware registers needed by driver. */
//17 kinds of related registers for ALS
struct light_reg_s {
	unsigned char sys_cfg;
	unsigned char flg_sts;
	unsigned char int_cfg;
	unsigned char waiting;
	unsigned char gain_cfg;
	unsigned char persist;
	unsigned char mean_time;
	unsigned char dummy;
	unsigned char raw_l;
	unsigned char raw_h;
	unsigned char raw_data;
	unsigned char low_thres_l;
	unsigned char low_thres_h;
	unsigned char high_thres_l;
	unsigned char high_thres_h;
	unsigned char calibration;
	unsigned char addr_bound;
};

//12 kinds of command for ALS
struct light_reg_command_s {
	struct command sw_rst;
	struct command active;
	struct command int_flg;
	struct command int_en;
	struct command suspend;
	struct command clear;
	struct command waiting;
	struct command gain;
	struct command ext_gain;
	struct command persist;
	struct command mean_time;
	struct command dummy;
};

const struct light_reg_s* get_light_reg(void);
const struct light_reg_command_s* get_light_reg_command(void);
int get_light_range(const struct light_data* const data);

int light_probe(struct i2c_client *client, const struct i2c_device_id *id);
int light_remove(struct i2c_client *client);

#endif	/* __LIGHT_H__ */
