/*
 * arch/arm/mach-tegra/board-keenhi_t40-panel.c
 *
 * Copyright (c) 2011-2014, NVIDIA Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <linux/ioport.h>
#include <linux/fb.h>
#include <linux/nvmap.h>
#include <linux/nvhost.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/tegra_pwm_bl.h>
#include <linux/regulator/consumer.h>
#include <linux/pwm_backlight.h>

#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/dc.h>
#include <mach/pinmux.h>
#include <mach/pinmux-t11.h>
#include <board-keenhi-t40.h>
#include "board.h"
#include "devices.h"
#include "gpio-names.h"
#include "board-panel.h"
#include "common.h"
#include "tegra11_host1x_devices.h"
#ifdef CONFIG_SN65DSI83_BRIDGE
#include <linux/sn65dsi83.h>
#define SN65DSI83_MIPI_INT TEGRA_GPIO_PI5
#define SN65DSI83_MIPI_EN TEGRA_GPIO_PH4
#endif


struct platform_device * __init keenhi_t40_host1x_init(void)
{
	struct platform_device *pdev = NULL;

#ifdef CONFIG_TEGRA_GRHOST
	pdev = tegra11_register_host1x_devices();
	if (!pdev) {
		pr_err("host1x devices registration failed\n");
		return NULL;
	}
#endif
	return pdev;
}

#ifdef CONFIG_TEGRA_DC

#if HDMI_SUPPORT
/* HDMI Hotplug detection pin */
#define keenhi_t40_hdmi_hpd	TEGRA_GPIO_PN7

static struct regulator *keenhi_t40_hdmi_reg;
static struct regulator *keenhi_t40_hdmi_pll;
static struct regulator *keenhi_t40_hdmi_vddio;
#endif

