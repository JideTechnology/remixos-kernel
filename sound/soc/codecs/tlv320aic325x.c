/*
 * linux/sound/soc/codecs/tlv320aic325x.c
 *
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 *
 * Based on sound/soc/codecs/wm8753.c by Liam Girdwood
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * History:
 *
 * Rev 0.1   ASoC driver support         		31-04-2009
 * The AIC32 ASoC driver is ported for the codec AIC325x.
 *
 *
 * Rev 1.0   Mini DSP support            		11-05-2009
 * Added mini DSP programming support
 *
 * Rev 1.1   Mixer controls              		18-01-2011
 * Added all the possible mixer controls.
 *
 * Rev 1.2   Additional Codec driver support          	2-02-2011
 * Support for AIC3253,AIC3206,AIC3256
 *
 * Rev 2.0   Ported the Codec driver to 2.6.39 kernel	30-03-2012
 *
 * Rev 2.1   PLL DAPM support added to the codec driver 03-04-2012
 *
 * Rev 2.2   Added event handlers for DAPM widgets	16-05-2012
 *	     Updated ENUM declerations
 *
 * Rev 2.3   Added updated PLL values as suggested      21-06-2012
 *	     by TI.
 *
 */

/*
 * Includes
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/jack.h>

#include <linux/slab.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include "tlv320aic325x.h"

/*
 * enable debug prints in the driver
 */
//#undef DBG
#ifdef DBG
	#define dprintk(x...)   printk(x)
#else
        #define dprintk(x...)
#endif

#define GPIO_SPKR_EN    BIT(0)
#define GPIO_RESET_EN BIT(1)

#define TIME_DELAY 			5
#define DELAY_COUNTER 		100

//#define HP_PA_CTL // normal HP PA control
static int has_hs = 0;

#define HP_DAC_VOL	 0xFC   //400
//#define HP_DAC_VOL	 0xEE  //177
#define SPK_DAC_VOL 0x00
//#if defined(CONFIG_MACH_T8400N) || defined(CONFIG_MACH_T1160N) || defined(CONFIG_MACH_NV8IA)
//#define LO_GAIN		6
//#elif  defined(CONFIG_MACH_TB610N)
//#define LO_GAIN		8
//#else
//#define LO_GAIN		0
//#endif
#define LO_GAIN 		6

#if defined(CONFIG_MACH_TB610N)
#define HP_GAIN		0 //0x3A//6  //0x3B//4 
#else
#define HP_GAIN		0x3B //0x3A//6  //0x3B//4 
#endif

#define HP_GAIN			3
/*
 * Macros
 */

#ifdef CONFIG_MINI_DSP
extern int aic325x_minidsp_program(struct snd_soc_codec *codec);
extern void aic325x_add_minidsp_controls(struct snd_soc_codec *codec);
#endif

#ifdef AIC325x_TiLoad
extern int aic325x_driver_init(struct snd_soc_codec *codec);
#endif


/* User defined Macros kcontrol builders */
#define SOC_SINGLE_AIC325x(xname)                                       \
	{                                                               \
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname,     \
		.info = __new_control_info, .get = __new_control_get, \
		.put = __new_control_put,                       \
		.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,      \
	}


/*
*****************************************************************************
* Function Prototype
*****************************************************************************
*/
inline void aic325x_write_reg_cache(struct snd_soc_codec *codec,
					u16 reg, u8 value);
int aic3i25x_reset_cache(struct snd_soc_codec *codec);
static int aic325x_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
				struct snd_soc_dai *);
static int aic325x_mute(struct snd_soc_dai *dai, int mute);
static int aic325x_set_dai_sysclk(struct snd_soc_dai *codec_dai,
					int clk_id, unsigned int freq, int dir);
static int aic325x_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt);
static int aic325x_set_bias_level(struct snd_soc_codec *codec,
					enum snd_soc_bias_level level);
static u8 aic325x_read(struct snd_soc_codec *codec, u16 reg);
static int __new_control_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo);
static int __new_control_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol);
static int __new_control_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol);
static int snd_soc_info_volsw_2r_aic325x(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo);

static int aic325x_hp_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event);
static int aic3256_cp_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event);
static int aic325x_lo_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event);
//int i2c_verify_book0(struct snd_soc_codec *codec);
int aic325x_change_page(struct snd_soc_codec *codec, u8 new_page);
int aic325x_wait_bits(struct snd_soc_codec *codec, unsigned int reg, 
			unsigned char mask, unsigned char val, 
			int sleep, int counter);
/*
*****************************************************************************
* Global Variable
*****************************************************************************
*/
static u8 aic325x_reg_ctl;

/* whenever aplay/arecord is run, aic325x_hw_params() function gets called.
 * This function reprograms the clock dividers etc. this flag can be used to
 * disable this when the clock dividers are programmed by pps config file
 */
static int soc_static_freq_config = 1;

static const char *mute[] = { "Unmute", "Mute" };

/* LDAC Mute Control */
SOC_ENUM_SINGLE_DECL(ldac_mute_enum, DAC_MUTE_CTRL_REG, 3, mute);

/* RDAC Mute COntrol */
SOC_ENUM_SINGLE_DECL(rdac_mute_enum, DAC_MUTE_CTRL_REG, 2, mute);

/* LADC Mute Control */
SOC_ENUM_SINGLE_DECL(ladc_mute_ctrl_enum, ADC_FGA, 7, mute);

/* RADC Mute COntrol */
SOC_ENUM_SINGLE_DECL(radc_mute_ctrl_enum, ADC_FGA, 3, mute);

/* Left HP Driver Mute Control */
SOC_ENUM_SINGLE_DECL(left_hp_mute_enum, HPL_GAIN, 6, mute);

/* Right HP Driver Mute Control */
SOC_ENUM_SINGLE_DECL(right_hp_mute_enum, HPR_GAIN, 6, mute);

/* Left LO Driver Mute control */
SOC_ENUM_SINGLE_DECL(left_lineout_drv_mute_enum, LOL_GAIN, 6, mute);

/* Right LO Driver Mute control */
SOC_ENUM_SINGLE_DECL(right_lineout_drv_mute_enum, LOR_GAIN, 6, mute);

/* DAC Volume Soft Step Control */
static const char *dacsoftstep_control[] = { "1 step/sample", "1 step/2 sample",
						"disabled" };
SOC_ENUM_SINGLE_DECL(dac_vol_soft_setp_enum, DAC_CHN_REG, 0, dacsoftstep_control);

/* Volume Mode Selection Control */
static const char *volume_extra[] = { "L&R Ind Vol", "LVol=RVol",
								"RVol=LVol" };
/* DAC Volume Mode Selection */
SOC_ENUM_SINGLE_DECL(dac_vol_extra_enum, DAC_MUTE_CTRL_REG, 0, volume_extra);

/* Beep Master Volume Control */
SOC_ENUM_SINGLE_DECL(beep_master_vol_enum, BEEP_CTRL_REG2, 6, volume_extra);

/* Headset Detection Enable/Disable Control */
static const char *headset_detection[] = { "Enabled","Disabled" };
SOC_ENUM_SINGLE_DECL(hs_det_ctrl_enum, HEADSET_DETECT, 7, headset_detection);

/* MIC BIAS Voltage Control */
static const char *micbias_voltage[] = { "1.04/1.25V", "1.425/1.7V",
						"2.075/2.5V", "POWER SUPPY" };
SOC_ENUM_SINGLE_DECL(micbias_voltage_enum, MICBIAS_CTRL, 4, micbias_voltage);

/* IN1L to Left MICPGA Positive Terminal Selection */
static const char *micpga_selection[] = { "off", "10k", "20k", "40k" };
SOC_ENUM_SINGLE_DECL(IN1L_LMICPGA_P_sel_enum, LMICPGA_PIN_CFG, 6, 
						micpga_selection);

/* IN2L to Left MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(IN2L_LMICPGA_P_sel_enum, LMICPGA_PIN_CFG, 4, 
						micpga_selection);

/* IN3L to Left MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(IN3L_LMICPGA_P_sel_enum, LMICPGA_PIN_CFG, 4, 
						micpga_selection);

/* IN1R to Left MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(IN1R_LMICPGA_P_sel_enum, LMICPGA_PIN_CFG, 2, 
						micpga_selection);

/* CM1L to Left MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(CM1L_LMICPGA_P_sel_enum, LMICPGA_PIN_CFG, 0, 
						micpga_selection);

/* IN2R to Left MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(IN2R_LMICPGA_P_sel_enum,LMICPGA_NIN_CFG, 6, 
						micpga_selection);

/* IN3R to Left MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(IN3R_LMICPGA_P_sel_enum, LMICPGA_NIN_CFG, 4, 
						micpga_selection);

/*CM2L to Left MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(CM2L_LMICPGA_P_sel_enum, LMICPGA_NIN_CFG, 2, 
						micpga_selection);

/* IN1R to Right MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(in1r_rmicpga_enum, RMICPGA_PIN_CFG, 6, 
						micpga_selection);

/* IN2R to Right MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(in2r_rmicpga_enum, RMICPGA_PIN_CFG, 4,  
						micpga_selection);

/* IN3R to Right MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(in3r_rmicpga_enum, RMICPGA_PIN_CFG, 2, 
						micpga_selection);

/* IN2L to Right MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(in2l_rmicpga_enum, RMICPGA_PIN_CFG, 0, 
						micpga_selection);

/* CM1R to Right MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(cm1r_rmicpga_enum, RMICPGA_NIN_CFG, 6, 
						micpga_selection);

/* IN1L to Right MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(in1l_rmicpga_enum, RMICPGA_NIN_CFG, 4, 
						micpga_selection);

/* IN3L to Right MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(in3l_rmicpga_enum, RMICPGA_NIN_CFG, 2, 
						micpga_selection);

/* CM2R to Right MICPGA Positive Terminal Selection */
SOC_ENUM_SINGLE_DECL(cm2r_rmicpga_enum, RMICPGA_NIN_CFG, 0, micpga_selection);

/* Power up/down */
static const char *powerup[] = { "Power Down", "Power Up" };

/* Mic Bias Power up/down */
SOC_ENUM_SINGLE_DECL(micbias_pwr_ctrl_enum, MICBIAS_CTRL, 6, powerup);

/* Left DAC Power Control */
SOC_ENUM_SINGLE_DECL(ldac_power_enum, DAC_CHN_REG, 7, powerup);

/* Right DAC Power Control */
SOC_ENUM_SINGLE_DECL(rdac_power_enum, DAC_CHN_REG, 6, powerup);

/* Left ADC Power Control */
SOC_ENUM_SINGLE_DECL(ladc_pwr_ctrl_enum, DAC_CHN_REG, 7, powerup);

/* Right ADC Power Control */
SOC_ENUM_SINGLE_DECL(radc_pwr_ctrl_enum, DAC_CHN_REG, 6, powerup);

/* HeadPhone Driver Power Control */
SOC_ENUM_DOUBLE_DECL(hp_pwr_ctrl_enum, OUT_PWR_CTRL, 5, 4, powerup);

/*Line-Out Driver Power Control */
SOC_ENUM_DOUBLE_DECL(lineout_pwr_ctrl_enum, OUT_PWR_CTRL, 3, 2, powerup);

/* Mixer Amplifiers Power Control */
SOC_ENUM_DOUBLE_DECL(mixer_amp_pwr_ctrl_enum, OUT_PWR_CTRL, 1, 0, powerup);

/* Mic Bias Generation */
static const char *vol_generation[] = { "AVDD", "LDOIN" };
SOC_ENUM_SINGLE_DECL(micbias_voltage_ctrl_enum, MICBIAS_CTRL, 3, 
						vol_generation);

/* DAC Data Path Control */
static const char *path_control[] = { "Disabled", "LDAC Data", "RDAC Data",
							 "L&RDAC Data" };
/* Left DAC Data Path Control */
SOC_ENUM_SINGLE_DECL(ldac_data_path_ctrl_enum, DAC_CHN_REG, 4, path_control);

/* Right DAC Data Path Control */
SOC_ENUM_SINGLE_DECL(rdac_data_path_ctrl_enum, DAC_CHN_REG, 2,  path_control);

/* Audio gain control (AGC) Enable/Disable Control */
static const char  *disable_enable[] = { "Disabled", "Enabled" };

/* Left Audio gain control (AGC) Enable/Disable Control */
SOC_ENUM_SINGLE_DECL(left_agc_enable_disable_enum, LEFT_AGC_REG1, 7, 
						disable_enable);

/* Left/Right Audio gain control (AGC) Enable/Disable Control */
SOC_ENUM_SINGLE_DECL(right_agc_enable_disable_enum, RIGHT_AGC_REG1, 7, 
						disable_enable);

/* Left MICPGA Gain Enabled/Disable */
SOC_ENUM_SINGLE_DECL(left_micpga_ctrl_enum, LMICPGA_VOL_CTRL, 7, 
						disable_enable);

/* Right MICPGA Gain Enabled/Disable */
SOC_ENUM_SINGLE_DECL(right_micpga_ctrl_enum, RMICPGA_VOL_CTRL, 7, 
						disable_enable);

/* DRC Enable/Disable Control */
SOC_ENUM_DOUBLE_DECL(drc_ctrl_enum, DRC_CTRL_REG1, 6, 5, disable_enable);

/* Beep generator Enable/Disable control */
SOC_ENUM_SINGLE_DECL(beep_gen_ctrl_enum, BEEP_CTRL_REG1, 7, disable_enable);

/* Headphone ground centered mode enable/disable control */
SOC_ENUM_SINGLE_DECL(hp_gnd_centred_mode_ctrl, HP_DRIVER_CONF_REG, 4, 
					disable_enable);

/* DMIC intput Selection control */
static const char  *dmic_input_sel[] = { "GPIO", "SCLK", "DIN" };
SOC_ENUM_SINGLE_DECL(dmic_input_enum, ADC_CHN_REG, 4, dmic_input_sel);

/*charge pump Enable*/
static const char  *charge_pump_ctrl_enum[] = { "Power_Down", "Reserved",\
								 "Power_Up" };
SOC_ENUM_SINGLE_DECL(charge_pump_ctrl, POW_CFG, 0, charge_pump_ctrl_enum);


/* Various Controls For AIC325x */
static const struct snd_kcontrol_new aic325x_snd_controls1[] = {

	/* IN1L to HPL Volume Control */
	SOC_SINGLE("IN1L to HPL volume control", IN1L_HPL_CTRL, 0, 0x72, 1),

	/* IN1R to HPR Volume Control */
	SOC_SINGLE("IN1R to HPR volume control", IN1R_HPR_CTRL, 0, 0x72, 1),

	/* IN1L to HPL routing */
	SOC_SINGLE("IN1L to HPL Route", HPL_ROUTE_CTRL, 2, 1, 0),

	/* MAL output to HPL */
	SOC_SINGLE("MAL Output to HPL Route", HPL_ROUTE_CTRL, 1, 1, 0),

	/*MAR output to HPL */
	SOC_SINGLE("MAR Output to HPL Route", HPL_ROUTE_CTRL, 0, 1, 0),

	/* IN1R to HPR routing */
	SOC_SINGLE("IN1R to HPR Route", HPR_ROUTE_CTRL, 2, 1, 0),

	/* MAR to HPR routing */
	SOC_SINGLE("MAR Output to HPR Route", HPR_ROUTE_CTRL, 1, 1, 0),

	/* HPL Output to HRP routing */
	SOC_SINGLE("HPL Output to HPR Route", HPR_ROUTE_CTRL, 0, 1, 0),

	/* MAL Output to LOL routing*/
	SOC_SINGLE("MAL Output to LOL Route", LOL_ROUTE_CTRL, 1, 1, 0),

	/* LOR Output to LOL routing*/
	SOC_SINGLE("LOR Output to LOL Route", LOL_ROUTE_CTRL, 0, 1, 0),

	/* MAR Output to LOR routing*/
	SOC_SINGLE("MAR Outout to LOR Route", LOR_ROUTE_CTRL, 1, 1, 0),

	/* DRC Threshold value  Control */
	SOC_SINGLE("DRC Threshold value",
					DRC_CTRL_REG1, 2, 0x07, 0),

	/* DRC Hysteresis value control */
	SOC_SINGLE("DRC Hysteresis value",
					DRC_CTRL_REG1, 0, 0x03, 0),

	/* DRC Hold time control */
	SOC_SINGLE("DRC hold time", DRC_CTRL_REG2, 3, 0x0F, 0),

	/* DRC Attack rate control */
	SOC_SINGLE("DRC attack rate", DRC_CTRL_REG3, 4, 0x0F, 0),

	/* DRC Decay rate control */
	SOC_SINGLE("DRC decay rate", DRC_CTRL_REG3, 0, 0x0F, 0),

	/* Beep Length MSB control */
	SOC_SINGLE("Beep Length MSB", BEEP_CTRL_REG3, 0, 255, 0),

	/* Beep Length MID control */
	SOC_SINGLE("Beep Length MID", BEEP_CTRL_REG4, 0, 255, 0),

	/* Beep Length LSB control */
	SOC_SINGLE("Beep Length LSB", BEEP_CTRL_REG5, 0, 255, 0),

	/* Beep Sin(x) MSB control */
	SOC_SINGLE("Beep Sin(x) MSB", BEEP_CTRL_REG6, 0, 255, 0),
	/* Beep Sin(x) LSB control */
	SOC_SINGLE("Beep Sin(x) LSB", BEEP_CTRL_REG7, 0, 255, 0),

	/* Beep Cos(x) MSB control */
	SOC_SINGLE("Beep Cos(x) MSB", BEEP_CTRL_REG8, 0, 255, 0),

	/* Beep Cos(x) LSB control */
	SOC_SINGLE("Beep Cos(x) LSB", BEEP_CTRL_REG9, 0, 255, 0),
};

