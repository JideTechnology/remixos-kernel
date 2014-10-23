/*
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef _TEGRA_USB_PAD_CTRL_INTERFACE_H_
#define _TEGRA_USB_PAD_CTRL_INTERFACE_H_

#define UTMIPLL_HW_PWRDN_CFG0			0x52c
#define   UTMIPLL_HW_PWRDN_CFG0_IDDQ_OVERRIDE  (1<<1)
#define   UTMIPLL_HW_PWRDN_CFG0_IDDQ_SWCTL     (1<<0)

#define UTMIP_BIAS_CFG0		0x80c
#define   UTMIP_OTGPD			(1 << 11)
#define   UTMIP_BIASPD			(1 << 10)
#define   UTMIP_HSSQUELCH_LEVEL(x)	(((x) & 0x3) << 0)
#define   UTMIP_HSDISCON_LEVEL(x)	(((x) & 0x3) << 2)
#define   UTMIP_HSDISCON_LEVEL_MSB	(1 << 24)

#ifdef CONFIG_ARCH_TEGRA_11x_SOC
#define XUSB_PADCTL_USB2_BIAS_PAD_CTL_0		0xa0
#else
#define XUSB_PADCTL_USB2_BIAS_PAD_CTL_0		0xb8
#endif
#define PD_MASK			(0x1 << 12)

int utmi_phy_pad_disable(void);
int utmi_phy_pad_enable(void);
int utmi_phy_iddq_override(bool set);
void tegra_usb_pad_reg_update(u32 reg_offset, u32 mask, u32 val);
u32 tegra_usb_pad_reg_read(u32 reg_offset);
void tegra_usb_pad_reg_write(u32 reg_offset, u32 val);
#endif
