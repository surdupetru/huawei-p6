/*
 *  Hisilicon K3 soc camera ISP driver source file
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/ctype.h>
#include <mach/hisi_mem.h>
#include <mach/irqs.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>
#include <linux/fb.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <linux/io.h>
#include <linux/bug.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/android_pmem.h>
#include <linux/device.h>
#include <linux/kernel.h>

#include <linux/time.h>
#include <mach/boardid.h>
#include <linux/pm_qos_params.h>
#include "k3_v4l2_capture.h"
#include "k3_isp.h"

#define DEBUG_DEBUG 0
#define LOG_TAG "K3_ISP"
#include "cam_log.h"

/* MACRO DEFINITION */
#define	WIDTH_ALIGN		0x0f	/* step = 16 */
#define	HEIGHT_ALIGN		0x03	/* step = 4 */
/* #define HEIGHT_ALIGN  0x0f   step = 16  not support 1080p  */

#define DDR_PREVIEW_MIN_PROFILE 360000
#define DDR_CAPTURE_MIN_PROFILE 360000
#define GPU_INIT_BLOCK_PROFILE 120000
#define GPU_BLOCK_PROFILE 360000

#define WAIT_CHECK_FLASH_LEVEL_COMPLT_TIMEOUT 1000


/* VARIABLES AND ARRARYS */
static k3_isp_data isp_data;
static isp_hw_controller *isp_hw_ctl;
static isp_tune_ops *camera_tune_ops;
static camera_flash_state flash_exif = FLASH_OFF;


static camera_capability k3_cap[] = {
	{V4L2_CID_AUTO_WHITE_BALANCE, THIS_AUTO_WHITE_BALANCE},
	{V4L2_CID_DO_WHITE_BALANCE, THIS_WHITE_BALANCE},
	{V4L2_CID_BRIGHTNESS, THIS_BRIGHTNESS},
	{V4L2_CID_CONTRAST, THIS_CONTRAST},
	{V4L2_CID_SCENE, THIS_SCENE},
	{V4L2_CID_SATURATION, THIS_SATURATION},
	{V4L2_CID_ISO, THIS_ISO},
	{V4L2_CID_EFFECT, THIS_EFFECT},
	{V4L2_CID_EXPOSURE_MAX, THIS_EXPOSURE},
	{V4L2_CID_EXPOSURE_STEP, THIS_EXPOSURE_STEP},
	{V4L2_CID_SHARPNESS, THIS_SHARPNESS},
	{V4L2_CID_METERING, THIS_METERING},
	{V4L2_CID_HFLIP, THIS_HFLIP},
	{V4L2_CID_VFLIP, THIS_VFLIP},
	{V4L2_CID_POWER_LINE_FREQUENCY, THIS_ANTI_BANDING},
};

/* FUNCTION DECLEARATION */
static void k3_isp_set_default(void);
static void k3_isp_calc_zoom(camera_state state, scale_strategy_t scale_strategy, u32 *in_width, u32 *in_height);

#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
static struct pm_qos_request_list g_specialpolicy;
#endif


extern int ispv1_need_lcd_compensation(camera_sensor *sensor);

int k3_check_lcd_compensation_supported(void)
{
	if(NULL==isp_data.sensor)
		return 0;
	return isp_data.sensor->lcd_compensation_supported;
}

int k3_check_lcd_compensation_needed(void)
{
	if(NULL==isp_data.sensor)
		return 0;
	return ispv1_need_lcd_compensation(isp_data.sensor);
}
/*
 **************************************************************************
 * FunctionName: k3_isp_preview_switch_mode;
 * Description : switch preview mode between hdr and non hdr preview
 * Input       : preview mode include hdr movie and non hdr ;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
 void k3_isp_preview_switch_mode(camera_preview_mode mode)
{
	print_debug("enter %s mode = %d", __func__,mode);
	switch(mode)
	{
		case HDR_MOVIE_PREVIEW_MODE:
			isp_data.hdr_switch_on = true;
			ispv1_hdr_isp_ae_ctrl(0);
			ispv1_hdr_isp_ae_switch(1);
			break;
		case NON_HDR_PREVIEW_MODE:
			isp_data.hdr_switch_on = false;
			ispv1_hdr_isp_ae_ctrl(1);
			ispv1_hdr_isp_ae_switch(0);
			break;
		default:
			;
	}
}

/*
 **************************************************************************
 * FunctionName: k3_isp_check_config;
 * Description : Set yuv offset according to width, height and format;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_check_config(struct v4l2_pix_format *pixfmt, camera_state state)
{
	print_debug("enter %s", __func__);

	/* calculate U and V offset accroding to ISP out format */
	return isp_hw_ctl->isp_check_config(pixfmt);
}

int k3_isp_get_process_mode(void)
{
	isp_hw_ctl->isp_set_process_mode(isp_data.pic_attr[STATE_CAPTURE].sensor_width,
					isp_data.pic_attr[STATE_CAPTURE].sensor_height);
	return (int)isp_hw_ctl->isp_get_process_mode();
}


int k3_isp_get_k3_capability(u32 id, u32 *value)
{
	int i;
	print_debug("enter %s", __func__);

	if (V4L2_CID_ZOOM_RELATIVE == id) {
		/* higt byte: 0x10 for zoom_ratio(100,110,120...400) */
		/* low byte: ZOOM_STEPS(1, 1.1, 1.2...4.0) */
		*value = 0x0A001E;
		return 0;
	}

	for (i = 0; i < sizeof(k3_cap) / sizeof(k3_cap[0]); ++i) {
		if (id == k3_cap[i].id) {
			*value = k3_cap[i].value;
			break;
		}
	}

	return 0;
}

void k3_isp_check_flash_level(camera_flash_state state)
{
	camera_flashlight *flashlight = get_camera_flash();

	if (NULL == flashlight) {
		isp_data.flash_flow = FLASH_DONE;
		isp_data.flash_on = false;
		isp_data.flash_state = FLASH_OFF;
		flash_exif = FLASH_OFF;
		return;
	}
#if 0
	if (isp_data.flash_on == true)
		/* now flash light is on, isp is taking a picture */
		return;
#endif
	if ((isp_data.sensor->sensor_index == CAMERA_SENSOR_PRIMARY) && (flashlight != NULL) && (CAMERA_SHOOT_SINGLE == isp_data.shoot_mode)) {
		if (FLASH_ON == state) {
			if (((isp_data.flash_mode == CAMERA_FLASH_AUTO) && (true == isp_hw_ctl->isp_is_need_flash(isp_data.sensor))) ||
				isp_data.flash_mode == CAMERA_FLASH_ON) {


				flashlight->turn_on(TORCH_MODE, LUM_LEVEL2);
				isp_data.flash_state = state;
				isp_data.flash_on = true;
				isp_hw_ctl->isp_check_flash_prepare();

				isp_data.flash_flow = FLASH_TESTING;
				flash_exif = FLASH_ON;
				return;
			}
		} else if ((FLASH_OFF == state) && (FLASH_ON == isp_data.flash_state)) {
			isp_data.flash_state = state;
			return;
		}
	}

	isp_data.flash_flow = FLASH_DONE;
	isp_data.flash_on = false;
	isp_data.flash_state = FLASH_OFF;
	flash_exif = FLASH_OFF;
}

