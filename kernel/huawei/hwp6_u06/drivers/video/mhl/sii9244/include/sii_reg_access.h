/***********************************************************************************/
/*  Copyright (c) 2002-2009, 2011 Silicon Image, Inc.  All rights reserved.        */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
#ifndef SII_REG_ACCESS
#define SII_REG_ACCESS

#include <linux/types.h>

/* Direct register access */

uint8_t	I2C_ReadByte(uint8_t deviceID, uint8_t offset);
void		I2C_WriteByte(uint8_t deviceID, uint8_t offset, uint8_t value);

uint8_t	ReadBytePage0 (uint8_t Offset);
void		WriteBytePage0 (uint8_t Offset, uint8_t Data);
void		ReadModifyWritePage0 (uint8_t Offset, uint8_t Mask, uint8_t Data);

uint8_t	ReadByteCBUS (uint8_t Offset);
void		WriteByteCBUS (uint8_t Offset, uint8_t Data);
void		ReadModifyWriteCBUS(uint8_t Offset, uint8_t Mask, uint8_t Value);

extern uint8_t PAGE_0_0X72;
extern uint8_t PAGE_1_0X7A;
extern uint8_t PAGE_2_0X92;
extern uint8_t PAGE_CBUS_0XC8;

#ifdef CONFIG_MHL_SII8240
bool GetMhlCableConnect(void);

#elif defined(CONFIG_MHL_SII9244)
void usb_switch_to_mhl(bool if_switch_to_MHL);
bool GetMhlCableConnect(void);
#endif

#endif
