/*
 *  sonyimx188 camera driver source file
 *
 *  CopyRight (C) Hisilicon Co., Ltd.
 *	Author :
 *  Version:  1.2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/videodev2.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <asm/div64.h>
#include <mach/hisi_mem.h>
#include "mach/hardware.h"
#include <mach/boardid.h>
#include <mach/gpio.h>
#include "../isp/sensor_common.h"
#include "sonyimx188.h"
/*#include "../isp/k3_isp_io.h"*/
#include <asm/bug.h>
#include <linux/device.h>

#define LOG_TAG "SONYIMX188"
/* #define DEBUG_DEBUG 1 */
#include "../isp/cam_log.h"
#include <../isp/cam_util.h>
#include <hsad/config_interface.h>
#include "../isp/k3_isp.h"
#include "../isp/k3_ispv1.h"
#include "../isp/k3_ispv1_afae.h"

#define SONYIMX188_SLAVE_ADDRESS 0x34
#define SONYIMX188_CHIP_ID       (0x0188)
#define SONYIMX188_SCAM_ID       (112)

#define SONYIMX188_CAM_MODULE_SKIPFRAME     4

#define SONYIMX188_FLIP		0x0101
#define SONYIMX188_PIXEL_ORDER_CHANGED	(1)

#define SONYIMX188_EXPOSURE_REG_H	0x0202
#define SONYIMX188_EXPOSURE_REG_L	0x0203
#define SONYIMX188_GAIN_REG_H		0x0204
#define SONYIMX188_GAIN_REG_L		0x0205

#define SONYIMX188_VTS_REG_H		0x0340
#define SONYIMX188_VTS_REG_L		0x0341

/*add for set awb gain begin*/
#define SONYIMX188_ABS_GAIN_B_H		0x0716
#define SONYIMX188_ABS_GAIN_B_L		0x0717
#define SONYIMX188_ABS_GAIN_GB_H	0x0718
#define SONYIMX188_ABS_GAIN_GB_L	0x0719
#define SONYIMX188_ABS_GAIN_GR_H	0x0712
#define SONYIMX188_ABS_GAIN_GR_L	0x0713
#define SONYIMX188_ABS_GAIN_R_H		0x0714
#define SONYIMX188_ABS_GAIN_R_L		0x0715


/* camera sensor override parameters, define in binning preview mode */
#define SONYIMX188_MAX_ISO			800
#define SONYIMX188_MIN_ISO			100

#define SONYIMX188_AUTOFPS_GAIN_LOW2MID		0x24
#define SONYIMX188_AUTOFPS_GAIN_MID2HIGH		0x24
#define SONYIMX188_AUTOFPS_GAIN_HIGH2MID		0x60
#define SONYIMX188_AUTOFPS_GAIN_MID2LOW		0x60

#define SONYIMX188_MAX_FRAMERATE		30
#define SONYIMX188_MIDDLE_FRAMERATE		20
#define SONYIMX188_MIN_FRAMERATE		10

/* camera sensor denoise parameters */
#define SONYIMX188_ISP_YDENOISE_COFF_1X		0x02
#define SONYIMX188_ISP_YDENOISE_COFF_2X		0x02
#define SONYIMX188_ISP_YDENOISE_COFF_4X		0x04
#define SONYIMX188_ISP_YDENOISE_COFF_8X		0x08
#define SONYIMX188_ISP_YDENOISE_COFF_16X		0x0c
#define SONYIMX188_ISP_YDENOISE_COFF_32X		0x16
#define SONYIMX188_ISP_YDENOISE_COFF_64X		0x16

#define SONYIMX188_ISP_UVDENOISE_COFF_1X		0x02
#define SONYIMX188_ISP_UVDENOISE_COFF_2X		0x04
#define SONYIMX188_ISP_UVDENOISE_COFF_4X		0x08
#define SONYIMX188_ISP_UVDENOISE_COFF_8X		0x10
#define SONYIMX188_ISP_UVDENOISE_COFF_16X		0x20
#define SONYIMX188_ISP_UVDENOISE_COFF_32X		0x20
#define SONYIMX188_ISP_UVDENOISE_COFF_64X		0x20

