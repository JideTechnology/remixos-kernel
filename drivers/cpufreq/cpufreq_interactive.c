/*
 * drivers/cpufreq/cpufreq_interactive.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Mike Chan (mike@android.com)
 *
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/mutex.h>

#include <asm/cputime.h>


struct cpufreq_interactive_cpuinfo {
	struct timer_list cpu_timer;
	int timer_idlecancel;
	u64 time_in_idle;
	u64 time_in_iowait;
	u64 idle_exit_time;
	u64 timer_run_time;
	int idling;
	u64 freq_change_time;
	u64 freq_change_time_in_idle;
	u64 freq_change_time_in_iowait;
	unsigned int io_consecutive;
	u64 last_high_freq_time;
	struct cpufreq_policy *policy;
	struct cpufreq_frequency_table *freq_table;
	unsigned int target_freq;
	int governor_enabled;
};

static DEFINE_PER_CPU(struct cpufreq_interactive_cpuinfo, cpuinfo);

/* realtime thread handles frequency scaling */
static struct task_struct *speedchange_task;
static cpumask_t speedchange_cpumask;
static spinlock_t speedchange_cpumask_lock;

/* Go to max speed when CPU load at or above this value. */
#define DEFAULT_GO_MAXSPEED_LOAD 85
static unsigned long go_maxspeed_load;

/* Base of exponential raise to max speed; if 0 - jump to maximum */
static unsigned long boost_factor;

/* Max frequency boost in Hz; if 0 - no max is enforced */
static unsigned long max_boost;

/* Consider IO as busy */
static unsigned long io_is_busy;

/* Consider IO as busy if consecutive IOs are above this value. */
static unsigned long io_busy_threshold;

/*
 * Targeted sustainable load relatively to current frequency.
 * If 0, target is set realtively to the max speed
 */
static unsigned long sustain_load;

/*
 * The minimum amount of time to spend at a frequency before we can ramp down.
 */
#define DEFAULT_MIN_SAMPLE_TIME 30000;
static unsigned long min_sample_time;

/*
 * The sample rate of the timer used to increase frequency
 */
#define DEFAULT_TIMER_RATE 20000;
static unsigned long timer_rate;

/*
 * The minimum delay before frequency is allowed to raise over normal rate.
 * Since it must remain at high frequency for a minimum of MIN_SAMPLE_TIME
 * once it rises, setting this delay to a multiple of MIN_SAMPLE_TIME
 * becomes the best way to enforce a square wave.
 * e.g. 5*MIN_SAMPLE_TIME = 20% high freq duty cycle
 */
#define DEFAULT_HIGH_FREQ_MIN_DELAY 5*DEFAULT_MIN_SAMPLE_TIME
static unsigned long high_freq_min_delay;

/*
 * The maximum frequency CPUs are allowed to run normally
 * 0 if disabled
 */
#define DEFAULT_MAX_NORMAL_FREQ 0
static unsigned long max_normal_freq;


/* Defines to control mid-range frequencies */
#define DEFAULT_MID_RANGE_GO_MAXSPEED_LOAD 95

static unsigned long midrange_freq;
static unsigned long midrange_go_maxspeed_load;
static unsigned long midrange_max_boost;

/*
 * gov_state_lock protects interactive node creation in governor start/stop.
 */
static DEFINE_MUTEX(gov_state_lock);

static struct mutex gov_state_lock;
static unsigned int active_count;

