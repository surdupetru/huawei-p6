

SI_PUSH_STRUCT_PACKING //(
typedef struct SI_PACK_THIS_STRUCT _TwoBytes_t
{
        unsigned char low;
        unsigned char high;
} TwoBytes_t,*PTwoBytes_t;


#define EDID_EXTENSION_TAG  0x02
#define EDID_EXTENSION_BLOCK_MAP 0xF0
#define EDID_REV_THREE      0x03
#define EDID_BLOCK_SIZE      128
#define EDID_BLOCK_0        0x00
#define EDID_BLOCK_2_3      0x01

typedef enum
{
        dbtcTerminator                 = 0
                        ,dbtcAudioDataBlock             = 1
                                        ,dbtcVideoDataBlock             = 2
                                                        ,dbtcVendorSpecificDataBlock    = 3
                                                                        ,dbtcSpeakerAllocationDataBlock = 4
                                                                                        ,dbtcVesaDtcDataBlock           = 5
                                                                                                        //reserved                      = 6
                                                                                                        ,dbtcUsedExtendedTag            = 7
} DataBlockTagCode_e;
typedef struct SI_PACK_THIS_STRUCT _DataBlockHeaderByte_t
{
        uint8_t lengthFollowingHeader:5;
        DataBlockTagCode_e tagCode:3;
} DataBlockHeaderByte_t,*PDataBlockHeaderByte_t;

typedef enum
{
        etcVideoCapabilityDataBlock                    =  0
                        ,etcVendorSpecificVideoDataBlock                =  1
                                        ,etcVESAVideoDisplayDeviceInformationDataBlock  =  2
                                                        ,etcVESAVideoDataBlock                          =  3
                                                                        ,etcHDMIVideoDataBlock                          =  4
                                                                                        ,etcColorimetryDataBlock                        =  5
                                                                                                        ,etcVideoRelated                                =  6

                                                                                                                        ,etcCEAMiscAudioFields                          = 16
                                                                                                                                        ,etcVendorSpecificAudioDataBlock                = 17
                                                                                                                                                        ,etcHDMIAudioDataBlock                          = 18
                                                                                                                                                                        ,etcAudiorRelated                               = 19

                                                                                                                                                                                        ,etcGeneral                                     = 32
} ExtendedTagCode_e;

typedef struct SI_PACK_THIS_STRUCT _ExtendedTagCode_t
{
        ExtendedTagCode_e extendedTagCode:8;
} ExtendedTagCode_t,*PExtendedTagCode_t;

typedef struct SI_PACK_THIS_STRUCT _CeaShortDescriptor_t
{
        unsigned char VIC:7;
        unsigned char native:1;

} CeaShortDescriptor_t,*PCeaShortDescriptor_t;

typedef struct SI_PACK_THIS_STRUCT  _MHLShortDesc_t
{
        CeaShortDescriptor_t ceaShortDesc;
        Mhl2VideoDescriptor_t mhlVidDesc;
} MHLShortDesc_t,*PMHLShortDesc_t;

typedef struct SI_PACK_THIS_STRUCT _VideoDataBlock_t
{
        DataBlockHeaderByte_t header;
        CeaShortDescriptor_t shortDescriptors[1];//open ended
} VideoDataBlock_t,*PVideoDataBlock_t;

typedef enum
{
        // reserved             =  0
        afdLinearPCM_IEC60958   =  1
                                   ,afdAC3                 =  2
                                                   ,afdMPEG1Layers1_2      =  3
                                                                   ,afdMPEG1Layer3         =  4
                                                                                   ,afdMPEG2_MultiChannel  =  5
                                                                                                   ,afdAAC                 =  6
                                                                                                                   ,afdDTS                 =  7
                                                                                                                                   ,afdATRAC               =  8
                                                                                                                                                   ,afdOneBitAudio         =  9
                                                                                                                                                                   ,afdDolbyDigital        = 10
                                                                                                                                                                                   ,afdDTS_HD              = 11
                                                                                                                                                                                                   ,afdMAT_MLP             = 12
                                                                                                                                                                                                                   ,afdDST                 = 13
                                                                                                                                                                                                                                   ,afdWMAPro              = 14
                                                                                                                                                                                                                                                   //reserved              = 15
} AudioFormatCodes_e;