int k3_isp_get_flash_result(void)
{
	int ret;
	if (FLASH_DONE == isp_data.flash_flow)
		ret = 0;
	else
		ret = -1;
	return ret;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_hw_init_reg;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_hw_init_regs(camera_sensor *sensor)
{
	print_debug("enter %s", __func__);

	if (NULL == sensor) {
		print_error("%s par is NULL", __func__);
		return -EINVAL;
	}

	isp_hw_ctl->isp_hw_init_regs(sensor->interface_type);

	return 0;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_set_camera;
 * Description : Set a new camera sensor to isp;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_set_camera(camera_sensor *open_sensor, camera_sensor *close_sensor, sensor_known_t known_sensor)
{
	int ret = 0;

	/* power off the current sensor and exit it */
	if (isp_data.sensor) {
		isp_data.sensor->reset(POWER_OFF);
		isp_data.sensor->power(POWER_OFF);
		isp_data.sensor->exit();
		isp_data.sensor = NULL;
	}

	/* switch to new sensor */
	isp_data.sensor = open_sensor;

	if (isp_data.sensor->init && (0 != isp_data.sensor->init())) {
		print_error("%s: fail to init sensor", __func__);
		ret = -ENODEV;
		goto init_fail;
	}

	if (isp_hw_ctl->set_i2c)
		isp_hw_ctl->set_i2c(&isp_data.sensor->i2c_config);

	if (isp_data.sensor->power && (0 != isp_data.sensor->power(POWER_ON))) {
		print_error("%s: fail to power sensor", __func__);
		ret = -ENODEV;
		goto fail;
	}

	if (isp_data.sensor->reset && (0 != isp_data.sensor->reset(POWER_ON))) {
		print_error("%s: fail to reset sensor", __func__);
		ret = -ENODEV;
		goto fail;
	}

	/* make sure sensor is stable */
	msleep(10);

	if ((isp_data.sensor->check_sensor && (0 != isp_data.sensor->check_sensor()))) {
		ret = -ENODEV;
		goto fail;
	}

	if (CAMERA_SENSOR_KNOWN == known_sensor) {
		if (NULL != close_sensor)
			close_sensor->shut_down();

		if (isp_data.sensor->isp_location == CAMERA_USE_SENSORISP)
			camera_tune_ops = isp_data.sensor->sensor_tune_ops;
		else
			camera_tune_ops = isp_hw_ctl->isp_tune_ops;

		isp_hw_ctl->refresh_support_fmt(&isp_data.support_pixfmt, &isp_data.pixfmt_count);


		if (isp_data.sensor->get_frame_intervals)
			isp_data.sensor->get_frame_intervals(&isp_data.frame_rate);

		/* y36721 2012-04-12 change. */
		isp_hw_ctl->isp_tune_ops_init(&isp_data);

		return 0;
	}
fail:
	/* sensor not existed */
	isp_data.sensor->reset(POWER_OFF);
	isp_data.sensor->power(POWER_OFF);
	isp_data.sensor->exit();
init_fail:
	isp_data.sensor = NULL;
	return ret;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_enum_fmt;
 * Description : enumerate preview format(s) that supported by isp;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_enum_fmt(struct v4l2_fmtdesc *fmt, camera_state state)
{
	print_debug("enter %s", __func__);

	if (fmt->index >= isp_data.pixfmt_count
	    || 0 == isp_data.support_pixfmt[fmt->index]) {
		print_info(" %s invalided parameters, index=%d, max_index=%d",
			   __func__, fmt->index, isp_data.pixfmt_count);
		return -EINVAL;
	}

	fmt->pixelformat = isp_data.support_pixfmt[fmt->index];
	return 0;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_enum_framesizes;
 * Description : Enumerate all frame size(s) that supported by camera sensor;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_enum_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	print_debug("enter %s()", __func__);

	if (isp_data.sensor->enum_framesizes)
		return isp_data.sensor->enum_framesizes(framesizes);

	return 0;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_enum_frameintervals;
 * Description : Enumerate all frame rate(s) that supported by camera sensor;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_enum_frameintervals(struct v4l2_frmivalenum *fi)
{
	print_debug("enter %s()", __func__);
	if (isp_data.sensor->enum_frame_intervals)
		return isp_data.sensor->enum_frame_intervals(fi);
	return 0;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_try_frameintervals;
 * Description : Test whether the given frame rate is supported by camera sensor;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_try_frameintervals(struct v4l2_frmivalenum *fi)
{
	print_debug("enter %s()", __func__);

	if (isp_data.sensor->try_frame_intervals)
		return isp_data.sensor->try_frame_intervals(fi);

	return 0;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_try_fmt;
 * Description : Test whether the given format is supported by isp and
 *               camera sensor;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_try_fmt(struct v4l2_format *fmt, camera_state state, camera_setting_view_type view_type)
{
	int i = 0;
	int ret = 0;
	u8 frame_index;
	struct v4l2_frmsizeenum fs;
	struct v4l2_frmsize_discrete sensor_frmsize;
	memset(&fs, 0x00, sizeof(fs));
	memset(&sensor_frmsize, 0x00, sizeof(sensor_frmsize));

	/* check format */
	print_debug("%s, pixfmt_count[%d], pixelformat[%d], NV12[%d],NV21[%d], YUYV[%d]",
		__func__, isp_data.pixfmt_count, fmt->fmt.pix.pixelformat,
	     V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_NV21, V4L2_PIX_FMT_YUYV);
	for (i = 0; i < isp_data.pixfmt_count; ++i) {
		/* format is supported by isp and camera sensor */
		if (fmt->fmt.pix.pixelformat == isp_data.support_pixfmt[i])
			break;
	}
	if (i >= isp_data.pixfmt_count) {
		print_error("%s failed: invalid parameters", __func__);
		return -EINVAL;
	}
    ispv1_set_isp_default_reg();
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP)
	{
		if(isp_data.sensor->support_hdr_movie)
		{
			if((HDR_MOVIE_SUPPORT == isp_data.sensor->support_hdr_movie()))
			{
				if(HDR_MOVIE_ON == isp_data.sensor->get_hdr_movie_switch())
				{
					k3_isp_preview_switch_mode(HDR_MOVIE_PREVIEW_MODE);
				}
				else
				{
					k3_isp_preview_switch_mode(NON_HDR_PREVIEW_MODE);
				}
			}
		}
	}


	/* check resulotion, small than sensor's max resulotion */
	fs.type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fs.discrete.width = fmt->fmt.pix.width;
	fs.discrete.height = fmt->fmt.pix.height;
	ret = isp_data.sensor->try_framesizes(&fs);
	if (ret != 0) {
		print_error("%s:fail to try sensor framesize, width = %d, height= %d",
		     __func__, fs.discrete.width, fs.discrete.height);
		return -EINVAL;
	}

	sensor_frmsize.width = fs.discrete.width;
	sensor_frmsize.height = fs.discrete.height;
	ret = isp_data.sensor->set_framesizes(state, &sensor_frmsize, 0, view_type);
	if (ret != 0) {
		print_error("%s:fail to set sensor framesize, width = %d, height= %d",
		     __func__, fs.discrete.width, fs.discrete.height);
		return -EINVAL;
	}

	if(isp_data.sensor->try_current_hdr_framesizes)
	{
		if(HDR_MOVIE_ON == isp_data.sensor->get_hdr_movie_switch())
		{
			ret = isp_data.sensor->try_current_hdr_framesizes(&sensor_frmsize);
			if(ret != 0)
			{
				print_info("ret = %d,fs.discrete.width = %d,fs.discrete.height = %d",ret,fs.discrete.width ,fs.discrete.height);
				isp_data.sensor->set_hdr_movie_switch(HDR_MOVIE_OFF);
				k3_isp_preview_switch_mode(NON_HDR_PREVIEW_MODE);
			}

		}
	}

	if (state == STATE_PREVIEW)
		frame_index = isp_data.sensor->preview_frmsize_index;
	else
		frame_index = isp_data.sensor->capture_frmsize_index;

	isp_data.pic_attr[state].startx = isp_data.sensor->frmsize_list[frame_index].left;
	isp_data.pic_attr[state].starty = isp_data.sensor->frmsize_list[frame_index].top;


	isp_data.pic_attr[state].sensor_width = sensor_frmsize.width;
	isp_data.pic_attr[state].sensor_height = sensor_frmsize.height;

	isp_data.pic_attr[state].in_width = (sensor_frmsize.width) & (~WIDTH_ALIGN);
	isp_data.pic_attr[state].in_height = (sensor_frmsize.height) & (~HEIGHT_ALIGN);

	isp_data.pic_attr[state].in_fmt = isp_data.sensor->fmt[state];
	print_debug("==state:%d sensor_width:%d", state, isp_data.pic_attr[state].sensor_width);
	print_debug("==state:%d sensor_height:%d", state, isp_data.pic_attr[state].sensor_height);


	/* update format info */
	isp_hw_ctl->isp_fill_fmt_info(&fmt->fmt.pix);
	if (state == STATE_CAPTURE)
		memcpy(&isp_data.pic_attr[STATE_IPP], &isp_data.pic_attr[STATE_CAPTURE], sizeof(isp_data.pic_attr[STATE_IPP]));
	/* buffer size */
	fmt->fmt.pix.priv = fmt->fmt.pix.sizeimage;
	if (state == STATE_CAPTURE && ISP_CAPTURE_OFFLINE == k3_isp_get_process_mode()) {
		u32 tmp;
		u32 bitspp = bits_per_pixel(isp_data.pic_attr[state].in_fmt);
		tmp = isp_data.pic_attr[state].sensor_width * isp_data.pic_attr[state].sensor_height * bitspp / 8;
		if (tmp > fmt->fmt.pix.priv) {
			fmt->fmt.pix.priv = tmp;
		}
	}
	print_debug("buffersize exported is %#x", fmt->fmt.pix.priv);

	return 0;
}

#ifdef READ_BACK_RAW
void k3_isp_update_read_ready(u8 buf_used)
{
	isp_hw_ctl->update_read_ready(buf_used);
}
#endif

/*
 **************************************************************************
 * FunctionName: k3_isp_stream_on;
 * Description : Set isp registers according to the parmeters that have been
 *		 set before for preview.
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_stream_on(struct v4l2_pix_format *pixfmt,
		     enum v4l2_buf_type buf_type, camera_state state)
{
	struct v4l2_fmtdesc fd;
	int ret = 0;
	print_debug("enter %s", __func__);

	memset(&fd, 0x00, sizeof(fd));
	if (k3_isp_check_config(pixfmt, state)) {
		print_error("error : k3_isp_check_config failed!");
		return -EINVAL;
	}

	fd.index = 0;
	fd.type = buf_type;
	if (isp_data.sensor->get_format) {
		isp_data.sensor->get_format(&fd);
		isp_data.pic_attr[state].in_fmt = fd.pixelformat;
	}

	/* FIXME : do we need set sensor's frame intervals,
	 * preview framesize and data format here?
	 */
	if (isp_data.sensor->set_frame_intervals)
		isp_data.sensor->set_frame_intervals(&isp_data.frame_rate);

	isp_data.pic_attr[state].out_width = pixfmt->width;
	isp_data.pic_attr[state].out_height = pixfmt->height;
	isp_data.pic_attr[state].out_stride = pixfmt->width;
	isp_data.pic_attr[state].out_fmt = pixfmt->pixelformat;
	print_debug("out_width=%d, out_height=%d", pixfmt->width, pixfmt->height);

	/*update zoom*/
	k3_isp_calc_zoom(state, YUV_SCALE_FIRST, &isp_data.pic_attr[state].in_width,
			 &isp_data.pic_attr[state].in_height);

	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP)
		isp_hw_ctl->isp_tune_ops_prepare(state);

	if (isp_data.cold_boot == false) {
		if (isp_data.sensor->update_flip)
			isp_data.sensor->update_flip(isp_data.pic_attr[state].in_width, isp_data.pic_attr[state].in_height);
	}
	if (state == STATE_PREVIEW) {
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
		/* pm_qos_update_request(&isp_data.qos_request, DDR_PREVIEW_MIN_PROFILE); */
#endif
		ret = isp_hw_ctl->start_preview(&isp_data.pic_attr[state], isp_data.sensor, isp_data.cold_boot, isp_data.scene);
	} else if (state == STATE_CAPTURE) {
		/*
		camera_flashlight *flashlight = get_camera_flash();
		if ((isp_data.sensor->sensor_index == CAMERA_SENSOR_PRIMARY) && (flashlight != NULL)) {
			if (true == isp_data.flash_on) {
				flashlight->turn_on(FLASH_MODE, LUM_LEVEL4);
			}
		}
		*/
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
		/* pm_qos_update_request(&isp_data.qos_request, DDR_CAPTURE_MIN_PROFILE); */
#endif
		ret = isp_hw_ctl->start_capture(&isp_data.pic_attr[state], isp_data.sensor, isp_data.bracket_ev, isp_data.flash_on, isp_data.scene);
	} else {
		print_error("state error for streamon");
		return -EINVAL;
	}

	if (isp_data.cold_boot) {
		if (isp_data.sensor->init_reg && (0 != isp_data.sensor->init_reg())) {
			ret = -EFAULT;
		}

		if(CAMERA_ISO_AUTO != isp_data.iso)
		{
			if (isp_data.sensor->hynix_set_lsc_shading) {
				isp_data.sensor->hynix_set_lsc_shading(isp_data.iso);
			}
		}

		if (isp_data.sensor->stream_on)
			isp_data.sensor->stream_on(state);

		if (isp_hw_ctl->cold_boot_set)
			isp_hw_ctl->cold_boot_set(isp_data.sensor);

		isp_data.cold_boot = false;
	}
	else
	{
		if((state == STATE_PREVIEW)&&(isp_data.sensor->isp_location == CAMERA_USE_K3ISP))
		{
			if(isp_data.sensor->get_hdr_movie_switch)
			{
				if(HDR_MOVIE_ON == isp_data.sensor->get_hdr_movie_switch())
				{
					if (isp_data.sensor->init_isp_reg && (0 != isp_data.sensor->init_isp_reg())) {
						ret = -EFAULT;
					}
				}
			}
		}
	}

	return ret;
}

int k3_isp_stream_off(camera_state state)
{
	int ret = 0;
	camera_flashlight *flashlight = get_camera_flash();

	/* Mao FIXME */
	isp_hw_ctl->isp_set_aecagc_mode(MANUAL_AECAGC);
	if (state == STATE_PREVIEW) {
		if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP)
			isp_hw_ctl->isp_tune_ops_withdraw(state);
		ret = isp_hw_ctl->stop_preview();
		if (true == isp_data.flash_on && flashlight)
			flashlight->turn_off();
	} else {
		ret = isp_hw_ctl->stop_capture();
		if ((isp_data.sensor->sensor_index == CAMERA_SENSOR_PRIMARY) && (flashlight != NULL)) {
			if (true == isp_data.flash_on && flashlight) {
				flashlight->turn_off();
				isp_data.flash_on = false;
				isp_data.already_turn_on_flash = false;
			}
		}
	}

	return ret;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_start_process;
 * Description :
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_start_process(struct v4l2_pix_format *pixfmt, ipp_mode mode)
{
	print_debug("enter %s", __func__);
#if 0
	if (k3_isp_check_config(pixfmt)) {
		print_error("check_config fail");
		return -EINVAL;
	}

	isp_data.out_width = pixfmt->width;
	isp_data.out_height = pixfmt->height;
	isp_data.out_stride = pixfmt->bytesperline;
	isp_data.capture_fmt = pixfmt->pixelformat;

	/* update zoom */
	k3_isp_calc_zoom(YUV_SCALE_FIRST, &isp_data.capture_in_width,
			 &isp_data.capture_in_height);
#endif
	return isp_hw_ctl->start_process(&isp_data.pic_attr[STATE_CAPTURE], mode);
}

/*
 **************************************************************************
 * FunctionName: k3_isp_start_process;
 * Description :
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_stop_process(ipp_mode mode)
{
	print_debug("enter %s", __func__);

	return isp_hw_ctl->stop_process(mode);
}

static void k3_isp_turn_on_flash(struct work_struct *work)
{
	camera_flashlight *flashlight = get_camera_flash();
	
	if(flashlight){
		flashlight->turn_on(FLASH_MODE, LUM_LEVEL4);
	}
}

/*
 **************************************************************************
 * FunctionName: k3_isp_init;
 * Description : Init isp;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_init(struct platform_device *pdev, data_queue_t *data_queue)
{
	int ret = 0;
	print_debug("enter %s", __func__);

	/* init isp_data struct */
	k3_isp_set_default();
	INIT_DELAYED_WORK(&(isp_data.turn_on_flash_work),k3_isp_turn_on_flash);

#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
	pm_qos_add_request(&g_specialpolicy, PM_QOS_IPPS_POLICY, PM_QOS_IPPS_POLICY_DEFAULT_VALUE);
	pm_qos_add_request(&isp_data.qos_request, PM_QOS_DDR_MIN_PROFILE, DDR_PREVIEW_MIN_PROFILE);
	pm_qos_add_request(&isp_data.qos_request_gpu, PM_QOS_GPU_PROFILE_BLOCK, GPU_INIT_BLOCK_PROFILE);
#endif
	/* init registers */
	isp_hw_ctl = get_isp_hw_controller();

	ret = isp_hw_ctl->isp_hw_init(pdev, data_queue);
	if (0 != ret) {
		print_error("%s isp_hw_init failed", __func__);
		goto fail;
	}
	/*int csi/clk */
	ret = k3_ispio_init(pdev);
	if (0 != ret) {
		print_error("%s k3_ispio_init failed ", __func__);
		goto fail;
	}

	ret = k3_isp_poweron();
	if (0 != ret)
		print_error("%s k3_isp_poweron failed ", __func__);

fail:
	return ret;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_exit;
 * Description : Deinit isp;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_exit(void *par)
{
	print_debug("enter %s", __func__);
	if (NULL != isp_data.sensor && isp_data.sensor->isp_location == CAMERA_USE_K3ISP)
		if (isp_hw_ctl->isp_tune_ops_exit)
			isp_hw_ctl->isp_tune_ops_exit();
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
	pm_qos_update_request(&g_specialpolicy, PM_QOS_IPPS_POLICY_DEFAULT_VALUE);
#endif
	if (isp_data.sensor) {
		isp_data.sensor->reset(POWER_OFF);
		isp_data.sensor->power(POWER_OFF);
		isp_data.sensor->exit();
		isp_data.sensor = NULL;
	}
	k3_isp_poweroff();
	k3_ispio_deinit();
	isp_hw_ctl->isp_hw_deinit();

#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
	pm_qos_remove_request(&isp_data.qos_request);
	pm_qos_remove_request(&g_specialpolicy);
	pm_qos_remove_request(&isp_data.qos_request_gpu);
#endif
	return 0;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_poweron;
 * Description : isp device poweroff;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_poweron(void)
{
	print_debug("enter %s", __func__);
	if (isp_data.powered) {
		print_warn("%s, isp aready poweron", __func__);
		return 0;
	}
	isp_data.cold_boot = true;
	isp_data.powered = true;

	return isp_hw_ctl->isp_poweron();
}

/*
 **************************************************************************
 * FunctionName: k3_isp_poweroff;
 * Description : isp device poweroff;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void k3_isp_poweroff(void)
{
	print_debug("enter %s", __func__);
	if (isp_data.powered == false) {
		print_warn("%s, isp is not poweron", __func__);
		return;
	}
	isp_data.powered = false;

	return isp_hw_ctl->isp_poweroff();
}

void k3_isp_auto_focus(int flag)
{
	print_debug("enter %s", __func__);
	

	if (isp_data.sensor->af_enable) {
		camera_tune_ops->isp_auto_focus(flag);
	}
}

int k3_isp_set_focus_mode(camera_focus mode)
{
	print_debug("enter %s", __func__);

	if (isp_data.sensor->af_enable) {
		if (-1 == camera_tune_ops->isp_set_focus_mode(mode))
			return -EINVAL;
		isp_data.focus = mode;
	}
	return 0;
}

int k3_isp_get_focus_mode(void)
{
	print_debug("enter %s", __func__);
	return isp_data.focus;
}

int k3_isp_set_focus_area(focus_area_s *area)
{
	print_debug("enter %s", __func__);
	if (isp_data.sensor->af_enable) {
		if (-1 == camera_tune_ops->isp_set_focus_area(area, isp_data.zoom))
			return -EINVAL;
	}
	return 0;
}

void k3_isp_get_focus_result(focus_result_s *result)
{
	if (isp_data.sensor->af_enable) {
		camera_tune_ops->isp_get_focus_result(result);
	} else {
		result->status = STATUS_FOCUSED;
		result->each_status[0] = STATUS_FOCUSED;
	}
}

/* For bracket information settings */
int k3_isp_set_bracket_info(int *ev)
{
	print_debug("enter %s", __func__);
	if (ev == NULL)
		return -EINVAL;
	memcpy(&isp_data.bracket_ev, ev, sizeof(isp_data.bracket_ev));
	return 0;
}

/* for anti-shaking */
int k3_isp_set_anti_shaking(camera_anti_shaking flag)
{
	int ret = 0;

	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) {
		ret = camera_tune_ops->isp_set_anti_shaking(flag);
		if (!ret)
			isp_data.anti_shaking_enable = flag;
	} else {
		isp_data.anti_shaking_enable = flag;
	}

	return ret;
}

int k3_isp_get_anti_shaking(void)
{
	print_debug("enter %s", __func__);
	return isp_data.anti_shaking_enable;
}

int k3_isp_set_anti_shaking_block(int block)
{
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) {
		return camera_tune_ops->isp_set_anti_shaking_block(block);
	} else {
		return 0;
	}
}

int k3_isp_get_anti_shaking_coordinate(coordinate_s *coordinate)
{
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) {
		return camera_tune_ops->isp_get_anti_shaking_coordinate(coordinate);
	} else {
		return 0;
	}
}

/* for awb */
int k3_isp_set_awb_mode(camera_white_balance awb_mode)
{
	int ret = 0;

	print_debug("enter %s", __func__);

	if (NULL != camera_tune_ops)
		if (camera_tune_ops->set_awb)
			ret = camera_tune_ops->set_awb(awb_mode);
	if (!ret)
		isp_data.awb_mode = awb_mode;

	return ret;
}

int k3_isp_get_awb_mode(void)
{
	print_debug("enter %s", __func__);
	return isp_data.awb_mode;
}

/* for awb */
int k3_isp_set_awb_lock( int awb_lock)
{
	int ret = 0;

	print_debug("enter %s", __func__);

	if (NULL != isp_hw_ctl)
		if (isp_hw_ctl->isp_set_awb_mode)
			isp_hw_ctl->isp_set_awb_mode(awb_lock);

		isp_data.awb_lock = awb_lock;

	return ret;
}

/* < zhoutian 00195335 12-7-16 added for auto scene detect begin */
int k3_isp_get_extra_coff(extra_coff *extra_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_get_extra_coff(extra_data);

	return ret;
}


int k3_isp_get_ae_coff(ae_coff *ae_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_get_ae_coff(ae_data);

	return ret;
}

int k3_isp_set_ae_coff(ae_coff *ae_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_set_ae_coff(ae_data);

	return ret;
}


int k3_isp_get_awb_coff(awb_coff *awb_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_get_awb_coff(awb_data);

	return ret;
}

int k3_isp_set_awb_coff(awb_coff *awb_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_set_awb_coff(awb_data);

	return ret;
}


int k3_isp_get_awb_ct_coff(awb_ct_coff *awb_ct_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_get_awb_ct_coff(awb_ct_data);

	return ret;
}

int k3_isp_set_awb_ct_coff(awb_ct_coff *awb_ct_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_set_awb_ct_coff(awb_ct_data);

	return ret;
}


int k3_isp_get_ccm_coff(ccm_coff *ccm_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_get_ccm_coff(ccm_data);

	return ret;
}

int k3_isp_set_ccm_coff(ccm_coff *ccm_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_set_ccm_coff(ccm_data);

	return ret;
}


int k3_isp_set_added_coff(added_coff *added_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_set_added_coff(added_data);

	return ret;
}

int k3_isp_set_focus_range(camera_focus focus_range)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_set_focus_range(focus_range);

	return ret;	
}


