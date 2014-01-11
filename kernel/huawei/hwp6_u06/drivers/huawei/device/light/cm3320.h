/* include/linux/cm3320.h
 *
 * Copyright (C) 2012 Capella Microsystems Inc.
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

#ifndef __LINUX_CM3320_H
#define __LINUX_CM3320_H

struct cm3320_platform_data {
	int (*power)(int, uint8_t); /* power to the chip */
	uint16_t ALS_slave_address;
};

#endif
