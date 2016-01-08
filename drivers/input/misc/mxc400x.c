/******************** (C) COPYRIGHT 2015 Memsic Inc. **************************
 *
 * File Name          : mxc400x_acc.c
 * Description        : MXC400X accelerometer sensor API
 *
 *******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
 * PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
 *

 ******************************************************************************/

#include	<linux/err.h>
#include	<linux/errno.h>
#include	<linux/delay.h>
#include	<linux/fs.h>
#include	<linux/i2c.h>
#include <linux/module.h>
#include	<linux/input.h>
#include	<linux/input-polldev.h>
#include	<linux/miscdevice.h>
#include	<linux/uaccess.h>
#include  <linux/slab.h>

#include	<linux/workqueue.h>
#include	<linux/irq.h>
#include	<linux/gpio.h>
#include	<linux/interrupt.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include        <linux/earlysuspend.h>
#endif

#include "mxc400x.h"

#ifdef CONFIG_ACPI
#include <linux/acpi.h>
#endif

#define SENSOR_BASE_NUM 0
#define SENSOR_SHOW_SIZE 100
#define	I2C_RETRY_DELAY		5
#define	I2C_RETRIES		5

#define DEVICE_INFO         "Memsic, MXC400X"
#define DEVICE_INFO_LEN     32

/* end RESUME STATE INDICES */

#define DEBUG    1
#define MXC400X_DEBUG  1 

#define	MAX_INTERVAL	50

#ifdef __KERNEL__
static struct mxc400x_acc_platform_data mxc400x_plat_data = {
    .poll_interval = 20,
    .min_interval = 10,
};
#endif

#ifdef I2C_BUS_NUM_STATIC_ALLOC
static struct i2c_board_info  mxc400x_i2c_boardinfo = {
        I2C_BOARD_INFO(MXC400X_ACC_DEV_NAME, MXC400X_ACC_I2C_ADDR),
#ifdef __KERNEL__
        .platform_data = &mxc400x_plat_data
#endif
};
#endif

struct mxc400x_acc_data {
	struct i2c_client *client;
	struct mxc400x_acc_platform_data *pdata;

	struct mutex lock;
	struct delayed_work input_work;

	struct input_dev *input_dev;

	int hw_initialized;
	/* hw_working=-1 means not tested yet */
	int hw_working;
	atomic_t enabled;
	int on_before_suspend;

#ifdef CONFIG_HAS_EARLYSUSPEND
        struct early_suspend early_suspend;
#endif
};

/*
 * Because misc devices can not carry a mxc400x from driver register to
 * open, we keep this global.  This limits the driver to a single instance.
 */
struct mxc400x_acc_data *mxc400x_acc_misc_data;
struct i2c_client      *mxc400x_i2c_client;
//int mxc400x_x = 0;
//int mxc400x_y = 0;
static int mxc400x_z = 0;