const struct isp_reg_t isp_init_regs_sonyimx188_sunny[] = {
	/*{0x65006, 0x01},//y36721 delete, not needed now.*/

	/* y36721 2012-03-28 added. */
	/* y36721 2012-03-31 changed blc and lenc. */
	/*blc value=0x40(10bit)*/
	{0x1c58b, 0xff},
	{0x1c58c, 0xff},

#ifndef OVISP_DEBUG_MODE
	/* AEC */
	{0x1c146, 0x30},//ori0x30 low AE target,should close
	{0x1c14a, 0x03},
	{0x1c14b, 0x0a},
	{0x1c14c, 0x0f},//aec fast step//          
	{0x1c14e, 0x08},//slow step//08
	{0x1c140, 0x01},//banding
	{0x1c13e, 0x02},//real gain mode for OV8830
	
	{0x66401, 0x00},//window weight
	{0x66402, 0x20},//StatWin_Left
	{0x66403, 0x00},
	{0x66404, 0x20},//StatWin_Top
	{0x66405, 0x00},
	{0x66406, 0x20},//StatWin_Right
	{0x66407, 0x00},
	{0x66408, 0x20},//StatWin_Bottom           
	{0x66409, 0x00},//definiton ofthe center 3x3 window
	{0x6640a, 0x80},//nWin_Left                
	{0x6640d, 0x00},                           
	{0x6640e, 0x70},//nWin_Top                 
	{0x66411, 0x03},                           
	{0x66412, 0xc0},//nWin_Width               
	{0x66415, 0x02},
	{0x66416, 0x00},//nWin_Height
	{0x6642e, 0x01},//nWin_Weight_0 weight pass
	{0x6642f, 0x01},//nWin_Weight_1
	{0x66430, 0x01},//nWin_Weight_2
	{0x66431, 0x01},//nWin_Weight_3
	{0x66432, 0x02},//nWin_Weight_4
	{0x66433, 0x02},//nWin_Weight_5
	{0x66434, 0x02},//nWin_Weight_6
	{0x66435, 0x02},//nWin_Weight_7
	{0x66436, 0x04},//nWin_Weight_8
	{0x66437, 0x02},//nWin_Weight_9
	{0x66438, 0x02},//nWin_Weight_10
	{0x66439, 0x02},//nWin_Weight_11
	{0x6643a, 0x02},//nWin_Weight_12
	{0x6644e, 0x03},//nWin_Weight_Shift
	{0x6644f, 0x04},//black level
	{0x66450, 0xf8},//saturate level
	{0x6645b, 0x1a},//black weight1
	{0x6645d, 0x1a},//black weight2
	{0x66460, 0x04},//saturate per1
	{0x66464, 0x0a},//saturate per2
	{0x66467, 0x14},//saturate weight1
	{0x66469, 0x14},//saturate weight2
	//auto AE control
	
/* Raw Stretch */
	{0x65020, 0x00},//RAW Stretch Target0x01-->00
	{0x66500, 0x28},
	{0x66501, 0x00},
	{0x66502, 0x8f},//0xff
	{0x66503, 0x0b},//0x0f
	{0x1c1b0, 0xff},
	{0x1c1b1, 0xff},
	{0x1c1b2, 0x01},
	{0x65905, 0x08},
	{0x66301, 0x02},//high level step
	{0x66302, 0xe0},//ref bin
	{0x66303, 0x06},//PsPer0 0a
	{0x66304, 0x10},//PsPer1
	{0x1c5a4, 0x01},//use new high stretch
	{0x1c5a5, 0x20},//stretch low step
	{0x1c5a6, 0x20},//stretch high step
	{0x1c5a7, 0x08},//stretch slow range
	{0x1c5a8, 0x02},//stretch slow step
	{0x1c1b8, 0x10},//ratio scale
	
	{0x1c5a0, 0x50},//AE target high, should close
	{0x1c5a2, 0x04},//target stable range
	{0x1c5a3, 0x06},//stretch target slow range
	
/* De-noise */
	{0x65604, 0x00},//Richard for new curve 0314
	{0x65605, 0x00},//Richard for new curve 0314
	{0x65606, 0x00},//Richard for new curve 0314
	{0x65607, 0x00},//Richard for new curve 0314

	{0x65510, 0x0C},//G dns slope change from 0x4 to 0xf Richard 0320
	{0x6551a, SONYIMX188_ISP_YDENOISE_COFF_1X},//Raw G Dns improve white pixel 20120728
	{0x6551b, SONYIMX188_ISP_YDENOISE_COFF_2X},//gain  2X
	{0x6551c, SONYIMX188_ISP_YDENOISE_COFF_4X},//gain  4X
	{0x6551d, SONYIMX188_ISP_YDENOISE_COFF_8X},//gain  8X
	{0x6551e, SONYIMX188_ISP_YDENOISE_COFF_16X},//gain 16X
	{0x6551f, SONYIMX188_ISP_YDENOISE_COFF_32X},//gain 32X
	{0x65520, SONYIMX188_ISP_YDENOISE_COFF_64X},//gain 64X
	{0x65522, 0x00},//RAW BR De-noise
	{0x65523, SONYIMX188_ISP_UVDENOISE_COFF_1X},//gain 1X
	{0x65524, 0x00},
	{0x65525, SONYIMX188_ISP_UVDENOISE_COFF_2X},//gain 2X
	{0x65526, 0x00},
	{0x65527, SONYIMX188_ISP_UVDENOISE_COFF_4X},//gain 4X
	{0x65528, 0x00},
	{0x65529, SONYIMX188_ISP_UVDENOISE_COFF_8X},//gain 8X
	{0x6552a, 0x00},
	{0x6552b, SONYIMX188_ISP_UVDENOISE_COFF_16X},//gain 16X
	{0x6552c, 0x00},
	{0x6552d, SONYIMX188_ISP_UVDENOISE_COFF_32X},//gain 32X
	{0x6552e, 0x00},
	{0x6552f, SONYIMX188_ISP_UVDENOISE_COFF_64X},//gain 64X
	
	{0x65c00, 0x03},//UV De-noise: gain 1X
	{0x65c01, 0x05},//gain 2X
	{0x65c02, 0x08},//gain 4X
	{0x65c03, 0x1f},//gain 8X
	{0x65c04, 0x1f},//gain 16X
	{0x65c05, 0x1f},//gain 32X
	

/* sharpeness */   
	{0x65600, 0x00},
	{0x65601, 0x30},//0319
	{0x65602, 0x00},
	{0x65603, 0x60}, //y00215412 change sharpness high gain threshod 0x40->0x60 20120814
	{0x65608, 0x06},
	{0x65609, 0x20},
	{0x6560c, 0x00},
	{0x6560d, 0x08}, //low gain sharpness, 20120814 0x20->0x30
	{0x6560e, 0x10},//MinSharpenTp
	{0x6560f, 0x60},//MaxSharpenTp
	{0x65610, 0x10},//MinSharpenTm
	{0x65611, 0x60},//MaxSharpenTm
	{0x65613, 0x10},//SharpenAlpha
	{0x65615, 0x08},//HFreq_thre
	{0x65617, 0x06},//HFreq_coef	

/* auto uv saturation */
	{0x1c4e8, 0x01},//Enable
	{0x1c4e9, 0xbf},// gain threshold1 40-->0b
	{0x1c4ea, 0xf7},//gain threshold2 78-->0d
	{0x1c4eb, 0x70}, //UV max saturation
	{0x1c4ec, 0x68}, //UV min saturation



/* Global Gamma */
	{0x1c49b, 0x01},
	{0x1c49c, 0x02},
	{0x1c49d, 0x01}, //gamma 2.0 0310
	{0x1c49e, 0x02},
	{0x1c49f, 0x01}, //gamma 2.0 0310
	{0x1c4a0, 0x00},
	{0x1c4a1, 0x18},
	{0x1c4a2, 0x00},
	{0x1c4a3, 0x88}, //gamma 2.0 0310 //avoid false contour Richard@0323


/* Tone Mapping */
	//contrast curve change for skin over exposure 20120728
        {0x1C4C0, 0x21},
	{0x1C4C1, 0x30},
	{0x1C4C2, 0x3c},
	{0x1C4C3, 0x47},
	{0x1C4C4, 0x50},
	{0x1C4C5, 0x58},
	{0x1C4C6, 0x61},
	{0x1C4C7, 0x6a},
	{0x1C4C8, 0x73},
	{0x1C4C9, 0x7e},
	{0x1C4CA, 0x8a},
	{0x1C4CB, 0x97},
	{0x1C4CC, 0xa6},
	{0x1C4CD, 0xbc},
	{0x1C4CE, 0xd8},
	{0x1c4d4, 0x20},//EDR scale
	{0x1c4d5, 0x20},//EDR scale
	{0x1c4cf, 0x80},
	{0x65a00, 0x1b},
	{0x65a01, 0xc0}, //huiyan 0801

	//dark boost
	{0x1c4b0, 0x02},
	{0x1c4b1, 0x80},

	//YUV curve gain control,expoure frist
	{0x1c1b3, 0x40}, //Gain thre1
	{0x1c1b4, 0x70}, //Gain thre2 
	{0x1c1b5, 0x01}, //EDR gain control
	{0x1c1b6, 0x01}, //Curve Gain control
	{0x1c1b7, 0x40}, //after gamma cut ratio

	//Manual UV curve
	{0x1C998, 0x00},
	{0x1C999, 0x8c},
	{0x1C99A, 0x00},
	{0x1C99B, 0xc8},
	{0x1C99C, 0x00},
	{0x1C99D, 0xe8},
	{0x1C99E, 0x00},
	{0x1C99F, 0xfc},
        {0x1C9A0, 0x01},
	{0x1C9A1, 0x08},
        {0x1C9A2, 0x01},
	{0x1C9A3, 0x0e},
        {0x1C9A4, 0x01},
	{0x1C9A5, 0x10},
        {0x1C9A6, 0x01},
	{0x1C9A7, 0x10},
        {0x1C9A8, 0x01},
	{0x1C9A9, 0x10},
        {0x1C9AA, 0x01},
	{0x1C9AB, 0x10},
        {0x1C9AC, 0x01},
	{0x1C9AD, 0x10},
        {0x1C9AE, 0x01},
	{0x1C9AF, 0x10},
        {0x1C9B0, 0x01},
	{0x1C9B1, 0x08},
	{0x1C9B2, 0x00},
	{0x1C9B3, 0xf5},
        {0x1C9B4, 0x00},
	{0x1C9B5, 0xd8},
        {0x1C9B6, 0x00},
	{0x1C9B7, 0xb0},

/* LENC */
	{0x1c247, 0x00},//three profile,color temperature based lens shading correction mode 1: enable 0: disable
	{0x1c24c, 0x00},
	{0x1c24d, 0x40},
	{0x1c24e, 0x00},
	{0x1c24f, 0x80},
	{0x1c248, 0x40},
	{0x1c24a, 0x20},
	{0x1c574, 0x00},
	{0x1c575, 0x20},
	{0x1c576, 0x00},
	{0x1c577, 0xf0},
	{0x1c578, 0x40},

	{0x65200, 0x0d},
	{0x65206, 0x3c},
	{0x65207, 0x04},
	{0x65208, 0x3c},
	{0x65209, 0x04},
	{0x6520a, 0x33},
	{0x6520b, 0x0c},
	{0x65214, 0x28},
	{0x65216, 0x20},
	{0x1d93d, 0x08},
	{0x1d93e, 0x00},
	{0x1d93f, 0x40},
	
	{0x1d940, 0xfc},
        {0x1d942, 0x04},
        {0x1d941, 0xfc},
        {0x1d943, 0x04},
        
        {0x1d944, 0x00},
        {0x1d946, 0x00},
        {0x1d945, 0xfc},
        {0x1d947, 0x04},
  
       


/* OVISP LENC setting for D65 Long Exposure (HDR/3D) */
	//Y channel re-back to old version(do not plus 8) 20120821 by y00215412
	{0x1c264, 0x11}, //Y1
	{0x1c265, 0xb }, 
	{0x1c266, 0xb }, 
	{0x1c267, 0xb }, 
	{0x1c268, 0xc }, 
	{0x1c269, 0x10}, 
	{0x1c26a, 0x7 }, //Y2
	{0x1c26b, 0x6 }, 
	{0x1c26c, 0x5 }, 
	{0x1c26d, 0x5 }, 
	{0x1c26e, 0x6 }, 
	{0x1c26f, 0x8 }, 
	{0x1c270, 0x5 }, //Y3
	{0x1c271, 0x2 }, 
	{0x1c272, 0x0 }, 
	{0x1c273, 0x0 }, 
	{0x1c274, 0x1 }, 
	{0x1c275, 0x4 }, 
	{0x1c276, 0x5 }, //Y4
	{0x1c277, 0x2 }, 
	{0x1c278, 0x0 }, 
	{0x1c279, 0x0 }, 
	{0x1c27a, 0x2 }, 
	{0x1c27b, 0x5 }, 
	{0x1c27c, 0x8 }, //Y5
	{0x1c27d, 0x7 }, 
	{0x1c27e, 0x6 }, 
	{0x1c27f, 0x6 }, 
	{0x1c280, 0x7 }, 
	{0x1c281, 0x8 }, 
	{0x1c282, 0x16}, //Y6
	{0x1c283, 0xe }, 
	{0x1c284, 0xd }, 
	{0x1c285, 0xd }, 
	{0x1c286, 0xe }, 
	{0x1c287, 0x13}, 
	{0x1c288, 0x20}, //Cb1
	{0x1c289, 0x1f}, 
	{0x1c28a, 0x1f}, 
	{0x1c28b, 0x1f}, 
	{0x1c28c, 0x1f}, 
	{0x1c28d, 0x1f}, //Cb2
	{0x1c28e, 0x20}, 
	{0x1c28f, 0x20}, 
	{0x1c290, 0x20}, 
	{0x1c291, 0x1f}, 
	{0x1c292, 0x20}, //Cb3
	{0x1c293, 0x20}, 
	{0x1c294, 0x20}, 
	{0x1c295, 0x20}, 
	{0x1c296, 0x20}, 
	{0x1c297, 0x1e}, //Cb4
	{0x1c298, 0x20}, 
	{0x1c299, 0x1f}, 
	{0x1c29a, 0x20}, 
	{0x1c29b, 0x1e}, 
	{0x1c29c, 0x23}, //Cb5
	{0x1c29d, 0x1e}, 
	{0x1c29e, 0x1f}, 
	{0x1c29f, 0x1e}, 
	{0x1c2a0, 0x21},
	{0x1c2a1, 0x29}, //Cr1
	{0x1c2a2, 0x2f},      
	{0x1c2a3, 0x30},      
	{0x1c2a4, 0x2f},      
	{0x1c2a5, 0x2a},      
	{0x1c2a6, 0x2c}, //Cr2
	{0x1c2a7, 0x27},      
	{0x1c2a8, 0x25},      
	{0x1c2a9, 0x26},      
	{0x1c2aa, 0x2a},      
	{0x1c2ab, 0x27}, //Cr3
	{0x1c2ac, 0x20},      
	{0x1c2ad, 0x1d},      
	{0x1c2ae, 0x1f},      
	{0x1c2af, 0x24},      
	{0x1c2b0, 0x2c}, //Cr4
	{0x1c2b1, 0x28},      
	{0x1c2b2, 0x26},      
	{0x1c2b3, 0x27},      
	{0x1c2b4, 0x2b},      
	{0x1c2b5, 0x28}, //cr5
	{0x1c2b6, 0x2d},      
	{0x1c2b7, 0x30},      
	{0x1c2b8, 0x2f},      
	{0x1c2b9, 0x28},   
	
/* AWB */
	{0x66201, 0x52},
	{0x66203, 0x14},//crop window
	{0x66211, 0xe8},//awb top limit
	{0x66212, 0x04},//awb bottom limit	
	
	{0x1c17c, 0x01},//CT mode
	{0x1c182, 0x04},
	{0x1c183, 0x00},//MinNum
	{0x1c184, 0x04},//AWB Step		
	{0x1c58d, 0x00},//LimitAWBAtD65Enable
	
	{0x1c1be, 0x00},//AWB offset
	{0x1c1bf, 0x00},
	{0x1c1c0, 0x00},
	{0x1c1c1, 0x00},

	{0x1c1aa, 0x00},//avgAllEnable	
	{0x1c1ad, 0x02},//weight of A
	{0x1c1ae, 0x10},//weight of D65
	{0x1c1af, 0x04},//weight of CWF
	
	{0x1c5ac, 0x80},//pre-gain 
	{0x1c5ad, 0x80},
	{0x1c5ae, 0x80},

	{0x1ccce, 0x02},//awb shift,open=02	
	{0x1cccf, 0x10},//B gain for A
	{0x1ccd0, 0xf0},//R gain for A
	
	{0x1c5b8, 0x00},//B gain for C outdoor Richard@0517
	{0x1c5b9, 0x00},//R gain for C outdoor Richard@0517
	{0x1ccd1, 0x00},//B gain for C indoor Richard@0517
	{0x1ccd2, 0x00},//R gain for C indoor Richard@0517
	
	{0x1ccd3, 0x00},//B gain for D indoor
	{0x1ccd4, 0x00},//R gain for D indoor
	{0x1cccc, 0x00},//B gain for D outdoor
	{0x1cccd, 0x00},//R gain for D outdoor
                                            
	// for detect indoor/outdoor awb shift on cwf light,epxo*gain>>8
	{0x1c5b4, 0x02},//C indoor/outdoor switch 
	{0x1c5b5, 0xff},//C indoor/outdoor switch 
	{0x1c5b6, 0x04},//C indoor/outdoor switch 
	{0x1c5b7, 0xff},//C indoor/outdoor switch 

	//add awb  shift detect parameter according 0x1c734~0x1c736 
	{0x1ccd5, 0x38}, //CT_A, 2012.06.02 yuanyabin
	{0x1ccd6, 0x62}, //CT_C
	{0x1ccd7, 0xb9}, //CT_D

	{0x1c5cd, 0x00},//ori0x01 high light awb shift, modified by Jiangtao to avoid blurish when high CT 0310
	{0x1c5ce, 0x00},
	{0x1c5cf, 0xf0},
	{0x1c5d0, 0x01},
	{0x1c5d1, 0x20},
	{0x1c5d2, 0x03},
	{0x1c5d3, 0x00},
	{0x1c5d4, 0x40},//
	{0x1c5d5, 0xa0},//
	{0x1c5d6, 0xb0},//
	{0x1c5d7, 0xe8},//
	{0x1c5d8, 0x40},//
	{0x1c1c2, 0x00},
	{0x1c1c3, 0x20},

	{0x66206, 0x14}, //center(cwf) window threshold D0
	{0x66207, 0x1b}, //A threshold, range DX  0x15
	{0x66208, 0x15}, //day threshold, range DY 0xd
	{0x66209, 0x91}, //CWF X
	{0x6620a, 0x70}, //CWF Y
	{0x6620b, 0xe3}, //K_A_X2Y, a slop
	{0x6620c, 0xa5}, //K_D65_Y2X, d slop
	{0x6620d, 0x52}, //D65 Limit
	{0x6620e, 0x40}, //A Limit
	{0x6620f, 0x81}, //D65 split
	{0x66210, 0x60}, //A split
	{0x66201, 0x52},
	
	//add ccm detect parameter according 0x1c734~0x1c736 
	{0x1c1c8 ,0x01}, //center CT, CWF
	{0x1c1c9 ,0x4c},
	{0x1c1cc, 0x00},//daylight
	{0x1c1cd, 0xb0},
	{0x1c1d0, 0x02},//a
	{0x1c1d1, 0x40},
	
	{0x1c254, 0x0 },
	{0x1c255, 0xce},
	{0x1c256, 0x0 },
	{0x1c257, 0xe7},
	{0x1c258, 0x1 },
	{0x1c259, 0x69},
	{0x1c25a, 0x1 },
	{0x1c25b, 0xd2},
	

/* Color matrix */
	{0x1C1d8, 0x01},//center matrix, 
	{0x1C1d9, 0xF6},
	{0x1C1da, 0xFE},
	{0x1C1db, 0xFF},
	{0x1C1dc, 0x00},
	{0x1C1dd, 0x0B},
	{0x1C1de, 0xFF},
	{0x1C1df, 0xEE},
	{0x1C1e0, 0x01},
	{0x1C1e1, 0x97},
	{0x1C1e2, 0xFF},
	{0x1C1e3, 0x7B},
	{0x1C1e4, 0x00},
	{0x1C1e5, 0x11},
	{0x1C1e6, 0xFF},
	{0x1C1e7, 0x0A},
	{0x1C1e8, 0x01},
	{0x1C1e9, 0xE5},		
	
	{0x1C1FC, 0x00},//cmx left delta,daylight
	{0x1C1FD, 0x02},
	{0x1C1FE, 0x00},
	{0x1C1FF, 0x14},
	{0x1C200, 0xFF},
	{0x1C201, 0xEA},
	{0x1C202, 0xFF},
	{0x1C203, 0xA6},
	{0x1C204, 0x00},
	{0x1C205, 0x2C},
	{0x1C206, 0x00},
	{0x1C207, 0x2E},
	{0x1C208, 0xFF},
	{0x1C209, 0xE2},
	{0x1C20A, 0x00},
	{0x1C20B, 0x1B},
	{0x1C20C, 0x00},
	{0x1C20D, 0x03},
	
	{0x1C220, 0x00},//cmx right delta,a light
	{0x1C221, 0x6B},
	{0x1C222, 0xFF},
	{0x1C223, 0x7E},
	{0x1C224, 0x00},
	{0x1C225, 0x17},
	{0x1C226, 0xFF},
	{0x1C227, 0xE1},
	{0x1C228, 0x00},
	{0x1C229, 0x21},
	{0x1C22A, 0xFF},
	{0x1C22B, 0xFE},
	{0x1C22C, 0xFF},
	{0x1C22D, 0xD9},
	{0x1C22E, 0x00},
	{0x1C22F, 0x9F},
	{0x1C230, 0xFF},
	{0x1C231, 0x88},
	
	/* dpc */
	{0x65409, 0x08},  
	{0x6540a, 0x08},  
	{0x6540b, 0x08},  
	{0x6540c, 0x08},  
	{0x6540d, 0x0c},  
	{0x6540e, 0x08},  
	{0x6540f, 0x08},  
	{0x65410, 0x08},  
	{0x65408, 0x0b},  

	//dynamic high gain Y curve for dark color change 20120728
	{0x1d963, 0x1a}, 
	{0x1d964, 0x28}, 
	{0x1d965, 0x33}, 
	{0x1d966, 0x3f}, 
	{0x1d967, 0x4c}, 
	{0x1d968, 0x59}, 
	{0x1d969, 0x66}, 
	{0x1d96a, 0x72}, 
	{0x1d96b, 0x7e}, 
	{0x1d96c, 0x8a}, 
	{0x1d96d, 0x96}, 
	{0x1d96e, 0xa4}, 
	{0x1d96f, 0xb5}, 
	{0x1d970, 0xC9}, 
	{0x1d971, 0xe2}, 
	{0x1d8fe, 0x01}, //UV cut gain control
	{0x1d8ff, 0x50}, //low gain thres
	{0x1d900, 0x70}, //high gain thres
	{0x1d97f, 0x14}, //UV cut low bright thres
	{0x1d973, 0x20}, //UV cut high bright thres
	{0x1d972, 0x00}, //adaptive gamma enable
	{0x1d974, 0x01}, //low gain gamma
	{0x1d975, 0xe6},
	{0x1d976, 0x01}, //high gain gamma
	{0x1d977, 0xc0},
	{0x1d978, 0x01}, //dark image gamma
	{0x1d979, 0xb3},
	{0x1d97a, 0x88}, //low gain slope
	{0x1d97b, 0x50}, //high gain slope
	{0x1d97c, 0x38}, //dark image slope
	{0x1d97d, 0x14}, //low bright thres
	{0x1d97e, 0x20}, //high bright thres

	{0x1d99e, 0x01}, //dynamic UV curve
	//low gain UV curve 1/2
	{0x1d904, 0x46},
	{0x1d905, 0x64},
	{0x1d906, 0x74},
	{0x1d907, 0x7e},
	{0x1d908, 0x84},
	{0x1d909, 0x87},
	{0x1d90a, 0x88},
	{0x1d90b, 0x88},
	{0x1d90c, 0x88},
	{0x1d90d, 0x88},
	{0x1d90e, 0x88},
	{0x1d90f, 0x88},
	{0x1d910, 0x84},
	{0x1d911, 0x7a},
	{0x1d912, 0x6c},
	{0x1d913, 0x58},
	//high gain UV curve 1/2
	{0x1d914, 0x46},
	{0x1d915, 0x64},
	{0x1d916, 0x74},
	{0x1d917, 0x7e},
	{0x1d918, 0x84},
	{0x1d919, 0x87},
	{0x1d91a, 0x88},
	{0x1d91b, 0x88},
	{0x1d91c, 0x88},
	{0x1d91d, 0x88},
	{0x1d91e, 0x88},
	{0x1d91f, 0x88},
	{0x1d920, 0x84},
	{0x1d921, 0x7a},
	{0x1d922, 0x6c},
	{0x1d923, 0x58},

	//dynamic CT AWB, huiyan 20120810 add for new firmware AWB
	{0x1d924, 0x00},//enable
	{0x1d950, 0x00},//Br thres,super highlight threshold
	{0x1d951, 0x08},//Br thres	
	{0x1d952, 0x30},//Br Ratio,Ratio for transition region
	{0x1d8dc, 0x00},//Br thres0
	{0x1d8dd, 0x80}, //Br thres0
	{0x1d8de, 0x44}, //Br thres1
	{0x1d8df, 0x34}, //Br thres1
	{0x1d8da, 0x10}, //Br Ratio0
	{0x1d8db, 0x06}, //Br Ratio1
	{0x1d949, 0x0f}, //super highlight cwf thres //66206
	{0x1d925, 0x0f}, //highlight cwf thres //66206
	{0x1d926, 0x0f}, //middlelight cwf thres
	{0x1d927, 0x0f}, //lowlight cwf thres
	{0x1d94a, 0x1c}, //super highlight A thres //66207
	{0x1d928, 0x1c}, //highlight A thres //66207
	{0x1d929, 0x1c}, //middlelight A thres
	{0x1d92a, 0x22}, //lowlight A thres	
	{0x1d94b, 0x17}, //super highlight D thres //66208
	{0x1d92b, 0x17}, //highlight D thres //66208
	{0x1d92c, 0x17}, //middlelight D thres
	{0x1d92d, 0x1f}, //lowlight D thres	
	{0x1d94c, 0x58}, //super highlight D limit //6620d
	{0x1d92e, 0x58}, //highlight D limit //6620d
	{0x1d92f, 0x54}, //middlelight D limit
	{0x1d930, 0x4d}, //lowlight D limit		
	{0x1d94d, 0x42}, //super highlight A limit //6620e
	{0x1d931, 0x42}, //highlight A limit //6620e
	{0x1d932, 0x42}, //middlelight A limit
	{0x1d933, 0x3e}, //lowlight A limit		
	{0x1d94e, 0x76}, //super highlight D split //6620f
	{0x1d934, 0x76}, //highlight D split //6620f
	{0x1d935, 0x76}, //middlelight D split
	{0x1d936, 0x76}, //lowlight D split
	{0x1d94f, 0x57},	//super highlight A split //66210
	{0x1d937, 0x57}, //highlight A split //66210
	{0x1d938, 0x57}, //middlelight A split
	{0x1d939, 0x57}, //lowlight A split		
	{0x1d998, 0x1},
#endif
};

