/* drivers/input/touchscreen/ftxxxx_ts.c
*
* FocalTech ftxxxx TouchScreen driver.
*
* Copyright (c) 2014  Focaltech Ltd.
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

#include <linux/hrtimer.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/input/mt.h>
#include "ftxxxx_ts.h"
#include <linux/switch.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_ACPI
#include <linux/acpi.h>
#endif


#define SYSFS_DEBUG
#define FTS_APK_DEBUG
#define FTS_CTL_IIC
//#define FTS_FW_AUTO_UPGRADE

//#define FTS_GESTRUE
/*#define FTXXXX_ENABLE_IRQ*/

//#define CONFIG_PM

#ifdef FTS_CTL_IIC
#include "focaltech_ctl.h"
#endif

#ifdef FTS_GESTRUE
/*zax 20140922*/
#define KEY_GESTURE_U		KEY_U
#define KEY_GESTURE_UP		KEY_UP
#define KEY_GESTURE_DOWN		KEY_DOWN
#define KEY_GESTURE_LEFT		KEY_LEFT
#define KEY_GESTURE_RIGHT		KEY_RIGHT
#define KEY_GESTURE_O		KEY_O
#define KEY_GESTURE_E		KEY_E
#define KEY_GESTURE_M		KEY_M
#define KEY_GESTURE_L		KEY_L
#define KEY_GESTURE_W		KEY_W
#define KEY_GESTURE_S		KEY_S
#define KEY_GESTURE_V		KEY_V
#define KEY_GESTURE_Z		KEY_Z

#define GESTURE_LEFT		0x20
#define GESTURE_RIGHT		0x21
#define GESTURE_UP			0x22
#define GESTURE_DOWN		0x23
#define GESTURE_DOUBLECLICK	0x24
#define GESTURE_O			0x30
#define GESTURE_W			0x31
#define GESTURE_M			0x32
#define GESTURE_E			0x33
#define GESTURE_L			0x44
#define GESTURE_S			0x46
#define GESTURE_V			0x54
#define GESTURE_Z			0x41

#define FTS_GESTRUE_POINTS 255
#define FTS_GESTRUE_POINTS_ONETIME 62
#define FTS_GESTRUE_POINTS_HEADER 8
#define FTS_GESTURE_OUTPUT_ADRESS 0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH 4

short pointnum = 0;
unsigned short coordinate_x[150] = {0};
unsigned short coordinate_y[150] = {0};

int gestrue_id = 0;
#endif

#ifdef SYSFS_DEBUG
#include "ftxxxx_ex_fun.h"
#endif

int focal_init_success = 0;
unsigned char IC_FW;
#define REPORT_TOUCH_DEBUG 0
#ifdef CONFIG_PM
static int ftxxxx_ts_suspend(struct device *dev);
static int ftxxxx_ts_resume(struct device *dev);
#endif

