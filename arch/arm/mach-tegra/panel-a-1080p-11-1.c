/*
 * arch/arm/mach-tegra/panel-a-1080p-11-6.c
 *
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mach/dc.h>
#include <mach/iomap.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/tegra_pwm_bl.h>
#include <linux/regulator/consumer.h>
#include <linux/pwm_backlight.h>
#include <linux/max8831_backlight.h>
#include <linux/leds.h>
#include <linux/ioport.h>
#include "board.h"
#include "board-panel.h"
#include "devices.h"
#include "gpio-names.h"
#include "tegra11_host1x_devices.h"

#define TEGRA_DSI_GANGED_MODE	0

#define DSI_PANEL_RESET		1
#define DSI_PANEL_BL_PWM	TEGRA_GPIO_PH1


#define DSI_MIPI_RESET		0
#define DSI_PANEL_LCD_EN_GPIO	TEGRA_GPIO_PH5 // 3.3V
#define DSI_PANEL_BL_EN_GPIO TEGRA_GPIO_PN4
#define DSI_PANEL_BL_PWM	TEGRA_GPIO_PH1
#define DSI_VDD_BL_EN_GPIO TEGRA_GPIO_PH0
#define DSI_LCD_BL_EN_GPIO TEGRA_GPIO_PO0
#define DSI_PANEL_BL_VDD_GPIO TEGRA_GPIO_PH2
#define SN65DSI83_MIPI_EN TEGRA_GPIO_PP2

#define DC_CTRL_MODE	TEGRA_DC_OUT_CONTINUOUS_MODE

//static atomic_t tegra_release_bootloader_fb_flag = ATOMIC_INIT(0);
//static atomic_t dsi2lvds_enabled = ATOMIC_INIT(0);

static bool reg_requested;
static bool gpio_requested;
static struct platform_device *disp_device;
static struct regulator *avdd_lcd_3v3;
static struct regulator *vdd_lcd_bl;
static struct regulator *vdd_lcd_bl_en;
static struct regulator *dvdd_lcd_1v8;
static struct regulator *vdd_ds_1v8;

static int gpio_lcd_bl_en;
static int gpio_vdd_bl_en;
static int gpio_vdd_bl;
static int gpio_bl_en;
static int gpio_lcd_rst;
static int gpio_bridge_mip_en;



static tegra_dc_bl_output dsi_a_1080p_11_1_bl_output_measured = {
	0, 0, 1, 2, 3, 4, 5, 6,
	7, 8, 9, 9, 10, 11, 12, 13,
	13, 14, 15, 16, 17, 17, 18, 19,
	20, 21, 22, 22, 23, 24, 25, 26,
	27, 27, 28, 29, 30, 31, 32, 32,
	33, 34, 35, 36, 37, 37, 38, 39,
	40, 41, 42, 42, 43, 44, 45, 46,
	47, 48, 48, 49, 50, 51, 52, 53,
	54, 55, 56, 57, 57, 58, 59, 60,
	61, 62, 63, 64, 65, 66, 67, 68,
	69, 70, 71, 71, 72, 73, 74, 75,
	76, 77, 77, 78, 79, 80, 81, 82,
	83, 84, 85, 87, 88, 89, 90, 91,
	92, 93, 94, 95, 96, 97, 98, 99,
	100, 101, 102, 103, 104, 105, 106, 107,
	108, 109, 110, 111, 112, 113, 115, 116,
	117, 118, 119, 120, 121, 122, 123, 124,
	125, 126, 127, 128, 129, 130, 131, 132,
	133, 134, 135, 136, 137, 138, 139, 141,
	142, 143, 144, 146, 147, 148, 149, 151,
	152, 153, 154, 155, 156, 157, 158, 158,
	159, 160, 161, 162, 163, 165, 166, 167,
	168, 169, 170, 171, 172, 173, 174, 176,
	177, 178, 179, 180, 182, 183, 184, 185,
	186, 187, 188, 189, 190, 191, 192, 194,
	195, 196, 197, 198, 199, 200, 201, 202,
	203, 204, 205, 206, 207, 208, 209, 210,
	211, 212, 213, 214, 215, 216, 217, 219,
	220, 221, 222, 224, 225, 226, 227, 229,
	230, 231, 232, 233, 234, 235, 236, 238,
	239, 240, 241, 242, 243, 244, 245, 246,
	247, 248, 249, 250, 251, 252, 253, 255
};

static struct tegra_dsi_cmd dsi_a_1080p_11_1_init_cmd[] = {
	/* no init command required */
};

