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
#include "clock_es.h"

static struct clk pll2_parent_clk;
static struct clk pll3_parent_clk;

static int k3v2_clk_set_rate_hdmim(struct clk *clk, unsigned rate)
{
	unsigned long long div = 0;
	unsigned long long div_temp = 0;/*the temp variant used to calculate the div*/
	unsigned long long div_mod = 0;
	unsigned long long divhigh = 0;/*div[16:23]*/
	unsigned long long divlow = 0;/*div[0:15]*/
	unsigned long long rate_ext = rate;/*64 bits to avoid overflow*/
	void __iomem *clk_addr1 = NULL, *clk_addr2 = NULL;
	u32 v1, v2;

	/*
	 *rate = div[23:0]/2^27*480MHz;
	 *div[0:15] was written at DIV_REG8;
	 *div[16:23] was written at DIV_REG9;
	 *div[23:0] = rate * 2^27 /480MHZ
	 */

	div_temp = rate_ext << 27;
	div_mod = do_div(div_temp, 480000000);
	if (div_mod != 0) {
		div = div_temp + 1;/*use */
	} else {
		div = div_temp;
	}

	/* the low 16 bits of div value */
	divlow	= div & LOW16BIT_MASK;
	clk_addr1 = (void __iomem	*)DIV_REG8;
	/* caculate the value of register */
	v1 = HDMI_MASK1 | (divlow << (__ffs(HDMI_MASK1) - 16));
	divhigh = div >> 16;/* the high 8bits value of div */
	clk_addr2 = (void __iomem	*)DIV_REG9;
	/* caculate the value of register */
	v2 = HDMI_MASK2 | (divhigh << (__ffs(HDMI_MASK2) - 16));

	writel(v1, clk_addr1);
	writel(v2, clk_addr2);

	clk->rate = rate;
	pr_info("CLOCK:%s's rate has been set as %ld.", clk->name, clk->rate);
	return 0;
}

/* define a special ops for hdmi_m,
because the hdmi_m's div is seprated into 0x120(15:0)&0x124(7:0) */
static struct clk_ops clock_ops_hdmim = {
	.enable = k3v2_clk_enable,
	.disable = k3v2_clk_disable,
	.round_rate = NULL,
	.set_rate = k3v2_clk_set_rate_hdmim,
	.set_parent = k3v2_clk_set_parent,
#ifdef CONFIG_DEBUG_FS
	.check_enreg = k3v2_enabled_check,
	.check_selreg = k3v2_source_check,
	.check_divreg = k3v2_rate_check,
#endif
};

/* clock sources as parents */
SRC_CLK(clkin_sys, "clkin_sys", 0, CLKIN_SYS_RATE, UNINITIALIZED, NULL, 0, NULL)
SRC_CLK(clkin_rf, "clkin_rf", 0, CLKIN_RF_RATE, UNINITIALIZED, NULL, 0, NULL)/*external clock of bt*/
SRC_CLK(clkin_rf32k, "clkin_rf32k", 0, CLKIN_RF32K_RATE, UNINITIALIZED, NULL, 0, NULL)
SRC_CLK(clk_pll0_src1600M, "clk_pll0", 1, PLL0_RATE, ON, PLLEN_REG0, 0,
	&clock_ops_src)
SRC_CLK(clk_pll1_src1600M, "clk_pll1", 1, PLL1_RATE, ON, PLLEN_REG1, 0,
	&clock_ops_src)
SRC_CLK(pll2_parent_clk, "clk_pll2", 3, PLL2_RATE, ON, PLLEN_REG2, 0, &clock_ops_src)
SRC_CLK(pll3_parent_clk, "clk_pll3", 0, PLL3_RATE, UNINITIALIZED, PLLEN_REG3, 0, &clock_ops_src)
SRC_CLK(pll4_parent_clk, "clk_pll4", 0, PLL4_RATE, UNINITIALIZED, PLLEN_REG4, 0, &clock_ops_src)
SRC_CLK(pll5_parent_clk, "clk_pll5", 0, PLL5_RATE, UNINITIALIZED, PLLEN_REG5, 0, &clock_ops_src)
SRC_CLK(edc1_src, "edc1_src", 0, EDC1_SRC_RATE, UNINITIALIZED, NULL, 0, &clock_ops)

static int k3v2_480M_clk_enable(struct clk *clk)
{
	void __iomem *clk_addr1 = NULL;
	int value = HSICSEL_VALUE;

	clk_addr1 =  (void __iomem *)DIV_REG12;
	writel(value, clk_addr1);
	value = HSICDIV_VALUE;
	writel(value, clk_addr1);/*peril PLL 1440M, div 3*/

	return 0;
}

static void k3v2_480M_clk_disble(struct clk *clk)
{
	return;
}

static struct clk_ops clock_ops_480M = {
	.enable = k3v2_480M_clk_enable,
	.disable = k3v2_480M_clk_disble,
};
CLK_FROM_PLL3(clk_480M, "clk_480M", 0, CLK_480M_RATE, CLK_480M_RATE, \
		CLK_480M_RATE, clock_ops_480M, NULL, 0, NULL, 0) \

static int k3v2_cfgaxi_clk_enable(struct clk *clk)
{
	int chip_id = 0;
	void __iomem *clk_addr1 = NULL;

	clk_addr1 =  (void __iomem *)DIV_REG0;
	/*set pll2 as source clock for axi*/
	chip_id = get_chipid();
	if (chip_id == CS_CHIP_ID)
		writel(CFGAXI_SEL_VALUE, clk_addr1);
	/*set rate*/
	writel(CFGAXI_NORMAL_DIV_VALUE, clk_addr1);
	clk->rate = CFGAXI_NORMAL_RATE;
	return 0;
}

static void k3v2_cfgaxi_clk_disble(struct clk *clk)
{
#if 0
	void __iomem *clk_addr1 = NULL;
	int value = CFGAXI_LOWPOWER_DIV_VALUE;

	clk_addr1 =  (void __iomem *)DIV_REG0;
	writel(value, clk_addr1);
	clk->rate = CFGAXI_LOWPOWER_RATE;
#endif
	return;
}

static struct clk_ops clock_ops_cfgaxi = {
	.enable = k3v2_cfgaxi_clk_enable,
	.disable = k3v2_cfgaxi_clk_disble,
};

