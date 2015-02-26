/*
 * sound\soc\sunxi\sunxi_sndsun50iw1.c
 * (C) Copyright 2014-2017
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 * liushaohua <liushaohua@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/input.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <linux/of.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include "sunxi_sun50iw1codec.h"
#include <linux/delay.h>


struct mc_private {
	struct delayed_work hs_insert_work;
	struct delayed_work hs_remove_work;
	struct delayed_work hs_button_work;
	struct mutex jack_mlock;
	struct snd_soc_jack jack;
	struct snd_soc_codec *codec;
	u32 state;	/*switch state: headphone,headset,idle */
	u32 jackirq;	/*switch irq*/
	u32 HEADSET_DATA;	/*threshod for switch insert*/
	u32 headset_basedata;
	u32 switch_status;
	u32 aif2master;
	u32 aif2fmt;
	u32 aif3fmt;
	struct timeval tv_headset_plugin;/*4*/
	u32 key_volup;
	u32 key_voldown;
	u32 key_hook;
};

static int  switch_state 		= 0;

/* Identify the jack type as Headset/Headphone/None */
static int sun50iw1_check_jack_type(struct snd_soc_jack *jack)
{
	u32 reg_val;
	u32 jack_type = 0,tempdata = 0,flag = 0;
	struct mc_private *ctx = container_of(jack, struct mc_private, jack);
	reg_val = snd_soc_read(ctx->codec, SUNXI_HMIC_STS);
	tempdata = (reg_val>>HMIC_DATA)&0x1f;

	while(tempdata >= 0x1D && flag != 20){
		msleep(45);
		tempdata = snd_soc_read(ctx->codec, SUNXI_HMIC_STS);
		tempdata &=(0x1f<<8);
		tempdata = tempdata>>8;
		flag++;
	}
	pr_debug("tempdata:%x,ctx->HEADSET_DATA:%x,reg_val:%x\n",tempdata,ctx->HEADSET_DATA,reg_val);

	ctx->headset_basedata = tempdata;

	if (tempdata >= ctx->HEADSET_DATA) {/*headset:4*/
		jack_type = SND_JACK_HEADSET;
	} else {/*headphone:3*/
		/*disable hbias and adc*/
		snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<HMICBIASEN), (0x0<<HMICBIASEN));
		snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<MICADCEN), (0x0<<MICADCEN));
		jack_type = SND_JACK_HEADPHONE;
	}
	return jack_type;
}

/* Checks jack insertion and identifies the jack type.*/
static void sun50iw1_check_hs_insert_status(struct work_struct *work)
{
	struct mc_private *ctx = container_of(work, struct mc_private, hs_insert_work.work);
	int jack_type = 0,reg_val = 0;

	mutex_lock(&ctx->jack_mlock);
	jack_type = sun50iw1_check_jack_type(&ctx->jack);
	if (jack_type != ctx->switch_status){
		ctx->switch_status = jack_type;
		snd_jack_report(ctx->jack.jack, jack_type);
		pr_debug("switch:%d\n",jack_type);
		switch_state = jack_type;
	}

	/*if SND_JACK_HEADSET,enable mic detect irq*/
	if (jack_type == SND_JACK_HEADSET){/*4*/
		/*clear headset insert pending.*/
		reg_val = snd_soc_read(ctx->codec, SUNXI_HMIC_STS);
		ctx->headset_basedata = (reg_val>>HMIC_DATA)&0x1f;
		ctx->headset_basedata -= 2;
		/*disable autowrite*/
		snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL2, (0x1f<<MDATA_THRESHOLD), (ctx->headset_basedata<<MDATA_THRESHOLD));
		snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL1, (0x1<<MIC_DET_IRQ_EN), (0x1<<MIC_DET_IRQ_EN));
		do_gettimeofday(&ctx->tv_headset_plugin);
	} else if (jack_type == SND_JACK_HEADPHONE) {
	/*if is HEADPHONE 3,close mic detect irq*/
		snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL1, (0x1<<MIC_DET_IRQ_EN), (0x0<<MIC_DET_IRQ_EN));
		ctx->tv_headset_plugin.tv_sec = 0;
	}
	snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL1, (0x1<<JACK_OUT_IRQ_EN), (0x1<<JACK_OUT_IRQ_EN));

	mutex_unlock(&ctx->jack_mlock);
}

