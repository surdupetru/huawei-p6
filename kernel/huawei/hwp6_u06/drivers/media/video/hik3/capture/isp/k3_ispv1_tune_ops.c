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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111 - 1307  USA
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

#include <mach/boardid.h>
#include "cam_util.h"
#include "cam_dbg.h"
#include "k3_isp.h"
#include "k3_ispv1.h"
#include "k3_ispv1_afae.h"
#include "k3_isp_io.h"
#include "k3_ispv1_hdr_movie_ae.h"
#include <hsad/config_interface.h>

#define DEBUG_DEBUG 0
#define LOG_TAG "K3_ISPV1_TUNE_OPS"
#include "cam_log.h"

k3_isp_data *this_ispdata;
static bool camera_ajustments_flag;

static int scene_target_y_low = DEFAULT_TARGET_Y_LOW;
static int scene_target_y_high = DEFAULT_TARGET_Y_HIGH;
bool fps_lock;

/* < zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight begin */
bool hw_lowlight_on_flg = false;
/* zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight end > */

static bool ispv1_change_frame_rate(
	camera_frame_rate_state *state, camera_frame_rate_dir direction, camera_sensor *sensor);
static int ispv1_set_frame_rate(camera_frame_rate_mode mode, camera_sensor *sensor);

static void ispv1_uv_stat_work_func(struct work_struct *work);
struct workqueue_struct *uv_stat_work_queue;
struct work_struct uv_stat_work;
/*
set parameter for products
FLASH_CAP_RAW_OVER_EXPO
DEFAULT_TARGET_Y_FLASH
*/
u32 flash_cap_raw_over_expo;
u32 default_target_y_flash;
#define EDGE_FLASH_CAP_RAW_OVER_EXPO  0xc0
#define EDGE_DEFAULT_TARGET_Y_FLASH      0x28
void set_tune_parameter_for_product(void)
{
	if(product_type("UEDGE"))
	{
		flash_cap_raw_over_expo = EDGE_FLASH_CAP_RAW_OVER_EXPO;
		default_target_y_flash = EDGE_DEFAULT_TARGET_Y_FLASH;
	}
	else if(product_type("TEDGE"))
	{
		flash_cap_raw_over_expo = EDGE_FLASH_CAP_RAW_OVER_EXPO;
		default_target_y_flash = EDGE_DEFAULT_TARGET_Y_FLASH;
	}
	else if(product_type("CEDGED")) 
	{
		flash_cap_raw_over_expo = EDGE_FLASH_CAP_RAW_OVER_EXPO;
		default_target_y_flash = EDGE_DEFAULT_TARGET_Y_FLASH;
	}
	else if(product_type("UEDGE_G"))
	{
		flash_cap_raw_over_expo = EDGE_FLASH_CAP_RAW_OVER_EXPO;
		default_target_y_flash = EDGE_DEFAULT_TARGET_Y_FLASH;
	}
	else
	{
		flash_cap_raw_over_expo = FLASH_CAP_RAW_OVER_EXPO;
		default_target_y_flash = DEFAULT_TARGET_Y_FLASH;
	}
}

/*
 * For anti-shaking, y36721 todo
 * ouput_width-2*blocksize
 * ouput_height-2*blocksize
*/
int ispv1_set_anti_shaking_block(int blocksize)
{
	camera_rect_s out, stat;
	u8 reg1;

	print_debug("enter %s", __func__);

	/* bit4: 0-before yuv downscale;1-after yuv dcw, but before yuv upscale */
	reg1 = GETREG8(REG_ISP_SCALE6_SELECT);
	reg1 |= (1 << 4);
	SETREG8(REG_ISP_SCALE6_SELECT, reg1);

	/* y36721 todo */
	out.left = 0;
	out.top = 0;

	out.width = blocksize;
	out.height = blocksize;

	k3_isp_antishaking_rect_out2stat(&out, &stat);

	SETREG8(REG_ISP_ANTI_SHAKING_BLOCK_SIZE, (stat.width & 0xff00) >> 8);
	SETREG8(REG_ISP_ANTI_SHAKING_BLOCK_SIZE + 1, stat.width & 0xff);

	/*
	 * y36721 todo
	 * should set REG_ISP_ANTI_SHAKING_ABSSTART_POSITION_H and V
	 * REG_ISP_ANTI_SHAKING_ABSSTART_POSITION_H
	 * REG_ISP_ANTI_SHAKING_ABSSTART_POSITION_V
	 */

	return 0;
}

int ispv1_set_anti_shaking(camera_anti_shaking flag)
{
	print_debug("enter %s", __func__);

	if (flag)
		SETREG8(REG_ISP_ANTI_SHAKING_ENABLE, 1);
	else
		SETREG8(REG_ISP_ANTI_SHAKING_ENABLE, 0);

	return 0;
}

/* read out anti-shaking coordinate */
int ispv1_get_anti_shaking_coordinate(coordinate_s *coordinate)
{
	u8 reg0;
	camera_rect_s out, stat;

	print_debug("enter %s", __func__);

	reg0 = GETREG8(REG_ISP_ANTI_SHAKING_ENABLE);

	if ((reg0 & 0x1) != 1) {
		print_error("anti_shaking not working!");
		return -1;
	}

	GETREG16(REG_ISP_ANTI_SHAKING_WIN_LEFT, stat.left);
	GETREG16(REG_ISP_ANTI_SHAKING_WIN_TOP, stat.top);
	stat.width = 0;
	stat.height = 0;

	k3_isp_antishaking_rect_stat2out(&out, &stat);

	coordinate->x = out.left;
	coordinate->y = out.top;

	return 0;

}

/* Added for ISO, target Y will not change with ISO */
int ispv1_set_iso(camera_iso iso)
{
	int max_iso, min_iso;
	int max_gain, min_gain;
	int retvalue = 0;
	camera_sensor *sensor;
	if (NULL == this_ispdata) {
		print_info("this_ispdata is NULL");
		return -1;
	}
	sensor = this_ispdata->sensor;

	print_debug("enter %s", __func__);

	/* < zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight begin */
	// when the ISO now is not AUTO, and capture mode changes from Single to Lowlight
	// do not set ISO to auto again, because we already doubled ISO value in set_iso_double_or_normal
	if (hw_lowlight_on_flg == true)
	{
		print_debug("%s: In lowlight mode, do not set ISO again!", __func__);
		return 0;
	}
	/* zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight end > */

	/* ISO is to change sensor gain, but is not same */
	switch (iso) {
	case CAMERA_ISO_AUTO:
		/* max and min iso should be overrided by sensor definitions */
		if (sensor->get_override_param) {
			/*iso limit is also sensor related, sensor itself should provide a method to define its iso limitation*/
			max_iso = sensor->get_override_param(OVERRIDE_ISO_HIGH);
			min_iso = sensor->get_override_param(OVERRIDE_ISO_LOW);
		} else {
			/*if sensor does not  provide this method,make sure that the default iso shuold be set here */
			max_iso = CAMERA_MAX_ISO;
			min_iso = CAMERA_MIN_ISO;
		}
		break;

	case CAMERA_ISO_100:
		max_iso = (100 + (100 / 8) * 2);
		min_iso = 100;
		break;

	case CAMERA_ISO_200:
		/* max and min iso should be fixed ISO200 */
		max_iso = (200 + 200 / 8);
		min_iso = (200 - 200 / 8);
		break;

	case CAMERA_ISO_400:
		/* max and min iso should be fixed ISO400 */
		max_iso = (400 + 400 / 8);
		min_iso = (400 - 400 / 8);
		break;

	case CAMERA_ISO_800:
		/* max and min iso should be fixed ISO800 */
		max_iso = 775;
		min_iso = 650;
		break;

	default:
		retvalue = -1;
		goto out;
		break;
	}

	if (sensor->hynix_set_lsc_shading) {
		sensor->hynix_set_lsc_shading(iso);
	}
	max_gain = max_iso * 0x10 / 100;
	min_gain = min_iso * 0x10 / 100;

	sensor->min_gain = min_gain;
	sensor->max_gain = max_gain;

	if(BINNING_SUMMARY== sensor->support_binning_type)
	{
		if(sensor->frmsize_list[sensor->preview_frmsize_index].binning == false)
		{
			max_gain = max_gain *2;
		}
	}
	/* set to ISP registers */
	SETREG8(REG_ISP_MAX_GAIN, (max_gain & 0xff00) >> 8);
	SETREG8(REG_ISP_MAX_GAIN + 1, max_gain & 0xff);
	SETREG8(REG_ISP_MIN_GAIN, (min_gain & 0xff00) >> 8);
	SETREG8(REG_ISP_MIN_GAIN + 1, min_gain & 0xff);


out:
	return retvalue;
}

int inline ispv1_iso2gain(int iso, bool binning)
{
	int gain;
	camera_sensor *sensor;
	if (NULL == this_ispdata) {
		print_info("this_ispdata is NULL");
		return -1;
	}
	sensor = this_ispdata->sensor;
	if(BINNING_SUMMARY== sensor->support_binning_type)
	{
		if (binning == false)
		iso *= 2;
	}
	gain = iso * 0x10 / 100;
	return gain;
}

int inline ispv1_gain2iso(int gain, bool binning)
{
	int iso;
	camera_sensor *sensor;
	if (NULL == this_ispdata) {
		print_info("this_ispdata is NULL");
		return -1;
	}
	sensor = this_ispdata->sensor;

	iso = gain * 100 / 0x10;

	if(BINNING_SUMMARY== sensor->support_binning_type)
	{
		if (binning == false)
			iso /= 2;
	}
	iso = (iso + 5) / 10 * 10;
	return iso;
}

u32 get_writeback_cap_gain(void)
{
	u8 reg5 = GETREG8(COMMAND_REG5);
	u32 gain = 0;

	reg5 &= 0x3;
	if (reg5 == 0 || reg5 == 2) {
		GETREG16(ISP_FIRST_FRAME_GAIN, gain);
	} else if (reg5 == 1) {
		GETREG16(ISP_SECOND_FRAME_GAIN, gain);
	} else if (reg5 == 3) {
		GETREG16(ISP_THIRD_FRAME_GAIN, gain);
	}

	return gain;
}

u32 get_writeback_cap_expo(void)
{
	u8 reg5 = GETREG8(COMMAND_REG5);
	u32 expo = 0;

	reg5 &= 0x3;
	if (reg5 == 0 || reg5 == 2) {
		GETREG32(ISP_FIRST_FRAME_EXPOSURE, expo);
	} else if (reg5 == 1) {
		GETREG32(ISP_SECOND_FRAME_EXPOSURE, expo);
	} else if (reg5 == 3) {
		GETREG32(ISP_THIRD_FRAME_EXPOSURE, expo);
	}

	return expo;
}

int ispv1_get_actual_iso(void)
{
	camera_sensor *sensor = this_ispdata->sensor;
	u16 gain;
	int iso;
	int index;
	bool binning;

	if (isp_hw_data.cur_state == STATE_PREVIEW) {
		index = sensor->preview_frmsize_index;
		gain = get_writeback_gain();
	} else {
		index = sensor->capture_frmsize_index;
		gain = get_writeback_cap_gain();
	}
	binning = sensor->frmsize_list[index].binning;

	iso = ispv1_gain2iso(gain, binning);
	return iso;
}
int ispv1_get_hdr_iso_exp(hdr_para_reserved *iso_exp)
{
	camera_sensor *sensor = this_ispdata->sensor;
	u16 gain;
	u32 expo, fps, vts;
	int index;
	bool binning;

	//get iso value
	index = sensor->capture_frmsize_index;
	binning = sensor->frmsize_list[index].binning;

	GETREG16(ISP_FIRST_FRAME_GAIN, gain);
	iso_exp->iso_speed[0] = ispv1_gain2iso(gain, binning);

	GETREG16(ISP_SECOND_FRAME_GAIN, gain);
	iso_exp->iso_speed[1] = ispv1_gain2iso(gain, binning);

	GETREG16(ISP_THIRD_FRAME_GAIN, gain);
	iso_exp->iso_speed[2] = ispv1_gain2iso(gain, binning);

	//get exp value
	fps = sensor->frmsize_list[index].fps;
	vts = sensor->frmsize_list[index].vts;

	GETREG32(ISP_FIRST_FRAME_EXPOSURE, expo);
	expo >>= 4;
	iso_exp->exposure_time[0] = ispv1_expo_line2time(expo, fps, vts);

	GETREG32(ISP_SECOND_FRAME_EXPOSURE, expo);
	expo >>= 4;
	iso_exp->exposure_time[1] = ispv1_expo_line2time(expo, fps, vts);

	GETREG32(ISP_THIRD_FRAME_EXPOSURE, expo);
	expo >>= 4;
	iso_exp->exposure_time[2] = ispv1_expo_line2time(expo, fps, vts);

	return 0;
}
int ispv1_get_exposure_time(void)
{
	u32 expo, fps, vts, index;
	int denominator_expo_time;
	camera_sensor *sensor = this_ispdata->sensor;

	if (isp_hw_data.cur_state == STATE_PREVIEW) {
		index = sensor->preview_frmsize_index;
		expo = get_writeback_expo();
	} else {
		index = sensor->capture_frmsize_index;
		expo = get_writeback_cap_expo();
	}

	expo >>= 4;

	fps = sensor->frmsize_list[index].fps;
	vts = sensor->frmsize_list[index].vts;

	denominator_expo_time = ispv1_expo_line2time(expo, fps, vts);
	return denominator_expo_time;
}
u32 ispv1_get_awb_gain(int withShift)
{
	u16 b_gain, r_gain;
	u32 return_val;
	if (withShift) {
		GETREG16(REG_ISP_AWB_GAIN_B, b_gain);
		GETREG16(REG_ISP_AWB_GAIN_R, r_gain);
	} else {
		GETREG16(REG_ISP_AWB_ORI_GAIN_B, b_gain);
		GETREG16(REG_ISP_AWB_ORI_GAIN_R, r_gain);
	}
	return_val = (b_gain << 16) | r_gain;
	return return_val;
}

u32 ispv1_get_expo_line(void)
{
	u32 ret;

	ret = get_writeback_expo();
	return ret;
}

u32 ispv1_get_sensor_vts(void)
{
	camera_sensor *sensor;
	u32 frame_index;
	u32 full_fps, fps;
	u32 basic_vts;
	u32 vts;

	if (NULL == this_ispdata) {
		print_info("this_ispdata is NULL");
		return 0;
	}
	sensor = this_ispdata->sensor;

	if (isp_hw_data.cur_state == STATE_PREVIEW)
		frame_index = sensor->preview_frmsize_index;
	else
		frame_index = sensor->capture_frmsize_index;

	full_fps = sensor->frmsize_list[frame_index].fps;
	basic_vts = sensor->frmsize_list[frame_index].vts;
	fps = sensor->fps;
	vts = basic_vts * full_fps / fps;

	return vts;
}

u32 ispv1_get_current_ccm_rgain(void)
{
	return GETREG8(REG_ISP_CCM_PREGAIN_R);
}

u32 ispv1_get_current_ccm_bgain(void)
{
	return GETREG8(REG_ISP_CCM_PREGAIN_B);
}

/*
 **************************************************************************
 * FunctionName: ispv1_get_sensor_lux_matrix;
 * Description : get lux matrix from sensor ,value is 14bit;
 * Input       : the space pointer of lux_matrix;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       :
 **************************************************************************
 */
int  ispv1_get_sensor_lux_matrix(lux_stat_matrix_tbl * lux_matrix)
{
	int  i;
	int size = 0;
	camera_sensor *sensor;

	if (NULL == this_ispdata) {
		print_info("this_ispdata is NULL");
		return -1;
	}
	sensor = this_ispdata->sensor;
       size = sensor->lux_matrix.size;
       for( i = 0;i < size ; i++)
       {
          lux_matrix->matrix_table[i] = sensor->lux_matrix.matrix_table[i];
	}
	 lux_matrix->size = size;
	return 0;
}
/*
 **************************************************************************
 * FunctionName: ispv1_get_sensor_hdr_points;
 * Description : get hdr control points from sensor.
 * Input       : the space pointer of atr_ctrl_points
 * Output      : NA;
 * ReturnValue : NA;
 * Other       :
 **************************************************************************
 */
int  ispv1_get_sensor_hdr_points(atr_ctrl_points* atr_points)
{
	camera_sensor *sensor;

	if (NULL == this_ispdata) {
		print_info("this_ispdata is NULL");
		return -1;
	}

	if(atr_points == NULL){
		print_error("atr_ctrl_points is NULL");
		return -1;
	}
	sensor = this_ispdata->sensor;

	atr_points->tc_out_noise = sensor->hdr_points.tc_out_noise;
	atr_points->tc_out_mid = sensor->hdr_points.tc_out_mid;
	atr_points->tc_out_sat = sensor->hdr_points.tc_out_sat;
	atr_points->tc_noise_brate= sensor->hdr_points.tc_noise_brate;
	atr_points->tc_mid_brate= sensor->hdr_points.tc_mid_brate;
	atr_points->tc_sat_brate= sensor->hdr_points.tc_sat_brate;
	atr_points->ae_sat= sensor->hdr_points.ae_sat;
	atr_points->wb_lmt= sensor->hdr_points.wb_lmt;
	return 0;
}
/*
 **************************************************************************
 * FunctionName: ispv1_get_sensor_hdr_info;
 * Description : get hdr info from sensor that hdr info include long shutter,long gain short shutter,short gain hdr on status,atr on status, ae target
 * Input       : the space pointer of hdr_info
 * Output      : NA;
 * ReturnValue : NA;
 * Other       :
 **************************************************************************
 */
int  ispv1_get_sensor_hdr_info(hdr_info* hdrInfo)
{
	camera_sensor *sensor;

	if (NULL == this_ispdata) {
		print_info("this_ispdata is NULL");
		return -1;
	}

	if(hdrInfo == NULL){
		print_error("hdr_info is NULL");
		return -1;
	}
	sensor = this_ispdata->sensor;

	hdrInfo->hdr_on = sensor->hdrInfo.hdr_on;
	hdrInfo->atr_on = sensor->hdrInfo.atr_on;
	hdrInfo->gain = sensor->hdrInfo.gain;
	hdrInfo->expo= sensor->hdrInfo.expo;
	hdrInfo->short_gain = sensor->hdrInfo.short_gain;
	hdrInfo->short_expo= sensor->hdrInfo.short_expo;
	hdrInfo->ae_target = sensor->hdrInfo.ae_target;
	hdrInfo->avgLux_ave= sensor->hdrInfo.avgLux_ave;
	hdrInfo->avgLux_weight= sensor->hdrInfo.avgLux_weight;
	hdrInfo->stats_max= sensor->hdrInfo.stats_max;
	hdrInfo->stats_diff= sensor->hdrInfo.stats_diff;
	hdrInfo->atr_over_expo_on= sensor->hdrInfo.atr_over_expo_on;
	hdrInfo->gainBeforeAjust = sensor->hdrInfo.gainBeforeAjust;
	hdrInfo->wb_lmt  = sensor->hdrInfo.wb_lmt;
	hdrInfo->ae_sat  = sensor->hdrInfo.ae_sat;

	hdrInfo->N_digital_h  	= sensor->hdrInfo.N_digital_h;
	hdrInfo->N_digital_l    = sensor->hdrInfo.N_digital_l;
	return 0;
}
/* Added for EV: exposure compensation.
 * Just set as this: ev+2 add 40%, ev+1 add 20% to current target Y
 * should config targetY and step
 */
