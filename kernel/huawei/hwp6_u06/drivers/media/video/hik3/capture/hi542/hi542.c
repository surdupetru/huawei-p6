/*
 *  hi542 camera driver source file
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
#include "hi542.h"
/*#include "../isp/k3_isp_io.h"*/
#include <asm/bug.h>
#include <linux/device.h>

#define LOG_TAG "HI542"
/* #define DEBUG_DEBUG 1 */
#include "../isp/cam_log.h"
#include <../isp/cam_util.h>
#include <hsad/config_interface.h>
#include "../isp/k3_isp.h"
#include "../isp/k3_ispv1.h"
#include "../isp/k3_ispv1_afae.h"

#define HI542_SLAVE_ADDRESS 0x40
#define HI542_CHIP_ID       (0xB1)

#define HI542_CAM_MODULE_SKIPFRAME     4

#define HI542_FLIP		0x0011
#define HI542_PIXEL_ORDER_CHANGED	(3)

#define HI542_GAIN_REG	0x0129
#define HI542_EXPOSURE_REG_H	0x0115
#define HI542_EXPOSURE_REG_M1	0x0116
#define HI542_EXPOSURE_REG_M2	0x0117
#define HI542_EXPOSURE_REG_L	0x0118

#define HI542_FOXCONN_AP_WRITEAE_MODE
/* camera sensor override parameters, define in binning preview mode */
#define HI542_MAX_ISO			1600
#define HI542_MIN_ISO			100

#define HI542_AUTOFPS_GAIN_LOW2MID		0x36
#define HI542_AUTOFPS_GAIN_MID2HIGH		0x36
#define HI542_AUTOFPS_GAIN_HIGH2MID		0xE0
#define HI542_AUTOFPS_GAIN_MID2LOW		0xE0

#define HI542_MAX_FRAMERATE		29
#define HI542_MIDDLE_FRAMERATE	8
#define HI542_MIN_FRAMERATE		8
#define HI542_MIN_CAP_FRAMERATE	8
#define HI542_FLASH_TRIGGER_GAIN	0x60

/* camera sensor denoise parameters */
#define HI542_ISP_YDENOISE_COFF_1X		0x03
#define HI542_ISP_YDENOISE_COFF_2X		0x06
#define HI542_ISP_YDENOISE_COFF_4X		0x09
#define HI542_ISP_YDENOISE_COFF_8X		0x0c
#define HI542_ISP_YDENOISE_COFF_16X		0x0f
#define HI542_ISP_YDENOISE_COFF_32X		0x18
#define HI542_ISP_YDENOISE_COFF_64X		0x1f

#define HI542_ISP_UVDENOISE_COFF_1X		0x02
#define HI542_ISP_UVDENOISE_COFF_2X		0x04
#define HI542_ISP_UVDENOISE_COFF_4X		0x08
#define HI542_ISP_UVDENOISE_COFF_8X		0x10
#define HI542_ISP_UVDENOISE_COFF_16X		0x20
#define HI542_ISP_UVDENOISE_COFF_32X		0x20
#define HI542_ISP_UVDENOISE_COFF_64X		0x20

#define HI542_SHARPNESS_PREVIEW	0x28
#define HI542_SHARPNESS_CAPTURE	0x18

#define HI542_DIGTAL_GAIN_H	0x0589
#define HI542_DIGTAL_GAIN_L	0x058a

#define HI542_MAX_ANALOG_GAIN 8

static camera_capability hi542_cap[] = {
	{V4L2_CID_FOCAL_LENGTH, 353},//3.53mm
};

