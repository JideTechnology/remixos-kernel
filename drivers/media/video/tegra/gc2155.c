/*
 * gc2155.c - gc2155 sensor driver
 *
 * Copyright (c) 2011 - 2012, NVIDIA, All Rights Reserved.
 *
 * Contributors:
 *      Abhinav Sinha <absinha@nvidia.com>
 *
 * Leverage soc380.c
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

/**
 * SetMode Sequence for 640x480. Phase 0. Sensor Dependent.
 * This sequence should put sensor in streaming mode for 640x480
 * This is usually given by the FAE or the sensor vendor.
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <media/gc2155.h>
#include <linux/regulator/consumer.h>

#include "gc2155_tables.h"

/* Focuser single step & full scale transition time truth table
 * in the format of:
 *    index	mode		single step transition	full scale transition
 *	0	0			0			0
 *	1	1			50uS			51.2mS
 *	2	1			100uS			102.3mS
 *	3	1			200uS			204.6mS
 *	4	1			400uS			409.2mS
 *	5	1			800uS			818.4mS
 *	6	1			1600uS			1637.0mS
 *	7	1			3200uS			3274.0mS
 *	8	0			0			0
 *	9	2			50uS			1.1mS
 *	A	2			100uS			2.2mS
 *	B	2			200uS			4.4mS
 *	C	2			400uS			8.8mS
 *	D	2			800uS			17.6mS
 *	E	2			1600uS			35.2mS
 *	F	2			3200uS			70.4mS
 */

/* pick up the mode index setting and its settle time from the above table */
#define SETTLETIME_MS 5

#define POS_LOW (0)
#define POS_HIGH (1023)
#define FPOS_COUNT 1024
#define FOCAL_LENGTH (10.0f)
#define FNUMBER (2.8f)

#define SIZEOF_I2C_TRANSBUF 64

struct gc2155_info {
	int mode;
	struct miscdevice miscdev_info;
	struct i2c_client *i2c_client;
	struct gc2155_platform_data *pdata;
	struct gc2155_config focuser;
	struct gc2155_power_rail regulators;
	int af_fw_loaded;
	struct kobject *kobj;
	struct device *dev;
	u8 i2c_trans_buf[SIZEOF_I2C_TRANSBUF];
};

#if 1
static int gc2155_read_reg(struct i2c_client *client, u8 addr, u8 *val)
{
	#if 0
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
	data[0] = (u8) (addr & 0xff);
	//data[1] = (u8) (addr & 0xff);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data + 2;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err != 2)
		return -EINVAL;

	*val = data[2];
	#else
	*val = i2c_smbus_read_byte_data(client,addr);
	#endif

	return 0;
}
#endif

#ifdef KERNEL_WARNING
static int gc2155_write_reg(struct i2c_client *client, u8 addr, u8 value)
{
	
	int count;
	struct i2c_msg msg[1];
	unsigned char data[4];

	if (!client->adapter)
		return -ENODEV;

	data[0] = addr;
	//data[1] = (u8) (addr & 0xff);
	data[1] = value;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = data;

	count = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (count == ARRAY_SIZE(msg))
		return 0;
	dev_err(&client->dev,
		"gc2155: i2c transfer failed, addr: %x, value: %02x\n",
	       addr, (u32)value);
	return -EIO;
}
#endif

#if 0
static int gc2155_write_bulk_reg(struct i2c_client *client, u8 *data, int len)
{
	int err;
	struct i2c_msg msg;

	if (!client->adapter)
		return -ENODEV;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = len;
	msg.buf = data;

	err = i2c_transfer(client->adapter, &msg, 1);
	if (err == 1)
		return 0;

	dev_err(&client->dev, "gc2155: i2c transfer failed at %x\n",
		(int)data[0] << 8 | data[1]);

	return err;
}
#endif