static int mxc400x_acc_i2c_read(struct mxc400x_acc_data *acc, u8 * buf, int len)
{
	int err;
	int tries = 0;

	struct i2c_msg	msgs[] = {
		{
			.addr = acc->client->addr,
			.flags = 0/*acc->client->flags & I2C_M_TEN*/,
			.len = 1,
			.buf = buf, },
		{
			.addr = acc->client->addr,
			.flags = /*(acc->client->flags & I2C_M_TEN) |*/ I2C_M_RD,
			.len = len,
			.buf = buf, },
	};

	do {
		err = i2c_transfer(acc->client->adapter, msgs, 2);
		if (err != 2)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 2) && (++tries < I2C_RETRIES));

	if (err != 2) {
		dev_err(&acc->client->dev, "read transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	
	return err;
}

static int mxc400x_acc_i2c_write(struct mxc400x_acc_data *acc, u8 * buf, int len)
{
	int err;
	int tries = 0;

	struct i2c_msg msgs[] = { { .addr = acc->client->addr,
			.flags = 0/*acc->client->flags & I2C_M_TEN*/,
			.len = len + 1, .buf = buf, }, };
	do {
		err = i2c_transfer(acc->client->adapter, msgs, 1);
		if (err != 1)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 1) && (++tries < I2C_RETRIES));

	if (err != 1) {
		dev_err(&acc->client->dev, "write transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}

static int mxc400x_acc_hw_init(struct mxc400x_acc_data *acc)
{
	int err = -1;
	u8 buf[7] = {0};

	printk(KERN_INFO "%s: hw init start\n", MXC400X_ACC_DEV_NAME);

	buf[0] = WHO_AM_I;
	err = mxc400x_acc_i2c_read(acc, buf, 1);
	if (err < 0)
		goto error_firstread;
	else
		acc->hw_working = 1;
	if ((buf[0] & 0x3F) != WHOAMI_MXC400X_ACC) {
		err = -1; /* choose the right coded error */
		goto error_unknown_device;
	}

	acc->hw_initialized = 1;
	printk(KERN_INFO "%s: hw init done\n", MXC400X_ACC_DEV_NAME);
	return 0;

error_firstread:
	acc->hw_working = 0;
	dev_warn(&acc->client->dev, "Error reading WHO_AM_I: is device "
		"available/working?\n");
	goto error1;
error_unknown_device:
	dev_err(&acc->client->dev,
		"device unknown. Expected: 0x%x,"
		" Replies: 0x%x\n", WHOAMI_MXC400X_ACC, buf[0]);
error1:
	acc->hw_initialized = 0;
	dev_err(&acc->client->dev, "hw init error 0x%x,0x%x: %d\n", buf[0],
			buf[1], err);
	return err;
}

static void mxc400x_acc_device_power_off(struct mxc400x_acc_data *acc)
{
	int err;
	u8 buf[2] = { MXC400X_REG_CTRL, MXC400X_CTRL_PWRDN };

	err = mxc400x_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		dev_err(&acc->client->dev, "soft power off failed: %d\n", err);
}

static int mxc400x_acc_device_power_on(struct mxc400x_acc_data *acc)
{
    
	int err = -1;
	u8 buf[2] = {MXC400X_REG_FAC, MXC400X_PASSWORD};
	printk(KERN_INFO "%s: %s entry\n",MXC400X_ACC_DEV_NAME, __func__);
	//+/-2G
/*	
	err = mxc400x_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		dev_err(&acc->client->dev, "soft factory password write failed: %d\n", err);
		
	buf[0] = MXC400X_REG_FSRE;
	buf[1] = MXC400X_RANGE_8G;
	err = mxc400x_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		dev_err(&acc->client->dev, "soft change to +/8g failed: %d\n", err);
*/	
	buf[0] = MXC400X_REG_CTRL;
	err = mxc400x_acc_i2c_read(acc, buf, 1);
	if (err < 0){
		dev_err(&acc->client->dev,"read ctrl reg failed\n");
	}else{
		dev_info(&acc->client->dev,"CTRL reg %#x\n", buf[0]);
	}
	buf[0] = MXC400X_REG_CTRL;
	buf[1] = MXC400X_CTRL_PWRON;
	err = mxc400x_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		dev_err(&acc->client->dev, "soft power on failed: %d\n", err);
	msleep(300);
	if (!acc->hw_initialized) {
		err = mxc400x_acc_hw_init(acc);
		if (acc->hw_working == 1 && err < 0) {
			mxc400x_acc_device_power_off(acc);
			return err;
		}
	}

	return 0;
}

static int mxc400x_acc_register_read(struct mxc400x_acc_data *acc, u8 *buf,
		u8 reg_address)
{

	int err = -1;
	buf[0] = (reg_address);
	err = mxc400x_acc_i2c_read(acc, buf, 1);
	return err;
}

/* */

static int mxc400x_acc_get_acceleration_data(struct mxc400x_acc_data *acc,
		int *xyz)
{
	int err = -1;
	int tmp;
	u8  tempture = 0; 
//	int mxc400x_x = 0;
//	int mxc400x_y = 0;
	/* Data bytes from hardware x, y,z */
	unsigned char acc_data[MXC400X_DATA_LENGTH] = {0, 0, 0, 0, 0, 0};
   
	acc_data[0] = MXC400X_REG_X;
	err = mxc400x_acc_i2c_read(acc, acc_data, MXC400X_DATA_LENGTH);
    																		
	if (err < 0)
    {
        #ifdef MXC400X_DEBUG
        printk(KERN_INFO "%s I2C read xy error %d\n", MXC400X_ACC_DEV_NAME, err);
        #endif
		return err;
    }
    
	err = mxc400x_acc_register_read(acc, &tempture, MXC400X_REG_TEMP);                

    xyz[0] = (signed short)(acc_data[0] << 8 | acc_data[1]) >> 4;
	xyz[1] = (signed short)(acc_data[2] << 8 | acc_data[3]) >> 4;
	tmp = xyz[0] * 4;
	xyz[0] = -xyz[1] * 4;
	
    xyz[1] = tmp;
	xyz[2] = (signed short)(acc_data[4] << 8 | acc_data[5]) >> 4;
	xyz[2] = xyz[2] * 4;
	xyz[3] = tempture;
//        xyz[2] = -xyz[2];
//	mxc400x_x = xyz[0];
//	mxc400x_y = xyz[1];
	#ifdef MXC400X_DEBUG
	// printk(KERN_INFO "%s chenbowei123 read x=%d, y=%d, z=%d\n",MXC400X_ACC_DEV_NAME, xyz[0], xyz[1], xyz[2]);
	#endif
	return err;
}

static void mxc400x_acc_report_values(struct mxc400x_acc_data *acc, int *xyz)
{
	input_report_rel(acc->input_dev, REL_X, xyz[0]);
	input_report_rel(acc->input_dev, REL_Y, xyz[1]);
	input_report_rel(acc->input_dev, REL_Z, xyz[2]);
	input_report_abs(acc->input_dev, ABS_HAT0X, xyz[3]);
	input_sync(acc->input_dev);
	/* printk("X %d Y %d Z %d  T %d !\n", xyz[0],xyz[1],xyz[2],xyz[3]); */
}

static int mxc400x_acc_enable(struct mxc400x_acc_data *acc)
{
	int err;
    	printk("mxc400x_acc_enable\n");
	if (!atomic_cmpxchg(&acc->enabled, 0, 1)) {
		err = mxc400x_acc_device_power_on(acc);
		
		if (err < 0) {
			atomic_set(&acc->enabled, 0);
			return err;
		}

		schedule_delayed_work(&acc->input_work, msecs_to_jiffies(
				acc->pdata->poll_interval));
	}

	return 0;
}

static int mxc400x_acc_disable(struct mxc400x_acc_data *acc)
{
	if (atomic_cmpxchg(&acc->enabled, 1, 0)) {
		cancel_delayed_work_sync(&acc->input_work);
		mxc400x_acc_device_power_off(acc);
	}

	return 0;
}

static void mxc400x_acc_input_work_func(struct work_struct *work)
{
	struct mxc400x_acc_data *acc;

	int xyz[4] = { 0 };
	int err;

	acc = container_of((struct delayed_work *)work,
			struct mxc400x_acc_data,	input_work);

	mutex_lock(&acc->lock);
	schedule_delayed_work(&acc->input_work, msecs_to_jiffies(
            acc->pdata->poll_interval));

	err = mxc400x_acc_get_acceleration_data(acc, xyz);
	if (err < 0)
	{
		dev_err(&acc->client->dev, "get_acceleration_data failed %d!\n", err);
		cancel_delayed_work(&acc->input_work);
	}
	else
		mxc400x_acc_report_values(acc, xyz);

	mutex_unlock(&acc->lock);
}

static int mxc400x_acc_validate_pdata(struct mxc400x_acc_data *acc)
{
	acc->pdata->poll_interval = max(acc->pdata->poll_interval,
			acc->pdata->min_interval);

	/* Enforce minimum polling interval */
	if (acc->pdata->poll_interval < acc->pdata->min_interval) {
		dev_err(&acc->client->dev, "minimum poll interval violated\n");
		return -EINVAL;
	}

	return 0;
}

static int mxc400x_acc_input_init(struct mxc400x_acc_data *acc)
{
	int res = 0;
	INIT_DELAYED_WORK(&acc->input_work, mxc400x_acc_input_work_func);
	pr_info("ptaccelerator driver: init\n");
	acc->input_dev = input_allocate_device();
	if (!acc->input_dev) {
		res = -ENOMEM;
		pr_err("%s: failed to allocate input device\n", __FUNCTION__);
		goto out;
	}

	input_set_drvdata(acc->input_dev, acc);
	set_bit(EV_ABS, acc->input_dev->evbit);
	input_set_abs_params(acc->input_dev, ABS_HAT0X, -2047, 2047, 0, 0);
	/* ptacceleration status, 0 ~ 3 */
	input_set_abs_params(acc->input_dev, ABS_WHEEL, 
		0, 100, 0, 0);

	/*
	 * Driver use EV_REL event to report data to user space
	 * instead of EV_ABS. Because EV_ABS event will be ignored
	 * if current input has same value as former one. which effect
	 * data smooth
	 */
	set_bit(EV_REL, acc->input_dev->evbit);
	set_bit(REL_X, acc->input_dev->relbit);
	set_bit(REL_Y, acc->input_dev->relbit);
	set_bit(REL_Z, acc->input_dev->relbit);

	acc->input_dev->name = MXC400X_ACC_INPUT_NAME;
	res = input_register_device(acc->input_dev);
	if (res) {
		pr_err("%s: unable to register input device: %s\n",
			__FUNCTION__, acc->input_dev->name);
		goto out_free_input;
	}
	
	return 0;

out_free_input:
	input_free_device(acc->input_dev);
out:
	return res;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxc400x_early_suspend (struct early_suspend* es);
static void mxc400x_early_resume (struct early_suspend* es);
#endif

static ssize_t mxc400x_enable_show(struct device *dev,
				  struct device_attribute *attr, char *buf){
	struct mxc400x_acc_data *acc = dev_get_drvdata(dev);
	int val;
	if (NULL == buf)
		return -EINVAL;
	val = atomic_read(&acc->enabled) ? 1 : 0;
	return scnprintf(buf, SENSOR_SHOW_SIZE, "%d\n", val);
}

static ssize_t mxc400x_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count){
	int val = 0;
	int err = 0;
	struct mxc400x_acc_data *acc = dev_get_drvdata(dev);
	if (NULL == buf)
		return -EINVAL;

	if (0 == count)
		return 0;

	if (strict_strtol(buf, SENSOR_BASE_NUM, (long *)&val))
		return -EINVAL;
	dev_info(dev, "%s: val = %d", __func__, val);

	val = val ? 1 : 0;
	
	mutex_lock(&acc->lock);
	if(val){
		err = mxc400x_acc_enable(acc);
	}else{
		mxc400x_acc_disable(acc);
	}
	mutex_unlock(&acc->lock);
	
	return count;

}

static ssize_t mxc400x_delay_show(struct device *dev,
				  struct device_attribute *attr, char *buf){
	struct mxc400x_acc_data *acc = dev_get_drvdata(dev);
	int val = 0;
	val = acc->pdata->poll_interval;

	return scnprintf(buf, SENSOR_SHOW_SIZE, "%d\n", val);
}

static ssize_t mxc400x_delay_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count){
	int val = 0;
	int ret;
	struct mxc400x_acc_data *acc = dev_get_drvdata(dev);
	
	if (NULL == buf)
		return -EINVAL;

	if (0 == count)
		return 0;

	if (strict_strtol(buf, SENSOR_BASE_NUM, (long *)&val))
		return -EINVAL;
	if(val < 0)
		return -EINVAL;
	dev_info(dev, "%s: val = %d", __func__, val);

	val = val > acc->pdata->min_interval ? val : acc->pdata->min_interval;
	acc->pdata->poll_interval = val;

	return count;
}


static struct device_attribute attributes[] = {
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP, mxc400x_enable_show,
	       mxc400x_enable_store),

	__ATTR(delay, S_IRUGO | S_IWUSR | S_IWGRP,
	       mxc400x_delay_show, mxc400x_delay_store),
};

static int mxc400x_register_interface(struct device *dev){
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto error;
	return 0;

error:
	for (; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return -1;
}

static int mxc400x_unregister_interface(struct device *dev){
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++){
		device_remove_file(dev, attributes + i);
	}
	return 0;
}


static int mxc400x_acc_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{

	struct mxc400x_acc_data *acc;
	struct mxc400x_acc_platform_data *pdata = client->dev.platform_data;

	int err = -1;
	int tempvalue;

	printk("%s: probe start!\n", MXC400X_ACC_DEV_NAME);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "client not i2c capable\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}
	else 
	// {printk("%s, if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))  =====OK chen !!!!! \n", __func__);}

	if (!i2c_check_functionality(client->adapter,
					I2C_FUNC_SMBUS_BYTE |
					I2C_FUNC_SMBUS_BYTE_DATA |
					I2C_FUNC_SMBUS_WORD_DATA)) {
		dev_err(&client->dev, "client not smb-i2c capable:2\n");
		err = -EIO;
		goto exit_check_functionality_failed;
	}
	else
	// {printk("%s, if (!i2c_check_functionality(client->adapter,  =====OK chen !!!!! \n", __func__);}


	if (!i2c_check_functionality(client->adapter,
						I2C_FUNC_SMBUS_I2C_BLOCK)){
		dev_err(&client->dev, "client not smb-i2c capable:3\n");
		err = -EIO;
		goto exit_check_functionality_failed;
	}
	else 
	// {printk("%s, if (!i2c_check_functionality(client->adapter,I2C_FUNC_SMBUS_I2C_BLOCK))  =====OK chen !!!!! \n", __func__);}
	/*
	 * OK. From now, we presume we have a valid client. We now create the
	 * client structure, even though we cannot fill it completely yet.
	 */

	acc = kzalloc(sizeof(struct mxc400x_acc_data), GFP_KERNEL);
	if (acc == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for module data: "
					"%d\n", err);
		goto exit_alloc_data_failed;
	}
	else 
	// {printk("%s, if (acc == NULL)  =====OK chen !!!!! \n", __func__);}
	mutex_init(&acc->lock);
	mutex_lock(&acc->lock);
		
	acc->client = client;
	mxc400x_i2c_client = client;
	i2c_set_clientdata(client, acc);

	/* read chip id */
	tempvalue = i2c_smbus_read_word_data(client, WHO_AM_I);
	dev_info(&client->dev, "Device 0x%x\n", tempvalue);
	
	acc->pdata = kmalloc(sizeof(*acc->pdata), GFP_KERNEL);
	if (acc->pdata == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for pdata: %d\n",
				err);
		goto exit_kfree_pdata;
	}
	else 
		memcpy(acc->pdata, &mxc400x_plat_data, sizeof(*acc->pdata));

	err = mxc400x_acc_validate_pdata(acc);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto exit_kfree_pdata;
	}
	else 
	// {printk("%s, err = mxc400x_acc_validate_pdata(acc); if (err < 0)   =====OK chen !!!!! \n", __func__);}

	i2c_set_clientdata(client, acc);


	if (acc->pdata->init) {
		err = acc->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err2;
		}
	}
	else 
	// {printk("%s, if (acc->pdata->init)   =====OK chen !!!!! \n", __func__);}

	err = mxc400x_acc_device_power_on(acc);
	if (err < 0) {
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err2;
	}
	else 
	// {printk("%s, err = mxc400x_acc_device_power_on(acc); if (err < 0)   =====OK chen !!!!! \n", __func__);}
	
	atomic_set(&acc->enabled, 1);

	err = mxc400x_acc_input_init(acc);
	if (err < 0) {
		dev_err(&client->dev, "input init failed\n");
		goto err_power_off;
	}
	mxc400x_acc_misc_data = acc;

	err = mxc400x_register_interface(&client->dev);
	if(err < 0){
		dev_err(&client->dev, "register_interface error");
		goto exit_kfree_pdata;
	}

	/*Yahua change begin
	err = misc_register(&mxc400x_acc_misc_device);
	if (err < 0) {
		dev_err(&client->dev,
				"misc MXC400X_ACC_DEV_NAME register failed\n");
		goto exit_kfree_pdata;
	}
	else 
	// {printk("%s, 	err = misc_register(&mxc400x_acc_misc_device); if (err < 0)   =====OK chen !!!!! \n", __func__);}
	Yahua change end*/
	
	mxc400x_acc_device_power_off(acc);

	/* As default, do not report information */
	atomic_set(&acc->enabled, 0);

    acc->on_before_suspend = 0;

 #ifdef CONFIG_HAS_EARLYSUSPEND
    acc->early_suspend.suspend = mxc400x_early_suspend;
    acc->early_suspend.resume  = mxc400x_early_resume;
    acc->early_suspend.level   = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    register_early_suspend(&acc->early_suspend);
