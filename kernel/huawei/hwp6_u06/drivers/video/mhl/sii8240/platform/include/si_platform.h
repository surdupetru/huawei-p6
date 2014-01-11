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
//!file     platform.h
//!brief    Silicon Image data type header (conforms to C99).
//***************************************************************************/

#ifndef __SI_PLATFORM_H__
#define __SI_PLATFORM_H__
/*
The contents of this file are obsolete, but the
include structure requires a file by this name, so if zero it out.
Some other content will be added when time allows some minor re-organization.
*/
#if 0 //(
/* C99 defined data types.  */

typedef int           int_t;
typedef unsigned int  uint_t;

#define LOW                     0
#define HIGH                    1

#define MSG_ALWAYS              0x00
#define MSG_STAT                0x01
#define MSG_DBG                 0x02

// see include/i2c_slave_addrs.h

#define SET_BITS    0xFF
#define CLEAR_BITS  0x00

#endif //)

#endif  // __SI_PLATFORM_H__

