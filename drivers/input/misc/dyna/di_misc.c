
#include	<linux/err.h>
#include	<linux/errno.h>
#include	<linux/delay.h>
#include	<linux/fs.h>
#include	<linux/i2c.h>

#include	<linux/input.h>
#include	<linux/input-polldev.h>
#include	<linux/miscdevice.h>
#include	<linux/uaccess.h>
#include	<linux/slab.h>
#include	<linux/workqueue.h>
#include	<linux/irq.h>
#include	<linux/gpio.h>
#include	<linux/interrupt.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include	<linux/earlysuspend.h>
#endif
#include	"di_sensor.h"

#include	"di_misc.h"

static struct class* DI_class = NULL;

struct class* get_DI_class(void)
{
	if(!DI_class)
	{
		DI_class = class_create(THIS_MODULE, "DynaImage");
	}

	return DI_class;
}

s64 get_time_ns(void)
{
	struct timespec ts;
	ktime_get_ts(&ts);
	return timespec_to_ns(&ts);
}

int i2c_read(struct i2c_client *client, u8 reg, u8* data, int len)
{
	int err;
	int tries = 0;

	struct i2c_msg	msgs[] = 
	{
		{
			.addr = client->addr,
			.flags = client->flags & I2C_M_TEN,
			.len = 1,
			.buf = &reg, 
		},
		{
			.addr = client->addr,
			.flags = (client->flags & I2C_M_TEN) | I2C_M_RD,
			.len = len,
			.buf = data, 
		},
	};

	do {
		err = i2c_transfer(client->adapter, msgs, 2);
		if (err != 2)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 2) && (++tries < I2C_RETRIES));

	if (err != 2) 
	{
		printk("0x%02X %s error, err = %d \n", client->addr,  __func__,err);
		err = -EIO;
	} 
	else 
	{
		err = 0;
	}

	return err;
}

int i2c_single_write(struct i2c_client *client, u8 reg, u8 data)
{
	int err;
	int tries = 0;
	u8 temp[2];
	struct i2c_msg msg; 

	temp[0] = reg;
	temp[1] = data;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = 2;
	msg.buf = temp;


	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err != 1)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 1) && (++tries < I2C_RETRIES));

	if (err != 1) 
	{
		printk("0x%02X %s error\n",client->addr, __func__);
		err = -EIO;
	}
	else 
	{
		err = 0;
	}

	return err;
}

int i2c_single_write_ignore_ack(struct i2c_client *client, u8 reg, u8 data)
{
	int err = 0;
	int tries = 0;
	u8 temp[2];
	struct i2c_msg msg; 

	temp[0] = reg;
	temp[1] = data;

	msg.addr = client->addr;
	msg.flags = (client->flags & I2C_M_TEN) | I2C_M_IGNORE_NAK;
	msg.len = 2;
	msg.buf = temp;

	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err != 1)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 1) && (++tries < I2C_RETRIES));

	if (err != 1) 
	{
		printk("0x%02X %s error\n",client->addr, __func__);
		err = -EIO;
	}
	else 
	{
		err = 0;
	}

	return err;
}


