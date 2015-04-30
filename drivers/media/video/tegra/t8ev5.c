/*
 * kernel/drivers/media/video/tegra
 *
 * Aptina MT9D115 sensor driver
 *
 * Copyright (C) 2010 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "t8ev5.h"
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/syscalls.h>
#define SENSOR_WIDTH_REG 0x2703
#define SENSOR_640_WIDTH_VAL 0x0518
#define CAM_CORE_A_OUTPUT_SIZE_WIDTH 0xC86C
#define ONE_TIME_TABLE_CHECK_REG 0x8016
#define ONE_TIME_TABLE_CHECK_DATA 0x086C


#define SETTLETIME_MS 5
#define POS_LOW (0)
#define POS_HIGH (1023)
#define FOCAL_LENGTH (10.0f)
#define FNUMBER (2.8f)
#define FPOS_COUNT 1024
#define T8EV5_FOCUS_MACRO		810
#define T8EV5_FOCUS_INFINITY	50 /* Exact value needs to be decided */

#define MAX_POS 8
#define ANDROID_WINDOW_PRECISION (1000)

#define PREVIEW_SIZE_W ANDROID_WINDOW_PRECISION*2
#define PREVIEW_SIZE_H ANDROID_WINDOW_PRECISION*2

#define PER_SIZE_W 250//PREVIEW_SIZE_W >> 3)
#define PER_SIZE_H 250//PREVIEW_SIZE_H >> 3)

#define MT9P111_PER_SIZE_W (500)
#define MT9P111_PER_SIZE_H (500)

#define FLASH_AGAIN_SIZE 752// if gain >752 needed flash
#define MT9P111_CAMERA_RST_PIN 223


#define DRIVER_VERSION "1.0.0"

#define MAX_LENS 103
#define NO_LENS_SHADING 1
char mDebugFileName[] = "/data/camera/effect.txt";
#define _debug_is 0
#define _debug_awb_is 0
#if _debug_is
#define debug_print(...) pr_err(__VA_ARGS__)
#else
#define debug_print(...) 
#endif

#if _debug_awb_is
#define debug_effect_print(...) pr_err(__VA_ARGS__)
#else
#define debug_effect_print(...) 
#endif

#define UPDATE_AWB 1
#define UPDATE_EXP 2
#define UPDATE_ALL 3

#define MAX_SENSOR_COUNT 2

struct sensor_info {
	int isRecoded;
	int mode;
	int current_wb;
	int current_exposure;
	int continuous_enable;
	int flash_mode;
	int led_en;
	int flash_en;
	int update_index;
	SensorType sensor_type;
	u8 cur_flag;
	bool flash_enable;
	bool force_flash_on;
	bool have_task;
	bool isFirstOpen;
	unsigned int timer_debounce;
	u16 current_addr;
	atomic_t power_on_loaded;
	atomic_t sensor_init_loaded;
	struct timer_list timer;
	struct work_struct work;
	//struct work_struct     work;
	struct mutex als_get_sensor_value_mutex;
	struct t8ev5_win_pos win_pos;
	struct t8ev5_win_pos win_pos_old;
	struct t8ev5_config config;
	struct i2c_client *i2c_client;
	struct yuv_t8ev5_sensor_platform_data *pdata;
};


static struct sensor_info *mt9p111_sensor_info;

static int t8ev5_read_8reg(struct i2c_client *client, u16 addr, u8 *val)
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

static int sensor_read_reg(struct i2c_client *client, u16 addr, u16 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[4];

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
	msg[1].len = 2;
	msg[1].buf = data + 2;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err != 2)
		return -EINVAL;

        swap(*(data+2),*(data+3)); //swap high and low byte to match table format
	memcpy(val, data+2, 2);

	return 0;
}

static int sensor_read_8reg(struct i2c_client *client, u16 addr, u8 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[2];

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
	msg[1].buf = data;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err != 2)
		return -EINVAL;
	
	memcpy(val, data, 1);
	return 0;
}

static int sensor_write_reg8(struct i2c_client *client, u16 addr, u8 val)
{
	int err;
	struct i2c_msg msg;
	unsigned char data[3];
	int retry = 0;

	if (!client->adapter)
		return -ENODEV;

	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);
	
	data[2] = (u8) (val & 0xff);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = data;

	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry++;
		pr_err("mt9p111 : i2c transfer failed, retrying %04x %02x %d\n",
		       addr, val, err);
		msleep(3);
	} while (retry <= SENSOR_MAX_RETRIES);

	return err;
}

static int sensor_write_reg16(struct i2c_client *client, u16 addr, u16 val)
{
	int err = 0;
	struct i2c_msg msg;
	unsigned char data[4];
	int retry = 0;

	if (!client->adapter)
		return -ENODEV;

	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);
        data[2] = (u8) (val >> 8);
	data[3] = (u8) (val & 0xff);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 4;
	msg.buf = data;

	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry++;
		pr_err("mt9p111 : i2c transfer failed, retrying %x %x %d\n",
		       addr, val, err);
		msleep(3);
	} while (retry <= SENSOR_MAX_RETRIES);

	return err;
}
static u8 get_sensor_8len_reg(struct i2c_client *client, u16 reg)
{
    int err;
	u8 val;
     
    err = sensor_read_8reg(client, reg, &val);

    if (err)
      return 0;

    return val;
}
static u8 check_sensor_type(struct sensor_info *info){
	u8 id_msb,id_lsb;
	u16 val =0x0000;
	u8 sensor_type = SENSOR_Invalid;

	id_msb = get_sensor_8len_reg(info->i2c_client,0x0000); 
	id_lsb = get_sensor_8len_reg(info->i2c_client,0x0001); 
	val = id_msb << 8|id_lsb;
	debug_print("%s:====>%02x %02x val = %x\n",__func__,id_msb,id_lsb,val);
	if(val == 0x1011) {
		sensor_type = SENSOR_T8EV5;
		goto found_sensor;
	}else if(val == 0x2880){
		sensor_type = SENSOR_MT9P11;
		goto found_sensor;
	}
	
	sensor_read_reg(info->i2c_client,0x0000,&val);
	debug_print("%s:====> %x\n",__func__ ,val);
	if(val == 0x2880) {
		sensor_type = SENSOR_MT9P11;
	}
found_sensor:
	debug_print("%s: sensor type =%d\n",__func__,sensor_type);
	return sensor_type;
}
static int sensor_open(struct inode *inode, struct file *file)
{
	pr_info("mt9p111 %s\n",__func__);
	file->private_data = mt9p111_sensor_info;
	
	return 0;
}

int sensor_release(struct inode *inode, struct file *file)
{

if (mt9p111_sensor_info->pdata && mt9p111_sensor_info->pdata->power_off)
		mt9p111_sensor_info->pdata->power_off(mt9p111_sensor_info->cur_flag);
	printk("%s:=========flag =0x%02x\n",__func__,mt9p111_sensor_info->cur_flag);
	atomic_set(&mt9p111_sensor_info->power_on_loaded, 0);
	atomic_set(&mt9p111_sensor_info->sensor_init_loaded, 0);
	file->private_data = NULL;
	return 0;
}

