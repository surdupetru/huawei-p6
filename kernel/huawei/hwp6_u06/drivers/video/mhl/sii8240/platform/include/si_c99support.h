/***********************************************************************************/
/*  Copyright (c) 2010-2011, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1140 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/

/*
   @file si_c99support.h
 */

#ifndef __SI_C99SUPPORT_H__
#define __SI_C99SUPPORT_H__


/*
This file is a place holder for C99 data types.
Since the GNU compiler is inherently C99 compliant, little is necessary here
However, this file must remain in the source tree to satisfy the #include directives
in the component, driver, and application modules.
*/

#include <linux/kernel.h>


// Emulate C99/C++ bool type to support the large amount of code that
// has yet to be ported to use bool.
typedef bool bool_t;

#ifndef __KERNEL__
# ifndef __intptr_t_defined
typedef int				intptr_t;
typedef unsigned int	uintptr_t;
#  define __intptr_t_defined
# endif
#endif

//typedef char BOOL;

#endif  // __SI_C99SUPPORT_H__

