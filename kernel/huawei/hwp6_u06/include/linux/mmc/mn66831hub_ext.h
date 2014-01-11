/*==============================================================================*/
/** @file      mn66831hub_ext.h
 *  @brief     Header file for mmc host driver.
 
               This program is free software; you can redistribute it and/or modify
               it under the terms of the GNU General Public License version 2 as
               published by the Free Software Foundation.

 *  @author    Copyright (C) 2011 Panasonic Corporation Semiconductor Company
 *  @attention Thing that information on SD Specification is not programmed in
               function called from this file.
 */
/*==============================================================================*/
#ifndef		_MN66831HUB_EXT_H_
#define		_MN66831HUB_EXT_H_

extern void SDHUB_SendCommand( int slot, unsigned int cmd, unsigned int arg, unsigned int *resp,
						void *buff, unsigned int size, unsigned int dir, unsigned int timeout, int *error ) ;



extern void SDHUB_GetRCA( int slot, void *rca ) ;
extern void SDHUB_GetCSD( int slot, void *csd ) ;
extern void SDHUB_GetCID( int slot, void *cid ) ;
extern void SDHUB_SeqLock( int lock ) ;

extern int SDHUB_CardInit( int slot ) ;

extern void SDHUB_Suspend( int suspend ) ;

#endif	/*	_MN66831HUB_EXT_H_	*/
