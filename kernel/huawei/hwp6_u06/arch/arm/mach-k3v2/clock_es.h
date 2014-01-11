/*
 *
 * arch/arm/mach-k3v2/include/mach/clock.h
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 * Clock definitions and inline macros
 *
 */
#ifndef __MACH_CLOCK_ES_H
#define __MACH_CLOCK_ES_H

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <asm/clkdev.h>
#include "clock.h"
/*PLL2*/
#define PLL2_RATE		(1080000000)/*1080M*/
#define PLL2_64DIV_RATE		(PLL2_RATE >> 6)
#define PLL2_32DIV_RATE		(PLL2_RATE >> 5)/*1080M/32*/
#define PLL2_16DIV_RATE		(PLL2_RATE >> 4)/*1080M/16*/
#define PLL2_8DIV_RATE		(PLL2_RATE >> 3)/*1080M/8*/
#define PLL2_4DIV_RATE		(PLL2_RATE >> 2)/*1080M/4*/
#define PLL2_2DIV_RATE		(PLL2_RATE >> 1)/*1080M/2*/
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
  *[4:0] = 0x9		;9 is 10 divison,
  */
#define CFGAXI_NORMAL_DIV_VALUE ((0x7F << 16) | (0x1 << 5) | (0x8))
#define CFGAXI_NORMAL_RATE	 (PLL2_RATE / 18)
#define CFGAXI_SEL_VALUE 0x80008000 /*set pll2 as source clock for axi*/
/*
  *set 30 division from pll2 for cfgaxi
  *[6:5] = 0x1		;1 is 2 division,
  *[4:0] = 0xE		;14 is 15 divison,
  */
#define CFGAXI_LOWPOWER_DIV_VALUE ((0x7F << 16) | (0x1 << 5) | (0xE))
#define CFGAXI_LOWPOWER_RATE	 (PLL2_RATE / 30)

/*get 480M(1440/3) rate,write 0 to bit 2 and write 2 to bit 1:0*/
#define HSICSEL_VALUE (((1<<2) << 16))
#define HSICDIV_VALUE ((((1<<1) | (1<<0)) << 16) | (1<<1))
#endif
