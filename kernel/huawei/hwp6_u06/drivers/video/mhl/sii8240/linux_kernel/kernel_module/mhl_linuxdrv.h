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

/**
 * @file mhl_linuxdrv.h
 *
 * @brief Main header file of the MHL Tx driver.
 *
 * $Author: Dave Canfield
 * $Rev: $
 * $Date: Jan 20, 2011
 *
 *****************************************************************************/


#if !defined(MHL_LINUXDRV_H)
#define MHL_LINUXDRV_H

#include <linux/input.h>
#include <linux/kernel.h>
#include "sii_hal.h"


#ifdef __cplusplus
extern "C" {
#endif  /* _defined (__cplusplus) */

        /***** macro definitions *****************************************************/
#if defined(MAKE_9244_DRIVER)

#define MHL_PART_NAME "Sil-9244"
#define MHL_DRIVER_NAME "sii9244drv"
#define MHL_DRIVER_DESC "Sil-9244 MHL Tx Driver"
#define MHL_DEVICE_NAME "siI-9244"

#elif defined(MAKE_8334_DRIVER)

#define MHL_PART_NAME "Sil-8334"	// WARNING! Must match I2C_BOARD_INFO() setting in board-omap3beagle.c
#define MHL_DRIVER_NAME "sii8334drv"
#define MHL_DRIVER_DESC "Sil-8334 MHL Tx Driver"
#define MHL_DEVICE_NAME "siI-833x"

#elif defined(MAKE_8332_36_DRIVER)

#define MHL_PART_NAME "Sil-8332_36"	// WARNING! Must match I2C_BOARD_INFO() setting in board-omap3beagle.c
#define MHL_DRIVER_NAME "sii8332_36drv"
#define MHL_DRIVER_DESC "Sil-8332_36 MHL Tx Driver"
#define MHL_DEVICE_NAME "siI-8332_36"

#elif defined(MAKE_9232_DRIVER)

#define MHL_PART_NAME "Sil-9232"
#define MHL_DRIVER_NAME "sii9232drv"
#define MHL_DRIVER_DESC "Sil-9232 MHL Tx Driver"
#define MHL_DEVICE_NAME "siI-9232"

#elif defined(MAKE_8240_DRIVER)

#define MHL_PART_NAME "Sil-8240"	// WARNING! Must match I2C_BOARD_INFO() setting in board-omap3beagle.c
#define MHL_DRIVER_NAME "sii8240drv"
#define MHL_DRIVER_DESC "Sil-8240 MHL Tx Driver"
#define MHL_DEVICE_NAME "siI-8240"

#else

#error "Need to add name and description strings for new drivers here!"

#endif



#define MHL_DRIVER_MINOR_MAX   1
#define EVENT_POLL_INTERVAL_30_MS	30

        /***** public type definitions ***********************************************/

#define MHL_STATE_FLAG_CONNECTED	0x01	// MHL connection is estabished
#define MHL_STATE_FLAG_RCP_READY	0x02	// connection ready for RCP requests
#define MHL_STATE_FLAG_RCP_SENT		0x04	// last RCP event was a key send
#define MHL_STATE_FLAG_RCP_RECEIVED	0x08	// last RCP event was a key code receive
#define MHL_STATE_FLAG_RCP_ACK		0x10	// last RCP key code sent was ACK'd
#define MHL_STATE_FLAG_RCP_NAK		0x20	// last RCP key code sent was NAK'd

        static unsigned short rcpKeyCode[] =
        {
                KEY_MENU,/* 139 */
                KEY_SELECT,/* 0x161 */
                KEY_UP,/* 103 */
                KEY_LEFT,/* 105 */
                KEY_RIGHT,/* 106 */
                KEY_DOWN,/* 108 */
                KEY_EXIT,/* 174 */
                KEY_1, /* 0x02 */
                KEY_2,
                KEY_3,
                KEY_4,
                KEY_5,
                KEY_6,
                KEY_7,
                KEY_8,
                KEY_9, /* 0x10 */
                KEY_0,
                KEY_DOT,/* 52 */
                KEY_ENTER,/* 28 */
                KEY_CLEAR,/* 0x163 *//* DEL */
                KEY_SOUND,/* 0x213 */
                KEY_PLAY,/* 207 */
                KEY_PAUSE,/* 119 */
                KEY_STOP,/* 128 */
                KEY_FASTFORWARD,/* 208 */
                KEY_REWIND,/* 168 */
                KEY_EJECTCD,/* 161 */
                KEY_FORWARD,/* 159 */
                KEY_BACK,/* 158 */
                KEY_PLAYCD,/* 200 */
                KEY_PAUSECD,/* 201 */
                KEY_STOP,/* 128 */
                KEY_F1,/* 59 */
                KEY_F2,/* 60 */
                KEY_F3,
                KEY_F4,
                KEY_F5,/* 63 */
                KEY_CHANNELUP, /* 0x192 */
                KEY_CHANNELDOWN, /* 0x193 */
                KEY_MUTE, /* 113 */
                KEY_PREVIOUS, /* 0x19c */
                KEY_VOLUMEDOWN, /* 114 */
                KEY_VOLUMEUP, /* 115 */
                KEY_RECORD, /* 167 */
                KEY_REPLY,
                KEY_PLAYPAUSE,
                KEY_PREVIOUSSONG, /* add  */
                KEY_BACKSPACE, /* DEL */
                KEY_HOMEPAGE
        };

        typedef struct
        {
                struct task_struct	*pTaskStruct;
                struct device		*pDevice;			// pointer to the driver's device object
                struct input_dev	*pRcpDevice;		    // pointer to inputdevice that report rcp key code
                unsigned short		RcpKeyCode[ARRAY_SIZE(rcpKeyCode)]; //rcp cmd to key code that linux support
                uint8_t				flags;				// various state flags
                uint8_t				keyCode;			// last RCP key code sent or received.
                uint8_t				errCode;			// last RCP NAK error code received
                uint8_t				devCapOffset;		// last Device Capability register
                // offset requested
                uint8_t				pendingEvent;		// event data wait for retrieval
                uint8_t				pendingEventData;	// by user mode application

        } MHL_DRIVER_CONTEXT_T, *PMHL_DRIVER_CONTEXT_T;


        /***** global variables ********************************************/

        extern MHL_DRIVER_CONTEXT_T gDriverContext;


        /***** public function prototypes ********************************************/
        /**
         * \defgroup driver_public_api Driver Public API
         * @{
         */


        /**
         * @brief Open the MHL transmitter device
         */
        int32_t SiiMhlOpen(struct inode *pInode, struct file *pFile);


        /**
         * @brief Close the MHL transmitter device
         */
        int32_t SiiMhlRelease(struct inode *pInode, struct file *pFile);


        /**
         * @brief IOCTL handler
         */
        long SiiMhlIoctl(struct file *pFile, unsigned int ioctlCode,
                         unsigned long ioctlParam);

        /**
         * @}
         * end driver_public_api group
         */



#ifdef __cplusplus
}
#endif  /* _defined (__cplusplus) */

#endif /* _defined (MHL_LINUXDRV_H) */