static struct tegra_dsi_out dsi_a_1080p_11_1_pdata = {
#ifdef CONFIG_ARCH_TEGRA_3x_SOC
	.n_data_lanes = 2,
	.controller_vs = DSI_VS_0,
#else
	.controller_vs = DSI_VS_1,
#endif
	.dsi2edp_bridge_enable = true,
	.n_data_lanes = 4,
	.video_burst_mode = TEGRA_DSI_VIDEO_NONE_BURST_MODE,

	.pixel_format = TEGRA_DSI_PIXEL_FORMAT_24BIT_P,
	.refresh_rate = 60,
	.virtual_channel = TEGRA_DSI_VIRTUAL_CHANNEL_0,

	.dsi_instance = DSI_INSTANCE_0,

	.panel_reset = DSI_PANEL_RESET,
	.power_saving_suspend = true,
	.video_data_type = TEGRA_DSI_VIDEO_TYPE_VIDEO_MODE,
	.video_clock_mode = TEGRA_DSI_VIDEO_CLOCK_CONTINUOUS,
	.dsi_init_cmd = dsi_a_1080p_11_1_init_cmd,
	.n_init_cmd = ARRAY_SIZE(dsi_a_1080p_11_1_init_cmd),
};

static int keenhi_dsi_regulator_get(struct device *dev)
{
	int err = 0;

	if (reg_requested)
		return 0;
	
	avdd_lcd_3v3 = regulator_get(dev, "avdd_lcd");
	if (IS_ERR_OR_NULL(avdd_lcd_3v3)) {
		pr_err("avdd_lcd regulator get failed\n");
		err = PTR_ERR(avdd_lcd_3v3);
		avdd_lcd_3v3 = NULL;
		goto fail;
	}

	reg_requested = true;
	return 0;
fail:
	return err;
}

static int keenhi_dsi_gpio_get(void)
{
	int err = 0;

	if (gpio_requested)
		return 0;

	gpio_lcd_rst = DSI_PANEL_LCD_EN_GPIO;
	err = gpio_request(gpio_lcd_rst, "panel_enable");
	if (err < 0) {
		pr_err("panel reset gpio request failed\n");
		goto fail;
	}
	//gpio_direction_output(gpio_lcd_rst, 0);//for test

	gpio_bridge_mip_en= SN65DSI83_MIPI_EN;
	err = gpio_request(gpio_bridge_mip_en, "mipi_enable");
	if (err < 0) {
		pr_err("panel reset gpio request failed\n");
		goto fail;
	}
	//gpio_direction_output(gpio_bridge_mip_en, 0);

	gpio_vdd_bl= DSI_PANEL_BL_VDD_GPIO;
	err = gpio_request(gpio_vdd_bl, "bl_vdd");
	if (err < 0) {
	    pr_err("panel reset gpio request failed\n");
	    gpio_vdd_bl = 0;
	    goto fail;
	}
	gpio_direction_output(DSI_PANEL_BL_VDD_GPIO, 1);

	gpio_bl_en = DSI_PANEL_BL_EN_GPIO;
	err = gpio_request(gpio_bl_en, "bl_enable");
	if (err < 0) {
		pr_err("panel reset gpio request failed\n");
		gpio_bl_en = 0;
		goto fail;
	}
	gpio_direction_output(gpio_bl_en, 1);

	gpio_vdd_bl_en = DSI_VDD_BL_EN_GPIO;
	err = gpio_request(gpio_vdd_bl_en, "bl_vdd_enable");
	if (err < 0) {
		pr_err("panel reset gpio request failed\n");
		gpio_vdd_bl_en = 0;
		goto fail;
	}
	gpio_direction_output(gpio_vdd_bl_en, 1);
	
	gpio_lcd_bl_en = DSI_LCD_BL_EN_GPIO;
	err = gpio_request(gpio_lcd_bl_en, "lcd_bl_enable");
	if (err < 0) {
		pr_err("panel reset gpio request failed\n");
		gpio_lcd_bl_en = 0;
		goto fail;
	}
	
	gpio_direction_output(gpio_lcd_bl_en, 1);
	
	/* free pwm GPIO */
	err = gpio_request(DSI_PANEL_BL_PWM, "panel_pwm");
	if (err < 0) {
		pr_err("panel pwm gpio request failed\n");
		goto fail;
	}
	gpio_free(DSI_PANEL_BL_PWM);
	gpio_requested = true;
	return 0;
fail:
	return err;
}

