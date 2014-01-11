/*
 *  sonyimx091 camera driver source file
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
#include "sonyimx091.h"
/*#include "../isp/k3_isp_io.h"*/
#include <asm/bug.h>
#include <linux/device.h>

#define LOG_TAG "SONYIMX091"
/* #define DEBUG_DEBUG 1 */
#include "../isp/cam_log.h"
#include <../isp/cam_util.h>

#define SONYIMX091_SLAVE_ADDRESS 0x34
#define SONYIMX091_CHIP_ID       (0x0800)

#define SONYIMX091_FLIP		0x0006

#define SONYIMX091_EXPOSURE_REG_H	0x0202
#define SONYIMX091_EXPOSURE_REG_L	0x0203
#define SONYIMX091_GAIN_REG_H		0x0202
#define SONYIMX091_GAIN_REG_L		0x0203

#define SONYIMX091_VTS_REG_H		0x0340
#define SONYIMX091_VTS_REG_L		0x0341

static camera_capability sonyimx091_cap[] = {
	{V4L2_CID_FLASH_MODE, THIS_FLASH},
	{V4L2_CID_FOCUS_MODE, THIS_FOCUS_MODE},
};

const struct isp_reg_t isp_init_regs_sonyimx091[] = {
	//{0x65006, 0x01},//y36721 delete, not needed now.

/* y36721 2012-03-28 added. */
/* y36721 2012-03-31 changed blc and lenc. */
//blc value=0x40(10bit)
{0x1c58b, 0xff},
{0x1c58c, 0xff},

#ifndef OVISP_DEBUG_MODE
/*OVISP LENC setting for DAY light Long Exposure (HDR/3D)*/
	{0x1c264, 0x28},
	{0x1c265, 0x1a},
	{0x1c266, 0x14},
	{0x1c267, 0x14},
	{0x1c268, 0x1b},
	{0x1c269, 0x2b},
	{0x1c26a, 0x11},
	{0x1c26b, 0x9},
	{0x1c26c, 0x6},
	{0x1c26d, 0x6},
	{0x1c26e, 0x9},
	{0x1c26f, 0x13},
	{0x1c270, 0xa},
	{0x1c271, 0x3},
	{0x1c272, 0x0},
	{0x1c273, 0x0},
	{0x1c274, 0x4},
	{0x1c275, 0xb},
	{0x1c276, 0xa},
	{0x1c277, 0x3},
	{0x1c278, 0x0},
	{0x1c279, 0x0},
	{0x1c27a, 0x4},
	{0x1c27b, 0xa},
	{0x1c27c, 0x13},
	{0x1c27d, 0x9},
	{0x1c27e, 0x6},
	{0x1c27f, 0x6},
	{0x1c280, 0xa},
	{0x1c281, 0x14},
	{0x1c282, 0x2b},
	{0x1c283, 0x1c},
	{0x1c284, 0x15},
	{0x1c285, 0x15},
	{0x1c286, 0x1d},
	{0x1c287, 0x2e},
	{0x1c288, 0x1c},
	{0x1c289, 0x1d},
	{0x1c28a, 0x1e},
	{0x1c28b, 0x1d},
	{0x1c28c, 0x1b},
	{0x1c28d, 0x1e},
	{0x1c28e, 0x1f},
	{0x1c28f, 0x1f},
	{0x1c290, 0x1f},
	{0x1c291, 0x1e},
	{0x1c292, 0x1e},
	{0x1c293, 0x1f},
	{0x1c294, 0x21},
	{0x1c295, 0x1f},
	{0x1c296, 0x1e},
	{0x1c297, 0x1e},
	{0x1c298, 0x1e},
	{0x1c299, 0x1f},
	{0x1c29a, 0x1f},
	{0x1c29b, 0x1e},
	{0x1c29c, 0x1c},
	{0x1c29d, 0x1d},
	{0x1c29e, 0x1d},
	{0x1c29f, 0x1d},
	{0x1c2a0, 0x1c},
	{0x1c2a1, 0x24},
	{0x1c2a2, 0x29},
	{0x1c2a3, 0x2b},
	{0x1c2a4, 0x29},
	{0x1c2a5, 0x25},
	{0x1c2a6, 0x28},
	{0x1c2a7, 0x27},
	{0x1c2a8, 0x25},
	{0x1c2a9, 0x27},
	{0x1c2aa, 0x29},
	{0x1c2ab, 0x29},
	{0x1c2ac, 0x21},
	{0x1c2ad, 0x1d},
	{0x1c2ae, 0x21},
	{0x1c2af, 0x2a},
	{0x1c2b0, 0x28},
	{0x1c2b1, 0x27},
	{0x1c2b2, 0x24},
	{0x1c2b3, 0x27},
	{0x1c2b4, 0x28},
	{0x1c2b5, 0x25},
	{0x1c2b6, 0x28},
	{0x1c2b7, 0x2b},
	{0x1c2b8, 0x29},
	{0x1c2b9, 0x25},

/*OVISP,LENC setting for A light Long Exposure (HDR/3D)*/
	{0x1c310, 0x35},
	{0x1c311, 0x21},
	{0x1c312, 0x19},
	{0x1c313, 0x19},
	{0x1c314, 0x23},
	{0x1c315, 0x36},
	{0x1c316, 0x16},
	{0x1c317, 0xb},
	{0x1c318, 0x8},
	{0x1c319, 0x8},
	{0x1c31a, 0xc},
	{0x1c31b, 0x18},
	{0x1c31c, 0xd},
	{0x1c31d, 0x4},
	{0x1c31e, 0x0},
	{0x1c31f, 0x0},
	{0x1c320, 0x5},
	{0x1c321, 0xd},
	{0x1c322, 0xd},
	{0x1c323, 0x5},
	{0x1c324, 0x0},
	{0x1c325, 0x0},
	{0x1c326, 0x5},
	{0x1c327, 0xe},
	{0x1c328, 0x17},
	{0x1c329, 0xc},
	{0x1c32a, 0x8},
	{0x1c32b, 0x8},
	{0x1c32c, 0xc},
	{0x1c32d, 0x18},
	{0x1c32e, 0x37},
	{0x1c32f, 0x22},
	{0x1c330, 0x1a},
	{0x1c331, 0x1a},
	{0x1c332, 0x24},
	{0x1c333, 0x37},
	{0x1c334, 0x1c},
	{0x1c335, 0x1d},
	{0x1c336, 0x1d},
	{0x1c337, 0x1d},
	{0x1c338, 0x1b},
	{0x1c339, 0x1e},
	{0x1c33a, 0x1f},
	{0x1c33b, 0x1f},
	{0x1c33c, 0x1f},
	{0x1c33d, 0x1e},
	{0x1c33e, 0x1e},
	{0x1c33f, 0x20},
	{0x1c340, 0x21},
	{0x1c341, 0x20},
	{0x1c342, 0x1e},
	{0x1c343, 0x1e},
	{0x1c344, 0x1f},
	{0x1c345, 0x1f},
	{0x1c346, 0x1f},
	{0x1c347, 0x1e},
	{0x1c348, 0x1c},
	{0x1c349, 0x1d},
	{0x1c34a, 0x1e},
	{0x1c34b, 0x1e},
	{0x1c34c, 0x1c},
	{0x1c34d, 0x28},
	{0x1c34e, 0x2d},
	{0x1c34f, 0x31},
	{0x1c350, 0x2d},
	{0x1c351, 0x29},
	{0x1c352, 0x2c},
	{0x1c353, 0x2a},
	{0x1c354, 0x26},
	{0x1c355, 0x2a},
	{0x1c356, 0x2c},
	{0x1c357, 0x2e},
	{0x1c358, 0x21},
	{0x1c359, 0x1c},
	{0x1c35a, 0x22},
	{0x1c35b, 0x2e},
	{0x1c35c, 0x2c},
	{0x1c35d, 0x29},
	{0x1c35e, 0x26},
	{0x1c35f, 0x2a},
	{0x1c360, 0x2c},
	{0x1c361, 0x29},
	{0x1c362, 0x2c},
	{0x1c363, 0x30},
	{0x1c364, 0x2d},
	{0x1c365, 0x29},

/*OVISP,LENC setting for CWF light Long Exposure (HDR/3D)*/
	{0x1c2ba, 0x26},
	{0x1c2bb, 0x19},
	{0x1c2bc, 0x13},
	{0x1c2bd, 0x14},
	{0x1c2be, 0x1a},
	{0x1c2bf, 0x26},
	{0x1c2c0, 0x11},
	{0x1c2c1, 0x9},
	{0x1c2c2, 0x6},
	{0x1c2c3, 0x6},
	{0x1c2c4, 0xa},
	{0x1c2c5, 0x12},
	{0x1c2c6, 0xa},
	{0x1c2c7, 0x4},
	{0x1c2c8, 0x0},
	{0x1c2c9, 0x0},
	{0x1c2ca, 0x4},
	{0x1c2cb, 0xb},
	{0x1c2cc, 0xa},
	{0x1c2cd, 0x4},
	{0x1c2ce, 0x0},
	{0x1c2cf, 0x0},
	{0x1c2d0, 0x4},
	{0x1c2d1, 0xb},
	{0x1c2d2, 0x13},
	{0x1c2d3, 0x9},
	{0x1c2d4, 0x7},
	{0x1c2d5, 0x7},
	{0x1c2d6, 0xa},
	{0x1c2d7, 0x13},
	{0x1c2d8, 0x29},
	{0x1c2d9, 0x1c},
	{0x1c2da, 0x14},
	{0x1c2db, 0x15},
	{0x1c2dc, 0x1c},
	{0x1c2dd, 0x29},
	{0x1c2de, 0x16},
	{0x1c2df, 0x1a},
	{0x1c2e0, 0x1b},
	{0x1c2e1, 0x1a},
	{0x1c2e2, 0x16},
	{0x1c2e3, 0x1c},
	{0x1c2e4, 0x1e},
	{0x1c2e5, 0x1e},
	{0x1c2e6, 0x1e},
	{0x1c2e7, 0x1c},
	{0x1c2e8, 0x1c},
	{0x1c2e9, 0x1f},
	{0x1c2ea, 0x21},
	{0x1c2eb, 0x1f},
	{0x1c2ec, 0x1c},
	{0x1c2ed, 0x1b},
	{0x1c2ee, 0x1e},
	{0x1c2ef, 0x1e},
	{0x1c2f0, 0x1e},
	{0x1c2f1, 0x1c},
	{0x1c2f2, 0x16},
	{0x1c2f3, 0x1a},
	{0x1c2f4, 0x1b},
	{0x1c2f5, 0x1a},
	{0x1c2f6, 0x16},
	{0x1c2f7, 0x1c},
	{0x1c2f8, 0x21},
	{0x1c2f9, 0x23},
	{0x1c2fa, 0x21},
	{0x1c2fb, 0x1d},
	{0x1c2fc, 0x22},
	{0x1c2fd, 0x23},
	{0x1c2fe, 0x22},
	{0x1c2ff, 0x23},
	{0x1c300, 0x22},
	{0x1c301, 0x23},
	{0x1c302, 0x20},
	{0x1c303, 0x1f},
	{0x1c304, 0x20},
	{0x1c305, 0x24},
	{0x1c306, 0x22},
	{0x1c307, 0x22},
	{0x1c308, 0x22},
	{0x1c309, 0x23},
	{0x1c30a, 0x22},
	{0x1c30b, 0x1d},
	{0x1c30c, 0x21},
	{0x1c30d, 0x23},
	{0x1c30e, 0x21},
	{0x1c30f, 0x1d},

/*OVISP CTAWB setting for Long Exposure (HDR/3D)*/
	{0x66206, 0x0c},
	{0x66207, 0x14},
	{0x66208, 0xd},
	{0x66209, 0x94},
	{0x6620a, 0x61},
	{0x6620b, 0xd5},
	{0x6620c, 0x9e},
	{0x6620d, 0x5b},
	{0x6620e, 0x46},
	{0x6620f, 0x81},
	{0x66210, 0x57},
	{0x66201, 0x52},
	{0x1c1cc, 0x1},
	{0x1c1cd, 0x0},
	{0x1c1c8, 0x0},
	{0x1c1c9, 0xd2},
	{0x1c1d0, 0x2},
	{0x1c1d1, 0x60},
	{0x1c254, 0x0},
	{0x1c255, 0xdc},
	{0x1c256, 0x0},
	{0x1c257, 0xee},
	{0x1c258, 0x1},
	{0x1c259, 0x89},
	{0x1c25a, 0x2},
	{0x1c25b, 0x12},
#endif
};

