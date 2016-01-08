/*
 * Filename: light.c
 *
 * Summary:
 *	DynaImage Ambient light sensor device driver.
 *
 * Modification History:
 * Date     By       Summary
 * -------- -------- ---------------
 * 11/03/14 Rex      Ver 2.0
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/string.h>
//#include <mach/gpio.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/workqueue.h>
#include <linux/timer.h>

#include "di_misc.h"
#include "light.h"

#define LIGHT_INTPUT_DEV_NAME	"di_light"
#define LIGHT_DRIVER_VERSION	"1.0"

#define DEFAULT_TIMER_DELAY 200
#define POLLING_MODE 1


static struct light_data *pLightData = NULL;
static const struct light_reg_s* reg = NULL;
static const struct light_reg_command_s* reg_cmd = NULL;

/*
 * internally used functions
 */

/* SW reset */
/*
static int light_sw_reset(struct light_data *data)
{
	u8 temp = 0;

	if(reg_cmd->sw_rst.max == 0)
	{
		printk("this chip doesn't support %s\n", __func__);
		return -ENODEV;
	}

	FORM_REG_VALUE(temp, 1, reg_cmd->sw_rst);
	if (i2c_single_write(data->client, reg->sys_cfg, temp))
			return -1;
	return 0;
}
*/
/* active */
static int light_set_active(struct light_data *data, int active)
{
	u8 temp = 0;
	pr_info("%s: reg_cmd->active.max = %d, active = %d",__func__,reg_cmd->active.max,active);
	if(reg_cmd->active.max == 0)
	{
		printk("this chip doesn't support %s\n", __func__);
		return -ENODEV;
	}

	if(active > reg_cmd->active.max)
	{
		printk("invalid active is requested: %d\n", active);
		return -EINVAL;
	}
	else
	{	int ret = 0;
		if (ret = i2c_read(data->client, reg->sys_cfg, &temp, 1)) {
			pr_info("%s,%d, ret = %d\n",__func__,__LINE__,ret);
			return -1;
		}
			pr_info("%s,%d\n",__func__,__LINE__);
		//temp &= reg_cmd->active.mask;
		//temp |= (active<<reg_cmd->active.lsb);
		FORM_REG_VALUE(temp, active, reg_cmd->active);

		if (i2c_single_write(data->client, reg->sys_cfg, temp))
			return -1;
	pr_info("%s,%d\n",__func__,__LINE__);
	}
	return 0;
}

/* waiting time */
static int light_set_waiting(struct light_data *data, int waiting)
{
	u8 temp = 0x0;

	if(reg_cmd->waiting.max == 0)
	{
		printk("this chip doesn't support %s\n", __func__);
		return -ENODEV;
	}

	if(waiting > reg_cmd->waiting.max)
	{
		printk("invalid waiting is requested: %d\n", waiting);
		return -EINVAL;
	}
	else
	{
		if (i2c_read(data->client, reg->waiting, &temp, 1))
			return -1;

		FORM_REG_VALUE(temp, waiting, reg_cmd->waiting);

		if (i2c_single_write(data->client, reg->waiting, temp))
			return -1;
	}
	return 0;
}

/* gain */
static int light_set_gain(struct light_data *data, int gain)
{
	u8 temp = 0x0;

	if(reg_cmd->gain.max == 0)
	{
		printk("this chip doesn't support %s\n", __func__);
		return -ENODEV;
	}

	if(gain > reg_cmd->gain.max)
	{
		printk("invalid gain is requested: %d\n", gain);
		return -EINVAL;
	}
	else
	{
		if (i2c_read(data->client, reg->gain_cfg, &temp, 1))
			return -1;

		FORM_REG_VALUE(temp, gain, reg_cmd->gain);

		if (i2c_single_write(data->client, reg->gain_cfg, temp))
			return -1;

		data->gain = gain;
	}
	return 0;
}

