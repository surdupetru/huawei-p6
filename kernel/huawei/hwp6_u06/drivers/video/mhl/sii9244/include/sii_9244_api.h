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

#ifndef __SII_9244_API_H__
#define __SII_9244_API_H__

#include <linux/types.h>

/* C99 defined data types.  */
typedef char Bool;
typedef bool bool_tt;

#define LOW                     (0)
#define HIGH                    (1)

#define DCDC_OFF	(0)
#define DCDC_ON		(1)

/*Generic Masks*/
#define _ZERO		      (0x00)
#define BIT0                   (0x01)
#define BIT1                   (0x02)
#define BIT2                   (0x04)
#define BIT3                   (0x08)
#define BIT4                   (0x10)
#define BIT5                   (0x20)
#define BIT6                   (0x40)
#define BIT7                   (0x80)

/*gpio_80 INT*/
#define MHL_PIN "t28"

/* This is the time in milliseconds we poll what we poll.*/
#define MONITORING_PERIOD	(50)

extern bool_tt vbusPowerState;
extern uint16_t Int_count;

static inline bool_tt get_mhl_ci2ca_value(void)
{
	return true;
}

void HalTimerWait(uint16_t m_sec);

/* AppMhlTxDisableInterrupts
This function or macro is invoked from MhlTx driver to secure the processor
before entering into a critical region.
Application module must provide this function.
*/
extern void AppMhlTxDisableInterrupts(void);

/*
AppMhlTxRestoreInterrupts
This function or macro is invoked from MhlTx driver to secure the processor
before entering into a critical region.
Application module must provide this function.
*/
extern void AppMhlTxRestoreInterrupts(void);

/*
AppVbusControl
This function or macro is invoked from MhlTx driver to ask application to
control the VBUS power. If powerOn is sent as non-zero, one should assume
peer does not need power so quickly remove VBUS power.
if value of "powerOn" is 0, then application must turn the VBUS power on
within 50ms of this call to meet MHL specs timing.
Application module must provide this function.
*/
extern void AppVbusControl(bool_tt powerOn);

/* VBUS power check for supply or charge */
#define VBUS_POWER_CHK ENABLE

/*
Software power states are a little bit different than the hardware states but
a close resemblance exists.

D3 matches well with hardware state. In this state we receive RGND interrupts
to initiate wake up pulse and device discovery

Chip wakes up in D2 mode and interrupts MCU for RGND. Firmware changes the 9244
into D0 mode and sets its own operation mode as POWER_STATE_D0_NO_MHL because
MHL connection has not yet completed.

For all practical reasons, firmware knows only two states of hardware - D0 and D3.

We move from POWER_STATE_D0_NO_MHL to POWER_STATE_D0_MHL only when MHL connection
is established.*/
#define    POWER_STATE_D3			3
#define    POWER_STATE_D0_NO_MHL	2
#define    POWER_STATE_D0_MHL		0
#define    POWER_STATE_FIRST_INIT	0xFF

#endif
