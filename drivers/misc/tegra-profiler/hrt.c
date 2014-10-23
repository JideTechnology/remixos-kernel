/*
 * drivers/misc/tegra-profiler/hrt.c
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
#include <linux/kallsyms.h>
#include <linux/sched.h>
#include <asm/cputype.h>
#include <linux/hrtimer.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/ratelimit.h>
#include <asm/irq_regs.h>
#include <linux/ptrace.h>

#include <linux/tegra_profiler.h>

#include "quadd.h"
#include "hrt.h"
#include "comm.h"
#include "mmap.h"
#include "ma.h"
#include "power_clk.h"
#include "tegra.h"
#include "debug.h"

static struct quadd_hrt_ctx hrt;

static void read_all_sources(struct pt_regs *regs, pid_t pid);

static void sample_time_prepare(void);
static void sample_time_finish(void);
static void sample_time_reset(struct quadd_cpu_context *cpu_ctx);

static enum hrtimer_restart hrtimer_handler(struct hrtimer *hrtimer)
{
	struct pt_regs *regs;

	regs = get_irq_regs();

	if (hrt.active == 0)
		return HRTIMER_NORESTART;

	qm_debug_handler_sample(regs);

	if (regs) {
		sample_time_prepare();
		read_all_sources(regs, -1);
		sample_time_finish();
	}

	hrtimer_forward_now(hrtimer, ns_to_ktime(hrt.sample_period));
	qm_debug_timer_forward(regs, hrt.sample_period);

	return HRTIMER_RESTART;
}

static void start_hrtimer(struct quadd_cpu_context *cpu_ctx)
{
	u64 period = hrt.sample_period;

	sample_time_reset(cpu_ctx);

	hrtimer_start(&cpu_ctx->hrtimer, ns_to_ktime(period),
		      HRTIMER_MODE_REL_PINNED);
	qm_debug_timer_start(NULL, period);
}

static void cancel_hrtimer(struct quadd_cpu_context *cpu_ctx)
{
	hrtimer_cancel(&cpu_ctx->hrtimer);
	qm_debug_timer_cancel();
}

static void init_hrtimer(struct quadd_cpu_context *cpu_ctx)
{
	sample_time_reset(cpu_ctx);

	hrtimer_init(&cpu_ctx->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cpu_ctx->hrtimer.function = hrtimer_handler;
}

u64 quadd_get_time(void)
{
	struct timespec ts;

	do_posix_clock_monotonic_gettime(&ts);
	return timespec_to_ns(&ts);
}

static u64 get_sample_time(void)
{
#ifndef QUADD_USE_CORRECT_SAMPLE_TS
	return quadd_get_time();
#else
	struct quadd_cpu_context *cpu_ctx = this_cpu_ptr(hrt.cpu_ctx);
	return cpu_ctx->current_time;
#endif
}

static void sample_time_prepare(void)
{
#ifdef QUADD_USE_CORRECT_SAMPLE_TS
	struct quadd_cpu_context *cpu_ctx = this_cpu_ptr(hrt.cpu_ctx);

	if (cpu_ctx->prev_time == ULLONG_MAX)
		cpu_ctx->current_time = quadd_get_time();
	else
		cpu_ctx->current_time = cpu_ctx->prev_time + hrt.sample_period;
#endif
}

static void sample_time_finish(void)
{
#ifdef QUADD_USE_CORRECT_SAMPLE_TS
	struct quadd_cpu_context *cpu_ctx = this_cpu_ptr(hrt.cpu_ctx);
	cpu_ctx->prev_time = cpu_ctx->current_time;
#endif
}

static void sample_time_reset(struct quadd_cpu_context *cpu_ctx)
{
#ifdef QUADD_USE_CORRECT_SAMPLE_TS
	cpu_ctx->prev_time = ULLONG_MAX;
	cpu_ctx->current_time = ULLONG_MAX;
#endif
}

static void put_header(void)
{
	int power_rate_period;
	struct quadd_record_data record;
	struct quadd_header_data *hdr = &record.hdr;
	struct quadd_parameters *param = &hrt.quadd_ctx->param;
	struct quadd_comm_data_interface *comm = hrt.quadd_ctx->comm;

	record.magic = QUADD_RECORD_MAGIC;
	record.record_type = QUADD_RECORD_TYPE_HEADER;
	record.cpu_mode = QUADD_CPU_MODE_NONE;

	hdr->version = QUADD_SAMPLES_VERSION;

	hdr->backtrace = param->backtrace;
	hdr->use_freq = param->use_freq;
	hdr->system_wide = param->system_wide;

	/* TODO: dynamically */
