/*
  Copyright (c) 2011, Silicon Image, Inc.  All rights reserved.
  No part of this work may be reproduced, modified, distributed, transmitted,
  transcribed, or translated into any language or computer format, in any form
  or by any means without written permission of: Silicon Image, Inc.,
  1140 East Arques Avenue, Sunnyvale, California 94085
*/
/*
   @file si_mhl_tx.c
*/
// Standard C Library
#include "si_memsegsupport.h"
#include "si_common.h"
#include "si_bitdefs.h"
#include "si_mhl_defs.h"
#include "si_mhl_tx_api.h"
#include "si_edid.h"
#include "si_drv_infoframe.h"
#include "si_tpi_regs.h"
#include "si_mhl_tx.h"
#include "si_mhl_tx_base_drv_api.h"  // generic driver interface
#include "si_drv_hdmi_tx_lite_edid.h"
#include "si_drv_mhl_tx.h"  // exported stuff from the driver
#include "si_hdmi_tx_lite_api.h"
#include "queue.h"

unsigned char VIDEO_CAPABILITY_D_BLOCK_found;
extern void *memset  (void *s, char val, int n);
#ifdef DO_HDCP
extern bool_t  DoHdcp;
#endif
uint8_t packedPixelNeeded=0;
#define DEVCAP_REFRESH_ACTIVE   (mhlTxConfig.ucDevCapCacheIndex <  sizeof(mhlTxConfig.devCapCache.aucDevCapCache))
#define DEVCAP_REFRESH_COMPLETE (mhlTxConfig.ucDevCapCacheIndex == sizeof(mhlTxConfig.devCapCache.aucDevCapCache))
#define DEVCAP_REFRESH_ONCE     (mhlTxConfig.ucDevCapCacheIndex <= sizeof(mhlTxConfig.devCapCache.aucDevCapCache))
#define DEVCAP_REFRESH_IDLE     (mhlTxConfig.ucDevCapCacheIndex >= sizeof(mhlTxConfig.devCapCache.aucDevCapCache))
#define UpdateDevCapCacheIndex(x) \
{   \
     mhlTxConfig.ucDevCapCacheIndex=(x); \
     CBUS_DEBUG_PRINT(("mhlTxConfig.ucDevCapCacheIndex: %02x\n",(uint16_t)mhlTxConfig.ucDevCapCacheIndex)); \
}
#define PackedPixelAvailable  (MHL_DEV_VID_LINK_SUPP_PPIXEL & mhlTxConfig.devCapCache.aucDevCapCache[DEVCAP_OFFSET_VID_LINK_MODE])
//#define PackedPixelAvailable ((MHL_DEV_VID_LINK_SUPP_PPIXEL & mhlTxConfig.devCapCache.aucDevCapCache[DEVCAP_OFFSET_VID_LINK_MODE]) && (MHL_DEV_VID_LINK_SUPP_PPIXEL & DEVCAP_VAL_VID_LINK_MODE) )
/*
queue implementation
*/
#define NUM_CBUS_EVENT_QUEUE_EVENTS 6
typedef struct _CBusQueue_t
{
        QueueHeader_t header;
        cbus_req_t queue[NUM_CBUS_EVENT_QUEUE_EVENTS];
} CBusQueue_t,*PCBusQueue_t;



// Because the Linux driver can be opened multiple times it can't
// depend on one time structure initialization done by the compiler.
//CBusQueue_t CBusQueue={0,0,{0}};
CBusQueue_t CBusQueue;

cbus_req_t *GetNextCBusTransactionImpl(void)
{
        if (0==QUEUE_DEPTH(CBusQueue))
        {
                return NULL;
        }
        else
        {
                cbus_req_t *retVal;
                retVal = &CBusQueue.queue[CBusQueue.header.head];
                ADVANCE_QUEUE_HEAD(CBusQueue)
                return retVal;
        }
}
#ifdef ENABLE_CBUS_DEBUG_PRINT //(
cbus_req_t *GetNextCBusTransactionWrapper(char *pszFunction,int iLine)
{
        CBUS_DEBUG_PRINT(("MhlTx:%d GetNextCBusTransaction: %s depth: %d  head: %d  tail: %d\n",
                          iLine,pszFunction,
                          (int)QUEUE_DEPTH(CBusQueue),
                          (int)CBusQueue.header.head,
                          (int)CBusQueue.header.tail));
        return  GetNextCBusTransactionImpl();
}

#define GetNextCBusTransaction(func) GetNextCBusTransactionWrapper(#func,__LINE__)
#else //)(
#define GetNextCBusTransaction(func) GetNextCBusTransactionImpl()
#endif //)

bool_t PutNextCBusTransactionImpl(cbus_req_t *pReq)
{
        if (QUEUE_FULL(CBusQueue))
        {
                //queue is full
                return false;
        }
        // at least one slot available
        CBusQueue.queue[CBusQueue.header.tail] = *pReq;
        ADVANCE_QUEUE_TAIL(CBusQueue)
        return true;
}
#ifdef ENABLE_CBUS_DEBUG_PRINT //(
// use this wrapper to do debugging output for the routine above.
bool_t PutNextCBusTransactionWrapper(cbus_req_t *pReq,int iLine)
{
        bool_t retVal;

        CBUS_DEBUG_PRINT(("MhlTx:%d PutNextCBusTransaction %02X %02X %02X depth:%d head: %d tail:%d\n"
                          ,iLine
                          ,(int)pReq->command
                          ,(int)((MHL_MSC_MSG == pReq->command)?pReq->payload_u.msgData[0]:pReq->offsetData)
                          ,(int)((MHL_MSC_MSG == pReq->command)?pReq->payload_u.msgData[1]:pReq->payload_u.msgData[0])
                          ,(int)QUEUE_DEPTH(CBusQueue)
                          ,(int)CBusQueue.header.head
                          ,(int)CBusQueue.header.tail
                         ));
        retVal = PutNextCBusTransactionImpl(pReq);

        if (!retVal)
        {
                ERROR_DEBUG_PRINT(("MhlTx:%d PutNextCBusTransaction queue full, when adding event %02x\n"
                                   , iLine
                                   , (int)pReq->command));
        }
        return retVal;
}
#define PutNextCBusTransaction(req) PutNextCBusTransactionWrapper(req,__LINE__)
#else //)(
#define PutNextCBusTransaction(req) PutNextCBusTransactionImpl(req)
#endif //)

bool_t PutPriorityCBusTransactionImpl(cbus_req_t *pReq)
{
        if (QUEUE_FULL(CBusQueue))
        {
                //queue is full
                return false;
        }
        // at least one slot available
        RETREAT_QUEUE_HEAD(CBusQueue)
        CBusQueue.queue[CBusQueue.header.head] = *pReq;
        return true;
}
#ifdef ENABLE_CBUS_DEBUG_PRINT //(
bool_t PutPriorityCBusTransactionWrapper(cbus_req_t *pReq,int iLine)
{
        bool_t retVal;
        CBUS_DEBUG_PRINT(("MhlTx:%d: PutPriorityCBusTransaction %02X %02X %02X depth:%d head: %d tail:%d\n"
                          ,iLine
                          ,(int)pReq->command
                          ,(int)((MHL_MSC_MSG == pReq->command)?pReq->payload_u.msgData[0]:pReq->offsetData)
                          ,(int)((MHL_MSC_MSG == pReq->command)?pReq->payload_u.msgData[1]:pReq->payload_u.msgData[0])
                          ,(int)QUEUE_DEPTH(CBusQueue)
                          ,(int)CBusQueue.header.head
                          ,(int)CBusQueue.header.tail
                         ));
        retVal = PutPriorityCBusTransactionImpl(pReq);
        if (!retVal)
        {
                CBUS_DEBUG_PRINT(("MhlTx:%d: PutPriorityCBusTransaction queue full, when adding event 0x%02X\n",iLine,(int)pReq->command));
        }
        return retVal;
}
#define PutPriorityCBusTransaction(pReq) PutPriorityCBusTransactionWrapper(pReq,__LINE__)
#else //)(
#define PutPriorityCBusTransaction(pReq) PutPriorityCBusTransactionImpl(pReq)
#endif //)

#define IncrementCBusReferenceCount(func) {mhlTxConfig.cbusReferenceCount++; CBUS_DEBUG_PRINT(("MhlTx:%s cbusReferenceCount:%d\n",#func,(int)mhlTxConfig.cbusReferenceCount)); }
#define DecrementCBusReferenceCount(func) {mhlTxConfig.cbusReferenceCount--; CBUS_DEBUG_PRINT(("MhlTx:%s cbusReferenceCount:%d\n",#func,(int)mhlTxConfig.cbusReferenceCount)); }

#define UpdateEdidLoopActiveStatus(x) { mhlTxConfig.flags.edidLoopActive= x; }

#define SetMhlTxFlag(x) { mhlTxConfig.flags.x = 1; ERROR_DEBUG_PRINT((#x":1\n")); }
#define ClrMhlTxFlag(x) { mhlTxConfig.flags.x = 0; ERROR_DEBUG_PRINT((#x":0\n")); }

#define SetMiscFlag(func,x) { mhlTxConfig.miscFlags |=  (x); TXC_DEBUG_PRINT(("MhlTx:%s set %s\n",#func,#x)); }
#define ClrMiscFlag(func,x) { mhlTxConfig.miscFlags &= ~(x); TXC_DEBUG_PRINT(("MhlTx:%s clr %s\n",#func,#x)); }
#define TestMiscFlag(x) (mhlTxConfig.miscFlags & (x))
//
// Store global config info here. This is shared by the driver.
//
//
//
// structure to hold operating information of MhlTx component
//
static	mhlTx_config_t	mhlTxConfig= {0};
//
// Functions used internally.
//
static bool_t SiiMhlTxSetDCapRdy( void );
static	bool_t 		SiiMhlTxRapkSend( void );

static	void		MhlTxResetStates( void );
static	bool_t		MhlTxSendMscMsg ( uint8_t command, uint8_t cmdData );
static void SiiMhlTxRefreshPeerDevCapEntries(void);
static void MhlTxDriveStates( void );
extern PLACE_IN_CODE_SEG uint8_t rcpSupportTable [];

bool_t MhlTxCBusBusy(void)
{
        return ((QUEUE_DEPTH(CBusQueue) > 0)||SiiMhlTxDrvCBusBusy() || mhlTxConfig.cbusReferenceCount)?true:false;
}

uint8_t edidBlockData [4*EDID_BLOCK_SIZE];
Type_EDID_Descriptors EDID_Data;        // holds parsed EDID data needed by the FW
#if 0
/*
	QualifyPathEnable
		Support MHL 1.0 sink devices.

	return value
		1 - consider PATH_EN received
		0 - consider PATH_EN NOT received
 */
static uint8_t QualifyPathEnable( void )
{
        uint8_t retVal = 0;  // return fail by default
        if (MHL_STATUS_PATH_ENABLED & mhlTxConfig.status_1)
        {
                TXC_DEBUG_PRINT(("   MHL_STATUS_PATH_ENABLED\n"));
                retVal = 1;
        }
        else
        {
                // first check for DEVCAP refresh inactivity
                if (DEVCAP_REFRESH_IDLE)
                {
                        if (0x10 == mhlTxConfig.devCapCache.aucDevCapCache[DEVCAP_OFFSET_MHL_VERSION])
                        {
                                if (0x44 == mhlTxConfig.devCapCache.aucDevCapCache[DEVCAP_OFFSET_INT_STAT_SIZE])
                                {
                                        retVal = 1;
                                        TXC_DEBUG_PRINT(("   Legacy Support for MHL_STATUS_PATH_ENABLED\n"));
                                }
                        }
                }
        }
        return retVal;
}
#endif
static int IsValidAviInfoFrame(PAVIInfoFrame_t pAviInfoFrame)
{
        uint8_t checkSum=0;
        checkSum = CalculateGenericCheckSum((uint8_t *)pAviInfoFrame,0,sizeof(*pAviInfoFrame));

        if (0 != checkSum)
        {
                INFO_DEBUG_PRINT(("ERROR: AVIF CheckSum: %d\n",(uint16_t)checkSum ));
                DumpIncomingInfoFrame(pAviInfoFrame,sizeof(*pAviInfoFrame));
                return 0;
        }
        else if (0x82 != pAviInfoFrame->header.type_code)
        {
                ERROR_DEBUG_PRINT(("Wrong pAviInfoFrame->header.type_code\n"));
                DumpIncomingInfoFrame(pAviInfoFrame,sizeof(*pAviInfoFrame));
                return 0;
        }
        else if (0x02 != pAviInfoFrame->header.version_number)
        {
                ERROR_DEBUG_PRINT(("Wrong pAviInfoFrame->header.version_number\n"));
                DumpIncomingInfoFrame(pAviInfoFrame,sizeof(*pAviInfoFrame));
                return 0;
        }
        else if (0x0D != pAviInfoFrame->header.length)
        {
                ERROR_DEBUG_PRINT(("Wrong pAviInfoFrame->header.length\n"));
                DumpIncomingInfoFrame(pAviInfoFrame,sizeof(*pAviInfoFrame));
                return 0;
        }
        else if(0 == pAviInfoFrame->payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC)
        {
                ERROR_DEBUG_PRINT(("VIC = 0 because APdidn't sent the VIC out when get timing From Detail timing\n"));
                DumpIncomingInfoFrame(pAviInfoFrame,sizeof(*pAviInfoFrame));
                return 1;
        }
        else
        {
                return 1;
        }
}


