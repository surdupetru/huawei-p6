/*
 *  mt9m114 camera driver source file
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
#include "mt9m114.h"
/*#include "../isp/k3_isp_io.h"*/
#include <asm/bug.h>
#include <linux/device.h>

#define LOG_TAG "MT9M114"
/* #define DEBUG_DEBUG 1 */
#include "../isp/cam_log.h"
#include "../isp/cam_dbg.h"
#include <hsad/config_interface.h>

#define QCIF_WIDTH		176
#define MT9M114_APERTURE_FACTOR		240 // F2.4
#define MT9M114_EQUIVALENT_FOCUS	31  // 31mm

#define MT9M114_BYD_SLAVE_ADDRESS     0x90
#define MT9M114_SUNNY_SLAVE_ADDRESS     0x90
#define MT9M114_FOXCONN_SLAVE_ADDRESS 0xBA
#define MT9M114_CHIP_ID       (0x2481)

#define MT9M114_CAM_MODULE_SKIPFRAME     4

#define MT9M114_NO_FLIP	0x00
#define MT9M114_H_FLIP	       0x01
#define MT9M114_V_FLIP	       0x02
#define MT9M114_HV_FLIP	0x03

#define MT9M114_EXPOSURE_REG	0x3012
#define MT9M114_GAIN_REG		0x305e

#define MT9M114_FLASH_TRIGGER_GAIN	255

#define MT9M114_HFLIP		((1 << CAMERA_H_FLIP) |\
					(1 << CAMERA_V_FLIP))

#define MT9M114_VFLIP		(\
					(1 << CAMERA_NO_FLIP) | \
					(1 << CAMERA_H_FLIP) \
				)

#define MT9M114_EFL_BYD		(223)
#define MT9M114_EFL_SUNNY		(214)
#define MT9M114_EFL_FOXCONN		(222)

static camera_capability mt9m114_cap[] = {
	{V4L2_CID_AUTO_WHITE_BALANCE, THIS_AUTO_WHITE_BALANCE},
	{V4L2_CID_DO_WHITE_BALANCE, THIS_WHITE_BALANCE},
	{V4L2_CID_EFFECT, THIS_EFFECT},
	{V4L2_CID_HFLIP, MT9M114_HFLIP},
	{V4L2_CID_VFLIP, MT9M114_VFLIP},
};

const struct isp_reg_t isp_init_regs_mt9m114[] = {
    {0x63022, 0x98},
    {0x63c12, 0x04},
    /*{0x63b34,0x03},*/
    /*{0x6502f,0x21},*/
};

/* Fixme: 50/60Hz anti-banding params should be calibrated. */
static framesize_s mt9m114_framesizes[] = {
	{0, 120, 1280, 720, 1604, 1101, 30, 30, 0, 0, 0x100, VIEW_FULL, RESOLUTION_16_9, false, {mt9m114_framesize_full, ARRAY_SIZE(mt9m114_framesize_full)} },
	{0, 0, 1280, 960, 1604, 1101, 30, 30, 0, 0, 0x100, VIEW_FULL, RESOLUTION_4_3, false, {mt9m114_framesize_full, ARRAY_SIZE(mt9m114_framesize_full)} },
};

/******************************************/

typedef struct _sensor_module{
	char  *sensor_name;
	u16   chip_id;
	u32   slave_addr;
	u32   gpio_14_0_val;
	struct _sensor_reg_t *preg;
	u32   reg_size;
} sensor_module;

static sensor_module mt9m114_sensor_module[] = {
	{"mt9m114_byd", MT9M114_CHIP_ID, MT9M114_BYD_SLAVE_ADDRESS, 1, mt9m114_byd_init_regs, ARRAY_SIZE(mt9m114_byd_init_regs)},
	{"mt9m114_sunny", MT9M114_CHIP_ID, MT9M114_SUNNY_SLAVE_ADDRESS, 0, mt9m114_sunny_init_regs, ARRAY_SIZE(mt9m114_sunny_init_regs)},
	{"mt9m114_foxconn", MT9M114_CHIP_ID, MT9M114_FOXCONN_SLAVE_ADDRESS, 1, mt9m114_foxconn_init_regs, ARRAY_SIZE(mt9m114_foxconn_init_regs)},
};

static int camera_index = -1;
/******************************************/