#ifdef QM_DEBUG_SAMPLES_ENABLE
	hdr->debug_samples = 1;
#else
	hdr->debug_samples = 0;
#endif

	hdr->period = hrt.sample_period;
	hdr->ma_period = hrt.ma_period;

	hdr->power_rate = quadd_power_clk_is_enabled(&power_rate_period);
	hdr->power_rate_period = power_rate_period;

	comm->put_sample(&record, NULL, 0);
}

void quadd_put_sample(struct quadd_record_data *data,
		      char *extra_data, unsigned int extra_length)
{
	struct quadd_comm_data_interface *comm = hrt.quadd_ctx->comm;

	if (data->record_type == QUADD_RECORD_TYPE_SAMPLE &&
		data->sample.period > 0x7FFFFFFF) {
		struct quadd_sample_data *sample = &data->sample;
		pr_err_once("very big period, sample id: %d\n",
			    sample->event_id);
		return;
	}

	comm->put_sample(data, extra_data, extra_length);
	atomic64_inc(&hrt.counter_samples);
}

static int get_sample_data(struct event_data *event,
			   struct pt_regs *regs,
			   struct quadd_sample_data *sample)
{
	u32 period;
	u32 prev_val, val;

	prev_val = event->prev_val;
	val = event->val;

	sample->event_id = event->event_id;

	sample->ip = instruction_pointer(regs);
	sample->cpu = quadd_get_processor_id(regs);
	sample->time = get_sample_time();

	if (prev_val <= val)
		period = val - prev_val;
	else
		period = QUADD_U32_MAX - prev_val + val;

	if (event->event_source == QUADD_EVENT_SOURCE_PL310) {
		int nr_current_active = atomic_read(&hrt.nr_active_all_core);
		if (nr_current_active > 1)
			period = period / nr_current_active;
	}

	sample->period = period;
	return 0;
}

static void read_source(struct quadd_event_source_interface *source,
			struct pt_regs *regs, pid_t pid)
{
	int nr_events, i;
	struct event_data events[QUADD_MAX_COUNTERS];
	struct quadd_record_data record_data;
	struct quadd_thread_data *t_data;
	char *extra_data = NULL;
	unsigned int extra_length = 0, callchain_nr = 0;
	struct quadd_cpu_context *cpu_ctx = this_cpu_ptr(hrt.cpu_ctx);
	struct quadd_callchain *callchain_data = &cpu_ctx->callchain_data;
	struct quadd_ctx *quadd_ctx = hrt.quadd_ctx;
	struct pt_regs *user_regs;

	if (!source)
		return;

	nr_events = source->read(events);

	if (nr_events == 0 || nr_events > QUADD_MAX_COUNTERS) {
		pr_err_once("Error number of counters: %d, source: %p\n",
				nr_events, source);
		return;
	}

	if (atomic_read(&cpu_ctx->nr_active) == 0)
		return;

	if (user_mode(regs))
		user_regs = regs;
	else
		user_regs = task_pt_regs(current);

	if (hrt.quadd_ctx->param.backtrace) {
		callchain_nr =
			quadd_get_user_callchain(user_regs, callchain_data);
		if (callchain_nr > 0) {
			extra_data = (char *)cpu_ctx->callchain_data.callchain;
			extra_length = callchain_nr * sizeof(u32);
		}
	}

	for (i = 0; i < nr_events; i++) {
		if (get_sample_data(&events[i], regs, &record_data.sample))
			return;

		record_data.magic = QUADD_RECORD_MAGIC;
		record_data.record_type = QUADD_RECORD_TYPE_SAMPLE;
		record_data.cpu_mode = user_mode(regs) ?
			QUADD_CPU_MODE_USER : QUADD_CPU_MODE_KERNEL;

		/* For security reasons, hide IPs from the kernel space. */
		if (record_data.cpu_mode == QUADD_CPU_MODE_KERNEL &&
		    !quadd_ctx->collect_kernel_ips)
			record_data.sample.ip = 0;

		if (pid > 0) {
			record_data.sample.pid = pid;
		} else {
			t_data = &cpu_ctx->active_thread;
			record_data.sample.pid = t_data->pid;
		}

		if (i == 0) {
			record_data.sample.callchain_nr = callchain_nr;
			quadd_put_sample(&record_data, extra_data,
					 extra_length);
		} else {
			record_data.sample.callchain_nr = 0;
			quadd_put_sample(&record_data, NULL, 0);
		}
	}
}

