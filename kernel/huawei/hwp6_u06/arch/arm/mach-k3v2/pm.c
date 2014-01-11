/*
 *
 * arch/arm/mach-k3v2/pm.c
 *
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 * Basic platform init and mapping functions.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <asm/memory.h>
#include <mach/system.h>
#include <mach/platform.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <asm/mach/time.h>
#include <mach/irqs.h>
#include <mach/hipm.h>
#include <mach/early-debug.h>
#include <linux/android_pmem.h>
#include <asm/hardware/gic.h>
#include <asm/hardware/arm_timer.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <mach/io.h>
#include <mach/gpio.h>
#include <linux/clk.h>
#include <linux/wakelock.h>
#include <mach/boardid.h>
#include <asm/cacheflush.h>
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
#include <linux/cpufreq-k3v2.h>
extern struct cpu_num_limit gcpu_num_limit;
#endif
#include "clock.h"
#ifdef CONFIG_CACHE_L2X0
extern int hisik3_pm_disable_l2x0(void);
extern int hisik3_pm_enable_l2x0(void);
#endif

extern void __iomem *gic_cpu_base_addr;
void __iomem *gic_dist_base_addr;

/* CPU0 use timer0 as input clk, before enter suspend status
   we disable the timer0_clk while enable timer0_clk when resume
   from suspend status.  */
static struct clk *timer0_clk;
static unsigned long timer0_base_addr;
unsigned long saved_interrupt_mask[128];
unsigned long saved_cpu_target_mask[128];

static int hisik3_pm_save_gic(void);
static int hisik3_pm_retore_gic(void);

typedef struct __timer_register {
	unsigned long timer_load;
	unsigned long timer_value;
	unsigned long timer_ctrl;
	unsigned long timer_bgload;
	unsigned long timer_control;
} timer_register;

static timer_register timer0[2];

static int in_panic;

static int panic_prep_restart(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	in_panic = 1;
	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call	= panic_prep_restart,
};


/* Protect and restore timer0_0 timer0_1 registert value */
static int protect_timer0_register(void)
{

	/* protect timer0_0 / timer0_1  timer_load value */
	timer0[0].timer_load = readl(timer0_base_addr + TIMER_LOAD);
	timer0[1].timer_load = readl(timer0_base_addr + 0x20 + TIMER_LOAD);

	/* protect timer0_0 / timer0_1  timer_load value */
	timer0[0].timer_value = readl(timer0_base_addr + TIMER_VALUE);
	timer0[1].timer_value = readl(timer0_base_addr + 0x20 + TIMER_VALUE);

	/* protect timer0_0 timer0_1 timer_ctrl value */
	timer0[0].timer_ctrl = readl(timer0_base_addr + TIMER_CTRL);
	timer0[1].timer_ctrl = readl(timer0_base_addr + 0x20 + TIMER_CTRL);

	/* protect timer0_0 timer0_1 timer_bgload value */
	timer0[0].timer_bgload = readl(timer0_base_addr + TIMER_BGLOAD);
	timer0[1].timer_bgload = readl(timer0_base_addr + 0x20 + TIMER_BGLOAD);

	/* protect timer control register */
	timer0[0].timer_control = readl(IO_ADDRESS(REG_BASE_SCTRL) + 0x18);

	/* disable timer0_0 timer0_1 */
	writel(0, timer0_base_addr + TIMER_CTRL);
	writel(0, timer0_base_addr + 0x20 + TIMER_CTRL);

#if 0  /* need not control timer0 clk during sr */
	/* disable timer0 clock */
	if (NULL != timer0_clk) {
		clk_disable(timer0_clk);
	}
#endif

	return 0;
}

