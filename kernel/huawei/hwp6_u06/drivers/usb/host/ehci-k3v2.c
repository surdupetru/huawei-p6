/*
 * linux/driver/usb/host/ehci-hisik3.c
 *
 * Copyright (c) 2011 Hisi technology corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation;version 2 of the License.
 *
 */
#include <mach/platform.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <mach/usb_phy.h>

#define HISIK3_USB_DMA_ALIGN 32

struct hisik3_ehci_hcd *hiusb_ehci_private;
#define CONFIG_USB_EHCI_ONOFF_FEATURE

static void hisik3_ehci_power_up(struct usb_hcd *hcd);
static void hisik3_ehci_power_down(struct usb_hcd *hcd);

#ifdef CONFIG_PM
static int hisik3_ehci_bus_suspend(struct usb_hcd *hcd);
static int hisik3_ehci_bus_resume(struct usb_hcd *hcd);
static int hisik3_usb_suspend(struct usb_hcd *hcd);
static int hisik3_usb_resume(struct usb_hcd *hcd);
#endif


#ifdef CONFIG_USB_EHCI_ONOFF_FEATURE
/* Stored ehci handle for hsic insatnce */
struct usb_hcd *ehci_handle;

extern int connect_status;

static ssize_t show_ehci_status(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%d\n", connect_status);
}

static DEFINE_MUTEX(ehci_power_lock);
static ssize_t store_ehci_power(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int power_on;
	int ret;
	struct hisik3_ehci_hcd *hiusb_ehci = dev_get_drvdata(dev);
	struct usb_hcd *hcd = ehci_to_hcd(hiusb_ehci->ehci);

	if (sscanf(buf, "%d", &power_on) != 1)
	{
		printk("%s::return EINVAL.\n", __func__);				
		return -EINVAL;
	}

	mutex_lock(&ehci_power_lock);

	if (power_on == 0 && ehci_handle != NULL) {
		/*enable hisc phy cnf clk,12MHz and 480MHz.*/
		hisik3_usb_phy_clk_enable(hiusb_ehci->phy);
		ret = clk_enable(hiusb_ehci->core_clk);
		hiusb_ehci->clock_state = 1;
		if (ret) {
			printk(KERN_ERR "%s.clk_enable core_clk failed\n", __func__);
		}

		connect_status = 0;
		clear_bit(HCD_FLAG_DEAD, &hcd->flags);
		usb_remove_hcd(hcd);
		hisik3_ehci_power_down(hcd);		
		ehci_handle = NULL;
	} else if (power_on == 1) {
		if (ehci_handle)
		{
			clear_bit(HCD_FLAG_DEAD, &hcd->flags);
			usb_remove_hcd(hcd);
			hisik3_ehci_power_down(hcd);
		}

		hisik3_ehci_power_up(hcd);

		hiusb_ehci->ehci->caps = hcd->regs;
		hiusb_ehci->ehci->regs = hcd->regs +
			 HC_LENGTH(hiusb_ehci->ehci, ehci_readl(hiusb_ehci->ehci, &hiusb_ehci->ehci->caps->hc_capbase));
		hiusb_ehci->ehci->hcs_params = ehci_readl(hiusb_ehci->ehci, &hiusb_ehci->ehci->caps->hcs_params);
		hiusb_ehci->ehci->sbrn = 0x20;

		if (usb_add_hcd(hiusb_ehci->hcd, hiusb_ehci->irq, IRQF_SHARED)) {
			printk("Failed to add USB HCD\n");
			mutex_unlock(&ehci_power_lock);
			return -1;
		}
		ehci_writel(hiusb_ehci->ehci, 1, &hiusb_ehci->ehci->regs->configured_flag);
		
		ehci_handle = hcd;

		hiusb_ehci->ehci_init = 1;
	/* For debug use */
	} else if (power_on == 2) {
		hisik3_ehci_bus_suspend(hcd);
	} else if (power_on == 3) {
		hisik3_ehci_bus_resume(hcd);
	} else if (power_on == 4) {
		hisik3_usb_suspend(hcd);
	} else if (power_on == 5) {
		hisik3_usb_resume(hcd);
	}

	mutex_unlock(&ehci_power_lock);
	return count;
}

static DEVICE_ATTR(ehci_power, S_IRUGO | S_IWUSR | S_IWGRP,
		show_ehci_status, store_ehci_power);

#endif

static void hisik3_ehci_power_up(struct usb_hcd *hcd)
{
	int ret = 0;
	struct hisik3_ehci_hcd *hiusb_ehci = dev_get_drvdata(hcd->self.controller);

	if (!hiusb_ehci->clock_state) {
		/*enable hisc phy cnf clk,12MHz and 480MHz.*/
		hisik3_usb_phy_clk_enable(hiusb_ehci->phy);
		ret = clk_enable(hiusb_ehci->core_clk);
		if (ret) {
			printk(KERN_ERR "%s.clk_enable core_clk failed\n", __func__);
		}

		hiusb_ehci->clock_state = 1;
	}

	hisik3_usb_phy_power_on(hiusb_ehci);
	hiusb_ehci->host_resumed = 1;
	return;
}