static camera_sensor mt9m114_sensor;
static bool sensor_inited;
static camera_effects sensor_effect = CAMERA_EFFECT_NONE;
static camera_white_balance sensor_awb_mode = CAMERA_WHITEBALANCE_AUTO;
static camera_frame_rate_mode sensor_framerate_mode = CAMERA_FRAME_RATE_AUTO;
static void mt9m114_set_default(void);
static void mt9m114_update_frame_rate(camera_frame_rate_mode fps_mode);
#ifdef CONFIG_DEBUG_FS
extern u32 get_slave_sensor_id(void);
#endif
/*
 **************************************************************************
 * FunctionName: mt9m114_read_reg;
 * Description : read mt9m114 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_read_reg(u16 reg, u16 *val)
{
	return k3_ispio_read_reg(mt9m114_sensor.i2c_config.index,
				 mt9m114_sensor.i2c_config.addr, reg, val, mt9m114_sensor.i2c_config.val_bits);
}

/*
 **************************************************************************
 * FunctionName: mt9m114_write_reg;
 * Description : write mt9m114 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_write_reg(u16 reg, u16 val, i2c_length length, u8 mask)
{
	return k3_ispio_write_reg(mt9m114_sensor.i2c_config.index,
			mt9m114_sensor.i2c_config.addr, reg, val, length, mask);
}

/*
 **************************************************************************
 * FunctionName: mt9m114_write_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int mt9m114_write_seq(const struct _sensor_reg_t *seq, u32 size, u8 mask)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	return k3_ispio_write_seq(mt9m114_sensor.i2c_config.index,
		mt9m114_sensor.i2c_config.addr, seq, size, mt9m114_sensor.i2c_config.val_bits, mask);
}

/*
 **************************************************************************
 * FunctionName: mt9m114_write_isp_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void mt9m114_write_isp_seq(const struct isp_reg_t *seq, u32 size)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	k3_ispio_write_isp_seq(seq, size);
}

/*
 **************************************************************************
 * FunctionName: mt9m114_enum_frame_intervals;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_enum_frame_intervals(struct v4l2_frmivalenum *fi)
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
 * FunctionName: mt9m114_get_format;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_get_format(struct v4l2_fmtdesc *fmt)
{
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_OVERLAY) {
		fmt->pixelformat = mt9m114_sensor.fmt[STATE_PREVIEW];
	} else {
		fmt->pixelformat = mt9m114_sensor.fmt[STATE_CAPTURE];
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: mt9m114_enum_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_enum_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	u32 max_index = ARRAY_SIZE(camera_framesizes) - 1;
	u32 this_max_index = ARRAY_SIZE(mt9m114_framesizes) - 1;

	assert(framesizes);

	print_debug("enter %s; ", __func__);

	if (framesizes->index > max_index) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	if ((camera_framesizes[framesizes->index].width > mt9m114_framesizes[this_max_index].width)
		|| (camera_framesizes[framesizes->index].height > mt9m114_framesizes[this_max_index].height)) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	framesizes->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	framesizes->discrete.width = mt9m114_framesizes[this_max_index].width;
	framesizes->discrete.height = mt9m114_framesizes[this_max_index].height;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: mt9m114_try_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_try_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	int max_index = ARRAY_SIZE(mt9m114_framesizes) - 1;

	assert(framesizes);

	print_debug("Enter Function:%s  ", __func__);

	if ((framesizes->discrete.width <= mt9m114_framesizes[max_index].width)
	    && (framesizes->discrete.height <= mt9m114_framesizes[max_index].height)) {
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
 * FunctionName: mt9m114_set_framesizes;
 * Description : NA;
 * Input       : flag: if 1, set framesize to sensor,
 *					   if 0, only store framesize to camera_interface;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_set_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs, int flag, camera_setting_view_type view_type)
{
	int i = 0;

	assert(fs);

	print_debug("Enter Function:%s State(%d), flag=%d, width=%d, height=%d",
		    __func__, state, flag, fs->width, fs->height);

	if (QCIF_WIDTH == fs->width)
		i = 0;
	else
		i = 1;
	fs->width = mt9m114_framesizes[i].width;
	fs->height = mt9m114_framesizes[i].height;

	if (i >= ARRAY_SIZE(mt9m114_framesizes)) {
		print_error("request resolution larger than sensor's max resolution");
		return -EINVAL;
	}

	if (state == STATE_PREVIEW) {
		mt9m114_sensor.preview_frmsize_index = i;
	} else {
		mt9m114_sensor.capture_frmsize_index = i;
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: mt9m114_get_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_get_framesizes(camera_state state,
				  struct v4l2_frmsize_discrete *fs)
{
	int frmsize_index;

	assert(fs);

	if (state == STATE_PREVIEW) {
		frmsize_index = mt9m114_sensor.preview_frmsize_index;
	} else if (state == STATE_CAPTURE) {
		frmsize_index = mt9m114_sensor.capture_frmsize_index;
	} else {
		return -EINVAL;
	}
	fs->width = mt9m114_framesizes[frmsize_index].width;
	fs->height = mt9m114_framesizes[frmsize_index].height;

	return 0;
}

void mt9m114_set_effect(camera_effects effect)
{
	print_debug("%s, effect:%d", __func__, effect);
	if (false == sensor_inited) {
		sensor_effect = effect;
		return;
	}

	switch (effect) {
	case CAMERA_EFFECT_NONE:
		mt9m114_write_reg(0xc874, 0, I2C_8BIT, 0x00);
		break;
	case CAMERA_EFFECT_MONO:
		mt9m114_write_reg(0xc874, 1, I2C_8BIT, 0x00);
		break;
	case CAMERA_EFFECT_SEPIA:
		mt9m114_write_reg(0xc874, 2, I2C_8BIT, 0x00);
		break;
	case CAMERA_EFFECT_NEGATIVE:
		mt9m114_write_reg(0xc874, 3, I2C_8BIT, 0x00);
		break;
	case CAMERA_EFFECT_SOLARIZE:
		mt9m114_write_reg(0xc874, 4, I2C_8BIT, 0x00);
		break;
	default:
		print_error("%s, Unsupport effect:%d", __func__, effect);
		return;
	}

	mt9m114_write_reg(0x098e, 0xc874, I2C_16BIT, 0x00);
	mt9m114_write_reg(0xdc00, 0x28, I2C_8BIT, 0x00);
	mt9m114_write_reg(0x0080, 0x8004, I2C_16BIT, 0x00);
	sensor_effect = effect;
}

void mt9m114_set_awb(camera_white_balance awb_mode)
{
	if (false == sensor_inited) {
		sensor_awb_mode = awb_mode;
		return;
	}

	switch (awb_mode) {
	case CAMERA_WHITEBALANCE_AUTO:
		print_info("AUTO WB");
		mt9m114_write_reg(0x098E, 0x0000, I2C_16BIT, 0x00);
		mt9m114_write_reg(0xC909, 0x03, I2C_8BIT, 0x00);
		break;
	case CAMERA_WHITEBALANCE_INCANDESCENT:
		print_info("INCANDESCENT");
		mt9m114_write_reg(0x098E, 0x0000, I2C_16BIT, 0x00);
		mt9m114_write_reg(0xC909, 0x01, I2C_8BIT, 0x00);
		mt9m114_write_reg(0xC8F0, 0x0AF0, I2C_16BIT, 0x00);  /* 2800K */
		break;
	case CAMERA_WHITEBALANCE_FLUORESCENT:
		print_info("FLUORESCENT");
		mt9m114_write_reg(0x098E, 0x0000, I2C_16BIT, 0x00);
		mt9m114_write_reg(0xC909, 0x01, I2C_8BIT, 0x00);
		mt9m114_write_reg(0xC8F0, 0x1068, I2C_16BIT, 0x00);  /* 4200K */
		break;
	case CAMERA_WHITEBALANCE_DAYLIGHT:
		print_info("DAYLIGHT");
		mt9m114_write_reg(0x098E, 0x0000, I2C_16BIT, 0x00);
		mt9m114_write_reg(0xC909, 0x01, I2C_8BIT, 0x00);
		mt9m114_write_reg(0xC8F0, 0x1388, I2C_16BIT, 0x00);  /* 5000K */
		break;
	case CAMERA_WHITEBALANCE_CLOUDY_DAYLIGHT:
		print_info("CLOUDY_DAYLIGHT");
		mt9m114_write_reg(0x098E, 0x0000, I2C_16BIT, 0x00);
		mt9m114_write_reg(0xC909, 0x01, I2C_8BIT, 0x00);
		mt9m114_write_reg(0xC8F0, 0x1964, I2C_16BIT, 0x00);  /* 6500K */
		break;
	default:
		print_error("%s, Unsupport awb_mode:%d", __func__, awb_mode);
		return;
	}
	sensor_awb_mode = awb_mode;
}

