/*
 * linux/sound/soc/codecs/aic325x_tiload.c
 *
 *
 * Copyright (C) 2012 Texas Instruments, Inc.
 *
 *
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * History:
 *
 * Rev 0.1 	 Tiload support          16-09-2010
 *
 *          The Tiload programming support is added to aic325x.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/control.h>

#include "tlv320aic325x.h"
#include "aic325x_tiload.h"

/* enable debug prints in the driver */
#define DEBUG
//#undef DEBUG

#ifdef DEBUG
#define dprintk(x...) 	printk(x)
#else
#define dprintk(x...)
#endif

#ifdef AIC325x_TiLoad

/* Function prototypes */
#ifdef REG_DUMP_aic325x
static void aic325x_dump_page(struct i2c_client *i2c, u8 page);
#endif

/* externs */
extern int aic325x_change_page(struct snd_soc_codec *codec, u8 new_page);
//extern int aic325x_change_book(struct snd_soc_codec *codec, u8 new_book);
extern int aic325x_write(struct snd_soc_codec *codec, u16 reg, u8 value);
int aic325x_driver_init(struct snd_soc_codec *codec);
/************** Dynamic aic325x driver, TI LOAD support  ***************/

static struct cdev *aic325x_cdev;
static int aic325x_major = 0;	/* Dynamic allocation of Mjr No. */
static int aic325x_opened = 0;	/* Dynamic allocation of Mjr No. */
static struct snd_soc_codec *aic325x_codec;
struct class *tiload_class;
static unsigned int magic_num = 0xE0;

/******************************** Debug section *****************************/

#ifdef REG_DUMP_aic325x
/*
 *----------------------------------------------------------------------------
 * Function : aic325x_dump_page
 * Purpose  : Read and display one codec register page, for debugging purpose
 *----------------------------------------------------------------------------
 */
static void aic325x_dump_page(struct i2c_client *i2c, u8 page)
{
	int i;
	u8 data;
	u8 test_page_array[8];

	dprintk("TiLoad DRIVER : %s\n", __FUNCTION__);
	aic325x_change_page(codec, page);

	data = 0x0;

	i2c_master_send(i2c, data, 1);
	i2c_master_recv(i2c, test_page_array, 8);

	printk("\n------- aic325x PAGE %d DUMP --------\n", page);
	for (i = 0; i < 8; i++) {
		printk(" [ %d ] = 0x%x\n", i, test_page_array[i]);
	}
}
#endif

/*
 *----------------------------------------------------------------------------
 * Function : tiload_open
 *
 * Purpose  : open method for aic325x-tiload programming interface
 *----------------------------------------------------------------------------
 */
