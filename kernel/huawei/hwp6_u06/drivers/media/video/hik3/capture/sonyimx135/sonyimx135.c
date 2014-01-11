/*
 *  sonyimx135 camera driver source file
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
#include "sonyimx135.h"
/*#include "../isp/k3_isp_io.h"*/
#include <asm/bug.h>
#include <linux/device.h>

#define LOG_TAG "SONYIMX135"
/* #define DEBUG_DEBUG 1 */
#include "../isp/cam_log.h"
#include <../isp/cam_util.h>
#include <linux/workqueue.h>
// modiyf format
#include "../isp/k3_ispv1_hdr_movie_ae.h"
#include <hsad/config_interface.h>
#include "../isp/k3_isp.h"
#include "../isp/k3_ispv1.h"
#include "../isp/k3_ispv1_afae.h"

/*add for set awb gain begin*/
#define DIG_GAIN_GR_H                        0x020E
#define DIG_GAIN_GR_L                        0x020F
#define DIG_GAIN_R_H                          0x0210
#define DIG_GAIN_R_L                           0x0211
#define DIG_GAIN_B_H                          0x0212
#define DIG_GAIN_B_L                           0x0213
#define DIG_GAIN_GB_H                        0x0214
#define DIG_GAIN_GB_L                         0x0215

#define HDR_MODE_ADRESS                 								(0x0238)
#define WD_INTEG_RATIO                        							(0x0239)
#define SONYIMX135_DIGITAL_GAIN_REG_GREEN_R_H 				(0x020E)
#define SONYIMX135_DIGITAL_GAIN_REG_GREEN_R_L 				(0x020F)
#define SONYIMX135_DIGITAL_GAIN_REG_GREEN_B_H 				(0x0214)
#define SONYIMX135_DIGITAL_GAIN_REG_GREEN_B_L 				(0x0215)
#define SONYIMX135_DIGITAL_GAIN_REG_RED_H         				(0x0210)
#define SONYIMX135_DIGITAL_GAIN_REG_RED_L     					(0x0211)
#define SONYIMX135_DIGITAL_GAIN_REG_BLUE_H    				(0x0212)
#define SONYIMX135_DIGITAL_GAIN_REG_BLUE_L    				(0x0213)

#define WD_COARSE_INTEG_TIME_DS_H     							(0x0230)
#define WD_COARSE_INTEG_TIME_DS_L     							(0x0231)
#define COARSE_INTEG_TIME_WHT_DIRECT_H    					(0x0240)
#define COARSE_INTEG_TIME_WHT_DIRECT_L    					(0x0241)
#define COARSE_INTEG_TIME_WHT_DS_DIRECT_H 					(0x023E)
#define COARSE_INTEG_TIME_WHT_DS_DIRECT_L 					(0x023F)
#define WD_ANA_GAIN_DS                    			    					(0x0233)

#define ATR_OFF_SETTING_1             								(0x446D)
#define ATR_OFF_SETTING_2             								(0x446E)
#define TRIGGER_SETTING                  								(0x446C)

#define  WB_LMT_REG_H                             							(0x441E)
#define  WB_LMT_REG_L                             							(0x441F)

#define  AE_SAT_REG_H                              							(0x4446)
#define  AE_SAT_REG_L                              							(0x4447)

/*add for set awb gain begin*/
#define SONYIMX135_ABS_GAIN_B_H								(0x0716)
#define SONYIMX135_ABS_GAIN_B_L								(0x0717)
#define SONYIMX135_ABS_GAIN_GB_H								(0x0718)
#define SONYIMX135_ABS_GAIN_GB_L								(0x0719)
#define SONYIMX135_ABS_GAIN_GR_H								(0x0712)
#define SONYIMX135_ABS_GAIN_GR_L								(0x0713)
#define SONYIMX135_ABS_GAIN_R_H								(0x0714)
#define SONYIMX135_ABS_GAIN_R_L								(0x0715)

#define ATR_OUT_NOISE_REG_H        								(0x4452)
#define ATR_OUT_NOISE_REG_L        								(0x4453)
#define ATR_OUT_MID_REG_H        									(0x4454)
#define ATR_OUT_MID_REG_L        									(0x4455)
#define ATR_OUT_SAT_REG_H        									(0x4456)
#define ATR_OUT_SAT_REG_L        									(0x4457)
#define ATR_NOISE_BRATE_REG     									(0x4458)
#define ATR_MID_BRATE_REG        									(0x4459)
#define ATR_SAT_BRATE_REG        									(0x445a)

#define CURVE_CHANGE_REG                  							(0x441C)

#define GROUP_HOLD_FUNCTION_REG   							(0x0104)

#define SONYIMX135_APERTURE_FACTOR		220 // F2.2
#define SONYIMX135_EQUIVALENT_FOCUS		28  // 28.5mm

#define SONYIMX135_SLAVE_ADDRESS1 0x20
#define SONYIMX135_SLAVE_ADDRESS2 0x34

#define SONYIMX135_CHIP_ID       (0x0087)

#define SONYIMX135_CAM_MODULE_SKIPFRAME     4

#define SONYIMX135_FLIP		0x0101

#define SONYIMX135_EXPOSURE_REG_H	0x0202
#define SONYIMX135_EXPOSURE_REG_L	0x0203
#define SONYIMX135_GAIN_REG_H		0x0204
#define SONYIMX135_GAIN_REG_L		0x0205

#define SONYIMX135_VTS_REG_H		0x0340
#define SONYIMX135_VTS_REG_L		0x0341

#define SONYIMX135_CHIP_ID_ES1_ADRESS_H       (0x0000)
#define SONYIMX135_CHIP_ID_ES1_ADRESS_L       (0x0001)
#define SONYIMX135_CHIP_ID_ES1                         (0x0087)

#define SONYIMX135_CHIP_ID_ES2_ADRESS_H       (0x0016)
#define SONYIMX135_CHIP_ID_ES2_ADRESS_L       (0x0017)
#define SONYIMX135_CHIP_ID_ES2                         (0x0135)

#define SONYIMX135_OTP_MODE_ENABLE_REG		0x3B00      
#define SONYIMX135_OTP_STATUS_REG			0x3B01
#define SONYIMX135_OTP_PAGE_SELECT_REG		0x3B02
#define SONYIMX135_OTP_CFA_FMT_REG			0x3B2C

#define SONYIMX135_SENSOR_LSC_MODE			0x0700
#define SONYIMX135_SENSOR_LSC_EN			0x4500
#define SONYIMX135_SENSOR_RAM_SEL			0x3A63

#define SONYIMX135_SENSOR_LSC_RAM			0x4800

/* OTP lens shading parameters are 9*7*4 in toal, each parameter takes 2 bytes. */
#define SONYIMX135_OTP_LSC_SIZE				504

#define SONYIMX135_OTP_LSC_FILE				"/data/lsc_param"

#define SONYIMX135_OTP_ID_WB_READ			(1 << 0)
#define SONYIMX135_OTP_VCM_READ				(1 << 1)
#define SONYIMX135_OTP_LSC_READ				(1 << 2)
#define SONYIMX135_OTP_LSC_WRITED			(1 << 3)
#define SONYIMX135_OTP_LSC_FILE_ERR         (1 << 4)

#define SONYIMX135_DPC_THRESHOLD_ISO			(0x800)
static u8 sonyimx135_otp_flag = 0;

/* OTP lens shading ID. */
static u8 sonyimx135_otp_lsc_id = 0x00;

/* VCM start and end values */
static u16 sonyimx135_vcm_start = 0;
static u16 sonyimx135_vcm_end = 0;

//add by Rose for start current offset
#define SONYIMX135_STARTCODE_OFFSET             0x40

#define SONYIMX135_ISP_ZOOM_LIMIT		3
/* OTP parameters. */
static otp_id_wb_t sonyimx135_otp_id_wb;
static otp_vcm_t sonyimx135_otp_vcm;
static u8 sonyimx135_otp_lsc_param[SONYIMX135_OTP_LSC_SIZE] = {
};

static camera_capability sonyimx135_cap[] = {
	{V4L2_CID_FLASH_MODE, THIS_FLASH},
	{V4L2_CID_FOCUS_MODE, THIS_FOCUS_MODE},
	{V4L2_CID_FOCAL_LENGTH, 222},//2.22mm
};

//#define SONYIMX135_AP_WRITEAE_MODE

/* camera sensor override parameters, define in binning preview mode */
#define SONYIMX135_MAX_ISO			1600
#define SONYIMX135_MIN_ISO			100

#define SONYIMX135_AUTOFPS_GAIN_LOW2MID		0x24
#define SONYIMX135_AUTOFPS_GAIN_MID2HIGH		0x24
#define SONYIMX135_AUTOFPS_GAIN_HIGH2MID		0x88
#define SONYIMX135_AUTOFPS_GAIN_MID2LOW		0x88

#define SONYIMX135_MAX_FRAMERATE		30
#define SONYIMX135_MIDDLE_FRAMERATE		20
#define SONYIMX135_MIN_FRAMERATE		10
#define SONYIMX135_MIN_CAP_FRAMERATE	8

#define SONYIMX135_FLASH_TRIGGER_GAIN	0xff

#define SONYIMX135_SHARPNESS_PREVIEW	0x10
#define SONYIMX135_SHARPNESS_CAPTURE	0x0A

/* camera sensor denoise parameters */
#define SONYIMX135_ISP_YDENOISE_COFF_1X		0x02
#define SONYIMX135_ISP_YDENOISE_COFF_2X		0x02
#define SONYIMX135_ISP_YDENOISE_COFF_4X		0x06
#define SONYIMX135_ISP_YDENOISE_COFF_8X		0x08
#define SONYIMX135_ISP_YDENOISE_COFF_16X		0x10
#define SONYIMX135_ISP_YDENOISE_COFF_32X		0x20
#define SONYIMX135_ISP_YDENOISE_COFF_64X		0x30

#define SONYIMX135_ISP_UVDENOISE_COFF_1X		0x04
#define SONYIMX135_ISP_UVDENOISE_COFF_2X		0x04
#define SONYIMX135_ISP_UVDENOISE_COFF_4X		0x0C
#define SONYIMX135_ISP_UVDENOISE_COFF_8X		0x10
#define SONYIMX135_ISP_UVDENOISE_COFF_16X		0x20
#define SONYIMX135_ISP_UVDENOISE_COFF_32X		0x40
#define SONYIMX135_ISP_UVDENOISE_COFF_64X		0x50

/*camera af param which are associated with sensor*/
#define SONYIMX135_AF_MIN_HEIGHT_RATIO	(7)
#define SONYIMX135_AF_MAX_FOCUS_STEP	(6)
#define SONYIMX135_AF_GSENSOR_INTERVAL_THRESHOLD	(70)
#define SONYIMX135_AF_WIDTH_PERCENT	(12)
#define SONYIMX135_AF_HEIGHT_PERCENT	(15)

static int sensor_mode_index = -1;
static framesize_s *sonyimx135_framesizes = NULL;

typedef enum _sonyimx135_sensor_check
{
  IMX135_UNKOWN = -1,
  IMX135_FOUND,
}sonyimx135_sensor_check;

