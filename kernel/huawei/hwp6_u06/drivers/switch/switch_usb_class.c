/*
 *  drivers/switch/switch_usb_class.c
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
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/switch_usb.h>
#include <mach/gpio.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/usb/hiusb_android.h>
#include <linux/mhl/mhl.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/switch_usb.h>
#include <hsad/config_interface.h>
#include <hsad/config_mgr.h>
#include "smart_gpio.h"

#define USB_SWITCH_SYSFS	"/sys/devices/virtual/usbswitch/usbsw/swstate"

struct switch_usb_info {
    struct atomic_notifier_head charger_type_notifier_head;
    spinlock_t reg_flag_lock;
};

static struct switch_usb_info *p_switch_usb_info = NULL;

struct class *switch_usb_class;
static DEFINE_MUTEX(usb_switch_lock);

enum USB_IRQ_STATUS_E
{
    USB_IRQ_ENABLED = 0,
    USB_IRQ_DISABLED = 1,
};
static int usb_irq1_status = USB_IRQ_DISABLED;
static int usb_irq2_status = USB_IRQ_DISABLED;

static ssize_t usb_state_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct switch_usb_dev *sdev = (struct switch_usb_dev *)
		dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", sdev->state);
}

#define COMPOSITE_USB_SWITCH_MIN 900000
#define COMPOSITE_USB_SWITCH_MAX (COMPOSITE_USB_SWITCH_MIN + 99999)
#define USB_LOAD_SWITCH_1_ON (USB_OFF + 1)
#define USB_LOAD_SWITCH_1_OFF (USB_OFF + 2)
#define USB_LOAD_SWITCH_2_ON (USB_OFF + 3)
#define USB_LOAD_SWITCH_2_OFF (USB_OFF + 4)

static void usb_debug_switch(struct device *dev, int state)
{
	struct switch_usb_dev *sdev = (struct switch_usb_dev *)
				dev_get_drvdata(dev);

	if ((state >= COMPOSITE_USB_SWITCH_MIN)
		&& (state <= COMPOSITE_USB_SWITCH_MAX)
		)
	{
		switch_usb_set_state(sdev, (state / 10000) % 10);
		msleep((state % 1000) * 1000);
		switch_usb_set_state(sdev, (state / 1000) % 10);
	}

	switch (state)
	{
	case USB_LOAD_SWITCH_1_ON:
		smart_gpio_set_value(sdev->pdata->modem_loadswitch1, GPIO_HI);
		break;
	case USB_LOAD_SWITCH_1_OFF:
		smart_gpio_set_value(sdev->pdata->modem_loadswitch1, GPIO_LOW);
		break;
	case USB_LOAD_SWITCH_2_ON:
		smart_gpio_set_value(sdev->pdata->modem_loadswitch2, GPIO_HI);
		break;
	case USB_LOAD_SWITCH_2_OFF:
		smart_gpio_set_value(sdev->pdata->modem_loadswitch2, GPIO_LOW);
		break;
	}
}

static ssize_t usb_state_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct switch_usb_dev *sdev = (struct switch_usb_dev *)
				dev_get_drvdata(dev);
	int state;

	sscanf(buf, "%d", &state);

	usb_debug_switch(dev, state);

	if (switch_usb_set_state(sdev, state) < 0) {
		dev_err(sdev->dev, "%s: switch_usb_set_state err\n", __func__);
		return -EINVAL;
	}

	return size;
}

static void notify_switch_state(int state)
{
	atomic_notifier_call_chain(&p_switch_usb_info->charger_type_notifier_head,
				state, NULL);
	pr_info("[%s]Notify usb switch state: %d\n", __func__, state);
}

static void set_gpio_to_target_state(int state, struct switch_usb_dev *sdev)
{
	struct usb_switch_platform_data *pdata = sdev->pdata;
	BUG_ON(pdata == NULL);

	switch (state) {
	case USB_TO_AP:
		pr_info("%s: USB_TO_AP\n", __func__);

		smart_gpio_set_value(pdata->control_gpio1, pdata->control_gpio1_boot_ap_value);
		smart_gpio_set_value(pdata->control_gpio2, pdata->control_gpio2_boot_ap_value);
		smart_gpio_set_value(pdata->control_gpio3, pdata->control_gpio3_boot_ap_value);
		smart_gpio_set_value(pdata->control_gpio4, pdata->control_gpio4_boot_ap_value);
		break;
	case USB_TO_MODEM1:
		pr_info("%s: USB_TO_MODEM1\n", __func__);

		smart_gpio_set_value(pdata->control_gpio1, pdata->control_gpio1_modem1_value);
		smart_gpio_set_value(pdata->control_gpio2, pdata->control_gpio2_modem1_value);
		smart_gpio_set_value(pdata->control_gpio3, pdata->control_gpio3_modem1_value);
		smart_gpio_set_value(pdata->control_gpio4, pdata->control_gpio4_modem1_value);
		smart_gpio_set_value(pdata->modem_loadswitch1, GPIO_HI);

		enable_irq(sdev->irq1);
		usb_irq1_status = USB_IRQ_ENABLED;
		break;
	case USB_TO_MODEM1_DOWNLOAD:
		pr_info("%s: USB_TO_MODEM1_DOWNLOAD\n", __func__);

		smart_gpio_set_value(pdata->control_gpio1, pdata->control_gpio1_modem1_value);
		smart_gpio_set_value(pdata->control_gpio2, pdata->control_gpio2_modem1_value);
		smart_gpio_set_value(pdata->control_gpio3, pdata->control_gpio3_modem1_value);
		smart_gpio_set_value(pdata->control_gpio4, pdata->control_gpio4_modem1_value);
		smart_gpio_set_value(pdata->modem_loadswitch1, GPIO_HI);

		enable_irq(sdev->irq1);
		usb_irq1_status = USB_IRQ_ENABLED;
		// Call Modem1 download mode here
		break;
	case USB_TO_MODEM2:
		pr_info("%s: USB_TO_MODEM2\n", __func__);

		smart_gpio_set_value(pdata->control_gpio1, pdata->control_gpio1_modem2_value);
		smart_gpio_set_value(pdata->control_gpio2, pdata->control_gpio2_modem2_value);
		smart_gpio_set_value(pdata->control_gpio3, pdata->control_gpio3_modem2_value);
		smart_gpio_set_value(pdata->control_gpio4, pdata->control_gpio4_modem2_value);
		smart_gpio_set_value(pdata->modem_loadswitch2, GPIO_HI);

		enable_irq(sdev->irq2);
		usb_irq2_status = USB_IRQ_ENABLED;
		break;
	case USB_TO_MODEM2_DOWNLOAD:
		pr_info("%s: USB_TO_MODEM2_DOWNLOAD\n", __func__);

		smart_gpio_set_value(pdata->control_gpio1, pdata->control_gpio1_modem2_value);
		smart_gpio_set_value(pdata->control_gpio2, pdata->control_gpio2_modem2_value);
		smart_gpio_set_value(pdata->control_gpio3, pdata->control_gpio3_modem2_value);
		smart_gpio_set_value(pdata->control_gpio4, pdata->control_gpio4_modem2_value);
		smart_gpio_set_value(pdata->modem_loadswitch2, GPIO_HI);

		enable_irq(sdev->irq2);
		usb_irq2_status = USB_IRQ_ENABLED;
		// Call Modem2 download mode here
		break;
	case USB_TO_MHL:
		pr_info("%s: USB_TO_MHL\n", __func__);

		smart_gpio_set_value(pdata->control_gpio1, pdata->control_gpio1_mhl_value);
		smart_gpio_set_value(pdata->control_gpio2, pdata->control_gpio2_mhl_value);
		smart_gpio_set_value(pdata->control_gpio3, pdata->control_gpio3_mhl_value);
		smart_gpio_set_value(pdata->control_gpio4, pdata->control_gpio4_mhl_value);

		break;
	case USB_OFF:
		pr_info("%s: USB_OFF\n", __func__);

		if ( pdata->modem1_supported
		   && (usb_irq1_status == USB_IRQ_ENABLED)
		   )
		{
			disable_irq(sdev->irq1);
			usb_irq1_status = USB_IRQ_DISABLED;
		}

		if ( pdata->modem2_supported
		   && (usb_irq2_status == USB_IRQ_ENABLED)
		   )
		{
			disable_irq(sdev->irq2);
			usb_irq2_status = USB_IRQ_DISABLED;
		}

		smart_gpio_set_value(pdata->modem_loadswitch1, GPIO_LOW);
		smart_gpio_set_value(pdata->modem_loadswitch2, GPIO_LOW);
		smart_gpio_set_value(pdata->control_gpio1, pdata->control_gpio1_off_value);
		smart_gpio_set_value(pdata->control_gpio2, pdata->control_gpio2_off_value);
		smart_gpio_set_value(pdata->control_gpio3, pdata->control_gpio3_off_value);
		smart_gpio_set_value(pdata->control_gpio4, pdata->control_gpio4_off_value);
		break;
	default:
		// Parameter already checked, impossible to reach
		BUG_ON(1);
		break;
	}
}

int switch_usb_set_state(struct switch_usb_dev *sdev, int state)
{
	int ret = 0;
	struct usb_switch_platform_data *pdata = sdev->pdata;
	BUG_ON(pdata == NULL);
	mutex_lock(&usb_switch_lock);

	if ((state < USB_TO_AP) || (state > USB_OFF))
	{
		dev_info(sdev->dev, "%s: swstate[%d] is invalid\n",
			__func__, state);
		ret = -EINVAL;
		mutex_unlock(&usb_switch_lock);
		return ret;
	}

	if (sdev->state == state)
	{
		dev_info(sdev->dev, "%s: swstate[%d] is not changed, new "
			"swstate[%d]\n", __func__, sdev->state, state);
		ret = -EINVAL;
		mutex_unlock(&usb_switch_lock);
		return ret;
	}

	// check if target state is not supported on this board
	if (	((pdata->modem1_supported == false)
			&& ((state == USB_TO_MODEM1)
				|| (state == USB_TO_MODEM1_DOWNLOAD)
				)
			)
		|| ((pdata->modem2_supported == false)
			&& ((state == USB_TO_MODEM2)
				|| (state == USB_TO_MODEM2_DOWNLOAD)
				)
			)
		|| ((pdata->mhl_supported == false)
			&& (state == USB_TO_MHL)
			)
		)
	{
		dev_info(sdev->dev, "target state %d is not supported", state);
		ret = -EINVAL;
		mutex_unlock(&usb_switch_lock);
		return ret;
	}

	// if MHL is plugged in, only MHL state is allowed
	// if MHL is not plugged in, MHL state is not allowed
#if defined(MHL_SII9244) || defined(MHL_SII8240)
	if ( ((true == GetMhlCableConnect()) && (state != USB_TO_MHL))
	   ||((false == GetMhlCableConnect()) && (state == USB_TO_MHL))
		)
	{
		dev_info(sdev->dev, "%s: swstate[%d] is unavailable because MHL state is [%d]\n",
			__func__, state, GetMhlCableConnect());
		ret = -EINVAL;
		mutex_unlock(&usb_switch_lock);
		return ret;
	}
#endif

	// Switch to Hi-Impedence anyway before any switch
	set_gpio_to_target_state(USB_OFF, sdev);

	if (state != USB_TO_MHL)
	{
		msleep(1000);
	}

	if (state != USB_OFF)
	{
		set_gpio_to_target_state(state, sdev);
	}

	// update the state
	sdev->state = state;

	notify_switch_state(state);

	mutex_unlock(&usb_switch_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(switch_usb_set_state);

void switch_usb_set_state_through_fs(int state)
{
	mm_segment_t oldfs;
	struct file *filp;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	// this will not work for soft-irq and irq context
	filp = filp_open(USB_SWITCH_SYSFS, O_RDWR, 0);

	if (!filp || IS_ERR(filp))
	{
		/* Fs node not exist */
		pr_err("MHL: Failed to access usb switch\n");
		set_fs(oldfs);
		return;
	}

	switch (state)
	{
		case USB_TO_AP:
			filp->f_op->write(filp, "0", 1, &filp->f_pos);
			break;
		case USB_TO_MHL:
			filp->f_op->write(filp, "5", 1, &filp->f_pos);
			break;
		default:
			printk("Other state is not available through this API");
	}

	filp_close(filp, NULL);
	set_fs(oldfs);
}
EXPORT_SYMBOL_GPL(switch_usb_set_state_through_fs);