/* ext gain */
static int light_set_ext_gain(struct light_data *data, int ext_gain)
{
	u8 temp = 0x0;

	if(reg_cmd->ext_gain.max == 0)
	{
		printk("this chip doesn't support %s\n", __func__);
		return -ENODEV;
	}

	if(ext_gain > reg_cmd->ext_gain.max)
	{
		printk("invalid ext_gain is requested: %d\n", ext_gain);
		return -EINVAL;
	}
	else
	{
		if (i2c_read(data->client, reg->gain_cfg, &temp, 1))
			return -1;

		FORM_REG_VALUE(temp, ext_gain, reg_cmd->ext_gain);

		if (i2c_single_write(data->client, reg->gain_cfg, temp))
			return -1;

		data->ext_gain = ext_gain;
	}
	return 0;
}

/* persist */
static int light_set_persist(struct light_data *data, int persist)
{
	u8 temp = 0x0;

	if(reg_cmd->persist.max == 0)
	{
		printk("this chip doesn't support %s\n", __func__);
		return -ENODEV;
	}

	if(persist > reg_cmd->persist.max)
	{
		printk("invalid persist is requested: %d\n", persist);
		return -EINVAL;
	}
	else
	{
		if (i2c_read(data->client, reg->persist, &temp, 1))
			return -1;

		FORM_REG_VALUE(temp, persist, reg_cmd->persist);

		if (i2c_single_write(data->client, reg->persist, temp))
			return -1;
	}
	return 0;
}

/* mean time */
static int light_set_mean_time(struct light_data *data, int mean)
{
	u8 temp = 0x0;

	if(reg_cmd->mean_time.max == 0)
	{
		printk("this chip doesn't support %s\n", __func__);
		return -ENODEV;
	}

	if(mean > reg_cmd->mean_time.max)
	{
		printk("invalid mean time is requested: %d\n", mean);
		return -EINVAL;
	}
	else
	{
		if (i2c_read(data->client, reg->mean_time, &temp, 1))
			return -1;

		FORM_REG_VALUE(temp, mean, reg_cmd->mean_time);

		if (i2c_single_write(data->client, reg->mean_time, temp))
			return -1;
	}
	return 0;
}

/* dummy */
static int light_set_dummy(struct light_data *data, int dummy)
{
	u8 temp = 0x0;
	if(reg_cmd->dummy.max == 0)
	{
		printk("this chip doesn't support %s\n", __func__);
		return -ENODEV;
	}

	if(dummy > reg_cmd->dummy.max)
	{
		printk("invalid dummy is requested: %d\n", dummy);
		return -EINVAL;
	}
	else
	{
		if (i2c_read(data->client, reg->dummy, &temp, 1))
			return -1;

		FORM_REG_VALUE(temp, dummy, reg_cmd->dummy);

		if (i2c_single_write(data->client, reg->dummy, temp))
			return -1;
	}
	return 0;
}

/* ALS low threshold */
static int light_set_low_thres(struct light_data *data, u16 threshold)
{
	u8 lsb, msb;
	
	msb = threshold >> 8;
	lsb = threshold & 0xFF;

	if (i2c_single_write(data->client, reg->low_thres_h, msb))
		return -1;
	if (i2c_single_write(data->client, reg->low_thres_l, lsb))
		return -1;

	data->low_thres = threshold;

	return 0;
}

/* ALS high threshold */
static int light_set_high_thres(struct light_data *data, u16 threshold)
{
	u8 lsb, msb;
	
	msb = threshold >> 8;
	lsb = threshold & 0xFF;

	if (i2c_single_write(data->client, reg->high_thres_h, msb))
		return -1;
	if (i2c_single_write(data->client, reg->high_thres_l, lsb))
		return -1;

	data->high_thres = threshold;

	return 0;
}

static int light_get_data(struct light_data *data, u16* raw)
{
	u8 byte[2] = { 0 };
	if( i2c_read(data->client, reg->raw_data, byte, 2) )
		return -1;
	*raw = ((u16)byte[1] << 8) | byte[0];
	return 0;
}

static int light_enable(struct light_data *data)
{
	if( light_set_active(data, 1) )
		return -1;

	pr_info("%s,%d\n",__func__,__LINE__);
	atomic_set(&data->enabled, 1);
#if POLLING_MODE	
	schedule_delayed_work(&data->light_work, msecs_to_jiffies(data->poll_interval));
	pr_info("schedule_delayed_work for ALS\n");
#endif
	return 0;
}