static int mt9m114_set_anti_banding(camera_anti_banding banding)
{
	print_debug("%s, anti-banding:%#x", __func__, banding);

	switch (banding) {
	case CAMERA_ANTI_BANDING_50Hz:
		mt9m114_write_reg(0x098E, 0xC88B, I2C_16BIT, 0x00);  /* LOGICAL_ADDRESS_ACCESS [CAM_AET_FLICKER_FREQ_HZ] */
		mt9m114_write_reg(0xC88B, 0x32, I2C_8BIT, 0x00);     /* CAM_AET_FLICKER_FREQ_HZ 50Hz */
		mt9m114_write_reg(0xDC00, 0x28, I2C_8BIT, 0x00);     /* SYSMGR_NEXT_STATE */
		mt9m114_write_reg(0x0080, 0x8002, I2C_16BIT, 0x00);  /* COMMAND_REGISTER */
		msleep(50);
		break;
	case CAMERA_ANTI_BANDING_60Hz:
		mt9m114_write_reg(0x098E, 0xC88B, I2C_16BIT, 0x00);  /* LOGICAL_ADDRESS_ACCESS [CAM_AET_FLICKER_FREQ_HZ] */
		mt9m114_write_reg(0xC88B, 0x3C, I2C_8BIT, 0x00);     /* CAM_AET_FLICKER_FREQ_HZ 60Hz */
		mt9m114_write_reg(0xDC00, 0x28, I2C_8BIT, 0x00);     /* SYSMGR_NEXT_STATE */
		mt9m114_write_reg(0x0080, 0x8002, I2C_16BIT, 0x00);  /* COMMAND_REGISTER */
		msleep(50);
		break;
	default:
		print_error("%s, Unsupport anti-banding:%#x", __func__, banding);
		return -1;
	}

	return 0;
}