static void hisik3_ehci_power_down(struct usb_hcd *hcd)
{
	struct hisik3_ehci_hcd *hiusb_ehci = dev_get_drvdata(hcd->self.controller);
	hiusb_ehci->host_resumed = 0;	
	hisik3_usb_phy_power_off(hiusb_ehci);

	if (hiusb_ehci->clock_state) {
		/*disable hisc phy cnf clk,12MHz and 480MHz.*/
		clk_disable(hiusb_ehci->core_clk);
		hisik3_usb_phy_clk_disable(hiusb_ehci->phy);
		hiusb_ehci->clock_state = 0;
	}

	return;
}

static int hisik3_ehci_internal_port_reset(
	struct ehci_hcd	*ehci,
	u32 __iomem	*portsc_reg
)
{
	u32		temp;
	unsigned long	flags;
	int		retval = 0;
	int		i, tries;
	u32		saved_usbintr;

	spin_lock_irqsave(&ehci->lock, flags);
	saved_usbintr = ehci_readl(ehci, &ehci->regs->intr_enable);
	/* disable USB interrupt */
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	spin_unlock_irqrestore(&ehci->lock, flags);

	/*
	 * Here we have to do Port Reset at most twice for
	 * Port Enable bit to be set.
	 */
	for (i = 0; i < 2; i++) {
		temp = ehci_readl(ehci, portsc_reg);
		temp |= PORT_RESET;
		ehci_writel(ehci, temp, portsc_reg);
		mdelay(10);
		temp &= ~PORT_RESET;
		ehci_writel(ehci, temp, portsc_reg);
		mdelay(1);
		tries = 100;
		do {
			mdelay(1);
			/*
			 * Up to this point, Port Enable bit is
			 * expected to be set after 2 ms waiting.
			 * USB1 usually takes extra 45 ms, for safety,
			 * we take 100 ms as timeout.
			 */
			temp = ehci_readl(ehci, portsc_reg);
		} while (!(temp & PORT_PE) && tries--);
		if (temp & PORT_PE)
			break;
	}
	if (i == 2)
		retval = -ETIMEDOUT;

	/*
	 * Clear Connect Status Change bit if it's set.
	 * We can't clear PORT_PEC. It will also cause PORT_PE to be cleared.
	 */
	if (temp & PORT_CSC)
		ehci_writel(ehci, PORT_CSC, portsc_reg);

	/*
	 * Write to clear any interrupt status bits that might be set
	 * during port reset.
	 */
	temp = ehci_readl(ehci, &ehci->regs->status);
	ehci_writel(ehci, temp, &ehci->regs->status);

	/* restore original interrupt enable bits */
	ehci_writel(ehci, saved_usbintr, &ehci->regs->intr_enable);
	return retval;
}

