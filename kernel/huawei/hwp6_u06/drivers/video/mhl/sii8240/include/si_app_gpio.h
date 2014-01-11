/**********************************************************************************
 * Si8240 Linux Driver
 *
 * Copyright (C) 2011-2012 Silicon Image Inc.
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
 *
 **********************************************************************************/


/*
  @file  si_app_gpio.h

  @brief  Definitions for GPIO on Linux

*/

#define SB_NONE				(0)
#define SB_EPV5_MARK_II		(1)
#define SB_STARTER_KIT_X01	(2)

#if BUILD_CONFIG == 0 //(
#define TARGET_CHIP "8240"
#define TARGET_DEVICE_ID WOLV60_DEVICE_ID
#define SYSTEM_BOARD		(SB_EPV5_MARK_II)
#elif BUILD_CONFIG == 1 //)(
#define TARGET_CHIP "8240"
#define TARGET_DEVICE_ID WOLV60_DEVICE_ID
#define SYSTEM_BOARD		(SB_STARTER_KIT_X01)
#elif BUILD_CONFIG == 2 //)(
#define TARGET_CHIP "8558"
#define TARGET_DEVICE_ID BISHOP_DEVICE_ID
#define SYSTEM_BOARD		(SB_EPV5_MARK_II)
#elif BUILD_CONFIG == 3 //)(
#define TARGET_CHIP "8558"
#define TARGET_DEVICE_ID BISHOP_DEVICE_ID
#define SYSTEM_BOARD		(SB_STARTER_KIT_X01)
#endif

#if (SYSTEM_BOARD == SB_EPV5_MARK_II) //(
#define SiI_TARGET_STRING(s)       "SiI"s" EPV5 MARK II"
#elif (SYSTEM_BOARD == SB_STARTER_KIT_X01) //)(
#define SiI_TARGET_STRING(s)       "SiI"s" Starter Kit A00"
#else //)(
#error "Unknown SYSTEM_BOARD definition."
#endif //)
