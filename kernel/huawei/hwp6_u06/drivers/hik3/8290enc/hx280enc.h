/* 
 * Encoder device driver (kernel module header)
 *
 * Copyright (C) 2012 Google Finland Oy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
--------------------------------------------------------------------------------
--
--  Abstract : 6280/7280/8270/8290 Encoder device driver (kernel module)
--
------------------------------------------------------------------------------*/

#ifndef _HX280ENC_H_
#define _HX280ENC_H_
#include <linux/ioctl.h>    /* needed for the _IOW etc stuff used later */

/*
 * Ioctl definitions
 */

/* Use 'k' as magic number */
#define HX280ENC_IOC_MAGIC  'k'
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */

#define HX280ENC_IOCGHWOFFSET      _IOR(HX280ENC_IOC_MAGIC,  1, unsigned long *)
#define HX280ENC_IOCGHWIOSIZE      _IOR(HX280ENC_IOC_MAGIC,  2, unsigned int *)
#define HX280ENC_IOC_CLI           _IO(HX280ENC_IOC_MAGIC,  3)
#define HX280ENC_IOC_STI           _IO(HX280ENC_IOC_MAGIC,  4)

#define HX280ENC_IOCHARDRESET      _IO(HX280ENC_IOC_MAGIC, 5)   /* debugging tool */
#define HX280ENC_IOCVIRTBASE       _IO(HX280ENC_IOC_MAGIC, 6)
#define HX280ENC_IOCWAITDONE       _IO(HX280ENC_IOC_MAGIC, 7)
#define HX280ENC_IOC_CLOCK_ON      _IO(HX280ENC_IOC_MAGIC, 8)
#define HX280ENC_IOC_CLOCK_OFF     _IO(HX280ENC_IOC_MAGIC, 9)

#define HX280ENC_IOC_MAXNR 9

#endif /* !_HX280ENC_H_ */
