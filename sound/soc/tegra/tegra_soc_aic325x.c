/*
 * tegra_aic325x.c - Tegra machine ASoC driver for boards using TI 3262 codec.
 *
 * Author: Vinod G. <vinodg@nvidia.com>
 * Copyright (C) 2011 - NVIDIA, Inc.
 *
 * Based on code copyright/by:
 *
 * (c) 2010, 2011 Nvidia Graphics Pvt. Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <asm/mach-types.h>

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif

#include <mach/tegra_aic325x_pdata.h>

#include <sound/core.h>
#include <sound/jack.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "../codecs/tlv320aic325x.h"

#include "tegra_pcm.h"
#include "tegra_asoc_utils.h"

//#ifdef CONFIG_ARCH_TEGRA_2x_SOC
//#include "tegra20_das.h"
///#else
#include "tegra30_ahub.h"
#include "tegra30_i2s.h"
#include "tegra30_dam.h"
//#endif
#include <linux/delay.h>

#define DRV_NAME "tegra-snd-aic325x"

#define GPIO_SPKR_EN    BIT(0)
#define GPIO_HP_DET		BIT(1)
#define GPIO_HP_MUTE    BIT(2)
#define GPIO_INT_MIC_EN BIT(3)
#define GPIO_EXT_MIC_EN BIT(4)
#define GPIO_RESET_EN BIT(5)

#define DAI_LINK_HIFI		0
#define DAI_LINK_SPDIF		1
#define DAI_LINK_BTSCO		2
#define DAI_LINK_VOICE_CALL	3
#define DAI_LINK_BT_VOICE_CALL	4
#define NUM_DAI_LINKS	5
#define __TEGRA_GPIO_DET__
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
const char *tegra_aic325x_i2s_dai_name[TEGRA30_NR_I2S_IFC] = {
	"tegra30-i2s.0",
	"tegra30-i2s.1",
	"tegra30-i2s.2",
	"tegra30-i2s.3",
	"tegra30-i2s.4",
};
#endif

struct tegra_aic325x {
	struct tegra_asoc_utils_data util_data;
	struct tegra_aic325x_platform_data *pdata;
	struct regulator *audio_reg;
	int gpio_requested;
	bool init_done;
	int is_call_mode;
	int is_device_bt;
#ifdef CONFIG_SWITCH
	int jack_status;
#endif
	enum snd_soc_bias_level bias_level;
	volatile int clock_enabled;
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	struct codec_config codec_info[NUM_I2S_DEVICES];
#endif
};

static int tegra_aic325x_get_mclk(int srate)
{
	int mclk = 0;
	switch (srate) {
	case 8000:
	case 16000:
	case 24000:
	case 32000:
	case 48000:
	case 64000:
	case 96000:
		mclk = 12288000;
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		mclk = 11289600;
		break;
	default:
		mclk = -EINVAL;
		break;
	}

	return mclk;
}
#if 0
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
static int tegra_aic325x_set_dam_cif(int dam_ifc, int srate,
			int channels, int bit_size, int src_on, int src_srate,
			int src_channels, int src_bit_size)
{
	tegra30_dam_set_gain(dam_ifc, TEGRA30_DAM_CHIN1, 0x1000);
	tegra30_dam_set_samplerate(dam_ifc, TEGRA30_DAM_CHOUT,
				srate);
	tegra30_dam_set_samplerate(dam_ifc, TEGRA30_DAM_CHIN1,
				srate);
	tegra30_dam_set_acif(dam_ifc, TEGRA30_DAM_CHIN1,
		channels, bit_size, channels,
				bit_size);
	tegra30_dam_set_acif(dam_ifc, TEGRA30_DAM_CHOUT,
		channels, bit_size, channels,
				bit_size);

	if (src_on) {
		tegra30_dam_set_gain(dam_ifc, TEGRA30_DAM_CHIN0_SRC, 0x1000);
		tegra30_dam_set_samplerate(dam_ifc, TEGRA30_DAM_CHIN0_SRC,
			src_srate);
		tegra30_dam_set_acif(dam_ifc, TEGRA30_DAM_CHIN0_SRC,
			src_channels, src_bit_size, 1, 16);
	}

	return 0;
}
#endif
#endif
static int tegra_aic325x_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = codec->card;
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
#if 0
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	struct tegra30_i2s *i2s = snd_soc_dai_get_drvdata(cpu_dai);
#endif
#endif
	int srate, mclk, sample_size, daifmt;
	int err;
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		sample_size = 16;
		break;
	default:
		return -EINVAL;
	}

	srate = params_rate(params);

	mclk = tegra_aic325x_get_mclk(srate);
	if (mclk < 0) {
		printk(" tegra_aic325x_get_mclk <0 \n");
		return mclk;
	}
	err = tegra_asoc_utils_set_rate(&machine->util_data, srate, mclk);
	printk("tegra_asoc_utils_set_rate ret Val: %d\n", err );
	if (err < 0) {
		if (!(machine->util_data.set_mclk % mclk))
			mclk = machine->util_data.set_mclk;
		else {
			dev_err(card->dev, "Can't configure clocks\n");
			return err;
		}
	}

	tegra_asoc_utils_lock_clk_rate(&machine->util_data, 1);

	daifmt = SND_SOC_DAIFMT_I2S |	SND_SOC_DAIFMT_NB_NF |
					SND_SOC_DAIFMT_CBS_CFS;

	err = snd_soc_dai_set_fmt(codec_dai, daifmt);
	if (err < 0) {
		dev_err(card->dev, "codec_dai fmt not set\n");
		return err;
	}

	err = snd_soc_dai_set_fmt(cpu_dai, daifmt);
	if (err < 0) {
		dev_err(card->dev, "cpu_dai fmt not set\n");
		return err;
	}

	err = snd_soc_dai_set_sysclk(codec_dai, 0, mclk,
					SND_SOC_CLOCK_IN);
	if (err < 0) {
		dev_err(card->dev, "codec_dai clock not set\n");
		return err;
	}
#if 0
#ifdef CONFIG_ARCH_TEGRA_2x_SOC
	err = tegra20_das_connect_dac_to_dap(TEGRA20_DAS_DAP_SEL_DAC1,
					TEGRA20_DAS_DAP_ID_1);
	if (err < 0) {
		dev_err(card->dev, "failed to set dap-dac path\n");
		return err;
	}

	err = tegra20_das_connect_dap_to_dac(TEGRA20_DAS_DAP_ID_1,
					TEGRA20_DAS_DAP_SEL_DAC1);
	if (err < 0) {
		dev_err(card->dev, "failed to set dac-dap path\n");
		return err;
	}

#else /*assumes tegra3*/
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK  && i2s->is_dam_used)
		tegra_aic325x_set_dam_cif(i2s->dam_ifc, srate,
			params_channels(params), sample_size, 0, 0, 0, 0);