/* DAC volume DB scale */
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -6350, 50, 0);
/* ADC volume DB scale */
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -1200, 50, 0);
/* Output Gain in DB scale */
static const DECLARE_TLV_DB_SCALE(output_gain_tlv, -600, 100, 0);
/* MicPGA Gain in DB */
static const DECLARE_TLV_DB_SCALE(micpga_gain_tlv, 0, 50, 0);

/* Various Controls For AIC325x */
static const struct snd_kcontrol_new aic325x_snd_controls2[] = {

	/* Left/Right DAC Digital Volume Control */
	//SOC_DOUBLE_R_SX_TLV("DAC Digital Volume Control",
	//		LDAC_VOL, RDAC_VOL, 8, 0xffffff81, 0x30, dac_vol_tlv),

	/* Left/Right ADC Fine Gain Adjust */
	//SOC_DOUBLE("L&R ADC Fine Gain Adjust", ADC_FGA, 4, 0, 0x04, 0),

	/* Left/Right ADC Volume Control */
	SOC_DOUBLE_R_SX_TLV("ADC Digital Volume Control",
		LADC_VOL, RADC_VOL, 7, 0xffffff68, 0x28 , adc_vol_tlv),

	/*HP Driver Gain Control*/
	//SOC_DOUBLE_R_SX_TLV("HP Driver Gain", HPL_GAIN, HPR_GAIN,
	//		6, 0xfffffffa, 0xe, output_gain_tlv),

	/*LO Driver Gain Control*/
	//SOC_DOUBLE_R_SX_TLV("Line Driver Gain", LOL_GAIN, LOR_GAIN, 6,
	//		0xfffffffa, 0x1d , output_gain_tlv),

	/* Mixer Amplifier Volume Control */
	SOC_DOUBLE_R("Mixer_Amp_Vol_Ctrl",
			MAL_CTRL_REG, MAR_CTRL_REG, 0, 0x28, 1),

	/*Left/Right MICPGA Volume Control */
	//SOC_DOUBLE_R_TLV("L&R_MICPGA_Vol_Ctrl",
	//LMICPGA_VOL_CTRL, RMICPGA_VOL_CTRL, 0, 0x5F, 0, micpga_gain_tlv),

	/* Beep generator Volume Control */
	SOC_DOUBLE_R("Beep_gen_Vol_Ctrl",
			BEEP_CTRL_REG1, BEEP_CTRL_REG2, 0, 0x3F, 1),

	/* Left/Right AGC Target level control */
	SOC_DOUBLE_R("AGC Target Level Control",
			LEFT_AGC_REG1, RIGHT_AGC_REG1, 4, 0x07, 1),

	/* Left/Right AGC Hysteresis Control */
	SOC_DOUBLE_R("AGC Hysteresis Control",
			LEFT_AGC_REG1, RIGHT_AGC_REG1, 0, 0x03, 0),

	/*Left/Right AGC Maximum PGA applicable */
	SOC_DOUBLE_R("AGC Maximum PGA Control",
			LEFT_AGC_REG3, RIGHT_AGC_REG3, 0, 0x7F, 0),

	/* Left/Right AGC Noise Threshold */
	SOC_DOUBLE_R("AGC Noise Threshold",
			LEFT_AGC_REG2, RIGHT_AGC_REG2, 1, 0x1F, 1),

	/* Left/Right AGC Attack Time control */
	SOC_DOUBLE_R("AGC Attack Time control",
		LEFT_AGC_REG4, RIGHT_AGC_REG4, 3, 0x1F, 0),

	/* Left/Right AGC Decay Time control */
	SOC_DOUBLE_R("AGC Decay Time control",
			LEFT_AGC_REG5, RIGHT_AGC_REG5, 3, 0x1F, 0),

	/* Left/Right AGC Noise Debounce control */
	SOC_DOUBLE_R("AGC Noice bounce control",
			LEFT_AGC_REG6, RIGHT_AGC_REG6, 0, 0x1F, 0),

	/* Left/Right AGC Signal Debounce control */
	SOC_DOUBLE_R("AGC_Signal bounce ctrl",
		LEFT_AGC_REG7, RIGHT_AGC_REG7, 0, 0x0F, 0),

	/* DAC Signal Processing Block Control*/
	SOC_SINGLE("DAC PRB Selection(1 to 25)", DAC_PRB,  0, 0x19, 0),
	/* ADC Signal Processing Block Control */
	SOC_SINGLE("ADC PRB Selection(1 to 18)", ADC_PRB,  0, 0x12, 0),

	/*charge pump configuration for n/8 peak load current*/
	//SOC_SINGLE("Charge_pump_peak_load_conf",
	//			CHRG_CTRL_REG, 4, 8, 0),

	/*charge pump clock divide control*/
	//SOC_SINGLE("charge_pump_clk_divider_ctrl", //CHRG_CTRL_REG, 0, 16, 0),

	/*HPL, HPR master gain control in ground centerd mode */
	SOC_SINGLE("HP_gain_ctrl_gnd_centered_mode",
				HP_DRIVER_CONF_REG, 5, 3, 0),

	/*headphone amplifier compensation adjustment */
	SOC_SINGLE(" hp_amp_compensation_adjustment",
				HP_DRIVER_CONF_REG, 7, 1, 0),

	/*headphone driver power configuration*/
	SOC_SINGLE(" HP_drv_pwr_conf",
				HP_DRIVER_CONF_REG, 2, 4, 0),

	/*DC offset correction*/
	SOC_SINGLE("DC offset correction", HP_DRIVER_CONF_REG, 0, 4, 0),

	/* sound new kcontrol for Programming the registers from user space */
	SOC_SINGLE_AIC325x("wreg"),
};

static const struct snd_kcontrol_new aic325x_snd_controls3[] = {

	SOC_ENUM("L_DAC_Mute_Control", ldac_mute_enum),
	SOC_ENUM("R_DAC_Mute_Control", rdac_mute_enum),
	SOC_ENUM("Left_ADC_Mute_Control", ladc_mute_ctrl_enum),
	SOC_ENUM("Right_ADC_Mute_Control", radc_mute_ctrl_enum),
	SOC_ENUM("Left HP driver mute", left_hp_mute_enum),
	SOC_ENUM("Right HP driver mute", right_hp_mute_enum),
	SOC_ENUM("Left LO driver mute", left_lineout_drv_mute_enum),
	SOC_ENUM("Right LO driver mute", right_lineout_drv_mute_enum),
	SOC_ENUM("DAC Volume Soft Stepping", dac_vol_soft_setp_enum),
	SOC_ENUM("DAC Extra Volume Control", dac_vol_extra_enum),
	SOC_ENUM("BEEP_MASTER_VOL_Enable/Disable", beep_master_vol_enum),
	//SOC_ENUM("Hs_det_control", hs_det_ctrl_enum ),
	SOC_ENUM("Mic Bias Vol", micbias_voltage_enum),
	SOC_ENUM("IN1L_LMICPGA_P_sel",IN1L_LMICPGA_P_sel_enum),
	SOC_ENUM("IN2L_LMICPGA_P_sel", IN2L_LMICPGA_P_sel_enum),
	SOC_ENUM("IN3L_LMICPGA_P_sel", IN3L_LMICPGA_P_sel_enum),
	SOC_ENUM("IN1R_LMICPGA_P_sel",	IN1R_LMICPGA_P_sel_enum),
	SOC_ENUM("CM1L_LMICPGA_P_sel", CM1L_LMICPGA_P_sel_enum),
	SOC_ENUM("IN3R_LMICPGA_P_sel", IN3R_LMICPGA_P_sel_enum),
	SOC_ENUM("CM2L_LMICPGA_P_sel", CM2L_LMICPGA_P_sel_enum),
	SOC_ENUM("IN1R_RMICPGA_P_terminal_sel",	in1r_rmicpga_enum),
	SOC_ENUM("IN2R_RMICPGA_P_terminal_sel",	in2r_rmicpga_enum),
	SOC_ENUM("IN3R_RMICPGA_P_terminal_sel",	in3r_rmicpga_enum),
	SOC_ENUM("IN2L_RMICPGA_P_terminal_sel",	in2l_rmicpga_enum),
	SOC_ENUM("CM1R_RMICPGA_P_terminal_sel",	cm1r_rmicpga_enum),
	SOC_ENUM("IN1L_RMICPGA_P_terminal_sel",	in1l_rmicpga_enum),
	SOC_ENUM("IN3L_RMICPGA_P_terminal_sel",	in3l_rmicpga_enum),
	SOC_ENUM("CM2R_RMICPGA_P_terminal_sel",	cm2r_rmicpga_enum),
	SOC_ENUM("Mic_Bias_Power_ctrl", micbias_pwr_ctrl_enum),
	SOC_ENUM("Left_DAC_channel_Power_ctrl",	ldac_power_enum),
	SOC_ENUM("Right_DAC_channel_Power_ctrl", rdac_power_enum),
	SOC_ENUM("Left_ADC_channel_Power_ctrl", ladc_pwr_ctrl_enum),
	SOC_ENUM("Right_ADC_channel_Power_ctrl", radc_pwr_ctrl_enum),
	SOC_ENUM("HP_Power_ctrl", hp_pwr_ctrl_enum),
	SOC_ENUM("LO_Power_ctrl", lineout_pwr_ctrl_enum),
	SOC_ENUM("Mixer_Amp_Power_ctrl", mixer_amp_pwr_ctrl_enum),
	SOC_ENUM("Mic Bias Voltage Generation",	micbias_voltage_ctrl_enum),
	SOC_ENUM("Left DAC Data Path Control", ldac_data_path_ctrl_enum),
	SOC_ENUM("Right DAC Data Path Control", rdac_data_path_ctrl_enum),
	SOC_ENUM("Left_AGC_Enable/Disable", left_agc_enable_disable_enum),
	SOC_ENUM("Right_AGC_Enable/Disable", right_agc_enable_disable_enum),
	SOC_ENUM("Left_MICPGA_Enable/Disable", left_micpga_ctrl_enum),
	SOC_ENUM("RIGHT_MICPGA_Enable/Disable", right_micpga_ctrl_enum),
	SOC_ENUM("DRC_CTRL_Enable/Disable", drc_ctrl_enum),
	SOC_ENUM("BEEP_GEN_Enable/Disable", beep_gen_ctrl_enum),
	SOC_ENUM("DMIC_Input_Selection", dmic_input_enum),
	SOC_ENUM("charge_pump_Enable_Disable", charge_pump_ctrl),
	SOC_ENUM("hp_ground_centered_mode_ctrl", hp_gnd_centred_mode_ctrl),

};

/* the sturcture contains the different values for mclk */
static const struct aic325x_rate_divs aic325x_divs[] = {
/*
 * mclk, rate, p_val, pll_j, pll_d, dosr, ndac, mdac, aosr, nadc, madc, blck_N,
 * codec_speficic_initializations
 */
	/* 8k rate */
#ifdef CONFIG_MINI_DSP
	{19200000, 8000, 1, 5, 1200, 768, 2, 8, 128, 2, 48, 4,
	 {} },
	{12288000, 8000, 1, 7, 5000, 768, 5, 3, 128, 5, 18, 24,
	{{60, 0}, {61, 0} } },
	{38400000, 8000, 1, 4, 1200, 256, 24, 4, 128, 12, 8, 8,
	 {} },
#else
	{12000000, 8000, 1, 8, 1920, 128, 12, 8, 128, 8, 6, 4,
	 {{60, 1}, {61, 1} } },
	{12288000, 8000, 1, 7, 5000, 768, 5, 3, 128, 5, 18, 24,
	{{60, 1}, {61, 1} } },
	{24000000, 8000, 1, 4, 96, 128, 12, 8, 128, 12, 8, 4,
	 {{60, 1}, {61, 1} } },
	{19200000, 8000, 1, 5, 1200, 768, 2, 8, 128, 2, 48, 4,
	 {{60, 1}, {61, 1} } },
	{38400000, 8000, 1, 4, 1200, 256, 24, 4, 128, 12, 8, 8,
	 {{60, 1}, {61, 1} } },
#endif
	/* 11.025k rate */
	{12000000, 11025, 1, 1, 8816, 1024, 8, 2, 128, 8, 2, 4,
	 {{60, 1}, {61, 1} } },
	{12288000, 11025, 1, 7, 3500, 768, 5, 3, 128, 5, 18, 24,
	{{60, 0}, {61, 0} } },
	{24000000, 11025, 1, 3, 7632, 128, 8, 8, 128, 8, 8, 4,
	 {{60, 1}, {61, 1} } },
	/* 16k rate */
	{12000000, 16000, 1, 8, 1920, 128, 8, 6, 128, 8, 6, 4,
	 {{60, 1}, {61, 1} } },
	{12288000, 16000, 1, 7, 5000, 768, 5, 3, 128, 5, 18, 24,
	{{60, 0}, {61, 0} } },
	{24000000, 16000, 1, 4, 96, 128, 8, 6, 128, 8, 6, 4,
	 {{60, 1}, {61, 1} } },
	{38400000, 16000, 1, 5, 1200, 256, 12, 4, 128, 8, 6, 8,
	 {{60, 1}, {61, 1} } },

	/* 22.05k rate */
	{12000000, 22050, 1, 3, 7632, 128, 8, 2, 128, 8, 2, 4,
	 {{60, 1}, {61, 1} } },
	{12288000, 22050, 1, 7, 3500, 768, 5, 3, 128, 5, 18, 24,
	{{60, 0}, {61, 0} } },
	{24000000, 22050, 1, 3, 7632, 128, 8, 3, 128, 8, 3, 4,
	 {{60, 1}, {61, 1} } },
	/* 32k rate */
	{12000000, 32000, 1, 5, 4613, 128, 8, 2, 128, 8, 2, 4,
	 {{60, 1}, {61, 1} } },
	{12288000, 32000, 1, 7, 5000, 768, 5, 3, 128, 5, 18, 24,
	{{60, 0}, {61, 0} } },
	{24000000, 32000, 1, 4, 96, 128, 6, 4, 128, 6, 4, 4,
	 {{60, 1}, {61, 1} } },
	{38400000, 32000, 1, 5, 1200, 256, 6, 4, 128, 6, 4, 8,
	 {{60, 1}, {61, 1} } },
	/* 44.1k rate */
#ifdef CONFIG_MINI_DSP
	{12000000, 44100, 1, 7, 5264, 128, 2, 8, 128, 2, 8, 4,
	 {} },
	{12288000, 44100, 1, 7, 3500, 768, 5, 3, 128, 5, 18, 24,
	{{}, {} } },
	{19200000, 44100, 1, 4, 7040, 128, 2, 8, 128, 2, 8, 4,
	 {} },
	{24000000, 44100, 2, 3, 7632, 128, 4, 4, 128, 4, 4, 4,
	 {} },
	{38400000, 44100, 2, 4, 7040, 128, 2, 8, 128, 2, 8, 4,
	 {} },
#else
	{12000000, 44100, 1, 7, 5264, 128, 8, 2, 128, 8, 2, 4,
	 {{60, 1}, {61, 1} } },
	{11289600, 44100, 1, 8, 0, 128, 2, 8, 128, 2, 8, 4,
	{{60,1}, {61,1} } },
	{12288000, 44100, 1, 7, 3500, 768, 5, 3, 128, 5, 18, 24,
	{{60, 1}, {61, 1} } },
	{19200000, 44100, 1, 4, 7040, 128, 8, 2, 128, 8, 2, 4,
	 {{60, 1}, {61, 1} } },
	{24000000, 44100, 1, 3, 7632, 128, 4, 4, 64, 4, 4, 4,
	 {{60, 1}, {61, 1} } },
	{38400000, 44100, 2, 4, 7040, 128, 2, 8, 128, 2, 8, 4,
	 {{60, 1}, {61, 1} } },
#endif
/* 48k rate */
#ifdef CONFIG_MINI_DSP
	{12000000, 48000, 1, 8, 000, 128, 2, 8, 128, 2, 8, 4,
	 {{}, {} } },
	{12288000, 48000, 1, 8, 0, 768, 5, 3, 128, 5, 18, 24,
	{{}, {} } },
	{19200000, 48000, 1, 5, 1200, 128, 2, 8, 128, 2, 8, 4,
	 {{}, {} } },
	{24000000, 48000, 1, 4, 960, 128, 4, 4, 128, 4, 4, 4,
	 {{}, {} } },
	{38400000, 48000, 2, 5, 1200, 128, 2, 8, 128, 2, 8, 4,
	 {{}, {} } },
#else
	/* 48k rate */
	{12000000, 48000, 1, 8, 000, 128, 8, 2, 128, 8, 2, 4,
	 {{60, 1}, {61, 1} } },
	{12288000, 48000, 1, 8, 0, 128, 2, 8, 128, 2, 8, 4,
	{{60, 1}, {61, 1} } },
	{19200000, 48000, 1, 5, 1200, 128, 8, 2, 128, 8, 2, 4,
	 {{60, 1}, {61, 1} } },
	{24000000, 48000, 1, 4, 960, 128, 4, 4, 128, 4, 4, 4,
	 {{60, 1}, {61, 1} } },
	{38400000, 48000, 2, 5, 1200, 128, 2, 8, 128, 2, 8, 4,
	 {{60, 1}, {61, 1} } },
#endif
	/*96k rate */
	{12000000, 96000, 1, 16, 3840, 128, 8, 2, 128, 8, 2 , 4,
	 {{60, 7}, {61, 7} } },
	{12288000, 96000, 1, 8, 0, 768, 5, 3, 128, 5, 18, 24,
	{{60, 0}, {61, 0} } },
	{24000000, 96000, 1, 4, 960, 128, 4, 2, 128, 4, 2, 2,
	 {{60, 7}, {61, 7} } },
	/*192k */
	{12000000, 192000, 1, 32, 7680, 128, 8, 2, 128, 8, 2, 4,
	 {{60, 17}, {61, 13} } },
	{12288000, 192000, 1, 8, 0, 768, 5, 3, 128, 5, 18, 24,
	{{60, 0}, {61, 0} } },
	{24000000, 192000, 1, 4, 960, 128, 2, 2, 128, 2, 2, 4,
	 {{60, 17}, {61, 13} } },
};