typedef struct SI_PACK_THIS_STRUCT _CeaShortAudioDescriptor_t
{
        unsigned char       maxChannelsMinusOne :3;
        AudioFormatCodes_e  audioFormatCode     :4;
        unsigned char       F17                 :1;

        unsigned char freq32Khz   :1;
        unsigned char freq44_1KHz :1;
        unsigned char freq48KHz   :1;
        unsigned char freq88_2KHz :1;
        unsigned char freq96KHz   :1;
        unsigned char freq176_4KHz:1;
        unsigned char freq192Khz  :1;
        unsigned char F27         :1;

        union
        {
                struct
                {
                        unsigned char res16bit:1;
                        unsigned char res20bit:1;
                        unsigned char res24bit:1;
                        unsigned char F33_37:5;
                } audioCode1LPCM;
                struct
                {
                        uint8_t maxBitRateDivBy8KHz;
                } audioCodes2_8;
                struct
                {
                        uint8_t defaultZero;
                } audioCodes9_15;
        } byte3;
} CeaShortAudioDescriptor_t,*PCeaShortAudioDescriptor_t;

typedef struct SI_PACK_THIS_STRUCT _AudioDataBlock_t
{
        DataBlockHeaderByte_t header;
        CeaShortAudioDescriptor_t  shortAudioDescriptors[1]; // open ended
} AudioDataBlock_t,*PAudioDataBlock_t;

typedef struct SI_PACK_THIS_STRUCT _SpeakerAllocationFlags_t
{
        unsigned char   spkFrontLeftFrontRight:1;
        unsigned char   spkLFE:1;
        unsigned char   spkFrontCenter:1;
        unsigned char   spkRearLeftRearRight:1;
        unsigned char   spkRearCenter:1;
        unsigned char   spkFrontLeftCenterFrontRightCenter:1;
        unsigned char   spkRearLeftCenterReadRightCenter:1;
        unsigned char   spkReserved:1;
} SpeakerAllocationFlags_t,*PSpeakerAllocationFlags_t;
typedef struct SI_PACK_THIS_STRUCT _SpeakerAllocationDataBlockPayLoad_t
{
        SpeakerAllocationFlags_t speakerAllocFlags;

        uint8_t         reserved[2];
} SpeakerAllocationDataBlockPayLoad_t,*PSpeakerAllocationDataBlockPayLoad_t;

typedef struct SI_PACK_THIS_STRUCT _SpeakerAllocationDataBlock_t
{
        DataBlockHeaderByte_t header;
        SpeakerAllocationDataBlockPayLoad_t payload;

} SpeakerAllocationDataBlock_t,*PSpeakerAllocationDataBlock_t;

typedef struct SI_PACK_THIS_STRUCT _HdmiLLC_BA_t
{
        unsigned char B:4;
        unsigned char A:4;
} HdmiLLC_BA_t,*PHdmiLLC_BA_t;

typedef struct SI_PACK_THIS_STRUCT _HdmiLLC_DC_t
{
        unsigned char D:4;
        unsigned char C:4;
} HdmiLLC_DC_t,*PHdmiLLC_DC_t;

typedef struct SI_PACK_THIS_STRUCT _HdmiLLC_Byte6_t
{
        unsigned char DVI_Dual      :1;
        unsigned char reserved      :2;
        unsigned char DC_Y444       :1;
        unsigned char DC_30bit      :1;
        unsigned char DC_36bit      :1;
        unsigned char DC_48bit      :1;
        unsigned char Supports_AI   :1;
} HdmiLLC_Byte6_t,*PHdmiLLC_Byte6_t;