const struct isp_reg_t isp_init_regs_sonyimx135[] = {
/*{0x65006, 0x01},//y36721 delete, not needed now.*/

/* y36721 2012-03-28 added. */
/* y36721 2012-03-31 changed blc and lenc. */
/*blc value=0x40(10bit)*/
{0x1c58b, 0xff},
{0x1c58c, 0xff},

#ifndef OVISP_DEBUG_MODE
/* AEC */
	{0x1c146, 0x30}, //ori0x30 low AE target,should close
	{0x1c14a, 0x03},
	{0x1c14b, 0x0a},
	{0x1c14c, 0x0f}, //aec fast step//
	{0x1c14e, 0x08}, //slow step//08
	{0x1c140, 0x01}, //banding
	{0x1c13e, 0x02}, //real gain mode for OV8830
	
	{0x66401, 0x00}, //window weight
	{0x66402, 0x20}, //StatWin_Left
	{0x66403, 0x00},
	{0x66404, 0x20}, //StatWin_Top
	{0x66405, 0x00},
	{0x66406, 0x20}, //StatWin_Right
	{0x66407, 0x00},
	{0x66408, 0x28}, //StatWin_Bottom
	{0x66409, 0x01}, //definiton ofthe center 3x3 window
	{0x6640a, 0x00}, //nWin_Left
	{0x6640d, 0x00},
	{0x6640e, 0xc0}, //nWin_Top
	{0x66411, 0x06},
	{0x66412, 0x00}, //nWin_Width
	{0x66415, 0x04},
	{0x66416, 0x80}, //nWin_Height
	{0x6642e, 0x01}, //nWin_Weight_0 weight pass
	{0x6642f, 0x01}, //nWin_Weight_1
	{0x66430, 0x01}, //nWin_Weight_2
	{0x66431, 0x01}, //nWin_Weight_3
	{0x66432, 0x02}, //nWin_Weight_4
	{0x66433, 0x02}, //nWin_Weight_5
	{0x66434, 0x02}, //nWin_Weight_6
	{0x66435, 0x02}, //nWin_Weight_7
	{0x66436, 0x04}, //nWin_Weight_8
	{0x66437, 0x02}, //nWin_Weight_9
	{0x66438, 0x02}, //nWin_Weight_10
	{0x66439, 0x02}, //nWin_Weight_11
	{0x6643a, 0x02}, //nWin_Weight_12
	{0x6644e, 0x03}, //nWin_Weight_Shift
	{0x6644f, 0x04}, //black level
	{0x66450, 0xf8}, //saturate level
	{0x6645b, 0x1a}, //black weight1
	{0x6645d, 0x1a}, //black weight2
	{0x66460, 0x04}, //saturate per1
	{0x66464, 0x0a}, //saturate per2
	{0x66467, 0x14}, //saturate weight1
	{0x66469, 0x14}, //saturate weight2
	//auto AE control
	
/* Raw Stretch */
	{0x65020, 0x00}, //RAW Stretch Target0x01-->00
	{0x66500, 0x28},
	{0x66501, 0x00},
	{0x66502, 0x8f}, //0xff
	{0x66503, 0x0b}, //0x0f
	{0x1c1b0, 0xff},
	{0x1c1b1, 0xff},
	{0x1c1b2, 0x01},
	{0x65905, 0x08},
	{0x66301, 0x02}, //high level step
	{0x66302, 0xd8},//ref bin
	{0x66303, 0x06}, //PsPer0 0a
	{0x66304, 0x10}, //PsPer1
	{0x1c5a4, 0x01}, //use new high stretch
	{0x1c5a5, 0x20}, //stretch low step
	{0x1c5a6, 0x20}, //stretch high step
	{0x1c5a7, 0x08}, //stretch slow range
	{0x1c5a8, 0x02}, //stretch slow step
	{0x1c1b8, 0x10}, //ratio scale
	
	{0x1c5a0, 0x50}, //AE target high, should close
	{0x1c5a2, 0x04}, //target stable range
	{0x1c5a3, 0x06}, //stretch target slow range
	
/* De-noise */
	{0x65604, 0x00}, //Richard for new curve 0314
	{0x65605, 0x00}, //Richard for new curve 0314
	{0x65606, 0x00}, //Richard for new curve 0314
	{0x65607, 0x00}, //Richard for new curve 0314

	{0x65510, 0x04}, //G dns slope change from 0x4 to 0xf Richard 0320
	{0x65511, 0x08}, //G dns slope change from 0x4 to 0xf Richard 0320
	{0x6551a, SONYIMX135_ISP_YDENOISE_COFF_1X},//Raw G Dns improve white pixel 20120728
	{0x6551b, SONYIMX135_ISP_YDENOISE_COFF_2X},//
	{0x6551c, SONYIMX135_ISP_YDENOISE_COFF_4X},//
	{0x6551d, SONYIMX135_ISP_YDENOISE_COFF_8X},//
	{0x6551e, SONYIMX135_ISP_YDENOISE_COFF_16X},//
	{0x6551f, SONYIMX135_ISP_YDENOISE_COFF_32X},//
	{0x65520, SONYIMX135_ISP_YDENOISE_COFF_64X},//
	{0x65522, 0x00},// 
	{0x65523, SONYIMX135_ISP_UVDENOISE_COFF_1X},//
	{0x65524, 0x00},
	{0x65525, SONYIMX135_ISP_UVDENOISE_COFF_2X},//
	{0x65526, 0x00},
	{0x65527, SONYIMX135_ISP_UVDENOISE_COFF_4X},
	{0x65528, 0x00},
	{0x65529, SONYIMX135_ISP_UVDENOISE_COFF_8X},
	{0x6552a, 0x00},
	{0x6552b, SONYIMX135_ISP_UVDENOISE_COFF_16X},
	{0x6552c, 0x00},
	{0x6552d, SONYIMX135_ISP_UVDENOISE_COFF_32X},
	{0x6552e, 0x00},
	{0x6552f, SONYIMX135_ISP_UVDENOISE_COFF_64X},
	
	{0x65c00, 0x03}, //UV De-noise
	{0x65c01, 0x05},
	{0x65c02, 0x08},
	{0x65c03, 0x1f}, // 8X 
	{0x65c04, 0x1f},
	{0x65c05, 0x1f},

/* sharpeness */   
	{0x65600, 0x00},
	{0x65601, 0x20}, //0319
	{0x65602, 0x00},
	{0x65603, 0x60}, //y00215412 change sharpness high gain threshod 0x40->0x60 20120814
	{0x65608, 0x10},
	{0x65609, 0x30},
	{0x6560c, 0x00},
	{0x6560d, 0x0a}, //low gain sharpness, 20120814 0x20->0x30
	{0x6560e, 0x10}, //MinSharpenTp
	{0x6560f, 0x60}, //MaxSharpenTp
	{0x65610, 0x10}, //MinSharpenTm
	{0x65611, 0x60}, //MaxSharpenTm
	{0x65613, 0x10}, //SharpenAlpha
	{0x65615, 0x08}, //HFreq_thre
	{0x65617, 0x06}, //HFreq_coef

/* auto uv saturation */
	{0x1c4e8, 0x01}, //Enable
	{0x1c4e9, 0xbf}, // gain threshold1 40-->0b
	{0x1c4ea, 0xf7}, //gain threshold2 78-->0d
	{0x1c4eb, 0x88}, //UV max saturation
	{0x1c4ec, 0x60}, //UV min saturation

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
        {0x1C4C0, 0x1a},
	{0x1C4C1, 0x28},
	{0x1C4C2, 0x33},
	{0x1C4C3, 0x3f},
	{0x1C4C4, 0x4c},
	{0x1C4C5, 0x59},
	{0x1C4C6, 0x66},
	{0x1C4C7, 0x72},
	{0x1C4C8, 0x7e},
	{0x1C4C9, 0x8a},
	{0x1C4CA, 0x96},
	{0x1C4CB, 0xa4},
	{0x1C4CC, 0xb5},
	{0x1C4CD, 0xC9},
	{0x1C4CE, 0xe2},
	{0x1c4d4, 0x20}, //EDR scale
	{0x1c4d5, 0x20}, //EDR scale
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
	{0x1C998, 0x01},
	{0x1C999, 0x02},
	{0x1C99A, 0x01},
	{0x1C99B, 0x10},
	{0x1C99C, 0x01},
	{0x1C99D, 0x16},
	{0x1C99E, 0x01},
	{0x1C99F, 0x16},
	{0x1C9A0, 0x01},
	{0x1C9A1, 0x16},
	{0x1C9A2, 0x01},
	{0x1C9A3, 0x16},
	{0x1C9A4, 0x01},
	{0x1C9A5, 0x16},
	{0x1C9A6, 0x01},
	{0x1C9A7, 0x16},
	{0x1C9A8, 0x01},
	{0x1C9A9, 0x16},
	{0x1C9AA, 0x01},
	{0x1C9AB, 0x16},
	{0x1C9AC, 0x01},
	{0x1C9AD, 0x16},
	{0x1C9AE, 0x01},
	{0x1C9AF, 0x16},
	{0x1C9B0, 0x01},
	{0x1C9B1, 0x16},
	{0x1C9B2, 0x01},
	{0x1C9B3, 0x08},
	{0x1C9B4, 0x00},
	{0x1C9B5, 0xe6},
	{0x1C9B6, 0x00},
	{0x1C9B7, 0xaa},

/* LENC */
	{0x1c247, 0x00},//one profile,color temperature based lens shading correction mode 1: enable 0: disable
	{0x1c246, 0x00},
	{0x1c24c, 0x00},
	{0x1c24d, 0xbe},
	{0x1c24e, 0x00},
	{0x1c24f, 0xfe},
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
	//Y channel re-back to old version(do not plus 8) 20120821 by y00215412
	{0x1c264, 0x13}, //Y1
	{0x1c265, 0xD }, 
	{0x1c266, 0x8 }, 
	{0x1c267, 0x8 }, 
	{0x1c268, 0xC }, 
	{0x1c269, 0x14}, 
	{0x1c26a, 0x7 }, //Y2
	{0x1c26b, 0x3 }, 
	{0x1c26c, 0x2 }, 
	{0x1c26d, 0x2 }, 
	{0x1c26e, 0x4 }, 
	{0x1c26f, 0x7 }, 
	{0x1c270, 0x3 }, //Y3
	{0x1c271, 0x1 }, 
	{0x1c272, 0x0 }, 
	{0x1c273, 0x0 }, 
	{0x1c274, 0x1 }, 
	{0x1c275, 0x4 }, 
	{0x1c276, 0x3 }, //Y4
	{0x1c277, 0x1 }, 
	{0x1c278, 0x0 }, 
	{0x1c279, 0x0 }, 
	{0x1c27a, 0x1 }, 
	{0x1c27b, 0x4 }, 
	{0x1c27c, 0x7 }, //Y5
	{0x1c27d, 0x3 }, 
	{0x1c27e, 0x2 }, 
	{0x1c27f, 0x2 }, 
	{0x1c280, 0x3 }, 
	{0x1c281, 0x7 }, 
	{0x1c282, 0x14}, //Y6
	{0x1c283, 0xD }, 
	{0x1c284, 0x8 }, 
	{0x1c285, 0x8 }, 
	{0x1c286, 0xC }, 
	{0x1c287, 0x13}, 
	{0x1c288, 0x20}, //Cb1
	{0x1c289, 0x20}, 
	{0x1c28a, 0x20}, 
	{0x1c28b, 0x20}, 
	{0x1c28c, 0x20}, 
	{0x1c28d, 0x20}, //Cb2
	{0x1c28e, 0x20}, 
	{0x1c28f, 0x20}, 
	{0x1c290, 0x20}, 
	{0x1c291, 0x20}, 
	{0x1c292, 0x20}, //Cb3
	{0x1c293, 0x20}, 
	{0x1c294, 0x20}, 
	{0x1c295, 0x20}, 
	{0x1c296, 0x20}, 
	{0x1c297, 0x20}, //Cb4
	{0x1c298, 0x20}, 
	{0x1c299, 0x20}, 
	{0x1c29a, 0x20}, 
	{0x1c29b, 0x20}, 
	{0x1c29c, 0x20}, //Cb5
	{0x1c29d, 0x20}, 
	{0x1c29e, 0x20}, 
	{0x1c29f, 0x20}, 
	{0x1c2a0, 0x20}, 	
	{0x1c2a1, 0x20}, //Cr1
	{0x1c2a2, 0x20},      
	{0x1c2a3, 0x20},      
	{0x1c2a4, 0x20},      
	{0x1c2a5, 0x20},      
	{0x1c2a6, 0x20}, //Cr2
	{0x1c2a7, 0x20},      
	{0x1c2a8, 0x20},      
	{0x1c2a9, 0x20},      
	{0x1c2aa, 0x20},      
	{0x1c2ab, 0x20}, //Cr3
	{0x1c2ac, 0x20},      
	{0x1c2ad, 0x20},      
	{0x1c2ae, 0x20},      
	{0x1c2af, 0x20},      
	{0x1c2b0, 0x20}, //Cr4
	{0x1c2b1, 0x20},      
	{0x1c2b2, 0x20},      
	{0x1c2b3, 0x20},      
	{0x1c2b4, 0x20},      
	{0x1c2b5, 0x20}, //cr5
	{0x1c2b6, 0x20},      
	{0x1c2b7, 0x20},      
	{0x1c2b8, 0x20},      
	{0x1c2b9, 0x20},   
	
/* AWB */
	{0x66201, 0x52},
	{0x66203, 0x14}, //crop window
	{0x66211, 0xe8}, //awb top limit
	{0x66212, 0x04}, //awb bottom limit	
	//{0x1c17c, 0x01}, //CT mode should close
	{0x1c182, 0x04},
	{0x1c183, 0x00}, //MinNum
	{0x1c184, 0x04}, //AWB Step		
	{0x1c58d, 0x00}, //LimitAWBAtD65Enable
	{0x1c1be, 0x00}, //AWB offset
	{0x1c1bf, 0x00},
	{0x1c1c0, 0x00},
	{0x1c1c1, 0x00},

	{0x1c1aa, 0x00}, //avgAllEnable	
	{0x1c1ad, 0x02}, //weight of A
	{0x1c1ae, 0x06}, //weight of D65
	{0x1c1af, 0x04}, //weight of CWF
	
	{0x1c5ac, 0x80}, //pre-gain 
	{0x1c5ad, 0x80},
	{0x1c5ae, 0x80},

	{0x1ccce, 0x02}, //awb shift,open=02
	{0x1cccf, 0x30},//B gain for A
	{0x1ccd0, 0x18},//R gain for A

	{0x1c5b8, 0x20},//B gain for C outdoor Richard@0517
	{0x1c5b9, 0x38},//R gain for C outdoor Richard@0517
	{0x1ccd1, 0x20},//B gain for C indoor Richard@0517
	{0x1ccd2, 0x38},//R gain for C indoor Richard@0517

	{0x1ccd3, 0x10},//B gain for D indoor
	{0x1ccd4, 0x30},//R gain for D indoor
	{0x1cccc, 0x10},//B gain for D outdoor
	{0x1cccd, 0x30},//R gain for D outdoor
	// for detect indoor/outdoor awb shift on cwf light,epxo*gain>>8
	{0x1c5b4, 0x02}, //C indoor/outdoor switch
	{0x1c5b5, 0xff}, //C indoor/outdoor switch
	{0x1c5b6, 0x04}, //C indoor/outdoor switch
	{0x1c5b7, 0xff}, //C indoor/outdoor switch

	//add awb  shift detect parameter according 0x1c734~0x1c736 
	{0x1ccd5, 0x41}, //CT_A, 2012.06.02 yuanyabin
	{0x1ccd6, 0x69}, //CT_C
	{0x1ccd7, 0xb8}, //CT_D

	{0x1c5cd, 0x00}, //high light awb shift, modified by Jiangtao to avoid blurish when high CT 0310
	{0x1c5ce, 0x00},
	{0x1c5cf, 0xf0},
	{0x1c5d0, 0x01},
	{0x1c5d1, 0x20},
	{0x1c5d2, 0x03},
	{0x1c5d3, 0x00},
	{0x1c5d4, 0x40},
	{0x1c5d5, 0xa0},
	{0x1c5d6, 0xb0},
	{0x1c5d7, 0xe8},
	{0x1c5d8, 0x40},
	{0x1c1c2, 0x00},
	{0x1c1c3, 0x20},

	{0x66206, 0x11}, //center(cwf) window threshold D0
	{0x66207, 0x24}, //A threshold, range DX  0x15
	{0x66208, 0x20}, //day threshold, range DY 0xd
	{0x66209, 0x76}, //CWF X
	{0x6620a, 0x63}, //CWF Y
	{0x6620b, 0xf4}, //K_A_X2Y, a slop
	{0x6620c, 0xc0}, //K_D65_Y2X, d slop
	{0x6620d, 0x4a}, //D65 Limit
	{0x6620e, 0x3e}, //A Limit
	{0x6620f, 0x68}, //D65 split
	{0x66210, 0x58}, //A split
	{0x66201, 0x52},
	
	//add ccm detect parameter according 0x1c734~0x1c736 
	{0x1c1c8 ,0x01}, //center CT, CWF
	{0x1c1c9 ,0x37},
	{0x1c1cc, 0x00}, //daylight
	{0x1c1cd, 0xb1},
	{0x1c1d0, 0x01}, //a
	{0x1c1d1, 0xf4},
	
	{0x1c254, 0x00},
	{0x1c255, 0xce},
	{0x1c256, 0x00},
	{0x1c257, 0xe7},
	{0x1c258, 0x01},
	{0x1c259, 0x69},
	{0x1c25a, 0x01},
	{0x1c25b, 0xd2},
	
/* Color matrix */
	{0x1C1d8, 0x02},//center matrix, 
	{0x1C1d9, 0x34},
	{0x1C1da, 0xFE},
	{0x1C1db, 0xC7},
	{0x1C1dc, 0x00},
	{0x1C1dd, 0x05},
	
	{0x1C1de, 0xFF},
	{0x1C1df, 0xD5},
	{0x1C1e0, 0x01},
	{0x1C1e1, 0xA6},
	{0x1C1e2, 0xFF},
	{0x1C1e3, 0x85},

	{0x1C1e4, 0x00},
	{0x1C1e5, 0x2A},
	{0x1C1e6, 0xFE},
	{0x1C1e7, 0xCD},
	{0x1C1e8, 0x02},
	{0x1C1e9, 0x09},

	{0x1C1FC, 0xFF},//cmx left delta,d
	{0x1C1FD, 0xD4},
	{0x1C1FE, 0x00},
	{0x1C1FF, 0x1A},
	{0x1C200, 0x00},
	{0x1C201, 0x12},
	{0x1C202, 0xFF},
	{0x1C203, 0xAE},
	{0x1C204, 0x00},
	{0x1C205, 0x0A},
	{0x1C206, 0x00},
	{0x1C207, 0x48},
	{0x1C208, 0xFF},
	{0x1C209, 0xDF},
	{0x1C20A, 0x00},
	{0x1C20B, 0x47},
	{0x1C20C, 0xFF},
	{0x1C20D, 0xDA},

	{0x1C220, 0xFF},//cmx right delta,a
	{0x1C221, 0xF3},
	{0x1C222, 0x00},
	{0x1C223, 0x09},
	{0x1C224, 0x00},
	{0x1C225, 0x04},
	
	{0x1C226, 0x00},
	{0x1C227, 0x09},
	{0x1C228, 0xff},
	{0x1C229, 0xe2},
	{0x1C22A, 0x00},
	{0x1C22B, 0x15},
	
	{0x1C22C, 0x00},
	{0x1C22D, 0x08},
	{0x1C22E, 0x00},
	{0x1C22F, 0x07},
	{0x1C230, 0xFF},
	{0x1C231, 0xF1},
	
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
	{0x1d963, 0x1a}, 
	{0x1d964, 0x28}, 
	{0x1d965, 0x33}, 
	{0x1d966, 0x3f}, 
	{0x1d967, 0x4a}, 
	{0x1d968, 0x56}, 
	{0x1d969, 0x61}, 
	{0x1d96a, 0x6c}, 
	{0x1d96b, 0x78}, 
	{0x1d96c, 0x84}, 
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
	{0x1d904, 0x81},
	{0x1d905, 0x88},
	{0x1d906, 0x8b},
	{0x1d907, 0x8b},
	{0x1d908, 0x8b},
	{0x1d909, 0x8b},
	{0x1d90a, 0x8b},
	{0x1d90b, 0x8b},
	{0x1d90c, 0x8b},
	{0x1d90d, 0x8b},
	{0x1d90e, 0x8b},
	{0x1d90f, 0x8b},
	{0x1d910, 0x8b},
	{0x1d911, 0x84},
	{0x1d912, 0x73},
	{0x1d913, 0x55},
	//high gain UV curve 1/2
	{0x1d914, 0x81},
	{0x1d915, 0x88},
	{0x1d916, 0x8b},
	{0x1d917, 0x8b},
	{0x1d918, 0x8b},
	{0x1d919, 0x8b},
	{0x1d91a, 0x8b},
	{0x1d91b, 0x8b},
	{0x1d91c, 0x8b},
	{0x1d91d, 0x8b},
	{0x1d91e, 0x8b},
	{0x1d91f, 0x8b},
	{0x1d920, 0x8b},
	{0x1d921, 0x84},
	{0x1d922, 0x73},
	{0x1d923, 0x55},

	//dynamic CT AWB, huiyan 20120810 add for new firmware AWB
	{0x1d924, 0x01}, //enable
	{0x1d950, 0x00}, //Br thres,super highlight threshold
	{0x1d951, 0x30}, //Br thres
	{0x1d952, 0x30}, //Br Ratio,Ratio for transition region
	
	{0x1d8dc, 0x00}, //Br thres0
	{0x1d8dd, 0xf0}, //Br thres0
	{0x1d8de, 0x44}, //Br thres1
	{0x1d8df, 0x34}, //Br thres1
	{0x1d8da, 0x20}, //Br Ratio0
	{0x1d8db, 0x06}, //Br Ratio1
	{0x1d949, 0x11}, //super highlight cwf thres //66206
	{0x1d925, 0x11}, //highlight cwf thres //66206
	{0x1d926, 0x11}, //middlelight cwf thres
	{0x1d927, 0x12}, //lowlight cwf thres
	
	{0x1d94a, 0x1b}, //super highlight A thres //66207
	{0x1d928, 0x1b}, //highlight A thres //66207
	{0x1d929, 0x26}, //middlelight A thres
	{0x1d92a, 0x26}, //lowlight A thres

	{0x1d94b, 0x13}, //super highlight D thres //66208                                 
	{0x1d92b, 0x20}, //highlight D thres //66208                                       
	{0x1d92c, 0x23}, //middlelight D thres                                             
	{0x1d92d, 0x23}, //lowlight D thres

	{0x1d94c, 0x57}, //super highlight D limit //6620    
	{0x1d92e, 0x53}, //highlight D limit //6620d          
	{0x1d92f, 0x4a}, //middlelight D limit                         
	{0x1d930, 0x4a}, //lowlight D limit

	{0x1d94d, 0x49}, //super highlight A limit //6620e                
	{0x1d931, 0x43}, //highlight A limit //6620e
	{0x1d932, 0x3d}, //middlelight A limit
	{0x1d933, 0x3d}, //lowlight A limit

	{0x1d94e, 0x67}, //super highlight D split //6620f
	{0x1d934, 0x67}, //highlight D split //6620f
	{0x1d935, 0x67}, //middlelight D split
	{0x1d936, 0x67}, //lowlight D split

	{0x1d94f, 0x58}, //super highlight A split //66210
	{0x1d937, 0x58}, //highlight A split //66210
	{0x1d938, 0x58}, //middlelight A split
	{0x1d939, 0x58}, //lowlight A split
	{0x1d998, 0x1},

#endif
};