static int cpufreq_governor_interactive(struct cpufreq_policy *policy,
		unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_INTERACTIVE
static
#endif
struct cpufreq_governor cpufreq_gov_interactive = {
	.name = "interactive",
	.governor = cpufreq_governor_interactive,
	.max_transition_latency = 10000000,
	.owner = THIS_MODULE,
};

static unsigned int cpufreq_interactive_get_target(
	int cpu_load, int load_since_change, struct cpufreq_policy *policy)
{
	unsigned int target_freq;
	unsigned int maxspeed_load = go_maxspeed_load;
	unsigned int mboost = max_boost;

	/*
	 * Choose greater of short-term load (since last idle timer
	 * started or timer function re-armed itself) or long-term load
	 * (since last frequency change).
	 */
	if (load_since_change > cpu_load)
		cpu_load = load_since_change;

	if (midrange_freq && policy->cur > midrange_freq) {
		maxspeed_load = midrange_go_maxspeed_load;
		mboost = midrange_max_boost;
	}

	if (cpu_load >= maxspeed_load) {
		if (!boost_factor)
			return policy->max;

		target_freq = policy->cur * boost_factor;

		if (mboost && target_freq > policy->cur + mboost)
			target_freq = policy->cur + mboost;
	}
	else {
		if (!sustain_load)
			return policy->max * cpu_load / 100;

		target_freq = policy->cur * cpu_load / sustain_load;
	}

	target_freq = min(target_freq, policy->max);
	return target_freq;
}

static inline cputime64_t get_cpu_iowait_time(
	unsigned int cpu, cputime64_t *wall)
{
	u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

	if (iowait_time == -1ULL)
		return 0;

	return iowait_time;
}

static void cpufreq_interactive_timer(unsigned long data)
{
	unsigned int delta_idle;
	unsigned int delta_iowait;
	unsigned int delta_time;
	unsigned int io_consecutive;
	int cpu_load;
	int load_since_change;
	u64 time_in_idle;
	u64 time_in_iowait;
	u64 idle_exit_time;
	struct cpufreq_interactive_cpuinfo *pcpu =
		&per_cpu(cpuinfo, data);
	u64 now_idle;
	u64 now_iowait;
	unsigned int new_freq;
	unsigned int index;
	unsigned long flags;

	smp_rmb();

	if (!pcpu->governor_enabled)
		goto exit;

	/*
	 * Once pcpu->timer_run_time is updated to >= pcpu->idle_exit_time,
	 * this lets idle exit know the current idle time sample has
	 * been processed, and idle exit can generate a new sample and
	 * re-arm the timer.  This prevents a concurrent idle
	 * exit on that CPU from writing a new set of info at the same time
	 * the timer function runs (the timer function can't use that info
	 * until more time passes).
	 */
	time_in_idle = pcpu->time_in_idle;
	time_in_iowait = pcpu->time_in_iowait;
	idle_exit_time = pcpu->idle_exit_time;
	now_idle = get_cpu_idle_time_us(data, &pcpu->timer_run_time);
	now_iowait = get_cpu_iowait_time(data, NULL);
	smp_wmb();

	/* If we raced with cancelling a timer, skip. */
	if (!idle_exit_time)
		goto exit;

	delta_idle = (unsigned int)(now_idle - time_in_idle);
	delta_iowait = (unsigned int)(now_iowait - time_in_iowait);
	delta_time = (unsigned int)(pcpu->timer_run_time - idle_exit_time);
	io_consecutive = pcpu->io_consecutive;

	/*
	 * If timer ran less than 1ms after short-term sample started, retry.
	 */
	if (delta_time < 1000)
		goto rearm;

	if (io_busy_threshold && delta_iowait)
		io_consecutive++;
	else if (io_consecutive)
		io_consecutive = 0;

	if (!io_is_busy &&
		(!io_consecutive || (io_consecutive < io_busy_threshold)))
		delta_idle += delta_iowait;

	if (delta_idle > delta_time)
		cpu_load = 0;
	else
		cpu_load = 100 * (delta_time - delta_idle) / delta_time;

	pcpu->io_consecutive = io_consecutive;

	delta_idle = (unsigned int)(now_idle - pcpu->freq_change_time_in_idle);
	delta_iowait = (unsigned int)(now_iowait - pcpu->freq_change_time_in_iowait);
	delta_time = (unsigned int)(pcpu->timer_run_time - pcpu->freq_change_time);

	if (!io_is_busy)
		delta_idle += delta_iowait;

	if ((delta_time == 0) || (delta_idle > delta_time))
		load_since_change = 0;
	else
		load_since_change =
			100 * (delta_time - delta_idle) / delta_time;
	/*
	 * Combine short-term load (since last idle timer started or timer
	 * function re-armed itself) and long-term load (since last frequency
	 * change) to determine new target frequency
	 */
	new_freq = cpufreq_interactive_get_target(cpu_load, load_since_change,
						  pcpu->policy);

	if (cpufreq_frequency_table_target(pcpu->policy, pcpu->freq_table,
					   new_freq, CPUFREQ_RELATION_H,
					   &index)) {
		pr_warn_once("timer %d: cpufreq_frequency_table_target error\n",
			     (int) data);
		goto rearm;
	}

	new_freq = pcpu->freq_table[index].frequency;

	if (pcpu->target_freq == new_freq)
		goto rearm_if_notmax;

	/*
	 * Do not scale down unless we have been at this frequency for the
	 * minimum sample time.
	 */
	if (new_freq < pcpu->target_freq) {
		if (pcpu->timer_run_time - pcpu->freq_change_time
		    < min_sample_time)
			goto rearm;
	}

	/*
	 * Can only overclock if the delay is satisfy. Otherwise, cap it to
	 * maximum allowed normal frequency
	 */
	if (max_normal_freq && (new_freq > max_normal_freq)) {
		if ((pcpu->timer_run_time - pcpu->last_high_freq_time)
				< high_freq_min_delay) {
			new_freq = max_normal_freq;
		}
		else {
			pcpu->last_high_freq_time = pcpu->timer_run_time;
		}
	}

	pcpu->target_freq = new_freq;
	spin_lock_irqsave(&speedchange_cpumask_lock, flags);
	cpumask_set_cpu(data, &speedchange_cpumask);
	spin_unlock_irqrestore(&speedchange_cpumask_lock, flags);
	wake_up_process(speedchange_task);

rearm_if_notmax:
	/*
	 * Already set max speed and don't see a need to change that,
	 * wait until next idle to re-evaluate, don't need timer.
	 */
	if (pcpu->target_freq == pcpu->policy->max)
		goto exit;

rearm:
	if (!timer_pending(&pcpu->cpu_timer)) {
		/*
		 * If already at min: if that CPU is idle, don't set timer.
		 * Else cancel the timer if that CPU goes idle.  We don't
		 * need to re-evaluate speed until the next idle exit.
		 */
		if (pcpu->target_freq == pcpu->policy->min) {
			smp_rmb();

			if (pcpu->idling)
				goto exit;

			pcpu->timer_idlecancel = 1;
		}

		pcpu->time_in_idle = get_cpu_idle_time_us(
			data, &pcpu->idle_exit_time);
		pcpu->time_in_iowait = get_cpu_iowait_time(
			data, NULL);

		mod_timer(&pcpu->cpu_timer,
			  jiffies + usecs_to_jiffies(timer_rate));
	}

exit:
	return;
}

static void cpufreq_interactive_idle_start(void)
{
	struct cpufreq_interactive_cpuinfo *pcpu =
		&per_cpu(cpuinfo, smp_processor_id());
	int pending;

	if (!pcpu->governor_enabled)
		return;

	pcpu->idling = 1;
	smp_wmb();
	pending = timer_pending(&pcpu->cpu_timer);

	if (pcpu->target_freq != pcpu->policy->min) {
#ifdef CONFIG_SMP
		/*
		 * Entering idle while not at lowest speed.  On some
		 * platforms this can hold the other CPU(s) at that speed
		 * even though the CPU is idle. Set a timer to re-evaluate
		 * speed so this idle CPU doesn't hold the other CPUs above
		 * min indefinitely.  This should probably be a quirk of
		 * the CPUFreq driver.
		 */
		if (!pending) {
			pcpu->time_in_idle = get_cpu_idle_time_us(
				smp_processor_id(), &pcpu->idle_exit_time);
			pcpu->time_in_iowait = get_cpu_iowait_time(
				smp_processor_id(), NULL);
			pcpu->io_consecutive = 0;
			pcpu->timer_idlecancel = 0;
			mod_timer(&pcpu->cpu_timer,
				  jiffies + usecs_to_jiffies(timer_rate));
		}
#endif
	} else {
		/*
		 * If at min speed and entering idle after load has
		 * already been evaluated, and a timer has been set just in
		 * case the CPU suddenly goes busy, cancel that timer.  The
		 * CPU didn't go busy; we'll recheck things upon idle exit.
		 */
		if (pending && pcpu->timer_idlecancel) {
			del_timer_sync(&pcpu->cpu_timer);
			/*
			 * Ensure last timer run time is after current idle
			 * sample start time, so next idle exit will always
			 * start a new idle sampling period.
			 */
			pcpu->idle_exit_time = 0;
			pcpu->timer_idlecancel = 0;
		}
	}

}

static void cpufreq_interactive_idle_end(void)
{
	struct cpufreq_interactive_cpuinfo *pcpu =
		&per_cpu(cpuinfo, smp_processor_id());

	if (!pcpu->governor_enabled)
		return;

	pcpu->idling = 0;
	smp_wmb();

	/*
	 * Arm the timer for 1-2 ticks later if not already, and if the timer
	 * function has already processed the previous load sampling
	 * interval.  (If the timer is not pending but has not processed
	 * the previous interval, it is probably racing with us on another
	 * CPU.  Let it compute load based on the previous sample and then
	 * re-arm the timer for another interval when it's done, rather
	 * than updating the interval start time to be "now", which doesn't
	 * give the timer function enough time to make a decision on this
	 * run.)
	 */
	if (timer_pending(&pcpu->cpu_timer) == 0 &&
	    pcpu->timer_run_time >= pcpu->idle_exit_time &&
	    pcpu->governor_enabled) {
		pcpu->time_in_idle =
			get_cpu_idle_time_us(smp_processor_id(),
					     &pcpu->idle_exit_time);
		pcpu->time_in_iowait =
			get_cpu_iowait_time(smp_processor_id(),
						NULL);
		pcpu->io_consecutive = 0;
		pcpu->timer_idlecancel = 0;
		mod_timer(&pcpu->cpu_timer,
			  jiffies + usecs_to_jiffies(timer_rate));
	}

}

static int cpufreq_interactive_speedchange_task(void *data)
{
	unsigned int cpu;
	cpumask_t tmp_mask;
	unsigned long flags;
	struct cpufreq_interactive_cpuinfo *pcpu;

	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);
		spin_lock_irqsave(&speedchange_cpumask_lock, flags);

		if (cpumask_empty(&speedchange_cpumask)) {
			spin_unlock_irqrestore(&speedchange_cpumask_lock,
					       flags);
			schedule();

			if (kthread_should_stop())
				break;

			spin_lock_irqsave(&speedchange_cpumask_lock, flags);
		}

		set_current_state(TASK_RUNNING);
		tmp_mask = speedchange_cpumask;
		cpumask_clear(&speedchange_cpumask);
		spin_unlock_irqrestore(&speedchange_cpumask_lock, flags);

		for_each_cpu(cpu, &tmp_mask) {
			unsigned int j;
			unsigned int max_freq = 0;

			pcpu = &per_cpu(cpuinfo, cpu);
			smp_rmb();

			if (!pcpu->governor_enabled)
				continue;

			for_each_cpu(j, pcpu->policy->cpus) {
				struct cpufreq_interactive_cpuinfo *pjcpu =
					&per_cpu(cpuinfo, j);

				if (pjcpu->target_freq > max_freq)
					max_freq = pjcpu->target_freq;
			}

			__cpufreq_driver_target(pcpu->policy,
						max_freq,
						CPUFREQ_RELATION_H);

			pcpu->freq_change_time_in_idle =
				get_cpu_idle_time_us(cpu,
						     &pcpu->freq_change_time);
			pcpu->freq_change_time_in_iowait =
				get_cpu_iowait_time(cpu, NULL);
		}
	}

	return 0;
}