static int hisik3_ehci_hub_control(
	struct usb_hcd	*hcd,
	u16		typeReq,
	u16		wValue,
	u16		wIndex,
	char		*buf,
	u16		wLength
)
{
	struct ehci_hcd	*ehci = hcd_to_ehci(hcd);
	struct hisik3_ehci_hcd *hiusb_ehci = dev_get_drvdata(hcd->self.controller);
	
	u32 __iomem	*status_reg;
	u32		temp;
	unsigned long	flags;
	int		retval = 0;
	printk("hisik3_ehci_hub_control.\n");

	status_reg = &ehci->regs->port_status[(wIndex & 0xff) - 1];

	spin_lock_irqsave(&ehci->lock, flags);

	/*
	 * In ehci_hub_control() for USB_PORT_FEAT_ENABLE clears the other bits
	 * that are write on clear, by writing back the register read value, so
	 * USB_PORT_FEAT_ENABLE is handled by masking the set on clear bits
	 */
	if (typeReq == ClearPortFeature && wValue == USB_PORT_FEAT_ENABLE) {
		temp = ehci_readl(ehci, status_reg) & ~PORT_RWC_BITS;
		ehci_writel(ehci, temp & ~PORT_PE, status_reg);
		goto done;
	}

	else if (typeReq == GetPortStatus) {
		temp = ehci_readl(ehci, status_reg);
		if (hiusb_ehci->port_resuming && !(temp & PORT_SUSPEND)) {
			/* Resume completed, re-enable disconnect detection */
			hiusb_ehci->port_resuming = 0;
			hisik3_usb_phy_postresume(hiusb_ehci->phy);
		}
	}

	else if (typeReq == SetPortFeature && wValue == USB_PORT_FEAT_SUSPEND) {
		temp = ehci_readl(ehci, status_reg);
		if ((temp & PORT_PE) == 0 || (temp & PORT_RESET) != 0) {
			retval = -EPIPE;
			goto done;
		}

		temp &= ~PORT_WKCONN_E;
		temp |= PORT_WKDISC_E | PORT_WKOC_E;
		ehci_writel(ehci, temp | PORT_SUSPEND, status_reg);

		/*
		 * If a transaction is in progress, there may be a delay in
		 * suspending the port. Poll until the port is suspended.
		 */
		if (handshake(ehci, status_reg, PORT_SUSPEND,
						PORT_SUSPEND, 5000))
			pr_err("%s: timeout waiting for SUSPEND\n", __func__);

		set_bit((wIndex & 0xff) - 1, &ehci->suspended_ports);
		goto done;
	}

	/* For USB1 port we need to issue Port Reset twice internally */
	if (hiusb_ehci->phy->instance == 0 &&
	   (typeReq == SetPortFeature && wValue == USB_PORT_FEAT_RESET)) {
		spin_unlock_irqrestore(&ehci->lock, flags);
		return hisik3_ehci_internal_port_reset(ehci, status_reg);
	}

	/*
	 * host controller will time the resume operation to clear the bit
	 * when the port control state switches to HS or FS Idle. This behavior
	 * is different from EHCI where the host controller driver is required
	 * to set this bit to a zero after the resume duration is timed in the
	 * driver.
	 */
	else if (typeReq == ClearPortFeature &&
					wValue == USB_PORT_FEAT_SUSPEND) {
		temp = ehci_readl(ehci, status_reg);
		if ((temp & PORT_RESET) || !(temp & PORT_PE)) {
			retval = -EPIPE;
			goto done;
		}

		if (!(temp & PORT_SUSPEND))
			goto done;

		/* Disable disconnect detection during port resume */
		hisik3_usb_phy_preresume(hiusb_ehci->phy);

		ehci->reset_done[wIndex-1] = jiffies + msecs_to_jiffies(25);

		temp &= ~(PORT_RWC_BITS | PORT_WAKE_BITS);
		/* start resume signalling */
		ehci_writel(ehci, temp | PORT_RESUME, status_reg);

		spin_unlock_irqrestore(&ehci->lock, flags);
		msleep(20);
		spin_lock_irqsave(&ehci->lock, flags);

		/* Poll until the controller clears RESUME and SUSPEND */
		if (handshake(ehci, status_reg, PORT_RESUME, 0, 2000))
			pr_err("%s: timeout waiting for RESUME\n", __func__);
		if (handshake(ehci, status_reg, PORT_SUSPEND, 0, 2000))
			pr_err("%s: timeout waiting for SUSPEND\n", __func__);

		ehci->reset_done[wIndex-1] = 0;

		hiusb_ehci->port_resuming = 1;
		goto done;
	}

	spin_unlock_irqrestore(&ehci->lock, flags);

	/* Handle the hub control events here */
	return ehci_hub_control(hcd, typeReq, wValue, wIndex, buf, wLength);
done:
	spin_unlock_irqrestore(&ehci->lock, flags);
	return retval;
}

static void hisik3_ehci_restart(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	ehci_reset(ehci);

	/* setup the frame list and Async q heads */
	ehci_writel(ehci, ehci->periodic_dma, &ehci->regs->frame_list);
	ehci_writel(ehci, (u32)ehci->async->qh_dma, &ehci->regs->async_next);
	/* setup the command register and set the controller in RUN mode */
	ehci->command &= ~(CMD_LRESET|CMD_IAAD|CMD_PSE|CMD_ASE|CMD_RESET);
	ehci->command |= CMD_RUN;
	ehci_writel(ehci, ehci->command, &ehci->regs->command);

	down_write(&ehci_cf_port_reset_rwsem);
	ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
	/* flush posted writes */
	ehci_readl(ehci, &ehci->regs->command);
	up_write(&ehci_cf_port_reset_rwsem);
}

static int hisik3_usb_suspend(struct usb_hcd *hcd)
{
	struct hisik3_ehci_hcd *hiusb_ehci = dev_get_drvdata(hcd->self.controller);
	
	struct ehci_regs __iomem *hw = hiusb_ehci->ehci->regs;
	unsigned long flags;
	printk("hisik3_usb_suspend.\n");

	spin_lock_irqsave(&hiusb_ehci->ehci->lock, flags);

	hiusb_ehci->port_speed = (readl(&hw->port_status[0]) >> 26) & 0x3;
	ehci_halt(hiusb_ehci->ehci);
	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	spin_unlock_irqrestore(&hiusb_ehci->ehci->lock, flags);

	hisik3_ehci_power_down(hcd);
	return 0;
}

