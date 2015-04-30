/*
 * drivers/misc/max1749.c
 *
 * Driver for MAX1749, vibrator motor driver.
 *
 * Copyright (c) 2011-2012 NVIDIA Corporation, All Rights Reserved.
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

#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/pwm.h>


#include <linux/slab.h>

#include "../staging/android/timed_output.h"

#define DRV8601_VIBRATOR_EN 226/*TEGRA_GPIO_PCC2*/
#define DRV8601_VIBRATOR_PWM 59/*TEGRA_GPIO_PH3*/
//#define DRV8601_PWM_EN

//#define DRV8601_DEBUG
#ifdef DRV8601_DEBUG
#define drv8601_dbg(format, arg...) \
	printk(format , ## arg)
#else
#define drv8601_dbg(format, arg...) \
	while(0){}
#endif

#ifdef CONFIG_DRV8601_HRTIMER
#define DRV8601_HRTIMER
#endif

struct drv8601_vibrator_data {
	struct timed_output_dev dev;
	struct pwm_device	*pwm;
	unsigned int enable_gpio;
#ifdef DRV8601_HRTIMER
	struct hrtimer timer;
	struct work_struct work;
#endif
	bool vibrator_on;
};

static struct drv8601_vibrator_data *data;

static struct regulator *keenhi_t40_vibrator_vddio;
static void drv8601_vibrator_power_supply(bool state)//true:vdd power on,false:vdd power off
{
    int ret=0;
    printk("=========================%s\n",__func__);
    if (!keenhi_t40_vibrator_vddio) {
        printk("get vdd_cam_1v8\n");
        keenhi_t40_vibrator_vddio = regulator_get(NULL, "vdd_cam_1v8");
        if (WARN_ON(IS_ERR(keenhi_t40_vibrator_vddio)))
        {
            pr_err("%s: couldn't get regulator vd_cam_1v8: %ld\n",
                __func__, PTR_ERR(keenhi_t40_vibrator_vddio));
            keenhi_t40_vibrator_vddio = NULL;
            return;
        }
    }

    if(keenhi_t40_vibrator_vddio) {
        printk("open vdd_cam_1v8 = %d\n",state);
        if(state==true)
        {
            ret = regulator_enable(keenhi_t40_vibrator_vddio);
            if (ret < 0)
                pr_err("%s: couldn't enable regulator vd_cam_1v8: %ld\n",
                    __func__, PTR_ERR(keenhi_t40_vibrator_vddio));
        }
        else if(state==false)
        {
            regulator_disable(keenhi_t40_vibrator_vddio);
            regulator_put(keenhi_t40_vibrator_vddio);
            keenhi_t40_vibrator_vddio = NULL;
        }
    }
}

static void drv8601_vibrator_start(void)
{
	drv8601_dbg("=======================%s\n",__func__);

	if (!data->vibrator_on) {
		gpio_direction_output(DRV8601_VIBRATOR_EN, 1);
		data->vibrator_on = true;
	}
}

static void drv8601_vibrator_stop(void)
{
	drv8601_dbg("=======================%s\n",__func__);
	if (data->vibrator_on) {
		gpio_direction_output(DRV8601_VIBRATOR_EN, 0);
		data->vibrator_on = false;
	}
}

#ifdef DRV8601_HRTIMER
static void drv8601_vibrator_work_func(unsigned long data)
{
	drv8601_dbg("=======================%s\n",__func__);
	drv8601_vibrator_stop();
}

static enum hrtimer_restart drv8601_vibrator_timer_func(struct hrtimer *timer)
{
	drv8601_dbg("=======================%s\n",__func__);
	schedule_work(&data->work);
	return HRTIMER_NORESTART;
}
#endif
/*
 * Timeout value can be changed from sysfs entry
 * created by timed_output_dev.
 * echo 100 > /sys/class/timed_output/vibrator/enable
 */
static void drv8601_vibrator_enable(struct timed_output_dev *dev, int value)
{
	drv8601_dbg("=======================%s value =%d\n",__func__,value);
#ifdef DRV8601_HRTIMER
	hrtimer_cancel(&data->timer);
#endif

	if (value > 0) {
		drv8601_vibrator_start();
#ifdef DRV8601_HRTIMER
		hrtimer_start(&data->timer,
			  ktime_set(value / 1000, (value % 1000) * 1000000),
			  HRTIMER_MODE_REL);
#endif
	} else {	
		drv8601_vibrator_stop();
	}
	return;
}

static int drv8601_vibrator_get_time(struct timed_output_dev *dev)
{
	drv8601_dbg("=======================%s\n",__func__);

#ifdef DRV8601_HRTIMER
	if (hrtimer_active(&data->timer)) {
		ktime_t r = hrtimer_get_remaining(&data->timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	} else
#endif

	return 0;
}

static struct timed_output_dev drv8601_vibrator_dev = {
	.name		= "vibrator",
	.get_time	= drv8601_vibrator_get_time,
	.enable		= drv8601_vibrator_enable,
};

static int drv8601_vibrator_probe(struct platform_device *pdev)
{
	int ret;
	data = kzalloc(sizeof(struct drv8601_vibrator_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	drv8601_vibrator_power_supply(true);

	ret = gpio_request(DRV8601_VIBRATOR_EN, "drv8601_en");
	if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s\n",
			__func__, "DRV8601_VIBRATOR_EN");
		goto err;
	}
	gpio_direction_output(DRV8601_VIBRATOR_EN, 0);

#ifdef DRV8601_PWM_EN
	ret = gpio_request(DRV8601_VIBRATOR_PWM, "drv8601_pwm");
	if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s\n",
			__func__, "DRV8601_VIBRATOR_EN");
		goto err_gpio;
	}
	gpio_free(DRV8601_VIBRATOR_PWM);

	data->pwm = pwm_request(3, "drv8601_vibrator");
	if (IS_ERR(data->pwm)) {
		pr_err("%s unable to request PWM for drv8601_vibrator %d\n",__func__,IS_ERR(data->pwm));
		ret = PTR_ERR(data->pwm);
		goto err_gpio;
	} else
		dev_dbg(&pdev->dev, "got pwm for drv8601_vibrator\n");
	
	pwm_config(data->pwm,10000,78770);
	pwm_enable(data->pwm);
#endif

#ifdef DRV8601_HRTIMER
	hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	/* Intialize the work queue */
	INIT_WORK(&data->work, drv8601_vibrator_work_func);
	data->timer.function = drv8601_vibrator_timer_func;
#endif
	data->enable_gpio = DRV8601_VIBRATOR_EN;
	data->dev = drv8601_vibrator_dev;
	data->vibrator_on = false;

	ret = timed_output_dev_register(&data->dev);
	if (ret)
	{
		pr_err("%s unable to request timed_output_dev\n",__func__);
		goto err_pwm;
	}

	return 0;
	
err_pwm:
#ifdef DRV8601_PWM_EN
	pwm_free(data->pwm);
err_gpio:
#endif
	gpio_free(data->enable_gpio);
err:
	kfree(data);
	return ret;
}