#define DECL_CPUFREQ_INTERACTIVE_ATTR(name) \
static ssize_t show_##name(struct kobject *kobj, \
	struct attribute *attr, char *buf) \
{ \
	return sprintf(buf, "%lu\n", name); \
} \
\
static ssize_t store_##name(struct kobject *kobj,\
		struct attribute *attr, const char *buf, size_t count) \
{ \
	int ret; \
	unsigned long val; \
\
	ret = strict_strtoul(buf, 0, &val); \
	if (ret < 0) \
		return ret; \
	name = val; \
	return count; \
} \
\
static struct global_attr name##_attr = __ATTR(name, 0644, \
		show_##name, store_##name);

DECL_CPUFREQ_INTERACTIVE_ATTR(go_maxspeed_load)
DECL_CPUFREQ_INTERACTIVE_ATTR(midrange_freq)
DECL_CPUFREQ_INTERACTIVE_ATTR(midrange_go_maxspeed_load)
DECL_CPUFREQ_INTERACTIVE_ATTR(boost_factor)
DECL_CPUFREQ_INTERACTIVE_ATTR(io_is_busy)
DECL_CPUFREQ_INTERACTIVE_ATTR(io_busy_threshold)
DECL_CPUFREQ_INTERACTIVE_ATTR(max_boost)
DECL_CPUFREQ_INTERACTIVE_ATTR(midrange_max_boost)
DECL_CPUFREQ_INTERACTIVE_ATTR(sustain_load)
DECL_CPUFREQ_INTERACTIVE_ATTR(min_sample_time)
DECL_CPUFREQ_INTERACTIVE_ATTR(timer_rate)
DECL_CPUFREQ_INTERACTIVE_ATTR(high_freq_min_delay)
DECL_CPUFREQ_INTERACTIVE_ATTR(max_normal_freq)