#define TPD_MAX_POINTS_2                        2
#define TPD_MAX_POINTS_5                        5
#define TPD_MAX_POINTS_10                        10
#define AUTO_CLB_NEED                             1
#define AUTO_CLB_NONEED                          0
struct Upgrade_Info fts_updateinfo[] =
{
       {0x55,"FT5x06",TPD_MAX_POINTS_10,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 10, 2000},
       {0x08,"FT5606",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 10, 0x79, 0x06, 100, 2000},
	{0x0a,"FT5x16",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x07, 10, 1500},
	{0x06,"FT6x06",TPD_MAX_POINTS_2,AUTO_CLB_NONEED,100, 30, 0x79, 0x08, 10, 2000},
	{0x36,"FT6x36",TPD_MAX_POINTS_2,AUTO_CLB_NONEED,10, 10, 0x79, 0x18, 10, 2000},
	{0x55,"FT5x06i",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 10, 2000},
	{0x14,"FT5336",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
	{0x13,"FT3316",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
	{0x12,"FT5436i",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
	{0x11,"FT5336i",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
	{0x54,"FT5x46",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,2, 2, 0x54, 0x2c, 20, 2000},
	{0x58,"FT5822",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,2, 2, 0x58, 0x2c, 20, 2000},//"FT5822",

};
struct Upgrade_Info fts_updateinfo_curr;
struct ftxxxx_ts_data *ftxxxx_ts;
//static bool touch_down_up_status;
struct switch_dev focal_sdev;
#define TOUCH_MAX_X						800
#define TOUCH_MAX_Y						1280

#define FTXXXX_RESET_PIN_NAME	"ftxxxx-rst"

#define FT_RST_PORT 366
#define FT_INT_PORT	360

#ifdef CONFIG_PLATFORM_DEVICE_PM_VIRT
struct device_state_pm_state ftxxxx_pm_states[] = {
	{.name = "disable", }, /* D3 */
	{.name = "enable", }, /* D0 */
};

#define FTXXXX_STATE_D0		1
#define FTXXXX_STATE_D3		0

/* Touchscreen PM states & class */
static int ftxxxx_set_pm_state(struct device *dev,
		struct device_state_pm_state *state)
{
	struct ftxxxx_ts_platform_data *ftxxxx_pdata = dev->platform_data;
	int id = device_state_pm_get_state_id(dev, state->name);
	int ret = 0;

	switch (id) {
	case FTXXXX_STATE_D0:
		if (ftxxxx_pdata->power)
			ret = regulator_enable(ftxxxx_pdata->power);
		if (ret)
			return ret;
		mdelay(50);

		if (ftxxxx_pdata->power2)
			ret = regulator_enable(ftxxxx_pdata->power2);
		if (ret)
			return ret;
		mdelay(50);

		break;

	case FTXXXX_STATE_D3:
		if (ftxxxx_pdata->power)
			regulator_disable(ftxxxx_pdata->power);

		if (ftxxxx_pdata->power2)
			regulator_disable(ftxxxx_pdata->power2);

		break;

	default:
		return id;
	}

	return 0;
}

static struct device_state_pm_state *ftxxxx_get_initial_state(
		struct device *dev)
{
	return &ftxxxx_pm_states[FTXXXX_STATE_D3];
}

static struct device_state_pm_ops ftxxxx_pm_ops = {
	.set_state = ftxxxx_set_pm_state,
	.get_initial_state = ftxxxx_get_initial_state,
};

DECLARE_DEVICE_STATE_PM_CLASS(ftxxxx);
#endif

#ifdef CONFIG_OF

#define OF_FTXXXX_SUPPLY	"i2c"
#define OF_FTXXXX_SUPPLY_A	"i2ca"
#define OF_FTXXXX_PIN_RESET	"intel,ts-gpio-reset"

static struct ftxxxx_ts_platform_data *ftxxxx_ts_of_get_platdata(
		struct i2c_client *client)
{
	struct device_node *np = client->dev.of_node;
	struct ftxxxx_ts_platform_data *ftxxxx_pdate;
	struct regulator *pow_reg;
	int ret;

	ftxxxx_pdate = devm_kzalloc(&client->dev,
			sizeof(*ftxxxx_pdate), GFP_KERNEL);
	if (!ftxxxx_pdate)
		return ERR_PTR(-ENOMEM);

	/* regulator */
	pow_reg = regulator_get(&client->dev, OF_FTXXXX_SUPPLY);
	ftxxxx_pdate->power = pow_reg;
	if (IS_ERR(pow_reg)) {
		dev_warn(&client->dev, "%s can't get %s-supply handle\n",
			np->name, OF_FTXXXX_SUPPLY);
		ftxxxx_pdate->power = NULL;
	}

	pow_reg = regulator_get(&client->dev, OF_FTXXXX_SUPPLY_A);
	ftxxxx_pdate->power2 = pow_reg;
	if (IS_ERR(pow_reg)) {
		dev_warn(&client->dev, "%s can't get %s-supply handle\n",
			np->name, OF_FTXXXX_SUPPLY_A);
		ftxxxx_pdate->power2 = NULL;
	}

	/* pinctrl */
	ftxxxx_pdate->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(ftxxxx_pdate->pinctrl)) {
		ret = PTR_ERR(ftxxxx_pdate->pinctrl);
		goto out;
	}

	ftxxxx_pdate->pins_default = pinctrl_lookup_state(
			ftxxxx_pdate->pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(ftxxxx_pdate->pins_default))
		dev_err(&client->dev, "could not get default pinstate\n");

	ftxxxx_pdate->pins_sleep = pinctrl_lookup_state(
			ftxxxx_pdate->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR(ftxxxx_pdate->pins_sleep))
		dev_err(&client->dev, "could not get sleep pinstate\n");

	ftxxxx_pdate->pins_inactive = pinctrl_lookup_state(
			ftxxxx_pdate->pinctrl, "inactive");
	if (IS_ERR(ftxxxx_pdate->pins_inactive))
		dev_err(&client->dev, "could not get inactive pinstate\n");

	/* gpio reset */
	ftxxxx_pdate->reset_pin = of_get_named_gpio_flags(client->dev.of_node,
			OF_FTXXXX_PIN_RESET, 0, NULL);
	if (ftxxxx_pdate->reset_pin <= 0) {
		dev_err(&client->dev,
			"error getting gpio for %s\n", OF_FTXXXX_PIN_RESET);
		ret = -EINVAL;
		goto out;
	}

	/* interrupt mode */
	if (of_property_read_bool(np, "intel,polling-mode"))
		ftxxxx_pdate->polling_mode = true;

	/* pm */
	ftxxxx_pdate->pm_platdata = of_device_state_pm_setup(np);
	if (IS_ERR(ftxxxx_pdate->pm_platdata)) {
		dev_warn(&client->dev, "Error during device state pm init\n");
		ret = PTR_ERR(ftxxxx_pdate->pm_platdata);
		goto out;
	}

	of_property_read_u32(np, "intel,x_pos_max",
			&ftxxxx_pdate->x_pos_max);
	of_property_read_u32(np, "intel,x_pos_min",
			&ftxxxx_pdate->x_pos_min);
	of_property_read_u32(np, "intel,y_pos_max",
			&ftxxxx_pdate->y_pos_max);
	of_property_read_u32(np, "intel,y_pos_min",
			&ftxxxx_pdate->y_pos_min);
	of_property_read_u32(np, "intel,screen_max_x",
			&ftxxxx_pdate->screen_max_x);
	of_property_read_u32(np, "intel,screen_max_y",
			&ftxxxx_pdate->screen_max_y);
	of_property_read_u32(np, "intel,key_y",
			&ftxxxx_pdate->key_y);

	ret = of_property_read_u32(np, "intel,key_home",
			&ftxxxx_pdate->key_x[KEY_HOME_INDEX]);
	if (!ret)
		dev_info(&client->dev, "home key x:%d\n",
				ftxxxx_pdate->key_x[KEY_HOME_INDEX]);

	ret = of_property_read_u32(np, "intel,key_back",
			&ftxxxx_pdate->key_x[KEY_BACK_INDEX]);
	if (!ret)
		dev_info(&client->dev, "back key x:%d\n",
				ftxxxx_pdate->key_x[KEY_BACK_INDEX]);

	ret = of_property_read_u32(np, "intel,key_menu",
			&ftxxxx_pdate->key_x[KEY_MENU_INDEX]);
	if (!ret)
		dev_info(&client->dev, "menu key x:%d\n",
				ftxxxx_pdate->key_x[KEY_MENU_INDEX]);

	ret = of_property_read_u32(np, "intel,key_search",
			&ftxxxx_pdate->key_x[KEY_SEARCH_INDEX]);
	if (!ret)
		dev_info(&client->dev, "search key x:%d\n",
				ftxxxx_pdate->key_x[KEY_SEARCH_INDEX]);

	return ftxxxx_pdate;

out:
	return ERR_PTR(ret);
}
#endif

static struct ftxxxx_ts_platform_data *ftxxxx_ts_of_get_platdata(
		struct i2c_client *client)
{
//	struct device_node *np = client->dev.of_node;
	struct ftxxxx_ts_platform_data *ftxxxx_pdate;
//	struct regulator *pow_reg;
//	int ret=0;

	ftxxxx_pdate = devm_kzalloc(&client->dev,
			sizeof(*ftxxxx_pdate), GFP_KERNEL);
	if (!ftxxxx_pdate)
		return ERR_PTR(-ENOMEM);

	// /* regulator */
	// pow_reg = regulator_get(&client->dev, OF_FTXXXX_SUPPLY);
	// ftxxxx_pdate->power = pow_reg;
	// if (IS_ERR(pow_reg)) {
		// dev_warn(&client->dev, "%s can't get %s-supply handle\n",
			// np->name, OF_FTXXXX_SUPPLY);
		// ftxxxx_pdate->power = NULL;
	// }

	// pow_reg = regulator_get(&client->dev, OF_FTXXXX_SUPPLY_A);
	// ftxxxx_pdate->power2 = pow_reg;
	// if (IS_ERR(pow_reg)) {
		// dev_warn(&client->dev, "%s can't get %s-supply handle\n",
			// np->name, OF_FTXXXX_SUPPLY_A);
		// ftxxxx_pdate->power2 = NULL;
	// }

	// /* pinctrl */
	// ftxxxx_pdate->pinctrl = devm_pinctrl_get(&client->dev);
	// if (IS_ERR(ftxxxx_pdate->pinctrl)) {
		// ret = PTR_ERR(ftxxxx_pdate->pinctrl);
		// goto out;
	// }

	// ftxxxx_pdate->pins_default = pinctrl_lookup_state(
			// ftxxxx_pdate->pinctrl, PINCTRL_STATE_DEFAULT);
	// if (IS_ERR(ftxxxx_pdate->pins_default))
		// dev_err(&client->dev, "could not get default pinstate\n");

	// ftxxxx_pdate->pins_sleep = pinctrl_lookup_state(
			// ftxxxx_pdate->pinctrl, PINCTRL_STATE_SLEEP);
	// if (IS_ERR(ftxxxx_pdate->pins_sleep))
		// dev_err(&client->dev, "could not get sleep pinstate\n");

	// ftxxxx_pdate->pins_inactive = pinctrl_lookup_state(
			// ftxxxx_pdate->pinctrl, "inactive");
	// if (IS_ERR(ftxxxx_pdate->pins_inactive))
		// dev_err(&client->dev, "could not get inactive pinstate\n");

	// /* gpio reset */
	// ftxxxx_pdate->reset_pin = of_get_named_gpio_flags(client->dev.of_node,
			// OF_FTXXXX_PIN_RESET, 0, NULL);
	// if (ftxxxx_pdate->reset_pin <= 0) {
		// dev_err(&client->dev,
			// "error getting gpio for %s\n", OF_FTXXXX_PIN_RESET);
		// ret = -EINVAL;
		// goto out;
	// }

	// /* interrupt mode */
	// if (of_property_read_bool(np, "intel,polling-mode"))
		// ftxxxx_pdate->polling_mode = true;

	// /* pm */
	// ftxxxx_pdate->pm_platdata = of_device_state_pm_setup(np);
	// if (IS_ERR(ftxxxx_pdate->pm_platdata)) {
		// dev_warn(&client->dev, "Error during device state pm init\n");
		// ret = PTR_ERR(ftxxxx_pdate->pm_platdata);
		// goto out;
	// }

	// of_property_read_u32(np, "intel,x_pos_max",
			// &ftxxxx_pdate->x_pos_max);
	// of_property_read_u32(np, "intel,x_pos_min",
			// &ftxxxx_pdate->x_pos_min);
	// of_property_read_u32(np, "intel,y_pos_max",
			// &ftxxxx_pdate->y_pos_max);
	// of_property_read_u32(np, "intel,y_pos_min",
			// &ftxxxx_pdate->y_pos_min);
	// of_property_read_u32(np, "intel,screen_max_x",
			// &ftxxxx_pdate->screen_max_x);
	// of_property_read_u32(np, "intel,screen_max_y",
			// &ftxxxx_pdate->screen_max_y);
	// of_property_read_u32(np, "intel,key_y",
			// &ftxxxx_pdate->key_y);

	// ret = of_property_read_u32(np, "intel,key_home",
			// &ftxxxx_pdate->key_x[KEY_HOME_INDEX]);
	// if (!ret)
		// dev_info(&client->dev, "home key x:%d\n",
				// ftxxxx_pdate->key_x[KEY_HOME_INDEX]);

	// ret = of_property_read_u32(np, "intel,key_back",
			// &ftxxxx_pdate->key_x[KEY_BACK_INDEX]);
	// if (!ret)
		// dev_info(&client->dev, "back key x:%d\n",
				// ftxxxx_pdate->key_x[KEY_BACK_INDEX]);

	// ret = of_property_read_u32(np, "intel,key_menu",
			// &ftxxxx_pdate->key_x[KEY_MENU_INDEX]);
	// if (!ret)
		// dev_info(&client->dev, "menu key x:%d\n",
				// ftxxxx_pdate->key_x[KEY_MENU_INDEX]);

	// ret = of_property_read_u32(np, "intel,key_search",
			// &ftxxxx_pdate->key_x[KEY_SEARCH_INDEX]);
	// if (!ret)
		// dev_info(&client->dev, "search key x:%d\n",
				// ftxxxx_pdate->key_x[KEY_SEARCH_INDEX]);
	
	
	ftxxxx_pdate->irq_pin = FT_INT_PORT;
	ftxxxx_pdate->reset_pin = FT_RST_PORT;
	ftxxxx_pdate->key_y = 1500;
	ftxxxx_pdate->key_x[KEY_HOME_INDEX] = 400;
	ftxxxx_pdate->key_x[KEY_BACK_INDEX] = 600;
	ftxxxx_pdate->key_x[KEY_MENU_INDEX] = 200;
	ftxxxx_pdate->key_x[KEY_SEARCH_INDEX] = 0;
	
	return ftxxxx_pdate;

}

/* static inline int ftxxxx_set_pinctrl_state(struct device *dev,
		struct pinctrl_state *state)
{
	struct ftxxxx_ts_platform_data *pdata = dev_get_platdata(dev);
	int ret = 0;

	if (!IS_ERR(state)) {
		ret = pinctrl_select_state(pdata->pinctrl, state);
		if (ret)
			dev_err(dev, "%d:could not set pins\n", __LINE__);
	}

	return ret;
} */

static int ftxxxx_ts_power_off(struct i2c_client *client)
{
/* 	struct ftxxxx_ts_platform_data *ftxxxx_pdate =
		client->dev.platform_data;*/
	int ret = 0; 

	disable_irq(client->irq);

/* 	if (ftxxxx_pdate->pins_sleep)
		ftxxxx_set_pinctrl_state(&client->dev,
				ftxxxx_pdate->pins_sleep);

	if (ftxxxx_pdate->pm_platdata &&
			ftxxxx_pdate->pm_platdata->pm_state_D3_name) {
		ret = device_state_pm_set_state_by_name(&client->dev,
				ftxxxx_pdate->pm_platdata->pm_state_D3_name);
	} */

	return ret;
}

static int ftxxxx_ts_power_on(struct i2c_client *client)
{
/* 	struct ftxxxx_ts_platform_data *ftxxxx_pdate =
		client->dev.platform_data; */
	int ret = 0;

/* 	if (ftxxxx_pdate->pins_default)
		ret = ftxxxx_set_pinctrl_state(&client->dev,
					ftxxxx_pdate->pins_default);

	if (ftxxxx_pdate->pm_platdata &&
			ftxxxx_pdate->pm_platdata->pm_state_D0_name)
		ret = device_state_pm_set_state_by_name(&client->dev,
			ftxxxx_pdate->pm_platdata->pm_state_D0_name); */

	if (!ret) 
		enable_irq(client->irq);

	return ret;
}
///////////////////////////////////////////////////


/*
*ftxxxx_i2c_Read-read data and write data by i2c
*@client: handle of i2c
*@writebuf: Data that will be written to the slave
*@writelen: How many bytes to write
*@readbuf: Where to store data read from slave
*@readlen: How many bytes to read
*
*Returns negative errno, else the number of messages executed
*
*
*/
static DEFINE_MUTEX(i2c_rw_access);
int ftxxxx_i2c_Read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen)
{
	int ret;
	mutex_lock(&i2c_rw_access);
	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
				.addr = client->addr,
				.flags = 0,
				.len = writelen,
				.buf = writebuf,
			},
			{
				.addr = client->addr,
				.flags = I2C_M_RD,
				.len = readlen,
				.buf = readbuf,
			},
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			pr_err("f%s: i2c read error. error code = %d \n", __func__, ret);
	} else {
		struct i2c_msg msgs[] = {
			{
				.addr = client->addr,
				.flags = I2C_M_RD,
				.len = readlen,
				.buf = readbuf,
			},
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			pr_err( "%s:i2c read error.  error code = %d \n", __func__, ret);
	}
	mutex_unlock(&i2c_rw_access);
	return ret;
}
/*write data by i2c*/
int ftxxxx_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;
	
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = writelen,
			.buf = writebuf,
		},
	};
	mutex_lock(&i2c_rw_access);
	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s i2c write error.\n", __func__);
	mutex_unlock(&i2c_rw_access);
	return ret;
}

