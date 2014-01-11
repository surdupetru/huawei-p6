/*
 * arch/arm/mach-k3v2/k3v2_clocks.c
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <mach/platform.h>
#include <mach/boardid.h>
#include <linux/clkdev.h>
#include <asm/clkdev.h>
#include "clock.h"
#include <mach/early-debug.h>
#include <hsad/config_interface.h>
static LIST_HEAD(clocks);

/* this function is used to check whether the parent clock
is this clock's source, and get the clksel */
static struct clksel *k3v2_getclksel_by_parent(struct clk *clk,
	struct clk *parent_clk)
{
	struct clksel *clks = clk->sel_parents;

	if (clks) {
		while (clks->sel_parent) {
			if (clks->sel_parent == parent_clk)
				break;
			clks++;
		}

		if (!clks->sel_parent) {
			pr_debug(" Could not find parent clock for %s.\r\n", clk->name);
			return NULL;
		}
	}

	return clks;
}

#ifdef CONFIG_DEBUG_FS
static char g_clk_status[5][10] = {"NOREG", "OK", "ERR", "uninit", "NODIV"};
char *k3v2_enabled_check(struct clk *c)
{
	int regvalue = 0;

	if (!c->enable_reg)
		return g_clk_status[0];

	if (c->refcnt > 0) {
		regvalue = readl(c->enable_reg + IS_ENABLE_REG_OFFSET)
			& (1 << c->enable_bit);
		regvalue = regvalue >> c->enable_bit;
		if (regvalue == 0x01)
			return g_clk_status[1];
		else if (regvalue == 0x00)
			return g_clk_status[2];

	} else if (c->refcnt == 0) {
		if ((c->state != ON) && (c->state != OFF)) {
			return g_clk_status[3];
		} else {
			regvalue = readl(c->enable_reg + IS_ENABLE_REG_OFFSET)
				& (1 << c->enable_bit);
			regvalue = regvalue >> c->enable_bit;
			if (regvalue == 0x01)
				return g_clk_status[2];
			else
				return g_clk_status[1];
		}
	}

	return g_clk_status[2];
}

char *k3v2_source_check(struct clk *c)
{
	int regvalue = 0;
	char *result = g_clk_status[2];
	struct clksel *clks = c->sel_parents;

	if (!c->clksel_reg)
		return g_clk_status[0];

	if ((c->state != ON) && (c->state != OFF))
		return g_clk_status[3];

	regvalue = readl(c->clksel_reg) & (c->clksel_mask >> 16);
	regvalue = regvalue >> (__ffs(c->clksel_mask) - 16);

	if (clks) {
		while (clks->sel_parent) {
			if (clks->sel_parent == c->parent)
				break;
			clks++;
		}

		if (regvalue == clks->sel_val)
			result = g_clk_status[1];
		else
			result = g_clk_status[2];
	}

	return result;
}

char *k3v2_rate_check(struct clk *c)
{
	int regvalue = 0;
	int div_value;
	char *result = NULL;
	struct clksel *clks = c->sel_parents;

	if (!c->clkdiv_reg)
		return g_clk_status[0];

	if ((c->state != ON) && (c->state != OFF))
		return g_clk_status[3];

	if (clks) {
		while (clks->sel_parent) {
			if (clks->sel_parent == c->parent)
				break;
			clks++;
		}
	}

	if (clks && (clks->parent_min == clks->parent_max))
		return g_clk_status[4];

	regvalue = readl(c->clkdiv_reg) & (c->clkdiv_mask >> 16);
	regvalue = regvalue >> (__ffs(c->clkdiv_mask) - 16);

	div_value = c->parent->rate / c->rate - 1;/*h0:1,h1:2,......*/

	if (regvalue == div_value)
		result = g_clk_status[1];
	else
		result = g_clk_status[2];

	return result;
}
#endif
void clk_enable_set(struct clk *clk)
{
	int i = 1000;
	int val = 0;
	int ret = 0;

	if (clk->enable_reg == NULL) {
		pr_debug("CLOCK:%s's enable register is NULL.\n", clk->name);
		return;
	}

	if (clk->friend) {
		ret = clk_enable(clk->friend);
		if (ret) {
			WARN(1, "CLOCK:Disable friend failed.\r\n");
		}
	}

	writel((u32)(1 << clk->enable_bit), clk->enable_reg);
	val = readl(clk->enable_reg + IS_ENABLE_REG_OFFSET) & (1 << clk->enable_bit);
	/*check status*/
	while (!val && i) {
		writel((u32)(1 << clk->enable_bit), clk->enable_reg);
		val = readl(clk->enable_reg + IS_ENABLE_REG_OFFSET)
			& (1 << clk->enable_bit);
		i--;
	}

	if (i == 0)
		WARN(1, "CLOCK:Attempting to write clock enable register 1000 times.\r\n");

	return;
}

