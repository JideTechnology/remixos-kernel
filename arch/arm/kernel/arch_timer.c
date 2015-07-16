/*
 *  linux/arch/arm/kernel/arch_timer.c
 *
 *  Copyright (C) 2011 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/jiffies.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/io.h>

#include <asm/cputype.h>
#include <asm/delay.h>
#include <asm/localtimer.h>
#include <asm/arch_timer.h>
#include <asm/system_info.h>
#include <asm/sched_clock.h>

static unsigned long arch_timer_rate;
static int arch_timer_ppi;
static int arch_timer_ppi2;

static struct clock_event_device __percpu **arch_timer_evt;
static struct delay_timer arch_delay_timer;

/*
 * Architected system timer support.
 */

#define ARCH_TIMER_CTRL_ENABLE		(1 << 0)
#define ARCH_TIMER_CTRL_IT_MASK		(1 << 1)
#define ARCH_TIMER_CTRL_IT_STAT		(1 << 2)

#define ARCH_TIMER_REG_CTRL		0
#define ARCH_TIMER_REG_FREQ		1
#define ARCH_TIMER_REG_TVAL		2

#define ARCH_TIMER_PHYS_ACCESS          0
#define ARCH_TIMER_VIRT_ACCESS          1
#define ARCH_TIMER_MEM_PHYS_ACCESS      2
#define ARCH_TIMER_MEM_VIRT_ACCESS      3

#define ARCH_TIMER_USR_PCT_ACCESS_EN    (1 << 0) /* physical counter */
#define ARCH_TIMER_USR_VCT_ACCESS_EN    (1 << 1) /* virtual counter */
#define ARCH_TIMER_VIRT_EVT_EN          (1 << 2)
#define ARCH_TIMER_EVT_TRIGGER_SHIFT    (4)
#define ARCH_TIMER_EVT_TRIGGER_MASK     (0xF << ARCH_TIMER_EVT_TRIGGER_SHIFT)
#define ARCH_TIMER_USR_VT_ACCESS_EN     (1 << 8) /* virtual timer registers */
#define ARCH_TIMER_USR_PT_ACCESS_EN     (1 << 9) /* physical timer registers */

static void arch_timer_reg_write(int reg, u32 val)
{
	switch (reg) {
	case ARCH_TIMER_REG_CTRL:
		asm volatile("mcr p15, 0, %0, c14, c2, 1" : : "r" (val));
		break;
	case ARCH_TIMER_REG_TVAL:
		asm volatile("mcr p15, 0, %0, c14, c2, 0" : : "r" (val));
		break;
	}

	isb();
}

static u32 arch_timer_reg_read(int reg)
{
	u32 val;

	switch (reg) {
	case ARCH_TIMER_REG_CTRL:
		asm volatile("mrc p15, 0, %0, c14, c2, 1" : "=r" (val));
		break;
	case ARCH_TIMER_REG_FREQ:
		asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r" (val));
		break;
	case ARCH_TIMER_REG_TVAL:
		asm volatile("mrc p15, 0, %0, c14, c2, 0" : "=r" (val));
		break;
	default:
		BUG();
	}

	return val;
}