typedef struct SI_PACK_THIS_STRUCT _HdmiLLC_Byte8_t
{
        unsigned char CNC0_AdjacentPixelsIndependent        :1;
        unsigned char CNC1_SpecificProcessingStillPictures  :1;
        unsigned char CNC2_SpecificProcessingCinemaContent  :1;
        unsigned char CNC3_SpecificProcessingLowAVLatency   :1;
        unsigned char reserved                              :1;
        unsigned char HDMI_VideoPresent                     :1;
        unsigned char I_LatencyFieldsPresent                :1;
        unsigned char LatencyFieldsPresent                  :1;
} HdmiLLC_Byte8_t,*PHdmiLLC_Byte8_t;

typedef enum
{
        imszNoAdditional                                    = 0
                        ,imszAspectRatioCorrectButNoGuarranteeOfCorrectSize = 1
                                        ,imszCorrectSizesRoundedToNearest1cm                = 2
                                                        ,imszCorrectSizesDividedBy5RoundedToNearest5cm      = 3
} ImageSize_e;

typedef struct SI_PACK_THIS_STRUCT _HdmiLLC_Byte13_t
{
        unsigned char reserved          :3;
        ImageSize_e Image_Size          :2;
        unsigned char _3D_Multi_present :2;
        unsigned char _3D_present       :1;
} HdmiLLC_Byte13_t,*PHdmiLLC_Byte13_t;

typedef struct SI_PACK_THIS_STRUCT _HdmiLLC_Byte14_t
{
        unsigned char HDMI_3D_LEN :5;
        unsigned char HDMI_VIC_LEN:3;
} HdmiLLC_Byte14_t,*PHdmiLLC_Byte14_t;

typedef struct SI_PACK_THIS_STRUCT _VSDBByte13ThroughByte15_t
{
        HdmiLLC_Byte13_t    byte13;
        HdmiLLC_Byte14_t    byte14;
        uint8_t vicList[1]; // variable length list base on HDMI_VIC_LEN
} VSDBByte13ThroughByte15_t,*PVSDBByte13ThroughByte15_t;

typedef struct SI_PACK_THIS_STRUCT _VSDBAllFieldsByte9ThroughByte15_t
{
        uint8_t Video_Latency;
        uint8_t Audio_Latency;
        uint8_t Interlaced_Video_Latency;
        uint8_t Interlaced_Audio_Latency;
        VSDBByte13ThroughByte15_t   byte13ThroughByte15;
        // There must be no fields after here
} VSDBAllFieldsByte9ThroughByte15_t,*PVSDBAllFieldsByte9ThroughByte15_t;

typedef struct SI_PACK_THIS_STRUCT _VSDBAllFieldsByte9ThroughByte15SansProgressiveLatency_t
{
        uint8_t Interlaced_Video_Latency;
        uint8_t Interlaced_Audio_Latency;
        VSDBByte13ThroughByte15_t   byte13ThroughByte15;
        // There must be no fields after here
} VSDBAllFieldsByte9ThroughByte15SansProgressiveLatency_t,*PVSDBAllFieldsByte9ThroughByte15SansProgressiveLatency_t;

typedef struct SI_PACK_THIS_STRUCT _VSDBAllFieldsByte9ThroughByte15SansInterlacedLatency_t
{
        uint8_t Video_Latency;
        uint8_t Audio_Latency;
        VSDBByte13ThroughByte15_t   byte13ThroughByte15;
        // There must be no fields after here
} VSDBAllFieldsByte9ThroughByte15SansInterlacedLatency_t,*PVSDBAllFieldsByte9ThroughByte15SansInterlacedLatency_t;

typedef struct SI_PACK_THIS_STRUCT _VSDBAllFieldsByte9ThroughByte15SansAllLatency_t
{
        VSDBByte13ThroughByte15_t   byte13ThroughByte15;
        // There must be no fields after here
} VSDBAllFieldsByte9ThroughByte15SansAllLatency_t,*PVSDBAllFieldsByte9ThroughByte15SansAllLatency_t;

