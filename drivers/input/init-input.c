/*
 * Copyright (c) 2013-2015 liming@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/init-input.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

static const char * const input_types[] = {
	[CTP_TYPE]	= "ctp",
	[GSENSOR_TYPE]	= "gsensor",
	[GYR_TYPE]	= "gyrsensor",
	[COMPASS_TYPE]	= "lssensor",
	[LS_TYPE]	= "compass",
	[MOTOR_TYPE]	= "motor",
};

static int get_sensor_node(enum input_sensor_type *type, struct device_node** np)
{
	struct device_node *hub = NULL;
	struct device_node *child = NULL;

	hub = of_find_node_by_name(NULL, SUNXI_INPUT_HUB);
	if (!hub) {
		pr_err("unable to find input hub!\n");
		return -1;
	}

	child = of_get_child_by_name(hub, input_types[*type]);
	if (!child) {
		pr_err("unable to find %s node !\n",input_types[*type]);
		return -1;
	}
	*np = child;

	return 0;
}

/*********************************CTP*******************************************/

/**
 * ctp_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int ctp_fetch_sysconfig_para(enum input_sensor_type *ctp_type)
{
	int ret = -1;
	u32 gpio;
	struct device_node *np = NULL;
	struct ctp_config_info *data = container_of(ctp_type,
					struct ctp_config_info, input_type);

	pr_info("=====%s=====. \n", __func__);

	if(get_sensor_node(ctp_type, &np))
		return -EPERM;
	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi ctp disable\n", __func__);
		goto script_get_item_err;
	}else
		data->ctp_used = 1;

	if (of_property_read_u32(np, "twi_id", &data->twi_id)){
		pr_err("%s: get ctp_twi_id failed", __func__);
		goto script_get_item_err;
	}

	if (of_property_read_u32(np, "screen_max_x", &data->screen_max_x)) {
		pr_err("%s: get screen_max_x failed\n", __func__);
		goto script_get_item_err;
	}

	if (of_property_read_u32(np, "screen_max_y", &data->screen_max_y)) {
		pr_err("%s: get screen_max_y failed\n", __func__);
		goto script_get_item_err;
	}

	if (of_property_read_u32(np, "revert_x_flag", &data->revert_x_flag)) {
		pr_err("%s: get revert_x_flag failed\n", __func__);
		goto script_get_item_err;
	}

	if (of_property_read_u32(np, "revert_y_flag", &data->revert_y_flag)) {
		pr_err("%s: get revert_y_flag failed\n", __func__);
		goto script_get_item_err;
	}

	if (of_property_read_u32(np, "exchange_x_y_flag", &data->exchange_x_y_flag)) {
		pr_err("%s: get exchange_x_y_flag failed\n", __func__);
		goto script_get_item_err;
	}

	gpio = of_get_named_gpio_flags(np, "int_port", 0,
			(enum of_gpio_flags *)&data->irq_gpio);
	if (!gpio_is_valid(gpio)){
		pr_err("%s: get int_port failed\n", __func__);
		goto script_get_item_err;
	}

	gpio = of_get_named_gpio_flags(np, "wakeup_port", 0,
			(enum of_gpio_flags *)&data->wakeup_gpio);
	if (!gpio_is_valid(gpio)){
		pr_err("%s: get wakeup_port failed\n", __func__);
		goto script_get_item_err;
	}

	if (of_property_read_u32(np, "power_ldo_vol", &data->ctp_power_vol)) {
		pr_err("%s: get power_ldo_vol failed\n", __func__);
		goto script_get_item_err;
	}
	data->ctp_power = "vcc-ctp";

	data->int_number = data->irq_gpio.gpio;
	pr_err("ctp_irq gpio number is %d\n", data->int_number);

	return 0;

script_get_item_err:
	pr_notice("=========script_get_item_err============\n");
	return ret;
}

/**
 * ctp_free_platform_resource - free ctp related resource
 * return value:
 */