/* Jacob : add for creating virtual_key_maps +++*/
#ifdef SYSFS_DEBUG
#define MAX_LEN		200
#ifdef VIRTUAL_KEYS
static ssize_t focalTP_virtual_keys_register(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{

	char *virtual_keys = 	__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":160:2500:30:30" "\n" \

				__stringify(EV_KEY) ":" __stringify(KEY_HOME) ":360:2500:30:30" "\n" \

				__stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":560:2500:30:30" "\n" ;

	return snprintf(buf, strnlen(virtual_keys, MAX_LEN) + 1 , "%s",	virtual_keys);

}

static struct kobj_attribute focalTP_virtual_keys_attr = {

	.attr = {
		.name = "virtualkeys.focal-touchscreen",
		.mode = S_IRWXUGO,
	},

	.show = &focalTP_virtual_keys_register,

};

static struct attribute *virtual_key_properties_attrs[] = {

	&focalTP_virtual_keys_attr.attr,

	NULL

};

static struct attribute_group virtual_key_properties_attr_group = {
	.attrs = virtual_key_properties_attrs,
};

struct kobject *focal_virtual_key_properties_kobj;

/* Jacob : add for creating virtual_key_maps ---*/
#endif

u8 get_focal_tp_fw(void)
{
	u8 fwver = 0;

	if (ftxxxx_read_reg(ftxxxx_ts->client, FTXXXX_REG_FW_VER, &fwver) < 0)
		return -1;
	else
		return fwver;
}

static ssize_t focal_show_tpfwver(struct switch_dev *sdev, char *buf)
{
	int num_read_chars = 0;
	IC_FW = get_focal_tp_fw();

	if (IC_FW == 255)
		num_read_chars = snprintf(buf, PAGE_SIZE, "get tp fw version fail!\n");
	else
		num_read_chars = snprintf(buf, PAGE_SIZE, "%d\n", IC_FW);
	return num_read_chars;
}
#endif

#ifdef FTS_GESTRUE/*zax 20140922*/
static void check_gesture(struct ftxxxx_ts_data *data, int gesture_id)
{

	pr_err("fts gesture_id==0x%x\n ", gesture_id);

	switch (gesture_id) {
	case GESTURE_LEFT:
		input_report_key(data->input_dev, KEY_GESTURE_LEFT, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_LEFT, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_RIGHT:
		input_report_key(data->input_dev, KEY_GESTURE_RIGHT, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_RIGHT, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_UP:
		input_report_key(data->input_dev, KEY_GESTURE_UP, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_UP, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_DOWN:
		input_report_key(data->input_dev, KEY_GESTURE_DOWN, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_DOWN, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_DOUBLECLICK:
		input_report_key(data->input_dev, KEY_GESTURE_U, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_U, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_O:
		input_report_key(data->input_dev, KEY_GESTURE_O, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_O, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_W:
		input_report_key(data->input_dev, KEY_GESTURE_W, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_W, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_M:

		input_report_key(data->input_dev, KEY_GESTURE_M, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_M, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_E:
		input_report_key(data->input_dev, KEY_GESTURE_E, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_E, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_L:
		input_report_key(data->input_dev, KEY_GESTURE_L, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_L, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_S:
		input_report_key(data->input_dev, KEY_GESTURE_S, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_S, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_V:
		input_report_key(data->input_dev, KEY_GESTURE_V, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_V, 0);
		input_sync(data->input_dev);
		break;

	case GESTURE_Z:
		input_report_key(data->input_dev, KEY_GESTURE_Z, 1);
		input_sync(data->input_dev);
		input_report_key(data->input_dev, KEY_GESTURE_Z, 0);
		input_sync(data->input_dev);
		break;

	default:

		break;

	}

}

static int fts_read_Gestruedata(struct ftxxxx_ts_data *data)
{
	unsigned char buf[FTS_GESTRUE_POINTS * 3] = { 0 };
	int ret = -1;
	int i = 0;
	int gestrue_id = 0;
	buf[0] = 0xd3;


	pointnum = 0;

	ret = ftxxxx_i2c_Read(data->client, buf, 1, buf, FTS_GESTRUE_POINTS_HEADER);
				pr_err("tpd read FTS_GESTRUE_POINTS_HEADER.\n");

	if (ret < 0) {
		pr_err("%s read touchdata failed.\n", __func__);
		return ret;
	}

	/* FW */
	if (fts_updateinfo_curr.CHIP_ID==0x54)
	{
			 gestrue_id = buf[0];
		pointnum = (short)(buf[1]) & 0xff;
		buf[0] = 0xd3;

		if ((pointnum * 4 + 8) < 255) {
			ret = ftxxxx_i2c_Read(data->client, buf, 1, buf, (pointnum * 4 + 8));
		} else {
			ret = ftxxxx_i2c_Read(data->client, buf, 1, buf, 255);
			ret = ftxxxx_i2c_Read(data->client, buf, 0, buf+255, (pointnum * 4 + 8) - 255);
		}
		if (ret < 0) {
			pr_err("%s read touchdata failed.\n", __func__);
			return ret;
		}
	check_gesture(data, gestrue_id);
	for (i = 0; i < pointnum; i++) {
		coordinate_x[i] =  (((s16) buf[0 + (4 * i)]) & 0x0F) <<
		8 | (((s16) buf[1 + (4 * i)]) & 0xFF);
		coordinate_y[i] = (((s16) buf[2 + (4 * i)]) & 0x0F) <<
		8 | (((s16) buf[3 + (4 * i)]) & 0xFF);
	}
	return -1;
	}
	
/*
	if (0x24 == buf[0]) {
		gestrue_id = 0x24;
		check_gesture(gestrue_id);
		return -1;
	}

	pointnum = (short)(buf[1]) & 0xff;
	buf[0] = 0xd3;

	if((pointnum * 4 + 8)<255) {
		ret = fts_i2c_Read(i2c_client, buf, 1, buf, (pointnum * 4 + 8));
	}
	else
	{
		 ret = fts_i2c_Read(i2c_client, buf, 1, buf, 255);
		 ret = fts_i2c_Read(i2c_client, buf, 0, buf+255, (pointnum * 4 + 8) -255);
	}
	if (ret < 0)
	{
		pr_err( "%s read touchdata failed.\n", __func__);
		return ret;
	}

	gestrue_id = fetch_object_sample(buf, pointnum);
	check_gesture(gestrue_id);

	for(i = 0;i < pointnum;i++)
	{
		coordinate_x[i] =  (((s16) buf[0 + (4 * i)]) & 0x0F) <<
			8 | (((s16) buf[1 + (4 * i)])& 0xFF);
		coordinate_y[i] = (((s16) buf[2 + (4 * i)]) & 0x0F) <<
			8 | (((s16) buf[3 + (4 * i)]) & 0xFF);
	}
	return -1;*/
}
#endif

static int ftxxxx_is_button(u16 x, u16 y)
{
	int i, key_x;
	struct ftxxxx_ts_platform_data *ftxxxx_pdata =
			ftxxxx_ts->pdata;

	if (y != ftxxxx_pdata->key_y)
		return 0;

	for (i = 0; i < KEY_INDEX_MAX; i++) {
		key_x = ftxxxx_pdata->key_x[i];
		if ((key_x != 0) && (x == key_x))
		{
			pr_err("[FTS] zax button,x = %d, y = %d,android_key[%d]=%d \n", x,y,i,android_key[i]);
			return android_key[i];

		}
	}
	return 0;
}

/*Read touch point information when the interrupt  is asserted.*/
static int ftxxxx_read_Touchdata(struct ftxxxx_ts_data *data)
{
	struct ts_event *event = &data->event;
	u8 buf[POINT_READ_BUF] = { 0 };
	int ret = -1;
	int i = 0;
	u8 pointid = FT_MAX_ID;

	ret = ftxxxx_i2c_Read(data->client, buf, 1, buf, POINT_READ_BUF);
	if (ret < 0) {
		dev_err(&data->client->dev, "[FTS] %s read touchdata failed.\n", __func__);
		return ret;
	}
	#if REPORT_TOUCH_DEBUG	
		for(i=0;i<POINT_READ_BUF;i++)
		{
			printk("\n [fts] zax buf[%d] =(0x%02x)  \n", i,buf[i]);
		}
	#endif
	/*Ft_Printf_Touchdata(data,buf);*/	/*\B4\F2ӡ\B1\A8\B5\E3\B5\F7\CA\D4\D0\C5Ϣ*/

	memset(event, 0, sizeof(struct ts_event));

	event->touch_point = 0;
	event->touch_point_num=buf[2] & 0x0F;
	//printk("[FTS] TPD_MAX_POINTS =%d \n",fts_updateinfo_curr.TPD_MAX_POINTS);
	for (i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++) {
		pointid = (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;
		if (pointid >= FT_MAX_ID)
			break;
		else
			event->touch_point++;
		event->au16_x[i] =
			(((s16) buf[FT_TOUCH_X_H_POS + FT_TOUCH_STEP * i]) & 0x0F) <<
			8 | (((s16) buf[FT_TOUCH_X_L_POS + FT_TOUCH_STEP * i])& 0xFF);
		event->au16_y[i] =
			(((s16) buf[FT_TOUCH_Y_H_POS + FT_TOUCH_STEP * i]) & 0x0F) <<
			8 | (((s16) buf[FT_TOUCH_Y_L_POS + FT_TOUCH_STEP * i]) & 0xFF);
		event->au8_touch_event[i] =
			buf[FT_TOUCH_EVENT_POS + FT_TOUCH_STEP * i] >> 6;
		event->au8_finger_id[i] =
			(buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;
		event->pressure[i] =
			(buf[FT_TOUCH_XY_POS + FT_TOUCH_STEP * i]);
		event->area[i] =
			(buf[FT_TOUCH_MISC + FT_TOUCH_STEP * i]) >> 4;
if((event->au8_touch_event[i]==0 || event->au8_touch_event[i]==2)&&((event->touch_point_num==0)))
			return 1;
		#if REPORT_TOUCH_DEBUG
			printk("\n [FTS] zax data (id= %d ,x=(0x%02x),y= (0x%02x))\n ", event->au8_finger_id[i],event->au16_x[i],event->au16_y[i]);
		#endif

	}

	/*event->pressure = FT_PRESS;*/
	/*event->pressure = 200;*/

	return 0;
}

/*
*report the point information
*/
static int ftxxxx_report_value(struct ftxxxx_ts_data *data)
{	
	struct ts_event *event = &data->event;
	int i,j;
	int up_point=0;
	int key_value;
	int touchs = 0;
	key_value = 0;
	/*protocol B*/
	for (i = 0; i < event->touch_point; i++) {
		if(data->x_max > event->au16_x[i] && data->y_max > event->au16_y[i]) 
		{
			input_mt_slot(data->input_dev, event->au8_finger_id[i]);
	
			if (event->au8_touch_event[i]== 0 || event->au8_touch_event[i] == 2) 
			{
				input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, true);
				input_report_abs(data->input_dev, ABS_MT_PRESSURE, FT_PRESS);//event->pressure[i]);
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, FT_PRESS);//event->area[i]);
				input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->au16_x[i]);
				input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->au16_y[i]);
				touchs |= BIT(event->au8_finger_id[i]);
				data->touchs |= BIT(event->au8_finger_id[i]);
				#if REPORT_TOUCH_DEBUG
					printk("\n [fts] zax down (id=%d ,x=(0x%02x), y=(0x%02x), pres=%d, area=%d) \n", event->au8_finger_id[i],event->au16_x[i],event->au16_y[i],event->pressure[i],event->area[i]);
				 #endif
			} 
			else 
			{
				#if REPORT_TOUCH_DEBUG
				printk("\n [fts] zax up (id=%d ,x=(0x%02x), y=(0x%02x), pres=%d, area=%d) \n", event->au8_finger_id[i],event->au16_x[i],event->au16_y[i],event->pressure[i],event->area[i]);
			 	#endif
				up_point++;
				data->touchs &= ~BIT(event->au8_finger_id[i]);
				input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
			}
		}
		else
		{
			key_value = ftxxxx_is_button(event->au16_x[i], event->au16_y[i]);
			#if REPORT_TOUCH_DEBUG	
				printk("[FTS] zax before Key *id=%d,event=%d,x = (0x%02x), y = (0x%02x) \n", event->au8_finger_id[i],event->au8_touch_event[i],event->au16_x[i], event->au16_y[i]);
			#endif
			if (key_value) 
			{
				if (event->au8_touch_event[i]== 0 || event->au8_touch_event[i] == 2)
				{
					//touchs |= BIT(event->au8_finger_id[i]);
					//data->touchs |= BIT(event->au8_finger_id[i]);
					 for (j = 0; j < CFG_MAX_TOUCH_POINTS; j++) 
		         		{
				            input_mt_slot(data->input_dev, j);
				            input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, 0);
					     #if REPORT_TOUCH_DEBUG	
							printk("\n [FTS] zax release va  \n");
					     #endif

	             			}
					#if REPORT_TOUCH_DEBUG	
						printk("\n [FTS] zax down Key *id=%d,event=%d,x = (0x%02x), y = (0x%02x) \n", event->au8_finger_id[i],event->au8_touch_event[i],event->au16_x[i], event->au16_y[i]);
					#endif
					input_report_key(data->input_dev, key_value, 1);
				}
				else 
				{
					
					//up_point++;
					//data->touchs &= ~BIT(event->au8_finger_id[i]);
					#if REPORT_TOUCH_DEBUG	
						printk("[FTS] zax up Key *id=%d,event=%d,x = (0x%02x), y = (0x%02x) \n", event->au8_finger_id[i],event->au8_touch_event[i],event->au16_x[i], event->au16_y[i]);
					#endif
					input_report_key(data->input_dev, key_value, 0);
				}
			pr_err("[FTS] Touch Key *** x = %d, y = %d ***\n", event->au16_x[i], event->au16_y[i]);
			}
			#if REPORT_TOUCH_DEBUG	
				printk("[FTS] zax after Key *id=%d,event=%d,x = (0x%02x), y = (0x%02x) \n", event->au8_finger_id[i],event->au8_touch_event[i],event->au16_x[i], event->au16_y[i]);
			#endif
			input_sync(data->input_dev);
			return 0;
		}
		
	}
	//if(unlikely(data->touchs ^ touchs)){
		for(i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++)
		{
			if(BIT(i) & (data->touchs ^ touchs))
			{
				up_point++;
                            #if REPORT_TOUCH_DEBUG
					printk("\n [fts] zax add up  id=%d \n", i);
				#endif
                            data->touchs &= ~BIT(i);
				input_mt_slot(data->input_dev, i);
				input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
			}
		}
	//}
        #if REPORT_TOUCH_DEBUG
		printk("\n [fts] zax  touchs=%d, data-touchs=%d, touch_point=%d, up_point=%d \n ", touchs,data->touchs,event->touch_point, up_point);
	#endif
	data->touchs = touchs;
	
	 if(/*(last_touchpoint>0)&&*/(event->touch_point_num==0))    //release all touches in final
	 {	
		for (j = 0; j < fts_updateinfo_curr.TPD_MAX_POINTS; j++) 
		{
			input_mt_slot(data->input_dev, j);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, 0);
		}
		//last_touchpoint=0;
              data->touchs=0;
		up_point=fts_updateinfo_curr.TPD_MAX_POINTS;
		event->touch_point = up_point;
    	} 
	#if REPORT_TOUCH_DEBUG
		printk("\n [fts] zax  touchs=%d, data-touchs=%d, touch_point=%d, up_point=%d \n ", touchs,data->touchs,event->touch_point, up_point);
	#endif

	if(event->touch_point == up_point) 
	{
		input_report_key(data->input_dev, BTN_TOUCH, 0);
		#if REPORT_TOUCH_DEBUG
			printk("\n [fts] zax  final touchs");
		#endif
	} 
	else 
	{
		input_report_key(data->input_dev, BTN_TOUCH, 1);			
	}
	input_sync(data->input_dev);
	return 0;
}

/*The ftxxxx device will signal the host about TRIGGER_FALLING.
*Processed when the interrupt is asserted.
*/
static irqreturn_t ftxxxx_ts_interrupt(int irq, void *dev_id)
{
/*	struct ftxxxx_ts_data *ftxxxx_ts = dev_id;  jacob use globle ftxxxx_ts data*/
	int ret = 0;
	
	disable_irq_nosync(ftxxxx_ts->client->irq);
#ifdef FTS_GESTRUE/*zax 20140922*/
				i2c_smbus_read_i2c_block_data(ftxxxx_ts->client, 0xd0, 1, &state);
				/*pr_err("tpd fts_read_Gestruedata state=%d\n", state);*/
				if (state == 1) {
					fts_read_Gestruedata(ftxxxx_ts);
					/*continue;*/
				} else {
#endif
	ret = ftxxxx_read_Touchdata(ftxxxx_ts);

	if (ret == 0)
		ftxxxx_report_value(ftxxxx_ts);
#ifdef FTS_GESTRUE/*zax 20140922*/
					}
#endif
	enable_irq(ftxxxx_ts->client->irq);
	return IRQ_HANDLED;
}

void ftxxxx_reset_tp(int HighOrLow)
{
	pr_info("[FTS] %s : set tp reset pin to %d\n", __func__, HighOrLow);
	gpio_set_value(FT_RST_PORT, HighOrLow);
}

void ftxxxx_Enable_IRQ(struct i2c_client *client, int enable)
{
	//if (FTXXXX_ENABLE_IRQ == enable)
	//if (FTXXXX_ENABLE_IRQ)
	//enable_irq(client->irq);
	//else
	//disable_irq_nosync(client->irq);
}

static int fts_init_gpio_hw(struct ftxxxx_ts_data *ftxxxx_ts)
{
	int ret = 0;

	ret = gpio_request(FT_RST_PORT, FTXXXX_RESET_PIN_NAME);
	if (ret) {
		pr_err("[FTS][Touch] %s: request GPIO %s for reset failed, ret = %d\n",
			__func__, FTXXXX_RESET_PIN_NAME, ret);
		return ret;
	}
/*
	s3c_gpio_setpull(FTXXXX_RESET_PIN, S3C_GPIO_PULL_NONE);
	* s3c_gpio_cfgpin(FTXXXX_RESET_PIN, S3C_GPIO_SFN(1));//  S3C_GPIO_OUTPUT
	* gpio_set_value(FTXXXX_RESET_PIN,  0);
*/
	ret = gpio_direction_output(FT_RST_PORT, 1);	/* change reset set high*/
	if (ret) {
			pr_err("[FTS][Touch] %s: set %s gpio to out put high failed, ret = %d\n",
				__func__, FTXXXX_RESET_PIN_NAME, ret);
			return ret;
		}
	pr_err("[FTS][Touch] %s: pass ret = %d\n",	__func__, ret);
	return ret;
}

static void fts_un_init_gpio_hw(struct ftxxxx_ts_data *ftxxxx_ts)
{
	gpio_free(FT_RST_PORT);
}
void focaltech_get_upgrade_array(void)
{

	u8 chip_id;
	u32 i;

	i2c_smbus_read_i2c_block_data(ftxxxx_ts->client,FTXXXX_REG_CHIP_ID,1,&chip_id);

	pr_err("[FTS]%s chip_id = %x\n", __func__, chip_id);

	for(i=0;i<sizeof(fts_updateinfo)/sizeof(struct Upgrade_Info);i++)
	{
		if(chip_id==fts_updateinfo[i].CHIP_ID)
		{
			memcpy(&fts_updateinfo_curr, &fts_updateinfo[i], sizeof(struct Upgrade_Info));
			pr_err("[FTS]%s match chip_id = %x\n", __func__, fts_updateinfo[i].CHIP_ID);
			break;
		}
	}

	if(i >= sizeof(fts_updateinfo)/sizeof(struct Upgrade_Info))
	{
		memcpy(&fts_updateinfo_curr, &fts_updateinfo[10], sizeof(struct Upgrade_Info));
		pr_err("[FTS]%s default chip_id = %x, fts_updateinfo_curr chip_id = %x\n", __func__, fts_updateinfo[10].CHIP_ID,fts_updateinfo_curr.CHIP_ID);
	}
}

static int ftxxxx_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
//	struct ftxxxx_ts_platform_data *pdata;
	struct input_dev *input_dev;
	int i;
	int err = 0;
	unsigned char uc_reg_value;
	unsigned char uc_reg_addr;
	pr_err("[FTS][Touch] FTXXXX probe process Start !\n");
	
//	client->addr = 0x38;
	pr_err("[FTS][Touch] FTXXXX hardcode I2C address = 0x%02x \n", client->addr);

/*	
#ifdef CONFIG_OF
	pdata = client->dev.platform_data =
		ftxxxx_ts_of_get_platdata(client);
	if (IS_ERR(pdata)) {
		err = PTR_ERR(pdata);
		return err;
	}
#else
	pdata = client->dev.platform_data;
#endif
*/

//	ftxxxx_set_pinctrl_state(&client->dev, pdata->pins_default);

/* 	if (pdata->pm_platdata) {
		err = device_state_pm_set_class(&client->dev,
			pdata->pm_platdata->pm_user_name);
		if (err) {
			dev_err(&client->dev,
				"Error while setting the pm class\n");
			goto exit_pm_class;
		}

		err = device_state_pm_set_state_by_name(&client->dev,
				pdata->pm_platdata->pm_state_D0_name);
		if (err)
			goto exit_pm_class;
	} */
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
	goto exit_check_functionality_failed;
	}

	ftxxxx_ts = kzalloc(sizeof(struct ftxxxx_ts_data), GFP_KERNEL);
	if (!ftxxxx_ts) {
		err = -ENOMEM;
	goto exit_alloc_data_failed;
	}

	i2c_set_clientdata(client, ftxxxx_ts);

	ftxxxx_ts->client = client;
	ftxxxx_ts->init_success = 0;
	ftxxxx_ts->pdata = ftxxxx_ts_of_get_platdata(client);
	ftxxxx_ts->x_max = TOUCH_MAX_X;
	ftxxxx_ts->y_max = TOUCH_MAX_Y;
	ftxxxx_ts->irq = gpio_to_irq(FT_INT_PORT);
	pr_err("[FTS][Touch] IRQ number 1 = %d \n",ftxxxx_ts->irq);
	ftxxxx_ts->client->irq = gpio_to_irq(FT_INT_PORT);
	pr_err("[FTS][Touch] IRQ number 2 = %d \n",ftxxxx_ts->client->irq);
	
	if (0 >= ftxxxx_ts->x_max)
		ftxxxx_ts->x_max = TOUCH_MAX_X;
	if (0 >= ftxxxx_ts->y_max)
		ftxxxx_ts->y_max = TOUCH_MAX_Y;

	 if (fts_init_gpio_hw(ftxxxx_ts) < 0)
		 goto exit_init_gpio;
	
	err = request_threaded_irq(ftxxxx_ts->client->irq, NULL, ftxxxx_ts_interrupt,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->dev.driver->name,
		ftxxxx_ts);

	if (ftxxxx_ts->client->irq < 0) {
		dev_err(&client->dev, "[FTS][Touch] %s: request irq fail. \n", __func__);
		goto exit_irq_request_failed;
	}

	disable_irq(ftxxxx_ts->client->irq);	/*need mutex protect, should add latter*/
	pr_err("[FTS][Touch] IRQ DISABLED  \n");
	
	input_dev = input_allocate_device();
	pr_err("[FTS][Touch] input_allocate_device done! \n");
	if (!input_dev) {
		err = -ENOMEM;
		pr_err("[FTS][Touch] %s: failed to allocate input device\n", __func__);
		goto exit_input_dev_alloc_failed;
	}
	pr_err("[FTS][Touch] input_allocate_device done 1! \n");
	ftxxxx_ts->input_dev = input_dev;
	pr_err("[FTS][Touch] input_allocate_device done 2! \n");
	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	pr_err("[FTS][Touch] input_allocate_device done 3! \n");
	for (i = 0; i < KEY_INDEX_MAX; i++) {
		if (ftxxxx_ts->pdata->key_x[i])
			set_bit(android_key[i], input_dev->keybit);
			pr_err("[FTS][Touch] set bit key %d = %d! \n",i,android_key[i]);
	}
	pr_err("[FTS][Touch] input_allocate_device done 4! \n");
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	pr_err("[FTS][Touch] set bit done! \n");
	input_mt_init_slots(input_dev, CFG_MAX_TOUCH_POINTS, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 0x31, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ftxxxx_ts->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ftxxxx_ts->y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, PRESS_MAX, 0, 0);

	input_dev->name = Focal_input_dev_name;
	pr_err("[FTS][Touch] before input_register_device! \n");
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev, "[FTS][Touch] %s: failed to register input device: %s\n", __func__, dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
	pr_err("[FTS][Touch] input_register_device done! \n");
	/*make sure CTP already finish startup process */
	msleep(200);
	focaltech_get_upgrade_array();
#ifdef SYSFS_DEBUG
	pr_err("[FTS][Touch] ftxxxx_create_sysfs Start !\n");
	ftxxxx_create_sysfs(client);
	pr_err("[FTS][Touch] ftxxxx_create_sysfs End !\n");

	mutex_init(&ftxxxx_ts->g_device_mutex);

	focal_sdev.name = "touch";
	focal_sdev.print_name = focal_show_tpfwver;

/* 	if(switch_dev_register(&focal_sdev) < 0)
	{
		goto exit_err_sdev_register_fail;
	} 
 */
#ifdef  VIRTUAL_KEYS
	/* Jacob: add for creating virtual_key_maps +++*/

	focal_virtual_key_properties_kobj = kobject_create_and_add("board_properties", NULL);

	if (focal_virtual_key_properties_kobj)

		err = sysfs_create_group(focal_virtual_key_properties_kobj, &virtual_key_properties_attr_group);

	if (!focal_virtual_key_properties_kobj || err)

		pr_err("[Touch_N] failed to create novaTP virtual key map!\n");

	/* Jacob: add for creating virtual_key_maps ---*/
#endif

#endif
#ifdef FTS_CTL_IIC
	if (ft_rw_iic_drv_init(client) < 0)
		dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n", __func__);
#endif
#ifdef FTS_APK_DEBUG
	ftxxxx_create_apk_debug_channel(client);
#endif

#ifdef FTS_FW_AUTO_UPGRADE
	fts_ctpm_auto_upgrade(client);
#endif

	/*get some register information */
	uc_reg_addr = FTXXXX_REG_FW_VER;
	pr_err("[FTS][Touch] get some register information \n");
	err = ftxxxx_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	if (err < 0)
		ftxxxx_ts->init_success = 0;
	else {
		ftxxxx_ts->init_success = 1;
		pr_err("[FTS] Firmware version = 0x%x\n", uc_reg_value);
		IC_FW = uc_reg_value;
		}

	uc_reg_addr = FTXXXX_REG_POINT_RATE;
	
	err = ftxxxx_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	if (err < 0)
		ftxxxx_ts->init_success = 0;
	else {
		ftxxxx_ts->init_success = 1;
		pr_err("[FTS] report rate is %dHz.\n", uc_reg_value * 10);
		}

	uc_reg_addr = FTXXXX_REG_THGROUP;
	err = ftxxxx_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	if (err < 0)
		ftxxxx_ts->init_success = 0;
	else {
		ftxxxx_ts->init_success = 1;
		pr_err("[FTS] touch threshold is %d.\n", uc_reg_value * 4);
		}
		
	uc_reg_addr = FTXXXX_REG_VENDOR_ID;
	err = ftxxxx_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	if (err < 0)
		ftxxxx_ts->init_success = 0;
	else {
		ftxxxx_ts->init_success = 1;
		pr_err("[FTS] VENDOR ID = 0x%x\n", uc_reg_value);
		}


#ifdef FTS_GESTRUE	/*zax 20140922*/
	/*init_para(720,1280,100,0,0);*/

	/*auc_i2c_write_buf[0] = 0xd0;*/
	/*auc_i2c_write_buf[1] = 0x01;*/
	/*ftxxxx_i2c_Write(client, auc_i2c_write_buf, 2);	//let fw open gestrue function*/

	/*auc_i2c_write_buf[0] = 0xd1;*/
	/*auc_i2c_write_buf[1] = 0xff;*/
	/*ftxxxx_i2c_Write(client, auc_i2c_write_buf, 2);*/
	/*
	auc_i2c_write_buf[0] = 0xd0;
	auc_i2c_write_buf[1] = 0x00;
	ftxxxx_i2c_Write(client, auc_i2c_write_buf, 2);	//let fw close gestrue function 
	*/
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_U);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_UP);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_DOWN);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_LEFT);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_RIGHT);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_O);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_E);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_M);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_L);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_W);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_S);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_V);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_Z);

	__set_bit(KEY_GESTURE_RIGHT, input_dev->keybit);
	__set_bit(KEY_GESTURE_LEFT, input_dev->keybit);
	__set_bit(KEY_GESTURE_UP, input_dev->keybit);
	__set_bit(KEY_GESTURE_DOWN, input_dev->keybit);
	__set_bit(KEY_GESTURE_U, input_dev->keybit);
	__set_bit(KEY_GESTURE_O, input_dev->keybit);
	__set_bit(KEY_GESTURE_E, input_dev->keybit);
	__set_bit(KEY_GESTURE_M, input_dev->keybit);
	__set_bit(KEY_GESTURE_W, input_dev->keybit);
	__set_bit(KEY_GESTURE_L, input_dev->keybit);
	__set_bit(KEY_GESTURE_S, input_dev->keybit);
	__set_bit(KEY_GESTURE_V, input_dev->keybit);
	__set_bit(KEY_GESTURE_Z, input_dev->keybit);