static int light_disable(struct light_data *data)
{
	if( light_set_active(data, 0) )
		return -1;

	atomic_set(&data->enabled, 0);
#if POLLING_MODE
	cancel_delayed_work_sync(&data->light_work);
	pr_info("cancel_delayed_work for ALS\n");
#endif
	return 0;
}

static int light_chip_init(struct light_data *data)
{
	int ret = 0;
	//light_sw_reset(data);

	data->cal_gain = 1 << CAL_GAIN_FP_SHIFT;

#ifdef CONFIG_SENSORS_AL3320
	atomic_set(&data->enabled, 0);
	data->gain = 0;
	data->ext_gain = 1;
	data->low_thres = 0x0000;
	data->high_thres = 0xFFFF;
	//set value for register
#endif
#ifdef CONFIG_SENSORS_AP3426
	atomic_set(&data->enabled, 0);
	data->gain = 0;
	data->low_thres = 0x0000;
	data->high_thres = 0xFFFF;
	//set value for register
#endif

	return ret;
}

static int light_input_init(struct light_data *data)
{
	int err;

	printk("allocating light sensor input device\n");
	data->input_dev = input_allocate_device();
	if (!data->input_dev) {
		printk("failed to allocate input device\n");
		err = -ENOMEM;
		goto err0;
	}

	input_set_drvdata(data->input_dev, data);
	data->input_dev->id.bustype = BUS_I2C;
	data->input_dev->id.version    = 1;
	data->input_dev->name = LIGHT_INTPUT_DEV_NAME;
	data->input_dev->dev.parent = &data->client->dev;

	set_bit(EV_ABS, data->input_dev->evbit);
	input_set_abs_params(data->input_dev, ABS_MISC, 0, 8, 0, 0);


	err = input_register_device(data->input_dev);
	if (err) {
		printk("unable to register input polled device %s\n", data->input_dev->name);
		goto err1;
	}

	return 0;

err1:
	input_free_device(data->input_dev);
err0:
	return err;
}

static void light_input_cleanup(struct light_data *data)
{
	input_unregister_device(data->input_dev);
	input_free_device(data->input_dev);
}


//------------------------sysfs--------------------------------------//
/* enable */
static ssize_t light_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct light_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", atomic_read(&data->enabled));
}

static ssize_t light_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct light_data *data = dev_get_drvdata(dev);
	unsigned int value = 0;
	
	value = (unsigned int)simple_strtoul(buf, NULL, 10);
	if(value)
		light_enable(data);
	else
		light_disable(data);

	return count;
}

/* raw data */
static ssize_t light_raw_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct light_data *data = dev_get_drvdata(dev);
	u16 raw;

	light_get_data(data, &raw);

	return sprintf(buf, "%d\n", raw);
}

/* delay */
static ssize_t light_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct light_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->poll_interval);
}

static ssize_t light_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct light_data *data = dev_get_drvdata(dev);
	unsigned int value = 0;
	
	value = (unsigned int)simple_strtoul(buf, NULL, 10);
	data->poll_interval = value;

	return count;
}

/* gain */
static ssize_t light_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct light_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->gain);
}

static ssize_t light_gain_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct light_data *data = dev_get_drvdata(dev);
	unsigned int value = 0;
	
	value = (unsigned int)simple_strtoul(buf, NULL, 10);
	light_set_gain(data, value);

	return count;
}

/* exgain */
static ssize_t light_exgain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct light_data *data = dev_get_drvdata(dev);
#ifdef CONFIG_SENSORS_AP3426
	return sprintf(buf, "doesn't support exgain\n");
#endif	
	return sprintf(buf, "%d\n", data->ext_gain);
}

static ssize_t light_exgain_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct light_data *data = dev_get_drvdata(dev);
	unsigned int value = 0;
#ifdef CONFIG_SENSORS_AP3426
	return -ENODEV;