static void ctp_free_platform_resource(enum input_sensor_type *ctp_type)
{
	struct ctp_config_info *data = container_of(ctp_type,
					struct ctp_config_info, input_type);
	gpio_free(data->wakeup_gpio.gpio);

	if(data->ctp_power_ldo) {
		regulator_put(data->ctp_power_ldo);
		data->ctp_power_ldo= NULL;
	} else if(0 != data->ctp_power_io.gpio) {
		gpio_free(data->ctp_power_io.gpio);
	}
	return;
}

/**
 * ctp_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int ctp_init_platform_resource(enum input_sensor_type *ctp_type)
{
	int ret = -1;
        struct ctp_config_info *data = container_of(ctp_type,
					struct ctp_config_info, input_type);

	if (data->ctp_power) {
		data->ctp_power_ldo = regulator_get(NULL, data->ctp_power);
		if (!data->ctp_power_ldo)
			pr_err("%s: could not get ctp ldo '%s' , check"
					"if ctp independent power supply by ldo,ignore"
					"firstly\n",__func__,data->ctp_power);
		else
			regulator_set_voltage(data->ctp_power_ldo,
					(int)(data->ctp_power_vol)*1000,
					(int)(data->ctp_power_vol)*1000);
	} else if(0 != data->ctp_power_io.gpio) {
		if(0 != gpio_request(data->ctp_power_io.gpio, NULL))
			pr_err("ctp_power_io gpio_request is failed,"
					"check if ctp independent power supply by gpio,"
					"ignore firstly\n");
		else
			gpio_direction_output(data->ctp_power_io.gpio, 1);
	}
	if(0 != gpio_request(data->wakeup_gpio.gpio, NULL)) {
		pr_err("wakeup gpio_request is failed\n");
		return ret;
	}
	if (0 != gpio_direction_output(data->wakeup_gpio.gpio, 1)) {
		pr_err("wakeup gpio set err!\n");
		return ret;
	}

	ret = 0;
	return ret;
}
/*********************************CTP END***************************************/

/*********************************GSENSOR***************************************/

/**
 * gsensor_free_platform_resource - free gsensor related resource
 * return value:
 */
static void gsensor_free_platform_resource(enum input_sensor_type *gsensor_type)
{
}

/**
 * gsensor_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int gsensor_init_platform_resource(enum input_sensor_type *gsensor_type)
{
	return 0;
}

/**
 * gsensor_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int gsensor_fetch_sysconfig_para(enum input_sensor_type *gsensor_type)
{
	int ret = -1;
	struct device_node *np = NULL;
	struct sensor_config_info *data = container_of(gsensor_type,
					struct sensor_config_info, input_type);
	pr_info("=====%s=====. \n", __func__);

	if(get_sensor_node(gsensor_type, &np))
		return -EPERM;
	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi gsensor disable\n", __func__);
		goto script_get_err;
	}else
		data->sensor_used = 1;

	if (of_property_read_u32(np, "twi_id", &data->twi_id)) {
		pr_err("%s: get twi_id failed\n", __func__);
		goto script_get_err;
	}

	return 0;
script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;

}

/*********************************GSENSOR END***********************************/

/********************************** GYR ****************************************/

/**
 * gyr_free_platform_resource - free gyr related resource
 * return value:
 */
static void gyr_free_platform_resource(enum input_sensor_type *gyr_type)
{
}

/**
 * gyr_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int gyr_init_platform_resource(enum input_sensor_type *gyr_type)
{
	return 0;
}

/**
 * gyr_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int gyr_fetch_sysconfig_para(enum input_sensor_type *gyr_type)
{
	int ret = -1;
	struct device_node *np = NULL;
	struct sensor_config_info *data = container_of(gyr_type,
					struct sensor_config_info, input_type);

	pr_info("=====%s=====. \n", __func__);

	if(get_sensor_node(gyr_type, &np))
		return -EPERM;
	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi gyr sensor disable\n", __func__);
		goto script_get_err;
	}else
		data->sensor_used = 1;

	if (of_property_read_u32(np, "twi_id", &data->twi_id)) {
		pr_err("%s: get twi_id failed\n", __func__);
		goto script_get_err;
	}

	return 0;
script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;
}

/********************************* GYR END *************************************/