void ispv1_calc_ev(u8 *target_low, u8 *target_high, int ev)
{
#ifdef OVISP_DEBUG_MODE
	return 0;
#endif
	int target_y_low = DEFAULT_TARGET_Y_LOW;
	int target_y_high = DEFAULT_TARGET_Y_HIGH;

	switch (ev) {
	case -2:
		target_y_low = DEFAULT_TARGET_Y_LOW - 0x10;
		target_y_low *= (EV_RATIO_NUMERATOR * EV_RATIO_NUMERATOR);
		target_y_low /= (EV_RATIO_DENOMINATOR * EV_RATIO_DENOMINATOR);
		target_y_high = target_y_low + 2;
		break;

	case -1:
		target_y_low = DEFAULT_TARGET_Y_LOW - 0x06;
		target_y_low *= EV_RATIO_NUMERATOR;
		target_y_low /= EV_RATIO_DENOMINATOR;
		target_y_high = target_y_low + 2;
		break;

	case 0:
		break;

	case 1:
		target_y_low = DEFAULT_TARGET_Y_LOW + 0x18;
		target_y_low *= EV_RATIO_DENOMINATOR;
		target_y_low /= EV_RATIO_NUMERATOR;
		target_y_high = target_y_low + 2;
		break;

	case 2:
		target_y_low = DEFAULT_TARGET_Y_LOW + 0x0e;
		target_y_low *= (EV_RATIO_DENOMINATOR * EV_RATIO_DENOMINATOR);
		target_y_low /= (EV_RATIO_NUMERATOR * EV_RATIO_NUMERATOR);
		target_y_high = target_y_low + 2;
		break;

	default:
		print_error("ev invalid");
		break;
	}

	*target_low = target_y_low;
	*target_high = target_y_high;
}

/* Added for EV: exposure compensation.
 * Just set as this: ev+2 add 40%, ev+1 add 20% to current target Y
 * should config targetY and step
 */
int ispv1_set_ev(int ev)
{
#ifdef OVISP_DEBUG_MODE
	return 0;
#endif
	u8 target_y_low, target_y_high;
	int ret = 0;

	print_debug("enter %s", __func__);

	/* ev is target exposure compensation value, decided by exposure time and sensor gain */
	ispv1_calc_ev(&target_y_low, &target_y_high, ev);

	if ((ev == 0) && (this_ispdata->ev != 0)) {
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_low);
		msleep(100);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
	} else {
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
	}

	scene_target_y_low = target_y_low;
	scene_target_y_high = target_y_high;

	return ret;
}

static void ispv1_init_sensor_config(camera_sensor *sensor)
{
	/* Enable AECAGC */
	SETREG8(REG_ISP_AECAGC_MANUAL_ENABLE, AUTO_AECAGC);

	/* new firmware support isp write all sensors's AEC/AGC registers. */
	if ((0 == sensor->aec_addr[0] && 0 == sensor->aec_addr[1] && 0 == sensor->aec_addr[2]) ||
		(0 == sensor->agc_addr[0] && 0 == sensor->agc_addr[1])) {
		SETREG8(REG_ISP_AECAGC_WRITESENSOR_ENABLE, ISP_WRITESENSOR_DISABLE);
	} else {
		SETREG8(REG_ISP_AECAGC_WRITESENSOR_ENABLE, ISP_WRITESENSOR_ENABLE);
	}
	/* changed by y00231328. differ from sensor type and bayer order. */
		SETREG8(REG_ISP_TOP6, sensor->sensor_rgb_type);

	SETREG16(REG_ISP_AEC_ADDR0, sensor->aec_addr[0]);
	SETREG16(REG_ISP_AEC_ADDR1, sensor->aec_addr[1]);
	SETREG16(REG_ISP_AEC_ADDR2, sensor->aec_addr[2]);

	SETREG16(REG_ISP_AGC_ADDR0, sensor->agc_addr[0]);
	SETREG16(REG_ISP_AGC_ADDR1, sensor->agc_addr[1]);

	if (0 == sensor->aec_addr[0])
		SETREG8(REG_ISP_AEC_MASK_0, 0x00);
	else
		SETREG8(REG_ISP_AEC_MASK_0, 0xff);

	if (0 == sensor->aec_addr[1])
		SETREG8(REG_ISP_AEC_MASK_1, 0x00);
	else
		SETREG8(REG_ISP_AEC_MASK_1, 0xff);

	if (0 == sensor->aec_addr[2])
		SETREG8(REG_ISP_AEC_MASK_2, 0x00);
	else
		SETREG8(REG_ISP_AEC_MASK_2, 0xff);

	if (0 == sensor->agc_addr[0])
		SETREG8(REG_ISP_AGC_MASK_H, 0x00);
	else
		SETREG8(REG_ISP_AGC_MASK_H, 0xff);

	if (0 == sensor->agc_addr[1])
		SETREG8(REG_ISP_AGC_MASK_L, 0x00);
	else
		SETREG8(REG_ISP_AGC_MASK_L, 0xff);

	/* changed by y00231328. some sensor such as S5K4E1, expo and gain active function is like sony type */
	if (sensor->sensor_type == SENSOR_LIKE_OV)
		SETREG8(REG_ISP_AGC_SENSOR_TYPE, SENSOR_OV);
	else if (sensor->sensor_type == SENSOR_LIKE_SONY)
		SETREG8(REG_ISP_AGC_SENSOR_TYPE, SENSOR_SONY);
	else
		SETREG8(REG_ISP_AGC_SENSOR_TYPE, sensor->sensor_type);
}

/* Added for anti_banding. y36721 todo */
int ispv1_set_anti_banding(camera_anti_banding banding)
{
	u32 op = 0;

	print_debug("enter %s", __func__);

	switch (banding) {
	case CAMERA_ANTI_BANDING_OFF:
		op = 0;
		SETREG8(REG_ISP_BANDFILTER_SHORT_EN, 0x1);
		break;

	case CAMERA_ANTI_BANDING_50Hz:
		op = 2;
		SETREG8(REG_ISP_BANDFILTER_SHORT_EN, 0x0);
		break;

	case CAMERA_ANTI_BANDING_60Hz:
		op = 1;
		SETREG8(REG_ISP_BANDFILTER_SHORT_EN, 0x0);
		break;

	case CAMERA_ANTI_BANDING_AUTO:
		/* y36721 todo */
		break;

	default:
		return -1;
	}

	SETREG8(REG_ISP_BANDFILTER_EN, 0x1);
	SETREG8(REG_ISP_BANDFILTER_FLAG, op);

	return 0;
}

int ispv1_get_anti_banding(void)
{
	u32 op = 0;
	camera_anti_banding banding;

	print_debug("enter %s", __func__);

	op = GETREG8(REG_ISP_BANDFILTER_FLAG);

	switch (op) {
	case 0:
		banding = CAMERA_ANTI_BANDING_OFF;
		break;

	case 1:
		banding = CAMERA_ANTI_BANDING_60Hz;
		break;

	case 2:
		banding = CAMERA_ANTI_BANDING_50Hz;
		break;
	default:
		return -1;
	}

	return banding;
}

/* blue,green,red gains */
u16 isp_mwb_gain[CAMERA_WHITEBALANCE_MAX][3] = {
	{0x0000, 0x0000, 0x0000}, /* AWB not care about it */
	{0x012c, 0x0080, 0x0089}, /* INCANDESCENT 2800K */
	{0x00f2, 0x0080, 0x00b9}, /* FLUORESCENT 4200K */
	{0x00a0, 0x00a0, 0x00a0}, /* WARM_FLUORESCENT, y36721 todo */
	{0x00d1, 0x0080, 0x00d2}, /* DAYLIGHT 5000K */
	{0x00b0, 0x0080, 0x00ec}, /* CLOUDY_DAYLIGHT 6500K*/
	{0x00a0, 0x00a0, 0x00a0}, /* TWILIGHT, y36721 todo */
	{0x0168, 0x0080, 0x0060}, /* CANDLELIGHT, 2300K */
};

#if 1
/* Added for awb */
int ispv1_set_awb(camera_white_balance awb_mode)
{
	print_debug("enter %s", __func__);
	/* default is auto, ...... */

#ifdef OVISP_DEBUG_MODE
	return 0;
#endif

	switch (awb_mode) {
	case CAMERA_WHITEBALANCE_AUTO:
		SETREG8(REG_ISP_AWB_MANUAL_ENABLE, 0x0);
		/*  Awb mode, should set CT-based AWB */
		SETREG8(REG_ISP_AWB_METHOD_TYPE, 0x1);
		break;

	case CAMERA_WHITEBALANCE_INCANDESCENT:
	case CAMERA_WHITEBALANCE_FLUORESCENT:
	case CAMERA_WHITEBALANCE_WARM_FLUORESCENT:
	case CAMERA_WHITEBALANCE_DAYLIGHT:
	case CAMERA_WHITEBALANCE_CLOUDY_DAYLIGHT:
	case CAMERA_WHITEBALANCE_TWILIGHT:
	case CAMERA_WHITEBALANCE_CANDLELIGHT:
		SETREG8(REG_ISP_AWB_MANUAL_ENABLE, 0x0);	/* y36721 fix it */
		SETREG8(REG_ISP_AWB_METHOD_TYPE, awb_mode + 2);
		SETREG16(REG_ISP_AWB_MANUAL_GAIN_BLUE(awb_mode - 1),
			 isp_mwb_gain[awb_mode][0]);
		SETREG16(REG_ISP_AWB_MANUAL_GAIN_GREEN(awb_mode - 1),
			 isp_mwb_gain[awb_mode][1]);
		SETREG16(REG_ISP_AWB_MANUAL_GAIN_RED(awb_mode - 1),
			 isp_mwb_gain[awb_mode][2]);
		break;

	default:
		print_error("unknow awb mode\n");
		return -1;
		break;
	}

	return 0;
}
#else
int ispv1_set_awb(camera_white_balance awb_mode)
{
	print_info("enter %s, awb mode", __func__, awb_mode);
	/* default is auto, ...... */

	switch (awb_mode) {
	case CAMERA_WHITEBALANCE_AUTO:
		SETREG8(REG_ISP_AWB_MANUAL_ENABLE, 0x0);
		/*  Awb mode, should set CT-based AWB */
		SETREG8(REG_ISP_AWB_METHOD_TYPE, 0x3);
		break;

	case CAMERA_WHITEBALANCE_INCANDESCENT:
		SETREG8(REG_ISP_AWB_MANUAL_ENABLE, 0x0);
		/*  Awb mode, should set CT-based AWB */
		SETREG8(REG_ISP_AWB_METHOD_TYPE, 0x4);
		break;
	case CAMERA_WHITEBALANCE_DAYLIGHT:
		SETREG8(REG_ISP_AWB_MANUAL_ENABLE, 0x0);
		/*  Awb mode, should set CT-based AWB */
		SETREG8(REG_ISP_AWB_METHOD_TYPE, 0x5);
		break;

	case CAMERA_WHITEBALANCE_FLUORESCENT:
		SETREG8(REG_ISP_AWB_MANUAL_ENABLE, 0x0);
		/*  Awb mode, should set CT-based AWB */
		SETREG8(REG_ISP_AWB_METHOD_TYPE, 0x6);
		break;

	case CAMERA_WHITEBALANCE_CLOUDY_DAYLIGHT:
		SETREG8(REG_ISP_AWB_MANUAL_ENABLE, 0x0);
		/*  Awb mode, should set CT-based AWB */
		SETREG8(REG_ISP_AWB_METHOD_TYPE, 0x7);
		break;

	default:
		print_error("unknow awb mode\n");
		return -1;
		break;
	}

	return 0;
}
#endif

/* < zhoutian 00195335 12-7-16 added for auto scene detect begin */
int ispv1_get_extra_coff(extra_coff *extra_data)
{
	print_debug("enter %s", __func__);

	extra_data->mean_y = GETREG8(0x1c75e);
	extra_data->motion_x = (s8)GETREG8(0x1cc7c);
	extra_data->motion_y = (s8)GETREG8(0x1cc7d);

	extra_data->focal_length = ispv1_get_focus_distance();
	extra_data->af_window_change = GETREG8(0x1cdd2);
	
	print_debug("mean_y = %d, motion_x = %d, motion_y = %d \n", extra_data->mean_y, extra_data->motion_x, extra_data->motion_y);
	print_debug("focal_length = %d \n", extra_data->focal_length);

	return 0;
}


int ispv1_get_ae_coff(ae_coff *ae_data)
{
	print_debug("enter %s", __func__);

	GETREG16(0x1c158, ae_data->exposure_max);
	GETREG16(0x1c15c, ae_data->exposure_min);
	GETREG16(0x1c150, ae_data->gain_max);
	GETREG16(0x1c154, ae_data->gain_min);
	ae_data->luma_target_high = GETREG8(REG_ISP_TARGET_Y_HIGH);
	ae_data->luma_target_low = GETREG8(REG_ISP_TARGET_Y_LOW);
	ae_data->exposure = get_writeback_expo() >> 4;
	ae_data->exposure_time = ispv1_get_exposure_time();
	ae_data->gain = get_writeback_gain();
	ae_data->iso = ispv1_get_actual_iso();

	return 0;
}


int ispv1_set_ae_coff(ae_coff *ae_data)
{
	print_debug("enter %s", __func__);

	u8 target_y_high = ae_data->luma_target_high;
	u8 target_y_low = ae_data->luma_target_low;	

	print_debug("target_y_high = %d, target_y_low = %d \n", target_y_high, target_y_low);

	SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
	SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);

	return 0;
}


int ispv1_get_awb_coff(awb_coff *awb_data)
{
	print_debug("enter %s", __func__);

	GETREG16(0x66c84, awb_data->auto_blue_gain);
	GETREG16(0x66c86, awb_data->auto_green1_gain);
	GETREG16(0x66c88, awb_data->auto_green2_gain);
	GETREG16(0x66c8a, awb_data->auto_red_gain);

	print_debug("auto_blue_gain = %d, auto_green1_gain = %d, auto_green2_gain = %d, auto_red_gain = %d \n"
		, awb_data->auto_blue_gain, awb_data->auto_green1_gain, awb_data->auto_green2_gain, awb_data->auto_red_gain);

	return 0;
}


int ispv1_set_awb_coff(awb_coff *awb_data)
{
	print_debug("enter %s", __func__);

	SETREG8(0x1c5ac, awb_data->blue_gain_ratio);
	SETREG8(0x1c5ad, awb_data->green_gain_ratio);
	SETREG8(0x1c5ae, awb_data->red_gain_ratio);

	return 0;
}


int ispv1_get_awb_ct_coff(awb_ct_coff *awb_ct_data)
{
	print_debug("enter %s", __func__);

	awb_ct_data->avg_all_en = GETREG8(0x66201);
	awb_ct_data->awb_window = GETREG8(0x66203);

	awb_ct_data->awb_b_block = 	GETREG8(0x66200);
	awb_ct_data->awb_s = 		GETREG8(0x1d925);
	awb_ct_data->awb_ec = 		GETREG8(0x1d928);
	awb_ct_data->awb_fc = 		GETREG8(0x1d92b);
	awb_ct_data->awb_x0 = 		GETREG8(0x66209);
	awb_ct_data->awb_y0 = 		GETREG8(0x6620a);
	awb_ct_data->awb_kx = 		GETREG8(0x6620b);
	awb_ct_data->awb_ky = 		GETREG8(0x6620c);
	awb_ct_data->day_limit = 		GETREG8(0x1d92e);
	awb_ct_data->a_limit = 		GETREG8(0x1d931);
	awb_ct_data->day_split = 		GETREG8(0x1d934);
	awb_ct_data->a_split = 		GETREG8(0x1d937);
	awb_ct_data->awb_top_limit = 	GETREG8(0x66211);
	awb_ct_data->awb_bot_limit = 	GETREG8(0x66212);

	return 0;
}


int ispv1_set_awb_ct_coff(awb_ct_coff *awb_ct_data)
{
	print_debug("enter %s", __func__);

	SETREG8(0x66201, awb_ct_data->avg_all_en);
	SETREG8(0x66203, awb_ct_data->awb_window);

	SETREG8(0x66200, awb_ct_data->awb_b_block);
	SETREG8(0x1d925, awb_ct_data->awb_s);
	SETREG8(0x1d928, awb_ct_data->awb_ec);
	SETREG8(0x1d92b, awb_ct_data->awb_fc);
	SETREG8(0x66209, awb_ct_data->awb_x0);
	SETREG8(0x6620a, awb_ct_data->awb_y0);
	SETREG8(0x6620b, awb_ct_data->awb_kx);
	SETREG8(0x6620c, awb_ct_data->awb_ky);
	SETREG8(0x1d92e, awb_ct_data->day_limit);
	SETREG8(0x1d931, awb_ct_data->a_limit);
	SETREG8(0x1d934, awb_ct_data->day_split);
	SETREG8(0x1d937, awb_ct_data->a_split);
	SETREG8(0x66211, awb_ct_data->awb_top_limit);
	SETREG8(0x66212, awb_ct_data->awb_bot_limit);

	return 0;
}


int ispv1_get_ccm_coff(ccm_coff *ccm_data)
{
	print_debug("enter %s", __func__);
	s16 temp;
	
	GETREG16(0x1c1d8, temp);		ccm_data->ccm_center[0][0] = temp;
	GETREG16(0x1c1da, temp);		ccm_data->ccm_center[0][1] = temp;
	GETREG16(0x1c1dc, temp);		ccm_data->ccm_center[0][2] = temp;
	GETREG16(0x1c1de, temp);		ccm_data->ccm_center[1][0] = temp;
	GETREG16(0x1c1e0, temp);		ccm_data->ccm_center[1][1] = temp;
	GETREG16(0x1c1e2, temp);		ccm_data->ccm_center[1][2] = temp;
	GETREG16(0x1c1e4, temp);		ccm_data->ccm_center[2][0] = temp;
	GETREG16(0x1c1e6, temp);		ccm_data->ccm_center[2][1] = temp;
	GETREG16(0x1c1e8, temp);		ccm_data->ccm_center[2][2] = temp;

	GETREG16(0x1c1fc, temp);		ccm_data->ccm_left[0][0] = temp;
	GETREG16(0x1c1fe, temp);		ccm_data->ccm_left[0][1] = temp;
	GETREG16(0x1c200, temp);		ccm_data->ccm_left[0][2] = temp;
	GETREG16(0x1c202, temp);		ccm_data->ccm_left[1][0] = temp;
	GETREG16(0x1c204, temp);		ccm_data->ccm_left[1][1] = temp;
	GETREG16(0x1c206, temp);		ccm_data->ccm_left[1][2] = temp;
	GETREG16(0x1c208, temp);		ccm_data->ccm_left[2][0] = temp;
	GETREG16(0x1c20a, temp);		ccm_data->ccm_left[2][1] = temp;
	GETREG16(0x1c20c, temp);		ccm_data->ccm_left[2][2] = temp;
	
	GETREG16(0x1c220, temp);		ccm_data->ccm_right[0][0] = temp;
	GETREG16(0x1c222, temp);		ccm_data->ccm_right[0][1] = temp;
	GETREG16(0x1c224, temp);		ccm_data->ccm_right[0][2] = temp;
	GETREG16(0x1c226, temp);		ccm_data->ccm_right[1][0] = temp;
	GETREG16(0x1c228, temp);		ccm_data->ccm_right[1][1] = temp;
	GETREG16(0x1c22a, temp);		ccm_data->ccm_right[1][2] = temp;
	GETREG16(0x1c22c, temp);		ccm_data->ccm_right[2][0] = temp;
	GETREG16(0x1c22e, temp);		ccm_data->ccm_right[2][1] = temp;
	GETREG16(0x1c230, temp);		ccm_data->ccm_right[2][2] = temp;
	
	return 0;
}


