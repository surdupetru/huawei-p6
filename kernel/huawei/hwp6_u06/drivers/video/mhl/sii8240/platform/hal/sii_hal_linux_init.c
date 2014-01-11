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
 * @file sii_hal_linux_init.c
 *
 * @brief Initializes the Hardware Abstraction Layer (HAL) used by Silicon Image
 * 		  MHL Linux drivers.
 *
 * $Author: Dave Canfield
 * $Rev: $
 * $Date: Jan. 24, 2011
 *
 *****************************************************************************/

#define SII_HAL_LINUX_INIT_C

/***** #include statements ***************************************************/
#include <linux/i2c.h>
#include "sii_hal.h"
#include "sii_hal_priv.h"
#include "si_c99support.h"
#include "si_osdebug.h"

/***** local macro definitions ***********************************************/


/***** local type definitions ************************************************/


/***** local variable declarations *******************************************/


/***** local function prototypes *********************************************/


/***** global variable declarations *******************************************/

bool gHalInitedFlag = false;

/* @brief table used to hold device names of supported MHL devices */
struct i2c_device_id gMhlI2cIdTable[2];

/** @brief Semaphore used to prevent driver access from user mode from
 * colliding with the threaded interrupt handler */
//DEFINE_SEMAPHORE(gIsrLock);

mhlDeviceContext_t gMhlDevice;




/***** local functions *******************************************************/

/***** public functions ******************************************************/


/*****************************************************************************/
/**
 *  @brief Check if Hal has been properly initialized.
 *
 *****************************************************************************/
halReturn_t HalInitCheck(void)
{
        if (!(gHalInitedFlag))
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "Error: Hal layer not currently initialized!\n");
                return HAL_RET_NOT_INITIALIZED;
        }

        return HAL_RET_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Initialize the HAL layer module.
 *
 *****************************************************************************/
halReturn_t HalInit(void)
{
        halReturn_t	status;


        if (gHalInitedFlag)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "Error: Hal layer already inited!\n");
                return HAL_RET_ALREADY_INITIALIZED;
        }

        gMhlDevice.driver.driver.name = NULL;
        gMhlDevice.driver.id_table = NULL;
        gMhlDevice.driver.probe = NULL;
        gMhlDevice.driver.remove = NULL;

        gMhlDevice.pI2cClient = NULL;

        gMhlDevice.irqHandler = NULL;

        status = HalGpioInit();
        if(status != HAL_RET_SUCCESS)
        {
                return status;
        }
        gHalInitedFlag = true;

        return HAL_RET_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Terminate access to the hardware abstraction layer.
 *
 *****************************************************************************/
halReturn_t HalTerm(void)
{
        halReturn_t		retStatus;


        retStatus = HalInitCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        HalGpioTerm();

        gHalInitedFlag = false;

        return retStatus;
}




/*****************************************************************************/
/**
 * @brief Acquire the lock that prevents races with the interrupt handler.
 *
 *****************************************************************************/
halReturn_t HalAcquireIsrLock()
{
        halReturn_t		retStatus;
        //int				status;


        retStatus = HalInitCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }
        /*
        	status = down_interruptible(&gIsrLock);
        	if (status != 0)
        	{
            	SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
            			"HalAcquireIsrLock failed to acquire lock\n");
        		return HAL_RET_FAILURE;
        	}
        */
        return HAL_RET_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Release the lock that prevents races with the interrupt handler.
 *
 *****************************************************************************/
halReturn_t HalReleaseIsrLock()
{
        halReturn_t		retStatus;


        retStatus = HalInitCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        //up(&gIsrLock);

        return retStatus;
}
