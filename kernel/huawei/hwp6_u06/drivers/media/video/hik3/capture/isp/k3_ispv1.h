/*
 *  Hisilicon K3 soc camera ISP driver header file
 *
 *  CopyRight (C) Hisilicon Co., Ltd.
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

#ifndef __K3_ISPV1_H__
#define __K3_ISPV1_H__

#include <linux/videodev2.h>
#include <linux/platform_device.h>
#include <mach/platform.h>
#include <mach/hardware.h>
#include <mach/io_mutex.h>

#define FIRMWARE_FILE_NAME              "/system/isp.bin"

/* big-endian to little-endian */
#define BIG_ENDIAN(a) ((a) + 3 - 2 * ((a) & 0x3))

/* REGISTER READ/WRITE DEFINITION */
#define GETREG8(reg) \
	(((reg) >= COMMAND_BUFFER_END) ? \
	(ioread8_mutex((isp_hw_data.base + (reg)))) : \
	(ioread8_mutex((isp_hw_data.base + BIG_ENDIAN((reg))))))

#define GETREG32(reg, value) \
	do { \
		(value) = (GETREG8(reg) << 24) + \
			(GETREG8((reg) + 1) << 16) + \
			(GETREG8((reg) + 2) << 8) + \
			(GETREG8((reg) + 3)); \
	} while (0)
	
#define GETREG16(reg, value) \
	do { \
		(value) = (GETREG8(reg) << 8) + \
		(GETREG8((reg) + 1)); \
	} while (0)

#if 0
#define SETREG8(reg, value) \
		(((reg) >= COMMAND_BUFFER_END) ? \
		iowrite8_mutex((value), (isp_hw_data.base + (reg))) : \
		iowrite8_mutex((value), (isp_hw_data.base + BIG_ENDIAN((reg)))))


#define SETREG16(reg, value) \
	do { \
		SETREG8((reg), ((value) >> 8) & 0xff); \
		SETREG8((reg) + 1, (value) & 0xff);    \
	} while (0)

#define SETREG32(reg, value) \
	do { \
		SETREG8(reg,     ((value) >> 24) & 0xff); \
		SETREG8(reg + 1, ((value) >> 16) & 0xff); \
		SETREG8(reg + 2, ((value) >> 8) & 0xff);  \
		SETREG8(reg + 3, ((value) >> 0) & 0xff);  \
	} while (0)
#endif


/* isp memory layout */
/* 1. PROGREM BUFFER(112KB): 0X0 ~ 0X01BFFF   */
#define FIRMWARE_BASE                   (0)

/* 2. DATA BUFFER (12KB): 0X01C000 ~ 0X01EFFF */

/* 3. CACHE BUFFER (4KB): 0X01F000 ~ 0X01FFFF */

/* 4. LINE BUFFER(192KB): 0X020000 ~ 0X04FFFF */

/* 5. HW REGISTER(128KB): 0X050000 ~ 0X06FFFF */

/* command set registers */
#define COMMAND_REG0            (0x63900)
#define COMMAND_REG1            (COMMAND_REG0 + 0x1)
#define COMMAND_REG2            (COMMAND_REG0 + 0x2)
#define COMMAND_REG3            (COMMAND_REG0 + 0x3)
#define COMMAND_REG4            (COMMAND_REG0 + 0x4)
#define COMMAND_REG5            (COMMAND_REG0 + 0x5)
#define COMMAND_REG6            (COMMAND_REG0 + 0x6)
#define COMMAND_REG7            (COMMAND_REG0 + 0x7)
#define COMMAND_REG8            (COMMAND_REG0 + 0x8)
#define COMMAND_REG9            (COMMAND_REG0 + 0x9)
#define COMMAND_REG10           (COMMAND_REG0 + 0xa)
#define COMMAND_REG11           (COMMAND_REG0 + 0xb)
#define COMMAND_REG12           (COMMAND_REG0 + 0xc)
#define COMMAND_REG13           (COMMAND_REG0 + 0xd)
#define COMMAND_REG14           (COMMAND_REG0 + 0xe)
#define COMMAND_REG15           (COMMAND_REG0 + 0xf)

#define COMMAND_FINISHED        (0x63910)
#define COMMAND_RESULT          (0x63911)

#define COMMAND_BUFFER          (0x1ea00)
#define COMMAND_BUFFER_END	 (0x1ef00)

#define COMMAND_SET_SUCCESS     (0x1)

/* command set id */
#define CMD_I2C_GRP_WR          (0x1)
#define CMD_SET_FORMAT          (0x2)
#define CMD_RESET_SENSOR        (0x3)
#define CMD_CAPTURE             (0x4)
#define CMD_OFFLINE_PROCESS     (0x5)
#define CMD_OFFLINE_HDR         (0x6)
#define CMD_FULL_SZIE_HDR       (0x7)
#define CMD_ISP_REG_GRP_WRITE   (0x8)
#define CMD_ABORT               (0x9)

#define CMD_AWB_MODE            (0x11)
#define CMD_ANTI_SHAKE_ENABLE   (0x12)
#define CMD_AUTOFOCUS_MODE      (0x13)
#define CMD_ZOOM_IN_MODE        (0x14)

/*firmware interrupt IDs from MCU*/
#define CMD_FIRMWARE_DOWNLOAD		(0xff)
#define CMD_BRACKET_CAPTURE			(0xfe)

/*y36721 added for new firmware(2011-11-24 V1.0) begin*/
#define CMD_WRITEBACK_EXPO_GAIN	(0xf1)
#define CMD_WRITEBACK_EXPO			(0xf2)
#define CMD_WRITEBACK_GAIN			(0xf3)
/*y36721 add for new firmware(2011-11-24 V1.0)  end*/

/* for command set CMD_I2C_GRP_WR */
/* REG1:I2C choice */
#define SELECT_I2C_NONE         (0x0)
#define SELECT_I2C_PRIMARY      (0x1)
#define SELECT_I2C_SECONDARY    (0x1<<1)

#define SELECT_I2C_16BIT_DATA   (0x1<<2)
#define SELECT_I2C_8BIT_DATA    (0x0)

#define SELECT_I2C_16BIT_ADDR   (0x1<<3)
#define SELECT_I2C_8BIT_ADDR    (0x0)

#define SELECT_I2C_WRITE        (0x1<<8)
#define SELECT_I2C_READ         (0x0)

#define ISP_FIRMWARE_REGBASE    (0x1e900)
#define ISP_FIRMWARE_REGEND     (0x1e9ff)

/* set format and capture format parameters */
#define ISP_INPUT_FORMAT                (0x1e900)
#define ISP_OUTPUT_FORMAT               (0x1e901)
#define SENSOR_OUTPUT_WIDTH             (0x1e902)
#define SENSOR_OUTPUT_HEIGHT            (0x1e904)
#define ISP_INPUT_WIDTH                 (0x1e906)
#define ISP_INPUT_HEIGHT                (0x1e908)
#define ISP_INPUT_H_START               (0x1e90a)
#define ISP_INPUT_V_START               (0x1e90c)
#define ISP_INPUT_H_SENSOR_START        (0x1e90e)
#define ISP_INPUT_V_SENSOR_START        (0x1e910)
#define ISP_OUTPUT_WIDTH                (0x1e912)
#define ISP_OUTPUT_HEIGHT               (0x1e914)
#define ISP_INPUT_H_START_3D            (0x1e916)
#define ISP_INPUT_V_START_3D            (0x1e918)
#define ISP_INPUT_H_SENSOR_START_3D     (0x1e91a)
#define ISP_INPUT_V_SENSOR_START_3D     (0x1e91c)
#define MAC_MEMORY_WIDTH                (0x1e91e)
#define ISP_INPUT_MODE                  (0x1e920)
#define ISP_FUNCTION_CTRL               (0x1e921)
#define ISP_SCALE_DOWN_H_RATIO1         (0x1e922)
#define ISP_SCALE_DOWN_H_RATIO2         (0x1e923)
#define ISP_SCALE_DOWN_V_RATIO1         (0x1e924)
#define ISP_SCALE_DOWN_V_RATIO2         (0x1e925)
#define ISP_SCALE_UP_H_RATIO            (0x1e926)
#define ISP_SCALE_UP_V_RATIO            (0x1e928)
#define ISP_YUV_CROP_H_START            (0x1e92a)
#define ISP_YUV_CROP_V_START            (0x1e92c)
#define ISP_YUV_CROP_WIDTH              (0x1e92e)
#define ISP_YUV_CROP_HEIGHT             (0x1e930)
#define ISP_EXPOSURE_RATIO              (0x1e932)
#define ISP_MAX_EXPOSURE                (0x1e934)
#define ISP_MIN_EXPOSURE                (0x1e936)
#define ISP_MAX_GAIN                    (0x1e938)
#define ISP_MIN_GAIN                    (0x1e93a)
#define ISP_MAX_EXPOSURE_FOR_HDR        (0x1e93c)
#define ISP_MIN_EXPOSURE_FOR_HDR        (0x1e93e)
#define ISP_MAX_GAIN_FOR_HDR            (0x1e940)
#define ISP_MIN_GAIN_FOR_HDR            (0x1e942)
#define ISP_VTS                         (0x1e944)
#define ISP_SKIP_WAITING_FRAME		(0x1c97f)

/* capture spcecial parameters register */
#define ISP_BRACKET_RATIO1              (0x1e946)
#define ISP_BRACKET_RATIO2              (0x1e948)
#define ISP_BASE_ADDR_LEFT              (0x1e94c)
#define ISP_BASE_ADDR_LEFT_UV           (0x1e950)
#define ISP_BASE_ADDR_RIGHT             (0x1e954)