int ispv1_set_ccm_coff(ccm_coff *ccm_data)
{
	print_debug("enter %s", __func__);

	SETREG16(0x1c1d8, ccm_data->ccm_center[0][0]);
	SETREG16(0x1c1da, ccm_data->ccm_center[0][1]);
	SETREG16(0x1c1dc, ccm_data->ccm_center[0][2]);
	SETREG16(0x1c1de, ccm_data->ccm_center[1][0]);
	SETREG16(0x1c1e0, ccm_data->ccm_center[1][1]);
	SETREG16(0x1c1e2, ccm_data->ccm_center[1][2]);
	SETREG16(0x1c1e4, ccm_data->ccm_center[2][0]);
	SETREG16(0x1c1e6, ccm_data->ccm_center[2][1]);
	SETREG16(0x1c1e8, ccm_data->ccm_center[2][2]);

	SETREG16(0x1c1fc, ccm_data->ccm_left[0][0]);
	SETREG16(0x1c1fe, ccm_data->ccm_left[0][1]);
	SETREG16(0x1c200, ccm_data->ccm_left[0][2]);
	SETREG16(0x1c202, ccm_data->ccm_left[1][0]);
	SETREG16(0x1c204, ccm_data->ccm_left[1][1]);
	SETREG16(0x1c206, ccm_data->ccm_left[1][2]);
	SETREG16(0x1c208, ccm_data->ccm_left[2][0]);
	SETREG16(0x1c20a, ccm_data->ccm_left[2][1]);
	SETREG16(0x1c20c, ccm_data->ccm_left[2][2]);
	
	SETREG16(0x1c220, ccm_data->ccm_right[0][0]);
	SETREG16(0x1c222, ccm_data->ccm_right[0][1]);
	SETREG16(0x1c224, ccm_data->ccm_right[0][2]);
	SETREG16(0x1c226, ccm_data->ccm_right[1][0]);
	SETREG16(0x1c228, ccm_data->ccm_right[1][1]);
	SETREG16(0x1c22a, ccm_data->ccm_right[1][2]);
	SETREG16(0x1c22c, ccm_data->ccm_right[2][0]);
	SETREG16(0x1c22e, ccm_data->ccm_right[2][1]);
	SETREG16(0x1c230, ccm_data->ccm_right[2][2]);

	return 0;
}


int ispv1_set_added_coff(added_coff *added_data)
{
	print_debug("enter %s", __func__);

	SETREG8(0x65604, added_data->denoise[0]);
	SETREG8(0x65605, added_data->denoise[1]);
	SETREG8(0x65606, added_data->denoise[2]);
	SETREG8(0x65607, added_data->denoise[3]);
	SETREG8(0x65510, added_data->denoise[4]);
	SETREG8(0x6551a, added_data->denoise[5]);
	SETREG8(0x6551b, added_data->denoise[6]);
	SETREG8(0x6551c, added_data->denoise[7]);
	SETREG8(0x6551d, added_data->denoise[8]);
	SETREG8(0x6551e, added_data->denoise[9]);
	SETREG8(0x6551f, added_data->denoise[10]);
	SETREG8(0x65520, added_data->denoise[11]);
	SETREG8(0x65522, added_data->denoise[12]);
	SETREG8(0x65523, added_data->denoise[13]);
	SETREG8(0x65524, added_data->denoise[14]);
	SETREG8(0x65525, added_data->denoise[15]);
	SETREG8(0x65526, added_data->denoise[16]);
	SETREG8(0x65527, added_data->denoise[17]);
	SETREG8(0x65528, added_data->denoise[18]);
	SETREG8(0x65529, added_data->denoise[19]);
	SETREG8(0x6552a, added_data->denoise[20]);
	SETREG8(0x6552b, added_data->denoise[21]);
	SETREG8(0x6552c, added_data->denoise[22]);
	SETREG8(0x6552d, added_data->denoise[23]);
	SETREG8(0x6552e, added_data->denoise[24]);
	SETREG8(0x6552f, added_data->denoise[25]);
	SETREG8(0x65c00, added_data->denoise[26]);
	SETREG8(0x65c01, added_data->denoise[27]);
	SETREG8(0x65c02, added_data->denoise[28]);
	SETREG8(0x65c03, added_data->denoise[29]);
	SETREG8(0x65c04, added_data->denoise[30]);
	SETREG8(0x65c05, added_data->denoise[31]);

	SETREG8(0x1C4C0, added_data->tone[0]);
	SETREG8(0x1C4C1, added_data->tone[1]);
	SETREG8(0x1C4C2, added_data->tone[2]);
	SETREG8(0x1C4C3, added_data->tone[3]);
	SETREG8(0x1C4C4, added_data->tone[4]);
	SETREG8(0x1C4C5, added_data->tone[5]);
	SETREG8(0x1C4C6, added_data->tone[6]);
	SETREG8(0x1C4C7, added_data->tone[7]);
	SETREG8(0x1C4C8, added_data->tone[8]);
	SETREG8(0x1C4C9, added_data->tone[9]);
	SETREG8(0x1C4CA, added_data->tone[10]);
	SETREG8(0x1C4CB, added_data->tone[11]);
	SETREG8(0x1C4CC, added_data->tone[12]);
	SETREG8(0x1C4CD, added_data->tone[13]);
	SETREG8(0x1C4CE, added_data->tone[14]);
	SETREG8(0x1c4d4, added_data->tone[15]);
	SETREG8(0x1c4d5, added_data->tone[16]);
	SETREG8(0x1c4cf, added_data->tone[17]);

	SETREG8(0x1C998, added_data->uv[0]);
	SETREG8(0x1C999, added_data->uv[1]);
	SETREG8(0x1C99A, added_data->uv[2]);
	SETREG8(0x1C99B, added_data->uv[3]);
	SETREG8(0x1C99C, added_data->uv[4]);
	SETREG8(0x1C99D, added_data->uv[5]);
	SETREG8(0x1C99E, added_data->uv[6]);
	SETREG8(0x1C99F, added_data->uv[7]);
	SETREG8(0x1C9A0, added_data->uv[8]);
	SETREG8(0x1C9A1, added_data->uv[9]);
	SETREG8(0x1C9A2, added_data->uv[10]);
	SETREG8(0x1C9A3, added_data->uv[11]);
	SETREG8(0x1C9A4, added_data->uv[12]);
	SETREG8(0x1C9A5, added_data->uv[13]);
	SETREG8(0x1C9A6, added_data->uv[14]);
	SETREG8(0x1C9A7, added_data->uv[15]);
	SETREG8(0x1C9A8, added_data->uv[16]);
	SETREG8(0x1C9A9, added_data->uv[17]);
	SETREG8(0x1C9AA, added_data->uv[18]);
	SETREG8(0x1C9AB, added_data->uv[19]);
	SETREG8(0x1C9AC, added_data->uv[20]);
	SETREG8(0x1C9AD, added_data->uv[21]);
	SETREG8(0x1C9AE, added_data->uv[22]);
	SETREG8(0x1C9AF, added_data->uv[23]);
	SETREG8(0x1C9B0, added_data->uv[24]);
	SETREG8(0x1C9B1, added_data->uv[25]);
	SETREG8(0x1C9B2, added_data->uv[26]);
	SETREG8(0x1C9B3, added_data->uv[27]);
	SETREG8(0x1C9B4, added_data->uv[28]);
	SETREG8(0x1C9B5, added_data->uv[29]);
	SETREG8(0x1C9B6, added_data->uv[30]);
	SETREG8(0x1C9B7, added_data->uv[31]);

	SETREG8(0x1d904, added_data->uv_low[0]);
	SETREG8(0x1d905, added_data->uv_low[1]);
	SETREG8(0x1d906, added_data->uv_low[2]);
	SETREG8(0x1d907, added_data->uv_low[3]);
	SETREG8(0x1d908, added_data->uv_low[4]);
	SETREG8(0x1d909, added_data->uv_low[5]);
	SETREG8(0x1d90a, added_data->uv_low[6]);
	SETREG8(0x1d90b, added_data->uv_low[7]);
	SETREG8(0x1d90c, added_data->uv_low[8]);
	SETREG8(0x1d90d, added_data->uv_low[9]);
	SETREG8(0x1d90e, added_data->uv_low[10]);
	SETREG8(0x1d90f, added_data->uv_low[11]);
	SETREG8(0x1d910, added_data->uv_low[12]);
	SETREG8(0x1d911, added_data->uv_low[13]);
	SETREG8(0x1d912, added_data->uv_low[14]);
	SETREG8(0x1d913, added_data->uv_low[15]);

	SETREG8(0x1d914, added_data->uv_high[0]);
	SETREG8(0x1d915, added_data->uv_high[1]);
	SETREG8(0x1d916, added_data->uv_high[2]);
	SETREG8(0x1d917, added_data->uv_high[3]);
	SETREG8(0x1d918, added_data->uv_high[4]);
	SETREG8(0x1d919, added_data->uv_high[5]);
	SETREG8(0x1d91a, added_data->uv_high[6]);
	SETREG8(0x1d91b, added_data->uv_high[7]);
	SETREG8(0x1d91c, added_data->uv_high[8]);
	SETREG8(0x1d91d, added_data->uv_high[9]);
	SETREG8(0x1d91e, added_data->uv_high[10]);
	SETREG8(0x1d91f, added_data->uv_high[11]);
	SETREG8(0x1d920, added_data->uv_high[12]);
	SETREG8(0x1d921, added_data->uv_high[13]);
	SETREG8(0x1d922, added_data->uv_high[14]);
	SETREG8(0x1d923, added_data->uv_high[15]);

	SETREG8(0x1cccf, added_data->awb_shift[0]);
	SETREG8(0x1ccd0, added_data->awb_shift[1]);
	SETREG8(0x1c5b8, added_data->awb_shift[2]);
	SETREG8(0x1c5b9, added_data->awb_shift[3]);
	SETREG8(0x1ccd1, added_data->awb_shift[4]);
	SETREG8(0x1ccd2, added_data->awb_shift[5]);
	SETREG8(0x1ccd3, added_data->awb_shift[6]);
	SETREG8(0x1ccd4, added_data->awb_shift[7]);
	SETREG8(0x1cccc, added_data->awb_shift[8]);
	SETREG8(0x1cccd, added_data->awb_shift[9]);

	SETREG8(0x1c5ce, added_data->highlight[0]);
	SETREG8(0x1c5cf, added_data->highlight[1]);
	SETREG8(0x1c5d0, added_data->highlight[2]);
	SETREG8(0x1c5d1, added_data->highlight[3]);
	SETREG8(0x1c5d2, added_data->highlight[4]);
	SETREG8(0x1c5d3, added_data->highlight[5]);
	SETREG8(0x1c5d4, added_data->highlight[6]);
	SETREG8(0x1c5d5, added_data->highlight[7]);
	SETREG8(0x1c5d6, added_data->highlight[8]);
	SETREG8(0x1c5d7, added_data->highlight[9]);
	SETREG8(0x1c5d8, added_data->highlight[10]);
	SETREG8(0x1c1c2, added_data->highlight[11]);
	SETREG8(0x1c1c3, added_data->highlight[12]);

	SETREG8(0x1d925, added_data->dynamic_awb[0]);
	SETREG8(0x1d926, added_data->dynamic_awb[1]);
	SETREG8(0x1d927, added_data->dynamic_awb[2]);
	SETREG8(0x1d928, added_data->dynamic_awb[3]);
	SETREG8(0x1d929, added_data->dynamic_awb[4]);
	SETREG8(0x1d92a, added_data->dynamic_awb[5]);
	SETREG8(0x1d92b, added_data->dynamic_awb[6]);
	SETREG8(0x1d92c, added_data->dynamic_awb[7]);
	SETREG8(0x1d92d, added_data->dynamic_awb[8]);
	SETREG8(0x1d92e, added_data->dynamic_awb[9]);
	SETREG8(0x1d92f, added_data->dynamic_awb[10]);
	SETREG8(0x1d930, added_data->dynamic_awb[11]);
	SETREG8(0x1d931, added_data->dynamic_awb[12]);
	SETREG8(0x1d932, added_data->dynamic_awb[13]);
	SETREG8(0x1d933, added_data->dynamic_awb[14]);
	SETREG8(0x1d934, added_data->dynamic_awb[15]);
	SETREG8(0x1d935, added_data->dynamic_awb[16]);
	SETREG8(0x1d936, added_data->dynamic_awb[17]);
	SETREG8(0x1d937, added_data->dynamic_awb[18]);
	SETREG8(0x1d938, added_data->dynamic_awb[19]);
	SETREG8(0x1d939, added_data->dynamic_awb[20]);

	return 0;
}


int ispv1_get_coff_seq(seq_coffs *seq_data)
{
	print_debug("enter %s", __func__);

	int i;
	int length = seq_data->length;
	int *reg = seq_data->reg;
	int *value = seq_data->value;

	for(i = 0; i < length; i++)
	{
		value[i] = GETREG8(reg[i]);
		print_debug("i = %d, get reg[0x%x] = 0x%x", i, reg[i], value[i]);
	}

	return 0;
}

int ispv1_set_coff_seq(seq_coffs *seq_data)
{
	print_debug("enter %s", __func__);

	int i;
	int length = seq_data->length;
	int *reg = seq_data->reg;
	int *value = seq_data->value;

	for(i = 0; i < length; i++)
	{
		SETREG8(reg[i], value[i]);
		print_debug("i = %d, set reg[0x%x] = 0x%x", i, reg[i], value[i]);
	}

	return 0;
}


int max_expo_time = 0;	//used in ispv1_capture_cmd
int ispv1_set_max_expo_time(int time)	//expo_time = 1/time s
{
	print_debug("enter %s", __func__);

	if(max_expo_time < 0 || max_expo_time > 100)
	{
		print_error("%s, time = %d", __func__, time);
		return time;
	}
	max_expo_time = time;

	return 0;
}

/* zhoutian 00195335 12-7-16 added for auto scene detect end > */
/* Added for sharpness, y36721 todo */
int ispv1_set_sharpness(camera_sharpness sharpness)
{
	print_debug("enter %s", __func__);

	return 0;
}

/* Added for saturation, y36721 todo */
int ispv1_set_saturation(camera_saturation saturation)
{
	print_debug("enter %s, %d", __func__, saturation);
	this_ispdata->saturation = saturation;

	return 0;
}

int ispv1_set_saturation_done(camera_saturation saturation)
{
	print_debug("enter %s, %d", __func__, saturation);

	switch (saturation) {
	case CAMERA_SATURATION_L2:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, GETREG8(REG_ISP_SDE_CTRL) | ISP_SATURATION_ENABLE);
		SETREG8(REG_ISP_SDE_U_SATURATION, 0x10);
		SETREG8(REG_ISP_SDE_V_SATURATION, 0x10);
		break;

	case CAMERA_SATURATION_L1:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, GETREG8(REG_ISP_SDE_CTRL) | ISP_SATURATION_ENABLE);
		SETREG8(REG_ISP_SDE_U_SATURATION, 0x28);
		SETREG8(REG_ISP_SDE_V_SATURATION, 0x28);
		break;

	case CAMERA_SATURATION_H0:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, GETREG8(REG_ISP_SDE_CTRL) | ISP_SATURATION_ENABLE);
		SETREG8(REG_ISP_SDE_U_SATURATION, 0x40);
		SETREG8(REG_ISP_SDE_V_SATURATION, 0x40);
		break;

	case CAMERA_SATURATION_H1:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, GETREG8(REG_ISP_SDE_CTRL) | ISP_SATURATION_ENABLE);
		SETREG8(REG_ISP_SDE_U_SATURATION, 0x58);
		SETREG8(REG_ISP_SDE_V_SATURATION, 0x58);
		break;

	case CAMERA_SATURATION_H2:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, GETREG8(REG_ISP_SDE_CTRL) | ISP_SATURATION_ENABLE);
		SETREG8(REG_ISP_SDE_U_SATURATION, 0x70);
		SETREG8(REG_ISP_SDE_V_SATURATION, 0x70);
		break;

	default:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) & (~ISP_SDE_ENABLE));
		break;
	}

	return 0;
}

int ispv1_set_contrast(camera_contrast contrast)
{
	print_debug("enter %s, %d", __func__, contrast);
	this_ispdata->contrast = contrast;

	return 0;
}

int ispv1_set_contrast_done(camera_contrast contrast)
{
	print_debug("enter %s, %d", __func__, contrast);

	SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
	SETREG8(REG_ISP_SDE_CTRL, GETREG8(REG_ISP_SDE_CTRL) | ISP_CONTRAST_ENABLE);

	ispv1_switch_contrast(STATE_PREVIEW, contrast);

	return 0;
}

int ispv1_switch_contrast(camera_state state, camera_contrast contrast)
{
	if (state == STATE_PREVIEW) {
		switch (contrast) {
		case CAMERA_CONTRAST_L2:
			SETREG8(REG_ISP_SDE_CONTRAST, SDE_CONTRAST_PREVIEW_L2);
			SETREG8(REG_ISP_TOP5, GETREG8(REG_ISP_TOP5) & (~SDE_MANUAL_OFFSET_ENABLE));
			break;

		case CAMERA_CONTRAST_L1:
			SETREG8(REG_ISP_SDE_CONTRAST, SDE_CONTRAST_PREVIEW_L1);
			SETREG8(REG_ISP_TOP5, GETREG8(REG_ISP_TOP5) & (~SDE_MANUAL_OFFSET_ENABLE));
			break;

		case CAMERA_CONTRAST_H0:
			SETREG8(REG_ISP_SDE_CONTRAST, SDE_CONTRAST_PREVIEW_H0);
			SETREG8(REG_ISP_TOP5, (GETREG8(REG_ISP_TOP5) | SDE_MANUAL_OFFSET_ENABLE));
			break;

		case CAMERA_CONTRAST_H1:
			SETREG8(REG_ISP_SDE_CONTRAST, SDE_CONTRAST_PREVIEW_H1);
			SETREG8(REG_ISP_TOP5, GETREG8(REG_ISP_TOP5) & (~SDE_MANUAL_OFFSET_ENABLE));
			break;

		case CAMERA_CONTRAST_H2:
			SETREG8(REG_ISP_SDE_CONTRAST, SDE_CONTRAST_PREVIEW_H2);
			SETREG8(REG_ISP_TOP5, GETREG8(REG_ISP_TOP5) & (~SDE_MANUAL_OFFSET_ENABLE));
			break;

		default:
			print_error("%s, not supported contrast %d", __func__, contrast);
			break;
		}
	} else if (state == STATE_CAPTURE) {
		switch (contrast) {
		case CAMERA_CONTRAST_L2:
			SETREG8(REG_ISP_SDE_CONTRAST, SDE_CONTRAST_CAPTURE_L2);
			SETREG8(REG_ISP_TOP5, GETREG8(REG_ISP_TOP5) & (~SDE_MANUAL_OFFSET_ENABLE));
			break;

		case CAMERA_CONTRAST_L1:
			SETREG8(REG_ISP_SDE_CONTRAST, SDE_CONTRAST_CAPTURE_L1);
			SETREG8(REG_ISP_TOP5, GETREG8(REG_ISP_TOP5) & (~SDE_MANUAL_OFFSET_ENABLE));
			break;

		case CAMERA_CONTRAST_H0:
			SETREG8(REG_ISP_SDE_CONTRAST, SDE_CONTRAST_CAPTURE_H0);
			SETREG8(REG_ISP_TOP5, GETREG8(REG_ISP_TOP5) & (~SDE_MANUAL_OFFSET_ENABLE));
			break;

		case CAMERA_CONTRAST_H1:
			SETREG8(REG_ISP_SDE_CONTRAST, SDE_CONTRAST_CAPTURE_H1);
			SETREG8(REG_ISP_TOP5, GETREG8(REG_ISP_TOP5) & (~SDE_MANUAL_OFFSET_ENABLE));
			break;

		case CAMERA_CONTRAST_H2:
			SETREG8(REG_ISP_SDE_CONTRAST, SDE_CONTRAST_CAPTURE_H2);
			SETREG8(REG_ISP_TOP5, GETREG8(REG_ISP_TOP5) & (~SDE_MANUAL_OFFSET_ENABLE));
			break;

		default:
			print_error("%s, not supported contrast %d", __func__, contrast);
			break;
		}
	}

	return 0;
}

