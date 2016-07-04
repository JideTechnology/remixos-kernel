/*
 *
 * Copyright (c) 2014 Intel Corporation. All Rights Reserved.
 *
*/

/*
	tbd:
		1> multi resolution
		2> multi res type: video preview
		4> v4l2-ctrl
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
//#include <media/v4l2-chip-ident.h>
#include <linux/io.h>
#include <linux/firmware.h>
#include "unicam.h"
#include "vcm.h"
#include <linux/acpi.h>
#include <linux/atomisp_gmin_platform.h>
#include "unicam_hw.h"

#define UNICAM_EXPOSURE_LOG 0
#define UNICAM_DEBUG_LOG 0
#define UNICAM_ALL_LOG 0

#if UNICAM_EXPOSURE_LOG
#define MYDBGEXP(fmt,...) printk("%s"fmt,"unicam exposure:",##__VA_ARGS__)
#else
#define MYDBGEXP(fmt,...)
#endif

#if UNICAM_DEBUG_LOG
#define MYDBGNP(fmt, ...) printk(fmt, ##__VA_ARGS__)
#define MYDBG(fmt, ...) printk("%s" fmt, "unicam:", ##__VA_ARGS__)
#else
#define MYDBGNP(fmt, ...)
#define MYDBG(fmt, ...)
#endif


#if UNICAM_ALL_LOG
#define MYDBGIN() printk("%s %s enter\n","unicam:",__func__)
#define MYDBGFMT(fmt,...) printk("%s"fmt,"unicam fmt:",##__VA_ARGS__)
#else
#define MYDBGIN()
#define MYDBGFMT(fmt,...)
#endif

#define OV5648_OTP
//#define OV5648_OTP_DUMP 

/* i2c read/write stuff */
static int uni_read_reg(struct i2c_client *client,
			   u16 data_length, u16 reg, u16 *val)
{
	int err;
	/* int i; */
	struct i2c_msg msg[2];
	unsigned char data[6];
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sensor_data *dev = to_sensor_data(sd);
	if (!client->adapter) {
		dev_err(&client->dev, "%s error, no client->adapter\n",
			__func__);
		return -ENODEV;
	}

	MYDBG("%s data_length:0x%x reg:0x%x \n",__func__,data_length,reg);

	if (data_length != UNICAM_8BIT && data_length != UNICAM_16BIT
					&& data_length != UNICAM_32BIT) {
		dev_err(&client->dev, "%s error, invalid data length\n",
			__func__);
		return -EINVAL;
	}

	memset(msg, 0 , sizeof(msg));

	msg[0].addr = client->addr;//dev->uni->i2c_addr?dev->uni->i2c_addr:client->addr;
	msg[0].flags = 0;

	if( dev->uni->flag & UNI_DEV_FLAG_1B_ADDR ){
		data[0] = (u8)(reg & 0xff);
		msg[0].len = 1;
	}else{
		/* high byte goes out first */
		data[0] = (u8)(reg >> 8);
		data[1] = (u8)(reg & 0xff);
		msg[0].len = 2;
	}
	msg[0].buf = data;

//	MYDBG("i2c dbgr: %d,%d:",msg[0].len,data_length);
//	for(i=0;i<msg[0].len;i++)
//		MYDBGNP(" %x ",msg[0].buf[i]);
//	MYDBGNP("\n");

	msg[1].addr =  client->addr;//dev->uni->i2c_addr?dev->uni->i2c_addr:client->addr;
	msg[1].len = data_length;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = data;

	err = i2c_transfer(client->adapter, msg, 2);
	if (err != 2) {
		if (err >= 0)
			err = -EIO;
		dev_err(&client->dev,
			"read from offset 0x%x error %d", reg, err);
		return err;
	}

	*val = 0;
	/* high byte comes first */
	if (data_length == UNICAM_8BIT)
		*val = (u8)data[0];
	else if (data_length == UNICAM_16BIT)
		*val = be16_to_cpu(*(u16 *)&data[0]);
	else
		*val = be32_to_cpu(*(u32 *)&data[0]);

//	MYDBG(" val: 0x%x\n",*val);

	return 0;
}

static int uni_i2c_write(struct i2c_client *client, u16 len, u8 *data)
{
	struct i2c_msg msg;
	const int num_msg = 1;
	int ret;
//	int i;
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sensor_data *dev = to_sensor_data(sd);

	if( dev->uni->flag & UNI_DEV_FLAG_1B_ADDR ){
		//BE, so it is easy, default is 16
		len--;
		data++;
	}

#if 0
	if (dev->uni->flag & UNI_DEV_FLAG_DBGI2C) {
		MYDBG("i2c dbgw: %d,", len);
		for (i = 0; i < len && i < 10; i++)
			MYDBGNP(" %x ", data[i]);
		MYDBGNP("\n");
	}
#endif
	/* dev->uni->i2c_addr?dev->uni->i2c_addr:client->addr; */
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = len;
	msg.buf = data;

	ret = i2c_transfer(client->adapter, &msg, 1);

	if(dev->uni->flag & UNI_DEV_FLAG_I2C_DELAY_100NS ) {
		usleep_range(100, 150);
	}

	return ret == num_msg ? 0 : -EIO;
}

static int uni_write_reg(struct i2c_client *client, u16 data_length,
							u16 reg, u16 val)
{
	int ret;
	unsigned char data[4] = {0};
	u16 *wreg = (u16 *)data;
	const u16 len = data_length + sizeof(u16); /* 16-bit address + data */

//	MYDBG("%s data_length:0x%x reg:0x%x val:0x%x\n",__func__,data_length,reg,val);

	if (data_length != UNICAM_8BIT && data_length != UNICAM_16BIT) {
		dev_err(&client->dev,
			"%s error, invalid data_length\n", __func__);
		return -EINVAL;
	}

	/* high byte goes out first */
	*wreg = cpu_to_be16(reg);

	if (data_length == UNICAM_8BIT) {
		data[2] = (u8)(val);
	} else {
		/* UNICAM_16BIT */
		u16 *wdata = (u16 *)&data[2];
		*wdata = cpu_to_be16(val);
	}

	ret = uni_i2c_write(client, len, data);
	if (ret)
		dev_err(&client->dev,
			"write error: wrote 0x%x to offset 0x%x error %d",
			val, reg, ret);

	return ret;
}

static int uni_write_reg_array(struct i2c_client *client,
				  const S_UNI_REG *reglist)
{
	const S_UNI_REG *next = reglist;

	int err=0;

	for (; next->type != UNICAM_TOK_TERM; next++) {
		switch (next->type ) {
		case UNICAM_TOK_DELAY:
			msleep(next->val);
			MYDBG("sleep %d\n",next->val);
			break;
		case UNICAM_8BIT:
		case UNICAM_16BIT:
		case UNICAM_32BIT:
			err=uni_write_reg(client, next->type,next->reg,next->val);
			break;
		default:
			break;
		}
	}

	return err;
}
/*
	tbd:
*/
static int uni_get_intg_factor(struct i2c_client *client,
				struct camera_mipi_info *info,
				const S_UNI_RESOLUTION *res)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sensor_data *dev = to_sensor_data(sd);
	struct atomisp_sensor_mode_data *buf = &info->data;

	/* get PCLK time */
	buf->vt_pix_clk_freq_mhz = res->vt_pix_clk_freq_mhz;

	/* get integration time */
	buf->coarse_integration_time_min = dev->uni->coarse_integration_time_min;
	buf->coarse_integration_time_max_margin =dev->uni->coarse_integration_time_max_margin;

	buf->fine_integration_time_min = dev->uni->fine_integration_time_min;
	buf->fine_integration_time_max_margin =dev->uni->fine_integration_time_max_margin;

	buf->fine_integration_time_def = dev->uni->fine_integration_time_min;
	buf->frame_length_lines = res->lines_per_frame;
	buf->line_length_pck = res->pixels_per_line;
	buf->read_mode = res->bin_mode;

	buf->crop_horizontal_start=res->crop_horizontal_start;
	buf->crop_vertical_start=res->crop_vertical_start;
	buf->crop_horizontal_end=res->crop_horizontal_end;
	buf->crop_vertical_end=res->crop_vertical_end;
	buf->output_width=res->output_width;
	buf->output_height=res->output_height;

	buf->binning_factor_x = res->bin_factor_x ?
					res->bin_factor_x : 1;
	buf->binning_factor_y = res->bin_factor_y ?
					res->bin_factor_y : 1;

	MYDBGFMT("vt_pix_clk_freq_mhz %d\n",buf->vt_pix_clk_freq_mhz );
	MYDBGFMT("coarse_integration_time_min %d\n",buf->coarse_integration_time_min );
	MYDBGFMT("coarse_integration_time_max_margin %d\n",buf->coarse_integration_time_max_margin );
	MYDBGFMT("fine_integration_time_min %d\n",buf->fine_integration_time_min );
	MYDBGFMT("fine_integration_time_max_margin %d\n",buf->fine_integration_time_max_margin );
	MYDBGFMT("fine_integration_time_def %d\n",buf->fine_integration_time_def );
	MYDBGFMT("frame_length_lines %d\n",buf->frame_length_lines );
	MYDBGFMT("line_length_pck %d\n",buf->line_length_pck );
	MYDBGFMT("read_mode %d\n",buf->read_mode );
	MYDBGFMT("crop_horizontal_start %d\n",buf->crop_horizontal_start);
	MYDBGFMT("crop_vertical_start %d\n",buf->crop_vertical_start);
	MYDBGFMT("crop_horizontal_end %d\n",buf->crop_horizontal_end);
	MYDBGFMT("crop_vertical_end %d\n",buf->crop_vertical_end);
	MYDBGFMT("output_width %d\n",buf->output_width);
	MYDBGFMT("output_height %d\n",buf->output_height);
	MYDBGFMT("binning_factor_x %d\n",buf->binning_factor_x);
	MYDBGFMT("binning_factor_y %d\n",buf->binning_factor_y);

	return 0;
}

static int getshift(int v)
{
		int i=0;

		for(i=0;i<32;i++){
			if(v & (1<<i)) break;
		}
		MYDBG("val:0x%x getshift:%d\n",v,i);
		return i;
}

static int getbyte(int v)
{
		int ret=0;
		if(v & 0xff000000) ret=4;
		else if(v & 0x00ff0000) ret=3;
		else if(v & 0x0000ff00) ret=2;
		else if(v & 0x000000ff) ret=1;

		return ret;
}

static int unicam_getLZeroBits(int val){
	int i =0;

	if(val == 0) return 32;//only for optimize when 0

	for(i=0;i<32;i++){
		if(val & 0x1<<i){
			break;
		}
	}
	MYDBG("val:0x%x zero count:%d\n",val,i);
	return i;
}


