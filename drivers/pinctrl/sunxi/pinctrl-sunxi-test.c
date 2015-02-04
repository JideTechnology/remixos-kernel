/*
 * Allwinner A1X SoCs pinctrl driver.
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/io.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sys_config.h>
#include <linux/string.h>

static struct of_device_id sunxi_pinctrl_test_match[] = {
	{ .compatible = "allwinner,sun50i-vdevice"},
};
MODULE_DEVICE_TABLE(of, sunxi_pinctrl_match);



static int sunxi_pinctrl_test_probe(struct platform_device *pdev)
{
	struct gpio_config config;
	unsigned int ret;
	struct device_node *node;

	node=of_find_node_by_type(NULL, "vdevice");
	if(!node){
		printk("find node\n");
	}
	ret = of_get_named_gpio_flags(node, "test-gpios", 0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(ret)) {
		return -EINVAL;
	}
	printk("config: gpio=%d mul=%d drive=%d pull=%d data=%d\n"
				, config.gpio
				, config.mul_sel
				, config.drv_level
				, config.pull
				, config.data);
	return 0;
}

static struct platform_driver sunxi_pinctrl_test_driver = {
	.probe = sunxi_pinctrl_test_probe,
	.driver = {
		.name = "vdevice",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_pinctrl_test_match,
	},
};

static int __init sunxi_pinctrl_test_init(void)
{
	int ret;
	ret = platform_driver_register(&sunxi_pinctrl_test_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("register sunxi pinctrl platform driver failed\n");
		return -EINVAL;
	}
	return 0;
}
late_initcall(sunxi_pinctrl_test_init);

MODULE_AUTHOR("Huangshr<huangshr@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner SUNXI Pinctrl driver test");
MODULE_LICENSE("GPL");

