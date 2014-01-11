/*
 *
 * arch/arm/mach-k3v2/include/mach/clock.h
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 * Clock definitions and inline macros
 *
 */
#ifndef __MACH_CLOCK_H
#define __MACH_CLOCK_H

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/clkdev.h>
#include <asm/clkdev.h>

/* PLL Clock enable registers */
#define	PLLEN_BASE	IO_ADDRESS(REG_BASE_PMCTRL)
#define	PLLEN_REG0	(PLLEN_BASE + 0x010)
#define	PLLEN_REG1	(PLLEN_BASE + 0x014)
#define	PLLEN_REG2	(PLLEN_BASE + 0x018)
#define	PLLEN_REG3	(PLLEN_BASE + 0x01C)
#define	PLLEN_REG4	(PLLEN_BASE + 0x020)
#define	PLLEN_REG5	(PLLEN_BASE + 0x028)
#define	PLL_LOCK_BIT	(1 << 26)
#define	G3D_CORE_DIV	(PLLEN_BASE + 0x0B8)
#define	G3D_SHADER_DIV	(PLLEN_BASE + 0x0BC)
#define	G3D_AXI_DIV	(PLLEN_BASE + 0x044)

/******************************
 *[4:0] = 0x18,division of channel 0 is 25
 *[6:5] = 0x00, switch peril pll for channel 0
 *[11:7] = 0x18, division of channel 1 is 25,
 *[13:12] = 0x00, switch peril pll for channel 1
*/
#define	G3D_DIV_EN_VAL	((0x00 << 12) | (0x0B << 7) | 0x0B)
/******************************
 *[4:0] = 0x18,division of channel 0 is 25
 *[6:5] = 0x01, switch peril pll for channel 0
 *[11:7] = 0x18, division of channel 1 is 25,
 *[13:12] = 0x01, switch usb pll for channel 1
*/
#define	G3D_DIV_DIS_VAL	((0x01 << 12) | (0x0B << 7) | (0x01 << 5) | 0x0B)

/* enable registers */
#define	CLKEN_BASE	IO_ADDRESS(REG_BASE_SCTRL)
#define	EN_REG0		(CLKEN_BASE + 0x020)
#define	DISEN_REG0	(CLKEN_BASE + 0x024)
#define	ISEN_REG0	(CLKEN_BASE + 0x028)
#define	EN_REG1		(CLKEN_BASE + 0x030)
#define	DISEN_REG1	(CLKEN_BASE + 0x034)
#define	ISEN_REG1	(CLKEN_BASE + 0x038)
#define	EN_REG2		(CLKEN_BASE + 0x040)
#define	DISEN_REG2	(CLKEN_BASE + 0x044)
#define	ISEN_REG2	(CLKEN_BASE + 0x048)
#define	EN_REG3		(CLKEN_BASE + 0x050)
#define	DISEN_REG3	(CLKEN_BASE + 0x054)
#define	ISEN_REG3	(CLKEN_BASE + 0x058)
#define	PMU32K_EN_REG	(IO_ADDRESS(REG_BASE_PMUSPI) + (0x50 << 2))
/*rst enable registers*/
#define	RST_EN_REG0	(CLKEN_BASE + 0x080)
#define	RST_EN_REG1	(CLKEN_BASE + 0x08C)
#define	RST_EN_REG2	(CLKEN_BASE + 0x098)
#define	RST_EN_REG3	(CLKEN_BASE + 0x0A4)
#define	RST_EN_REG4	(CLKEN_BASE + 0x0B0)
/* div and sel registers */
#define	DIV_REG0	(CLKEN_BASE + 0x100)
#define	DIV_REG1	(CLKEN_BASE + 0x104)
#define	DIV_REG2	(CLKEN_BASE + 0x108)
#define	DIV_REG3	(CLKEN_BASE + 0x10C)
#define	DIV_REG4	(CLKEN_BASE + 0x110)
#define	DIV_REG5	(CLKEN_BASE + 0x114)
#define	DIV_REG6	(CLKEN_BASE + 0x118)
#define	DIV_REG7	(CLKEN_BASE + 0x11C)
#define	DIV_REG8	(CLKEN_BASE + 0x120)
#define	DIV_REG9	(CLKEN_BASE + 0x124)
#define	DIV_REG10	(CLKEN_BASE + 0x128)
#define	DIV_REG11	(CLKEN_BASE + 0x12C)
#define	DIV_REG12	(CLKEN_BASE + 0x130)
#define	DIV_REG13	(CLKEN_BASE + 0x134)
#define	DIV_REG14	(CLKEN_BASE + 0x140)