static int gc2155_init_1600_1200(struct gc2155_info *info)
{
	int err = 0;
	
    err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x80);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x80);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x80);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfc, 0x06);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf2, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf3, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf4, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf5, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf9, 0xfe);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf6, 0x00);  
		/*
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf7, 0x05);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf8, 0x85);    //85 */
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf7, 0x05);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf8, 0x89);
		
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfa, 0x00);  

		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x82, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb3, 0x60);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb4, 0x40);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb5, 0x60);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x03, 0x05);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x04, 0x50);  //63
		
		
		//////mesure window////
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xec, 0x04);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xed, 0x04);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xee, 0x60);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xef, 0x90);  
		
		//////==============anlog
		err = i2c_smbus_write_byte_data(info->i2c_client,0x0a, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x0c, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x0d, 0x04);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x0e, 0xc0);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x0f, 0x06);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x10, 0x58);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x17, 0x14);//0x14
		err = i2c_smbus_write_byte_data(info->i2c_client,0x18, 0x0e);  //0X0A
		err = i2c_smbus_write_byte_data(info->i2c_client,0x19, 0x0c);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x1a, 0x01);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x1b, 0x8b); //48
		err = i2c_smbus_write_byte_data(info->i2c_client,0x1e, 0x88);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x1f, 0x08);  //ÈÈÔë
		err = i2c_smbus_write_byte_data(info->i2c_client,0x20, 0x05);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x21, 0x0f);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x22, 0xf0);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x23, 0xc3);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x24, 0x16);  
		
		////////===========aec
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x01);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x11, 0x20);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x1f, 0x80); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x20, 0x40);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x47, 0x30); //40
		err = i2c_smbus_write_byte_data(info->i2c_client,0x0b, 0x10);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x13, 0x75);  

		//err = i2c_smbus_write_byte_data(info->i2c_client,0x3e, 0x40);//
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb6, 0x03); // aec enable 
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);  
		
		
		err = i2c_smbus_write_byte_data(info->i2c_client,0x3f, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x40, 0x77);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x42, 0x7f);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x43, 0x30);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x5c, 0x08);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x5e, 0x20);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x5f, 0x20);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x60, 0x20);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x61, 0x20);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x62, 0x20);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x63, 0x20);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x64, 0x20);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x65, 0x20);  
		
		
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x80, 0xff);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x81, 0x26);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x03, 0x05);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x04, 0x50);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x84, 0x00);   // 02
		err = i2c_smbus_write_byte_data(info->i2c_client,0x86, 0x02);  
		//err = i2c_smbus_write_byte_data(info->i2c_client,0x87, 0x80);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x8b, 0xbc);  
		//err = i2c_smbus_write_byte_data(info->i2c_client,0xa7, 0x80);  
		//err = i2c_smbus_write_byte_data(info->i2c_client,0xa8, 0x80);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb0, 0x80);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc0, 0x40);  
		
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x01);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc2, 0x25);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc3, 0x23);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc4, 0x15);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc8, 0x0e);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc9, 0x07);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xca, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xbc, 0x28);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xbd, 0x0f);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xbe, 0x0e);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb6, 0x35);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb7, 0x38);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb8, 0x1b);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc5, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc6, 0x0c);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc7, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xcb, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xcc, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xcd, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xbf, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc0, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc1, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb9, 0x04);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xba, 0x28);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xbb, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xaa, 0x14);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xab, 0x07);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xac, 0x14);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xad, 0x11);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xae, 0x0c);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xaf, 0x16);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb0, 0x0f);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb1, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb2, 0x02);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb3, 0x11);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb4, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb5, 0x11);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xd0, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xd2, 0x0b);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xd3, 0x07);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xd8, 0x04);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xda, 0x07);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xdb, 0x05);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xdc, 0x28);//  00
		err = i2c_smbus_write_byte_data(info->i2c_client,0xde, 0x0f);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xdf, 0x11);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xd4, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xd6, 0x26);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xd7, 0x10);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa4, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa5, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa6, 0x70);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa7, 0x20);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa8, 0x50);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa9, 0x44);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa1, 0x80);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa2, 0x80);
		
		////===========cc
		/*err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x02);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa4, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);  */
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x02);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc0, 0x01);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc1, 0x40);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc2, 0xfc);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc3, 0x05);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc4, 0xec);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc5, 0x42);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc6, 0xf8);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc7, 0x40);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc8, 0xf8);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xc9, 0x06);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xca, 0xfd);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xcb, 0x3e);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xcc, 0xf3);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xcd, 0x36);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xce, 0xf6);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xcf, 0x04);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xe3, 0x0c);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xe4, 0x44);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xe5, 0xe5); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00); 
		
		
		//======awb
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x01);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4f, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0x10);///////////00
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0x20); /////////////10
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0x30);  ///////////////20
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x04);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x02);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0x40); //////////////////30
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x04);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x08);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x02);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0x50);  //////////////////////40
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0x60); //////////////////50
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x10);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0x70); /////////////////60
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x20);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0x80); ///////////////////70
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0x90); /////////////////////80
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);           
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0xa0); //////////////////////90
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0xb0); /////////////////a0
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0xc0); //////////////////////////////////b0
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0xd0); //////////////////////////////////c0
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0xe0); ////////////////////////////d0
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4d, 0xf0); /////////////////////////////////f0
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4e, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x4f, 0x01);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe,0x01);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4f,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0x10);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0x20);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0x30);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x04);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x02);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0x40);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x08);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x04);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x04);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0x50);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x08);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x08);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x04);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0x60);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x10);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0x70);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x20);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0x80);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x20);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x20);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0x90);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0xa0);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0xb0);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0xc0);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0xd0);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0xe0);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4d,0xf0);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4e,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x4f,0x01);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x56,0x02);
    err = i2c_smbus_write_byte_data(info->i2c_client,0x5b,0x08);
    err = i2c_smbus_write_byte_data(info->i2c_client,0xfe,0x00);
    err = i2c_smbus_write_byte_data(info->i2c_client,0xec,0x06);
    err = i2c_smbus_write_byte_data(info->i2c_client,0xed,0x06);
    err = i2c_smbus_write_byte_data(info->i2c_client,0xee,0x62);
    err = i2c_smbus_write_byte_data(info->i2c_client,0xef,0x92);

		
		////awb////
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x01);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x50, 0x88);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x52, 0x40);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x56, 0x06);  // 04
		err = i2c_smbus_write_byte_data(info->i2c_client,0x57, 0x20);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x58, 0x01);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x5b, 0x02);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x61, 0xaa);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x62, 0xaa);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x71, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x74, 0x10);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x77, 0x08);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x78, 0xfd);  
		//err = i2c_smbus_write_byte_data(info->i2c_client,0x80, 0x15);  
		//err = i2c_smbus_write_byte_data(info->i2c_client,0x84, 0x0a);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x86, 0x30);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x87, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x88, 0x04);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x8a, 0xc0);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x89, 0x75);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x84, 0x08);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x8b, 0x01);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x8d, 0x70);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x8e, 0x70);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x8f, 0xf4);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x90, 0x65);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x92, 0x60);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x82, 0x02);  //awb enable
		
		//////asde////////
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x01);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x21, 0xbf);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x02);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa4, 0x00); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa5, 0x40); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa2, 0xa0);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa6, 0x80);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa7, 0x80);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xab, 0x31);  
		  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xa9, 0x6f);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb0, 0x99);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb1, 0x34);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xb3, 0x80);  
		//err = i2c_smbus_write_byte_data(info->i2c_client,0x8c, 0xf6);  
		//err = i2c_smbus_write_byte_data(info->i2c_client,0x89, 0x03);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xde, 0xb6);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x38, 0x0f);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x39, 0x60);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x81, 0x26);  
		//	err = i2c_smbus_write_byte_data(info->i2c_client,0x87, 0x80);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x02);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x83, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x84, 0x45);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xd1, 0x24);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xd2, 0x24); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0xdc, 0x30); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0xdd, 0xb8);  //  38
		err = i2c_smbus_write_byte_data(info->i2c_client,0x88, 0x15);//add dndd.intpee
		err = i2c_smbus_write_byte_data(info->i2c_client,0x8c, 0xf6);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x89, 0x03);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x90, 0x6c);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x97, 0x48);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00); 
		///rgb big gamma///

		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe,0x02);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x15,0x0a);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x16,0x12);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x17,0x19);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x18,0x1f);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x19,0x2c);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x1a,0x38);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x1b,0x42);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x1c,0x4e);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x1d,0x63);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x1e,0x76);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x1f,0x87);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x20,0x96);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x21,0xa2);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x22,0xb8);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x23,0xca);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x24,0xd8);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x25,0xe3);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x26,0xf0);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x27,0xf8);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x28,0xfd);  
   err = i2c_smbus_write_byte_data(info->i2c_client,0x29,0xff);  

		
		/////y gamma///////
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x02);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2b, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2c, 0x04);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2d, 0x09);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2e, 0x18);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2f, 0x27);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x30, 0x37);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x31, 0x49);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x32, 0x5c);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x33, 0x7e);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x34, 0xa0);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x35, 0xc0);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x36, 0xe0);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x37, 0xff);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x82, 0xfe);
		
		///durk sun///
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x02);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x40, 0x06);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x41, 0x23);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x42, 0x3f);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x43, 0x06);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x44, 0x80);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x45, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x46, 0x14);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x47, 0x09);  
		////aec exposure///
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x05, 0x00);//
		err = i2c_smbus_write_byte_data(info->i2c_client,0x06, 0xfb);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x07, 0x00);//
		err = i2c_smbus_write_byte_data(info->i2c_client,0x08, 0x18);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x01);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x27, 0x00);//
		err = i2c_smbus_write_byte_data(info->i2c_client,0x28, 0x88);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x29, 0x04);//
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2a, 0xc8);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2b, 0x06);//
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2c, 0x4a);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2d, 0x07);//
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2e, 0x8c);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2f, 0x0a);//
		err = i2c_smbus_write_byte_data(info->i2c_client,0x30, 0x10);  
		err = i2c_smbus_write_byte_data(info->i2c_client,0x3e, 0x40);  
		
		//MIPI
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf2, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf3, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf4, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xf5, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x01);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x0b, 0x90);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x87, 0x10);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);
		
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x03);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x01, 0x03);  //
		err = i2c_smbus_write_byte_data(info->i2c_client,0x02, /*0x37*/0x07);  // 13 17
		err = i2c_smbus_write_byte_data(info->i2c_client,0x03, 0x11);  // 0x11
		err = i2c_smbus_write_byte_data(info->i2c_client,0x06, 0x80);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x11, 0x1E);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x12, 0x80);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x13, 0x0c);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x15, 0x10);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x04, 0x20);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x05, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x17, 0x00);
		
		err = i2c_smbus_write_byte_data(info->i2c_client,0x21, 0x01);//01
		err = i2c_smbus_write_byte_data(info->i2c_client,0x22, 0x02);//02 06
		err = i2c_smbus_write_byte_data(info->i2c_client,0x23, 0x01);//01
		err = i2c_smbus_write_byte_data(info->i2c_client,0x29, 0x02);//02 06
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2a, 0x01);//01
		err = i2c_smbus_write_byte_data(info->i2c_client,0x2b, 0x06);
		 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x10, 0x84);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);
		
