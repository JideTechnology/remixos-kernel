/* drivers/input/misc/cm3218.c - cm3218 optical sensors driver
 *
 * Copyright (C) 2011 Capella Microsystems Inc.
 *                                    
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
//#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
//#include <asm/mach-types.h>
#include <asm/setup.h>
#include <linux/wakelock.h>
#include <linux/jiffies.h>
#include <linux/types.h>
#include <linux/ioctl.h>

#include "cm3218.h" //lzk

#define LIGHTSENSOR_IOCTL_MAGIC 'l'

#define LIGHTSENSOR_IOCTL_GET_ENABLED _IOR(LIGHTSENSOR_IOCTL_MAGIC, 1, int *)
#define LIGHTSENSOR_IOCTL_ENABLE _IOW(LIGHTSENSOR_IOCTL_MAGIC, 2, int *)
#define LIGHTSENSOR_IOCTL_START _IOW(LIGHTSENSOR_IOCTL_MAGIC, 3, int *)
#define LIGHTSENSOR_IOCTL_END _IOW(LIGHTSENSOR_IOCTL_MAGIC, 4, int *)

#define I2C_RETRY_COUNT 10

#define NEAR_DELAY_TIME ((100 * HZ) / 1000)

#define CONTROL_INT_ISR_REPORT        0x00
#define CONTROL_ALS                   0x01

#define D(x...) pr_info(x)
#define CM3218_I2C_NAME "CM3218_ALS"  //lzk cm3218

#define _CM3218_AD  // if use CM3218 ad, remove this comment to normal definition code

/* Define Slave Address*/
#if  defined(_CM3218_AD)
#define		CM3218_ALS_cmd	0x90>>1
#else
#define		CM3218_ALS_cmd	0x20>>1
#endif  //ifdef CM3218_AD

#define		CM3218_check_INI	0x19>>1

#define ALS_CALIBRATED		0x6F17

/*cm3218*/
/*Define ALS Command Code*/
#define		ALS_CMD		  0x00
#define		ALS_HW  	  0x01
#define		ALS_LW	    0x02
#define		ALS_READ    0x04

/*for ALS command*/
#define CM3218_ALS_SM_1 	    (0 << 11)
#define CM3218_ALS_SM_2 	    (1 << 11)
#define CM3218_ALS_SM_HALF 	  (2 << 11)
#define CM3218_ALS_IT_125ms 	(0 << 6)
#define CM3218_ALS_IT_250ms 	(1 << 6)
#define CM3218_ALS_IT_500ms 	(2 << 6)
#define CM3218_ALS_IT_1000ms 	(3 << 6)
#define CM3218_ALS_PERS_1 	  (0 << 4)
#define CM3218_ALS_PERS_2 	  (1 << 4)
#define CM3218_ALS_PERS_4 	  (2 << 4)
#define CM3218_ALS_PERS_8 	  (3 << 4)
#define CM3218_ALS_RES_1      (1 << 2)
#define CM3218_ALS_INT_EN     (1 << 1)
#define CM3218_ALS_INT_MASK   0x10
#define CM3218_ALS_SD			    (1 << 0)/*enable/disable ALS func, 1:disable , 0: enable*/
#define CM3218_ALS_SD_MASK		0xFE 

#define LS_PWR_ON					(1 << 0)
#define PS_PWR_ON					(1 << 1)


struct lightsensor_mpp_config_data {
	uint32_t lightsensor_mpp;
	uint32_t lightsensor_amux;
};

struct lightsensor_smd_platform_data {
	const char      *name;
	uint16_t        levels[10];
	uint16_t        golden_adc;
	int             (*ls_power)(int, uint8_t);
	struct lightsensor_mpp_config_data mpp_data;
};



struct cm3218_platform_data {
	int intr;
	uint16_t levels[10];
	uint16_t golden_adc;
	int (*power)(int, uint8_t); /* power to the chip */
	uint8_t ALS_slave_address;  	
	uint8_t check_interrupt_add;
	uint16_t is_cmd;
};



extern void lnw_gpio_set_pull_strength(int gpio, int alt);

static int record_init_fail = 0;
static void sensor_irq_do_work(struct work_struct *work);
static DECLARE_WORK(sensor_irq_work, sensor_irq_do_work);

struct cm3218_info {
	struct class *cm3218_class;
	struct device *ls_dev;

	struct input_dev *ls_input_dev;

//	struct early_suspend early_suspend;
	struct i2c_client *i2c_client;
	struct workqueue_struct *lp_wq;

	int intr_pin;
	int als_enable;

	uint16_t *adc_table;
	uint16_t cali_table[10];
	int irq;

	int ls_calibrate;
	int (*power)(int, uint8_t); /* power to the chip */

	uint32_t als_kadc;
	uint32_t als_gadc;
	uint16_t golden_adc;

	struct wake_lock ps_wake_lock;
	int lightsensor_opened;
	uint8_t ALS_cmd_address;
	uint8_t check_interrupt_add;

	int current_level;
	uint16_t current_adc;

	unsigned long j_start;
	unsigned long j_end;

	uint16_t is_cmd;
	uint8_t record_clear_int_fail;
};

struct cm3218_info *lp_info;
int enable_log = 1;
int fLevel=-1;
static struct mutex als_enable_mutex, als_disable_mutex, als_get_adc_mutex;
static struct mutex CM3218_control_mutex;
static int lightsensor_enable(struct cm3218_info *lpi,int flag);
static int lightsensor_disable(struct cm3218_info *lpi,int flag);
static int initial_cm3218(struct cm3218_info *lpi);

int32_t als_kadc;

static int control_and_report(struct cm3218_info *lpi, uint8_t mode, uint8_t cmd_enable);