/*
 * should be calibrated, three lights, from 0x1c264
 * here is long exposure
 */
char sonyimx091_lensc_param[86 * 3] = {
};

/* should be calibrated, 6 groups 3x3, from 0x1c1d8 */
short sonyimx091_ccm_param[54] = {
};

char sonyimx091_awb_param[] = {
};

static framesize_s sonyimx091_framesizes[] = {
	//{0, 12, 1280, 720, 4620, 1616, 30, 25, 0, 0, 0x100, VIEW_FULL,RESOLUTION_16_9, {sonyimx091_framesize_1280x720, ARRAY_SIZE(sonyimx091_framesize_1280x720)} },
	//{0, 12, 1280, 960, 4620, 1616, 30, 25, 0, 0, 0x200, VIEW_FULL,RESOLUTION_4_3, {sonyimx091_framesize_1280_960, ARRAY_SIZE(sonyimx091_framesize_1280_960)} },
	//{0, 6, 1920, 1080, 4620, 1616, 30, 25, 0, 0, 0x200, VIEW_FULL,RESOLUTION_16_9, {sonyimx091_framesize_1080p_wide, ARRAY_SIZE(sonyimx091_framesize_1080p_wide)} },
	{0, 6, 2104, 1560, 4620, 1616, 30, 25, 0, 0, 0x200, VIEW_FULL, RESOLUTION_4_3, true, {sonyimx091_framesize_2104_1560, ARRAY_SIZE(sonyimx091_framesize_2104_1560)} },
	//{0, 14, 3280, 2464, 4620, 3232, 15, 12, 0, 0, 0x1F6, VIEW_FULL,RESOLUTION_4_3, false, {sonyimx091_framesize_3280_2464, ARRAY_SIZE(sonyimx091_framesize_3280_2464)} },
	{0, 14, 4208, 3120, 4620, 3232, 12, 10, 0, 0, 0x189, VIEW_FULL, RESOLUTION_4_3, false, {sonyimx091_framesize_full, ARRAY_SIZE(sonyimx091_framesize_full)} },
};

