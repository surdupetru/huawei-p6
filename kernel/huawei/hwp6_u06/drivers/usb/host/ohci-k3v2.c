/*
 * linux/driver/usb/host/ohci-k3v2.c
 *
 * Copyright (c) 2011 Hisi technology corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation;version 2 of the License.
 *
 */

#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/delay.h>

#if 0
#define GPIO0_BASE                      0xFE105000
#define GPIO1_BASE                      0xFE107000
#define GPIO2_BASE                      0xFE108000
#define GPIO3_BASE                      0xFE109000

#define GPIO_REG_LEN                  0x00001000

#define GPIO_DATA(__X__)     (*((unsigned volatile*)(__X__)))
#define GPIO_DATA_ALL(__X__)     (*((unsigned volatile*)(__X__ + 0x3FC)))
#define GPIO_DIR(__X__)     (*((unsigned volatile*)(__X__ + 0x400)))

void __iomem	*gpio0_base;
void __iomem	*gpio1_base;
void __iomem	*gpio2_base;
void __iomem	*gpio3_base;

/* read GPIO by address */
int read_GPIO(uint32_t addr)
{
	volatile int i = 0;
	for (i = 0; i < 400; i++)
		;
	return ioread32(addr);
}

/* write GPIO by address */
void write_GPIO(uint32_t addr, uint8_ t data)
{
	volatile int i = 0;
	for (i = 0; i < 400; i++)
		;
	iowrite32(data, addr);
	return;
}

void ehci_write8(uint8_t regaddr, uint8_t value)
{
	iowrite32(0xff, gpio1_base + 0x400);
	iowrite32(0xff, gpio2_base + 0x400);
	iowrite32(0xff, gpio1_base + 0x3fc);
#if 1
	write_GPIO((uint32_t)gpio2_base + 0x3fc, regaddr);

	/*cs low, d/c high */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xfb);
	/*wr low */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xeb);
	/*wr high */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xfb);
	/*cs high */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xff);


	write_GPIO((uint32_t)gpio2_base + 0x3fc, value);
	/*cs low, d/c low */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xf9);
	/*wr low */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xe9);
	/*wr high */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xf9);
	/*cs high */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xff);
#endif
}

uint8_t ehci_read8(uint8_t regaddr)
{
	uint8_t value = 0;

	iowrite32(0xff, gpio1_base + 0x400);
	iowrite32(0xff, gpio2_base + 0x400);
	iowrite32(0xff, gpio1_base + 0x3fc);

	write_GPIO((uint32_t)gpio2_base + 0x3fc, regaddr);
	/*cs low, d/c high */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xfb);
	/*wr low */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xeb);
	/*wr high */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xfb);
	/*cs high */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xff);

	iowrite32(0x00, gpio2_base + 0x400);

	/*cs low, d/c low */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xf9);
	/*rd low */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xf1);
	/*rd high */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xf9);
	value = read_GPIO((uint32_t)gpio2_base + 0x3fc);
	/*cs high */
	write_GPIO((uint32_t)gpio1_base + 0x78, 0xff);
	return value;
}
#endif

static int ohci_k3v2_start(struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);

	ohci_hcd_init(ohci);
	ohci_init(ohci);
	ohci_run(ohci);
	hcd->state = HC_STATE_RUNNING;
	return 0;
}

static const struct hc_driver ohci_k3v2_hc_driver = {
	.description =		hcd_name,
	.product_desc =    "K3V2 OHCI Host Controller",
	.hcd_priv_size =	sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start =		ohci_k3v2_start,
	.stop =			ohci_stop,
	.shutdown =		ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend =		ohci_bus_suspend,
	.bus_resume =		ohci_bus_resume,
#endif
	.start_port_reset =	ohci_start_port_reset,
};

/*-------------------------------------------------------------------------*/
static int ohci_k3v2_probe(struct platform_device *pdev)
{
	struct resource *res = NULL;
	struct usb_hcd *hcd = NULL;
	int irq = -1;
	int ret;
	uint8_t val;

	printk(KERN_ERR "%s++.\n", __func__);

	if (usb_disabled())
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		err("platform_get_resource error.\n");
		return -ENODEV;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		err("platform_get_irq error.\n");
		return -ENODEV;
	}

	/* initialize hcd */
	hcd = usb_create_hcd(&ohci_k3v2_hc_driver,
			&pdev->dev, (char *)hcd_name);
	if (!hcd) {
		err("Failed to create hcd");
		return -ENOMEM;
	}

	hcd->regs = (void __iomem *)res->start;
	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	printk(KERN_ERR "%s::rsrc_start:0x%x, rsrc_len:0x%x, irq:%d...\n",
			__func__, hcd->rsrc_start, hcd->rsrc_len, irq);
	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		err("request_mem_region failed\n");
		ret = -EBUSY;
		goto err_put;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		err("ioremap failed\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	ohci_hcd_init(hcd_to_ohci(hcd));

	ret = usb_add_hcd(hcd, irq, IRQF_DISABLED);
	if (ret != 0) {
		err("Failed to add hcd");
		goto err_addhcd;
		return ret;
	}
	return 0;
 err_addhcd:
	iounmap(hcd->regs);
 err_ioremap:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
 err_put:
	usb_put_hcd(hcd);
	return ret;

}
#if 0
	gpio0_base = ioremap(GPIO0_BASE, GPIO_REG_LEN);
	if (!gpio0_base) {
		printk("ioremap failed\n");
		return 0;
	}
	gpio1_base = ioremap(GPIO1_BASE, GPIO_REG_LEN);
	if (!gpio1_base) {
		printk("ioremap failed\n");
		return 0;
	}
	gpio2_base = ioremap(GPIO2_BASE, GPIO_REG_LEN);
	if (!gpio2_base) {
		printk("ioremap failed\n");
		return 0;
	}
	gpio3_base = ioremap(GPIO3_BASE, GPIO_REG_LEN);
	if (!gpio3_base) {
		printk("ioremap failed\n");
		return 0;
	}

	iowrite32(0xff, gpio3_base + 0x400);
	write_GPIO((uint32_t)gpio3_base + 0xc, 0x3);

	val = ehci_read8(0x80);
	printk(KERN_ERR "%s::0x80:0x%x.\n", __func__, val);

	/* ehci_write8(0x81, 0x55); */
	val = ehci_read8(0x81);
	printk(KERN_ERR "%s::0x81:0x%x.\n", __func__, val);

	/* ehci_write8(0x82, 0xaa); */
	val = ehci_read8(0x82);
	printk(KERN_ERR "%s::0x82:0x%x.\n", __func__, val);
	val = ehci_read8(0x83);
	printk(KERN_ERR "%s::0x83:0x%x.\n", __func__, val);

	write_GPIO((uint32_t)gpio3_base + 0x1c, 0x7);
#endif

static int ohci_k3v2_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	printk(KERN_ERR "%s++.\n", __func__);

	if (!hcd) {
		return -1;
	}

	usb_remove_hcd(hcd);
	usb_put_hcd(hcd);

	return 0;
}

static struct platform_driver ohci_k3v2_driver = {
	.probe		= ohci_k3v2_probe,
	.remove		= __devexit_p(ohci_k3v2_remove),
	/* .shutdown	= usb_hcd_platform_shutdown, */
	.driver		= {
		.name	= "hisik3-ohci_mod",
		.owner	= THIS_MODULE,
	},
};

MODULE_AUTHOR("John <jiangguang@huawei.com>");
MODULE_DESCRIPTION("K3V2 usb ohci driver!");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:hisik3-ohci");
