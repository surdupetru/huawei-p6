/*********************************************************************************/
/*  Copyright (c) 2002-2011, Silicon Image, Inc.  All rights reserved.           */
/*  No part of this work may be reproduced, modified, distributed, transmitted,  */
/*  transcribed, or translated into any language or computer format, in any form */
/*  or by any means without written permission of: Silicon Image, Inc.,          */
/*  1140 East Arques Avenue, Sunnyvale, California 94085                         */
/*********************************************************************************/

#include "si_memsegsupport.h"
#include "si_common.h"
#ifdef __KERNEL__
#include "sii_hal.h"
#else
#include "hal_timers.h"
#endif

//#include "si_platform.h"
#include "si_cra.h"
#include "si_cra_cfg.h"
#include "si_8240_regs.h"
#include "si_tpi_regs.h"
#include "si_mhl_defs.h"
#include "si_mhl_tx_base_drv_api.h"
#include "si_hdmi_tx_lite_api.h"
#include "si_app_devcap.h"
#include "si_bitdefs.h"
#include "si_edid.h"
#include "si_drv_hdmi_tx_lite_edid.h"
extern unsigned char VIDEO_CAPABILITY_D_BLOCK_found;

static void SiiHdmiTxLiteDrvPrepareHwAviInfoFrame( PHwAviPayLoad_t pPayLoad );
#ifdef ENABLE_INFO_DEBUG_PRINT //(

#define DumpAviInfoFrame(pPayLoad,pszLabel) DumpAVIInfoFrameImpl(pPayLoad,pszLabel, 13); //DumpAviInfoFrameImpl(__FILE__,__LINE__,pPayLoad,pszLabel);

void DumpAVIInfoFrameImpl(PHwAviPayLoad_t pPayLoad,char *pszLabel, uint8_t length)
{
        TXD_DEBUG_PRINT(("MhlTx:pInfoFrame length:0x%02x -%s----AVI info frame: \n",length,pszLabel));
        TXD_DEBUG_PRINT((
                                "%02x %02x %02x %02x %02x %02x %02x "
                                "%02x %02x %02x %02x %02x %02x %02x\n"
                                ,(uint16_t)pPayLoad->ifData[0x0]
                                ,(uint16_t)pPayLoad->ifData[0x1]
                                ,(uint16_t)pPayLoad->ifData[0x2]
                                ,(uint16_t)pPayLoad->ifData[0x3]
                                ,(uint16_t)pPayLoad->ifData[0x4]
                                ,(uint16_t)pPayLoad->ifData[0x5]
                                ,(uint16_t)pPayLoad->ifData[0x6]
                                ,(uint16_t)pPayLoad->ifData[0x7]
                                ,(uint16_t)pPayLoad->ifData[0x8]
                                ,(uint16_t)pPayLoad->ifData[0x9]
                                ,(uint16_t)pPayLoad->ifData[0xA]
                                ,(uint16_t)pPayLoad->ifData[0xB]
                                ,(uint16_t)pPayLoad->ifData[0xC]
                                ,(uint16_t)pPayLoad->ifData[0xD]
                        ));
}

#define AT_ROW_END(i,length) (i & (length-1)) == (length-1)

void DumpIncomingInfoFrameImpl(PAVIInfoFrame_t pInfoFrame,char *pszLabel, uint8_t length)
{
        uint8_t j;
        uint8_t *pData = (uint8_t *)pInfoFrame;
        TXD_DEBUG_PRINT(("MhlTx:pInfoFrame length:0x%02x -%s----\n",length,pszLabel));

        for (j = 0; j < length; j++)
        {
                TXD_DEBUG_PRINT(("pData[%d]=%02X;", j,(uint8_t)pData[j]));
                //if (AT_ROW_END(j,32))
                {
                        TXD_DEBUG_PRINT(("\n"));
                }
        }
        TXD_DEBUG_PRINT(("\n"));

}

#else //)(

#define DumpAviInfoFrame(pPayLoad,pszLabel) /*nothing*/

#endif //)

uint8_t CalculateSum(uint8_t *info,uint8_t sum,uint8_t length)
{
        uint8_t i;

        INFO_DEBUG_PRINT(("sum: %02x length:%x\n",(uint16_t)sum,(uint16_t)length));
        for (i = 0; i < length; i++)
        {
                sum += info[i];
        }
        INFO_DEBUG_PRINT(("sum: %02x\n",(uint16_t)sum));
        return sum;
}

