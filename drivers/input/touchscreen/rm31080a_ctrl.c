/*
 * Raydium RM31080 touchscreen driver
 *
 * Copyright (C) 2012-2013, Raydium Semiconductor Corporation.
 * All Rights Reserved.
 * Copyright (C) 2012-2013, NVIDIA Corporation.  All Rights Reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
/*=============================================================================
	INCLUDED FILES
=============================================================================*/
#include <linux/device.h>
#include <linux/uaccess.h>	/* copy_to_user() */
#include <linux/delay.h>
#include <linux/module.h>	/* Module definition */

#include <linux/spi/rm31080a_ts.h>
#include <linux/spi/rm31080a_ctrl.h>

/*=============================================================================
	GLOBAL VARIABLES DECLARATION
=============================================================================*/
struct rm_tch_ctrl_para g_stCtrl;

/*=============================================================================
	FUNCTION DECLARATION
=============================================================================*/

/*=============================================================================
	Description:
			Control functions for Touch IC
	Input:

	Output:

=============================================================================*/
int rm_tch_ctrl_clear_int(void)
{
	u8 flag;
	if (g_stCtrl.bICVersion >= 0xD0)
		return rm_tch_spi_byte_read(0x72, &flag);
	else
		return rm_tch_spi_byte_read(0x02, &flag);
}

int rm_tch_ctrl_scan_start(void)
{
	return rm_tch_spi_byte_write(RM31080_REG_11, 0x17);
}

void rm_tch_ctrl_wait_for_scan_finish(void)
{
	u8 u8reg11;
	int i;
	/*50ms = 20Hz*/
	for (i = 0; i < 50; i++) {
		rm_tch_spi_byte_read(RM31080_REG_11, &u8reg11);
		if (u8reg11 & 0x01)
			usleep_range(1000, 2000);/* msleep(1); */
		else
			break;
	}
}

/*=============================================================================
	Description:

	Input:
			N/A
	Output:
			N/A
=============================================================================*/
void rm_set_repeat_times(u8 u8Times)
{
	u8 bReg1_1Fh = 0x00;

	rm_tch_spi_byte_write(0x0E, u8Times&0x1F);

	bReg1_1Fh &= ~FILTER_NONTHRESHOLD_MODE;

	if (u8Times != REPEAT_1)
		bReg1_1Fh |= 0x44;

	rm_tch_spi_byte_write(RM31080_REG_1F, bReg1_1Fh);
}

/*=============================================================================
	Description:

	Input:
			N/A
	Output:
			N/A
=============================================================================*/
void rm_tch_ctrl_init(void)
{
	memset(&g_stCtrl, 0, sizeof(struct rm_tch_ctrl_para));
}

/*=============================================================================
	Description: To transfer the value to HAL layer

	Input:
			N/A
	Output:
			N/A
=============================================================================*/
unsigned char rm_tch_ctrl_get_idle_mode(u8 *p)
{
	u32 u32Ret;
	u32Ret = copy_to_user(p, &g_stCtrl.bfIdleModeCheck, 1);
	if (u32Ret != 0)
		return 0;
	return 1;
}

/*=============================================================================
	Description:

	Input:
			N/A
	Output:
			N/A
=============================================================================*/
void rm_tch_ctrl_set_parameter(void *arg)
{
	memcpy(&g_stCtrl, arg, sizeof(struct rm_tch_ctrl_para));
}

/*===========================================================================*/
MODULE_AUTHOR("xxxxxxxxxx <xxxxxxxx@rad-ic.com>");
MODULE_DESCRIPTION("Raydium touchscreen control functions");
MODULE_LICENSE("GPL");