static struct clk clk_cfgaxi = {
	.name		= "clk_cfgaxi",
	.parent		= &pll2_parent_clk,
	.refcnt		= 0,
	.rate		= CFGAXI_NORMAL_RATE,
	.min_rate	= CFGAXI_LOWPOWER_RATE,
	.max_rate	= CFGAXI_NORMAL_RATE,
	.ops		= &clock_ops_cfgaxi,
	.enable_reg	= (void __iomem	*)NULL,
	.enable_bit	= 0,
	.reset_reg	= (void __iomem	*)NULL,
	.reset_bit	= 0,
	.cansleep	= false,
};
/*
 *parent_min and parent_max are decided by source clocks' rate and div value,
 *they are different ,although the parents are same,so they are
 *different with clks' max_rate and min_rate
 */

/* clock's parents */
static struct clksel CLKSRC_CFGAXI26M[] = {
	{.sel_parent = &clkin_sys, .sel_val = 0,
		.parent_min = CLKIN_SYS_RATE, .parent_max = CLKIN_SYS_RATE},
	{.sel_parent = &clk_cfgaxi, .sel_val = 1,
		.parent_min = CFGAXI_NORMAL_RATE, .parent_max = CFGAXI_NORMAL_RATE},
	{.sel_parent = NULL },
};

static struct clksel PLL23_4DIV[] = {
	{.sel_parent = &pll2_parent_clk, .sel_val = 0,
		.parent_min = PLL2_4DIV_RATE, .parent_max = PLL2_RATE},
	{.sel_parent = &pll3_parent_clk, .sel_val = 1,
		.parent_min = PLL3_4DIV_RATE, .parent_max = PLL3_RATE},
	{.sel_parent = NULL },
};

static struct clksel PLL23_16DIV[] = {
	{.sel_parent = &pll2_parent_clk, .sel_val = 0,
		.parent_min = PLL2_16DIV_RATE, .parent_max = PLL2_RATE},
	{.sel_parent = &pll3_parent_clk, .sel_val = 1,
		.parent_min = PLL3_16DIV_RATE, .parent_max = PLL3_RATE},
	{.sel_parent = NULL },
};

static struct clksel PLL23_32DIV[] = {
	{.sel_parent = &pll2_parent_clk, .sel_val = 0,
		.parent_min = PLL2_32DIV_RATE, .parent_max = PLL2_RATE},
	{.sel_parent = &pll3_parent_clk, .sel_val = 1,
		.parent_min = PLL3_32DIV_RATE, .parent_max = PLL3_RATE},
	{.sel_parent = NULL },
};

static struct clksel PLL23_DIV64[] = {
	{.sel_parent = &pll2_parent_clk, .sel_val = 0,
		.parent_min = PLL2_64DIV_RATE, .parent_max = PLL2_RATE},
	{.sel_parent = &pll3_parent_clk, .sel_val = 1,
		.parent_min = PLL3_64DIV_RATE, .parent_max = PLL3_RATE},
	{.sel_parent = NULL },
};

static struct clksel PLL2345_DIV64[] = {
	{.sel_parent = &pll2_parent_clk, .sel_val = 0,
		.parent_min = PLL2_64DIV_RATE, .parent_max = PLL2_RATE},
	{.sel_parent = &pll4_parent_clk, .sel_val = 1,
		.parent_min = PLL4_64DIV_RATE, .parent_max = PLL4_RATE},
	{.sel_parent = &pll3_parent_clk, .sel_val = 2,
		.parent_min = PLL3_64DIV_RATE, .parent_max = PLL3_RATE},
	{.sel_parent = &pll5_parent_clk, .sel_val = 3,
		.parent_min = PLL5_64DIV_RATE, .parent_max = PLL5_RATE},
	{.sel_parent = NULL },
};

static struct clksel PLL23_EDC1[] = {
	{.sel_parent = &pll2_parent_clk, .sel_val = 0,
		.parent_min = PLL2_64DIV_RATE, .parent_max = PLL2_RATE},
	{.sel_parent = &pll3_parent_clk, .sel_val = 1,
		.parent_min = PLL3_64DIV_RATE, .parent_max = PLL3_RATE},
	{.sel_parent = &edc1_src, .sel_val = 2,/*synchronize with edc0*/
		.parent_min = PLL2_64DIV_RATE, .parent_max = PLL3_RATE},
	{.sel_parent = NULL },
};

static struct clksel CLKSEL_480M26M[] = {
	{.sel_parent = &clk_480M, .sel_val = 0,
		.parent_min = CLK_480MDIV16_RATE, .parent_max = CLK_480M_RATE},
	{.sel_parent = &clkin_rf, .sel_val = 1,
		.parent_min = CLKIN_RF_RATE, .parent_max = CLKIN_RF_RATE},
	{.sel_parent = NULL },
};

static struct clksel CLKSEL_32K26MDIV8[] = {
	{.sel_parent = &clkin_rf32k, .sel_val = 0,
		.parent_min = CLKIN_RF32K_RATE, .parent_max = CLKIN_RF32K_RATE},
	{.sel_parent = &clkin_sys, .sel_val = 1,
		.parent_min = CLKIN_SYS_8DIV_RATE, .parent_max = CLKIN_SYS_RATE},
	{.sel_parent = NULL },
};

static struct clksel CLKSEL_32K26MDIV32[] = {
	{.sel_parent = &clkin_rf32k, .sel_val = 0,
		.parent_min = CLKIN_RF32K_RATE, .parent_max = CLKIN_RF32K_RATE},
	{.sel_parent = &clkin_sys, .sel_val = 1,
		/*the division is shared by pwm0 and pwm1*/
		.parent_min = CLKIN_SYS_32DIV_RATE, .parent_max = CLKIN_SYS_RATE},
	{.sel_parent = NULL },
};

static struct clksel PLL23_2DIV[] = {
	{.sel_parent = &pll2_parent_clk, .sel_val = 0,
		.parent_min = PLL2_2DIV_RATE, .parent_max = PLL2_RATE},
	{.sel_parent = &pll3_parent_clk, .sel_val = 1,
		.parent_min = PLL3_2DIV_RATE, .parent_max = PLL3_RATE},
	{.sel_parent = NULL },
};

static struct clksel PLL23_SYS_16DIV[] = {
	{.sel_parent = &clkin_sys, .sel_val = 0,
		.parent_min = CLKIN_SYS_RATE, .parent_max = CLKIN_SYS_RATE},
	{.sel_parent = &pll2_parent_clk, .sel_val = 2,
		.parent_min = PLL2_16DIV_RATE, .parent_max = PLL2_RATE},
	{.sel_parent = &pll3_parent_clk, .sel_val = 3,
		.parent_min = PLL3_16DIV_RATE, .parent_max = PLL3_RATE},
	{.sel_parent = NULL },
};

