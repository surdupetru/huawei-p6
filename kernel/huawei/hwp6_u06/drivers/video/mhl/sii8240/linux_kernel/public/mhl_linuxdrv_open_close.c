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
 * @file mhl_linuxdrv_open_close.c
 *
 * @brief This module provides the device open/release functionality of the
 *  Linux driver for Silicon Image MHL transmitters.
 *
 * $Author: Dave Canfield
 * $Rev: $
 * $Date: Jan. 20, 2011
 *
 *****************************************************************************/

#define MHL_LINUXDRV_MAIN_C

/***** #include statements ***************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include "si_memsegsupport.h"
#include "mhl_linuxdrv.h"
#include "si_mhl_defs.h"  //TB - added
#include "si_mhl_tx_api.h"
#include "si_osdebug.h"


/***** local macro definitions ***********************************************/

/***** local type definitions ************************************************/

/***** local variable declarations *******************************************/

static bool	bTxOpen = false;	// true if transmitter has been opened

/***** global variable declarations *******************************************/

/***** local functions *******************************************************/


/**
 *  @brief Open the MHL transmitter device
 *
 *  Called by the system in response to a user mode application calling open()
 *  on the MHL transmitter device exposed by the driver.
 *
 *  This driver only allows access by one process at a time.  So all that
 *  needs to be done here is to check to make sure that the driver isn't
 *  already being used by another process.
 *
 *  @param[in]		pInode	pointer to inode representing transmitter device
 *  @param[in]		pFile	pointer to file descriptor created by open
 *
 *  @return     0 if successful, negative error code otherwise
 *
 *****************************************************************************/
int32_t SiiMhlOpen(struct inode *pInode, struct file *pFile)
{

        if(bTxOpen)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "Driver already open, failing open request\n");
                return -EBUSY;
        }

        bTxOpen = true;		// flag driver has been opened
        return 0;
}


/**
 *  @brief Close the MHL transmitter device
 *
 *  This is the file operation handler for release requests received from the
 *  system.  It is called when the last process calls close() on the File
 *  structure created by open().
 *
 *  Since this driver is only concerned with restricting access to one process
 *  at at time all that needs to be done here is to flag that the driver is no
 *  longer open by any process.
 *
 *  @param[in]		pInode	pointer to inode representing transmitter device
 *  @param[in]		pFile	pointer to file descriptor being closed.
 *
 *  @return		Always returns 0 to indicate success.
 *
 *****************************************************************************/
int32_t SiiMhlRelease(struct inode *pInode, struct file *pFile)
{
        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "Close %s\n", MHL_DRIVER_NAME);

        bTxOpen = false;	// flag driver has been closed

        return 0;
}
