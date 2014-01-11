#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/semaphore.h>
#include <linux/wakelock.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <mach/irqs.h>
#include <mach/lm.h>
#include <mach/platform.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/usb/hiusb_android.h>
#include <hsad/config_interface.h>
#include <linux/mhl/mhl.h>
#include <linux/mux.h>

#include "dwc_otg_pcd.h"
#include "dwc_otg_driver.h"
#include "dwc_otg_cil.h"
#include "dwc_otg_hcd.h"
#include "dwc_otg_hcd_if.h"
#include "dwc_otg_core_if.h"

#define DUMP_REALTED_REGISTER 0


#define hiusblog(format, arg...)    \
	do {                 \
		printk(KERN_INFO "[hiusb]"format, ##arg); \
	} while (0)


enum usb_cable_status {
	USB_CABLE_CONNECTED = 0,
	USB_CABLE_DISCONNECTED,
};


enum hiusb_status {
	HIUSB_OFF = 0,
	HIUSB_DEVICE,
	HIUSB_HOST,
	HIUSB_SUSPEND
};

enum event_type {
	CHARGER_OUT_INTR = 0,
	CHARGER_IN_INTR,
	ID_RISE_EVENT,
	ID_FALL_EVENT
};

struct hiusb_info {
	enum hiusb_status hiusb_status;
	unsigned hiusb_phy_param;
	int is_allow_connect;
	int gadget_init;
	int usb_in_irq_installed;
	int usb_out_irq_installed;
	int dvc_clk_on;
	int pico_clk_on;
	int charger_type;

	int is_dvc_regu_on;
	int is_phy_regu_on;

	struct clk *dvc_clk;
	struct clk *pico_phy_clk;
	struct regulator    *dvc_regu;
	struct regulator    *phy_regu;
	struct lm_device    *hiusb_lm_dev;
	struct semaphore    hiusb_lock;

	struct wake_lock    hiusb_wake_lock;
	struct delayed_work k3v2_otg_intr_work;
	struct atomic_notifier_head charger_type_notifier_head;
	struct iomux_block *usb_block;
	struct block_config *usb_config;

	unsigned int intr_flag;
	unsigned int intr_jiffies;
	spinlock_t intr_flag_lock;
};

static struct hiusb_info *hiusb_info_p;

void hiusb_switch_to_host_test_package(void)
{
	hprt0_data_t reg_value;
	dwc_otg_device_t *otg_dev = lm_get_drvdata(hiusb_info_p->hiusb_lm_dev);

	printk("[%s]+\n", __func__);
	reg_value.d32 = dwc_otg_get_hprt0(otg_dev->core_if);
	printk("hprt value: 0x%x\n", reg_value.d32);
	reg_value.b.prttstctl = 4;
	dwc_otg_set_hprt0(otg_dev->core_if, reg_value.d32);
	reg_value.d32 = dwc_otg_get_hprt0(otg_dev->core_if);
	printk("new hprt value: 0x%x\n", reg_value.d32);
	printk("[%s]-\n", __func__);
}

void hiusb_switch_to_device_test_package(void)
{
	dctl_data_t reg_value;
	dwc_otg_device_t *otg_dev = lm_get_drvdata(hiusb_info_p->hiusb_lm_dev);

	printk("[%s]+\n", __func__);
	reg_value.d32 = DWC_READ_REG32(&otg_dev->core_if->dev_if->dev_global_regs->dctl);
	printk("dctl value: 0x%x\n", reg_value.d32);
	reg_value.b.tstctl = 4;
	DWC_WRITE_REG32(&otg_dev->core_if->dev_if->dev_global_regs->dctl, reg_value.d32);
	reg_value.d32 = DWC_READ_REG32(&otg_dev->core_if->dev_if->dev_global_regs->dctl);
	printk("dctl value: 0x%x\n", reg_value.d32);
	printk("[%s]-\n", __func__);
}

unsigned int hiusb_read_phy_param(void)
{
	unsigned int reg_value;
	reg_value = readl(PERI_CTRL17);
	return reg_value;
}

unsigned int hiusb_set_phy_param(unsigned int value)
{
	hiusb_info_p->hiusb_phy_param = value;
	return hiusb_info_p->hiusb_phy_param;
}

unsigned int hiusb_get_phy_param(void)
{
	return hiusb_info_p->hiusb_phy_param;
}

int get_charger_name(void)
{
	int ret = -1;

	if (NULL == hiusb_info_p) {
		dev_err(&hiusb_info_p->hiusb_lm_dev->dev, "kzalloc failed!\n");
		return ret;
	}

	ret = down_interruptible(&hiusb_info_p->hiusb_lock);
	if (ret) {
		dev_err(&hiusb_info_p->hiusb_lm_dev->dev, "lock sem failed!\n");
		return ret;
	}

	ret = hiusb_info_p->charger_type;

	up(&hiusb_info_p->hiusb_lock);

	return ret;
}

int hiusb_charger_registe_notifier(struct notifier_block *nb)
{
	int ret = -1;
	if (hiusb_info_p)
		ret = atomic_notifier_chain_register(
				&hiusb_info_p->charger_type_notifier_head, nb);
	else
		printk(KERN_ERR "Notifier head not yet init.\n");
	return ret;
}
EXPORT_SYMBOL_GPL(hiusb_charger_registe_notifier);

int hiusb_charger_unregiste_notifier(struct notifier_block *nb)
{
	int ret = -1;
	if (hiusb_info_p)
		ret = atomic_notifier_chain_unregister(
				&hiusb_info_p->charger_type_notifier_head, nb);
	else
		printk(KERN_ERR "Notifier head not yet init.\n");
	return ret;
}
EXPORT_SYMBOL_GPL(hiusb_charger_unregiste_notifier);

static void notify_charger_type(unsigned long val)
{
	atomic_notifier_call_chain(&hiusb_info_p->charger_type_notifier_head,
				hiusb_info_p->charger_type, hiusb_info_p);
	hiusblog("[%s]Notify charger type: %d\n", __func__, hiusb_info_p->charger_type);
}

/**
 * detect_charger_type() - check charger type standard or non-standard.
 * return 0x00 means non-standard, return 0x01 means standard.
 */
static unsigned int detect_charger_type(void)
{
	unsigned int reg_value = 0;
	unsigned int type = 0;
	unsigned long jiffies_expire;
	int i = 0;

	/* enable BC */
	writel(0x1, (BC_EN_ADDR));

	/* picophy suspend */
	reg_value = readl(BC_CMD_ADDR);
	reg_value &= ~(1 << 7);
	writel(reg_value, (BC_CMD_ADDR));

	/* T = (200/255) * Tsensor_Value - 40 */
	if (readl(TSENSOR_ADDR) > 153) {
		hiusblog("[%s]T > 80\n", __func__);
		msleep(900);
	} else {
		/* enable DCD */
		writel((readl(BC_CMD_ADDR) | (1 << 3)), (BC_CMD_ADDR));

		jiffies_expire = jiffies + msecs_to_jiffies(900);
		msleep(50);

		while (i < 10) {
			reg_value = readl(BC_ST_ADDR);
			if ((reg_value & (1 << 2)) == 0) {
				i++;
			} else {
				hiusblog("D+ is not connected\n");
				i = 0;
			}

			msleep(1);

			if (time_after(jiffies, jiffies_expire)) {
				hiusblog("wait for D+ connection timeout\n");
				break;
			}
		}

		/* disable DCD */
		writel((readl(BC_CMD_ADDR) & ~(1 << 3)), (BC_CMD_ADDR));
	}

	/* enable vdect */
	reg_value = readl(BC_CMD_ADDR);
	reg_value &= ~(0x7);
	reg_value |= 0x6;
	writel(reg_value, (BC_CMD_ADDR));

	msleep(10);

	/* we can detect sdp or cdp dcp */
	reg_value = readl(BC_ST_ADDR);

	if ((reg_value & 0x2) == 0) {
		type = CHARGER_TYPE_USB;
	} else {
		/* cdp or dcp */
		type = CHARGER_TYPE_STANDARD;
	}

	/* disable vdect */
	writel((readl(BC_CMD_ADDR) & ~(0x6)), (BC_CMD_ADDR));

	/* picophy suspend = 1, nomal mode */
	reg_value = readl(BC_CMD_ADDR);
	reg_value |= 1<<7;
	writel(reg_value, (BC_CMD_ADDR));

	/* disable BC */
	reg_value = 0x0;
	writel(0x0, (BC_EN_ADDR));

	/* distinguish CDP from DCP */
	if (CHARGER_TYPE_STANDARD == type) {
		dwc_otg_device_t *otg_dev = lm_get_drvdata(hiusb_info_p->hiusb_lm_dev);

		dwc_otg_pcd_connect_us(otg_dev->pcd, 50);
		udelay(100);
		reg_value = readl(BC_ST_ADDR);
		hiusblog("detect CDP, BC_ST: 0x%x\n", reg_value);
		if (((reg_value >> 10) & 3) == 3)
			type = CHARGER_TYPE_STANDARD;
		else
			type = CHARGER_TYPE_BC_USB;
		dwc_otg_pcd_disconnect_us(otg_dev->pcd, 50);
	}

	hiusblog("detect_charger_type, type: 0x%x\n", type);

	return type;
}


static int enable_dvc_clk(struct hiusb_info *hi)
{
	int ret = 0;
	if (0 == (hi->dvc_clk_on)) {
		ret = clk_enable(hi->dvc_clk);
		if (!ret)
			hi->dvc_clk_on = 1;
	}
	return ret;
}

static void disable_dvc_clk(struct hiusb_info *hi)
{
	if (hi->dvc_clk_on) {
		clk_disable(hi->dvc_clk);
		hi->dvc_clk_on = 0;
	}
}

static int enable_pico_clk(struct hiusb_info *hi)
{
	int ret = 0;
	if (0 == (hi->pico_clk_on)) {
		ret = clk_enable(hi->pico_phy_clk);
		if (!ret)
			hi->pico_clk_on = 1;
	}
	return ret;
}

static void disable_pico_clk(struct hiusb_info *hi)
{
	if (hi->pico_clk_on) {
		clk_disable(hi->pico_phy_clk);
		hi->pico_clk_on = 0;
	}
}

static int enable_dvc_regu(void)
{
	int ret = 0;

	if(0 == hiusb_info_p->is_dvc_regu_on) {
		ret = regulator_enable(hiusb_info_p->dvc_regu);
		hiusb_info_p->is_dvc_regu_on = 1;
	}
	if (ret < 0) {
		printk(KERN_ERR "[usbotg]regulator_enable dvc failed!\n");
	}

	return ret;
}

static int enable_phy_regu(void)
{
	int ret = 0;

	if(0 == hiusb_info_p->is_phy_regu_on) {
		ret = regulator_enable(hiusb_info_p->phy_regu);
		hiusb_info_p->is_phy_regu_on = 1;
	}
	if (ret < 0) {
		printk(KERN_ERR "[usbotg]regulator_enable phy failed!\n");
	}

	return ret;
}


static void disable_dvc_regu(void)
{
	if(1 == hiusb_info_p->is_dvc_regu_on) {
		regulator_disable(hiusb_info_p->dvc_regu);
		hiusb_info_p->is_dvc_regu_on = 0;
	}
}

static void disable_phy_regu(void)
{
	if(1 == hiusb_info_p->is_phy_regu_on) {
		regulator_disable(hiusb_info_p->phy_regu);
		hiusb_info_p->is_phy_regu_on = 0;
	}
}

static int setup_dvc_and_phy(void)
{
	unsigned long reg_value;
	int ret = 0;
	int usbphy_tune;

	ret = blockmux_set(hiusb_info_p->usb_block, hiusb_info_p->usb_config, NORMAL);
	if (ret) {
		printk(KERN_ERR "Set VBusdrv failed!\n");
	}

	ret = enable_dvc_regu();
	if (ret) {
		printk(KERN_ERR "[usbotg]open dvc regu failed.\n");
		goto out;
	}

	ret = enable_phy_regu();
	if (ret) {
		printk(KERN_ERR "[usbotg]open phy regu failed.\n");
		goto out;
	}

	udelay(200);

	/* DVC Core reset */
	writel(IP_RST_USB2DVC, (PER_RST_EN_3_ADDR));
	/* DVC PHY reset */
	writel(IP_RST_USB2DVC_PHY, (PER_RST_EN_3_ADDR));

	/* pico phy reset */
	writel(IP_RST_PICOPHY, (PER_RST_EN_1_ADDR));
	/* pico_phy_por */
	writel(IP_PICOPHY_POR, (PER_RST_EN_3_ADDR));
	/* pico clk off */
	disable_pico_clk(hiusb_info_p);

	disable_dvc_clk(hiusb_info_p);

	reg_value = readl((PERI_CTRL16));
	reg_value &= (~(PICOPHY_SIDDQ));
	reg_value |= PERI_CTRL16_PICOPHY_TXPREEMPHASISTUNE;
	reg_value &= ~(0x1 << 9); /* usb1_phy_otgdisable */
	reg_value &= ~(0x3 << 10); /* usb1_phy_vbusvldextsel */
	/* bit[19:17]: usb1_phy_compdistune[2:0], disconnect detect threshold tuning */
	reg_value &= ~(0x7 << 17);
	reg_value |= (0x6 << 17);
	writel(reg_value, (PERI_CTRL16));

	/* set usb1 phy param */
	reg_value = readl(PERI_CTRL17);
	reg_value &= ~0x3F;

	usbphy_tune = get_usbphy_tune();
	switch (usbphy_tune) {
		/* config for Development Board */
	case E_USBPHY_TUNE_PLATFORM:
		reg_value |= 0x23;
		break;
	case E_USBPHY_TUNE_U9510:
		/* USB PHY config for phone */
		reg_value |= 0x03;
		reg_value |= (0x0A << 2);
		reg_value |= (0x01 << 15);
		reg_value &= ~(0x03 << 17);
		reg_value |= (0x01 << 17);
		reg_value &= ~(0x03 << 6);
		reg_value |= (0x01 << 6);
		break;
	default:
		pr_err("invalid usbphy_tune.\n");
		break;
	}

	if (hiusb_get_phy_param()) {
		reg_value = hiusb_get_phy_param();
		printk(KERN_INFO "new phy param: 0x%08x\n", reg_value);
	}

	hiusblog("phy param: 0x%08x\n", reg_value);

	writel(reg_value, PERI_CTRL17);

	/* id and vbus setting */
	reg_value = readl((PERI_CTRL21));
	/* 2:1 otg_id_sel
	 * 5 otg_drvvbus_sel
	 * 6 otg_drvvbus
	 * 8 otg_idpullupsel
	 * 9 otg_idpullup
	 * 10 otg_acaenb_sel
	 * 11 otg_acaenb
	 * 14 otg_vbusvalid_sel */
	reg_value &= ~((0x3 << 1) | (0x3 << 8) | (0x3 << 10));
	reg_value |= ((0x1 << 1) | (0x3 << 8) | (0x1 << 10));
	writel(reg_value, PERI_CTRL21);

	ret = enable_pico_clk(hiusb_info_p);
	if (ret) {
		dev_info(&hiusb_info_p->hiusb_lm_dev->dev,
			"Enable pico clk failed!\n");
		goto out;
	}

	udelay(10);

	writel(IP_RST_PICOPHY, (PER_RST_DIS_1_ADDR));
	writel(IP_PICOPHY_POR, (PER_RST_DIS_3_ADDR));

	udelay(1000);

	ret =  enable_dvc_clk(hiusb_info_p);
	if (ret) {
		dev_info(&hiusb_info_p->hiusb_lm_dev->dev,
			"Enable dvc_clk failed!\n");
		goto out;
	}

	/* DVC PHY undo reset */
	writel(IP_RST_USB2DVC_PHY, (PER_RST_DIS_3_ADDR));
	udelay(1);
	/* DVC undo reset */
	writel(IP_RST_USB2DVC, (PER_RST_DIS_3_ADDR));

out:
	return ret;
}


static void clearup_dvc_and_phy(void)
{
	int ret = 0;
	unsigned long reg_value;

	/* Reset USB DVC */
	writel(IP_RST_USB2DVC, (PER_RST_EN_3_ADDR));
	writel(IP_RST_USB2DVC_PHY, (PER_RST_EN_3_ADDR));

	disable_dvc_clk(hiusb_info_p);

	/* Reset USB PHY... */
	writel(IP_RST_PICOPHY, (PER_RST_EN_1_ADDR));
	writel(IP_PICOPHY_POR, (PER_RST_EN_3_ADDR));
	disable_pico_clk(hiusb_info_p);

	/* PICO PHY enter siddq */
	reg_value = readl((PERI_CTRL16));
	reg_value |= (PICOPHY_SIDDQ);
	writel(reg_value, (PERI_CTRL16));

	disable_phy_regu();
	disable_dvc_regu();
	hiusblog("phy regu: %s\n",
		(hiusb_info_p->is_phy_regu_on ? "on" : "off"));
	hiusblog("dvc regu: %s\n",
		(hiusb_info_p->is_dvc_regu_on ? "on" : "off"));

	ret = blockmux_set(hiusb_info_p->usb_block, hiusb_info_p->usb_config, LOWPOWER);

	if (ret)
		hiusblog("the return value of the blockmux_set is error\n");

}


#if DUMP_REALTED_REGISTER
static void dump_realted_register(void)
{
	unsigned int reg_value;
	printk(KERN_ERR "------------------------\n");
	reg_value = readl((PER_CLK_STATUS_1_ADDR));
	printk(KERN_ERR "  clk status 1: 0x%x.\n", reg_value);

	printk(KERN_ERR "  nano clk status 1: %d.\n"
			"  pico clk status 1: %d.\n"
			"  hsic clk status 1: %d.\n"
			"  480 clk status 1: %d.\n",
			!!(reg_value & (GT_CLK_USBNANOPHY)),
			!!(reg_value & (GT_CLK_USBPICOPHY)),
			!!(reg_value & (GT_CLK_USBHSICPHY)),
			!!(reg_value & (GT_CLK_USBHSICPHY480)));

	reg_value = readl((PER_CLK_STATUS_3_ADDR));
	printk(KERN_ERR "  clk status 1: 0x%x.\n", reg_value);

	printk(KERN_ERR "  usb2dvc clk: %d.\n"
			"  usb2hst clk: %d.\n",
			!!(reg_value & (GT_CLK_USB2DVC)),
			!!(reg_value & (GT_CLK_USB2HST)));

	reg_value = readl((PERI_CTRL16));
	printk(KERN_ERR "  pctl16: 0x%x.\n", reg_value);

	reg_value = readl((PERI_CTRL14));
	printk(KERN_ERR "  pctl14: 0x%x.\n", reg_value);

	printk(KERN_ERR "------------------------\n");
}
#endif

static enum usb_cable_status hiusb_detect_usb_cable(void)
{
	unsigned int reg_value;

	reg_value = ioread32(PMU_STATUS0_ADDR);
	if (reg_value & (VBUS4P5_D10 | VBUS_COMP_VBAT_D20)) {
		return USB_CABLE_CONNECTED;
	}
	return USB_CABLE_DISCONNECTED;
}

static void clear_charger_interrupt(void)
{
	unsigned reg_value;

	/* vbus_comp_vbat_f	4	W1C	0x0
	 * vbus_comp_vbat_r	3	W1C	0x0 */
	reg_value = (1 << 3) | (1 << 4);
	iowrite32(reg_value, PMU_IRQ2_ADDR);
}

static void k3v2_otg_setup(void)
{
	//wake_lock(&(hiusb_info_p->hiusb_wake_lock));
	//hiusblog("hiusb wake lock\n");
	setup_dvc_and_phy();
	hiusb_info_p->hiusb_status = HIUSB_DEVICE;
	return;
}


static void k3v2_otg_clearup(void)
{
	dwc_otg_device_t *otg_dev = lm_get_drvdata(hiusb_info_p->hiusb_lm_dev);

	if (HIUSB_OFF == hiusb_info_p->hiusb_status) {
		hiusblog("[%s]Already power off.\n",__func__);
		return;
	}

	/* Disable the dvc interrupt. */
	dwc_otg_disable_global_interrupts(otg_dev->core_if);

	hiusb_info_p->hiusb_status = HIUSB_OFF;
	hiusb_info_p->is_allow_connect = 0;

	/* close clock ldo */
	clearup_dvc_and_phy();

	//wake_unlock(&(hiusb_info_p->hiusb_wake_lock));
	//hiusblog("hiusb wake unlock\n");
	return ;
}


static void k3v2_otg_interrupt_work(struct work_struct *work)
{
	unsigned long flags;
	unsigned long intr_jiffies;
	enum event_type intr_flag;
	enum usb_cable_status usb_cable_status;
	dwc_otg_device_t *otg_dev = lm_get_drvdata(hiusb_info_p->hiusb_lm_dev);

	hiusblog("[%s]+\n", __func__);

	spin_lock_irqsave(&(hiusb_info_p->intr_flag_lock), flags);
	intr_flag = hiusb_info_p->intr_flag;
	intr_jiffies = hiusb_info_p->intr_jiffies;
	spin_unlock_irqrestore(&(hiusb_info_p->intr_flag_lock), flags);

	if (down_interruptible(&hiusb_info_p->hiusb_lock)) {
		hiusblog("lock sem failed!\n");
		return;
	}

	hiusblog("[%s]intr_flag:%d\n", __func__, intr_flag);

	usb_cable_status = hiusb_detect_usb_cable();

	switch (intr_flag) {
	case CHARGER_IN_INTR:
		if (USB_CABLE_DISCONNECTED == usb_cable_status) {
			hiusblog("USB cable not connected.\n");
			goto error;
		}

		if (HIUSB_DEVICE == hiusb_info_p->hiusb_status) {
			hiusblog("Already in device mode.\n");
			dwc_otg_pcd_disconnect_us(otg_dev->pcd, 50);

			hiusb_info_p->charger_type = detect_charger_type();
			notify_charger_type(0);

			if (hiusb_info_p->gadget_init)
				dwc_otg_pcd_connect_us(otg_dev->pcd, 50);

		} else if ((HIUSB_OFF == hiusb_info_p->hiusb_status)
		    || (HIUSB_SUSPEND == hiusb_info_p->hiusb_status)) {
			/* USB cable connected, do not alow sleep. */
			//wake_lock(&(hiusb_info_p->hiusb_wake_lock));
			//hiusblog("hiusb wake lock\n");

			/* open ldo clock */
			setup_dvc_and_phy();

			/* Get charger type. */
			hiusb_info_p->charger_type = detect_charger_type();
			if ((hiusb_info_p->charger_type == CHARGER_TYPE_USB)
			    ||(hiusb_info_p->charger_type == CHARGER_TYPE_BC_USB)){
				wake_lock(&(hiusb_info_p->hiusb_wake_lock));
				hiusblog("hiusb wake lock\n");
			}
			notify_charger_type(0);

			/* disable usb core interrupt */
			dwc_otg_disable_global_interrupts(otg_dev->core_if);

			/* reinit usb core */
			hiusblog("dwc_otg_core_init\n");
			dwc_otg_core_init(otg_dev->core_if);
			if (dwc_otg_is_device_mode(otg_dev->core_if)) {
				hiusblog("cil_pcd_start\n");
				cil_pcd_start(otg_dev->core_if);
				otg_dev->core_if->op_state = B_PERIPHERAL;
			}

			/* enable usb core interrupt */
			dwc_otg_enable_global_interrupts(otg_dev->core_if);

			/* do software disconnect and connect for host enum again */
			dwc_otg_pcd_disconnect_us(otg_dev->pcd,50);
			udelay(100);

			if (hiusb_info_p->gadget_init)
				dwc_otg_pcd_connect_us(otg_dev->pcd, 50);

			hiusblog("hiusb_status: OFF -> DEVICE\n");
			hiusb_info_p->hiusb_status = HIUSB_DEVICE;
			hiusb_info_p->is_allow_connect = 1;
		} else if (HIUSB_HOST == hiusb_info_p->hiusb_status) {
			hiusblog("In intr in HOST mode \n");
		}
		break;

	case CHARGER_OUT_INTR:
		if (USB_CABLE_CONNECTED == usb_cable_status) {
			hiusblog("USB cable still connected.\n");
			goto error;
		}

		if (HIUSB_OFF == hiusb_info_p->hiusb_status) {
			hiusblog("Already in OFF mode.\n");
		} else if (HIUSB_DEVICE == hiusb_info_p->hiusb_status) {
			/* For send uevent of disconnect. */
			otg_dev->pcd->fops->disconnect(otg_dev->pcd);

			/* Disable the dvc interrupt. */
			dwc_otg_disable_global_interrupts(otg_dev->core_if);

			/* close clock ldo */
			clearup_dvc_and_phy();

			/* Charger type notify */
			hiusblog("Notifiy: charger removed.\n");
			hiusb_info_p->charger_type = CHARGER_REMOVED;
			notify_charger_type(0);

			/* set poweroff flags */
			hiusb_info_p->is_allow_connect = 0;
			hiusb_info_p->hiusb_status = HIUSB_OFF;

			hiusblog("hiusb_status: DEVICE -> OFF\n");
			if (wake_lock_active(&(hiusb_info_p->hiusb_wake_lock))) {
			    wake_unlock(&(hiusb_info_p->hiusb_wake_lock));
			    hiusblog("hiusb wake unlock\n");
            }
		} else if (HIUSB_HOST == hiusb_info_p->hiusb_status) {
			hiusblog("OUT intr in HOST mode\n");
		} else if (HIUSB_SUSPEND == hiusb_info_p->hiusb_status) {
			hiusblog("OUT intr in SUSPEND mode\n");
		}
		break;
	case ID_FALL_EVENT:
		if ((HIUSB_OFF == hiusb_info_p->hiusb_status)
		    || (HIUSB_SUSPEND == hiusb_info_p->hiusb_status)) {
			unsigned int count = 0;

			dwc_hcd_keep_wake_lock(otg_dev->hcd);
			setup_dvc_and_phy();
			dwc_otg_disable_global_interrupts(otg_dev->core_if);
			while (!dwc_otg_is_host_mode(otg_dev->core_if)) {
				msleep(10);
				if(++count > 100) {
					printk(KERN_ERR "wait host mode timeout\n");
					goto error;
				}
			}
			dwc_otg_core_init(otg_dev->core_if);

			msleep(100);
			if (dwc_otg_is_host_mode(otg_dev->core_if)) {
				otg_dev->core_if->op_state = A_HOST;
				hiusblog("dwc otg is host mode.\n");
			}

			cil_hcd_start(otg_dev->core_if);
			//dwc_otg_enable_global_interrupts(otg_dev->core_if);
			hiusb_info_p->hiusb_status = HIUSB_HOST;
			hiusblog("hiusb_status: OFF -> HOST\n");
            hiusb_info_p->charger_type = USB_EVENT_OTG_ID;
            atomic_notifier_call_chain(
                    &hiusb_info_p->charger_type_notifier_head,
                    USB_EVENT_OTG_ID, hiusb_info_p);
		} else if (HIUSB_DEVICE == hiusb_info_p->hiusb_status) {
			hiusblog("hiusb_status: DEVICE -> HOST\n");
			hiusb_info_p->hiusb_status = HIUSB_HOST;
		} else if (HIUSB_HOST == hiusb_info_p->hiusb_status) {
			hiusblog("already in host mode\n");
		}
		break;
	case ID_RISE_EVENT:
		if (HIUSB_HOST == hiusb_info_p->hiusb_status) {
			dwc_otg_disable_global_interrupts(otg_dev->core_if);
			clearup_dvc_and_phy();
			hiusb_info_p->hiusb_status = HIUSB_OFF;
			dwc_hcd_drop_wake_lock(otg_dev->hcd);
#if defined(MHL_SII9244)||defined(MHL_SII8240)
                    /* reset the mhl */
                    SiiMhlReset();
                    SiiCBusIDSwitcherOpen();
#endif
			hiusblog("hiusb_status: HOST -> OFF\n");
            atomic_notifier_call_chain(
                    &hiusb_info_p->charger_type_notifier_head,
                    CHARGER_REMOVED, hiusb_info_p);
            hiusb_info_p->charger_type = CHARGER_REMOVED;
		} else if (HIUSB_DEVICE == hiusb_info_p->hiusb_status) {
			hiusblog("ID rise in DEVICE mode\n");
			hiusblog("hiusb_status: HOST -> DEVICE\n");
		} else if (HIUSB_OFF == hiusb_info_p->hiusb_status) {
			hiusblog("Already in OFF mode\n");
		} else if (HIUSB_SUSPEND == hiusb_info_p->hiusb_status) {
			hiusblog("ID rise in SUSPEND mode\n");
			hiusb_info_p->hiusb_status = HIUSB_OFF;
		} else {
			hiusblog("illegal status\n");
		}
		break;
	default:
		hiusblog("can't judge interrupt flag\n");
	}

error:

	up(&hiusb_info_p->hiusb_lock);

	hiusblog("[%s]-\n", __func__);
	return;
}

static irqreturn_t hiusb_in_interrupt(int irq, void *_dev)
{
	hiusblog("[hiusb_in_interrupt]+\n");

	spin_lock(&(hiusb_info_p->intr_flag_lock));
	hiusb_info_p->intr_flag = CHARGER_IN_INTR;
	hiusb_info_p->intr_jiffies = jiffies;
	spin_unlock(&(hiusb_info_p->intr_flag_lock));

	schedule_delayed_work(&hiusb_info_p->k3v2_otg_intr_work, 0);

	hiusblog("[hiusb_in_interrupt]-\n");
	return IRQ_HANDLED;
}

static irqreturn_t hiusb_out_interrupt(int irq, void *_dev)
{
	hiusblog("[hiusb_out_interrupt]+\n");

	/* set interrupt flag */
	spin_lock(&(hiusb_info_p->intr_flag_lock));
	hiusb_info_p->intr_flag = CHARGER_OUT_INTR;
	hiusb_info_p->intr_jiffies = jiffies;
	spin_unlock(&(hiusb_info_p->intr_flag_lock));

	schedule_delayed_work(&hiusb_info_p->k3v2_otg_intr_work, 0);

	hiusblog("[hiusb_out_interrupt]-\n");
	return IRQ_HANDLED;
}

int hiusb_probe_phase1(struct lm_device *_dev)
{
	int ret = 0;
	struct hiusb_info   *hi;
	struct clk          *dvc_clk;
	struct clk          *phy_clk;
	struct regulator    *dvc_regu;
	struct regulator    *phy_regu;

	hi = kzalloc(sizeof(struct hiusb_info), GFP_KERNEL);
	if (NULL == hi) {
		dev_err(&_dev->dev, "kzalloc failed!\n");
		return -ENOMEM;
	}

	hiusb_info_p = hi;

	/* init notifier chain. */
	ATOMIC_INIT_NOTIFIER_HEAD(&hi->charger_type_notifier_head);

	wake_lock_init(&hi->hiusb_wake_lock,
			WAKE_LOCK_SUSPEND, "hiusb_wake_lock");
	spin_lock_init(&hi->intr_flag_lock);

	hi->hiusb_lm_dev = _dev;

	hi->usb_in_irq_installed = 0;
	hi->usb_out_irq_installed = 0;
	hi->is_allow_connect = 0;
	hi->gadget_init = 0;
	hi->charger_type = CHARGER_REMOVED;
	hiusb_info_p->hiusb_phy_param = 0;
	sema_init(&hi->hiusb_lock, 0);

	hi->usb_block = iomux_get_block("block_vbusdrv");
	hi->usb_config = iomux_get_blockconfig("block_vbusdrv");

	/* get clocks */
	dvc_clk = clk_get(NULL, "clk_usb2dvc");
	ret = (IS_ERR(dvc_clk));
	if (ret) {
		dev_err(&_dev->dev, "get dvc_clk failed!\n");
		goto get_dvc_clk_fail;
	}
	hi->dvc_clk = dvc_clk;

	phy_clk = clk_get(NULL, "clk_usbpicophy");
	ret = (IS_ERR(phy_clk));
	if (ret) {
		dev_err(&_dev->dev, "get phy_clk failed!\n");
		goto get_phy_clk_fail;
	}
	hi->pico_phy_clk = phy_clk;

	/* clock status record */
	hi->dvc_clk_on = 0;
	hi->pico_clk_on = 0;

	/* get retulator. */
	dvc_regu = regulator_get(&_dev->dev,"usb20-vcc");
	ret = (IS_ERR(dvc_regu));
	if (ret) {
		dev_err(&_dev->dev, "regulator_get dvc failed!\r\n");
		goto usb20_vcc_fail;
	}
	hi->dvc_regu = dvc_regu;
	hi->is_dvc_regu_on = 0;

	phy_regu = regulator_get(&_dev->dev,"usbphy-vcc");
	ret = (IS_ERR(phy_regu));
	if (ret) {
		dev_err(&_dev->dev, "regulator_get phy failed!\r\n");
		goto usbphy_vcc_fail;
	}
	hi->phy_regu = phy_regu;
	hi->is_phy_regu_on = 0;

	/* initialize work. */
	INIT_DELAYED_WORK(&hi->k3v2_otg_intr_work, k3v2_otg_interrupt_work);

	/* During probe,power on USB DVC controller and PHY */
	hi->hiusb_status = HIUSB_OFF;
	k3v2_otg_setup();

	dev_info(&_dev->dev, "hiusb_probe_phase1 done.\n");
	return 0;

usbphy_vcc_fail:
	regulator_put(hi->dvc_regu);
usb20_vcc_fail:

	clk_put(hi->pico_phy_clk);
get_phy_clk_fail:
	clk_put(hi->dvc_clk);
get_dvc_clk_fail:
	wake_lock_destroy(&hi->hiusb_wake_lock);
	kfree(hi);
	hiusb_info_p = NULL;
	return ret;
}


int hiusb_request_irq(struct lm_device *_dev)
{
	int ret;
	int irq;

	irq = IRQ_VBUS_COMP_VBAT_RISING;
	ret = request_irq(irq, hiusb_in_interrupt,
		IRQF_NO_SUSPEND, "hiusb_in_interrupt", _dev);
	if (ret) {
		dev_err(&_dev->dev, "request irq failed.\n");
		return ret;
	}
	hiusb_info_p->usb_in_irq_installed = 1;

	irq = IRQ_VBUS_COMP_VBAT_FALLING;
	ret = request_irq(irq, hiusb_out_interrupt,
		IRQF_NO_SUSPEND, "hiusb_out_interrupt", _dev);
		//0, "hiusb_out_interrupt", _dev);
	if (ret) {
		dev_err(&_dev->dev, "request irq failed.\n");
		irq = IRQ_VBUS_COMP_VBAT_RISING;
		free_irq(irq, _dev);
		return ret;
	}
	hiusb_info_p->usb_out_irq_installed = 1;

	dev_err(&_dev->dev, "hiusb_request_irq done.\n");
	return 0;
}

int hiusb_pullup(struct usb_gadget *gadget, int is_on)
{
	dwc_otg_device_t *hiusb_otg_dev;

	is_on = !!is_on;

	if (gadget == 0) {
		return -ENODEV;
	}

	hiusb_otg_dev = lm_get_drvdata(hiusb_info_p->hiusb_lm_dev);

	if (down_interruptible(&hiusb_info_p->hiusb_lock)) {
		dev_info(&hiusb_info_p->hiusb_lm_dev->dev, "lock sem failed!\n");
		return -ENODEV;
	}

	if (is_on) {
		hiusb_info_p->gadget_init = 1;
	}

	if ((hiusb_info_p->is_allow_connect)
		&& (HIUSB_DEVICE == hiusb_info_p->hiusb_status)) {
		dwc_otg_pcd_pullup(hiusb_otg_dev->pcd,is_on);
		hiusblog("allow_connect and powered on, is_on: %d\n", is_on);
	}

	up(&hiusb_info_p->hiusb_lock);
	return 0;
}

#ifdef CONFIG_INPUT_HW_ATE
uint8_t usb_status_ate;
#endif

int hiusb_probe_phase2(struct lm_device *_dev)
{
	dwc_otg_device_t *otg_dev = lm_get_drvdata(_dev);
	hiusblog("[%s]+\n", __func__);

	/* Clear charger in and charger out interrupt. */
	clear_charger_interrupt();

	if (dwc_otg_is_host_mode(otg_dev->core_if)) {
		hiusblog("now host mode\n");
		hiusb_info_p->hiusb_status = HIUSB_HOST;
		dwc_otg_enable_global_interrupts(otg_dev->core_if);
		/* host mode unlock the wakelock of device mode */
		wake_unlock(&hiusb_info_p->hiusb_wake_lock);
		hiusblog("hiusb wake unlock\n");
	} else if (USB_CABLE_DISCONNECTED == hiusb_detect_usb_cable()) {
		/* usb cable disconnected, so enter power off mode */
		hiusblog("USB cable disconnected.\n");
		k3v2_otg_clearup();
#ifdef CONFIG_INPUT_HW_ATE
		usb_status_ate = USB_CABLE_DISCONNECTED;
#endif
	} else {
		hiusblog("USB cable connected.\n");

		/* detect charger type again */
		hiusb_info_p->charger_type = detect_charger_type();
		notify_charger_type(0);

		/* keep disconnect status untill gadget connect */
		dwc_otg_pcd_disconnect_us(otg_dev->pcd, 50);
		udelay(100);
		dwc_otg_enable_global_interrupts(otg_dev->core_if);
		hiusb_info_p->is_allow_connect = 1;
#ifdef CONFIG_INPUT_HW_ATE
		usb_status_ate = USB_CABLE_CONNECTED;
#endif
	}

	/* Registe charger in and charger out interrupt. */
	hiusb_request_irq(hiusb_info_p->hiusb_lm_dev);

	up(&hiusb_info_p->hiusb_lock);

	dev_info(&_dev->dev, "hiusb_probe_phase2 done.\n");
	hiusblog("[%s]-\n", __func__);
	return 0;
}

int hiusb_remove(struct lm_device *_dev)
{
	struct hiusb_info *hi = hiusb_info_p;

	/* irq. */
	if (hiusb_info_p->usb_in_irq_installed) {
		free_irq(IRQ_VBUS_COMP_VBAT_RISING, _dev);
		hiusb_info_p->usb_in_irq_installed = 0;
	}

	if (hiusb_info_p->usb_out_irq_installed) {
		free_irq(IRQ_VBUS_COMP_VBAT_FALLING, _dev);
		hiusb_info_p->usb_out_irq_installed = 0;
	}

	/* put clk */
	clk_put(hi->dvc_clk);
	clk_put(hi->pico_phy_clk);

	/* close ldo4 and ldo8 */
	regulator_disable(hi->dvc_regu);
	regulator_disable(hi->phy_regu);
	regulator_put(hi->dvc_regu);
	regulator_put(hi->phy_regu);
	wake_lock_destroy(&hi->hiusb_wake_lock);
	kfree(hi);
	hiusb_info_p = NULL;
	dev_info(&_dev->dev, "hiusb_remove done.\n");
	return 0;
}

int hiusb_suspend(struct lm_device *_dev)
{
	dwc_otg_device_t *otg_dev = lm_get_drvdata(hiusb_info_p->hiusb_lm_dev);
	unsigned int reg_value;
	if (down_interruptible(&hiusb_info_p->hiusb_lock)) {
		printk(KERN_INFO "lock sem failed!\n");
		return -EBUSY;
	}
	hiusblog("[%s]+\n", __func__);

	if (HIUSB_HOST == hiusb_info_p->hiusb_status) {
		/* mask charger out interrupt */
		reg_value = ioread32(PMU_IRQ2_MASK_ADDR);
		reg_value |= (1 << 4);
		iowrite32(reg_value, PMU_IRQ2_MASK_ADDR);

		hiusblog("Suspend in host mode\n");
		dwc_otg_set_prtpower(otg_dev->hcd->core_if, 0);
		msleep(10);
		dwc_otg_disable_global_interrupts(otg_dev->core_if);
		clearup_dvc_and_phy();
		hiusb_info_p->hiusb_status = HIUSB_SUSPEND;

	}

	up(&hiusb_info_p->hiusb_lock);

	hiusblog("[%s]-\n", __func__);
	return 0;
}

int hiusb_resume(struct lm_device *_dev)
{
	dwc_otg_device_t *otg_dev = lm_get_drvdata(hiusb_info_p->hiusb_lm_dev);
	unsigned int reg_value;

	if (down_interruptible(&hiusb_info_p->hiusb_lock)) {
		printk(KERN_INFO "lock sem failed!\n");
		return -EBUSY;
	}

	hiusblog("[%s]+\n", __func__);
	if (HIUSB_SUSPEND == hiusb_info_p->hiusb_status) {
		/* unmask charger out interrupt */
		reg_value = ioread32(PMU_IRQ2_MASK_ADDR);
		reg_value &= ~(1 << 4);
		iowrite32(reg_value, PMU_IRQ2_MASK_ADDR);

		/* mhl reset */
        #if defined(MHL_SII9244)//||defined(MHL_SII8240)
		SiiMhlReset();
		SiiCBusIDSwitcherOpen();
        #endif

		hiusblog("system sleep in host mode, so need to resume controller\n");
		setup_dvc_and_phy();
		dwc_otg_disable_global_interrupts(otg_dev->core_if);
		dwc_otg_core_init(otg_dev->core_if);
		if (dwc_otg_is_host_mode(otg_dev->core_if)) {
			otg_dev->core_if->op_state = A_HOST;
			cil_hcd_start(otg_dev->core_if);
			dwc_otg_enable_global_interrupts(otg_dev->core_if);
			hiusb_info_p->hiusb_status = HIUSB_HOST;
		} else {
			hiusblog("Not in host mode, shall power off\n");
			dwc_otg_disable_global_interrupts(otg_dev->core_if);
			clearup_dvc_and_phy();
			hiusb_info_p->hiusb_status = HIUSB_OFF;
		}
	/* Deep sleep will power off pctl, so set phy into siddq state when resume */
	} else if (HIUSB_OFF == hiusb_info_p->hiusb_status) {
		setup_dvc_and_phy();
		clearup_dvc_and_phy();
	}
	hiusblog("[%s]-\n", __func__);
	up(&hiusb_info_p->hiusb_lock);
	return 0;
}


int k3v2_otg_id_status_change(int id_status)
{
	unsigned long flags;
	int ret = 0;
	char *s = "illegal status";
	hiusblog("[%s]+\n", __func__);

	spin_lock_irqsave(&(hiusb_info_p->intr_flag_lock), flags);
	if (ID_FALL == id_status) {
		hiusb_info_p->intr_flag = ID_FALL_EVENT;
		s = "ID fall";
	} else if (ID_RISE == id_status) {
		hiusb_info_p->intr_flag = ID_RISE_EVENT;
		s = "ID rise";
	} else {
		hiusblog("[%s]illegal status: %d!\n", __func__, id_status);
		s = "illegal status";
		spin_unlock_irqrestore(&(hiusb_info_p->intr_flag_lock), flags);
		return -EINVAL;
	}

	hiusb_info_p->intr_jiffies = jiffies;
	ret = schedule_delayed_work(&hiusb_info_p->k3v2_otg_intr_work, 0);
	spin_unlock_irqrestore(&(hiusb_info_p->intr_flag_lock), flags);
	hiusblog("intr_flag: %s\n", s);
	hiusblog("[%s]-\n", __func__);
	return ret;
}