static struct clk clk_hdmi_m = {
	.name		= "clk_hdmi_m",
	.parent		= &clk_480M,
	.refcnt		= 0,
	.rate		= 25000000,
	.min_rate	= 3,
	.max_rate	= 60000000,
	.sel_parents	= NULL,
	.enable_reg	= (void __iomem	*)EN_REG3,
	.enable_bit	= 27,
	.clksel_reg	= NULL,
	.clkdiv_reg	= (void __iomem	*)DIV_REG8,
	.clkdiv_mask	= 0xFFFF0000,
	.reset_reg	= NULL,
	.reset_bit	= 0,
	.ops		= &clock_ops_hdmim,
};

/*sd card is special*/
static int k3v2_clksd_enable(struct clk *clk)
{
	void __iomem *clk_addr1 = NULL;
	int value = SDSEL_VALUE;

	BUG_ON(clk == NULL);
	clk_addr1 =  (void __iomem *)DIV_REG2;
	writel(value, clk_addr1);
	value = SDDIV_VALUE;
	writel(value, clk_addr1);
	clk_enable_set(clk);
	return 0;
}

static void k3v2_clksd_disable(struct clk *clk)
{
	clk_disable_set(clk);
	return;
}

static struct clk_ops clock_ops_sd = {
	.enable = k3v2_clksd_enable,
	.disable = k3v2_clksd_disable,
#ifdef CONFIG_DEBUG_FS
	.check_enreg = k3v2_enabled_check,
	.check_selreg = k3v2_source_check,
	.check_divreg = k3v2_rate_check,
#endif
};

static struct clk clk_sd = {
	.name		= "clk_sd",
	.parent		= &pll2_parent_clk,
	.refcnt		= 0,
	.rate		= CLK_SD_RATE,
	.min_rate	= CLK_SD_RATE,
	.max_rate	= CLK_SD_RATE,
	.ops		= &clock_ops_sd,
	.sel_parents	= NULL,
	.enable_reg	= (void __iomem	*)EN_REG3,
	.enable_bit	= 20,
	.clksel_reg	= NULL,
	.clksel_mask	= 0,
	.clkdiv_reg	= NULL,
	.clkdiv_mask	= 0,
};

static int k3v2_vpp_clk_enable(struct clk *clk)
{
	BUG_ON(clk == NULL);

	/*enable clock*/
	clk_enable_set(clk);
	/*disable rst*/
	rst_disable_set(clk);
	return 0;
}

static void k3v2_vpp_clk_disable(struct clk *clk)
{
	BUG_ON(clk == NULL);

	/*enable rst*/
	rst_enable_set(clk);
	/*disable clock*/
	clk_disable_set(clk);
	return;
}

static struct clk_ops clock_ops_vpp = {
	.enable = k3v2_vpp_clk_enable,
	.disable = k3v2_vpp_clk_disable,
	.round_rate = k3v2_clk_round_rate,
	.set_rate = k3v2_clk_set_rate,
	.set_parent = k3v2_clk_set_parent,
#ifdef CONFIG_DEBUG_FS
	.check_enreg = k3v2_enabled_check,
	.check_selreg = k3v2_source_check,
	.check_divreg = k3v2_rate_check,
#endif
};

static struct clk clk_vpp = {
	.name		= "clk_vpp",
	.parent		= &pll2_parent_clk,
	.refcnt		= 0,
	.rate		= PLL2_DIV8_RATE,
	.min_rate	= PLL2_32DIV_RATE,
	.max_rate	= PLL3_RATE,
	.ops		= &clock_ops_vpp,
	.sel_parents	= PLL23_32DIV,
	.enable_reg	= (void __iomem	*)EN_REG1,
	.enable_bit	= 6,
	.clksel_reg	= (void __iomem	*)DIV_REG4,
	.clksel_mask	= 0x8000000,
	.clkdiv_reg	= (void __iomem	*)DIV_REG4,
	.clkdiv_mask	= 0x7C00000,
	.reset_reg	= (void __iomem	*)RST_EN_REG1,\
	.reset_bit	= 6,
	.cansleep	= false,
};

PERIPH_CLK(clk_mmc3, "clk_mmc3", clk_sd,
	0, CLK_SD_RATE, CLK_SD_RATE, CLK_SD_RATE, NULL,
	EN_REG3, 23, NULL, 0x8000000, NULL, 0, RST_EN_REG3, 26)
PERIPH_CLK(clk_mmc2, "clk_mmc2", clk_sd,
	0, CLK_SD_RATE, CLK_SD_RATE, CLK_SD_RATE, NULL,
	EN_REG3, 22, NULL, 0x4000000, NULL, 0, RST_EN_REG3, 25)
PERIPH_CLK(clk_mmc1, "clk_mmc1", clk_sd,
	0, CLK_SD_RATE, CLK_SD_RATE, CLK_SD_RATE, NULL,
	EN_REG3, 21, NULL, 0x2000000, NULL, 0, RST_EN_REG3, 24)

/*hsic clock is defined specially*/
PERIPH_CLK(clk_hsic, "clk_hsic", clk_480M,
	0, CLK_480M_RATE, CLK_480M_RATE, CLK_480M_RATE, NULL,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0)
PERIPH_CLK(clk_hsi, "clk_hsi", pll2_parent_clk,/*hsi clock*/
	0, PLL2_DIV8_RATE, PLL2_32DIV_RATE, PLL3_RATE, PLL23_32DIV,
	EN_REG3, 30, DIV_REG7, 0x200000, DIV_REG7, 0xF0000, RST_EN_REG3, 12)
/* if the registers are set as NULL,it means we can't set them ,
they've been set by hardware defaultly. */

PERIPH_CLK(clk_venc, "clk_venc", pll2_parent_clk,
	0, PLL2_DIV6_RATE, PLL2_32DIV_RATE, PLL3_RATE, PLL23_32DIV,
	EN_REG1, 4, DIV_REG3, 0x8000000, DIV_REG3, 0x7C00000, RST_EN_REG1, 4)
PERIPH_CLK(clk_vdec, "clk_vdec", pll2_parent_clk,
	0, PLL2_DIV4_RATE, PLL2_32DIV_RATE, PLL3_RATE, PLL23_32DIV,
	EN_REG1, 5, DIV_REG4, 0x200000, DIV_REG4, 0x1F0000, RST_EN_REG1, 5)
PERIPH_CLK(clk_ldi0, "clk_ldi0", pll2_parent_clk,
	1, PLL2_DIV6_RATE, PLL4_64DIV_RATE, PLL3_RATE, PLL2345_DIV64,
	EN_REG1, 9, DIV_REG5, 0x60000000, DIV_REG5, 0x1F800000, RST_EN_REG1, 9)