#if _debug_is
static void sn65dsi83_dump_regs(struct i2c_client *client,const struct sensor_reg *next){
	u8 val=0x00;
	u16 val_16=0x0000;
	if(next->op==WRITE_REG_DATA8){
		sensor_read_8reg(client,next->addr,&val);
	dev_info(&client->dev,
		"%s:0x%04x = 0x%02x\n", client->name,next->addr,val);
		}
	else if(next->op==WRITE_REG_DATA16){
		sensor_read_reg(client,next->addr,&val_16);
		dev_info(&client->dev,
		"%s:0x%04x = 0x%04x\n", client->name,next->addr,val_16);
		}
	
}

static int bridge_read_table(struct i2c_client *client,
			      const struct sensor_reg table[])
{
	const struct sensor_reg *next;

    
	dev_info(&client->dev," %s: =========>exe start\n",__func__);
       next = table ;       

	for (next = table; next->op!= SENSOR_TABLE_END; next++) {
	       sn65dsi83_dump_regs(client, next);
	}
	dev_info(&client->dev," %s: =========>exe end\n",__func__);
	return 0;
}
#endif
static bool T8EV5_af_get_cpu_st(struct i2c_client *client)
{
	int i=10;
	while(i)
	{
		i--;
		if(get_sensor_8len_reg(client, 0x2800) & 0x80)
			continue;
		else
			return true;
	}
	return false;
}

static int sensor_write_table(struct i2c_client *client,
			      const struct sensor_reg table[])
{
	const struct sensor_reg *next;
	int err,index = 0,bef_idx=0,af_idx=0;
    	u8 val;
	//pr_info("mt9p111 %s \n",__func__);
    
    next = table ;       

	for (next = table; next->op != SENSOR_TABLE_END; next++) {

		switch (next->op) {
		case WRITE_REG_DATA8:
		{
	        //pr_info("write reg data 8 addr=%04x\n",next->addr);
	              val = (u8)(next->val&0xff);
			err = sensor_write_reg8(client, next->addr,
				val);
			if (err)
				return err;
			break;
		}
		case WRITE_REG_DATA16:
		{
	        //pr_info("write reg data 16 \n");
			err = sensor_write_reg16(client, next->addr,
				next->val);
			if (err)
				return err;
			break;
		}
		case T8EV5_FOCUS_Get_CPU_ST:
		{
			bef_idx = af_idx = index;
			bef_idx -= 1;
			af_idx += 1;
			if(!T8EV5_af_get_cpu_st(client))
				printk("%s:==at 0x%04x~0x%04x af set fail\n",__func__,((bef_idx>=0)?table[bef_idx].addr:0),table[af_idx].addr);
			break;
		}
		case SENSOR_WAIT_MS:
		{
			msleep(next->val);
			break;
		}
		default:
			pr_err("%s: invalid operation 0x%x\n", __func__,
				next->op);
			return err;
		}
		index++;
	}
	return 0;
}

static bool camdrv_mt9p111_check_flash_needed(struct sensor_info *info)
{    

	u16 average = 0;   
	int err = 0;    
	bool needflash = false;    // get average	

	err = sensor_write_reg16(info->i2c_client, 0x098E, 0xb804);
    	if (err){
		dev_err(&info->i2c_client->dev,"write 0x098E fail!!\n");
		needflash = false;
    	}
		
	err = sensor_read_reg(info->i2c_client, 0xb804, &average);
    	if (err){
		dev_err(&info->i2c_client->dev,"read 0xb804 fail!!\n");
		needflash = false;	
    	}
	 
	pr_err("%s:============>average= %d\n",__func__,average);
	if (average < 0x04)		
		needflash = true;	
	else		
		needflash = false;	
	return needflash;
}

static bool check_t8ev5_need_flash(struct sensor_info *info)
{
	// Get the Gain value calculated by ALC
	u16 gain_AG;
	u16 againH,againL;
	bool needflash = false;    // get average	
	
	againH=get_sensor_8len_reg(info->i2c_client,0x0361);				
	againL=get_sensor_8len_reg(info->i2c_client,0x0362);
	
	gain_AG=againH<<8|againL; //max 
	debug_print("%s:=========>gain_AG = %d\n",__func__,gain_AG);
	if(gain_AG >= FLASH_AGAIN_SIZE)
		needflash = true;	
	else		
		needflash = false;	
	return needflash;
}

static bool check_sensor_flash(struct sensor_info *info){
	bool need_flash = false;
	if(info->sensor_type == SENSOR_MT9P11)
		need_flash = camdrv_mt9p111_check_flash_needed(info);
	else if(info->sensor_type == SENSOR_T8EV5)
		need_flash = check_t8ev5_need_flash(info);
	return need_flash;
}
static void flash_work_func(struct work_struct *work)
{

	struct sensor_info *info =
		container_of(work, struct sensor_info, work);
	if(info->force_flash_on){
		switch(info->flash_mode){
			  case NVMMCAMERA_FLASHMODE_STILL_MANUAL:
			  	gpio_set_value(info->led_en, 1);
				gpio_set_value(info->flash_en, 0);
			  	break;
	                case NVMMCAMERA_FLASHMODE_STILL_AUTO:
				if(check_sensor_flash(info)){
					gpio_set_value(info->led_en, 1);
					gpio_set_value(info->flash_en, 0);
				}
	                     break;
			 case NVMMCAMERA_FLASHMODE_REDEYE_REDUCTION:
			  	gpio_set_value(info->led_en, 1);
				gpio_set_value(info->flash_en, 1);
				msleep(50);
				gpio_set_value(info->led_en, 0);
				gpio_set_value(info->flash_en, 0);
				msleep(10);
				gpio_set_value(info->led_en, 1);
				gpio_set_value(info->flash_en, 0);
			  	break;
	                default:
	                     break;
			}
		}else{
			switch(info->flash_mode){
				case NVMMCAMERA_FLASHMODE_STILL_MANUAL:
				case NVMMCAMERA_FLASHMODE_STILL_AUTO:
				case NVMMCAMERA_FLASHMODE_REDEYE_REDUCTION:
					gpio_set_value(info->led_en, 0);
					gpio_set_value(info->flash_en, 0);
					break;
		}
		}
}
static void af_flash_timer(unsigned long _data)
{
	struct sensor_info *info = (struct sensor_info *)_data;
	debug_print("%s:======>af_flash_timer flash off\n",__func__);
	info->force_flash_on = false;
	info->have_task = false;
	schedule_work(&info->work);
}
static void trigger_flash(struct sensor_info *info,bool delay,bool isForced)
{
	if(!info->flash_enable) return;
	debug_print("%s:======>timer_debounce=%dms\n",__func__,info->timer_debounce);
	info->force_flash_on = isForced;
	if(!delay&&!isForced)goto work;
	if(info->flash_mode == NVMMCAMERA_FLASHMODE_STILL_AUTO&&!check_sensor_flash(info)) return;
	if ((info->timer_debounce&&delay)||(isForced&&!delay&&info->have_task)){
		info->have_task = true;
		debug_print("%s:======>start delay\n",__func__);
		mod_timer(&info->timer,
			jiffies + msecs_to_jiffies(info->timer_debounce));
	}
work:
	schedule_work(&info->work);
}

