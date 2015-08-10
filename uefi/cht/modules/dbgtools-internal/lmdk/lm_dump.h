/*
 * lm_dump.h
 *
 * Copyright (C) 2013 Intel Corp
 * Author: jeremy.rocher@intel.com
 *
 * Lakemore dump driver : retrieve Lakemore buffer informations and
 * pass them to emmc_ipanic driver, which save buffer in emmc.
 *
 * -----------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#ifndef _LM_DUMP
#define _LM_DUMP

struct hw_soc_diff {
	unsigned int lm_port_id;
	unsigned int visa_port_id;
};

/*
 * Generic defines (base was VLV2)
 */

/*
 * SB base registers :
 *  - MCR : Message Control Register
 *  - MCRE : Message Control Register Extended
 *  - MDR : Message Data Register
 *
 * Offsets and read/write functions
 */
#define MSG_CTRL_REG_OFFSET 0xD0
#define MSG_DATA_REG_OFFSET 0xD4
#define MSG_CTRL_REG_EXT_OFFSET 0xD8

/*
 * Lakemore registers offsets
 */
#define LM_PORT_ID 0x17

#define LM_STPCTL 0x1004
#define LM_OSMC 0x2004
#define LM_OSMC_SWSUSP (1<<13)
#define LM_OSMC_SWSTOP (1<<11)
#define LM_MEMWRPNT 0x2028
#define LM_STORMEMBAR_L 0x201C
#define LM_STORMEMBAR_H 0x2020
#define LM_OSTAT 0x2014
#define LM_OSTAT_ADDROVRFLW (1<<5)
#define LM_MEMDEPTH 0x2024

/*
 * DebugFS Blobs
 */
#define LKM_BLOB_NUMBER		2
#define LKM_BLOB_FILE_NAME	"lkm_buf"
#define LKM_BLOB_FILE_NAME_LEN	8

/*
 * VISA registers offsets
 */
#define VISA_PORT_ID 0x18

#define VISA_CTRL_ADDR 0x0
#define VISA_CTRL_DATA 0x4
#define VISA_CTRL_PHY0 0x8

/*
 * Defines specific to TNG
 */

#define TNG_LM_PORT_ID		0x19
#define TNG_VISA_PORT_ID	0x1A

/*
 * Defines specific to ANN
 */

#define ANN_LM_PORT_ID		0x22
#define ANN_VISA_PORT_ID	0x23

#endif	/* _LM_DUMP */