uint8_t CalculateGenericCheckSum(uint8_t *infoFrameData,uint8_t checkSum,uint8_t length)
{
        checkSum = 0x100 - CalculateSum(infoFrameData,checkSum,length);

        INFO_DEBUG_PRINT(("checkSum: %02x\n",(uint16_t)checkSum));
        return checkSum;
}

#define SIZE_AUDIO_INFOFRAME 14
//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  SendAudioInfoFrame()
//
// PURPOSE       :  Load Audio InfoFrame data into registers and send to sink
//
// INPUT PARAMS  :  (1) Channel count (2) speaker configuration per CEA-861D
//                  Tables 19, 20 (3) Coding type: 0x09 for DSD Audio. 0 (refer
//                                      to stream header) for all the rest (4) Sample Frequency. Non
//                                      zero for HBR only (5) Audio Sample Length. Non zero for HBR
//                                      only.
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  true
//
//////////////////////////////////////////////////////////////////////////////
void SendAudioInfoFrame (void)
{
        if (sink_type_HDMI == SiiMhlTxGetSinkType())
        {
                uint8_t ifData[SIZE_AUDIO_INFOFRAME];
                uint8_t i;


                ifData[0] = 0x84;
                ifData[1] = 0x01;
                ifData[2] = 0x0A;
                for (i = 3; i < SIZE_AUDIO_INFOFRAME; ++i)
                {
                        ifData[i] = 0;
                }

                ifData[3] = CalculateGenericCheckSum(ifData,0,SIZE_AUDIO_INFOFRAME);


                SiiRegWrite(REG_TPI_INFO_FSEL
                            , BIT_TPI_INFO_EN
                            | BIT_TPI_INFO_RPT
                            | BIT_TPI_INFO_READ_FLAG_NO_READ
                            | BIT_TPI_INFO_SEL_Audio
                           );
                SiiRegWriteBlock(REG_TPI_INFO_BYTE00,ifData,SIZE_AUDIO_INFOFRAME);

                INFO_DEBUG_PRINT(("REG_TPI_INFO_BYTE13: %02x\n",REG_TPI_INFO_BYTE13));
        }
}
#define SIZE_AVI_INFOFRAME				14
static uint8_t CalculateAviInfoFrameChecksum (PHwAviPayLoad_t pPayLoad)
{
        uint8_t checksum;

        checksum = 0x82 + 0x02 + 0x0D;  // these are set by the hardware
        return CalculateGenericCheckSum(pPayLoad->ifData,checksum,SIZE_AVI_INFOFRAME);
}

HwAviPayLoad_t aviPayLoad;
void SiiHmdiTxLiteDrvSendHwAviInfoFrame( void )
{
        /*
        extern void *memcpy(void *dest, const void *src, uint8_t n);
        extern void *memset  (void *s, char val, int n);
        uint8_t Real_aviPayLoad[17];*/
        INFO_DEBUG_PRINT(("Output Mode: %s\n",(sink_type_HDMI == SiiMhlTxGetSinkType())?"HDMI":"DVI"));
        if (sink_type_HDMI == SiiMhlTxGetSinkType())
        {
                SiiRegWrite(REG_TPI_INFO_FSEL
                            , BIT_TPI_INFO_EN
                            | BIT_TPI_INFO_RPT
                            | BIT_TPI_INFO_READ_FLAG_NO_READ
                            | BIT_TPI_INFO_SEL_AVI
                           );
                /*
                memset(&Real_aviPayLoad[0], 0x00, 17);
                memcpy(&Real_aviPayLoad[3],aviPayLoad.ifData,sizeof(aviPayLoad.ifData));
                Real_aviPayLoad[0]=0x82;
                Real_aviPayLoad[1]=0x02;
                Real_aviPayLoad[2]=0x0D;

                SiiRegWriteBlock(REG_TPI_AVI_CHSUM, (uint8_t*)&Real_aviPayLoad, 17);
                DumpAviInfoFrame(&Real_aviPayLoad[0],"outgoing")  // no semicolon needed
                */
                SiiRegWriteBlock(REG_TPI_AVI_CHSUM, (uint8_t *)&aviPayLoad.ifData, sizeof(aviPayLoad.ifData));
                DumpAviInfoFrame(&aviPayLoad,"outgoing")  // no semicolon needed
        }
}