static void JT8EV5WriteGain(struct sensor_info *info,u32 gain)
{
	sensor_write_reg8(info->i2c_client,0x0309,(u8)((gain&0x0F000000) >> 24)); 
	sensor_write_reg8(info->i2c_client,0x030A,(u8)((gain&0x00FF0000) >> 16));
	sensor_write_reg8(info->i2c_client,0x030B,(u8)((gain&0x000003FF) >> 2));
}
static void JT8EV5WriteShutter(struct sensor_info *info,u16 Shutter)
{
	sensor_write_reg8(info->i2c_client,0x0307,(u8)((Shutter&0x0000FF00)>>8));
	sensor_write_reg8(info->i2c_client,0x0308,(u8)(Shutter&0x000000FF) );
} 


static u32 JT8EV5ReadGain(struct sensor_info *info)
{
	// Get the Gain value calculated by ALC
	u16 gain_DG,gain_AG;
	u16 againH,againL;
	u16 dgainH,dgainL;
	
	againH=get_sensor_8len_reg(info->i2c_client,0x0361);				
	againL=get_sensor_8len_reg(info->i2c_client,0x0362);
	

	dgainH=get_sensor_8len_reg(info->i2c_client,0x0363);				
	dgainL=get_sensor_8len_reg(info->i2c_client,0x0364);
	
	gain_AG=againH<<8|againL; 
	gain_DG=(dgainH<<8|dgainL);
	return ((u32) gain_AG <<16 | gain_DG);
}

static void JT8EV5AeEnable(struct sensor_info *info,int Enable)
{
	// to enable or disable AE
  	sensor_write_reg8(info->i2c_client,0x0300, (Enable?0x02:0x00));	
}	

static void JT8EV5AwbEnable(struct sensor_info *info,int Enable)
{
//    To enable or disable AWB	
//	sensor_write_reg8(info->i2c_client,0x0320,(get_sensor_8len_reg(info->i2c_client,0x0320)&(0x7F))|(Enable?0x80:0x00));
    if (Enable) {
		sensor_write_reg8(info->i2c_client,0x033E,0x45);//CGRANGE[1:0]/CGRANGE_EX/-/AWBSPD[3:0];
	} else {
		sensor_write_reg8(info->i2c_client,0x033E,0x40);//CGRANGE[1:0]/CGRANGE_EX/-/AWBSPD[3:0];
	}
}

static int sensor_set_sensor_flag(struct sensor_info *info, u8 flag)
{
	int i;
	u8 sensor_type = SENSOR_T8EV5,change = 0;

	for(i = 1;i<MAX_SENSOR_COUNT;i++){
		if (mt9p111_sensor_info->pdata && mt9p111_sensor_info->pdata->power_on){
			
			if(!(flag & T8EV5_FRONT_ID)){
				flag |= (1<<(i+1));
			}
			
			debug_print("%s:=====start=====>flag =0x%02x\n",__func__,flag);
			if(mt9p111_sensor_info->cur_flag != 0){
				 change = ((mt9p111_sensor_info->cur_flag&0x01)  ^ ( flag&0x01));
				if(!change)flag = mt9p111_sensor_info->cur_flag;
				printk("%s:============>old flag =0x%02x,is change front bit=%x\n",__func__,flag,change);
			}
			mt9p111_sensor_info->pdata->power_on(flag);
			
			if(mt9p111_sensor_info->cur_flag != 0&&!change){
				printk("%s:=====cur flag is none null and front bit unchange so skip=====>flag =0x%02x\n",__func__,flag);
				if(mt9p111_sensor_info->cur_flag&MT9P111_TYPE_ID)
					sensor_type = SENSOR_MT9P11;
				else
					sensor_type = SENSOR_T8EV5;
				goto found;
			}
			if(flag & T8EV5_FRONT_ID){
				if(flag&MT9P111_TYPE_ID)
					sensor_type = SENSOR_MT9P11;
				else
					sensor_type = SENSOR_T8EV5;
				printk("%s:=====is front so found=====>flag =0x%02x\n",__func__,flag);
				goto found;
			}
			sensor_type = check_sensor_type(mt9p111_sensor_info);
			if(sensor_type != SENSOR_Invalid){
				goto found;
			}else{
				mt9p111_sensor_info->pdata->power_off(flag);
				flag &= (~ (1<<(i+1)));
				debug_print("%s:===remove=======>flag =0x%02x\n",__func__,flag);
			}
			
		}
	}
	return -EFAULT;
found:
	printk("%s: sensor type= %d current flag =0x%02x\n",__func__,sensor_type,flag);
	atomic_set(&mt9p111_sensor_info->power_on_loaded, (sensor_type != SENSOR_Invalid));
	atomic_set(&mt9p111_sensor_info->sensor_init_loaded, 0);
	mt9p111_sensor_info->sensor_type = sensor_type;
	mt9p111_sensor_info->cur_flag = flag;
	return 0;
}
static int sensor_set_af_mode(struct sensor_info *info, u8 mode)
{
	int err = 0;

	pr_info("%s: mode %d\n",__func__, mode);
	
	if(mode != SENSOR_AF_FULLTRIGER&&mode != SENSOR_AF_INIFINITY&&mode != SENSOR_AF_CONTINUOUS)return err;
	debug_print("%s: ##############################af mode %d\n",__func__, mode);
	if(info->sensor_type == SENSOR_T8EV5)
		err = sensor_write_table(info->i2c_client, GET_AF_MODE_TABLE(t8ev5)[mode]);
	else if(info->sensor_type == SENSOR_MT9P11)
		err = sensor_write_table(info->i2c_client, GET_AF_MODE_TABLE(mt9p111)[mode]);
    	
	if (err)
	    return err;

	return 0;
}

static int sensor_start_flash_handler(struct sensor_info *info, int mode)
{


	debug_print("%s: flash mode %d\n",__func__, mode);
	switch(mode){
                case NVMMCAMERA_FLASHMODE_TORCH:
		   	debug_print("%s:==========> set llp3367 torch mode\n",__func__);
			gpio_set_value(info->led_en, 1);
			gpio_set_value(info->flash_en, 0);
                     break;

                default:
			debug_print("%s:==========> set llp3367 flash off mode\n",__func__);
			gpio_set_value(info->led_en, 0);
			gpio_set_value(info->flash_en, 0);
                     break;
		}
	info->flash_mode = mode;
	return 0;
}

