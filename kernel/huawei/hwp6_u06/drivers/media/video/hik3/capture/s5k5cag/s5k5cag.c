/*
 *  s5k5cag camera driver source file
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
#include <mach/gpio.h>
#include "../isp/sensor_common.h"
#include "s5k5cag.h"
/*#include "../isp/k3_isp_io.h"*/
#include <asm/bug.h>
#include <linux/device.h>

#define LOG_TAG "S5K5CAG"
#define DEBUG_DEBUG 1
#include "../isp/cam_log.h"
#include "../isp/cam_dbg.h"

#define S5K5CAG_SUNNY_SLAVE_ADDRESS     (0x5a)
#define S5K5CAG_CHIP_ID       (0x05ca)

#define S5K5CAG_CAM_MODULE_SKIPFRAME     4


#define S5K5CAG_NO_FLIP	0x00
#define S5K5CAG_H_FLIP	       0x01
#define S5K5CAG_V_FLIP	       0x02
#define S5K5CAG_HV_FLIP	0x03

#define S5K5CAG_EXPOSURE_REG	0x3012
#define S5K5CAG_GAIN_REG		0x305e


#define S5K5CAG_HFLIP		((1 << CAMERA_H_FLIP) |\
					(1 << CAMERA_V_FLIP))

#define S5K5CAG_VFLIP		(\
					(1 << CAMERA_NO_FLIP) | \
					(1 << CAMERA_H_FLIP) \
				)

#define S5K5CAG_ISO		(\
					(1 << CAMERA_ISO_AUTO) | \
					(1 << CAMERA_ISO_100) | \
					(1 << CAMERA_ISO_200) | \
					(1 << CAMERA_ISO_400)  \
				)

static camera_capability 	s5k5cag_cap[] = {
	{V4L2_CID_AUTO_WHITE_BALANCE, THIS_AUTO_WHITE_BALANCE},
	{V4L2_CID_DO_WHITE_BALANCE, THIS_WHITE_BALANCE},
	{V4L2_CID_EFFECT, THIS_EFFECT},
	{V4L2_CID_HFLIP, S5K5CAG_HFLIP},
	{V4L2_CID_VFLIP, S5K5CAG_VFLIP},
	{V4L2_CID_BRIGHTNESS, THIS_BRIGHTNESS},
	{V4L2_CID_CONTRAST, THIS_CONTRAST},
	{V4L2_CID_SATURATION, THIS_SATURATION},
	{V4L2_CID_ISO, S5K5CAG_ISO},
	{V4L2_CID_EXPOSURE_MAX, THIS_EXPOSURE},
	{V4L2_CID_EXPOSURE_STEP, THIS_EXPOSURE_STEP},
};

const struct isp_reg_t isp_init_regs_s5k5cag[] = {
	{0x63022,0x98},
	{0x63c12,0x04},
	//{0x63b34,0x03},
	//{0x6502f,0x21},
};

/* Fixme: 50/60Hz anti-banding params should be calibrated. */
static framesize_s s5k5cag_framesizes[] = {
#if 0
	{0, 120, 1280, 720, 1604, 1101, 30, 30, 0x100, VIEW_FULL, RESOLUTION_16_9, {s5k5cag_framesize_full, ARRAY_SIZE(s5k5cag_framesize_full)}},
	{0, 0, 1024, 768, 1604, 1101, 30, 30, 0x100, VIEW_FULL, RESOLUTION_4_3, {s5k5cag_framesize_full, ARRAY_SIZE(s5k5cag_framesize_full)}},
#else
{0, 0, 1024, 768, 1024, 768, 15, 15, 0 , 0, 0x100, VIEW_FULL, RESOLUTION_4_3, false, {s5k5cag_framesize_1024x768, ARRAY_SIZE(s5k5cag_framesize_1024x768)}},
{0, 0, 2048, 1536, 2048, 1536, 5, 5, 0 , 0, 0x100, VIEW_FULL, RESOLUTION_4_3, false, {s5k5cag_framesize_2048x1536, ARRAY_SIZE(s5k5cag_framesize_2048x1536)}},
#endif
};

/******************************************/

/******************************************/