void clk_disable_set(struct clk *clk)
{
	int i = 1000;
	int val = 0;

	if (clk->enable_reg == NULL) {
		pr_debug("CLOCK:%s's disable register is NULL.\n", clk->name);
		return;
	}

#ifndef CONFIG_K3_CLK_ALWAYS_ON
	writel((u32)(1 << clk->enable_bit), clk->enable_reg + DISABLE_REG_OFFSET);
	val = readl(clk->enable_reg + IS_ENABLE_REG_OFFSET) & (1 << clk->enable_bit);
	/*check status*/
	while (val && i) {
		writel((u32)(1 << clk->enable_bit), clk->enable_reg + DISABLE_REG_OFFSET);
		val = readl(clk->enable_reg + IS_ENABLE_REG_OFFSET) & (1 << clk->enable_bit);
		i--;
	}
#endif

	if (i == 0)
		WARN(1, "CLOCK:Attempting to write clock disable register 1000 times.\r\n");

	if (clk->friend) {
		clk_disable(clk->friend);
	}

	return;
}

void rst_enable_set(struct clk *clk)
{
	int i = 1000;
	int val = 0;

	if (clk->reset_reg == NULL) {
		pr_debug("CLOCK:%s's rst register is NULL.\n", clk->name);
		return;
	}

	writel((u32)(1 << clk->reset_bit), clk->reset_reg);
	val = readl(clk->reset_reg + RST_STATUS_REG_OFFSET) & (1 << clk->reset_bit);
	/*check status*/
	while (!val && i) {
		writel((u32)(1 << clk->reset_bit), clk->reset_reg);
		val = readl(clk->reset_reg + RST_STATUS_REG_OFFSET) & (1 << clk->reset_bit);
		i--;
	}

	if (i == 0)
		WARN(1, "CLOCK:Attempting to write rst enable register 1000 times.\r\n");

	return;
}

void rst_disable_set(struct clk *clk)
{
	int i = 1000;
	int val = 0;

	if (clk->reset_reg == NULL) {
		pr_debug("CLOCK:%s's rst register is NULL.\n", clk->name);
		return;
	}

	writel((u32)(1 << clk->reset_bit), clk->reset_reg + DISABLE_RESET_OFFSET);
	val = readl(clk->reset_reg + RST_STATUS_REG_OFFSET) & (1 << clk->enable_bit);
	/*check status*/
	while (val && i) {
		writel((u32)(1 << clk->reset_bit), clk->reset_reg + DISABLE_RESET_OFFSET);
		val = readl(clk->reset_reg + RST_STATUS_REG_OFFSET) & (1 << clk->enable_bit);
		i--;
	}

	if (i == 0)
		WARN(1, "CLOCK:Attempting to write rst disable register 1000 times.\r\n");

	return;
}

int k3v2_clk_enable(struct clk *clk)
{
	BUG_ON(clk == NULL);
	clk_enable_set(clk);
	return 0;
}