/*
 *----------------------------------------------------------------------------
 * @struct  snd_soc_codec_dai_ops |
 *          It is SoC Codec DAI Operations structure
 *----------------------------------------------------------------------------
 */
static struct snd_soc_dai_ops aic325x_dai_ops = {
	.hw_params = aic325x_hw_params,
	.digital_mute = aic325x_mute,
	.set_sysclk = aic325x_set_dai_sysclk,
	.set_fmt = aic325x_set_dai_fmt,
};

/*
 * It is SoC Codec DAI structure which has DAI capabilities viz., playback
 * and capture, DAI runtime information viz. state of DAI and pop wait state,
 * and DAI private data.  The aic31xx rates ranges from 8k to 192k The PCM
 * bit format supported are 16, 20, 24 and 32 bits
 */


static struct snd_soc_dai_driver tlv320aic325x_dai_driver[] = {
	{
	.name = "tlv320aic325x-MM_EXT",
	.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = AIC325x_RATES,
			.formats = AIC325x_FORMATS,
		},
	.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = AIC325x_RATES,
			.formats = AIC325x_FORMATS,
		},
	.ops = &aic325x_dai_ops,
},

};

/*
*****************************************************************************
* Initializations
*****************************************************************************
*/

/*
 * AIC325x register cache
 * We are caching the registers here.
 * There is no point in caching the reset register.
 * NOTE: In AIC325x, there are 127 registers supported in both page0 and page1
 *       The following table contains the page0 and page1 registers values.
 */
static const u8 aic325x_reg[AIC325x_CACHEREGNUM] = {
	0x00, 0x00, 0x00, 0x00,	/* 0 */
	0x00, 0x11, 0x04, 0x00,	/* 4 */
	0x00, 0x00, 0x00, 0x01,	/* 8 */
	0x01, 0x00, 0x80, 0x02,	/* 12 */
	0x00, 0x08, 0x01, 0x01,	/* 16 */
	0x80, 0x01, 0x00, 0x04,	/* 20 */
	0x00, 0x00, 0x01, 0x00,	/* 24 */
	0x00, 0x00, 0x01, 0x00,	/* 28 */
	0x00, 0x00, 0x00, 0x00,	/* 32 */
	0x00, 0x00, 0x00, 0x00,	/* 36 */
	0x00, 0x00, 0x00, 0x00,	/* 40 */
	0x00, 0x00, 0x00, 0x00,	/* 44 */
	0x00, 0x00, 0x00, 0x00,	/* 48 */
	0x00, 0x42, 0x02, 0x02,	/* 52 */
	0x42, 0x02, 0x02, 0x02,	/* 56 */
	0x00, 0x00, 0x00, 0x01,	/* 60 */
	0x01, 0x00, 0x14, 0x00,	/* 64 */
	0x0C, 0x00, 0x00, 0x00,	/* 68 */
	0x00, 0x00, 0x00, 0xEE,	/* 72 */
	0x10, 0xD8, 0x10, 0xD8,	/* 76 */
	0x00, 0x00, 0x88, 0x00,	/* 80 */
	0x00, 0x00, 0x00, 0x00,	/* 84 */
	0x7F, 0x00, 0x00, 0x00,	/* 88 */
	0x00, 0x00, 0x00, 0x00,	/* 92 */
	0x7F, 0x00, 0x00, 0x00,	/* 96 */
	0x00, 0x00, 0x00, 0x00,	/* 100 */
	0x00, 0x00, 0x00, 0x00,	/* 104 */
	0x00, 0x00, 0x00, 0x00,	/* 108 */
	0x00, 0x00, 0x00, 0x00,	/* 112 */
	0x00, 0x00, 0x00, 0x00,	/* 116 */
	0x00, 0x00, 0x00, 0x00,	/* 120 */
	0x00, 0x00, 0x00, 0x00,	/* 124 - PAGE0 Registers(127) ends here */
	0x01, 0x00, 0x08, 0x00,	/* 128, PAGE1-0 */
	0x00, 0x00, 0x00, 0x00,	/* 132, PAGE1-4 */
	0x00, 0x00, 0x00, 0x10,	/* 136, PAGE1-8 */
	0x00, 0x00, 0x00, 0x00,	/* 140, PAGE1-12 */
	0x40, 0x40, 0x40, 0x40,	/* 144, PAGE1-16 */
	0x00, 0x00, 0x00, 0x00,	/* 148, PAGE1-20 */
	0x00, 0x00, 0x00, 0x00,	/* 152, PAGE1-24 */
	0x00, 0x00, 0x00, 0x00,	/* 156, PAGE1-28 */
	0x00, 0x00, 0x00, 0x00,	/* 160, PAGE1-32 */
	0x00, 0x00, 0x00, 0x00,	/* 164, PAGE1-36 */
	0x00, 0x00, 0x00, 0x00,	/* 168, PAGE1-40 */
	0x00, 0x00, 0x00, 0x00,	/* 172, PAGE1-44 */
	0x00, 0x00, 0x00, 0x00,	/* 176, PAGE1-48 */
	0x00, 0x00, 0x00, 0x00,	/* 180, PAGE1-52 */
	0x00, 0x00, 0x00, 0x80,	/* 184, PAGE1-56 */
	0x80, 0x00, 0x00, 0x00,	/* 188, PAGE1-60 */
	0x00, 0x00, 0x00, 0x00,	/* 192, PAGE1-64 */
	0x00, 0x00, 0x00, 0x00,	/* 196, PAGE1-68 */
	0x00, 0x00, 0x00, 0x00,	/* 200, PAGE1-72 */
	0x00, 0x00, 0x00, 0x00,	/* 204, PAGE1-76 */
	0x00, 0x00, 0x00, 0x00,	/* 208, PAGE1-80 */
	0x00, 0x00, 0x00, 0x00,	/* 212, PAGE1-84 */
	0x00, 0x00, 0x00, 0x00,	/* 216, PAGE1-88 */
	0x00, 0x00, 0x00, 0x00,	/* 220, PAGE1-92 */
	0x00, 0x00, 0x00, 0x00,	/* 224, PAGE1-96 */
	0x00, 0x00, 0x00, 0x00,	/* 228, PAGE1-100 */
	0x00, 0x00, 0x00, 0x00,	/* 232, PAGE1-104 */
	0x00, 0x00, 0x00, 0x00,	/* 236, PAGE1-108 */
	0x00, 0x00, 0x00, 0x00,	/* 240, PAGE1-112 */
	0x00, 0x00, 0x00, 0x00,	/* 244, PAGE1-116 */
	0x00, 0x00, 0x00, 0x00,	/* 248, PAGE1-120 */
	0x00, 0x00, 0x00, 0x00	/* 252, PAGE1-124 */
};
static const struct aic325x_configs aic325x_reg_mic[]={
	       {MICBIAS_CTRL, 0x40},
       {86, 0xa0},
    // PGA
        {PAGE_1 + 59, 60},
        {PAGE_1 + 60, 60},
        {87, 50},
        {88, 80},
        {89, 22<<3},
        {90, 31<<3},
        {91, 110},
        {92, 110},
        // RIGHT AGC
        {94, 0xa0},
        {95, 50},
        {96, 80},
        {97, 22<<3},
        {98, 31<<3},
        {99, 110},
        {100, 110},
};
/*
 * aic325x initialization data
 * This structure initialization contains the initialization required for
 * AIC325x.
 * These registers values (reg_val) are written into the respective AIC325x
 * register offset (reg_offset) to  initialize AIC325x.
 * These values are used in aic325x_init() function only.
 */