static int I2C_RxData(uint16_t slaveAddr, uint8_t cmd, uint8_t *rxData, int length)
{
	uint8_t loop_i;
	int val;
  uint8_t subaddr[1];
  
  subaddr[0] = cmd;	
	struct cm3218_info *lpi = lp_info;

	struct i2c_msg msg[] = {
		{
		 .addr = slaveAddr,
		 .flags = 0,
		 .len = 1,
		 .buf = subaddr,
		 },
		{
		 .addr = slaveAddr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxData,
		 },
	};

	for (loop_i = 0; loop_i < I2C_RETRY_COUNT; loop_i++) {

		if (i2c_transfer(lp_info->i2c_client->adapter, msg, 2) > 0)
			break;

		val = gpio_get_value(lpi->intr_pin);
		/*check intr GPIO when i2c error*/
		if (loop_i == 0 || loop_i == I2C_RETRY_COUNT -1)
			D("[PS][CM3218 error] %s, i2c err, slaveAddr 0x%x ISR gpio %d  = %d, record_init_fail %d \n",
				__func__, slaveAddr, lpi->intr_pin, val, record_init_fail);

		msleep(10);
	}
	if (loop_i >= I2C_RETRY_COUNT) {
		printk(KERN_ERR "[PS_ERR][CM3218 error] %s retry over %d\n",
			__func__, I2C_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

static int I2C_RxData2(uint16_t slaveAddr, uint8_t *rxData, int length)
{
	uint8_t loop_i;
	int val;
	struct cm3218_info *lpi = lp_info;

	struct i2c_msg msg[] = {
		{
		 .addr = slaveAddr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxData,
		 },
	};

	for (loop_i = 0; loop_i < I2C_RETRY_COUNT; loop_i++) {

		if (i2c_transfer(lp_info->i2c_client->adapter, msg, 1) > 0)
			break;

		val = gpio_get_value(lpi->intr_pin);
		/*check intr GPIO when i2c error*/
		if (loop_i == 0 || loop_i == I2C_RETRY_COUNT -1)
			D("[PS][CM3218 error] %s, i2c err, slaveAddr 0x%x ISR gpio %d  = %d, record_init_fail %d \n",
				__func__, slaveAddr, lpi->intr_pin, val, record_init_fail);

		msleep(10);
	}
	if (loop_i >= I2C_RETRY_COUNT) {
		printk(KERN_ERR "[PS_ERR][CM3218 error] %s retry over %d\n",
			__func__, I2C_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

static int I2C_TxData(uint16_t slaveAddr, uint8_t *txData, int length)
{
	uint8_t loop_i;
	int val;
	struct cm3218_info *lpi = lp_info;
	struct i2c_msg msg[] = {
		{
		 .addr = slaveAddr,
		 .flags = 0,
		 .len = length,
		 .buf = txData,
		 },
	};

	for (loop_i = 0; loop_i < I2C_RETRY_COUNT; loop_i++) {
		if (i2c_transfer(lp_info->i2c_client->adapter, msg, 1) > 0)
			break;

		val = gpio_get_value(lpi->intr_pin);
		/*check intr GPIO when i2c error*/
		if (loop_i == 0 || loop_i == I2C_RETRY_COUNT -1)
			D("[PS][CM3218 error] %s, i2c err, slaveAddr 0x%x, value 0x%x, ISR gpio%d  = %d, record_init_fail %d\n",
				__func__, slaveAddr, txData[0], lpi->intr_pin, val, record_init_fail);

		msleep(10);
	}

	if (loop_i >= I2C_RETRY_COUNT) {
		printk(KERN_ERR "[PS_ERR][CM3218 error] %s retry over %d\n",
			__func__, I2C_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

static int _cm3218_I2C_Read_Byte(uint16_t slaveAddr, uint8_t *pdata)
{
	uint8_t buffer = 0;
	int ret = 0;

	if (pdata == NULL)
		return -EFAULT;

	ret = I2C_RxData2(slaveAddr, &buffer, 1);
	if (ret < 0) {
		pr_err(
			"[PS_ERR][CM3218 error]%s: I2C_RxData fail, slave addr: 0x%x\n",
			__func__, slaveAddr);
		return ret;
	}

	*pdata = buffer;
#if 0
	/* Debug use */
	printk(KERN_DEBUG "[CM3218] %s: I2C_RxData[0x%x] = 0x%x\n",
		__func__, slaveAddr, *pdata);
#endif
	return ret;
}

static int _cm3218_I2C_Read_Word(uint16_t slaveAddr, uint8_t cmd, uint16_t *pdata)
{
	uint8_t buffer[2];
	int ret = 0;

	if (pdata == NULL)
		return -EFAULT;

	ret = I2C_RxData(slaveAddr, cmd, buffer, 2);
	if (ret < 0) {
		pr_err(
			"[PS_ERR][CM3218 error]%s: I2C_RxData fail [0x%x, 0x%x]\n",
			__func__, slaveAddr, cmd);


		return ret;
	}

	*pdata = (buffer[1]<<8)|buffer[0];
#if 0
	/* Debug use */
	printk(KERN_DEBUG "[CM3218] %s: I2C_RxData[0x%x, 0x%x] = 0x%x\n",
		__func__, slaveAddr, cmd, *pdata);
#endif


	return ret;
}

static int _cm3218_I2C_Write_Word(uint16_t SlaveAddress, uint8_t cmd, uint16_t data)
{
	char buffer[3];
	int ret = 0;
#if 0
	/* Debug use */
	printk(KERN_DEBUG
	"[CM3218] %s: _cm3218_I2C_Write_Word[0x%x, 0x%x, 0x%x]\n",
		__func__, SlaveAddress, cmd, data);
#endif
	buffer[0] = cmd;
	buffer[1] = (uint8_t)(data&0xff);
	buffer[2] = (uint8_t)((data&0xff00)>>8);	
	
	ret = I2C_TxData(SlaveAddress, buffer, 3);
	if (ret < 0) {
		pr_err("[PS_ERR][CM3218 error]%s: I2C_TxData fail\n", __func__);


		return -EIO;
	}

	return ret;
}

static int get_ls_adc_value(uint16_t *als_step, bool resume)
{
	struct cm3218_info *lpi = lp_info;
	uint16_t tmpResult;
	int ret = 0;

	if (als_step == NULL)
		return -EFAULT;

	/* Read ALS data: */
	ret = _cm3218_I2C_Read_Word(lpi->ALS_cmd_address, ALS_READ, als_step);

	if (ret < 0) {
		pr_err(
			"[LS][CM3218 error]%s: _cm3218_I2C_Read_Word fail\n",
			__func__);
		return -EIO;
	}

	if (!lpi->ls_calibrate) 
	{
		tmpResult = (uint32_t)(*als_step) * lpi->als_gadc / lpi->als_kadc;
		tmpResult = tmpResult*200;
		if (tmpResult > 0xFFFF)
		{
			*als_step = 0xFFFF;
		}
		else
		{
			*als_step = tmpResult;
		}  			
	}
	//D("--------aaaaaa---------als_step[%02x]---[%d]----------------\n",*als_step,*als_step);
	return ret;
}


static int set_lsensor_range(uint16_t low_thd, uint16_t high_thd)
{
	int ret = 0;
	struct cm3218_info *lpi = lp_info;

	_cm3218_I2C_Write_Word(lpi->ALS_cmd_address, ALS_HW, high_thd);
	_cm3218_I2C_Write_Word(lpi->ALS_cmd_address, ALS_LW, low_thd);

	return ret;
}

static void sensor_irq_do_work(struct work_struct *work)
{
	struct cm3218_info *lpi = lp_info;
	control_and_report(lpi, CONTROL_INT_ISR_REPORT, 0);

	enable_irq(lpi->irq);
}

static irqreturn_t cm3218_irq_handler(int irq, void *data)
{
	struct cm3218_info *lpi = data;
	disable_irq_nosync(lpi->irq);

	queue_work(lpi->lp_wq, &sensor_irq_work);

	return IRQ_HANDLED;
}

static int als_power(int enable)
{
	struct cm3218_info *lpi = lp_info;

	if (lpi->power)
		lpi->power(LS_PWR_ON, 1);

	return 0;
}

static int ls_initial_cmd(struct cm3218_info *lpi)  //void
{	
	/*must disable l-sensor interrupt befrore IST create*//*disable ALS func*/ 		
	lpi->is_cmd |= CM3218_ALS_SD;
	return _cm3218_I2C_Write_Word(lpi->ALS_cmd_address, ALS_HW, lpi->is_cmd);
}

void lightsensor_set_kvalue(struct cm3218_info *lpi)
{
	if (!lpi) {
		pr_err("[LS][CM3218 error]%s: ls_info is empty\n", __func__);
		return;
	}

	if (als_kadc >> 16 == ALS_CALIBRATED)
		lpi->als_kadc = als_kadc & 0xFFFF;
	else {
		lpi->als_kadc = 0;
		D("[LS][CM3218] %s: no ALS calibrated\n", __func__);
	}

	if (lpi->als_kadc && lpi->golden_adc > 0) {
		lpi->als_kadc = (lpi->als_kadc > 0 && lpi->als_kadc < 0x1000) ?
				lpi->als_kadc : lpi->golden_adc;
		lpi->als_gadc = lpi->golden_adc;
	} else {
		lpi->als_kadc = 1;
		lpi->als_gadc = 1;
	}
}

static int lightsensor_update_table(struct cm3218_info *lpi)
{
	uint32_t tmpData[10];
	int i;
	for (i = 0; i < 10; i++) {
		tmpData[i] = (uint32_t)(*(lpi->adc_table + i))
				* lpi->als_kadc / lpi->als_gadc ;
		if( tmpData[i] <= 0xFFFF ){
      lpi->cali_table[i] = (uint16_t) tmpData[i];		
    } else {
      lpi->cali_table[i] = 0xFFFF;    
    }         
	}

	return 0;
}

static int lightsensor_enable(struct cm3218_info *lpi,int flag)
{
	int ret = -EIO;
	
	mutex_lock(&als_enable_mutex);
	//D("[LS][CM3218] %s\n", __func__);

	if (lpi->als_enable) {
		//D("[LS][CM3218] %s: already enabled\n", __func__);
		ret = 0;
	} 
	else
	{
		if(flag == 1)
		{
			lpi->als_enable = 1;
		}
  		ret = control_and_report(lpi, CONTROL_ALS, 1);
	}	
	mutex_unlock(&als_enable_mutex);
	return ret;
}

static int lightsensor_disable(struct cm3218_info *lpi,int flag)
{
	int ret = -EIO;
	mutex_lock(&als_disable_mutex);
	//D("[LS][CM3218] %s\n", __func__);

	if ( lpi->als_enable == 0 ) {
		//D("[LS][CM3218] %s: already disabled\n", __func__);
		ret = 0;
	}
	else
	{
		if(flag == 1)
		{
			lpi->als_enable = 0;
		}	
	        ret = control_and_report(lpi, CONTROL_ALS, 0);
		
	}
	mutex_unlock(&als_disable_mutex);
	return ret;
}

static int lightsensor_open(struct inode *inode, struct file *file)
{
	struct cm3218_info *lpi = lp_info;
	int rc = 0;

	//D("[LS][CM3218] %s\n", __func__);
	if (lpi->lightsensor_opened) {
		pr_err("[LS][CM3218 error]%s: already opened\n", __func__);
		rc = -EBUSY;
	}
	lpi->lightsensor_opened = 1;
	return rc;
}

static int lightsensor_release(struct inode *inode, struct file *file)
{
	struct cm3218_info *lpi = lp_info;

	//D("[LS][CM3218] %s\n", __func__);
	lpi->lightsensor_opened = 0;
	return 0;
}

static long lightsensor_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int rc, val;
	struct cm3218_info *lpi = lp_info;

	/*D("[CM3218] %s cmd %d\n", __func__, _IOC_NR(cmd));*/

	switch (cmd) {
	case LIGHTSENSOR_IOCTL_ENABLE:
		if (get_user(val, (unsigned long __user *)arg)) {
			rc = -EFAULT;
			break;
		}
		//D("[LS][CM3218] %s LIGHTSENSOR_IOCTL_ENABLE, value = %d\n",
		//	__func__, val);
		rc = val ? lightsensor_enable(lpi,1) : lightsensor_disable(lpi,1);
		break;
	case LIGHTSENSOR_IOCTL_GET_ENABLED:
		val = lpi->als_enable;
		//D("[LS][CM3218] %s LIGHTSENSOR_IOCTL_GET_ENABLED, enabled %d\n",
			//__func__, val);
		rc = put_user(val, (unsigned long __user *)arg);
		break;

	case LIGHTSENSOR_IOCTL_START:
		//D("[LS][CM3218] LIGHTSENSOR_IOCTL_START\n");
		rc = lightsensor_enable(lpi,1);
		break;
	case LIGHTSENSOR_IOCTL_END:
		//D("[LS][CM3218] LIGHTSENSOR_IOCTL_END\n");
		rc = lightsensor_disable(lpi,1);
		break;

	default:
		pr_err("[LS][CM3218 error]%s: invalid cmd %d\n",
			__func__, _IOC_NR(cmd));
		rc = -EINVAL;
	}

	return rc;
}

static const struct file_operations lightsensor_fops = {
	.owner = THIS_MODULE,
	.open = lightsensor_open,
	.release = lightsensor_release,
	.unlocked_ioctl = lightsensor_ioctl
};

static struct miscdevice lightsensor_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lightsensor",
	.fops = &lightsensor_fops
};

static ssize_t ls_adc_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int ret;
	struct cm3218_info *lpi = lp_info;
	ret = sprintf(buf, "ADC[0x%04X] => level %d\n",
		lpi->current_adc, lpi->current_level);

	return ret;
}

static ssize_t ls_enable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct cm3218_info *lpi = lp_info;

	ret = sprintf(buf, "Light sensor Auto Enable = %d\n",
			lpi->als_enable);

	return ret;
}

static ssize_t ls_enable_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	int ret = 0;
	int ls_auto;
	struct cm3218_info *lpi = lp_info;

	ls_auto = -1;
	sscanf(buf, "%d", &ls_auto);

	if (ls_auto != 0 && ls_auto != 1 && ls_auto != 147)
		return -EINVAL;

	if (ls_auto) {
		ret = lightsensor_enable(lpi,1);
	} else {
		ret = lightsensor_disable(lpi,1);
	}

	if (ret < 0)
		pr_err(
		"[LS][CM3218 error]%s: set auto light sensor fail\n",
		__func__);

	return count;
}

static ssize_t ls_kadc_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cm3218_info *lpi = lp_info;
	int ret;

	ret = sprintf(buf, "kadc = 0x%x",
			lpi->als_kadc);


	return ret;
}

static ssize_t ls_kadc_store(struct device *dev,

				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct cm3218_info *lpi = lp_info;
	int kadc_temp = 0;

	sscanf(buf, "%d", &kadc_temp);

	mutex_lock(&als_get_adc_mutex);
  if(kadc_temp != 0) {
		lpi->als_kadc = kadc_temp;
		if(  lpi->als_gadc != 0){
  		if (lightsensor_update_table(lpi) < 0)
				printk(KERN_ERR "[LS][CM3218 error] %s: update ls table fail\n", __func__);
  	} else {
			printk(KERN_INFO "[LS]%s: als_gadc =0x%x wait to be set\n",
					__func__, lpi->als_gadc);
  	}		
	} else {
		printk(KERN_INFO "[LS]%s: als_kadc can't be set to zero\n",
				__func__);
	}
				
	mutex_unlock(&als_get_adc_mutex);
	return count;
}

static ssize_t ls_gadc_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cm3218_info *lpi = lp_info;
	int ret;

	ret = sprintf(buf, "gadc = 0x%x\n", lpi->als_gadc);

	return ret;
}

static ssize_t ls_gadc_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct cm3218_info *lpi = lp_info;
	int gadc_temp = 0;

	sscanf(buf, "%d", &gadc_temp);
	
	mutex_lock(&als_get_adc_mutex);
  if(gadc_temp != 0) {
		lpi->als_gadc = gadc_temp;
		if(  lpi->als_kadc != 0){
  		if (lightsensor_update_table(lpi) < 0)
				printk(KERN_ERR "[LS][CM3218 error] %s: update ls table fail\n", __func__);
  	} else {
			printk(KERN_INFO "[LS]%s: als_kadc =0x%x wait to be set\n",
					__func__, lpi->als_kadc);
  	}		
	} else {
		printk(KERN_INFO "[LS]%s: als_gadc can't be set to zero\n",
				__func__);
	}
	mutex_unlock(&als_get_adc_mutex);
	return count;
}

static ssize_t ls_adc_table_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	unsigned length = 0;
	int i;

	for (i = 0; i < 10; i++) {
		length += sprintf(buf + length,
			"[CM3218]Get adc_table[%d] =  0x%x ; %d, Get cali_table[%d] =  0x%x ; %d, \n",
			i, *(lp_info->adc_table + i),
			*(lp_info->adc_table + i),
			i, *(lp_info->cali_table + i),
			*(lp_info->cali_table + i));
	}
	return length;
}

