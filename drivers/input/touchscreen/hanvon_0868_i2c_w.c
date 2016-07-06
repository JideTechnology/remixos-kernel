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
 * !!!!!!!!!!!!!! NOTICE !!!!!!!!!!!!!!!!
 * Nvidia tegra 2 ventana demo board.
 * OS: android 4.0.3
 * version: 0.4.0
 * Features:
 * 		1. firmware update function
 *		2. work with calibration App
 *		3. add side key function
 *      4. Dual OS support
 * Updated: 2014/12/11
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
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#define HANVON_NAME						"hanvon_0868"
#define CHIP_NAME						"Hanvon0868"
#define MAX_EVENTS						10
#define SCR_X							1920
#define SCR_Y							1200
#define MAX_X							0x27de
#define MAX_Y							0x1cfe
#define MAX_PRESSURE					1024

/* if uncomment this definition, calibrate function is enabled. */
//#define HW0868_CALIBRATE

#define MAX_PACKET_SIZE					10

#define DEBUG_SHOW_RAW					0X00000001
#define DEBUG_SHOW_COORD				0X00000010

#define UPDATE_SLAVE_ADDR				0x34
#define HW0868_GPIO_INT      			444
#define HW0868_GPIO_RESET				447
#define HW0868_GPIO_CONFIG				2
#if 0
#define HW0868_GPIO_POWER				0
#endif 

#define HW0868_CMD_RESET				0x08680000
#define HW0868_CMD_CONFIG_HIGH			0x08680001
#define HW0868_CMD_CONFIG_LOW			0x08680002
#define HW0868_CMD_UPDATE				0x08680003
#define HW0868_CMD_GET_VERSION			0x08680004
#define HW0868_CMD_CALIBRATE			0x08680005

/* define pen flags, 10-bytes protocal, for Windows HID-OVER I2C.*/
#define PEN_POINTER_UP					0x10
#define PEN_POINTER_DOWN				0x11
#define PEN_BUTTON_UP					0x12
#define PEN_BUTTON_DOWN					0x13
#define PEN_RUBBER_UP					0x18
#define PEN_RUBBER_DOWN					0x19
#define PEN_ALL_LEAVE					0x80

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
};

/* global I2C client. */
static struct i2c_client *g_client;

/* when pen detected, this flag is set 1 */
volatile int isPenDetected = 0;

/* DEBUG micro, for user interface. */
static unsigned int debug = 0;

static unsigned int isPenAndTPpriority=0;

/* version number buffer */
static unsigned char ver_info[9] = {0};
static int ver_size = 9;

/* calibration parameter */
static bool isCalibrated = false;
static int a[7];
static int A = 65535, B = 0, C = 16, D = 0, E = 65535, F = 0, scale = 65536;