void ispv1_set_fps_lock(int lock)
{
	print_debug("enter %s", __func__);
	fps_lock = lock;
}

void ispv1_change_fps(camera_frame_rate_mode mode)
{
	print_debug("enter :%s, mode :%d", __func__, mode);
	if (fps_lock == true) {
		print_error("fps is locked");
		return;
	}
	if (mode >= CAMERA_FRAME_RATE_MAX)
		print_error("inviable camera_frame_rate_mode: %d", mode);

	if (CAMERA_FRAME_RATE_FIX_MAX == mode) {
		this_ispdata->sensor->fps_min = isp_hw_data.fps_max;
		this_ispdata->sensor->fps_max = isp_hw_data.fps_max;
		ispv1_set_frame_rate(CAMERA_FRAME_RATE_FIX_MAX, this_ispdata->sensor);
	} else if (CAMERA_FRAME_RATE_FIX_MIN == mode) {
		this_ispdata->sensor->fps_min = isp_hw_data.fps_min;
		this_ispdata->sensor->fps_max = isp_hw_data.fps_min;
		ispv1_set_frame_rate(CAMERA_FRAME_RATE_FIX_MIN, this_ispdata->sensor);
	} else if (CAMERA_FRAME_RATE_AUTO == mode) {
		this_ispdata->sensor->fps_min = isp_hw_data.fps_min;
		this_ispdata->sensor->fps_max = isp_hw_data.fps_max;
	}

	this_ispdata->fps_mode = mode;
}

void ispv1_change_max_exposure(camera_sensor *sensor, camera_max_exposrure mode)
{
	u8 frame_index, fps;
	u32 vts;
	u16 max_exposure;

	frame_index = sensor->preview_frmsize_index;
	fps = sensor->frmsize_list[frame_index].fps;
	vts = sensor->frmsize_list[frame_index].vts;

	if (CAMERA_MAX_EXPOSURE_LIMIT == mode)
		max_exposure = (fps * vts / 100 - 14);
	else
	{
		max_exposure = ((fps * vts / sensor->fps) - 14);
		if(sensor->get_support_vts)
		{
			max_exposure = sensor->get_support_vts(sensor->fps,fps,vts);
		}
	}



	SETREG16(REG_ISP_MAX_EXPOSURE, max_exposure);
	SETREG16(REG_ISP_MAX_EXPOSURE_SHORT, max_exposure);
}

/* < zhoutian 00195335 12-7-16 added for auto scene detect begin */
int ispv1_set_max_exposure(camera_max_exposrure mode)
{
	print_debug("enter %s, mode = %d ", __func__, mode);

	ispv1_change_max_exposure(this_ispdata->sensor, mode);

	return 0;
}
/* zhoutian 00195335 12-7-16 added for auto scene detect end > */

/* < zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight begin */
typedef enum {
	CAMERA_ISO_NORMAL = 0,
	CAMERA_ISO_DOUBLE,
} camera_iso_double;

void set_iso_double_or_normal(camera_iso_double value)
{
	print_debug("enter %s", __func__);

	camera_sensor *sensor;
	sensor = this_ispdata->sensor;
	int max_iso, min_iso;
	int max_gain, min_gain;

	max_iso = sensor->get_override_param(OVERRIDE_ISO_HIGH);
	min_iso = sensor->get_override_param(OVERRIDE_ISO_LOW);

	if (value == CAMERA_ISO_DOUBLE)
		max_iso = max_iso * 2;

	max_gain = max_iso * 0x10 / 100;
	min_gain = min_iso * 0x10 / 100;

	if(BINNING_SUMMARY== sensor->support_binning_type)
	{
		if(sensor->frmsize_list[sensor->preview_frmsize_index].binning == true)
		{
			sensor->max_gain = max_gain;
		}
	}
	else
	{
		sensor->max_gain = max_gain;
	}
	sensor->min_gain = min_gain;
	SETREG8(REG_ISP_MAX_GAIN, (max_gain & 0xff00) >> 8);
	SETREG8(REG_ISP_MAX_GAIN + 1, max_gain & 0xff);
	SETREG8(REG_ISP_MIN_GAIN, (min_gain & 0xff00) >> 8);
	SETREG8(REG_ISP_MIN_GAIN + 1, min_gain & 0xff);
/*
	if (value == CAMERA_ISO_DOUBLE) {
		SETREG8(0x1d9ae, 2); //max digital gain value
		SETREG8(0x1d9b0, 1); //0:800, 1:1600, 2:disable
		SETREG8(0x1d9af, 1); //enable preview digital gain
		SETREG8(0x1d9b1, 1); //enable capture digital gain
	} else {
		SETREG8(0x1d9ae, 4); //max digital gain value
		SETREG8(0x1d9b0, 2); //0:800, 1:1600, 2:disable
		SETREG8(0x1d9af, 0); //disable preview digital gain
		SETREG8(0x1d9b1, 0); //disable capture digital gain
	}
*/
}
/* zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight end > */

int ispv1_set_scene(camera_scene scene)
{
	u8 target_y_low = scene_target_y_low;
	u8 target_y_high = scene_target_y_high;

#ifdef OVISP_DEBUG_MODE
	return 0;
#endif

	print_debug("enter %s, scene:%d", __func__, scene);

	switch (scene) {
	case CAMERA_SCENE_AUTO:
	/* < lijiahuan 00175705 2012-10-22 add for HwAuto begin */
	case CAMERA_SCENE_HWAUTO:
	/* lijiahuan 00175705 2012-10-22 add for HwAuto end > */
		print_info("case CAMERA_SCENE_AUTO ");
		ispv1_change_fps(CAMERA_FRAME_RATE_AUTO);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = isp_hw_data.flash_mode;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_ENABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x80);
		ispv1_set_focus_mode(CAMERA_FOCUS_CONTINUOUS_PICTURE);
		ispv1_set_awb(this_ispdata->awb_mode);
		break;
	/* < lijiahuan 00175705 2013-06-01 add for hdr scene mode begin */
	case CAMERA_SCENE_HDR:
		{
			print_info("case CAMERA_SCENE_HDR ");
			ispv1_change_fps(CAMERA_FRAME_RATE_AUTO);
			ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
			SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
			SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
			this_ispdata->flash_mode = isp_hw_data.flash_mode;
			SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_ENABLE);
			SETREG8(REG_ISP_UV_SATURATION, 0x80);
			ispv1_set_focus_mode(CAMERA_FOCUS_CONTINUOUS_PICTURE);
			ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		}
		break;
	/* lijiahuan 00175705 2013-06-01 add for hdr scene mode end > */

	case CAMERA_SCENE_ACTION:
		print_info("case CAMERA_SCENE_ACTION ");
		/*
		 * Reduce max exposure time 1/100s
		 * Pre-focus recommended. (optional)
		 */
		ispv1_change_fps(CAMERA_FRAME_RATE_FIX_MAX);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_LIMIT);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = CAMERA_FLASH_OFF;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_ENABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x80);
		ispv1_set_focus_mode(CAMERA_FOCUS_CONTINUOUS_PICTURE);
		ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		break;

	case CAMERA_SCENE_PORTRAIT:
		print_info("case CAMERA_SCENE_PORTRAIT ");
		/*
		 * Pre-tuned color matrix for better skin tone recommended. (optional)
		 */
		ispv1_change_fps(CAMERA_FRAME_RATE_AUTO);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = isp_hw_data.flash_mode;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_ENABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x90);
		ispv1_set_focus_mode(CAMERA_FOCUS_CONTINUOUS_PICTURE);
		ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		break;

	case CAMERA_SCENE_LANDSPACE:
		print_info("case CAMERA_SCENE_LANDSPACE ");
		/*
		 * Focus mode set to infinity
		 * Manual AWB to daylight (optional)
		 */
		ispv1_change_fps(CAMERA_FRAME_RATE_AUTO);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = CAMERA_FLASH_OFF;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_ENABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x98);
		ispv1_set_focus_mode(CAMERA_FOCUS_INFINITY);
		ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		break;

	case CAMERA_SCENE_NIGHT:
		print_info("case CAMERA_SCENE_NIGHT ");
		/*
		 * Increase max exposure time£¬Ö¡ÂÊ½µµÍµ½5fps×óÓÒ
		 * Turn off the flash
		 * Turn off the UV adjust
		 * Focus mode set to infinity
		 */
		ispv1_change_fps(CAMERA_FRAME_RATE_FIX_MIN);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = CAMERA_FLASH_OFF;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_DISABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x80);
		ispv1_set_focus_mode(CAMERA_FOCUS_CONTINUOUS_PICTURE);
		ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		break;

	case CAMERA_SCENE_NIGHT_PORTRAIT:
		print_info("case CAMERA_SCENE_NIGHT_PORTRAIT ");
		/*
		 * Increase max exposure time
		 * Turn off UV adjust
		 * Turn on the flash (optional)
		 */
		ispv1_change_fps(CAMERA_FRAME_RATE_FIX_MIN);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = CAMERA_FLASH_OFF;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_DISABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x90);
		ispv1_set_focus_mode(CAMERA_FOCUS_CONTINUOUS_PICTURE);
		ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		break;

	case CAMERA_SCENE_THEATRE:
		print_info("case CAMERA_SCENE_THEATRE ");
		/*
		 * Increase max exposure time£¬Ö¡ÂÊ½µµÍµ½5fps×óÓÒ
		 * Turn off the flash
		 * Turn off UV adjust
		 */
		ispv1_change_fps(CAMERA_FRAME_RATE_FIX_MIN);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = CAMERA_FLASH_OFF;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_DISABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x70);
		ispv1_set_focus_mode(CAMERA_FOCUS_INFINITY);
		ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		break;

	case CAMERA_SCENE_BEACH:
		print_info("case CAMERA_SCENE_BEACH ");
		/*
		 * Reduce AE target
		 * Manual AWB set to daylight (optional)
		 */
		ispv1_change_fps(CAMERA_FRAME_RATE_AUTO);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		ispv1_calc_ev(&target_y_low, &target_y_high, -2);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = CAMERA_FLASH_OFF;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_ENABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x80);
		ispv1_set_focus_mode(CAMERA_FOCUS_CONTINUOUS_PICTURE);
		ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		break;

	case CAMERA_SCENE_SNOW:
		print_info("case CAMERA_SCENE_SNOW ");
		/*
		 * Increase AE target
		 * Enable EDR mode
		 * Manual AWB set to daylight (optional)
		 */
		ispv1_change_fps(CAMERA_FRAME_RATE_AUTO);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		ispv1_calc_ev(&target_y_low, &target_y_high, 2);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = CAMERA_FLASH_OFF;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_ENABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x80);
		ispv1_set_focus_mode(CAMERA_FOCUS_CONTINUOUS_PICTURE);
		ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		break;

	case CAMERA_SCENE_FIREWORKS:
		print_info("case CAMERA_SCENE_FIREWORKS ");
		/*
		 * Increase max exposure time£¬Ö¡ÂÊ½µµÍµ½5fps×óÓÒ
		 * Focus mode set to infinity
		 * Turn off the flash
		 */
		ispv1_change_fps(CAMERA_FRAME_RATE_FIX_MIN);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = CAMERA_FLASH_OFF;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_ENABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x80);
		ispv1_set_focus_mode(CAMERA_FOCUS_INFINITY);
		ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		break;

	case CAMERA_SCENE_CANDLELIGHT:
		print_info("case CAMERA_SCENE_CANDLELIGHT ");
		/*
		 * Increase max exposure time£¬Ö¡ÂÊ½µµÍµ½5fps×óÓÒ
		 * Turn off the flash
		 * Turn off UV adjust
		 * AWB set to manual with fixed AWB gain with yellow/warm tone
		 */
		ispv1_change_fps(CAMERA_FRAME_RATE_FIX_MIN);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = CAMERA_FLASH_OFF;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_DISABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x80);
		ispv1_set_focus_mode(CAMERA_FOCUS_CONTINUOUS_PICTURE);
		ispv1_set_awb(CAMERA_WHITEBALANCE_CANDLELIGHT);
		break;

	case CAMERA_SCENE_FLOWERS:
		print_info("case CAMERA_SCENE_FLOWER ");

		ispv1_change_fps(CAMERA_FRAME_RATE_AUTO);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_ENABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x80);
		this_ispdata->flash_mode = CAMERA_FLASH_OFF;
		ispv1_set_focus_mode(CAMERA_FOCUS_MACRO);
		ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		break;
	/* < zhoutian 00195335 12-7-16 added for auto scene detect begin */
	case CAMERA_SCENE_DETECTED_NIGHT:
		print_info("CAMERA_SCENE_DETECTED_NIGHT");
		break;
		
	case CAMERA_SCENE_DETECTED_ACTION:
		print_info("CAMERA_SCENE_DETECTED_ACTION");
		break;

	case CAMERA_SCENE_DETECTED_AUTO:
		print_info("CAMERA_SCENE_DETECTED_AUTO");
		break;	
	/* zhoutian 00195335 12-7-16 added for auto scene detect end > */

	case CAMERA_SCENE_SUNSET:
	case CAMERA_SCENE_STEADYPHOTO:
	case CAMERA_SCENE_SPORTS:
	case CAMERA_SCENE_BARCODE:
	default:
		print_info("This scene not supported yet. ");
		ispv1_change_fps(CAMERA_FRAME_RATE_AUTO);
		ispv1_change_max_exposure(this_ispdata->sensor, CAMERA_MAX_EXPOSURE_RESUME);
		SETREG8(REG_ISP_TARGET_Y_LOW, target_y_low);
		SETREG8(REG_ISP_TARGET_Y_HIGH, target_y_high);
		this_ispdata->flash_mode = isp_hw_data.flash_mode;
		this_ispdata->scene = CAMERA_SCENE_AUTO;
		SETREG8(REG_ISP_UV_ADJUST, UV_ADJUST_ENABLE);
		SETREG8(REG_ISP_UV_SATURATION, 0x80);
		ispv1_set_focus_mode(CAMERA_FOCUS_AUTO);
		ispv1_set_awb(CAMERA_WHITEBALANCE_AUTO);
		goto out;
	}

	this_ispdata->scene = scene;

out:
	return 0;
}

int ispv1_set_brightness(camera_brightness brightness)
{
	print_debug("enter %s, %d", __func__, brightness);
	this_ispdata->brightness = brightness;

	return 0;
}

int ispv1_set_brightness_done(camera_brightness brightness)
{
	print_debug("enter %s, %d", __func__, brightness);

	switch (brightness) {
	case CAMERA_BRIGHTNESS_L2:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, GETREG8(REG_ISP_SDE_CTRL) | ISP_BRIGHTNESS_ENABLE);
		SETREG8(REG_ISP_SDE_SIGN_SET, GETREG8(REG_ISP_SDE_SIGN_SET) | ISP_BRIGHTNESS_SIGN_NEGATIVE);
		break;

	case CAMERA_BRIGHTNESS_L1:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, GETREG8(REG_ISP_SDE_CTRL) | ISP_BRIGHTNESS_ENABLE);
		SETREG8(REG_ISP_SDE_SIGN_SET, GETREG8(REG_ISP_SDE_SIGN_SET) | ISP_BRIGHTNESS_SIGN_NEGATIVE);
		break;

	case CAMERA_BRIGHTNESS_H0:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, GETREG8(REG_ISP_SDE_CTRL) | ISP_BRIGHTNESS_ENABLE);
		SETREG8(REG_ISP_SDE_SIGN_SET, GETREG8(REG_ISP_SDE_SIGN_SET) & (~ISP_BRIGHTNESS_SIGN_NEGATIVE));
		break;

	case CAMERA_BRIGHTNESS_H1:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, GETREG8(REG_ISP_SDE_CTRL) | ISP_BRIGHTNESS_ENABLE);
		SETREG8(REG_ISP_SDE_SIGN_SET, GETREG8(REG_ISP_SDE_SIGN_SET) & (~ISP_BRIGHTNESS_SIGN_NEGATIVE));
		break;

	case CAMERA_BRIGHTNESS_H2:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, GETREG8(REG_ISP_SDE_CTRL) | ISP_BRIGHTNESS_ENABLE);
		SETREG8(REG_ISP_SDE_SIGN_SET, GETREG8(REG_ISP_SDE_SIGN_SET) & (~ISP_BRIGHTNESS_SIGN_NEGATIVE));
		break;

	default:
		print_error("%s, not supported brightness %d", __func__, brightness);
		break;
	}

	ispv1_switch_brightness(STATE_PREVIEW, brightness);

	return 0;
}

int ispv1_switch_brightness(camera_state state, camera_brightness brightness)
{
	if (state == STATE_PREVIEW) {
		switch (brightness) {
		case CAMERA_BRIGHTNESS_L2:
			SETREG8(REG_ISP_SDE_BRIGHTNESS, SDE_BRIGHTNESS_PREVIEW_L2);
			break;

		case CAMERA_BRIGHTNESS_L1:
			SETREG8(REG_ISP_SDE_BRIGHTNESS, SDE_BRIGHTNESS_PREVIEW_L1);
			break;

		case CAMERA_BRIGHTNESS_H0:
			SETREG8(REG_ISP_SDE_BRIGHTNESS, SDE_BRIGHTNESS_PREVIEW_H0);
			break;

		case CAMERA_BRIGHTNESS_H1:
			SETREG8(REG_ISP_SDE_BRIGHTNESS, SDE_BRIGHTNESS_PREVIEW_H1);
			break;

		case CAMERA_BRIGHTNESS_H2:
			SETREG8(REG_ISP_SDE_BRIGHTNESS, SDE_BRIGHTNESS_PREVIEW_H2);
			break;

		default:
			break;
		}
	} else if (state == STATE_CAPTURE) {
		switch (brightness) {
		case CAMERA_BRIGHTNESS_L2:
			SETREG8(REG_ISP_SDE_BRIGHTNESS, SDE_BRIGHTNESS_CAPTURE_L2);
			break;

		case CAMERA_BRIGHTNESS_L1:
			SETREG8(REG_ISP_SDE_BRIGHTNESS, SDE_BRIGHTNESS_CAPTURE_L1);
			break;

		case CAMERA_BRIGHTNESS_H0:
			SETREG8(REG_ISP_SDE_BRIGHTNESS, SDE_BRIGHTNESS_CAPTURE_H0);
			break;

		case CAMERA_BRIGHTNESS_H1:
			SETREG8(REG_ISP_SDE_BRIGHTNESS, SDE_BRIGHTNESS_CAPTURE_H1);
			break;

		case CAMERA_BRIGHTNESS_H2:
			SETREG8(REG_ISP_SDE_BRIGHTNESS, SDE_BRIGHTNESS_CAPTURE_H2);
			break;

		default:
			break;
		}
	}

	return 0;
}

int ispv1_set_effect(camera_effects effect)
{
	print_debug("enter %s, %d", __func__, effect);
	this_ispdata->effect = effect;

	return 0;
}

