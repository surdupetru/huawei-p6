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
 * @file sii_hal_linux_i2c.c
 *
 * @brief Linux implementation of I2c access functions required by Silicon Image
 *        MHL devices.
 *
 * $Author: Dave Canfield
 * $Rev: $
 * $Date: Jan. 24, 2011
 *
 *****************************************************************************/

#define SII_HAL_LINUX_I2C_C

/***** #include statements ***************************************************/
#include <linux/i2c.h>
#include <linux/slab.h>
#include "sii_hal.h"
#include "sii_hal_priv.h"
#include "si_c99support.h"
#include "si_common.h"
#include "si_cra_cfg.h"
#include "si_cra_internal.h"
#include "si_osdebug.h"
#include <hsad/config_mgr.h>
#include <linux/regulator/consumer.h>


/***** local macro definitions ***********************************************/


/***** local type definitions ************************************************/


/***** local variable declarations *******************************************/


/***** local function prototypes *********************************************/

static int32_t MhlI2cProbe(struct i2c_client *client, const struct i2c_device_id *id);
static int32_t MhlI2cRemove(struct i2c_client *client);


/***** global variable declarations *******************************************/


/***** local functions *******************************************************/

/**
 *  @brief Standard Linux probe callback.
 *
 *  Probe is called if the I2C device name passed to HalOpenI2cDevice matches
 *  the name of an I2C device on the system.
 *
 *  All we need to do is store the passed in i2c_client* needed when performing
 *  I2C bus transactions with the device.
 *
 *  @param[in]      client     		Pointer to i2c client structure of the matching
 *  								I2C device.
 *  @param[in]      i2c_device_id	Index within MhlI2cIdTable of the matching
 *  								I2C device.
 *
 *  @return     Always returns zero to indicate success.
 *
 *****************************************************************************/
static int32_t MhlI2cProbe(struct i2c_client *client, const struct i2c_device_id *id)
{
        gMhlDevice.pI2cClient = client;

        char mhl_ldo_supply[15];
        bool ret =0;

        ret = get_hw_config_string("MHL/ldo_supply",mhl_ldo_supply,15,NULL);

        if(ret && (!strcmp(mhl_ldo_supply, "yes")))
        {
                printk("mhl: avcc was supplied by ldo\n");

                gMhlDevice.avcc = regulator_get(&client->dev, "mhl-avcc");
                if (IS_ERR(gMhlDevice.avcc))
                {
                        printk("mhl: failed to get mhl avcc\n");
                        return PTR_ERR(gMhlDevice.avcc);
                }

                if (regulator_set_voltage(gMhlDevice.avcc, 3300000, 3300000))
                {
                        printk("mhl: failed to set mhl avcc to 3.3v!\n");
                }

                if (regulator_enable(gMhlDevice.avcc))
                {
                        printk("mhl: failed to enable mhl avcc\n");
                }
        }
        return 0;
}


/**
 *  @brief Standard Linux remove callback.
 *
 *  Remove would be called if the I2C device were removed from the system (very unlikley).
 *
 *  All we need to do is clear our copy of the i2c_client pointer to indicate we no longer
 *  have an I2C device to work with.
 *
 *  @param[in]	client	Pointer to the client structure representing the I2C device that
 *  					was removed.
 *
 *  @return     Always returns zero to indicate success.
 *
 *****************************************************************************/
static int32_t MhlI2cRemove(struct i2c_client *client)
{
        gMhlDevice.pI2cClient = NULL;
        return 0;
}


/***** public functions ******************************************************/


/*****************************************************************************/
/**
 *  @brief Check if I2c access is allowed.
 *
 *****************************************************************************/
halReturn_t I2cAccessCheck(void)
{
        halReturn_t		retStatus;

        retStatus = HalInitCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        if(gMhlDevice.pI2cClient == NULL)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "I2C device not currently open\n");
                retStatus = HAL_RET_DEVICE_NOT_OPEN;
        }
        return retStatus;
}