typedef struct SI_PACK_THIS_STRUCT _HdmiLLCVendorSpecificDataBlockPayload_t
{
        HdmiLLC_BA_t B_A;
        HdmiLLC_DC_t D_C;
        HdmiLLC_Byte6_t byte6;
        uint8_t maxTMDSclock;
        HdmiLLC_Byte8_t byte8;
        union
        {
                VSDBAllFieldsByte9ThroughByte15SansAllLatency_t         vsdbAllFieldsByte9ThroughByte15SansAllLatency;
                VSDBAllFieldsByte9ThroughByte15SansProgressiveLatency_t vsdbAllFieldsByte9ThroughByte15SansProgressiveLatency;
                VSDBAllFieldsByte9ThroughByte15SansInterlacedLatency_t  vsdbAllFieldsByte9ThroughByte15SansInterlacedLatency;
                VSDBAllFieldsByte9ThroughByte15_t                       vsdbAllFieldsByte9ThroughByte15;
        } vsdbFieldsByte9ThroughByte15;
        // There must be no fields after here
} HdmiLLCVendorSpecificDataBlockPayload_t,*PHdmiLLCVendorSpecificDataBlockPayload_t;

typedef struct SI_PACK_THIS_STRUCT st_3D_Structure_ALL_15_8_t
{
        uint8_t framePacking    : 1;
        uint8_t reserved1       : 5;
        uint8_t topBottom       : 1;
        uint8_t reserved2       : 1;
} _3D_Structure_ALL_15_8_t,*P_3D_Structure_ALL_15_8_t;

typedef struct SI_PACK_THIS_STRUCT st_3D_Structure_ALL_7_0_t
{
        uint8_t sideBySide      : 1;
        uint8_t reserved        : 7;
} _3D_Structure_ALL_7_0_t,*P_3D_Structure_ALL_7_0_t;


typedef struct SI_PACK_THIS_STRUCT tag_3D_MultiPresent_01_or_10_t
{
        _3D_Structure_ALL_15_8_t _3D_Structure_ALL_15_8;
        _3D_Structure_ALL_7_0_t _3D_Structure_ALL_7_0;
} _3D_StructureAll_t,*P_3D_StructureAll_t;


typedef struct SI_PACK_THIS_STRUCT _3D_MultiPresent_01_10_t
{
        uint8_t _3D_Mask_15_8;
        uint8_t _3D_Mask_7_0;
} _3D_Mask_t,*P_3D_Mask_t;



typedef struct SI_PACK_THIS_STRUCT tag_2D_VIC_order_3D_Structure_t
{
        ThreeDStructure_e _3D_Structure:4;     // definition from info frame
        unsigned    char _2D_VIC_order:4;
} _2D_VIC_order_3D_Structure_t,*P_2D_VIC_order_3D_Structure_t;

typedef struct SI_PACK_THIS_STRUCT tag_3D_Detail_t
{
        unsigned char reserved    :4;
        unsigned char _3D_Detail  :4;
} _3D_Detail_t,*P_3D_Detail_t;

typedef struct SI_PACK_THIS_STRUCT tag_3DStructureAndDetailEntrySansByte1_t
{
        _2D_VIC_order_3D_Structure_t    byte0;
//see page 156 of HDMI spec 1.4 w.r.t presence of 3D_Detail_X and reserved(0) fields
} _3DStructureAndDetailEntrySansByte1_t,*P_3DStructureAndDetailEntrySansByte1_t;

typedef struct SI_PACK_THIS_STRUCT tag_3DStructureAndDetailEntryWithByte1_t
{
        _2D_VIC_order_3D_Structure_t    byte0;
        _3D_Detail_t                    byte1;
} _3DStructureAndDetailEntryWithByte1_t,*P_3DStructureAndDetailEntryWithByte1_t;