static void read_all_sources(struct pt_regs *regs, pid_t pid)
{
	struct quadd_ctx *ctx = hrt.quadd_ctx;
	struct quadd_cpu_context *cpu_ctx = this_cpu_ptr(hrt.cpu_ctx);

	if (!regs)
		return;

	quadd_get_mmap_object(cpu_ctx, regs, pid);

	if (ctx->pmu && ctx->pmu_info.active)
		read_source(ctx->pmu, regs, pid);

	if (ctx->pl310 && ctx->pl310_info.active)
		read_source(ctx->pl310, regs, pid);
}

static inline int is_profile_process(pid_t pid)
{
	int i;
	pid_t profile_pid;
	struct quadd_ctx *ctx = hrt.quadd_ctx;

	for (i = 0; i < ctx->param.nr_pids; i++) {
		profile_pid = ctx->param.pids[i];
		if (profile_pid == pid)
			return 1;
	}
	return 0;
}

static int
add_active_thread(struct quadd_cpu_context *cpu_ctx, pid_t pid, pid_t tgid)
{
	struct quadd_thread_data *t_data = &cpu_ctx->active_thread;

	if (t_data->pid > 0 ||
		atomic_read(&cpu_ctx->nr_active) > 0) {
		pr_warn_once("Warning for thread: %d\n", (int)pid);
		return 0;
	}

	t_data->pid = pid;
	t_data->tgid = tgid;
	return 1;
}

static int remove_active_thread(struct quadd_cpu_context *cpu_ctx, pid_t pid)
{
	struct quadd_thread_data *t_data = &cpu_ctx->active_thread;

	if (t_data->pid < 0)
		return 0;

	if (t_data->pid == pid) {
		t_data->pid = -1;
		t_data->tgid = -1;
		return 1;
	}

	pr_warn_once("Warning for thread: %d\n", (int)pid);
	return 0;
}

void __quadd_task_sched_in(struct task_struct *prev,
			   struct task_struct *task)
{
	int current_flag;
	struct quadd_cpu_context *cpu_ctx = this_cpu_ptr(hrt.cpu_ctx);
	struct quadd_ctx *ctx = hrt.quadd_ctx;
	struct event_data events[QUADD_MAX_COUNTERS];
	/* static DEFINE_RATELIMIT_STATE(ratelimit_state, 5 * HZ, 2); */

	if (likely(hrt.active == 0))
		return;
/*
	if (__ratelimit(&ratelimit_state))
		pr_info("sch_in, cpu: %d, prev: %u (%u) \t--> curr: %u (%u)\n",
			smp_processor_id(), (unsigned int)prev->pid,
			(unsigned int)prev->tgid, (unsigned int)task->pid,
			(unsigned int)task->tgid);
*/
	current_flag = is_profile_process(task->tgid);

	if (current_flag) {
		add_active_thread(cpu_ctx, task->pid, task->tgid);
		atomic_inc(&cpu_ctx->nr_active);

		if (atomic_read(&cpu_ctx->nr_active) == 1) {
			if (ctx->pmu)
				ctx->pmu->start();

			if (ctx->pl310)
				ctx->pl310->read(events);

			start_hrtimer(cpu_ctx);
			atomic_inc(&hrt.nr_active_all_core);
		}
	}
}

void __quadd_task_sched_out(struct task_struct *prev,
			    struct task_struct *next)
{
	int n, prev_flag;
	struct quadd_cpu_context *cpu_ctx = this_cpu_ptr(hrt.cpu_ctx);
	struct quadd_ctx *ctx = hrt.quadd_ctx;
	/* static DEFINE_RATELIMIT_STATE(ratelimit_state, 5 * HZ, 2); */

	if (likely(hrt.active == 0))
		return;
/*
	if (__ratelimit(&ratelimit_state))
		pr_info("sch_out: cpu: %d, prev: %u (%u) \t--> next: %u (%u)\n",
			smp_processor_id(), (unsigned int)prev->pid,
			(unsigned int)prev->tgid, (unsigned int)next->pid,
			(unsigned int)next->tgid);
*/
	prev_flag = is_profile_process(prev->tgid);

