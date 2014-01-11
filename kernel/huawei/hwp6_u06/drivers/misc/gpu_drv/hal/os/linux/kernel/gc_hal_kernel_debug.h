/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/




#ifndef __gc_hal_kernel_debug_h_
#define __gc_hal_kernel_debug_h_

#include <gc_hal_kernel_linux.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
****************************** OS-dependent Macros *****************************
\******************************************************************************/

typedef va_list gctARGUMENTS;

#define gcmkARGUMENTS_START(Arguments, Pointer) \
    va_start(Arguments, Pointer)

#define gcmkARGUMENTS_END(Arguments) \
    va_end(Arguments)

#define gcmkDECLARE_LOCK(__spinLock__) \
    static DEFINE_SPINLOCK(__spinLock__);

#define gcmkLOCKSECTION(__spinLock__) \
    spin_lock(&__spinLock__)

#define gcmkUNLOCKSECTION(__spinLock__) \
    spin_unlock(&__spinLock__)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#   define gcmkGETPROCESSID() \
        task_tgid_vnr(current)
#else
#   define gcmkGETPROCESSID() \
        current->tgid
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#   define gcmkGETTHREADID() \
        task_pid_vnr(current)
#else
#   define gcmkGETTHREADID() \
        current->pid
#endif

#define gcmkOUTPUT_STRING(String) \
    if(gckDEBUGFS_IsEnabled()) \
        gckDEBUGFS_Print(String);\
    else\
        printk(String); \
        touch_softlockup_watchdog()

#define gcmkSPRINTF(Destination, Size, Message, Value) \
    snprintf(Destination, Size, Message, Value)

#define gcmkSPRINTF2(Destination, Size, Message, Value1, Value2) \
    snprintf(Destination, Size, Message, Value1, Value2)

#define gcmkSPRINTF3(Destination, Size, Message, Value1, Value2, Value3) \
    snprintf(Destination, Size, Message, Value1, Value2, Value3)

#define gcmkVSPRINTF(Destination, Size, Message, Arguments) \
    vsnprintf(Destination, Size, Message, *(va_list *) &Arguments)

#define gcmkSTRCAT(Destination, Size, String) \
    strncat(Destination, String, Size)

/* If not zero, forces data alignment in the variable argument list
   by its individual size. */
#define gcdALIGNBYSIZE      1

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_kernel_debug_h_ */