static int set_t8ev5_mode(struct sensor_info *info,int sensor_table){

	int err = 0;
  	u16 Preview_H_count=1472,Capshutter,Capture_H_count=2880;	
	u8 wbmr_msb,wbmr_lsb,wbmg_msb,wbmg_lsb,wbmb_msb,wbmb_lsb;
	u16 PVshutter;
	u32 gain,Preview_spclk_in_K,Capture_spclk_in_K;
	u32 SPCLK = 45600;

	
	if(sensor_table == SENSOR_MODE_2592x1944){
		
		mutex_lock(&info->als_get_sensor_value_mutex);
		
		wbmr_msb = (u8)(get_sensor_8len_reg(info->i2c_client,0x036E)&0x03); 									 
		wbmr_lsb = (u8)get_sensor_8len_reg(info->i2c_client,0x036F);//the RGain value read from AWB for manual WB
		wbmg_msb = (u8)(get_sensor_8len_reg(info->i2c_client,0x0370)&0x03); 									 
		wbmg_lsb = (u8)get_sensor_8len_reg(info->i2c_client,0x0371);//the GGain value read from AWB for manual WB
		wbmb_msb = (u8)(get_sensor_8len_reg(info->i2c_client,0x0372)&0x03); 									 
		wbmb_lsb = (u8)get_sensor_8len_reg(info->i2c_client,0x0373);//the BGain value read from AWB for manual WB
				
		gain = JT8EV5ReadGain(info);

		PVshutter = ((get_sensor_8len_reg(info->i2c_client,0x035F) << 8 )| get_sensor_8len_reg(info->i2c_client,0x0360));
		Preview_spclk_in_K = SPCLK;
		if(Preview_spclk_in_K==0) Preview_spclk_in_K=1;
		
		
		JT8EV5AwbEnable(info,0);   // Turn off AWB
		JT8EV5AeEnable(info,0);    // Turn off AE
	
		JT8EV5WriteGain(info,gain);
	
		Capture_spclk_in_K = SPCLK;
		if (Capture_spclk_in_K==0) Capture_spclk_in_K=1; // Prevent Divided by Zero
	
		// Calculate Shutter
		Capshutter=PVshutter;
		Capshutter=Capshutter*(Capture_spclk_in_K)/(Preview_spclk_in_K);
		Capshutter=Capshutter*(Preview_H_count)/(Capture_H_count);
		
		// Write the shutter back to sensor
		JT8EV5WriteShutter(info,Capshutter);
		mutex_unlock(&info->als_get_sensor_value_mutex);

	}else{
		JT8EV5AwbEnable(info,1);   // Turn on AWB
		JT8EV5AeEnable(info,1);    // Turn on AE
	}
	return err;
}
static int set_sensor_effect(struct sensor_info *info,int type){
	int err = 0;
	if(info->current_wb>YUV_Whitebalance_Invalid&&info->current_wb<YUV_Whitebalance_Shade&&(type == UPDATE_AWB||type == UPDATE_ALL)){
		if(info->sensor_type == SENSOR_T8EV5)
			err = sensor_write_table(info->i2c_client, GET_AWB_MODE_TABLE(t8ev5)[info->current_wb]);
		else if(info->sensor_type == SENSOR_MT9P11)
			err = sensor_write_table(info->i2c_client, GET_AWB_MODE_TABLE(mt9p111)[info->current_wb]);
		debug_effect_print("%s:===current_wb=%d======>\n",__func__,info->current_wb);
		if (err)
	    	goto exit;
	}
	if(info->current_exposure>YUV_YUVExposure_Invalid&&info->current_exposure<=YUV_YUVExposure_Negative2&&(type == UPDATE_EXP||type == UPDATE_ALL)){
		if(info->sensor_type == SENSOR_T8EV5)
		err = sensor_write_table(info->i2c_client, GET_EXPOSURE_MODE_TABLE(t8ev5)[info->current_exposure]);
		else if(info->sensor_type == SENSOR_MT9P11)
		err = sensor_write_table(info->i2c_client, GET_EXPOSURE_MODE_TABLE(mt9p111)[info->current_exposure]);
		debug_effect_print("%s:===current_exposure=%d==>\n",__func__,info->current_exposure);
	}
	
exit:
	return err;
}
void SOC5140_8405_Polling(struct sensor_info *info,u8 para)

{

   u8  temp_data = 0xff;

   u16  count = 0;

 // return;
    sensor_write_reg16(info->i2c_client, 0x098E, 0x8405);

   while(count < 100)

   {   

      sensor_read_8reg(info->i2c_client,0x8405,&temp_data);

     //printk("SOC5140_8405_Polling 0x8405=0x%02x\n",temp_data);

      if(temp_data == para)

      {
           
		//printk("  kevin 8405 ok  \n");

           break;

      }

      count ++;

      if(count > 99)

      {
		 // printk("kevin8405  error\n");

      }

      msleep(10);

      

  }

 

}

static int sensor_set_mode(struct sensor_info *info, struct sensor_mode *mode)
{
	//struct sensor_reg *mod_table,*int_table;
	int sensor_table;
	int err =0;
	u8 mode_val;

	if (mode->xres == 2592 && mode->yres == 1944)
		sensor_table = SENSOR_MODE_2592x1944;
	else if (mode->xres == 1280 && mode->yres == 720)
		sensor_table = SENSOR_MODE_1280x720;
	else if (mode->xres == 960 && mode->yres == 720)
		sensor_table = SENSOR_MODE_960x720;
	else if (mode->xres == 640 && mode->yres == 480)
		sensor_table = SENSOR_MODE_640x480;
	else {
		pr_err("%s: invalid resolution supplied to set mode %d %d\n",
		       __func__, mode->xres, mode->yres);
		return -EINVAL;
	}
	
	if(sensor_table == SENSOR_MODE_2592x1944){
		trigger_flash(info,false,true);
	}else{
		trigger_flash(info,false,false);
	}
	
	if(atomic_read(&info->power_on_loaded)&&!atomic_read(&info->sensor_init_loaded)){
		if(info->sensor_type == SENSOR_MT9P11){
			gpio_set_value(MT9P111_CAMERA_RST_PIN,0);
			msleep(5);
			gpio_set_value(MT9P111_CAMERA_RST_PIN,1);
			msleep(10);
			err = sensor_write_table(info->i2c_client,  GET_INIT_DATA(mt9p111));
			info->isFirstOpen = true;
		}else if(info->sensor_type == SENSOR_T8EV5){
			err = sensor_write_table(info->i2c_client, GET_INIT_DATA(t8ev5));
		}
			debug_print("%s:sensor camera init setting reg uninit will load reg!\n",__func__);
			if (err){
				pr_err("%s:sensor camera init setting reg failed!\n",__func__);
				atomic_set(&info->sensor_init_loaded, 0);
    				return err;
			}
			
			atomic_set(&info->sensor_init_loaded, 1);
	}else
		printk("%s:sensor camera init setting reg inited!\n",__func__);
	printk("%s:sensor type =%d!\n",__func__,info->sensor_type);
	if(info->sensor_type == SENSOR_T8EV5){
		err = set_t8ev5_mode(info,sensor_table);
		if (err)
    			return err;
	}
	printk("%s:set res (%d,%d)!\n",__func__,mode->xres,mode->yres);
	if(info->sensor_type == SENSOR_MT9P11){
		
		if(SENSOR_MODE_1280x720 == sensor_table){
                       gpio_set_value(MT9P111_CAMERA_RST_PIN,0);
                       msleep(5);
                       gpio_set_value(MT9P111_CAMERA_RST_PIN,1);
                       msleep(10);
                       err = sensor_write_table(info->i2c_client,  GET_INIT_DATA(mt9p111));
               }
		
		err = sensor_write_table(info->i2c_client, GET_MODE_TABLE(mt9p111)[sensor_table]);
	}else if(info->sensor_type == SENSOR_T8EV5){
		err = sensor_write_table(info->i2c_client, GET_MODE_TABLE(t8ev5)[sensor_table]);
	}
	
    	if (err)
    		return err;
	
	if(info->sensor_type == SENSOR_MT9P11)
	{
		if(sensor_table == SENSOR_MODE_2592x1944){
			mode_val = 0x07;
		}else{
			mode_val = 0x03;
		}
		debug_print("%s:start sensor polling mode_va=%d!\n",__func__,mode_val);
		SOC5140_8405_Polling(info,mode_val);
		debug_print("%s:end sensor polling!\n",__func__);
	}
	
	info->mode = sensor_table;
	return 0;
}