int k3_isp_set_fps_range(camera_frame_rate_mode mode)
{
	int ret = 1;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		camera_tune_ops->isp_set_fps_range(mode);

	return ret;	
}


int k3_isp_set_max_exposure(camera_max_exposrure mode)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_set_max_exposure(mode);

	return ret;
}


int k3_isp_get_coff_seq(seq_coffs *seq_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_get_coff_seq(seq_data);

	return ret;
}

int k3_isp_set_coff_seq(seq_coffs *seq_data)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_set_coff_seq(seq_data);

	return ret;
}


int k3_isp_set_max_expo_time(int time)
{
	int ret = 0;
	
	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) 
		ret = camera_tune_ops->isp_set_max_expo_time(time);

	return ret;
}

/* zhoutian 00195335 12-7-16 added for auto scene detect end > */
/* for iso */
int k3_isp_get_iso(void)
{
	print_debug("enter %s", __func__);
	return isp_data.iso;
}

int k3_isp_set_iso(camera_iso iso)
{
	int ret = 0;

	print_debug("enter %s", __func__);

	if (NULL != camera_tune_ops)
		if (camera_tune_ops->set_iso)
			ret = camera_tune_ops->set_iso(iso);
	if (!ret)
		isp_data.iso = iso;
	return ret;
}