static ssize_t ls_adc_table_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct cm3218_info *lpi = lp_info;
	char *token[10];
	uint16_t tempdata[10];
	int i;

	//printk(KERN_INFO "[LS][CM3218]%s\n", buf);
	for (i = 0; i < 10; i++) {
		token[i] = strsep((char **)&buf, " ");
		tempdata[i] = simple_strtoul(token[i], NULL, 16);
		if (tempdata[i] < 1 || tempdata[i] > 0xffff) {
			printk(KERN_ERR
			"[LS][CM3218 error] adc_table[%d] =  0x%x Err\n",
			i, tempdata[i]);
			return count;
		}
	}
	mutex_lock(&als_get_adc_mutex);
	for (i = 0; i < 10; i++) {
		lpi->adc_table[i] = tempdata[i];
		printk(KERN_INFO
		"[LS][CM3218]Set lpi->adc_table[%d] =  0x%x\n",
		i, *(lp_info->adc_table + i));
	}
	if (lightsensor_update_table(lpi) < 0)
		printk(KERN_ERR "[LS][CM3218 error] %s: update ls table fail\n",
		__func__);
	mutex_unlock(&als_get_adc_mutex);
	//D("[LS][CM3218] %s\n", __func__);

	return count;
}

static uint8_t ALS_CONF = 0;
static ssize_t ls_conf_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "ALS_CONF = %x\n", ALS_CONF);
}