const struct isp_reg_t isp_init_regs_hi542[] = {
#ifndef OVISP_DEBUG_MODE
/* BLC */
	{0x1c58b, 0x00},
	{0x1c58c, 0x00},

/* AEC */
	{0x1c146, 0x2c}, //ori0x30 low AE target,should close
	{0x1c14a, 0x03},
	{0x1c14b, 0x0a},
	{0x1c14c, 0x0a}, //aec fast step//
	{0x1c14e, 0x08},//slow step//08
	{0x1c140, 0x01},//banding
	{0x1c13e, 0x02},//real gain mode for OV8830

	{0x66401, 0x00}, //window weight
	{0x66402, 0x80}, //StatWin_Left
	{0x66403, 0x00},
	{0x66404, 0x60}, //StatWin_Top
	{0x66405, 0x00},
	{0x66406, 0x80}, //StatWin_Right
	{0x66407, 0x00},
	{0x66408, 0x60}, //StatWin_Bottom
	{0x66409, 0x01}, //definiton ofthe center 3x3 window
	{0x6640a, 0x1f}, //nWin_Left
	{0x6640d, 0x00},
	{0x6640e, 0xd7}, //nWin_Top
	{0x66411, 0x02},
	{0x66412, 0xbf}, //nWin_Width
	{0x66415, 0x02},
	{0x66416, 0x0f}, //nWin_Height
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
	{0x65020, 0x01}, //RAW Stretch Target
	{0x66500, 0x28},
	{0x66501, 0x00},
	{0x66502, 0xff},
	{0x66503, 0x0f},
	{0x1c1b0, 0xff},
	{0x1c1b1, 0xff},
	{0x1c1b2, 0x01},
	{0x65905, 0x08},
	{0x66301, 0x02}, //high level step
	{0x66302, 0xb0}, //ref bin//90
	{0x66303, 0x0a}, //PsPer0
	{0x66304, 0x10}, //PsPer1
	{0x1c5a4, 0x01}, //use new high stretch
	{0x1c5a5, 0x20}, //stretch low step
	{0x1c5a6, 0x20}, //stretch high step
	{0x1c5a7, 0x08}, //stretch slow range
	{0x1c5a8, 0x02}, //stretch slow step
	{0x1c1b8, 0x10}, //ratio scale

	{0x1c5a0, 0x4c}, //AE target high, should close
	{0x1c5a2, 0x04},//target stable range
	{0x1c5a3, 0x06},//stretch target slow range

/* De-noise */
	{0x65604, 0x00},//Richard for new curve 0314
	{0x65605, 0x00},//Richard for new curve 0314
	{0x65606, 0x00},//Richard for new curve 0314
	{0x65607, 0x00},//Richard for new curve 0314

	{0x65510, 0x0F}, //G dns slope change from 0x4 to 0xf Richard 0320
	{0x65511, 0x1E}, //G dns slope change from 0x4 to 0xf Richard 0320

	{0x6551a, HI542_ISP_YDENOISE_COFF_1X}, //Raw G Dns, Richard 0320
	{0x6551b, HI542_ISP_YDENOISE_COFF_2X}, //Richard for new curve 0320
	{0x6551c, HI542_ISP_YDENOISE_COFF_4X}, //Richard for new curve 0320
	{0x6551d, HI542_ISP_YDENOISE_COFF_8X}, //Richard for new curve 0320
	{0x6551e, HI542_ISP_YDENOISE_COFF_16X}, //Richard for new curve 0320
	{0x6551f, HI542_ISP_YDENOISE_COFF_32X}, //Richard for new curve 0314
	{0x65520, HI542_ISP_YDENOISE_COFF_64X}, //Richard for new curve 0314
	{0x65522, 0x00}, //RAW BR De-noise
	{0x65523, HI542_ISP_UVDENOISE_COFF_1X}, // 1X
	{0x65524, 0x00},
	{0x65525, HI542_ISP_UVDENOISE_COFF_2X}, // 2X
	{0x65526, 0x00},
	{0x65527, HI542_ISP_UVDENOISE_COFF_4X}, // 4X
	{0x65528, 0x00},
	{0x65529, HI542_ISP_UVDENOISE_COFF_8X}, // 8X
	{0x6552a, 0x00},
	{0x6552b, HI542_ISP_UVDENOISE_COFF_16X}, // 16X
	{0x6552c, 0x00},
	{0x6552d, HI542_ISP_UVDENOISE_COFF_32X}, // 32X
	{0x6552e, 0x00},
	{0x6552f, HI542_ISP_UVDENOISE_COFF_64X}, // 64X

	{0x65c00, 0x04}, //UV De-noise
	{0x65c01, 0x08},
	{0x65c02, 0x0f},
	{0x65c03, 0x1f}, // 8X
	{0x65c04, 0x1f},
	{0x65c05, 0x1f},

/* sharpeness */
	{0x65600, 0x00},
	{0x65601, 0x20}, //0319
	{0x65602, 0x00},
	{0x65603, 0x60}, //y00215412 change sharpness high gain threshod 0x40->0x60 20120814
	{0x65608, 0x06},
	{0x65609, 0x20},  //30
	{0x6560c, 0x08},  //18
	{0x6560d, 0x08}, //low gain sharpness, 20120814 0x20->0x30 //28
	{0x6560e, 0x10},//MinSharpenTp
	{0x6560f, 0x60},//MaxSharpenTp
	{0x65610, 0x10},//MinSharpenTm
	{0x65611, 0x60},//MaxSharpenTm
	{0x65613, 0x18}, //SharpenAlpha
	{0x65615, 0x08},//HFreq_thre
	{0x65617, 0x06},//HFreq_coef

/* auto uv saturation */
	{0x1c4e8, 0x01}, //Enable
	{0x1c4e9, 0x30},
	{0x1c4ea, 0x78}, //gain threshold2 78-->0d
	{0x1c4eb, 0x80}, //UV max saturation
	{0x1c4ec, 0x68}, //ori is 0x70, yangyang for awb

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
    {0x1C4C0, 0x1c},
	{0x1C4C1, 0x2c},
	{0x1C4C2, 0x38},
	{0x1C4C3, 0x43},
	{0x1C4C4, 0x4d},
	{0x1C4C5, 0x58},
	{0x1C4C6, 0x63},
	{0x1C4C7, 0x6e},
	{0x1C4C8, 0x79},
	{0x1C4C9, 0x85},
	{0x1C4CA, 0x91},
	{0x1C4CB, 0xa0},
	{0x1C4CC, 0xb5},
	{0x1C4CD, 0xcf},
	{0x1C4CE, 0xe8},

	{0x1c4d4, 0x20}, //EDR scale
	{0x1c4d5, 0x20}, //EDR scale
	{0x1c4cf, 0x80},
	{0x65a00, 0x1b},
	{0x65a01, 0xc0}, //huiyan 0801

	//dark boost
	{0x1c4b0, 0x02},
	{0x1c4b1, 0x80},

	//curve gain control
	{0x1c1b3, 0x50}, //Gain thre1
	{0x1c1b4, 0x70}, //Gain thre2
	{0x1c1b5, 0x01}, //EDR gain control
	{0x1c1b6, 0x01}, //Curve Gain control
	{0x1c1b7, 0x40}, //after gamma cut ratio

	//Manual UV curve
	{0x1C998, 0x01},
	{0x1C999, 0x00},
	{0x1C99A, 0x01},
	{0x1C99B, 0x00},
	{0x1C99C, 0x01},
	{0x1C99D, 0x00},
	{0x1C99E, 0x01},
	{0x1C99F, 0x00},
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
	{0x1C9B0, 0x01},
	{0x1C9B1, 0x00},
	{0x1C9B2, 0x01},
	{0x1C9B3, 0x00},
	{0x1C9B4, 0x01},
	{0x1C9B5, 0x00},
	{0x1C9B6, 0x01},
	{0x1C9B7, 0x00},

/* LENC */
	{0x1c247, 0x02},//one profile,color temperature based lens shading correction mode 1: enable 0: disable
	{0x1c246, 0x00},
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
/* OVISP LENC setting for D50 Long Exposure (HDR/3D) */
	//Daylight//Y channel re-back to old version(do not plus 8) 20120821 by y00215412

	//D Light
	{0x1c264, 0x05}, //Y1
	{0x1c265, 0x05},
	{0x1c266, 0x05},
	{0x1c267, 0x05},
	{0x1c268, 0x05},
	{0x1c269, 0x06},
	{0x1c26a, 0x05}, //Y2
	{0x1c26b, 0x05},
	{0x1c26c, 0x04},
	{0x1c26d, 0x04},
	{0x1c26e, 0x05},
	{0x1c26f, 0x05},
	{0x1c270, 0x05}, //Y3
	{0x1c271, 0x05},
	{0x1c272, 0x04},
	{0x1c273, 0x04},
	{0x1c274, 0x05},
	{0x1c275, 0x06},
	{0x1c276, 0x05}, //Y4
	{0x1c277, 0x05},
	{0x1c278, 0x04},
	{0x1c279, 0x04},
	{0x1c27a, 0x05},
	{0x1c27b, 0x06},
	{0x1c27c, 0x05}, //Y5
	{0x1c27d, 0x05},
	{0x1c27e, 0x05},
	{0x1c27f, 0x05},
	{0x1c280, 0x05},
	{0x1c281, 0x05},
	{0x1c282, 0x05}, //Y6
	{0x1c283, 0x05},
	{0x1c284, 0x05},
	{0x1c285, 0x05},
	{0x1c286, 0x05},
	{0x1c287, 0x06},
	{0x1c288, 0x1d}, //Cb1
	{0x1c289, 0x1e},
	{0x1c28a, 0x1d},
	{0x1c28b, 0x1d},
	{0x1c28c, 0x1e},
	{0x1c28d, 0x1f}, //Cb2
	{0x1c28e, 0x1f},
	{0x1c28f, 0x1e},
	{0x1c290, 0x1e},
	{0x1c291, 0x1d},
	{0x1c292, 0x20}, //Cb3
	{0x1c293, 0x21},
	{0x1c294, 0x21},
	{0x1c295, 0x20},
	{0x1c296, 0x1d},
	{0x1c297, 0x20}, //Cb4
	{0x1c298, 0x1f},
	{0x1c299, 0x20},
	{0x1c29a, 0x1f},
	{0x1c29b, 0x1e},
	{0x1c29c, 0x1f}, //Cb5
	{0x1c29d, 0x1e},
	{0x1c29e, 0x1e},
	{0x1c29f, 0x1e},
	{0x1c2a0, 0x1c},

	{0x1c2a1, 0x21}, //Cr1
	{0x1c2a2, 0x23},
	{0x1c2a3, 0x23},
	{0x1c2a4, 0x22},
	{0x1c2a5, 0x21},
	{0x1c2a6, 0x21}, //Cr2
	{0x1c2a7, 0x22},
	{0x1c2a8, 0x21},
	{0x1c2a9, 0x22},
	{0x1c2aa, 0x23},
	{0x1c2ab, 0x22}, //Cr3
	{0x1c2ac, 0x21},
	{0x1c2ad, 0x1f},
	{0x1c2ae, 0x20},
	{0x1c2af, 0x23},
	{0x1c2b0, 0x22}, //Cr4
	{0x1c2b1, 0x22},
	{0x1c2b2, 0x20},
	{0x1c2b3, 0x21},
	{0x1c2b4, 0x23},
	{0x1c2b5, 0x22}, //cr5
	{0x1c2b6, 0x24},
	{0x1c2b7, 0x25},
	{0x1c2b8, 0x25},
	{0x1c2b9, 0x23},

	//CWF

	{0x1c2ba,0x05}, //Y1
	{0x1c2bb,0x05},
	{0x1c2bc,0x05},
	{0x1c2bd,0x05},
	{0x1c2be,0x05},
	{0x1c2bf,0x05},
	{0x1c2c0,0x05}, //Y2
	{0x1c2c1,0x05},
	{0x1c2c2,0x04},
	{0x1c2c3,0x04},
	{0x1c2c4,0x05},
	{0x1c2c5,0x05},
	{0x1c2c6,0x05}, //Y3
	{0x1c2c7,0x04},
	{0x1c2c8,0x04},
	{0x1c2c9,0x04},
	{0x1c2ca,0x05},
	{0x1c2cb,0x05},
	{0x1c2cc,0x05}, //Y4
	{0x1c2cd,0x05},
	{0x1c2ce,0x04},
	{0x1c2cf,0x04},
	{0x1c2d0,0x05},
	{0x1c2d1,0x05},
	{0x1c2d2,0x05}, //Y5
	{0x1c2d3,0x05},
	{0x1c2d4,0x05},
	{0x1c2d5,0x05},
	{0x1c2d6,0x05},
	{0x1c2d7,0x05},
	{0x1c2d8,0x05}, //Y6
	{0x1c2d9,0x05},
	{0x1c2da,0x05},
	{0x1c2db,0x05},
	{0x1c2dc,0x05},
	{0x1c2dd,0x06},

	{0x1c2de, 0x1d}, //Cb1
	{0x1c2df, 0x1d},
	{0x1c2e0, 0x1c},
	{0x1c2e1, 0x1d},
	{0x1c2e2, 0x1e},
	{0x1c2e3, 0x1e}, //Cb2
	{0x1c2e4, 0x1e},
	{0x1c2e5, 0x1e},
	{0x1c2e6, 0x1d},
	{0x1c2e7, 0x1c},
	{0x1c2e8, 0x1f}, //Cb3
	{0x1c2e9, 0x21},
	{0x1c2ea, 0x21},
	{0x1c2eb,0x20},
	{0x1c2ec, 0x1d},
	{0x1c2ed, 0x1f}, //Cb4
	{0x1c2ee,0x1f},
	{0x1c2ef,0x20},
	{0x1c2f0, 0x1f},
	{0x1c2f1, 0x1d},
	{0x1c2f2,0x1f}, //Cb5
	{0x1c2f3,0x1e},
	{0x1c2f4, 0x1e},
	{0x1c2f5, 0x1e},
	{0x1c2f6, 0x1d},

	{0x1c2f7, 0x20}, //Cr1
	{0x1c2f8, 0x22},
	{0x1c2f9,0x22},
	{0x1c2fa, 0x22},
	{0x1c2fb, 0x1f},
	{0x1c2fc,0x20}, //Cr2
	{0x1c2fd, 0x21},
	{0x1c2fe, 0x20},
	{0x1c2ff, 0x21},
	{0x1c300,0x22},
	{0x1c301, 0x21}, //Cr3
	{0x1c302, 0x20},
	{0x1c303, 0x20},
	{0x1c304, 0x20},
	{0x1c305,0x21},
	{0x1c306,0x20}, //Cr4
	{0x1c307,0x20},
	{0x1c308,0x1f},
	{0x1c309, 0x20},
	{0x1c30a,0x22},
	{0x1c30b, 0x20}, //cr5
	{0x1c30c, 0x23},
	{0x1c30d, 0x23},
	{0x1c30e, 0x23},
	{0x1c30f, 0x21},

	//a
	{0x1c310,0x06},//Y1
	{0x1c311,0x05},
	{0x1c312,0x05},
	{0x1c313,0x05},
	{0x1c314,0x05},
	{0x1c315,0x06},
	{0x1c316,0x06},//Y2
	{0x1c317,0x05},
	{0x1c318,0x05},
	{0x1c319,0x05},
	{0x1c31a,0x06},
	{0x1c31b,0x06},
	{0x1c31c,0x06},//Y3
	{0x1c31d,0x05},
	{0x1c31e,0x04},
	{0x1c31f,0x04},
	{0x1c320,0x05},
	{0x1c321,0x07},
	{0x1c322,0x06},//Y4
	{0x1c323,0x05},
	{0x1c324,0x04},
	{0x1c325,0x04},
	{0x1c326,0x05},
	{0x1c327,0x07},
	{0x1c328,0x06},//Y5
	{0x1c329,0x06},
	{0x1c32a,0x05},
	{0x1c32b,0x05},
	{0x1c32c,0x06},
	{0x1c32d,0x06},
	{0x1c32e,0x06},//Y6
	{0x1c32f,0x06},
	{0x1c330,0x06},
	{0x1c331,0x06},
	{0x1c332,0x06},
	{0x1c333,0x07},
	{0x1c334,0x1b},//Cb1
	{0x1c335,0x1c},
	{0x1c336,0x1b},
	{0x1c337,0x1c},
	{0x1c338,0x1d},
	{0x1c339,0x1d},//Cb2
	{0x1c33a,0x1e},
	{0x1c33b,0x1d},
	{0x1c33c,0x1d},
	{0x1c33d,0x1c},
	{0x1c33e,0x1d},//Cb3
	{0x1c33f,0x20},
	{0x1c340,0x21},
	{0x1c341,0x20},
	{0x1c342,0x1c},
	{0x1c343,0x1e},//Cb4
	{0x1c344,0x1e},
	{0x1c345,0x1f},
	{0x1c346,0x1f},
	{0x1c347,0x1c},
	{0x1c348,0x1c},//Cb5
	{0x1c349,0x1e},
	{0x1c34a,0x1d},
	{0x1c34b,0x1d},
	{0x1c34c,0x1c},
	{0x1c34d,0x23},//Cr1
	{0x1c34e,0x27},
	{0x1c34f,0x28},
	{0x1c350,0x27},
	{0x1c351,0x24},
	{0x1c352,0x26},//Cr2
	{0x1c353,0x25},
	{0x1c354,0x23},
	{0x1c355,0x25},
	{0x1c356,0x27},
	{0x1c357,0x27},//Cr3
	{0x1c358,0x21},
	{0x1c359,0x1e},
	{0x1c35a,0x21},
	{0x1c35b,0x27},
	{0x1c35c,0x25},//Cr4
	{0x1c35d,0x24},
	{0x1c35e,0x22},
	{0x1c35f,0x23},
	{0x1c360,0x27},
	{0x1c361,0x24},//cr5
	{0x1c362,0x28},
	{0x1c363,0x29},
	{0x1c364,0x28},
	{0x1c365,0x25},

/* AWB */
	{0x66201, 0x52},
	{0x66203, 0x14},//crop window
	{0x66211, 0xe8},//awb top limit
	{0x66212, 0x04},//awb bottom limit
	//{0x1c17c, 0x01},//CT mode
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
	{0x1c1ae, 0x08},//weight of D65
	{0x1c1af, 0x04},//weight of CWF

	{0x1c5ac, 0x80},//pre-gain
	{0x1c5ad, 0x80},
	{0x1c5ae, 0x80},

	{0x1ccce, 0x02}, //awb shift,open=02
	/* new AWB shift */
	{0x1d902, 0x01},//Enable new awb shift
	{0x1d8e0, 0x00},//nAWBShiftBrThres1
	{0x1d8e1, 0xc8},//super highlight	/* 200lines */
	{0x1d8e2, 0x01},//nAWBShiftBrThres2
	{0x1d8e3, 0xf4},//highlight_TH_H	/* 500lines */
	{0x1d8f4, 0x03},//nAWBShiftBrThres3
	{0x1d8f5, 0x84},//highlight_TH_L	/* 900lines */
	{0x1d8f6, 0x07},//nAWBShiftBrThres4
	{0x1d8f7, 0x08},//middlelight		/* 1800lines */
	{0x1d8f2, 0x04},//nAWBShiftLowBrightThres lowlight_TH_L
	{0x1d8f3, 0x24},//nAWBShiftHighBrightThres lowlight_TH_H

	{0x1ccd5, 0x38}, //CT_A, 2012.06.02 yuanyabin
	{0x1ccd6, 0x58}, //CT_C
	{0x1ccd7, 0x88}, //CT_D

	{0x1d9c8, 0x78},//B gain for A High light
	{0x1d9c9, 0x78},//B gain for A
	{0x1d9ca, 0x78},//B gain for A
	{0x1d9cb, 0x78},//B gain for A low light

	{0x1d9cc, 0x80},//R gain for A High light
	{0x1d9cd, 0x80},//R gain for A
	{0x1d9ce, 0x80},//R gain for A
	{0x1d9cf, 0x80},//R gain for A low light

	{0x1d9d0, 0x7c},//B gain for cwf High light
	{0x1d9d1, 0x7c},//B gain for cwf
	{0x1d9d2, 0x7c},//B gain for cwf
	{0x1d9d3, 0x7c},//B gain for cwf low light

	{0x1d9d4, 0x84},//R gain for cwf High light
	{0x1d9d5, 0x84},//R gain for cwf
	{0x1d9d6, 0x84},//R gain for cwf
	{0x1d9d7, 0x84},//R gain for cwf low light

	{0x1d9d8, 0x80},//B gain for D High light
	{0x1d9d9, 0x80},//B gain for D
	{0x1d9da, 0x80},//B gain for D
	{0x1d9db, 0x80},//B gain for D low light

	{0x1d9dc, 0x80},//R gain for D High light
	{0x1d9dd, 0x80},//R gain for D
	{0x1d9de, 0x80},//R gain for D
	{0x1d9df, 0x80},//R gain for D low light

	//add by wenjuan 00182148 for new shift(sloving green in lowlight) 20130228  end

	{0x1c5cd, 0x00}, //high light awb shift, modified by Jiangtao to avoid blurish when high CT 0310
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

	{0x66206, 0x0e}, //center(cwf) window threshold D0
	{0x66207, 0x14}, //A threshold, range DX  0x15
	{0x66208, 0x11}, //day threshold, range DY 0xd
	{0x66209, 0x7a}, //CWF X
	{0x6620a, 0x63}, //CWF Y
	{0x6620b, 0xc0}, //K_A_X2Y, a slop
	{0x6620c, 0xa8}, //K_D65_Y2X, d slop
	{0x6620d, 0x52}, //D65 Limit
	{0x6620e, 0x3c}, //A Limit
	{0x6620f, 0x6e}, //D65 split
	{0x66210, 0x56}, //A split
	{0x66201, 0x52},

	//add ccm detect parameter according 0x1c734~0x1c736
	{0x1c1c8 ,0x01}, //center CT, CWF
	{0x1c1c9 ,0x78},
	{0x1c1cc, 0x00}, //daylight
	{0x1c1cd, 0xd8},
	{0x1c1d0, 0x02}, //a
	{0x1c1d1, 0x18},

	{0x1c254, 0x00},
	{0x1c255, 0xe0},//d
	{0x1c256, 0x01},
	{0x1c257, 0x00},//d_c
	{0x1c258, 0x01},
	{0x1c259, 0xf0},//c_a
	{0x1c25a, 0x02},
	{0x1c25b, 0x10},//a

/* Color matrix */
	{0x1c1d8, 0x01},
	{0x1c1d9, 0x64},
	{0x1c1da, 0x00},
	{0x1c1db, 0x00},
	{0x1c1dc, 0xff},
	{0x1c1dd, 0x9c},

	{0x1c1de, 0xFF},
	{0x1c1df, 0x83},
	{0x1c1e0, 0x01},
	{0x1c1e1, 0xfa},
	{0x1c1e2, 0xFF},
	{0x1c1e3, 0x83},

	{0x1c1e4, 0x00},
	{0x1c1e5, 0x64},
	{0x1c1e6, 0xFd},
	{0x1c1e7, 0x44},
	{0x1c1e8, 0x03},
	{0x1c1e9, 0x58},
	//d
	{0x1c1fc, 0x00},
	{0x1c1fd, 0x00},
	{0x1c1fe, 0x00},
	{0x1c1ff, 0x00},
	{0x1c200, 0x00},
	{0x1c201, 0x00},

	{0x1c202, 0x00},
	{0x1c203, 0x00},
	{0x1c204, 0x00},
	{0x1c205, 0x00},
	{0x1c206, 0x00},
	{0x1c207, 0x00},

	{0x1c208, 0x00},
	{0x1c209, 0x32},
	{0x1c20a, 0x00},
	{0x1c20b, 0x00},
	{0x1c20c, 0xff},
	{0x1c20d, 0xce},
	//a
	{0x1c220, 0x00},
	{0x1c221, 0xc8},
	{0x1c222, 0xfe},
	{0x1c223, 0xd4},
	{0x1c224, 0x00},
	{0x1c225, 0x64},

	{0x1c226, 0xFF},
	{0x1c227, 0xe2},
	{0x1c228, 0x00},
	{0x1c229, 0x32},
	{0x1c22a, 0xFF},
	{0x1c22b, 0xEc},

	{0x1c22c, 0xff},
	{0x1c22d, 0x06},
	{0x1c22e, 0x01},
	{0x1c22f, 0xc2},
	{0x1c230, 0xff},
	{0x1c231, 0x38},


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
    //low gain Y curv

	//dynamic high gain Y curve for dark color change 20120728
	{0x1d963, 0x1c},
	{0x1d964, 0x2f},
	{0x1d965, 0x3e},
	{0x1d966, 0x4a},
	{0x1d967, 0x54},
	{0x1d968, 0x5d},
	{0x1d969, 0x66},
	{0x1d96a, 0x70},
	{0x1d96b, 0x7a},
	{0x1d96c, 0x85},
	{0x1d96d, 0x91},
	{0x1d96e, 0xa0},
	{0x1d96f, 0xb0},
	{0x1d970, 0xc5},
	{0x1d971, 0xdf},

/* auto uv saturation */
	{0x1d8fe, 0x01}, //UV cut gain control
	{0x1d8ff, 0x50}, //low gain thres
	{0x1d900, 0x70}, //high gain thres
	{0x1d97f, 0x14}, //UV cut low bright thres
	{0x1d973, 0x20}, //UV cut high bright thres
	{0x1d972, 0x01}, //adaptive gamma enable
	{0x1d974, 0x01}, //low gain gamma	1e6: 1.89
	{0x1d975, 0xc0},
	{0x1d976, 0x01}, //high gain gamma	1c0: 1.75
	{0x1d977, 0xb3},
	{0x1d978, 0x01}, //dark image gamma	1b3: 1.7
	{0x1d979, 0x99}, //			199: 1.6
	{0x1d97a, 0x58}, //low gain slope	0x88: 8.5
	{0x1d97b, 0x38}, //high gain slope	0x50: 5
	{0x1d97c, 0x28}, //dark image slope	0x38: 3.5;0x28: 2.5
	{0x1d97d, 0x14}, //low bright thres
	{0x1d97e, 0x20}, //high bright thres

	{0x1d99e, 0x01}, //dynamic UV curve
	//low gain UV curve 1/2
	{0x1d904, 0x30},
	{0x1d905, 0x50},
	{0x1d906, 0x67},
	{0x1d907, 0x74},
	{0x1d908, 0x78},
	{0x1d909, 0x78},
	{0x1d90a, 0x78},
	{0x1d90b, 0x78},
	{0x1d90c, 0x78},
	{0x1d90d, 0x78},
	{0x1d90e, 0x78},
	{0x1d90f, 0x78},
	{0x1d910, 0x78},
	{0x1d911, 0x78},
	{0x1d912, 0x73},
	{0x1d913, 0x55},
	//high gain UV curve 1/2
	{0x1d914, 0x30},
	{0x1d915, 0x50},
	{0x1d916, 0x67},
	{0x1d917, 0x74},
	{0x1d918, 0x78},
	{0x1d919, 0x78},
	{0x1d91a, 0x78},
	{0x1d91b, 0x78},
	{0x1d91c, 0x78},
	{0x1d91d, 0x78},
	{0x1d91e, 0x78},
	{0x1d91f, 0x78},
	{0x1d920, 0x78},
	{0x1d921, 0x78},
	{0x1d922, 0x73},
	{0x1d923, 0x55},

	//dynamic CT AWB, huiyan 20120810 add for new firmware AWB
	{0x1d924, 0x00}, //enable
	{0x1d950, 0x00}, //Br thres,super highlight threshold
	{0x1d951, 0x40}, //Br thres
	{0x1d952, 0x30}, //Br Ratio,Ratio for transition region

	{0x1d8dc, 0x00}, //Br thres0
	{0x1d8dd, 0xe0}, //Br thres0
	{0x1d8de, 0x52}, //Br thres1
	{0x1d8df, 0x08}, //Br thres1
	{0x1d8da, 0x20}, //Br Ratio0
	{0x1d8db, 0x06}, //Br Ratio1

	{0x1d949, 0x0e}, //super highlight cwf D0 //66206
	{0x1d925, 0x0e}, //highlight cwf D0 //66206
	{0x1d926, 0x0e}, //middlelight cwf D0
	{0x1d927, 0x11}, //lowlight cwf D0

	{0x1d94a, 0x0e}, //super highlight A Range DX //66207
	{0x1d928, 0x0e}, //highlight A Range DX //66207
	{0x1d929, 0x0e}, //middlelight A Range DX
	{0x1d92a, 0x20}, //lowlight A Range DX

	{0x1d94b, 0x11}, //super highlight D Range DY //66208
	{0x1d92b, 0x11}, //highlight D Range DY //66208
	{0x1d92c, 0x11}, //middlelight D Range DY
	{0x1d92d, 0x13}, //lowlight D Range DY

	{0x1d94c, 0x52}, //super highlight D limit //6620d
	{0x1d92e, 0x52}, //highlight D limit //6620d
	{0x1d92f, 0x52}, //middlelight D limit
	{0x1d930, 0x52}, //lowlight D limit

	{0x1d94d, 0x3e}, //super highlight A limit //6620e
	{0x1d931, 0x3e}, //highlight A limit //6620e
	{0x1d932, 0x3e}, //middlelight A limit
	{0x1d933, 0x38}, //lowlight A limit

	{0x1d94e, 0x6e}, //super highlight D split //6620f
	{0x1d934, 0x6e}, //highlight D split //6620f
	{0x1d935, 0x6e}, //middlelight D split
	{0x1d936, 0x6e}, //lowlight D split

	{0x1d94f, 0x56}, //super highlight A split //66210
	{0x1d937, 0x56}, //highlight A split //66210
	{0x1d938, 0x56}, //middlelight A split
	{0x1d939, 0x56}, //lowlight A split
#endif
};