static int hisik3_usb_resume(struct usb_hcd *hcd)
{
	struct hisik3_ehci_hcd *hiusb_ehci = dev_get_drvdata(hcd->self.controller);
	struct ehci_hcd	*ehci = hcd_to_ehci(hcd);
	struct ehci_regs __iomem *hw = ehci->regs;
	unsigned long val;
	printk("hisik3_usb_resume.\n");

	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	hisik3_ehci_power_up(hcd);

	if (hiusb_ehci->port_speed > HISIK3_USB_PHY_PORT_SPEED_HIGH) {
		/* Wait for the phy to detect new devices
		 * before we restart the controller */
		msleep(10);
		goto restart;
	}

	/* Force the phy to keep data lines in suspend state */
	hisik3_ehci_phy_restore_start(hiusb_ehci->phy, hiusb_ehci->port_speed);

	/* Enable host mode */
	tdi_reset(ehci);

	/* Enable Port Power */
	val = readl(&hw->port_status[0]);
	val |= PORT_POWER;
	writel(val, &hw->port_status[0]);
	udelay(10);

	/* Check if the phy resume from LP0. When the phy resume from LP0
	 * USB register will be reset. */
	if (!readl(&hw->async_next)) {
		/* Program the field PTC based on the saved speed mode */
		val = readl(&hw->port_status[0]);
		val &= ~PORT_TEST(~0);
		if (hiusb_ehci->port_speed == HISIK3_USB_PHY_PORT_SPEED_HIGH)
			val |= PORT_TEST_FORCE;
		else if (hiusb_ehci->port_speed == HISIK3_USB_PHY_PORT_SPEED_FULL)
			val |= PORT_TEST(6);
		else if (hiusb_ehci->port_speed == HISIK3_USB_PHY_PORT_SPEED_LOW)
			val |= PORT_TEST(7);
		writel(val, &hw->port_status[0]);
		udelay(10);

		/* Disable test mode by setting PTC field to NORMAL_OP */
		val = readl(&hw->port_status[0]);
		val &= ~PORT_TEST(~0);
		writel(val, &hw->port_status[0]);
		udelay(10);
	}

	/* Poll until CCS is enabled */
	if (handshake(ehci, &hw->port_status[0], PORT_CONNECT,
						 PORT_CONNECT, 2000)) {
		pr_err("%s: timeout waiting for PORT_CONNECT\n", __func__);
		goto restart;
	}

	/* Poll until PE is enabled */
	if (handshake(ehci, &hw->port_status[0], PORT_PE,
						 PORT_PE, 2000)) {
		pr_err("%s: timeout waiting for USB_PORTSC1_PE\n", __func__);
		goto restart;
	}

	/* Clear the PCI status, to avoid an interrupt taken upon resume */
	val = readl(&hw->status);
	val |= STS_PCD;
	writel(val, &hw->status);

	/* Put controller in suspend mode by writing 1 to SUSP bit of PORTSC */
	val = readl(&hw->port_status[0]);
	if ((val & PORT_POWER) && (val & PORT_PE)) {
		val |= PORT_SUSPEND;
		writel(val, &hw->port_status[0]);

		/* Wait until port suspend completes */
		if (handshake(ehci, &hw->port_status[0], PORT_SUSPEND,
							 PORT_SUSPEND, 1000)) {
			pr_err("%s: timeout waiting for PORT_SUSPEND\n",
								__func__);
			goto restart;
		}
	}

	hisik3_ehci_phy_restore_end(hiusb_ehci->phy);
	return 0;

restart:
	if (hiusb_ehci->port_speed <= HISIK3_USB_PHY_PORT_SPEED_HIGH)
		hisik3_ehci_phy_restore_end(hiusb_ehci->phy);

	hisik3_ehci_restart(hcd);
	return 0;
}

static void hisik3_ehci_shutdown(struct usb_hcd *hcd)
{
	struct hisik3_ehci_hcd *hiusb_ehci = dev_get_drvdata(hcd->self.controller);

	ehci_dbg(hiusb_ehci->ehci, "hisik3_ehci_shutdown.\n");

	/* ehci_shutdown touches the USB controller registers, make sure
	 * controller has clocks to it */
	if (!hiusb_ehci->host_resumed)
		hisik3_ehci_power_up(hcd);

	if (hiusb_ehci->ehci_init)
		ehci_shutdown(hcd);
}

static int hisik3_ehci_setup(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int retval;

	/* EHCI registers start at offset 0x100 */
	ehci->caps = hcd->regs + 0x100;
	ehci->regs = hcd->regs + 0x100 +
		HC_LENGTH(ehci, readl(&ehci->caps->hc_capbase));

	dbg_hcs_params(ehci, "reset");
	dbg_hcc_params(ehci, "reset");

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	/* switch to host mode */
	hcd->has_tt = 1;
	ehci_reset(ehci);

	retval = ehci_halt(ehci);
	if (retval)
		return retval;

	/* data structure init */
	retval = ehci_init(hcd);
	if (retval)
		return retval;

	ehci->sbrn = 0x20;

	ehci_port_power(ehci, 1);
	return retval;
}

