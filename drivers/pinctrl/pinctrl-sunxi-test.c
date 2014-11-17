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
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

struct pinctrl *devices_pinctrl[256];


#if defined(CONFIG_OF)
static struct of_device_id sunxi_pinctrl_test_match[] = {
	{ .compatible = "allwinner,sun50i-vdevice"},
};
MODULE_DEVICE_TABLE(of, sunxi_pinctrl_match);
#endif


int test_pinctrl_get_select_default(struct platform_device *pdev)
{
	struct pinctrl *pinctrl;
	const struct of_device_id *device;
	device = of_match_device(sunxi_pinctrl_test_match, &pdev->dev);
	if (!device){
		return -ENODEV;
	}
	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(pinctrl)) {
		dev_err(&pdev->dev, "can't get/select pinctrl\n");
		return PTR_ERR(pinctrl);
	}
	return 0;
}

static int sunxi_pinctrl_test_probe(struct platform_device *pdev)
{
	int ret = 0;
	pr_warn("enter probe..\n");
	//ret = test_pinctrl_get_select_default(pdev);
	pr_warn("exit probe..\n");

	return ret;
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

