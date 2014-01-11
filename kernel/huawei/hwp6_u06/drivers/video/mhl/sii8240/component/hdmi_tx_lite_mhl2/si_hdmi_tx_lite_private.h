/*
  Copyright (c) 2011, 2012 Silicon Image, Inc.  All rights reserved.
  No part of this work may be reproduced, modified, distributed, transmitted,
  transcribed, or translated into any language or computer format, in any form
  or by any means without written permission of: Silicon Image, Inc.,
  1140 East Arques Avenue, Sunnyvale, California 94085
*/
/*
   @file si_hdmi_tx_lite_private.h
*/


void SiiHdmiTxLiteHdmiDrvHpdStatusChange(bool_t connected);
void SiiHdmiTxLiteHdmiDrvTmdsActive(void);
void SiiHdmiTxLiteHdcpDrvTmdsActive(void);
void SiiHdmiTxLiteHdmiDrvTmdsInactive(void);
void SiiHdmiTxLiteHdcpDrvTmdsInactive(void);

void SiiHdmiTxLiteDrvSendVendorSpecificInfoFrame( PVendorSpecificInfoFrame_t pVendorSpecificInfoFrame);