PERIPH_CLK(clk_ldi1, "clk_ldi1", pll2_parent_clk,
	0, PLL2_DIV6_RATE, PLL4_64DIV_RATE, PLL3_RATE, PLL2345_DIV64,
	EN_REG1, 11, DIV_REG6, 0xC0000000, DIV_REG6, 0x3F000000, RST_EN_REG1, 11)
PERIPH_CLK(clk_isp, "clk_isp", pll2_parent_clk,
	0, PLL2_DIV8_RATE, PLL2_32DIV_RATE, PLL3_RATE, PLL23_32DIV,
	EN_REG1, 12, DIV_REG7, 0x8000000, DIV_REG7, 0x7C00000, RST_EN_REG1, 12)
PERIPH_CLK(clk_isp_cs, "clk_isp", pll2_parent_clk,
	0, PLL2_DIV8_RATE, PLL2_32DIV_RATE, PLL3_RATE, PLL23_32DIV,
	EN_REG1, 1, DIV_REG7, 0x8000000, DIV_REG7, 0x7C00000, RST_EN_REG1, 12)
/*
 *clk_asphdmi is used for data transforming between asp and hdmi,
 *it's different with clk_hdmi
 */
PERIPH_CLK(clk_asphdmi, "clk_asphdmi", pll2_parent_clk,
	0, PLL2_DIV4_RATE, PLL2_16DIV_RATE, PLL3_RATE, PLL23_16DIV,
	NULL, 0, DIV_REG9, 0x10000000, DIV_REG9, 0xF000000, NULL, 0)
PERIPH_CLK(clk_ispmipi, "clk_ispmipi", pll2_parent_clk,
	0, PLL2_DIV8_RATE, PLL2_32DIV_RATE, PLL3_RATE, PLL23_32DIV,
	EN_REG1, 3, DIV_REG10, 0x80000000, DIV_REG10, 0x7C000000, NULL, 0)
PERIPH_CLK(clk_bt, "clk_bt", clk_480M,
	0, 60000000, CLKIN_RF_RATE, CLK_480M_RATE, CLKSEL_480M26M,
	EN_REG0, 7, DIV_REG13, 0x1000000, DIV_REG11, 0x3C000000, RST_EN_REG1, 18)
PERIPH_CLK(clk_clockout1, "clk_clockout1", clkin_rf32k,
	0, CLKIN_RF32K_RATE, CLKIN_RF32K_RATE, CLKIN_SYS_RATE,
	CLKSEL_32K26MDIV8, EN_REG1, 21, DIV_REG13, 0x100000,
	DIV_REG13, 0xE00000, NULL, 0)
PERIPH_CLK(clk_clockout0, "clk_clockout0", clkin_rf32k,
	0, CLKIN_RF32K_RATE, CLKIN_RF32K_RATE, CLKIN_SYS_RATE,
	CLKSEL_32K26MDIV8, EN_REG1, 20, DIV_REG13, 0x10000,
	DIV_REG13, 0xE0000, NULL, 0)
PERIPH_CLK(clk_pwm0, "clk_pwm0", clkin_rf32k,
	0, CLKIN_RF32K_RATE, CLKIN_RF32K_RATE, CLKIN_SYS_RATE,
	CLKSEL_32K26MDIV32, EN_REG2, 7, DIV_REG1, 0x4000000,
	DIV_REG1, 0x3E00000, RST_EN_REG2, 7)
PERIPH_CLK(clk_kpc, "clk_kpc", clkin_rf32k,
	0, 16384, 8192, CLKIN_RF32K_RATE, NULL,
	EN_REG0, 6, NULL, 0, DIV_REG1, 0x30000000, RST_EN_REG0, 4)
PERIPH_CLK(clk_spi2, "clk_spi2", clkin_sys,
	0, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CFGAXI_NORMAL_RATE, CLKSRC_CFGAXI26M,
	EN_REG2, 23, DIV_REG0, 0x4000000, NULL, 0, RST_EN_REG2, 23)
PERIPH_CLK(clk_spi1, "clk_spi1", clkin_sys,
	0, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CFGAXI_NORMAL_RATE, CLKSRC_CFGAXI26M,
	EN_REG2, 22, DIV_REG0, 0x2000000, NULL, 0, RST_EN_REG2, 22)
PERIPH_CLK(clk_spi0, "clk_spi0", clkin_sys,
	0, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CFGAXI_NORMAL_RATE, CLKSRC_CFGAXI26M,
	EN_REG2, 21, DIV_REG0, 0x1000000, NULL, 0, RST_EN_REG2, 21)
PERIPH_CLK(clk_uart4, "clk_uart4", clkin_sys,
	0, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CFGAXI_NORMAL_RATE, CLKSRC_CFGAXI26M,
	EN_REG2, 20, DIV_REG0, 0x8000000, NULL, 0, RST_EN_REG2, 20)
PERIPH_CLK(clk_uart3, "clk_uart3", clkin_sys,
	0, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CFGAXI_NORMAL_RATE, CLKSRC_CFGAXI26M,
	EN_REG2, 19, DIV_REG0, 0x4000000, NULL, 0, RST_EN_REG2, 19)
PERIPH_CLK(clk_uart2, "clk_uart2", clkin_sys,
	0, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CFGAXI_NORMAL_RATE, CLKSRC_CFGAXI26M,
	EN_REG2, 18, DIV_REG0, 0x2000000, NULL, 0, RST_EN_REG2, 18)
PERIPH_CLK(clk_uart1, "clk_uart1", clkin_sys,
	0, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CFGAXI_NORMAL_RATE, CLKSRC_CFGAXI26M,
	EN_REG2, 17, DIV_REG0, 0x1000000, NULL, 0, RST_EN_REG2, 17)
PERIPH_CLK(clk_uart0, "clk_uart0", clkin_sys,
	0, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CFGAXI_NORMAL_RATE, CLKSRC_CFGAXI26M,
	EN_REG2, 16, DIV_REG0, 0x800000, NULL, 0, RST_EN_REG2, 16)
