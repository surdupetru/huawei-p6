/*==============================================================================*/
/** @file      mn66831hub_api.h
 *  @brief     Header file for mmc host driver.
 
               This program is free software; you can redistribute it and/or modify
               it under the terms of the GNU General Public License version 2 as
               published by the Free Software Foundation.

 *  @author    Copyright (C) 2011 Panasonic Corporation Semiconductor Company
 *  @attention Thing that information on SD Specification is not programmed in
               function called from this file.
 */
/*==============================================================================*/
#ifndef		_MN66831HUB_API_H_
#define		_MN66831HUB_API_H_

/*  */
#define		SDHUB_PACKAGE_COUNT	(1)

/*  */
#define		SDHUB_DEVID0		(0)

/*  */
#define		MN66831_HUB0_ID		SDHUB_DEVID0

extern void SDHUB_Connect( struct mmc_host *master_host, unsigned int package_id ) ;
extern void SDHUB_Connect_hotplug( struct mmc_host *master_host, unsigned int package_id ) ;
extern void SDHUB_Disconnect( struct mmc_host *master_host, unsigned int package_id ) ;
extern int  SDHUB_Powerup_Vddio(void);
extern int  SDHUB_Set_LowPower_Mode(void);
extern int  SDHUB_Get_Vddio_Status(void);

#endif	/*	_MN66831HUB_API_H_	*/
