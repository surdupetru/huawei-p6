/*
 *  linux/arch/arm/mach-k3v2/localtimer.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/clockchips.h>
#include <asm/localtimer.h>
#include <asm/smp_twd.h>
#include <asm/irq.h>
#include <asm/localtimer.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <asm/hardware/arm_timer.h>
#include <asm/mach/time.h>
#include <mach/early-debug.h>
#include <mach/platform.h>
#include <mach/hardware.h>
#include <mach/hisi_cortex-a9.h>
#include <mach/irqs.h>

/*
 * These timers are currently always setup to be clocked at 26MHz.
 */
#define TIMER_RELOAD (BUSCLK_TO_TIMER_RELOAD(CONFIG_DEFAULT_TIMERCLK))

/* per-cpu timer name */
#define MAX_CPU_NAME 8

/* enable timer bit */
#define TIMER2_ENABLE_BIT  1<<3
#define TIMER3_ENABLE_BIT  1<<4

static unsigned long timer0_clk_hz = (CONFIG_DEFAULT_TIMERCLK);

static void __iomem *clkevt_base[NR_CPUS] = {NULL};
static unsigned int cpu_irq_status[NR_CPUS] __cacheline_aligned_in_smp = {
	[0 ... NR_CPUS-1] = 0,
};

static void sp804_set_mode(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	unsigned long ctrl = TIMER_CTRL_32BIT | TIMER_CTRL_IE;
	unsigned int cpu = smp_processor_id();

    writel(1, clkevt_base[cpu] + TIMER_INTCLR);
	writel(ctrl, clkevt_base[cpu] + TIMER_CTRL);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		writel(TIMER_RELOAD, clkevt_base[cpu] + TIMER_LOAD);
		ctrl |= TIMER_CTRL_PERIODIC | TIMER_CTRL_ENABLE;
		break;

	case CLOCK_EVT_MODE_ONESHOT:
		/* period set, and timer enabled in 'next_event' hook */
		ctrl |= TIMER_CTRL_ONESHOT;
		break;

	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		break;
	}

	writel(ctrl, clkevt_base[cpu] + TIMER_CTRL);
}

static int sp804_set_next_event(unsigned long next,
	struct clock_event_device *evt)
{
	unsigned int cpu = smp_processor_id();
	unsigned long ctrl = readl(clkevt_base[cpu] + TIMER_CTRL);

	writel(next, clkevt_base[cpu] + TIMER_LOAD);
	writel(ctrl | TIMER_CTRL_ENABLE, clkevt_base[cpu] + TIMER_CTRL);

	return 0;
}

static unsigned char cpu_name[NR_CPUS][MAX_CPU_NAME];
static void sp804_clockevents_init(struct clock_event_device *evt)
{
	unsigned int cpu = smp_processor_id();
	unsigned long ctrl = TIMER_CTRL_32BIT | TIMER_CTRL_IE;

	edb_trace(1);

    writel(1, clkevt_base[cpu] + TIMER_INTCLR);
	writel(ctrl, clkevt_base[cpu] + TIMER_CTRL);

	writel(TIMER_RELOAD, clkevt_base[cpu] + TIMER_LOAD);

	ctrl |= TIMER_CTRL_PERIODIC | TIMER_CTRL_ENABLE;

	writel(ctrl, clkevt_base[cpu] + TIMER_CTRL);

	snprintf(&cpu_name[cpu][0], 7, "timer%d", cpu);

	evt->name = &cpu_name[cpu][0];

	/* NOTES: no need CLOCK_EVT_FEAT_C3STOP */
	evt->features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	evt->rating = 300;
	evt->shift = 32;
	evt->set_mode = sp804_set_mode;
	evt->set_next_event = sp804_set_next_event;

	evt->mult = div_sc(timer0_clk_hz, NSEC_PER_SEC, evt->shift);
	evt->max_delta_ns = clockevent_delta2ns(0xffffffff, evt);
	evt->min_delta_ns = clockevent_delta2ns(0xf, evt);

	clockevents_register_device(evt);
}

/*
 * IRQ handler for the system timer
 */
static irqreturn_t local_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;
	unsigned int cpu = smp_processor_id();
	unsigned int orig_cpu;

	if(unlikely(irq < IRQ_TIMER4) || unlikely(irq > IRQ_TIMER6)){
		pr_err("error irq handle is %d\n", irq);
		return IRQ_HANDLED;
	}
	orig_cpu = irq - IRQ_TIMER4 + 1;

	/* when cpu hotplug, irq move to cpu0 */
	if (unlikely(cpu != orig_cpu)) {
		unsigned int i;
		//pr_warn("warning cpu %d clear time irq %d\n", cpu, irq);

		if(clkevt_base[orig_cpu] == NULL){
			pr_err("error local timer in cpu%d not set\n", orig_cpu);
			return IRQ_HANDLED;
		}

		if ((readl(clkevt_base[orig_cpu] + TIMER_RIS)) & 0x1) {
			writel(1, clkevt_base[orig_cpu] + TIMER_INTCLR);
		}
	} else {
		if(clkevt_base[cpu] == NULL) {
			pr_err("error local timer in cpu%d not set\n", cpu);
			return IRQ_HANDLED;
		}
		if ((readl(clkevt_base[cpu] + TIMER_RIS)) & 0x1) {

			/* clear the interrupt */
			writel(1, clkevt_base[cpu] + TIMER_INTCLR);
			evt->event_handler(evt);
		}
	}
	return IRQ_HANDLED;
}