PERIPH_CLK(clk_i2c0, "clk_i2c0", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG2, 24, NULL, 0, NULL, 0, RST_EN_REG2, 24)
PERIPH_CLK(clk_i2c1, "clk_i2c1", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG2, 25, NULL, 0, NULL, 0, RST_EN_REG2, 25)
PERIPH_CLK(clk_i2c2, "clk_i2c2", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG2, 28, NULL, 0, NULL, 0, RST_EN_REG2, 28)
PERIPH_CLK(clk_i2c3, "clk_i2c3", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG2, 29, NULL, 0, NULL, 0, RST_EN_REG2, 28)
PERIPH_CLK(clk_timer0, "clk_timer0", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG0, 1, NULL, 0, NULL, 0, RST_EN_REG0, 0)
PERIPH_CLK(clk_timer1, "clk_timer1", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG0, 3, NULL, 0, NULL, 0, RST_EN_REG0, 1)
PERIPH_CLK(clk_timer2, "clk_timer2", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG2, 3, NULL, 0, NULL, 0, RST_EN_REG2, 3)
PERIPH_CLK(clk_timer3, "clk_timer3", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG2, 4, NULL, 0, NULL, 0, RST_EN_REG2, 4)
PERIPH_CLK(clk_timer4, "clk_timer4", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG2, 5, NULL, 0, NULL, 0, RST_EN_REG2, 5)
PERIPH_CLK(clk_edc1, "clk_edc1", pll2_parent_clk,
	0, PLL2_DIV5_RATE, PLL2_64DIV_RATE, PLL3_RATE, PLL23_EDC1,
	EN_REG1, 10, DIV_REG6, 0xC00000, DIV_REG6, 0x3F0000, RST_EN_REG1, 10)
PERIPH_CLK(clk_aspspdif, "clk_aspspdif", clk_480M,
	0, 25000000, 25000000, 25000000, NULL,
	EN_REG3, 28, NULL, 0, NULL, 0, NULL, 0)
PERIPH_CLK(clk_sci, "clk_sci", clkin_sys,
	0, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CLKIN_SYS_RATE, NULL,
	EN_REG2, 26, NULL, 0, NULL, 0, RST_EN_REG2, 26)
PERIPH_CLK(clk_dphy2, "clk_dphy2", clkin_sys,
	0, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CLKIN_SYS_RATE, NULL,
	EN_REG1, 17, NULL, 0, NULL, 0, RST_EN_REG1, 17)
PERIPH_CLK(clk_dphy1, "clk_dphy1", clkin_sys,
	0, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CLKIN_SYS_RATE, NULL,
	EN_REG1, 16, NULL, 0, NULL, 0, RST_EN_REG1, 16)
PERIPH_CLK(clk_dphy0, "clk_dphy0", clkin_sys,
	1, CLKIN_SYS_RATE, CLKIN_SYS_RATE, CLKIN_SYS_RATE, NULL,
	EN_REG1, 15, NULL, 0, NULL, 0, RST_EN_REG1, 15)
PERIPH_CLK(clk_usb2hst, "clk_usb2hst", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG3, 18, NULL, 0, NULL, 0, RST_EN_REG3, 18)
PERIPH_CLK(clk_usb2dvc, "clk_usb2dvc", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG3, 17, NULL, 0, NULL, 0, RST_EN_REG3, 17)
PERIPH_CLK(aclk_isp, "aclk_isp", clk_cfgaxi,
	0, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, CFGAXI_NORMAL_RATE, NULL,
	EN_REG1, 12, NULL, 0, NULL, 0, NULL, 0)

static int k3v2_edc0_clk_enable(struct clk *clk)
{
	BUG_ON(clk == NULL);

	/*step1:enable edc clock and ldi clock*/
	clk_enable_set(clk);
	clk_enable_set(&clk_ldi0);
	/*step2:enable rst*/
	rst_enable_set(clk);
	udelay(50);
	/*set rate to 64 div*/
	k3v2_clk_set_rate(clk, PLL2_64DIV_RATE);
	/*step4:disable edc clock and ldi clock*/
	clk_disable_set(clk);
	clk_disable_set(&clk_ldi0);
	/*step5:disable rst*/
	rst_disable_set(clk);
	udelay(50);
	/*step6:enable clock*/
	if (clk_ldi0.refcnt > 0)
		clk_enable_set(&clk_ldi0);
	clk_enable_set(clk);
	return 0;
}