/* Check for hook release */
static void sun50iw1_check_hs_button_status(struct work_struct *work)
{
	struct mc_private *ctx = container_of(work, struct mc_private, hs_button_work.work);
	u32 i = 0;
	mutex_lock(&ctx->jack_mlock);
	for (i = 0;i < 5; i++){
		if (ctx->key_hook == 0 ){
			pr_debug("Hook (2)!!\n");
			ctx->switch_status &= ~SND_JACK_BTN_0;
			snd_jack_report(ctx->jack.jack, ctx->switch_status);
			break;
		}
		msleep(8);
	}
	mutex_unlock(&ctx->jack_mlock);
}
/* Checks jack removal. */
static void sun50iw1_check_hs_remove_status(struct work_struct *work)
{
	struct mc_private *ctx = container_of(work, struct mc_private, hs_remove_work.work);
	mutex_lock(&ctx->jack_mlock);
	ctx->switch_status = 0;
	/*clear headset pulgout pending.*/
	snd_jack_report(ctx->jack.jack, ctx->switch_status);
	switch_state = ctx->switch_status;
	pr_debug("switch:%d\n",ctx->switch_status);
	ctx->tv_headset_plugin.tv_sec = 0;

	mutex_unlock(&ctx->jack_mlock);
}
static irqreturn_t jack_interrupt(int irq, void *dev_id)
{
	struct mc_private *ctx = dev_id;
	struct timeval tv;
	u32 tempdata = 0,regval = 0;
	if ((snd_soc_read(ctx->codec, SUNXI_HMIC_STS)&(1<<JACK_DET_IIN_ST)) && (snd_soc_read(ctx->codec, SUNXI_HMIC_CTRL1)&(1<<JACK_IN_IRQ_EN))) {/*headphone insert*/
		snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL1, (0x1<<JACK_IN_IRQ_EN), (0x0<<JACK_IN_IRQ_EN));
		regval = snd_soc_read(ctx->codec, SUNXI_HMIC_STS);
		regval |= 0x1<<JACK_DET_IIN_ST;
		regval &= ~(0x1<<JACK_DET_OUT_ST);
		snd_soc_write(ctx->codec, SUNXI_HMIC_STS, regval);
		/*enable hbias and adc*/
		snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<HMICBIASEN), (0x1<<HMICBIASEN));
		snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<MICADCEN), (0x1<<MICADCEN));
		schedule_delayed_work(&ctx->hs_insert_work,msecs_to_jiffies(500));

	} else if((snd_soc_read(ctx->codec, SUNXI_HMIC_STS)&(1<<JACK_DET_OUT_ST)) &&(snd_soc_read(ctx->codec, SUNXI_HMIC_CTRL1)&(1<<JACK_OUT_IRQ_EN))){/*headphone plugout*/
		snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL1, (0x1<<JACK_OUT_IRQ_EN), (0x0<<JACK_OUT_IRQ_EN));

		regval = snd_soc_read(ctx->codec, SUNXI_HMIC_STS);
		regval &= ~(0x1<<JACK_DET_IIN_ST);
		regval |= 0x1<<JACK_DET_OUT_ST;
		snd_soc_write(ctx->codec, SUNXI_HMIC_STS, regval);

		snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL1, (0x1<<MIC_DET_IRQ_EN), (0x0<<MIC_DET_IRQ_EN));

		/*diable hbias and adc*/
		snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<HMICBIASEN), (0x0<<HMICBIASEN));
		snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<MICADCEN), (0x0<<MICADCEN));
		snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL1, (0x1<<JACK_IN_IRQ_EN), (0x1<<JACK_IN_IRQ_EN));

		schedule_delayed_work(&ctx->hs_remove_work,msecs_to_jiffies(1));
	} else if (snd_soc_read(ctx->codec, SUNXI_HMIC_STS)&(1<<MIC_DET_ST)) {/*key*/
		regval = snd_soc_read(ctx->codec, SUNXI_HMIC_STS);
		regval &= ~(0x1<<JACK_DET_IIN_ST);
		regval &= ~(0x1<<JACK_DET_OUT_ST);
		regval |= 0x1<<MIC_DET_ST;
		snd_soc_write(ctx->codec, SUNXI_HMIC_STS, regval);

		do_gettimeofday(&tv);
		if ((tv.tv_sec > ctx->tv_headset_plugin.tv_sec) &&(tv.tv_sec - ctx->tv_headset_plugin.tv_sec > 2)){
			tempdata =snd_soc_read(ctx->codec, SUNXI_HMIC_STS);
			tempdata = (tempdata&0x1f00)>>8;
			if(tempdata == 2){
				ctx->key_hook = 0;
				ctx->key_voldown = 0;
				ctx->key_volup ++;
				if(ctx->key_volup == 60) {
					pr_debug("Volume + !!\n");
					ctx->key_volup = 0;
					ctx->switch_status |= SND_JACK_BTN_1;
					snd_jack_report(ctx->jack.jack, ctx->switch_status);
					ctx->switch_status &= ~SND_JACK_BTN_1;
					snd_jack_report(ctx->jack.jack, ctx->switch_status);

				}
			}else if (tempdata == 5) {
				ctx->key_volup = 0;
				ctx->key_hook = 0;
				ctx->key_voldown ++;
				if(ctx->key_voldown == 60) {
					pr_debug("Volume - !!\n");
					ctx->key_voldown = 0;
					ctx->switch_status |= SND_JACK_BTN_2;
					snd_jack_report(ctx->jack.jack, ctx->switch_status);
					ctx->switch_status &= ~SND_JACK_BTN_2;
					snd_jack_report(ctx->jack.jack, ctx->switch_status);
				}
			} else if(tempdata == 1 || tempdata == 0) {
				ctx->key_volup = 0;
				ctx->key_voldown = 0;
				ctx->key_hook ++;
				if(ctx->key_hook >= 40){
					ctx->key_hook = 1;
					if ((ctx->switch_status & SND_JACK_BTN_0) == 0){
						ctx->switch_status |= SND_JACK_BTN_0;
						snd_jack_report(ctx->jack.jack, ctx->switch_status);
						pr_debug("Hook (1)!!\n");
					}
					schedule_delayed_work(&ctx->hs_button_work,msecs_to_jiffies(180));
				}


			} else {
				pr_debug("tempdata:%d,Key data err:\n",tempdata);
				ctx->key_volup = 0;
				ctx->key_voldown = 0;
				ctx->key_hook = 0;
			}
		} else {

		}
	}
	return IRQ_HANDLED;
}

