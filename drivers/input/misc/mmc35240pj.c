/*
 * Copyright (C) 2014 MEMSIC, Inc.
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
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <linux/input-polldev.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/ioctl.h>

#ifdef CONFIG_ACPI
#include <linux/acpi.h>
#endif
#define INTEL_SOFIA
#define INTEL_CHT_PM_WR
//#define INTEL_SOFIA_DEBUG
#ifdef INTEL_SOFIA_DEBUG
#define DEBUG_SOFIA_VERBOSE(...) dev_info(__VA_ARGS__)
#else
#define DEBUG_SOFIA_VERBOSE(...)
#endif
#define MMC3524X_NAME		"mmc3524x"
//#define MMC3524X_NAME		"mmc3416x"

#define MMC3524X_I2C_ADDR		0x30

#define MMC3524X_REG_CTRL		0x07
#define MMC3524X_REG_BITS		0x08
#define MMC3524X_REG_DATA		0x00
#define MMC3524X_REG_DS			0x06
#define MMC3524X_REG_PRODUCTID_0		0x10
#define MMC3524X_REG_PRODUCTID_1		0x20

#define MMC3524X_CTRL_TM			0x01
#define MMC3524X_CTRL_CM			0x02
#define MMC3524X_CTRL_50HZ		0x0C
#define MMC3524X_CTRL_25HZ		0x08
#define MMC3524X_CTRL_12HZ		0x04
#define MMC3524X_CTRL_1HZ		    0x00
#define MMC3524X_CTRL_NOBOOST            0x10
#define MMC3524X_CTRL_SET	        0x20
#define MMC3524X_CTRL_RESET              0x40
#define MMC3524X_CTRL_REFILL             0x80

#define MMC3524X_BITS_SLOW_16            0x00
#define MMC3524X_BITS_FAST_16            0x01
#define MMC3524X_BITS_14                 0x02
/* Use 'm' as magic number */
#define MMC3524X_IOM			'm'

struct read_reg_str {
	unsigned char start_register;
	unsigned char length;
	unsigned char reg_contents[10];
};

/* IOCTLs for MMC3524X device */

#define MMC3524X_IOC_TM			_IO(MMC3524X_IOM, 0x00)
#define MMC3524X_IOC_SET			_IO(MMC3524X_IOM, 0x01)
#define MMC3524X_IOC_READ		_IOR(MMC3524X_IOM, 0x02, int[3])
#define MMC3524X_IOC_READXYZ		_IOR(MMC3524X_IOM, 0x03, int[3])
#define MMC3524X_IOC_RESET               _IO(MMC3524X_IOM, 0x04)
#define MMC3524X_IOC_NOBOOST             _IO(MMC3524X_IOM, 0x05)
#define MMC3524X_IOC_ID                  _IOR(MMC3524X_IOM, 0x06, short)
#define MMC3524X_IOC_DIAG                _IOR(MMC3524X_IOM, 0x14, int[1])
#define MMC3524X_IOC_READ_REG		_IOWR(MMC3524X_IOM, 0x15,\
		unsigned char)
#define MMC3524X_IOC_WRITE_REG		_IOW(MMC3524X_IOM, 0x16,\
		unsigned char[2])
#define MMC3524X_IOC_READ_REGS		_IOWR(MMC3524X_IOM, 0x17,\
		unsigned char[10])
#define MMC3524X_IOC_WRITE_REGS		_IOW(MMC3524X_IOM, 0x18,\
		unsigned char[10])


struct mmc3524_platform_data {

	u32  poll_interval;
	u32 min_interval;

	u8 fs_range;

	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;

	int x_offset;
	int y_offset;
	int z_offset;

	int (*init)(struct device *dev);
	void (*exit)(void);
	int (*power_on)(struct device *dev);
	int (*power_off)(struct device *dev);

	struct device_pm_platdata *pm_platdata;

	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_sleep;
	struct pinctrl_state *pins_inactive;
};