static int restore_timer0_register(void)
{
#if 0  /* need not control timer0 clk during sr */
	/* enable timer0 clock */
	if (NULL != timer0_clk) {
		clk_enable(timer0_clk);
	}
#endif
	/* restore timer control register */
	writel(timer0[0].timer_control,IO_ADDRESS(REG_BASE_SCTRL) + 0x18);
	/* disable timer0_0 timer0_1 */
	writel(0, timer0_base_addr + TIMER_CTRL);
	writel(0, timer0_base_addr + 0x20 + TIMER_CTRL);

	/* clear timer0_0 timer0_1 intr */
	writel(1, timer0_base_addr + TIMER_INTCLR);
	writel(1, timer0_base_addr + 0x20 + TIMER_INTCLR);

	/* restore timer0_0 timer0_1 load value to load before enable */
	writel(timer0[0].timer_value, timer0_base_addr + TIMER_LOAD);
	writel(timer0[1].timer_value, timer0_base_addr + 0x20 + TIMER_LOAD);

	/* restore timer0_0 timer0_1 ctrl value */
	writel(timer0[0].timer_ctrl, timer0_base_addr + TIMER_CTRL);
	writel(timer0[1].timer_ctrl, timer0_base_addr + 0x20 + TIMER_CTRL);

	/* restore timer0_0 timer0_1 bgload value. when reg value to zero then to bgload */
	writel(timer0[0].timer_bgload, timer0_base_addr + TIMER_BGLOAD);
	writel(timer0[1].timer_bgload, timer0_base_addr + 0x20 + TIMER_BGLOAD);

	return 0;
}
#define GET_IRQ_BIT(irq)   (1<<((irq)%32))

static int hisik3_pm_save_gic()
{
	unsigned int max_irq, i;
	unsigned int intack;

	/* disable gic dist */
	writel(0 ,  gic_dist_base_addr + GIC_DIST_CTRL);

	/*
	 * Find out how many interrupts are supported.
	 */
	max_irq = readl(gic_dist_base_addr + GIC_DIST_CTR) & 0x1f;
	max_irq = (max_irq + 1) * 32;

	/*
	 * The GIC only supports up to 1020 interrupt sources.
	 * Limit this to either the architected maximum, or the
	 * platform maximum.
	 */
	if (max_irq > max(1020 , NR_IRQS)) {
		max_irq = max(1020 , NR_IRQS);
	}

	/* save Dist target */
	for (i = 32; i < max_irq; i += 4) {
		saved_cpu_target_mask[i/4] =
				readl(gic_dist_base_addr + GIC_DIST_TARGET + i * 4 / 4);
	}

	/* save mask irq */
	for (i = 0; i < max_irq; i += 32) {
		saved_interrupt_mask[i/32] =
				readl(gic_dist_base_addr + GIC_DIST_ENABLE_SET + i * 4 / 32);
	}

	/* clear all interrupt */
	for (i = 0; i < max_irq; i += 32) {
		writel(0xffffffff, gic_dist_base_addr + GIC_DIST_ENABLE_CLEAR + i * 4 / 32);
	}

	/* read INT_ACK in CPU interface, until result is 1023 */
	for(i = 0;i<max_irq;i++) {
		intack = readl(gic_cpu_base_addr + 0x0c);
		if(1023 == intack) {
			break;
		}
		writel(intack,gic_cpu_base_addr + 0x10);
	}

#if 0   /* comment off wakeup intr, cause we will go directly to deepsleep */
	/* enable softinterrupt mask */
	writel(0xffff, gic_dist_base_addr + GIC_DIST_ENABLE_SET);

	/* enable KPC/TBC/RTC interrupt */
	writel(GET_IRQ_BIT(IRQ_KPC)|GET_IRQ_BIT(IRQ_TBC)|GET_IRQ_BIT(IRQ_RTC),
			gic_dist_base_addr + GIC_DIST_ENABLE_SET + 4);

	writel(0, gic_dist_base_addr + GIC_DIST_ENABLE_SET + 8);

	/* enable all gpio interrupt */
	writel(0x3fffff, gic_dist_base_addr + GIC_DIST_ENABLE_SET + 0xc);
#endif
	/* Enable GIC Dist Ctrl, SR needed */
	/* no intr response mode, need not enable dist ctrl */
	//writel(1, gic_dist_base_addr + GIC_DIST_CTRL);

	return 0;
}