static camera_sensor sonyimx091_sensor;
static void sonyimx091_set_default(void);

/*
 **************************************************************************
 * FunctionName: sonyimx091_read_reg;
 * Description : read sonyimx091 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_read_reg(u16 reg, u8 *val)
{
	return k3_ispio_read_reg(sonyimx091_sensor.i2c_config.index,
				 sonyimx091_sensor.i2c_config.addr, reg, (u16*)val, sonyimx091_sensor.i2c_config.val_bits);
}

static int sonyimx091_read_reg_print_info(const struct _sensor_reg_t *seq, u32 size)
{
	int ret = 0;
	u16 reg;
	u16 val = 0;
	int i = 0;
	print_info("enter %s",__func__);
	for( i = 0; i< size;i++) {
		reg = seq[i].subaddr;
		ret = k3_ispio_read_reg(sonyimx091_sensor.i2c_config.index,
		sonyimx091_sensor.i2c_config.addr, reg, &val, sonyimx091_sensor.i2c_config.val_bits);
	    print_info("i2c_addr = 0x%x,reg = 0x%x,val = 0x%x", sonyimx091_sensor.i2c_config.addr,reg,val);
	}
	return ret;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_write_reg;
 * Description : write sonyimx091 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_write_reg(u16 reg, u8 val, u8 mask)
{
	return k3_ispio_write_reg(sonyimx091_sensor.i2c_config.index,
			sonyimx091_sensor.i2c_config.addr, reg, val, sonyimx091_sensor.i2c_config.val_bits, mask);
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_write_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int sonyimx091_write_seq(const struct _sensor_reg_t *seq, u32 size, u8 mask)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	return k3_ispio_write_seq(sonyimx091_sensor.i2c_config.index,
			sonyimx091_sensor.i2c_config.addr, seq, size, sonyimx091_sensor.i2c_config.val_bits, mask);
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_write_isp_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void sonyimx091_write_isp_seq(const struct isp_reg_t *seq, u32 size)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	k3_ispio_write_isp_seq(seq, size);
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_get_framecount
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_get_framecount(void)
{
	u8 icount = 0;
	print_info("enter %s",__func__);
	sonyimx091_read_reg(0x0005, &icount);
	print_info("sonyimx091 frame count:%d", icount);
	return icount;
}


/*
 **************************************************************************
 * FunctionName: sonyimx091_enum_frame_intervals;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_enum_frame_intervals(struct v4l2_frmivalenum *fi)
{
	assert(fi);

	print_debug("enter %s", __func__);
	if (fi->index >= CAMERA_MAX_FRAMERATE) {
		return -EINVAL;
	}

	fi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fi->discrete.numerator = 1;
	fi->discrete.denominator = (fi->index + 1);
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_get_format;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_get_format(struct v4l2_fmtdesc *fmt)
{
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_OVERLAY) {
		fmt->pixelformat = sonyimx091_sensor.fmt[STATE_PREVIEW];
	} else {
		fmt->pixelformat = sonyimx091_sensor.fmt[STATE_CAPTURE];
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_enum_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_enum_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	u32 max_index = ARRAY_SIZE(camera_framesizes) - 1;
	u32 this_max_index = ARRAY_SIZE(sonyimx091_framesizes) - 1;

	assert(framesizes);

	print_debug("enter %s; ", __func__);

	if (framesizes->index > max_index) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	if ((camera_framesizes[framesizes->index].width > sonyimx091_framesizes[this_max_index].width)
		|| (camera_framesizes[framesizes->index].height > sonyimx091_framesizes[this_max_index].height)) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	framesizes->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	framesizes->discrete.width = sonyimx091_framesizes[this_max_index].width;
	framesizes->discrete.height = sonyimx091_framesizes[this_max_index].height;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_try_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_try_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	int max_index = ARRAY_SIZE(sonyimx091_framesizes) - 1;

	assert(framesizes);

	print_info("Enter Function:%s  ", __func__);


	if ((framesizes->discrete.width <= sonyimx091_framesizes[max_index].width)
	    && (framesizes->discrete.height <= sonyimx091_framesizes[max_index].height)) {
		print_info("===========width = %d", framesizes->discrete.width);
		print_info("===========height = %d", framesizes->discrete.height);
		return 0;
	}

	print_error("frame size too large, [%d,%d]",
		    framesizes->discrete.width, framesizes->discrete.height);
	return -EINVAL;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_set_framesizes;
 * Description : NA;
 * Input       : flag: if 1, set framesize to sensor,
 *					   if 0, only store framesize to camera_interface;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_set_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs, int flag)
{
	int i = 0;

	assert(fs);

	print_info("Enter Function:%s State(%d), flag=%d, width=%d, height=%d",
		   __func__, state, flag, fs->width, fs->height);

	for (i = 0; i < ARRAY_SIZE(sonyimx091_framesizes); i++) {
		if ((sonyimx091_framesizes[i].width >= fs->width)
		    && (sonyimx091_framesizes[i].height >= fs->height)) {
			fs->width = sonyimx091_framesizes[i].width;
			fs->height = sonyimx091_framesizes[i].height;
			break;
		}
	}
	if (i >= ARRAY_SIZE(sonyimx091_framesizes)) {
		print_error("request resolution larger than sensor's max resolution");
		return -EINVAL;
	}

	if (state == STATE_PREVIEW) {
		sonyimx091_sensor.preview_frmsize_index = i;
	} else {
		sonyimx091_sensor.capture_frmsize_index = i;
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_get_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_get_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs)
{
	int frmsize_index;

	assert(fs);

	if (state == STATE_PREVIEW) {
		frmsize_index = sonyimx091_sensor.preview_frmsize_index;
	} else if (state == STATE_CAPTURE) {
		frmsize_index = sonyimx091_sensor.capture_frmsize_index;
	} else {
		return -EINVAL;
	}
	fs->width = sonyimx091_framesizes[frmsize_index].width;
	fs->height = sonyimx091_framesizes[frmsize_index].height;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_init_reg;
 * Description : download initial seq for sensor init;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_init_reg(void)
{
	int size = 0;

	print_debug("Enter Function:%s  , initsize=%d",
		    __func__, sizeof(sonyimx091_init_regs));
	size = ARRAY_SIZE(isp_init_regs_sonyimx091);
	sonyimx091_write_isp_seq(isp_init_regs_sonyimx091, size);

	if (0 != k3_ispio_init_csi(sonyimx091_sensor.mipi_index,
			sonyimx091_sensor.mipi_lane_count, sonyimx091_sensor.lane_clk)) {
		return -EFAULT;
	}

	size = ARRAY_SIZE(sonyimx091_init_regs);
	if (0 != sonyimx091_write_seq(sonyimx091_init_regs, size, 0x00)) {
		print_error("line %d, fail to init sonyimx091 sensor",__LINE__);
		return -EFAULT;
	}
       //sonyimx091_read_reg_print_info(sonyimx091_init_regs,size);
	return 0;
}

static int sonyimx091_get_capability(u32 id, u32 *value)
{
	int i;
	for (i = 0; i < sizeof(sonyimx091_cap) / sizeof(sonyimx091_cap[0]); ++i) {
		if (id == sonyimx091_cap[i].id) {
			*value = sonyimx091_cap[i].value;
			break;
		}
	}
	return 0;
}

static int sonyimx091_set_hflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);
	sonyimx091_sensor.hflip = flip;
	return 0;
}
static int sonyimx091_get_hflip(void)
{
	print_debug("enter %s", __func__);
	return sonyimx091_sensor.hflip;
}
static int sonyimx091_set_vflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);
	sonyimx091_sensor.vflip = flip;
	return 0;
}
static int sonyimx091_get_vflip(void)
{
	print_debug("enter %s", __func__);
	return sonyimx091_sensor.vflip;
}

static int sonyimx091_update_flip(u16 width, u16 height)
{
	u8 new_flip = ((sonyimx091_sensor.vflip << 1) | sonyimx091_sensor.hflip);
       print_debug("Enter %s  ", __func__);
	if(sonyimx091_sensor.old_flip != new_flip) {
		k3_ispio_update_flip((sonyimx091_sensor.old_flip ^ new_flip) & 0x03, width, height, PIXEL_ORDER_CHANGED);

		sonyimx091_sensor.old_flip = new_flip;
		sonyimx091_write_reg(SONYIMX091_FLIP, sonyimx091_sensor.vflip ? 0x02 : 0x00, ~0x02);
		sonyimx091_write_reg(SONYIMX091_FLIP, sonyimx091_sensor.hflip ? 0x01 : 0x00, ~0x01);
	}
	msleep(200);
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_framesize_switch;
 * Description : switch frame size, used by preview and capture
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_framesize_switch(camera_state state)
{
	u8 next_frmsize_index;

	if (state == STATE_PREVIEW)
		next_frmsize_index = sonyimx091_sensor.preview_frmsize_index;
	else
		next_frmsize_index = sonyimx091_sensor.capture_frmsize_index;

	print_debug("Enter Function:%s frm index=%d", __func__, next_frmsize_index);

	if (next_frmsize_index >= ARRAY_SIZE(sonyimx091_framesizes)){
		print_error("Unsupport sensor setting index: %d", next_frmsize_index);
		return -ETIME;
	}

	if (0 != sonyimx091_write_seq(sonyimx091_sensor.frmsize_list[next_frmsize_index].sensor_setting.setting,
		sonyimx091_sensor.frmsize_list[next_frmsize_index].sensor_setting.seq_size, 0x00)) {
		print_error("fail to init sonyimx091 sensor");
		return -ETIME;
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_stream_on;
 * Description : download preview seq for sensor preview;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_stream_on(camera_state state)
{
	print_debug("Enter Function:%s ", __func__);
	return sonyimx091_framesize_switch(state);
}

/*  **************************************************************************
* FunctionName: sonyimx091_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static int sonyimx091_check_sensor(void)
{
	u8 idl = 0;
	u8 idh = 0;
	u16 id = 0;

	sonyimx091_read_reg(0x300A, &idh);
	sonyimx091_read_reg(0x300B, &idl);

	id = ((idh << 8) | idl);
	print_info("sonyimx091 product id:0x%x", id);
	if (SONYIMX091_CHIP_ID != id) {
		print_error("Invalid product id ,Could not load sensor sonyimx091");
		return -ENODEV;
	}
	return 0;
}

/****************************************************************************
* FunctionName: sonyimx091_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
int sonyimx091_power(camera_power_state power)
{
	int ret = 0;
	if (power == POWER_ON) {
		if ((ret = gpio_get_value(GPIO_13_7)) > 0) {
			print_info("%s,use sonyimx091, ret :%d", __func__, ret);
		} else {
			print_info("%s,use sony or sumsung, ret :%d", __func__, ret);
		}

		k3_ispldo_power_sensor(power, "pri-cameralog-vcc");
		ret = camera_power_core_ldo(power);
		k3_ispldo_power_sensor(power, "camera-vcc");
		k3_ispldo_power_sensor(power, "cameravcm-vcc");
		k3_ispldo_power_sensor(power, "sec-cameralog-vcc");
		k3_ispgpio_power_sensor(&sonyimx091_sensor, power);
		k3_ispio_ioconfig(&sonyimx091_sensor, power);
	} else {
		k3_ispio_deinit_csi(sonyimx091_sensor.mipi_index);
		k3_ispio_ioconfig(&sonyimx091_sensor, power);
		k3_ispgpio_power_sensor(&sonyimx091_sensor, power);
		k3_ispldo_power_sensor(power, "sec-cameralog-vcc");
		k3_ispldo_power_sensor(power, "cameravcm-vcc");
		camera_power_core_ldo(power);
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
u32 sonyimx091_gain_to_iso(int gain)
{
	return (gain * 100) / 0x10;
}

u32 sonyimx091_iso_to_gain(int iso)
{
	return (iso * 0x10) / 100;
}

void sonyimx091_set_gain(u32 gain)
{
	if (gain == 0)
		return;
	gain = 256 - (256 * 16) / gain;
	//sonyimx091_write_reg(SONYIMX091_GAIN_REG_H, (gain >> 8) & 0xff, 0x00);
		print_debug("Enter %s   gain = %x", __func__,gain &0xff);
	sonyimx091_write_reg(SONYIMX091_GAIN_REG_L, gain & 0xff, 0x00);
}

void sonyimx091_set_exposure(u32 exposure)
{
	exposure >>= 4;
	print_debug("Enter %s  : exposure>> 8 = %x, exposure = %x", __func__,(exposure >> 8) & 0xff,exposure &0xff);
	sonyimx091_write_reg(SONYIMX091_EXPOSURE_REG_H, (exposure >> 8) & 0xff, 0x00);
	sonyimx091_write_reg(SONYIMX091_EXPOSURE_REG_L, exposure & 0xff, 0x00);
}

void sonyimx091_set_vts(u16 vts)
{
	print_debug("Enter %s  : vts>> 8 = %x, vts = %x", __func__,(vts >> 8) & 0xff,vts &0xff);
	sonyimx091_write_reg(SONYIMX091_VTS_REG_H, (vts >> 8) & 0xff, 0x00);
	sonyimx091_write_reg(SONYIMX091_VTS_REG_L, vts & 0xff, 0x00);
}

u32 sonyimx091_get_vts_reg_addr(void)
{
	return SONYIMX091_VTS_REG_H;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_reset;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_reset(camera_power_state power_state)
{
	print_debug("%s  ", __func__);

	if (POWER_ON == power_state) {
		k3_isp_io_enable_mclk(MCLK_ENABLE, sonyimx091_sensor.sensor_index);
		k3_ispgpio_reset_sensor(sonyimx091_sensor.sensor_index, power_state,
			      sonyimx091_sensor.power_conf.reset_valid);
	} else {
		k3_ispgpio_reset_sensor(sonyimx091_sensor.sensor_index, power_state,
			      sonyimx091_sensor.power_conf.reset_valid);
		k3_isp_io_enable_mclk(MCLK_DISABLE, sonyimx091_sensor.sensor_index);
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_init;
 * Description : sonyimx091 init function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : Error code indicating success or failure;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx091_init(void)
{
	print_debug("%s  ", __func__);

	if (!camera_timing_is_match(0)) {
		print_error("%s: sensor timing don't match.\n", __func__);
		return -ENODEV;
	}
	if (sonyimx091_sensor.owner && !try_module_get(sonyimx091_sensor.owner)) {
		print_error("%s: try_module_get fail", __func__);
		return -ENOENT;
	}

	k3_ispio_power_init("pri-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V */
	k3_ispio_power_init("camera-vcc", LDO_VOLTAGE_18V, LDO_VOLTAGE_18V);	/*IO 1.8V */
	k3_ispio_power_init("cameravcm-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*AF 2.85V */
	k3_ispio_power_init("sec-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V */

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_exit;
 * Description : sonyimx091 exit function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx091_exit(void)
{
	print_debug("enter %s", __func__);

	k3_ispio_power_deinit();

	if (sonyimx091_sensor.owner) {
		module_put(sonyimx091_sensor.owner);
	}
	print_debug("exit %s", __func__);
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_shut_down;
 * Description : sonyimx091 shut down function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx091_shut_down(void)
{
	print_debug("enter %s", __func__);
	k3_ispgpio_power_sensor(&sonyimx091_sensor, POWER_OFF);
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_set_default;
 * Description : init sonyimx091_sensor;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx091_set_default(void)
{
	unsigned int chip_id;
	int i;
	sonyimx091_sensor.init = sonyimx091_init;
	sonyimx091_sensor.exit = sonyimx091_exit;
	sonyimx091_sensor.shut_down = sonyimx091_shut_down;
	sonyimx091_sensor.reset = sonyimx091_reset;
	sonyimx091_sensor.check_sensor = sonyimx091_check_sensor;
	sonyimx091_sensor.power = sonyimx091_power;
	sonyimx091_sensor.init_reg = sonyimx091_init_reg;
	sonyimx091_sensor.stream_on = sonyimx091_stream_on;

	sonyimx091_sensor.get_format = sonyimx091_get_format;
	sonyimx091_sensor.set_flash = NULL;
	sonyimx091_sensor.get_flash = NULL;
	sonyimx091_sensor.set_scene = NULL;
	sonyimx091_sensor.get_scene = NULL;

	sonyimx091_sensor.enum_framesizes = sonyimx091_enum_framesizes;
	sonyimx091_sensor.try_framesizes = sonyimx091_try_framesizes;
	sonyimx091_sensor.set_framesizes = sonyimx091_set_framesizes;
	sonyimx091_sensor.get_framesizes = sonyimx091_get_framesizes;
	sonyimx091_sensor.enum_frame_intervals = sonyimx091_enum_frame_intervals;
	sonyimx091_sensor.try_frame_intervals = NULL;
	sonyimx091_sensor.set_frame_intervals = NULL;
	sonyimx091_sensor.get_frame_intervals = NULL;

	sonyimx091_sensor.get_capability = sonyimx091_get_capability;

	sonyimx091_sensor.set_hflip = sonyimx091_set_hflip;
	sonyimx091_sensor.get_hflip = sonyimx091_get_hflip;
	sonyimx091_sensor.set_vflip = sonyimx091_set_vflip;
	sonyimx091_sensor.get_vflip = sonyimx091_get_vflip;
	sonyimx091_sensor.update_flip = sonyimx091_update_flip;

	strncpy(sonyimx091_sensor.info.name, "sonyimx091", sizeof(sonyimx091_sensor.info.name));
	sonyimx091_sensor.interface_type = MIPI1;
	sonyimx091_sensor.mipi_lane_count = CSI_LINES_4;
	sonyimx091_sensor.mipi_index = CSI_INDEX_0;
	sonyimx091_sensor.sensor_index = CAMERA_SENSOR_PRIMARY;
	sonyimx091_sensor.skip_frames = 0;

	sonyimx091_sensor.power_conf.pd_valid = LOW_VALID;
	sonyimx091_sensor.power_conf.reset_valid = LOW_VALID;
	sonyimx091_sensor.power_conf.vcmpd_valid = LOW_VALID;

	sonyimx091_sensor.i2c_config.index = I2C_PRIMARY;
	sonyimx091_sensor.i2c_config.speed = I2C_SPEED_400;
	sonyimx091_sensor.i2c_config.addr = SONYIMX091_SLAVE_ADDRESS;
	sonyimx091_sensor.i2c_config.addr_bits = 16;
	sonyimx091_sensor.i2c_config.val_bits = I2C_8BIT;

	sonyimx091_sensor.preview_frmsize_index = 0;
	sonyimx091_sensor.capture_frmsize_index = 0;
	sonyimx091_sensor.frmsize_list = sonyimx091_framesizes;
	sonyimx091_sensor.fmt[STATE_PREVIEW] = V4L2_PIX_FMT_RAW10;
	sonyimx091_sensor.fmt[STATE_CAPTURE] = V4L2_PIX_FMT_RAW10;

	sonyimx091_sensor.aec_addr[0] = 0x0000;
	sonyimx091_sensor.aec_addr[1] = 0x0202;
	sonyimx091_sensor.aec_addr[2] = 0x0203;
	sonyimx091_sensor.agc_addr[0] = 0x0000;	/*0x0204 high byte not needed*/
	sonyimx091_sensor.agc_addr[1] = 0x0205;
	sonyimx091_sensor.sensor_type = SENSOR_SONY;
	sonyimx091_sensor.sensor_rgb_type = SENSOR_RGGB;/* changed by y00231328. add bayer order*/
	sonyimx091_sensor.set_gain = sonyimx091_set_gain;
	sonyimx091_sensor.set_exposure = sonyimx091_set_exposure;

	sonyimx091_sensor.set_vts = sonyimx091_set_vts;
	sonyimx091_sensor.get_vts_reg_addr = sonyimx091_get_vts_reg_addr;

	sonyimx091_sensor.sensor_gain_to_iso = sonyimx091_gain_to_iso;
	sonyimx091_sensor.sensor_iso_to_gain = sonyimx091_iso_to_gain;
	sonyimx091_sensor.set_effect = NULL;

	sonyimx091_sensor.isp_location = CAMERA_USE_K3ISP;
	sonyimx091_sensor.sensor_tune_ops = NULL;

	sonyimx091_sensor.get_sensor_aperture = NULL;
	sonyimx091_sensor.get_equivalent_focus = NULL;

	sonyimx091_sensor.af_enable = 1;
	sonyimx091_sensor.vcm = &vcm_ad5823;

	sonyimx091_sensor.image_setting.lensc_param = sonyimx091_lensc_param;
	sonyimx091_sensor.image_setting.ccm_param = sonyimx091_ccm_param;
	sonyimx091_sensor.image_setting.awb_param = sonyimx091_awb_param;

	sonyimx091_sensor.fps_max = 30;
	sonyimx091_sensor.fps_min = 12;
	sonyimx091_sensor.fps = 25;

	sonyimx091_sensor.owner = THIS_MODULE;

	sonyimx091_sensor.info.facing = CAMERA_FACING_BACK;
	sonyimx091_sensor.info.orientation = 270;
	sonyimx091_sensor.info.focal_length = 439;	/* 4.39mm */
	sonyimx091_sensor.info.h_view_angle = 66;	/*  66.1 degree */
	sonyimx091_sensor.info.v_view_angle = 50;
	sonyimx091_sensor.lane_clk = CLK_400M;
	sonyimx091_sensor.hflip = 0;
	sonyimx091_sensor.vflip = 0;
	sonyimx091_sensor.old_flip = 0;

	chip_id = get_chipid();

	if (chip_id == DI_CHIP_ID) {
		for(i = 0; i < (ARRAY_SIZE(sonyimx091_framesizes)); i++) {
			sonyimx091_framesizes[i].fps = sonyimx091_framesizes[i].fps_es;
		}

		/* full size, add by y36721 */
		i = ARRAY_SIZE(sonyimx091_framesizes) - 1;
		sonyimx091_framesizes[i].sensor_setting.setting = sonyimx091_framesize_full_es;
		sonyimx091_framesizes[i].sensor_setting.seq_size = ARRAY_SIZE(sonyimx091_framesize_full_es);
	}
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_module_init;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static __init int sonyimx091_module_init(void)
{
	sonyimx091_set_default();
	return register_camera_sensor(sonyimx091_sensor.sensor_index, &sonyimx091_sensor);
}

/*
 **************************************************************************
 * FunctionName: sonyimx091_module_exit;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void __exit sonyimx091_module_exit(void)
{
	unregister_camera_sensor(sonyimx091_sensor.sensor_index, &sonyimx091_sensor);
}

MODULE_AUTHOR("Hisilicon");
module_init(sonyimx091_module_init);
module_exit(sonyimx091_module_exit);
MODULE_LICENSE("GPL");

/********************************** END **********************************************/