const struct isp_reg_t isp_init_regs_sonyimx135_HDR[] = {
/*{0x65006, 0x01},//y36721 delete, not needed now.*/

/* y36721 2012-03-28 added. */
/* y36721 2012-03-31 changed blc and lenc. */
/*blc value=0x40(10bit)*/
{0x1c58b, 0xff},
{0x1c58c, 0xff},

#ifndef HDR_MOVIE_DEBUG_MODE
/* AEC */
	{0x1c146, 0x30}, //ori0x30 low AE target,should close
	{0x1c14a, 0x03},
	{0x1c14b, 0x0a},
	{0x1c14c, 0x0f}, //aec fast step//
	{0x1c14e, 0x08}, //slow step//08
	{0x1c140, 0x01}, //banding
	{0x1c13e, 0x02}, //real gain mode for OV8830
	
	{0x66401, 0x00}, //window weight
	{0x66402, 0x20}, //StatWin_Left
	{0x66403, 0x00},
	{0x66404, 0x20}, //StatWin_Top
	{0x66405, 0x00},
	{0x66406, 0x20}, //StatWin_Right
	{0x66407, 0x00},
	{0x66408, 0x28}, //StatWin_Bottom
	{0x66409, 0x01}, //definiton ofthe center 3x3 window
	{0x6640a, 0x00}, //nWin_Left
	{0x6640d, 0x00},
	{0x6640e, 0xc0}, //nWin_Top
	{0x66411, 0x06},
	{0x66412, 0x00}, //nWin_Width
	{0x66415, 0x04},
	{0x66416, 0x80}, //nWin_Height
	{0x6642e, 0x01}, //nWin_Weight_0 weight pass
	{0x6642f, 0x01}, //nWin_Weight_1
	{0x66430, 0x01}, //nWin_Weight_2
	{0x66431, 0x01}, //nWin_Weight_3
	{0x66432, 0x02}, //nWin_Weight_4
	{0x66433, 0x02}, //nWin_Weight_5
	{0x66434, 0x02}, //nWin_Weight_6
	{0x66435, 0x02}, //nWin_Weight_7
	{0x66436, 0x04}, //nWin_Weight_8
	{0x66437, 0x02}, //nWin_Weight_9
	{0x66438, 0x02}, //nWin_Weight_10
	{0x66439, 0x02}, //nWin_Weight_11
	{0x6643a, 0x02}, //nWin_Weight_12
	{0x6644e, 0x03}, //nWin_Weight_Shift
	{0x6644f, 0x04}, //black level
	{0x66450, 0xf8}, //saturate level
	{0x6645b, 0x1a}, //black weight1
	{0x6645d, 0x1a}, //black weight2
	{0x66460, 0x04}, //saturate per1
	{0x66464, 0x0a}, //saturate per2
	{0x66467, 0x14}, //saturate weight1
	{0x66469, 0x14}, //saturate weight2
	//auto AE control
	
/* Raw Stretch */
	{0x65020, 0x00}, //RAW Stretch Target0x01-->00
	{0x66500, 0x28},
	{0x66501, 0x00},
	{0x66502, 0x8f}, //0xff
	{0x66503, 0x0b}, //0x0f
	{0x1c1b0, 0xff},
	{0x1c1b1, 0xff},
	{0x1c1b2, 0x01},
	{0x65905, 0x08},
	{0x66301, 0x02}, //high level step
	{0x66302, 0xd8},//ref bin
	{0x66303, 0x06}, //PsPer0 0a
	{0x66304, 0x10}, //PsPer1
	{0x1c5a4, 0x01}, //use new high stretch
	{0x1c5a5, 0x20}, //stretch low step
	{0x1c5a6, 0x20}, //stretch high step
	{0x1c5a7, 0x08}, //stretch slow range
	{0x1c5a8, 0x02}, //stretch slow step
	{0x1c1b8, 0x10}, //ratio scale
	
	{0x1c5a0, 0x50}, //AE target high, should close
	{0x1c5a2, 0x04}, //target stable range
	{0x1c5a3, 0x06}, //stretch target slow range
	
/* De-noise */
	{0x65604, 0x00}, //Richard for new curve 0314
	{0x65605, 0x00}, //Richard for new curve 0314
	{0x65606, 0x00}, //Richard for new curve 0314
	{0x65607, 0x00}, //Richard for new curve 0314
	
	{0x65510, 0x0F}, //G dns slope change from 0x4 to 0xf Richard 0320
	{0x65511, 0x1E}, //G dns slope change from 0x4 to 0xf Richard 0320
	{0x6551a, 0x00}, // 1X  Raw G Dns improve white pixel 20120728
	{0x6551b, 0x04}, // 2X
	{0x6551c, 0x0C}, // 4X
	{0x6551d, 0x12}, // 8X
	{0x6551e, 0x18}, // 16X
	{0x6551f, 0x30}, // 32X
	{0x65520, 0x40}, // 64X
	{0x65522, 0x00}, //RAW BR De-noise
	{0x65523, 0x00}, // 1X
	{0x65524, 0x00},
	{0x65525, 0x08}, // 2X
	{0x65526, 0x00},
	{0x65527, 0x16}, // 4X  
	{0x65528, 0x00},
	{0x65529, 0x20}, // 8X 
	{0x6552a, 0x00},
	{0x6552b, 0x30}, // 16X 
	{0x6552c, 0x00},
	{0x6552d, 0x40}, // 32X 
	{0x6552e, 0x00},
	{0x6552f, 0x50}, // 64X 
	
	{0x65c00, 0x03}, //UV De-noise
	{0x65c01, 0x05},
	{0x65c02, 0x08},
	{0x65c03, 0x1f}, // 8X 
	{0x65c04, 0x1f},
	{0x65c05, 0x1f},
/* sharpeness */   
	{0x65600, 0x00},
	{0x65601, 0x20}, //0319
	{0x65602, 0x00},
	{0x65603, 0x60}, //y00215412 change sharpness high gain threshod 0x40->0x60 20120814
	{0x65608, 0x06},
	{0x65609, 0x20},
	{0x6560c, 0x02},
	{0x6560d, 0x08}, //low gain sharpness, 20120814 0x20->0x30
	{0x6560e, 0x10}, //MinSharpenTp
	{0x6560f, 0x60}, //MaxSharpenTp
	{0x65610, 0x10}, //MinSharpenTm
	{0x65611, 0x60}, //MaxSharpenTm
	{0x65613, 0x10}, //SharpenAlpha
	{0x65615, 0x08}, //HFreq_thre
	{0x65617, 0x06}, //HFreq_coef

/* auto uv saturation */
	{0x1c4e8, 0x01}, //Enable
	{0x1c4e9, 0xbf}, // gain threshold1 40-->0b
	{0x1c4ea, 0xf7}, //gain threshold2 78-->0d
	{0x1c4eb, 0x68}, //UV max saturation
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
	{0x1C4C0, 0x18}, //0x1a
	{0x1C4C1, 0x27}, //0x28
	{0x1C4C2, 0x33}, //0x33
	{0x1C4C3, 0x3f}, //0x3f
	{0x1C4C4, 0x4c}, //0x4c
	{0x1C4C5, 0x59}, //0x59
	{0x1C4C6, 0x66}, //0x66
	{0x1C4C7, 0x72}, //0x72
	{0x1C4C8, 0x7e}, //0x7e
	{0x1C4C9, 0x8a}, //0x8a
	{0x1C4CA, 0x96}, //0x96
	{0x1C4CB, 0xa4}, //0xa4
	{0x1C4CC, 0xb5}, //0xb5
	{0x1C4CD, 0xC8}, //0xC9
	{0x1C4CE, 0xe0}, //0xe2
	{0x1c4d4, 0x20}, //0x20 //EDR scale
	{0x1c4d5, 0x20}, //0x20 //EDR scale
	{0x1c4cf, 0x80}, //0x80
	{0x65a00, 0x1b}, //0x1b
	{0x65a01, 0xc0}, //0xc0 //huiyan 0801

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
	{0x1C998, 0x01},
	{0x1C999, 0x02},
	{0x1C99A, 0x01},
	{0x1C99B, 0x0a},
	{0x1C99C, 0x01},
	{0x1C99D, 0x0d},
	{0x1C99E, 0x01},
	{0x1C99F, 0x10},
	{0x1C9A0, 0x01},
	{0x1C9A1, 0x10},
	{0x1C9A2, 0x01},
	{0x1C9A3, 0x10},
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
	{0x1C9B1, 0x10},
	{0x1C9B2, 0x01},
	{0x1C9B3, 0x0d},
	{0x1C9B4, 0x00},
	{0x1C9B5, 0xe6},
	{0x1C9B6, 0x00},
	{0x1C9B7, 0xaa},

/* LENC */
	{0x1c247, 0x00},//one profile,color temperature based lens shading correction mode 1: enable 0: disable
	{0x1c246, 0x00},
	{0x1c24c, 0x00},
	{0x1c24d, 0xbe},
	{0x1c24e, 0x00},
	{0x1c24f, 0xfe},
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
	//Y channel re-back to old version(do not plus 8) 20120821 by y00215412
	{0x1c264, 0x13}, //Y1
	{0x1c265, 0xD }, 
	{0x1c266, 0x8 }, 
	{0x1c267, 0x8 }, 
	{0x1c268, 0xC }, 
	{0x1c269, 0x14}, 
	{0x1c26a, 0x7 }, //Y2
	{0x1c26b, 0x3 }, 
	{0x1c26c, 0x2 }, 
	{0x1c26d, 0x2 }, 
	{0x1c26e, 0x4 }, 
	{0x1c26f, 0x7 }, 
	{0x1c270, 0x3 }, //Y3
	{0x1c271, 0x1 }, 
	{0x1c272, 0x0 }, 
	{0x1c273, 0x0 }, 
	{0x1c274, 0x1 }, 
	{0x1c275, 0x4 }, 
	{0x1c276, 0x3 }, //Y4
	{0x1c277, 0x1 }, 
	{0x1c278, 0x0 }, 
	{0x1c279, 0x0 }, 
	{0x1c27a, 0x1 }, 
	{0x1c27b, 0x4 }, 
	{0x1c27c, 0x7 }, //Y5
	{0x1c27d, 0x3 }, 
	{0x1c27e, 0x2 }, 
	{0x1c27f, 0x2 }, 
	{0x1c280, 0x3 }, 
	{0x1c281, 0x7 }, 
	{0x1c282, 0x14}, //Y6
	{0x1c283, 0xD }, 
	{0x1c284, 0x8 }, 
	{0x1c285, 0x8 }, 
	{0x1c286, 0xC }, 
	{0x1c287, 0x13}, 
	{0x1c288, 0x20}, //Cb1
	{0x1c289, 0x20}, 
	{0x1c28a, 0x20}, 
	{0x1c28b, 0x20}, 
	{0x1c28c, 0x20}, 
	{0x1c28d, 0x20}, //Cb2
	{0x1c28e, 0x20}, 
	{0x1c28f, 0x20}, 
	{0x1c290, 0x20}, 
	{0x1c291, 0x20}, 
	{0x1c292, 0x20}, //Cb3
	{0x1c293, 0x20}, 
	{0x1c294, 0x20}, 
	{0x1c295, 0x20}, 
	{0x1c296, 0x20}, 
	{0x1c297, 0x20}, //Cb4
	{0x1c298, 0x20}, 
	{0x1c299, 0x20}, 
	{0x1c29a, 0x20}, 
	{0x1c29b, 0x20}, 
	{0x1c29c, 0x20}, //Cb5
	{0x1c29d, 0x20}, 
	{0x1c29e, 0x20}, 
	{0x1c29f, 0x20}, 
	{0x1c2a0, 0x20}, 	
	{0x1c2a1, 0x20}, //Cr1
	{0x1c2a2, 0x20},      
	{0x1c2a3, 0x20},      
	{0x1c2a4, 0x20},      
	{0x1c2a5, 0x20},      
	{0x1c2a6, 0x20}, //Cr2
	{0x1c2a7, 0x20},      
	{0x1c2a8, 0x20},      
	{0x1c2a9, 0x20},      
	{0x1c2aa, 0x20},      
	{0x1c2ab, 0x20}, //Cr3
	{0x1c2ac, 0x20},      
	{0x1c2ad, 0x20},      
	{0x1c2ae, 0x20},      
	{0x1c2af, 0x20},      
	{0x1c2b0, 0x20}, //Cr4
	{0x1c2b1, 0x20},      
	{0x1c2b2, 0x20},      
	{0x1c2b3, 0x20},      
	{0x1c2b4, 0x20},      
	{0x1c2b5, 0x20}, //cr5
	{0x1c2b6, 0x20},      
	{0x1c2b7, 0x20},      
	{0x1c2b8, 0x20},      
	{0x1c2b9, 0x20},   
	
/* AWB */
	{0x66201, 0x52},
	{0x66203, 0x14}, //crop window
	{0x66211, 0xe8}, //awb top limit
	{0x66212, 0x04}, //awb bottom limit	
	//{0x1c17c, 0x01}, //CT mode should close
	{0x1c182, 0x04},
	{0x1c183, 0x00}, //MinNum
	{0x1c184, 0x04}, //AWB Step		
	{0x1c58d, 0x00}, //LimitAWBAtD65Enable
	{0x1c1be, 0x00}, //AWB offset
	{0x1c1bf, 0x00},
	{0x1c1c0, 0x00},
	{0x1c1c1, 0x00},

	{0x1c1aa, 0x00}, //avgAllEnable	
	{0x1c1ad, 0x02}, //weight of A
	{0x1c1ae, 0x06}, //weight of D65
	{0x1c1af, 0x04}, //weight of CWF
	
	{0x1c5ac, 0x80}, //pre-gain 
	{0x1c5ad, 0x80},
	{0x1c5ae, 0x80},

	{0x1ccce, 0x02}, //awb shift,open=02	
	{0x1cccf, 0x38},//B gain for A                                   
	{0x1ccd0, 0x28},//R gain for A         
	                          
	{0x1c5b8, 0x20},//B gain for C outdoor Richard@0517                       
	{0x1c5b9, 0x34},//R gain for C outdoor Richard@0517                       
	{0x1ccd1, 0x20},//B gain for C indoor Richard@0517                        
	{0x1ccd2, 0x34},//R gain for C indoor Richard@0517                        
	                  
	{0x1ccd3, 0x00},//B gain for D indoor                                   
	{0x1ccd4, 0x38},//R gain for D indoor                                   
	{0x1cccc, 0x00},//B gain for D outdoor                                  
	{0x1cccd, 0x38},//R gain for D outdoor  
	                                
	// for detect indoor/outdoor awb shift on cwf light,epxo*gain>>8
	{0x1c5b4, 0x02}, //C indoor/outdoor switch 
	{0x1c5b5, 0xff}, //C indoor/outdoor switch 
	{0x1c5b6, 0x04}, //C indoor/outdoor switch 
	{0x1c5b7, 0xff}, //C indoor/outdoor switch 

	//add awb  shift detect parameter according 0x1c734~0x1c736 
	{0x1ccd5, 0x41}, //CT_A, 2012.06.02 yuanyabin
	{0x1ccd6, 0x69}, //CT_C
	{0x1ccd7, 0xb8}, //CT_D

	{0x1c5cd, 0x00}, //high light awb shift, modified by Jiangtao to avoid blurish when high CT 0310
	{0x1c5ce, 0x00},
	{0x1c5cf, 0xf0},
	{0x1c5d0, 0x01},
	{0x1c5d1, 0x20},
	{0x1c5d2, 0x03},
	{0x1c5d3, 0x00},
	{0x1c5d4, 0x40},
	{0x1c5d5, 0xa0},
	{0x1c5d6, 0xb0},
	{0x1c5d7, 0xe8},
	{0x1c5d8, 0x40},
	{0x1c1c2, 0x00},
	{0x1c1c3, 0x20},

	{0x66206, 0x11}, //center(cwf) window threshold D0
	{0x66207, 0x24}, //A threshold, range DX  0x15
	{0x66208, 0x20}, //day threshold, range DY 0xd
	{0x66209, 0x76}, //CWF X
	{0x6620a, 0x63}, //CWF Y
	{0x6620b, 0xf2}, //K_A_X2Y, a slop
	{0x6620c, 0xc0}, //K_D65_Y2X, d slop
	{0x6620d, 0x4a}, //D65 Limit
	{0x6620e, 0x3e}, //A Limit
	{0x6620f, 0x68}, //D65 split
	{0x66210, 0x58}, //A split
	{0x66201, 0x52},
	
	//add ccm detect parameter according 0x1c734~0x1c736 
	{0x1c1c8 ,0x01}, //center CT, CWF
	{0x1c1c9 ,0x37},
	{0x1c1cc, 0x00}, //daylight
	{0x1c1cd, 0xb1},
	{0x1c1d0, 0x01}, //a
	{0x1c1d1, 0xf4},
	
	{0x1c254, 0x00},
	{0x1c255, 0xce},
	{0x1c256, 0x00},
	{0x1c257, 0xe7},
	{0x1c258, 0x01},
	{0x1c259, 0x69},
	{0x1c25a, 0x01},
	{0x1c25b, 0xd2},
	
/* Color matrix */
	{0x1C1d8, 0x02},//center matrix, 
	{0x1C1d9, 0x6a},
	{0x1C1da, 0xfe},
	{0x1C1db, 0x9e},
	{0x1C1dc, 0xff},
	{0x1C1dd, 0xf8},
	{0x1C1de, 0xff},
	{0x1C1df, 0xb1},
	{0x1C1e0, 0x01},
	{0x1C1e1, 0xe3},
	{0x1C1e2, 0xff},
	{0x1C1e3, 0x6c},
	{0x1C1e4, 0x00},
	{0x1C1e5, 0x06},
	{0x1C1e6, 0xfe},
	{0x1C1e7, 0xae},
	{0x1C1e8, 0x02},
	{0x1C1e9, 0x4c},		
	
	{0x1C1FC, 0xff},//cmx left delta,daylight
	{0x1C1FD, 0xd9},
	{0x1C1FE, 0x00},
	{0x1C1FF, 0x1f},
	{0x1C200, 0x00},
	{0x1C201, 0x08},
	{0x1C202, 0xff},
	{0x1C203, 0xa8},
	{0x1C204, 0x00},
	{0x1C205, 0x0b},
	{0x1C206, 0x00},
	{0x1C207, 0x4d},
	{0x1C208, 0xff},
	{0x1C209, 0xe3},
	{0x1C20A, 0x00},
	{0x1C20B, 0x33},
	{0x1C20C, 0xff},
	{0x1C20D, 0xea},
	
	{0x1C220, 0x00},//cmx right delta,a light
	{0x1C221, 0x9b},
	{0x1C222, 0xff},
	{0x1C223, 0x77},
	{0x1C224, 0xff},
	{0x1C225, 0xee},
	{0x1C226, 0xff},
	{0x1C227, 0xda},
	{0x1C228, 0x00},
	{0x1C229, 0x1c},
	{0x1C22A, 0x00},
	{0x1C22B, 0x0a},
	{0x1C22C, 0xff},
	{0x1C22D, 0xb7},
	{0x1C22E, 0x01},
	{0x1C22F, 0x23},
	{0x1C230, 0xff},
	{0x1C231, 0x26},
	
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
	{0x1d963, 0x1a}, 
	{0x1d964, 0x28}, 
	{0x1d965, 0x33}, 
	{0x1d966, 0x3f}, 
	{0x1d967, 0x4a}, 
	{0x1d968, 0x56}, 
	{0x1d969, 0x61}, 
	{0x1d96a, 0x6c}, 
	{0x1d96b, 0x78}, 
	{0x1d96c, 0x84}, 
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
	{0x1d904, 0x81},
	{0x1d905, 0x85},
	{0x1d906, 0x86},
	{0x1d907, 0x88},
	{0x1d908, 0x88},
	{0x1d909, 0x88},
	{0x1d90a, 0x88},
	{0x1d90b, 0x88},
	{0x1d90c, 0x88},
	{0x1d90d, 0x88},
	{0x1d90e, 0x88},
	{0x1d90f, 0x88},
	{0x1d910, 0x88},
	{0x1d911, 0x86}, 
	{0x1d912, 0x73},
	{0x1d913, 0x55},
	//high gain UV curve 1/2
	{0x1d914, 0x81},
	{0x1d915, 0x85},
	{0x1d916, 0x86},
	{0x1d917, 0x88},
	{0x1d918, 0x88},
	{0x1d919, 0x88},
	{0x1d91a, 0x88},
	{0x1d91b, 0x88},
	{0x1d91c, 0x88},
	{0x1d91d, 0x88},
	{0x1d91e, 0x88},
	{0x1d91f, 0x88},
	{0x1d920, 0x88},
	{0x1d921, 0x86},
	{0x1d922, 0x73},
	{0x1d923, 0x55},

	//dynamic CT AWB, huiyan 20120810 add for new firmware AWB
	{0x1d924, 0x01}, //enable
	{0x1d950, 0x00}, //Br thres,super highlight threshold
	{0x1d951, 0x30}, //Br thres
	{0x1d952, 0x30}, //Br Ratio,Ratio for transition region
	
	{0x1d8dc, 0x00}, //Br thres0
	{0x1d8dd, 0xf0}, //Br thres0
	{0x1d8de, 0x44}, //Br thres1
	{0x1d8df, 0x34}, //Br thres1
	{0x1d8da, 0x20}, //Br Ratio0
	{0x1d8db, 0x06}, //Br Ratio1
	{0x1d949, 0x11}, //super highlight cwf thres //66206
	{0x1d925, 0x11}, //highlight cwf thres //66206
	{0x1d926, 0x11}, //middlelight cwf thres
	{0x1d927, 0x12}, //lowlight cwf thres
	
	{0x1d94a, 0x1b}, //super highlight A thres //66207
	{0x1d928, 0x1b}, //highlight A thres //66207
	{0x1d929, 0x24}, //middlelight A thres
	{0x1d92a, 0x24}, //lowlight A thres
		
	{0x1d94b, 0x13}, //super highlight D thres //66208                                 
	{0x1d92b, 0x20}, //highlight D thres //66208                                       
	{0x1d92c, 0x23}, //middlelight D thres                                             
	{0x1d92d, 0x23}, //lowlight D thres
		                                          
	{0x1d94c, 0x57}, //super highlight D limit //6620    
	{0x1d92e, 0x53}, //highlight D limit //6620d          
	{0x1d92f, 0x4a}, //middlelight D limit                         
	{0x1d930, 0x4a}, //lowlight D limit
			            
	{0x1d94d, 0x49}, //super highlight A limit //6620e                
	{0x1d931, 0x49}, //highlight A limit //6620e                      
	{0x1d932, 0x40}, //middlelight A limit                            
	{0x1d933, 0x40}, //lowlight A limit
			               
	{0x1d94e, 0x67}, //super highlight D split //6620f                
	{0x1d934, 0x67}, //highlight D split //6620f                      
	{0x1d935, 0x67}, //middlelight D split                            
	{0x1d936, 0x67}, //lowlight D split  
	                             
	{0x1d94f, 0x58},	//super highlight A split //66210                
	{0x1d937, 0x58}, //highlight A split //66210                      
	{0x1d938, 0x58}, //middlelight A split                            
	{0x1d939, 0x58}, //lowlight A split		               
	{0x1d998, 0x1},


#endif
};
/*
 * should be calibrated, three lights, from 0x1c264B gain for D outdoor
 * here is long exposure
 */
