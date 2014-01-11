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

//***************************************************************************
//!file     si_cra.c
//!brief    Silicon Image Device register I/O support.
//***************************************************************************/

#include "si_c99support.h"
#include "si_common.h"
#include "si_cra.h"
#include "si_cra_cfg.h"
#include "si_cra_internal.h"
#include "si_cra_cfg.h"


static uint8_t		l_pageInstance[SII_CRA_DEVICE_PAGE_COUNT] = {0};
extern pageConfig_t g_addrDescriptor[SII_CRA_MAX_DEVICE_INSTANCES][SII_CRA_DEVICE_PAGE_COUNT];
extern SiiReg_t     g_siiRegPageBaseReassign [];
extern SiiReg_t     g_siiRegPageBaseRegs[SII_CRA_DEVICE_PAGE_COUNT];

CraInstanceData_t craInstance =
{
        0,                          // structVersion
        0,                          // instanceIndex
        SII_SUCCESS,                // lastResultCode
        0                          // statusFlags
};


// Translate SiiPlatformStatus_t return codes into SiiResultCodes_t status codes.
static SiiResultCodes_t TranslatePlatformStatus(SiiPlatformStatus_t platformStatus)
{
        switch(platformStatus)
        {
        case PLATFORM_SUCCESS:
                return SII_SUCCESS;

        case PLATFORM_FAIL:
                return SII_ERR_FAIL;

        case PLATFORM_INVALID_PARAMETER:
                return SII_ERR_INVALID_PARAMETER;

        case PLATFORM_I2C_READ_FAIL:
        case PLATFORM_I2C_WRITE_FAIL:
        default:
                return SII_ERR_FAIL;
        }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static SiiResultCodes_t CraReadBlockI2c (deviceAddrTypes_t busIndex, uint8_t deviceId, uint8_t regAddr, uint8_t *pBuffer, uint16_t count )
{
        SiiI2cMsg_t			msgs[2];
        SiiPlatformStatus_t	platformStatus;
        SiiResultCodes_t	status = SII_ERR_FAIL;

        msgs[0].addr = deviceId;
        msgs[0].cmdFlags = 0;
        msgs[0].len = 1;
        msgs[0].pBuf = &regAddr;

        msgs[1].addr = deviceId;
        msgs[1].cmdFlags = SII_MI2C_RD;
        msgs[1].len = count;
        msgs[1].pBuf = pBuffer;

        platformStatus = SiiMasterI2cTransfer(busIndex, msgs, 2);

        status = TranslatePlatformStatus(platformStatus);

        return( status );
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static SiiResultCodes_t CraWriteBlockI2c (deviceAddrTypes_t busIndex, uint8_t deviceId, uint8_t regAddr, const uint8_t *pBuffer, uint16_t count )
{
        SiiI2cMsg_t			msgs[2];
        SiiPlatformStatus_t	platformStatus;
        SiiResultCodes_t	status = SII_ERR_FAIL;


        msgs[0].addr = deviceId;
        msgs[0].cmdFlags = SII_MI2C_APPEND_NEXT_MSG;
        msgs[0].len = 1;
        msgs[0].pBuf = &regAddr;

        msgs[1].addr = 0;
        msgs[1].cmdFlags = 0;
        msgs[1].len = count;
        msgs[1].pBuf = (uint8_t *)pBuffer;	// cast gets rid of const warning

        platformStatus = SiiMasterI2cTransfer(busIndex, msgs, 2);

        status = TranslatePlatformStatus(platformStatus);

        return( status );
}


//------------------------------------------------------------------------------
// Function:    SiiCraInitialize
// Description: Initialize the CRA page instance array and perform any register
//              page base address reassignments required.
// Parameters:  none
// Returns:     None
//------------------------------------------------------------------------------

bool_t SiiCraInitialize ( void )
{
        uint8_t		idx;
        uint16_t	pageIdx;

        craInstance.lastResultCode = SII_SUCCESS;

        for (idx = 0; idx < SII_CRA_DEVICE_PAGE_COUNT; idx++)
        {
                l_pageInstance[idx] = 0;
        }

        // Perform any register page base address reassignments
        idx = 0;
        while ( g_siiRegPageBaseReassign[idx] != 0xFFFF )
        {
                pageIdx = g_siiRegPageBaseReassign[idx] >> 8;
                if (( pageIdx < SII_CRA_DEVICE_PAGE_COUNT ) && ( g_siiRegPageBaseRegs[pageIdx] != 0xFF))
                {
                        // The page base registers allow reassignment of the
                        // I2C device ID for almost all device register pages.
                        SiiRegWrite( g_siiRegPageBaseRegs[pageIdx], g_siiRegPageBaseReassign[pageIdx] & 0x00FF );
                }
                else
                {
                        craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
                        break;
                }
                idx++;
        }

        return( craInstance.lastResultCode == SII_SUCCESS );
}

//------------------------------------------------------------------------------
// Function:    SiiCraGetLastResult
// Description: Returns the result of the last call to a CRA driver function.
// Parameters:  none.
// Returns:     Returns the result of the last call to a CRA driver function
//------------------------------------------------------------------------------

SiiResultCodes_t SiiCraGetLastResult ( void )
{
        return( craInstance.lastResultCode );
}

//------------------------------------------------------------------------------
// Function:    SiiRegInstanceSet
// Description: Sets the instance for subsequent register accesses.  The register
//              access functions use this value as an instance index of the multi-
//              dimensional virtual address lookup table.
// Parameters:  newInstance - new value for instance axis of virtual address table.
// Returns:     None
//------------------------------------------------------------------------------

bool_t SiiRegInstanceSet ( SiiReg_t virtualAddress, uint8_t newInstance )
{
        uint16_t va = virtualAddress >> 8;

        craInstance.lastResultCode = SII_SUCCESS;
        if (( va < SII_CRA_DEVICE_PAGE_COUNT) && ( newInstance < SII_CRA_MAX_DEVICE_INSTANCES ))
        {
                l_pageInstance[ va ] = newInstance;
                return( true );
        }

        craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
        return( false );
}

//------------------------------------------------------------------------------
// Function:    SiiRegReadBlock
// Description: Reads a block of data from sequential registers.
// Parameters:  regAddr - Sixteen bit register address, including device page.
// Returns:     None
//
// NOTE:        This function relies on the auto-increment model used by
//              Silicon Image devices.  Because of this, if a FIFO register
//              is encountered before the end of the requested count, the
//              data remaining from the count is read from the FIFO, NOT
//              from subsequent registers.
//------------------------------------------------------------------------------

void SiiRegReadBlock ( SiiReg_t virtualAddr, uint8_t *pBuffer, uint16_t count )
{
        uint8_t             regOffset = (uint8_t)virtualAddr;
        pageConfig_t        *pPage;


        virtualAddr >>= 8;
        if(virtualAddr >= SII_CRA_DEVICE_PAGE_COUNT)
        {
                craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
                return;
        }

        pPage = &g_addrDescriptor[l_pageInstance[virtualAddr]][virtualAddr];

        switch ( pPage->busType )
        {
        case DEV_I2C_0:
        case DEV_I2C_1:
        case DEV_I2C_2:
        case DEV_I2C_3:
                craInstance.lastResultCode = CraReadBlockI2c(  pPage->busType,
                                             (uint8_t)pPage->address,
                                             regOffset, pBuffer, count );
                break;
        case DEV_I2C_OFFSET:
        case DEV_I2C_1_OFFSET:
        case DEV_I2C_2_OFFSET:
        case DEV_I2C_3_OFFSET:
                craInstance.lastResultCode = CraReadBlockI2c( pPage->busType - DEV_I2C_OFFSET,
                                             (uint8_t)pPage->address,
                                             regOffset + (uint8_t)(pPage->address >> 8),
                                             pBuffer, count );
                break;
        default:
                craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
                break;
        }
}

//------------------------------------------------------------------------------
// Function:    SiiRegRead
// Description: Read a one byte register.
//              The register address parameter is translated into an I2C slave
//              address and offset. The I2C slave address and offset are used
//              to perform an I2C read operation.
//------------------------------------------------------------------------------

uint8_t SiiRegRead ( SiiReg_t virtualAddr )
{
        uint8_t             value = 0;
        uint8_t             regOffset = (uint8_t)virtualAddr;
        pageConfig_t        *pPage;


        virtualAddr >>= 8;
        if(virtualAddr >= SII_CRA_DEVICE_PAGE_COUNT)
        {
                craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
                return value;
        }

        pPage = &g_addrDescriptor[l_pageInstance[virtualAddr]][virtualAddr];

        switch ( pPage->busType )
        {
        case DEV_I2C_0:
        case DEV_I2C_1:
        case DEV_I2C_2:
        case DEV_I2C_3:
                craInstance.lastResultCode = CraReadBlockI2c(  pPage->busType,
                                             (uint8_t)pPage->address,
                                             regOffset, &value, 1 );
                break;
        case DEV_I2C_OFFSET:
        case DEV_I2C_1_OFFSET:
        case DEV_I2C_2_OFFSET:
        case DEV_I2C_3_OFFSET:
                craInstance.lastResultCode = CraReadBlockI2c( pPage->busType - DEV_I2C_OFFSET,
                                             (uint8_t)pPage->address,
                                             regOffset + (uint8_t)(pPage->address >> 8),
                                             &value, 1 );
                break;
        default:
                craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
                break;
        }

        return( value );
}

//------------------------------------------------------------------------------
// Function:    SiiRegWriteBlock
// Description: Writes a block of data to sequential registers.
// Parameters:  regAddr - Sixteen bit register address, including device page.
// Returns:     None
//
// NOTE:        This function relies on the auto-increment model used by
//              Silicon Image devices.  Because of this, if a FIFO register
//              is encountered before the end of the requested count, the
//              data remaining from the count is read from the FIFO, NOT
//              from subsequent registers.
//------------------------------------------------------------------------------

void SiiRegWriteBlock ( SiiReg_t virtualAddr, const uint8_t *pBuffer, uint16_t count )
{
        uint8_t             regOffset = (uint8_t)virtualAddr;
        pageConfig_t        *pPage;


        virtualAddr >>= 8;
        if(virtualAddr >= SII_CRA_DEVICE_PAGE_COUNT)
        {
                craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
                return;
        }

        pPage = &g_addrDescriptor[l_pageInstance[virtualAddr]][virtualAddr];

        switch ( pPage->busType )
        {
        case DEV_I2C_0:
        case DEV_I2C_1:
        case DEV_I2C_2:
        case DEV_I2C_3:
                craInstance.lastResultCode = CraWriteBlockI2c( pPage->busType,
                                             (uint8_t)pPage->address,
                                             regOffset, pBuffer, count );
                break;
        case DEV_I2C_OFFSET:
        case DEV_I2C_1_OFFSET:
        case DEV_I2C_2_OFFSET:
        case DEV_I2C_3_OFFSET:
                craInstance.lastResultCode = CraWriteBlockI2c( pPage->busType - DEV_I2C_OFFSET,
                                             (uint8_t)pPage->address,
                                             regOffset + (uint8_t)(pPage->address >> 8),
                                             pBuffer, count );
                break;
        default:
                craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
                break;
        }
}

//------------------------------------------------------------------------------
// Function:    SiiRegWrite
// Description: Write a one byte register.
//              The register address parameter is translated into an I2C slave
//              address and offset. The I2C slave address and offset are used
//              to perform an I2C write operation.
//------------------------------------------------------------------------------

void SiiRegWrite ( SiiReg_t virtualAddr, uint8_t value )
{
        uint8_t             regOffset = (uint8_t)virtualAddr;
        pageConfig_t        *pPage;


        virtualAddr >>= 8;
        if(virtualAddr >= SII_CRA_DEVICE_PAGE_COUNT)
        {
                craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
                return;
        }

        pPage = &g_addrDescriptor[l_pageInstance[virtualAddr]][virtualAddr];

        switch ( pPage->busType )
        {
        case DEV_I2C_0:
        case DEV_I2C_1:
        case DEV_I2C_2:
        case DEV_I2C_3:
                craInstance.lastResultCode = CraWriteBlockI2c( pPage->busType,
                                             (uint8_t)pPage->address,
                                             regOffset, &value, 1 );
                break;
        case DEV_I2C_OFFSET:
        case DEV_I2C_1_OFFSET:
        case DEV_I2C_2_OFFSET:
        case DEV_I2C_3_OFFSET:
                craInstance.lastResultCode = CraWriteBlockI2c( pPage->busType - DEV_I2C_OFFSET,
                                             (uint8_t)pPage->address,
                                             regOffset + (uint8_t)(pPage->address >> 8),
                                             &value, 1 );
                break;
        default:
                craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
                break;
        }
}

//------------------------------------------------------------------------------
// Function:    SiiRegModify
// Description: Reads the register, performs an AND function on the data using
//              the mask parameter, and an OR function on the data using the
//              value ANDed with the mask. The result is then written to the
//              device register specified in the regAddr parameter.
// Parameters:  regAddr - Sixteen bit register address, including device page.
//              mask    - Eight bit mask
//              value   - Eight bit data to be written, combined with mask.
// Returns:     None
//------------------------------------------------------------------------------

void SiiRegModify ( SiiReg_t virtualAddr, uint8_t mask, uint8_t value)
{
        uint8_t aByte;

        aByte = SiiRegRead( virtualAddr );
        aByte &= (~mask);                       // first clear all bits in mask
        aByte |= (mask & value);                // then set bits from value
        SiiRegWrite( virtualAddr, aByte );
}

//------------------------------------------------------------------------------
// Function:    SiiRegBitsSet
// Description: Reads the register, sets the passed bits, and writes the
//              result back to the register.  All other bits are left untouched
// Parameters:  regAddr - Sixteen bit register address, including device page.
//              bits   - bit data to be written
// Returns:     None
//------------------------------------------------------------------------------

void SiiRegBitsSet ( SiiReg_t virtualAddr, uint8_t bitMask, bool_t setBits )
{
        uint8_t aByte;

        aByte = SiiRegRead( virtualAddr );
        aByte = (setBits) ? (aByte | bitMask) : (aByte & ~bitMask);
        SiiRegWrite( virtualAddr, aByte );
}

//------------------------------------------------------------------------------
// Function:    SiiRegBitsSetNew
// Description: Reads the register, sets or clears the specified bits, and
//              writes the result back to the register ONLY if it would change
//              the current register contents.
// Parameters:  regAddr - Sixteen bit register address, including device page.
//              bits   - bit data to be written
//              setBits- true == set, false == clear
// Returns:     None
//------------------------------------------------------------------------------

void SiiRegBitsSetNew ( SiiReg_t virtualAddr, uint8_t bitMask, bool_t setBits )
{
        uint8_t newByte, oldByte;

        oldByte = SiiRegRead( virtualAddr );
        newByte = (setBits) ? (oldByte | bitMask) : (oldByte & ~bitMask);
        if ( oldByte != newByte )
        {
                SiiRegWrite( virtualAddr, newByte );
        }
}

//------------------------------------------------------------------------------
// Function:    SiiRegEdidReadBlock
// Description: Reads a block of data from EDID record over DDC link.
// Parameters:  segmentAddr - EDID segment address (16 bit), including device page;
//              offsetAddr  - Sixteen bit register address, including device page.
// Returns:     success flag
//
//------------------------------------------------------------------------------

void SiiRegEdidReadBlock ( SiiReg_t segmentAddr, SiiReg_t virtualAddr, uint8_t *pBuffer, uint16_t count )
{
        uint8_t             regOffset = (uint8_t)virtualAddr;
        pageConfig_t        *pPage;
        SiiI2cMsg_t			msgs[3];
        uint8_t				msgCount = 2;
        SiiPlatformStatus_t	platformStatus;


        if ((segmentAddr & 0xFF) != 0)  // Default segment #0 index should not be sent explicitly
        {
                msgCount = 3;
                regOffset = (uint8_t)segmentAddr;
                segmentAddr >>= 8;
                if(segmentAddr >= SII_CRA_DEVICE_PAGE_COUNT)
                {
                        craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
                        return;
                }
                pPage = &g_addrDescriptor[l_pageInstance[segmentAddr]][segmentAddr];

                // Setup to send the segment number to the EDID device.
                msgs[0].addr = pPage->address;
                msgs[0].cmdFlags = 0;
                msgs[0].len = 1;
                msgs[0].pBuf = &regOffset;
        }

        // Read the actual EDID data
        regOffset = (uint8_t)virtualAddr;
        virtualAddr >>= 8;
        if(virtualAddr >= SII_CRA_DEVICE_PAGE_COUNT)
        {
                craInstance.lastResultCode = SII_ERR_INVALID_PARAMETER;
                return;
        }

        pPage = &g_addrDescriptor[l_pageInstance[virtualAddr]][virtualAddr];

        msgs[1].addr = pPage->address;
        msgs[1].cmdFlags = 0;
        msgs[1].len = 1;
        msgs[1].pBuf = &regOffset;

        msgs[2].addr = pPage->address;
        msgs[2].cmdFlags = SII_MI2C_RD;
        msgs[2].len = count;
        msgs[2].pBuf = pBuffer;

        platformStatus = SiiMasterI2cTransfer(pPage->busType, &msgs[3 - msgCount], msgCount);

        craInstance.lastResultCode = TranslatePlatformStatus(platformStatus);
}