/*****************************************************************************/
/**
 * @brief Request access to the specified I2c device.
 *
 *****************************************************************************/
halReturn_t HalOpenI2cDevice(char const *DeviceName, char const *DriverName)
{
        halReturn_t		retStatus;
        int32_t 		retVal;


        retStatus = HalInitCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        retVal = strnlen(DeviceName, I2C_NAME_SIZE);
        if (retVal >= I2C_NAME_SIZE)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "I2c device name too long!\n");
                return HAL_RET_PARAMETER_ERROR;
        }

        memcpy(gMhlI2cIdTable[0].name, DeviceName, retVal);
        gMhlI2cIdTable[0].name[retVal] = 0;
        gMhlI2cIdTable[0].driver_data = 0;

        gMhlDevice.driver.driver.name = DriverName;
        gMhlDevice.driver.id_table = gMhlI2cIdTable;
        gMhlDevice.driver.probe = MhlI2cProbe;
        gMhlDevice.driver.remove = MhlI2cRemove;


        retVal = i2c_add_driver(&gMhlDevice.driver);
        if (retVal != 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "I2C driver add failed\n");
                retStatus = HAL_RET_FAILURE;
        }
        else
        {
                if (gMhlDevice.pI2cClient == NULL)
                {
                        i2c_del_driver(&gMhlDevice.driver);
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "I2C driver add failed\n");
                        retStatus = HAL_RET_NO_DEVICE;
                }
                else
                {
                        retStatus = HAL_RET_SUCCESS;
                }
        }
        return retStatus;
}



/*****************************************************************************/
/**
 * @brief Terminate access to the specified I2c device.
 *
 *****************************************************************************/
halReturn_t HalCloseI2cDevice(void)
{
        halReturn_t		retStatus;


        retStatus = HalInitCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        if(gMhlDevice.pI2cClient == NULL)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "I2C device not currently open\n");
                retStatus = HAL_RET_DEVICE_NOT_OPEN;
        }
        else
        {
                i2c_del_driver(&gMhlDevice.driver);
                gMhlDevice.pI2cClient = NULL;
                retStatus = HAL_RET_SUCCESS;
        }
        return retStatus;
}


#if 0
/*****************************************************************************/
/**
 * @brief Read a byte from an I2c device using SMBus protocol.
 *
 *****************************************************************************/
halReturn_t HalSmbusReadByteData(uint8_t command, uint8_t *pRetByteRead)
{
        halReturn_t		retStatus;
        int32_t			status;

        retStatus = I2cAccessCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        status = i2c_smbus_read_byte_data(gMhlDevice.pI2cClient, command);
        if (status < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "i2c_smbus_read_byte_data returned error: %d\n",status);
                return HAL_RET_FAILURE;
        }

        *pRetByteRead = (uint8_t)status;
        return HAL_RET_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Write a byte to an I2c device using SMBus protocol.
 *
 *****************************************************************************/
halReturn_t HalSmbusWriteByteData(uint8_t command, uint8_t writeByte)
{
        halReturn_t		retStatus;
        int32_t			status;

        retStatus = I2cAccessCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        status = i2c_smbus_write_byte_data(gMhlDevice.pI2cClient,
                                           command, writeByte);
        if (status < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "i2c_smbus_write_byte_data returned error: %d\n",status);
                return HAL_RET_FAILURE;
        }

        return HAL_RET_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Read a word from an I2c device using SMBus protocol.
 *
 *****************************************************************************/
halReturn_t HalSmbusReadWordData(uint8_t command, uint16_t *pRetWordRead)
{
        halReturn_t		retStatus;
        int32_t			status;

        retStatus = I2cAccessCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        status = i2c_smbus_read_word_data(gMhlDevice.pI2cClient, command);
        if (status < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "i2c_smbus_read_word_data returned error: %d\n",status);
                return HAL_RET_FAILURE;
        }

        *pRetWordRead = (uint16_t)status;
        return HAL_RET_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Write a word to an I2c device using SMBus protocol.
 *
 *****************************************************************************/