static int unicam_read_i2c_with_mask(struct i2c_client *client, u16 start_reg, u32 mask,int * value, char * name){
	int ret = 0;
	int val = 0;
	u16 regval = 0;
	int im = 0;
	int i = 0;
	int sft = 0;
	int iregidx = 0;
	MYDBG("%s start_reg:0x%x mask:0x%x value:0x%x\n",name,start_reg,mask,*value);
	if(mask == 0) return ret;// mask ==0, return OK.

	if(start_reg <0) return ret;//start_reg < 0, invalid.

	for(i=0; i<4; i++){
		int j = 3-i;
		im = mask & (0xFF<<(j*8));
		if(im == 0)continue;

		ret = uni_read_reg(client, UNICAM_8BIT,(u16)start_reg+iregidx, &regval);
		MYDBG("reg:0x%x regval:0x%x\n",start_reg+iregidx,regval);
		iregidx++;
		if (ret)
			goto err;
		val = val |((regval<<(j*8))&im);
	}
	sft = unicam_getLZeroBits(mask);
	MYDBG("%s mask:0x%x sft:0x%x\n",name,mask,sft);
	val = val>>(sft);
	MYDBG("%s regval:0x%x val:0x%x\n",name,regval,val);
	*value = val;
	MYDBG("%s value:0x%x",name,*value);
err:
	return ret;
}

static int unicam_write_i2c_with_mask(struct i2c_client *client,u16 start_reg, u32 mask,int value, char * name){
	int ret = 0;
	u16 regval = 0;
	int im = 0;
	int i = 0;
	int sft = 0;
	int iregidx = 0;
	u16 regval_get=0;

	MYDBG("%s start_reg:0x%x mask:0x%x value:0x%x\n",name,start_reg,mask,value);

	if (mask == 0)
		return ret; /* mask ==0, return OK. */

	if (start_reg < 0)
		return ret; /* start_reg < 0, invalid. */

	sft = unicam_getLZeroBits(mask);
	MYDBG("%s start_reg:0x%x mask:0x%x value:0x%x\n",name,start_reg,mask,value);
	//? check max here or in caller.
	if(value > (mask>>sft)) value = (mask >> sft);

	value = value << sft;

	for(i=0; i<4; i++){
		int j = 3-i;
		im = mask & (0xFF<<(j*8));
		if(im == 0)continue;
		regval = (value & im)>>(j*8);

		if(im != (0xFF<<(j*8))){//optimize
			ret = uni_read_reg(client, UNICAM_8BIT,(u16)start_reg+iregidx, &regval_get);
			MYDBG("reg:0x%x regval:0x%x\n",start_reg+iregidx,regval);
			regval = regval|(regval_get&(~(im>>(j*8))));
		}

		ret = uni_write_reg(client, UNICAM_8BIT,(u16)start_reg+iregidx, regval);
		MYDBG("reg:0x%x regval:0x%x \n",start_reg+iregidx,regval);

		iregidx++;
		if (ret)
			goto err;
		MYDBG("%s mask:0x%x im:0x%x value:0x%x regval:0x%x \n",name,mask,im,value,regval);
	}
err:
	return ret;

}


//OTP Start
#ifdef OV5648_OTP
/* Add OTP operation
*/
#define BG_RATIO_TYPICAL  0x16E
#define RG_RATIO_TYPICAL  0x189

struct otp_struct {
		u16 module_integrator_id;
		u16 lens_id;
		u16 rg_ratio;
		u16 bg_ratio;
		u16 user_data[2];
		u16 light_rg;
		u16 light_bg;
		int r_gain;
		int g_gain;
		int b_gain;
};

/*
 *Camera driver need to load AWB calibration data
 *stored in OTP and write to gain registers after
 *initialization of register settings.
 * index: index of otp group. (1, 2, 3)
 * return: 0, group index is empty
 *		1, group index has invalid data
 *		2, group index has valid data
 */
static int ov5648_check_otp(struct v4l2_subdev *sd, int index)
{
	int i;
	u16 rg = 0, bg = 0, flag = 0;
	struct sensor_data *dev = to_sensor_data(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	/* clear otp buffer */
	for (i = 0; i < 16; i++)
		uni_write_reg(client, UNICAM_8BIT, 0x3d00 + i, 0x00);
	
	if(index == 1) //Group 1
	{
		//read otp bank 0
	    uni_write_reg(client, UNICAM_8BIT,0x3d84,0xc0);	
		uni_write_reg(client, UNICAM_8BIT,0x3d85,0x00); //OTP start address, bank 0
		uni_write_reg(client, UNICAM_8BIT,0x3d86,0x0f); //OTP end address
	    uni_write_reg(client, UNICAM_8BIT,0x3d81,0x01); //OTP Read Enabl
		msleep(10); // delay 5ms
		
	    uni_read_reg(client, UNICAM_8BIT,0x3d05, &flag);	
	    uni_read_reg(client, UNICAM_8BIT,0x3d07, &rg);	
	    uni_read_reg(client, UNICAM_8BIT,0x3d08, &bg);	
	}else if(index == 2)  //Group 2
    {

		//read otp bank 0
	    uni_write_reg(client, UNICAM_8BIT,0x3d84,0xc0);	
		uni_write_reg(client, UNICAM_8BIT,0x3d85,0x00); //OTP start address, bank 0
		uni_write_reg(client, UNICAM_8BIT,0x3d86,0x0f); //OTP end address
	    uni_write_reg(client, UNICAM_8BIT,0x3d81,0x01); //OTP Read Enable

		msleep(10); // delay 5ms
		
	    uni_read_reg(client, UNICAM_8BIT,0x3d0e, &flag);	
    	//read otp bank 1
        uni_write_reg(client, UNICAM_8BIT,0x3d84,0xc0);	
		uni_write_reg(client, UNICAM_8BIT,0x3d85,0x10); //OTP start address, bank 0
		uni_write_reg(client, UNICAM_8BIT,0x3d86,0x1f); //OTP end address
	    uni_write_reg(client, UNICAM_8BIT,0x3d81,0x01); //OTP Read Enable        
		msleep(10); // delay 5ms

	    uni_read_reg(client, UNICAM_8BIT,0x3d00, &rg);	
	    uni_read_reg(client, UNICAM_8BIT,0x3d01, &bg);	

    }else if(index == 3)  //Group 2
    {
    	//read otp bank 1
        uni_write_reg(client, UNICAM_8BIT,0x3d84,0xc0);	
		uni_write_reg(client, UNICAM_8BIT,0x3d85,0x10); //OTP start address, bank 0
		uni_write_reg(client, UNICAM_8BIT,0x3d86,0x1f); //OTP end address
	    uni_write_reg(client, UNICAM_8BIT,0x3d81,0x01); //OTP Read Enable        
		msleep(10); // delay 5ms

	    uni_read_reg(client, UNICAM_8BIT,0x3d07, &flag);	
	    uni_read_reg(client, UNICAM_8BIT,0x3d09, &rg);	
	    uni_read_reg(client, UNICAM_8BIT,0x3d0a, &bg);	

    }
	msleep(10); // delay 5ms

#ifdef OV5648_OTP_DUMP 
	for (i = 0; i < 16; i++)
	{
        uni_read_reg(client, UNICAM_8BIT,0x3d00 + i, &rg);	
	 	printk("=================== ov5648 dump rg_temp=%d --0x%x ==================\n",rg,rg);
		rg = 0;
	}
#endif

	//disable otp read
	uni_write_reg(client, UNICAM_8BIT, 0x3d81, 0x00);

	/* clear otp buffer */
	for (i = 0; i < 16; i++)
		uni_write_reg(client, UNICAM_8BIT, 0x3d00 + i, 0x00);

	flag = flag & 0x80;
	if (flag)
	{
		return 1;
	} else {
	if (rg == 0 && bg == 0)
		return 0;
	else
		return 2;
	}
	/*
	return 0 empty 
	return 1 invalid 
	return 2 valid
	*/
}

/* index: index of otp group. (1, 2, 3)
 * return: 0,
 */
static int ov5648_read_otp(struct v4l2_subdev *sd,
	    int index, struct otp_struct *otp_ptr)
{
	int i;
	u16 temp;
	
	struct sensor_data *dev = to_sensor_data(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (index == 1) {
		/* read otp --Bank 0 */
		uni_write_reg(client, UNICAM_8BIT, 0x3d84, 0xc0);
		uni_write_reg(client, UNICAM_8BIT, 0x3d85, 0x00);
		uni_write_reg(client, UNICAM_8BIT, 0x3d86, 0x0f);
		uni_write_reg(client, UNICAM_8BIT, 0x3d81, 0x01);
		msleep(10); // delay 5ms
		uni_read_reg(client, UNICAM_8BIT,
			0x3d05, &((*otp_ptr).module_integrator_id));
		(*otp_ptr).module_integrator_id =
			(*otp_ptr).module_integrator_id & 0x7f;
		uni_read_reg(client, UNICAM_8BIT,
			0x3d06, &((*otp_ptr).lens_id));
		uni_read_reg(client, UNICAM_8BIT, 0x3d0b, &temp);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d07, &((*otp_ptr).rg_ratio));
		(*otp_ptr).rg_ratio =
			((*otp_ptr).rg_ratio<<2) + ((temp>>6) & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d08, &((*otp_ptr).bg_ratio));
		(*otp_ptr).bg_ratio =
			((*otp_ptr).bg_ratio<<2) + ((temp>>4) & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d0c, &((*otp_ptr).light_rg));
		(*otp_ptr).light_rg =
			((*otp_ptr).light_rg<<2) + ((temp>>2) & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d0d, &((*otp_ptr).light_bg));
		(*otp_ptr).light_bg =
			((*otp_ptr).light_bg<<2) + (temp & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d09, &((*otp_ptr).user_data[0]));
		uni_read_reg(client, UNICAM_8BIT,
			0x3d0a, &((*otp_ptr).user_data[1]));
	}else if (index == 2) {
		/* read otp --Bank 0 */
		uni_write_reg(client, UNICAM_8BIT, 0x3d84, 0xc0);
		uni_write_reg(client, UNICAM_8BIT, 0x3d85, 0x00);
		uni_write_reg(client, UNICAM_8BIT, 0x3d86, 0x0f);
		uni_write_reg(client, UNICAM_8BIT, 0x3d81, 0x01);
		msleep(10); // delay 5ms
		uni_read_reg(client, UNICAM_8BIT,
			0x3d0e, &((*otp_ptr).module_integrator_id));
		(*otp_ptr).module_integrator_id =
			(*otp_ptr).module_integrator_id & 0x7f;
		uni_read_reg(client, UNICAM_8BIT,
			0x3d0f, &((*otp_ptr).lens_id));
		/* read otp --Bank 1 */
		uni_write_reg(client, UNICAM_8BIT, 0x3d84, 0xc0);
		uni_write_reg(client, UNICAM_8BIT, 0x3d85, 0x10);
		uni_write_reg(client, UNICAM_8BIT, 0x3d86, 0x1f);
		uni_write_reg(client, UNICAM_8BIT, 0x3d81, 0x01);
		msleep(10); // delay 5ms
		uni_read_reg(client, UNICAM_8BIT, 0x3d04, &temp);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d00, &((*otp_ptr).rg_ratio));
		(*otp_ptr).rg_ratio =
			((*otp_ptr).rg_ratio<<2) + ((temp>>6) & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d01, &((*otp_ptr).bg_ratio));
		(*otp_ptr).bg_ratio =
			((*otp_ptr).bg_ratio<<2) + ((temp>>4) & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d05, &((*otp_ptr).light_rg));
		(*otp_ptr).light_rg =
			((*otp_ptr).light_rg<<2) + ((temp>>2) & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d06, &((*otp_ptr).light_bg));
		(*otp_ptr).light_bg =
			((*otp_ptr).light_bg<<2) + (temp & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d02, &((*otp_ptr).user_data[0]));
		uni_read_reg(client, UNICAM_8BIT,
			0x3d03, &((*otp_ptr).user_data[1]));
	}else if (index == 3) {
		/* read otp --Bank 1 */
		uni_write_reg(client, UNICAM_8BIT, 0x3d84, 0xc0);
		uni_write_reg(client, UNICAM_8BIT, 0x3d85, 0x10);
		uni_write_reg(client, UNICAM_8BIT, 0x3d86, 0x1f);
		uni_write_reg(client, UNICAM_8BIT, 0x3d81, 0x01);
		msleep(10); // delay 5ms
		uni_read_reg(client, UNICAM_8BIT,
			0x3d07, &((*otp_ptr).module_integrator_id));
		(*otp_ptr).module_integrator_id =
			(*otp_ptr).module_integrator_id & 0x7f;
		uni_read_reg(client, UNICAM_8BIT,
			0x3d08, &((*otp_ptr).lens_id));
		uni_read_reg(client, UNICAM_8BIT, 0x3d0d, &temp);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d09, &((*otp_ptr).rg_ratio));
		(*otp_ptr).rg_ratio =
			((*otp_ptr).rg_ratio<<2) + ((temp>>6) & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d0a, &((*otp_ptr).bg_ratio));
		(*otp_ptr).bg_ratio =
			((*otp_ptr).bg_ratio<<2) + ((temp>>4) & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d0e, &((*otp_ptr).light_rg));
		(*otp_ptr).light_rg =
			((*otp_ptr).light_rg<<2) + ((temp>>2) & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d0f, &((*otp_ptr).light_bg));
		(*otp_ptr).light_bg =
			((*otp_ptr).light_bg<<2) + (temp & 0x03);
		uni_read_reg(client, UNICAM_8BIT,
			0x3d0b, &((*otp_ptr).user_data[0]));
		uni_read_reg(client, UNICAM_8BIT,
			0x3d0c, &((*otp_ptr).user_data[1]));
	}
	/* clear otp buffer */
	for (i = 0; i < 16; i++)
		uni_write_reg(client, UNICAM_8BIT, 0x3d00 + i, 0x00);

	return 0;
}
/* call this function after OV5648 initialization
 * return: 0 update success
 *		1, no OTP
 */