static ssize_t ls_conf_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct cm3218_info *lpi = lp_info;
	int value = 0;
	sscanf(buf, "0x%x", &value);

	ALS_CONF = value;
	printk(KERN_INFO "[LS]set ALS_CONF = %x\n", ALS_CONF);
	_cm3218_I2C_Write_Word(lpi->ALS_cmd_address, ALS_CMD, ALS_CONF);
	return count;
}

static ssize_t ls_fLevel_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "fLevel = %d\n", fLevel);
}

static ssize_t ls_fLevel_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct cm3218_info *lpi = lp_info;
	int value=0;
	sscanf(buf, "%d", &value);
	(value>=0)?(value=min(value,10)):(value=max(value,-1));
	fLevel=value;
	input_report_abs(lpi->ls_input_dev, ABS_MISC, fLevel);
	input_sync(lpi->ls_input_dev);
	printk(KERN_INFO "[LS]set fLevel = %d\n", fLevel);

	msleep(1000);
	fLevel=-1;
	return count;
}


static ssize_t CM3218_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	return count;
}


static struct device_attribute dev_attr_light_enable =
__ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH, ls_enable_show, ls_enable_store);

static struct device_attribute dev_attr_light_kadc =
__ATTR(kadc, S_IRUGO | S_IWUSR | S_IWGRP, ls_kadc_show, ls_kadc_store);