#define hw0868_dbg_raw(fmt, args...)        \
	    if(debug & DEBUG_SHOW_RAW) \
        printk(KERN_INFO "[HW0868 raw]: "fmt, ##args);
#define hw0868_dbg_coord(fmt, args...)    \
	    if(debug & DEBUG_SHOW_COORD) \
        printk(KERN_INFO "[HW0868 coord]: "fmt, ##args);

static struct i2c_device_id hanvon_i2c_idtable[] = {
	{ "hanvon_0868", 0 }, 
	{ } 
};

static void hw0868_reset(void)
{
	printk("Hanvon 0868 reset!\n");
	//tegra_gpio_enable(HW0868_GPIO_RESET);
	gpio_direction_output(HW0868_GPIO_RESET, 0);
	msleep(50);
	gpio_direction_output(HW0868_GPIO_RESET, 1);
	//msleep(50);
}
static void hw0868_set_config_pin(int i)
{
	printk("Config pin status(%d)\n", i);
//	tegra_gpio_enable(HW0868_GPIO_CONFIG);
	if (i == 0)
		gpio_direction_output(HW0868_GPIO_CONFIG, 0);
	if (i == 1)
		gpio_direction_output(HW0868_GPIO_CONFIG, 1);
}

static int hw0868_get_version(struct i2c_client *client)
{
	int ret = -1;
	unsigned char ver_cmd[] = {0xcd, 0x5f};
	//hw0868_reset();
	ret = i2c_master_send(client, ver_cmd, 2);
	if (ret < 0)
	{
		printk("Get version ERROR!\n");
		return ret;
	}
	return ret;
}

static int read_calibrate_param(void)
{
	int ret = -1;
	mm_segment_t old_fs;
	struct file *file = NULL;
	printk("kernel read calibrate param.\n");
	old_fs = get_fs();
	set_fs(get_ds());
	file = filp_open("/data/calibrate", O_RDONLY, 0);
	if (file == NULL)
		return -1;
	if (file->f_op->read == NULL)
		return -1;

	// TODO: read file
	if (file->f_op->read(file, (unsigned char*)a, sizeof(int)*7, &file->f_pos) == 28)
	{
		printk("calibrate param: %d, %d, %d, %d, %d, %d, %d\n", a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
		A = a[1];
		B = a[2];
		C = a[0];
		D = a[4];
		E = a[5];
		F = a[3];
		scale = a[6];
		isCalibrated = true;
	}
	else
	{
		filp_close(file, NULL);
		set_fs(old_fs);
		return -1;
	}

	filp_close(file, NULL);
	set_fs(old_fs);
	return 0;
}

int fw_i2c_master_send(const struct i2c_client *client, const char *buf, int count)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

	msg.addr = UPDATE_SLAVE_ADDR;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (char *)buf;

	ret = i2c_transfer(adap, &msg, 1);
	printk("[hanvon 0868 update] fw_i2c_master_send  ret = %d\n", ret);
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

	msg.addr = UPDATE_SLAVE_ADDR;
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = buf;

	ret = i2c_transfer(adap, &msg, 1);
	printk("[hanvon 0868 update] fw_i2c_master_recv  ret = %d\n", ret);
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
	printk("Receive CSW package.\n");
	count = fw_i2c_master_recv(g_client, csw_packet, 13);
	if (count < 0)
	{
		return -1;
	}
	printk("[num 01] read %d bytes.\n", count);
	for(i = 0; i < count; i++)
	{
		printk("%.2x \n", csw_packet[i]);
	}
	return sprintf(buf, "%s", csw_packet);
}

ssize_t hw0868_i2c_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	printk(KERN_INFO "%x\n", buf[3]);
	printk(KERN_INFO "%x\n", buf[2]);
	printk(KERN_INFO "%x\n", buf[1]);
	printk(KERN_INFO "%x\n", buf[0]);
	printk(KERN_INFO "%s is called, count=%d.\n", __func__, count);
	if ((count == 31) && (buf[1] == 0x57) && (buf[0] == 0x48))
	{
		printk(KERN_INFO "Send CBW package.\n");
		ret = fw_i2c_master_send(g_client, buf, count);
		return ret;
	}

	/* transfer file */
	if (count == 32)
	{
		printk(KERN_INFO "Transfer file.\n");
		ret = fw_i2c_master_send(g_client, buf, count);
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
			printk("Command: reset.\n");
			hw0868_reset();
			break;
		case HW0868_CMD_CONFIG_HIGH:
			printk("Command: set config pin high.\n");
			hw0868_set_config_pin(1);
			break;
		case HW0868_CMD_CONFIG_LOW:
			printk("Command: set config pin low.\n");
			hw0868_set_config_pin(0);
			break;
		case HW0868_CMD_GET_VERSION:
			printk("Command: get firmware version.\n");
			hw0868_get_version(g_client);
			break;
		case HW0868_CMD_CALIBRATE:
			printk(KERN_INFO "Command: Calibrate.\n");
			read_calibrate_param();
			break;
	}
	return count;
}

DEVICE_ATTR(hw0868_entry, 0666, hw0868_i2c_show, hw0868_i2c_store);

/* get version */
ssize_t hw0868_version_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "version number: %.2x %.2x %.2x %.2x %.2x %.2x\n", 
			ver_info[1], ver_info[2], ver_info[3],ver_info[4], ver_info[5], ver_info[6]);
}

ssize_t hw0868_version_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}
DEVICE_ATTR(version, 0666, hw0868_version_show, hw0868_version_store);

MODULE_DEVICE_TABLE(i2c, hanvon_i2c_idtable);

static long hw0868_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static struct file_operations hanvon_cdev_fops = {
	.owner= THIS_MODULE,
	.unlocked_ioctl= hw0868_ioctl,
};

static struct miscdevice hanvon_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "hw0868",
	.fops = &hanvon_cdev_fops,
};

static struct input_dev * allocate_hanvon_input_dev(void)
{
	int ret;
	struct input_dev *p_inputdev=NULL;

