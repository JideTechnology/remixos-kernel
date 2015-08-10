/*
 * lm_dump.c
 *
 * Copyright (C) 2013-2015 Intel Corp
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <asm/intel-mid.h>

#define LM_DEBUG

#include "lm_dump.h"
#include "lbr_dump.c"

#undef pr_fmt
#define pr_fmt(fmt) "lmlog:%s: " fmt, __func__

struct lm_ctx {
	struct pci_dev *dev;
	phys_addr_t buf_addr;
	unsigned char buf_bios;
	unsigned long buf_size;
	void __iomem *buf_base;
};

static struct lm_ctx *ctx;

static struct hw_soc_diff *soc_diff;

static unsigned int lkm_buf_size;

module_param(lkm_buf_size, uint, 0444);
MODULE_PARM_DESC(lkm_buf_size,
"Size (in kB), of the buffer to be allocated.");

static inline void sb_pci_init(void)
{
	unsigned int reg;

	reg = inl(0xCF8);
	reg &= 0x7f000003;
	reg |= 0x80000000;
	outl(0xCF8, reg);
}

/*
 * SideBand base registers (MCR, MCRE, MDR) direct access read/write
 * functions
 */
static inline int mcr_write(struct pci_dev *dev, u32 val)
{
	return pci_write_config_dword(dev, MSG_CTRL_REG_OFFSET, val);
}

static inline int mcr_read(struct pci_dev *dev, u32 *val)
{
	return pci_read_config_dword(dev, MSG_CTRL_REG_OFFSET, val);
}

static inline int mcre_write(struct pci_dev *dev, u32 val)
{
	return pci_write_config_dword(dev, MSG_CTRL_REG_EXT_OFFSET, val);
}

static inline int mcre_read(struct pci_dev *dev, u32 *val)
{
	return pci_read_config_dword(dev, MSG_CTRL_REG_EXT_OFFSET, val);
}

static inline int mdr_write(struct pci_dev *dev, u32 val)
{
	return pci_write_config_dword(dev, MSG_DATA_REG_OFFSET, val);
}

static inline int mdr_read(struct pci_dev *dev, u32 *val)
{
	return pci_read_config_dword(dev, MSG_DATA_REG_OFFSET, val);
}

/*
 * SB indirect register access read/write functions
 */
static int sb_reg_read(struct pci_dev *dev, u8 port_id, u32 reg, u32 *val)
{
	mcre_write(dev, reg & 0xFFFFFF00);
	mcr_write(dev, 0 | (port_id << 16) | ((reg & 0x000000FF) << 8) | 0xF0);
	mdr_read(dev, val);

	return 0;
}

static int sb_reg_write(struct pci_dev *dev, u8 port_id, u32 reg, u32 val)
{
	mcre_write(dev, reg & 0xFFFFFF00);
	mdr_write(dev, val);
	mcr_write(dev, 0x01000000 | (port_id << 16) |
		  ((reg & 0x000000FF) << 8) | 0xF0);

	return 0;
}

/*
 * Lakemore high level functions
 */
static void lm_stop(struct pci_dev *dev)
{
	u32 osmc_data;

	sb_reg_read(dev, soc_diff->lm_port_id, LM_OSMC, &osmc_data);
	sb_reg_write(dev, soc_diff->lm_port_id, LM_OSMC,
		     osmc_data | LM_OSMC_SWSUSP | LM_OSMC_SWSTOP);
}

static u32 lm_get_woff(struct pci_dev *dev)
{
	u32 woff;

	sb_reg_read(dev, soc_diff->lm_port_id, LM_MEMWRPNT, &woff);

	return woff;
}