/* for ev */
int k3_isp_set_ev(int ev)
{
	int ret = 0;

	print_debug("enter %s", __func__);

	if (NULL != camera_tune_ops)
		if (camera_tune_ops->set_ev)
			ret = camera_tune_ops->set_ev(ev);
	if (!ret)
		isp_data.ev = ev;
	return ret;
}

int k3_isp_get_ev(void)
{
	print_debug("enter %s", __func__);
	return isp_data.ev;
}

/* For metering area information settings */
int k3_isp_set_metering_area(metering_area_s *area)
{
	print_debug("enter %s", __func__);
	int ret = 0;
	if (NULL != camera_tune_ops)
		if (camera_tune_ops->set_metering_area) {
			ret |= camera_tune_ops->set_metering_area(area, isp_data.zoom);
			memcpy(&isp_data.ae_area, area, sizeof(metering_area_s));
		}
	return ret;
}

/* for metering mode */
int k3_isp_set_metering_mode(camera_metering metering)
{
	int ret = 0;

	print_debug("enter %s", __func__);
	if (NULL != camera_tune_ops)
		if (camera_tune_ops->set_metering_mode)
			ret = camera_tune_ops->set_metering_mode(metering);
	if (!ret)
		isp_data.metering = metering;

	return ret;
}

int k3_isp_get_metering_mode(void)
{
	print_debug("enter %s", __func__);
	return isp_data.metering;
}

