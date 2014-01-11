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
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>
#include <linux/fb.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <asm/io.h>
#include <asm/bug.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/android_pmem.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/time.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

#include <linux/workqueue.h>
#include <linux/semaphore.h>

#include "cam_util.h"
#include "cam_dbg.h"
#include "k3_isp.h"
#include "k3_ispv1.h"
#include "k3_ispv1_afae.h"
#include "k3_isp_io.h"

#include "mach/irqs.h"

#define LOG_TAG "K3_ISPV1_AFAE"
#include "cam_log.h"

#define ISP_MAX_FOCUSED_WIN		4
#define ISP_MAX_FOCUS_WIN		25

/* camera af param default definitionr */
/* same as metering default CWA area */
#define DEFAULT_AF_WIDTH_PERCENT	(12)
#define DEFAULT_AF_HEIGHT_PERCENT	(15)
#define DEFAULT_AF_MIN_HEIGHT_RATIO	(7)
#define DEFAULT_AF_MAX_FOCUS_STEP	(6)
#define DEFAULT_AF_GSENSOR_INTERVAL_THRESHOLD	(70)

/* with stabilizer, max size is larger than 1080P. */
#define MAX_PREVIEW_WIDTH	2112
#define MAX_PREVIEW_HEIGHT	1200

#define AF_SINGLE_WINMODE_ONLY

typedef struct _focus_win_info_s{
	u32 left;
	u32 top;
	u32 width;
	u32 height;

	u32 width1;
	u32 height1;

	u32 weight[ISP_MAX_FOCUS_WIN];
} focus_win_info_s;

typedef struct _focused_result_s {
	u32 focused_win_num;
	u32 focused_win[ISP_MAX_FOCUSED_WIN];
} focused_result_s;

/* point to global isp_data */
extern k3_isp_data *this_ispdata;

u32 raw_unit_area;
ispv1_afae_ctrl *afae_ctrl;
static camera_metering this_metering = CAMERA_METERING_CWA;
struct semaphore sem_af_schedule;

axis_triple mXYZ;

#define AF_TIME_PRINT
#ifdef AF_TIME_PRINT
	static struct timeval tv_start, tv_end;
#endif

static int __attribute__((unused)) ispv1_get_merged_rect(focus_area_s *area,
				camera_rect_s *yuv_rect, u32 preview_width, u32 preview_height);
static int __attribute__((unused)) ispv1_get_focus_win_info(camera_rect_s *raw_rect,
				focus_win_info_s *win_info, int *binning);
static int __attribute__((unused)) ispv1_get_map_table(focus_area_s *area,
				focus_win_info_s *win_info, int *map_table);

static bool ispv1_check_rect_differ(camera_rect_s *rect1, camera_rect_s *rect2,
				   u32 preview_width, u32 preview_height);

struct workqueue_struct *af_start_work_queue;
struct work_struct af_start_work;
static void ispv1_af_start_work_func(struct work_struct *work);

static bool ispv1_setreg_vcm_code(u32 vcm_code);
static void ispv1_focus_vcm_go_infinite(u8 delay_flag);

static void ispv1_setreg_focus_win(focus_win_info_s *win_info);
static int ispv1_setreg_vcm_DLC_mode(void);
static int ispv1_setreg_vcm_lsc_mode(u16 para1, u16 para2);
static void ispv1_setreg_focus_init(vcm_info_s *vcm_info);
static void ispv1_setreg_focus_framerate(int framerate);
static void ispv1_setreg_vcm(vcm_info_s *vcm_info);
static int ispv1_setreg_af_mode(camera_af_mode mode);
static int ispv1_getreg_focus_result(focused_result_s *result, int multi_win);

static int ispv1_set_focus_mode_done(camera_focus focus_mode);
static int ispv1_set_focus_area_done(focus_area_s *area, u32 zoom);

u32 ispv1_focus_get_win_lum(lum_win_info_s *lum_info);

static void ispv1_assistant_af(bool action);
static bool ispv1_focus_need_flash(u32 cur_lum, u32 cur_gain, bool binning);
static bool ispv1_af_flash_needed(camera_sensor *sensor, camera_flash flash_mode, lum_win_info_s lum_info);
static void ispv1_af_flash_check_open(void);
static void ispv1_af_flash_check_close(void);

static void ispv1_focus_status_reset(void);

static bool ispv1_af_need_wait_stable(u8 stage, u8 hold_cnt);
static void ispv1_af_range_cal(af_run_param *af_info, pos_info *curr, af_run_stage next_stage);
static void ispv1_vcaf_range_cal(af_run_param *af_info, pos_info *curr, af_run_stage next_stage, FOCUS_STATUS result);
static af_run_stage ispv1_af_recognise_curve(af_run_param *af_info, pos_info *curr, vcm_info_s *vcm);
static af_run_stage ispv1_vcaf_recognise_curve(af_run_param *af_info, pos_info *curr, vcm_info_s *vcm, u32 *reserved);
static af_run_stage ispv1_af_search_top(af_run_param *af_info, pos_info *curr, vcm_info_s *vcm, bool coarse);
static af_run_stage ispv1_vcaf_search_top(af_run_param *af_info, pos_info *curr, vcm_info_s *vcm);
static FOCUS_STATUS ispv1_af_judge_result(af_run_param *af_info);

bool ispv1_check_caf_need_restart(focus_frame_stat_s *start_data, focus_frame_stat_s *end_data);

void ispv1_k3focus_run(void);
u32 ispv1_get_single_win_raw_lum(u8 win_idx, u32 stat_unit_area);

static inline FOCUS_STATUS get_focus_result(void)
{
	return afae_ctrl->af_result;
}

static inline void set_focus_result(FOCUS_STATUS status)
{
	afae_ctrl->af_result = status;
}

static inline void save_focus_code(int code)
{
	afae_ctrl->curr.code = code;
}

static inline int get_focus_code(void)
{
	return afae_ctrl->curr.code;
}

static inline u16 get_caf_forcestart(void)
{
	return afae_ctrl->force_start;
}

static inline void set_caf_forcestart(int value)
{
	afae_ctrl->force_start = value;
}

inline focus_state_e get_focus_state(void)
{
	return afae_ctrl->focus_state;
}

inline void set_focus_state(focus_state_e state)
{
	afae_ctrl->focus_state = state;
}

inline af_run_stage get_focus_stage(void)
{
	return afae_ctrl->af_stage;
}

inline void set_focus_stage(af_run_stage stage)
{
	afae_ctrl->af_stage = stage;
}

inline bool afae_ctrl_is_valid(void)
{
	return (afae_ctrl != NULL);
}

static inline void set_pre_focus_mode(camera_focus mode)
{
	afae_ctrl->pre_mode = mode;
}

static inline camera_focus get_pre_focus_mode(void)
{
	return afae_ctrl->pre_mode;
}

static inline camera_focus get_focus_mode(void)
{
	return afae_ctrl->mode;
}

static inline void set_focus_mode(camera_focus mode)
{
	afae_ctrl->mode = mode;
}

static inline vcm_info_s *get_vcm_ptr(void)
{
	return this_ispdata->sensor->vcm;
}

static inline bool ispv1_get_af_run_sem(void)
{
	long jiffies = 0;

	jiffies = msecs_to_jiffies(2000);
	if (down_timeout(&(afae_ctrl->af_run_sem), jiffies)) {
		print_error("wait focus semaphore timeout");
		return false;
	} else {
		print_debug("focus semaphore waking up###########");
		return true;
	}
}

static inline void ispv1_free_af_run_sem(void)
{
	up(&(afae_ctrl->af_run_sem));
}

inline u32 ispv1_get_stat_unit_area(void)
{
	return raw_unit_area;
}

inline void ispv1_set_stat_unit_area(u32 unit_area)
{
	raw_unit_area = unit_area;
}

u32 ispv1_get_focus_code(void)
{
	u32 code = 0;
	focus_result_s result;
	int ret;

	code = get_focus_code();
	ret = ispv1_get_focus_result(&result);

	code &= 0xfffc;
	code |= result.status;

	return code;
}

/* set focus performance registers */
static void ispv1_setvcm_init(vcm_info_s *vcm_info)
{
	print_debug("enter %s", __func__);

	/* init vcm ad5823 */
	if ((vcm_info->moveLensAddr[0] == 0x4) && (vcm_info->moveLensAddr[1] == 0x5)) {
		ispv1_write_vcm(vcm_info->vcm_id, 0x02, 0x01, I2C_8BIT, SCCB_BUS_MUTEX_WAIT);
		ispv1_write_vcm(vcm_info->vcm_id, 0x03, 0x80, I2C_8BIT, SCCB_BUS_MUTEX_WAIT);
	}
}

/* added for micro distance */
static u32 get_af_large_step(u32 step, u32 ratio)
{
	u32 ret;
	ret  = step * ratio / FOCUS_PARAM_LARGE_STRIDE_BASE;
	return ret;
}

static void ispv1_adjust_vcm_range(vcm_info_s *vcm)
{
	int range;
	u32 large_step;

	range = vcm->normalDistanceEnd - vcm->infiniteDistance;

	/* calculate focus range in picture mode */
	large_step = get_af_large_step(vcm->normalStep, vcm->normalStrideRatio);
	if ((range - vcm->strideOffset) % large_step != 0) {
		range = vcm->strideOffset + ((range - vcm->strideOffset) / large_step + 1) * large_step;
	}

	vcm->normalDistanceEnd = vcm->infiniteDistance + range;

	range = vcm->videoDistanceEnd - vcm->infiniteDistance;

	/* calculate focus range in video mode */
	large_step = get_af_large_step(vcm->videoStep, vcm->videoStrideRatio);
	if ((range - vcm->strideOffset) % large_step != 0) {
		range = vcm->strideOffset + ((range - vcm->strideOffset) / large_step + 1) * large_step;
	}

	vcm->videoDistanceEnd = vcm->infiniteDistance + range;
}

static void ispv1_cal_vcm_range(vcm_info_s *vcm)
{
	u16 otp_start = 0;
	u16 otp_end = 0;
	int otp_range;

	int normal_range;
	int normal_end;

	int video_range;
	int video_end;

	if (vcm->get_vcm_otp != NULL) {
		vcm->get_vcm_otp(&otp_start, &otp_end);
		print_info("get 10bit otp start 0x%x, end 0x%x***************", otp_start, otp_end);

		if (otp_start > 0xf0)
			otp_start = 0xf0;

		if (otp_end != 0) {
			normal_range = vcm->normalDistanceEnd - vcm->infiniteDistance;
			video_range = vcm->videoDistanceEnd - vcm->infiniteDistance;
			otp_range = (otp_end & 0x3f0) - (otp_start & 0x3f0);

			normal_range = (normal_range < otp_range) ? normal_range:otp_range;
			video_range = (video_range < otp_range) ? video_range:otp_range;
			if( (otp_start & 0x3f0) < (2 * vcm->normalStep) ){
				vcm->infiniteDistance = (2 * vcm->normalStep);
			}else{
				vcm->infiniteDistance = (otp_start & 0x3f0);
			}

			normal_end = vcm->infiniteDistance + normal_range;
			if (normal_end >= 0x400) {
				print_error("normalDistanceEnd 0x%x too large!", vcm->normalDistanceEnd);
				normal_range = (0x3f0 - vcm->infiniteDistance);
				normal_range -= (normal_range % vcm->normalStep);
				normal_end = vcm->infiniteDistance + normal_range;
			} else if (normal_end < vcm->normalDistanceEnd) {
				normal_end = vcm->normalDistanceEnd;
			}
			vcm->normalDistanceEnd = normal_end;

			video_end = vcm->infiniteDistance + video_range;
			if (vcm->videoDistanceEnd >= 0x400) {
				print_error("videoDistanceEnd 0x%x too large!", vcm->videoDistanceEnd);
				video_range = (0x3f0 - vcm->infiniteDistance);
				video_range -= (normal_range % vcm->videoStep);
				video_end = vcm->infiniteDistance + video_range;
			} else if (video_end < vcm->videoDistanceEnd) {
				video_end = vcm->videoDistanceEnd;
			}
			vcm->videoDistanceEnd = video_end;
		}
	}

	/*adjust range again */
	ispv1_adjust_vcm_range(vcm);
	print_info("focus infiniteDistance 0x%x, normalDistanceEnd 0x%x, videoDistanceEnd 0x%x***************",
		vcm->infiniteDistance, vcm->normalDistanceEnd, vcm->videoDistanceEnd);
}
static int ispv1_setreg_vcm_code_done(vcm_info_s *vcm, u32 vcm_code)
{
	int ret = 0;
#if 0
	SETREG16(0x1c5da, vcm_code & 0xffff);
	SETREG8(0x1cddd, 0x01);
#else
	if (vcm->vcm_type == VCM_AD5823) {
		ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], ((vcm_code >> 8) | 0x04), I2C_8BIT, SCCB_BUS_MUTEX_NOWAIT);
		ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[1], (vcm_code & 0xFF), I2C_8BIT, SCCB_BUS_MUTEX_NOWAIT);
	} else if (vcm->vcm_type == VCM_DW9714) {
		/*in dw9714 case, ad value must set to bit4 ~ bit13*/
		ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], vcm_code << 4, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
	} else if (vcm->vcm_type == VCM_DW9714_SS || vcm->vcm_type == VCM_DW9714_Sunny || vcm->vcm_type == VCM_DW9714_Liteon) {
		/*in dw9714 case, ad value must set to bit4 ~ bit13*/
		ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], (vcm_code << 4) | 0x04, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
	} else {
		print_error("%s: unsupported vcm",  __func__);
		ret = -1;
	}
#endif
	return ret;
}


static bool ispv1_setreg_vcm_code(u32 dest_code)
{
	int ret = 0;
	vcm_info_s *vcm = get_vcm_ptr();
	u32 curr_code = get_focus_code();
	bool divide_mode = false;
	int unit_step = 0;
	u32 safe_position = vcm->offsetInit + vcm->strideDivideOffset;
	int max_step_cnt = 0;
	int step_index = 0;

	/* if the dest code smaller than safe_position and the current code is larger than safe_position, divide the stride to avoid clash */
	if ((dest_code < safe_position) && (curr_code > safe_position)
	    && ((curr_code - dest_code) > vcm->strideDivideOffset)) {
		divide_mode = true;
		if (vcm->vcm_type == VCM_AD5823) {
			/* for AD5823, divide the stride into four steps */
			unit_step = (curr_code - dest_code) / 4;
			/* limit the minimal unit step to be 0x40*/
			if (unit_step < 0x40)
				unit_step = 0x40;
			if ((curr_code - dest_code) % unit_step == 0) {
				max_step_cnt = (curr_code - dest_code) / unit_step;
			} else {
				max_step_cnt = (curr_code - dest_code) / unit_step + 1;
			}
		} else if (vcm->vcm_type == VCM_DW9714) {
			/* for DW9714, divide the stride into two steps */
			unit_step = curr_code - safe_position;
			max_step_cnt = 2;
		}  else if ((vcm->vcm_type == VCM_DW9714_SS  || vcm->vcm_type == VCM_DW9714_Sunny || vcm->vcm_type == VCM_DW9714_Liteon) ){
			if((curr_code - dest_code) > 550) {
				ispv1_setreg_vcm_lsc_mode(0xA106, 0xF2B0);
			} else if ((curr_code - dest_code) > 500) {
				ispv1_setreg_vcm_lsc_mode(0xA106, 0xF298);
			} else if ((curr_code - dest_code) > 450) {
				ispv1_setreg_vcm_lsc_mode(0xA105, 0xF258);
			} else if ((curr_code - dest_code) > 400) {
				ispv1_setreg_vcm_lsc_mode(0xA105, 0xF228);
			} else if ((curr_code - dest_code) > 350) {
				ispv1_setreg_vcm_lsc_mode(0xA105, 0xF2F8);
			} else if ((curr_code - dest_code) > 300) {
				ispv1_setreg_vcm_lsc_mode(0xA105, 0xF2D0);
			} else if ((curr_code - dest_code) > 250) {
				ispv1_setreg_vcm_lsc_mode(0xA105, 0xF2A8);
			} else if ((curr_code - dest_code) > 200) {
				ispv1_setreg_vcm_lsc_mode(0xA105, 0xF288);
			} else if ((curr_code - dest_code) > 150) {
				ispv1_setreg_vcm_lsc_mode(0xA104, 0xF2F8);
			} else if ((curr_code - dest_code) > 100) {
				ispv1_setreg_vcm_lsc_mode(0xA104, 0xF2A8);				
			}else if ((curr_code - dest_code) > 50) {
				ispv1_setreg_vcm_lsc_mode(0xA104, 0xF280);				
			}  else {
				ispv1_setreg_vcm_DLC_mode();
			}
			max_step_cnt = 1;
		} else {
			print_error("%s: unsupported vcm",  __func__);
			ret = -1;
			return ret;
		}

		for (step_index = 0; step_index < max_step_cnt - 1; step_index++) {
			curr_code -= unit_step;
			ret = ispv1_setreg_vcm_code_done(vcm, curr_code);
			msleep(vcm->motorDelayTime);
		}
		ret = ispv1_setreg_vcm_code_done(vcm, dest_code);
	} else {
		if (vcm->vcm_type == VCM_DW9714_SS || vcm->vcm_type == VCM_DW9714_Sunny || vcm->vcm_type == VCM_DW9714_Liteon)
			ispv1_setreg_vcm_DLC_mode();
		ret = ispv1_setreg_vcm_code_done(vcm, dest_code);
	}

	if (ret != -1)
		save_focus_code(dest_code);

	return divide_mode;
}

/*
  *************************************************************************
  * FunctionName: ispv1_focus_vcm_go_infinite;
  * Description : using for vcm go to infinite position
  * Input       : delay_flag : 0 meens needless hold time;
  *			       1 meens need hold time
  * Output      : NA;
  * ReturnValue : NA;
  * Other       : in delay_flag equals 1 case, using for exit camera ;
  **************************************************************************
  */
static void ispv1_focus_vcm_go_infinite(u8 delay_flag)
{
	vcm_info_s *vcm = get_vcm_ptr();
	int val_vcm = get_focus_code();
	bool divide_mode = false;

	print_debug("%s, val_vcm 0x%x, infiniteDistance 0x%x, vcm_id 0x%x, Addr 0x%x, 0x%x",
		__func__, val_vcm, vcm->infiniteDistance, vcm->vcm_id,
		vcm->moveLensAddr[0], vcm->moveLensAddr[1]);

	ispv1_auto_focus(FOCUS_STOP);	/*Stop auto focus mode */

	/*
	 * in power off camera case,
	 * if current position bigger than infinite distance(safe position),
	 * then must return to infinite position by slow speed.
	 * after arriving infinite position, holding power few micro seconds,
	 * then power off camera.
	 * but, in change mode case,
	 * set current position to infinite distance directly.
	 */
	if (val_vcm <= vcm->infiniteDistance)
		return;

	divide_mode = ispv1_setreg_vcm_code(vcm->infiniteDistance);

	if ((1 == delay_flag) && (true == divide_mode)) {
		msleep(50);	/*holding time */
	}
}
static int ispv1_setreg_vcm_lsc_mode(u16 para1, u16 para2)
{
	int ret = 0;
	vcm_info_s *vcm = this_ispdata->sensor->vcm;
	print_info("enter %s, para1:0x%x, para2:0x%x", __func__, para1, para2);
	ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xECA3, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
	ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], para1, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
	ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], para2, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
	ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xDC51, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);

	return ret;
}
static int ispv1_setreg_vcm_DLC_mode()
	{	
		int ret = 0;	
		vcm_info_s *vcm = this_ispdata->sensor->vcm;	

		if ((vcm->moveLensAddr[0] == 0x0) && (vcm->moveLensAddr[1] == 0x0)) {
			/*set DLC mode*/		
			/*			-------------DLC Mode Setting Value -------------		
			DW9714A_WRITE(0x18, 0xEC, 0xA3); // Ringing Setting ON		
			DW9714A_WRITE(0x18, 0xF2, 0x20); //Tvib/2 =5.8ms setting		
			DW9714A_WRITE(0x18, 0xA1, 0x0D); //DLC=b’1, MCLK[1:0]=01		
			DW9714A_WRITE(0x18, 0xDC, 0x51); // Ringing Setting OFF		

			---------------------------------------------------------------------------------------------		*/	
			ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xECA3, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);		
		if (vcm->vcm_type == VCM_DW9714_SS) {
			ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xF2e8, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
			ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xA10d, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
		} else if (vcm->vcm_type == VCM_DW9714) {
			ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xF220, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
			ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xA10D, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
		} else if (vcm->vcm_type == VCM_DW9714_Sunny) {
			ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xF260, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
			ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xA10D, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
		} else if (vcm->vcm_type == VCM_DW9714_Liteon) {
			ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xF208, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
			ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xA10D, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);
		}
			ret |= ispv1_write_vcm(vcm->vcm_id, vcm->moveLensAddr[0], 0xDC51, I2C_16BIT, SCCB_BUS_MUTEX_NOWAIT);	
			} else {		
				print_error("%s: unsupported vcm",  __func__);	
			}	

			return ret;
	}



/* Set focus windows(suitable for ISP) to registers. */
static void ispv1_setreg_focus_win(focus_win_info_s *win_info)
{
	u32 loopi;

	print_debug("enter %s", __func__);

	assert(win_info);

	SETREG16(REG_ISP_FOCUS_WIN_X0, win_info->left);
	SETREG16(REG_ISP_FOCUS_WIN_Y0, win_info->top);
	SETREG16(REG_ISP_FOCUS_WIN_W0, win_info->width);
	SETREG16(REG_ISP_FOCUS_WIN_H0, win_info->height);

	if ((win_info->width1 == 0) && (win_info->height1 == 0)) {
		print_debug("%s: single win mode", __func__);
		return;
	}

	SETREG16(REG_ISP_FOCUS_WIN_W1, win_info->width1);
	SETREG16(REG_ISP_FOCUS_WIN_H1, win_info->height1);

	for (loopi = 0; loopi < ISP_MAX_FOCUS_WIN; loopi++) {
		SETREG16(REG_ISP_FOCUS_WEIGHT_LIST(loopi), win_info->weight[loopi]);
	}
}

