/* drivers/mfd/sm5702_core.c
 * SM5702 Multifunction Device Driver
 * Charger / LED / FlashLED / FG
 *
 * Copyright (C) 2015 Siliconmitus Technology Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/mfd/core.h>
#include <linux/mfd/sm5702.h>
#include <linux/mfd/sm5702_irq.h>
#include <linux/power/charger/sm5702_charger.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#define SM5702_DECLARE_IRQ(irq) { \
	irq, irq, \
	irq##_NAME, IORESOURCE_IRQ }

#ifdef CONFIG_CHARGER_SM5702
const static struct resource sm5702_charger_res[] = {
	SM5702_DECLARE_IRQ(SM5702_NOBAT_IRQ),
    SM5702_DECLARE_IRQ(SM5702_CHGRSTF_IRQ),
    SM5702_DECLARE_IRQ(SM5702_VBUSLIMIT_IRQ),
    SM5702_DECLARE_IRQ(SM5702_VBUSOVP_IRQ),
    SM5702_DECLARE_IRQ(SM5702_VBUSUVLO_IRQ),
    SM5702_DECLARE_IRQ(SM5702_VBUSPOK_IRQ),
    SM5702_DECLARE_IRQ(SM5702_BATOVP_IRQ),
    SM5702_DECLARE_IRQ(SM5702_AICL_IRQ),
    
    SM5702_DECLARE_IRQ(SM5702_VSYSOLP_IRQ),
    SM5702_DECLARE_IRQ(SM5702_VSYSNG_IRQ),
    SM5702_DECLARE_IRQ(SM5702_VSYSOK_IRQ),
    SM5702_DECLARE_IRQ(SM5702_OTGFAIL_IRQ),
    SM5702_DECLARE_IRQ(SM5702_PRETMROFF_IRQ),
    SM5702_DECLARE_IRQ(SM5702_FASTTMROFF_IRQ),
    SM5702_DECLARE_IRQ(SM5702_DONE_IRQ),
    SM5702_DECLARE_IRQ(SM5702_TOPOFF_IRQ),   
};

static struct mfd_cell sm5702_charger_devs[] = {
	{
		.name			= "sm5702-charger",
		.num_resources	= ARRAY_SIZE(sm5702_charger_res),
		.id				= -1,
		.resources		= sm5702_charger_res,
	},
};
#endif /*CONFIG_CHARGER_SM5702*/

#ifdef CONFIG_FLED_SM5702
const static struct resource sm5702_fled_res[] = {
    SM5702_DECLARE_IRQ(SM5702_LOWBATT_IRQ),
    SM5702_DECLARE_IRQ(SM5702_ABSTMRON_IRQ),
    SM5702_DECLARE_IRQ(SM5702_BOOSTPOK_IRQ),
    SM5702_DECLARE_IRQ(SM5702_BOOSTPOK_NG_IRQ),
    SM5702_DECLARE_IRQ(SM5702_FLEDOPEN_IRQ),
    SM5702_DECLARE_IRQ(SM5702_FLEDSHORT_IRQ),
    SM5702_DECLARE_IRQ(SM5702_THEMREG_IRQ),
    SM5702_DECLARE_IRQ(SM5702_THEMSHDN_IRQ),	
};
static struct mfd_cell sm5702_fled_devs[] = {
	{
		.name			= "sm5702-fled",
		.num_resources	= ARRAY_SIZE(sm5702_fled_res),
		.id				= -1,
		.resources		= sm5702_fled_res,
	},
};
#endif /*CONFIG_FLED_SM5702*/




inline static int sm5702_read_device(struct i2c_client *i2c,
		int reg, int bytes, void *dest)
{
	int ret;
	if (bytes > 1) {
		ret = i2c_smbus_read_i2c_block_data(i2c, reg, bytes, dest);           
	} else {
		ret = i2c_smbus_read_byte_data(i2c, reg);
		if (ret < 0)
			return ret;
		*(unsigned char *)dest = (unsigned char)ret;
        
        //pr_err("%s : ret = 0x%x, reg = 0x%d, dest = 0x%d\n", __func__, ret, reg, *(unsigned char *)dest);
	}    
    
	return ret;
}

inline static int sm5702_write_device(struct i2c_client *i2c,
		int reg, int bytes, const void *src)
{
	int ret;
	const uint8_t *data;
	if (bytes > 1)
		ret = i2c_smbus_write_i2c_block_data(i2c, reg, bytes, src);
	else {
		data = src;
		ret = i2c_smbus_write_byte_data(i2c, reg, *data);
        
        //pr_err("%s : ret = 0x%x, reg = 0x%x, data = 0x%x\n", __func__, ret, reg, *data);
	}
        
	return ret;
}