#define DELAY_REG 45
static const struct aic325x_configs aic325x_reg_init_rec[] = {
	{0x89, 0x00},
	{DELAY_REG, 20},
	{1,	1},
	{DELAY_REG, 100},
	{0x0b, 0x81}, // NDAC
	{0x0c, 0x82}, // MDAC
	{0x12, 0x81}, // NADC
	{0x13, 0x82}, // MADC
	//{0x1b, 0x00}, // I2S 16b
	//{0x3c, 0x01}, // PRB
	//{0x3d, 0x01}, // PRB

	{0xec, 0x16}, // CP
	{0xec, 0x06}, // CP
	{0x81, 0x0a}, // CP on
	{0x82, 0x00}, // power on
	{DELAY_REG, 10},
	{0xbd, 0x00}, // PTM
	{0xc7, 0x32}, // analog input charging
	{0xeb, 0x05}, // vref
	{DELAY_REG, 50},

	{0x8c, 0x08}, // hp mix
	{0x8d, 0x08}, // hp mix
	{0x8e, 0x08}, // lo mix
	{0x8f, 0x08}, // lo mix

	/* HPL unmute and gain -5db */
	{HPL_GAIN, 0x40|HP_GAIN},
	/* HPR unmute and gain -5db */
	{HPR_GAIN, 0x40|HP_GAIN},

	{0x3f, 0xd6}, // dac
	{DELAY_REG, 20},
	{0xed, 0x12}, // hp config, calib
	{DELAY_REG, 5},
	{0x89, 0x3c}, // hp
	{DELAY_REG, 80},
	{0x3f, 0x15}, // dac
	{DELAY_REG, 10},
	//{0x8c, 0x00}, // hp mix
	//{0x8d, 0x00}, // hp mix
	{0x8e, 0x00}, // lo mix
	{0x8f, 0x00}, // lo mix
#ifdef HP_PA_CTL
	{0x89, 0x0c}, // hp
	{DELAY_REG, 30},
	{0xed, 0x12}, // hp config, calib
	{0x81, 0x08}, // CP off
	{DELAY_REG, 15},
#endif

	{65, SPK_DAC_VOL},
	{66, SPK_DAC_VOL},
	{DELAY_REG, 10},

	{0x0b, 0x01}, // NDAC
	{0x0c, 0x02}, // MDAC
	{0x12, 0x01}, // NADC
	{0x13, 0x02}, // MADC
	
	{0x44, 0x03}, // DRC

	/* Left mic PGA unmuted */
	{LMICPGA_VOL_CTRL, 0x00},
	/* Right mic PGA unmuted */
	{RMICPGA_VOL_CTRL, 0x00},
	/* ADC volume control change by 2 gain step per ADC Word Clock */
	{ADC_CHN_REG, 0x01},
	/* Unmute ADC left and right channels */
	{ADC_FGA, 0x00},

	/* PLL is CODEC_CLKIN */
	{CLK_REG_1, PLLCLK_2_CODEC_CLKIN},

	/* Connect IN1_L and IN1_R to CM */
	{INPUT_CFG_REG, 0xc0},
	/* PLL is CODEC_CLKIN */
	{CLK_REG_1, PLLCLK_2_CODEC_CLKIN},
	/* DAC_MOD_CLK is BCLK source */
	{AIS_REG_3, DAC_MOD_CLK_2_BDIV_CLKIN},

	/* Setting up DAC Channel */
	//{DAC_CHN_REG,
	// LDAC_2_LCHN | RDAC_2_RCHN | SOFT_STEP_2WCLK},

	{MICBIAS_CTRL, 0x40},
	/* Headphone powerup */
	{HPHONE_STARTUP_CTRL, 0x00},
	/* HPL unmute and gain -5db */
	{HPL_GAIN, 0x40|HP_GAIN},
	/* HPR unmute and gain -5db */
	{HPR_GAIN, 0x40|HP_GAIN},

	/* LOL unmute and gain 0db */
	{LOL_GAIN, LO_GAIN},
	/* LOR unmute and gain 0db */
	{LOR_GAIN, LO_GAIN},

	{HP_DRIVER_CONF_REG, 0x12},

	{GPIO_CTRL, 0x14 },
	/*  Headset Insertion event will generate a INT1 interrupt */
	{INT1_CTRL, 0x80},
	/*Enable headset detection and button press with a debounce time 64ms */
	{HEADSET_DETECT, 0x09},//0x89
	{PAGE_1 + 0x0a, 0x00},
	{PAGE_1 + 0x0b, 0x1E},
	// PGA
	{PAGE_1 + 59, 35}, // 63 //35
	{PAGE_1 + 60, 35},//63 //35

	{0xB4, 0x44}, // LPGA P
	{0xB6, 0x40}, // LPGA N
	{0x51, 0x80},

	{0x53,0x28},
	{0x54,0x28},
	{0x40,0x10},
/*	// LEFT AGC
	{86, 0xC3},
	{87, 0x68},
	{88, 70},
	{89, 0xB8},
	{90, 0xBA},
	{91, 0x0B},
	{92, 0x08},
	// RIGHT AGC
	{94, 0xC3},
	{95, 0x68},
	{96, 70},
	{97, 0xB8},
	{98, 0xBA},
	{99, 0x0B},
	{100, 0x08},
i*/
	// LEFT AGC
/*	
	{86, 0xa0},
	{87, 50},
	{88, 80},
	{89, 22<<3},
	{90, 31<<3},
	{91, 110},
	{92, 110},
	// RIGHT AGC
	{94, 0xa0},
	{95, 50},
	{96, 80},
	{97, 22<<3},
	{98, 31<<3},
	{99, 110},
	{100, 110},

*/
#if 0
	{PAGE_1 + 0x09, 0x00},

	{PAGE_0 + 0x0B, 0x81},

	{PAGE_0 + 0x0C, 0x82},

	{PAGE_0 + 0x1B, 0x00},

	{PAGE_0 + 0x3C, 0x08},

	{PAGE_0 + 0x3D, 0x01},

	{PAGE_1 + 0x7C, 0x06},

	{PAGE_1 + 0x01, 0x0A},

	{PAGE_1 + 0x02, 0x00},

	{PAGE_1 + 0x7B, 0x05},

	{PAGE_1 + 0x0C, 0x08},

	{PAGE_1 + 0x0D, 0x08},

	{PAGE_1 + 0x0E, 0x08},

	{PAGE_1 + 0x0F, 0x08},

	{PAGE_0 + 0x3F, 0xD6},

	{PAGE_1 + 0x7D, 0x12},

	{PAGE_1 + 0x10, 0x00},

	{PAGE_1 + 0x11, 0x00},

	{PAGE_1 + 0x09, 0xF0},

	{PAGE_0 + 0x40, 0x00},
#endif
};
static const struct aic325x_configs aic325x_reg_init[] = {
	{0x89, 0x00},
	{DELAY_REG, 20},
	{1,	1},
	{DELAY_REG, 100},
	{0x0b, 0x81}, // NDAC
	{0x0c, 0x82}, // MDAC
	{0x12, 0x81}, // NADC
	{0x13, 0x82}, // MADC
	//{0x1b, 0x00}, // I2S 16b
	//{0x3c, 0x01}, // PRB
	//{0x3d, 0x01}, // PRB

	{0xec, 0x16}, // CP
	{0xec, 0x06}, // CP
	{0x81, 0x0a}, // CP on
	{0x82, 0x00}, // power on
	{DELAY_REG, 10},
	{0xbd, 0x00}, // PTM
	{0xc7, 0x32}, // analog input charging
	{0xeb, 0x05}, // vref
	{DELAY_REG, 50},

	{0x8c, 0x08}, // hp mix
	{0x8d, 0x08}, // hp mix
	{0x8e, 0x08}, // lo mix
	{0x8f, 0x08}, // lo mix

	/* HPL unmute and gain -5db */
	{HPL_GAIN, 0x40|HP_GAIN},
	/* HPR unmute and gain -5db */
	{HPR_GAIN, 0x40|HP_GAIN},

	{0x3f, 0xd6}, // dac
	{DELAY_REG, 20},
	{0xed, 0x12}, // hp config, calib
	{DELAY_REG, 5},
	{0x89, 0x3c}, // hp
	{DELAY_REG, 80},
	{0x3f, 0x15}, // dac
	{DELAY_REG, 10},
	//{0x8c, 0x00}, // hp mix
	//{0x8d, 0x00}, // hp mix
	{0x8e, 0x00}, // lo mix
	{0x8f, 0x00}, // lo mix
#ifdef HP_PA_CTL
	{0x89, 0x0c}, // hp
	{DELAY_REG, 30},
	{0xed, 0x12}, // hp config, calib
	{0x81, 0x08}, // CP off
	{DELAY_REG, 15},
#endif

	{65, SPK_DAC_VOL},
	{66, SPK_DAC_VOL},
	{DELAY_REG, 10},

	{0x0b, 0x01}, // NDAC
	{0x0c, 0x02}, // MDAC
	{0x12, 0x01}, // NADC
	{0x13, 0x02}, // MADC
	
	{0x44, 0x03}, // DRC

	/* Left mic PGA unmuted */
	{LMICPGA_VOL_CTRL, 0x00},
	/* Right mic PGA unmuted */
	{RMICPGA_VOL_CTRL, 0x00},
	/* ADC volume control change by 2 gain step per ADC Word Clock */
	{ADC_CHN_REG, 0x01},
	/* Unmute ADC left and right channels */
	{ADC_FGA, 0x00},

	/* PLL is CODEC_CLKIN */
	{CLK_REG_1, PLLCLK_2_CODEC_CLKIN},

	/* Connect IN1_L and IN1_R to CM */
	{INPUT_CFG_REG, 0xc0},
	/* PLL is CODEC_CLKIN */
	{CLK_REG_1, PLLCLK_2_CODEC_CLKIN},
	/* DAC_MOD_CLK is BCLK source */
	{AIS_REG_3, DAC_MOD_CLK_2_BDIV_CLKIN},

	/* Setting up DAC Channel */
	//{DAC_CHN_REG,
	// LDAC_2_LCHN | RDAC_2_RCHN | SOFT_STEP_2WCLK},

	{MICBIAS_CTRL, 0x40},
	/* Headphone powerup */
	{HPHONE_STARTUP_CTRL, 0x00},
	/* HPL unmute and gain -5db */
	{HPL_GAIN, 0x40|HP_GAIN},
	/* HPR unmute and gain -5db */
	{HPR_GAIN, 0x40|HP_GAIN},

	/* LOL unmute and gain 0db */
	{LOL_GAIN, LO_GAIN},
	/* LOR unmute and gain 0db */
	{LOR_GAIN, LO_GAIN},

	{HP_DRIVER_CONF_REG, 0x12},

	{GPIO_CTRL, 0x14 },
	/*  Headset Insertion event will generate a INT1 interrupt */
	{INT1_CTRL, 0x80},
	/*Enable headset detection and button press with a debounce time 64ms */
	{HEADSET_DETECT, 0x09},//0x89
	{PAGE_1 + 0x0a, 0x00},
	{PAGE_1 + 0x0b, 0x1E},
	// PGA

#if defined(CONFIG_MACH_TB610N)
	{PAGE_1 + 59, 0x3B}, // 63
	{PAGE_1 + 60, 0x3B},//63
#else
	{PAGE_1 + 59, 63}, // 63
	{PAGE_1 + 60, 63},//63
#endif
	{0xB4, 0x44}, // LPGA P
	{0xB6, 0x40}, // LPGA N
	{0x51, 0x80},

	{0x53,0x28},
	{0x54,0x28},
	{0x40,0x10},
/*	// LEFT AGC
	{86, 0xC3},
	{87, 0x68},
	{88, 70},
	{89, 0xB8},
	{90, 0xBA},
	{91, 0x0B},
	{92, 0x08},
	// RIGHT AGC
	{94, 0xC3},
	{95, 0x68},
	{96, 70},
	{97, 0xB8},
	{98, 0xBA},
	{99, 0x0B},
	{100, 0x08},
i*/
	// LEFT AGC
#if defined(CONFIG_MACH_TB610N)	
	{86, 0xa0},
	{87, 50},
	{88, 80},
	{89, 22<<3},
	{90, 31<<3},
	{91, 110},
	{92, 110},
	// RIGHT AGC
	{94, 0xa0},
	{95, 50},
	{96, 80},
	{97, 22<<3},
	{98, 31<<3},
	{99, 110},
	{100, 110},
#endif
#if 0
	{PAGE_1 + 0x09, 0x00},

	{PAGE_0 + 0x0B, 0x81},

	{PAGE_0 + 0x0C, 0x82},

	{PAGE_0 + 0x1B, 0x00},

	{PAGE_0 + 0x3C, 0x08},

	{PAGE_0 + 0x3D, 0x01},

	{PAGE_1 + 0x7C, 0x06},

	{PAGE_1 + 0x01, 0x0A},

	{PAGE_1 + 0x02, 0x00},

	{PAGE_1 + 0x7B, 0x05},

	{PAGE_1 + 0x0C, 0x08},

	{PAGE_1 + 0x0D, 0x08},

	{PAGE_1 + 0x0E, 0x08},

	{PAGE_1 + 0x0F, 0x08},

	{PAGE_0 + 0x3F, 0xD6},

	{PAGE_1 + 0x7D, 0x12},

	{PAGE_1 + 0x10, 0x00},

	{PAGE_1 + 0x11, 0x00},

	{PAGE_1 + 0x09, 0xF0},

	{PAGE_0 + 0x40, 0x00},
#endif
};

// 800/300(2)/230
static void coeff_init(struct snd_soc_codec *codec)
{
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);
	unsigned char v[8];
#if 1
	unsigned char coeff_0[] = {
		0,
		0x00,0x7f,0xe1,0x00,
		0x00,0x80,0xa1,0x00,
		0x00,0x7e,0xee,0x00,
		0x00,0x7f,0x5f,0x00,
		0x00,0x81,0x30,0x00,
		};
	unsigned char coeff_1[] = { // eq
		0,
		0x00,0x7F,0x9d,0x00,
		0x00,0x83,0x63,0x00,
		0x00,0x7a,0x41,0x00,
		0x00,0x7c,0x9d,0x00,
		0x00,0x86,0x21,0x00,

		};
	unsigned char coeff_2[] = {
		0,	
		0x00, 0x7F, 0x8b, 0x00,
		0x00, 0x83, 0x82, 0x00,
		0x00, 0x7b, 0xfe, 0x00,
		0x00, 0x7c, 0x7e, 0x00,
		0x00, 0x84, 0x76, 0x00,
		};
	unsigned char coeff_3[] = {
		0,	
		0x00, 0x7F, 0xFF, 0x00,
		0x00, 0x80, 0xf0, 0x00,
		0x00, 0x7e, 0x8b, 0x00,
		0x00, 0x7f, 0x10, 0x00,
		0x00, 0x81, 0x75, 0x00,
		};
#else
	unsigned char coeff_0[] = {
		0,
		0x7F, 0xFF, 0xFF, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		};
	unsigned char coeff_1[] = {
		0,
		0x7F, 0xFF, 0xFF, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		};
	unsigned char coeff_2[] = {
		0,
		0x7F, 0xFF, 0xFF, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		};
#endif
	/*aic325x_write_reg_cache(codec, 0x00, 0x00);//page 0
	aic325x_write_reg_cache(codec, 0x3c, 0x01);//reg 60 pbr_p1
	aic325x_write_reg_cache(codec, 0x00, 0x2c);//page 44
	aic325x_write_reg_cache(codec, 0x01, 0x04);
	coeff_init(codec);
	aic325x_write_reg_cache(codec, 0x00, 0x2c);//page 44
	aic325x_write_reg_cache(codec, 0x01, 0x05);
	aic325x_wait_bits(codec,0x01,0x01,0x00,TIME_DELAY,DELAY_COUNTER);
	coeff_init(codec);*/
	v[0] = 0x00;
	v[1] = 0x00; //00<-00
	i2c_master_send((struct i2c_client *)(codec->control_data),
		v, 2);
	v[0] = 0x3c;
	v[1] = 0x01; //60<-01
	i2c_master_send((struct i2c_client *)(codec->control_data),
		v, 2);	
	v[0] = 0x00;
	v[1] = 0x2c; //00<-44
	i2c_master_send((struct i2c_client *)(codec->control_data),
		v, 2);	
	v[0] = 0x01;
	v[1] = 0x04; //01<-04
	i2c_master_send((struct i2c_client *)(codec->control_data),
		v, 2);
//	v[0] = 0;
//	v[1] = 44;
//	i2c_master_send((struct i2c_client *)(codec->control_data),
//		v, 2);
//----------------------------------------------------------------------
	coeff_0[0] = 12;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_1, 21);

	coeff_1[0] = 32;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_2, 21);

	coeff_2[0] = 52;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_0, 21);
	
//-------------------------------------------------------------------------
	v[0] = 0;
	v[1] = 45;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		v, 2);

	coeff_0[0] = 20;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_1, 21);

	coeff_1[0] = 40;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_2, 21);

	coeff_2[0] = 60;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_0, 21);
//------------------------------------------------------------------
	v[0] = 0x00;
	v[1] = 0x2c; //00<-44
	i2c_master_send((struct i2c_client *)(codec->control_data),
		v, 2);
	v[0] = 0x01;
	v[1] = 0x05; //01<-05
	i2c_master_send((struct i2c_client *)(codec->control_data),
		v, 2);	
	aic325x_wait_bits(codec,0x01,0x01,0x00,TIME_DELAY,DELAY_COUNTER);
//---------------------------------------------------------------------
	coeff_0[0] = 12;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_1, 21);

	coeff_1[0] = 32;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_2, 21);

	coeff_2[0] = 52;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_0, 21);
	
//-------------------------------------------------------------------------
	v[0] = 0;
	v[1] = 45;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		v, 2);

	coeff_0[0] = 20;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_1, 21);

	coeff_1[0] = 40;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_2, 21);

	coeff_2[0] = 60;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_0, 21);
//------------------------------------------------------------------
	aic325x->page_no = 45;
}
static void coeff_init_off(struct snd_soc_codec *codec)
{
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);

	unsigned char v[8];

#if 0
	unsigned char coeff_0[] = {
		0,
		0x76,0x3B,0x77,0x00,
		0x8D,0x0B,0x75,0x00,
		0x6F,0xAD,0xDD,0x00,
		0x72,0xF4,0x8B,0x00,
		0x9A,0x16,0xAB,0x00,
		};
	unsigned char coeff_1[] = { // eq
		0,
		0x76,0x3B,0x77,0x00,
		0x8D,0x0B,0xD0,0x00,
		0x6F,0xAD,0xDD,0x00,
		0x72,0xF4,0x30,0x00,
		0x9A,0x16,0xAB,0x00,
		};
	unsigned char coeff_2[] = {
		0,	
		0x7F,0xFF,0xFF,0x00,
		0x9F,0x2E,0x13,0x00,
		0x65,0xFD,0x39,0x00,
		0x64,0x97,0x85,0x00,
		0x91,0x0D,0x15,0x00,
		};
#else
	unsigned char coeff_0[] = {
		0,
		0x00, 0x7F, 0xFF, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		};
	unsigned char coeff_1[] = {
		0,
		0x00, 0x7F, 0xFF, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		};
	unsigned char coeff_2[] = {
		0,
		0x00, 0x7F, 0xFF, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		};
	unsigned char coeff_3[] = {
		0,
		0x00, 0x7F, 0xFF, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		};
#endif

	v[0] = 0;
	v[1] = 44;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		v, 2);

	coeff_1[0] = 12;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_1, 21);

	coeff_2[0] = 32;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_2, 21);

	coeff_0[0] = 52;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_0, 21);
	
	coeff_3[0] = 72;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_3, 21);

	v[0] = 0;
	v[1] = 45;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		v, 2);

	coeff_1[0] = 20;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_1, 21);

	coeff_2[0] = 40;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_2, 21);

	coeff_0[0] = 60;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_0, 21);

	coeff_3[0] = 80;
	i2c_master_send((struct i2c_client *)(codec->control_data),
		coeff_3, 21);

	aic325x->page_no = 45;
}
/* Left HPL Mixer */
static const struct snd_kcontrol_new hpl_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("L_DAC switch", HPL_ROUTE_CTRL, 3, 1, 0),
	SOC_DAPM_SINGLE("IN1_L switch", HPL_ROUTE_CTRL, 2, 1, 0),
	SOC_DAPM_SINGLE("MAL switch", HPL_ROUTE_CTRL, 1, 1, 0),
	SOC_DAPM_SINGLE("MAR switch", HPL_ROUTE_CTRL, 0, 1, 0),
};

static const char *adc_mux_text[] = {
	"Analog", "Digital"
};

SOC_ENUM_SINGLE_DECL(adcl_enum, ADC_CHN_REG, 3, adc_mux_text); 
SOC_ENUM_SINGLE_DECL(adcr_enum, ADC_CHN_REG, 2, adc_mux_text); 

static const struct snd_kcontrol_new adcl_mux =
	SOC_DAPM_ENUM("Left ADC Route", adcl_enum);

static const struct snd_kcontrol_new adcr_mux =
	SOC_DAPM_ENUM("Right ADC Route", adcr_enum);

/* Right HPR Mixer */
static const struct snd_kcontrol_new hpr_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("L_DAC switch", HPR_ROUTE_CTRL, 4, 1, 0),
	SOC_DAPM_SINGLE("R_DAC switch", HPR_ROUTE_CTRL, 3, 1, 0),
	SOC_DAPM_SINGLE("IN1_R switch", HPR_ROUTE_CTRL, 2, 1, 0),
	SOC_DAPM_SINGLE("MAR switch", HPR_ROUTE_CTRL, 1, 1, 0),
};