/*
 **************************************************************************
 * FunctionName: ispv1_setreg_af_mode;
 * Description : set auto focus mode;
 * Input       : mode; 0-snapshot auto mode;
 *                             1-snapshot single mode;
 *                             2-snapshot weighted mode;
 *                             3-video servo mode.
 * Output      : NA;
 * ReturnValue : ture or false;
 * Other       : NA;
 **************************************************************************
 */
static int ispv1_setreg_af_mode(camera_af_mode mode)
{
	u8 reg1 = 0;

	print_debug("enter %s, af mode %d", __func__, mode);

	SETREG16(REG_ISP_FOCUS_AFCCTRL2, 0x10);

	/*
	 * 0x66100: [2] bIfBinning: (1:binning; 0:non-binning); [1] nMode: (1:5x5;0:single)
	 * ISP_VIDEO_SERVO_MODE/ISP_SINGLE_SNAPSHOT_MODE is 0x4;
	 * ISP_AUTO_SNAPSHOT_MODE/ISP_WEIGHTED_SNAPSHOT_MODE is 0x2;
	 *
	 * REG_ISP_FOCUS_MODE; video 1; snapshot 0
	 * REG_ISP_FOCUS_WINMODE; single 0; 5x5 1;
	 * REG_ISP_FOCUS_bIFAUTO; auto 1;
	 * REG_ISP_FOCUS_bIFWEIGHTED; weighted 1; only works when nWinMode=1;
	 */
#ifdef AF_SINGLE_WINMODE_ONLY
	reg1 = ISP_FOCUS_NONBINNING;
#else
	if (afae_ctrl->binning)
		reg1 = ISP_FOCUS_BINNING;
	else
		reg1 = ISP_FOCUS_NONBINNING;
#endif
	/* set register REG_ISP_FOCUS_AFCCTRL0's bit0 to 1  */
	reg1 |= 0x01;
	switch (mode) {
	case CAMERA_VIDEO_SERVO_MODE:
		SETREG8(REG_ISP_FOCUS_AFCCTRL0, reg1 | ISP_FOCUS_SINGLE_WINDOW);
		SETREG8(REG_ISP_FOCUS_MODE, ISP_FOCUS_VIDEO_MODE);
		SETREG8(REG_ISP_FOCUS_WINMODE, ISP_FOCUS_WINMODE_SINGLE);
		/*
		 * No need to set REG_ISP_FOCUS_bIFAUTO
		 * No need to set REG_ISP_FOCUS_bIFWEIGHTED
		 */
		break;

	case CAMERA_SINGLE_SNAPSHOT_MODE:
		SETREG8(REG_ISP_FOCUS_AFCCTRL0, reg1 | ISP_FOCUS_SINGLE_WINDOW);
		SETREG8(REG_ISP_FOCUS_MODE, ISP_FOCUS_SNAPSHOT_MODE);
		SETREG8(REG_ISP_FOCUS_WINMODE, ISP_FOCUS_WINMODE_SINGLE);
		/*
		 * No need to set REG_ISP_FOCUS_bIFAUTO
		 * No need to set REG_ISP_FOCUS_bIFWEIGHTED
		 */
		break;

	case CAMERA_AUTO_SNAPSHOT_MODE:
		SETREG8(REG_ISP_FOCUS_AFCCTRL0, reg1 | ISP_FOCUS_5X5_WINDOW);
		SETREG8(REG_ISP_FOCUS_MODE, ISP_FOCUS_SNAPSHOT_MODE);
		SETREG8(REG_ISP_FOCUS_WINMODE, ISP_FOCUS_WINMODE_5X5);
		SETREG8(REG_ISP_FOCUS_IFAUTO, ISP_FOCUS_IS_AUTO);
		SETREG8(REG_ISP_FOCUS_IFWEIGHTED, ISP_FOCUS_ISNOT_WEIGHTED);
		break;

	case CAMERA_WEIGHTED_SNAPSHOT_MODE:
		SETREG8(REG_ISP_FOCUS_AFCCTRL0, reg1 | ISP_FOCUS_5X5_WINDOW);
		SETREG8(REG_ISP_FOCUS_MODE, ISP_FOCUS_SNAPSHOT_MODE);
		SETREG8(REG_ISP_FOCUS_WINMODE, ISP_FOCUS_WINMODE_5X5);
		SETREG8(REG_ISP_FOCUS_IFAUTO, ISP_FOCUS_ISNOT_AUTO);
		SETREG8(REG_ISP_FOCUS_IFWEIGHTED, ISP_FOCUS_IS_WEIGHTED);
		break;

	default:
		print_error("ispv1_set_focus_mode param error!\n");
		return -1;
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: ispv1_getreg_focus_result;
 * Description : get focus result from isp
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : STATUS_FOCUSING, STATUS_FOCUSED, STATUS_OUT_FOCUS
 * Other       : NA;
 **************************************************************************
 */
static int ispv1_getreg_focus_result(focused_result_s *result, int multi_win)
{
	assert(result);
	print_debug("enter %s", __func__);

	result->focused_win_num = 1;
	result->focused_win[0] = 0;

	return get_focus_result();
}

/*Just set some fixed parameters. And get some resource. */
int ispv1_focus_init(void)
{
	vcm_info_s *vcm = get_vcm_ptr();
	u32 size;
	u32 width_precent = DEFAULT_AF_WIDTH_PERCENT;
	u32 height_precent = DEFAULT_AF_HEIGHT_PERCENT;

	if(this_ispdata->sensor->get_af_param){
		width_precent = (u32)(this_ispdata->sensor->get_af_param(AF_WIDTH_PERCENT));
		height_precent = (u32)(this_ispdata->sensor->get_af_param(AF_HEIGHT_PERCENT));
	}

	afae_ctrl = kmalloc(sizeof(ispv1_afae_ctrl), GFP_KERNEL);
	if (!afae_ctrl) {
		print_error("malloc is failed in %s function at line#%d\n", __func__, __LINE__);
		return -ENOMEM;
	}
	memset(afae_ctrl, 0, sizeof(ispv1_afae_ctrl));

	afae_ctrl->map_table = kmalloc(MAX_FOCUS_RECT * ISP_MAX_FOCUS_WIN * sizeof(int), GFP_KERNEL);
	if (!afae_ctrl->map_table) {
		print_error("malloc is failed in %s function at line#%d\n", __func__, __LINE__);
		kfree(afae_ctrl);
		afae_ctrl = NULL;
		return -ENOMEM;
	}
	memset(afae_ctrl->map_table, 0, MAX_FOCUS_RECT * ISP_MAX_FOCUS_WIN * sizeof(int));

	/* request stat data for copy and calculate contrast extend threshold. */
	size = MAX_PREVIEW_WIDTH * MAX_PREVIEW_HEIGHT *
		width_precent * height_precent / 100 / 100;
	afae_ctrl->stat_data = vmalloc(size);
	if (afae_ctrl->stat_data == NULL) {
		print_error("malloc is failed in %s function at line#%d\n", __func__, __LINE__);
		kfree(afae_ctrl->map_table);
		kfree(afae_ctrl);
		afae_ctrl->map_table = NULL;
		afae_ctrl = NULL;
		return -ENOMEM;
	}

	set_focus_mode(CAMERA_FOCUS_CONTINUOUS_PICTURE);
	set_focus_state(FOCUS_STATE_STOPPED);
	set_focus_result(STATUS_FOCUSED);

	memset(&afae_ctrl->compare_data, 0, sizeof(focus_frame_stat_s));
	set_caf_forcestart(CAF_FORCESTART_FORCEWAIT);

	afae_ctrl->binning = 0;
	afae_ctrl->focus_failed = 0;

	memset(&afae_ctrl->cur_rect, 0, sizeof(camera_rect_s));

	/* init af start work queue. */
	af_start_work_queue = create_singlethread_workqueue("af_start_wq");
	if (!af_start_work_queue) {
		print_error("create workqueue is failed in %s function at line#%d\n", __func__, __LINE__);
		kfree(afae_ctrl);
		afae_ctrl = NULL;
		return -1;
	}

	ispv1_setreg_vcm_DLC_mode();
	sema_init(&sem_af_schedule, 0);
	INIT_WORK(&af_start_work, ispv1_af_start_work_func);

	ispv1_cal_vcm_range(vcm);
	ispv1_setvcm_init(vcm);

	ispv1_setreg_vcm_code(vcm->infiniteDistance);

	sema_init(&(afae_ctrl->af_run_sem), 1);
	return 0;
}

/*called by exit */
int ispv1_focus_exit(void)
{
	ispv1_focus_vcm_go_infinite(1);
	if (afae_ctrl->map_table) {
		kfree(afae_ctrl->map_table);
		afae_ctrl->map_table = NULL;
	}

	if (afae_ctrl->stat_data) {
		vfree(afae_ctrl->stat_data);
		afae_ctrl->stat_data = NULL;
	}

	destroy_workqueue(af_start_work_queue);

	if (afae_ctrl) {
		kfree(afae_ctrl);
		afae_ctrl = NULL;
	}

	return 0;
}

/* called by start_preview */
int ispv1_focus_prepare(void)
{
	print_debug("enter %s", __func__);

	set_focus_mode(CAMERA_FOCUS_CONTINUOUS_PICTURE);
	set_focus_state(FOCUS_STATE_STOPPED);
	afae_ctrl->af_area.focus_rect_num = 1;
	afae_ctrl->focus_failed = 0;
	afae_ctrl->k3focus_running = false;

	afae_ctrl->focus_stat_frames = 0;
	memset(&afae_ctrl->compare_data, 0, sizeof(focus_frame_stat_s));

	return 0;
}

/* called by stop_preview */
int ispv1_focus_withdraw(void)
{
	/* ispv1_wait_caf_done(); */
	ispv1_auto_focus(FOCUS_STOP);
	set_focus_state(FOCUS_STATE_STOPPED);
	flush_workqueue(af_start_work_queue);
	afae_ctrl->focus_failed = 0;
	afae_ctrl->af_area.focus_rect_num = 0;
	return 0;
}

static void ispv1_exit_focus_workqueue(camera_focus focus_mode)
{
	afae_ctrl->k3focus_running = false;
	ispv1_wakeup_focus_schedule(true);
	flush_workqueue(af_start_work_queue);
	if ((focus_mode != CAMERA_FOCUS_CONTINUOUS_VIDEO) &&
		(focus_mode != CAMERA_FOCUS_CONTINUOUS_PICTURE)) {
		ispv1_af_flash_check_close();
		ispv1_set_aecagc_mode(AUTO_AECAGC);
	}
}

int ispv1_auto_focus(int flag)
{
	int ret = 0;
	print_info("enter %s: flag: %d, focus state:%d, focus_mode:%d, result:%d",
		__func__, flag, get_focus_state(), get_focus_mode(), afae_ctrl->af_result);

	if(!this_ispdata->sensor->af_enable)
		return ret;

	if (ispv1_get_af_run_sem() == false) {
		return -1;
	}
	if (flag == FOCUS_START) {
		if (get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_VIDEO) {
			if (get_focus_state() == FOCUS_STATE_STOPPED) {
				set_focus_state(FOCUS_STATE_CAF_PREPARING);
				set_focus_result(STATUS_FOCUSED);
			} else {
				print_error("line %d: not valid status %d", __LINE__, get_focus_state());
				ret = -1;
				goto free_out;
			}
		} else if (get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_PICTURE) {
			if (get_focus_state() == FOCUS_STATE_STOPPED) {
				set_focus_state(FOCUS_STATE_CAF_PREPARING);
			} else {
				print_error("line %d: not valid status %d", __LINE__, get_focus_state());
				ret = -1;
				goto free_out;
			}
		} else if (get_focus_mode() == CAMERA_FOCUS_AUTO_VIDEO) {
			if (get_focus_state() == FOCUS_STATE_STOPPED) {
				set_focus_state(FOCUS_STATE_AF_PREPARING);
				set_focus_result(STATUS_FOCUSING);
			} else {
				print_error("line %d: not valid status %d", __LINE__, get_focus_state());
				ret = -1;
				goto free_out;
			}
		} else {
			/* if this time is snapshot focus,
			 * should goto AF_PREPARING state
			 */
			set_focus_state(FOCUS_STATE_AF_PREPARING);
			set_focus_result(STATUS_FOCUSING);

			afae_ctrl->area_changed = false;
		}
		queue_work(af_start_work_queue, &af_start_work);
		goto normal_out;
	} else if ((flag == FOCUS_STOP) && (get_focus_state() != FOCUS_STATE_STOPPED)) {

		if ((get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_PICTURE) ||
			(get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_VIDEO)) {
			if (get_focus_state() != FOCUS_STATE_CAF_DETECTING) {
				print_info("caf doesn't stoped");
				set_focus_result(STATUS_OUT_FOCUS);
			} else {
				set_focus_result(STATUS_FOCUSED);
			}
		} else {
			if (get_focus_result() == STATUS_FOCUSING) {
				print_info("auto focus timeout");
				set_focus_result(STATUS_OUT_FOCUS);
			}
		}
		ispv1_exit_focus_workqueue(get_focus_mode());
		set_focus_state(FOCUS_STATE_STOPPED);

		goto free_out;
	} else if (flag == FOCUS_CAF_FORCE_AF) {
		if ((get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_PICTURE) ||
			(get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_VIDEO)) {
			if (get_focus_state() != FOCUS_STATE_CAF_RUNNING) {
				set_focus_result(STATUS_FOCUSING);
				ispv1_exit_focus_workqueue(get_focus_mode());

				set_pre_focus_mode(get_focus_mode());
				if (get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_VIDEO){
					set_focus_mode(CAMERA_FOCUS_AUTO_VIDEO);
				} else {
					set_focus_mode(CAMERA_FOCUS_CAF_FORCE_AF);
				}
				set_focus_state(FOCUS_STATE_AF_PREPARING);
				queue_work(af_start_work_queue, &af_start_work);
				goto normal_out;
			} else {
				print_info("%s, CAF already running, must wait CAF done.", __func__);
				goto free_out;
			}
		} else {
			print_error("line %d: not valid status %d", __LINE__, get_focus_state());
			goto free_out;
		}
	} else if (flag == FOCUS_AF_RESUME_CAF) {
		if ((get_focus_mode() != CAMERA_FOCUS_CONTINUOUS_PICTURE) &&
			(get_focus_mode() != CAMERA_FOCUS_CONTINUOUS_VIDEO)) {
			set_focus_result(STATUS_OUT_FOCUS);
			ispv1_exit_focus_workqueue(get_focus_mode());

			set_caf_forcestart(CAF_FORCESTART_FORCE);
			set_focus_mode(get_pre_focus_mode());
			set_focus_state(FOCUS_STATE_CAF_PREPARING);
			queue_work(af_start_work_queue, &af_start_work);
			goto normal_out;
		} else {
			print_error("line %d: not valid status %d", __LINE__, get_focus_state());
			goto free_out;
		}
	} else {
		print_error("ispv1_auto_focus param error!");
		ret = -1;
		goto free_out;
	}

free_out:
	ispv1_free_af_run_sem();
normal_out:
	return ret;
}

static void ispv1_af_start_work_func(struct work_struct *work)
{
	camera_sensor *sensor =  this_ispdata->sensor;
	camera_focus cur_focus_mode = get_focus_mode();

	/* if focus state has changed, should return immediately. */
	if ((get_focus_state() != FOCUS_STATE_CAF_PREPARING) &&
		(get_focus_state() != FOCUS_STATE_AF_PREPARING))
		return;

	if (cur_focus_mode < CAMERA_FOCUS_MAX)
		ispv1_set_focus_mode_done(cur_focus_mode);

	ispv1_set_focus_area_done(&afae_ctrl->af_area, afae_ctrl->zoom);


	this_ispdata->af_need_flash = ispv1_af_flash_needed(sensor, this_ispdata->flash_mode, afae_ctrl->lum_info);


	if ((cur_focus_mode == CAMERA_FOCUS_CONTINUOUS_PICTURE)
		|| (cur_focus_mode== CAMERA_FOCUS_CONTINUOUS_VIDEO)){
		set_focus_state(FOCUS_STATE_CAF_DETECTING);

		if (afae_ctrl->area_changed == true)
			set_caf_forcestart(CAF_FORCESTART_FORCEWAIT);
		else
			ispv1_focus_status_reset();

		if (afae_ctrl->k3focus_running == false)
			ispv1_k3focus_run();
		else {
			print_error("fatal error in %s line %d: state error!", __func__, __LINE__);
			ispv1_free_af_run_sem();
		}
	} else if ((cur_focus_mode == CAMERA_FOCUS_CAF_FORCE_AF) &&
			(false == this_ispdata->af_need_flash) && (CAF_FORCESTART_CLEAR == get_caf_forcestart())) {
		print_info("%s, don't excute auto focus, af_need_flash:%d, force_start:0x%x",
			__func__, this_ispdata->af_need_flash, get_caf_forcestart());
		set_focus_result(STATUS_FOCUSED);
		set_focus_state(FOCUS_STATE_STOPPED);
		ispv1_free_af_run_sem();
	} else {
		set_focus_state(FOCUS_STATE_AF_RUNNING);
		set_focus_stage(AF_RUN_STAGE_PREPARE);

		if (afae_ctrl->k3focus_running == false) {
			ispv1_k3focus_run();
		} else {
			print_error("fatal error in %s line %d: state error!", __func__, __LINE__);
			ispv1_free_af_run_sem();
		}
	}
}

static int ispv1_get_merged_rect(focus_area_s *area, camera_rect_s *yuv_rect,
				u32 preview_width, u32 preview_height)
{
	u32 rect_index;
	camera_rect_s *cur_rect;

	yuv_rect->left = preview_width;
	yuv_rect->top = preview_height;
	yuv_rect->width = 0;
	yuv_rect->height = 0;

	for (rect_index = 0; rect_index < area->focus_rect_num; rect_index++) {
		cur_rect = &area->rect[rect_index];
		if (((cur_rect->left + cur_rect->width) > preview_width) ||
		    ((cur_rect->top + cur_rect->height) > preview_height)) {
			return -1;
		}

		if (cur_rect->left < yuv_rect->left)
			yuv_rect->left = cur_rect->left;
		if (cur_rect->top < yuv_rect->top)
			yuv_rect->top = cur_rect->top;

		if ((cur_rect->left + cur_rect->width) > (yuv_rect->left + yuv_rect->width))
			yuv_rect->width = (cur_rect->left + cur_rect->width) - yuv_rect->left;

		if ((cur_rect->top + cur_rect->height) > (yuv_rect->left + yuv_rect->height))
			yuv_rect->height = (cur_rect->top + cur_rect->height) - yuv_rect->top;
	}

	return 0;
}

static int ispv1_get_yuvrect_withzoom(camera_rect_s *yuv_rect, u32 zoom)
{
	u32 ratio = isp_zoom_to_ratio(zoom, 0);
	u32 centerx, centery;

	centerx = yuv_rect->left + yuv_rect->width / 2;
	centery = yuv_rect->top + yuv_rect->height / 2;

	yuv_rect->left = centerx - yuv_rect->width * ISP_ZOOM_BASE_RATIO / ratio / 2;
	yuv_rect->top = centery - yuv_rect->height * ISP_ZOOM_BASE_RATIO / ratio / 2;
	yuv_rect->width = yuv_rect->width * ISP_ZOOM_BASE_RATIO / ratio;	
	yuv_rect->height = yuv_rect->height * ISP_ZOOM_BASE_RATIO / ratio;

	return 0;
}

static int ispv1_get_focus_win_info(camera_rect_s *raw_rect,
				focus_win_info_s *win_info, int *binning)
{
	u32 width, height;

	width = raw_rect->width / 5;
	height = raw_rect->height / 5;

	if (width <= 126) {
		if (height <= 126) {
			*binning = 1;
		} else {
			height = 126;
			*binning = 1;
		}
	} else if (width <= 252) {
		if (height <= 126) {
			width = 126;
			*binning = 1;
		} else if (height <= 252) {
			*binning = 0;
		} else {
			height = 252;
			*binning = 0;
		}
	} else {
		if (height <= 126) {
			width = 126;
			*binning = 1;
		} else if (height <= 252) {
			width = 252;
			*binning = 0;
		} else {
			width = 252;
			height = 252;
			*binning = 0;
		}
	}

	win_info->left = raw_rect->left;
	win_info->top = raw_rect->top;
	win_info->width = width;
	win_info->height = height;
	win_info->width1 = raw_rect->width - width;
	win_info->height1 = raw_rect->height - height;

	return 0;
}

static void ispv1_get_raw_win(int index, focus_win_info_s *win_info,
			      camera_rect_s *raw_rect)
{
	raw_rect->left = win_info->left + (index % 5) * (win_info->width1 / 4);
	raw_rect->top = win_info->top + (index / 5) * (win_info->height1 / 4);
	raw_rect->width = win_info->width;
	raw_rect->height = win_info->height;
}

/*
 * 判断两个矩形的中心坐标的水平和垂直距离，
 * 只要这两个值满足某种条件就可以相交。
 * 矩形A的宽 Wa = Xa2-Xa1 高 Ha = Ya2-Ya1
 * 矩形B的宽 Wb = Xb2-Xb1 高 Hb = Yb2-Yb1
 * 矩形A的中心坐标 (Xa3,Ya3) = （ (Xa2+Xa1)/2 ，(Ya2+Ya1)/2 ）
 * 矩形B的中心坐标 (Xb3,Yb3) = （ (Xb2+Xb1)/2 ，(Yb2+Yb1)/2 ）
 * 所以只要同时满足下面两个式子，就可以说明两个矩形相交。
 * 1） | Xb3-Xa3 | <= Wa/2 + Wb/2
 * 2） | Yb3-Ya3 | <= Ha/2 + Hb/2
 * 即：
 * | Xb2+Xb1-Xa2-Xa1 | <= Xa2-Xa1 + Xb2-Xb1
 * | Yb2+Yb1-Ya2-Ya1 | <=Y a2-Ya1 + Yb2-Yb1
 *
 * return value: 0--intersection; -1--no intersection.
 */
static int ispv1_check_rect_intersection(camera_rect_s *rect1, camera_rect_s *rect2)
{
	coordinate_s center1, center2;

	center1.x = rect1->left + rect1->width / 2;
	center1.y = rect1->top + rect1->height / 2;

	center2.x = rect2->left + rect2->width / 2;
	center2.y = rect2->top + rect2->height / 2;

	if ((abs(center2.x-center1.x) <= (rect1->width / 2 + rect2->width / 2)) &&
	   (abs(center2.y-center1.y) <= (rect1->height / 2 + rect2->height / 2)))
		return 0;
	else
		return -1;
}


static bool ispv1_check_rect_center(camera_rect_s *rect, u32 preview_width, u32 preview_height)
{
	bool ret = false;
	coordinate_s center, rect_center;

	if (rect->width == 0 || rect->height == 0)
		return true;

	center.x = preview_width / 2;
	center.y = preview_height / 2;
	rect_center.x = rect->left + rect->width / 2;
	rect_center.y = rect->top + rect->height / 2;

	if (abs(rect_center.x - center.x) < 4 && abs(rect_center.y - center.y) < 4)
		ret = true;

	return ret;
}


static bool ispv1_check_rect_differ(camera_rect_s *rect1, camera_rect_s *rect2,
				   u32 preview_width, u32 preview_height)
{
	coordinate_s center1, center2;
	u32 width_diff, height_diff;

	center1.x = rect1->left + rect1->width / 2;
	center1.y = rect1->top + rect1->height / 2;

	center2.x = rect2->left + rect2->width / 2;
	center2.y = rect2->top + rect2->height / 2;

	/* if center pointer differ a lot, then it is differ. */
	if ((abs(center2.x - center1.x) > (preview_width / 16)) || (abs(center2.y - center1.y) > (preview_height / 16)))
		return true;

	/* if size is differ a lot, then it is differ. */
	width_diff = abs(rect1->width - rect2->width);
	height_diff = abs(rect1->height - rect2->height);
	if ((width_diff > rect1->width / 2) || (width_diff > rect2->width / 2)
	    || (height_diff > rect1->height / 2) || (height_diff > rect2->height / 2))
		return true;

	return false;
}

static int ispv1_get_map_table(focus_area_s *area,
				focus_win_info_s *win_info, int *map_table)
{
	u32 area_idx;
	u32 win_idx;
	camera_rect_s user_rect;
	camera_rect_s isp_rect;
	int ret;

	/* check every area,
	 * it is in which yuv win, maybe in several wins, maybe none
	 */
	for (area_idx = 0; area_idx < area->focus_rect_num; area_idx++) {
		print_debug("map table for region %d:", area_idx);

		/* get each yuv win */
		ret = k3_isp_yuvrect_to_rawrect(&area->rect[area_idx], &user_rect);
		if (ret) {
			print_error("%s:line %d error", __func__, __LINE__);
			return ret;
		}

		for (win_idx = 0; win_idx < ISP_MAX_FOCUS_WIN; win_idx++) {
			/* get each raw win */
			ispv1_get_raw_win(win_idx, win_info, &isp_rect);
			print_debug("isp_rect %d: %d,%d,%d,%d", win_idx,
				isp_rect.left, isp_rect.top, isp_rect.width, isp_rect.height);

			ret = ispv1_check_rect_intersection(&user_rect, &isp_rect);
			if (ret == 0) {
				*(map_table + area_idx * ISP_MAX_FOCUS_WIN + win_idx) = 1;
				win_info->weight[win_idx] = 1;
			} else
				*(map_table + area_idx * ISP_MAX_FOCUS_WIN + win_idx) = 0;
		}
	}

	return 0;
}

/* changed 2012-03-15 for zero size rect*/
static int ispv1_focus_get_default_yuvrect(camera_rect_s *rectin, u32 preview_width, u32 preview_height)
{
	u32 width_precent = DEFAULT_AF_WIDTH_PERCENT;
	u32 height_precent = DEFAULT_AF_HEIGHT_PERCENT;

	if(this_ispdata->sensor->get_af_param){
		width_precent = (u32)(this_ispdata->sensor->get_af_param(AF_WIDTH_PERCENT));
		height_precent = (u32)(this_ispdata->sensor->get_af_param(AF_HEIGHT_PERCENT));
	}

	rectin->left = preview_width * (100 - width_precent) / 200;
	rectin->top = preview_height * (100 - height_precent) / 200;
	rectin->width = preview_width * width_precent / 100;
	rectin->height = preview_height * height_precent / 100;

	return 0;
}
static int ispv1_focus_adjust_yuvrect(camera_rect_s *yuv, u32 preview_width, u32 preview_height, u32 zoom)
{
	u32 ratio = isp_zoom_to_ratio(zoom, 0);
	u32 width = yuv->width;
	u32 height = yuv->height;
	int left, top;
	u32 height_ratio;
	u32 max_hzoom_ratio;
	u32 width_precent = DEFAULT_AF_WIDTH_PERCENT;
	u32 height_precent = DEFAULT_AF_HEIGHT_PERCENT;
	u32 min_height_ratio = DEFAULT_AF_MIN_HEIGHT_RATIO;

	if(this_ispdata->sensor->get_af_param){
		width_precent = (u32)(this_ispdata->sensor->get_af_param(AF_WIDTH_PERCENT));
		height_precent = (u32)(this_ispdata->sensor->get_af_param(AF_HEIGHT_PERCENT));
		min_height_ratio = (u32)(this_ispdata->sensor->get_af_param(AF_MIN_HEIGHT_RATIO));
	}

	/* first re-size ratio to fit focus: 1x-4x change to 1x-2x. */
	ratio = (ratio - ISP_ZOOM_BASE_RATIO) * (ISP_FOCUS_ZOOM_MAX_RATIO - ISP_ZOOM_BASE_RATIO);
	ratio /= (ISP_ZOOM_MAX_RATIO - ISP_ZOOM_BASE_RATIO);
	ratio += ISP_ZOOM_BASE_RATIO;
	max_hzoom_ratio = min_height_ratio * (ISP_FOCUS_ZOOM_MAX_RATIO / ISP_ZOOM_BASE_RATIO);

	height_ratio = min_height_ratio * ratio / ISP_ZOOM_BASE_RATIO;

	/* adjust height ratio. */
	if (height_ratio < min_height_ratio)
		height_ratio = min_height_ratio;
	else if (height_ratio > min_height_ratio * ISP_FOCUS_ZOOM_MAX_RATIO / ISP_ZOOM_BASE_RATIO)
		height_ratio = max_hzoom_ratio;

	print_debug("before left:%d, top:%d, yuv->width:%d; yuv->height:%d, height_ratio 0x%x",
		yuv->left, yuv->top, yuv->width, yuv->height, height_ratio);

	/* first, check if edge is not valid, set to default. */
	if ((yuv->left > preview_width) || (yuv->top > preview_height) ||
		(yuv->width > preview_width) || (yuv->height > preview_height)) {
		ispv1_focus_get_default_yuvrect(yuv, preview_width, preview_height);
	} else if (((yuv->left + yuv->width) > preview_width) || ((yuv->top + yuv->height) > preview_height)) {
		if ((yuv->left + yuv->width) > preview_width) {
			yuv->width -= ((yuv->left + yuv->width) - preview_width);
		}
		if ((yuv->top + yuv->height) > preview_height) {
			yuv->height -= ((yuv->top + yuv->height) - preview_height);
		}
	}

	/* second, check if width/height is larger than default or not */
	width = yuv->width;
	height = yuv->height;
	if (yuv->width > (preview_width * width_precent / 100))
		width = preview_width * width_precent / 100;
	if (yuv->height > (preview_height * height_precent / 100))
		height = preview_height * height_precent / 100;

	yuv->left = yuv->left + ((int)yuv->width - (int)width) / 2;
	yuv->top = yuv->top + ((int)yuv->height - (int)height) / 2;
	yuv->width = width;
	yuv->height = height;

	/* avoid zero divide error. */
	if (yuv->height <= 0)
		yuv->height = 1;

	/* third, check if rect is too small */
	if (yuv->height < (preview_height / height_ratio)) {
		width = yuv->width * (preview_height / height_ratio) / yuv->height;
		height = preview_height / height_ratio;

		if (width > (height * 3 / 2)) {
			width =height * 3 / 2;
		} else if (width < (height * 2 / 3)) {
			width = height * 2 / 3;
		}

		left = yuv->left - ((int)width - (int)yuv->width) / 2;
		top = yuv->top - ((int)height - (int)yuv->height) / 2;

		if (left < 0) {
			/* left is out of valid area */
			yuv->left = 0;
		} else if ((left + width) > preview_width) {
			/* right is larger than preview width */
			yuv->left = preview_width - width;
		} else {
			yuv->left = left;
		}

		if (top < 0) {
			/* top is out of valid area */
			yuv->top = 0;
		} else if ((top + height) > preview_height) {
			/* buttom is larger than preview height */
			yuv->top = preview_height - height;
		} else {
			yuv->top = top;
		}

		print_info("%s, focus rect from [%d X %d]->[%d,%d, %d X %d]",
			__func__, yuv->width, yuv->height, yuv->left, yuv->top, width, height);
		yuv->height = height;
		yuv->width = width;
	}
	print_debug("after left:%d, top:%d, yuv->width:%d; yuv->height:%d",
		yuv->left, yuv->top, yuv->width, yuv->height);
	return 0;
}

static int ispv1_focus_adjust_rawwin(focus_win_info_s *win_info, u32 raw_width, u32 raw_height)
{
	if (win_info->left < 24) {
		win_info->left = 24;
		if (win_info->width > 72) {
			win_info->width -= 24;
		} else {
			win_info->width = 48;
		}
	}

	if (win_info->top < 24) {
		win_info->top = 24;
		if (win_info->height > 72) {
			win_info->height -= 24;
		} else {
			win_info->height = 48;
		}
	}

	if ((win_info->top + win_info->height) > raw_height)
		win_info->top -= (win_info->top + win_info->height) - raw_height;

	if ((win_info->left + win_info->width) > raw_width)
		win_info->left -= (win_info->left + win_info->width) - raw_width;

	/* win startx+starty should be odd. */
	if ((win_info->left + win_info->top) % 2 == 0) {
		win_info->left += 1;
	}

	/* width and height should be mutiple of 6 */
	win_info->width -= (win_info->width % 6);
	win_info->height -= (win_info->height % 6);

	return 0;
}

#define ISP_LUM_WIN_CENTER_X(width, win_id) \
	((width) / ISP_LUM_WIN_WIDTH_NUM * ((win_id) % ISP_LUM_WIN_WIDTH_NUM) \
	+ (width) / ISP_LUM_WIN_WIDTH_NUM / 2)

#define ISP_LUM_WIN_CENTER_Y(height, win_id) \
	((height) / ISP_LUM_WIN_HEIGHT_NUM * ((win_id) / ISP_LUM_WIN_WIDTH_NUM) \
	+ (height) / ISP_LUM_WIN_HEIGHT_NUM / 2)

static inline u32 ispv1_get_distance_to_lumwin(coordinate_s *curr, coordinate_s *lum_win)
{
	u32 x_coff, y_coff;

	x_coff = abs((int)curr->x - (int)lum_win->x);
	x_coff *= x_coff;

	y_coff = abs((int)curr->y - (int)lum_win->y);
	y_coff *= y_coff;

	return (x_coff + y_coff);
}


void ispv1_focus_get_lumwin_info(focus_win_info_s *win_info, u32 raw_width, u32 raw_height, lum_win_info_s *lum_info)
{
	coordinate_s center;
	coordinate_s lumwin_center;
	u32 distance;
	u32 saved_distance[4];
	int loop;

	saved_distance[0] = 0xffff;
	saved_distance[1] = 0xffff;
	saved_distance[2] = 0xffff;
	saved_distance[3] = 0xffff;

	lum_info->index[0] = 0;
	lum_info->index[1] = 0;
	lum_info->index[2] = 0;
	lum_info->index[3] = 0;

	center.x = win_info->left + win_info->width / 2;
	center.y = win_info->top + win_info->height / 2;

	for (loop = 0; loop < (ISP_LUM_WIN_WIDTH_NUM * ISP_LUM_WIN_HEIGHT_NUM); loop++) {
		lumwin_center.x = ISP_LUM_WIN_CENTER_X(raw_width, loop);
		lumwin_center.y = ISP_LUM_WIN_CENTER_Y(raw_height, loop);

		distance = ispv1_get_distance_to_lumwin(&center, &lumwin_center);
		if (distance < saved_distance[0]) {
			saved_distance[3] = saved_distance[2];
			lum_info->index[3] = lum_info->index[2];

			saved_distance[2] = saved_distance[1];
			lum_info->index[2] = lum_info->index[1];

			saved_distance[1] = saved_distance[0];
			lum_info->index[1] = lum_info->index[0];

			saved_distance[0] = distance;
			lum_info->index[0] = loop;
		} else if (distance < saved_distance[1]) {
			saved_distance[3] = saved_distance[2];
			lum_info->index[3] = lum_info->index[2];

			saved_distance[2] = saved_distance[1];
			lum_info->index[2] = lum_info->index[1];

			saved_distance[1] = distance;
			lum_info->index[1] = loop;
		} else if (distance < saved_distance[2]) {
			saved_distance[3] = saved_distance[2];
			lum_info->index[3] = lum_info->index[2];

			saved_distance[2] = distance;
			lum_info->index[2] = loop;
		} else if (distance < saved_distance[3]) {
			saved_distance[3] = distance;
			lum_info->index[3] = loop;
		}
	}

	lum_info->width = (raw_width / 8);
	lum_info->height = (raw_height / 6);
}

static lum_table_s lum_table[] = {
	{256, 20480},{255, 20465},{254, 20451},{253, 20436},{252, 20421},{251, 20407},{250, 20392},{249, 20377},
	{248, 20362},{247, 20347},{246, 20332},{245, 20317},{244, 20302},{243, 20287},{242, 20272},{241, 20256},
	{240, 20241},{239, 20226},{238, 20210},{237, 20195},{236, 20179},{235, 20163},{234, 20148},{233, 20132},
	{232, 20116},{231, 20100},{230, 20084},{229, 20068},{228, 20052},{227, 20035},{226, 20019},{225, 20003},
	{224, 19986},{223, 19970},{222, 19953},{221, 19937},{220, 19920},{219, 19903},{218, 19886},{217, 19869},
	{216, 19852},{215, 19835},{214, 19818},{213, 19800},{212, 19783},{211, 19766},{210, 19748},{209, 19730},
	{208, 19713},{207, 19695},{206, 19677},{205, 19659},{204, 19641},{203, 19623},{202, 19605},{201, 19586},
	{200, 19568},{199, 19549},{198, 19531},{197, 19512},{196, 19493},{195, 19474},{194, 19455},{193, 19436},
	{192, 19417},{191, 19398},{190, 19378},{189, 19359},{188, 19339},{187, 19320},{186, 19300},{185, 19280},
	{184, 19260},{183, 19240},{182, 19219},{181, 19199},{180, 19179},{179, 19158},{178, 19137},{177, 19117},
	{176, 19096},{175, 19075},{174, 19053},{173, 19032},{172, 19011},{171, 18989},{170, 18968},{169, 18946},
	{168, 18924},{167, 18902},{166, 18880},{165, 18857},{164, 18835},{163, 18812},{162, 18790},{161, 18767},
	{160, 18744},{159, 18720},{158, 18697},{157, 18674},{156, 18650},{155, 18626},{154, 18602},{153, 18578},
	{152, 18554},{151, 18530},{150, 18505},{149, 18481},{148, 18456},{147, 18431},{146, 18405},{145, 18380},
	{144, 18355},{143, 18329},{142, 18303},{141, 18277},{140, 18250},{139, 18224},{138, 18197},{137, 18170},
	{136, 18143},{135, 18116},{134, 18089},{133, 18061},{132, 18033},{131, 18005},{130, 17977},{129, 17948},
	{128, 17920},{127, 17891},{126, 17861},{125, 17832},{124, 17802},{123, 17772},{122, 17742},{121, 17712},
	{120, 17681},{119, 17650},{118, 17619},{117, 17588},{116, 17556},{115, 17524},{114, 17492},{113, 17459},
	{112, 17426},{111, 17393},{110, 17360},{109, 17326},{108, 17292},{107, 17258},{106, 17223},{105, 17188},
	{104, 17153},{103, 17117},{102, 17081},{101, 17045},{100, 17008},{99 , 16971},{98 , 16933},{97 , 16895},
	{96 , 16857},{95 , 16818},{94 , 16779},{93 , 16740},{92 , 16700},{91 , 16659},{90 , 16619},{89 , 16577},
	{88 , 16536},{87 , 16493},{86 , 16451},{85 , 16408},{84 , 16364},{83 , 16320},{82 , 16275},{81 , 16230},
	{80 , 16184},{79 , 16137},{78 , 16090},{77 , 16042},{76 , 15994},{75 , 15945},{74 , 15896},{73 , 15845},
	{72 , 15795},{71 , 15743},{70 , 15690},{69 , 15637},{68 , 15583},{67 , 15529},{66 , 15473},{65 , 15417},
	{64 , 15360},{63 , 15301},{62 , 15242},{61 , 15182},{60 , 15121},{59 , 15059},{58 , 14996},{57 , 14932},
	{56 , 14866},{55 , 14800},{54 , 14732},{53 , 14663},{52 , 14593},{51 , 14521},{50 , 14448},{49 , 14373},
	{48 , 14297},{47 , 14219},{46 , 14140},{45 , 14059},{44 , 13976},{43 , 13891},{42 , 13804},{41 , 13715},
	{40 , 13624},{39 , 13530},{38 , 13434},{37 , 13336},{36 , 13235},{35 , 13130},{34 , 13023},{33 , 12913},
	{32 , 12800},{31 , 12682},{30 , 12561},{29 , 12436},{28 , 12306},{27 , 12172},{26 , 12033},{25 , 11888},
	{24 , 11737},{23 , 11580},{22 , 11416},{21 , 11244},{20 , 11064},{19 , 10874},{18 , 10675},{17 , 10463},
	{16 , 10240},{15 , 10001},{14 , 9746 },{13 , 9473 },{12 , 9177 },{11 , 8856 },{10 , 8504 },{9  , 8115 },
	{8  , 7680 },{7  , 7186 },{6  , 6617 },{5  , 5944 },{4  , 5120 },{3  , 4057 },{2  , 2560 },{1  , 0    },

};

/*
 **************************************************************************
 * FunctionName: ispv1_convert_win_lum;
 * Description : NA;
 * Input       : stat_value:luminance stat value
 *			lum_tbl:luminance table,
 *			size:table size;
 * Output      : NA;
 * ReturnValue : luminance value;
 * Other       : NA;
 **************************************************************************
 */
u32 ispv1_convert_win_lum(u32 stat_value, lum_table_s *lum_tbl, u32 size)
{
	int top = 0;
	int bottom = size - 1;
	int  middle;

	while (abs(top - bottom) > 1) {
		middle = (top + bottom) / 2;
		if (stat_value >= lum_tbl[middle].value)
			bottom = middle;
		else
			top = middle;
	}

	return lum_tbl[top].lum;
}

/*
 **************************************************************************
 * FunctionName: ispv1_get_raw_lum_info;
 * Description : NA;
 * Input       : aec_data_t *ae_data, stat_unit_area:area of minimum luminance stat window;
 * Output      : lum_max, lum_sum;
 * ReturnValue : void;
 * Other       : NA;
 **************************************************************************
 */
void ispv1_get_raw_lum_info(aec_data_t *ae_data, u32 stat_unit_area)
{
	u32 win_idx;
	u32 width_idx, height_idx;
	u32 lum;
	u32 lum_max = 0;
	u32 idx_max = 0;
	u32 lum_sum = 0;

	for (width_idx = 2; width_idx < ISP_LUM_WIN_WIDTH_NUM - 2; width_idx++) {
		for (height_idx = 1; height_idx < ISP_LUM_WIN_HEIGHT_NUM - 1; height_idx++) {
			win_idx = height_idx * ISP_LUM_WIN_WIDTH_NUM + width_idx;
			/* get each raw win raw */
			lum = ispv1_get_single_win_raw_lum(win_idx, stat_unit_area);

			/* reflash max luminance */
			if (lum_max < lum) {
				lum_max = lum;
				idx_max = win_idx;
			}
			/* reflash sum of raw data */
			lum_sum += lum;
		}
	}

	ae_data->lum_max = lum_max;
	ae_data->lum_sum = lum_sum;

	print_debug("%s, idx_max:%d, lum_max:0x%x, lum_sum:0x%x, current_y:0x%x",
		__func__, idx_max, lum_max, lum_sum, get_current_y());
}

u32 ispv1_focus_get_win_lum(lum_win_info_s *lum_info)
{
	u32 lum[4];
	u32 lum_sum;

	lum[0]= get_win_lum(lum_info->index[0]); lum[0] /= (lum_info->width * lum_info->height);
	lum[1]= get_win_lum(lum_info->index[1]); lum[1] /= (lum_info->width * lum_info->height);
	lum[2]= get_win_lum(lum_info->index[2]); lum[2] /= (lum_info->width * lum_info->height);
	lum[3]= get_win_lum(lum_info->index[3]); lum[3] /= (lum_info->width * lum_info->height);

	lum_sum = lum[0] + lum[1] + lum[2] + lum[3];
	lum_sum /= 4;

	return(ispv1_convert_win_lum(lum_sum, lum_table, ARRAY_SIZE(lum_table)));
}

/*
 **************************************************************************
 * FunctionName: ispv1_get_single_win_raw_lum;
 * Description : NA;
 * Input       : win_idx:index of luminance window,
 *			stat_unit_area:area of minimum luminance stat window;
 * Output      : NA;
 * ReturnValue : lum:luminance of minimum luminance stat window;
 * Other       : NA;
 **************************************************************************
 */
u32 ispv1_get_single_win_raw_lum(u8 win_idx, u32 stat_unit_area)
{
	u32 stat_value;
	u32 lum = 0;

	stat_value = get_win_lum(win_idx)  / stat_unit_area;
	lum = ispv1_convert_win_lum(stat_value, lum_table, ARRAY_SIZE(lum_table));

	return lum;
}


int ispv1_set_vcm_parameters(camera_focus focus_mode)
{
	int ret = 0;
	vcm_info_s *vcm = get_vcm_ptr();

	print_debug("enter %s, focus_mode:%d", __func__, focus_mode);

	/*
	 * should update following params, such as:
	 * offsetInit/fullRange/coarseStep/fineStep/frameRate
	 */
	switch (focus_mode) {
	case CAMERA_FOCUS_AUTO:
	case CAMERA_FOCUS_MACRO:
	case CAMERA_FOCUS_CONTINUOUS_PICTURE:
	case CAMERA_FOCUS_CAF_FORCE_AF:
		vcm->offsetInit = vcm->infiniteDistance;
		vcm->fullRange = vcm->normalDistanceEnd;
		vcm->moveRange = RANGE_NORMAL;
		break;

	case CAMERA_FOCUS_INFINITY:
		vcm->offsetInit = vcm->infiniteDistance;
		vcm->fullRange = vcm->infiniteDistance + vcm->normalStep * 2;
		vcm->moveRange = RANGE_INFINITY;
		break;

	case CAMERA_FOCUS_CONTINUOUS_VIDEO:
	case CAMERA_FOCUS_AUTO_VIDEO:
		vcm->offsetInit = vcm->infiniteDistance;
		vcm->fullRange = vcm->videoDistanceEnd;
		vcm->moveRange = RANGE_NORMAL;
		break;
	case CAMERA_FOCUS_FIXED:
		break;
	case CAMERA_FOCUS_EDOF:
		ret = -1;
		print_error("focus mode not supported: %d", focus_mode);
		break;
	default:
		ret = -1;
		print_error("focus range mode unknow: %d", focus_mode);
		break;
	}

	if (ret == -1)
		return ret;

	if (CAMERA_FOCUS_CONTINUOUS_VIDEO == focus_mode) {
		vcm->coarseStep = vcm->videoStep;
	} else if ((RANGE_NORMAL == vcm->moveRange) || (RANGE_MACRO == vcm->moveRange)) {
		vcm->coarseStep = vcm->normalStep;
	} else if (RANGE_INFINITY == vcm->moveRange) {
		vcm->coarseStep = vcm->normalStep;
	}

	vcm->fineStep = vcm->coarseStep / 2;

	return ret;
}


int ispv1_set_focus_mode(camera_focus focus_mode)
{
	print_info("Enter %s, focus_mode:%d", __func__, focus_mode);

	if(!this_ispdata->sensor->af_enable)
		return 0;

	ispv1_set_focus_range(focus_mode);
	set_focus_mode(focus_mode);
	return 0;
}

/*
 **************************************************************************
 * FunctionName: ispv1_set_focus_mode_done;
 * Description : called by ispv1_auto_focus();
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int ispv1_set_focus_mode_done(camera_focus focus_mode)
{
	int ret = 0;
	camera_sensor *sensor = this_ispdata->sensor;

	print_info("enter %s, focus mode %d", __func__, focus_mode);

	if (!sensor->af_enable) {
		print_error("This sensor not support AF!");
		return -1;
	}
	ret = ispv1_set_vcm_parameters(focus_mode);

	if (ret == -1)
		return ret;

	if ((focus_mode == CAMERA_FOCUS_CONTINUOUS_VIDEO) ||
	    (focus_mode == CAMERA_FOCUS_CONTINUOUS_PICTURE))
		ret = ispv1_setreg_af_mode(CAMERA_VIDEO_SERVO_MODE);
	else if (afae_ctrl->multi_win)
		ret = ispv1_setreg_af_mode(CAMERA_WEIGHTED_SNAPSHOT_MODE);
	else
		ret = ispv1_setreg_af_mode(CAMERA_SINGLE_SNAPSHOT_MODE);

	if (ret) {
		print_error("%s:line %d error", __func__, __LINE__);
		return -EINVAL;
	}

	return ret;
}

int ispv1_set_focus_zoom(u32 zoom)
{
	ispv1_set_focus_area(&afae_ctrl->af_area, zoom);
	afae_ctrl->zoom = zoom;
	return 0;
}

int ispv1_set_sharpness_zoom(u32 zoom)
{
	camera_sensor *sensor = this_ispdata->sensor;
	u32 max_sharpness;
	u32 min_sharpness;
	u32 sharpness;
	u32 ratio;

	if (sensor->get_override_param != NULL) {
		max_sharpness = sensor->get_override_param(OVERRIDE_SHARPNESS_PREVIEW);
		min_sharpness = sensor->get_override_param(OVERRIDE_SHARPNESS_CAPTURE);
	} else {
		max_sharpness = CAMERA_SHARPNESS_PREVIEW;
		min_sharpness = CAMERA_SHARPNESS_CAPTURE;
	}

	ratio = isp_zoom_to_ratio(zoom, 0);
	print_debug("enter %s, ratio: 0x%0x", __func__, ratio);

	sharpness = max_sharpness - (max_sharpness - min_sharpness) \
			* (ratio - ISP_ZOOM_BASE_RATIO) / 0x300;

	SETREG8(REG_ISP_SHARPNESS, sharpness);

	return 0;
}

static bool ispv1_check_focus_area_changed(camera_rect_s *rect1, camera_rect_s *rect2)
{
	if ((rect1->left != rect2->left) || (rect1->top != rect2->top) ||
		(rect1->width != rect2->width) || (rect1->height != rect2->height))
		return true;
	else
		return false;
}

/*
 * ispv1_set_focus_area just save focus area
 */
int ispv1_set_focus_area(focus_area_s *area, u32 zoom)
{
	camera_rect_s *current_rect;
	camera_rect_s *previous_rect;
	u32 preview_width = this_ispdata->pic_attr[STATE_PREVIEW].out_width;
	u32 preview_height = this_ispdata->pic_attr[STATE_PREVIEW].out_height;
	bool ret = false;

	print_debug("enter %s", __func__);

	/* y36721 0229 add */
	if (get_focus_state() == FOCUS_STATE_CAF_RUNNING ||
		get_focus_state() == FOCUS_STATE_CAF_DETECTING) {
		/*
		 * If caf is running and CAF area changed,
		 * such as face is detected,
		 * should change focus area and force start.
		 */

		/*If it is CAF and new area is differ a little with previous,
		 * should ommit it
		 */
#ifndef AF_SINGLE_WINMODE_ONLY
		current_rect = &area->rect[0];
		previous_rect = &afae_ctrl->af_area.rect[0];
#else
		current_rect = &area->rect[area->focus_rect_num - 1];
			previous_rect = &afae_ctrl->af_area.rect[area->focus_rect_num - 1];
#endif

		if (zoom == afae_ctrl->zoom) {
			//ret = ispv1_check_rect_differ(current_rect, previous_rect, preview_width, preview_height);
			ret = ispv1_check_focus_area_changed(current_rect, previous_rect);
			if (ret == false) {
				/* if focus area not changed, return directly. */
				print_info("CAF rect not change, ommit it");
				return 0;
			}
		}

		ret = ispv1_check_rect_center(current_rect, preview_width, preview_height);
		if (ret == true) {
			/* new focus area is center focus. */
			if (get_focus_state() == FOCUS_STATE_CAF_RUNNING)
				set_focus_stage(AF_RUN_STAGE_BREAK);
			set_caf_forcestart(CAF_FORCESTART_FORCEWAIT);
		} else {
			/* if large differ flag is set and CAF is running, should break current focusing and restart immediately */
			ret = ispv1_check_rect_differ(current_rect, previous_rect, preview_width, preview_height);
			if (ret == true) {
				if (get_focus_state() == FOCUS_STATE_CAF_RUNNING)
					set_focus_stage(AF_RUN_STAGE_BREAK);
				set_caf_forcestart(CAF_FORCESTART_FORCE);
			} else {
				/*very small change, do nothing */
				print_info("CAF rect change little, ommit it");
				return 0;
			}
		}

		ispv1_set_focus_area_done(area, zoom);
	}

	memcpy(&afae_ctrl->af_area, area, sizeof(focus_area_s));

	return 0;
}


static int ispv1_set_focus_area_done(focus_area_s *area, u32 zoom)
{
	u32 preview_width = this_ispdata->pic_attr[STATE_PREVIEW].out_width;
	u32 preview_height = this_ispdata->pic_attr[STATE_PREVIEW].out_height;
	u32 raw_width = this_ispdata->pic_attr[STATE_PREVIEW].in_width;
	u32 raw_height = this_ispdata->pic_attr[STATE_PREVIEW].in_height;
	camera_rect_s ori_rect, cur_rect, raw_rect;
	/*camera_rect_s merged_yuv_rect;*/
	focus_win_info_s win_info;
	int binning = 0;
	int multi_win = 0;
	int index;
	int ret;

	/*int *map_table = afae_ctrl->map_table;*/

	print_debug("enter %s", __func__);

	print_debug("focus_area: focus_area_num %d", area->focus_rect_num);
	for (index = 0; index < area->focus_rect_num; index++) {
		print_debug("focus rect %d:%d,%d,%d,%d,%d", index,
					area->rect[index].left,
					area->rect[index].top,
					area->rect[index].width,
					area->rect[index].height,
					area->rect[index].weight);
	}

	memset(&win_info, 0, sizeof(focus_win_info_s));

	/*y36721 changed for supporting one windows only. */
#ifndef AF_SINGLE_WINMODE_ONLY
	multi_win = (area->focus_rect_num > 1) ? 1 : 0;
#else
	multi_win = 0;
#endif

	print_debug("%s, line %d: multi_win %d", __func__, __LINE__, multi_win);

	if (multi_win == 0) {
		/*y36721 changed for supporting one windows only.*/
		#ifndef AF_SINGLE_WINMODE_ONLY
			memcpy(&ori_rect, &area->rect[0], sizeof(camera_rect_s));
		#else
			memcpy(&ori_rect, &area->rect[area->focus_rect_num - 1], sizeof(camera_rect_s));
		#endif

		/* check width and height are valid, then adjust it. */
		if (ori_rect.width == 0 || ori_rect.height == 0) {
			ispv1_focus_get_default_yuvrect(&ori_rect, preview_width, preview_height);
			print_debug("default yuv rect:%d,%d,%d,%d",
			    ori_rect.left, ori_rect.top, ori_rect.width, ori_rect.height);
		}

		/* most case is just one focus rect. */
		if ((ori_rect.left + ori_rect.width) > preview_width || (ori_rect.top + ori_rect.height) > preview_height) {
			print_error("%s, line %d: rect area error!", __func__, __LINE__);
			return -1;
		}

		ispv1_get_yuvrect_withzoom(&ori_rect, zoom);
		memcpy(&cur_rect, &ori_rect, sizeof(camera_rect_s));

		/* convert yuv rect to raw rect. */
		ispv1_focus_adjust_yuvrect(&cur_rect, preview_width, preview_height, zoom);
		print_info("Focus area adjust: zoom %d, [%d,%d:%d x %d]->[%d,%d:%d x %d]",
			zoom, ori_rect.left, ori_rect.top, ori_rect.width, ori_rect.height,
			cur_rect.left, cur_rect.top, cur_rect.width, cur_rect.height);

		ret = k3_isp_yuvrect_to_rawrect(&cur_rect, &raw_rect);
		if (ret) {
			print_error("%s, line %d: error", __func__, __LINE__);
			return ret;
		}

		win_info.left = raw_rect.left;
		win_info.top = raw_rect.top;
		win_info.width = raw_rect.width;
		win_info.height = raw_rect.height;
		win_info.width1 = 0;
		win_info.height1 = 0;

		print_debug("win_info before %d:%d:%d:%d", win_info.left, win_info.top, win_info.width, win_info.height);

		goto setreg_out;
	}
#ifndef AF_SINGLE_WINMODE_ONLY
	/*
	 * Get a YUV area include all of user defined rects
	 * If there is any rect is out of range, then return false.
	 */
	ret = ispv1_get_merged_rect(area, &merged_yuv_rect, preview_width, preview_height);
	if (ret) {
		print_error("%s:line %d error", __func__, __LINE__);
		return ret;
	}

	ispv1_get_yuvrect_withzoom(&merged_yuv_rect, zoom);

	ispv1_focus_adjust_yuvrect(&merged_yuv_rect, preview_width, preview_height, zoom);
	/* convert yuv rect to raw rect. */
	ret = k3_isp_yuvrect_to_rawrect(&merged_yuv_rect, &raw_rect);
	if (ret) {
		print_error("%s:line %d error", __func__, __LINE__);
		return ret;
	}

	/* caculate ISP focus windows information, include binning flag */
	ret = ispv1_get_focus_win_info(&raw_rect, &win_info, &binning);
	if (ret) {
		print_error("%s:line %d error", __func__, __LINE__);
		return ret;
	}

	/* Map user defined rects to ISP defined rects */
	ret = ispv1_get_map_table(area, &win_info, map_table);
	if (ret) {
		print_error("%s:line %d error", __func__, __LINE__);
		return ret;
	}
#endif

setreg_out:
	afae_ctrl->binning = binning;
	afae_ctrl->multi_win = multi_win;

	/*Added by y36721 for adjust focus windows 2012-02-16.*/
	ispv1_focus_adjust_rawwin(&win_info, raw_width, raw_height);
	print_debug("win_info after %d:%d:%d:%d", win_info.left, win_info.top, win_info.width, win_info.height);

	/* Set ISP defined rects to ISP register */
	ispv1_setreg_focus_win(&win_info);

	ispv1_focus_get_lumwin_info(&win_info, raw_width, raw_height, &afae_ctrl->lum_info);

	afae_ctrl->area_changed = ispv1_check_focus_area_changed(&afae_ctrl->cur_rect, &cur_rect);
	print_info("######afae_ctrl->area_changed %d, lum_info[%d,%d,%d,%d]######",\
		afae_ctrl->area_changed,\
		afae_ctrl->lum_info.index[0],\
		afae_ctrl->lum_info.index[1],\
		afae_ctrl->lum_info.index[2],\
		afae_ctrl->lum_info.index[3]);

	memcpy(&afae_ctrl->cur_rect, &cur_rect, sizeof(camera_rect_s));

	return 0;
}

/*
 **************************************************************************
 * FunctionName: ispv1_get_focus_result;
 * Description : get focus result
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : STATUS_FOCUSING, STATUS_FOCUSED, STATUS_OUT_FOCUS
 * Other       : NA;
 **************************************************************************
 */
int ispv1_get_focus_result(focus_result_s *result)
{
	u32 win_idx = 0;
	u32 rect_idx = 0;
	int status;
	focused_result_s isp_result;
	int ret = 0;

	focus_area_s *area;
	int *map_table;

	print_debug("enter %s", __func__);

	if(!this_ispdata->sensor->af_enable) {
		return 0;
	}

	area = &afae_ctrl->af_area;
	map_table = afae_ctrl->map_table;

	assert(result);

	memset(result, 0, sizeof(focus_result_s));
	afae_ctrl->focus_failed = 0;

	status = ispv1_getreg_focus_result(&isp_result, afae_ctrl->multi_win);
	if (status != STATUS_FOCUSED) {
		result->status = status;

		/* added reason: if last time out focus, then maybe also not run in continus mode */
		if ((result->status == STATUS_OUT_FOCUS) && (get_focus_mode() == CAMERA_FOCUS_AUTO)) {
			afae_ctrl->focus_failed = 1;
		}

		ret = -1;
		goto out;
	}

	/* If it is focused and single win mode, then just set status and each_status[0]. */
	if (afae_ctrl->multi_win == 0) {
		result->status = STATUS_FOCUSED;

		/*y36721 changed for supporting one windows only.*/
		#ifndef AF_SINGLE_WINMODE_ONLY
			result->each_status[0] = STATUS_FOCUSED;
		#else
			for (win_idx = 0; win_idx < isp_result.focused_win_num; win_idx++) {
				result->each_status[win_idx] = STATUS_FOCUSED;
			}
		#endif
	} else {
		/*
		 * If it is focused and multi win mode,
		 * then search map table and set result.
		 * use ISP defined win id to get user defined rect id.
		 */
		result->status = STATUS_FOCUSED;
		for (win_idx = 0; win_idx < isp_result.focused_win_num; win_idx++) {
			for (rect_idx = 0; rect_idx < area->focus_rect_num; rect_idx++) {
				if (*(map_table + rect_idx * ISP_MAX_FOCUS_WIN + isp_result.focused_win[win_idx]) == 1) {
					result->each_status[rect_idx] = STATUS_FOCUSED;
				}
			}
		}
	}

out:
	return ret;
}

/*
 * Default metering rect is like that:
 * SPOT is rect of quarter width and quarter height
 * CWA is  rect of half width and half height
 * AVERAGE is all of preview region;
 */
static int ispv1_get_default_metering_rect(camera_metering metering,
				camera_rect_s *yuv, u32 out_width, u32 out_height)
{
	int retvalue = 0;
	camera_sensor *sensor = this_ispdata->sensor;//y00231328 0311 change to set ae statwin

	print_debug("enter %s", __func__);

	switch (metering) {
	case CAMERA_METERING_SPOT:
		if(sensor->sensor_index == CAMERA_SENSOR_SECONDARY){
			yuv->width = out_width * METERING_SPOT_WIDTH_PERCENT_FOR_SECONDARY_CAMERA / 100;
			yuv->height = out_height * METERING_SPOT_HEIGHT_PERCENT_FOR_SECONDARY_CAMERA / 100;
			yuv->left = out_width * (100 - METERING_SPOT_WIDTH_PERCENT_FOR_SECONDARY_CAMERA) / 200;
			yuv->top = out_height * (100 - METERING_SPOT_HEIGHT_PERCENT_FOR_SECONDARY_CAMERA) / 200;
		} else {
			yuv->width = out_width * METERING_SPOT_WIDTH_PERCENT / 100;
			yuv->height = out_height * METERING_SPOT_HEIGHT_PERCENT / 100;
			yuv->left = out_width * (100 - METERING_SPOT_WIDTH_PERCENT) / 200;
			yuv->top = out_height * (100 - METERING_SPOT_HEIGHT_PERCENT) / 200;
		}
		break;

	case CAMERA_METERING_CWA:
		if(sensor->sensor_index == CAMERA_SENSOR_SECONDARY){
			yuv->width = out_width * METERING_CWA_WIDTH_PERCENT_FOR_SECONDARY_CAMERA / 100;
			yuv->height = out_height * METERING_CWA_HEIGHT_PERCENT_FOR_SECONDARY_CAMERA / 100;
			yuv->left = out_width * (100 - METERING_CWA_WIDTH_PERCENT_FOR_SECONDARY_CAMERA) / 200;
			yuv->top = out_height * (100 - METERING_CWA_HEIGHT_PERCENT_FOR_SECONDARY_CAMERA) / 200;
		} else {
			yuv->width = out_width * METERING_CWA_WIDTH_PERCENT / 100;
			yuv->height = out_height * METERING_CWA_HEIGHT_PERCENT / 100;
			yuv->left = out_width * (100 - METERING_CWA_WIDTH_PERCENT) / 200;
			yuv->top = out_height * (100 - METERING_CWA_HEIGHT_PERCENT) / 200;
		}
		break;

	case CAMERA_METERING_AVERAGE:
		yuv->width = out_width;
		yuv->height = out_height;
		yuv->left = 0;
		yuv->top = 0;
		break;

	default:
		retvalue = -1;
		print_error("metering mode invalid!");
		goto out;
		break;
	}

out:
	return retvalue;

}

/*
 * Called by ispv1_set_metering_area
 */
int ispv1_setreg_metering_area(camera_rect_s *raw, u32 raw_width, u32 raw_height, int roi)
{
	print_debug("enter %s", __func__);

	/*
	 * Just care about center 3x3 windows. 3x3 weight is all 1.
	 * Long&short exposure are same
	 * set raw rect to ISP registers
	 */
	if (raw->height < 80)
		raw->height = 80;

	if (raw->width < 80)
		raw->width = 80;

	if ((raw->top + raw->height) > raw_height)
		raw->top -= (raw->top + raw->height) - raw_height;

	if ((raw->left + raw->width) > raw_width)
		raw->left -= (raw->left + raw->width) - raw_width;

	if (roi == 0) {
		SETREG16(REG_ISP_AECAGC_CENTER_LEFT, raw->left);
		SETREG16(REG_ISP_AECAGC_CENTER_LEFT_SHORT, raw->left);
		SETREG16(REG_ISP_AECAGC_CENTER_TOP, raw->top);
		SETREG16(REG_ISP_AECAGC_CENTER_TOP_SHORT, raw->top);

		SETREG16(REG_ISP_AECAGC_CENTER_WIDTH, raw->width);
		SETREG16(REG_ISP_AECAGC_CENTER_WIDTH_SHORT, raw->width);

		SETREG16(REG_ISP_AECAGC_CENTER_HEIGHT, raw->height);
		SETREG16(REG_ISP_AECAGC_CENTER_HEIGHT_SHORT, raw->height);

		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_IN, 1);
		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_IN_SHORT, 1);
		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_OUT, 1);
		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_OUT_SHORT, 1);

		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(0), 1);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(1), 1);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(2), 1);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(3), 1);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(4), 9);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(5), 9);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(6), 9);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(7), 9);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(8), 18);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(9), 9);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(10), 9);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(11), 9);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(12), 9);

		//weight shift
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT_SHIFT, 3);
	} else {
	#if 0 //real roi mode
		SETREG16(REG_ISP_AECAGC_ROI_LEFT, raw->left);
		SETREG16(REG_ISP_AECAGC_ROI_LEFT_SHORT, raw->left);
		SETREG16(REG_ISP_AECAGC_ROI_TOP, raw->top);
		SETREG16(REG_ISP_AECAGC_ROI_TOP_SHORT, raw->top);

		SETREG16(REG_ISP_AECAGC_ROI_RIGHT, (raw_width - (raw->left + raw->width)));
		SETREG16(REG_ISP_AECAGC_ROI_RIGHT_SHORT, (raw_width - (raw->left + raw->width)));
		SETREG16(REG_ISP_AECAGC_ROI_BOTTOM, (raw_height - (raw->top + raw->height)));
		SETREG16(REG_ISP_AECAGC_ROI_BOTTOM_SHORT, (raw_height - (raw->top + raw->height)));

		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_IN, 1);
		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_IN_SHORT, 1);
		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_OUT, 0);
		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_OUT_SHORT, 0);
	#else //enhanced 3x3 win mode
		SETREG16(REG_ISP_AECAGC_CENTER_LEFT, raw->left);
		SETREG16(REG_ISP_AECAGC_CENTER_LEFT_SHORT, raw->left);
		SETREG16(REG_ISP_AECAGC_CENTER_TOP, raw->top);
		SETREG16(REG_ISP_AECAGC_CENTER_TOP_SHORT, raw->top);

		SETREG16(REG_ISP_AECAGC_CENTER_WIDTH, raw->width);
		SETREG16(REG_ISP_AECAGC_CENTER_WIDTH_SHORT, raw->width);

		SETREG16(REG_ISP_AECAGC_CENTER_HEIGHT, raw->height);
		SETREG16(REG_ISP_AECAGC_CENTER_HEIGHT_SHORT, raw->height);

		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_IN, 1);
		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_IN_SHORT, 1);
		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_OUT, 1);
		SETREG8(REG_ISP_AECAGC_ROI_WEIGHT_OUT_SHORT, 1);

		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(0), 1);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(1), 1);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(2), 1);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(3), 1);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(4), 0xff);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(5), 0xff);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(6), 0xff);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(7), 0xff);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(8), 0xff);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(9), 0xff);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(10), 0xff);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(11), 0xff);
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT(12), 0xff);

		//weight shift
		SETREG8(REG_ISP_AECAGC_WIN_WEIGHT_SHIFT, 8);
	#endif
	}

	return 0;
}