//	mdelay(400);
//1600x1200 init	
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfa, 0x11);


		err = i2c_smbus_write_byte_data(info->i2c_client,0x90, 0x01);//crop enable
		err = i2c_smbus_write_byte_data(info->i2c_client,0x95, 0x04);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x96, 0xb0);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x97, 0x06);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x98, 0x40);
              
    err = i2c_smbus_write_byte_data(info->i2c_client,0xfe,0x00);  

		
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x03);
		err = i2c_smbus_write_byte_data(info->i2c_client,0x12, 0x80); 
		err = i2c_smbus_write_byte_data(info->i2c_client,0x13, 0x0c); //LWC SET
		err = i2c_smbus_write_byte_data(info->i2c_client,0x04, 0x20); 
              err = i2c_smbus_write_byte_data(info->i2c_client,0x05, 0x00); 
	      
		err = i2c_smbus_write_byte_data(info->i2c_client,0x10, 0x94);
		err = i2c_smbus_write_byte_data(info->i2c_client,0xfe, 0x00);
	
	printk("gc2155 init err:%d\n",err);
	return err;
}

static int gc2155_write_table(struct gc2155_info *info,
			      struct gc2155_reg table[],
			      struct gc2155_reg override_list[],
			      int num_override_regs)
{
	int err;
	struct gc2155_reg *next;

	for (next = table; next->addr != GC2155_TABLE_END; next++) {
		if (next->addr == GC2155_TABLE_WAIT_MS) {
			msleep(next->val);
			continue;
		}

		//err = gc2155_write_bulk_reg(info->i2c_client,
		//	info->i2c_trans_buf, buf_filled);
		err = i2c_smbus_write_byte_data(info->i2c_client,next->addr,next->val);
		
		if (err)
		{
			printk("next->addr=%x,next->val=%x,err= %d\n",next->addr,next->val,err);
			return err;
		}
			
		printk("next->addr=%x,next->val=%x\n",next->addr,next->val);
	}
	
	return 0;
}

