/*********************************************************************************/
/*  Copyright (c) 2002-2011, Silicon Image, Inc.  All rights reserved.           */
/*  No part of this work may be reproduced, modified, distributed, transmitted,  */
/*  transcribed, or translated into any language or computer format, in any form */
/*  or by any means without written permission of: Silicon Image, Inc.,          */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                         */
/*********************************************************************************/
#ifdef DO_HDCP
void SiiHdmiTxLiteInitialize (bool_t enableHdcp);
void SiiHdmiTxLiteHandleEvents (uint8_t HdcpStatus,uint8_t queryData);

void SiiHdmiTxLiteDisableEncryption (void);

void SiiHdmiTxLiteHpdStatusChange(bool_t connected);
void SiiHdmiTxLiteTmdsActive(void);
void SiiHdmiTxLiteTmdsInactive(void);

uint8_t SiiHdmiTxLiteReadEdid (void);
void	AppNotifyMhlDownStreamHPDStatusChange(bool_t connected);

void AppDisableOutput (void);
void SiiHdmiTxLiteHdmiDrvTmdsActive(void);

void	SiiDrvVideoMute( void );
void	SiiDrvVideoUnmute( void );

typedef enum
{
        AUTH_IDLE
        ,AUTH_PENDING
        ,AUTH_CURRENT
} AuthenticationState_e;
void SiiMhlTxLiteDrvSetAuthenticationState( AuthenticationState_e state);
AuthenticationState_e SiiMhlTxLiteDrvGetAuthenticationState( void );
#endif