static bool chectRectEq(struct t8ev5_win_pos old_rect,struct t8ev5_win_pos new_rect)
{
	bool ret = false;
	ret = (old_rect.left == new_rect.left)&&(old_rect.top == new_rect.top)&&
		 (old_rect.bottom== new_rect.bottom)&&(old_rect.right== new_rect.right);

	return ret;
}

static int t8ev5_get_windows_position(struct sensor_info *info,u8* pos){
	int ret = 0;
	int i ;
	
	int pointW = 0,pointH = 0;
	int point_x=0,point_y=0;
	u8 selected_x=3,selected_y=3;
	
	struct t8ev5_win_pos windows = info->win_pos;
	if(chectRectEq(info->win_pos_old, info->win_pos)&&0){

		pr_err("%s:rect eq so return do nonthing!!!\n",__func__);
		return -EINVAL;
	}
	debug_print("%s:========>(%d,%d,%d,%d)\n",__func__,windows.left,windows.top,windows.right,windows.bottom);
         if (windows.left < (-ANDROID_WINDOW_PRECISION) ||
             windows.right > (ANDROID_WINDOW_PRECISION) ||
             windows.top < (-ANDROID_WINDOW_PRECISION) ||
             windows.bottom > (ANDROID_WINDOW_PRECISION) ||
             windows.left >= windows.right ||
             windows.top >= windows.bottom )
         {
             dev_err(&info->i2c_client->dev,"Failed windows capability check, "\
                 "please check your window bounds and : "\
                 "(%d,%d,%d,%d)",
                 windows.left,
                 windows.top,
                 windows.right,
                 windows.bottom);
		return -EINVAL;
         }
       pointW = windows.right-windows.left;
	pointH = windows.bottom-windows.top;
	
	point_x = ANDROID_WINDOW_PRECISION + (windows.left+pointW/2);
	point_y = ANDROID_WINDOW_PRECISION + (windows.top+pointH/2);
	//x1= -625,x2= -375, x3= -125,x4= -625,x1= -625,x1= -625,x1= -625,x1= -625
	debug_print("t8ev5_get_windows_position:(%d,%d)\n",point_x,point_y);
	for(i=0;i<MAX_POS-1;i++){
		if(point_x>=0&&point_x< 3*(PER_SIZE_W/2))
			selected_x=0;
		else if(point_x>= 3*(PER_SIZE_W/2)&&point_x<3*(PER_SIZE_W/2)+PER_SIZE_W)
			selected_x=1;
		else if(point_x>=3*(PER_SIZE_W/2)+PER_SIZE_W&&point_x<3*(PER_SIZE_W/2)+2*PER_SIZE_W)
			selected_x=2;
		else if(point_x>=3*(PER_SIZE_W/2)+2*PER_SIZE_W&&point_x<3*(PER_SIZE_W/2)+3*PER_SIZE_W)
			selected_x=3;
		else if(point_x>=3*(PER_SIZE_W/2)+3*PER_SIZE_W&&point_x<3*(PER_SIZE_W/2)+4*PER_SIZE_W)
			selected_x=4;
		else if(point_x>=3*(PER_SIZE_W/2)+4*PER_SIZE_W&&point_x<3*(PER_SIZE_W/2)+5*PER_SIZE_W)
			selected_x=5;
		else if(point_x>=3*(PER_SIZE_W/2)+5*PER_SIZE_W&&point_x<PREVIEW_SIZE_W)
			selected_x=6;
		else
			 dev_err(&info->i2c_client->dev,"fRegions.win_x = %d unfound!!\n",point_x);
		
		if(point_y>=0&&point_y<3*(PER_SIZE_H/2))//135
			selected_y =0;
		else if(point_y>=3*(PER_SIZE_H/2)&&point_y<3*(PER_SIZE_H/2)+PER_SIZE_H)
			selected_y=1;
		else if(point_y>=3*(PER_SIZE_H/2)+PER_SIZE_H&&point_y<3*(PER_SIZE_H/2)+2*PER_SIZE_H)
			selected_y=2;
		else if(point_y>=3*(PER_SIZE_H/2)+2*PER_SIZE_H&&point_y<3*(PER_SIZE_H/2)+3*PER_SIZE_H)
			selected_y=3;
		else if(point_y>=3*(PER_SIZE_H/2)+3*PER_SIZE_H&&point_y<3*(PER_SIZE_H/2)+4*PER_SIZE_H)
			selected_y=4;
		else if(point_y>=3*(PER_SIZE_H/2)+4*PER_SIZE_H&&point_y<3*(PER_SIZE_H/2)+5*PER_SIZE_H)
			selected_y=5;
		else if(point_y>=3*(PER_SIZE_H/2)+5*PER_SIZE_H&&point_y<PREVIEW_SIZE_H)
			selected_y=6;
		else
			 dev_err(&info->i2c_client->dev,"fRegions.win_y = %d unfound!!\n",point_y);
	}
		  if(selected_x<=1)
	  {
		  if(selected_y<=1)
		  {
			  //A1
			  sensor_write_reg8(info->i2c_client, 0x030C,0xD0);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x10);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x00);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x00);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
		  else if(selected_y<=4)
		  {
			  //B1
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x40);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x34);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x04);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x00);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
		  else 
		  {
			  //C1
			  sensor_write_reg8(info->i2c_client, 0x030C,0x00);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x10);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x0D);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x00);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
	  }
	  else if(selected_x==2)
	  {
		  if(selected_y<=1)
		  {
			  //A2
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x74);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x04);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x00);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x00);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
		  else if(selected_y<=4)
		  {
			  //B2
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x10);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x1D);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x01);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x00);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
		  else 
		  {
			  //C2
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x00);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x04);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x07);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x40);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
	  }
	  else if(selected_x==3)
	  {
		  if(selected_y<=1)
		  {
			  //A3
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x1D);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x01);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x00);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x00);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
		  else if(selected_y<=4)
		  {
			  //B3
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x04);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x07);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x40);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x40);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
		  else 
		  {
			  //C3
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x00);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x01);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x01);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0xD0);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
	  }
	  else if(selected_x==4)
	  {   
		  if(selected_y<=1)
		  {
			  //A4
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x07);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x40);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x40);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x00);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
		  else if(selected_y<=4)
		  {
			  //B4
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x01);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x01);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0xD0);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x54);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
		  else 
		  {
			  //C4
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x00);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x00);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x40);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x74);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
		  
	  }
	  else
	  {
		  if(selected_y<=1)
		  {
			  //A5
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x01);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0xC0);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x10);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x00);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
		  else if(selected_y<=4)
		  {
			  //B5
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x00);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x40);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x70);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x04);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
		  else 
		  {
			  //C5
			  
			  sensor_write_reg8(info->i2c_client, 0x030C,0x00);//A1WEIGHT[1:0]/A2WEIGHT[1:0]/A3WEIGHT[1:0]/A4WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030D,0x00);//A5WEIGHT[1:0]/B1WEIGHT[1:0]/B2WEIGHT[1:0]/B3WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030E,0x10);//B4WEIGHT[1:0]/B5WEIGHT[1:0]/C1WEIGHT[1:0]/C2WEIGHT[1:0]
			  sensor_write_reg8(info->i2c_client, 0x030F,0x1C);//C3WEIGHT[1:0]/C4WEIGHT[1:0]/C5WEIGHT[1:0]/-/-
		  }
	   }
	*pos=((selected_x << 4) |selected_y);
	info->win_pos_old = info->win_pos;
	debug_print("t8ev5_get_windows_position:select_pos=0x%02x\n",*pos);
	return ret ;
}
static int mt9p111_get_windows_position(struct sensor_info *info,u8* pos){
	int ret = 0;
	int y,x,index = 0;
	
	int pointW = 0,pointH = 0;
	int point_x=0,point_y=0;
	
	struct t8ev5_win_pos windows = info->win_pos;
	if(chectRectEq(info->win_pos_old, info->win_pos)&&0){

		pr_err("%s:rect eq so return do nonthing!!!\n",__func__);
		return -EINVAL;
	}
	debug_print("%s:========>(%d,%d,%d,%d)\n",__func__,windows.left,windows.top,windows.right,windows.bottom);
         if (windows.left < (-ANDROID_WINDOW_PRECISION) ||
             windows.right > (ANDROID_WINDOW_PRECISION) ||
             windows.top < (-ANDROID_WINDOW_PRECISION) ||
             windows.bottom > (ANDROID_WINDOW_PRECISION) ||
             windows.left >= windows.right ||
             windows.top >= windows.bottom )
         {
             dev_err(&info->i2c_client->dev,"Failed windows capability check, "\
                 "please check your window bounds and : "\
                 "(%d,%d,%d,%d)",
                 windows.left,
                 windows.top,
                 windows.right,
                 windows.bottom);
		return -EINVAL;
         }
       pointW = windows.right-windows.left;
	pointH = windows.bottom-windows.top;
	
	point_x = ANDROID_WINDOW_PRECISION + (windows.left+pointW/2);
	point_y = ANDROID_WINDOW_PRECISION + (windows.top+pointH/2);
	//x1= -625,x2= -375, x3= -125,x4= -625,x1= -625,x1= -625,x1= -625,x1= -625
	debug_print("mt9p111_get_windows_position:(%d,%d)\n",point_x,point_y);
	for(y=0;y<4;y++){
		for(x=0;x<4;x++){
			if((MT9P111_PER_SIZE_W*x)<=point_x&&point_x<(MT9P111_PER_SIZE_W*(x+1))&& \
				(MT9P111_PER_SIZE_H*y)<=point_y&&point_y<(MT9P111_PER_SIZE_H*(y+1))){
				goto found;
			}
			index +=1;
		}
	}
	*pos = 7;
	return ret;
found:
	*pos = index;
	info->win_pos_old = info->win_pos;
	debug_print("mt9p111_get_windows_position:select_pos=%d\n",*pos);
	return 0 ;
}
static void sensor_set_op_mode(struct sensor_info *info, KhOperationalMode *mode)
{
	//fix 24fps

	u16 addr = 0x27FF;
   	u8 val = 0x30;
	int err = 0;
	pr_info(" sensor_set_op_mode : pA = %d,vA = %d,\n",mode->PreviewActive,mode->VideoActive);
	
	info->isRecoded = (mode->PreviewActive&&mode->VideoActive)?1:0;
	if(info->isRecoded){
		pr_info(" sensor_set_op_mode : it will set fixed rate!!\n");
		if(info->sensor_type == SENSOR_T8EV5){
			err = sensor_write_reg8(info->i2c_client,addr, val);
			if (err){
				pr_err("%s:=========>wirte 0x%04x fail!err=%d\n",__func__,addr,err);
			}
		}
		
	}else if(atomic_read(&info->power_on_loaded)&&atomic_read(&info->sensor_init_loaded)){
		pr_info(" sensor_set_op_mode : it will set auto rate!!\n");
		if(info->sensor_type == SENSOR_T8EV5){
			addr = 0x27FF;
			val = get_sensor_8len_reg(info->i2c_client,addr);
			
		   	if(val == 0x30){
				err = sensor_write_reg8(info->i2c_client,addr, 0xf7);
				if (err){
					pr_err("%s:=========>wirte 0x%04x fail!err=%d\n",__func__,addr,err);
				}
				pr_info(" sensor_set_op_mode : it will set 0x0xf7!!\n");
		   	}
		}
		
	}
}
static int t8ev5_set_windows_position(struct sensor_info *info)
{
       u8 val;         
	int err = 0,i,try_max = 10;
	u8 pos =0x33;
	debug_print("t8ev5_set_windows_position:=============> exe\n");
	err=t8ev5_get_windows_position(info,&pos);
	if (err)return err;
	trigger_flash(info,true,true);
	err = sensor_write_reg8(info->i2c_client, 0x2401,0x15);//SET_WINDOW_POSITION
	if (err)return err;
	err = sensor_write_reg8(info->i2c_client, 0x2402,pos);//Y[0-6]/X[0-6] 
	if (err)return err;
	for(i =0;i<try_max;i++){
		val = get_sensor_8len_reg(info->i2c_client, 0x2800);//Wait for 0x2800 until finish 
		debug_print("%s:=============>val(0x2800) = %x\n",__func__,val);
		if(!(val&(1<<7)))break;
		else
			mdelay(10);
	}
	//pr_err("%s:=============>posx = %d,posy =%d pos=0x%02x\n",__func__,info->win_pos.win_x,info->win_pos.win_y,pos);
 
	///  ((info->win_pos.win_x << 4) |info->win_pos.win_y)
	if(i<try_max){
		pr_err("%s:=============>will write 0x27ff\n",__func__);
		err = sensor_write_reg8(info->i2c_client, 0x27FF,0xE1);//WRITE
		if (err)return err;
		err = sensor_write_reg8(info->i2c_client, 0x27FF,0x28);//WRITE one shut
		if (err)return err;
	}else
		err = -EINVAL;
	return err;
}
static int mt9p111_set_windows_position(struct sensor_info *info)
{
	int err = 0;
	u8 pos =0x7;
	debug_print("mt9p111_set_windows_position:=============> exe\n");
	err=mt9p111_get_windows_position(info,&pos);
	if (err)return err;
	trigger_flash(info,true,true);
	err = sensor_write_table(info->i2c_client,  mt9p111_point_af[pos]);
	return err;
}