static struct irqaction local_timer_irq[CONFIG_NR_CPUS] = {
#if (CONFIG_NR_CPUS >= 2)
	[1] = {
		.name		= "cpu1-timer",
		.flags		= IRQF_SHARED | IRQF_DISABLED | IRQF_TIMER,
		.handler	= local_timer_interrupt,
	},
#endif

#if (CONFIG_NR_CPUS >= 3)
	[2] = {
		.name		= "cpu2-timer",
		.flags		= IRQF_SHARED | IRQF_DISABLED | IRQF_TIMER,
		.handler	= local_timer_interrupt,
	},
#endif

#if (CONFIG_NR_CPUS >= 4)
	[3] = {
		.name		= "cpu3-timer",
		.flags		= IRQF_SHARED | IRQF_DISABLED | IRQF_TIMER,
		.handler	= local_timer_interrupt,
	},
#endif
};

static unsigned char timer_name[CONFIG_NR_CPUS][12] = {
#if (CONFIG_NR_CPUS >= 2)
	[1] = "clk_timer2",
#endif
#if (CONFIG_NR_CPUS >= 3)
	[2] = "clk_timer2",
#endif
#if (CONFIG_NR_CPUS >= 4)
	[3] = "clk_timer3",
#endif
};

static void local_timer_clkenable(int cpu)
{
	unsigned long ctrl=0;

	if((cpu == 1)||(cpu == 2)){
		//BIT MAP, only bit 1 take effect.
		ctrl = TIMER2_ENABLE_BIT;
		writel(ctrl,(IO_ADDRESS(REG_BASE_SCTRL)+0x40));
	}
	else if(cpu == 3){
		ctrl = TIMER3_ENABLE_BIT;
		writel(ctrl,(IO_ADDRESS(REG_BASE_SCTRL)+0x40));
	}
}


/*
 * Setup the local clock events for a CPU.
 * K3V2 using timer01 timre23 for per-CPU timer
 */
int __cpuinit local_timer_setup(struct clock_event_device *evt)
{
	unsigned int cpu = smp_processor_id();

	edb_putstr("local timer setup in cpu ");
	edb_puthex(cpu);
	edb_putstr("\n");

	if (!cpu) {
		/* FIXME: cpu0 timer already setup in timer.c */
		pr_info("local timer setup already set in cpu0\n");
		return 0;
	}
#if (CONFIG_NR_CPUS >= 2)
	else if(cpu == 1) {
		evt->irq = IRQ_TIMER4;
		local_timer_irq[cpu].dev_id = evt;
		clkevt_base[cpu] = (void *) IO_ADDRESS(REG_BASE_TIMER2);
	}
#endif
#if (CONFIG_NR_CPUS >= 3)
	else if (cpu == 2) {
		evt->irq = IRQ_TIMER5;
		local_timer_irq[cpu].dev_id = evt;
		clkevt_base[cpu] = (void *) IO_ADDRESS(REG_BASE_TIMER2) + 0x20;
	}
#endif
#if (CONFIG_NR_CPUS >= 4)
	else if (cpu == 3) {
		evt->irq = IRQ_TIMER6;
		local_timer_irq[cpu].dev_id = evt;
		clkevt_base[cpu] = (void *) IO_ADDRESS(REG_BASE_TIMER3);
	}
#endif
	//pr_info("local timer in cpu[%d] timer base[0x%lx] with irq[%d]\n",
	//		cpu, (unsigned long) clkevt_base[cpu], evt->irq);

	edb_trace(1);

	local_timer_clkenable(cpu);

	//pr_info("local_timer_setup clock event init\n");

	/* outer timer need register once per-CPU */
	if(cpu_irq_status[cpu] == 0) {
		edb_putstr("setup irq in cpu\n");

		setup_irq(evt->irq, &local_timer_irq[cpu]);

		cpu_irq_status[cpu] = 1;

		edb_trace(1);
	}

	k3v2_irqaffinity_register(evt->irq, cpu);

	sp804_clockevents_init(evt);

	//pr_info("local_timer_setup fine\n");

	edb_trace(1);

	return 0;
}

/*
 *using to stop per-cpu timer;using in kernel/arch/arm/kernel/smp.c
 * */
void local_timer_stop(struct clock_event_device *evt)
{
	unsigned int cpu = smp_processor_id();

	if (cpu == 0) {
		/*FIXME:local_timer_setup do nothing about the cup0's timer */
		return ;
	}
#if (CONFIG_NR_CPUS >= 2)
	else if(cpu == 1) {
		if (cpu_irq_status[cpu] == 1) {
			remove_irq(evt->irq, &local_timer_irq[cpu]);
			cpu_irq_status[cpu] = 0;
		}
		return ;
	}
#endif
#if (CONFIG_NR_CPUS >= 3)
	else if (cpu == 2) {
		if (cpu_irq_status[cpu] == 1) {
			remove_irq(evt->irq, &local_timer_irq[cpu]);
			cpu_irq_status[cpu] = 0;
		}
		return ;
	}
#endif
#if (CONFIG_NR_CPUS >= 4)
	else if (cpu == 3) {
		if (cpu_irq_status[cpu] == 1) {
			remove_irq(evt->irq, &local_timer_irq[cpu]);
			cpu_irq_status[cpu] = 0;
		}
		return ;
	}
#endif
	else
		printk("cann't stop the unkown cpu");
    return ;
}