#endif
#endif
	return 0;
}
#if 0
static int tegra_aic325x_spdif_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
	int srate, mclk, min_mclk;
	int err;

	srate = params_rate(params);
	mclk = tegra_aic325x_get_mclk(srate);
	if (mclk < 0)
		return mclk;

	min_mclk = 128 * srate;

	err = tegra_asoc_utils_set_rate(&machine->util_data, srate, mclk);
	if (err < 0) {
		if (!(machine->util_data.set_mclk % min_mclk))
			mclk = machine->util_data.set_mclk;
		else {
			dev_err(card->dev, "Can't configure clocks\n");
			return err;
		}
	}

	tegra_asoc_utils_lock_clk_rate(&machine->util_data, 1);

	return 0;
}

static int tegra_aic325x_bt_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
//#ifndef CONFIG_ARCH_TEGRA_2x_SOC
//	struct tegra30_i2s *i2s = snd_soc_dai_get_drvdata(rtd->cpu_dai);
//#endif
	struct snd_soc_card *card = rtd->card;
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
	int err, srate, mclk, min_mclk, sample_size;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		sample_size = 16;
		break;
	default:
		return -EINVAL;
	}

	srate = params_rate(params);

	mclk = tegra_aic325x_get_mclk(srate);
	if (mclk < 0)
		return mclk;

	min_mclk = 64 * srate;

	err = tegra_asoc_utils_set_rate(&machine->util_data, srate, mclk);
	if (err < 0) {
		if (!(machine->util_data.set_mclk % min_mclk))
			mclk = machine->util_data.set_mclk;
		else {
			dev_err(card->dev, "Can't configure clocks\n");
			return err;
		}
	}

	tegra_asoc_utils_lock_clk_rate(&machine->util_data, 1);

	err = snd_soc_dai_set_fmt(rtd->cpu_dai,
			SND_SOC_DAIFMT_DSP_A |
			SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBS_CFS);
	if (err < 0) {
		dev_err(rtd->codec->card->dev, "cpu_dai fmt not set\n");
		return err;
	}