int ispv1_set_effect_done(camera_effects effect)
{
	print_debug("enter %s, %d", __func__, effect);

	switch (effect) {
	case CAMERA_EFFECT_NONE:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, ISP_CONTRAST_ENABLE | ISP_BRIGHTNESS_ENABLE | ISP_SATURATION_ENABLE);
		break;

	case CAMERA_EFFECT_MONO:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, ISP_MONO_EFFECT_ENABLE);
		break;

	case CAMERA_EFFECT_NEGATIVE:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, ISP_NEGATIVE_EFFECT_ENABLE);
		break;

	case CAMERA_EFFECT_SEPIA:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) | ISP_SDE_ENABLE);
		SETREG8(REG_ISP_SDE_CTRL, ISP_FIX_U_ENABLE | ISP_FIX_V_ENABLE);
		SETREG8(REG_ISP_SDE_U_REG, 0x30);
		SETREG8(REG_ISP_SDE_V_REG, 0xb0);
		break;

	default:
		SETREG8(REG_ISP_TOP2, GETREG8(REG_ISP_TOP2) & (~ISP_SDE_ENABLE));
		break;
	}

	return 0;
}

int ispv1_set_effect_saturation_done(camera_effects effect, camera_saturation saturation)
{
	if(effect == CAMERA_EFFECT_SEPIA) {
		ispv1_set_effect_done(effect);
	} else {
		ispv1_set_effect_done(effect);
		ispv1_set_saturation_done(saturation);
	}
	return 0;
}


/* < zhoutian 00195335 2013-02-07 modify for hwscope begin */
static u8 denoise[33];
static u8 sharpness[15];
static u8 UV[2];
bool denoise_already_closed_flg = false;
bool hwscope_with_ISP_crop_flg = false;
hwscope_coff crop_data;

//1--backup coff& disable denoise, 2--revocer coff
//need backup coffs before disable denoise, and avoid save action when denoise disabled
int ispv1_set_hwscope(hwscope_coff *hwscope_data)
{
	print_info("enter %s, mode = %d", __func__, hwscope_data->mode);

	if(hwscope_data->mode > HW_SCOPE_OFF)
	{
		print_error("%s:unknow mode id:%d", __func__, hwscope_data->mode);
		return 2;
	}

	// ISP crop size set
	if(hwscope_data->mode == HW_SCOPE_ON_WITH_CROP)
	{
		print_debug("ISP crop zoom = %d, border = %d", hwscope_data->zoom, hwscope_data->border);

		hwscope_with_ISP_crop_flg = true;
		crop_data.zoom = hwscope_data->zoom;
		crop_data.border = hwscope_data->border;
	}
	// ISP crop size get
	else if(hwscope_data->mode == HW_SCOPE_GET_CROP_INFO)
	{
		hwscope_data->out_width = crop_data.out_width;
		hwscope_data->out_height = crop_data.out_height;
		hwscope_data->crop_x = crop_data.crop_x;
		hwscope_data->crop_y = crop_data.crop_y;
		hwscope_data->crop_width = crop_data.crop_width;
		hwscope_data->crop_height = crop_data.crop_height;
	}
	else
	{
		hwscope_with_ISP_crop_flg = false;
	}

	// denoise & sharpness ISP coff set
	if(hwscope_data->mode == HW_SCOPE_ON || hwscope_data->mode == HW_SCOPE_ON_WITH_CROP)    //disable denoise
	{
		print_debug("close ISP's denoise and sharpness");

		if(denoise_already_closed_flg == true)
		{
			print_error("%s:last time camera exit abend, so do not backup coff this time", __func__);
		}
		else
		{
			print_debug("backup ISP's denoise and sharpness coff");
			//sonyimx135.c
			denoise[0] = GETREG8(0x65604);
			denoise[1] = GETREG8(0x65605);
			denoise[2] = GETREG8(0x65606);
			denoise[3] = GETREG8(0x65607);

			denoise[4] = GETREG8(0x65510);
			denoise[5] = GETREG8(0x65511);
			denoise[6] = GETREG8(0x6551a);
			denoise[7] = GETREG8(0x6551b);
			denoise[8] = GETREG8(0x6551c);
			denoise[9] = GETREG8(0x6551d);
			denoise[10] = GETREG8(0x6551e);
			denoise[11] = GETREG8(0x6551f);
			denoise[12] = GETREG8(0x65520);
			denoise[13] = GETREG8(0x65522);
			denoise[14] = GETREG8(0x65523);
			denoise[15] = GETREG8(0x65524);
			denoise[16] = GETREG8(0x65525);
			denoise[17] = GETREG8(0x65526);
			denoise[18] = GETREG8(0x65527);
			denoise[19] = GETREG8(0x65528);
			denoise[20] = GETREG8(0x65529);
			denoise[21] = GETREG8(0x6552a);
			denoise[22] = GETREG8(0x6552b);
			denoise[23] = GETREG8(0x6552c);
			denoise[24] = GETREG8(0x6552d);
			denoise[25] = GETREG8(0x6552e);
			denoise[26] = GETREG8(0x6552f);

			denoise[27] = GETREG8(0x65c00);
			denoise[28] = GETREG8(0x65c01);
			denoise[29] = GETREG8(0x65c02);
			denoise[30] = GETREG8(0x65c03);
			denoise[31] = GETREG8(0x65c04);
			denoise[32] = GETREG8(0x65c05);

			sharpness[0] = GETREG8(0x65600);
			sharpness[1] = GETREG8(0x65601);
			sharpness[2] = GETREG8(0x65602);
			sharpness[3] = GETREG8(0x65603);
			sharpness[4] = GETREG8(0x65608);
			sharpness[5] = GETREG8(0x65609);
			sharpness[6] = GETREG8(0x6560c);
			sharpness[7] = GETREG8(0x6560d);
			sharpness[8] = GETREG8(0x6560e);
			sharpness[9] = GETREG8(0x6560f);
			sharpness[10] = GETREG8(0x65610);
			sharpness[11] = GETREG8(0x65611);
			sharpness[12] = GETREG8(0x65613);
			sharpness[13] = GETREG8(0x65615);
			sharpness[14] = GETREG8(0x65617);

			UV[0] = GETREG8(0x1c4eb);
			UV[1] = GETREG8(0x1c4ec);
		}

		//set coff to 0
		SETREG8(0x65604, 0x00);
		SETREG8(0x65605, 0x00);
		SETREG8(0x65606, 0x00);
		SETREG8(0x65607, 0x00);

		SETREG8(0x65510, 0x00);
		SETREG8(0x65511, 0x00);
		SETREG8(0x6551a, 0x00);
		SETREG8(0x6551b, 0x00);
		SETREG8(0x6551c, 0x00);
		SETREG8(0x6551d, 0x00);
		SETREG8(0x6551e, 0x00);
		SETREG8(0x6551f, 0x00);
		SETREG8(0x65520, 0x00);
		SETREG8(0x65522, 0x00);
		SETREG8(0x65523, 0x00);
		SETREG8(0x65524, 0x00);
		SETREG8(0x65525, 0x00);
		SETREG8(0x65526, 0x00);
		SETREG8(0x65527, 0x00);
		SETREG8(0x65528, 0x00);
		SETREG8(0x65529, 0x00);
		SETREG8(0x6552a, 0x00);
		SETREG8(0x6552b, 0x00);
		SETREG8(0x6552c, 0x00);
		SETREG8(0x6552d, 0x00);
		SETREG8(0x6552e, 0x00);
		SETREG8(0x6552f, 0x00);

		SETREG8(0x65c00, 0x00);
		SETREG8(0x65c01, 0x00);
		SETREG8(0x65c02, 0x00);
		SETREG8(0x65c03, 0x00);
		SETREG8(0x65c04, 0x00);
		SETREG8(0x65c05, 0x00);

		SETREG8(0x65600, 0x00);
		SETREG8(0x65601, 0x00);
		SETREG8(0x65602, 0x00);
		SETREG8(0x65603, 0x00);
		SETREG8(0x65608, 0x00);
		SETREG8(0x65609, 0x00);
		SETREG8(0x6560c, 0x00);
		SETREG8(0x6560d, 0x00);
		SETREG8(0x6560e, 0x00);
		SETREG8(0x6560f, 0x00);
		SETREG8(0x65610, 0x00);
		SETREG8(0x65611, 0x00);
		SETREG8(0x65613, 0x00);
		SETREG8(0x65615, 0x00);
		SETREG8(0x65617, 0x00);

		SETREG8(0x1c4eb, 0x78);
		SETREG8(0x1c4ec, 0x78);

		//k3_ispv1.c
		SETREG8(0x65001, GETREG8(0x65001)&(~0x01));    //bit[0] set to 0
		SETREG8(0x65002, GETREG8(0x65002)&(~0x10));    //bit[4] set to 0

		denoise_already_closed_flg = true;

	}
	else if(hwscope_data->mode == HW_SCOPE_OFF)    //recover denoise
	{
		if(denoise_already_closed_flg == false)
		{
			print_error("%s:ISP's denoise and sharpness is on, no need to recover coff ", __func__);
			return 1;
		}

		print_debug("recover ISP's denoise and sharpness");

		//sonyimx135.c
		SETREG8(0x65604, denoise[0]);
		SETREG8(0x65605, denoise[1]);
		SETREG8(0x65606, denoise[2]);
		SETREG8(0x65607, denoise[3]);

		SETREG8(0x65510, denoise[4]);
		SETREG8(0x65511, denoise[5]);
		SETREG8(0x6551a, denoise[6]);
		SETREG8(0x6551b, denoise[7]);
		SETREG8(0x6551c, denoise[8]);
		SETREG8(0x6551d, denoise[9]);
		SETREG8(0x6551e, denoise[10]);
		SETREG8(0x6551f, denoise[11]);
		SETREG8(0x65520, denoise[12]);
		SETREG8(0x65522, denoise[13]);
		SETREG8(0x65523, denoise[14]);
		SETREG8(0x65524, denoise[15]);
		SETREG8(0x65525, denoise[16]);
		SETREG8(0x65526, denoise[17]);
		SETREG8(0x65527, denoise[18]);
		SETREG8(0x65528, denoise[19]);
		SETREG8(0x65529, denoise[20]);
		SETREG8(0x6552a, denoise[21]);
		SETREG8(0x6552b, denoise[22]);
		SETREG8(0x6552c, denoise[23]);
		SETREG8(0x6552d, denoise[24]);
		SETREG8(0x6552e, denoise[25]);
		SETREG8(0x6552f, denoise[26]);

		SETREG8(0x65c00, denoise[27]);
		SETREG8(0x65c01, denoise[28]);
		SETREG8(0x65c02, denoise[29]);
		SETREG8(0x65c03, denoise[30]);
		SETREG8(0x65c04, denoise[31]);
		SETREG8(0x65c05, denoise[32]);

		SETREG8(0x65600, sharpness[0]);
		SETREG8(0x65601, sharpness[1]);
		SETREG8(0x65602, sharpness[2]);
		SETREG8(0x65603, sharpness[3]);
		SETREG8(0x65608, sharpness[4]);
		SETREG8(0x65609, sharpness[5]);
		SETREG8(0x6560c, sharpness[6]);
		SETREG8(0x6560d, sharpness[7]);
		SETREG8(0x6560e, sharpness[8]);
		SETREG8(0x6560f, sharpness[9]);
		SETREG8(0x65610, sharpness[10]);
		SETREG8(0x65611, sharpness[11]);
		SETREG8(0x65613, sharpness[12]);
		SETREG8(0x65615, sharpness[13]);
		SETREG8(0x65617, sharpness[14]);

		SETREG8(0x1c4eb, UV[0]);
		SETREG8(0x1c4ec, UV[1]);

		//k3_ispv1.c
		SETREG8(0x65001, GETREG8(0x65001)|0x01);    //bit[0] set to 1
		SETREG8(0x65002, GETREG8(0x65002)|0x10);    //bit[4] set to 1
	
		denoise_already_closed_flg = false;

	}

	return 0;
}
/* zhoutian 00195335 2013-02-07 modify for hwscope end > */

/* < zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight begin */
int ispv1_set_hw_lowlight(int ctl)
{
	camera_sensor *sensor;
	sensor = this_ispdata->sensor;
	print_debug("enter %s, %d", __func__, ctl);

	if(sensor->support_hw_lowlight)
	{
		if(sensor->support_hw_lowlight() == 0)
		{
			print_error("lowlight only support by sensor sonyimx135, but sensor used now is :%s", sensor->info.name);
			return -1;
		}
	}

	if(ctl == 1)
	{
		if(hw_lowlight_on_flg == true)
		{
			print_error("%s:ISP's seq already switch to lowlight, no need to set again", __func__);
			return 1;
		}
		print_info("enter lowlight mode");
		hw_lowlight_on_flg = true;
		//set_iso_double_or_normal(CAMERA_ISO_DOUBLE);	//double the ISO value
		if(sensor->switch_to_lowlight_isp_seq)
			sensor->switch_to_lowlight_isp_seq(true);
	}
	else if(ctl == 0)
	{
		if(hw_lowlight_on_flg == false)
		{
			print_error("%s:ISP's seq already switch to normal mode, no need to set again", __func__);
			return 1;
		}

		print_info("exit lowlight mode");
		hw_lowlight_on_flg = false;
		//set_iso_double_or_normal(CAMERA_ISO_NORMAL);	//recover ISO to normal
		if(sensor->switch_to_lowlight_isp_seq)
			sensor->switch_to_lowlight_isp_seq(false);
	}
	else
	{
		print_error("%s:unknow ctl id:%d", __func__, ctl);
		return -1;
	}

	return 0;
}


int ispv1_get_binning_size(binning_size *size)
{
	camera_sensor *sensor;
	int index, width, height;
	bool binning = false;
	sensor = this_ispdata->sensor;
	print_debug("enter %s", __func__);

	if(sensor->support_hw_lowlight)
	{
		if(sensor->support_hw_lowlight() == 0)
		{
			print_error("lowlight only support by sensor sonyimx135, but sensor used now is :%s", sensor->info.name);
			return -1;
		}
	}

	index = sensor->preview_frmsize_index;
	width = sensor->frmsize_list[index].width;
	height = sensor->frmsize_list[index].height;
	binning = sensor->frmsize_list[index].binning;
//	if(binning == true)
	{
		size->width = width;
		size->height = height;
		print_debug("frmsize_list[%d] = %d x %d", index, width, height);
		return 0;
	}

	return -1;
}

/* zhoutian 00195335 2013-03-02 added for SuperZoom-LowLight end > */

/*
 * Added for hue, y36721 todo
 * flag: if 1 is on, 0 is off, default is off
 */
int ispv1_set_hue(int flag)
{
	print_debug("enter %s", __func__);

	return 0;
}

/*
 * before start_preview or start_capture, it should be called to update size information
 * please see ISP manual or software manual.
 */
int ispv1_update_LENC_scale(u32 inwidth, u32 inheight)
{
	u32 scale;

	print_debug("enter %s", __func__);

	scale = (0x100000 * 3) / inwidth;
	SETREG16(REG_ISP_LENC_BRHSCALE, scale);

	scale = (0x100000 * 3) / inheight;
	SETREG16(REG_ISP_LENC_BRVSCALE, scale);

	scale = (0x100000 * 4) / inwidth;
	SETREG16(REG_ISP_LENC_GHSCALE, scale);

	scale = (0x80000 * 4) / inheight;
	SETREG16(REG_ISP_LENC_GVSCALE, scale);

	return 0;

}

/*
 * Related to sensor.
 */
int ispv1_init_LENC(u8 *lensc_param)
{
	u32 loopi;
	u8 *param;

	print_debug("enter %s", __func__);

	assert(lensc_param);

	/* set long exposure */
	param = lensc_param;
	for (loopi = 0; loopi < LENS_CP_ARRAY_BYTES; loopi++)
		SETREG8(REG_ISP_LENS_CP_ARRAY_LONG + loopi, *param++);

	/* set short exposure, just set same with long exposure, y36721 todo */
	param = lensc_param;
	for (loopi = 0; loopi < LENS_CP_ARRAY_BYTES; loopi++)
		SETREG8(REG_ISP_LENS_CP_ARRAY_SHORT + loopi, *param++);

	return 0;
}

/*
 * Related to sensor.
 */
int ispv1_init_CCM(u16 *ccm_param)
{
	u32 loopi;
	u16 *param;

	print_debug("enter %s ", __func__);

	assert(ccm_param);

	param = ccm_param;
	for (loopi = 0; loopi < CCM_MATRIX_ARRAY_SIZE16; loopi++) {
		SETREG8(REG_ISP_CCM_MATRIX + loopi * 2, (*param & 0xff00) >> 8);
		SETREG8(REG_ISP_CCM_MATRIX + loopi * 2 + 1, *param & 0xff);
		param++;
	}

	return 0;
}

/*
 * Related to sensor.
 */
int ispv1_init_AWB(u8 *awb_param)
{
	u32 loopi;
	u8 *param;

	print_debug("enter %s", __func__);

	assert(awb_param);

	param = awb_param;
	for (loopi = 0; loopi < AWB_CTRL_ARRAY_BYTES; loopi++)
		SETREG8(REG_ISP_AWB_CTRL + loopi, *param++);

	SETREG8(REG_ISP_CCM_CENTERCT_THRESHOLDS, *param++);
	SETREG8(REG_ISP_CCM_CENTERCT_THRESHOLDS + 1, *param++);
	SETREG8(REG_ISP_CCM_LEFTCT_THRESHOLDS, *param++);
	SETREG8(REG_ISP_CCM_LEFTCT_THRESHOLDS + 1, *param++);
	SETREG8(REG_ISP_CCM_RIGHTCT_THRESHOLDS, *param++);
	SETREG8(REG_ISP_CCM_RIGHTCT_THRESHOLDS + 1, *param++);

	for (loopi = 0; loopi < LENS_CT_THRESHOLDS_SIZE16; loopi++) {
		SETREG8(REG_ISP_LENS_CT_THRESHOLDS + loopi * 2, *param++);
		SETREG8(REG_ISP_LENS_CT_THRESHOLDS + loopi * 2 + 1, *param++);
	}

	return 0;
}

/* Added for binning correction, y36721 todo */
int ispv1_init_BC(int binningMode, int mirror, int filp)
{
	print_debug("enter %s", __func__);

	return 0;
}

/* Added for defect_pixel_correction, y36721 todo */
int ispv1_init_DPC(int bWhitePixel, int bBlackPixel)
{
	print_debug("enter %s", __func__);

	return 0;
}

/*
 * Added for raw DNS, y36721 todo
 * flag: if 1 is on, 0 is off, default is on
 */
int ispv1_init_rawDNS(int flag)
{
	print_debug("enter %s", __func__);

	return 0;
}

/*
 * Added for uv DNS, y36721 todo
 * flag: if 1 is on, 0 is off, default is on
 */
int ispv1_init_uvDNS(int flag)
{
	print_debug("enter %s", __func__);

	return 0;
}

/*
 * Added for GbGr DNS, y36721 todo
 * level: if >0 is on, 0 is off, default is 6
 * 16: keep full Gb/Gr difference as resolution;
 * 8: remove half Gb/Gr difference;
 * 0: remove all Gb/Gr difference;
 */
int ispv1_init_GbGrDNS(int level)
{
	print_debug("enter %s", __func__);

	return 0;
}

/*
 * Added for GRB Gamma, y36721 todo
 * flag: if 1 is on, 0 is off, default is on, 0x65004 bit[5]
 */
int ispv1_init_RGBGamma(int flag)
{
	print_debug("enter %s", __func__);

	return 0;
}

#define AP_WRITE_AE_TIME_PRINT
/*
 * For not OVT sensors,  MCU won't write exposure and gain back to sensor directly.
 * In this case, exposure and gain need Host to handle.
 */