static int drv8601_vibrator_remove(struct platform_device *pdev)
{
	timed_output_dev_unregister(&data->dev);
	drv8601_vibrator_power_supply(false);
#ifdef DRV8601_PWM_EN
	pwm_config(data->pwm,0,39380);
	pwm_disable(data->pwm);
	pwm_free(data->pwm);
#endif
	gpio_free(data->enable_gpio);
	kfree(data);

	return 0;
}
static int tegra_drv8601_suspend(struct device *dev)
{
	drv8601_vibrator_power_supply(false);
	return 0;
}

static int tegra_drv8601_kbc_resume(struct device *dev)
{
	drv8601_vibrator_power_supply(true);
	return 0;
}

static const struct dev_pm_ops tegra_drv8601_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	.suspend = tegra_drv8601_suspend,
	.resume = tegra_drv8601_kbc_resume,
#endif
};

static struct platform_driver drv8601_vibrator_driver = {
	.probe = drv8601_vibrator_probe,
	.remove = drv8601_vibrator_remove,
	.driver = {
		.name = "tegra-vibrator",
		.owner = THIS_MODULE,
		.pm	= &tegra_drv8601_pm_ops,
	},
};

static int __init drv8601_vibrator_init(void)
{
	return platform_driver_register(&drv8601_vibrator_driver);
}

static void __exit drv8601_vibrator_exit(void)
{
	platform_driver_unregister(&drv8601_vibrator_driver);
}

MODULE_DESCRIPTION("timed output vibrator device");
MODULE_AUTHOR("GPL");

module_init(drv8601_vibrator_init);
module_exit(drv8601_vibrator_exit);