struct mmc3524x {
	u32 read_idx;
	struct class *mag_class;
	struct i2c_client *client;
	struct input_dev *indev;
	struct mutex lock;
	int ecompass_delay;
	struct delayed_work work;
	int enabled;
	int enabled_before_suspend;
	u32 poll_ms;
	struct mmc3524_platform_data p;
};

#define DEBUG			0
#define MAX_FAILURE_COUNT	3
#define READMD			0

#define MMC3524X_DELAY_TM    10	/* ms */
#define MMC3524X_DELAY_SET    75	/* ms */
#define MMC3524X_DELAY_RESET    75     /* ms */

#define MMC3524X_RETRY_COUNT	3
#define MMC3524X_SET_INTV	250

#define H_MAX			32767
#define MMC3524X_SENSITIVITY    1024//2048
#define MMC3524X_OFFSET       32768

#define MMC3524X_DEV_NAME	"mmc3524x"

static DEFINE_MUTEX(ecompass_lock);

static int mmc3xxx_i2c_rx_data(struct mmc3524x *d , char *buf, int len)
{
	uint8_t i;
	struct i2c_msg msgs[] = {
		{
			.addr	= d->client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= buf,
		},
		{
			.addr	= d->client->addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= buf,
		}
	};

	for (i = 0; i < MMC3524X_RETRY_COUNT; i++) {
		if (i2c_transfer(d->client->adapter, msgs, 2) >= 0)
			break;

		mdelay(10);
	}

	if (i >= MMC3524X_RETRY_COUNT) {
		dev_err(&d->client->dev, "%s:retry over %d\n", __func__,
				MMC3524X_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

static int mmc3xxx_i2c_tx_data(struct mmc3524x *d, char *buf, int len)
{
	uint8_t i;
	struct i2c_msg msg[] = {
		{
			.addr	= d->client->addr,
			.flags	= 0,
			.len	= len,
			.buf	= buf,
		}
	};

	for (i = 0; i < MMC3524X_RETRY_COUNT; i++) {
		if (i2c_transfer(d->client->adapter, msg, 1) >= 0)
			break;

		mdelay(10);
	}

	if (i >= MMC3524X_RETRY_COUNT) {
		dev_err(&d->client->dev, "%s: retry over %d\n", __func__,
				MMC3524X_RETRY_COUNT);
		return -EIO;
	}
	return 0;
}

static unsigned char mmc3524x_mag_poll_hz(struct mmc3524x *d)
{
	unsigned char poll_hz;

	if (d->poll_ms <= 40)
		poll_hz = MMC3524X_CTRL_50HZ;
	else if (d->poll_ms <= 75)
		poll_hz = MMC3524X_CTRL_25HZ;
	else if (d->poll_ms <= 800)
		poll_hz = MMC3524X_CTRL_12HZ;
	else
		poll_hz = MMC3524X_CTRL_1HZ;

	return poll_hz;
}

static void mmc3524x_mag_disable(struct mmc3524x *d)
{
	unsigned char data[2];
	if (d->enabled) {
		data[0] = MMC3524X_REG_CTRL;
		data[1] = 0;
		if (mmc3xxx_i2c_tx_data(d, data, 2) < 0)
			dev_err(&d->client->dev, "%s tx err\r\n", __func__);

		d->enabled = 0;
		cancel_delayed_work(&d->work);
	}
}

static void mmc3524x_mag_enable(struct mmc3524x *d)
{
	unsigned char data[2];

	if (!d->enabled) {
		data[0] = MMC3524X_REG_CTRL;
		data[1] = mmc3524x_mag_poll_hz(d) | MMC3524X_CTRL_CM;
		if (mmc3xxx_i2c_tx_data(d, data, 2) < 0)
			dev_dbg(&d->client->dev, "%s tx err\r\n", __func__);

		d->enabled = 1;
		schedule_delayed_work(&d->work, msecs_to_jiffies(d->poll_ms));
	}
}

static ssize_t attr_set_polling_rate(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	unsigned long interval_ms;
	struct mmc3524x *d = dev_get_drvdata(dev);

	if (kstrtoul(buf, 10, &interval_ms))
		return -EINVAL;
	/* FIXME */
	if (!interval_ms)
		return -EINVAL;
	mutex_lock(&ecompass_lock);
	d->poll_ms = interval_ms;
	mutex_unlock(&ecompass_lock);

	return size;
}

static ssize_t attr_get_polling_rate(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	int val;
	struct mmc3524x *d = dev_get_drvdata(dev);

	mutex_lock(&ecompass_lock);
	val = d->poll_ms;
	mutex_unlock(&ecompass_lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_get_enable(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int ret;
	struct mmc3524x *d = dev_get_drvdata(dev);

	mutex_lock(&ecompass_lock);
	ret = sprintf(buf, "%d\n", d->enabled);
	mutex_unlock(&ecompass_lock);
	return ret;
}

static ssize_t attr_set_enable(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	unsigned long val;
	struct mmc3524x *d = dev_get_drvdata(dev);

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;

	if (val)
		mmc3524x_mag_enable(d);
	else
		mmc3524x_mag_disable(d);

	dev_info(dev, "sensor %s\n", val ? "enable" : "disable");

	return size;
}

#define INTEL_CHERRITRAIL
#ifdef INTEL_CHERRITRAIL
static DEVICE_ATTR(pollrate_ms, 0666, attr_get_polling_rate, attr_set_polling_rate);
static DEVICE_ATTR(enable_device, 0666, attr_get_enable, attr_set_enable);

static struct attribute *mmc_attributes[] = {
	&dev_attr_pollrate_ms.attr,
	&dev_attr_enable_device.attr,
	NULL
};


static struct attribute_group dev_attr_grp = {
	.attrs = mmc_attributes,
};
#else
static struct device_attribute attributes[] = {
	__ATTR(pollrate_ms, 0666, attr_get_polling_rate, attr_set_polling_rate),
	__ATTR(enable_device, 0666, attr_get_enable, attr_set_enable),
	/*__ATTR(mmc3524x, 0444, mmc3524x, NULL),
	__ATTR(fs, 0666, mmc3524x_fs_read, mmc3524x_fs_write);*/
};
#endif

static int create_sysfs_interfaces(struct device *dev)
{
	int i;
	struct mmc3524x *d = dev_get_drvdata(dev);
        
#ifdef INTEL_CHERRITRAIL
        int err = 0;
	err = sysfs_create_group(&d->indev->dev.kobj, &dev_attr_grp);
	if (err) {
		dev_err(dev, "%s: sysfs_create_group returned err = %d. Abort.\n", __func__,err);
                return -1;
	}
	return 0;
#else
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
	
		if (device_create_file(dev, attributes + i))
			goto error;
	return 0;

error:
	for (; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	dev_err(&d->client->dev, "%s:Unable to create interface\n", __func__);
	return -1;
#endif
}

static int remove_sysfs_interfaces(struct device *dev)
{
	int i;
#ifdef INTEL_CHERRITRAIL

	struct mmc3524x *d = dev_get_drvdata(dev);
	sysfs_remove_group(&d->indev->dev.kobj, &dev_attr_grp);
#else
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);
#endif
	return 0;
}
#ifdef INTEL_SOFIA
static int int_otp_x = 0;
static int int_otp_y = 0;
static int int_otp_z = 0;
#define MMC3524X_OTP_START_ADDR (27)
#define OPT_CONVERT(REG) (((REG) >=32 ? (32 - (REG)) : (REG)) * 6)
#define MAG_UNIFY_INT(_r, _o)	(((_r) - (_o))

static int mag_mmc3524x_read_registers(struct mmc3524x *d, u8 reg,u8 len,u8 read_regs[])
{
        if(read_regs ==NULL)
                return -EINVAL;
        read_regs[0] = reg;
        DEBUG_SOFIA_VERBOSE(&d->client->dev,"%s, reg:0x%x,len=%d", __func__,reg,len);
	if (mmc3xxx_i2c_rx_data(d, read_regs, len) < 0)
	return -EFAULT;
       DEBUG_SOFIA_VERBOSE(&d->client->dev,"%s, read regs from 0x%x, 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x ", __func__,reg,read_regs[0],read_regs[1],read_regs[2],read_regs[3],read_regs[4],read_regs[5]);
       return 0;
}

static void mag_mmc3524x_parse_otp_matrix(unsigned char* reg_data)
{
#if 0
	f_OTP_matrix[0] = 1.0;
	f_OTP_matrix[1] = 0.0;	//OPT_CONVERT(reg_data[0] >> 2);
	f_OTP_matrix[2] = 0.0;	//OPT_CONVERT(reg_data[1] >> 2);
	f_OTP_matrix[3] = 0.0;
	f_OTP_matrix[4] = OPT_CONVERT(((reg_data[1] & 0x03) << 4) | (reg_data[2] >> 4)) + 1.0;
	f_OTP_matrix[5] = 0.0;	//OPT_CONVERT(((reg_data[2] & 0x0f) << 2) | (reg_data[3] >> 6));
	f_OTP_matrix[6] = 0.0;
	f_OTP_matrix[7] = 0.0;
	f_OTP_matrix[8] = (OPT_CONVERT(reg_data[3] & 0x3f)+1) * 1.35;
#else
        int_otp_x = 1;
        int_otp_y = OPT_CONVERT(((reg_data[1] & 0x03) << 4) | (reg_data[2] >> 4)) + 1000;
        int_otp_z = (OPT_CONVERT(reg_data[3] & 0x3f) + 1000) * 135;
        
#endif
	printk("i_otp_x : %d\t i_otp_y : %d\t i_otp_z : %d\t",int_otp_x,int_otp_y,int_otp_z) ;
}

int mag_mmc3524x_read_otp(struct mmc3524x *d)
{
	int i, result = 0;
	unsigned char reg_data[6] = {0};	
	//result = mag_mmc3524x_read_registers(fd, MMC3524X_OTP_START_ADDR, 6, reg_data);
        //result = mmc3xxx_i2c_rx_data(d, reg_data,6);
        result = mag_mmc3524x_read_registers(d,MMC3524X_OTP_START_ADDR,6,reg_data);
   	if(result < 0){
                dev_err(&d->client->dev,"mag_mmc3524x_read_registers error code = %d", result);
		return result;
	}	
	DEBUG_SOFIA_VERBOSE(&d->client->dev,"mag_mmc3524x_read_otp  --------result=%d \n",result);
	mag_mmc3524x_parse_otp_matrix(reg_data);
	return result;
}

int mag_mmc3524x_read_data(struct mmc3524x *d, int *data)
{
	int data_reg[3] = {0};
        u8 reg_data[6] = {0}; 
	int result = -1;

        result = mag_mmc3524x_read_registers(d,MMC3524X_REG_DATA,6,reg_data); 
        if(result < 0)
           return result;
        data_reg[0] = (reg_data[1]<<8)| reg_data[0];
        data_reg[1] = (reg_data[3]<<8)| reg_data[2];
        data_reg[2] = (reg_data[5]<<8)| reg_data[4];
	data[0] = data_reg[0];
	data[1] = data_reg[1]  - data_reg[2]  + 32768;
	data[2] = data_reg[1]  + data_reg[2]  - 32768;
        /*unfiy*/
        data[0] -= 32768;
        data[1] -= 32768;
        data[2] -= 32768;
        data[2] *= -1;
	return 0;
}

static int mmc3524x_get_data(struct mmc3524x *d, int xyz[])
{
       //rawx = MAG_UNIFY_INT(mag_mmc3524x_read_data)
       // otp = mag_mmc3524x_read_otp
       //otp_correct = rawx * otp
       int raw[3] = {0};
       //int otp[3] = {0};
       int ret = 0;
        
       ret = mag_mmc3524x_read_data(d,raw);
       if(ret < 0) {
             dev_err(&d->client->dev,"mag_mmc3524x_read_data error code = %d",ret);
             return ret;
       }
       if(ret < 0) {
             dev_err(&d->client->dev,"%s read otp error code = %d",__func__,ret);
             return ret;
       }
       xyz[0] = raw[0] * int_otp_x;
       xyz[1] = raw[1] * int_otp_y;
       xyz[2] = raw[2] * int_otp_z;
        
       DEBUG_SOFIA_VERBOSE(&d->client->dev,"%s: x = %d , y = %d , z = %d",__func__,xyz[0],xyz[1],xyz[2]);
       return 0;
}

#else
static int mmc3524x_get_data(struct mmc3524x *d, int xyz[])
{
	unsigned char data[16] = {0};
	/* x,y,z hardware data */
	int raw[3] = { 0 };
	int hw_d[3] = { 0 };

	/* read xyz raw data */
	d->read_idx++;
	data[0] = MMC3524X_REG_DATA;
	if (mmc3xxx_i2c_rx_data(d, data, 6) < 0)
		return -EFAULT;

	raw[0] = data[1] << 8 | data[0];
	raw[1] = data[3] << 8 | data[2];
	raw[2] = data[5] << 8 | data[4];
	raw[2] = 65536 - raw[2];
	
	DEBUG_SOFIA_VERBOSE(&d->client->dev,"3416_raw [X - %04x] [Y - %04x] [Z - %04x]\n", 
			raw[0], raw[1], raw[2]);
	/* axis map */
	hw_d[0] = ((d->p.negate_x) ?
			(-raw[d->p.axis_map_x]) : (raw[d->p.axis_map_x]));
	hw_d[1] = ((d->p.negate_y) ?
			(-raw[d->p.axis_map_y]) : (raw[d->p.axis_map_y]));
	hw_d[2] = ((d->p.negate_z) ? (-raw[d->p.axis_map_z])
	   : (raw[d->p.axis_map_z]));
	/* data unit is mGaus */
	xyz[0] = (hw_d[0] - MMC3524X_OFFSET) * 1000 / MMC3524X_SENSITIVITY;
	xyz[1] = (hw_d[1] - MMC3524X_OFFSET) * 1000 / MMC3524X_SENSITIVITY;
	xyz[2] = (hw_d[2] - MMC3524X_OFFSET) * 1000 / MMC3524X_SENSITIVITY;

	DEBUG_SOFIA_VERBOSE(&d->client->dev, "x =   %d   y =   %d   z =   %d\n",
			xyz[0], xyz[1], xyz[2]);
	return 0;
}

#endif

static void mmc3524x_work_func(struct work_struct *work)
{
	int xyz[3] = { 0 };
	struct mmc3524x *d;

	d = container_of((struct delayed_work *)work, struct mmc3524x, work);
        DEBUG_SOFIA_VERBOSE(&d->client->dev,"%s",__func__);
	mutex_lock(&ecompass_lock);

	/* check mmc3524x_enabled staues first */
	if (d->enabled == 0) {
		mutex_unlock(&ecompass_lock);
		return;
	}
	/* read data */
	if (mmc3524x_get_data(d, xyz) < 0)
         	dev_err(&d->client->dev, "get_data failed\n");
	/* report data */
	input_report_rel(d->indev, REL_X, xyz[0]);
	input_report_rel(d->indev, REL_Y, xyz[1]);
	input_report_rel(d->indev, REL_Z, xyz[2]);
	input_sync(d->indev);
	//pr_info("%s: x-%d, y-%d,z-%d",__func__,xyz[0],xyz[1],xyz[2]);
	schedule_delayed_work(&d->work, msecs_to_jiffies(d->poll_ms));
	mutex_unlock(&ecompass_lock);
}

static int mmc3524x_input_init(struct mmc3524x *d)
{
	int status;
	int err = -1;

	d->indev = input_allocate_device();
	if (!d->indev) {
		err = -ENOMEM;
		dev_err(&d->client->dev, "input device allocate failed\n");
		return err;
	}
	d->indev->name = "mmc3416x_mag";//MMC3524X_NAME;
	d->indev->id.bustype = BUS_I2C;
	d->indev->dev.parent = &d->client->dev;
	input_set_drvdata(d->indev, d);
	/*
	 * Driver use EV_REL event to report data to user space
	 * instead of EV_ABS. Because EV_ABS event will be ignored
	 * if current input has same value as former one. which effect
	 * data smooth
	 */
	set_bit(EV_REL, d->indev->evbit);
	set_bit(REL_X, d->indev->relbit);
	set_bit(REL_Y, d->indev->relbit);
	set_bit(REL_Z, d->indev->relbit);

	status = input_register_device(d->indev);
	if (status) {
		dev_dbg(&d->client->dev,
			"error registering input device\n");
		input_free_device(d->indev);
		return err;
	}

	return 0;
}

static int mmc3524x_hardware_init(struct mmc3524x *d)
{
	unsigned char data[16] = {0};
        int ret = 0;
	data[0] = MMC3524X_REG_CTRL;
	data[1] = MMC3524X_CTRL_REFILL;
	if (mmc3xxx_i2c_tx_data(d, data, 2) < 0)
		dev_err(&d->client->dev, "i2c tx err at %d\n", __LINE__);

	msleep(MMC3524X_DELAY_SET);

	data[0] = MMC3524X_REG_CTRL;
	data[1] = MMC3524X_CTRL_RESET;
	if (mmc3xxx_i2c_tx_data(d, data, 2) < 0)
		dev_err(&d->client->dev, "tx err.\n");
	mdelay(1);
	data[0] = MMC3524X_REG_CTRL;
	data[1] = 0;
	if (mmc3xxx_i2c_tx_data(d, data, 2) < 0)
		dev_err(&d->client->dev, "tx err.\n");
	msleep(MMC3524X_DELAY_SET);

	data[0] = MMC3524X_REG_BITS;
	data[1] = MMC3524X_BITS_SLOW_16;
	if (mmc3xxx_i2c_tx_data(d, data, 2) < 0)
		dev_err(&d->client->dev, "tx err.\n");
	msleep(MMC3524X_DELAY_TM);

	data[0] = MMC3524X_REG_CTRL;
	data[1] = MMC3524X_CTRL_TM;
	if (mmc3xxx_i2c_tx_data(d, data, 2) < 0)
		dev_err(&d->client->dev, "tx err.\n");
	msleep(MMC3524X_DELAY_TM);
#ifdef INTEL_SOFIA
        ret = mag_mmc3524x_read_otp(d);
        dev_err(&d->client->dev,"%s,%d",__func__,__LINE__);
        return ret;

#endif

	return 0;
}

static inline int mag_set_pinctrl_state(struct device *dev,
		struct pinctrl_state *state)
{
	struct mmc3524x *d = dev_get_drvdata(dev);
	int ret = 0;

	if (!IS_ERR_OR_NULL(d)) {
		if (!IS_ERR_OR_NULL(&d->p)) {
			dev_err(&d->client->dev, "Unable to retrieve  platform data\n");
			return -EINVAL;
		}
		ret = pinctrl_select_state(d->p.pinctrl, state);
		if (ret)
			dev_err(&d->client->dev,
					"%d:could not set pins\n", __LINE__);
	}
	return ret;
}

#ifdef CONFIG_OF
static int mag_power_on(struct device *dev)
{
	struct mmc3524x *d = dev_get_drvdata(dev);
	int err = device_state_pm_set_state_by_name(dev,
			d->p.pm_platdata->pm_state_D0_name);
	if (err)
		dev_err(&d->client->dev, "Power ON Failed: %d\n", err);

	return err;
}

static int mag_power_off(struct device *dev)
{
	struct mmc3524x *d = dev_get_drvdata(dev);
	int err = device_state_pm_set_state_by_name(dev,
			d->p.pm_platdata->pm_state_D3_name);
	if (err)
		dev_err(&d->client->dev, "Power OFF Failed:\n");

	return err;
}

static int mag_init(struct device *dev)
{
	struct mmc3524x *d = dev_get_drvdata(dev);
	if (!d->p.pm_platdata ||
	!d->p.pm_platdata->pm_state_D0_name ||
	!d->p.pm_platdata->pm_state_D3_name)
		return -EINVAL;

	return device_state_pm_set_class(dev,
			d->p.pm_platdata->pm_user_name);
}

static void mag_exit(void)
{
	return;
}
#endif

static int mmc3524x_get_platdata(
		struct mmc3524x *d)
{
#ifdef CONFIG_OF

#define OF_AXIS_MAP		"intel,axis-map"
#define OF_NEGATE		"intel,negate"
#define OF_POLL_INTERVAL	"intel,poll-interval"

	u32 out_values[3];
	struct device_node *np = d->client->dev.of_node;

	/* pinctrl */
	d->p.pinctrl = devm_pinctrl_get(&d->client->dev);
	if (IS_ERR(d->p.pinctrl))
		goto skip_pinctrl;

	d->p.pins_default = pinctrl_lookup_state(d->p.pinctrl,
			PINCTRL_STATE_DEFAULT);
	if (IS_ERR(d->p.pins_default))
		dev_err(&d->client->dev, "could not get default pinstate\n");

	d->p.pins_sleep = pinctrl_lookup_state(d->p.pinctrl,
			PINCTRL_STATE_SLEEP);
	if (IS_ERR(d->p.pins_sleep))
		dev_err(&d->client->dev, "could not get sleep pinstate\n");

	d->p.pins_inactive = pinctrl_lookup_state(d->p.pinctrl,
			"inactive");
	if (IS_ERR(d->p.pins_inactive))
		dev_err(&d->client->dev, "could not get inactive pinstate\n");

skip_pinctrl:

	/* Axis map properties */
	if (of_property_read_u32_array(np, OF_AXIS_MAP, out_values, 3) < 0) {
		dev_err(&d->client->dev, "Error parsing %s property of node %s\n",
			OF_AXIS_MAP, np->name);
		goto out;
	}
	d->p.axis_map_x = (u8)out_values[0];
	d->p.axis_map_y = (u8)out_values[1];
	d->p.axis_map_z = (u8)out_values[2];

	/* Negate properties */
	if (of_property_read_u32_array(np, OF_NEGATE, out_values, 3) < 0) {
		dev_err(&d->client->dev, "Error parsing %s property of node %s\n",
			OF_NEGATE, np->name);
		goto out;
	}
	d->p.negate_x = (u8)out_values[0];
	d->p.negate_y = (u8)out_values[1];
	d->p.negate_z = (u8)out_values[2];

	/* Poll interval property */
	if (of_property_read_u32(np, OF_POLL_INTERVAL,
			&d->p.poll_interval) < 0) {
		dev_err(&d->client->dev, "Error parsing %s property of node %s\n",
			OF_POLL_INTERVAL, np->name);
		goto out;
	}

	/* device pm */
	d->p.pm_platdata = of_device_state_pm_setup(np);
	if (IS_ERR(d->p.pm_platdata)) {
		dev_err(&d->client->dev, "Error during device state pm init\n");
		goto out;
	}

	d->p.init = mag_init;
	d->p.exit = mag_exit;
	d->p.power_on = mag_power_on;
	d->p.power_off = mag_power_off;

#else
	d->p.axis_map_x = 0;
	d->p.axis_map_y = 1;
	d->p.axis_map_z = 2;
	d->p.negate_x = 0;
	d->p.negate_y = 0;
	d->p.negate_z = 0;
	d->p.poll_interval = 200;
#endif

	return 0;

out:
	return -EINVAL;
}

#ifdef CONFIG_PM
static int mag_suspend(struct device *dev)
{
	struct mmc3524x *d = dev_get_drvdata(dev);
	if (d->enabled)
		d->enabled_before_suspend = 1;
	else
		d->enabled_before_suspend = 0;

	mmc3524x_mag_disable(d);
#ifndef INTEL_CHT_PM_WR
	d->p.power_off(&d->indev->dev);
	mag_set_pinctrl_state(dev, d->p.pins_sleep);
#endif
	return 0;
}

static int mag_resume(struct device *dev)
{
	struct mmc3524x *d = dev_get_drvdata(dev);
	if (d->enabled_before_suspend) {
#ifndef INTEL_CHT_PM_WR
		mag_set_pinctrl_state(dev, d->p.pins_default);
		d->p.power_on(&d->indev->dev);
#endif
		mmc3524x_mag_enable(d);
	}

	return 0;
}
#else
#define mag_suspend NULL
#define mag_resume NULL
#endif

static const struct dev_pm_ops mmc3524x_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(mag_suspend, mag_resume)
};

static int mmc3524x_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{

	int res = 0;
	struct mmc3524x *d;
	/*struct device *dev_t;*/

	/* check i2c */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: functionality check failed\n", __func__);
		res = -ENODEV;
		goto out;
	}
	/* allocate driver data */
	d = kzalloc(sizeof(*d), GFP_KERNEL);
	if (d == NULL) {
		dev_err(&client->dev, "failed to allocate memory\n");
		res = -ENOMEM;
		goto out;
	}
        /* new addrs*/
        client->addr = 0x30;

	d->client = client;

	/* platform data */
	if (mmc3524x_get_platdata(d))
		goto out_0;

	i2c_set_clientdata(client, d);

	/* check hardware */
	if (mmc3524x_hardware_init(d))
		goto out_0;

	/* input dev */
	if (mmc3524x_input_init(d))
		goto out_0;

	if (d->p.init)
		if (d->p.init(&d->indev->dev))
			goto out_0;
	if (d->p.power_on)
		if (d->p.power_on(&d->indev->dev))
			goto out_0;

	/* init delayed work for polling mode */
	d->poll_ms = 200;
	INIT_DELAYED_WORK(&d->work, mmc3524x_work_func);

	/* sysfs interface */
	res = create_sysfs_interfaces(&client->dev);
	if (res < 0) {
		dev_err(&client->dev, "%s device register failed\n",
				MMC3524X_NAME);
		goto out_2;
	}

	/* set init status and power off before exit */
	mutex_init(&d->lock);
	mmc3524x_mag_disable(d);
#ifndef INTEL_CHT_PM_WR 
	mag_set_pinctrl_state(&client->dev, d->p.pins_default);

	if (d->p.power_off)
		d->p.power_off(&client->dev);
#endif
	dev_info(&client->dev, "%s probed successfully\n", MMC3524X_NAME);

	return 0;

out_2:
	input_unregister_device(d->indev);
	input_free_device(d->indev);
#if 0
out_1:
	misc_deregister(&mmc3524x_device);
#endif
out_0:
	mag_set_pinctrl_state(&client->dev, d->p.pins_inactive);
	kfree(d);
out:
	return res;
}


static int mmc3524x_remove(struct i2c_client *client)
{
	struct mmc3524x *d = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "%s\n", __func__);
	if (d->p.power_off)
		d->p.power_off(&client->dev);
	if (d->p.exit)
		d->p.exit();

	mag_set_pinctrl_state(&client->dev, d->p.pins_inactive);
	cancel_delayed_work(&d->work);
	/*misc_deregister(&mmc3524x_device);*/
	input_unregister_device(d->indev);
	input_free_device(d->indev);
	remove_sysfs_interfaces(&client->dev);
	kfree(d);

	return 0;
}



static const struct i2c_device_id mmc3524x_id[] = {
	{ MMC3524X_NAME, 0 },
	{ }
};

#ifdef CONFIG_ACPI
static const struct acpi_device_id compass_acpi_match[] = {
        {"MMCP3524", 0},
        { }
};
#endif

static struct i2c_driver mmc3524x_driver = {
	.probe		= mmc3524x_probe,
	.remove		= mmc3524x_remove,
	.id_table	= mmc3524x_id,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= MMC3524X_NAME,
		.pm = &mmc3524x_pm,
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(compass_acpi_match),
#endif
	},
};

static int __init mmc3524x_init(void)
{
	return i2c_add_driver(&mmc3524x_driver);
}

static void __exit mmc3524x_exit(void)
{
	i2c_del_driver(&mmc3524x_driver);
}

module_init(mmc3524x_init);
module_exit(mmc3524x_exit);

MODULE_DESCRIPTION("MEMSIC MMC3524X Magnetic Sensor Driver");
MODULE_LICENSE("GPL");