static struct resource keenhi_t40_disp1_resources[] = {
	{
		.name	= "irq",
		.start	= INT_DISPLAY_GENERAL,
		.end	= INT_DISPLAY_GENERAL,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "regs",
		.start	= TEGRA_DISPLAY_BASE,
		.end	= TEGRA_DISPLAY_BASE + TEGRA_DISPLAY_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "fbmem",
		.start	= 0, /* Filled in by keenhi_t40_panel_init() */
		.end	= 0, /* Filled in by keenhi_t40_panel_init() */
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "ganged_dsia_regs",
		.start	= 0, /* Filled in the panel file by init_resources() */
		.end	= 0, /* Filled in the panel file by init_resources() */
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "ganged_dsib_regs",
		.start	= 0, /* Filled in the panel file by init_resources() */
		.end	= 0, /* Filled in the panel file by init_resources() */
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "dsi_regs",
		.start	= 0, /* Filled in the panel file by init_resources() */
		.end	= 0, /* Filled in the panel file by init_resources() */
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "mipi_cal",
		.start	= TEGRA_MIPI_CAL_BASE,
		.end	= TEGRA_MIPI_CAL_BASE + TEGRA_MIPI_CAL_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

#if HDMI_SUPPORT
static struct resource keenhi_t40_disp2_resources[] = {
	{
		.name	= "irq",
		.start	= INT_DISPLAY_B_GENERAL,
		.end	= INT_DISPLAY_B_GENERAL,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "regs",
		.start	= TEGRA_DISPLAY2_BASE,
		.end	= TEGRA_DISPLAY2_BASE + TEGRA_DISPLAY2_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "fbmem",
		.start	= 0, /* Filled in by keenhi_t40_panel_init() */
		.end	= 0, /* Filled in by keenhi_t40_panel_init() */
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "hdmi_regs",
		.start	= TEGRA_HDMI_BASE,
		.end	= TEGRA_HDMI_BASE + TEGRA_HDMI_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};
#endif

static struct tegra_dc_sd_settings sd_settings;

static struct tegra_dc_out keenhi_t40_disp1_out = {
	.type		= TEGRA_DC_OUT_DSI,
	.sd_settings	= &sd_settings,
};

#if HDMI_SUPPORT
static int keenhi_t40_hdmi_enable(struct device *dev)
{
	int ret;
	if (!keenhi_t40_hdmi_reg) {
		keenhi_t40_hdmi_reg = regulator_get(dev, "avdd_hdmi");
		if (IS_ERR_OR_NULL(keenhi_t40_hdmi_reg)) {
			pr_err("hdmi: couldn't get regulator avdd_hdmi\n");
			keenhi_t40_hdmi_reg = NULL;
			return PTR_ERR(keenhi_t40_hdmi_reg);
		}
	}
	ret = regulator_enable(keenhi_t40_hdmi_reg);
	if (ret < 0) {
		pr_err("hdmi: couldn't enable regulator avdd_hdmi\n");
		return ret;
	}
	if (!keenhi_t40_hdmi_pll) {
		keenhi_t40_hdmi_pll = regulator_get(dev, "avdd_hdmi_pll");
		if (IS_ERR_OR_NULL(keenhi_t40_hdmi_pll)) {
			pr_err("hdmi: couldn't get regulator avdd_hdmi_pll\n");
			keenhi_t40_hdmi_pll = NULL;
			regulator_put(keenhi_t40_hdmi_reg);
			keenhi_t40_hdmi_reg = NULL;
			return PTR_ERR(keenhi_t40_hdmi_pll);
		}
	}
	ret = regulator_enable(keenhi_t40_hdmi_pll);
	if (ret < 0) {
		pr_err("hdmi: couldn't enable regulator avdd_hdmi_pll\n");
		return ret;
	}
	return 0;
}

static int keenhi_t40_hdmi_disable(void)
{
	if (keenhi_t40_hdmi_reg) {
		regulator_disable(keenhi_t40_hdmi_reg);
		regulator_put(keenhi_t40_hdmi_reg);
		keenhi_t40_hdmi_reg = NULL;
	}

	if (keenhi_t40_hdmi_pll) {
		regulator_disable(keenhi_t40_hdmi_pll);
		regulator_put(keenhi_t40_hdmi_pll);
		keenhi_t40_hdmi_pll = NULL;
	}

	return 0;
}

static int keenhi_t40_hdmi_postsuspend(void)
{
	if (keenhi_t40_hdmi_vddio) {
		regulator_disable(keenhi_t40_hdmi_vddio);
		regulator_put(keenhi_t40_hdmi_vddio);
		keenhi_t40_hdmi_vddio = NULL;
	}
	return 0;
}

static int keenhi_t40_hdmi_hotplug_init(struct device *dev)
{
	if (!keenhi_t40_hdmi_vddio) {
		keenhi_t40_hdmi_vddio = regulator_get(dev, "vdd_hdmi_5v0");
		if (WARN_ON(IS_ERR(keenhi_t40_hdmi_vddio))) {
			pr_err("%s: couldn't get regulator vdd_hdmi_5v0: %ld\n",
				__func__, PTR_ERR(keenhi_t40_hdmi_vddio));
				keenhi_t40_hdmi_vddio = NULL;
		} else {
			regulator_enable(keenhi_t40_hdmi_vddio);
			mdelay(5);
		}
	}

	return 0;
}

static void keenhi_t40_hdmi_hotplug_report(bool state)
{
	if (state) {
		tegra_pinmux_set_pullupdown(TEGRA_PINGROUP_DDC_SDA,
						TEGRA_PUPD_PULL_DOWN);
		tegra_pinmux_set_pullupdown(TEGRA_PINGROUP_DDC_SCL,
						TEGRA_PUPD_PULL_DOWN);
	} else {
		tegra_pinmux_set_pullupdown(TEGRA_PINGROUP_DDC_SDA,
						TEGRA_PUPD_NORMAL);
		tegra_pinmux_set_pullupdown(TEGRA_PINGROUP_DDC_SCL,
						TEGRA_PUPD_NORMAL);
	}
}

/* Electrical characteristics for HDMI, all modes must be declared here */
struct tmds_config keenhi_t40_tmds_config[] = {
	{ /* 480p : 27 MHz and below */
		.pclk = 27000000,
		.pll0 = 0x01003010,
		.pll1 = 0x00301b00,
		.drive_current = 0x23232323,
		.pe_current = 0x00000000,
		.peak_current = 0x00000000,
	},
	{ /* 720p : 74.25MHz modes */
		.pclk = 74250000,
		.pll0 = 0x01003110,
		.pll1 = 0x00301b00,
		.drive_current = 0x25252525,
		.pe_current = 0x00000000,
		.peak_current = 0x03030303,
	},
	{ /* 1080p : 148.5MHz modes */
		.pclk = 148500000,
		.pll0 = 0x01003310,
		.pll1 = 0x00301b00,
		.drive_current = 0x27272727,
		.pe_current = 0x00000000,
		.peak_current = 0x03030303,
	},
	{ /* 4K : 297MHz modes */
		.pclk = INT_MAX,
		.pll0 = 0x01003f10,
		.pll1 = 0x00300f00,
		.drive_current = 0x303f3f3f,
		.pe_current = 0x00000000,
		.peak_current = 0x040f0f0f,
	},
};

struct tegra_hdmi_out keenhi_t40_hdmi_out = {
	.tmds_config = keenhi_t40_tmds_config,
	.n_tmds_config = ARRAY_SIZE(keenhi_t40_tmds_config),
};

static struct tegra_dc_out keenhi_t40_disp2_out = {
	.type		= TEGRA_DC_OUT_HDMI,
	.flags		= TEGRA_DC_OUT_HOTPLUG_HIGH,
	.parent_clk	= "pll_d2_out0",