static int set_sensor_windows_position(struct sensor_info *info)
{
       int err = 0;
	if(info->sensor_type == SENSOR_T8EV5)
		err = t8ev5_set_windows_position(info);
	else if(info->sensor_type == SENSOR_MT9P11)
		err = mt9p111_set_windows_position(info);
	return err;
}
static int sensor_get_af_status(struct sensor_info *info,u8 *val)
{
	int err;
	if(info->sensor_type == SENSOR_T8EV5){
		err = t8ev5_read_8reg(info->i2c_client, 0x2800, val);
		if (err)goto fail;
	}else if(info->sensor_type == SENSOR_MT9P11){
		err = sensor_write_reg16(info->i2c_client, 0x098E, 0xB006);
		if (err)goto fail;
   		 err = sensor_read_8reg(info->i2c_client, 0xB006, (u8*)val);
		 if (err)goto fail;
	}
	debug_print("%s: value 0x%02x sensor_type =%d\n", __func__, *val,info->sensor_type);
	return 0;
fail:
	pr_err("%s: err =  %d \n",__func__, err);
    	return err;
}
static long sensor_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	struct sensor_info *info = file->private_data;


	switch (cmd) 
	{
		case SENSOR_IOCTL_SET_MODE:
			{
				struct sensor_mode mode;
				if (copy_from_user(&mode,
							(const void __user *)arg,
							sizeof(struct sensor_mode))) {
					return -EFAULT;
				}

				return sensor_set_mode(info, &mode);
			}
		case T8EV5_IOCTL_SET_WINDOWS_POS:
			{
				if (copy_from_user(&info->win_pos,
							(const void __user *)arg,
							sizeof(struct t8ev5_win_pos))) {
					return -EFAULT;
				}

				return set_sensor_windows_position(info);
			}
		case SENSOR_IOCTL_SET_OP_MODE:
			{
				KhOperationalMode mode;
				if (copy_from_user(&mode,
							(const void __user *)arg,
							sizeof(KhOperationalMode))) {
					return -EFAULT;
				}

				sensor_set_op_mode(info, &mode);
			}
		case SENSOR_IOCTL_SET_SENSOR_ID:
			{
				u8 flag;

				if (copy_from_user(&flag,
							(const void __user *)arg,
							sizeof(flag))) {
					return -EFAULT;
				}
				return sensor_set_sensor_flag(info, flag);
			}
		case SENSOR_IOCTL_SET_FLASH_MODE:
			{
				u8 flash_mode;

				if (copy_from_user(&flash_mode,
							(const void __user *)arg,
							sizeof(flash_mode))) {
					return -EFAULT;
				}
				sensor_start_flash_handler(info,flash_mode);
			}
		case SENSOR_IOCTL_GET_AF_STATUS:
			{
				int err;
				u8 val;

				err = sensor_get_af_status(info, &val);
				if (err)
					return err;

				if (copy_to_user((void __user *) arg,
							&val, sizeof(val))) {
					pr_err("%s: 0x%x\n", __func__, __LINE__);
					return -EFAULT;
				}
				break;
			}
		case SENSOR_IOCTL_GET_SENSOR_TYPE:
			{
				u8 val;

				val = info->sensor_type;

				if (copy_to_user((void __user *) arg,
							&val, sizeof(val))) {
					pr_err("%s: 0x%x\n", __func__, __LINE__);
					return -EFAULT;
				}
				break;
			}
		case SENSOR_IOCTL_SET_AF_MODE:
			{
				return sensor_set_af_mode(info, (u8)arg);
			}break;
		case SENSOR_IOCTL_SET_WHITE_BALANCE:
		case SENSOR_IOCTL_SET_YUV_EXPOSURE:
			{
				u8 effec_val;
				int type;

				if (copy_from_user(&effec_val,
							(const void __user *)arg,
							sizeof(effec_val))) {
					return -EFAULT;
				}

				if(cmd == SENSOR_IOCTL_SET_WHITE_BALANCE){
					info->current_wb= effec_val;
					type = UPDATE_AWB;
				} else{
					info->current_exposure= effec_val;
					type = UPDATE_EXP;
				}
				return set_sensor_effect(info,type);
			}

		default:
			return -EINVAL;
	}
	return 0;
}


