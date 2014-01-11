/*
 *  drivers/switch/switch_gpio.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Jake.Chen
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch_usb.h>
#include <linux/workqueue.h>
#include <mach/gpio.h>
#include <linux/io.h>
#include <linux/delay.h>
#include "smart_gpio.h"
#include <linux/mux.h>

static struct switch_usb_dev sdev;
static struct iomux_block *usbswitch_iomux_block = NULL;
static struct block_config *usbswitch_block_config = NULL;

#define IOMUX_BLOCK_NAME "block_usb_switch"

static void usb_switch_work(struct work_struct *work)
{
	switch_usb_set_state(&sdev, USB_TO_AP);
}

static irqreturn_t int_gpio_irq_handler(int irq, void *dev_id)
{
	const int MAX_RETRY = 5;
	int i = 0;
	int int_gpio_value1 = 0;
	int int_gpio_value2 = 0;

	dev_info(sdev.dev, "%s entry.\n", __func__);

	for (i = 0; i < MAX_RETRY; i++) {
		udelay(2);
		if (sdev.pdata->modem1_supported)
		{
			int_gpio_value1 = gpio_get_value(sdev.pdata->modem_int_gpio1);
			dev_info(sdev.dev, "%s: int_gpio_value: %d.\n",
					__func__, int_gpio_value1);
		}
		if (sdev.pdata->modem2_supported)
		{
			int_gpio_value2 = gpio_get_value(sdev.pdata->modem_int_gpio2);
			dev_info(sdev.dev, "%s: int_gpio_value: %d.\n",
					__func__, int_gpio_value2);
		}
		if ((GPIO_HI == int_gpio_value1)
		   ||(GPIO_HI == int_gpio_value2)
		   )
		{
			schedule_work(&sdev.work);
			break;
		}
	}
	dev_info(sdev.dev, "%s end with retry time=%d\n", __func__, i);

	return IRQ_HANDLED;
}

#ifdef CONFIG_PM
static int before_suspend_gpio1_value = SMART_GPIO_NO_TOUCH;
static int before_suspend_gpio2_value = SMART_GPIO_NO_TOUCH;
static int before_suspend_gpio3_value = SMART_GPIO_NO_TOUCH;
static int before_suspend_gpio4_value = SMART_GPIO_NO_TOUCH;

static int usb_switch_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;
	struct usb_switch_platform_data *pdata = pdev->dev.platform_data;

	dev_info(sdev.dev, "%s entry.\n", __func__);

	ret = blockmux_set(usbswitch_iomux_block, usbswitch_block_config, LOWPOWER);
	if (0 > ret) {
		printk(KERN_ERR "%s: set iomux to gpio lowpower error", __FUNCTION__);
	}

	before_suspend_gpio1_value = smart_gpio_get_value(pdata->control_gpio1);
	before_suspend_gpio2_value = smart_gpio_get_value(pdata->control_gpio2);
	before_suspend_gpio3_value = smart_gpio_get_value(pdata->control_gpio3);
	before_suspend_gpio4_value = smart_gpio_get_value(pdata->control_gpio4);

	smart_gpio_set_value(pdata->control_gpio1, pdata->control_gpio1_suspend_value);
	smart_gpio_set_value(pdata->control_gpio2, pdata->control_gpio2_suspend_value);
	smart_gpio_set_value(pdata->control_gpio3, pdata->control_gpio3_suspend_value);
	smart_gpio_set_value(pdata->control_gpio4, pdata->control_gpio4_suspend_value);

	return 0;
}

static int usb_switch_resume(struct platform_device *pdev)
{
	int ret = 0;
	struct usb_switch_platform_data *pdata = pdev->dev.platform_data;

	dev_info(sdev.dev, "%s entry.\n", __func__);

	ret = blockmux_set(usbswitch_iomux_block, usbswitch_block_config, NORMAL);
	if (0 > ret) {
		printk(KERN_ERR "%s: set iomux to gpio normal error", __FUNCTION__);
	}

	smart_gpio_set_value(pdata->control_gpio1, before_suspend_gpio1_value);
	smart_gpio_set_value(pdata->control_gpio2, before_suspend_gpio2_value);
	smart_gpio_set_value(pdata->control_gpio3, before_suspend_gpio3_value);
	smart_gpio_set_value(pdata->control_gpio4, before_suspend_gpio4_value);

	return 0;
}
#else
#define usb_switch_suspend       NULL
#define usb_switch_resume        NULL
#endif /* CONFIG_PM */