	p_inputdev = input_allocate_device();
	if(p_inputdev == NULL)
	{
		return NULL;
	}

	p_inputdev->name = "Hanvon electromagnetic pen";
	p_inputdev->phys = "I2C";
	p_inputdev->id.bustype = BUS_I2C;
	
	set_bit(EV_ABS, p_inputdev->evbit);
	__set_bit(INPUT_PROP_DIRECT, p_inputdev->propbit);
	__set_bit(EV_ABS, p_inputdev->evbit);
	__set_bit(EV_KEY, p_inputdev->evbit);
	__set_bit(BTN_TOUCH, p_inputdev->keybit);
	__set_bit(BTN_STYLUS, p_inputdev->keybit);
	__set_bit(BTN_TOOL_PEN, p_inputdev->keybit);
	__set_bit(BTN_TOOL_RUBBER, p_inputdev->keybit);

#ifdef HW0868_CALIBRATE
	input_set_abs_params(p_inputdev, ABS_X, 0, SCR_X, 0, 0);
	input_set_abs_params(p_inputdev, ABS_Y, 0, SCR_Y, 0, 0);
#else
	input_set_abs_params(p_inputdev, ABS_X, 0, MAX_X, 0, 0);
	input_set_abs_params(p_inputdev, ABS_Y, 0, MAX_Y, 0, 0);
#endif
	
	input_set_abs_params(p_inputdev, ABS_PRESSURE, 0, MAX_PRESSURE, 0, 0);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	input_set_events_per_packet(p_inputdev, MAX_EVENTS);
#endif

	ret = input_register_device(p_inputdev);
	if(ret) 
	{
		printk("Unable to register input device.\n");
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
	int sum_x, sum_y;

	do {
		//mdelay(2);
		count = i2c_master_recv(client, x_buf, MAX_PACKET_SIZE);
	}
	while(count == EAGAIN);
	/*DEBUG: show raw packets.*/

/*printk("Reading data. %.2x %.2x %.2x %.2x %.2x %.2x %.2x,%.2x,%.2x,%.2x, count=%d\n", 
				x_buf[0], x_buf[1], x_buf[2], x_buf[3], x_buf[4], 
				x_buf[5], x_buf[6], x_buf[7], x_buf[8], x_buf[9],count);
*/
	if (x_buf[0] == 0x80)
	{
		printk("Get version number ok!\n");
		memcpy(ver_info, x_buf, ver_size);
	}
	data.flag = x_buf[3];
	data.x |= ((x_buf[5] << 8) | x_buf[4]);			// x
	data.y |= ((x_buf[7] << 8) | x_buf[6]);			// y
	data.pressure |= ((x_buf[9] << 8) | x_buf[8]);	// pressure

#ifdef HW0868_CALIBRATE
	/* transform raw coordinate to srceen coord */
	data.x = data.x / MAX_X * SCR_X;
	data.y = data.y / MAX_Y * SCR_Y;

	/* perform calibrate */
	if (isCalibrated)
	{
		sum_x = data.x*A + data.y*B + C;
		if ((sum_x % scale) > 32768)
		{
			data.x = (sum_x >> 16) + 1;
		}
		else
		{
			data.x = sum_x >> 16;
		}
		
		sum_y = data.x*D + data.y*E + F;
		if ((sum_y % scale) > 32768)
		{
			data.y = (sum_y >> 16) + 1;
		}
		else
		{
			data.y = sum_y >> 16;
		}
	}
#endif

	return data;
}
#define GPIO_TP_IRQ		133

static int hanvon_report_event(struct hanvon_i2c_chip *phic)
{
	struct hanvon_pen_data data = {0};
	data = hanvon_get_packet(phic);

	if (data.flag == 0xe0)
	{
		
			if(isPenAndTPpriority){
			enable_irq(gpio_to_irq(GPIO_TP_IRQ));
			isPenAndTPpriority=0;
			}
		return 0;
	}
//	printk("x=%d\ty=%d\tpressure=%d\tflag=%d\n", data.x, data.y, data.pressure, data.flag);

	switch(data.flag)
	{
		/* side key events */
		case PEN_BUTTON_DOWN:
		{
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 1);
			input_report_key(phic->p_inputdev, BTN_TOOL_PEN, 1);
			input_report_key(phic->p_inputdev, BTN_STYLUS, 1);
			break;
		}
		case PEN_BUTTON_UP:
		{
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 0);
			input_report_key(phic->p_inputdev, BTN_TOOL_PEN, 0);
			input_report_key(phic->p_inputdev, BTN_STYLUS, 0);
			break;
		}
		/* rubber events */
		case PEN_RUBBER_DOWN:
		{   
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 1);
			input_report_key(phic->p_inputdev, BTN_TOOL_RUBBER, 1);
			break;
        	}
		case PEN_RUBBER_UP:
		{
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 0);
			input_report_key(phic->p_inputdev, BTN_TOOL_RUBBER, 0);
			break;
		}
		/* pen pointer events */
		case PEN_POINTER_DOWN:
		{
			if(!isPenAndTPpriority){
			disable_irq_nosync(gpio_to_irq(GPIO_TP_IRQ));
			isPenAndTPpriority=1;
		}
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 1);
			input_report_key(phic->p_inputdev, BTN_TOOL_PEN, 1);
			break;
		}
		case PEN_POINTER_UP:
		{
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 0);
			if (isPenDetected == 0)
				input_report_key(phic->p_inputdev, BTN_TOOL_PEN, 1);
			isPenDetected = 1;
			break;
		}
		/* Leave the induction area. */
		case PEN_ALL_LEAVE:
		{
			input_report_abs(phic->p_inputdev, ABS_X, data.x);
			input_report_abs(phic->p_inputdev, ABS_Y, data.y);
			input_report_abs(phic->p_inputdev, ABS_PRESSURE, data.pressure);
			input_report_key(phic->p_inputdev, BTN_TOUCH, 0);
			input_report_key(phic->p_inputdev, BTN_TOOL_PEN, 0);
			input_report_key(phic->p_inputdev, BTN_STYLUS, 0);
			isPenDetected = 0;
			break;
		}
		default:
			printk(KERN_ERR "Hanvon stylus device[0868,I2C]: Invalid input event.\n");
	}
	input_sync(phic->p_inputdev);
	return 0;
}

