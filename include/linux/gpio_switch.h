/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 * Copyright 2010, 2011 David Jander <david@protonic.nl>
 *
 * Copyright 2010-2011 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __GPIO_SWITCH_H
#define __GPIO_SWITCH_H
struct gpio_switch_platform_data {
	int gpio;		/* -1 if this key does not support gpio */
	bool disabled;
};

struct gpio_switch_drvdata {
	struct gpio_switch_platform_data *data;
	struct mutex mutex;
};
#endif
