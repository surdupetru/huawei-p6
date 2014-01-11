/*********************************************************************************/
/*  Copyright (c) 2002-2011, Silicon Image, Inc.  All rights reserved.           */
/*  No part of this work may be reproduced, modified, distributed, transmitted,  */
/*  transcribed, or translated into any language or computer format, in any form */
/*  or by any means without written permission of: Silicon Image, Inc.,          */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                         */
/*********************************************************************************/

#ifdef __KERNEL__
#include "sii_hal.h"
#else
#endif

#include "si_common.h"
#include "si_mhl_defs.h"
#include "si_hdmi_tx_lite_api.h"
#include "si_hdmi_tx_lite_private.h"
#include "si_drv_hdmi_tx_lite_hdcp.h"
#include "si_edid.h"
#include "si_mhl_tx_base_drv_api.h"

#ifdef DO_HDCP
#ifdef ENABLE_LITE_DEBUG_PRINT //(
#ifdef C99_VA_ARG_SUPPORT //(
#define LITE_DEBUG_PRINT_WRAPPER(...) SiiOsDebugPrint(__FILE__,__LINE__,SII_OSAL_DEBUG_HDMI_TRACE,__VA_ARGS__)
#define LITE_DEBUG_PRINT(x)  LITE_DEBUG_PRINT_WRAPPER x
#else //)(

#define LITE_DEBUG_PRINT(x) SII_DEBUG_PRINT(SII_OSAL_DEBUG_HDMI_TRACE,x)

#endif //)

#else //)(

#ifdef C99_VA_ARG_SUPPORT //(
#define LITE_DEBUG_PRINT(...)	/* nothing */
#else //)(
#define LITE_DEBUG_PRINT(x)   /* nothing */
#endif //)

#endif //)

static bool_t TxSupportsHdcp;
static bool_t TxValidAksv;

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SiiHdmiTxLiteInitialize()
//
// PURPOSE      :   Tests Tx and Rx support of HDCP. If found, checks if
//                  and attempts to set the security level accordingly.
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :	TxSupportsHdcp - initialized to false, set to true if supported by this device
//					TxValidAksv - initialized to false, set to true if valid AKSVs are read from this device
//
// RETURNS      :   None
//
//////////////////////////////////////////////////////////////////////////////

void SiiHdmiTxLiteInitialize (bool_t enableHdcp)
{
        TxSupportsHdcp = false;
        TxValidAksv = false;

        // If the caller does not want HDCP enabled, no need to execute the rest of the function.
        if (enableHdcp == false)
        {
                // The rest of the code will execute as if TX does not support HDCP,
                // so authentication will never be attempted.
                // Video will be shown as soon as TMDS is enabled.
                return;
        }

        // This is TX-related... need only be done once.
        if (!SiiDrvHdmiTxLiteIsHdcpSupported())
        {
                // The TX does not support HDCP, so authentication will never be attempted.
                // Video will be shown as soon as TMDS is enabled.
                return;
        }

        TxSupportsHdcp = true;

        // This is TX-related... need only be done once.
        if (!SiiDrvHdmiTxLiteIsAksvValid())
        {
                // The TX supports HDCP, but does not have valid AKSVs.
                // Video will not be shown.
                return;
        }

        TxValidAksv = true;

        LITE_DEBUG_PRINT(("HDCP -> Supported by TX, AKSVs valid\n"));
}


void SiiHdmiTxLiteHandleEvents (uint8_t HdcpStatus,uint8_t queryData)
{

        LITE_DEBUG_PRINT(("%s %s\n",(TxSupportsHdcp == true)? "TxSupportsHdcp":"No HDCP here",(TxValidAksv == true)?"TxValidAksv":"No valid Aksv"));
        if ((TxSupportsHdcp == true) && (TxValidAksv == true))
        {
                LITE_DEBUG_PRINT(("%s %s\n",(TxSupportsHdcp == true)? "TxSupportsHdcp":"No HDCP here",(TxValidAksv == true)?"TxValidAksv":"No valid Aksv"));
                SiiDrvHdmiTxLiteHandleHdcpEvents(HdcpStatus,queryData);
        }
}

void SiiHdmiTxLiteDisableEncryption (void)
{
        SiiDrvHdmiTxLiteDisableEncryption();
}


void SiiHdmiTxLiteHpdStatusChange(bool_t connected)
{
        SiiHdmiTxLiteHdmiDrvHpdStatusChange(connected);
}

void SiiHdmiTxLiteTmdsActive(void)
{
        SiiHdmiTxLiteHdmiDrvTmdsActive();
        SiiHdmiTxLiteHdcpDrvTmdsActive();
}

void SiiHdmiTxLiteTmdsInactive(void)
{
        SiiHdmiTxLiteHdmiDrvTmdsInactive();
        SiiHdmiTxLiteHdcpDrvTmdsInactive();
}

#endif