#define	LOW16BIT_MASK	(0xFFFF)
#define	HDMI_MASK1	(0xFFFF0000)
#define	HDMI_MASK2	(0xFF0000)
#define	SDPLL_MASK	(0x10)
/*clk offset*/
#define	DISABLE_REG_OFFSET		(0x04)
#define	IS_ENABLE_REG_OFFSET		(0x08)
#define	ENABLE_STATUS_REG_OFFSET	(0x0C)
/*rst offset*/
#define	DISABLE_RESET_OFFSET		(0x04)
#define	RST_STATUS_REG_OFFSET		(0x08)

#define	G2D_RST_MASK			((0x01 << 2) | (0x01 << 3))
#define	G2D_CORE_RST_BIT		(0x01 << 2)
#define	G2D_INTF_RST_BIT		(0x01 << 3)
/*define rate*/
/*PLL0 & PLL1 rate*/
#define PLL0_RATE			(1600000000)
#define PLL1_RATE			(1600000000)

/*PLL2 is defined at clock_es.h and clock_cs.h*/

/*PLL3*/
#define PLL3_RATE		(1440000000)/*1440M*/
#define PLL3_64DIV_RATE		(PLL3_RATE >> 6)
#define PLL3_32DIV_RATE		(PLL3_RATE >> 5)
#define PLL3_16DIV_RATE		(PLL3_RATE >> 4)
#define PLL3_8DIV_RATE		(PLL3_RATE >> 3)
#define PLL3_4DIV_RATE		(PLL3_RATE >> 2)
#define PLL3_2DIV_RATE		(PLL3_RATE >> 1)
#define PLL3_DIV3_RATE		(PLL3_RATE / 3)
/*PLL4*/
#define PLL4_RATE			(1188000000)/*1188M*/
#define PLL4_64DIV_RATE		(PLL4_RATE >> 6)
#define PLL4_32DIV_RATE		(PLL4_RATE >> 5)
#define PLL4_16DIV_RATE		(PLL4_RATE >> 4)
#define PLL4_8DIV_RATE		(PLL4_RATE >> 3)
#define PLL4_4DIV_RATE		(PLL4_RATE >> 2)
#define PLL4_2DIV_RATE		(PLL4_RATE >> 1)
/*PLL5*/
#define PLL5_RATE			(1300000000)/*1300M*/
#define PLL5_64DIV_RATE		(PLL5_RATE >> 6)
#define PLL5_32DIV_RATE		(PLL5_RATE >> 5)
#define PLL5_16DIV_RATE		(PLL5_RATE >> 4)
#define PLL5_8DIV_RATE		(PLL5_RATE >> 3)
#define PLL5_4DIV_RATE		(PLL5_RATE >> 2)
#define PLL5_2DIV_RATE		(PLL5_RATE >> 1)

/*others parent clock*/
#define CLKIN_SYS_RATE		(26000000)/*26M*/
#define CLKIN_SYS_8DIV_RATE	(CLKIN_SYS_RATE >> 3)
#define CLKIN_SYS_32DIV_RATE	(CLKIN_SYS_RATE >> 5)
#define CLKIN_RF_RATE		(26000000)/*26M*/
#define CLKIN_RF32K_RATE	(32768)/*32K*/
#define EDC1_SRC_RATE		(26000000)/*26M*/
#define CLK_480M_RATE		(480000000)/*480M*/
#define CLK_480MDIV16_RATE	(CLK_480M_RATE >> 4)/*480M/16*/
#define CLK_480MDIV40_RATE	(CLK_480M_RATE / 40)
#define CLK_SD_RATE			(160000000)/*160M*/
#define CLK_NODIV_BUS_RATE	(108000000)/*105M*/
#define CLK_PMUSPI_RATE		(15000000)
#define CLK_PMU32K_RATE	(32768)

