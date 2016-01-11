/*
 * drivers/battery/sm5702_charger_oper.c
 *
 * SM5702 Charger Operation Mode controller
 *
 * Copyright (C) 2015 Siliconmitus Technology Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/power/charger/sm5702_charger.h>
#include <linux/power/charger/sm5702_charger_oper.h>
#include <linux/mfd/sm5702.h>

enum {
	BST_OUT_4000mV              = 0x0,
	BST_OUT_4100mV              = 0x1,
	BST_OUT_4200mV              = 0x2,
	BST_OUT_4300mV              = 0x3,
	BST_OUT_4400mV              = 0x4,
	BST_OUT_4500mV              = 0x5,
	BST_OUT_4600mV              = 0x6,
	BST_OUT_4700mV              = 0x7,
	BST_OUT_4800mV              = 0x8,
	BST_OUT_4900mV              = 0x9,
	BST_OUT_5000mV              = 0xA,
	BST_OUT_5100mV              = 0xB,
};

//#define SM5702_OPERATION_MODE_MASK  0x07
//#define SM5702_BSTOUT_MASK          0x0F

struct sm5702_charger_oper_table_info {
	unsigned char status;
	unsigned char oper_mode;
	unsigned char BST_OUT;
};

struct sm5702_charger_oper_info {
	struct i2c_client *i2c;

	int max_table_num;
	struct sm5702_charger_oper_table_info current_table;

    bool is_force_trans_mode;
    unsigned char save_status;
};
static struct sm5702_charger_oper_info oper_info;

/**
 *  (VBUS in/out) (FLASH on/off) (TORCH on/off) (OTG cable in/out)
 **/
static struct sm5702_charger_oper_table_info sm5702_charger_operation_mode_table[] = {
	/* Charger mode : Charging ON */
	{ make_OP_STATUS(0,0,0,0), SM5702_CHARGER_OP_MODE_CHG_OFF, BST_OUT_4500mV},
	{ make_OP_STATUS(1,0,0,0), SM5702_CHARGER_OP_MODE_CHG_ON, BST_OUT_4500mV},
	{ make_OP_STATUS(1,0,1,0), SM5702_CHARGER_OP_MODE_CHG_ON, BST_OUT_4500mV},
	/* Charger mode : Flash Boost */
	{ make_OP_STATUS(0,1,0,0), SM5702_CHARGER_OP_MODE_FLASH_BOOST, BST_OUT_4500mV},
	{ make_OP_STATUS(0,1,0,1), SM5702_CHARGER_OP_MODE_FLASH_BOOST, BST_OUT_4500mV},
	{ make_OP_STATUS(1,1,0,0), SM5702_CHARGER_OP_MODE_FLASH_BOOST, BST_OUT_4500mV},
	{ make_OP_STATUS(0,0,1,0), SM5702_CHARGER_OP_MODE_FLASH_BOOST, BST_OUT_4500mV},
	/* Charger mode : USB OTG */
	{ make_OP_STATUS(0,0,1,1), SM5702_CHARGER_OP_MODE_USB_OTG, BST_OUT_5100mV},
	{ make_OP_STATUS(0,0,1,0), SM5702_CHARGER_OP_MODE_USB_OTG, BST_OUT_5100mV},
	{ make_OP_STATUS(0,0,0,1), SM5702_CHARGER_OP_MODE_USB_OTG, BST_OUT_5100mV},
	{ make_OP_STATUS(0,0,0,0), SM5702_CHARGER_OP_MODE_USB_OTG, BST_OUT_5100mV},
};

/**
 * SM5702 Charger operation mode controller relative I2C setup
 */

static int sm5702_charger_oper_set_mode(struct i2c_client *i2c, unsigned char mode)
{
	return sm5702_assign_bits(i2c, SM5702_CNTL, SM5702_OPERATION_MODE_MASK, mode << SM5702_OPERATION_MODE_SHIFT);
}

static int sm5702_charger_oper_set_BSTOUT(struct i2c_client *i2c, unsigned char BSTOUT)
{
	return sm5702_assign_bits(i2c, SM5702_FLEDCNTL6, SM5702_BSTOUT_MASK, BSTOUT << SM5702_BSTOUT_SHIFT);
}

/**
 * SM5702 Charger operation mode controller API functions.
 */

static inline unsigned char _update_status(int event_type, bool enable, unsigned char base_status)
{
	if (event_type > SM5702_CHARGER_OP_EVENT_VBUS) {
		return base_status;
	}

	if (enable) {
		return (base_status | (1 << event_type));
	} else {
		return (base_status & ~(1 << event_type));
	}
}