const struct isp_reg_t isp_init_regs_sonyimx188_byd[] = {
	/*{0x65006, 0x01},//y36721 delete, not needed now.*/

	/* y36721 2012-03-28 added. */
	/* y36721 2012-03-31 changed blc and lenc. */
	/*blc value=0x40(10bit)*/
	{0x1c58b, 0xff},
	{0x1c58c, 0xff},

#ifndef OVISP_DEBUG_MODE
/* AEC */
	{0x1c146, 0x30},//ori0x30 low AE target,should close
	{0x1c14a, 0x03},
	{0x1c14b, 0x0a},
	{0x1c14c, 0x0f},//aec fast step//          
	{0x1c14e, 0x08},//slow step//08
	{0x1c140, 0x01},//banding
	{0x1c13e, 0x02},//real gain mode for OV8830
	
	{0x66401, 0x00},//window weight
	{0x66402, 0x20},//StatWin_Left
	{0x66403, 0x00},
	{0x66404, 0x20},//StatWin_Top
	{0x66405, 0x00},
	{0x66406, 0x20},//StatWin_Right
	{0x66407, 0x00},
	{0x66408, 0x20},//StatWin_Bottom           
	{0x66409, 0x00},//definiton ofthe center 3x3 window
	{0x6640a, 0x80},//nWin_Left                
	{0x6640d, 0x00},                           
	{0x6640e, 0x70},//nWin_Top                 
	{0x66411, 0x03},                           
	{0x66412, 0xc0},//nWin_Width               
	{0x66415, 0x02},
	{0x66416, 0x00},//nWin_Height
	{0x6642e, 0x01},//nWin_Weight_0 weight pass
	{0x6642f, 0x01},//nWin_Weight_1
	{0x66430, 0x01},//nWin_Weight_2
	{0x66431, 0x01},//nWin_Weight_3
	{0x66432, 0x02},//nWin_Weight_4
	{0x66433, 0x02},//nWin_Weight_5
	{0x66434, 0x02},//nWin_Weight_6
	{0x66435, 0x02},//nWin_Weight_7
	{0x66436, 0x04},//nWin_Weight_8
	{0x66437, 0x02},//nWin_Weight_9
	{0x66438, 0x02},//nWin_Weight_10
	{0x66439, 0x02},//nWin_Weight_11
	{0x6643a, 0x02},//nWin_Weight_12
	{0x6644e, 0x03},//nWin_Weight_Shift
	{0x6644f, 0x04},//black level
	{0x66450, 0xf8},//saturate level
	{0x6645b, 0x1a},//black weight1
	{0x6645d, 0x1a},//black weight2
	{0x66460, 0x04},//saturate per1
	{0x66464, 0x0a},//saturate per2
	{0x66467, 0x14},//saturate weight1
	{0x66469, 0x14},//saturate weight2
	//auto AE control
	
/* Raw Stretch */
	{0x65020, 0x00},//RAW Stretch Target0x01-->00
	{0x66500, 0x28},
	{0x66501, 0x00},
	{0x66502, 0x8f},//0xff
	{0x66503, 0x0b},//0x0f
	{0x1c1b0, 0xff},
	{0x1c1b1, 0xff},
	{0x1c1b2, 0x01},
	{0x65905, 0x08},
	{0x66301, 0x02},//high level step
	{0x66302, 0xe0},//ref bin
	{0x66303, 0x06},//PsPer0 0a
	{0x66304, 0x10},//PsPer1
	{0x1c5a4, 0x01},//use new high stretch
	{0x1c5a5, 0x20},//stretch low step
	{0x1c5a6, 0x20},//stretch high step
	{0x1c5a7, 0x08},//stretch slow range
	{0x1c5a8, 0x02},//stretch slow step
	{0x1c1b8, 0x10},//ratio scale
	
	{0x1c5a0, 0x50},//AE target high, should close
	{0x1c5a2, 0x04},//target stable range
	{0x1c5a3, 0x06},//stretch target slow range
	
/* De-noise */
	{0x65604, 0x00},//Richard for new curve 0314
	{0x65605, 0x00},//Richard for new curve 0314
	{0x65606, 0x00},//Richard for new curve 0314
	{0x65607, 0x00},//Richard for new curve 0314

	{0x65510, 0x0F},//G dns slope change from 0x4 to 0xf Richard 0320
	{0x65511, 0x1E},//G dns slope change from 0x4 to 0xf Richard 0320
	{0x6551a, 0x03},// 1X  Raw G Dns improve white pixel 20120728
	{0x6551b, 0x09},// 2X
	{0x6551c, 0x0C},// 4X
	{0x6551d, 0x15},// 8X
	{0x6551e, 0x18},// 16X
	{0x6551f, 0x30},// 32X
	{0x65520, 0x40},// 64X
	{0x65522, 0x00},//RAW BR De-noise
	{0x65523, 0x00},  // 1X
	{0x65524, 0x00},
	{0x65525, 0x08},  // 2X
	{0x65526, 0x00},
	{0x65527, 0x16},  // 4X  
	{0x65528, 0x00},
	{0x65529, 0x20},   // 8X 
	{0x6552a, 0x00},
	{0x6552b, 0x30},  // 16X 
	{0x6552c, 0x00},
	{0x6552d, 0x40},  // 32X 
	{0x6552e, 0x00},
	{0x6552f, 0x50},  // 64X 
	
	{0x65c00, 0x03},//UV De-noise: gain 1X
	{0x65c01, 0x05},//gain 2X
	{0x65c02, 0x08},//gain 4X
	{0x65c03, 0x1f},//gain 8X
	{0x65c04, 0x1f},//gain 16X
	{0x65c05, 0x1f},//gain 32X
	

/* sharpeness */   
	{0x65600, 0x00},
	{0x65601, 0x20},//0319
	{0x65602, 0x00},
	{0x65603, 0x60}, //y00215412 change sharpness high gain threshod 0x40->0x60 20120814
	{0x65608, 0x06},
	{0x65609, 0x20},
	{0x6560c, 0x02},
	{0x6560d, 0x08}, //low gain sharpness, 20120814 0x20->0x30
	{0x6560e, 0x10},//MinSharpenTp
	{0x6560f, 0x60},//MaxSharpenTp
	{0x65610, 0x10},//MinSharpenTm
	{0x65611, 0x60},//MaxSharpenTm
	{0x65613, 0x10},//SharpenAlpha
	{0x65615, 0x08},//HFreq_thre
	{0x65617, 0x06},//HFreq_coef	

/* auto uv saturation */
	{0x1c4e8, 0x01},//Enable
	{0x1c4e9, 0xbf},// gain threshold1 40-->0b
	{0x1c4ea, 0xf7},//gain threshold2 78-->0d
	{0x1c4eb, 0x80}, //UV max saturation
	{0x1c4ec, 0x50}, //UV min saturation



/* Global Gamma */
	{0x1c49b, 0x01},
	{0x1c49c, 0x02},
	{0x1c49d, 0x01}, //gamma 2.0 0310
	{0x1c49e, 0x02},
	{0x1c49f, 0x01}, //gamma 2.0 0310
	{0x1c4a0, 0x00},
	{0x1c4a1, 0x18},
	{0x1c4a2, 0x00},
	{0x1c4a3, 0x88}, //gamma 2.0 0310 //avoid false contour Richard@0323


/* Tone Mapping */
	//contrast curve change for skin over exposure 20120728
	{0x1C4C0, 0x19},
	{0x1C4C1, 0x2f},
	{0x1C4C2, 0x3f},
	{0x1C4C3, 0x4b},
	{0x1C4C4, 0x55},
	{0x1C4C5, 0x5f},
	{0x1C4C6, 0x67},
	{0x1C4C7, 0x6F},
	{0x1C4C8, 0x78},
	{0x1C4C9, 0x82},
	{0x1C4CA, 0x8f},
	{0x1C4CB, 0xa0},
	{0x1C4CC, 0xB9},
	{0x1C4CD, 0xd1},
	{0x1C4CE, 0xe9},
	{0x1c4d4, 0x20},//EDR scale
	{0x1c4d5, 0x20},//EDR scale
	{0x1c4cf, 0x80},
	{0x65a00, 0x1b},
	{0x65a01, 0xc0}, //huiyan 0801

	//dark boost
	{0x1c4b0, 0x02},
	{0x1c4b1, 0x80},

	//YUV curve gain control,expoure frist
	{0x1c1b3, 0x40}, //Gain thre1
	{0x1c1b4, 0x70}, //Gain thre2 
	{0x1c1b5, 0x01}, //EDR gain control
	{0x1c1b6, 0x01}, //Curve Gain control
	{0x1c1b7, 0x40}, //after gamma cut ratio

	//Manual UV curve
	{0x1C998, 0x00},
	{0x1C999, 0x90},
	{0x1C99A, 0x00},
	{0x1C99B, 0xc8},
	{0x1C99C, 0x00},
	{0x1C99D, 0xef},
	{0x1C99E, 0x00},
	{0x1C99F, 0xff},
	{0x1C9A0, 0x01},
	{0x1C9A1, 0x00},
	{0x1C9A2, 0x01},
	{0x1C9A3, 0x00},
	{0x1C9A4, 0x01},
	{0x1C9A5, 0x00},
	{0x1C9A6, 0x01},
	{0x1C9A7, 0x00},
	{0x1C9A8, 0x01},
	{0x1C9A9, 0x00},
	{0x1C9AA, 0x01},
	{0x1C9AB, 0x00},
	{0x1C9AC, 0x01},
	{0x1C9AD, 0x00},
	{0x1C9AE, 0x01},
	{0x1C9AF, 0x00},
	{0x1C9B0, 0x00},
	{0x1C9B1, 0xfe},
	{0x1C9B2, 0x00},
	{0x1C9B3, 0xf5},
	{0x1C9B4, 0x00},
	{0x1C9B5, 0xe0},
	{0x1C9B6, 0x00},
	{0x1C9B7, 0xc0},

/* LENC */
	{0x1c247, 0x00},//three profile,color temperature based lens shading correction mode 1: enable 0: disable
	{0x1c24c, 0x00},
	{0x1c24d, 0x40},
	{0x1c24e, 0x00},
	{0x1c24f, 0x80},
	{0x1c248, 0x40},
	{0x1c24a, 0x20},
	{0x1c574, 0x00},
	{0x1c575, 0x20},
	{0x1c576, 0x00},
	{0x1c577, 0xf0},
	{0x1c578, 0x40},

	{0x65200, 0x0d},
	{0x65206, 0x3c},
	{0x65207, 0x04},
	{0x65208, 0x3c},
	{0x65209, 0x04},
	{0x6520a, 0x33},
	{0x6520b, 0x0c},
	{0x65214, 0x28},
	{0x65216, 0x20},
	{0x1d93d, 0x08},
	{0x1d93e, 0x00},
	{0x1d93f, 0x40},
	
	{0x1d940, 0xfc},
    {0x1d942, 0x04},
    {0x1d941, 0xfc},
    {0x1d943, 0x04},

    {0x1d944, 0x00},
    {0x1d946, 0x00},
    {0x1d945, 0xfc},
    {0x1d947, 0x04},
    
    {0x1c246, 0x01},//0:disable color temperature based LENC,1: enable color temperature based LENC


/* OVISP LENC setting for D65 Long Exposure (HDR/3D) */
	//Y channel re-back to old version(do not plus 8) 20120821 by y00215412
	{0x1c264, 0x1c}, //Y1
	{0x1c265, 0x12}, 
	{0x1c266, 0x11}, 
	{0x1c267, 0x11}, 
	{0x1c268, 0x13}, 
	{0x1c269, 0x1d}, 
	{0x1c26a, 0xa }, //Y2
	{0x1c26b, 0x9 }, 
	{0x1c26c, 0x7 }, 
	{0x1c26d, 0x7 }, 
	{0x1c26e, 0x9 }, 
	{0x1c26f, 0xb }, 
	{0x1c270, 0x7 }, //Y3
	{0x1c271, 0x2 }, 
	{0x1c272, 0x0 }, 
	{0x1c273, 0x0 }, 
	{0x1c274, 0x2 }, 
	{0x1c275, 0x7 }, 
	{0x1c276, 0x5 }, //Y4
	{0x1c277, 0x1 }, 
	{0x1c278, 0x0 }, 
	{0x1c279, 0x0 }, 
	{0x1c27a, 0x1 }, 
	{0x1c27b, 0x5 }, 
	{0x1c27c, 0xa }, //Y5
	{0x1c27d, 0x7 }, 
	{0x1c27e, 0x5 }, 
	{0x1c27f, 0x5 }, 
	{0x1c280, 0x7 }, 
	{0x1c281, 0xa }, 
	{0x1c282, 0x12}, //Y6
	{0x1c283, 0xf }, 
	{0x1c284, 0x10}, 
	{0x1c285, 0x10}, 
	{0x1c286, 0x10}, 
	{0x1c287, 0x14}, 
	{0x1c288, 0x22}, //Cb1
	{0x1c289, 0x1f}, 
	{0x1c28a, 0x1f}, 
	{0x1c28b, 0x20}, 
	{0x1c28c, 0x20}, 
	{0x1c28d, 0x1f}, //Cb2
	{0x1c28e, 0x20}, 
	{0x1c28f, 0x20}, 
	{0x1c290, 0x20}, 
	{0x1c291, 0x20}, 
	{0x1c292, 0x21}, //Cb3
	{0x1c293, 0x20}, 
	{0x1c294, 0x20}, 
	{0x1c295, 0x21}, 
	{0x1c296, 0x20}, 
	{0x1c297, 0x1f}, //Cb4
	{0x1c298, 0x20}, 
	{0x1c299, 0x20}, 
	{0x1c29a, 0x20}, 
	{0x1c29b, 0x20}, 
	{0x1c29c, 0x23}, //Cb5
	{0x1c29d, 0x1e}, 
	{0x1c29e, 0x20}, 
	{0x1c29f, 0x1f}, 
	{0x1c2a0, 0x1e},
	{0x1c2a1, 0x2e}, //Cr1
	{0x1c2a2, 0x31},      
	{0x1c2a3, 0x31},      
	{0x1c2a4, 0x31},      
	{0x1c2a5, 0x2d},      
	{0x1c2a6, 0x2c}, //Cr2
	{0x1c2a7, 0x28},      
	{0x1c2a8, 0x26},      
	{0x1c2a9, 0x28},      
	{0x1c2aa, 0x2d},      
	{0x1c2ab, 0x26}, //Cr3
	{0x1c2ac, 0x20},      
	{0x1c2ad, 0x1e},      
	{0x1c2ae, 0x20},      
	{0x1c2af, 0x25},      
	{0x1c2b0, 0x2b}, //Cr4
	{0x1c2b1, 0x26},      
	{0x1c2b2, 0x24},      
	{0x1c2b3, 0x26},      
	{0x1c2b4, 0x2b},      
	{0x1c2b5, 0x31}, //cr5
	{0x1c2b6, 0x33},      
	{0x1c2b7, 0x33},      
	{0x1c2b8, 0x33},      
	{0x1c2b9, 0x31},   
	


/*OVISP LENC setting for CWF light Long Exposure (HDR/3D)*/
        {0x1c2ba,0x1c},
        {0x1c2bb,0x12},
        {0x1c2bc,0x11},
        {0x1c2bd,0x11},
        {0x1c2be,0x13},
        {0x1c2bf,0x1d},
        {0x1c2c0,0xa },
    {0x1c2c1,0x9 },
    {0x1c2c2,0x7 },
        {0x1c2c3,0x7 },
    {0x1c2c4,0x9 },
        {0x1c2c5,0xb },
    {0x1c2c6,0x7 },
    {0x1c2c7,0x2 },
    {0x1c2c8,0x0 },
    {0x1c2c9,0x0 },
        {0x1c2ca,0x2 },
        {0x1c2cb,0x7 },
        {0x1c2cc,0x5 },
        {0x1c2cd,0x1 },
	{0x1c2ce,0x0 },
	{0x1c2cf,0x0 },
        {0x1c2d0,0x1 },
        {0x1c2d1,0x5 },
        {0x1c2d2,0xa },
        {0x1c2d3,0x7 },
        {0x1c2d4,0x5 },
        {0x1c2d5,0x5 },
        {0x1c2d6,0x7 },
        {0x1c2d7,0xa },
        {0x1c2d8,0x12},
        {0x1c2d9,0xf },
        {0x1c2da,0x10},
        {0x1c2db,0x10},
        {0x1c2dc,0x10},
        {0x1c2dd,0x14},
        {0x1c2de,0x22},
        {0x1c2df,0x1f},
        {0x1c2e0,0x1f},
	{0x1c2e1,0x20},
	{0x1c2e2,0x20},
        {0x1c2e3,0x1f},
	{0x1c2e4,0x20},
	{0x1c2e5,0x20},
	{0x1c2e6,0x20},
    {0x1c2e7,0x20},
        {0x1c2e8,0x21},
	{0x1c2e9,0x20},
	{0x1c2ea,0x20},
        {0x1c2eb,0x21},
	{0x1c2ec,0x20},
        {0x1c2ed,0x1f},
	{0x1c2ee,0x20},
	{0x1c2ef,0x20},
	{0x1c2f0,0x20},
	{0x1c2f1,0x20},
        {0x1c2f2,0x23},
        {0x1c2f3,0x1e},
	{0x1c2f4,0x20},
        {0x1c2f5,0x1f},
        {0x1c2f6,0x1e},
        {0x1c2f7,0x2e},
        {0x1c2f8,0x31},
        {0x1c2f9,0x31},
        {0x1c2fa,0x31},
        {0x1c2fb,0x2d},
    {0x1c2fc,0x2c},
        {0x1c2fd,0x28},
        {0x1c2fe,0x26},
        {0x1c2ff,0x28},
        {0x1c300,0x2d},
	{0x1c301,0x26},
    {0x1c302,0x20},
	{0x1c303,0x1e},
        {0x1c304,0x20},
        {0x1c305,0x25},
    {0x1c306,0x2b},
        {0x1c307,0x26},
        {0x1c308,0x24},
    {0x1c309,0x26},
        {0x1c30a,0x2b},
        {0x1c30b,0x31},
        {0x1c30c,0x33},
        {0x1c30d,0x33},
        {0x1c30e,0x33},
        {0x1c30f,0x31},
             
/*OVISP LENC setting for A light Long Exposure (HDR/3D)*/ 
        {0x1c310,0x1c},
        {0x1c311,0x12},
        {0x1c312,0x11},
        {0x1c313,0x11},
        {0x1c314,0x13},
        {0x1c315,0x1d},
        {0x1c316,0xa },
    {0x1c317,0x9 },
    {0x1c318,0x7 },
        {0x1c319,0x7 },
    {0x1c31a,0x9 },
        {0x1c31b,0xb },
    {0x1c31c,0x7 },
    {0x1c31d,0x2 },
    {0x1c31e,0x0 },
    {0x1c31f,0x0 },
        {0x1c320,0x2 },
        {0x1c321,0x7 },
        {0x1c322,0x5 },
        {0x1c323,0x1 },
	{0x1c324,0x0 },
	{0x1c325,0x0 },
        {0x1c326,0x1 },
        {0x1c327,0x5 },
        {0x1c328,0xa },
        {0x1c329,0x7 },
        {0x1c32a,0x5 },
        {0x1c32b,0x5 },
        {0x1c32c,0x7 },
        {0x1c32d,0xa },
        {0x1c32e,0x12},
        {0x1c32f,0xf },
        {0x1c330,0x10},
        {0x1c331,0x10},
        {0x1c332,0x10},
        {0x1c333,0x14},
        {0x1c334,0x22},
        {0x1c335,0x1f},
        {0x1c336,0x1f},
	{0x1c337,0x20},
	{0x1c338,0x20},
        {0x1c339,0x1f},
	{0x1c33a,0x20},
	{0x1c33b,0x20},
	{0x1c33c,0x20},
    {0x1c33d,0x20},
        {0x1c33e,0x21},
	{0x1c33f,0x20},
	{0x1c340,0x20},
        {0x1c341,0x21},
	{0x1c342,0x20},
        {0x1c343,0x1f},
	{0x1c344,0x20},
	{0x1c345,0x20},
	{0x1c346,0x20},
	{0x1c347,0x20},
        {0x1c348,0x23},
        {0x1c349,0x1e},
	{0x1c34a,0x20},
        {0x1c34b,0x1f},
        {0x1c34c,0x1e},
        {0x1c34d,0x2e},
        {0x1c34e,0x31},
        {0x1c34f,0x31},
        {0x1c350,0x31},
        {0x1c351,0x2d},
    {0x1c352,0x2c},
        {0x1c353,0x28},
        {0x1c354,0x26},
        {0x1c355,0x28},
        {0x1c356,0x2d},
	{0x1c357,0x26},
    {0x1c358,0x20},
	{0x1c359,0x1e},
        {0x1c35a,0x20},
        {0x1c35b,0x25},
    {0x1c35c,0x2b},
        {0x1c35d,0x26},
        {0x1c35e,0x24},
    {0x1c35f,0x26},
        {0x1c360,0x2b},
        {0x1c361,0x31},
        {0x1c362,0x33},
        {0x1c363,0x33},
        {0x1c364,0x33},
        {0x1c365,0x31},

/* AWB */
	{0x66201, 0x52},
	{0x66203, 0x14},//crop window
	{0x66211, 0xe8},//awb top limit
	{0x66212, 0x04},//awb bottom limit	
	
	{0x1c17c, 0x01},//CT mode
	{0x1c182, 0x04},
	{0x1c183, 0x00},//MinNum
	{0x1c184, 0x04},//AWB Step		
	{0x1c58d, 0x00},//LimitAWBAtD65Enable
	
	{0x1c1be, 0x00},//AWB offset
	{0x1c1bf, 0x00},
	{0x1c1c0, 0x00},
	{0x1c1c1, 0x00},

	{0x1c1aa, 0x00},//avgAllEnable	
	{0x1c1ad, 0x02},//weight of A
	{0x1c1ae, 0x10},//weight of D65
	{0x1c1af, 0x04},//weight of CWF
	
	{0x1c5ac, 0x80},//pre-gain 
	{0x1c5ad, 0x80},
	{0x1c5ae, 0x80},

	{0x1ccce, 0x02},//awb shift,open=02	
	{0x1cccf, 0x10},//B gain for A
	{0x1ccd0, 0x00},//R gain for A
	{0x1c5b8, 0x00},//B gain for C outdoor Richard@0517
	{0x1c5b9, 0x00},//R gain for C outdoor Richard@0517
	{0x1ccd1, 0x00},//B gain for C indoor Richard@0517
	{0x1ccd2, 0x00},//R gain for C indoor Richard@0517
	{0x1ccd3, 0x00},//B gain for D indoor
	{0x1ccd4, 0x00},//R gain for D indoor
	{0x1cccc, 0x00},//B gain for D outdoor
	{0x1cccd, 0x00},//R gain for D outdoor
                                            
	// for detect indoor/outdoor awb shift on cwf light,epxo*gain>>8
	{0x1c5b4, 0x02},//C indoor/outdoor switch 
	{0x1c5b5, 0xff},//C indoor/outdoor switch 
	{0x1c5b6, 0x04},//C indoor/outdoor switch 
	{0x1c5b7, 0xff},//C indoor/outdoor switch 

	//add awb  shift detect parameter according 0x1c734~0x1c736 
	{0x1ccd5, 0x3c}, //CT_A, 2012.06.02 yuanyabin
	{0x1ccd6, 0x67}, //CT_C
	{0x1ccd7, 0xb9}, //CT_D

	{0x1c5cd, 0x00},//ori0x01 high light awb shift, modified by Jiangtao to avoid blurish when high CT 0310
	{0x1c5ce, 0x00},
	{0x1c5cf, 0xf0},
	{0x1c5d0, 0x01},
	{0x1c5d1, 0x20},
	{0x1c5d2, 0x03},
	{0x1c5d3, 0x00},
	{0x1c5d4, 0x40},//
	{0x1c5d5, 0xa0},//
	{0x1c5d6, 0xb0},//
	{0x1c5d7, 0xe8},//
	{0x1c5d8, 0x40},//
	{0x1c1c2, 0x00},
	{0x1c1c3, 0x20},

	{0x66206, 0x14}, //center(cwf) window threshold D0
	{0x66207, 0x22}, //A threshold, range DX  0x15
	{0x66208, 0x13}, //day threshold, range DY 0xd
	{0x66209, 0x8f}, //CWF X
	{0x6620a, 0x73}, //CWF Y
	{0x6620b, 0xe0}, //K_A_X2Y, a slop
	{0x6620c, 0xa4}, //K_D65_Y2X, d slop
	{0x6620d, 0x50}, //D65 Limit
	{0x6620e, 0x3e}, //A Limit
	{0x6620f, 0x7e}, //D65 split
	{0x66210, 0x62}, //A split
	{0x66201, 0x52},
	
	//add ccm detect parameter according 0x1c734~0x1c736 
	{0x1c1c8 ,0x01}, //center CT, CWF
	{0x1c1c9 ,0x3a},
	{0x1c1cc, 0x00},//daylight
	{0x1c1cd, 0xb0},
	{0x1c1d0, 0x02},//a
	{0x1c1d1, 0x1d},
	
	{0x1c254, 0x0 },
	{0x1c255, 0xce},
	{0x1c256, 0x0 },
	{0x1c257, 0xe7},
	{0x1c258, 0x1 },
	{0x1c259, 0x69},
	{0x1c25a, 0x1 },
	{0x1c25b, 0xd2},
	

/* Color matrix */
	{0x1C1d8, 0x01},//center matrix, 
	{0x1C1d9, 0xF9},
	{0x1C1da, 0xFE},
	{0x1C1db, 0xF9},
	{0x1C1dc, 0x00},
	{0x1C1dd, 0x0E},
	{0x1C1de, 0xFF},
	{0x1C1df, 0xFC},
	{0x1C1e0, 0x01},
	{0x1C1e1, 0x76},
	{0x1C1e2, 0xFF},
	{0x1C1e3, 0x8E},
	{0x1C1e4, 0x00},
	{0x1C1e5, 0x10},
	{0x1C1e6, 0xFF},
	{0x1C1e7, 0x08},
	{0x1C1e8, 0x01},
	{0x1C1e9, 0xE8},		
	
	{0x1C1FC, 0xFF},//cmx left delta,daylight
	{0x1C1FD, 0xE0},
	{0x1C1FE, 0x00},
	{0x1C1FF, 0x24},
	{0x1C200, 0xFF},
	{0x1C201, 0xFC},
	{0x1C202, 0xFF},
	{0x1C203, 0xB1},
	{0x1C204, 0x00},
	{0x1C205, 0x1E},
	{0x1C206, 0x00},
	{0x1C207, 0x31},
	{0x1C208, 0xFF},
	{0x1C209, 0xEB},
	{0x1C20A, 0x00},
	{0x1C20B, 0x25},
	{0x1C20C, 0xFF},
	{0x1C20D, 0xF0},
	
	{0x1C220, 0x00},//cmx right delta,a light
	{0x1C221, 0x6B},
	{0x1C222, 0xFF},
	{0x1C223, 0x76},
	{0x1C224, 0x00},
	{0x1C225, 0x1F},
	{0x1C226, 0xFF},
	{0x1C227, 0xEA},
	{0x1C228, 0x00},
	{0x1C229, 0x19},
	{0x1C22A, 0xFF},
	{0x1C22B, 0xFD},
	{0x1C22C, 0xFF},
	{0x1C22D, 0xF7},
	{0x1C22E, 0x00},
	{0x1C22F, 0x8C},
	{0x1C230, 0xFF},
	{0x1C231, 0x7D},
	
	/* dpc */
	{0x65409, 0x08},  
	{0x6540a, 0x08},  
	{0x6540b, 0x08},  
	{0x6540c, 0x08},  
	{0x6540d, 0x0c},  
	{0x6540e, 0x08},  
	{0x6540f, 0x08},  
	{0x65410, 0x08},  
	{0x65408, 0x0b},  

	//dynamic high gain Y curve for dark color change 20120728
	{0x1d963, 0x19}, 
	{0x1d964, 0x2f}, 
	{0x1d965, 0x3f}, 
	{0x1d966, 0x4b}, 
	{0x1d967, 0x55}, 
	{0x1d968, 0x5f}, 
	{0x1d969, 0x67}, 
	{0x1d96a, 0x6F}, 
	{0x1d96b, 0x78}, 
	{0x1d96c, 0x82}, 
	{0x1d96d, 0x8f}, 
	{0x1d96e, 0xa0}, 
	{0x1d96f, 0xB9}, 
	{0x1d970, 0xd1}, 
	{0x1d971, 0xe9}, 
	{0x1d8fe, 0x01}, //UV cut gain control
	{0x1d8ff, 0x50}, //low gain thres
	{0x1d900, 0x70}, //high gain thres
	{0x1d97f, 0x14}, //UV cut low bright thres
	{0x1d973, 0x20}, //UV cut high bright thres
	{0x1d972, 0x00}, //adaptive gamma enable
	{0x1d974, 0x01}, //low gain gamma
	{0x1d975, 0xe6},
	{0x1d976, 0x01}, //high gain gamma
	{0x1d977, 0xc0},
	{0x1d978, 0x01}, //dark image gamma
	{0x1d979, 0xb3},
	{0x1d97a, 0x88}, //low gain slope
	{0x1d97b, 0x50}, //high gain slope
	{0x1d97c, 0x38}, //dark image slope
	{0x1d97d, 0x14}, //low bright thres
	{0x1d97e, 0x20}, //high bright thres

	{0x1d99e, 0x01}, //dynamic UV curve
	//low gain UV curve 1/2
	{0x1d904, 0x48},
	{0x1d905, 0x64},
	{0x1d906, 0x77},
	{0x1d907, 0x7F},
	{0x1d908, 0x80},
	{0x1d909, 0x80},
	{0x1d90a, 0x80},
	{0x1d90b, 0x80},
	{0x1d90c, 0x80},
	{0x1d90d, 0x80},
	{0x1d90e, 0x80},
	{0x1d90f, 0x80},
	{0x1d910, 0x7F},
	{0x1d911, 0x7A},
	{0x1d912, 0x70},
	{0x1d913, 0x60},
	//high gain UV curve 1/2
	{0x1d914, 0x48},
	{0x1d915, 0x64},
	{0x1d916, 0x77},
	{0x1d917, 0x7F},
	{0x1d918, 0x80},
	{0x1d919, 0x80},
	{0x1d91a, 0x80},
	{0x1d91b, 0x80},
	{0x1d91c, 0x80},
	{0x1d91d, 0x80},
	{0x1d91e, 0x80},
	{0x1d91f, 0x80},
	{0x1d920, 0x7F},
	{0x1d921, 0x7A},
	{0x1d922, 0x70},
	{0x1d923, 0x60},

	//dynamic CT AWB, huiyan 20120810 add for new firmware AWB
	{0x1d924, 0x00},//enable
	{0x1d950, 0x00},//Br thres,super highlight threshold
	{0x1d951, 0x08},//Br thres	
	{0x1d952, 0x30},//Br Ratio,Ratio for transition region
	{0x1d8dc, 0x00},//Br thres0
	{0x1d8dd, 0x80}, //Br thres0
	{0x1d8de, 0x44},//Br thres1
	{0x1d8df, 0x34},//Br thres1
	{0x1d8da, 0x10},//Br Ratio0
	{0x1d8db, 0x06},//Br Ratio1
	{0x1d949, 0x0f},//super highlight cwf thres //66206
	{0x1d925, 0x0f},//highlight cwf thres //66206
	{0x1d926, 0x0f},//middlelight cwf thres
	{0x1d927, 0x0f},//lowlight cwf thres
	{0x1d94a, 0x1c},//super highlight A thres //66207
	{0x1d928, 0x1c},//highlight A thres //66207
	{0x1d929, 0x1c},//middlelight A thres
	{0x1d92a, 0x22},//lowlight A thres	
	{0x1d94b, 0x17},//super highlight D thres //66208
	{0x1d92b, 0x17},//highlight D thres //66208
	{0x1d92c, 0x17},//middlelight D thres
	{0x1d92d, 0x1f},//lowlight D thres	
	{0x1d94c, 0x58},//super highlight D limit //6620d
	{0x1d92e, 0x58},//highlight D limit //6620d
	{0x1d92f, 0x54},//middlelight D limit
	{0x1d930, 0x4d},//lowlight D limit		
	{0x1d94d, 0x42},//super highlight A limit //6620e
	{0x1d931, 0x42},//highlight A limit //6620e
	{0x1d932, 0x42},//middlelight A limit
	{0x1d933, 0x3e},//lowlight A limit		
	{0x1d94e, 0x76},//super highlight D split //6620f
	{0x1d934, 0x76},//highlight D split //6620f
	{0x1d935, 0x76},//middlelight D split
	{0x1d936, 0x76},//lowlight D split
	{0x1d94f, 0x57},//super highlight A split //66210
	{0x1d937, 0x57},//highlight A split //66210
	{0x1d938, 0x57},//middlelight A split
	{0x1d939, 0x57},//lowlight A split		
	{0x1d998, 0x1 },

#endif
};