#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	//if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	//	tegra_aic325x_set_dam_cif(i2s->dam_ifc, params_rate(params),
	//			params_channels(params), sample_size);
#endif

	return 0;
}
#endif
static int tegra_aic325x_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(rtd->card);

	tegra_asoc_utils_lock_clk_rate(&machine->util_data, 0);

	return 0;
}
#if 0
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
static int tegra_aic325x_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct tegra30_i2s *i2s = snd_soc_dai_get_drvdata(cpu_dai);

	if ((substream->stream != SNDRV_PCM_STREAM_PLAYBACK) ||
		!(i2s->is_dam_used))
		return 0;

	/*dam configuration*/
	if (!i2s->dam_ch_refcount)
		i2s->dam_ifc = tegra30_dam_allocate_controller();

	tegra30_dam_allocate_channel(i2s->dam_ifc, TEGRA30_DAM_CHIN1);
	i2s->dam_ch_refcount++;
	tegra30_dam_enable_clock(i2s->dam_ifc);
	tegra30_dam_set_gain(i2s->dam_ifc, TEGRA30_DAM_CHIN1, 0x1000);

	tegra30_ahub_set_rx_cif_source(TEGRA30_AHUB_RXCIF_DAM0_RX1 +
			(i2s->dam_ifc*2), i2s->txcif);

	/*
	*make the dam tx to i2s rx connection if this is the only client
	*using i2s for playback
	*/
	if (i2s->playback_ref_count == 1)
		tegra30_ahub_set_rx_cif_source(
			TEGRA30_AHUB_RXCIF_I2S0_RX0 + i2s->id,
			TEGRA30_AHUB_TXCIF_DAM0_TX0 + i2s->dam_ifc);

	/* enable the dam*/
	tegra30_dam_enable(i2s->dam_ifc, TEGRA30_DAM_ENABLE,
			TEGRA30_DAM_CHIN1);

	return 0;
}

static void tegra_aic325x_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct tegra30_i2s *i2s = snd_soc_dai_get_drvdata(cpu_dai);

	if ((substream->stream != SNDRV_PCM_STREAM_PLAYBACK) ||
		!(i2s->is_dam_used))
		return;

	/* disable the dam*/
	tegra30_dam_enable(i2s->dam_ifc, TEGRA30_DAM_DISABLE,
			TEGRA30_DAM_CHIN1);

	/* disconnect the ahub connections*/
	tegra30_ahub_unset_rx_cif_source(TEGRA30_AHUB_RXCIF_DAM0_RX1 +
				(i2s->dam_ifc*2));

	/* disable the dam and free the controller */
	tegra30_dam_disable_clock(i2s->dam_ifc);
	tegra30_dam_free_channel(i2s->dam_ifc, TEGRA30_DAM_CHIN1);
	i2s->dam_ch_refcount--;
	if (!i2s->dam_ch_refcount)
		tegra30_dam_free_controller(i2s->dam_ifc);
}
#endif
#endif
#if 0
static int tegra_aic325x_voice_call_hw_params(
			struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = codec->card;
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
	int srate, mclk;
	int err;

	srate = params_rate(params);
	mclk = tegra_aic325x_get_mclk(srate);
	if (mclk < 0)
		return mclk;


	err = tegra_asoc_utils_set_rate(&machine->util_data, srate, mclk);
	if (err < 0) {
		if (!(machine->util_data.set_mclk % mclk))
			mclk = machine->util_data.set_mclk;
		else {
			dev_err(card->dev, "Can't configure clocks\n");
			return err;
		}
	}

	tegra_asoc_utils_lock_clk_rate(&machine->util_data, 1);

	err = snd_soc_dai_set_fmt(codec_dai,
					SND_SOC_DAIFMT_DSP_B |
					SND_SOC_DAIFMT_NB_NF |
					SND_SOC_DAIFMT_CBS_CFS);
	if (err < 0) {
		dev_err(card->dev, "codec_dai fmt not set\n");
		return err;
	}

	err = snd_soc_dai_set_sysclk(codec_dai, 0, mclk,
					SND_SOC_CLOCK_IN);
	if (err < 0) {
		dev_err(card->dev, "codec_dai clock not set\n");
		return err;
	}

#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	/* codec configuration */
	machine->codec_info[HIFI_CODEC].rate = params_rate(params);
	machine->codec_info[HIFI_CODEC].channels = params_channels(params);
	machine->codec_info[HIFI_CODEC].bitsize = 16;
	machine->codec_info[HIFI_CODEC].is_i2smaster = 1;
	machine->codec_info[HIFI_CODEC].is_format_dsp = 0;

	/* baseband configuration */
	machine->codec_info[BASEBAND].bitsize = 16;
	machine->codec_info[BASEBAND].is_i2smaster = 1;
	machine->codec_info[BASEBAND].is_format_dsp = 1;
#endif

	machine->is_device_bt = 0;

	return 0;
}

