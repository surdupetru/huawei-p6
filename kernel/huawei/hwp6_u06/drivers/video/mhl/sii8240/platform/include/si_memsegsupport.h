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

/*
   @file si_memsegsupport.h
 */
// These defines are only relevant when building firmware for the 8051
// platform.  For Linux drivers they're just dummy defines to prevent
// compile errors.
#ifndef _MEMSEGSUPPORT_ //(
#define _MEMSEGSUPPORT_
#define PLACE_IN_CODE_SEG             // 8051 type of ROM memory
#define PLACE_IN_DATA_SEG             // 8051 type of external memory

#define SI_PUSH_STRUCT_PACKING
#define SI_POP_STRUCT_PACKING
#define SI_PACK_THIS_STRUCT __attribute__((__packed__))
#endif //)