typedef union tag_3DStructureAndDetailEntry_u
{
        _3DStructureAndDetailEntrySansByte1_t   sansByte1;
        _3DStructureAndDetailEntryWithByte1_t   withByte1;
} _3DStructureAndDetailEntry_u,*P_3DStructureAndDetailEntry_u;

typedef struct SI_PACK_THIS_STRUCT _Hdmi3DSubBlockSansAllAndMask_t
{
        _3DStructureAndDetailEntry_u    _3DStructureAndDetailList[1];
} Hdmi3DSubBlockSansAllAndMask_t,*PHdmi3DSubBlockSansAllAndMask_t;

typedef struct SI_PACK_THIS_STRUCT _Hdmi3DSubBlockSansMask_t
{
        _3D_StructureAll_t  _3D_StructureAll;
        _3DStructureAndDetailEntry_u    _3DStructureAndDetailList[1];
} Hdmi3DSubBlockSansMask_t,*PHdmi3DSubBlockSansMask_t;

typedef struct SI_PACK_THIS_STRUCT _Hdmi3DSubBlockWithAllAndMask_t
{
        _3D_StructureAll_t  _3D_StructureAll;
        _3D_Mask_t          _3D_Mask;
        _3DStructureAndDetailEntry_u    _3DStructureAndDetailList[1];
} Hdmi3DSubBlockWithAllAndMask_t,*PHdmi3DSubBlockWithAllAndMask_t;

typedef union
{
        Hdmi3DSubBlockSansAllAndMask_t   hdmi3DSubBlockSansAllAndMask;
        Hdmi3DSubBlockSansMask_t         hdmi3DSubBlockSansMask;
        Hdmi3DSubBlockWithAllAndMask_t   hdmi3DSubBlockWithAllAndMask;
} Hdmi3DSubBlock_t,*PHdmi3DSubBlock_t;

typedef struct SI_PACK_THIS_STRUCT _VendorSpecificDataBlock_t
{
        DataBlockHeaderByte_t header;
        uint8_t IEEE_OUI[3];
        union
        {
                HdmiLLCVendorSpecificDataBlockPayload_t HdmiLLC;
                uint8_t payload[1]; // open ended
        } payload_u;
} VendorSpecificDataBlock_t,*PVendorSpecificDataBlock_t;

typedef enum
{
        xvYCC_601   = 1
                      ,xvYCC_709   = 2
} Colorimetry_xvYCC_e;

typedef struct SI_PACK_THIS_STRUCT _Colorimetry_xvYCC_t
{
        Colorimetry_xvYCC_e     xvYCC       :2;
        unsigned char           reserved1   :6;
} Colorimetry_xvYCC_t,*PColorimetry_xvYCC_t;

typedef struct SI_PACK_THIS_STRUCT _ColorimeteryMetaData_t
{
        unsigned char           metaData    :3;
        unsigned char           reserved2   :5;
} ColorimeteryMetaData_t,*PColorimeteryMetaData_t;

typedef struct SI_PACK_THIS_STRUCT _ColorimeteryDataPayLoad_t
{
        Colorimetry_xvYCC_t     ciData;
        ColorimeteryMetaData_t  cmMetaData;
} ColorimeteryDataPayLoad_t,*PColorimeteryDataPayLoad_t;
typedef struct SI_PACK_THIS_STRUCT _ColorimetryDataBlock_t
{
        DataBlockHeaderByte_t   header;
        ExtendedTagCode_t       extendedTag;
        ColorimeteryDataPayLoad_t payload;

} ColorimetryDataBlock_t,*PColorimetryDataBlock_t;

typedef enum
{
        ceouNeither                    = 0
                        ,ceouAlwaysOverScanned          = 1
                                        ,ceouAlwaysUnderScanned         = 2
                                                        ,ceouBoth                       = 3
} CEOverScanUnderScanBehavior_e;

