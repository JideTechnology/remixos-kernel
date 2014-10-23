/*
 * imx135.c - imx135 sensor driver
 *
 * Copyright (c) 2013, NVIDIA CORPORATION, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/regulator/consumer.h>
#include <media/imx135.h>
#include <linux/gpio.h>
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include "imx135_tables.h"

struct imx135_info {
	struct miscdevice		miscdev_info;
	int				mode;
	struct imx135_power_rail	power;
	struct nvc_fuseid		fuse_id;
	struct i2c_client		*i2c_client;
	struct imx135_platform_data	*pdata;
	struct mutex			imx135_camera_lock;
	struct dentry			*debugdir;
	atomic_t			in_use;
};
static struct imx135_reg flash_strobe_mod[] = {
	{0x0800, 0x01},	/* flash strobe output enble on ERS mode */
	{0x0801, 0x01},	/* reference dividor fron EXT CLK */
	{0x0804, 0x04},	/* shutter sync mode */
	{0x0806, 0x00},	/* ref point hi */
	{0x0807, 0x00},	/* ref point lo */
	{0x0808, 0x00},	/* latency hi from ref point */
	{0x0809, 0x00},	/* latency lo from ref point */
	{0x080A, 0x09},	/* high period of XHS for ERS hi */
	{0x080B, 0x60},	/* high period of XHS for ERS lo */
	{0x080C, 0x00},	/* low period of XHS for ERS hi */
	{0x080D, 0x00},	/* low period of XHS for ERS lo */
	{0x3100, 0x01},	/* XHS output control, 1 - FSTROBE enabled */
	{0x3104, 0x00},	/* XHS driving capability, 0 - 2mA */
	{0x3108, 0x00},	/* for 'sync to XVS' mode */
	{0x3109, 0x00},	/* AE bracketing mode, 0 - normal */
	{0x080E, 0x01},	/* num of ERS flash pulse */
	{IMX135_TABLE_END, 0x00}
};

static inline void
msleep_range(unsigned int delay_base)
{
	usleep_range(delay_base*1000, delay_base*1000+500);
}

static inline void
imx135_get_frame_length_regs(struct imx135_reg *regs, u32 frame_length)
{
	regs->addr = 0x0340;
	regs->val = (frame_length >> 8) & 0xff;
	(regs + 1)->addr = 0x0341;
	(regs + 1)->val = (frame_length) & 0xff;
}

static inline void
imx135_get_coarse_time_regs(struct imx135_reg *regs, u32 coarse_time)
{
	regs->addr = 0x202;
	regs->val = (coarse_time >> 8) & 0xff;
	(regs + 1)->addr = 0x203;
	(regs + 1)->val = (coarse_time) & 0xff;
}

static inline void
imx135_get_coarse_time_short_regs(struct imx135_reg *regs, u32 coarse_time)
{
	regs->addr = 0x230;
	regs->val = (coarse_time >> 8) & 0xff;
	(regs + 1)->addr = 0x231;
	(regs + 1)->val = (coarse_time) & 0xff;
}

static inline void
imx135_get_gain_reg(struct imx135_reg *regs, u16 gain)
{
	regs->addr = 0x205;
	regs->val = gain;
}

static inline void
imx135_get_gain_short_reg(struct imx135_reg *regs, u16 gain)
{
	regs->addr = 0x233;
	regs->val = gain;
}

static int
imx135_read_reg(struct i2c_client *client, u16 addr, u8 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[3];

	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = data;

	/* high byte goes out first */
	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data + 2;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err != 2)
		return -EINVAL;

	*val = data[2];
	return 0;
}

static int
imx135_write_reg(struct i2c_client *client, u16 addr, u8 val)
{
	int err;
	struct i2c_msg msg;
	unsigned char data[3];

	if (!client->adapter)
		return -ENODEV;

	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);
	data[2] = (u8) (val & 0xff);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = data;

	err = i2c_transfer(client->adapter, &msg, 1);
	if (err == 1)
		return 0;

	pr_err("%s:i2c write failed, %x = %x\n",
			__func__, addr, val);

	return err;
}