static const struct snd_kcontrol_new ac_pin_controls[] = {
	SOC_DAPM_PIN_SWITCH("External Speaker"),
	SOC_DAPM_PIN_SWITCH("Headphone"),
	SOC_DAPM_PIN_SWITCH("Earpiece"),
	SOC_DAPM_PIN_SWITCH("src clk"),
};

static const struct snd_soc_dapm_widget sun50iw1_ac_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("External MainMic", NULL),
	SND_SOC_DAPM_MIC("HeadphoneMic", NULL),
	SND_SOC_DAPM_MIC("DigitalMic", NULL),
};

static const struct snd_soc_dapm_route audio_map[] = {
	{"MainMic Bias", NULL, "External MainMic"},
	{"MIC1P", NULL, "MainMic Bias"},
	{"MIC1N", NULL, "MainMic Bias"},

	{"MIC2", NULL, "HeadphoneMic"},
	/*d-mic*/
	{"MainMic Bias", NULL, "DigitalMic"},
	{"D_MIC", NULL, "MainMic Bias"},
};

/*
 * Card initialization
 */
static int sunxi_audio_init(struct snd_soc_pcm_runtime *runtime)
{
	//int ret;
	struct snd_soc_codec *codec = runtime->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct mc_private *ctx = snd_soc_card_get_drvdata(runtime->card);
	ctx->codec = runtime->codec;
	snd_soc_dapm_disable_pin(&codec->dapm,	"HPOUTR");
	snd_soc_dapm_disable_pin(&codec->dapm,	"HPOUTL");
	snd_soc_dapm_disable_pin(&codec->dapm,	"EAROUTP");
	snd_soc_dapm_disable_pin(&codec->dapm,	"EAROUTN");
	snd_soc_dapm_disable_pin(&codec->dapm,	"SPKL");
	snd_soc_dapm_disable_pin(&codec->dapm,	"SPKR");

	snd_soc_dapm_disable_pin(&runtime->card->dapm,	"External Speaker");
	snd_soc_dapm_disable_pin(&runtime->card->dapm,	"Headphone");
	snd_soc_dapm_disable_pin(&runtime->card->dapm,	"Earpiece");
	snd_soc_dapm_disable_pin(&runtime->card->dapm,	"src clk");
	snd_soc_dapm_sync(dapm);
	return 0;
}
static int sunxi_sndpcm_hw_params(struct snd_pcm_substream *substream,
                                       struct snd_pcm_hw_params *params)
{
	int ret  = 0;
	u32 freq_in = 22579200;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	unsigned long sample_rate = params_rate(params);

