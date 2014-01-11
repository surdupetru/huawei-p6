/*
 * arch/arm/mach-tegra/usb_phy.c
 *
 * Copyright (C) 2010 Google, Inc.
 *
 * Author:
 *	Erik Gilling <konkers@google.com>
 *	Benoit Goby <benoit@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/resource.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/usb/otg.h>
#include <linux/usb/ulpi.h>
#include <asm/mach-types.h>
#include <mach/platform.h>
#include <mach/usb_phy.h>



#define USB_PORTSC1		0x184
#define   USB_PORTSC1_PTS(x)	(((x) & 0x3) << 30)
#define   USB_PORTSC1_PSPD(x)	(((x) & 0x3) << 26)
#define   USB_PORTSC1_PHCD	(1 << 23)
#define   USB_PORTSC1_WKOC	(1 << 22)
#define   USB_PORTSC1_WKDS	(1 << 21)
#define   USB_PORTSC1_WKCN	(1 << 20)
#define   USB_PORTSC1_PTC(x)	(((x) & 0xf) << 16)
#define   USB_PORTSC1_PP	(1 << 12)
#define   USB_PORTSC1_SUSP	(1 << 7)
#define   USB_PORTSC1_PE	(1 << 2)
#define   USB_PORTSC1_CCS	(1 << 0)

#define USB_SUSP_CTRL		0x400
#define   USB_WAKE_ON_CNNT_EN_DEV	(1 << 3)
#define   USB_WAKE_ON_DISCON_EN_DEV	(1 << 4)
#define   USB_SUSP_CLR		(1 << 5)
#define   USB_PHY_CLK_VALID	(1 << 7)
#define   UTMIP_RESET			(1 << 11)
#define   UHSIC_RESET			(1 << 11)
#define   UTMIP_PHY_ENABLE		(1 << 12)
#define   ULPI_PHY_ENABLE	(1 << 13)
#define   USB_SUSP_SET		(1 << 14)
#define   USB_WAKEUP_DEBOUNCE_COUNT(x)	(((x) & 0x7) << 16)

#define USB1_LEGACY_CTRL	0x410
#define   USB1_NO_LEGACY_MODE			(1 << 0)
#define   USB1_VBUS_SENSE_CTL_MASK		(3 << 1)
#define   USB1_VBUS_SENSE_CTL_VBUS_WAKEUP	(0 << 1)
#define   USB1_VBUS_SENSE_CTL_AB_SESS_VLD_OR_VBUS_WAKEUP \
						(1 << 1)
#define   USB1_VBUS_SENSE_CTL_AB_SESS_VLD	(2 << 1)
#define   USB1_VBUS_SENSE_CTL_A_SESS_VLD	(3 << 1)

#define ULPI_TIMING_CTRL_0	0x424
#define   ULPI_OUTPUT_PINMUX_BYP	(1 << 10)
#define   ULPI_CLKOUT_PINMUX_BYP	(1 << 11)

#define ULPI_TIMING_CTRL_1	0x428
#define   ULPI_DATA_TRIMMER_LOAD	(1 << 0)
#define   ULPI_DATA_TRIMMER_SEL(x)	(((x) & 0x7) << 1)
#define   ULPI_STPDIRNXT_TRIMMER_LOAD	(1 << 16)
#define   ULPI_STPDIRNXT_TRIMMER_SEL(x)	(((x) & 0x7) << 17)
#define   ULPI_DIR_TRIMMER_LOAD		(1 << 24)
#define   ULPI_DIR_TRIMMER_SEL(x)	(((x) & 0x7) << 25)

#define UTMIP_PLL_CFG1		0x804
#define   UTMIP_XTAL_FREQ_COUNT(x)		(((x) & 0xfff) << 0)
#define   UTMIP_PLLU_ENABLE_DLY_COUNT(x)	(((x) & 0x1f) << 27)

#define UTMIP_XCVR_CFG0		0x808
#define   UTMIP_XCVR_SETUP(x)			(((x) & 0xf) << 0)
#define   UTMIP_XCVR_LSRSLEW(x)			(((x) & 0x3) << 8)
#define   UTMIP_XCVR_LSFSLEW(x)			(((x) & 0x3) << 10)
#define   UTMIP_FORCE_PD_POWERDOWN		(1 << 14)
#define   UTMIP_FORCE_PD2_POWERDOWN		(1 << 16)
#define   UTMIP_FORCE_PDZI_POWERDOWN		(1 << 18)
#define   UTMIP_XCVR_HSSLEW_MSB(x)		(((x) & 0x7f) << 25)

#define UTMIP_BIAS_CFG0		0x80c
#define   UTMIP_OTGPD			(1 << 11)
#define   UTMIP_BIASPD			(1 << 10)

#define UTMIP_HSRX_CFG0		0x810
#define   UTMIP_ELASTIC_LIMIT(x)	(((x) & 0x1f) << 10)
#define   UTMIP_IDLE_WAIT(x)		(((x) & 0x1f) << 15)

#define UTMIP_HSRX_CFG1		0x814
#define   UTMIP_HS_SYNC_START_DLY(x)	(((x) & 0x1f) << 1)

#define UTMIP_TX_CFG0		0x820
#define   UTMIP_FS_PREABMLE_J		(1 << 19)
#define   UTMIP_HS_DISCON_DISABLE	(1 << 8)

#define UTMIP_MISC_CFG0		0x824
#define   UTMIP_DPDM_OBSERVE		(1 << 26)
#define   UTMIP_DPDM_OBSERVE_SEL(x)	(((x) & 0xf) << 27)
#define   UTMIP_DPDM_OBSERVE_SEL_FS_J	UTMIP_DPDM_OBSERVE_SEL(0xf)
#define   UTMIP_DPDM_OBSERVE_SEL_FS_K	UTMIP_DPDM_OBSERVE_SEL(0xe)
#define   UTMIP_DPDM_OBSERVE_SEL_FS_SE1 UTMIP_DPDM_OBSERVE_SEL(0xd)
#define   UTMIP_DPDM_OBSERVE_SEL_FS_SE0 UTMIP_DPDM_OBSERVE_SEL(0xc)
#define   UTMIP_SUSPEND_EXIT_ON_EDGE	(1 << 22)

#define UTMIP_MISC_CFG1		0x828
#define   UTMIP_PLL_ACTIVE_DLY_COUNT(x)	(((x) & 0x1f) << 18)
#define   UTMIP_PLLU_STABLE_COUNT(x)	(((x) & 0xfff) << 6)

#define UTMIP_DEBOUNCE_CFG0	0x82c
#define   UTMIP_BIAS_DEBOUNCE_A(x)	(((x) & 0xffff) << 0)

#define UTMIP_BAT_CHRG_CFG0	0x830
#define   UTMIP_PD_CHRG			(1 << 0)

#define UTMIP_SPARE_CFG0	0x834
#define   FUSE_SETUP_SEL		(1 << 3)

#define UTMIP_XCVR_CFG1		0x838
#define   UTMIP_FORCE_PDDISC_POWERDOWN	(1 << 0)
#define   UTMIP_FORCE_PDCHRP_POWERDOWN	(1 << 2)
#define   UTMIP_FORCE_PDDR_POWERDOWN	(1 << 4)
#define   UTMIP_XCVR_TERM_RANGE_ADJ(x)	(((x) & 0xf) << 18)

#define UTMIP_BIAS_CFG1		0x83c
#define   UTMIP_BIAS_PDTRK_COUNT(x)	(((x) & 0x1f) << 3)



static inline int phy_is_hsic(struct hisik3_usb_phy *phy)
{
	//return (phy->instance == 1);
	
	/*******************************
	思路:
	计划在这一部分设计的根据phy->instance来区分控制器或者对接的PHY;
	*******************************/
	/*
	bit0: NANO PHY
	bit1: HSIC PHY
	*/

	/*******************************
	if use picophy, must set the configs:
		CONFIG_USB_UAS=y
		CONFIG_USB_STORAGE=y
		CONFIG_USB_DEBUG=y
		CONFIG_USB_USBNET=y
		CONFIG_SCSI_WAIT_SCAN=y
		CONFIG_SCSI_MULTI_LUN=y
		CONFIG_BLK_DEV_SD=y
		CONFIG_SCSI=y
		CONFIG_SCSI_DMA=y

	and power on the VBUS:
		iowrite32(0x2,IO_ADDRESS(0xfc803000+0x4))
	*******************************/
	return IS_HAVE_HSICPHY; //Select hsic phy defaultly.
}