static int
imx135_write_table(struct i2c_client *client,
				 const struct imx135_reg table[],
				 const struct imx135_reg override_list[],
				 int num_override_regs)
{
	int err;
	const struct imx135_reg *next;
	int i;
	u16 val;

	for (next = table; next->addr != IMX135_TABLE_END; next++) {
		if (next->addr == IMX135_TABLE_WAIT_MS) {
			msleep_range(next->val);
			continue;
		}

		val = next->val;

		/* When an override list is passed in, replace the reg */
		/* value to write if the reg is in the list            */
		if (override_list) {
			for (i = 0; i < num_override_regs; i++) {
				if (next->addr == override_list[i].addr) {
					val = override_list[i].val;
					break;
				}
			}
		}

		err = imx135_write_reg(client, next->addr, val);
		if (err) {
			pr_err("%s:imx135_write_table:%d", __func__, err);
			return err;
		}
	}
	return 0;
}

static int imx135_set_flash_output(struct imx135_info *info)
{
	struct imx135_flash_control *fctl;

	if (!info->pdata)
		return 0;

	fctl = &info->pdata->flash_cap;
	dev_dbg(&info->i2c_client->dev, "%s: %x\n", __func__, fctl->enable);
	dev_dbg(&info->i2c_client->dev, "edg: %x, st: %x, rpt: %x, dly: %x\n",
		fctl->edge_trig_en, fctl->start_edge,
		fctl->repeat, fctl->delay_frm);
	return imx135_write_table(info->i2c_client, flash_strobe_mod, NULL, 0);
}

static int imx135_get_flash_cap(struct imx135_info *info)
{
	struct imx135_flash_control *fctl;

	dev_dbg(&info->i2c_client->dev, "%s: %p\n", __func__, info->pdata);
	if (info->pdata) {
		fctl = &info->pdata->flash_cap;
		dev_dbg(&info->i2c_client->dev,
			"edg: %x, st: %x, rpt: %x, dl: %x\n",
			fctl->edge_trig_en,
			fctl->start_edge,
			fctl->repeat,
			fctl->delay_frm);

		if (fctl->enable)
			return 0;
	}
	return -ENODEV;
}

static inline int imx135_set_flash_control(
	struct imx135_info *info, struct imx135_flash_control *fc)
{
	dev_dbg(&info->i2c_client->dev, "%s\n", __func__);
	return imx135_write_reg(info->i2c_client, 0x0802, 0x01);
}