static int dsi_a_1080p_11_1_enable(struct device *dev)
{
	int err = 0;
	pr_err("%s:=============>\n",__func__);
//	if(!atomic_read(&tegra_release_bootloader_fb_flag)) {
//		tegra_release_bootloader_fb();
//		atomic_set(&tegra_release_bootloader_fb_flag, 1);
//	}
	err = keenhi_dsi_regulator_get(dev);
	if (err < 0) {
		pr_err("dsi regulator get failed\n");
		goto fail;
	}
	err = keenhi_dsi_gpio_get();
	if (err < 0) {
		pr_err("dsi gpio request failed\n");
		goto fail;
	}
//	if(atomic_read(&dsi2lvds_enabled))return 0;
	if (vdd_ds_1v8) {
		err = regulator_enable(vdd_ds_1v8);
		if (err < 0) {
			pr_err("vdd_ds_1v8 regulator enable failed\n");
			goto fail;
		}
	}

	if (dvdd_lcd_1v8) {
		err = regulator_enable(dvdd_lcd_1v8);
		if (err < 0) {
			pr_err("dvdd_lcd regulator enable failed\n");
			goto fail;
		}
	}

	if (avdd_lcd_3v3) {
		err = regulator_enable(avdd_lcd_3v3);
		if (err < 0) {
			pr_err("avdd_lcd regulator enable failed\n");
			goto fail;
		}
	}

	if (vdd_lcd_bl) {
		err = regulator_enable(vdd_lcd_bl);
		if (err < 0) {
			pr_err("vdd_lcd_bl regulator enable failed\n");
			goto fail;
		}
	}

	if (vdd_lcd_bl_en) {
		err = regulator_enable(vdd_lcd_bl_en);
		if (err < 0) {
			pr_err("vdd_lcd_bl_en regulator enable failed\n");
			goto fail;
		}
	}
	if(gpio_lcd_rst>0){
		gpio_set_value(gpio_lcd_rst, 1);
	}
	msleep(100);
	
	
	if(gpio_vdd_bl_en>0){
		gpio_set_value(gpio_vdd_bl_en, 1);
	}

	if(gpio_vdd_bl>0){
	    gpio_set_value(gpio_vdd_bl, 1);
	}

	if(gpio_bl_en>0){
		gpio_set_value(gpio_bl_en, 1);
	}

	if(gpio_lcd_bl_en>0){
		gpio_set_value(gpio_lcd_bl_en, 1);
	}
	
	gpio_set_value(gpio_bridge_mip_en, 1);
	msleep(50);
	gpio_set_value(gpio_bridge_mip_en, 0);
	msleep(50);
	gpio_set_value(gpio_bridge_mip_en, 1);
	msleep(50);
	
#if DSI_PANEL_RESET
	gpio_direction_output(DSI_PANEL_LCD_EN_GPIO, 1);
	usleep_range(1000, 5000);
	gpio_set_value(DSI_PANEL_LCD_EN_GPIO, 0);
	msleep(150);
	gpio_set_value(DSI_PANEL_LCD_EN_GPIO, 1);
	msleep(1500);
#endif
//	atomic_set(&dsi2lvds_enabled, 1);
	return 0;
fail:
	return err;
}