#undef DECL_CPUFREQ_INTERACTIVE_ATTR

static struct attribute *interactive_attributes[] = {
	&go_maxspeed_load_attr.attr,
	&midrange_freq_attr.attr,
	&midrange_go_maxspeed_load_attr.attr,
	&boost_factor_attr.attr,
	&max_boost_attr.attr,
	&midrange_max_boost_attr.attr,
	&io_is_busy_attr.attr,
	&io_busy_threshold_attr.attr,
	&sustain_load_attr.attr,
	&min_sample_time_attr.attr,
	&timer_rate_attr.attr,
	&high_freq_min_delay_attr.attr,
	&max_normal_freq_attr.attr,
	NULL,
};

static struct attribute_group interactive_attr_group = {
	.attrs = interactive_attributes,
	.name = "interactive",
};

static int cpufreq_interactive_idle_notifier(struct notifier_block *nb,
					     unsigned long val,
					     void *data)
{
	switch (val) {
	case IDLE_START:
		cpufreq_interactive_idle_start();
		break;
	case IDLE_END:
		cpufreq_interactive_idle_end();
		break;
	}

	return 0;
}

static struct notifier_block cpufreq_interactive_idle_nb = {
	.notifier_call = cpufreq_interactive_idle_notifier,
};

static int cpufreq_governor_interactive(struct cpufreq_policy *policy,
		unsigned int event)
{
	int rc;
	unsigned int j;
	struct cpufreq_interactive_cpuinfo *pcpu;
	struct cpufreq_frequency_table *freq_table;

	switch (event) {
	case CPUFREQ_GOV_START:
		if (!cpu_online(policy->cpu))
			return -EINVAL;

		freq_table =
			cpufreq_frequency_get_table(policy->cpu);

		for_each_cpu(j, policy->cpus) {
			pcpu = &per_cpu(cpuinfo, j);
			pcpu->policy = policy;
			pcpu->target_freq = policy->cur;
			pcpu->freq_table = freq_table;
			pcpu->freq_change_time_in_idle =
				get_cpu_idle_time_us(j,
					     &pcpu->freq_change_time);
			pcpu->time_in_idle = pcpu->freq_change_time_in_idle;
			pcpu->idle_exit_time = pcpu->freq_change_time;
			pcpu->freq_change_time_in_iowait =
				get_cpu_iowait_time(j, NULL);
			pcpu->time_in_iowait = pcpu->freq_change_time_in_iowait;
			pcpu->io_consecutive = 0;
			if (!pcpu->last_high_freq_time)
				pcpu->last_high_freq_time = pcpu->freq_change_time;
			pcpu->timer_idlecancel = 1;
			pcpu->governor_enabled = 1;
			smp_wmb();

			if (!timer_pending(&pcpu->cpu_timer))
				mod_timer(&pcpu->cpu_timer, jiffies + 2);
		}

		mutex_lock(&gov_state_lock);
		active_count++;

		/*
		 * Do not register the idle hook and create sysfs
		 * entries if we have already done so.
		 */
		if (active_count == 1) {
			rc = sysfs_create_group(cpufreq_global_kobject,
					&interactive_attr_group);

			if (rc) {
				mutex_unlock(&gov_state_lock);
				return rc;
			}
			idle_notifier_register(&cpufreq_interactive_idle_nb);
		}
		mutex_unlock(&gov_state_lock);

		break;

	case CPUFREQ_GOV_STOP:
		for_each_cpu(j, policy->cpus) {
			pcpu = &per_cpu(cpuinfo, j);
			pcpu->governor_enabled = 0;
			smp_wmb();
			del_timer_sync(&pcpu->cpu_timer);

			/*
			 * Reset idle exit time since we may cancel the timer
			 * before it can run after the last idle exit time,
			 * to avoid tripping the check in idle exit for a timer
			 * that is trying to run.
			 */
			pcpu->idle_exit_time = 0;
		}

		mutex_lock(&gov_state_lock);
		active_count--;

		if (active_count == 0) {
			idle_notifier_unregister(&cpufreq_interactive_idle_nb);

			sysfs_remove_group(cpufreq_global_kobject,
					&interactive_attr_group);
		}
		mutex_unlock(&gov_state_lock);

		break;

	case CPUFREQ_GOV_LIMITS:
		if (policy->max < policy->cur)
			__cpufreq_driver_target(policy,
					policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > policy->cur)
			__cpufreq_driver_target(policy,
					policy->min, CPUFREQ_RELATION_L);

		/* reschedule the timer if we stopped it */
		pcpu = &per_cpu(cpuinfo, policy->cpu);

		if (pcpu && !timer_pending(&pcpu->cpu_timer))
			mod_timer(&pcpu->cpu_timer,
				jiffies + usecs_to_jiffies(timer_rate));

		break;
	}
	return 0;
}