static int
imx135_set_mode(struct imx135_info *info, struct imx135_mode *mode)
{
	int sensor_mode;
	u8 quality_hdr;
	int err;
	struct imx135_reg reg_list[8];

	pr_info("%s: xres %u yres %u framelength %u coarsetime %u gain %u, hdr %d\n",
			 __func__, mode->xres, mode->yres, mode->frame_length,
			 mode->coarse_time, mode->gain, mode->hdr_en);

	if (mode->hdr_en == true) {
		dev_info(&info->i2c_client->dev, "HDR mode\n");
		if (mode->xres == 4208 && mode->yres == 3120) {
			sensor_mode = IMX135_MODE_4208X3120_HDR;
			quality_hdr = 1;
		} else if (mode->xres == 2104 && mode->yres == 1560) {
			sensor_mode = IMX135_MODE_2104X1560_HDR;
			quality_hdr = 1;
		} else if (mode->xres == 1920 && mode->yres == 1080) {
			sensor_mode = IMX135_MODE_1920X1080_HDR;
			quality_hdr = 1;
		} else if (mode->xres == 2616 && mode->yres == 1472) {
			sensor_mode = IMX135_MODE_2616X1472_HDR;
			quality_hdr = 1;
		} else if (mode->xres == 3840 && mode->yres == 2160) {
			sensor_mode = IMX135_MODE_3840X2160_HDR;
			quality_hdr = 1;
		} else {
			pr_err("%s: invalid resolution supplied to set mode %d %d\n",
				 __func__, mode->xres, mode->yres);
			return -EINVAL;
		}
	} else {
		dev_info(&info->i2c_client->dev, "none HDR mode\n");
		if (mode->xres == 4208 && mode->yres == 3120) {
			sensor_mode = IMX135_MODE_4208X3120;
			quality_hdr = 0;
		} else if (mode->xres == 3840 && mode->yres == 2160) {
			sensor_mode = IMX135_MODE_3840X2160;
			quality_hdr = 0;
		} else if (mode->xres == 2104 && mode->yres == 1560) {
			sensor_mode = IMX135_MODE_2104X1560;
			quality_hdr = 0;
		} else if (mode->xres == 1920 && mode->yres == 1080) {
			sensor_mode = IMX135_MODE_1920X1080;
			quality_hdr = 0;
		} else if (mode->xres == 1280 && mode->yres == 720) {
			sensor_mode = IMX135_MODE_1280X720;
			quality_hdr = 0;
		} else if (mode->xres == 640 && mode->yres == 480) {
			sensor_mode = IMX135_MODE_640X480;
			quality_hdr = 0;
		} else {
			pr_err("%s: invalid resolution supplied to set mode %d %d\n",
				 __func__, mode->xres, mode->yres);
			return -EINVAL;
		}
	}
	/* get a list of override regs for the asking frame length, */
	/* coarse integration time, and gain.                       */
	imx135_get_frame_length_regs(reg_list, mode->frame_length);
	imx135_get_coarse_time_regs(reg_list + 2, mode->coarse_time);
	imx135_get_gain_reg(reg_list + 4, mode->gain);
	/* if HDR is enabled */
	if (mode->hdr_en == 1) {
		imx135_get_gain_short_reg(reg_list + 5, mode->gain);
		imx135_get_coarse_time_short_regs(
			reg_list + 6, mode->coarse_time_short);
	}

	err = imx135_write_table(info->i2c_client,
				mode_table[sensor_mode],
				reg_list, mode->hdr_en ? 8 : 5);
	if (err)
		return err;
	if (quality_hdr)
		err = imx135_write_table(info->i2c_client,
				mode_table[IMX135_MODE_QUALITY_HDR],
				reg_list, 0);
	else
		err = imx135_write_table(info->i2c_client,
				mode_table[IMX135_MODE_QUALITY],
				reg_list, 0);
	if (err)
		return err;

	imx135_set_flash_output(info);

	info->mode = sensor_mode;
	pr_info("[IMX135]: stream on.\n");
	return 0;
}

static int
imx135_get_status(struct imx135_info *info, u8 *dev_status)
{
	*dev_status = 0;
	return 0;
}

static int
imx135_set_frame_length(struct imx135_info *info, u32 frame_length,
						 bool group_hold)
{
	struct imx135_reg reg_list[2];
	int i = 0;
	int ret;

	imx135_get_frame_length_regs(reg_list, frame_length);

	if (group_hold) {
		ret = imx135_write_reg(info->i2c_client, 0x0104, 0x01);
		if (ret)
			return ret;
	}

	for (i = 0; i < 2; i++) {
		ret = imx135_write_reg(info->i2c_client, reg_list[i].addr,
			 reg_list[i].val);
		if (ret)
			return ret;
	}

	if (group_hold) {
		ret = imx135_write_reg(info->i2c_client, 0x0104, 0x0);
		if (ret)
			return ret;
	}

	return 0;
}

static int
imx135_set_coarse_time(struct imx135_info *info, u32 coarse_time,
						 bool group_hold)
{
	int ret;

	struct imx135_reg reg_list[2];
	int i = 0;

	imx135_get_coarse_time_regs(reg_list, coarse_time);

	if (group_hold) {
		ret = imx135_write_reg(info->i2c_client, 0x104, 0x01);
		if (ret)
			return ret;
	}

	for (i = 0; i < 2; i++) {
		ret = imx135_write_reg(info->i2c_client, reg_list[i].addr,
			 reg_list[i].val);
		if (ret)
			return ret;
	}

	if (group_hold) {
		ret = imx135_write_reg(info->i2c_client, 0x104, 0x0);
		if (ret)
			return ret;
	}
	return 0;
}