char sonyimx135_lensc_param[86*3] = {
};

/* should be calibrated, 6 groups 3x3, from 0x1c1d8 */
short sonyimx135_ccm_param[54] = {
};

char sonyimx135_awb_param[] = {
};

static framesize_s sonyimx135_rgbw_es1_framesizes[] = {
	{0, 2, 2096, 1552, 4572, 1850, 30, 25, 0x19A, 0x155, 0x100, VIEW_FULL, RESOLUTION_4_3, true, {sonyimx135_rgbw_es1_framesize_preview, ARRAY_SIZE(sonyimx135_rgbw_es1_framesize_preview)} },
	{0, 2, 4208, 3120, 4572, 3142, 10, 5, 0x15D, 0x123, 0xA0, VIEW_FULL, RESOLUTION_4_3, false, {sonyimx135_rgbw_es1_framesize_full, ARRAY_SIZE(sonyimx135_rgbw_es1_framesize_full)} },
};

static framesize_s sonyimx135_rgb_framesizes[] = {
	{0, 2, 1920, 1088, 4572, 2336, 30, 10, 0x2BB, 0x247,0x100, VIEW_HDR_MOVIE, RESOLUTION_16_9,true,{sonyimx135_rgb_framesize_1920x1080_HDR, ARRAY_SIZE(sonyimx135_rgb_framesize_1920x1080_HDR)} },
	{0, 2, 2048, 1200, 4572, 2424, 28, 25, 0x2BB, 0x247,0x100, VIEW_HDR_MOVIE, RESOLUTION_16_9,true,{sonyimx135_rgb_framesize_2048x1200_HDR, ARRAY_SIZE(sonyimx135_rgb_framesize_2048x1200_HDR)} },

	{0, 2, 2096, 1552, 4900, 2184, 30, 13, 0x28D, 0x220, 0x100, VIEW_FULL, RESOLUTION_4_3, true, {sonyimx135_rgb_framesize_preview, ARRAY_SIZE(sonyimx135_rgb_framesize_preview)} },
	{0, 2, 4208, 3120, 4992, 3242, 10, 7, 0x140, 0x10B, 0xEA, VIEW_FULL, RESOLUTION_4_3, false, {sonyimx135_rgb_framesize_full, ARRAY_SIZE(sonyimx135_rgb_framesize_full)} },
};

static framesize_s sonyimx135_rgbw_es2_framesizes[] = {
	{0, 2, 2096, 1552, 4572, 1850, 30, 25, 0x19A, 0x155, 0x100, VIEW_FULL, RESOLUTION_4_3, true, {sonyimx135_rgbw_es2_framesize_preview, ARRAY_SIZE(sonyimx135_rgbw_es2_framesize_preview)} },
	{0, 2, 4208, 3120, 4572, 3142, 10, 5, 0x15D, 0x123, 0xA0, VIEW_FULL, RESOLUTION_4_3, false, {sonyimx135_rgbw_es2_framesize_full, ARRAY_SIZE(sonyimx135_rgbw_es2_framesize_full)} },
};

/******************************************/

typedef struct _imx135_sensor_module{
	char  *sensor_name;
	u16    i2c_address_h;
	u16    i2c_address_l;
	u16   chip_id;
	u32   slave_addr;
	u8   cfa_fmt;
	struct _sensor_seq *preg;
	u32   reg_size;
	struct framesize_s *framesizes;
	u32   frame_size;
} imx135_sensor_module;

static imx135_sensor_module sonyimx135_sensor_module[] = {
	{"sonyimx135_liteon",  SONYIMX135_CHIP_ID_ES2_ADRESS_H,SONYIMX135_CHIP_ID_ES2_ADRESS_L,SONYIMX135_CHIP_ID_ES2, SONYIMX135_SLAVE_ADDRESS1, 0x80,
	     sonyimx135_rgb_init_regs, ARRAY_SIZE(sonyimx135_rgb_init_regs),sonyimx135_rgb_framesizes,ARRAY_SIZE(sonyimx135_rgb_framesizes)},
	{"sonyimx135_liteon",  SONYIMX135_CHIP_ID_ES2_ADRESS_H,SONYIMX135_CHIP_ID_ES2_ADRESS_L,SONYIMX135_CHIP_ID_ES2, SONYIMX135_SLAVE_ADDRESS2, 0x80,
	    sonyimx135_rgb_init_regs, ARRAY_SIZE(sonyimx135_rgb_init_regs),sonyimx135_rgb_framesizes,ARRAY_SIZE(sonyimx135_rgb_framesizes)},
	{"sonyimx135_rgbwes1", SONYIMX135_CHIP_ID_ES1_ADRESS_H,SONYIMX135_CHIP_ID_ES1_ADRESS_L,SONYIMX135_CHIP_ID_ES1, SONYIMX135_SLAVE_ADDRESS1, 0x00,
		sonyimx135_rgbw_es1_init_regs, ARRAY_SIZE(sonyimx135_rgbw_es1_init_regs),sonyimx135_rgbw_es1_framesizes,ARRAY_SIZE(sonyimx135_rgbw_es1_framesizes)},
	{"sonyimx135_rgbwes2", SONYIMX135_CHIP_ID_ES2_ADRESS_H,SONYIMX135_CHIP_ID_ES2_ADRESS_L,SONYIMX135_CHIP_ID_ES2, SONYIMX135_SLAVE_ADDRESS1, 0x00,
	      sonyimx135_rgbw_es2_init_regs, ARRAY_SIZE(sonyimx135_rgbw_es2_init_regs),sonyimx135_rgbw_es2_framesizes,ARRAY_SIZE(sonyimx135_rgbw_es2_framesizes)},};