static camera_sensor s5k5cag_sensor;
static bool sensor_inited = false;
static camera_effects sensor_effect = CAMERA_EFFECT_NONE;
static camera_white_balance sensor_awb_mode = CAMERA_WHITEBALANCE_AUTO;
static camera_iso sensor_iso = CAMERA_ISO_AUTO;
static int sensor_ev = 0;
static int sensor_brightness = CAMERA_BRIGHTNESS_H0;
static int sensor_saturation = CAMERA_SATURATION_H0;
static int sensor_contrast = CAMERA_CONTRAST_H0;
static void s5k5cag_set_default(void);
static void s5k5cag_change_frame_rate(camera_frame_rate_mode fps_mode);
static void s5k5cag_update_frame_rate(void);
static int s5k5cag_set_brightness(camera_brightness brightness);
static int s5k5cag_set_contrast(camera_contrast contrast);
static int s5k5cag_set_saturation(camera_saturation saturation);
/*
 **************************************************************************
 * FunctionName: s5k5cag_read_reg;
 * Description : read s5k5cag_byd reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_read_reg(u16 reg, u16 *val)
{
	return k3_ispio_read_reg(s5k5cag_sensor.i2c_config.index,
				 s5k5cag_sensor.i2c_config.addr, reg, val,s5k5cag_sensor.i2c_config.val_bits);
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_write_reg;
 * Description : write s5k5cag_byd reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_write_reg(u16 reg, u16 val, i2c_length length, u8 mask)
{
	return k3_ispio_write_reg(s5k5cag_sensor.i2c_config.index,
			s5k5cag_sensor.i2c_config.addr, reg, val, length, mask);
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_write_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int s5k5cag_write_seq(const struct _sensor_reg_t *seq, u32 size, u8 mask)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	return k3_ispio_write_seq(s5k5cag_sensor.i2c_config.index,
			s5k5cag_sensor.i2c_config.addr, seq, size,s5k5cag_sensor.i2c_config.val_bits, mask);
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_write_isp_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void s5k5cag_write_isp_seq(const struct isp_reg_t *seq, u32 size)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	k3_ispio_write_isp_seq(seq, size);
}


/*
 **************************************************************************
 * FunctionName: s5k5cag_enum_frame_intervals;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_enum_frame_intervals(struct v4l2_frmivalenum *fi)
{
	assert(fi);

	print_debug("enter %s", __func__);
	if (fi->index >= CAMERA_MAX_FRAMERATE) {
		return -EINVAL;
	}

	fi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fi->discrete.numerator = 1;
	fi->discrete.denominator = (fi->index+1);
	return 0;
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_get_format;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_get_format(struct v4l2_fmtdesc *fmt)
{
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_OVERLAY) {
		fmt->pixelformat = s5k5cag_sensor.fmt[STATE_PREVIEW];
	} else {
		fmt->pixelformat = s5k5cag_sensor.fmt[STATE_CAPTURE];
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_enum_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_enum_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	u32 max_index = ARRAY_SIZE(camera_framesizes) - 1;
	u32 this_max_index = ARRAY_SIZE(s5k5cag_framesizes) - 1;

	assert(framesizes);

	print_debug("enter %s; ", __func__);

	if (framesizes->index > max_index) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	if ((camera_framesizes[framesizes->index].width > s5k5cag_framesizes[this_max_index].width)
		|| (camera_framesizes[framesizes->index].height > s5k5cag_framesizes[this_max_index].height)) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	framesizes->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	framesizes->discrete.width = s5k5cag_framesizes[this_max_index].width;
	framesizes->discrete.height = s5k5cag_framesizes[this_max_index].height;
	return 0;
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_try_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_try_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	int max_index = ARRAY_SIZE(s5k5cag_framesizes) - 1;

	assert(framesizes);

	print_debug("Enter Function:%s  ", __func__);


	if ((framesizes->discrete.width <= s5k5cag_framesizes[max_index].width)
	    && (framesizes->discrete.height <= s5k5cag_framesizes[max_index].height)) {
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
 * FunctionName: s5k5cag_set_framesizes;
 * Description : NA;
 * Input       : flag: if 1, set framesize to sensor,
 *					   if 0, only store framesize to camera_interface;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_set_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs, int flag, camera_setting_view_type view_type)
{

	int i = 0;
	bool match = false;

	assert(fs);

	print_debug("Enter Function:%s State(%d), flag=%d, width=%d, height=%d",
		    __func__, state, flag, fs->width, fs->height);

	for (i = 0; i < ARRAY_SIZE(s5k5cag_framesizes); i++) {
		if ((s5k5cag_framesizes[i].width >= fs->width)
		    && (s5k5cag_framesizes[i].height >= fs->height)
		    && (s5k5cag_framesizes[i].width * fs->height
		    == s5k5cag_framesizes[i].height * fs->width)) {
			fs->width = s5k5cag_framesizes[i].width;
			fs->height = s5k5cag_framesizes[i].height;
			match = true;
			break;
		}
	}

	if (false == match) {
		for (i = 0; i < ARRAY_SIZE(s5k5cag_framesizes); i++) {
			if ((s5k5cag_framesizes[i].width >= fs->width)
			    && (s5k5cag_framesizes[i].height >= fs->height)) {
				fs->width = s5k5cag_framesizes[i].width;
				fs->height = s5k5cag_framesizes[i].height;
				break;
			}
		}
	}
	if (i >= ARRAY_SIZE(s5k5cag_framesizes)) {
		print_error("request resolution larger than sensor's max resolution");
		return -EINVAL;
	}

	if (state == STATE_PREVIEW) {
		s5k5cag_sensor.preview_frmsize_index = i;
	} else {
		s5k5cag_sensor.capture_frmsize_index = i;
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_get_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_get_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs)
{
	int frmsize_index;

	assert(fs);

	if (state == STATE_PREVIEW) {
		frmsize_index = s5k5cag_sensor.preview_frmsize_index;
	} else if (state == STATE_CAPTURE) {
		frmsize_index = s5k5cag_sensor.capture_frmsize_index;
	} else {
		return -EINVAL;
	}
	fs->width = s5k5cag_framesizes[frmsize_index].width;
	fs->height = s5k5cag_framesizes[frmsize_index].height;

	return 0;
}

static int s5k5cag_set_effect(camera_effects effect)
{
	print_debug("%s, effect:%d", __func__, effect);
	if (false == sensor_inited) {
		sensor_effect = effect;
		return;
	}

	switch (effect) {
	case CAMERA_EFFECT_NONE:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x021e, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0000, I2C_16BIT, 0x00);
		break;

	case CAMERA_EFFECT_MONO:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x021e, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0001, I2C_16BIT, 0x00);
		break;

	case CAMERA_EFFECT_SEPIA:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x021e, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0004, I2C_16BIT, 0x00);
		break;

	case CAMERA_EFFECT_NEGATIVE:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x021e, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0003, I2C_16BIT, 0x00);
		break;

	case CAMERA_EFFECT_SOLARIZE:
		break;

	default:
		print_error("%s, Unsupport effect:%d", __func__, effect);
	return;
	}

	sensor_effect = effect;
}


static int s5k5cag_set_awb(camera_white_balance awb_mode)
{
	print_debug("enter %s", __func__);
	if (false == sensor_inited) {
		sensor_awb_mode = awb_mode;
		return -1;
	}

	switch (awb_mode) {
	case CAMERA_WHITEBALANCE_AUTO:
		print_info("AUTO WB");
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0X04D2, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x067F, I2C_16BIT, 0x00);
		mdelay(50);
		break;

	case CAMERA_WHITEBALANCE_INCANDESCENT:
		print_info("INCANDESCENT");
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0x04D2, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0677, I2C_16BIT, 0x00); // AWB Off
		s5k5cag_write_reg(0x002A, 0x04A0, I2C_16BIT, 0x00); //#REG_SF_USER_Rgain
		s5k5cag_write_reg(0x0F12, 0x0380, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0400, I2C_16BIT, 0x00); //REG_SF_USER_Ggain
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x09C0, I2C_16BIT, 0x00); //#REG_SF_USER_Bgain
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		mdelay(50);
		break;

	case CAMERA_WHITEBALANCE_FLUORESCENT:
		print_info("FLUORESCENT");
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0x04D2, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0677, I2C_16BIT, 0x00); // AWB Off
		s5k5cag_write_reg(0x002A, 0x04A0, I2C_16BIT, 0x00); //#REG_SF_USER_Rgain
		s5k5cag_write_reg(0x0F12, 0x0400, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0400, I2C_16BIT, 0x00); //REG_SF_USER_Ggain
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x083C, I2C_16BIT, 0x00); //#REG_SF_USER_Bgain
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		mdelay(50);
		break;

	case CAMERA_WHITEBALANCE_DAYLIGHT:
		print_info("DAYLIGHT");
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0x04D2, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0677, I2C_16BIT, 0x00); // AWB Off
		s5k5cag_write_reg(0x002A, 0x04A0, I2C_16BIT, 0x00); //#REG_SF_USER_Rgain
		s5k5cag_write_reg(0x0F12, 0x05A0, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0400, I2C_16BIT, 0x00); //REG_SF_USER_Ggain
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x05F0, I2C_16BIT, 0x00); // #REG_SF_USER_Bgain
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		mdelay(50);
		break;

	case CAMERA_WHITEBALANCE_CLOUDY_DAYLIGHT:
		print_info("CLOUDY_DAYLIGHT");
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0x04D2, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0677, I2C_16BIT, 0x00); // AWB Off
		s5k5cag_write_reg(0x002A, 0x04A0, I2C_16BIT, 0x00); //#REG_SF_USER_Rgain
		s5k5cag_write_reg(0x0F12, 0x0540, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00); //#REG_SF_USER_Rgainchanged
		s5k5cag_write_reg(0x0F12, 0x0400, I2C_16BIT, 0x00); //REG_SF_USER_Ggain
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00); //REG_SF_USER_Ggainchanged
		s5k5cag_write_reg(0x0F12, 0x0500, I2C_16BIT, 0x00); // #REG_SF_USER_Bgain
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00); // #REG_SF_USER_Bgainchanged
		mdelay(50);
		break;

	default:
		print_error("%s, Unsupport awb_mode:%d", __func__, awb_mode);
		return -1;
	}
	sensor_awb_mode = awb_mode;
	return 0;
}

static int s5k5cag_set_iso(camera_iso iso)
{
	print_info("enter %s", __func__);
	if (false == sensor_inited) {
		sensor_awb_mode = iso;
		return -1;
	}

	switch (iso) {
	case CAMERA_ISO_AUTO:
		print_info("ISO AUTO");
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0X04B4, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0X0000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		break;

	case CAMERA_ISO_100:
		print_info("ISO 100");
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0X04B4, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0X0064, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		break;

	case CAMERA_ISO_200:
		print_info("ISO 200");
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0X04B4, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0X00C8, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		break;

	case CAMERA_ISO_400:
		print_info("ISO 300");
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0X04B4, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0X012C, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0F12, 0x0001, I2C_16BIT, 0x00);
		break;

	default:
		print_error("%s, Unsupport ISO value:%d", __func__, iso);
		return -1;
	}
	print_info("exit %s", __func__);
	sensor_iso = iso;
	return 0;
}


static int s5k5cag_set_ev(int ev)
{
	print_info("enter %s", __func__);
	if (false == sensor_inited) {
		sensor_ev = ev;
		return -1;
	}

	switch(ev) {

	case -2:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0x0F70, I2C_16BIT, 0x00);	//#TVAR_ae_BrAve //ae target
		s5k5cag_write_reg(0x0F12, 0x003D - (2 * 12), I2C_16BIT, 0x00);
		mdelay(50);
		break;

	case -1:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0x0F70, I2C_16BIT, 0x00);	//#TVAR_ae_BrAve //ae target
		s5k5cag_write_reg(0x0F12, 0x003D - 12, I2C_16BIT, 0x00);
		mdelay(50);
		break;

	case 0:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0x0F70, I2C_16BIT, 0x00);	//#TVAR_ae_BrAve //ae target
		s5k5cag_write_reg(0x0F12, 0x003D, I2C_16BIT, 0x00);
		mdelay(50);
		break;

	case 1:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0x0F70, I2C_16BIT, 0x00);	//#TVAR_ae_BrAve //ae target
		s5k5cag_write_reg(0x0F12, 0x003D + 12, I2C_16BIT, 0x00);
		mdelay(50);
		break;

	case 2:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002A, 0x0F70, I2C_16BIT, 0x00);	//#TVAR_ae_BrAve //ae target
		s5k5cag_write_reg(0x0F12, 0x003D + (2 * 12), I2C_16BIT, 0x00);
		mdelay(50);
		break;

	default:
		print_error("%s, Unsupport ev:%d", __func__, ev);
		return -1;
	}
	sensor_ev = ev;
	return 0;
}
/*
 **************************************************************************
 * FunctionName: s5k5cag_init_reg;
 * Description : download initial seq for sensor init;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_init_reg(void)
{
	int size = 0;
	int i = 0;

	u16 id = 0;

	//struct sensor_reg const * reg_init;

	print_info("Enter Function:%s  Line:%d, initsize=%d",
		__func__, __LINE__, sizeof(s5k5cag_sunny_init_regs));
	msleep(10);
/////////////////////////////////////////////////////////////////////

	/* Read the Model ID of the sensor */
	s5k5cag_write_reg(0x002c, 0x0000, I2C_16BIT, 0x00);
	s5k5cag_write_reg(0x002e, 0x0040, I2C_16BIT, 0x00);
	s5k5cag_read_reg(0x0f12, &id);
	print_info("=========>s5k5cag product id:0x%x", id);

	size = sizeof(s5k5cag_sunny_init_regs)/sizeof(s5k5cag_sunny_init_regs[0]);

	for(i = 0; i < size; i++) {
		if (0 == s5k5cag_sunny_init_regs[i].subaddr) {
			msleep(s5k5cag_sunny_init_regs[i].value);
		} else {
			switch (s5k5cag_sunny_init_regs[i].size) {
			case 1:
				s5k5cag_write_reg(s5k5cag_sunny_init_regs[i].subaddr, s5k5cag_sunny_init_regs[i].value & 0xff, I2C_8BIT, 0x00);
				break;
			case 2:
				s5k5cag_write_reg(s5k5cag_sunny_init_regs[i].subaddr, s5k5cag_sunny_init_regs[i].value & 0xffff, I2C_16BIT, 0x00);
				break;
			case 4:
				s5k5cag_write_reg(s5k5cag_sunny_init_regs[i].subaddr, (s5k5cag_sunny_init_regs[i].value >> 16) & 0xffff, I2C_16BIT, 0x00);
				s5k5cag_write_reg(s5k5cag_sunny_init_regs[i].subaddr + 2, s5k5cag_sunny_init_regs[i].value & 0xffff, I2C_16BIT, 0x00);
				break;
			default:
				print_error("%s, Unsupport reg size:%d", __func__, s5k5cag_sunny_init_regs[i].size);
				return -EFAULT;
			}
		}
	}

	if (0 != k3_ispio_init_csi(s5k5cag_sensor.mipi_index,
			s5k5cag_sensor.mipi_lane_count, s5k5cag_sensor.lane_clk)) {
		print_error("fail to init csi");
		return -EFAULT;
	}

	msleep(100);

	sensor_inited = true;

	if (CAMERA_EFFECT_NONE != sensor_effect)
		s5k5cag_set_effect(sensor_effect);
	s5k5cag_set_awb(sensor_awb_mode);
	s5k5cag_set_iso(sensor_iso);
	s5k5cag_set_ev(sensor_ev);
	s5k5cag_set_brightness(sensor_brightness);
	s5k5cag_set_contrast(sensor_contrast);
	s5k5cag_set_saturation(sensor_saturation);
	return 0;
}

