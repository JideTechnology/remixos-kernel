/*
 * drivers/pwm/pwm-sunxi.c
 *
 * Allwinnertech pulse-width-modulation controller driver
 *
 * Copyright (C) 2015 AllWinner
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

#include <linux/io.h>

#define PWM_DEBUG 0
#define PWM_NUM_MAX 4

#define PWM_ENABLE 4
#define PWM_POLARITY 5
#define PWM_CLK_GATE 6
#define PWM_PULSE_MODE 7
#define PWM_BYPASS 9
#define PWM_REG_CONTROL 0x00
#define PWM_REG_PERIOD 0x04

#define PWM_OFFSET 15

#if PWM_DEBUG
#define pwm_debug(msg...) pr_info
#else
#define pwm_debug(msg...)
#endif

struct sunxi_pwm_cfg {
	unsigned int period_ns;
	unsigned int duty_ns;
	unsigned int polarity;
	bool enabled;
};

struct sunxi_pwm_chip {
	struct pwm_chip chip;
	void __iomem *base;
};

static struct sunxi_pwm_cfg pwm_cfg[PWM_NUM_MAX];

static inline struct sunxi_pwm_chip *to_sunxi_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct sunxi_pwm_chip, chip);
}

static inline u32 sunxi_pwm_readl(struct pwm_chip *chip, u32 offset)
{
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);
	u32 value = 0;

   value = readl(pc->base + offset);

	return value;
}

static inline u32 sunxi_pwm_writel(struct pwm_chip *chip, u32 offset, u32 value)
{
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	writel(value, pc->base + offset);

	return 0;
}

static void sunxi_pwm_get_config(int pwm, struct sunxi_pwm_cfg *sunxi_pwm_cfg)
{

}

static int sunxi_pwm_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm, enum pwm_polarity polarity)
{
	u32 temp;

	temp = sunxi_pwm_readl(chip, PWM_REG_CONTROL);
	if (polarity == PWM_POLARITY_NORMAL)
		temp |= ((1 << PWM_POLARITY) << (PWM_OFFSET * pwm->pwm));
	else
		temp &= ~((1 << PWM_POLARITY) << (PWM_OFFSET * pwm->pwm));

	sunxi_pwm_writel(chip, PWM_REG_CONTROL, temp);

	return 0;
}

static int sunxi_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
		int duty_ns, int period_ns)
{
	u32 pre_scal[11][2] = {
		/* reg_value  clk_pre_div */
		{15, 1},
		{0, 120},
		{1, 180},
		{2, 240},
		{3, 360},
		{4, 480},
		{8, 12000},
		{9, 24000},
		{10, 36000},
		{11, 48000},
		{12, 72000}
		};
	u32 freq;
	u32 pre_scal_id = 0;
	u32 entire_cycles = 256;
	u32 active_cycles = 192;
	u32 entire_cycles_max = 65536;
	u32 temp;

	if (period_ns < 42) {
		/* if freq lt 24M, then direct output 24M clock */
		temp = sunxi_pwm_readl(chip, pwm->pwm * 0x10);
		temp |= (0x1 << PWM_BYPASS);//pwm bypass
		sunxi_pwm_writel(chip, pwm->pwm * 0x10, temp);

		return 0;
	}

	if (period_ns < 10667)
		freq = 93747;
	else if (period_ns > 1000000000)
		freq = 1;
	else
		freq = 1000000000 / period_ns;

	/* clock source rate is  24Mhz */
	entire_cycles = 24000000 / freq / pre_scal[pre_scal_id][1];

	while (entire_cycles > entire_cycles_max) {
		pre_scal_id++;

		if (pre_scal_id > 10)
			break;

		entire_cycles = 24000000 / freq / pre_scal[pre_scal_id][1];
	}

	if (period_ns < 5*100*1000)
		active_cycles = (duty_ns * entire_cycles + (period_ns/2)) /period_ns;
	else if (period_ns >= 5*100*1000 && period_ns < 6553500)
		active_cycles = ((duty_ns / 100) * entire_cycles + (period_ns /2 / 100)) / (period_ns/100);
	else
		active_cycles = ((duty_ns / 10000) * entire_cycles + (period_ns /2 / 10000)) / (period_ns/10000);

	temp = sunxi_pwm_readl(chip, PWM_REG_CONTROL);

	temp &= ~(0xf << (PWM_OFFSET * pwm->pwm));
	temp |= pre_scal[pre_scal_id][0] << (PWM_OFFSET * pwm->pwm);

	sunxi_pwm_writel(chip, PWM_REG_CONTROL, temp);

	sunxi_pwm_writel(chip, (pwm->pwm + 1)  * PWM_REG_PERIOD, ((entire_cycles - 1)<< 16) | active_cycles);

	pwm_debug("PWM _TEST: duty_ns=%d, period_ns=%d, freq=%d, per_scal=%d, period_reg=0x%x\n", duty_ns, period_ns, freq, pre_scal_id, temp);

	return 0;
}

static int sunxi_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	u32 temp;
	u32 value = 0;

#if 0
	int i;
	u32 ret = 0;
	char pin_name[255];
	u32 config;

	for (i = 0; i < pwm_pin_count[pwm->pwm]; i++) {
		ret = gpio_request(pwm_cfg[pwm->pwm].list[i].gpio.gpio, NULL);
		if (ret != 0) {
			pr_warn("pwm gpio %d request failed!\n", pwm_cfg[pwm->pwm].list[i].gpio.gpio);
		}
		if (!IS_AXP_PIN(pwm_cfg[pwm->pwm].list[i].gpio.gpio)) {
			sunxi_gpio_to_name(pwm_cfg[pwm->pwm].list[i].gpio.gpio, pin_name);
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pwm_cfg[pwm->pwm].list[i].gpio.mul_sel);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		else {
			pr_warn("this is axp pin!\n");
		}

		gpio_free(pwm_cfg[pwm->pwm].list[i].gpio.gpio);
	}