void ispv1_cmd_id_do_ecgc(camera_state state)
{
	u32 expo, gain, gain0;
	camera_sensor *sensor = this_ispdata->sensor;
#ifdef AP_WRITE_AE_TIME_PRINT
	struct timeval tv_start, tv_end;
	do_gettimeofday(&tv_start);
#endif

	print_debug("enter %s, cmd id 0x%x", __func__, isp_hw_data.aec_cmd_id);

	if (ispv1_is_hdr_movie_mode() == true)
		return;

	expo = get_writeback_expo();
	gain = get_writeback_gain();

	if (CMD_WRITEBACK_EXPO_GAIN == isp_hw_data.aec_cmd_id ||
		CMD_WRITEBACK_EXPO == isp_hw_data.aec_cmd_id ||
		CMD_WRITEBACK_GAIN == isp_hw_data.aec_cmd_id) {
			/* changed by y00231328.some sensor such as S5K4E1, expo and gain should be set together in holdon mode */
			if (sensor->set_exposure_gain) {
				sensor->set_exposure_gain(expo, gain);
				gain0 = get_writeback_gain();
				if(gain0 != gain)
				{
					print_info("gain0 != gain");
					sensor->set_exposure_gain(expo, gain0);
				}
			} else {
				if (sensor->set_exposure)
					sensor->set_exposure(expo);
				if (sensor->set_gain)
					sensor->set_gain(gain);
			}

	} else {
		print_error("%s:unknow cmd id", __func__);
	}

#ifdef AP_WRITE_AE_TIME_PRINT
	do_gettimeofday(&tv_end);
	print_info("expo 0x%x, gain 0x%x, aec_cmd_id 0x%x, used %dus",
		expo, gain, isp_hw_data.aec_cmd_id,
		(int)((tv_end.tv_sec - tv_start.tv_sec)*1000000 + (tv_end.tv_usec - tv_start.tv_usec)));
#endif
}

static int frame_rate_level;
static camera_frame_rate_state fps_state = CAMERA_FPS_STATE_HIGH;;

int ispv1_get_frame_rate_level()
{
	return frame_rate_level;
}

void ispv1_set_frame_rate_level(int level)
{
	frame_rate_level = level;
	print_info("%s: level %d", __func__, level);
}

camera_frame_rate_state ispv1_get_frame_rate_state(void)
{
	return fps_state;
}

void ispv1_set_frame_rate_state(camera_frame_rate_state state)
{
	fps_state = state;
}

static int ispv1_set_frame_rate(camera_frame_rate_mode mode, camera_sensor *sensor)
{
	int frame_rate_level = 0;
	u16 vts, fullfps, fps;
	u32 max_fps, min_fps;
	u16 new_vts;
	fullfps = sensor->frmsize_list[sensor->preview_frmsize_index].fps;
	if (CAMERA_FRAME_RATE_FIX_MAX == mode) {
		max_fps = (isp_hw_data.fps_max > fullfps) ? fullfps : isp_hw_data.fps_max;
		min_fps = max_fps;
		frame_rate_level = 0;
	} else if (CAMERA_FRAME_RATE_FIX_MIN == mode) {
		min_fps = (isp_hw_data.fps_min < fullfps) ? isp_hw_data.fps_min : fullfps;
		max_fps = (isp_hw_data.fps_max > fullfps) ? fullfps : isp_hw_data.fps_max;
		frame_rate_level = max_fps - min_fps;
	}

	fps = fullfps - frame_rate_level;

	sensor->fps = fps;

	/* rules: vts1*fps1 = vts2*fps2 */
	vts = sensor->frmsize_list[sensor->preview_frmsize_index].vts;
	vts = vts * fullfps / fps;

	if (sensor->set_vts) {
		sensor->set_vts(vts);
	} else {
		print_error("set_vts null");
		goto error;
	}
	if(sensor->get_support_vts)
	{
		new_vts = sensor->get_support_vts(fps,fullfps,sensor->frmsize_list[sensor->preview_frmsize_index].vts);
	}
	else
	{
		new_vts = vts - 14;
	}
	SETREG16(REG_ISP_MAX_EXPOSURE, new_vts);
	ispv1_set_frame_rate_level(frame_rate_level);
	return 0;
error:
	return -1;
}

static bool ispv1_change_frame_rate(
	camera_frame_rate_state *state, camera_frame_rate_dir direction, camera_sensor *sensor)
{
	int frame_rate_level = ispv1_get_frame_rate_level();
	u16 vts, fullfps, fps,new_vts;
	bool level_changed = false;
	u32 max_fps, min_fps, mid_fps;
	u32 shift_level, max_level;

	fullfps = sensor->frmsize_list[sensor->preview_frmsize_index].fps;
	max_fps = (isp_hw_data.fps_max > fullfps) ? fullfps : isp_hw_data.fps_max;
	mid_fps = (isp_hw_data.fps_mid > fullfps) ? fullfps : isp_hw_data.fps_mid;
	min_fps = (isp_hw_data.fps_min < fullfps) ? isp_hw_data.fps_min : fullfps;

	if (mid_fps > max_fps)
		mid_fps = max_fps;

	if (min_fps > max_fps)
		min_fps = max_fps;
	max_level = max_fps - min_fps;

	print_debug("%s: state  %d, frame_rate_level %d", __func__, *state, frame_rate_level);

	if (*state == CAMERA_EXPO_PRE_REDUCE1) {
		/* desired level should go to FPS_HIGH level */
		level_changed = true;
		shift_level = max_fps - mid_fps;
		frame_rate_level -= shift_level;
		*state = CAMERA_FPS_STATE_HIGH;
		goto framerate_set_done;
	} else if (*state == CAMERA_EXPO_PRE_REDUCE2) {
		/* desired level should go to FPS_MIDDLE level */
		level_changed = true;
		shift_level = mid_fps - min_fps;
		frame_rate_level -= shift_level;
		*state = CAMERA_FPS_STATE_MIDDLE;
		goto framerate_set_done;
	}

	if (CAMERA_FRAME_RATE_DOWN == direction) {
		if (frame_rate_level >= max_level) {
			print_debug("Has arrival max frame_rate level");
			return true;
		} else if (frame_rate_level == 0) {
			shift_level = max_fps - mid_fps;
			frame_rate_level += shift_level;
			*state = CAMERA_FPS_STATE_MIDDLE;
			level_changed = true;
		} else {
			shift_level = mid_fps - min_fps;
			frame_rate_level += shift_level;
			*state = CAMERA_FPS_STATE_LOW;
			level_changed = true;
		}
	} else if (CAMERA_FRAME_RATE_UP == direction) {
		if (0 == frame_rate_level) {
			print_debug("Has arrival min frame_rate level");
			return true;
		} else if (frame_rate_level == max_fps - mid_fps) {
			shift_level = max_fps - mid_fps;
			frame_rate_level -= shift_level;
			*state = CAMERA_FPS_STATE_HIGH;
			level_changed = true;
		} else {
			shift_level = mid_fps - min_fps;
			frame_rate_level -= shift_level;
			*state = CAMERA_FPS_STATE_MIDDLE;
			level_changed = true;
		}
	}

framerate_set_done:
	fps = fullfps - frame_rate_level;
	if ((fps > sensor->fps_max) || (fps < sensor->fps_min)) {
		print_info("auto fps:%d", fps);
		return true;
	}

	if (true == level_changed) {
		if ((sensor->fps_min <= fps) && (sensor->fps_max >= fps)) {
			sensor->fps = fps;
		} else if (sensor->fps_min > fps) {
			frame_rate_level = 0;
			sensor->fps = fullfps;
			fps = fullfps;
		} else {
			print_error("can't do auto fps, level:%d, cur_fps:%d, tar_fps:%d, ori_fps:%d, max_fps:%d, min_fps:%d",
				frame_rate_level, sensor->fps, fps, fullfps, sensor->fps_max, sensor->fps_min);
			goto error_out;
		}

		/* rules: vts1*fps1 = vts2*fps2 */
		vts = sensor->frmsize_list[sensor->preview_frmsize_index].vts;
		vts = vts * fullfps / fps;

		if(sensor->get_support_vts)
		{
			new_vts = sensor->get_support_vts(fps,fullfps,sensor->frmsize_list[sensor->preview_frmsize_index].vts);
		}
		else
		{
			new_vts = vts - 14;
		}
		/* y36721 2012-04-23 add begin to avoid preview flicker when frame rate up */
		if (new_vts < (get_writeback_expo() / 0x10)) {
			SETREG16(REG_ISP_MAX_EXPOSURE, new_vts);
			print_warn("current expo too large");
			if (*state == CAMERA_FPS_STATE_HIGH)
				*state = CAMERA_EXPO_PRE_REDUCE1;
			else
				*state = CAMERA_EXPO_PRE_REDUCE2;
			return true;
		}
		/* y36721 2012-04-23 add end */
		if (CAMERA_FRAME_RATE_DOWN == direction) {
			if(sensor->set_vts_change)
			{
				sensor->set_vts_change(1);
			}
		}

		if (sensor->set_vts) {
			sensor->set_vts(vts);
		} else {
			print_error("set_vts null");
			goto error_out;
		}
		SETREG16(REG_ISP_MAX_EXPOSURE, new_vts);
		ispv1_set_frame_rate_level(frame_rate_level);
	}
	return true;

error_out:
	return false;
}

/* #define PLATFORM_TYPE_PAD_S10 */
#define BOARD_ID_CS_U9510		0x67
#define BOARD_ID_CS_U9510E		0x66
#define BOARD_ID_CS_T9510E		0x06


awb_gain_t flash_platform_awb[FLASH_PLATFORM_MAX] =
{
	{0xc8, 0x80, 0x80, 0x104}, /* U9510 */
	{0xd6, 0x80, 0x80, 0x104}, /* U9510E/T9510E, recording AWB is 0xd0,0xfc, change a little */
	{0xd0, 0x80, 0x80, 0x100}, /* s10 */
	{0xc8, 0x80, 0x80, 0xfb}, /*D2*/
};

static void ispv1_cal_capture_awb(
	awb_gain_t *preview_awb, awb_gain_t *flash_awb, awb_gain_t *capture_awb,
	u32 weight_env, u32 weight_flash)
{
	capture_awb->b_gain = (weight_flash * flash_awb->b_gain + weight_env * preview_awb->b_gain) / 0x100;
	capture_awb->gb_gain = (weight_flash * flash_awb->gb_gain + weight_env * preview_awb->gb_gain) / 0x100;
	capture_awb->gr_gain = (weight_flash * flash_awb->gr_gain + weight_env * preview_awb->gr_gain) / 0x100;
	capture_awb->r_gain = (weight_flash * flash_awb->r_gain + weight_env * preview_awb->r_gain) / 0x100;
}

void ispv1_get_wb_value(awb_gain_t *awb)
{
	GETREG16(MANUAL_AWB_GAIN_B, awb->b_gain);
	GETREG16(MANUAL_AWB_GAIN_GB, awb->gb_gain);
	GETREG16(MANUAL_AWB_GAIN_GR, awb->gr_gain);
	GETREG16(MANUAL_AWB_GAIN_R, awb->r_gain);
}

void ispv1_set_wb_value(awb_gain_t *awb)
{
	SETREG16(MANUAL_AWB_GAIN_B, awb->b_gain);
	SETREG16(MANUAL_AWB_GAIN_GB, awb->gb_gain);
	SETREG16(MANUAL_AWB_GAIN_GR, awb->gr_gain);
	SETREG16(MANUAL_AWB_GAIN_R, awb->r_gain);
}

static void ispv1_cal_weight(
	u32 lum_env, u32 lum_flash,
	u32 *weight_env, u32 *weight_flash)
{
	if ((lum_flash + lum_env) != 0) {
		*weight_env = 0x100 * lum_env / (lum_flash + lum_env);
		*weight_flash = 0x100 * lum_flash / (lum_flash + lum_env);
	} else {
		*weight_env = 0x100;
		*weight_flash = 0;
	}
}

static void ispv1_cal_flash_awb(
	u32 weight_env, u32 weight_flash,
	awb_gain_t *preview_awb, awb_gain_t *preflash_awb, awb_gain_t *flash_awb)
{
	if (weight_flash != 0) {
		flash_awb->b_gain = abs((int)preflash_awb->b_gain * 0x100 - (int)preview_awb->b_gain * weight_env) / weight_flash;
		flash_awb->gb_gain = abs((int)preflash_awb->gb_gain * 0x100 - (int)preview_awb->gb_gain * weight_env) / weight_flash;
		flash_awb->gr_gain = abs((int)preflash_awb->gr_gain * 0x100 - (int)preview_awb->gr_gain * weight_env) / weight_flash;
		flash_awb->r_gain = abs((int)preflash_awb->r_gain * 0x100 - (int)preview_awb->r_gain * weight_env) / weight_flash;
	} else {
		flash_awb->b_gain = preview_awb->b_gain;
		flash_awb->gb_gain = preview_awb->gb_gain;
		flash_awb->gr_gain = preview_awb->gr_gain;
		flash_awb->r_gain = preview_awb->r_gain;
	}
}

static bool ispv1_check_awb_match(awb_gain_t *flash_awb, awb_gain_t *led0, awb_gain_t *led1, u32 weight_env)
{
	awb_gain_t diff_led0, diff_led1;
	bool ret = true;

	/* if calculated flash_awb is differ a lot from preset_flash_awb, use preset_flash_awb. */
	diff_led0.b_gain = abs((int)led0->b_gain - (int)flash_awb->b_gain);
	diff_led0.r_gain = abs((int)led0->r_gain - (int)flash_awb->r_gain);

	diff_led1.b_gain = abs((int)led1->b_gain - (int)flash_awb->b_gain);
	diff_led1.r_gain = abs((int)led1->r_gain - (int)flash_awb->r_gain);

	if ((diff_led0.b_gain >= led0->b_gain / 3 || diff_led0.r_gain >= led0->r_gain / 3) &&
		(diff_led1.b_gain >= led1->b_gain / 3 || diff_led1.r_gain >= led1->r_gain / 3)) {
		ret = false;
		print_info("differ a lot, cal awb[B 0x%x, R 0x%x]", flash_awb->b_gain, flash_awb->r_gain);
	}

	return ret;
}

static bool ispv1_need_rcc(camera_sensor *sensor)
{
	bool ret = false;
	if (sensor->isp_location == CAMERA_USE_K3ISP && sensor->rcc_enable == true)
		ret = true;
	return ret;
}

static void ispv1_cal_ratio_lum(
	aec_data_t *preview_ae, aec_data_t *preflash_ae,
	aec_data_t *capflash_ae, u32 *preview_ratio_lum, flash_weight_t *weight)
{
	aec_data_t ratio_ae;
	int delta_lum, delta_lum_max, delta_lum_sum;
	u32 ratio = 0x100;

	print_debug("preview:[0x%x,0x%x,lum 0x%x,lumMax 0x%x,lumSum 0x%x]; preflash:[0x%x,0x%x,lum 0x%x,LumMax 0x%x,lumSum 0x%x]",
		preview_ae->gain, preview_ae->expo, preview_ae->lum, preview_ae->lum_max, preview_ae->lum_sum,
		preflash_ae->gain, preflash_ae->expo, preflash_ae->lum, preflash_ae->lum_max, preflash_ae->lum_sum);

	ratio = ratio * (preflash_ae->gain * preflash_ae->expo) / (preview_ae->gain * preview_ae->expo);
	if (ratio != 0) {
		ratio_ae.lum = preview_ae->lum * ratio / 0x100;
		ratio_ae.lum_max = preview_ae->lum_max * ratio / 0x100;
		ratio_ae.lum_sum = preview_ae->lum_sum * ratio / 0x100;
	} else {
		ratio_ae.lum = 0;
		ratio_ae.lum_max = 0;
		ratio_ae.lum_sum = 0;
	}

	/* delta_lum is lum of env part */
	delta_lum = preflash_ae->lum - ratio_ae.lum;
	if (delta_lum < 0)
		delta_lum = 0;

	delta_lum_max = preflash_ae->lum_max - ratio_ae.lum_max;
	if (delta_lum_max < 0)
		delta_lum_max = 0;

	delta_lum_sum = preflash_ae->lum_sum - ratio_ae.lum_sum;
	if (delta_lum_sum < 0)
		delta_lum_sum = 0;

	if (product_type("U9900") || product_type("U9900L") || product_type("T9900")){
		capflash_ae->lum = FLASH_CAP2PRE_RATIO * delta_lum + ratio_ae.lum;
		capflash_ae->lum_max = FLASH_CAP2PRE_RATIO * delta_lum_max + ratio_ae.lum_max;
		capflash_ae->lum_sum = FLASH_CAP2PRE_RATIO * delta_lum_sum + ratio_ae.lum_sum;
	}else{
		capflash_ae->lum = D2_FLASH_CAP2PRE_RATIO * delta_lum + ratio_ae.lum;
		capflash_ae->lum_max = D2_FLASH_CAP2PRE_RATIO * delta_lum_max + ratio_ae.lum_max;
		capflash_ae->lum_sum = D2_FLASH_CAP2PRE_RATIO * delta_lum_sum + ratio_ae.lum_sum;
	}

	/* if low light, we will use lum_sum to calculate weight for accuracy. */
	if (preflash_ae->lum < FLASH_PREFLASH_LOWLIGHT_TH) {
		ispv1_cal_weight(ratio_ae.lum_sum, preflash_ae->lum_sum - ratio_ae.lum_sum, &(weight->preflash_env), &(weight->preflash_flash));
		ispv1_cal_weight(ratio_ae.lum_sum, capflash_ae->lum_sum - ratio_ae.lum_sum, &(weight->capflash_env), &(weight->capflash_flash));
	} else {
		ispv1_cal_weight(ratio_ae.lum, preflash_ae->lum - ratio_ae.lum, &(weight->preflash_env), &(weight->preflash_flash));
		ispv1_cal_weight(ratio_ae.lum, capflash_ae->lum - ratio_ae.lum, &(weight->capflash_env), &(weight->capflash_flash));
	}

	/* if calculated capture flash lum is zero, set it as 1 to avoid zero divide panic. */
	if (capflash_ae->lum == 0)
		capflash_ae->lum = 1;

	*preview_ratio_lum = ratio_ae.lum;
}
#if 0
u32 ispv1_cal_ratio_factor(aec_data_t *preview_ae, aec_data_t *preflash_ae, aec_data_t *capflash_ae, 
	u32 target_y_low)
{
	u32 ratio_factor = 0x100;
	u32 temp_ratio = 0x100;
	u32 capture_target_lum;

	if (capflash_ae->lum != 0) {
		if (preview_ae->lum < target_y_low) {
			capture_target_lum = DEFAULT_TARGET_Y_FLASH;
			ratio_factor = ratio_factor * capture_target_lum / capflash_ae->lum;
			capflash_ae->lum_max = capflash_ae->lum_max * ratio_factor / 0x100;
			if (capflash_ae->lum_max > FLASH_CAP_RAW_OVER_EXPO) {
				/* decrease capflash max lum to  FLASH_CAP_RAW_OVER_EXPO */
				temp_ratio = 0x100 * FLASH_CAP_RAW_OVER_EXPO / capflash_ae->lum_max;
				ratio_factor = ratio_factor * temp_ratio / 0x100;
				capflash_ae->lum_max = capflash_ae->lum_max * temp_ratio / 0x100;
				print_info("%s, capflash lum_max 0x%x, temp_ratio 0x%x, new ratio_factor 0x%x",
					__func__, capflash_ae->lum_max, temp_ratio, ratio_factor);
			}
		} else {
			capture_target_lum = preview_ae->lum;
			ratio_factor = ratio_factor * capture_target_lum / capflash_ae->lum;
		}
	} else {
		ratio_factor = ISP_EXPOSURE_RATIO_MAX;
	}

	return ratio_factor;
}
#else
u32 ispv1_cal_ratio_factor(aec_data_t *preview_ae, aec_data_t *preflash_ae, aec_data_t *capflash_ae, 
	u32 target_y_low)
{
	u32 ratio_factor = 0x100;
	u32 temp_ratio = 0x100;
	u32 capture_target_lum;

	if (capflash_ae->lum != 0) {
		if (preview_ae->lum < target_y_low) {
			capture_target_lum = default_target_y_flash;
			ratio_factor = ratio_factor * capture_target_lum / capflash_ae->lum;
			capflash_ae->lum_max = capflash_ae->lum_max * ratio_factor / 0x100;
			if (capflash_ae->lum_max > flash_cap_raw_over_expo) {
				/* decrease capflash max lum to  flash_cap_raw_over_expo */
				temp_ratio = 0x100 * flash_cap_raw_over_expo / capflash_ae->lum_max;
				ratio_factor = ratio_factor * temp_ratio / 0x100;
				capflash_ae->lum_max = capflash_ae->lum_max * temp_ratio / 0x100;
				print_info("%s, capflash lum_max 0x%x, temp_ratio 0x%x, new ratio_factor 0x%x",
					__func__, capflash_ae->lum_max, temp_ratio, ratio_factor);
			}
		} else {
			capture_target_lum = preview_ae->lum;
			ratio_factor = ratio_factor * capture_target_lum / capflash_ae->lum;
		}
	} else {
		ratio_factor = ISP_EXPOSURE_RATIO_MAX;
	}

	return ratio_factor;
}