static int
imx135_set_gain(struct imx135_info *info, u16 gain, bool group_hold)
{
	int ret;
	struct imx135_reg reg_list;

	imx135_get_gain_reg(&reg_list, gain);

	if (group_hold) {
		ret = imx135_write_reg(info->i2c_client, 0x104, 0x1);
		if (ret)
			return ret;
	}

	ret = imx135_write_reg(info->i2c_client, reg_list.addr, reg_list.val);
	/* writing second gain register for HDR */
	ret = imx135_write_reg(info->i2c_client, 0x233, reg_list.val);
	if (ret)
		return ret;

	if (group_hold) {
		ret = imx135_write_reg(info->i2c_client, 0x104, 0x0);
		if (ret)
			return ret;
	}
	return 0;
}

static int
imx135_set_hdr_coarse_time(struct imx135_info *info, struct imx135_hdr *values)
{
	struct imx135_reg reg_list[2];
	struct imx135_reg reg_list_short[2];
	int ret, i = 0;

	/* get long and short coarse time registers */
	imx135_get_coarse_time_regs(reg_list, values->coarse_time_long);
	imx135_get_coarse_time_short_regs(reg_list_short,
			values->coarse_time_short);
	/* set to direct mode */
	ret = imx135_write_reg(info->i2c_client, 0x238, 0x1);
	if (ret)
		return ret;
	/* set group hold */
	ret = imx135_write_reg(info->i2c_client, 0x104, 0x1);
	if (ret)
		return ret;
	/* writing long exposure */
	for (i = 0; i < 2; i++) {
		ret = imx135_write_reg(info->i2c_client, reg_list[i].addr,
			 reg_list[i].val);
		if (ret)
			return ret;
	}
	/* writing short exposure */
	for (i = 0; i < 2; i++) {
		ret = imx135_write_reg(info->i2c_client, reg_list_short[i].addr,
			 reg_list_short[i].val);
		if (ret)
			return ret;
	}
	ret = imx135_write_reg(info->i2c_client, 0x104, 0x0);
	if (ret)
		return ret;

	return 0;
}

static int
imx135_set_group_hold(struct imx135_info *info, struct imx135_ae *ae)
{
	int ret;
	int count = 0;
	bool groupHoldEnabled = false;
	struct imx135_hdr values;

	values.coarse_time_long = ae->coarse_time;
	values.coarse_time_short = ae->coarse_time_short;

	if (ae->gain_enable)
		count++;
	if (ae->coarse_time_enable)
		count++;
	if (ae->frame_length_enable)
		count++;
	if (count >= 2)
		groupHoldEnabled = true;

	if (groupHoldEnabled) {
		ret = imx135_write_reg(info->i2c_client, 0x104, 0x1);
		if (ret)
			return ret;
	}

	if (ae->gain_enable)
		imx135_set_gain(info, ae->gain, false);
	if (ae->coarse_time_enable)
		imx135_set_hdr_coarse_time(info, &values);
	if (ae->frame_length_enable)
		imx135_set_frame_length(info, ae->frame_length, false);

	if (groupHoldEnabled) {
		ret = imx135_write_reg(info->i2c_client, 0x104, 0x0);
		if (ret)
			return ret;
	}

	return 0;
}

static int imx135_get_sensor_id(struct imx135_info *info)
{
	int ret = 0;
	int i;
	u8 bak = 0;

	pr_info("%s\n", __func__);
	if (info->fuse_id.size)
		return 0;

	/* Note 1: If the sensor does not have power at this point
	Need to supply the power, e.g. by calling power on function */

	ret |= imx135_write_reg(info->i2c_client, 0x3B02, 0x00);
	ret |= imx135_write_reg(info->i2c_client, 0x3B00, 0x01);
	for (i = 0; i < 9 ; i++) {
		ret |= imx135_read_reg(info->i2c_client, 0x3B24 + i, &bak);
		info->fuse_id.data[i] = bak;
	}

	if (!ret)
		info->fuse_id.size = i;

	/* Note 2: Need to clean up any action carried out in Note 1 */

	return ret;
}