static void nano_phy_power_on(struct hisik3_usb_phy *phy)
{
	u32 uregv = 0;
	int ret;
	printk(KERN_INFO "%s.\n", __func__);

	//Enable nano phy clk
	ret = clk_enable(phy->nano_phy_clk);
	if (ret) {
		printk(KERN_ERR "%s.clk_enable nano_phy_clk failed\n", __func__);
	}

	//free clock sel,use nano phy's 60Mhz clock
	writel(USB2HST_FREE_SEL<<16, IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL1));
	
	uregv = readl(IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL15));
	uregv &= ~NANOPHY_COMMON_N;	// nanophy commonon_n
	writel(uregv, IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL15));

	uregv = readl(IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL14));
	uregv &= ~NANOPHY_SIDDQ;	// nanophy exit siddq
	writel(uregv, IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL14));	

	return;
}

static void pico_phy_power_on(struct hisik3_usb_phy *phy)
{
	u32 uregv = 0;
	int ret;
	printk(KERN_INFO "%s.\n", __func__);

	/*Enable pico phy clk*/
	ret = clk_enable(phy->nano_phy_clk);
	if (ret) {
		printk(KERN_ERR "%s.clk_enable nano_phy_clk failed\n", __func__);
	}

	/*free clock sel,use nano phy's 60Mhz clock*/
	writel(USB2HST_FREE_SEL<<16, IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL1));

	/*picophy commonon_n and TXVREFTUNE0 is 4'b1000*/
	uregv = readl(IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL15));
	uregv &= ~PICOPHY_COMMON_N;
	uregv &= ~(TXVREFTUNE_2);
	uregv |= (TXVREFTUNE_1);
	writel(uregv, IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL15));

	/*picophy exit siddq*/
	uregv = readl(IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL14));
	uregv &= ~PICOPHY_SIDDQ;
	writel(uregv, IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL14));

	/*TXPREEMPAMPTUNE0 is 2'b11*/
	uregv = readl(IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL21));
	uregv |= TXPREEMPAMPTUNE;
	writel(uregv, IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL21));

	return;
}

