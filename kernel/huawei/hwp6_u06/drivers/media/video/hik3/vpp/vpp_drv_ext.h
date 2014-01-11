
#ifndef __VPP_DRV_EXT_H__
#define __VPP_DRV_EXT_H__

#define   K3_SUCCESS                      0
#define   K3_FAILURE                    (-1)

#define VPP_IRECT_MIN_W 64
#define VPP_IRECT_MIN_H 64
#define VPP_IRECT_MAX_W 4096
#define VPP_IRECT_MAX_H 4096

#define VPP_ORECT_MIN_W 64
#define VPP_ORECT_MIN_H 64
#define VPP_ORECT_MAX_W 1920
#define VPP_ORECT_MAX_H 1088

#define VPP_MAX_RATIO 16
#define VPP_MIN_RARIO 1/16

typedef enum 
{
    k3_false    = 0,
    k3_true     = 1,
} k3_bool;


/*视频数据读取模式*/
typedef enum VPP_VHD_FOD_E_
{
    VPP_T_FIRST = 0,
    VPP_B_FIRST = 1,
    
    VPP_VHD_FOD_BUTT
} VPP_VHD_FOD_E;

/* vpp die st mode */
typedef enum VPP_DIE_STMD_E_
{
    VPP_DIE_STMD_Q   = 0,
    VPP_DIE_STMD_N,
    VPP_DIE_STMD_M,
    VPP_DIE_STMD_L,
    VPP_DIE_STMD_K,
    VPP_DIE_STMD_KLM,
    VPP_DIE_STMD_KLQR,
    VPP_DIE_STMD_KLQRM,    
    VPP_DIE_STMD_STKLM,    
    VPP_DIE_STMD_KLMN,    
    VPP_DIE_STMD_DISALL,

    VPP_DIE_STMD_BUTT
    
}VPP_DIE_STMD_E;

/* vpp die mode */
typedef enum VPP_DIE_MODE_E_
{
    VPP_DIE_MODE_5FIELD = 0,
    VPP_DIE_MODE_4FIELD,
    VPP_DIE_MODE_3FIELD,
    VPP_DIE_MODE_1FIELD,

    VPP_DIE_MODE_BUTT
    
}VPP_DIE_MODE_E;

/*CSC色度转换系数*/
typedef enum VPP_CSCCOEF_E_
{
    VPP_CSC_YUV2YUV_709_601,
    VPP_CSC_YUV2YUV_601_709,
    VPP_CSC_YUV2YUV,
   
    VPP_CSC_BUTT
} VPP_CSCCOEF_E;

/*视频帧图像格式*/
typedef enum VPP_PIXELFORMAT_E_
{
    VPP_INPUTFMT_YCBCR_SEMIPLANAR_400     = 0x1,
    VPP_INPUTFMT_YCBCR_PLANAR_444         = 0x2,
    VPP_INPUTFMT_YCBCR_SEMIPLANAR_420     = 0x3,
    VPP_INPUTFMT_YCBCR_SEMIPLANAR_422     = 0x4,
    VPP_INPUTFMT_YCBCR_SEMIPLANAR_444     = 0x5, 
    VPP_INPUTFMT_YCBCR_PLANAR_420         = 0x6,    
    VPP_INPUTFMT_YCBCR_PLANAR_422         = 0x7,

    VPP_INPUTFMT_CBYCRY_PACKAGE_422       = 0x9,    
    VPP_INPUTFMT_YCBYCR_PACKAGE_422       = 0xa,    
    VPP_INPUTFMT_YCRYCB_PACKAGE_422       = 0xb,  
    VPP_INPUTFMT_CRYCBY_PACKAGE_422       = 0xc,  

    VPP_INPUTFMT_YCBCR_SEMIPLANAR_420S    = 0xe,
    VPP_INPUTFMT_YCBCR_SEMIPLANAR_422S    = 0xd,
    VPP_INPUTFMT_YCBCR_SEMIPLANAR_444S    = 0xf, 

    VPP_PIXELFORMAT_BUTT 
    
}VPP_PIXELFORMAT_E;