static void mt9m114_check_state(u8 mask)
{
	u16 val = 0;
	int count = 0;
	do {
		msleep(5);
		mt9m114_read_reg(0x0080, &val);
		if (count++ > 20)
			break;
	} while (val & mask);
}

static int mt9m114_check_sensor_stable()
{
	u16 data = 0;
	int i = 0;

	/* Polling sensor status register before initialization. */
	for (i = 0; i < 10; i++) {
		/* Repeat 10 times to read status register, '0x5200' means the sensor is ok, it can be initialized. */
		msleep(10);
		mt9m114_write_reg(0x098E, 0xDC01, I2C_16BIT, 0);
		mt9m114_read_reg(0xDC01, &data);
		if (0x5200 == data) {
			return 0;
		}
	}
	print_error("%s Sensor resetting is not finished. status = %#x", __func__, data);
	return 1;
}

/*
 **************************************************************************
 * FunctionName: mt9m114_init_reg;
 * Description : download initial seq for sensor init;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_init_reg(void)
{
	int index;
	int size = 0;
	int i;
	int ret = 0;

	print_debug("Enter Function:%s", __func__);

	mt9m114_check_sensor_stable();

	size = sizeof(isp_init_regs_mt9m114)/sizeof(isp_init_regs_mt9m114[0]);
	mt9m114_write_isp_seq(isp_init_regs_mt9m114, size);

	ret = k3_ispio_init_csi(mt9m114_sensor.mipi_index, mt9m114_sensor.mipi_lane_count, mt9m114_sensor.lane_clk);
	if (ret) {
		print_error("fail to init csi");
		return ret;
	}

	index = camera_index;
	size = mt9m114_sensor_module[index].reg_size;
	for (i = 0; i < size; i++) {
		if (0 == mt9m114_sensor_module[index].preg[i].subaddr) {
			/*mt9m114_check_state(mt9m114_sensor_module[index].preg[i].mask); */
			msleep(mt9m114_sensor_module[index].preg[i].value);
		} else {
			switch (mt9m114_sensor_module[index].preg[i].size) {
			case 1:
				mt9m114_write_reg(mt9m114_sensor_module[index].preg[i].subaddr, mt9m114_sensor_module[index].preg[i].value & 0xff, I2C_8BIT, 0x00);
				break;
			case 2:
				if (mt9m114_sensor_module[index].preg[i].subaddr == MT9M114REG_FLIP) {
					if(E_CAMERA_SENSOR_FLIP_TYPE_NONE == get_secondary_sensor_flip_type())
					{
						mt9m114_write_reg(MT9M114REG_FLIP,
							(mt9m114_sensor.hflip | mt9m114_sensor.vflip<<1) & 0x03, I2C_16BIT, 0x00);
					}
					else
					{
						mt9m114_write_reg(MT9M114REG_FLIP,
							~(mt9m114_sensor.hflip | mt9m114_sensor.vflip<<1) & 0x03, I2C_16BIT, 0x00);
					}
				} else {
					mt9m114_write_reg(mt9m114_sensor_module[index].preg[i].subaddr, mt9m114_sensor_module[index].preg[i].value & 0xffff, I2C_16BIT, 0x00);
				}
				break;
			case 4:
				mt9m114_write_reg(mt9m114_sensor_module[index].preg[i].subaddr, (mt9m114_sensor_module[index].preg[i].value >> 16) & 0xffff, I2C_16BIT, 0x00);
				mt9m114_write_reg(mt9m114_sensor_module[index].preg[i].subaddr + 2, mt9m114_sensor_module[index].preg[i].value & 0xffff, I2C_16BIT, 0x00);
				break;
			}
		}
	}

	msleep(100);
	mt9m114_write_reg(0x3c40, 0x7834, I2C_16BIT, 0x00);
	sensor_inited = true;
	if (CAMERA_EFFECT_NONE != sensor_effect)
		mt9m114_set_effect(sensor_effect);
	mt9m114_set_awb(sensor_awb_mode);
	mt9m114_update_frame_rate(sensor_framerate_mode);
	return ret;
}

static int mt9m114_get_capability(u32 id, u32 *value)
{
	int i;
	for (i = 0; i < sizeof(mt9m114_cap) / sizeof(mt9m114_cap[0]); ++i) {
		if (id == mt9m114_cap[i].id) {
			*value = mt9m114_cap[i].value;
			break;
		}
	}

	if(V4L2_CID_FOCAL_LENGTH==id){
		switch(camera_index){
		case 0:
			*value = MT9M114_EFL_BYD;
			break;
		case 1:
			*value = MT9M114_EFL_SUNNY;
			break;
		case 2:
			*value = MT9M114_EFL_FOXCONN;
			break;
		default:
			break;
		}
	}

	return 0;
}