#if 1 /* firmware 2013-02-01 new registers definitions */
#define ISP_FIRST_FRAME_EXPOSURE        (0x1e978)
#define ISP_SECOND_FRAME_EXPOSURE       (0x1e97c)
#define ISP_THIRD_FRAME_EXPOSURE       (0x1e988)
#else /* old firmware, 16bit is not enough */
#define ISP_FIRST_FRAME_EXPOSURE        (0x1e95c)
#define ISP_SECOND_FRAME_EXPOSURE       (0x1e960)
#define ISP_THIRD_FRAME_EXPOSURE       (0x1e964)
#endif
#define ISP_FIRST_FRAME_GAIN            (0x1e95e)
#define ISP_SECOND_FRAME_GAIN           (0x1e962)
#define ISP_THIRD_FRAME_GAIN           (0x1e966)

/* ISP banding step parameters registers */
#define REG_ISP_BANDING_STEP_50HZ           (0x1c166)
#define REG_ISP_BANDING_STEP_60HZ           (0x1c164)

/* firmware banding step parameters registers */
#define ISP_BANDING_STEP_50HZ           (0x1e968)
#define ISP_BANDING_STEP_60HZ           (0x1e96a)

#define ISP_CMDSET_CCM_PREGAIN_ENABLE		(0x1e980) /* bit[0] ccm pre-gain; bit[1] awb offset. */
#define ISP_CMDSET_CCM_PREGAIN_B		(0x1e981)
#define ISP_CMDSET_CCM_PREGAIN_G		(0x1e982)
#define ISP_CMDSET_CCM_PREGAIN_R		(0x1e983)

#define REG_ISP_CCM_PREGAIN_B		(0x1c5ac)
#define REG_ISP_CCM_PREGAIN_G		(0x1c5ad)
#define REG_ISP_CCM_PREGAIN_R		(0x1c5ae)

#define ISP_CMDSET_AWB_OFFSET_B		(0x1e984)
#define ISP_CMDSET_AWB_OFFSET_G		(0x1e985)
#define ISP_CMDSET_AWB_OFFSET_R		(0x1e986)

#define REG_ISP_AWB_OFFSET_B		(0x1c1be)
#define REG_ISP_AWB_OFFSET_GB		(0x1c1bf)
#define REG_ISP_AWB_OFFSET_GR		(0x1c1c0)
#define REG_ISP_AWB_OFFSET_R		(0x1c1c1)

#define ISP_YDENOISE_COFF_1X		2
#define ISP_YDENOISE_COFF_2X		2
#define ISP_YDENOISE_COFF_4X		6
#define ISP_YDENOISE_COFF_8X		8
#define ISP_YDENOISE_COFF_16X   	16
#define ISP_YDENOISE_COFF_32X		0x20
#define ISP_YDENOISE_COFF_64X		0x30

#define REG_ISP_YDENOISE_1X		(0x6551a)
#define REG_ISP_YDENOISE_2X		(0x6551b)
#define REG_ISP_YDENOISE_4X		(0x6551c)
#define REG_ISP_YDENOISE_8X		(0x6551d)
#define REG_ISP_YDENOISE_16X		(0x6551e)
#define REG_ISP_YDENOISE_32X		(0x6551f)
#define REG_ISP_YDENOISE_64X		(0x65520)

#define REG_ISP_UVDENOISE_1X		(0x65522)
#define REG_ISP_UVDENOISE_2X		(0x65524)
#define REG_ISP_UVDENOISE_4X		(0x65526)
#define REG_ISP_UVDENOISE_8X		(0x65528)
#define REG_ISP_UVDENOISE_16X		(0x6552a)
#define REG_ISP_UVDENOISE_32X		(0x6552c)
#define REG_ISP_UVDENOISE_64X		(0x6552e)

#define REG_ISP_REF_BIN					(0x66302)

#define PRIVIEW_WIDTH_HIGH			960
#define PRIVIEW_WIDTH_LOW				320

#define RED_CLIP_FRAME_INTERVAL 8
#define RED_CLIP_DECTECT_RANGE		50
#define RED_CLIP_UV_RESAMPLE_HIGH		8
#define RED_CLIP_UV_RESAMPLE_MIDDLE	4
#define RED_CLIP_UV_RESAMPLE_LOW		2
#define RED_CLIP_RECT_ROW_NUM			5
#define RED_CLIP_RECT_COL_NUM			5
#define RED_CLIP_REFBIN_LOW			0x80
#define RED_CLIP_REFBIN_HIGH			0xf0
#define RED_CLIP_V_THRESHOLD_HIGH		160
#define RED_CLIP_V_THRESHOLD_LOW		130

typedef enum {
	RED_CLIP_NONE = 0,
	RED_CLIP_SHADE,
	RED_CLIP,
} red_clip_status;

/* cmd_set and capture_cmd register operation */
#define CMD_SET_ISP_IN_FMT_SIZE(fmt, width, height) \
	do { \
		SETREG8(ISP_INPUT_FORMAT, fmt);    \
		SETREG16(ISP_INPUT_WIDTH, width);  \
		SETREG16(ISP_INPUT_HEIGHT, height); \
	} while (0)

#define CMD_SET_SENSOR_OUT_SIZE(width, height) \
	do { \
		SETREG16(SENSOR_OUTPUT_WIDTH, width);   \
		SETREG16(SENSOR_OUTPUT_HEIGHT, height); \
	} while (0)

#define    	CMD_SET_ISP_IN_START_POS(x, y) \
	do { \
		SETREG16(ISP_INPUT_H_START, (x)); \
		SETREG16(ISP_INPUT_V_START, (y)); \
	} while (0)

#define CMD_SET_SENSOR_START_POS(x, y) \
	do { \
		SETREG16(ISP_INPUT_H_SENSOR_START, (x)); \
		SETREG16(ISP_INPUT_V_SENSOR_START, (y)); \
	} while (0)

#define CMD_SET_ISP_OUT_FMT_SIZE(fmt, width, height) \
	do { \
		SETREG8(ISP_OUTPUT_FORMAT, (fmt));     \
		SETREG16(ISP_OUTPUT_WIDTH, (width));   \
		SETREG16(ISP_OUTPUT_HEIGHT, (height)); \
	} while (0)

#define CMD_SET_ISP_IN_START_3D_POS(x, y) \
	do { \
		SETREG16(ISP_INPUT_H_START_3D, (x)); \
		SETREG16(ISP_INPUT_V_START_3D, (y)); \
	} while (0)

#define CMD_SET_SENSOR_START_3D_POS(x, y) \
	do { \
		SETREG16(ISP_INPUT_H_SENSOR_START_3D, (x)); \
		SETREG16(ISP_INPUT_V_SENSOR_START_3D, (y)); \
	} while (0)

#define CMD_SET_MAC_MEM_STRIDE(stride) \
		SETREG16(MAC_MEMORY_WIDTH, (stride))

#define ISP_INPUT_FLIP   (0x1)
#define ISP_INPUT_MIRROR (0x1 << 1)
#define CMD_SET_ISP_INPUT_MODE(mode) \
		SETREG8(ISP_INPUT_MODE, (mode))

#define CMD_SET_ISP_FUNCTION_CTRL(value) \
		SETREG8(ISP_FUNCTION_CTRL, (value) & 0xff)

#define CMD_SET_ISP_SCALE_DOWN_RATIO1(dcw_h, dcw_v) \
	do { \
		BUG_ON(dcw_h > 3); \
		BUG_ON(dcw_v > 3); \
		SETREG8(ISP_SCALE_DOWN_H_RATIO1, (dcw_h)); \
		SETREG8(ISP_SCALE_DOWN_V_RATIO1, (dcw_v)); \
	} while (0)

#define CMD_SET_ISP_SCALE_DOWN_RATIO2(nscale_h, nscale_v) \
	do { \
		SETREG8(ISP_SCALE_DOWN_H_RATIO2, (nscale_h)); \
		SETREG8(ISP_SCALE_DOWN_V_RATIO2, (nscale_v)); \
	} while (0)

#define CMD_SET_ISP_SCALE_UP_RATIO(nscale_h, nscale_v) \
	do { \
		SETREG8(ISP_SCALE_UP_H_RATIO, ((nscale_h) >> 8) & 0x1); \
		SETREG8(ISP_SCALE_UP_H_RATIO + 1, (nscale_h) & 0xff);   \
		SETREG8(ISP_SCALE_UP_V_RATIO, ((nscale_v) >> 8) & 0x1); \
		SETREG8(ISP_SCALE_UP_V_RATIO + 1, (nscale_v) & 0xff);   \
	} while (0)

#define CMD_SET_ISP_YUV_CROP_POS(crop_x, crop_y) \
	do { \
		SETREG16(ISP_YUV_CROP_H_START, (crop_x)); \
		SETREG16(ISP_YUV_CROP_V_START, (crop_y)); \
	} while (0)

#define CMD_SET_ISP_YUV_CROP_SIZE(crop_w, crop_h) \
	do { \
		SETREG16(ISP_YUV_CROP_WIDTH, (crop_w));  \
		SETREG16(ISP_YUV_CROP_HEIGHT, (crop_h)); \
	} while (0)

#define CMD_SET_ISP_SET_EXPOSURE_RATIO(nexposure) \
		SETREG16(ISP_EXPOSURE_RATIO, (nexposure))

#define CMD_SET_ISP_SET_EXPOSURE_RANGE(max, min) \
	do { \
		SETREG16(ISP_MAX_EXPOSURE, (max)); \
		SETREG16(ISP_MIN_EXPOSURE, (min)); \
	} while (0)

#define CMD_SET_ISP_SET_GAIN_RANGE(max, min) \
	do { \
		SETREG16(ISP_MAX_GAIN, (max)); \
		SETREG16(ISP_MIN_GAIN, (min)); \
	} while (0)

