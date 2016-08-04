/*
 * drivers/input/touchscreen/gslX680.c
 *
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>

//#include <mach/gpio_data.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pm_runtime.h>

#include <linux/input/mt.h>

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/gpio.h>
//#include <linux/earlysuspend.h>
//#include <mach/system.h>
//#include <mach/hardware.h>
#include "gslX68X.h"
//#define	TPD_PROXIMITY		// add ps define

#ifdef TPD_PROC_DEBUG
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>  
//static struct proc_dir_entry *gsl_config_proc = NULL;
#define GSL_CONFIG_PROC_FILE "gsl_config"
#define CONFIG_LEN 31
static char gsl_read[CONFIG_LEN];
static u8 gsl_data_proc[8] = {0};
static u8 gsl_proc_flag = 0;
#endif

#ifdef GSL_MONITOR
static struct delayed_work gsl_monitor_work;
static struct workqueue_struct *gsl_monitor_workqueue = NULL;
static char int_1st[4] = {0};
static char int_2nd[4] = {0};
static char b0_counter = 0;
static char bc_counter = 0;
static char i2c_lock_flag = 0;
#endif 

#ifdef TPD_PROXIMITY
#include <linux/wakelock.h>
#include <linux/workqueue.h>
static u8 tpd_proximity_flag = 0; //flag whether start alps
static u8 tpd_proximity_detect = 1;//0-->close ; 1--> far away
static struct wake_lock ps_lock;
static u8 gsl_psensor_data[8]={0};
struct gsl_ts *ts_ps;
#endif



#ifdef HAVE_TOUCH_KEY
static u16 key = 0;
static int key_state_flag = 0;
struct key_data {
	u16 key;
	u16 x_min;
	u16 x_max;
	u16 y_min;
	u16 y_max;	
};

const u16 key_array[]={
                                      KEY_BACK,
                                      KEY_HOME,
                                      KEY_MENU,
                                      KEY_SEARCH,
                                     }; 
#define MAX_KEY_NUM     (sizeof(key_array)/sizeof(key_array[0]))

struct key_data gsl_key_data[MAX_KEY_NUM] = {
	{KEY_BACK, 2048, 2048, 2048, 2048},
	{KEY_HOME, 2028, 2068, 2028, 2068},	
	{KEY_MENU, 2048, 2048, 2048, 2048},
	{KEY_SEARCH, 2048, 2048, 2048, 2048},
};
#endif

struct gsl_ts_data {
	u8 x_index;
	u8 y_index;
	u8 z_index;
	u8 id_index;
	u8 touch_index;
	u8 data_reg;
	u8 status_reg;
	u8 data_size;
	u8 touch_bytes;
	u8 update_data;
	u8 touch_meta_data;
	u8 finger_size;
};

static struct gsl_ts_data devices[] = {
	{
		.x_index = 6,
		.y_index = 4,
		.z_index = 5,
		.id_index = 7,
		.data_reg = GSL_DATA_REG,
		.status_reg = GSL_STATUS_REG,
		.update_data = 0x4,
		.touch_bytes = 4,
		.touch_meta_data = 4,
		.finger_size = 70,
	},
};

struct gsl_ts {
	struct i2c_client *client;
	struct input_dev *input;
	struct input_dev *input_ps;
	struct work_struct work;
	struct workqueue_struct *wq;
	struct gsl_ts_data *dd;
	u8 *touch_data;
	u8 device_id;
	int irq;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
};

static struct i2c_client *gsl_client = NULL;
static u32 id_sign[MAX_CONTACTS+1] = {0};
static u8 id_state_flag[MAX_CONTACTS+1] = {0};
static u8 id_state_old_flag[MAX_CONTACTS+1] = {0};
static u16 x_old[MAX_CONTACTS+1] = {0};
static u16 y_old[MAX_CONTACTS+1] = {0};
static u16 x_new = 0;
static u16 y_new = 0;

#define ACTIVE_PEN  //主动笔矫正代码，有主动笔功能，需要打开这个宏


static int gslX680_init(void)
{
	int ret;
	GSL_DEBUG_FUNC();
	ret = gpio_request(IRQ_PORT, "GSL_IRQ");
	if (ret<0) {
		printk( "ret = %d : could not req gpio irq\n", ret);
	}
        gpio_direction_input(IRQ_PORT); 

	ret = gpio_request(RST_PORT, "GSL_RST");
	if (ret) {
		printk( "ret = %d : could not req gpio reset\n", ret);
	}
	
	gpio_direction_input(RST_PORT);
	gpio_direction_output(RST_PORT, 1);
	msleep(20);

	return 0;
}

static int gslX680_shutdown_low(void)
{
	gpio_direction_output(RST_PORT, 1);
	return 0;
}

static int gslX680_shutdown_high(void)
{
	gpio_direction_output(RST_PORT, 0);
	return 0;
}

static inline u16 join_bytes(u8 a, u8 b)
{
	u16 ab = 0;
	ab = ab | a;
	ab = ab << 8 | b;
	return ab;
}
#if 1
/*
static u32 gsl_read_interface(struct i2c_client *client, u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[2];

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = &reg;

	xfer_msg[1].addr = client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags |= I2C_M_RD;
	xfer_msg[1].buf = buf;

	if (reg < 0x80) {
		i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg));
		msleep(5);
	}

	return i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg)) == ARRAY_SIZE(xfer_msg) ? 0 : -EFAULT;
} */
static int gsl_read_interface(struct i2c_client *client, u8 reg, u8 *buf, u32 num)
{

	int err = 0;
	u8 temp = reg;
	//mutex_lock(&gsl_i2c_lock);
	if(temp < 0x80)
	{
		temp = (temp+8)&0x5c;
		i2c_master_send(client,&temp,1);
		err = i2c_master_recv(client,&buf[0],4);

		temp = reg;
		i2c_master_send(client,&temp,1);
		err = i2c_master_recv(client,&buf[0],4);
	}
	i2c_master_send(client,&reg,1);
	err = i2c_master_recv(client,&buf[0],num);
	//mutex_unlock(&gsl_i2c_lock);
	return err;
}
#endif
static u32 gsl_write_interface(struct i2c_client *client, const u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[1];

	buf[0] = reg;

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num + 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = buf;

	return i2c_transfer(client->adapter, xfer_msg, 1) == 1 ? 0 : -EFAULT;
}

static int gsl_ts_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;
	if (datalen > 125)
	{
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}
	
	tmp_buf[0] = addr;
	bytelen++;
	
	if (datalen != 0 && pdata != NULL)
	{
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}
	
	ret = i2c_master_send(client, tmp_buf, bytelen);
	return ret;
}

static int gsl_ts_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126)
	{
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}

	ret = gsl_ts_write(client, addr, NULL, 0);
	if (ret < 0)
	{
		printk("%s set data address fail!\n", __func__);
		return ret;
	}
	
	return i2c_master_recv(client, pdata, datalen);
}