static int gc2155_set_mode(struct gc2155_info *info, struct gc2155_mode *mode)
{
	int sensor_mode;
	int err = -EINVAL;

	pr_info("%s: xres %u yres %u\n", __func__, mode->xres, mode->yres);
	if (mode->xres == 1600 && mode->yres == 1200)
	{
	  sensor_mode = GC2155_MODE_1600x1200;
		err =  gc2155_init_1600_1200(info);
	}
	else {
		pr_err("%s: invalid resolution supplied to set mode %d %d\n",
		       __func__, mode->xres, mode->yres);
		return -EINVAL;
	}

	if (err)
		return err;

	info->mode = sensor_mode;
	return 0;
}

static int gc2155_set_power(struct gc2155_info *info, u32 level)
{
	switch (level) {
	case GC2155_POWER_LEVEL_OFF:
	case GC2155_POWER_LEVEL_SUS:
		if (info->pdata && info->pdata->power_off)
			info->pdata->power_off(&info->regulators);
		info->af_fw_loaded = 0;
		info->mode = 0;
		break;
	case GC2155_POWER_LEVEL_ON:
		if (info->pdata && info->pdata->power_on)
			info->pdata->power_on(&info->regulators);
		break;
	default:
		dev_err(info->dev, "unknown power level %d.\n", level);
		return -EINVAL;
	}

	return 0;
}