#endif

	mutex_unlock(&acc->lock);

	dev_info(&client->dev, "%s: probed\n", MXC400X_ACC_DEV_NAME);
	printk("mxc400x_acc_probe OK!\n");
	return 0;


err2:
	if (acc->pdata->exit) acc->pdata->exit();
exit_kfree_pdata:
	kfree(acc->pdata);
err_power_off:
    mxc400x_acc_device_power_off(acc);
err_mutexunlockfreedata:
	kfree(acc);
 	mutex_unlock(&acc->lock);
    i2c_set_clientdata(client, NULL);
    mxc400x_acc_misc_data = NULL;
exit_alloc_data_failed:
exit_check_functionality_failed:
	printk(KERN_ERR "%s: Driver Init failed\n", MXC400X_ACC_DEV_NAME);
	return err;
}

static int /*__devexit*/ mxc400x_acc_remove(struct i2c_client *client)
{
	/* TODO: revisit ordering here once _probe order is finalized */
	struct mxc400x_acc_data *acc = i2c_get_clientdata(client);
	
    	mxc400x_acc_device_power_off(acc);
		mxc400x_unregister_interface(&client->dev);
    	if (acc->pdata->exit)
    		acc->pdata->exit();
    	kfree(acc->pdata);
    	kfree(acc);
    
	return 0;
}