	.dcc_bus	= 3,
	.hotplug_gpio	= keenhi_t40_hdmi_hpd,
	.hdmi_out	= &keenhi_t40_hdmi_out,
	.max_pixclock	= KHZ2PICOS(297000),

	.align		= TEGRA_DC_ALIGN_MSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,

	.enable		= keenhi_t40_hdmi_enable,
	.disable	= keenhi_t40_hdmi_disable,
	.postsuspend	= keenhi_t40_hdmi_postsuspend,
	.hotplug_init	= keenhi_t40_hdmi_hotplug_init,
	.hotplug_report	= keenhi_t40_hdmi_hotplug_report,
};
#endif

static struct tegra_fb_data keenhi_t40_disp1_fb_data = {
	.win		= 0,
	.bits_per_pixel = 32,
	.flags		= TEGRA_FB_FLIP_ON_PROBE,
};

static struct tegra_dc_platform_data keenhi_t40_disp1_pdata = {
	.flags		= TEGRA_DC_FLAG_ENABLED,
	.default_out	= &keenhi_t40_disp1_out,
	.fb		= &keenhi_t40_disp1_fb_data,
	.emc_clk_rate	= 204000000,
#ifdef CONFIG_TEGRA_DC_CMU
	.cmu_enable	= 1,
#endif
};

#if HDMI_SUPPORT
static struct tegra_fb_data keenhi_t40_disp2_fb_data = {
	.win		= 0,
	.xres		= 1024,
	.yres		= 600,
	.bits_per_pixel = 32,
	.flags		= TEGRA_FB_FLIP_ON_PROBE,
};

static struct tegra_dc_platform_data keenhi_t40_disp2_pdata = {
	.default_out	= &keenhi_t40_disp2_out,
	.fb		= &keenhi_t40_disp2_fb_data,
	.emc_clk_rate	= 300000000,
};

static struct platform_device keenhi_t40_disp2_device = {
	.name		= "tegradc",
	.id		= 1,
	.resource	= keenhi_t40_disp2_resources,
	.num_resources	= ARRAY_SIZE(keenhi_t40_disp2_resources),
	.dev = {
		.platform_data = &keenhi_t40_disp2_pdata,
	},
};
#endif

static struct platform_device keenhi_t40_disp1_device = {
	.name		= "tegradc",
	.id		= 0,
	.resource	= keenhi_t40_disp1_resources,
	.num_resources	= ARRAY_SIZE(keenhi_t40_disp1_resources),
	.dev = {
		.platform_data = &keenhi_t40_disp1_pdata,
	},
};

static struct nvmap_platform_carveout keenhi_t40_carveouts[] = {
	[0] = {
		.name		= "iram",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_IRAM,
		.base		= TEGRA_IRAM_BASE + TEGRA_RESET_HANDLER_SIZE,
		.size		= TEGRA_IRAM_SIZE - TEGRA_RESET_HANDLER_SIZE,
		.buddy_size	= 0, /* no buddy allocation for IRAM */
	},
	[1] = {
		.name		= "generic-0",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_GENERIC,
		.base		= 0, /* Filled in by keenhi_t40_panel_init() */
		.size		= 0, /* Filled in by keenhi_t40_panel_init() */
		.buddy_size	= SZ_32K,
	},
	[2] = {
		.name		= "vpr",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_VPR,
		.base		= 0, /* Filled in by keenhi_t40_panel_init() */
		.size		= 0, /* Filled in by keenhi_t40_panel_init() */
		.buddy_size	= SZ_32K,
	},
};

static struct nvmap_platform_data keenhi_t40_nvmap_data = {
	.carveouts	= keenhi_t40_carveouts,
	.nr_carveouts	= ARRAY_SIZE(keenhi_t40_carveouts),
};
static struct platform_device keenhi_t40_nvmap_device = {
	.name	= "tegra-nvmap",
	.id	= -1,
	.dev	= {
		.platform_data = &keenhi_t40_nvmap_data,
	},
};

static struct tegra_dc_sd_settings keenhi_t40_sd_settings = {
	.enable = 1, /* enabled by default. */
	.use_auto_pwm = false,
	.hw_update_delay = 0,
	.bin_width = -1,
	.aggressiveness = 5,
	.use_vid_luma = false,
	.phase_in_adjustments = 0,
	.k_limit_enable = true,
	.k_limit = 200,
	.sd_window_enable = false,
	.soft_clipping_enable = true,
	/* Low soft clipping threshold to compensate for aggressive k_limit */
	.soft_clipping_threshold = 128,
	.smooth_k_enable = true,
	.smooth_k_incr = 4,
	/* Default video coefficients */
	.coeff = {5, 9, 2},
	.fc = {0, 0},
	/* Immediate backlight changes */
	.blp = {1024, 255},
	/* Gammas: R: 2.2 G: 2.2 B: 2.2 */
	/* Default BL TF */
	.bltf = {
			{
				{57, 65, 73, 82},
				{92, 103, 114, 125},
				{138, 150, 164, 178},
				{193, 208, 224, 241},
			},
		},
	/* Default LUT */
	.lut = {
			{
				{255, 255, 255},
				{199, 199, 199},
				{153, 153, 153},
				{116, 116, 116},
				{85, 85, 85},
				{59, 59, 59},
				{36, 36, 36},
				{17, 17, 17},
				{0, 0, 0},
			},
		},
	.sd_brightness = &sd_brightness,
	.use_vpulse2 = true,
};

static void keenhi_t40_panel_select(void)
{
	struct tegra_panel *panel = NULL;
	struct board_info board;

	tegra_get_display_board_info(&board);

	switch (board.board_id) {
	case BOARD_E1639:
//		panel = &dsi_s_wqxga_10_1;
//		break;
	default:
		panel = &dsi_u_1080p_11_6;
		break;
	}
	if (panel) {
		if (panel->init_sd_settings)
			panel->init_sd_settings(&sd_settings);

		if (panel->init_dc_out)
			panel->init_dc_out(&keenhi_t40_disp1_out);

		if (panel->init_fb_data)
			panel->init_fb_data(&keenhi_t40_disp1_fb_data);

		if (panel->init_cmu_data)
			panel->init_cmu_data(&keenhi_t40_disp1_pdata);

		if (panel->set_disp_device)
			panel->set_disp_device(&keenhi_t40_disp1_device);

		if (panel->init_resources)
			panel->init_resources(keenhi_t40_disp1_resources,
				ARRAY_SIZE(keenhi_t40_disp1_resources));

		if (panel->register_bl_dev)
			panel->register_bl_dev();

		if (panel->register_i2c_bridge)
			panel->register_i2c_bridge();
	}

}
int __init keenhi_t40_panel_init(void)
{
	int err = 0;
	struct resource __maybe_unused *res;
	struct platform_device *phost1x;

	sd_settings = keenhi_t40_sd_settings;

	keenhi_t40_disp1_fb_data.flags |=  TEGRA_FB_DISABLE_BLANK;
	keenhi_t40_panel_select();

#ifdef CONFIG_TEGRA_NVMAP
	keenhi_t40_carveouts[1].base = tegra_carveout_start;
	keenhi_t40_carveouts[1].size = tegra_carveout_size;
	keenhi_t40_carveouts[2].base = tegra_vpr_start;
	keenhi_t40_carveouts[2].size = tegra_vpr_size;

	err = platform_device_register(&keenhi_t40_nvmap_device);
	if (err) {
		pr_err("nvmap device registration failed\n");
		return err;
	}
#endif

	phost1x = keenhi_t40_host1x_init();
	if (!phost1x) {
		pr_err("host1x devices registration failed\n");
		return -EINVAL;
	}
#if HDMI_SUPPORT
	gpio_request(keenhi_t40_hdmi_hpd, "hdmi_hpd");
	gpio_direction_input(keenhi_t40_hdmi_hpd);
#endif
	res = platform_get_resource_byname(&keenhi_t40_disp1_device,
					 IORESOURCE_MEM, "fbmem");
	res->start = tegra_fb_start;
	res->end = tegra_fb_start + tegra_fb_size - 1;

	/* Copy the bootloader fb to the fb. */
	__tegra_move_framebuffer(&keenhi_t40_nvmap_device,
		tegra_fb_start, tegra_bootloader_fb_start,
			min(tegra_fb_size, tegra_bootloader_fb_size));

	keenhi_t40_disp1_device.dev.parent = &phost1x->dev;
	err = platform_device_register(&keenhi_t40_disp1_device);
	if (err) {
		pr_err("disp1 device registration failed\n");
		return err;
	}
#if HDMI_SUPPORT
	err = tegra_init_hdmi(&keenhi_t40_disp2_device, phost1x);
	if (err)
		return err;
#endif
#ifdef CONFIG_TEGRA_NVAVP
	nvavp_device.dev.parent = &phost1x->dev;
	err = platform_device_register(&nvavp_device);
	if (err) {
		pr_err("nvavp device registration failed\n");
		return err;
	}
#endif
	return err;
}
#else
int __init keenhi_t40_panel_init(void)
{
	if (keenhi_t40_host1x_init())
		return 0;
	else
		return -EINVAL;
}
#endif
