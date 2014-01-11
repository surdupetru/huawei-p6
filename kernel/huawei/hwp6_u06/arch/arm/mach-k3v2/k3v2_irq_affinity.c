/*
 *  arch/arm/mach-k3v2/xxx.c
 *
 *  Copyright (C) 2011 Hisilicon Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/smp.h>
#include <linux/cpumask.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/cpu.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <mach/irqs.h>

static DEFINE_SPINLOCK(irqaff_lock);
#ifdef CONFIG_SMP
static unsigned long irqaffinity_mask[NR_CPUS][NR_IRQS] = {
	[0 ... NR_CPUS-1] = {0},
};

int k3v2_irqaffinity_register(unsigned int irq, int cpu)
{
	unsigned int i;

	if (cpu >= NR_CPUS) {
		pr_err(KERN_ERR "irq affinity set error cpu %d larger than max cpu num\n", cpu);
		return -EINVAL;
	}

	spin_lock(&irqaff_lock);

	for (i = 1; i < NR_CPUS; i++) {
		if (i == cpu)
			irqaffinity_mask[i][irq] = 1;
		else
			irqaffinity_mask[i][irq] = 0;
	}

	spin_unlock(&irqaff_lock);

	irq_set_affinity(irq, cpumask_of(cpu));

	pr_info("k3v2 irqaffinity irq %u register irq to cpu %d\n", irq, cpu);

	return 0;
}
EXPORT_SYMBOL_GPL(k3v2_irqaffinity_register);

void k3v2_irqaffinity_unregister(unsigned int irq)
{
	unsigned int cpu = 0;
	unsigned int i;

	spin_lock(&irqaff_lock);

	for (i = 1; i < NR_CPUS; i++) {
		if(irqaffinity_mask[i][irq])
			cpu = i;
		irqaffinity_mask[cpu][irq] = 0;
	}

	spin_unlock(&irqaff_lock);

	pr_info("k3v2 irqaffinity irq %u unregister irq from cpu %d to cpu 0\n", irq, cpu);

	irq_set_affinity(irq, cpumask_of(0));

	return;
}
EXPORT_SYMBOL_GPL(k3v2_irqaffinity_unregister);

static int __cpuinit k3v2_hotplug_notify(struct notifier_block *self,
				      unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned int) hcpu;
	unsigned int irq;

	action &= 0xf;

	switch (action) {
		case CPU_ONLINE:
			for (irq = 0; irq  < NR_IRQS; irq++) {
				/* set irq to affinity cpu when this cpu online */
				if(irqaffinity_mask[cpu][irq]) {
					irq_set_affinity(irq, cpumask_of(cpu));
					/*pr_info("k3v2 set irq %d affinity to cpu %d again\n", irq, cpu);*/
				}
			}
		default:
			break;
	}

	return NOTIFY_OK;
}
#else
#define k3v2_hotplug_notify	NULL
#endif

static int __init k3v2_irqaffinity_init(void)
{
	/* Register hotplug notifier. */
	hotcpu_notifier(k3v2_hotplug_notify, 0);

	pr_info("k3v2 irqaffinity init\n");

	return 0;
}
module_init(k3v2_irqaffinity_init);
