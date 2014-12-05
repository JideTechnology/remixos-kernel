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
	void __iomem *base_add_1, *base_add_2;
	int ret;
	unsigned int gpio_index, flags;
	struct device_node *node = pdev->dev.of_node;

	base_add_1 = of_iomap(node, 0);
	pr_warn("1-%p\n", base_add_1);
	base_add_2 = of_iomap(node, 1);
	pr_warn("2-%p\n", base_add_2);

	gpio_index = of_get_named_gpio_flags(node, "gpios", 0, &flags);
	pr_warn("gpio_index: %d\n", gpio_index);
	if (!gpio_is_valid(gpio_index)){
		pr_warn("gpio[%d]is invalid\n", gpio_index);
	}
	gpio_request(gpio_index,"GPIO_TEST");
	gpio_direction_output(gpio_index,1);

	pr_warn("[%s][%d]\n", __func__, __LINE__);
	pinctrl_get_select(&pdev->dev,"active");
	pr_warn("[%s][%d]\n", __func__, __LINE__);
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