static int ov5648_update_otp(struct v4l2_subdev *sd)
{
	struct otp_struct current_otp;
	struct sensor_data *dev = to_sensor_data(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, ret;
	int otp_index;
	u16 temp;
	int r_gain, g_gain, b_gain, g_gain_r, g_gain_b;
	u16 rg = 1, bg = 1;

	/* R/G and B/G of current camera module is read out from sensor OTP
	 * check first OTP with valid data
	 */
	for(i = 1; i <= 3; i++) {
		if (ov5648_check_otp(sd, i) == 2) {
			otp_index = i;
			break;
		}
	}
	if(i > 3)
	{  
		//no valid wb OTP data
	 	printk("=================== ov5648 no valid wb OTP data!==================");
		return 1;
	}
	ov5648_read_otp(sd, otp_index, &current_otp);
	
	if (current_otp.light_rg == 0) {
		/* no light source information in OTP */
		rg = current_otp.rg_ratio;
	} else {
		/* light source information found in OTP */
		rg = current_otp.rg_ratio * (current_otp.light_rg + 512) / 1024;
	}
	if (current_otp.light_bg == 0) {
		/* no light source information in OTP */
		bg = current_otp.bg_ratio;
	} else {
		/* light source information found in OTP */
		bg = current_otp.bg_ratio * (current_otp.light_bg + 512) / 1024;
	}
	MYDBGEXP("_ov5648_: %s :rg:%x bg %x\n", __func__, rg, bg);
	if (rg == 0)
		rg = 1;
	if (bg == 0)
		bg = 1;
	/*calculate G gain
	 *0x400 = 1x gain
	 */
	if (bg < BG_RATIO_TYPICAL) {
		if (rg < RG_RATIO_TYPICAL) {
			/* current_otp.bg_ratio < BG_RATIO_TYPICAL &&
			 * current_otp.rg_ratio < RG_RATIO_TYPICAL
			 */
			g_gain = 0x400;
			b_gain = 0x400 * BG_RATIO_TYPICAL / bg;
			r_gain = 0x400 * RG_RATIO_TYPICAL / rg;
		} else {
			/* current_otp.bg_ratio < BG_RATIO_TYPICAL &&
			 * current_otp.rg_ratio >= RG_RATIO_TYPICAL
			 */
			r_gain = 0x400;
			g_gain = 0x400 * rg / RG_RATIO_TYPICAL;
			b_gain = g_gain * BG_RATIO_TYPICAL / bg;
		}
	} else {
		if (rg < RG_RATIO_TYPICAL) {
			/* current_otp.bg_ratio >= BG_RATIO_TYPICAL &&
			 * current_otp.rg_ratio < RG_RATIO_TYPICAL
			 */
			b_gain = 0x400;
			g_gain = 0x400 * bg / BG_RATIO_TYPICAL;
			r_gain = g_gain * RG_RATIO_TYPICAL / rg;
		} else {
			/* current_otp.bg_ratio >= BG_RATIO_TYPICAL &&
			 * current_otp.rg_ratio >= RG_RATIO_TYPICAL
			 */
			g_gain_b = 0x400 * bg / BG_RATIO_TYPICAL;
			g_gain_r = 0x400 * rg / RG_RATIO_TYPICAL;
			if (g_gain_b > g_gain_r) {
				b_gain = 0x400;
				g_gain = g_gain_b;
				r_gain = g_gain * RG_RATIO_TYPICAL / rg;
			} else {
				r_gain = 0x400;
				g_gain = g_gain_r;
				b_gain = g_gain * BG_RATIO_TYPICAL / bg;
			}
		}
	}

	//write sensor wb gain to register
	if (r_gain > 0x400) {
		uni_write_reg(client, UNICAM_8BIT, 0x5186, r_gain>>8);
		uni_write_reg(client, UNICAM_8BIT, 0x5187, r_gain & 0x00ff);
	}
	
	if (g_gain > 0x400) {
		uni_write_reg(client, UNICAM_8BIT, 0x5188, g_gain>>8);
		uni_write_reg(client, UNICAM_8BIT, 0x5189, g_gain & 0x00ff);
	}
	
	if (b_gain > 0x400) {
		uni_write_reg(client, UNICAM_8BIT, 0x518a, b_gain>>8);
		uni_write_reg(client, UNICAM_8BIT, 0x518b, b_gain & 0x00ff);
	}
	//write sensor wb gain success
	
	return ret;
}

#endif
//OTP End


static long __gc2355_set_exposure(struct v4l2_subdev *sd, int coarse_itg,
				 int gain, int digitgain)
{

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct sensor_data *dev = to_sensor_data(sd);
	long ret = 0;
	int dbgonce = 0;
	int total_gain, ana_gain, global_gain, pre_gain;

	mutex_lock(&dev->input_lock);

#if 0
	vts = dev->cur_res[dev->fmt_idx].vts;
	if (coarse_itg + dev->cur_res[dev->fmt_idx].vts_fix > vts)
		vts = coarse_itg + dev->cur_res[dev->fmt_idx].vts_fix;
#endif

	MYDBGEXP("GC2355 exp: coarse gain digigain %d:%08x %d:%08x %d:%08x\n",
			coarse_itg, coarse_itg, gain,
			gain, digitgain, digitgain);

	ret = uni_write_reg_array(client, dev->uni->group_hold_start);
	if (ret)
		goto exposure_err;

	/* set lines per frame */
#if 0
	ret = unicam_write_i2c_with_mask(client,
		dev->uni->exposevts,
		dev->uni->exposevts_mask,
		vts,
		"set vts");
	if (ret)
		goto exposure_err;
#endif

	MYDBGNP("coarse ################5########## entry %s dev->uni->exposeanaloggain = 0x%x, dev->uni->exposeanaloggain_mask = 0x%x, gain = 0x%x\n",
			__func__,
			dev->uni->exposeanaloggain,
			dev->uni->exposeanaloggain_mask,
			gain);
	/* set exposure */
	/* Lower four bit should be 0*/

    if (coarse_itg < 7) coarse_itg = 7;
    if (coarse_itg > 16383) coarse_itg = 16383;

	ret = uni_write_reg(client, UNICAM_8BIT,
		0x03, (coarse_itg >> 8) & 0x003F);
	if (ret)
		goto exposure_err;

	ret = uni_write_reg(client, UNICAM_8BIT,
		0x04, coarse_itg & 0x00FF);
	if (ret)
		goto exposure_err;



	if (gain < 0x40)
		gain = 0x40;

	if (gain <= GC2355_ANALOG_GAIN_2) //88
    {
		ana_gain = 0x00;
		total_gain = gain;
		global_gain = total_gain >> 6;
		pre_gain = (total_gain << 2) & 0xfc;
    }
	else if (gain < GC2355_ANALOG_GAIN_3) //122
    {
		ana_gain = 0x01;
		total_gain = 64 * gain / GC2355_ANALOG_GAIN_2;
		global_gain = total_gain >> 6;
		pre_gain = (total_gain << 2) & 0xfc;
    }
	else if (gain < GC2355_ANALOG_GAIN_4) //168
    {
		ana_gain = 0x02;
		total_gain = 64 * gain / GC2355_ANALOG_GAIN_3;
		global_gain = total_gain >> 6;
		pre_gain = (total_gain << 2) & 0xfc;
    }
	else
	{
		ana_gain = 0x03;
		total_gain = 64 * gain / GC2355_ANALOG_GAIN_4;
		global_gain = total_gain >> 6;
		pre_gain = (total_gain << 2) & 0xfc;
	}


	/* set analog gain */
	ret = uni_write_reg(client, UNICAM_8BIT,
		0xB6, ana_gain);
	if (ret)
		goto exposure_err;

	ret = uni_write_reg(client, UNICAM_8BIT,
		0xB1, global_gain);
	if (ret)
		goto exposure_err;

	ret = uni_write_reg(client, UNICAM_8BIT,
		0xB2, pre_gain);
	if (ret)
		goto exposure_err;

	dbgonce=1;
	mutex_unlock(&dev->input_lock);
	return ret;

exposure_err:
	dev_err(&client->dev, "unicam write again err.\n");
	mutex_unlock(&dev->input_lock);
	return ret;
}

static long __ov5648_set_exposure(struct v4l2_subdev *sd, int coarse_itg,
				 int gain, int digitgain)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct sensor_data *dev = to_sensor_data(sd);

	int ret=0;
	int exposegain_S;
	int exposedigitgain_S;
	static int dbgonce;
	int dgain_max,again_max;
	int vts;
	int exp_val;

    /* mutex_lock(&dev->input_lock); */

	vts=dev->cur_res[dev->fmt_idx].vts;

	if(coarse_itg+dev->cur_res[dev->fmt_idx].vts_fix > vts){
		vts = coarse_itg+dev->cur_res[dev->fmt_idx].vts_fix;
	}


	MYDBGEXP("OV5648 exp: coarse gain digigain %d:%08x %d:%08x %d:%08x \n",coarse_itg,coarse_itg,gain,gain,digitgain,digitgain);

	ret = uni_write_reg_array(client, dev->uni->group_hold_start);
	if (ret)
		goto exposure_err;

	exposegain_S=getshift(dev->uni->exposeanaloggain_mask);
	exposedigitgain_S=getshift(dev->uni->exposedigitgain_mask);

	again_max=(((u32)dev->uni->exposeanaloggain_mask)>>exposegain_S);
	dgain_max=(((u32)dev->uni->exposedigitgain_mask)>>exposedigitgain_S);


	MYDBGNP("again_max = %d  dgain_max = %d \n",again_max,dgain_max);

	/* set lines per frame */
	ret = unicam_write_i2c_with_mask(client,dev->uni->exposevts,dev->uni->exposevts_mask,vts,"set vts");
	if (ret)
		goto exposure_err;

	MYDBGNP(" coarse ################5########## entry %s dev->uni->exposeanaloggain =0x%x, dev->uni->exposeanaloggain_mask =0x%x ,gain =0x%x \n", __func__, dev->uni->exposeanaloggain,dev->uni->exposeanaloggain_mask,gain);
	/* set exposure */
	/* Lower four bit should be 0*/
//    if (coarse_itg < 7) coarse_itg = 7;
//    if (coarse_itg > 16383) coarse_itg = 16383;
	exp_val = coarse_itg << 4;

	ret = uni_write_reg(client, UNICAM_8BIT,
		0x3502, exp_val & 0xFF);
	if (ret)
		goto exposure_err;

	ret = uni_write_reg(client, UNICAM_8BIT,
		0x3501, (exp_val >> 8) & 0xFF);
	if (ret)
		goto exposure_err;

	ret = uni_write_reg(client, UNICAM_8BIT,
		0x3500, (exp_val >> 16) & 0x0F);
	if (ret)
		goto exposure_err;

	//apply again shift
	if(gain>again_max){
			MYDBGNP("use max again %d\n",gain);
			gain=again_max;
	}

	if(digitgain>dgain_max){
			MYDBGNP("use max dgain %d \n",digitgain);
			digitgain=dgain_max;
	}

	/* set analog gain */
	ret = uni_write_reg(client, UNICAM_8BIT,
		0x350B, gain & 0xff);
	if (ret)
		goto exposure_err;

	ret = uni_write_reg(client, UNICAM_8BIT,
		0x350A, (gain >> 8) & 0xff);
	if (ret)
		goto exposure_err;

	ret = uni_write_reg_array(client, dev->uni->group_hold_end);
	if (ret)
		goto exposure_err;

	dbgonce=1;
//	mutex_unlock(&dev->input_lock);
	return ret;

exposure_err:
	dev_err(&client->dev, "unicam write again err.\n");
//	mutex_unlock(&dev->input_lock);
	return ret;

}



