/*
 * CPUFreq k3hotplug governor
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/pm_qos_params.h>
#include <linux/ipps.h>
#include <linux/cpufreq-k3v2.h>
#include <mach/boardid.h>
#include <linux/suspend.h>
#include <linux/ipps.h>

/* pm_qos interface global val*/
struct pm_qos_lst {
	struct pm_qos_request_list *lst;
	int qos_class;
	s32 dvalue;
};

static struct pm_qos_request_list g_cpumaxlimits;
static struct pm_qos_request_list g_cpuminlimits;

static struct pm_qos_lst pm_qos_list[] = {
{&g_cpumaxlimits, PM_QOS_CPU_MAX_PROFILE, PM_QOS_CPU_MAXPROFILE_DEFAULT_VALUE},
{&g_cpuminlimits, PM_QOS_CPU_MIN_PROFILE, PM_QOS_CPU_MINPROFILE_DEFAULT_VALUE},
};

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
		unsigned int event);

struct cpufreq_governor cpufreq_gov_k3hotplug = {
	.name                   = "k3hotplug",
	.governor               = cpufreq_governor_dbs,
	.owner                  = THIS_MODULE,
};

struct cpu_dbs_info_s {
	struct cpufreq_policy *cur_policy;
	struct cpufreq_frequency_table *freq_table;
	int cpu;
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, hp_cpu_dbs_info);

static unsigned int dbs_enable;	/* number of CPUs using this policy */

/*
 * dbs_mutex protects data in gippslimit from concurrent changes on
 * different CPUs. It protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

struct cpu_num_limit gcpu_num_limit = {
	.max = NR_CPUS,
	.min = 1,
	.block = 0,
};

/***************************cpu hotplug*************************/
#ifndef NO_CPU_HOTPLUG

#define DEFAULT_HOTPLUG_IN_LOAD			(95)
#define DEFAULT_HOTPLUG_OUT_LOAD		(3)
#define DEFAULT_DIFFERENTIAL			(10)

#define DEFAULT_SAMPLING_PERIOD			(50000)
/* 20s booting not hotplug */
#define BOOTING_SAMPLING_PERIOD			(20000)

/*hotplug task running*/
#define DEFAULT_HOTPLUG_IN_RUN			(3)
#define DEFAULT_HOTPLUG_OUT_RUN			(6)

/*task running threshold*/
#define TASK_THRESHOLD_H				(199)
#define TASK_THRESHOLD_L				(110)

/*CPU NUM WATERSHED*/
#define CPU_NUM_WATERSHED				(2)

/* default number of sampling periods to average before hotplug-in decision */
#define DEFAULT_HOTPLUG_IN_SAMPLING_PERIODS		(3)

/* default number of sampling periods to average before hotplug-out decision */
#define DEFAULT_HOTPLUG_OUT_SAMPLING_PERIODS	(20)
#define DEFAULT_HOTPLUG_OUT_SAMPLING_PERIODS1	(200)

struct cpu_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
};
static DEFINE_PER_CPU(struct cpu_info_s, hp_cpu_info);

static struct delayed_work k_work;

/*average nr_running on cpu*/
static int g_iavruning = 0;
static int g_iavraddcnt = 0;
static int g_iavrsubcnt = 0;
static int hotplug_in_count = 0;
static int hotplug_out_count = 0;

#ifdef CONFIG_IPPS_SUPPORT
static void ippsclient_add(struct ipps_device *device)
{
}

static void ippsclient_remove(struct ipps_device *device)
{
}

static struct ipps_client vcc_ipps_client = {
	.name   = "cpufreq_k3hotplug",
	.add    = ippsclient_add,
	.remove = ippsclient_remove
};
#endif

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time;
	u64 iowait_time;

	/* cpufreq-hotplug always assumes CONFIG_NO_HZ */
	idle_time = get_cpu_idle_time_us(cpu, wall);

	iowait_time = get_cpu_iowait_time_us(cpu, wall);

	/* cpufreq-hotplug always assumes CONFIG_NO_HZ */
	if (iowait_time != -1ULL && idle_time >= iowait_time)
		idle_time -= iowait_time;

	return idle_time;
}