static int __init cpufreq_interactive_init(void)
{
	unsigned int i;
	struct cpufreq_interactive_cpuinfo *pcpu;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };

	go_maxspeed_load = DEFAULT_GO_MAXSPEED_LOAD;
	midrange_go_maxspeed_load = DEFAULT_MID_RANGE_GO_MAXSPEED_LOAD;
	min_sample_time = DEFAULT_MIN_SAMPLE_TIME;
	timer_rate = DEFAULT_TIMER_RATE;
	high_freq_min_delay = DEFAULT_HIGH_FREQ_MIN_DELAY;
	max_normal_freq = DEFAULT_MAX_NORMAL_FREQ;

	/* Initalize per-cpu timers */
	for_each_possible_cpu(i) {
		pcpu = &per_cpu(cpuinfo, i);
		init_timer(&pcpu->cpu_timer);
		pcpu->cpu_timer.function = cpufreq_interactive_timer;
		pcpu->cpu_timer.data = i;
	}

	spin_lock_init(&speedchange_cpumask_lock);
	speedchange_task =
		kthread_create(cpufreq_interactive_speedchange_task, NULL,
			       "cfinteractive");
	if (IS_ERR(speedchange_task))
		return PTR_ERR(speedchange_task);

	sched_setscheduler_nocheck(speedchange_task, SCHED_FIFO, &param);
	get_task_struct(speedchange_task);

	/* NB: wake up so the thread does not look hung to the freezer */
	wake_up_process(speedchange_task);

	return cpufreq_register_governor(&cpufreq_gov_interactive);
}

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_INTERACTIVE
fs_initcall(cpufreq_interactive_init);
#else
module_init(cpufreq_interactive_init);
#endif

static void __exit cpufreq_interactive_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_interactive);
	kthread_stop(speedchange_task);
	put_task_struct(speedchange_task);
}

module_exit(cpufreq_interactive_exit);

MODULE_AUTHOR("Mike Chan <mike@android.com>");
MODULE_DESCRIPTION("'cpufreq_interactive' - A cpufreq governor for "
	"Latency sensitive workloads");
MODULE_LICENSE("GPL");