#endif	
	value = (unsigned int)simple_strtoul(buf, NULL, 10);
	light_set_ext_gain(data, value);

	return count;
}

/* range */
static ssize_t light_range_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct light_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", get_light_range(data));
}

/* ALS low threshold */
static ssize_t light_low_thres_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct light_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->low_thres);
}

static ssize_t light_low_thres_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct light_data *data = dev_get_drvdata(dev);
	unsigned int value = 0;
	
	value = (unsigned int)simple_strtoul(buf, NULL, 10);
	light_set_low_thres(data, value);

	return count;
}

/* ALS high threshold */
static ssize_t light_high_thres_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct light_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->high_thres);
}

static ssize_t light_high_thres_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct light_data *data = dev_get_drvdata(dev);
	unsigned int value = 0;
	
	value = (unsigned int)simple_strtoul(buf, NULL, 10);
	light_set_high_thres(data, value);

	return count;
}

/* calibration */
static ssize_t light_calibration_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct light_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->cal_gain);
}

#define CAL_SAMPLE_PERIOD_MS 	(100)
#define CAL_ACCUMULATE_NUM	(10)
//echo command value > calibration
//command could be "do-cal" or "set-gain"
static ssize_t light_calibration_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct light_data *data = dev_get_drvdata(dev);
	unsigned int value = 0;
	char command[20];

	int i=0;
	u32 sum = 0;
	u32 stdls = 0;
	u32 range = get_light_range(data);
	u32 cal_gain = data->cal_gain;

	if( !atomic_read(&data->enabled) )
	{
		printk("light sensor calibration is failed, enable chip\n");
		return -ENODEV;
	}

	sscanf(buf, "%s %d", command, &value);
	if(!strcmp(command, "do-cal"))
	{
		//value means the strength of standard light source in lux
		if( value >= range )
		{
			printk("strength of standard light source is out of range(0~%d lux)\n", range);
			return -EINVAL;
		}

		//stdls means the the strength of standard light source in count
		//max value of stdls is 2^16-1;
		stdls = ((u32)value << 16) / range;

		for(i=0; i<CAL_ACCUMULATE_NUM; i++)
		{
			u16 raw_data;
			msleep(CAL_SAMPLE_PERIOD_MS);
			if( light_get_data(data, &raw_data) < 0 )
				break;		
			sum += raw_data;
		}

		if(i<CAL_ACCUMULATE_NUM) {
			printk(buf, "light sensor calibration is failed\n");
			return -EIO;
		}

		if((CAL_ACCUMULATE_NUM==0) || (sum==0))
		{
			printk(buf, "divide zero, CAL_ACCUMULATE_NUM = %d, sum =%d\n", CAL_ACCUMULATE_NUM, sum);
			return -EINVAL;
		}
		//max value of (stdls << CAL_GAIN_FP_SHIFT) is 2^32-1
		cal_gain = (stdls << CAL_GAIN_FP_SHIFT) / (sum/CAL_ACCUMULATE_NUM);
		if(cal_gain > (MAX_CAL_GAIN<<CAL_GAIN_FP_SHIFT)) {
			printk(buf, "light sensor cal_gain is too large, more than %d\n", MAX_CAL_GAIN);
			return -EINVAL;
		}
		else
			data->cal_gain = cal_gain;
	}
	else if(!strcmp(command, "set-gain"))
	{
		data->cal_gain = value;
	}
	else
	{
		printk("error command for calibration");
		return -EINVAL;
	}

	return count;
}

/* reg_dump */
static ssize_t light_reg_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct light_data *data = dev_get_drvdata(dev);
	char byte = 0x0;
	ssize_t bytes_printed = 0;
	unsigned char* reg_addr = (unsigned char*)reg;

	do
	{
		if(*reg_addr == 0xFF)
			continue;

		i2c_read(data->client, *reg_addr, &byte, 1);
		bytes_printed += sprintf(buf + bytes_printed, "%#2x: %#2x\n",
					 *reg_addr, byte);
		//reg_addr++;
		if(*reg_addr >= reg->addr_bound)
			break;
	}while(reg_addr++);

	return bytes_printed;
}