static void auto_hotplug(void)
{
	/* single largest CPU load percentage*/
	unsigned int max_load = 0;
	unsigned int min_load = 100;
	int cpun = 0;
	unsigned int j;

	cpufreq_get(0);

	/*
	 * cpu load accounting
	 * get highest load, total load and average load across all CPUs
	 */
	for_each_online_cpu(j) {
		unsigned int load;
		unsigned int idle_time, wall_time;
		cputime64_t cur_wall_time = 0, cur_idle_time;
		struct cpu_info_s *j_info;

		j_info = &per_cpu(hp_cpu_info, j);

		/* update both cur_idle_time and cur_wall_time */
		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);

		/* how much wall time has passed since last iteration? */
		wall_time = (unsigned int) cputime64_sub(cur_wall_time,
				j_info->prev_cpu_wall);
		j_info->prev_cpu_wall = cur_wall_time;

		/* how much idle time has passed since last iteration? */
		idle_time = (unsigned int) cputime64_sub(cur_idle_time,
				j_info->prev_cpu_idle);

		j_info->prev_cpu_idle = cur_idle_time;

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		/* load is the percentage of time not spent in idle */
		load = 100 * (wall_time - idle_time) / wall_time;

		/* keep track of highest single load across all CPUs */
		if (load > max_load)
			max_load = load;
		if (load < min_load)
			min_load = load;
	}

	cpun = num_online_cpus();

	/*avg running task on each cpu*/
	g_iavruning = nr_running() * 100 / cpun;

	if (g_iavruning > TASK_THRESHOLD_H) {
		g_iavraddcnt ++ ;
		g_iavrsubcnt = 0 ;
	} else if (g_iavruning < TASK_THRESHOLD_L) {
		g_iavraddcnt = 0 ;
		g_iavrsubcnt ++ ;
	}

	/*min_load bigger than (95-(cpu number-1)*10)*/
	if (min_load > (DEFAULT_HOTPLUG_IN_LOAD - (cpun-1) * DEFAULT_DIFFERENTIAL)) {
		hotplug_in_count++;
		hotplug_out_count = 0;
	} else if ((cpun > CPU_NUM_WATERSHED)
		&& ((max_load + min_load) < DEFAULT_HOTPLUG_IN_LOAD
			|| min_load < cpun*DEFAULT_HOTPLUG_OUT_LOAD)) {
		/*max+min load lower than 95 or min load lower than cpun * 5*/
		hotplug_out_count++;
		hotplug_in_count = 0;
	} else if ((cpun <= CPU_NUM_WATERSHED) && (max_load + min_load) < DEFAULT_HOTPLUG_IN_LOAD) {
		/*max+min load lower than 95*/
		hotplug_out_count++;
		hotplug_in_count = 0;
	}

	if ((g_iavraddcnt >= DEFAULT_HOTPLUG_IN_RUN)
		&& (hotplug_in_count >= DEFAULT_HOTPLUG_IN_SAMPLING_PERIODS)
		&& (gcpu_num_limit.max > cpun)) {
#ifdef CONFIG_HOTPLUG_CPU
		cpu_up(num_online_cpus());
#endif
		hotplug_out_count = 0;
		hotplug_in_count = hotplug_in_count/2;
		g_iavraddcnt = 0 ;
		g_iavrsubcnt = 0 ;

		return;
	}

	if (((g_iavrsubcnt >= DEFAULT_HOTPLUG_OUT_RUN)
		&& (gcpu_num_limit.min < cpun))
		&& (((cpun > CPU_NUM_WATERSHED) && (hotplug_out_count >= DEFAULT_HOTPLUG_OUT_SAMPLING_PERIODS))
			|| ((cpun <= CPU_NUM_WATERSHED) && (hotplug_out_count >= DEFAULT_HOTPLUG_OUT_SAMPLING_PERIODS1)))) {

#ifdef CONFIG_HOTPLUG_CPU
		cpu_down(num_online_cpus()-1);
#endif
		hotplug_in_count  = 0;
		hotplug_out_count = 0;
		g_iavraddcnt = 0 ;
		g_iavrsubcnt = 0 ;

		return;
	}
}

static void do_dbs_timer(struct work_struct *work)
{
	int delay = usecs_to_jiffies(DEFAULT_SAMPLING_PERIOD);
	delay -= jiffies % delay;
	if (gcpu_num_limit.block == 0) {
		auto_hotplug();
	}
	schedule_delayed_work_on(0, &k_work, delay);
}

static inline void dbs_timer_init(void)
{
	INIT_DELAYED_WORK_DEFERRABLE(&k_work, do_dbs_timer);

	if (RUNMODE_FLAG_NORMAL == runmode_is_factory())
		schedule_delayed_work_on(0, &k_work, usecs_to_jiffies(DEFAULT_SAMPLING_PERIOD));
}

static inline void dbs_timer_exit(void)
{
	cancel_delayed_work_sync(&k_work);
}

#endif