halReturn_t HalSmbusWriteWordData(uint8_t command, uint16_t wordData)
{
        halReturn_t		retStatus;
        int32_t			status;

        retStatus = I2cAccessCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        status = i2c_smbus_write_word_data(gMhlDevice.pI2cClient,
                                           command, wordData);
        if (status < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "i2c_smbus_write_word_data returned error: %d\n",status);
                return HAL_RET_FAILURE;
        }

        return HAL_RET_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Read a series of bytes from an I2c device using SMBus protocol.
 *
 *****************************************************************************/
halReturn_t HalSmbusReadBlock(uint8_t command, uint8_t *buffer, uint8_t *bufferLen)
{
        halReturn_t		retStatus;
        int32_t			status;

        retStatus = I2cAccessCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        if(*bufferLen > I2C_SMBUS_BLOCK_MAX)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "HalSmbusReadBlock, bufferLen param too big (%d) max size (%d)!\n",
                                *bufferLen, I2C_SMBUS_BLOCK_MAX);
                return HAL_RET_PARAMETER_ERROR;

        }
        status = i2c_smbus_read_i2c_block_data(gMhlDevice.pI2cClient, command,
                                               *bufferLen, buffer);
        if (status < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "i2c_smbus_read_i2c_block_data returned error: %d\n",status);
                return HAL_RET_FAILURE;
        }

        *bufferLen = (uint8_t)status;	/* return # of bytes read */

        return HAL_RET_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Write a series of bytes to an I2c device using SMBus protocol.
 *
 *****************************************************************************/
halReturn_t HalSmbusWriteBlock(uint8_t command, uint8_t const *blockData,
                               uint8_t length)
{
        halReturn_t		retStatus;
        int32_t			status;

        retStatus = I2cAccessCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        if(length > I2C_SMBUS_BLOCK_MAX)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "HalSmbusWriteBlock, bufferLen param too big (%d) max size (%d)!\n",
                                length, I2C_SMBUS_BLOCK_MAX);
                return HAL_RET_PARAMETER_ERROR;

        }
        status = i2c_smbus_write_i2c_block_data(gMhlDevice.pI2cClient, command,
                                                length, blockData);
        if (status < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "i2c_smbus_write_i2c_block_data returned error: %d\n",status);
                return HAL_RET_FAILURE;
        }

        return HAL_RET_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Write a series of bytes to an I2c device.
 *
 *****************************************************************************/
halReturn_t HalI2cMasterWrite(uint8_t i2cAddr, uint8_t length, uint8_t *buffer)
{
        struct i2c_msg	i2cMsg;

        halReturn_t		retStatus;
        int32_t			status;

        retStatus = I2cAccessCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        i2cMsg.addr = (i2cAddr >> 1);
        i2cMsg.flags = 0;
        i2cMsg.len = length;
        i2cMsg.buf = buffer;

        status = i2c_transfer(gMhlDevice.pI2cClient->adapter, &i2cMsg, 1);
        if (status < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "HalI2cMasterWrite, i2c_transfer error: %d\n",status);
                return HAL_RET_FAILURE;
        }

        return HAL_RET_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Read a series of bytes from an I2c device.
 *
 *****************************************************************************/
halReturn_t HalI2cMasterRead(uint8_t i2cAddr, uint8_t length,
                             uint8_t *buffer)
{
        struct i2c_msg	i2cMsg;

        halReturn_t		retStatus;
        int32_t			status;

        retStatus = I2cAccessCheck();
        if (retStatus != HAL_RET_SUCCESS)
        {
                return retStatus;
        }

        i2cMsg.addr = (i2cAddr >> 1);
        i2cMsg.flags = I2C_M_RD;
        i2cMsg.len = length;
        i2cMsg.buf = buffer;

        status = i2c_transfer(gMhlDevice.pI2cClient->adapter, &i2cMsg, 1);
        if (status < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "i2c_transfer error: %d\n",status);
                return HAL_RET_FAILURE;
        }

        return HAL_RET_SUCCESS;
}