static int gc2155_set_wb(struct gc2155_info *info, u8 val)
{
	int err = 0;

	switch (val) {
	case GC2155_WB_AUTO:
		err = gc2155_write_table(info, wb_table[GC2155_WB_AUTO], NULL, 0);
		break;
	case GC2155_WB_INCANDESCENT:
		err = gc2155_write_table(info, wb_table[GC2155_WB_INCANDESCENT], NULL, 0);
		break;
	case GC2155_WB_DAYLIGHT:
		err = gc2155_write_table(info, wb_table[GC2155_WB_DAYLIGHT], NULL, 0);
		break;
	case GC2155_WB_FLUORESCENT:
		err = gc2155_write_table(info, wb_table[GC2155_WB_FLUORESCENT], NULL, 0);
		break;
	case GC2155_WB_CLOUDY:
		err = gc2155_write_table(info, wb_table[GC2155_WB_CLOUDY], NULL, 0);
		break;
	default:
		dev_err(info->dev, "this wb setting not supported!\n");
		return -EINVAL;
	}

	return err;
}

#if 1
static int gc2155_read_id(struct gc2155_info *info)
{

	int sensor_id = 0;
	u8 valh = 0;
	u8 vall = 0;
	printk("%s\n",__func__);

  gc2155_read_reg(info->i2c_client, 0xf0, &valh);
	gc2155_read_reg(info->i2c_client, 0xf1, &vall);
	sensor_id = (valh << 8) | vall;
	printk("%s sensorid:%x",__func__,sensor_id);
   return 0;
}
#endif

static long gc2155_ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	struct gc2155_info *info = file->private_data;

	switch (cmd) {
	case GC2155_IOCTL_SET_SENSOR_MODE:
	{
		struct gc2155_mode mode;
		if (copy_from_user(&mode,
				   (const void __user *)arg,
				   sizeof(struct gc2155_mode))) {
			return -EFAULT;
		}

		return gc2155_set_mode(info, &mode);
	}
	case GC2155_IOCTL_POWER_LEVEL:
		return gc2155_set_power(info, (u32)arg);
	case GC2155_IOCTL_GET_SENSOR_STATUS:
	{
		u8 status = 0;
		if (copy_to_user((void __user *)arg, &status,
				 1)) {
			dev_info(info->dev, "%s %d\n", __func__, __LINE__);
			return -EFAULT;
		}
		return 0;
	}
	case GC2155_IOCTL_SET_WB:
		gc2155_read_id(info);
		return gc2155_set_wb(info, (u8)arg);
	default:
		return -EINVAL;
	}
	return 0;
}