/*
 * should be calibrated, three lights, from 0x1c264
 * here is long exposure
 */
char sonyimx188_lensc_param[86*3] = {
};

/* should be calibrated, 6 groups 3x3, from 0x1c1d8 */
short sonyimx188_ccm_param[54] = {
};

char sonyimx188_awb_param[] = {
};

static framesize_s sonyimx188_framesizes[] = {
	{0, 14, 1280, 800, 3000, 900, 30, 25, 0x10E, 0xE1, 0x100, VIEW_FULL, RESOLUTION_4_3, true, {sonyimx188_framesize_full_30fps, ARRAY_SIZE(sonyimx188_framesize_full_30fps)} },
};

static camera_sensor sonyimx188_sensor;
static void sonyimx188_set_default(void);
#ifdef CONFIG_DEBUG_FS
extern u32 get_main_sensor_id(void);
#endif
/*
 **************************************************************************
 * FunctionName: sonyimx188_read_reg;
 * Description : read sonyimx188 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_read_reg(u16 reg, u8 *val)
{
	return k3_ispio_read_reg(sonyimx188_sensor.i2c_config.index,
				 sonyimx188_sensor.i2c_config.addr, reg, (u16 *)val, sonyimx188_sensor.i2c_config.val_bits);
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_write_reg;
 * Description : write sonyimx188 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_write_reg(u16 reg, u8 val, u8 mask)
{
	return k3_ispio_write_reg(sonyimx188_sensor.i2c_config.index,
			sonyimx188_sensor.i2c_config.addr, reg, val, sonyimx188_sensor.i2c_config.val_bits, mask);
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_write_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int sonyimx188_write_seq(const struct _sensor_reg_t *seq, u32 size, u8 mask)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	return k3_ispio_write_seq(sonyimx188_sensor.i2c_config.index,
			sonyimx188_sensor.i2c_config.addr, seq, size, sonyimx188_sensor.i2c_config.val_bits, mask);
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_write_isp_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void sonyimx188_write_isp_seq(const struct isp_reg_t *seq, u32 size)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	k3_ispio_write_isp_seq(seq, size);
}


/*
 **************************************************************************
 * FunctionName: sonyimx188_enum_frame_intervals;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_enum_frame_intervals(struct v4l2_frmivalenum *fi)
{
	assert(fi);

	print_debug("enter %s", __func__);
	if (fi->index >= CAMERA_MAX_FRAMERATE)
		return -EINVAL;

	fi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fi->discrete.numerator = 1;
	fi->discrete.denominator = (fi->index+1);
	return 0;
}


/*
 **************************************************************************
 * FunctionName: sonyimx188_get_format;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_get_format(struct v4l2_fmtdesc *fmt)
{
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_OVERLAY)
		fmt->pixelformat = sonyimx188_sensor.fmt[STATE_PREVIEW];
	else
		fmt->pixelformat = sonyimx188_sensor.fmt[STATE_CAPTURE];

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_enum_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_enum_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	u32 max_index = ARRAY_SIZE(camera_framesizes) - 1;
	u32 this_max_index = ARRAY_SIZE(sonyimx188_framesizes) - 1;

	assert(framesizes);

	print_debug("enter %s; ", __func__);

	if (framesizes->index > max_index) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	if ((camera_framesizes[framesizes->index].width > sonyimx188_framesizes[this_max_index].width)
		|| (camera_framesizes[framesizes->index].height > sonyimx188_framesizes[this_max_index].height)) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	framesizes->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	framesizes->discrete.width = sonyimx188_framesizes[framesizes->index].width;
	framesizes->discrete.height = sonyimx188_framesizes[framesizes->index].height;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_try_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_try_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	int max_index = ARRAY_SIZE(sonyimx188_framesizes) - 1;

	assert(framesizes);

	print_debug("Enter Function:%s  ", __func__);


	if ((framesizes->discrete.width <= sonyimx188_framesizes[max_index].width)
	    && (framesizes->discrete.height <= sonyimx188_framesizes[max_index].height)) {
		print_debug("===========width = %d", framesizes->discrete.width);
		print_debug("===========height = %d", framesizes->discrete.height);
		return 0;
	}

	print_error("frame size too large, [%d,%d]",
		    framesizes->discrete.width, framesizes->discrete.height);
	return -EINVAL;
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_set_framesizes;
 * Description : NA;
 * Input       : flag: if 1, set framesize to sensor,
 *					   if 0, only store framesize to camera_interface;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_set_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs, int flag, camera_setting_view_type view_type)
{
	int i = 0;
	bool match = false;

	assert(fs);

	print_info("Enter Function:%s State(%d), flag=%d, width=%d, height=%d",
		    __func__, state, flag, fs->width, fs->height);

	if (VIEW_FULL == view_type) {
		for (i = 0; i < ARRAY_SIZE(sonyimx188_framesizes); i++) {
			if ((sonyimx188_framesizes[i].width >= fs->width)
			    && (sonyimx188_framesizes[i].height >= fs->height)
			    && (VIEW_FULL == sonyimx188_framesizes[i].view_type)
			    && (camera_get_resolution_type(fs->width, fs->height)
			    <= sonyimx188_framesizes[i].resolution_type)) {
				fs->width = sonyimx188_framesizes[i].width;
				fs->height = sonyimx188_framesizes[i].height;
				match = true;
				break;
			}
		}
	}

	if (false == match) {
		for (i = 0; i < ARRAY_SIZE(sonyimx188_framesizes); i++) {
			if ((sonyimx188_framesizes[i].width >= fs->width)
			    && (sonyimx188_framesizes[i].height >= fs->height)
			    && (camera_get_resolution_type(fs->width, fs->height)
			    <= sonyimx188_framesizes[i].resolution_type)) {
				fs->width = sonyimx188_framesizes[i].width;
				fs->height = sonyimx188_framesizes[i].height;
				break;
			}
		}
	}

	if (i >= ARRAY_SIZE(sonyimx188_framesizes)) {
		print_error("request resolution larger than sensor's max resolution");
		return -EINVAL;
	}

	if (state == STATE_PREVIEW)
		sonyimx188_sensor.preview_frmsize_index = i;
	else
		sonyimx188_sensor.capture_frmsize_index = i;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_get_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_get_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs)
{
	int frmsize_index;

	assert(fs);

	if (state == STATE_PREVIEW)
		frmsize_index = sonyimx188_sensor.preview_frmsize_index;
	else if (state == STATE_CAPTURE)
		frmsize_index = sonyimx188_sensor.capture_frmsize_index;
	else
		return -EINVAL;

	fs->width = sonyimx188_framesizes[frmsize_index].width;
	fs->height = sonyimx188_framesizes[frmsize_index].height;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_init_reg;
 * Description : download initial seq for sensor init;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_init_reg(void)
{
	int size = 0;
	int scamera_id_value=0;

	print_debug("Enter Function:%s  , initsize=%d",
		    __func__, sizeof(sonyimx188_init_regs));

	scamera_id_value = gpio_get_value(SONYIMX188_SCAM_ID);
	if(scamera_id_value)
	{
		print_info("Enter Function:%s,BYD MODULE ",__func__);		
		strcpy(sonyimx188_sensor.info.name, "sonyimx188_byd");
		size = ARRAY_SIZE(isp_init_regs_sonyimx188_byd);
		sonyimx188_write_isp_seq(isp_init_regs_sonyimx188_byd, size);
	}
	else
	{
		print_info("Enter Function:%s,SUNNY MODULE ",__func__);		
		strcpy(sonyimx188_sensor.info.name, "sonyimx188_sunny");
		size = ARRAY_SIZE(isp_init_regs_sonyimx188_sunny);
		sonyimx188_write_isp_seq(isp_init_regs_sonyimx188_sunny, size);
	}

	if (0 != k3_ispio_init_csi(sonyimx188_sensor.mipi_index,
			sonyimx188_sensor.mipi_lane_count, sonyimx188_sensor.lane_clk)) {
		return -EFAULT;
	}

	size = ARRAY_SIZE(sonyimx188_init_regs);
	if (0 != sonyimx188_write_seq(sonyimx188_init_regs, size, 0x00)) {
		print_error("line %d, fail to init sonyimx188 sensor", __LINE__);
		return -EFAULT;
	}

	return 0;
}

static int sonyimx188_set_hflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);
	sonyimx188_sensor.hflip = flip;
	return 0;
}
static int sonyimx188_get_hflip(void)
{
	print_debug("enter %s", __func__);

	return sonyimx188_sensor.hflip;
}
static int sonyimx188_set_vflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);

	sonyimx188_sensor.vflip = flip;

	return 0;
}
static int sonyimx188_get_vflip(void)
{
	print_debug("enter %s", __func__);
	return sonyimx188_sensor.vflip;
}

static int sonyimx188_update_flip(u16 width, u16 height)
{
	u8 new_flip = ((sonyimx188_sensor.vflip << 1) | sonyimx188_sensor.hflip);
	print_debug("Enter %s  ", __func__);
	if (sonyimx188_sensor.old_flip != new_flip) {
		k3_ispio_update_flip((sonyimx188_sensor.old_flip ^ new_flip) & 0x03, width, height, PIXEL_ORDER_CHANGED);

		sonyimx188_sensor.old_flip = new_flip;
		sonyimx188_write_reg(SONYIMX188_FLIP, sonyimx188_sensor.vflip ? 0x02 : 0x00, ~0x02);
		sonyimx188_write_reg(SONYIMX188_FLIP, sonyimx188_sensor.hflip ? 0x01 : 0x00, ~0x01);
		msleep(200);
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_framesize_switch;
 * Description : switch frame size, used by preview and capture
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_framesize_switch(camera_state state)
{
	u8 next_frmsize_index;


	if (state == STATE_PREVIEW)
		next_frmsize_index = sonyimx188_sensor.preview_frmsize_index;
	else
		next_frmsize_index = sonyimx188_sensor.capture_frmsize_index;

	print_debug("Enter Function:%s frm index=%d", __func__, next_frmsize_index);

	if (next_frmsize_index >= ARRAY_SIZE(sonyimx188_framesizes)) {
		print_error("Unsupport sensor setting index: %d", next_frmsize_index);
		return -ETIME;
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_stream_on;
 * Description : download preview seq for sensor preview;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_stream_on(camera_state state)
{
	print_debug("Enter Function:%s ", __func__);
	return sonyimx188_framesize_switch(state);
}


/*  **************************************************************************
* FunctionName: sonyimx188_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static int sonyimx188_check_sensor(void)
{
	u8 idl = 0x1;
	u8 idh = 0x1;
	u16 id = 0;

	int ret = -1;
	u8 val = 1;

	ret = sonyimx188_read_reg(0x0000, &idh);
	ret = sonyimx188_read_reg(0x0001, &idl);
#if 0
	ret = sonyimx188_read_reg(0x0002, &val);
	print_info("%s: read 0x0002, ret=%d, val=%d.", __func__, ret, val);

	ret = sonyimx188_read_reg(0x0004, &val);
	print_info("%s: read 0x0004, ret=%d, val=%d.", __func__, ret, val);

	sonyimx188_write_reg(SONYIMX188_FLIP, 0x3,  0);
	ret = sonyimx188_read_reg(SONYIMX188_FLIP, &val);
	print_info("%s: read 0x0101, ret=%d, val=%d.", __func__, ret, val);
#endif
	id = ((idh << 8) | idl);
	print_info("sonyimx188 product id:0x%x", id);
#ifdef CONFIG_DEBUG_FS
	if (SONYIMX188_CHIP_ID != id) {
		id = (u16)get_main_sensor_id();
		print_info("sonyimx188 debugfs product id:0x%x", id);
	}
#endif
	if (SONYIMX188_CHIP_ID != id) {
		print_error("Invalid product id ,Could not load sensor sonyimx188");
		return -ENODEV;
	}

	return 0;
}

/****************************************************************************
* FunctionName: sonyimx188_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
int sonyimx188_power(camera_power_state power)
{
	int ret = 0;

	if (power == POWER_ON) {
		k3_ispldo_power_sensor(power, "pri-cameralog-vcc");
		ret = camera_power_core_ldo(power);
		udelay(200);
		k3_ispldo_power_sensor(power, "camera-vcc");
		k3_ispldo_power_sensor(power, "cameravcm-vcc");
		udelay(1);
		k3_ispldo_power_sensor(power, "sec-cameralog-vcc");
		//k3_ispgpio_power_sensor(&sonyimx188_sensor, power);
		k3_ispio_ioconfig(&sonyimx188_sensor, power);
	} else {
		k3_ispio_deinit_csi(sonyimx188_sensor.mipi_index);
		k3_ispio_ioconfig(&sonyimx188_sensor, power);
		//k3_ispgpio_power_sensor(&sonyimx188_sensor, power);
		k3_ispldo_power_sensor(power, "sec-cameralog-vcc");
		k3_ispldo_power_sensor(power, "cameravcm-vcc");
		camera_power_core_ldo(power);
		udelay(200);
		k3_ispldo_power_sensor(power, "pri-cameralog-vcc");
		k3_ispldo_power_sensor(power, "camera-vcc");
	}
	return ret;
}
/*
 * Here gain is in unit 1/16 of sensor gain,
 * y36721 todo, temporarily if sensor gain=0x10, ISO is 100
 * in fact we need calibrate an ISO-ET-gain table.
 */
