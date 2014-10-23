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

#include <linux/export.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <mach/iomap.h>
#include <mach/tegra_usb_pad_ctrl.h>

static DEFINE_SPINLOCK(utmip_pad_lock);
static DEFINE_SPINLOCK(xusb_padctl_lock);
static int utmip_pad_count;
static struct clk *utmi_pad_clk;

int utmi_phy_iddq_override(bool set)
{
	unsigned long val, flags;
	void __iomem *clk_base = IO_ADDRESS(TEGRA_CLK_RESET_BASE);

	spin_lock_irqsave(&utmip_pad_lock, flags);
	val = readl(clk_base + UTMIPLL_HW_PWRDN_CFG0);
	if (set && !utmip_pad_count)
		val |= UTMIPLL_HW_PWRDN_CFG0_IDDQ_OVERRIDE;
	else if (!set && utmip_pad_count)
		val &= ~UTMIPLL_HW_PWRDN_CFG0_IDDQ_OVERRIDE;
	else
		goto out1;
	val |= UTMIPLL_HW_PWRDN_CFG0_IDDQ_SWCTL;
	writel(val, clk_base + UTMIPLL_HW_PWRDN_CFG0);

out1:
	spin_unlock_irqrestore(&utmip_pad_lock, flags);
	return 0;
}
EXPORT_SYMBOL_GPL(utmi_phy_iddq_override);

int utmi_phy_pad_enable(void)
{
	unsigned long val, flags;
	void __iomem *pad_base =  IO_ADDRESS(TEGRA_USB_BASE);

	if (!utmi_pad_clk)
		utmi_pad_clk = clk_get_sys("utmip-pad", NULL);

	clk_enable(utmi_pad_clk);

	spin_lock_irqsave(&utmip_pad_lock, flags);
	utmip_pad_count++;

	val = readl(pad_base + UTMIP_BIAS_CFG0);
	val &= ~(UTMIP_OTGPD | UTMIP_BIASPD);
	val |= UTMIP_HSSQUELCH_LEVEL(0x2) | UTMIP_HSDISCON_LEVEL(0x1) |
		UTMIP_HSDISCON_LEVEL_MSB;
	writel(val, pad_base + UTMIP_BIAS_CFG0);
	tegra_usb_pad_reg_update(XUSB_PADCTL_USB2_BIAS_PAD_CTL_0,
		PD_MASK, 0);
	spin_unlock_irqrestore(&utmip_pad_lock, flags);

	clk_disable(utmi_pad_clk);

	return 0;
}
EXPORT_SYMBOL_GPL(utmi_phy_pad_enable);

int utmi_phy_pad_disable(void)
{
	unsigned long val, flags;
	void __iomem *pad_base =  IO_ADDRESS(TEGRA_USB_BASE);

	if (!utmi_pad_clk)
		utmi_pad_clk = clk_get_sys("utmip-pad", NULL);

	clk_enable(utmi_pad_clk);
	spin_lock_irqsave(&utmip_pad_lock, flags);

	if (!utmip_pad_count) {
		pr_err("%s: utmip pad already powered off\n", __func__);
		goto out;
	}
	if (--utmip_pad_count == 0) {
		val = readl(pad_base + UTMIP_BIAS_CFG0);
		val |= UTMIP_OTGPD | UTMIP_BIASPD;
		val &= ~(UTMIP_HSSQUELCH_LEVEL(~0) | UTMIP_HSDISCON_LEVEL(~0) |
			UTMIP_HSDISCON_LEVEL_MSB);
		writel(val, pad_base + UTMIP_BIAS_CFG0);
		tegra_usb_pad_reg_update(XUSB_PADCTL_USB2_BIAS_PAD_CTL_0,
			PD_MASK, 1);
	}
out:
	spin_unlock_irqrestore(&utmip_pad_lock, flags);
	clk_disable(utmi_pad_clk);

	return 0;
}
EXPORT_SYMBOL_GPL(utmi_phy_pad_disable);

void tegra_usb_pad_reg_update(u32 reg_offset, u32 mask, u32 val)
{
	void __iomem *pad_base = IO_ADDRESS(TEGRA_XUSB_PADCTL_BASE);
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&xusb_padctl_lock, flags);

	reg = readl(pad_base + reg_offset);
	reg &= ~mask;
	reg |= val;
	writel(reg, pad_base + reg_offset);

	spin_unlock_irqrestore(&xusb_padctl_lock, flags);
}
EXPORT_SYMBOL_GPL(tegra_usb_pad_reg_update);

u32 tegra_usb_pad_reg_read(u32 reg_offset)
{
	void __iomem *pad_base = IO_ADDRESS(TEGRA_XUSB_PADCTL_BASE);
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&xusb_padctl_lock, flags);
	reg = readl(pad_base + reg_offset);
	spin_unlock_irqrestore(&xusb_padctl_lock, flags);

	return reg;
}
EXPORT_SYMBOL_GPL(tegra_usb_pad_reg_read);

void tegra_usb_pad_reg_write(u32 reg_offset, u32 val)
{
	void __iomem *pad_base = IO_ADDRESS(TEGRA_XUSB_PADCTL_BASE);
	unsigned long flags;
	spin_lock_irqsave(&xusb_padctl_lock, flags);
	writel(val, pad_base + reg_offset);
	spin_unlock_irqrestore(&xusb_padctl_lock, flags);
}
EXPORT_SYMBOL_GPL(tegra_usb_pad_reg_write);
