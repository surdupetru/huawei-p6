/*
 *  Switch_usb class driver
 *
 * Copyright (C) 2011 Google, Inc.
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

#ifndef __LINUX_SWITCH_USB_H__
#define __LINUX_SWITCH_USB_H__

#define GPIO_HI		                        1
#define	GPIO_LOW	                        0

enum USB_SWITCH_STATE
{
	USB_TO_AP = 0,
	USB_TO_MODEM1 = 1,
	USB_TO_MODEM2 = 2,
	USB_TO_MODEM1_DOWNLOAD = 3,
	USB_TO_MODEM2_DOWNLOAD = 4,
	USB_TO_MHL = 5,
	USB_OFF = 6,
};

struct usb_switch_platform_data {
	const char	*name;
	bool		enable;
	bool		modem1_supported;
	unsigned	modem_loadswitch1;		//must, if modem1 is supported
	unsigned	modem_int_gpio1;		//must, if modem1 is supported
	bool		modem2_supported;
	unsigned	modem_loadswitch2;		//must, if modem2 is supported
	unsigned	modem_int_gpio2;		//must, if modem2 is supported
	bool		mhl_supported;
	int			control_gpio1;			//optional, -1 means this pin is unavailable on this board
	int			control_gpio2;			//optional, -1 means this pin is unavailable on this board
	int			control_gpio3;			//optional, -1 means this pin is unavailable on this board
	int			control_gpio4;			//optional, -1 means this pin is unavailable on this board
	unsigned	control_gpio1_suspend_value;
	unsigned	control_gpio2_suspend_value;
	unsigned	control_gpio3_suspend_value;
	unsigned	control_gpio4_suspend_value;
	unsigned	control_gpio1_boot_ap_value;
	unsigned	control_gpio2_boot_ap_value;
	unsigned	control_gpio3_boot_ap_value;
	unsigned	control_gpio4_boot_ap_value;
	unsigned	control_gpio1_off_value;
	unsigned	control_gpio2_off_value;
	unsigned	control_gpio3_off_value;
	unsigned	control_gpio4_off_value;
	unsigned	control_gpio1_modem1_value;
	unsigned	control_gpio2_modem1_value;
	unsigned	control_gpio3_modem1_value;
	unsigned	control_gpio4_modem1_value;
	unsigned	control_gpio1_modem2_value;
	unsigned	control_gpio2_modem2_value;
	unsigned	control_gpio3_modem2_value;
	unsigned	control_gpio4_modem2_value;
	unsigned	control_gpio1_mhl_value;
	unsigned	control_gpio2_mhl_value;
	unsigned	control_gpio3_mhl_value;
	unsigned	control_gpio4_mhl_value;
	unsigned long	irq_flags;
};

struct switch_usb_dev {
	const char	*name;
	struct device	*dev;
	int		state;
	int		irq1;
	int		irq2;
	struct work_struct      work;
	struct usb_switch_platform_data *pdata;
};

extern int switch_usb_dev_register(struct switch_usb_dev *sdev);
extern void switch_usb_dev_unregister(struct switch_usb_dev *sdev);
extern int switch_usb_set_state(struct switch_usb_dev *sdev, int state);
extern void switch_usb_set_state_through_fs(int state);
extern int switch_usb_register_notifier(struct notifier_block *nb);
extern int switch_usb_unregister_notifier(struct notifier_block *nb);

#endif /* __LINUX_SWITCH_USB_H__ */