#define CMD_SET_ISP_SET_HDR_EXPOSURE_RANGE(max, min) \
	do { \
		SETREG16(ISP_MAX_EXPOSURE_FOR_HDR, (max)); \
		SETREG16(ISP_MIN_EXPOSURE_FOR_HDR, (min)); \
	} while (0)

#define CMD_SET_ISP_SET_HDR_GAIN_RANGE(max, min) \
	do { \
		SETREG16(ISP_MAX_GAIN_FOR_HDR, (max)); \
		SETREG16(ISP_MIN_GAIN_FOR_HDR, (min)); \
	} while (0)

#define CMD_SET_SENSOR_VTS(vts) \
		SETREG16(ISP_VTS, (vts))

/* capture spcecial parameters register operation*/
#define CMD_SET_BASE_ADDR(left, phyaddr)\
	do { \
		if (left == BUF_LEFT) {\
			SETREG32(ISP_BASE_ADDR_LEFT, (phyaddr));\
		} else {\
			SETREG32(ISP_BASE_ADDR_RIGHT, (phyaddr));\
		} \
	} while (0)

#define CMD_SET_BASE_ADDR0(phyaddr) \
	do { \
		SETREG32(REG_BASE_ADDR0_Y, (phyaddr));\
	} while (0)

#define CMD_SET_BASE_ADDR1(phyaddr) \
	do { \
		SETREG32(REG_BASE_ADDR1_Y, (phyaddr));\
	} while (0)
#define CMD_SET_BRACKET_RATIO(ratio1, ratio2) \
	do { \
		SETREG16(ISP_BRACKET_RATIO1, ratio1); \
		SETREG16(ISP_BRACKET_RATIO2, ratio2); \
	} while (0)

/* offline parameters */
#define ISP_OFFLINE_INPUT_WIDTH                 (0x1e902)
#define ISP_OFFLINE_INPUT_HEIGHT                (0x1e904)
#define ISP_OFFLINE_MAC_READ_MEM_WIDTH          (0x1e906)
#define ISP_OFFLINE_OUTPUT_WIDTH                (0x1e908)
#define ISP_OFFLINE_OUTPUT_HEIGHT               (0x1e90a)
#define ISP_OFFLINE_MAC_WRITE_MEM_WIDTH         (0x1e90c)
#define ISP_OFFLINE_INPUT_BASE_ADDR             (0x1e910)
#define ISP_OFFLINE_INPUT_BASE_ADDR_RIGHT       (0x1e914)
#define ISP_OFFLINE_OUTPUT_BASE_ADDR            (0x1e918)
#define ISP_OFFLINE_OUTPUT_BASE_ADDR_UV         (0x1e91c)
#define ISP_OFFLINE_OUTPUT_BASE_ADDR_RIGHT      (0x1e920)
#define ISP_OFFLINE_OUTPUT_BASE_ADDR_RIGHT_UV   (0x1e924)
#define ISP_OFFLINE_FUNCTION_CONTROL            (0x1e929)
#define ISP_OFFLINE_SCALE_DOWN_H_RATIO_1        (0x1e92a)
#define ISP_OFFLINE_SCALE_DOWN_H_RATIO_2        (0x1e92b)
#define ISP_OFFLINE_SCALE_DOWN_V_RATIO_1        (0x1e92c)
#define ISP_OFFLINE_SCALE_DOWN_V_RATIO_2        (0x1e92d)
#define ISP_OFFLINE_SCALE_UP_H_RATIO            (0x1e92e)
#define ISP_OFFLINE_SCALE_UP_V_RATIO            (0x1e930)
#define ISP_OFFLINE_CROP_H_START                (0x1e932)
#define ISP_OFFLINE_CROP_V_START                (0x1e934)
#define ISP_OFFLINE_CROP_WIDTH                  (0x1e936)
#define ISP_OFFLINE_CROP_HEIGHT                 (0x1e938)

/* offline register register operation*/
#define CMD_SET_ISP_IN_FMT_SIZE_OFFLINE(fmt, width, height) \
	do { \
		SETREG8(ISP_INPUT_FORMAT, fmt); \
		SETREG16(ISP_OFFLINE_INPUT_WIDTH, (width));  \
		SETREG16(ISP_OFFLINE_INPUT_HEIGHT, (height)); \
	} while (0)

#define CMD_SET_ISP_OUT_FMT_SIZE_OFFLINE(fmt, width, height) \
	do { \
		SETREG8(ISP_OUTPUT_FORMAT, fmt); \
		SETREG16(ISP_OFFLINE_OUTPUT_WIDTH, (width));   \
		SETREG16(ISP_OFFLINE_OUTPUT_HEIGHT, (height)); \
	} while (0)

#define CMD_SET_READ_MEM_STRIDE_OFFLINE(stride)\
		SETREG16(ISP_OFFLINE_MAC_READ_MEM_WIDTH, (stride));

#define CMD_SET_WRITE_MEM_STRIDE_OFFLINE(stride)\
		SETREG16(ISP_OFFLINE_MAC_WRITE_MEM_WIDTH, (stride));

#define CMD_SET_IN_BASE_ADDR_OFFLINE(left, addr) \
	do { \
		if (left == BUF_LEFT) {\
			SETREG32(ISP_OFFLINE_INPUT_BASE_ADDR, (addr)); \
		} else {\
			SETREG32(ISP_OFFLINE_INPUT_BASE_ADDR_RIGHT, (addr)); \
		} \
	} while (0)

#define CMD_SET_OUT_BASE_ADDR_OFFLINE(left, y_addr, uv_addr) \
	do { \
		if (BUF_LEFT == left) { \
			SETREG32(ISP_OFFLINE_OUTPUT_BASE_ADDR, (y_addr));      \
			SETREG32(ISP_OFFLINE_OUTPUT_BASE_ADDR_UV, (uv_addr)); \
		} else {\
			SETREG32(ISP_OFFLINE_OUTPUT_BASE_ADDR_RIGHT, (y_addr));     \
			SETREG32(ISP_OFFLINE_OUTPUT_BASE_ADDR_RIGHT_UV, (uv_addr)); \
		} \
	} while (0)

#define CMD_SET_ISP_FUNCTION_CTRL_OFFLINE(value) \
		SETREG8(ISP_OFFLINE_FUNCTION_CONTROL, (value) & 0xff);

#define CMD_SET_SCALE_DOWN_RATIO_1_OFFLINE(dcw_h, dcw_v) \
	do { \
		SETREG8(ISP_OFFLINE_SCALE_DOWN_H_RATIO_1, (dcw_h) & 0xff); \
		SETREG8(ISP_OFFLINE_SCALE_DOWN_V_RATIO_1, (dcw_v) & 0xff); \
	} while (0)

#define CMD_SET_SCALE_DOWN_RATIO_2_OFFLINE(nscale_h, nscale_v) \
	do { \
		SETREG8(ISP_OFFLINE_SCALE_DOWN_H_RATIO_2, (nscale_h) & 0xff); \
		SETREG8(ISP_OFFLINE_SCALE_DOWN_V_RATIO_2, (nscale_v) & 0xff); \
	} while (0)

#define CMD_SET_SCALE_UP_RATIO_OFFLINE(nscale_h, nscale_v) \
	do { \
		SETREG8(ISP_OFFLINE_SCALE_UP_H_RATIO, ((nscale_h) >> 8) & 0x1); \
		SETREG8(ISP_OFFLINE_SCALE_UP_H_RATIO + 1, (nscale_h) & 0xff);   \
		SETREG8(ISP_OFFLINE_SCALE_UP_V_RATIO, ((nscale_v) >> 8) & 0x1); \
		SETREG8(ISP_OFFLINE_SCALE_UP_V_RATIO + 1, (nscale_v) & 0xff);   \
	} while (0)

#define CMD_SET_YUV_CROP_START_POS_OFFLINE(x, y) \
	do { \
		SETREG16(ISP_OFFLINE_CROP_H_START, (x)); \
		SETREG16(ISP_OFFLINE_CROP_V_START, (y)); \
	} while (0)

#define CMD_SET_YUV_CROP_SIZE_OFFLINE(width, height) \
	do { \
		SETREG16(ISP_OFFLINE_CROP_WIDTH, (width)); \
		SETREG16(ISP_OFFLINE_CROP_HEIGHT, (height)); \
	} while (0)

/* hdr parameters */
#define ISP_HDR_INPUT_WIDTH             (0x1e902)
#define ISP_HDR_INPUT_HEIGHT            (0x1e904)
#define ISP_HDR_MAC_READ_WIDTH          (0x1e906)
#define ISP_HDR_OUTPUT_WIDTH            (0x1e908)
#define ISP_HDR_OUTPUT_HEIGHT           (0x1e90a)
#define ISP_HDR_MAC_WRITE_WIDTH         (0x1e90c)
#define ISP_HDR_PROCESS_CONTROL         (0x1e90e)
#define ISP_HDR_INPUT_BASE_ADDR_LONG    (0x1e910)
#define ISP_HDR_INPUT_BASE_ADDR_SHORT   (0x1e914)
#define ISP_HDR_OUTPUT_BASE_ADDR        (0x1e918)
#define ISP_HDR_OUTPUT_BASE_ADDR_UV     (0x1e91C)
#define ISP_HDR_LONG_EXPOSURE           (0x1e920)
#define ISP_HDR_LONG_GAIN               (0x1e922)
#define ISP_HDR_SHORT_EXPOSURE          (0x1e924)
#define ISP_HDR_SHORT_GAIN              (0x1e926)
#define ISP_HDR_FUNCTION_CONTROL        (0x1e929)
#define ISP_HDR_SCALE_DOWN_H_RATIO1     (0x1e92a)
#define ISP_HDR_SCALE_DOWN_H_RATIO2     (0x1e92b)
#define ISP_HDR_SCALE_DOWN_V_RATIO1     (0x1e92c)
#define ISP_HDR_SCALE_DOWN_V_RATIO2     (0x1e92d)
#define ISP_HDR_SCALE_UP_H_RATIO        (0x1e92e)
#define ISP_HDR_SCALE_UP_V_RATIO        (0x1e930)
#define ISP_HDR_CROP_H_START            (0x1e932)
#define ISP_HDR_CROP_V_START            (0x1e934)
#define ISP_HDR_CROP_WIDTH              (0x1e936)
#define ISP_HDR_CROP_HEIGHT             (0x1e938)