static camera_sensor sonyimx135_sensor;
static void sonyimx135_set_default(void);
static void sonyimx135_otp_get_vcm(u16 *vcm_start, u16 *vcm_end);
static void sonyimx135_otp_update_afwb(void);
static void sonyimx135_otp_enable_lsc(bool enable);
static void sonyimx135_otp_record_lsc_func(struct work_struct *work);
struct work_struct record_otp_lsc_work;
static bool sonyimx135_otp_read_lsc(void);
static bool sonyimx135_otp_set_lsc(void);
#ifdef CONFIG_DEBUG_FS
extern u32 get_main_sensor_id(void);
#endif
/*
 **************************************************************************
 * FunctionName: sonyimx135_read_reg;
 * Description : read sonyimx135 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_read_reg(u16 reg, u8 *val)
{
	return k3_ispio_read_reg(sonyimx135_sensor.i2c_config.index,
				 sonyimx135_sensor.i2c_config.addr, reg, (u16 *)val, sonyimx135_sensor.i2c_config.val_bits);
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_write_reg;
 * Description : write sonyimx135 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_write_reg(u16 reg, u8 val, u8 mask)
{
	return k3_ispio_write_reg(sonyimx135_sensor.i2c_config.index,
			sonyimx135_sensor.i2c_config.addr, reg, val, sonyimx135_sensor.i2c_config.val_bits, mask);
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_write_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int sonyimx135_write_seq(const struct _sensor_reg_t *seq, u32 size, u8 mask)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	return k3_ispio_write_seq(sonyimx135_sensor.i2c_config.index,
			sonyimx135_sensor.i2c_config.addr, seq, size, sonyimx135_sensor.i2c_config.val_bits, mask);
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_write_isp_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void sonyimx135_write_isp_seq(const struct isp_reg_t *seq, u32 size)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	k3_ispio_write_isp_seq(seq, size);
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_write_isp_reg;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void sonyimx135_write_isp_reg(u32 reg_addr, u32 data, u32 size)
{
	struct isp_reg_t reg_seq[4];
	int i = 0;
	print_debug("Enter %s, size=%d", __func__, size);
	print_debug("data:%x", data);
	
	/*initialize buffer */
	for (i = 0; i < size; i++) {
		reg_seq[i].subaddr = reg_addr;
		reg_seq[i].value = data & 0xFF;
		reg_seq[i].mask = 0x00;
		reg_addr++;
		data = data >> 8;
		print_debug("reg_seq[%d].subaddr:%x", i, reg_seq[i].subaddr);
		print_debug("reg_seq[%d].value:%x", i, reg_seq[i].value);
	}
	
	/*write register of isp for imx135 */
	sonyimx135_write_isp_seq(reg_seq, size);
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_read_isp_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void sonyimx135_read_isp_seq(struct isp_reg_t *seq, u32 size)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	k3_ispio_read_isp_seq(seq, size);
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_read_isp_reg;
 * Description : Read ISP register;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static u32 sonyimx135_read_isp_reg(u32 reg_addr, u32 size)
{
	struct isp_reg_t reg_seq[4];
	int i = 0;
	u32 reg_value = 0x00;
	print_debug("Enter %s, size=%d", __func__, size);
	
	/*initialize buffer */
	for (i = 0; i < size; i++) {
		reg_seq[i].subaddr = reg_addr;
		reg_seq[i].value = 0x00;
		reg_seq[i].mask = 0x00;
		reg_addr++;
	}
	
	/*read register of isp for imx135 */
	sonyimx135_read_isp_seq(reg_seq, size);
	/*construct return value */
	do {
		i--;
		reg_value = reg_value << 8;
		reg_value |= reg_seq[i].value;
		print_debug("reg_seq[%d].value:%x", i, reg_seq[i].value);
	} while (i > 0);
	return reg_value;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_enum_frame_intervals;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_enum_frame_intervals(struct v4l2_frmivalenum *fi)
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
 * FunctionName: sonyimx135_get_format;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_get_format(struct v4l2_fmtdesc *fmt)
{
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_OVERLAY)
		fmt->pixelformat = sonyimx135_sensor.fmt[STATE_PREVIEW];
	else
		fmt->pixelformat = sonyimx135_sensor.fmt[STATE_CAPTURE];

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_enum_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_enum_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	u32 max_index = ARRAY_SIZE(camera_framesizes) - 1;
	u32 this_max_index =    sonyimx135_sensor_module[sensor_mode_index].frame_size -1;

	assert(framesizes);

	print_debug("enter %s; ", __func__);

	if (framesizes->index > max_index) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	if ((camera_framesizes[framesizes->index].width > sonyimx135_framesizes[this_max_index].width)
		|| (camera_framesizes[framesizes->index].height > sonyimx135_framesizes[this_max_index].height)) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	framesizes->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	framesizes->discrete.width = sonyimx135_framesizes[this_max_index].width;
	framesizes->discrete.height = sonyimx135_framesizes[this_max_index].height;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_try_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_try_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	int max_index = sonyimx135_sensor_module[sensor_mode_index].frame_size -1;

	assert(framesizes);

	print_debug("Enter Function:%s  ", __func__);


	if ((framesizes->discrete.width <= sonyimx135_framesizes[max_index].width)
	    && (framesizes->discrete.height <= sonyimx135_framesizes[max_index].height)) {
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
 * FunctionName: sonyimx135_set_framesizes;
 * Description : NA;
 * Input       : flag: if 1, set framesize to sensor,
 *					   if 0, only store framesize to camera_interface;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_set_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs, int flag, camera_setting_view_type view_type)
{
	int i = 0;
	bool match = false;
       int size = 0;
	assert(fs);

	print_info("Enter Function:%s State(%d), flag=%d, width=%d, height=%d",
		    __func__, state, flag, fs->width, fs->height);

        size = sonyimx135_sensor_module[sensor_mode_index].frame_size;

	if(HDR_MOVIE_ON == sonyimx135_sensor.hdrInfo.hdr_on){
		for (i = 0; i < size; i++) {
			if ((sonyimx135_framesizes[i].width >= fs->width)
			    && (sonyimx135_framesizes[i].height >= fs->height)
			    && (VIEW_HDR_MOVIE == sonyimx135_framesizes[i].view_type))
		     {
				fs->width = sonyimx135_framesizes[i].width;
				fs->height = sonyimx135_framesizes[i].height;
				match = true;
				break;
			}
		}
	}else {
			if (VIEW_FULL == view_type) {
				for (i = 0; i < size; i++) {
					if ((sonyimx135_framesizes[i].width >= fs->width)
					    && (sonyimx135_framesizes[i].height >= fs->height)
					    && (VIEW_FULL == sonyimx135_framesizes[i].view_type)
					    && (camera_get_resolution_type(fs->width, fs->height)
					    <= sonyimx135_framesizes[i].resolution_type)) {
						fs->width = sonyimx135_framesizes[i].width;
						fs->height = sonyimx135_framesizes[i].height;
						match = true;
						break;
					}
				}
			}
	}
	if (false == match) {
		for (i = 0; i < size; i++) {
			if ((sonyimx135_framesizes[i].width >= fs->width)
			    && (sonyimx135_framesizes[i].height >= fs->height)
			    && (camera_get_resolution_type(fs->width, fs->height)
			   <= sonyimx135_framesizes[i].resolution_type)
	    			&& (VIEW_HDR_MOVIE != sonyimx135_framesizes[i].view_type)) {
				fs->width = sonyimx135_framesizes[i].width;
				fs->height = sonyimx135_framesizes[i].height;
				break;
			}
		}
	}

	if (i >= size) {
		print_error("request resolution larger than sensor's max resolution");
		return -EINVAL;
	}

	if (state == STATE_PREVIEW)
		sonyimx135_sensor.preview_frmsize_index = i;
	else
		sonyimx135_sensor.capture_frmsize_index = i;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_get_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_get_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs)
{
	int frmsize_index;

	assert(fs);

	if (state == STATE_PREVIEW)
		frmsize_index = sonyimx135_sensor.preview_frmsize_index;
	else if (state == STATE_CAPTURE)
		frmsize_index = sonyimx135_sensor.capture_frmsize_index;
	else
		return -EINVAL;

	fs->width = sonyimx135_framesizes[frmsize_index].width;
	fs->height = sonyimx135_framesizes[frmsize_index].height;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_init_isp_reg;
 * Description : load initial seq for sensor init;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_init_isp_reg(void)
{
	int size = 0;


	if( HDR_MOVIE_ON== sonyimx135_sensor.hdrInfo.hdr_on)
	{
		size = ARRAY_SIZE(isp_init_regs_sonyimx135_HDR);
		sonyimx135_write_isp_seq(isp_init_regs_sonyimx135_HDR, size);
	}
	else
	{
		size = ARRAY_SIZE(isp_init_regs_sonyimx135);
		sonyimx135_write_isp_seq(isp_init_regs_sonyimx135, size);
	}

	return 0;
}
/*
 **************************************************************************
 * FunctionName: sonyimx135_support_hdr_movie;
 * Description : check sensor support hdr movie or not;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_support_hdr_movie(void)
{
	   return HDR_MOVIE_SUPPORT;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_get_hdr_movie_switch;
 * Description : the function that get hdr movie status on or off;
 * Input       : NA;
 * Output      : the status of hdr movie status;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_get_hdr_movie_switch(void)
{
	   return sonyimx135_sensor.hdrInfo.hdr_on;
}
/*
 **************************************************************************
 * FunctionName: sonyimx135_set_hdr_movie_switch;
 * Description : the function that set hdr movie status;
 * Input       : the status of hdr movie on or off;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx135_set_hdr_movie_switch(hdr_movie_switch on)
{
	print_info("Enter Function:%s on = %d",__func__,on);
	sonyimx135_sensor.hdrInfo.hdr_on = on;
}
/*
 **************************************************************************
 * FunctionName: sonyimx135_init_reg;
 * Description : download initial seq for sensor init;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_init_reg(void)
{
	int size = 0;
       struct _sensor_reg_t *sonyimx135_init_regs = sonyimx135_sensor_module[sensor_mode_index].preg;
	
	sonyimx135_init_isp_reg();

	if (0 != k3_ispio_init_csi(sonyimx135_sensor.mipi_index,
			sonyimx135_sensor.mipi_lane_count, sonyimx135_sensor.lane_clk)) {
		return -EFAULT;
	}

	size = sonyimx135_sensor_module[sensor_mode_index].reg_size;
	if (0 != sonyimx135_write_seq(sonyimx135_init_regs, size, 0x00)) {
		print_error("line %d, fail to init sonyimx135 sensor", __LINE__);
		return -EFAULT;
	}

	if(E_CAMERA_SENSOR_FLIP_TYPE_H_V == get_primary_sensor_flip_type()) {
		sonyimx135_write_reg(SONYIMX135_FLIP, 0x03, 0x00); //turn camera layout
	}
	
	sonyimx135_otp_enable_lsc(false);
	if ((sonyimx135_otp_flag & SONYIMX135_OTP_LSC_READ) || sonyimx135_otp_read_lsc()) {
		if (sonyimx135_otp_set_lsc()) {
			sonyimx135_otp_enable_lsc(true);
		}
	}
	print_info("%s, OTP flag = %#x", __func__, sonyimx135_otp_flag);
	if (!(sonyimx135_otp_flag & SONYIMX135_OTP_LSC_WRITED)) {
		schedule_work(&record_otp_lsc_work);
	}

	return 0;
}

static int sonyimx135_set_hflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);
	sonyimx135_sensor.hflip = flip;
	return 0;
}
static int sonyimx135_get_hflip(void)
{
	print_debug("enter %s", __func__);

	return sonyimx135_sensor.hflip;
}
static int sonyimx135_set_vflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);

	sonyimx135_sensor.vflip = flip;

	return 0;
}
static int sonyimx135_get_vflip(void)
{
	print_debug("enter %s", __func__);
	return sonyimx135_sensor.vflip;
}

static int sonyimx135_update_flip(u16 width, u16 height)
{
	u8 new_flip = ((sonyimx135_sensor.vflip << 1) | sonyimx135_sensor.hflip);
	print_debug("Enter %s  ", __func__);
	if (sonyimx135_sensor.old_flip != new_flip) {
		k3_ispio_update_flip((sonyimx135_sensor.old_flip ^ new_flip) & 0x03, width, height, PIXEL_ORDER_CHANGED);

		sonyimx135_sensor.old_flip = new_flip;
		if(E_CAMERA_SENSOR_FLIP_TYPE_H_V == get_primary_sensor_flip_type()) 
		{
			sonyimx135_write_reg(SONYIMX135_FLIP, sonyimx135_sensor.vflip ? 0x00 : 0x02, ~0x02);
			sonyimx135_write_reg(SONYIMX135_FLIP, sonyimx135_sensor.hflip ? 0x00 : 0x01, ~0x01);
		}
		else
		{
			sonyimx135_write_reg(SONYIMX135_FLIP, sonyimx135_sensor.vflip ? 0x02 : 0x00, ~0x02);
			sonyimx135_write_reg(SONYIMX135_FLIP, sonyimx135_sensor.hflip ? 0x01 : 0x00, ~0x01);
		}
		msleep(200);
	}
	//msleep(200);    //OLD  zhoupengtao move for cut the 13M camera capture Interval
	return 0;
}

static int sonyimx135_get_capability(u32 id, u32 *value)
{
	int i;
	for (i = 0; i < sizeof(sonyimx135_cap) / sizeof(sonyimx135_cap[0]); ++i) {
		if (id == sonyimx135_cap[i].id) {
			*value = sonyimx135_cap[i].value;
			break;
		}
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_framesize_switch;
 * Description : switch frame size, used by preview and capture
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_framesize_switch(camera_state state)
{
	u32 size = 0;
	u8 next_frmsize_index;


	if (state == STATE_PREVIEW)
		next_frmsize_index = sonyimx135_sensor.preview_frmsize_index;
	else
		next_frmsize_index = sonyimx135_sensor.capture_frmsize_index;

	print_debug("Enter Function:%s frm index=%d", __func__, next_frmsize_index);

    size = sonyimx135_sensor_module[sensor_mode_index].frame_size;
	if (next_frmsize_index >= size) {
		print_error("Unsupport sensor setting index: %d", next_frmsize_index);
		return -ETIME;
	}

	if (0 != sonyimx135_write_seq(sonyimx135_sensor.frmsize_list[next_frmsize_index].sensor_setting.setting
		, sonyimx135_sensor.frmsize_list[next_frmsize_index].sensor_setting.seq_size, 0x00)) {
		print_error("fail to init sonyimx135 sensor");
		return -ETIME;
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_stream_on;
 * Description : download preview seq for sensor preview;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_stream_on(camera_state state)
{
	print_debug("Enter Function:%s ", __func__);
	return sonyimx135_framesize_switch(state);
}

/****************************************************************************
* FunctionName: sonyimx135_open_otp_page;
* Description : Open selected OTP page.;
* Input       : page;
* Output      : NA;
* ReturnValue : bool;
* Other       : NA;
***************************************************************************/
static bool sonyimx135_open_otp_page(u8 page)
{
	int loop = 0;
	u8 status = 0;
	
	sonyimx135_write_reg(SONYIMX135_OTP_PAGE_SELECT_REG, page, 0x00); /* Select OTP page. */
	sonyimx135_write_reg(SONYIMX135_OTP_MODE_ENABLE_REG, 0x01, 0x00); /* Turn on OTP read mode. */
	udelay(2);

	/* Check OTP read ready status. */
	for (loop=0; loop<5; loop++) {
		sonyimx135_read_reg(SONYIMX135_OTP_STATUS_REG, &status);
		status &= 0x01;
		udelay(1);
		if(status)
			break;
	}
	if (!status) {
		print_error("%s: Wait OTP page%d read ready status timeout.", __func__, page);
		return false;
	}

	return true;
}

/*
 **************************************************************************
 * FunctionName: ov8830_reset_i2c;
 * Description : reset i2c address to isp;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx135_reset_i2c(struct i2c_t *i2c)
{
	k3_ispio_reset_i2c(i2c);
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_find_sensor;
 * Description :    The function is that finding valid sensor by use of  I2c address , camera id register, cfa_fmt register which 
 *                       disinguish rgbw and rgb
 * Input         : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */

static int sonyimx135_find_sensor(imx135_sensor_module *module)
{
	u8  read_fmt = 0;
	u16 read_id=0,read_idl=0,read_idh=0;

	sonyimx135_sensor.i2c_config.addr = module->slave_addr;
	sonyimx135_reset_i2c(&sonyimx135_sensor.i2c_config);
	sonyimx135_read_reg(module->i2c_address_h, &read_idh);
	sonyimx135_read_reg(module->i2c_address_l, &read_idl);
	read_id = ((read_idh << 8) | read_idl);

	if ( module->chip_id == read_id) {
		if(!sonyimx135_open_otp_page(0)){
			print_error("%s cannot access module otp",__func__);
			return IMX135_UNKOWN;
		}

		sonyimx135_read_reg(SONYIMX135_OTP_CFA_FMT_REG, &read_fmt);
		if( (module->cfa_fmt&0x80) != (read_fmt&0x80) ){
			print_info("%s not match:module=%s,module_fmt=%x,read_fmt=%x",
				__func__,module->sensor_name,module->cfa_fmt,read_fmt);
			return IMX135_UNKOWN;
		}

		snprintf(sonyimx135_sensor.info.name, sizeof(sonyimx135_sensor.info.name),"%s",module->sensor_name);
		return IMX135_FOUND;
	}
	else{
	      print_info("%s not match:module=%s,read_id=0x%x",__func__,module->sensor_name,read_id);
	      return IMX135_UNKOWN;
	}
}


/*  **************************************************************************
* FunctionName: sonyimx135_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static int sonyimx135_check_sensor(void)
{
	u8 idl = 0x1;
	u8 idh = 0x1;
	u16 id = 0;

	int ret = -1;
	u8 val = 1;

	ret = sonyimx135_read_reg(0x0000, &idh);
	ret = sonyimx135_read_reg(0x0001, &idl);
	
#if 0
	ret = sonyimx135_read_reg(0x0002, &val);
	print_info("%s: read 0x0002, ret=%d, val=%d.", __func__, ret, val);

	ret = sonyimx135_read_reg(0x0004, &val);
	print_info("%s: read 0x0004, ret=%d, val=%d.", __func__, ret, val);

	sonyimx135_write_reg(SONYIMX135_FLIP, 0x3,  0);
	ret = sonyimx135_read_reg(SONYIMX135_FLIP, &val);
	print_info("%s: read 0x0101, ret=%d, val=%d.", __func__, ret, val);
#endif
	id = ((idh << 8) | idl);
	print_info("sonyimx135 product id:0x%x", id);
#ifdef CONFIG_DEBUG_FS
	if (SONYIMX135_CHIP_ID != id) {
		id = (u16)get_main_sensor_id();
		print_info("sonyimx135 debugfs product id:0x%x", id);
	}
#endif
	if (SONYIMX135_CHIP_ID != id) {
		print_error("Invalid product id ,Could not load sensor sonyimx135");
		return -ENODEV;
	}

	sonyimx135_sensor.vcm = &vcm_dw9714;

	return 0;
}

/*  **************************************************************************
* FunctionName: sonyimx135_check_sensor_new;
* Description :   The function work for distinguishing all kinds of sensor,include rgbw and rgb
*                       , I2c adress  different camera id;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static int sonyimx135_check_sensor_new(void)
{
	int size;
	int i;
       int ret = IMX135_UNKOWN;
	print_debug("enter %s", __func__);
	size = sizeof(sonyimx135_sensor_module) / sizeof(sonyimx135_sensor_module[0]);
      	msleep(20);
	for (i = 0; i < size; i++) {
		
		ret = sonyimx135_find_sensor( &(sonyimx135_sensor_module[i]) );
		if(ret != IMX135_UNKOWN)
		{
		       sensor_mode_index = i;
                     sonyimx135_framesizes = sonyimx135_sensor_module[i].framesizes;
			sonyimx135_sensor.frmsize_list = sonyimx135_sensor_module[i].framesizes;
			if(!sonyimx135_framesizes)
			{
                           print_error("pointer of sonyimx135_framesizes is Null.");
		             return -ENODEV;
			}
                     break;
		}
	}
       
	if (ret == IMX135_UNKOWN) {
		print_error("Invalid product id ,Could not load sensor sonyimx135");
		return -ENODEV;
	} else {
		print_info("sonyimx135 module index:%d,sensor name =%s",
			sensor_mode_index,
			sonyimx135_sensor_module[sensor_mode_index].sensor_name);
	}
	sonyimx135_sensor.vcm = &vcm_dw9714;
	sonyimx135_sensor.vcm->get_vcm_otp = sonyimx135_otp_get_vcm;
	sonyimx135_otp_update_afwb();
	print_info("%s, OTP flag = %#x", __func__, sonyimx135_otp_flag);

	return 0;
}

/****************************************************************************
* FunctionName: sonyimx135_power;
* Description : NA;
* Input       : power state of camera;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
int sonyimx135_power(camera_power_state power)
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
		k3_ispgpio_power_sensor(&sonyimx135_sensor, power);
		k3_ispio_ioconfig(&sonyimx135_sensor, power);
		msleep(2);
	} else {
		k3_ispio_deinit_csi(sonyimx135_sensor.mipi_index);
		k3_ispio_ioconfig(&sonyimx135_sensor, power);
		k3_ispgpio_power_sensor(&sonyimx135_sensor, power);
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
u32 sonyimx135_gain_to_iso(int gain)
{
	return (gain * 100) / 0x10;
}

u32 sonyimx135_iso_to_gain(int iso)
{
	return (iso * 0x10) / 100;
}

void sonyimx135_set_gain(u32 gain)
{
	if (gain == 0)
		return;
	gain = 256 - (256 * 16) / gain;
	/*sonyimx135_write_reg(SONYIMX135_GAIN_REG_H, (gain >> 8) & 0xff, 0x00);*/
	sonyimx135_write_reg(SONYIMX135_GAIN_REG_L, gain & 0xff, 0x00);
}

static int sonyimx135_get_af_param(camera_af_param_t type)
{
	int ret;

	switch(type){
	case AF_MIN_HEIGHT_RATIO:
		ret = SONYIMX135_AF_MIN_HEIGHT_RATIO;
		break;
	case AF_MAX_FOCUS_STEP:
		ret = SONYIMX135_AF_MAX_FOCUS_STEP;
		break;
	case AF_GSENSOR_INTERVAL_THRESHOLD:
		ret = SONYIMX135_AF_GSENSOR_INTERVAL_THRESHOLD;
		break;
	case AF_WIDTH_PERCENT:
		ret = SONYIMX135_AF_WIDTH_PERCENT;
		break;
	case AF_HEIGHT_PERCENT:
		ret = SONYIMX135_AF_HEIGHT_PERCENT;
		break;
	default:
		print_error("%s:invalid argument",__func__);
		ret = -1;
		break;
	}

	return ret;
}

void sonyimx135_set_exposure(u32 exposure)
{
	exposure >>= 4;
	sonyimx135_write_reg(SONYIMX135_EXPOSURE_REG_H, (exposure >> 8) & 0xff, 0x00);
	sonyimx135_write_reg(SONYIMX135_EXPOSURE_REG_L, exposure & 0xff, 0x00);
}

/****************************************************************************
* FunctionName: sonyimx135_set_awb_gain;
* Description : NA;
* Input       : R,GR,GB,B gain from isp;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
void sonyimx135_set_awb_gain(u16 b_gain, u16 gb_gain, u16 gr_gain, u16 r_gain)
{

	sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold

	sonyimx135_write_reg(SONYIMX135_ABS_GAIN_B_H, (b_gain >> 8) & 0xff, 0x00);
	sonyimx135_write_reg(SONYIMX135_ABS_GAIN_B_L, b_gain & 0xff, 0x00);
	sonyimx135_write_reg(SONYIMX135_ABS_GAIN_GB_H, (gb_gain >> 8) & 0xff, 0x00);
	sonyimx135_write_reg(SONYIMX135_ABS_GAIN_GB_L, gb_gain & 0xff, 0x00);
	sonyimx135_write_reg(SONYIMX135_ABS_GAIN_GR_H, (gr_gain >> 8) & 0xff, 0x00);
	sonyimx135_write_reg(SONYIMX135_ABS_GAIN_GR_L, gr_gain & 0xff, 0x00);
	sonyimx135_write_reg(SONYIMX135_ABS_GAIN_R_H, (r_gain >> 8) & 0xff, 0x00);
	sonyimx135_write_reg(SONYIMX135_ABS_GAIN_R_L, r_gain & 0xff, 0x00);

	sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group open
}
void sonyimx135_set_vts(u16 vts)
{
	print_debug("Enter %s  ", __func__);
	sonyimx135_write_reg(SONYIMX135_VTS_REG_H, (vts >> 8) & 0xff, 0x00);
	sonyimx135_write_reg(SONYIMX135_VTS_REG_L, vts & 0xff, 0x00);
}

u32 sonyimx135_get_vts_reg_addr(void)
{
	return SONYIMX135_VTS_REG_H;
}

static sensor_reg_t* sonyimx135_construct_vts_reg_buf(u16 vts, u16 *psize)
{
	static sensor_reg_t sonyimx135_vts_regs[] = {
		{SONYIMX135_VTS_REG_H, 0x00},
		{SONYIMX135_VTS_REG_L, 0x00},
	};

	print_debug("Enter %s,vts=%u", __func__, vts);
	sonyimx135_vts_regs[0].value = (vts >> 8) & 0xff ;
	sonyimx135_vts_regs[1].value = vts & 0xff;

	*psize = ARRAY_SIZE(sonyimx135_vts_regs);
	return sonyimx135_vts_regs;
}
/*
 **************************************************************************
 * FunctionName: sonyimx135_read_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_read_seq(struct _sensor_reg_t *seq, u32 size)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	return k3_ispio_read_seq(sonyimx135_sensor.i2c_config.index,
			sonyimx135_sensor.i2c_config.addr, seq, size, sonyimx135_sensor.i2c_config.val_bits);
}
/*
 **************************************************************************
 * FunctionName: sonyimx135_get_hdr_y;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_get_lux_matrix(struct _sensor_reg_t *buf,lux_stat_matrix_tbl *lux_tbl )
{
	u32 size = sizeof(sonyimx135_hdr_regs_y)/sizeof(sonyimx135_hdr_regs_y[0]);
	u16 y = 0;
	int i ,j;
	u16 y_l = 0;
	u16 y_h = 0;
	assert(buf);
	assert(lux_tbl);
	sonyimx135_read_seq(buf,size);

	lux_tbl->size = size/2;
	sonyimx135_sensor.lux_matrix.size = lux_tbl->size;
	for (i = 0,j = 0; i< size;i+=2,j++)
	{
		y_h = sonyimx135_hdr_regs_y[i].value & 0xff;
		y_l = sonyimx135_hdr_regs_y[i+1].value;
		y   = y_h;
		y   = y<<8|y_l;
		lux_tbl->matrix_table[j] = y;
		sonyimx135_sensor.lux_matrix.matrix_table[j] = y;
		print_debug("j = %d,every y = 0x%d",j,lux_tbl->matrix_table[j]);
	}
	return 0;
}
/*
 **************************************************************************
 * FunctionName: sonyimx135_get_gain;
 * Description :get gain from sensor
 * Input       	 NA;
 * Output      	 gain value from sensor
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
u32 sonyimx135_get_gain(void)
{
	u8 gain_l = 0;

	u8 short_gain = 0;
	u32 gain = 0;
	sonyimx135_read_reg(SONYIMX135_GAIN_REG_L, &gain_l);

	sonyimx135_read_reg(WD_ANA_GAIN_DS, &short_gain);
	//gain = (gain_h << 8) | gain_l;
	gain = gain_l;

	if(_IS_DEBUG_AE)
	{
		print_info("%s-0x0205=%x",__func__,gain_l);
		print_info("%s-0x0233=%x",__func__,short_gain);
	}
	return gain;
}
/*
 **************************************************************************
 * FunctionName: sonyimx135_get_exposure;
 * Description :get exposure from sensor
 * Input       	 NA;
 * Output      	 exposure value from sensor
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
u32  sonyimx135_get_exposure(void)
{
	u32 expo = 0;
	u8 expo_h = 0;
	u8 expo_l = 0;
	u8 expo_short_h = 0;
	u8 expo_shot_l = 0;
	sonyimx135_read_reg(SONYIMX135_EXPOSURE_REG_H, &expo_h);
	sonyimx135_read_reg(SONYIMX135_EXPOSURE_REG_L, &expo_l);

	sonyimx135_read_reg(WD_COARSE_INTEG_TIME_DS_H, &expo_short_h);
	sonyimx135_read_reg(WD_COARSE_INTEG_TIME_DS_L, &expo_shot_l);

	if(_IS_DEBUG_AE)
	{
		print_info("sonyimx135_get_exposure-0x0202=%x,0x0203=%x",expo_h,expo_l);
		print_info("sonyimx135_get_exposure-0x0230=%x,0x0231=%x",expo_short_h,expo_shot_l);
	}
	expo = expo_h &0xff;
	expo = expo << 8 | expo_l;
	return expo;
}
/*
 **************************************************************************
 * FunctionName: sonyimx135_set_hdr_exposure_gain;
 * Description 	   : set exposure and gain to sensor.
 * Input		   : the value of exposure and gain ,that include long exposure short exposure and long gain db ,short gain db
 		          :  gain is db but not times
 * Output      	   : NA
 * ReturnValue   : NA;
 * Other       	   : NA;
 **************************************************************************
*/
void sonyimx135_set_hdr_exposure_gain(u32 expo_long,u16 gain_global,u32 expo_short,u16 short_gain)
{
       u16 gain_switch_long = 0;
	u16 gain_switch_short = 0;

	gain_switch_long = gain_global;
	gain_switch_short = short_gain;
	if(_IS_DEBUG_AE)
	{
	       print_info("__debug_esen: Enter %s  gain_switch_long = %d ", __func__,gain_switch_long);
	       print_info("__debug_esen: Enter %s  expo_long = %d ", __func__,expo_long);
		print_info("__debug_esen: Enter %s  gain_switch_short = %d ", __func__,gain_switch_short);
	       print_info("__debug_esen: Enter %s  expo_short = %d", __func__,expo_short);
	}

	sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
	sonyimx135_write_reg(SONYIMX135_EXPOSURE_REG_H, (expo_long >> 8) & 0xff, 0x00);
	sonyimx135_write_reg(SONYIMX135_EXPOSURE_REG_L, expo_long & 0xff, 0x00);
	sonyimx135_write_reg(WD_COARSE_INTEG_TIME_DS_H, (expo_short >> 8) & 0xff, 0x00);
	sonyimx135_write_reg(WD_COARSE_INTEG_TIME_DS_L, expo_short & 0xff, 0x00);
	sonyimx135_write_reg(SONYIMX135_GAIN_REG_L, gain_switch_long, 0x00);
	sonyimx135_write_reg(WD_ANA_GAIN_DS, gain_switch_short, 0x00);
	sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release


}

/*
 **************************************************************************
 * FunctionName: sonyimx135_set_ATR_ctrl_points;
 * Description 	   : modify hdr atr curve with parameter inputed atr_ctrl_points .
 * Input		   : ATR control points parameter atr_ctrl_points
 * Output      	   : NA
 * ReturnValue   : NA;
 * Other       	   : NA;
 **************************************************************************
*/
void sonyimx135_set_ATR_ctrl_points(atr_ctrl_points * in_atr_ctr_pints)
{
 	print_debug("enter %s",__func__);
 	assert(in_atr_ctr_pints);

	sonyimx135_sensor.hdr_points.tc_out_sat = in_atr_ctr_pints->tc_out_sat;
	sonyimx135_sensor.hdr_points.tc_out_mid = in_atr_ctr_pints->tc_out_mid;
	sonyimx135_sensor.hdr_points.tc_out_noise = in_atr_ctr_pints->tc_out_noise;
	sonyimx135_sensor.hdr_points.tc_sat_brate = in_atr_ctr_pints->tc_sat_brate;
	sonyimx135_sensor.hdr_points.tc_mid_brate= in_atr_ctr_pints->tc_mid_brate;
	sonyimx135_sensor.hdr_points.tc_noise_brate= in_atr_ctr_pints->tc_noise_brate;
	sonyimx135_sensor.hdr_points.ae_sat= in_atr_ctr_pints->ae_sat;
	sonyimx135_sensor.hdr_points.wb_lmt= in_atr_ctr_pints->wb_lmt;

	sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
	sonyimx135_write_reg(ATR_OUT_NOISE_REG_H, (in_atr_ctr_pints->tc_out_noise >> 8) & 0x03f, 0x00);
	sonyimx135_write_reg(ATR_OUT_NOISE_REG_L, in_atr_ctr_pints->tc_out_noise & 0xff, 0x00);
	sonyimx135_write_reg(ATR_OUT_MID_REG_H, (in_atr_ctr_pints->tc_out_mid >> 8) & 0x03f, 0x00);
	sonyimx135_write_reg(ATR_OUT_MID_REG_L, in_atr_ctr_pints->tc_out_mid & 0xff, 0x00);
	sonyimx135_write_reg(ATR_OUT_SAT_REG_H, (in_atr_ctr_pints->tc_out_sat >> 8) & 0x03f, 0x00);
	sonyimx135_write_reg(ATR_OUT_SAT_REG_L, in_atr_ctr_pints->tc_out_sat & 0xff, 0x00);
	sonyimx135_write_reg(ATR_NOISE_BRATE_REG, in_atr_ctr_pints->tc_noise_brate& 0x03f, 0x00);
	sonyimx135_write_reg(ATR_MID_BRATE_REG, in_atr_ctr_pints->tc_mid_brate & 0xff, 0x00);
	sonyimx135_write_reg(ATR_SAT_BRATE_REG, in_atr_ctr_pints->tc_sat_brate & 0xff, 0x00);
#if 0
	sonyimx135_write_reg(WB_LMT_REG_H, (in_atr_ctr_pints->ae_sat >> 8) & 0xff,0x00);
	sonyimx135_write_reg(WB_LMT_REG_L, in_atr_ctr_pints->ae_sat & 0xff,0x00);
	sonyimx135_write_reg(AE_SAT_REG_H, (in_atr_ctr_pints->wb_lmt >> 8) & 0xff,0x00);
	sonyimx135_write_reg(AE_SAT_REG_L, in_atr_ctr_pints->wb_lmt & 0xff,0x00);
#endif
	sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release
}
/*
 **************************************************************************
 * FunctionName: sonyimx135_get_ATR_ctrl_points;
 * Description 	   : get ATR control points info with atr_ctrl_points struct.
 * Input		   : the storage space of atr_ctrl_points
 * Output      	   : NA
 * ReturnValue   : NA;
 * Other       	   : NA;
 **************************************************************************
*/
void sonyimx135_get_ATR_ctrl_points(atr_ctrl_points * in_atr_ctr_pints)
{
	u8 tc_out_noise_h = 0;
	u8 tc_out_noise_l = 0;
	u8 tc_out_mid_h = 0;
	u8 tc_out_mid_l = 0;
	u8 tc_out_sat_h = 0;
	u8 tc_out_sat_l = 0;
	u8 tc_out_noise = 0;
	u8 tc_out_mid = 0;
	u8 tc_out_sat = 0;
	 print_debug("enter %s",__func__);
	 assert(in_atr_ctr_pints);
	sonyimx135_read_reg(ATR_OUT_NOISE_REG_H, &tc_out_noise_h);
	sonyimx135_read_reg(ATR_OUT_NOISE_REG_L, &tc_out_noise_l);
	sonyimx135_read_reg(ATR_OUT_MID_REG_H, &tc_out_noise_h);
	sonyimx135_read_reg(ATR_OUT_MID_REG_L, &tc_out_mid_l);
	sonyimx135_read_reg(ATR_OUT_SAT_REG_H, &tc_out_sat_h);
	sonyimx135_read_reg(ATR_OUT_SAT_REG_L, &tc_out_sat_l);
	tc_out_noise = tc_out_noise_h;
	tc_out_noise = (tc_out_noise << 8)|tc_out_noise_l;
	tc_out_mid = tc_out_mid_h;
	tc_out_mid = (tc_out_mid << 8)|tc_out_mid_l;
	tc_out_sat = tc_out_sat_h;
	tc_out_sat = (tc_out_sat << 8)|tc_out_sat_l;
	in_atr_ctr_pints->tc_out_noise = tc_out_noise;
	in_atr_ctr_pints->tc_out_mid = tc_out_mid;
	in_atr_ctr_pints->tc_out_sat = tc_out_sat;
}
/****************************************************************************
* FunctionName: sonyimx135_otp_lsc_file_exist;
* Description : Check if LSC parameter file exist;
* Input       : NA;
* Output      : NA;
* ReturnValue : bool;
* Other       : NA;
***************************************************************************/
static bool sonyimx135_otp_lsc_file_exist(void)
{
	struct file *filp = NULL;

	filp = filp_open(SONYIMX135_OTP_LSC_FILE, O_RDONLY, 0666);
	if (!IS_ERR_OR_NULL(filp)) {
		filp_close(filp, NULL);
		return true;
	}
	print_info("%s, OTP file is not exist.", __func__);
	
	return false;
}

/****************************************************************************
* FunctionName: sonyimx135_otp_read_file;
* Description : Read OTP LSC parameters to file;
* Input       : NA;
* Output      : NA;
* ReturnValue : bool;
* Other       : NA;
***************************************************************************/
static bool sonyimx135_otp_read_file(void)
{
	bool ret = true;
	mm_segment_t fs;
	struct file *filp = NULL;

	print_debug("enter %s", __func__);
	filp = filp_open(SONYIMX135_OTP_LSC_FILE, O_RDONLY, 0666);
	if (IS_ERR_OR_NULL(filp)) {
		print_error("%s, failed to open file!", __func__);
		ret = false;
		goto ERROR;
	}
	fs = get_fs();
	set_fs(KERNEL_DS);
	if (SONYIMX135_OTP_LSC_SIZE != vfs_read(filp, sonyimx135_otp_lsc_param, SONYIMX135_OTP_LSC_SIZE, &filp->f_pos)) {
		set_fs(fs);
		ret = false;
		print_error("%s, read file error!", __func__);
		goto ERROR;
	}
	set_fs(fs);
	print_info("%s, read OTP file OK.", __func__);
	
ERROR:
	if (NULL != filp) {
		filp_close(filp, NULL);
	}

	if (ret) {
		sonyimx135_otp_flag |= SONYIMX135_OTP_LSC_READ;
		sonyimx135_otp_flag |= SONYIMX135_OTP_LSC_WRITED;
	} else {
		sonyimx135_otp_flag |= SONYIMX135_OTP_LSC_FILE_ERR;
	}

	return ret;
}

/****************************************************************************
* FunctionName: sonyimx135_otp_write_file;
* Description : Write OTP LSC parameters to file;
* Input       : NA;
* Output      : NA;
* ReturnValue : bool;
* Other       : NA;
***************************************************************************/
static bool sonyimx135_otp_write_file(void)
{
	bool ret = true;
	mm_segment_t fs;
	struct file *filp = NULL;
	
	print_debug("enter %s", __func__);
	filp = filp_open(SONYIMX135_OTP_LSC_FILE, O_CREAT|O_WRONLY, 0666);
	if (IS_ERR_OR_NULL(filp)) {
		ret = false;
		print_error("%s, OTP failed to open file!", __func__);
		goto ERROR;
	}
	fs = get_fs();
	set_fs(KERNEL_DS);
	if (SONYIMX135_OTP_LSC_SIZE != vfs_write(filp, sonyimx135_otp_lsc_param, SONYIMX135_OTP_LSC_SIZE, &filp->f_pos)) {
		set_fs(fs);
		ret = false;
		print_error("%s, OTP write file error!", __func__);
		goto ERROR;
	}
	set_fs(fs);
	print_info("%s, write OTP file OK.", __func__);
	
ERROR:
	if (NULL != filp) {
		filp_close(filp, NULL);
	}

	if (ret) {
		sonyimx135_otp_flag |= SONYIMX135_OTP_LSC_WRITED;
		sonyimx135_otp_flag &= ~SONYIMX135_OTP_LSC_FILE_ERR;
	}

	return ret;
}

/****************************************************************************
* FunctionName: sonyimx135_otp_check_page;
* Description : Check which page this parameters belong to.;
* Input       : otp_type;
* Output      : NA;
* ReturnValue : bool;
* Other       : NA;
***************************************************************************/
static bool sonyimx135_otp_check_page(otp_type type)
{
	int index = 0;
	int start = 0;
	int end = 0;
	u8 page = 0;
	u8 val1 = 0;
	u8 val2 = 0;
	
	print_debug("enter %s, OTP type is %d", __func__, type);

	switch (type) {
		case OTP_ID_WB:
			/* Module ID and WB parameters take up Page9~Page12, and can be burned 4 times.*/
			start = OTP_PAGE12;
			end = OTP_PAGE9;
			page = OTP_PAGE12;
			break;
		case OTP_VCM:
			/* AF VCM parameters take up Page13~Page16, and can be burned 4 times. */
			start = OTP_PAGE16;
			end = OTP_PAGE13;
			page = OTP_PAGE16;
			break;
		default:
			print_error("%s: Unsupported OTP type %d.", __func__, type);
			return false;
	}

	/* Find out valid OTP page. */
	for (index=start; index>=end; index--) {
		if (sonyimx135_open_otp_page(page)) {
			/* Check whether current page is valid. */
			sonyimx135_read_reg(0x3B05, &val1);
			sonyimx135_read_reg(0x3B07, &val2);
			if (val1 | val2) {
				break;
			}
		}
		page--;
	}
	if (index < end) {
		print_error("%s: There is no OTP data written.", __func__);
		return false;
	}

	return true;
}

/****************************************************************************
* FunctionName: sonyimx135_otp_read_id_wb;
* Description : Get module ID and WB parameters form OTP.;
* Input       : otp_id_wb_t;
* Output      : NA;
* ReturnValue : bool;
* Other       : NA;
***************************************************************************/
static bool sonyimx135_otp_read_id_wb()
{
	print_debug("enter %s", __func__);

	if (!sonyimx135_otp_check_page(OTP_ID_WB)) {
		print_error("%s: Check OTP module info and WB parameters page failed.", __func__);
		return false;
	}

	/* Read module information */
	sonyimx135_read_reg(0x3B04, &sonyimx135_otp_id_wb.year);
	sonyimx135_read_reg(0x3B05, &sonyimx135_otp_id_wb.month);
	sonyimx135_read_reg(0x3B06, &sonyimx135_otp_id_wb.day);
	sonyimx135_read_reg(0x3B07, &sonyimx135_otp_id_wb.cam_code);
	sonyimx135_read_reg(0x3B08, &sonyimx135_otp_id_wb.vend_ver);
	print_debug("%s: Module yield date: %x-%x-%x", __func__, 
		sonyimx135_otp_id_wb.year, sonyimx135_otp_id_wb.month, sonyimx135_otp_id_wb.day);
	print_debug("%s: Module code is: %#x, vendor and version info is %#x: ", 
		__func__, sonyimx135_otp_id_wb.cam_code, sonyimx135_otp_id_wb.vend_ver);

    /* Read WB parameters */
	sonyimx135_read_reg(0x3B09, &sonyimx135_otp_id_wb.rg_h);
	sonyimx135_read_reg(0x3B0A, &sonyimx135_otp_id_wb.rg_l);
	sonyimx135_read_reg(0x3B0B, &sonyimx135_otp_id_wb.bg_h);
	sonyimx135_read_reg(0x3B0C, &sonyimx135_otp_id_wb.bg_l);
	sonyimx135_read_reg(0x3B0D, &sonyimx135_otp_id_wb.gbgr_h);
	sonyimx135_read_reg(0x3B0E, &sonyimx135_otp_id_wb.gbgr_l);

	/* Read lens shading ID */
	sonyimx135_read_reg(0x3B0F, &sonyimx135_otp_lsc_id);

	/* OTP module info and WB param are read  */
	sonyimx135_otp_flag |= SONYIMX135_OTP_ID_WB_READ;

	return true;
}

/****************************************************************************
* FunctionName: sonyimx135_otp_set_id;
* Description : Set module ID information.;
* Input       : otp_id_wb_t;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static void sonyimx135_otp_set_id()
{	
	print_debug("enter %s", __func__);
}

/****************************************************************************
* FunctionName: sonyimx135_otp_set_wb;
* Description : Set WB parameters to ISP.;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static void sonyimx135_otp_set_wb()
{
	print_debug("enter %s", __func__);
}

/****************************************************************************
* FunctionName: sonyimx135_otp_read_lsc;
* Description : Read lens shading parameters form OTP.;
* Input       : NA;
* Output      : NA;
* ReturnValue : bool;
* Other       : NA;
***************************************************************************/
static bool sonyimx135_otp_read_lsc(void)
{
	u8 *pval = NULL;
	u8 page = 0;
	u16 addr_start = 0;
	u16 addr_end = 0;
	
	print_debug("enter %s", __func__);

	/* If lens shading ID is not read, then read it first. */
	if (0x00 == sonyimx135_otp_lsc_id) {
		if (sonyimx135_otp_check_page(OTP_ID_WB)) {
			sonyimx135_read_reg(0x3B0F, &sonyimx135_otp_lsc_id);
		} else {
			print_error("%s: Read lens shading ID failed.", __func__);
			return false;
		}
	}

	/* Lens shading burned OK. */
	if (0xFF == sonyimx135_otp_lsc_id) {
		if (sonyimx135_otp_lsc_file_exist() && sonyimx135_otp_read_file()) {
			/* LSC param is read  */
			print_info("%s, read OTP LSC form file.", __func__);
			return true;
		}
		
		pval = &sonyimx135_otp_lsc_param[0];
		memset(sonyimx135_otp_lsc_param, 0, SONYIMX135_OTP_LSC_SIZE);

		/* Lens shading parameters take up Page1~Page8 of OTP, and can be burned only once. */
		for (page=OTP_PAGE1; page<=OTP_PAGE8; page++) {
			if (sonyimx135_open_otp_page(page)) {
				/* Get lens shading parameters from each page. */
				addr_start = 0x3B04;
				if (OTP_PAGE8 == page) {
					addr_end = 0x3B3B; /* In page8, the valid data range is 0x3B04~0x3B3B. */
				} else {
					addr_end = 0x3B43; /* From page1 to page7, the valid data range is 0x3B04~0x3B43. */
				}
				do {
					sonyimx135_read_reg(addr_start, pval);
					addr_start++;
					pval++;
				} while (addr_start <= addr_end);
			}
		}
		print_info("%s, read OTP LSC from sensor OK.", __func__);
	}

	/* LSC param is read  */
	sonyimx135_otp_flag |= SONYIMX135_OTP_LSC_READ;

	return true;
}

/****************************************************************************
* FunctionName: sonyimx135_otp_set_lsc;
* Description : Set lens shading parameters to sensor registers.;
* Input       : NA;
* Output      : NA;
* ReturnValue : bool;
* Other       : NA;
***************************************************************************/
static bool sonyimx135_otp_set_lsc(void)
{
	u8 *pval = NULL;
	int i = 0;

	print_debug("enter %s", __func__);

	/* Lens shading parameters are burned OK. */
	if (0xFF == sonyimx135_otp_lsc_id) {
		/* Get parameters from static array, which read from OTP. */
		pval = sonyimx135_otp_lsc_param;
	} else {
		print_error("%s: Unsupported lens shading ID, %#x", __func__, sonyimx135_otp_lsc_id);
		return false;
	}
	
	/* Write lens shading parameters to sensor registers. */
	for (i=0; i<SONYIMX135_OTP_LSC_SIZE; i++) {
		sonyimx135_write_reg(SONYIMX135_SENSOR_LSC_RAM+i, *(pval+i), 0x00);
	}
    print_info("%s, set OTP LSC to sensor OK.", __func__);

	return true;
}

/****************************************************************************
* FunctionName: sonyimx135_otp_enable_lsc;
* Description : Enable LSC correct.;
* Input       : bool;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static void sonyimx135_otp_enable_lsc(bool enable)
{
	u8 lscMode = 0x00;
	u8 selToggle = 0x00;
	u8 lscEnable = 0x00;
	
	print_debug("enter %s", __func__);

	/* Open OTP lsc mode */
	if (enable) {
		lscMode = 0x01;
		selToggle = 0x01;
		lscEnable = 0x1F;
		print_info("%s, OTP LSC enabled!", __func__);
	}
	
	sonyimx135_write_reg(SONYIMX135_SENSOR_LSC_MODE, lscMode, 0x00);
	sonyimx135_write_reg(SONYIMX135_SENSOR_RAM_SEL, selToggle, 0x00);
	sonyimx135_write_reg(SONYIMX135_SENSOR_LSC_EN, lscEnable, 0x00);
}

