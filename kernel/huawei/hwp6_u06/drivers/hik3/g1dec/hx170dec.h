/* 
 * Decoder device driver (kernel module headers)
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
------------------------------------------------------------------------------*/

#ifndef _HX170DEC_H_
#define _HX170DEC_H_
#include <linux/ioctl.h>    /* needed for the _IOW etc stuff used later */

/*
 * Macros to help debugging
 */
#undef PDEBUG           /* undef it, just in case */
#ifdef HX170DEC_DEBUG
#  ifdef __KERNEL__
/* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk(KERN_INFO "x170: " fmt, ## args)
#  else
/* This one for user space */
#    define PDEBUG(fmt, args...) printf(__FILE__ ":%d: " fmt, __LINE__ , ## args)
#  endif
#else
#  define PDEBUG(fmt, args...)  /* not debugging: nothing */
#endif

/*
 * Ioctl definitions
 */

/* Use 'k' as magic number */
#define HX170DEC_IOC_MAGIC  'k'
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */

/* the client is pp instance */
#define HX170DEC_PP_INSTANCE    _IO(HX170DEC_IOC_MAGIC, 1)
/* decode/pp time for HW performance */
#define HX170DEC_HW_PERFORMANCE _IO(HX170DEC_IOC_MAGIC, 2)
#define HX170DEC_IOCGHWOFFSET   _IOR(HX170DEC_IOC_MAGIC,  3, unsigned long *)
#define HX170DEC_IOCGHWIOSIZE   _IOR(HX170DEC_IOC_MAGIC,  4, unsigned int *)

#define HX170DEC_IOC_CLI           _IO(HX170DEC_IOC_MAGIC,  5)
#define HX170DEC_IOC_STI           _IO(HX170DEC_IOC_MAGIC,  6)
#define HX170DEC_IOC_WAIT_DONE     _IO(HX170DEC_IOC_MAGIC,  7)
#define HX170DEC_IOC_CLOCK_ON      _IO(HX170DEC_IOC_MAGIC,  8)
#define HX170DEC_IOC_CLOCK_OFF     _IO(HX170DEC_IOC_MAGIC,  9)

/* debugging tool */
#define HX170DEC_IOCHARDRESET      _IO(HX170DEC_IOC_MAGIC, 10)

#define HX170DEC_IOC_MAXNR 10

#endif /* !_HX170DEC_H_ */
