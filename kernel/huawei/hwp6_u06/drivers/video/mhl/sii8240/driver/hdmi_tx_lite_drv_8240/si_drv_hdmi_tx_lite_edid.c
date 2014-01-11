/*********************************************************************************/
/*  Copyright (c) 2002-2011, Silicon Image, Inc.  All rights reserved.           */
/*  No part of this work may be reproduced, modified, distributed, transmitted,  */
/*  transcribed, or translated into any language or computer format, in any form */
/*  or by any means without written permission of: Silicon Image, Inc.,          */
/*  1140 East Arques Avenue, Sunnyvale, California 94085                         */
/*********************************************************************************/

/*
 *****************************************************************************
 * @file  EDID.c
 *
 * @brief Implementation of the Foo API.
 *
 *****************************************************************************
*/


#include "si_memsegsupport.h"
//#include "si_platform.h"
#include "si_common.h"
#if !defined(__KERNEL__) //(
#include "hal_timers.h"
#else //)(
#include "sii_hal.h"
#endif //)
#include "si_cra.h"
#include "si_cra_cfg.h"
#include "si_8240_regs.h"
#include "si_tpi_regs.h"
#include "si_bitdefs.h"
#include "si_mhl_defs.h"
#include "si_mhl_tx_base_drv_api.h"

