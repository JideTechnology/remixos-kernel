/*
 *
 * Electromagnetic Pen I2C Driver for Hanvon
 *
 * Copyright (C) 1999-2012  Hanvon Technology Inc.
 * All Rights Reserved.
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
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include "../../../arch/arm/mach-tegra/gpio-names.h"
#include <mach/gpio-tegra.h>
 
#define CHIP_NAME	"Hanvon0868"
#define MAX_EVENTS	5
#define MAX_X		1280//0x27de
#define MAX_Y		800//0x1cfe


#define DEBUG_I2C_TRANSTER			1
#define MAX_PACKET_SIZE				7

#define UPDATE_SLAVE_ADDR			0x34

#define HW0868_GPIO_RESET			TEGRA_GPIO_PQ6
//#define HW0868_GPIO_CONFIG			TEGRA_GPIO_PU4
//#define HW0868_GPIO_POWER			TEGRA_GPIO_PS2

#define HW0868_CMD_RESET			0x08680000
#define HW0868_CMD_CONFIG_HIGH			0x08680001
#define HW0868_CMD_CONFIG_LOW			0x08680002
#define HW0868_CMD_UPDATE			0x08680003

/**define stylus flags**/
#define PEN_POINTER_UP					0xa0
#define PEN_POINTER_DOWN				0xa1
#define PEN_BUTTON_DOWN					0xa2
#define PEN_BUTTON_UP					0xa3
#define PEN_RUBBER_UP					0xa4
#define PEN_RUBBER_DOWN					0xa5
#define PEN_ALL_LEAVE					0xe0
int os_start;
int hw_irq;
int flag;
atomic_t disable_ctp;
struct hanvon_pen_data
{
    u16 x;
    u16 y;
    u16 pressure;
    u8 flag;
};

struct hanvon_i2c_chip {
	unsigned char * chipname;
	struct workqueue_struct *ktouch_wq;
	struct work_struct work_irq;
	struct mutex mutex_wq;
	struct i2c_client *client;
	unsigned char work_state;
	struct input_dev *p_inputdev;
	struct timer_list timer;
};

static struct i2c_device_id hanvon_i2c_idtable[] = {
	{ "hanvon_0868_i2c", 0 }, 
	{ } 
};

static void hw0868_reset()
{
	int rc;
	printk(KERN_INFO "Hanvon 0868 RESET!\n");
	rc = gpio_request(TEGRA_GPIO_PQ4,"hw0868_cfgo");
	gpio_direction_output(TEGRA_GPIO_PQ4, 0);
//	gpio_set_value(0);
	rc = gpio_request(HW0868_GPIO_RESET,"hw0868_reset");
//	tegra_gpio_enable(HW0868_GPIO_RESET);
	gpio_direction_output(HW0868_GPIO_RESET, 1);
	mdelay(100);
	gpio_direction_output(HW0868_GPIO_RESET, 0);
}

static void hw0868_set_power(int i)
{
	printk(KERN_INFO "Hanvon 0868 set power (%d)\n", i);
//	if (i == 0)
//		gpio_direction_output(HW0868_GPIO_POWER, 0);
//	if (i == 1)
//		gpio_direction_output(HW0868_GPIO_POWER, 1);
}

static void hw0868_set_config_pin(int i)
{
	int rc;
	printk(KERN_INFO "Config pin status(%d)\n", i);
//	rc = gpio_request(HW0868_GPIO_CONFIG,"hw0868_config");
//	tegra_gpio_enable(HW0868_GPIO_CONFIG);
//	if (i == 0)
//		gpio_direction_output(HW0868_GPIO_CONFIG, 0);
//	if (i == 1)
//		gpio_direction_output(HW0868_GPIO_CONFIG, 1);
}

int fw_i2c_master_send(const struct i2c_client *client, const char *buf, int count)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

	//msg.addr = client->addr;
	printk(KERN_INFO "11111111fw_i2c_master_send111111");
	msg.addr = UPDATE_SLAVE_ADDR;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (char *)buf;

	ret = i2c_transfer(adap, &msg, 1);
	printk(KERN_INFO "-------[hanvon] fw_i2c_master_send  ret = %d---------", ret);
	/* 
	 * If everything went ok (i.e. 1 msg transmitted), return #bytes
	 * transmitted, else error code.
	 */
	return (ret == 1) ? count : ret;
}

int fw_i2c_master_recv(const struct i2c_client *client, char *buf, int count)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int ret;

	//msg.addr = client->addr;
	printk(KERN_INFO "222222fw_i2c_master_recv22222");
	msg.addr = UPDATE_SLAVE_ADDR;
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = buf;

	ret = i2c_transfer(adap, &msg, 1);
	printk(KERN_INFO "-------[hanvon] fw_i2c_master_recv  ret = %d---------", ret);
	/*
	 * If everything went ok (i.e. 1 msg received), return #bytes received,
	 * else error code.
	 */
	return (ret == 1) ? count : ret;
}