#define ISP_HDR_IMAGE_NUMBER		2

/* HDR register operation */
#define CMD_SET_HDR_IN_SIZE(fmt, width, height) \
	do { \
		SETREG8(ISP_INPUT_FORMAT, fmt);		\
		SETREG16(ISP_HDR_INPUT_WIDTH, width);	\
		SETREG16(ISP_HDR_INPUT_HEIGHT, height);	\
	} while (0)

#define CMD_SET_HDR_OUT_SIZE(fmt, width, height)	\
	do { \
		SETREG8(ISP_OUTPUT_FORMAT, fmt);	\
		SETREG16(ISP_HDR_OUTPUT_WIDTH, width);	\
		SETREG16(ISP_HDR_OUTPUT_HEIGHT, height);\
	} while (0)

#define CMD_SET_HDR_READ_MEM_STRIDE(stride)	\
		SETREG16(ISP_HDR_MAC_READ_WIDTH, stride);

#define CMD_SET_HDR_WRITE_MEME_STRIDE(stride)	\
		SETREG16(ISP_HDR_MAC_WRITE_WIDTH, stride);

#define CMD_SET_HDR_ISP_FUNCTION_CTRL(value)	\
		SETREG8(ISP_HDR_FUNCTION_CONTROL, (value) & 0xff);

#define CMD_SET_HDR_SCALE_DOWN_RATIO_1(nscale_h, nscale_v) \
	do { \
		SETREG8(ISP_HDR_SCALE_DOWN_H_RATIO1, (nscale_h) & 0xff); \
		SETREG8(ISP_HDR_SCALE_DOWN_V_RATIO1, (nscale_v) & 0xff); \
	} while (0)

#define CMD_SET_HDR_SCALE_DOWN_RATIO_2(nscale_h, nscale_v) \
	do { \
		SETREG8(ISP_HDR_SCALE_DOWN_H_RATIO2, (nscale_h) & 0xff); \
		SETREG8(ISP_HDR_SCALE_DOWN_V_RATIO2, (nscale_v) & 0xff); \
	} while (0)

#define CMD_SET_HDR_SCALE_UP_RATIO(nscale_h, nscale_v) \
	do { \
		SETREG16(ISP_HDR_SCALE_UP_H_RATIO, nscale_h);	\
		SETREG16(ISP_HDR_SCALE_UP_V_RATIO, nscale_v);	\
	} while (0)

#define CMD_SET_HDR_YUV_CROP_START_POS(x, y) \
	do { \
		SETREG16(ISP_HDR_CROP_H_START, (x)); \
		SETREG16(ISP_HDR_CROP_V_START, (y)); \
	} while (0)

#define CMD_SET_HDR_YUV_CROP_SIZE(width, height) \
	do { \
		SETREG16(ISP_HDR_CROP_WIDTH, (width)); \
		SETREG16(ISP_HDR_CROP_HEIGHT, (height)); \
	} while (0)

#define CMD_SET_HDR_OUT_BASE_ADDR(y_addr, uv_addr) \
	do { \
		SETREG32(ISP_HDR_OUTPUT_BASE_ADDR, (y_addr));	\
		SETREG32(ISP_HDR_OUTPUT_BASE_ADDR_UV, (uv_addr)); \
	} while (0)

#define HDR_GET_EXPOSURE_AND_GAIN(long_exposure, short_exposure, long_gain, short_gain)	\
	do { \
		GETREG16(ISP_FIRST_FRAME_EXPOSURE, long_exposure); \
		GETREG16(ISP_SECOND_FRAME_EXPOSURE, short_exposure); \
		GETREG16(ISP_FIRST_FRAME_GAIN, long_gain); \
		GETREG16(ISP_SECOND_FRAME_GAIN, short_gain); \
	} while (0)

#define CMD_SET_HDR_EXPOSURE_AND_GAIN(long_exposure, short_exposure, long_gain, short_gain)	\
	do { \
		SETREG16(ISP_HDR_LONG_EXPOSURE, long_exposure);	 \
		SETREG16(ISP_HDR_SHORT_EXPOSURE, short_exposure); \
		SETREG16(ISP_HDR_LONG_GAIN, long_gain);	 \
		SETREG16(ISP_HDR_SHORT_GAIN, short_gain); \
	} while (0)

#define REG_ISP_BANDFILTER_FLAG         (0x1c13f)
#define REG_ISP_BANDFILTER_EN           (0x1c140)
#define REG_ISP_BANDFILTER_SHORT_EN     (0x1c141)

/* command set CMD_ZOOM_IN_MODE relative parameters */
/* REG1 bit[0] : 1 for high quality, 0 for save power mode */
#define HIGH_QUALITY_MODE               (0x1)
#define SAVE_POWER_MODE                 (0x0)

/* ISP registers */
#define REG_ISP_TOP0                    (0x65000)
#define REG_ISP_TOP2                    (0x65002)

/*ISP work mode; Combine Mode (HDR) */
#define REG_ISP_TOP4                    (0x65004)

#define REG_ISP_TOP5                    (0x65005)
#define SDE_MANUAL_OFFSET_ENABLE	(1<<3)

#define REG_ISP_TOP6                    (0x65006)

/*NV12-->NV21, please write 0x40 to 65007 */
#define REG_ISP_TOP7                    (0x65007)

/* YUV DCW & YUV scale down */
#define REG_ISP_TOP35                   (0x65023)

/* ISP binning reg[1:0] raw dcw scale shift */
#define REG_ISP_BIN_CTRL0               (0x65080)

/*
 * ISP input size:
 * horizol: 0x65010 [12:8], 0x65011 [7:0]
 * vertical:0x65012 [12:8], 0x65013 [7:0]
 * ISP output size
 * horizol: 0x65014 [12:8], 0x65015 [7:0]
 * vertical:0x65016 [12:8], 0x65017 [7:0]
 */
#define REG_ISP_IN_WIDTH                (0x65010)
#define REG_ISP_IN_HEIGHT               (0x65012)
#define REG_ISP_OUT_WIDTH               (0x65014)
#define REG_ISP_OUT_HEIGHT              (0x65016)

/*
 * ISP LENC horizontal offset:
 * high:	0x65018
 * low:		0x65019
 * ISP LENC verital offset:
 * high:	0x6501a
 * low:		0x6501b
 */
#define REG_ISP_LENC_HORIZONTAL_OFFSET	(0x65018)
#define REG_ISP_LENC_VERITAL_OFFSET		(0x6501a)

/*
 * raw scale output
 * horizontal: 0x65032[8], 0x65033[7:0]
 * vertical:   0x65034[8], 0x65035[7:0]
 */
#define REG_ISP_RAW_DCW_OUT_H           (0x65032)
#define REG_ISP_RAW_DCW_OUT_V           (0x65034)

/*
 * down_nscale_x: 0x65024[7:0]
 * down_nscale_y: 0x65025[7:0]
 * up_nscale_x: 0x65026[8], 0x65027[7:0]
 * up_nscale_y: 0x65028[8], 0x65029[7:0]
 */
#define REG_ISP_SCALE_DOWN_X            (0x65024)
#define REG_ISP_SCALE_DOWN_Y            (0x65025)
#define REG_ISP_SCALE_UP_Y              (0x65026)
#define REG_ISP_SCALE_UP_X              (0x65028)

/*
 * manual awb reg
 */
#define MANUAL_AWB_GAIN_B				(0x65300)
#define MANUAL_AWB_GAIN_GB				(0x65302)
#define MANUAL_AWB_GAIN_GR				(0x65304)
#define MANUAL_AWB_GAIN_R				(0x65306)

/*
 * YUV crop register
 * yuv crop_x :0x650f0[12:8], 0x650f1[7:0]
 * yuv crop_y :0x650f2[12:8], 0x650f3[7:0]
 * yuv crop_w :0x650f4[12:8], 0x650f5[7:0]
 * yuv crop_h :0x650f6[12:8], 0x650f7[7:0]
 */
#define REG_ISP_YUV_CROP_LEFT           (0x650f0)
#define REG_ISP_YUV_CROP_TOP            (0x650f2)
#define REG_ISP_YUV_CROP_WIDTH          (0x650f4)
#define REG_ISP_YUV_CROP_HEIGHT         (0x650f6)


typedef enum {
	FLASH_AWBTEST_POLICY_FIXED = 0,
	FLASH_AWBTEST_POLICY_FREEGO,
} FLASH_AWBTEST_POLICY;

typedef struct _irq_reg {
	u32 irq_status;
	u32 mac_irqstatus_h;
	u32 mac_irqstatus_l;
} irq_reg_t;

typedef struct _aecawb_step {
	u8 stable_range0; /* UNSTABLE2STABLE_RANGE: 0x1c14a */
	u8 stable_range1; /* STABLE2UNSTABLE_RANGE: 0x1c14b */
	u8 fast_step; /* 0x1c14c */
	u8 slow_step; /* 0x1c14e */
	u8 awb_step; /* 0x1c184 */
} aecawb_step_t;

