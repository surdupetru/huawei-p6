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

/****************************************************************************/
/* !file     si_cra.h                                                       */
/* !brief    Silicon Image Device register I/O support.                     */
/****************************************************************************/

#ifndef __SI_CRA_H__
#define __SI_CRA_H__

typedef uint16_t    SiiReg_t;


bool_t              SiiCraInitialize( void );
SiiResultCodes_t    SiiCraGetLastResult( void );
bool_t              SiiRegInstanceSet( SiiReg_t virtualAddress, uint8_t newInstance );

void    SiiRegReadBlock ( SiiReg_t virtualAddr, uint8_t *pBuffer, uint16_t count );
uint8_t SiiRegRead ( SiiReg_t virtualAddr );
void    SiiRegWriteBlock ( SiiReg_t virtualAddr, const uint8_t *pBuffer, uint16_t count );
void    SiiRegWrite ( SiiReg_t virtualAddr, uint8_t value );
void    SiiRegModify ( SiiReg_t virtualAddr, uint8_t mask, uint8_t value);
void    SiiRegBitsSet ( SiiReg_t virtualAddr, uint8_t bitMask, bool_t setBits );
void    SiiRegBitsSetNew ( SiiReg_t virtualAddr, uint8_t bitMask, bool_t setBits );

// Special purpose
void    SiiRegEdidReadBlock ( SiiReg_t segmentAddr, SiiReg_t virtualAddr, uint8_t *pBuffer, uint16_t count );

#endif  // __SI_CRA_H__
