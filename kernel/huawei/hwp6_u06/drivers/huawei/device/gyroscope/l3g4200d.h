/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name		: l3g4200d.h
* Authors		: MH - C&I BU - Application Team
*			: Carmine Iascone (carmine.iascone@st.com)
*			: Matteo Dameno (matteo.dameno@st.com)
* Version		: V 1.1 sysfs
* Date			: 2010/Aug/19
* Description		: L3G4200D digital output gyroscope sensor API
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
********************************************************************************
* REVISON HISTORY
*
* VERSION	| DATE		| AUTHORS		| DESCRIPTION
*******************************************************************************/
#ifndef __L3G4200D_H__
#define __L3G4200D_H__

#define L3G4200D_GYR_DEV_NAME	"l3g4200d_gyr"
#define	GYRO_POWER_NAME	"GYROSCOPE_VDD_SENSOR"

#define L3G4200D_GYR_FS_250DPS	0x00
#define L3G4200D_GYR_FS_500DPS	0x10
#define L3G4200D_GYR_FS_2000DPS	0x30

#define L3G4200D_GYR_ENABLED	1
#define L3G4200D_GYR_DISABLED	0
/** Maximum polled-device-reported rot speed value value in dps*/
#define FS_MAX			32768

#define MAX_VAL         11000
#define MIN_VAL         2800
#define ENABLE_SELFTEST 1
#define DISABLE_SELFTEST 0
#define TRUE_SELFTEST 1
#define ERR_SELFTEST -1

/* l3g4200d gyroscope registers */
#define WHO_AM_I        0x0F

#define CTRL_REG1       0x20    /* CTRL REG1 */
#define CTRL_REG2       0x21    /* CTRL REG2 */
#define CTRL_REG3       0x22    /* CTRL_REG3 */
#define CTRL_REG4       0x23    /* CTRL_REG4 */
#define CTRL_REG5       0x24    /* CTRL_REG5 */

/* CTRL_REG1 */
#define PM_OFF		0x00
#define PM_NORMAL	0x08
#define ENABLE_ALL_AXES	0x07
#define BW00		0x00
#define BW01		0x10
#define BW10		0x20
#define BW11		0x30
#define GYRO_ODR100		0x00  /* ODR = 100Hz */
#define GYRO_ODR200		0x40  /* ODR = 200Hz */
#define GYRO_ODR400		0x80  /* ODR = 400Hz */
#define GYRO_ODR800		0xC0  /* ODR = 800Hz */

/* CTRL_REG4 bits */
#define	FS_MASK				0x30

#define	SELFTEST_MASK			0x06
#define L3G4200D_SELFTEST_DIS		0x00
#define L3G4200D_SELFTEST_EN_POS	0x02
#define L3G4200D_SELFTEST_EN_NEG	0x04

#define AXISDATA_REG    0x28

#define FUZZ			0
#define FLAT			0
#define AUTO_INCREMENT		0x80

/* RESUME STATE INDICES */
#define	GYRO_RES_CTRL_REG1		0
#define	GYRO_RES_CTRL_REG2		1
#define	GYRO_RES_CTRL_REG3		2
#define	GYRO_RES_CTRL_REG4		3
#define	GYRO_RES_CTRL_REG5		4
#define	GYRO_RESUME_ENTRIES		5

/*#define DEBUG 1*/

/** Registers Contents */
#define WHOAMI_L3G4200D		0x00D4	/* Expected content for WAI register*/

/*
 * L3G4200D gyroscope data
 * brief structure containing gyroscope values for yaw, pitch and roll in
 * signed short
 */

struct l3g4200d_triple {
	short	x,	/* x-axis angular rate data. */
		y,	/* y-axis angluar rate data. */
		z;	/* z-axis angular rate data. */
};

struct output_rate {
	int poll_rate_ms;
	u8 mask;
};

struct l3g4200d_data {
	struct i2c_client *client;
	struct l3g4200d_gyr_platform_data *pdata;

	struct mutex lock;
	//struct delayed_work input_work;
	struct input_dev *input_dev;
	//struct input_polled_dev *input_poll_dev;
	int hw_initialized;
	int selftest_enabled;
	atomic_t enabled;

	u8 reg_addr;
	u8 resume_state[GYRO_RESUME_ENTRIES];
	int on_before_suspend;
	struct early_suspend early_suspend;
	struct hrtimer timer;
	struct work_struct  work;

};

#ifdef __KERNEL__
struct l3g4200d_gyr_platform_data {
	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);
	int poll_interval;
	int min_interval;

	u8 fs_range;

	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;
	int (*get_regulator)(struct device *);
	void (*put_regulator)(void);
};
#endif /* __KERNEL__ */
#endif  /* __L3G4200D_H__ */