uint8_t SiiMhlTxReadyForVideo(void)
{
        uint8_t retVal=0;
        uint16_t thisFar=__LINE__;

        if (MHL_RSEN & mhlTxConfig.mhlHpdRSENflags)
        {
                thisFar=__LINE__;
                TXC_DEBUG_PRINT ((" MHL_RSEN\n"));
                if (MHL_HPD & mhlTxConfig.mhlHpdRSENflags)
                {
                        thisFar=__LINE__;
                        TXC_DEBUG_PRINT (("  MHL_HPD\n"));
                        //if (QualifyPathEnable())
                        {
                                thisFar=__LINE__;
                                if (RAP_CONTENT_ON & mhlTxConfig.rapFlags)
                                {
                                        thisFar=__LINE__;
                                        TXC_DEBUG_PRINT(("    RAP CONTENT_ON\n"));
                                        if (TestMiscFlag(FLAGS_EDID_READ_DONE))
                                        {
                                                thisFar=__LINE__;
                                                TXC_DEBUG_PRINT(("     EDID_READ_DONE\n"));
//                        if (mhlTxConfig.flags.syncDetect)
                                                {
                                                        thisFar=__LINE__;
                                                        TXC_DEBUG_PRINT(("      Sync Detect\n"));
                                                        if (mhlTxConfig.flags.upStreamHPD)
                                                        {
                                                                thisFar=__LINE__;
                                                                TXC_DEBUG_PRINT(("       Upstream HPD\n"));
                                                                switch(EDID_Data.sinkType)
                                                                {
                                                                case sink_type_pending:
                                                                        TXC_DEBUG_PRINT(("         sink type pending\n"));
                                                                        thisFar=__LINE__;
                                                                        break;
                                                                case sink_type_DVI:
                                                                        TXC_DEBUG_PRINT(("         sink type DVI\n"));
                                                                        if (mhlTxConfig.flags.syncDetect)
                                                                        {
                                                                                /* Wait for SCDT for DVI cases */
                                                                                return 1;
                                                                        }
                                                                        break;
                                                                case sink_type_HDMI:
                                                                        thisFar=__LINE__;
                                                                        TXC_DEBUG_PRINT(("         sink type HDMI\n"));
                                                                        //if (mhlTxConfig.flags.validAviInfoFrame)
                                                                        {
                                                                                TXC_DEBUG_PRINT(("          valid AVI-IF\n"));
                                                                                return 1;
                                                                        }
                                                                        break;
                                                                }
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }
        ERROR_DEBUG_PRINT(("No video(L: %d)\n",thisFar));
        return retVal;
}

///////////////////////////////////////////////////////////////////////////////
// SiiMhlTxTmdsEnable
//
// Implements conditions on enabling TMDS output stated in MHL spec section 7.6.1
//
//
void SiiMhlTxTmdsEnable(void)
{

        TXC_DEBUG_PRINT (("MhlTx:SiiMhlTxTmdsEnable\n"));
        if (mhlTxConfig.modeFlags & mfTranscodeMode)
        {
                if (MHL_RSEN & mhlTxConfig.mhlHpdRSENflags)
                {
                        TXC_DEBUG_PRINT (("\tMHL_RSEN\n"));
                        if (MHL_HPD & mhlTxConfig.mhlHpdRSENflags)
                        {
                                TXC_DEBUG_PRINT (("\t\tMHL_HPD\n"));
                                //if (QualifyPathEnable())
                                {
                                        TXC_DEBUG_PRINT(("   MHL_STATUS_PATH_ENABLED\n"));
                                        if (RAP_CONTENT_ON & mhlTxConfig.rapFlags)
                                        {
                                                TXC_DEBUG_PRINT(("\t\t\t\tRAP CONTENT_ON\n"));
                                                TXC_DEBUG_PRINT(("\t\t\t\tcbus_req queue depth: %d\n",(int)QUEUE_DEPTH(CBusQueue)));
                                                SiiMhlTxDrvTmdsControl( true );
                                                AppNotifyMhlEvent(MHL_TX_EVENT_TMDS_ENABLED,0,NULL);
                                        }
                                }
                        }
                }
        }
        else
        {
                if (SiiMhlTxReadyForVideo())
                {
                        if (SiiMhlTxAttemptPackedPixel())
                        {
                                TXC_DEBUG_PRINT(("\t\t\t\t\t\t\tVIC:%d\n",(uint16_t)mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC));
                                if (QUEUE_DEPTH(CBusQueue))
                                {
                                        TXC_DEBUG_PRINT(("cbus_req queue depth: %d\n",(int)QUEUE_DEPTH(CBusQueue)));
                                        // empty one event from the CBUS queue to lessen likelihood of interfering with authentication
                                        MhlTxDriveStates();
                                        TXC_DEBUG_PRINT(("cbus_req queue depth: %d\n",(int)QUEUE_DEPTH(CBusQueue)));
                                }
                                SiiMhlTxDrvTmdsControl( true );
#if 1//def INFO_FRAMES_AT_TMDS_ENABLE //(
                                SiiHmdiTxLiteDrvSendHwAviInfoFrame();
#endif //)
                                AppNotifyMhlEvent(MHL_TX_EVENT_TMDS_ENABLED,0,NULL);
                        }
                }
        }
}

void SiiMhlTxTmdsDisable( void )
{
        SiiMhlTxDrvTmdsControl( false );
        AppNotifyMhlEvent(MHL_TX_EVENT_TMDS_DISABLED,0,NULL);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxSetInt
//
// Set MHL defined INTERRUPT bits in peer's register set.
//
// This function returns true if operation was successfully performed.
//
//  regToWrite      Remote interrupt register to write
//
//  mask            the bits to write to that register
//
//  priority        0:  add to head of CBusQueue
//                  1:  add to tail of CBusQueue
//
static bool_t SiiMhlTxSetInt( uint8_t regToWrite,uint8_t  mask, uint8_t priorityLevel )
{
        cbus_req_t	req;
        bool_t retVal;

        // find the offset and bit position
        // and feed
        req.retryCount  = 2;
        req.command     = MHL_SET_INT;
        req.offsetData  = regToWrite;
        req.payload_u.msgData[0]  = mask;
        if (0 == priorityLevel)
        {
                retVal = PutPriorityCBusTransaction(&req);
        }
        else
        {
                retVal = PutNextCBusTransaction(&req);
        }
        MhlTxDriveStates();
        return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDoWriteBurst
//
static bool_t SiiMhlTxDoWriteBurst( uint8_t startReg, uint8_t *pData,uint8_t length )
{
        if (FLAGS_WRITE_BURST_PENDING & mhlTxConfig.miscFlags)
        {
                cbus_req_t	req;
                bool_t retVal;

                SCRPAD_DEBUG_PRINT (("MhlTx:SiiMhlTxDoWriteBurst startReg:%d length:%d\n",(int)startReg,(int)length ));

                req.retryCount  = 1;
                req.command     = MHL_WRITE_BURST;
                req.length      = length;
                req.offsetData  = startReg;
                req.payload_u.pdatabytes  = pData;

                retVal = PutPriorityCBusTransaction(&req);
                ClrMiscFlag(SiiMhlTxDoWriteBurst, FLAGS_WRITE_BURST_PENDING)
                return retVal;
        }
        return false;
}

/////////////////////////////////////////////////////////////////////////
// SiiMhlTxRequestWriteBurst
//
ScratchPadStatus_e SiiMhlTxRequestWriteBurst(uint8_t startReg, uint8_t length, uint8_t *pData)
{
        ScratchPadStatus_e retVal = SCRATCHPAD_BUSY;

        if (!(MHL_FEATURE_SP_SUPPORT & mhlTxConfig.devCapCache.mdc.featureFlag))
        {
                CBUS_DEBUG_PRINT (("MhlTx:SiiMhlTxRequestWriteBurst failed SCRATCHPAD_NOT_SUPPORTED\n" ));
                retVal= SCRATCHPAD_NOT_SUPPORTED;
        }
        else
        {
                if (
                        (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)
                        ||
                        MhlTxCBusBusy()
                )
                {
                        SCRPAD_DEBUG_PRINT (("MhlTx:SiiMhlTxRequestWriteBurst failed FLAGS_SCRATCHPAD_BUSY \n" ));
                }
                else
                {
                        bool_t temp;
                        uint8_t i,reg;
                        SCRPAD_DEBUG_PRINT (("MhlTx:SiiMhlTxRequestWriteBurst, request sent\n"));
                        for (i = 0,reg=startReg; (i < length) && (reg < SCRATCHPAD_SIZE); ++i,++reg)
                        {
                                mhlTxConfig.outgoingScratchPad.asBytes[reg]=pData[i];
                        }

                        temp =  SiiMhlTxSetInt(MHL_RCHANGE_INT,MHL_INT_REQ_WRT, 1);
                        retVal = temp ? SCRATCHPAD_SUCCESS: SCRATCHPAD_FAIL;
                }
        }
        return retVal;
}

///////////////////////////////////////////////////////////////////////////////
// SiiMhlTxInitialize
//
// Sets the transmitter component firmware up for operation, brings up chip
// into power on state first and then back to reduced-power mode D3 to conserve
// power until an MHL cable connection has been established. If the MHL port is
// used for USB operation, the chip and firmware continue to stay in D3 mode.
// Only a small circuit in the chip observes the impedance variations to see if
// processor should be interrupted to continue MHL discovery process or not.
//
// All device events will result in call to the function SiiMhlTxDeviceIsr()
// by host's hardware or software(a master interrupt handler in host software
// can call it directly). This implies that the MhlTx component shall make use
// of AppDisableInterrupts() and AppRestoreInterrupts() for any critical section
// work to prevent concurrency issues.
//
// Parameters
//
// pollIntervalMs		This number should be higher than 0 and lower than
//						51 milliseconds for effective operation of the firmware.
//						A higher number will only imply a slower response to an
//						event on MHL side which can lead to violation of a
//						connection disconnection related timing or a slower
//						response to RCP messages.
//
//
//
//
void SiiMhlTxInitialize(uint8_t pollIntervalMs )
{
        TXC_DEBUG_PRINT (("MhlTx:SiiMhlTxInitialize\n"));

        // Initialize queue of pending CBUS requests.
        CBusQueue.header.head = 0;
        CBusQueue.header.tail = 0;

        //
        // Remember mode of operation.
        //
        mhlTxConfig.pollIntervalMs  = pollIntervalMs;

        TXC_DEBUG_PRINT(("MhlTx: HPD: %d RSEN: %d\n"
                         ,(int)((mhlTxConfig.mhlHpdRSENflags & MHL_HPD)?1:0)
                         ,(int)((mhlTxConfig.mhlHpdRSENflags & MHL_RSEN)?1:0)
                        ));
        MhlTxResetStates( );
        TXC_DEBUG_PRINT(("MhlTx: HPD: %d RSEN: %d\n"
                         ,(mhlTxConfig.mhlHpdRSENflags & MHL_HPD)?1:0
                         ,(mhlTxConfig.mhlHpdRSENflags & MHL_RSEN)?1:0
                        ));

        SiiMhlTxChipInitialize ();
}



///////////////////////////////////////////////////////////////////////////////
//
// MhlTxProcessEvents
//
// This internal function is called at the end of interrupt processing.  It's
// purpose is to process events detected during the interrupt.  Some events are
// internally handled here but most are handled by a notification to the application
// layer.
//
void MhlTxProcessEvents(void)
{
        // Make sure any events detected during the interrupt are processed.
        MhlTxDriveStates();
        if( mhlTxConfig.mhlConnectionEvent )
        {
                TXC_DEBUG_PRINT (("MhlTx:MhlTxProcessEvents mhlConnectionEvent\n"));

                // Consume the message
                mhlTxConfig.mhlConnectionEvent = false;

                //
                // Let app know about the change of the connection state.
                //
                AppNotifyMhlEvent(mhlTxConfig.mhlConnected, mhlTxConfig.devCapCache.mdc.featureFlag,NULL);

                // If connection has been lost, reset all state flags.
                if(MHL_TX_EVENT_DISCONNECTION == mhlTxConfig.mhlConnected)
                {
                        MhlTxResetStates( );
                }
                else if (MHL_TX_EVENT_CONNECTION == mhlTxConfig.mhlConnected)
                {
                        SiiMhlTxSetDCapRdy();
                }
        }
        else if( mhlTxConfig.mscMsgArrived )
        {
                TXC_DEBUG_PRINT (("MhlTx:MhlTxProcessEvents MSC MSG <%02X, %02X>\n"
                                  ,(int) ( mhlTxConfig.mscMsgSubCommand )
                                  ,(int) ( mhlTxConfig.mscMsgData )
                                 ));

                // Consume the message
                mhlTxConfig.mscMsgArrived = false;

                //
                // Map sub-command to an event id
                //
                switch( mhlTxConfig.mscMsgSubCommand )
                {
                case	MHL_MSC_MSG_RCP:
                        // If we get a RCP key that we do NOT support, send back RCPE
                        // Do not notify app layer.
                        if(MHL_LOGICAL_DEVICE_MAP & rcpSupportTable [mhlTxConfig.mscMsgData & 0x7F] )
                        {
                                AppNotifyMhlEvent(MHL_TX_EVENT_RCP_RECEIVED, mhlTxConfig.mscMsgData,NULL);
                                //done in app layer SiiMhlTxRcpkSend( mhlTxConfig.mscMsgData );
                        }
                        else
                        {
                                // Save keycode to send a RCPK after RCPE.
                                mhlTxConfig.mscSaveRcpKeyCode = mhlTxConfig.mscMsgData; // key code
                                SiiMhlTxRcpeSend( RCPE_INEFFECTIVE_KEY_CODE );
                        }
                        break;

                case	MHL_MSC_MSG_RCPK:
                        AppNotifyMhlEvent(MHL_TX_EVENT_RCPK_RECEIVED, mhlTxConfig.mscMsgData,NULL);
                        DecrementCBusReferenceCount(MhlTxProcessEvents)
                        mhlTxConfig.mscLastCommand = 0;
                        mhlTxConfig.mscMsgLastCommand = 0;

                        TXC_DEBUG_PRINT (("MhlTx:MhlTxProcessEvents RCPK\n"));
                        break;

                case	MHL_MSC_MSG_RCPE:
                        AppNotifyMhlEvent(MHL_TX_EVENT_RCPE_RECEIVED, mhlTxConfig.mscMsgData,NULL);
                        break;

                case	MHL_MSC_MSG_RAP:
                        // RAP is fully handled here.
                        //
                        // Handle RAP sub-commands here itself
                        //
                        if( MHL_RAP_CONTENT_ON == mhlTxConfig.mscMsgData)
                        {
                                mhlTxConfig.rapFlags |= RAP_CONTENT_ON;
                                TXC_DEBUG_PRINT(("RAP CONTENT_ON\n"));

                                if (mhlTxConfig.modeFlags & mfTranscodeMode)
                                {
                                        SiiMhlTxTmdsEnable();
                                }
                        }
                        else if( MHL_RAP_CONTENT_OFF == mhlTxConfig.mscMsgData)
                        {
                                mhlTxConfig.rapFlags &= ~RAP_CONTENT_ON;
                                TXC_DEBUG_PRINT(("RAP CONTENT_OFF\n"));
                                SiiMhlTxDrvTmdsControl( false );
                        }
                        // Always RAPK to the peer
                        SiiMhlTxRapkSend( );
                        break;

                case	MHL_MSC_MSG_RAPK:
                        // Do nothing if RAPK comes, except decrement the reference counter
                        DecrementCBusReferenceCount(MhlTxProcessEvents)
                        mhlTxConfig.mscLastCommand = 0;
                        mhlTxConfig.mscMsgLastCommand = 0;
                        TXC_DEBUG_PRINT (("MhlTx:MhlTxProcessEventsRAPK\n"));
                        break;

                case	MHL_MSC_MSG_UCP:
                        AppNotifyMhlEvent(MHL_TX_EVENT_UCP_RECEIVED, mhlTxConfig.mscMsgData,NULL);
                        break;

                case	MHL_MSC_MSG_UCPK:
                        AppNotifyMhlEvent(MHL_TX_EVENT_UCPK_RECEIVED, mhlTxConfig.mscMsgData,NULL);
                        DecrementCBusReferenceCount(MhlTxProcessEvents)
                        mhlTxConfig.mscLastCommand = 0;
                        mhlTxConfig.mscMsgLastCommand = 0;

                        TXC_DEBUG_PRINT (("MhlTx:MhlTxProcessEvents UCPK\n"));
                        break;

                case	MHL_MSC_MSG_UCPE:
                        AppNotifyMhlEvent(MHL_TX_EVENT_UCPE_RECEIVED, mhlTxConfig.mscMsgData,NULL);
                        break;

                default:
                        // Any freak value here would continue with no event to app
                        break;
                }
        }
}

///////////////////////////////////////////////////////////////////////////////
//
// MhlTxDriveStates
//
// This function is called by the interrupt handler in the driver layer.
// to move the MSC engine to do the next thing before allowing the application
// to run RCP APIs.
//
static void MhlTxDriveStates( void )
{
        // process queued CBus transactions
        if (QUEUE_DEPTH(CBusQueue) > 0)
        {
                if (!SiiMhlTxDrvCBusBusy())
                {
                        int reQueueRequest = 0;
                        cbus_req_t *pReq = GetNextCBusTransaction(MhlTxDriveStates);
                        // coordinate write burst requests and grants.
                        if (MHL_SET_INT == pReq->command)
                        {
                                if (MHL_RCHANGE_INT == pReq->offsetData)
                                {
                                        if (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)
                                        {
                                                if (MHL_INT_REQ_WRT == pReq->payload_u.msgData[0])
                                                {
                                                        reQueueRequest= 1;
                                                }
                                                else if (MHL_INT_GRT_WRT == pReq->payload_u.msgData[0])
                                                {
                                                        reQueueRequest= 0;
                                                }
                                        }
                                        else
                                        {
                                                if (MHL_INT_REQ_WRT == pReq->payload_u.msgData[0])
                                                {
                                                        IncrementCBusReferenceCount(MhlTxDriveStates)
                                                        SetMiscFlag(MhlTxDriveStates, FLAGS_SCRATCHPAD_BUSY)
                                                        SetMiscFlag(MhlTxDriveStates, FLAGS_WRITE_BURST_PENDING)
                                                }
                                                else if (MHL_INT_GRT_WRT == pReq->payload_u.msgData[0])
                                                {
                                                        SetMiscFlag(MhlTxDriveStates, FLAGS_SCRATCHPAD_BUSY)
                                                }
                                        }
                                }
                        }
                        if (reQueueRequest)
                        {
                                // send this one to the back of the line for later attempts
                                if (pReq->retryCount-- > 0)
                                {
                                        PutNextCBusTransaction(pReq);
                                }
                        }
                        else
                        {
                                bool_t success;
                                if (MHL_MSC_MSG == pReq->command)
                                {
                                        mhlTxConfig.mscMsgLastCommand = pReq->payload_u.msgData[0];
                                        mhlTxConfig.mscMsgLastData    = pReq->payload_u.msgData[1];
                                }
                                else
                                {
                                        mhlTxConfig.mscLastOffset  = pReq->offsetData;
                                        mhlTxConfig.mscLastData    = pReq->payload_u.msgData[0];

                                }
                                mhlTxConfig.mscLastCommand = pReq->command;
                                CBUS_DEBUG_PRINT(("last command: %d offset:%d\n",(uint16_t)mhlTxConfig.mscLastCommand,(uint16_t)mhlTxConfig.mscLastOffset ));
                                IncrementCBusReferenceCount(MhlTxDriveStates)
                                success =
                                        SiiMhlTxDrvSendCbusCommand( pReq  );
                                if (!success)
                                {
                                        if (MHL_READ_EDID_BLOCK == mhlTxConfig.mscLastCommand)
                                        {
                                                // give up on EDID reading
                                                UpdateEdidLoopActiveStatus(0)
                                        }
                                }
                                CBUS_DEBUG_PRINT(("last command: %d\n",(uint16_t)mhlTxConfig.mscLastCommand ));
                        }
                }
        }
}


static void ExamineLocalAndPeerVidLinkMode( void )
{
        // set default values
        mhlTxConfig.linkMode &= ~MHL_STATUS_CLK_MODE_MASK;
        mhlTxConfig.linkMode |= MHL_STATUS_CLK_MODE_NORMAL;

        // when more modes than PPIXEL and normal are supported,
        //   this should become a table lookup.
        if (PackedPixelAvailable)
        {
                mhlTxConfig.linkMode &= ~MHL_STATUS_CLK_MODE_MASK;
                mhlTxConfig.linkMode |= mhlTxConfig.preferredClkMode;
        }
        // call driver here to set CLK_MODE
        SiMhlTxDrvSetClkMode(mhlTxConfig.linkMode & MHL_STATUS_CLK_MODE_MASK);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxSetStatus
//
// Set MHL defined STATUS bits in peer's register set.
//
// register	    MHLRegister to write
//
// value        data to write to the register
//

static bool_t SiiMhlTxSetStatus( uint8_t regToWrite, uint8_t value )
{
        cbus_req_t	req;
        bool_t retVal;

        // find the offset and bit position
        // and feed
        req.retryCount  = 2;
        req.command     = MHL_WRITE_STAT;
        req.offsetData  = regToWrite;
        req.payload_u.msgData[0]  = value;

        TXC_DEBUG_PRINT (("MhlTx:SiiMhlTxSetStatus\n"));
        retVal = PutNextCBusTransaction(&req);
        return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
//  SiiMhlTxSendLinkMode
//
static bool_t SiiMhlTxSendLinkMode(void)
{
        return SiiMhlTxSetStatus( MHL_STATUS_REG_LINK_MODE, mhlTxConfig.linkMode);
}



uint8_t QualifyPixelClockForMhl(uint32_t pixelClockFrequency,uint8_t bitsPerPixel)
{
        uint32_t pixelClockFrequencyDiv8;
        uint32_t linkClockFrequency;
        uint32_t maxLinkClockFrequency;
        uint8_t retVal;

        pixelClockFrequencyDiv8 = pixelClockFrequency / 8;

        linkClockFrequency = pixelClockFrequencyDiv8 * (uint32_t)bitsPerPixel;

        if ( PackedPixelAvailable )
        {
                maxLinkClockFrequency = 300000000;
        }
        else
        {
                maxLinkClockFrequency = 225000000;
        }

        if (linkClockFrequency <  maxLinkClockFrequency)
        {
                retVal = 1;
        }
        else
        {
                retVal = 0;
        }
        PIXCLK_DEBUG_PRINT(("Link clock:%lu Hz %12s for MHL at %d bpp (max: %lu Hz)\n"
                            ,linkClockFrequency
                            ,retVal?"valid":"unattainable"
                            ,(uint16_t)bitsPerPixel
                            ,maxLinkClockFrequency));
        return retVal;
}


PLACE_IN_CODE_SEG char *pszSpace           ="n/a";
PLACE_IN_CODE_SEG char *pszFrameSequential ="FS ";
PLACE_IN_CODE_SEG char *pszTopBottom       ="TB ";
PLACE_IN_CODE_SEG char *pszLeftRight       ="LR ";

typedef enum
{
        vifSingleFrameRate = 0x00
                             ,vifDualFrameRate   = 0x01

} VicInfoFlags_e;

typedef enum
{
        vsmProgressive  = 0
                          ,vsmInterlaced   = 1
} VicScanMode_e;

typedef enum
{
        par1to1
        ,par16to15
        ,par16to27
        ,par16to45
        ,par16to45_160to45
        ,par1to15_10to15
        ,par1to9_10to9
        ,par2to15_20to15
        ,par2to9
        ,par2to9_20to9
        ,par32to27
        ,par32to45
        ,par4to27_40to27
        ,par4to9
        ,par4to15
        ,par64to45
        ,par8to15
        ,par8to27
        ,par8to27_80to27
        ,par8to45_80to45
        ,par8to9
} PixelAspectRatio_e;



typedef struct _VIC_Info_Fields_t
{
        ImageAspectRatio_e  imageAspectRatio        :2;
        VicScanMode_e 		interlaced              :1;
        PixelAspectRatio_e 	pixelAspectRatio        :5;

        VicInfoFlags_e  frameRateInfo   :1;
        uint8_t         clocksPerPixelShiftCount:2;
        uint8_t         field2VBlank    :2;
        uint8_t         reserved        :3;
} VIC_Info_Fields_t,*PVIC_Info_Fields_t;



typedef struct _VIC_Info_t
{
        uint16_t columns;
        uint16_t rows;
        uint16_t HBlankInPixels;
        uint16_t VBlankInPixels;
        uint32_t fieldRateInMilliHz;
        VIC_Info_Fields_t fields;
//    uint16_t pixClockDiv10000;
} VIC_Info_t,*PVIC_Info_t;

//#define FILL_THIS_IN 0
#define Cea861D_VicInfoEntry(modeNum,columns,rows,HBlank,VBLank,FieldRate,ImageAspectRatio,scanmode,PixelAspectRatio,flags,clocksPerPelShift,AdditionalVBlank,pixClockDiv10000) \
                                    {columns,rows,HBlank,VBLank,FieldRate,{ImageAspectRatio,scanmode,PixelAspectRatio,flags,clocksPerPelShift,AdditionalVBlank}/*,pixClockDiv10000*/} // VIC:modeNum
PLACE_IN_CODE_SEG  VIC_Info_t vicInfo[]=
{
        Cea861D_VicInfoEntry( 0,   0,   0,  0, 0,  0000 ,iar16to10 ,vsmProgressive,par1to1           ,vifSingleFrameRate,0,0,0) // place holder
        ,Cea861D_VicInfoEntry( 1, 640, 480,160,45, 60000 ,iar4to3   ,vsmProgressive,par1to1           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry( 2, 720, 480,138,45, 60000 ,iar4to3   ,vsmProgressive,par8to9           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry( 3, 720, 480,138,45, 60000 ,iar16to9  ,vsmProgressive,par32to27         ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry( 4,1280, 720,370,30, 60000 ,iar16to9  ,vsmProgressive,par1to1           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry( 5,1920,1080,280,22, 60000 ,iar16to9  ,vsmInterlaced ,par1to1           ,vifDualFrameRate  ,0,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry( 6, 720, 480,276,22, 60000 ,iar4to3   ,vsmInterlaced ,par8to9           ,vifDualFrameRate  ,1,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry( 7, 720, 480,276,22, 60000 ,iar16to9  ,vsmInterlaced ,par32to27         ,vifDualFrameRate  ,1,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry( 8, 720, 240,276,22, 60000 ,iar4to3   ,vsmProgressive,par4to9           ,vifDualFrameRate  ,1,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry( 9, 720, 428,276,22, 60000 ,iar16to9  ,vsmProgressive,par16to27         ,vifDualFrameRate  ,1,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(10,2880, 480,552,22, 60000 ,iar4to3   ,vsmInterlaced ,par2to9_20to9     ,vifDualFrameRate  ,0,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(11,2880, 480,552,22, 60000 ,iar16to9  ,vsmInterlaced ,par8to27_80to27   ,vifDualFrameRate  ,0,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(12,2880, 240,552,22, 60000 ,iar4to3   ,vsmProgressive,par1to9_10to9     ,vifDualFrameRate  ,0,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(13,2880, 240,552,22, 60000 ,iar16to9  ,vsmProgressive,par4to27_40to27   ,vifDualFrameRate  ,0,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(14,1440, 480,276,45, 60000 ,iar4to3   ,vsmProgressive,par4to9           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(15,1440, 480,276,45, 60000 ,iar16to9  ,vsmProgressive,par16to27         ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(16,1920,1080,280,45, 60000 ,iar16to9  ,vsmProgressive,par1to1           ,vifDualFrameRate  ,0,0,14850)
        ,Cea861D_VicInfoEntry(17, 720, 576,144,49, 50000 ,iar4to3   ,vsmProgressive,par16to15         ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(18, 720, 576,144,49, 50000 ,iar16to9  ,vsmProgressive,par64to45         ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(19,1280, 720,700,30, 50000 ,iar16to9  ,vsmProgressive,par1to1           ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(20,1920,1080,720,22, 50000 ,iar16to9  ,vsmInterlaced ,par1to1           ,vifSingleFrameRate,0,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(21, 720, 576,288,24, 50000 ,iar4to3   ,vsmInterlaced ,par16to15         ,vifSingleFrameRate,1,1,FILL_THIS_IN)   //(1440)
        ,Cea861D_VicInfoEntry(22, 720, 576,288,24, 50000 ,iar16to9  ,vsmInterlaced ,par64to45         ,vifSingleFrameRate,1,1,FILL_THIS_IN)   //(1440)
        ,Cea861D_VicInfoEntry(23, 720, 288,288,24, 50000 ,iar4to3   ,vsmProgressive,par8to15          ,vifSingleFrameRate,1,2,FILL_THIS_IN)   //(1440)
        ,Cea861D_VicInfoEntry(24, 720, 288,288,24, 50000 ,iar16to9  ,vsmProgressive,par32to45         ,vifSingleFrameRate,1,2,FILL_THIS_IN)   //(1440)
        ,Cea861D_VicInfoEntry(25,2880, 576,576,24, 50000 ,iar4to3   ,vsmInterlaced ,par2to15_20to15   ,vifSingleFrameRate,0,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(26,2880, 576,576,24, 50000 ,iar16to9  ,vsmInterlaced ,par16to45_160to45 ,vifSingleFrameRate,0,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(27,2880, 288,576,24, 50000 ,iar4to3   ,vsmProgressive,par1to15_10to15   ,vifSingleFrameRate,0,2,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(28,2880, 288,576,24, 50000 ,iar16to9  ,vsmProgressive,par8to45_80to45   ,vifSingleFrameRate,0,2,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(29,1440, 576,288,49, 50000 ,iar4to3   ,vsmProgressive,par8to15          ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(30,1440, 576,288,49, 50000 ,iar16to9  ,vsmProgressive,par32to45         ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(31,1920,1080,720,45, 50000 ,iar16to9  ,vsmProgressive,par1to1           ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(32,1920,1080,830,45, 24000 ,iar16to9  ,vsmProgressive,par1to1           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(33,1920,1080,720,45, 25000 ,iar16to9  ,vsmProgressive,par1to1           ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(34,1920,1080,280,45, 30000 ,iar16to9  ,vsmProgressive,par1to1           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(35,2880, 480,552,45, 60000 ,iar4to3   ,vsmProgressive,par2to9           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(36,2880, 480,552,45, 60000 ,iar16to9  ,vsmProgressive,par8to27          ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(37,2880, 576,576,49, 50000 ,iar4to3   ,vsmProgressive,par4to15          ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(38,2880, 576,576,49, 50000 ,iar16to9  ,vsmProgressive,par16to45         ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(39,1920,1080,384,85, 50000 ,iar16to9  ,vsmInterlaced ,par1to1           ,vifSingleFrameRate,0,0,FILL_THIS_IN) /*1250,total*/
        ,Cea861D_VicInfoEntry(40,1920,1080,720,22,100000 ,iar16to9  ,vsmInterlaced ,par1to1           ,vifSingleFrameRate,0,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(41,1280, 720,700,30,100000 ,iar16to9  ,vsmProgressive,par1to1           ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(42, 720, 576,144,49,100000 ,iar4to3   ,vsmProgressive,par16to15         ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(43, 720, 576,144,49,100000 ,iar16to9  ,vsmProgressive,par64to45         ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(44, 720, 576,288,24,100000 ,iar4to3   ,vsmInterlaced ,par16to15         ,vifSingleFrameRate,1,1,FILL_THIS_IN) //(1440)
        ,Cea861D_VicInfoEntry(45, 720, 576,288,24,100000 ,iar16to9  ,vsmInterlaced ,par64to45         ,vifSingleFrameRate,1,1,FILL_THIS_IN) //(1440)
        ,Cea861D_VicInfoEntry(46,1920,1080,280,22,120000 ,iar16to9  ,vsmInterlaced ,par1to1           ,vifDualFrameRate  ,0,1,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(47,1280, 720,370,30,120000 ,iar16to9  ,vsmProgressive,par1to1           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(48, 720, 480,138,45,120000 ,iar4to3   ,vsmProgressive,par8to9           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(49, 720, 480,138,45,120000 ,iar16to9  ,vsmProgressive,par32to27         ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(50, 720, 480,276,22,120000 ,iar4to3   ,vsmInterlaced ,par8to9           ,vifDualFrameRate  ,1,1,FILL_THIS_IN) //(1440)
        ,Cea861D_VicInfoEntry(51, 720, 480,276,22,120000 ,iar16to9  ,vsmInterlaced ,par32to27         ,vifDualFrameRate  ,1,1,FILL_THIS_IN) //(1440)
        ,Cea861D_VicInfoEntry(52, 720, 576,144,49,200000 ,iar4to3   ,vsmProgressive,par16to15         ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(53, 720, 576,144,49,200000 ,iar16to9  ,vsmProgressive,par64to45         ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(54, 720, 576,288,24,200000 ,iar4to3   ,vsmInterlaced ,par16to15         ,vifSingleFrameRate,1,1,FILL_THIS_IN) //(1440)
        ,Cea861D_VicInfoEntry(55, 720, 576,288,24,200000 ,iar16to9  ,vsmInterlaced ,par64to45         ,vifSingleFrameRate,1,1,FILL_THIS_IN) //(1440)
        ,Cea861D_VicInfoEntry(56, 720, 480,138,45,240000 ,iar4to3   ,vsmProgressive,par8to9           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(57, 720, 480,138,45,240000 ,iar16to9  ,vsmProgressive,par32to27         ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(58, 720, 480,276,22,240000 ,iar4to3   ,vsmInterlaced ,par8to9           ,vifDualFrameRate  ,1,1,FILL_THIS_IN) //(1440)

        ,Cea861D_VicInfoEntry(60,1280, 720,370,30, 24000 ,iar16to9  ,vsmProgressive,par1to1           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(61,1280, 720,370,30, 25000 ,iar16to9  ,vsmProgressive,par1to1           ,vifSingleFrameRate,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(62,1280, 720,370,30, 30000 ,iar16to9  ,vsmProgressive,par1to1           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(63,1920,1080,280,45,120000 ,iar16to9  ,vsmProgressive,par1to1           ,vifDualFrameRate  ,0,0,FILL_THIS_IN)
        ,Cea861D_VicInfoEntry(64,1920,1080,280,45,100000 ,iar16to9  ,vsmProgressive,par1to1           ,vifSingleFrameRate  ,0,0,FILL_THIS_IN)
};


typedef struct _HDMI_VIC_Info_t
{
        uint16_t columns;
        uint16_t rows;
        uint32_t fieldRate0InMilliHz,fieldRate1InMilliHz;
        uint32_t pixelClock0,pixelClock1;
} HDMI_VIC_Info_t,*PHDMI_VIC_Info_t;
/*
    union
    {
        uint32_t pixelClock;
    }fieldsOrPixelClock;
*/
#define HDMI_VicInfoEntry(modeNum,columns,rows,FieldRate0,FieldRate1,pixelClock0,pixelClock1) \
                            {columns,rows,FieldRate0,FieldRate1,pixelClock0,pixelClock1} // VIC:modeNum

PLACE_IN_CODE_SEG  HDMI_VIC_Info_t HDMI_VicInfo[]=
{
        HDMI_VicInfoEntry( 0,   0,   0,    0,    0,         0,        0) // place holder
        ,HDMI_VicInfoEntry( 1,3840,2160,30000,29970, 297000000,296703000)
        ,HDMI_VicInfoEntry( 2,3840,2160,25000,25000, 297000000,297000000)
        ,HDMI_VicInfoEntry( 3,3840,2160,24000,23976, 297000000,296703000)
        ,HDMI_VicInfoEntry( 4,4096,2160,24000,24000, 297000000,297000000)
};

uint32_t CalculatePixelClock(uint16_t columns, uint16_t rows, uint32_t verticalSyncFrequencyInMilliHz,uint8_t VIC)
{
        uint32_t pixelClockFrequency;
        uint32_t verticalSyncPeriodInMicroSeconds, verticalActivePeriodInMicroSeconds, verticalBlankPeriodInMicroSeconds;
        uint32_t horizontalSyncFrequencyInHundredthsOfKHz, horizontalSyncPeriodInNanoSeconds, horizontalActivePeriodInNanoSeconds,horizontalBlankPeriodInNanoSeconds;
        PIXCLK_DEBUG_PRINT(("verticalSyncFrequencyInMilliHz: %lu\n",verticalSyncFrequencyInMilliHz ));
        if(verticalSyncFrequencyInMilliHz!=0)
        {
                verticalSyncPeriodInMicroSeconds = 1000000000/verticalSyncFrequencyInMilliHz;
        }
        PIXCLK_DEBUG_PRINT(("verticalSyncFrequencyInMilliHz:%lu verticalSyncPeriodInMicroSeconds: %lu\n",verticalSyncFrequencyInMilliHz,verticalSyncPeriodInMicroSeconds));

        if (0 == VIC)
        {
                // rule of thumb:
                verticalActivePeriodInMicroSeconds = (verticalSyncPeriodInMicroSeconds * 8) / 10;

        }
        else
        {
                uint16_t vTotalInPixels;
                uint16_t vBlankInPixels;

                if (vsmInterlaced == vicInfo[VIC].fields.interlaced)
                {
                        // fix up these values
                        verticalSyncFrequencyInMilliHz /= 2;
                        verticalSyncPeriodInMicroSeconds *= 2;
                        PIXCLK_DEBUG_PRINT(("verticalSyncFrequencyInMilliHz:%lu verticalSyncPeriodInMicroSeconds: %lu\n",verticalSyncFrequencyInMilliHz,verticalSyncPeriodInMicroSeconds));

                        vBlankInPixels = 2 * vicInfo[VIC].VBlankInPixels + vicInfo[VIC].fields.field2VBlank;
                }
                else
                {
                        // when multiple vertical blanking values present,
                        //  allow for higher clocks by calculating maximum possible
                        vBlankInPixels = vicInfo[VIC].VBlankInPixels + vicInfo[VIC].fields.field2VBlank;
                }
                vTotalInPixels = vicInfo[VIC].rows +vBlankInPixels ;
                verticalActivePeriodInMicroSeconds = (verticalSyncPeriodInMicroSeconds * vicInfo[VIC].rows) / vTotalInPixels;

        }
        PIXCLK_DEBUG_PRINT(("verticalActivePeriodInMicroSeconds: %lu\n",verticalActivePeriodInMicroSeconds));

        // rigorous calculation:
        verticalBlankPeriodInMicroSeconds  = verticalSyncPeriodInMicroSeconds - verticalActivePeriodInMicroSeconds;
        PIXCLK_DEBUG_PRINT(("verticalBlankPeriodInMicroSeconds: %lu\n",verticalBlankPeriodInMicroSeconds));

        horizontalSyncFrequencyInHundredthsOfKHz = rows * 100000;
        horizontalSyncFrequencyInHundredthsOfKHz /= verticalActivePeriodInMicroSeconds;

        PIXCLK_DEBUG_PRINT(("horizontalSyncFrequencyInHundredthsOfKHz: %lu\n",horizontalSyncFrequencyInHundredthsOfKHz));

        horizontalSyncPeriodInNanoSeconds    = 100000000 / horizontalSyncFrequencyInHundredthsOfKHz;

        PIXCLK_DEBUG_PRINT(("horizontalSyncPeriodInNanoSeconds: %lu\n",horizontalSyncPeriodInNanoSeconds));

        if (0 == VIC)
        {
                // rule of thumb:
                horizontalActivePeriodInNanoSeconds = (horizontalSyncPeriodInNanoSeconds * 8) / 10;
                PIXCLK_DEBUG_PRINT(("horizontalActivePeriodInNanoSeconds: %lu\n",horizontalActivePeriodInNanoSeconds));
        }
        else
        {
                uint16_t hTotalInPixels;
                uint16_t hClocks;
                hClocks = vicInfo[VIC].columns << vicInfo[VIC].fields.clocksPerPixelShiftCount;
                hTotalInPixels = hClocks + vicInfo[VIC].HBlankInPixels;
                horizontalActivePeriodInNanoSeconds = (horizontalSyncPeriodInNanoSeconds * hClocks) / hTotalInPixels;
        }
        // rigorous calculation:
        horizontalBlankPeriodInNanoSeconds = horizontalSyncPeriodInNanoSeconds - horizontalActivePeriodInNanoSeconds;
        PIXCLK_DEBUG_PRINT(("horizontalBlankPeriodInNanoSeconds: %lu\n",horizontalBlankPeriodInNanoSeconds));

        pixelClockFrequency = columns * (1000000000/ horizontalActivePeriodInNanoSeconds);

        PIXCLK_DEBUG_PRINT(("pixelClockFrequency: %lu\n",pixelClockFrequency));

        return pixelClockFrequency;
}


/*
  IsMhlTimingMode

    MHL has a maximum link clock of 75Mhz.
    For now, we use a rule of thumb regarding
        blanking intervals to calculate a pixel clock,
        then we convert it to a link clock and compare to 75MHz

*/


static uint8_t IsMhlTimingMode(uint16_t columns, uint16_t rows, uint32_t verticalSyncFrequencyInMilliHz,PMhl2VideoDescriptor_t pMhl2VideoDescriptorParm,uint8_t VIC)
{
        uint32_t pixelClockFrequency;
        uint8_t retVal = 0;
        Mhl2VideoDescriptor_t dummy;
        PMhl2VideoDescriptor_t pMhl2VideoDescriptor = pMhl2VideoDescriptorParm;
        if (NULL == pMhl2VideoDescriptorParm)
        {
                dummy.FrameSequential=0;
                dummy.LeftRight=0;
                dummy.TopBottom=0;
                dummy.reservedHigh=0;
                dummy.reservedLow=0;
                pMhl2VideoDescriptor=&dummy;
        }

        pixelClockFrequency = CalculatePixelClock(columns, rows, verticalSyncFrequencyInMilliHz,VIC);

        if (QualifyPixelClockForMhl(pixelClockFrequency, 24))
        {
                AppDisplayTimingEnumerationCallBack(columns, rows, 24, verticalSyncFrequencyInMilliHz,*pMhl2VideoDescriptor);
                retVal = 1;
        }
        if (QualifyPixelClockForMhl(pixelClockFrequency, 16))
        {
                AppDisplayTimingEnumerationCallBack(columns, rows, 16, verticalSyncFrequencyInMilliHz,*pMhl2VideoDescriptor);
                retVal = 1;
        }

        return retVal;
}

typedef struct _TimingModeFromDataSheet_t
{
        uint16_t hTotal;
        uint16_t vTotal;
        uint32_t pixelClock;
        struct
        {
                uint8_t interlaced:1;
                uint8_t reserved:7;
        } flags;
} TimingModeFromDataSheet_t,*PTimingModeFromDataSheet_t;


TimingModeFromDataSheet_t timingModesFromDataSheet[]=
{
        { 800, 525, 25175000,{0,0}}
        ,{1088, 517, 33750000,{0,0}}
        ,{1056, 628, 40000000,{0,0}}
        ,{1344, 806, 65000000,{0,0}}
        ,{1716, 262, 27000000,{1,0}}// DS has VTOTAL for progressive
        ,{1728, 312, 27000000,{1,0}}// DS has VTOTAL for progressive
        ,{ 858, 525, 27000000,{0,0}}
        ,{ 864, 625, 27000000,{0,0}}
        ,{1650, 750, 74250000,{0,0}}
        ,{2200, 562, 74250000,{1,0}}// DS has VTOTAL for progressive
        ,{2750,1125, 74250000,{0,0}}
        ,{2640,1125,148500000,{0,0}}
        ,{2200,1125,148500000,{0,0}}
};

uint32_t SiiMhlTxFindTimingsFromTotals( void )
{
        uint32_t retVal=0;
        uint8_t i;
        uint16_t hTotal, vTotal;
        // Measure the HTOTAL and VTOTAL and look them up in a table
        hTotal = SiiMhlTxDrvGetIncomingHorzTotal();
        vTotal = SiiMhlTxDrvGetIncomingVertTotal();
        for (i = 0 ; i < sizeof(timingModesFromDataSheet)/sizeof(timingModesFromDataSheet[0]); ++i)
        {
                if (timingModesFromDataSheet[i].hTotal == hTotal)
                {
                        if (timingModesFromDataSheet[i].vTotal == vTotal)
                        {
                                retVal = timingModesFromDataSheet[i].pixelClock;
                                break;
                        }
                }
        }
        INFO_DEBUG_PRINT(("VIC was zero maybe caused by AP geting EDID's detail timing!!! hTotal: %d vTotal:%d\n",hTotal,vTotal));
        return retVal;
}
#if 0
static int8_t aviInfoFrameCmp(PAVIInfoFrame_t p0,PAVIInfoFrame_t p1)
{
        uint8_t i;
        uint8_t retVal=0;
        uint8_t *puc0,*puc1;
        uint8_t temp0=0, temp1=0;
        puc0 = (uint8_t *)p0;
        puc1 = (uint8_t *)p1;
        for (i = 0; i < sizeof(*p0); ++i)
        {
                temp0 = *puc0++;
                temp1 = *puc1++;
                if (temp0 == temp1)
                {
                        continue;
                }
                if ( temp0 < temp1)
                {
                        retVal = -1;
                        break;
                }
                if (temp0 > temp1)
                {
                        retVal = 1;
                        break;
                }
        }
        return retVal;
}
#endif
static int8_t vsifCmp(PVendorSpecificInfoFrame_t p0,PVendorSpecificInfoFrame_t p1)
{
        uint8_t length = p0->header.length;
        if (length > p1->header.length)
        {
                return 1;
        }
        else if (length < p1->header.length)
        {
                return -1;
        }
        else
        {
                uint8_t i;
                uint8_t retVal=0;
                uint8_t *puc0,*puc1;
                uint8_t temp0=0, temp1=0;
                length += sizeof(p0->payLoad.checksum) + sizeof(p0->header);
                puc0 = (uint8_t *)p0;
                puc1 = (uint8_t *)p1;
                for (i = 0; i < length; ++i)
                {
                        temp0 = *puc0++;
                        temp1 = *puc1++;
                        if (temp0 == temp1)
                        {
                                continue;
                        }
                        if ( temp0 < temp1)
                        {
                                retVal = -1;
                                break;
                        }
                        if (temp0 > temp1)
                        {
                                retVal = 1;
                                break;
                        }
                }
                return retVal;
        }
}

static int IsValidVendorSpecificInfoFrame(PVendorSpecificInfoFrame_t pVsInfoFrame)
{
        uint8_t checkSum;
        checkSum = CalculateSum((uint8_t *)pVsInfoFrame,0
                                ,sizeof(pVsInfoFrame->header)
                                +sizeof(pVsInfoFrame->payLoad.checksum)
                                +pVsInfoFrame->header.length
                               );
        if (0 != checkSum)
        {
                INFO_DEBUG_PRINT(("VSIF: ERROR!!! VSIF CheckSum: %x\n",(uint16_t)checkSum ));
                return 0;
        }
        // VSIF can be all zeroes
#if 0 //(
        else if (0x81 != pVsInfoFrame->header.type_code)
        {
                INFO_DEBUG_PRINT(("VSIF: ERROR!!! VSIF type code: %x\n",(uint16_t)pVsInfoFrame->header.type_code));
                return 0;
        }
        else if (0x01 != pVsInfoFrame->header.version_number)
        {
                INFO_DEBUG_PRINT(("VSIF: ERROR!!! VSIF version: %x\n",(uint16_t)pVsInfoFrame->header.version_number));
                return 0;
        }
#endif //)
        else
        {
                return 1;
        }
}

///////////////////////////////////////////////////////////////////////////////
//
//  SiiMhlTxAttemptPackedPixel
//
uint8_t	SiiMhlTxAttemptPackedPixel( void )
{
        uint32_t threeDPixelClockRatio = 1;
        uint32_t pixelClockFrequency;
        uint8_t retVal = 0;
        int timingInfoSource=__LINE__;
        PP_DEBUG_PRINT(("Mhl2Tx: SiiMhlTxAttemptPackedPixel\n"));

        packedPixelNeeded=0;
        SiiMhlTxSetPreferredPixelFormat(MHL_STATUS_CLK_MODE_NORMAL);
        if (mhlTxConfig.flags.validVendorSpecificInfoFrame)
        {
                PP_DEBUG_PRINT(("MhlTx: valid VSIF\n"));
                if (hvf3DFormatIndicationPresent == mhlTxConfig.currentVendorSpecificInfoFrame.payLoad.pb4.HDMI_Video_Format)
                {
                        PP_DEBUG_PRINT(("MhlTx: hvf3DFormatIndicationPresent\n"));
                        if (tdsFramePacking ==mhlTxConfig.currentVendorSpecificInfoFrame.payLoad.pb5.ThreeDStructure.threeDStructure)
                        {
                                PP_DEBUG_PRINT(("MhlTx: tdsFramePacking\n"));
                                threeDPixelClockRatio = 2;
                                SiiMhlTxDrvSet3DMode(1,tdsFramePacking);
                        }
                        else
                        {
                                SiiMhlTxDrvSet3DMode(0,0);
                        }
                }
                else
                {
                        SiiMhlTxDrvSet3DMode(0,0);
                }
        }
        else
        {
                SiiMhlTxDrvSet3DMode(0,0);
        }
        if ( !mhlTxConfig.flags.validAviInfoFrame)
        {
                timingInfoSource=__LINE__;
                INFO_DEBUG_PRINT(("unexpected type code:\n"));
                pixelClockFrequency = SiiMhlTxFindTimingsFromTotals();
        }
        else
        {

                INFO_DEBUG_PRINT(("MhlTx: VIC: %d\n",(uint16_t)mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC));

                INFO_DEBUG_PRINT(("\t\t\t\t\t\t\tVIC:%d\n",(uint16_t)mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC));

                if (0 == mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC)
                {
                        PP_DEBUG_PRINT(("Mhl2Tx: Packed Pixel Available checking HDMI VIC:%d\n",(uint16_t)mhlTxConfig.currentVendorSpecificInfoFrame.payLoad.pb5.HDMI_VIC ));
                        // look at the incoming info frame to the the VIC code
                        if (0 == mhlTxConfig.currentVendorSpecificInfoFrame.payLoad.pb5.HDMI_VIC)
                        {
                                timingInfoSource=__LINE__;
                                pixelClockFrequency = SiiMhlTxFindTimingsFromTotals();
                        }
                        else
                        {
                                timingInfoSource=__LINE__;
                                pixelClockFrequency = HDMI_VicInfo[mhlTxConfig.currentVendorSpecificInfoFrame.payLoad.pb5.HDMI_VIC].pixelClock0;
                        }
                }
                else
                {
                        timingInfoSource=__LINE__;
                        PP_DEBUG_PRINT(("Mhl2Tx: Packed Pixel Available checking VIC:%d\n",(uint16_t)mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC));
                        // look at the incoming info frame to the the VIC code
                        pixelClockFrequency = CalculatePixelClock((uint16_t)vicInfo[mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC].columns
                                              , (uint16_t)vicInfo[mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC].rows
                                              , (uint32_t)vicInfo[mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC].fieldRateInMilliHz
                                              ,mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC
                                                                 );
                }
        }
        // make decision about packed pixel mode here
        if (0 == pixelClockFrequency)
        {
                INFO_DEBUG_PRINT(("VIC was zero and we could not identify incoming timing mode,\n"
                                  "\tdefaulting to non-packed pixel\n"
                                  "\tinfo src:%d\n"
                                  "\thTotal: %04x\n"
                                  "\tvTotal: %04x\n\n"
                                  ,timingInfoSource
                                  ,SiiMhlTxDrvGetIncomingHorzTotal()
                                  ,SiiMhlTxDrvGetIncomingVertTotal()
                                 ));
                packedPixelNeeded=0;
        }
        else
        {
                pixelClockFrequency *= threeDPixelClockRatio;
                PP_DEBUG_PRINT(("Mhl2Tx: pixel clock:%lu\n",pixelClockFrequency));


                if (QualifyPixelClockForMhl(pixelClockFrequency, 24))
                {
                        PP_DEBUG_PRINT(("Drv: OK for 24 bit pixels\n"));
                }
                else
                {
                        // not enough bandwidth for uncompressed video
                        PP_DEBUG_PRINT(("Mhl2Tx: YCbCr422 support:%s\n",EDID_Data.YCbCr_4_2_2?"Yes":"No"));
                        if (EDID_Data.YCbCr_4_2_2)
                        {
                                PP_DEBUG_PRINT(("Mhl2Tx: YCbCr422 support!\n"));
                                if (QualifyPixelClockForMhl(pixelClockFrequency, 16))
                                {
                                        // enough for packed pixel
                                        packedPixelNeeded=1;
                                }
                                else
                                {
                                        INFO_DEBUG_PRINT(("Drv: We really can't do this display timing mode\n"));
                                }
                        }
                }
        }
        SiiMhlTxDrvSetPixelClockFrequency(pixelClockFrequency);
        /*
        // check for user override via DIP switches
        if (PlatformGPIOGet(pinPackedPixelUserControl))
        {
        if (PlatformGPIOGet(pinPackedPixelUserOn))
        {
            packedPixelNeeded=1;
        }
        else
        {
            packedPixelNeeded=0;
        }
        }
        */
        SiiMhlTxDrvSetPackedPixelStatus( 0 );
        TXC_DEBUG_PRINT(("PackedPixelAvailable=%d, packedPixelNeeded=%d\n",PackedPixelAvailable,packedPixelNeeded));
        if (PackedPixelAvailable)
        {
                if (packedPixelNeeded)
                {
                        TXC_DEBUG_PRINT(("setting packed pixel mode\n"));
                        SiiMhlTxDrvSetOutputColorSpace( BIT_EDID_FIELD_FORMAT_YCbCr422 );
                        SiiMhlTxSetPreferredPixelFormat(MHL_STATUS_CLK_MODE_PACKED_PIXEL);
                        SiiMhlTxDrvSetPackedPixelStatus( 1 );
                }
        }
        else
        {
                PP_DEBUG_PRINT(("Packed Pixel not available on sink.  Sink's link mode:0x%02x\n",(uint16_t)mhlTxConfig.devCapCache.aucDevCapCache[DEVCAP_OFFSET_VID_LINK_MODE]));
                if (packedPixelNeeded)
                {
                        INFO_DEBUG_PRINT(("Mhl2Tx: Sink does not support packed pixel mode!\n\n"));
                }
        }
        // do these before overriding with packed pixel mode.
        AppNotifyMhlEvent(MHL_TX_EVENT_INFO_FRAME_CHANGE, InfoFrameType_AVI,(void *)&mhlTxConfig.currentAviInfoFrame);

        if (!SiiMhlTxDrvGetPackedPixelStatus())
        {
                uint8_t inputClrSpc;
                // check to see if sink supports the input color space
                //  if it does, then just use it.  If not then take the
                //  next best option
                inputClrSpc = SiiMhlTxDrvGetInputColorSpace();
                switch(inputClrSpc)
                {
                case acsRGB:
                        break;
                case acsYCbCr422:
                        if (!EDID_Data.YCbCr_4_2_2)
                        {
                                if (EDID_Data.YCbCr_4_4_4)
                                {
                                        TXC_DEBUG_PRINT(("setting outputClrSpc to acsYCbCr444\n"));
                                        inputClrSpc = acsYCbCr444;
                                }
                                else
                                {
                                        TXC_DEBUG_PRINT(("setting outputClrSpc to acsRGB\n"));
                                        inputClrSpc = acsRGB;
                                }
                        }
                        break;
                case acsYCbCr444:
                        if (!EDID_Data.YCbCr_4_4_4)
                        {
                                if (EDID_Data.YCbCr_4_2_2)
                                {
                                        TXC_DEBUG_PRINT(("setting outputClrSpc to acsYcbCr422\n"));
                                        inputClrSpc = acsYCbCr422;
                                }
                                else
                                {
                                        TXC_DEBUG_PRINT(("setting outputClrSpc to acsRGB\n"));
                                        inputClrSpc = acsRGB;
                                }
                        }
                        break;
                }
                TXC_DEBUG_PRINT(("setting outputClrSpc to %d\n",(uint16_t)inputClrSpc));
                SiiMhlTxDrvSetOutputColorSpace(inputClrSpc);
        }
        DumpIncomingInfoFrame(&mhlTxConfig.currentAviInfoFrame,sizeof(mhlTxConfig.currentAviInfoFrame));

        // update the info frame to account for packed pixel settings
        SiiHmdiTxLiteDrvPrepareAviInfoframe();

        // output color space value chosen by EDID parser
        //  and input settings from incoming info frame
        SiiMhlTxDrvApplyColorSpaceSettings();

        //if (0 != mhlTxConfig.currentVICfromAviInfoFrame)
        {
                void SiiHdmiTxLiteDrvSetAudioMode (void);
                SiiHdmiTxLiteDrvSetAudioMode();
        }
        SiiMhlTxSendLinkMode();
        retVal = 1;
        return retVal;
}

#if 0
void SiiMhlTxPruneDTDList(P_18ByteDescriptor_u pDesc,uint8_t limit)
{
        uint8_t i;
        uint8_t numberThatWePruned=0;
        TX_PRUNE_PRINT(("limit: %d\n",(uint16_t)limit));
        if (limit)
        {
                for (i = 0 ; i < limit -1 ; ++i)
                {
                        if ((0 != pDesc->dtd.pixelClockLow) || (0 != pDesc->dtd.pixelClockHigh))
                        {
                                if ((0 == pDesc->dtd.horzActive7_0)&&(0 == pDesc->dtd.horzActiveBlankingHigh.horzActive11_8))
                                {
                                        P_18ByteDescriptor_u pHolder=pDesc,pNextDesc = pDesc+1;
                                        uint8_t j;
                                        numberThatWePruned++;
                                        for (j = i+1; j < limit ; ++j)
                                        {
                                                // move the rest of the entries one by one
                                                *pDesc++ = *pNextDesc++;
                                        }
                                        // re-consider the new occupant of the i'th entry on the next iteration
                                        i--;
                                        pDesc=pHolder;
                                }
                        }
                }
                TX_PRUNE_PRINT(("limit: %d\n",(uint16_t)limit));
                // at this point "i" holds the value of mhlTxConfig.svdDataBlockLength-1
                //  and pDesc points to the last entry in the list
                for (; numberThatWePruned >0; --numberThatWePruned,--pDesc)
                {
                        uint8_t *pu8Temp = (uint8_t *)pDesc;
                        uint8_t size;

                        for (size = sizeof(*pDesc); size > 0; --size)
                        {
                                *pu8Temp-- = 0;
                        }
                }
                TX_PRUNE_PRINT(("limit: %d\n",(uint16_t)limit));
        }
}
#else
void SiiMhlTxPruneDTDList(P_18ByteDescriptor_u pDesc,uint8_t limit)
{
        uint8_t i;
        uint8_t numberThatWePruned=0;
        PEDID_Block0_t pEdidBlock0 = (PEDID_Block0_t)&edidBlockData[0];

        TX_PRUNE_PRINT(("limit1: %d\n",(uint16_t)limit));
        DumpEdidBlock(pEdidBlock0,sizeof(*pEdidBlock0));

        if (limit&&(0x20 > mhlTxConfig.devCapCache.aucDevCapCache[DEVCAP_OFFSET_MHL_VERSION]))
        {
                for (i = 0 ; i < limit -1 ; ++i)
                {
                        TX_PRUNE_PRINT(("limit4444: %d,i=%d,(pDesc->dtd.pixelClockHigh):0x%02x,(mhlTxConfig.devCapCache.aucDevCapCache[DEVCAP_OFFSET_MHL_VERSION]):0x%02x\n",
                                        (uint16_t)limit,i,(uint16_t)(pDesc->dtd.pixelClockHigh),(uint16_t)(mhlTxConfig.devCapCache.aucDevCapCache[DEVCAP_OFFSET_MHL_VERSION])));
                        //DumpEdidBlock(pEdidBlock0,sizeof(*pEdidBlock0));

                        if ((0 != pDesc->dtd.pixelClockLow) || (0 != pDesc->dtd.pixelClockHigh))
                        {
                                if ((0 == pDesc->dtd.horzActive7_0)&&(0 == pDesc->dtd.horzActiveBlankingHigh.horzActive11_8))
                                {
                                        P_18ByteDescriptor_u pHolder=pDesc,pNextDesc = pDesc+1;
                                        uint8_t j;
                                        numberThatWePruned++;
                                        for (j = i+1; j < limit ; ++j)
                                        {
                                                TX_PRUNE_PRINT(("limit555555: %d, i=%d, j=%d\n",(uint16_t)limit,i,j));
                                                // move the rest of the entries one by one
                                                *pDesc++ = *pNextDesc++;
                                        }
                                        // re-consider the new occupant of the i'th entry on the next iteration
                                        //i--;
                                        pDesc=pHolder;
                                }
                        }
                        if ((pDesc->dtd.pixelClockHigh>0x20)&&(mhlTxConfig.devCapCache.aucDevCapCache[DEVCAP_OFFSET_MHL_VERSION] < 0x20))
                        {
                                P_18ByteDescriptor_u pNextDesc = pDesc+1;//pHolder=pDesc,
                                uint8_t j;
                                numberThatWePruned++;
                                for (j = i+1; j < limit ; ++j)
                                {
                                        TX_PRUNE_PRINT(("limit6666666: %d,*pDesc:0x%02x\n",(uint16_t)limit,*pDesc));
                                        // move the rest of the entries one by one
                                        *pDesc++ = *pNextDesc++;
                                }
                                // re-consider the new occupant of the i'th entry on the next iteration
                                i--;
                                //pDesc=pHolder;
                        }
                }
                TX_PRUNE_PRINT(("limit2: %d\n",(uint16_t)limit));
                // at this point "i" holds the value of mhlTxConfig.svdDataBlockLength-1
                //  and pDesc points to the last entry in the list
                for (; numberThatWePruned >0; --numberThatWePruned,--pDesc)
                {
                        uint8_t *pu8Temp = (uint8_t *)pDesc;
                        uint8_t size;

                        for (size = sizeof(*pDesc); size > 0; --size)
                        {
                                TX_PRUNE_PRINT(("*pu8Temp: %d\n",(uint16_t)*pu8Temp));
                                *pu8Temp++ = 0;
                        }
                }
                TX_PRUNE_PRINT(("limit3: %d\n",(uint16_t)limit));
        }
}
#endif


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SiiMhlTxParseDetailedTiming()
//
// PURPOSE      :   Parse the detailed timing section of EDID Block 0 and
//                  print their decoded meaning to the screen.
//
// INPUT PARAMS :   Pointer to the array where the data read from EDID
//                  Block0 is stored.
//
//                  Offset to the beginning of the Detailed Timing Descriptor
//                  data.
//
//                                      Block indicator to distinguish between block #0 and blocks
//                                      #2, #3
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   true if valid timing, false if not
//
//////////////////////////////////////////////////////////////////////////////

static bool_t SiiMhlTxParseDetailedTiming(P_18ByteDescriptor_u pDesc , uint8_t Block, uint8_t *pIsTiming,PMhl2VideoDescriptor_t pMhl2VideoDescriptor)
{
        uint8_t TmpByte;
        uint8_t i;
        uint16_t TmpWord;

#ifdef ENABLE_TX_EDID_PRINT //(
        {
                uint8_t *pucTemp=(uint8_t *) pDesc;
                TX_EDID_PRINT(("address: %x detailed timing:",pDesc));
                for (i=0; i < sizeof(*pDesc); ++i)
                {
                        SiiOsDebugPrintAlwaysShort(" %02x",(uint16_t)pucTemp[i]);
                }
                SiiOsDebugPrintAlwaysShort("\n");
        }
#endif //)

        *pIsTiming = 0;
        TmpWord = pDesc->dtd.pixelClockLow + 256 * pDesc->dtd.pixelClockHigh;

        if (TmpWord == 0x00)            // 18 byte partition is used as either for Monitor Name or for Monitor Range Limits or it is unused
        {
                if (Block == EDID_BLOCK_0)      // if called from Block #0 and first 2 bytes are 0 => either Monitor Name or for Monitor Range Limits
                {
                        if (0xFC == pDesc->name.dataTypeTag) // these 13 bytes are ASCII coded monitor name
                        {
                                TX_EDID_PRINT(("Monitor Name: "));

                                for (i = 0; i < 13; i++)
                                {
                                        TX_EDID_PRINT_SIMPLE(("%c", pDesc->name.asciiName[i])); // Display monitor name
                                }
                                TX_EDID_PRINT_SIMPLE(("\n"));
                        }

                        else if (0xFD == pDesc->name.dataTypeTag) // these 13 bytes contain Monitor Range limits, binary coded
                        {
                                TX_EDID_PRINT(("Monitor Range Limits:\n\n"));

                                //i = 0;
                                TX_EDID_PRINT(("Min Vertical Rate in Hz: %d\n"                        , (int)pDesc->rangeLimits.minVerticalRateHz      ));
                                TX_EDID_PRINT(("Max Vertical Rate in Hz: %d\n"                        , (int)pDesc->rangeLimits.maxVerticalRateHz      ));
                                TX_EDID_PRINT(("Min Horizontal Rate in KHz: %d\n"                     , (int)pDesc->rangeLimits.minHorizontalRateKHz   ));
                                TX_EDID_PRINT(("Max Horizontal Rate in KHz: %d\n"                     , (int)pDesc->rangeLimits.maxHorizontalRateKHz   ));
                                TX_EDID_PRINT(("Max Supported pixel clock rate in MHz/10: %d\n"       , (int)pDesc->rangeLimits.maxPixelClockMHzDiv10  ));
                                TX_EDID_PRINT(("Tag for secondary timing formula (00h=not used): %d\n", (int)pDesc->rangeLimits.tagSecondaryFormula    ));
                                TX_EDID_PRINT(("\n"));
                        }
                }

                else if (Block == EDID_BLOCK_2_3)
                {
                        // if called from block #2 or #3 and first 2 bytes are 0x00 (padding) then this
                        // descriptor partition is not used and parsing should be stopped
                        TX_EDID_PRINT(("No More Detailed descriptors in this block\n"));
                        TX_EDID_PRINT(("\n"));
                        return false;
                }
        }

        else // first 2 bytes are not 0 => this is a detailed timing descriptor from either block
        {
                uint32_t pixelClockFrequency;
                uint16_t columns,rows,verticalSyncPeriodInLines;
                uint32_t verticalRefreshRateInMilliHz,horizontalSyncFrequencyInHz,horizontalSyncPeriodInPixels;

                *pIsTiming = 1;

                pixelClockFrequency = (uint32_t)TmpWord * 10000;

                PIXCLK_DEBUG_PRINT(("Pixel Clock: %d.%02d MHz or %ld Hz (0x%lx Hz)\n", TmpWord/100, TmpWord%100,pixelClockFrequency,pixelClockFrequency));

                columns = pDesc->dtd.horzActive7_0 + 256 * pDesc->dtd.horzActiveBlankingHigh.horzActive11_8;
                PIXCLK_DEBUG_PRINT(("Horizontal Active Pixels: %d\n", columns));

                TmpWord = pDesc->dtd.horzBlanking7_0 + 256 * pDesc->dtd.horzActiveBlankingHigh.horzBlanking11_8;
                TX_EDID_PRINT(("Horizontal Blanking (Pixels): %d\n", TmpWord));

                horizontalSyncPeriodInPixels = (uint32_t)columns + (uint32_t)TmpWord;
                horizontalSyncFrequencyInHz = pixelClockFrequency/horizontalSyncPeriodInPixels;

                PIXCLK_DEBUG_PRINT(("horizontal period %lu pixels,  horizontalSyncFrequencyInHz: %lu Hz\n", horizontalSyncPeriodInPixels,horizontalSyncFrequencyInHz));

                rows = pDesc->dtd.vertActive7_0 + 256 * pDesc->dtd.vertActiveBlankingHigh.vertActive11_8;
                PIXCLK_DEBUG_PRINT(("Vertical Active (Lines): %u\n", rows));

                TmpWord = pDesc->dtd.vertBlanking7_0 + 256 * pDesc->dtd.vertActiveBlankingHigh.vertBlanking11_8;
                PIXCLK_DEBUG_PRINT(("Vertical Blanking (Lines): %u\n", TmpWord));

                verticalSyncPeriodInLines = rows + TmpWord;
                verticalRefreshRateInMilliHz = horizontalSyncFrequencyInHz*1000/(verticalSyncPeriodInLines);
                PIXCLK_DEBUG_PRINT(("vertical period %u lines, frequency %lu MilliHz\n", verticalSyncPeriodInLines,verticalRefreshRateInMilliHz));

                TmpWord = pDesc->dtd.horzSyncOffset7_0 + 256 * pDesc->dtd.hsOffsetHsPulseWidthVsOffsetVsPulseWidth.horzSyncOffset9_8;
                TX_EDID_PRINT(("Horizontal Sync Offset (Pixels): %d\n", TmpWord));

                TmpWord = pDesc->dtd.horzSyncPulseWidth7_0 + 256 * pDesc->dtd.hsOffsetHsPulseWidthVsOffsetVsPulseWidth.horzSyncPulseWidth9_8;
                TX_EDID_PRINT(("Horizontal Sync Pulse Width (Pixels): %d\n", TmpWord));

                TmpWord = pDesc->dtd.vertSyncOffsetWidth.vertSyncOffset3_0                             +  16 * pDesc->dtd.hsOffsetHsPulseWidthVsOffsetVsPulseWidth.vertSyncOffset5_4;
                TX_EDID_PRINT(("Vertical Sync Offset (Lines): %d\n", TmpWord));

                TmpWord = pDesc->dtd.vertSyncOffsetWidth.vertSyncPulseWidth3_0 +                       +  16 * pDesc->dtd.hsOffsetHsPulseWidthVsOffsetVsPulseWidth.vertSyncPulseWidth5_4;
                TX_EDID_PRINT(("Vertical Sync Pulse Width (Lines): %d\n", TmpWord));

                TmpWord = pDesc->dtd.horzImageSizemm_7_0                                               + 256 * pDesc->dtd.imageSizeHigh.horzImageSizemm_11_8;
                TX_EDID_PRINT(("Horizontal Image Size (mm): %d\n", TmpWord));

                TmpWord = pDesc->dtd.vertImageSizemm_7_0                                               + 256 * pDesc->dtd.imageSizeHigh.vertImageSizemm_11_8;
                TX_EDID_PRINT(("Vertical Image Size (mm): %d\n", TmpWord));

                TmpByte = pDesc->dtd.horzBorderLines;
                TX_EDID_PRINT(("Horizontal Border (Pixels): %d\n", (int)TmpByte));

                TmpByte = pDesc->dtd.vertBorderPixels;
                TX_EDID_PRINT(("Vertical Border (Lines): %d\n", (int)TmpByte));

                if (pDesc->dtd.flags.interlaced)
                {
                        TX_EDID_PRINT(("Interlaced\n"));
                }
                else
                {
                        TX_EDID_PRINT(("Non-Interlaced\n"));
                }

                switch (pDesc->dtd.flags.stereoBits2_1)
                {
                case 0:
                        TX_EDID_PRINT(("Normal Display, No Stereo\n"));
                        break;
                case 1:
                        if (0 == pDesc->dtd.flags.stereoBit0)
                        {
                                TX_EDID_PRINT(("Field sequential stereo, right image when stereo sync signal == 1\n"));
                        }
                        else
                        {
                                TX_EDID_PRINT(("2-way interleaved stereo, right image on even lines\n"));
                        }
                        break;
                case 2:
                        if (0 == pDesc->dtd.flags.stereoBit0)
                        {
                                TX_EDID_PRINT(("field-sequential stereo, left image when stereo sync signal == 1\n"));
                        }
                        else
                        {
                                TX_EDID_PRINT(("2-way interleaved stereo, left image on even lines.\n"));
                        }
                        break;
                case 3:
                        if (0 == pDesc->dtd.flags.stereoBit0)
                        {
                                TX_EDID_PRINT(("4-way interleaved stereo\n"));
                        }
                        else
                        {
                                TX_EDID_PRINT(("side-by-side interleaved stereo.\n"));
                        }
                        break;
                }

                switch ( pDesc->dtd.flags.syncSignalType )
                {
                case 0x0:
                        TX_EDID_PRINT(("Analog Composite\n"));
                        break;
                case 0x1:
                        TX_EDID_PRINT(("Bipolar Analog Composite\n"));
                        break;
                case 0x2:
                        TX_EDID_PRINT(("Digital Composite\n"));
                        break;
                case 0x3:
                        TX_EDID_PRINT(("Digital Separate\n"));
                        break;
                }
                if (pMhl2VideoDescriptor)
                {
                        uint8_t thisModeDoable=0;
                        if ((thisModeDoable=QualifyPixelClockForMhl(pixelClockFrequency,24)))
                        {
                                AppDisplayTimingEnumerationCallBack(columns, rows, 24, verticalRefreshRateInMilliHz,*pMhl2VideoDescriptor);
                        }

                        if (thisModeDoable |=QualifyPixelClockForMhl(pixelClockFrequency,16))
                        {
                                AppDisplayTimingEnumerationCallBack(columns, rows, 16, verticalRefreshRateInMilliHz,*pMhl2VideoDescriptor);
                        }
                        if (!thisModeDoable)
                        {
                                return false;
                        }
                }
        }
        return true;
}

uint8_t SiiMhlTxParse861LongDescriptors(uint8_t *pEdidBlockData)
{
        PCeaExtension_t pCeaExtension = (PCeaExtension_t)pEdidBlockData;


        if (!pCeaExtension->byteOffsetTo18ByteDescriptors)                         // per CEA-861-D, table 27
        {
                ERROR_DEBUG_PRINT(("EDID -> No Detailed Descriptors\n"));
                return EDID_NO_DETAILED_DESCRIPTORS;
        }
        else
        {
                uint8_t *pucNextBlock;
                uint8_t DescriptorNum = 1;
                union
                {
                        uint8_t *pucDataBlock;
                        P_18ByteDescriptor_u pLongDescriptors;
                } pData_u;

                pData_u.pLongDescriptors= (P_18ByteDescriptor_u)(((uint8_t *)pCeaExtension) + pCeaExtension->byteOffsetTo18ByteDescriptors);
                pucNextBlock = ((uint8_t *)pCeaExtension) + EDID_BLOCK_SIZE;

                while ((uint8_t *)(pData_u.pLongDescriptors + 1) < pucNextBlock)
                {
                        uint8_t isTiming=0;
                        TX_EDID_PRINT(("Parse Results - CEA-861 Long Descriptor #%d:\n", (int) DescriptorNum));
                        TX_EDID_PRINT(("===============================================================\n"));

                        if (!SiiMhlTxParseDetailedTiming(pData_u.pLongDescriptors, EDID_BLOCK_2_3,&isTiming,NULL))
                        {
                                break;
                        }
                        pData_u.pLongDescriptors++;
                        DescriptorNum++;
                }

                return EDID_LONG_DESCRIPTORS_OK;
        }

}


void SiiMhlTxPruneEDID(void)
{
        PEDID_Block0_t pEdidBlock0 = (PEDID_Block0_t)&edidBlockData[0];
        uint8_t dtdLimit;
        PCeaExtension_t pCeaExtension = (PCeaExtension_t)&edidBlockData[EDID_BLOCK_SIZE];
        PBlockMap_t pBlockMap = NULL;
        uint8_t *pbLimit;
        union
        {
                uint8_t *pucDataBlock;
                P_18ByteDescriptor_u pLongDescriptors;
        } pData_u;

        if (EDID_EXTENSION_BLOCK_MAP == pCeaExtension->tag)
        {
                // save to overwrite later
                pBlockMap = (PBlockMap_t)pCeaExtension;

                //advance to next block
                pCeaExtension++;
        }
        pbLimit = (uint8_t *)(pCeaExtension+1);

        ERROR_DEBUG_PRINT(("Parse and Prune EDID\n"));
        pData_u.pucDataBlock = (uint8_t *)pCeaExtension + pCeaExtension->byteOffsetTo18ByteDescriptors;

        DumpEdidBlock(pEdidBlock0,sizeof(*pEdidBlock0))  // no semicolon needed here
        // zero out checksums before modifying data
        pCeaExtension->checkSum=0;
        pEdidBlock0->checkSum = 0;

        TX_PRUNE_PRINT(("Is there a vsdb?\n"));
        // Is there an HDMI VSDB?
        if (mhlTxConfig.pHdmiVendorSpecificDataBlock)
        {
                PHdmiLLCVendorSpecificDataBlockPayload_t pHdmiVendorSpecificPayload = &mhlTxConfig.pHdmiVendorSpecificDataBlock->payload_u.HdmiLLC;
                uint8_t *pNextDB = (uint8_t *)pHdmiVendorSpecificPayload +mhlTxConfig.pHdmiVendorSpecificDataBlock->header.lengthFollowingHeader;
                TX_PRUNE_PRINT(("mhlTxConfig.pHdmiVendorSpecificDataBlock->header.lengthFollowingHeader=0x%x\n",
                                mhlTxConfig.pHdmiVendorSpecificDataBlock->header.lengthFollowingHeader));

                TX_PRUNE_PRINT(("Is deep color info provided?\n"));
                // if deep color information is provided...
                if (((uint8_t *)&pHdmiVendorSpecificPayload->byte6) < pNextDB)
                {
                        TX_PRUNE_PRINT(("Clearing deep color bits\n"));
                        pHdmiVendorSpecificPayload->byte6.DC_Y444 =0;
                        pHdmiVendorSpecificPayload->byte6.DC_30bit=0;
                        pHdmiVendorSpecificPayload->byte6.DC_36bit=0;
                        pHdmiVendorSpecificPayload->byte6.DC_48bit=0;
                }
        }
        // prune the DTDs in block 0
        dtdLimit = sizeof(pEdidBlock0->detailedTimingDescriptors)/sizeof(pEdidBlock0->detailedTimingDescriptors[0]);
        SiiMhlTxPruneDTDList((P_18ByteDescriptor_u)&pEdidBlock0->detailedTimingDescriptors[0],dtdLimit);

        DumpEdidBlock(pEdidBlock0,sizeof(*pEdidBlock0))  // no semicolon needed here

        DumpEdidBlock(pCeaExtension,sizeof(*pCeaExtension))  // no semicolon needed here
        // prune the DTDs in the CEA-861D extension
        dtdLimit = (uint8_t)pCeaExtension->version_u.version3.miscSupport.totalNumberDetailedTimingDescriptorsInEntireEDID;
        SiiMhlTxPruneDTDList(&pData_u.pLongDescriptors[0],dtdLimit);

        // adjust the mask according to which 2D VICs were set to zero
        if (mhlTxConfig.p3DMask)
        {
                uint8_t lowerMask;
                uint32_t mask32;
                int8_t index = mhlTxConfig.pVideoDataBlock2D[0]->header.lengthFollowingHeader-1;
                index = (index > 15) ? 15 : index;

                mask32 = 0xFFFF00 >> (15 - index);
                lowerMask = (index > 7) ? 0x7F : (0x7F >> (7 - index));

                TX_PRUNE_PRINT(("3d mask 15..8: 0x%02x 0:0x%x\n",(uint16_t)mhlTxConfig.p3DMask->_3D_Mask_15_8,(uint16_t)0));
                for (
                        ; index >= 8
                        ; mask32>>=1,lowerMask >>=1, --index)
                {
                        if (0 == mhlTxConfig.pVideoDataBlock2D[0]->shortDescriptors[index].VIC)
                        {
                                uint8_t lowerBits,upperBits;
                                uint8_t upperMask;
                                upperMask = (uint8_t)mask32;

                                // preserve the lower bits
                                lowerBits = lowerMask  &  mhlTxConfig.p3DMask->_3D_Mask_15_8;

                                // and out the bit in question
                                upperBits = upperMask  &  mhlTxConfig.p3DMask->_3D_Mask_15_8;

                                // adjust the positions of the upper bits
                                upperBits >>=1;

                                mhlTxConfig.p3DMask->_3D_Mask_15_8 = lowerBits | upperBits;
                                TX_PRUNE_PRINT(("3d mask 15..8: 0x%02x\n",(uint16_t)mhlTxConfig.p3DMask->_3D_Mask_15_8));
                        }
                }
                TX_PRUNE_PRINT(("3d mask 7..0: 0x%02x\n",(uint16_t)mhlTxConfig.p3DMask->_3D_Mask_7_0));
                lowerMask =   0x7F >> (7 - index);
                for (
                        ; index >= 0
                        ; mask32>>=1,lowerMask >>=1, --index)
                {
                        if (0 == mhlTxConfig.pVideoDataBlock2D[0]->shortDescriptors[index].VIC)
                        {
                                uint8_t lowerBits,upperBits;
                                uint8_t upperMask;
                                upperMask = (uint8_t)mask32;

                                // preserve the lower bits
                                lowerBits = lowerMask  &  mhlTxConfig.p3DMask->_3D_Mask_7_0;

                                // and out the bit in question
                                upperBits = upperMask  &  mhlTxConfig.p3DMask->_3D_Mask_7_0;

                                // adjust the positions of the upper bits
                                upperBits >>=1;

                                mhlTxConfig.p3DMask->_3D_Mask_7_0 = lowerBits | upperBits;
                                TX_PRUNE_PRINT(("3d mask 7..0: 0x%02x\n",(uint16_t)mhlTxConfig.p3DMask->_3D_Mask_7_0));
                        }
                }
        }

        if (mhlTxConfig.pThreeD)
        {
                uint8_t num3DStructureBytesPruned=0;
                union
                {
                        P_3DStructureAndDetailEntry_u            pThreeD;
                        P_3DStructureAndDetailEntrySansByte1_t   pSansByte1;
                        P_3DStructureAndDetailEntryWithByte1_t   pWithByte1;
                        uint8_t                                 *pAsBytes;
                } pThreeD_u;

                uint32_t deletionMask=0;
                uint8_t limit2dVIC = mhlTxConfig.pVideoDataBlock2D[0]->header.lengthFollowingHeader;
                // make a bitmap of the positions of the VICs that are zero
                {
                        uint8_t i;
                        uint32_t thisBit;
                        for (i =0,thisBit=1; i < limit2dVIC; ++i,thisBit<<=1)
                        {
                                uint8_t VIC;
                                VIC = mhlTxConfig.pVideoDataBlock2D[0]->shortDescriptors[i].VIC;
                                if (0 == VIC)
                                {
                                        // set the bit that corresponds to the VIC that was set to zero
                                        deletionMask |= thisBit;
                                        TX_PRUNE_PRINT(("vic: 0x%02x deletionMask:0x%04x%04x thisBit:0x%04x%04x\n"
                                                        ,(uint16_t)VIC
                                                        ,(uint16_t)(deletionMask>>16)
                                                        ,(uint16_t)(deletionMask&0xFFFF)
                                                        ,(uint16_t)(thisBit>>16)
                                                        ,(uint16_t)(thisBit&0xFFFF)
                                                       ));
                                }
                        }
                }
                pThreeD_u.pThreeD = mhlTxConfig.pThreeD;

                while ( pThreeD_u.pAsBytes < mhlTxConfig.p3DLimit)
                {
                        uint8_t _2D_VIC_order           = pThreeD_u.pSansByte1->byte0._2D_VIC_order;
                        ThreeDStructure_e _3D_Structure = pThreeD_u.pSansByte1->byte0._3D_Structure;
                        uint8_t VIC;
                        VIC = mhlTxConfig.pVideoDataBlock2D[0]->shortDescriptors[_2D_VIC_order].VIC;
                        if (0 == VIC)
                        {
                                // delete this 3D_Structure/3D_Detail information
                                uint8_t *pSrc,*pDest=pThreeD_u.pAsBytes;

                                if (_3D_Structure < tdsSideBySide)
                                {
                                        pSrc = (uint8_t *)(pThreeD_u.pSansByte1+1);
                                        num3DStructureBytesPruned+=sizeof(*pThreeD_u.pSansByte1);
                                }
                                else
                                {
                                        pSrc = (uint8_t *)(pThreeD_u.pWithByte1+1);
                                        num3DStructureBytesPruned+=sizeof(*pThreeD_u.pWithByte1);
                                }

                                while (pSrc < pbLimit)
                                {
                                        *pDest++=*pSrc++;
                                }
                                while (pDest < pbLimit)
                                {
                                        *pDest++=0;
                                }
                        }
                        else
                        {
                                uint8_t i;
                                uint8_t limit = _2D_VIC_order;
                                uint32_t thisBit;
                                TX_PRUNE_PRINT(("2D vic order: 0x%02x deletionMask:0x%04x%04x\n"
                                                ,(uint16_t)_2D_VIC_order
                                                ,(uint16_t)(deletionMask>>16)
                                                ,(uint16_t)(deletionMask&0xFFFF)
                                               ));

                                for (i = 0,thisBit=1; i < limit; ++i,thisBit<<=1)
                                {
                                        if (thisBit & deletionMask)
                                        {
                                                _2D_VIC_order--;
                                        }
                                }
                                pThreeD_u.pSansByte1->byte0._2D_VIC_order = _2D_VIC_order;
                                TX_PRUNE_PRINT(("2D vic order: 0x%02x thisBit:0x%04x%04x\n"
                                                ,(uint16_t)_2D_VIC_order
                                                ,(uint16_t)(thisBit>>16)
                                                ,(uint16_t)(thisBit&0xFFFF)
                                               ));


                                if (_3D_Structure < tdsSideBySide)
                                {
                                        pThreeD_u.pSansByte1++;
                                }
                                else
                                {
                                        pThreeD_u.pWithByte1++;
                                }
                        }
                }
                TX_PRUNE_PRINT(("num3DStructureBytesPruned:0x%x "
                                "byte14: 0x%02x "
                                "offset to DTDs: 0x%x "
                                "vsdb header: 0x%x\n"
                                ,(uint16_t)num3DStructureBytesPruned
                                ,(uint16_t)*((uint8_t *)&mhlTxConfig.pByte13ThroughByte15->byte14)
                                ,(uint16_t)pCeaExtension->byteOffsetTo18ByteDescriptors
                                ,(uint16_t)*((uint8_t *)&mhlTxConfig.pHdmiVendorSpecificDataBlock->header)
                               ));

                mhlTxConfig.pByte13ThroughByte15->byte14.HDMI_3D_LEN                   -= num3DStructureBytesPruned;
                pCeaExtension->byteOffsetTo18ByteDescriptors                           -= num3DStructureBytesPruned;
                mhlTxConfig.pHdmiVendorSpecificDataBlock->header.lengthFollowingHeader -= num3DStructureBytesPruned;

                TX_PRUNE_PRINT(("num3DStructureBytesPruned:0x%x "
                                "byte14: 0x%02x "
                                "offset to DTDs: 0x%x "
                                "vsdb header: 0x%x\n"
                                ,(uint16_t)num3DStructureBytesPruned
                                ,(uint16_t)*((uint8_t *)&mhlTxConfig.pByte13ThroughByte15->byte14)
                                ,(uint16_t)pCeaExtension->byteOffsetTo18ByteDescriptors
                                ,(uint16_t)*((uint8_t *)&mhlTxConfig.pHdmiVendorSpecificDataBlock->header)
                               ));
        }
        // now prune the HDMI VSDB VIC list
        if (mhlTxConfig.pByte13ThroughByte15)
        {
                uint8_t lengthVIC= mhlTxConfig.pByte13ThroughByte15->byte14.HDMI_VIC_LEN;

                if (0 ==lengthVIC)
                {
                        TX_PRUNE_PRINT(("lengthVIC:%d \n",(uint16_t)lengthVIC));
                }
                else
                {
                        uint8_t i,numHdmiVicsPruned=0;
                        uint8_t innerLoopLimit;
                        uint8_t outerLoopLimit;
                        innerLoopLimit = lengthVIC;
                        outerLoopLimit = lengthVIC - 1;
                        TX_PRUNE_PRINT(("lengthVIC:%d innerLoopLimit: %d outerLoopLimit: %d \n",(uint16_t)lengthVIC,(uint16_t)innerLoopLimit,(uint16_t)outerLoopLimit));
                        for (i=0; i < outerLoopLimit;)
                        {
                                if (0 == mhlTxConfig.pByte13ThroughByte15->vicList[i])
                                {
                                        uint8_t j,prev;
                                        for (prev=i,j = i+1; j < innerLoopLimit; ++j,++prev)
                                        {
                                                uint16_t VIC0,VIC1;

                                                VIC0 = mhlTxConfig.pByte13ThroughByte15->vicList[prev];
                                                VIC1 = mhlTxConfig.pByte13ThroughByte15->vicList[j];
                                                TX_PRUNE_PRINT(("replacing VIC: %3d at index: %3d with VIC:%3d from index: %3d \n"
                                                                ,VIC0
                                                                ,(uint16_t)prev
                                                                ,VIC1
                                                                ,(uint16_t)j
                                                               ));
                                                mhlTxConfig.pByte13ThroughByte15->vicList[prev]
                                                        = mhlTxConfig.pByte13ThroughByte15->vicList[j];
                                        }
                                        // we just removed one
                                        numHdmiVicsPruned++;
                                        innerLoopLimit--;
                                        outerLoopLimit--;
                                }
                                else
                                {
                                        // this mode is doable on MHL, so move on to the next index
                                        ++i;
                                }
                        }
                        // check the last one
                        if (0 == mhlTxConfig.pByte13ThroughByte15->vicList[outerLoopLimit])
                        {
                                numHdmiVicsPruned++;
                                innerLoopLimit--;
                        }

                        DumpEdidBlock(pCeaExtension,sizeof(*pCeaExtension))  // no semicolon needed here
                        //now move all other data up
                        if (numHdmiVicsPruned)
                        {
                                uint8_t *pbDest  = (uint8_t *)&mhlTxConfig.pByte13ThroughByte15->vicList[innerLoopLimit];
                                uint8_t *pbSrc   = (uint8_t *)&mhlTxConfig.pByte13ThroughByte15->vicList[lengthVIC];

                                SII_ASSERT(EDID_BLOCK_SIZE==sizeof(*pCeaExtension),("\n\n unexpected extension size\n\n"));
                                while(pbSrc < pbLimit)
                                {
                                        //TX_PRUNE_PRINT(("moving data up %02x(0x%02x) <- %02x(0x%02x)\n",pbDest,(uint16_t)*pbDest,pbSrc,(uint16_t)*pbSrc));
                                        *pbDest++=*pbSrc++;
                                }

                                while(pbDest < pbLimit)
                                {
                                        TX_PRUNE_PRINT(("clearing data %02x <- 0\n",pbDest));
                                        *pbDest++=0;
                                }

                        }
                        mhlTxConfig.pByte13ThroughByte15->byte14.HDMI_VIC_LEN = innerLoopLimit;
                        pCeaExtension->byteOffsetTo18ByteDescriptors         -= numHdmiVicsPruned;
                        if (mhlTxConfig.pHdmiVendorSpecificDataBlock)
                        {
                                mhlTxConfig.pHdmiVendorSpecificDataBlock->header.lengthFollowingHeader -= numHdmiVicsPruned;
                        }
                }
        }

        // Now prune the SVD list and move the CEA 861-D data blocks and DTDs up
        {
                uint8_t i,numCeaVicsPruned=0;
#ifdef ENABLE_TX_PRUNE_PRINT //(
                char *pszNative="Native";
                char *pszNonNative="";
#endif //)
                int8_t vdb_index = mhlTxConfig.num_video_data_blocks-1;
                // collapse each VDB
                for (; vdb_index >=0; --vdb_index)
                {
                        uint8_t innerLoopLimit = mhlTxConfig.pVideoDataBlock2D[vdb_index]->header.lengthFollowingHeader;
                        if (innerLoopLimit)
                        {
                                uint8_t outerLoopLimit = innerLoopLimit-1;
                                TX_PRUNE_PRINT(("outerLoopLimit:%d innerLoopLimit:%d\n"
                                                , (uint16_t)outerLoopLimit
                                                , (uint16_t)innerLoopLimit
                                               ));
                                for (i=0; i < outerLoopLimit;)
                                {
                                        if (0 == mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[i].VIC)
                                        {
                                                uint8_t j,prev;
                                                numCeaVicsPruned++;
                                                for (prev=i,j = i+1; j < innerLoopLimit; ++j,++prev)
                                                {
                                                        uint16_t VIC0,VIC1;

                                                        VIC0 = mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[prev].VIC;
                                                        VIC1 = mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[j].VIC;
                                                        TX_PRUNE_PRINT(("replacing SVD:%6s %3d at index: %3d with SVD:%6s %3d from index: %3d \n"
                                                                        ,mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[prev].native?pszNative:pszNonNative
                                                                        ,VIC0
                                                                        ,(uint16_t)prev
                                                                        ,mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[j].native?pszNative:pszNonNative
                                                                        ,VIC1
                                                                        ,(uint16_t)j
                                                                       ));
                                                        mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[prev]
                                                                = mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[j];
                                                }
                                                // we just removed one
                                                innerLoopLimit--;
                                                outerLoopLimit--;
                                                TX_PRUNE_PRINT(("outerLoopLimit:%d innerLoopLimit:%d\n"
                                                                , (uint16_t)outerLoopLimit
                                                                , (uint16_t)innerLoopLimit
                                                               ));
                                        }
                                        else
                                        {
                                                // this mode is doable on MHL, so move on to the next index
                                                ++i;
                                        }
                                }
                                // check the last one
                                if (0 == mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[outerLoopLimit].VIC)
                                {
                                        numCeaVicsPruned++;
                                        innerLoopLimit--;
                                        mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[outerLoopLimit].native=0;
                                }


                                DumpEdidBlock(pCeaExtension,sizeof(*pCeaExtension))  // no semicolon needed here

                                //now move all other data up
                                {
                                        uint8_t *pbDest = (uint8_t *)&mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[innerLoopLimit];
                                        uint8_t *pbSrc= (uint8_t *)&mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[mhlTxConfig.pVideoDataBlock2D[vdb_index]->header.lengthFollowingHeader];

                                        SII_ASSERT(EDID_BLOCK_SIZE==sizeof(*pCeaExtension),("\n\n unexpected extension size\n\n"));
                                        while(pbSrc < pbLimit)
                                        {
                                                //size_t stSrc;
                                                //  stSrc = pbSrc;
                                                /*
                                                  TX_PRUNE_PRINT(("moving data up pointer  size:%d %x(0x%02x) <- %x(0x%02x)\n"
                                                      ,(unsigned int)sizeof(pbSrc)
                                                      ,(unsigned int)pbDest
                                                      ,(uint16_t)(*pbDest)
                                                      ,(unsigned int)pbSrc
                                                      ,(uint16_t)(*pbSrc)
                                                      ));*/
                                                *pbDest++=*pbSrc++;
                                        }

                                        while(pbDest < pbLimit)
                                        {
                                                TX_PRUNE_PRINT(("clearing data %02x <- 0\n",pbDest));
                                                *pbDest++=0;
                                        }

                                }
                                TX_PRUNE_PRINT(("CEA-861-D DTDs began at 0x%02x"
                                                "CEA-861-D SVD count: 0x%x\n"
                                                ,(uint16_t)pCeaExtension->byteOffsetTo18ByteDescriptors
                                                ,(uint16_t)mhlTxConfig.pVideoDataBlock2D[vdb_index]->header.lengthFollowingHeader
                                               ));

                                pCeaExtension->byteOffsetTo18ByteDescriptors               -= numCeaVicsPruned;
                                mhlTxConfig.pVideoDataBlock2D[vdb_index]->header.lengthFollowingHeader = innerLoopLimit;

                                TX_PRUNE_PRINT(("CEA-861-D DTDs now begin at 0x%02x"
                                                "CEA-861-D SVD count: 0x%x\n"
                                                ,(uint16_t)pCeaExtension->byteOffsetTo18ByteDescriptors
                                                ,(uint16_t)mhlTxConfig.pVideoDataBlock2D[vdb_index]->header.lengthFollowingHeader
                                               ));

                                DumpEdidBlock(pCeaExtension,sizeof(*pCeaExtension))  // no semicolon needed here
                        }
                }
        }

        // re-compute the checksum(s)
        SII_ASSERT(EDID_BLOCK_SIZE==sizeof(*pEdidBlock0),("\n\n unexpected size for block 0\n\n"));
        SII_ASSERT(EDID_BLOCK_SIZE==sizeof(*pCeaExtension),("\n\n unexpected size for  CEA extension\n\n"));

        if (pBlockMap)
        {
                PCeaExtension_t pCeaExtensionDest=(PCeaExtension_t)pBlockMap;
                *pCeaExtensionDest = *pCeaExtension;
        }

        pEdidBlock0->checkSum = CalculateGenericCheckSum((uint8_t *)pEdidBlock0,0,sizeof(*pEdidBlock0));

        pCeaExtension->checkSum=CalculateGenericCheckSum((uint8_t *)pCeaExtension,0,sizeof(*pCeaExtension));


        DumpEdidBlock(pCeaExtension,sizeof(*pCeaExtension))  // no semicolon needed here

        // parse the new block;
        //SiiMhlTxParse861Block((uint8_t *)pCeaExtension);
#ifndef EDID_PASSTHROUGH //(
        if (0 == SiiMhlTxDrvSetUpstreamEDID(edidBlockData,2*EDID_BLOCK_SIZE))
#endif //)
        {
                SetMiscFlag(SiiMhlTxPruneEDID,FLAGS_EDID_READ_DONE)
                //ClrMhlTxFlag(syncDetect)
                SiiMhlTxTmdsEnable();
        }
}

/*
*/
static uint8_t IsQualifiedMhlVIC(uint8_t VIC,PMhl2VideoDescriptor_t pMhlVideoDescriptor)
{
        uint8_t retVal=0;
        if (VIC > 0)
        {
                retVal= IsMhlTimingMode(vicInfo[VIC].columns, vicInfo[VIC].rows, vicInfo[VIC].fieldRateInMilliHz,pMhlVideoDescriptor,VIC);
                if (vifDualFrameRate == vicInfo[VIC].fields.frameRateInfo)
                {
                        uint32_t fieldRateInMilliHz;
                        switch(vicInfo[VIC].fieldRateInMilliHz)
                        {
                        case 24000: //23.97
                                fieldRateInMilliHz = 23970;
                                break;

                        case 30000: //
                                fieldRateInMilliHz = 29970;
                                break;

                        case 60000: //
                                fieldRateInMilliHz = 59940;
                                break;

                        case 120000: //
                                fieldRateInMilliHz = 119880;
                                break;

                        case 240000: //
                                fieldRateInMilliHz = 239760;
                                break;

                        default: //error or unknown case
                                fieldRateInMilliHz=0;
                                TXC_DEBUG_PRINT(("fieldRateInMilliHz=0;" ));
                                break;
                        }
                        retVal |= IsMhlTimingMode(vicInfo[VIC].columns, vicInfo[VIC].rows, fieldRateInMilliHz,pMhlVideoDescriptor,VIC);
                }
        }
        return retVal;
}

/*
*/
static uint8_t IsQualifiedMhlHdmiVIC(uint8_t VIC,PMhl2VideoDescriptor_t pMhl2VideoDescriptor)
{
        uint8_t retVal=0;

        if (QualifyPixelClockForMhl(HDMI_VicInfo[VIC].pixelClock0, 24))
        {
                AppDisplayTimingEnumerationCallBack(HDMI_VicInfo[VIC].columns, HDMI_VicInfo[VIC].rows, 24, HDMI_VicInfo[VIC].fieldRate0InMilliHz,*pMhl2VideoDescriptor);
                retVal = 1;
        }
        if (QualifyPixelClockForMhl(HDMI_VicInfo[VIC].pixelClock0, 16))
        {
                AppDisplayTimingEnumerationCallBack(HDMI_VicInfo[VIC].columns, HDMI_VicInfo[VIC].rows, 16, HDMI_VicInfo[VIC].fieldRate0InMilliHz,*pMhl2VideoDescriptor);
                retVal = 1;
        }
        if (HDMI_VicInfo[VIC].pixelClock0 != HDMI_VicInfo[VIC].pixelClock1)
        {
                if (QualifyPixelClockForMhl(HDMI_VicInfo[VIC].pixelClock1, 24))
                {
                        AppDisplayTimingEnumerationCallBack(HDMI_VicInfo[VIC].columns, HDMI_VicInfo[VIC].rows, 24, HDMI_VicInfo[VIC].fieldRate1InMilliHz,*pMhl2VideoDescriptor);
                        retVal = 1;
                }
                if (QualifyPixelClockForMhl(HDMI_VicInfo[VIC].pixelClock1, 16))
                {
                        AppDisplayTimingEnumerationCallBack(HDMI_VicInfo[VIC].columns, HDMI_VicInfo[VIC].rows, 16, HDMI_VicInfo[VIC].fieldRate1InMilliHz,*pMhl2VideoDescriptor);
                        retVal = 1;
                }
        }
        return retVal;
}

void SiiMhlTxEnumerateHdmiVsdb(void)
{
        int8_t vdb_index=0;
        if (mhlTxConfig.pHdmiVendorSpecificDataBlock)
        {
                PHdmiLLCVendorSpecificDataBlockPayload_t pHdmiVendorSpecificPayload = &mhlTxConfig.pHdmiVendorSpecificDataBlock->payload_u.HdmiLLC;
                uint8_t *pNextDB = (uint8_t *)pHdmiVendorSpecificPayload +mhlTxConfig.pHdmiVendorSpecificDataBlock->header.lengthFollowingHeader;
                // if 3D_present field is included
                if (mhlTxConfig.pByte13ThroughByte15)
                {
                        if (((uint8_t *)&mhlTxConfig.pByte13ThroughByte15->byte13) < pNextDB)
                        {
                                uint8_t hdmi3D_present          = mhlTxConfig.pByte13ThroughByte15->byte13._3D_present;
                                uint8_t hdmi_3D_Multi_present   = mhlTxConfig.pByte13ThroughByte15->byte13._3D_Multi_present;
                                // re-visit uint8_t Image_Size              = mhlTxConfig.pByte13ThroughByte15->byte13.Image_Size;

                                // if HDMI_VIC_LEN is present...
                                if (((uint8_t *)&mhlTxConfig.pByte13ThroughByte15->byte14) < pNextDB)
                                {
                                        uint8_t lengthVIC;
                                        uint8_t index;
                                        Mhl2VideoDescriptor_t mhl2VideoDescriptor;
                                        lengthVIC =  mhlTxConfig.pByte13ThroughByte15->byte14.HDMI_VIC_LEN;
                                        mhl2VideoDescriptor.LeftRight      = 0;
                                        mhl2VideoDescriptor.TopBottom      = 0;
                                        mhl2VideoDescriptor.FrameSequential= 0;
                                        for (index = 0; index < lengthVIC; ++index)
                                        {
                                                uint8_t VIC;

                                                VIC = mhlTxConfig.pByte13ThroughByte15->vicList[index];
                                                if (!IsQualifiedMhlHdmiVIC(VIC,&mhl2VideoDescriptor))
                                                {
                                                        TX_PRUNE_PRINT(("'can't do HDMI VIC:%d\n",(uint16_t)VIC));
                                                        mhlTxConfig.pByte13ThroughByte15->vicList[index] = 0;
                                                }
                                        }
                                        if (hdmi3D_present)
                                        {
                                                uint8_t length3D;
                                                PHdmi3DSubBlock_t pThree3DSubBlock= (PHdmi3DSubBlock_t)&mhlTxConfig.pByte13ThroughByte15->vicList[lengthVIC];
                                                union
                                                {
                                                        P_3DStructureAndDetailEntry_u            pThreeD;
                                                        P_3DStructureAndDetailEntrySansByte1_t   pSansByte1;
                                                        P_3DStructureAndDetailEntryWithByte1_t   pWithByte1;
                                                        uint8_t                                 *pAsBytes;
                                                } pThreeD_u;
                                                uint8_t limit;
                                                pThreeD_u.pThreeD=NULL;
                                                length3D  =  mhlTxConfig.pByte13ThroughByte15->byte14.HDMI_3D_LEN;
                                                limit =mhlTxConfig.pVideoDataBlock2D[vdb_index]->header.lengthFollowingHeader;
                                                // only do the first 16
                                                limit = (limit > 16)?16:limit;
                                                switch(hdmi_3D_Multi_present)
                                                {
                                                case 0x00:
                                                        //3D_Structure_ALL_15..0 and 3D_MASK_15..0 fields are not present
                                                        pThreeD_u.pThreeD = &pThree3DSubBlock->hdmi3DSubBlockSansAllAndMask._3DStructureAndDetailList[0];
                                                        break;
                                                case 0x01:
                                                        /*
                                                            3D_Structure_ALL_15..0 is present and assigns 3D formats
                                                                to all of the VICs listed in the first 16 entries in the EDID
                                                            3D_Mask_15..0 is not present
                                                        */
                                                {
                                                        P_3D_StructureAll_t p3DStructureAll=(P_3D_StructureAll_t)&mhlTxConfig.pByte13ThroughByte15->vicList[lengthVIC];
                                                        mhl2VideoDescriptor.LeftRight      = p3DStructureAll->_3D_Structure_ALL_7_0.sideBySide;
                                                        mhl2VideoDescriptor.TopBottom      = p3DStructureAll->_3D_Structure_ALL_15_8.topBottom;
                                                        mhl2VideoDescriptor.FrameSequential= p3DStructureAll->_3D_Structure_ALL_15_8.framePacking;
                                                        for (index = 0; index < limit; ++index)
                                                        {
                                                                uint8_t VIC;

                                                                VIC = mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[index].VIC;
                                                                if (VIC)
                                                                {
                                                                        if (!IsQualifiedMhlVIC(VIC,&mhl2VideoDescriptor))
                                                                        {
                                                                                mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[index].VIC=0;
                                                                        }
                                                                }
                                                        }
                                                        length3D -= sizeof(*p3DStructureAll);
                                                }
                                                pThreeD_u.pThreeD = &pThree3DSubBlock->hdmi3DSubBlockSansMask._3DStructureAndDetailList[0];
                                                break;
                                                case 0x02:
                                                        /*
                                                            3D_Structure_ALL_15..0 and 3D_Mask_15..0 are present and
                                                                assign 3D formats to some of the VICS listed in the first
                                                                16 entries in the EDID

                                                        */
                                                {
                                                        P_3D_StructureAll_t p3DStructureAll=(P_3D_StructureAll_t)&mhlTxConfig.pByte13ThroughByte15->vicList[lengthVIC];
                                                        P_3D_Mask_t p3DMask = (P_3D_Mask_t)(p3DStructureAll+1);
                                                        uint8_t mask;
                                                        mhl2VideoDescriptor.LeftRight      = p3DStructureAll->_3D_Structure_ALL_7_0.sideBySide;
                                                        mhl2VideoDescriptor.TopBottom      = p3DStructureAll->_3D_Structure_ALL_15_8.topBottom;
                                                        mhl2VideoDescriptor.FrameSequential= p3DStructureAll->_3D_Structure_ALL_15_8.framePacking;
                                                        for (mask=1,index = 0; (mask > 0) && (index < limit); ++index,mask<<=1)
                                                        {
                                                                uint8_t VIC;
                                                                Mhl2VideoDescriptor_t thisMhl2VideoDescriptor;

                                                                if (mask & p3DMask->_3D_Mask_7_0)
                                                                {
                                                                        thisMhl2VideoDescriptor = mhl2VideoDescriptor;
                                                                }
                                                                else
                                                                {
                                                                        thisMhl2VideoDescriptor.LeftRight      = 0;
                                                                        thisMhl2VideoDescriptor.TopBottom      = 0;
                                                                        thisMhl2VideoDescriptor.FrameSequential= 0;
                                                                }

                                                                VIC = mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[index].VIC;
                                                                if (VIC)
                                                                {
                                                                        if (!IsQualifiedMhlVIC(VIC,&mhl2VideoDescriptor))
                                                                        {
                                                                                mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[index].VIC=0;
                                                                        }
                                                                }
                                                        }
                                                        for (mask=1; (mask > 0) && (index < limit); ++index,mask<<=1)
                                                        {
                                                                uint8_t VIC;
                                                                Mhl2VideoDescriptor_t thisMhl2VideoDescriptor;

                                                                if (mask & p3DMask->_3D_Mask_15_8)
                                                                {
                                                                        thisMhl2VideoDescriptor = mhl2VideoDescriptor;
                                                                }
                                                                else
                                                                {
                                                                        thisMhl2VideoDescriptor.LeftRight      = 0;
                                                                        thisMhl2VideoDescriptor.TopBottom      = 0;
                                                                        thisMhl2VideoDescriptor.FrameSequential= 0;
                                                                }

                                                                VIC = mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[index].VIC;
                                                                if (VIC)
                                                                {
                                                                        if (!IsQualifiedMhlVIC(VIC,&mhl2VideoDescriptor))
                                                                        {
                                                                                mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[index].VIC=0;
                                                                        }
                                                                }
                                                        }
                                                        length3D -= sizeof(*p3DStructureAll);
                                                        length3D -= sizeof(*p3DMask);
                                                }
                                                pThreeD_u.pThreeD = &pThree3DSubBlock->hdmi3DSubBlockWithAllAndMask._3DStructureAndDetailList[0];
                                                mhlTxConfig.p3DMask = &pThree3DSubBlock->hdmi3DSubBlockWithAllAndMask._3D_Mask;
                                                break;
                                                case 0x03:
                                                        /*
                                                            Reserved for future use.
                                                            3D_Structure_ALL_15..0 and 3D_Mask_15..0 are NOT present
                                                        */
                                                        pThreeD_u.pThreeD = &pThree3DSubBlock->hdmi3DSubBlockSansAllAndMask._3DStructureAndDetailList[0];
                                                        break;
                                                }
                                                mhlTxConfig.pThreeD =pThreeD_u.pThreeD;
                                                mhlTxConfig.p3DLimit = &pThreeD_u.pAsBytes[length3D];
                                                while ( pThreeD_u.pAsBytes < mhlTxConfig.p3DLimit)
                                                {
                                                        uint8_t _2D_VIC_order           = pThreeD_u.pSansByte1->byte0._2D_VIC_order;
                                                        ThreeDStructure_e _3D_Structure = pThreeD_u.pSansByte1->byte0._3D_Structure;
                                                        uint8_t VIC;
                                                        VIC = mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[_2D_VIC_order].VIC;
                                                        //this VIC position might have gotten disqualified already
                                                        if (VIC)
                                                        {
                                                                mhl2VideoDescriptor.LeftRight           = 0;
                                                                mhl2VideoDescriptor.TopBottom           = 0;
                                                                mhl2VideoDescriptor.FrameSequential     = 0;
                                                                switch(_3D_Structure)
                                                                {
                                                                case tdsSideBySide:
                                                                {
                                                                        //re-visit uint8_t _3D_Detail    = pThreeD_u.pWithByte1->byte1._3D_Detail;
                                                                        mhl2VideoDescriptor.LeftRight   = 1;
                                                                }
                                                                break;
                                                                case tdsTopAndBottom:
                                                                        mhl2VideoDescriptor.TopBottom       = 1;
                                                                        break;
                                                                case tdsFramePacking:
                                                                        mhl2VideoDescriptor.FrameSequential = 1;
                                                                        break;
                                                                }
                                                                if (!IsQualifiedMhlVIC(VIC,&mhl2VideoDescriptor))
                                                                {
                                                                        mhlTxConfig.pVideoDataBlock2D[vdb_index]->shortDescriptors[_2D_VIC_order].VIC=0;
                                                                }
                                                        }
                                                        if (_3D_Structure < tdsSideBySide)
                                                        {
                                                                pThreeD_u.pSansByte1++;
                                                        }
                                                        else
                                                        {
                                                                pThreeD_u.pWithByte1++;
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }
}

void SiiMhlTxDisplayTimingEnumerationEnd(void)
{
        // finish off with any 3D modes reported via the HDMI VSDB
        SiiMhlTxEnumerateHdmiVsdb();
        // notify the app (board specific) layer
        AppDisplayTimingEnumerationEnd();
        SetMiscFlag(SiiMhlTxDisplayTimingEnumerationEnd,FLAGS_BURST_3D_DONE);
        SiiMhlTxPruneEDID();
}


/*
*/
static void SiiMhlTxProcess3D_VICBurst(
        PMhl2VideoFormatData_t pWriteBurstData // from 3D_REQ
)
{
        uint8_t blockIndex = 0;
        PMhl2VideoDescriptor_t pMhlVideoDescriptor;
//re-visit uint8_t edidLimit = mhlTxConfig.pByte13ThroughByte15->byte14.HDMI_3D_LEN;

        SCRPAD_DEBUG_PRINT(("burstEntryCount3D_VIC: %d\n",mhlTxConfig.burstEntryCount3D_VIC));

        if (mhlTxConfig.vic2Dindex >= mhlTxConfig.pVideoDataBlock2D[mhlTxConfig.video_data_block_index]->header.lengthFollowingHeader)
        {
                mhlTxConfig.video_data_block_index++;
        }
        if (mhlTxConfig.burstEntryCount3D_VIC < mhlTxConfig.pVideoDataBlock2D[mhlTxConfig.video_data_block_index]->header.lengthFollowingHeader)
        {
                for (// blockIndex is set to zero above
                        ; (blockIndex < pWriteBurstData->numEntriesThisBurst)
                        &&
                        (mhlTxConfig.burstEntryCount3D_VIC < pWriteBurstData->totalEntries )
                        ; ++blockIndex
                        ,++mhlTxConfig.burstEntryCount3D_VIC
                        ,++mhlTxConfig.vic2Dindex)        // each SVD is 1 byte long
                {
                        uint8_t VIC;
                        uint8_t thisModeDoable=0;
                        if (mhlTxConfig.vic2Dindex >= mhlTxConfig.pVideoDataBlock2D[mhlTxConfig.video_data_block_index]->header.lengthFollowingHeader)
                        {
                                mhlTxConfig.video_data_block_index++;
                        }
                        pMhlVideoDescriptor = &pWriteBurstData->videoDescriptors[blockIndex];
                        VIC = mhlTxConfig.pVideoDataBlock2D[mhlTxConfig.video_data_block_index]->shortDescriptors[mhlTxConfig.vic2Dindex].VIC;

                        if (VIC)
                        {
                                SCRPAD_DEBUG_PRINT(("short Descriptor[%d] 3D VIC: %d %s %s %s\n"
                                                    ,mhlTxConfig.burstEntryCount3D_VIC
                                                    ,VIC
                                                    ,pMhlVideoDescriptor->LeftRight      ?pszLeftRight      :pszSpace
                                                    ,pMhlVideoDescriptor->TopBottom      ?pszTopBottom      :pszSpace
                                                    ,pMhlVideoDescriptor->FrameSequential?pszFrameSequential:pszSpace
                                                   ));
                                thisModeDoable = IsQualifiedMhlVIC(VIC,pMhlVideoDescriptor);
                                if (!thisModeDoable)
                                {
                                        // prune this mode from EDID

                                        TX_PRUNE_PRINT(("'can't do CEA VIC:%d\n",(uint16_t)VIC));
                                        mhlTxConfig.pVideoDataBlock2D[mhlTxConfig.video_data_block_index]->shortDescriptors[mhlTxConfig.vic2Dindex].VIC = 0;
                                }
                        }
                }
        }

        SCRPAD_DEBUG_PRINT(("burstEntryCount3D_VIC: %d\n",mhlTxConfig.burstEntryCount3D_VIC));
        if ( mhlTxConfig.burstEntryCount3D_VIC >= pWriteBurstData->totalEntries )
        {
                SetMiscFlag(SiiMhlTxProcess3D_VICBurst,FLAGS_BURST_3D_VIC_DONE)
                if (TestMiscFlag(FLAGS_BURST_3D_DTD_DONE))
                {
                        if (!TestMiscFlag(FLAGS_BURST_3D_DONE))
                        {
                                SiiMhlTxDisplayTimingEnumerationEnd();
                        }
                }
        }
}

void Check3D_DTD_Sequence_Done(PMhl2VideoFormatData_t pWriteBurstData,uint8_t dtdLimit)
{
        int flag=0;
        if (mhlTxConfig.cea861DTDindex >=  dtdLimit)
        {
                flag = 1;
        }

        if ( mhlTxConfig.burstEntryCount3D_DTD >= pWriteBurstData->totalEntries )
        {
                flag =1;
        }
        if (flag)
        {
                SetMiscFlag(Check3D_DTD_Sequence_Done,FLAGS_BURST_3D_DTD_DONE)

                if (TestMiscFlag(FLAGS_BURST_3D_VIC_DONE))
                {
                        if (!TestMiscFlag(FLAGS_BURST_3D_DONE))
                        {
                                SiiMhlTxDisplayTimingEnumerationEnd();
                        }
                }
        }
}
static void SiiMhlTxProcess3D_DTDBurst(PMhl2VideoFormatData_t pWriteBurstData)
{
        PMhl2VideoDescriptor_t pMhlVideoDescriptor;
        int burstIndex=0;
        uint8_t isTiming=0;
        PCeaExtension_t pCeaExtension = (PCeaExtension_t)&edidBlockData[EDID_BLOCK_SIZE];
        uint8_t dtdLimit = (uint8_t)pCeaExtension->version_u.version3.miscSupport.totalNumberDetailedTimingDescriptorsInEntireEDID;

        if (0 == pWriteBurstData->totalEntries)
        {
                SetMiscFlag(SiiMhlTxProcess3D_DTDBurst,FLAGS_BURST_3D_DTD_VESA_DONE)
                Check3D_DTD_Sequence_Done(pWriteBurstData,dtdLimit);
                return;
        }
        if (!(FLAGS_BURST_3D_DTD_VESA_DONE & mhlTxConfig.miscFlags))
        {
                PEDID_Block0_t pEdidBlock0 = (PEDID_Block0_t)&edidBlockData[0];

                // up to four DTDs are possible in the base VESA EDID
                //  this will be covered by a single burst.
                for (// burstIndex is set to zero above
                        ; (burstIndex < pWriteBurstData->numEntriesThisBurst)
                        &&
                        (mhlTxConfig.burstEntryCount3D_DTD < pWriteBurstData->totalEntries )
                        &&
                        (mhlTxConfig.vesaDTDindex < sizeof(pEdidBlock0->detailedTimingDescriptors)/sizeof(pEdidBlock0->detailedTimingDescriptors[0]))
                        ; ++mhlTxConfig.vesaDTDindex)
                {
                        P_18ByteDescriptor_u pDesc = (P_18ByteDescriptor_u)&pEdidBlock0->detailedTimingDescriptors[mhlTxConfig.vesaDTDindex];
                        bool_t isValid=0;

                        pMhlVideoDescriptor = &pWriteBurstData->videoDescriptors[burstIndex];
                        isValid = SiiMhlTxParseDetailedTiming(pDesc, EDID_BLOCK_0,&isTiming,pMhlVideoDescriptor);
                        // only count it if it's a valid timing
                        if (isTiming)
                        {

                                if (isValid)
                                {
                                        SCRPAD_DEBUG_PRINT(("VESA DTD index: %d burst index:%d DTD SP index:%d %s %s %s\n"
                                                            ,(uint16_t)mhlTxConfig.vesaDTDindex,(uint16_t)burstIndex,(uint16_t)mhlTxConfig.burstEntryCount3D_DTD
                                                            ,pMhlVideoDescriptor->LeftRight      ?pszLeftRight      :pszSpace
                                                            ,pMhlVideoDescriptor->TopBottom      ?pszTopBottom      :pszSpace
                                                            ,pMhlVideoDescriptor->FrameSequential?pszFrameSequential:pszSpace
                                                           ));
                                }
                                else
                                {
                                        // mark this mode for pruning by setting horizontal active to zero
                                        pDesc->dtd.horzActive7_0 = 0;
                                        pDesc->dtd.horzActiveBlankingHigh.horzActive11_8= 0;
                                }

                                burstIndex++;
                                mhlTxConfig.burstEntryCount3D_DTD++;
                        }
                        else
                        {
                                SCRPAD_DEBUG_PRINT(("VESA DTD index: %d\n",(uint16_t)mhlTxConfig.vesaDTDindex));
                        }
                }

                if (mhlTxConfig.vesaDTDindex >= sizeof(pEdidBlock0->detailedTimingDescriptors)/sizeof(pEdidBlock0->detailedTimingDescriptors[0]))
                {
                        // we got past the VESA DTDs in this burst
                        SetMiscFlag(SiiMhlTxProcess3D_DTDBurst,FLAGS_BURST_3D_DTD_VESA_DONE)
                }
                else
                {
                        Check3D_DTD_Sequence_Done(pWriteBurstData,dtdLimit);
                        // more VESA DTDs to process in next burst
                        SCRPAD_DEBUG_PRINT(("%s\n",TestMiscFlag(FLAGS_BURST_3D_DTD_DONE)?"3D DTD descriptors exhausted":"more VESA DTDs to process"));
                        return;
                }
        }
        {
                union
                {
                        uint8_t *pucDataBlock;
                        P_18ByteDescriptor_u pLongDescriptors;
                } pData_u;
                pData_u.pLongDescriptors= (P_18ByteDescriptor_u)(((uint8_t *)pCeaExtension) + pCeaExtension->byteOffsetTo18ByteDescriptors);
                SCRPAD_DEBUG_PRINT(("continuing with CEA-861-D/E DTDs"
                                    "\n\tburstIndex: %d"
                                    "\n\tburstEntryCount3D_DTD: %d"
                                    "\n\tnumEntriesThisBurst: %d"
                                    "\n\ttotalEntries:%d"
                                    "\n\tdtdLimit:%d"
                                    "\n\toffsetTo18ByteDescriptors:0x%x\n"
                                    ,(uint16_t)burstIndex
                                    ,(uint16_t)mhlTxConfig.burstEntryCount3D_DTD
                                    ,(uint16_t)pWriteBurstData->numEntriesThisBurst
                                    ,(uint16_t)pWriteBurstData->totalEntries
                                    ,(uint16_t)dtdLimit
                                    ,(uint16_t)pCeaExtension->byteOffsetTo18ByteDescriptors
                                   ));
                // continue with CEA-861-D/E DTDs when done with VESA DTDs
                for (// burstIndex is set to zero above
                        ; (burstIndex < pWriteBurstData->numEntriesThisBurst)
                        &&
                        (mhlTxConfig.burstEntryCount3D_DTD < pWriteBurstData->totalEntries )
                        &&
                        (mhlTxConfig.cea861DTDindex <  dtdLimit)
                        ; ++mhlTxConfig.cea861DTDindex)
                {
                        P_18ByteDescriptor_u pDesc = &pData_u.pLongDescriptors[mhlTxConfig.cea861DTDindex];
                        bool_t isValid=0;
                        pMhlVideoDescriptor = &pWriteBurstData->videoDescriptors[burstIndex];
                        isValid=SiiMhlTxParseDetailedTiming(pDesc, EDID_BLOCK_2_3,&isTiming,pMhlVideoDescriptor);
                        // only count it if it's a valid timing
                        if (isTiming)
                        {

                                if (isValid)
                                {
                                        SCRPAD_DEBUG_PRINT(("CEA-861 DTD index: %d burst index:%d DTD SP index:%d %s %s %s\n\n"
                                                            ,(uint16_t)mhlTxConfig.cea861DTDindex,(uint16_t)burstIndex,(uint16_t)mhlTxConfig.burstEntryCount3D_DTD
                                                            ,pMhlVideoDescriptor->LeftRight      ?pszLeftRight      :pszSpace
                                                            ,pMhlVideoDescriptor->TopBottom      ?pszTopBottom      :pszSpace
                                                            ,pMhlVideoDescriptor->FrameSequential?pszFrameSequential:pszSpace
                                                           ));
                                }
                                else
                                {
                                        // mark this mode for pruning by setting horizontal active to zero
                                        pDesc->dtd.horzActive7_0 = 0;
                                        pDesc->dtd.horzActiveBlankingHigh.horzActive11_8= 0;
                                }

                                ++burstIndex;
                                ++mhlTxConfig.burstEntryCount3D_DTD;
                        }
                        else
                        {
                                SCRPAD_DEBUG_PRINT(("CEA-861 DTD index: %d\n",(uint16_t)mhlTxConfig.vesaDTDindex));
                        }
                }

                SCRPAD_DEBUG_PRINT(("DTD burst complete\n"));
                Check3D_DTD_Sequence_Done(pWriteBurstData,dtdLimit);
        }
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SiiMhlTxParseEstablishedTiming()
//
// PURPOSE      :   Parse the established timing section of EDID Block 0 and
//                  print their decoded meaning to the screen.
//
// INPUT PARAMS :   Pointer to the array where the data read from EDID
//                  Block0 is stored.
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   Void
//
//////////////////////////////////////////////////////////////////////////////

#define STRINGIZE(x) #x
#define DumpOffset(s,m) EDID_TRACE_PRINT((STRINGIZE(m)" offset:%x\n",SII_OFFSETOF(s,m) ));
#define DumpEstablishedTiming(group,width,height,refresh,progressive) \
    if (pEdidBlock0->group.et##width##x##height##_##refresh##Hz##progressive) \
    { \
        EDID_TRACE_PRINT((STRINGIZE(group)"."STRINGIZE(width)"x"STRINGIZE(height)"_"STRINGIZE(refresh)"Hz"STRINGIZE(progressive)"\n")); \
        if (!IsMhlTimingMode(width, height, refresh*1000,NULL,0)) \
        { \
            pEdidBlock0->group.et##width##x##height##_##refresh##Hz##progressive = 0; \
        } \
    }

static void SiiMhlTxParseEstablishedTiming (PEDID_Block0_t pEdidBlock0)
{


        DumpOffset(EDID_Block0_t,headerData[0]);
        DumpOffset(EDID_Block0_t,idManufacturerName);
        DumpOffset(EDID_Block0_t,idProductCode);
        DumpOffset(EDID_Block0_t,serialNumber[0]);
        DumpOffset(EDID_Block0_t,weekOfManufacture);
        DumpOffset(EDID_Block0_t,yearOfManufacture);
        DumpOffset(EDID_Block0_t,edidVersion);
        DumpOffset(EDID_Block0_t,edidRevision);
        DumpOffset(EDID_Block0_t,videoInputDefinition);
        DumpOffset(EDID_Block0_t,horzScreenSizeOrAspectRatio);
        DumpOffset(EDID_Block0_t,vertScreenSizeOrAspectRatio);
        DumpOffset(EDID_Block0_t,displayTransferCharacteristic);
        DumpOffset(EDID_Block0_t,featureSupport);
        DumpOffset(EDID_Block0_t,redGreenBits1_0);
        DumpOffset(EDID_Block0_t,blueWhiteBits1_0);
        DumpOffset(EDID_Block0_t,Red_x);
        DumpOffset(EDID_Block0_t,Red_y);
        DumpOffset(EDID_Block0_t,Green_x);
        DumpOffset(EDID_Block0_t,Green_y);
        DumpOffset(EDID_Block0_t,Blue_x);
        DumpOffset(EDID_Block0_t,Blue_y);
        DumpOffset(EDID_Block0_t,White_x);
        DumpOffset(EDID_Block0_t,White_y);

        // MHL cannot support these modes, so prune them
        pEdidBlock0->establishedTimingsII.et1280x1024_75Hz = 0;
        pEdidBlock0->manufacturersTimings.et1152x870_75Hz = 0;

        EDID_TRACE_PRINT(("Parsing Established Timing:\n"));
        EDID_TRACE_PRINT(("===========================\n"));


        DumpOffset(EDID_Block0_t,establishedTimingsI) // no semicolon needed here ... ... see macro
        // Parse Established Timing Byte #0
        DumpEstablishedTiming(establishedTimingsI,720,400,70,)
        DumpEstablishedTiming(establishedTimingsI,720,400,88,)
        DumpEstablishedTiming(establishedTimingsI,640,480,60,)
        DumpEstablishedTiming(establishedTimingsI,640,480,67,)
        DumpEstablishedTiming(establishedTimingsI,640,480,72,)
        DumpEstablishedTiming(establishedTimingsI,640,480,75,)
        DumpEstablishedTiming(establishedTimingsI,800,600,56,)
        DumpEstablishedTiming(establishedTimingsI,800,600,60,)

        // Parse Established Timing Byte #1:

        DumpOffset(EDID_Block0_t,establishedTimingsII) // no semicolon needed here ... ... see macro
        DumpEstablishedTiming(establishedTimingsII, 800, 600,72,)
        DumpEstablishedTiming(establishedTimingsII, 800, 600,75,)
        DumpEstablishedTiming(establishedTimingsII, 832, 624,75,)
        DumpEstablishedTiming(establishedTimingsII,1024, 768,87,I)
        DumpEstablishedTiming(establishedTimingsII,1024, 768,60,)
        DumpEstablishedTiming(establishedTimingsII,1024, 768,70,)
        DumpEstablishedTiming(establishedTimingsII,1024, 768,75,)
        DumpEstablishedTiming(establishedTimingsII,1280,1024,75,)

        // Parse Established Timing Byte #2:

        DumpOffset(EDID_Block0_t,manufacturersTimings) // no semicolon needed here ... ... see macro
        DumpEstablishedTiming(manufacturersTimings,1152,870,75,)

        if(   (!pEdidBlock0->headerData[0])
                        &&(0 == *((uint8_t *)&pEdidBlock0->establishedTimingsII)  )
                        &&(!pEdidBlock0->headerData[2])
          )
        {
                EDID_DEBUG_PRINT(("No established video modes\n"));
        }
}
//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SiiMhlTxParseStandardTiming()
//
// PURPOSE      :   Parse the standard timing section of EDID Block 0 and
//                  print their decoded meaning to the screen.
//
// INPUT PARAMS :   Pointer to the array where the data read from EDID
//                  Block0 is stored.
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   Void
//
//////////////////////////////////////////////////////////////////////////////

static void SiiMhlTxParseStandardTiming(PEDID_Block0_t pEdidBlock0)
{
        uint8_t i;
        uint8_t AR_Code;

        EDID_TRACE_PRINT(("Parsing Standard Timing:\n"));
        EDID_TRACE_PRINT(("========================\n"));

        for (i = 0; i < sizeof(pEdidBlock0->standardTimings)/sizeof(pEdidBlock0->standardTimings[0]); i += 2)
        {
                if (
                        (1 == pEdidBlock0->standardTimings[i].horzPixDiv8Minus31)
                        && (1 == pEdidBlock0->standardTimings[i].fieldRefreshRateMinus60)
                        && (0 == pEdidBlock0->standardTimings[i].imageAspectRatio)
                )
                {
                        // per VESA EDID standard, Release A, Revision 1, February 9, 2000, Sec. 3.9
                        EDID_TRACE_PRINT(("Standard Timing Undefined\n"));
                }
                else
                {
                        uint16_t horzActive=(uint16_t)((pEdidBlock0->standardTimings[i].horzPixDiv8Minus31 + 31)*8);
                        uint16_t vertActive=0;
                        uint16_t refreshRateInMilliHz = (uint16_t)(pEdidBlock0->standardTimings[i].fieldRefreshRateMinus60+ 60)*1000;
                        char *pszRatioString="";

                        // per VESA EDID standard, Release A, Revision 1, February 9, 2000, Table 3.15
                        AR_Code = pEdidBlock0->standardTimings[i].imageAspectRatio;

                        switch(AR_Code)
                        {
                        case iar16to10:
                                pszRatioString = "16:10";
                                vertActive = horzActive * 10/16;
                                break;

                        case iar4to3:
                                pszRatioString = "4:3";
                                vertActive = horzActive *  3/ 4;
                                break;

                        case iar5to4:
                                pszRatioString = "5:4";
                                vertActive = horzActive *  4/ 5;
                                break;

                        case iar16to9:
                                pszRatioString = "16:9";
                                vertActive = horzActive *  9/16;
                                break;
                        }
                        EDID_TRACE_PRINT(("Aspect Ratio: %5s %4d x %4d at %3d Hz.\n",pszRatioString, horzActive,vertActive, refreshRateInMilliHz  ));

                        if (!IsMhlTimingMode(horzActive, vertActive, refreshRateInMilliHz,NULL,0))
                        {
                                // disable this mode
                                pEdidBlock0->standardTimings[i].horzPixDiv8Minus31 = 1;
                                pEdidBlock0->standardTimings[i].fieldRefreshRateMinus60 = 1;
                                pEdidBlock0->standardTimings[i].imageAspectRatio = 0;
                                EDID_TRACE_PRINT(("Disabled\n\n"));
                        }
                }
        }
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SiiMhlTxParseBlockZeroTimingDescriptors()
//
// PURPOSE      :   Parse EDID Block 0 timing descriptors per EEDID 1.3
//                  standard. printf() values to screen.
//
// INPUT PARAMS :   Pointer to the 128 byte array where the data read from EDID
//                  Block0 is stored.
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   Void
//
//////////////////////////////////////////////////////////////////////////////

static void SiiMhlTxParseBlockZeroTimingDescriptors(PEDID_Block0_t pEdidBlock0)
{
        uint8_t i;
        uint8_t isTiming=0;
        SiiMhlTxParseEstablishedTiming(pEdidBlock0);
        SiiMhlTxParseStandardTiming(pEdidBlock0);

        for (i = 0; i < sizeof(pEdidBlock0->detailedTimingDescriptors)/sizeof(pEdidBlock0->detailedTimingDescriptors[0]); i++)
        {
                TX_EDID_PRINT(("Parse Results, EDID Block #0, Detailed Descriptor Number %d:\n",(int)i));
                TX_EDID_PRINT(("===========================================================\n\n"));
                SiiMhlTxParseDetailedTiming((P_18ByteDescriptor_u)&pEdidBlock0->detailedTimingDescriptors[i], EDID_BLOCK_0,&isTiming,NULL);
        }
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   bool SiiMhlTxDoEDID_Checksum()
//
// PURPOSE      :   Calculte checksum of the 128 byte block pointed to by the
//                  pointer passed as parameter
//
// INPUT PARAMS :   Pointer to a 128 byte block whose checksum needs to be
//                  calculated
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   true if chcksum is 0. false if not.
//
//////////////////////////////////////////////////////////////////////////////

bool_t SiiMhlTxDoEDID_Checksum(uint8_t *pEdidBlockData)
{
        uint8_t i;
        uint8_t CheckSum = 0;

        for (i = 0; i < EDID_BLOCK_SIZE; i++)
        {
                CheckSum += pEdidBlockData[i];
        }

        if (CheckSum)
        {
                return false;
        }

        return true;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   bool SiiMhlTxCheckEDID_Header()
//
// PURPOSE      :   Checks if EDID header is correct per VESA E-EDID standard
//					Must be 00 FF FF FF FF FF FF 00
//
// INPUT PARAMS :   Pointer to 1st EDID block
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   true if Header is correct. false if not.
//
//////////////////////////////////////////////////////////////////////////////


// Block 0
#define EDID_OFFSET_HEADER_FIRST_00	0x00
#define EDID_OFFSET_HEADER_FIRST_FF	0x01
#define EDID_OFFSET_HEADER_LAST_FF	0x06
#define EDID_OFFSET_HEADER_LAST_00	0x07
static bool_t SiiMhlTxCheckEDID_Header(PEDID_Block0_t pEdidBlock0)
{
        uint8_t i = 0;

        if (0 != pEdidBlock0->headerData[EDID_OFFSET_HEADER_FIRST_00])
        {

                ERROR_DEBUG_PRINT(("EDID 0 first check failed\n\n\n"));
                DumpEdidBlock(pEdidBlock0,sizeof(*pEdidBlock0))
                return false;
        }

        for (i = EDID_OFFSET_HEADER_FIRST_FF; i <= EDID_OFFSET_HEADER_LAST_FF; i++)
        {
                if(0xFF != pEdidBlock0->headerData[i])
                {

                        DumpEdidBlock(pEdidBlock0,sizeof(*pEdidBlock0))
                        ERROR_DEBUG_PRINT(("EDID -1 check failed\n"));
                        return false;
                }
        }

        if (0x00 != pEdidBlock0->headerData[EDID_OFFSET_HEADER_LAST_00])
        {
                DumpEdidBlock(pEdidBlock0,sizeof(*pEdidBlock0))
                ERROR_DEBUG_PRINT(("EDID 0 last check failed\n"));
                return false;
        }

        return true;
}

void SiiMhlTxMakeItDVI(PEDID_Block0_t pEdidBlock0)
{
        // Make it DVI
        EDID_Data.sinkType= sink_type_DVI;
        SiiMhlTxDrvSetInputColorSpace(acsRGB);
        SiiMhlTxDrvSetOutputColorSpace(BIT_EDID_FIELD_FORMAT_DVI_TO_RGB );

        {
                uint8_t *pEdidBlockData =(uint8_t *)pEdidBlock0;
                uint16_t counter;

                pEdidBlock0->extensionFlag = 0;

                // blank out the second block of the upstream EDID
                ERROR_DEBUG_PRINT(("DVI EDID ...Setting second block to 0xFF %d\n",(uint16_t)EDID_REV_ADDR_ERROR));
                for (counter=EDID_BLOCK_SIZE; counter < (2*EDID_BLOCK_SIZE); ++counter)
                {
                        pEdidBlockData[counter]=0xFF;
                }
        }

        ERROR_DEBUG_PRINT(("EDID ERROR...Switching to DVI. error: %d\n",(uint16_t)EDID_REV_ADDR_ERROR));
}
static void SiiMhlTx3dReqForNonTranscodeMode( void )
{
        if (mhlTxConfig.flags.abort3DReq)
        {
                mhlTxConfig.flags.abort3DReq=0;
        }
        else
        {
                TXC_DEBUG_PRINT(("Mhl2Tx: outputMode: %s\n",(sink_type_HDMI == EDID_Data.sinkType) ? "HDMI":(sink_type_DVI == EDID_Data.sinkType)?"DVI":"pending"));
                if (sink_type_HDMI == EDID_Data.sinkType)
                {
                        TXC_DEBUG_PRINT(("Mhl2Tx: remote MHL version: 0x%02x\n",(uint16_t)mhlTxConfig.devCapCache.aucDevCapCache[DEVCAP_OFFSET_MHL_VERSION]));
                        if (0x20 <= mhlTxConfig.devCapCache.aucDevCapCache[DEVCAP_OFFSET_MHL_VERSION])
                        {
                                TXC_DEBUG_PRINT(("MHL 2.x sink detected\n"));

                                AppDisplayTimingEnumerationBegin();
                                // ask the sink to begin sending 3D write bursts
                                SiiMhlTxSetInt( MHL_RCHANGE_INT, MHL2_INT_3D_REQ,0);
                                mhlTxConfig.video_data_block_index=0;
                                mhlTxConfig.burstEntryCount3D_DTD=0;
                                mhlTxConfig.vesaDTDindex=0;
                                mhlTxConfig.burstEntryCount3D_VIC=0;
                                mhlTxConfig.vic2Dindex=0;
                                mhlTxConfig.vic3Dindex=0;
                                mhlTxConfig.cea861DTDindex=0;

                                SetMiscFlag(SiiMhlTx3dReqForNonTranscodeMode,FLAGS_SENT_3D_REQ)
                                ClrMiscFlag(SiiMhlTx3dReqForNonTranscodeMode,FLAGS_BURST_3D_DONE|FLAGS_BURST_3D_DTD_DONE|FLAGS_BURST_3D_VIC_DONE | FLAGS_BURST_3D_DTD_VESA_DONE)
                        }
                        else
                        {
                                TXC_DEBUG_PRINT(("MHL 1.x sink detected\n"));
                                SiiMhlTxPruneEDID();
                        }
                }
                else
                {
#ifndef EDID_PASSTHROUGH //(
                        if (0 == SiiMhlTxDrvSetUpstreamEDID(edidBlockData,2*EDID_BLOCK_SIZE))
#endif //)
                        {
                                SetMiscFlag(SiiMhlTx3dReqForNonTranscodeMode,FLAGS_EDID_READ_DONE);
                                //ClrMhlTxFlag(syncDetect)
                                SiiMhlTxTmdsEnable();
                        }
                }
        }
}

int SiiMhlTxGetNumCea861Extensions(uint8_t blockNumber)
{
        PEDID_Block0_t pEdidBlock0 = (PEDID_Block0_t)&edidBlockData;
        uint8_t limitBlocks = sizeof(edidBlockData)/EDID_BLOCK_SIZE;
        uint8_t *pbData = &edidBlockData[EDID_BLOCK_SIZE * blockNumber];

        if (SiiMhlTxDrvGetEdidFifoNextBlock(pbData))
        {
                if (0 == blockNumber)
                {
                        if (!SiiMhlTxCheckEDID_Header(pEdidBlock0))
                        {
                                // first 8 bytes of EDID must be {0, FF, FF, FF, FF, FF, FF, 0}
                                ERROR_DEBUG_PRINT(("EDID -> Incorrect Header pbData:%x\n",(unsigned int)pbData));
                                DumpEdidBlock(pbData,sizeof(*pEdidBlock0))  // no semicolon needed here

                                // check for misalignment by one byte
                                if (SiiMhlTxCheckEDID_Header((PEDID_Block0_t)&edidBlockData[1]))
                                {
                                        return neBadHeaderOffsetBy1;
                                }
                                return neBadHeader;
                        }

                }

                if (!SiiMhlTxDoEDID_Checksum(pbData))
                {
                        // non-zero EDID checksum
                        ERROR_DEBUG_PRINT(("EDID -> Checksum Error pbData:%x\n",(unsigned int)pbData));
                        DumpEdidBlock(pbData,EDID_BLOCK_SIZE)  // no semicolon needed here
                        return neBadCheckSum;
                }
                if ( pEdidBlock0->extensionFlag < limitBlocks )
                {
                        return pEdidBlock0->extensionFlag;
                }
                else
                {
                        ERROR_DEBUG_PRINT(("not enough room for %d extensions.\n"
                                           "\tblockNumber:%02x\n"
                                           "\t&edidBlockData:%x\n"
                                           "\tpbData:%x\n"
                                           "\tpEdidBlock0: %x\n"
                                           ,(uint16_t)pEdidBlock0->extensionFlag
                                           ,(uint16_t)blockNumber
                                           ,(unsigned int)&edidBlockData
                                           ,(unsigned int)pbData
                                           ,(unsigned int)pEdidBlock0
                                          ));

                        DumpEdidBlock(pbData,sizeof(*pEdidBlock0))  // no semicolon needed here
                        return (int)(limitBlocks - 1);
                }
        }
        else
        {
                return neNoHPD;
        }
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRequestEdidBlock
//
// This function sends a READ DEVCAP MHL command to the peer.
// It  returns true if successful in doing so.
//
// The value of devcap should be obtained by making a call to SiiMhlTxGetEvents()
//
// offset		Which byte in devcap register is required to be read. 0..0x0E
//
bool_t SiiMhlTxRequestEdidBlock( uint8_t blockNum )
{
        bool_t retVal;
        cbus_req_t	req;
        EDID_DEBUG_PRINT (("MhlTx:SiiMhlTxRequestEdidBlock(%d)\n",(uint16_t)blockNum));
        //
        // Send MHL_READ_EDID_BLOCK command
        //
        req.retryCount  = 2;
        req.command     = MHL_READ_EDID_BLOCK;
        req.offsetData  = blockNum;
        req.payload_u.msgData[0]  = 0;  // do this to avoid confusion

        retVal = PutNextCBusTransaction(&req);

        MhlTxDriveStates();

        EDID_DEBUG_PRINT(("last command: %d\n",(uint16_t)mhlTxConfig.mscLastCommand ));
        return retVal;
}


static void SiiMhlTxInitiateEdidSequence(void)
{
        mhlTxConfig.num_video_data_blocks=0;
        if (mhlTxConfig.modeFlags & mfTranscodeMode)
        {
                SiiMhlTxTmdsEnable();
        }
        else
        {
                /*
                    Initiate the EDID reading sequence see
                     SiiMhlTxMscCommandDone for additional processing.
                */


                if (!mhlTxConfig.flags.edidLoopActive)
                {
                        UpdateEdidLoopActiveStatus(1)
                        SiiMhlTxRequestEdidBlock( 0 );
                        CBUS_DEBUG_PRINT(("last command: %d\n",(uint16_t)mhlTxConfig.mscLastCommand ));
                }
                else
                {
                        ERROR_DEBUG_PRINT(("edid loop active!!!!\n"));
                }
        }
}

uint8_t SiiMhlTxParse861ShortDescriptors (
        uint8_t *pEdidBlockData
)
{
        uint8_t i;
        PCeaExtension_t pCeaExtension = (PCeaExtension_t)pEdidBlockData;
        VIDEO_CAPABILITY_D_BLOCK_found=0;


        if (EDID_EXTENSION_TAG != pCeaExtension->tag)
        {
                ERROR_DEBUG_PRINT(("EDID -> Extension Tag Error\n"));
                DumpEdidBlock(pEdidBlockData,sizeof(*pCeaExtension))
                return EDID_EXT_TAG_ERROR;
        }
        else
        {
                if (EDID_REV_THREE != pCeaExtension->revision)
                {
                        ERROR_DEBUG_PRINT(("EDID -> Invalid EIA-861 Revision ID. Expected %02X. Got %02X\n", (int)EDID_REV_THREE, (int)pCeaExtension->revision));
                        return EDID_REV_ADDR_ERROR;
                }
                else
                {
                        //uint8_t DataIndex;
                        //uint8_t LongDescriptorOffset;
                        PCeaExtensionVersion3_t pCeaExtensionVersion3 = &pCeaExtension->version_u.version3;
                        union
                        {
                                uint8_t *pucDataBlock;
                                PCeaDataBlockCollection_t pCeaDataBlock;
                        } pData_u;
                        uint8_t *pucLongDescriptors;
                        // block offset where long descriptors start
                        pucLongDescriptors= ((uint8_t *)pCeaExtension) + pCeaExtension->byteOffsetTo18ByteDescriptors;

                        // byte #3 of CEA extension version 3
                        EDID_Data.UnderScan   = pCeaExtensionVersion3->miscSupport.underscanITformatsByDefault?1:0;
                        EDID_Data.BasicAudio  = pCeaExtensionVersion3->miscSupport.basicAudioSupport?1:0;
                        EDID_Data.YCbCr_4_4_4 = pCeaExtensionVersion3->miscSupport.YCrCb444Support;
                        EDID_Data.YCbCr_4_2_2 = pCeaExtensionVersion3->miscSupport.YCrCb422Support;
                        TX_EDID_PRINT(("misc support index-> %02x\n",(uint16_t)*((uint8_t *)&pCeaExtensionVersion3->miscSupport) ));

                        // choose output color depth in order of preference
                        if (EDID_Data.YCbCr_4_4_4)
                        {
                                SiiMhlTxDrvSetOutputColorSpace( BIT_EDID_FIELD_FORMAT_YCbCr444 );
                        }
                        else if (EDID_Data.YCbCr_4_2_2)
                        {
                                SiiMhlTxDrvSetOutputColorSpace( BIT_EDID_FIELD_FORMAT_YCbCr422 );
                        }
                        else
                        {
                                SiiMhlTxDrvSetOutputColorSpace( BIT_EDID_FIELD_FORMAT_HDMI_TO_RGB);
                        }


                        pData_u.pucDataBlock = &pCeaExtension->version_u.version3.Offset4_u.dataBlockCollection[0];

                        while (pData_u.pucDataBlock < pucLongDescriptors)
                        {
                                DataBlockTagCode_e TagCode;//uint8_t TagCode;
                                uint8_t DataBlockLength;
                                TagCode = pData_u.pCeaDataBlock->header.tagCode;
                                DataBlockLength = pData_u.pCeaDataBlock->header.lengthFollowingHeader;
                                TX_EDID_PRINT(("TagCode:%d DataBlockLength:%d\n",TagCode,DataBlockLength));
                                if (( pData_u.pucDataBlock + DataBlockLength) > pucLongDescriptors)
                                {
                                        ERROR_DEBUG_PRINT(("EDID -> V Descriptor Overflow\n"));
                                        return EDID_V_DESCR_OVERFLOW;
                                }

                                i = 0;                                  // num of short video descriptors in current data block

                                switch (TagCode)
                                {
                                case dbtcVideoDataBlock:
                                {
                                        TX_EDID_PRINT(("dbtcVideoDataBlock:\n"));
                                        if (mhlTxConfig.num_video_data_blocks < NUM_VIDEO_DATA_BLOCKS_LIMIT)
                                        {
                                                mhlTxConfig.pVideoDataBlock2D[mhlTxConfig.num_video_data_blocks] = (PVideoDataBlock_t)pData_u.pucDataBlock;

                                                while (i < DataBlockLength)        // each SVD is 1 byte long
                                                {
                                                        uint8_t VIC;
                                                        VIC = mhlTxConfig.pVideoDataBlock2D[mhlTxConfig.num_video_data_blocks]->shortDescriptors[i].VIC;
                                                        TX_EDID_PRINT(("short desc[%d]: VIC: %d\n",i,VIC));
                                                        if (!IsQualifiedMhlVIC(VIC,NULL))
                                                        {
                                                                mhlTxConfig.pVideoDataBlock2D[mhlTxConfig.num_video_data_blocks]->shortDescriptors[i].VIC = 0;
                                                        }
                                                        i++;
                                                }
                                                TX_EDID_PRINT(("EDID -> %d descriptors in Video Data Block[%d]\n",i,(uint16_t)mhlTxConfig.num_video_data_blocks));
                                                mhlTxConfig.num_video_data_blocks++;
                                        }
                                }
                                break;

                                case dbtcAudioDataBlock:
                                {
                                        PAudioDataBlock_t pAudioDataBlock = (PAudioDataBlock_t) pData_u.pucDataBlock;
                                        uint8_t A_DescriptorIndex = 0;  // static to support more than one extension

                                        // each SAD is 3 bytes long
                                        while (i < DataBlockLength/sizeof(pAudioDataBlock->shortAudioDescriptors[0]) )
                                        {
                                                *((PCeaShortAudioDescriptor_t)&EDID_Data.AudioDescriptor[A_DescriptorIndex])
                                                        = pAudioDataBlock->shortAudioDescriptors[i];
                                                A_DescriptorIndex++;
                                                i++;
                                        }
                                        TX_EDID_PRINT(("EDID -> Short Descriptor Audio Block\n"));
                                }
                                break;

                                case dbtcSpeakerAllocationDataBlock:
                                {
                                        PSpeakerAllocationDataBlock_t pSpeakerAllocationDataBlock = (PSpeakerAllocationDataBlock_t) pData_u.pucDataBlock;

                                        *((PSpeakerAllocationFlags_t)&EDID_Data.SpkrAlloc[i++]) = pSpeakerAllocationDataBlock->payload.speakerAllocFlags;
                                        TX_EDID_PRINT(("EDID -> Short Descriptor Speaker Allocation Block\n"));
                                }
                                break;

                                case dbtcUsedExtendedTag:
                                {
                                        ExtendedTagCode_t ExtendedTagCode;
                                        ExtendedTagCode = pData_u.pCeaDataBlock->payload_u.extendedTag;
                                        switch (ExtendedTagCode.extendedTagCode)
                                        {
                                        case etcVideoCapabilityDataBlock:
                                        {
                                                PVideoCapabilityDataBlock_t pVideoCapabilityDataBlock = (PVideoCapabilityDataBlock_t)pData_u.pucDataBlock;
                                                EDID_Data.VideoCapabilityFlags = pVideoCapabilityDataBlock->payload;
                                                mhlTxConfig.pVideoCapabilityDataBlock = pVideoCapabilityDataBlock;
                                                VIDEO_CAPABILITY_D_BLOCK_found = 1;
                                                TX_EDID_PRINT(("EDID -> Short Descriptor Video Capability Block\n"));
                                                INFO_DEBUG_PRINT(("%s %02x\n",(EDID_Data.VideoCapabilityFlags.QS)?"VCF":"vcf",(uint16_t)*((uint8_t *)&EDID_Data.VideoCapabilityFlags)));
                                        }
                                        break;
                                        case etcColorimetryDataBlock:
                                        {
                                                PColorimetryDataBlock_t pColorimetryDataBlock = (PColorimetryDataBlock_t)pData_u.pucDataBlock;
                                                PColorimeteryDataPayLoad_t pPayLoad= &pColorimetryDataBlock->payload;
                                                EDID_Data.ColorimetrySupportFlags = pPayLoad->ciData.xvYCC;
                                                EDID_Data.MetadataProfile         = pPayLoad->cmMetaData.metaData;
                                        }

                                        TX_EDID_PRINT(("EDID -> Short Descriptor Colorimetry Block\n"));
                                        break;
                                        }
                                }

                                break;

                                case dbtcVendorSpecificDataBlock:
                                {
                                        PVendorSpecificDataBlock_t pVendorSpecificDataBlock = (PVendorSpecificDataBlock_t) pData_u.pucDataBlock;
                                        uint8_t *pucNextDB = ((uint8_t *)&pVendorSpecificDataBlock->header) + sizeof(pVendorSpecificDataBlock->header) + DataBlockLength;

                                        if (   (pVendorSpecificDataBlock->IEEE_OUI[0] == 0x03)
                                                        && (pVendorSpecificDataBlock->IEEE_OUI[1] == 0x0C)
                                                        && (pVendorSpecificDataBlock->IEEE_OUI[2] == 0x00)
                                           )

                                        {
                                                PHdmiLLCVendorSpecificDataBlockPayload_t pHdmiVendorSpecificPayload = &pVendorSpecificDataBlock->payload_u.HdmiLLC;
                                                mhlTxConfig.pHdmiVendorSpecificDataBlock = pVendorSpecificDataBlock;

                                                SII_ASSERT (5 <= DataBlockLength,("unexpected DataBlockLength:%d\n",DataBlockLength))
                                                EDID_Data.sinkType = sink_type_HDMI;
                                                *((PHdmiLLC_BA_t)&EDID_Data.CEC_A_B) = pHdmiVendorSpecificPayload->B_A;   // CEC Physical address
                                                *((PHdmiLLC_DC_t)&EDID_Data.CEC_C_D) = pHdmiVendorSpecificPayload->D_C;
                                                // Offset of 3D_Present bit in VSDB
                                                if (pHdmiVendorSpecificPayload->byte8.LatencyFieldsPresent)
                                                {
                                                        if(pHdmiVendorSpecificPayload->byte8.I_LatencyFieldsPresent)
                                                        {
                                                                mhlTxConfig.pByte13ThroughByte15= &pHdmiVendorSpecificPayload->vsdbFieldsByte9ThroughByte15.vsdbAllFieldsByte9ThroughByte15.byte13ThroughByte15;
                                                        }
                                                        else
                                                        {
                                                                mhlTxConfig.pByte13ThroughByte15 = &pHdmiVendorSpecificPayload->vsdbFieldsByte9ThroughByte15.vsdbAllFieldsByte9ThroughByte15SansInterlacedLatency.byte13ThroughByte15;
                                                        }
                                                }
                                                else
                                                {
                                                        if(pHdmiVendorSpecificPayload->byte8.I_LatencyFieldsPresent)
                                                        {
                                                                mhlTxConfig.pByte13ThroughByte15 = &pHdmiVendorSpecificPayload->vsdbFieldsByte9ThroughByte15.vsdbAllFieldsByte9ThroughByte15SansProgressiveLatency.byte13ThroughByte15;
                                                        }
                                                        else
                                                        {
                                                                mhlTxConfig.pByte13ThroughByte15 = &pHdmiVendorSpecificPayload->vsdbFieldsByte9ThroughByte15.vsdbAllFieldsByte9ThroughByte15SansAllLatency.byte13ThroughByte15;
                                                        }
                                                }
                                                if  ( ((uint8_t *)&mhlTxConfig.pByte13ThroughByte15->byte13) >= pucNextDB )
                                                {
                                                        EDID_Data._3D_Supported = false;
                                                }
                                                else if (mhlTxConfig.pByte13ThroughByte15->byte13._3D_present)
                                                {
                                                        EDID_Data._3D_Supported = true;
                                                }
                                                else
                                                {
                                                        EDID_Data._3D_Supported = false;
                                                }

                                                TX_EDID_PRINT(("EDID indicates %s3D support\n",EDID_Data._3D_Supported?"":"NO " ));
                                        }
                                        else
                                        {
                                                EDID_Data.sinkType= sink_type_DVI;
                                        }

                                        TX_EDID_PRINT(("EDID -> Short Descriptor Vendor Block\n\n"));
                                }

                                break;
                                case dbtcTerminator:
                                        TX_EDID_PRINT(("MhlTx: SiiMhlTxParse861ShortDescriptors found terminator tag code\n"));
                                        return EDID_SHORT_DESCRIPTORS_OK;
                                        break;

                                default:
                                        ERROR_DEBUG_PRINT(("EDID -> Unknown Tag Code:0x%02x\n",(uint16_t)TagCode));
                                        return EDID_UNKNOWN_TAG_CODE;

                                }
                                pData_u.pucDataBlock += sizeof(pData_u.pCeaDataBlock->header)+ DataBlockLength;
                        }

                        return EDID_SHORT_DESCRIPTORS_OK;
                }

        }
}

static uint8_t SiiMhlTxParse861Block(uint8_t *pEdidBlockData)
{

        uint8_t ErrCode;
        PCeaExtension_t pCeaExtension = (PCeaExtension_t)pEdidBlockData;
        mhlTxConfig.pHdmiVendorSpecificDataBlock = NULL;
        mhlTxConfig.pVideoDataBlock2D[mhlTxConfig.num_video_data_blocks]=NULL;

        if (EDID_EXTENSION_BLOCK_MAP == pCeaExtension->tag)
        {
                PBlockMap_t pBlockMap = (PBlockMap_t)pEdidBlockData;
                int i;
                EDID_DEBUG_PRINT(("Edid: Block Map\n"));
                // loop limit is adjusted by one to account for block map
                for(i=0; i<mhlTxConfig.numEdidExtensions-1; ++i)
                {
                        if (EDID_EXTENSION_TAG != pBlockMap->blockTags[i])
                        {
                                EDID_DEBUG_PRINT(("Edid: Adjusting number of extensions according to Block Map\n"));
                                mhlTxConfig.numEdidExtensions=i; /* include block map in count */
                                break;
                        }
                }

                return EDID_OK;

        }
        else
        {
                ErrCode = SiiMhlTxParse861ShortDescriptors(pEdidBlockData);
                if (ErrCode != EDID_SHORT_DESCRIPTORS_OK)
                {
                        ERROR_DEBUG_PRINT(("MhlTx2: Errcode:%d\n",(uint16_t)ErrCode));
                        return ErrCode;
                }

                // adjust
                ErrCode = SiiMhlTxParse861LongDescriptors(pEdidBlockData);
                if (ErrCode != EDID_LONG_DESCRIPTORS_OK)
                {
                        ERROR_DEBUG_PRINT(("MhlTx2: Errcode:%d\n",(uint16_t)ErrCode));
                        return ErrCode;
                }
        }
        return EDID_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxMscCommandDone
//
// This function is called by the driver to inform of completion of last command.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
void	SiiMhlTxMscCommandDone( uint8_t data1 )
{
        CBUS_DEBUG_PRINT (("MhlTx:SiiMhlTxMscCommandDone. data1 = %02X last command:%02x\n", (int) data1, (uint16_t)mhlTxConfig.mscLastCommand ));

        DecrementCBusReferenceCount(SiiMhlTxMscCommandDone)
        if ( MHL_READ_DEVCAP == mhlTxConfig.mscLastCommand )
        {
                uint8_t param;
                data1 = data1;  //avoid compiler warning

                // indicate that the DEVCAP cache is up to date.
                UpdateDevCapCacheIndex(sizeof(mhlTxConfig.devCapCache.aucDevCapCache));
                SiiMhlTxDrvGetPeerDevCapRegs(&mhlTxConfig.devCapCache);
                SetMiscFlag(SiiMhlTxMscCommandDone,FLAGS_HAVE_DEV_CATEGORY);
                param  = mhlTxConfig.devCapCache.mdc.deviceCategory & MHL_DEV_CATEGORY_POW_BIT;

                // Give the OEM a chance at handling power for himself
                if (MHL_TX_EVENT_STATUS_PASSTHROUGH == AppNotifyMhlEvent(MHL_TX_EVENT_POW_BIT_CHG, param,NULL))
                {
                        SiiMhlTxDrvPowBitChange((bool_t)param );
                }
                SetMiscFlag(SiiMhlTxMscCommandDone,FLAGS_HAVE_DEV_FEATURE_FLAGS)
                ExamineLocalAndPeerVidLinkMode();

                SetMiscFlag(SiiMhlTxMscCommandDone,FLAGS_HAVE_COMPLETE_DEVCAP);

                //This is necessary for both firmware and linux driver.
                AppNotifyMhlEvent(MHL_TX_EVENT_DCAP_CHG, 0,NULL);
                mhlTxConfig.mscLastCommand = 0;
                mhlTxConfig.mscLastOffset  = 0;

                // see MHL spec section 5.9.1
                if (MHL_HPD & mhlTxConfig.mhlHpdRSENflags)
                {
                        TXC_DEBUG_PRINT(("HPD\n"));
                        SiiMhlTxInitiateEdidSequence();
                        CBUS_DEBUG_PRINT(("last command: %d\n",(uint16_t)mhlTxConfig.mscLastCommand ));
                }
                else
                {
                        TXC_DEBUG_PRINT(("no HPD\n"));
                }
                CBUS_DEBUG_PRINT(("last command: %d\n",(uint16_t)mhlTxConfig.mscLastCommand ));
        }
        else if (MHL_READ_EDID_BLOCK == mhlTxConfig.mscLastCommand)
        {
                mhlTxConfig.mscLastCommand = 0;
                mhlTxConfig.mscLastOffset  = 0;
                MhlTxDriveStates();
                if (0 == data1)
                {
                        PEDID_Block0_t pEdidBlock0 = (PEDID_Block0_t)&edidBlockData[0];
                        uint8_t Result = EDID_OK;

                        //
#ifdef  EDID_PASSTHROUGH //(
                        SiiMhlTxDrvSetUpstreamEDID(edidBlockData,2*EDID_BLOCK_SIZE);
#endif //)

                        EDID_DEBUG_PRINT(("Entire EDID Read complete\n"));
                        SiiMhlTxParseBlockZeroTimingDescriptors(pEdidBlock0);			// Parse EDID Block #0 Desctiptors

                        EDID_DEBUG_PRINT(("EDID -> Number of 861 Extensions = %d\n", (uint16_t)pEdidBlock0->extensionFlag));

                        // number of extensions is one less than number of blocks
                        mhlTxConfig.numEdidExtensions = pEdidBlock0->extensionFlag;
                        if (0 == pEdidBlock0->extensionFlag)
                        {
                                DumpEdidBlock((uint8_t *)pEdidBlock0,EDID_BLOCK_SIZE)
                                EDID_Data.sinkType = sink_type_DVI;
                                SiiMhlTxDrvSetOutputColorSpace(BIT_EDID_FIELD_FORMAT_DVI_TO_RGB );
                                EDID_DEBUG_PRINT(("EDID -> No 861 Extensions\n"));
                        }
                        else
                        {
                                uint8_t counter;
                                for (counter = 1; counter <= mhlTxConfig.numEdidExtensions; ++counter)
                                {
                                        Result = SiiMhlTxParse861Block(&edidBlockData[EDID_BLOCK_SIZE * counter]);
                                        if (EDID_OK != Result)
                                        {
                                                ERROR_DEBUG_PRINT(("EDID -> Extension Parse FAILED: Result:%d\n",(uint16_t)Result));
                                                SiiMhlTxMakeItDVI(pEdidBlock0);
                                                Result = EDID_OK;
                                        }
                                }
                        }
                        pEdidBlock0->checkSum=0;
                        pEdidBlock0->checkSum = CalculateGenericCheckSum((uint8_t *)pEdidBlock0,0,sizeof(*pEdidBlock0));
                        SiiMhlTx3dReqForNonTranscodeMode();
                        UpdateEdidLoopActiveStatus(0)

                }
                else
                {
                        // give up on EDID reading
                        UpdateEdidLoopActiveStatus(0)
                }
        }
        else if(MHL_WRITE_STAT == mhlTxConfig.mscLastCommand)
        {

                CBUS_DEBUG_PRINT(("MhlTx: WRITE_STAT miscFlags: %02X\n\n", (int) mhlTxConfig.miscFlags));
                if (MHL_STATUS_REG_CONNECTED_RDY == mhlTxConfig.mscLastOffset)
                {
                        if (MHL_STATUS_DCAP_RDY & mhlTxConfig.mscLastData)
                        {
                                SetMiscFlag(SiiMhlTxMscCommandDone,FLAGS_SENT_DCAP_RDY)

                                CBUS_DEBUG_PRINT(("MhlTx:SiiMhlTxMscCommandDone FLAGS_SENT_DCAP_RDY\n"));
                                SiiMhlTxSetInt(MHL_RCHANGE_INT,MHL_INT_DCAP_CHG, 0); // priority request
                        }
                }
                else if (MHL_STATUS_REG_LINK_MODE == mhlTxConfig.mscLastOffset)
                {
                        if ( MHL_STATUS_PATH_ENABLED & mhlTxConfig.mscLastData)
                        {
                                SetMiscFlag(SiiMhlTxMscCommandDone,FLAGS_SENT_PATH_EN);
                                CBUS_DEBUG_PRINT(("MhlTx:SiiMhlTxMscCommandDone FLAGS_SENT_PATH_EN\n"));
                        }
                }

                mhlTxConfig.mscLastCommand = 0;
                mhlTxConfig.mscLastOffset  = 0;
        }
        else if (MHL_MSC_MSG == mhlTxConfig.mscLastCommand)
        {
                if (SiiMhlTxDrvMscMsgNacked())
                {
                        cbus_req_t retry;
                        retry.retryCount = 2;
                        retry.command = mhlTxConfig.mscLastCommand;
                        retry.payload_u.msgData[0] = mhlTxConfig.mscMsgLastCommand;
                        retry.payload_u.msgData[1] = mhlTxConfig.mscMsgLastData;
                        SiiMhlTxDrvSendCbusCommand( &retry );
                }
                else
                {
                        if(MHL_MSC_MSG_RCPE == mhlTxConfig.mscMsgLastCommand)
                        {
                                //
                                // RCPE is always followed by an RCPK with original key code that came.
                                //
                                if( SiiMhlTxRcpkSend( mhlTxConfig.mscSaveRcpKeyCode ) )
                                {
                                }
                        }
                        else if(MHL_MSC_MSG_UCPE == mhlTxConfig.mscMsgLastCommand)
                        {
                                //
                                // UCPE is always followed by an UCPK with original key code that came.
                                //
                                if( SiiMhlTxUcpkSend( mhlTxConfig.mscSaveUcpKeyCode ) )
                                {
                                }
                        }
                        else
                        {
                                CBUS_DEBUG_PRINT (("MhlTx:SiiMhlTxMscCommandDone default\n"
                                                   "\tmscLastCommand: 0x%02X \n"
                                                   "\tmscMsgLastCommand: 0x%02X mscMsgLastData: 0x%02X\n"
                                                   "\tcbusReferenceCount: %d\n"
                                                   ,(int)mhlTxConfig.mscLastCommand
                                                   ,(int)mhlTxConfig.mscMsgLastCommand
                                                   ,(int)mhlTxConfig.mscMsgLastData
                                                   ,(int)mhlTxConfig.cbusReferenceCount
                                                  ));
                        }
                        mhlTxConfig.mscLastCommand = 0;
                }
        }
        else if (MHL_WRITE_BURST == mhlTxConfig.mscLastCommand)
        {
                SCRPAD_DEBUG_PRINT (("MhlTx:SiiMhlTxMscCommandDone MHL_WRITE_BURST\n"));
                mhlTxConfig.mscLastCommand = 0;
                mhlTxConfig.mscLastOffset  = 0;
                mhlTxConfig.mscLastData    = 0;

                // all CBus request are queued, so this is OK to call here
                // use priority 0 so that other queued commands don't interfere
                SiiMhlTxSetInt( MHL_RCHANGE_INT,MHL_INT_DSCR_CHG,0 );
        }
        else if (MHL_SET_INT == mhlTxConfig.mscLastCommand)
        {
                CBUS_DEBUG_PRINT (("MhlTx:SiiMhlTxMscCommandDone MHL_SET_INT\n"));
                if (MHL_RCHANGE_INT == mhlTxConfig.mscLastOffset)
                {
                        CBUS_DEBUG_PRINT (("MhlTx:SiiMhlTxMscCommandDone MHL_RCHANGE_INT\n"));
                        if (MHL_INT_DSCR_CHG == mhlTxConfig.mscLastData)
                        {
                                DecrementCBusReferenceCount(SiiMhlTxMscCommandDone)  // this one is for the write burst request
                                SCRPAD_DEBUG_PRINT (("MhlTx:SiiMhlTxMscCommandDone MHL_INT_DSCR_CHG\n"));
                                ClrMiscFlag(SiiMhlTxMscCommandDone, FLAGS_SCRATCHPAD_BUSY)
                        }
                }
                // Once the command has been sent out successfully, forget this case.
                mhlTxConfig.mscLastCommand = 0;
                mhlTxConfig.mscLastOffset  = 0;
                mhlTxConfig.mscLastData    = 0;
        }
        else
        {
                CBUS_DEBUG_PRINT (("MhlTx:SiiMhlTxMscCommandDone default\n"
                                   "\tmscLastCommand: 0x%02X mscLastOffset: 0x%02X\n"
                                   "\tcbusReferenceCount: %d\n"
                                   ,(int)mhlTxConfig.mscLastCommand
                                   ,(int)mhlTxConfig.mscLastOffset
                                   ,(int)mhlTxConfig.cbusReferenceCount
                                  ));
        }
        if (!(FLAGS_RCP_READY & mhlTxConfig.miscFlags))
        {
#define FLAG_OR_NOT(x) (FLAGS_HAVE_##x & mhlTxConfig.miscFlags)?#x:""
#define SENT_OR_NOT(x) (FLAGS_SENT_##x & mhlTxConfig.miscFlags)?#x:""

                CBUS_DEBUG_PRINT (("MhlTx:SiiMhlTxMscCommandDone. have(%s %s) sent(%s %s)\n"
                                   , FLAG_OR_NOT(DEV_CATEGORY)
                                   , FLAG_OR_NOT(DEV_FEATURE_FLAGS)
                                   , SENT_OR_NOT(PATH_EN)
                                   , SENT_OR_NOT(DCAP_RDY)
                                  ));
                if (FLAGS_HAVE_DEV_CATEGORY & mhlTxConfig.miscFlags)
                {
                        if (FLAGS_HAVE_DEV_FEATURE_FLAGS& mhlTxConfig.miscFlags)
                        {
                                if (FLAGS_SENT_PATH_EN & mhlTxConfig.miscFlags)
                                {
                                        if (FLAGS_SENT_DCAP_RDY & mhlTxConfig.miscFlags)
                                        {
                                                if (DEVCAP_REFRESH_IDLE)
                                                {
                                                        SetMiscFlag(SiiMhlTxMscCommandDone,FLAGS_RCP_READY)

                                                        // Now we can entertain App commands for RCP
                                                        // Let app know this state
                                                        mhlTxConfig.mhlConnectionEvent = true;
                                                        mhlTxConfig.mhlConnected = MHL_TX_EVENT_RCP_READY;
                                                }
                                        }
                                }
                        }
                }
        }
}

/*
*/
void SiiMhlTxProcessWriteBurstData( void )
{
        TXC_DEBUG_PRINT(("SiiMhlTxProcessWriteBurstData\n"));
        {
                BurstId_e burstId;
                SiiMhlTxDrvGetScratchPad(0,mhlTxConfig.incomingScratchPad.asBytes,sizeof(mhlTxConfig.incomingScratchPad));
#ifdef ENABLE_SCRPAD_DEBUG_PRINT //(
                {
                        uint8_t i;
                        SCRPAD_DEBUG_PRINT (("MhlTx: Write Burst Data:"));
                        for (i = 0; i < sizeof(mhlTxConfig.incomingScratchPad); ++i)
                        {
                                SiiOsDebugPrintAlwaysShort(" %02X", (int) mhlTxConfig.incomingScratchPad.asBytes[i]);
                        }
                        SiiOsDebugPrintAlwaysShort("\n");
                }
#endif //)

                burstId =BURST_ID(mhlTxConfig.incomingScratchPad.videoFormatData.burstId);

                switch(burstId)
                {
                case burstId_3D_VIC:
                        SiiMhlTxProcess3D_VICBurst(&mhlTxConfig.incomingScratchPad.videoFormatData);
                        break;
                case burstId_3D_DTD:
                        SiiMhlTxProcess3D_DTDBurst(&mhlTxConfig.incomingScratchPad.videoFormatData);
                        break;
                default:
                        TXC_DEBUG_PRINT(("SiiMhlTxProcessWriteBurstData burstId: %d\n",(uint16_t)burstId));
                        SII_ASSERT((&mhlTxConfig.incomingScratchPad.videoFormatData.burstId.high== &mhlTxConfig.incomingScratchPad.asBytes[0])
                                   ,(" high at offset 0x%x asBytes[0] at offset 0x%x\n"
                                     ,&mhlTxConfig.incomingScratchPad.videoFormatData.burstId.high
                                     ,&mhlTxConfig.incomingScratchPad.asBytes[0]
                                    ));
                        SII_ASSERT((&mhlTxConfig.incomingScratchPad.videoFormatData.burstId.low== &mhlTxConfig.incomingScratchPad.asBytes[1])
                                   ,(" low  at offset 0x%x asBytes[1] at offset 0x%x\n"
                                     ,&mhlTxConfig.incomingScratchPad.videoFormatData.burstId.low
                                     ,&mhlTxConfig.incomingScratchPad.asBytes[1]
                                    ));
                        SCRPAD_DEBUG_PRINT (("MhlTx: BurstId: 0x%04x\n",burstId));
                }
        }
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxMscWriteBurstDone
//
// This function is called by the driver to inform of completion of a write burst.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
void	SiiMhlTxMscWriteBurstDone( uint8_t data1 )
{
        data1=data1;  // make this compile for NON debug builds
        SCRPAD_DEBUG_PRINT (("MhlTx:SiiMhlTxMscWriteBurstDone(%02X) Incoming\n",(int)data1  ));
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxSetPathEn
//
static bool_t SiiMhlTxSetPathEn(void )
{
        TXC_DEBUG_PRINT(("MhlTx:SiiMhlTxSetPathEn\n"));
        SiiMhlTxTmdsEnable();
        mhlTxConfig.linkMode |= MHL_STATUS_PATH_ENABLED;     // update local copy and send to remote
        return SiiMhlTxSetStatus( MHL_STATUS_REG_LINK_MODE, mhlTxConfig.linkMode);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxClrPathEn
//
static bool_t SiiMhlTxClrPathEn( void )
{
        TXC_DEBUG_PRINT(("MhlTx: SiiMhlTxClrPathEn\n"));
        SiiMhlTxDrvTmdsControl( false );
        mhlTxConfig.linkMode &= ~MHL_STATUS_PATH_ENABLED;    // update local copy
        return SiiMhlTxSetStatus( MHL_STATUS_REG_LINK_MODE, mhlTxConfig.linkMode);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlMscMsg
//
// This function is called by the driver to inform of arrival of a MHL MSC_MSG
// such as RCP, RCPK, RCPE. To quickly return back to interrupt, this function
// remembers the event (to be picked up by app later in task context).
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing of its own,
//
// No printfs.
//
// Application shall not call this function.
//
void	SiiMhlTxGotMhlMscMsg( uint8_t subCommand, uint8_t cmdData )
{
        // Remember the event.
        mhlTxConfig.mscMsgArrived		= true;
        mhlTxConfig.mscMsgSubCommand	= subCommand;
        mhlTxConfig.mscMsgData			= cmdData;
        SiiOsDebugPrintAlwaysShort("recieved MSC msg %02x %02x\n",(uint16_t) subCommand,(uint16_t) cmdData);
}

static void SiiMhlTxResetEdidStatus( void )
{
        int i;
        uint8_t *pData=(uint8_t *) &EDID_Data;
        // clear out EDID parse results
        for (i=0; i < sizeof(EDID_Data); ++i)
        {
                pData[i]=0;
        }
        SiiMhlTxDrvSetOutputColorSpace( BIT_EDID_FIELD_FORMAT_HDMI_TO_RGB);
        SiiMhlTxDrvSetPackedPixelStatus( 0 );
        EDID_Data.sinkType = sink_type_pending;
        ClrMiscFlag(SiiMhlTxResetEdidStatus,FLAGS_EDID_READ_DONE);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlIntr
//
// This function is called by the driver to inform of arrival of a MHL INTERRUPT.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
void	SiiMhlTxGotMhlIntr( uint8_t intr_0, uint8_t intr_1 )
{
        TXC_DEBUG_PRINT (("MhlTx: INTERRUPT Arrived. %02X, %02X\n", (int) intr_0, (int) intr_1 ));

        //
        // Handle DCAP_CHG INTR here
        //
        if(MHL_INT_DCAP_CHG & intr_0)
        {
#ifndef NEW_DCAP_HANDLING //(
                if (!DEVCAP_REFRESH_COMPLETE)
                {
                        SiiMhlTxRefreshPeerDevCapEntries();
                }
#endif //)
        }

        if( MHL_INT_DSCR_CHG & intr_0)
        {
                // remote WRITE_BURST is complete
                SiiMhlTxProcessWriteBurstData();
                ClrMiscFlag(SiiMhlTxGotMhlIntr, FLAGS_SCRATCHPAD_BUSY)
                AppNotifyMhlEvent(MHL_TX_EVENT_DSCR_CHG,0,NULL);
        }
        if( MHL_INT_REQ_WRT  & intr_0)
        {

                // this is a request from the sink device.
                if (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)
                {
                        // use priority 1 to defer sending grant until
                        //  local traffic is done
                        SiiMhlTxSetInt( MHL_RCHANGE_INT, MHL_INT_GRT_WRT,1);
                }
                else
                {
                        SetMiscFlag(SiiMhlTxGotMhlIntr, FLAGS_SCRATCHPAD_BUSY)
                        // OK to call this here, since all requests are queued
                        // use priority 0 to respond immediately
                        SiiMhlTxSetInt( MHL_RCHANGE_INT, MHL_INT_GRT_WRT,0);
                }
        }
        if( MHL_INT_GRT_WRT  & intr_0)
        {
                uint8_t length =sizeof(mhlTxConfig.outgoingScratchPad);
                TXC_DEBUG_PRINT(("MhlTx: MHL_INT_GRT_WRT length:%d\n",(int)length));
                SiiMhlTxDoWriteBurst(0x40, mhlTxConfig.outgoingScratchPad.asBytes, length);
        }

        // removed "else", since interrupts are not mutually exclusive of each other.
        if(MHL_INT_EDID_CHG & intr_1)
        {
                TXC_DEBUG_PRINT(("EDID change\n"));
                // force upstream source to read the EDID again.
                // Most likely by appropriate toggling of HDMI HPD
                if (mhlTxConfig.flags.edidLoopActive)
                {
                        mhlTxConfig.flags.abort3DReq=1;
                }
                else
                {
                        SiiMhlTxResetEdidStatus();
                }
                SiiMhlTxDrvNotifyEdidChange ( );
                if ( DEVCAP_REFRESH_COMPLETE )
                {
                        if (MHL_HPD & mhlTxConfig.mhlHpdRSENflags)
                        {
                                SiiMhlTxInitiateEdidSequence();
                        }
                }
                // DCAP_CHG has its own interrupt.
                else if (!(MHL_INT_DCAP_CHG & intr_0))
                {
                        SiiMhlTxRefreshPeerDevCapEntries();
                }
        }
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlStatus
//
// This function is called by the driver to inform of arrival of a MHL STATUS.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
void	SiiMhlTxGotMhlStatus( uint8_t status_0, uint8_t status_1 )
{
//	TXC_DEBUG_PRINT(("MhlTx: STATUS Arrived. %02X, %02X\n", (int) status_0, (int) status_1));
        //
        // Handle DCAP_RDY STATUS here itself
        //
        uint8_t StatusChangeBitMask0,StatusChangeBitMask1;
        StatusChangeBitMask0 = status_0 ^ mhlTxConfig.status_0;
        StatusChangeBitMask1 = status_1 ^ mhlTxConfig.status_1;
        // Remember the event.   (other code checks the saved values, so save the values early, but not before the XOR operations above)
        mhlTxConfig.status_0 = status_0;
        mhlTxConfig.status_1 = status_1;

        if(MHL_STATUS_DCAP_RDY & StatusChangeBitMask0)
        {

                TXC_DEBUG_PRINT(("MhlTx: DCAP_RDY changed\n"));
                if (MHL_STATUS_DCAP_RDY & mhlTxConfig.status_0)
                {
                        SiiMhlTxRefreshPeerDevCapEntries();
                }
                else
                {
                        UpdateDevCapCacheIndex(1+sizeof(mhlTxConfig.devCapCache.aucDevCapCache));
                }
        }


        // did PATH_EN change?
        if(MHL_STATUS_PATH_ENABLED & StatusChangeBitMask1)
        {
                TXC_DEBUG_PRINT(("MhlTx: PATH_EN changed\n"));
                if(MHL_STATUS_PATH_ENABLED & status_1)
                {
                        // OK to call this here since all requests are queued
                        SiiMhlTxSetPathEn();
                }
                else
                {
                        // OK to call this here since all requests are queued
                        SiiMhlTxClrPathEn();
                }
        }
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRcpSend
//
// This function checks if the peer device supports RCP and sends rcpKeyCode. The
// function will return a value of true if it could successfully send the RCP
// subcommand and the key code. Otherwise false.
//
// The followings are not yet utilized.
//
// (MHL_FEATURE_RAP_SUPPORT & mhlTxConfig.devCapCache.mdc.featureFlag))
// (MHL_FEATURE_SP_SUPPORT & mhlTxConfig.devCapCache.mdc.featureFlag))
//
//
bool_t SiiMhlTxRcpSend( uint8_t rcpKeyCode )
{
        bool_t retVal;
        //
        // If peer does not support do not send RCP or RCPK/RCPE commands
        //

        if((0 == (MHL_FEATURE_RCP_SUPPORT & mhlTxConfig.devCapCache.mdc.featureFlag))
                        ||
                        !(FLAGS_RCP_READY & mhlTxConfig.miscFlags)
          )
        {
                TXC_DEBUG_PRINT (("MhlTx:SiiMhlTxRcpSend failed\n"));
                retVal=false;
        }

        retVal=MhlTxSendMscMsg ( MHL_MSC_MSG_RCP, rcpKeyCode );
        if(retVal)
        {
                TXC_DEBUG_PRINT (("MhlTx:SiiMhlTxRcpSend\n"));
                IncrementCBusReferenceCount(SiiMhlTxRcpSend)
                MhlTxDriveStates();
        }
        return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRcpkSend
//
// This function sends RCPK to the peer device.
//
bool_t SiiMhlTxRcpkSend( uint8_t rcpKeyCode )
{
        bool_t	retVal;

        retVal = MhlTxSendMscMsg(MHL_MSC_MSG_RCPK, rcpKeyCode);
        if(retVal)
        {
                MhlTxDriveStates();
        }
        return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRcpeSend
//
// The function will return a value of true if it could successfully send the RCPE
// subcommand. Otherwise false.
//
// When successful, MhlTx internally sends RCPK with original (last known)
// keycode.
//
bool_t SiiMhlTxRcpeSend( uint8_t rcpeErrorCode )
{
        bool_t	retVal;

        retVal = MhlTxSendMscMsg(MHL_MSC_MSG_RCPE, rcpeErrorCode);
        if(retVal)
        {
                MhlTxDriveStates();
        }
        return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxUcpSend
//
// This function checks if the peer device supports UCP and sends ucpKeyCode. The
// function will return a value of true if it could successfully send the UCP
// subcommand and the key code. Otherwise false.
//
// The followings are not yet utilized.
//
// (MHL_FEATURE_RAP_SUPPORT & mhlTxConfig.devCapCache.mdc.featureFlag))
// (MHL_FEATURE_SP_SUPPORT & mhlTxConfig.devCapCache.mdc.featureFlag))
//
//
bool_t SiiMhlTxUcpSend( uint8_t ucpKeyCode )
{
        bool_t retVal;
        //
        // If peer does not support do not send UCP or UCPK/UCPE commands
        //

        if((0 == (MHL_FEATURE_UCP_RECV_SUPPORT & mhlTxConfig.devCapCache.mdc.featureFlag))
                        ||
                        !(FLAGS_RCP_READY & mhlTxConfig.miscFlags)  // Yes, FLAGS_RCP_READY is correct here.
          )
        {
                TXC_DEBUG_PRINT (("MhlTx:SiiMhlTxUcpSend failed\n"));
                retVal=false;
        }

        retVal=MhlTxSendMscMsg ( MHL_MSC_MSG_UCP, ucpKeyCode );
        if(retVal)
        {
                TXC_DEBUG_PRINT (("MhlTx:SiiMhlTxUcpSend\n"));
                IncrementCBusReferenceCount(SiiMhlTxUcpSend)
                MhlTxDriveStates();
        }
        return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxUcpkSend
//
// This function sends UCPK to the peer device.
//
bool_t SiiMhlTxUcpkSend( uint8_t ucpKeyCode )
{
        bool_t	retVal;
        retVal = MhlTxSendMscMsg(MHL_MSC_MSG_UCPK, ucpKeyCode);
        if(retVal)
        {
                MhlTxDriveStates();
        }
        return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxUcpeSend
//
// The function will return a value of true if it could successfully send the UCPE
// subcommand. Otherwise false.
//
// When successful, MhlTx internally sends UCPK with original (last known)
// keycode.
//
bool_t SiiMhlTxUcpeSend( uint8_t ucpeErrorCode )
{
        bool_t	retVal;

        retVal = MhlTxSendMscMsg(MHL_MSC_MSG_UCPE, ucpeErrorCode);
        if(retVal)
        {
                MhlTxDriveStates();
        }
        return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRapkSend
//
// This function sends RAPK to the peer device.
//
static	bool_t SiiMhlTxRapkSend( void )
{
        bool_t retVal;

        retVal = MhlTxSendMscMsg(MHL_MSC_MSG_RAPK, 0);
        if(retVal)
        {
                MhlTxDriveStates();
        }
        return retVal;
}

/*
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRapSend
//
// This function checks if the peer device supports RAP and sends rcpKeyCode. The
// function will return a value of true if it could successfully send the RCP
// subcommand and the key code. Otherwise false.
//

bool_t SiiMhlTxRapSend( uint8_t rapActionCode )
{
bool_t retVal;
    if (!(FLAGS_RCP_READY & mhlTxConfig.miscFlags))
    {
    	TXC_DEBUG_PRINT (("MhlTx:SiiMhlTxRapSend failed\n"));
        retVal = false;
    }
    else
    {
    	retVal = MhlTxSendMscMsg ( MHL_MSC_MSG_RAP, rapActionCode );
        if(retVal)
        {
            IncrementCBusReferenceCount
            TXC_DEBUG_PRINT (("MhlTx: SiiMhlTxRapSend\n"));
        }
    }
    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlWriteBurst
//
// This function is called by the driver to inform of arrival of a scratchpad data.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
// Application shall not call this function.
//
void	SiiMhlTxGotMhlWriteBurst( uint8_t *spadArray )
{
}
*/
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxSetDCapRdy
//
static bool_t SiiMhlTxSetDCapRdy( void )
{
        mhlTxConfig.connectedReady |= MHL_STATUS_DCAP_RDY;   // update local copy
        return SiiMhlTxSetStatus( MHL_STATUS_REG_CONNECTED_RDY, mhlTxConfig.connectedReady);
}


///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxReadDevcap
//
// This function sends a READ DEVCAP MHL command to the peer.
// It  returns true if successful in doing so.
//
// The value of devcap should be obtained by making a call to SiiMhlTxGetEvents()
//
// offset		Which byte in devcap register is required to be read. 0..0x0E
//
bool_t SiiMhlTxReadDevcap( uint8_t offset )
{
        bool_t retVal;
        cbus_req_t	req;
        CBUS_DEBUG_PRINT (("MhlTx:SiiMhlTxReadDevcap\n"));
        //
        // Send MHL_READ_DEVCAP command
        //
        req.retryCount  = 2;
        req.command     = MHL_READ_DEVCAP;
        req.offsetData  = offset;
        req.payload_u.msgData[0]  = 0;  // do this to avoid confusion

        retVal = PutNextCBusTransaction(&req);

        MhlTxDriveStates();

        return retVal;
}

/*
 * SiiMhlTxRefreshPeerDevCapEntries
 */

static void SiiMhlTxRefreshPeerDevCapEntries(void)
{
        if (MHL_STATUS_DCAP_RDY & mhlTxConfig.status_0)
        {
                CBUS_DEBUG_PRINT(("refreshing DEVCAP\n"));
                // only issue if no existing refresh is in progress
                if (DEVCAP_REFRESH_IDLE)
                {
                        UpdateDevCapCacheIndex(0)
                        SiiMhlTxReadDevcap( mhlTxConfig.ucDevCapCacheIndex );
                }
                else
                {
                        INFO_DEBUG_PRINT(("DCAP_REFRESH_ACTIVE index:%d\n",(uint16_t)mhlTxConfig.ucDevCapCacheIndex));
                }
        }
        else
        {
                ERROR_DEBUG_PRINT(("no DCAP_RDY\n"));
        }
}

///////////////////////////////////////////////////////////////////////////////
//
// MhlTxSendMscMsg
//
// This function sends a MSC_MSG command to the peer.
// It  returns true if successful in doing so.
//
// The value of devcap should be obtained by making a call to SiiMhlTxGetEvents()
//
// offset		Which byte in devcap register is required to be read. 0..0x0E
//
static bool_t MhlTxSendMscMsg ( uint8_t command, uint8_t cmdData )
{
        cbus_req_t	req;
        uint8_t		ccode;

        //
        // Send MSC_MSG command
        //
        // Remember last MSC_MSG command (RCPE particularly)
        //
        req.retryCount  = 2;
        req.command     = MHL_MSC_MSG;
        req.payload_u.msgData[0]  = command;
        req.payload_u.msgData[1]  = cmdData;
        ccode = PutNextCBusTransaction(&req);
        return( (bool_t) ccode );
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxNotifyConnection
//
//
void	SiiMhlTxNotifyConnection( bool_t mhlConnected )
{
        mhlTxConfig.mhlConnectionEvent = true;

        TXC_DEBUG_PRINT(("MhlTx: SiiMhlTxNotifyConnection MSC_STATE_IDLE %01X\n", (int) mhlConnected ));

        if(mhlConnected)
        {
                mhlTxConfig.rapFlags |= RAP_CONTENT_ON;
                TXC_DEBUG_PRINT(("RAP CONTENT_ON\n"));
                mhlTxConfig.mhlConnected = MHL_TX_EVENT_CONNECTION;
                mhlTxConfig.mhlHpdRSENflags |= MHL_RSEN;
#if 0 //(
                SiiMhlTxTmdsEnable();
#else //)(
                INFO_DEBUG_PRINT(("'would have called SiiMhlTxTmdsEnable here\n"));
#endif //)
        }
        else
        {
                mhlTxConfig.rapFlags &= ~RAP_CONTENT_ON;
                TXC_DEBUG_PRINT(("RAP CONTENT_OFF\n"));
                mhlTxConfig.mhlConnected = MHL_TX_EVENT_DISCONNECTION;
                mhlTxConfig.mhlHpdRSENflags &= ~MHL_RSEN;
        }
        MhlTxProcessEvents();
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxNotifyDsHpdChange
// Driver tells about arrival of SET_HPD or CLEAR_HPD by calling this function.
//
// Turn the content off or on based on what we got.
//
void	SiiMhlTxNotifyDsHpdChange( uint8_t dsHpdStatus )
{
        if( 0 == dsHpdStatus )
        {
                TXC_DEBUG_PRINT(("MhlTx: Got HPD = LOW from DS\n"));
                mhlTxConfig.mhlHpdRSENflags &= ~(MHL_HPD );

                // MN: 1/11/2013: Normally we should have forgotten previous AVIF on DS CLR_HPD.
                // But a workaround forces us to memorize last AVIF
//		mhlTxConfig.flags.validAviInfoFrame = 0;

                UpdateEdidLoopActiveStatus(0)

                SiiMhlTxResetEdidStatus();

                // indicate DEVCAP refresh not started.
                UpdateDevCapCacheIndex( 1 + sizeof(mhlTxConfig.devCapCache.aucDevCapCache))
                AppNotifyMhlDownStreamHPDStatusChange(dsHpdStatus);

                SiiMhlTxDrvTmdsControl( false );
        }
        else
        {
                TXC_DEBUG_PRINT(("MhlTx: Got HPD = HIGH from DS\n"));
                mhlTxConfig.mhlHpdRSENflags |= MHL_HPD;
                AppNotifyMhlDownStreamHPDStatusChange(dsHpdStatus);

#if 0 //(
                // DEVCAP is stale -- refresh now.
                UpdateDevCapCacheIndex(sizeof(mhlTxConfig.devCapCache.aucDevCapCache));
                CBUS_DEBUG_PRINT(("refreshing DEVCAP\n"));
                SiiMhlTxRefreshPeerDevCapEntries();
#else //)(
                // see MHL spec section 5.9.1
                if ( DEVCAP_REFRESH_COMPLETE )
                {
                        SiiMhlTxInitiateEdidSequence();
                }
                else
                {
                        CBUS_DEBUG_PRINT(("refreshing DEVCAP\n"));
                        SiiMhlTxRefreshPeerDevCapEntries();
                }
#endif //)
        }
}

void SiiMhlTxNoAvi( void )
{
//uint8_t *pTemp,i;
        // MN: 1/11/2013: Normally we should have forgotten previous AVIF on DS CLR_HPD.
        // But a workaround forces us to memorize last AVIF
//    mhlTxConfig.flags.validAviInfoFrame = 0;
        /*
            for (pTemp=(uint8_t *)&mhlTxConfig.currentAviInfoFrame,i=0;i<sizeof(mhlTxConfig.currentAviInfoFrame);++i)
            {
                *pTemp++=0;
            }
        */

}

void SiiMhlTxNoVsi( void )
{
        uint8_t *pTemp,i;
        mhlTxConfig.flags.validVendorSpecificInfoFrame = 0;
        for (pTemp=(uint8_t *)&mhlTxConfig.currentVendorSpecificInfoFrame,i=0; i<sizeof(mhlTxConfig.currentVendorSpecificInfoFrame); ++i)
        {
                *pTemp++=0;
        }
}
///////////////////////////////////////////////////////////////////////////////
//
// MhlTxResetStates
//
// Application picks up mhl connection and rcp events at periodic intervals.
// Interrupt handler feeds these variables. Reset them on disconnection.
//
static void	MhlTxResetStates( void )
{
        mhlTxConfig.mhlConnectionEvent	= false;
        mhlTxConfig.mhlConnected		= MHL_TX_EVENT_DISCONNECTION;
        mhlTxConfig.mhlHpdRSENflags     &= ~(MHL_RSEN | MHL_HPD );
        mhlTxConfig.rapFlags 			&= ~RAP_CONTENT_ON;
        TXC_DEBUG_PRINT(("RAP CONTENT_OFF\n"));
        mhlTxConfig.mscMsgArrived		= false;


        mhlTxConfig.status_0            = 0;
        mhlTxConfig.status_1            = 0;
        mhlTxConfig.connectedReady      = 0;
        mhlTxConfig.linkMode            = MHL_STATUS_CLK_MODE_NORMAL; // indicate normal (24-bit) mode
        mhlTxConfig.preferredClkMode	= MHL_STATUS_CLK_MODE_NORMAL;  // this can be overridden by the application calling SiiMhlTxSetPreferredPixelFormat()
        mhlTxConfig.cbusReferenceCount  = 0;
        mhlTxConfig.miscFlags           = 0;
        mhlTxConfig.mscLastCommand      = 0;
        mhlTxConfig.mscMsgLastCommand   = 0;

        UpdateEdidLoopActiveStatus(0)
        mhlTxConfig.flags.abort3DReq=0;

        SiiMhlTxNoAvi();
        SiiMhlTxNoVsi();

        EDID_Data.sinkType = sink_type_pending;

        UpdateDevCapCacheIndex(1 + sizeof(mhlTxConfig.devCapCache.aucDevCapCache))

}

/*
    SiiTxReadConnectionStatus
    returns:
    0: if not fully connected
    1: if fully connected
*/
uint8_t    SiiTxReadConnectionStatus(void)
{
        return (mhlTxConfig.mhlConnected >= MHL_TX_EVENT_RCP_READY)?1:0;
}

/*
  SiiMhlTxSetPreferredPixelFormat

	clkMode - the preferred pixel format for the CLK_MODE status register

	Returns: 0 -- success
		     1 -- failure - bits were specified that are not within the mask
 */
uint8_t SiiMhlTxSetPreferredPixelFormat(uint8_t clkMode)
{
        if (~MHL_STATUS_CLK_MODE_MASK & clkMode)
        {
                // bits were specified that are not within the mask
                return 1;
        }
        else
        {
                mhlTxConfig.preferredClkMode = clkMode;

                // check to see if a refresh has happened since the last call
                //   to MhlTxResetStates()
                if (DEVCAP_REFRESH_ONCE)
                {
                        // check to see if DevCap cache update has already updated this yet.
                        if (mhlTxConfig.ucDevCapCacheIndex > DEVCAP_OFFSET_VID_LINK_MODE)
                        {
                                ExamineLocalAndPeerVidLinkMode();
                        }
                }
                return 0;
        }
}

/*
	SiiTxGetPeerDevCapEntry
	index -- the devcap index to get
	*pData pointer to location to write data
	returns
		0 -- success
		1 -- busy.
 */
uint8_t SiiTxGetPeerDevCapEntry(uint8_t index,uint8_t *pData)
{
        if (DEVCAP_REFRESH_ACTIVE)
        {
                // update is in progress
                return 1;
        }
        else
        {
                *pData = mhlTxConfig.devCapCache.aucDevCapCache[index];
                return 0;
        }
}

/*
	SiiGetScratchPadVector
	offset -- The beginning offset into the scratch pad from which to fetch entries.
	length -- The number of entries to fetch
	*pData -- A pointer to an array of bytes where the data should be placed.

	returns:
		ScratchPadStatus_e see si_mhl_tx_api.h for details

*/
ScratchPadStatus_e SiiGetScratchPadVector(uint8_t offset,uint8_t length, uint8_t *pData)
{
        if (!(MHL_FEATURE_SP_SUPPORT & mhlTxConfig.devCapCache.mdc.featureFlag))
        {
                TXC_DEBUG_PRINT (("MhlTx:SiiMhlTxRequestWriteBurst failed SCRATCHPAD_NOT_SUPPORTED\n" ));
                return SCRATCHPAD_NOT_SUPPORTED;
        }
        else if (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)
        {
                return SCRATCHPAD_BUSY;
        }
        else if ((offset >= sizeof(mhlTxConfig.incomingScratchPad)) ||
                        (length > (sizeof(mhlTxConfig.incomingScratchPad)- offset)) )
        {
                return SCRATCHPAD_BAD_PARAM;
        }
        else
        {
                uint8_t i,reg;
                uint8_t *pIncoming = (uint8_t *)&mhlTxConfig.incomingScratchPad;
                for (i = 0,reg=offset; (i < length) && (reg < sizeof(mhlTxConfig.incomingScratchPad)); ++i,++reg)
                {
                        pData[i] = pIncoming[reg];
                }
                return SCRATCHPAD_SUCCESS;
        }
}

void SiiMhlTxNotifyRgndMhl( void )
{
        if (MHL_TX_EVENT_STATUS_PASSTHROUGH == AppNotifyMhlEvent(MHL_TX_EVENT_RGND_MHL, 0,NULL))
        {
                // Application layer did not claim this, so send it to the driver layer
                SiiMhlTxDrvProcessRgndMhl();
        }
}

void SiiMhlTxHwReset(uint16_t hwResetPeriod,uint16_t hwResetDelay)
{
        AppResetMhlTx(hwResetPeriod,hwResetDelay );
}

/*
    SiiMhlTxGetDeviceId
    returns chip Id
 */
uint16_t SiiMhlTxGetDeviceId(void)
{
        return SiiMhlTxDrvGetDeviceId();
}

/*
    SiiMhlTxGetDeviceRev
    returns chip revision
 */
uint8_t SiiMhlTxGetDeviceRev(void)
{
        return SiiMhlTxDrvGetDeviceRev();
}


#ifdef PASS_980TEST
uint8_t SiiMhlTxIncomingInfoFrameChanged(PAVIInfoFrame_t pAviInfoFrame)
{
        uint8_t *pData = (uint8_t *)pAviInfoFrame;
        AviInfoFrameDataByte4_t inputVideoCode;
        AviInfoFrameDataByte2_t colAspRat;

        mhlTxConfig.currentAviInfoFrame = *pAviInfoFrame;

        mhlTxConfig.currentAviInfoFrame.header.type_code=pData[0];

        colAspRat.ActiveFormatAspectRatio=(pData[5]&0x0F);
        colAspRat.PictureAspectRatio=((pData[5]&0x30)>>4);
        colAspRat.Colorimetry=((pData[5]&0x0C0)>>6);

        mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC=pData[7]&0x7F;
        TXD_DEBUG_PRINT(("mhlTxConfig.currentAviInfoFrame.header.type_code=%02X,\n	mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC=0x%x;\n",
                         mhlTxConfig.currentAviInfoFrame.header.type_code,
                         mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC));
        TXD_DEBUG_PRINT(("colAspRat.ActiveFormatAspectRatio=%02X,\n	colAspRat.PictureAspectRatio=0x%x;colAspRat.Colorimetryo=0x%x;\n",
                         colAspRat.ActiveFormatAspectRatio,colAspRat.PictureAspectRatio, colAspRat.Colorimetry));

        inputVideoCode          = mhlTxConfig.currentAviInfoFrame.payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC;
        SiiMhlTxDrvSetInputVideoCode(inputVideoCode);
        SiiMhlTxDrvSetColorimetryAspectRatio(colAspRat);
        TXD_DEBUG_PRINT(("inputVideoCode=%02X,\n",
                         inputVideoCode.VIC));
        return 1;
}
#endif
/*
	SiiMhlTxProcessAviInfoFrameChange
		optionally called by the MHL Tx driver when a
        new AVI info frame is received from upstream.
*/
static int SiiMhlTxProcessAviInfoFrameChange(PAVIInfoFrame_t pAviInfoFrame)
{
        mhlTxConfig.flags.validAviInfoFrame = IsValidAviInfoFrame(pAviInfoFrame);
        DumpIncomingInfoFrame(pAviInfoFrame,sizeof(*pAviInfoFrame));

        INFO_DEBUG_PRINT(("VIC = %d, mhlTxConfig.flags.validAviInfoFrame=%d\n",(uint16_t)pAviInfoFrame->payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC,
                          mhlTxConfig.flags.validAviInfoFrame));
#ifdef PASS_980TEST
        SiiMhlTxIncomingInfoFrameChanged(pAviInfoFrame);// add by Garyyuan for 980 test
#endif
        if (mhlTxConfig.flags.validAviInfoFrame)
        {
                INFO_DEBUG_PRINT(("AVIF VIC: %d\n"
                                  ,(uint16_t)pAviInfoFrame->payLoad.hwPayLoad.namedIfData.ifData_u.bitFields.VIC.VIC));

                mhlTxConfig.currentAviInfoFrame = *pAviInfoFrame;
                AppNotifyMhlEvent(MHL_TX_EVENT_INFO_FRAME_CHANGE, InfoFrameType_AVI,(void *)pAviInfoFrame);
                return 1;
        }
        return 0;
}

/*
	SiiMhlTxProcessVendorSpecificInfoFrameChange
		optionally called by the MHL Tx driver when a
        new Vendor specific info frame is received from upstream.
*/
static int SiiMhlTxProcessVendorSpecificInfoFrameChange(PVendorSpecificInfoFrame_t pVsInfoFrame)
{
        mhlTxConfig.flags.validVendorSpecificInfoFrame = IsValidVendorSpecificInfoFrame(pVsInfoFrame);

        if (mhlTxConfig.flags.validVendorSpecificInfoFrame)
        {
                if (0!=vsifCmp(pVsInfoFrame,&mhlTxConfig.currentVendorSpecificInfoFrame))
                {
                        mhlTxConfig.currentVendorSpecificInfoFrame    = *pVsInfoFrame;
                        DumpIncomingInfoFrame(pVsInfoFrame,sizeof(*pVsInfoFrame));
                        return 1;
                }
        }

        return 0;
}
void SiiMhlTxProcessInfoFrameChange(PVendorSpecificInfoFrame_t pVsInfoFrame,
                                    PAVIInfoFrame_t pAviInfoFrame)
{
        int modeChange=0;
        if (NULL != pVsInfoFrame)
        {
                modeChange = SiiMhlTxProcessVendorSpecificInfoFrameChange(pVsInfoFrame);
        }
        if (NULL != pAviInfoFrame)
        {
                modeChange |= SiiMhlTxProcessAviInfoFrameChange(pAviInfoFrame);
        }

        if (modeChange)
        {
                // Disable TMDS
                SiiMhlTxDrvTmdsControl(false);

                // if it is time yet, this will restart HDCP
                SiiMhlTxTmdsEnable();
        }
#ifdef DO_HDCP
        else if (AUTH_IDLE == SiiMhlTxLiteDrvGetAuthenticationState())
#else
        else
#endif
        {
                // if it is time yet, this will restart HDCP
                SiiMhlTxTmdsEnable();
        }
#ifdef	NEVER
        else
        {
                ERROR_DEBUG_PRINT(("info frame change, doing nothing: %d\n",(int)SiiMhlTxLiteDrvGetAuthenticationState()));
        }
#endif	// NEVER
}
/*
    SiiMhlTxPreInitSetModeFlags
*/
void SiiMhlTxPreInitSetModeFlags(uint8_t modeFlags)
{
        mhlTxConfig.modeFlags = modeFlags;
        SiiMhlTxDrvPreInitSetModeFlags(modeFlags);
}


void SiiMhlTxNotifyAuthentication( void )
{
        SiiMhlTxDrvVideoUnmute( );
#if 1//ndef INFO_FRAMES_AT_TMDS_ENABLE //(	
        SiiHmdiTxLiteDrvSendHwAviInfoFrame( );
        //SendAudioInfoFrame();
#endif //)

}

void SiiMhlTxNotifySyncDetect(uint8_t state)
{
        mhlTxConfig.flags.syncDetect = state?1:0;
        INFO_DEBUG_PRINT(("%sSyncDetect\n",mhlTxConfig.flags.syncDetect?"":"No "));
        if (state)
        {
                SiiMhlTxTmdsEnable();
        }
        else
        {
                if (!(mhlTxConfig.modeFlags & mfTranscodeMode))
                {
#ifdef DO_HDCP
                        // Disable HDCP
                        SiiHdmiTxLiteDisableEncryption();
#endif
                }
        }
}

void SiiMhlTxNotifyUpStreamHPD(uint8_t state)
{
        // info frame buffer contents stay valid across disconnects (both MHL and HDMI)
        mhlTxConfig.flags.upStreamHPD = state?1:0;
}


SinkType_e SiiMhlTxGetSinkType( void )
{
        return EDID_Data.sinkType;
}

int SiiMhlTxEdidActive( void )
{
        return (mhlTxConfig.flags.edidLoopActive)?1:0;
}

void AppDisplayTimingEnumerationBegin(void)
{
        TXC_DEBUG_PRINT(("Begin MHL display timing enumeration\n"));
}
#if 0
void AppDisplayTimingEnumerationCallBack(uint16_t columns, uint16_t rows, uint8_t bitsPerPixel, uint32_t verticalRefreshRateInMilliHz,Mhl2VideoDescriptor_t mhl2VideoDescriptor)
{
        // avoid compiler warnings
        columns                     = columns;
        rows                        = rows;
        bitsPerPixel                = bitsPerPixel;
        verticalRefreshRateInMilliHz= verticalRefreshRateInMilliHz;
        mhl2VideoDescriptor         = mhl2VideoDescriptor;

        TXC_DEBUG_PRINT(("%4d x %4d, %2d bpp at %lu Hz 3D - %16s %16s %16s\n\n"
                         ,columns
                         ,rows
                         ,(uint16_t)bitsPerPixel
                         ,verticalRefreshRateInMilliHz
                         ,mhl2VideoDescriptor.LeftRight      ?"Left/Right"      :""
                         ,mhl2VideoDescriptor.TopBottom      ?"Top/Bottom"      :""
                         ,mhl2VideoDescriptor.FrameSequential?"Frame Sequential":""
                        ));
}
#else
void AppDisplayTimingEnumerationCallBack(uint16_t columns, uint16_t rows, uint8_t bitsPerPixel, uint32_t verticalRefreshRateInMilliHz,Mhl2VideoDescriptor_t mhl2VideoDescriptor)
{
}
#endif

void AppDisplayTimingEnumerationEnd(void)
{
        TXC_DEBUG_PRINT(("End MHL display timing enumeration\n"));
}


///////////////////////////////////////////////////////////////////////////////
//
// CBusQueueReset
//
void CBusQueueReset(void)
{

        TXD_DEBUG_PRINT (("MhlTx:CBusQueueReset\n"));

        memset(&CBusQueue,0,sizeof(CBusQueue));
        // reset queue of pending CBUS requests.
        CBusQueue.header.head = 0;
        CBusQueue.header.tail = 0;
        //QUEUE_DEPTH(CBusQueue)=0;
}