void ispv1_setreg_ae_statwin(u32 x_offset, u32 y_offset)
{
	SETREG16(REG_ISP_AECAGC_STATWIN_LEFT, x_offset);
	SETREG16(REG_ISP_AECAGC_STATWIN_TOP, y_offset);
	SETREG16(REG_ISP_AECAGC_STATWIN_RIGHT, x_offset);
	SETREG16(REG_ISP_AECAGC_STATWIN_BOTTOM, y_offset);
}

int ispv1_set_ae_statwin(pic_attr_t *pic_attr, u32 zoom)
{
	camera_rect_s yuv;
	camera_rect_s raw;
	u32 x_offset, y_offset;
	u32 ratio = isp_zoom_to_ratio(zoom, 0);
	camera_sensor *sensor = this_ispdata->sensor;//y00231328 0311 change to set ae statwin


	yuv.left = 0;
	yuv.top = 0;
	yuv.width = pic_attr->out_width;
	yuv.height = pic_attr->out_height;
	if (k3_isp_yuvrect_to_rawrect(&yuv, &raw)) {
		print_error("%s:line %d error", __func__, __LINE__);
		return -1;
	}

	//y00231328 0311 change to set ae statwin
	if(sensor->sensor_index == CAMERA_SENSOR_SECONDARY){
		x_offset = (pic_attr->in_width - raw.width * METERING_AECAGC_WINDOW_PERCENT_FOR_SECONDARY_CAMERA \
				* ISP_ZOOM_BASE_RATIO / ratio / 100) / 2;
		y_offset = (pic_attr->in_height - raw.height * METERING_AECAGC_WINDOW_PERCENT_FOR_SECONDARY_CAMERA \
				* ISP_ZOOM_BASE_RATIO / ratio / 100) / 2;
	}else {
		x_offset = (pic_attr->in_width - raw.width * METERING_AECAGC_WINDOW_PERCENT \
				* ISP_ZOOM_BASE_RATIO / ratio / 100) / 2;
		y_offset = (pic_attr->in_height - raw.height * METERING_AECAGC_WINDOW_PERCENT \
				* ISP_ZOOM_BASE_RATIO / ratio / 100) / 2;
	}

	ispv1_setreg_ae_statwin(x_offset, y_offset);
	return 0;
}

