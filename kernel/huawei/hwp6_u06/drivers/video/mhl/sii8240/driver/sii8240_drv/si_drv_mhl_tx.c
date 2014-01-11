/*
 *	si_drv_mhl_tx.c <Firmware or Driver> * *  Copyright (c)
 2002-2012, Silicon Image, Inc.  All rights reserved. *  No part of this
 work may be reproduced, modified, distributed, transmitted, *
 transcribed, or translated into any language or computer format, in any
 form *  or by any means without written permission of: Silicon Image,
 Inc., *  1140 East Arques Avenue, Sunnyvale, California 94085
 */

/*
 *   !file     si_drv_mhl_tx.c
 *   !brief    Silicon Image implementation of MHL driver.
 *
 */
#include "si_common.h"
#ifndef	__KERNEL__ //(
#include "hal_timers.h"
#endif //)

#include "si_cra.h"
#include "si_cra_cfg.h"
#include "si_bitdefs.h"
#include "si_mhl_defs.h"
#include "si_mhl_tx_api.h"
#include "si_mhl_tx_base_drv_api.h"  // generic driver interface to MHL tx component
#include "si_hdmi_tx_lite_api.h"
#include "si_hdmi_tx_lite_private.h"
#include "si_drv_hdmi_tx_lite_hdcp.h"
#include "si_8240_regs.h"
#include "si_tpi_regs.h"
#include "si_drv_mhl_tx.h"
#include "si_osscheduler.h"
#include "si_edid.h"
#include "si_app_gpio.h"
#include <hsad/config_mgr.h>
#ifdef K3_HDMI
#include "k3_hdmi.h"
#endif
#include <linux/usb/hiusb_android.h>


#define SILICON_IMAGE_ADOPTER_ID 322

#define TRANSCODER_DEVICE_ID TARGET_DEVICE_ID

uint8_t g_chipRevId;
uint16_t g_chipDeviceId;
#ifdef DO_HDCP
bool_t  DoHdcp=true;
#else
bool_t  DoHdcp=false;
#endif
#define Disconnected_USB_ID_with_CBUS_ID_As_defult  1

#define	INTR_4_DESIRED_MASK				(BIT0 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6)
#define	UNMASK_INTR_4_INTERRUPTS		SiiRegWrite(REG_INTR4_MASK, INTR_4_DESIRED_MASK)
#define	MASK_INTR_4_INTERRUPTS			SiiRegWrite(REG_INTR4_MASK, 0x00)

//
// Software power states are a little bit different than the hardware states but
// a close resemblance exists.
//
// D3 matches well with hardware state. In this state we receive RGND interrupts
// to initiate wake up pulse and device discovery
//
// Chip wakes up in D2 mode and interrupts MCU for RGND. Firmware changes the TX
// into D0 mode and sets its own operation mode as POWER_STATE_D0_NO_MHL because
// MHL connection has not yet completed.
//
// For all practical reasons, firmware knows only two states of hardware - D0 and D3.
//
// We move from POWER_STATE_D0_NO_MHL to POWER_STATE_D0_MHL only when MHL connection
// is established.
/*
//
//                             S T A T E     T R A N S I T I O N S
//
//
//                    POWER_STATE_D3                      POWER_STATE_D0_NO_MHL
//                   /--------------\                        /------------\
//                  /                \                      /     D0       \
//                 /                  \                \   /                \
//                /   DDDDDD  333333   \     RGND       \ /   NN  N  OOO     \
//                |   D     D     33   |-----------------|    N N N O   O     |
//                |   D     D  3333    |      IRQ       /|    N  NN  OOO      |
//                \   D     D      33  /               /  \                  /
//                 \  DDDDDD  333333  /                    \   CONNECTION   /
//                  \                /\                     /\             /
//                   \--------------/  \  TIMEOUT/         /  -------------
//                         /|\          \-------/---------/        ||
//                        / | \            500ms\                  ||
//                          |                     \                ||
//                          |  RSEN_LOW                            || MHL_EST
//                           \ (STATUS)                            ||  (IRQ)
//                            \                                    ||
//                             \      /------------\              //
//                              \    /              \            //
//                               \  /                \          //
//                                \/                  \ /      //
//                                 |    CONNECTED     |/======//
//                                 |                  |\======/
//                                 \   (OPERATIONAL)  / \
//                                  \                /
//                                   \              /
//                                    \-----------/
//                                   POWER_STATE_D0_MHL
//
//
//
*/
#define	POWER_STATE_D3				3
#define	POWER_STATE_D0_NO_MHL		2
#define	POWER_STATE_D0_MHL			0
#define	POWER_STATE_FIRST_INIT		0xFF

#define TX_HW_RESET_PERIOD		10	// 10 ms.
#define TX_HW_RESET_DELAY			100
#define TX_EDID_POLL_MAX            256
#define TX_CKDT_POLL_INTERVAL       100

typedef struct _StateFlags_t
{
        uint8_t     upStreamMuted  : 1;
        uint8_t     reserved       : 7;
} StateFlags_t,*PStateFlags_t;
//
// To remember the current power state.
//
static	uint8_t	fwPowerState = POWER_STATE_FIRST_INIT;
#define UpdatePowerState(x) \
{ \
    fwPowerState = x; \
    TXD_DEBUG_PRINT(("fwPowerState = %d\n",(uint16_t)x)); \
}

//
// To serialize the RCP commands posted to the CBUS engine, this flag
// is maintained by the function SiiMhlTxDrvSendCbusCommand()
//
static bool_t	mscCmdInProgress;	// false when it is okay to send a new command
static uint8_t  cache_TPI_SYSTEM_CONTROL_DATA_REG=0;
#define WriteTpiSystemControlDataReg(value) \
{   \
    SiiRegWrite(TPI_SYSTEM_CONTROL_DATA_REG,cache_TPI_SYSTEM_CONTROL_DATA_REG=value); \
    HDCP_DEBUG_PRINT(("TPI_SYSTEM_CONTROL_DATA_REG: %02x\n",(uint16_t) cache_TPI_SYSTEM_CONTROL_DATA_REG)); \
}

#define ReadTpiSystemControlDataReg(dummy) (cache_TPI_SYSTEM_CONTROL_DATA_REG= SiiRegRead(TPI_SYSTEM_CONTROL_DATA_REG))
#define ModifyTpiSystemControlDataReg(mask,value) \
{ \
	uint8_t holder = cache_TPI_SYSTEM_CONTROL_DATA_REG; \
	cache_TPI_SYSTEM_CONTROL_DATA_REG &= ~(mask); \
	cache_TPI_SYSTEM_CONTROL_DATA_REG |= ((mask) & value); \
	if (holder != cache_TPI_SYSTEM_CONTROL_DATA_REG) \
	{ \
		SiiRegWrite(TPI_SYSTEM_CONTROL_DATA_REG,cache_TPI_SYSTEM_CONTROL_DATA_REG); \
		HDCP_DEBUG_PRINT(("TPI_SYSTEM_CONTROL_DATA_REG: %02x\n",(uint16_t) cache_TPI_SYSTEM_CONTROL_DATA_REG)); \
	} \
}

static uint8_t cache_REG_EDID_CTRL;
static uint8_t cache_REG_HPD_CTRL;
static uint8_t cache_REG_TPI_SEL;
//
// Preserve Downstream HPD status
//
static	uint8_t	dsHpdStatus = 0;
#ifdef SWWA_INFOFRAME //(
typedef enum
{
        cdsFirstTime
        ,cdsTriggered
        ,cdsComplete
} ClockDetectState_e;
static uint8_t ckdtStatus = cdsFirstTime;
static uint8_t old_r281;
#endif //)
static  uint8_t upStreamHPD = 0;
static  uint8_t g_modeFlags = 0;
static  StateFlags_t stateFlags = {0,0};

#define SetUpStreamHPDStatus(x) { upStreamHPD = x; ERROR_DEBUG_PRINT(("Setting US HPD = %s\n",upStreamHPD?"HIGH":"LOW")); }

#ifdef INTERRUPT_DRIVEN_EDID //(
static  uint8_t currentEdidRequestBlock;
static  uint8_t edidFifoBlockNumber=0;
#endif //)

static uint8_t hwInputColorSpace  = BIT_TPI_INPUT_FORMAT_RGB;
static uint8_t hwOutputColorSpace = BIT_TPI_OUTPUT_FORMAT_HDMI_TO_RGB;
static QuantizationSettings_e hwInputQuantizationSettings = qsAutoSelectByColorSpace;
static QuantizationSettings_e hwOutputQuantizationSettings = qsAutoSelectByColorSpace;
static AviInfoFrameDataByte2_t colorimetryAspectRatio = {0x8,0x01,0x00 };//0x18;
static AviInfoFrameDataByte4_t inputVideoCode = {2,0}; //2
static uint8_t packedPixelStatus=0;
static uint32_t pixelClockFrequency=0;
static uint8_t	mdt_tester = 0;
uint8_t SiiDrvMipiGetSourceStatus( void )
{
        return 1;
}

static PLACE_IN_CODE_SEG uint8_t colorSpaceTranslateInfoFrameToHw[]=
{
        BIT_TPI_INPUT_FORMAT_RGB
        ,BIT_TPI_INPUT_FORMAT_YCbCr422
        ,BIT_TPI_INPUT_FORMAT_YCbCr444
        ,BIT_TPI_INPUT_FORMAT_INTERNAL_RGB // reserved for future
};
static PLACE_IN_CODE_SEG AviColorSpace_e colorSpaceTranslateHwToInfoFrame[] =
{
        acsRGB
        ,acsYCbCr444
        ,acsYCbCr422
        ,acsFuture
};


void SiiMhlTxDrvSetPixelClockFrequency(uint32_t pixelClockFrequencyParm)
{
        pixelClockFrequency = pixelClockFrequencyParm;
}
void SiiMhlTxDrvSetOutputColorSpaceImpl(uint8_t  outputClrSpc)
{
        hwOutputColorSpace = colorSpaceTranslateInfoFrameToHw[outputClrSpc];
}
void SiiMhlTxDrvSetInputColorSpaceImpl(uint8_t inputClrSpc)
{
        hwInputColorSpace = colorSpaceTranslateInfoFrameToHw[inputClrSpc];
}

#ifdef ENABLE_COLOR_SPACE_DEBUG_PRINT //(

void PrintColorSpaceSettingsImpl(char *pszId,int iLine)
{
        COLOR_SPACE_DEBUG_PRINT(("\n%s:%d\n"
                                 "\t\tICS: sw:%02x hw:%02x \n"
                                 "\t\tOCS: sw:%02x hw:%02x\n\n"
                                 ,pszId,iLine
                                 ,(uint16_t)colorSpaceTranslateHwToInfoFrame[hwInputColorSpace]
                                 ,(uint16_t)hwInputColorSpace
                                 ,(uint16_t)colorSpaceTranslateHwToInfoFrame[hwOutputColorSpace]
                                 ,(uint16_t)hwOutputColorSpace
                                ));
}

#define PrintColorSpaceSettings(id,line) PrintColorSpaceSettingsImpl(id,line);

void SiiMhlTxDrvSetOutputColorSpaceWrapper(char *pszId,int iLine,uint8_t  outputClrSpc)
{
        SiiMhlTxDrvSetOutputColorSpaceImpl(outputClrSpc);
        PrintColorSpaceSettings(pszId,iLine)
}

void SiiMhlTxDrvSetInputColorSpaceWrapper(char *pszId,int iLine,uint8_t inputClrSpc)
{
        SiiMhlTxDrvSetInputColorSpaceImpl(inputClrSpc);
        PrintColorSpaceSettings(pszId,iLine)
}

#else //)(

#define PrintColorSpaceSettings(id,line)

#endif //)


void InitTranscodeMode(uint8_t enableDiscovery)
{
        SiiRegWrite(REG_TMDS_CLK_EN, 0x01); // Enable TxPLL clock
        SiiRegWrite(REG_TMDS_CH_EN, 0x11); // Enable Tx clock path & Equalizer
        SiiRegWrite(REG_TMDS_CCTRL, BIT_TMDS_CCTRL_BGRCTL_MASK & 4); // Enable Rx PLL clock
        SiiRegWrite(REG_DISC_CTRL1, enableDiscovery?0x27:0x26); // wait until just before D3 to enable discovery
}

//-------------------------------------------------------------------------------------------------
//! @brief      Driver API to set or clear MHL 2 enhancement of higher modes/freq. Called only when dcap read is done.
//!
//! @param[in]  supportPackedPixel - indicate if video link will be supporting packed pixel as per peer's devcap
//! @retval     None
//-------------------------------------------------------------------------------------------------
void	SiiMhlTxDrvSetPackedPixelStatus( int supportPackedPixel )
{
        EDID_DEBUG_PRINT(("Setting Packed Pixel = %02X\n", supportPackedPixel));
        packedPixelStatus = supportPackedPixel;
}

uint8_t SiiMhlTxDrvGetPackedPixelStatus( void )
{
        return packedPixelStatus;
}

void SiiMhlTxDrvSetInputQuantizationRange(QuantizationSettings_e qsData)
{
        hwInputQuantizationSettings = qsData;
}

void SiiMhlTxDrvSetOutputQuantizationRange(QuantizationSettings_e qsData)
{
        hwOutputQuantizationSettings = qsData;
}

void SiiMhlTxDrvSet3DMode(uint8_t do3D,uint8_t three3ModeParm)
{

        if (do3D)
        {
                ThreeDStructure_e three3Mode = (ThreeDStructure_e) three3ModeParm;
                if (tdsFramePacking == three3Mode)
                {
                        PP_DEBUG_PRINT(("Drv: using frame packing\n"));
                        SiiRegModify(REG_VID_OVRRD,BIT_VID_OVRRD_3DCONV_EN_MASK,BIT_VID_OVRRD_3DCONV_EN_FRAME_PACK);
                }
                else
                {
                        PP_DEBUG_PRINT(("Drv: NOT using frame packing\n"));
                        SiiRegModify(REG_VID_OVRRD,BIT_VID_OVRRD_3DCONV_EN_MASK,BIT_VID_OVRRD_3DCONV_EN_NORMAL);
                }
        }
        else
        {
                PP_DEBUG_PRINT(("Drv: NOT using frame packing\n"));
                SiiRegModify(REG_VID_OVRRD,BIT_VID_OVRRD_3DCONV_EN_MASK,BIT_VID_OVRRD_3DCONV_EN_NORMAL);
        }
}

void SiiMhlTxDrvSetHdmiMode( void )
{
#ifndef LET_HW_DECIDE_UPSTREAM_DVI //(
        // Page2.0xA1[2] = 1
        SiiRegModify(REG_RX_HDMI_CTRL0
                     , BIT_REG_RX_HDMI_CTRL0_hdmi_mode_overwrite_MASK
                     , BIT_REG_RX_HDMI_CTRL0_hdmi_mode_overwrite_SW_CTRL
                    );

        // Page2.0xA1[3] = 1
        SiiRegModify(REG_RX_HDMI_CTRL0
                     , BIT_REG_RX_HDMI_CTRL0_hdmi_mode_sw_value_MASK
                     , BIT_REG_RX_HDMI_CTRL0_hdmi_mode_sw_value_HDMI
                    );
#endif //)

        TXD_DEBUG_PRINT(("SiiMhlTxDrvSetHdmiMode (HDMI)\n"));
        ModifyTpiSystemControlDataReg(TMDS_OUTPUT_MODE_MASK, TMDS_OUTPUT_MODE_HDMI)
        SiiRegModify(REG_RX_HDMI_CTRL2
                     ,BIT_RX_HDMI_CTRL2_USE_AV_MUTE_SUPPORT_MASK
                     ,BIT_RX_HDMI_CTRL2_USE_AV_MUTE_SUPPORT_ENABLE
                    );
}

void SiiMhlTxDrvSetDviMode( void )
{
#ifndef LET_HW_DECIDE_UPSTREAM_DVI //(
        // Page2.0xA1[2] = 1
        SiiRegModify(REG_RX_HDMI_CTRL0
                     , BIT_REG_RX_HDMI_CTRL0_hdmi_mode_overwrite_MASK
                     , BIT_REG_RX_HDMI_CTRL0_hdmi_mode_overwrite_SW_CTRL
                    );

        // Page2.0xA1[3] = 0
        SiiRegModify(REG_RX_HDMI_CTRL0
                     , BIT_REG_RX_HDMI_CTRL0_hdmi_mode_sw_value_MASK
                     , BIT_REG_RX_HDMI_CTRL0_hdmi_mode_sw_value_DVI
                    );
#endif //)

        TXD_DEBUG_PRINT(("SiiMhlTxDrvSetDviMode (DVI)\n"));
        ModifyTpiSystemControlDataReg(TMDS_OUTPUT_MODE_MASK, TMDS_OUTPUT_MODE_DVI)
        SiiRegModify(REG_RX_HDMI_CTRL2
                     ,BIT_RX_HDMI_CTRL2_USE_AV_MUTE_SUPPORT_MASK
                     ,BIT_RX_HDMI_CTRL2_USE_AV_MUTE_SUPPORT_DISABLE
                    );
}

void SiiMhlTxDrvApplyColorSpaceSettings( void)
{
        //( 2012-05-25 bugzilla 24790
        if (mfdrvTranscodeMode & g_modeFlags)
        {
                TXD_DEBUG_PRINT(("Drv: Transcode zone control: D0\n"));
                SiiRegWrite(REG_ZONE_CTRL_SW_RST,0xD0);
        }
        else
        {
                if (pixelClockFrequency > 75000000)
                {
                        TXD_DEBUG_PRINT(("Drv: zone control: D0\n"));
                        SiiRegWrite(REG_ZONE_CTRL_SW_RST,0xD0);
                }
                else
                {
                        TXD_DEBUG_PRINT(("Drv: zone control: E0\n"));
                        SiiRegWrite(REG_ZONE_CTRL_SW_RST,0xE0);
                }
                TXD_DEBUG_PRINT(("Drv: pixel clock:%ld %04x rev %02x\n",pixelClockFrequency,g_chipDeviceId,(uint16_t)g_chipRevId));
                // don't do this for rev 0.0 of 8240 and 8558 (no others supported in this source code base)
                if (g_chipRevId > 0)
                {
                        if (pixelClockFrequency < 30000000)
                        {
                                SiiRegWrite(REG_TXMZ_CTRL2,0x01);
                        }
                        else
                        {
                                SiiRegWrite(REG_TXMZ_CTRL2,0x00);
                        }
                }
        }
        //)

        if(sink_type_HDMI == SiiMhlTxGetSinkType())
        {
                SiiMhlTxDrvSetHdmiMode();
        }
        else
        {
                SiiMhlTxDrvSetDviMode();
        }

        if (packedPixelStatus)
        {
                // PackedPixel Mode
                SiiRegModify(REG_VID_MODE,BIT_VID_MODE_m1080p_MASK,BIT_VID_MODE_m1080p_ENABLE); // Packed Pixel mode enabled.
                SiiRegModify(REG_MHLTX_CTL4,BIT_MHLTX_CTL4_MHL_CLK_RATIO_MASK,BIT_MHLTX_CTL4_MHL_CLK_RATIO_2X);  // Bit6 set to 0 for PP Mode clock 2x.

                SiiRegWrite (REG_MHLTX_CTL6, 0x60);  // rcommon mode clock
                PP_DEBUG_PRINT(("Drv: Using 16-bit mode (Packed Pixel)\n"));
        }
        else
        {
                // normal Mode
                // Packed Pixel mode disabled.
                SiiRegModify(REG_VID_MODE,BIT_VID_MODE_m1080p_MASK,BIT_VID_MODE_m1080p_DISABLE);

                // Bit6 set to 1 for normal Mode clock 3x.
                SiiRegModify(REG_MHLTX_CTL4,BIT_MHLTX_CTL4_MHL_CLK_RATIO_MASK,BIT_MHLTX_CTL4_MHL_CLK_RATIO_3X);

                SiiRegWrite (REG_MHLTX_CTL6, 0xA0);  // rcommon mode clock
                PP_DEBUG_PRINT(("Drv: Using 24-bit mode (non-Packed Pixel)\n"));
        }
        // Set input color space
        SiiRegModify(REG_TPI_INPUT
                     , BIT_TPI_INPUT_FORMAT_MASK  | BIT_TPI_INPUT_QUAN_RANGE_MASK
                     , hwInputColorSpace          | (hwInputQuantizationSettings << 2)
                    );
        // Set output color space
        SiiRegModify(REG_TPI_OUTPUT
                     , BIT_TPI_OUTPUT_FORMAT_MASK | BIT_TPI_OUTPUT_QUAN_RANGE_MASK
                     , hwOutputColorSpace         | (hwOutputQuantizationSettings<< 2)
                    );

        PrintColorSpaceSettings("SiiMhlTxDrvApplyColorSpaceSettings",__LINE__)

}

AviColorSpace_e SiiMhlTxDrvGetOutputColorSpace(void)
{
        AviColorSpace_e retVal;
        retVal = colorSpaceTranslateHwToInfoFrame[hwOutputColorSpace];
        COLOR_SPACE_DEBUG_PRINT(("hwOutputColorSpace:0x%02x retVal:0x%02x\n",(uint16_t)hwOutputColorSpace,(uint16_t)retVal));
        return retVal;
}

uint8_t SiiMhlTxDrvGetInputColorSpace( void )
{
        uint8_t retVal;
        retVal = colorSpaceTranslateHwToInfoFrame[hwInputColorSpace];
        COLOR_SPACE_DEBUG_PRINT(("hwInputColorSpace:0x%02x retVal:0x%02x\n",(uint16_t)hwInputColorSpace,(uint16_t)retVal));
        return retVal;
}

void SiiMhlTxDrvSetColorimetryAspectRatio (AviInfoFrameDataByte2_t colAspRat )
{
        colorimetryAspectRatio = colAspRat;
}

AviInfoFrameDataByte2_t SiiMhlTxDrvGetColorimetryAspectRatio ( void )
{
        return colorimetryAspectRatio;
}

void SiiMhlTxDrvSetInputVideoCode (AviInfoFrameDataByte4_t inputVIC)
{
        inputVideoCode = inputVIC;
}

AviInfoFrameDataByte4_t SiiMhlTxDrvGetInputVideoCode ( void )
{
        return inputVideoCode;
}