#endif

	enable_irq(ftxxxx_ts->client->irq);
	pr_err("[FTS][Touch][INFO] client name = %s irq = %d\n", client->name, client->irq);
	pr_err("[FTS][Touch] X-RES = %d, Y-RES = %d, RST gpio = %d, irq gpio  = %d, client irq = %d\n",
		ftxxxx_ts->x_max, ftxxxx_ts->y_max, FT_RST_PORT, FT_INT_PORT, ftxxxx_ts->client->irq);

	if (ftxxxx_ts->init_success == 1)
		focal_init_success = 1;
	return 0;
#ifdef SYSFS_DEBUG
//exit_err_sdev_register_fail:
#endif
exit_input_register_device_failed:
	input_free_device(input_dev);

exit_input_dev_alloc_failed:
	free_irq(client->irq, ftxxxx_ts);
#ifdef CONFIG_PM
#if 0
exit_request_reset:
	gpio_free(FT_RST_PORT);
#endif
#endif

exit_init_gpio:
	fts_un_init_gpio_hw(ftxxxx_ts);

exit_irq_request_failed:
	i2c_set_clientdata(client, NULL);
	kfree(ftxxxx_ts);

exit_alloc_data_failed:
	pr_err("[%s] alloc ftxxxx_ts_data failed !! \n", __func__);