/********************************* COMPASS *************************************/

/**
 * e_compass_free_platform_resource - free e_compass related resource
 * return value:
 */
static void e_compass_free_platform_resource(enum input_sensor_type *e_compass_type)
{
}

/**
 * e_compass_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int e_compass_init_platform_resource(enum input_sensor_type *e_compass_type)
{
	return 0;
}

/**
 * e_compass_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */

static int e_compass_fetch_sysconfig_para(enum input_sensor_type *e_compass_type)
{
	int ret = -1;
	struct device_node *np = NULL;
	struct sensor_config_info *data = container_of(e_compass_type,
					struct sensor_config_info, input_type);
	pr_info("=====%s=====. \n", __func__);

	if(get_sensor_node(e_compass_type, &np))
		return -EPERM;
	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi e compass disable\n", __func__);
		goto script_get_err;
	}else
		data->sensor_used = 1;

	if (of_property_read_u32(np, "twi_id", &data->twi_id)) {
		pr_err("%s: get twi_id failed\n", __func__);
		goto script_get_err;
	}

	return 0;
script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;

}
/******************************* COMPASS END ***********************************/

/****************************** LIGHT SENSOR ***********************************/

/**
 * ls_free_platform_resource - free ls related resource
 * return value:
 */
static void ls_free_platform_resource(enum input_sensor_type *ls_type)
{
	struct regulator *ldo = NULL;
	struct sensor_config_info *data = container_of(ls_type,
					struct sensor_config_info, input_type);
	/* disable ldo if it exist */
	if (data->ldo) {
		ldo = regulator_get(NULL, data->ldo);
		if (!ldo) {
			pr_err("%s: could not get ldo '%s' in remove, something error ???, "
					"ignore it here !!!!!!!!!\n", __func__, data->ldo);
		} else {
			regulator_disable(ldo);
			regulator_put(ldo);
		}
	}
}

/**
 * ls_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int ls_init_platform_resource(enum input_sensor_type *ls_type)
{
	struct regulator *ldo = NULL;
	struct sensor_config_info *data = container_of(ls_type,
					struct sensor_config_info, input_type);

	/* enalbe ldo if it exist */
	if (data->ldo) {
		ldo = regulator_get(NULL, data->ldo);
		if (!ldo) {
			pr_err("%s: could not get sensor ldo '%s' in probe, maybe config error,"
					"ignore firstly !!!!!!!\n", __func__, data->ldo);
		}
		regulator_set_voltage(ldo, 3000000, 3000000);
		regulator_enable(ldo);
		regulator_put(ldo);
		usleep_range(10000, 15000);
	}
	return 0;
}

/**
 * ls_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */

static int ls_fetch_sysconfig_para(enum input_sensor_type *ls_type)
{
	int ret = -1;
	struct device_node *np;
	struct sensor_config_info *data = container_of(ls_type,
					struct sensor_config_info, input_type);

	if(get_sensor_node(ls_type, &np))
		return -EPERM;
	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi ls sensor disable\n", __func__);
		goto script_get_err;
	}else
		data->sensor_used = 1;

	pr_info("=====%s=====. \n", __func__);

	if (of_property_read_u32(np, "twi_id", &data->twi_id)) {
		pr_err("%s: get twi_id failed\n", __func__);
		goto script_get_err;
	}

	return 0;
script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;

}
/**************************** LIGHT SENSOR END *********************************/

/********************************** MOTOR *************************************/

/**
 * motor_free_platform_resource - free ths related resource
 * return value:
 */
static void motor_free_platform_resource(enum input_sensor_type *motor_type)
{
	struct motor_config_info *data = container_of(motor_type,
						struct motor_config_info, input_type);
	if (0 != data->motor_gpio.gpio) {
		gpio_free(data->motor_gpio.gpio);
	}

	return;
}

