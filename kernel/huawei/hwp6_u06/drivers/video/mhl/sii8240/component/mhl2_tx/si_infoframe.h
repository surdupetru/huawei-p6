#include "si_memsegsupport.h"

SI_PUSH_STRUCT_PACKING //(

typedef struct SI_PACK_THIS_STRUCT _InfoFrameHeader_t
{
        uint8_t  type_code;
        uint8_t  version_number;
        uint8_t  length;
} InfoFrameHeader_t,*PInfoFrameHeader_t;

typedef enum
{
        acsRGB         = 0
                         ,acsYCbCr422    = 1
                                         ,acsYCbCr444    = 2
                                                         ,acsFuture      = 3
} AviColorSpace_e;

//------------------------------------------------------------------------------
// AVI Info Frame Structure
//------------------------------------------------------------------------------
typedef struct SI_PACK_THIS_STRUCT _AviInfoFrameDataByte1_t
{
        unsigned char ScanInfo:2;
        unsigned char BarInfo:2;
        unsigned char ActiveFormatInfoPresent:1;
        AviColorSpace_e colorSpace:2;
        unsigned char futureMustBeZero:1;
} AviInfoFrameDataByte1_t,*PAviInfoFrameDataByte1_t;

typedef struct SI_PACK_THIS_STRUCT _AviInfoFrameDataByte2_t
{
        unsigned char ActiveFormatAspectRatio:4;
        unsigned char PictureAspectRatio:2;
        unsigned char Colorimetry:2;
} AviInfoFrameDataByte2_t,*PAviInfoFrameDataByte2_t;

typedef struct SI_PACK_THIS_STRUCT _AviInfoFrameDataByte3_t
{
        unsigned char NonUniformPictureScaling:2;
        unsigned char RGBQuantizationRange:2;
        unsigned char ExtendedColorimetry:3;
        unsigned char ITContent:1;
} AviInfoFrameDataByte3_t,*PAviInfoFrameDataByte3_t;

typedef struct SI_PACK_THIS_STRUCT _AviInfoFrameDataByte4_t
{
        unsigned char VIC:7;
        unsigned char futureMustBeZero:1;
} AviInfoFrameDataByte4_t,*PAviInfoFrameDataByte4_t;

typedef enum
{
        cnGraphics  = 0
                      ,cnPhoto    = 1
                                    ,cnCinema   = 2
                                                    ,cnGame     = 3
} BitsContent_e;

typedef enum
{
        aqLimitedRange = 0
                         ,aqFullRange    = 1
                                         ,aqReserved0    = 2
                                                         ,aqReserved1    = 3
} AviQuantization_e;

typedef struct SI_PACK_THIS_STRUCT _AviInfoFrameDataByte5_t
{
        unsigned char pixelRepetitionFactor:4;
        BitsContent_e       content             :2;
        AviQuantization_e   quantization        :2;
} AviInfoFrameDataByte5_t,*PAviInfoFrameDataByte5_t;

typedef struct SI_PACK_THIS_STRUCT _HwAviNamedPayLoad_t
{
        uint8_t  checksum;
        union
        {
                struct
                {
                        AviInfoFrameDataByte1_t  pb1;
                        AviInfoFrameDataByte2_t  colorimetryAspectRatio;
                        AviInfoFrameDataByte3_t  pb3;
                        AviInfoFrameDataByte4_t  VIC;
                        AviInfoFrameDataByte5_t  pb5;
                        uint8_t                  LineNumEndTopBarLow;
                        uint8_t                  LineNumEndTopBarHigh;
                        uint8_t                  LineNumStartBottomBarLow;
                        uint8_t                  LineNumStartBottomBarHigh;
                        uint8_t                  LineNumEndLeftBarLow;
                        uint8_t                  LineNumEndLeftBarHigh;
                        uint8_t                  LineNumStartRightBarLow;
                        uint8_t                  LineNumStartRightBarHigh;
                } bitFields;
                uint8_t  infoFrameData[13];
        } ifData_u;
} HwAviNamedPayLoad_t,*PHwAviNamedPayLoad_t;

// this union overlays the TPI HW for AVI InfoFrames, starting at REG_TPI_AVI_CHSUM.
typedef union _HwAviPayLoad_t
{
        HwAviNamedPayLoad_t namedIfData;
        uint8_t     ifData[14];
} HwAviPayLoad_t,*PHwAviPayLoad_t;

typedef struct SI_PACK_THIS_STRUCT _AviPayLoad_t
{
        HwAviPayLoad_t      hwPayLoad;
        uint8_t  byte_14;
        uint8_t  byte_15;
} AviPayLoad_t,*PAviPayLoad_t;

