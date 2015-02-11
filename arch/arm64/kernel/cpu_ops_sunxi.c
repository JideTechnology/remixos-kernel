/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright (C) 2014 Allwinner Technology Co., Ltd.
 *
 * Author: sunny <sunny@allwinnertech.com>
 */

#include <linux/cpuidle.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/smp.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/cpu.h>
#include <linux/types.h>

#include <asm/compiler.h>
#include <asm/cpu_ops.h>
#include <asm/errno.h>
#include <asm/psci.h>
#include <asm/smp_plat.h>
#include <asm/suspend.h>

#ifdef CONFIG_SMP

#define SUN50I_PRCM_PBASE	(0x01F01400)
#define SUN50I_CPUCFG_PBASE	(0x01700000)
#define SUN50I_RCPUCFG_PBASE	(0x01F01C00)

#define SUNXI_CPU_PWR_CLAMP(cluster, cpu)         (0x140 + (cluster*4 + cpu)*0x04)
#define SUNXI_CLUSTER_PWROFF_GATING(cluster)      (0x100 + (cluster)*0x04)
#define SUNXI_CLUSTER_PWRON_RESET(cluster)        (0x30  + (cluster)*0x04)

#define SUNXI_CLUSTER_CPU_STATUS(cluster)         (0x30 + (cluster)*0x4)
#define SUNXI_CPU_RST_CTRL(cluster)               (0x80 + (cluster)*0x4)
#define SUNXI_CLUSTER_CTRL0(cluster)              (0x00 + (cluster)*0x10)
#define SUNXI_CLUSTER_CTRL1(cluster)              (0x04 + (cluster)*0x10)

#define SUNXI_CPU_RVBA_L(cpu)	(0xA0 + (cpu)*0x8)
#define SUNXI_CPU_RVBA_H(cpu)   (0xA4 + (cpu)*0x8)

static void __iomem *sun50i_prcm_base;
static void __iomem *sun50i_cpucfg_base;
static void __iomem *sun50i_r_cpucfg_base;

static void sun50i_set_secondary_entry(unsigned long entry, unsigned int cpu)
{
	printk("%s %x\n", __func__, entry);
	writel(entry, sun50i_cpucfg_base + SUNXI_CPU_RVBA_L(cpu));
	writel(0, sun50i_cpucfg_base + SUNXI_CPU_RVBA_H(cpu));
}

static void sun50i_set_AA32nAA64(unsigned int cluster, unsigned int cpu, bool is_aa64)
{
	unsigned int value;

	value = readl(sun50i_cpucfg_base + SUNXI_CLUSTER_CTRL0(cluster));
	value &= ~(1<<(cpu + 24));
	value |=  (is_aa64 <<(cpu + 24));
	writel(value, sun50i_cpucfg_base + SUNXI_CLUSTER_CTRL0(cluster));
}