/****************************************************************************
* FunctionName: sonyimx135_otp_read_vcm;
* Description : Get AF motor parameters from OTP.;
* Input       : NA;
* Output      : NA;
* ReturnValue : bool;
* Other       : NA;
***************************************************************************/
static bool sonyimx135_otp_read_vcm()
{
	print_debug("enter %s", __func__);
	
	if (!sonyimx135_otp_check_page(OTP_VCM)) {
		print_error("%s: Check OTP AF VCM page failed.", __func__);
		return false;
	}

	sonyimx135_read_reg(0x3B04, &sonyimx135_otp_vcm.start_curr_h);
	sonyimx135_read_reg(0x3B05, &sonyimx135_otp_vcm.start_curr_l);
	sonyimx135_read_reg(0x3B06, &sonyimx135_otp_vcm.infinity_curr_h);
	sonyimx135_read_reg(0x3B07, &sonyimx135_otp_vcm.infinity_curr_l);
	sonyimx135_read_reg(0x3B08, &sonyimx135_otp_vcm.macro_curr_h);
	sonyimx135_read_reg(0x3B09, &sonyimx135_otp_vcm.macro_curr_l);
	sonyimx135_read_reg(0x3B0A, &sonyimx135_otp_vcm.meter_curr_h);
	sonyimx135_read_reg(0x3B0B, &sonyimx135_otp_vcm.meter_curr_l);
	
	/* VCM param is read  */
	sonyimx135_otp_flag |= SONYIMX135_OTP_VCM_READ;

	return true;
}