static DEVICE_ATTR(enable, 0666, light_enable_show, light_enable_store);
static DEVICE_ATTR(raw_data, 0444, light_raw_data_show, NULL);
static DEVICE_ATTR(delay, 0666, light_delay_show, light_delay_store);
static DEVICE_ATTR(gain, 0666, light_gain_show, light_gain_store);
static DEVICE_ATTR(exgain, 0666, light_exgain_show, light_exgain_store);
static DEVICE_ATTR(range, 0444, light_range_show, NULL);
static DEVICE_ATTR(low_thres, 0666, light_low_thres_show, light_low_thres_store);
static DEVICE_ATTR(high_thres, 0666, light_high_thres_show, light_high_thres_store);
static DEVICE_ATTR(calibration, 0666, light_calibration_show, light_calibration_store);
static DEVICE_ATTR(reg_dump, 0444, light_reg_dump_show, NULL);

static struct device_attribute *light_attributes[] = {
	&dev_attr_enable,
	&dev_attr_raw_data,
	&dev_attr_delay,
	&dev_attr_gain,
	&dev_attr_exgain,
	&dev_attr_range,
	&dev_attr_low_thres,
	&dev_attr_high_thres,
	&dev_attr_calibration,
	&dev_attr_reg_dump,
	NULL
};

static int create_device_attributes(struct device *dev,
	struct device_attribute **attrs)
{
	int i;
	int err = 0;
	for (i = 0 ; NULL != attrs[i] ; ++i) {
		err = sysfs_create_file(&dev->kobj, &attrs[i]->attr);
		if (err != 0)
			break;
	}
	if (err != 0) {
		for (; i >= 0 ; --i)
			sysfs_remove_file(&dev->kobj, &attrs[i]->attr);
	}
	return err;
}

static dev_t const device_dev_t = MKDEV(MISC_MAJOR, 252);