int ispv1_set_gsensor_stat(axis_triple *xyz)
{
	camera_sensor *sensor = this_ispdata->sensor;
	int interval;

	if (!sensor->af_enable) {
		return -1;
	}

	interval = (xyz->time.tv_sec - mXYZ.time.tv_sec) * 1000
		+ (xyz->time.tv_usec - mXYZ.time.tv_usec) / 1000;
	afae_ctrl->gsensor_interval = interval;

	memcpy(&mXYZ, xyz, sizeof(axis_triple));
	print_debug("%s: ts %dms, current xyz: 0x%4x,  0x%4x, 0x%4x",
		__func__, (int)(mXYZ.time.tv_sec * 1000 + mXYZ.time.tv_usec / 1000), mXYZ.x, mXYZ.y, mXYZ.z);
	return 0;
}

/*
 * Added for metering mode
 * spot, CWA, average
 */
int ispv1_set_metering_mode(camera_metering mode)
{
	print_debug("enter %s, mode %d", __func__, mode);

	this_metering = mode;
	return 0;
}

int ispv1_set_metering_area(metering_area_s *area, u32 zoom)
{
	k3_isp_data *ispdata = this_ispdata;
	u32 loopi;
	u32 preview_width = ispdata->pic_attr[STATE_PREVIEW].out_width;
	u32 preview_height = ispdata->pic_attr[STATE_PREVIEW].out_height;
	u32 in_width = ispdata->pic_attr[STATE_PREVIEW].in_width;
	u32 in_height = ispdata->pic_attr[STATE_PREVIEW].in_height;

	camera_rect_s yuv;
	camera_rect_s raw;
	int roi_flag = 0;

	print_debug("enter %s", __func__);

	if (ispdata->sensor->isp_location == CAMERA_USE_SENSORISP)
		return 0;

	if (NULL == area) {
		print_error("%s:no metering area", __func__);
		return -1;
	}

	for (loopi = 0; loopi < area->metering_rect_num; loopi++) {
		print_info("metering rect %d: %d,%d,%d,%d,weight %d",
			   loopi,
			   area->rect[loopi].left, area->rect[loopi].top,
			   area->rect[loopi].width, area->rect[loopi].height,
			   area->rect[loopi].weight);
	}

	/* y36721 2012-03-28
	 * set ROI metering before take picture when auto focus not selected.
	 */
	memcpy(&yuv, &area->rect[0], sizeof(camera_rect_s));

	/* check width and height are valid, then adjust it. */
	if (yuv.width == 0 || yuv.height == 0) {
		ispv1_get_default_metering_rect(this_metering, &yuv, preview_width, preview_height);
		roi_flag = 0;
	} else {
		roi_flag = 1;
	}
	ispv1_get_yuvrect_withzoom(&yuv, zoom);

	if (k3_isp_yuvrect_to_rawrect(&yuv, &raw)) {
		print_error("%s:line %d error", __func__, __LINE__);
		return -1;
	}

	ispv1_set_ae_statwin(&ispdata->pic_attr[STATE_PREVIEW], zoom);
	ispv1_setreg_metering_area(&raw, in_width, in_height, roi_flag);

	return 0;
}
int ispv1_set_focus_range(camera_focus focus_mode)
{
	int ret = 0;
	int val_vcm = 0;
	vcm_info_s *vcm = get_vcm_ptr();

	if(!this_ispdata->sensor->af_enable)
		return 0;
	print_debug("enter %s, focus_mode:%d", __func__, focus_mode);
	ret = ispv1_set_vcm_parameters(focus_mode);

	if (ret == -1)
		return ret;

	val_vcm = get_focus_code();
	if (val_vcm < vcm->offsetInit) {
		ispv1_setreg_vcm_code(vcm->offsetInit);
	} else if (val_vcm > vcm->fullRange) {
		ispv1_setreg_vcm_code(vcm->fullRange);
	}
	return ret;
}