#ifndef PASS_980TEST
static void SiiHdmiTxLiteDrvPrepareHwAviInfoFrame( PHwAviPayLoad_t pPayLoad )
{
        uint8_t                 outputClrSpc;
        AviInfoFrameDataByte2_t colorimetryAspectRatio;
        AviInfoFrameDataByte4_t inputVideoCode;
        QuantizationSettings_e  settings;

        DumpAviInfoFrame(pPayLoad,"preparing")  // no semicolon needed
        outputClrSpc            = SiiMhlTxDrvGetOutputColorSpace();  // originates from EDID

        colorimetryAspectRatio  = SiiMhlTxDrvGetColorimetryAspectRatio();
        inputVideoCode          = SiiMhlTxDrvGetInputVideoCode();

        pPayLoad->namedIfData.checksum = 0; // the checksum itself is included in the calculation.

        pPayLoad->namedIfData.ifData_u.bitFields.pb1.colorSpace = outputClrSpc;
        pPayLoad->namedIfData.ifData_u.bitFields.colorimetryAspectRatio = colorimetryAspectRatio;
        pPayLoad->namedIfData.ifData_u.bitFields.VIC = inputVideoCode;

        settings = qsAutoSelectByColorSpace;
        INFO_DEBUG_PRINT(("%s %02x\n",(EDID_Data.VideoCapabilityFlags.QS)?"VCF":"vcf",(uint16_t)*((uint8_t *)&EDID_Data.VideoCapabilityFlags)));
        if (EDID_Data.VideoCapabilityFlags.QS)
        {
                switch(outputClrSpc)
                {
                case acsRGB:
                        INFO_DEBUG_PRINT(("RGB\n"));
                        break;
                case acsYCbCr422:
                        INFO_DEBUG_PRINT(("422\n"));
                case acsYCbCr444:
                        switch (pPayLoad->namedIfData.ifData_u.bitFields.pb5.quantization)
                        {
                        case aqLimitedRange:
                                INFO_DEBUG_PRINT(("444-limited\n"));
                                settings = qsLimitedRange;
                                break;
                        case aqFullRange:
                                INFO_DEBUG_PRINT(("444-full\n"));
                                settings = qsFullRange;
                                break;
                        case aqReserved0:
                        case aqReserved1:
                                INFO_DEBUG_PRINT(("444-reserved\n"));
                                // undefined by CEA-861-E
                                break;
                        }
                        break;
                }
        }
        SiiMhlTxDrvSetOutputQuantizationRange(settings);
        SiiMhlTxDrvSetInputQuantizationRange(settings);

        pPayLoad->namedIfData.checksum = CalculateAviInfoFrameChecksum(pPayLoad);
        INFO_DEBUG_PRINT(("Drv: outputClrSpc: %02x\n",(uint16_t)outputClrSpc));

        DumpIncomingInfoFrame(pPayLoad,sizeof(*pPayLoad));


        DumpAviInfoFrame(pPayLoad,"preparing")  // no semicolon needed
        aviPayLoad = *pPayLoad;
}

