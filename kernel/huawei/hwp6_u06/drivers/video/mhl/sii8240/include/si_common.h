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
 * !file     si_common.h
 * !brief    Silicon Image common definitions header.
 *
*/


#ifndef __SI_COMMON_H__
#define __SI_COMMON_H__
#include "si_buildflags.h"
#include "si_c99support.h"
#include "si_memsegsupport.h" //TB
#include "si_osdebug.h"
#include "si_app_gpio.h"
#include "si_app_devcap.h"
#include "si_bitdefs.h"
#include "si_infoframe.h" //TB-JB

#if defined(__KERNEL__)
#include "sii_hal.h"
#else
#include "si_debug.h"
#endif

// Standard result codes are in the range of 0 - 4095
typedef enum _SiiResultCodes_t
{
        SII_SUCCESS      = 0,           // Success.
        SII_ERR_FAIL,                   // General failure.
        SII_ERR_INVALID_PARAMETER,      //
        SII_ERR_IN_USE,                 // Module already initialized.
        SII_ERR_NOT_AVAIL,              // Allocation of resources failed.
} SiiResultCodes_t;

typedef enum
{
        SiiSYSTEM_NONE      = 0,
        SiiSYSTEM_SINK,
        SiiSYSTEM_SWITCH,
        SiiSYSTEM_SOURCE,
        SiiSYSTEM_REPEATER,
} SiiSystemTypes_t;

#define YES                         1
#define NO                          0

//------------------------------------------------------------------------------
//  Basic system functions
//------------------------------------------------------------------------------
#ifdef NEVER
uint8_t SiiTimerExpired( uint8_t timer );
long    SiiTimerElapsed( uint8_t index );
long    SiiTimerTotalElapsed ( void );
void    SiiTimerWait( uint16_t m_sec );
void    SiiTimerSet( uint8_t index, uint16_t m_sec );
void    SiiTimerInit( void );
#endif

#endif  // __SI_COMMON_H__