/*****************************************************************************/
/**
 * @brief Read a single byte from a register within an I2c device.
 *
 *****************************************************************************/
uint8_t I2C_ReadByte(uint8_t deviceID, uint8_t offset)
{
        uint8_t					accessI2cAddr;
        uint8_t					addrOffset;
        union i2c_smbus_data	data;
        int32_t					status;


        if (I2cAccessCheck() != HAL_RET_SUCCESS)
        {
                /* Driver expects failed I2C reads to return 0xFF */
                return 0xFF;
        }

        // Figure I2c address offset from base of I2c device to requested
        // I2c address.
        addrOffset = deviceID - BASE_I2C_ADDR;

        // Get REAL base address of the I2c device on the platform.
        accessI2cAddr = (uint8_t)(gMhlDevice.pI2cClient->addr << 1);

        // Calculate REAL I2c access address.
        accessI2cAddr += addrOffset;

        accessI2cAddr >>= 1;

        status = i2c_smbus_xfer(gMhlDevice.pI2cClient->adapter, accessI2cAddr,
                                0, I2C_SMBUS_READ, offset, I2C_SMBUS_BYTE_DATA,
                                &data);
        if (status < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "I2C_ReadByte(0x%02x, 0x%02x), i2c_transfer error: %d\n",
                                deviceID, offset, status);
                data.byte = 0xFF;
        }

        return data.byte;
}



/*****************************************************************************/
/**
 * @brief Write a single byte to a register within an I2c device.
 *
 *****************************************************************************/
void I2C_WriteByte(uint8_t deviceID, uint8_t offset, uint8_t value)
{
        uint8_t					accessI2cAddr;
        uint8_t					addrOffset;
        union i2c_smbus_data	data;
        int32_t					status;


        if (I2cAccessCheck() != HAL_RET_SUCCESS)
        {
                return;
        }

        // Figure I2c address offset from base of I2c device to requested
        // I2c address.
        addrOffset = deviceID - BASE_I2C_ADDR;

        // Get REAL base address of the I2c device on the platform.
        accessI2cAddr = (uint8_t)(gMhlDevice.pI2cClient->addr << 1);

        // Calculate REAL I2c access address.
        accessI2cAddr += addrOffset;

        accessI2cAddr >>= 1;

        data.byte = value;

        status = i2c_smbus_xfer(gMhlDevice.pI2cClient->adapter, accessI2cAddr,
                                0, I2C_SMBUS_WRITE, offset, I2C_SMBUS_BYTE_DATA,
                                &data);
        if (status < 0)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "I2C_WriteByte(0x%02x, 0x%02x, 0x%02x), i2c_transfer error: %d\n",
                                deviceID, offset, value, status);
        }
}
#endif // #if 0

/*****************************************************************************/
/**
 * @brief Linux implementation of CRA driver platform interface function
 * 		  SiiMasterI2cTransfer.
 *
 *****************************************************************************/