static framesize_s hi542_framesizes[] = {
	{0, 0, 1280, 960, 2791, 1036 , 29, 27, 0x12C, 0xFA, 0x100, VIEW_FULL, RESOLUTION_4_3, false, {hi542_framesize_pre_30fps, ARRAY_SIZE(hi542_framesize_pre_30fps)} },
	//{0, 0, 1920, 1088, 2791,1152, 26, 14, 0x12c, 0xFA, 0x100, VIEW_FULL, RESOLUTION_4_3, false, {hi542_1080P, ARRAY_SIZE(hi542_1080P)} },
	{0, 0, 2592, 1952, 2791, 2080, 14, 14, 0x12c, 0xFA, 0x100, VIEW_FULL, RESOLUTION_4_3, false, {hi542_framesize_full_15fps, ARRAY_SIZE(hi542_framesize_full_15fps)} },
};

static camera_sensor hi542_sensor;
static void hi542_set_default(void);
#ifdef CONFIG_DEBUG_FS
extern u32 get_main_sensor_id(void);
#endif
/*
 **************************************************************************
 * FunctionName: hi542_read_reg;
 * Description : read hi542 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_read_reg(u16 reg, u8 *val)
{
	return k3_ispio_read_reg(hi542_sensor.i2c_config.index,
				 hi542_sensor.i2c_config.addr, reg, (u16 *)val, hi542_sensor.i2c_config.val_bits);
}

/*
 **************************************************************************
 * FunctionName: hi542_write_reg;
 * Description : write hi542 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_write_reg(u16 reg, u8 val, u8 mask)
{
	return k3_ispio_write_reg(hi542_sensor.i2c_config.index,
			hi542_sensor.i2c_config.addr, reg, val, hi542_sensor.i2c_config.val_bits, mask);
}

/*
 **************************************************************************
 * FunctionName: hi542_write_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int hi542_write_seq(const struct _sensor_reg_t *seq, u32 size, u8 mask)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	return k3_ispio_write_seq(hi542_sensor.i2c_config.index,
			hi542_sensor.i2c_config.addr, seq, size, hi542_sensor.i2c_config.val_bits, mask);
}

/*
 **************************************************************************
 * FunctionName: hi542_write_isp_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void hi542_write_isp_seq(const struct isp_reg_t *seq, u32 size)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	k3_ispio_write_isp_seq(seq, size);
}


/*
 **************************************************************************
 * FunctionName: hi542_enum_frame_intervals;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_enum_frame_intervals(struct v4l2_frmivalenum *fi)
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
 * FunctionName: hi542_get_format;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_get_format(struct v4l2_fmtdesc *fmt)
{
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_OVERLAY)
		fmt->pixelformat = hi542_sensor.fmt[STATE_PREVIEW];
	else
		fmt->pixelformat = hi542_sensor.fmt[STATE_CAPTURE];

	return 0;
}

/*
 **************************************************************************
 * FunctionName: hi542_get_capability;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_get_capability(u32 id, u32 *value)
{
	int i=0;
	for (i = 0; i < ARRAY_SIZE(hi542_cap); ++i) {
		if (id == hi542_cap[i].id) {
			*value = hi542_cap[i].value;
			break;
		}
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: hi542_enum_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_enum_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	u32 max_index = ARRAY_SIZE(camera_framesizes) - 1;
	u32 this_max_index = ARRAY_SIZE(hi542_framesizes) - 1;

	assert(framesizes);

	print_debug("enter %s; ", __func__);

	if (framesizes->index > max_index) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	if ((camera_framesizes[framesizes->index].width > hi542_framesizes[this_max_index].width)
		|| (camera_framesizes[framesizes->index].height > hi542_framesizes[this_max_index].height)) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	framesizes->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	framesizes->discrete.width = hi542_framesizes[this_max_index].width;
	framesizes->discrete.height = hi542_framesizes[this_max_index].height;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: hi542_try_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_try_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	int max_index = ARRAY_SIZE(hi542_framesizes) - 1;

	assert(framesizes);

	print_debug("Enter Function:%s  ", __func__);


	if ((framesizes->discrete.width <= hi542_framesizes[max_index].width)
	    && (framesizes->discrete.height <= hi542_framesizes[max_index].height)) {
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
 * FunctionName: hi542_set_framesizes;
 * Description : NA;
 * Input       : flag: if 1, set framesize to sensor,
 *					   if 0, only store framesize to camera_interface;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_set_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs, int flag, camera_setting_view_type view_type)
{
	int i = 0;
	bool match = false;

	assert(fs);

	print_info("Enter Function:%s State(%d), flag=%d, width=%d, height=%d",
		    __func__, state, flag, fs->width, fs->height);

	if (VIEW_FULL == view_type) {
		for (i = 0; i < ARRAY_SIZE(hi542_framesizes); i++) {
			if ((hi542_framesizes[i].width >= fs->width)
			    && (hi542_framesizes[i].height >= fs->height)
			    && (VIEW_FULL == hi542_framesizes[i].view_type)
			    && (camera_get_resolution_type(fs->width, fs->height)
			    <= hi542_framesizes[i].resolution_type)) {
				fs->width = hi542_framesizes[i].width;
				fs->height = hi542_framesizes[i].height;
				match = true;
				break;
			}
		}
	}

	if (false == match) {
		for (i = 0; i < ARRAY_SIZE(hi542_framesizes); i++) {
			if ((hi542_framesizes[i].width >= fs->width)
			    && (hi542_framesizes[i].height >= fs->height)
			    && (camera_get_resolution_type(fs->width, fs->height)
			    <= hi542_framesizes[i].resolution_type)) {
				fs->width = hi542_framesizes[i].width;
				fs->height = hi542_framesizes[i].height;
				break;
			}
		}
	}

	if (i >= ARRAY_SIZE(hi542_framesizes)) {
		print_error("request resolution larger than sensor's max resolution");
		return -EINVAL;
	}

	if (state == STATE_PREVIEW)
		hi542_sensor.preview_frmsize_index = i;
	else
		hi542_sensor.capture_frmsize_index = i;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: hi542_get_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_get_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs)
{
	int frmsize_index;

	assert(fs);

	if (state == STATE_PREVIEW)
		frmsize_index = hi542_sensor.preview_frmsize_index;
	else if (state == STATE_CAPTURE)
		frmsize_index = hi542_sensor.capture_frmsize_index;
	else
		return -EINVAL;

	fs->width = hi542_framesizes[frmsize_index].width;
	fs->height = hi542_framesizes[frmsize_index].height;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: hi542_init_reg;
 * Description : download initial seq for sensor init;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_init_reg(void)
{
	int size = 0;

	print_debug("Enter Function:%s  , initsize=%d",
		    __func__, sizeof(hi542_init_regs));

	size = ARRAY_SIZE(isp_init_regs_hi542);
	hi542_write_isp_seq(isp_init_regs_hi542, size);

	if (0 != k3_ispio_init_csi(hi542_sensor.mipi_index,
			hi542_sensor.mipi_lane_count, hi542_sensor.lane_clk)) {
		return -EFAULT;
	}

	size = ARRAY_SIZE(hi542_init_regs);
	if (0 != hi542_write_seq(hi542_init_regs, size, 0x00)) {
		print_error("line %d, fail to init hi542 sensor", __LINE__);
		return -EFAULT;
	}

	return 0;
}

static int hi542_set_hflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);
	hi542_sensor.hflip = flip;
	return 0;
}
static int hi542_get_hflip(void)
{
	print_debug("enter %s", __func__);

	return hi542_sensor.hflip;
}
static int hi542_set_vflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);

	hi542_sensor.vflip = flip;

	return 0;
}
static int hi542_get_vflip(void)
{
	print_debug("enter %s", __func__);
	return hi542_sensor.vflip;
}

static int hi542_update_flip(u16 width, u16 height)
{
	u8 new_flip = ((hi542_sensor.vflip << 1) | hi542_sensor.hflip);
	print_debug("Enter %s  ", __func__);
	if (hi542_sensor.old_flip != new_flip) {
		k3_ispio_update_flip((hi542_sensor.old_flip ^ new_flip) & 0x03, width, height, HI542_PIXEL_ORDER_CHANGED);

		hi542_sensor.old_flip = new_flip;
		hi542_write_reg(HI542_FLIP, hi542_sensor.vflip ? 0x02 : 0x00, ~0x02);
		hi542_write_reg(HI542_FLIP, hi542_sensor.hflip ? 0x01 : 0x00, ~0x01);
		msleep(200);
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: hi542_framesize_switch;
 * Description : switch frame size, used by preview and capture
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_framesize_switch(camera_state state)
{
	u8 next_frmsize_index;

	if (state == STATE_PREVIEW)
		next_frmsize_index = hi542_sensor.preview_frmsize_index;
	else
		next_frmsize_index = hi542_sensor.capture_frmsize_index;

	print_debug("Enter Function:%s frm index=%d", __func__, next_frmsize_index);

	if (next_frmsize_index >= ARRAY_SIZE(hi542_framesizes)){
		print_error("Unsupport sensor setting index: %d",next_frmsize_index);
		return -ETIME;
	}

	if (0 != hi542_write_seq(hi542_sensor.frmsize_list[next_frmsize_index].sensor_setting.setting
		,hi542_sensor.frmsize_list[next_frmsize_index].sensor_setting.seq_size, 0x00)) {
		print_error("fail to init hi542 sensor");
		return -ETIME;
	}

	return 0;
}


/*
 **************************************************************************
 * FunctionName: hi542_stream_on;
 * Description : download preview seq for sensor preview;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_stream_on(camera_state state)
{
	print_debug("Enter Function:%s ", __func__);
	return hi542_framesize_switch(state);
}


/*  **************************************************************************
* FunctionName: hi542_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static int hi542_check_sensor(void)
{
	u8 idl = 0x1;
	u8 idh = 0x1;
	u16 id = 0;

	int ret = -1;
	u8 val = 1;

	//ret = hi542_read_reg(0x0000, &idh);
	//ret = hi542_read_reg(0x0001, &idl);
#if 0
	ret = hi542_read_reg(0x0002, &val);
	print_info("%s: read 0x0002, ret=%d, val=%d.", __func__, ret, val);

	ret = hi542_read_reg(0x0004, &val);
	print_info("%s: read 0x0004, ret=%d, val=%d.", __func__, ret, val);

	hi542_write_reg(HI542_FLIP, 0x3,  0);
	ret = hi542_read_reg(HI542_FLIP, &val);
	print_info("%s: read 0x0101, ret=%d, val=%d.", __func__, ret, val);
#endif

	ret = hi542_read_reg(0x0004, &idl);
	//id = ((idh << 8) | idl);
	id = idl;
	print_info("hi542 product id:0x%x", id);
#ifdef CONFIG_DEBUG_FS
	if (HI542_CHIP_ID != id) {
		id = (u16)get_main_sensor_id();
		print_info("hi542 debugfs product id:0x%x", id);
	}
#endif
	if (HI542_CHIP_ID != id) {
		print_error("Invalid product id ,Could not load sensor hi542");
		return -ENODEV;
	}

	return 0;
}

/****************************************************************************
* FunctionName: hi542_power;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
int hi542_power(camera_power_state power)
{
	int ret = 0;

	if (power == POWER_ON) {
		k3_ispldo_power_sensor(power, "pri-cameralog-vcc");
		ret = camera_power_core_ldo(power);
		udelay(200);
		k3_ispldo_power_sensor(power, "camera-vcc");
		ret = k3_ispio_ioconfig(&hi542_sensor, power);
		k3_ispgpio_reset_sensor(hi542_sensor.sensor_index, power,
					hi542_sensor.power_conf.reset_valid);
		udelay(1);
		k3_ispldo_power_sensor(power, "sec-cameralog-vcc");
		udelay(20);
		ret = k3_ispgpio_power_sensor(&hi542_sensor, power);
		k3_isp_io_enable_mclk(MCLK_ENABLE, hi542_sensor.sensor_index);
		udelay(500);
	} else {
		k3_ispio_deinit_csi(hi542_sensor.mipi_index);
		k3_isp_io_enable_mclk(MCLK_DISABLE, hi542_sensor.sensor_index);
		ret = k3_ispio_ioconfig(&hi542_sensor, power);
		ret = k3_ispgpio_power_sensor(&hi542_sensor, power);
		k3_ispldo_power_sensor(power, "sec-cameralog-vcc");
		camera_power_core_ldo(power);
		udelay(200);
		k3_ispldo_power_sensor(power, "pri-cameralog-vcc");
		k3_ispgpio_reset_sensor(hi542_sensor.sensor_index, power,
					hi542_sensor.power_conf.reset_valid);
		k3_ispldo_power_sensor(power, "camera-vcc");

	}
	return ret;
}

void hi542_set_gain(u32 gain)
{
	print_debug("enter %s, gain=%u", __func__, gain>>4);
	u8    digital_h = 0;
	u8    digital_l = 0;
	u32  analog_gain = 0;

	u32  gain_left_part = 0;
	u32  gain_right_part = 0;
	if (gain == 0)
		return;

	//if(gain > 0xac)// the limit of hynix gain is 0xac
	//	gain = 0xac;

	if(gain>HI542_MAX_ANALOG_GAIN*Q16)
	{
		gain_left_part  = gain / 0x80;

		gain_right_part = gain % 0x80;


		digital_h = (gain_left_part >> 1) & 0x3;

		digital_l = (gain_right_part )&0x7F | ((gain_left_part &0x1) << 7);

		analog_gain = HI542_MAX_ANALOG_GAIN*Q16;
	}
	else
	{
		analog_gain = gain;
		digital_h = 0;
		digital_l = 0x80;
	}

	if (analog_gain > 0x80) {
		analog_gain = 0x00;
	}else {
		analog_gain *= 1000;
		analog_gain = 256 * 1000 / ((analog_gain  >> 4) & 0xffff) - 32;
	}

	hi542_write_reg(HI542_GAIN_REG, analog_gain & 0xff, 0x00);
	hi542_write_reg(HI542_DIGTAL_GAIN_H, digital_h & 0xff, 0x00);
	hi542_write_reg(HI542_DIGTAL_GAIN_L, digital_l & 0xff, 0x00);

	//print_info("analog_gain = %d,digital_h=%x,digital_l=%x",analog_gain,digital_h,digital_l);
	//hi542_write_reg(0x0578, ~analog_gain& 0xff, 0x00);

}

u32 hi542_get_exposure( void )
{
	u8 regh,regm1,regm2,regl;
	u32 expo_time;

	hi542_read_reg(0x0111, &regh);
	hi542_read_reg(0x0112, &regm1);
	hi542_read_reg(0x0113, &regm2);
	hi542_read_reg(0x0114, &regl);

	expo_time = ((regh<<24) | (regm1<<16) | (regm2<<8) | regl);

	return (expo_time+1)/2791 ;
}
void hi542_set_exposure(u32 exposure)
{
	u32 fixed_expo = 0;
	u32 hi542_expo = 0;

	exposure >>= 4;
	print_debug("enter %s, exposure=%u", __func__, exposure);

	hi542_expo = exposure * 2791;
	hi542_write_reg(HI542_EXPOSURE_REG_H, (hi542_expo >> 24) & 0xff, 0x00);
	hi542_write_reg(HI542_EXPOSURE_REG_M1, (hi542_expo >> 16) & 0xff, 0x00);
	hi542_write_reg(HI542_EXPOSURE_REG_M2, (hi542_expo >> 8) & 0xff, 0x00);
	hi542_write_reg(HI542_EXPOSURE_REG_L, hi542_expo & 0xff, 0x00);
}

static sensor_reg_t* hi542_construct_vts_reg_buf(u16 vts, u16 *psize)
{
	static sensor_reg_t hi542_vts_regs[] = {
		{0x0123, 0x00},
		{0x0122, 0x00},
		{0x0121, 0x00},
		{0x0120, 0x00},
		{0x011f, 0x00},
		{0x011e, 0x00},
		{0x011d, 0x00},
		{0x011c, 0x00},
	};
	u32 max_exposure = 0;
	u32 fixed_expo = 0;

	print_debug("Enter %s,vts=%u", __func__, vts);
	max_exposure = (vts -2) * 2791;
	hi542_vts_regs[0].value = max_exposure & 0xff ;
	hi542_vts_regs[1].value =  (max_exposure >> 8) & 0xff;
	hi542_vts_regs[2].value = (max_exposure >> 16) & 0xff;
	hi542_vts_regs[3].value = (max_exposure >> 24) & 0xff;

	fixed_expo = vts * 2791;
	hi542_vts_regs[4].value = fixed_expo & 0xff ;
	hi542_vts_regs[5].value = (fixed_expo >> 8) & 0xff;
	hi542_vts_regs[6].value = (fixed_expo >> 16) & 0xff;
	hi542_vts_regs[7].value = (fixed_expo >> 24) & 0xff;
	*psize = ARRAY_SIZE(hi542_vts_regs);
	return hi542_vts_regs;
}

void hi542_set_vts(u16 vts)
{
	u16 vblank;
	u32 max_exposure = 0;
	u32 fixed_expo = 0;

	max_exposure = (vts -2) * 2791;
	hi542_write_reg(0x123, max_exposure & 0xff, 0x00);
	hi542_write_reg(0x122, (max_exposure >> 8) & 0xff, 0x00);
	hi542_write_reg(0x121, (max_exposure >> 16) & 0xff, 0x00);
	hi542_write_reg(0x120, (max_exposure >> 24) & 0xff, 0x00);

	fixed_expo = vts * 2791;
	hi542_write_reg(0x011f, fixed_expo & 0xff, 0x00);
	hi542_write_reg(0x011e, (fixed_expo >> 8) & 0xff, 0x00);
	hi542_write_reg(0x011d, (fixed_expo >> 16) & 0xff, 0x00);
	hi542_write_reg(0x011c, (fixed_expo >> 24) & 0xff, 0x00);
}



static u32 hi542_get_override_param(camera_override_type_t type)
{
	u32 ret_val = sensor_override_params[type];

	switch (type) {
	case OVERRIDE_ISO_HIGH:
		ret_val = HI542_MAX_ISO;
		break;

	case OVERRIDE_ISO_LOW:
		ret_val = HI542_MIN_ISO;
		break;
	case OVERRIDE_FPS_MAX:
		ret_val = HI542_MAX_FRAMERATE;
		break;
	case OVERRIDE_FPS_MIN:
		ret_val = HI542_MIN_FRAMERATE;
		break;
	case OVERRIDE_FPS_MIDDLE:
		ret_val = HI542_MIDDLE_FRAMERATE;
		break;
	case OVERRIDE_CAP_FPS_MIN:
		ret_val = HI542_MIN_CAP_FRAMERATE;
		break;
	case OVERRIDE_FLASH_TRIGGER_GAIN:
		ret_val = HI542_FLASH_TRIGGER_GAIN;
		break;
	case OVERRIDE_AUTOFPS_GAIN_LOW2MID:
		ret_val = HI542_AUTOFPS_GAIN_LOW2MID;
		break;
	case OVERRIDE_AUTOFPS_GAIN_MID2HIGH:
		ret_val = HI542_AUTOFPS_GAIN_MID2HIGH;
		break;

	/* reduce frame rate gain threshold */
	case OVERRIDE_AUTOFPS_GAIN_MID2LOW:
		ret_val = HI542_AUTOFPS_GAIN_MID2LOW;
		break;
	case OVERRIDE_AUTOFPS_GAIN_HIGH2MID:
		ret_val = HI542_AUTOFPS_GAIN_HIGH2MID;
		break;

	case OVERRIDE_SHARPNESS_PREVIEW:
		ret_val = HI542_SHARPNESS_PREVIEW;
		break;
	case OVERRIDE_SHARPNESS_CAPTURE:
		ret_val = HI542_SHARPNESS_CAPTURE;
		break;

	default:
		//print_error("%s:not override or invalid type %d, use default",__func__, type);
		break;
	}

	return ret_val;
}