/*
	MMC clock configuration (m53980)
	[12]	= 1'b0		; with [3:0] 5'b01100 is 13 division
	[3:0]	= 4'b1100	;
	[11]	= 1'b0		; 0 is 2 division, so mmc3 clock is 40M
	[10]	= 1'b0		; 0 is 2 division, so mmc2 clock is 40M
	[9]	= 1'b1		; 1 is pll clock, so mmc1 clock is 80M
	[8:6]	= 3'b000	; 0 is 1 division, so mmc1/mmc2/mmc3 clock source is 80M
				; let these bits done in mmc driver
	[5]	= 1'b0		; 0 is 2 division, so the mmc0 clock is 40M
				; let this bit done in mmc driver
	[4]	= 1'b0		; perilpll is 1080M, so the clock source is 1080M/13 = 83M
*/
#define SDSEL_VALUE ((1<<4)<<16)
#define SDDIV_VALUE ((0x1E0F<<16) | (0x0C) | (1<<9))

struct clk;
struct clksel {
	struct clk	*sel_parent;
	/* the value should be set to the registers' enable bit */
	int	sel_val;
	int	parent_min;
	int	parent_max;
};

enum clk_state {
	UNINITIALIZED = 0,
	ON,
	OFF,
};

struct clk_ops {
	void	(*init)(struct clk *);
	int	(*enable)(struct clk *clk);
	void	(*disable)(struct clk *clk);
	long	(*round_rate)(struct clk *clk, long rate);
	int	(*set_rate)(struct clk *clk, unsigned rate);
	int	(*set_parent)(struct clk *clk, struct clk *parent);
	int	(*is_enabled)(struct clk *clk);
	void	(*reset)(struct clk *, bool);
#ifdef CONFIG_DEBUG_FS
	char *(*check_enreg)(struct clk *clk);
	char *(*check_selreg)(struct clk *clk);
	char *(*check_divreg)(struct clk *clk);
#endif
};

struct clk {
#ifdef CONFIG_DEBUG_FS
	struct dentry	*dent;
	struct dentry	*parent_dent;
#endif
	char	*name;
	struct clk	*parent;
	unsigned int	refcnt;
	unsigned long	rate;
	unsigned int	min_rate;
	unsigned int	max_rate;
	u32	div;
	u32	mul;
	struct clk	*child;
	struct clk	*friend;
	enum clk_state	state;
	bool	set;
	u32	flags;
	struct clk_ops	*ops;
	struct clksel	*sel_parents;
	void __iomem	*enable_reg;
	u8	enable_bit;
	void __iomem	*reset_reg;
	u8	reset_bit;
	void __iomem	*clksel_reg;
	u32	clksel_mask;
	void __iomem	*clkdiv_reg;
	u32	clkdiv_mask;
	bool	is_enabled;
	bool	cansleep;
	struct mutex	mutex;
	spinlock_t	spinlock;
	struct list_head	node;
};

#define PERIPH_CLK(_clk, _clkname, _parent, _refcnt, _rate, _min_rate, \
	_max_rate, _sel_parents, _enable_reg, _enable_bit, _clksel_reg,	\
	_clksel_mask, _clkdiv_reg, _clkdiv_mask, _reset_reg, _reset_bit)	\
static struct clk _clk = {\
	.name		= _clkname,\
	.parent		= &_parent,\
	.refcnt		= _refcnt,\
	.rate		= _rate,\
	.min_rate	= _min_rate, \
	.max_rate	= _max_rate,\
	.ops		= &clock_ops,\
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

#define PERIPH_CLK_CS(_clk, _clkname, _parent, _refcnt, _rate, _min_rate, \
	_max_rate, _sel_parents, _enable_reg, _enable_bit, _clksel_reg,	\
	_clksel_mask, _clkdiv_reg, _clkdiv_mask, _reset_reg, _reset_bit)	\