static void tegra_aic325x_voice_call_shutdown(
					struct snd_pcm_substream *substream)
{
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct tegra_aic325x *machine  =
			snd_soc_card_get_drvdata(rtd->codec->card);

	machine->codec_info[HIFI_CODEC].rate = 0;
	machine->codec_info[HIFI_CODEC].channels = 0;
#endif
}

static int tegra_aic325x_bt_voice_call_hw_params(
			struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
	int err, srate, mclk, min_mclk;

	srate = params_rate(params);

	mclk = tegra_aic325x_get_mclk(srate);
	if (mclk < 0)
		return mclk;

	min_mclk = 64 * srate;

	err = tegra_asoc_utils_set_rate(&machine->util_data, srate, mclk);
	if (err < 0) {
		if (!(machine->util_data.set_mclk % min_mclk))
			mclk = machine->util_data.set_mclk;
		else {
			dev_err(card->dev, "Can't configure clocks\n");
			return err;
		}
	}

	tegra_asoc_utils_lock_clk_rate(&machine->util_data, 1);

#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	/* codec configuration */
	machine->codec_info[BT_SCO].rate = params_rate(params);
	machine->codec_info[BT_SCO].channels = params_channels(params);
	machine->codec_info[BT_SCO].bitsize = 16;
	machine->codec_info[BT_SCO].is_i2smaster = 1;
	machine->codec_info[BT_SCO].is_format_dsp = 1;

	/* baseband configuration */
	machine->codec_info[BASEBAND].bitsize = 16;
	machine->codec_info[BASEBAND].is_i2smaster = 1;
	machine->codec_info[BASEBAND].is_format_dsp = 1;
#endif

	machine->is_device_bt = 1;

	return 0;
}

static void tegra_aic325x_bt_voice_call_shutdown(
				struct snd_pcm_substream *substream)
{
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct tegra_aic325x *machine  =
			snd_soc_card_get_drvdata(rtd->codec->card);

	machine->codec_info[BT_SCO].rate = 0;
	machine->codec_info[BT_SCO].channels = 0;
#endif
}



static struct snd_soc_ops tegra_aic325x_spdif_ops = {
	.hw_params = tegra_aic325x_spdif_hw_params,
	.hw_free = tegra_aic325x_hw_free,
};

static struct snd_soc_ops tegra_aic325x_voice_call_ops = {
	.hw_params = tegra_aic325x_voice_call_hw_params,
	.shutdown = tegra_aic325x_voice_call_shutdown,
	.hw_free = tegra_aic325x_hw_free,
};

static struct snd_soc_ops tegra_aic325x_bt_voice_call_ops = {
	.hw_params = tegra_aic325x_bt_voice_call_hw_params,
	.shutdown = tegra_aic325x_bt_voice_call_shutdown,
	.hw_free = tegra_aic325x_hw_free,
};

static struct snd_soc_ops tegra_aic325x_bt_ops = {
	.hw_params = tegra_aic325x_bt_hw_params,
	.hw_free = tegra_aic325x_hw_free,
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	.startup = tegra_aic325x_startup,
	.shutdown = tegra_aic325x_shutdown,
#endif
};
#endif
static struct snd_soc_ops tegra_aic325x_hifi_ops = {
	.hw_params = tegra_aic325x_hw_params,
	.hw_free = tegra_aic325x_hw_free,
//#ifndef CONFIG_ARCH_TEGRA_2x_SOC
//	.startup = tegra_aic325x_startup,
//	.shutdown = tegra_aic325x_shutdown,
//s#endif
};
static struct snd_soc_jack tegra_aic325x_hp_jack;
static struct snd_soc_jack_gpio tegra_aic325x_hp_jack_gpio = {
	.name = "headphone detect",
	.report = SND_JACK_HEADPHONE,
	.debounce_time = 150,
	.invert = 1,
};
#ifdef CONFIG_SWITCH
static struct switch_dev aic325x_wired_switch_dev = {
	.name = "h2w",
};