/**
 * motor_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int motor_init_platform_resource(enum input_sensor_type *motor_type)
{
	struct motor_config_info *data = container_of(motor_type,
						struct motor_config_info, input_type);
	if (0 != data->motor_gpio.gpio) {
		if(0 != gpio_request(data->motor_gpio.gpio, "vibe")) {
			pr_err("ERROR: vibe Gpio_request is failed\n");
			goto exit;
		}
		gpio_direction_output(data->motor_gpio.gpio, data->vibe_off);
	}

	return 0;
exit:
	return -1;
}

/**
 * motor_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int motor_fetch_sysconfig_para(enum input_sensor_type *motor_type)
{
	u32 gpio;
	struct device_node *np;
	struct motor_config_info *data = container_of(motor_type,
					struct motor_config_info, input_type);
	pr_info("=====%s=====. \n", __func__);

	if(get_sensor_node(motor_type, &np))
		return -EPERM;
	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi ls sensor disable\n", __func__);
		goto script_get_err;
	}else
		data->motor_used = 1;

	if (of_property_read_u32(np, "ldo_voltage", &data->ldo_voltage)) {
		pr_err("%s: get ldo_voltage failed\n", __func__);
		goto script_get_err;
	}

	gpio = of_get_named_gpio_flags(np, "shake", 0,
			(enum of_gpio_flags *)&data->motor_gpio);
	if (!gpio_is_valid(gpio)){
		pr_err("no motor_shake, ignore it!\n");
		data->ldo = "vcc-vibrator";
	}

	return 0;
script_get_err:
	pr_notice("=========script_get_err============\n");
	return -1;
}

/******************************** MOTOR END ***********************************/


static int (* const fetch_sysconfig_para[])(enum input_sensor_type *input_type) = {
	ctp_fetch_sysconfig_para,
	gsensor_fetch_sysconfig_para,
	gyr_fetch_sysconfig_para,
	e_compass_fetch_sysconfig_para,
	ls_fetch_sysconfig_para,
	motor_fetch_sysconfig_para
};

static int (*init_platform_resource[])(enum input_sensor_type *input_type) = {
	ctp_init_platform_resource,
	gsensor_init_platform_resource,
	gyr_init_platform_resource,
	e_compass_init_platform_resource,
	ls_init_platform_resource,
	motor_init_platform_resource
};

static void (*free_platform_resource[])(enum input_sensor_type *input_type) = {
	ctp_free_platform_resource,
	gsensor_free_platform_resource,
	gyr_free_platform_resource,
	e_compass_free_platform_resource,
	ls_free_platform_resource,
	motor_free_platform_resource
};

int input_set_power_enable(enum input_sensor_type *input_type, u32 enable)
{
	int ret = -1;
	struct regulator *ldo = NULL;
	u32 power_io = 0;
	void *data = NULL;
	switch (*input_type) {
		case CTP_TYPE:
			data = container_of(input_type,
					struct ctp_config_info, input_type);
			ldo = ((struct ctp_config_info *)data)->ctp_power_ldo;
			power_io = ((struct ctp_config_info *)data)->ctp_power_io.gpio;
			break;
		case GSENSOR_TYPE:
			break;
		case LS_TYPE:
			break;
		default:
			break;
	}
	if ((enable != 0) && (enable != 1)) {
		return ret;
	}
	if(ldo) {
		if(enable) {
			regulator_enable(ldo);
		} else {
			if (regulator_is_enabled(ldo))
				regulator_disable(ldo);
		}
	} else if(power_io) {
		if(enable) {
			__gpio_set_value(power_io,1);
		} else {
			__gpio_set_value(power_io,0);
		}
	}
	return 0;
}
EXPORT_SYMBOL(input_set_power_enable);
/**
 * input_set_int_enable - input set irq enable
 * Input:
 * 	type:
 *      enable:
 * return value: 0 : success
 *               -EIO :  i/o err.
 */