static long __hm5040_set_exposure(struct v4l2_subdev *sd, int coarse_itg,
				 int gain, int digitgain)
{

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct sensor_data *dev = to_sensor_data(sd);

	int ret = 0;
	static int dbgonce;
	int vts;
	int gain_tmp0, Reg_Cgain, Reg_Fgain;

	mutex_lock(&dev->input_lock);

	MYDBGEXP("HM5040 exp: coarse gain digigain %d:%08x %d:%08x %d:%08x\n",
			coarse_itg, coarse_itg, gain,
			gain, digitgain, digitgain);

	/* coarse_itg = 3665; */

	MYDBGEXP("HM5040 exp: coarse gain digigain %d:%08x %d:%08x %d:%08x\n",
			coarse_itg, coarse_itg, gain,
			gain, digitgain, digitgain);

	vts = dev->cur_res[dev->fmt_idx].vts;

	MYDBGEXP("HM5040 vts: vts = %d:%08x\n", vts);

	if(coarse_itg+dev->cur_res[dev->fmt_idx].vts_fix > vts){
		vts = coarse_itg+dev->cur_res[dev->fmt_idx].vts_fix;
	}

	MYDBGEXP("HM5040 vts: vts = %d:%08x  \n",vts);

	ret = uni_write_reg_array(client, dev->uni->group_hold_start);
	if (ret)
		goto exposure_err;

	/* set lines per frame */
	ret = unicam_write_i2c_with_mask(client,dev->uni->exposevts,dev->uni->exposevts_mask,vts,"set vts");
	if (ret)
		goto exposure_err;

	MYDBGNP(" coarse ################5########## entry %s dev->uni->exposeanaloggain =0x%x, dev->uni->exposeanaloggain_mask =0x%x ,gain =0x%x \n", __func__, dev->uni->exposeanaloggain,dev->uni->exposeanaloggain_mask,gain);
	/* set exposure */
	/* Lower four bit should be 0*/

	/* set exposure time 33ms fps= 30 -> coarse = 1454 */


    if (coarse_itg < 4) coarse_itg = 4;
    if (coarse_itg > 16383) coarse_itg = 16383;

	ret = uni_write_reg(client, UNICAM_8BIT,
		0x0202, (coarse_itg >> 8) & 0x00FF);
	if (ret)
		goto exposure_err;

	ret = uni_write_reg(client, UNICAM_8BIT,
		0x0203, coarse_itg & 0x00FF);
	if (ret)
		goto exposure_err;


	gain_tmp0 = gain >> 4;

	if (gain_tmp0 < 2) {
		Reg_Cgain = 0;
		Reg_Fgain = gain << 4;
	} else if (gain_tmp0 < 4) {
		Reg_Cgain = 0x80;
		Reg_Fgain = (gain >> 1) << 4;
	} else if (gain_tmp0 < 8) {
		Reg_Cgain = 0xc0;
		Reg_Fgain = (gain >> 2) << 4;
	} else if (gain_tmp0 < 16) {
		Reg_Cgain = 0xe0;
		Reg_Fgain = (gain >> 3) << 4;
	} else {
		Reg_Cgain = 0xf0;
		Reg_Fgain = (gain >> 4) << 4;
	}
	Reg_Cgain = Reg_Cgain & 0xFF;
	Reg_Fgain = Reg_Fgain;

	ret = uni_write_reg(client, UNICAM_8BIT, 0x0205, Reg_Cgain);
	if (ret)
		goto exposure_err;
	ret = uni_write_reg(client, UNICAM_8BIT, 0x0204, Reg_Cgain >> 8);
	if (ret)
		goto exposure_err;
	ret = uni_write_reg(client, UNICAM_8BIT, 0x020f, Reg_Fgain & 0xff);
	if (ret)
		goto exposure_err;
	ret = uni_write_reg(client, UNICAM_8BIT, 0x020e, Reg_Fgain >> 8);
	if (ret)
		goto exposure_err;
	ret = uni_write_reg(client, UNICAM_8BIT, 0x0211, Reg_Fgain & 0xff);
	if (ret)
		goto exposure_err;
	ret = uni_write_reg(client, UNICAM_8BIT, 0x0210, Reg_Fgain >> 8);
	if (ret)
		goto exposure_err;
	ret = uni_write_reg(client, UNICAM_8BIT, 0x0213, Reg_Fgain & 0xff);
	if (ret)
		goto exposure_err;
	ret = uni_write_reg(client, UNICAM_8BIT, 0x0212, Reg_Fgain >> 8);
	if (ret)
		goto exposure_err;
	ret = uni_write_reg(client, UNICAM_8BIT, 0x0215, Reg_Fgain & 0xff);
	if (ret)
		goto exposure_err;
	ret = uni_write_reg(client, UNICAM_8BIT, 0x0214, Reg_Fgain >> 8);
	if (ret)
		goto exposure_err;

	ret = uni_write_reg_array(client, dev->uni->group_hold_end);
	if (ret)
		goto exposure_err;

	dbgonce = 1;
	mutex_unlock(&dev->input_lock);
	return ret;

exposure_err:
	dev_err(&client->dev, "unicam write again err.\n");
	mutex_unlock(&dev->input_lock);
	return ret;
}

/*
	todo:
		group update
		FL modify
*/
static long __uni_set_exposure(struct v4l2_subdev *sd, int coarse_itg,
				 int gain, int digitgain)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct sensor_data *dev = to_sensor_data(sd);

	int ret=0;
	int exposegain_S;
	int exposedigitgain_S;
	static int dbgonce;
	int useadgain=0;
	int i;
	int adgain_a,adgain_d;
	int dgain_max,again_max;
	int vts;

	mutex_lock(&dev->input_lock);

	vts=dev->cur_res[dev->fmt_idx].vts;
	if(coarse_itg+dev->cur_res[dev->fmt_idx].vts_fix > vts){
		vts = coarse_itg+dev->cur_res[dev->fmt_idx].vts_fix;
	}

	MYDBGNP("exp: coarse gain digigain %d:%08x %d:%08x %d:%08x \n",coarse_itg,coarse_itg,gain,gain,digitgain,digitgain);

	ret = uni_write_reg_array(client, dev->uni->group_hold_start);
	if (ret) {
		mutex_unlock(&dev->input_lock);
		dev_err(&client->dev, "unicam write register err.\n");
		return ret;
	}

	exposegain_S=getshift(dev->uni->exposeanaloggain_mask);
	exposedigitgain_S=getshift(dev->uni->exposedigitgain_mask);

	again_max=(((u32)dev->uni->exposeanaloggain_mask)>>exposegain_S);
	dgain_max=(((u32)dev->uni->exposedigitgain_mask)>>exposedigitgain_S);

	ret = unicam_write_i2c_with_mask(client,dev->uni->exposevts,dev->uni->exposevts_mask,vts,"set vts");

	if (ret) {
		mutex_unlock(&dev->input_lock);
		dev_err(&client->dev, "unicam write vts err.\n");
		return ret;
	}

	ret = unicam_write_i2c_with_mask(client,dev->uni->exposecoarse,dev->uni->exposecoarse_mask,coarse_itg,"set exp");


	if (ret) {
		mutex_unlock(&dev->input_lock);
		dev_err(&client->dev, "unicam write coarse_itg err.\n");
		return ret;
	}

	if(dev->uni->flag & UNI_DEV_FLAG_USEADGAIN ) useadgain=1;

	/* computer adgain */
	/* a gain map */
	if(useadgain){
		int mapval,useispgain;
		int Lv,Rv,d_base;

		useispgain=gain;

		mapval=(useispgain*dev->uni->exposeadgain_param1gain)/dev->uni->exposeadgain_isp1gain;

		adgain_a=-1;
		for(i=0;i<dev->uni->exposeadgain_n/2;i++){
			Lv=dev->uni->exposeadgain_param[i*2];
			Rv=dev->uni->exposeadgain_param[i*2+1];
			if(mapval>=Rv) {
				adgain_a=Lv;
				d_base=Rv;
			}
		}
		if(adgain_a==-1){
			MYDBGNP("use min %d\n",mapval);
			adgain_a=dev->uni->exposeadgain_param[0];
			mapval=d_base=dev->uni->exposeadgain_param[1];
		}

		adgain_d=dev->uni->exposeadgain_min+
				(((mapval-d_base)*dev->uni->exposeadgain_fra)/d_base);

		if(adgain_d > dev->uni->exposeadgain_max){
				MYDBGNP("use max d old value:%d\n",adgain_d);
				adgain_d=dev->uni->exposeadgain_max;
		}

		MYDBGNP("exp: dump %d %d %d %d %d \n",useispgain,mapval,adgain_a,d_base,adgain_d);
		adgain_d=adgain_d*dev->uni->exposeadgain_step;
	}

	if(useadgain){
		MYDBGNP("use adgain\n");
		gain=adgain_a;
		digitgain=adgain_d;
	}

	//apply again shift

	if(gain>again_max){
			MYDBGNP("use max again %d\n",gain);
			gain=again_max;
	}

	if (digitgain > dgain_max) {
		MYDBGNP("use max dgain %d\n", digitgain);
		digitgain = dgain_max;
	}

	/* set analog gain */
	ret = unicam_write_i2c_with_mask(client,
			dev->uni->exposeanaloggain,
			dev->uni->exposeanaloggain_mask,
			gain,
			"set analog gain");

	if (ret) {
		dev_err(&client->dev, "unicam write again err.\n");
		mutex_unlock(&dev->input_lock);
		return ret;
	}

	/* set digital gain */
	ret = unicam_write_i2c_with_mask(client,dev->uni->exposedigitgainR,dev->uni->exposedigitgain_mask,digitgain,"set dgain R");
	ret += unicam_write_i2c_with_mask(client,dev->uni->exposedigitgainGR,dev->uni->exposedigitgain_mask,digitgain,"set dgain GR");
	ret += unicam_write_i2c_with_mask(client,dev->uni->exposedigitgainGB,dev->uni->exposedigitgain_mask,digitgain,"set dgain GB");
	ret += unicam_write_i2c_with_mask(client,dev->uni->exposedigitgainB,dev->uni->exposedigitgain_mask,digitgain,"set dgain B");

	if (ret) {
		dev_err(&client->dev, "unicam write digitgain err.\n");
		mutex_unlock(&dev->input_lock);
		return ret;
	}

	ret = uni_write_reg_array(client, dev->uni->group_hold_end);
	if (ret) {
		dev_err(&client->dev, "unicam write register err.\n");
		mutex_unlock(&dev->input_lock);
		return ret;
	}

	dbgonce=1;
	mutex_unlock(&dev->input_lock);

	return ret;
}
#if 0
static int uni_set_exposure(struct v4l2_subdev *sd, int exposure,
	int gain, int digitgain)
{
	struct sensor_data *dev = to_sensor_data(sd);
	int ret;

//	mutex_lock(&dev->input_lock);
	ret = __uni_set_exposure(sd, exposure, gain, digitgain);
//	mutex_unlock(&dev->input_lock);

	return ret;
}
#endif
static long uni_s_exposure(struct v4l2_subdev *sd,
			       struct atomisp_exposure *exposure)
{
	int exp = exposure->integration_time[0];
	int gain = exposure->gain[0];
	int digitgain = exposure->gain[1];
	struct sensor_data *dev = to_sensor_data(sd);