/****************************************************************************
* FunctionName: sonyimx135_otp_set_vcm;
* Description : Set AF motor parameters to ISP.;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static void sonyimx135_otp_set_vcm()
{
	u16 start_curr = 0;
	u16 infinity_curr = 0;
	u16 macro_curr = 0;
	u8 otp_h = 0;
	u8 otp_l = 0;
	u32 rang = 0;
	
	print_debug("enter %s", __func__);

	otp_h = sonyimx135_otp_vcm.start_curr_h;
	otp_l = sonyimx135_otp_vcm.start_curr_l;
	start_curr = (otp_h << 8) | otp_l;

	otp_h = sonyimx135_otp_vcm.infinity_curr_h;
	otp_l = sonyimx135_otp_vcm.infinity_curr_l;
	infinity_curr = (otp_h << 8) | otp_l;

	otp_h = sonyimx135_otp_vcm.macro_curr_h;
	otp_l = sonyimx135_otp_vcm.macro_curr_l;
	macro_curr = (otp_h << 8) | otp_l;

	sonyimx135_vcm_start = start_curr;
	sonyimx135_vcm_end = sonyimx135_sensor.vcm->normalDistanceEnd;
	
	print_debug("%s, start = %#x, infinity = %#x, macro = %#x", 
		__func__, start_curr, infinity_curr, macro_curr);
}

/****************************************************************************
* FunctionName: sonyimx135_otp_get_vcm;
* Description : Get vcm start and stop parameters read from OTP.;
* Input       : NA;
* Output      : vcm_start vcm_end
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static void sonyimx135_otp_get_vcm(u16 *vcm_start, u16 *vcm_end)
{
	if (0 == sonyimx135_vcm_start) {
		*vcm_start = sonyimx135_sensor.vcm->infiniteDistance;
	} else {
		 if (sonyimx135_vcm_start > SONYIMX135_STARTCODE_OFFSET )
			*vcm_start = sonyimx135_vcm_start - SONYIMX135_STARTCODE_OFFSET;
		else
			*vcm_start = 0;
	}

	if (0 == sonyimx135_vcm_end) {
		*vcm_end = sonyimx135_sensor.vcm->normalDistanceEnd;
	} else {
		*vcm_end = sonyimx135_vcm_end;
	}
	
	print_info("%s, start: %#x, end: %#x", __func__, sonyimx135_vcm_start, sonyimx135_vcm_end);
}

/****************************************************************************
* FunctionName: sonyimx135_otp_update_afwb;
* Description : Update parameters form OTP when initializing.;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static void sonyimx135_otp_update_afwb(void)
{
	print_debug("enter %s", __func__);
	
	if ((sonyimx135_otp_flag & SONYIMX135_OTP_ID_WB_READ)
		|| sonyimx135_otp_read_id_wb()) {
		sonyimx135_otp_set_id();
		sonyimx135_otp_set_wb();
	} else {
		print_error("%s: Read OTP Module info and WB parameters failed.", __func__);
	}
	
	if ((sonyimx135_otp_flag & SONYIMX135_OTP_VCM_READ) || sonyimx135_otp_read_vcm()) {
		sonyimx135_otp_set_vcm();
	} else {
		print_error("%s: Read OTP AF VCM parameters failed.", __func__);
	}
}

/****************************************************************************
* FunctionName: sonyimx135_otp_record_lsc_func;
* Description : Recored OTP LSC parameter function, schedule by workqueue.;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static void sonyimx135_otp_record_lsc_func(struct work_struct *work)
{
	print_debug("enter %s", __func__);

	if ((sonyimx135_otp_flag & SONYIMX135_OTP_LSC_FILE_ERR) || (!sonyimx135_otp_lsc_file_exist())) {
		sonyimx135_otp_write_file();
		print_info("%s, OTP flag is: %#x", __func__, sonyimx135_otp_flag);
	}
}


static u32 imx135_get_override_param(camera_override_type_t type)
{
	u32 ret_val = sensor_override_params[type];

	switch (type) {
	case OVERRIDE_ISO_HIGH:
		ret_val = SONYIMX135_MAX_ISO;
		break;

	case OVERRIDE_ISO_LOW:
		ret_val = SONYIMX135_MIN_ISO;
		break;

	/* increase frame rate gain threshold */
	case OVERRIDE_AUTOFPS_GAIN_LOW2MID:
		ret_val = SONYIMX135_AUTOFPS_GAIN_LOW2MID;
		break;
	case OVERRIDE_AUTOFPS_GAIN_MID2HIGH:
		ret_val = SONYIMX135_AUTOFPS_GAIN_MID2HIGH;
		break;

	/* reduce frame rate gain threshold */
	case OVERRIDE_AUTOFPS_GAIN_MID2LOW:
		ret_val = SONYIMX135_AUTOFPS_GAIN_MID2LOW;
		break;
	case OVERRIDE_AUTOFPS_GAIN_HIGH2MID:
		ret_val = SONYIMX135_AUTOFPS_GAIN_HIGH2MID;
		break;

	case OVERRIDE_FPS_MAX:
		ret_val = SONYIMX135_MAX_FRAMERATE;
		break;

	case OVERRIDE_FPS_MIDDLE:
		ret_val = SONYIMX135_MIDDLE_FRAMERATE;
		break;

	case OVERRIDE_FPS_MIN:
		ret_val = SONYIMX135_MIN_FRAMERATE;
		break;

	case OVERRIDE_CAP_FPS_MIN:
		ret_val = SONYIMX135_MIN_CAP_FRAMERATE;
		break;

	case OVERRIDE_FLASH_TRIGGER_GAIN:
		ret_val = SONYIMX135_FLASH_TRIGGER_GAIN;
		break;

	case OVERRIDE_SHARPNESS_PREVIEW:
		ret_val = SONYIMX135_SHARPNESS_PREVIEW;
		break;

	case OVERRIDE_SHARPNESS_CAPTURE:
		ret_val = SONYIMX135_SHARPNESS_CAPTURE;
		break;

	default:
		print_error("%s:not override or invalid type %d, use default",__func__, type);
		break;
	}

	return ret_val;
}

