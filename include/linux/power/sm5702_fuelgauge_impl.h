/*
 * drivers/battery/sm5702_fuelgauge-impl.h
 *
 * Header of Richtek sm5702 Fuelgauge Driver Implementation
 *
 * Copyright (C) 2013 SiliconMitus
 * Author: SW Jung
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef sm5702_FUELGAUGE_IMPL_H
#define sm5702_FUELGAUGE_IMPL_H

/* Definitions of sm5702 Fuelgauge Registers */
/* I2C Register */
#define sm5702_REG_DEVICE_ID				 0x00
#define sm5702_REG_CNTL						 0x01
#define sm5702_REG_INTFG					 0x02
#define sm5702_REG_INTFG_MASK				 0x03
#define sm5702_REG_STATUS					 0x04
#define sm5702_REG_SOC						 0x05
#define sm5702_REG_OCV						 0x06
#define sm5702_REG_VOLTAGE					 0x07
#define sm5702_REG_CURRENT					 0x08
#define sm5702_REG_TEMPERATURE				 0x09
/* #define sm5702_REG_FG_OP_STATUS			   0x10 //diff */

#define sm5702_REG_V_ALARM					 0x0C
#define sm5702_REG_T_ALARM					 0x0D
#define sm5702_REG_SOC_ALARM				 0x0E

/* #define sm5702_REG_TOPOFFSOC				   0x12 //diff */
#define sm5702_REG_PARAM_CTRL				 0x13
#define sm5702_REG_PARAM_RUN_UPDATE			 0x14
#define sm5702_REG_VIT_PERIOD				 0x1A
#define sm5702_REG_MIX_RATE					 0x1B
#define sm5702_REG_MIX_INIT_BLANK			 0x1C

/* for cal */
/* #define sm5702_REG_VOLT_CAL		  0x2B //diff */
#define sm5702_REG_CURR_CAL			0x8D


#define sm5702_REG_RCE0				0x20
#define sm5702_REG_RCE1				0x21
#define sm5702_REG_RCE2				0x22
#define sm5702_REG_DTCD				0x23
#define sm5702_REG_RS				0x24

#define sm5702_REG_RS_MIX_FACTOR	0x25
#define sm5702_REG_RS_MAX			0x26
#define sm5702_REG_RS_MIN			0x27
#define sm5702_REG_RS_MAN			0x29


#define sm5702_REG_RS_TUNE			0x28
#define sm5702_REG_CURRENT_EST		0x85
#define sm5702_REG_CURRENT_ERR		0x86
#define sm5702_REG_Q_EST			0x87





#define sm5702_FG_PARAM_UNLOCK_CODE	  0x3700
#define sm5702_FG_PARAM_LOCK_CODE	  0x0000

#define sm5702_FG_TABLE_LEN	   0xF /* real table length */

/* start reg addr for table */
#define sm5702_REG_TABLE_START	0xA0

#define RS_MAN_CNTL				0x0800

/* control register value */
#define ENABLE_MIX_MODE			0x8000
#define ENABLE_TEMP_MEASURE		0x4000
#define ENABLE_TOPOFF_SOC		0x2000
#define ENABLE_SOC_ALARM		0x0008
#define ENABLE_T_H_ALARM		0x0004
#define ENABLE_T_L_ALARM		0x0002
#define ENABLE_V_ALARM			0x0001
#define ENABLE_RS_MAN_MODE  	0x0800

#define ENABLE_RE_INIT			0x0008

#define ENABLE_L_SOC_INT  0x0008
#define ENABLE_H_TEM_INT  0x0004
#define ENABLE_L_TEM_INT  0x0002
#define ENABLE_L_VOL_INT  0x0001

#define FULL_SOC						100

#endif /* sm5702_FUELGAUGE_IMPL_H */

