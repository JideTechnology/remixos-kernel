#ifndef __ASMARM_ARCH_TIMER_H
#define __ASMARM_ARCH_TIMER_H

#include <linux/ioport.h>
#include <linux/clocksource.h>
#include <asm/errno.h>

struct arch_timer {
	struct resource	res[2];
};

#ifdef CONFIG_ARM_ARCH_TIMER
int arch_timer_of_register(void);
int arch_timer_sched_clock_init(void);

// @jide keep in sync with clocksource.h
typedef u64 cycle_t;

static __always_inline cycle_t arch_counter_get_cntpct(void)
{
	u32 cvall, cvalh;

	asm volatile("mrrc p15, 0, %0, %1, c14" : "=r" (cvall), "=r" (cvalh));

	return ((cycle_t) cvalh << 32) | cvall;
}

static __always_inline cycle_t arch_counter_get_cntvct(void)
{
	u32 cvall, cvalh;

	asm volatile("mrrc p15, 1, %0, %1, c14" : "=r" (cvall), "=r" (cvalh));

	return ((cycle_t) cvalh << 32) | cvall;
}

#else
static inline int arch_timer_of_register(void)
{
	return -ENXIO;
}

static inline int arch_timer_sched_clock_init(void)
{
	return -ENXIO;
}
#endif

#endif