static int dsi_a_1080p_11_1_disable(void)
{
pr_err("%s:=============>\n",__func__);
//	if(!atomic_read(&dsi2lvds_enabled))return 0; //prevent unbalanced disable
	if(gpio_lcd_bl_en>0){
		gpio_set_value(gpio_lcd_bl_en, 0);
	}

	if(gpio_vdd_bl_en>0){
		gpio_set_value(gpio_vdd_bl_en, 0);
	}

	if(gpio_bl_en>0){
		gpio_set_value(gpio_bl_en, 0);
	}

	if(gpio_vdd_bl>0){
	    gpio_set_value(gpio_vdd_bl, 0);
	}
	
	if (avdd_lcd_3v3)
		regulator_disable(avdd_lcd_3v3);
	
	if(gpio_lcd_rst>0){
		gpio_set_value(gpio_lcd_rst, 0);
	}

	if(gpio_bridge_mip_en>0){
		gpio_set_value(gpio_bridge_mip_en, 0);
	}
	
	if (vdd_lcd_bl)
		regulator_disable(vdd_lcd_bl);

	if (vdd_lcd_bl_en)
		regulator_disable(vdd_lcd_bl_en);

	if (dvdd_lcd_1v8)
		regulator_disable(dvdd_lcd_1v8);

	if (vdd_ds_1v8)
		regulator_disable(vdd_ds_1v8);
//	atomic_set(&dsi2lvds_enabled, 0);
	return 0;
}

static int dsi_a_1080p_11_1_postsuspend(void)
{
	return 0;
}

static struct tegra_dc_mode dsi_a_1080p_11_1_modes[] = {
	{//v =38,h = 188
#if 1
		.pclk = 69302970,//70700000,
		.h_ref_to_sync = 4,
		.v_ref_to_sync = 1,
		
		.v_sync_width = 12,
		.h_sync_width = 50,
		
		.v_back_porch = 12,
		.h_back_porch = 50,
		.v_active = 1920,
		.h_active = 1080,
		.v_front_porch = 14,
		.h_front_porch = 88,
#else
		.pclk = 144250000,
		.h_ref_to_sync = 4,
		.v_ref_to_sync = 1,
		.h_sync_width = 28,
		.v_sync_width = 5,
		.h_back_porch = 148,
		.v_back_porch = 23,
		.h_active = 1920,
		.v_active = 1080,
		.h_front_porch = 66,
		.v_front_porch = 4,
#endif
	},

};

static int dsi_a_1080p_11_1_bl_notify(struct device *unused, int brightness)
{
	int cur_sd_brightness = atomic_read(&sd_brightness);

	/* SD brightness is a percentage */
	brightness = (brightness * cur_sd_brightness) / 255;

	/* Apply any backlight response curve */
	if (brightness > 255)
		pr_info("Error: Brightness > 255!\n");
	else
		brightness = dsi_a_1080p_11_1_bl_output_measured[brightness];

	return brightness;
}

static int dsi_a_1080p_11_1_check_fb(struct device *dev, struct fb_info *info)
{
	return info->device == &disp_device->dev;
}

static struct platform_pwm_backlight_data dsi_a_1080p_11_1_bl_data = {
	.pwm_id		= 1,
	.max_brightness	= 255,
	.dft_brightness	= 224,
	.pwm_period_ns	= 1000000,
	.notify		= dsi_a_1080p_11_1_bl_notify,
	/* Only toggle backlight on fb blank notifications for disp1 */
	.check_fb	= dsi_a_1080p_11_1_check_fb,
};