/* Left Line out mixer */
static const struct snd_kcontrol_new lol_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("R_DAC switch", LOL_ROUTE_CTRL, 4, 1, 0),
	SOC_DAPM_SINGLE("L_DAC switch", LOL_ROUTE_CTRL, 3, 1, 0),
	SOC_DAPM_SINGLE("MAL switch", LOL_ROUTE_CTRL, 1, 1, 0),
	SOC_DAPM_SINGLE("LOR switch", LOL_ROUTE_CTRL, 0, 1, 0),
};
/* Right Line out Mixer */
static const struct snd_kcontrol_new lor_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("R_DAC switch", LOR_ROUTE_CTRL, 3, 1, 0),
	SOC_DAPM_SINGLE("MAR switch", LOR_ROUTE_CTRL, 1, 1, 0),
};

/* Left Input Mixer */
static const struct snd_kcontrol_new left_input_mixer_controls[] = {
	SOC_DAPM_SINGLE("IN1_L switch", LMICPGA_PIN_CFG, 6, 3, 0),
	SOC_DAPM_SINGLE("IN2_L switch", LMICPGA_PIN_CFG, 4, 3, 0),
	SOC_DAPM_SINGLE("IN3_L switch", LMICPGA_PIN_CFG, 2, 3, 0),
	SOC_DAPM_SINGLE("IN1_R switch", LMICPGA_PIN_CFG, 0, 3, 0),

	SOC_DAPM_SINGLE("CM1L switch", LMICPGA_NIN_CFG, 6, 3, 0),
	SOC_DAPM_SINGLE("IN2_R switch", LMICPGA_NIN_CFG, 4, 3, 0),
	SOC_DAPM_SINGLE("IN3_R switch", LMICPGA_NIN_CFG, 2, 3, 0),
	SOC_DAPM_SINGLE("CM2L switch", LMICPGA_NIN_CFG, 0, 3, 0),
};

/* Right Input Mixer */
static const struct snd_kcontrol_new right_input_mixer_controls[] = {
	SOC_DAPM_SINGLE("IN1_R switch", RMICPGA_PIN_CFG, 6, 3, 0),
	SOC_DAPM_SINGLE("IN2_R switch", RMICPGA_PIN_CFG, 4, 3, 0),
	SOC_DAPM_SINGLE("IN3_R switch", RMICPGA_PIN_CFG, 2, 3, 0),
	SOC_DAPM_SINGLE("IN2_L switch", RMICPGA_PIN_CFG, 0, 3, 0),
	SOC_DAPM_SINGLE("CM1R switch", RMICPGA_NIN_CFG, 6, 3, 0),
	SOC_DAPM_SINGLE("IN1_L switch", RMICPGA_NIN_CFG, 4, 3, 0),
	SOC_DAPM_SINGLE("IN3_L switch", RMICPGA_NIN_CFG, 2, 3, 0),
	SOC_DAPM_SINGLE("CM2R switch", RMICPGA_NIN_CFG, 0, 3, 0),
};

int aic325x_wait_bits(struct snd_soc_codec *codec, unsigned int reg, 
			unsigned char mask, unsigned char val, 
			int sleep, int counter)
{
	int status;
	int timeout =  sleep*counter;

	status = snd_soc_read(codec, reg);

	while (((status & mask) != val) && counter) {
		msleep(sleep);
		status = snd_soc_read(codec, reg);
		counter--;
	};
	if(!counter)
		dev_dbg(codec->dev, 
		"wait_bits timedout (%d millisecs). lastval 0x%x\n",
		timeout,status);
	return counter;
}

static int pll_power_on_event(struct snd_soc_dapm_widget *w, \
                        struct snd_kcontrol *kcontrol, int event)
{
        struct snd_soc_codec *codec = w->codec;

       // if (event == SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD)
                msleep(50);
}

static int aic325x_dac_power_up_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	int reg_mask = 0;
	int ret_wbits = 0;
	struct snd_soc_codec *codec = w->codec;

	if (SND_SOC_DAPM_EVENT_ON(event)) {

		/* Check for the DAC FLAG register to know if the DAC is really
		 * powered up
		 */
	 	//if(machine_is_tb610n() || machine_is_t8400n())
		//coeff_init(codec);
		// printk("*****************************************%s,    ONs\n",__func__);
		if (w->shift == 7) {
			reg_mask = 0x80;
			ret_wbits = aic325x_wait_bits(codec,DAC_FLAG_1,reg_mask,
				reg_mask,TIME_DELAY,DELAY_COUNTER);
			if(!ret_wbits) {
			 	dev_err(w->codec->dev,
				"Line %d Func %s, LDAC_PMU wait_bits timedout\n",
				__LINE__,__FUNCTION__);
				return -1;
			}

		} else if (w->shift == 6) {
			reg_mask = 0x08;
			ret_wbits = aic325x_wait_bits(codec,DAC_FLAG_1,reg_mask,
				reg_mask,TIME_DELAY,DELAY_COUNTER);
			if(!ret_wbits) {
			 	dev_err(w->codec->dev,
				"Line %d Func %s, RDAC_PMU wait_bits timedout\n",
				__LINE__,__FUNCTION__);
				return -1;
			}
		}

	} else if (SND_SOC_DAPM_EVENT_OFF(event)) {

		/* Check for the DAC FLAG register to know if the DAC is
		 * powered down
		 */
	 	
		//  printk("*****************************************%s    off\n",__func__);
		if (w->shift == 7) {
			reg_mask = 0x80;
			ret_wbits = aic325x_wait_bits(codec,DAC_FLAG_1,reg_mask,
					0,TIME_DELAY,DELAY_COUNTER);
			if(!ret_wbits) {
			 	dev_err(w->codec->dev,
				"Line %d Func %s, LDAC_PMD wait_bits timedout\n",
				__LINE__,__FUNCTION__);
				return -1;
			}

		} else if (w->shift == 6) {
			reg_mask = 0x08;
			ret_wbits = aic325x_wait_bits(codec,DAC_FLAG_1,reg_mask,
					0,TIME_DELAY,DELAY_COUNTER);
			if(!ret_wbits) {
			 	dev_err(w->codec->dev,
				"Line %d Func %s, RDAC_PMD wait_bits timedout\n",
				__LINE__,__FUNCTION__);
				return -1;
			}
		}
	//	if(machine_is_t8400n() || machine_is_tb610n() )
	//		coeff_init_off(codec);
	}
	return 0;
}

static int aic325x_adc_power_up_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	int reg_mask = 0;
	int ret_wbits = 0;
	struct snd_soc_codec *codec = w->codec;

#if 0
	if (SND_SOC_DAPM_EVENT_ON(event)) {

		/* Check for the ADC FLAG register to know if the ADC is
		 * really powered up
		 */
		reg_mask = 0x40;
		ret_wbits = aic325x_wait_bits(codec,ADC_FLAG,reg_mask,reg_mask,
				TIME_DELAY,DELAY_COUNTER);
			
		//snd_soc_write(codec,HEADSET_DETECT ,0x09);
		if(!ret_wbits) {
			dev_err(w->codec->dev,
			"Line %d Func %s, ADC_ON wait_bits timedout\n",
			__LINE__,__FUNCTION__);
			return -1;
		}


	} else if (SND_SOC_DAPM_EVENT_OFF(event)) {

		/* Check for the ADC FLAG register to know if the ADC is
		 * powered down
		 */
		reg_mask = 0x40;
		ret_wbits = aic325x_wait_bits(codec,ADC_FLAG,reg_mask,0,
			TIME_DELAY,DELAY_COUNTER);

		//snd_soc_write(codec,HEADSET_DETECT ,0x89);
		if(!ret_wbits) {
			dev_err(w->codec->dev,
			"Line %d Func %s, ADC_OFF wait_bits timedout\n",
			__LINE__,__FUNCTION__);
			return -1;
		}
	}
#endif
	return 0;
}

