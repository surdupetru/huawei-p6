/*
 * linux/arch/arm/mach-k3v2/k3v2_cpuidle.c
 *
 * K3V2 CPU IDLE Routines
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/sched.h>
#include <linux/cpuidle.h>
#include <asm/proc-fns.h>
#include <linux/io.h>
#include <mach/hipm.h>
#include <linux/suspend.h>
#include <linux/pm_qos_params.h>

#ifdef CONFIG_CPU_IDLE
#define NUM_OF_PLL_REGISTER					2
#define APLL0_CTRL_REG						0x010		/* APLL0 Control */
#define APLL1_CTRL_REG						0x014		/* APLL1 Control */
#define PLL_BASE_REG						0xfca08000   /* Regulator Base*/

#define PLL_NORMAL_MODE_ENA				0x0
#define PLL_BYPASS_MODE_ENA				0x2
#define PLL_BYPASS_ENA_MASK				0x2

struct cpuidle_params {
	unsigned int	exit_latency; /* in US */
	unsigned int	target_residency; /* in US */
	unsigned int	flags;
};

static const u16 pll_ctrl_to_reg[NUM_OF_PLL_REGISTER] = {
	APLL0_CTRL_REG,
	APLL1_CTRL_REG,
};

static unsigned int pll_base;
static unsigned int ususpendflg;

void (*cpuenteridle)();

/*
 * The latencies/thresholds for various C states have
 * to be configured from the respective board files.
 * These are some default values (which might not provide
 * the best power savings) used on boards which do not
 * pass these details from the board file.
 */
static struct cpuidle_params cpuidle_params_table[] = {
	/* PLL Bypass */
	{0, 0, 1},
	{1000, 15000, 1},
};

#define K3V2_NUM_STATES ARRAY_SIZE(cpuidle_params_table)