static void nano_phy_power_off(struct hisik3_usb_phy *phy)
{
	u32 uregv = 0;
	printk(KERN_INFO "%s.\n", __func__);

	uregv = readl(IO_ADDRESS(REG_BASE_PCTRL) + PERI_CTRL14); 
	uregv |= NANOPHY_SIDDQ;   // nanophy enter siddq
	writel(uregv, IO_ADDRESS(REG_BASE_PCTRL) + PERI_CTRL14);

	//disable nano phy cnf clk.
	clk_disable(phy->nano_phy_clk);
	
	return;
}


static void pico_phy_power_off(struct hisik3_usb_phy *phy)
{
	u32 uregv = 0;
	printk(KERN_INFO "%s.\n", __func__);

	/*picophy enter siddq*/
	uregv = readl(IO_ADDRESS(REG_BASE_PCTRL) + PERI_CTRL14);
	uregv |= PICOPHY_SIDDQ;
	writel(uregv, IO_ADDRESS(REG_BASE_PCTRL) + PERI_CTRL14);

	/*disable phy cnf clk.*/
	clk_disable(phy->nano_phy_clk);

	return;
}

static void hsic_phy_clk_disable(struct hisik3_usb_phy *phy)
{
	printk(KERN_INFO "%s.\n", __func__);

	/*disable hisc phy cnf clk,12MHz and 480MHz.*/
	clk_disable(phy->hsic_phy_clk);
	clk_disable(phy->hsic_phy_clk_480);
}

