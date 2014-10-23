/*
 * drivers/misc/tegra-profiler/mmap.h
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

#ifndef __QUADD_MMAP_H
#define __QUADD_MMAP_H

#include <linux/types.h>

struct quadd_cpu_context;
struct quadd_ctx;
struct quadd_mmap_data;

#define QUADD_MMAP_SIZE_ARRAY	4096

struct quadd_mmap_cpu_ctx {
	char *tmp_buf;
};

struct quadd_mmap_ctx {
	u32 *hash_array;
	unsigned int nr_hashes;
	spinlock_t lock;

	struct quadd_mmap_cpu_ctx * __percpu cpu_ctx;

	struct quadd_ctx *quadd_ctx;
};

void quadd_get_mmap_object(struct quadd_cpu_context *cpu_ctx,
			   struct pt_regs *regs, pid_t pid);

int quadd_get_current_mmap(struct quadd_cpu_context *cpu_ctx, pid_t pid);

struct quadd_mmap_ctx *quadd_mmap_init(struct quadd_ctx *quadd_ctx);
void quadd_mmap_deinit(void);
void quadd_mmap_reset(void);

#endif  /* __QUADD_MMAP_H */
