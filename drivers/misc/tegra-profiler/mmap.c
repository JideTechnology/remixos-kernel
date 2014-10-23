/*
 * drivers/misc/tegra-profiler/mmap.c
 *
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/crc32.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/sched.h>

#include <linux/tegra_profiler.h>

#include "mmap.h"
#include "hrt.h"
#include "debug.h"

static struct quadd_mmap_ctx mmap_ctx;

static int binary_search_and_add(unsigned int *array,
			unsigned int length, unsigned int key)
{
	unsigned int i_min, i_max, mid;

	if (length == 0) {
		array[0] = key;
		return 1;
	} else if (length == 1 && array[0] == key) {
		return 0;
	}

	i_min = 0;
	i_max = length;

	if (array[0] > key) {
		memmove((char *)((unsigned int *)array + 1), array,
			length * sizeof(unsigned int));
		array[0] = key;
		return 1;
	} else if (array[length - 1] < key) {
		array[length] = key;
		return 1;
	}

	while (i_min < i_max) {
		mid = i_min + (i_max - i_min) / 2;

		if (key <= array[mid])
			i_max = mid;
		else
			i_min = mid + 1;
	}

	if (array[i_max] == key) {
		return 0;
	} else {
		memmove((char *)((unsigned int *)array + i_max + 1),
			(char *)((unsigned int *)array + i_max),
			(length - i_max) * sizeof(unsigned int));
		array[i_max] = key;
		return 1;
	}
}

static int check_hash(u32 key)
{
	int res;
	unsigned long flags;

	spin_lock_irqsave(&mmap_ctx.lock, flags);

	if (mmap_ctx.nr_hashes >= QUADD_MMAP_SIZE_ARRAY) {
		spin_unlock_irqrestore(&mmap_ctx.lock, flags);
		return 1;
	}

	res = binary_search_and_add(mmap_ctx.hash_array,
				    mmap_ctx.nr_hashes, key);
	if (res > 0) {
		mmap_ctx.nr_hashes++;
		spin_unlock_irqrestore(&mmap_ctx.lock, flags);
		return 0;
	}

	spin_unlock_irqrestore(&mmap_ctx.lock, flags);
	return 1;
}

static int find_file(const char *file_name, unsigned long addr,
		     unsigned long len)
{
	u32 crc;
	size_t length;

	if (!file_name)
		return 0;

	length = strlen(file_name);

	crc = crc32_le(~0, file_name, length);
	crc = crc32_le(crc, (unsigned char *)&addr,
		       sizeof(addr));
	crc = crc32_le(crc, (unsigned char *)&len,
		       sizeof(len));

	return check_hash(crc);
}

static void
put_mmap_sample(struct quadd_mmap_data *s, char *extra_data,
		size_t extra_length)
{
	struct quadd_record_data r;

	r.magic = QUADD_RECORD_MAGIC;
	r.record_type = QUADD_RECORD_TYPE_MMAP;
	r.cpu_mode = QUADD_CPU_MODE_USER;

	memcpy(&r.mmap, s, sizeof(*s));
	r.mmap.filename_length = extra_length;

	pr_debug("MMAP: pid: %d, file_name: '%s', addr: %#x, length: %u",
		 s->pid, extra_data, s->addr, extra_length);

	quadd_put_sample(&r, extra_data, extra_length);
}

void quadd_get_mmap_object(struct quadd_cpu_context *cpu_ctx,
			   struct pt_regs *regs, pid_t pid)
{
	unsigned long ip;
	size_t length, length_aligned;
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	struct file *vm_file;
	struct path *path;
	char *file_name = NULL;
	struct quadd_mmap_data sample;
	struct quadd_mmap_cpu_ctx *mm_cpu_ctx = this_cpu_ptr(mmap_ctx.cpu_ctx);
	char *tmp_buf = mm_cpu_ctx->tmp_buf;

	if (!mm)
		return;

	ip = instruction_pointer(regs);

	if (user_mode(regs)) {
		for (vma = find_vma(mm, ip); vma; vma = vma->vm_next) {
			if (ip < vma->vm_start || ip >= vma->vm_end)
				continue;

			vm_file = vma->vm_file;
			if (!vm_file)
				break;

			path = &vm_file->f_path;

			file_name = d_path(path, tmp_buf, PATH_MAX);
			if (IS_ERR(file_name)) {
				file_name = NULL;
			} else {
				sample.addr = vma->vm_start;
				sample.len = vma->vm_end - vma->vm_start;
				sample.pgoff =
					(u64)vma->vm_pgoff << PAGE_SHIFT;
			}
			break;
		}
	} else {
#ifdef CONFIG_MODULES
		struct module *mod;

		preempt_disable();
		mod = __module_address(ip);
		preempt_enable();

		if (mod) {
			file_name = mod->name;
			if (file_name) {
				sample.addr = (u32) mod->module_core;
				sample.len = mod->core_size;
				sample.pgoff = 0;
			}
		}
#endif
	}

	if (file_name) {
		if (!find_file(file_name, sample.addr, sample.len)) {
			length = strlen(file_name) + 1;
			if (length > PATH_MAX)
				return;

			sample.pid = pid;

			strcpy(cpu_ctx->mmap_filename, file_name);
			length_aligned = ALIGN(length, 8);

			put_mmap_sample(&sample, cpu_ctx->mmap_filename,
					length_aligned);
		}
	}
}

int quadd_get_current_mmap(struct quadd_cpu_context *cpu_ctx, pid_t pid)
{
	struct vm_area_struct *vma;
	struct file *vm_file;
	struct path *path;
	char *file_name;
	struct task_struct *task;
	struct mm_struct *mm;
	struct quadd_mmap_data sample;
	size_t length, length_aligned;
	struct quadd_mmap_cpu_ctx *mm_cpu_ctx = this_cpu_ptr(mmap_ctx.cpu_ctx);
	char *tmp_buf = mm_cpu_ctx->tmp_buf;

	rcu_read_lock();
	task = pid_task(find_vpid(pid), PIDTYPE_PID);
	rcu_read_unlock();
	if (!task) {
		pr_err("Process not found: %u\n", pid);
		return -ESRCH;
	}
	mm = task->mm;

	pr_info("Get mapped memory objects\n");

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		vm_file = vma->vm_file;
		if (!vm_file)
			continue;

		path = &vm_file->f_path;

		file_name = d_path(path, tmp_buf, PATH_MAX);
		if (IS_ERR(file_name))
			continue;

		if (!(vma->vm_flags & VM_EXEC))
			continue;

		length = strlen(file_name) + 1;
		if (length > PATH_MAX)
			continue;

		sample.pid = pid;
		sample.addr = vma->vm_start;
		sample.len = vma->vm_end - vma->vm_start;
		sample.pgoff = (u64)vma->vm_pgoff << PAGE_SHIFT;

		if (!find_file(file_name, sample.addr, sample.len)) {
			strcpy(cpu_ctx->mmap_filename, file_name);
			length_aligned = ALIGN(length, 8);

			put_mmap_sample(&sample, file_name, length_aligned);
		}
	}
	return 0;
}

struct quadd_mmap_ctx *quadd_mmap_init(struct quadd_ctx *quadd_ctx)
{
	int cpu_id;
	u32 *hash;
	char *tmp;
	struct quadd_mmap_cpu_ctx *cpu_ctx;

	mmap_ctx.quadd_ctx = quadd_ctx;

	hash = kzalloc(QUADD_MMAP_SIZE_ARRAY * sizeof(unsigned int),
		       GFP_KERNEL);
	if (!hash) {
		pr_err("Failed to allocate mmap buffer\n");
		return ERR_PTR(-ENOMEM);
	}
	mmap_ctx.hash_array = hash;

	mmap_ctx.nr_hashes = 0;
	spin_lock_init(&mmap_ctx.lock);

	mmap_ctx.cpu_ctx = alloc_percpu(struct quadd_mmap_cpu_ctx);
	if (!mmap_ctx.cpu_ctx)
		return ERR_PTR(-ENOMEM);

	for (cpu_id = 0; cpu_id < nr_cpu_ids; cpu_id++) {
		cpu_ctx = per_cpu_ptr(mmap_ctx.cpu_ctx, cpu_id);

		tmp = kzalloc(PATH_MAX + sizeof(unsigned long long),
			      GFP_KERNEL);
		if (!tmp) {
			pr_err("Failed to allocate mmap buffer\n");
			return ERR_PTR(-ENOMEM);
		}
		cpu_ctx->tmp_buf = tmp;
	}

	return &mmap_ctx;
}

void quadd_mmap_reset(void)
{
	unsigned long flags;

	spin_lock_irqsave(&mmap_ctx.lock, flags);
	mmap_ctx.nr_hashes = 0;
	spin_unlock_irqrestore(&mmap_ctx.lock, flags);
}

void quadd_mmap_deinit(void)
{
	int cpu_id;
	unsigned long flags;
	struct quadd_mmap_cpu_ctx *cpu_ctx;

	spin_lock_irqsave(&mmap_ctx.lock, flags);
	kfree(mmap_ctx.hash_array);
	mmap_ctx.hash_array = NULL;

	for (cpu_id = 0; cpu_id < nr_cpu_ids; cpu_id++) {
		cpu_ctx = per_cpu_ptr(mmap_ctx.cpu_ctx, cpu_id);
		kfree(cpu_ctx->tmp_buf);
	}
	free_percpu(mmap_ctx.cpu_ctx);

	spin_unlock_irqrestore(&mmap_ctx.lock, flags);
}