static int usb_switch_probe(struct platform_device *pdev)
{
	struct usb_switch_platform_data *pdata = pdev->dev.platform_data;
	int ret = 0;

	printk(KERN_INFO "usb_switch_probe entry.\n");

	if (!pdata)
		return -EBUSY;

	sdev.pdata = pdata;
	sdev.name = pdata->name;

	usbswitch_iomux_block = iomux_get_block(IOMUX_BLOCK_NAME);
	if (!usbswitch_iomux_block) {
		printk(KERN_ERR "%s: get iomux block error", __FUNCTION__);
		return -ENODEV;
	}

	usbswitch_block_config = iomux_get_blockconfig(IOMUX_BLOCK_NAME);
	if (!usbswitch_block_config) {
		printk(KERN_ERR "%s: get block config error", __FUNCTION__);
		return -ENODEV;
	}

	INIT_WORK(&sdev.work, usb_switch_work);

	ret = switch_usb_dev_register(&sdev);
	if (ret < 0)
		goto err_switch_usb_dev_register;

	if (pdata->modem1_supported)
	{
		ret = gpio_request(pdata->modem_int_gpio1, "usb_sw_interrupt1");
		if (ret < 0)
			goto err_request_int_gpio1;

		ret = gpio_direction_input(pdata->modem_int_gpio1);
		if (ret < 0)
			goto err_set_gpio_input1;

		sdev.irq1 = gpio_to_irq(pdata->modem_int_gpio1);
		if (sdev.irq1 < 0) {
			ret = sdev.irq1;
			goto err_detect_irq_num_failed1;
		}

		ret = request_irq(sdev.irq1, int_gpio_irq_handler,
				  pdata->irq_flags, "usb_sw_interrupt1", &sdev);
		if (ret < 0)
			goto err_request_irq1;

		/* disable irq interrupt */
		disable_irq(sdev.irq1);
	}

	if (pdata->modem2_supported)
	{
		ret = gpio_request(pdata->modem_int_gpio2, "usb_sw_interrupt2");
		if (ret < 0)
			goto err_request_int_gpio2;

		ret = gpio_direction_input(pdata->modem_int_gpio2);
		if (ret < 0)
			goto err_set_gpio_input2;

		sdev.irq2 = gpio_to_irq(pdata->modem_int_gpio2);
		if (sdev.irq2 < 0) {
			ret = sdev.irq2;
			goto err_detect_irq_num_failed2;
		}

		ret = request_irq(sdev.irq2, int_gpio_irq_handler,
				  pdata->irq_flags, "usb_sw_interrupt2", &sdev);
		if (ret < 0)
			goto err_request_irq2;

		/* disable irq interrupt */
		disable_irq(sdev.irq2);
	}

	return 0;

err_request_irq2:
err_detect_irq_num_failed2:
err_set_gpio_input2:
	smart_gpio_free(pdata->modem_int_gpio2);
err_request_int_gpio2:
err_request_irq1:
err_detect_irq_num_failed1:
err_set_gpio_input1:
	smart_gpio_free(pdata->modem_int_gpio1);
err_request_int_gpio1:
	switch_usb_dev_unregister(&sdev);
err_switch_usb_dev_register:

	return ret;
}

static int __devexit usb_switch_remove(struct platform_device *pdev)
{
	struct usb_switch_platform_data *pdata = pdev->dev.platform_data;

	if (!pdata) {
		return -EINVAL;
	}

	cancel_work_sync(&sdev.work);
	smart_gpio_free(pdata->modem_int_gpio1);
	smart_gpio_free(pdata->modem_int_gpio2);
	switch_usb_dev_unregister(&sdev);
	free_irq(sdev.irq1, &sdev);
	free_irq(sdev.irq2, &sdev);

	return 0;
}

static struct platform_driver usb_switch_driver = {
	.probe		= usb_switch_probe,
	.remove		= __devexit_p(usb_switch_remove),
#ifdef CONFIG_PM
	.suspend	= usb_switch_suspend,
	.resume		= usb_switch_resume,
#endif
	.driver		= {
		.name	= "switch-usb",
		.owner	= THIS_MODULE,
	},
};

static int __init usb_switch_init(void)
{
	return platform_driver_register(&usb_switch_driver);
}

static void __exit usb_switch_exit(void)
{
	platform_driver_unregister(&usb_switch_driver);
}

module_init(usb_switch_init);
module_exit(usb_switch_exit);

MODULE_AUTHOR("Jake.chen");
MODULE_DESCRIPTION("USB Switch driver");
MODULE_LICENSE("GPL");
