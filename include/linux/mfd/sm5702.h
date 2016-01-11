/*
 * include/linux/mfd/sm5702.h
 *
 * Driver to Siliconmitus SM5702
 * Multi function device -- Charger / Battery Gauge / DCDC Converter / LED Flashlight
 *
 * Copyright (C) 2013 Siliconmitus Technology Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef SM5702_H
#define SM5702_H
/* delete below header caused it is only for this module*/
/*#include <linux/rtdefs.h> */

#include <linux/mutex.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/kernel.h>

#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/wakelock.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
//#include <linux/power/charger_debug.h>

#include <linux/pinctrl/consumer.h>

#define SM5702_DRV_VER "0.0.1"

#define SM5702CF_SLAVE_ADDR   (0x49)//CHARGER,FLASH

/* Control Register */
#define SM5702_INT1	                0x00
#define SM5702_INT2			        0x01
#define SM5702_INT3			        0x02
#define SM5702_INT4			        0x03
#define SM5702_INTMSK1			    0x04
#define SM5702_INTMSK2			    0x05
#define SM5702_INTMSK3				0x06
#define SM5702_INTMSK4			    0x07
#define SM5702_STATUS1				0x08
#define SM5702_STATUS2				0x09
#define SM5702_STATUS3				0x0A
#define SM5702_STATUS4				0x0B

#define SM5702_CNTL				    0x0C
#define SM5702_VBUSCNTL				0x0D
#define SM5702_CHGCNTL1				0x0E
#define SM5702_CHGCNTL2				0x0F
#define SM5702_CHGCNTL3				0x10
#define SM5702_CHGCNTL4				0x11
#define SM5702_CHGCNTL5				0x12

#define SM5702_FLEDCNTL1			0x13
#define SM5702_FLEDCNTL2			0x14
#define SM5702_FLEDCNTL3			0x15
#define SM5702_FLEDCNTL4			0x16
#define SM5702_FLEDCNTL5			0x17
#define SM5702_FLEDCNTL6			0x18


#define SM5702_INT1_IRQ_REGS_NR 1
#define SM5702_INT2_IRQ_REGS_NR 1
#define SM5702_INT3_IRQ_REGS_NR 1
#define SM5702_INT4_IRQ_REGS_NR 1

#define SM5702_CFIRQ_REGS_NR \
    (SM5702_INT1_IRQ_REGS_NR + \
     SM5702_INT2_IRQ_REGS_NR + \
     SM5702_INT3_IRQ_REGS_NR + \
     SM5702_INT4_IRQ_REGS_NR)

#define SM5702_IRQ_REGS_NR  SM5702_CFIRQ_REGS_NR

typedef union sm5702_irq_status {
    struct {
        uint8_t int1_irq_status[SM5702_INT1_IRQ_REGS_NR];
        uint8_t int2_irq_status[SM5702_INT2_IRQ_REGS_NR];
        uint8_t int3_irq_status[SM5702_INT3_IRQ_REGS_NR];
        uint8_t int4_irq_status[SM5702_INT4_IRQ_REGS_NR];
    };
    struct {
        uint16_t regs[SM5702_IRQ_REGS_NR];
    };
} sm5702_irq_status_t;

//#ifdef CONFIG_FLED_SM5702 
//struct sm5702_fled_platform_data;
//#endif

#ifdef CONFIG_CHARGER_SM5702
typedef struct sm5702_charger_platform_data {
    int chg_float_voltage;

    char *charger_name;
    int chgen_gpio; //nCHGEN Pin
} sm5702_charger_platform_data_t;
#endif

struct sm5702_mfd_platform_data {
	int irq;
	int irq_base;
//#ifdef CONFIG_CHARGER_SM5702
//    sm5702_charger_platform_data_t *charger_platform_data;
//#endif
};

#define sm5702_mfd_platform_data_t \
	struct sm5702_mfd_platform_data

struct sm5702_charger_data;

struct sm5702_mfd_chip {
	struct i2c_client *i2c_client;
	struct device *dev;
	sm5702_mfd_platform_data_t *pdata;
	int irq;
	int irq_base;
	struct mutex io_lock;
	struct mutex irq_lock;
	struct mutex suspend_flag_lock;
	struct wake_lock irq_wake_lock;
	int irq_masks_cache[SM5702_IRQ_REGS_NR];
    
	int irq_status_index;
	sm5702_irq_status_t irq_status[SM5702_IRQ_REGS_NR];
	int suspend_flag;
	int pending_irq;

//#ifdef CONFIG_CHARGER_SM5702    
//	struct sm5702_charger_info *charger;
//#endif
};

#define sm5702_mfd_chip_t \
	struct sm5702_mfd_chip

extern int sm5702_block_read_device(struct i2c_client *i2c,
		int reg, int bytes, void *dest);

extern int sm5702_block_write_device(struct i2c_client *i2c,
		int reg, int bytes, const void *src);

extern int sm5702_reg_read(struct i2c_client *i2c, int reg_addr);
extern int sm5702_reg_write(struct i2c_client *i2c, int reg_addr, unsigned char data);
extern int sm5702_assign_bits(struct i2c_client *i2c, int reg_addr, unsigned char mask,
		unsigned char data);
extern int sm5702_set_bits(struct i2c_client *i2c, int reg_addr, unsigned char mask);
extern int sm5702_clr_bits(struct i2c_client *i2c, int reg_addr, unsigned char mask);
extern void sm5702_cf_dump_read(struct i2c_client *i2c);

typedef enum {
        SM5702_PREV_STATUS = 0,
        SM5702_NOW_STATUS } sm5702_irq_status_sel_t;

extern sm5702_irq_status_t *sm5702_get_irq_status(sm5702_mfd_chip_t *mfd_chip,
		sm5702_irq_status_sel_t sel);

#endif // SM5702_H