static irqreturn_t arch_timer_handler(int irq, void *dev_id)
{
	struct clock_event_device *evt = *(struct clock_event_device **)dev_id;
	unsigned long ctrl;

	ctrl = arch_timer_reg_read(ARCH_TIMER_REG_CTRL);
	if (ctrl & ARCH_TIMER_CTRL_IT_STAT) {
		ctrl |= ARCH_TIMER_CTRL_IT_MASK;
		arch_timer_reg_write(ARCH_TIMER_REG_CTRL, ctrl);
		evt->event_handler(evt);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static void arch_timer_disable(void)
{
	unsigned long ctrl;

	ctrl = arch_timer_reg_read(ARCH_TIMER_REG_CTRL);
	ctrl &= ~ARCH_TIMER_CTRL_ENABLE;
	arch_timer_reg_write(ARCH_TIMER_REG_CTRL, ctrl);
}

static void arch_timer_set_mode(enum clock_event_mode mode,
				struct clock_event_device *clk)
{
	switch (mode) {
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
		arch_timer_disable();
		break;
	default:
		break;
	}
}

static int arch_timer_set_next_event(unsigned long evt,
				     struct clock_event_device *unused)
{
	unsigned long ctrl;

	ctrl = arch_timer_reg_read(ARCH_TIMER_REG_CTRL);
	ctrl |= ARCH_TIMER_CTRL_ENABLE;
	ctrl &= ~ARCH_TIMER_CTRL_IT_MASK;

	arch_timer_reg_write(ARCH_TIMER_REG_TVAL, evt);
	arch_timer_reg_write(ARCH_TIMER_REG_CTRL, ctrl);

	return 0;
}

static inline u32 arch_timer_get_cntkctl(void)
{
	u32 cntkctl;
	asm volatile("mrc p15, 0, %0, c14, c1, 0" : "=r" (cntkctl));
	return cntkctl;
}

static inline void arch_timer_set_cntkctl(u32 cntkctl)
{
	asm volatile("mcr p15, 0, %0, c14, c1, 0" : : "r" (cntkctl));
}

static void arch_counter_set_user_access(void)
{
	u32 cntkctl = arch_timer_get_cntkctl();

	/* Disable user access to the timers and the physical counter */
	/* Also disable virtual event stream */
	cntkctl &= ~(ARCH_TIMER_USR_PT_ACCESS_EN
			| ARCH_TIMER_USR_VT_ACCESS_EN
			| ARCH_TIMER_VIRT_EVT_EN
			| ARCH_TIMER_USR_PCT_ACCESS_EN);

	/* Enable user access to the virtual counter */
	cntkctl |= ARCH_TIMER_USR_VCT_ACCESS_EN;

	arch_timer_set_cntkctl(cntkctl);
}

static int __cpuinit arch_timer_setup(struct clock_event_device *clk)
{
	/* Be safe... */
	arch_timer_disable();

	clk->features = CLOCK_EVT_FEAT_ONESHOT | CLOCK_EVT_FEAT_C3STOP;
	clk->name = "arch_sys_timer";
	clk->rating = 450;
	clk->set_mode = arch_timer_set_mode;
	clk->set_next_event = arch_timer_set_next_event;
	clk->irq = arch_timer_ppi;

	clockevents_config_and_register(clk, arch_timer_rate,
					0xf, 0x7fffffff);

	*__this_cpu_ptr(arch_timer_evt) = clk;

	enable_percpu_irq(clk->irq, 0);
	if (arch_timer_ppi2)
		enable_percpu_irq(arch_timer_ppi2, 0);

	arch_counter_set_user_access();

	return 0;
}

/* Is the optional system timer available? */
static int local_timer_is_architected(void)
{
	return (cpu_architecture() >= CPU_ARCH_ARMv7) &&
	       ((read_cpuid_ext(CPUID_EXT_PFR1) >> 16) & 0xf) == 1;
}

static int arch_timer_available(void)
{
	unsigned long freq;

	if (!local_timer_is_architected())
		return -ENXIO;

	if (arch_timer_rate == 0) {
		arch_timer_reg_write(ARCH_TIMER_REG_CTRL, 0);
		freq = arch_timer_reg_read(ARCH_TIMER_REG_FREQ);

		/* Check the timer frequency. */
		if (freq == 0) {
			pr_warn("Architected timer frequency not available\n");
			return -EINVAL;
		}

		arch_timer_rate = freq;
		pr_info("Architected local timer running at %lu.%02luMHz.\n",
			arch_timer_rate / 1000000, (arch_timer_rate / 10000) % 100);
	}

	return 0;
}

static u32 notrace arch_counter_get_cntvct32(void)
{
	cycle_t cntvct = arch_counter_get_cntvct();

	/*
	 * The sched_clock infrastructure only knows about counters
	 * with at most 32bits. Forget about the upper 24 bits for the
	 * time being...
	 */
	return (u32)(cntvct & (u32)~0);
}

static cycle_t arch_counter_read(struct clocksource *cs)
{
	return arch_counter_get_cntpct();
}

static unsigned long arch_timer_read_current_timer(void)
{
	return arch_counter_get_cntpct();
}

static struct clocksource clocksource_counter = {
	.name	= "arch_sys_counter",
	.rating	= 400,
	.read	= arch_counter_read,
	.mask	= CLOCKSOURCE_MASK(56),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static void __cpuinit arch_timer_stop(struct clock_event_device *clk)
{
	pr_debug("arch_timer_teardown disable IRQ%d cpu #%d\n",
		 clk->irq, smp_processor_id());
	disable_percpu_irq(clk->irq);
	if (arch_timer_ppi2)
		disable_percpu_irq(arch_timer_ppi2);
	arch_timer_set_mode(CLOCK_EVT_MODE_UNUSED, clk);
}

static struct local_timer_ops arch_timer_ops __cpuinitdata = {
	.setup	= arch_timer_setup,
	.stop	= arch_timer_stop,
};

static unsigned int saved_cntkctl;
static int arch_timer_cpu_pm_notify(struct notifier_block *self,
				    unsigned long action, void *hcpu)
{
	if (action == CPU_PM_ENTER)
		saved_cntkctl = arch_timer_get_cntkctl();
	else if (action == CPU_PM_ENTER_FAILED || action == CPU_PM_EXIT)
		arch_timer_set_cntkctl(saved_cntkctl);
	return NOTIFY_OK;
}

static struct notifier_block arch_timer_cpu_pm_notifier = {
	.notifier_call = arch_timer_cpu_pm_notify,
};

static int __init arch_timer_cpu_pm_init(void)
{
	return cpu_pm_register_notifier(&arch_timer_cpu_pm_notifier);
}

static struct clock_event_device arch_timer_global_evt;

static int __init arch_timer_common_register(void)
{
	int err;

	err = arch_timer_available();
	if (err)
		return err;

	arch_timer_evt = alloc_percpu(struct clock_event_device *);
	if (!arch_timer_evt)
		return -ENOMEM;

	clocksource_register_hz(&clocksource_counter, arch_timer_rate);

	err = request_percpu_irq(arch_timer_ppi, arch_timer_handler,
				 "arch_timer", arch_timer_evt);
	if (err) {
		pr_err("arch_timer: can't register interrupt %d (%d)\n",
		       arch_timer_ppi, err);
		goto out_free;
	}

	if (arch_timer_ppi2) {
		err = request_percpu_irq(arch_timer_ppi2, arch_timer_handler,
					 "arch_timer", arch_timer_evt);
		if (err) {
			pr_err("arch_timer: can't register interrupt %d (%d)\n",
			       arch_timer_ppi2, err);
			arch_timer_ppi2 = 0;
			goto out_free_irq;
		}
	}

	err = arch_timer_cpu_pm_init();
	if (err)
		goto out_free_irq;

	err = local_timer_register(&arch_timer_ops);
	if (err) {
		/*
		 * We couldn't register as a local timer (could be
		 * because we're on a UP platform, or because some
		 * other local timer is already present...). Try as a
		 * global timer instead.
		 */
		arch_timer_global_evt.cpumask = cpumask_of(0);
		err = arch_timer_setup(&arch_timer_global_evt);
	}
	if (err)
		goto out_free_irq;
	arch_counter_set_user_access();

	/* Use the architected timer for the delay loop. */
	arch_delay_timer.read_current_timer = &arch_timer_read_current_timer;
	arch_delay_timer.freq = arch_timer_rate;
	register_current_timer_delay(&arch_delay_timer);
	return 0;

out_free_irq:
	free_percpu_irq(arch_timer_ppi, arch_timer_evt);
	if (arch_timer_ppi2)
		free_percpu_irq(arch_timer_ppi2, arch_timer_evt);

out_free:
	free_percpu(arch_timer_evt);

	return err;
}

int __init arch_timer_register(struct arch_timer *at)
{
	if (at->res[0].start <= 0 || !(at->res[0].flags & IORESOURCE_IRQ))
		return -EINVAL;

	arch_timer_ppi = at->res[0].start;

	if (at->res[1].start > 0 || (at->res[1].flags & IORESOURCE_IRQ))
		arch_timer_ppi2 = at->res[1].start;

	return arch_timer_common_register();
}

#ifdef CONFIG_OF
static const struct of_device_id arch_timer_of_match[] __initconst = {
	{ .compatible	= "arm,armv7-timer",	},
	{},
};

int __init arch_timer_of_register(void)
{
	struct device_node *np;
	u32 freq;

	np = of_find_matching_node(NULL, arch_timer_of_match);
	if (!np) {
		pr_err("arch_timer: can't find DT node\n");
		return -ENODEV;
	}

	/* Try to determine the frequency from the device tree or CNTFRQ */
	if (!of_property_read_u32(np, "clock-frequency", &freq))
		arch_timer_rate = freq;

	arch_timer_ppi = irq_of_parse_and_map(np, 0);
	arch_timer_ppi2 = irq_of_parse_and_map(np, 1);
	pr_info("arch_timer: found %s irqs %d %d\n",
		np->name, arch_timer_ppi, arch_timer_ppi2);

	return arch_timer_common_register();
}
#endif

int __init arch_timer_sched_clock_init(void)
{
	int err;

	err = arch_timer_available();
	if (err)
		return err;

	setup_sched_clock(arch_counter_get_cntvct32, 32, arch_timer_rate);
	return 0;
}