/********************cpu hotplug end**************************/
void k3hotplug_pm_qos_add(void)
{
	int i = 0;
	int ilength = sizeof(pm_qos_list)/sizeof(struct pm_qos_lst);

	for (i = 0; i < ilength; i++) {
		pm_qos_add_request(pm_qos_list[i].lst, pm_qos_list[i].qos_class,
			pm_qos_list[i].dvalue);
	}
}

void k3hotplug_pm_qos_remove(void)
{
	int i = 0;
	int ilength = sizeof(pm_qos_list)/sizeof(struct pm_qos_lst);

	for (i = 0; i < ilength; i++)
		pm_qos_remove_request(pm_qos_list[i].lst);
}

static int cpufreq_pm_notify(struct notifier_block *nfb, unsigned long action, void *ignored)
{
	pr_info("%s %ld +\n", __func__, action);
	switch (action)
	{
	case PM_SUSPEND_PREPARE:
	case PM_HIBERNATION_PREPARE:
		dbs_timer_exit();
		break;
	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
		dbs_timer_init();
		break;
	default:
		pr_warn("%s %d %ld\n", __func__, __LINE__, action);
		break;
	}

	pr_info("%s -\n", __func__);

	return NOTIFY_OK;
}

static struct notifier_block hotplug_pm_nb = {
	.notifier_call = cpufreq_pm_notify,
};

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	unsigned int uippsmode = 0;
	struct cpu_dbs_info_s *this_dbs_info = NULL;

	this_dbs_info = &per_cpu(hp_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		mutex_lock(&dbs_mutex);
		dbs_enable++;

		this_dbs_info->cpu = cpu;
		this_dbs_info->freq_table = cpufreq_frequency_get_table(cpu);

		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (cpu == 0) {
#ifdef CONFIG_IPPS_SUPPORT
			uippsmode = IPPS_DVFS_AVS_ENABLE;
			ipps_set_func(&vcc_ipps_client, IPPS_OBJ_CPU, &uippsmode);
#endif
			dbs_timer_init();
			k3hotplug_pm_qos_add();
			register_pm_notifier(&hotplug_pm_nb);
		}

		mutex_unlock(&dbs_mutex);

		break;

	case CPUFREQ_GOV_STOP:

		mutex_lock(&dbs_mutex);

		dbs_enable--;

		if (cpu == 0) {
			dbs_timer_exit();
			k3hotplug_pm_qos_remove();
#ifdef CONFIG_IPPS_SUPPORT
			uippsmode = IPPS_DVFS_AVS_DISABLE;
			ipps_set_func(&vcc_ipps_client, IPPS_OBJ_CPU, &uippsmode);
#endif
			unregister_pm_notifier(&hotplug_pm_nb);
		}

		mutex_unlock(&dbs_mutex);

		break;

	case CPUFREQ_GOV_LIMITS:
		if (cpu == 0) {
			mutex_lock(&dbs_mutex);
			pm_qos_update_request(&g_cpumaxlimits, policy->max);
			pm_qos_update_request(&g_cpuminlimits, policy->min);
			mutex_unlock(&dbs_mutex);
		}
		break;
	default:
		pr_err("[%s] defualt run=%x\n", __func__, event);
		break;
	}
	return 0;
}

static int hotplug_reboot_notify(struct notifier_block *nb,
				unsigned long code, void *unused)
{
	if ((code == SYS_RESTART) || (code == SYS_POWER_OFF) ||
		(code == SYS_HALT)) {
		printk("hotplug_reboot_notify code 0x%lx stop hotplug\n", code);
		dbs_timer_exit();
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static struct notifier_block hotplug_reboot_nb = {
	.notifier_call	= hotplug_reboot_notify,
	.next		= NULL,
	.priority	= INT_MAX, /* before any real devices */
};

static int __init cpufreq_gov_dbs_init(void)
{
	int err = 0;
	dbs_enable = 0;

#ifdef CONFIG_IPPS_SUPPORT
	err = ipps_register_client(&vcc_ipps_client);
	if (err != 0) {
		printk("regulator vcc register client failed, please check!");
		return err;
	}
#endif

	err = cpufreq_register_governor(&cpufreq_gov_k3hotplug);
	if (err)
		pr_err("cpufreq_gov_k3hotplug register err=%d\n", err);

	register_reboot_notifier(&hotplug_reboot_nb);
	return err;
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	unregister_reboot_notifier(&hotplug_reboot_nb);
	cpufreq_unregister_governor(&cpufreq_gov_k3hotplug);
#ifdef CONFIG_IPPS_SUPPORT
	ipps_unregister_client(&vcc_ipps_client);
#endif
}

MODULE_AUTHOR("s00107748");
MODULE_DESCRIPTION("'cpufreq_k3hotplug' - cpufreq governor for hotplug");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_K3HOTPLUG
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