static long
imx135_ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct imx135_info *info = file->private_data;

	switch (cmd) {
	case IMX135_IOCTL_SET_POWER:
		if (!info->pdata)
			break;
		if (arg && info->pdata->power_on)
			info->pdata->power_on(&info->power);
		if (!arg && info->pdata->power_off)
			info->pdata->power_off(&info->power);

		break;
	case IMX135_IOCTL_SET_MODE:
	{
		struct imx135_mode mode;
		if (copy_from_user(&mode, (const void __user *)arg,
			sizeof(struct imx135_mode))) {
			pr_err("%s:Failed to get mode from user.\n", __func__);
			return -EFAULT;
		}
		return imx135_set_mode(info, &mode);
	}
	case IMX135_IOCTL_SET_FRAME_LENGTH:
		return imx135_set_frame_length(info, (u32)arg, true);
	case IMX135_IOCTL_SET_COARSE_TIME:
		return imx135_set_coarse_time(info, (u32)arg, true);
	case IMX135_IOCTL_SET_GAIN:
		return imx135_set_gain(info, (u16)arg, true);
	case IMX135_IOCTL_GET_STATUS:
	{
		u8 status;

		err = imx135_get_status(info, &status);
		if (err)
			return err;
		if (copy_to_user((void __user *)arg, &status, 1)) {
			pr_err("%s:Failed to copy status to user\n", __func__);
			return -EFAULT;
			}
		return 0;
	}
	case IMX135_IOCTL_GET_FUSEID:
	{
		err = imx135_get_sensor_id(info);

		if (err) {
			pr_err("%s:Failed to get fuse id info.\n", __func__);
			return err;
		}
		if (copy_to_user((void __user *)arg, &info->fuse_id,
				sizeof(struct nvc_fuseid))) {
			pr_info("%s:Failed to copy fuse id to user space\n",
				__func__);
			return -EFAULT;
		}
		return 0;
	}
	case IMX135_IOCTL_SET_GROUP_HOLD:
	{
		struct imx135_ae ae;
		if (copy_from_user(&ae, (const void __user *)arg,
			sizeof(struct imx135_ae))) {
			pr_info("%s:fail group hold\n", __func__);
			return -EFAULT;
		}
		return imx135_set_group_hold(info, &ae);
	}
	case IMX135_IOCTL_SET_HDR_COARSE_TIME:
	{
		struct imx135_hdr values;

		dev_dbg(&info->i2c_client->dev,
				"IMX135_IOCTL_SET_HDR_COARSE_TIME\n");
		if (copy_from_user(&values,
			(const void __user *)arg,
			sizeof(struct imx135_hdr))) {
				err = -EFAULT;
				break;
		}
		err = imx135_set_hdr_coarse_time(info, &values);
		break;
	}
	case IMX135_IOCTL_SET_FLASH_MODE:
	{
		struct imx135_flash_control values;

		dev_dbg(&info->i2c_client->dev,
			"IMX135_IOCTL_SET_FLASH_MODE\n");
		if (copy_from_user(&values,
			(const void __user *)arg,
			sizeof(struct imx135_flash_control))) {
			err = -EFAULT;
			break;
		}
		err = imx135_set_flash_control(info, &values);
		break;
	}
	case IMX135_IOCTL_GET_FLASH_CAP:
		err = imx135_get_flash_cap(info);
		break;
	default:
		pr_err("%s:unknown cmd.\n", __func__);
		err = -EINVAL;
	}

	return err;
}

static int imx135_debugfs_show(struct seq_file *s, void *unused)
{
	struct imx135_info *dev = s->private;

	dev_dbg(&dev->i2c_client->dev, "%s: ++\n", __func__);

	mutex_lock(&dev->imx135_camera_lock);
	mutex_unlock(&dev->imx135_camera_lock);

	return 0;
}