typedef enum
{
        itouNeither                    = 0
                        ,itouAlwaysOverScanned          = 1
                                        ,itouAlwaysUnderScanned         = 2
                                                        ,itouBoth                       = 3
} ITOverScanUnderScanBehavior_e;

typedef enum
{
        ptouNeither                    = 0
                        ,ptouAlwaysOverScanned          = 1
                                        ,ptouAlwaysUnderScanned         = 2
                                                        ,ptouBoth                       = 3
} PTOverScanUnderScanBehavior_e;

typedef struct SI_PACK_THIS_STRUCT _VideoCapabilityDataPayLoad_t
{
        CEOverScanUnderScanBehavior_e S_CE     :2;
        ITOverScanUnderScanBehavior_e S_IT     :2;
        PTOverScanUnderScanBehavior_e S_PT     :2;
        unsigned char                 QS       :1;
        unsigned char                 reserved :1;
} VideoCapabilityDataPayLoad_t,*PVideoCapabilityDataPayLoad_t;

typedef struct SI_PACK_THIS_STRUCT _VideoCapabilityDataBlock_t
{
        DataBlockHeaderByte_t   header;
        ExtendedTagCode_t       extendedTag;
        VideoCapabilityDataPayLoad_t payload;

} VideoCapabilityDataBlock_t,*PVideoCapabilityDataBlock_t;

typedef struct SI_PACK_THIS_STRUCT _CeaDataBlockCollection_t
{
        DataBlockHeaderByte_t header;

        union
        {
                ExtendedTagCode_t extendedTag;
                CeaShortDescriptor_t shortDescriptor;
        } payload_u;
        // open ended array of CeaShortDescriptor_t starts here
} CeaDataBlockCollection_t,*PCeaDataBlockCollection_t;

typedef struct SI_PACK_THIS_STRUCT _CeaExtensionVersion1_t
{
        uint8_t reservedMustBeZero;
        uint8_t reserved[123];
} CeaExtensionVersion1_t,*PCeaExtensionVersion1_t;

typedef struct SI_PACK_THIS_STRUCT _CeaExtension2_3MiscSupport_t
{
        uint8_t totalNumberDetailedTimingDescriptorsInEntireEDID:4;
        uint8_t YCrCb422Support:1;
        uint8_t YCrCb444Support:1;
        uint8_t basicAudioSupport:1;
        uint8_t underscanITformatsByDefault:1;
} CeaExtension2_3MiscSupport_t,*PCeaExtension2_3MiscSupport_t;
typedef struct SI_PACK_THIS_STRUCT _CeaExtensionVersion2_t
{
        CeaExtension2_3MiscSupport_t miscSupport;

        uint8_t reserved[123];
} CeaExtensionVersion2_t,*PCeaExtensionVersion2_t;

typedef struct SI_PACK_THIS_STRUCT _CeaExtensionVersion3_t
{
        CeaExtension2_3MiscSupport_t miscSupport;
        union
        {
                uint8_t dataBlockCollection[123];
                uint8_t reserved[123];
        } Offset4_u;
} CeaExtensionVersion3_t,*PCeaExtensionVersion3_t;

typedef struct SI_PACK_THIS_STRUCT _BlockMap_t
{
        uint8_t tag;
        uint8_t blockTags[126];
        uint8_t checkSum;
} BlockMap_t, *PBlockMap_t;

typedef struct SI_PACK_THIS_STRUCT _CeaExtension_t
{
        uint8_t tag;
        uint8_t revision;
        uint8_t byteOffsetTo18ByteDescriptors;
        union
        {
                CeaExtensionVersion1_t      version1;
                CeaExtensionVersion2_t      version2;
                CeaExtensionVersion3_t      version3;
        } version_u;
        uint8_t checkSum;
} CeaExtension_t,*PCeaExtension_t;

