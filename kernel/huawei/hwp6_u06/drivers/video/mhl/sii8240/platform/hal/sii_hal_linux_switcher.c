/*

SiI8240 Linux Driver

Copyright (C) 2011-2012 Huawei Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation version 2.

This program is distributed .as is. WITHOUT ANY WARRANTY of any
kind, whether express or implied; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the
GNU General Public License for more details.

*/


/**
* @file sii_hal_linux_switcher.c
*
* @brief Linux usb switcher support needed by Silicon Image
*        MHL devices.
*
* $Author: Manfred Yang
* $Rev: $
* $Date: Jun. 9, 2012
*
*****************************************************************************/
#define SII_HAL_LINUX_SWITCHER_C

/***** #include statements ***************************************************/
#include "si_common.h"
#include "sii_hal.h"
#include "sii_hal_priv.h"
#include "si_c99support.h"
#include "si_osdebug.h"

#include <linux/mux.h>
#ifdef SHARE_SWITCHER_WITH_USB
#include <linux/switch_usb.h>
#endif

/***** local macro definitions ***********************************************/


/***** local type definitions ************************************************/


/***** local variable declarations *******************************************/


/***** local function prototypes *********************************************/


/***** global variable declarations *******************************************/


bool	isMhlConnected	= false;		// MHL Connected flag


/***** global variable declarations *******************************************/
bool GetMhlCableConnect(void)
{
        return isMhlConnected;
}

/***** local functions *******************************************************/


/*****************************************************************************/
/**
* @brief Usb and MHL DP DN switcher operation.
*
*****************************************************************************/

#ifdef SHARE_SWITCHER_WITH_USB
halReturn_t HalSwitcherSwitchToMhl(bool isSwitchToMhl)
{
        isMhlConnected = isSwitchToMhl;

        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                        "HalSwitcherSwitchToMhl: isSwitchToMhl = %d \n",isSwitchToMhl);

        if (isMhlConnected)
        {
                switch_usb_set_state_through_fs(USB_TO_MHL);
        }
        else
        {
                switch_usb_set_state_through_fs(USB_TO_AP);
        }
        return HAL_RET_SUCCESS;
}
#else
/* control switcher by MHL self using gpio */
#define SWITCHER_SEL0 174
#define SWITCHER_SEL1 42
#define PIN_NAME_SEL0 "h26"
#define PIN_NAME_SEL1 "e16"
static halReturn_t HalSwitcherSetResource(void)
{
        struct iomux_pin        *pSel0Pin = NULL; //interrupt pin "T28"
        struct iomux_pin        *pSel1Pin = NULL; //Reset pin "T26"
        /*Set Sel0 GPIO resource*/
        pSel0Pin = iomux_get_pin(PIN_NAME_SEL0);
        if( !pSel0Pin )
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "HalSwitcherSetResource: get pin of GPIO %d failed\n",
                                SWITCHER_SEL0);
                return HAL_RET_FAILURE;
        }

        /*Set Sel1 GPIO resource*/
        pSel1Pin = iomux_get_pin(PIN_NAME_SEL1);
        if( !pSel1Pin )
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "HalSwitcherSetResource get pin of GPIO %d failed\n",
                                SWITCHER_SEL1);
                return HAL_RET_FAILURE;
        }

        return HAL_RET_SUCCESS;
}

halReturn_t HalSwitcherSwitchToMhl(bool isSwitchToMhl)
{
        int ret;
        isMhlConnected = isSwitchToMhl;
        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                        "HalSwitcherSwitchToMhl: Enter, The isMhlConnected =%d \n",isMhlConnected);

        HalSwitcherSetResource();

        ret = gpio_request(SWITCHER_SEL0, "usb_sel0_gpio");
        if (ret < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "HalSwitcherSwitchToMhl: Failed to Request Sel0\n");
                return HAL_RET_FAILURE;
        }
        ret = gpio_request(SWITCHER_SEL1, "usb_sel1_gpio");
        if (ret < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "HalSwitcherSwitchToMhl: Failed to Request Sel1\n");
                return HAL_RET_FAILURE;
        }
        if(isSwitchToMhl)
        {
                gpio_set_value(SWITCHER_SEL0, 0);
                gpio_set_value(SWITCHER_SEL1, 1);
        }
        else
        {
                gpio_set_value(SWITCHER_SEL0, 0);
                gpio_set_value(SWITCHER_SEL1, 0);
        }

        gpio_free(SWITCHER_SEL0);
        gpio_free(SWITCHER_SEL0);

        return HAL_RET_SUCCESS;
}


#endif
