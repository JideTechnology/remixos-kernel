/*
 * arch/arm/mach-tegra/include/mach/tegra_aic326x_pdata.h
 *
 * Copyright 2011 NVIDIA, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __MACH_TEGRA_TLVAIC325X_H
#define __MACH_TEGRA_TLVAIC325X_H

#define	HIFI_CODEC		0
#define	BASEBAND		1
#define	BT_SCO			2
#define	NUM_I2S_DEVICES		3

#define	TEGRA_DAIFMT_DSP_A		0
#define	TEGRA_DAIFMT_DSP_B		1
#define	TEGRA_DAIFMT_I2S		2
#define	TEGRA_DAIFMT_RIGHT_J		3
#define	TEGRA_DAIFMT_LEFT_J		4

struct baseband_config {
	int rate;
	int channels;
};
struct i2s_config {
	int audio_port_id;
	int is_i2s_master;
	int i2s_mode;
	int sample_size;
	int rate;
	int channels;
};


struct tegra_aic325x_platform_data {
	const char *codec_name;
	const char *codec_dai_name;
	int gpio_spkr_en;
	int gpio_hp_det;
	int gpio_hp_mute;
	int gpio_int_mic_en;
	int gpio_ext_mic_en;
	int gpio_aic325x_reset ;
	int audio_port_id[NUM_I2S_DEVICES];
	struct baseband_config baseband_param;
	bool irq_active_low;   /* Set if IRQ active low, default high */

        /* Default register value for R6 (Mic bias), used to configure
	 * microphone detection.  In conjunction with gpio_cfg this
	 * can be used to route the microphone status signals out onto
	 * the GPIOs for use with snd_soc_jack_add_gpios().
	 */
	u16 micdet_cfg;

	int micdet_delay;      /* Delay after microphone detection (ms) */
	struct i2s_config i2s_param[NUM_I2S_DEVICES];
	int (*hw_detect)(void);
	
};
#endif