u32 sonyimx188_gain_to_iso(int gain)
{
	return (gain * 100) / 0x10;
}

u32 sonyimx188_iso_to_gain(int iso)
{
	return (iso * 0x10) / 100;
}

void sonyimx188_set_gain(u32 gain)
{
	if (gain == 0)
		return;
	gain = 256 - (256 * 16) / gain;
	/*sonyimx188_write_reg(SONYIMX188_GAIN_REG_H, (gain >> 8) & 0xff, 0x00);*/
	sonyimx188_write_reg(SONYIMX188_GAIN_REG_L, gain & 0xff, 0x00);
}

void sonyimx188_set_exposure(u32 exposure)
{
	exposure >>= 4;
	sonyimx188_write_reg(SONYIMX188_EXPOSURE_REG_H, (exposure >> 8) & 0xff, 0x00);
	sonyimx188_write_reg(SONYIMX188_EXPOSURE_REG_L, exposure & 0xff, 0x00);
}


void sonyimx188_set_vts(u16 vts)
{
	print_debug("Enter %s  ", __func__);
	sonyimx188_write_reg(SONYIMX188_VTS_REG_H, (vts >> 8) & 0xff, 0x00);
	sonyimx188_write_reg(SONYIMX188_VTS_REG_L, vts & 0xff, 0x00);
}