#ifdef ACTIVE_PEN
static void GSL_Adjust_Freq(struct i2c_client *client)
{
	
	u32 cpu_start, cpu_end, ret;
	u8 buf[4];

	buf[3] = 0x01;
	buf[2] = 0xfe;
	buf[1] = 0x02;
	buf[0] = 0x00;
	
	gsl_ts_write(client,0xf0,buf,4);
	buf[3] = 0xff;
	buf[2] = 0xff;
	buf[1] = 0xff;
	buf[0] = 0xff;	
	gsl_ts_write(client, 0x04, buf, 4);
	
	buf[3] = 0;
	buf[2] = 0;
	buf[1] = 0;
	buf[0] = 0x09;	
	gsl_ts_write(client, 0x08, buf, 4);
	
	msleep(200);
	buf[3] = 0x01;
	buf[2] = 0xfe;
	buf[1] = 0x02;
	buf[0] = 0x00;
	gsl_ts_write(client, 0xf0, buf, 4);
	
	gsl_ts_read(client, 0, buf, 4);
	gsl_ts_read(client, 0, buf, 4);
	
	
	cpu_start = (buf[3]<<24) + (buf[2]<<16) + (buf[1]<<8) + buf[0];

	printk("#########cpu_start = 0x%x  #########\n", cpu_start);

	
	msleep(1000);
	buf[3] = 0x01;
	buf[2] = 0xfe;
	buf[1] = 0x02;
	buf[0] = 0x00;
	gsl_ts_write(client, 0xf0, buf, 4);
	
	gsl_ts_read(client, 0, buf, 4);
	gsl_ts_read(client, 0, buf, 4);
	
	cpu_end = (buf[3]<<24) + (buf[2]<<16) + (buf[1]<<8) + buf[0];
	printk("#########cpu_end = 0x%x  #########\n", cpu_end);

	ret = (cpu_start - cpu_end)/226;
	
	printk("#########ret = 0x%x  #########\n", ret);
	
	
	buf[3] = 0x00;
	buf[2] = 0x00;
	buf[1] = 0x00;
	buf[0] = 0x03;
	gsl_ts_write(client, 0xf0, buf, 4);
	
	buf[3] = (u8)((ret>>24)&0xff);
	buf[2] = (u8)((ret>>16)&0xff);
	buf[1] = (u8)((ret>>8)&0xff);
	buf[0] = (u8)(ret&0xff);
	gsl_ts_write(client, 0x7c, buf, 4);

	return 1;
}
#endif

static __inline__ void fw2buf(u8 *buf, const u32 *fw)
{
	u32 *u32_buf = (int *)buf;
	*u32_buf = *fw;
}

static void gsl_load_fw(struct i2c_client *client)
{
	u8 buf[DMA_TRANS_LEN*4 + 1] = {0};
	u8 send_flag = 1;
	u8 *cur = buf + 1;
	u32 source_line = 0;
	u32 source_len;
	struct fw_data *ptr_fw;

	printk("=============gsl_load_fw start==============\n");

	ptr_fw = GSLX680_FW;
	source_len =fw_size;  // ARRAY_SIZE(GSLX680_FW);  lzk

	for (source_line = 0; source_line < source_len; source_line++) 
	{
		/* init page trans, set the page val */
		if (GSL_PAGE_REG == ptr_fw[source_line].offset)
		{
			fw2buf(cur, &ptr_fw[source_line].val);
			gsl_write_interface(client, GSL_PAGE_REG, buf, 4);
			send_flag = 1;
		}
		else 
		{
			if (1 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20))
	    			buf[0] = (u8)ptr_fw[source_line].offset;

			fw2buf(cur, &ptr_fw[source_line].val);
			cur += 4;

			if (0 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20)) 
			{
	    			gsl_write_interface(client, buf[0], buf, cur - buf - 1);
	    			cur = buf + 1;
			}

			send_flag++;
		}
	}

	printk("=============gsl_load_fw end==============\n");

}


static int test_i2c(struct i2c_client *client)
{
	u8 read_buf = 0;
	u8 write_buf = 0x12;
	int ret, rc = 1;
	
	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if  (ret  < 0)  
    		rc --;
	else
		printk("I read reg 0xf0 is %x\n", read_buf);
	
	msleep(2);
	ret = gsl_ts_write(client, 0xf0, &write_buf, sizeof(write_buf));
	if(ret  >=  0 )
		printk("I write reg 0xf0 0x12\n");
	
	msleep(2);
	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if(ret <  0 )
		rc --;
	else
		printk("I read reg 0xf0 is 0x%x\n", read_buf);

	return rc;
}


static void startup_chip(struct i2c_client *client)
{
	u8 tmp = 0x00;
	
#ifdef GSL_NOID_VERSION
	gsl_DataInit(gsl_config_data_id);
#endif
	gsl_ts_write(client, 0xe0, &tmp, 1);
	msleep(10);	
}

static void reset_chip(struct i2c_client *client)
{
	u8 tmp = 0x88;
	u8 buf[4] = {0x00};
	GSL_DEBUG_FUNC();
	gsl_ts_write(client, 0xe0, &tmp, sizeof(tmp));
	msleep(20);
	tmp = 0x04;
	gsl_ts_write(client, 0xe4, &tmp, sizeof(tmp));
	msleep(10);
	gsl_ts_write(client, 0xbc, buf, sizeof(buf));
	msleep(10);
}

