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
 * @file mhl_linuxdrv_ioctl.c
 *
 * @brief Implements the Ioctl interface of the Linux MHL Tx driver.
 *
 * $Author: $
 * $Rev: $
 * $Date: $
 *
 *****************************************************************************/

#define MHL_LINUXDRV_IOCTL_C

/***** #include statements ***************************************************/
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "si_memsegsupport.h"
#include "mhl_linuxdrv.h"
#include "sii_hal.h"
#include "mhl_linuxdrv_ioctl.h"
#include "si_mhl_defs.h"  //TB - added
#include "si_mhl_tx_api.h"
#include "si_osdebug.h"
#include "si_c99support.h"


/***** local macro definitions ***********************************************/

/***** local type definitions ************************************************/

/***** local variable declarations *******************************************/


/***** local function prototypes *********************************************/

/***** global variable declarations *******************************************/



/***** local functions *******************************************************/


/***** public functions ******************************************************/

/**
 * @addtogroup driver_public_api
 * @{
 */


/**
 * @brief IOCTL handler
 *
 * This function handles IOCTL requests received by the driver.
 *
 *  @param[in]	pFile
 *  @param[in]	ioctlCode	Code specifying IOCTL function to execute.
 *  @param[in]	ioctlParam	IOCTL control code specific parameter.
 *
 *  @return 	EINVAL if the ioctl code is unrecognized, otherwise the completion
 *  			status of the requested ioctl function is returned.
 *
 *****************************************************************************/