int sm5702_block_read_device(struct i2c_client *i2c,
		int reg, int bytes, void *dest)
{
	struct sm5702_mfd_chip *chip = i2c_get_clientdata(i2c);
	int ret;
	mutex_lock(&chip->io_lock);
	ret = sm5702_read_device(i2c, reg, bytes, dest);
	mutex_unlock(&chip->io_lock);
	return ret;
}
EXPORT_SYMBOL(sm5702_block_read_device);

int sm5702_block_write_device(struct i2c_client *i2c,
		int reg, int bytes, const void *src)
{
	struct sm5702_mfd_chip *chip = i2c_get_clientdata(i2c);
	int ret;
	mutex_lock(&chip->io_lock);
	ret = sm5702_write_device(i2c, reg, bytes, src);
	mutex_unlock(&chip->io_lock);
	return ret;
}
EXPORT_SYMBOL(sm5702_block_write_device);

int sm5702_reg_read(struct i2c_client *i2c, int reg)
{
	struct sm5702_mfd_chip *chip = i2c_get_clientdata(i2c);
	unsigned char data = 0;
	int ret;

	mutex_lock(&chip->io_lock);
	ret = sm5702_read_device(i2c, reg, 1, &data);
	mutex_unlock(&chip->io_lock);

    //pr_err("%s : ret = 0x%x, reg = 0x%x, data = 0x%x\n", __func__, ret, reg, data);

	if (ret < 0)
		return ret;
	else
		return (int)data;
}
EXPORT_SYMBOL(sm5702_reg_read);

int sm5702_reg_write(struct i2c_client *i2c, int reg,
		unsigned char data)
{
	struct sm5702_mfd_chip *chip = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&chip->io_lock);
	ret = i2c_smbus_write_byte_data(i2c, reg, data);
	mutex_unlock(&chip->io_lock);

    //pr_err("%s : ret = 0x%x, reg = 0x%x, data = 0x%x\n", __func__, ret, reg, data);

	return ret;
}
EXPORT_SYMBOL(sm5702_reg_write);

int sm5702_assign_bits(struct i2c_client *i2c, int reg,
		unsigned char mask, unsigned char data)
{
	struct sm5702_mfd_chip *chip = i2c_get_clientdata(i2c);
	unsigned char value;
	int ret;

	mutex_lock(&chip->io_lock);
	ret = sm5702_read_device(i2c, reg, 1, &value);

	if (ret < 0)
		goto out;

	value &= ~mask;
	value |= data;
	ret = i2c_smbus_write_byte_data(i2c, reg, value);

    //pr_err("%s : ret = 0x%x, reg = 0x%x, value = 0x%x, data = 0x%x\n", __func__, ret, reg, value,data);

out:
	mutex_unlock(&chip->io_lock);
	return ret;
}
EXPORT_SYMBOL(sm5702_assign_bits);

int sm5702_set_bits(struct i2c_client *i2c, int reg,
		unsigned char mask)
{
	return sm5702_assign_bits(i2c,reg,mask,mask);
}
EXPORT_SYMBOL(sm5702_set_bits);

int sm5702_clr_bits(struct i2c_client *i2c, int reg,
		unsigned char mask)
{
	return sm5702_assign_bits(i2c,reg,mask,0);
}
EXPORT_SYMBOL(sm5702_clr_bits);



	/*pr_err("%s: ret = %d, addr = 0x%x, data = 0x%x\n",
	__func__, ret, reg_addr, data);*/



	/* not use big endian*/
	/*data = cpu_to_be16(data);*/

	/*pr_err("%s: ret = %d, addr = 0x%x, data = 0x%x\n",
	__func__, ret, reg_addr, data);*/

void sm5702_cf_dump_read(struct i2c_client *i2c)
{
	uint8_t data;
	int i;

    	pr_err("%s: dump start\n", __func__);

	for (i = SM5702_INTMSK1; i <= SM5702_FLEDCNTL6; i++) {
		data = sm5702_reg_read(i2c, i);
		pr_err("0x%0x = 0x%02x, ", i, data);
	}
    
	pr_err("%s: dump end\n", __func__);
}
EXPORT_SYMBOL(sm5702_cf_dump_read);

extern int sm5702_init_irq(sm5702_mfd_chip_t *chip);
extern int sm5702_exit_irq(sm5702_mfd_chip_t *chip);