static int mxc400x_acc_resume(struct i2c_client *client)
{
	struct mxc400x_acc_data *acc = i2c_get_clientdata(client);
#ifdef MXC400X_DEBUG
    printk("%s.\n", __func__);
#endif

	if (acc != NULL && acc->on_before_suspend) {
        acc->on_before_suspend = 0;
		return mxc400x_acc_enable(acc);
    }

	return 0;
}

static int mxc400x_acc_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct mxc400x_acc_data *acc = i2c_get_clientdata(client);
#ifdef MXC400X_DEBUG
    printk("%s.\n", __func__);
#endif
    if (acc != NULL) {
        if (atomic_read(&acc->enabled)) {
            acc->on_before_suspend = 1;
            return mxc400x_acc_disable(acc);
        }
	//	else
		// {printk("%s, if (atomic_read(&acc->enabled))  =====OK chen !!!!! \n", __func__);}
    }
//	else 
	// {printk("%s,  if (acc != NULL)   =====OK chen !!!!! \n", __func__);}
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static void mxc400x_early_suspend (struct early_suspend* es)
{
#ifdef MXC400X_DEBUG
    printk("%s.\n", __func__);
#endif
    mxc400x_acc_suspend(mxc400x_i2c_client,
         (pm_message_t){.event=0});
}