/* These values are copied from WiredAccessoryObserver */
enum headset_state {
	BIT_NO_HEADSET = 0,
	BIT_HEADSET = (1 << 0),
	BIT_HEADSET_NO_MIC = (1 << 1),
};
int aic325x_hp(struct snd_soc_codec *codec, int jack_insert);
static int aic325x_headset_switch_notify(struct notifier_block *self,
	unsigned long action, void *dev)
{
	int state = 0;
	switch (action) {
	case SND_JACK_HEADPHONE:
//		state |= BIT_HEADSET_NO_MIC;
//		break;
	case SND_JACK_HEADSET:
		state |= BIT_HEADSET;
		break;
	default:
		state |= BIT_NO_HEADSET;
	}
	switch_set_state(&aic325x_wired_switch_dev, state);
	return NOTIFY_OK;
}

static struct notifier_block aic325x_headset_switch_nb = {
	.notifier_call = aic325x_headset_switch_notify,
};
#else
static struct snd_soc_jack_pin tegra_aic325x_hp_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};
#endif

static int tegra_aic325x_event_int_spk(struct snd_soc_dapm_widget *w,
					struct snd_kcontrol *k, int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
	struct tegra_aic325x_platform_data *pdata = machine->pdata;
	if (!(machine->gpio_requested & GPIO_SPKR_EN))
		return 0;
	printk("show speaker state %d\n",event);
	gpio_set_value_cansleep(pdata->gpio_spkr_en,
				SND_SOC_DAPM_EVENT_ON(event));

	return 0;
}

static int tegra_aic325x_event_int_mic(struct snd_soc_dapm_widget *w,
					struct snd_kcontrol *k, int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
	struct tegra_aic325x_platform_data *pdata = machine->pdata;


	if (!(machine->gpio_requested & GPIO_INT_MIC_EN))
		return 0;

	gpio_set_value_cansleep(pdata->gpio_int_mic_en,
				SND_SOC_DAPM_EVENT_ON(event));

	return 0;
}
#if 0

static int tegra_aic325x_event_hp(struct snd_soc_dapm_widget *w,
					struct snd_kcontrol *k, int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
	struct tegra_aic325x_platform_data *pdata = machine->pdata;


	if (!(machine->gpio_requested & GPIO_HP_MUTE))
		return 0;

	gpio_set_value_cansleep(pdata->gpio_hp_mute,
				!SND_SOC_DAPM_EVENT_ON(event));

	return 0;
}
#endif

static const struct snd_soc_dapm_widget tegra_aic325x_dapm_widgets[] = {

	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	//SND_SOC_DAPM_LINE("Line out", NULL),

	SND_SOC_DAPM_MIC("Int Mic", tegra_aic325x_event_int_mic),
//	SND_SOC_DAPM_LINE("Linein", NULL),
	SND_SOC_DAPM_MIC("Ext Mic", NULL),
//	SND_SOC_DAPM_SPK("Int Spk",NULL),
	SND_SOC_DAPM_SPK("Int Spk", tegra_aic325x_event_int_spk),
};

static const struct snd_soc_dapm_route aic325x_audio_map[] = {

	/* Headphone connected to HPL, HPR */
	{"Headphone Jack", NULL, "HPL"},
	{"Headphone Jack", NULL, "HPR"},

	/* Line Out connected to LOL, LOR */
//	{"Line out", NULL, "LOL"},
//	{"Line out", NULL, "LOR"},

	{"Int Spk", NULL, "LOL"},
	{"Int Spk", NULL, "LOR"},

	{"IN1_L", NULL, "Ext Mic"},
	{"IN1_R", NULL, "Ext Mic"},

//	{"IN2_L", NULL, "Linein"}, /*For internal testing*/
//	{"IN2_R", NULL, "Linein"},

	{"IN3_L", NULL, "Int Mic"}, /*For internal testing*/
	{"IN3_R", NULL, "Int Mic"},

	{"IN1_L", NULL, "Ext Mic"},
	{"IN1_R", NULL, "Ext Mic"},

//	{"IN2_L", NULL, "Ext Mic"},
//	{"IN2_R", NULL, "Ext Mic"},

	{"IN3_L", NULL, "Ext Mic"},
	{"IN3_R", NULL, "Ext Mic"},

	{"IN1_L", NULL, "Int Mic"}, 
	{"IN1_R", NULL, "Int Mic"},

	{"Left DMIC", NULL, "Int Mic"}, 
	{"Right DMIC", NULL, "Int Mic"},
};