static ssize_t imx135_debugfs_write(
	struct file *file,
	char const __user *buf,
	size_t count,
	loff_t *offset)
{
	struct imx135_info *dev =
			((struct seq_file *)file->private_data)->private;
	struct i2c_client *i2c_client = dev->i2c_client;
	int ret = 0;
	char buffer[24];
	u32 address;
	u32 data;
	u8 readback;

	dev_dbg(&i2c_client->dev, "%s: ++\n", __func__);

	if (copy_from_user(&buffer, buf, sizeof(buffer)))
		goto debugfs_write_fail;

	if (sscanf(buf, "0x%x 0x%x", &address, &data) == 2)
		goto set_attr;
	if (sscanf(buf, "0X%x 0X%x", &address, &data) == 2)
		goto set_attr;
	if (sscanf(buf, "%d %d", &address, &data) == 2)
		goto set_attr;

	if (sscanf(buf, "0x%x 0x%x", &address, &data) == 1)
		goto read;
	if (sscanf(buf, "0X%x 0X%x", &address, &data) == 1)
		goto read;
	if (sscanf(buf, "%d %d", &address, &data) == 1)
		goto read;

	dev_err(&i2c_client->dev, "SYNTAX ERROR: %s\n", buf);
	return -EFAULT;

set_attr:
	dev_info(&i2c_client->dev,
			"new address = %x, data = %x\n", address, data);
	ret |= imx135_write_reg(i2c_client, address, data);
read:
	ret |= imx135_read_reg(i2c_client, address, &readback);
	dev_dbg(&i2c_client->dev,
			"wrote to address 0x%x with value 0x%x\n",
			address, readback);

	if (ret)
		goto debugfs_write_fail;

	return count;

debugfs_write_fail:
	dev_err(&i2c_client->dev,
			"%s: test pattern write failed\n", __func__);
	return -EFAULT;
}

static int imx135_debugfs_open(struct inode *inode, struct file *file)
{
	struct imx135_info *dev = inode->i_private;
	struct i2c_client *i2c_client = dev->i2c_client;

	dev_dbg(&i2c_client->dev, "%s: ++\n", __func__);

	return single_open(file, imx135_debugfs_show, inode->i_private);
}

