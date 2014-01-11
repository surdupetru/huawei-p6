/**********************************************************************************
 * Si8240 Linux Driver
 *
 * Copyright (C) 2011-2012 Silicon Image Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **********************************************************************************/

#define BUILD_NUMBER	62
#define BUILD_STRING	""
//#define BUILD_STRING	"PP Mode for Ops to test"

#define BUILD_CONFIG 1  /* CP8240 Starter Kit  -- see si_app_gpio.h */
//#define BUILD_CONFIG 2  /* CP8558 FPGA  -- see si_app_gpio.h */
#define BISHOP_DEVICE_ID 0x8558
#define WOLV60_DEVICE_ID 0x8240

#define ONE_EDID_BLOCK_PER_REQUEST
#define PARSE_EDID_MHL_3D
#define MHL2_0_SUPPORT
#define PACKED_PIXEL_SUPPORT
#define DEVCAP_CACHE
#define NEW_TIMER_API
#define NEW_SERIAL_CODE
#define eTMDS_SOURCE
#define	GET_DVI_MODE_FROM_SINK_EDID_NOT_REGISTER
#define	HW_ASSISTED_EDID_READ
#define ROKU_FIX
#define  PASS_980TEST
//#define  DO_HDCP
//#define THREE_D_SUPPORT

//#define PROCESS_BKSV_ERR
//#define EXTRA_TMDS_TOGGLE_FOR_HDCP_START


// debug only -- fake out the 3D write burst data
//#define FAKE_3D_DATA

#if 1 //(

#define BUILD_VARIATION "Drive Upstream HPD - Transcode mode supported"

#else //)(

#define BUILD_VARIATION "Release Upstream HPD control"
#define TRANSCODE_HPD_CONTROL

#endif //)
//#define AVI_PASSTHROUGH

// define this only if SYSTEM_BOARD == SB_EPV5_MARK_II
//#define ENABLE_OCS_OVERRIDE
#define PRINT_CONFIG   1

#if PRINT_CONFIG
#define SII_DEBUG_CONFIG_RESOURCE_CONSTRAINED
//#define ENABLE_APP_DEBUG_PRINT
#define ENABLE_TX_DEBUG_PRINT
#define ENABLE_PP_DEBUG_PRINT
#define ENABLE_EDID_DEBUG_PRINT
#define ENABLE_EDID_TRACE_PRINT
#define ENABLE_TX_EDID_PRINT
#define ENABLE_TX_PRUNE_PRINT
#define ENABLE_DUMP_INFOFRAME
#define ENABLE_COLOR_SPACE_DEBUG_PRINT
#define ENABLE_CBUS_DEBUG_PRINT
#define ENABLE_SCRPAD_DEBUG_PRINT
#define ENABLE_INFO_DEBUG_PRINT
#ifdef	DO_HDCP
#define ENABLE_HDCP_DEBUG_PRINT
#define ENABLE_HDCP_TRACE_PRINT
#endif
#define ENABLE_LITE_DEBUG_PRINT
#define  ENABLE_TXC_DEBUG_PRINT
#define ENABLE_TXD_DEBUG_PRINT
#define ENABLE_ERROR_DEBUG_PRINT
#define	EDID_DUMP_8240_BUFFER
#define	EDID_DUMP_SW_BUFFER
#endif

#define DEBUG
//#define	EDID_CHANGE_NOTIFICATION_REQUIRED
//#define	US_HPD_CONTROL_FROM_CBUS_INTERRUPT
//#define	SACHIKO_HDCP_OFF

//#define	EDID_READ_FIFO_BYTE_MODE
//#define DUMP_INTERRUPT_STATUS
//#define	DEBUG_RI_VALUES
#define	HANDLE_GCP_TO_MUTE
//#define	LEIS_REQUEST_TO_MAKE_2E3_0
//#define	MDT_TESTER

/* ************* Add by Huawei ***********************************/
#define SHARE_SWITCHER_WITH_USB
//#define CBUS_ID_SWITCHER_DEFAULT_OPEN