static void hanvon_i2c_wq(struct work_struct *work)
{
	struct hanvon_i2c_chip *phid = container_of(work, struct hanvon_i2c_chip, work_irq);
	struct i2c_client *client = phid->client;

	mutex_lock(&phid->mutex_wq);
	hanvon_report_event(phid);
//	schedule();
	mutex_unlock(&phid->mutex_wq);
//	enable_irq(client->irq);
}

static irqreturn_t hanvon_i2c_interrupt(int irq, void *dev_id)
{
	struct hanvon_i2c_chip *phid = (struct hanvon_i2c_chip *)dev_id;
	disable_irq_nosync(irq);
	queue_work(phid->ktouch_wq, &phid->work_irq);
//	printk("%s:Interrupt handled.\n", __func__);
	enable_irq(irq);
	return IRQ_HANDLED;
}

static int hanvon_i2c_probe(struct i2c_client * client, const struct i2c_device_id * idp)
{
	int result = -1;
	int err = 0;
	struct device *pdev = NULL;
	struct hanvon_i2c_chip *hidp = NULL;
	client->irq=444;
	g_client = client;	
	hidp = (struct hanvon_i2c_chip*)kzalloc(sizeof(struct hanvon_i2c_chip), GFP_KERNEL);
	if(!hidp)
	{
		printk("request memory failed.\n");
		result = -ENOMEM;
		goto fail1;
	}
	
	/* setup input device. */
	hidp->p_inputdev = allocate_hanvon_input_dev();
	
	/* setup work queue. */
	hidp->client = client;
	hidp->ktouch_wq = create_singlethread_workqueue("hanvon0868");
	mutex_init(&hidp->mutex_wq);
	INIT_WORK(&hidp->work_irq, hanvon_i2c_wq);
	i2c_set_clientdata(client, hidp);
	err = gpio_request(HW0868_GPIO_RESET, "Hanvon 0868 reset");
     if (err < 0) {
          printk(KERN_ERR "%s:failed to set gpio reset.\n",
             __func__);
     }

     err = gpio_direction_output(HW0868_GPIO_RESET, 1);
     if (err) {
         printk(KERN_ERR "set reset direction error=%d\n", err);
         gpio_free(HW0868_GPIO_RESET);
     }
	 
 /*init INTERRUPT pin*/
     err = gpio_request(HW0868_GPIO_INT, client->name);
     if (err < 0)
         printk(KERN_ERR "Failed to request GPIO%d (hw0868 int) err=%d\n",  HW0868_GPIO_INT, err);
 
     err = gpio_direction_input(HW0868_GPIO_INT);
     if (err) {
         printk(KERN_ERR "set int direction err=%d\n", err);
         gpio_free(HW0868_GPIO_INT);
     }
	/* request irq. */
	client->irq = gpio_to_irq(HW0868_GPIO_INT);
	result = request_irq(client->irq, hanvon_i2c_interrupt, IRQF_DISABLED | IRQF_TRIGGER_FALLING /*IRQF_TRIGGER_LOW*/, client->name, hidp);
	if(result)
	{
		printk(" Request irq(%d) failed\n", client->irq);
		goto fail1;
	}
   disable_irq_nosync(client->irq); 

	/*  define a entry for update use.
	 *  register misc device  */
	result = misc_register(&hanvon_misc_dev);
	device_create_file(hanvon_misc_dev.this_device, &dev_attr_hw0868_entry);
	device_create_file(hanvon_misc_dev.this_device, &dev_attr_version);

	printk(KERN_INFO "%s done.\n", __func__);
	printk(KERN_INFO "Name of device: %s.\n", client->dev.kobj.name);

 /*
 	err = gpio_direction_output(HW0868_GPIO_POWER, 1);
     	if (err) {
         printk(KERN_ERR "set reset direction error=%d\n", err);
         gpio_free(HW0868_GPIO_POWER);
     }
*/
 enable_irq(client->irq);  
/*	hw0868_reset();
	//Try to get firmware version here!
	hw0868_get_version(client);
*/
	int ret = -1;
	unsigned char v_cmd[] = {0x05, 0x00, 0x00, 0x08};
	//hw0868_reset();
	ret = i2c_master_send(client, v_cmd, 4);
	if (ret < 0)
	{
		printk("Send ERROR!\n");
		return ret;
	}
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
	return 0;
}