static int sonyimx135_fill_denoise_buf(u8 *ybuf, u16 *uvbuf, u8 size, camera_state state, bool flash_on)
{
	u32 ae_th[3];
	u32 ae_value = get_writeback_gain() * get_writeback_expo() / 0x10;
	int index = sonyimx135_sensor.preview_frmsize_index;

	if( (ybuf==NULL)||(uvbuf==NULL)||(size <7 ) ){
		print_error("%s invalid arguments",__func__);
		return -1;
	}

	ybuf[0] = SONYIMX135_ISP_YDENOISE_COFF_1X;
	ybuf[1] = SONYIMX135_ISP_YDENOISE_COFF_2X;
	ybuf[2] = SONYIMX135_ISP_YDENOISE_COFF_4X;
	ybuf[3] = SONYIMX135_ISP_YDENOISE_COFF_8X;
	ybuf[4] = SONYIMX135_ISP_YDENOISE_COFF_16X;
	ybuf[5] = SONYIMX135_ISP_YDENOISE_COFF_32X;
	ybuf[6] = SONYIMX135_ISP_YDENOISE_COFF_64X;

	uvbuf[0] = SONYIMX135_ISP_UVDENOISE_COFF_1X;
	uvbuf[1] = SONYIMX135_ISP_UVDENOISE_COFF_2X;
	uvbuf[2] = SONYIMX135_ISP_UVDENOISE_COFF_4X;
	uvbuf[3] = SONYIMX135_ISP_UVDENOISE_COFF_8X;
	uvbuf[4] = SONYIMX135_ISP_UVDENOISE_COFF_16X;
	uvbuf[5] = SONYIMX135_ISP_UVDENOISE_COFF_32X;
	uvbuf[6] = SONYIMX135_ISP_UVDENOISE_COFF_64X;
	if (flash_on == false) {
		ae_th[0] = sonyimx135_sensor.frmsize_list[index].banding_step_50hz * 0x18;
		ae_th[1] = 3 * sonyimx135_sensor.frmsize_list[index].banding_step_50hz * 0x10;
		ae_th[2] = sonyimx135_sensor.frmsize_list[index].vts * 0x20;

		if (sonyimx135_sensor.frmsize_list[index].binning == false) {
			ae_th[0] *= 2;
			ae_th[1] *= 2;
			ae_th[2] *= 2;
		}

		/* simplify dynamic denoise threshold*/
		if (ae_value <= ae_th[0]) {
			ybuf[0] = SONYIMX135_ISP_YDENOISE_COFF_1X;
			ybuf[1] = SONYIMX135_ISP_YDENOISE_COFF_2X;
		} else {
			ybuf[0] = SONYIMX135_ISP_YDENOISE_COFF_2X;
			ybuf[1] = SONYIMX135_ISP_YDENOISE_COFF_2X;
		}
	} else {
		/* should calculated in capture_cmd again. */
		ybuf[0] = SONYIMX135_ISP_YDENOISE_COFF_1X;
		ybuf[1] = SONYIMX135_ISP_YDENOISE_COFF_2X;
	}

	return 0;
}
/*
/*
 **************************************************************************
 * FunctionName: sonyimx135_reset;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_reset(camera_power_state power_state)
{
	print_debug("%s  ", __func__);

	if (POWER_ON == power_state) {
		k3_isp_io_enable_mclk(MCLK_ENABLE, sonyimx135_sensor.sensor_index);
		k3_ispgpio_reset_sensor(sonyimx135_sensor.sensor_index, power_state,
			      sonyimx135_sensor.power_conf.reset_valid);
		udelay(500);
	} else {
		k3_ispgpio_reset_sensor(sonyimx135_sensor.sensor_index, power_state,
			      sonyimx135_sensor.power_conf.reset_valid);
		udelay(10);
		k3_isp_io_enable_mclk(MCLK_DISABLE, sonyimx135_sensor.sensor_index);
	}

	return 0;
}


/*
 **************************************************************************
 * FunctionName: sonyimx135_init;
 * Description : sonyimx135 init function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : Error code indicating success or failure;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx135_init(void)
{
	static bool sonyimx135_check = false;
	print_debug("%s  ", __func__);

	if (false == sonyimx135_check)
	{
		if (check_suspensory_camera("SONYIMX135") != 1)
		{
			return -ENODEV;
		}
		sonyimx135_check = true;
	}

	if(E_CAMERA_SENSOR_FLIP_TYPE_H_V == get_primary_sensor_flip_type())
		sonyimx135_sensor.sensor_rgb_type = SENSOR_BGGR;
	else
		sonyimx135_sensor.sensor_rgb_type = SENSOR_RGGB;

#if 0
	if (!camera_timing_is_match(0)) {
		print_error("%s: sensor timing don't match.\n", __func__);
		return -ENODEV;
	}
#endif
	
	if (sonyimx135_sensor.owner && !try_module_get(sonyimx135_sensor.owner)) {
		print_error("%s: try_module_get fail", __func__);
		return -ENOENT;
	}

	k3_ispio_power_init("pri-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V*/
	k3_ispio_power_init("camera-vcc", LDO_VOLTAGE_18V, LDO_VOLTAGE_18V);	/*IO 1.8V*/
	k3_ispio_power_init("cameravcm-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*AF 2.85V*/
	k3_ispio_power_init("sec-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V*/

	INIT_WORK(&record_otp_lsc_work, sonyimx135_otp_record_lsc_func);

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_exit;
 * Description : sonyimx135 exit function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx135_exit(void)
{
	print_debug("enter %s", __func__);

	k3_ispio_power_deinit();

	if (sonyimx135_sensor.owner)
		module_put(sonyimx135_sensor.owner);

	print_debug("exit %s", __func__);
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_shut_down;
 * Description : sonyimx135 shut down function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx135_shut_down(void)
{
	print_debug("enter %s", __func__);
	k3_ispgpio_power_sensor(&sonyimx135_sensor, POWER_OFF);
}

static int sonyimx135_get_sensor_aperture()
{
	return SONYIMX135_APERTURE_FACTOR;
}

static int sonyimx135_get_equivalent_focus()
{
	return SONYIMX135_EQUIVALENT_FOCUS;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_set_ATR;
 * Description : turn on or off atr curve;
 * Input       : 1 turn on atr ,0 turn off atr;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
void sonyimx135_set_ATR(int on)
{
       if(on == 0)
       {//ATR off
		sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
		sonyimx135_write_reg(ATR_OFF_SETTING_1,0x00, 0x00);
		sonyimx135_write_reg(ATR_OFF_SETTING_2,0x00, 0x00);
		sonyimx135_write_reg(TRIGGER_SETTING,0x01, 0x00);
		sonyimx135_write_reg(0x4344,0x01, 0x00);//ARNR
		sonyimx135_write_reg(0x4364,0x0B, 0x00);
		sonyimx135_write_reg(0x436C,0x00, 0x00);
		sonyimx135_write_reg(0x436D,0x00, 0x00);
		sonyimx135_write_reg(0x436E,0x00, 0x00);
		sonyimx135_write_reg(0x436F,0x00, 0x00);
		sonyimx135_write_reg(0x4369,0x00, 0x00);
		sonyimx135_write_reg(0x437B,0x00, 0x00);
		sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release
	}
	else
	{//ATR on
		sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
		sonyimx135_write_reg(0x437B,0x01, 0x00);
		sonyimx135_write_reg(0x4369,0x0f, 0x00);
		sonyimx135_write_reg(0x436F,0x06, 0x00);
		sonyimx135_write_reg(0x436E,0x00, 0x00);
		sonyimx135_write_reg(0x436D,0x00, 0x00);
		sonyimx135_write_reg(0x436C,0x00, 0x00);
		sonyimx135_write_reg(0x4364,0x0B, 0x00);

		sonyimx135_write_reg(0x4344,0x01, 0x00);

		sonyimx135_write_reg(TRIGGER_SETTING,0x00, 0x00);
		sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release
	}
	sonyimx135_sensor.hdrInfo.atr_on  = on;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_over_exposure_adjust;
 * Description :
 * Input       	:
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
void sonyimx135_over_exposure_adjust(int on,stats_hdr_frm * frm_stats)
{
	print_debug("enter %s  on = %d",__func__,on);
	if(on == 1)
	{
		sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
		sonyimx135_write_reg(0x4470,frm_stats->frm_ave_h&0xff,0x00);//frm_ave
		sonyimx135_write_reg(0x4471,frm_stats->frm_ave_l&0xff,0x00);//frm_ave2
		sonyimx135_write_reg(0x4472,frm_stats->frm_min_h&0xff,0x00);//frm_min1
		sonyimx135_write_reg(0x4473,frm_stats->frm_min_l&0xff,0x00);//frm_min2
		sonyimx135_write_reg(0x4474,frm_stats->frm_max_h&0xff,0x00);//frm_max1
		sonyimx135_write_reg(0x4475,frm_stats->frm_max_l&0xff,0x00);//frm_max2
		sonyimx135_write_reg(0x4476,0x01,0x00);
		sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release
		if(_IS_DEBUG_AE)
		{
			print_info(" frm_ave_h = %d,frm_ave_l =%d,frm_min_h=%d,frm_min_l=%d,frm_max_h=%d,frm_max_l=%d,",
				frm_stats->frm_ave_h,frm_stats->frm_ave_l,frm_stats->frm_min_h,frm_stats->frm_min_l,
				frm_stats->frm_max_h,frm_stats->frm_max_l);
		}
	}
	else
	{
		sonyimx135_write_reg(0x4476,0x00,0x00);
	}
	sonyimx135_sensor.hdrInfo.atr_over_expo_on = on;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_set_ATR;
 * Description : turn on or off atr curve;
 * Input       : 1 turn on atr ,0 turn off atr;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
void sonyimx135_set_lmt_sat(u32 lmt_value, u32 sat_value)
{
	if(_IS_DEBUG_AE)
	{
		       print_info(" Enter %s  lmt_value = %x ", __func__,lmt_value);
			print_info(" Enter %s  sat_value = %x ", __func__,sat_value);

}

	sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
	sonyimx135_write_reg(WB_LMT_REG_H, (lmt_value >> 8) & 0xff, 0x00);
	sonyimx135_write_reg(WB_LMT_REG_L, lmt_value & 0xff, 0x00);
	sonyimx135_write_reg(AE_SAT_REG_H, (sat_value >> 8) & 0xff, 0x00);
	sonyimx135_write_reg(AE_SAT_REG_L, sat_value & 0xff, 0x00);
	sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release
}


/*
 **************************************************************************
 * FunctionName: sonyimx135_get_hdr_movie_ae_param;
 * Description : The ae arith in HAL get init parm from sensor by this interface.
 * Input       : the space of hdr_ae_constant_param for keeping ratio,max gain,min gain and vts;
 * Output      : the value of ratio,max gain,min gain and vts;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
int  sonyimx135_get_hdr_movie_ae_param(hdr_ae_constant_param * hdr_ae_param)
{
	if(NULL != hdr_ae_param)
	{
		hdr_ae_param->hdr_ae_ratio 		= sonyimx135_sensor.ae_hdr_ratio;
		hdr_ae_param->sensor_max_gain	= sonyimx135_sensor.sensor_max_gain;
		hdr_ae_param->sensor_min_gain	= sonyimx135_sensor.sensor_min_gain;
		hdr_ae_param->max_analog_gain	= sonyimx135_sensor.sensor_max_analog_gain;
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_set_digital_gain;
 * Description : set digital gain to sensor's digital registers
 * Input       : the struct of digital_gain_st saving digital value
 * Output      : NA
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
void sonyimx135_set_digital_gain(digital_gain_st * digital_gain)
{
	u8 digital_gain_h = digital_gain->digital_gain_h;
	u8 digital_gain_l = digital_gain->digital_gain_l;
	if(_IS_DEBUG_AE)
	{
	       print_info("Enter %s  digital_gain_h = %d ,digital_gain_l=%d", __func__,digital_gain_h,digital_gain_l);
	}
	sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
	sonyimx135_write_reg(DIG_GAIN_GR_H, digital_gain_h, 0x00);
	sonyimx135_write_reg(DIG_GAIN_GR_L, digital_gain_l, 0x00);
	sonyimx135_write_reg(DIG_GAIN_R_H, digital_gain_h, 0x00);
	sonyimx135_write_reg(DIG_GAIN_R_L, digital_gain_l, 0x00);
	sonyimx135_write_reg(DIG_GAIN_B_H,digital_gain_h, 0x00);
	sonyimx135_write_reg(DIG_GAIN_B_L, digital_gain_l, 0x00);
	sonyimx135_write_reg(DIG_GAIN_GB_H, digital_gain_h, 0x00);
	sonyimx135_write_reg(DIG_GAIN_GB_L,digital_gain_l, 0x00);
	sonyimx135_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release
}
 /*
  *************************************************************************
  * FunctionName: 	:ispv1_get_digital_gain;
  * Description 	: get digital gain from sensor's digital registers
  * Input			: the struct of digital_gain_st for storging digital value
  * Output			: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
void sonyimx135_get_digital_gain(digital_gain_st * digital_gain)
{
	u8 digital_gain_h = 0;
	u8 digital_gain_l  = 0;
	sonyimx135_read_reg(DIG_GAIN_GR_H, &digital_gain_h);
	sonyimx135_read_reg(DIG_GAIN_GR_L, &digital_gain_l);
	sonyimx135_read_reg(DIG_GAIN_R_H, &digital_gain_h);
	sonyimx135_read_reg(DIG_GAIN_R_L, &digital_gain_l);
	sonyimx135_read_reg(DIG_GAIN_B_H,&digital_gain_h);
	sonyimx135_read_reg(DIG_GAIN_B_L, &digital_gain_l);
	sonyimx135_read_reg(DIG_GAIN_GB_H, &digital_gain_h);
	sonyimx135_read_reg(DIG_GAIN_GB_L,&digital_gain_l);


	digital_gain->digital_gain_h = digital_gain_h;
	digital_gain->digital_gain_l = digital_gain_l;
}
  /*
  *************************************************************************
  * FunctionName: 	:sonyimx135_get_sensor_preview_max_size;
  * Description 	: get digital gain from sensor's digital registers
  * Input			: the struct of digital_gain_st for storging digital value
  * Output			: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
 int sonyimx135_get_sensor_preview_max_size(preview_size * pre_size)
 {
	if(HDR_MOVIE_ON == sonyimx135_sensor.hdrInfo.hdr_on)
	{
		pre_size->width = 2048;
		pre_size->height = 1200;
	}else{
		pre_size->width = 2048;
		pre_size->height = 1536;
	}
	return 0;
 }

  /*
  *************************************************************************
  * FunctionName: 	:sonyimx135_check_zoom_limit
  * Description 	: check zoom limit for zoom problem for isp defect
  * Input			: zoom value
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
 int sonyimx135_check_zoom_limit(u32 *zoom)
 {
	if(zoom == NULL)
	{
		return -1;
	}
	if(*zoom < SONYIMX135_ISP_ZOOM_LIMIT && *zoom  > 0)
	{
	       *zoom = SONYIMX135_ISP_ZOOM_LIMIT;
	}
	return 0;
 }
    /*
  *************************************************************************
  * FunctionName: 	: sonyimx135_set_dpc_funciton;
  * Description 	: set dpc register to isp and sensor according to iso
  * Input			: the struct of _sensor_reg_t for storging sensor register
  * address and value, iso is used for differentiating dpc strong and weak, size is
  * used for keeping the size of  dpc_reg array.
  * Output			: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
 int sonyimx135_set_dpc_funciton(struct _sensor_reg_t *dpc_reg,u32 iso,u16 * size)
 {
	print_info("%s,iso=%d",__func__,iso);
	int isp_dpc_size = 0;
	if(dpc_reg == NULL|| size == NULL)
	{
		return -1;
	}
	dpc_reg->subaddr = 0x4100;
	if(iso > SONYIMX135_DPC_THRESHOLD_ISO)
	{
		dpc_reg->value	  = 0xF8;
		const struct isp_reg_t  isp_dpc_reg[]={
			{0x65409,0x08},
			{0x6540a,0x08},
			{0x6540b,0x01},
			{0x6540c,0x01},
			{0x6540d,0x08},
			{0x6540e,0x08},
			{0x6540f,0x01},
			{0x65410,0x01},
			{0x65408,0x0b},
		};
		isp_dpc_size = ARRAY_SIZE(isp_dpc_reg);
		sonyimx135_write_isp_seq(isp_dpc_reg, isp_dpc_size);
	}
	else
	{
		dpc_reg->value	  = 0xE8;
		const struct isp_reg_t  isp_dpc_reg[]={
			{0x65409,0x08},
			{0x6540a,0x08},
			{0x6540b,0x08},
			{0x6540c,0x08},
			{0x6540d,0x0c},
			{0x6540e,0x08},
			{0x6540f,0x08},
			{0x65410,0x08},
			{0x65408,0x0b},
		};
		isp_dpc_size = ARRAY_SIZE(isp_dpc_reg);
		sonyimx135_write_isp_seq(isp_dpc_reg, isp_dpc_size);
	}
	*size = 1;

	return 0;
 }

/*
 **************************************************************************
 * FunctionName: sonyimx135_set_default;
 * Description : init sonyimx135_sensor;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx135_set_default(void)
{
	sonyimx135_sensor.init = sonyimx135_init;
	sonyimx135_sensor.exit = sonyimx135_exit;
	sonyimx135_sensor.shut_down = sonyimx135_shut_down;
	sonyimx135_sensor.reset = sonyimx135_reset;
	sonyimx135_sensor.check_sensor = sonyimx135_check_sensor_new;
	sonyimx135_sensor.power = sonyimx135_power;
	sonyimx135_sensor.init_reg = sonyimx135_init_reg;
	sonyimx135_sensor.stream_on = sonyimx135_stream_on;

	sonyimx135_sensor.get_format = sonyimx135_get_format;
	sonyimx135_sensor.set_flash = NULL;
	sonyimx135_sensor.get_flash = NULL;
	sonyimx135_sensor.set_scene = NULL;
	sonyimx135_sensor.get_scene = NULL;

	sonyimx135_sensor.enum_framesizes = sonyimx135_enum_framesizes;
	sonyimx135_sensor.try_framesizes = sonyimx135_try_framesizes;
	sonyimx135_sensor.set_framesizes = sonyimx135_set_framesizes;
	sonyimx135_sensor.get_framesizes = sonyimx135_get_framesizes;

	sonyimx135_sensor.enum_frame_intervals = sonyimx135_enum_frame_intervals;
	sonyimx135_sensor.try_frame_intervals = NULL;
	sonyimx135_sensor.set_frame_intervals = NULL;
	sonyimx135_sensor.get_frame_intervals = NULL;

	sonyimx135_sensor.get_capability = sonyimx135_get_capability;

	sonyimx135_sensor.set_hflip = sonyimx135_set_hflip;
	sonyimx135_sensor.get_hflip = sonyimx135_get_hflip;
	sonyimx135_sensor.set_vflip = sonyimx135_set_vflip;
	sonyimx135_sensor.get_vflip = sonyimx135_get_vflip;
	sonyimx135_sensor.update_flip = sonyimx135_update_flip;
	sonyimx135_sensor.get_sensor_aperture = sonyimx135_get_sensor_aperture;
	sonyimx135_sensor.get_equivalent_focus = sonyimx135_get_equivalent_focus;
	
	snprintf(sonyimx135_sensor.info.name, sizeof(sonyimx135_sensor.info.name),"sonyimx135");
	sonyimx135_sensor.interface_type = MIPI1;
	sonyimx135_sensor.mipi_lane_count = CSI_LINES_4;
	sonyimx135_sensor.mipi_index = CSI_INDEX_0;
	sonyimx135_sensor.sensor_index = CAMERA_SENSOR_PRIMARY;
	sonyimx135_sensor.skip_frames = 1;

	sonyimx135_sensor.power_conf.pd_valid = LOW_VALID;
	sonyimx135_sensor.power_conf.reset_valid = LOW_VALID;
	sonyimx135_sensor.power_conf.vcmpd_valid = LOW_VALID;

	sonyimx135_sensor.i2c_config.index = I2C_PRIMARY;
	sonyimx135_sensor.i2c_config.speed = I2C_SPEED_400;
	//sonyimx135_sensor.i2c_config.addr = SONYIMX135_SLAVE_ADDRESS;
	sonyimx135_sensor.i2c_config.addr_bits = 16;
	sonyimx135_sensor.i2c_config.val_bits = I2C_8BIT;

	sonyimx135_sensor.preview_frmsize_index = 0;
	sonyimx135_sensor.capture_frmsize_index = 0;
	//sonyimx135_sensor.frmsize_list = sonyimx135_framesizes;
	sonyimx135_sensor.fmt[STATE_PREVIEW] = V4L2_PIX_FMT_RAW10;
	sonyimx135_sensor.fmt[STATE_CAPTURE] = V4L2_PIX_FMT_RAW10;

#ifdef SONYIMX135_AP_WRITEAE_MODE /* just an example and test case for AP write AE mode */
	sonyimx135_sensor.aec_addr[0] = 0;
	sonyimx135_sensor.aec_addr[1] = 0;
	sonyimx135_sensor.aec_addr[2] = 0;
	sonyimx135_sensor.agc_addr[0] = 0;
	sonyimx135_sensor.agc_addr[1] = 0;
	sonyimx135_sensor.ap_writeAE_delay = 1500; /* 5 expo and gain registers, 1500us is enough */
#else
	sonyimx135_sensor.aec_addr[0] = 0x0000;
	sonyimx135_sensor.aec_addr[1] = 0x0202;
	sonyimx135_sensor.aec_addr[2] = 0x0203;
	sonyimx135_sensor.agc_addr[0] = 0x0000; /*0x0204 high byte not needed*/
	sonyimx135_sensor.agc_addr[1] = 0x0205;
#endif
	sonyimx135_sensor.sensor_type = SENSOR_SONY;
	//sonyimx135_sensor.sensor_rgb_type = SENSOR_RGGB;/* changed by y00231328. add bayer order*/

	sonyimx135_sensor.set_gain = sonyimx135_set_gain;
	sonyimx135_sensor.set_exposure = sonyimx135_set_exposure;

	sonyimx135_sensor.set_vts = sonyimx135_set_vts;
	sonyimx135_sensor.construct_vts_reg_buf = sonyimx135_construct_vts_reg_buf;
	sonyimx135_sensor.sensor_gain_to_iso = NULL;
	sonyimx135_sensor.sensor_iso_to_gain = NULL;
	sonyimx135_sensor.get_af_param = sonyimx135_get_af_param;
	sonyimx135_sensor.set_effect = NULL;
	sonyimx135_sensor.fill_denoise_buf = sonyimx135_fill_denoise_buf;
	sonyimx135_sensor.get_override_param = imx135_get_override_param;

	sonyimx135_sensor.isp_location = CAMERA_USE_K3ISP;
	sonyimx135_sensor.sensor_tune_ops = NULL;

	sonyimx135_sensor.af_enable = 1;
	sonyimx135_sensor.vcm = &vcm_dw9714;

	sonyimx135_sensor.image_setting.lensc_param = sonyimx135_lensc_param;
	sonyimx135_sensor.image_setting.ccm_param = sonyimx135_ccm_param;
	sonyimx135_sensor.image_setting.awb_param = sonyimx135_awb_param;

	sonyimx135_sensor.fps_max = 30;
	sonyimx135_sensor.fps_min = 10;
	sonyimx135_sensor.fps = 25;

	sonyimx135_sensor.owner = THIS_MODULE;

	sonyimx135_sensor.info.facing = CAMERA_FACING_BACK;
	sonyimx135_sensor.info.orientation = 90;   /*270;*/
	sonyimx135_sensor.info.focal_length = 439; /* 4.39mm*/
	sonyimx135_sensor.info.h_view_angle = 66; /*  66.1 degree */
	sonyimx135_sensor.info.v_view_angle = 50;
	sonyimx135_sensor.lane_clk = CLK_450M;
	sonyimx135_sensor.hflip = 0;
	sonyimx135_sensor.vflip = 0;
	sonyimx135_sensor.old_flip = 0;

	//modiy format
	// for HDR movie funtion
	sonyimx135_sensor.set_awb_gain 				= sonyimx135_set_awb_gain;
	sonyimx135_sensor.set_hdr_exposure_gain 		= sonyimx135_set_hdr_exposure_gain;
	sonyimx135_sensor.set_atr_ctrl_points   		= sonyimx135_set_ATR_ctrl_points;
	sonyimx135_sensor.get_atr_ctrl_points 			= sonyimx135_get_ATR_ctrl_points;
	sonyimx135_sensor.hdr_regs_y 				= sonyimx135_hdr_regs_y;
	sonyimx135_sensor.set_ATR_switch     			= sonyimx135_set_ATR;
	sonyimx135_sensor.get_hdr_lux_matrix    		= sonyimx135_get_lux_matrix;
	sonyimx135_sensor.get_gain     				= sonyimx135_get_gain;
	sonyimx135_sensor.get_exposure   			= sonyimx135_get_exposure;
	sonyimx135_sensor.init_isp_reg 				= sonyimx135_init_isp_reg;
	sonyimx135_sensor.support_hdr_movie			= sonyimx135_support_hdr_movie;
	sonyimx135_sensor.get_hdr_movie_switch		= sonyimx135_get_hdr_movie_switch;
	sonyimx135_sensor.set_hdr_movie_switch		= sonyimx135_set_hdr_movie_switch;
	sonyimx135_sensor.over_exposure_adjust		= sonyimx135_over_exposure_adjust;
	sonyimx135_sensor.set_lmt_sat 				= sonyimx135_set_lmt_sat;
	sonyimx135_sensor.get_hdr_movie_ae_param	= sonyimx135_get_hdr_movie_ae_param;
	sonyimx135_sensor.sensor_write_reg 			= sonyimx135_write_reg;
	sonyimx135_sensor.set_digital_gain			= sonyimx135_set_digital_gain;
	sonyimx135_sensor.get_digital_gain			= sonyimx135_get_digital_gain;
	sonyimx135_sensor.get_sensor_preview_max_size = sonyimx135_get_sensor_preview_max_size;
	sonyimx135_sensor.set_dpc_funciton 			= sonyimx135_set_dpc_funciton;
	sonyimx135_sensor.check_zoom_limit 			= sonyimx135_check_zoom_limit;
	//check default parameter
	sonyimx135_sensor.ae_hdr_ratio          			= 8;
	sonyimx135_sensor.hdrInfo.atr_on     			= ATR_ON;
	sonyimx135_sensor.sensor_max_gain			=32;
	sonyimx135_sensor.sensor_max_analog_gain       = 8;
	sonyimx135_sensor.sensor_min_gain			=1;
	sonyimx135_sensor.gain_switch 				= 100;//0x66
	sonyimx135_sensor.hdrInfo.hdr_on 			= HDR_MOVIE_OFF;//default atr on
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_module_init;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static __init int sonyimx135_module_init(void)
{
	sonyimx135_set_default();
	return register_camera_sensor(sonyimx135_sensor.sensor_index, &sonyimx135_sensor);
}

/*
 **************************************************************************
 * FunctionName: sonyimx135_module_exit;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void __exit sonyimx135_module_exit(void)
{
	unregister_camera_sensor(sonyimx135_sensor.sensor_index, &sonyimx135_sensor);
}

MODULE_AUTHOR("Hisilicon");
module_init(sonyimx135_module_init);
module_exit(sonyimx135_module_exit);
MODULE_LICENSE("GPL");

/********************************** END **********************************************/