int k3_isp_set_gsensor_stat(axis_triple *xyz)
{
	print_debug("enter %s", __func__);

	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) {
		return camera_tune_ops->set_gsensor_stat(xyz);
	} else {
		return 0;
	}
}

/* for anti-banding */
int k3_isp_set_anti_banding(camera_anti_banding banding)
{
	int ret = 0;

	print_debug("enter %s", __func__);
	if (NULL != camera_tune_ops)
		if (camera_tune_ops->set_anti_banding)
			ret = camera_tune_ops->set_anti_banding(banding);
	if (!ret)
		isp_data.anti_banding = banding;

	return ret;
}

int k3_isp_get_anti_banding(void)
{
	camera_anti_banding banding = CAMERA_ANTI_BANDING_AUTO;

	print_debug("enter %s", __func__);
	if (NULL != camera_tune_ops)
		if (camera_tune_ops->get_anti_banding)
			banding = camera_tune_ops->get_anti_banding();
	if (CAMERA_ANTI_BANDING_AUTO < banding)
		isp_data.anti_banding = banding;

	return isp_data.anti_banding;
}

/* for sharpness */
int k3_isp_set_sharpness(camera_sharpness sharpness)
{
	int ret = 0;

	print_debug("enter %s", __func__);
	if (NULL != camera_tune_ops)
		if (camera_tune_ops->set_sharpness)
			ret = camera_tune_ops->set_sharpness(sharpness);
	if (!ret)
		isp_data.sharpness = sharpness;
	return ret;
}

int k3_isp_get_sharpness(void)
{
	print_debug("enter %s", __func__);
	return isp_data.sharpness;
}

/* for saturation */
int k3_isp_set_saturation(camera_saturation saturation)
{
	int ret = 0;

	print_debug("enter %s", __func__);

	if (NULL != camera_tune_ops)
		if (camera_tune_ops->set_saturation)
			ret = camera_tune_ops->set_saturation(saturation);
	if (!ret)
			isp_data.saturation = saturation;

	return ret;
}

int k3_isp_get_saturation(void)
{
	print_debug("enter %s", __func__);
	return isp_data.saturation;
}

/* for contrast */
int k3_isp_set_contrast(camera_contrast contrast)
{
	int ret = 0;

	print_debug("enter %s", __func__);

	if (NULL != camera_tune_ops)
		if (camera_tune_ops->set_contrast)
			ret = camera_tune_ops->set_contrast(contrast);
	if (!ret)
		isp_data.contrast = contrast;
	return ret;
}

int k3_isp_get_contrast(void)
{
	print_debug("enter %s", __func__);
	return isp_data.contrast;
}

/* for scene */
int k3_isp_set_scene(camera_scene scene)
{
	int ret = 0;

	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP) {
		ret = camera_tune_ops->set_scene(scene);
		if (!ret)
			isp_data.scene = scene;
	} else {
		isp_data.scene = scene;
	}

	return ret;
}

int k3_isp_get_scene(void)
{
	print_debug("enter %s", __func__);
	return isp_data.scene;
}

/* for brightness */
int k3_isp_set_brightness(camera_brightness brightness)
{
	int ret = 0;

	print_debug("enter %s", __func__);

	if (NULL != camera_tune_ops)
		if (camera_tune_ops->set_brightness)
			ret = camera_tune_ops->set_brightness(brightness);
	if (!ret)
		isp_data.brightness = brightness;
	return ret;
}

int k3_isp_get_brightness(void)
{
	print_debug("enter %s", __func__);
	return isp_data.brightness;
}

/* for effect */
int k3_isp_set_effect(camera_effects effect)
{
	int ret = 0;

	print_debug("enter %s", __func__);
	if (NULL != camera_tune_ops)
		if (camera_tune_ops->set_effect)
			ret = camera_tune_ops->set_effect(effect);
	if (!ret)
		isp_data.effect = effect;

	return ret;
}

/* < zhoutian 00195335 2012-10-20 added for hwscope begin */
int k3_isp_set_hwscope(hwscope_coff *hwscope_data)
{
	int ret = 0;

	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP)
	{
		ret = camera_tune_ops->isp_set_hwscope(hwscope_data);
	}

	return ret;
}
/* zhoutian 00195335 2012-10-20 added for hwscope end > */

/* < zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight begin */
int k3_isp_set_hw_lowlight(int ctl)
{
	int ret = 0;

	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP)
	{
		ret = camera_tune_ops->isp_set_hw_lowlight(ctl);
	}

	return ret;
}

int k3_isp_get_hw_lowlight_support()
{
	print_debug("enter %s", __func__);

	if (isp_data.sensor->support_hw_lowlight)
	{
		return isp_data.sensor->support_hw_lowlight();
	}

	return 0;
}

int k3_isp_get_binning_size(binning_size *size)
{
	int ret = 0;

	print_debug("enter %s", __func__);
	if (isp_data.sensor->isp_location == CAMERA_USE_K3ISP)
	{
		ret = camera_tune_ops->isp_get_binning_size(size);
	}

	return ret;
}
/* zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight end > */

int k3_isp_get_effect(void)
{
	print_debug("enter %s", __func__);
	return isp_data.effect;
}

/* for flash mode */
int k3_isp_set_flash_mode(camera_flash flash_mode)
{
	camera_flashlight *flashlight = get_camera_flash();
	print_debug("enter %s", __func__);
	if (isp_data.sensor->sensor_index != CAMERA_SENSOR_PRIMARY) {
		print_error("only primary camera support flash");
		return 0;
	}

	if (flashlight == NULL) {
		print_error("failed to find flash light device");
		return -ENODEV;
	}

	switch (flash_mode) {
	case CAMERA_FLASH_TORCH:
		{
			print_info("set camera flash TORCH MODE");
			flashlight->turn_on(TORCH_MODE, LUM_LEVEL2);
			break;
		}
	case CAMERA_FLASH_OFF:
		{
			print_info("set camera flash OFF MODE");
			if (isp_data.flash_mode == CAMERA_FLASH_TORCH
				&& flashlight) {
				flashlight->turn_off();
			}

			isp_hw_ctl->isp_set_auto_flash(0, flash_mode);

			break;
		}
	case CAMERA_FLASH_AUTO:
		{
			print_info("set camera flash AUTO MODE");
			isp_hw_ctl->isp_set_auto_flash(1, flash_mode);
			break;
		}
	case CAMERA_FLASH_ON:
		{
			isp_hw_ctl->isp_set_auto_flash(0, flash_mode);
			print_info("set camera flash ON MODE");
			break;
		}
	default:
		{
			print_error("invalid parameter");
			return -EINVAL;
		}
	}

	isp_data.flash_mode = flash_mode;

	return 0;

}