#ifdef CONFIG_PM
typedef void (*FN)(void);
FN current_post_suspend=NULL;
FN current_pre_resume=NULL;

int hisik3_ehci_bus_post_suspend_register( struct device *dev, void (*fn)(void) )
{
    struct hisik3_ehci_hcd *hiusb_ehci = hiusb_ehci_private;

    if ( hiusb_ehci && hiusb_ehci->pdata ) {
        printk("%s: post_suspend %s\n", __func__, hiusb_ehci->pdata->post_suspend ? "already registered" : "will be registered");
        hiusb_ehci->pdata->post_suspend = fn;
        current_post_suspend = fn;
        return 0;
    } else {
        printk("%s: hiusb_ehci %s, post_suspend will be registered later\n", __func__, "NULL");
        current_post_suspend = fn;
        return -1;
    }
}
EXPORT_SYMBOL_GPL(hisik3_ehci_bus_post_suspend_register);
void hisik3_ehci_bus_post_suspend_unregister( struct device *dev )
{
    struct hisik3_ehci_hcd *hiusb_ehci = hiusb_ehci_private;

    if ( hiusb_ehci && hiusb_ehci->pdata ) {
        printk("%s: post_suspend %s\n", __func__, hiusb_ehci->pdata->post_suspend ? "will be unregistered" : "already unregistered");
        hiusb_ehci->pdata->post_suspend = NULL;
        current_post_suspend = NULL;
    } else {
        printk("%s: hiusb_ehci %s\n", __func__, "NULL");
        current_post_suspend = NULL;
    }
}
EXPORT_SYMBOL_GPL(hisik3_ehci_bus_post_suspend_unregister);

int hisik3_ehci_bus_pre_resume_register( struct device *dev, void (*fn)(void) )
{
    struct hisik3_ehci_hcd *hiusb_ehci = hiusb_ehci_private;

    if ( hiusb_ehci && hiusb_ehci->pdata ) {
        pr_info("%s: pre_resume %s\n", __func__, hiusb_ehci->pdata->pre_resume ? "already registered" : "will be registered");
        hiusb_ehci->pdata->pre_resume = fn;
        current_pre_resume = fn;
        return 0;
    } else {
        printk("%s: hiusb_ehci %s, pre_resume will be registered later\n", __func__, "NULL");
        current_pre_resume = fn;
        return -1;
    }
}
EXPORT_SYMBOL_GPL(hisik3_ehci_bus_pre_resume_register);
void hisik3_ehci_bus_pre_resume_unregister( struct device *dev )
{
    struct hisik3_ehci_hcd *hiusb_ehci = hiusb_ehci_private;

    if ( hiusb_ehci && hiusb_ehci->pdata ) {
        printk("%s: pre_resume %s\n", __func__, hiusb_ehci->pdata->pre_resume ? "will be unregistered" : "already unregistered");
        hiusb_ehci->pdata->pre_resume = NULL;
        current_pre_resume = NULL;
    } else {
        printk("%s: hiusb_ehci %s\n", __func__, "NULL");
        current_pre_resume = NULL;
    }
}
EXPORT_SYMBOL_GPL(hisik3_ehci_bus_pre_resume_unregister);

static int hisik3_ehci_bus_suspend(struct usb_hcd *hcd)
{
	struct hisik3_ehci_hcd *hiusb_ehci = dev_get_drvdata(hcd->self.controller);
	int error_status = 0;

	printk("hisik3_ehci_bus_suspend.\n");

      if (hiusb_ehci->pdata && hiusb_ehci->pdata->post_suspend)
		hiusb_ehci->pdata->post_suspend();

	error_status = ehci_bus_suspend(hcd);
	if (!error_status && hiusb_ehci->power_down_on_bus_suspend) {
		hisik3_usb_suspend(hcd);
		hiusb_ehci->bus_suspended = 1;
	}

	if (hiusb_ehci->clock_state) {
		/*disable hisc phy cnf clk,12MHz and 480MHz.*/
		clk_disable(hiusb_ehci->core_clk);
		hisik3_usb_phy_clk_disable(hiusb_ehci->phy);
		hiusb_ehci->clock_state = 0;
	}

	return error_status;
}