int input_set_int_enable(enum input_sensor_type *input_type, u32 enable)
{
	int ret = -1;
	u32 irq_number = 0;
	void *data = NULL;

	switch (*input_type)
	{
	case CTP_TYPE:
		data = container_of(input_type,
					struct ctp_config_info, input_type);
		irq_number = gpio_to_irq(((struct ctp_config_info *)data)->int_number);
		break;
	case GSENSOR_TYPE:
		break;
	case LS_TYPE:
		data = container_of(input_type,
					struct sensor_config_info, input_type);
		irq_number = gpio_to_irq(((struct sensor_config_info *)data)->int_number);
		break;
	default:
		break;
	}

	if ((enable != 0) && (enable != 1)) {
		return ret;
	}
	if (1 == enable)
		enable_irq(irq_number);
	else
		disable_irq_nosync(irq_number);

	return 0;
}
EXPORT_SYMBOL(input_set_int_enable);

/**
 * input_free_int - input free irq
 * Input:
 * 	type:
 * return value: 0 : success
 *               -EIO :  i/o err.
 */
int input_free_int(enum input_sensor_type *input_type, void *para)
{
	int irq_number = 0;
	void *data = NULL;
	struct device *dev = NULL;

	switch (*input_type)
	{
	case CTP_TYPE:
		data = container_of(input_type,
					struct ctp_config_info, input_type);
		irq_number = gpio_to_irq(((struct ctp_config_info *)data)->int_number);

		dev = ((struct ctp_config_info *)data)->dev;
		break;
	case GSENSOR_TYPE:
		break;
	case LS_TYPE:
		data = container_of(input_type,
					struct sensor_config_info, input_type);
		irq_number = gpio_to_irq(((struct sensor_config_info *)data)->int_number);

		dev = ((struct sensor_config_info *)data)->dev;
		break;
	default:
		break;
	}

	devm_free_irq(dev, irq_number, para);

	return 0;
}
EXPORT_SYMBOL(input_free_int);

/**
 * input_request_int - input request irq
 * Input:
 * 	type:
 *      handle:
 *      trig_gype:
 *      para:
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
int input_request_int(enum input_sensor_type *input_type, irq_handler_t handle,
			unsigned long trig_type, void *para)
{
	int ret = -1;
	int irq_number = 0;

	void *data = NULL;
	struct device *dev = NULL;

	switch (*input_type)
	{
	case CTP_TYPE:
		data = container_of(input_type,
					struct ctp_config_info, input_type);
		irq_number = gpio_to_irq(((struct ctp_config_info *)data)->int_number);
		if (IS_ERR_VALUE(irq_number)) {
			pr_warn("map gpio [%d] to virq failed, errno = %d\n",
				GPIOA(3), irq_number);
			return -EINVAL;
		}

		dev = ((struct ctp_config_info *)data)->dev;
		break;
	case GSENSOR_TYPE:
		break;
	case LS_TYPE:
		data = container_of(input_type,
					struct sensor_config_info, input_type);
		irq_number = gpio_to_irq(((struct sensor_config_info *)data)->int_number);
		if (IS_ERR_VALUE(irq_number)) {
			pr_warn("map gpio [%d] to virq failed, errno = %d\n",
				GPIOA(3), irq_number);
			return -EINVAL;
		}

		dev = ((struct sensor_config_info *)data)->dev;
		break;
	default:
		break;
	}

	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(dev, irq_number, handle,
			       trig_type, "PA3_EINT", para);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("request virq %d failed, errno = %d\n",
		         irq_number, ret);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(input_request_int);

/**
 * input_free_platform_resource - free platform related resource
 * Input:
 * 	event:
 * return value:
 */
void input_free_platform_resource(enum input_sensor_type *input_type)
{
        (*free_platform_resource[*input_type])(input_type);
	return;
}
EXPORT_SYMBOL(input_free_platform_resource);

/**
 * input_init_platform_resource - initialize platform related resource
 * Input:
 * 	type:
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
int input_init_platform_resource(enum input_sensor_type *input_type)
{
	int ret = -1;

	ret = (*init_platform_resource[*input_type])(input_type);

	return ret;
}
EXPORT_SYMBOL(input_init_platform_resource);

/**
 * input_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * Input:
 * 	type:
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
int input_fetch_sysconfig_para(enum input_sensor_type *input_type)
{
	int ret = -1;

	ret = (*fetch_sysconfig_para[*input_type])(input_type);

	return ret;
}
EXPORT_SYMBOL(input_fetch_sysconfig_para);