/* AIC325x Widget Structure */
static const struct snd_soc_dapm_widget aic325x_dapm_widgets[] = {

	/* dapm widget (stream domain) for left DAC */
	SND_SOC_DAPM_DAC_E("Left DAC", "Left Playback", DAC_CHN_REG, 7, 0,
			aic325x_dac_power_up_event, SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_POST_PMD),

	/* dapm widget (path domain) for left DAC_L Mixer */
	SND_SOC_DAPM_MIXER("HPL Output Mixer", SND_SOC_NOPM, 0, 0,
			   &hpl_output_mixer_controls[0],
			   ARRAY_SIZE(hpl_output_mixer_controls)),

	/* dapm widget for Left Head phone Power */
#ifdef HP_PA_CTL
	SND_SOC_DAPM_PGA_E("HPL PGA", OUT_PWR_CTRL, 5, 0, NULL, 0,
				aic325x_hp_event, SND_SOC_DAPM_PRE_PMU),
#else
	SND_SOC_DAPM_PGA_E("HPL PGA", SND_SOC_NOPM, 5, 0, NULL, 0,
				aic325x_hp_event, SND_SOC_DAPM_PRE_PMD),
#endif

	/* dapm widget (path domain) for Left Line-out Output Mixer */
	SND_SOC_DAPM_MIXER("LOL Output Mixer", SND_SOC_NOPM, 0, 0,
	&lol_output_mixer_controls[0], ARRAY_SIZE(lol_output_mixer_controls)),

	/* dapm widget for Left Line-out Power */
	//SND_SOC_DAPM_PGA_E("LOL PGA", OUT_PWR_CTRL, 3, 0, NULL, 0,
	SND_SOC_DAPM_PGA_E("LOL PGA", SND_SOC_NOPM, 3, 0, NULL, 0,
				aic325x_lo_event, SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_PRE_PMD),

	/* dapm widget (stream domain) for right DAC */
	SND_SOC_DAPM_DAC_E("Right DAC", "Right Playback", DAC_CHN_REG, 6, 0,
				aic325x_dac_power_up_event, 
				SND_SOC_DAPM_POST_PMU |
				SND_SOC_DAPM_POST_PMD),

	/* dapm widget (path domain) for right DAC_R mixer */
	SND_SOC_DAPM_MIXER("HPR Output Mixer", SND_SOC_NOPM, 0, 0,
	&hpr_output_mixer_controls[0], ARRAY_SIZE(hpr_output_mixer_controls)),

	/* dapm widget for Right Head phone Power */
#ifdef HP_PA_CTL
	SND_SOC_DAPM_PGA_E("HPR PGA", OUT_PWR_CTRL, 4, 0, NULL, 0,
				aic325x_hp_event, SND_SOC_DAPM_POST_PMU),
#else
	SND_SOC_DAPM_PGA_E("HPR PGA", SND_SOC_NOPM, 4, 0, NULL, 0,
				aic325x_hp_event, SND_SOC_DAPM_POST_PMU),
#endif

	/* dapm widget for (path domain) Right Line-out Output Mixer */
	SND_SOC_DAPM_MIXER("LOR Output Mixer", SND_SOC_NOPM, 0, 0,
			   &lor_output_mixer_controls[0],
			   ARRAY_SIZE(lor_output_mixer_controls)),

	/* dapm widget for Right Line-out Power */
	//SND_SOC_DAPM_PGA_E("LOR PGA", OUT_PWR_CTRL, 2, 0, NULL, 0,
	SND_SOC_DAPM_PGA_E("LOR PGA", SND_SOC_NOPM, 2, 0, NULL, 0,
				aic325x_lo_event, SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_PRE_PMD),

	/* dapm supply widget for Charge pump */
#ifdef HP_PA_CTL
	SND_SOC_DAPM_SUPPLY("Charge Pump", POW_CFG, 1, 0, aic3256_cp_event,
						SND_SOC_DAPM_POST_PMU),
#endif
	/* Input DAPM widget for CM */
	SND_SOC_DAPM_INPUT("CM"),
	/* Input DAPM widget for CM1L */
	SND_SOC_DAPM_INPUT("CM1L"),
	/* Input DAPM widget for CM2L */
	SND_SOC_DAPM_INPUT("CM2L"),
	/* Input DAPM widget for CM1R */
	SND_SOC_DAPM_INPUT("CM1R"),
	/* Input DAPM widget for CM2R */
	SND_SOC_DAPM_INPUT("CM2R"),

	/* Stream widget for Left ADC */
	//SND_SOC_DAPM_ADC_E("Left ADC", "Left Capture", ADC_CHN_REG, 7, 0,
	SND_SOC_DAPM_ADC_E("Left ADC", "Left Capture", SND_SOC_NOPM, 7, 0,
			aic325x_adc_power_up_event, //SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_POST_PMD),


	/* Stream widget for Right ADC */
	//SND_SOC_DAPM_ADC_E("Right ADC", "Right Capture", ADC_CHN_REG, 6, 0,
	SND_SOC_DAPM_ADC_E("Right ADC", "Right Capture", SND_SOC_NOPM, 6, 0,
			aic325x_adc_power_up_event, //SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_POST_PMD),

	/* Left Inputs to Left MicPGA */
	SND_SOC_DAPM_PGA("Left MicPGA", LMICPGA_VOL_CTRL , 7, 1, NULL, 0),

	/* Right Inputs to Right MicPGA */
	SND_SOC_DAPM_PGA("Right MicPGA", RMICPGA_VOL_CTRL, 7, 1, NULL, 0),

	/* Left MicPGA to Mixer PGA Left */
	SND_SOC_DAPM_PGA("MAL PGA", OUT_PWR_CTRL , 1, 0, NULL, 0),

	/* Right Inputs to Mixer PGA Right */
	SND_SOC_DAPM_PGA("MAR PGA", OUT_PWR_CTRL, 0, 0, NULL, 0),

	/* dapm widget for Left Input Mixer*/
	SND_SOC_DAPM_MIXER("Left Input Mixer", SND_SOC_NOPM, 0, 0,
			   &left_input_mixer_controls[0],
			   ARRAY_SIZE(left_input_mixer_controls)),

	/* dapm widget for Right Input Mixer*/
	SND_SOC_DAPM_MIXER("Right Input Mixer", SND_SOC_NOPM, 0, 0,
			   &right_input_mixer_controls[0],
			   ARRAY_SIZE(right_input_mixer_controls)),
	/* dapm widget (platform domain) name for HPLOUT */
	SND_SOC_DAPM_OUTPUT("HPL"),

	/* dapm widget (platform domain) name for HPROUT */
	SND_SOC_DAPM_OUTPUT("HPR"),

	/* dapm widget (platform domain) name for LOLOUT */
	SND_SOC_DAPM_OUTPUT("LOL"),

	/* dapm widget (platform domain) name for LOROUT */
	SND_SOC_DAPM_OUTPUT("LOR"),

	/* dapm widget (platform domain) name for LINE1L */
	SND_SOC_DAPM_INPUT("IN1_L"),

	/* dapm widget (platform domain) name for LINE1R */
	SND_SOC_DAPM_INPUT("IN1_R"),

	/* dapm widget (platform domain) name for LINE2L */
	SND_SOC_DAPM_INPUT("IN2_L"),

	/* dapm widget (platform domain) name for LINE2R */
	SND_SOC_DAPM_INPUT("IN2_R"),

	/* dapm widget (platform domain) name for LINE3L */
	SND_SOC_DAPM_INPUT("IN3_L"),

	/* dapm widget (platform domain) name for LINE3R */
	SND_SOC_DAPM_INPUT("IN3_R"),
	
	/* DAPM widget for MICBIAS power control */
	SND_SOC_DAPM_MICBIAS("Mic Bias", MICBIAS_CTRL, 6, 0),

	/* Left DMIC Input Widget */
	SND_SOC_DAPM_INPUT("Left DMIC"),
	/* Right DMIC Input Widget */
	SND_SOC_DAPM_INPUT("Right DMIC"),

	/* Left Channel ADC Route */	
	SND_SOC_DAPM_MUX("Left ADC Route", SND_SOC_NOPM, 0, 0, &adcl_mux),
	/* Right Channel ADC Route */ 
	SND_SOC_DAPM_MUX("Right ADC Route", SND_SOC_NOPM, 0, 0, &adcr_mux), 

	/* Supply widget for PLL */
	SND_SOC_DAPM_SUPPLY("PLLCLK", CLK_REG_2, 7, 0, pll_power_on_event, \
                                SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	
	/* Supply widget for CODEC_CLK_IN */
        SND_SOC_DAPM_SUPPLY("CODEC_CLK_IN", SND_SOC_NOPM, 0, 0, NULL, 0),
	/* Supply widget for NDAC divider */
        SND_SOC_DAPM_SUPPLY("NDAC_DIV", NDAC_CLK_REG_6, 7, 0, NULL, 0),
	/* Supply widget for MDAC divider */
        SND_SOC_DAPM_SUPPLY("MDAC_DIV", MDAC_CLK_REG_7, 7, 0, NULL, 0),
	/* Supply widget for NADC divider */
        SND_SOC_DAPM_SUPPLY("NADC_DIV", NADC_CLK_REG_8, 7, 0, NULL, 0),
	/* Supply widget for MADC divider */
        SND_SOC_DAPM_SUPPLY("MADC_DIV", MADC_CLK_REG_9, 7, 0, NULL, 0),
	/* Supply widget for Bit Clock divider */
        SND_SOC_DAPM_SUPPLY("BCLK_N_DIV", CLK_REG_11, 7, 0, NULL, 0),

};

static const struct snd_soc_dapm_route aic325x_dapm_routes[] = {

	/* PLL routing */
	{"CODEC_CLK_IN", NULL, "PLLCLK"},
	{"NDAC_DIV", NULL, "CODEC_CLK_IN"},
	{"NADC_DIV", NULL, "CODEC_CLK_IN"},
	{"MDAC_DIV", NULL, "NDAC_DIV"},
	{"MADC_DIV", NULL, "NADC_DIV"},
	{"BCLK_N_DIV", NULL, "MADC_DIV"},
	{"BCLK_N_DIV", NULL, "MDAC_DIV"},

	/* Clock routing for ADC */
	{"Left ADC", NULL, "MADC_DIV"},
	{"Right ADC", NULL, "MADC_DIV"},
	{"Left ADC", NULL, "BCLK_N_DIV"},
	{"Right ADC", NULL, "BCLK_N_DIV"},

	/* Clock routing for DAC */
	{"Left DAC", NULL, "MDAC_DIV" },
	{"Right DAC", NULL, "MDAC_DIV"},
	{"Left DAC", NULL, "BCLK_N_DIV" },
	{"Right DAC", NULL, "BCLK_N_DIV"},

	/* Left  Headphone Output */
	{"HPL Output Mixer", "L_DAC switch", "Left DAC"},
	{"HPL Output Mixer", "IN1_L switch", "IN1_L"},
	{"HPL Output Mixer", "MAL switch", "MAL PGA"},
	{"HPL Output Mixer", "MAR switch", "MAR PGA"},

	/* Right Headphone Output */
	{"HPR Output Mixer", "R_DAC switch", "Right DAC"},
	{"HPR Output Mixer", "IN1_R switch", "IN1_R"},
	{"HPR Output Mixer", "MAR switch", "MAR PGA"},
	{"HPR Output Mixer", "L_DAC switch", "Left DAC"},

	/* HP output mixer to HP PGA */
	{"HPL PGA", NULL, "HPL Output Mixer"},
	{"HPR PGA", NULL, "HPR Output Mixer"},

	/* HP PGA to HP output pins */
	{"HPL", NULL, "HPL PGA"},
	{"HPR", NULL, "HPR PGA"},

	/* Charge pump to HP PGA */
#ifdef HP_PA_CTL
	{"HPL PGA", NULL, "Charge Pump"},
	{"HPR PGA", NULL, "Charge Pump"},
#endif

	/* Left Line-out Output */
	{"LOL Output Mixer", "L_DAC switch", "Left DAC"},
	{"LOL Output Mixer", "MAL switch", "MAL PGA"},
	{"LOL Output Mixer", "R_DAC switch", "Right DAC"},
	{"LOL Output Mixer", "LOR switch", "LOR PGA"},

	/* Right Line-out Output */
	{"LOR Output Mixer", "R_DAC switch", "Right DAC"},
	{"LOR Output Mixer", "MAR switch", "MAR PGA"},

	{"LOL PGA", NULL, "LOL Output Mixer"},
	{"LOR PGA", NULL, "LOR Output Mixer"},

	{"LOL", NULL, "LOL PGA"},
	{"LOR", NULL, "LOR PGA"},

	/* ADC portions */
	/* Left Positive PGA input */
	{"Left Input Mixer", "IN1_L switch", "IN1_L"},
	{"Left Input Mixer", "IN2_L switch", "IN2_L"},
	{"Left Input Mixer", "IN3_L switch", "IN3_L"},
	{"Left Input Mixer", "IN1_R switch", "IN1_R"},
	/* Left Negative PGA input */
	{"Left Input Mixer", "IN2_R switch", "IN2_R"},
	{"Left Input Mixer", "IN3_R switch", "IN3_R"},
	{"Left Input Mixer", "CM1L switch", "CM1L"},
	{"Left Input Mixer", "CM2L switch", "CM2L"},
	/* Right Positive PGA Input */
	{"Right Input Mixer", "IN1_R switch", "IN1_R"},
	{"Right Input Mixer", "IN2_R switch", "IN2_R"},
	{"Right Input Mixer", "IN3_R switch", "IN3_R"},
	{"Right Input Mixer", "IN2_L switch", "IN2_L"},
	/* Right Negative PGA Input */
	{"Right Input Mixer", "IN1_L switch", "IN1_L"},
	{"Right Input Mixer", "IN3_L switch", "IN3_L"},
	{"Right Input Mixer", "CM1R switch", "CM1R"},
	{"Right Input Mixer", "CM2R switch", "CM2R"},

	{"CM1L", NULL, "CM"},
	{"CM2L", NULL, "CM"},
	{"CM1R", NULL, "CM"},
	{"CM1R", NULL, "CM"},

	/* Left MicPGA */
	{"Left MicPGA", NULL, "Left Input Mixer"},

	/* Right MicPGA */
	{"Right MicPGA", NULL, "Right Input Mixer"},

	{"Left ADC", NULL, "Left MicPGA"},
	{"Right ADC", NULL, "Right MicPGA"},

	{"Left ADC Route", "Analog", "Left MicPGA"},
	{"Left ADC Route", "Digital", "Left DMIC"},
	
	/* Selection of Digital/Analog Mic */
	{"Right ADC Route", "Analog", "Right MicPGA"},
	{"Right ADC Route", "Digital", "Right DMIC"},

	{"Left ADC", NULL, "Left ADC Route"},
	{"Right ADC", NULL, "Right ADC Route"},

	{"MAL PGA", NULL, "Left MicPGA"},
	{"MAR PGA", NULL, "Right MicPGA"},
};

#define AIC325x_DAPM_ROUTE_NUM (sizeof(aic325x_dapm_routes)/\
			sizeof(struct snd_soc_dapm_route))

/*
*****************************************************************************
* Function Definitions
*****************************************************************************
*/

#if 0
int i2c_verify_book0(struct snd_soc_codec *codec)
{
	int i, j, k = 0;
	u8 val1;

	dprintk(KERN_INFO "starting i2c_verify\n");
	dprintk(KERN_INFO "Resetting page to 0\n");
	for (j = 0; j < 2; j++) {
		if (j == 0) {
			aic325x_change_page(codec, 0);
			k = 0;
		}
		if (j == 1) {
			aic325x_change_page(codec, 1);
			k = 1;
		}
		for (i = 0; i <= 127; i++)
			val1 = i2c_smbus_read_byte_data(codec->control_data, i);
	}
	return 0;
}
#endif

/*
 * Event for line out driver power changes.
 */
//static int spk_powred_once = 0;
//static int spk_powered = 0;
//static int gain_to_set = 0;
static int aic325x_lo_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	int reg_mask = 0;
	int ret_wbits = 0;
	int value;

	struct snd_soc_codec *codec = w->codec;
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);

	if(event & SND_SOC_DAPM_PRE_PMD)
	{
	//	if(spk_powered)
	//	{
	//		spk_powered = 0;
			//snd_soc_write(codec, 0x92, 0x40|LO_GAIN);
			//snd_soc_write(codec, 0x93, 0x40|LO_GAIN);
			//msleep(8);
			// disable PA
		//	printk("******************************************aic325x Disable PA\n");
			// pa todo: disable
			if(0){//(machine_is_nabi2_xd()){
				aic325x_dac_power_up_event(w,kcontrol, SND_SOC_DAPM_POST_PMD);
			}
	//		if(gpio_is_valid(aic325x->pdata->gpio_spkr_en))
	//			gpio_direction_output(aic325x->pdata->gpio_spkr_en, 0);

			if(0){//(machine_is_nabi2_xd()){
				if (w->shift == 7) {
					reg_mask = 0x80;
					ret_wbits = aic325x_wait_bits(codec,DAC_FLAG_1,reg_mask,
						reg_mask,TIME_DELAY,DELAY_COUNTER);
					if(!ret_wbits) {
					 	dev_err(w->codec->dev,
						"Line %d Func %s, LDAC_PMU wait_bits timedout\n",
						__LINE__,__FUNCTION__);
						return -1;
					}

				} else if (w->shift == 6) {
					reg_mask = 0x08;
					ret_wbits = aic325x_wait_bits(codec,DAC_FLAG_1,reg_mask,
						reg_mask,TIME_DELAY,DELAY_COUNTER);
					if(!ret_wbits) {
					 	dev_err(w->codec->dev,
						"Line %d Func %s, RDAC_PMU wait_bits timedout\n",
						__LINE__,__FUNCTION__);
						return -1;
					}
		}
				}
			//msleep(8);
			//snd_soc_write(codec, 0x92, 0x40);
			//snd_soc_write(codec, 0x93, 0x40);
//		}
	}

	if (event & SND_SOC_DAPM_POST_PMU) {
		if (w->shift == 3) {
			reg_mask = 0x40;
			ret_wbits = aic325x_wait_bits(codec,DAC_FLAG_1,
				reg_mask,reg_mask,TIME_DELAY,DELAY_COUNTER);
			if(!ret_wbits) {
				dev_err(w->codec->dev,
			"Line %d Func %s, LOL_PMU wait_bits timedout\n",
			__LINE__,__FUNCTION__);
				return -1;
			}
		}
		if (w->shift == 2) {
			reg_mask = 0x04;
			ret_wbits = aic325x_wait_bits(codec,DAC_FLAG_1,
				reg_mask,reg_mask, TIME_DELAY,DELAY_COUNTER);
			if(!ret_wbits) {
				dev_err(w->codec->dev,
			"Line %d Func %s, LOR_PMU wait_bits timedout\n",
			__LINE__,__FUNCTION__);
				return -1;
			}
		}
	
//		spk_powered ++;
		//value = snd_soc_read(codec, OUT_PWR_CTRL);
		//if((value & (0x03 << 2)) == (0x03 << 2))
	//	if(spk_powered == 2)
//		{
			//if(spk_powred_once)
//			{
//				gain_to_set = 1;
				//snd_soc_write(codec, 0x92, 0x40|LO_GAIN);
				//snd_soc_write(codec, 0x93, 0x40|LO_GAIN);
//				msleep(30);
			//	printk("********************************************8888aic325x Enable PA\n");
				// enable PA
				// pa todo: enable
	//			if(gpio_is_valid(aic325x->pdata->gpio_spkr_en))
	//				gpio_direction_output(aic325x->pdata->gpio_spkr_en, 1);
				//msleep(8);
				//test snd_soc_write(codec, 0x92, LO_GAIN);
				// testsnd_soc_write(codec, 0x93, LO_GAIN);
				snd_soc_write(codec, 65, SPK_DAC_VOL);
				snd_soc_write(codec, 66, SPK_DAC_VOL);
				msleep(10);
			//} else {
			//	spk_powred_once = 1;
//			}
//		}
	}
	return 0;
}

/*
 * Event for headphone amplifier power changes.Special
 * power up/down sequences are required in order to maximise pop/click
 * performance.
 */
#ifdef HP_PA_CTL
static int aic325x_hp_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	int reg_mask = 0;
	int ret_wbits = 0;

	struct snd_soc_codec *codec = w->codec;

	/* Before powering up, MUTE the HPL and HPR Drivers */
	if (event & SND_SOC_DAPM_PRE_PMU) {
		if(w->shift == 4) {
			snd_soc_write(codec, HPR_GAIN, 0x40|HP_GAIN);
			//snd_soc_update_bits (codec, HPR_GAIN, 0x40, 0x01);
		}
		if(w->shift == 5) {
			snd_soc_write(codec, HPL_GAIN, 0x40|HP_GAIN);
			//snd_soc_update_bits (codec, HPL_GAIN, 0x40, 0x01);
		}
	}
	if (event & SND_SOC_DAPM_POST_PMU) {
		if(w->shift == 4)
			reg_mask = 0x02;
		if(w->shift == 5)
			reg_mask = 0x20;

		ret_wbits = aic325x_wait_bits(codec,DAC_FLAG_1,
				reg_mask,reg_mask,TIME_DELAY,DELAY_COUNTER);
		if(!ret_wbits) {
			dev_err(w->codec->dev,
			"Line %d Func %s, HP_PMU [%s] wait_bits timedout\n",
			__LINE__,__FUNCTION__, w->name);
		}
	}
	return 0;
}
#else
static int aic325x_hp_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	int reg_mask = 0;
	int ret_wbits = 0;

	struct snd_soc_codec *codec = w->codec;

	if (event & SND_SOC_DAPM_POST_PMU) {
		//printk("aic325x Enable HP\n");
		snd_soc_write(codec, 65, 0x81);
		snd_soc_write(codec, 66, 0x81);
		msleep(8);
		snd_soc_write(codec, HPR_GAIN, HP_GAIN);
		snd_soc_write(codec, HPL_GAIN, HP_GAIN);
		snd_soc_write(codec, 65, HP_DAC_VOL);
		snd_soc_write(codec, 66, HP_DAC_VOL);
		msleep(8);

	}
	if (event & SND_SOC_DAPM_PRE_PMD) {
		//printk("aic325x Disable HP\n");
		snd_soc_write(codec, 65, 0x81);
		snd_soc_write(codec, 66, 0x81);
		msleep(8);
	//	snd_soc_write(codec, HPR_GAIN, 0x40|HP_GAIN);
	//	snd_soc_write(codec, HPL_GAIN, 0x40|HP_GAIN);
		snd_soc_write(codec, 65, SPK_DAC_VOL);
		snd_soc_write(codec, 66, SPK_DAC_VOL);
		msleep(8);
	}

	return 0;
}
#endif

