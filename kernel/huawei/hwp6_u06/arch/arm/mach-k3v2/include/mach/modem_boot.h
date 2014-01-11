/*
 * arch\arm\mach-k3v2\include\mach\modem_boot.h
 *
 * Adaptation to modem.  GPIO config info.
 *
 * Copyright (C) 2012 Huawei Device Co. Ltd.
 *
 * Author: June Yang <june.yang@huawei.com>
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _MODEM_BOOT_H_
#define _MODEM_BOOT_H_

#include <linux/platform_device.h>
#include <linux/gpio.h>

// Common definitions
extern const char* const modem_state_str[];

typedef enum ENUM_MODEM_STATE {
    MODEM_STATE_OFF=0,
    MODEM_STATE_POWER,
    MODEM_STATE_READY,
    MODEM_STATE_INVALID,
} ENUM_MODEM_STATE_T;

#define GPIO_CP_PM_INIT  0
#define GPIO_CP_PM_RDY  ( !(GPIO_CP_PM_INIT) )

#define GPIO_CP_SW_INIT  1
#define GPIO_CP_SW_RDY  ( !(GPIO_CP_SW_INIT) )

#define GPIO_WAKEUP_HOST_ACTIVE  0
#define GPIO_WAKEUP_HOST_INACTIVE  ( !(GPIO_WAKEUP_HOST_ACTIVE) )

#define GPIO_WAKEUP_SLAVE_ACTIVE  0
#define GPIO_WAKEUP_SLAVE_INACTIVE  ( !(GPIO_WAKEUP_SLAVE_ACTIVE) )

typedef struct STRUCT_MODEM_WAKEUP {
    volatile int cawake_value;
    int irq_flag;   //irq_flag: -1,IRQ request failed; true,IRQ enable; false,IRQ disabel;
    struct wake_lock wake_host_lock;
} STRUCT_MODEM_WAKEUP_T;

#define QUEUE_NUM   8
// Special definitions
typedef struct STRUCT_MODEM_BOOT {
    struct platform_device *pdev;
    int control_state;
    int mdm_state;   //ENUM_MODEM_STATE, spinlock protection
    struct work_struct uevent_work;
    char *envp[QUEUE_NUM][2];
    unsigned char head;
    unsigned char tail;
    spinlock_t lock;
    struct STRUCT_MODEM_WAKEUP mdm_wakeup;
} STRUCT_MODEM_BOOT_T;


#endif  //#ifndef _MODEM_BOOT_H_