	switch (sample_rate) {
		case 8000:
		case 16000:
		case 32000:
		case 64000:
		case 128000:
		case 12000:
		case 24000:
		case 48000:
		case 96000:
		case 192000:
			freq_in = 24576000;
			break;
	}
	/* set the codec FLL */
	ret = snd_soc_dai_set_pll(codec_dai, PLLCLK, 0, freq_in, freq_in);
	if (ret < 0) {
		pr_err("err:%s,set codec dai pll failed.\n", __func__);
		return ret;
	}
	/*set system clock source freq */
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, freq_in, 0);
	if (ret < 0) {
		pr_err("err:%s,set cpu dai sysclk failed.\n", __func__);
		return ret;
	}
	/*set system clock source freq_in and set the mode as tdm or pcm*/
	ret = snd_soc_dai_set_sysclk(codec_dai, AIF1_CLK, freq_in, 0);
	if (ret < 0) {
		pr_err("err:%s,set codec dai sysclk faided.\n", __func__);
		return ret;
	}
	/*set system fmt:api2s:master aif1:slave*/
	ret = snd_soc_dai_set_fmt(cpu_dai, 0);
	if (ret < 0) {
		pr_err("%s,set cpu dai fmt failed.\n", __func__);
		return ret;
	}

	/*
	* codec: slave. AP: master
	*/
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		pr_err("%s,set codec dai fmt failed.\n", __func__);
		return ret;
	}

	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0) {
		pr_err("%s, set cpu dai clkdiv faided.\n", __func__);
		return ret;
	}

	return 0;
}

static struct snd_soc_ops sunxi_sndpcm_ops = {
       .hw_params              = sunxi_sndpcm_hw_params,
};

static int bb_voice_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_card *card = rtd->card;
	struct mc_private *ctx = NULL;

	int ret = 0;
	int freq_in = 24576000;
	if (params_rate(params) != 8000)
		return -EINVAL;
	ctx = snd_soc_card_get_drvdata(card);
	if (ctx == NULL ){
		pr_err("err:%s,get ctx failed.\n",__func__);
		return -EINVAL;
	}
	/* set the codec aif1clk/aif2clk from pllclk */
	ret = snd_soc_dai_set_pll(codec_dai, PLLCLK, 0, freq_in,freq_in);
	if (ret < 0) {
		pr_err("err:%s,set codec dai pll failed.\n", __func__);
		return ret;
	}
	/*set system clock source aif2*/
	ret = snd_soc_dai_set_sysclk(codec_dai, AIF2_CLK , 0, 0);
	if (ret < 0) {
		pr_err("err:%s,set codec dai sysclk faied\n", __func__);
		return ret;
	}
	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A | (ctx->aif2fmt << 8) | (ctx->aif2master << 12));
	if (ret < 0) {
		pr_err("err:%s,set codec dai fmt failed.\n", __func__);
	}
	return ret;
}

