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

#ifndef __K3_ISP_H__
#define __K3_ISP_H__

#include <linux/videodev2.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <mach/platform.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <linux/pm_qos_params.h>
#include "sensor_common.h"
#include "cam_util.h"

#include "k3_ispv1_hdr_movie_ae.h"
#define V4L2_PIX_FMT_INVALID  0

/* yuv scale down ratio = N/0x10000 */
/* yuv scale up ratio = 0x10000/N */
#define YUV_SCALE_DIVIDEND	0x10000

#define ZOOM_BASE	0x100	/* 1x */
#define isp_zoom_to_ratio(zoom, video_stab) \
		(video_stab) ? (0x120 + (zoom)*24):(((zoom) * 10 + 100) * 0x100 / 100)

/* command set CMD_ZOOM_IN_MODE relative parameters */
/* REG1 bit[0] : 1 for high quality, 0 for save power mode */
typedef enum {
	SAVE_POWER_MODE = 0x0,
	HIGH_QUALITY_MODE = 0x1,
} zoom_quality_t;

/* ISP In/Out data type */
enum {
	RAW_DATA = 0,
	YUV_DATA,
	RGB_DATA,
	JPEG_DATA,
	PACKED_DATA
};

/* RAW_DATA type */
enum {
	RAW8 = 0,
	RAW10,
	RAW12,
	RAW14
};

/* YUV422 type */
enum {
	UYVY = 0,
	VYUY,
	YUYV,
	YVYU
};

enum {
	CAMERA_CTL_OFF = 0,
	CAMERA_CTL_OVERLAY_START,
	CAMERA_CTL_CAPTURE_START,
	CAMERA_CTL_OVERLAY,
	CAMERA_CTL_CAPTURE,
};

enum {
	BUF_LEFT = 0,
	BUF_RIGHT,
	BUF_INVALID,
};

typedef enum {
	AUTO_AECAGC = 0,
	MANUAL_AECAGC,
} aecagc_mode_t;

typedef enum {
	AUTO_AWB = 0,
	MANUAL_AWB,
} awb_mode_t;

typedef enum {
	RAW_SCALE_FIRST,
	YUV_SCALE_FIRST,
} scale_strategy_t;

typedef enum {
	UV_ADJUST_DISABLE,
	UV_ADJUST_ENABLE,
} uv_adjust_t;

typedef struct _pic_attr {
	u32 sensor_width;	/* sensor output width */
	u32 sensor_height;
	u32 in_width;		/* isp input width */
	u32 in_height;
	u32 startx;
	u32 starty;
	u32 crop_x;
	u32 crop_y;
	u32 crop_width;
	u32 crop_height;
	u32 out_width;
	u32 out_height;
	u32 out_stride;

	u32 raw_scale_down;
	u32 yuv_dcw;
	u32 yuv_in_width;
	u32 yuv_in_height;
	u32 yuv_down_scale_nscale;
	u32 yuv_up_scale_nscale;

	u32 in_fmt;
	u32 out_fmt;
} pic_attr_t;

typedef struct _k3_isp_data {
	bool powered;
	bool cold_boot;
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
	struct pm_qos_request_list qos_request;
	struct pm_qos_request_list qos_request_gpu;
	struct pm_qos_request_list qos_request_cpu;
#endif

	struct delayed_work	turn_on_flash_work;
	bool already_turn_on_flash;
	/* picture attribute varibles */
	pic_attr_t pic_attr[STATE_MAX];
	int bracket_ev[MAX_BRACKET_COUNT];	/* [-4, 4] step is 0.5 */
	u32 *support_pixfmt;
	u32 pixfmt_count;
	u32 zoom;
	int video_stab;
	struct v4l2_fract frame_rate;

	camera_sensor *sensor;

	/* AndroidK3 added by y36721 for focus 2011-10-28 begin */
	camera_focus focus;

	camera_anti_shaking	anti_shaking_enable;
	camera_metering		metering;
	metering_area_s		ae_area;
	camera_anti_banding	anti_banding;
	camera_white_balance	awb_mode;

	awb_mode_t                 awb_lock;
	camera_iso		iso;
	camera_sharpness	sharpness;
	camera_saturation	saturation;
	camera_contrast		contrast;
	camera_brightness	brightness;
	camera_effects		effect;

	camera_flash		flash_mode;
	camera_flash_flow	flash_flow;
	camera_flash_state	flash_state;
	bool 			flash_on;
	bool			assistant_af_flash;
	bool			af_need_flash;
	camera_shoot_mode	shoot_mode;

	camera_scene		scene;
	int					ev;
	camera_frame_rate_mode fps_mode;
	camera_frame_buf		*current_frame;

	struct semaphore frame_sem;
	/* AndroidK3 added by y36721 for focus 2011-10-28 end */
    lux_stat_matrix_tbl lux_stat_matrix;
	bool  hdr_switch_on;

	scene_type sceneInfo;
} k3_isp_data;