static struct device_attribute dev_attr_light_gadc =
__ATTR(gadc, S_IRUGO | S_IWUSR | S_IWGRP, ls_gadc_show, ls_gadc_store);

static struct device_attribute dev_attr_light_adc_table =
__ATTR(adc_table, S_IRUGO | S_IWUSR | S_IWGRP, ls_adc_table_show, ls_adc_table_store);

static struct device_attribute dev_attr_light_conf =
__ATTR(conf, S_IRUGO | S_IWUSR | S_IWGRP, ls_conf_show, ls_conf_store);

static struct device_attribute dev_attr_light_fLevel =
__ATTR(fLevel, S_IRUGO | S_IWUSR | S_IWGRP, ls_fLevel_show, ls_fLevel_store);

static struct device_attribute dev_attr_light_adc =
__ATTR(adc, S_IRUGO | S_IWUSR | S_IWGRP, ls_adc_show, NULL);

static struct device_attribute dev_attr_light_delay =  __ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH, NULL,CM3218_delay_store); //lzk


static struct attribute *light_sysfs_attrs[] = {
&dev_attr_light_enable.attr,
&dev_attr_light_kadc.attr,
&dev_attr_light_gadc.attr,
&dev_attr_light_adc_table.attr,
&dev_attr_light_conf.attr,
&dev_attr_light_fLevel.attr,
&dev_attr_light_adc.attr,
&dev_attr_light_delay.attr,
NULL
};

static struct attribute_group light_attribute_group = {
.attrs = light_sysfs_attrs,
};

static int lightsensor_setup(struct cm3218_info *lpi)
{
	int ret;

	lpi->ls_input_dev = input_allocate_device();
	if (!lpi->ls_input_dev) {
		pr_err(
			"[LS][CM3218 error]%s: could not allocate ls input device\n",
			__func__);
		return -ENOMEM;
	}
	lpi->ls_input_dev->name =CM3218_I2C_NAME;//lzk "cm3218-ls";
	set_bit(EV_ABS, lpi->ls_input_dev->evbit);
	input_set_abs_params(lpi->ls_input_dev, ABS_MISC, 0, 9, 0, 0);

	ret = input_register_device(lpi->ls_input_dev);
	if (ret < 0) {
		pr_err("[LS][CM3218 error]%s: can not register ls input device\n",
				__func__);
		goto err_free_ls_input_device;
	}

	ret = misc_register(&lightsensor_misc);
	if (ret < 0) {
		pr_err("[LS][CM3218 error]%s: can not register ls misc device\n",
				__func__);
		goto err_unregister_ls_input_device;
	}

	return ret;

err_unregister_ls_input_device:
	input_unregister_device(lpi->ls_input_dev);
err_free_ls_input_device:
	input_free_device(lpi->ls_input_dev);
	return ret;
}