typedef struct _flash_weight {
	u32 preflash_env;
	u32 preflash_flash;
	u32 capflash_env;
	u32 capflash_flash;
} flash_weight_t;

typedef struct _aec_data {
	u32 gain;
	u32 expo;
	u32 lum;
	u32 lum_max; /* using for save max lum in 4x4 raw lum stat window */
	u32 lum_sum;
} aec_data_t;

typedef struct _expo_table {
	u32 expo; /* in fact this is the reciprocal of expo */
	u32 iso;
} expo_table_t;

typedef struct _isp_hw_data {

	struct semaphore sem_cmd_done;
	struct semaphore sem_wrt_done;
	spinlock_t irq_status_lock;

	/* camera work mode */
	camera_state cur_state;
	struct i2c_t i2c_config;

	u32 base;
	u32 irq;
	irq_reg_t irq_val;
	struct clk *isp_clk;
	struct clk *ispmipi_clk;
	struct clk *isp_aclk;
	struct regulator *inter_ldo;

	u32 pix_fmt;
	u32 uoffset;
	u32 voffset;
	u8 command_set_id;

	aec_data_t preview_ae;
	awb_gain_t preview_awb;
	aec_data_t preflash_ae;
	u32 preview_ratio_lum;

	aecawb_step_t aecawb_step;
	awb_gain_t flash_preset_awb;
	bool flash_resume;
	bool ae_resume;
	awb_gain_t led_awb[2];
	FLASH_AWBTEST_POLICY awb_test; /* used to save policy of flash awb test. */

	isp_process_mode_t process_mode;

	volatile u8 aec_cmd_id;
	u32 frame_count;

	data_queue_t *data_queue_ref;

	offline_state_t	offline_state;
	bool preview_stop;
	u8 fps_max;
	u8 fps_min;
	u8 fps_mid;

	camera_flash	flash_mode;

	u32 ratio_factor;
} isp_hw_data_t;

extern isp_hw_data_t isp_hw_data;

static inline void SETREG8(u32 reg, u8 value) {
	(reg >= COMMAND_BUFFER_END) ?
		iowrite8_mutex(value, isp_hw_data.base + reg) : 
		iowrite8_mutex(value, isp_hw_data.base + BIG_ENDIAN(reg));
}

#define SETREG16(reg, value) \
	do { \
		SETREG8((reg), ((value) >> 8) & 0xff); \
		SETREG8((reg) + 1, (value) & 0xff);    \
	} while (0)

#define SETREG32(reg, value) \
	do { \
		SETREG8(reg,     ((value) >> 24) & 0xff); \
		SETREG8(reg + 1, ((value) >> 16) & 0xff); \
		SETREG8(reg + 2, ((value) >> 8) & 0xff);  \
		SETREG8(reg + 3, ((value) >> 0) & 0xff);  \
	} while (0)

#define REG_SET_ISP_IN_SIZE(width, height) \
	do { \
		SETREG16(REG_ISP_IN_WIDTH, width & 0x0FFF);	\
		SETREG16(REG_ISP_IN_HEIGHT, height & 0x07FF);\
	} while (0)

#define REG_SET_ISP_OUT_SIZE(width, height) \
	do { \
		SETREG16(REG_ISP_OUT_WIDTH, width & 0x0FFF);   \
		SETREG16(REG_ISP_OUT_HEIGHT, height & 0x07FF); \
	} while (0)

#define REG_SET_RAW_DCW_ENABLE(enable) \
	(enable) ? \
	SETREG8(REG_ISP_TOP0, GETREG8(REG_ISP_TOP0) | 0x40) : \
	SETREG8(REG_ISP_TOP0, GETREG8(REG_ISP_TOP0) & (~0x40))

#define REG_SET_RAW_DCW_RATIO(raw_dcw_e) \
	SETREG8(REG_ISP_BIN_CTRL0, (GETREG8(REG_ISP_BIN_CTRL0) & (~0x03)) | ((raw_dcw_e) & 0x03))

#define REG_SET_YUV_SCALE_UP_ENABLE(enable) \
	(enable) ? \
	SETREG8(REG_ISP_TOP2, (GETREG8(REG_ISP_TOP2) | 0x20) & (~0x08)) : \
	SETREG8(REG_ISP_TOP2, (GETREG8(REG_ISP_TOP2) | 0x08) & (~0x20))

#define REG_SET_YUV_DCW_RATIO(dcw_e_x, dcw_e_y) \
	SETREG8(REG_ISP_TOP35, \
	((GETREG8(REG_ISP_TOP35) & 0x3c) | ((dcw_e_x) & 0x3) << 6) | ((dcw_e_y) & 0x3))

#define REG_SET_YUV_DOWN_NSCALE(down_nscale_x, down_nscale_y) \
	do { \
		SETREG8(REG_ISP_SCALE_DOWN_X, (down_nscale_x)); \
		SETREG8(REG_ISP_SCALE_DOWN_Y, (down_nscale_y)); \
	} while (0)

#define REG_SET_YUV_UP_NSCALE(up_nscale_x, up_nscale_y) \
	do { \
		SETREG16(REG_ISP_SCALE_UP_X, up_nscale_x & 0x01FF); \
		SETREG16(REG_ISP_SCALE_UP_Y, up_nscale_y & 0x01FF); \
	} while (0)

#define REG_SET_SCALE_OUT_SIZE(width, height) \
	do { \
		SETREG16(REG_ISP_RAW_DCW_OUT_H, width & 0x1FFF);   \
		SETREG16(REG_ISP_RAW_DCW_OUT_V, height & 0x0FFF);     \
	} while (0)

#define REG_SET_YUV_CROP_POSE(left, top) \
	do { \
		SETREG16(REG_ISP_YUV_CROP_LEFT, left & 0x1FFF);   \
		SETREG16(REG_ISP_YUV_CROP_TOP, (top) & 0x0FFF);     \
	} while (0)

#define REG_SET_YUV_CROP_SIZE(width, height) \
	do { \
		SETREG16(REG_ISP_YUV_CROP_WIDTH, width & 0x1FFF);   \
		SETREG16(REG_ISP_YUV_CROP_HEIGHT, height & 0x0FFF);    \
	} while (0)

#define REG_UV_ORDER_V_AHEAD			(0x40)
#define REG_UV_ORDER_U_AHEAD			(0x0)

#define REG_ISP_SOFT_STANDBY            (0x60100)
#define REG_ISP_SOFT_RST                (0x60103)
#define DO_SOFTWARE_STAND_BY            0x1
#define MCU_SOFT_RST                    0x1
#define DO_SOFT_RST                     0x1
#define RELEASE_SOFT_RST                0x0

#define REG_ISP_INPUT_CHANNEL           (0x63108)

/*
 * When generating SetFormat/Capture command set, if 0x63905[7] is 1,
 * command set won't care about sensor interrupt,
 * it can solve this issue:set format add an option to write ISP registers directly in order to handle first preview
 * otherwise it is normal as now.

 * When generating SetFormat/Capture command set, if 0x63905[6] is 1,
 * command set will take care MAC interrupt as description, otherwise it is normal as now.

 * if 0x63905[5] is 1, online capture cmdset also use exposure ratio
 */

#define ISP_SENSOR_INT_DISABLE		(1<<7)
#define ISP_SENSOR_INT_ENABLE		(0<<7)
#define ISP_MACINT_SET_ENABLE		(1<<6)
#define ISP_MACINT_SET_DISABLE		(0<<6)
#define ISP_EXPO_RATIO_ENABLE		(0<<5)
#define ISP_EXPO_RATIO_DISABLE		(1<<5)
#define ISP_IDI_CONFIG_ENABLE		(1<<4)

#define ISP_SDE_ENABLE  			(1<<2)
#define ISP_NEGATIVE_EFFECT_ENABLE  (1<<6)
#define ISP_MONO_EFFECT_ENABLE  		(1<<5)
#define ISP_FIX_V_ENABLE			(1<<4)
#define ISP_FIX_U_ENABLE			(1<<3)
#define ISP_CONTRAST_ENABLE  			(1<<2)
#define ISP_BRIGHTNESS_ENABLE  		(1<<2)
#define ISP_SATURATION_ENABLE  		(1<<1)
#define ISP_BRIGHTNESS_SIGN_NEGATIVE	(1<<3)

#define ISP_INPUT_CHANNEL_DVP           0x0
#define ISP_INPUT_CHANNEL_MAC           0x2
#define ISP_INPUT_CHANNEL_MIPI1         0x4
#define ISP_INPUT_CHANNEL_MIPI2         0x8

#define REG_ISP_CAPTURE_SKIP_FRAME		(0x63908)

#define REG_ISP_GAIN_EFFECT_MODE	(0x1d8d7)
#define SENSOR_GAIN_EFFECT_NEXT	1
#define SENSOR_GAIN_EFFECT_NEXT2	0

#define REG_ISP_GENERAL_PURPOSE_REG1    (0x63910)
#define REG_ISP_GENERAL_PURPOSE_REG2    (0x63912)
#define REG_ISP_GENERAL_PURPOSE_REG3    (0x63913)
#define REG_ISP_GENERAL_PURPOSE_REG7    (0x63917)

#define REG_ISP_CLK_DIVIDER             (0x63023)

#define REG_ISP_GPIO_SEL                (0x6302f)
#define REG_ISP_GPIO_CTL_H		(0x63030)
#define REG_ISP_GPIO_CTL_L		(0x63031)

#define REG_ISP_BYTE_SWITCH             (0x63b35)