static void hsic_phy_clk_enable(struct hisik3_usb_phy *phy)
{
	int ret = 0;

	printk(KERN_INFO "%s.\n", __func__);

	/*Enable hisc phy cnf clk,12MHz and 480MHz.*/
	ret = clk_enable(phy->hsic_phy_clk);
	if (ret) {
		printk(KERN_ERR "%s.clk_enable hsic_phy_clk failed\n", __func__);
	}

	ret = clk_enable(phy->hsic_phy_clk_480);
	if (ret) {
		printk(KERN_ERR "%s.clk_enable hsic_phy_clk_480 failed\n", __func__);
	}
}

static void hsic_phy_preresume(struct hisik3_usb_phy *phy)
{
	unsigned long val;
	void __iomem *base = phy->regs;

	val = readl(base + UTMIP_TX_CFG0);
	val |= UTMIP_HS_DISCON_DISABLE;
	writel(val, base + UTMIP_TX_CFG0);
}

static void hsic_phy_postresume(struct hisik3_usb_phy *phy)
{
	unsigned long val;
	void __iomem *base = phy->regs;

	val = readl(base + UTMIP_TX_CFG0);
	val &= ~UTMIP_HS_DISCON_DISABLE;
	writel(val, base + UTMIP_TX_CFG0);
}

static void hsic_phy_restore_start(struct hisik3_usb_phy *phy,
				   enum hisik3_usb_phy_port_speed port_speed)
{
	unsigned long val;
	void __iomem *base = phy->regs;

	val = readl(base + UTMIP_MISC_CFG0);
	val &= ~UTMIP_DPDM_OBSERVE_SEL(~0);
	if (port_speed == HISIK3_USB_PHY_PORT_SPEED_LOW)
		val |= UTMIP_DPDM_OBSERVE_SEL_FS_K;
	else
		val |= UTMIP_DPDM_OBSERVE_SEL_FS_J;
	writel(val, base + UTMIP_MISC_CFG0);
	udelay(1);

	val = readl(base + UTMIP_MISC_CFG0);
	val |= UTMIP_DPDM_OBSERVE;
	writel(val, base + UTMIP_MISC_CFG0);
	udelay(10);
}

static void hsic_phy_restore_end(struct hisik3_usb_phy *phy)
{
	unsigned long val;
	void __iomem *base = phy->regs;

	val = readl(base + UTMIP_MISC_CFG0);
	val &= ~UTMIP_DPDM_OBSERVE;
	writel(val, base + UTMIP_MISC_CFG0);
	udelay(10);
}

static void hsic_phy_power_on(struct hisik3_usb_phy *phy)
{
	u32 uregv = 0;
	int ret;

	printk(KERN_INFO "%s.\n", __func__);

	//free clock sel,use hsic phy's 60Mhz clock
	writel(USB2HST_FREE_SEL<<16|USB2HST_FREE_SEL, IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL1));
	
	uregv = readl(IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL15));
	uregv &= ~HSICPHY_SIDDQ;	// hsicphy exit siddq
	uregv &= ~HSICPHY_COMMON_N;	// hsicphy commonon_n
	/*hsicphy txsrtune*/
	uregv |= HSICPHY_TXSRTUNE;
	writel(uregv, IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL15));

	//enable hsic mode in port1 (peri ctrl)
	writel(USB2H_HSIC_EN_MASK |USB2H_HSIC_EN_PORT1, IO_ADDRESS(REG_BASE_PCTRL+PERI_CTRL3));
	
	return;
}

static void hsic_phy_power_off(struct hisik3_usb_phy *phy)
{
	u32 uregv = 0;
	printk(KERN_INFO "%s.\n", __func__);

	uregv = readl(IO_ADDRESS(REG_BASE_PCTRL) + PERI_CTRL15); 
	uregv |= HSICPHY_SIDDQ; // hsicphy enter siddq
	writel(uregv, IO_ADDRESS(REG_BASE_PCTRL) + PERI_CTRL15);	
	
	return;
}