static int hi542_fill_denoise_buf(u8 *ybuf, u16 *uvbuf, u8 size, camera_state state, bool flash_on)
{
	u32 ae_th[3];
	u32 ae_value = get_writeback_gain() * get_writeback_expo() / 0x10;
	int index = hi542_sensor.preview_frmsize_index;

	if( (ybuf==NULL)||(uvbuf==NULL)||(size <7 ) ){
		print_error("%s invalid arguments",__func__);
		return -1;
	}

	ybuf[0] = HI542_ISP_YDENOISE_COFF_1X;
	ybuf[1] = HI542_ISP_YDENOISE_COFF_2X;
	ybuf[2] = HI542_ISP_YDENOISE_COFF_4X;
	ybuf[3] = HI542_ISP_YDENOISE_COFF_8X;
	ybuf[4] = HI542_ISP_YDENOISE_COFF_16X;
	ybuf[5] = HI542_ISP_YDENOISE_COFF_32X;
	ybuf[6] = HI542_ISP_YDENOISE_COFF_64X;

	uvbuf[0] = HI542_ISP_UVDENOISE_COFF_1X;
	uvbuf[1] = HI542_ISP_UVDENOISE_COFF_2X;
	uvbuf[2] = HI542_ISP_UVDENOISE_COFF_4X;
	uvbuf[3] = HI542_ISP_UVDENOISE_COFF_8X;
	uvbuf[4] = HI542_ISP_UVDENOISE_COFF_16X;
	uvbuf[5] = HI542_ISP_UVDENOISE_COFF_32X;
	uvbuf[6] = HI542_ISP_UVDENOISE_COFF_64X;

	if (flash_on == false) {
		ae_th[0] = hi542_sensor.frmsize_list[index].banding_step_50hz * 0x18;
		ae_th[1] = 3 * hi542_sensor.frmsize_list[index].banding_step_50hz * 0x10;
		ae_th[2] = hi542_sensor.frmsize_list[index].vts * 0x20;

		if (hi542_sensor.frmsize_list[index].binning == false) {
			ae_th[0] *= 2;
			ae_th[1] *= 2;
			ae_th[2] *= 2;
		}

		/* simplify dynamic denoise threshold*/
		if (ae_value <= ae_th[0]) {
			ybuf[0] = HI542_ISP_YDENOISE_COFF_1X;
			ybuf[1] = HI542_ISP_YDENOISE_COFF_2X;
		} else {
			ybuf[0] = HI542_ISP_YDENOISE_COFF_2X;
			ybuf[1] = HI542_ISP_YDENOISE_COFF_2X;
		}
	} else {
		/* should calculated in capture_cmd again. */
		ybuf[0] = HI542_ISP_YDENOISE_COFF_1X;
		ybuf[1] = HI542_ISP_YDENOISE_COFF_2X;
	}

	return 0;
}
/*
 **************************************************************************
 * FunctionName: hi542_reset;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/

static int hi542_reset(camera_power_state power_state)
{
	print_debug("%s  ", __func__);
/*
	if (POWER_ON == power_state) {
		k3_isp_io_enable_mclk(MCLK_ENABLE, hi542_sensor.sensor_index);
		k3_ispgpio_reset_sensor(hi542_sensor.sensor_index, power_state,
			      hi542_sensor.power_conf.reset_valid);
		udelay(500);
	} else {
		k3_ispgpio_reset_sensor(hi542_sensor.sensor_index, power_state,
			      hi542_sensor.power_conf.reset_valid);
		udelay(10);
		k3_isp_io_enable_mclk(MCLK_DISABLE, hi542_sensor.sensor_index);
	}
*/
	return 0;
}

