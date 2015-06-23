/*
 * sound\soc\sunxi\sunxi-cpudai.c
 * (C) Copyright 2014-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 * Liu shaohua <liushaohua@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <asm/dma.h>
#include <linux/dma/sunxi-dma.h>
#include "sun8iw10_cpudai.h"
#include "sunxi_rw_func.h"
#include "sun8iw10_codec.h"

#define DRV_NAME "sunxi-internal-cpudai"
#define SUNXI_PCM_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)

static int sun8iw10_cpudai_set_sysclk(struct snd_soc_dai *dai,
				  		int clk_id, unsigned int freq, int dir)
{
	struct sun8iw10_cpudai *sun8iw10_cpudai = snd_soc_dai_get_drvdata(dai);

	if (clk_set_rate(sun8iw10_cpudai->pllclk, freq)) {
		pr_err("[audio-cpudai]try to set the pll clk rate failed!\n");
	}
	return 0;
}

static struct snd_soc_dai_ops sun8iw10_cpudai_dai_ops = {
	.set_sysclk = sun8iw10_cpudai_set_sysclk,
};

static int sun8iw10_cpudai_probe(struct snd_soc_dai *dai)
{
	struct sun8iw10_cpudai *sun8iw10_cpudai = snd_soc_dai_get_drvdata(dai);

	dai->capture_dma_data = &sun8iw10_cpudai->capture_dma_param;
	dai->playback_dma_data = &sun8iw10_cpudai->play_dma_param;

	return 0;
}

static int sun8iw10_cpudai_suspend(struct snd_soc_dai *cpu_dai)
{
	struct sun8iw10_cpudai *sun8iw10_cpudai = snd_soc_dai_get_drvdata(cpu_dai);
	pr_debug("[internal-cpudai] suspend entered. %s\n", __func__);

	if (sun8iw10_cpudai->moduleclk != NULL)
		clk_disable(sun8iw10_cpudai->moduleclk);

	if (sun8iw10_cpudai->pllclk != NULL)
		clk_disable(sun8iw10_cpudai->pllclk);

	pr_debug("[internal-cpudai] suspend out. %s\n", __func__);
	return 0;
}

static int sun8iw10_cpudai_resume(struct snd_soc_dai *cpu_dai)
{
	struct sun8iw10_cpudai *sun8iw10_cpudai = snd_soc_dai_get_drvdata(cpu_dai);
	pr_debug("[internal-cpudai] resume entered. %s\n", __func__);

	if (sun8iw10_cpudai->pllclk != NULL) {
		if (clk_prepare_enable(sun8iw10_cpudai->pllclk)) {
			pr_err("open sun8iw10_cpudai->pllclk failed! line = %d\n", __LINE__);
		}
	}

	if (sun8iw10_cpudai->moduleclk != NULL) {
		if (clk_prepare_enable(sun8iw10_cpudai->moduleclk)) {
			pr_err("open sun8iw10_cpudai->moduleclk failed! line = %d\n", __LINE__);
		}
	}

	return 0;
}

static struct snd_soc_dai_driver sunxi_pcm_dai = {
	.probe = sun8iw10_cpudai_probe,
	.suspend 	= sun8iw10_cpudai_suspend,
	.resume 	= sun8iw10_cpudai_resume,
	.playback 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_PCM_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_PCM_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops 		= &sun8iw10_cpudai_dai_ops,

};

static const struct snd_soc_component_driver sun8iw10_cpudai_component = {
	.name		= DRV_NAME,
};
static const struct of_device_id sun8iw10_cpudai_of_match[] = {
	{ .compatible = "allwinner,sunxi-internal-cpudai", },
	{},
};

static int __init sunxi_internal_cpudai_platform_probe(struct platform_device *pdev)
{
	s32 ret = 0;
	const struct of_device_id *device;
	struct resource res;
	struct sun8iw10_cpudai *sun8iw10_cpudai;
	struct device_node *node = pdev->dev.of_node;

	if (!node) {
		dev_err(&pdev->dev,
			"can not get dt node for this device.\n");
		ret = -EINVAL;
		goto err0;
	}
	sun8iw10_cpudai = devm_kzalloc(&pdev->dev, sizeof(struct sun8iw10_cpudai), GFP_KERNEL);
	if (!sun8iw10_cpudai) {
		dev_err(&pdev->dev, "Can't allocate sun8iw10_cpudai.\n");
		ret = -ENOMEM;
		goto err0;
	}
	dev_set_drvdata(&pdev->dev, sun8iw10_cpudai);
	sun8iw10_cpudai->dai = sunxi_pcm_dai;
	sun8iw10_cpudai->dai.name = dev_name(&pdev->dev);

	device = of_match_device(sun8iw10_cpudai_of_match, &pdev->dev);
	if (!device)
		return -ENODEV;
	ret = of_address_to_resource(node, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "Can't parse device node resource\n");
		return -ENODEV;
	}

	sun8iw10_cpudai->pllclk = of_clk_get(node, 0);
	sun8iw10_cpudai->moduleclk= of_clk_get(node, 1);
	if (IS_ERR(sun8iw10_cpudai->pllclk) || IS_ERR(sun8iw10_cpudai->moduleclk)){
		dev_err(&pdev->dev, "[audio-cpudai]Can't get cpudai clocks\n");
		if (IS_ERR(sun8iw10_cpudai->pllclk))
			ret = PTR_ERR(sun8iw10_cpudai->pllclk);
		else
			ret = PTR_ERR(sun8iw10_cpudai->moduleclk);
		goto err1;
	} else {
		if (clk_set_parent(sun8iw10_cpudai->moduleclk, sun8iw10_cpudai->pllclk)) {
			pr_err("try to set parent of sunxi_spdif->moduleclk to sunxi_spdif->pllclk failed! line = %d\n",__LINE__);
		}
		clk_prepare_enable(sun8iw10_cpudai->pllclk);
		clk_prepare_enable(sun8iw10_cpudai->moduleclk);
	}

	sun8iw10_cpudai->play_dma_param.dma_addr = res.start+AC_ADC_TXDATA;
	sun8iw10_cpudai->play_dma_param.dma_drq_type_num = DRQDST_AUDIO_CODEC;
	sun8iw10_cpudai->play_dma_param.dst_maxburst = 4;
	sun8iw10_cpudai->play_dma_param.src_maxburst = 4;

	sun8iw10_cpudai->capture_dma_param.dma_addr = res.start+AC_ADC_RXDATA;
	sun8iw10_cpudai->capture_dma_param.dma_drq_type_num = DRQSRC_AUDIO_CODEC;
	sun8iw10_cpudai->capture_dma_param.src_maxburst = 4;
	sun8iw10_cpudai->capture_dma_param.dst_maxburst = 4;

	ret = snd_soc_register_component(&pdev->dev, &sun8iw10_cpudai_component,
				   &sun8iw10_cpudai->dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -ENOMEM;
		goto err1;
	}
	ret = asoc_dma_platform_register(&pdev->dev,0);
	if (ret) {
		dev_err(&pdev->dev, "Could not register PCM: %d\n", ret);
		goto err2;
	}

	return 0;

err2:
	snd_soc_unregister_component(&pdev->dev);
err1:
err0:
	return ret;

}

static int sunxi_internal_cpudai_platform_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver sunxi_internal_cpudai_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sun8iw10_cpudai_of_match,
	},
	.probe = sunxi_internal_cpudai_platform_probe,
	.remove = sunxi_internal_cpudai_platform_remove,
};
module_platform_driver(sunxi_internal_cpudai_driver);

/* Module information */
MODULE_AUTHOR("REUUIMLLA");
MODULE_DESCRIPTION("sunxi cpudai-internal SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);


