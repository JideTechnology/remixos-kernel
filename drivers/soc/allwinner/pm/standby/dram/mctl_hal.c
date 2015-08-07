/********************************************************************************
 * Allwinner Technology, All Right Reserved. 2006-2010 Copyright (c)
 *
 * File: mctl_hal.c
 *
 * Description:  This file implements standby for AW1699 DRAM controller
 *
 * History:
 *
 * date		 author		version		note
 *
 * 20150731	lhk		   v0.1		 initial
 *
 *
 *
 *
 ********************************************************************************/
//#include "include.h"
#include <linux/types.h>
#include <linux/power/aw_pm.h>
#include "mctl_reg.h"

#define STANDBY_FPGA

#ifdef STANDBY_FPGA
/********************************************************************************
 *FPGA standby code
 ********************************************************************************/
static unsigned int __dram_power_save_process(void)
{
	unsigned int reg_val =0;

	mctl_write_w(0,MC_MAER);
	//1.enter self refresh
	reg_val = mctl_read_w(PWRCTL);
	reg_val |= 0x1<<0;
	reg_val |= (0x1<<8);
	mctl_write_w(reg_val,PWRCTL);
	//confirm dram controller has enter selfrefresh
	while(((mctl_read_w(STATR)&0x7) != 0x3));

	/* 3.pad hold */
	reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);
	reg_val |= 0x3<<0;
	mctl_write_w(reg_val,VDD_SYS_PWROFF_GATING);

	return 0;
}

static unsigned int __dram_power_up_process(dram_para_t *para)
{
	unsigned int reg_val =0;
	/* 1.pad release */
	reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);
	reg_val &= ~(0x3<<0);
	mctl_write_w(reg_val,VDD_SYS_PWROFF_GATING);
	/*2.exit self refresh */
	reg_val = mctl_read_w(PWRCTL);
	reg_val &= ~(0x1<<0);
	reg_val &= ~(0x1<<8);
	mctl_write_w(reg_val,PWRCTL);
	//confirm dram controller has enter selfrefresh
	while(((mctl_read_w(STATR)&0x7) != 0x1));
	/*3.enable master access */
	mctl_write_w(0xffffffff,MC_MAER);
	return 0 ;
}
unsigned int dram_power_save_process(void)
{
	 __dram_power_save_process();
	 return 0;
}

unsigned int dram_power_up_process(dram_para_t *dram_para)
{
	__dram_power_up_process(dram_para);

	return 0;
}
#else
/********************************************************************************
 *IC standby code
 ********************************************************************************/
#endif