ssize_t hw0868_i2c_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count, i;
	unsigned char csw_packet[13] = {1};
	printk(KERN_INFO "Receive CSW package.\n");
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	count = fw_i2c_master_recv(client, csw_packet, 13);
	if (count < 0)
	{
		return -1;
	}
	//memcpy(buf, csw_packet, count);
	printk(KERN_INFO "[num 01] read %d bytes.\n", count);
	for(i = 0; i < count; i++)
	{
		printk(KERN_INFO "%.2x \n", csw_packet[i]);
	}
	return sprintf(buf, "%s", csw_packet);
}

ssize_t hw0868_i2c_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	// printk(KERN_INFO "%x\n", buf[3]);
	// printk(KERN_INFO "%x\n", buf[2]);
	// printk(KERN_INFO "%x\n", buf[1]);
	// printk(KERN_INFO "%x\n", buf[0]);
	printk(KERN_INFO "%s is called, count=%d.\n", __func__, count);
	if ((count == 31) && (buf[1] == 0x57) && (buf[0] == 0x48))
	{
		printk(KERN_INFO "Send CBW package.\n");
		struct i2c_client *client;
		client = container_of(dev, struct i2c_client, dev);
		ret = fw_i2c_master_send(client, buf, count);
		return ret;
	}

	// transfer file
	if (count == 32)
	{
		printk(KERN_INFO "Transfer file.\n");
		struct i2c_client *client;
		client = container_of(dev, struct i2c_client, dev);
		ret = fw_i2c_master_send(client, buf, count);
		return ret;
	}
	
	int cmd = *((int *)buf);
	if ((cmd & 0x08680000) != 0x08680000)
	{
		printk(KERN_INFO "Invalid command (0x%08x).\n", cmd);
		return -1;
	}

	switch(cmd)
	{
		case HW0868_CMD_RESET:
			printk(KERN_INFO "Command: reset.\n");
			hw0868_reset();
			break;
		case HW0868_CMD_CONFIG_HIGH:
			printk(KERN_INFO "Command: config high.\n");
			hw0868_set_config_pin(1);
			break;
		case HW0868_CMD_CONFIG_LOW:
			printk(KERN_INFO "Command: low.\n");
			hw0868_set_config_pin(0);
			break;
	}
	return count;
}

DEVICE_ATTR(hw0868_entry, 0666, hw0868_i2c_show, hw0868_i2c_store);

MODULE_DEVICE_TABLE(i2c, hanvon_i2c_idtable);

static struct input_dev * allocate_hanvon_input_dev(void)
{
	int ret;
	struct input_dev *p_inputdev=NULL;

	p_inputdev = input_allocate_device();
	if(p_inputdev == NULL)
	{
		return NULL;
	}

//	p_inputdev->name = "Hanvon electromagnetic pen";
	p_inputdev->name = "Hanvon_ts";
	p_inputdev->phys = "I2C";
	p_inputdev->id.bustype = BUS_I2C;
	
	__set_bit(INPUT_PROP_DIRECT, p_inputdev->propbit);
	__set_bit(EV_ABS, p_inputdev->evbit);
	__set_bit(EV_KEY, p_inputdev->evbit);
	__set_bit(BTN_TOUCH, p_inputdev->keybit);
	__set_bit(BTN_TOOL_PEN, p_inputdev->keybit);
	__set_bit(BTN_TOOL_RUBBER, p_inputdev->keybit);
	input_set_abs_params(p_inputdev, ABS_X, 0, MAX_X, 0, 0);
	input_set_abs_params(p_inputdev, ABS_Y, 0, MAX_Y, 0, 0);
	input_set_abs_params(p_inputdev, ABS_PRESSURE, 0, 1024, 0, 0);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	input_set_events_per_packet(p_inputdev, MAX_EVENTS);
#endif

	ret = input_register_device(p_inputdev);
	if(ret) 
	{
		printk(KERN_INFO "Unable to register input device.\n");
		input_free_device(p_inputdev);
		p_inputdev = NULL;
	}
	
	return p_inputdev;
}

static struct hanvon_pen_data hanvon_get_packet(struct hanvon_i2c_chip *phic)
{
	struct hanvon_pen_data data = {0};
	struct i2c_client *client = phic->client;
	u8 x_buf[MAX_PACKET_SIZE];
	int count;

