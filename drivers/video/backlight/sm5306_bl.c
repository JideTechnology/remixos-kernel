/*
* Simple driver for Silicon Mitus SM5306 Backlight driver chip
* Copyright (C) 2015 Silicon Mitus
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/regmap.h>
#include <linux/platform_data/sm5306_bl.h>
#include <linux/fb.h>
#include <linux/acpi.h>

#define REG_CTRL         0x00
#define REG_CONFIG       0x01
#define REG_BOOST_CTRL   0x02
#define REG_BRT_MSB      0x03
#define REG_BRT_LSB      0x04
#define REG_CURRENT      0x05
#define REG_INT_STATUS   0x09
#define REG_INT_EN       0x0A
#define REG_FAULT        0x0B
#define REG_PWM_OUTLOW   0x12
#define REG_PWM_OUTHIGH  0x13
#define REG_MAX          0x1F

#define INT_DEBOUNCE_MSEC   10

enum sm5306_leds {
	BLED_ALL = 0,
	BLED_1,
	BLED_2
};

static const char * const bled_name[] = {
	[BLED_ALL] = "sm5306_bled", /*Bank1 controls all string */
	[BLED_1] = "sm5306_bled1",  /*Bank1 controls bled1 */
	[BLED_2] = "sm5306_bled2",  /*Bank1 or 2 controls bled2 */
};

struct sm5306_chip_data {
	struct device *dev;
	struct delayed_work work;
	int irq;
	struct workqueue_struct *irqthread;
	struct sm5306_platform_data *pdata;
	struct backlight_device *bled;
	struct regmap *regmap;
};

static struct sm5306_chip_data *local_chip_data;

/* initialize chip */
static int sm5306_chip_init(struct sm5306_chip_data *pchip)
{
	int ret;
	unsigned int reg_val;
	struct sm5306_platform_data *pdata = pchip->pdata;

	pr_err("[SM5306]%s\n", __func__);

	/* LED_EN1:enable, LED_EN2:enable*/
	pdata->bank_a_ctrl = 0x03;

	/*pwm control : PWM_LOW:0  PWM_EN:0*/
	reg_val = ((pdata->pwm_active & 0x00) << 2) | (pdata->pwm_ctrl & 0x00);
	ret = regmap_update_bits(pchip->regmap, REG_CONFIG, 0x07, reg_val);
	pr_err("[SM5306]regmap_update_bits(): 0x%x = 0x%x\n",
				REG_CONFIG, reg_val);
	if (ret < 0)
		goto out;

	/* Boost control : BOOST_OVP : 40V */
	reg_val = (0x03 << 5);/* 00b:16V , 01b:24V , 10b:32V , 11b:40V */
	ret = regmap_update_bits(pchip->regmap, REG_BOOST_CTRL, 0x60, reg_val);
	pr_err("[SM5306]regmap_update_bits(): 0x%x = 0x%x\n",
				REG_BOOST_CTRL, reg_val);
	if (ret < 0)
		goto out;

    /* Current : 0x14 */
	reg_val = 0x1F;
	ret = regmap_write(pchip->regmap,
				REG_CURRENT, reg_val);
	pr_err("[SM5306]regmap_write(): 0x%x = 0x%x\n",
				REG_CURRENT, reg_val);
	if (ret < 0)
		goto out;

	/* bank control : LED_EN1 : 1   LED_EN2_ : 1  LINERA : 1*/
	reg_val = (((pdata->bank_a_ctrl & 0x03) << 1) | 0x10);
	ret = regmap_update_bits(pchip->regmap, REG_CTRL, 0x16, reg_val);
	pr_err("[SM5306]regmap_update_bits(): 0x%x = 0x%x\n",
				REG_CTRL, reg_val);
	if (ret < 0)
		goto out;

    /* SLEEP_CMD : enable:0 */
	ret = regmap_update_bits(pchip->regmap, REG_CTRL, 0x80, 0x00);
	pr_err("[SM5306]regmap_update_bits(): 0x%x = 0x%x\n",
				REG_CTRL, 0x00);
	if (ret < 0)
		goto out;

	/* set initial brightness */
    /* LSB write*/
	ret = regmap_write(pchip->regmap,
			   REG_BRT_LSB, 0x00);
	pr_err("[SM5306]regmap_write(): 0x%x = 0x%x\n",
				REG_BRT_LSB, 0x00);
	if (ret < 0)
		goto out;

    /* MSB write*/
	ret = regmap_write(pchip->regmap,
			   REG_BRT_MSB, pdata->init_brt_led - 1);
	pr_err("[SM5306]regmap_write(): 0x%x = 0x%x\n",
				REG_BRT_MSB, pdata->init_brt_led - 1);
	if (ret < 0)
		goto out;

	return ret;

out:
	pr_err("i2c failed to access register\n");
	return ret;
}