/*
 **************************************************************************
 * FunctionName: ispv1_get_focus_distance;
 * Description : get focus_distance value ( cm )
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int ispv1_get_focus_distance(void)
{
    /* < zhoutian 00195335 12-9-25 added for auto scene detect begin */
    int ret = 0;
    int val_vcm = 0;
    vcm_info_s *vcm = this_ispdata->sensor->vcm;
    GETREG16(REG_ISP_FOCUS_MOTOR_CURR, val_vcm);

    int focus_status = 0;
    focus_status = GETREG8(0x1cdd4);

    if (focus_status == STATUS_FOCUSING)
        ret = ISP_FOCAL_LENGTH_FOCUSING;
    else
    {
        if (val_vcm >= (vcm->normalDistanceEnd - 280))	//水平:0.1~0.5m,
            ret = ISP_FOCAL_LENGTH_MACRO;
        else if (val_vcm <=  (vcm->infiniteDistance + 130)) //水平:> 2m
            ret = ISP_FOCAL_LENGTH_INFINITY;
        else
            ret = ISP_FOCAL_LENGTH_MIDDLE;
    }
    
    return ret;
    /* zhoutian 00195335 12-9-25 added for auto scene detect end > */
    /* just reserve interface */
    //return 0;     //zhoutian 00195335 removed for auto scene detect
}
static bool ispv1_focus_need_flash(u32 cur_lum, u32 cur_gain, bool binning)
{
	bool ret = false;
	u8 compare_lum;

	if (binning == false)
		cur_gain /= 2;

	if (cur_gain < FOCUS_PARAM_ASSISTANT_AF_GAINTH_HIGH) {
		compare_lum = FOCUS_PARAM_ASSISTANT_AF_LUMTH_1X + (cur_gain - 0x10) / 0x10 * 4;
		if (cur_lum <= compare_lum)
			ret = true;
		else
			ret = false;
	} else {
		ret = true;
	}

	return ret;
}


static void ispv1_assistant_af(bool action)
{
	camera_flashlight *flashlight = get_camera_flash();
	print_debug("enter %s,action:%d", __func__, action);
	if (flashlight == NULL)
		return;

	if (true == action) {
		ispv1_config_aecawb_step(true, &isp_hw_data.aecawb_step);
		flashlight->turn_on(TORCH_MODE, LUM_LEVEL1);
	} else {
		ispv1_config_aecawb_step(false, &isp_hw_data.aecawb_step);
		flashlight->turn_off();
	}
}


static bool ispv1_af_flash_needed(camera_sensor *sensor, camera_flash flash_mode, lum_win_info_s lum_info)
{
	u32 index = sensor->preview_frmsize_index;
	bool summary = sensor->frmsize_list[index].binning;
	u32 cur_lum;
	u32 cur_gain;
	bool ret = false;

	if (get_focus_state() == FOCUS_STATE_AF_PREPARING) {
		cur_lum = ispv1_focus_get_win_lum(&lum_info);
		cur_gain = get_writeback_gain();
		if (((CAMERA_FLASH_AUTO ==  flash_mode) || (CAMERA_FLASH_ON == flash_mode))
			&& (true == ispv1_focus_need_flash(cur_lum, cur_gain, summary))) {
			ret = true;
		}
	}
	return ret;
}


static void ispv1_af_flash_check_open(void)
{
	camera_sensor *sensor =  this_ispdata->sensor;
	u8 framerate = sensor->fps;
	u32 cur_lum;
	u8 over_expo_th = GETREG8(REG_ISP_TARGET_Y_HIGH) + DEFAULT_FLASH_AEC_FAST_STEP;
	u8 frame_count = 0;

	if (this_ispdata->af_need_flash == true) {
		ispv1_assistant_af(true);
		this_ispdata->assistant_af_flash = true;
		msleep((1000 / framerate) * 2);
		cur_lum = get_current_y();
		while ((frame_count++ < FLASH_TEST_MAX_COUNT) &&
			(cur_lum > over_expo_th) && (get_focus_state() != FOCUS_STATE_STOPPED)) {
			msleep(1000 / framerate);
			cur_lum = get_current_y();
		}
		print_debug("%s, af_need_flash:%d, frame_cout:%d, focus_state:%d, cur_lum:0x%x, over_expo_th:0x%x",
			__func__, this_ispdata->af_need_flash, frame_count, get_focus_state(), cur_lum, over_expo_th);
	}
}


static void ispv1_af_flash_check_close(void)
{
	camera_sensor *sensor =  this_ispdata->sensor;
	u8 framerate = sensor->fps;

	print_debug("%s, af_need_flash:%d", __func__, this_ispdata->af_need_flash);
	if (true == this_ispdata->assistant_af_flash) {
		ispv1_assistant_af(false);
		msleep((1000 / framerate) * 3);
		this_ispdata->assistant_af_flash = false;
	}
	this_ispdata->af_need_flash = false;
}

int ispv1_focus_status_collect(focus_frame_stat_s *curr_data, focus_frame_stat_s *mean_data)
{
	int loopi;
	u32 variance[5] = {0, 0, 0, 0, 0};
	u32 contrast_diff = 0;
	u32 lum_diff = 0;
	axis_triple xyz_diff = {0, 0, 0};
	int stat_cnt;

	afae_ctrl->focus_stat_frames++;

	/* skip first frame, and get mean of next 4 frames contrast/gain/expo/lum value as compare data. */
	if ((afae_ctrl->focus_stat_frames >= CAF_STAT_COMPARE_START_FRAME)
		&& (afae_ctrl->focus_stat_frames <= CAF_STAT_COMPARE_END_FRAME)) {
		afae_ctrl->compare_data.contrast += curr_data->contrast;
		afae_ctrl->compare_data.lum += curr_data->lum;
		afae_ctrl->compare_data.ae += curr_data->ae;
		afae_ctrl->compare_data.rbratio += curr_data->rbratio;
		afae_ctrl->compare_data.xyz.x += curr_data->xyz.x;
		afae_ctrl->compare_data.xyz.y += curr_data->xyz.y;
		afae_ctrl->compare_data.xyz.z += curr_data->xyz.z;

		if (afae_ctrl->focus_stat_frames == CAF_STAT_COMPARE_END_FRAME) {
			afae_ctrl->compare_data.contrast /= CAF_STAT_COMPARE_FRAMES;
			afae_ctrl->compare_data.lum /= CAF_STAT_COMPARE_FRAMES;
			afae_ctrl->compare_data.ae /= CAF_STAT_COMPARE_FRAMES;
			afae_ctrl->compare_data.rbratio /= CAF_STAT_COMPARE_FRAMES;
			afae_ctrl->compare_data.xyz.x /= CAF_STAT_COMPARE_FRAMES;
			afae_ctrl->compare_data.xyz.y /= CAF_STAT_COMPARE_FRAMES;
			afae_ctrl->compare_data.xyz.z /= CAF_STAT_COMPARE_FRAMES;

			/*
			 * init all frame's contrast/gain/expo/lum array as compare data.
			 * used in preview tasklet to judge scene switch.
			 */
			for (loopi = 0; loopi < CAF_STAT_FRAME; loopi++) {
				memcpy(&afae_ctrl->frame_stat[loopi], &afae_ctrl->compare_data,
					sizeof(focus_frame_stat_s));
			}
		}
	}

	if (afae_ctrl->focus_stat_frames <= CAF_STAT_SKIP_FRAME)
		return -1;

	if (curr_data->fps <= 20)
		stat_cnt = CAF_STAT_FRAME_LOW;
	else
		stat_cnt = CAF_STAT_FRAME;

	/* update new contrast/gain/expo/lum value array */
	memcpy(&afae_ctrl->frame_stat[0], &afae_ctrl->frame_stat[1],
		(stat_cnt - 1) * sizeof(focus_frame_stat_s));

	memcpy(&afae_ctrl->frame_stat[stat_cnt - 1], curr_data,
		sizeof(focus_frame_stat_s));

	/* calculate current mean contrast/gain/expo/lum */
	memset(mean_data, 0, sizeof(focus_frame_stat_s));
	for (loopi = 0; loopi < stat_cnt; loopi++) {
		mean_data->contrast += afae_ctrl->frame_stat[loopi].contrast;
		mean_data->lum += afae_ctrl->frame_stat[loopi].lum;
		mean_data->ae += afae_ctrl->frame_stat[loopi].ae;
		mean_data->rbratio += afae_ctrl->frame_stat[loopi].rbratio;
		mean_data->xyz.x += afae_ctrl->frame_stat[loopi].xyz.x;
		mean_data->xyz.y += afae_ctrl->frame_stat[loopi].xyz.y;
		mean_data->xyz.z += afae_ctrl->frame_stat[loopi].xyz.z;
	}

	mean_data->contrast /= stat_cnt;
	mean_data->lum /= stat_cnt;
	mean_data->ae /= stat_cnt;
	mean_data->rbratio /= stat_cnt;
	mean_data->xyz.x /= stat_cnt;
	mean_data->xyz.y /= stat_cnt;
	mean_data->xyz.z /= stat_cnt;

	for (loopi = 0; loopi < stat_cnt; loopi++) {
		/* use last stat data as base calculate value */
		variance[0] = abs(afae_ctrl->frame_stat[loopi].contrast - afae_ctrl->frame_stat[stat_cnt - 1].contrast);
		variance[1] = abs(afae_ctrl->frame_stat[loopi].lum - afae_ctrl->frame_stat[stat_cnt - 1].lum);
		variance[2] = abs(afae_ctrl->frame_stat[loopi].xyz.x - afae_ctrl->frame_stat[stat_cnt - 1].xyz.x);
		variance[3] = abs(afae_ctrl->frame_stat[loopi].xyz.y - afae_ctrl->frame_stat[stat_cnt - 1].xyz.y);
		variance[4] = abs(afae_ctrl->frame_stat[loopi].xyz.z - afae_ctrl->frame_stat[stat_cnt - 1].xyz.z);

		variance[0] *= variance[0];
		variance[1] *= variance[1];
		variance[2] *= variance[2];
		variance[3] *= variance[3];
		variance[4] *= variance[4];

		contrast_diff += variance[0];
		lum_diff += variance[1];
		xyz_diff.x += variance[2];
		xyz_diff.y += variance[3];
		xyz_diff.z += variance[4];
	}
	contrast_diff /= stat_cnt;
	lum_diff /= stat_cnt;
	xyz_diff.x /= stat_cnt;
	xyz_diff.y /= stat_cnt;
	xyz_diff.z /= stat_cnt;

	mean_data->contrast_var = contrast_diff;
	mean_data->lum_var = lum_diff;
	mean_data->xyz_var.x = xyz_diff.x;
	mean_data->xyz_var.y = xyz_diff.y;
	mean_data->xyz_var.z = xyz_diff.z;

	return 0;
}

static void ispv1_focus_status_reset(void)
{
	afae_ctrl->focus_stat_frames = 0;
	set_caf_forcestart(CAF_FORCESTART_CLEAR); /* clear force start bits */
	memset(&afae_ctrl->compare_data, 0, sizeof(focus_frame_stat_s));
}


u16 ispv1_calc_frame_stat_diff(focus_frame_stat_s *pre_data, focus_frame_stat_s *cur_data)
{
	u16 ret_val = 0;
	focus_frame_stat_s diff;

	diff.contrast = abs(pre_data->contrast - cur_data->contrast);
	diff.lum = abs(pre_data->lum - cur_data->lum);
	diff.ae = abs(pre_data->ae - cur_data->ae);
	diff.rbratio= abs(pre_data->rbratio - cur_data->rbratio);

	diff.xyz.x = abs(pre_data->xyz.x - cur_data->xyz.x);
	diff.xyz.y = abs(pre_data->xyz.y - cur_data->xyz.y);
	diff.xyz.z = abs(pre_data->xyz.z - cur_data->xyz.z);

	if ((diff.contrast >= (pre_data->contrast / FOCUS_PARAM_COMPARE_RATIO_CONTRAST) ||
		diff.contrast >= (cur_data->contrast / FOCUS_PARAM_COMPARE_RATIO_CONTRAST))
		&& (diff.contrast >= FOCUS_PARAM_MIN_TRIGGER_CONTRAST_DIFF))
		ret_val |= 0x01;
	/*
	if ((diff.lum >= (pre_data->lum / FOCUS_PARAM_COMPARE_RATIO_LUM))
		&& (diff.lum >= FOCUS_PARAM_MIN_TRIGGER_LUM_DIFF))
		ret_val |= 0x02;
	*/
	if (diff.ae >= (pre_data->ae / FOCUS_PARAM_COMPARE_RATIO_AE))
		ret_val |= 0x04;
	if (diff.rbratio >= (pre_data->rbratio / FOCUS_PARAM_COMPARE_RATIO_AWB))
		ret_val |= 0x08;

	if (diff.xyz.x >= FOCUS_PARAM_COMPARE_DIFF_XYZ)
		ret_val |= 0x10;
	if (diff.xyz.y >= FOCUS_PARAM_COMPARE_DIFF_XYZ)
		ret_val |= 0x20;
	if (diff.xyz.z >= FOCUS_PARAM_COMPARE_DIFF_XYZ)
		ret_val |= 0x40;

	return ret_val;
}