struct hisik3_usb_phy *hisik3_usb_phy_open(int instance, void __iomem *regs,
			void *config, enum hisik3_usb_phy_mode phy_mode)
{
	struct hisik3_usb_phy *phy;
	int err;
	printk(KERN_INFO "%s.\n", __func__);

	phy = kmalloc(sizeof(struct hisik3_usb_phy), GFP_KERNEL);
	if (!phy)
		return ERR_PTR(-ENOMEM);

	phy->instance = instance;
	phy->regs = regs;
	phy->config = config;
	phy->mode = phy_mode;

	//select hsic phy
	phy->hsic_phy_clk_480 = clk_get(NULL, "clk_usbhsicphy480");
	if (IS_ERR(phy->hsic_phy_clk_480)) {
		err = PTR_ERR(phy->hsic_phy_clk_480);
		printk(KERN_ERR "Can't get hsic phy 480MHz clock, err=%#x\n", err);
	}

	phy->hsic_phy_clk = clk_get(NULL, "clk_usbhsicphy");
	if (IS_ERR(phy->hsic_phy_clk)) {
		err = PTR_ERR(phy->hsic_phy_clk);
		printk(KERN_ERR "Can't get hsic phy 12MHz clock, err=%#x\n", err);
	}

	if (phy_is_hsic(phy) & IS_HAVE_HSICPHY)
	{
		err = clk_enable(phy->hsic_phy_clk_480);
		if (err) {
			printk(KERN_ERR "%s.clk_enable hsic_phy_clk_480 failed\n", __func__);
		}
		err = clk_enable(phy->hsic_phy_clk);
		if (err) {
			printk(KERN_ERR "%s.clk_enable hsic_phy_clk failed\n", __func__);
		}
	}

	phy->nano_phy_clk = clk_get(NULL, "clk_usbnanophy");
	if (IS_ERR(phy->nano_phy_clk)) {
		err = PTR_ERR(phy->nano_phy_clk);
		printk(KERN_ERR "Can't get nano phy clock, err=%#x\n", err);
	}

	if (phy_is_hsic(phy) & (IS_HAVE_NANOPHY | IS_HAVE_PICOPHY))
	{
		err = clk_enable(phy->nano_phy_clk);
		if (err) {
			printk(KERN_ERR "%s.clk_enable nano_phy_clk failed\n", __func__);
		}
	}

	return phy;
}

void hisik3_usb_phy_power_on(struct hisik3_ehci_hcd *hiusb_ehci)
{
	/*******************************
	思路:
	计划在这一部分设计的是PHY的上电配置.
	可能还有对控制器的配置.
	*******************************/
	int ret;

	struct hisik3_usb_phy *phy = hiusb_ehci->phy;
	printk(KERN_INFO "%s.\n", __func__);

	if (phy_is_hsic(phy) & IS_HAVE_HSICPHY)
		hsic_phy_power_on(phy);
	if (phy_is_hsic(phy) & IS_HAVE_NANOPHY)
		nano_phy_power_on(phy);	
	if (phy_is_hsic(phy) & IS_HAVE_PICOPHY)
		pico_phy_power_on(phy);

	if (phy_is_hsic(phy) & IS_HAVE_HSICPHY)
	{
		writel(IP_RST_HSICPHY, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTDIS1));// hsicphy port reset
		writel(IP_HSICPHY_POR, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTDIS3));// hsicphy por reset
		writel(IP_RST_USB2H_UTMI1, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTDIS3));//host port1
	}	
	if (phy_is_hsic(phy) & (IS_HAVE_NANOPHY | IS_HAVE_PICOPHY))
	{
		writel(IP_RST_NANOPHY, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTDIS1));// nanophy port reset
		writel(IP_NANOPHY_POR, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTDIS3));// nanophy por reset	
		writel(IP_RST_USB2H_UTMI0, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTDIS3));//host port0
	}
	udelay(100);
	writel(IP_RST_USB2H_PHY, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTDIS3));//host phy rstn
	udelay(100);
	writel(IP_RST_USB2HOST, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTDIS3));//host hresetn
	udelay(100);

	//confirure enable hsic, synopsys EHCI INSNREG08
	if (phy_is_hsic(phy) & IS_HAVE_HSICPHY)
		writel(2, IO_ADDRESS(REG_BASE_USB2EHCI+0xB0)); 

	//redo the reset of host phy rstn
	writel(IP_RST_USB2H_PHY, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTEN3));//host phy rstn
	writel(IP_RST_USB2H_PHY, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTDIS3));//host phy rstn
	
}