	MYDBG("%s switch:0x%x\n",__func__,dev->uni->idval);
	switch (dev->uni->idval) {
	case 0x5648:
		return __ov5648_set_exposure(sd, exp, gain, digitgain);
	case 0x2355:
		return __gc2355_set_exposure(sd, exp, gain, digitgain);
	case 0x03BB:
		return __hm5040_set_exposure(sd, exp, gain, digitgain);
	default:
		return __uni_set_exposure(sd, exp, gain, digitgain);
	}
}

static long uni_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	switch (cmd) {
	case ATOMISP_IOC_S_EXPOSURE:
		return uni_s_exposure(sd, arg);
	default:
		return -EINVAL;
	}
	return 0;
}

/*
	power on
*/

static int uni_init(struct v4l2_subdev *sd)
{
	struct sensor_data *dev = to_sensor_data(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	mutex_lock(&dev->input_lock);

	/* restore settings */
	dev->cur_res = dev->uni->ress;
	dev->n_res = dev->uni->ress_n;

	ret = uni_write_reg_array(client, dev->uni->global_setting);
	if (ret) {
		dev_err(&client->dev, "unicam write register err.\n");
		return ret;
	}
	
#ifdef OV5648_OTP
	if(dev->uni->idval == 0x5648)
	{
		uni_write_reg_array(client, dev->uni->stream_on );
		ov5648_update_otp(sd);	
		uni_write_reg_array(client, dev->uni->stream_off );
	}
#endif

	mutex_unlock(&dev->input_lock);

	return 0;
}

static int power_ctrl(struct v4l2_subdev *sd, int flag)
{
	int ret;
	struct sensor_data *dev = to_sensor_data(sd);

	if (!dev || !dev->platform_data)
		return -ENODEV;

	/* Non-gmin platforms use the legacy callback */
	if (dev->platform_data->power_ctrl)
		return dev->platform_data->power_ctrl(sd, flag);

       if (flag) {
               ret = dev->platform_data->v1p8_ctrl(sd, 1);
               usleep_range(60, 90);
               ret = dev->platform_data->v2p8_ctrl(sd, 1);
       } else {
               ret = dev->platform_data->v2p8_ctrl(sd, 0);
               ret |= dev->platform_data->v1p8_ctrl(sd, 0);
       }
       return ret;
}

static int gpio_ctrl(struct v4l2_subdev *sd, int flag)
{
	int ret;
	struct sensor_data *dev = to_sensor_data(sd);

	if (!dev || !dev->platform_data)
		return -ENODEV;

	/* Non-gmin platforms use the legacy callback */
	if (dev->platform_data->gpio_ctrl)
		return dev->platform_data->gpio_ctrl(sd, flag);

	MYDBGNP("################5########## entry %s hw_reset_gpio_polarity %d hw_reset_gpio_polarity %d \n", __func__,!dev->uni->hw_reset_gpio_polarity,dev->uni->hw_reset_gpio_polarity);
	/* GPIO0 == "reset" (active low), GPIO1 == "power down" */
	if (flag) {
		ret = dev->platform_data->gpio0_ctrl(sd, !dev->uni->hw_reset_gpio_polarity);
		msleep(2);
		ret = dev->platform_data->gpio0_ctrl(sd, dev->uni->hw_reset_gpio_polarity);
		msleep(dev->uni->hw_reset_delayms);
		ret = dev->platform_data->gpio0_ctrl(sd, !dev->uni->hw_reset_gpio_polarity);
		msleep(2);
		ret |= dev->platform_data->gpio1_ctrl(sd, !dev->uni->hw_pwd_gpio_polarity);
	} else {
		ret = dev->platform_data->gpio1_ctrl(sd, dev->uni->hw_pwd_gpio_polarity);
		ret |= dev->platform_data->gpio0_ctrl(sd, dev->uni->hw_pwd_gpio_polarity);
	}
	return ret;
}

static int __power_up(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct sensor_data *dev = to_sensor_data(sd);
	int ret;

	/* Enable power */
	ret = power_ctrl(sd, 1);
	if (ret)
		goto fail_power;

	usleep_range(5000, 6500);

	/* Enable clock */
	ret = dev->platform_data->flisclk_ctrl(sd, 1);
	if (ret)
		goto fail_clk;

	usleep_range(5000, 6500);

	/* gpio ctrl*/
	ret = gpio_ctrl(sd, 1);
	if (ret) {
		dev_err(&client->dev, "gpio failed\n");
		goto fail_gpio;
	}
#if 0
	/*MCLK to PWDN*/
	usleep_range(5000, 6500);

	/* gpio ctrl*/
	ret = gpio_ctrl(sd, 1);
	if (ret) {
		dev_err(&client->dev, "gpio failed\n");
		goto fail_gpio;
	}

	if (dev->vcm_driver && dev->vcm_driver->power_up)
		ret = dev->vcm_driver->power_up(sd);
	if (ret)
		dev_err(&client->dev, "%s,unicam vcm power-up failed.\n",__func__);
#endif

	msleep(20);

	return 0;

fail_gpio:
	gpio_ctrl(sd, 0);
fail_clk:
	dev->platform_data->flisclk_ctrl(sd, 0);
fail_power:
	power_ctrl(sd, 0);
	dev_err(&client->dev, "sensor power-up failed\n");

	return ret;
}

static int power_down(struct v4l2_subdev *sd)
{
	struct sensor_data *dev = to_sensor_data(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	/* gpio ctrl*/
	ret = gpio_ctrl(sd, 0);
	if (ret)
		dev_err(&client->dev, "gpio failed\n");

	//usleep_range(5000, 6500);

	ret = dev->platform_data->flisclk_ctrl(sd, 0);
	if (ret)
		dev_err(&client->dev, "flisclk failed\n");

	//usleep_range(5000, 6500);

	/* power control */
	ret = power_ctrl(sd, 0);
	if (ret)
		dev_err(&client->dev, "vprog failed.\n");

#if 0
	if (dev->vcm_driver && dev->vcm_driver->power_down)
		ret = dev->vcm_driver->power_down(sd);
	if (ret)
		dev_err(&client->dev, "%s,unicam vcm power-down failed.\n",__func__);

	usleep_range(5000, 6500);
#endif

	return ret;
}

static int power_up(struct v4l2_subdev *sd)
{
	static const int retry_count = 4;
	int i, ret;

	for (i = 0; i < retry_count; i++) {
		ret = __power_up(sd);
		if (!ret)
			return 0;

		power_down(sd);
	}
	return ret;
}


static int uni_s_power(struct v4l2_subdev *sd, int on)
{
	int ret;

	MYDBGNP("################5########## entry %s iic 0x%x\n",
			__func__, dev->uni->i2c_addr);

	MYDBG("%s,%s\n",__func__,on?"on":"off");
	if (on == 0)
		return power_down(sd);
	else {
		ret = power_up(sd);
		if (!ret)
			return uni_init(sd);
	}
	return ret;
}

static int get_resolution_index(struct v4l2_subdev *sd,int w, int h)
{
	struct sensor_data *dev = to_sensor_data(sd);
	int i;

	for (i = 0; i < dev->n_res; i++) {
		if (w != dev->cur_res[i].width)
			continue;
		if (h != dev->cur_res[i].height)
			continue;

		return i;
	}

	return -1;
}

static int distance(const S_UNI_RESOLUTION *res, u32 w, u32 h)
{
	unsigned int w_ratio = ((res->width << RATIO_SHIFT_BITS)/w);
	unsigned int h_ratio;
	int match;

	MYDBGFMT("%s w_ratio: %d res->width: %d w: %d\n",__func__,w_ratio,res->width,w);

	if (h == 0)
		return -1;
	h_ratio = ((res->height << RATIO_SHIFT_BITS) / h);
	MYDBGFMT("%s h_ratio: %d res->height: %d h: %d\n",__func__,h_ratio,res->height,h);

	if (h_ratio == 0)
		return -1;
	match   = abs(((w_ratio << RATIO_SHIFT_BITS) / h_ratio)
			- ((int)(1 << RATIO_SHIFT_BITS)));
	MYDBGFMT("%s match: %d \n",__func__,match);

	if ((w_ratio < (int)(1 << RATIO_SHIFT_BITS))
	    || (h_ratio < (int)(1 << RATIO_SHIFT_BITS)))
		return -1;

	return w_ratio + h_ratio;
}

static int nearest_resolution_index(struct v4l2_subdev *sd,
				    int w, int h)
{
	struct sensor_data *dev = to_sensor_data(sd);
	int i;
	int idx = -1;
	int dist;
	int min_dist = INT_MAX;
	const S_UNI_RESOLUTION *tmp_res = NULL;

	MYDBGFMT("%s, ###FMT###, n_res: %d, dev->mode: %d\n",__func__,dev->n_res, dev->mode);

	for (i = 0; i < dev->n_res; i++) {
		MYDBGFMT("%s i: %d  dist: %d line %d \n", __func__, i, dist, __LINE__);
		if( dev->cur_res[i].res_type & UNI_DEV_RES_DEFAULT) {
			idx = i;
			break;
		}
		if(dev->mode & dev->cur_res[i].res_type) {
			tmp_res = &dev->cur_res[i];
			dist = distance(tmp_res, w, h);
			MYDBGFMT("%s i: %d  dist: %d line %d \n", __func__, i, dist, __LINE__);
			if (dist == -1)
				continue;
			if (dist < min_dist) {
				min_dist = dist;
				idx = i;
			}
		}
	}

	return idx;
}

static int uni_try_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct sensor_data *dev = to_sensor_data(sd);
	int idx;
	int w, h;

	MYDBGFMT("%s in i2c_addr=0x%x %d %d 0x%x\n",__func__,dev->uni->i2c_addr,fmt->width,fmt->height,fmt->code);

	w = fmt->width;
	h = fmt->height;

	if (!fmt)
		return -EINVAL;
	idx = nearest_resolution_index(sd, fmt->width, fmt->height);

	if (idx == -1) {
		/* return the largest resolution */
		fmt->width = dev->cur_res[dev->uni->ress_n - 1].width;
		fmt->height = dev->cur_res[dev->uni->ress_n - 1].height;
	} else {
		fmt->width = dev->cur_res[idx].width;
		fmt->height = dev->cur_res[idx].height;
	}

	fmt->code = V4L2_MBUS_FMT_SBGGR10_1X10;

	printk("bingo...%s() src %dx%d, out %dx%d, 0x%x\n", __func__, w, h, fmt->width, fmt->height, fmt->code);

	return 0;
}



/*
	add common regs? no support, if no actural need, check bios encode the paramters
*/
static int startup(struct v4l2_subdev *sd)
{
	struct sensor_data *dev = to_sensor_data(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	MYDBG("use %d,%d,%s, %d\n",dev->n_res,dev->fmt_idx,dev->cur_res[dev->fmt_idx].desc,dev->cur_res[dev->fmt_idx].regs[0].type);

	ret = uni_write_reg_array(client, dev->cur_res[dev->fmt_idx].regs);
	if (ret) {
		dev_err(&client->dev, "unicam write register err.\n");
		return ret;
	}

	return ret;
}

static int uni_s_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct sensor_data *dev = to_sensor_data(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_mipi_info *uni_info = NULL;
	int ret = 0;

	MYDBGIN();
	uni_info = v4l2_get_subdev_hostdata(sd);
	if (uni_info == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);

	//must need to fix the fmt??
	ret = uni_try_mbus_fmt(sd, fmt);
	if (ret == -1) {
		dev_err(&client->dev, "unicam try fmt fail\n");
		goto done;
	}

	dev->fmt_idx = get_resolution_index(sd,fmt->width,
					      fmt->height);
	if (dev->fmt_idx == -1) {
		dev_err(&client->dev, "unicam get resolution fail\n");
		mutex_unlock(&dev->input_lock);
		return -EINVAL;
	}

	ret = startup(sd);
	if (ret) {
		dev_err(&client->dev, "unicam write fmt register err.\n");
		goto done;
	}

	MYDBGFMT("%s, idx: %d\n",__func__,dev->fmt_idx);

	ret = uni_get_intg_factor(client, uni_info,
					&dev->cur_res[dev->fmt_idx]);
	if (ret)
		dev_err(&client->dev, "unicam failed to get integration_factor\n");

done:
	mutex_unlock(&dev->input_lock);
	return ret;

}

static int uni_g_mbus_fmt(struct v4l2_subdev *sd,
			     struct v4l2_mbus_framefmt *fmt)
{
	struct sensor_data *dev = to_sensor_data(sd);

	mutex_lock(&dev->input_lock);
	fmt->width = dev->cur_res[dev->fmt_idx].width;
	fmt->height = dev->cur_res[dev->fmt_idx].height;
	fmt->code = (dev->uni->hw_format==ATOMISP_INPUT_FORMAT_RAW_10)?V4L2_MBUS_FMT_SBGGR10_1X10:V4L2_MBUS_FMT_SGRBG8_1X8;
	mutex_unlock(&dev->input_lock);

	return 0;
}

static int uni_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct sensor_data *dev = to_sensor_data(sd);

	mutex_lock(&dev->input_lock);
	interval->interval.numerator = 1;
	interval->interval.denominator = dev->cur_res[dev->fmt_idx].fps;
	mutex_unlock(&dev->input_lock);

	return 0;
}

static int uni_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_data *dev = to_sensor_data(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	MYDBG("%s %s\n",__func__,enable?"on":"off");
	mutex_lock(&dev->input_lock);

	ret = uni_write_reg_array(client, enable ? dev->uni->stream_on: dev->uni->stream_off);

	mutex_unlock(&dev->input_lock);
	return ret;
}


/*unicam enum frame size, frame intervals */
static int uni_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	struct sensor_data *dev = to_sensor_data(sd);
	unsigned int index = fsize->index;

	mutex_lock(&dev->input_lock);

	if (index >= dev->n_res) {
		mutex_unlock(&dev->input_lock);
		return -EINVAL;
	}

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = dev->cur_res[index].width;
	fsize->discrete.height = dev->cur_res[index].height;
	fsize->reserved[0] = dev->cur_res[index].used;

	mutex_unlock(&dev->input_lock);

	return 0;
}