static DEVICE_ATTR(swstate, S_IRUGO | S_IWUSR, usb_state_show, usb_state_store);

static int create_switch_usb_class(void)
{
	if (!switch_usb_class) {
		switch_usb_class = class_create(THIS_MODULE, "usbswitch");
		if (IS_ERR(switch_usb_class))
			return PTR_ERR(switch_usb_class);
	}

	return 0;
}

static void dump_and_check_usb_switch_configuration(struct usb_switch_platform_data *pdata, int *ret)
{
	bool config_ret;

	config_ret = get_hw_config_bool("usb_switch/enable", &pdata->enable, NULL);
	if (config_ret == false) {
		pdata->enable = false;
	}

	config_ret = get_hw_config_bool("usb_switch/modem1_supported", &pdata->modem1_supported, NULL);
	if (config_ret == false) {
		pdata->modem1_supported = false;
	}

	config_ret = get_hw_config_bool("usb_switch/modem2_supported", &pdata->modem2_supported, NULL);
	if (config_ret == false) {
		pdata->modem2_supported = false;
	}

	config_ret = get_hw_config_bool("usb_switch/mhl_supported", &pdata->mhl_supported, NULL);
	if (config_ret == false) {
		pdata->mhl_supported = false;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio1", &pdata->control_gpio1, NULL);
	if (config_ret == false) {
		pdata->control_gpio1 = UNAVAILABLE_PIN;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio2", &pdata->control_gpio2, NULL);
	if (config_ret == false) {
		pdata->control_gpio2 = UNAVAILABLE_PIN;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio3", &pdata->control_gpio3, NULL);
	if (config_ret == false) {
		pdata->control_gpio3 = UNAVAILABLE_PIN;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio4", &pdata->control_gpio4, NULL);
	if (config_ret == false) {
		pdata->control_gpio4 = UNAVAILABLE_PIN;
	}

	config_ret = get_hw_config_int("usb_switch/modem_loadswitch1", &pdata->modem_loadswitch1, NULL);
	if (config_ret == false) {
		pdata->modem_loadswitch1 = UNAVAILABLE_PIN;
	}

	config_ret = get_hw_config_int("usb_switch/modem_loadswitch2", &pdata->modem_loadswitch2, NULL);
	if (config_ret == false) {
		pdata->modem_loadswitch2 = UNAVAILABLE_PIN;
	}

	config_ret = get_hw_config_int("usb_switch/modem_int_gpio1", &pdata->modem_int_gpio1, NULL);
	if (config_ret == false) {
		pdata->modem_int_gpio1 = UNAVAILABLE_PIN;
	}

	config_ret = get_hw_config_int("usb_switch/modem_int_gpio2", &pdata->modem_int_gpio2, NULL);
	if (config_ret == false) {
		pdata->modem_int_gpio2 = UNAVAILABLE_PIN;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio1_suspend_value", &pdata->control_gpio1_suspend_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio1_suspend_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio2_suspend_value", &pdata->control_gpio2_suspend_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio2_suspend_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio3_suspend_value", &pdata->control_gpio3_suspend_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio3_suspend_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio4_suspend_value", &pdata->control_gpio4_suspend_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio4_suspend_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio1_boot_ap_value", &pdata->control_gpio1_boot_ap_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio1_boot_ap_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio2_boot_ap_value", &pdata->control_gpio2_boot_ap_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio2_boot_ap_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio3_boot_ap_value", &pdata->control_gpio3_boot_ap_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio3_boot_ap_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio4_boot_ap_value", &pdata->control_gpio4_boot_ap_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio4_boot_ap_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio1_off_value", &pdata->control_gpio1_off_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio1_off_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio2_off_value", &pdata->control_gpio2_off_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio2_off_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio3_off_value", &pdata->control_gpio3_off_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio3_off_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio4_off_value", &pdata->control_gpio4_off_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio4_off_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio1_modem1_value", &pdata->control_gpio1_modem1_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio1_modem1_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio2_modem1_value", &pdata->control_gpio2_modem1_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio2_modem1_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio3_modem1_value", &pdata->control_gpio3_modem1_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio3_modem1_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio4_modem1_value", &pdata->control_gpio4_modem1_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio4_modem1_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio1_modem2_value", &pdata->control_gpio1_modem2_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio1_modem2_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio2_modem2_value", &pdata->control_gpio2_modem2_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio2_modem2_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio3_modem2_value", &pdata->control_gpio3_modem2_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio3_modem2_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio4_modem2_value", &pdata->control_gpio4_modem2_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio4_modem2_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio1_mhl_value", &pdata->control_gpio1_mhl_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio1_mhl_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio2_mhl_value", &pdata->control_gpio2_mhl_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio2_mhl_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio3_mhl_value", &pdata->control_gpio3_mhl_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio3_mhl_value = SMART_GPIO_NO_TOUCH;
	}

	config_ret = get_hw_config_int("usb_switch/control_gpio4_mhl_value", &pdata->control_gpio4_mhl_value, NULL);
	if (config_ret == false) {
		pdata->control_gpio4_mhl_value = SMART_GPIO_NO_TOUCH;
	}

	if (pdata->modem1_supported)
	{
		if ( ((pdata->modem_loadswitch1) == UNAVAILABLE_PIN)
			|| ((pdata->modem_int_gpio1) == UNAVAILABLE_PIN)
			)
		{
			pr_err("%s: invalid loadswitch %d or interrupt %d\n",
				__func__, pdata->modem_loadswitch1, pdata->modem_int_gpio1);
			*ret = -EINVAL;
			return;
		}
	}

	if (pdata->modem2_supported)
	{
		if ( ((pdata->modem_loadswitch2) == UNAVAILABLE_PIN)
			|| ((pdata->modem_int_gpio2) == UNAVAILABLE_PIN)
			)
		{
			pr_err("%s: invalid loadswitch %d or interrupt %d\n",
				__func__, pdata->modem_loadswitch2, pdata->modem_int_gpio2);
			*ret = -EINVAL;
			return;
		}
	}
}

int switch_usb_dev_register(struct switch_usb_dev *sdev)
{
	int ret = 0;
	struct usb_switch_platform_data *pdata = sdev->pdata;
	BUG_ON(pdata == NULL);

	dump_and_check_usb_switch_configuration(pdata, &ret);

	smart_gpio_request(pdata->control_gpio1, "control_gpio1", &ret);
	smart_gpio_request(pdata->control_gpio2, "control_gpio2", &ret);
	smart_gpio_request(pdata->control_gpio3, "control_gpio3", &ret);
	smart_gpio_request(pdata->control_gpio4, "control_gpio4", &ret);
	smart_gpio_request(pdata->modem_loadswitch1, "modem_loadswitch1", &ret);
	smart_gpio_request(pdata->modem_loadswitch2, "modem_loadswitch2", &ret);

	// switch to AP and turn off loadswitch by default
	smart_gpio_direction_output(pdata->control_gpio1, pdata->control_gpio1_boot_ap_value, &ret);
	smart_gpio_direction_output(pdata->control_gpio2, pdata->control_gpio2_boot_ap_value, &ret);
	smart_gpio_direction_output(pdata->control_gpio3, pdata->control_gpio3_boot_ap_value, &ret);
	smart_gpio_direction_output(pdata->control_gpio4, pdata->control_gpio4_boot_ap_value, &ret);
	smart_gpio_direction_output(pdata->modem_loadswitch1, GPIO_LOW, &ret);
	smart_gpio_direction_output(pdata->modem_loadswitch2, GPIO_LOW, &ret);

	if (ret != 0)
	{
		goto err_gpio_request;
	}

	if (!switch_usb_class) {
		ret = create_switch_usb_class();
		if (ret < 0)
		{
			goto err_gpio_request;
		}
	}

	sdev->dev = device_create(switch_usb_class, NULL,
		MKDEV(0, 0), NULL, sdev->name);
	if (IS_ERR(sdev->dev))
	{
		ret = PTR_ERR(sdev->dev);
		goto err_gpio_request;
	}

	ret = device_create_file(sdev->dev, &dev_attr_swstate);
	if (ret < 0)
		goto err_create_file;

	dev_set_drvdata(sdev->dev, sdev);
	sdev->state = USB_TO_AP;
	return 0;

err_create_file:
	device_destroy(switch_usb_class, MKDEV(0, 0));
	dev_err(sdev->dev, "switch_usb: Failed to register driver %s\n",
		sdev->name);
err_gpio_request:
	smart_gpio_free(pdata->modem_loadswitch2);
	smart_gpio_free(pdata->modem_loadswitch1);
	smart_gpio_free(pdata->control_gpio4);
	smart_gpio_free(pdata->control_gpio3);
	smart_gpio_free(pdata->control_gpio2);
	smart_gpio_free(pdata->control_gpio1);

	return ret;
}
EXPORT_SYMBOL_GPL(switch_usb_dev_register);

void switch_usb_dev_unregister(struct switch_usb_dev *sdev)
{
	struct usb_switch_platform_data *pdata = sdev->pdata;
	BUG_ON(pdata == NULL);

	device_remove_file(sdev->dev, &dev_attr_swstate);
	device_destroy(switch_usb_class, MKDEV(0, 0));
	dev_set_drvdata(sdev->dev, NULL);
	smart_gpio_free(pdata->modem_loadswitch2);
	smart_gpio_free(pdata->modem_loadswitch1);
	smart_gpio_free(pdata->control_gpio4);
	smart_gpio_free(pdata->control_gpio3);
	smart_gpio_free(pdata->control_gpio2);
	smart_gpio_free(pdata->control_gpio1);
}
EXPORT_SYMBOL_GPL(switch_usb_dev_unregister);


int switch_usb_register_notifier(struct notifier_block *nb)
{
	unsigned long flags;
	int ret = -1;
	if(p_switch_usb_info != NULL) {
		spin_lock_irqsave(&p_switch_usb_info->reg_flag_lock, flags);
		ret = atomic_notifier_chain_register(
				&p_switch_usb_info->charger_type_notifier_head, nb);
		spin_unlock_irqrestore(&p_switch_usb_info->reg_flag_lock, flags);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(switch_usb_register_notifier);

int switch_usb_unregister_notifier(struct notifier_block *nb)
{
	unsigned long flags;
	int ret = -1;
	if(p_switch_usb_info != NULL) {
		spin_lock_irqsave(&p_switch_usb_info->reg_flag_lock, flags);
		ret = atomic_notifier_chain_unregister(
				&p_switch_usb_info->charger_type_notifier_head, nb);
		spin_unlock_irqrestore(&p_switch_usb_info->reg_flag_lock, flags);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(switch_usb_unregister_notifier);

static int __init switch_usb_class_init(void)
{
	struct switch_usb_info *sui;

	if(NULL == p_switch_usb_info)
	{
		sui = kzalloc(sizeof(struct switch_usb_info), GFP_KERNEL);
		if(NULL == sui) {
			pr_err("kzalloc failed!\n");
			return -ENOMEM;
		}
		p_switch_usb_info = sui;
		ATOMIC_INIT_NOTIFIER_HEAD(&sui->charger_type_notifier_head);
		spin_lock_init(&sui->reg_flag_lock);
	}

	return create_switch_usb_class();
}

static void __exit switch_usb_class_exit(void)
{
	struct switch_usb_info *sui = p_switch_usb_info;

	class_destroy(switch_usb_class);

	if(NULL != sui)
	{
		kfree(sui);
		p_switch_usb_info = NULL;
	}
}

module_init(switch_usb_class_init);
module_exit(switch_usb_class_exit);

MODULE_AUTHOR("Jake.Chen");
MODULE_DESCRIPTION("Switch usb class driver");
MODULE_LICENSE("GPL");