u32 sonyimx188_get_vts_reg_addr(void)
{
	return SONYIMX188_VTS_REG_H;
}

static sensor_reg_t* sonyimx188_construct_vts_reg_buf(u16 vts, u16 *psize)
{
	static sensor_reg_t sonyimx188_vts_regs[] = {
		{SONYIMX188_VTS_REG_H, 0x00},
		{SONYIMX188_VTS_REG_L, 0x00},
	};

	print_debug("Enter %s,vts=%u", __func__, vts);
	sonyimx188_vts_regs[0].value = (vts >> 8) & 0xff ;
	sonyimx188_vts_regs[1].value = vts & 0xff;

	*psize = ARRAY_SIZE(sonyimx188_vts_regs);
	return sonyimx188_vts_regs;
}

static u32 sonyimx188_get_override_param(camera_override_type_t type)
{
	u32 ret_val = sensor_override_params[type];

	switch (type) {
	case OVERRIDE_ISO_HIGH:
		ret_val = SONYIMX188_MAX_ISO;
		break;

	case OVERRIDE_ISO_LOW:
		ret_val = SONYIMX188_MIN_ISO;
		break;

	case OVERRIDE_AUTOFPS_GAIN_LOW2MID:
		ret_val = SONYIMX188_AUTOFPS_GAIN_LOW2MID;
		break;
	case OVERRIDE_AUTOFPS_GAIN_MID2HIGH:
		ret_val = SONYIMX188_AUTOFPS_GAIN_MID2HIGH;
		break;

	/* reduce frame rate gain threshold */
	case OVERRIDE_AUTOFPS_GAIN_MID2LOW:
		ret_val = SONYIMX188_AUTOFPS_GAIN_MID2LOW;
		break;
	case OVERRIDE_AUTOFPS_GAIN_HIGH2MID:
		ret_val = SONYIMX188_AUTOFPS_GAIN_HIGH2MID;
		break;

	case OVERRIDE_FPS_MAX:
		ret_val = SONYIMX188_MAX_FRAMERATE;
		break;

	case OVERRIDE_FPS_MIDDLE:
		ret_val = SONYIMX188_MIDDLE_FRAMERATE;
        break;

	case OVERRIDE_FPS_MIN:
		ret_val = SONYIMX188_MIN_FRAMERATE;
		break;

	default:
		//print_error("%s:not override or invalid type %d, use default",__func__, type);
		break;
	}

	return ret_val;
}

