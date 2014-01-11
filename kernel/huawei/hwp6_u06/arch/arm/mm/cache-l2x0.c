/*
 * arch/arm/mm/cache-l2x0.c - L210/L220 cache controller support
 *
 * Copyright (C) 2007 ARM Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/hardware/cache-l2x0.h>

#define CACHE_LINE_SIZE		32

static void __iomem *l2x0_base;
static DEFINE_RAW_SPINLOCK(l2x0_lock);

static uint32_t l2x0_way_mask;	/* Bitmask of active ways */
static uint32_t l2x0_size;
static u32 l2x0_cache_id;
static unsigned int l2x0_sets;
static unsigned int l2x0_ways;

static inline bool is_pl310_rev(int rev)
{
	return (l2x0_cache_id &
		(L2X0_CACHE_ID_PART_MASK | L2X0_CACHE_ID_REV_MASK)) ==
			(L2X0_CACHE_ID_PART_L310 | rev);
}

static inline void cache_wait_way(void __iomem *reg, unsigned long mask)
{
	/* wait for cache operation by line or way to complete */
	while (readl_relaxed(reg) & mask)
		;
}

#ifdef CONFIG_CACHE_PL310
static inline void cache_wait(void __iomem *reg, unsigned long mask)
{
	/* cache operations by line are atomic on PL310 */
}
#else
#define cache_wait	cache_wait_way
#endif

static inline void cache_sync(void)
{
	void __iomem *base = l2x0_base;

#ifdef CONFIG_ARM_ERRATA_753970
	/* write to an unmmapped register */
	writel_relaxed(0, base + L2X0_DUMMY_REG);
#else
	writel_relaxed(0, base + L2X0_CACHE_SYNC);
#endif
	cache_wait(base + L2X0_CACHE_SYNC, 1);
}

static inline void l2x0_clean_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_LINE_PA);
}

static inline void l2x0_inv_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_INV_LINE_PA);
}

#if defined(CONFIG_PL310_ERRATA_588369) || defined(CONFIG_PL310_ERRATA_727915)

#define debug_writel(val)	outer_cache.set_debug(val)

static void l2x0_set_debug(unsigned long val)
{
	writel_relaxed(val, l2x0_base + L2X0_DEBUG_CTRL);
}
#else
/* Optimised out for non-errata case */
static inline void debug_writel(unsigned long val)
{
}

#define l2x0_set_debug	NULL
#endif

#ifdef CONFIG_PL310_ERRATA_588369
static inline void l2x0_flush_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;

	/* Clean by PA followed by Invalidate by PA */
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_LINE_PA);
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_INV_LINE_PA);
}
#else

static inline void l2x0_flush_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_CLEAN_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_INV_LINE_PA);
}
#endif

void l2x0_cache_sync(void)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

#ifdef CONFIG_PL310_ERRATA_727915
static void l2x0_for_each_set_way(void __iomem *reg)
{
	int set;
	int way;
	unsigned long flags;

	for (way = 0; way < l2x0_ways; way++) {
		raw_spin_lock_irqsave(&l2x0_lock, flags);
		for (set = 0; set < l2x0_sets; set++)
			writel_relaxed((way << 28) | (set << 5), reg);
		cache_sync();
		raw_spin_unlock_irqrestore(&l2x0_lock, flags);
	}
}
#endif

static void __l2x0_flush_all(void)
{
	debug_writel(0x03);
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_CLEAN_INV_WAY);
	cache_wait_way(l2x0_base + L2X0_CLEAN_INV_WAY, l2x0_way_mask);
	cache_sync();
	debug_writel(0x00);
}