/*接口输出数据格式*/
typedef enum VPP_INTFDATAFMT_E_
{ 
    VPP_INTFDATAFMT_CBYCRY_PACKAGE_422   = 0x0,
    VPP_INTFDATAFMT_YCBYCR_PACKAGE_422   = 0x1,
    VPP_INTFDATAFMT_YCRYCB_PACKAGE_422   = 0x2,
    
    VPP_INTFDATAFMT_BUTT          
} VPP_INTFDATAFMT_E;

typedef struct VPP_DIE_CFG_S_
{
    VPP_DIE_MODE_E   edie_lum_mode;    /* 亮度插值场数模式 */
    VPP_DIE_MODE_E   edie_chm_mode;   /* 色度模式 <＝ 亮度模式插值场数*/

    VPP_DIE_STMD_E   edie_st_mode;    /*静止次数统计模式*/

    VPP_VHD_FOD_E    edie_fod;           /*码流场序TB or BT*/
}VPP_DIE_CFG_S;

typedef struct VPP_INCROPPING_S_
{
    unsigned int cropX;  //裁剪的起始点坐标
    unsigned int cropY;
    unsigned int height;//裁剪的高度，单位是像素
    unsigned int width; //裁剪的宽度，单位是像素
} VPP_INCROPPING_S;

typedef struct VPP_INIMAGE_S_
{
    VPP_PIXELFORMAT_E  pixformat;
    unsigned int       width;//输入图像宽度，以像素为单位
    unsigned int       height;//输入图像高度，以像素为单位
    unsigned int       buffer_bus_addr;//输入图像帧Buffer起始地址，物理地址,四字节对齐
    unsigned int       stride; //输入图像stride 
}VPP_INIMAGE_S;

typedef struct VPP_OUTIMAGE_S_
{
    VPP_INTFDATAFMT_E  pixformat;
    unsigned int       width;   //输出图像宽度，以像素为单位
    unsigned int       height;  //输出图像高度，以像素为单位
    unsigned int       buffer_bus_addr;//输出图像帧Buffer起始地址，物理地址,四字节对齐
    unsigned int       stride;
}VPP_OUTIMAGE_S;

//S00198515 add for support DIE 20120823 [start]
typedef struct VPP_DIE_ADDRINFO_S_
{
    unsigned int   lluma_addr;       /* just for deinterlace*/
    unsigned int   lchroma_addr;
    unsigned int   cluma_addr;
    unsigned int   cchroma_addr;
    unsigned int   nluma_addr;
    unsigned int   nchroma_addr;
}VPP_DIE_ADDRINFO_S;

typedef struct VPP_FRAME_ADDRINFO_S_
{
    unsigned int   vhd_clum_addr;
    unsigned int   vhd_cchm_addr;

    unsigned int   vhd_llum_addr;       /*single frame,  planar : V address ; */ 
    unsigned int   vhd_lchm_addr;       /*single frame,  planar : not used ; */ 

    unsigned short vhd_lum_str;		/*stride */
    unsigned short vhd_chm_str;
}VPP_FRAME_ADDRINFO_S;
//S00198515 add for support DIE 20120823 [end]

typedef struct VPP_CONFIG_S_
{
    k3_bool   bincrop_enable;//是否使能crop功能，crop的具体参数vpp_in_crop
    k3_bool   bdie_enbale;     //此功能未实现,请设置为false
    k3_bool   bcsc_enable;     //是否使用颜色空间转换
    k3_bool   bscal_enable;

    VPP_INIMAGE_S           vpp_in_img; //输入图像
    VPP_OUTIMAGE_S          vpp_out_img;//输入图像
    VPP_INCROPPING_S        vpp_in_crop;//Crop参数
    VPP_DIE_CFG_S           vpp_die_config;//此功能未实现
    VPP_CSCCOEF_E           encsc_mode; //颜色空间转换类型
    VPP_DIE_ADDRINFO_S       vpp_die_addrinfo;//S00198515 add //vpp dieaddrinfo 参数，主要用于进行DIE设置
}VPP_CONFIG_S;

#endif  