static int sonyimx188_fill_denoise_buf(u8 *ybuf, u16 *uvbuf, u8 size, camera_state state, bool flash_on)
{
	u32 ae_th[3];
	u32 ae_value = get_writeback_gain() * get_writeback_expo() / 0x10;
	int index = sonyimx188_sensor.preview_frmsize_index;

	if( (ybuf==NULL)||(uvbuf==NULL)||(size <7 ) ){
		print_error("%s invalid arguments",__func__);
		return -1;
	}

	ybuf[0] = SONYIMX188_ISP_YDENOISE_COFF_1X;
	ybuf[1] = SONYIMX188_ISP_YDENOISE_COFF_2X;
	ybuf[2] = SONYIMX188_ISP_YDENOISE_COFF_4X;
	ybuf[3] = SONYIMX188_ISP_YDENOISE_COFF_8X;
	ybuf[4] = SONYIMX188_ISP_YDENOISE_COFF_16X;
	ybuf[5] = SONYIMX188_ISP_YDENOISE_COFF_32X;
	ybuf[6] = SONYIMX188_ISP_YDENOISE_COFF_64X;

	uvbuf[0] = SONYIMX188_ISP_UVDENOISE_COFF_1X;
	uvbuf[1] = SONYIMX188_ISP_UVDENOISE_COFF_2X;
	uvbuf[2] = SONYIMX188_ISP_UVDENOISE_COFF_4X;
	uvbuf[3] = SONYIMX188_ISP_UVDENOISE_COFF_8X;
	uvbuf[4] = SONYIMX188_ISP_UVDENOISE_COFF_16X;
	uvbuf[5] = SONYIMX188_ISP_UVDENOISE_COFF_32X;
	uvbuf[6] = SONYIMX188_ISP_UVDENOISE_COFF_64X;
	if (flash_on == false) {
		ae_th[0] = sonyimx188_sensor.frmsize_list[index].banding_step_50hz * 0x18;
		ae_th[1] = 3 * sonyimx188_sensor.frmsize_list[index].banding_step_50hz * 0x10;
		ae_th[2] = sonyimx188_sensor.frmsize_list[index].vts * 0x20;

		if (sonyimx188_sensor.frmsize_list[index].binning == false) {
			ae_th[0] *= 2;
			ae_th[1] *= 2;
			ae_th[2] *= 2;
		}

		/* simplify dynamic denoise threshold*/
		if (ae_value <= ae_th[0]) {
			ybuf[0] = SONYIMX188_ISP_YDENOISE_COFF_1X;
			ybuf[1] = SONYIMX188_ISP_YDENOISE_COFF_2X;
		} else {
			ybuf[0] = SONYIMX188_ISP_YDENOISE_COFF_2X;
			ybuf[1] = SONYIMX188_ISP_YDENOISE_COFF_2X;
		}
	} else {
		/* should calculated in capture_cmd again. */
		ybuf[0] = SONYIMX188_ISP_YDENOISE_COFF_1X;
		ybuf[1] = SONYIMX188_ISP_YDENOISE_COFF_2X;
	}

	return 0;
}
/*
 **************************************************************************
 * FunctionName: sonyimx188_reset;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_reset(camera_power_state power_state)
{
	print_debug("%s  ", __func__);

	if (POWER_ON == power_state) {
		k3_ispgpio_reset_sensor(sonyimx188_sensor.sensor_index, power_state,
			      sonyimx188_sensor.power_conf.reset_valid);
		k3_isp_io_enable_mclk(MCLK_ENABLE, sonyimx188_sensor.sensor_index);
		udelay(500);
	} else {
		k3_isp_io_enable_mclk(MCLK_DISABLE, sonyimx188_sensor.sensor_index);
		k3_ispgpio_reset_sensor(sonyimx188_sensor.sensor_index, power_state,
			      sonyimx188_sensor.power_conf.reset_valid);
		udelay(10);
	}

	return 0;
}


/*
 **************************************************************************
 * FunctionName: sonyimx188_init;
 * Description : sonyimx188 init function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : Error code indicating success or failure;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx188_init(void)
{
	static bool sonyimx188_check = false;
	print_debug("%s  ", __func__);

	if (false == sonyimx188_check)
	{
		if (check_suspensory_camera("SONYIMX188") != 1)
		{
			return -ENODEV;
		}
		sonyimx188_check = true;
	}	
	
#if 0
	if (!camera_timing_is_match(0)) {
		print_error("%s: sensor timing don't match.\n", __func__);
		return -ENODEV;
	}
#endif

	if (sonyimx188_sensor.owner && !try_module_get(sonyimx188_sensor.owner)) {
		print_error("%s: try_module_get fail", __func__);
		return -ENOENT;
	}

	k3_ispio_power_init("pri-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V*/
	k3_ispio_power_init("camera-vcc", LDO_VOLTAGE_18V, LDO_VOLTAGE_18V);	/*IO 1.8V*/
	k3_ispio_power_init("cameravcm-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*AF 2.85V*/
	k3_ispio_power_init("sec-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V*/

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_exit;
 * Description : sonyimx188 exit function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx188_exit(void)
{
	print_debug("enter %s", __func__);

	k3_ispio_power_deinit();

	if (sonyimx188_sensor.owner)
		module_put(sonyimx188_sensor.owner);

	print_debug("exit %s", __func__);
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_shut_down;
 * Description : sonyimx188 shut down function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx188_shut_down(void)
{
	print_debug("enter %s", __func__);
	//k3_ispgpio_power_sensor(&sonyimx188_sensor, POWER_OFF);
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_set_default;
 * Description : init sonyimx188_sensor;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx188_set_default(void)
{
	sonyimx188_sensor.init = sonyimx188_init;
	sonyimx188_sensor.exit = sonyimx188_exit;
	sonyimx188_sensor.shut_down = sonyimx188_shut_down;
	sonyimx188_sensor.reset = sonyimx188_reset;
	sonyimx188_sensor.check_sensor = sonyimx188_check_sensor;
	sonyimx188_sensor.power = sonyimx188_power;
	sonyimx188_sensor.init_reg = sonyimx188_init_reg;
	sonyimx188_sensor.stream_on = sonyimx188_stream_on;

	sonyimx188_sensor.get_format = sonyimx188_get_format;
	sonyimx188_sensor.set_flash = NULL;
	sonyimx188_sensor.get_flash = NULL;
	sonyimx188_sensor.set_scene = NULL;
	sonyimx188_sensor.get_scene = NULL;

	sonyimx188_sensor.enum_framesizes = sonyimx188_enum_framesizes;
	sonyimx188_sensor.try_framesizes = sonyimx188_try_framesizes;
	sonyimx188_sensor.set_framesizes = sonyimx188_set_framesizes;
	sonyimx188_sensor.get_framesizes = sonyimx188_get_framesizes;

	sonyimx188_sensor.enum_frame_intervals = sonyimx188_enum_frame_intervals;
	sonyimx188_sensor.try_frame_intervals = NULL;
	sonyimx188_sensor.set_frame_intervals = NULL;
	sonyimx188_sensor.get_frame_intervals = NULL;

	sonyimx188_sensor.get_capability = NULL;

	sonyimx188_sensor.set_hflip = sonyimx188_set_hflip;
	sonyimx188_sensor.get_hflip = sonyimx188_get_hflip;
	sonyimx188_sensor.set_vflip = sonyimx188_set_vflip;
	sonyimx188_sensor.get_vflip = sonyimx188_get_vflip;
	sonyimx188_sensor.update_flip = sonyimx188_update_flip;

	strcpy(sonyimx188_sensor.info.name, "sonyimx188");
	sonyimx188_sensor.interface_type = MIPI2;
	sonyimx188_sensor.mipi_lane_count = CSI_LINES_1;
	sonyimx188_sensor.mipi_index = CSI_INDEX_1;
	sonyimx188_sensor.sensor_index = CAMERA_SENSOR_SECONDARY;
	sonyimx188_sensor.skip_frames = 1;

	sonyimx188_sensor.power_conf.pd_valid = LOW_VALID;
	sonyimx188_sensor.power_conf.reset_valid = LOW_VALID;
	sonyimx188_sensor.power_conf.vcmpd_valid = LOW_VALID;

	sonyimx188_sensor.i2c_config.index = I2C_PRIMARY;
	sonyimx188_sensor.i2c_config.speed = I2C_SPEED_400;
	sonyimx188_sensor.i2c_config.addr = SONYIMX188_SLAVE_ADDRESS;
	sonyimx188_sensor.i2c_config.addr_bits = 16;
	sonyimx188_sensor.i2c_config.val_bits = I2C_8BIT;

	sonyimx188_sensor.preview_frmsize_index = 0;
	sonyimx188_sensor.capture_frmsize_index = 0;
	sonyimx188_sensor.frmsize_list = sonyimx188_framesizes;
	sonyimx188_sensor.fmt[STATE_PREVIEW] = V4L2_PIX_FMT_RAW10;
	sonyimx188_sensor.fmt[STATE_CAPTURE] = V4L2_PIX_FMT_RAW10;
#ifdef SONYIMX188_AP_WRITEAE_MODE /* just an example and test case for AP write AE mode */
	sonyimx188_sensor.aec_addr[0] = 0;
	sonyimx188_sensor.aec_addr[1] = 0;
	sonyimx188_sensor.aec_addr[2] = 0;
	sonyimx188_sensor.agc_addr[0] = 0; /*0x0204 high byte not needed*/
	sonyimx188_sensor.agc_addr[1] = 0;
	sonyimx188_sensor.ap_writeAE_delay = 1500; /* 5 expo and gain registers, 1500us is enough */
#else
	sonyimx188_sensor.aec_addr[0] = 0x0000;
	sonyimx188_sensor.aec_addr[1] = 0x0202;
	sonyimx188_sensor.aec_addr[2] = 0x0203;
	sonyimx188_sensor.agc_addr[0] = 0x0000; /*0x0204 high byte not needed*/
	sonyimx188_sensor.agc_addr[1] = 0x0205;
#endif
	sonyimx188_sensor.sensor_type = SENSOR_SONY;
	sonyimx188_sensor.sensor_rgb_type = SENSOR_RGGB;/* changed by y00231328. add bayer order*/

	sonyimx188_sensor.set_gain = sonyimx188_set_gain;
	sonyimx188_sensor.set_exposure = sonyimx188_set_exposure;

	sonyimx188_sensor.set_vts = sonyimx188_set_vts;
	sonyimx188_sensor.construct_vts_reg_buf = sonyimx188_construct_vts_reg_buf;
	sonyimx188_sensor.sensor_gain_to_iso = NULL;
	sonyimx188_sensor.sensor_iso_to_gain = NULL;
	sonyimx188_sensor.fill_denoise_buf = sonyimx188_fill_denoise_buf;
	sonyimx188_sensor.get_override_param = sonyimx188_get_override_param;

	sonyimx188_sensor.set_effect = NULL;

	sonyimx188_sensor.isp_location = CAMERA_USE_K3ISP;
	sonyimx188_sensor.sensor_tune_ops = NULL;

	sonyimx188_sensor.af_enable = 0;

	sonyimx188_sensor.image_setting.lensc_param = sonyimx188_lensc_param;
	sonyimx188_sensor.image_setting.ccm_param = sonyimx188_ccm_param;
	sonyimx188_sensor.image_setting.awb_param = sonyimx188_awb_param;

	sonyimx188_sensor.fps_max = 30;
	sonyimx188_sensor.fps_min = 16;
	sonyimx188_sensor.fps = 25;

	sonyimx188_sensor.owner = THIS_MODULE;

	sonyimx188_sensor.info.facing = CAMERA_FACING_BACK;
	sonyimx188_sensor.info.orientation = 90;   /*270;*/
	sonyimx188_sensor.info.focal_length = 439; /* 4.39mm*/
	sonyimx188_sensor.info.h_view_angle = 66; /*  66.1 degree */
	sonyimx188_sensor.info.v_view_angle = 50;
	sonyimx188_sensor.lane_clk = CLK_450M;
	sonyimx188_sensor.hflip = 0;
	sonyimx188_sensor.vflip = 0;
	sonyimx188_sensor.old_flip = 0;

}

/*
 **************************************************************************
 * FunctionName: sonyimx188_module_init;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static __init int sonyimx188_module_init(void)
{
	sonyimx188_set_default();
	return register_camera_sensor(sonyimx188_sensor.sensor_index, &sonyimx188_sensor);
}

/*
 **************************************************************************
 * FunctionName: sonyimx188_module_exit;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void __exit sonyimx188_module_exit(void)
{
	unregister_camera_sensor(sonyimx188_sensor.sensor_index, &sonyimx188_sensor);
}

MODULE_AUTHOR("Hisilicon");
module_init(sonyimx188_module_init);
module_exit(sonyimx188_module_exit);
MODULE_LICENSE("GPL");

/********************************** END **********************************************/