void SiiHmdiTxLiteDrvPrepareAviInfoframe ( void )
{
#ifdef AVI_PASSTHROUGH //(
        INFO_DEBUG_PRINT(("PrepareAviInfoframe\n"));
        SiiHdmiTxLiteDrvSendHwAviInfoFrame(&aviPayLoad);

#else //)(
        HwAviPayLoad_t hwIfData;
        INFO_DEBUG_PRINT(("PrepareAviInfoframe\n"));

        hwIfData.namedIfData.ifData_u.bitFields.pb1.colorSpace = SiiMhlTxDrvGetOutputColorSpace(); // originates from incoming infoframe
        hwIfData.namedIfData.ifData_u.bitFields.colorimetryAspectRatio = SiiMhlTxDrvGetColorimetryAspectRatio();
        hwIfData.ifData[0x3] = 0x00;
        hwIfData.namedIfData.ifData_u.bitFields.VIC = SiiMhlTxDrvGetInputVideoCode();
        hwIfData.namedIfData.ifData_u.bitFields.pb5 = aviPayLoad.namedIfData.ifData_u.bitFields.pb5;
        hwIfData.ifData[0x6] = 0x00;
        hwIfData.ifData[0x7] = 0x00;
        hwIfData.ifData[0x8] = 0x00;
        hwIfData.ifData[0x9] = 0x00;
        hwIfData.ifData[0xA] = 0x00;
        hwIfData.ifData[0xB] = 0x00;
        hwIfData.ifData[0xC] = 0x00;
        hwIfData.ifData[0xD] = 0x00;

        SiiHdmiTxLiteDrvPrepareHwAviInfoFrame(&hwIfData);
#endif //)
}
#else
static void SiiHdmiTxLiteDrvPrepareHwAviInfoFrame( PHwAviPayLoad_t pPayLoad )
{
//uint8_t                 outputClrSpc;
        AviInfoFrameDataByte2_t colorimetryAspectRatio;
        AviInfoFrameDataByte4_t inputVideoCode;
        QuantizationSettings_e  settings;

        //outputClrSpc            = SiiMhlTxDrvGetOutputColorSpace();  // originates from EDID

        colorimetryAspectRatio  = SiiMhlTxDrvGetColorimetryAspectRatio();
        inputVideoCode          = SiiMhlTxDrvGetInputVideoCode();

        pPayLoad->namedIfData.checksum = 0; // the checksum itself is included in the calculation.

        //pPayLoad->namedIfData.ifData_u.bitFields.pb1.colorSpace = outputClrSpc&0x7F;
        pPayLoad->namedIfData.ifData_u.bitFields.colorimetryAspectRatio = colorimetryAspectRatio;
        pPayLoad->namedIfData.ifData_u.bitFields.VIC = inputVideoCode;

        settings = qsAutoSelectByColorSpace;
        switch (VIDEO_CAPABILITY_D_BLOCK_found)
        {
        case 1:
                settings = qsLimitedRange;
                pPayLoad->namedIfData.ifData_u.bitFields.pb3.RGBQuantizationRange=1;
                break;
        case 0:
                settings = qsFullRange;
                pPayLoad->namedIfData.ifData_u.bitFields.pb3.RGBQuantizationRange=0;
                break;
        default:
                settings = qsFullRange;
                pPayLoad->namedIfData.ifData_u.bitFields.pb3.RGBQuantizationRange=0;
                break;
        }
        //pPayLoad->namedIfData.ifData_u.bitFields.pb3.RGBQuantizationRange=1;
        TXD_DEBUG_PRINT(("Drv: VIDEO_CAPABILITY_D_BLOCK_found settings:%d ,VIDEO_CAPABILITY_D_BLOCK_found=0x%02x\n",settings,VIDEO_CAPABILITY_D_BLOCK_found));

        SiiMhlTxDrvSetOutputQuantizationRange(settings);
        SiiMhlTxDrvSetInputQuantizationRange(qsFullRange);

        pPayLoad->namedIfData.checksum = CalculateAviInfoFrameChecksum(pPayLoad);
        //INFO_DEBUG_PRINT(("Drv: outputClrSpc: %02x\n",outputClrSpc));

        DumpIncomingInfoFrame(pPayLoad,sizeof(*pPayLoad));


        // output color space value chosen by EDID parser
        // and input settings from incoming info frame
        SiiMhlTxDrvApplyColorSpaceSettings();

        aviPayLoad = *pPayLoad;
        // don't send the info frame until we un-mute the output
        //SiiHmdiTxLiteDrvSendHwAviInfoFrame( );

}