static int tiload_open(struct inode *in, struct file *filp)
{
	dprintk("TiLoad DRIVER : %s\n", __FUNCTION__);
	if (aic325x_opened) {
		printk("%s device is already opened\n", "aic325x");
		printk("%s: only one instance of driver is allowed\n",
		       "aic325x");
		return -1;
	}
	aic325x_opened++;
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : tiload_release
 *
 * Purpose  : close method for aic325x_tilaod programming interface
 *----------------------------------------------------------------------------
 */
static int tiload_release(struct inode *in, struct file *filp)
{
	dprintk("TiLoad DRIVER : %s\n", __FUNCTION__);
	aic325x_opened--;
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : tiload_read
 *
 * Purpose  : read method for mini dsp programming interface
 *----------------------------------------------------------------------------
 */
static ssize_t tiload_read(struct file *file, char __user * buf,
			   size_t count, loff_t * offset)
{
	static char rd_data[8];
	char reg_addr;
	size_t size;
	#ifdef DEBUG
	int i;
	#endif
	struct i2c_client *i2c = aic325x_codec->control_data;

	dprintk("TiLoad DRIVER : %s\n", __FUNCTION__);
	if (count > 128) {
		printk("Max 128 bytes can be read\n");
		count = 128;
	}

	/* copy register address from user space  */
	size = copy_from_user(&reg_addr, buf, 1);
	if (size != 0) {
		printk("read: copy_from_user failure\n");
		return -1;
	}
	/* Send the address to device thats is to be read */

	if (i2c_master_send(i2c, &reg_addr, 1) != 1) {
		dprintk("Can not write register address\n");
		return -1;
	}
	/* read the codec device registers */
	size = i2c_master_recv(i2c, rd_data, count);
#ifdef DEBUG
	printk(KERN_ERR "read size = %d, reg_addr= %x , count = %d\n",
	       (int)size, reg_addr, (int)count);
	for (i = 0; i < (int)size; i++) {
		printk(KERN_ERR "rd_data[%d]=%x\n", i, rd_data[i]);
	}
#endif
	if (size != count) {
		printk("read %d registers from the codec\n", size);
	}

	if (copy_to_user(buf, rd_data, size) != 0) {
		dprintk("copy_to_user failed\n");
		return -1;
	}

	return size;
}

/*
 *----------------------------------------------------------------------------
 * Function : tiload_write
 *
 * Purpose  : write method for aic325x_tiload programming interface
 *----------------------------------------------------------------------------
 */
static ssize_t tiload_write(struct file *file, const char __user * buf,
			    size_t count, loff_t * offset)
{
	static char wr_data[8];
	u8 pg_no;
	#ifdef DEBUG
	int i;
	#endif
	struct i2c_client *i2c = aic325x_codec->control_data;
	struct aic325x_priv *aic325x_private = snd_soc_codec_get_drvdata(aic325x_codec);

	dprintk("TiLoad DRIVER : %s\n", __FUNCTION__);
	/* copy buffer from user space  */
	if (copy_from_user(wr_data, buf, count)) {
		printk("copy_from_user failure\n");
		return -1;
	}
#ifdef DEBUG
	printk(KERN_ERR "write size = %d\n", (int)count);
	for (i = 0; i < (int)count; i++) {
		printk(KERN_INFO "\nwr_data[%d]=%x\n", i, wr_data[i]);
	}
#endif
	if (wr_data[0] == 0) {
		aic325x_change_page(aic325x_codec, wr_data[1]);
		return count;
	}
	pg_no = aic325x_private->page_no;

	/*if ((wr_data[0] == 127) && (pg_no == 0)) {
		aic325x_change_book(aic325x_codec, wr_data[1]);
		return count;
	} */
	return i2c_master_send(i2c, wr_data, count);
}

static int tiload_ioctl( struct file *filp,
			unsigned int cmd, unsigned long arg)
{
	int num = 0;
	void __user *argp = (void __user *)arg;
	if (_IOC_TYPE(cmd) != aic325x_IOC_MAGIC)
		return -ENOTTY;

	dprintk("TiLoad DRIVER : %s\n", __FUNCTION__);
	switch (cmd) {
	case aic325x_IOMAGICNUM_GET:
		num = copy_to_user(argp, &magic_num, sizeof(int));
		break;
	case aic325x_IOMAGICNUM_SET:
		num = copy_from_user(&magic_num, argp, sizeof(int));
		break;
	}
	return num;
}

/*********** File operations structure for aic325x-tiload programming *************/
static struct file_operations aic325x_fops = {
	.owner = THIS_MODULE,
	.open = tiload_open,
	.release = tiload_release,
	.read = tiload_read,
	.write = tiload_write,
	.unlocked_ioctl = tiload_ioctl,
};

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_driver_init
 *
 * Purpose  : Register a char driver for dynamic aic325x-tiload programming
 *----------------------------------------------------------------------------
 */
int aic325x_driver_init(struct snd_soc_codec *codec)
{
	int result;

	dev_t dev = MKDEV(aic325x_major, 0);
	dprintk("TiLoad DRIVER : %s\n", __FUNCTION__);
	aic325x_codec = codec;

	dprintk("allocating dynamic major number\n");

	result = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	if (result < 0) {
		dprintk("cannot allocate major number %d\n", aic325x_major);
		return result;
	}
	tiload_class = class_create(THIS_MODULE, DEVICE_NAME);
	aic325x_major = MAJOR(dev);
	dprintk("allocated Major Number: %d\n", aic325x_major);

	aic325x_cdev = cdev_alloc();
	cdev_init(aic325x_cdev, &aic325x_fops);
	aic325x_cdev->owner = THIS_MODULE;
	aic325x_cdev->ops = &aic325x_fops;

	if (cdev_add(aic325x_cdev, dev, 1) < 0) {
		dprintk("aic325x_driver: cdev_add failed \n");
		unregister_chrdev_region(dev, 1);
		aic325x_cdev = NULL;
		return 1;
	}
	printk("Registered aic325x TiLoad driver, Major number: %d \n",
	       aic325x_major);
	//class_device_create(tiload_class, NULL, dev, NULL, DEVICE_NAME, 0);
	return 0;
}

#endif