static const struct file_operations sensor_fileops = {
	.owner = THIS_MODULE,
	.open = sensor_open,
	.unlocked_ioctl = sensor_ioctl,
	.release = sensor_release,
};

static struct miscdevice sensor_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SENSOR_NAME,
	.fops = &sensor_fileops,
};
static ssize_t camera_sensor_type(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_info *info = mt9p111_sensor_info;
	ssize_t res = 0;
	u8 sensor_type = SENSOR_Invalid;
	if(info!=NULL)
		sensor_type = info->sensor_type;
	debug_print("%s:==========>sensorType = %d\n",__func__,sensor_type);
   	res = snprintf(buf, 128,"%d",sensor_type);
	return res;
}
static ssize_t hm_camera_effect_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sensor_info *info = mt9p111_sensor_info;
    ssize_t res = 0;

	debug_print("%s:==========>current_addr=========>0x%04x\n",__func__,info->current_addr);

	mutex_lock(&info->als_get_sensor_value_mutex);
	res = snprintf(buf, 1024,
		"0x%02x\n",get_sensor_8len_reg(info->i2c_client,info->current_addr));
	mutex_unlock(&info->als_get_sensor_value_mutex);


	
	return res;
}
#if _debug_is
static ssize_t hm_camera_effect_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sensor_info *info = mt9p111_sensor_info;
    //sscanf(buf, "%s",&vales);
   u16 addr = 0x0000;
   u8 val = 0x00;
   u16 val_16=0x0000;
   u8 val_r8=0x00;
   u16 val_r16=0x0000;
   u8 byte_count=0;
   int err=0;
   unsigned int orgfs;
   long lfile=-1;
    char data[33];
   memset(data, 0, sizeof(data));
   strncpy(data,buf,sizeof(data)); 
   if(strncmp(data, "txt", 3)){
	   addr = simple_strtoul(buf, NULL, 16);
   	       if(count==7)
	   	{
	   	byte_count = simple_strtoul(buf+5, NULL, 16);
		if(byte_count==1)
			{
	   		if(!sensor_read_8reg(info->i2c_client,addr,&val_r8))
				debug_print("sensor_read_reg addr = 0x%04x,vales = 0x%02x\n",addr,val_r8);
			}
		else if(byte_count==2)
			{
	   		if(!sensor_read_reg(info->i2c_client,addr,&val_r16))
				debug_print("sensor_read_reg addr = 0x%04x,vales = 0x%04x\n",addr,val_r16);
			}
	   	}
	   if(count == 8)
	   	val = simple_strtoul(buf+5, NULL, 16);
	   if(count == 10)
	   	val_16 = simple_strtoul(buf+5, NULL, 16);
	   // mutex_lock(&info->als_get_sensor_value_mutex);
	    if(count == 8){
		    err = sensor_write_reg8(info->i2c_client,addr, val);
		    if (err){
				pr_err("%s:=========>wirte 0x%04x fail!err=%d\n",__func__,addr,err);
		    	}
			
			pr_err("%s:==> addr = 0x%04x,vales = 0x%02x,count=%d,inputLen=%d\n",__func__,addr, val,count,strlen(buf));
	    }
	  if(count == 10){
		    err = sensor_write_reg16(info->i2c_client,addr, val_16);
		    if (err){
				pr_err("%s:=========>wirte 0x%04x fail!err=%d\n",__func__,addr,err);
		    	}
			
			pr_err("%s:==> addr = 0x%04x,vales = 0x%04x,count=%d,inputLen=%d\n",__func__,addr, val_16,count,strlen(buf));
	    }
	  if(count<=2&&addr == 1){
	    	pr_info("%s:=========> 1\n",__func__);
	    	bridge_read_table(info->i2c_client,GET_INIT_DATA(mt9p111));

	    }
	    		
	   info->current_addr = addr;
	   mutex_unlock(&info->als_get_sensor_value_mutex);
   }else{
	orgfs = get_fs();
// Set segment descriptor associated to kernel space
	set_fs(KERNEL_DS);
	
        lfile=sys_open(mDebugFileName,O_WRONLY|O_CREAT, 0777);
	if (lfile < 0)
	{
 	 	printk("sys_open %s error!!. %ld\n",mDebugFileName,lfile);
	}
	else
	{
   	 	sys_lseek(lfile, 142, 0);  
		   	
		sys_read(lfile, data, sizeof(data)-1);
		debug_print("cid_show: =%s\n",data);
		
	 	sys_close(lfile);
	}
	set_fs(orgfs);
    }
    
    
    debug_print("%s:==> addr = 0x%04x,vales = 0x%02x,count=%d,inputLen=%d\n",__func__,addr, val,count,strlen(buf));
    return count;
}
#endif
#if _debug_is
static DEVICE_ATTR(hm_camera_effect, S_IROTH|S_IWOTH, hm_camera_effect_show,hm_camera_effect_store);
#else
static DEVICE_ATTR(hm_camera_effect, S_IROTH, hm_camera_effect_show,NULL);
#endif
static DEVICE_ATTR(hm_camera_type, S_IROTH, camera_sensor_type,NULL);