#define REG_ISP_SDE_CTRL		(0x65b00)
#define REG_ISP_SDE_U_SATURATION	(0x65b03)
#define REG_ISP_SDE_V_SATURATION	(0x65b04)
#define REG_ISP_SDE_U_REG			(0x65b03)
#define REG_ISP_SDE_V_REG			(0x65b04)
#define REG_ISP_SDE_CONTRAST		(0x65b06)
#define REG_ISP_SDE_BRIGHTNESS		(0x65b07)
#define REG_ISP_SDE_SIGN_SET		(0x65b08)

#define REG_ISP_SDE_YOFFSET			(0x65b05)
#define REG_ISP_SDE_YGAIN			(0x65b06)
#define REG_ISP_SDE_YBRIGHT			(0x65b07)

#define REG_ISP_SHARPNESS			(0x6560d)

#define REG_ISP_SHARPNESS_REG1		(0x65608)
#define REG_ISP_SHARPNESS_REG2		(0x65609)
#define REG_ISP_SHARPNESS_REG3		(0x6560c)
#define REG_ISP_SHARPNESS_REG4		(0x6560d)


#define SDE_BRIGHTNESS_CAPTURE_STEP		(0x10)
#define SDE_BRIGHTNESS_CAPTURE_H2		(SDE_BRIGHTNESS_CAPTURE_STEP * 2)
#define SDE_BRIGHTNESS_CAPTURE_H1		(SDE_BRIGHTNESS_CAPTURE_STEP)
#define SDE_BRIGHTNESS_CAPTURE_H0		(0x00)
#define SDE_BRIGHTNESS_CAPTURE_L1		(SDE_BRIGHTNESS_CAPTURE_STEP)
#define SDE_BRIGHTNESS_CAPTURE_L2		(SDE_BRIGHTNESS_CAPTURE_STEP * 2)

#define SDE_BRIGHTNESS_PREVIEW_STEP		(0x10)
#define SDE_BRIGHTNESS_PREVIEW_H0		(0x12) /* 0x10->0x12, be careful for below */
#define SDE_BRIGHTNESS_PREVIEW_H1		(SDE_BRIGHTNESS_PREVIEW_H0 + SDE_BRIGHTNESS_PREVIEW_STEP)
#define SDE_BRIGHTNESS_PREVIEW_H2		(SDE_BRIGHTNESS_PREVIEW_H0 + SDE_BRIGHTNESS_PREVIEW_STEP * 2)
#define SDE_BRIGHTNESS_PREVIEW_L1		(SDE_BRIGHTNESS_PREVIEW_H0 - SDE_BRIGHTNESS_PREVIEW_STEP)
#define SDE_BRIGHTNESS_PREVIEW_L2		(SDE_BRIGHTNESS_PREVIEW_STEP * 2 - SDE_BRIGHTNESS_PREVIEW_H0)

#define SDE_CONTRAST_CAPTURE_STEP		(0x08)
#define SDE_CONTRAST_CAPTURE_H0			(0x20)
#define SDE_CONTRAST_CAPTURE_H1			(SDE_CONTRAST_CAPTURE_H0 + SDE_CONTRAST_CAPTURE_STEP)
#define SDE_CONTRAST_CAPTURE_H2			(SDE_CONTRAST_CAPTURE_H0 + SDE_CONTRAST_CAPTURE_STEP * 2)
#define SDE_CONTRAST_CAPTURE_L1			(SDE_CONTRAST_CAPTURE_H0 - SDE_CONTRAST_CAPTURE_STEP)
#define SDE_CONTRAST_CAPTURE_L2			(SDE_CONTRAST_CAPTURE_H0 - SDE_CONTRAST_CAPTURE_STEP * 2)

#define SDE_CONTRAST_PREVIEW_STEP		(0x08)
#define SDE_CONTRAST_PREVIEW_H0			(0x1c) /* 0x1d->0x1c */
#define SDE_CONTRAST_PREVIEW_H1			(SDE_CONTRAST_PREVIEW_H0 + SDE_CONTRAST_PREVIEW_STEP)
#define SDE_CONTRAST_PREVIEW_H2			(SDE_CONTRAST_PREVIEW_H0 + SDE_CONTRAST_PREVIEW_STEP * 2)
#define SDE_CONTRAST_PREVIEW_L1			(SDE_CONTRAST_PREVIEW_H0 - SDE_CONTRAST_PREVIEW_STEP)
#define SDE_CONTRAST_PREVIEW_L2			(SDE_CONTRAST_PREVIEW_H0 - SDE_CONTRAST_PREVIEW_STEP * 2)

/* 0x63b35
[6] r_byte_switch2
[5] r_byte_switch1
[4] r_rb_switch // for RGB565
[3] w_byte_switch2
[2] w_byte_switch1
[1] w_rb_switch2 // for RGB888
[0] w_rb_switch1 // for RGB565

w_byte_switch1: ABCDEFGH => BADCFEHG
w_byte_switch2: ABCDEFGH => ADCBEHGF
The switches are pipelined:  w_byte_switch2 -> w_byte_switch1.

w_rb_switch1: switch R and B for RGB565
w_rb_switch2: switch R and B for RGB888

r_byte_switch1: ABCDEFGH => BADCFEHG
r_byte_switch2: ABCDEFGH => ADCBEHGF
The switches are pipelined:  r_byte_switch1-> r_byte_switch2

r_rb_switch: switch R and B for RGB565
*/

#define REG_SET_BYTESWITCH(enable) \
		(enable) ? \
		SETREG8(REG_ISP_BYTE_SWITCH, 0x04) : \
		SETREG8(REG_ISP_BYTE_SWITCH, 0x0)

#define REG_ISP_MEM_CTRL2		(0x63c13)
#define REG_ISP_MEM_OUTPUT_NUM		(0x63c15)

#define REG_ISP_INT_STAT                (0x6392b)
#define REG_ISP_INT_EN                  (0x63927)

/*note that: manuel is wrong!!! bit3 is cmd_set_done interrupt */
#define MASK_EOF_INT_ENABLE			(1 << 5)
#define MASK_SOF_INT_ENABLE			(1 << 4)
#define MASK_CMDSET_INT_ENABLE			(1 << 3)
#define MASK_MAC_INT_ENABLE				(1 << 1)
#define MASK_GPIO_INT_ENABLE			(1 << 0)

#define REG_ISP_MAC_INT_STAT_H          (0x63b32)
#define REG_ISP_MAC_INT_STAT_L          (0x63b33)
#define REG_ISP_MAC_INT_EN_H            (0x63b53)
#define REG_ISP_MAC_INT_EN_L            (0x63b54)
#define REG_ISP_INT_CLR_MSK             0x10
#define REG_ISP_FORMAT_CTRL             (0x63b34)

#define MASK_INT_FRAME_START            (1)
#define MASK_INT_DONE                   (1 << 1)

#define MASK_INT_WRITE_START0           (1)
#define MASK_INT_WRITE_DONE0            (1 << 1)
#define MASK_INT_DROP0                  (1 << 2)
#define MASK_INT_OVERFLOW0              (1 << 3)
#define MASK_INT_WRITE_START1           (1 << 4)
#define MASK_INT_WRITE_DONE1            (1 << 5)
#define MASK_INT_DROP1                  (1 << 6)
#define MASK_INT_OVERFLOW1              (1 << 7)

#define ISP_MAC_INT_H  (MASK_INT_FRAME_START | MASK_INT_DONE)
#define ISP_MAC_INT_L  (MASK_INT_WRITE_DONE0 | MASK_INT_WRITE_DONE1 \
				| MASK_INT_DROP0 | MASK_INT_DROP1 \
				| MASK_INT_OVERFLOW0 | MASK_INT_OVERFLOW1 \
				| MASK_INT_WRITE_START0 | MASK_INT_WRITE_START1)

#define REG_BASE_ADDR0_Y                (0x63b00)
#define REG_BASE_ADDR0_UV               (0x63b04)
#define REG_BASE_ADDR1_Y                (0x63b10)
#define REG_BASE_ADDR1_UV               (0x63b14)
#define REG_BASE_ADDR_READY             (0x63b30)
#define WRITE_ADDR0_READY               (1)
#define READ_ADDR0_READY                (1 << 2)
#define WRITE_ADDR1_READY               (1 << 1)
#define READ_ADDR1_READY                (1 << 3)
#define REG_BASE_WORKING_ADDR           (0x63b31)
#define REG_BASE_ADDR1_WRITING          (1 << 7)
#define REG_BASE_ADDR1_READING          (1 << 6)
#define REG_BASE_DROPING                (1 << 5)
#define	REG_SET_ADDR(reg_y, reg_uv, addr_y, addr_uv) \
	do {\
		SETREG32(reg_y, (addr_y));\
		SETREG32(reg_uv, (addr_uv));\
	} while (0)

#define REG_SET_ISP_IN_CHANNEL(sensor_type) \
	do { \
		switch (sensor_type) { \
		case MIPI1: \
			SETREG8(REG_ISP_INPUT_CHANNEL, ISP_INPUT_CHANNEL_MIPI1); \
			break; \
		case MIPI2:\
			SETREG8(REG_ISP_INPUT_CHANNEL, ISP_INPUT_CHANNEL_MIPI2); \
			break; \
		case DVP:\
			SETREG8(REG_ISP_INPUT_CHANNEL, ISP_INPUT_CHANNEL_DVP); \
			break; \
		default: \
			SETREG8(REG_ISP_INPUT_CHANNEL, ISP_INPUT_CHANNEL_MAC); \
			break; \
		} \
	} while (0)

#define REG_ISP_SCALEUP_X_H             (0x65026)
#define REG_ISP_SCALEUP_Y_L             (0x65027)
#define REG_ISP_CLK_USED_BY_MCU         (0x63042)