static int sun50i_power_switch_set(unsigned int cluster, unsigned int cpu, bool enable)
{
	if (enable) {
		if (0x00 == readl(sun50i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			pr_debug("%s: power switch enable already\n", __func__);
			return 0;
		}
		/* de-active cpu power clamp */
		writel(0xFE, sun50i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(20);

		writel(0xF8, sun50i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(10);

		writel(0xE0, sun50i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(10);

		writel(0x80, sun50i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(10);

		writel(0x00, sun50i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(20);
		while(0x00 != readl(sun50i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			;
		}
	} else {
		if (0xFF == readl(sun50i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			pr_debug("%s: power switch disable already\n", __func__);
			return 0;
		}
		writel(0xFF, sun50i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(30);
		while(0xFF != readl(sun50i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			;
		}
	}
	return 0;
}

int sun50i_cpu_power_up(unsigned int cluster, unsigned int cpu)
{
	unsigned int value;

	/*
	 * power-up cpu core process
	 */
	pr_debug("sun50i power-up cluster-%d cpu-%d\n", cluster, cpu);

	/* assert cpu core reset */
	value  = readl(sun50i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
	value &= (~(1<<cpu));
	writel(value, sun50i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
	udelay(10);

	/* assert cpu power-on reset */
	value  = readl(sun50i_r_cpucfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
	value &= (~(1<<cpu));
	writel(value, sun50i_r_cpucfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
	udelay(10);

	/* set AA32nAA64 to AA64 */
	sun50i_set_AA32nAA64(cluster, cpu, 1);

	/* release power switch */
	sun50i_power_switch_set(cluster, cpu, 1);

	/* clear power-off gating */
	value = readl(sun50i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
	value &= (~(0x1<<cpu));
	writel(value, sun50i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
	udelay(20);

	/* de-assert cpu power-on reset */
	value  = readl(sun50i_r_cpucfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
	value |= ((1<<cpu));
	writel(value, sun50i_r_cpucfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
	udelay(10);

	/* de-assert core reset */
	value  = readl(sun50i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
	value |= (1<<cpu);
	writel(value, sun50i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
	udelay(10);

	pr_debug("sun50i power-up cluster-%d cpu-%d already\n", cluster, cpu);

	return 0;
}


int sun50i_cpu_power_down(unsigned int cluster, unsigned int cpu)
{
	unsigned int value;

	/*
	 * power-down cpu core process
	 */
	pr_debug("sun50i power-down cluster-%d cpu-%d\n", cluster, cpu);

	/* enable cpu power-off gating */
	value = readl(sun50i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
	value |= (1 << cpu);
	writel(value, sun50i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
	udelay(20);

	/* active the power output switch */
	sun50i_power_switch_set(cluster, cpu, 0);

	pr_debug("sun50i power-down cpu%d ok.\n", cpu);

	return 0;
}

static int __init sunxi_cpu_init(struct device_node *dn, unsigned int cpu)
{
	return 0;
}

static volatile int sunxi_iomap_init = 0;
static int sunxi_cpu_iomap_init(void)
{
	printk("%s: enter\n", __func__);

	sun50i_prcm_base     = ioremap(SUN50I_PRCM_PBASE, SZ_4K);
	sun50i_cpucfg_base   = ioremap(SUN50I_CPUCFG_PBASE, SZ_4K);
	sun50i_r_cpucfg_base = ioremap(SUN50I_RCPUCFG_PBASE, SZ_4K);

	printk("%s: done\n", __func__);
	return 0;
}

static int __init sunxi_cpu_prepare(unsigned int cpu)
{
	return 0;
}

static int sunxi_cpu_boot(unsigned int cpu)
{
	int err;

	if (sunxi_iomap_init == 0) {
		sunxi_cpu_iomap_init();
		sunxi_iomap_init = 1;
	}

	sun50i_set_secondary_entry(__pa(secondary_entry), cpu_logical_map(cpu));

	err = sun50i_cpu_power_up(0, cpu_logical_map(cpu));
	if (err) {
		pr_err("%s: failed to boot CPU%d (%d)\n", __func__, cpu, err);
	}
	return err;
}

#ifdef CONFIG_HOTPLUG_CPU
static int sunxi_cpu_disable(unsigned int cpu)
{
	return 0;
}

static void sunxi_cpu_die(unsigned int cpu)
{
	int ret;

	if (sunxi_iomap_init == 0) {
		sunxi_cpu_iomap_init();
		sunxi_iomap_init = 1;
	}

	ret = sun50i_cpu_power_down(0, cpu_logical_map(cpu));

	pr_crit("unable to power off CPU%u (%d)\n", cpu, ret);
}
#endif

#ifdef CONFIG_ARM64_CPU_SUSPEND
static int sunxi_cpu_suspend(unsigned long index)
{
	return -EOPNOTSUPP;
}
#endif

const struct cpu_operations cpu_ops_sunxi = {
	.name		= "sunxi",
	.cpu_init	= sunxi_cpu_init,
	.cpu_prepare	= sunxi_cpu_prepare,
	.cpu_boot	= sunxi_cpu_boot,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_disable	= sunxi_cpu_disable,
	.cpu_die	= sunxi_cpu_die,
#endif
#ifdef CONFIG_ARM64_CPU_SUSPEND
	.cpu_suspend	= sunxi_cpu_suspend,
#endif
};

#endif