static int mt9m114_update_flip(u16 width, u16 height)
{
	u16 reg = 0;
	u8 new_flip = ((mt9m114_sensor.vflip << 1) | mt9m114_sensor.hflip);
	print_debug("enter %s", __func__);

	if (mt9m114_sensor.old_flip != new_flip) {

		if (mt9m114_sensor.hflip)
			reg |= 0x01;

		if (mt9m114_sensor.vflip)
			reg |= 0x02;
		mt9m114_sensor.old_flip = new_flip;
		if(E_CAMERA_SENSOR_FLIP_TYPE_NONE == get_secondary_sensor_flip_type())
		{ 
			reg = reg & 0x03;
		}
		else
		{
			reg = ~reg & 0x03;
		}
		mt9m114_write_reg(MT9M114REG_FLIP, reg, I2C_16BIT, 0x00);
		mt9m114_write_reg(0x098e, 0xdc00, I2C_16BIT, 0x00);
		mt9m114_write_reg(0xdc00, 0x28, I2C_8BIT, 0x00);
		mt9m114_write_reg(0x0080, 0x8002, I2C_16BIT, 0x00);
		msleep(250);
	}

	return 0;
}

static u32 mt9m114_get_gain(void)
{
	u16 gain_reg_val = 0;
	u32 digital_gain, analog_gain;
	mt9m114_read_reg(MT9M114_GAIN_REG, &gain_reg_val);
	digital_gain = (((gain_reg_val >> 15) & 0x01) * 8)
	    + (((gain_reg_val >> 14) & 0x01) * 4)
	    + (((gain_reg_val >> 13) & 0x01) * 2)
	    + ((gain_reg_val >> 12) & 0x01);
	analog_gain = (1 << ((gain_reg_val >> 6) & 0x03))
	    * (gain_reg_val & 0x3f);
	return digital_gain * analog_gain;
}

static u32 mt9m114_get_exposure(void)
{
	u16 exposure = 0;
	mt9m114_read_reg(MT9M114_EXPOSURE_REG, &exposure);
	return exposure * 33;
}

/*
 **************************************************************************
 * FunctionName: mt9m114_framesize_switch;
 * Description : switch frame size, used by preview and capture
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_framesize_switch(camera_state state)
{
	u32 size = 0;
	u8 next_frmsize_index;

	if (state == STATE_PREVIEW)
		next_frmsize_index = mt9m114_sensor.preview_frmsize_index;
	else
		next_frmsize_index = mt9m114_sensor.capture_frmsize_index;

	print_info("Enter Function:%s frm index=%d", __func__, next_frmsize_index);
	mt9m114_update_flip(mt9m114_framesizes[next_frmsize_index].width, mt9m114_framesizes[next_frmsize_index].height);

	return 0;
}

/*
 **************************************************************************
 * FunctionName: mt9m114_stream_on;
 * Description : download preview seq for sensor preview;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_stream_on(camera_state state)
{
	print_debug("Enter Function:%s ", __func__);
	return mt9m114_framesize_switch(state);
}

/*
 **************************************************************************
 * FunctionName: mt9m114_find_sensor;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int mt9m114_find_sensor(sensor_module fix_sensor, int index)
{
	u16 id = 0;
	mt9m114_sensor.i2c_config.addr = fix_sensor.slave_addr;

	mt9m114_check_sensor_stable();

	mt9m114_read_reg(0x0000, &id);
	print_info("mt9m114 product id:0x%x", id);
#ifdef CONFIG_DEBUG_FS
	if (fix_sensor.chip_id != id) {
		id = (u16)get_slave_sensor_id();
		print_info("mt9m114 debugfs product id:0x%x", id);
	}
#endif
	if (fix_sensor.chip_id == id) {
		camera_index = index;
		memset(mt9m114_sensor.info.name, 0, sizeof(mt9m114_sensor.info.name));
		strncpy(mt9m114_sensor.info.name, fix_sensor.sensor_name, sizeof(mt9m114_sensor.info.name) - 1);
		print_info("mt9m114 module slave address=%d, %s verify successfully.\n", fix_sensor.slave_addr, mt9m114_sensor.info.name);
		return 1;
	} else {
		print_info("mt9m114 module slave address=%d, %s verify fail. chip_id[%#x], id[%#x]\n",
			fix_sensor.slave_addr, mt9m114_sensor.info.name, fix_sensor.chip_id, id);
		return 0;
	}
}

/*  **************************************************************************
* FunctionName: mt9m114_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static int mt9m114_check_sensor(void)
{
	int size;
	int i;
	int gpio_value = -1;

	gpio_value = gpio_get_value(GPIO_14_0);
	print_info("mt9m114 product i2c  gpio_value= %d", gpio_value);
	size = sizeof(mt9m114_sensor_module) / sizeof(mt9m114_sensor_module[0]);

	for (i = 0; i < size; i++) {
		if ((gpio_value == 0) && (mt9m114_sensor_module[i].gpio_14_0_val) == 0) {
			if (mt9m114_find_sensor(mt9m114_sensor_module[i], i))
				return 0;
		} else if ((gpio_value > 0) && (mt9m114_sensor_module[i].gpio_14_0_val > 0)) {
			if (mt9m114_find_sensor(mt9m114_sensor_module[i], i))
				return 0;
		}
	}
	print_error("Invalid product id ,Could not load sensor mt9m114");
	return -EFAULT;
}

/****************************************************************************
* FunctionName: mt9m114_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
int mt9m114_power(camera_power_state power)
{
	int ret = 0;

	if (power == POWER_ON) {
		k3_ispldo_power_sensor(power, "camera-vcc");

		ret = k3_ispio_ioconfig(&mt9m114_sensor, power);

		k3_ispgpio_reset_sensor(mt9m114_sensor.sensor_index, POWER_ON,
					mt9m114_sensor.power_conf.reset_valid);

		k3_ispldo_power_sensor(power, "pri-cameralog-vcc");
		ret = camera_power_core_ldo(power);
		k3_ispldo_power_sensor(power, "sec-cameralog-vcc");

		ret = k3_ispgpio_power_sensor(&mt9m114_sensor, power);
	} else {
		k3_ispio_deinit_csi(mt9m114_sensor.mipi_index);
		ret = k3_ispio_ioconfig(&mt9m114_sensor, power);
		ret = k3_ispgpio_power_sensor(&mt9m114_sensor, power);

		k3_ispldo_power_sensor(power, "sec-cameralog-vcc");
		camera_power_core_ldo(power);
		k3_ispldo_power_sensor(power, "pri-cameralog-vcc");
		k3_ispldo_power_sensor(power, "camera-vcc");
		sensor_inited = false;
	}
	return ret;
}
/*
 * Here gain is in unit 1/16 of sensor gain,
 * y36721 todo, temporarily if sensor gain=0x10, ISO is 100
 * in fact we need calibrate an ISO-ET-gain table.
 */