exit_check_functionality_failed:
exit_pm_class:
	return err;
}

#ifdef CONFIG_PM
static int ftxxxx_ts_suspend(struct device *dev)
{
int i=0;
/*#ifdef FTS_GESTRUE
	struct ftxxxx_ts_data *ts = ftxxxx_ts;

			pr_err("[FTS] open gestrue mode");
			ftxxxx_write_reg(ts->client, 0xd0, 0x01);
			if (fts_updateinfo_curr.CHIP_ID==0x54)
		  	{
				ftxxxx_write_reg(ts->client, 0xd1, 0xff);
				ftxxxx_write_reg(ts->client, 0xd2, 0xff);
				ftxxxx_write_reg(ts->client, 0xd5, 0xff);
				ftxxxx_write_reg(ts->client, 0xd6, 0xff);
				ftxxxx_write_reg(ts->client, 0xd7, 0xff);
				ftxxxx_write_reg(ts->client, 0xd8, 0xff);
			}
			return 0;
#else
*/
	pr_err("[FTS] ftxxxx Touch suspend\n");
	disable_irq_nosync(ftxxxx_ts->client->irq);
	pr_err("[FTS] disable irq = %d",ftxxxx_ts->client->irq);
	ftxxxx_write_reg(ftxxxx_ts->client,0xa5,0x03);
	msleep(20);
	for (i = 0; i <CFG_MAX_TOUCH_POINTS; i++) 
	{
		input_mt_slot(ftxxxx_ts->input_dev, i);
		input_mt_report_slot_state(ftxxxx_ts->input_dev, MT_TOOL_FINGER, 0);
		#if 1
		pr_err("\n [FTS] zax release all finger \n");
		#endif
	}
	//input_mt_report_pointer_emulation(ftxxxx_ts>input_dev, false);
	input_report_key(ftxxxx_ts->input_dev, BTN_TOUCH, 0);
	input_sync(ftxxxx_ts->input_dev);
	gpio_set_value(ftxxxx_ts->pdata->reset_pin, 0);
	pr_err("[FTS] ftxxxx Touch suspend RST pull Down reset_pin = %d\n",ftxxxx_ts->pdata->reset_pin);
//	return ftxxxx_ts_power_off(client);
	return 0;
//#endif
}

