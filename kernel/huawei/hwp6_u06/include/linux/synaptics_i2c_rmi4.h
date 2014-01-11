/*
 * include/linux/synaptics_i2c_rmi4.h - platform data structure
 *
 * Copyright (C) 2008 Google, Inc.
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

#ifndef _LINUX_SYNAPTICS_I2C_RMI4_H
#define _LINUX_SYNAPTICS_I2C_RMI4_H

#include <linux/interrupt.h>
#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/irqs.h>

#define SYNAPTICS_RMI4_NAME "synaptics_rmi4"
#define SYNAPTICS_INIT_NAME "synaptics-ts"
#define SYNAPTICS_WQ_NAME "synaptics_wq"

#define SYNAPTICS_VBUS "ts-vbus"
#define SYNAPTICS_VDD "ts-vdd"


#define SYNAPTICS_GPIO_BLOCK_NAME "block_ts"

#define SYNAPTICS_RMI4_I2C_ADDR 0x70
#define SYNAPTICS_NO_FLIP	(0)
#define SYNAPTICS_FLIP_X	(1UL << 0)
#define SYNAPTICS_FLIP_Y	(1UL << 1)

struct rmi_function_info {

	/** This is the number of data points supported - for example, for
	 *  function $11 (2D sensor) the number of data points is equal to the number
	 *  of fingers - for function $19 (buttons)it is eqaul to the number of buttons
	 */
	unsigned char points_supported;

	/** This is the interrupt register and mask - needed for enabling the interrupts
	 *  and for checking what source had caused the attention line interrupt.
	 */
	unsigned char interrupt_offset;
	unsigned char interrupt_mask;

	unsigned char data_offset;
	unsigned char data_length;
};

enum f11_finger_status {
	f11_finger_none = 0,
	f11_finger_accurate = 1,
	f11_finger_inaccurate = 2,
};

struct f11_finger_data {
	enum f11_finger_status status;
	unsigned int x;
	unsigned int y;
	unsigned char z;
	unsigned int speed;
	bool active;
};

struct synaptics_rmi4 {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct hrtimer timer;
	struct work_struct  work;
	struct workqueue_struct *synaptics_wq;
	struct early_suspend early_suspend;
	struct regulator *vdd;
	struct regulator *vbus;
	struct iomux_block *gpio_block;
	struct block_config *gpio_block_config;

	bool use_irq;

	unsigned char data_reg;
	unsigned char data_length;
	unsigned char *data;
	struct i2c_msg data_i2c_msg[2];

	struct rmi_function_info f01;

	bool hasF11;
	struct rmi_function_info f11;
	int f11_has_gestures;
	int f11_has_relative;
	int f11_max_x, f11_max_y;
	unsigned char *f11_egr;
	bool hasEgrPinch;
	bool hasEgrPress;
	bool hasEgrFlick;
	bool hasEgrEarlyTap;
	bool hasEgrDoubleTap;
	bool hasEgrTapAndHold;
	bool hasEgrSingleTap;
	bool hasEgrPalmDetect;
    bool f11_has_Sensitivity_Adjust;
    bool is_support_multi_touch;
	struct f11_finger_data *f11_fingers;

	bool hasF19;
	struct rmi_function_info f19;

	bool hasF30;
	struct rmi_function_info f30;
};


/*define some tp type*/
#define LCD_X_QVGA			320
#define LCD_Y_QVGA			240
#define LCD_X_HVGA			320
#define LCD_Y_HVGA			480
#define LCD_X_WVGA			480
#define LCD_Y_WVGA			800
#define LCD_X_720P			720
#define LCD_Y_720P			1280
#define LCD_Y_ALL_HVGA		510  /* include virtual keys area */
#define LCD_Y_ALL_WVGA		882
#define LCD_Y_ALL_720P		1408

struct synaptics_rmi4_platform_data {
	int				irq;
	unsigned long	irq_flag;      /*IRQ type*/
	uint32_t		flip_flags;		/*Need to flip x/y axis or not*/
	int				x_res;			/*LCD x axis display resolution without Virtual key area.*/
	int				y_res;			/*LCD y axis display resolution without Virtual key area.*/
	int				y_all;          /*All LCD y axis resolution include Virtual key area.*/
	uint32_t		fuzz_x;			/*Fuzz of x axis*/
	uint32_t		fuzz_y;			/*Fuzz of y axis*/
	int				fuzz_p;			/*Fuzz of pressure*/
	int				fuzz_w;			/*Fuzz of touch width*/

	uint32_t		reset_pin;
};

#endif /* _LINUX_SYNAPTICS_I2C_RMI_H */