caf_detect_result ispv1_check_caf_need_trigger(focus_frame_stat_s *compare_data, focus_frame_stat_s *mean_data, focus_frame_stat_s cur_data)
{
	u8 unpeace = 0;
	static u16 unpeace_ms;
	u16 force_start = get_caf_forcestart();
	u16 diff_val = 0;
	u8 fps = this_ispdata->sensor->fps;
	bool trigger = false;

	if (force_start == 0) {
		/* if diff too much, should force focus start. */
		diff_val = ispv1_calc_frame_stat_diff(compare_data, mean_data);
		force_start = diff_val;
		set_caf_forcestart(force_start);
	} else if ((force_start & CAF_FORCESTART_FORCE) != 0) {
		unpeace_ms = 0;
		return CAF_DETECT_RESULT_TRIGGER;
	}

	if (force_start == 0)
		return CAF_DETECT_RESULT_NONE;

	if (mean_data->contrast_var > (mean_data->contrast * mean_data->contrast / FOCUS_PARAM_VAR_RATIO_CONTRAST))
		unpeace |= 0x01;
	if (mean_data->lum_var > (mean_data->lum * mean_data->lum / FOCUS_PARAM_VAR_RATIO_LUM))
		unpeace |= 0x02;

	if (mean_data->xyz_var.x > FOCUS_PARAM_VAR_DIFF_XYZ)
		unpeace |= 0x10;
	if (mean_data->xyz_var.y > FOCUS_PARAM_VAR_DIFF_XYZ)
		unpeace |= 0x20;
	if (mean_data->xyz_var.z > FOCUS_PARAM_VAR_DIFF_XYZ)
		unpeace |= 0x40;

	print_debug("mean contrast var [0x%x:0x%x], lum var [0x%x:0x%x], xyz var [%d, %d, %d]",
		mean_data->contrast_var, mean_data->contrast,
		mean_data->lum_var, mean_data->lum,
		mean_data->xyz_var.x, mean_data->xyz_var.y, mean_data->xyz_var.z);

	if (force_start && unpeace == 0) {
		if (mean_data->lum >= FOCUS_PARAM_MIN_TRIGGER_LUM) {
			if (afae_ctrl->focus_stat_frames > CAF_STAT_SKIP_FRAME
				&& (force_start & (CAF_FORCESTART_FORCE | CAF_FORCESTART_FORCEWAIT)) == 0) {

				diff_val = ispv1_calc_frame_stat_diff(compare_data, &cur_data);
				if (diff_val != 0)
					trigger = true;
			}else {
				/* CAF_FORCESTART_FORCE or CAF_FORCESTART_FORCEWAIT */
				trigger = true;
			}
			unpeace_ms = 0;
			if (trigger == true) {
				print_info("CAF scene changed reason: 0x%.4x; contrast [0x%x->0x%x], diff_val 0x%x!",
					force_start, compare_data->contrast, mean_data->contrast, diff_val);
				return CAF_DETECT_RESULT_TRIGGER;
			} else {
				set_caf_forcestart(CAF_FORCESTART_CLEAR);
				return CAF_DETECT_RESULT_NOISE;
			}
		} else {
			return CAF_DETECT_RESULT_NONE;
		}
	} else {
		print_debug("force_start 0x%x, but unpeace 0x%x, gsensor unpeace_ms %d", force_start, unpeace, unpeace_ms);

		/* if unpeace caused by Gsensor more than XX seconds, force VCM go to infinite. */
		if ((unpeace & 0xf0) != 0) {
			if (unpeace_ms < FOCUS_PARAM_GOTO_INFINITE_TIMEOUT) {
				unpeace_ms += 1000 / fps;
				return CAF_DETECT_RESULT_NONE;
			} else {
				/* if gsensor is unpeace than (XX * fps), force VCM go to infinite */
				unpeace_ms = 0;
				return CAF_DETECT_RESULT_INFINITE;
			}
		} else {
			/* if gsensor is stable, Gsensor unpeace count set zero and return RESULT_NONE. */
			unpeace_ms = 0;
			return CAF_DETECT_RESULT_NONE;
		}
	}
}

/*
 **************************************************************************
 * FunctionName: ispv1_focus_get_curr_data
 * Description : using for save current envionment variables
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void ispv1_focus_get_curr_data(focus_frame_stat_s *curr_data)
{
	awb_gain_t awb_gain;
	u32 gain, expo;

	GETREG16(REG_ISP_FOCUS_CONTRAST(0), curr_data->contrast);
	gain = get_writeback_gain();
	expo = get_writeback_expo();

	curr_data->lum = ispv1_focus_get_win_lum(&afae_ctrl->lum_info);
	curr_data->ae = gain * expo;
	curr_data->fps = this_ispdata->sensor->fps;

	ispv1_get_wb_value(&awb_gain);
	if ((awb_gain.b_gain != 0) && (awb_gain.r_gain != 0)) {		
		curr_data->rbratio = 0x8000 / (awb_gain.b_gain * 0x100 / awb_gain.r_gain);
	} else {
		curr_data->rbratio = 0x100;
	}
	memcpy(&curr_data->xyz, &mXYZ, sizeof(axis_triple));

	print_debug("current contrast 0x%.3x, lum 0x%.2x, xyz: %.4d, %.4d, %.4d",
		curr_data->contrast, curr_data->lum,
		curr_data->xyz.x, curr_data->xyz.y, curr_data->xyz.z);
}

void ispv1_get_default_focusrect(camera_rect_s *rect)
{
	u32 preview_width = this_ispdata->pic_attr[STATE_PREVIEW].out_width;
	u32 preview_height = this_ispdata->pic_attr[STATE_PREVIEW].out_height;

	ispv1_focus_get_default_yuvrect(rect, preview_width, preview_height);
}

u32 ispv1_get_focus_rect(camera_rect_s *rect)
{
	u32 preview_width = this_ispdata->pic_attr[STATE_PREVIEW].out_width;
	u32 preview_height = this_ispdata->pic_attr[STATE_PREVIEW].out_height;

	memcpy(rect, &afae_ctrl->af_area.rect[0], sizeof(camera_rect_s));
	if (rect->width == 0 || rect->height == 0)
		ispv1_get_default_focusrect(rect);

	ispv1_focus_adjust_yuvrect(rect, preview_width, preview_height, afae_ctrl->zoom);
	return 0;
}

bool ispv1_copy_preview_data(u8 *dest, camera_rect_s *rect)
{
	camera_frame_buf *frame = this_ispdata->current_frame;
	u32 preview_width = this_ispdata->pic_attr[STATE_PREVIEW].out_width;
	u32 preview_height = this_ispdata->pic_attr[STATE_PREVIEW].out_height;
	u32 index;
	u8 *src;
	bool ret = false;

	if (down_trylock(&(this_ispdata->frame_sem)) != 0) {
		print_warn("enter %s, frame_sem already locked", __func__);
		goto error_out2;
	}

	if (NULL == dest || NULL == frame || 0x0 == frame->phyaddr) {
		print_error("params error, dest 0x%x, frame 0x%x", (unsigned int)dest, (unsigned int)frame);
		goto error_out1;
	}

	if ((0 == frame->phyaddr) || (NULL != frame->viraddr)) {
		print_error("params error, frame->phyaddr 0x%x, frame->viraddr:0x%x",
			(unsigned int)frame->phyaddr, (unsigned int)frame->viraddr);
		goto error_out1;
	}

	frame->viraddr = ioremap_nocache(frame->phyaddr, preview_width * preview_height);
	if (NULL == frame->viraddr) {
		print_error("%s line %d error", __func__, __LINE__);
		goto error_out1;
	}

	for(index = 0; index < rect->height; index++) {
		src = (u8 *)frame->viraddr + preview_width * (rect->top + index) + rect->left;
		memcpy(dest, src, rect->width);
		dest += rect->width;
	}
	iounmap(frame->viraddr);
	frame->viraddr = NULL;
	ret = true;

error_out1:
	up(&(this_ispdata->frame_sem));

error_out2:
	return ret;
}

static int ispv1_focus_calc_variance(u8 *pdata, u32 size)
{
	int index;
	u32 average_y = 0;
	u32 variance_y = 0;

	print_debug("enter %s", __func__);

	/* calculate average */
	if (pdata == NULL) {
		print_error("pdata NULL in %s function at line %d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	for (index = 0; index < size; index++)
		average_y += pdata[index];
	average_y /= size;

	for (index = 0; index < size; index++) {
		variance_y += (pdata[index] - average_y) * (pdata[index] - average_y);
	}

	/* variance of mean */
	variance_y /= size;
	return variance_y;
}

#define FOCUS_VAR_LOW_LIMIT		0x100
#define FOCUS_VAR_HIGH_LIMIT		0x400

#define FOCUS_VAR_LOW_COEFF		0x18
#define FOCUS_VAR_HIGH_COEFF		0x10

void ispv1_focus_update_threshold_ext(u8 *pdata, u32 size)
{
	u32 curr_y;
	u32 curr_gain;
	u32 threshold_ext;
	u32 variance_y = 0;
	u32 coeff = 0;

	variance_y = ispv1_focus_calc_variance(pdata, size);
	if (variance_y <= FOCUS_VAR_LOW_LIMIT) {
		coeff = FOCUS_VAR_LOW_COEFF;
	} else if ((variance_y > FOCUS_VAR_LOW_LIMIT) && (variance_y <= FOCUS_VAR_HIGH_LIMIT)) {
		coeff =  (FOCUS_VAR_HIGH_LIMIT - variance_y);
		coeff *=  (FOCUS_VAR_LOW_COEFF - FOCUS_VAR_HIGH_COEFF);
		coeff /= (FOCUS_VAR_HIGH_LIMIT - FOCUS_VAR_LOW_LIMIT);
		coeff += FOCUS_VAR_HIGH_COEFF;
	} else {
		coeff = FOCUS_VAR_HIGH_COEFF;
	}

	curr_y = get_current_y();
	curr_gain = get_writeback_gain();
	//curr_gain = (curr_gain > 0x70) ? 0x70 : (curr_gain & 0xf8);
	curr_gain = (curr_gain > 0x40) ? 0x40 : (curr_gain & 0xf8); //revised by y00215412

	threshold_ext = (0xd0 - curr_gain) / 0xc;
	if (curr_y < 0x10) {
		threshold_ext = 0;
	} else if ((curr_y >= 0x10) && (curr_y < 0x20)) {
		threshold_ext = (curr_y - 0x10) * threshold_ext / 0x10;
	}

	threshold_ext = threshold_ext * coeff / FOCUS_VAR_HIGH_COEFF;

	SETREG16(REG_ISP_FOCUS_AFCCTRL2, threshold_ext);
	print_info("y:0x%.2x, y_var 0x%x, gain: 0x%.2x, threshold_ext: 0x%.2x, coeff 0x%x",
		curr_y, variance_y, curr_gain, threshold_ext, coeff);
}

/*
 **************************************************************************
 * FunctionName: ispv1_focus_calc_edge;
 * Description : using for calculate contrast;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : in 1080P preview case, coarse_flag must set to true;
 **************************************************************************
 */
static u32 ispv1_focus_calc_edge(u8 *pdata, camera_rect_s *rect, int contrast_shift, bool coarse_flag)
{
	u32 i, j;
	u32 sum = 0;
	u32 total = 1;
	u32 value1, value2;
	u32 thr = 1;
	u32 result = 0;
	u8 *sdata;
	u32 step;
	u32 width = rect->width;
	u32 height = rect->height;

	struct timeval tv1, tv2;
	int t1;

	do_gettimeofday(&tv1);

	thr >>= contrast_shift;
	step = (coarse_flag == true) ? 3 : 1;

	for (i = 0; i < (height - 3); i += step) {
		sdata = pdata + width * i;
		for (j = 0; j < (width - 3); j += step) {
			value1 = abs(sdata[j + 2] + (sdata[width + j + 2]<<1) + sdata[(width<<1) + j + 2]
					-sdata[j] - (sdata[width + j]<<1) - sdata[(width<<1) + j]);
			value2 = abs(sdata[(width<<1) + j] + (sdata[(width<<1) + j + 1]<<1) + sdata[(width<<1) + j + 2]
					-sdata[j] - (sdata[j + 1]<<1) - sdata[j + 2]);

			if (value1 < thr)
				value1 = 0;
			if (value2 < thr)
				value2 = 0;
			sum += (value1 + value2);
			total++;
		}
	}
	result = (sum << 4) / total;

	do_gettimeofday(&tv2);

	t1 = (tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec) / 1000;

	print_debug("%s: w %3d, h %3d, coarse_flag %d, used %dms", __func__, width, height, coarse_flag, t1);

	return result;
}

int ispv1_focus_need_schedule(void)
{
	int schedule_case = -1;

	if (afae_ctrl->k3focus_running == false)
		return schedule_case;

	if (get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_PICTURE &&
	    get_focus_state() == FOCUS_STATE_CAF_DETECTING) {
		schedule_case = FOCUS_SCHEDULE_CASE_CAF_PICTURE_DETECT;
	} else if (get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_PICTURE &&
	    get_focus_state() == FOCUS_STATE_CAF_RUNNING) {
		schedule_case = FOCUS_SCHEDULE_CASE_CAF_PICTURE_MOVE;
	} else if (get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_VIDEO &&
	    get_focus_state() == FOCUS_STATE_CAF_DETECTING) {
		schedule_case = FOCUS_SCHEDULE_CASE_CAF_VIDEO_DETECT;
	} else if (get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_VIDEO &&
	    get_focus_state() == FOCUS_STATE_CAF_RUNNING) {
		schedule_case = FOCUS_SCHEDULE_CASE_CAF_VIDEO_MOVE;
	} else if (get_focus_state() == FOCUS_STATE_AF_RUNNING) {
		schedule_case = FOCUS_SCHEDULE_CASE_AF_MOVE;
	} else if (get_focus_mode() == CAMERA_FOCUS_AUTO_VIDEO &&
	    get_focus_state() == FOCUS_STATE_AF_RUNNING) {
		schedule_case = FOCUS_SCHEDULE_CASE_VAF_MOVE;
	}

	return schedule_case;
}

void ispv1_wakeup_focus_schedule(bool force_flag)
{
	u8 framerate = this_ispdata->sensor->fps;
	u16 curr_contrast;
	static int frame_count;

	if(!this_ispdata->sensor->af_enable) {
		return;
	}

	if (force_flag == true) {
		up(&sem_af_schedule);
		return;
	}

	if ((frame_count++ % 100) == 0) {
		GETREG16(REG_ISP_FOCUS_CONTRAST(0), curr_contrast);
		print_info("fps %d, expo 0x%x, gain 0x%x, current y 0x%x, contrast 0x%x(100 frames print once)",
			framerate, get_writeback_expo(), get_writeback_gain(), get_current_y(), curr_contrast);
	}

	if ((ispv1_focus_need_schedule() != -1) && (afae_ctrl->k3focus_running == true))
		up(&sem_af_schedule);
}

static int ispv1_wait_focus_schedule_timeout(int time_out)
{
	long jiffies = 0;

	jiffies = msecs_to_jiffies(time_out);
	if (down_timeout(&sem_af_schedule, jiffies)) {
		print_error("wait focus schedule timeout\n");
		return -ETIME;
	} else {
		print_debug("focus schedule waking up###########");
		return 0;
	}
}

/* for Picture CAF, adjust current code to a smaller code if it is not aligned. */
static int af_adjust_curr_vcmcode(vcm_info_s *vcm, int curr)
{
	int strideSeparator;
	int ret_code;
	u32 strideRatio;

	strideSeparator = vcm->offsetInit + vcm->strideOffset;

	if ((get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_VIDEO)
		|| (get_focus_mode() == CAMERA_FOCUS_AUTO_VIDEO)) {
		strideRatio = vcm->videoStrideRatio;
	} else {
		strideRatio = vcm->normalStrideRatio;
	}

	if (curr >= strideSeparator) {
		ret_code = curr - (curr - strideSeparator) % get_af_large_step(vcm->coarseStep, strideRatio);
	} else {
		ret_code = curr - (curr - vcm->offsetInit) % vcm->coarseStep;
	}

	return ret_code;
}

/*
 **************************************************************************
 * FunctionName: af_get_next_vcmcode
 * Description : calculate next position;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : overflow_flag true:within, false: overflow;
 * Other       : 
 **************************************************************************
 */
static bool af_get_next_vcmcode(af_trip_info *trip, int curr_code, int direction, af_code_step step_type, int *next)
{
	vcm_info_s *vcm = get_vcm_ptr();

	int stride = vcm->coarseStep;
	int large_stride;
	int next_code = 0;
	int strideSeparator;

	int left, right; /* left is small code, right is large code */

	left = (trip->end_pos > trip->start_pos) ? trip->start_pos: trip->end_pos;
	right = (trip->end_pos > trip->start_pos) ? trip->end_pos: trip->start_pos;

	/* check current code, already overflow */
	if (curr_code <= left && direction == AF_RUN_DIRECTION_BACKWARD)
		return false;
	if (curr_code >= right && direction == AF_RUN_DIRECTION_FORWARD)
		return false;

	/* maybe large stride in macro distance. */
	if (step_type == AF_CODE_STEP_VIDEO)
		large_stride = get_af_large_step(vcm->coarseStep, vcm->videoStrideRatio);
	else
		large_stride = get_af_large_step(vcm->coarseStep, vcm->normalStrideRatio);

	if ((step_type == AF_CODE_STEP_COARSE) || (step_type == AF_CODE_STEP_VIDEO)) {
		strideSeparator = vcm->offsetInit + vcm->strideOffset;
		strideSeparator += ((vcm->fullRange - strideSeparator) % large_stride);
		if ((curr_code == strideSeparator && direction == AF_RUN_DIRECTION_FORWARD) || (curr_code > strideSeparator))
			stride = large_stride;
		else
			stride = vcm->coarseStep;
	} else if (step_type == AF_CODE_STEP_FINE) {
		stride = vcm->fineStep;
	}

	next_code = curr_code + direction * stride;

	/* check next code again */
	if (next_code > right || next_code < left) {
		return false;
	} else {
		*next = next_code;
		return true;
	}
}

/*
 **************************************************************************
 * FunctionName: check_vcmcode_is_edge
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : overflow_flag true:overflow, false: within;
 * Other       :
 **************************************************************************
 */
static inline bool check_vcmcode_is_edge(af_trip_info *trip, int code)
{
	int left, right; /* left is small code, right is large code */

	left = (trip->end_pos > trip->start_pos) ? trip->start_pos: trip->end_pos;
	right = (trip->end_pos > trip->start_pos) ? trip->end_pos: trip->start_pos;

	/* check current code. */
	if (code <= left && trip->direction == AF_RUN_DIRECTION_BACKWARD)
		return true;
	if (code >= right && trip->direction == AF_RUN_DIRECTION_FORWARD)
		return true;

	return false;
}

/* for CAF try stage to coarse, current trip should reverse. */
static void af_reverse_curr_trip(vcm_info_s *vcm, af_trip_info *trip, u32 start_code)
{
	/* end_pos goto another side */
	trip->start_pos = start_code;
	trip->direction = 0 - trip->direction;

	if (trip->direction == AF_RUN_DIRECTION_BACKWARD)
		trip->end_pos = vcm->offsetInit;
	else
		trip->end_pos = vcm->fullRange;
}

static inline focus_position_e check_postion_partition(int curr, int focus_range)
{
	if (curr <= (focus_range / FOCUS_PARAM_RANGE_OF_TRY))
		return FOCUS_POSITION_PART_LEFT;
	else if (curr > (focus_range / FOCUS_PARAM_RANGE_OF_TRY) && curr <= focus_range / 2)
		return FOCUS_POSITION_PART_CENTER_LEFT;
	else if (curr > (focus_range / 2) &&
		curr < (focus_range * (FOCUS_PARAM_RANGE_OF_TRY - 1)  / FOCUS_PARAM_RANGE_OF_TRY))
		return FOCUS_POSITION_PART_CENTER_RIGHT;
	else
		return FOCUS_POSITION_PART_RIGHT;
}

/*
 **************************************************************************
 * FunctionName: analysis_contrast_value;
 * Description : analysis contrast value of coarse step phase,
 			and judge focus result in this time
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
bool analysis_contrast_value(u32 *array, af_run_param *af_info)
{
	af_trip_info *trip = &(af_info->trip);
	u32 array_size = trip->step_cnt;
	u32 index = 0;
	u32 top_cnt = 0;
	u32 max_cnt = DEFAULT_AF_MAX_FOCUS_STEP;
	u32 contrast_top = 0;
	u8 top_index = 0;
	bool ret = true;
	u8 start_index = 0;
	u32 contrast_threshold = 0;
	u32 focus_lum;

	if(this_ispdata->sensor->get_af_param)
		max_cnt = (u32)(this_ispdata->sensor->get_af_param(AF_MAX_FOCUS_STEP));

	/* search top contrast and count that */
	af_info->af_analysis = 0;

	/* search top contrast's index and count that */
	for (index = 0; index < array_size; index++) {
		if (contrast_top < array[index]) {
			contrast_top = array[index];
			top_index = index;
			af_info->af_analysis = 0;
		} else if (contrast_top == array[index]) {
			if ((index - top_index) > 2 ) {
				af_info->af_analysis |= 0x02;
			}
		}
	}

	if ((abs(contrast_top - array[0]) < FOCUS_PARAM_JUDGE_ERROR_MINDIFF) &&
		(array_size > FOCUS_PARAM_JUDGE_ERROR_MINSTEPS) &&
		contrast_top < FOCUS_PARAM_JUDGE_LOW_CONTRAST) {
		af_info->af_analysis |= 0x04;
	}

	focus_lum = ispv1_focus_get_win_lum(&afae_ctrl->lum_info);
	if (focus_lum <= FOCUS_PARAM_JUDGE_ERROR_LUM) {
		af_info->af_analysis |= 0x04;
	}

	contrast_threshold = contrast_top * FOCUS_PARAM_STAT_THRESHOLD_PERCENT / 100;
	/* analysis slope of curve */
	if (array_size > 10) {
		if (top_index < 3) {
			start_index = 0;
		} else if (top_index > array_size - 4) {
			start_index = array_size - 7;
		} else {
			start_index = top_index - 3;
		}

		for (index = start_index; index < start_index + 7; index++) {
			if (contrast_threshold < array[index]) {
				top_cnt++;
			}
		}
		if (top_cnt > max_cnt) {
			af_info->af_analysis |= 0x01;
		}
	}

	if (af_info->af_analysis != 0) {
		ret = false;
	}
	print_info("%s, af_info->af_analysis:0x%x, top_count:%d, contrast_top:0x%x, array_size:%d, start_index:%d",
		__func__, af_info->af_analysis, top_cnt, contrast_top, array_size, start_index);
	return ret;
}