static int uni_enum_frameintervals(struct v4l2_subdev *sd,
				      struct v4l2_frmivalenum *fival)
{
	unsigned int index = fival->index;
	struct sensor_data *dev = to_sensor_data(sd);
	MYDBG("%s %d/%d\n",__func__,index,dev->n_res);

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->width = dev->cur_res[index].width;
	fival->height = dev->cur_res[index].height;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = dev->cur_res[index].fps;

	return 0;
}

static int uni_enum_mbus_fmt(struct v4l2_subdev *sd,
				unsigned int index,
				enum v4l2_mbus_pixelcode *code)
{
	struct sensor_data *dev = to_sensor_data(sd);

	MYDBG("%s %d\n",__func__,index);

	*code = (dev->uni->hw_format==ATOMISP_INPUT_FORMAT_RAW_10)?V4L2_MBUS_FMT_SBGGR10_1X10:V4L2_MBUS_FMT_SGRBG8_1X8;

	return 0;
}

/*
		//i2c access need the subdevinit
*/

#define I2C_M_WR 0

int gc_read_i2c_reg(
	struct i2c_client *client,
	u16 data_length,
	u8 reg,
	u32 *val)
{
	int ret = 0;
	struct i2c_msg msg[1];
	unsigned char data[2] = { 0, 0 };

	if (!client->adapter) {
		MYDBG("client->adapter NULL\n");
		return -ENODEV;
	}

	msg->addr = client->addr;
	msg->flags = I2C_M_WR;
	msg->len = 1;
	msg->buf = data;

	/* High byte goes out first */
	data[0] = (u8) (reg & 0xff);

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret >= 0) {
		mdelay(3);
		msg->flags = I2C_M_RD;
		msg->len = data_length;
		i2c_transfer(client->adapter, msg, 1);
	}
	if (ret >= 0) {
		*val = 0;
		/* High byte comes first */
		if (data_length == 1)
			*val = data[0];
		else if (data_length == 2)
			*val = data[1] + (data[0] << 8);
		else
		;

		return 0;
	}
	MYDBG("i2c read from offset 0x%08x failed with error %d\n", reg, ret);
	return ret;
}


static int det_id(struct i2c_client *client)
{
	u16 val=0;
	u16 vp;
	int ret = 0;
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sensor_data *dev = to_sensor_data(sd);
	int len=getbyte(dev->uni->idmask);

	MYDBG("i2c addr:  0x%x 0x%x\n",dev->uni->i2c_addr,client->addr);

	if (dev->uni->i2c_addr != client->addr) {
		if (dev->uni->desc) {
			MYDBG("client->addr != dev->uni->i2c_addr!\n");
			return -ENODEV;
		}

		client->addr = dev->uni->i2c_addr;
	}

	if (len == 0)
		return 0;

	if (dev->uni->i2c_addr == 0x3C) {
		ret  = gc_read_i2c_reg(client, 1,
				dev->uni->idreg, &vp);
		val |= (vp & 0xff) << 8;
		ret  = gc_read_i2c_reg(client, 1,
				dev->uni->idreg + 1, &vp);
		val |= vp & 0xff;
		MYDBG("det_id 0x%x need 0x%x\n",
				val, dev->uni->idval);
		return ret;
	}

	/* read_reg have bug, overwrite entire int32 */
	if (dev->uni->flag & UNI_DEV_FLAG_1B_ACCESS) {
		if (len == 2) {
			MYDBG("2 1B access\n");
			ret = uni_read_reg(client, UNICAM_8BIT,
					dev->uni->idreg, &vp);
			val |= (vp & 0xff) << 8;
			ret = uni_read_reg(client, UNICAM_8BIT,
					dev->uni->idreg + 1, &vp);
			val |= vp & 0xff;
		} else if (len == 1) {
			ret = uni_read_reg(client, UNICAM_8BIT,
					dev->uni->idreg, &val);
		}
	} else {
		ret = uni_read_reg(client,
				   len, dev->uni->idreg, &val);
	}
	if (ret) {

		MYDBG("det_id fail i2c access\n");
		return -1;
	}

	MYDBG("det_id 0x%x need 0x%x\n",val,dev->uni->idval);

	if(val != dev->uni->idval)
		return -ENODEV;

	return ret;
}

static struct unicam_vcm * find_vcm_mapping(int id){
	int find = 0;
	const struct unicam_vcm_mapping *mapping = supported_vcms;
	while (mapping != NULL) {
		if (mapping->id == id){
			/* compare name */
			find = 1;
			break;
		}
		mapping++;
	}
	if(find == 0) return NULL;

	return mapping->vcm;
}