static int initial_cm3218(struct cm3218_info *lpi)
{
	int val, ret, fail_counter = 0;
	uint8_t add = 0;
	int data_reserved = 0x00;

	val = gpio_get_value(lpi->intr_pin);
	//D("[LS][CM3218] %s, INTERRUPT GPIO val = %d\n", __func__, val);

check_interrupt_gpio:
  if (fail_counter >= 10) {
  		D("[LS][CM3218] %s, initial fail_counter = %d\n", __func__, fail_counter);
  		if (record_init_fail == 0)
  			record_init_fail = 1;
  		return -ENOMEM;/*If devices without cm3218 chip and did not probe driver*/
  }
  // add by linfayang modify for init 90 03 ->00 00
 	ret = _cm3218_I2C_Write_Word(lpi->ALS_cmd_address, ALS_RESERVED, data_reserved );
	if ((ret < 0) && (fail_counter < 10)) {	
		fail_counter++;
 		printk("in (ret < 0) && (fail_counter < 10) \n");
		val = gpio_get_value(lpi->intr_pin);
		if( val == 0 ){
			D("[LS][CM3218] %s, interrupt GPIO val = %d, , inital fail_counter %d\n",
				__func__, val, fail_counter);
 			ret =_cm3218_I2C_Read_Byte(lpi->check_interrupt_add, &add);
 			D("[LS][CM3218] %s, check_interrupt_add value = 0x%x, ret %d\n",
				__func__, add, ret);
 		}
		val = gpio_get_value(lpi->intr_pin);
		if( val == 0 ){
			D("[LS][CM3218] %s, interrupt GPIO val = %d, , inital fail_counter %d\n",
				__func__, val, fail_counter);
 			ret =_cm3218_I2C_Read_Byte(lpi->check_interrupt_add, &add);
 			D("[LS][CM3218] %s, check_interrupt_add value = 0x%x, ret %d\n",
				__func__, add, ret);
 		} 		
		goto	check_interrupt_gpio;
	}
	
  	lpi->is_cmd = lpi->is_cmd | CM3218_ALS_SD; 
	ret = _cm3218_I2C_Write_Word(lpi->ALS_cmd_address, ALS_CMD, lpi->is_cmd );
	if (ret < 0) {
		pr_err("[LS][CM3218 error]%s: _cm3218_I2C_Write_Word  request failed (%d)\n",
			__func__, ret);
		return ret;
	}
	return 0;
}

static int i2c_test_flag=0;
int config_gpio_ctrl0reg(struct gpio_chip *chip,u32 value, unsigned offset);
int config_gpio_ctrl1reg(struct gpio_chip *chip,u32 value, unsigned offset);

static int cm3218_setup(struct cm3218_info *lpi)
{
	int ret = 0;
	struct gpio_chip *chip = gpiod_to_chip(gpio_to_desc(332));
	unsigned	offset = gpio_chip_hwgpio(gpio_to_desc(332));

	als_power(1);
	msleep(5);
	//ret = gpio_request(lpi->intr_pin, "gpio_cm3218_intr");  //lzk ceshi
	if (ret < 0) {
		pr_err("[LS][CM3218 error]%s: gpio %d request failed (%d)\n",
			__func__, lpi->intr_pin, ret);
		return ret;
	}

	ret = gpio_direction_input(lpi->intr_pin);
	if (ret < 0) {
		pr_err(
			"[LS][CM3218 error]%s: fail to set gpio %d as input (%d)\n",
			__func__, lpi->intr_pin, ret);
		goto fail_free_intr_pin;
	}

	ret = initial_cm3218(lpi);
	if (ret < 0) {
		pr_err(
			"[LS_ERR][CM3218 error]%s: fail to initial cm3218 (%d)\n",
			__func__, ret);
		goto fail_free_intr_pin;
	}
	
	/*Default disable L sensor*/
        i2c_test_flag= ls_initial_cmd(lpi);  //lzk
        if(i2c_test_flag<0){
           int i=5;
           while((i>0)&&(i2c_test_flag<0)){
               i2c_test_flag= ls_initial_cmd(lpi);
               i=i-1;               
           }
           printk("----cm3218--i2c--test--5-count-i2c_test_flag=%d\n",i2c_test_flag);
           if(i2c_test_flag <0){
                printk("----cm3218--i2c--test-error--\n");
                ret = i2c_test_flag;
                goto fail_free_intr_pin; 
           }       
        }else{
           printk("-----cm3218--i2c--test--ok-\n");
        }

	config_gpio_ctrl0reg(chip,  0x4c108200, offset);
	config_gpio_ctrl1reg(chip,  0x04c00044, offset);
	config_gpio_ctrl1reg(chip,	0x84c00044, offset);

	ret = request_any_context_irq(lpi->irq,	cm3218_irq_handler, IRQF_TRIGGER_LOW,"cm3218", lpi);
	if (ret < 0) {
		pr_err(
			"[LS][CM3218 error]%s: req_irq(%d) fail for gpio %d (%d)\n",
			__func__, lpi->irq,
			lpi->intr_pin, ret);
		goto fail_free_intr_pin;
	}

	return ret;

fail_free_intr_pin:
	gpio_free(lpi->intr_pin);
	return ret;
}

static void cm3218_early_suspend(struct early_suspend *h)
{
	struct cm3218_info *lpi = lp_info;

	//D("[LS][CM3218] %s\n", __func__);

	if (lpi->als_enable)
		lightsensor_disable(lpi,0);
}

static void cm3218_late_resume(struct early_suspend *h)
{
	struct cm3218_info *lpi = lp_info;

	//D("[LS][CM3218] %s\n", __func__);

	if (lpi->als_enable)
		lightsensor_enable(lpi,0);
}

static int cm3218_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct cm3218_info *lpi;
	struct cm3218_platform_data *pdata;


	lpi = kzalloc(sizeof(struct cm3218_info), GFP_KERNEL);
	if (!lpi)
		return -ENOMEM;
	/*D("[CM3218] %s: client->irq = %d\n", __func__, client->irq);*/