static struct snd_soc_ops bb_voice_ops = {
	.hw_params = bb_voice_hw_params,
};
static int bb_clk_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_card *card = rtd->card;
	struct mc_private *ctx = NULL;

	int ret = 0;
	int freq_in = 24576000;
	if (params_rate(params) != 8000)
		return -EINVAL;
	ctx = snd_soc_card_get_drvdata(card);
	if (ctx == NULL ){
		pr_err("err:%s,get ctx failed.\n",__func__);
		return -EINVAL;
	}
	/* set the codec aif1clk/aif2clk from pllclk */
	ret = snd_soc_dai_set_pll(codec_dai, PLLCLK, 0, freq_in,freq_in);
	if (ret < 0) {
		pr_err("err:%s,set codec dai pll failed.\n", __func__);
		return ret;
	}
	/*set system clock source aif2*/
	ret = snd_soc_dai_set_sysclk(codec_dai, AIF2_CLK , 0, 0);
	if (ret < 0) {
		pr_err("err:%s,set codec dai sysclk faied\n", __func__);
		return ret;
	}
	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A | SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0) {
		pr_err("err:%s,set codec dai fmt failed.\n", __func__);
	}
	return ret;
}
static struct snd_soc_ops bb_clk_ops = {
	.hw_params = bb_clk_hw_params,
};
static int bt_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct mc_private *ctx = NULL;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_card *card = rtd->card;
	int ret = 0;
	if (params_rate(params) != 8000)
		return -EINVAL;
	ctx = snd_soc_card_get_drvdata(card);
	if (ctx == NULL ){
		pr_err("err:%s,get ctx failed.\n",__func__);
		return -EINVAL;
	}
	/* set codec aif3 configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, (ctx->aif3fmt << 8));
	if (ret < 0)
		return ret;
	return 0;
}
static struct snd_soc_ops bt_voice_ops = {
	.hw_params = bt_hw_params,
};
static struct snd_soc_dai_link sunxi_sndpcm_dai_link[] = {
	{
	.name = "audiocodec",
	.stream_name 	= "SUNXI-CODEC",
	.cpu_dai_name 	= "sunxi-internal-i2s",
	.codec_dai_name = "codec-aif1",
	.platform_name 	= "sunxi-internal-i2s",
	.codec_name 	= "sunxi-internal-codec",
	.init 			= sunxi_audio_init,
    	.ops = &sunxi_sndpcm_ops,

	},
/**/
	{
	.name = "Voice",
	.stream_name = "bb Voice",
	.cpu_dai_name = "bb-dai",
	.codec_dai_name = "codec-aif2",
	.codec_name = "sunxi-pcm-codec",
	.ops = &bb_voice_ops,
	},
/**/
	{
	.name = "bbclk",
	.stream_name = "bb-bt-clk",
	.cpu_dai_name = "bb-dai",
	.codec_dai_name = "codec-aif2",
	.codec_name = "sunxi-pcm-codec",
	.ops = &bb_clk_ops,
	},
	{
	.name = "bt",
	.stream_name = "bt Voice",
	.cpu_dai_name = "bb-dai",
	.codec_dai_name = "codec-aif3",
	.codec_name = "sunxi-pcm-codec",
	.ops = &bt_voice_ops,
	},
/**/
};