static const struct file_operations imx135_debugfs_fops = {
	.open		= imx135_debugfs_open,
	.read		= seq_read,
	.write		= imx135_debugfs_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void imx135_remove_debugfs(struct imx135_info *dev)
{
	struct i2c_client *i2c_client = dev->i2c_client;

	dev_dbg(&i2c_client->dev, "%s: ++\n", __func__);

	if (dev->debugdir)
		debugfs_remove_recursive(dev->debugdir);
	dev->debugdir = NULL;
}

static void imx135_create_debugfs(struct imx135_info *dev)
{
	struct dentry *ret;
	struct i2c_client *i2c_client = dev->i2c_client;

	dev_dbg(&i2c_client->dev, "%s\n", __func__);

	dev->debugdir =
		debugfs_create_dir(dev->miscdev_info.this_device->kobj.name,
							NULL);
	if (!dev->debugdir)
		goto remove_debugfs;

	ret = debugfs_create_file("d",
				S_IWUSR | S_IRUGO,
				dev->debugdir, dev,
				&imx135_debugfs_fops);
	if (!ret)
		goto remove_debugfs;

	return;
remove_debugfs:
	dev_err(&i2c_client->dev, "couldn't create debugfs\n");
	imx135_remove_debugfs(dev);
}

static int
imx135_open(struct inode *inode, struct file *file)
{
	struct miscdevice	*miscdev = file->private_data;
	struct imx135_info *info;

	info = container_of(miscdev, struct imx135_info, miscdev_info);
	/* check if the device is in use */
	if (atomic_xchg(&info->in_use, 1)) {
		pr_info("%s:BUSY!\n", __func__);
		return -EBUSY;
	}

	file->private_data = info;

	return 0;
}

static int
imx135_release(struct inode *inode, struct file *file)
{
	struct imx135_info *info = file->private_data;

	file->private_data = NULL;

	/* warn if device is already released */
	WARN_ON(!atomic_xchg(&info->in_use, 0));
	return 0;
}

static int imx135_power_put(struct imx135_power_rail *pw)
{
	if (unlikely(!pw))
		return -EFAULT;

	if (likely(pw->avdd))
		regulator_put(pw->avdd);

	if (likely(pw->iovdd))
		regulator_put(pw->iovdd);

	if (likely(pw->dvdd))
		regulator_put(pw->dvdd);

	pw->avdd = NULL;
	pw->iovdd = NULL;
	pw->dvdd = NULL;

	return 0;
}

static int imx135_regulator_get(struct imx135_info *info,
	struct regulator **vreg, char vreg_name[])
{
	struct regulator *reg = NULL;
	int err = 0;

	reg = regulator_get(&info->i2c_client->dev, vreg_name);
	if (unlikely(IS_ERR_OR_NULL(reg))) {
		dev_err(&info->i2c_client->dev, "%s %s ERR: %d\n",
			__func__, vreg_name, (int)reg);
		err = PTR_ERR(reg);
		reg = NULL;
	} else
		dev_dbg(&info->i2c_client->dev, "%s: %s\n",
			__func__, vreg_name);

	*vreg = reg;
	return err;
}

static int imx135_power_get(struct imx135_info *info)
{
	struct imx135_power_rail *pw = &info->power;

	imx135_regulator_get(info, &pw->avdd, "vana"); /* ananlog 2.7v */
	imx135_regulator_get(info, &pw->dvdd, "vdig"); /* digital 1.2v */
	imx135_regulator_get(info, &pw->iovdd, "vif"); /* interface 1.8v */

	return 0;
}

static const struct file_operations imx135_fileops = {
	.owner = THIS_MODULE,
	.open = imx135_open,
	.unlocked_ioctl = imx135_ioctl,
	.release = imx135_release,
};

static struct miscdevice imx135_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "imx135",
	.fops = &imx135_fileops,
};


static int
imx135_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct imx135_info *info;
	int err;

	pr_info("[IMX135]: probing sensor.\n");

	info = devm_kzalloc(&client->dev,
			sizeof(struct imx135_info), GFP_KERNEL);
	if (!info) {
		pr_err("%s:Unable to allocate memory!\n", __func__);
		return -ENOMEM;
	}

	info->pdata = client->dev.platform_data;
	info->i2c_client = client;
	atomic_set(&info->in_use, 0);
	info->mode = -1;

	imx135_power_get(info);

	memcpy(&info->miscdev_info,
		&imx135_device,
		sizeof(struct miscdevice));

	err = misc_register(&info->miscdev_info);
	if (err) {
		pr_err("%s:Unable to register misc device!\n", __func__);
		goto imx135_probe_fail;
	}

	i2c_set_clientdata(client, info);
	/* create debugfs interface */
	imx135_create_debugfs(info);
	return 0;

imx135_probe_fail:
	imx135_power_put(&info->power);

	return err;
}

static int
imx135_remove(struct i2c_client *client)
{
	struct imx135_info *info;
	info = i2c_get_clientdata(client);
	misc_deregister(&imx135_device);

	imx135_power_put(&info->power);

	imx135_remove_debugfs(info);
	return 0;
}

static const struct i2c_device_id imx135_id[] = {
	{ "imx135", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, imx135_id);

static struct i2c_driver imx135_i2c_driver = {
	.driver = {
		.name = "imx135",
		.owner = THIS_MODULE,
	},
	.probe = imx135_probe,
	.remove = imx135_remove,
	.id_table = imx135_id,
};

static int __init imx135_init(void)
{
	pr_info("[IMX135] sensor driver loading\n");
	return i2c_add_driver(&imx135_i2c_driver);
}

static void __exit imx135_exit(void)
{
	i2c_del_driver(&imx135_i2c_driver);
}

module_init(imx135_init);
module_exit(imx135_exit);