static void mxc400x_early_resume (struct early_suspend* es)
{
#ifdef MXC400X_DEBUG
    printk("%s.\n", __func__);
#endif
    mxc400x_acc_resume(mxc400x_i2c_client);
}

#endif /* CONFIG_HAS_EARLYSUSPEND */

static const struct i2c_device_id mxc400x_acc_id[]
				= { { MXC400X_ACC_DEV_NAME, 0 }, { }, };

MODULE_DEVICE_TABLE(i2c, mxc400x_acc_id);

#ifdef CONFIG_ACPI
static const struct acpi_device_id mxc400x_acpi_match[] = {
	{"MMCX4005", 0},
	{ }
};
#endif

static struct i2c_driver mxc400x_acc_driver = {
	.driver = {
			.name = MXC400X_ACC_I2C_NAME,
			#ifdef CONFIG_ACPI
			.acpi_match_table = ACPI_PTR(mxc400x_acpi_match),
			#endif				
		  },
	.probe = mxc400x_acc_probe,
	.remove = mxc400x_acc_remove,
	.resume = mxc400x_acc_resume,
	.suspend = mxc400x_acc_suspend,
	.id_table = mxc400x_acc_id,
};


#ifdef I2C_BUS_NUM_STATIC_ALLOC

int i2c_static_add_device(struct i2c_board_info *info)
{
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	int err;

	adapter = i2c_get_adapter(I2C_STATIC_BUS_NUM);
	if (!adapter) {
		pr_err("%s: can't get i2c adapter\n", __FUNCTION__);
		err = -ENODEV;
		goto i2c_err;
	}

	client = i2c_new_device(adapter, info);
	if (!client) {
		pr_err("%s:  can't add i2c device at 0x%x\n",
			__FUNCTION__, (unsigned int)info->addr);
		err = -ENODEV;
		goto i2c_err;
	}

	i2c_put_adapter(adapter);

	return 0;

i2c_err:
	return err;
}