typedef struct SI_PACK_THIS_STRUCT _DetailedTimingDescriptor_t
{
        uint8_t pixelClockLow;
        uint8_t pixelClockHigh;
        uint8_t horzActive7_0;
        uint8_t horzBlanking7_0;
        struct
        {
                unsigned char horzBlanking11_8    :4;
                unsigned char horzActive11_8      :4;
        } horzActiveBlankingHigh;
        uint8_t vertActive7_0;
        uint8_t vertBlanking7_0;
        struct
        {
                unsigned char vertBlanking11_8    :4;
                unsigned char vertActive11_8      :4;
        } vertActiveBlankingHigh;
        uint8_t horzSyncOffset7_0;
        uint8_t horzSyncPulseWidth7_0;
        struct
        {
                unsigned char vertSyncPulseWidth3_0 :4;
                unsigned char vertSyncOffset3_0     :4;
        } vertSyncOffsetWidth;
        struct
        {
                unsigned char vertSyncPulseWidth5_4 :2;
                unsigned char vertSyncOffset5_4     :2;
                unsigned char horzSyncPulseWidth9_8 :2;
                unsigned char horzSyncOffset9_8     :2;
        } hsOffsetHsPulseWidthVsOffsetVsPulseWidth;
        uint8_t horzImageSizemm_7_0;
        uint8_t vertImageSizemm_7_0;
        struct
        {
                unsigned char vertImageSizemm_11_8  :4;
                unsigned char horzImageSizemm_11_8  :4;
        } imageSizeHigh;
        uint8_t horzBorderLines;
        uint8_t vertBorderPixels;
        struct
        {
                unsigned char stereoBit0            :1;
                unsigned char syncSignalOptions     :2;
                unsigned char syncSignalType        :2;
                unsigned char stereoBits2_1         :2;
                unsigned char interlaced            :1;
        } flags;
} DetailedTimingDescriptor_t,*PDetailedTimingDescriptor_t;

typedef struct SI_PACK_THIS_STRUCT _RedGreenBits1_0_t
{
        unsigned char   Green_y :2;
        unsigned char   Green_x :2;
        unsigned char   Red_y   :2;
        unsigned char   Red_x   :2;
} RedGreenBits1_0_t,*PRedGreenBits1_0_t;

typedef struct SI_PACK_THIS_STRUCT _BlueWhiteBits1_0_t
{
        unsigned char   White_y :2;
        unsigned char   White_x :2;
        unsigned char   Blue_y  :2;
        unsigned char   Blue_x  :2;

} BlueWhiteBits1_0_t,*PBlueWhiteBits1_0_t;


typedef struct SI_PACK_THIS_STRUCT _EstablishedTimingsI_t
{
        unsigned char et800x600_60Hz  :1;
        unsigned char et800x600_56Hz  :1;
        unsigned char et640x480_75Hz  :1;
        unsigned char et640x480_72Hz  :1;
        unsigned char et640x480_67Hz  :1;
        unsigned char et640x480_60Hz  :1;
        unsigned char et720x400_88Hz  :1;
        unsigned char et720x400_70Hz  :1;
} EstablishedTimingsI_t,*PEstablishedTimingsI_t;

typedef struct SI_PACK_THIS_STRUCT _EstablishedTimingsII_t
{
        unsigned char et1280x1024_75Hz:1;
        unsigned char et1024x768_75Hz :1;
        unsigned char et1024x768_70Hz :1;
        unsigned char et1024x768_60Hz :1;
        unsigned char et1024x768_87HzI:1;
        unsigned char et832x624_75Hz  :1;
        unsigned char et800x600_75Hz  :1;
        unsigned char et800x600_72Hz  :1;
} EstablishedTimingsII_t,*PEstablishedTimingsII_t;

typedef struct SI_PACK_THIS_STRUCT _ManufacturersTimings_t
{
        unsigned char   reserved        :7;
        unsigned char   et1152x870_75Hz :1;
} ManufacturersTimings_t,*PManufacturersTimings_t;