int sm5306_led_on(void)
{
	int ret;
	unsigned int reg_val;
	struct sm5306_chip_data *pchip = local_chip_data;

	pr_err("[SM5306]%s\n", __func__);

	/* SLEEP_CMD : enable:0 */
	reg_val = 0x00;
	ret = regmap_update_bits(pchip->regmap, REG_CTRL, 0x80, reg_val);
	pr_err("[SM5306]regmap_update_bits(): 0x%x = 0x%x\n",
				REG_CTRL, reg_val);

	return ret;
}
EXPORT_SYMBOL(sm5306_led_on);

int sm5306_led_off(void)
{
	int ret;
	unsigned int reg_val;
	struct sm5306_chip_data *pchip = local_chip_data;

	pr_err("[SM5306]%s\n", __func__);

	/* SLEEP_CMD : enable:1 */
	reg_val = 0x80;
	ret = regmap_update_bits(pchip->regmap, REG_CTRL, 0x80, reg_val);
	pr_err("[SM5306]regmap_update_bits(): 0x%x = 0x%x\n",
				REG_CTRL, reg_val);

	return ret;
}
EXPORT_SYMBOL(sm5306_led_off);

int sm5306_led_set(unsigned int brightness)
{
	int ret;
	struct sm5306_chip_data *pchip = local_chip_data;

	pr_err("[SM5306]%s brightness = %d\n", __func__, brightness);

	/* LSB write*/
	ret = regmap_write(pchip->regmap,
					REG_BRT_LSB, 0x00);
	pr_err("[SM5306]regmap_write(): 0x%x = 0x%x\n",
				REG_BRT_LSB, 0x00);
	if (ret < 0)
		goto out;

	/* MSB write*/
	ret = regmap_write(pchip->regmap,
				REG_BRT_MSB, brightness);
	pr_err("[SM5306]regmap_write(): 0x%x = 0x%x\n",
				REG_BRT_MSB, brightness);
	if (ret < 0)
		goto out;

	return ret;

out:
	pr_err("i2c failed to access register\n");
	return ret;
}
EXPORT_SYMBOL(sm5306_led_set);

/* interrupt handling */
static void sm5306_delayed_func(struct work_struct *work)
{
	int ret;
	unsigned int reg_val;
	struct sm5306_chip_data *pchip;

	pr_err("[SM5306]%s\n", __func__);

	pchip = container_of(work, struct sm5306_chip_data, work.work);

	ret = regmap_read(pchip->regmap, REG_INT_STATUS, &reg_val);
	if (ret < 0) {
		pr_err("i2c failed to access REG_INT_STATUS Register\n");
		return;
	}

	dev_info(pchip->dev, "REG_INT_STATUS Register is 0x%x\n", reg_val);
}