static int s5k5cag_get_capability(u32 id, u32 *value)
{
	int i;
	for (i = 0; i < sizeof(s5k5cag_cap) / sizeof(s5k5cag_cap[0]); ++i) {
		if (id == s5k5cag_cap[i].id) {
			*value = s5k5cag_cap[i].value;
			break;
		}
	}
	return 0;
}

static int s5k5cag_update_flip(u16 width, u16 height)
{
	u16 reg = 0;
	u8 new_flip = ((s5k5cag_sensor.vflip << 1) | s5k5cag_sensor.hflip);
	print_debug("enter %s", __func__);

	if(s5k5cag_sensor.old_flip != new_flip) {

		if(s5k5cag_sensor.hflip)
			reg |= 0x01;

		if(s5k5cag_sensor.vflip)
			reg |= 0x02;
		s5k5cag_sensor.old_flip = new_flip;

		s5k5cag_write_reg(S5K5CAGREG_FLIP, reg, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x098e, 0xdc00, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0xdc00, 0x28, I2C_8BIT, 0x00);
		s5k5cag_write_reg(0x0080, 0x8002, I2C_16BIT, 0x00);
		msleep(250);
	}

	return 0;
}

static u32 s5k5cag_get_gain(void)
{
	u16 gain_reg_val = 0;
	u32 digital_gain, analog_gain;
	s5k5cag_read_reg(S5K5CAG_GAIN_REG, &gain_reg_val);
	digital_gain = (((gain_reg_val >> 15) & 0x01) * 8)
		+ (((gain_reg_val >> 14) & 0x01) * 4)
		+ (((gain_reg_val >> 13) & 0x01) * 2)
		+ ((gain_reg_val >> 12) & 0x01);
	analog_gain = (1 << ((gain_reg_val >> 6) & 0x03))
		* (gain_reg_val & 0x3f);
	return digital_gain * analog_gain;
}