/*I2C control register */
#define REG_SCCB_MAST1_SPEED            (0x63600)
#define REG_SCCB_MAST1_SLAVE_ID         (0x63601)
#define REG_SCCB_MAST1_ADDRESS_H        (0x63602)
#define REG_SCCB_MAST1_ADDRESS_L        (0x63603)
#define REG_SCCB_MAST1_OUTPUT_DATA_H    (0x63604)
#define REG_SCCB_MAST1_OUTPUT_DATA_L    (0x63605)
#define REG_SCCB_MAST1_2BYTE_CONTROL    (0x63606)
#define REG_SCCB_MAST1_INPUT_DATA_H     (0x63607)
#define REG_SCCB_MAST1_INPUT_DATA_L     (0x63608)
#define REG_SCCB_MAST1_COMMAND          (0x63609)
#define REG_SCCB_MAST1_STATUS           (0x6360a)

#define REG_SCCB_MAST2_SPEED            (0x63700)
#define REG_SCCB_MAST2_SLAVE_ID         (0x63701)
#define REG_SCCB_MAST2_ADDRESS_H        (0x63702)
#define REG_SCCB_MAST2_ADDRESS_L        (0x63703)
#define REG_SCCB_MAST2_OUTPUT_DATA_H    (0x63704)
#define REG_SCCB_MAST2_OUTPUT_DATA_L    (0x63705)
#define REG_SCCB_MAST2_2BYTE_CONTROL    (0x63706)
#define REG_SCCB_MAST2_INPUT_DATA_H     (0x63707)
#define REG_SCCB_MAST2_INPUT_DATA_L     (0x63708)
#define REG_SCCB_MAST2_COMMAND          (0x63709)
#define REG_SCCB_MAST2_STATUS           (0x6370a)

#define REG_SCCB_FIRMWARE1_ID           (0x1c528)
#define REG_SCCB_FIRMWARE2_ID           (0x1c529)

#define REG_SCCB_BUS_MUTEX 		(0x1c4ee)

#define MASK_16BIT_ADDR_ENABLE          (1)
#define MASK_16BIT_DATA_ENABLE          (1 << 1)

#define SCCB_MASTER_BUSY                (1)
#define SCCB_MASTER_LOCK           (1)
#define SCCB_MASTER_UNLOCK      (0)

#define SCCB_BUS_MUTEX_WAIT			(1)
#define SCCB_BUS_MUTEX_NOWAIT		(0)

#define REG_GPIO_R_REQ_CTRL_72          (0x63d72)
#define REG_GPIO_R_REQ_CTRL_74          (0x63d74)

/* EV RATIO */
#define EV_RATIO_NUMERATOR		7	/* EV RATIO numerator */
#define EV_RATIO_DENOMINATOR	10	/* EV RATIO denominator */
#define EV_RATIO_DIVIDEND		0x100

#define ISP_EXPOSURE_RATIO_MAX		0xff00

/* EV RATIO */
#define EV_BRACKET_RATIO_NUMERATOR		635 /* EV BRACKET RATIO numerator */
#define EV_BRACKET_RATIO_DENOMINATOR		1000 /* EV BRACKET RATIO denominator */

/*Anti hand shaking registers*/
#define REG_ISP_ANTI_SHAKING_ENABLE		(0x1c4d8)

#define REG_ISP_ANTI_SHAKING_ABSSTART_POSITION_H		(0x1c764)
#define REG_ISP_ANTI_SHAKING_ABSSTART_POSITION_V		(0x1c766)

#define REG_ISP_ANTI_SHAKING_BLOCK_SIZE		(0x1c244)

#define REG_ISP_ANTI_SHAKING_WIN_LEFT	(0x1c768)
#define REG_ISP_ANTI_SHAKING_WIN_TOP		(0x1c76a)

/* for isp tune ops added registers begin */
#define REG_ISP_MAX_GAIN				(0x1c150)
#define REG_ISP_MAX_GAIN_SHORT		(0x1c152)
#define REG_ISP_MIN_GAIN				(0x1c154)
#define REG_ISP_MIN_GAIN_SHORT		(0x1c156)
#define REG_ISP_MANUAL_GAIN			(0x1c170)
#define REG_ISP_MANUAL_GAIN_SHORT		(0x1c172)

#define REG_ISP_MAX_EXPOSURE			(0x1c158)
#define REG_ISP_MAX_EXPOSURE_SHORT		(0x1c15a)
#define REG_ISP_MIN_EXPOSURE			(0x1c15c)
#define REG_ISP_MIN_EXPOSURE_SHORT		(0x1c15e)
#define REG_ISP_MANUAL_EXPOSURE			(0x1c16a)
#define REG_ISP_MANUAL_EXPOSURE_SHORT		(0x1c16e)

/* in AP write sensor's expo/gain mode, read from below writeback registers. */
#define REG_ISP_WRITEBACK_EXPO			(0x1c79c)
#define REG_ISP_WRITEBACK_GAIN			(0x1c7a4)

/* in ISP write sensor's expo/gain mode, read from below registers. */
#define REG_ISP_RESERVED_EXPO			(0x1c168)
#define REG_ISP_RESERVED_GAIN			(0x1c170)

#define REG_ISP_AWB_MANUAL_ENABLE		(0x65320)
#define REG_ISP_AWB_METHOD_TYPE		(0x1c17c)

#define REG_ISP_AWB_MANUAL_GAIN_BLUE(group)		(0x1c4f0+6*(group))
#define REG_ISP_AWB_MANUAL_GAIN_GREEN(group)	(0x1c4f2+6*(group))
#define REG_ISP_AWB_MANUAL_GAIN_RED(group)		(0x1c4f4+6*(group))

#define REG_ISP_WIN_LUM(index)		(0x1cb9c + 4 * (index))

#define REG_ISP_AWB_GAIN_B		(0x65300)
#define REG_ISP_AWB_GAIN_GB		(0x65302)
#define REG_ISP_AWB_GAIN_GR		(0x65304)
#define REG_ISP_AWB_GAIN_R		(0x65306)

#define REG_ISP_AWB_ORI_GAIN_B		(0x1C734)
#define REG_ISP_AWB_ORI_GAIN_G		(0x1C736)
#define REG_ISP_AWB_ORI_GAIN_R		(0x1C738)

#define REG_ISP_LENS_CP_ARRAY_LONG		(0x1c264)
#define REG_ISP_LENS_CP_ARRAY_SHORT	(0x1c366)
#define LENS_CP_ARRAY_BYTES				(86*3)

#define REG_ISP_CCM_MATRIX			(0x1c1d8)
#define CCM_MATRIX_ARRAY_SIZE16		(54)

#define REG_ISP_AWB_CTRL		(0x66206L)
#define AWB_CTRL_ARRAY_BYTES	11

#define REG_ISP_CCM_CENTERCT_THRESHOLDS	(0x1c1c8)
#define REG_ISP_CCM_LEFTCT_THRESHOLDS		(0x1c1cc)
#define REG_ISP_CCM_RIGHTCT_THRESHOLDS		(0x1c1d0)

#define REG_ISP_LENS_CT_THRESHOLDS		(0x1c254)
#define LENS_CT_THRESHOLDS_SIZE16		4

#define REG_ISP_LENC_BRHSCALE	(0x65102)
#define REG_ISP_LENC_BRVSCALE	(0x65104)
#define REG_ISP_LENC_GHSCALE		(0x65106)
#define REG_ISP_LENC_GVSCALE		(0x65108)

/* scene add by j00179721*/
#define REG_ISP_UV_ADJUST	0x1c4e8
#define REG_ISP_UV_SATURATION	0x1c4eb

/* aecagc mask add by j00179721*/
#define REG_ISP_AEC_ADDR0		0x1c52c
#define REG_ISP_AEC_ADDR1		0x1c52e
#define REG_ISP_AEC_ADDR2		0x1c530

#define REG_ISP_AGC_ADDR0		0x1c538
#define REG_ISP_AGC_ADDR1		0x1c53a

#define REG_ISP_AEC_MASK_0		0x1c55c
#define REG_ISP_AEC_MASK_1		0x1c55d
#define REG_ISP_AEC_MASK_2		0x1c55e

#define REG_ISP_AGC_MASK_H		0x1c562
#define REG_ISP_AGC_MASK_L		0x1c563

#define REG_ISP_AGC_SENSOR_TYPE		0x1c5de

/* interrupts bits define */
#define ISP_MAC_INT                     (1 << 1)
#define ISP_COMMAND_DONE_INT            (1 << 3)	/*note that: manuel is wrong!!! bit3 is cmd_set_done interrupt */

#define ISP_READ_DONE_INT               (1 << 1)
#define ISP_FRAME_START_INT             (1)

#define ISP_OVERFLOW_INT1               (1 << 7)
#define ISP_FRAME_DROP_INT1             (1 << 6)
#define ISP_WRITE_DONE_INT1             (1 << 5)
#define ISP_WRITE_START_INT1            (1 << 4)

#define ISP_OVERFLOW_INT0               (1 << 3)
#define ISP_FRAME_DROP_INT0             (1 << 2)
#define ISP_WRITE_DONE_INT0             (1 << 1)
#define ISP_WRITE_START_INT0            (1)

#define I2C_MAX 254

#define get_writeback_expo() \
	(GETREG8(REG_ISP_AECAGC_WRITESENSOR_ENABLE) ? \
	((GETREG8(REG_ISP_RESERVED_EXPO) << 24) | (GETREG8(REG_ISP_RESERVED_EXPO + 1) << 16) | \
	(GETREG8(REG_ISP_RESERVED_EXPO + 2) << 8) | (GETREG8(REG_ISP_RESERVED_EXPO + 3))) : \
	((GETREG8(REG_ISP_WRITEBACK_EXPO) << 24) | (GETREG8(REG_ISP_WRITEBACK_EXPO + 1) << 16) | \
	(GETREG8(REG_ISP_WRITEBACK_EXPO + 2) << 8) | (GETREG8(REG_ISP_WRITEBACK_EXPO + 3))))

