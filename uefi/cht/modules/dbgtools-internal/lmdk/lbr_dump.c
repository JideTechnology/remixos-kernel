/*
 * lbr_dump.c
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <asm/msr.h>
#include <linux/cpumask.h>
#include <linux/kallsyms.h>

#include "lbr_dump.h"

static char *lbr_dump_buf;
static u32 lbr_buf_offset;

static int lbr_ctl_show(void *out, u64 *val)
{
	u32 cpu;
	u32 data[2];
	int err = 0;

	for_each_online_cpu(cpu) {
		err = rdmsr_safe_on_cpu(cpu, MSR_IA32_DEBUGCTLMSR,
					&data[0], &data[1]);
		MSR_CHECK_ERR(err);
		if (err)
			continue;
		pr_info("CORE%d LBR_CTL value H 0x%x, L 0x%x\n",
			cpu, data[1], data[0]);
		if (data[0] & DEBUGCTLMSR_LBR)
			pr_info(" Bit LBR set\n");
		if (data[0] & DEBUGCTLMSR_BTF)
			pr_info(" Bit BTF set\n");
		if (data[0] & DEBUGCTLMSR_TR)
			pr_info(" Bit TR set\n");
		if (data[0] & DEBUGCTLMSR_BTS)
			pr_info(" Bit BTS set\n");
		if (data[0] & DEBUGCTLMSR_BTINT)
			pr_info(" Bit BTINT set\n");
		if (data[0] & DEBUGCTLMSR_BTS_OFF_OS)
			pr_info(" Bit BTS_OFF_OS set\n");
		if (data[0] & DEBUGCTLMSR_BTS_OFF_USR)
			pr_info(" Bit BTS_OFF_USR set\n");
		if (data[0] & DEBUGCTLMSR_FREEZE_LBRS_ON_PMI)
			pr_info(" Bit FREEZE_LBRS_ON_PMI set\n");
		if (data[0] & DEBUGCTLMSR_FREEZE_LBRS_ON_PMI)
			pr_info(" Bit FREEZE_PERFMON_ON_PMI set\n");
		if (data[0] & DEBUGCTLMSR_ENABLE_UNCORE_PMI)
			pr_info(" Bit ENABLE_UNCORE_PMI set\n");
		if (data[0] & DEBUGCTLMSR_FREEZE_WHILE_SMM)
			pr_info(" Bit FREEZE_WHILE_SMM set\n");
	}
	*val = 0;
	return 0;
}

static int lbr_ctl(void *in, u64 val)
{
	u32 cpu;
	u32 data[2];
	int err = 0;

	for_each_online_cpu(cpu) {
		err = rdmsr_safe_on_cpu(cpu, MSR_IA32_DEBUGCTLMSR,
					&data[0], &data[1]);
		MSR_CHECK_ERR(err);
		/* Stop LBR if needed */
		if (data[0] & val & DEBUGCTLMSR_LBR) {
			err = wrmsr_safe_on_cpu(cpu, MSR_IA32_DEBUGCTLMSR,
					data[0] & ~DEBUGCTLMSR_LBR, data[1]);
			MSR_CHECK_ERR(err);
		}
		if (val & DEBUGCTLMSR_LBR) {
			err = wrmsr_safe_on_cpu(cpu, MSR_IA32_LASTINTTOIP,
				0, 0);
			MSR_CHECK_ERR(err);
		}
		if (val & DEBUGCTLMSR_LBR) {
			err = wrmsr_safe_on_cpu(cpu, MSR_IA32_LASTINTFROMIP,
				0, 0);
			MSR_CHECK_ERR(err);
		}
		err = wrmsr_safe_on_cpu(cpu, MSR_IA32_DEBUGCTLMSR,
				lower_32_bits(val),
				upper_32_bits(val));
		MSR_CHECK_ERR(err);
	}
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(lbr_ctl_fops, lbr_ctl_show, lbr_ctl,
				"See pr_err.%llu\n");

static void lbr_buf_write(char *str)
{
	int ret;
	if ((lbr_dump_buf) && (lbr_buf_offset < LBR_DUMP_BUF_SIZE)) {
		ret = snprintf(&lbr_dump_buf[lbr_buf_offset],
				LBR_DUMP_BUF_SIZE - lbr_buf_offset, "%s", str);
		lbr_buf_offset += ret;
		if (lbr_buf_offset > LBR_DUMP_BUF_SIZE)
			lbr_buf_offset = LBR_DUMP_BUF_SIZE;
	}
}

static void lbr_buf_write_symbol(u32 address)
{
	char str[KSYM_SYMBOL_LEN];

	snprintf(str, KSYM_SYMBOL_LEN, "[<%x>] ", address);
	lbr_buf_write(str);
#ifdef CONFIG_KALLSYMS
	sprint_symbol(str, address);
	lbr_buf_write(str);
#endif
	lbr_buf_write("\n");
}