#endif /*I2C_BUS_NUM_STATIC_ALLOC*/

static int __init mxc400x_acc_init(void)
{
        int  ret = 0;

    	printk("%s accelerometer driver: init\n",
						MXC400X_ACC_I2C_NAME);
#ifdef I2C_BUS_NUM_STATIC_ALLOC
	 pr_info("mxc400x_acc_init I2C_BUS_NUM_STATIC_ALLOC!\n");
        ret = i2c_static_add_device(&mxc400x_i2c_boardinfo);
        if (ret < 0) {
            pr_err("%s: add i2c device error %d\n", __FUNCTION__, ret);
            goto init_err;
        }
#endif
        ret = i2c_add_driver(&mxc400x_acc_driver);
        printk(KERN_INFO "%s add driver: %d\n",
						MXC400X_ACC_I2C_NAME,ret);
        return ret;

init_err:
        return ret;
}

static void __exit mxc400x_acc_exit(void)
{
	printk(KERN_INFO "%s accelerometer driver exit\n", MXC400X_ACC_DEV_NAME);

	#ifdef I2C_BUS_NUM_STATIC_ALLOC
        i2c_unregister_device(mxc400x_i2c_client);
	#endif

	i2c_del_driver(&mxc400x_acc_driver);
	return;
}

module_init(mxc400x_acc_init);
module_exit(mxc400x_acc_exit);


MODULE_AUTHOR("Robbie Cao<hjcao@memsic.com>");
MODULE_DESCRIPTION("MEMSIC MXC400X (DTOS) Accelerometer Sensor Driver");
MODULE_LICENSE("GPL");
