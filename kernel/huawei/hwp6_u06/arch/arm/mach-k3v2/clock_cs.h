/*
 *
 * arch/arm/mach-k3v2/include/mach/clock.h
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 * Clock definitions and inline macros
 *
 */
#ifndef __MACH_CLOCK_CS_H
#define __MACH_CLOCK_CS_H

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <asm/clkdev.h>
/*PLL2*/
#define PLL2_RATE		(1440000000)
#define PLL2_64DIV_RATE		(PLL2_RATE >> 6)/*PLL2_RATE/64*/
#define PLL2_32DIV_RATE		(PLL2_RATE >> 5)/*PLL2_RATE/32*/
#define PLL2_16DIV_RATE		(PLL2_RATE >> 4)/*PLL2_RATE/16*/
#define PLL2_8DIV_RATE		(PLL2_RATE >> 3)/*PLL2_RATE/8*/
#define PLL2_4DIV_RATE		(PLL2_RATE >> 2)/*PLL2_RATE/4*/
#define PLL2_2DIV_RATE		(PLL2_RATE >> 1)/*PLL2_RATE/2*/

/*clock's default rate*/
#define PLL2_DIV1_RATE		(PLL2_RATE / 1)
#define PLL2_DIV2_RATE		(PLL2_RATE / 2)
#define PLL2_DIV3_RATE		(PLL2_RATE / 3)
#define PLL2_DIV4_RATE		(PLL2_RATE / 4)
#define PLL2_DIV5_RATE		(PLL2_RATE / 5)
#define PLL2_DIV6_RATE		(PLL2_RATE / 6)
#define PLL2_DIV7_RATE		(PLL2_RATE / 7)
#define PLL2_DIV8_RATE		(PLL2_RATE / 8)
#define PLL2_DIV9_RATE		(PLL2_RATE / 9)
#define PLL2_DIV10_RATE		(PLL2_RATE / 10)
#define PLL2_DIV11_RATE		(PLL2_RATE / 11)
#define PLL2_DIV12_RATE		(PLL2_RATE / 12)

/*
  *set 20 division from pll2 for cfgaxi
  *[6:5] = 0x1		;1 is 2 division,
  *there are constraint about axi clock's rate,
  *more than 48M is needed by uart.
  *[4:0] = 0x0E	;0x0E is 15 divison,
  */
#define CFGAXI_NORMAL_DIV_VALUE ((0x7F << 16) | (0x1 << 5) | (0x0E))
#define CFGAXI_NORMAL_RATE	 (PLL2_RATE / 30)
#define CFGAXI_SEL_VALUE 0x80008000 /*set pll2 as source clock for axi*/

/*
  *set 30 division from pll2 for cfgaxi
  *[6:5] = 0x1		;1 is 2 division,
  *[4:0] = 0xE		;14 is 15 divison,
  */
#define CFGAXI_LOWPOWER_DIV_VALUE ((0x7F << 16) | (0x1 << 5) | (0xE))
#define CFGAXI_LOWPOWER_RATE	 (PLL2_RATE / 30)

/*get 480M(1440/3) rate,write 1 to bit 2 and write 2 to bit 1:0*/
#define HSICSEL_VALUE (((1<<2) << 16) | (1 << 2))
#define HSICDIV_VALUE ((((1<<1) | (1<<0)) << 16) | (1<<1))

#define CLK_SPECIAL_OPS(_clk, _clkname, _parent, _refcnt, _rate, _min_rate, _max_rate, \
	_ops, _sel_parents, _enable_reg, _enable_bit, _clksel_reg,	\
	_clksel_mask, _clkdiv_reg, _clkdiv_mask, _reset_reg, _reset_bit)	\
static struct clk _clk = {\
	.name		= _clkname,\
	.parent		= &_parent,\
	.refcnt		= _refcnt,\
	.rate		= _rate,\
	.min_rate	= _min_rate, \
	.max_rate	= _max_rate,\
	.ops		= &_ops,\
	.sel_parents	= _sel_parents,\
	.enable_reg	= (void __iomem	*)_enable_reg,\
	.enable_bit	= _enable_bit,\
	.clksel_reg	= (void __iomem	*)_clksel_reg,\
	.clksel_mask	= _clksel_mask,\
	.clkdiv_reg	= (void __iomem	*)_clkdiv_reg,\
	.clkdiv_mask	= _clkdiv_mask,\
	.reset_reg	= (void __iomem	*)_reset_reg,\
	.reset_bit	= _reset_bit,\
	.cansleep	= false,\
};

#define CLK_WITH_FRIEND(_clk, _clkname, _parent, _refcnt, _rate, _min_rate, _max_rate, \
	_firend, _ops, _sel_parents, _enable_reg, _enable_bit, _clksel_reg,	\
	_clksel_mask, _clkdiv_reg, _clkdiv_mask, _reset_reg, _reset_bit)	\
static struct clk _clk = {\
	.name		= _clkname,\
	.parent		= &_parent,\
	.refcnt		= _refcnt,\
	.rate		= _rate,\
	.min_rate	= _min_rate, \
	.max_rate	= _max_rate,\
	.friend	= &_firend,\
	.ops		= &_ops,\
	.sel_parents	= _sel_parents,\
	.enable_reg	= (void __iomem	*)_enable_reg,\
	.enable_bit	= _enable_bit,\
	.clksel_reg	= (void __iomem	*)_clksel_reg,\
	.clksel_mask	= _clksel_mask,\
	.clkdiv_reg	= (void __iomem	*)_clkdiv_reg,\
	.clkdiv_mask	= _clkdiv_mask,\
	.reset_reg	= (void __iomem	*)_reset_reg,\
	.reset_bit	= _reset_bit,\
	.cansleep	= false,\
};
#endif