void k3v2_clk_disable(struct clk *clk)
{
	BUG_ON(clk == NULL);
	clk_disable_set(clk);
	return;
}
static int k3v2_clk_write_sel_register(struct clk *clk, struct clk *parent)
{
	u32 v;
	struct clksel *clks = NULL;

	BUG_ON(clk == NULL || parent == NULL);

	/* if there is no parent, parent setting is a invalid operation ,
	    return -1 as error number */
	if (clk->parent == NULL) {
		pr_debug("CLOCK:%s has no parent, it's a source clock.\n", clk->name);
		return -1;
	}

	/* if there is only one parent, the clksel_reg must be NULL */
	if (clk->clksel_reg == NULL) {
		if (clk->parent == parent) {
			return 0;
		} else {
			pr_debug("CLOCK:there is only one parent,"
				"can't set the parent as %s.\n", parent->name);
			/* the parent is not this clock's source,
			this is a invalid operation */
			return -1;
		}
	}

	/* find clksel from the clock source */
	clks = k3v2_getclksel_by_parent(clk, parent);
	if (!clks) {
		pr_debug("CLOCK:%s is not %s's parent.\r\n", parent->name, clk->name);
		return -1;/*this clock is not the parent*/
	}

	/* set the register's bit to get the clock source */
	v = clk->clksel_mask
		| (clks->sel_val << (__ffs(clk->clksel_mask) - 16));
	writel(v, clk->clksel_reg);

	return 0;
}
int k3v2_clk_set_parent(struct clk *clk, struct clk *parent)
{
	k3v2_clk_write_sel_register(clk, parent);
	clk->parent = parent;
	return 0;
}

int k3v2_clk_switch_pll(struct clk *clk, struct clk *parent)
{
	k3v2_clk_write_sel_register(clk, parent);
	return 0;
}

/* this function is used to find the right parent which can provide the rate */
static struct clk *k3v2_clk_find_parent_from_selparents(struct clk *clk, unsigned rate)
{
	struct clksel *clks = NULL;
	struct clk *clkparent = NULL;

	BUG_ON(clk == NULL);
	BUG_ON(clk->sel_parents == NULL);

	for (clks = clk->sel_parents; clks->sel_parent; clks++) {
		/* this rate can be brought by this parent */
		if (rate <= clks->parent_max && rate >= clks->parent_min) {
			clkparent = clks->sel_parent;
			break; /* Found the requested parent */
		}
	}

	return clkparent;
}

/* get the right rate */
long k3v2_clk_round_rate(struct clk *clk, long rate)
{
	int ret = 0;
	unsigned int div = 0;
	unsigned int modvalue = 0;
	struct clk *clkparent = NULL;
	struct clksel *clks = NULL;

	BUG_ON(clk == NULL);
	if (rate < clk->min_rate || rate > clk->max_rate) {
		pr_debug("CLOCK:%ld is invalid for %s.\n", rate, clk->name);
		return -1;
	}

	if (!clk->parent) {
		pr_debug("CLOCK:%s has no parent, don't need to set rate.\n", clk->name);
		return -1;
	}

	if (!clk->sel_parents)
		goto round_rate;/*there's only one parent, and the rate is not invalid*/

	/*check whether the current parent provide this rate, if can't , find another one*/
	clks = k3v2_getclksel_by_parent(clk, clk->parent);
	if (clks && rate >= clks->parent_min && rate <= clks->parent_max)
		goto round_rate;

	/* find parent */
	clkparent = k3v2_clk_find_parent_from_selparents(clk, rate);
	/* no parent can offer this rate */
	if (clkparent == NULL) {
		pr_debug("CLOCK:no parent can offer rate:%ld to %s.\r\n",
			rate, clk->name);
		return -1;
	}

	if (clk->refcnt == 0)/*check whether this clock has been enabled*/
		ret = -1;
	/* change the parent , if the clk->sel_parents == NULL,
	clk->parent must be the same as clkparent */
	if (ret >= 0) {
		if (clk->parent != clkparent) {
			clk_disable(clk->parent); /* disable current parents */
			ret = clk_enable(clkparent);
			if (ret >= 0) {
				if (!clk->ops || !clk->ops->set_parent) {
					pr_debug("CLOCK:%s has no parent \
						ops.\n", clk->name);
					return -1;
				}
				clk->ops->set_parent(clk, clkparent);
			}
		}
	} else { /* if clock hasnt enabled,don't need to count */
		if (!clk->ops || !clk->ops->set_parent) {
			pr_debug("CLOCK:%s has no parent ops.\n", clk->name);
			return -1;
		}
		clk->ops->set_parent(clk, clkparent);
	}

round_rate:
	div = clk->parent->rate / rate;
	modvalue = clk->parent->rate % rate;
	if (modvalue != 0)
		rate = clk->parent->rate / (div + 1);
	else
		rate = clk->parent->rate / div;

	return rate;
}