static void clr_reg(struct i2c_client *client)
{
	u8 write_buf[4]	= {0};

	write_buf[0] = 0x88;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(20);
	write_buf[0] = 0x03;
	gsl_ts_write(client, 0x80, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x04;
	gsl_ts_write(client, 0xe4, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x00;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(20);
}

static void init_chip(struct i2c_client *client)
{
	int rc;
	GSL_DEBUG_FUNC();
	gslX680_shutdown_low();	
	msleep(20); 	
	gslX680_shutdown_high();	
	msleep(20); 		
	rc = test_i2c(client);
	if(rc < 0)
	{
		printk("------gslX680 test_i2c error------\n");	
		return;
	}
	clr_reg(client);
	reset_chip(client);
	gsl_load_fw(client);			
	startup_chip(client);	
#ifdef ACTIVE_PEN
	GSL_Adjust_Freq(client);
#endif
	reset_chip(client);	
	startup_chip(client);	
}

static void check_mem_data(struct i2c_client *client)
{
	u8 read_buf[4]  = {0};
	
	msleep(30);
	gsl_ts_read(client,0xb0, read_buf, sizeof(read_buf));
	
	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
	{
		printk("#########check mem read 0xb0 = %x %x %x %x #########\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip(client);
	}
}

#ifdef TPD_PROC_DEBUG
#ifdef GSL_APPLICATION
static int gsl_read_MorePage(struct i2c_client *client,u32 addr,u8 *buf,u32 num)
{
	int i;
	u8 tmp_buf[4] = {0};
	u8 tmp_addr;
	for(i=0;i<num/8;i++){
		tmp_buf[0]=(char)((addr+i*8)/0x80);
		tmp_buf[1]=(char)(((addr+i*8)/0x80)>>8);
		tmp_buf[2]=(char)(((addr+i*8)/0x80)>>16);
		tmp_buf[3]=(char)(((addr+i*8)/0x80)>>24);
		gsl_ts_write(client,0xf0,tmp_buf,4);
		tmp_addr = (char)((addr+i*8)%0x80);
		gsl_read_interface(client,tmp_addr,(buf+i*8),8);
	}
	if(i*8<num){
		tmp_buf[0]=(char)((addr+i*8)/0x80);
		tmp_buf[1]=(char)(((addr+i*8)/0x80)>>8);
		tmp_buf[2]=(char)(((addr+i*8)/0x80)>>16);
		tmp_buf[3]=(char)(((addr+i*8)/0x80)>>24);
		gsl_ts_write(client,0xf0,tmp_buf,4);
		tmp_addr = (char)((addr+i*8)%0x80);
		gsl_read_interface(client,tmp_addr,(buf+i*8),4);
	}
}
#endif
static int char_to_int(char ch)
{
	if(ch>='0' && ch<='9')
		return (ch-'0');
	else
		return (ch-'a'+10);
}

//static int gsl_config_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
static int gsl_config_read_proc(struct seq_file *m,void *v)
{
	unsigned char temp_data[5] = {0};
	unsigned int tmp=0;
	if('v'==gsl_read[0]&&'s'==gsl_read[1])
	{
#ifdef GSL_NOID_VERSION 
		tmp=gsl_version_id();
#else 
		tmp=0x20121215;
#endif
		seq_printf(m,"version:%x\n",tmp);
	}
	else if('r'==gsl_read[0]&&'e'==gsl_read[1])
	{
		if('i'==gsl_read[3])
		{
#ifdef GSL_NOID_VERSION 
		/*	tmp=(gsl_data_proc[5]<<8) | gsl_data_proc[4];
			seq_printf(m,"gsl_config_data_id[%d] = ",tmp);
			if(tmp>=0&&tmp<gsl_cfg_table[gsl_cfg_index].data_size)
				seq_printf(m,"%d\n",gsl_cfg_table[gsl_cfg_index].data_id[tmp]); */

			tmp=(gsl_data_proc[5]<<8) | gsl_data_proc[4];
			//ptr +=sprintf(ptr,"gsl_config_data_id[%d] = ",tmp);
			//if(tmp>=0&&tmp<512)
			//	ptr +=sprintf(ptr,"%d\n",gsl_config_data_id[tmp]); 
#endif
		}
		else 
		{
			gsl_ts_write(gsl_client,0xf0,&gsl_data_proc[4],4);
			gsl_read_interface(gsl_client,gsl_data_proc[0],temp_data,4);
			seq_printf(m,"offset : {0x%02x,0x",gsl_data_proc[0]);
			seq_printf(m,"%02x",temp_data[3]);
			seq_printf(m,"%02x",temp_data[2]);
			seq_printf(m,"%02x",temp_data[1]);
			seq_printf(m,"%02x};\n",temp_data[0]);
		}
	}
#ifdef GSL_APPLICATION
	else if('a'==gsl_read[0]&&'p'==gsl_read[1]){
		char *buf;
		int temp1;
		tmp = (unsigned int)(((gsl_data_proc[2]<<8)|gsl_data_proc[1])&0xffff);
		buf=kzalloc(tmp,GFP_KERNEL);
		if(buf==NULL)
			return -1;
		if(3==gsl_data_proc[0]){
			gsl_read_interface(gsl_client,gsl_data_proc[3],buf,tmp);
			if(tmp < m->size){
				memcpy(m->buf,buf,tmp);
			}
		}else if(4==gsl_data_proc[0]){
			temp1=((gsl_data_proc[6]<<24)|(gsl_data_proc[5]<<16)|
				(gsl_data_proc[4]<<8)|gsl_data_proc[3]);
			gsl_read_MorePage(gsl_client,temp1,buf,tmp);
			if(tmp < m->size){
				memcpy(m->buf,buf,tmp);
			}
		}
		kfree(buf);
	}
#endif
	return 0;
}
static int gsl_config_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
	u8 buf[8] = {0};
	unsigned char temp_buf[CONFIG_LEN];
	unsigned char *path_buf;
	int tmp = 0;
	int tmp1 = 0;
	print_info("[tp-gsl][%s] \n",__func__);
	if(count > 512)
	{
		print_info("size not match [%d:%ld]\n", CONFIG_LEN, count);
        	return -EFAULT;
	}
	path_buf=kzalloc(count,GFP_KERNEL);
	if(!path_buf)
	{
		printk("alloc path_buf memory error \n");
		return -1;
	}	
	if(copy_from_user(path_buf, buffer, count))
	{
		print_info("copy from user fail\n");
		goto exit_write_proc_out;
	}
	memcpy(temp_buf,path_buf,(count<CONFIG_LEN?count:CONFIG_LEN));
	print_info("[tp-gsl][%s][%s]\n",__func__,temp_buf);
#ifdef GSL_APPLICATION
	if('a'!=temp_buf[0]||'p'!=temp_buf[1]){
#endif
	buf[3]=char_to_int(temp_buf[14])<<4 | char_to_int(temp_buf[15]);	
	buf[2]=char_to_int(temp_buf[16])<<4 | char_to_int(temp_buf[17]);
	buf[1]=char_to_int(temp_buf[18])<<4 | char_to_int(temp_buf[19]);
	buf[0]=char_to_int(temp_buf[20])<<4 | char_to_int(temp_buf[21]);
	
	buf[7]=char_to_int(temp_buf[5])<<4 | char_to_int(temp_buf[6]);
	buf[6]=char_to_int(temp_buf[7])<<4 | char_to_int(temp_buf[8]);
	buf[5]=char_to_int(temp_buf[9])<<4 | char_to_int(temp_buf[10]);
	buf[4]=char_to_int(temp_buf[11])<<4 | char_to_int(temp_buf[12]);
#ifdef GSL_APPLICATION
	}
#endif
	if('v'==temp_buf[0]&& 's'==temp_buf[1])//version //vs
	{
		memcpy(gsl_read,temp_buf,4);
		printk("gsl version\n");
	}
	else if('s'==temp_buf[0]&& 't'==temp_buf[1])//start //st
	{
	#ifdef GSL_MONITOR
		cancel_delayed_work_sync(&gsl_monitor_work);
	#endif
		gsl_proc_flag = 1;
		reset_chip(gsl_client);  //gsl_reset_core
	}
	else if('e'==temp_buf[0]&&'n'==temp_buf[1])//end //en
	{
		msleep(20);
		reset_chip(gsl_client);  //gsl_reset_core
		startup_chip(gsl_client); // gsl_start_core
		gsl_proc_flag = 0;
	}
	else if('r'==temp_buf[0]&&'e'==temp_buf[1])//read buf //
	{
		memcpy(gsl_read,temp_buf,4);
		memcpy(gsl_data_proc,buf,8);
	}
	else if('w'==temp_buf[0]&&'r'==temp_buf[1])//write buf
	{
		gsl_ts_write(gsl_client,buf[4],buf,4);
	}
	
#ifdef GSL_NOID_VERSION
	else if('i'==temp_buf[0]&&'d'==temp_buf[1])//write id config //
	{
		tmp1=(buf[7]<<24)|(buf[6]<<16)|(buf[5]<<8)|buf[4];
		tmp=(buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
		if(tmp1>=0 && tmp1<512)
		{
			gsl_config_data_id[tmp1] = tmp;
		}
	}
#endif
#ifdef GSL_APPLICATION
	else if('a'==temp_buf[0]&&'p'==temp_buf[1]){
		if(1==path_buf[3]){
			tmp=((path_buf[5]<<8)|path_buf[4]);
			gsl_ts_write(gsl_client,path_buf[6],&path_buf[10],tmp);
		}else if(2==path_buf[3]){
			tmp = ((path_buf[5]<<8)|path_buf[4]);
			tmp1=((path_buf[9]<<24)|(path_buf[8]<<16)|(path_buf[7]<<8)
				|path_buf[6]);
			buf[0]=(char)((tmp1/0x80)&0xff);
			buf[1]=(char)(((tmp1/0x80)>>8)&0xff);
			buf[2]=(char)(((tmp1/0x80)>>16)&0xff);
			buf[3]=(char)(((tmp1/0x80)>>24)&0xff);
			buf[4]=(char)(tmp1%0x80);
			gsl_ts_write(gsl_client,0xf0,buf,4);
			gsl_ts_write(gsl_client,buf[4],&path_buf[10],tmp);
		}else if(3==path_buf[3]||4==path_buf[3]){
			memcpy(gsl_read,temp_buf,4);
			memcpy(gsl_data_proc,&path_buf[3],7);
		}
	}
#endif
exit_write_proc_out:
	kfree(path_buf);
	return count;
}
static int gsl_server_list_open(struct inode *inode,struct file *file)
{
	return single_open(file,gsl_config_read_proc,NULL);
}
static const struct file_operations gsl_seq_fops = {
	.open = gsl_server_list_open,
	.read = seq_read,
	.release = single_release,
	.write = gsl_config_write_proc,
	.owner = THIS_MODULE,
};
#endif

static void record_point(u16 x, u16 y , u8 id)
{
	u16 x_err =0;
	u16 y_err =0;

	id_sign[id]=id_sign[id]+1;
	
	if(id_sign[id]==1){
		x_old[id]=x;
		y_old[id]=y;
	}

	x = (x_old[id] + x)/2;
	y = (y_old[id] + y)/2;
		
	if(x>x_old[id]){
		x_err=x -x_old[id];
	}
	else{
		x_err=x_old[id]-x;
	}

	if(y>y_old[id]){
		y_err=y -y_old[id];
	}
	else{
		y_err=y_old[id]-y;
	}

	if( (x_err > 3 && y_err > 1) || (x_err > 1 && y_err > 3) ){
		x_new = x;     x_old[id] = x;
		y_new = y;     y_old[id] = y;
	}
	else{
		if(x_err > 3){
			x_new = x;     x_old[id] = x;
		}
		else
			x_new = x_old[id];
		if(y_err> 3){
			y_new = y;     y_old[id] = y;
		}
		else
			y_new = y_old[id];
	}

	if(id_sign[id]==1){
		x_new= x_old[id];
		y_new= y_old[id];
	}
	
}

#ifdef HAVE_TOUCH_KEY
static void report_key(struct gsl_ts *ts, u16 x, u16 y)
{
	u16 i = 0;

	for(i = 0; i < MAX_KEY_NUM; i++) 
	{
		if((gsl_key_data[i].x_min < x) && (x < gsl_key_data[i].x_max)&&(gsl_key_data[i].y_min < y) && (y < gsl_key_data[i].y_max))
		{
			key = gsl_key_data[i].key;	
			input_report_key(ts->input, key, 1);
			input_sync(ts->input); 		
			key_state_flag = 1;
			break;
		}
	}
}
#endif

static void report_data(struct gsl_ts *ts, u16 x, u16 y, u8 pressure, u8 id)
{

	print_info("#####id=%d,x=%d,y=%d######\n",id,x,y);

	if(x > SCREEN_MAX_X || y > SCREEN_MAX_Y)
	{
	#ifdef HAVE_TOUCH_KEY
		report_key(ts,x,y);
	#endif
		return;
	}
	
#ifdef REPORT_DATA_ANDROID_4_0
	input_mt_slot(ts->input, id);		
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pressure);
	input_report_abs(ts->input, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);	
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
#else
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pressure);
	input_report_abs(ts->input, ABS_MT_POSITION_X,x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
	input_mt_sync(ts->input);
#endif
}



#ifdef TPD_PROXIMITY
static int tpd_get_ps_value(void)
{
	return tpd_proximity_detect;//send to OS to controll backlight on/off
}
#endif

static void gslX680_ts_worker(struct work_struct *work)
{
	int rc, i;
	u8 id, touches;
	u16 x, y;

	struct gsl_ts *ts = container_of(work, struct gsl_ts,work);

	  GSL_DEBUG_FUNC();				 

#ifdef GSL_MONITOR
	if(i2c_lock_flag != 0)
		goto i2c_lock_schedule;
	else
		i2c_lock_flag = 1;
#endif

#ifdef GSL_NOID_VERSION
	u32 tmp1;
	u8 buf[4] = {0};
	struct gsl_touch_info cinfo = {0};
#endif
	
#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1)
		goto schedule;
#endif


#ifdef TPD_PROXIMITY       //add ps

	int err,ps_value;
	char buf_ps[4] = {0};

	//hwm_sensor_data sensor_data;
	/*added by bernard*/
	if (tpd_proximity_flag == 1) {
		//	gsl_read_interface(ddata->client,0xac,buf_ps,4);
		i2c_smbus_read_i2c_block_data(ts->client,0xac,4,buf_ps);
		printk("--wc--gslX680   buf_ps[0] = %d buf_ps[1] = %d,  buf_ps[2] = %d  buf[3] = %d \n",buf_ps[0],buf_ps[1],buf_ps[2],buf_ps[3]);

		if (buf_ps[0] == 0 && buf_ps[1] == 0 && buf_ps[2] == 0 && buf_ps[3] == 0) {
			if(tpd_proximity_detect ==0) {
				//gsl_ts_resume(&ts->client->dev); 
				//reset_chip(ts->client);
				//startup_chip(ts->client);
			}

			tpd_proximity_detect = 1;
			ps_value=tpd_get_ps_value();
			input_report_abs(ts->input_ps, ABS_DISTANCE, ps_value);  
			input_sync(ts->input_ps);                             
		} else {
			tpd_proximity_detect = 0;

			ps_value=tpd_get_ps_value();
			if(ps_value==0)
				ps_value=0x26;
			input_report_abs(ts->input_ps, ABS_DISTANCE, ps_value);  
			input_sync(ts->input_ps);

			enable_irq(ts->irq);
			return;

			/*	while(!(buf_ps[0] == 0 && buf_ps[1] == 0 && buf_ps[2] == 0 && buf_ps[3]==0))
			{
			i2c_smbus_read_i2c_block_data(ts->client,0xac,4,buf_ps);
			msleep(50);
			}
			gsl_ts_resume(&ts->client->dev);
			tpd_proximity_detect = 1;
			*/              
		}
	}
#endif

	rc = gsl_ts_read(ts->client, 0x80, ts->touch_data, ts->dd->data_size);
	if (rc < 0) 
	{
		dev_err(&ts->client->dev, "read failed\n");
		goto schedule;
	}
		
	touches = ts->touch_data[ts->dd->touch_index];
	print_info("-----touches: %d -----\n", touches);		
#ifdef GSL_NOID_VERSION
	cinfo.finger_num = touches;
	print_info("tp-gsl  finger_num = %d\n",cinfo.finger_num);
	for(i = 0; i < (touches < MAX_CONTACTS ? touches : MAX_CONTACTS); i ++)
	{
		cinfo.x[i] = join_bytes( ( ts->touch_data[ts->dd->x_index  + 4 * i + 1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
		cinfo.y[i] = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i ]);
		print_info("tp-gsl  x = %d y = %d \n",cinfo.x[i],cinfo.y[i]);
	}
	cinfo.finger_num=(ts->touch_data[3]<<24)|(ts->touch_data[2]<<16)
		|(ts->touch_data[1]<<8)|(ts->touch_data[0]);
	gsl_alg_id_main(&cinfo);
	tmp1=gsl_mask_tiaoping();
	print_info("[tp-gsl] tmp1=%x\n",tmp1);
	if(tmp1>0&&tmp1<0xffffffff)
	{
		buf[0]=0xa;buf[1]=0;buf[2]=0;buf[3]=0;
		gsl_ts_write(ts->client,0xf0,buf,4);
		buf[0]=(u8)(tmp1 & 0xff);
		buf[1]=(u8)((tmp1>>8) & 0xff);
		buf[2]=(u8)((tmp1>>16) & 0xff);
		buf[3]=(u8)((tmp1>>24) & 0xff);
		print_info("tmp1=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n",
			tmp1,buf[0],buf[1],buf[2],buf[3]);
		gsl_ts_write(ts->client,0x8,buf,4);
	}
	touches = cinfo.finger_num;
#endif
	
	for(i = 1; i <= MAX_CONTACTS; i ++)
	{
		if(touches == 0)
			id_sign[i] = 0;	
		id_state_flag[i] = 0;
	}
	for(i= 0;i < (touches > MAX_FINGERS ? MAX_FINGERS : touches);i ++)
	{
	#ifdef GSL_NOID_VERSION
		id = cinfo.id[i];
		x =  cinfo.x[i];
		y =  cinfo.y[i];	
	#else
		x = join_bytes( ( ts->touch_data[ts->dd->x_index  + 4 * i + 1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
		y = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i ]);
		id = ts->touch_data[ts->dd->id_index + 4 * i] >> 4;
	#endif

		if(1 <=id && id <= MAX_CONTACTS)
		{
			record_point(x, y , id);
			report_data(ts, x_new, y_new, 10, id);		
			id_state_flag[id] = 1;
		}
	}
	for(i = 1; i <= MAX_CONTACTS; i ++)
	{	
		if( (0 == touches) || ((0 != id_state_old_flag[i]) && (0 == id_state_flag[i])) )
		{
		#ifdef REPORT_DATA_ANDROID_4_0
			input_mt_slot(ts->input, i);
			input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(ts->input, MT_TOOL_FINGER, false);
		#endif
			id_sign[i]=0;
		}
		id_state_old_flag[i] = id_state_flag[i];
	}
#ifndef REPORT_DATA_ANDROID_4_0
	if(0 == touches)
	{	
		input_mt_sync(ts->input);
	#ifdef HAVE_TOUCH_KEY
		if(key_state_flag)
		{
        	input_report_key(ts->input, key, 0);
			input_sync(ts->input);
			key_state_flag = 0;
		}
	#endif			
	}
#endif
#ifdef HAVE_TOUCH_KEY
		if(key_state_flag)
		{
        	input_report_key(ts->input, key, 0);
			input_sync(ts->input);
			key_state_flag = 0;
		}
#endif
	input_sync(ts->input);
	
schedule:
#ifdef GSL_MONITOR
	i2c_lock_flag = 0;
i2c_lock_schedule:
#endif
	enable_irq(ts->irq);
		
}

#ifdef GSL_MONITOR
static void gsl_monitor_worker(void)
{
	char write_buf[4] = {0};
	char read_buf[4]  = {0};
	char init_chip_flag = 0;
	
	print_info("----------------gsl_monitor_worker-----------------\n");	

	if(i2c_lock_flag != 0)
		goto queue_monitor_work;
	else
		i2c_lock_flag = 1;
	
	gsl_ts_read(gsl_client, 0xb0, read_buf, 4);
	if(read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
		b0_counter ++;
	else
		b0_counter = 0;

	if(b0_counter > 1)
	{
		printk("======read 0xb0: %x %x %x %x ======\n",read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip_flag = 1;
		b0_counter = 0;
	}
	
	gsl_ts_read(gsl_client, 0xb4, read_buf, 4);	
	int_2nd[3] = int_1st[3];
	int_2nd[2] = int_1st[2];
	int_2nd[1] = int_1st[1];
	int_2nd[0] = int_1st[0];
	int_1st[3] = read_buf[3];
	int_1st[2] = read_buf[2];
	int_1st[1] = read_buf[1];
	int_1st[0] = read_buf[0];

	if(int_1st[3] == int_2nd[3] && int_1st[2] == int_2nd[2] &&int_1st[1] == int_2nd[1] && int_1st[0] == int_2nd[0]) 
	{
		printk("======int_1st: %x %x %x %x , int_2nd: %x %x %x %x ======\n",int_1st[3], int_1st[2], int_1st[1], int_1st[0], int_2nd[3], int_2nd[2],int_2nd[1],int_2nd[0]);
		init_chip_flag = 1;
	}

	gsl_ts_read(gsl_client, 0xbc, read_buf, 4);
	if(read_buf[3] != 0 || read_buf[2] != 0 || read_buf[1] != 0 || read_buf[0] != 0)
		bc_counter ++;
	else
		bc_counter = 0;

	if(bc_counter > 1)
	{
		printk("======read 0xbc: %x %x %x %x ======\n",read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip_flag = 1;
		bc_counter = 0;
	}

	if(init_chip_flag)
		init_chip(gsl_client);
	
	i2c_lock_flag = 0;
	
queue_monitor_work:	
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 100);
}
#endif

static irqreturn_t gsl_ts_irq(int irq, void *dev_id)
{	
	struct gsl_ts *ts = dev_id;

	  GSL_DEBUG_FUNC();			 

	disable_irq_nosync(ts->irq);

	if (!work_pending(&ts->work)) 
	{
		queue_work(ts->wq, &ts->work);
	}
	
	return IRQ_HANDLED;

}

static int gslX680_ts_init(struct i2c_client *client, struct gsl_ts *ts)
{
	struct input_dev *input_device;
	struct input_dev *input_device_ps;
	
	int rc = 0;
#ifdef HAVE_TOUCH_KEY
	int i;
#endif
	
	  GSL_DEBUG_FUNC();

	ts->dd = &devices[ts->device_id];

	if (ts->device_id == 0) {
		ts->dd->data_size = MAX_FINGERS * ts->dd->touch_bytes + ts->dd->touch_meta_data;
		ts->dd->touch_index = 0;
	}

	ts->touch_data = kzalloc(ts->dd->data_size, GFP_KERNEL);
	if (!ts->touch_data) {
		pr_err("%s: Unable to allocate memory\n", __func__);
		return -ENOMEM;
	}

	input_device = input_allocate_device();
	if (!input_device) {
		rc = -ENOMEM;
		goto error_alloc_dev;
	}

#ifdef TPD_PROXIMITY
        input_device_ps = input_allocate_device();  
	if (!input_device_ps) {
		rc = -ENOMEM;
		goto error_alloc_dev;
	}
#endif

	ts->input = input_device;
	input_device->name = GSLX680_I2C_NAME;
	input_device->id.bustype = BUS_I2C;
	input_device->dev.parent = &client->dev;
	input_set_drvdata(input_device, ts);

#ifdef REPORT_DATA_ANDROID_4_0
	__set_bit(EV_ABS, input_device->evbit);
	__set_bit(EV_KEY, input_device->evbit);
	__set_bit(EV_REP, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	input_mt_init_slots(input_device, (MAX_CONTACTS+1), 0);
#else
	input_set_abs_params(input_device,ABS_MT_TRACKING_ID, 0, (MAX_CONTACTS+1), 0, 0);
	set_bit(EV_ABS, input_device->evbit);
	set_bit(EV_KEY, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	input_device->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif

#ifdef HAVE_TOUCH_KEY
	//input_device->evbit[0] = BIT_MASK(EV_KEY);
	//input_device->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	for (i = 0; i < MAX_KEY_NUM; i++)
		set_bit(key_array[i], input_device->keybit);
#endif

	set_bit(ABS_MT_POSITION_X, input_device->absbit);
	set_bit(ABS_MT_POSITION_Y, input_device->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, input_device->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_device->absbit);

	input_set_abs_params(input_device,ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_device,ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_device,ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_device,ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	client->irq = gpio_to_irq(IRQ_PORT);
	ts->irq = client->irq;

	ts->wq = create_singlethread_workqueue("kworkqueue_ts");
	if (!ts->wq) {
		dev_err(&client->dev, "Could not create workqueue\n");
		goto error_wq_create;
	}
	flush_workqueue(ts->wq);	

	INIT_WORK(&ts->work, gslX680_ts_worker);

	rc = input_register_device(input_device);
	if (rc)
		goto error_unreg_device;

#ifdef TPD_PROXIMITY
        ts->input_ps = input_device_ps;
	input_device_ps->name = "gsl680_ps";
	input_device_ps->id.bustype = BUS_I2C;
	input_device_ps->dev.parent = &client->dev;
	input_set_drvdata(input_device_ps, ts);

        set_bit(EV_ABS, input_device_ps->evbit);
	input_set_capability(input_device_ps, EV_ABS, ABS_DISTANCE);
	//input_set_capability(input_device_ps, EV_ABS, ABS_MISC);
	input_set_abs_params(input_device_ps, ABS_DISTANCE, 0, 255, 0, 0);
	//input_set_abs_params(input_device_ps, ABS_MISC, 0, ALS_MAX, 0, 0);
	rc = input_register_device(input_device_ps);
	if (rc)
		goto error_unreg_device;
#endif

	return 0;

error_unreg_device:
	destroy_workqueue(ts->wq);
error_wq_create:
	input_free_device(input_device);
#ifdef TPD_PROXIMITY
        input_free_device(input_device_ps);
#endif
error_alloc_dev:
	kfree(ts->touch_data);
	return rc;
}

#ifdef TPD_PROXIMITY
static void gsl_gain_psensor_data(struct i2c_client *client)
{
	int tmp = 0;
	u8 buf[4]={0};
	/**************************/
	buf[0]=0x3;
	i2c_smbus_write_i2c_block_data(client,0xf0,4,buf);
	tmp = i2c_smbus_read_i2c_block_data(client,0x0,4,&gsl_psensor_data[0]);
	if(tmp <= 0)
	{
		 i2c_smbus_read_i2c_block_data(client,0x0,4,&gsl_psensor_data[0]);
	}
	/**************************/

	buf[0]=0x4;
	i2c_smbus_write_i2c_block_data(client,0xf0,4,buf);
	tmp = i2c_smbus_read_i2c_block_data(client,0x0,4,&gsl_psensor_data[4]);
	if(tmp <= 0)
	{
		i2c_smbus_read_i2c_block_data(client,0x0,4,&gsl_psensor_data[4]);
	}

	
}

static int tpd_enable_ps(int enable)
{

	u8 buf[4];
	if (enable) {
		wake_lock(&ps_lock);
		buf[3] = 0x00;
		buf[2] = 0x00;
		buf[1] = 0x00;
		buf[0] = 0x3;
		i2c_smbus_write_i2c_block_data(gsl_client, 0xf0, 4, buf);
	//	gsl_write_interface(ddata->client, 0xf0, buf, 4);
		buf[3] = 0x5a;
		buf[2] = 0x5a;
		buf[1] = 0x5a;
		buf[0] = 0x5a;
		i2c_smbus_write_i2c_block_data(gsl_client, 0, 4, buf);
	//	gsl_write_interface(ddata->client, 0, buf, 4);

		buf[3] = 0x00;
		buf[2] = 0x00;
		buf[1] = 0x00;
		buf[0] = 0x4;
		i2c_smbus_write_i2c_block_data(gsl_client, 0xf0, 4, buf);
	//	gsl_write_interface(ddata->client, 0xf0, buf, 4);
		buf[3] = 0x0;
		buf[2] = 0x0;
		buf[1] = 0x0;
		buf[0] = 0x2;
		i2c_smbus_write_i2c_block_data(gsl_client, 0, 4, buf);
	//	gsl_write_interface(ddata->client, 0, buf, 4);
		
		tpd_proximity_flag = 1;
		//add alps of function
		printk("tpd-ps function is on\n");
	}
	else 
	{
		tpd_proximity_flag = 0;
		wake_unlock(&ps_lock);
		buf[3] = 0x00;
		buf[2] = 0x00;
		buf[1] = 0x00;
		buf[0] = 0x3;
		i2c_smbus_write_i2c_block_data(gsl_client, 0xf0, 4, buf);		
//		gsl_write_interface(ddata->client, 0xf0, buf, 4);
		buf[3] = gsl_psensor_data[3];
		buf[2] = gsl_psensor_data[2];
		buf[1] = gsl_psensor_data[1];
		buf[0] = gsl_psensor_data[0];

		i2c_smbus_write_i2c_block_data(gsl_client, 0, 4, buf);
		
//		gsl_write_interface(ddata->client, 0, buf, 4);

		buf[3] = 0x00;
		buf[2] = 0x00;
		buf[1] = 0x00;
		buf[0] = 0x4;

		i2c_smbus_write_i2c_block_data(gsl_client, 0xf0, 4, buf);				
//		gsl_write_interface(ddata->client, 0xf0, buf, 4);
		buf[3] = gsl_psensor_data[7];
		buf[2] = gsl_psensor_data[6];
		buf[1] = gsl_psensor_data[5];
		buf[0] = gsl_psensor_data[4];
		i2c_smbus_write_i2c_block_data(gsl_client, 0x0, 4, buf);	
//		gsl_write_interface(ddata->client, 0, buf, 4);
		printk("tpd-ps function is off\n");
		//gsl_ts_resume(&gsl_client->dev);    

	}
	return 0;
}

static void CM36283_work_func(struct work_struct *work)
{
	/*	int err,ps_value;
                char buf_ps[4] = {0};
		//hwm_sensor_data sensor_data;
		if (tpd_proximity_flag == 1)
		{
		//	gsl_read_interface(ddata->client,0xac,buf_ps,4);

	                i2c_smbus_read_i2c_block_data(ts_ps->client,0xac,4,buf_ps);


                        print_info("gslX680   buf_ps[0] = %d buf_ps[1] = %d,  buf_ps[2] = %d  buf[3] = %d \n",buf_ps[0],buf_ps[1],buf_ps[2],buf_ps[3]);
			
			if (buf_ps[0] == 1 && buf_ps[1] == 0 && buf_ps[2] == 0 && buf_ps[3] == 0)
			{
				tpd_proximity_detect = 0;
				//sensor_data.values[0] = 0;
			}
			else
			{
				tpd_proximity_detect = 1;
				//sensor_data.values[0] = 1;
			}
			//get raw data
			print_info("gslX680    ps change   tpd_proximity_detect = %d  \n",tpd_proximity_detect);
			//map and store data to hwm_sensor_data
			//sensor_data.values[0] = tpd_get_ps_value();
                        ps_value=tpd_get_ps_value();
		       // if (ps_value > 0) {  // -1
                             printk("---lzk--ceshi--gsl680---ps_value=%d \n",ps_value);
			     input_report_abs(ts_ps->input_ps, ABS_DISTANCE, ps_value);  //lzk
			     input_sync(ts_ps->input_ps);
		       // }
                       schedule_delayed_work(&ts_ps->ps_work, 200);                        
		} */
}

static ssize_t gsl680_ps_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int data;
	data = tpd_proximity_flag;

	return sprintf(buf, "%d\n", data);
}

static ssize_t gsl680_ps_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{

	unsigned long data;
	int error;
	error = strict_strtoul(buf, 10, &data);
	if(error)
		return error;
	pr_debug("%s set ps = %ld\n", __func__, data);

	if(data)
	{
		if(tpd_proximity_flag==0)
		{
            if((tpd_enable_ps(1) != 0)){
            printk("enable ps fail: \n");
            return -1;
            }
		}
	}
	else
	{
	    if(tpd_proximity_flag==1)
		    {
                if((tpd_enable_ps(0) != 0)){
                printk("disable ps fail: \n");
                return -1;
                }
	        }   
	}
	return count;
}

static DEVICE_ATTR(enable_ps, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		gsl680_ps_enable_show, gsl680_ps_enable_store);

static struct attribute *gsl680_attributes[] = {
	&dev_attr_enable_ps.attr,
	NULL
};
static struct attribute_group gsl680_attribute_group = {
	.attrs = gsl680_attributes
};

#endif


static int gsl_ts_suspend(struct device *dev)
{
	struct gsl_ts *ts = dev_get_drvdata(dev);

  	  GSL_DEBUG_FUNC();
#ifdef GSL_MONITOR
	printk( "gsl_ts_suspend () : cancel gsl_monitor_work\n");
	cancel_delayed_work_sync(&gsl_monitor_work);
#endif
	
	disable_irq(ts->irq);	
		   
    gslX680_shutdown_low();

	return 0;
}

static int gsl_ts_resume(struct device *dev)
{
	struct gsl_ts *ts = dev_get_drvdata(dev);
	
  GSL_DEBUG_FUNC();

	
	gslX680_shutdown_high();
	msleep(20); 	
	reset_chip(ts->client);
	startup_chip(ts->client);	
	check_mem_data(ts->client);
	
#ifdef GSL_MONITOR
	printk( "gsl_ts_resume () : queue gsl_monitor_work\n");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 300);
#endif	

#ifdef TPD_PROXIMITY
	if (tpd_proximity_flag == 1)
	{
	    tpd_enable_ps(1);
	}
#endif
	enable_irq(ts->irq);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void gsl_ts_early_suspend(struct early_suspend *h)
{
	struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	GSL_DEBUG_FUNC();

#ifdef TPD_PROXIMITY
	if (tpd_proximity_flag == 1)
	{
	    return 0;
	}
#endif
	gsl_ts_suspend(&ts->client->dev);
}

static void gsl_ts_late_resume(struct early_suspend *h)
{
	struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	printk("[GSLX680] Enter %s\n", __func__);
	gsl_ts_resume(&ts->client->dev);
}
#endif

static int  gsl_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct gsl_ts *ts;
	int rc , i;
	GSL_DEBUG_FUNC();
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -ENODEV;
	}
 
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	ts->client = client;
	i2c_set_clientdata(client, ts);
	ts->device_id = id->driver_data;
	gsl_client = client;

	rc = gslX680_ts_init(client, ts);
	if (rc < 0) {
		dev_err(&client->dev, "GSLX680 init failed\n");
		goto error_mutex_destroy;
	}
	gslX680_init();    	
	init_chip(ts->client);
    for(i=0; i<2; i++){
	        u8 read_buf = 0;
	        rc = gsl_ts_read( client, 0xfc, &read_buf, sizeof(read_buf) );
	        if  (rc  < 0){  
    		    printk("--gsl_ts_probe---test--i2c--error--ret=%d--i=%d\n", rc,i);                    
                    if(i==1){
                        gpio_free(IRQ_PORT);
                        gpio_free(RST_PORT);
                        printk("----lzk--gsl_ts_probe---goto--\n");
                        goto error_mutex_destroy;
                    }
                }
	        else{
		    printk("--gsl_ts_probe--test-i2c-ok--id=0x%x \n", read_buf);
                    break;
                }
         }

	check_mem_data(ts->client);         
	rc=  request_irq(client->irq, gsl_ts_irq, IRQF_TRIGGER_RISING, client->name, ts);
	if (rc < 0) {
		printk( "gsl_probe: request irq failed\n");
		goto error_req_irq_fail;
	}

	/* create debug attribute */
	//rc = device_create_file(&ts->input->dev, &dev_attr_debug_enable);

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	//ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	ts->early_suspend.suspend = gsl_ts_early_suspend;
	ts->early_suspend.resume = gsl_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif


#ifdef TPD_PROC_DEBUG
#if 0
    gsl_config_proc = create_proc_entry(GSL_CONFIG_PROC_FILE, 0666, NULL);
    printk("[tp-gsl] [%s] gsl_config_proc = %x \n",__func__,gsl_config_proc);
    if (gsl_config_proc == NULL)  
    {
        print_info("create_proc_entry %s failed\n", GSL_CONFIG_PROC_FILE);
    }
    else
    {
        gsl_config_proc->read_proc = gsl_config_read_proc;
        gsl_config_proc->write_proc = gsl_config_write_proc; 
    }
#else
    proc_create(GSL_CONFIG_PROC_FILE,0666,NULL,&gsl_seq_fops);
#endif
    gsl_proc_flag = 0;
#endif

#ifdef GSL_MONITOR
	printk( "gsl_ts_probe () : queue gsl_monitor_workqueue\n");

	INIT_DELAYED_WORK(&gsl_monitor_work, gsl_monitor_worker);
	gsl_monitor_workqueue = create_singlethread_workqueue("gsl_monitor_workqueue");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 1000);
#endif

#ifdef TPD_PROXIMITY
    gsl_gain_psensor_data(client); // add ps
	rc = sysfs_create_group(&client->dev.kobj,&gsl680_attribute_group);  //&client->dev.kobj
        if(rc<0)
            printk("-%s-sysfs_create_group- error \n", __func__);
        wake_lock_init(&ps_lock, WAKE_LOCK_SUSPEND, "ps wakelock");

        //INIT_DELAYED_WORK(&ts->ps_work, CM36283_work_func);
        ts_ps=ts;
#endif

	printk("[GSLX680] End %s\n", __func__);

	return 0;

//exit_set_irq_mode:	
error_req_irq_fail:
    free_irq(ts->irq, ts);	

error_mutex_destroy:
	input_free_device(ts->input);
	kfree(ts);
	return rc;
}

static int  gsl_ts_remove(struct i2c_client *client)
{
	struct gsl_ts *ts = i2c_get_clientdata(client);
	GSL_DEBUG_FUNC();

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif

#ifdef GSL_MONITOR
	cancel_delayed_work_sync(&gsl_monitor_work);
	destroy_workqueue(gsl_monitor_workqueue);
#endif
	device_init_wakeup(&client->dev, 0);
	cancel_work_sync(&ts->work);
	free_irq(ts->irq, ts);
	destroy_workqueue(ts->wq);
	input_unregister_device(ts->input);
	//device_remove_file(&ts->input->dev, &dev_attr_debug_enable);
	
	kfree(ts->touch_data);
	kfree(ts);

	return 0;
}
static const struct dev_pm_ops gsl_pm_ops ={
	.suspend =  gsl_ts_suspend,
	.resume  =  gsl_ts_resume,	
};
static const struct i2c_device_id gsl_ts_id[] = {
	{GSLX680_I2C_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, gsl_ts_id);

static struct i2c_driver gsl_ts_driver = {
	.driver = {
		.name  = GSLX680_I2C_NAME,
		.owner = THIS_MODULE,
		.pm    = &gsl_pm_ops,
	},
	//.suspend	= gsl_ts_suspend,
	//.resume	    = gsl_ts_resume,
	.probe		= gsl_ts_probe,
	.remove		= gsl_ts_remove,
	.id_table	= gsl_ts_id,
};
static struct i2c_board_info gsl_i2c_info = {
    .type = GSLX680_I2C_NAME,
    .addr = GSLX680_I2C_ADDR,
};

//extern void i2c_set_pull_strength(int bus, int strength);
//extern void set_gpio_pulldown(int gpio);
static int  gsl_ts_init(void)
{
    struct i2c_client *client;
    struct i2c_adapter *adapter;
    int ret;
	GSL_DEBUG_FUNC();
#if 0
    int panel_id = 0;
    panel_id = 0x0D;//em_config_get_panel_id();
    printk("--wc-- panel_id = 0x%x\n",panel_id);
    switch(panel_id) {
	case 0x10://pa03 CHT
		//GSLX680_ADAPTER_INDEX = 2; 
		SCREEN_MAX_X =1920;
		SCREEN_MAX_Y =1200;
		//i2c_set_pull_strength(1, 2);
		gsl_config_data_id = gsl_config_data_id_CHT;
		GSLX680_FW =  GSLX680_FW_CHT;
		fw_size =  ARRAY_SIZE(GSLX680_FW_CHT);
		break;
		
	case 0x14://PD02
		SCREEN_MAX_X =600;
		SCREEN_MAX_Y =1024;
		GSLX680_ADAPTER_INDEX = 2;
		//i2c_set_pull_strength(1, 2);
		gsl_config_data_id = gsl_config_data_id_PD02;
		GSLX680_FW =  GSLX680_FW_PD02;
		fw_size =  ARRAY_SIZE(GSLX680_FW_PD02);
		break;
		
	case 0x15://S783G  PA02
		SCREEN_MAX_X =1536;//768(?)
		SCREEN_MAX_Y =2048;//1024(?)
		gsl_config_data_id = gsl_config_data_id_S783G;
		GSLX680_FW =  GSLX680_FW_S783G;
		fw_size =  ARRAY_SIZE(GSLX680_FW_S783G);
		break;
        
    case 0x1A://PD02
		SCREEN_MAX_X =600;
		SCREEN_MAX_Y =1024;
		GSLX680_ADAPTER_INDEX = 2;
		//i2c_set_pull_strength(1, 2);
		gsl_config_data_id = gsl_config_data_id_PD02;
		GSLX680_FW =  GSLX680_FW_PD02;
		fw_size =  ARRAY_SIZE(GSLX680_FW_PD02);
		break;

    case 0x0D://HA01
        SCREEN_MAX_X = 800;
		SCREEN_MAX_Y = 1280;
        gsl_config_data_id = gsl_config_data_id_HA01;
		GSLX680_FW = GSLX680_FW_HA01;
		fw_size = ARRAY_SIZE(GSLX680_FW_HA01);
        break;
        
    default://PA02
		gsl_config_data_id = gsl_config_data_id_PA02;
		GSLX680_FW =  GSLX680_FW_PA02;
		fw_size =  ARRAY_SIZE(GSLX680_FW_PA02);
    }
#endif
		SCREEN_MAX_X =1280;
		SCREEN_MAX_Y =1920;
		//i2c_set_pull_strength(1, 2);
		gsl_config_data_id = gsl_config_data_id_CHT;
		GSLX680_FW =  GSLX680_FW_CHT;
		fw_size =  ARRAY_SIZE(GSLX680_FW_CHT);
	    adapter = i2c_get_adapter(GSLX680_ADAPTER_INDEX);
   	    if (!adapter) 
		    {
     		    printk("Can'n get %d adapter failed", GSLX680_ADAPTER_INDEX);
        		return -ENODEV;
   				}

    client = i2c_new_device(adapter, &gsl_i2c_info);
    if (!client) 
	{
        printk("get i2c device error");
        ret = -ENODEV;
        goto out_put_apapter;
    }

   // i2c_put_adapter(adapter);

    ret = i2c_add_driver(&gsl_ts_driver);
    if (ret) 
	{
        printk("i2c add driver failed");
        goto err_add_driver;
    }
    return 0;

err_add_driver:
    kfree(client);
out_put_apapter:
    kfree(adapter);
    return ret;
}
static void  gsl_ts_exit(void)
{
	GSL_DEBUG_FUNC();
	i2c_del_driver(&gsl_ts_driver);
	return;
}

late_initcall(gsl_ts_init);
module_exit(gsl_ts_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Sileadinc touchscreen controller driver");
MODULE_AUTHOR("leweihua");