#endif
void ispv1_save_aecawb_step(aecawb_step_t *step)
{
	step->stable_range0 = GETREG8(REG_ISP_UNSTABLE2STABLE_RANGE);
	step->stable_range1 = GETREG8(REG_ISP_STABLE2UNSTABLE_RANGE);
	step->fast_step = GETREG8(REG_ISP_FAST_STEP);
	step->slow_step = GETREG8(REG_ISP_SLOW_STEP);
	step->awb_step = GETREG8(REG_ISP_AWB_STEP);
}

void ispv1_config_aecawb_step(bool flash_mode, aecawb_step_t *step)
{
	if (flash_mode == true) {
		SETREG8(REG_ISP_UNSTABLE2STABLE_RANGE, (DEFAULT_FLASH_AEC_FAST_STEP / 4));
		SETREG8(REG_ISP_STABLE2UNSTABLE_RANGE, DEFAULT_FLASH_AEC_FAST_STEP);
		SETREG8(REG_ISP_FAST_STEP, DEFAULT_FLASH_AEC_FAST_STEP);
		SETREG8(REG_ISP_SLOW_STEP, DEFAULT_FLASH_AEC_SLOW_STEP);
		SETREG8(REG_ISP_AWB_STEP, DEFAULT_FLASH_AWB_STEP); //awb step
	} else {
		SETREG8(REG_ISP_UNSTABLE2STABLE_RANGE, step->stable_range0);
		SETREG8(REG_ISP_STABLE2UNSTABLE_RANGE, step->stable_range1);
		SETREG8(REG_ISP_FAST_STEP, step->fast_step);
		SETREG8(REG_ISP_SLOW_STEP, step->slow_step);
		SETREG8(REG_ISP_AWB_STEP, step->awb_step); //awb step
	}
}

/* check if current env AE status is need auto-flash. */
bool ae_is_need_flash(camera_sensor *sensor, aec_data_t *ae_data, u32 target_y_low)
{
	u32 frame_index = sensor->preview_frmsize_index;
	u32 compare_gain = CAMERA_FLASH_TRIGGER_GAIN;
	bool binning;
	bool ret;

	if (sensor->get_override_param)
		compare_gain = sensor->get_override_param(OVERRIDE_FLASH_TRIGGER_GAIN);

	binning = sensor->frmsize_list[frame_index].binning;
	if(BINNING_SUMMARY == sensor->support_binning_type)
	{
		if (binning == false)
			compare_gain *= 2;
	}

	if (ae_data->lum <= (target_y_low * FLASH_TRIGGER_LUM_RATIO / 0x100) ||
		ae_data->gain >= compare_gain)
		ret = true;
	else
		ret = false;

	return ret;
}

static inline void set_awbtest_policy(FLASH_AWBTEST_POLICY action)
{
	isp_hw_data.awb_test = action;
}

static inline FLASH_AWBTEST_POLICY get_awbtest_policy(void)
{
	return isp_hw_data.awb_test;
}

void ispv1_check_flash_prepare(void)
{
	camera_sensor *sensor = this_ispdata->sensor;

	flash_platform_t type;
	int boardid = 0;

	awb_gain_t *preset_awb = &(isp_hw_data.flash_preset_awb);
	awb_gain_t *led_awb0 = &(isp_hw_data.led_awb[0]);
	awb_gain_t *led_awb1 = &(isp_hw_data.led_awb[1]);
	FLASH_AWBTEST_POLICY action;

	if ((afae_ctrl_is_valid() == true) &&
		(FOCUS_STATE_CAF_RUNNING == get_focus_state() ||
		FOCUS_STATE_CAF_DETECTING == get_focus_state() ||
		FOCUS_STATE_AF_RUNNING == get_focus_state())) {
		print_debug("enter %s, must stop focus, before turn on preflash. ", __func__);
		ispv1_auto_focus(FOCUS_STOP);
	}

	/* enlarge AEC/AGC step to make AEC/AGC stable quickly. */
	ispv1_config_aecawb_step(true, &isp_hw_data.aecawb_step);

	/* get platform type */
	#ifdef PLATFORM_TYPE_PAD_S10
		type = FLASH_PLATFORM_S10;
	#else
		boardid = get_boardid();
		print_info("%s : boardid=0x%x.", __func__, boardid);

		/* if unknow board ID, should use flash params as X9510E */
		if (boardid == BOARD_ID_CS_U9510E || boardid == BOARD_ID_CS_T9510E)
			type = FLASH_PLATFORM_9510E;
		else if (boardid == BOARD_ID_CS_U9510)
			type = FLASH_PLATFORM_U9510;
		else if (product_type("U9900") || product_type("U9900L") || product_type("T9900"))
			type = FLASH_PLATFORM_9510E;
		else
			type = FLASH_PLATFORM_D2;
	#endif

	if(sensor->get_flash_awb != NULL)
		sensor->get_flash_awb(type, preset_awb);
	else
		memcpy(preset_awb, &flash_platform_awb[type], sizeof(awb_gain_t));

	if (preset_awb->gb_gain == 0x80 && preset_awb->gr_gain == 0x80) {
		action = FLASH_AWBTEST_POLICY_FIXED;
	} else {
		led_awb0->b_gain = preset_awb->b_gain;
		led_awb0->gb_gain = 0x80;
		led_awb0->gr_gain = 0x80;
		led_awb0->r_gain = preset_awb->gb_gain;

		led_awb1->b_gain = preset_awb->gr_gain;
		led_awb1->gb_gain = 0x80;
		led_awb1->gr_gain = 0x80;
		led_awb1->r_gain = preset_awb->r_gain;

		preset_awb->b_gain = led_awb0->b_gain;
		preset_awb->gb_gain = 0x80;
		preset_awb->gr_gain = 0x80;
		preset_awb->r_gain = led_awb0->r_gain;

		action = FLASH_AWBTEST_POLICY_FREEGO;
	}

	set_awbtest_policy(action);

	/* If clearly know led type, need do noting, else will do someting. */
	if (action == FLASH_AWBTEST_POLICY_FIXED) {
		return;
	}

	ispv1_set_awb_mode(MANUAL_AWB);
	ispv1_set_wb_value(preset_awb);

	print_info("unsure flash led AWB, preset_awb[B 0x%x, R 0x%x]", preset_awb->b_gain, preset_awb->r_gain);
}

void ispv1_check_flash_exit(void)
{
	ispv1_config_aecawb_step(false, &isp_hw_data.aecawb_step);
}
static void ispv1_poll_flash_lum(void)
{
	static u8 frame_count;
	static u8 frozen_frame;
	k3_isp_data *ispdata = this_ispdata;
	camera_sensor *sensor = ispdata->sensor;

	u32 volatile cur_lum;

	aec_data_t *preview_ae = &(isp_hw_data.preview_ae);
	aec_data_t *preflash_ae = &(isp_hw_data.preflash_ae);
	aec_data_t capflash_ae;
	u32 preview_ratio_lum;

	awb_gain_t *preview_awb = &(isp_hw_data.preview_awb);
	awb_gain_t *preset_awb = &(isp_hw_data.flash_preset_awb);
	awb_gain_t flash_awb = {0x80, 0x80, 0x80, 0x80};
	awb_gain_t capture_awb;

	static awb_gain_t last_awb;
	awb_gain_t cur_awb;
	awb_gain_t diff_awb;
	awb_gain_t *led_awb0 = &(isp_hw_data.led_awb[0]);
	awb_gain_t *led_awb1 = &(isp_hw_data.led_awb[1]);

	flash_weight_t weight;
	bool match_result;
	bool need_flash;
	bool low_ct;

	u8 target_y_low = GETREG8(REG_ISP_TARGET_Y_LOW);
	u8 over_expo_th = GETREG8(REG_ISP_TARGET_Y_HIGH) + DEFAULT_FLASH_AEC_SLOW_STEP;

	u32 unit_area;

	cur_lum = get_current_y();
	ispv1_get_wb_value(&cur_awb);

	/* stage 1: skip first 2 frames */
	if (++frame_count <= 2) {
		if (get_awbtest_policy() == FLASH_AWBTEST_POLICY_FIXED) {
			ispv1_set_aecagc_mode(AUTO_AECAGC);
			ispv1_set_awb_mode(AUTO_AWB);
		} else {
			ispv1_set_aecagc_mode(AUTO_AECAGC);

			/*
			 * first frame set manual AWB and init WB value again to ensure it take effect;
			 * second frame set Auto awb.
			 */
			if (frame_count == 1) {
				ispv1_set_awb_mode(MANUAL_AWB);
				ispv1_set_wb_value(preset_awb);
			} else if (frame_count == 2)
				ispv1_set_awb_mode(AUTO_AWB);
		}
		return;
	}

	/* stage 2: FLASH_TESTING: if aec/agc suitable or frame_count larger than threshold, should go to FLASH_FROZEN. */
	if ((ispdata->flash_flow == FLASH_TESTING) &&
		(cur_lum <= over_expo_th || frame_count >= FLASH_TEST_MAX_COUNT)) {
		ispv1_set_aecagc_mode(MANUAL_AECAGC);
		print_info("AEC/AGC OK, set to manual AECAGC, frame_count %d########", frame_count);

		if (get_awbtest_policy() == FLASH_AWBTEST_POLICY_FIXED) {
			ispv1_set_awb_mode(MANUAL_AWB);
			print_info("AWB set to manual, need not test flash awb########");
		}
		ispdata->flash_flow = FLASH_FROZEN;
	} else if (ispdata->flash_flow == FLASH_FROZEN) {
		/* stage 3: FLASH_FROZEN */
		frozen_frame++;

		if (get_awbtest_policy() == FLASH_AWBTEST_POLICY_FIXED) {
			/* frozen second frame, get locked AE value, maybe need adjust lum_max */
			if (frozen_frame > 1) {
				preflash_ae->gain = get_writeback_gain();
				preflash_ae->expo = get_writeback_expo();
				preflash_ae->lum = cur_lum;
				preflash_ae->lum_max = cur_lum; /* set default value */
				preflash_ae->lum_sum = cur_lum; /* set default value */

				/* if Low light, get preflash lum_max */
				if (preview_ae->lum < target_y_low) {
					unit_area = ispv1_get_stat_unit_area();
					ispv1_get_raw_lum_info(preflash_ae, unit_area);
				}
				goto normal_fixawb_out;
			} else {
				/* do nothing, just wait and return */
				return;
			}
		} else {
			/* check whether awb is stable. */
			if (frozen_frame == 1) {
				/* first frame, set last_awb init value as cur_awb */
				memcpy(&last_awb, &cur_awb, sizeof(awb_gain_t));
				return;
			}

			/* odd frames, we compare cur_awb with last_awb, if differ too much, continue. */
			if (frozen_frame % 2 != 0 && frozen_frame < FLASH_TEST_MAX_COUNT) {
				diff_awb.b_gain = abs((int)cur_awb.b_gain - (int)last_awb.b_gain);
				diff_awb.r_gain = abs((int)cur_awb.r_gain - (int)last_awb.r_gain);

				if (diff_awb.b_gain < 0x3 && diff_awb.r_gain < 0x3) {
					frozen_frame = FLASH_TEST_MAX_COUNT;
				} else {
					/* update last_awb using cur_awb. */
					memcpy(&last_awb, &cur_awb, sizeof(awb_gain_t));
					return;
				}
			}

			if (frozen_frame < FLASH_TEST_MAX_COUNT) {
				return;
			} else if (frozen_frame == FLASH_TEST_MAX_COUNT) {
				/* awb test OK or frozen_frame count reach MAX_COUNT*/
				ispv1_set_awb_mode(MANUAL_AWB);
				print_info("AWB differ OK or Reach MAX_COUNT, set to manual AWB########");
				return;
			} else {
				/* get locked AE value, maybe need adjust lum_max */
				preflash_ae->gain = get_writeback_gain();
				preflash_ae->expo = get_writeback_expo();
				preflash_ae->lum = cur_lum;
				preflash_ae->lum_max = cur_lum; /* set default value */
				preflash_ae->lum_sum = cur_lum; /* set default value */

				/* if Low light, get preflash lum_max */
				if (preview_ae->lum < target_y_low) {
					unit_area = ispv1_get_stat_unit_area();
					ispv1_get_raw_lum_info(preflash_ae, unit_area);
				}

				/* calculate capture flash lum */
				ispv1_cal_ratio_lum(preview_ae, preflash_ae, &capflash_ae, &preview_ratio_lum, &weight);

				/* calculate flash awb */
				ispv1_cal_flash_awb(weight.preflash_env, weight.preflash_flash, preview_awb, &cur_awb, &flash_awb);

				match_result = ispv1_check_awb_match(&flash_awb, led_awb0, led_awb1, weight.capflash_env);
				need_flash = ae_is_need_flash(sensor, preview_ae, target_y_low);
				if (preview_awb->b_gain >= FLASH_PREVIEW_LOWCT_TH)
					low_ct = true;
				else
					low_ct = false;

				if (low_ct == true && need_flash == false) {
					memcpy(&flash_awb, preset_awb, sizeof(awb_gain_t));
				} else if (match_result == true) {
					print_info("awb freego match, before compesate awb[B 0x%x, R 0x%x]", flash_awb.b_gain, flash_awb.r_gain);
					flash_awb.b_gain -= (flash_awb.b_gain / 16);
					flash_awb.r_gain += (flash_awb.r_gain / 16);
				} else {
					memcpy(&flash_awb, preset_awb, sizeof(awb_gain_t));
				}

				goto awbtest_out;
			}
		}

normal_fixawb_out:
		ispv1_cal_ratio_lum(preview_ae, preflash_ae, &capflash_ae, &preview_ratio_lum, &weight);
		memcpy(&flash_awb, preset_awb, sizeof(awb_gain_t));

awbtest_out:
		ispv1_cal_capture_awb(preview_awb, &flash_awb, &capture_awb,
			weight.capflash_env, weight.capflash_flash);

		/* To Capture_cmd: update preview_ratio_lum and flash ratio factor */
		isp_hw_data.preview_ratio_lum = preview_ratio_lum;

		/* configure ratio_factor */
		isp_hw_data.ratio_factor = ispv1_cal_ratio_factor(preview_ae, preflash_ae, &capflash_ae, target_y_low);

		print_info("preview[gain:0x%x,expo:0x%x,lum:0x%x,lumMax:0x%x,lumSum:0x%x], awb[B 0x%x, R 0x%x], preflash_weight_env 0x%x",
			preview_ae->gain, preview_ae->expo, preview_ae->lum, preview_ae->lum_max, preview_ae->lum_sum,
			preview_awb->b_gain, preview_awb->r_gain, weight.preflash_env);
		print_info("preflash[gain:0x%x,expo:0x%x,lum:0x%x,lumMax:0x%x,lumSum:0x%x], awb[B 0x%x, R 0x%x], preview_ratio_lum 0x%x",
			preflash_ae->gain, preflash_ae->expo, preflash_ae->lum, preflash_ae->lum_max, preflash_ae->lum_sum,
			cur_awb.b_gain, cur_awb.r_gain, preview_ratio_lum);
		print_info("capflash y:0x%x, max:0x%x, ratio_factor:0x%x, awb[0x%x:0x%x], capflash weight 0x%x",
			capflash_ae.lum, capflash_ae.lum_max, isp_hw_data.ratio_factor,
			capture_awb.b_gain, capture_awb.r_gain, weight.capflash_flash);

		ispv1_set_wb_value(&capture_awb);
		ispdata->flash_flow = FLASH_DONE;
		ispv1_check_flash_exit();
		frozen_frame = 0;
		frame_count = 0;
	}
}

#define ISP_DENOISE_ARRAY_SIZE		7

void ispv1_dynamic_ydenoise(camera_sensor *sensor, camera_state state, bool flash_on)
{
	u8 ydenoise_value[ISP_DENOISE_ARRAY_SIZE];
	u16 uvdenoise_value[ISP_DENOISE_ARRAY_SIZE];

	if(NULL == sensor->fill_denoise_buf)
		return;

	memset(ydenoise_value, 0, sizeof(ydenoise_value));
	memset(uvdenoise_value, 0, sizeof(uvdenoise_value));

	sensor->fill_denoise_buf(ydenoise_value, uvdenoise_value,ISP_DENOISE_ARRAY_SIZE,state,flash_on);

	SETREG8(REG_ISP_YDENOISE_1X, ydenoise_value[0]);
	SETREG8(REG_ISP_YDENOISE_2X, ydenoise_value[1]);
	SETREG8(REG_ISP_YDENOISE_4X, ydenoise_value[2]);
	SETREG8(REG_ISP_YDENOISE_8X, ydenoise_value[3]);
	SETREG8(REG_ISP_YDENOISE_16X, ydenoise_value[4]);
	SETREG8(REG_ISP_YDENOISE_32X, ydenoise_value[5]);
	SETREG8(REG_ISP_YDENOISE_64X, ydenoise_value[6]);

	SETREG16(REG_ISP_UVDENOISE_1X, uvdenoise_value[0]);
	SETREG16(REG_ISP_UVDENOISE_2X, uvdenoise_value[1]);
	SETREG16(REG_ISP_UVDENOISE_4X, uvdenoise_value[2]);
	SETREG16(REG_ISP_UVDENOISE_8X, uvdenoise_value[3]);
	SETREG16(REG_ISP_UVDENOISE_16X, uvdenoise_value[4]);
	SETREG16(REG_ISP_UVDENOISE_32X, uvdenoise_value[5]);
	SETREG16(REG_ISP_UVDENOISE_64X, uvdenoise_value[6]);
}

