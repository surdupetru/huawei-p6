/*
 *
 * arch/arm/mach-k3v2/include/mach/k3v2_clocks_init_data.h
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 * Clock definitions and inline macros
 *
 */
#ifndef __MACH_K3V2_CLOCKS_INIT_DATA_H
#define __MACH_K3V2_CLOCKS_INIT_DATA_H
#include "clock.h"
extern struct k3v2_clk_init_table common_clk_init_table_es[];
extern struct k3v2_clk_init_table common_clk_init_table_cs[];
#ifdef CONFIG_SUPPORT_B3750000_BITRATE
extern struct k3v2_clk_init_table common_clk_init_table_cs_60M[];
#endif
#endif