//	lnw_gpio_set_pull_strength(CM3218_IRQ_GPIO, 2);//set pullup to 20k
	lpi->i2c_client = client;
	pdata = client->dev.platform_data;
	if (!pdata) {
		pr_err("[LS][CM3218 error]%s: Assign platform_data error!!\n",
			__func__);
		ret = -EBUSY;
		goto err_platform_data_null;
	}

        ret=gpio_request(CM3218_IRQ_GPIO, "CM3218_SENSOR_IRQ");  //lzk 
        if (ret < 0) 
        {
           printk("Failed to request GPIO:63, ERRNO:%d", ret);
           return -ENODEV;
        }
        else
        {               
            gpio_direction_input(CM3218_IRQ_GPIO);
            client->irq=gpio_to_irq(CM3218_IRQ_GPIO);
        }
        

	lpi->irq = client->irq;

	i2c_set_clientdata(client, lpi);
	
  	lpi->intr_pin = pdata->intr;
	lpi->adc_table = pdata->levels;
	lpi->power = pdata->power;
	
	lpi->ALS_cmd_address = pdata->ALS_slave_address;
	lpi->check_interrupt_add = pdata->check_interrupt_add;

	lpi->is_cmd  = pdata->is_cmd;
	
	lpi->j_start = 0;
	lpi->j_end = 0;
	lpi->record_clear_int_fail=0;
	
	if (pdata->is_cmd == 0) {
		lpi->is_cmd  = CM3218_ALS_SM_2 | CM3218_ALS_IT_250ms | CM3218_ALS_PERS_1 | CM3218_ALS_RES_1;
	}

	lp_info = lpi;

	mutex_init(&CM3218_control_mutex);

	mutex_init(&als_enable_mutex);
	mutex_init(&als_disable_mutex);
	mutex_init(&als_get_adc_mutex);

	ret = lightsensor_setup(lpi);
	if (ret < 0) {
		pr_err("[LS][CM3218 error]%s: lightsensor_setup error!!\n",
			__func__);
		goto err_lightsensor_setup;
	}

  //SET LUX STEP FACTOR HERE
  // if adc raw value 70 eqauls to 1 lux
  // the following will set the factor 1/70
  // and lpi->golden_adc = 1;  
  // set als_kadc = (ALS_CALIBRATED <<16) | 70;

        als_kadc = (ALS_CALIBRATED <<16) | 70;
        lpi->golden_adc = 1;

  //ls calibrate always set to 1 
        lpi->ls_calibrate = 1;

	lightsensor_set_kvalue(lpi);
	ret = lightsensor_update_table(lpi);
	if (ret < 0) {
		pr_err("[LS][CM3218 error]%s: update ls table fail\n",
			__func__);
		goto err_lightsensor_update_table;
	}

	lpi->lp_wq = create_singlethread_workqueue("cm3218_wq");
	if (!lpi->lp_wq) {
		pr_err("[PS][CM3218 error]%s: can't create workqueue\n", __func__);
		ret = -ENOMEM;
		goto err_create_singlethread_workqueue;
	}
	wake_lock_init(&(lpi->ps_wake_lock), WAKE_LOCK_SUSPEND, "proximity");

	ret = cm3218_setup(lpi);
	if (ret < 0) {
		pr_err("[PS_ERR][CM3218 error]%s: cm3218_setup error!\n", __func__);
		goto err_cm3218_setup;
	}

	lpi->cm3218_class = class_create(THIS_MODULE, "optical_sensors");
	if (IS_ERR(lpi->cm3218_class)) {
		ret = PTR_ERR(lpi->cm3218_class);
		lpi->cm3218_class = NULL;
		goto err_create_class;
	}

	lpi->ls_dev = device_create(lpi->cm3218_class,
				NULL, 0, "%s", "lightsensor");
	if (unlikely(IS_ERR(lpi->ls_dev))) {
		ret = PTR_ERR(lpi->ls_dev);
		lpi->ls_dev = NULL;
		goto err_create_ls_device;
	}

	/* register the attributes */
	//ret = sysfs_create_group(&lpi->ls_input_dev->dev.kobj,&light_attribute_group);
        ret =  sysfs_create_group(&client->dev.kobj,&light_attribute_group);
	if (ret) {
		pr_err("[LS][CM3232 error]%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	/*lpi->early_suspend.level =
			EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	lpi->early_suspend.suspend = cm3218_early_suspend;
	lpi->early_suspend.resume = cm3218_late_resume;
	register_early_suspend(&lpi->early_suspend);*/
	D("[PS][CM3218] %s: Probe success!\n", __func__);
  	lpi->als_enable=0;
  
	return ret;

err_sysfs_create_group_light:
	device_unregister(lpi->ls_dev);
err_create_ls_device:
	class_destroy(lpi->cm3218_class);
err_create_class:
err_cm3218_setup:
	destroy_workqueue(lpi->lp_wq);
	wake_lock_destroy(&(lpi->ps_wake_lock));

	input_unregister_device(lpi->ls_input_dev);
	input_free_device(lpi->ls_input_dev);
err_create_singlethread_workqueue:
err_lightsensor_update_table:
	mutex_destroy(&CM3218_control_mutex);
	misc_deregister(&lightsensor_misc);
err_lightsensor_setup:
	mutex_destroy(&als_enable_mutex);
	mutex_destroy(&als_disable_mutex);
	mutex_destroy(&als_get_adc_mutex);
err_platform_data_null:
	kfree(lpi);
	return ret;
}
   
static int control_and_report( struct cm3218_info *lpi, uint8_t mode, uint8_t cmd_enable ) {
	int ret=0;
	uint16_t adc_value = 0;	
	int level = 0, i, val;
	int fail_counter = 0;
	uint8_t add = 0;

	mutex_lock(&CM3218_control_mutex);

	while(1)
	{
		val = gpio_get_value(lpi->intr_pin);  

		if( val == 0)
		{
			ret = _cm3218_I2C_Read_Byte(lpi->check_interrupt_add, &add);
		}
		val = gpio_get_value(lpi->intr_pin);
		if( val == 0)
		{
			ret = _cm3218_I2C_Read_Byte(lpi->check_interrupt_add, &add);
		}

		lpi->is_cmd &= CM3218_ALS_INT_MASK;
		ret = _cm3218_I2C_Write_Word(lpi->ALS_cmd_address, ALS_CMD, lpi->is_cmd);
		if( ret == 0 )
		{
			break;
		}
		else 
		{	
			fail_counter++;
			val = gpio_get_value(lpi->intr_pin);	
		}   
		if (fail_counter >= 10) 
		{
			D("[CM3218] %s, clear INT fail_counter = %d\n", __func__, fail_counter);
			if (lpi->record_clear_int_fail == 0)
			lpi->record_clear_int_fail = 1;
			ret=-ENOMEM;
			goto error_clear_interrupt;
		}
	}

	if( mode == CONTROL_ALS )
	{
		if(cmd_enable)
		{
			lpi->is_cmd &= CM3218_ALS_SD_MASK;      
		} 
		else 
		{
			lpi->is_cmd |= CM3218_ALS_SD;
		}
		_cm3218_I2C_Write_Word(lpi->ALS_cmd_address, ALS_CMD, lpi->is_cmd);
		lpi->als_enable=cmd_enable;
	}
  
	if((mode == CONTROL_ALS)&&(cmd_enable==1))
	{
		  input_report_abs(lpi->ls_input_dev, ABS_MISC, -1);
		  input_sync(lpi->ls_input_dev);
		  msleep(100);  
	}

	if(lpi->als_enable)
	{
		get_ls_adc_value(&adc_value, 0); 
		lpi->is_cmd|=CM3218_ALS_INT_EN;
	}

	if(lpi->als_enable)
	{
#if 0
		if( lpi->ls_calibrate ) 
		{
			for (i = 0; i < 10; i++) 
			{
				if (adc_value <= (*(lpi->cali_table + i))) 
				{
					level = i;
					if (*(lpi->cali_table + i))
					{
	  					break;
					}	
				}
				if ( i == 9) 
				{/*avoid  i = 10, because 'cali_table' of size is 10 */
					level = i;
					break;
				}
			}
    		}
		else 
		{
			for (i = 0; i < 10; i++) 
			{
				if (adc_value <= (*(lpi->adc_table + i))) 
				{
					level = i;
					if (*(lpi->adc_table + i))
					{
						break;
					}
				}
				if ( i == 9) 
				{/*avoid  i = 10, because 'cali_table' of size is 10 */
					level = i;	
					break;
				}
			}
		}

		ret = set_lsensor_range(((i == 0) || (adc_value == 0)) ? 0 :*(lpi->cali_table + (i - 1)) + 1,*(lpi->cali_table + i));	  

		lpi->current_level = level;
		lpi->current_adc = adc_value;  
#endif  
		input_report_abs(lpi->ls_input_dev, ABS_MISC, adc_value);
		input_sync(lpi->ls_input_dev);
 	}
 
  	ret = _cm3218_I2C_Write_Word(lpi->ALS_cmd_address, ALS_CMD, lpi->is_cmd);

	error_clear_interrupt:
	mutex_unlock(&CM3218_control_mutex);
	return ret;
}
static int cm3218_suspend(struct device *dev){
	struct cm3218_info *lpi = lp_info;

	D("[LS][CM3218] %s\n", __func__);

	if (lpi->als_enable)
		lightsensor_enable(lpi,1);
	}
static int cm3218_resume(struct device *dev){
	struct cm3218_info *lpi = lp_info;

	D("[LS][CM3218] %s\n", __func__);

	if (lpi->als_enable)
		lightsensor_enable(lpi,0);
	}
static const struct dev_pm_ops cm3218_pm_ops = { 
			.suspend = cm3218_suspend,
			.resume  = cm3218_resume,
	};

static const struct i2c_device_id cm3218_i2c_id[] = {
	{CM3218_I2C_NAME, 0},
	{}
};

static struct i2c_driver cm3218_driver = {
	.id_table = cm3218_i2c_id,
	.probe = cm3218_probe,
	.driver = {
		.name  = CM3218_I2C_NAME,
		.owner = THIS_MODULE,
		.pm    =&cm3218_pm_ops
	},
};

static struct cm3218_platform_data cm3218_pdata = 
{
	.intr = OMAP3_CM3218_INT_N,
	.levels = { 0x01, 0x20, 0x70, 0xf0, 0x140,0x500,0xA28, 0x16A8, 0x1F40, 0x2800},
	.power = NULL,
	.ALS_slave_address = CM3218_ALS_cmd,
	.check_interrupt_add = CM3218_check_INI,
	.is_cmd = CM3218_ALS_SM_2 | CM3218_ALS_IT_250ms |CM3218_ALS_PERS_1 | CM3218_ALS_RES_1,
};

static int cm3218_create_i2c_device(void)            //lzk
{
        struct i2c_adapter *adapter;
        struct i2c_client *client;
        struct i2c_board_info info = {
                I2C_BOARD_INFO(CM3218_I2C_NAME, CM3218_ALS_cmd),
               // .type = "gp2ap050als",
               // .addr = 0x39,
              //  .platform_data = &als_i2c_gp2ap050_platform_data,
                .platform_data = &cm3218_pdata,
              //  .irq = OMAP_GPIO_IRQ(OMAP3_CM3218_INT_N),
        };//CM3218 LightSensor.

        adapter = i2c_get_adapter(2);
        if (!adapter) {
                printk(KERN_ERR "---i2c_get_adapter failed\n");
                return -EINVAL;
        }

        client = i2c_new_device(adapter, &info);
        if (!client) {
                printk(KERN_ERR "----i2c_new_device() failed\n");
                i2c_put_adapter(adapter);
                return -EINVAL;
        }
        return 0;
}


static int __init cm3218_init(void)
{
	cm3218_create_i2c_device(); 

	return i2c_add_driver(&cm3218_driver);
}
static void __exit cm3218_exit(void)
{
	i2c_del_driver(&cm3218_driver);
}

late_initcall(cm3218_init);
module_exit(cm3218_exit);

MODULE_DESCRIPTION("CM3218 Driver");
MODULE_LICENSE("GPL");