static irqreturn_t sm5306_isr_func(int irq, void *chip)
{
	int ret;
	struct sm5306_chip_data *pchip = chip;
	unsigned long delay = msecs_to_jiffies(INT_DEBOUNCE_MSEC);
	pr_err("[SM5306]%s\n", __func__);

	queue_delayed_work(pchip->irqthread, &pchip->work, delay);

	ret = regmap_update_bits(pchip->regmap, REG_CTRL, 0x80, 0x00);
	if (ret < 0)
		goto out;

	return IRQ_HANDLED;
out:
	pr_err("i2c failed to access register\n");
	return IRQ_HANDLED;
}

static int sm5306_intr_config(struct sm5306_chip_data *pchip)
{
	pr_err("[SM5306]%s\n", __func__);

	INIT_DELAYED_WORK(&pchip->work, sm5306_delayed_func);
	pchip->irqthread = create_singlethread_workqueue("sm5306-irqthd");
	if (!pchip->irqthread) {
		pr_err("create irq thread fail...\n");
		return -1;
	}
	if (request_threaded_irq
	    (pchip->irq, NULL, sm5306_isr_func,
	     IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "sm5306_irq", pchip)) {
		pr_err("request threaded irq fail..\n");
		return -1;
	}
	return 0;
}

static bool
set_intensity(struct backlight_device *bl, struct sm5306_chip_data *pchip)
{
	if (!pchip->pdata->pwm_set_intensity)
		return false;
	pchip->pdata->pwm_set_intensity(bl->props.brightness - 1,
					pchip->pdata->pwm_period);
	return true;
}

/* update and get brightness */
static int sm5306_bank_a_update_status(struct backlight_device *bl)
{
	int ret;
	unsigned int reg_val;
	int brightness;
	struct sm5306_chip_data *pchip = bl_get_data(bl);
	enum sm5306_pwm_ctrl pwm_ctrl = pchip->pdata->pwm_ctrl;

	pr_err("[SM5306]%s\n", __func__);
	pr_err("[SM5306]pwm_ctrl = %d\n", pwm_ctrl);
	pr_err("[SM5306]bl->props.brightness = 0x%x\n", bl->props.brightness);
	pr_err("[SM5306]bl->props.power = 0x%x\n", bl->props.power);
	pr_err("[SM5306]bl->props.fb_blank = 0x%x\n", bl->props.fb_blank);
	pr_err("[SM5306]bl->props.state = 0x%x\n", bl->props.state);

	brightness = bl->props.brightness;

	if (bl->props.fb_blank != FB_BLANK_UNBLANK)
		brightness = 0;

	/* brightness 0 means disable */
	if (!brightness) {
		reg_val = 0x80;
		ret = regmap_update_bits(pchip->regmap,
					REG_CTRL, 0x80, reg_val);
		pr_err("[SM5306]regmap_update_bits(): 0x%x = 0x%x\n",
						REG_CTRL, reg_val);

		if (ret < 0)
			goto out;
		return brightness;
	}

	/* pwm control */
	if (pwm_ctrl == PWM_CTRL_BANK_ALL) {
		if (!set_intensity(bl, pchip))
			pr_err("No pwm control func. in plat-data\n");
	} else {
		/* i2c control */
		ret = regmap_update_bits(pchip->regmap, REG_CTRL, 0x80, 0x00);
		if (ret < 0)
			goto out;
		mdelay(1);

		/*LSB*/
		ret = regmap_write(pchip->regmap,
					REG_BRT_LSB, 0x00);
		pr_err("[SM5306]regmap_write(): 0x%x = 0x%x\n",
				REG_BRT_LSB, 0);
		if (ret < 0)
			goto out;

		/*MSB*/
		ret = regmap_write(pchip->regmap,
				   REG_BRT_MSB, bl->props.brightness - 1);
		pr_err("[SM5306]regmap_write(): 0x%x = 0x%x\n",
				REG_BRT_MSB, bl->props.brightness - 1);
		if (ret < 0)
			goto out;
	}
	return bl->props.brightness;
out:
	pr_err("i2c failed to access REG_CTRL\n");
	return bl->props.brightness;
}