static const struct snd_kcontrol_new tegra_aic325x_controls[] = {
	SOC_DAPM_PIN_SWITCH("Int Spk"),
//	SOC_DAPM_PIN_SWITCH("Earpiece"),
	SOC_DAPM_PIN_SWITCH("Headphone Jack"),
	SOC_DAPM_PIN_SWITCH("Int Mic"),
	SOC_DAPM_PIN_SWITCH("Ext Mic"),
//	SOC_DAPM_PIN_SWITCH("Linein"),
};

static int tegra_aic325x_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = codec->card;
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
	struct tegra_aic325x_platform_data *pdata = machine->pdata;

	int ret;



	if (machine->init_done)
		return 0;

	machine->init_done = true;


#if 1
	if (gpio_is_valid(pdata->gpio_spkr_en)) {
		ret = gpio_request(pdata->gpio_spkr_en, "spkr_en");
		if (ret) {
			dev_err(card->dev, "cannot get spkr_en gpio\n");
			return ret;
		}
		machine->gpio_requested |= GPIO_SPKR_EN;

		gpio_direction_output(pdata->gpio_spkr_en, 0);
	}
#endif
	if (gpio_is_valid(pdata->gpio_hp_mute)) {
		ret = gpio_request(pdata->gpio_hp_mute, "hp_mute");
		if (ret) {
			dev_err(card->dev, "cannot get hp_mute gpio\n");
			return ret;
		}
		machine->gpio_requested |= GPIO_HP_MUTE;

		gpio_direction_output(pdata->gpio_hp_mute, 0);
	}

	if (gpio_is_valid(pdata->gpio_int_mic_en)) {
		ret = gpio_request(pdata->gpio_int_mic_en, "int_mic_en");
		if (ret) {
			dev_err(card->dev, "cannot get int_mic_en gpio\n");
			return ret;
		}
		machine->gpio_requested |= GPIO_INT_MIC_EN;

		/* Disable int mic; enable signal is active-high */
		gpio_direction_output(pdata->gpio_int_mic_en, 0);
	}

	if (gpio_is_valid(pdata->gpio_ext_mic_en)) {
		ret = gpio_request(pdata->gpio_ext_mic_en, "ext_mic_en");
		if (ret) {
			dev_err(card->dev, "cannot get ext_mic_en gpio\n");
			return ret;
		}
		machine->gpio_requested |= GPIO_EXT_MIC_EN;

		/* Enable ext mic; enable signal is active-low */
		gpio_direction_output(pdata->gpio_ext_mic_en, 0);
	}
	#if 0
	if (gpio_is_valid(pdata->gpio_aic325x_reset)) {
		ret = gpio_request(pdata->gpio_aic325x_reset, "reset_en");
		if (ret) {
			dev_err(card->dev, "cannot get reset_en gpio\n");
			return ret;
		}
		machine->gpio_requested |= GPIO_RESET_EN;

		/* Enable reset; enable signal is active-low */
		printk("aic325x hw reset\n");
		gpio_direction_output(pdata->gpio_aic325x_reset, 0);
		mdelay(50);
		gpio_direction_output(pdata->gpio_aic325x_reset, 1);
		printk("aic325x hw reset done\n");
	}
	#endif

#ifdef __TEGRA_GPIO_DET__
#if defined(CONFIG_MACH_T8400N_11_6CM)
	tegra_aic325x_hp_jack_gpio.invert =0; 
#else
	tegra_aic325x_hp_jack_gpio.invert =1; 
#endif
	tegra_aic325x_hp_jack_gpio.gpio = pdata->gpio_hp_det;
#endif
	ret = snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET,
			&tegra_aic325x_hp_jack);
	if (ret < 0) {
		printk("**Err snd_soc_jack_new\n");
		return ret;
	}