void SiiHmdiTxLiteDrvPrepareAviInfoframe ( void )
{
        extern uint8_t packedPixelNeeded;
        HwAviPayLoad_t hwIfData;
        AviInfoFrameDataByte4_t inputVideoCode;
        INFO_DEBUG_PRINT(("PrepareAviInfoframe\n"));
        hwIfData.namedIfData.ifData_u.bitFields.VIC= SiiMhlTxDrvGetInputVideoCode();
        inputVideoCode= SiiMhlTxDrvGetInputVideoCode();
        TXD_DEBUG_PRINT(("inputVideoCode=%02X,hwIfData.namedIfData.ifData_u.bitFields.VIC.VIC=0x%x\n",
                         inputVideoCode.VIC,hwIfData.namedIfData.ifData_u.bitFields.VIC.VIC));

        if(packedPixelNeeded==1)
        {
                hwIfData.namedIfData.ifData_u.bitFields.pb1.colorSpace = 1;        //Ycbcr422
        }
        else
        {
                hwIfData.namedIfData.ifData_u.bitFields.pb1.colorSpace = 0;        //RGB
        }
        hwIfData.namedIfData.ifData_u.bitFields.pb1.futureMustBeZero = 0; // originates from incoming infoframe
        hwIfData.namedIfData.ifData_u.bitFields.pb1.ScanInfo = 2; // originates from incoming infoframe
        hwIfData.namedIfData.ifData_u.bitFields.pb1.ActiveFormatInfoPresent = 0; // originates from incoming infoframe
        hwIfData.namedIfData.ifData_u.bitFields.colorimetryAspectRatio = SiiMhlTxDrvGetColorimetryAspectRatio();
        hwIfData.ifData[0x3] = 0x00;

        hwIfData.namedIfData.ifData_u.bitFields.pb3.RGBQuantizationRange=1;

        TXC_DEBUG_PRINT(("\t\thwIfData.namedIfData.ifData_u.bitFields.VIC.VIC:%d\n, hwIfData.namedIfData.ifData_u.bitFields.pb1.colorSpace=0x%x,\nhwIfData.namedIfData.ifData_u.bitFields.colorimetryAspectRatio=0x%x\n",
                         (uint16_t)hwIfData.namedIfData.ifData_u.bitFields.VIC.VIC,hwIfData.namedIfData.ifData_u.bitFields.pb1.colorSpace,hwIfData.namedIfData.ifData_u.bitFields.colorimetryAspectRatio));
        hwIfData.ifData[0x5] = 0x00;
        hwIfData.ifData[0x6] = 0x00;
        hwIfData.ifData[0x7] = 0x00;
        hwIfData.ifData[0x8] = 0x00;
        hwIfData.ifData[0x9] = 0x00;
        hwIfData.ifData[0xA] = 0x00;
        hwIfData.ifData[0xB] = 0x00;
        hwIfData.ifData[0xC] = 0x00;
        hwIfData.ifData[0xD] = 0x00;
#if 0
        if(VIDEO_CAPABILITY_D_BLOCK_found)
        {
                hwIfData.ifData[6] = 0x04;
                TXC_DEBUG_PRINT(("VIDEO_CAPABILITY_D_BLOCK_found = true, limited range\n"));
        }
        else
        {
                hwIfData.ifData[6] = 0x00;
                TXC_DEBUG_PRINT(("VIDEO_CAPABILITY_D_BLOCK_found= false. defult range\n"));
        }
#endif
        SiiHdmiTxLiteDrvPrepareHwAviInfoFrame(&hwIfData);

}

#endif
void SiiHdmiTxLiteDrvSendVendorSpecificInfoFrame(PVendorSpecificInfoFrame_t pVendorSpecificInfoFrame)
{

        if (sink_type_HDMI == SiiMhlTxGetSinkType())
        {
                INFO_DEBUG_PRINT(("VSIF: length:%02x constant:%02x\n"
                                  ,(uint16_t)pVendorSpecificInfoFrame->header.length
                                  ,(uint16_t)sizeof(pVendorSpecificInfoFrame->payLoad)
                                 ));
                SII_ASSERT((11==sizeof(*pVendorSpecificInfoFrame)),("\n\n\ninfo frame size:%02x not expected\n\n\n"
                                ,(uint16_t)sizeof(*pVendorSpecificInfoFrame)) )

                pVendorSpecificInfoFrame->payLoad.checksum = 0;
                pVendorSpecificInfoFrame->payLoad.checksum = CalculateGenericCheckSum((uint8_t *)&pVendorSpecificInfoFrame->payLoad,0,sizeof(pVendorSpecificInfoFrame->payLoad));;
                SiiRegWriteBlock(REG_TPI_INFO_BYTE00, (uint8_t *)pVendorSpecificInfoFrame, sizeof(*pVendorSpecificInfoFrame));
        }
}