static void l2x0_flush_all(void)
{
	unsigned long flags;

#ifdef CONFIG_PL310_ERRATA_727915
	if (is_pl310_rev(REV_PL310_R2P0)) {
		l2x0_for_each_set_way(l2x0_base + L2X0_CLEAN_INV_LINE_IDX);
		return;
	}
#endif

	/* clean all ways */
	raw_spin_lock_irqsave(&l2x0_lock, flags);
	__l2x0_flush_all();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_clean_all(void)
{
	unsigned long flags;

#ifdef CONFIG_PL310_ERRATA_727915
	if (is_pl310_rev(REV_PL310_R2P0)) {
		l2x0_for_each_set_way(l2x0_base + L2X0_CLEAN_LINE_IDX);
		return;
	}
#endif

	/* clean all ways */
	raw_spin_lock_irqsave(&l2x0_lock, flags);
	debug_writel(0x03);
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_CLEAN_WAY);
	cache_wait_way(l2x0_base + L2X0_CLEAN_WAY, l2x0_way_mask);
	cache_sync();
	debug_writel(0x00);
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_inv_all(void)
{
	unsigned long flags;

	/* invalidate all ways */
	raw_spin_lock_irqsave(&l2x0_lock, flags);
	/* Invalidating when L2 is enabled is a nono */
	BUG_ON(readl(l2x0_base + L2X0_CTRL) & 1);
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_INV_WAY);
	cache_wait_way(l2x0_base + L2X0_INV_WAY, l2x0_way_mask);
	cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_inv_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2x0_base;
	unsigned long flags;

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		debug_writel(0x03);
		l2x0_flush_line(start);
		debug_writel(0x00);
		start += CACHE_LINE_SIZE;
	}

	if (end & (CACHE_LINE_SIZE - 1)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		debug_writel(0x03);
		l2x0_flush_line(end);
		debug_writel(0x00);
	}

	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_inv_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
			raw_spin_unlock_irqrestore(&l2x0_lock, flags);
			raw_spin_lock_irqsave(&l2x0_lock, flags);
		}
	}
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_clean_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2x0_base;
	unsigned long flags;

	if ((end - start) >= l2x0_size) {
		l2x0_clean_all();
		return;
	}

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_clean_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
			raw_spin_unlock_irqrestore(&l2x0_lock, flags);
			raw_spin_lock_irqsave(&l2x0_lock, flags);
		}
	}
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_flush_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2x0_base;
	unsigned long flags;

	if ((end - start) >= l2x0_size) {
		l2x0_flush_all();
		return;
	}

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		debug_writel(0x03);
		while (start < blk_end) {
			l2x0_flush_line(start);
			start += CACHE_LINE_SIZE;
		}
		debug_writel(0x00);

		if (blk_end < end) {
			raw_spin_unlock_irqrestore(&l2x0_lock, flags);
			raw_spin_lock_irqsave(&l2x0_lock, flags);
		}
	}
	cache_wait(base + L2X0_CLEAN_INV_LINE_PA, 1);
	cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_disable(void)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	__l2x0_flush_all();
	writel_relaxed(0, l2x0_base + L2X0_CTRL);
	dsb();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static unsigned char ev_name[2][20];
static unsigned long l2x0_ev_enable;
static int l2x0_proc_show(struct seq_file *m, void *v)
{
	printk("--------l2 cache function call--------\n");
	if (l2x0_ev_enable) {
		printk("ev_count0:        %8u\n", readl(l2x0_base + L2X0_EVENT_CNT0_VAL));
		printk("ev_count0_cfg:    %8s\n", ev_name[0]);
		printk("ev_count1:        %8u\n", readl(l2x0_base + L2X0_EVENT_CNT1_VAL));
		printk("ev_count1_cfg:    %8s\n", ev_name[1]);
	}

	return 0;
}
static int l2x0_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, l2x0_proc_show, NULL);
}
static unsigned char l2x0_cmd[100];
static unsigned char l2x0_attr[10][20] = {
	[0] = "disable",
	[1] = "co",
	[2] = "drhit",
	[3] = "drreq",
	[4] = "dwhit",
	[5] = "dwreq",
	[6] = "dwtreq",
	[7] = "irhit",
	[8] = "irreq",
	[8] = "wa",
	[9] = "pf",
};

static unsigned int l2x0_attr_code[10] = {
	[0] = 0x0,
	[1] = 0x1,
	[2] = 0x2,
	[3] = 0x3,
	[4] = 0x4,
	[5] = 0x5,
	[6] = 0x6,
	[7] = 0x7,
	[8] = 0x8,
	[8] = 0x9,
	[9] = 0xA,
};