static int sm5306_bank_a_get_brightness(struct backlight_device *bl)
{
	unsigned int reg_val;
	int brightness, ret;
	struct sm5306_chip_data *pchip = bl_get_data(bl);
	enum sm5306_pwm_ctrl pwm_ctrl = pchip->pdata->pwm_ctrl;

	pr_err("[SM5306]%s\n", __func__);
	pr_err("[SM5306]pwm_ctrl = %d\n", pwm_ctrl);

	if (pwm_ctrl == PWM_CTRL_BANK_ALL) {
		ret = regmap_read(pchip->regmap, REG_PWM_OUTHIGH, &reg_val);
		if (ret < 0)
			goto out;
		brightness = reg_val & 0x01;
		ret = regmap_read(pchip->regmap, REG_PWM_OUTLOW, &reg_val);
		if (ret < 0)
			goto out;
		brightness = ((brightness << 8) | reg_val) + 1;
	} else {
		/* i2c control */
		ret = regmap_update_bits(pchip->regmap, REG_CTRL, 0x80, 0x00);
		if (ret < 0)
			goto out;
		mdelay(1);
		ret = regmap_read(pchip->regmap, REG_BRT_MSB, &reg_val);
		pr_err("regmap_read(): 0x%x = 0x%x\n",
				REG_BRT_MSB, reg_val);
		if (ret < 0)
			goto out;
		brightness = reg_val + 1;
	}
	bl->props.brightness = brightness;
	return bl->props.brightness;
out:
	pr_err("i2c failed to access register\n");
	return 0;
}

static const struct backlight_ops sm5306_bank_a_ops = {
	.options = BL_CORE_SUSPENDRESUME,
	.update_status = sm5306_bank_a_update_status,
	.get_brightness = sm5306_bank_a_get_brightness,
};


static int sm5306_backlight_register(struct sm5306_chip_data *pchip,
				     enum sm5306_leds ledno)
{
	const char *name = bled_name[ledno];
	struct backlight_properties props;
	struct sm5306_platform_data *pdata = pchip->pdata;

	pr_err("[SM5306]%s\n", __func__);

	props.type = BACKLIGHT_RAW;
	props.brightness = pdata->init_brt_led;
	props.max_brightness = pdata->max_brt_led;
	pchip->bled =
	    backlight_device_register(name, pchip->dev, pchip,
				      &sm5306_bank_a_ops, &props);
	if (IS_ERR(pchip->bled))
		return PTR_ERR(pchip->bled);

	return 0;
}

static void sm5306_backlight_unregister(struct sm5306_chip_data *pchip)
{
	if (pchip->bled)
		backlight_device_unregister(pchip->bled);
}

static int sm5306_parse_dt(struct device *dev,
						struct sm5306_chip_data *pchip)
{
	struct sm5306_platform_data *pdata = pchip->pdata;
	int max_brt_led = 255;
	int init_brt_led = 175;

	pr_err("[SM5306]%s\n", __func__);

	if (dev == NULL) {
		pr_err("(dev == NULL)\n");
		return -1;
	}
#if 0
	if (np == NULL) {
		pr_err("(np == NULL)\n");
		return -1;
	}

	pr_err("dev = %d\n", dev);
	pr_err("name  = %s\n", np->name);
	/* get max_brt_led*/
	if (of_property_read_u32(np, "max_brt_led", &max_brt_led) < 0)
		pr_err("not max_brt_led,id property\n");
	pdata->max_brt_led = max_brt_led;
	pr_err("[SM5306]max_brt_led = %d\n", pdata->max_brt_led);

	/* get init_brt_led*/
	if (of_property_read_u32(np, "init_brt_led", &init_brt_led) < 0)
		pr_err("not init_brt_led,id property\n");
	pdata->init_brt_led = init_brt_led;
	pr_err("[SM5306]init_brt_led = %d\n", pdata->init_brt_led);
#endif
	pdata->max_brt_led = max_brt_led;
	pdata->init_brt_led = init_brt_led;

