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
 * @file sii_hal.h
 *
 * @brief Defines the hardware / OS abstraction layer API used by Silicon
 *        Image MHL drivers.
 *
 * $Author: Dave Canfield
 * $Rev: $
 * $Date: Jan. 24, 2011
 *
 *****************************************************************************/

#if !defined(SII_HAL_PRIV_H)
#define SII_HAL_PRIV_H

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>


#ifdef __cplusplus
extern "C" {
#endif  /* _defined (__cplusplus) */

        /***** macro definitions *****************************************************/
#ifdef MAKE_9244_DRIVER
#	define BASE_I2C_ADDR	0x76
#endif

#ifdef MAKE_8334_DRIVER
#	define BASE_I2C_ADDR   0x76
#endif

        //TB {
#ifdef MAKE_8240_DRIVER
#	define BASE_I2C_ADDR   0x76
#endif
        //TB }

#ifdef MAKE_9232_DRIVER
#	define BASE_I2C_ADDR	0x76
#endif


        /* Beagleboard GPIO pin numbers used */
#define W_INT_GPIO			80		/* transmitter interrupt */
#define M2U_VBUS_CTRL_M	999		/* USB power control */
#define W_RST_GPIO			81		/* transmitter reset */
#define MHL_VBUS_CTRL		999		/* MHL port power control */

#define PIN_NAME_INT    "mhl_int"   /* Pin name of Interrupt*/
#define PIN_NAME_RST    "mhl_rst"   /* Pin name of Reset*/

#define MAX_I2C_TRANSFER	255		/* Maximum # of bytes in a single I2c transfer */
#define MAX_I2C_MESSAGES	3		/* Maximum # of I2c message segments supported
        by SiiMasterI2cTransfer					   */

        /***** public type definitions ***********************************************/

        /** Type used internally within the HAL to encapsulate the state of the
         * MHL TX device. */
        typedef struct
        {
                struct	i2c_driver	driver;
                struct	i2c_client	*pI2cClient;
                fwIrqHandler_t		irqHandler;
                struct regulator *avcc;
        } mhlDeviceContext_t, *pMhlDeviceContext;



        /***** global variables used internally by HAL ********************************************/

        /** @brief If true, HAL has been initialized. */
        extern bool gHalInitedFlag;

        extern struct i2c_device_id gMhlI2cIdTable[2];

        /** @brief Maintain state info of the MHL TX device */
        extern mhlDeviceContext_t gMhlDevice;



        /***** public function prototypes ********************************************/


        /*****************************************************************************/
        /*
         *  @brief Check if HAL access is allowed.
         *
         *  Various HAL functions call this function to make sure the HAL has been
         *  properly initialized.
         *
         *  @return         HAL_RET_SUCCESS if the HAL has been initialized,
         *  				otherwise returns HAL_RET_NOT_INITIALIZED.
         *
         *****************************************************************************/
        halReturn_t HalInitCheck(void);



        /*****************************************************************************/
        /*
         *  @brief Check if I2c access is allowed.
         *
         *  If the Hal layer has been inited and the I2c device has been successfully
         *  opened, success is returned, otherwise an approprite HAL error code is
         *  returned.
         *
         *  @return         status (success or error code)
         *
         *****************************************************************************/
        halReturn_t I2cAccessCheck(void);


        /*****************************************************************************/
        /*
         * @brief Initialize HAL timer support
         *
         *  This function is responsible for initializing all the timer counters
         *  used by the driver.
         *
         *  @return         Nothing.
         *
         *****************************************************************************/
        void HalTimerInit(void);


        /*****************************************************************************/
        /**
         * @brief Terminate HAL timer support.
         *
         *  This function is responsible for stopping any timers that may be running.
         *
         *  @return         Nothing.
         *
         *****************************************************************************/
        void HalTimerTerm(void);


        /*****************************************************************************/
        /*
         * @brief Initialize HAL GPIO support
         *
         *  This function is responsible for acquiring and configuring GPIO pins
         *  needed by the driver.
         *
         *  @return         status (success or error code)
         *
         *****************************************************************************/
        halReturn_t HalGpioInit(void);


        /*****************************************************************************/
        /**
         * @brief Terminate HAL GPIO support.
         *
         *  This function is responsible for releasing any GPIO pin resources
         *  acquired by HalGpioInit
         *
         * @return         Success status if GPIO support was previously initialized,
         * 				   error status otherwise.
         *****************************************************************************/
        halReturn_t HalGpioTerm(void);



#ifdef __cplusplus
}
#endif  /* _defined (__cplusplus) */

#endif /* _defined (SII_HAL_PRIV_H) */
