/*
 * lbr_dump.h
 *
 * Copyright (C) 2013 Intel Corp
 * Author: laurent.josse@intel.com
 *
 * LBR dump driver : enable/dump LBR informations
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

#ifndef _LBR_DUMP
#define _LBR_DUMP

#define pr_fmt(fmt) fmt
#define LOG_PREFIX "lbr_dump:"

#define LBR_DUMP_BUF_SIZE		(16*1024)
#define LBR_STRING_LEN			64

#define MSR_CHECK_ERR(val)						\
	do {								\
		if (val)						\
			pr_err(" %d: error %d\n", __LINE__, val);	\
	} while (0)

#define MSR_LBR_TOS_MASK			0x00000007
#define DEBUGCTLMSR_FREEZE_PERFMON_ON_PMI	(1UL << 12)
#define DEBUGCTLMSR_ENABLE_UNCORE_PMI		(1UL << 13)
#define DEBUGCTLMSR_FREEZE_WHILE_SMM		(1UL << 14)

#endif /* _LBR_DUMP */