static int create_sysfs_interfaces(struct light_data *data)
{
	int result = 0;

	data->class = get_DI_class();//class_create(THIS_MODULE, "DynaImage");
	if (IS_ERR(data->class)) {
		result = PTR_ERR(data->class);
		goto exit_nullify_class;
	}

	data->dev = device_create(data->class, &data->client->dev, device_dev_t, data, "light");
	if (IS_ERR(data->dev)) {
		result = PTR_ERR(data->dev);
		goto exit_destroy_class;
	}

	result = create_device_attributes( data->dev, light_attributes);
	if (result < 0)
		goto exit_destroy_device;

	return 0;

exit_destroy_device:
	device_destroy(data->class, device_dev_t);
exit_destroy_class:
	data->dev = NULL;
	class_destroy(data->class);
exit_nullify_class:
	data->class = NULL;
	return result;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void light_early_suspend(struct early_suspend *es)
{	
	struct light_data *data = container_of(es, struct light_data, early_suspend);
	printk("%s\n", __func__);

	if ( (data != NULL) && atomic_read(&data->enabled) ) 
	{
		if (light_set_active(data, 0) < 0) //use light_set_active(), we wouldn't like to change the "enabled" flag 
			printk("early_suspend is failed\n");
		else
			printk("light is inactivated\n");
	}
}

static void light_late_resume(struct early_suspend *es)
{	
	struct light_data *data = container_of(es, struct light_data, early_suspend);
	printk("%s\n", __func__);

	if ( (data != NULL) && atomic_read(&data->enabled) ) 
	{
		if (light_set_active(data, 1) < 0) //use light_set_active(), we wouldn't like to change the "enabled" flag 
			printk("late_resume is failed\n");
		else
			printk("light is activated\n");
	}
}
#endif


#if POLLING_MODE
static void light_work_function(struct work_struct *work)
#else
static irqreturn_t light_work_function(int irq, void *dev_id)
#endif
{
	struct light_data *data;
	u16 light_raw;
#if POLLING_MODE
	pr_info("%s",__func__);
	data = container_of((struct delayed_work*)work, struct light_data, light_work);
#else
	data = (struct light_data *)dev_id;
#endif

	if( light_get_data(data, &light_raw) ) {
		printk("%s: get_ALS_data failed\n", __func__);
	}
	else {
		u16 calibrated_light_data;
		calibrated_light_data = ((u64)light_raw*data->cal_gain) >> CAL_GAIN_FP_SHIFT;
		input_report_abs(data->input_dev, ABS_MISC, calibrated_light_data);
		input_sync(data->input_dev);
	}

#if POLLING_MODE
	schedule_delayed_work(&data->light_work, msecs_to_jiffies(data->poll_interval));
#else
	return IRQ_HANDLED;
#endif
}

//static int __devinit light_probe(struct i2c_client *client, const struct i2c_device_id *id)
int light_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct light_data *data;
    	int err = 0;

	printk("%s\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{
		printk("client not i2c capable\n");
		err = -ENODEV;
		goto exit_no_free;
	}

	data = kzalloc(sizeof(struct light_data), GFP_KERNEL);
	if (data == NULL) 
	{
		err = -ENOMEM;
		printk("failed to allocate memory for module data: %d\n", err);
		goto exit_no_free;
	}

	//i2c_set_clientdata(client, data);
	pLightData = data;
	data->client = client;

	pr_info("%s,%d, client->addr=%d\n",__func__,__LINE__,client->addr);
	reg = get_light_reg();
	reg_cmd = get_light_reg_command();
#if DEBUG
	u8 reg_val = 0x0;
        reg_val = i2c_smbus_read_bytei_data(client,reg->dummy);
        pr_info("%s,%d, reg_val(%d) = %d",__func__,__LINE__,reg->dummy,reg_val); 
#endif
	data->poll_interval = DEFAULT_TIMER_DELAY;

	mutex_init(&data->lock);

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.suspend = light_early_suspend;
	data->early_suspend.resume  = light_late_resume;
	data->early_suspend.level   = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	register_early_suspend(&data->early_suspend);
#endif

#if POLLING_MODE
	data->irq = 0;
	INIT_DELAYED_WORK(&data->light_work, light_work_function);
#else
	data->irq = client->irq;
	if(data->irq <= 0) {
		dev_err(&client->dev, "not assign irq number\n");
		goto exit_kfree;
	}
	err = request_threaded_irq(data->irq, NULL, light_work_function, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "light_irq", (void*)data);
	if (err) {
		dev_err(&client->dev, "ret: %d, could not get IRQ %d\n",err,data->irq);
		goto exit_kfree;
	}
#endif

	/* initialize light sensor */
	err = light_chip_init(data);
	if (err) {
		dev_err(&client->dev, "failed to initialize sensor\n");
#if !POLLING_MODE
		goto exit_free_irq;
#else
		goto exit_kfree;
#endif
	}

    	err = light_input_init(data);
    	if (err) {
		dev_err(&client->dev, "failed to create input device\n");
#if !POLLING_MODE
		goto exit_free_irq;
#else
		goto exit_kfree;
#endif
    	}

	err = create_sysfs_interfaces(data);
	if (err) {
		dev_err(&client->dev, "failed to create sysfs\n");
		goto exit_free_input_device;
	}

	dev_info(&client->dev, "DI light is probed successfully (ver %s)\n", LIGHT_DRIVER_VERSION);

	return 0;

exit_free_input_device:
	light_input_cleanup(data);
#if !POLLING_MODE
exit_free_irq:
	if(data->irq > 0)
		free_irq(data->irq, data);
#endif
exit_kfree:
	//i2c_set_clientdata(client, NULL);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
	kfree(data);
exit_no_free:
	dev_info(&client->dev, "%s is failed\n", __func__);
	return err;
}

int light_remove(struct i2c_client *client)
{
	if(pLightData != NULL) {
#if !POLLING_MODE
		if(pLightData->irq)
			free_irq(pLightData->irq, pLightData);
#endif
		light_input_cleanup(pLightData);
#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&pLightData->early_suspend);
#endif
		kfree(pLightData);
	}
	return 0;
}