void ispv1_dynamic_framerate(camera_sensor *sensor, camera_iso iso)
{
	static u32 count;

	u16 gain_th_up[2] = {CAMERA_AUTOFPS_GAIN_LOW2MID, CAMERA_AUTOFPS_GAIN_MID2HIGH};
	u16 gain_th_down[2] = {CAMERA_AUTOFPS_GAIN_HIGH2MID, CAMERA_AUTOFPS_GAIN_MID2LOW};
	u16 gain;
	int ret;
	int index;

	camera_frame_rate_dir direction;
	camera_frame_rate_state state = ispv1_get_frame_rate_state();

	if (sensor->get_override_param) {
		gain_th_up[0] = sensor->get_override_param(OVERRIDE_AUTOFPS_GAIN_LOW2MID);
		gain_th_up[1] = sensor->get_override_param(OVERRIDE_AUTOFPS_GAIN_MID2HIGH);
		gain_th_down[0] = sensor->get_override_param(OVERRIDE_AUTOFPS_GAIN_HIGH2MID);
		gain_th_down[1] = sensor->get_override_param(OVERRIDE_AUTOFPS_GAIN_MID2LOW);
	}

	if (CAMERA_ISO_AUTO != iso) {
		if (state == CAMERA_EXPO_PRE_REDUCE1 || state == CAMERA_EXPO_PRE_REDUCE2
			|| ispv1_get_frame_rate_level() != 0) {
			ispv1_change_frame_rate(&state, CAMERA_FRAME_RATE_UP, sensor);
			ispv1_set_frame_rate_state(state);
		}
	} else {
		if (state == CAMERA_EXPO_PRE_REDUCE1 || state == CAMERA_EXPO_PRE_REDUCE2) {
			ret = ispv1_change_frame_rate(&state, CAMERA_FRAME_RATE_UP, sensor);
			ispv1_set_frame_rate_state(state);
			return;
		}

		/*get gain from isp */
		gain = get_writeback_gain();

		/* added for non-binning frame size auto frame rate control start */
		index = sensor->preview_frmsize_index;

		if(BINNING_SUMMARY == sensor->support_binning_type)
		{
		if (sensor->frmsize_list[index].binning == false) {
			gain_th_up[0] *= 2;
			gain_th_up[1] *= 2;
			gain_th_down[0] *= 2;
			gain_th_down[1] *= 2;
		}
		}

		if ((state == CAMERA_FPS_STATE_LOW && gain < gain_th_up[0])
			|| (state == CAMERA_FPS_STATE_MIDDLE && gain < gain_th_up[1])) {
			direction = CAMERA_FRAME_RATE_UP;
			count++;
		} else if ((state == CAMERA_FPS_STATE_HIGH && gain > gain_th_down[0])
			|| (state == CAMERA_FPS_STATE_MIDDLE && gain > gain_th_down[1])) {
			direction = CAMERA_FRAME_RATE_DOWN;
			count++;
		} else {
			count = 0;
		}
		/* added for non-binning frame size auto frame rate control end */

		if (count >= AUTO_FRAME_RATE_TRIGER_COUNT) {
			if (GETREG8(REG_ISP_AECAGC_STABLE)) {
				ret = ispv1_change_frame_rate(&state, direction, sensor);
				ispv1_set_frame_rate_state(state);
				count = 0;
			}
		}
	}
}

void ispv1_preview_done_do_tune(void)
{
	k3_isp_data *ispdata = this_ispdata;
	camera_sensor *sensor;
	static k3_last_state last_state = {CAMERA_SATURATION_MAX, CAMERA_CONTRAST_MAX, CAMERA_BRIGHTNESS_MAX, CAMERA_EFFECT_MAX};
	u8 target_y_low = GETREG8(REG_ISP_TARGET_Y_LOW);
	u32 unit_area;

	camera_frame_rate_state state = ispv1_get_frame_rate_state();

	if (NULL == ispdata || isp_hw_data.frame_count <= 1) {
		return;
	}

	sensor = ispdata->sensor;

	print_debug("preview_done, gain 0x%x, expo 0x%x, current_y 0x%x, flash_on %d",
		get_writeback_gain(), get_writeback_expo(), get_current_y(), this_ispdata->flash_on);

	if (false == ispdata->flash_on) {
		isp_hw_data.preview_ae.gain = get_writeback_gain();
		isp_hw_data.preview_ae.expo = get_writeback_expo();
		isp_hw_data.preview_ae.lum = get_current_y();
		/* set default value for lum_max and lum_sum */
		isp_hw_data.preview_ae.lum_max = isp_hw_data.preview_ae.lum;
		isp_hw_data.preview_ae.lum_sum = isp_hw_data.preview_ae.lum;

		if ((CAMERA_USE_K3ISP == sensor->isp_location) &&
			(isp_hw_data.preview_ae.lum < target_y_low)) {
			unit_area = ispv1_get_stat_unit_area();
			ispv1_get_raw_lum_info(&isp_hw_data.preview_ae, unit_area);
		}
		ispv1_get_wb_value(&isp_hw_data.preview_awb);
	}

	if (CAMERA_USE_SENSORISP == sensor->isp_location) {
		print_debug("auto frame_rate only effect at k3 isp");
		return;
	}

	if ((FLASH_TESTING == ispdata->flash_flow) || (FLASH_FROZEN == ispdata->flash_flow)) {
		if (state == CAMERA_EXPO_PRE_REDUCE1 || state == CAMERA_EXPO_PRE_REDUCE2
			|| ispv1_get_frame_rate_level() != 0) {
			ispv1_set_aecagc_mode(AUTO_AECAGC);
			ispv1_change_frame_rate(&state, CAMERA_FRAME_RATE_UP, sensor);
			ispv1_set_frame_rate_state(state);
			isp_hw_data.ae_resume = true;
		} else {
			ispv1_poll_flash_lum();
		}
	} else if (afae_ctrl_is_valid() == true &&
		((get_focus_state() == FOCUS_STATE_AF_PREPARING) ||
		(get_focus_state() == FOCUS_STATE_AF_RUNNING) ||
		(get_focus_state() == FOCUS_STATE_CAF_RUNNING))) {
		print_debug("focusing metering, should not change frame rate.");
	} else if (isp_hw_data.ae_resume == true && ispdata->flash_on == false) {
		ispv1_set_aecagc_mode(AUTO_AECAGC);
		ispv1_change_frame_rate(&state, CAMERA_FRAME_RATE_DOWN, sensor);
		ispv1_set_frame_rate_state(state);
		isp_hw_data.ae_resume = false;
	} else {
		ispv1_dynamic_framerate(sensor, ispdata->iso);
	}
	if (true == camera_ajustments_flag) {
		last_state.saturation = CAMERA_SATURATION_MAX;
		last_state.contrast = CAMERA_CONTRAST_MAX;
		last_state.brightness = CAMERA_BRIGHTNESS_MAX;
		last_state.effect = CAMERA_EFFECT_MAX;
		camera_ajustments_flag = false;
	}

	/*camera_effect_saturation_done*/
	if ((ispdata->effect != last_state.effect)||(ispdata->saturation != last_state.saturation)) {
		ispv1_set_effect_saturation_done(ispdata->effect, ispdata->saturation);
		last_state.effect = ispdata->effect;
		last_state.saturation = ispdata->saturation;
	}

	/*contrast_done*/
	if (ispdata->contrast != last_state.contrast) {
		ispv1_set_contrast_done(ispdata->contrast);
		last_state.contrast = ispdata->contrast;
	}

	/*brightness_done*/
	if (ispdata->brightness != last_state.brightness) {
		ispv1_set_brightness_done(ispdata->brightness);
		last_state.brightness = ispdata->brightness;
	}

	if (sensor->awb_dynamic_ccm_gain != NULL) {
		u32 frame_index, ae_value, gain, exposure_line;
		awb_gain_t awb_gain;
		ccm_gain_t ccm_gain;

		GETREG16(REG_ISP_AWB_ORI_GAIN_B, awb_gain.b_gain);
		GETREG16(REG_ISP_AWB_ORI_GAIN_R, awb_gain.r_gain);

		gain= get_writeback_gain();
		exposure_line= get_writeback_expo() >> 4;
		ae_value = gain * exposure_line;

		frame_index = sensor->preview_frmsize_index;
		sensor->awb_dynamic_ccm_gain(frame_index, ae_value, &awb_gain, &ccm_gain);

		SETREG8(REG_ISP_CCM_PREGAIN_R, ccm_gain.r_gain);
		SETREG8(REG_ISP_CCM_PREGAIN_B, ccm_gain.b_gain);
	}

#ifndef OVISP_DEBUG_MODE
//	ispv1_dynamic_ydenoise(sensor, ispdata->flash_on);
#endif

	if (isp_hw_data.frame_count == 0)
		return;

	ispv1_wakeup_focus_schedule(false);

	if ((ispv1_need_rcc(this_ispdata->sensor) == true)
		&& (isp_hw_data.frame_count % RED_CLIP_FRAME_INTERVAL == 0)) {
		if (afae_ctrl_is_valid() == false) {
			ispv1_uv_stat_excute();
		} else if ((get_focus_state() != FOCUS_STATE_CAF_RUNNING) &&
			(get_focus_state() != FOCUS_STATE_AF_RUNNING)) {
			ispv1_uv_stat_excute();
		}
	}
}

/*
 * Used for tune ops and AF functions to get isp_data handler
 */
void ispv1_tune_ops_init(k3_isp_data *ispdata)
{
	this_ispdata = ispdata;

	if (ispdata->sensor->isp_location != CAMERA_USE_K3ISP)
		return;

	sema_init(&(this_ispdata->frame_sem), 0);

	/* maybe some fixed configurations, such as focus, ccm, lensc... */
	if (ispdata->sensor->af_enable)
		ispv1_focus_init();
	if (ispv1_need_rcc(this_ispdata->sensor) == true)
		ispv1_uv_stat_init();

	if(this_ispdata->sensor->support_hdr_movie)
		ispv1_hdr_ae_init();

	ispv1_init_DPC(1, 1);

	ispv1_init_rawDNS(1);
	ispv1_init_uvDNS(1);
	ispv1_init_GbGrDNS(1);
	ispv1_init_RGBGamma(1);
	camera_ajustments_flag = true;
	ispv1_set_frame_rate_state(CAMERA_FPS_STATE_HIGH);

	/*
	 *y36721 2012-02-08 delete them for performance tunning.
	 *ispv1_init_CCM(ispdata->sensor->image_setting.ccm_param);
	 *ispv1_init_LENC(ispdata->sensor->image_setting.lensc_param);
	 *ispv1_init_AWB(ispdata->sensor->image_setting.awb_param);
	 */

	ispv1_init_sensor_config(ispdata->sensor);

	ispv1_save_aecawb_step(&isp_hw_data.aecawb_step);
	set_tune_parameter_for_product();
}

/*
 * something need to do after camera exit
 */
void ispv1_tune_ops_exit(void)
{
	if (this_ispdata->sensor->af_enable == 1)
		ispv1_focus_exit();

	if (ispv1_need_rcc(this_ispdata->sensor) == true)
		ispv1_uv_stat_exit();

	if(this_ispdata->sensor->support_hdr_movie)
		ispv1_hdr_ae_exit();

}


void ispv1_tune_ops_prepare(camera_state state)
{
	k3_isp_data *ispdata = this_ispdata;
	camera_sensor *sensor = ispdata->sensor;
	u32 unit_width = this_ispdata->pic_attr[STATE_PREVIEW].in_width / ISP_LUM_WIN_WIDTH_NUM;
	u32 unit_height = this_ispdata->pic_attr[STATE_PREVIEW].in_height / ISP_LUM_WIN_HEIGHT_NUM;

	if (STATE_PREVIEW == state) {

		/* For AF update */
		if (sensor->af_enable)
			ispv1_focus_prepare();

		if (CAMERA_USE_K3ISP == sensor->isp_location) {
			//ispv1_set_ae_statwin(&ispdata->pic_attr[state], ISP_ZOOM_BASE_RATIO);
			ispv1_set_stat_unit_area(unit_height * unit_width);
			up(&(this_ispdata->frame_sem));
		}

		/* need to check whether there is binning or not */
		ispv1_init_BC(1, 0, 0);

		ispdata->flash_flow = FLASH_DONE;
	} else if (STATE_CAPTURE == state) {
		/* we can add some other things to do before capture */
	}

	/* update lens correction scale size */
	ispv1_update_LENC_scale(ispdata->pic_attr[state].in_width, ispdata->pic_attr[state].in_height);
}

/*
 * something need to do before stop_preview and stop_capture
 */
void ispv1_tune_ops_withdraw(camera_state state)
{
	if (this_ispdata->sensor->af_enable)
		ispv1_focus_withdraw();
	if ((ispv1_need_rcc(this_ispdata->sensor) == true)
		&& (down_trylock(&(this_ispdata->frame_sem)) != 0)) {
			/*
			  * when need to excute rcc function and
			  * semaphore frame_sem has already been locked,
			  * flush_workqueue should be called, then relock frame_sem again
			  */
			flush_workqueue(uv_stat_work_queue);
			down(&(this_ispdata->frame_sem));
	}

	if(this_ispdata->sensor->support_hdr_movie)
	{
		if((HDR_MOVIE_SUPPORT == this_ispdata->sensor->support_hdr_movie())
			&&(HDR_MOVIE_ON == this_ispdata->sensor->get_hdr_movie_switch()))
		{
			this_ispdata->sensor->set_hdr_movie_switch(HDR_MOVIE_OFF);
			if (this_ispdata->sensor->init_isp_reg )
			{
				this_ispdata->sensor->init_isp_reg();
			}
		}
	}

}

bool ispv1_is_hdr_movie_mode(void)
{
	u8 ae_ctrl_mode = GETREG8(ISP_AE_CTRL_MODE); /* 1 - AP's ae . 0 - ISP ae. */
	u8 ae_write_mode = GETREG8(ISP_AE_WRITE_MODE); /* 1 - ISP write sensor shutter/gain; 0 - AP write shutter/gain. */

	ae_ctrl_mode &= 0x1;
	ae_write_mode &= 0x1;

	/* AP's AE; AP write sensor shutter & gain.*/
	if (ae_ctrl_mode == AE_CTRL_MODE_AP && ae_write_mode == AE_WRITE_MODE_AP)
		return true;
	else
		return false;
}

/*
 **************************************************************************
 * FunctionName: deal_uv_data_from_preview;
 * Description : To solve red color clip by adjust the value of Refbin register.
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */

void deal_uv_data_from_preview(u8 *pdata, u32 preview_width, u32 preview_height)
{
	u32 uv_strt_addr, uv_pixel_addr;
	u8 rect_idx, rect_idx_x, rect_idx_y;
	u32 i, j;
	u32 v_total, u_total;
	u32 v_mean, u_mean;
	u32 uv_count;
	u8 current_refbin;
	u32 first_rect_idx_row;
	u32 first_rect_idx_col;
	u32 rect_row_num;
	u32 rect_col_num;
	u8 uv_resample;
	red_clip_status rect_clip = RED_CLIP_NONE;

	if(preview_width <= PRIVIEW_WIDTH_LOW)
		uv_resample = RED_CLIP_UV_RESAMPLE_LOW;
	else if(preview_width >= PRIVIEW_WIDTH_HIGH)
		uv_resample = RED_CLIP_UV_RESAMPLE_HIGH;
	else
		uv_resample = RED_CLIP_UV_RESAMPLE_MIDDLE;
	rect_row_num = preview_height * RED_CLIP_DECTECT_RANGE / (200 * RED_CLIP_RECT_ROW_NUM);
	rect_col_num = preview_width * RED_CLIP_DECTECT_RANGE / (100 * RED_CLIP_RECT_COL_NUM);
	first_rect_idx_row = preview_height * (100 - RED_CLIP_DECTECT_RANGE) / 400;
	first_rect_idx_col = preview_width  * (100 - RED_CLIP_DECTECT_RANGE) / 200;

	for(rect_idx = 0; rect_idx < RED_CLIP_RECT_ROW_NUM * RED_CLIP_RECT_COL_NUM; rect_idx++){
		rect_idx_x = rect_idx / RED_CLIP_RECT_COL_NUM;
		rect_idx_y = rect_idx % RED_CLIP_RECT_COL_NUM;
		uv_strt_addr = (first_rect_idx_row + rect_row_num * rect_idx_x) * preview_width
						+ first_rect_idx_col+ rect_col_num * rect_idx_y;
		v_total = 0;
		u_total = 0;
		uv_count = 0;

		for (i = 0; i < rect_row_num; i = i + uv_resample) {
			for (j = 0; j < rect_col_num; j = j + uv_resample * 2) {
				uv_pixel_addr = uv_strt_addr + i * preview_width + j;
				v_total += pdata[uv_pixel_addr / 2 * 2];
				u_total += pdata[uv_pixel_addr / 2 * 2 + 1];
				uv_count++;
			}
		}
		v_mean = v_total / uv_count;
		u_mean = u_total / uv_count;
		current_refbin = GETREG8(REG_ISP_REF_BIN);
		/*  ( (v - 128) > (u - 128) * 4  / 3 && (v - 128) > -(u - 128) * 4 / 3 && v > th_high) */
		if ((v_mean >= RED_CLIP_V_THRESHOLD_HIGH) && (v_mean + 128 / 3 > u_mean * 4 / 3) && (v_mean + u_mean * 4 / 3 > 128 * 7 / 3)) {
			rect_clip = RED_CLIP;
			break;
		}
		/*  ( (v - 128) > (u - 128) && (v - 128) > -(u - 128) && v > th_low)   */
		if((v_mean >= RED_CLIP_V_THRESHOLD_LOW) && (v_mean > u_mean )&& (u_mean + v_mean > 256)){
			rect_clip = RED_CLIP_SHADE;
		}
	}

	if (rect_clip == RED_CLIP) {
		SETREG8(REG_ISP_REF_BIN, RED_CLIP_REFBIN_LOW);
	} else if (rect_clip == RED_CLIP_SHADE) {
		print_debug("Refbin register value do not change");
	} else {
		SETREG8(REG_ISP_REF_BIN, RED_CLIP_REFBIN_HIGH);
	}

}

static void ispv1_uv_stat_work_func(struct work_struct *work)
{
	u32 preview_width = this_ispdata->pic_attr[STATE_PREVIEW].out_width;
	u32 preview_height = this_ispdata->pic_attr[STATE_PREVIEW].out_height;
	u32 preview_size = preview_width * preview_height;
	camera_frame_buf *frame = this_ispdata->current_frame;

	if (down_trylock(&(this_ispdata->frame_sem)) != 0) {
		print_warn("enter %s, frame_sem already locked", __func__);
		goto error_out2;
	}

	if (NULL == frame) {
		print_error("params error, frame 0x%x", (unsigned int)frame);
		goto error_out1;
	}

	if ((0 == frame->phyaddr) || (NULL != frame->viraddr)) {
		print_error("params error, frame->phyaddr 0x%x, frame->viraddr:0x%x",
			(unsigned int)frame->phyaddr, (unsigned int)frame->viraddr);
		goto error_out1;
	}

	frame->viraddr = ioremap_nocache(frame->phyaddr + preview_size, preview_size / 2);
	if (NULL == frame->viraddr) {
		print_error("%s line %d error", __func__, __LINE__);
		goto error_out1;
	}
	deal_uv_data_from_preview((u8 *)frame->viraddr, preview_width, preview_height);
	iounmap(frame->viraddr);
	frame->viraddr = NULL;

error_out1:
	up(&(this_ispdata->frame_sem));

error_out2:
	return;
}

/*
 **************************************************************************
 * FunctionName: ispv1_uv_stat_init;
 * Description : NA
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int ispv1_uv_stat_init(void)
{
	/* init uv stat work queue. */
	uv_stat_work_queue = create_singlethread_workqueue("uv_stat_wq");
	if (!uv_stat_work_queue) {
		print_error("create workqueue is failed in %s function at line#%d\n", __func__, __LINE__);
		return -1;
	}
	INIT_WORK(&uv_stat_work, ispv1_uv_stat_work_func);
	return 0;
}

/*
 **************************************************************************
 * FunctionName: ispv1_uv_stat_excute;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
 void ispv1_uv_stat_excute(void)
{
	print_debug("enter %s", __func__);
	queue_work(uv_stat_work_queue, &uv_stat_work);
}

/*
 **************************************************************************
 * FunctionName: ispv1_uv_stat_exit;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void ispv1_uv_stat_exit(void)
{
	print_debug("enter %s", __func__);
	destroy_workqueue(uv_stat_work_queue);
}
