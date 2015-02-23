/*
 * pwm-crc.c - Intel Crystal Cove PWM Driver
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Shobhit Kumar <shobhit.kumar@intel.com>
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/mfd/intel_soc_pmic.h>
#include <linux/pwm.h>

#define PWM0_CLK_DIV		0x4B
#define  PWM_OUTPUT_ENABLE	(1<<7)
#define  PWM_DIV_CLK_0		0x00 /* DIVIDECLK = BASECLK */
#define  PWM_DIV_CLK_100	0x63 /* DIVIDECLK = BASECLK/100 */
#define  PWM_DIV_CLK_128	0x7F /* DIVIDECLK = BASECLK/128 */

#define PWM0_DUTY_CYCLE		0x4E
#define BACKLIGHT_EN		0x51

#define PWM_MAX_LEVEL		0xFF

#define PWM_BASE_CLK		6000	/* 6 MHz */
#define PWM_MAX_PERIOD_NS	21333 /* 46.875KHz */

/**
 * struct crystalcove_pwm - Crystal Cove PWM controller
 * @chip: the abstract pwm_chip structure.
 * @regmap: the regmap from the parent device.
 */
struct crystalcove_pwm {
	struct pwm_chip chip;
	struct platform_device *pdev;
	struct regmap *regmap;
};

static inline struct crystalcove_pwm *to_crc_pwm(struct pwm_chip *pc)
{
	return container_of(pc, struct crystalcove_pwm, chip);
}

static int crc_pwm_enable(struct pwm_chip *c, struct pwm_device *pwm)
{
	struct crystalcove_pwm *crc_pwm = to_crc_pwm(c);

	regmap_write(crc_pwm->regmap, BACKLIGHT_EN, 1);

	return 0;
}

static void crc_pwm_disable(struct pwm_chip *c, struct pwm_device *pwm)
{
	struct crystalcove_pwm *crc_pwm = to_crc_pwm(c);

	regmap_write(crc_pwm->regmap, BACKLIGHT_EN, 0);
}

static int crc_pwm_config(struct pwm_chip *c, struct pwm_device *pwm,
				  int duty_ns, int period_ns)
{
	struct crystalcove_pwm *crc_pwm = to_crc_pwm(c);
	struct device *dev = &crc_pwm->pdev->dev;
	int level, percent;

	if (period_ns > PWM_MAX_PERIOD_NS) {
		dev_err(dev, "un-supported period_ns\n");
		return -1;
	}

	if (pwm->period != period_ns) {
		int clk_div;

		/* changing the clk divisor, need to disable fisrt */
		crc_pwm_disable(c, pwm);
		clk_div = PWM_BASE_CLK * period_ns / 1000000;

		regmap_write(crc_pwm->regmap, PWM0_CLK_DIV,
					clk_div | PWM_OUTPUT_ENABLE);

		/* enable back */
		crc_pwm_enable(c, pwm);
	}

	if (duty_ns > period_ns) {
		dev_err(dev, "duty cycle cannot be greater than cycle period\n");
		return -1;
	}

	/* change the pwm duty cycle */
	percent = duty_ns * 100 / period_ns;
	level = percent * PWM_MAX_LEVEL / 100;
	regmap_write(crc_pwm->regmap, PWM0_DUTY_CYCLE, level);

	return 0;
}

static const struct pwm_ops crc_pwm_ops = {
	.config = crc_pwm_config,
	.enable = crc_pwm_enable,
	.disable = crc_pwm_disable,
	.owner = THIS_MODULE,
};

static int crystalcove_pwm_probe(struct platform_device *pdev)
{
	struct crystalcove_pwm *pwm;
	int retval;
	struct device *dev = pdev->dev.parent;
	struct intel_soc_pmic *pmic = dev_get_drvdata(dev);

	pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
	if (!pwm)
		return -ENOMEM;

	pwm->chip.dev = &pdev->dev;
	pwm->chip.ops = &crc_pwm_ops;
	pwm->chip.base = -1;
	pwm->chip.npwm = 1;

	/* get the PMIC regmap */
	pwm->regmap = pmic->regmap;

	retval = pwmchip_add(&pwm->chip);
	if (retval < 0)
		return retval;

	dev_dbg(&pdev->dev, "crc-pwm probe successful\n");
	platform_set_drvdata(pdev, pwm);

	return 0;
}

static int crystalcove_pwm_remove(struct platform_device *pdev)
{
	struct crystalcove_pwm *pwm = platform_get_drvdata(pdev);
	int retval;

	retval = pwmchip_remove(&pwm->chip);
	if (retval < 0)
		return retval;

	dev_dbg(&pdev->dev, "crc-pwm driver removed\n");

	return 0;
}

static struct platform_driver crystalcove_pwm_driver = {
	.probe = crystalcove_pwm_probe,
	.remove = crystalcove_pwm_remove,
	.driver = {
		.name = "crystal_cove_pwm",
	},
};

module_platform_driver(crystalcove_pwm_driver);

MODULE_AUTHOR("Shobhit Kumar <shobhit.kumar@intel.com>");
MODULE_DESCRIPTION("Intel Crystal Cove PWM Driver");
MODULE_LICENSE("GPL v2");