static u32 s5k5cag_get_exposure(void)
{
	u16 exposure = 0;
	s5k5cag_read_reg(S5K5CAG_EXPOSURE_REG, &exposure);
	return (exposure * 33);
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_framesize_switch;
 * Description : switch frame size, used by preview and capture
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_framesize_switch(camera_state state)
{
	print_info("the capture index is %d ", s5k5cag_sensor.capture_frmsize_index);
	u32 size = 0;
	u8 next_frmsize_index;
	int i =0;

	if (state == STATE_PREVIEW)
		next_frmsize_index = s5k5cag_sensor.preview_frmsize_index;
	else
		next_frmsize_index = s5k5cag_sensor.capture_frmsize_index;


	size = s5k5cag_framesizes[next_frmsize_index].sensor_setting.seq_size;
	for(i = 0; i < size; i++) {
		if (0 == s5k5cag_framesizes[next_frmsize_index].sensor_setting.setting[i].subaddr) {
			msleep(s5k5cag_framesizes[next_frmsize_index].sensor_setting.setting[i].value);
		} else {
			switch (s5k5cag_framesizes[next_frmsize_index].sensor_setting.setting[i].size) {
			case 1:
				s5k5cag_write_reg(s5k5cag_framesizes[next_frmsize_index].sensor_setting.setting[i].subaddr, s5k5cag_framesizes[next_frmsize_index].sensor_setting.setting[i].value & 0xff, I2C_8BIT, 0x00);
				break;
			case 2:
				s5k5cag_write_reg(s5k5cag_framesizes[next_frmsize_index].sensor_setting.setting[i].subaddr, s5k5cag_framesizes[next_frmsize_index].sensor_setting.setting[i].value & 0xffff, I2C_16BIT, 0x00);
				break;
			case 4:
				s5k5cag_write_reg(s5k5cag_framesizes[next_frmsize_index].sensor_setting.setting[i].subaddr, (s5k5cag_framesizes[next_frmsize_index].sensor_setting.setting[i].value >> 16) & 0xffff, I2C_16BIT, 0x00);
				s5k5cag_write_reg(s5k5cag_framesizes[next_frmsize_index].sensor_setting.setting[i].subaddr + 2, s5k5cag_framesizes[next_frmsize_index].sensor_setting.setting[i].value & 0xffff, I2C_16BIT, 0x00);
				break;
			}
		}
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_stream_on;
 * Description : download preview seq for sensor preview;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_stream_on(camera_state state)
{
	print_info("-------------------->Enter Function:%s ", __func__);
	return s5k5cag_framesize_switch(state);
}


/***************************************************************************
* FunctionName: s5k5cag_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static int s5k5cag_check_sensor(void)
{
	u16 id = 0;

	/* Read the Model ID of the sensor */
	s5k5cag_write_reg(0x002c, 0x0000, I2C_16BIT, 0x00);
	s5k5cag_write_reg(0x002e, 0x0040, I2C_16BIT, 0x00);

	s5k5cag_read_reg(0x0f12, &id);

	print_info("============>s5k5cag product id:0x%x", id);

	if (id != S5K5CAG_CHIP_ID) {
		print_error("Invalid product id ,Could not load sensor s5k5cag");
		return -EFAULT;
	}

	return 0;
}

/****************************************************************************
* FunctionName: s5k5cag_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
int s5k5cag_power(camera_power_state power)
{
	int ret = 0;

	if (power == POWER_ON) {
		k3_ispldo_power_sensor(power,"pri-cameralog-vcc");  // 2.8V 模拟电压
		k3_ispldo_power_sensor(power,"camera-vcc");         // 1.8V 数字电压
		k3_ispio_ioconfig(&s5k5cag_sensor, power);
		ret = camera_power_core_ldo(power);                 // 1.2V核电压
		//k3_ispldo_power_sensor(power,"cameravcm-vcc");      //马达电压
		k3_ispldo_power_sensor(power,"sec-cameralog-vcc");  //前置模拟电压 2.8v
	} else {
		k3_ispio_deinit_csi(s5k5cag_sensor.mipi_index);
		k3_ispldo_power_sensor(power,"sec-cameralog-vcc");
		//k3_ispldo_power_sensor(power,"cameravcm-vcc");      //马达电压
		camera_power_core_ldo(power);
		udelay(200);
		k3_ispio_ioconfig(&s5k5cag_sensor, power);
		k3_ispldo_power_sensor(power,"camera-vcc");
		udelay(10);
		k3_ispldo_power_sensor(power,"pri-cameralog-vcc");
		sensor_inited = false;
	}

	return ret;

}
/*
 * Here gain is in unit 1/16 of sensor gain,
 * y36721 todo, temporarily if sensor gain=0x10, ISO is 100
 * in fact we need calibrate an ISO-ET-gain table.
 */
u32 s5k5cag_gain_to_iso(int gain)
{
	return (gain * 100) / 0x10;
}

u32 s5k5cag_iso_to_gain(int iso)
{
	return (iso * 0x10) / 100;
}

void s5k5cag_set_gain(u32 gain)
{
}

void s5k5cag_set_exposure(u32 exposure)
{
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_reset;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_reset( camera_power_state power_state)
{
	print_debug("%s  ", __func__);

	if (POWER_ON == power_state) {
		k3_isp_io_enable_mclk(MCLK_ENABLE, s5k5cag_sensor.sensor_index);
		k3_ispgpio_reset_sensor(s5k5cag_sensor.sensor_index, power_state,
				s5k5cag_sensor.power_conf.reset_valid);

		k3_ispgpio_power_sensor(&s5k5cag_sensor, POWER_ON);
		udelay(500);
		k3_ispgpio_power_sensor(&s5k5cag_sensor, POWER_OFF);
	} else {
		k3_ispgpio_reset_sensor(s5k5cag_sensor.sensor_index, power_state,
				s5k5cag_sensor.power_conf.reset_valid);
		k3_isp_io_enable_mclk(MCLK_DISABLE, s5k5cag_sensor.sensor_index);
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_init;
 * Description : s5k5cag init function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : Error code indicating success or failure;
 * Other       : NA;
 **************************************************************************
*/
static int s5k5cag_init(void)
{
	print_debug("%s  ", __func__);
	if (s5k5cag_sensor.owner && !try_module_get(s5k5cag_sensor.owner)) {
		print_error("%s: try_module_get fail", __func__);
		return -ENOENT;
	}

	k3_ispio_power_init("pri-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V*/
	k3_ispio_power_init("camera-vcc", LDO_VOLTAGE_18V, LDO_VOLTAGE_18V);	/*IO 1.8V*/
	k3_ispio_power_init("sec-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V*/

	return 0;
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_exit;
 * Description : s5k5cag exit function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void s5k5cag_exit(void)
{
	print_debug("enter %s", __func__);

	k3_ispio_power_deinit();

	if (s5k5cag_sensor.owner) {
		module_put(s5k5cag_sensor.owner);
	}
	print_debug("exit %s", __func__);
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_shut_down;
 * Description : s5k5cag shut down function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void s5k5cag_shut_down(void)
{
	print_info("--------------------------->enter %s", __func__);
	k3_ispgpio_reset_sensor(CAMERA_SENSOR_PRIMARY , POWER_OFF, s5k5cag_sensor.power_conf.reset_valid);
}

static int s5k5cag_set_hflip(int flip)
{
	print_info("enter %s flip=%d", __func__, flip);
	s5k5cag_sensor.hflip = flip;
	return 0;
}

static int s5k5cag_get_hflip(void)
{
	print_info("enter %s", __func__);
	return s5k5cag_sensor.hflip;
}

static int s5k5cag_set_vflip(int flip)
{
	print_info("enter %s flip=%d", __func__, flip);

	s5k5cag_sensor.vflip = flip;
	return 0;
}

static int s5k5cag_get_vflip(void)
{
	print_info("enter %s", __func__);
	return s5k5cag_sensor.vflip;
}

static int s5k5cag_set_saturation(camera_saturation saturation)
{
	print_debug("enter %s", __func__);

	if (false == sensor_inited) {
		sensor_saturation = saturation;
		return -1;
	}
	switch (saturation) {
	case CAMERA_SATURATION_L2:
		//CAM_saturation_NEG_5_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x0210, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0xFF9C, I2C_16BIT, 0x00);
		break;

	case CAMERA_SATURATION_L1:
		//CAM_saturation_NEG_2_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x0210, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0xFFD8, I2C_16BIT, 0x00);
		break;

	case CAMERA_SATURATION_H0:
		//CAM_saturation_ZERO:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x0210, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0000, I2C_16BIT, 0x00);
		break;

	case CAMERA_SATURATION_H1:
		//CAM_saturation_POS_2_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x0210, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0028, I2C_16BIT, 0x00);
		break;

	case CAMERA_SATURATION_H2:
		//CAM_saturation_POS_5_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x0210, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0064, I2C_16BIT, 0x00);
		break;
	default:
		print_error("%s, Unsupport saturation: %d", __func__, saturation);
		return -1;
	}
	sensor_saturation = saturation;
	return 0;
}

static int s5k5cag_set_contrast(camera_contrast contrast)
{
	print_debug("enter %s", __func__);

	if (false == sensor_inited) {
		sensor_contrast = contrast;
		return -1;
	}
	switch (contrast) {
	case CAMERA_CONTRAST_L2:
		//CAM_contrast_NEG_5_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x020E, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0xFF9C, I2C_16BIT, 0x00);
		break;

	case CAMERA_CONTRAST_L1:
		//CAM_contrast_NEG_2_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x020E, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0xFFD8, I2C_16BIT, 0x00);
		break;

	case CAMERA_CONTRAST_H0:
		//CAM_contrast_ZERO:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x020E, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0000, I2C_16BIT, 0x00);
		break;

	case CAMERA_CONTRAST_H1:
		//CAM_contrast_POS_2_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x020E, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0028, I2C_16BIT, 0x00);
		break;

	case CAMERA_CONTRAST_H2:
		//CAM_contrast_POS_5_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x020E, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0064, I2C_16BIT, 0x00);
		break;
	default:
		print_error("%s, Unsupport contrast:%d", __func__, contrast);
		return -1;
	}
	sensor_contrast = contrast;
	return 0;
}
static int s5k5cag_set_brightness(camera_brightness brightness)
{
	print_debug("enter %s", __func__);

	if (false == sensor_inited) {
		sensor_brightness = brightness;
		return -1;
	}
	switch (brightness) {
	case CAMERA_BRIGHTNESS_L2:
		//CAM_sharpness_NEG_5_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x0212, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0xFF9C, I2C_16BIT, 0x00);
		break;

	case CAMERA_BRIGHTNESS_L1:
		//CAM_sharpness_NEG_2_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x0212, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0xFFD8, I2C_16BIT, 0x00);
		break;

	case CAMERA_BRIGHTNESS_H0:
		//CAM_sharpness_ZERO:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x0212, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0000, I2C_16BIT, 0x00);
		break;

	case CAMERA_BRIGHTNESS_H1:
		//CAM_sharpness_POS_2_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x0212, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0028, I2C_16BIT, 0x00);
		break;

	case CAMERA_BRIGHTNESS_H2:
		//CAM_sharpness_POS_5_3:
		s5k5cag_write_reg(0x0028, 0x7000, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x002a, 0x0212, I2C_16BIT, 0x00);
		s5k5cag_write_reg(0x0f12, 0x0064, I2C_16BIT, 0x00);
		break;
	default:
		print_error("%s, Unsupport brightness:%d", __func__, brightness);
		return -1;
	}
	sensor_brightness = brightness;
	return 0;
}


/*
 **************************************************************************
 * FunctionName: s5k5cag_set_default;
 * Description : init s5k5cag_sensor;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void s5k5cag_set_default(void)
{
	s5k5cag_sensor.init = s5k5cag_init;
	s5k5cag_sensor.exit = s5k5cag_exit;
	s5k5cag_sensor.shut_down = s5k5cag_shut_down;
	s5k5cag_sensor.reset = s5k5cag_reset;
	s5k5cag_sensor.check_sensor = s5k5cag_check_sensor;
	s5k5cag_sensor.power = s5k5cag_power;
	s5k5cag_sensor.init_reg = s5k5cag_init_reg;
	s5k5cag_sensor.stream_on = s5k5cag_stream_on;
	s5k5cag_sensor.sensor_type = SENSOR_SAMSUNG;

	s5k5cag_sensor.get_format = s5k5cag_get_format;
	s5k5cag_sensor.set_flash = NULL;
	s5k5cag_sensor.get_flash = NULL;
	s5k5cag_sensor.set_scene = NULL;
	s5k5cag_sensor.get_scene = NULL;

	s5k5cag_sensor.enum_framesizes = s5k5cag_enum_framesizes;
	s5k5cag_sensor.try_framesizes = s5k5cag_try_framesizes;
	s5k5cag_sensor.set_framesizes = s5k5cag_set_framesizes;
	s5k5cag_sensor.get_framesizes = s5k5cag_get_framesizes;

	s5k5cag_sensor.set_hflip = s5k5cag_set_hflip;
	s5k5cag_sensor.get_hflip = s5k5cag_get_hflip;
	s5k5cag_sensor.set_vflip = s5k5cag_set_vflip;
	s5k5cag_sensor.get_vflip = s5k5cag_get_vflip;
	s5k5cag_sensor.update_flip = s5k5cag_update_flip;

	s5k5cag_sensor.get_gain = s5k5cag_get_gain;
	s5k5cag_sensor.get_exposure = s5k5cag_get_exposure;

	s5k5cag_sensor.enum_frame_intervals = s5k5cag_enum_frame_intervals;
	s5k5cag_sensor.try_frame_intervals = NULL;
	s5k5cag_sensor.set_frame_intervals = NULL;
	s5k5cag_sensor.get_frame_intervals = NULL;

	s5k5cag_sensor.get_capability = s5k5cag_get_capability;

	strncpy(s5k5cag_sensor.info.name, "s5k5cag", sizeof(s5k5cag_sensor.info.name));
	s5k5cag_sensor.interface_type = MIPI1;
	s5k5cag_sensor.mipi_lane_count = CSI_LINES_1;//CSI_LINES_2;   zhangjun
	s5k5cag_sensor.mipi_index = CSI_INDEX_0;
	s5k5cag_sensor.sensor_index = CAMERA_SENSOR_PRIMARY;  //zhangjun
	s5k5cag_sensor.skip_frames = 0;

	s5k5cag_sensor.power_conf.pd_valid = LOW_VALID; //zhangjun
	s5k5cag_sensor.power_conf.reset_valid = LOW_VALID;
	s5k5cag_sensor.power_conf.vcmpd_valid = LOW_VALID;

	s5k5cag_sensor.i2c_config.index = I2C_PRIMARY;
	s5k5cag_sensor.i2c_config.speed = I2C_SPEED_400;
	s5k5cag_sensor.i2c_config.addr = S5K5CAG_SUNNY_SLAVE_ADDRESS;
	s5k5cag_sensor.i2c_config.addr_bits = 16;
	s5k5cag_sensor.i2c_config.val_bits = I2C_16BIT;

	s5k5cag_sensor.preview_frmsize_index = 0;
	s5k5cag_sensor.capture_frmsize_index = 1;
	s5k5cag_sensor.frmsize_list = s5k5cag_framesizes;
	s5k5cag_sensor.fmt[STATE_PREVIEW] = V4L2_PIX_FMT_YUYV; //
	s5k5cag_sensor.fmt[STATE_CAPTURE] = V4L2_PIX_FMT_YUYV; //

	s5k5cag_sensor.aec_addr[0] = 0;
	s5k5cag_sensor.aec_addr[1] = 0;
	s5k5cag_sensor.aec_addr[2] = 0;
	s5k5cag_sensor.agc_addr[0] = 0;
	s5k5cag_sensor.agc_addr[1] = 0;

	s5k5cag_sensor.set_gain = NULL;
	s5k5cag_sensor.set_exposure = NULL;

	s5k5cag_sensor.set_vts = NULL;
	s5k5cag_sensor.get_vts_reg_addr = NULL;

	s5k5cag_sensor.sensor_gain_to_iso = NULL;
	s5k5cag_sensor.sensor_iso_to_gain = NULL;
	s5k5cag_sensor.get_ccm_pregain = NULL;

	s5k5cag_sensor.get_sensor_aperture = NULL;
	s5k5cag_sensor.get_equivalent_focus = NULL;

	s5k5cag_sensor.isp_location = CAMERA_USE_SENSORISP;
	s5k5cag_sensor.sensor_tune_ops = (isp_tune_ops *)kmalloc(sizeof(isp_tune_ops), GFP_KERNEL);

	if (s5k5cag_sensor.sensor_tune_ops == NULL) {
		print_error("failed to kmalloc isp_tune_ops");
		return -ENOMEM;
	}

	s5k5cag_sensor.af_enable = 0;

	s5k5cag_sensor.image_setting.lensc_param = NULL;//s5k5cag_byd_lensc_param;
	s5k5cag_sensor.image_setting.ccm_param = NULL;//s5k5cag_byd_ccm_param;
	s5k5cag_sensor.image_setting.awb_param = NULL;//s5k5cag_byd_awb_param;


	s5k5cag_sensor.fps_max = 30;
	s5k5cag_sensor.fps_min = 16;
	s5k5cag_sensor.fps = 25;

	s5k5cag_sensor.owner = THIS_MODULE;

	s5k5cag_sensor.info.facing = CAMERA_FACING_BACK;
	s5k5cag_sensor.info.orientation = 270;
	s5k5cag_sensor.lane_clk = CLK_400M; //zhangjun
	s5k5cag_sensor.hflip = 0;
	s5k5cag_sensor.vflip = 0;
	s5k5cag_sensor.old_flip = 0;

	memset(s5k5cag_sensor.sensor_tune_ops, 0, sizeof(isp_tune_ops));
	s5k5cag_sensor.sensor_tune_ops->set_saturation	= s5k5cag_set_saturation;
	s5k5cag_sensor.sensor_tune_ops->set_contrast 	= s5k5cag_set_contrast;
	s5k5cag_sensor.sensor_tune_ops->set_brightness 	= s5k5cag_set_brightness;
	s5k5cag_sensor.sensor_tune_ops->set_iso 		= s5k5cag_set_iso;
	s5k5cag_sensor.sensor_tune_ops->set_ev 		= s5k5cag_set_ev;
	s5k5cag_sensor.sensor_tune_ops->set_effect		= s5k5cag_set_effect;
	s5k5cag_sensor.sensor_tune_ops->set_awb		= s5k5cag_set_awb;
	s5k5cag_sensor.sensor_tune_ops->set_anti_banding = NULL;
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_module_init;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static __init int s5k5cag_module_init(void)
{
	s5k5cag_set_default();
	return register_camera_sensor(s5k5cag_sensor.sensor_index, &s5k5cag_sensor);
}

/*
 **************************************************************************
 * FunctionName: s5k5cag_module_exit;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void __exit s5k5cag_module_exit(void)
{
	if (s5k5cag_sensor.sensor_tune_ops) {
		kfree(s5k5cag_sensor.sensor_tune_ops);
		s5k5cag_sensor.sensor_tune_ops = NULL;
	}
	unregister_camera_sensor(s5k5cag_sensor.sensor_index, &s5k5cag_sensor);
}
/*
 **************************************************************************
 * FunctionName: s5k5cag_change_frame_rate;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void s5k5cag_change_frame_rate(camera_frame_rate_mode fps_mode)
{
	if (fps_mode == CAMERA_FRAME_RATE_FIX_MAX){
		/*Set max frame rate*/
		s5k5cag_write_reg(0xC88C, 0x1D99, I2C_16BIT, 0);
		/*Set min frame rate*/
		s5k5cag_write_reg(0xC88E, 0x1D99, I2C_16BIT, 0);
	} else {
		/*Set max frame rate*/
		s5k5cag_write_reg(0xC88C, 0x1D99, I2C_16BIT, 0);
		/*Set min frame rate*/
		s5k5cag_write_reg(0xC88E, 0x0A00, I2C_16BIT, 0);
	}

	s5k5cag_write_reg(0x098E, 0xDC00, I2C_16BIT, 0);
	/*Change-Config*/
	s5k5cag_write_reg(0xDC00, 0x28, I2C_8BIT, 0);
	/*Set COMMAND_REGISTER to (HOST_COMMAND_OK | HOST_COMMAND_1)*/
	s5k5cag_write_reg(0x0080, 0x8002, I2C_16BIT, 0);
	/*Delay 3ms*/
	s5k5cag_write_reg(0x0000, 3, I2C_16BIT, 0);
}
/*
 **************************************************************************
 * FunctionName: s5k5cag_update_frame_rate;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void s5k5cag_update_frame_rate(void){
	if (s5k5cag_sensor.fps_max == s5k5cag_sensor.fps_min) {
		s5k5cag_change_frame_rate(CAMERA_FRAME_RATE_FIX_MAX);
	} else {
		s5k5cag_change_frame_rate(CAMERA_FRAME_RATE_AUTO);
	}
}
MODULE_AUTHOR("Hisilicon");
module_init(s5k5cag_module_init);
module_exit(s5k5cag_module_exit);
MODULE_LICENSE("GPL");

/********************************** END **********************************************/