	if (prev_flag) {
		n = remove_active_thread(cpu_ctx, prev->pid);
		atomic_sub(n, &cpu_ctx->nr_active);

		if (atomic_read(&cpu_ctx->nr_active) == 0) {
			cancel_hrtimer(cpu_ctx);
			atomic_dec(&hrt.nr_active_all_core);

			if (ctx->pmu)
				ctx->pmu->stop();
		}
	}
}

static void reset_cpu_ctx(void)
{
	int cpu_id;
	struct quadd_cpu_context *cpu_ctx;
	struct quadd_thread_data *t_data;

	for (cpu_id = 0; cpu_id < nr_cpu_ids; cpu_id++) {
		cpu_ctx = per_cpu_ptr(hrt.cpu_ctx, cpu_id);
		t_data = &cpu_ctx->active_thread;

		atomic_set(&cpu_ctx->nr_active, 0);

		t_data->pid = -1;
		t_data->tgid = -1;

		sample_time_reset(cpu_ctx);
	}
}

int quadd_hrt_start(void)
{
	int err;
	u64 period;
	long freq;
	unsigned int extra;
	struct quadd_ctx *ctx = hrt.quadd_ctx;
	struct quadd_cpu_context *cpu_ctx = this_cpu_ptr(hrt.cpu_ctx);
	struct quadd_parameters *param = &ctx->param;

	freq = ctx->param.freq;
	freq = max_t(long, QUADD_HRT_MIN_FREQ, freq);
	period = NSEC_PER_SEC / freq;
	hrt.sample_period = period;

	if (ctx->param.ma_freq > 0)
		hrt.ma_period = MSEC_PER_SEC / ctx->param.ma_freq;
	else
		hrt.ma_period = 0;

	atomic64_set(&hrt.counter_samples, 0);

	reset_cpu_ctx();

	put_header();

	extra = param->reserved[QUADD_PARAM_IDX_EXTRA];

	if (extra & QUADD_PARAM_IDX_EXTRA_GET_MMAP) {
		err = quadd_get_current_mmap(cpu_ctx, param->pids[0]);
		if (err) {
			pr_err("error: quadd_get_current_mmap\n");
			return err;
		}
	}

	if (ctx->pl310)
		ctx->pl310->start();

	quadd_ma_start(&hrt);

	hrt.active = 1;

	pr_info("Start hrt: freq/period: %ld/%llu\n", freq, period);
	return 0;
}

void quadd_hrt_stop(void)
{
	struct quadd_ctx *ctx = hrt.quadd_ctx;

	pr_info("Stop hrt, number of samples: %llu\n",
		atomic64_read(&hrt.counter_samples));

	if (ctx->pl310)
		ctx->pl310->stop();

	quadd_ma_stop(&hrt);

	hrt.active = 0;

	atomic64_set(&hrt.counter_samples, 0);

	/* reset_cpu_ctx(); */
}

void quadd_hrt_deinit(void)
{
	if (hrt.active)
		quadd_hrt_stop();

	free_percpu(hrt.cpu_ctx);
}

void quadd_hrt_get_state(struct quadd_module_state *state)
{
	state->nr_all_samples = atomic64_read(&hrt.counter_samples);
	state->nr_skipped_samples = 0;
}

struct quadd_hrt_ctx *quadd_hrt_init(struct quadd_ctx *ctx)
{
	int cpu_id;
	u64 period;
	long freq;
	struct quadd_cpu_context *cpu_ctx;

	hrt.quadd_ctx = ctx;
	hrt.active = 0;

	freq = ctx->param.freq;
	freq = max_t(long, QUADD_HRT_MIN_FREQ, freq);
	period = NSEC_PER_SEC / freq;
	hrt.sample_period = period;

	if (ctx->param.ma_freq > 0)
		hrt.ma_period = MSEC_PER_SEC / ctx->param.ma_freq;
	else
		hrt.ma_period = 0;

	atomic64_set(&hrt.counter_samples, 0);

	hrt.cpu_ctx = alloc_percpu(struct quadd_cpu_context);
	if (!hrt.cpu_ctx)
		return NULL;

	for (cpu_id = 0; cpu_id < nr_cpu_ids; cpu_id++) {
		cpu_ctx = per_cpu_ptr(hrt.cpu_ctx, cpu_id);

		atomic_set(&cpu_ctx->nr_active, 0);

		cpu_ctx->active_thread.pid = -1;
		cpu_ctx->active_thread.tgid = -1;

		init_hrtimer(cpu_ctx);
	}

	return &hrt;
}