static inline void sm5702_charger_oper_change_state(unsigned char new_status)
{
	int i;

	for (i=0; i < oper_info.max_table_num; ++i) {
		if (new_status == sm5702_charger_operation_mode_table[i].status) {
			break;
		}
	}
	if (i == oper_info.max_table_num) {
		pr_err("sm5702-charger: %s: can't find matched Charger Operation Mode Table (status = 0x%x)\n", __func__, new_status);
		return;
	}

	if (sm5702_charger_operation_mode_table[i].BST_OUT != oper_info.current_table.BST_OUT) {
		sm5702_charger_oper_set_BSTOUT(oper_info.i2c, sm5702_charger_operation_mode_table[i].BST_OUT);
		oper_info.current_table.BST_OUT = sm5702_charger_operation_mode_table[i].BST_OUT;
	}

	/* USB_OTG to CHG_ON work-around for BAT_REG stabilize */
	if (oper_info.current_table.oper_mode == SM5702_CHARGER_OP_MODE_USB_OTG && \
		sm5702_charger_operation_mode_table[i].oper_mode == SM5702_CHARGER_OP_MODE_CHG_ON) {
		pr_info("sm5702-charger: %s: trans op_mode:suspend for BAT_REG stabilize (time=100ms)\n", __func__);
		sm5702_charger_oper_set_mode(oper_info.i2c, SM5702_CHARGER_OP_MODE_SUSPEND);
		msleep(100);
	}

	if (sm5702_charger_operation_mode_table[i].oper_mode != oper_info.current_table.oper_mode) {
		sm5702_charger_oper_set_mode(oper_info.i2c, sm5702_charger_operation_mode_table[i].oper_mode);
		oper_info.current_table.oper_mode = sm5702_charger_operation_mode_table[i].oper_mode;
	}
	oper_info.current_table.status = new_status;

	pr_info("sm5702-charger: %s: New table[%d] info (STATUS: 0x%x, MODE: %d, BST_OUT: 0x%x\n", \
			__func__, i, oper_info.current_table.status, oper_info.current_table.oper_mode, oper_info.current_table.BST_OUT);
}

int sm5702_charger_oper_push_event(int event_type, bool enable)
{
	unsigned char new_status;

	if (oper_info.i2c == NULL) {
		pr_err("sm5702-charger: %s: required sm5702 charger operation table initialize\n", __func__);
		return -ENOENT;
	}

	pr_info("sm5702-charger: %s: event_type=%d, enable=%d\n", __func__, event_type, enable);

    if (oper_info.is_force_trans_mode) {
        pr_info("sm5702-charger: %s: skip to change state, now force trans mode\n", __func__);
    	new_status = _update_status(event_type, enable, oper_info.save_status);
    	if (new_status == oper_info.save_status) {
    		goto out;
    	}
        oper_info.save_status = new_status;
    } else {
    	new_status = _update_status(event_type, enable, oper_info.current_table.status);
    	if (new_status == oper_info.current_table.status) {
    		goto out;
    	}
        sm5702_charger_oper_change_state(new_status);
    }

out:
	return 0;
}
EXPORT_SYMBOL(sm5702_charger_oper_push_event);

int sm5702_charger_oper_table_init(struct i2c_client *i2c)
{
	if (i2c == NULL) {
		pr_err("sm5702-charger: %s: invalid i2c client handler=n", __func__);
		return -EINVAL;
	}
	oper_info.i2c = i2c;
    oper_info.is_force_trans_mode = 0;

	/* set default operation mode condition */
	oper_info.max_table_num = ARRAY_SIZE(sm5702_charger_operation_mode_table);
	oper_info.current_table.status = make_OP_STATUS(0, 0, 0, 0);
	oper_info.current_table.oper_mode = SM5702_CHARGER_OP_MODE_CHG_ON;
	oper_info.current_table.BST_OUT = BST_OUT_4500mV;

	sm5702_charger_oper_set_mode(oper_info.i2c, oper_info.current_table.oper_mode);
	sm5702_charger_oper_set_BSTOUT(oper_info.i2c, oper_info.current_table.BST_OUT);

	pr_info("sm5702-charger: %s: current table info (STATUS: 0x%x, MODE: %d, BST_OUT: 0x%x\n", \
			__func__, oper_info.current_table.status, oper_info.current_table.oper_mode, oper_info.current_table.BST_OUT);

	return 0;
}
EXPORT_SYMBOL(sm5702_charger_oper_table_init);

int sm5702_charger_oper_get_current_status(void)
{
	return oper_info.current_table.status;
}
EXPORT_SYMBOL(sm5702_charger_oper_get_current_status);

int sm5702_charger_oper_get_current_op_mode(void)
{
	return oper_info.current_table.oper_mode;
}
EXPORT_SYMBOL(sm5702_charger_oper_get_current_op_mode);


/* force transation status controls */
int sm5702_charger_oper_enter_force_trans_mode(int mode)
{
    if (oper_info.i2c == NULL) {
        return -ENODEV;
    }

    oper_info.is_force_trans_mode = 1;
    oper_info.save_status = oper_info.current_table.status;
    sm5702_charger_oper_set_mode(oper_info.i2c, mode);

    pr_info("sm5702-charger: %s: trans mode=%d\n", __func__, mode);

    return 0;
}
EXPORT_SYMBOL(sm5702_charger_oper_enter_force_trans_mode);

int sm5702_charger_oper_exit_force_trans_mode(void)
{
    if (oper_info.is_force_trans_mode == 0) {
        return 0;
    }

    if (oper_info.i2c == NULL) {
        return -ENODEV;
    }

    sm5702_charger_oper_set_mode(oper_info.i2c, oper_info.current_table.oper_mode);

    if (oper_info.save_status != oper_info.current_table.status) {
        pr_info("sm5702-charger: %s: restore backup status - %d\n", __func__, oper_info.save_status);
        sm5702_charger_oper_change_state(oper_info.save_status);
    }

    oper_info.is_force_trans_mode = 0;
    pr_info("sm5702-charger: %s: done.\n", __func__);

    return 0;
}
EXPORT_SYMBOL(sm5702_charger_oper_exit_force_trans_mode);

int sm5702_charger_oper_is_force_trans_mode(void)
{
    return oper_info.is_force_trans_mode;
}
EXPORT_SYMBOL(sm5702_charger_oper_is_force_trans_mode);
