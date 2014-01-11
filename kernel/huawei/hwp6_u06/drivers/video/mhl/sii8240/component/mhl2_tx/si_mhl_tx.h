/**********************************************************************************/
/*  Copyright (c) 2011, Silicon Image, Inc.  All rights reserved.                 */
/*  No part of this work may be reproduced, modified, distributed, transmitted,   */
/*  transcribed, or translated into any language or computer format, in any form  */
/*  or by any means without written permission of: Silicon Image, Inc.,           */
/*  1140 East Arques Avenue, Sunnyvale, California 94085                          */
/**********************************************************************************/
/*
   @file si_mhl_tx.h
*/

#define SCRATCHPAD_SIZE 16
typedef union
{
        Mhl2VideoFormatData_t   videoFormatData;
        uint8_t     asBytes[SCRATCHPAD_SIZE];
} ScratchPad_u,*PScratchPad_u;

typedef struct _BitFieldFlags_t
{
        unsigned char   syncDetect                      :1;
        unsigned char   upStreamHPD                     :1;
        unsigned char	abort3DReq	                    :1;
        unsigned char   validVendorSpecificInfoFrame    :1;
        unsigned char   validAviInfoFrame               :1;
#ifdef INTERRUPT_DRIVEN_EDID //(
        unsigned char   edidLoopActive                  :1;
        unsigned char   reserved                        :2;
#else //)(
        unsigned char   reserved                        :3;
#endif //)
} BitFieldFlags_t,*PBitFieldFlags_t;

#define NUM_VIDEO_DATA_BLOCKS_LIMIT 3

//
// structure to hold operating information of MhlTx component
//
typedef struct
{
        uint8_t		pollIntervalMs;		// Remember what app set the polling frequency as.

        uint8_t		status_0;			// Received status from peer is stored here
        uint8_t		status_1;			// Received status from peer is stored here

        uint8_t     connectedReady;     // local MHL CONNECTED_RDY register value
        uint8_t     linkMode;           // local MHL LINK_MODE register value
        uint8_t     mhlHpdStatus;       // keep track of SET_HPD/CLR_HPD
        uint8_t     mhlRequestWritePending;

        bool_t		mhlConnectionEvent;
        uint8_t		mhlConnected;

        uint8_t     mhlHpdRSENflags;       // keep track of SET_HPD/CLR_HPD

        // mscMsgArrived == true when a MSC MSG arrives, false when it has been picked up
        bool_t		mscMsgArrived;
        uint8_t		mscMsgSubCommand;
        uint8_t		mscMsgData;

        uint8_t     cbusReferenceCount;  // keep track of CBUS requests
        // Remember last command, offset that was sent.
        // Mostly for READ_DEVCAP command and other non-MSC_MSG commands
        uint8_t		mscLastCommand;
        uint8_t		mscLastOffset;
        uint8_t		mscLastData;

        // Remember last MSC_MSG command (RCPE particularly)
        uint8_t		mscMsgLastCommand;
        uint8_t		mscMsgLastData;
        uint8_t		mscSaveRcpKeyCode;
        uint8_t		mscSaveUcpKeyCode;

        //  support WRITE_BURST
        ScratchPad_u    incomingScratchPad;
        ScratchPad_u    outgoingScratchPad;
        uint8_t     burstEntryCount3D_VIC;
        uint8_t     vic2Dindex;
        uint8_t     vic3Dindex;
        uint8_t     burstEntryCount3D_DTD;
        uint8_t     vesaDTDindex;
        uint8_t     cea861DTDindex;
        uint16_t     miscFlags;          // such as SCRATCHPAD_BUSY
        uint8_t     modeFlags;

        uint8_t		ucDevCapCacheIndex;

        MHLDevCap_u devCapCache;

        uint8_t		rapFlags;		// CONTENT ON/OFF
        uint8_t		preferredClkMode;

        PVendorSpecificDataBlock_t pHdmiVendorSpecificDataBlock;
        PVideoDataBlock_t pVideoDataBlock2D[NUM_VIDEO_DATA_BLOCKS_LIMIT];
        PVideoCapabilityDataBlock_t pVideoCapabilityDataBlock;
        PVSDBByte13ThroughByte15_t pByte13ThroughByte15;
        P_3D_Mask_t p3DMask;
        P_3DStructureAndDetailEntry_u    pThreeD;
        uint8_t    *p3DLimit;
        uint8_t     num_video_data_blocks; // counter for initial EDID parsing, persists afterwards
        uint8_t     video_data_block_index; // counter for 3D write burst parsing.

        AVIInfoFrame_t currentAviInfoFrame;
        VendorSpecificInfoFrame_t currentVendorSpecificInfoFrame;
        BitFieldFlags_t flags;
        uint8_t     numEdidExtensions;
} mhlTx_config_t;

// bits for mhlHpdRSENflags:
typedef enum
{
        MHL_HPD            = 0x01
                             , MHL_RSEN           = 0x02
} MhlHpdRSEN_e;

typedef enum
{
        FLAGS_SCRATCHPAD_BUSY         = 0x0001
                                        , FLAGS_REQ_WRT_PENDING         = 0x0002
                                                        , FLAGS_WRITE_BURST_PENDING     = 0x0004
                                                                        , FLAGS_RCP_READY               = 0x0008
                                                                                        , FLAGS_HAVE_DEV_CATEGORY       = 0x0010
                                                                                                        , FLAGS_HAVE_DEV_FEATURE_FLAGS  = 0x0020
                                                                                                                        , FLAGS_HAVE_COMPLETE_DEVCAP    = 0x0040
                                                                                                                                        , FLAGS_SENT_DCAP_RDY           = 0x0080
                                                                                                                                                        , FLAGS_SENT_PATH_EN            = 0x0100
                                                                                                                                                                        , FLAGS_SENT_3D_REQ             = 0x0200
                                                                                                                                                                                        , FLAGS_BURST_3D_VIC_DONE       = 0x0400
                                                                                                                                                                                                        , FLAGS_BURST_3D_DTD_DONE       = 0x0800
                                                                                                                                                                                                                        , FLAGS_BURST_3D_DTD_VESA_DONE  = 0x1000
                                                                                                                                                                                                                                        , FLAGS_BURST_3D_DONE           = 0x2000
                                                                                                                                                                                                                                                        , FLAGS_EDID_READ_DONE          = 0x4000
} MiscFlags_e;
typedef enum
{
        RAP_CONTENT_ON = 0x01
} rapFlags_e;