static struct platform_device __maybe_unused
		dsi_a_1080p_11_1_bl_device = {
	.name	= "pwm-backlight",
	.id	= -1,
	.dev	= {
		.platform_data = &dsi_a_1080p_11_1_bl_data,
	},
};

static struct platform_device __maybe_unused
			*dsi_a_1080p_11_1_bl_devices[] __initdata = {
	&tegra_pwfm1_device,
	&dsi_a_1080p_11_1_bl_device,
};

static int  __init dsi_a_1080p_11_1_register_bl_dev(void)
{
	int err = 0;
	err = platform_add_devices(dsi_a_1080p_11_1_bl_devices,
				ARRAY_SIZE(dsi_a_1080p_11_1_bl_devices));
	if (err) {
		pr_err("disp1 bl device registration failed");
		return err;
	}
	return err;
}

static void dsi_a_1080p_11_1_set_disp_device(
	struct platform_device *keenhi_display_device)
{
	disp_device = keenhi_display_device;
}

static void dsi_a_1080p_11_1_resources_init(struct resource *
resources, int n_resources)
{
	int i;
	for (i = 0; i < n_resources; i++) {
		struct resource *r = &resources[i];
		if (resource_type(r) == IORESOURCE_MEM &&
			!strcmp(r->name, "dsi_regs")) {
			r->start = TEGRA_DSI_BASE;
			r->end = TEGRA_DSI_BASE + TEGRA_DSI_SIZE - 1;
		}
	}
}

static void dsi_a_1080p_11_1_dc_out_init(struct tegra_dc_out *dc)
{
	dc->dsi = &dsi_a_1080p_11_1_pdata;
	dc->parent_clk = "pll_d_out0";
	dc->modes = dsi_a_1080p_11_1_modes;
	dc->n_modes = ARRAY_SIZE(dsi_a_1080p_11_1_modes);
	dc->enable = dsi_a_1080p_11_1_enable;
	dc->disable = dsi_a_1080p_11_1_disable;
	dc->postsuspend	= dsi_a_1080p_11_1_postsuspend,
	dc->width = 256;
	dc->height = 144;
	dc->flags = DC_CTRL_MODE;
}

static void dsi_a_1080p_11_1_fb_data_init(struct tegra_fb_data *fb)
{
	fb->xres = dsi_a_1080p_11_1_modes[0].h_active;
	fb->yres = dsi_a_1080p_11_1_modes[0].v_active;
}

static void
dsi_a_1080p_11_1_sd_settings_init(struct tegra_dc_sd_settings *settings)
{
	settings->bl_device_name = "pwm-backlight";
}

static struct i2c_board_info keenhi_sn65dsi86_dsi2edp_board_info __initdata = {
		I2C_BOARD_INFO("sn65dsi86_dsi2edp", 0x2C),
};

static int __init dsi_a_1080p_11_1_i2c_bridge_register(void)
{
	int err = 0;
	err = i2c_register_board_info(0,
			&keenhi_sn65dsi86_dsi2edp_board_info, 1);
	return err;
}
struct tegra_panel __initdata dsi_a_1080p_11_1 = {
	.init_sd_settings = dsi_a_1080p_11_1_sd_settings_init,
	.init_dc_out = dsi_a_1080p_11_1_dc_out_init,
	.init_fb_data = dsi_a_1080p_11_1_fb_data_init,
	.init_resources = dsi_a_1080p_11_1_resources_init,
	.register_bl_dev = dsi_a_1080p_11_1_register_bl_dev,
	.register_i2c_bridge = dsi_a_1080p_11_1_i2c_bridge_register,
	.set_disp_device = dsi_a_1080p_11_1_set_disp_device,
};
EXPORT_SYMBOL(dsi_a_1080p_11_1);