/*
 **************************************************************************
 * FunctionName: hi542_init;
 * Description : hi542 init function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : Error code indicating success or failure;
 * Other       : NA;
 **************************************************************************
*/
static int hi542_init(void)
{
	static bool hi542_check = false;
	print_debug("%s  ", __func__);
/*
	if (false == hi542_check)
	{
		if (check_suspensory_camera("HI542") != 1)
		{
			return -ENODEV;
		}
		hi542_check = true;
	}
*/
#if 0
	if (!camera_timing_is_match(0)) {
		print_error("%s: sensor timing don't match.\n", __func__);
		return -ENODEV;
	}
#endif

	if (hi542_sensor.owner && !try_module_get(hi542_sensor.owner)) {
		print_error("%s: try_module_get fail", __func__);
		return -ENOENT;
	}

	k3_ispio_power_init("pri-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V*/
	k3_ispio_power_init("camera-vcc", LDO_VOLTAGE_18V, LDO_VOLTAGE_18V);	/*IO 1.8V*/
	//k3_ispio_power_init("cameravcm-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*AF 2.85V*/
	k3_ispio_power_init("sec-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V*/

	return 0;
}

/*
 **************************************************************************
 * FunctionName: hi542_exit;
 * Description : hi542 exit function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void hi542_exit(void)
{
	print_debug("enter %s", __func__);

	k3_ispio_power_deinit();

	if (hi542_sensor.owner)
		module_put(hi542_sensor.owner);

	hi542_sensor.skip_frames_first_enter = 4;
	print_debug("exit %s", __func__);
}

/*
 **************************************************************************
 * FunctionName: hi542_shut_down;
 * Description : hi542 shut down function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void hi542_shut_down(void)
{
	print_debug("enter %s", __func__);
	k3_ispgpio_power_sensor(&hi542_sensor, POWER_OFF);
}

static u32 hi542_get_sensor_gain_range(camera_gain_range_type_t param)
{
	print_debug("enter %s", __func__);
	u32 ret_val = 0;
	u8 frame_index = -1;
	switch (param) {
	case AE_PREVIEW_GAIN_MAX:
		ret_val = HI542_MAX_ISO* 0x10 / 100;
		break;
	case AE_PREVIEW_GAIN_MIN:
		ret_val = HI542_MIN_ISO* 0x10 / 100;
		break;
	case AE_CAPTURE_GAIN_MAX:
		ret_val = HI542_MAX_ISO* 0x10 / 100;

		break;
	case AE_CAPTURE_GAIN_MIN:
		ret_val = HI542_MIN_ISO* 0x10 / 100;
		break;
	default:
		print_error("%s:not override or invalid type %d, use default,ret_val=%d",__func__, param,ret_val);
		break;
	}
	return ret_val;
}


void hi542_set_lsc_shading(camera_iso iso)
{
	switch (iso) {
	case CAMERA_ISO_AUTO:
		hi542_write_reg(0x0229, 0x2d & 0xff, 0x00);

		hi542_write_reg(0x00B4, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x00B5, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x00B6, 0x02 & 0xff, 0x00);
		hi542_write_reg(0x00B7, 0x01 & 0xff, 0x00);
		hi542_write_reg(0x0554, 0x52 & 0xff, 0x00);
		hi542_write_reg(0x0555, 0x52 & 0xff, 0x00);
		hi542_write_reg(0x0556, 0x52 & 0xff, 0x00);

		hi542_write_reg(0x0571, 0x60 & 0xff, 0x00);
		hi542_write_reg(0x0500, 0x1b & 0xff, 0x00);
		break;
	case CAMERA_ISO_100:
		hi542_write_reg(0x0229, 0x01 & 0xff, 0x00);

		hi542_write_reg(0x00B4, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x00B5, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x00B6, 0x01 & 0xff, 0x00);
		hi542_write_reg(0x00B7, 0x01 & 0xff, 0x00);
		hi542_write_reg(0x0554, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x0555, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x0556, 0x00 & 0xff, 0x00);

		hi542_write_reg(0x0571, 0x60 & 0xff, 0x00);
		hi542_write_reg(0x0500, 0x1b & 0xff, 0x00);
		break;

	case CAMERA_ISO_200:
		hi542_write_reg(0x0229, 0x2d & 0xff, 0x00);

		hi542_write_reg(0x00B4, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x00B5, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x00B6, 0x01 & 0xff, 0x00);
		hi542_write_reg(0x00B7, 0x01 & 0xff, 0x00);
		hi542_write_reg(0x0554, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x0555, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x0556, 0x00 & 0xff, 0x00);

		hi542_write_reg(0x0571, 0x60 & 0xff, 0x00);
		hi542_write_reg(0x0500, 0x1b & 0xff, 0x00);
		break;
	case CAMERA_ISO_400:
		hi542_write_reg(0x0229, 0x2d & 0xff, 0x00);

		hi542_write_reg(0x00B4, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x00B5, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x00B6, 0x01 & 0xff, 0x00);
		hi542_write_reg(0x00B7, 0x01 & 0xff, 0x00);
		hi542_write_reg(0x0554, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x0555, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x0556, 0x00 & 0xff, 0x00);

		hi542_write_reg(0x0571, 0x60 & 0xff, 0x00);
		hi542_write_reg(0x0500, 0x1b & 0xff, 0x00);
		break;
	case CAMERA_ISO_800:
		hi542_write_reg(0x0229, 0x2d & 0xff, 0x00);

		hi542_write_reg(0x00B4, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x00B5, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x00B6, 0x02 & 0xff, 0x00);
		hi542_write_reg(0x00B7, 0x01 & 0xff, 0x00);
		hi542_write_reg(0x0554, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x0555, 0x00 & 0xff, 0x00);
		hi542_write_reg(0x0556, 0x00 & 0xff, 0x00);

		hi542_write_reg(0x0571, 0x60 & 0xff, 0x00);
		hi542_write_reg(0x0500, 0x1b & 0xff, 0x00);
		break;
	default:
		break;
	}

}
/*
 **************************************************************************
 * FunctionName: s5k4e1ga_get_support_vts;
 * Description : the function that provide the real vts value on the different
 fps;
 * Input       : new_fps the fps to be change;
 *             : old fps and old vts
 * Output      : the vts for new fps;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
u32 hi542_get_support_vts(int new_fps,int old_fps,int old_vts)
{
	u32 new_vts = 0;

	if(new_fps == 0)
	{
		return 0;
	}
	new_vts = old_fps * old_vts/new_fps;
	if(new_fps == 10)
	{
		new_vts = new_vts -4;
	}
	else if(new_fps == 8)
	{
		new_vts = 3750;
	}
       else if(new_fps == 30)
       {
		new_vts = new_vts - 14;
	}

	print_info("%s,new_vts = %d,new_fps = %d,old_fps = %d,old_vts=%d",__func__,new_vts,new_fps,old_fps,old_vts);
	return new_vts;
}
/*
 **************************************************************************
 * FunctionName: hi542_check_video_fps;
 * Description : checking support video change or not;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
int hi542_check_video_fps()
{
	return VIDEO_FPS_CHANGE;
}
/*
 **************************************************************************
 * FunctionName: hi542_set_default;
 * Description : init hi542_sensor;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void hi542_set_default(void)
{
	unsigned int chip_id;
	int i;

	hi542_sensor.init = hi542_init;
	hi542_sensor.exit = hi542_exit;
	hi542_sensor.shut_down = hi542_shut_down;
	hi542_sensor.reset = hi542_reset;
	hi542_sensor.check_sensor = hi542_check_sensor;
	hi542_sensor.power = hi542_power;
	hi542_sensor.init_reg = hi542_init_reg;
	hi542_sensor.stream_on = hi542_stream_on;

	hi542_sensor.get_format = hi542_get_format;
	hi542_sensor.set_flash = NULL;
	hi542_sensor.get_flash = NULL;
	hi542_sensor.set_scene = NULL;
	hi542_sensor.get_scene = NULL;

	hi542_sensor.enum_framesizes = hi542_enum_framesizes;
	hi542_sensor.try_framesizes = hi542_try_framesizes;
	hi542_sensor.set_framesizes = hi542_set_framesizes;
	hi542_sensor.get_framesizes = hi542_get_framesizes;

	hi542_sensor.enum_frame_intervals = hi542_enum_frame_intervals;
	hi542_sensor.try_frame_intervals = NULL;
	hi542_sensor.set_frame_intervals = NULL;
	hi542_sensor.get_frame_intervals = NULL;

	hi542_sensor.get_capability = hi542_get_capability;

	hi542_sensor.set_hflip = hi542_set_hflip;
	hi542_sensor.get_hflip = hi542_get_hflip;
	hi542_sensor.set_vflip = hi542_set_vflip;
	hi542_sensor.get_vflip = hi542_get_vflip;
	hi542_sensor.update_flip = hi542_update_flip;

	strcpy(hi542_sensor.info.name, "hi542_sunny");
	hi542_sensor.interface_type = MIPI2;
	hi542_sensor.mipi_lane_count = CSI_LINES_2;
	hi542_sensor.mipi_index = CSI_INDEX_1;
	hi542_sensor.sensor_index = CAMERA_SENSOR_SECONDARY;
	hi542_sensor.skip_frames = 0;
	hi542_sensor.skip_frames_first_enter = 4;
	hi542_sensor.power_conf.pd_valid = LOW_VALID;
	hi542_sensor.power_conf.reset_valid = LOW_VALID;
	hi542_sensor.power_conf.vcmpd_valid = LOW_VALID;

	hi542_sensor.i2c_config.index = I2C_PRIMARY;
	hi542_sensor.i2c_config.speed = I2C_SPEED_400;
	hi542_sensor.i2c_config.addr = HI542_SLAVE_ADDRESS;
	hi542_sensor.i2c_config.addr_bits = 16;
	hi542_sensor.i2c_config.val_bits = I2C_8BIT;

	hi542_sensor.preview_frmsize_index = 0;
	hi542_sensor.capture_frmsize_index = 0;
	hi542_sensor.frmsize_list = hi542_framesizes;
	hi542_sensor.fmt[STATE_PREVIEW] = V4L2_PIX_FMT_RAW10;
	hi542_sensor.fmt[STATE_CAPTURE] = V4L2_PIX_FMT_RAW10;
#ifdef HI542_FOXCONN_AP_WRITEAE_MODE
	hi542_sensor.aec_addr[0] = 0; /*high byte*/
	hi542_sensor.aec_addr[1] = 0;
	hi542_sensor.aec_addr[2] = 0;
	hi542_sensor.agc_addr[0] = 0; /*high byte*/
	hi542_sensor.agc_addr[1] = 0;
	hi542_sensor.ap_writeAE_delay = 6000;
#else
	hi542_sensor.aec_addr[0] = 0x0111; /*high byte*/
	hi542_sensor.aec_addr[1] = 0x0112;
	hi542_sensor.aec_addr[2] = 0x0113;
	hi542_sensor.agc_addr[0] = 0x0589; /*high byte*/
	hi542_sensor.agc_addr[1] = 0x058A;
#endif

	hi542_sensor.sensor_type = 0;
	hi542_sensor.construct_vts_reg_buf = hi542_construct_vts_reg_buf;
	hi542_sensor.set_gain = hi542_set_gain;
	hi542_sensor.set_exposure = hi542_set_exposure;
	hi542_sensor.get_exposure = hi542_get_exposure;
	hi542_sensor.set_vts = hi542_set_vts;
	hi542_sensor.get_sensor_gain_range = hi542_get_sensor_gain_range;

	hi542_sensor.get_override_param = hi542_get_override_param;
	hi542_sensor.fill_denoise_buf = hi542_fill_denoise_buf;
	hi542_sensor.sensor_rgb_type = SENSOR_BGGR;
	hi542_sensor.sensor_gain_to_iso = NULL;
	hi542_sensor.sensor_iso_to_gain = NULL;

	hi542_sensor.set_effect = NULL;

	hi542_sensor.isp_location = CAMERA_USE_K3ISP;
	hi542_sensor.sensor_tune_ops = NULL;

	hi542_sensor.af_enable = 0;

	hi542_sensor.image_setting.lensc_param =NULL;
	hi542_sensor.image_setting.ccm_param =NULL;
	hi542_sensor.image_setting.awb_param = NULL;

	hi542_sensor.fps_max = 27;
	hi542_sensor.fps_min = 13;
	hi542_sensor.fps = 25;

	hi542_sensor.owner = THIS_MODULE;

	hi542_sensor.info.facing = CAMERA_FACING_BACK;
	hi542_sensor.info.orientation = 90;   /*270;*/
	hi542_sensor.info.focal_length = 439; /* 4.39mm*/
	hi542_sensor.info.h_view_angle = 66; /*  66.1 degree */
	hi542_sensor.info.v_view_angle = 50;
	hi542_sensor.lane_clk = CLK_400M;
	hi542_sensor.hflip = 0;
	hi542_sensor.vflip = 0;
	hi542_sensor.old_flip = 0;
	hi542_sensor.lcd_compensation_supported = 1;
	hi542_sensor.support_binning_type	= BINNING_AVERAGE;
	hi542_sensor.hynix_set_lsc_shading = hi542_set_lsc_shading;
	hi542_sensor.get_support_vts = hi542_get_support_vts;
	hi542_sensor.check_video_fps = hi542_check_video_fps;
}

/*
 **************************************************************************
 * FunctionName: hi542_module_init;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static __init int hi542_module_init(void)
{
	hi542_set_default();
	return register_camera_sensor(hi542_sensor.sensor_index, &hi542_sensor);
}

/*
 **************************************************************************
 * FunctionName: hi542_module_exit;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void __exit hi542_module_exit(void)
{
	unregister_camera_sensor(hi542_sensor.sensor_index, &hi542_sensor);
}

MODULE_AUTHOR("Hisilicon");
module_init(hi542_module_init);
module_exit(hi542_module_exit);
MODULE_LICENSE("GPL");

/********************************** END **********************************************/