static int hanvon_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	/* reset hw0868 chip */
	hw0868_reset();
	return 0;
}

static int hanvon_i2c_resume(struct i2c_client *client)
{
	/* reset hw0868 chip */
	hw0868_reset();
	return 0;
}

static struct i2c_driver hanvon_i2c_driver = {

	.probe		= hanvon_i2c_probe,	
	.remove		= hanvon_i2c_remove,
	.id_table	= hanvon_i2c_idtable,
	.driver = {
		.name 	= HANVON_NAME,
		.owner    = THIS_MODULE,
	},
};
static struct i2c_board_info hw0868_i2c_info = {
	.type = HANVON_NAME,
	.addr = 0x18,
};

static int register_i2c_client(void)
{
	int ret=0;
	int i2c_bus_num=1;
	struct i2c_client *client;
	struct i2c_adapter *adapter;
	adapter = i2c_get_adapter(i2c_bus_num);
	if(!adapter){
		printk("Can't get adapter form i2c_bus_num[%d]",i2c_bus_num);
        return -ENODEV;
		}
	client  = i2c_new_device(adapter ,&hw0868_i2c_info);
	if(!client){
		printk("Can't register i2c client!");
        kfree(adapter);
        return -ENODEV;
		}
	return ret;
	
}
static int hw0868_i2c_init(void)
{
	int ret=0;
	ret=register_i2c_client();
	if(ret<0){
		printk("hw0868 register i2c client failed...\n ");
		return ret;
	}

	return i2c_add_driver(&hanvon_i2c_driver);
}

static void hw0868_i2c_exit(void)
{
	i2c_del_driver(&hanvon_i2c_driver);
}

late_initcall(hw0868_i2c_init);
module_exit(hw0868_i2c_exit);

module_param(debug, uint, S_IRUGO | S_IWUSR);


/*static int __init hw0868_platform_init(void)
{
	static const struct i2c_board_info hw0868_i2c_boardinfo []= {
	{
		I2C_BOARD_INFO("hanvon_0868_i2c", 0x18),
		.irq =444,
	}
	};

	return i2c_register_board_info(4, hw0868_i2c_boardinfo, ARRAY_SIZE(hw0868_i2c_boardinfo));
}

arch_initcall(hw0868_platform_init);*/

MODULE_AUTHOR("Zhang Nian");
MODULE_DESCRIPTION("Hanvon Electromagnetic Pen");
MODULE_LICENSE("GPL");
