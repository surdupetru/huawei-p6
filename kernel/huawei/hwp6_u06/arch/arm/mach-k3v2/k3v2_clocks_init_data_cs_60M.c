/*
 *
 * arch/arm/mach-k3v2//k3v2_clocks_init_data.c
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 * Clock definitions and inline macros
 *
 */
#include "k3v2_clocks_init_data.h"
#include "clock_cs_60M.h"


struct k3v2_clk_init_table common_clk_init_table_cs_60M[] = {
	/* name		parent		rate		enabled */
	{ "clk_uart4",          "clk_cfgaxi",    CFGAXI_NORMAL_RATE,       0},
	{ "clk_uart2",          "clk_cfgaxi",    CFGAXI_NORMAL_RATE,       0},
	{ "clk_uart1",          "clk_cfgaxi",    CFGAXI_NORMAL_RATE,       0},
	/*
	 *the pmu registers will be visited after suspend by the irq of power key,
	 *the pmuspi clock must be enabled all the time.
	 */
	{ "clk_pmuspi",	NULL,		0,		1},
	//z00186406 add begin, enable 32kb clock for connectivity
	{ "clk_pmu32kb",	NULL,		0,		1},
	//z00186406 add end, enable 32kb clock for connectivity
	{ "clk_ldi1",		"clk_pll4",		198000000,	0},
	{ "clk_acp",		NULL,		0,		1},
	{ "clk_mmc3",		NULL,	0,	0},
	{ "clk_mmc2",		NULL,	0,	0},
	{ "clk_sd",		NULL,	0,	0},
	{ "clk_mmc1",		"clkin_sys",   26000000,    1},
	{ "clk_venc",		NULL,	0,	0},
	{ "clk_g2d",		NULL,	0, 	0},
	{ "clk_vpp",		NULL,	0,	0},
	{ "clk_vdec",		NULL,	0,	0},
	{ "clk_ldi0",		NULL,	0,	0},
	{ "clk_edc0",		NULL,	0,	0},
	{ "clk_edc1",		NULL,	0,	0},
	{ "clk_isp",		NULL,	0,	0},
	{ "clk_asphdmi",	NULL,	0,	0},
	{ "clk_ispmipi",		NULL,	0,	0},
	{ "clk_hsic",		NULL,	0,	0},
	{ "clk_hsi",		NULL,	0,	0},
	{ "clk_pciphy",		NULL,	0,	0},
	{ "clk_pclk_hdmi",	NULL,	0,	0},
	{ NULL,			NULL,		0,		0},
};
