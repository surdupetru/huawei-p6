
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
//***************************************************************************
//!file     si_timer_cfg.h
//!brief    Silicon Image timer definitions header.


//
// put debug channel information here
//------------------------------------------------------------------------------
// Array of timer values
//------------------------------------------------------------------------------
// Timers - Target system uses these timers
typedef enum TimerId
{
        TIMER_0 = 0		// DO NOT USE - reserved for TimerWait()
                  ,TIMER_POLLING		// Reserved for main polling loop
        ,TIMER_COUNT			// MUST BE LAST!!!!
} timerId_e;

#define T_MONITORING_PERIOD		200

void    SiiMhlTxDrvTimer_0CallBack(void *pArg);
void    SiiMhlTxDrvTimerPollingCallBack(void *pArg);

#define NAME_AND_POINTER(pointer) #pointer,pointer


// the order of this initialization list must match the order of timerId_e
#define SI_DRV_TIMER_INIT {NAME_AND_POINTER(SiiMhlTxDrvTimer_0CallBack)            ,NULL} \
                         ,{NAME_AND_POINTER(SiiMhlTxDrvTimerPollingCallBack)       ,NULL}