#ifdef CONFIG_SWITCH
	snd_soc_jack_notifier_register(&tegra_aic325x_hp_jack,
		&aic325x_headset_switch_nb);
#else /*gpio based headset detection*/
	snd_soc_jack_add_pins(&tegra_aic325x_hp_jack,
		ARRAY_SIZE(tegra_aic325x_hp_jack_pins),
		tegra_aic325x_hp_jack_pins);
#endif
#ifdef __TEGRA_GPIO_DET__
	snd_soc_jack_add_gpios(&tegra_aic325x_hp_jack,1,&tegra_aic325x_hp_jack_gpio);
	machine->gpio_requested |= GPIO_HP_DET;
#endif

	 ret = tegra_asoc_utils_register_ctls(&machine->util_data);
      if (ret < 0)
              return ret;

	snd_soc_dapm_nc_pin(dapm, "IN2L");
	snd_soc_dapm_nc_pin(dapm, "IN2R");
	snd_soc_dapm_sync(dapm);

	return 0;
}

static struct snd_soc_dai_link tegra_aic325x_dai[] = {
	[DAI_LINK_HIFI] = {
		.name = "TLV320AIC325x",
		.stream_name = "TLV320AIC325x",
		.codec_name = "tlv320aic325x.0-0018",
		.platform_name = "tegra-pcm-audio",
		.cpu_dai_name = "tegra30-i2s.1",
		.codec_dai_name = "tlv320aic325x-MM_EXT",
		.init = tegra_aic325x_init,
		.ops = &tegra_aic325x_hifi_ops,
		},

};
static int tegra_aic325x_resume_pre(struct snd_soc_card *card)
{
	int val;
	struct snd_soc_jack_gpio *gpio = &tegra_aic325x_hp_jack_gpio;

	if (gpio_is_valid(gpio->gpio)) {
		val = gpio_get_value(gpio->gpio);
		val = gpio->invert ? !val : val;
		snd_soc_jack_report(gpio->jack, val, gpio->report);
	}

	return 0;
}


static int tegra_aic325x_set_bias_level(struct snd_soc_card *card,
	struct snd_soc_dapm_context *dapm, enum snd_soc_bias_level level)
{
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
//	SOCDBG("--------->\n");
	if (machine->bias_level == SND_SOC_BIAS_OFF &&
		level != SND_SOC_BIAS_OFF && (!machine->clock_enabled)) {
		machine->clock_enabled = 1;
		tegra_asoc_utils_clk_enable(&machine->util_data);
		machine->bias_level = level;
	}

	return 0;
}

static int tegra_aic325x_set_bias_level_post(struct snd_soc_card *card,
	struct snd_soc_dapm_context *dapm, enum snd_soc_bias_level level)
{
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
//	SOCDBG("--------->\n");
	if (machine->bias_level != SND_SOC_BIAS_OFF &&
		level == SND_SOC_BIAS_OFF && machine->clock_enabled) {
		machine->clock_enabled = 0;
		tegra_asoc_utils_clk_disable(&machine->util_data);
	}

	machine->bias_level = level;

	return 0 ;
}

static struct snd_soc_card snd_soc_tegra_aic325x = {
	.name = "tegra-aic325x",
	.dai_link = tegra_aic325x_dai,
	.num_links = ARRAY_SIZE(tegra_aic325x_dai),
	.resume_pre = tegra_aic325x_resume_pre,
	.set_bias_level = tegra_aic325x_set_bias_level,
	.set_bias_level_post = tegra_aic325x_set_bias_level_post,
	.controls = tegra_aic325x_controls,
	.num_controls = ARRAY_SIZE(tegra_aic325x_controls),
	.dapm_widgets = tegra_aic325x_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(tegra_aic325x_dapm_widgets),
	.dapm_routes = aic325x_audio_map,
	.num_dapm_routes = ARRAY_SIZE(aic325x_audio_map),
	.fully_routed = true,
};

static __devinit int tegra_aic325x_driver_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &snd_soc_tegra_aic325x;
	struct tegra_aic325x *machine;
	struct tegra_aic325x_platform_data *pdata;
	int ret, i;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "No platform data supplied\n");
		return -EINVAL;
	}
//	if (pdata->codec_name)
//		card->dai_link->codec_name = pdata->codec_name;