int k3_isp_get_flash_mode(void)
{
	print_debug("enter %s", __func__);
	return flash_exif;
}
/*
 **************************************************************************
 * FunctionName: k3_isp_get_zoom;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : current zoom step;
 * Other       : NA;
 **************************************************************************
*/
int k3_isp_get_zoom(void)
{
	print_debug("Enter Function:%s ", __func__);
	return isp_data.zoom;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_set_zoom;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : 0:success  other:failed;
 * Other       : NA;
 **************************************************************************
*/
int k3_isp_set_zoom(char preview_running, u32 zoom)
{
	int ret = 0;

	print_debug("Enter Function:%s	zoom:%d", __func__, zoom);

	isp_data.zoom = zoom;

	if(isp_data.sensor->check_zoom_limit)
	{
		isp_data.sensor->check_zoom_limit(&zoom);
	}
	if (preview_running) {
		ret |= isp_hw_ctl->isp_set_zoom(zoom, HIGH_QUALITY_MODE);
		if (isp_data.sensor->af_enable)
			ret |= camera_tune_ops->isp_set_focus_zoom(zoom);
		if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
			ret |= camera_tune_ops->set_metering_area(&isp_data.ae_area, zoom);
			ret |= camera_tune_ops->isp_set_sharpness_zoom(zoom);
		}
	}

#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
	if (0 == ret) {
		if (zoom > 10) { // total is 30 steps
			pm_qos_update_request(&isp_data.qos_request_gpu, GPU_BLOCK_PROFILE);
		} else {
			pm_qos_update_request(&isp_data.qos_request_gpu, GPU_INIT_BLOCK_PROFILE);
		}
	}
#endif
	return ret;
}

int k3_isp_get_exposure_time(void)
{
	int ret = 0;
	print_debug("enter %s", __func__);

	if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
		ret = camera_tune_ops->isp_get_exposure_time();
	} else if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		if (isp_data.sensor->get_exposure)
			ret = 1000000 / isp_data.sensor->get_exposure();
	}
	return ret;
}
int k3_isp_get_fps(camera_fps fps)
{
	print_debug("enter %s", __func__);
	return isp_hw_ctl->isp_get_fps(isp_data.sensor, fps);
}

int k3_isp_set_fps(camera_fps fps, u8 value)
{
	int ret = 0;
	print_debug("enter %s", __func__);
	ret = isp_hw_ctl->isp_set_fps(isp_data.sensor, fps, value);
	if ((CAMERA_FPS_MIN == fps) && (CAMERA_SCENE_ACTION == isp_data.scene
		|| CAMERA_SCENE_NIGHT == isp_data.scene
		|| CAMERA_SCENE_NIGHT_PORTRAIT == isp_data.scene
		|| CAMERA_SCENE_THEATRE == isp_data.scene
		|| CAMERA_SCENE_FIREWORKS == isp_data.scene
		|| CAMERA_SCENE_CANDLELIGHT == isp_data.scene)) {
		ret |= k3_isp_set_scene(isp_data.scene);
	}

	return ret;
}

int k3_isp_get_actual_iso(void)
{
	int ret = 0;
	print_debug("enter %s", __func__);

	if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
		ret = camera_tune_ops->isp_get_actual_iso();
	} else if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		if (isp_data.sensor->get_gain) {
			ret = isp_data.sensor->get_gain() * 100 / 0x10;
			ret = ((ret / 2) + 5) / 10 * 10;
		}
	}
	return ret;
}

int k3_isp_get_hdr_iso_exp(hdr_para_reserved *iso_exp)
{

	int ret = 0;
	print_debug("enter %s", __func__);

	if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
		ret = camera_tune_ops->isp_get_hdr_iso_exp(iso_exp);
	}

	return ret;
}
int k3_isp_get_focus_distance(void)
{
	print_debug("enter %s", __func__);
	return camera_tune_ops->isp_get_focus_distance();
}
int k3_isp_get_awb_gain(int withShift)
{
	int ret = 0;

	if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		ret = 0;
	} else if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
		ret = camera_tune_ops->isp_get_awb_gain(withShift);
	}
	return ret;
}


int k3_isp_get_focus_code(void)
{
	int ret = 0;

	if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		ret = 0;
	} else if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
		ret = camera_tune_ops->isp_get_focus_code();
	}
	return ret;
}

int k3_isp_get_focus_rect(camera_rect_s *rect)
{
	int ret = 0;

	if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		ret = 0;
	} else if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
		ret = camera_tune_ops->isp_get_focus_rect(rect);
	}
	return ret;
}

int k3_isp_get_expo_line(void)
{
	int ret = 0;

	if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		ret = 0;
	} else if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
		ret = camera_tune_ops->isp_get_expo_line();
	}
	return ret;
}

int k3_isp_get_sensor_vts(void)
{
	int ret = 0;

	if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		ret = 0;
	} else if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
		ret = camera_tune_ops->isp_get_sensor_vts();
	}
	return ret;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_get_sensor_lux_matrix;
 * Description : HAL get lux matrix vlaue though this interface ;
 * Input       : the pointer of lux_stat_matrix_tbl used for saving lux matrix from sensor;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       :
 **************************************************************************
 */

int   k3_isp_get_sensor_lux_matrix(lux_stat_matrix_tbl * lux_matrix)
{
	int ret = 0;
	if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		ret = 0;
	} else if ((CAMERA_USE_K3ISP == isp_data.sensor->isp_location)&& isp_data.hdr_switch_on) {
		ret = camera_tune_ops->isp_get_sensor_lux_matrix(lux_matrix);
	}

	return ret;
}
/*
 **************************************************************************
 * FunctionName: k3_isp_get_sensor_hdr_points;
 * Description : HAL get atr control points though this interface ;
 * Input       : the pointer of atr_ctrl_points used for saving ctrl point;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       :
 **************************************************************************
 */
int k3_isp_get_sensor_hdr_points(atr_ctrl_points * hdrPoints)
{
	int ret = 0;
	if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		ret = 0;
	} else if ((CAMERA_USE_K3ISP == isp_data.sensor->isp_location)&&isp_data.hdr_switch_on) {
		ret = camera_tune_ops->isp_get_sensor_hdr_points(hdrPoints);
	}

	return ret;
}
/*
 **************************************************************************
 * FunctionName: k3_isp_get_sensor_hdr_info;
 * Description : HAL get hdr info though this interface include hdr switch,atr switch long gain ,short gain,
 *			   : long shutter ,short shutter ,avg lux
 * Input       : the pointer of hdr_info used for saving hdr info;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       :
 **************************************************************************
 */
int k3_isp_get_sensor_hdr_info(hdr_info * hdrInfo)
{
	int ret = 0;
	if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		ret = 0;
	} else if ((CAMERA_USE_K3ISP == isp_data.sensor->isp_location)&&isp_data.hdr_switch_on) {
		ret = camera_tune_ops->isp_get_sensor_hdr_info(hdrInfo);
	}

	return ret;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_get_sensor_preview_max_size;
 * Description : HAL get  max preview size for  stoping switter
 * Input       : the pointer of preview_size used for saving preview_size from sensor;
 * Output      : the value of preview_size used for saving preview_size from sensor;;
 * ReturnValue : NA;
 * Other       :
 **************************************************************************
 */

int   k3_isp_get_sensor_preview_max_size(preview_size * pre_size)
{

	int ret = 0;
	if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		ret = 0;
	} else if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
		if (isp_data.sensor->get_sensor_preview_max_size) {
			ret = isp_data.sensor->get_sensor_preview_max_size(pre_size);
		}
		else
		{
			pre_size->width = -1;
			pre_size->height = -1;
		}
	}
	return ret;
}

int k3_isp_get_sensor_aperture(void)
{
	if (isp_data.sensor->get_sensor_aperture) {
		return isp_data.sensor->get_sensor_aperture();
	}

	return 0;
}

int k3_isp_get_equivalent_focus(void)
{
	if (isp_data.sensor->get_equivalent_focus) {
		return isp_data.sensor->get_equivalent_focus();
	}

	return 0;
}

