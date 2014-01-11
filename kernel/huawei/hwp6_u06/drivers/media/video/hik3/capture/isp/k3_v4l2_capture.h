/*
 *  Hisilicon K3 soc camera v4l2 driver header file
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

#ifndef __K3_V4L2_CAPTURE_H__
#define __K3_V4L2_CAPTURE_H__

#include <linux/videodev2.h>
#include <linux/semaphore.h>
#include "cam_util.h"
#include "camera.h"
#include "sensor_common.h"

#define VIDIOC_CHECK_CAP  _IOWR('V', 200, struct v4l2_ext_controls)

/*
 * key and value pair struct
 * used for setting parameters
 */
enum {
	PKEY_IN_WIDTH = 1,	/* camera sensor input width */
	PKEY_IN_HEIGHT,		/* camera sensor input height */
	PKEY_IN_RESOLUTION,
	PKEY_START_X,		/* capture start x position */
	PKEY_START_Y,
	PKEY_START_POS,
	PKEY_OUT_WIDTH,
	PKEY_OUT_HEIGHT,
	PKEY_OUT_RESOLUTION,
	PKEY_OUT_FMT,
	PKEY_MIRROR,
	PKEY_BRIGHT,
	PKEY_CONTRAST,
	PKEY_FRAMERATE,
	PKEY_OUT_STRIDE,
};

typedef struct _keypair_struct {
	int key;
	union {
		u32 uint_value;
		int int_value;
		char ch_value;
		int *pint_value;
		char *pchar_value;
		void *pv_value;
	} value;

} keypair_struct;

struct platform_device;
/* camera data struct */
typedef struct _v4l2_ctl_struct {
	struct video_device *video_dev;
	struct platform_device *pdev;
	struct semaphore busy_lock;
	int open_count;

	char running[STATE_MAX];
	pid_t pid[STATE_MAX];

	struct v4l2_fract frame_rate;

	/* format info */
	struct v4l2_format fmt[STATE_MAX];

	/* camera frame buffer */
	buffer_arr_t buffer_arr[STATE_MAX];
	data_queue_t data_queue;

	/* camera sensors */
	camera_sensor *sensor;
	int cur_sensor;

	/* dvfs variables */
	int pm_client_id;
	int pm_profile_flag;

} v4l2_ctl_struct;

/* ISP related interface */
int k3_isp_init(struct platform_device *pdev, data_queue_t *data_queue);
int k3_isp_exit(void *);
int k3_isp_set_parameter(struct _keypair_struct *argv, int argc);
int k3_isp_hw_init_regs(camera_sensor *sensor);
int k3_isp_set_camera(camera_sensor *sensor, camera_sensor *close_sensor, sensor_known_t known_sensor);
int k3_isp_enum_fmt(struct v4l2_fmtdesc *f, camera_state state);
int k3_isp_stream_on(struct v4l2_pix_format *pixfmt, enum v4l2_buf_type buf_type, camera_state state);
#ifdef READ_BACK_RAW
void k3_isp_update_read_ready(u8 buf_used);
#endif
int k3_isp_stream_off(camera_state state);
int k3_isp_start_process(struct v4l2_pix_format *pixfmt, ipp_mode mode);
int k3_isp_stop_process(ipp_mode mode);
int k3_isp_try_fmt(struct v4l2_format *fmt, camera_state state, camera_setting_view_type view_type);
int k3_isp_enum_framesizes(struct v4l2_frmsizeenum *fs);
int k3_isp_enum_frameintervals(struct v4l2_frmivalenum *fi);
int k3_isp_try_frameintervals(struct v4l2_frmivalenum *fi);
int k3_isp_set_zoom(char preview_running, u32 zoom);
int k3_isp_get_zoom(void);
int k3_isp_get_process_mode(void);
int k3_isp_poweron(void);
void k3_isp_poweroff(void);
/*AndroidK3 added by y36721 begin*/
int k3_isp_get_common_capability(u32 id, u32 *value);
int k3_isp_get_k3_capability(u32 id, u32 *value);
int k3_isp_get_primary_capability(u32 id, u32 *value);
void k3_isp_check_flash_level(camera_flash_state state);
int k3_isp_get_flash_result(void);

/* All below functions are used for focus */
void k3_isp_auto_focus(int flag);
int k3_isp_set_focus_mode(camera_focus mode);
int k3_isp_get_focus_mode(void);
int k3_isp_set_focus_area(focus_area_s *area);
void k3_isp_get_focus_result(focus_result_s *result);

/* < zhoutian 00195335 12-7-16 added for auto scene detect begin */
int k3_isp_get_extra_coff(extra_coff *extra_data);

int k3_isp_get_ae_coff(ae_coff *ae_data);
int k3_isp_set_ae_coff(ae_coff *ae_data);

int k3_isp_get_awb_coff(awb_coff *awb_data);
int k3_isp_set_awb_coff(awb_coff *awb_data);

int k3_isp_get_awb_ct_coff(awb_ct_coff *awb_ct_data);
int k3_isp_set_awb_ct_coff(awb_ct_coff *awb_ct_data);