u32 mt9m114_gain_to_iso(int gain)
{
	return (gain * 100) / 0x10;
}

u32 mt9m114_iso_to_gain(int iso)
{
	return (iso * 0x10) / 100;
}

void mt9m114_set_gain(u32 gain)
{
}

void mt9m114_set_exposure(u32 exposure)
{
}

/*
 **************************************************************************
 * FunctionName: mt9m114_reset;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_reset(camera_power_state power_state)
{
	print_debug("%s  ", __func__);

	if (POWER_ON == power_state) {
		k3_ispgpio_reset_sensor(mt9m114_sensor.sensor_index, POWER_ON,
					mt9m114_sensor.power_conf.reset_valid);
		k3_isp_io_enable_mclk(MCLK_ENABLE, mt9m114_sensor.sensor_index);
		k3_ispgpio_reset_sensor(mt9m114_sensor.sensor_index, POWER_OFF,
					mt9m114_sensor.power_conf.reset_valid);
		udelay(400);
		k3_ispgpio_reset_sensor(mt9m114_sensor.sensor_index, POWER_ON,
					mt9m114_sensor.power_conf.reset_valid);
	} else {
		k3_ispgpio_reset_sensor(mt9m114_sensor.sensor_index, POWER_OFF,
			      mt9m114_sensor.power_conf.reset_valid);
		k3_isp_io_enable_mclk(MCLK_DISABLE, mt9m114_sensor.sensor_index);
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: mt9m114_init;
 * Description : mt9m114 init function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : Error code indicating success or failure;
 * Other       : NA;
 **************************************************************************
*/
static int mt9m114_init(void)
{
	static bool mt9m114_check = false;
	print_debug("%s  ", __func__);

	if (false == mt9m114_check)
	{
		if (check_suspensory_camera("MT9M114") != 1)
		{
			return -ENODEV;
		}
		mt9m114_check = true;
	}
	
	if (mt9m114_sensor.owner && !try_module_get(mt9m114_sensor.owner)) {
		print_error("%s: try_module_get fail", __func__);
		return -ENOENT;
	}

	k3_ispio_power_init("pri-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V */
	k3_ispio_power_init("camera-vcc", LDO_VOLTAGE_18V, LDO_VOLTAGE_18V);	/*IO 1.8V */
	k3_ispio_power_init("sec-cameralog-vcc", LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V */

	return 0;
}

/*
 **************************************************************************
 * FunctionName: mt9m114_exit;
 * Description : mt9m114 exit function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void mt9m114_exit(void)
{
	print_debug("enter %s", __func__);

	k3_ispio_power_deinit();

	if (mt9m114_sensor.owner) {
		module_put(mt9m114_sensor.owner);
	}
	print_debug("exit %s", __func__);
}

/*
 **************************************************************************
 * FunctionName: mt9m114_shut_down;
 * Description : mt9m114 shut down function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void mt9m114_shut_down(void)
{
	print_debug("enter %s", __func__);
	k3_ispgpio_reset_sensor(CAMERA_SENSOR_SECONDARY , POWER_OFF, mt9m114_sensor.power_conf.reset_valid);
}

static int mt9m114_set_hflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);
	mt9m114_sensor.hflip = flip;
	return 0;
}

static int mt9m114_get_hflip(void)
{
	print_debug("enter %s", __func__);
	return mt9m114_sensor.hflip;
}

static int mt9m114_set_vflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);

	mt9m114_sensor.vflip = flip;
	return 0;
}

static int mt9m114_get_vflip(void)
{
	print_debug("enter %s", __func__);
	return mt9m114_sensor.vflip;
}

static int mt9m114_get_sensor_aperture(void)
{
	print_info("enter %s", __func__);
	return MT9M114_APERTURE_FACTOR;
}

static int mt9m114_get_equivalent_focus()
{
	print_info("enter %s", __func__);
	return MT9M114_EQUIVALENT_FOCUS;
}

static u32 mt9m114_get_override_param(camera_override_type_t type)
{
	u32 ret_val = sensor_override_params[type];

	switch (type) {
	case OVERRIDE_FLASH_TRIGGER_GAIN:
		ret_val = MT9M114_FLASH_TRIGGER_GAIN;
		break;
	default:
		//print_error("%s:not override or invalid type %d, use default",__func__, type);
		break;
	}

	return ret_val;
}
/*
 **************************************************************************
 * FunctionName: mt9m114_set_default;
 * Description : init mt9m114_sensor;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void mt9m114_set_default(void)
{
	mt9m114_sensor.init = mt9m114_init;
	mt9m114_sensor.exit = mt9m114_exit;
	mt9m114_sensor.shut_down = mt9m114_shut_down;
	mt9m114_sensor.reset = mt9m114_reset;
	mt9m114_sensor.check_sensor = mt9m114_check_sensor;
	mt9m114_sensor.power = mt9m114_power;
	mt9m114_sensor.init_reg = mt9m114_init_reg;
	mt9m114_sensor.stream_on = mt9m114_stream_on;

	mt9m114_sensor.get_format = mt9m114_get_format;
	mt9m114_sensor.set_flash = NULL;
	mt9m114_sensor.get_flash = NULL;
	mt9m114_sensor.set_scene = NULL;
	mt9m114_sensor.get_scene = NULL;

	mt9m114_sensor.enum_framesizes = mt9m114_enum_framesizes;
	mt9m114_sensor.try_framesizes = mt9m114_try_framesizes;
	mt9m114_sensor.set_framesizes = mt9m114_set_framesizes;
	mt9m114_sensor.get_framesizes = mt9m114_get_framesizes;

	mt9m114_sensor.set_hflip = mt9m114_set_hflip;
	mt9m114_sensor.get_hflip = mt9m114_get_hflip;
	mt9m114_sensor.set_vflip = mt9m114_set_vflip;
	mt9m114_sensor.get_vflip = mt9m114_get_vflip;
	mt9m114_sensor.update_flip = mt9m114_update_flip;

	mt9m114_sensor.get_gain = mt9m114_get_gain;
	mt9m114_sensor.get_exposure = mt9m114_get_exposure;

	mt9m114_sensor.enum_frame_intervals = mt9m114_enum_frame_intervals;
	mt9m114_sensor.try_frame_intervals = NULL;
	mt9m114_sensor.set_frame_intervals = NULL;
	mt9m114_sensor.get_frame_intervals = NULL;

	mt9m114_sensor.get_capability = mt9m114_get_capability;

	mt9m114_sensor.update_framerate = mt9m114_update_frame_rate;

	strncpy(mt9m114_sensor.info.name, "mt9m114", sizeof(mt9m114_sensor.info.name));
	mt9m114_sensor.interface_type = MIPI2;
	mt9m114_sensor.mipi_lane_count = CSI_LINES_1;
	mt9m114_sensor.mipi_index = CSI_INDEX_1;
	mt9m114_sensor.sensor_index = CAMERA_SENSOR_SECONDARY;
	mt9m114_sensor.skip_frames = 0;

	mt9m114_sensor.power_conf.pd_valid = LOW_VALID;
	mt9m114_sensor.power_conf.reset_valid = LOW_VALID;
	mt9m114_sensor.power_conf.vcmpd_valid = LOW_VALID;

	mt9m114_sensor.i2c_config.index = I2C_PRIMARY;
	mt9m114_sensor.i2c_config.speed = I2C_SPEED_400;
	/*mt9m114_sensor.i2c_config.addr = MT9M114_SLAVE_ADDRESS;  //add in mt9m114_check_sensor()*/
	mt9m114_sensor.i2c_config.addr_bits = 16;
	mt9m114_sensor.i2c_config.val_bits = I2C_16BIT;

	mt9m114_sensor.preview_frmsize_index = 0;
	mt9m114_sensor.capture_frmsize_index = 0;
	mt9m114_sensor.frmsize_list = mt9m114_framesizes;
	mt9m114_sensor.fmt[STATE_PREVIEW] = V4L2_PIX_FMT_YUYV;
	mt9m114_sensor.fmt[STATE_CAPTURE] = V4L2_PIX_FMT_YUYV;

	mt9m114_sensor.aec_addr[0] = 0;
	mt9m114_sensor.aec_addr[1] = 0;
	mt9m114_sensor.agc_addr[0] = 0;
	mt9m114_sensor.agc_addr[1] = 0;

	mt9m114_sensor.set_gain = NULL;
	mt9m114_sensor.set_exposure = NULL;

	mt9m114_sensor.set_vts = NULL;
	mt9m114_sensor.get_vts_reg_addr = NULL;

	mt9m114_sensor.sensor_gain_to_iso = NULL;
	mt9m114_sensor.sensor_iso_to_gain = NULL;

	mt9m114_sensor.set_effect = mt9m114_set_effect;
	mt9m114_sensor.set_awb = mt9m114_set_awb;

	mt9m114_sensor.set_anti_banding = mt9m114_set_anti_banding;
	mt9m114_sensor.get_sensor_aperture = mt9m114_get_sensor_aperture;
	mt9m114_sensor.get_equivalent_focus = mt9m114_get_equivalent_focus;
	mt9m114_sensor.get_override_param = mt9m114_get_override_param;
	mt9m114_sensor.isp_location = CAMERA_USE_SENSORISP;
	mt9m114_sensor.sensor_tune_ops = (isp_tune_ops *)kmalloc(sizeof(isp_tune_ops), GFP_KERNEL);
	if (mt9m114_sensor.sensor_tune_ops == NULL) {
		print_error("failed to kmalloc isp_tune_ops");
		return -ENOMEM;
	}

	mt9m114_sensor.af_enable = 0;

	mt9m114_sensor.image_setting.lensc_param = NULL;/*mt9m114_lensc_param;*/
	mt9m114_sensor.image_setting.ccm_param = NULL;	/*mt9m114_ccm_param;*/
	mt9m114_sensor.image_setting.awb_param = NULL;	/*mt9m114_awb_param;*/

	mt9m114_sensor.fps_max = 30;
	mt9m114_sensor.fps_min = 16;
	mt9m114_sensor.fps = 25;

	mt9m114_sensor.owner = THIS_MODULE;

	mt9m114_sensor.info.facing = CAMERA_FACING_BACK;
	mt9m114_sensor.info.orientation = 270;
	mt9m114_sensor.lane_clk = CLK_750M;
	mt9m114_sensor.hflip = 0;
	mt9m114_sensor.vflip = 0;
	mt9m114_sensor.old_flip = 0;

	memset(mt9m114_sensor.sensor_tune_ops, 0, sizeof(isp_tune_ops));
	mt9m114_sensor.sensor_tune_ops->set_effect 	= mt9m114_set_effect;
	mt9m114_sensor.sensor_tune_ops->set_awb		= mt9m114_set_awb;
	mt9m114_sensor.sensor_tune_ops->set_anti_banding = NULL;

	mt9m114_sensor.lcd_compensation_supported = 0;
}

/*
 **************************************************************************
 * FunctionName: mt9m114_module_init;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static __init int mt9m114_module_init(void)
{
	mt9m114_set_default();
	return register_camera_sensor(mt9m114_sensor.sensor_index, &mt9m114_sensor);
}

/*
 **************************************************************************
 * FunctionName: mt9m114_module_exit;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void __exit mt9m114_module_exit(void)
{
	unregister_camera_sensor(mt9m114_sensor.sensor_index, &mt9m114_sensor);
}

/*
 **************************************************************************
 * FunctionName: mt9m114_update_frame_rate;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void mt9m114_update_frame_rate(camera_frame_rate_mode fps_mode)
{
	if (false == sensor_inited) {
		sensor_framerate_mode = fps_mode;
		return;
	}

	if (fps_mode == CAMERA_FRAME_RATE_FIX_MAX) {
		/*Set max frame rate */
		mt9m114_write_reg(0xC88C, 0x1D99, I2C_16BIT, 0);
		/*Set min frame rate */
		mt9m114_write_reg(0xC88E, 0x1D99, I2C_16BIT, 0);
	} else {
		/*Set max frame rate */
		mt9m114_write_reg(0xC88C, 0x1D99, I2C_16BIT, 0);
		/*Set min frame rate */
		mt9m114_write_reg(0xC88E, 0x0A00, I2C_16BIT, 0);
	}

	mt9m114_write_reg(0x098E, 0xDC00, I2C_16BIT, 0);
	/*Change-Config */
	mt9m114_write_reg(0xDC00, 0x28, I2C_8BIT, 0);
	/*Set COMMAND_REGISTER to (HOST_COMMAND_OK | HOST_COMMAND_1) */
	mt9m114_write_reg(0x0080, 0x8002, I2C_16BIT, 0);
	/*Delay 3ms */
	mt9m114_write_reg(0x0000, 3, I2C_16BIT, 0);
}
MODULE_AUTHOR("Hisilicon");
module_init(mt9m114_module_init);
module_exit(mt9m114_module_exit);
MODULE_LICENSE("GPL");

/********************************** END **********************************************/
