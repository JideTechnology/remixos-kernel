/*
 * drivers/battery/sm5702_charger_oper.h
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

#include <linux/init.h>
#include <linux/mfd/sm5702.h>

enum {
	SM5702_CHARGER_OP_MODE_SUSPEND              = 0x0,
	SM5702_CHARGER_OP_MODE_CHG_OFF              = 0x4,
	SM5702_CHARGER_OP_MODE_CHG_ON               = 0x5,
	SM5702_CHARGER_OP_MODE_FLASH_BOOST          = 0x6,
	SM5702_CHARGER_OP_MODE_USB_OTG              = 0x7,
};

enum SM5702_CHARGER_OP_EVENT_TYPE {
	SM5702_CHARGER_OP_EVENT_VBUS                = 0x3,
	SM5702_CHARGER_OP_EVENT_FLASH               = 0x2,
	SM5702_CHARGER_OP_EVENT_TORCH               = 0x1,
	SM5702_CHARGER_OP_EVENT_OTG                 = 0x0,
};

#define make_OP_STATUS(vbus,flash,torch,otg)   (((vbus & 0x1) << SM5702_CHARGER_OP_EVENT_VBUS)         | \
                                                 ((flash & 0x1) << SM5702_CHARGER_OP_EVENT_FLASH)       | \
                                                 ((torch & 0x1) << SM5702_CHARGER_OP_EVENT_TORCH)       | \
                                                 ((otg & 0x1) << SM5702_CHARGER_OP_EVENT_OTG))

int sm5702_charger_oper_push_event(int event_type, bool enable);
int sm5702_charger_oper_table_init(struct i2c_client *i2c);

int sm5702_charger_oper_get_current_status(void);
int sm5702_charger_oper_get_current_op_mode(void);

/* force transation status controls */
int sm5702_charger_oper_enter_force_trans_mode(int mode);
int sm5702_charger_oper_exit_force_trans_mode(void);
int sm5702_charger_oper_is_force_trans_mode(void);