static struct clk_ops clock_edc0_ops = {
	.enable = k3v2_edc0_clk_enable,
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

static struct clk clk_edc0 = {
	.name		= "clk_edc0",
	.parent		= &pll2_parent_clk,
	.refcnt		= 1,
	.rate		= PLL2_DIV5_RATE,
	.min_rate	= PLL2_64DIV_RATE,
	.max_rate	= PLL3_RATE,
	.ops		= &clock_edc0_ops,
	.sel_parents	= PLL23_DIV64,
	.enable_reg	= (void __iomem	*)EN_REG1,
	.enable_bit	= 8,
	.clksel_reg	= (void __iomem	*)DIV_REG5,
	.clksel_mask	= 0x400000,
	.clkdiv_reg	= (void __iomem	*)DIV_REG5,
	.clkdiv_mask	= 0x3F0000,
	.reset_reg	= (void __iomem	*)RST_EN_REG1,
	.reset_bit	= 8,
	.cansleep	= false,
};

static void g2d_enable_softrst(struct clk *clk)
{
	/*enable rst including core and intf for 32 cycle*/
	writel(G2D_RST_MASK, RST_EN_REG1);
	/*delay circle*/
	udelay(5);/*50 cycles of 30M HZ clock*/
	/*disable core rst*/
	writel(G2D_CORE_RST_BIT, RST_EN_REG1 + DISABLE_RESET_OFFSET);
	/*disable intf rst*/
	writel(G2D_INTF_RST_BIT, RST_EN_REG1 + DISABLE_RESET_OFFSET);
	/*keep 128 cycles*/
	udelay(5);/*150 cycles(more than 128 cycles) of 32M HZ clock*/
}

static int k3v2_g2d_clk_enable(struct clk *clk)
{
	BUG_ON(clk == NULL);

	/*step1:enable clock*/
	clk_enable_set(clk);
	/*step2:enable rst*/
	g2d_enable_softrst(clk);
	return 0;
}

static struct clk_ops clock_g2d_ops = {
	.enable = k3v2_g2d_clk_enable,
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

static struct clk clk_g2d = {
	.name		= "clk_g2d",
	.parent		= &pll2_parent_clk,
	.refcnt		= 0,
	.rate		= PLL2_DIV5_RATE,
	.min_rate	= PLL2_32DIV_RATE,
	.max_rate	= PLL3_RATE,
	.ops		= &clock_g2d_ops,
	.sel_parents	= PLL23_32DIV,
	.enable_reg	= (void __iomem	*)EN_REG1,
	.enable_bit	= 2,
	.clksel_reg	= (void __iomem	*)DIV_REG3,
	.clksel_mask	= 0x200000,
	.clkdiv_reg	= (void __iomem	*)DIV_REG3,
	.clkdiv_mask	= 0x1F0000,
	.reset_reg	= (void __iomem	*)RST_EN_REG1,
	.reset_bit	= 3,
	.cansleep	= false,
};

/* child clocks without div registers*/
CLK_FROM_PLL3(clk_usbhsicphy480, "clk_usbhsicphy480", 0, CLK_480M_RATE, \
	CLK_480M_RATE, CLK_480M_RATE, clock_ops, EN_REG1, 26, NULL, 0)
CLK_FROM_PLL3(clk_usbhsicphy, "clk_usbhsicphy", 0, CLK_480MDIV40_RATE, \
	CLK_480MDIV40_RATE, CLK_480MDIV40_RATE, clock_ops, EN_REG1, 25, RST_EN_REG1, 25)
CLK_FROM_PLL3(clk_usbpicophy, "clk_usbpicophy", 0, CLK_480MDIV40_RATE, \
	CLK_480MDIV40_RATE, CLK_480MDIV40_RATE, clock_ops, EN_REG1, 24, RST_EN_REG1, 24)
CLK_FROM_PLL3(clk_usbnanophy, "clk_usbnanophy", 0, CLK_480MDIV40_RATE, \
	CLK_480MDIV40_RATE, CLK_480MDIV40_RATE, clock_ops, EN_REG1, 23, RST_EN_REG1, 23)

/*
 *these clocks are not sensitive to cfgaxi's rate,
 *so they are defined wihout parents.
 */
NO_DIVISION_CLK(clk_gpio0, "clk_gpio0", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 8, RST_EN_REG0, 8)
NO_DIVISION_CLK(clk_gpio1, "clk_gpio1", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 9, RST_EN_REG0, 9)
NO_DIVISION_CLK(clk_gpio2, "clk_gpio2", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 10, RST_EN_REG0, 10)
NO_DIVISION_CLK(clk_gpio3, "clk_gpio3", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 11, RST_EN_REG0, 11)
NO_DIVISION_CLK(clk_gpio4, "clk_gpio4", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 12, RST_EN_REG0, 12)
NO_DIVISION_CLK(clk_gpio5, "clk_gpio5", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 13, RST_EN_REG0, 13)
NO_DIVISION_CLK(clk_gpio6, "clk_gpio6", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 14, RST_EN_REG0, 14)
NO_DIVISION_CLK(clk_gpio7, "clk_gpio7", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 15, RST_EN_REG0, 15)
NO_DIVISION_CLK(clk_gpio8, "clk_gpio8", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 16, RST_EN_REG0, 16)
NO_DIVISION_CLK(clk_gpio9, "clk_gpio9", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 17, RST_EN_REG0, 17)
NO_DIVISION_CLK(clk_gpio10, "clk_gpio10", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 18, RST_EN_REG0, 18)
NO_DIVISION_CLK(clk_gpio11, "clk_gpio11", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 19, RST_EN_REG0, 19)
NO_DIVISION_CLK(clk_gpio12, "clk_gpio12", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 20, RST_EN_REG0, 20)
NO_DIVISION_CLK(clk_gpio13, "clk_gpio13", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 21, RST_EN_REG0, 21)
NO_DIVISION_CLK(clk_gpio14, "clk_gpio14", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 22, RST_EN_REG0, 22)
NO_DIVISION_CLK(clk_gpio15, "clk_gpio15", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 23, RST_EN_REG0, 23)
NO_DIVISION_CLK(clk_gpio16, "clk_gpio16", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 24, RST_EN_REG0, 24)
NO_DIVISION_CLK(clk_gpio17, "clk_gpio17", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 25, RST_EN_REG0, 25)
NO_DIVISION_CLK(clk_gpio18, "clk_gpio18", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 26, RST_EN_REG0, 26)
NO_DIVISION_CLK(clk_gpio19, "clk_gpio19", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 27, RST_EN_REG0, 27)
NO_DIVISION_CLK(clk_gpio20, "clk_gpio20", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 28, RST_EN_REG0, 28)
NO_DIVISION_CLK(clk_gpio21, "clk_gpio21", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 29, RST_EN_REG0, 29)
NO_DIVISION_CLK(clk_mcu, "clk_mcu", 0, CFGAXI_NORMAL_RATE,\
	EN_REG3, 24, RST_EN_REG3, 27)
NO_DIVISION_CLK(clk_wd, "clk_wd", 0, CFGAXI_NORMAL_RATE, \
	EN_REG2, 6, NULL, 0)
NO_DIVISION_CLK(clk_asp, "clk_asp", 0, CFGAXI_NORMAL_RATE, \
	EN_REG3, 14, RST_EN_REG3, 14)
NO_DIVISION_CLK(clk_rtc, "clk_rtc", 0, CFGAXI_NORMAL_RATE, \
	EN_REG0, 5, RST_EN_REG0, 3)
NO_DIVISION_CLK(clk_dmac, "clk_dmac", 0, CFGAXI_NORMAL_RATE, \
	EN_REG3, 10, RST_EN_REG3, 10)

/*these clocks have no parents and don't need to divide*/
NO_DIVISION_CLK(clk_acp, "clk_acp", 0, PLL2_DIV2_RATE, \
	EN_REG1, 28, NULL, 29)
NO_DIVISION_CLK(clk_hdmi, "clk_hdmi", 0, CFGAXI_NORMAL_RATE, \
	EN_REG1, 18, NULL, 0)
NO_DIVISION_CLK(clk_g3d, "clk_g3d", 0, CFGAXI_NORMAL_RATE, \
	EN_REG1, 0, RST_EN_REG1, 14)
NO_DIVISION_CLK(clk_aspsio, "clk_aspsio", 0, PLL2_DIV10_RATE, \
	EN_REG3, 26, NULL, 0)
NO_DIVISION_CLK(clk_pclk_hdmi, "clk_pclk_hdmi", 0, CFGAXI_NORMAL_RATE, \
	NULL, 0, NULL, 0)

static int k3v2_clkpmu_enable(struct clk *clk)
{
	BUG_ON(clk == NULL);

	/*clock enable*/
	if (clk->enable_reg == NULL) {
		pr_warning("the enable register is NULL.\n");
		return 0; /* return 0 just for count */
	}

	/*set division of 120M spi as 8 for pmuspi clock*/
	writel(0xFF0003, IO_ADDRESS(REG_BASE_PCTRL + 0x08));
	/*enable clock*/
	writel((u32)(1 << clk->enable_bit), clk->enable_reg);

	/*rst disable*/
	/* rst_disable_set(clk); */
	return 0;
}

static struct clk_ops clock_pmu_ops = {
	.enable = k3v2_clkpmu_enable,
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

static struct clk clk_pmuspi = {
	.name		= "clk_pmuspi",
	.parent		= NULL,
	.refcnt		= 0,
	.rate		= CLK_PMUSPI_RATE,
	.min_rate	= CLK_PMUSPI_RATE,
	.max_rate	= CLK_PMUSPI_RATE,
	.ops		= &clock_pmu_ops,
	.sel_parents	= NULL,
	.enable_reg	= (void __iomem	*)EN_REG2,
	.enable_bit	= 1,
	.clksel_reg	= NULL,
	.clkdiv_reg	= NULL,
	.reset_reg	=  (void __iomem *)RST_EN_REG2,
	.reset_bit = 1,
};

/*define pmu 32k clock*/
static int k3v2_pmu32kclk_enable(struct clk *clk)
{
	int value = 0;
	int ret = 0;

	BUG_ON(clk == NULL);

	/*open pmuspi clock*/
	ret = clk_enable(&clk_pmuspi);
	if (ret) {
		pr_err("clk_pmuspi enable failed!\r\n");
		return -1;
	}

	/*open 32k clock*/
	value = readl(clk->enable_reg);
	value = value | (1 << clk->enable_bit);
	writel(value, clk->enable_reg);

	/*close pmuspi clock*/
	clk_disable(&clk_pmuspi);

	return 0;
}

static void k3v2_pmu32kclk_disable(struct clk *clk)
{
	int value = 0;

	BUG_ON(clk == NULL);

	/*open pmuspi clock*/
	clk_enable(&clk_pmuspi);
	/*close 32k clock*/
	value = readl(clk->enable_reg);
	value = value & (~(1 << clk->enable_bit));
	writel(value, clk->enable_reg);
	/*close pmuspi clock*/
	clk_disable(&clk_pmuspi);
	wmb();
}

#ifdef CONFIG_DEBUG_FS
static char g_clk_status[5][10] = {"NOREG", "OK", "ERR", "uninit", "NODIV"};
static char *k3v2_pmu32kenabled_check(struct clk *c)
{
	int regvalue = 0;

	if (!c->enable_reg)
		return g_clk_status[0];

	if (c->refcnt > 0) {
		regvalue = readl(c->enable_reg)
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
			regvalue = readl(c->enable_reg)
				& (1 << c->enable_bit);
			if (regvalue)
				return g_clk_status[2];
			else
				return g_clk_status[1];
		}
	}