static int sm5702_mfd_probe(struct i2c_client *i2c,
						const struct i2c_device_id *id)
{
	int ret = 0;
	sm5702_mfd_chip_t *chip;
	//sm5702_mfd_platform_data_t *pdata = i2c->dev.platform_data;
#ifdef CONFIG_ACPI    
    struct gpio_desc *gpio;
#endif

	pr_err("%s : SM5702 MFD Driver %s start probing\n",
            __func__, SM5702_DRV_VER);

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (chip == NULL) {
		dev_err(&i2c->dev, "Memory is not enough.\n");
		ret = -ENOMEM;
		goto err_mfd_nomem;
	}
	chip->dev = &i2c->dev;

	ret = i2c_check_functionality(i2c->adapter, I2C_FUNC_SMBUS_BYTE_DATA |
			I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_I2C_BLOCK);
	if (!ret) {
		ret = i2c_get_functionality(i2c->adapter);
		pr_err("I2C functionality is not supported.\n");
		ret = -ENOSYS;
		goto err_i2cfunc_not_support;
	}

	chip->i2c_client = i2c;
	chip->i2c_client->addr = SM5702CF_SLAVE_ADDR;
	//chip->pdata = pdata;

/* 
    if (pdata->irq_base < 0)
        pdata->irq_base = irq_alloc_descs(-1, 0, SM5702_IRQS_NR, 0);
	if (pdata->irq_base < 0) {
		pr_err("%s:%s irq_alloc_descs Fail! ret(%d)\n",
				"sm5702-mfd", __func__, pdata->irq_base);
		ret = -EINVAL;
		goto irq_base_err;
	} else {
		chip->irq_base = pdata->irq_base;
		pr_err("%s:%s irq_base = %d\n",
				"sm5702-mfd", __func__, chip->irq_base);
	}
*/
        chip->irq_base = irq_alloc_descs(-1, 0, SM5702_IRQS_NR, 0);
	if (chip->irq_base < 0) {
		pr_err("%s:%s irq_alloc_descs Fail! ret(%d)\n",
				"sm5702-mfd", __func__, chip->irq_base);
		ret = -EINVAL;
		goto irq_base_err;
	} else {
		pr_err("%s:%s irq_base = %d\n",
				"sm5702-mfd", __func__, chip->irq_base);
	}
#ifdef CONFIG_ACPI
        gpio = devm_gpiod_get_index(chip->dev, "chrg_int", 0);
        pr_err("%s: gpio=%d\n", __func__, gpio);
        if (IS_ERR(gpio)) {
                pr_err("acpi gpio get index failed\n");
                return PTR_ERR(gpio);
        } else {
                ret = gpiod_direction_input(gpio);
                if (ret < 0)
                        pr_err("SM5702 CHRG set direction failed\n");

                chip->irq = gpiod_to_irq(gpio);
                pr_err("%s: irq=%d\n", __func__, chip->irq);
        }
#endif     

	i2c_set_clientdata(i2c, chip);
	mutex_init(&chip->io_lock);
	mutex_init(&chip->suspend_flag_lock);

	wake_lock_init(&(chip->irq_wake_lock), WAKE_LOCK_SUSPEND,
			"sm5702mfd_wakelock");

	ret = sm5702_init_irq(chip);
	if (ret < 0) {
		pr_err("Error : can't initialize SM5702 MFD irq\n");
		goto err_init_irq;
	}
 
#ifdef CONFIG_CHARGER_SM5702
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(3,6,0))
	ret = mfd_add_devices(chip->dev, 0, &sm5702_charger_devs[0],
			ARRAY_SIZE(sm5702_charger_devs),
			NULL, chip->irq_base, NULL);
#else
	ret = mfd_add_devices(chip->dev, 0, &sm5702_charger_devs[0],
			ARRAY_SIZE(sm5702_charger_devs),
			NULL, chip->irq_base);
#endif
	if (ret<0) {
		dev_err(chip->dev, "Failed : add charger devices\n");
		goto err_add_chg_devs;
	}
	pr_err("%s : CONFIG_CHARGER_SM5702 MFD ADD\n",__func__);
#endif /*CONFIG_CHARGER_SM5702*/


#ifdef CONFIG_FLED_SM5702
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(3,6,0))
	ret = mfd_add_devices(chip->dev, 2, &sm5702_fled_devs[0],
			ARRAY_SIZE(sm5702_fled_devs),
			NULL, chip->irq_base, NULL);
#else
	ret = mfd_add_devices(chip->dev, 2, &sm5702_fled_devs[0],
			ARRAY_SIZE(sm5702_fled_devs),
			NULL, chip->irq_base);