static void lbr_create_line(u32 cpu, u8 index, char *str)
{
	u32 data[2];
	int err = 0, ret = 0;

	err = rdmsr_safe_on_cpu(cpu, MSR_LBR_CORE_TO + index,
				&data[0], &data[1]);
	MSR_CHECK_ERR(err);
	if (!err) {
		ret = snprintf(str, LBR_STRING_LEN, "  TO%d: ", index);
		lbr_buf_write(str);
		lbr_buf_write_symbol(data[0]);

		err = rdmsr_safe_on_cpu(cpu, MSR_LBR_CORE_FROM + index,
					&data[0], &data[1]);
		MSR_CHECK_ERR(err);
		if (!err) {
			snprintf(str, LBR_STRING_LEN - ret, "FROM%d: ", index);
			lbr_buf_write(str);
			lbr_buf_write_symbol(data[0]);
		}
	}
}


void lbr_dump(void)
{
	u8 index, loop;
	u32 cpu;
	u32 data[2];
	int err = 0;
	char str[LBR_STRING_LEN];

	lbr_buf_offset = 0;

	lbr_buf_write("LBR_DUMP\n");
	for_each_online_cpu(cpu) {
		snprintf(str, LBR_STRING_LEN, "Core %d\n", cpu);

		lbr_buf_write(str);
		err = rdmsr_safe_on_cpu(cpu, MSR_IA32_DEBUGCTLMSR,
					&data[0], &data[1]);
		MSR_CHECK_ERR(err);

		if (!err && (data[0] & DEBUGCTLMSR_LBR)) {
			err = wrmsr_safe_on_cpu(cpu, MSR_IA32_DEBUGCTLMSR,
					data[0] & ~DEBUGCTLMSR_LBR, data[1]);
			MSR_CHECK_ERR(err);
			lbr_buf_write("Stop LBR\n");
			pr_err(LOG_PREFIX "%s: LBR was enabled ! %d\n",
			       __func__, err);
		}

		err = rdmsr_safe_on_cpu(cpu, MSR_LBR_TOS, &data[0], &data[1]);
		if (!err) {
			index = data[0] & MSR_LBR_TOS_MASK;
			snprintf(str, LBR_STRING_LEN, "TOS %d\n", index);
			lbr_buf_write(str);
		} else {
			index = 0;
			snprintf(str, LBR_STRING_LEN, "TOS default %d\n",
				index);
			MSR_CHECK_ERR(err);
		}

		for (loop = 8; loop > 0; loop--)
			lbr_create_line(cpu, (loop + index) % 8, str);

		err = rdmsr_safe_on_cpu(cpu, MSR_IA32_LASTINTTOIP,
					&data[0], &data[1]);
		if (!err) {
			snprintf(str, LBR_STRING_LEN, "LER_TO_LIP: ");
			lbr_buf_write(str);
			lbr_buf_write_symbol(data[0]);
		}
		err = rdmsr_safe_on_cpu(cpu, MSR_IA32_LASTINTFROMIP,
					&data[0], &data[1]);
		if (!err) {
			snprintf(str, LBR_STRING_LEN, "LER_FROM_LIP: ");
			lbr_buf_write(str);
			lbr_buf_write_symbol(data[0]);
		}
	}
}

static void lbr_init(void)
{
	lbr_dump_buf = kmalloc(LBR_DUMP_BUF_SIZE, GFP_KERNEL);
	if (!lbr_dump_buf)
		pr_err(LOG_PREFIX "%s: no memory.\n", __func__);
	else
		lbr_dump();

	lbr_ctl(NULL, DEBUGCTLMSR_LBR);
}

static void lbr_free(void)
{
	if (!lbr_dump_buf) {
		kfree(lbr_dump_buf);
		lbr_dump_buf = 0;
	}
}

static ssize_t lbr_read(struct file *fp, char __user *user_buf,
				size_t cnt, loff_t *offset)
{
	return simple_read_from_buffer(user_buf, cnt, offset, lbr_dump_buf,
					lbr_buf_offset);
}

static ssize_t lbr_write(struct file *fp, const char __user *user_buf,
			size_t cnt, loff_t *offset)
{
	if (cnt > 0)
		lbr_dump();
	return cnt;
}
static const struct file_operations lbr_dbg_fops = {
	.write	= lbr_write,
	.read	= lbr_read,
};

struct dentry *lbr_dbg_dir;

int lbr_dump_dbg_init(void)
{
	int err = 0;

	lbr_init();

	lbr_dbg_dir = debugfs_create_dir("lbr_dump", NULL);
	if (!lbr_dbg_dir) {
		pr_err(LOG_PREFIX "%s: debugfs dir failed\n", __func__);
		return -ENOMEM;
	}
	debugfs_create_file("lbr_dump", S_IRUGO | S_IWUSR,
			    lbr_dbg_dir, NULL, &lbr_dbg_fops);
	debugfs_create_file("lbr_ctl", S_IRUGO | S_IWUSR,
			    lbr_dbg_dir, NULL, &lbr_ctl_fops);

	return err;
}

void lbr_dump_dbg_exit(void)
{
	debugfs_remove_recursive(lbr_dbg_dir);

	lbr_free();
}