typedef struct SI_PACK_THIS_STRUCT _AVIInfoFrame_t
{
        InfoFrameHeader_t   header;
        AviPayLoad_t      payLoad;
} AVIInfoFrame_t,*PAVIInfoFrame_t;

// these values determine the interpretation of PB5
typedef enum
{
        hvfNoAdditionalHDMIVideoFormatPresent =0
                        ,hvfExtendedResolutionFormatPresent    =1
                                        ,hvf3DFormatIndicationPresent          =2
} HDMI_Video_Format_e;

typedef enum
{
        tdsFramePacking = 0x00
                          ,tdsTopAndBottom = 0x06
                                          ,tdsSideBySide   = 0x08
} ThreeDStructure_e;

typedef enum
{
        tdedHorizontalSubSampling    = 0x0
                                       ,tdedQuincunxOddLeftOddRight  = 0x4
                                                       ,tdedQuincunxOddLeftEvenRight = 0x5
                                                                       ,tdedQuincunxEvenLeftOddRight = 0x6
                                                                                       ,tdedQuincunxEvenLeftEvenRight= 0x7

} ThreeDExtData_e;

typedef enum
{
        tdmdParallaxIso23022_3Section6_x_2_2 = 0
} ThreeDMetaDataType_e;
typedef struct SI_PACK_THIS_STRUCT _VendorSpecificPayLoad_t
{
        uint8_t             checksum;
        uint8_t             IEEERegistrationIdentifier[3];  // must be 0x000C03 Little Endian
        struct
        {
                unsigned char    reserved:5;

                HDMI_Video_Format_e HDMI_Video_Format:3; //HDMI_Video_Format_e
        } pb4;
        union
        {
                uint8_t HDMI_VIC;
                struct _ThreeDStructure
                {
                        unsigned char reserved:3;
                        unsigned char ThreeDMetaPresent:1;
                        ThreeDStructure_e threeDStructure:4;  //ThreeDStructure_e
                } ThreeDStructure;
        } pb5;
        struct
        {
                unsigned char reserved:4;
                unsigned char threeDExtData:4; //ThreeDExtData_e
        } pb6;
        struct _PB7
        {
                unsigned char   threeDMetaDataLength:5;
                unsigned char   threeDMetaDataType:3; //ThreeDMetaDataType_e

        } pb7;
} VendorSpecificPayLoad_t,*PVendorSpecificPayLoad_t;
typedef struct SI_PACK_THIS_STRUCT _VendorSpecificInfoFrame_t
{
        InfoFrameHeader_t   header;
        VendorSpecificPayLoad_t payLoad;
} VendorSpecificInfoFrame_t,*PVendorSpecificInfoFrame_t;
//------------------------------------------------------------------------------
// MPEG Info Frame Structure
// Table 8-11 on page 141 of HDMI Spec v1.4
//------------------------------------------------------------------------------
typedef struct SI_PACK_THIS_STRUCT
{
        InfoFrameHeader_t   header;
        uint8_t  checksum;
        uint8_t  byte_1;
        uint8_t  byte_2;
        uint8_t  byte_3;
        uint8_t  byte_4;
        uint8_t  byte_5;
        uint8_t  byte_6;
} UNRInfoFrame_t;

typedef struct SI_PACK_THIS_STRUCT _InfoFrame_t
{
        union
        {
                InfoFrameHeader_t   header;
                AVIInfoFrame_t avi;
                VendorSpecificInfoFrame_t vendorSpecific;
                UNRInfoFrame_t unr;
        } body;

} InfoFrame_t,*PInfoFrame_t;

typedef enum
{
        // just define these three for now
        InfoFrameType_AVI
        ,InfoFrameType_VendorSpecific
        ,InfoFrameType_Audio

} InfoFrameType_e;


#ifdef ENABLE_DUMP_INFOFRAME //(

//void DumpIncomingInfoFrameImpl(char *pszId,char *pszFile,int iLine,void *pInfoFrame,uint8_t length);
void DumpIncomingInfoFrameImpl(PAVIInfoFrame_t pInfoFrame,char *pszLabel, uint8_t length);

#define DumpIncomingInfoFrame(pData,length) DumpIncomingInfoFrameImpl((PAVIInfoFrame_t)pData,"Incoming", length)

#else //)(

#define DumpIncomingInfoFrame(pData,length) /* do nothing */

#endif //)

SI_POP_STRUCT_PACKING //)