long SiiMhlIoctl(struct file *pFile, unsigned int ioctlCode,
                 unsigned long ioctlParam)
{
        mhlTxEventPacket_t		mhlTxEventPacket;
        mhlTxReadDevCap_t		mhlTxReadDevCap;
        mhlTxScratchPadAccess_t	mhlTxScratchPadAccess;
        ScratchPadStatus_e		scratchPadStatus;
        long					retStatus = 0;
        bool_t					status;

        if(HalAcquireIsrLock() != HAL_RET_SUCCESS)
        {
                return -ERESTARTSYS;
        }

        switch (ioctlCode)
        {
        case SII_MHL_GET_MHL_TX_EVENT:

                mhlTxEventPacket.event = gDriverContext.pendingEvent;
                mhlTxEventPacket.eventParam = gDriverContext.pendingEventData;

                if(copy_to_user((mhlTxEventPacket_t *)ioctlParam,
                                &mhlTxEventPacket, sizeof(mhlTxEventPacket_t)))
                {
                        retStatus = -EFAULT;
                        break;
                }

                gDriverContext.pendingEvent = MHL_TX_EVENT_NONE;
                gDriverContext.pendingEventData = 0;
                break;

        case SII_MHL_RCP_SEND:

                gDriverContext.flags &= ~(MHL_STATE_FLAG_RCP_RECEIVED |
                                          MHL_STATE_FLAG_RCP_ACK |
                                          MHL_STATE_FLAG_RCP_NAK);
                gDriverContext.flags |= MHL_STATE_FLAG_RCP_SENT;
                gDriverContext.keyCode = (uint8_t)ioctlParam;
                status = SiiMhlTxRcpSend((uint8_t)ioctlParam);
                break;

        case SII_MHL_RCP_SEND_ACK:

                status = SiiMhlTxRcpkSend((uint8_t)ioctlParam);
                break;

        case SII_MHL_RCP_SEND_ERR_ACK:

                status = SiiMhlTxRcpeSend((uint8_t)ioctlParam);
                break;

        case SII_MHL_GET_MHL_CONNECTION_STATUS:

                if(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY)
                {
                        mhlTxEventPacket.event = MHL_TX_EVENT_RCP_READY;
                }

                else if(gDriverContext.flags & MHL_STATE_FLAG_CONNECTED)
                {
                        mhlTxEventPacket.event = MHL_TX_EVENT_CONNECTION;
                }

                else
                {
                        mhlTxEventPacket.event = MHL_TX_EVENT_DISCONNECTION;
                }

                mhlTxEventPacket.eventParam = 0;

                if(copy_to_user((mhlTxEventPacket_t *)ioctlParam,
                                &mhlTxEventPacket, sizeof(mhlTxEventPacket_t)))
                {
                        retStatus = -EFAULT;
                }
                break;

        case SII_MHL_GET_DEV_CAP_VALUE:

                if(copy_from_user(&mhlTxReadDevCap, (mhlTxReadDevCap_t *)ioctlParam,
                                  sizeof(mhlTxReadDevCap_t)))
                {
                        retStatus = -EFAULT;
                        break;
                }

                // Make sure there is an MHL connection and that the requested
                // DEVCAP register number is valid.
                if(!(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY) ||
                                (mhlTxReadDevCap.regNum > 0x0F))
                {
                        retStatus = -EINVAL;
                        break;
                }

                retStatus = SiiTxGetPeerDevCapEntry(mhlTxReadDevCap.regNum,
                                                    &mhlTxReadDevCap.regValue);
                if(retStatus != 0)
                {
                        // Driver is busy and cannot provide the requested DEVCAP
                        // register value right now so inform caller they need to
                        // try again later.
                        retStatus = -EAGAIN;
                        break;
                }

                if(copy_to_user((mhlTxReadDevCap_t *)ioctlParam,
                                &mhlTxReadDevCap, sizeof(mhlTxReadDevCap_t)))
                {
                        retStatus = -EFAULT;
                }
                break;

        case SII_MHL_WRITE_SCRATCH_PAD:

                if(copy_from_user(&mhlTxScratchPadAccess,
                                  (mhlTxScratchPadAccess_t *)ioctlParam,
                                  sizeof(mhlTxScratchPadAccess_t)))
                {
                        retStatus = -EFAULT;
                        break;
                }

                // Make sure there is an MHL connection and that the requested
                // data transfer parameters don't exceed the address space of
                // the scratch pad.  NOTE: The address space reserved for the
                // Scratch Pad registers is 64 bytes but sources and sink devices
                // are only required to implement the 1st 16 bytes.
                if(!(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY) ||
                                (mhlTxScratchPadAccess.length < ADOPTER_ID_SIZE) ||
                                (mhlTxScratchPadAccess.length > MAX_SCRATCH_PAD_TRANSFER_SIZE) ||
                                (mhlTxScratchPadAccess.offset >
                                 (MAX_SCRATCH_PAD_TRANSFER_SIZE - ADOPTER_ID_SIZE)) ||
                                (mhlTxScratchPadAccess.offset + mhlTxScratchPadAccess.length >
                                 MAX_SCRATCH_PAD_TRANSFER_SIZE))
                {
                        retStatus = -EINVAL;
                        break;
                }

                scratchPadStatus =  SiiMhlTxRequestWriteBurst(mhlTxScratchPadAccess.offset,
                                    mhlTxScratchPadAccess.length,
                                    mhlTxScratchPadAccess.data);
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "SiiMhlIoctl, SiiMhlTxRequestWriteBurst returned 0x%x\n", scratchPadStatus);

                switch(scratchPadStatus)
                {
                case SCRATCHPAD_SUCCESS:
                        break;

                case SCRATCHPAD_BUSY:
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "SiiMhlIoctl, translating return status to -EAGAIN\n");
                        retStatus = -EAGAIN;
                        break;

                default:
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "SiiMhlIoctl, translating return status to -EFAULT\n");
                        retStatus = -EFAULT;
                        break;
                }
                break;

        case SII_MHL_READ_SCRATCH_PAD:

                if(copy_from_user(&mhlTxScratchPadAccess,
                                  (mhlTxScratchPadAccess_t *)ioctlParam,
                                  sizeof(mhlTxScratchPadAccess_t)))
                {
                        retStatus = -EFAULT;
                        break;
                }

                // Make sure there is an MHL connection and that the requested
                // data transfer parameters don't exceed the address space of
                // the scratch pad.  NOTE: The address space reserved for the
                // Scratch Pad registers is 64 bytes but sources and sink devices
                // are only required to implement the 1st 16 bytes.
                if(!(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY) ||
                                (mhlTxScratchPadAccess.length > MAX_SCRATCH_PAD_TRANSFER_SIZE) ||
                                (mhlTxScratchPadAccess.offset >= MAX_SCRATCH_PAD_TRANSFER_SIZE) ||
                                (mhlTxScratchPadAccess.offset + mhlTxScratchPadAccess.length >
                                 MAX_SCRATCH_PAD_TRANSFER_SIZE))
                {
                        retStatus = -EINVAL;
                        break;
                }

                scratchPadStatus =  SiiGetScratchPadVector(mhlTxScratchPadAccess.offset,
                                    mhlTxScratchPadAccess.length,
                                    mhlTxScratchPadAccess.data);
                switch(scratchPadStatus)
                {
                case SCRATCHPAD_SUCCESS:
                        break;

                case SCRATCHPAD_BUSY:
                        retStatus = -EAGAIN;
                        break;
                default:
                        retStatus = -EFAULT;
                        break;
                }
                break;

        default:
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "SiiMhlIoctl, unrecognized ioctlCode 0x%0x received!\n", ioctlCode);
                retStatus = -ENOTTY;
        }

        HalReleaseIsrLock();

        return retStatus;
}

/**
 * @}
 * Done with driver_public_api group
 */