int k3_isp_get_hdr_movie_support(void)
{
	if (isp_data.sensor->support_hdr_movie) {
		return isp_data.sensor->support_hdr_movie();
	}

	return 0;
}


int k3_isp_get_current_ccm_rgain(void)
{
	int ret = 0;

	if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		ret = 0;
	} else if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
		ret = camera_tune_ops->isp_get_current_ccm_rgain();
	}
	return ret;
}

int k3_isp_get_current_ccm_bgain(void)
{
	int ret = 0;

	if (CAMERA_USE_SENSORISP == isp_data.sensor->isp_location) {
		ret = 0;
	} else if (CAMERA_USE_K3ISP == isp_data.sensor->isp_location) {
		ret = camera_tune_ops->isp_get_current_ccm_bgain();
	}
	return ret;
}

/*
 * YUV windows to RAW windows(after RAW DCW)
 * this function is useful for anti_shaking, aec/agc metering and focus
 * Here we just consider aec/agc metering and focus, focus configure as after RAW DCW.
 */

int k3_isp_yuvrect_to_rawrect(camera_rect_s *yuv, camera_rect_s *raw)
{
	pic_attr_t *attr = &isp_data.pic_attr[STATE_PREVIEW];

	/*scale up */
	if (attr->yuv_up_scale_nscale < YUV_SCALE_DIVIDEND) {
		raw->left = yuv->left * attr->yuv_up_scale_nscale / YUV_SCALE_DIVIDEND  + attr->crop_x;
		raw->top = yuv->top * attr->yuv_up_scale_nscale / YUV_SCALE_DIVIDEND  + attr->crop_y;
		raw->width = yuv->width * attr->yuv_up_scale_nscale / YUV_SCALE_DIVIDEND;
		raw->height = yuv->height * attr->yuv_up_scale_nscale / YUV_SCALE_DIVIDEND;
	} else {
		raw->left = yuv->left * attr->yuv_dcw * attr->yuv_down_scale_nscale / YUV_SCALE_DIVIDEND + attr->crop_x;
		raw->top = yuv->top * attr->yuv_dcw * attr->yuv_down_scale_nscale / YUV_SCALE_DIVIDEND + attr->crop_y;
		raw->width = yuv->width * attr->yuv_dcw * attr->yuv_down_scale_nscale / YUV_SCALE_DIVIDEND;
		raw->height = yuv->height * attr->yuv_dcw * attr->yuv_down_scale_nscale / YUV_SCALE_DIVIDEND;
	}

	return 0;
}

int k3_isp_rawrect_to_yuvrect(camera_rect_s *yuv, camera_rect_s *raw)
{
	pic_attr_t *attr = &isp_data.pic_attr[STATE_PREVIEW];

	if ((raw->left < attr->crop_x) || (raw->top < attr->crop_y)) {
		print_error("raw rect in the croped area");
		return -1;
	}

	/*scale up, fixme: */
	if (attr->yuv_up_scale_nscale < YUV_SCALE_DIVIDEND) {
		yuv->left = (raw->left - attr->crop_x) * YUV_SCALE_DIVIDEND / attr->yuv_up_scale_nscale;
		yuv->top = (raw->top - attr->crop_y) * YUV_SCALE_DIVIDEND / attr->yuv_up_scale_nscale;
		yuv->width = raw->width * YUV_SCALE_DIVIDEND / attr->yuv_up_scale_nscale;
		yuv->height = raw->height * YUV_SCALE_DIVIDEND / attr->yuv_up_scale_nscale;
	} else {
		yuv->left = (raw->left - attr->crop_x) * YUV_SCALE_DIVIDEND / (attr->yuv_dcw * attr->yuv_down_scale_nscale);
		yuv->top = (raw->top - attr->crop_y) * YUV_SCALE_DIVIDEND / (attr->yuv_dcw * attr->yuv_down_scale_nscale);
		yuv->width = raw->width * YUV_SCALE_DIVIDEND / (attr->yuv_dcw * attr->yuv_down_scale_nscale);
		yuv->height =  raw->height * YUV_SCALE_DIVIDEND / (attr->yuv_dcw * attr->yuv_down_scale_nscale);
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_antishaking_rect_out2stat;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_isp_antishaking_rect_out2stat(camera_rect_s *out, camera_rect_s *stat)
{
	pic_attr_t *attr = &isp_data.pic_attr[STATE_PREVIEW];
	print_debug("enter %s", __func__);

	/*scale up */
	if (attr->yuv_up_scale_nscale < YUV_SCALE_DIVIDEND) {
		/*x and y are both 0 */
		stat->left = out->left * attr->yuv_up_scale_nscale / YUV_SCALE_DIVIDEND;
		stat->top = out->top * attr->yuv_up_scale_nscale / YUV_SCALE_DIVIDEND;
		stat->width = out->width * attr->yuv_up_scale_nscale / YUV_SCALE_DIVIDEND;
		stat->height = out->height * attr->yuv_up_scale_nscale / YUV_SCALE_DIVIDEND;
	} else {
		stat->left = out->left;
		stat->top = out->top;
		stat->width = out->width;
		stat->height = out->height;
	}

	return 0;
}


int k3_isp_antishaking_rect_stat2out(camera_rect_s *out, camera_rect_s *stat)
{
	pic_attr_t *attr = &isp_data.pic_attr[STATE_PREVIEW];
	print_debug("enter %s", __func__);

	/*scale up */
	if (attr->yuv_up_scale_nscale < YUV_SCALE_DIVIDEND) {
		/*width and height are both 0 */
		out->left = stat->left * YUV_SCALE_DIVIDEND  / attr->yuv_up_scale_nscale;
		out->top = stat->top * YUV_SCALE_DIVIDEND  / attr->yuv_up_scale_nscale;
		out->width = stat->width * YUV_SCALE_DIVIDEND / attr->yuv_up_scale_nscale;
		out->height = stat->height * YUV_SCALE_DIVIDEND / attr->yuv_up_scale_nscale;
	} else {
		out->left = stat->left;
		out->top = stat->top;
		out->width = stat->width;
		out->height = stat->height;
	}
	return 0;
}

/* AndroidK3 added by y36721 end */

/*
 **************************************************************************
 * FunctionName: k3_isp_calc_zoom;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void k3_isp_calc_zoom(camera_state state, scale_strategy_t scale_strategy, u32 *in_width, u32 *in_height)
{
	u32 scale_width;
	u32 scale_height;
	u32 temp_width;
	u32 temp_height;
	u32 scale;
	u32 ratio;
	u32 temp;
	pic_attr_t *attr = &isp_data.pic_attr[state];

	print_debug("%s, zoom=%d, in_width=%d, in_height=%d",
		    __func__, isp_data.zoom, *in_width, *in_height);

	if ((0 != (*in_width & WIDTH_ALIGN)) || (0 != (*in_height & HEIGHT_ALIGN))) {
		print_error("width %d is not 16x or height %d is not 4x", *in_width, *in_height);
		return;
	}

	if ((*in_height) * (attr->out_width) > (*in_width) * (attr->out_height)) {
		/*do raw height crop*/
		temp_height = (*in_width) * (attr->out_height) / (attr->out_width);
		temp_width = temp_height * (attr->out_width) / (attr->out_height);
	} else {
		/*do raw width crop */
		temp_width = (*in_height) * (attr->out_width) / (attr->out_height);
		temp_height = temp_width * (attr->out_height) / (attr->out_width);
	}

	/* do not support float compute, so multiple YUV_SCALE_DIVIDEND */
	scale_width = (YUV_SCALE_DIVIDEND * temp_width) / attr->out_width;
	scale_height = (YUV_SCALE_DIVIDEND * temp_height) / attr->out_height;
	scale = (scale_width < scale_height) ? scale_width : scale_height;
	print_debug("scale_width = %d, scale_height = %d, scale = %d",
		    scale_width, scale_height, scale);

	/* we do not need raw scale down */
	attr->raw_scale_down = 1;
	attr->yuv_in_width = *in_width;
	attr->yuv_in_height = *in_height;

	temp = isp_zoom_to_ratio(isp_data.zoom, isp_data.video_stab);

	/* if scale down ratio < zoom in ratio ,
	   we need crop and yuv scale up */
	if (scale * ZOOM_BASE <= YUV_SCALE_DIVIDEND * temp) {
		/* first we get zoom in ratio */
		attr->yuv_up_scale_nscale = (scale * ZOOM_BASE) / temp;
		print_debug("yuv_up_scale_nscale = %d", attr->yuv_up_scale_nscale);
		/* we do not need yuv scale down*/
		attr->yuv_dcw = 1;
		attr->yuv_down_scale_nscale = YUV_SCALE_DIVIDEND;
		print_debug("raw_scale_down = %d, yuv_down_scale_nscale = %d",
			    attr->raw_scale_down, attr->yuv_down_scale_nscale);
	} else {
		/* we need scale down, and we get scale down ratio */
		ratio = (scale * ZOOM_BASE) / temp;
		print_debug("scale down ratio is = %d", ratio);

		if (ratio >= (2 * YUV_SCALE_DIVIDEND)) {
			/* we need YUV_DCW_Fliter */
			print_debug("we need YUV_DCW_Fliter");
			if (ratio >= (8 * YUV_SCALE_DIVIDEND))
				attr->yuv_dcw = 8;	/*distortion */
			else if (ratio >= (4 * YUV_SCALE_DIVIDEND))
				attr->yuv_dcw = 4;
			else
				attr->yuv_dcw = 2;
		} else {
			attr->yuv_dcw = 1;
		}

		/* we need YUVDownScaleFliter */
		attr->yuv_down_scale_nscale = ratio / attr->yuv_dcw;
		attr->yuv_up_scale_nscale = YUV_SCALE_DIVIDEND;
		print_debug("yuv_dcw=%d, yuv_down_scale_nscale = %d, yuv_up_scale_nscale=%d",
			attr->yuv_dcw, attr->yuv_down_scale_nscale,
			attr->yuv_up_scale_nscale);
	}

	attr->crop_width = temp_width * ZOOM_BASE / temp;
	attr->crop_height = temp_height * ZOOM_BASE / temp;

	if ((attr->crop_width * temp) < (temp_width * ZOOM_BASE))
		attr->crop_width++;
	if ((attr->crop_height * temp) < (temp_height * ZOOM_BASE))
		attr->crop_height++;

	attr->crop_x = (attr->yuv_in_width - attr->crop_width) / 2;
	attr->crop_y = (attr->yuv_in_height - attr->crop_height) / 2;

	print_debug("crop_width = %d, crop_height = %d, crop_x = %d, crop_y = %d",
	     attr->crop_width, attr->crop_height, attr->crop_x, attr->crop_y);
}