/* set the div for the rate ,the parent must be set firstly */
int k3v2_clk_set_rate(struct clk *clk, unsigned rate)
{
	unsigned int div = 0;
	u32 v = 0;
	struct clksel *clks = NULL;

	clks = k3v2_getclksel_by_parent(clk, clk->parent);
	if (clks) {
		if (clks->parent_min == clks->parent_max) {
			pr_debug("CLOCK:the source clock can't be divided  "
			"the rate is constant.\n");
			clk->rate = clk->parent->rate;
			return 0;
		}
	}

	if (clk->clkdiv_reg != NULL) {
		div = clk->parent->rate / rate; /*calculate the div */
		/* caculate the value of register, the function __ffs()
		 *is used to find the first non zero bit, and it is defined
		 *at include/asm-generic/bitops/__ffs.h
		 */
		v = clk->clkdiv_mask | ((div - 1) << (__ffs(clk->clkdiv_mask) - 16));
		writel(v, clk->clkdiv_reg);
		clk->rate = rate;
		pr_debug("CLOCK:%s's rate has been set as %ld.", clk->name, clk->rate);
		return 0;
	}
	pr_debug("CLOCK:%s rate set failed.\r\n", clk->name);
	return -1;
}

unsigned int k3v2_clk_get_rate(struct clk *clk)
{
	return clk->rate;
}

struct clk_ops clock_ops = {
	.enable = k3v2_clk_enable,
	.disable = k3v2_clk_disable,
	.round_rate = k3v2_clk_round_rate,
	.set_rate = k3v2_clk_set_rate,
	.set_parent = k3v2_clk_set_parent,
#ifdef CONFIG_DEBUG_FS
	.check_enreg = k3v2_enabled_check,
	.check_selreg = k3v2_source_check,
	.check_divreg = k3v2_rate_check,
#endif
};

int k3v2_pll_clk_enable(struct clk *clk)
{
	u32 v = 0;
	u32 val = 0;
	u32 i = 1000;

	v = readl(clk->enable_reg);
	v |= 1 << clk->enable_bit;
	writel(v, clk->enable_reg);
	val = readl(clk->enable_reg) & PLL_LOCK_BIT;
	/*check status*/
	while (!val && i) {
		writel(v, clk->enable_reg);
		val = readl(clk->enable_reg) & PLL_LOCK_BIT;
		i--;
	}

	return 0;
}

void k3v2_pll_clk_disable(struct clk *clk)
{
	u32 v = 0;

#ifndef CONFIG_K3_CLK_ALWAYS_ON
	v = readl(clk->enable_reg);
	v &=  ~(1 << clk->enable_bit);
	writel(v, clk->enable_reg);
#endif
}

struct clk_ops clock_ops_src = {
	.enable = k3v2_pll_clk_enable,
	.disable = k3v2_pll_clk_disable,
};

void __init k3v2_init_clocks(void)
{
	int i = 0;
	int chip_id = 0;
	struct clk_lookup *cl_lookups = &k3v2_clk_lookups_cs[0];
	struct clk_lookup *cl = NULL;

	chip_id = get_chipid();
	if (chip_id == CS_CHIP_ID) {
#ifdef CONFIG_SUPPORT_B3750000_BITRATE
	    if(get_if_use_3p75M_uart_clock())
		{
		 printk("k3v2_init_clocks. use_3p75M_uart_clock\n");
		 cl_lookups = &k3v2_clk_lookups_cs_60M[0];
		}
		else
#endif
		  cl_lookups = &k3v2_clk_lookups_cs[0];
	} else if (chip_id == DI_CHIP_ID) {
		cl_lookups = &k3v2_clk_lookups_es[0];
	} else {
		pr_debug("%s %d no proper chip id find.\n", __func__, __LINE__);
	}

	while((cl_lookups[i].clk != NULL) ) {
		cl = &cl_lookups[i];
		clk_init(cl->clk);
		clkdev_add(cl);
		i++;
	}
}