static int aic3256_cp_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{

		if (event & SND_SOC_DAPM_POST_PMU)
			msleep(20);
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : __new_control_info
 * Purpose  : This function is to initialize data for new control required to
 *            program the AIC325x registers.
 *----------------------------------------------------------------------------
 */
static int __new_control_info(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 65535;
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : __new_control_get
 * Purpose  : This function is to read data of new control for
 *            program the AIC325x registers.
 *----------------------------------------------------------------------------
 */
static int __new_control_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u32 val;
	val = aic325x_read(codec, aic325x_reg_ctl);
	ucontrol->value.integer.value[0] = val;
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : __new_control_put
 * Purpose  : new_control_put is called to pass data from user/application to
 *            the driver.
 *----------------------------------------------------------------------------
 */
static int __new_control_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);
	u32 data_from_user = ucontrol->value.integer.value[0];
	u8 data[2];

	aic325x_reg_ctl = data[0] = (u8) ((data_from_user & 0xFF00) >> 8);
	data[1] = (u8) ((data_from_user & 0x00FF));

#if 0
	if (!data[0])
		aic325x->page_no = data[1];
	if (codec->hw_write(codec->control_data, data, 2) != 2) {
		printk("Error in i2c write\n");
		return -EIO;
	}
#else
	snd_soc_write(codec, data[0], data[1]);
#endif

	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_reset_cache
 * Purpose  : This function is to reset the cache.
 *----------------------------------------------------------------------------
 */
int aic325x_reset_cache(struct snd_soc_codec *codec)
{
#if defined(EN_REG_CACHE)
	if (codec->reg_cache != NULL) {
		memcpy(codec->reg_cache, aic325x_reg, sizeof(aic325x_reg));
		return 0;
	}
	codec->reg_cache = kmemdup(aic325x_reg, sizeof(aic325x_reg),
							GFP_KERNEL);
	if (codec->reg_cache == NULL) {
		printk(KERN_ERR "aic325x: kmemdup failed\n");
		return -ENOMEM;
	}
#endif
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_hw_write
 * Purpose  : i2c write function
 *----------------------------------------------------------------------------
 */
int aic325x_hw_write(struct snd_soc_codec *codec, const char *buf, 
						unsigned int count)
{
	struct i2c_client *client = codec->control_data;
	u8 data[3];
	int ret;	

	data[0] = *buf;
	data[1] = *(buf+1);
	data[2] = *(buf+2);
	
	ret = i2c_master_send(codec->control_data, data, count);

	if (ret < count) {
		printk(KERN_ERR "#%s: I2C write Error: bytes written = %d @ %d\n\n", 
				__func__, ret, data[1]);
		return -EIO;
	}
	return ret;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_hw_read
 * Purpose  : i2c read function
 *----------------------------------------------------------------------------
 */
int aic325x_hw_read(struct snd_soc_codec *codec, unsigned int reg)
{
	struct i2c_client *client = codec->control_data;
	int data =0;
	
	if(i2c_master_send(client, (char *)&reg, 1) < 0){
		printk(KERN_ERR "%s : I2C write Error @ %d\n", __func__, reg);
		return -EIO;
	}
		
	if(i2c_master_recv(client, (char *)&data, 1) < 0) {
		printk(KERN_ERR "%s : I2C read Error @ %d\n", __func__, reg);
		return -EIO;
	}

	return (data & 0x00FF);
}
/*
 *----------------------------------------------------------------------------
 * Function : aic325x_change_page
 * Purpose  : This function is to switch between page 0 and page 1.
 *----------------------------------------------------------------------------
 */
int aic325x_change_page(struct snd_soc_codec *codec, u8 new_page)
{
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);
	u8 data[4];

	data[0] = 0;
	data[1] = new_page;

	if (codec->hw_write(codec, (char *)&data, 2) < 0) {
		printk(KERN_ERR "#%s: Error in changing page to %d\n", 
				__func__, new_page);
		return -EIO;
	}
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_write_reg_cache
 * Purpose  : This function is to write aic325x register cache
 *----------------------------------------------------------------------------
 */
inline void aic325x_write_reg_cache(struct snd_soc_codec *codec,
					u16 reg, u8 value)
{
	u8 *cache = codec->reg_cache;
	if (reg >= AIC325x_CACHEREGNUM)
		return;
	cache[reg] = value;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_write
 * Purpose  : This function is to write to the aic325x register space.
 *----------------------------------------------------------------------------
 */
int aic325x_write(struct snd_soc_codec *codec, u16 reg, u8 value)
{
	u8 data[3];
	u8 page;
	int ret;
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&aic325x->io_lock);
	page = reg / 128;
	data[AIC325x_REG_OFFSET_INDEX] = reg % 128;

	if (aic325x->page_no != page) {
		ret = aic325x_change_page(codec, page);
		if (ret < 0) {
			dprintk("#%s: Change page failed for page= %d\n", 
					__func__, page);
			mutex_unlock(&aic325x->io_lock);
			return -EIO;
		}
		/* Change Page successful. Update the Global Pointer */
		aic325x->page_no = page;
	}
	/* data is
	*   D15..D8 aic325x register offset
	*   D7...D0 register data
	*/
	data[AIC325x_REG_DATA_INDEX] = value & AIC325x_8BITS_MASK;

#if defined(EN_REG_CACHE)
	if ((page == 0) || (page == 1))
		aic325x_write_reg_cache(codec, reg, value);
#endif
	dprintk("#%s: Page %d Reg %d Value 0x%x\r\n", __func__,
		page, (reg % 128), value); 

	if (codec->hw_write(codec, (char *)&data, 2) < 0) {
		printk(KERN_ERR "#%s: Error in i2c write\n", __func__);
		mutex_unlock(&aic325x->io_lock);
		return -EIO;
	}

	mutex_unlock(&aic325x->io_lock);
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_read
 * Purpose  : This function is to read the aic325x register space.
 *----------------------------------------------------------------------------
 */
static u8 aic325x_read(struct snd_soc_codec *codec, u16 reg)
{

	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);
	int value = 0;
	int ret;
	u8 page = reg / 128;

	mutex_lock(&aic325x->io_lock);
	reg = reg % 128;

	if (aic325x->page_no != page) {
		ret = aic325x_change_page(codec, page);
		if (ret < 0) {
			printk(KERN_ERR "%s: change_page failed for page= %d\n",
								__func__, page);
			mutex_unlock(&aic325x->io_lock);
			return -EIO;
		}
		/* Change Page successful. Update the Global Pointer */
		aic325x->page_no = page;
	}
	value = codec->hw_read(codec, reg);

	mutex_unlock(&aic325x->io_lock);
	return value;

}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_get_divs
 * Purpose  : This function is to get required divisor from the "aic325x_divs"
 *            table.
 *----------------------------------------------------------------------------
 */
static inline int aic325x_get_divs(int mclk, int rate)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(aic325x_divs); i++) {
		if ((aic325x_divs[i].rate == rate)
			&& (aic325x_divs[i].mclk == mclk)) {
			return i;
		}
	}

	printk(KERN_INFO
	"Master clock and sample rate is not supported,mclk = %d rate = %d\n",
				mclk, rate);
	return -EINVAL;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_add_widgets
 * Purpose  : This function is to add the dapm widgets. This routine will be
 *	      invoked during the Audio Driver Initialization.
 *            The following are the main widgets supported :
 *                # Left DAC to Left Outputs
 *                # Right DAC to Right Outputs
 *		  # Left Inputs to Left ADC
 *		  # Right Inputs to Right ADC
 *----------------------------------------------------------------------------
 */
static int aic325x_add_widgets(struct snd_soc_codec *codec)
{
	int i, ret;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_dapm_new_controls(dapm, aic325x_dapm_widgets,
				ARRAY_SIZE(aic325x_dapm_widgets));
	
	ret = snd_soc_dapm_add_routes(dapm, aic325x_dapm_routes,
				ARRAY_SIZE(aic325x_dapm_routes));
	if (!ret)
		dprintk("#Completed adding DAPM routes = %d\n",
			ARRAY_SIZE(aic325x_dapm_routes));

	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_hw_params
 * Purpose  : This function is to set the hardware parameters for AIC325x.
 *            The functions set the sample rate and audio serial data word
 *            length.
 *----------------------------------------------------------------------------
 */
static int aic325x_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
					struct snd_soc_dai *dai)
{

	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;

	int i, j, value;
	u8 data;
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);

	        /* Setting the playback status */
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
                aic325x->playback_stream = 1;
        else if ((substream->stream != SNDRV_PCM_STREAM_PLAYBACK) && \
                                                        (codec->active < 2))
                aic325x->playback_stream = 0;

        if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
                aic325x->record_stream = 1;
        else if ((substream->stream != SNDRV_PCM_STREAM_CAPTURE)  && \
                                                        (codec->active < 2))
                aic325x->record_stream = 0;


	dprintk(KERN_INFO "Function : %s, %d\n", __func__, aic325x->sysclk);

	
	codec->dapm.bias_level = 2;

	i = aic325x_get_divs(aic325x->sysclk, params_rate(params));
	if (i < 0) {
		printk(KERN_ERR "#%s: Sampling rate %d not supported\n",
			__func__, params_rate(params));
		return i;
	}
//	printk(KERN_ERR "#%s: Sampling rate %d not supported\n",
//			__func__, params_rate(params));
	if (soc_static_freq_config && codec->active < 2) {
		/* We will fix R to 1 and will make P & J=K.D as varialble */

		/* Setting P & R values */
		snd_soc_update_bits(codec, CLK_REG_2, PLL_P_DIV_MASK,
			((aic325x_divs[i].p_val << 4) | 0x01));

		/* J value */
		snd_soc_update_bits(codec, CLK_REG_3, PLL_J_DIV_MASK,
					aic325x_divs[i].pll_j);

		/* MSB & LSB for D value */
		snd_soc_update_bits(codec, CLK_REG_4, PLL_D_MSB_DIV_MASK, 
				(aic325x_divs[i].pll_d >> 8));

		snd_soc_update_bits(codec, CLK_REG_5, PLL_D_LSB_DIV_MASK,
			(aic325x_divs[i].pll_d & AIC325x_8BITS_MASK));
	
		/* NDAC divider value */
		snd_soc_update_bits(codec, NDAC_CLK_REG_6, PLL_NDAC_DIV_MASK,
					 aic325x_divs[i].ndac);
	
		/* MDAC divider value */
		snd_soc_update_bits(codec, MDAC_CLK_REG_7, PLL_MDAC_DIV_MASK, 
					aic325x_divs[i].mdac);

		/* DOSR MSB & LSB values */
		snd_soc_update_bits(codec, DAC_OSR_MSB, PLL_DOSR_MSB_MASK, 
					aic325x_divs[i].dosr >> 8);

		snd_soc_update_bits(codec, DAC_OSR_LSB, PLL_DOSR_LSB_MASK,
			aic325x_divs[i].dosr & AIC325x_8BITS_MASK);

		/* NADC divider value */
		snd_soc_update_bits(codec, NADC_CLK_REG_8, PLL_NADC_DIV_MASK, 
					aic325x_divs[i].nadc);

		/* MADC divider value */
		snd_soc_update_bits(codec, MADC_CLK_REG_9, PLL_MADC_DIV_MASK, 
					aic325x_divs[i].madc);

		/* AOSR value */
		snd_soc_update_bits(codec, ADC_OSR_REG, PLL_AOSR_DIV_MASK, 
					aic325x_divs[i].aosr);
	}
	/* BCLK N divider */
	snd_soc_update_bits(codec, CLK_REG_11, PLL_BCLK_DIV_MASK, \
				aic325x_divs[i].blck_N);

	data = snd_soc_read(codec, INTERFACE_SET_REG_1);

	data = data & ~(3 << 4);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		data |= (AIC325x_WORD_LEN_20BITS << DAC_OSR_MSB_SHIFT);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		data |= (AIC325x_WORD_LEN_24BITS << DAC_OSR_MSB_SHIFT);
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		data |= (AIC325x_WORD_LEN_32BITS << DAC_OSR_MSB_SHIFT);
		break;
	}

	snd_soc_update_bits(codec, INTERFACE_SET_REG_1,
				INTERFACE_REG_MASK, data);

	for (j = 0; j < NO_FEATURE_REGS; j++) {

		snd_soc_write(codec,
			aic325x_divs[i].codec_specific_regs[j].reg_offset,
				aic325x_divs[i].codec_specific_regs[j].reg_val);
	}
	return 0;
}


/*
 *-----------------------------------------------------------------------
 * Function : aic325x_dump_regs
 * Purpose  : This function is to dump the Page 0 and 1 Registers
 *-----------------------------------------------------------------------
 */
static void aic325x_dump_regs(struct snd_soc_codec *codec)
{
	int counter;
	u8 value;

	//aic325x_change_page (codec, 0);
	printk(KERN_INFO "#%s: Page 0 Dump::::::::::::\n\n", __func__);
	for (counter = 0; counter < 127; counter++) {
		value = aic325x_read (codec, counter);
		printk (KERN_INFO "#%d -> 0x%x\n", (counter % 128), value);
	}
	//aic325x_change_page (codec, 1);
	printk(KERN_INFO "#%s: Page 1 Dump::::::::::::\n\n", __func__);
	for (counter = 128; counter < 255; counter++) {
		value = aic325x_read (codec, counter);
		printk (KERN_INFO "#%d -> 0x%x\n", (counter % 128), value);
	}
	//aic325x_change_page (codec, 0);
	return 0;
}

/*
 *-----------------------------------------------------------------------
 * Function : aic325x_mute
 * Purpose  : This function is to mute or unmute the left and right DAC
 *-----------------------------------------------------------------------
 */
static int aic325x_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	u8 dac_reg, adc_reg;
	dprintk(KERN_INFO "Function : %s\n", __func__);
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);

	if (mute) {
		if(codec->active !=0)  {
			if ((aic325x->playback_stream == 1) && \
					(aic325x->record_stream ==1)) {
				printk(KERN_INFO "Session is still going on\n");
				return 0;
			}
		} 
		if (aic325x->playback_stream) {
			snd_soc_update_bits(codec, DAC_MUTE_CTRL_REG, 
					DAC_MUTE_MASK, DAC_MUTE_ON);
		}
		if (aic325x->record_stream) {
			
			snd_soc_update_bits(codec, ADC_FGA, ADC_MUTE_MASK, 
							ADC_MUTE_ON);
		}
		
	} else {
		if (aic325x->playback_stream) {
			snd_soc_update_bits(codec, DAC_MUTE_CTRL_REG, 
					DAC_MUTE_MASK, (~DAC_MUTE_ON));
		}
		if (aic325x->record_stream) {
			
		    snd_soc_update_bits(codec, ADC_FGA, ADC_MUTE_MASK, 
					(~ ADC_MUTE_ON));
		}

		// aic325x_dump_regs(codec); 
	}
	dprintk(KERN_INFO "Function : %s Exiting\n", __func__);

	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_set_dai_sysclk
 * Purpose  : This function is to set the DAI system clock
 *----------------------------------------------------------------------------
 */
static int aic325x_set_dai_sysclk(struct snd_soc_dai *codec_dai,
					int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);

	dprintk(KERN_INFO "Function : %s, %d\n", __func__, freq);

	switch (freq) {
	case AIC325x_FREQ_12000000:
	case AIC325x_FREQ_12288000:
	case AIC325x_FREQ_11289600:
	case AIC325x_FREQ_24000000:
	case AIC325x_FREQ_19200000:
	case AIC325x_FREQ_38400000:
		aic325x->sysclk = freq;
		return 0;
	}

	printk(KERN_INFO "Invalid frequency to set DAI system clock\n");

	return -EINVAL;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_set_dai_fmt
 * Purpose  : This function is to set the DAI format
 *----------------------------------------------------------------------------
 */
static int aic325x_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	u8 iface_reg;
	u8 iface_reg1;
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);

	iface_reg = snd_soc_read(codec, INTERFACE_SET_REG_1);
	iface_reg = iface_reg & ~(3 << 6 | 3 << 2);
	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		aic325x->master = 1;
		iface_reg |= BIT_CLK_MASTER | WORD_CLK_MASTER;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		aic325x->master = 0;
		iface_reg1 &= ~(BIT_CLK_MASTER | WORD_CLK_MASTER);
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		aic325x->master = 0;
		iface_reg1 |= BIT_CLK_MASTER;
		iface_reg1 &= ~(WORD_CLK_MASTER);
		break;
	default:
		printk(KERN_INFO "Invalid DAI master/slave interface\n");
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		break;
	case SND_SOC_DAIFMT_DSP_A:
		iface_reg |= (AIC325x_DSP_MODE << CLK_REG_3_SHIFT);
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		iface_reg |= (AIC325x_RIGHT_JUSTIFIED_MODE << CLK_REG_3_SHIFT);
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iface_reg |= (AIC325x_LEFT_JUSTIFIED_MODE << CLK_REG_3_SHIFT);
		break;
	default:
		printk(KERN_INFO "Invalid DAI interface format\n");
		return -EINVAL;
	}

	snd_soc_update_bits(codec, INTERFACE_SET_REG_1, INTERFACE_REG_MASK,\
							iface_reg);
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_set_bias_level
 * Purpose  : This function is to get triggered when dapm events occurs.
 *----------------------------------------------------------------------------
 */