SiiPlatformStatus_t SiiMasterI2cTransfer(deviceAddrTypes_t busIndex,
                SiiI2cMsg_t *pMsgs, uint8_t msgNum)
{
        uint8_t				idx;
        uint8_t				msgCount = 0;
        struct i2c_msg		i2cMsg[MAX_I2C_MESSAGES];
        uint8_t				*pBuffer = NULL;
        SiiPlatformStatus_t	siiStatus = PLATFORM_FAIL;
        int					i2cStatus;


        do
        {
                if (I2cAccessCheck() != HAL_RET_SUCCESS)
                {
                        break;
                }

                if(busIndex != DEV_I2C_0)
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "SiiMasterI2cTransfer error: implementation supports" \
                                        "only one I2C bus\n");
                        break;
                }

                if(msgNum > MAX_I2C_MESSAGES)
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "SiiMasterI2cTransfer error: implementation supports" \
                                        "only %d message segments\n", MAX_I2C_MESSAGES);
                        break;
                }

                // Function parameter checks passed, assume at this point that the
                // function will complete successfully.
                siiStatus = PLATFORM_SUCCESS;

                for(idx=0; idx < msgNum; idx++)
                {
                        i2cMsg[idx].addr	= pMsgs[idx].addr >> 1;
                        i2cMsg[idx].buf		= pMsgs[idx].pBuf;
                        i2cMsg[idx].len		= pMsgs[idx].len;
                        i2cMsg[idx].flags	= (pMsgs[idx].cmdFlags & SII_MI2C_RD) ? I2C_M_RD : 0;
                        if(pMsgs[idx].cmdFlags & SII_MI2C_TEN)
                        {
                                pMsgs[idx].cmdFlags |= I2C_M_TEN;
                        }
                        if(pMsgs[idx].cmdFlags & SII_MI2C_APPEND_NEXT_MSG)
                        {
                                // Caller is asking that we append the buffer from the next
                                // message to this one.  We will do this IF there is a next
                                // message AND the direction of the two messages is the same
                                // AND we haven't already appended a message.

                                siiStatus = PLATFORM_INVALID_PARAMETER;
                                if(idx+1 < msgNum && pBuffer == NULL)
                                {
                                        if(!((pMsgs[idx].cmdFlags ^ pMsgs[idx+1].cmdFlags) & SII_MI2C_RD))
                                        {

                                                i2cMsg[idx].len += pMsgs[idx+1].len;

                                                pBuffer = kmalloc(i2cMsg[idx].len, GFP_KERNEL);
                                                if(pBuffer == NULL)
                                                {
                                                        siiStatus = PLATFORM_FAIL;
                                                        break;
                                                }

                                                i2cMsg[idx].buf = pBuffer;
                                                memmove(pBuffer, pMsgs[idx].pBuf, pMsgs[idx].len);
                                                memmove(&pBuffer[pMsgs[idx].len], pMsgs[idx+1].pBuf, pMsgs[idx+1].len);

                                                idx += 1;
                                                siiStatus = PLATFORM_SUCCESS;
                                        }
                                }
                        }
                        msgCount++;
                }

                if(siiStatus != PLATFORM_SUCCESS)
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "SiiMasterI2cTransfer failed, returning error: %d\n", siiStatus);

                        if(pBuffer != NULL)
                        {
                                kfree(pBuffer);
                        }

                        return siiStatus;
                }

                i2cStatus = i2c_transfer(gMhlDevice.pI2cClient->adapter, i2cMsg, msgCount);

                if(pBuffer != NULL)
                {
                        kfree(pBuffer);
                }

                if(i2cStatus < msgCount)
                {
                        // All the messages were not transferred, some sort of error occurred.
                        // Try to return the most appropriate error code to the caller.
                        if (i2cStatus < 0)
                        {
                                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                                "SiiMasterI2cTransfer, i2c_transfer error: %d  " \
                                                "deviceId: 0x%02x regOffset: 0x%02x\n",
                                                i2cStatus, pMsgs->addr, *pMsgs->pBuf);
                                siiStatus = PLATFORM_FAIL;
                        }
                        else
                        {
                                // One or more messages transferred so error probably occurred on the
                                // first unsent message.  Look to see if the message was a read or write
                                // and set the appropriate return code.
                                if(pMsgs[i2cStatus].cmdFlags & SII_MI2C_RD)
                                {
                                        siiStatus = PLATFORM_I2C_READ_FAIL;
                                }
                                else
                                {
                                        siiStatus = PLATFORM_I2C_WRITE_FAIL;
                                }
                        }
                }

        }
        while(0);

        return siiStatus;
}