static int hisik3_ehci_bus_resume(struct usb_hcd *hcd)
{
	int ret = 0;
	struct hisik3_ehci_hcd *hiusb_ehci = dev_get_drvdata(hcd->self.controller);
	printk("hisik3_ehci_bus_resume.\n");

	if (hiusb_ehci->pdata && hiusb_ehci->pdata->pre_resume)
		hiusb_ehci->pdata->pre_resume();

	if (!hiusb_ehci->clock_state) {
		/*enable hisc phy cnf clk,12MHz and 480MHz.*/
		hisik3_usb_phy_clk_enable(hiusb_ehci->phy);
		ret = clk_enable(hiusb_ehci->core_clk);
		if (ret) {
			printk(KERN_ERR "%s.clk_enable core_clk failed\n", __func__);
		}

		hiusb_ehci->clock_state = 1;
	}

	if (hiusb_ehci->bus_suspended && hiusb_ehci->power_down_on_bus_suspend) {
		hisik3_usb_resume(hcd);
		hiusb_ehci->bus_suspended = 0;
	}

	hisik3_usb_phy_preresume(hiusb_ehci->phy);
	hiusb_ehci->port_resuming = 1;
	return ehci_bus_resume(hcd);
}
#endif

struct temp_buffer {
	void *kmalloc_ptr;
	void *old_xfer_buffer;
	u8 data[0];
};

static void free_temp_buffer(struct urb *urb)
{
	enum dma_data_direction dir;
	struct temp_buffer *temp;

	if (!(urb->transfer_flags & URB_ALIGNED_TEMP_BUFFER))
		return;

	dir = usb_urb_dir_in(urb) ? DMA_FROM_DEVICE : DMA_TO_DEVICE;

	temp = container_of(urb->transfer_buffer, struct temp_buffer,
			    data);

	if (dir == DMA_FROM_DEVICE)
		memcpy(temp->old_xfer_buffer, temp->data,
		       urb->transfer_buffer_length);
	urb->transfer_buffer = temp->old_xfer_buffer;
	kfree(temp->kmalloc_ptr);

	urb->transfer_flags &= ~URB_ALIGNED_TEMP_BUFFER;
}

static int alloc_temp_buffer(struct urb *urb, gfp_t mem_flags)
{
	enum dma_data_direction dir;
	struct temp_buffer *temp, *kmalloc_ptr;
	size_t kmalloc_size;

	if (urb->num_sgs || urb->sg ||
	    urb->transfer_buffer_length == 0 ||
	    !((uintptr_t)urb->transfer_buffer & (HISIK3_USB_DMA_ALIGN - 1)))
		return 0;

	dir = usb_urb_dir_in(urb) ? DMA_FROM_DEVICE : DMA_TO_DEVICE;

	/* Allocate a buffer with enough padding for alignment */
	kmalloc_size = urb->transfer_buffer_length +
		sizeof(struct temp_buffer) + HISIK3_USB_DMA_ALIGN - 1;

	kmalloc_ptr = kmalloc(kmalloc_size, mem_flags);
	if (!kmalloc_ptr)
		return -ENOMEM;

	/* Position our struct temp_buffer such that data is aligned */
	temp = PTR_ALIGN(kmalloc_ptr + 1, HISIK3_USB_DMA_ALIGN) - 1;

	temp->kmalloc_ptr = kmalloc_ptr;
	temp->old_xfer_buffer = urb->transfer_buffer;
	if (dir == DMA_TO_DEVICE)
		memcpy(temp->data, urb->transfer_buffer,
		       urb->transfer_buffer_length);
	urb->transfer_buffer = temp->data;

	urb->transfer_flags |= URB_ALIGNED_TEMP_BUFFER;

	return 0;
}

static int hisik3_ehci_map_urb_for_dma(struct usb_hcd *hcd, struct urb *urb,
				      gfp_t mem_flags)
{
	int ret;

	ret = alloc_temp_buffer(urb, mem_flags);
	if (ret)
		return ret;

	ret = usb_hcd_map_urb_for_dma(hcd, urb, mem_flags);
	if (ret)
		free_temp_buffer(urb);

	return ret;
}

static void hisik3_ehci_unmap_urb_for_dma(struct usb_hcd *hcd, struct urb *urb)
{
	usb_hcd_unmap_urb_for_dma(hcd, urb);
	free_temp_buffer(urb);
}

static int hisik3_ehci_update_device(struct usb_hcd *hcd, struct usb_device *udev)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int rc = 0;
	printk("hisik3_ehci_update_device.\n");

	if (!udev->parent) /* udev is root hub itself, impossible */
		rc = -1;
	/* we only support lpm device connected to root hub yet */
	if (ehci->has_lpm && !udev->parent->parent) {
		rc = ehci_lpm_set_da(ehci, udev->devnum, udev->portnum);
		if (!rc)
			rc = ehci_lpm_check(ehci, udev->portnum);
	}
	return rc;
}