typedef enum
{
        iar16to10   = 0
                      ,iar4to3    = 1
                                    ,iar5to4    = 2
                                                    ,iar16to9   = 3
} ImageAspectRatio_e;

typedef struct SI_PACK_THIS_STRUCT _StandardTiming_t
{
        unsigned char   horzPixDiv8Minus31;
        unsigned char   fieldRefreshRateMinus60:6;
        ImageAspectRatio_e imageAspectRatio    :2;
} StandardTiming_t,*PStandardTiming_t;

typedef struct SI_PACK_THIS_STRUCT _EDID_Block0_t
{
        unsigned char               headerData[8];
        TwoBytes_t                  idManufacturerName;
        TwoBytes_t                  idProductCode;
        unsigned char               serialNumber[4];
        unsigned char               weekOfManufacture;
        unsigned char               yearOfManufacture;
        unsigned char               edidVersion;
        unsigned char               edidRevision;
        unsigned char               videoInputDefinition;
        unsigned char               horzScreenSizeOrAspectRatio;
        unsigned char               vertScreenSizeOrAspectRatio;
        unsigned char               displayTransferCharacteristic;
        unsigned char               featureSupport;
        RedGreenBits1_0_t           redGreenBits1_0;
        BlueWhiteBits1_0_t          blueWhiteBits1_0;
        unsigned char               Red_x;
        unsigned char               Red_y;
        unsigned char               Green_x;
        unsigned char               Green_y;
        unsigned char               Blue_x;
        unsigned char               Blue_y;
        unsigned char               White_x;
        unsigned char               White_y;
        EstablishedTimingsI_t       establishedTimingsI;
        EstablishedTimingsII_t      establishedTimingsII;
        ManufacturersTimings_t      manufacturersTimings;
        StandardTiming_t            standardTimings[8];
        DetailedTimingDescriptor_t  detailedTimingDescriptors[4];
        unsigned char               extensionFlag;
        unsigned char               checkSum;

} EDID_Block0_t,*PEDID_Block0_t;

typedef struct SI_PACK_THIS_STRUCT _MonitorName_t
{
        uint8_t flagRequired[2];
        uint8_t flagReserved;
        uint8_t dataTypeTag;
        uint8_t flag;
        uint8_t asciiName[13];


} MonitorName_t,*PMonitorName_t;

typedef struct SI_PACK_THIS_STRUCT _MonitorRangeLimits_t
{

        uint8_t flagRequired[2];
        uint8_t flagReserved;
        uint8_t dataTypeTag;
        uint8_t flag;
        uint8_t minVerticalRateHz;
        uint8_t maxVerticalRateHz;
        uint8_t minHorizontalRateKHz;
        uint8_t maxHorizontalRateKHz;
        uint8_t maxPixelClockMHzDiv10;
        uint8_t tagSecondaryFormula;
        uint8_t filler[7];
} MonitorRangeLimits_t,*PMonitorRangeLimits_t;


typedef union tag_18ByteDescriptor_u
{
        DetailedTimingDescriptor_t  dtd;
        MonitorName_t               name;
        MonitorRangeLimits_t        rangeLimits;
} _18ByteDescriptor_u,*P_18ByteDescriptor_u;


typedef struct SI_PACK_THIS_STRUCT _DisplayMode3DInfo_t
{
        unsigned char dmi3Dsupported:1;
        unsigned char dmiSufficientBandwidth:1;
} DisplayMode3DInfo_t,*PDisplayMode3DInfo_t;



void	CopyEdidBlock(uint8_t *pData, uint8_t offset);
uint8_t ParseEDID (uint8_t *numExt);

uint8_t CalculateGenericCheckSum(uint8_t *infoFrameData,uint8_t checkSum,uint8_t length);
uint8_t CalculateSum(uint8_t *info,uint8_t sum,uint8_t length);
SI_POP_STRUCT_PACKING //)