/*
 **************************************************************************
 * FunctionName: ispv1_judge_skip_frame;
 * Description : calculate idle time of sensor, if idle time less than 10ms
 			and current frame rate large than 15fps, than need skip frame;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
bool ispv1_judge_skip_frame(void)
{
	bool ret = false;

	camera_sensor *sensor = this_ispdata->sensor;
	vcm_info_s *vcm = get_vcm_ptr();
	u32 frame_index = sensor->preview_frmsize_index;
	u32 basic_vts = sensor->frmsize_list[frame_index].vts;
	u32 fullfps = sensor->frmsize_list[frame_index].fps;
	u32 height = sensor->frmsize_list[frame_index].height;
	u32 fps = sensor->fps;
	u32 vts = basic_vts * fullfps / fps;
	int expo_time_ms;
	int stable_expo_ms;

	expo_time_ms = (get_writeback_expo() >> 4) * 1000 / (basic_vts * fullfps);
	stable_expo_ms = (vts - height) * 1000 / (vts * fps) - vcm->motorResTime;

	if ((sensor->fps > 15) && (stable_expo_ms <= (expo_time_ms / 2))) {
		ret = true;
	} else {
		ret = false;
	}
	print_info("expo_line:%d, vts:%d, expo time:%d, stable_expo_ms:%d ret:%d, fps:%d",
		get_writeback_expo()>>4, vts, expo_time_ms, stable_expo_ms, ret, sensor->fps);
	return ret;
}

void ispv1_k3focus_run(void)
{
	vcm_info_s *vcm = get_vcm_ptr();
	u32 preview_width = this_ispdata->pic_attr[STATE_PREVIEW].out_width;
	u32 preview_height = this_ispdata->pic_attr[STATE_PREVIEW].out_height;
	int schedule_case = -1;
	FOCUS_STATUS result = STATUS_FOCUSING;
	int ret;

	/* hold and detect case use these. */
	caf_detect_result check_result = CAF_DETECT_RESULT_NONE;
	focus_frame_stat_s curr_data;
	focus_frame_stat_s mean_data;

	focus_frame_stat_s start_data;
	bool restart = false;

	camera_rect_s *rect = &afae_ctrl->cur_rect;
	u8 *pstat_data = afae_ctrl->stat_data;

	af_run_param af_info;
	af_trip_info *trip = &(af_info.trip);
	pos_info *top = &(af_info.top);
	pos_info *histtop = &(af_info.histtop);
	pos_info curr;

	int next_code = 0;
	int tryrewind_code = 0;

	af_run_stage next_stage = AF_RUN_STAGE_PREPARE;

	int aec_stable;

	/* used for ispv1_focus_calc_edge(), set coarse flag or not */
	bool edge_calc_coarse = true;

	/* picture AF case use these */
	u32 focus_range = 0;
	u32 pos_offset = 0;

	bool vcm_stable = false;
	focus_position_e pos_part;

	u32 contrast_array[30];

	/*video/picture CAF move case use this. */
	u32 frame_count = 0;
	bool skip_frame = false;
	bool copy_ret = true;

	/*init picture AF params */
	memset(&af_info, 0, sizeof(af_run_param));

	memset(&curr_data, 0, sizeof(focus_frame_stat_s));
	memset(&mean_data, 0, sizeof(focus_frame_stat_s));

	/* first set edge_calc_coarse flag first */
	if ((preview_width * preview_height) > FOCUS_PARAM_EDGE_CALC_COARSE_SIZE)
		edge_calc_coarse = true;
	else
		edge_calc_coarse = false;

	/*video CAF params init */
	curr.code = get_focus_code();
	curr.contrast = 0;

	top->code = curr.code;
	top->contrast = 0;
	histtop->code = curr.code;
	histtop->contrast = 0;

	schedule_case = ispv1_focus_need_schedule();
	print_info("enter %s, case %d ###########, current code 0x%x", __func__, schedule_case, curr.code);

	/*set k3focus_running flag is true */
	afae_ctrl->k3focus_running = true;

	ispv1_free_af_run_sem();

	ispv1_af_flash_check_open();

	/* init params */
	sema_init(&sem_af_schedule, 0);

	while ((schedule_case = ispv1_focus_need_schedule()) != -1) {
		ret = ispv1_wait_focus_schedule_timeout(120);
		if (ret != 0)
			continue;

		/* recheck again. */
		schedule_case = ispv1_focus_need_schedule();
		print_debug("af schedule waked up case %d ###########", schedule_case);

		switch (schedule_case) {
		case FOCUS_SCHEDULE_CASE_CAF_PICTURE_DETECT:
		case FOCUS_SCHEDULE_CASE_CAF_VIDEO_DETECT:
			result = get_focus_result();
			if ((result == STATUS_FOCUSED) || (result == STATUS_OUT_FOCUS)) {
				if ((get_caf_forcestart() & CAF_FORCESTART_FORCE) == 0) {
					ispv1_focus_get_curr_data(&curr_data);
					ret = ispv1_focus_status_collect(&curr_data, &mean_data);
					if (ret != 0)
						break;
				}

				/* if diff too much and quiet, should force focus start. */
				check_result = ispv1_check_caf_need_trigger(&afae_ctrl->compare_data, &mean_data, curr_data);
				if (check_result == CAF_DETECT_RESULT_TRIGGER
					&& get_focus_state() == FOCUS_STATE_CAF_DETECTING) {
					set_focus_state(FOCUS_STATE_CAF_RUNNING);
					ispv1_focus_status_reset();
					frame_count = 0;
					if (schedule_case == FOCUS_SCHEDULE_CASE_CAF_PICTURE_DETECT)
						set_focus_stage(AF_RUN_STAGE_PREPARE);
					else
						set_focus_stage(VCAF_RUN_STAGE_PREPARE);
				} else if (check_result == CAF_DETECT_RESULT_INFINITE) {
					if (curr.code != vcm->offsetInit) {
						curr.code = vcm->offsetInit;
						ispv1_setreg_vcm_code(curr.code);
						ispv1_focus_status_reset();

						/* should wait peace and force CAF */
						set_caf_forcestart(CAF_FORCESTART_FORCEWAIT);
						print_info("unpeace timeout, force go to infinite. !!!!!!!!!!!!!!!!!!!!!");
					}
				} else if (check_result == CAF_DETECT_RESULT_NOISE) {
					print_debug("%s, detected temperarily sudden changes of scene", __func__);
					afae_ctrl->focus_stat_frames = CAF_STAT_SKIP_FRAME;
				}
			}

			break;

		case FOCUS_SCHEDULE_CASE_CAF_PICTURE_MOVE:
		case FOCUS_SCHEDULE_CASE_AF_MOVE:
		case FOCUS_SCHEDULE_CASE_VAF_MOVE:
			copy_ret = ispv1_copy_preview_data(pstat_data, rect);
			if (false == copy_ret)
				break;

			curr.contrast = ispv1_focus_calc_edge(pstat_data, rect, 0, edge_calc_coarse);
			print_info("PAF move stage %d, dir %d, [0x%3x->0x%3x], curr[0x%.3x, 0x%.3x], top[0x%.3x, 0x%.3x]######",
				get_focus_stage(), trip->direction, trip->start_pos, trip->end_pos, curr.code, curr.contrast, top->code, top->contrast);

			if ((get_focus_stage() == AF_RUN_STAGE_TRY) ||(get_focus_stage() == AF_RUN_STAGE_COARSE) ||(get_focus_stage() == AF_RUN_STAGE_FINE)) {
				if (skip_frame == true) {
					if (trip->step_cnt == 0)
						frame_count += 2;
					else if (frame_count ++ % 2 == 0)
						break;
				}
			}

			switch (get_focus_stage()) {
				case AF_RUN_STAGE_PREPARE:

					#ifdef AF_TIME_PRINT
						do_gettimeofday(&tv_start);
					#endif

					/* CAF need wait AEC/AGC stable; touch AF need not wait, but need lock AEC/AGC */
					if (get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_PICTURE) {
						aec_stable = GETREG8(REG_ISP_AECAGC_STABLE);
						if (aec_stable == 0) {
							break;
						}
					} else
						ispv1_set_aecagc_mode(MANUAL_AECAGC);

					/* before move vcm, must calculate compare rate by current expo time and frame rate */
					skip_frame = ispv1_judge_skip_frame();

					/* touch AF need pre-move */
					if ((get_focus_mode() != CAMERA_FOCUS_CONTINUOUS_PICTURE)
						&& (get_focus_mode() != CAMERA_FOCUS_AUTO_VIDEO)) {
						pos_offset = 0;
					} else {
						pos_offset = curr.code - vcm->offsetInit;
					}
					focus_range = vcm->fullRange - vcm->offsetInit;

					/* next_stage just save next primary stage, in fact 'af_stage' should goto PREPARE_POST first */
					pos_part = check_postion_partition(pos_offset, focus_range);
					if (pos_part == FOCUS_POSITION_PART_CENTER_LEFT ||
						pos_part == FOCUS_POSITION_PART_CENTER_RIGHT) {
						next_stage = AF_RUN_STAGE_TRY;
						next_code = curr.code;
					} else if (pos_part == FOCUS_POSITION_PART_LEFT) {
						next_stage = AF_RUN_STAGE_COARSE;
						next_code = vcm->offsetInit;
						trip->direction = AF_RUN_DIRECTION_FORWARD;
					} else if (pos_part == FOCUS_POSITION_PART_RIGHT) {
						next_stage = AF_RUN_STAGE_COARSE;
						next_code = vcm->fullRange;
						trip->direction = AF_RUN_DIRECTION_BACKWARD;
					}

					set_focus_result(STATUS_FOCUSING);

					/*
					 * calculate init focus postion, and go to init position
					 * maybe offsetInit, maybe a little shift from current position
					 */
					next_code = af_adjust_curr_vcmcode(vcm, next_code);
					if (next_code != curr.code) {
						ispv1_setreg_vcm_code(next_code);
						curr.code = next_code;
						af_info.hold_cnt = 0; /* should hold some frames to wait stable */
						set_focus_stage(AF_RUN_STAGE_PREPARE_POST);
					} else {
						/* if do not need adjust vcm code, then go to TRY or COARSE stage directly */
						goto do_prepare_post;
					}
					break;

				case AF_RUN_STAGE_PREPARE_POST:
					af_info.hold_cnt++;
					vcm_stable = ispv1_af_need_wait_stable(get_focus_stage(), af_info.hold_cnt);
					/* if vcm not stable, should wait next frame */
					if (vcm_stable == false) {
						break;
					}
do_prepare_post:
					af_info.hold_cnt = 0; /* set zero for next stages */

					/* prepare top information for next stage */
					top->code = curr.code;
					top->contrast = 0;
					histtop->code = curr.code;
					histtop->contrast = 0;

					ispv1_af_range_cal(&af_info, &curr, next_stage);
					set_focus_stage(next_stage);
					break;

				case AF_RUN_STAGE_TRY:
					/* ensure top->contrast value is same with start point. */
					if (top->code == curr.code)
						top->contrast = curr.contrast;

					next_stage = ispv1_af_recognise_curve(&af_info, &curr, vcm);
					if (next_stage == AF_RUN_STAGE_COARSE) {
						set_focus_stage(AF_RUN_STAGE_TRY_POST);
					} else if (next_stage == AF_RUN_STAGE_FINE) {
						set_focus_stage(AF_RUN_STAGE_TRY_POST);
					} else if (next_stage == AF_RUN_STAGE_END) {
						set_focus_stage(AF_RUN_STAGE_END);
						goto af_end_out;
					}
					break;

				case AF_RUN_STAGE_TRY_POST:
					af_info.hold_cnt++;
					vcm_stable = ispv1_af_need_wait_stable(get_focus_stage(), af_info.hold_cnt);
					/* if vcm not stable, should wait next frame */
					if (vcm_stable == false) {
						break;
					}
					af_info.hold_cnt = 0; /* set zero for next stages */

					ispv1_af_range_cal(&af_info, top, next_stage);

					histtop->code = top->code;
					histtop->contrast = 0;
					set_focus_stage(next_stage);
					break;

				case AF_RUN_STAGE_COARSE:
					/* ensure top->contrast value is same with start point. */
					if (top->code == curr.code)
						top->contrast = curr.contrast;

					/* save contrast value to array */
					contrast_array[trip->step_cnt] = curr.contrast;

					next_stage = ispv1_af_search_top(&af_info, &curr, vcm, true);
					if (next_stage == AF_RUN_STAGE_FINE) {
						/* analysis contrast value */
						if (analysis_contrast_value(contrast_array, &af_info)) {
							set_focus_stage(AF_RUN_STAGE_COARSE_POST);
						} else {
							set_focus_stage(AF_RUN_STAGE_END);
						}
					} else if (next_stage == AF_RUN_STAGE_END) {
						analysis_contrast_value(contrast_array, &af_info);
						set_focus_stage(AF_RUN_STAGE_END);
					}
					break;

				case AF_RUN_STAGE_COARSE_POST:
					af_info.hold_cnt++;
					vcm_stable = ispv1_af_need_wait_stable(get_focus_stage(), af_info.hold_cnt);
					/* if vcm not stable, should wait next frame */
					if (vcm_stable == false) {
						break;
					}
					af_info.hold_cnt = 0; /* set zero for next stages */

					ispv1_af_range_cal(&af_info, &curr, next_stage);

					top->code = curr.code;
					top->contrast = 0;
					histtop->code = curr.code;
					histtop->contrast = 0;

					set_focus_stage(next_stage);
					break;

				case AF_RUN_STAGE_FINE:
					next_stage = ispv1_af_search_top(&af_info, &curr, vcm, false);
					if (next_stage != AF_RUN_STAGE_FINE) {
						set_focus_stage(AF_RUN_STAGE_END);
						goto af_end_out;
					}
					break;

af_end_out:
				case AF_RUN_STAGE_END:
					result = ispv1_af_judge_result(&af_info);
					set_focus_result(result);

					if (af_info.af_analysis & 0x04) {
						curr.code = vcm->offsetInit;
						ispv1_setreg_vcm_code(curr.code);
					}

					#ifdef AF_TIME_PRINT
						do_gettimeofday(&tv_end);
						print_info("*****focus TIME: %.4dms******, result %d",
							(int)((tv_end.tv_sec - tv_start.tv_sec)*1000 + (tv_end.tv_usec - tv_start.tv_usec) / 1000), result);
					#endif

					set_focus_stage(AF_RUN_STAGE_PREPARE);
					if ((get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_PICTURE)
						&& (get_focus_state() == FOCUS_STATE_CAF_RUNNING))
						set_focus_state(FOCUS_STATE_CAF_DETECTING);
					else {
						ispv1_set_aecagc_mode(AUTO_AECAGC);
						goto run_out;
					}
					break;
				case AF_RUN_STAGE_BREAK:
					set_focus_result(STATUS_OUT_FOCUS);
					print_info("picture AF_RUN_STAGE_BREAK");
					set_focus_stage(AF_RUN_STAGE_PREPARE);
					if (get_focus_mode() == CAMERA_FOCUS_CONTINUOUS_PICTURE)
						set_focus_state(FOCUS_STATE_CAF_DETECTING);
					else {
						ispv1_set_aecagc_mode(AUTO_AECAGC);
						goto run_out;
					}
					break;

				default:
					print_info("FOCUS_SCHEDULE_CASE_AF_PICTURE_MOVE wrong case");
					break;
			}
			break;

		case FOCUS_SCHEDULE_CASE_CAF_VIDEO_MOVE:

			if ((get_focus_stage() == VCAF_RUN_STAGE_TRY) || (get_focus_stage() == VCAF_RUN_STAGE_FORWARD)) {
				/* even frame should skip */
				if (frame_count++ % 2 == 0)
					break;
			}

		#ifdef YUV_EDGE_STAT_MODE  //use new k3 contrast calculate mode
			copy_ret = ispv1_copy_preview_data(pstat_data, rect);
			if (false == copy_ret)
				break;

			curr.contrast = ispv1_focus_calc_edge(pstat_data, rect, 0, edge_calc_coarse);
		#else //use ISP contrast mode
			if (get_focus_stage() == VCAF_RUN_STAGE_STARTUP) {
				if (frame_count++ % 2 == 0)
					break;
			}
			GETREG16(REG_ISP_FOCUS_CONTRAST(0), curr.contrast);
		#endif

			print_debug("VCAF move stage %d, dir %d, [0x%3x->0x%3x], curr[0x%.3x, 0x%.3x], top[0x%.3x, 0x%.3x]######",
				get_focus_stage(), trip->direction, trip->start_pos, trip->end_pos, curr.code, curr.contrast, top->code, top->contrast);

			switch (get_focus_stage()) {
			case VCAF_RUN_STAGE_PREPARE:
				#ifdef AF_TIME_PRINT
					do_gettimeofday(&tv_start);
				#endif
				/* CAF need wait AEC/AGC stable */
				aec_stable = GETREG8(REG_ISP_AECAGC_STABLE);
				if (aec_stable == 0) {
					break;
				}
				/*
				 * calculate init focus postion, and go to init position
				 * maybe offsetInit, maybe a little shift from current position
				 */
				next_code = curr.code;
				next_code = af_adjust_curr_vcmcode(vcm, next_code);
				if (next_code != curr.code) {
					ispv1_setreg_vcm_code(next_code);
					curr.code = next_code;
					af_info.hold_cnt = 0; /* should hold some frames to wait stable */
				}

			#ifdef YUV_EDGE_STAT_MODE  //use new k3 contrast calculate mode
				print_info("STAGE_PREPARE:goto stage %d(startup)###########", VCAF_RUN_STAGE_STARTUP);

				/*in this mode, we need not break, can goto CAF_RUN_STAGE_STARTUP directly. */
				set_focus_stage(VCAF_RUN_STAGE_STARTUP);
				/* set this frame is first frame, next frame need skip for stable */
				frame_count = 0;
			#else
				copy_ret = ispv1_copy_preview_data(pstat_data, rect);
				if (false == copy_ret)
					break;

				ispv1_focus_update_threshold_ext(pstat_data, rect->width * rect->height);
				set_focus_stage(VCAF_RUN_STAGE_STARTUP);

				/* set this frame is first frame, next frame need skip for stable */
				frame_count = 0;
				break;
			#endif

			case VCAF_RUN_STAGE_STARTUP:
				top->code = curr.code;
				top->contrast = curr.contrast;
				histtop->code = curr.code;
				histtop->contrast = curr.contrast;

				ispv1_vcaf_range_cal(&af_info, &curr, VCAF_RUN_STAGE_TRY, result);

				set_focus_result(STATUS_FOCUSING);
				ispv1_focus_get_curr_data(&start_data);

				af_get_next_vcmcode(trip, curr.code, trip->direction, AF_CODE_STEP_VIDEO, &next_code);
				ispv1_setreg_vcm_code(next_code); /* need not judge this code */
				curr.code = next_code;

				set_focus_stage(VCAF_RUN_STAGE_TRY);
				break;

			case VCAF_RUN_STAGE_TRY:
				contrast_array[trip->step_cnt++] = curr.contrast;
				next_stage = ispv1_vcaf_recognise_curve(&af_info, &curr, vcm, &tryrewind_code);
				set_focus_stage(next_stage);
				if (next_stage == VCAF_RUN_STAGE_REWIND) {
					if ((top->contrast < histtop->contrast) ||
					((top->contrast == histtop->contrast) && (top->code > histtop->code))) {
						top->contrast = histtop->contrast;
						top->code = histtop->code;
					}
					analysis_contrast_value(contrast_array, &af_info);
					result = ispv1_af_judge_result(&af_info);
					ispv1_vcaf_range_cal(&af_info, &curr, VCAF_RUN_STAGE_REWIND, result);
				}
				break;

			case VCAF_RUN_STAGE_TRYREWIND:
				if (curr.code != tryrewind_code) {
					af_get_next_vcmcode(trip, curr.code, trip->direction, AF_CODE_STEP_VIDEO, &next_code);
					ispv1_setreg_vcm_code(next_code);
					curr.code = next_code;
				} else {
					set_focus_stage(VCAF_RUN_STAGE_FORWARD);
				}
				break;

			case VCAF_RUN_STAGE_FORWARD:
				contrast_array[trip->step_cnt++] = curr.contrast;
				next_stage = ispv1_vcaf_search_top(&af_info, &curr, vcm);
				set_focus_stage(next_stage);
				if (next_stage == VCAF_RUN_STAGE_REWIND) {
					analysis_contrast_value(contrast_array, &af_info);
					result = ispv1_af_judge_result(&af_info);
					ispv1_vcaf_range_cal(&af_info, &curr, VCAF_RUN_STAGE_REWIND, result);
				}
				break;

			case VCAF_RUN_STAGE_REWIND:
				if (curr.code != top->code) {
					af_get_next_vcmcode(trip, curr.code, trip->direction, AF_CODE_STEP_VIDEO, &next_code);
					curr.code = next_code;
					ispv1_setreg_vcm_code(curr.code);
				} else {
					top->contrast = curr.contrast;
					set_focus_stage(VCAF_RUN_STAGE_PREPARE);
					if (get_focus_state() == FOCUS_STATE_CAF_RUNNING)
						set_focus_state(FOCUS_STATE_CAF_DETECTING);
					set_focus_result(STATUS_FOCUSED);
					ispv1_focus_get_curr_data(&curr_data);
					restart = ispv1_check_caf_need_restart(&start_data, &curr_data);
					if (restart == true)
						set_caf_forcestart(CAF_FORCESTART_FORCEWAIT);

					#ifdef AF_TIME_PRINT
						do_gettimeofday(&tv_end);
						print_info("*****focus TIME: %.4dms******",
							(int)((tv_end.tv_sec - tv_start.tv_sec)*1000 + (tv_end.tv_usec - tv_start.tv_usec) / 1000));
					#endif
				}
				break;
			case AF_RUN_STAGE_BREAK:
				set_focus_result(STATUS_FOCUSED);
				print_info("video AF_RUN_STAGE_BREAK");
				set_focus_stage(VCAF_RUN_STAGE_PREPARE);
				set_focus_state(FOCUS_STATE_CAF_DETECTING);
				break;

			default:
				print_info("error:unknow vcaf_stage %d ###########", get_focus_stage());
				break;
			}
			break;

		default:
			print_info("error:unknow af schedule waked up case %d ###########", schedule_case);
			goto run_out;
		}
	}
run_out:
	afae_ctrl->k3focus_running = false;
	print_info("exit %s, ###########", __func__);
	return;
}

/*
 **************************************************************************
 * FunctionName: ispv1_af_need_wait_stable
 * Description : VCM steering required to waiting for several frames at the specified location,
                       in order to prevent the inaccuracy of contrast caused by lens jitter.;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : 
 **************************************************************************
 */
static bool ispv1_af_need_wait_stable(u8 stage, u8 hold_cnt)
{
	u8 hold_frames = 0;

	switch(stage) {
		case AF_RUN_STAGE_PREPARE_POST:
			hold_frames = FOCUS_PARAM_STAGE_HOLDING_FRAMES+1;
			break;
		case AF_RUN_STAGE_TRY_POST:
		case AF_RUN_STAGE_COARSE_POST:
			hold_frames = FOCUS_PARAM_STAGE_HOLDING_FRAMES;
			break;
		default:
			print_info("enter %s, wrong stage", __func__);
			break;
	}

	if (hold_cnt < hold_frames) {
		return false;
	} else {
		return true;
	}
}