	do {
		count = i2c_master_recv(client, x_buf, MAX_PACKET_SIZE);
	}
	while(count == EAGAIN);
//	printk(KERN_INFO "Reading data. %.2x %.2x %.2x %.2x %.2x %.2x %.2x, count=%d\n", 
//				x_buf[0], x_buf[1], x_buf[2], x_buf[3], x_buf[4], 
//				x_buf[5], x_buf[6], count);

#if DEBUG_I2C_TRANSTER	
	data.x |= ((x_buf[1]&0x7f) << 9) | (x_buf[2] << 2) | (x_buf[6] >> 5); // x
	data.y |= ((x_buf[3]&0x7f) << 9) | (x_buf[4] << 2) | ((x_buf[6] >> 3)&0x03); // y
	data.pressure |= ((x_buf[6]&0x07) << 7) | (x_buf[5]);  // pressure
	data.flag = x_buf[0];
#endif
	return data;
}

static int hanvon_report_event(struct hanvon_i2c_chip *phic)
{
	struct hanvon_pen_data data = {0};
	data = hanvon_get_packet(phic);
	//hanvon_get_packet(phic);
#if DEBUG_I2C_TRANSTER
//	printk(KERN_INFO "data.x=%d, data.y=%d, pressure=%d, flag=%d\n", data.x, data.y, data.pressure, data.flag);
	data.x=(u16)((data.x*1280)/10206);
//	data.x=(u16)(data.x/8-5);
//	data.y=(u16)(7422-data.y);//
	data.y=(u16)((data.y*800)/7422);
//	data.y=(u16)(data.y/9);
//	printk(KERN_INFO "x=%d, y=%d, pressure=%d, flag=%d\n", data.x, data.y, data.pressure, data.flag);
	static int last_status = 0;

	switch(data.flag)
	{
		case PEN_RUBBER_DOWN:
		{   
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 1);
			input_report_key(phic->p_inputdev, BTN_TOOL_RUBBER, 1);
			last_status = data.flag;
			break;
        	}
		case PEN_RUBBER_UP:
		{
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 0);
			input_report_key(phic->p_inputdev, BTN_TOOL_RUBBER, 0);
			last_status = 0;
			break;
		}
		case PEN_POINTER_DOWN:
		{
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 1);
			input_report_key(phic->p_inputdev, BTN_TOOL_PEN, 1);
			last_status = data.flag;
			break;
		}
		case PEN_POINTER_UP:
		{
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 0);
			input_report_key(phic->p_inputdev, BTN_TOOL_PEN, 0);
			last_status = 0;
			break;
		}
		case PEN_ALL_LEAVE:
		{
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 0);
			input_report_key(phic->p_inputdev, BTN_TOOL_PEN, 0);
			last_status = 0;
			break;
		}
	}
	input_sync(phic->p_inputdev);
#endif
	return 0;
}