void hisik3_usb_phy_power_off(struct hisik3_ehci_hcd *hiusb_ehci)
{
	struct hisik3_usb_phy *phy = hiusb_ehci->phy;

	printk(KERN_INFO "%s.\n", __func__);

	writel(IP_RST_NANOPHY, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTEN1));// nanophy port reset
	writel(IP_NANOPHY_POR, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTEN3));// nanophy por reset

	writel(IP_RST_HSICPHY, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTEN1));// hsicphy port reset
	writel(IP_HSICPHY_POR, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTEN3));// hsicphy por reset

	writel(IP_RST_USB2HOST, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTEN3));//host hresetn
	writel(IP_RST_USB2H_PHY, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTEN3));//host phy rstn
	writel(IP_RST_USB2H_UTMI0, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTEN3));//host port0
	writel(IP_RST_USB2H_UTMI1, IO_ADDRESS(REG_BASE_SCTRL+SCPERRSTEN3));//host port1

	if (phy_is_hsic(phy) & IS_HAVE_HSICPHY)
		hsic_phy_power_off(phy);
	if (phy_is_hsic(phy) & IS_HAVE_NANOPHY)
		nano_phy_power_off(phy);
	if (phy_is_hsic(phy) & IS_HAVE_PICOPHY)
		pico_phy_power_off(phy);
	return;
}

void hisik3_usb_phy_preresume(struct hisik3_usb_phy *phy)
{
	printk(KERN_INFO "%s.\n", __func__);

	if (phy_is_hsic(phy) & IS_HAVE_HSICPHY)
		hsic_phy_preresume(phy);
}

void hisik3_usb_phy_postresume(struct hisik3_usb_phy *phy)
{
	printk(KERN_INFO "%s.\n", __func__);
	if (phy_is_hsic(phy) & IS_HAVE_HSICPHY)
		hsic_phy_postresume(phy);
}

void hisik3_ehci_phy_restore_start(struct hisik3_usb_phy *phy,
				 enum hisik3_usb_phy_port_speed port_speed)
{
	/* Force the phy to keep data lines in suspend state */
	/*******************************
	思路:
	计划在这一部分设计的是
	配置将物理层的数据线保持在suspend状态
	注意区分低速全速的不同.
	*******************************/

	if (phy_is_hsic(phy) & IS_HAVE_HSICPHY)
		hsic_phy_restore_start(phy, port_speed);
}

void hisik3_ehci_phy_restore_end(struct hisik3_usb_phy *phy)
{
	/* Cancel the phy to keep data lines in suspend state */
	/*******************************
	思路:
	计划在这一部分设计的是
	配置取消将物理层的数据线保持在suspend状态
	注意区分低速全速的不同.
	*******************************/
	if (phy_is_hsic(phy) & IS_HAVE_HSICPHY)
		hsic_phy_restore_end(phy);
}

void hisik3_usb_phy_clk_disable(struct hisik3_usb_phy *phy)
{
	if (phy_is_hsic(phy) & IS_HAVE_HSICPHY)
		hsic_phy_clk_disable(phy);
}

void hisik3_usb_phy_clk_enable(struct hisik3_usb_phy *phy)
{
	if (phy_is_hsic(phy) & IS_HAVE_HSICPHY)
		hsic_phy_clk_enable(phy);
}

void hisik3_usb_phy_close(struct hisik3_usb_phy *phy)
{
	printk(KERN_INFO "%s.\n", __func__);

	clk_disable(phy->hsic_phy_clk_480);
	clk_put(phy->hsic_phy_clk_480);
	clk_disable(phy->hsic_phy_clk);
	clk_put(phy->hsic_phy_clk);
	clk_disable(phy->nano_phy_clk);
	clk_put(phy->nano_phy_clk);
	
	kfree(phy);
}