static int ftxxxx_ts_resume(struct device *dev)
{
	struct ftxxxx_ts_data *ts = ftxxxx_ts;
	
	pr_err("[FTS]ftxxxx resume.\n");
//#ifdef FTS_GESTRUE	/*zax 20140922*/
//	ftxxxx_write_reg(ts->client, 0xD0, 0x00);
//#endif
	gpio_set_value(ts->pdata->reset_pin, 0);
	msleep(20);
	gpio_set_value(ts->pdata->reset_pin, 1);
	pr_err("[FTS] ftxxxx Touch suspend RST pull Up reset_pin = %d\n",ts->pdata->reset_pin);
	msleep(260);
	enable_irq(ftxxxx_ts->client->irq);
	pr_err("[FTS] enable irq = %d",ftxxxx_ts->client->irq);
//	return ftxxxx_ts_power_on(client);	
	return 0;
}
#else
#define ftxxxx_ts_suspend		NULL
#define ftxxxx_ts_resume		NULL
#endif

static int ftxxxx_ts_remove(struct i2c_client *client)
{
	struct ftxxxx_ts_data *ftxxxx_ts;
	ftxxxx_ts = i2c_get_clientdata(client);
	input_unregister_device(ftxxxx_ts->input_dev);

#ifdef CONFIG_PM
	gpio_free(ftxxxx_ts->pdata->reset_pin);
#endif
#ifdef FTS_CTL_IIC
	ft_rw_iic_drv_exit();
#endif
#ifdef SYSFS_DEBUG
	ftxxxx_remove_sysfs(client);
#endif
#ifdef FTS_APK_DEBUG
	ftxxxx_release_apk_debug_channel();
#endif

	fts_un_init_gpio_hw(ftxxxx_ts);

	free_irq(client->irq, ftxxxx_ts);

	kfree(ftxxxx_ts);
	i2c_set_clientdata(client, NULL);

	return 0;
}