uint16_t SiiMhlTxDrvGetIncomingHorzTotal(void)
{
        uint16_t retVal;
        retVal = (((uint16_t)SiiRegRead(REG_HRESH)) <<8) | (uint16_t)SiiRegRead(REG_HRESL);
        return retVal;
}

uint16_t SiiMhlTxDrvGetIncomingVertTotal(void)
{
        uint16_t retVal;
        retVal = (((uint16_t)SiiRegRead(REG_VRESH)) <<8) | (uint16_t)SiiRegRead(REG_VRESL);
        return retVal;
}


#define	SET_BIT(offset,bitnumber)		SiiRegModify(offset,(1<<bitnumber),(1<<bitnumber))
#define	CLR_BIT(offset,bitnumber)		SiiRegModify(offset,(1<<bitnumber),0x00)
//
//
#define	DISABLE_DISCOVERY				SiiRegModify(REG_DISC_CTRL1,BIT0,0);
#define	ENABLE_DISCOVERY				SiiRegModify(REG_DISC_CTRL1,BIT0,BIT0);

#define STROBE_POWER_ON					SiiRegModify(REG_DISC_CTRL1,BIT1,0);

typedef struct _InterruptEnableMaskInfo_t
{
        uint16_t regNameStatus;
        uint16_t regNameMask;
        uint8_t  value;
} InterruptEnableMaskInfo_t,*PInterruptEnableMaskInfo_t;

PLACE_IN_CODE_SEG InterruptEnableMaskInfo_t  g_TranscodeInterruptMasks[]=
{
        {REG_INTR7         ,REG_INTR7_MASK            , 0 }
        ,{REG_INTR8         ,REG_INTR8_MASK            , 0 }
        ,{REG_TPI_INTR_ST0  ,REG_TPI_INTR_ST0_ENABLE   , 0 }
};

PLACE_IN_CODE_SEG InterruptEnableMaskInfo_t  g_NonTranscodeInterruptMasks[]=
{
        {
                REG_INTR7         ,REG_INTR7_MASK
                ,(
                        BIT_INTR7_CEA_NO_AVI
                        | BIT_INTR7_CEA_NO_VSI
                        //  | BIT_INTR7_CP_NEW_CP
                        | BIT_INTR7_CP_SET_MUTE
                        | BIT_INTR7_CP_CLR_MUTE
                )
        }
        ,{
                REG_INTR8         ,REG_INTR8_MASK
                ,(
                        BIT_INTR8_CEA_NEW_AVI
                        | BIT_INTR8_CEA_NEW_VSI
                )
        }
#ifdef	DO_HDCP
        ,{
                REG_TPI_INTR_ST0  ,REG_TPI_INTR_ST0_ENABLE
                ,(
                        BIT_TPI_INTR_ST0_HDCP_AUTH_STATUS_CHANGE_EVENT
                        | BIT_TPI_INTR_ST0_HDCP_VPRIME_VALUE_READY_EVENT
                        | BIT_TPI_INTR_ST0_HDCP_SECURITY_CHANGE_EVENT
                        | BIT_TPI_INTR_ST0_BKSV_DONE
                        // | BIT_TPI_INTR_ST0_BKSV_ERR
                )
        }
#endif
};

PLACE_IN_CODE_SEG InterruptEnableMaskInfo_t g_InterruptMasks[]=
{
        {
                REG_CBUS_INT_0 ,REG_CBUS_INT_0_MASK
                ,(
                        BIT_CBUS_CNX_CHG
                        | BIT_CBUS_MSC_MT_DONE
                        | BIT_CBUS_HPD_RCVD
                        | BIT_CBUS_MSC_MR_WRITE_STAT
                        | BIT_CBUS_MSC_MR_MSC_MSG
                        | BIT_CBUS_MSC_MR_WRITE_BURST
                        | BIT_CBUS_MSC_MR_SET_INT
                        | BIT_CBUS_MSC_MT_DONE_NACK
                )
        }
        ,{
                REG_CBUS_INT_1 ,REG_CBUS_INT_1_MASK
                ,(
                        BIT_CBUS_DDC_ABRT
                        | BIT_CBUS_CEC_ABRT
                        | BIT_CBUS_CMD_ABORT
                )
        }
#ifdef PROCESS_INTR1 //(
        ,{
                REG_INTR1 ,REG_INTR1_MASK
                ,(
                        BIT_INTR1_HPD_CHG
                        | BIT_INTR1_RSEN_CHG
                )
        }
#endif //)
        ,{REG_INTR2 ,REG_INTR2_MASK            ,0}
        ,{REG_INTR3 ,REG_INTR3_MASK            ,0}
#define DEFAULT_INTR4_MASK      ( \
                                      BIT_INTR4_VBUS_CHG \
                                    | BIT_INTR4_MHL_EST   \
                                    | BIT_INTR4_NON_MHL_EST \
                                    | BIT_INTR4_CBUS_LKOUT \
                                    | BIT_INTR4_CBUS_DISCONNECT \
                                    | BIT_INTR4_RGND_DETECTION \
                                    | BIT_INTR4_SOFT \
                                )

        ,{REG_INTR4 ,REG_INTR4_MASK ,DEFAULT_INTR4_MASK}
        ,{
                REG_INTR5 ,REG_INTR5_MASK
                ,(
                        //BIT_INTR5_CKDT_CHANGE|
                        BIT_INTR5_SCDT_CHANGE
                )
        }
        ,{
                REG_INTR9 ,REG_INTR9_MASK
                ,(
                        BIT_INTR9_DEVCAP_DONE_MASK
#ifdef HW_ASSISTED_EDID_READ //(
                        | BIT_INTR9_EDID_DONE_MASK
                        | BIT_INTR9_EDID_ERROR
#endif //)
                )
        }
};


#define I2C_INACCESSIBLE -1
#define I2C_ACCESSIBLE 1

//
// Local scope functions.
//
static void Int1Isr (void);
static int  Int4Isr (void);
static void Int5Isr (void);
static void MhlCbusIsr (void);

static void SiiMhlTxDrvEnableInterruptMasks (void);
void SiiMhlTxDrvDisableAllInterruptsExceptRgnd( void );
static void SwitchToD0 (void);
static void SwitchToD3 (void);
static void WriteInitialRegisterValuesPartOne (void);
static void WriteInitialRegisterValuesPartTwo (uint8_t enableDiscovery);
static void InitCBusRegs (void);
static void ForceUsbIdSwitchOpen (void);
static void ReleaseUsbIdSwitchOpen (void);
static void MhlTxDrvProcessConnection (void);
static void MhlTxDrvProcessDisconnection (void);

