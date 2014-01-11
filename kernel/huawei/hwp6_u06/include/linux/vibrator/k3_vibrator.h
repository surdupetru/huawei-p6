

/* include/linux/vibrator_k3.h
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

#ifndef _LINUX_VIBRATOR_K3_H
#define _LINUX_VIBRATOR_K3_H

#define	K3_VIBRATOR "vibrator"

#define	DR2_CTRL	(0x49 << 2)
#define	DR2_ISET	(0x4A << 2)

#define	CLEAR_ZERO	(0x00)
#define	PERIOD	(0x20) /* period 4ms, dirct current enable*/
#define PERIOD_QUICK (0x04)
#define SET_MODE	(0x01) /*direct mode*/
#define	ISET_POWER	(0xF0)  /* power */
#define ISET_POWERSTRONG (0xe0)

/*#define	DR2_DISABLE	(0xFE) /* linearity  disable */
#define	DR2_DISABLE	(0xFC) /*  direct current disable */


#define	TIMEOUT_MIN 	(35)

struct k3_vph_pwr_vol_vib_iset {
	int vph_voltage;
	int vreg_value;
};

#define TIMEOUT_MORE 	(50)

enum _motor_type{
    MOTOR_DEFAULT = 0,
    MOTOR_LDO,
    MOTOR_LINEAR
};

struct k3_vibrator_platform_data {
	u8 low_freq;
	u8 low_power;
	u8 mode;
	u8 high_freq;
	u8 high_power;
};
#endif