int k3_isp_set_hflip(int flip)
{
	print_debug("enter %s", __func__);

	if (isp_data.sensor->set_hflip)
		return isp_data.sensor->set_hflip(flip);

	return 0;
}
int k3_isp_get_hflip(void)
{
	print_debug("enter %s()", __func__);

	if (isp_data.sensor->get_hflip)
		return isp_data.sensor->get_hflip();
	return 0;
}
int k3_isp_set_vflip(int flip)
{
	print_debug("enter %s", __func__);

	if (isp_data.sensor->set_vflip)
		return isp_data.sensor->set_vflip(flip);
	return 0;
}
int k3_isp_get_vflip(void)
{
	print_debug("enter %s()", __func__);

	if (isp_data.sensor->get_vflip)
		return isp_data.sensor->get_vflip();
	return 0;
}


void k3_isp_set_shoot_mode(camera_shoot_mode shoot_mode)
{
	print_info("enter %s, shot mode is %d", __func__, shoot_mode);
	isp_data.shoot_mode = shoot_mode;
}

void k3_isp_set_pm_mode(u8 pm_mode)
{
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
	pm_qos_update_request(&g_specialpolicy, pm_mode);
#endif
}

int k3_isp_get_current_vts(void)
{
	print_debug("enter %s()", __func__);
	return isp_hw_ctl->isp_get_current_vts(isp_data.sensor);
}

int k3_isp_get_current_fps(void)
{
	print_debug("enter %s()", __func__);
	return isp_hw_ctl->isp_get_current_fps(isp_data.sensor);
}
int k3_isp_get_band_threshold(void)
{
	print_debug("enter %s()", __func__);
	return isp_hw_ctl->isp_get_band_threshold(isp_data.sensor, isp_data.anti_banding);
}

void k3_isp_set_fps_lock(int lock)
{
	print_debug("enter %s()", __func__);
	camera_tune_ops->isp_set_fps_lock(lock);
}
/*
 **************************************************************************
 * FunctionName: k3_isp_switch_hdr_movie;
 * Description : set hdr movie switch option
 * Input       : 1 on ,0 off;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       :
 **************************************************************************
 */
void k3_isp_switch_hdr_movie(u8 on)
{
	if (isp_data.sensor->set_hdr_movie_switch)
		return isp_data.sensor->set_hdr_movie_switch(on);
	return 0;
}

void k3_isp_set_video_stabilization(int bStabilization)
{
	print_debug("enter %s()", __func__);
	isp_data.video_stab = bStabilization;
}

void k3_isp_get_yuv_crop_rect(crop_rect_s *rect)
{
	print_debug("enter %s()", __func__);
	isp_hw_ctl->isp_get_yuv_crop_rect(rect);
}

void k3_isp_set_yuv_crop_pos(int point)
{
	print_debug("enter %s()", __func__);
	isp_hw_ctl->isp_set_yuv_crop_pos(point);
}

int k3_isp_set_scene_type(scene_type *type)
{
	print_debug("enter %s", __func__);
	int ret = 0;
	if(type == NULL)
	{
		return -1;
	}
	memcpy(&isp_data.sceneInfo, type, sizeof(scene_type));
	return ret;
}

/*
 **************************************************************************
 * FunctionName: k3_isp_set_default;
 * Description : Set default value of v4l2_callback & isp_data struct;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void k3_isp_set_default(void)
{
	camera_state state = 0;
	/* init data struct */
	memset(&isp_data.bracket_ev, 0, sizeof(isp_data.bracket_ev));
	for (state = STATE_PREVIEW; state < STATE_MAX; state++) {
		isp_data.pic_attr[state].sensor_width		= 640;
		isp_data.pic_attr[state].sensor_height		= 480;
		isp_data.pic_attr[state].in_width		= 640;
		isp_data.pic_attr[state].in_height		= 480;
		isp_data.pic_attr[state].startx			= 0;
		isp_data.pic_attr[state].starty			= 0;
		isp_data.pic_attr[state].out_width		= 640;
		isp_data.pic_attr[state].out_height		= 480;
		isp_data.pic_attr[state].raw_scale_down		= 1;
		isp_data.pic_attr[state].yuv_dcw		= 1;
		isp_data.pic_attr[state].yuv_in_width		= 640;
		isp_data.pic_attr[state].yuv_in_height		= 480;
		isp_data.pic_attr[state].yuv_down_scale_nscale	= YUV_SCALE_DIVIDEND;
		isp_data.pic_attr[state].yuv_up_scale_nscale	= YUV_SCALE_DIVIDEND;
		isp_data.pic_attr[state].crop_x		= 0; /* isp_data.pic_attr[state].startx; */
		isp_data.pic_attr[state].crop_y		= 0; /* isp_data.pic_attr[state].starty; */
		isp_data.pic_attr[state].crop_width		= isp_data.pic_attr[state].in_width;
		isp_data.pic_attr[state].crop_height		= isp_data.pic_attr[state].in_height;
		isp_data.pic_attr[state].in_fmt			= V4L2_PIX_FMT_RAW10;
		isp_data.pic_attr[state].out_fmt		= V4L2_PIX_FMT_NV12;
	}
	isp_data.video_stab = 0;
	isp_data.zoom				= 0;
	isp_data.sensor				= NULL;
	isp_data.support_pixfmt			= NULL;
	isp_data.pixfmt_count			= 0;

	isp_data.fps_mode	=	CAMERA_FRAME_RATE_AUTO;
	isp_data.assistant_af_flash = false;
	isp_data.af_need_flash = false;

	isp_data.shoot_mode = CAMERA_SHOOT_SINGLE;
	
	isp_data.already_turn_on_flash = false;
}

/************************ END **************************/