static int hisik3_pm_retore_gic()
{
	unsigned int max_irq, i;

	/* PRINT OUT the GIC Status */
	unsigned int irq_status[5];

	for (i = 0; i < 5; i++) {
		irq_status[i] = readl(gic_dist_base_addr + 0xd00 + i*4);
		printk("irq[%02x]=%08x\n", i, irq_status[i]);
	}

	writel(0, gic_dist_base_addr + GIC_DIST_CTRL);
	writel(0, gic_cpu_base_addr + GIC_CPU_CTRL);

	/*
	 * Find out how many interrupts are supported.
	 */
	max_irq = readl(gic_dist_base_addr + GIC_DIST_CTR) & 0x1f;
	max_irq = (max_irq + 1) * 32;

	/*
	 * The GIC only supports up to 1020 interrupt sources.
	 * Limit this to either the architected maximum, or the
	 * platform maximum.
	 */
	if (max_irq > max(1020, NR_IRQS)) {
		max_irq = max(1020, NR_IRQS);
	}

	/*
	 * Set all global interrupts to be level triggered, active low.
	 */
	for (i = 32; i < max_irq; i += 16) {
		writel(0, gic_dist_base_addr + GIC_DIST_CONFIG + i * 4 / 16);
	}

	/*
	 * Set all global interrupts to this CPU only.
	 */
	for (i = 32; i < max_irq; i += 4) {
		writel(saved_cpu_target_mask[i/4], gic_dist_base_addr + GIC_DIST_TARGET + i * 4 / 4);
	}

	/*
	 * Set priority on all interrupts.
	 */
	for (i = 0; i < max_irq; i += 4) {
		writel(0xa0a0a0a0, gic_dist_base_addr + GIC_DIST_PRI + i * 4 / 4);
	}

	/*
	 * Disable all interrupts.
	 */
	for (i = 0; i < max_irq; i += 32) {
		writel(0xffffffff, gic_dist_base_addr + GIC_DIST_ENABLE_CLEAR + i * 4 / 32);
	}


	for (i = 0; i < max_irq; i += 32) {
		writel(saved_interrupt_mask[i/32] , gic_dist_base_addr + GIC_DIST_ENABLE_SET + i * 4 / 32);
	}

	writel(1, gic_dist_base_addr + GIC_DIST_CTRL);

	/* set the BASE priority 0xf0 */
	writel(0xf0, gic_cpu_base_addr + GIC_CPU_PRIMASK);

	writel(1, gic_cpu_base_addr + GIC_CPU_CTRL);

	return 0;
}

#define SECRAM_RESET_ADDR	IO_ADDRESS(REG_BASE_PMUSPI + (0x87 << 2))
#define SECRAM_REST_FLAG_LEN	(0x8)
#define SECRAM_REST_INFO_LEN	(0x20)

#define RESET_COLD_FLAG		"coldboot"
#define RESET_WARM_FLAG		"warmboot"
#define SCTRL_SCSYSSTAT		0x004
#define SCTRL_SCPERRSTEN0	0x080
#define PMU_RST_CTRL		(0x035<<2)

extern void (*k3v2_reset)(char mode, const char *cmd);
static void reboot_board(void)
{
	unsigned long sctrl_addr = (unsigned long)IO_ADDRESS(REG_BASE_SCTRL);

	printk(KERN_EMERG "reboot board...\n");

	while(1) {
		writel(0xdeadbeef, sctrl_addr + SCTRL_SCSYSSTAT);
	}
}

struct k3v2_cmdword
{
	unsigned char name[16];
	unsigned long num;
};

static struct k3v2_cmdword k3v2_map[] =
{
	{"coldboot", 0x10},
	{"bootloader", 0x01},
	{"recovery", 0x02},
	{"resetfactory", 0x03},
	{"resetuser", 0x04},
	{"sdupdate", 0x05},
	{"panic", 0x06},
	{"resize", 0x07},
	{"modemupdate", 0x08},
	{"usbupdate", 0x09},
	{"oem_rtc", 0x0a},
	{"systemcrash", 0x18},	
};

static unsigned long find_rebootmap(const char* str)
{
	unsigned long n = 0;
	unsigned long ret = 0;

	for (n = 0; n < sizeof(k3v2_map)/sizeof(struct k3v2_cmdword); n++) {
		if(!strcmp(k3v2_map[n].name, str)) {
			printk(KERN_INFO "rebootmap = %s\n", k3v2_map[n].name);
			ret = k3v2_map[n].num;
			break;
		}
	}

	return ret;
}