static struct clk _clk = {\
	.name		= _clkname,\
	.parent		= &_parent,\
	.refcnt		= _refcnt,\
	.rate		= _rate,\
	.min_rate	= _min_rate, \
	.max_rate	= _max_rate,\
	.ops		= &clock_ops_cs,\
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

/* child clock witout div registers */
#define NO_DIVISION_CLK(_clk, _name, _refcnt, _rate, _enable_reg, _enable_bit, _reset_reg, _reset_bit)	\
static struct clk _clk = {\
	.name		= _name,\
	.parent		= NULL,\
	.refcnt		= _refcnt,\
	.rate		= _rate,\
	.min_rate	= _rate, \
	.max_rate	= _rate,\
	.ops		= &clock_ops,\
	.sel_parents	= NULL,\
	.enable_reg	= (void __iomem	*)_enable_reg,\
	.enable_bit	= _enable_bit,\
	.clksel_reg	= NULL,\
	.clkdiv_reg	= NULL,\
	.reset_reg	= (void __iomem	*)_reset_reg,\
	.reset_bit	= _reset_bit,\
};

/* source clocks */
#define SRC_CLK(_clk, _name, _refcnt, _rate, _state, _enable_reg, \
	_enable_bit, _clkops)	\
static struct clk _clk = {\
	.name		= _name,\
	.parent		= NULL,\
	.refcnt		= _refcnt,\
	.ops		= _clkops,\
	.rate		= _rate, \
	.min_rate	= _rate, \
	.max_rate	= _rate,\
	.enable_reg	= (void __iomem	*)_enable_reg,\
	.enable_bit	= _enable_bit,\
	.state = _state,\
};

#define CLK_FROM_PLL3(_clk, _clkname, _refcnt, _rate, _min_rate, _max_rate, \
	_ops, _enable_reg, _enable_bit, _reset_reg, _reset_bit) \
static struct clk _clk = {\
	.name		= _clkname,\
	.parent		= &pll3_parent_clk,\
	.refcnt		= 0,\
	.rate		= _rate,\
	.min_rate	= _min_rate, \
	.max_rate	= _max_rate,\
	.ops		= &_ops,\
	.enable_reg	= (void __iomem	*)_enable_reg,\
	.enable_bit	= _enable_bit,\
	.reset_reg	= (void __iomem	*)_reset_reg,\
	.reset_bit	= _reset_bit,\
	.cansleep	= false,\
};

#define CLK_FROM_PLL2(_clk, _clkname, _refcnt, _rate, _min_rate, _max_rate, \
	_ops, _enable_reg, _enable_bit, _reset_reg, _reset_bit) \
static struct clk _clk = {\
	.name		= _clkname,\
	.parent		= &pll2_parent_clk,\
	.refcnt		= 0,\
	.rate		= _rate,\
	.min_rate	= _min_rate, \
	.max_rate	= _max_rate,\
	.ops		= &_ops,\
	.enable_reg	= (void __iomem	*)_enable_reg,\
	.enable_bit	= _enable_bit,\
	.reset_reg	= (void __iomem	*)_reset_reg,\
	.reset_bit	= _reset_bit,\
	.cansleep	= false,\
};

struct k3v2_clk_init_table {
	char *name;
	char *parent;
	unsigned long rate;
	bool enabled;
};

#ifdef CONFIG_DEBUG_FS
char *k3v2_enabled_check(struct clk *c);
char *k3v2_source_check(struct clk *c);
char *k3v2_rate_check(struct clk *c);
#endif
void clk_enable_set(struct clk *clk);
void clk_disable_set(struct clk *clk);
void rst_enable_set(struct clk *clk);
void rst_disable_set(struct clk *clk);
int k3v2_clk_enable(struct clk *clk);
void k3v2_clk_disable(struct clk *clk);
int k3v2_clk_set_parent(struct clk *clk, struct clk *parent);
int k3v2_clk_switch_pll(struct clk *clk, struct clk *parent);
long k3v2_clk_round_rate(struct clk *clk, long rate);
int k3v2_clk_set_rate(struct clk *clk, unsigned rate);
extern struct clk_ops clock_ops;
extern struct clk_ops clock_ops_src;

extern struct clk_lookup k3v2_clk_lookups_cs[];
#ifdef CONFIG_SUPPORT_B3750000_BITRATE
extern struct clk_lookup k3v2_clk_lookups_cs_60M[];
#endif
extern struct clk_lookup k3v2_clk_lookups_es[];

void k3v2_clk_init_from_table(struct k3v2_clk_init_table *table);
void k3v2_init_clocks(void);
void clk_init(struct clk *clk);
void k3v2_init_clock(void);
#endif