static void hanvon_i2c_wq(struct work_struct *work)
{
	struct hanvon_i2c_chip *phid = container_of(work, struct hanvon_i2c_chip, work_irq);
	struct i2c_client *client = phid->client;
	//int gpio = irq_to_gpio(client->irq);
	//printk(KERN_INFO "%s: queue work.gpio(%d)\n", __func__, gpio);
	mutex_lock(&phid->mutex_wq);
	hanvon_report_event(phid);
	schedule();
	mutex_unlock(&phid->mutex_wq);
	enable_irq(client->irq);
}
static void disable_ctp_timer(unsigned long dev_id)
{
	printk(KERN_INFO "%s:timer handled.\n", __func__);	
	flag=0;
	atomic_set(&disable_ctp, flag);	
}
static irqreturn_t hanvon_i2c_interrupt(int irq, void *dev_id)
{
	struct hanvon_i2c_chip *phid = (struct hanvon_i2c_chip *)dev_id;

	disable_irq_nosync(irq);
//	if(os_start!=0)
	queue_work(phid->ktouch_wq, &phid->work_irq);
//	printk(KERN_INFO "%s:Interrupt handled.\n", __func__);
	flag=1;
	atomic_set(&disable_ctp, flag);
	mod_timer(&phid->timer,jiffies + msecs_to_jiffies(100));
	return IRQ_HANDLED;
}
static 	ssize_t etp_on_func(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	if (!(sscanf(buf, "%u\n", &val))) return -EINVAL;

		switch(val) {
			case 0:				
			printk("----------------------etp_on 0 on-----------------");
			os_start=0;
			disable_irq_nosync(hw_irq);
			break;

			case 1:
			printk("----------------------etp_on 1 on-----------------");
			if(os_start==0)
			{os_start=1;
			enable_irq(hw_irq);}
			break;

			default:
			break;
		}
	return count;
}
static	DEVICE_ATTR(etp_on, 0666, NULL, etp_on_func);
static 	ssize_t etp_reset_func(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	if (!(sscanf(buf, "%u\n", &val))) return -EINVAL;

		switch(val) {
			case 0:				
			printk("----------------------etp_reset 0 on-----------------");
			hw0868_reset();
			break;

			case 1:
			printk("----------------------etp_reset 1 on-----------------");
			hw0868_reset();
			break;

			default:
			break;
		}
	return count;
}
static	DEVICE_ATTR(etp_reset, 0666, NULL, etp_reset_func);
static int hanvon_i2c_probe(struct i2c_client * client, const struct i2c_device_id * idp)
{
	int result = -1;
//	int gpio = irq_to_gpio(client->irq);
//	printk(KERN_INFO "starting %s, irq(%d).\n", __func__, gpio);
	struct hanvon_i2c_chip *hidp = NULL;
	hidp = (struct hanvon_i2c_chip*)kzalloc(sizeof(struct hanvon_i2c_chip), GFP_KERNEL);
	if(!hidp)
	{
		printk(KERN_INFO "request memory failed.\n");
		result = -ENOMEM;
		goto fail1;
	}
	// setup input device.
	hidp->p_inputdev = allocate_hanvon_input_dev();
	// setup work queue.
	hidp->client = client;
	hidp->ktouch_wq = create_singlethread_workqueue("hanvon0868");
	mutex_init(&hidp->mutex_wq);
	INIT_WORK(&hidp->work_irq, hanvon_i2c_wq);
	setup_timer(&hidp->timer,disable_ctp_timer, (unsigned long)hidp);
	i2c_set_clientdata(client, hidp);
	// request irq.
	os_start=0;
	hw_irq=client->irq;
	result = request_irq(client->irq, hanvon_i2c_interrupt, IRQF_DISABLED | IRQF_TRIGGER_LOW,
		 client->name, hidp);
//	result = request_irq(client->irq, hanvon_i2c_interrupt, IRQF_DISABLED | IRQF_TRIGGER_FALLING,
//		 client->name, hidp);
//	result = request_irq(client->irq, hanvon_i2c_interrupt, IRQF_DISABLED | IRQF_TRIGGER_HIGH,
//		 client->name, hidp);
	if(result)
	{
		printk(KERN_INFO " Request irq(%d) failed\n", client->irq);
		goto fail1;
	}
	//define a entry for update use.
	device_create_file(&client->dev, &dev_attr_hw0868_entry);
	device_create_file(&client->dev, &dev_attr_etp_on);
	device_create_file(&client->dev, &dev_attr_etp_reset);
	printk(KERN_INFO "%s done.\n", __func__);
	printk(KERN_INFO "Name of device: %s.\n", client->dev.kobj.name);
	hw0868_reset();
	disable_irq_nosync(client->irq);
	//hw0868_set_power(0);
	//mdelay(100);
	//hw0868_set_power(1);
	return 0;
fail2:
	i2c_set_clientdata(client, NULL);
	destroy_workqueue(hidp->ktouch_wq);
	free_irq(client->irq, hidp);
	input_unregister_device(hidp->p_inputdev);
	hidp->p_inputdev = NULL;
fail1:
	kfree(hidp);
	hidp = NULL;
	return result;
}

static int hanvon_i2c_remove(struct i2c_client * client)
{
//	del_timer_sync(hidp->timer);
	return 0;
}

static int hanvon_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	//reset hw0868 chip
	hw0868_reset();
//	tegra_gpio_init_configure(TEGRA_GPIO_PS2, false, 0);
	gpio_direction_output(TEGRA_GPIO_PS2, 0);
	return 0;
}

static int hanvon_i2c_resume(struct i2c_client *client)
{
	// reset hw0868 chip
	tegra_gpio_init_configure(TEGRA_GPIO_PS2, false, 1);
	gpio_direction_output(TEGRA_GPIO_PS2, 1);
	mdelay(50);
	hw0868_reset();
	return 0;
}
static struct dev_pm_ops hanvon_pm_ops = {
	.suspend	= hanvon_i2c_suspend,
	.resume		= hanvon_i2c_resume,
};
static struct i2c_driver hanvon_i2c_driver = {
	.driver = {
		.name 	= "hanvon_0868_i2c",
		.pm	= &hanvon_pm_ops,
	},
	.id_table	= hanvon_i2c_idtable,
	.probe		= hanvon_i2c_probe,
	.remove		= __devexit_p(hanvon_i2c_remove),
};

static int hw0868_i2c_init(void)
{
	printk(KERN_INFO "hw0868 chip initializing ....\n");
	return i2c_add_driver(&hanvon_i2c_driver);
}

static void hw0868_i2c_exit(void)
{
	printk(KERN_INFO "hw0868 driver exit.\n");
	i2c_del_driver(&hanvon_i2c_driver);
}

module_init(hw0868_i2c_init);
module_exit(hw0868_i2c_exit);

MODULE_AUTHOR("zhang nian");
MODULE_DESCRIPTION("Electromagnetic Pen");
MODULE_LICENSE("GPL");
