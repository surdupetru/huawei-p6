/*
 * SiIxxxx <Firmware or Driver>
 *
 * Copyright (C) 2011 Silicon Image Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the
 * GNU General Public License for more details.
*/

#ifndef __MHL_H__
#define __MHL_H__

#ifdef CONFIG_MHL_SII9244
#define MHL_SII9244
#endif

#ifdef CONFIG_MHL_SII8240
#define MHL_SII8240
#endif


#ifdef MHL_SII9244
#include <mach/gpio.h>
#define	MHL_SII9244_PAGE0_ADDR			(0x3B)
#define	MHL_SII9244_PAGE1_ADDR			(0x3F)
#define	MHL_SII9244_PAGE2_ADDR			(0x4B)
#define	MHL_SII9244_CBUS_ADDR			(0x66)

#define	MHL_GPIO_WAKE_UP			        (GPIO_9_7)
#define	MHL_GPIO_INT					(GPIO_10_0)
#define	MHL_GPIO_RESET				(GPIO_10_1)
#define    MHL_NEW_BROAD

#ifdef SHARE_SWITCHER_WITH_USB
#define	MHL_GPIO_SWITCH_1			(GPIO_18_0)
#define	MHL_GPIO_SWITCH_2			(GPIO_21_6)
#endif

#define	MHL_GPIO_DCDC_MODE			(GPIO_15_4)

struct mhl_platform_data 
{
	int gpio_reset;
	int gpio_wake_up;
	int gpio_int;
};

#endif



#if defined(MHL_SII9244)||defined(MHL_SII8240)

#define SUPPORT_MHL     //export a marco, tell the other module it support mhl 

int      SiiMhlReset(void);
bool    GetMhlCableConnect(void);
void    SiiCBusIDSwitcherOpen(void);


#endif

#endif