#endif

	temp = sunxi_pwm_readl(chip, PWM_REG_CONTROL);
	value |= (0x1 << PWM_ENABLE) | (0x1 << PWM_CLK_GATE);
	value = value << (PWM_OFFSET * pwm->pwm);
	temp |= value;

	sunxi_pwm_writel(chip, PWM_REG_CONTROL, temp);

	return 0;
}

static void sunxi_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	u32 temp;
	u32 value = 0;

#if 0
	int i;
	u32 ret = 0;
	char pin_name[255];
	u32 config;

	for (i = 0; i < pwm_pin_count[pwm->pwm]; i++) {
		ret = gpio_request(pwm_cfg[pwm->pwm].list[i].gpio.gpio, NULL);
		if (ret != 0) {
			pr_warn("pwm gpio %d request failed!\n", pwm_cfg[pwm->pwm].list[i].gpio.gpio);
		}
		if (!IS_AXP_PIN(pwm_cfg[pwm->pwm].list[i].gpio.gpio)) {
			sunxi_gpio_to_name(pwm_cfg[pwm->pwm].list[i].gpio.gpio, pin_name);
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0x7);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		else {
			pr_warn("this is axp pin!\n");
		}

		gpio_free(pwm_cfg[pwm->pwm].list[i].gpio.gpio);
	}
#endif

	temp = sunxi_pwm_readl(chip, 0);
	value |= (0x1 << PWM_ENABLE) | (0x1 << PWM_CLK_GATE);
	value = value << (PWM_OFFSET << pwm->pwm);
	temp &= ~value;

	sunxi_pwm_writel(chip, PWM_REG_CONTROL, temp);
}

static struct pwm_ops sunxi_pwm_ops = {
	.config = sunxi_pwm_config,
	.enable = sunxi_pwm_enable,
	.disable = sunxi_pwm_disable,
	.set_polarity = sunxi_pwm_set_polarity,
	.owner = THIS_MODULE,
};

static int sunxi_pwm_probe(struct platform_device *pdev)
{
	int ret;
	struct sunxi_pwm_chip* pwm;
	struct device_node *np = pdev->dev.of_node;
	int i;

	pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
	if (!pwm) {
		dev_err(&pdev->dev, "failed to allocate memory!\n");
		return -ENOMEM;
	}

	/* io map pwm base */
	pwm->base = (void __iomem *)of_iomap(pdev->dev.of_node, 0);
	if (!pwm->base) {
		dev_err(&pdev->dev, "unable to map pwm registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}

	/* read property pwm-number */
	ret = of_property_read_u32(np, "pwm-number", &pwm->chip.npwm);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get pwm number: %d, force to one!\n", ret);
		/* force to one pwm if read property fail */
		pwm->chip.npwm = 1;
	}
	pwm->chip.dev = &pdev->dev;
	pwm->chip.ops = &sunxi_pwm_ops;
	pwm->chip.base = -1;

	/* add pwm chip to pwm-core */
	ret = pwmchip_add(&pwm->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
		goto err_add;
	}
	platform_set_drvdata(pdev, pwm);

	for (i=0; i<pwm->chip.npwm; i++) {
		sunxi_pwm_get_config(i, &pwm_cfg[i]);
  }
    return 0;

err_add:
	iounmap(pwm->base);
err_iomap:
	return ret;
}

static int sunxi_pwm_remove(struct platform_device *pdev)
{
	struct sunxi_pwm_chip *pwm = platform_get_drvdata(pdev);

	return pwmchip_remove(&pwm->chip);
}

static int sunxi_pwm_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int sunxi_pwm_resume(struct platform_device *pdev)
{
	return 0;
}

#if !defined(CONFIG_OF)
struct platform_device sunxi_pwm_device = {
	.name = "sunxi_pwm",
	.id = -1,
};
#else
static const struct of_device_id sunxi_pwm_match[] = {
	{ .compatible = "allwinner,sunxi-pwm0", },
	{},
};
#endif

static struct platform_driver sunxi_pwm_driver = {
	.probe = sunxi_pwm_probe,
	.remove = sunxi_pwm_remove,
	.suspend = sunxi_pwm_suspend,
	.resume = sunxi_pwm_resume,
	.driver = {
		.name = "sunxi_pwm",
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_pwm_match,
	 },
};

static int __init pwm_module_init(void)
{
	int ret = 0;

	pr_info("pwm module init!\n");

#if !defined(CONFIG_OF)
	ret = platform_device_register(&sunxi_pwm_device);
#endif
	if (ret == 0) {
		ret = platform_driver_register(&sunxi_pwm_driver);
	}

	return ret;
}

static void __exit pwm_module_exit(void)
{
	pr_info("pwm module exit!\n");

	platform_driver_unregister(&sunxi_pwm_driver);
#if !defined(CONFIG_OF)
	platform_device_unregister(&sunxi_pwm_device);
#endif
}

subsys_initcall(pwm_module_init);
module_exit(pwm_module_exit);

MODULE_AUTHOR("tyle");
MODULE_DESCRIPTION("pwm driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pwm");