#endif
	if (ret < 0) {
		dev_err(chip->dev, "Failed : add FlashLED devices");
		goto err_add_fled_devs;
	}
#endif /*CONFIG_FLED_SM5702*/
	sm5702_cf_dump_read(chip->i2c_client);

	pr_err("%s : SM5702 MFD Driver Fin probe\n", __func__);
	return ret;

#ifdef CONFIG_CHARGER_SM5702
err_add_chg_devs:
#endif /*CONFIG_FUEL_GAUGE_SM5702*/
#ifdef CONFIG_FLED_SM5702
err_add_fled_devs:
#endif /*CONFIG_FLED_SM5702*/
	mfd_remove_devices(chip->dev);
err_init_irq:
	wake_lock_destroy(&(chip->irq_wake_lock));
	mutex_destroy(&chip->io_lock);
	kfree(chip);
irq_base_err:
err_mfd_nomem:
err_i2cfunc_not_support:
	return ret;
}

static int sm5702_mfd_remove(struct i2c_client *i2c)
{
	sm5702_mfd_chip_t *chip = i2c_get_clientdata(i2c);

	pr_err("%s : SM5702 MFD Driver remove\n", __func__);

	mfd_remove_devices(chip->dev);
	wake_lock_destroy(&(chip->irq_wake_lock));
	mutex_destroy(&chip->suspend_flag_lock);
	mutex_destroy(&chip->io_lock);
	kfree(chip);

	return 0;
}

#ifdef CONFIG_PM
extern int sm5702_irq_suspend(sm5702_mfd_chip_t *chip);
extern int sm5702_irq_resume(sm5702_mfd_chip_t *chip);
int sm5702_mfd_suspend(struct device *dev)
{

	struct i2c_client *i2c =
		container_of(dev, struct i2c_client, dev);

	sm5702_mfd_chip_t *chip = i2c_get_clientdata(i2c);
	BUG_ON(chip == NULL);
	return sm5702_irq_suspend(chip);
}

int sm5702_mfd_resume(struct device *dev)
{
	struct i2c_client *i2c =
		container_of(dev, struct i2c_client, dev);
	sm5702_mfd_chip_t *chip = i2c_get_clientdata(i2c);
	BUG_ON(chip == NULL);
	return sm5702_irq_resume(chip);
}
#endif /* CONFIG_PM */




static void sm5702_mfd_shutdown(struct i2c_client *client)
{
	struct i2c_client *i2c = client;

    sm5702_assign_bits(i2c, SM5702_CNTL, SM5702_OPERATION_MODE, SM5702_OPERATION_MODE_CHARGING_ON);
}

#ifdef CONFIG_PM
const struct dev_pm_ops sm5702_pm = {
	.suspend = sm5702_mfd_suspend,
	.resume = sm5702_mfd_resume,
};
#endif

#ifdef CONFIG_ACPI
static const struct i2c_device_id sm5702_mfd_id_table[] = {
	{ "EXCG0000", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, sm5702_id_table);

static struct acpi_device_id sm5702_acpi_match[] = {
        { "EXCG0000", 0}, 
        { },
};
MODULE_DEVICE_TABLE(acpi, pmic_acpi_match);
#else
static const struct i2c_device_id sm5702_mfd_id_table[] = {
	{"sm5702-mfd", 0},
	{}
};
#endif

static struct i2c_driver sm5702_mfd_driver = {
	.driver	= {
		.name	= "sm5702-mfd",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm		= &sm5702_pm,
#endif
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(sm5702_acpi_match),
#endif
	},
	.shutdown = sm5702_mfd_shutdown,
	.probe		= sm5702_mfd_probe,
	.remove		= sm5702_mfd_remove,
#ifdef CONFIG_ACPI	
	.id_table	= sm5702_mfd_id_table,
#else
	.id_table	= sm5702_mfd_id_table,
#endif	
};

static int __init sm5702_mfd_i2c_init(void)
{
	int ret;

	pr_err("%s : SM5702 init\n", __func__);
	ret = i2c_add_driver(&sm5702_mfd_driver);
	if (ret != 0)
		pr_err("%s : Failed to register SM5702 MFD I2C driver\n",
		__func__);

	return ret;
}
subsys_initcall(sm5702_mfd_i2c_init);

static void __exit sm5702_mfd_i2c_exit(void)
{
	i2c_del_driver(&sm5702_mfd_driver);
}
module_exit(sm5702_mfd_i2c_exit);

MODULE_DESCRIPTION("Siliconmitus SM5702 MFD I2C Driver");
MODULE_VERSION(SM5702_DRV_VER);
MODULE_LICENSE("GPL");