static int cpuidle_pm_notify(struct notifier_block *nfb, unsigned long action, void *ignored)
{
	switch (action) {
	case PM_SUSPEND_PREPARE:
		disable_hlt();
		pr_info("PM_SUSPEND_PREPARE for CPUIDLE\n");
		return NOTIFY_OK;
	case PM_POST_RESTORE:
	case PM_POST_SUSPEND:
		enable_hlt();
		pr_info("PM_POST_SUSPEND for CPUIDLE\n");
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static struct notifier_block cpuidle_pm_nb = {
	.notifier_call = cpuidle_pm_notify,
};

static inline void pll_state_set_bits(void)
{
	u32 tmp , vsel;
	vsel = pll_base + 0x10;
	tmp = readl(vsel);
	tmp = tmp | PLL_BYPASS_MODE_ENA;
	writel(tmp, vsel);

	vsel = pll_base + 0x14;
	tmp = readl(vsel);
	tmp = tmp | PLL_BYPASS_MODE_ENA;
	writel(tmp, vsel);
}

static inline void pll_state_set_bits_recover(void)
{
	u32 tmp , vsel;

	vsel = pll_base + 0x10;
	tmp = readl(vsel);
	tmp = tmp & 0xFFFFFFFD;
	writel(tmp, vsel);

	vsel = pll_base + 0x14;
	tmp = readl(vsel);
	tmp = tmp & 0xFFFFFFFD;
	writel(tmp, vsel);
}

static inline int canidle()
{
	u32 tmp = readl(0xFE2A205C);
	if (tmp & 0x00F05800)
		return 0;

	tmp = readl(0xFE2A203C);
	if(tmp & 0x100)
		return 0;

	return 1;
}

/**
 * k3v2_enter_idle - Programs K3V2 to enter the specified state
 * @dev: cpuidle device
 * @state: The target state to be programmed
 *
 * Called from the CPUidle framework to program the device to the
 * specified target state selected by the governor.
 */
static inline int k3v2_enter_wfi(struct cpuidle_device *dev,
			struct cpuidle_state *state)
{
	struct timeval before, after;
	int idle_time;

	local_irq_disable();

	/* Used to keep track of the total time in idle */
	getnstimeofday(&before);

	cpu_do_idle();

	getnstimeofday(&after);

	local_irq_enable();

	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

	return idle_time;
}

static inline int k3v2_enter_ddrself(struct cpuidle_device *dev,
			struct cpuidle_state *state)
{
	struct timeval before, after;
	int idle_time;

	local_irq_disable();

	/* Used to keep track of the total time in idle */
	getnstimeofday(&before);

	pll_state_set_bits();

	cpuenteridle();

	pll_state_set_bits_recover();

	getnstimeofday(&after);

	local_irq_enable();

	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

	return idle_time;
}

static int k3v2_enter_lowpm(struct cpuidle_device *dev,
			struct cpuidle_state *state)
{
	int icanidle = 0;
	int idle_time;
	struct cpuidle_state *new_state = state;

	icanidle = canidle();

	if ((num_online_cpus() != 1) || (0 == icanidle)) {
		new_state = dev->safe_state;
	}

	dev->last_state = new_state;

	if (new_state == &dev->states[0])
		idle_time = k3v2_enter_wfi(dev, new_state);
	else
		idle_time = k3v2_enter_ddrself(dev, new_state);

	return idle_time;
}

static struct cpuidle_state k3v2_cpuidle_set[] = {
	[0] = {
		.enter			= k3v2_enter_wfi,
		.exit_latency		= 1,
		.target_residency	= 100,
		.flags			= CPUIDLE_FLAG_TIME_VALID,
		.name			= "IDLE",
		.desc			= "ARM clock gating(WFI)",
	},
	[1] = {
		.enter			= k3v2_enter_lowpm,
		.exit_latency		= 30,
		.target_residency	= 100,
		.flags			= CPUIDLE_FLAG_TIME_VALID,
		.name			= "LOW_POWER",
		.desc			= "ARM power down",
	},
};


DEFINE_PER_CPU(struct cpuidle_device, k3v2_idle_dev);

struct cpuidle_driver k3v2_idle_driver = {
	.name =		"k3v2_idle",
	.owner =	THIS_MODULE,
};

#if 0
static struct pm_qos_request_list g_cpulockimits;
#endif

/**
 * k3v2_idle_init - Init routine for k3v2 idle
 *
 * Registers the K3V2 specific cpuidle driver to the cpuidle
 * framework with the valid set of states.
 */
int __init k3v2_idle_init(void)
{
	struct cpuidle_device *dev;
	int i, max_cpuidle_state, cpu_id;

	cpuenteridle = (void *)IO_ADDRESS(DISMMU_CODE_IN_SECURAM);

	pll_base = IO_ADDRESS(PLL_BASE_REG);

#if 0
	/*lock to maxcpu*/
	pm_qos_add_request(&g_cpulockimits, PM_QOS_CPU_NUMBER_LOCK,
		PM_QOS_CPU_NUMBER_MAX_DEFAULT_VALUE);
#endif

	cpuidle_register_driver(&k3v2_idle_driver);

	for_each_cpu(cpu_id, cpu_online_mask) {
		dev = &per_cpu(k3v2_idle_dev, cpu_id);
		dev->cpu = cpu_id;

		printk(KERN_ERR "cpu=%d\n", dev->cpu);

		if (cpu_id == 0)
			dev->state_count = ARRAY_SIZE(k3v2_cpuidle_set);
		else
			dev->state_count = 1;	/* Support IDLE only */

		max_cpuidle_state = dev->state_count;

		for (i = 0; i < max_cpuidle_state; i++) {
			memcpy(&dev->states[i], &k3v2_cpuidle_set[i],
					sizeof(struct cpuidle_state));
		}

		dev->safe_state = &dev->states[0];

		if (cpuidle_register_device(dev)) {
			cpuidle_unregister_driver(&k3v2_idle_driver);
			printk(KERN_ERR "CPUidle register device failed\n,");
			return -EIO;
		}
	}

#if 0
	pm_qos_remove_request(&g_cpulockimits);
#endif

	register_pm_notifier(&cpuidle_pm_nb);

	return 0;
}
#else
int __init k3v2_idle_init(void)
{
	return 0;
}
#endif /* CONFIG_CPU_IDLE */
late_initcall(k3v2_idle_init);