static int gc2155_open(struct inode *inode, struct file *file)
{
	struct miscdevice *miscdev = file->private_data;
	struct gc2155_info *info;

	pr_info("%s\n", __func__);
	if (!miscdev) {
		pr_err("miscdev == NULL\n");
		return -1;
	}
	info = container_of(miscdev, struct gc2155_info, miscdev_info);
	file->private_data = info;

	return 0;
}

int gc2155_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	pr_info("%s\n", __func__);
	return 0;
}

static const struct file_operations gc2155_fileops = {
	.owner = THIS_MODULE,
	.open = gc2155_open,
	.unlocked_ioctl = gc2155_ioctl,
	.release = gc2155_release,
};

static struct miscdevice gc2155_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gc2155",
	.fops = &gc2155_fileops,
};

static int gc2155_regulator_get(
	struct gc2155_info *info, struct regulator **vreg, char vreg_name[])
{
	struct regulator *reg = NULL;
	int err = 0;

	reg = regulator_get(&info->i2c_client->dev, vreg_name);
	if (IS_ERR(reg)) {
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

static void gc2155_pm_init(struct gc2155_info *info)
{
	int err;
	struct gc2155_power_rail *pw = &info->regulators;
    gc2155_regulator_get(info, &pw->vdd, "avdd_osc");
	gc2155_regulator_get(info, &pw->vpp_fuse, "cam2_fuse");
	gc2155_regulator_get(info, &pw->avdd, "avdd");
	gc2155_regulator_get(info, &pw->vdd_cam_1v8,"vdd_cam_1v8");
	
	err = regulator_enable(pw->vdd_cam_1v8);
	if (unlikely(err)){
	   dev_err(&info->i2c_client->dev, "%s ERR: enable vdd cam 1v8\n",
		__func__);
	
	}else
	{		
	   regulator_disable(pw->vdd_cam_1v8);
	}

}

static int gc2155_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct gc2155_info *info;
	int err;

	dev_info(&client->dev, "gc2155: probing sensor.\n");

	info = devm_kzalloc(&client->dev,
			sizeof(struct gc2155_info), GFP_KERNEL);
	if (!info) {
		dev_err(&client->dev, "gc2155: Unable to allocate memory!\n");
		return -ENOMEM;
	}

	info->i2c_client = client;
	if (client->dev.platform_data) {
		info->pdata = client->dev.platform_data;
	}

	i2c_set_clientdata(client, info);

	gc2155_pm_init(info);

	memcpy(&(info->miscdev_info),
		&gc2155_device,
		sizeof(struct miscdevice));

	err = misc_register(&(info->miscdev_info));
	if (err) {
		dev_err(&client->dev,
			"gc2155: Unable to register misc device!\n");
		devm_kfree(&client->dev, info);
		return err;
	}

	//client->addr = 0x78;
	printk("gc2155 probe i2c addr:%x\n",client->addr);
	info->dev = &client->dev;
	info->pdata = client->dev.platform_data;
	info->i2c_client = client;
	info->focuser.settle_time = SETTLETIME_MS;
	info->focuser.focal_length = FOCAL_LENGTH;
	info->focuser.fnumber = FNUMBER;
	info->focuser.pos_low = POS_LOW;
	info->focuser.pos_high = POS_HIGH;

	i2c_set_clientdata(client, info);
	//gc2155_read_id(info);
	return 0;
}

static int gc2155_remove(struct i2c_client *client)
{
	struct gc2155_info *info;
	info = i2c_get_clientdata(client);
	misc_deregister(&(info->miscdev_info));
	return 0;
}

static const struct i2c_device_id gc2155_id[] = {
	{ "gc2155", 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, gc2155_id);

static struct i2c_driver gc2155_i2c_driver = {
	.driver = {
		.name = "gc2155",
		.owner = THIS_MODULE,
	},
	.probe = gc2155_probe,
	.remove = gc2155_remove,
	.id_table = gc2155_id,
};

static int __init gc2155_init(void)
{
	pr_info("gc2155 sensor driver loading\n");
	return i2c_add_driver(&gc2155_i2c_driver);
}

static void __exit gc2155_exit(void)
{
	i2c_del_driver(&gc2155_i2c_driver);
}

module_init(gc2155_init);
module_exit(gc2155_exit);
MODULE_LICENSE("GPL v2");