int k3_isp_get_ccm_coff(ccm_coff *ccm_data);
int k3_isp_set_ccm_coff(ccm_coff *ccm_data);

int k3_isp_set_added_coff(added_coff *added_data);

int k3_isp_set_focus_range(camera_focus focus_range);

int k3_isp_set_fps_range(camera_frame_rate_mode mode);

int k3_isp_set_max_exposure(camera_max_exposrure mode);

int k3_isp_get_coff_seq(seq_coffs *seq_data);
int k3_isp_set_coff_seq(seq_coffs *seq_data);

int k3_isp_set_max_expo_time(int time);
/* zhoutian 00195335 12-7-16 added for auto scene detect end > */
/* For bracket information settings */
int k3_isp_set_bracket_info(int *ev);

/* All below functions are used for anti_shaking */
int k3_isp_set_anti_shaking(camera_anti_shaking flag);
int k3_isp_get_anti_shaking(void);
int k3_isp_set_anti_shaking_block(int block);
int k3_isp_get_anti_shaking_coordinate(coordinate_s *coordinate);

int k3_isp_set_awb_mode(camera_white_balance awb_mode);
int k3_isp_get_awb_mode(void);
int k3_isp_set_awb_lock(int lock);
int k3_isp_set_iso(camera_iso iso);
int k3_isp_get_iso(void);

int k3_isp_set_ev(int ev);
int k3_isp_get_ev(void);

/* For metering area information settings */
int k3_isp_set_metering_area(metering_area_s *area);
int k3_isp_set_metering_mode(camera_metering mode);
int k3_isp_get_metering_mode(void);

int k3_isp_set_gsensor_stat(axis_triple *xyz);

int k3_isp_set_anti_banding(camera_anti_banding banding);
int k3_isp_get_anti_banding(void);

int k3_isp_set_sharpness(camera_sharpness sharpness);
int k3_isp_get_sharpness(void);

int k3_isp_set_saturation(camera_saturation saturation);
int k3_isp_get_saturation(void);

int k3_isp_set_contrast(camera_contrast contrast);
int k3_isp_get_contrast(void);

int k3_isp_set_scene(camera_scene scene);
int k3_isp_get_scene(void);

int k3_isp_set_brightness(camera_brightness brightness);
int k3_isp_get_brightness(void);

int k3_isp_set_effect(camera_effects effect);
int k3_isp_get_effect(void);

int k3_isp_get_exposure_time(void);

/* < zhoutian 00195335 2012-10-20 added for hwscope begin */
int k3_isp_set_hwscope(hwscope_coff *hwscope_data);
/* zhoutian 00195335 2012-10-20 added for hwscope end > */

/* < zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight begin */
int k3_isp_set_hw_lowlight(int ctl);
int k3_isp_get_hw_lowlight_support();
int k3_isp_get_binning_size(binning_size *size);
/* zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight end > */

int k3_isp_get_fps(camera_fps fps);
int k3_isp_set_fps(camera_fps fps, u8 value);
int k3_isp_get_actual_iso(void);
int k3_isp_get_hdr_iso_exp(hdr_para_reserved *iso_exp);
int k3_isp_get_awb_gain(int withShift);
int k3_isp_get_focus_code(void);
int k3_isp_get_focus_rect(camera_rect_s *rect);
int k3_isp_get_expo_line(void);
int k3_isp_get_sensor_vts(void);
int k3_isp_get_current_ccm_rgain(void);
int k3_isp_get_current_ccm_bgain(void);
int k3_isp_get_sensor_lux_matrix(lux_stat_matrix_tbl * lux_matrix);
int k3_isp_get_sensor_hdr_info(hdr_info * hdrInfo);
int  k3_isp_get_sensor_preview_max_size(preview_size * pre_size);
/*AndroidK3 added by y36721 end*/
int k3_isp_get_sensor_aperture(void);
int k3_isp_get_equivalent_focus(void);

int k3_isp_set_flash_mode(camera_flash flash_mode);
int k3_isp_get_flash_mode(void);

int k3_isp_set_hflip(int flip);
int k3_isp_get_hflip(void);
int k3_isp_set_vflip(int flip);
int k3_isp_get_vflip(void);

int k3_isp_get_focus_distance(void);
int k3_isp_get_current_vts(void);
int k3_isp_get_current_fps(void);
int k3_isp_get_band_threshold(void);
void k3_isp_set_fps_lock(int);
void k3_isp_switch_hdr_movie(u8 on);
void k3_isp_set_shoot_mode(camera_shoot_mode shoot_mode);

void k3_isp_set_pm_mode(u8 pm_mode);

void k3_isp_set_video_stabilization(int bStabilization);
void k3_isp_set_yuv_crop_pos(int point);
void k3_isp_get_yuv_crop_rect(crop_rect_s *rect);

int k3_isp_get_hdr_movie_support(void);

int k3_check_lcd_compensation_supported(void);
int k3_check_lcd_compensation_needed(void);
int k3_isp_set_scene_type(scene_type *type);
#endif /*__K3_V4L2_CAPTURE_H__ */

/********************************* END ***********************************************/