/*
 **************************************************************************
 * FunctionName: ispv1_af_judge_result;
 * Description : decide if there exists contrast peak during the AF process at last;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       :
 **************************************************************************
 */
static FOCUS_STATUS ispv1_af_judge_result(af_run_param *af_info)
{
	FOCUS_STATUS result;

	print_debug("enter %s, af_info->af_analysis:0x%x", __func__, af_info->af_analysis);
	if (af_info->af_analysis != 0) {
		result = STATUS_OUT_FOCUS;
		if (af_info->af_analysis & 0x01) {
			print_warn("slope of curve is small");
		}
		if (af_info->af_analysis & 0x02) {
			print_warn("exist several top contrast value");
		}
		if (af_info->af_analysis & 0x04) {
			print_warn("contrast diff is too low or env too dark.");
		}
	} else {
		result = STATUS_FOCUSED;
		print_debug("focus stop:val_vcm_top:0x%.3x, val_contrast:0x%.3x",\
			af_info->top.code, af_info->top.contrast);
	}
	return result;
}

int get_af_judge_threshold_low(int contrast)
{
	int threshold;
	int judge_base;

	if (contrast <= FOCUS_PARAM_ULTRALOW_CONTRAST) {
		judge_base = FOCUS_PARAM_JUDGE_BASE_LOW;
	} else if (contrast <= FOCUS_PARAM_LOW_CONTRAST) {
		judge_base = (contrast - FOCUS_PARAM_ULTRALOW_CONTRAST)
			* (FOCUS_PARAM_JUDGE_BASE_HIGH - FOCUS_PARAM_JUDGE_BASE_LOW)
			/ (FOCUS_PARAM_LOW_CONTRAST - FOCUS_PARAM_ULTRALOW_CONTRAST)
			+ FOCUS_PARAM_JUDGE_BASE_LOW;
	} else {
		judge_base = FOCUS_PARAM_JUDGE_BASE_HIGH;
	}

	threshold = (contrast)  * (judge_base - 1) / judge_base;
	return threshold;
}

int get_af_judge_threshold_high(int contrast)
{
	int threshold;
	int judge_base;

	if (contrast <= FOCUS_PARAM_ULTRALOW_CONTRAST) {
		judge_base = FOCUS_PARAM_JUDGE_BASE_LOW;
	} else if (contrast <= FOCUS_PARAM_LOW_CONTRAST) {
		judge_base = (contrast - FOCUS_PARAM_ULTRALOW_CONTRAST)
			* (FOCUS_PARAM_JUDGE_BASE_HIGH - FOCUS_PARAM_JUDGE_BASE_LOW)
			/ (FOCUS_PARAM_LOW_CONTRAST - FOCUS_PARAM_ULTRALOW_CONTRAST)
			+ FOCUS_PARAM_JUDGE_BASE_LOW;
	} else {
		judge_base = FOCUS_PARAM_JUDGE_BASE_HIGH;
	}

	threshold = (contrast)  * (judge_base + 1) / judge_base;
	return threshold;
}

/*
 **************************************************************************
 * FunctionName: ispv1_af_range_cal;
 * Description : initialize the parameters of running each range;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void ispv1_af_range_cal(af_run_param *af_info, pos_info *curr, af_run_stage next_stage)
{
	vcm_info_s *vcm = get_vcm_ptr();

	af_trip_info *trip = &(af_info->trip);
	pos_info *top = &(af_info->top);

	u32 focus_range = vcm->fullRange - vcm->offsetInit;
	u32 pos_offset = curr->code - vcm->offsetInit;

	switch (next_stage) {
	case AF_RUN_STAGE_TRY:
		trip->start_pos = curr->code;

		/* decide move direction in try stage, we want to move more steps */
		if (pos_offset < focus_range / 2) {
			trip->end_pos = vcm->fullRange;
			trip->direction = AF_RUN_DIRECTION_FORWARD;
		} else {
			trip->end_pos = vcm->offsetInit;
			trip->direction = AF_RUN_DIRECTION_BACKWARD;
		}
		trip->step = vcm->coarseStep;
		trip->step_cnt = 0;
		break;

	case AF_RUN_STAGE_COARSE:
		trip->start_pos = curr->code;
		if (trip->direction == AF_RUN_DIRECTION_FORWARD) {
			trip->end_pos = vcm->fullRange;
		} else {
			trip->end_pos = vcm->offsetInit;
		}

		trip->step = vcm->coarseStep;
		trip->step_cnt = 0;
		break;

	case AF_RUN_STAGE_FINE:
		if (trip->direction == AF_RUN_DIRECTION_BACKWARD) {
			trip->start_pos = top->code + vcm->fineStep;
			trip->end_pos = top->code - vcm->fineStep;
			if (trip->end_pos < vcm->offsetInit) {
				trip->end_pos = vcm->offsetInit;
			}
			if (trip->start_pos > vcm->fullRange) {
				trip->start_pos = vcm->fullRange;
			}
		} else {
			trip->start_pos = top->code - vcm->fineStep;
			trip->end_pos = top->code + vcm->fineStep;
			if (trip->start_pos < vcm->offsetInit) {
				trip->start_pos = vcm->offsetInit;
			}
			if (trip->end_pos > vcm->fullRange) {
				trip->end_pos = vcm->fullRange;
			}
		}

		trip->step = vcm->fineStep;
		trip->step_cnt = 0;
		break;

	default:
		print_error("wrong af run stage");
		break;
	}

	print_debug("next stage %d, start_pos:0x%.3x, end_pos:0x%.3x",next_stage, trip->start_pos, trip->end_pos);
}
static void ispv1_vcaf_range_cal(af_run_param *af_info, pos_info *curr, af_run_stage next_stage, FOCUS_STATUS result)
{
	vcm_info_s *vcm = get_vcm_ptr();
	af_trip_info *trip = &(af_info->trip);
	pos_info *top = &(af_info->top);

	switch (next_stage) {
	case VCAF_RUN_STAGE_TRY:
		trip->start_pos = curr->code;

		/* decide move direction in try stage, we want to move more steps */
		if (curr->code >= vcm->fullRange) {
			trip->end_pos = vcm->offsetInit;
			trip->direction = AF_RUN_DIRECTION_BACKWARD;
		} else {
			trip->end_pos = vcm->fullRange;
			trip->direction = AF_RUN_DIRECTION_FORWARD;
		}
		trip->step = vcm->coarseStep;
		trip->step_cnt = 0;
		break;

	/* other case is changed by ispv1_vcaf_recognise_curve() or ispv1_vcaf_search_top() */
	case VCAF_RUN_STAGE_TRYREWIND:
		break;

	case VCAF_RUN_STAGE_FORWARD:
		break;

	case VCAF_RUN_STAGE_REWIND:
		/* if out focus,than go to offsetinit position */
		if (result == STATUS_OUT_FOCUS) {
			top->code = vcm->offsetInit;
			trip->start_pos = curr->code;
			trip->end_pos = vcm->offsetInit;
			trip->direction = AF_RUN_DIRECTION_BACKWARD;
		}
		break;

	default:
		print_error("wrong video caf run stage");
		break;
	}

	print_debug("next stage %d, start_pos:0x%.3x, end_pos:0x%.3x",next_stage, trip->start_pos, trip->end_pos);
}

/*
 **************************************************************************
 * FunctionName: ispv1_af_recognise_curve;
 * Description : Used in the CAF "try" stage, identifying the rising or decling
 *	         trend of the contrast value;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : ret: 0-continue; 1-go to coarse stage; 2-go to fine stage;
 * Other       : NA;
 **************************************************************************
 */
static af_run_stage ispv1_af_recognise_curve(af_run_param *af_info, pos_info *curr, vcm_info_s *vcm)
{
	af_trip_info *trip = &(af_info->trip);
	pos_info *top = &(af_info->top);
	pos_info *histtop = &(af_info->histtop);

	u32 next_code = 0;
	bool code_found = false;
	af_run_stage next_stage = AF_RUN_STAGE_TRY;

	if (curr->contrast <= get_af_judge_threshold_low(top->contrast) ||
		(curr->contrast <= top->contrast  && check_vcmcode_is_edge(trip, curr->code) == true)) {
		/*
		 * should do following steps:
		 * (1) get actual top code;
		 * (2) turn back right direction, top code as start_pos;
		 * (3) go back to top code.
		 * (4) next_stage set as AF_RUN_STAGE_COARSE stage.
		 */

		if ((top->contrast < histtop->contrast) ||
			(top->contrast == histtop->contrast && top->code > histtop->code)) {
			top->contrast = histtop->contrast;
			top->code = histtop->code;
		}

		print_debug("try top code 0x%x found!!!!!!!", top->code);

		af_reverse_curr_trip(vcm, trip, top->code);
		curr->code = top->code;
		next_stage = AF_RUN_STAGE_COARSE;
	} else if (curr->contrast <= get_af_judge_threshold_high(top->contrast) &&
		check_vcmcode_is_edge(trip, curr->code) == false) {
		/* retry because diff is too small */
		next_stage = AF_RUN_STAGE_TRY;

		if (curr->contrast >= top->contrast && curr->contrast >= histtop->contrast) {
			histtop->contrast = curr->contrast;
			histtop->code = curr->code;
		}

		/* need not judge this code */
		(void)af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_COARSE, &next_code);
		trip->step_cnt++;
		curr->code = next_code;
	} else {
		/* update top info, set as stage FORWARD and goto next position. */
		top->contrast = curr->contrast;
		top->code = curr->code;

		code_found = af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_COARSE, &next_code);
		trip->step_cnt++;
		if (code_found == true) {
			next_stage = AF_RUN_STAGE_COARSE;
			curr->code = next_code;
			print_debug("%s line %d: go to STAGE COARSE!", __func__, __LINE__);
		} else {
			print_error("%s line %d:next vcmcode will out of range!!!!!!!", __func__, __LINE__);
			/* Top code is this, goto REWIND stage */
			next_stage = AF_RUN_STAGE_END;
			return next_stage;
		}
	}

	ispv1_setreg_vcm_code(curr->code);

	return next_stage;
}

static af_run_stage ispv1_vcaf_recognise_curve(af_run_param *af_info, pos_info *curr, vcm_info_s *vcm, u32 *reserved)
{
	af_trip_info *trip = &(af_info->trip);
	pos_info *top = &(af_info->top);
	pos_info *histtop = &(af_info->histtop);

	u32 next_code = 0;
	bool code_found = false;
	af_run_stage next_stage = VCAF_RUN_STAGE_TRY;

	if (curr->contrast <= get_af_judge_threshold_low(top->contrast) ||
		(curr->contrast <= top->contrast  && check_vcmcode_is_edge(trip, curr->code) == true)) {
		/*
		 * should do following steps:
		 * (1) get actual top code;
		 * (2) turn back right direction, top code as start_pos;
		 * (3) go back to top side code.
		 * (4) next_stage set as TRYREWIND stage.
		 */

		if ((top->contrast < histtop->contrast) ||
			(top->contrast == histtop->contrast && top->code > histtop->code)) {
			top->contrast = histtop->contrast;
			top->code = histtop->code;
		}

		print_debug("try top code 0x%x found!!!!!!!", top->code);

		af_reverse_curr_trip(vcm, trip, curr->code);
		print_info("trip [0x%x -> 0x%x, dir %d]!!!!!!!", trip->start_pos, trip->end_pos, trip->direction);

		code_found = af_get_next_vcmcode(trip, top->code, trip->direction, AF_CODE_STEP_VIDEO, reserved);
		if (code_found == true) {
			af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_VIDEO, &next_code);
			curr->code = next_code;
			print_info("try rewind to 0x%x!!!!!!!", *reserved);
			next_stage = VCAF_RUN_STAGE_TRYREWIND;
		} else {
			print_error("%s line %d:next vcmcode will out of range!!!!!!!", __func__, __LINE__);
			/* Top code is just top, set vcm to top and goto REWIND stage */
			curr->code = top->code;
			next_stage = VCAF_RUN_STAGE_REWIND;
		}
	} else if (curr->contrast <= get_af_judge_threshold_high(top->contrast) &&
		check_vcmcode_is_edge(trip, curr->code) == false) {
		/* retry because diff is too small */
		next_stage = VCAF_RUN_STAGE_TRY;

		if (curr->contrast >= top->contrast) {
			/* if current code is same as histtop, lower code has high priority */
			if ((curr->contrast > histtop->contrast) ||
				((curr->contrast == histtop->contrast) && (curr->code < histtop->code))) {
				histtop->contrast = curr->contrast;
				histtop->code = curr->code;
			}
		}

		/* need not judge this code */
		af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_VIDEO, &next_code);
		curr->code = next_code;
	} else {
		/* update top info, set as stage FORWARD and goto next position. */
		top->contrast = curr->contrast;
		top->code = curr->code;

		code_found = af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_VIDEO, &next_code);
		if (code_found == true) {
			next_stage = VCAF_RUN_STAGE_FORWARD;
			curr->code = next_code;
		} else {
			print_error("%s line %d:next vcmcode will out of range!!!!!!!", __func__, __LINE__);
			/* Top code is this, no need to set direction, goto REWIND stage */
			next_stage = VCAF_RUN_STAGE_REWIND;
			return next_stage;
		}
	}

	ispv1_setreg_vcm_code(curr->code);

	return next_stage;
}

/*
 **************************************************************************
 * FunctionName: ispv1_af_search_top;
 * Description : using for recognise top;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : ret 0: continuous; 1:change to next phase; 2: go to judge phase;
 * Other       :
 **************************************************************************
 */
static af_run_stage ispv1_af_search_top(af_run_param *af_info, pos_info *curr, vcm_info_s *vcm, bool coarse)
{
	af_trip_info *trip = &(af_info->trip);
	pos_info *top = &(af_info->top);
	pos_info *histtop = &(af_info->histtop);

	u32 next_code = 0;
	bool code_found = false;
	af_run_stage next_stage;

	/* set default next_stage value */
	if (coarse == true)
		next_stage = AF_RUN_STAGE_COARSE;
	else
		next_stage = AF_RUN_STAGE_FINE;

	if (curr->contrast <= get_af_judge_threshold_low(top->contrast) ||
		(curr->contrast <= top->contrast  && check_vcmcode_is_edge(trip, curr->code) == true)) {
		/* valid range run over or top condition is found, than change to next stage */
		if ((top->contrast < histtop->contrast) ||
			(top->contrast == histtop->contrast && top->code > histtop->code)) {
			top->contrast = histtop->contrast;
			top->code = histtop->code;
		}

		print_debug("top code 0x%x found!!!!!!!", top->code);

		if (coarse == true) {
			/* same direction, get top hand fine code(same side with current) */
			af_get_next_vcmcode(trip, top->code, trip->direction, AF_CODE_STEP_FINE, &next_code);
			/* turn back */
			trip->direction = 0 - trip->direction;
			next_stage = AF_RUN_STAGE_FINE;
		} else {
			next_code = top->code;
			next_stage = AF_RUN_STAGE_END;
		}

		curr->code = next_code;
		trip->step_cnt++;
		ispv1_setreg_vcm_code(curr->code);
	} else if (check_vcmcode_is_edge(trip, curr->code) == true) {
		/* reach edge code, no matter current is COARSE or FINE, stay here */
		/* valid range run over, last position is top position */
		print_info("reach edge code, coarse flag %d, top code is current 0x%x !!!!!!!", coarse, curr->code);
		trip->step_cnt++;
		next_stage = AF_RUN_STAGE_END;
		top->code = curr->code;
	} else if (curr->contrast <= top->contrast) {
		/* retry and goto next position, because diff is too small. */
		if (coarse == true)
			code_found = af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_COARSE, &next_code);
		else
			code_found = af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_FINE, &next_code);

		if (code_found == true) {
			if (curr->contrast >= top->contrast && curr->contrast >= histtop->contrast) {
				histtop->contrast = curr->contrast;
				histtop->code = curr->code;
			}
			curr->code = next_code;
			trip->step_cnt++;
			ispv1_setreg_vcm_code(curr->code);
		} else {
			print_error("%s line %d:next vcmcode will out of range!!!!!!!", __func__, __LINE__);
			/* Top code found: (1) go to next stage if in coarse stage; (2)rewind and goto END if in fine stage; */
			if (coarse == true) {
				/* same direction, get top hand code(same side with current) */
				af_get_next_vcmcode(trip, top->code, trip->direction, AF_CODE_STEP_FINE, &next_code);
				/* turn back */
				trip->direction = 0 - trip->direction;
				next_stage = AF_RUN_STAGE_FINE;
			} else {
				next_code = top->code;
				next_stage = AF_RUN_STAGE_END;
			}
			trip->step_cnt++;
			curr->code = next_code;
			ispv1_setreg_vcm_code(curr->code);
		}
	} else {
		/* update top info and goto next position. */
		top->contrast = curr->contrast;
		top->code = curr->code;

		if (coarse == true)
			code_found = af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_COARSE, &next_code);
		else
			code_found = af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_FINE, &next_code);

		if (code_found == true) {
			curr->code = next_code;
			trip->step_cnt++;
			ispv1_setreg_vcm_code(curr->code);
		} else {
			print_error("%s line %d:next vcmcode will out of range!!!!!!!", __func__, __LINE__);
			/* Top code is this, END stage */
			next_stage = AF_RUN_STAGE_END;
		}
	}
	return next_stage;
}

static af_run_stage ispv1_vcaf_search_top(af_run_param *af_info, pos_info *curr, vcm_info_s *vcm)
{
	af_trip_info *trip = &(af_info->trip);
	pos_info *top = &(af_info->top);
	pos_info *histtop = &(af_info->histtop);

	u32 next_code = 0;
	bool code_found = false;
	af_run_stage next_stage = VCAF_RUN_STAGE_FORWARD;

	if (curr->contrast <= get_af_judge_threshold_low(top->contrast) ||
		(curr->contrast <= top->contrast  && check_vcmcode_is_edge(trip, curr->code) == true)) {
		/* Top code found, goto REWIND, in this case, should not go back by 1step. */

		if ((top->contrast < histtop->contrast) ||
			(top->contrast == histtop->contrast && top->code > histtop->code)) {
			top->contrast = histtop->contrast;
			top->code = histtop->code;
		}

		/* prepare next code for rewind. */
		if (abs(top->code - curr->code) >= get_af_large_step(vcm->coarseStep, vcm->videoStrideRatio)) {
			if (top->code > curr->code)
				trip->direction = AF_RUN_DIRECTION_FORWARD;
			else
				trip->direction = AF_RUN_DIRECTION_BACKWARD;

			af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_VIDEO, &next_code);
		} else
			next_code = top->code;

		curr->code = next_code;

		print_debug("top code 0x%x found!!!!!!!", top->code);

		/* rewind to next code. */
		ispv1_setreg_vcm_code(curr->code);
		next_stage = VCAF_RUN_STAGE_REWIND;
	} else if (check_vcmcode_is_edge(trip, curr->code) == true) {
		/* reach edge code, stay here */
		next_stage = VCAF_RUN_STAGE_REWIND;
		top->code = curr->code;
	} else if (curr->contrast <= top->contrast) {
		/* retry and goto next position, because diff is too small. */
		code_found = af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_VIDEO, &next_code);
		if (code_found == true) {
			if (curr->contrast >= top->contrast && curr->contrast >= histtop->contrast) {
				histtop->contrast = curr->contrast;
				histtop->code = curr->code;
			}
			curr->code = next_code;
			ispv1_setreg_vcm_code(curr->code);
		} else {
			print_error("%s line %d:next vcmcode will out of range!!!!!!!", __func__, __LINE__);
			/* Top code found, goto REWIND, in this case, should not go back by 1step. */

			/* prepare next code for rewind. */
			if (abs(top->code - curr->code) >= get_af_large_step(vcm->coarseStep, vcm->videoStrideRatio)) {
				if (top->code > curr->code)
					trip->direction = AF_RUN_DIRECTION_FORWARD;
				else
					trip->direction = AF_RUN_DIRECTION_BACKWARD;

				af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_VIDEO, &next_code);
			} else
				next_code = top->code;

			curr->code = next_code;
			/* rewind to next code. */
			ispv1_setreg_vcm_code(curr->code);
			next_stage = VCAF_RUN_STAGE_REWIND;
		}
	} else {
		/* update top info and goto next position. */
		top->contrast = curr->contrast;
		top->code = curr->code;

		code_found = af_get_next_vcmcode(trip, curr->code, trip->direction, AF_CODE_STEP_VIDEO, &next_code);
		if (code_found == true) {
			curr->code = next_code;
			ispv1_setreg_vcm_code(curr->code);
		} else {
			print_error("%s line %d:next vcmcode will out of range!!!!!!!", __func__, __LINE__);
			/* Top code is this, goto REWIND stage */
			next_stage = VCAF_RUN_STAGE_REWIND;
		}
	}

	return next_stage;
}

/*
 **************************************************************************
 * FunctionName: ispv1_check_caf_need_restart
 * Description : decide whether restart caf
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : ret: false - do not need restart CAF, true - need restart CAF.
 * Other       : NA;
 **************************************************************************
 */
bool ispv1_check_caf_need_restart(focus_frame_stat_s *start_data, focus_frame_stat_s *end_data)
{
	u32 diff_x, diff_y, diff_z;
	u16 force_start = 0;

	diff_x = abs(start_data->xyz.x - end_data->xyz.x);
	diff_y = abs(start_data->xyz.y - end_data->xyz.y);
	diff_z = abs(start_data->xyz.z - end_data->xyz.z);

	/* If xyz diff too much, should force focus restart. */
	if (diff_x >= FOCUS_PARAM_CAF_RESTART_DIFF_XYZ)
		force_start |= 0x01;
	if (diff_y >= FOCUS_PARAM_CAF_RESTART_DIFF_XYZ)
		force_start |= 0x02;
	if (diff_z >= FOCUS_PARAM_CAF_RESTART_DIFF_XYZ)
		force_start |= 0x04;

	if (force_start) {
		print_info("CAF need restart reason: 0x%x", force_start);
		return true;
	} else {
		return false;
	}
}
