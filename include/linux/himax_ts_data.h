#ifndef __TOUCHSCREEN_HIMAX_TS_H__
#define __TOUCHSCREEN_HIMAX_TS_H__

struct hxt_platform_data 
{ 
	unsigned long irqflags;
	bool	i2c_pull_up;
	bool	digital_pwr_regulator;
	int reset_gpio;
	u32 reset_gpio_flags;
	int irq_gpio;
	u32 irq_gpio_flags;

	int (*init_hw) (bool);
	int (*power_on) (bool);
};

#endif