//static SIMPLE_DEV_PM_OPS(ftxxxx_pm_ops, ftxxxx_ts_suspend, ftxxxx_ts_resume);
static const struct dev_pm_ops ftxxxx_pm_ops = {
        .suspend        = ftxxxx_ts_suspend,
        .resume         = ftxxxx_ts_resume,
};
static const struct i2c_device_id ftxxxx_ts_id[] = {
	{ FTXXXX_NAME, 0 },
	{ }
};
#ifdef CONFIG_ACPI
static const struct acpi_device_id ftxxxx_acpi_match[] = {
	{"FOCA5526", 0}, //Hack to use default SILEAD touch ID
	{ },
};
#endif
/*MODULE_DEVICE_TABLE(i2c, ftxxxx_ts_id);*/

static struct i2c_driver ftxxxx_ts_driver = {
	.probe = ftxxxx_ts_probe,
	.remove = ftxxxx_ts_remove,
	.id_table = ftxxxx_ts_id,
//	.suspend = ftxxxx_ts_suspend,
//	.resume = ftxxxx_ts_resume,
	.driver = {
		.name = FTXXXX_NAME,
		.owner = THIS_MODULE,
		.pm       = &ftxxxx_pm_ops,
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(ftxxxx_acpi_match),
#endif
	},
};

static int __init ftxxxx_ts_init(void)
{
#ifdef CONFIG_PLATFORM_DEVICE_PM_VIRT
	int ret = device_state_pm_add_class(&ftxxxx_pm_class);
	if (ret)
		return ret;
#else
	int ret;
#endif

	pr_err("[FTS][Touch] ftxxxx_ts_init !\n");
	ret = i2c_add_driver(&ftxxxx_ts_driver);
	if (ret) {
		pr_err(KERN_WARNING "Adding ftxxxx driver failed "
			"(errno = %d)\n", ret);
	} else {
		pr_info("[FTS][Touch] Successfully added driver %s\n",
			ftxxxx_ts_driver.driver.name);
	}
	return ret;
}

static void __exit ftxxxx_ts_exit(void)
{
	i2c_del_driver(&ftxxxx_ts_driver);
}

module_init(ftxxxx_ts_init);
module_exit(ftxxxx_ts_exit);

MODULE_AUTHOR("<OSTeam>");
MODULE_DESCRIPTION("FocalTech TouchScreen driver");
MODULE_LICENSE("GPL");