static int aic325x_set_bias_level(struct snd_soc_codec *codec,
					enum snd_soc_bias_level level)
{
	u8 value ;
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);

	dprintk(KERN_INFO "Function : %s\n", __func__);
	switch (level) {
		/* full On */
	case SND_SOC_BIAS_ON:

		dprintk(KERN_INFO "Function : %s: BIAS ON\n", __func__);
		/* all power is driven by DAPM system */
		break;

		/* partial On */
	case SND_SOC_BIAS_PREPARE:

		dprintk(KERN_INFO "Function : %s: BIAS PREPARE\n", __func__);
	
		snd_soc_update_bits(codec, AIS_REG_3,
		BCLK_WCLK_BUFFER_POWER_CONTROL_MASK, BCLK_WCLK_BUFFER_ON);

		break;

		/* Off, with power */
	case SND_SOC_BIAS_STANDBY:
		dprintk(KERN_INFO "Function : %s: BIAS STANDBY\n", __func__);


		snd_soc_update_bits(codec, AIS_REG_3,
				BCLK_WCLK_BUFFER_POWER_CONTROL_MASK, 0);

		//snd_soc_update_bits(codec, //REF_PWR_UP_CONF_REG, REF_PWR_UP_MASK,
		//			FORCED_REF_PWR_UP);

		/*
		 * all power is driven by DAPM system,
		 * so output power is safe if bypass was set
		 */

		if (codec->dapm.bias_level == SND_SOC_BIAS_OFF) {

			snd_soc_update_bits(codec, POW_CFG, 
				AVDD_CONNECTED_TO_DVDD_MASK,
				DISABLE_AVDD_TO_DVDD);

			snd_soc_update_bits(codec, PWR_CTRL_REG, 
					ANALOG_BLOCK_POWER_CONTROL_MASK, 
					ENABLE_ANALOG_BLOCK);

		}
		break;

		/* Off, without power */
	case SND_SOC_BIAS_OFF:
		dprintk(KERN_INFO "Function : %s: BIAS OFF\n", __func__);
		//snd_soc_update_bits(codec, REF_PWR_UP_CONF_REG,
		//			REF_PWR_UP_MASK, AUTO_REF_PWR_UP);
		snd_soc_update_bits(codec, PWR_CTRL_REG,
			ANALOG_BLOCK_POWER_CONTROL_MASK, DISABLE_ANALOG_BLOCK);

		snd_soc_update_bits(codec, POW_CFG,
			AVDD_CONNECTED_TO_DVDD_MASK, ENABLE_AVDD_TO_DVDD);

	/* force all power off */
		break;
	}

	codec->dapm.bias_level = level;

	return 0;
}

/*
*----------------------------------------------------------------------------
* Function : aic325x_suspend
* Purpose  : This function is to suspend the AIC325x driver.
*----------------------------------------------------------------------------
*/

static int aic325x_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
#ifndef HP_PA_CTL
	//snd_soc_update_bits(codec, 0x89, 0x30, 0x00);
#endif
	dprintk(KERN_INFO "Function : %s\n", __func__);
	aic325x_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_resume
 * Purpose  : This function is to resume the AIC325x driver
 *----------------------------------------------------------------------------
 */

static int aic325x_resume(struct snd_soc_codec *codec)
{
	dprintk(KERN_INFO "Function : %s\n", __func__);
	aic325x_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
#ifndef HP_PA_CTL
	if(has_hs)
	{
		//snd_soc_update_bits(codec, 0x89, 0x30, 0x03);
	}
#endif
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_init
 * Purpose  : This function is to initialise the AIC325x driver
 *            register the mixer and dsp interfaces with the kernel.
 *
 *----------------------------------------------------------------------------
 */

static int tlv320aic325x_init(struct snd_soc_codec *codec)
{
	int ret = 0,status;
	int i = 0;
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);
	aic325x->page_no = 0;

	dprintk(KERN_INFO "Function : %s\n", __func__);

	/* codec register initalization*/

	//printk("aic325x sw reset\n");
	/* Soft Reset */	
	//snd_soc_write (codec, RESET, 0x01);
	//mdelay(100);

	printk("aic325x reg init\n");
	if(aic325x->pdata->hw_detect)
	{
		status = aic325x->pdata->hw_detect();
		printk("aic325x rec status = %d\n",status);
		if(status == HW_VER1){
			for (i = 0;
		     i < sizeof(aic325x_reg_init) / sizeof(struct aic325x_configs);
		     i++) {
			 if(DELAY_REG == aic325x_reg_init[i].reg_offset) {
				msleep(aic325x_reg_init[i].reg_val);
			 }else{
				snd_soc_write(codec, aic325x_reg_init[i].reg_offset,
					aic325x_reg_init[i].reg_val);
			 }
			}
		}else if( status == HW_VER0){
			for (i = 0;
		     i < sizeof(aic325x_reg_init_rec) / sizeof(struct aic325x_configs);
		     i++) {
			 if(DELAY_REG == aic325x_reg_init_rec[i].reg_offset) {
				msleep(aic325x_reg_init_rec[i].reg_val);
			 }else{
				snd_soc_write(codec, aic325x_reg_init_rec[i].reg_offset,
					aic325x_reg_init_rec[i].reg_val);
			 }
			}

		}


	}else {
		for (i = 0;
		     i < sizeof(aic325x_reg_init_rec) / sizeof(struct aic325x_configs);
		     i++) {
			 if(DELAY_REG == aic325x_reg_init_rec[i].reg_offset) {
				msleep(aic325x_reg_init_rec[i].reg_val);
			 }else{
				snd_soc_write(codec, aic325x_reg_init_rec[i].reg_offset,
					aic325x_reg_init_rec[i].reg_val);
			 }
		}
	}
	snd_soc_add_codec_controls(codec, aic325x_snd_controls1,
				ARRAY_SIZE(aic325x_snd_controls1));
	snd_soc_add_codec_controls(codec, aic325x_snd_controls2,
				ARRAY_SIZE(aic325x_snd_controls2));
	snd_soc_add_codec_controls(codec, aic325x_snd_controls3,
				ARRAY_SIZE(aic325x_snd_controls3));
	aic325x_add_widgets(codec);
	
#ifdef CONFIG_MINI_DSP
	aic325x_minidsp_program(codec);
	aic325x_add_minidsp_controls(codec);
#endif

#ifdef AIC325x_TiLoad
	aic325x_driver_init(codec);
#endif


	return ret;

}
int aic325x_hp(struct snd_soc_codec *codec,int on_off)
{
#if 0
	int i;	
	for (i = 0;
	     i < sizeof(aic325x_reg_mic) / sizeof(struct aic325x_configs);
	     i++) {
	snd_soc_write(codec, aic325x_reg_mic[i].reg_offset,
				aic325x_reg_mic[i].reg_val);
	}
	msleep(100);
#else
	//if(on_off)	
	//	snd_soc_write(codec,HEADSET_DETECT ,0x89);
	//else
	//	snd_soc_write(codec,HEADSET_DETECT,0x09);
#endif		
}

EXPORT_SYMBOL(aic325x_hp);
/*
* aic3262_jack_handler
*
* This function is called from the Interrupt Handler
* to check the status of the AIC3262 Registers related to Headset Detection
*/
#define AIC325x_NO_JACK		BIT(0)
#define AIC325x_HEADSET_DET	BIT(1)
#define AIC325x_HEADPHO_DET	BIT(2)
int aic325x_headset_detect(struct snd_soc_codec *codec, int jack_insert)
{
	int jack_type;
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);

	unsigned int value;
	unsigned int micbits, hsbits = 0;

	//aic325x_change_page(codec, 0);
	printk("enter %s\n",__func__);
	//msleep(100);
	//snd_soc_write(codec, HEADSET_DETECT, 0x89);
	//msleep(300);
	/* Read the Jack Status Register*/
	value = snd_soc_read(codec, STICKY_FLAG1);
	
	hsbits = value & FLAG_HS_MASKBITS;
	
	/* No Headphone or Headset*/
	if (!hsbits) {
		//printk(KERN_INFO "no headset/headphone\n");
		//snd_soc_jack_report(aic325x->headset_jack,
		///		0, SND_JACK_HEADSET);
		jack_type = AIC325x_NO_JACK;
		has_hs = 0;
#ifndef HP_PA_CTL
		snd_soc_update_bits(codec, 0x89, 0x30, 0x00);
#endif
	}

	/* Headphone Detected */
	if (hsbits) {
		printk(KERN_INFO "headphone\n");
		//snd_soc_jack_report(aic325x->headset_jack,
		//		SND_JACK_HEADPHONE, SND_JACK_HEADSET);
		//jack_type = RT5631_HEADPHO_DET;
		jack_type = AIC325x_HEADSET_DET;	
		has_hs = 1;
#ifndef HP_PA_CTL
		snd_soc_update_bits(codec, 0x89, 0x30, 0x03);
#endif
	}

	printk(KERN_INFO "%s--: HS status = %x\n", __func__,
			snd_soc_read(codec, STATUS_FLAG1));
	return jack_type;
}
EXPORT_SYMBOL(aic325x_headset_detect);
static irqreturn_t aic325x_jack_handler(int irq, void *data)
{
	struct snd_soc_codec *codec = data;
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);
	unsigned int value;
	unsigned int micbits, hsbits = 0;

	//aic325x_change_page(codec, 0);
	printk("enter %s\n",__func__);
	/* Read the Jack Status Register*/
	value = snd_soc_read(codec, STICKY_FLAG1);
	
	hsbits = value & FLAG_HS_MASKBITS;
	
	
	/* No Headphone or Headset*/
	if (!hsbits) {
		printk(KERN_INFO "no headset/headphone\n");
		snd_soc_jack_report(aic325x->headset_jack,
				0, SND_JACK_HEADSET);
	}

	/* Headphone Detected */
	if (hsbits) {
		printk(KERN_INFO "headphone\n");
		snd_soc_jack_report(aic325x->headset_jack,
				SND_JACK_HEADPHONE, SND_JACK_HEADSET);
	}

	printk(KERN_INFO "%s--: HS status = %x\n", __func__,
			snd_soc_read(codec, STATUS_FLAG1));
	return IRQ_HANDLED;
}



static int
aic325x_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	struct i2c_adapter *adapter;
	struct aic325x_priv *aic325x = snd_soc_codec_get_drvdata(codec);
#if 0


	if (gpio_is_valid(aic325x->pdata->gpio_aic325x_reset)) {
		ret = gpio_request(aic325x->pdata->gpio_aic325x_reset, "reset_en");
		if (ret) {
			printk(KERN_INFO, "cannot get reset_en gpio\n");
			return ret;
		}
		aic325x->gpio_requested |= GPIO_RESET_EN;

		/* Enable reset; enable signal is active-low */
		printk("aic325x hw reset\n");
		gpio_direction_output(aic325x->pdata->gpio_aic325x_reset, 0);
		mdelay(50);
		gpio_direction_output(aic325x->pdata->gpio_aic325x_reset, 1);
		mdelay(100);
		printk("aic325x hw reset done\n");
	}
#endif
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	codec->hw_write = (hw_write_t) aic325x_hw_write;
	codec->hw_read = aic325x_hw_read;
	codec->control_data = aic325x->control_data;
#else
	/* Add other interfaces here */
#endif
/*	if (aic325x->irq) {
	// audio interrupt 
	ret = request_threaded_irq(aic325x->irq, NULL,
			aic325x_jack_handler,
			IRQF_TRIGGER_FALLING,
			"tlv320aic325x", codec);
		if (ret) {
			printk(KERN_INFO "Failed to request IRQ: %d\n", ret);
			return ret;
		} else
			printk(KERN_INFO
			"#%s: irq Registration for IRQ %d done..\n",
				__func__, aic325x->irq);
	} else {
		printk(KERN_INFO "#%s: I2C IRQ Configuration is Wrong. \
			Please check it..\n", __func__);
	}
*/
	tlv320aic325x_init(codec);
	
	return ret;

}

/*
 *----------------------------------------------------------------------------
 * Function : aic325x_remove
 * Purpose  : to remove aic325x soc device
 *
 *----------------------------------------------------------------------------
 */

static int
aic325x_remove(struct snd_soc_codec *codec)
{
	/* power down chip */
	if (codec->control_data)
		aic325x_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}



static struct snd_soc_codec_driver soc_codec_driver_aic325x = {
	.probe = aic325x_probe,
	.remove = aic325x_remove,
	.suspend = aic325x_suspend,
	.resume = aic325x_resume,
	.read = aic325x_read,
	.write = aic325x_write,
	.set_bias_level = aic325x_set_bias_level,
	.reg_cache_size = ARRAY_SIZE(aic325x_reg),
	.reg_word_size = sizeof(u8),
	.reg_cache_default = aic325x_reg,
};




static int aic325x_i2c_probe(struct i2c_client *pdev,
				const struct i2c_device_id *id)
{
	int ret,status = 0;

	struct aic325x_priv *aic325x;
	struct snd_soc_codec *codec;

	printk(KERN_INFO "AIC325x Audio Codec %s\n", AIC325x_VERSION);

	aic325x = kzalloc(sizeof(struct aic325x_priv), GFP_KERNEL);
	if (aic325x == NULL)
		return -ENOMEM;

	mutex_init(&aic325x->io_lock);

	i2c_set_clientdata(pdev, aic325x);
	aic325x->control_data = pdev;
	aic325x->irq = pdev->irq;
	aic325x->pdata = pdev->dev.platform_data;

        if (gpio_is_valid(aic325x->pdata->gpio_aic325x_reset)) {
                ret = gpio_request(aic325x->pdata->gpio_aic325x_reset, "reset_en");
                if (ret) {
                        printk(KERN_INFO, "cannot get reset_en gpio\n");
                        return ret;
                }
                aic325x->gpio_requested |= GPIO_RESET_EN;

                /* Enable reset; enable signal is active-low */
                printk("aic325x hw reset\n");
                gpio_direction_output(aic325x->pdata->gpio_aic325x_reset, 0);
                mdelay(50);
                gpio_direction_output(aic325x->pdata->gpio_aic325x_reset, 1);
                mdelay(100);
                printk("aic325x hw reset done\n");
        }

#if  0
	if (gpio_is_valid(aic325x->pdata->gpio_spkr_en)) {
		ret = gpio_request(aic325x->pdata->gpio_spkr_en, "spkr_en");
		if (ret) {
			printk(KERN_INFO, "cannot get spkr_en gpio\n");
			return ret;
		}
		aic325x->gpio_requested |= GPIO_SPKR_EN;
		gpio_direction_output(aic325x->pdata->gpio_spkr_en, 0);
	}
#endif
	ret = snd_soc_register_codec(&pdev->dev,
			&soc_codec_driver_aic325x, tlv320aic325x_dai_driver,
			ARRAY_SIZE(tlv320aic325x_dai_driver));
	if (ret)
		printk(KERN_ERR "codec: %s : snd_soc_register_codec failed\n",
						__func__);
	else
		printk(KERN_ALERT "codec: %s : snd_soc_register_codec success\n",
						 __func__);
	return ret;

}

static int aic325x_i2c_remove(struct i2c_client *pdev)
{

	snd_soc_unregister_codec(&pdev->dev);
	
	return 0;
}
static const struct i2c_device_id aic3256_i2c_id[] = {
	{ "tlv320aic325x", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, aic325x_i2c_id);

static struct i2c_driver aic325x_i2c_driver = {
	.driver = {
		.name = "tlv320aic325x",
		.owner = THIS_MODULE,
	},
	.probe = aic325x_i2c_probe,
	.remove = aic325x_i2c_remove,
	.id_table = aic3256_i2c_id,
};

/*
 *----------------------------------------------------------------------------
 * Function : tlv320aic3256_modinit
 * Purpose  : module init function. First function to run.
 *
 *----------------------------------------------------------------------------
 */
static int __init tlv320aic325x_modinit(void)
{
	int ret = 0;

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	 ret = i2c_add_driver(&aic325x_i2c_driver);

	if (ret != 0)
		printk(KERN_ERR
		"Failed to register TLV320AIC325x I2C driver: %d\n", ret);

#endif
	return ret;
}
module_init(tlv320aic325x_modinit);

/*
 *----------------------------------------------------------------------------
 * Function : tlv320aic3256_exit
 * Purpose  : module init function. First function to run.
 *
 *----------------------------------------------------------------------------
 */
static void __exit tlv320aic325x_exit(void)
{
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
		i2c_del_driver(&aic325x_i2c_driver);
#endif
}

module_exit(tlv320aic325x_exit);


MODULE_DESCRIPTION("ASoC TLV320AIC325x codec driver");
MODULE_AUTHOR("Nisar Khan <nisar.aka@mistralsolutions.com>");
MODULE_AUTHOR("Shubhro Mitra <shubhro.mitra@mistralsolutions.com>");
MODULE_LICENSE("GPL");
