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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include "si_common.h"
#include "mhl_linuxdrv.h"
#include "osal/include/osal.h"
#include "si_mhl_defs.h"  //TB
#include "si_mhl_tx_api.h"
#include "si_drvisrconfig.h"
#include "si_cra.h"
#include "si_hdmi_tx_lite_api.h"
//#include "si_mhl_tx.h"    //TB

#ifdef ENABLE_APP_DEBUG_PRINT //(

#define APP_DEBUG_PRINT(...) SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP,__VA_ARGS__)

#else //)(

#define APP_DEBUG_PRINT(...) /*nothing*/

#endif //)

#if 0 //( //TB-JB
void AppEnableOutput (void)
{
        //SiiMhlTxTmdsEnable();
}

void AppDisableOutput (void)
{
        //SiiMhlTxTmdsDisable();
}
#endif //) //TB-JB
void    SiiMhlTxDrvTimer_0CallBack(void *pArg)
{
        APP_DEBUG_PRINT("TIMER_0 expired\n");
        // do nothing
}
void    SiiMhlTxDrvTimerPollingCallBack(void *pArg)
{
        APP_DEBUG_PRINT("TIMER_POLLING expired\n");
        //PlatformGPIOSet(pinSinkVbusOn,PlatformGPIOGet(pinMhlVbusSense));

        HalTimerSet(TIMER_POLLING,T_MONITORING_PERIOD);
        // do nothing
}
void AppDisplayTimingEnumerationBegin(void)
{
        APP_DEBUG_PRINT(("Begin MHL display timing enumeration\n"));
}

void AppDisplayTimingEnumerationCallBack(uint16_t columns, uint16_t rows, uint8_t bitsPerPixel, uint32_t verticalRefreshRateInMilliHz,Mhl2VideoDescriptor_t mhl2VideoDescriptor)
{
        // avoid compiler warnings
        columns                     = columns;
        rows                        = rows;
        bitsPerPixel                = bitsPerPixel;
        verticalRefreshRateInMilliHz= verticalRefreshRateInMilliHz;
        mhl2VideoDescriptor         = mhl2VideoDescriptor;

        APP_DEBUG_PRINT(("%4d x %4d, %2d bpp at %lu Hz 3D - %16s %16s %16s\n\n"
                         ,columns
                         ,rows
                         ,(uint16_t)bitsPerPixel
                         ,verticalRefreshRateInMilliHz
                         ,mhl2VideoDescriptor.LeftRight      ?"Left/Right"      :""
                         ,mhl2VideoDescriptor.TopBottom      ?"Top/Bottom"      :""
                         ,mhl2VideoDescriptor.FrameSequential?"Frame Sequential":""
                        ));
}

void AppDisplayTimingEnumerationEnd(void)
{
        APP_DEBUG_PRINT(("End MHL display timing enumeration\n"));
}