	return 0;
}

static const struct regmap_config sm5306_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = REG_MAX,
};

static int sm5306_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct sm5306_platform_data *pdata;
	struct sm5306_chip_data *pchip;
	int ret;

	pr_err("[SM5306]%s\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SM5306]fail : i2c functionality check...\n");
		return -EOPNOTSUPP;
	}

	pchip = devm_kzalloc(&client->dev, sizeof(struct sm5306_chip_data),
				GFP_KERNEL);
	if (!pchip)
		return -ENOMEM;

	pdata = devm_kzalloc(&client->dev, sizeof(struct sm5306_platform_data),
				GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pchip->pdata = pdata;
	pchip->dev = &client->dev;
	local_chip_data = pchip;

	ret = sm5306_parse_dt(&client->dev, pchip);
	if (ret < 0)
		goto err_chip_init;

	pchip->regmap = devm_regmap_init_i2c(client, &sm5306_regmap);
	if (IS_ERR(pchip->regmap)) {
		ret = PTR_ERR(pchip->regmap);
		pr_err("fail : allocate register map: %d\n", ret);
		return ret;
	}
	i2c_set_clientdata(client, pchip);
	client->addr = 0x36;
	/* chip initialize */
	ret = sm5306_chip_init(pchip);
	if (ret < 0) {
		pr_err("[SM5306]fail : init chip\n");
		goto err_chip_init;
	}

	ret = sm5306_backlight_register(pchip, BLED_ALL);
	if (ret < 0)
		goto err_bl_reg;

	/* interrupt enable  : irq 0 is not allowed for sm5306 */
/*
	pchip->irq = client->irq;
	if (pchip->irq)
		sm5306_intr_config(pchip);
*/
	pr_err("[SM5306]SM5306 backlight register OK.\n");
	return 0;

err_bl_reg:
	pr_err("[SM5306]fail : backlight register.\n");
	sm5306_backlight_unregister(pchip);
err_chip_init:
	return ret;
}

static int sm5306_remove(struct i2c_client *client)
{
	int ret;
	struct sm5306_chip_data *pchip = i2c_get_clientdata(client);

	pr_err("[SM5306]%s\n", __func__);

	ret = regmap_write(pchip->regmap, REG_BRT_LSB, 0);
	if (ret < 0)
		pr_err("i2c failed to access register\n");

	ret = regmap_write(pchip->regmap, REG_BRT_MSB, 0);
	if (ret < 0)
		pr_err("i2c failed to access register\n");

	sm5306_backlight_unregister(pchip);
	if (pchip->irq) {
		free_irq(pchip->irq, pchip);
		flush_workqueue(pchip->irqthread);
		destroy_workqueue(pchip->irqthread);
	}

	return 0;
}

static const struct i2c_device_id sm5306_id[] = {
        { "EXFG0000", 0 },
};
MODULE_DEVICE_TABLE(i2c, sm5306_id);


#ifdef CONFIG_ACPI
static struct acpi_device_id extbl_acpi_match[] = {
        {"EXFG0000", 0 },
        {}
};
MODULE_DEVICE_TABLE(acpi, extbl_acpi_match);
#endif


static struct i2c_driver sm5306_i2c_driver = {
	.driver = {
		.name = SM5306_NAME,
#ifdef CONFIG_ACPI
                .acpi_match_table = ACPI_PTR(extbl_acpi_match),
#endif
	},
	.probe = sm5306_probe,
	.remove = sm5306_remove,
	.id_table = sm5306_id,
};

module_i2c_driver(sm5306_i2c_driver);

MODULE_DESCRIPTION("Siliconmitus Backlight driver for SM5306");
MODULE_LICENSE("GPL v2");