static int uni_s_config(struct v4l2_subdev *sd,
			   int irq, void *pdata)
{
	struct sensor_data *dev = to_sensor_data(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	unsigned char old_addr = 0;

	MYDBGNP("################5########## entry %s \
			line = %d\n", __func__, __LINE__);

	if (pdata == NULL) {
		dev_err(&client->dev, "unicam platform_data err.\n");
		return -ENODEV;
	}

	dev->platform_data =
		(struct camera_sensor_platform_data *)pdata;

//	if (dev->platform_data->platform_init) {
//		ret = dev->platform_data->platform_init(client);
//		if (ret)
//			return -ENODEV;
//	}

	mutex_lock(&dev->input_lock);


	/* power off the module, then power on it in future
	 * as first power on by board may not fulfill the
	 * power on sequqence needed by the module
	 */
	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "unicam power-off err.\n");
		goto fail_csi_cfg;
	}

	msleep(5);

	ret = power_up(sd);
	if (ret) {
		dev_err(&client->dev, "unicam power-up err.\n");
		goto fail_power_on;
	}

	msleep(5);

	ret = dev->platform_data->csi_cfg(sd, 1);
	if (ret) {
		dev_err(&client->dev, "unicam csi_cfg err.\n");
		goto fail_csi_cfg;
	}

	old_addr = client->addr;
	ret = det_id(client);
	if (ret) {
		dev_err(&client->dev, "unicam_detect err s_config.\n");
		client->addr = old_addr;
		goto fail_csi_cfg;
	}

	ret = power_down(sd);
	if (ret) {
		dev_err(&client->dev, "unicam power-off err.\n");
		goto fail_csi_cfg;
	}

	mutex_unlock(&dev->input_lock);

	return 0;

fail_csi_cfg:
	dev->platform_data->csi_cfg(sd, 0);
fail_power_on:
	power_down(sd);
	mutex_unlock(&dev->input_lock);

	if (dev->platform_data->platform_deinit)
		dev->platform_data->platform_deinit();

	return ret;
}

static int uni_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct sensor_data *dev = to_sensor_data(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	MYDBGNP("################5########## entry %s  line = %d \n", __func__, __LINE__);
	if (!param)
		return -EINVAL;

	if (param->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		dev_err(&client->dev,  "unsupported buffer type.\n");
		return -EINVAL;
	}

	memset(param, 0, sizeof(*param));
	param->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	mutex_lock(&dev->input_lock);
	if (dev->fmt_idx >= 0 && dev->fmt_idx < dev->n_res) {
		param->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
		param->parm.capture.timeperframe.numerator = 1;
		param->parm.capture.capturemode = dev->parm_mode;
		param->parm.capture.timeperframe.denominator =
					dev->cur_res[dev->fmt_idx].fps;
	}
	mutex_unlock(&dev->input_lock);

	return 0;
}

static int uni_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct sensor_data *dev = to_sensor_data(sd);
	dev->parm_mode = param->parm.capture.capturemode;

	MYDBGNP("\n##### entry %s:run_mode :%x\n", __func__, dev->parm_mode);

	mutex_lock(&dev->input_lock);
	switch (dev->parm_mode) {
	case CI_MODE_VIDEO:

		dev->cur_res = dev->uni->ress;
		dev->n_res = dev->uni->ress_n;
		dev->mode = UNI_DEV_RES_VIDEO;
		break;
	case CI_MODE_STILL_CAPTURE:

		dev->cur_res = dev->uni->ress;
		dev->n_res = dev->uni->ress_n;
		dev->mode = UNI_DEV_RES_STILL;
		break;
	default:

		dev->cur_res = dev->uni->ress;
		dev->n_res = dev->uni->ress_n;
		dev->mode = UNI_DEV_RES_PREVIEW;
		break;
	}
	mutex_unlock(&dev->input_lock);
	return 0;
}

/*
	tbd by check pixformat to dete x8 support
*/
static int uni_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_mbus_code_enum *code)
{
	struct sensor_data *dev = to_sensor_data(sd);

	MYDBG("%s %d, 0x%x\n",__func__,code->index,(int)fh);

	code->code = (dev->uni->hw_format==ATOMISP_INPUT_FORMAT_RAW_10)?V4L2_MBUS_FMT_SBGGR10_1X10:V4L2_MBUS_FMT_SGRBG8_1X8;

	return 0;
}

static int uni_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_frame_size_enum *fse)
{
	struct sensor_data *dev = to_sensor_data(sd);
	int index = fse->index;

	mutex_lock(&dev->input_lock);

	MYDBGNP("################5########## entry %s  line = %d \n", __func__, __LINE__);

	if (index >= dev->n_res) {
		mutex_unlock(&dev->input_lock);
		return -EINVAL;
	}

	fse->min_width = dev->cur_res[index].width;
	fse->min_height = dev->cur_res[index].height;
	fse->max_width = dev->cur_res[index].width;
	fse->max_height = dev->cur_res[index].height;

	mutex_unlock(&dev->input_lock);
	return 0;
}

static int uni_get_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct sensor_data *dev = to_sensor_data(sd);
	struct v4l2_mbus_framefmt *format;

	MYDBGNP("################5########## entry %s  line = %d \n", __func__, __LINE__);

	switch (fmt->which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		format = v4l2_subdev_get_try_format(fh, fmt->pad);
		break;
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		format = &dev->format;
		break;
	default:
		format = NULL;
	}

	if (!format)
		return -EINVAL;

	fmt->format = *format;

	return 0;
}

static int uni_set_pad_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_format *fmt)
{
	struct sensor_data *dev = to_sensor_data(sd);

	mutex_lock(&dev->input_lock);

	MYDBGNP("################5########## entry %s  line = %d \n", __func__, __LINE__);

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		dev->format = fmt->format;

	mutex_unlock(&dev->input_lock);
	return 0;
}

static int uni_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
	struct sensor_data *dev = to_sensor_data(sd);

	mutex_lock(&dev->input_lock);
	*frames = dev->cur_res[dev->fmt_idx].skip_frames;
	mutex_unlock(&dev->input_lock);

	return 0;
}

static int unicam_q_exposure(struct v4l2_subdev *sd, s32 *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct sensor_data *dev = to_sensor_data(sd);
	int val=0;
	int ret = 0;

	MYDBGNP("################5########## entry %s  line = %d \n", __func__, __LINE__);


	MYDBG("%s switch:0x%x\n",__func__,dev->uni->idval);
	switch (dev->uni->idval) {
	case 0x5648:
		ret = unicam_read_i2c_with_mask(client, dev->uni->exposecoarse, dev->uni->exposecoarse_mask, &val, "q_exp");
		if (ret) {
			dev_err(&client->dev, "unicam unicam_q_exposure err.\n");
			return ret;
		}
		*value = val>>4;
		break;
	case 0x2355:
		ret = uni_write_reg(client, UNICAM_8BIT, 0xFE, 0x00);
		if (ret) {
			dev_err(&client->dev, "unicam unicam_q_exposure err.\n");
			return ret;
		}
		ret = unicam_read_i2c_with_mask(client, dev->uni->exposecoarse, dev->uni->exposecoarse_mask, &val, "q_exp");
		if (ret) {
			dev_err(&client->dev, "unicam unicam_q_exposure err.\n");
			return ret;
		}
		*value = val&0x1fff;
		break;
	default:
		ret = unicam_read_i2c_with_mask(client, dev->uni->exposecoarse, dev->uni->exposecoarse_mask, &val, "q_exp");
		if (ret) {
			dev_err(&client->dev, "unicam unicam_q_exposure err.\n");
			return ret;
		}
		*value = val;
		break;
	}

	return ret;
}


int unicam_t_focus_abs(struct v4l2_subdev *sd, s32 value)
{
	struct sensor_data *dev = to_sensor_data(sd);
	int ret = 0;

	if (dev->vcm_driver && dev->vcm_driver->t_focus_abs)
		ret = dev->vcm_driver->t_focus_abs(sd, value);

	return ret;
}

int unicam_t_focus_rel(struct v4l2_subdev *sd, s32 value)
{
	struct sensor_data *dev = to_sensor_data(sd);
	int ret = 0;

	if (dev->vcm_driver && dev->vcm_driver->t_focus_rel)
		ret = dev->vcm_driver->t_focus_rel(sd, value);

	return ret;
}

int unicam_q_focus_status(struct v4l2_subdev *sd, s32 *value)
{
	struct sensor_data *dev = to_sensor_data(sd);
	int ret = 0;

	if (dev->vcm_driver && dev->vcm_driver->q_focus_status)
		ret = dev->vcm_driver->q_focus_status(sd, value);

	return ret;
}

int unicam_q_focus_abs(struct v4l2_subdev *sd, s32 *value)
{
	struct sensor_data *dev = to_sensor_data(sd);
	int ret = 0;

	if (dev->vcm_driver && dev->vcm_driver->q_focus_abs)
		ret = dev->vcm_driver->q_focus_abs(sd, value);

	return ret;
}

static int unicam_g_focal(struct v4l2_subdev *sd, s32 *val)
{
	*val = (UNICAM_FOCAL_LENGTH_NUM << 16) | UNICAM_FOCAL_LENGTH_DEM;
	return 0;
}

static int unicam_g_fnumber(struct v4l2_subdev *sd, s32 *val)
{
	/*const f number for unicam*/
	*val = (UNICAM_F_NUMBER_DEFAULT_NUM << 16) | UNICAM_F_NUMBER_DEM;
	return 0;
}

static int unicam_g_fnumber_range(struct v4l2_subdev *sd, s32 *val)
{
	*val = (UNICAM_F_NUMBER_DEFAULT_NUM << 24) |
		(UNICAM_F_NUMBER_DEM << 16) |
		(UNICAM_F_NUMBER_DEFAULT_NUM << 8) | UNICAM_F_NUMBER_DEM;

	return 0;
}

static int unicam_g_bin_factor_x(struct v4l2_subdev *sd, s32 *val)
{
	struct sensor_data *dev = to_sensor_data(sd);

	*val = dev->cur_res[dev->fmt_idx].bin_factor_x;

	return 0;
}

static int unicam_g_bin_factor_y(struct v4l2_subdev *sd, s32 *val)
{
	struct sensor_data *dev = to_sensor_data(sd);

	*val = dev->cur_res[dev->fmt_idx].bin_factor_y;

	return 0;
}