/* cmd as
	drreq1 means ev 1 drreq
	irhit0 means ev 0 irhit
	pf2-r means ev 0&1 pf
 */
static ssize_t l2x0_proc_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *pos)
{
	int buf_size;
	int ev_num;
	int i;

	buf_size = count;

	copy_from_user(l2x0_cmd, buffer, buf_size);

	printk("cmd--->%s buf_size %d\n", l2x0_cmd, buf_size);
	for (i = 0; i < 10; i++) {
		if (memcmp(l2x0_attr[i], l2x0_cmd, (buf_size - 2)) == 0) {
			break;
		}
	}
	if (i >= 10) {
		printk("error cmd!\n");
		return count;
	}


	switch (l2x0_cmd[buf_size - 2]) {
	case '0':
		ev_num = 0;
		break;
	case '1':
		ev_num = 1;
		break;
	case '2':
		ev_num = 2;
		break;
	default:
		ev_num = 2;
		break;
	}

	printk("cmd--->%s l2x0_cmd[%d] %c ev_num %d\n", l2x0_cmd, buf_size - 2, l2x0_cmd[buf_size - 2], ev_num);

	if ((i == 0) && (ev_num == 2)) {
		/* disable */
		l2x0_ev_enable = 0;
	} else
		l2x0_ev_enable = 1;

	/* configure and enable ev register
	 * disable ev and configure
	 */
	writel(0, l2x0_base + L2X0_EVENT_CNT_CTRL);

	if (ev_num == 0) {
		writel(l2x0_attr_code[i] << 2, l2x0_base + L2X0_EVENT_CNT0_CFG);
		memcpy(ev_name[0], l2x0_attr[i], strlen(l2x0_attr[i]) + 1);
	} else if (ev_num == 1) {
		writel(l2x0_attr_code[i] << 2, l2x0_base + L2X0_EVENT_CNT1_CFG);
		memcpy(ev_name[1], l2x0_attr[i], strlen(l2x0_attr[i]) + 1);
	} else if (ev_num == 2) {
		writel(l2x0_attr_code[i] << 2, l2x0_base + L2X0_EVENT_CNT0_CFG);
		writel(l2x0_attr_code[i] << 2, l2x0_base + L2X0_EVENT_CNT1_CFG);
		memcpy(ev_name[0], l2x0_attr[i], strlen(l2x0_attr[i]) + 1);
		memcpy(ev_name[1], l2x0_attr[i], strlen(l2x0_attr[i]) + 1);
	}

	writel(3, l2x0_base + L2X0_EVENT_CNT_CTRL);

	return count;
}
static const struct file_operations l2x0_proc_fops = {
	.owner          = THIS_MODULE,
	.open           = l2x0_proc_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
	.write          = l2x0_proc_write,
};

static void l2x0_proc_init(void)
{
	l2x0_ev_enable = 0;
	proc_create("l2x0-proc", 0, NULL, &l2x0_proc_fops);
}

static void l2x0_lockdown_vectors(void)
{
	unsigned long vector_addr;
	unsigned long unuse;
	unsigned long flage;
	int i = 0;

	vector_addr = 0xFFFF0000;

	local_irq_save(flage);

	l2x0_flush_all();

	/* enable line lockdown */
	writel_relaxed(1, l2x0_base + 0x950);

	isb();
	dsb();

	for (i = 0; i < 1024; i++)
		unuse = readl(vector_addr + (i << 2));
	isb();
	dsb();

	/* disable unlock all lines */
	writel_relaxed(0, l2x0_base + 0x950);
	local_irq_restore(flage);
}