static struct snd_soc_card snd_soc_sunxi_sndpcm = {
	.name 		= "audiocodec",
	.owner 		= THIS_MODULE,
	.dai_link 	= sunxi_sndpcm_dai_link,
	.num_links 	= ARRAY_SIZE(sunxi_sndpcm_dai_link),
	.dapm_widgets = sun50iw1_ac_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(sun50iw1_ac_dapm_widgets),
	.dapm_routes = audio_map,
	.num_dapm_routes = ARRAY_SIZE(audio_map),
	.controls = ac_pin_controls,
	.num_controls = ARRAY_SIZE(ac_pin_controls),
};
static struct snd_soc_dai_driver voice_dai[] = {
	{
		.name = "bb-dai",
		.id = 1,
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	},
	{
		.name = "no-dai",
		.id = 2,
	}
};

static const struct snd_soc_component_driver voice_component = {
	.name		= "bb-voice",
};
static int sun50iw1_machine_probe(struct platform_device *pdev)
{
	int ret = 0;
	u32 temp_val;
	struct mc_private *ctx = NULL;
	struct device_node *np = pdev->dev.of_node;
	if (!np) {
		dev_err(&pdev->dev,
			"can not get dt node for this device.\n");
		return -EINVAL;
	}
	ctx = devm_kzalloc(&pdev->dev, sizeof(struct mc_private), GFP_KERNEL);
	if (!ctx) {
		pr_err("[audio] allocation mem failed\n");
		return -ENOMEM;
	}
	/* register voice DAI here */
	ret = snd_soc_register_component(&pdev->dev, &voice_component,
					 		voice_dai, ARRAY_SIZE(voice_dai));
	if (ret) {
		dev_err(&pdev->dev, "register DAI failed\n");
		goto err0;
	}

	/* register the soc card */
	snd_soc_sunxi_sndpcm.dev = &pdev->dev;
	snd_soc_card_set_drvdata(&snd_soc_sunxi_sndpcm, ctx);
	platform_set_drvdata(pdev, &snd_soc_sunxi_sndpcm);
	ctx->jackirq = platform_get_irq(pdev, 0);
	if (ctx->jackirq < 0) {
		pr_err("[audio]irq failed\n");
		ret = -ENODEV;
		goto err1;
	}

	ret = of_property_read_u32(np, "aif2fmt",&temp_val);
	if (ret < 0) {
		pr_err("[audio]aif2fmt configurations missing or invalid.\n");
	} else {
		ctx->aif2fmt = temp_val;
	}

	ret = of_property_read_u32(np, "aif3fmt",&temp_val);
	if (ret < 0) {
		pr_err("[audio]aif3fmt configurations missing or invalid.\n");
	} else {
		ctx->aif3fmt = temp_val;
	}

	ret = of_property_read_u32(np, "aif3fmt",&temp_val);
	if (ret < 0) {
		pr_err("[audio]aif3fmt configurations missing or invalid.\n");
	} else {
		ctx->aif3fmt = temp_val;
	}
	ret = of_property_read_u32(np, "aif2master",&temp_val);
	if (ret < 0) {
		pr_err("[audio]aif2master configurations missing or invalid.\n");
	} else {
		ctx->aif2master = temp_val;
	}
	sunxi_sndpcm_dai_link[0].cpu_dai_name = NULL;
	sunxi_sndpcm_dai_link[0].cpu_of_node = of_parse_phandle(np,
				"sunxi,i2s-controller", 0);
	if (!sunxi_sndpcm_dai_link[0].cpu_of_node) {
		dev_err(&pdev->dev,
			"Property 'sunxi,i2s-controller' missing or invalid\n");
			ret = -EINVAL;
			goto err1;
	}
	sunxi_sndpcm_dai_link[0].platform_name = NULL;
	sunxi_sndpcm_dai_link[0].platform_of_node = sunxi_sndpcm_dai_link[0].cpu_of_node;

	sunxi_sndpcm_dai_link[0].codec_name = NULL;
	sunxi_sndpcm_dai_link[0].codec_of_node = of_parse_phandle(np,"sunxi,audio-codec", 0);
	if (!sunxi_sndpcm_dai_link[0].codec_of_node) {
		dev_err(&pdev->dev,
			"Property 'sunxi,audio-codec' missing or invalid\n");
		ret = -EINVAL;
		goto err1;
	}
	sunxi_sndpcm_dai_link[1].codec_name = NULL;
	sunxi_sndpcm_dai_link[1].codec_of_node = sunxi_sndpcm_dai_link[0].codec_of_node;

	sunxi_sndpcm_dai_link[2].codec_name = NULL;
	sunxi_sndpcm_dai_link[2].codec_of_node = sunxi_sndpcm_dai_link[0].codec_of_node;

	sunxi_sndpcm_dai_link[3].codec_name = NULL;
	sunxi_sndpcm_dai_link[3].codec_of_node = sunxi_sndpcm_dai_link[0].codec_of_node;

	ret = snd_soc_register_card(&snd_soc_sunxi_sndpcm);
	if (ret) {
		pr_err("snd_soc_register_card failed %d\n", ret);
		goto err1;
	}
	/*
	*initial the parameters for judge switch state
	*/
	ctx->HEADSET_DATA = 0x10;
	INIT_DELAYED_WORK(&ctx->hs_insert_work, sun50iw1_check_hs_insert_status);
	INIT_DELAYED_WORK(&ctx->hs_button_work, sun50iw1_check_hs_button_status);
	INIT_DELAYED_WORK(&ctx->hs_remove_work, sun50iw1_check_hs_remove_status);

	mutex_init(&ctx->jack_mlock);
	/*0x314*/
	snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL2, (0xffff<<0), (0x0<<0));
	snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL2, (0x1f<<8), (0x17<<8));
	/*0x318*/
	snd_soc_update_bits(ctx->codec, SUNXI_HMIC_STS, (0xffff<<0), (0x0<<0));
	/*0x1c*/
	snd_soc_update_bits(ctx->codec, MDET_CTRL, (0xffff<<0), (0x0<<0));
	/*0x1d*/
	snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0xffff<<0), (0x0<<0));
	snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<JACKDETEN), (0x1<<JACKDETEN));
	snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<INNERRESEN), (0x1<<INNERRESEN));
	snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<AUTOPLEN), (0x1<<AUTOPLEN));

	/*enable jack in /out irq*//*0x310*/
	snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL1, (0x1<<JACK_IN_IRQ_EN), (0x1<<JACK_IN_IRQ_EN));
	snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL1, (0xf<<HMIC_N), (0xf<<HMIC_N));

	ret = snd_soc_jack_new(ctx->codec, "sun50iw1 Audio Jack",
			       SND_JACK_HEADSET | SND_JACK_HEADPHONE | SND_JACK_BTN_0 | SND_JACK_BTN_1 | SND_JACK_BTN_2,
			       &ctx->jack);
	if (ret) {
		pr_err("jack creation failed\n");
		return ret;
	}
	snd_jack_set_key(ctx->jack.jack, SND_JACK_BTN_0, KEY_MEDIA);
	snd_jack_set_key(ctx->jack.jack, SND_JACK_BTN_1, KEY_VOLUMEUP);
	snd_jack_set_key(ctx->jack.jack, SND_JACK_BTN_2, KEY_VOLUMEDOWN);
	pr_debug("register jack interrupt.,jackirq:%d\n",ctx->jackirq);
	ret = request_irq(ctx->jackirq, jack_interrupt, 0, "audio jack irq", ctx);

	pr_debug("%s,line:%d,0X310:%X,0X314:%X,0X318:%X,0X1C:%X,0X1D:%X\n",__func__,__LINE__,
				snd_soc_read(ctx->codec, 0x310),
					snd_soc_read(ctx->codec, 0x314),
					snd_soc_read(ctx->codec, 0x318),
					snd_soc_read(ctx->codec, 0x1C),
					snd_soc_read(ctx->codec, 0x1D));

	return 0;