static const struct hc_driver hisik3_ehci_hc_driver = {
	.description     = hcd_name,
	.product_desc  = "Hisilicon K3 EHCI Host Controller",
	.hcd_priv_size  = sizeof(struct ehci_hcd),

	.irq			       = ehci_irq,
	.flags			= HCD_USB2 | HCD_MEMORY,

	//.reset			= hisik3_ehci_setup, //may be used in otg's env
	.reset                    = ehci_init,
	.start			= ehci_run,
	
	.stop			= ehci_stop,
	.shutdown		= hisik3_ehci_shutdown,
	
	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	
	.get_frame_number	= ehci_get_frame,
	
	//.map_urb_for_dma	= hisik3_ehci_map_urb_for_dma,
	//.unmap_urb_for_dma	= hisik3_ehci_unmap_urb_for_dma,
	.endpoint_reset		= ehci_endpoint_reset,
	.hub_status_data	       = ehci_hub_status_data,
	
	//.hub_control		= hisik3_ehci_hub_control,
	.hub_control		= ehci_hub_control,
	//.clear_tt_buffer_complete = ehci_clear_tt_buffer_complete,
#ifdef CONFIG_PM
	.bus_suspend		= hisik3_ehci_bus_suspend,
	.bus_resume		= hisik3_ehci_bus_resume,
#endif
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	/*
	 * call back when device connected and addressed
	 hisik3_ehci_update_device,used for LPM,
	 */
	//.update_device =	hisik3_ehci_update_device,
};

static int hisik3_ehci_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct usb_hcd *hcd;
	struct hisik3_ehci_hcd *hiusb_ehci;
	struct hisik3_ehci_platform_data *pdata;
	int err = 0;
	int irq;

	if (usb_disabled())
		return -ENODEV;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		pr_err( "Platform data missing\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!res)) {
		return -ENXIO;
	}
	
	irq = platform_get_irq(pdev, 0);
	if (unlikely(irq < 0)) {
		return -EINTR;
	}

	hcd = usb_create_hcd(&hisik3_ehci_hc_driver, &pdev->dev, hcd_name);
	if (!hcd) {
		pr_err( "Unable to create HCD\n");
		return -ENODEV;
	}

	hiusb_ehci = kzalloc(sizeof(struct hisik3_ehci_hcd), GFP_KERNEL);
	if (!hiusb_ehci)
	{
		goto fail_alloc;
	}

	platform_set_drvdata(pdev, hiusb_ehci);

	hcd->regs = (void __iomem *)res->start;
	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	
	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		err = -EBUSY;
		goto fail_mregion;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (unlikely(hcd->regs == NULL)) {
		err = -EFAULT;
		goto fail_io;
	}
	
	hiusb_ehci->host_resumed = 1;
	hiusb_ehci->power_down_on_bus_suspend = pdata->power_down_on_bus_suspend;
	printk("%s:: hiusb_ehci->power_down_on_bus_suspend = %d.\n", __func__, hiusb_ehci->power_down_on_bus_suspend);
	hiusb_ehci->ehci = hcd_to_ehci(hcd);

	/*save private infos*/
	hiusb_ehci->hcd = hcd;
	hiusb_ehci->irq = irq;
	hiusb_ehci->instance = pdev->id; //用来支持多个EHCI/OHCI 控制器的情况。
	hiusb_ehci->pdata = pdata;
	hiusb_ehci->ehci_init = 0;
	hiusb_ehci->clock_state = 1;
	hiusb_ehci_private = hiusb_ehci;

	hiusb_ehci->core_clk = clk_get(NULL, "clk_usb2hst");
	if (IS_ERR(hiusb_ehci->core_clk)) {
		pr_err( "Can't get ehci clock\n");
		err = PTR_ERR(hiusb_ehci->core_clk);
		goto fail_clk;
	}
	clk_enable(hiusb_ehci->core_clk);

	hiusb_ehci->phy = hisik3_usb_phy_open(0, hiusb_ehci->hcd->regs, hiusb_ehci->pdata->phy_config,
						HISIK3_USB_PHY_MODE_HOST);//此instance是用来区分不同 PHY。
	if (IS_ERR(hiusb_ehci->phy)) {
		printk("Failed to open USB phy\n");
		err =  -ENXIO;
		goto fail_phy;
	}

	hisik3_ehci_power_down(hcd);

    //Register hisik3_ehci_bus_post_suspend and hisik3_ehci_bus_pre_resume function for dynamic compatible with multi modem
    err = hisik3_ehci_bus_post_suspend_register( &(pdev->dev), current_post_suspend );
    if (err)
        dev_info(&pdev->dev, "Warning: Register hisik3_ehci_bus_post_suspend fn failed!\n");

    err = hisik3_ehci_bus_pre_resume_register( &(pdev->dev), current_pre_resume );
    if (err)
        dev_info(&pdev->dev, "Warning: Register hisik3_ehci_bus_pre_resume fn failed!\n");