static void _k3v2oem1_reset(char mode, const char *cmd)
{
	unsigned long num = 0;

	printk(KERN_EMERG "_k3v2oem1_reset cmd:%s.\n", cmd);

	if(in_panic)
	{
		num = find_rebootmap("systemcrash");
		writel(num, SECRAM_RESET_ADDR);
		printk(KERN_EMERG "_k3v2oem1_reset type [%s 0x%lx]\n", cmd, num);

		reboot_board();
		return;
	}

	if (cmd == NULL) {
		/* cmd = NULL; case: cold boot */
		num = find_rebootmap(RESET_COLD_FLAG);
		if (readl(SECRAM_RESET_ADDR) & (1 << 5)) {
			num = 0x06;
			printk(KERN_EMERG "_k3v2oem1_reset cmd:panic.\n");
		}
		writel(num, SECRAM_RESET_ADDR);
	}
	else {
		/* cmd != null; case: warm boot */
		if (!strcmp(cmd, "bootloader") ||
			!strcmp(cmd, "recovery") ||
			!strcmp(cmd, "resetfactory") ||
			!strcmp(cmd, "resetuser") ||
			!strcmp(cmd, "oem_rtc") ||
			!strcmp(cmd, "modemupdate") ||
			!strcmp(cmd, "resize") ||
			!strcmp(cmd, "usbupdate") ||
			!strcmp(cmd, "sdupdate")) {

			num = (find_rebootmap(cmd));
			writel(num, SECRAM_RESET_ADDR);
			printk(KERN_EMERG "_k3v2oem1_reset type [%s 0x%lx]\n", cmd, num);
		} else {
			/* otherwise cold boot */
			printk(KERN_EMERG "reboot: non-supported mode [%s]\n", cmd);
			num = find_rebootmap(RESET_COLD_FLAG);
			writel(num, SECRAM_RESET_ADDR);
		}
	}

	printk(KERN_EMERG "reboot: mode reg 0x%x\n", readl(SECRAM_RESET_ADDR));

	reboot_board();
}

static void hisik3_power_off(void);
static void shutdown_wdt_timer(unsigned long code)
{
	printk("shutdown wdt timer out code 0x%lx\n", code);

	if (code == SYS_POWER_OFF) {
		hisik3_power_off();
	}
	else if (code == SYS_RESTART) {
		_k3v2oem1_reset(0, NULL);
	}
}