void __init l2x0_init(void __iomem *base, __u32 aux_val, __u32 aux_mask)
{
	__u32 aux;
	__u32 way_size = 0;
	const char *type;

	l2x0_base = base;

	l2x0_cache_id = readl_relaxed(l2x0_base + L2X0_CACHE_ID);
	aux = readl_relaxed(l2x0_base + L2X0_AUX_CTRL);

	aux &= aux_mask;
	aux |= aux_val;

	/* Determine the number of ways */
	switch (l2x0_cache_id & L2X0_CACHE_ID_PART_MASK) {
	case L2X0_CACHE_ID_PART_L310:
		if (aux & (1 << 16))
			l2x0_ways = 16;
		else
			l2x0_ways = 8;
		type = "L310";
		break;
	case L2X0_CACHE_ID_PART_L210:
		l2x0_ways = (aux >> 13) & 0xf;
		type = "L210";
		break;
	default:
		/* Assume unknown chips have 8 ways */
		l2x0_ways = 8;
		type = "L2x0 series";
		break;
	}

	l2x0_way_mask = (1 << l2x0_ways) - 1;

	/*
	 * L2 cache Size =  Way size * Number of ways
	 */
	way_size = (aux & L2X0_AUX_CTRL_WAY_SIZE_MASK) >> 17;
	way_size = SZ_1K << (way_size + 3);
	l2x0_size = l2x0_ways * way_size;
	l2x0_sets = way_size / CACHE_LINE_SIZE;

	/*
	 * Check if l2x0 controller is already enabled.
	 * If you are booting from non-secure mode
	 * accessing the below registers will fault.
	 */
	if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & 1)) {

		/* l2x0 controller is disabled */
		writel_relaxed(aux, l2x0_base + L2X0_AUX_CTRL);

		l2x0_inv_all();

		/* enable L2X0 */
		writel_relaxed(1, l2x0_base + L2X0_CTRL);
	}

	outer_cache.inv_range = l2x0_inv_range;
	outer_cache.clean_range = l2x0_clean_range;
	outer_cache.flush_range = l2x0_flush_range;
	outer_cache.sync = l2x0_cache_sync;
	outer_cache.flush_all = l2x0_flush_all;
	outer_cache.inv_all = l2x0_inv_all;
	outer_cache.disable = l2x0_disable;
	outer_cache.set_debug = l2x0_set_debug;

	mb();
	printk(KERN_INFO "%s cache controller enabled\n", type);
	printk(KERN_INFO "l2x0: %d ways, CACHE_ID 0x%08x, AUX_CTRL 0x%08x, Cache size: %d B\n",
			l2x0_ways, l2x0_cache_id, aux, l2x0_size);
	l2x0_proc_init();

	/* open double LANE Fetch, Offset to f */
	writel(0x70000008,l2x0_base+0xf60);
}

static __u32 aux_ctrl;
static __u32 latency_ctrl;
/*
 *  hisik3_pm_disable_l2x0()/hisik3_pm_enable_l2x0() is designed to
 *  disable and enable l2-cache during Suspend-Resume phase
 */
int hisik3_pm_disable_l2x0(void)
{
	/* backup aux control register value */
	aux_ctrl = readl_relaxed(l2x0_base + L2X0_AUX_CTRL);

	latency_ctrl = readl_relaxed(l2x0_base + L2X0_DATA_LATENCY_CTRL);

	/* flush cache all */
	l2x0_flush_all();

	/* disable l2x0 cache */
	writel_relaxed(0, l2x0_base + L2X0_CTRL);

	/* barrier */
	dmb();

	printk(KERN_ERR "[PM]l2x0 Cache disabled.\r\n");

	return 0;
}

int hisik3_pm_disable_l2x0_locked(void)
{
	l2x0_disable();
	return 0;
}

int hisik3_pm_enable_l2x0(void)
{
	/*enable dynamic clk gating and standby mode*/
	writel_relaxed(L2X0_DYNAMIC_CLK_GATING_EN|L2X0_STNDBY_MODE_EN,
		l2x0_base+L2X0_POWER_CTRL);

	/* disable cache */
	writel_relaxed(0, l2x0_base + L2X0_CTRL);

	/* restore aux control register */
	writel_relaxed(aux_ctrl, l2x0_base + L2X0_AUX_CTRL);
	writel_relaxed(latency_ctrl, l2x0_base + L2X0_DATA_LATENCY_CTRL);

	/* invalidate l2x0 cache */
	l2x0_inv_all();

	/* enable l2x0 cache */
	writel_relaxed(1, l2x0_base + L2X0_CTRL);

	mb();

	/* open double LANE Fetch, Offset to 7 */
	writel(0x70000007,l2x0_base+0xf60);

	printk(KERN_ERR "[PM]L2 Cache enabled\r\n");

	return 0;
}