static struct attribute *ets_attributes[] =
{

    &dev_attr_hm_camera_effect.attr,
    &dev_attr_hm_camera_type.attr,
    NULL,
};

static struct attribute_group ets_attr_group =
{
    .attrs = ets_attributes,
};

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err;
	struct platform_device * sensor_t8ev5_dev;
	pr_info("t8ev5 %s\n",__func__);

	mt9p111_sensor_info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (!mt9p111_sensor_info) {
		pr_err("t8ev5 : Unable to allocate memory!\n");
		return -ENOMEM;
	}

	err = misc_register(&sensor_device);
	if (err) {
		pr_err("t8ev5 : Unable to register misc device!\n");
		goto EXIT;
	}
	
	mt9p111_sensor_info->pdata = client->dev.platform_data;
	mt9p111_sensor_info->i2c_client = client;

	mt9p111_sensor_info->config.settle_time = SETTLETIME_MS;
	mt9p111_sensor_info->config.focal_length = FOCAL_LENGTH;
	mt9p111_sensor_info->config.fnumber = FNUMBER;
	mt9p111_sensor_info->config.pos_low = POS_LOW;
	mt9p111_sensor_info->config.pos_high = POS_HIGH;
	mt9p111_sensor_info->config.focus_macro = T8EV5_FOCUS_MACRO,
	mt9p111_sensor_info->config.focus_infinity = T8EV5_FOCUS_INFINITY,

	atomic_set(&mt9p111_sensor_info->power_on_loaded, 0);
	atomic_set(&mt9p111_sensor_info->sensor_init_loaded, 0);

	sensor_t8ev5_dev = platform_device_register_simple("t8ev5", -1, NULL, 0);
       if (IS_ERR(sensor_t8ev5_dev))
       {
           printk ("t8ev5 sensor_dev_init: error\n");
           //goto EXIT2;
       }
	err = sysfs_create_group(&sensor_t8ev5_dev->dev.kobj, &ets_attr_group);
      if (err !=0)
       {
         dev_err(&client->dev,"%s:create sysfs group error", __func__);
        //goto EXIT2;
       }
	mutex_init(&mt9p111_sensor_info->als_get_sensor_value_mutex);
	
	if(mt9p111_sensor_info->pdata){
		mt9p111_sensor_info->led_en = mt9p111_sensor_info->pdata->led_en;
		mt9p111_sensor_info->flash_en= mt9p111_sensor_info->pdata->flash_en;
		mt9p111_sensor_info->timer_debounce = mt9p111_sensor_info->pdata->flash_delay*1000;
		if(mt9p111_sensor_info->led_en!=0&&mt9p111_sensor_info->flash_en!=0){
			mt9p111_sensor_info->flash_enable= true;
			INIT_WORK(&mt9p111_sensor_info->work, flash_work_func);
		}else
			mt9p111_sensor_info->flash_enable = false;
			
		if(mt9p111_sensor_info->timer_debounce&&mt9p111_sensor_info->flash_enable)
		setup_timer(&mt9p111_sensor_info->timer,
			    	af_flash_timer, (unsigned long)mt9p111_sensor_info);
	}
	
	i2c_set_clientdata(client, mt9p111_sensor_info);
	pr_info("t8ev5 %s register successfully!\n",__func__);
	return 0;

EXIT:
	kfree(mt9p111_sensor_info);
	return err;
}

static int sensor_remove(struct i2c_client *client)
{
	struct sensor_info *info;

	pr_info("t8ev5 %s\n",__func__);
	info = i2c_get_clientdata(client);
	misc_deregister(&sensor_device);
	mutex_destroy(&info->als_get_sensor_value_mutex);
	kfree(info);
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{ SENSOR_NAME, 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_i2c_driver = {
	.driver = {
		.name = SENSOR_NAME,
		.owner = THIS_MODULE,
	},
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static int __init sensor_init(void)
{
	pr_info("t8ev5 %s\n",__func__);
	return i2c_add_driver(&sensor_i2c_driver);
}

static void __exit sensor_exit(void)
{
	pr_info("t8ev5 %s\n",__func__);
	i2c_del_driver(&sensor_i2c_driver);
}

module_init(sensor_init);
module_exit(sensor_exit);