typedef struct _k3_last_state {
	camera_saturation	saturation;
	camera_contrast	contrast;
	camera_brightness	brightness;
	camera_effects	effect;
} k3_last_state;

typedef struct _v4l2_callback {
	int (*preview_start_callback) (void);
	int (*preview_done_callback) (void);

	int (*capture_start_callback) (void);
	int (*capture_done_callback) (void);
} frame_callback_t;

struct i2c_t;
typedef struct _isp_hw_controller {
	int (*isp_hw_init)(struct platform_device *pdev, data_queue_t *data_queue);
	void (*isp_hw_deinit) (void);
	void (*isp_hw_init_regs) (data_interface_t data_interface);
	void (*isp_hw_deinit_regs) (void);

	void (*set_i2c) (struct i2c_t *i2c);
	void (*isp_set_process_mode) (u32 w, u32 h);
	isp_process_mode_t(*isp_get_process_mode) (void);
	int (*isp_check_config) (struct v4l2_pix_format *pixfmt);
	int (*isp_set_zoom) (u32 ratio, zoom_quality_t quality);
	int (*isp_get_fps) (camera_sensor *sensor, camera_fps fps);
	int (*isp_set_fps) (camera_sensor *sensor, camera_fps fps, u8 value);

	int (*start_preview) (pic_attr_t *pic_attr, camera_sensor *sensor, bool cold_boot, camera_scene scene);
	int (*start_capture) (pic_attr_t *pic_attr, camera_sensor *sensor,  int *ev, bool flash_on, camera_scene scene);
	#ifdef READ_BACK_RAW
	void (*update_read_ready) (u8 buf_used);
	#endif
	int (*start_process) (pic_attr_t *pic_attr, ipp_mode mode);
	int (*stop_preview) (void);
	int (*stop_capture) (void);
	int (*stop_process) (ipp_mode mode);
	void (*refresh_support_fmt) (u32 **pixfmt, u32 *cnt);
	void (*isp_fill_fmt_info) (struct v4l2_pix_format *f);
	int (*capture_update_addr) (u32 phyaddr);
	int (*preview_update_addr) (u32 phyaddr);

	int (*isp_poweron) (void);
	void (*isp_poweroff) (void);

	void (*isp_tune_ops_init) (k3_isp_data *ispdata);
	void (*isp_tune_ops_prepare) (camera_state state);
	void (*isp_tune_ops_withdraw) (camera_state state);
	void (*isp_tune_ops_exit) (void);

	isp_tune_ops *isp_tune_ops;

	void (*isp_set_auto_flash) (int status, camera_flash flash_mode);
	bool (*isp_is_need_flash) (camera_sensor *sensor);

	void (*isp_set_aecagc_mode) (aecagc_mode_t mode);
	/*h00206029_20120221*/
	void (*isp_set_awb_mode) (awb_mode_t mode);

	void (*isp_check_flash_prepare) (void);

	int (*cold_boot_set)(camera_sensor *sensor);

	int (*isp_get_current_vts)(camera_sensor *sensor);
	int (*isp_get_current_fps)(camera_sensor *sensor);
	int (*isp_get_band_threshold)(camera_sensor *sensor, camera_anti_banding banding);
	void (*isp_set_video_stabilization)(int bStabilization);
	void (*isp_set_yuv_crop_pos)(int point);
	void (*isp_get_yuv_crop_rect)(crop_rect_s *rect);
} isp_hw_controller;

/* get isp hardware control data struct */
isp_hw_controller *get_isp_hw_controller(void);

int k3_ispio_init(struct platform_device *pdev);
void k3_ispio_deinit(void);

/* Used to switch between yuv rect and raw rect */
int k3_isp_yuvrect_to_rawrect(camera_rect_s *yuv, camera_rect_s *raw);
int k3_isp_rawrect_to_yuvrect(camera_rect_s *yuv, camera_rect_s *raw);
int k3_isp_antishaking_rect_stat2out(camera_rect_s *out, camera_rect_s *stat);
int k3_isp_antishaking_rect_out2stat(camera_rect_s *out, camera_rect_s *stat);

#endif /*__K3_ISP_H__ */

/********************** END ***********************/