static phys_addr_t lm_get_base_addr(struct pci_dev *dev)
{
	phys_addr_t base_addr = 0;
	u32 addr_l;
#ifdef CONFIG_PHYS_ADDR_T_64BIT
	u32 addr_h;
#endif

	sb_reg_read(dev, soc_diff->lm_port_id, LM_STORMEMBAR_L, &addr_l);

#ifdef CONFIG_PHYS_ADDR_T_64BIT
	sb_reg_read(dev, soc_diff->lm_port_id, LM_STORMEMBAR_H, &addr_h);
	base_addr = addr_h;
	base_addr = (base_addr << 32) | addr_l;
#else
	base_addr = addr_l;
#endif

	return base_addr;
}

#define LM_SIZE_COEFF (1024 * 8)
#define LM_MAX_SIZE (ULONG_MAX / LM_SIZE_COEFF)
static unsigned long lm_get_size(struct pci_dev *dev)
{
	unsigned long size = 0;
	u32 m_depth;

	sb_reg_read(dev, soc_diff->lm_port_id, LM_MEMDEPTH, &m_depth);
	size = m_depth & 0x000FFFFF;
	if (unlikely(size > LM_MAX_SIZE)) {
		pr_err("lm buffer size > LM_MAX_SIZE (%lu > %lu)\n",
		       size, LM_MAX_SIZE);
		size = LM_MAX_SIZE;
	}
	size *= LM_SIZE_COEFF;

	return size;
}

static bool lm_has_overflow(struct pci_dev *dev)
{
	u32 ostat_data;

	sb_reg_read(dev, soc_diff->lm_port_id, LM_OSTAT, &ostat_data);

	return (bool)(ostat_data & LM_OSTAT_ADDROVRFLW);
}

/*
 * Called by panic handler at early stage, prepare Lakemore buffer
 * for post reset dump.
 */
static int panic_lmlog_setup_call(struct notifier_block *this,
				unsigned long event,
				void *ptr)
{
	size_t lm_buf_woff;
	size_t lm_buf_size;
	unsigned char * lm_buf_base;

	pr_debug("LM panic notifier entry\n");

	if (!ctx) {
		pr_err("ctx not available, exit\n");
		goto out;
	}

	sb_pci_init();

	/* Stop Lakemore module */
	lm_stop(ctx->dev);

	/* Read buffer size */
	lm_buf_size = (size_t)ctx->buf_size;
	pr_debug("buf size 0x%zx\n", lm_buf_size);

	/* Provide buffer base address */
	lm_buf_base = (unsigned char *)ctx->buf_base;
	pr_debug("buf virt addr 0x%p\n", lm_buf_base);

	/* Read buffer write offset */
	lm_buf_woff = (size_t)lm_get_woff(ctx->dev);
	pr_debug("buf woff 0x%zx\n", lm_buf_woff);

	/* 
	 * When PDM option is enabled in BIOS menu, it allocates a buffer whose size
	 * is twice the requested one. This allows to have 2 contiguous buffers
	 * with the same requested size:
	 * - buf0 is used at runtime to store the logs.
	 * - buf1 is used after a reset to backup a copy of the prior to reset buf0.
	 *
	 * The reset makes lose the MEMWRPNT. So, to allow the decoding of the
	 * buffer by PDT, it is required to prepare the buffer for the dump.
	 * Let's use buf1 as temporary buffer to re-arrange buf0 if overflowed.
	 * If there was an overflow, the older logs are at the end of the buffer,
	 * from MEMWRPNT (woff) to the end of the buffer. They must be placed
	 * before the more recent logs to allow the decoding by PDT.
	 */
	if (lm_has_overflow(ctx->dev)) {
		/* copy end of buf0 (from MEMWRPNT) to beginning of buf1 */
		memcpy((void *)(lm_buf_base + lm_buf_size),
			   (void *)(lm_buf_base + lm_buf_woff),
			   lm_buf_size - lm_buf_woff);
		/* copy beginning of buf0 (up to MEMWRPNT) to buf1 after previous data*/
		memcpy((void *)(lm_buf_base + 2 * lm_buf_size - lm_buf_woff),
			   (void *)(lm_buf_base),
			   lm_buf_woff);
		/* copy back buf1 to buf0 */
		memcpy((void *)(lm_buf_base),
			   (void *)(lm_buf_base + lm_buf_size),
			   lm_buf_size);
	}
	else {
		/*
		 * fill unused part of the buffer with harmless data for the host
		 * decoding tool, and that must be 0xFF.
		 */
		memset((void *)(lm_buf_base + lm_buf_woff), 0xFF,
			   lm_buf_size - lm_buf_woff);
	}

	pr_info("LM buffer prepared successfully\n");

out:
	return NOTIFY_DONE;
}