//	if (pdata->codec_dai_name)
//		card->dai_link->codec_dai_name = pdata->codec_dai_name;

	machine = kzalloc(sizeof(struct tegra_aic325x), GFP_KERNEL);
	if (!machine) {
		dev_err(&pdev->dev, "Can't allocate tegra_aic325x struct\n");
		printk("No MEMORY\n");
		return -ENOMEM;
	}

	machine->pdata = pdata;

	ret = tegra_asoc_utils_init(&machine->util_data, &pdev->dev,card);
	if (ret) {
		printk(" tegra_asoc_utils_init failed\n");
		goto err_free_machine;
	
	}
	machine->bias_level = SND_SOC_BIAS_STANDBY;
	machine->clock_enabled = 1;
#ifdef CONFIG_SWITCH
	/* Add h2w switch class support */
	ret = switch_dev_register(&aic325x_wired_switch_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "not able to register switch device %d\n",
			ret);
		goto err_unregister_card;
	}
#endif
	card->dev = &pdev->dev;
	platform_set_drvdata(pdev, card);
	snd_soc_card_set_drvdata(card, machine);
	card->dapm.idle_bias_off = 1;
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	for (i = 0; i < NUM_I2S_DEVICES ; i++)
		machine->codec_info[i].i2s_id = pdata->audio_port_id[i];

	machine->codec_info[BASEBAND].rate = pdata->baseband_param.rate;
	machine->codec_info[BASEBAND].channels = pdata->baseband_param.channels;
	machine->codec_info[BASEBAND].i2s_id = -1;

#endif
	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n",
			ret);
		goto err_fini_utils;
	}

	if (!card->instantiated) {
		ret = -ENODEV;
		dev_err(&pdev->dev, "sound card not instantiated (%d)\n",
			ret);
		goto err_unregister_card;
	}

#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	ret = tegra_asoc_utils_set_parent(&machine->util_data,
				pdata->i2s_param[HIFI_CODEC].is_i2s_master);
	if (ret) {
		dev_err(&pdev->dev, "tegra_asoc_utils_set_parent failed (%d)\n",
			ret);
		goto err_unregister_card;
	}
#endif

	return 0;

err_unregister_card:
	snd_soc_unregister_card(card);
err_fini_utils:
	tegra_asoc_utils_fini(&machine->util_data);
err_free_machine:
	kfree(machine);
	return ret;
}

static int __devexit tegra_aic325x_driver_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct tegra_aic325x *machine = snd_soc_card_get_drvdata(card);
	struct tegra_aic325x_platform_data *pdata = machine->pdata;

	snd_soc_unregister_card(card);

#ifdef CONFIG_SWITCH
	switch_dev_unregister(&aic325x_wired_switch_dev);
#endif
	
	if(machine->gpio_requested & GPIO_HP_DET)
		snd_soc_jack_free_gpios(&tegra_aic325x_hp_jack,1,&tegra_aic325x_hp_jack_gpio);
	tegra_asoc_utils_fini(&machine->util_data);

	if (machine->gpio_requested & GPIO_EXT_MIC_EN)
		gpio_free(pdata->gpio_ext_mic_en);
	if (machine->gpio_requested & GPIO_INT_MIC_EN)
		gpio_free(pdata->gpio_int_mic_en);
	if (machine->gpio_requested & GPIO_HP_MUTE)
		gpio_free(pdata->gpio_hp_mute);
	if (machine->gpio_requested & GPIO_SPKR_EN)
		gpio_free(pdata->gpio_spkr_en);
	if (machine->gpio_requested & GPIO_RESET_EN)
		gpio_free(pdata->gpio_aic325x_reset);
	kfree(machine);

	return 0;
}

static struct platform_driver tegra_aic325x_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
	},
	.probe = tegra_aic325x_driver_probe,
	.remove = __devexit_p(tegra_aic325x_driver_remove),
};

static int __init tegra_aic325x_modinit(void)
{
	int ret;
	ret=platform_driver_register(&tegra_aic325x_driver);
	return ret;
}

module_init(tegra_aic325x_modinit);

static void __exit tegra_aic325x_modexit(void)
{
	platform_driver_unregister(&tegra_aic325x_driver);
}
module_exit(tegra_aic325x_modexit);

/* Module information */
MODULE_AUTHOR("Vinod G. <vinodg@nvidia.com>");
MODULE_DESCRIPTION("Tegra+AIC3262 machine ASoC driver");
MODULE_DESCRIPTION("Tegra ALSA SoC");
MODULE_LICENSE("GPL");