#ifdef CONFIG_USB_EHCI_ONOFF_FEATURE
	err = device_create_file(&(pdev->dev), &dev_attr_ehci_power);
	if (err) {
		dev_err(&pdev->dev, "Failed to create sysfs entry\n");
		goto fail_attr;
	}
#endif
	return err;

fail:	
	hisik3_ehci_power_down(hcd);
	usb_remove_hcd(hcd);
fail_attr:	
	hisik3_usb_phy_close(hiusb_ehci->phy);
fail_phy:	
	clk_put(hiusb_ehci->core_clk);
fail_clk:
	iounmap(hcd->regs);
fail_io:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
fail_mregion:
	kfree(hiusb_ehci);
fail_alloc:
	usb_put_hcd(hcd);
	return err;
}

#ifdef CONFIG_PM
static int hisik3_ehci_resume(struct platform_device *pdev)
{
	struct hisik3_ehci_hcd *hiusb_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = ehci_to_hcd(hiusb_ehci->ehci);
	printk("hisik3_ehci_resume.\n");

	if (hiusb_ehci->bus_suspended)
		return 0;

	return hisik3_usb_resume(hcd);
}

static int hisik3_ehci_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct hisik3_ehci_hcd *hiusb_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = ehci_to_hcd(hiusb_ehci->ehci);

	printk("hisik3_ehci_suspend.\n");

	if (hiusb_ehci->bus_suspended)
		return 0;

	if (time_before(jiffies, hiusb_ehci->ehci->next_statechange))
		msleep(10);

	return hisik3_usb_suspend(hcd);
}
#endif

static int hisik3_ehci_remove(struct platform_device *pdev)
{
	struct hisik3_ehci_hcd *hiusb_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = NULL;

	if (!hiusb_ehci)
		return -EINVAL;

	hcd = ehci_to_hcd(hiusb_ehci->ehci);

	if (!hcd)
		return -EINVAL;

	usb_remove_hcd(hcd);
	usb_put_hcd(hcd);

	hisik3_usb_phy_power_off(hiusb_ehci);
	hisik3_usb_phy_close(hiusb_ehci->phy);
	iounmap(hcd->regs);

	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);

	clk_disable(hiusb_ehci->core_clk);
	clk_put(hiusb_ehci->core_clk);

	kfree(hiusb_ehci);
	
	return 0;
}

static void hisik3_ehci_hcd_shutdown(struct platform_device *pdev)
{
	struct hisik3_ehci_hcd *hiusb_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = NULL;

	if (!hiusb_ehci) {
		return;
	}

	hcd = ehci_to_hcd(hiusb_ehci->ehci);

	if (!hcd) {
		return;
	}

	if (hcd->driver->shutdown)
		hcd->driver->shutdown(hcd);
}

#ifdef CONFIG_PM
static int ehci_k3v2_pm_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	bool wakeup = device_may_wakeup(dev);

	dev_dbg(dev, "k3v2 ehci PM suspend\n");

	/*
	 * EHCI helper function has also the same check before manipulating
	 * port wakeup flags.  We do check here the same condition before
	 * calling the same helper function to avoid bringing hardware
	 * from Low power mode when there is no need for adjusting port
	 * wakeup flags.
	 */
	if (hcd->self.root_hub->do_remote_wakeup && !wakeup) {
		pm_runtime_resume(dev);
		ehci_prepare_ports_for_controller_suspend(hcd_to_ehci(hcd),
				wakeup);
	}

	return 0;
}

static int ehci_k3v2_pm_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);

	dev_dbg(dev, "k3v2 ehci PM resume\n");
	ehci_prepare_ports_for_controller_resume(hcd_to_ehci(hcd));

	return 0;
}
#else
#define ehci_k3v2_pm_suspend	NULL
#define ehci_k3v2_pm_resume	NULL
#endif

static const struct dev_pm_ops ehci_k3v2_dev_pm_ops = {
	.suspend         = ehci_k3v2_pm_suspend,
	.resume          = ehci_k3v2_pm_resume,
};

static struct platform_driver ehci_k3v2_driver = {
	.probe		= hisik3_ehci_probe,
	.remove		= hisik3_ehci_remove,
#ifdef CONFIG_PM
//	.suspend	= hisik3_ehci_suspend,
//	.resume		= hisik3_ehci_resume,
#endif
	.shutdown	= hisik3_ehci_hcd_shutdown,
	.driver		= {
		.name	= "hisik3-ehci",
		.owner = THIS_MODULE,
		//.pm = &ehci_k3v2_dev_pm_ops,
	}
};

MODULE_AUTHOR("John <jiangguang@huawei.com>");
MODULE_DESCRIPTION("hisik3 usb ehci driver!");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:hisk3-ehci");
