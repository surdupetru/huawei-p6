/*
  Copyright (c) 2011, 2012 Silicon Image, Inc.  All rights reserved.
  No part of this work may be reproduced, modified, distributed, transmitted,
  transcribed, or translated into any language or computer format, in any form
  or by any means without written permission of: Silicon Image, Inc.,
  1140 East Arques Avenue, Sunnyvale, California 94085
*/
/*
   @file si_hdmi_tx_lite_hdcp.h
*/

#ifdef DO_HDCP
void SiiDrvHdmiTxLiteHdcpInitialize (void);

bool_t SiiDrvHdmiTxLiteIsHdcpSupported (void);
bool_t SiiDrvHdmiTxLiteIsAksvValid (void);

void SiiDrvHdmiTxLiteHandleHdcpEvents (uint8_t HdcpStatus,uint8_t queryData);

void SiiDrvHdmiTxLiteDisableEncryption (void);
#endif