// make sure that TMDS is not enabled prior to enabling TPI
#define SetTPIMode \
{ \
	TXD_DEBUG_PRINT(("Drv: REG_TPI_SEL:%02x\n",(uint16_t)cache_REG_TPI_SEL)); \
	CachedRegModify(REG_TPI_SEL,BIT_TPI_SEL_SW_TPI_EN_MASK,BIT_TPI_SEL_SW_TPI_EN_HW_TPI); \
	TXD_DEBUG_PRINT(("Drv: REG_TPI_SEL:%02x\n",(uint16_t)cache_REG_TPI_SEL)); \
}
#define ClrTPIMode CachedRegModify(REG_TPI_SEL,BIT_TPI_SEL_SW_TPI_EN_MASK ,BIT_TPI_SEL_SW_TPI_EN_NON_HW_TPI);

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvAcquireUpstreamHPDControl
//
// Acquire the direct control of Upstream HPD.
//
void SiiMhlTxDrvAcquireUpstreamHPDControl ( void )
{
        // set reg_hpd_out_ovr_en to first control the hpd
        CachedRegModify(REG_HPD_CTRL
                        , BIT_HPD_CTRL_HPD_OUT_OVR_EN_MASK
                        , BIT_HPD_CTRL_HPD_OUT_OVR_EN_ON
                       );
        TXD_DEBUG_PRINT(("Drv: Upstream HPD Acquired.\n"));
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow
//
// Acquire the direct control of Upstream HPD.
//
void SiiMhlTxDrvAcquireUpstreamHPDControlDriveLowImpl ( void )
{
        CachedRegModify (REG_EDID_CTRL
                         , BIT_EDID_CTRL_EDID_PRIME_VALID_MASK
                         , BIT_EDID_CTRL_EDID_PRIME_VALID_DISABLE
                        );
        // set reg_hpd_out_ovr_en to first control the hpd and clear reg_hpd_out_ovr_val
        CachedRegModify(REG_HPD_CTRL
                        , (BIT_HPD_CTRL_HPD_OUT_OVR_VAL_MASK | BIT_HPD_CTRL_HPD_OUT_OVR_EN_MASK)
                        , (BIT_HPD_CTRL_HPD_OUT_OVR_VAL_OFF  | BIT_HPD_CTRL_HPD_OUT_OVR_EN_ON)
                       );
        SetUpStreamHPDStatus(0);
        SiiMhlTxNotifyUpStreamHPD(upStreamHPD);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvAcquireUpstreamHPDControlDriveHigh
//
// Acquire the direct control of Upstream HPD.
//
void SiiMhlTxDrvAcquireUpstreamHPDControlDriveHighImpl ( void )
{
        // set reg_hpd_out_ovr_en to first control the hpd and clear reg_hpd_out_ovr_val
        CachedRegModify(REG_HPD_CTRL
                        , (BIT_HPD_CTRL_HPD_OUT_OVR_VAL_MASK | BIT_HPD_CTRL_HPD_OUT_OVR_EN_MASK)
                        , (BIT_HPD_CTRL_HPD_OUT_OVR_VAL_ON   | BIT_HPD_CTRL_HPD_OUT_OVR_EN_ON)
                       );

        SetUpStreamHPDStatus(1);
#ifdef DO_HDCP
        SiiMhlTxLiteDrvSetAuthenticationState(AUTH_IDLE);
#endif
        SiiMhlTxNotifyUpStreamHPD(upStreamHPD);
}


///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvReleaseUpstreamHPDControl
//
// Release the direct control of Upstream HPD.
//
void SiiMhlTxDrvReleaseUpstreamHPDControlImpl ( void )
{
        // Un-force HPD (it was kept low, now propagate to source)
        // let HPD float by clearing reg_hpd_out_ovr_en
        CachedRegModify(REG_HPD_CTRL
                        , BIT_HPD_CTRL_HPD_OUT_OVR_EN_MASK
                        , BIT_HPD_CTRL_HPD_OUT_OVR_EN_OFF
                       );
}

#ifdef ENABLE_TXD_DEBUG_PRINT //(

void SiiMhlTxDrvAcquireUpstreamHPDControlDriveLowWrapper (char *pszFile,int iLine)
{
        SiiMhlTxDrvAcquireUpstreamHPDControlDriveLowImpl();
        TXD_DEBUG_PRINT(("Drv: Upstream HPD Acquired - driven low.called from %s:%d\n",pszFile,iLine));
}
#define SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow() SiiMhlTxDrvAcquireUpstreamHPDControlDriveLowWrapper(__FILE__,__LINE__)

void SiiMhlTxDrvAcquireUpstreamHPDControlDriveHighWrapper(char *pszFile,int iLine)
{
        SiiMhlTxDrvAcquireUpstreamHPDControlDriveHighImpl();
        TXD_DEBUG_PRINT(("Drv: Upstream HPD Acquired - driven high. called from %s:%d\n",pszFile,iLine));
}
#define SiiMhlTxDrvAcquireUpstreamHPDControlDriveHigh() SiiMhlTxDrvAcquireUpstreamHPDControlDriveHighWrapper(__FILE__,__LINE__)

void SiiMhlTxDrvReleaseUpstreamHPDControlWrapper(char *pszFile,int iLine)
{
        SiiMhlTxDrvReleaseUpstreamHPDControlImpl();
        TXD_DEBUG_PRINT(("Drv: Upstream HPD released. called from %s:%d\n",pszFile,iLine));
}
#define SiiMhlTxDrvReleaseUpstreamHPDControl()         SiiMhlTxDrvReleaseUpstreamHPDControlWrapper (__FILE__,__LINE__)

#else //)(

#define SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow() SiiMhlTxDrvAcquireUpstreamHPDControlDriveLowImpl()
#define SiiMhlTxDrvAcquireUpstreamHPDControlDriveHigh() SiiMhlTxDrvAcquireUpstreamHPDControlDriveHighImpl()
#define SiiMhlTxDrvReleaseUpstreamHPDControl() SiiMhlTxDrvReleaseUpstreamHPDControlImpl ()

#endif //)

static void Int1Isr(void)
{
#ifdef PROCESS_INTR1 //(
        uint8_t regIntr1;
        regIntr1 = SiiRegRead(REG_INTR1);
        if (regIntr1)
        {
                // Clear all interrupts coming from this register.
                SiiRegWrite(REG_INTR1,regIntr1);
        }
#endif //)
}

void    SiiMhlTxDrvTimerHDCPCallBack(void)
{
        TXD_DEBUG_PRINT(("TIMER_HDCP expired\n"));
}


#ifdef ENABLE_HDCP_DEBUG_PRINT //(

void DumpTPIDebugRegsImpl(void)
{
        uint8_t debugStatus[7];
        SiiRegReadBlock(REG_TPI_HW_DBG1,debugStatus,sizeof(debugStatus));
        INFO_DEBUG_PRINT((
                                 "HDCP: debugStatus[]={%02x %02x %02x %02x %02x %02x %02x}\n"
                                 , (uint16_t)debugStatus[0]
                                 , (uint16_t)debugStatus[1]
                                 , (uint16_t)debugStatus[2]
                                 , (uint16_t)debugStatus[3]
                                 , (uint16_t)debugStatus[4]
                                 , (uint16_t)debugStatus[5]
                                 , (uint16_t)debugStatus[6]
                         ));
}
#define DumpTPIDebugRegs DumpTPIDebugRegsImpl();

#else //)(

#define DumpTPIDebugRegs /* nothing */

#endif //)

#ifdef	DO_HDCP
static void HdcpIsr(void)
{
        uint8_t tpiIntStatus;
        tpiIntStatus = SiiRegRead(REG_TPI_INTR_ST0);
        if (tpiIntStatus)
        {
                uint8_t queryData = SiiRegRead(TPI_HDCP_QUERY_DATA_REG);
#ifdef ENABLE_HDCP_DEBUG_PRINT //(
                uint8_t debugStatus[7];
                SiiRegReadBlock(REG_TPI_HW_DBG1,debugStatus,sizeof(debugStatus));
#endif //)
                ERROR_DEBUG_PRINT((" hdcp [3d] = %02X. [29]=%02X\n", (int)tpiIntStatus, (int)queryData));

                // ALWAYS clear interrupt status BEFORE processing!
                SiiRegWrite(REG_TPI_INTR_ST0,tpiIntStatus);

                HDCP_DEBUG_PRINT(("HDCP: debugStatus[]={%02x %02x %02x %02x %02x %02x %02x}\n\n"
                                  , (uint16_t)debugStatus[0]
                                  , (uint16_t)debugStatus[1]
                                  , (uint16_t)debugStatus[2]
                                  , (uint16_t)debugStatus[3]
                                  , (uint16_t)debugStatus[4]
                                  , (uint16_t)debugStatus[5]
                                  , (uint16_t)debugStatus[6]
                                 ));

#if 0 //def ENABLE_HDCP_DEBUG_PRINT //(
                while (0x10 == debugStatus[5])
                {
                        uint8_t riStatus[2];
                        uint8_t temp = SiiRegRead(REG_INTR2);
                        uint8_t intStatus = SiiRegRead(REG_TPI_INTR_ST0);
                        uint8_t riPeerStatus[2];
                        SiiRegReadBlock(TPI_HDCP_RI_LOW_REG,riStatus,sizeof(riStatus));
                        SiiRegReadBlock(REG_PEER_RI_RX_1,riPeerStatus,sizeof(riPeerStatus));
                        SiiRegReadBlock(REG_TPI_HW_DBG1,debugStatus,sizeof(debugStatus));
                        if (0x10 == debugStatus[5])
                        {
                                ERROR_DEBUG_PRINT(("Intr2 status:%02x intStatus:%02x riLow: %02x riHigh:%02x\n"
                                                   ,(uint16_t)temp
                                                   ,(uint16_t)intStatus
                                                   ,(uint16_t)riStatus[0]
                                                   ,(uint16_t)riStatus[1]
                                                  ));
                                HalTimerWait(10);
                        }
                }
#endif //)

                HDCP_DEBUG_PRINT(("REG_TPI_INTR_ST0: %02x qd: %02x\n",(uint16_t)tpiIntStatus,(uint16_t)queryData));
                if (SiiMhlTxReadyForVideo())
                {
#ifdef DO_HDCP
                        HDCP_DEBUG_PRINT(("HdcpIsr int7 mask:0x%x\n",(uint16_t)SiiRegRead(REG_INTR7_MASK)));
                        if (0==(TMDS_OUTPUT_CONTROL_POWER_DOWN & cache_TPI_SYSTEM_CONTROL_DATA_REG))
                        {
                                SiiHdmiTxLiteHandleEvents(tpiIntStatus,queryData);
                        }
#endif
                }
                else
                {
                        INFO_DEBUG_PRINT(("Unexpected HDCP interrupt\n"));
                }
        }

}
#endif
////////////////////////////////////////////////////////////////////
//
// E X T E R N A L L Y    E X P O S E D   A P I    F U N C T I O N S
//
////////////////////////////////////////////////////////////////////

#ifdef	MDT_TESTER //(
static	uint8_t	outBuffer[17] = {0x0f};
static	uint8_t	inBuffer [17] = {0x0f};
extern void *memset  (void *s, char val, int n);
void	MdtErrorHalt(void)
{
        int	i;

        SiiOsDebugPrintAlwaysShort(("\n\n\n\n\MDT_ERROR: HALTED\n\n\n\nRegister values...."));

        SiiOsDebugPrintAlwaysShort("Reg 0x86 = %02X\n", (int)SiiRegRead(TX_PAGE_CBUS | 0x0086));
        SiiOsDebugPrintAlwaysShort("Reg 0x87 = %02X\n", (int)SiiRegRead(TX_PAGE_CBUS | 0x0087));
        SiiOsDebugPrintAlwaysShort("Reg 0x88 = %02X\n", (int)SiiRegRead(TX_PAGE_CBUS | 0x0088));
        SiiOsDebugPrintAlwaysShort("Reg 0x8A = %02X\n", (int)SiiRegRead(TX_PAGE_CBUS | 0x008A));
        SiiOsDebugPrintAlwaysShort("Reg 0x8B = %02X\n", (int)SiiRegRead(TX_PAGE_CBUS | 0x008B));
        SiiOsDebugPrintAlwaysShort("Reg 0x8C = %02X\n", (int)SiiRegRead(TX_PAGE_CBUS | 0x008C));
        SiiOsDebugPrintAlwaysShort("Reg 0x8D = %02X\n", (int)SiiRegRead(TX_PAGE_CBUS | 0x008D));
        SiiOsDebugPrintAlwaysShort("Reg 0x8E = %02X\n", (int)SiiRegRead(TX_PAGE_CBUS | 0x008E));

        SiiOsDebugPrintAlwaysShort("Incoming buffer is.. LENGTH = %03d\n", (int)inBuffer[0] );
        for(i = 1 ; i < 17; i++)
        {
                SiiOsDebugPrintAlwaysShort("%02X ", (int) inBuffer[ i ] );
        }

        // Completely stall to avoid any I2C traffic
        while(true);
}
//
//
//

void MdtDataCompare()
{
        uint8_t	currentByteOut = 0;
        uint8_t	currentByteIn  = 0;
        uint8_t	thisLength = 16;
        uint8_t	readLevel;
        uint8_t	writeLevel;
        //uint8_t temp[17];

        uint8_t	status;
        uint16_t sendCount = 0;
        uint8_t i = 0;

        SiiRegWrite ((TX_PAGE_CBUS | 0x0088), 0x80);			// 0x88[7] = 1	to enable the MDT Transmitter
        SiiRegWrite ((TX_PAGE_CBUS | 0x0086), 0x80);			// 0x86[7] = 1	to enable the MDT Receiver
        SiiRegWrite ((TX_PAGE_CBUS | 0x008D), 0x03);			// 0x86[7] = 1	to enable the MDT Receiver

        currentByteOut = 0;

        while(true)
        {
                //HalTimerWait(100);
                // Send when you can
                if( (SiiRegRead(TX_PAGE_CBUS | 0x008C) & BIT1) != BIT1  )
                {
                        status = SiiRegRead(TX_PAGE_CBUS | 0x008B);
                        writeLevel = (status & 0xE0) >> 5;
                        if( writeLevel )
                        {
                                if(sendCount % 128 == 0)
                                {
                                        SiiOsDebugPrintAlwaysShort("\nMDT Iteration: %02X, X-Fifo Levels: %02X\n", (int)sendCount, (int)writeLevel);
                                }

                                memset(&outBuffer[0], 0xAA, 17);
                                outBuffer[0] = 0x0F;


                                SiiOsDebugPrintAlwaysShort("outBuffer:: ");
                                for(i = 1 ; i < 17; i++)
                                {
                                        outBuffer[ i ] = currentByteOut++;
                                        if( i ==1 )
                                        {
                                                SiiOsDebugPrintAlwaysShort("First Byte %02X\n", (int)outBuffer[i]);
                                        }
                                        //SiiOsDebugPrintAlwaysShort("%02X ", (int)outBuffer[i]);
                                }
                                //SiiOsDebugPrintAlwaysShort("\n");


                                SiiRegWriteBlock((TX_PAGE_CBUS | 0x0089), &outBuffer[0], 17);


                                /*
                                SiiRegReadBlock((TX_PAGE_CBUS | 0x0089), &temp[0], 17);

                                if( temp[0] != outBuffer[1] )
                                {
                                	ERROR_DEBUG_PRINT(("Data wasn't written correctly to X-FIFO, first byte written: %02X, first byte read: %02X\n\n", outBuffer[1], temp[0] ));
                                	MdtErrorHalt();
                                }
                                */

                                sendCount++;
                        }
                }
                //HalTimerWait(150);
                //
                // Receive when you can
                if( SiiRegRead(TX_PAGE_CBUS | 0x008C) & BIT0 )
                {
                        readLevel = status = SiiRegRead(TX_PAGE_CBUS | 0x008A);
                        readLevel = (readLevel & 0xE0) >> 5;
                        SiiRegWrite ( (TX_PAGE_CBUS | 0x008C), BIT0 );
                }

                while ( readLevel > 0 )
                {
                        //SiiOsDebugPrintAlwaysShort("R-Fifo Levels = %02X ", (int)readLevel);
                        //SiiOsDebugPrintAlwaysShort(": Reading bytes Expecting from %02X\n", (int)currentByteIn);

                        memset(&inBuffer[0], 0xAA, 17);
                        SiiRegReadBlock((TX_PAGE_CBUS | 0x0087), &inBuffer[0], 17);

                        //
                        // Check length
                        //
                        if( inBuffer[ 0 ] != 16)
                        {
                                ERROR_DEBUG_PRINT(("MDT_ERROR: LENGTH MISMATCH. Expected %02d, Got %02d\n", (int)16, (int)inBuffer[ 0 ]));
                                MdtErrorHalt();
                        }
                        //
                        // Check data
                        //
                        SiiOsDebugPrintAlwaysShort("inBuffer:: ");
                        for(i = 1 ; i < 17; i++)
                        {
                                if( i ==1 )
                                {
                                        SiiOsDebugPrintAlwaysShort("First Byte %02X\n ", (int)inBuffer[i]);
                                }
                                //SiiOsDebugPrintAlwaysShort("%02X ", (int)inBuffer[i]);
                                if( inBuffer[i] != currentByteIn)
                                {
                                        ERROR_DEBUG_PRINT(("MDT_ERROR: DATA MISMATCH. Expected %02X, Got %02X\n", (int)currentByteIn, (int)inBuffer[i]));
                                        MdtErrorHalt();
                                }
                                currentByteIn++;
                        }
                        SiiOsDebugPrintAlwaysShort("\n");

                        //Move pointer to the next 86[0] = 0x81
                        SiiRegModify((TX_PAGE_CBUS | 0x0086), 0x01, 0x01);	//		Move pointer to the next 86[0] = 0x81
                        readLevel--;
                }
        }
}
#endif	//)

///////////////////////////////////////////////////////////////////////////////
//
// SiiAnyPollingDebug
//
// Alert the driver that the peer's POW bit has changed so that it can take
// action if necessary.
//
#ifdef	DEBUG_RI_VALUES //(
static	uint8_t	counter = 0;
#endif //)
int		g_ri_display = false;

void	SiiAnyPollingDebug( void )
{

#ifdef	MDT_TESTER
        if( mdt_tester )
        {
                uint8_t	enableMdtCode;
                enableMdtCode = SiiRegRead(REG_CBUS_DEVICE_CAP_0);

                if(enableMdtCode)
                {
                        SiiRegWrite(REG_CBUS_DEVICE_CAP_0, 0x00);
                        MdtDataCompare();
                }
        }
#endif	//	MDT_TESTER


#ifdef	DEBUG_RI_VALUES

        if (!(mfdrvTranscodeMode & g_modeFlags))
        {
                uint8_t		remoteRiLow, remoteRiHigh;
                uint8_t		localRiLow, localRiHigh;
                if(g_ri_display)
                {
                        SiiRegWrite(REG_RI_LOCAL_OR_REMOTE, 0x40);
                        remoteRiLow  = SiiRegRead(TPI_HDCP_RI_LOW_REG);
                        remoteRiHigh = SiiRegRead(TPI_HDCP_RI_HIGH_REG);

                        SiiRegWrite(REG_RI_LOCAL_OR_REMOTE, 0x00);
                        localRiLow   = SiiRegRead(TPI_HDCP_RI_LOW_REG);
                        localRiHigh  = SiiRegRead(TPI_HDCP_RI_HIGH_REG);

                        if( 0 == (0x20 & (counter++)))
                        {
                                TXD_DEBUG_PRINT(("Remote Ri = %02X, %02X. Local  Ri = %02X, %02X\n",
                                                 (int) remoteRiLow,
                                                 (int) remoteRiHigh,
                                                 (int) localRiLow,
                                                 (int) localRiHigh));
                        }
                }
        }
#endif //	DEBUG_RI_VALUES
}

//////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxChipInitialize
//
// Chip specific initialization.
// This function is for SiI 8332/8336 Initialization: HW Reset, Interrupt enable.
//
//////////////////////////////////////////////////////////////////////////////

bool_t SiiMhlTxChipInitialize (void)
{
        mscCmdInProgress = false;	// false when it is okay to send a new command
        dsHpdStatus = 0;
#ifdef SWWA_INFOFRAME //(
        ckdtStatus=cdsFirstTime;
#endif //)
#ifdef INTERRUPT_DRIVEN_EDID //(
        edidFifoBlockNumber=0;

#ifdef REQUEST_EDID_ATOMICALLY //(

        currentEdidRequestBlock = 0;

#endif //)
#endif //)

        UpdatePowerState(POWER_STATE_D0_MHL);
        SI_OS_DISABLE_DEBUG_CHANNEL(SII_OSAL_DEBUG_SCHEDULER);

        /*	SiiOsDebugPrintAlwaysShort("\nFor MHL CTS HDCP Testing,\n"
                               "\talways use a source that \n"
                               "\tparses the EDID that we present upstream\n"
                               "\tand sends HDMI or DVI appropriately\n\n");
        */
#if TARGET_DEVICE_ID == WOLV60_DEVICE_ID //(

        g_chipRevId = SiiMhlTxDrvGetDeviceRev();
        g_chipDeviceId = SiiMhlTxDrvGetDeviceId();

        SiiMhlTxHwReset(TX_HW_RESET_PERIOD,TX_HW_RESET_DELAY);  // call up through the stack to accomplish reset.
        ERROR_DEBUG_PRINT(("Drv: SiiMhlTxChipInitialize: chip rev: %02X"
                           " chip id: %04X"
                           " in %s Mode\n"
                           ,(int)g_chipRevId
                           ,(uint16_t)SiiMhlTxDrvGetDeviceId()
                           ,(mfdrvTranscodeMode & g_modeFlags)?"Transcode":"Non-Transcode"
                          ));

        // setup device registers. Ensure RGND interrupt would happen.
        WriteInitialRegisterValuesPartOne();


#elif TARGET_DEVICE_ID == BISHOP_DEVICE_ID //)(
        SiiMhlTxHwReset(TX_HW_RESET_PERIOD,TX_HW_RESET_DELAY);  // call up through the stack to accomplish reset.
        WriteInitialRegisterValuesPartOne();

        g_chipRevId = SiiMhlTxDrvGetDeviceRev();
        g_chipDeviceId = SiiMhlTxDrvGetDeviceId();

        TXD_DEBUG_PRINT(("HdmiTxDrv: HDCP revision:0x%02x\n",(uint16_t)SiiRegRead(TPI_HDCP_REVISION_DATA_REG) ));
        ERROR_DEBUG_PRINT(("Drv: SiiMhlTxChipInitialize:"
                           " device: 0x%04x"
                           " rev: %02x"
                           " in %s Mode\n"
                           ,(uint16_t)g_chipDeviceId
                           ,(uint16_t)g_chipRevId
                           ,(mfdrvTranscodeMode & g_modeFlags)?"Transcode":"Non-Transcode"
                          ));

        // setup device registers. Ensure RGND interrupt would happen.
#endif//)
        WriteInitialRegisterValuesPartTwo(0);
#ifdef DO_HDCP
        // setup TX
        if (!(mfdrvTranscodeMode & g_modeFlags))
        {
                SiiHdmiTxLiteInitialize(DoHdcp?true:false);
        }
#endif

        SwitchToD3();

        return true;
}

void	HDCP_On (void);

void SiiMhlTxDrvGetAviInfoFrame(PAVIInfoFrame_t pAviInfoFrame)
{
        uint8_t	*avif = (uint8_t *)pAviInfoFrame;

        SiiRegModify(REG_RX_HDMI_CTRL2
                     ,BIT_RX_HDMI_CTRL2_VSI_MON_SEL_MASK
                     ,BIT_RX_HDMI_CTRL2_VSI_MON_SEL_AVI_INFOFRAME
                    );
        SiiRegReadBlock(REG_RX_HDMI_MON_PKT_HEADER1,(uint8_t *)pAviInfoFrame,sizeof(*pAviInfoFrame));
        ERROR_DEBUG_PRINT(("Below list is AVI info which 8240 get from AP:\n\tAVIF =%02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X\n" ,(int)avif[0] ,(int)avif[1] ,(int)avif[2]
                           ,(int)avif[3], (int)avif[4], (int)avif[5], (int)avif[6], (int)avif[7]
                           ,(int)avif[8], (int)avif[9], (int)avif[10], (int)avif[11]
                          ));
}

//-------------------------------------------------------------------------------------------------
//! @brief      Interrupt Service Routine for INTR 7
//!
//! @param[in]  None
//! @retval     None
//-------------------------------------------------------------------------------------------------

void Int7Isr(void)
{
        uint8_t statusIntr7;
        statusIntr7 = SiiRegRead(REG_INTR7);
        if (statusIntr7)
        {
                // 		TXD_DEBUG_PRINT(("-----------------> GOT statusIntr7 = %02X\n\n", (int)statusIntr7));
                // clear all
                SiiRegWrite(REG_INTR7,statusIntr7);
                if (BIT_INTR7_CEA_NO_AVI & statusIntr7)
                {
                        ERROR_DEBUG_PRINT(("NO_AVI\n"));
                        SiiMhlTxNoAvi();
                }
                if (BIT_INTR7_CEA_NO_VSI & statusIntr7)
                {
                        ERROR_DEBUG_PRINT(("NO_VSI\n"));
                        SiiMhlTxNoVsi();
                }
                if (BIT_INTR7_CP_NEW_CP & statusIntr7)
                {
                        INFO_DEBUG_PRINT(("NEW_CP\n"));
                }
                if (BIT_INTR7_CP_SET_MUTE & statusIntr7)
                {
                        TXD_DEBUG_PRINT(("-----------------> Got AV MUTE statusIntr7 = %02X\n\n", (int)statusIntr7));

                        if (!stateFlags.upStreamMuted)
                        {
                                stateFlags.upStreamMuted = 1;
                                SiiMhlTxDrvVideoMute( );
                        }
                }
                if (BIT_INTR7_CP_CLR_MUTE & statusIntr7)
                {
                        TXD_DEBUG_PRINT(("-----------------> Got AV UNMUTE statusIntr7 = %02X\n\n", (int)statusIntr7));

                        TXD_DEBUG_PRINT(("BIT_INTR7_CP_CLR_MUTE %s\n",stateFlags.upStreamMuted?"upstream mute":"already unmuted"));

                        // if it is time yet, this will restart HDCP
                        stateFlags.upStreamMuted = 0;
                        SiiMhlTxDrvVideoUnmute();
                }
        }
}



void SiiMhlTxDrvGetVendorSpecificInfoFrame(PVendorSpecificInfoFrame_t pVendorSpecificInfoFrame)
{
        SiiRegModify(REG_RX_HDMI_CTRL2
                     ,BIT_RX_HDMI_CTRL2_VSI_MON_SEL_MASK
                     ,BIT_RX_HDMI_CTRL2_VSI_MON_SEL_VS_INFOFRAME
                    );
        SiiRegReadBlock(REG_RX_HDMI_MON_PKT_HEADER1,(uint8_t *)pVendorSpecificInfoFrame,sizeof(*pVendorSpecificInfoFrame));
}

//-------------------------------------------------------------------------------------------------
//! @brief      Interrupt Service Routine for INTR 8
//!
//! @param[in]  None
//! @retval     None
//-------------------------------------------------------------------------------------------------
void Int8Isr(void)
{
        uint8_t statusIntr8;
        statusIntr8 = SiiRegRead(REG_INTR8);
        if (statusIntr8)
        {
                AVIInfoFrame_t aviInfoFrame;
                VendorSpecificInfoFrame_t vendorSpecificInfoFrame;
                // clear all interrupts
                SiiRegWrite(REG_INTR8,statusIntr8);

                INFO_DEBUG_PRINT(("Got INTR8 = %02X\n", (int)statusIntr8));
                // 		TXD_DEBUG_PRINT(("-----------------> GOT statusIntr8 = %02X\n\n", (int)statusIntr8));
                if (BIT_INTR8_CEA_NEW_VSI & statusIntr8)
                {
                        INFO_DEBUG_PRINT(("Drv: NEW_VSI\n"));
                        //#define BITFIELD_TEST
#ifdef BITFIELD_TEST //(
                        {
                                int i;
                                extern void *memset  (void *s, char val, int n);
                                memset(&vendorSpecificInfoFrame,0,sizeof(vendorSpecificInfoFrame));
                                for(i = 0; i < 8; ++i)
                                {
                                        vendorSpecificInfoFrame.pb4.HDMI_Video_Format=i;
                                        vendorSpecificInfoFrame.pb5.ThreeDStructure.threeDStructure=i;
                                        DumpIncomingInfoFrame(&vendorSpecificInfoFrame,sizeof(vendorSpecificInfoFrame));
                                }
                                INFO_DEBUG_PRINT(("avi.checksum offset:%x\n",SII_OFFSETOF(InfoFrame_t,body.avi.checksum) ));
                                INFO_DEBUG_PRINT(("avi.byte_4 offset:%x\n",SII_OFFSETOF(InfoFrame_t,body.avi.byte_4) ));
                                INFO_DEBUG_PRINT(("vendorSpecific.checksum offset:%x\n",SII_OFFSETOF(InfoFrame_t,body.vendorSpecific.checksum) ));
                                INFO_DEBUG_PRINT(("vendorSpecific.pb4 offset:%x\n",SII_OFFSETOF(InfoFrame_t,body.vendorSpecific.pb4) ));
                                INFO_DEBUG_PRINT(("vendorSpecific.pb5 offset:%x\n",SII_OFFSETOF(InfoFrame_t,body.vendorSpecific.pb5) ));
                                INFO_DEBUG_PRINT(("vendorSpecific.pb6 offset:%x\n",SII_OFFSETOF(InfoFrame_t,body.vendorSpecific.pb6) ));
                                INFO_DEBUG_PRINT(("vendorSpecific.pb7 offset:%x\n",SII_OFFSETOF(InfoFrame_t,body.vendorSpecific.pb7) ));
                        }
#endif //)
                        SiiMhlTxDrvGetVendorSpecificInfoFrame(&vendorSpecificInfoFrame);
                }

                if (BIT_INTR8_CEA_NEW_AVI & statusIntr8)
                {
                        ERROR_DEBUG_PRINT(("NEW_AVI \n"));
#ifdef SWWA_INFOFRAME //(
                        //          if (cdsTriggered==ckdtStatus)
                        if(cdsComplete != ckdtStatus)
                        {
                                uint8_t new_ckdt;

                                /*  Disable TMDS output power coz input is unstable */
                                SiiRegModify(REG_TMDS_CCTRL, BIT_TMDS_CCTRL_TMDS_OE, 0);
                                /* Restore old value of 281[5:0]*/
                                SiiRegWrite(REG_MHLTX_CTL2, old_r281);
                                /* Make sure CKDT did not vanish during previous I2C cycles. */
                                new_ckdt = SiiRegRead(REG_TMDS_CSTAT_P3);
                                new_ckdt = (BIT_TMDS_CSTAT_P3_PDO_MASK & new_ckdt);
                                if(BIT_TMDS_CSTAT_P3_PDO_CLOCK_DETECTED==new_ckdt)
                                {
                                        /* Mask out CKDT interrupt */
                                        SiiRegModify(REG_INTR5_MASK, BIT_INTR5_CKDT_CHANGE, 0x00);
                                        /* SCDT controls tx_en and MHL TX TMDS PLL */
                                        ERROR_DEBUG_PRINT(("SWWA_INFR part 2. [80] = %02X, [2A0] = %02X, [7C] = %02X\n",
                                                           (int)SiiRegRead(REG_TMDS_CCTRL),
                                                           (int)SiiRegRead(REG_TMDS_CSTAT_P3),
                                                           (int)SiiRegRead(REG_INTR8)
                                                          ));
                                }
                                ckdtStatus=cdsComplete;
                        }
#endif //)
                        SiiMhlTxDrvGetAviInfoFrame(&aviInfoFrame);
                }
                switch(statusIntr8 &(BIT_INTR8_CEA_NEW_VSI|BIT_INTR8_CEA_NEW_AVI))
                {
                case (BIT_INTR8_CEA_NEW_VSI|BIT_INTR8_CEA_NEW_AVI):
                        INFO_DEBUG_PRINT(("Drv: NEW_VSI and NEW_AVI arrivaled\n"));
                        SiiMhlTxProcessInfoFrameChange(&vendorSpecificInfoFrame,
                                                       &aviInfoFrame);
                        break;
                case BIT_INTR8_CEA_NEW_VSI:
                        INFO_DEBUG_PRINT(("Drv: NEW_VSI arrivaled\n"));
                        SiiMhlTxProcessInfoFrameChange(&vendorSpecificInfoFrame,NULL);
                        break;
                case BIT_INTR8_CEA_NEW_AVI:
                        INFO_DEBUG_PRINT(("Drv: NEW_AVI arrivaled\n"));
                        SiiMhlTxProcessInfoFrameChange(NULL,&aviInfoFrame);
                        break;
                }
        }
}

#ifdef HW_ASSISTED_EDID_READ //(

static void SiiMhlTxDrvResetDDCFifo( void )
{
        uint8_t ddcStatus;
        ddcStatus = SiiRegRead(REG_DDC_STATUS);
        // clear the fifo
        ClrTPIMode
        if (BIT_DDC_STATUS_ddc_no_ack & ddcStatus)
        {
                ERROR_DEBUG_PRINT(("clearing DDC ack status\n\n"));
                SiiRegWrite(REG_DDC_STATUS,ddcStatus & ~BIT_DDC_STATUS_ddc_no_ack);
        }
        SiiRegModify(REG_DDC_CMD,BIT_DDC_CMD_command_MASK,BIT_DDC_CMD_command_clear_fifo);
        SetTPIMode
}


bool_t SiiMhlTxDrvIssueEdidReadRequest(uint8_t blockNum)
{
        uint8_t cbusStatus;
        // check to see if downstream HPD still asserted
        cbusStatus = SiiRegRead(REG_CBUS_STATUS);
        if (BIT_CBUS_HPD & cbusStatus)
        {
                EDID_DEBUG_PRINT((
                                         " req EDID blk:%d"
                                         " cur EDID req blk:%d"
                                         " edidFifoBlockNumber:%d\n"
                                         ,(uint16_t)blockNum
                                         ,(uint16_t)currentEdidRequestBlock
                                         ,(uint16_t)edidFifoBlockNumber
                                 ));
                // Setup which block to read

                CachedRegModify(REG_EDID_CTRL
                                ,(BIT_EDID_CTRL_FIFO_SELECT_MASK | BIT_EDID_CTRL_EDID_MODE_EN_MASK)
                                ,(BIT_EDID_CTRL_FIFO_SELECT_EDID | BIT_EDID_CTRL_EDID_MODE_EN_ENABLE)
                               );
#ifndef INTERRUPT_DRIVEN_EDID //(
                // clear any residual EDID_DONE interrupt
                SiiRegModify (REG_INTR9, BIT_INTR9_EDID_DONE_MASK, BIT_INTR9_EDID_DONE);		// CLEAR 2E0[5]
#endif //)

                SiiMhlTxDrvResetDDCFifo( );
                // Setup which block to read
                if( 0 == blockNum)
                {
                        SiiRegWrite (REG_TPI_CBUS_START, BIT_TPI_CBUS_START_EDID_READ_BLOCK_0);		// 2E2[1] shared with RCP etc
                }
                else
                {
                        uint8_t param= (1 << (blockNum-1));
                        // Select EDID rather than devcap, kick off read

                        EDID_DEBUG_PRINT(("EDID HW Assist: Programming Reg %02X to %02X\n", (int)REG_EDID_HW_ASSIST_READ_BLOCK_ADDR, (int)param ));
                        SiiRegWrite (REG_EDID_HW_ASSIST_READ_BLOCK_ADDR,param);	// 2ED
                }
                return true;
        }
        else
        {
                ERROR_DEBUG_PRINT((
                                          "\n\tNo HPD for EDID block request:%d\n"
                                          "\tcurrentEdidRequestBlock:%d\n"
                                          "\tedidFifoBlockNumber:%d\n"
                                          ,(uint16_t)blockNum
                                          ,(uint16_t)currentEdidRequestBlock
                                          ,(uint16_t)edidFifoBlockNumber
                                  ));
                return false;
        }
}
#endif //)

int Int9Isr(void)
{
        uint8_t int9Status;
        int9Status = SiiRegRead(REG_INTR9);
        if (0xFF == int9Status)
        {
                INFO_DEBUG_PRINT(("I2C failure!!!\n"));
        }
        else if (int9Status)
        {
                TXD_DEBUG_PRINT(("Int9Isr: 0x%02x\n",(uint16_t)int9Status));
                SiiRegWrite(REG_INTR9,int9Status);
                if (BIT_INTR9_DEVCAP_DONE & int9Status)
                {
                        mscCmdInProgress = false;
                        TXD_DEBUG_PRINT(("Int9Isr:\n"));

                        // Notify the component layer
                        SiiMhlTxMscCommandDone(0);

                }
                if (BIT_INTR9_EDID_DONE & int9Status)
                {
#ifdef INTERRUPT_DRIVEN_EDID //(
                        int ddcStatus;
                        ddcStatus = SiiRegRead(REG_DDC_STATUS);
                        if (BIT_DDC_STATUS_ddc_no_ack & ddcStatus)
                        {
                                if (!SiiMhlTxDrvIssueEdidReadRequest(currentEdidRequestBlock=0))
                                {
                                        mscCmdInProgress = false;
                                        // Notify the component layer with error
                                        SiiMhlTxMscCommandDone(1);
                                }
                        }
                        else
                        {
#ifdef REQUEST_EDID_ATOMICALLY //(
                                int  numExtensions;
                                EDID_DEBUG_PRINT(("Int9Isr: EDID block read complete\n"));
                                numExtensions =SiiMhlTxGetNumCea861Extensions(currentEdidRequestBlock);

                                if (numExtensions < 0)
                                {
                                        mscCmdInProgress = false;
                                        ERROR_DEBUG_PRINT(("edid problem:%d\n\n",numExtensions));
                                        if (neNoHPD == numExtensions)
                                        {
                                                // no HPD, so start over
                                                SiiMhlTxMscCommandDone(1);
                                        }
                                        else
                                        {
                                                SiiMhlTxDrvResetDDCFifo( );
                                                if (!SiiMhlTxDrvIssueEdidReadRequest(currentEdidRequestBlock=0))
                                                {
                                                        mscCmdInProgress = false;
                                                        // Notify the component layer with error
                                                        SiiMhlTxMscCommandDone(1);
                                                }
                                        }
                                }
                                else if (currentEdidRequestBlock <  numExtensions)
                                {
                                        if (!SiiMhlTxDrvIssueEdidReadRequest(++currentEdidRequestBlock))
                                        {
                                                ERROR_DEBUG_PRINT(("could not issue EDID req\n"));
                                                mscCmdInProgress = false;
                                                // Notify the component layer with error
                                                SiiMhlTxMscCommandDone(1);
                                        }
                                }
                                else
                                {
                                        // done with last block
                                        mscCmdInProgress = false;
                                        // Notify the component layer
                                        SiiMhlTxMscCommandDone(0);
                                }
#else //)(
                                EDID_DEBUG_PRINT(("Int9Isr: EDID block read complete\n"));
                                mscCmdInProgress = false;
                                // Notify the component layer
                                SiiMhlTxMscCommandDone(0);
#endif //)
                        }
#else //)(
                        ERROR_DEBUG_PRINT(("Expected EDID_DONE in-line\n"));
#endif //)
                }
                if (BIT_INTR9_EDID_ERROR & int9Status)
                {

#ifdef INTERRUPT_DRIVEN_EDID //(
                        uint8_t cbusStatus;
                        cbusStatus = SiiRegRead(REG_CBUS_STATUS);

                        ERROR_DEBUG_PRINT(("EDID read error, %sHPD\n\n",(BIT_CBUS_HPD & cbusStatus)?"":"no "));
                        DumpTPIDebugRegs
                        edidFifoBlockNumber=0;
#ifdef REQUEST_EDID_ATOMICALLY //(
                        currentEdidRequestBlock=0;
#endif //)
                        SiiMhlTxDrvResetDDCFifo( );
                        if (BIT_CBUS_HPD & cbusStatus)
                        {
                                if (!SiiMhlTxDrvIssueEdidReadRequest(currentEdidRequestBlock=0))
                                {
                                        mscCmdInProgress = false;
                                        // Notify the component layer with error
                                        SiiMhlTxMscCommandDone(1);
                                }
                        }
                        else
                        {
                                SiiMhlTxMscCommandDone(1);
                        }
#else //)(
                        ERROR_DEBUG_PRINT(("Expected EDID_ERROR in-line\n"));
#endif //)
                }
        }
        return I2C_ACCESSIBLE;
}

void SiiMhlTxDrvGetPeerDevCapRegs(PMHLDevCap_u pDevCap)
{

        TXD_DEBUG_PRINT(("SiiMhlTxDrvGetPeerDevCapRegs:\n"));
        // choose devcap instead of EDID to appear at the FIFO
        CachedRegModify(REG_EDID_CTRL
                        ,BIT_EDID_CTRL_FIFO_SELECT_MASK
                        ,BIT_EDID_CTRL_FIFO_SELECT_DEVCAP
                       );
        // set the starting address
        SiiRegWrite (REG_EDID_FIFO_ADDR, 0);	// 2E9 = Starting address of FIFO

        TXD_DEBUG_PRINT(("SiiMhlTxDrvGetPeerDevCapRegs:\n"));

        // read the DEVCAP into the FIFO
        SiiRegReadBlock ( REG_EDID_FIFO_RD_DATA, pDevCap->aucDevCapCache, sizeof(*pDevCap));

}




///////////////////////////////////////////////////////////////////////////////
// SiiMhlTxDeviceIsr
//
// This function must be called from a master interrupt handler or any polling
// loop in the host software if during initialization call the parameter
// interruptDriven was set to true. SiiMhlTxGetEvents will not look at these
// events assuming firmware is operating in interrupt driven mode. MhlTx component
// performs a check of all its internal status registers to see if a hardware event
// such as connection or disconnection has happened or an RCP message has been
// received from the connected device. Due to the interruptDriven being true,
// MhlTx code will ensure concurrency by asking the host software and hardware to
// disable interrupts and restore when completed. Device interrupts are cleared by
// the MhlTx component before returning back to the caller. Any handling of
// programmable interrupt controller logic if present in the host will have to
// be done by the caller after this function returns back.

// This function has no parameters and returns nothing.
//
// This is the master interrupt handler for 9244. It calls sub handlers
// of interest. Still couple of status would be required to be picked up
// in the monitoring routine (Sii9244TimerIsr)
//
// To react in least amount of time hook up this ISR to processor's
// interrupt mechanism.
//
// Just in case environment does not provide this, set a flag so we
// call this from our monitor (Sii9244TimerIsr) in periodic fashion.
//
// Device Interrupts we would look at
//		RGND		= to wake up from D3
//		MHL_EST 	= connection establishment
//		CBUS_LOCKOUT= Service USB switch
//		CBUS 		= responder to peer messages
//					  Especially for DCAP etc time based events
//
void SiiMhlTxDeviceIsr (void)
{

        if( POWER_STATE_D0_MHL != fwPowerState )
        {
                //
                // Check important RGND, MHL_EST, CBUS_LOCKOUT and SCDT interrupts
                // During D3 we only get RGND but same ISR can work for both states
                //
                if (I2C_INACCESSIBLE == Int4Isr())
                {
                        return; // don't do any more I2C traffic until the next interrupt.
                }
        }
        else if( POWER_STATE_D0_MHL == fwPowerState )
        {
                TXD_DEBUG_PRINT(("SiiRegRead(REG_INTR1_MASK) = %02X\n\n", (int)SiiRegRead(REG_INTR1_MASK)));
                TXD_DEBUG_PRINT(("SiiRegRead(REG_INTR5_MASK) = %02X\n\n", (int)SiiRegRead(REG_INTR5_MASK)));
                if (I2C_INACCESSIBLE == Int4Isr())
                {
                        return; // don't do any more I2C traffic until the next interrupt.
                }
                // it is no longer necessary to check if the call to Int4Isr()
                //  put us into D3 mode, since we now return immediately in that case

                // Check for any peer messages for DCAP_CHG etc
                // Dispatch to have the CBUS module working only once connected.

                //  Check CBUS first for performance and downstream HPD reasons

                MhlCbusIsr();


                if (mfdrvTranscodeMode & g_modeFlags)
                {
                        Int5Isr();
                }
                else
                {
                        if (upStreamHPD)
                        {
                                Int5Isr();

                                Int7Isr();
                                Int8Isr();
#ifdef	DO_HDCP
                                HdcpIsr();
#endif
                        }
                }

                Int1Isr();
                if ( I2C_INACCESSIBLE == Int9Isr())
                {
                        // don't do any more processing until next RGND interrupt
                        return;
                }
        }
        if( POWER_STATE_D3 != fwPowerState )
        {
                // Call back into the MHL component to give it a chance to
                // take care of any message processing caused by this interrupt.
                MhlTxProcessEvents();
        }

}



//-------------------------------------------------------------------------------------------------
//! @brief      DriverAPI function to mute and unmute video
//!
//! @param[in]  mute - non zero will mute the video else unmute.
//! @retval     None
//-------------------------------------------------------------------------------------------------
#ifdef	DEBUG_RI_VALUES
extern	int	g_ri_display;
#endif	// DEBUG_RI_VALUES

void	SiiMhlTxDrvVideoMute( void )
{
#ifdef	DEBUG_RI_VALUES
        g_ri_display = false;
#endif	// DEBUG_RI_VALUES

        // Print always
        HDCP_DEBUG_PRINT(("SiiDrvVideoMute: AV muted\n"));
        ModifyTpiSystemControlDataReg(AV_MUTE_MASK,AV_MUTE_MUTED)
}

void	SiiMhlTxDrvVideoUnmute( void )
{
#ifdef DO_HDCP
        if (AUTH_CURRENT==SiiMhlTxLiteDrvGetAuthenticationState())
#endif
        {
#ifdef	DEBUG_RI_VALUES
                g_ri_display = true;
#endif	// DEBUG_RI_VALUES

                // Change register
                ModifyTpiSystemControlDataReg(AV_MUTE_MASK,AV_MUTE_NORMAL)
//    	HDCP_DEBUG_PRINT(("AV unmuted.\n"));
                ERROR_DEBUG_PRINT(("AV unmuted\n\n"));

        }
}

#ifdef DO_HDCP
void	SiiToggleTmdsForHdcpAuthentication(void)
{
        ERROR_DEBUG_PRINT(("start hdcp\n"));
        // a three step process is mandatory
        ModifyTpiSystemControlDataReg(AV_MUTE_MASK,AV_MUTE_MUTED)
        ModifyTpiSystemControlDataReg(TMDS_OUTPUT_CONTROL_MASK,TMDS_OUTPUT_CONTROL_POWER_DOWN)
        ModifyTpiSystemControlDataReg(TMDS_OUTPUT_CONTROL_MASK,TMDS_OUTPUT_CONTROL_ACTIVE)
        SiiMhlTxLiteDrvSetAuthenticationState(AUTH_PENDING);
}
#endif
void	SiiMhlTxDrvSetOutputType( void )
{

        // select output mode (HDMI/DVI) according to what chip register says
        // isHdmi =  (CONNECTOR_TYPE_HDMI == SiiRegRead(TPI_HDCP_QUERY_DATA_REG) & CONNECTOR_TYPE_MASK;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvTmdsControl
//
// Control the TMDS output. MhlTx uses this to support RAP content on and off.
//
void SiiMhlTxDrvTmdsControl (bool_t enable)
{
        ERROR_DEBUG_PRINT(("%sable TMDS\n", enable?"en":"dis"));
        if( enable )
        {
                SiiRegModify(REG_MHLTX_CTL2,BIT_MHL_TX_CTL2_TX_OE_MASK ,0x3C); //2012-04-17-1600 PDT
                SiiRegModify(REG_MHLTX_CTL8,BIT_MHLTX_CTL8_PLL_BW_CTL_MASK,0x3); //2012-04-17-1600 PDT

                if ( mfdrvTranscodeMode & g_modeFlags )
                {
                        SET_BIT(REG_TMDS_CCTRL, 4);
                        TXD_DEBUG_PRINT(("Drv: TMDS Output Enabled\n"));
                        SiiMhlTxDrvReleaseUpstreamHPDControl();  // this triggers an EDID read
                }
                else
                {
                        //
                        // Now control the AV Mute and toggle TMDS_OE to kick start HDCP authentication
                        //
#ifndef DO_HDCP
                        //if (DoHdcp == false)
                        {
                                TXD_DEBUG_PRINT(("SiiMhlTxDrvTmdsControl: TMDS Output Enabled (Video UnMuted, DS does not support HDCP)\n"));

                                // must set UNMUTE and output enable in same write to avoid starting HDCP
                                ModifyTpiSystemControlDataReg(
                                        (TMDS_OUTPUT_CONTROL_MASK   |  AV_MUTE_MASK)
                                        , (TMDS_OUTPUT_CONTROL_ACTIVE | AV_MUTE_NORMAL)
                                )
#ifdef DO_HDCP
                                SiiMhlTxLiteDrvSetAuthenticationState(AUTH_CURRENT);
#endif
                                SiiMhlTxNotifyAuthentication();
                        }
#else	// Downstream supports HDCP. Toggle TMDS to kick off HDCP Authentication
                        {
                                HDCP_DEBUG_PRINT(("SiiMhlTxDrvTmdsControl: Examining TMDS_OE:%02x\n",(uint16_t) cache_TPI_SYSTEM_CONTROL_DATA_REG));
                                if (TMDS_OUTPUT_CONTROL_POWER_DOWN == (TMDS_OUTPUT_CONTROL_MASK & cache_TPI_SYSTEM_CONTROL_DATA_REG))
                                {
                                        TXD_DEBUG_PRINT(("SiiMhlTxDrvTmdsControl: TMDS_OE indicated power down\n"));

                                        SiiToggleTmdsForHdcpAuthentication();
                                }
                                else
                                {
                                        uint8_t currentStatus,linkStatus;
                                        currentStatus = SiiRegRead(TPI_HDCP_QUERY_DATA_REG);
                                        linkStatus = LINK_STATUS_MASK & currentStatus;
                                        TXD_DEBUG_PRINT(("SiiMhlTxDrvTmdsControl: Examining TPI_HDCP_QUERY_DATA_REG: 0x%02x\n",(uint16_t)currentStatus));

                                        switch(linkStatus)
                                        {
                                        case LINK_STATUS_NORMAL:
                                                if ( AV_MUTE_MUTED & cache_TPI_SYSTEM_CONTROL_DATA_REG)
                                                {
                                                        SiiMhlTxDrvVideoUnmute();
                                                }
                                                break;
                                        case LINK_STATUS_LINK_LOST:
                                                break;
                                        case LINK_STATUS_RENEGOTIATION_REQ:
                                                break;
                                        case LINK_STATUS_LINK_SUSPENDED:
#ifdef DO_HDCP
                                                SiiToggleTmdsForHdcpAuthentication();
#endif
                                                break;
                                        }
                                }
                        }
#endif
                        if(sink_type_HDMI == SiiMhlTxGetSinkType())
                        {
                                // Change packet filters to drop AIF and GCP.
                                SiiRegWrite(REG_PKT_FILTER_0, BIT_PKT_FILTER_0_DROP_GCP_PKT
                                            | BIT_PKT_FILTER_0_DROP_AVI_PKT
                                            | BIT_PKT_FILTER_0_DROP_MPEG_PKT
                                            | BIT_PKT_FILTER_0_DROP_CEA_GAMUT_PKT);

                                // make sure that bit 7 is cleared to convert incoming VSIF from HDMI format to MHL format
                                SiiRegWrite(REG_PKT_FILTER_1, 0x02);
                        }
                }
        }
        else
        {

                SiiRegModify(REG_MHLTX_CTL2,BIT_MHL_TX_CTL2_TX_OE_MASK ,0x0); //2012-04-17-1600 PDT
                SiiRegModify(REG_MHLTX_CTL8,BIT_MHLTX_CTL8_PLL_BW_CTL_MASK,0x3); //2012-04-17-1600 PDT

                if ( mfdrvTranscodeMode & g_modeFlags )
                {
                        CLR_BIT(REG_TMDS_CCTRL, 4);
                        TXD_DEBUG_PRINT(("Drv: TMDS Ouput Disabled\n"));
                }
                else
                {
#ifdef DO_HDCP
                        if (DoHdcp)
                        {
                                // Disable HDCP
                                SiiHdmiTxLiteDisableEncryption();
                        }
#endif
                        TXD_DEBUG_PRINT(("SiiMhlTxDrvTmdsControl: Turn off TMDS OE, AV mute and Disable HDCP\n"));
                        ModifyTpiSystemControlDataReg(
                                (TMDS_OUTPUT_CONTROL_MASK          | AV_MUTE_MASK)
                                , (TMDS_OUTPUT_CONTROL_POWER_DOWN    | AV_MUTE_MUTED)
                        )
#ifdef DO_HDCP
                        SiiMhlTxLiteDrvSetAuthenticationState(AUTH_IDLE);
#endif
                }
        }
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvNotifyEdidChange
//
// MhlTx may need to inform upstream device of an EDID change. This can be
// achieved by toggling the HDMI HPD signal or by simply calling EDID read
// function.
//
void SiiMhlTxDrvNotifyEdidChange (void)
{
        TXD_DEBUG_PRINT(("Drv: SiiMhlTxDrvNotifyEdidChange\n"));
        if ( mfdrvTranscodeMode & g_modeFlags )
        {
                //
                // Prepare to toggle HPD to upstream
                //
                SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow();

                // wait a bit
                HalTimerWait(110);

                // drive HPD back to high by reg_hpd_out_ovr_val = HIGH
                CachedRegModify(REG_HPD_CTRL
                                , BIT_HPD_CTRL_HPD_OUT_OVR_VAL_MASK
                                , BIT_HPD_CTRL_HPD_OUT_OVR_VAL_ON
                               );

                // release control to allow transcoder to modulate for CLR_HPD and SET_HPD
                SiiMhlTxDrvReleaseUpstreamHPDControl();
        }
        else
        {
                // component layer will re-read EDID
                SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow();

                //SiiMhlTxDrvTmdsControl(false);
#ifdef DO_HDCP
                SiiHdmiTxLiteDisableEncryption();
#endif
                // prevent HDCP interrupts from coming in
                ClrTPIMode  // disable TPI Mode
                SetTPIMode // Enable TPI Mode
        }
}
//------------------------------------------------------------------------------
// Function:    SiiMhlTxDrvSendCbusCommand
//
// Write the specified Sideband Channel command to the CBUS.
// Command can be a MSC_MSG command (RCP/RAP/RCPK/RCPE/RAPK), or another command
// such as READ_DEVCAP, SET_INT, WRITE_STAT, etc.
//
// Parameters:
//              pReq    - Pointer to a cbus_req_t structure containing the
//                        command to write
// Returns:     true    - successful write
//              false   - write failed
//------------------------------------------------------------------------------

bool_t SiiMhlTxDrvSendCbusCommand (cbus_req_t *pReq)
{
        bool_t  success = true;


        //
        // If not connected, return with error
        //
        if( (POWER_STATE_D0_MHL != fwPowerState ) || (mscCmdInProgress))
        {
                ERROR_DEBUG_PRINT(("Error: Drv: fwPowerState: %02X, or CBUS(0x0A):%02X mscCmdInProgress = %d\n",
                                   (int) fwPowerState,
                                   (int) SiiRegRead(REG_CBUS_STATUS),
                                   (int) mscCmdInProgress));

                return false;
        }
        // Now we are getting busy
        mscCmdInProgress	= true;


        switch ( pReq->command )
        {
        case MHL_SET_INT:
                CBUS_DEBUG_PRINT(("Drv: Sending SET_INT command %02X, %02X, %02X\n"
                                  , (int)pReq->command
                                  , (int)(pReq->offsetData)
                                  , (int)pReq->payload_u.msgData[0]
                                 ));
                SiiRegWrite(REG_CBUS_MSC_CMD_OR_OFFSET, pReq->offsetData); 	// set offset
                SiiRegWrite(REG_CBUS_MSC_1ST_TRANSMIT_DATA, pReq->payload_u.msgData[0]);
                SiiRegWrite(REG_CBUS_MSC_COMMAND_START, BIT_CBUS_MSC_WRITE_STAT_OR_SET_INT);
                break;

        case MHL_WRITE_STAT:
                CBUS_DEBUG_PRINT(("Drv: Sending WRITE_STATE command %02X, %02X, %02X\n"
                                  , (int)pReq->command
                                  , (int)(pReq->offsetData)
                                  , (int)pReq->payload_u.msgData[0]
                                 ));
                SiiRegWrite(REG_CBUS_MSC_CMD_OR_OFFSET, pReq->offsetData); 	// set offset
                SiiRegWrite(REG_CBUS_MSC_1ST_TRANSMIT_DATA, pReq->payload_u.msgData[0]);
                SiiRegWrite(REG_CBUS_MSC_COMMAND_START,  BIT_CBUS_MSC_WRITE_STAT_OR_SET_INT);
                break;

        case MHL_READ_DEVCAP:
                CBUS_DEBUG_PRINT(("Reading DEVCAP\n"));
                // read the entire DEVCAP array in one command (new in 8240)
                SiiMhlTxDrvResetDDCFifo( );
                SiiRegWrite(REG_TPI_CBUS_START,BIT_TPI_CBUS_START_DEVCAP_READ_START);
                break;
        case MHL_READ_EDID_BLOCK:
#ifdef INTERRUPT_DRIVEN_EDID //(
#ifdef REQUEST_EDID_ATOMICALLY //(
                currentEdidRequestBlock = 0;
#else //)(
                currentEdidRequestBlock = pReq->offsetData;
#endif //)
                success = SiiMhlTxDrvIssueEdidReadRequest(currentEdidRequestBlock);
                if (!success)
                {
                        mscCmdInProgress = false;
                }
                break;
#endif //)

        case MHL_GET_STATE:			// 0x62 -
        case MHL_GET_VENDOR_ID:		// 0x63 - for vendor id
        case MHL_SET_HPD:			// 0x64	- Set Hot Plug Detect in follower
        case MHL_CLR_HPD:			// 0x65	- Clear Hot Plug Detect in follower
        case MHL_GET_SC1_ERRORCODE:		// 0x69	- Get channel 1 command error code
        case MHL_GET_DDC_ERRORCODE:		// 0x6A	- Get DDC channel command error code.
        case MHL_GET_MSC_ERRORCODE:		// 0x6B	- Get MSC command error code.
        case MHL_GET_SC3_ERRORCODE:		// 0x6D	- Get channel 3 command error code.
                CBUS_DEBUG_PRINT(("Drv: Sending misc. command %02X, %02X\n"
                                  , (int)pReq->command
                                  , (int)pReq->payload_u.msgData[0]
                                 ));
                SiiRegWrite(REG_CBUS_MSC_CMD_OR_OFFSET, pReq->command );
                SiiRegWrite(REG_CBUS_MSC_1ST_TRANSMIT_DATA, pReq->payload_u.msgData[0]);
                SiiRegWrite(REG_CBUS_MSC_COMMAND_START, BIT_CBUS_MSC_PEER_CMD);
                break;

        case MHL_MSC_MSG:
                CBUS_DEBUG_PRINT(("Drv: Sending MSC_MSG command %02X, %02X, %02X\n"
                                  , (int)pReq->command
                                  , (int)pReq->payload_u.msgData[0]
                                  , (int)pReq->payload_u.msgData[1]
                                 ));
                SiiRegWrite(REG_CBUS_MSC_CMD_OR_OFFSET, pReq->command );
                SiiRegWrite(REG_CBUS_MSC_1ST_TRANSMIT_DATA, pReq->payload_u.msgData[0]);
                SiiRegWrite(REG_CBUS_MSC_2ND_TRANSMIT_DATA, pReq->payload_u.msgData[1]);
                SiiRegWrite(REG_CBUS_MSC_COMMAND_START, BIT_CBUS_MSC_MSG);
                break;

        case MHL_WRITE_BURST:

                // Now copy all bytes from array to local scratchpad
                if (NULL == pReq->payload_u.pdatabytes)
                {
                        TXD_DEBUG_PRINT(("\nDrv: Put pointer to WRITE_BURST data in req.pdatabytes!!!\n\n"));
                        success = false;
                }
                else
                {
                        uint8_t *pData = pReq->payload_u.pdatabytes;
                        CBUS_DEBUG_PRINT(("Drv: Sending WRITE_BURST command %02X, %02X, %02\n"
                                          , (int)pReq->offsetData
                                          , (int)pReq->payload_u.msgData[0]
                                          , (int)pReq->length -1
                                         ));
                        SiiRegWrite(REG_CBUS_MSC_CMD_OR_OFFSET, pReq->offsetData); 	// set offset
                        SiiRegWrite(REG_CBUS_MSC_1ST_TRANSMIT_DATA, pReq->payload_u.msgData[0]);
                        SiiRegWrite(REG_CBUS_MSC_WRITE_BURST_DATA_LEN, pReq->length -1 );
                        TXD_DEBUG_PRINT(("\nDrv: Writing data into scratchpad\n\n"));
                        SiiRegWriteBlock(REG_CBUS_WB_XMIT_DATA_0,  pData,pReq->length);
                        SiiRegWrite(REG_CBUS_MSC_COMMAND_START, BIT_CBUS_MSC_WRITE_BURST);
                }
                break;

        default:
                ERROR_DEBUG_PRINT(("Error: Drv: fwPowerState: %02X, or CBUS(0x0A):%02X mscCmdInProgress = %d\n",
                                   (int) fwPowerState,
                                   (int) SiiRegRead(REG_CBUS_STATUS),
                                   (int) mscCmdInProgress));
                success = false;
                break;
        }
        return( success );
}

bool_t SiiMhlTxDrvCBusBusy (void)
{
        return mscCmdInProgress ? true : false;
}

///////////////////////////////////////////////////////////////////////////
// WriteInitialRegisterValuesPartOne
//
//
///////////////////////////////////////////////////////////////////////////

static void WriteInitialRegisterValuesPartOne (void)
{
        TXD_DEBUG_PRINT(("Drv: WriteInitialRegisterValuesPartOne\n"));
        // Power Up
#if (TARGET_DEVICE_ID == BISHOP_DEVICE_ID) //)(
        {
                SiiRegWrite(REG_DPD, 0x7D);
        }
#elif (TARGET_DEVICE_ID == WOLV60_DEVICE_ID) //)(
        {

                SiiRegWrite(REG_DPD, 0x75);			// Power up CVCC 1.2V core
        }
#endif //)
        // make sure that incoming FIFOs do not overflow prior to enabling TMDS output
        SiiRegWrite(REG_PKT_FILTER_0, 0xFF);
        SiiRegWrite(REG_PKT_FILTER_1, 0xFF);
}
///////////////////////////////////////////////////////////////////////////
// WriteInitialRegisterValuesPartTwo
//
//
///////////////////////////////////////////////////////////////////////////

static void WriteInitialRegisterValuesPartTwo (uint8_t enableDiscovery)
{
        TXD_DEBUG_PRINT(("Drv: WriteInitialRegisterValuesPartTwo\n"));
        InitCBusRegs();

        SiiMhlTxDrvEnableInterruptMasks();

        if (mfdrvTranscodeMode & g_modeFlags)
        {
                InitTranscodeMode(enableDiscovery);
                // Don't force HPD to 0 during wake-up from D3
                if (fwPowerState != TX_POWER_STATE_D3)
                {
                        //SiiRegModify(REG_DPD,BIT_DPD_PDIDCK_MASK,BIT_DPD_PDIDCK_POWER_DOWN);
                        SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow(); // Force HPD to 0 when not in MHL mode.
                }

                if (g_chipDeviceId == BISHOP_DEVICE_ID)
                {
                        SiiRegWrite(REG_DCTL, BIT_DCTL_EXT_DDC_SEL | BIT_DCTL_TRANSCODE_ON  | BIT_DCTL_TLCK_PHASE_INVERTED );
                }
                // 8240 has correct default falue for REG_DCTL
                // wake pulses necessary in transcode mode
                // No OTG, Discovery pulse proceed, Wake pulse not bypassed
                SiiRegWrite(REG_DISC_CTRL9
                            , BIT_DC9_CBUS_LOW_TO_DISCONNECT
                            | BIT_DC9_WAKE_DRVFLT
                            | BIT_DC9_DISC_PULSE_PROCEED
                            //bugzilla 24170                | BIT_DC9_VBUS_OUTPUT_CAPABILITY_SRC
                           );

        }
        else
        {
                SiiRegWrite(REG_TMDS_CLK_EN, 0x01);			// Enable TxPLL Clock
                SiiRegWrite(REG_TMDS_CH_EN, 0x11);			// Enable Tx Clock Path & Equalizer

                cache_REG_TPI_SEL =SiiRegRead(REG_TPI_SEL); // initialize reg cache
                SetTPIMode // Enable TPI Mode
#ifdef DO_HDCP
                SiiDrvHdmiTxLiteHdcpInitialize();
#endif
                // ksv exchange will not occur on tmds_oe ->0, unless av mute is already on
                cache_TPI_SYSTEM_CONTROL_DATA_REG = 0x10;  //default value;
                cache_REG_EDID_CTRL = BIT_EDID_CTRL_EDID_FIFO_ADDR_AUTO_ENABLE; // default value
                cache_REG_HPD_CTRL = SiiRegRead(REG_HPD_CTRL);
                ModifyTpiSystemControlDataReg(AV_MUTE_MASK,  AV_MUTE_MUTED)

                SiiRegWrite(REG_TPI_HW_OPT3,0x76);
                SiiRegWrite(REG_TPI_INTR_ST0,0xFF); // clear stale HDCP interrupt status

                SiiRegWrite(REG_RX_HDMI_CTRL3,0x00);

                SiiRegWrite(REG_MHLTX_CTL1, BIT_MHLTX_CTL1_TX_TERM_MODE_100DIFF | BIT_MHLTX_CTL1_DISC_OVRIDE_ON); // TX Source termination ON

                // MHLTX_CTL6 was split into two registers
                //default value 0xA0	SiiRegWrite(REG_MHLTX_CTL6, 0xBC); // Enable 1X MHL clock output
                //default value 0x1E	SiiRegWrite(REG_MHLTX_CTRL_AON,0x1E);

                //default value 0x3C	SiiRegWrite(REG_MHLTX_CTL2, 0x3C); // TX Differential Driver Config
                SiiRegModify(REG_MHLTX_CTL3
                             , BIT_MHLTX_CTL3_DAMPING_SEL_MASK
                             , BIT_MHLTX_CTL3_DAMPING_SEL_150_OHM
                            );
                SiiRegModify(REG_MHLTX_CTL4
                             ,BIT_CLK_SWING_CTL_MASK | BIT_DATA_SWING_CTL_MASK
                             ,0x33
                            ); //2012-05-10-bugzilla 24790

                SiiRegWrite(REG_CBUS_LINK_XMIT_BIT_TIME,0x1C); //2012-05-25 bugzilla 24790

                //default value 0x03	SiiRegWrite(REG_MHLTX_CTL7, 0x03); // 2011-10-10
                //2011-12-08 bit 3 soon to be defaulted on, but not yet
                SiiRegWrite(REG_MHLTX_CTL8, 0x0A); // PLL bias current, PLL BW Control

                // Analog PLL Control
                //default value 0x08    SiiRegWrite(REG_TMDS_CCTRL, 0x08);			// Enable Rx PLL clock 2011-10-10 - select BGR circuit for voltage references

                //default value 0x8C	SiiRegWrite(REG_USB_CHARGE_PUMP, 0x8C);		// 2011-10-10 USB charge pump clock

#ifdef SWWA_INFOFRAME //(
                // SCDT controls MHL TX TMDS PLL
                SiiRegWrite(REG_TMDS_CTRL4, 0x02);
#else //)(
                // SCDT controls tx_en and MHL TX TMDS PLL
                SiiRegWrite(REG_TMDS_CTRL4, 0x03);
#endif //)


                SiiRegWrite(REG_TMDS0_CCTRL2, 0x00);
                //default value 0x07	SiiRegModify(REG_DVI_CTRL3, BIT5, 0);      // 2011-10-10
                //dafault value 0x60	SiiRegWrite(REG_TMDS_TERMCTRL1, 0x60);

                //default value 0x03	SiiRegWrite(REG_PLL_CALREFSEL, 0x03);			// PLL Calrefsel
                //default value 0x20	SiiRegWrite(REG_PLL_VCOCAL, 0x20);			// VCO Cal
                //default value 0xE0	SiiRegWrite(REG_EQ_DATA0, 0xE0);			// Auto EQ
                //default value 0xC0	SiiRegWrite(REG_EQ_DATA1, 0xC0);			// Auto EQ
                //default value 0xA0	SiiRegWrite(REG_EQ_DATA2, 0xA0);			// Auto EQ
                //default value 0x80	SiiRegWrite(REG_EQ_DATA3, 0x80);			// Auto EQ
                //default value 0x60	SiiRegWrite(REG_EQ_DATA4, 0x60);			// Auto EQ
                //default value 0x40	SiiRegWrite(REG_EQ_DATA5, 0x40);			// Auto EQ
                //default value 0x20	SiiRegWrite(REG_EQ_DATA6, 0x20);			// Auto EQ
                //default value 0x00	SiiRegWrite(REG_EQ_DATA7, 0x00);			// Auto EQ

                //default value 0x0A	SiiRegWrite(REG_BW_I2C, 0x0A);			// Rx PLL BW ~ 4MHz
                //default value 0x06	SiiRegWrite(REG_EQ_PLL_CTRL1, 0x06);			// Rx PLL BW value from I2C

                //default value 0x06	SiiRegWrite(REG_MON_USE_COMP_EN, 0x06);

                // synchronous s/w reset
                SiiRegWrite(REG_ZONE_CTRL_SW_RST, 0x60);			// Manual zone control
                SiiRegWrite(REG_ZONE_CTRL_SW_RST, 0xD0);			// Manual zone control 2012-05-30 0xE0->0xD0 as initial value.

                //default value	SiiRegWrite(REG_MODE_CONTROL, 0x00);			// PLL Mode Value

                //no longer needed SiiRegWrite(REG_SYS_CTRL1, 0x35);			// bring out from power down (script moved this here from above)


                //default value 0xAD
                SiiRegWrite(REG_DISC_CTRL2, 0xA5);
                //experiment default value 0x57    SiiRegWrite(REG_DISC_CTRL5, 0x0x66);				// 1.8V CBUS VTH 5K pullup for MHL state

                if (g_chipDeviceId == BISHOP_DEVICE_ID)
                {
                        //		SiiRegWrite(REG_DISC_CTRL5, 0x02);				// 1.8V CBUS VTH 5K pullup for MHL state
                        SiiRegWrite(REG_DISC_CTRL6, 0x01);				// RGND & single discovery attempt (RGND blocking)
                        SiiRegWrite(REG_DISC_CTRL8, 0x01);				// Ignore VBUS
                }
                else if (g_chipDeviceId == WOLV60_DEVICE_ID)
                {
                        SiiRegWrite(REG_DISC_CTRL6, 0x11);				// RGND & single discovery attempt (RGND blocking)
                        SiiRegWrite(REG_DISC_CTRL8, 0x03);				// Ignore VBUS, wait for usbint_clr
                }

                // No OTG, Discovery pulse proceed, Wake pulse not bypassed
                SiiRegWrite(REG_DISC_CTRL9
                            , BIT_DC9_CBUS_LOW_TO_DISCONNECT
                            | BIT_DC9_WAKE_DRVFLT
                            | BIT_DC9_DISC_PULSE_PROCEED
                            //bugzilla 24170                | BIT_DC9_VBUS_OUTPUT_CAPABILITY_SRC
                           );

                // leave bit 3 reg_usb_en at its default value of 1
                //default value 0x8C    SiiRegWrite(REG_DISC_CTRL4, 0x8C);				// Pull-up resistance off for IDLE state and 10K for discovery state.
                if (g_chipDeviceId == WOLV60_DEVICE_ID)
                {
                        SiiRegWrite(REG_DISC_CTRL1, enableDiscovery?0x27:0x26);				// wait until just before D3 to enable discovery
                        SiiRegWrite(REG_DISC_CTRL3, 0x86);				// MHL CBUS discovery
                }
                else if (g_chipDeviceId == BISHOP_DEVICE_ID)
                {
                        SiiRegWrite(REG_DISC_CTRL1, enableDiscovery?0x25:0x24);				// wait until just before D3 to enable discovery
                }
                //default value 0x20 SiiRegWrite(REG_DISC_CTRL7, 0x20);				// use 1K only setting


#if 1//def ASSERT_PUSH_PULL
                CachedRegModify(REG_HPD_CTRL,BIT6,0);	// Assert Push/Pull
#endif

                // do this for non-trancode mode
                SiiRegModify(REG_DPD,BIT_DPD_PDIDCK_MASK,BIT_DPD_PDIDCK_POWER_DOWN);
                SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow(); // Force HPD to 0 when not in MHL mode.
#if 0 //(
                SiiRegModify(REG_HDMI_CLR_BUFFER
                             ,BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_W_AVI_EN_MASK  | BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_EN_MASK
                             ,BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_W_AVI_EN_CLEAR | BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_EN_CLEAR
                            );
#endif //)

#if 1 //(
                SiiRegModify(REG_HDMI_CLR_BUFFER
                             ,BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_EN_MASK
                             ,BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_EN_CLEAR
                            );
#endif //)

                //TODO re-investigate
                SiiRegWrite(REG_SRST, 0x84); 					// Enable Auto soft reset on SCDT = 0

                if (g_chipDeviceId == BISHOP_DEVICE_ID)
                {
                        SiiRegWrite(REG_PWD_SRST, 0x80); 	 // MHL Auto FIFO Reset
                }

                SiiRegWrite(REG_DCTL, BIT_DCTL_TRANSCODE_OFF | BIT_DCTL_TLCK_PHASE_INVERTED );
#ifdef SWWA_INFOFRAME //(
                // Infoframe workaround - enable CKDT status
                SiiRegModify(REG_TMDS_CCTRL, BIT_TMDS_CCTRL_CKDT_EN, BIT_TMDS_CCTRL_CKDT_EN);
#endif //)
        }

#ifdef WAKE_ON_CBUS_FALLING_EDGE_ONLY //(
        SiiRegWrite(REG_DISC_STAT2, BIT_DISC_STAT2_FALLING_EDGE_WAKE_ENABLE);	// Exit D3 via CBUS falling edge
#endif //)

        if (g_chipDeviceId == BISHOP_DEVICE_ID)
        {
                SiiRegWrite(REG_PWD_SRST, 0x80); 	 // MHL Auto FIFO Reset
        }
#ifdef Disconnected_USB_ID_with_CBUS_ID_As_defult
        //SiiRegModify(REG_DISC_CTRL3, BIT3, 0);
        SiiRegModify(REG_DISC_CTRL6, BIT6, BIT6);
#endif
}

///////////////////////////////////////////////////////////////////////////
// InitCBusRegs
//
///////////////////////////////////////////////////////////////////////////
PLACE_IN_CODE_SEG uint8_t devCap[16]=
{
        DEVCAP_VAL_DEV_STATE
        ,DEVCAP_VAL_MHL_VERSION
        ,DEVCAP_VAL_DEV_CAT
        ,DEVCAP_VAL_ADOPTER_ID_H
        ,DEVCAP_VAL_ADOPTER_ID_L
        ,DEVCAP_VAL_VID_LINK_MODE
        ,DEVCAP_VAL_AUD_LINK_MODE
        ,DEVCAP_VAL_VIDEO_TYPE
        ,DEVCAP_VAL_LOG_DEV_MAP
        ,DEVCAP_VAL_BANDWIDTH
        ,DEVCAP_VAL_FEATURE_FLAG
        ,DEVCAP_VAL_DEVICE_ID_H
        ,DEVCAP_VAL_DEVICE_ID_L
        ,DEVCAP_VAL_SCRATCHPAD_SIZE
        ,DEVCAP_VAL_INT_STAT_SIZE
        ,DEVCAP_VAL_RESERVED
};
static void InitCBusRegs (void)
{
        TXD_DEBUG_PRINT(("Drv: InitCBusRegs\n"));

//2011-12-08 no need to write	SiiRegWrite(REG_CBUS_COMMON_CONFIG, 0xF2); 			 // Increase DDC translation layer timer

        // Setup our devcap
        // make devcap loading quicker by using block write
        SiiRegWriteBlock(REG_CBUS_DEVICE_CAP_0,devCap,sizeof(devCap));

}

///////////////////////////////////////////////////////////////////////////
//
// ForceUsbIdSwitchOpen
//
///////////////////////////////////////////////////////////////////////////
static void ForceUsbIdSwitchOpen (void)
{
#ifdef Disconnected_USB_ID_with_CBUS_ID_As_defult
        SiiRegModify(REG_DISC_CTRL6, BIT6, BIT6);				// Force USB ID switch to open
        SiiRegWrite(REG_DISC_CTRL3, 0x86);//SiiRegWrite(REG_DISC_CTRL3, 0x8E);
#else
        DISABLE_DISCOVERY
        SiiRegModify(REG_DISC_CTRL6, BIT_DC6_USB_OVERRIDE_MASK, BIT_DC6_USB_OVERRIDE_ON);				// Force USB ID switch to open
        SiiRegWrite(REG_DISC_CTRL3,  BIT_DC3_COMM_IMME_ON
                    | BIT_DC3_FORCE_MHL_OFF
                    | BIT_DC3_DISC_SIMODE_OFF
                    | BIT_DC3_FORCE_USB_OFF
                    | BIT_DC3_USB_EN_OFF
                    | BIT_DC3_DLYTRG_SEL_064ms
                   );
#endif
        // Force HPD to 0 when not in Mobile HD mode.
        SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow ();
}
///////////////////////////////////////////////////////////////////////////
//
// ReleaseUsbIdSwitchOpen
//
///////////////////////////////////////////////////////////////////////////
static void ReleaseUsbIdSwitchOpen (void)
{
#ifdef Disconnected_USB_ID_with_CBUS_ID_As_defult
        SiiRegModify(REG_DISC_CTRL3, BIT3, BIT3);
        SiiRegModify(REG_DISC_CTRL6, BIT6, BIT6);
#else
        HalTimerWait(50); // per spec
        SiiRegModify(REG_DISC_CTRL6, BIT6, 0x00);
        ENABLE_DISCOVERY
#endif
}
/*
	SiiMhlTxDrvProcessMhlConnection
		optionally called by the MHL Tx Component after giving the OEM layer the
		first crack at handling the event.
*/
void SiiMhlTxDrvProcessRgndMhl( void )
{
        //perhaps this could be done in the future, but not now SiiRegModify(REG_DISC_CTRL9, BIT_DC9_VBUS_OUTPUT_CAPABILITY_SRC, BIT_DC9_VBUS_OUTPUT_CAPABILITY_SRC);
}
///////////////////////////////////////////////////////////////////////////
// ProcessRgnd
//
// H/W has detected impedance change and interrupted.
// We look for appropriate impedance range to call it MHL and enable the
// hardware MHL discovery logic. If not, disable MHL discovery to allow
// USB to work appropriately.
//
// In current chip a firmware driven slow wake up pulses are sent to the
// sink to wake that and setup ourselves for full D0 operation.
///////////////////////////////////////////////////////////////////////////
uint8_t RgndResultIsMhl (void)
{
        if (g_chipDeviceId == BISHOP_DEVICE_ID)
        {
                return 1;
        }
        else if (g_chipDeviceId == WOLV60_DEVICE_ID)
        {
                uint8_t rgndImpedance;

                //
                // Impedance detection has completed - process interrupt
                //
                rgndImpedance = SiiRegRead(REG_DISC_STAT2) & 0x03;
                TXD_DEBUG_PRINT(("Drv: RGND = %02X : \n", (int)rgndImpedance));

                //
                // 00, 01 or 11 means USB.
                // 10 means 1K impedance (MHL)
                //
                // If 1K, then only proceed with wake up pulses
                if (0x02 == rgndImpedance)
                {
                        TXD_DEBUG_PRINT(("(MHL Device)\n"));
                        SiiRegModify(REG_DISC_CTRL9, BIT_DC9_USB_EST,0 );	// USB Established
                        return 1;
                }
                else
                {
#ifdef Disconnected_USB_ID_with_CBUS_ID_As_defult
                        //STROBE_POWER_ON // Force Power State to ON
                        SiiRegModify(TPI_DEVICE_POWER_STATE_CTRL_REG, TX_POWER_STATE_MASK, 0x00);
                        SiiRegModify(REG_DISC_CTRL6, BIT6, 0x00);
                        if(rgndImpedance == 0x03)
                        {
                                SiiRegModify(REG_DISC_CTRL6, BIT6, BIT6);
                                SiiRegModify(REG_DISC_CTRL3, BIT3, BIT3);
                                MASK_INTR_4_INTERRUPTS;
                                k3v2_otg_id_status_change(ID_FALL);
                                //if USB OTG,not switch to D3,just keep in D0,this will increase current consumption
                                return 0;
                        }
#endif

                        SiiRegModify(REG_DISC_CTRL9, BIT_DC9_USB_EST, BIT_DC9_USB_EST);	// USB Established
                        TXD_DEBUG_PRINT(("(Non-MHL Device)\n"));
                        return 0;
                }
        }
        return 0;
}


////////////////////////////////////////////////////////////////////
// SwitchToD0
// This function performs s/w as well as h/w state transitions.
//
// Chip comes up in D2. Firmware must first bring it to full operation
// mode in D0.
////////////////////////////////////////////////////////////////////
void SwitchToD0 (void)
{
        TXD_DEBUG_PRINT(("Drv: Switch to D0\n"));

        //
        // WriteInitialRegisterValuesPartOne switches the chip to full power mode.
        //
        WriteInitialRegisterValuesPartOne();

        // Force Power State to ON

        STROBE_POWER_ON // Force Power State to ON

        UpdatePowerState(POWER_STATE_D0_NO_MHL);
}


#ifdef Disconnected_USB_ID_with_CBUS_ID_As_defult
void CBUS_ID_Opened_AfterPlugedout( void )
{
        TXD_DEBUG_PRINT(("Drv: USB_ID should disconnected with CBUS_ID now when OTG pluged out.\n"));

        SiiRegWrite(REG_INTR4, 0xFF); // clear all interrupts except MHL_EST
        SiiRegWrite(REG_INTR1, 0xFF);
        SiiRegWrite(REG_INTR2, 0xFF);
        SiiRegWrite(REG_INTR3, 0xFF);
        SiiRegWrite(REG_INTR5, 0xFF);
        SiiRegWrite(REG_INTR8, 0xFF);
        SiiRegWrite(REG_INTR7, 0xFF);
        SiiRegWrite(REG_INTR9, 0xFF);
        SiiRegWrite(REG_INTR9, 0xFF);
        SiiRegWrite(REG_TPI_INTR_ST0,0xFF);
        SiiRegWrite(REG_CBUS_INT_0,0xFF);
        SiiRegWrite(REG_CBUS_INT_1, 0xFF);
        UNMASK_INTR_4_INTERRUPTS;
        ForceUsbIdSwitchOpen();
        ENABLE_DISCOVERY
        CLR_BIT(REG_DPD, 0);
        fwPowerState = POWER_STATE_D3;
}


static void	DetectRgndtoSetUSBID( void )
{
        uint8_t		reg99RGNDRange;
        //
        // Impedance detection has completed - process interrupt
        //
        reg99RGNDRange = SiiRegRead(REG_DISC_STAT2) & 0x03;
        TXD_DEBUG_PRINT(("Drv:before D3 detect RGND,  RGND Reg 99 = %02X\n", (int)reg99RGNDRange));

        //
        // Reg 0x99
        // 00, 01 or 11 means USB.
        // 10 means 1K impedance (MHL)
        if(0x00 == reg99RGNDRange|| 0x02 == reg99RGNDRange)
        {
                TXD_DEBUG_PRINT(("Drv: MHL peer pluged out, USB_ID should disconnected with CBUS_ID now.\n"));
                ForceUsbIdSwitchOpen();
                {
                        uint16_t temp;
                        temp = SiiRegRead(REG_INTR4);
                        TXD_DEBUG_PRINT(("SiiRegRead(REG_INTR4): %02x have been cleaned\n",temp));
                        SiiRegWrite(REG_INTR4, 0xBF); // clear all interrupts except MHL_EST
                        SiiRegWrite(REG_INTR1, 0xFF);
                        SiiRegWrite(REG_INTR2, 0xFF);
                        SiiRegWrite(REG_INTR3, 0xFF);
                        SiiRegWrite(REG_INTR5, 0xFF);
                        SiiRegWrite(REG_INTR8, 0xFF);
                        SiiRegWrite(REG_INTR7, 0xFF);
                        SiiRegWrite(REG_INTR9, 0xFF);
                        SiiRegWrite(REG_INTR9, 0xFF);
                        SiiRegWrite(REG_TPI_INTR_ST0,0xFF);
                        SiiRegWrite(REG_CBUS_INT_0,0xFF);
                        SiiRegWrite(REG_CBUS_INT_1, 0xFF);

                }
                //TXD_DEBUG_PRINT(("Drv: ->D3  0x92:0x%02x  0x95:0x%02x\n",(int)ReadBytePage0(0x92),(int)ReadBytePage0(0x95)));
                //
                // Change state to D3 by clearing bit 0 of 3D (SW_TPI, Page 1) register
                //
                //CLR_BIT(REG_DPD, 0);
                SiiRegWrite(REG_DPD, 0x74);
                UpdatePowerState( POWER_STATE_D3 );
        }
        else
        {

                TXD_DEBUG_PRINT(("Drv: OTG detected. USB_ID should connected with CBUS_ID now.\n"));
                MASK_INTR_4_INTERRUPTS;
                ReleaseUsbIdSwitchOpen();
                //SiiRegWrite(REG_DPD, 0x74);
                //UpdatePowerState( POWER_STATE_D3 );
        }

}
#endif

////////////////////////////////////////////////////////////////////
// SwitchToD3
//
// This function performs s/w as well as h/w state transitions.
//
////////////////////////////////////////////////////////////////////
extern void CBusQueueReset(void);
void SwitchToD3 (void)
{
        if(POWER_STATE_D3 != fwPowerState)
        {
                TXD_DEBUG_PRINT(("Drv: Switch To D3...\n"));

#ifndef	__KERNEL__ //(
                pinM2uVbusCtrlM = 1;
                pinMhlConn = 1;
                pinUsbConn = 0;
#endif	//)
                // Force HPD to 0 when not in MHL mode.
                SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow();

                // Change TMDS termination to high impedance on disconnection
                // Bits 1:0 set to 11


                SiiRegWrite(REG_MHLTX_CTL1, BIT_MHLTX_CTL1_TX_TERM_MODE_OFF | BIT_MHLTX_CTL1_DISC_OVRIDE_ON) ;

                SiiRegWrite(REG_DISC_CTRL2, 0xAD); // set back to default value.

                //( FP1798 let cbus discovery state machine control RSEN detection
                // SiiRegModify(REG_IO_CTRL,BIT_IO_CTRL_RSEN_CTRL_MASK,BIT_IO_CTRL_RSEN_CTRL_CDSM);
                //) FP1798

                //FP1798  clear discovery interrupts before going to sleep.
                {
                        uint16_t temp;
                        temp = SiiRegRead(REG_INTR4);
                        TXD_DEBUG_PRINT(("temp: %02x\n",temp));
#ifdef CLEAR_INT_ENABLE_IN_D3 //(
                        SiiMhlTxDrvDisableAllInterruptsExceptRgnd();
#else //)(
                        SiiRegWrite(REG_INTR4, 0xFB); // clear all interrupts except MHL_EST
#endif //)
                }
                //
                // GPIO controlled from SiIMon can be utilized to disallow
                // low power mode, thereby allowing SiIMon to read/write register contents.
                // Otherwise SiIMon reads all registers as 0xFF
                //
                //if(PlatformGPIOGet(pinAllowD3))
                // wait Tsrc:cbus_float
                HalTimerWait(50);
                ENABLE_DISCOVERY

                CBusQueueReset();
#ifdef Disconnected_USB_ID_with_CBUS_ID_As_defult
                DetectRgndtoSetUSBID();
#else
//		if(pinAllowD3)
//		{
                // wait Tsrc:cbus_float
                //HalTimerWait(50);
                //
                // Change state to D3 by clearing bit 0 of 3D (SW_TPI, Page 1) register
                // ReadModifyWriteIndexedRegister(INDEXED_PAGE_1, 0x3D, BIT_0, 0x00);
                //
                {
                        uint16_t temp;
                        temp = SiiRegRead(REG_INTR4);
                        TXD_DEBUG_PRINT(("SiiRegRead(REG_INTR4): %02x have been cleaned\n",temp));
                        SiiRegWrite(REG_INTR4, 0xBF); // clear all interrupts except MHL_EST
                        SiiRegWrite(REG_INTR1, 0xFF);
                        SiiRegWrite(REG_INTR2, 0xFF);
                        SiiRegWrite(REG_INTR3, 0xFF);
                        SiiRegWrite(REG_INTR5, 0xFF);
                        SiiRegWrite(REG_INTR8, 0xFF);
                        SiiRegWrite(REG_INTR7, 0xFF);
                        SiiRegWrite(REG_INTR9, 0xFF);
                        SiiRegWrite(REG_INTR9, 0xFF);
                        SiiRegWrite(REG_TPI_INTR_ST0,0xFF);
                        SiiRegWrite(REG_CBUS_INT_0,0xFF);
                        SiiRegWrite(REG_CBUS_INT_1, 0xFF);

                }
                SiiRegWrite(REG_DPD, 0x74);

                UpdatePowerState( POWER_STATE_D3 );

//		}
#endif

                /*
                else
                {
                ENABLE_DISCOVERY
                	UpdatePowerState(POWER_STATE_D0_NO_MHL);
                }*/
        }
}


////////////////////////////////////////////////////////////////////
// Int4Isr
//
//
//	Look for interrupts on INTR4 (Register 0x21)
//		7 = RSVD		(reserved)
//		6 = RGND Rdy	(interested)
//		5 = CBUS Disconnect	(interested)
//		4 = CBUS LKOUT	(interested)
//		3 = USB EST		(interested)
//		2 = MHL EST		(interested)
//		1 = RPWR5V Change	(ignore)
//		0 = SCDT Change	(interested during D0)
//
////////////////////////////////////////////////////////////////////
static int Int4Isr (void)
{
        uint8_t int4Status;

        int4Status = SiiRegRead(REG_INTR4);	// read status

        // When I2C is inoperational (D3) and a previous interrupt brought us here, do nothing.
        if(0xFF != int4Status)
        {
                if(int4Status)
                {
                        TXD_DEBUG_PRINT(("Drv: INT4 Status = %02X\n", (int) int4Status));
                        /* Now that RGND interrupts are deferred until the chip has
                            awoken sufficiently to allow I2C reads to succeed,
                            we can clear the interupt right away.
                        */

#define DISCOVERY_D3_INT_MASK (BIT_INTR4_RGND_DETECTION | BIT_INTR4_CBUS_DISCONNECT | BIT_INTR4_NON_MHL_EST)
                        SiiRegWrite(REG_INTR4,int4Status
#ifdef PRESERVE_RGND_IMPEDANCE_UNTIL_VALUE_READ//(
                                    & ~DISCOVERY_D3_INT_MASK
#endif //)
                                   ); // clear all interrupts
                        TXD_DEBUG_PRINT(("Drv: INT4 Status = %02X\n", (int) SiiRegRead(REG_INTR4)));

                        if (int4Status & BIT_INTR4_RGND_DETECTION)
                        {
                                // already an MHL connection?
                                if (POWER_STATE_D0_MHL != fwPowerState)
                                {
                                        if (RgndResultIsMhl())
                                        {
                                                // Switch to full power mode.
                                                SwitchToD0();

                                                SiiMhlTxNotifyRgndMhl(); // this will call the application and then optionally call
                                                // ignore other events until next interrupt.
#ifdef PRESERVE_RGND_IMPEDANCE_UNTIL_VALUE_READ //(
                                                SiiRegWrite(REG_INTR4,int4Status);
#endif //)
                                                // enable a few more interrupts
                                                SiiRegWrite(REG_INTR4_MASK,DEFAULT_INTR4_MASK);

                                                return I2C_ACCESSIBLE;
                                        }
                                        else
                                        {

                                                UpdatePowerState(POWER_STATE_D0_NO_MHL);

#ifdef PRESERVE_RGND_IMPEDANCE_UNTIL_VALUE_READ //(
                                                SiiRegWrite(REG_INTR4,int4Status);
#endif //)
                                                SiiRegWrite(REG_INTR4_MASK,DEFAULT_INTR4_MASK);
                                                return I2C_INACCESSIBLE;
                                        }
                                }
#ifdef PRESERVE_RGND_IMPEDANCE_UNTIL_VALUE_READ //(
                                else
                                {
                                        ERROR_DEBUG_PRINT(("unexpected RGND interrupt\n"));
                                        // clear the interrupt if in D0_MHL
                                        SiiRegWrite(REG_INTR4,int4Status);
                                }
#endif //)
                        }

                        if (POWER_STATE_D0_NO_MHL == fwPowerState)
                        {
                                if(int4Status & BIT_INTR4_MHL_EST) // MHL_EST_INT
                                {
                                        if (g_chipDeviceId == BISHOP_DEVICE_ID)
                                        {
                                                // Switch to full power mode.
                                                SwitchToD0();
                                        }
                                        WriteInitialRegisterValuesPartTwo(1);  // different behavior from first time init.
                                        MhlTxDrvProcessConnection();
                                        // ignore other events until next interrupt.
                                        return I2C_ACCESSIBLE;

                                }

                                if(int4Status & BIT_INTR4_NON_MHL_EST)
                                {
                                        TXD_DEBUG_PRINT(("Drv: Non- MHL device detected.\n"));

                                        SET_BIT(REG_DPD, 0);
                                        STROBE_POWER_ON
#ifdef PRESERVE_RGND_IMPEDANCE_UNTIL_VALUE_READ //(
                                        SiiRegWrite(REG_INTR4,int4Status);
#endif //)
                                        SwitchToD3();
                                        return I2C_INACCESSIBLE;
                                }
                        }
                        else if (POWER_STATE_D0_MHL == fwPowerState)
                        {
                                if (int4Status & BIT_INTR4_CBUS_DISCONNECT)
                                {
                                        TXD_DEBUG_PRINT(("Drv: CBUS Disconnect.\n"));
                                        MhlTxDrvProcessDisconnection();

                                        return I2C_INACCESSIBLE;
                                }
                        }
                        // Can't succeed at these in D3
                        if(fwPowerState != POWER_STATE_D3)
                        {
                                // CBUS Lockout interrupt?
                                if (int4Status & BIT_INTR4_CBUS_LKOUT)
                                {
                                        TXD_DEBUG_PRINT(("Drv: CBus Lockout\n"));

                                        ForceUsbIdSwitchOpen();
                                        ReleaseUsbIdSwitchOpen();
                                }
                        }
                }
        }
        return I2C_ACCESSIBLE;
}

static void ProcessClockDetect(void)
{
        uint8_t cstatP3 = SiiRegRead(REG_TMDS_CSTAT_P3);

//  ERROR_DEBUG_PRINT(("CKDT Reg (2A0) = %02X\n",(uint16_t)cstatP3));
        if (BIT_TMDS_CSTAT_P3_PDO_CLOCK_DETECTED== (cstatP3 & BIT_TMDS_CSTAT_P3_PDO_MASK))
        {
                //uint8_t intr5Mask = SiiRegRead(REG_INTR5_MASK);
                ERROR_DEBUG_PRINT(("CKDT HIGH\n"));
#ifdef SWWA_INFOFRAME //(
                if (BIT_INTR5_CKDT_CHANGE & intr5Mask)
                {
                        uint8_t new_ckdt = BIT_TMDS_CSTAT_P3_PDO_MASK & SiiRegRead(REG_TMDS_CSTAT_P3);
                        if (new_ckdt)
                        {
                                // Save old value of 281[5:0] and write 0
                                old_r281 = SiiRegRead(REG_MHLTX_CTL2);

                                ERROR_DEBUG_PRINT(("SWWA_INFR: 080= %02X, 2A0= %02X, 07C= %02X, 281= %02X\n",
                                                   (int)SiiRegRead(REG_TMDS_CCTRL),
                                                   (int)SiiRegRead(REG_TMDS_CSTAT_P3),
                                                   (int)SiiRegRead(REG_INTR8),
                                                   (int)old_r281
                                                  ));
                                // Enable TMDS output power (keep muted) to work around infoframe interrupt issue.
                                SiiRegWrite(REG_MHLTX_CTL2, (old_r281 & (~BIT_MHL_TX_CTL2_TX_OE_MASK)));	// Preserve bits 6, 7

                                // Enable TMDS output power (keep muted) to work around infoframe interrupt issue.
                                SiiRegModify(REG_TMDS_CCTRL, BIT_TMDS_CCTRL_TMDS_OE, BIT_TMDS_CCTRL_TMDS_OE);

                                // do this again to be sure
                                SiiRegModify(REG_MHLTX_CTL2,BIT_MHL_TX_CTL2_TX_OE_MASK,0);

                                ckdtStatus = cdsTriggered;
                        }
                }
#endif //)
        }
        else
        {
                ERROR_DEBUG_PRINT(("CKDT LOW\n"));
        }
}

////////////////////////////////////////////////////////////////////
// Int5Isr
//
//
////////////////////////////////////////////////////////////////////
static void Int5Isr (void)
{
        uint8_t int5Status;

        int5Status = SiiRegRead(REG_INTR5);

        if (int5Status)
        {
//		ERROR_DEBUG_PRINT(("Got INTR5 = %02X\n", (int)int5Status));
                if(int5Status & BIT_INTR5_CKDT_CHANGE)
                {
                        SiiRegWrite(REG_INTR5, BIT_INTR5_CKDT_CHANGE);	// clear CKDT change interrupt
                        int5Status &= ~BIT_INTR5_CKDT_CHANGE;

//            ERROR_DEBUG_PRINT(("CKDT interrupt\n"));
                        mdt_tester = 1;
                        /*
                            Disable the timer that we set when we
                                enabled upstream HPD
                        */
                        ProcessClockDetect();
                }

                if (int5Status & BIT_INTR5_SCDT_CHANGE)
                {
                        TXD_DEBUG_PRINT(("Drv: BIT_INTR5_SCDT_CHANGE.\n"));
                        if (!(mfdrvTranscodeMode & g_modeFlags))
                        {
                                uint8_t cstatP3 = SiiRegRead(REG_TMDS_CSTAT_P3);
                                if(BIT_TMDS_CSTAT_P3_SCDT & cstatP3)
                                {
                                        ERROR_DEBUG_PRINT(("SCDT HIGH\n"));
                                        {
                                                /*
                                                 * This workaround ensures NEW_AVI interrupt when AVI does not change
                                                 * but due to downstream HDMI or MHL hot plug, we toggle upstream HPD
                                                 * and expect a new AVI.
                                                 */
                                                HalTimerWait(25);
                                                SiiRegWrite(REG_HDMI_BCH_CORRECTED_THRESHOLD, 0x01);
                                                SiiRegWrite(REG_RX_HDMI_CTRL4, 0x30);
                                        }
                                        SiiMhlTxNotifySyncDetect(BIT_TMDS_CSTAT_P3_SCDT & cstatP3);
                                }
                                else
                                {
                                        ERROR_DEBUG_PRINT(("SCDT LOW\n"));
#ifdef	NEVER
                                        SiiMhlTxDrvTmdsControl(false);
#endif // NEVER
                                        {
                                                /* Next two lines implement missing NEW_AVI interrupt during hot plugs */
                                                SiiRegWrite(REG_HDMI_BCH_CORRECTED_THRESHOLD,0);
                                                SiiRegWrite(REG_RX_HDMI_CTRL4, 0x00);
                                        }
                                }
                        }
                }

                if (int5Status & BIT_INTR5_RPWR5V_CHG)
                {
                        /*
                        	todo: upstream transmitter is ready to read EDID.
                        	Set a flag here to check later when downstream HPD
                        	comes in.
                        */
                        uint8_t tmdsCStatus;
                        tmdsCStatus = SiiRegRead(REG_TMDS_CSTAT);
                        if (tmdsCStatus & BIT_TMDS_CSTAT_RPWR5V_STATUS)
                        {
                                TXD_DEBUG_PRINT(("Drv: RPWR5V low to high xition.\n"));
                                /* todo: refresh EDID on transition from low to high */
                        }
                }
                SiiRegWrite(REG_INTR5, int5Status);	// clear all interrupts
        }

}
///////////////////////////////////////////////////////////////////////////
//
// MhlTxDrvProcessConnection
//
///////////////////////////////////////////////////////////////////////////
static void MhlTxDrvProcessConnection (void)
{
        TXD_DEBUG_PRINT (("Drv: MHL Cable Connected. CBUS:0x0A = %02X fwPowerState:%d\n", (int) SiiRegRead(REG_CBUS_STATUS),(unsigned int)fwPowerState));  // New :  CBUS_STATUS

        if( POWER_STATE_D0_MHL == fwPowerState )
        {
                TXD_DEBUG_PRINT (("Drv: Already in D0_MHL fwPowerState:%d\n", (unsigned int)fwPowerState));
                return;
        }
//( FP1798
        /*
            if we're called when power state is D3,
                then we abort and wait for RGND
        */
        if ( POWER_STATE_D3 == fwPowerState)
        {
                TXD_DEBUG_PRINT (("Drv: I2C registers inaccessible in fwPowerState:%d\n", (unsigned int)fwPowerState));
                return;
        }
//) FP1798

#ifndef	__KERNEL__ //(
        TXD_DEBUG_PRINT (("Drv: Controlling VBUS fwPowerState:%d\n", (unsigned int)fwPowerState));
        // VBUS control gpio
        pinM2uVbusCtrlM = 0;
        pinMhlConn = 0;
        pinUsbConn = 1;
#endif	// )

        TXD_DEBUG_PRINT (("Drv: trying to control upstream HDP\n"));
        SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow();
        //
        // Discovery over-ride: reg_disc_ovride
        //
        SiiRegWrite(REG_MHLTX_CTL1, BIT_MHLTX_CTL1_TX_TERM_MODE_100DIFF | BIT_MHLTX_CTL1_DISC_OVRIDE_ON);

        UpdatePowerState(POWER_STATE_D0_MHL);

        //
        // Increase DDC translation layer timer (uint8_t mode)
        //SiiRegWrite(REG_CBUS_DDC_TIMEOUT,0x0F);


        // Keep the discovery enabled. Need RGND interrupt
        ENABLE_DISCOVERY

        // Notify upper layer of cable connection
        SiiMhlTxNotifyConnection(true);
        HalTimerWait(500);
}

///////////////////////////////////////////////////////////////////////////
//
// MhlTxDrvProcessDisconnection
//
///////////////////////////////////////////////////////////////////////////
static void MhlTxDrvProcessDisconnection (void)
{

        TXD_DEBUG_PRINT(("Drv: MhlTxDrvProcessDisconnection\n"));

        // clear all interrupts
        SiiRegWrite(REG_INTR4, SiiRegRead(REG_INTR4));

        SiiRegWrite(REG_MHLTX_CTL1, BIT_MHLTX_CTL1_TX_TERM_MODE_OFF | BIT_MHLTX_CTL1_DISC_OVRIDE_ON);


        dsHpdStatus &= ~BIT_CBUS_HPD;  //cable disconnect implies downstream HPD low
#ifdef SWWA_INFOFRAME //(
        ckdtStatus=cdsFirstTime;
#endif //)
#ifdef INTERRUPT_DRIVEN_EDID //(
        edidFifoBlockNumber=0;
#ifdef REQUEST_EDID_ATOMICALLY //(
        currentEdidRequestBlock = 0;
#endif //)
#endif //)
        SiiRegWrite(REG_CBUS_STATUS, dsHpdStatus);    // New :	 MSC_MT_ABORT_INT
        SiiMhlTxNotifyDsHpdChange(0);

        if( POWER_STATE_D0_MHL == fwPowerState )
        {
                // Notify upper layer of cable removal
                SiiMhlTxNotifyConnection(false);
        }

        // Now put chip in sleep mode
        SwitchToD3();
}

///////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvEnableInterruptMasks
//
///////////////////////////////////////////////////////////////////////////
void SiiMhlTxDrvEnableInterruptMasks (void)
{
        uint8_t enable[4]= {0xff,0xff,0xff,0xff}; // must write 0xFF to clear regardless!

        mscCmdInProgress = false;

        {
                int i;
                for (i =0; i <sizeof(g_InterruptMasks)/sizeof(g_InterruptMasks[0]); ++i)
                {
                        SiiRegWrite(g_InterruptMasks[i].regNameMask,g_InterruptMasks[i].value);
                }
        }

        if (mfdrvTranscodeMode & g_modeFlags)
        {
                int i;
                for (i =0; i <sizeof(g_TranscodeInterruptMasks)/sizeof(g_TranscodeInterruptMasks[0]); ++i)
                {
                        SiiRegWrite(g_TranscodeInterruptMasks[i].regNameMask,g_TranscodeInterruptMasks[i].value);
                }
        }
        else
        {
                int i;
                for (i =0; i <sizeof(g_NonTranscodeInterruptMasks)/sizeof(g_NonTranscodeInterruptMasks[0]); ++i)
                {
                        SiiRegWrite(g_NonTranscodeInterruptMasks[i].regNameMask,g_NonTranscodeInterruptMasks[i].value);
                }
        }
        // Enable SET_INT interrupt for writes to all 4 MSC Interrupt registers.
        SiiRegWriteBlock(REG_CBUS_SET_INT_ENABLE_0,enable,4);
}

#ifdef CLEAR_INT_ENABLE_IN_D3 //(
void SiiMhlTxDrvDisableAllInterruptsExceptRgnd( void )
{
        uint8_t disable[4]= {0x00,0x00,0x00,0x00};
        uint8_t clear[4]= {0xFF,0xFF,0xFF,0xFF};

        {
                int i;
                for (i =0; i <sizeof(g_InterruptMasks)/sizeof(g_InterruptMasks[0]); ++i)
                {

                        if (REG_INTR4 != g_InterruptMasks[i].regNameStatus)
                        {
                                SiiRegWrite(g_InterruptMasks[i].regNameMask,0x00);
                                SiiRegWrite(g_InterruptMasks[i].regNameStatus,0xFF);
                        }
                        else
                        {
                                SiiRegWrite(REG_INTR4_MASK,BIT_INTR4_RGND_DETECTION);
                                SiiRegWrite(REG_INTR4,0xFF);  // not sure about this
                        }
                }
        }

        if (mfdrvTranscodeMode & g_modeFlags)
        {
                int i;
                for (i =0; i <sizeof(g_TranscodeInterruptMasks)/sizeof(g_TranscodeInterruptMasks[0]); ++i)
                {
                        SiiRegWrite(g_TranscodeInterruptMasks[i].regNameMask,0x00);
                        SiiRegWrite(g_TranscodeInterruptMasks[i].regNameStatus,0xFF);
                }
        }
        else
        {
                int i;
                for (i =0; i <sizeof(g_NonTranscodeInterruptMasks)/sizeof(g_NonTranscodeInterruptMasks[0]); ++i)
                {
                        SiiRegWrite(g_NonTranscodeInterruptMasks[i].regNameMask,0x00);
                        SiiRegWrite(g_NonTranscodeInterruptMasks[i].regNameStatus,0xFF);
                }
        }
        // disable SET_INT interrupt for writes to all 4 MSC Interrupt registers.
        SiiRegWriteBlock(REG_CBUS_SET_INT_ENABLE_0,disable,4);
        SiiRegWriteBlock(REG_CBUS_SET_INT_0, clear, 4);

}
#endif //)
///////////////////////////////////////////////////////////////////////////
//
// CBusProcessErrors
//
//
///////////////////////////////////////////////////////////////////////////
static uint8_t CBusProcessErrors (uint8_t int1Status)
{
        uint8_t result          = 0;
//    uint8_t mscAbortReason  = 0;
        uint8_t ddcAbortReason  = 0;



        /* At this point, we only need to look at the abort interrupts. */

        int1Status &= (BIT_CBUS_MSC_ABORT_RCVD| BIT_CBUS_DDC_ABRT | BIT_CBUS_CMD_ABORT);

        if ( int1Status )
        {
//      result = ERROR_CBUS_ABORT;		// No Retry will help

                /* If transfer abort or MSC abort, clear the abort reason register. */
                if( int1Status & BIT_CBUS_DDC_ABRT)
                {
                        result = ddcAbortReason = SiiRegRead(REG_CBUS_DDC_ABORT_INT);
                        TXD_DEBUG_PRINT(("CBUS DDC ABORT happened, reason:: %02X\n", (int)(ddcAbortReason)));
                }

                if (BIT_CBUS_CMD_ABORT & int1Status)
                {
                        // last command sent was not successful
                }

                if ( int1Status & BIT_CBUS_MSC_ABORT_RCVD)  // New :   CBUS_INT_1
                {
                        uint8_t mscMtAbortIntStatus   = 0;

                        mscMtAbortIntStatus = SiiRegRead(REG_CBUS_MSC_MT_ABORT_INT);
                        if (mscMtAbortIntStatus)
                        {
                                TXD_DEBUG_PRINT(("CBUS:: MSC Transfer ABORTED. Clearing 0x0D\n"));
                                SiiRegWrite(REG_CBUS_MSC_MT_ABORT_INT,mscMtAbortIntStatus);
                                if (BIT_CBUS_MSC_MT_ABORT_INT_MAX_FAIL & mscMtAbortIntStatus)
                                {
                                        TXD_DEBUG_PRINT(("Requestor MAXFAIL - retry threshold exceeded\n"));
                                }
                                if (BIT_CBUS_MSC_MT_ABORT_INT_PROTO_ERR & mscMtAbortIntStatus)
                                {
                                        TXD_DEBUG_PRINT(("Protocol Error\n"));
                                }
                                if (BIT_CBUS_MSC_MT_ABORT_INT_TIMEOUT & mscMtAbortIntStatus)
                                {
                                        TXD_DEBUG_PRINT(("Requestor translation layer timeout\n"));
                                }
                                if (BIT_CBUS_MSC_MT_ABORT_INT_UNDEF_CMD & mscMtAbortIntStatus)
                                {
                                        TXD_DEBUG_PRINT(("Undefined opcode\n"));
                                }
                                if (BIT_CBUS_MSC_MT_ABORT_INT_MSC_MT_PEER_ABORT & mscMtAbortIntStatus)
                                {
                                        TXD_DEBUG_PRINT(("CBUS:: MSC Peer sent an ABORT.\n"));
                                }
                        }
                }
        }
        return( result );
}

void SiiMhlTxDrvGetScratchPad (uint8_t startReg,uint8_t *pData,uint8_t length)
{
        uint8_t offset=startReg & 0xF;
        uint8_t count = 16-offset;

        TXD_DEBUG_PRINT(("Drv: Getting Scratch Pad.\n"));
        while (length > 0)
        {
                SiiRegReadBlock(REG_CBUS_MHL_SCRPAD_0+offset, pData, count);
                length -=count;
                count = (length > 16)? 16 : length;
        }
}

static uint8_t lastCBusInt0Status=0;
/*
 SiiMhlTxDrvMscMsgNacked
    returns:
        0 - message was not NAK'ed
        non-zero message was NAK'ed
 */
int SiiMhlTxDrvMscMsgNacked()
{
        if (BIT_CBUS_MSC_MT_DONE_NACK & lastCBusInt0Status)
        {

                TXD_DEBUG_PRINT(("MSC MSG NAK'ed - retrying...\n\n"));
                return 1;
        }
        return 0;
}

void SiiMhlTxDrvProcessDownStreamHPDChange(uint8_t cbusStatus)
{
        uint8_t status = cbusStatus & BIT_CBUS_HPD;
        // Remember and update before calling up to componenet layer
        dsHpdStatus = cbusStatus;

        // Inform upper layer of change in Downstream HPD
        SiiMhlTxNotifyDsHpdChange( status );

        if ( mfdrvTranscodeMode & g_modeFlags )
        {
                if (status)
                {
                        SiiMhlTxDrvReleaseUpstreamHPDControl();  // this triggers an EDID read
                }
        }
        else
        {
                if (!status)
                {
                        SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow();
#ifdef INTERRUPT_DRIVEN_EDID //(
#ifdef REQUEST_EDID_ATOMICALLY //(
                        currentEdidRequestBlock = 0;
#endif //)
#endif //)
                }
        }
}

///////////////////////////////////////////////////////////////////////////
//
// MhlCbusIsr
//
// Only when MHL connection has been established. This is where we have the
// first looks on the CBUS incoming commands or returned data bytes for the
// previous outgoing command.
//
// It simply stores the event and allows application to pick up the event
// and respond at leisure.
//
// Look for interrupts on CBUS:CBUS_INTR_STATUS [0xC8:0x08]
//		7 = RSVD			(reserved)
//		6 = MSC_RESP_ABORT	(interested)
//		5 = MSC_REQ_ABORT	(interested)
//		4 = MSC_REQ_DONE	(interested)
//		3 = MSC_MSG_RCVD	(interested)
//		2 = DDC_ABORT		(interested)
//		1 = RSVD			(reserved)
//		0 = rsvd			(reserved)
///////////////////////////////////////////////////////////////////////////
static void MhlCbusIsr (void)
{
        uint8_t		cbusInt;

        //
        // Main CBUS interrupts on CBUS_INTR_STATUS
        //
        cbusInt = SiiRegRead(REG_CBUS_INT_0);   // New :   CBUS_INT_0

        // When I2C is inoperational (D3) and a previous interrupt brought us here, do nothing.
        if (cbusInt == 0xFF)
        {
                return;
        }

        if (cbusInt)
        {
                lastCBusInt0Status = cbusInt; // save for SiiMhlTxDrvMscMsgNacked()
                //
                // Clear all interrupts that were raised even if we did not process
                //
                SiiRegWrite(REG_CBUS_INT_0, cbusInt);

                CBUS_DEBUG_PRINT(("Drv: Clear CBUS INTR_0: %02X\n", (int) cbusInt));

                // Downstream HPD handline is high priority
                if (BIT_CBUS_HPD_RCVD & cbusInt)
                {
                        uint8_t cbusStatus;
                        //
                        // Check if a SET_HPD came from the downstream device.
                        //
                        cbusStatus = SiiRegRead(REG_CBUS_STATUS);

                        if (cbusStatus & BIT_CBUS_HPD)
                        {
                                ERROR_DEBUG_PRINT(("Got SET_HPD (%02X)\n", (int) cbusStatus));
                        }
                        else
                        {
                                ERROR_DEBUG_PRINT(("Got CLR_HPD (%02X)\n", (int) cbusStatus));
                        }
                        // CBUS_HPD status bit
                        if(BIT_CBUS_HPD & (dsHpdStatus ^ cbusStatus))
                        {
                                SiiMhlTxDrvProcessDownStreamHPDChange(cbusStatus);
                        }
                        else
                        {
                                ERROR_DEBUG_PRINT(("Drv: Missed HPD change cbusStatus: %02X\n", (int) cbusStatus));
                                // must have missed an interrupt, so simulate the missing one
                                if (BIT_CBUS_HPD & cbusStatus)
                                {
                                        // only do this when CLR_HPD was missed
                                        SiiMhlTxDrvProcessDownStreamHPDChange(cbusStatus^BIT_CBUS_HPD);
                                        SiiMhlTxDrvProcessDownStreamHPDChange(cbusStatus);
                                }
                        }
                }

                // MSC_REQ_DONE received.
                if (BIT_CBUS_MSC_MT_DONE & cbusInt)
                {
                        CBUS_DEBUG_PRINT(("Drv: MSC_REQ_DONE\n"));

                        mscCmdInProgress = false;

                        // only do this after cBusInt interrupts are cleared above
                        SiiMhlTxMscCommandDone( SiiRegRead(REG_CBUS_PRI_RD_DATA_1ST) );    // New :  MSC_MT_RCVD_DATA0
                }

                if (BIT_CBUS_MSC_MR_WRITE_STAT & cbusInt)   // New :   CBUS_INT_0
                {
                        uint8_t status[4];
                        //uint8_t clear[4]={0xff,0xff,0xff,0xff};// must write 0xFF to clear regardless!
                        //re-visit

                        // don't put debug output here, it just creates confusion.
                        SiiRegReadBlock(REG_CBUS_WRITE_STAT_0, status, 4);   // New :   MHL_STAT_*

                        //SiiRegWriteBlock(REG_CBUS_WRITE_STAT_0, clear, 4);   // New :   MHL_STAT_*
                        SiiMhlTxGotMhlStatus( status[0], status[1] );
                }
                // MSC_MSG (RCP/RAP)
                if ((BIT_CBUS_MSC_MR_MSC_MSG & cbusInt))
                {
                        uint8_t mscMsg[2];
                        TXD_DEBUG_PRINT(("Drv: MSC_MSG Received\n"));
                        //
                        // Two bytes arrive at registers formerly mapped at 0x18 and 0x19
                        //
                        mscMsg[0] = SiiRegRead(REG_CBUS_MSC_MR_MSC_MSG_RCVD_1ST_DATA);  // New :   MSC_MR_MSC_MSG_RCVD_1ST_DATA
                        mscMsg[1] = SiiRegRead(REG_CBUS_MSC_MR_MSC_MSG_RCVD_2ND_DATA);  // New :   MSC_MR_MSC_MSG_RCVD_2ND_DATA

                        TXD_DEBUG_PRINT(("Drv: MSC MSG: %02X %02X\n", (int)mscMsg[0], (int)mscMsg[1] ));
                        SiiMhlTxGotMhlMscMsg( mscMsg[0], mscMsg[1] );
                }

                if ( BIT_CBUS_MSC_MR_WRITE_BURST & cbusInt)
                {
                        // WRITE_BURST complete
                        SiiMhlTxMscWriteBurstDone( cbusInt );
                }

                if(BIT_CBUS_MSC_MR_SET_INT & cbusInt)
                {
                        uint8_t intr[4];

                        TXD_DEBUG_PRINT(("Drv: MHL INTR Received\n"));
                        SiiRegReadBlock(REG_CBUS_SET_INT_0, intr, 4);
                        SiiRegWriteBlock(REG_CBUS_SET_INT_0, intr, 4);

                        // We are interested only in first two bytes. and the scratch pad index
                        SiiMhlTxGotMhlIntr( intr[0], intr[1] );
                }
        }


        cbusInt =SiiRegRead(REG_CBUS_INT_1);
        if(cbusInt)
        {
                //
                // Clear all interrupts that were raised even if we did not process
                //
                SiiRegWrite(REG_CBUS_INT_1, cbusInt);   // New :   CBUS_INT_0

                CBUS_DEBUG_PRINT(("Drv: Clear CBUS INTR_1: %02X\n", (int) cbusInt));
                CBusProcessErrors(cbusInt);
        }
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvPowBitChange
//
// Alert the driver that the peer's POW bit has changed so that it can take
// action if necessary.
//
void SiiMhlTxDrvPowBitChange (bool_t enable)
{
        // MHL peer device has it's own power
        if (enable)
        {
                // this does not exist in 8240 SiiRegModify(REG_DISC_CTRL8, 0x04, 0x04);
                TXD_DEBUG_PRINT(("Drv: POW bit 0->1, set DISC_CTRL8[2] = 1\n"));
        }
}
/*
	SiMhlTxDrvSetClkMode
	-- Set the hardware this this clock mode.
 */
void SiMhlTxDrvSetClkMode(uint8_t clkMode)
{
        TXD_DEBUG_PRINT(("SiMhlTxDrvSetClkMode:0x%02x\n",(int)clkMode));

        clkMode = clkMode;
        // nothing to do here since we only suport MHL_STATUS_CLK_MODE_NORMAL
        // if we supported SUPP_PPIXEL, this would be the place to write the register
}
/*
    SiiMhlTxDrvGetDeviceId
    returns chip Id
 */

uint16_t SiiMhlTxDrvGetDeviceId(void)
{
        uint16_t retVal;
        retVal =  SiiRegRead(REG_DEV_IDH);
        retVal <<= 8;
        retVal |= SiiRegRead(REG_DEV_IDL);
        return retVal;
}
/*
    SiiMhlTxDrvGetDeviceRev
    returns chip revision
 */
uint8_t SiiMhlTxDrvGetDeviceRev(void)
{
        return SiiRegRead(REG_DEV_REV);
}


/*
    SiiMhlTxDrvPreInitSetModeFlags
*/
void SiiMhlTxDrvPreInitSetModeFlags(uint8_t flags)
{
        g_modeFlags = flags;
}

#if	1
void	DumpEdidFifoImpl( int blockNumber, int override )
{
        uint8_t j;
        CachedRegModify(REG_EDID_CTRL
                        ,BIT_EDID_CTRL_FIFO_SELECT_MASK
                        ,BIT_EDID_CTRL_FIFO_SELECT_EDID
                       );


        SiiOsDebugPrintAlwaysShort("\nShowing the 8240 EDID SRAM blockNumber = %02X\n", (int)blockNumber);
        SiiRegWrite (REG_EDID_FIFO_ADDR, (blockNumber << 7));				// 2E9 Starting address of FIFO

        for (j = 0; j < 128; j++)
        {
                if ((j & 0x0F) == 0x00)
                {
                        SiiOsDebugPrintAlwaysShort("\n");
                }
                SiiOsDebugPrintAlwaysShort("%02X ", (int) SiiRegRead (REG_EDID_FIFO_RD_DATA) );
        }
        SiiOsDebugPrintAlwaysShort("\n");
}
#endif //)

#if 0
void	DumpEdidFifoImpl( int blockNumber )
{
        //if (SiiOsDebugChannelIsEnabled(SII_OSAL_DEBUG_EDID_DBG))
        {
                uint8_t j;
                uint8_t temp;
                temp = SiiRegRead(REG_EDID_CTRL);
                /*
                SiiRegWrite (REG_EDID_CTRL,							// 2E3 = 11
                BIT_EDID_CTRL_EDID_FIFO_ADDR_AUTO_ENABLE
                | BIT_EDID_CTRL_EDID_MODE_EN_ENABLE
                );*/
                CachedRegModify(REG_EDID_CTRL
                                ,BIT_EDID_CTRL_FIFO_SELECT_MASK
                                ,BIT_EDID_CTRL_FIFO_SELECT_EDID
                               );


                SiiOsDebugPrintAlwaysShort("\nShowing the 8240 EDID SRAM blockNumber = %02X\n", (int)blockNumber);
                SiiRegWrite (REG_EDID_FIFO_ADDR, (blockNumber << 7));				// 2E9 Starting address of FIFO

                for (j = 0; j < 128; j++)
                {
                        if ((j & 0x0F) == 0x00)
                        {
                                SiiOsDebugPrintAlwaysShort("\n");
                        }
                        SiiOsDebugPrintAlwaysShort("%02X ", (int) SiiRegRead (REG_EDID_FIFO_RD_DATA) );
                }
                SiiOsDebugPrintAlwaysShort("\n");
                SiiRegWrite(REG_EDID_CTRL,temp);
        }
}
#endif //)

#ifdef EDID_DUMP_SW_BUFFER //(

void DumpEdidBlockImpl(char *pszFile, int iLineNum,uint8_t *pData,uint16_t length)
{
        //if (SiiOsDebugChannelIsEnabled(SII_OSAL_DEBUG_EDID_DBG))
        {
                uint16_t i;
                SiiOsDebugPrintAlwaysShort("EDID DATA:\n");
                for (i = 0; i < length; )
                {
                        uint16_t j;
                        uint16_t temp = i;
                        for(j=0; (j < 16)&& (i<length); ++j,++i)
                        {
                                SiiOsDebugPrintAlwaysShort("%02X ", (int) pData[i]);
                        }
                        SiiOsDebugPrintAlwaysShort(" | ");
                        for(j=0; (j < 16)&& (temp<length); ++j,++temp)
                        {
                                SiiOsDebugPrintAlwaysShort("%c",((pData[temp]>=' ')&&(pData[temp]<='z'))?pData[temp]:'.');
                        }
                        SiiOsDebugPrintAlwaysShort("\n");
                }
        }
}
#endif //)

#ifdef EDID_CLEAR_HW_BUFFER //(

void ClearEdidBlockImpl(uint8_t *pData)
{
        uint8_t j;

        for (j = 0; j < EDID_BLOCK_SIZE; j++)
        {
                pData[j] = j;
        }

#ifdef	EDID_READ_FIFO_BYTE_MODE //(
        {
                int	i;

                EDID_DEBUG_PRINT(("ClearEdidBlockImpl: Copy 128 bytes - byte mode\n"));
                for(i = 0; i < EDID_BLOCK_SIZE; i++)
                {
                        SiiRegWrite (REG_EDID_FIFO_WR_DATA, pData[i]);
                }
        }
#else //)(
        SiiRegWriteBlock ( REG_EDID_FIFO_WR_DATA, pData, EDID_BLOCK_SIZE );
#endif //)
}
#endif //)



#ifndef INTERRUPT_DRIVEN_EDID //(
#ifndef HW_ASSISTED_EDID_READ //(
#define T_DDC_ACCESS    50
#define EDID_BLOCK_0_OFFSET 0x0000
//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  GetDDC_Access(void)
//
// PURPOSE       :  Request access to DDC bus from the receiver
//
// INPUT PARAMS  :  None
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  true if bus obtained successfully. false if failed.
//
//////////////////////////////////////////////////////////////////////////////

static bool_t GetDDC_Access ( void )
{
        uint8_t sysCtrl;
        uint8_t DDCReqTimeout = T_DDC_ACCESS;
        uint8_t TPI_ControlImage;

        TXD_DEBUG_PRINT(("GetDDC_Access\n"));

        // Read and store original value. Will be passed into ReleaseDDC()
        sysCtrl = cache_TPI_SYSTEM_CONTROL_DATA_REG;

        sysCtrl |= DDC_BUS_REQUEST_REQUESTED;
        WriteTpiSystemControlDataReg( sysCtrl );

        while (DDCReqTimeout--)											// Loop till 0x1A[1] reads "1"
        {
                TPI_ControlImage = ReadTpiSystemControlDataReg();

                if (TPI_ControlImage & DDC_BUS_GRANT_MASK)					// When 0x1A[1] reads "1"
                {
                        sysCtrl |= DDC_BUS_GRANT_GRANTED;
                        WriteTpiSystemControlDataReg(sysCtrl);		// lock host DDC bus access (0x1A[2:1] = 11)
                        return true;
                }
                WriteTpiSystemControlDataReg(sysCtrl);			// 0x1A[2] = "1" - Requst the DDC bus
                HalTimerWait(200);
        }

        WriteTpiSystemControlDataReg(sysCtrl);				// Failure... restore original value.
        return false;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  ReleaseDDC(void)
//
// PURPOSE       :  Release DDC bus
//
// INPUT PARAMS  :  None
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  true if bus released successfully. false if failed.
//
//////////////////////////////////////////////////////////////////////////////

static bool_t ReleaseDDC( void )
{
        uint8_t DDCReqTimeout = T_DDC_ACCESS;
        uint8_t TPI_ControlImage;

        TXD_DEBUG_PRINT(("ReleaseDDC\n"));

        // Just to be sure bits [2:1] are 0 before it is written
        cache_TPI_SYSTEM_CONTROL_DATA_REG &= ~(DDC_BUS_REQUEST_MASK| DDC_BUS_GRANT_MASK);

        while (DDCReqTimeout--)						// Loop till 0x1A[1] reads "0"
        {
                // Cannot use SiiRegModify() here. A read of TPI_SYSTEM_CONTROL_DATA_REG is invalid while DDC is granted.
                // Doing so will return 0xFF, and cause an invalid value to be written back.

                WriteTpiSystemControlDataReg( cache_TPI_SYSTEM_CONTROL_DATA_REG );
                TPI_ControlImage = ReadTpiSystemControlDataReg(0);

                if (!(TPI_ControlImage & (DDC_BUS_REQUEST_MASK| DDC_BUS_GRANT_MASK)))		// When 0x1A[2:1] read "0"
                {
                        return true;
                }
        }

        return false;								// Failed to release DDC bus control
}

#endif //)


SiiMhlTxDrvGetEdidBlockResult_e SiiMhlTxDrvGetEdidBlock(uint8_t *pBufEdid,uint8_t blockNumber,uint8_t blockSize)
{

        uint16_t	offset = (blockNumber << 7);

#ifdef HW_ASSISTED_EDID_READ //(
        uint16_t    counter=0;


        EDID_DEBUG_PRINT(("EDID HW Assist: Read EDID block number %02X\n", (int)blockNumber));

        SiiMhlTxDrvIssueEdidReadRequest(blockNumber);

        EDID_DEBUG_PRINT(("EDID HW Assist: Waiting for Completion\n" ));
        // Wait for completion
        for (counter =0; counter < TX_EDID_POLL_MAX; ++counter)
        {
                uint8_t temp = SiiRegRead(REG_INTR9);
                if (temp)
                {
                        if( BIT_INTR9_EDID_DONE & temp )	// 2E0[5]
                        {
                                // clear the interrupt
                                SiiRegWrite(REG_INTR9,BIT_INTR9_EDID_DONE);
                                break;
                        }
                        ERROR_DEBUG_PRINT(("intr9 status: %02x\n",(uint16_t)temp));
                        if (BIT_INTR9_EDID_ERROR & temp)
                        {
                                // clear the interrupt
                                SiiRegWrite(REG_INTR9,BIT_INTR9_EDID_ERROR);
                                edidFifoBlockNumber=0;
                                ERROR_DEBUG_PRINT(("EDID read error, retrying\n"));
                                DumpTPIDebugRegs

                                SiiMhlTxDrvIssueEdidReadRequest(blockNumber);
                        }
                }
                // wait a bit
                HalTimerWait(1);
        }
        if (counter >= TX_EDID_POLL_MAX)
        {
                ERROR_DEBUG_PRINT(("EDID HW Assist: Timed Out. counter:%d\n",counter));
                return gebTimedOut;
        }
        EDID_DEBUG_PRINT(("EDID HW Assist: Setting address for read from buffer to %02X counter:%d\n", (int)offset,(uint16_t)counter));

        SiiRegWrite (REG_EDID_FIFO_ADDR, offset);				// 2E9 = Starting address of FIFO

#ifdef	EDID_READ_FIFO_BYTE_MODE //(
        {
                int	i;

                // Copy data into the internal buffer for parsing as we did in the past
                for(i = 0; i < blockSize; i++)
                {
                        pBufEdid[ i ] = SiiRegRead (REG_EDID_FIFO_RD_DATA);	// from 2EC
                }
        }
#else //)( #ifdef	EDID_READ_FIFO_BYTE_MODE
        SiiRegReadBlock ( REG_EDID_FIFO_RD_DATA, pBufEdid, blockSize );
#endif //)	EDID_READ_FIFO_BYTE_MODE

        EDID_DEBUG_PRINT(("Done reading EDID from FIFO using HW_ASSIST.\n"));

        DumpEdidFifo(blockNumber,0);
        DumpEdidFifo(blockNumber+1,0);
        DumpEdidBlock(pBufEdid,blockSize)  // no semicolon needed here
        return gebSuccess;
#else //)(

        // Request access to DDC bus from the receiver
        if (GetDDC_Access())
        {
                SiiRegEdidReadBlock(TX_PAGE_DDC_SEGM | 0x0000, TX_PAGE_DDC_EDID | offset, pBufEdid, blockSize);

                if (!ReleaseDDC())				// Host must release DDC bus once it is done reading EDID
                {
                        ERROR_DEBUG_PRINT(("EDID -> DDC bus release failed\n"));
                        return gebReleaseFailed;
                }
        }
        else
        {
                ERROR_DEBUG_PRINT(("EDID -> DDC bus acquisition failed\n"));
                return gebAcquisitionFailed;
        }
        return gebSuccess;
#endif	//) HW_ASSISTED_EDID_READ
}
#else //)(

int SiiMhlTxDrvGetEdidFifoNextBlock(uint8_t *pBufEdid)
{
        uint8_t cbusStatus;
        uint8_t offset = EDID_BLOCK_SIZE * (edidFifoBlockNumber & 0x01);

        edidFifoBlockNumber++;

        /* 2E9 = Starting address of FIFO */
        SiiRegWrite (REG_EDID_FIFO_ADDR, offset);

        // choose EDID instead of devcap to appear at the FIFO
        CachedRegModify(REG_EDID_CTRL
                        ,BIT_EDID_CTRL_FIFO_SELECT_MASK
                        ,BIT_EDID_CTRL_FIFO_SELECT_EDID
                       );
        SiiRegReadBlock ( REG_EDID_FIFO_RD_DATA, pBufEdid, EDID_BLOCK_SIZE );

        DumpEdidBlock(pBufEdid,EDID_BLOCK_SIZE)  // no semicolon needed here
        DumpEdidBlock(pBufEdid,EDID_BLOCK_SIZE)  // no semicolon needed here
        // check to see if downstream HPD still asserted
        cbusStatus = SiiRegRead(REG_CBUS_STATUS);
        if (BIT_CBUS_HPD & cbusStatus)
        {
                EDID_DEBUG_PRINT(("Done reading EDID from FIFO using HW_ASSIST.\n"));
                return 1;
        }
        else
        {
                //HPD went away
                ERROR_DEBUG_PRINT(("DS HPD LOW during EDID read\n"));
                return 0;
        }
}
#endif //)

int SiiMhlTxDrvSetUpstreamEDID(uint8_t *pEDID,uint16_t length)
{
        uint8_t cbusStatus;
        // check to be sure that we still have downstream HPD
        cbusStatus = SiiRegRead(REG_CBUS_STATUS);
        if (BIT_CBUS_HPD & cbusStatus)
        {
                DumpEdidBlock(pEDID,length)  // no semicolon needed here
                SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow();
                CachedRegModify(REG_EDID_CTRL
                                ,(BIT_EDID_CTRL_FIFO_SELECT_MASK | BIT_EDID_CTRL_EDID_MODE_EN_MASK)
                                ,(BIT_EDID_CTRL_FIFO_SELECT_EDID | BIT_EDID_CTRL_EDID_MODE_EN_ENABLE)
                               );
#ifdef K3_HDMI
                k3_hdmi_enable_hpd(true);
                HalTimerWait(500);
#endif

                SiiRegWrite (REG_EDID_FIFO_ADDR, 0);				// 2E9 = 0		// Starting address of FIFO as 0

#ifdef	EDID_READ_FIFO_BYTE_MODE //(
                TXD_DEBUG_PRINT(("SiiMhlTxDrvSetUpstreamEDID: Copy %d bytes - byte mode\n",length));
                {
                        int	i;

                        for(i = 0; i < length; ++i)
                        {
                                SiiRegWrite (REG_EDID_FIFO_WR_DATA, pEDID[i]);
                        }
                }
#else //)(
                TXD_DEBUG_PRINT(("SiiMhlTxDrvSetUpstreamEDID: Copy %d bytes - block mode\n",length));
                SiiRegWriteBlock ( REG_EDID_FIFO_WR_DATA, pEDID, length );
#endif //)
                TXD_DEBUG_PRINT(("%d bytes written to 8240 upstream EDID SRAM\n",length));

                DumpEdidFifo( 0,0 ) // no semicolon needed here
                DumpEdidFifo( 1,0 ) // no semicolon needed here

                CachedRegModify(REG_EDID_CTRL
                                , BIT_EDID_CTRL_EDID_PRIME_VALID_MASK
                                , BIT_EDID_CTRL_EDID_PRIME_VALID_ENABLE
                               );

                EDID_DEBUG_PRINT(("Open EDID access to the upstream device. 2E3 = %02X\n", (int)CachedRegRead(REG_EDID_CTRL) ));
                // power up the receiver
                SiiRegModify(REG_DPD,BIT_DPD_PDIDCK_MASK,BIT_DPD_PDIDCK_NORMAL_OPERATION);

                // make sure that incoming FIFOs do not overflow prior to enabling TMDS output
                SiiRegWrite(REG_PKT_FILTER_0, 0xFF);
                SiiRegWrite(REG_PKT_FILTER_1, 0xFF);

                // toggle to make sure that the EDID gets read by the upstream device.
                SiiMhlTxDrvAcquireUpstreamHPDControlDriveHigh();
                return 0;
        }
        return -1;
}


#ifdef MDT_STATE_MACHINE //(
#include "queue.h"

typedef enum
{
        mdteNoOp
        ,mdteLegacyModeRequest
        ,mdteLegacyModeGrant
        ,mdteMdtRequest
        ,mdteMdtQueueExhausted
        ,mdteREQ_WRT_FromPeer
        ,mdteREQ_WRT
        ,mdteGRT_WRT_FromPeer
        ,mdteGRT_WRT
        ,mdteLegacyRequest
        ,mdteLegacyCommandDone
} MdtEvent_e;

typedef enum
{
        mdtsMdtModeIdle
        ,mdtsMdtModeIdleLegacyRequestPending
        ,mdtsMdtModeActive
        ,mdtsMdtModeActiveLegacyRequestPending
        ,mdtsLegacyModeIdle
        ,mdtsLegacyModeActive
        ,mdtsLegacyModeWriteBurstPending
        ,mdtsLegacyModeWriteBurstActive

        ,mdtsNumStates  // this entry must be last

} MdtState_e;
typedef struct _MdtStateTableEntry_t
{
        MdtEvent_e        event;
        MdtState_e        newState;
        void (*pfnTransitionHandler)(void);  // note that the size of this structure, on an 8051, should be exactly 4 bytes.
} MdtStateTableEntry_t,*PMdtStateTableEntry_t;

typedef struct _MdtStateTableRowHeader_t
{
        uint8_t numValidEvents;
        void (*pfnEventGatherer)(void);  // note that the size of this structure, on an 8051, should be exactly 4 bytes.
        PMdtStateTableEntry_t  pStateRow;
} MdtStateTableRowHeader_t,*PMdtStateTableRowHeader_t;

#define NUM_TX_EVENT_QUEUE_EVENTS 8
typedef struct _MdtEventQueue_t
{
        QueueHeader_t header;
        MdtEvent_e queue[NUM_TX_EVENT_QUEUE_EVENTS];
} MdtEventQueue_t,*PMdtEventQueue_t;

MdtEventQueue_t MdtEventQueue= {0,0,{0}};

MdtEvent_e GetNextMdtEvent()
{
        if (0==QUEUE_DEPTH(MdtEventQueue))
        {
                return mdteNoOp;
        }
        else
        {
                MdtEvent_e retVal;
                retVal = MdtEventQueue.queue[MdtEventQueue.header.head];
                ADVANCE_QUEUE_HEAD(MdtEventQueue)
                return retVal;
        }
}



bool_t PutNextMdtEventImpl(MdtEvent_e event)
{
        if (QUEUE_FULL(MdtEventQueue))
        {
                //queue is full
                return false;
        }
        // at least one slot available
        MdtEventQueue.queue[MdtEventQueue.header.tail] = event;
        ADVANCE_QUEUE_TAIL(MdtEventQueue)
        return true;
}
// use this wrapper to do debugging output for the routine above.
bool_t PutNextMdtEventWrapper(MdtEvent_e event,char *pszEvent,int iLine)
{
        bool_t retVal;

        TXD_DEBUG_PRINT(("PutNextMdtEventWrapper: called from line:%d event: %s(%d) depth:%d head: %d tail:%d\n"
                         ,iLine
                         ,pszEvent
                         ,(int)event
                         ,(int)QUEUE_DEPTH(MdtEventQueue)
                         ,(int)MdtEventQueue.header.head
                         ,(int)MdtEventQueue.header.tail
                        ));
        retVal = PutNextMdtEventImpl(event);

        if (!retVal)
        {
                TXD_DEBUG_PRINT(("queue full, when adding event %d called from line:%d\n",(int)event,iLine));
        }
        return retVal;
}
#define PutNextTxEvent(event) PutNextTxEventWrapper(event,#event,__LINE__);


void MdtModeIdleToMdtModeActiveViaMdtRequest(void)
{
}

void MdtModeIdleToMdtModeIdleLegacyRequestPendingViaLegacyModeRequest(void)
{
}

void MdtModeIdleLegacyRequestPendingToLegacyModeIdleViaLegacyModeRequest(void)
{
}

void MdtModeIdleLegacyRequestPendingToLegacyModeIdleViaLegacyModeGrant(void)
{
}

void MdtModeActiveToMdtModeIdleViaMdtQueueExhausted(void)
{
}

void MdtModeActiveToMdtModeActiveLegacyRequestPendingViaLegacyModeRequest(void)
{
}

void MdtModeActiveLegacyRequestPendingToLegacyModeIdleViaLegacyModeGrant(void)
{
}

void mdteMdtModeRequest(void)
{
}

void LegacyModeIdleToMdtModeIdleViaMdtModeRequest(void)
{
}

void LegacyModeIdleToLegacyModeWriteBurstPendingViaREQ_WRT_FromPeer(void)
{
}

void LegacyModeIdleToLegacyModeActiveViaLegacyRequest(void)
{
}

void LegacyModeActiveToLegacyModeIdleViaLegacyCommandDone(void)
{
}

void mdtsLegacyWriteBurstActive(void)
{
}

void LegacyModeWriteBurstPendingToLegacyWriteBurstActiveViaGRT_WRT(void)
{
}

void LegacyModeWriteBurstActiveToLegacyModeIdleViaLegacyCommandDone(void)
{
}

void GathererMdtModeIdle(void)
{
}

void GathererMdtModeIdleLegacyRequestPending(void)
{
}

void GathererMdtModeActive(void)
{
}

void GathererMdtModeActiveLegacyRequestPending(void)
{
}

void GathererLegacyModeIdle(void)
{
}

void GathererLegacyModeActive(void)
{
}

void GathererLegacyModeWriteBurstPending(void)
{
}

void GathererLegacyModeWriteBurstActive(void)
{
}


#define StateTransitionEntry(state,event,newState) {mdte##event,mdts##newState,state##To##newState##Via##event}
PLACE_IN_CODE_SEG MdtStateTableEntry_t  mdtste_MdtModeIdle[] =
{
        StateTransitionEntry(MdtModeIdle,MdtRequest,MdtModeActive)
        ,StateTransitionEntry(MdtModeIdle,LegacyModeRequest,MdtModeIdleLegacyRequestPending)
};

PLACE_IN_CODE_SEG MdtStateTableEntry_t mdtste_MdtModeIdleLegacyRequestPending[] =
{
        StateTransitionEntry(MdtModeIdleLegacyRequestPending,LegacyModeGrant,LegacyModeIdle)
};

PLACE_IN_CODE_SEG MdtStateTableEntry_t mdtste_MdtModeActive[] =
{
        StateTransitionEntry(MdtModeActive,MdtQueueExhausted,MdtModeIdle)
        ,StateTransitionEntry(MdtModeActive,LegacyModeRequest,MdtModeActiveLegacyRequestPending)
};

PLACE_IN_CODE_SEG MdtStateTableEntry_t mdtste_MdtModeActiveLegacyRequestPending[] =
{
        StateTransitionEntry(MdtModeActiveLegacyRequestPending,LegacyModeGrant,LegacyModeIdle)
};

PLACE_IN_CODE_SEG MdtStateTableEntry_t mdtste_LegacyModeIdle[] =
{
        StateTransitionEntry(LegacyModeIdle,MdtModeRequest,MdtModeIdle)
        ,StateTransitionEntry(LegacyModeIdle,REQ_WRT_FromPeer,LegacyModeWriteBurstPending)
        ,StateTransitionEntry(LegacyModeIdle,LegacyRequest,LegacyModeActive)
};

PLACE_IN_CODE_SEG MdtStateTableEntry_t mdtste_LegacyModeActive[] =
{
        StateTransitionEntry(LegacyModeActive,LegacyCommandDone,LegacyModeIdle)
};

PLACE_IN_CODE_SEG MdtStateTableEntry_t mdtste_LegacyModeWriteBurstPending[] =
{
        StateTransitionEntry(LegacyModeWriteBurstPending,GRT_WRT,LegacyWriteBurstActive)
};

PLACE_IN_CODE_SEG MdtStateTableEntry_t mdtste_LegacyModeWriteBurstActive[] =
{
        StateTransitionEntry(LegacyModeWriteBurstActive,LegacyCommandDone,LegacyModeIdle)
};

// The order of the following table is CRITICAL.
//  Do NOT insert anything without a corresponding insertion into MdtState_e
//   See statetable.h for the definition of MdtState_e
PLACE_IN_CODE_SEG MdtStateTableRowHeader_t MdtStateTransitionAndResponseTable[mdtsNumStates]=
{
#define MDT_RowEntry(state)  {sizeof(mdtste_##state)/sizeof(mdtste_##state[0]), Gatherer##state, mdtste_##state }
        MDT_RowEntry(MdtModeIdle)
        , MDT_RowEntry(MdtModeIdleLegacyRequestPending)
        , MDT_RowEntry(MdtModeActive)
        , MDT_RowEntry(MdtModeActiveLegacyRequestPending)
        , MDT_RowEntry(LegacyModeIdle)
        , MDT_RowEntry(LegacyModeActive)
        , MDT_RowEntry(LegacyModeWriteBurstPending)
        , MDT_RowEntry(LegacyModeWriteBurstActive)
};

PMdtStateTableEntry_t LookupNextState(MdtState_e state, uint8_t event)
{
        PMdtStateTableEntry_t pMdtStateTableEntry = MdtStateTransitionAndResponseTable[state].pStateRow;
        uint8_t i,limit;
        limit = MdtStateTransitionAndResponseTable[state].numValidEvents;
        for (i=0; i < limit; ++i)
        {
                if (event == pMdtStateTableEntry->event)
                {
                        TXD_DEBUG_PRINT(("lookup state:%d event:%d\n",(int)state,(int)event));

                        return pMdtStateTableEntry;
                }
                pMdtStateTableEntry++;
        }
        TXD_DEBUG_PRINT(("lookup failed. event:%d\n",(int)event));
        return NULL;
}
#endif //)

#ifdef Disconnected_USB_ID_with_CBUS_ID_As_defult
void SiiCBusIDSwitcherOpen( void )
{
        char mhl_chip_name[15];
        bool ret =0;

        ret = get_hw_config_string("MHL/chip_name",mhl_chip_name,15,NULL);
        if(ret)
        {
                if(strstr(mhl_chip_name,"none"))
                {
                        printk("sii8240 doesn't support on this production");
                        return;
                }
        }
        //else
        //{
        //      if not set mhl config,default this chip is support on this production
        //}

        UNMASK_INTR_4_INTERRUPTS;
        ENABLE_DISCOVERY;
        ReleaseUsbIdSwitchOpen();
        CLR_BIT(REG_DPD, 0);
        fwPowerState = POWER_STATE_D3;
}
/* wakeup the mhl chip when plug out OTG device.  */
int SiiMhlReset(void)
{
        char mhl_chip_name[15];
        bool ret =0;

        ret = get_hw_config_string("MHL/chip_name",mhl_chip_name,15,NULL);
        if(ret)
        {
                if(strstr(mhl_chip_name,"none"))
                {
                        printk("sii8240 doesn't support on this production");
                        return 0;
                }
        }
        //else
        //{
        //      if not set mhl config,default this chip is support on this production
        //}

        SiiMhlTxHwReset(TX_HW_RESET_PERIOD,TX_HW_RESET_DELAY);
        return 0;
}
#endif