struct unicam_control unicam_controls[] = {
	{
		.qc = {
			.id = V4L2_CID_EXPOSURE_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "exposure",
			.minimum = 0x0,
			.maximum = 0xffff,
			.step = 0x01,
			.default_value = 0x00,
			.flags = 0,
		},
		.query = unicam_q_exposure,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCAL_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focal length",
			.minimum = UNICAM_FOCAL_LENGTH_DEFAULT,
			.maximum = UNICAM_FOCAL_LENGTH_DEFAULT,
			.step = 0x01,
			.default_value = UNICAM_FOCAL_LENGTH_DEFAULT,
			.flags = 0,
		},
		.query = unicam_g_focal,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number",
			.minimum = UNICAM_F_NUMBER_DEFAULT,
			.maximum = UNICAM_F_NUMBER_DEFAULT,
			.step = 0x01,
			.default_value = UNICAM_F_NUMBER_DEFAULT,
			.flags = 0,
		},
		.query = unicam_g_fnumber,
	},
	{
		.qc = {
			.id = V4L2_CID_FNUMBER_RANGE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "f-number range",
			.minimum = UNICAM_F_NUMBER_RANGE,
			.maximum =  UNICAM_F_NUMBER_RANGE,
			.step = 0x01,
			.default_value = UNICAM_F_NUMBER_RANGE,
			.flags = 0,
		},
		.query = unicam_g_fnumber_range,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCUS_ABSOLUTE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focus move absolute",
			.minimum = 0,
			.maximum = UNICAM_MAX_FOCUS_POS,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = unicam_t_focus_abs,
		.query = unicam_q_focus_abs,

	},
	{
		.qc = {
			.id = V4L2_CID_FOCUS_RELATIVE,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focus move relative",
			.minimum = UNICAM_MAX_FOCUS_NEG,
			.maximum = UNICAM_MAX_FOCUS_POS,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.tweak = unicam_t_focus_rel,
	},
	{
		.qc = {
			.id = V4L2_CID_FOCUS_STATUS,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "focus status",
			.minimum = 0,
			.maximum = 100, /* allow enum to grow in the future */
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = unicam_q_focus_status,
	},
	{
		.qc = {
			.id = V4L2_CID_BIN_FACTOR_HORZ,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "horizontal binning factor",
			.minimum = 0,
			.maximum = UNICAM_BIN_FACTOR_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = unicam_g_bin_factor_x,
	},
	{
		.qc = {
			.id = V4L2_CID_BIN_FACTOR_VERT,
			.type = V4L2_CTRL_TYPE_INTEGER,
			.name = "vertical binning factor",
			.minimum = 0,
			.maximum = UNICAM_BIN_FACTOR_MAX,
			.step = 1,
			.default_value = 0,
			.flags = 0,
		},
		.query = unicam_g_bin_factor_y,
	},
};

#define N_CONTROLS (ARRAY_SIZE(unicam_controls))

static struct unicam_control *unicam_find_control(u32 id)
{
	int i;

	for (i = 0; i < N_CONTROLS; i++)
		if (unicam_controls[i].qc.id == id)
			return &unicam_controls[i];
	return NULL;
}

static int uni_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	struct unicam_control *ctrl = unicam_find_control(qc->id);
	struct sensor_data *dev = to_sensor_data(sd);

	MYDBGNP("################5########## entry %s  line = %d \n", __func__, __LINE__);

	if (ctrl == NULL)
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	*qc = ctrl->qc;
	mutex_unlock(&dev->input_lock);

	return 0;
}

/* imx control set/get */
static int uni_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct unicam_control *s_ctrl;
	struct sensor_data *dev = to_sensor_data(sd);
	int ret;

	MYDBGNP("################5########## entry %s  line = %d \n", __func__, __LINE__);

	if (!ctrl)
		return -EINVAL;

	s_ctrl = unicam_find_control(ctrl->id);
	if ((s_ctrl == NULL) || (s_ctrl->query == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = s_ctrl->query(sd, &ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static int uni_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct unicam_control *octrl = unicam_find_control(ctrl->id);
	struct sensor_data *dev = to_sensor_data(sd);
	int ret;

	MYDBGNP("################5########## entry %s  line = %d \n", __func__, __LINE__);

	if ((octrl == NULL) || (octrl->tweak == NULL))
		return -EINVAL;

	mutex_lock(&dev->input_lock);
	ret = octrl->tweak(sd, ctrl->value);
	mutex_unlock(&dev->input_lock);

	return ret;
}

static const struct v4l2_subdev_sensor_ops uni_sensor_ops = {
	.g_skip_frames	= uni_g_skip_frames,
};

static const struct v4l2_subdev_video_ops uni_video_ops = {
	.s_stream = uni_s_stream,
	.s_parm = uni_s_parm,
	.g_parm = uni_g_parm,
	.try_mbus_fmt = uni_try_mbus_fmt,
	.enum_framesizes = uni_enum_framesizes,
	.enum_frameintervals = uni_enum_frameintervals,
	.enum_mbus_fmt = uni_enum_mbus_fmt,
	.s_mbus_fmt = uni_s_mbus_fmt,
	.g_mbus_fmt = uni_g_mbus_fmt,
	.g_frame_interval = uni_g_frame_interval,
};

static const struct v4l2_subdev_core_ops uni_core_ops = {
	.s_power = uni_s_power,
	.ioctl = uni_ioctl,
	.queryctrl = uni_queryctrl,
	.g_ctrl = uni_g_ctrl,
	.s_ctrl = uni_s_ctrl,
};

static const struct v4l2_subdev_pad_ops uni_pad_ops = {
	.enum_mbus_code = uni_enum_mbus_code,
	.enum_frame_size = uni_enum_frame_size,
	.get_fmt = uni_get_pad_format,
	.set_fmt = uni_set_pad_format,
};

static const struct v4l2_subdev_ops uni_ops = {
	.core = &uni_core_ops,
	.video = &uni_video_ops,
	.pad = &uni_pad_ops,
	.sensor = &uni_sensor_ops,
};

#if 0
static S_UNI_DEVICE *  getuni(const char*name)
{
	int i;
	for(i=0;m_data[i].uni!=NULL;i++){
		if(!strncmp(name,m_data[i].name,MATCH_NAME_LEN)) return m_data[i].uni;
	}

	return NULL;
}

static int linkdata(u8*data,int len)
{
    S_UNI_DEVICE * dev=(void*)data;
    int offlen=*(int*)(data+len-8);
    int *goffset=(int *)(data+len-offlen*8);
    int i;
    int *p;

    for(i=0;i<offlen-1;i++){
        MYDBG("%d %d %d \n",i,goffset[i*2],goffset[i*2+1]);
        p=(int*)(data+goffset[i*2]);
        *p=(int)(data+goffset[i*2+1]);
    }

    MYDBG(" %d %08x %08x %x\n",dev->ress[0].regs_n,dev->ress,dev,dev->ress[0].regs);
    MYDBG(" %s %s %s\n",dev->ress[0].desc,dev->ress[1].desc,dev->ress[2].desc);
    MYDBG(" %d %d %d\n",dev->ress[0].regs[0].type,dev->ress[1].regs[0].type,dev->ress[2].regs[0].type);
    return 0;
}
#endif

#define UNI_FW_PATH "camera.data"

static int uni_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sensor_data *dev = to_sensor_data(sd);
	dev_dbg(&client->dev, "unicam_remove...\n");
	MYDBGNP("unicam_remove...\n");

	if (dev->platform_data->platform_deinit)
		dev->platform_data->platform_deinit();

	dev->platform_data->csi_cfg(sd, 0);

	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&dev->sd.entity);
	kfree(dev);

	return 0;
}

static int uni_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct sensor_data *dev;
	int i;
	int ret = 0;
//	const struct firmware *fw;
	struct match_data *uni_data = NULL;
//	S_UNI_DEVICE *fwdev=NULL;
	void *pdata = client->dev.platform_data;
	static int unicam_data = 0;

	MYDBG("uni_probe name: %s, unicam_data = %d\n",
			client->name, unicam_data);

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	if (client->name &&
			!strncmp(client->name, "front", 5))
		uni_data = m_data_front;
	else
		uni_data = m_data;

	/* ret = request_firmware(&fw, UNI_FW_PATH, &client->dev); */
	if (1) {
		MYDBG("not find firmware PPR\n");
		for (i = 0; uni_data[i].uni != NULL; i++) {
			dev->uni = uni_data[i].uni;
			MYDBG("uni_probe name: uni_data[%d].name %s\n",
					i,
					uni_data[i].name);
			mutex_init(&dev->input_lock);
			dev->fmt_idx = 0;
			v4l2_i2c_subdev_init(&(dev->sd), client, &uni_ops);

			if (ACPI_COMPANION(&client->dev))
				pdata = gmin_camera_platform_data(&dev->sd,
								  ATOMISP_INPUT_FORMAT_RAW_10,
								  dev->uni->hw_bayer_order);
			if (!pdata)
				goto out_free;

		//i2c access need the subdevinit

		ret = uni_s_config(&dev->sd, client->irq,
			       pdata);
		if (ret) {
			if(uni_data[i+1].uni==NULL) {
				goto out_free;}

			v4l2_device_unregister_subdev(&dev->sd);
		}else{
//			ret = unicam_atomisp_register_i2c_module(&dev->sd, pdata, RAW_CAMERA);
			ret = atomisp_register_i2c_module(&dev->sd, pdata, RAW_CAMERA);

			if (ret)
				goto out_free;
				snprintf(dev->sd.name, sizeof(dev->sd.name),
						"%s %d-%04x",
						uni_data[i].name,
						i2c_adapter_id(client->adapter),
						client->addr);
				break;
		}
	}
#if 0
		dev->uni=getuni(id->name);
		if(dev->uni==NULL){
			MYDBG("not find uni dev, break probe\n");
			return -1;
		}
#endif
		if(unicam_data == 0){
			unicam_data = 1;
		}else{
			unicam_data = 0;
		}
		MYDBG("use build in table\n");
	}else{
		//size, data
		return -1;
	}

	dev->fmt_idx = 0;
	dev->cur_res = dev->uni->ress;
	dev->n_res =  dev->uni->ress_n;
	dev->mode = UNI_DEV_RES_PREVIEW;

	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dev->pad.flags = MEDIA_PAD_FL_SOURCE;
	dev->format.code = (dev->uni->hw_format==ATOMISP_INPUT_FORMAT_RAW_10)?V4L2_MBUS_FMT_SBGGR10_1X10:V4L2_MBUS_FMT_SGRBG8_1X8;
	dev->sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	dev->vcm_driver = find_vcm_mapping(dev->uni->af_type);

	ret = media_entity_init(&dev->sd.entity, 1, &dev->pad, 0);
	if (ret) {
		dev_err(&client->dev, "media_entity_init error.\n");
		uni_remove(client);
	}
	MYDBG("%s,af_type,%d\n",__func__,dev->uni->af_type);
	/* vcm initialization */
	if (dev->vcm_driver && dev->vcm_driver->init)
		ret = dev->vcm_driver->init(&dev->sd);
	if (ret) {
		//uni_remove(client);
		dev_err(&client->dev, "vcm init failed.\n");
	}

	return ret;

out_free:
	atomisp_gmin_remove_subdev(&dev->sd);
	v4l2_device_unregister_subdev(&dev->sd);
	kfree(dev);

	return ret;
}


//ppr no module
MODULE_DEVICE_TABLE(i2c, uni_id_ppr_cam);

static struct acpi_device_id unicam_acpi_match[] = {
#if 0
	{"INT33F3"},
	{"HIMX8131"},
	{"OVTI8865"},
	{"INT33F0"},
	{"HIMX2051"},
	{"XXGC0310"},
#endif
	{"INT3477"},
	{"INT5648"},
	{"INT5040"},

	{"GCTI2355"},
	{},
};
MODULE_DEVICE_TABLE(acpi, unicam_acpi_match);

static struct i2c_driver uni_driver_ppr_cam = {
	.driver = {
		.owner = THIS_MODULE,
		.name = SENSOR_PPR_CAM,
		.acpi_match_table = ACPI_PTR(unicam_acpi_match),
	},
	.probe = uni_probe,
	.remove = uni_remove,
	.id_table = uni_id_ppr_cam,
};

//module_i2c_driver(uni_driver_ppr_cam);

static __init int init_unicam(void)
{
	MYDBG("%s\n",__func__);
	return i2c_add_driver(&uni_driver_ppr_cam);
}

static __exit void exit_unicam(void)
{
	i2c_del_driver(&uni_driver_ppr_cam);
}

module_init(init_unicam);
module_exit(exit_unicam);

MODULE_AUTHOR("Jun CHEN <junx.chen@intel.com>");
MODULE_DESCRIPTION("A low-level driver for unicam sensors");
MODULE_LICENSE("GPL");