err1:
	snd_soc_unregister_component(&pdev->dev);
err0:
	return ret;
}

static void snd_sun50iw1_unregister_jack(struct mc_private *ctx)
{
	/* Set process button events to false so that the button
	   delayed work will not be scheduled.*/
	cancel_delayed_work_sync(&ctx->hs_insert_work);
	cancel_delayed_work_sync(&ctx->hs_button_work);
	cancel_delayed_work_sync(&ctx->hs_remove_work);
}

static void sun50iw1_machine_shutdown(struct platform_device *pdev)
{
	struct snd_soc_card *soc_card = platform_get_drvdata(pdev);
	struct mc_private *ctx = snd_soc_card_get_drvdata(soc_card);
	snd_sun50iw1_unregister_jack(ctx);
}

static const struct of_device_id sunxi_sun50iw1machine_of_match[] = {
	{ .compatible = "allwinner,sunxi-codec-machine", },
	{},
};
#ifdef CONFIG_PM
static int sun50iw1_machine_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	struct snd_soc_card *soc_card = platform_get_drvdata(pdev);
	struct mc_private *ctx = snd_soc_card_get_drvdata(soc_card);
	pr_debug("[codec-machine]  suspend.\n")
	disable_irq(ctx->jackirq);
	return 0;
}

static int sun50iw1_machine_resume(struct platform_device *pdev)
{
	struct snd_soc_card *soc_card = platform_get_drvdata(pdev);
	struct mc_private *ctx = snd_soc_card_get_drvdata(soc_card);
	pr_debug("[codec-machine]  resume.\n")
	/*0x314*/
	snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL2, (0xffff<<0), (0x0<<0));
	snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL2, (0x1f<<8), (0x17<<8));
	/*0x318*/
	snd_soc_update_bits(ctx->codec, SUNXI_HMIC_STS, (0xffff<<0), (0x0<<0));
	/*0x1c*/
	snd_soc_update_bits(ctx->codec, MDET_CTRL, (0xffff<<0), (0x0<<0));
	/*0x1d*/
	snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0xffff<<0), (0x0<<0));
	snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<JACKDETEN), (0x1<<JACKDETEN));
	snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<INNERRESEN), (0x1<<INNERRESEN));
	snd_soc_update_bits(ctx->codec, JACK_MIC_CTRL, (0x1<<AUTOPLEN), (0x1<<AUTOPLEN));

	/*enable jack in /out irq*//*0x310*/
	snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL1, (0x1<<JACK_IN_IRQ_EN), (0x1<<JACK_IN_IRQ_EN));
	snd_soc_update_bits(ctx->codec, SUNXI_HMIC_CTRL1, (0xf<<HMIC_N), (0xf<<HMIC_N));
	enable_irq(ctx->jackirq);
	return 0;
}
#else
#define	sun50iw1_machine_suspend	NULL
#define	sun50iw1_machine_resume	NULL
#endif

/*method relating*/
static struct platform_driver sun50iw1_machine_driver = {
	.driver = {
		.name = "sunxi-codec-machine",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = sunxi_sun50iw1machine_of_match,
	},
	.probe = sun50iw1_machine_probe,
	.shutdown = sun50iw1_machine_shutdown,
	.suspend	= sun50iw1_machine_suspend,
	.resume		= sun50iw1_machine_resume,
};

static int __init sun50iw1_machine_init(void)
{
	int err = 0;
	if ((err = platform_driver_register(&sun50iw1_machine_driver)) < 0)
		return err;

	return 0;
}
module_init(sun50iw1_machine_init);
static void __exit sun50iw1_machine_exit(void)
{
	platform_driver_unregister(&sun50iw1_machine_driver);
}

module_exit(sun50iw1_machine_exit);
module_param_named(switch_state, switch_state, int, S_IRUGO | S_IWUSR);

MODULE_AUTHOR("huangxin,liushaohua");
MODULE_DESCRIPTION("SUNXI_sndpcm ALSA SoC audio driver");
MODULE_LICENSE("GPL");