/*
 * debugfs functions
 */

static int lkm_dbg_get(void *data, u64 *val)
{
	size_t lm_buf_woff;

	sb_pci_init();

	pr_info("Buf addr=0x%llx\n", lm_get_base_addr(ctx->dev));
	pr_info("Buf size=0x%08lx\n", lm_get_size(ctx->dev));

	lm_buf_woff = (size_t)lm_get_woff(ctx->dev);

	pr_info("dd if=/d/lm_dump/lkm_buf0 ibs=1 count=%zu of=/logs/a\n",
		lm_buf_woff);
	if (lm_has_overflow(ctx->dev)) {
		pr_info("overflow occured\n");
		pr_info(
"dd if=/d/lm_dump/lkm_buf0 ibs=1 skip=%zu count=%lu of=/logs/b\n",
			lm_buf_woff, ctx->buf_size - lm_buf_woff);
	}

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(lkm_dbg_fops, lkm_dbg_get, NULL, "%llu\n");

static int lkm_stop(void *data, u64 val)
{
	pr_info("stop\n");

	sb_pci_init();
	lm_stop(ctx->dev);

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(lkm_stop_fops, NULL, lkm_stop, "%llu\n");

#ifdef LM_DEBUG

static int lkm_clear(void *data, u64 val)
{
	pr_info("clear\n");

	sb_pci_init();

	sb_reg_write(ctx->dev, soc_diff->lm_port_id, LM_STPCTL, 0x00000000);
	sb_reg_write(ctx->dev, soc_diff->lm_port_id, LM_OSMC, 0x008D6000);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(lkmcl_dbg_fops, NULL, lkm_clear, "%llu\n");

static int dbg_osmc_r(void *data, u64 *val)
{
	u32 rval;

	sb_reg_read(ctx->dev, soc_diff->lm_port_id, LM_OSMC, &rval);
	*val = rval;
	pr_info("OSMC : 0x%X\n", rval);

	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(dbg_osmc_fops, dbg_osmc_r, NULL, "OSMC 0x%llx\n");

static int dbg_ostat_r(void *data, u64 *val)
{
	u32 rval;

	sb_reg_read(ctx->dev, soc_diff->lm_port_id, LM_OSTAT, &rval);
	*val = rval;
	pr_info("OSTAT : 0x%X\n", rval);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(dbg_ostat_fops, dbg_ostat_r, NULL, "OSTAT 0x%llx\n");

static int dbg_memwrpnt_r(void *data, u64 *val)
{
	u32 rval;

	rval = lm_get_woff(ctx->dev);
	*val = rval;
	pr_info("MEMWRPNT : 0x%X\n", rval);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(dbg_memwrpnt_fops, dbg_memwrpnt_r, NULL,
			"MEMWRTPTR 0x%llx\n");

#endif /* LM_DEBUG */

/*
 * VISA Low level functions
 */
static u8 visa_replay_cnt;

static void visa_replay_reg_write(struct pci_dev *dev, u16 offset, u32 data)
{
	u32 ctrladdr;

	ctrladdr = ((offset & 0xFFFC) >> 2) | (visa_replay_cnt << 16) |
		(1 << 15);
	sb_reg_write(dev, soc_diff->visa_port_id, VISA_CTRL_ADDR, ctrladdr);
	sb_reg_write(dev, soc_diff->visa_port_id, VISA_CTRL_DATA, data);
	visa_replay_cnt += 1;
}

static void visa_replay_trig_en(struct pci_dev *dev, bool force, bool trig_en)
{
	u32 phy0;

	phy0 = ((visa_replay_cnt & 0xff) << 8) | (force << 27) |
		(trig_en << 16);
	sb_reg_write(dev, soc_diff->visa_port_id, VISA_CTRL_PHY0, phy0);
}

struct dentry *lm_dbg_dir;

struct debugfs_blob_wrapper lkm_blob[LKM_BLOB_NUMBER];

static void lkm_create_blob(u8 index)
{
	char blob_name[LKM_BLOB_FILE_NAME_LEN + 4];

	if ((index >= 1) && (!ctx->buf_bios))
		return;

	pr_info("index %d\n", index);

	if (index >= LKM_BLOB_NUMBER)
		return;

	lkm_blob[index].data = (void *)ctx->buf_base + index * ctx->buf_size;
	lkm_blob[index].size = ctx->buf_size;

	sprintf(blob_name, "%s%d", LKM_BLOB_FILE_NAME, index);

	debugfs_create_blob(blob_name, S_IRUGO,
			    lm_dbg_dir, &lkm_blob[index]);

	return;
}

static int dbg_clear_mem(void *data, u64 val)
{
	int index, cnt;
	u32 *u32_ptr;

	cnt = val > 0 ? val : 256;
	pr_info("clear %d u32\n", cnt);
	u32_ptr = (u32 *)lkm_blob[0].data;
	for (index = 0; index < cnt; index++)
		u32_ptr[index] = 0;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(dbg_clear_mem_fops, NULL,
			dbg_clear_mem, "%llu\n");

static char custom_lkm;

static void custom_action(u32 addr, u32 val)
{
	struct pci_dev *dev = ctx->dev;

	pr_info("lkm %d addr 0x%X val 0x%X\n", custom_lkm, addr, val);

	if (custom_lkm)
		sb_reg_write(ctx->dev, soc_diff->lm_port_id, addr, val);
	else {
		sb_reg_write(dev, soc_diff->visa_port_id, addr, val);
		visa_replay_reg_write(dev, addr, val);
	}
}

static void custom_parse(char *buf, size_t cnt)
{
	char *config_ptr;
	u32 addr, val;
	int arg, index;
	size_t len;
	struct pci_dev *dev = ctx->dev;

	config_ptr = buf;
	config_ptr[cnt] = '\0';
	for (index = 0; index < cnt; index++)
		if ((config_ptr[index] == '\n') || (config_ptr[index] == '\r'))
			config_ptr[index] = '\0';

	for (; 1;) {
		len = strlen(config_ptr);
		if (len) {
			arg = sscanf(config_ptr, "%X %X\n", &addr, &val);
			if (arg == 2) {
				custom_action(addr, val);
			} else {
				arg = strncasecmp(config_ptr, "LKM", 3);
				if (arg == 0) {
					pr_err("Visa replay %d done; configure LKM !\n",
						visa_replay_cnt);
					custom_lkm = 1;
					visa_replay_trig_en(dev, 0, 1);
				}
			}
		}
		/* Also add \0 */
		config_ptr += len + 1;
		if (config_ptr >= (buf + cnt - 1))
			break;
	}
}

static ssize_t custom_write(struct file *fp, const char __user *usr_buf,
				size_t cnt, loff_t *pos)
{
	char *buf;
	loff_t pon;
	ssize_t count;

	sb_pci_init();

	if (0 == pos) {
		pr_err("Stop LKM\n");
		sb_reg_write(ctx->dev, soc_diff->lm_port_id,
				LM_STPCTL, 0x00000000);
		sb_reg_write(ctx->dev, soc_diff->lm_port_id,
				LM_OSMC, 0x008D6000);
		visa_replay_cnt = 0;
		custom_lkm = 0;
	}

	buf = kzalloc(cnt + 1, GFP_KERNEL);
	if (!buf) {
		pr_err("No memory...\n");
		goto custom_write_end;
	}
	pon = 0;
	do {
		count = simple_write_to_buffer(buf, cnt, &pon, usr_buf, cnt);
		custom_parse(buf, count);
	} while ((pon < (cnt - 1)) || !(count));

	pr_err("done.\n");
	kfree(buf);

custom_write_end:
	return cnt;
}

static const struct file_operations custom_fops = {
	.write = custom_write,
};

/*
 * Init/Exit functions
 */
static struct notifier_block panic_lmlog_setup = {
	.notifier_call = panic_lmlog_setup_call,
	.next = NULL,
	.priority = INT_MAX
};

static int __init lm_dump_init(void)
{
	int err = 0;
	u8 index;

	lbr_dump_dbg_init();

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (ctx == NULL) {
		pr_err("kzalloc() returned NULL memory\n");
		return -ENOMEM;
	}

	soc_diff = kzalloc(sizeof(*soc_diff), GFP_KERNEL);
	if (soc_diff == NULL) {
		pr_err("kzalloc() returned NULL memory for soc_diff\n");
		err = -ENOMEM;
		goto err_free_ctx;
	}

	switch (intel_mid_identify_cpu()) {
#if 0
	case INTEL_MID_CPU_CHIP_ANNIEDALE:
		soc_diff->lm_port_id = ANN_LM_PORT_ID;
		soc_diff->visa_port_id = ANN_VISA_PORT_ID;
		break;
#endif
	case INTEL_MID_CPU_CHIP_TANGIER:
		soc_diff->lm_port_id = TNG_LM_PORT_ID;
		soc_diff->visa_port_id = TNG_VISA_PORT_ID;
		break;
	default:
		soc_diff->lm_port_id = LM_PORT_ID;
		soc_diff->visa_port_id = VISA_PORT_ID;
	}

	ctx->dev = pci_get_bus_and_slot(0, PCI_DEVFN(0, 0));
	if (!ctx->dev) {
		pr_err("pci dev (0, 0, 0) not available\n");
		err = -ENODEV;
		goto err_free_soc_diff;
	}

	sb_pci_init();

	ctx->buf_size = lm_get_size(ctx->dev);
	if (!ctx->buf_size) {
		pr_err("buf size is 0\n");
/* IFWI should reserve the area */
		if (!lkm_buf_size) {
			err = -ENOMEM;
			goto err_pci;
		}
		ctx->buf_base = kzalloc(1024*lkm_buf_size, GFP_KERNEL);
		if (!ctx->buf_base) {
			pr_err("allocated buf size is 0\n");
			err = -ENOMEM;
			goto err_pci;
		}
		ctx->buf_size = 1024 * lkm_buf_size;
		ctx->buf_bios = 0;
		pr_err("configuring LKM with 0x%lx buf size\n", ctx->buf_size);
		sb_reg_write(ctx->dev, soc_diff->lm_port_id, LM_STORMEMBAR_L,
				virt_to_phys(ctx->buf_base) & 0xFFFFFFFF);
#ifdef CONFIG_PHYS_ADDR_T_64BIT
		sb_reg_write(ctx->dev, soc_diff->lm_port_id,
			LM_STORMEMBAR_H, virt_to_phys(ctx->buf_base) >> 32);
#else
		sb_reg_write(ctx->dev, soc_diff->lm_port_id,
			LM_STORMEMBAR_H, 0);
#endif
		sb_reg_write(ctx->dev, soc_diff->lm_port_id, LM_MEMDEPTH,
				lkm_buf_size/8);
		goto skip_ioremap;
	} else
		lkm_buf_size = ctx->buf_size >> 10;

	ctx->buf_addr = lm_get_base_addr(ctx->dev);
	/* Map 2 * size as buffer is backed-up by FW just after the
	 * current buffer to retrieve it after a warm reset.
	 */
	ctx->buf_base = ioremap_nocache(ctx->buf_addr, ctx->buf_size * 2);
	if (!ctx->buf_base) {
		pr_err("can't map phys / nocache (0x%lx@0x%llx) (size * 2)\n",
		       ctx->buf_size * 2, (unsigned long long)ctx->buf_addr);
	} else
		goto after_ioremap;
	ctx->buf_base = ioremap_cache(ctx->buf_addr, ctx->buf_size * 2);
	if (!ctx->buf_base) {
		pr_err("can't map phys / cache (0x%lx@0x%llx) (size * 2)\n",
		       ctx->buf_size * 2, (unsigned long long)ctx->buf_addr);
		err = -ENOMEM;
		goto err_pci;
	}
after_ioremap:
	ctx->buf_bios = 1;
skip_ioremap:
	err = atomic_notifier_chain_register(&panic_notifier_list,
					     &panic_lmlog_setup);
	if (err < 0) {
		pr_err("register panic notifier failed %d\n", err);
		goto err_pci_map;
	}

	lm_dbg_dir = debugfs_create_dir("lm_dump", NULL);
	if (!lm_dbg_dir) {
		pr_err("debugfs dir failed");
		err = -ENOMEM;
		goto err_notifier;
	}


	for (index = 0; index < LKM_BLOB_NUMBER; index++)
		lkm_create_blob(index);

	debugfs_create_file("lm_dump_dbg", S_IRUGO,
			    lm_dbg_dir, NULL, &lkm_dbg_fops);
	debugfs_create_file("lkmstop", S_IWUSR,
			    lm_dbg_dir, NULL, &lkm_stop_fops);
	debugfs_create_file("customconfig", S_IWUSR,
			    lm_dbg_dir, NULL, &custom_fops);
	debugfs_create_file("clearmem", S_IWUSR,
			    lm_dbg_dir, NULL, &dbg_clear_mem_fops);

#ifdef LM_DEBUG
	debugfs_create_file("osmc", S_IRUGO,
			    lm_dbg_dir, NULL, &dbg_osmc_fops);
	debugfs_create_file("ostat", S_IRUGO,
			    lm_dbg_dir, NULL, &dbg_ostat_fops);
	debugfs_create_file("memwrpnt", S_IRUGO,
			    lm_dbg_dir, NULL, &dbg_memwrpnt_fops);
	debugfs_create_file("lkmclear", S_IWUSR,
			    lm_dbg_dir, NULL, &lkmcl_dbg_fops);
#endif /* LM_DEBUG */

	return 0;

err_notifier:
	atomic_notifier_chain_unregister(&panic_notifier_list,
					       &panic_lmlog_setup);
err_pci_map:
	if (ctx->buf_bios)
		iounmap(ctx->buf_base);
	else
		kfree(ctx->buf_base);
err_pci:
	pci_dev_put(ctx->dev);
err_free_soc_diff:
	kfree(soc_diff);
err_free_ctx:
	kfree(ctx);
	ctx = NULL;

	lbr_dump_dbg_exit();

	return err;
}

static void  __exit lm_dump_exit(void)
{
	int err;

	err = atomic_notifier_chain_unregister(&panic_notifier_list,
					       &panic_lmlog_setup);

	if (err < 0)
		pr_err("unregister panic notifier failed %d\n", err);

	debugfs_remove_recursive(lm_dbg_dir);

	if (ctx->buf_bios)
		iounmap(ctx->buf_base);
	else {
		/* Stop Lakemore module */
		lm_stop(ctx->dev);

		sb_reg_write(ctx->dev, soc_diff->lm_port_id,
			LM_MEMDEPTH, 0);
		kfree(ctx->buf_base);
	}

	pci_dev_put(ctx->dev);
	kfree(ctx);
	ctx = NULL;

	lbr_dump_dbg_exit();
}

module_init(lm_dump_init);
module_exit(lm_dump_exit);

MODULE_LICENSE("GPL");