static struct timer_list shutdown_timer;
static int pm_reboot_notify(struct notifier_block *nb,
				unsigned long code, void *unused)
{
	if ((code == SYS_RESTART) || (code == SYS_POWER_OFF) ||
		(code == SYS_HALT)) {
		printk("pm_reboot_notify code 0x%lx\n", code);

#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
		//disable_nonboot_cpus();
		gcpu_num_limit.max = 1;
		msleep(200);
#endif
		init_timer(&shutdown_timer);
		shutdown_timer.function = shutdown_wdt_timer;
		shutdown_timer.expires = jiffies + (HZ << 2);
		shutdown_timer.data = code;
		add_timer(&shutdown_timer);

		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static struct notifier_block pm_reboot_nb = {
	.notifier_call	= pm_reboot_notify,
	.next		= NULL,
	.priority	= INT_MAX, /* before any real devices */
};

/* system power off func */
static void hisik3_power_off(void)
{
	int ret = gpio_request(GPIO_19_6, 0);
	if (ret != 0)
		printk(KERN_EMERG "request GPIO_19_6 error:%d. \n", ret);

	writel(0, SECRAM_RESET_ADDR); /* clear the abnormal reset flag*/

	while (1) {
		printk(KERN_EMERG "system power off now");

		gpio_direction_output(GPIO_19_6, 0);
		gpio_set_value(GPIO_19_6, 0);

		msleep(1000);
	}

	gpio_free(GPIO_19_6);
}

#ifdef CONFIG_LOWPM_DEBUG
extern void setiolowpower(void);
extern void ioshowstatus(int check);
extern void pmulowpowerall(int isuspend);
extern void pmulowpower_show(int check);
extern void rtc_enable(void);
extern void timer0_0_enable(void);
void timer0_0_disable(void);
void debuguart_reinit(void);
#endif

extern void pmulowpower(int isuspend);
static void pmctrl_reinit(void)
{
	/*
	 *the div of g3d clock has been changed in fastboot,
	 *it's different with the status which was set by
	 *clk_disable(), so reinit here.
	 */
	writel(G3D_DIV_DIS_VAL, G3D_CORE_DIV);
	writel(G3D_DIV_DIS_VAL, G3D_SHADER_DIV);
	writel(G3D_DIV_DIS_VAL, G3D_AXI_DIV);
}
static void sysctrl_reinit(void)
{
	/*
	 *bit 7:gt_clk_ddrc_codec
	 *bit 6:gt_clk_ddrc_disp
	 *bit 4:gt_clk_ddrc_gpu
	 */
	unsigned i = 1000;
	unsigned uregv_status = 0;
	unsigned uregv = (1 << 7) | (1 << 6) | (1 << 4);

	writel(uregv, DISEN_REG3);
	uregv_status = readl(ISEN_REG3) & uregv;
	while (uregv_status & i) {
		writel(uregv, DISEN_REG3);
		uregv_status = readl(ISEN_REG3) & uregv;
		i --;
	}
	if (0 == i)
		WARN(1, "CLOCK:Attempting to write clock enable register 1000 times.\r\n");
}
static void pctrl_reinit(void)
{
	unsigned uregv = 0;

	/*bit 29:reinit test_pddq*/
	uregv = readl(IO_ADDRESS(REG_BASE_PCTRL) + 0x030) | (1 << 29);
	writel(uregv, IO_ADDRESS(REG_BASE_PCTRL) + 0x030);

	/*bit 1:reinit nanophy_siddq*/
	uregv = readl(IO_ADDRESS(REG_BASE_PCTRL) + 0x038) | (1 << 0);
	writel(uregv, IO_ADDRESS(REG_BASE_PCTRL) + 0x038);
}
static int hisik3_pm_enter(suspend_state_t state)
{
	unsigned long flage = 0;

	switch (state) {
		case PM_SUSPEND_STANDBY:
		case PM_SUSPEND_MEM:
			break;
		default:
			return -EINVAL;
	}

	if (has_wake_lock(WAKE_LOCK_SUSPEND)) {
		printk("hisik3_pm_enter has wake lock\n");
		return -EAGAIN;
	}

	local_irq_save(flage);

	hisik3_pm_save_gic();

	flush_cache_all();

#ifdef CONFIG_CACHE_L2X0
	hisik3_pm_disable_l2x0();
#endif

	/*set pmu to low power*/
	pmulowpower(1);

	/* here is an workround way to delay 40ms
         * make sure LDO0 is poweroff very clean */
	mdelay(40);

	/* protect timer0_0 timer0_1 and disable timer0 clk */
	protect_timer0_register();

#ifdef CONFIG_LOWPM_DEBUG
	/*set io to lowpower mode*/
	ioshowstatus(1);
	setiolowpower();
	ioshowstatus(1);

	/*set pmu to low power mode*/
	pmulowpower_show(1);
	pmulowpowerall(1);
	pmulowpower_show(1);
#endif

	edb_putstr("[PM]Enter hilpm_cpu_godpsleep...\r\n");

#ifdef CONFIG_LOWPM_DEBUG
	/*time enable*/
	timer0_0_enable();

	/*rtc*/
	rtc_enable();
#endif

	hilpm_cpu_godpsleep();

	/*
	 *the status has been changed in fastboot,
	 *it causes difference with kernel's status,
	 */
	pmctrl_reinit();
	pctrl_reinit();
	sysctrl_reinit();

	/*uart init.*/
	edb_reinit();

#ifdef CONFIG_LOWPM_DEBUG
	/*restore debug uart0*/
	debuguart_reinit();

	/*disable timer0*/
	timer0_0_disable();

	/*restore pmu config*/
	pmulowpowerall(0);
#endif

	/*PMU regs restore*/
	pmulowpower(0);

	/* restore timer0_0 timer0_1 and enable timer0 clk */
	restore_timer0_register();

	flush_cache_all();

#ifdef CONFIG_CACHE_L2X0
	hisik3_pm_enable_l2x0();
#endif

	hisik3_pm_retore_gic();

	local_irq_restore(flage);

	pr_notice("[PM]Restore OK.\r\n");

	return 0;
}

static struct platform_suspend_ops hisik3_pm_ops = {
	.enter		= hisik3_pm_enter,
	.valid		= suspend_valid_only_mem,
};

extern unsigned long hi_cpu_godpsleep_ddrbase;
extern unsigned long hi_cpu_godpsleep_phybase;

/* save the initial code address  slave CPU,
  which copy from REG_BASE_SCTRL + 0x314
  refer to platsmp.c platform_smp_prepare_cpus()*/
extern unsigned long hi_slave_orig_entry_address;

/* save the suspend back address address of each slave CPUs,
  refer to hilpm-cpugodp.S*/
extern unsigned long hi_slave_mark_phybase;
#define SR_PROTECT_CTX_BUFF_SIZE  (2048)

static int __init hisik3_pm_init(void)
{
	unsigned int addr;

	/* alloc memmory for suspend */
	hi_cpu_godpsleep_ddrbase = (unsigned long)kzalloc((SR_PROTECT_CTX_BUFF_SIZE) , GFP_DMA|GFP_KERNEL);
	if (0 == hi_cpu_godpsleep_ddrbase) {
		pr_err("[PM]kmallc err for hi_cpu_godpsleep_ddrbase!\r\n");
		return -1;
	}

	/* timer0 is for CPU0's tick input, here get timer0's clk handle,
	   we will close it before we enter deep sleep status */
	timer0_clk = clk_get(NULL, "clk_timer0");
	if (NULL == timer0_clk) {
		pr_warn("[PM]WARN:get \"clk_timer0\" clk failed!\r\n");
	}

	hi_cpu_godpsleep_phybase = __pa(hi_cpu_godpsleep_ddrbase);

	pr_notice("[PM]hi_cpu_godpsleep_ddrbase =0x%lx\r\n.hi_cpu_godpsleep_phybase=0x%lx.\r\n",
			hi_cpu_godpsleep_ddrbase,
			hi_cpu_godpsleep_phybase);

	/**
	* slave cpu entry address is already setting to
	* addr = (unsigned long) IO_ADDRESS(REG_BASE_SCTRL + 0x314)
	* we take it out. relpace it.
	**/
	addr = (unsigned long) IO_ADDRESS(REG_BASE_SCTRL + 0x314);
	hi_slave_orig_entry_address = readl(addr);
	hi_slave_mark_phybase = hi_cpu_godpsleep_phybase;

	/* hilpm_cpu_godpsleep() use disable_mmu() and enable_mmu()
	   which running in securam.we copy the two functions during init phase*/
	hilpm_cp_securam_code();

#ifdef CONFIG_CPU_IDLE
	hilpm_cp_cpuidle_code();
#endif

	/* get the base address of timer0 */
	timer0_base_addr = (unsigned long) IO_ADDRESS(REG_BASE_TIMER0);
	/* change the slave cpu entry address */
	writel(SLAVE_CPU_ENTRY_CODE_ADDR, addr);

	addr = (unsigned long) IO_ADDRESS(REG_BASE_SCTRL + 0x10);
	if (get_pmuid() >= 0x24)
		writel(SCXTALCTRL_CFG_VAL_005MS, addr);
	else
		writel(SCXTALCTRL_CFG_VAL_200MS, addr);

	suspend_set_ops(&hisik3_pm_ops);

	/* power off function */
	pm_power_off = hisik3_power_off;

	/* reset notify noboot cpu hotplug */
	k3v2_reset = _k3v2oem1_reset;
	arm_pm_restart = k3v2_reset;

	register_reboot_notifier(&pm_reboot_nb);

	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);

	return 0;
}
module_init(hisik3_pm_init);