	return g_clk_status[2];
}
#endif

static struct clk_ops clock_pmu32kclk_ops = {
	.enable = k3v2_pmu32kclk_enable,
	.disable = k3v2_pmu32kclk_disable,
	.round_rate = k3v2_clk_round_rate,
	.set_rate = k3v2_clk_set_rate,
	.set_parent = k3v2_clk_set_parent,
#ifdef CONFIG_DEBUG_FS
	.check_enreg = k3v2_pmu32kenabled_check,
	.check_selreg = k3v2_source_check,
	.check_divreg = k3v2_rate_check,
#endif
};

#define PMU_32KCLK(_clk, _clkname, _refcnt, _enable_reg, _enable_bit)	\
static struct clk _clk = {\
	.name		= _clkname,\
	.parent		= NULL,\
	.refcnt		= 0,\
	.rate		= CLK_PMU32K_RATE,\
	.min_rate	= CLK_PMU32K_RATE, \
	.max_rate	= CLK_PMU32K_RATE,\
	.ops		= &clock_pmu32kclk_ops,\
	.sel_parents		= NULL,\
	.enable_reg	= (void __iomem	*)(_enable_reg),\
	.enable_bit = _enable_bit,\
	.clksel_reg = NULL,\
	.clkdiv_reg = NULL,\
};
PMU_32KCLK(clk_pmu32ka, "clk_pmu32ka", 0, PMU32K_EN_REG, 0)
PMU_32KCLK(clk_pmu32kb, "clk_pmu32kb", 0, PMU32K_EN_REG, 1)
PMU_32KCLK(clk_pmu32kc, "clk_pmu32kc", 0, PMU32K_EN_REG, 2)

#define _REGISTER_CLOCK(d, n, c) \
		.dev_id = d, \
		.con_id = n, \
		.clk = &c,

struct clk_lookup k3v2_clk_lookups_es[] = {
	/*source  clock*/
	{_REGISTER_CLOCK(NULL,	"clkin_sys", clkin_sys)},
	{_REGISTER_CLOCK(NULL,	"clkin_rf", clkin_rf)},
	{_REGISTER_CLOCK(NULL,	"clkin_rf32k", clkin_rf32k)},
	{_REGISTER_CLOCK(NULL,	"clk_pll0",	clk_pll0_src1600M)},
	{_REGISTER_CLOCK(NULL,	"clk_pll1",	clk_pll1_src1600M)},
	{_REGISTER_CLOCK(NULL,	"clk_pll2",	pll2_parent_clk)},
	{_REGISTER_CLOCK(NULL,	"clk_pll3",	pll3_parent_clk)},
	{_REGISTER_CLOCK(NULL,	"clk_pll4",	pll4_parent_clk)},
	{_REGISTER_CLOCK(NULL,	"edc1_src", edc1_src)},
	{_REGISTER_CLOCK(NULL,	"clk_480M", clk_480M)},
	/*children clocks*/
	{_REGISTER_CLOCK("pl061-rtc",	NULL,	clk_rtc)},
	{_REGISTER_CLOCK("amba-uart.0",	NULL,	clk_uart0)},
	{_REGISTER_CLOCK("amba-uart.1",	NULL,	clk_uart1)},
	{_REGISTER_CLOCK("amba-uart.2",	NULL,	clk_uart2)},
	{_REGISTER_CLOCK("amba-uart.3",	NULL,	clk_uart3)},
	{_REGISTER_CLOCK("amba-uart.4",	NULL,	clk_uart4)},
	{_REGISTER_CLOCK("dw-i2c.0",	NULL,	clk_i2c0)},
	{_REGISTER_CLOCK("dw-i2c.1",	NULL,	clk_i2c1)},
	{_REGISTER_CLOCK("hidmav300",	NULL,	clk_dmac)},
	{_REGISTER_CLOCK("k3_keypad",	NULL,	clk_kpc)},
	{_REGISTER_CLOCK("gpio0",		NULL,	clk_gpio0)},
	{_REGISTER_CLOCK("gpio1",		NULL,	clk_gpio1)},
	{_REGISTER_CLOCK("gpio2",		NULL,	clk_gpio2)},
	{_REGISTER_CLOCK("gpio3",		NULL,	clk_gpio3)},
	{_REGISTER_CLOCK("gpio4",		NULL,	clk_gpio4)},
	{_REGISTER_CLOCK("gpio5",		NULL,	clk_gpio5)},
	{_REGISTER_CLOCK("gpio6",		NULL,	clk_gpio6)},
	{_REGISTER_CLOCK("gpio7",		NULL,	clk_gpio7)},
	{_REGISTER_CLOCK("gpio8",		NULL,	clk_gpio8)},
	{_REGISTER_CLOCK("gpio9",		NULL,	clk_gpio9)},
	{_REGISTER_CLOCK("gpio10",		NULL,	clk_gpio10)},
	{_REGISTER_CLOCK("gpio11",		NULL,	clk_gpio11)},
	{_REGISTER_CLOCK("gpio12",		NULL,	clk_gpio12)},
	{_REGISTER_CLOCK("gpio13",		NULL,	clk_gpio13)},
	{_REGISTER_CLOCK("gpio14",		NULL,	clk_gpio14)},
	{_REGISTER_CLOCK("gpio15",		NULL,	clk_gpio15)},
	{_REGISTER_CLOCK("gpio16",		NULL,	clk_gpio16)},
	{_REGISTER_CLOCK("gpio17",		NULL,	clk_gpio17)},
	{_REGISTER_CLOCK("gpio18",		NULL,	clk_gpio18)},
	{_REGISTER_CLOCK("gpio19",		NULL,	clk_gpio19)},
	{_REGISTER_CLOCK("gpio20",		NULL,	clk_gpio20)},
	{_REGISTER_CLOCK("gpio21",		NULL,	clk_gpio21)},
	{_REGISTER_CLOCK(NULL,	"clk_venc", clk_venc)},
	{_REGISTER_CLOCK(NULL, "clk_g2d", clk_g2d)},
	{_REGISTER_CLOCK(NULL,	"clk_vpp", clk_vpp)},
	{_REGISTER_CLOCK(NULL,	"clk_vdec",	clk_vdec)},
	{_REGISTER_CLOCK(NULL,	"clk_ldi0",	clk_ldi0)},
	{_REGISTER_CLOCK(NULL,	"clk_edc0",	clk_edc0)},
	{_REGISTER_CLOCK(NULL,	"clk_ldi1",	clk_ldi1)},
	{_REGISTER_CLOCK(NULL,	"clk_edc1",	clk_edc1)},
	{_REGISTER_CLOCK(NULL,	"clk_hdmi_m",	clk_hdmi_m)},
	{_REGISTER_CLOCK(NULL,	"clk_asphdmi",	clk_asphdmi)},
	{_REGISTER_CLOCK(NULL,	"clk_isp",	clk_isp)},
	{_REGISTER_CLOCK(NULL,	"clk_ispmipi",	clk_ispmipi)},
	{_REGISTER_CLOCK(NULL,	"clk_bt",		clk_bt)},
	{_REGISTER_CLOCK(NULL,	"clk_hsic",		clk_hsic)},
	{_REGISTER_CLOCK(NULL,	"clk_hsi",		clk_hsi)},
	{_REGISTER_CLOCK(NULL,	"clk_clockout1",	clk_clockout1)},
	{_REGISTER_CLOCK(NULL,	"clk_clockout0",	clk_clockout0)},
	{_REGISTER_CLOCK(NULL,	"clk_pwm0",	clk_pwm0)},
	{_REGISTER_CLOCK(NULL,	"clk_mmc3",	clk_mmc3)},
	{_REGISTER_CLOCK(NULL,	"clk_mmc2",	clk_mmc2)},
	{_REGISTER_CLOCK(NULL,	"clk_mmc1",	clk_mmc1)},
	{_REGISTER_CLOCK(NULL,	"clk_sd",	clk_sd)},
	{_REGISTER_CLOCK("dev:ssp2",	NULL,	clk_spi2)},
	{_REGISTER_CLOCK("dev:ssp1",	NULL,	clk_spi1)},
	{_REGISTER_CLOCK("dev:ssp0",	NULL,	clk_spi0)},
	{_REGISTER_CLOCK(NULL,	"clk_mcu",	clk_mcu)},
	{_REGISTER_CLOCK(NULL,	"clk_cfgaxi",	clk_cfgaxi)},
	{_REGISTER_CLOCK(NULL,	"clk_timer1",	clk_timer1)},
	{_REGISTER_CLOCK(NULL,	"clk_timer0",	clk_timer0)},
	{_REGISTER_CLOCK(NULL,	"clk_acp",		clk_acp)},
	{_REGISTER_CLOCK(NULL,	"clk_usbhsicphy480",	clk_usbhsicphy480)},
	{_REGISTER_CLOCK(NULL,	"clk_usbhsicphy",	clk_usbhsicphy)},
	{_REGISTER_CLOCK(NULL,	"clk_usbpicophy",	clk_usbpicophy)},
	{_REGISTER_CLOCK(NULL,	"clk_usbnanophy",	clk_usbnanophy)},
	{_REGISTER_CLOCK(NULL,	"clk_hdmi",	clk_hdmi)},
	{_REGISTER_CLOCK(NULL,	"clk_dphy2",	clk_dphy2)},
	{_REGISTER_CLOCK(NULL,	"clk_dphy1",	clk_dphy1)},
	{_REGISTER_CLOCK(NULL,	"clk_dphy0",	clk_dphy0)},
	{_REGISTER_CLOCK(NULL,	"clk_g3d",	clk_g3d)},
	{_REGISTER_CLOCK(NULL,	"clk_sci",	clk_sci)},
	{_REGISTER_CLOCK(NULL,	"clk_wd",	clk_wd)},
	{_REGISTER_CLOCK(NULL,	"clk_timer4",	clk_timer4)},
	{_REGISTER_CLOCK(NULL,	"clk_timer3",	clk_timer3)},
	{_REGISTER_CLOCK(NULL,	"clk_timer2",	clk_timer2)},
	{_REGISTER_CLOCK(NULL,	"clk_pmuspi",	clk_pmuspi)},
	{_REGISTER_CLOCK(NULL,	"clk_pmu32ka",	clk_pmu32ka)},
	{_REGISTER_CLOCK(NULL,	"clk_pmu32kb",	clk_pmu32kb)},
	{_REGISTER_CLOCK(NULL,	"clk_pmu32kc",	clk_pmu32kc)},
	{_REGISTER_CLOCK(NULL,	"clk_aspspdif",	clk_aspspdif)},
	{_REGISTER_CLOCK(NULL,	"clk_aspsio",	clk_aspsio)},
	{_REGISTER_CLOCK(NULL,	"clk_usb2hst",	clk_usb2hst)},
	{_REGISTER_CLOCK(NULL,	"clk_usb2dvc",	clk_usb2dvc)},
	{_REGISTER_CLOCK(NULL,	"clk_asp",		clk_asp)},
	{_REGISTER_CLOCK(NULL,	"clk_pclk_hdmi",		clk_pclk_hdmi)},
	{NULL,	NULL,	NULL},
};