#define get_writeback_gain()	 \
	(GETREG8(REG_ISP_AECAGC_WRITESENSOR_ENABLE) ? \
	((GETREG8(REG_ISP_RESERVED_GAIN) << 8) | (GETREG8(REG_ISP_RESERVED_GAIN + 1))) : \
	((GETREG8(REG_ISP_WRITEBACK_GAIN) << 8) | (GETREG8(REG_ISP_WRITEBACK_GAIN + 1))))

#define get_current_y() GETREG8(REG_ISP_CURRENT_Y)

#define get_win_lum(index) \
	((GETREG8(REG_ISP_WIN_LUM(index)) << 24) | (GETREG8(REG_ISP_WIN_LUM(index) + 1) << 16) | \
	(GETREG8(REG_ISP_WIN_LUM(index) + 2) << 8) | (GETREG8(REG_ISP_WIN_LUM(index) + 3)))

#define ISP_ZOOM_BASE_RATIO 0x100
#define ISP_ZOOM_MAX_RATIO 0x400
#define ISP_FOCUS_ZOOM_MAX_RATIO 0x200

#define ISP_RUNNING_CLOCK (160000000L)

#define ISP_AE_CTRL_MODE	(0x1d9a0) /* 1 - AP's ae . 0 - ISP ae. */
#define ISP_AE_WRITE_MODE	(0x1d9ad) /* 1 - ISP write sensor shutter and gain; 0 - AP write shutter and gain. */

#define AE_CTRL_MODE_AP	1
#define AE_CTRL_MODE_ISP	0

#define AE_WRITE_MODE_AP	0
#define AE_WRITE_MODE_ISP	1

/* Used for tune ops and AF functions to get isp_data handler */
void ispv1_tune_ops_init(k3_isp_data *ispdata);
void ispv1_tune_ops_exit(void);
void ispv1_tune_ops_prepare(camera_state state);
void ispv1_tune_ops_withdraw(camera_state state);

/* for tune ops */
int ispv1_set_anti_shaking_block(int blocksize);
int ispv1_set_anti_shaking(camera_anti_shaking flag);
int ispv1_get_anti_shaking_coordinate(coordinate_s *coordinate);

/*
 * set ISO: convert ISO to gain and set to sensor
 */
int ispv1_set_iso(camera_iso iso);

int ispv1_iso2gain(int iso, bool binning);

int ispv1_gain2iso(int gain, bool binning);

u32 get_writeback_cap_expo(void);
u32 get_writeback_cap_gain(void);

/* only useful for ISO auto mode */
int ispv1_get_actual_iso(void);
int ispv1_get_hdr_iso_exp(hdr_para_reserved *iso_exp);

#define ispv1_expo_time2line(expo_time, fps, vts) ((fps) * (vts) / (expo_time))
#define ispv1_expo_line2time(expo_line, fps, vts) ((fps) * (vts) / (expo_line))
int ispv1_get_exposure_time(void);

/* < zhoutian 00195335 2012-10-20 added for hwscope begin */
int ispv1_set_hwscope(hwscope_coff *hwscope_data);
/* zhoutian 00195335 2012-10-20 added for hwscope end > */
/* < zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight begin */
int ispv1_set_hw_lowlight(int ctl);
int ispv1_get_binning_size(binning_size *size);
/* zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight end > */

u32 ispv1_get_awb_gain(int withShift);
u32 ispv1_get_focus_code(void);
u32 ispv1_get_focus_rect(camera_rect_s *rect);

u32 ispv1_get_expo_line(void);
u32 ispv1_get_sensor_vts(void);

u32 ispv1_get_current_ccm_rgain(void);
u32 ispv1_get_current_ccm_bgain(void);

//hdr movie interface debug
int  ispv1_get_sensor_lux_matrix(lux_stat_matrix_tbl * lux_matrix);
int  ispv1_get_sensor_hdr_points(atr_ctrl_points* hdr_points);
int  ispv1_get_sensor_hdr_info(hdr_info* hdr_info);
int ispv1_get_fps(camera_sensor *sensor, camera_fps fps);
int ispv1_set_fps(camera_sensor *sensor, camera_fps fps, u8 value);

/*
 * set exposure value
 */
int ispv1_set_ev(int ev);

/*
 * set anti_banding
 */
int ispv1_set_anti_banding(camera_anti_banding banding);
int ispv1_get_anti_banding(void);

/*
 * related to sensor.
 */
int ispv1_set_awb(camera_white_balance awb_mode);

/* < zhoutian 00195335 12-7-16 added for auto scene detect begin */
int ispv1_get_extra_coff(extra_coff *extra_data);

int ispv1_get_ae_coff(ae_coff *ae_data);
int ispv1_set_ae_coff(ae_coff *ae_data);

int ispv1_get_awb_coff(awb_coff *awb_data);
int ispv1_set_awb_coff(awb_coff *awb_data);

int ispv1_get_awb_ct_coff(awb_ct_coff *awb_ct_data);
int ispv1_set_awb_ct_coff(awb_ct_coff *awb_ct_data);

int ispv1_get_ccm_coff(ccm_coff *ccm_data);
int ispv1_set_ccm_coff(ccm_coff *ccm_data);

int ispv1_set_added_coff(added_coff *added_data);

void ispv1_change_fps(camera_frame_rate_mode mode);

int ispv1_set_max_exposure(camera_max_exposrure mode);

int ispv1_get_coff_seq(seq_coffs *seq_data);
int ispv1_set_coff_seq(seq_coffs *seq_data);

int ispv1_set_max_expo_time(int time);
/* zhoutian 00195335 12-7-16 added for auto scene detect end > */
/*also called edge enhancement*/
int ispv1_set_sharpness(camera_sharpness sharpness);
int ispv1_set_saturation(camera_saturation saturation);
int ispv1_set_contrast(camera_contrast contrast);
int ispv1_set_scene(camera_scene scene);
int ispv1_set_brightness(camera_brightness brightness);

int ispv1_switch_brightness(camera_state state, camera_brightness brightness);
int ispv1_switch_contrast(camera_state state, camera_contrast contrast);

int ispv1_set_effect(camera_effects effect);

/* if 1 is on, 0 is off, default is off */
int ispv1_set_hue(int flag);

/*
 * related to sensor.
 */
int ispv1_init_LENC(u8 *lensc_param);
int ispv1_init_CCM(u16 *ccm_param);
int ispv1_init_AWB(u8 *awb_param);

/*
 * before start_preview or start_capture, it should be called to update size information
 */
int ispv1_update_LENC_scale(u32 inwidth, u32 inheight);

/*
 * binning correction
 */
int ispv1_init_BC(int binningMode, int mirror, int filp);

/*
 * defect_pixel_correction
 */
int ispv1_init_DPC(int bWhitePixel, int bBlackPixel);

/* Flag: if 1 is on, 0 is off, default is off */
int ispv1_init_rawDNS(int flag);
/* Flag: if 1 is on, 0 is off, default is off */
int ispv1_init_uvDNS(int flag);

/*
 * level: if >0 is on, 0 is off, default is 6
 * 16: keep full Gb/Gr difference as resolution;
 * 8: remove half Gb/Gr difference;
 * 0: remove all Gb/Gr difference;
 */
int ispv1_init_GbGrDNS(int level);

/* Flag: if 1 is on, 0 is off, default is off, default is on, 0x65004 bit[5] */
int ispv1_init_RGBGamma(int flag);
/*using for write ad register(set position) of vcm driver ic by i2c bus*/
int ispv1_write_vcm(u8 i2c_addr, u16 reg, u16 val, i2c_length length, int mutex_wait);

int ispv1_get_current_vts(camera_sensor *sensor);
int ispv1_get_current_fps(camera_sensor *sensor);
int ispv1_get_band_threshold(camera_sensor *sensor, camera_anti_banding banding);
void ispv1_set_fps_lock(int lock);
void ispv1_preview_done_do_tune(void);
void ispv1_cmd_id_do_ecgc(camera_state state);
void ispv1_set_aecagc_mode(aecagc_mode_t mode);
void ispv1_set_awb_mode(awb_mode_t mode);

void ispv1_save_aecawb_step(aecawb_step_t *step);
void ispv1_config_aecawb_step(bool flash_mode, aecawb_step_t *step);
void ispv1_check_flash_prepare(void);

void ispv1_get_wb_value(awb_gain_t *awb);
void ispv1_set_wb_value(awb_gain_t *awb);
bool ispv1_copy_preview_data(u8 *dest, camera_rect_s *rect);

void ispv1_write_isp_reg(u32 isp_addr, u8 reg_value);

int ispv1_get_frame_rate_level(void);
void ispv1_set_frame_rate_level(int level);

camera_frame_rate_state ispv1_get_frame_rate_state(void);
void ispv1_set_frame_rate_state(camera_frame_rate_state state);

bool ae_is_need_flash(camera_sensor *sensor, aec_data_t *ae_data, u32 target_y_low);

bool ispv1_is_hdr_movie_mode(void);

void ispv1_red_clip_correction(void);
int ispv1_uv_stat_init(void);
void ispv1_uv_stat_excute(void);
void ispv1_uv_stat_exit(void);

int ispv1_load_isp_setting(char *filename);
int ispv1_load_sensor_setting(camera_sensor *sensor,char *filename);
void ispv1_dynamic_ydenoise(camera_sensor *sensor, camera_state state, bool flash_on);


#endif /*__K3_ISPV1_H__ */
/********************************* END ***********************************************/
