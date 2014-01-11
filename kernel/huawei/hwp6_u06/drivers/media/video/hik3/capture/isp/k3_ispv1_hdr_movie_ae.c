
#include <asm/io.h>
#include <linux/slab.h>
#include "k3_isp.h"
#include "k3_ispv1.h"
#include "cam_log.h"
#include <linux/workqueue.h>

#include "k3_ispv1_hdr_movie_ae.h"
extern k3_isp_data *this_ispdata;

unsigned short gain_time_table[GAIN_TABLE_LEN] = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
66, 67, 68, 69, 71, 72, 73, 74, 76, 77,
79, 80, 82, 84, 85, 87, 89, 91, 93, 95,
98, 100, 102, 105, 108, 111, 114, 117,
120, 124, 128, 132, 137, 141, 146, 152,
158, 164, 171,178, 186, 195, 205, 216,
228, 241, 256};


unsigned short gain_db_table[GAIN_TABLE_LEN] = {7, 21, 34, 45, 56, 65, 73, 81, 88, 95,
101, 107, 112, 117, 121, 125, 129, 133, 137, 140,
143, 146, 149, 152, 154, 157, 159, 161, 163, 165,
167, 169, 171, 173, 174, 176, 177, 179, 180, 182,
183, 184, 185, 187, 188, 189, 190, 191, 192, 193,
194, 195, 196, 197, 198, 199, 200, 201, 202, 203,
204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
234, 235, 236, 237, 238, 239, 240};

ispv1_hdr_movie_ae_ctrl_s * hdr_movie_ae_ctrl;
 /*
  *************************************************************************
  * FunctionName: 	:ispv1_set_digital_gain;
  * Description 	: set digital gain to sensor's digital registers
  * Input			: the struct of digital_gain_st saving digital value
  * Output			: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
void ispv1_set_digital_gain(digital_gain_st * digital_gain)
{
	camera_sensor * sensor = this_ispdata->sensor;
	sensor->set_digital_gain(digital_gain);
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
void ispv1_get_digital_gain(digital_gain_st * digital_gain)
{
	camera_sensor * sensor = this_ispdata->sensor;
	sensor->get_digital_gain(digital_gain);
}
 /*
  *************************************************************************
  * FunctionName: 	hdr_gain_time_to_db;
  * Description 	: the function  switch gain times to gain db according to gain_time table and gain db table
  * Input			: gain time value
  * Output		: gain db value;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
int hdr_gain_time_to_db(int gain_time)
{

	int gain_db = 0;
	int i;
	for(i=0; i<GAIN_TABLE_LEN; i++)
	{
		if(gain_time<=gain_time_table[i])
		{
			if(i==0)
				gain_db = gain_db_table[0];
			else
			{
				if(gain_time - gain_time_table[i-1] > gain_time_table[i] - gain_time)
					gain_db = gain_db_table[i];
				else
					gain_db = gain_db_table[i-1];
			}
			break;
		}
	}
	return gain_db;
}

 /*
  *************************************************************************
  * FunctionName: 	hdr_gain_db_to_time;
  * Description 	: the function  switch gain db to gain time according to gain_time table and gain db table.
  * Input			: gain db value
  * Output		: gain time value;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
int hdr_gain_db_to_time(int gain_db)
{
	int gain_time = 0;
	int i;
	 for(i=0; i<GAIN_TABLE_LEN; i++)
	{
		if(gain_db<=gain_db_table[i])
		{
			if(i==0)
			{
			    gain_time = gain_time_table[0];
			}
			else
			{
				if((gain_db_table[i] - gain_db)>(gain_db - gain_db_table[i-1]))
				    gain_time = gain_time_table[i-1];
				       else
				    gain_time = gain_time_table[i];
			}
	   	break;
		}
	}

	 return gain_time;
}

 /*
  *************************************************************************
  * FunctionName: 	ispv1_hdr_average_arith;
  * Description 	: the function  using average  arithmatic for getting difference from ae target.
				  formula as follow:
				  ((16*area(min) + 15*(area(sencond) + ....+ 1*area(max)))/(1+ 2 +...+16))
  * Input			: the stats lux matrix array of sensor ,
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
int ispv1_hdr_average_arith(int ae_target,unsigned int *iaverageValue)//atr on mode
{
    int i;
    int sum = 0, total = 0, mean = 0, value;

    int n_cut_blks = 0;
    short int fg_weight = 3;
    short int bg_weight = 2;
    int bg_fg_flag[16] = {0, 0, 0, 0,
                          0, 1, 1, 0,
                          0, 1, 1, 0,
                          1, 1, 1, 1
                         };
    unsigned int weight_sum = 0;
    short int cur_w;
    int mean_offset;
    int single_blk_offset = 20;
    int bright_thresh = 10 * single_blk_offset;
    int dark_thresh = 0;



    for(i = 0; i < HDR_STATS_ARRAY_SIZE; i++)
    {
        value = (int)(iaverageValue[i] >> 0);

        if(value > 1023)
        {
            value = 1023;
            n_cut_blks++;
        }
        if(1 == bg_fg_flag[i])
            cur_w = fg_weight;
        else
            cur_w = bg_weight;
        sum += cur_w * value;
        weight_sum += cur_w;
        total++;
    }

    mean = sum / weight_sum;
    mean_offset = n_cut_blks * single_blk_offset;
    if(mean_offset > bright_thresh)
        mean_offset = bright_thresh;
    else if(mean_offset < dark_thresh)
        mean_offset = dark_thresh;

   // mean = mean + mean_offset;
    if(mean < 0)
        mean = 0;
	if(abs((ae_target - mean)) <= HDR_AE_THRESHOLD)
	{
	    return 0;
	}
	else
	{
	    return (ae_target-mean);
	}
}

 /*
  *************************************************************************
  * FunctionName: 	ispv1_hdr_weight_arith;
  * Description 	: the function  using weight arithmatic for getting difference from ae target.
				  formula as follow:
				  ((16*area(min) + 15*(area(sencond) + ....+ 1*area(max)))/(1+ 2 +...+16))
  * Input			: the stats lux matrix array of sensor ,
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
int ispv1_hdr_weight_arith(int ae_target,unsigned int *iaverageValue)//atr off mode
{
    int i, j;
    int sum = 0, total = 0, mean = 0, value;

    int n_cut_blks = 0;

    unsigned int weight_sum = 0;
    short int cur_w;



    int brightness[16];
    int temp;

   for(i=0;i<HDR_STATS_ARRAY_SIZE;i++) brightness[i]=(int)(iaverageValue[i]) ;

  for(i=1;i<HDR_STATS_ARRAY_SIZE-1;i++)
	for(j=0;j<HDR_STATS_ARRAY_SIZE-i;j++)
		{
			if(brightness[j]>brightness[j+1])
			{
				temp=brightness[j];
				brightness[j]=brightness[j+1];
				 brightness[j+1]=temp;
			}
		}

    for(i = 0; i < HDR_STATS_ARRAY_SIZE; i++)
    {
        value = brightness[i];

        if(value > 1023)
        {
            value = 1023;
            n_cut_blks++;
        }
	cur_w=16-i;
        sum += cur_w * value;
        weight_sum += cur_w;

        total++;
    }

    mean = sum / weight_sum ;
    if(mean < 0)
        mean = 0;

	if(abs((ae_target - mean)) <= HDR_AE_THRESHOLD)
	{
	    return 0;
	}
	else
	{
	    return (ae_target-mean);
	}
}
  /*
  *************************************************************************
  * FunctionName: 	hdr_movie_adjust_process;
  * Description 	: the function  that adjust ae arithmatic and atr curve of  sensor
  * Input			: ae result : the struct include long shutter ,short shutter long gain and short gain.
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
void hdr_movie_adjust_process(hdr_ae_algo_result *ae_result)
{
	camera_sensor *sensor = this_ispdata->sensor;
	u16 N_gain = ae_result->N_gain;
	u16 N_gainBeforeAjust = ae_result->N_gainBeforeAjust;
	u16 atr_switch_gain = hdr_movie_ae_ctrl->hdr_atr_switch_gain;
	u16 gain_interval = 0;
	u32 wb_lmt = ae_result->N_wb_lmt;
	u32 ae_sat = ae_result->N_ae_sat;
	u16 rbratio =  ispv1_awb_dynamic_ccm_gain();
	if(rbratio < HDR_RBRATIO_THESHOLD)
	{
		atr_switch_gain = HDR_ATR_SWITCH_INDOOR;
		gain_interval = HDR_ATR_INTERVAL_INDOOR;
	}
	else
	{
		atr_switch_gain = HDR_ATR_SWITCH_OUTDOOR;
		gain_interval = HDR_ATR_INTERVAL_OUTDOOR;
	}
#ifdef HDR_MOVIE_DEBUG_MODE

	if(N_gainBeforeAjust > sensor->ae_arithmatic_switch_gain)
	{
		hdr_movie_ae_ctrl->ae_arith_method = HDR_AE_AVERAGE_MATH;
	}
	else
	{
		hdr_movie_ae_ctrl->ae_arith_method = HDR_AE_WEIGHT_MATH;
	}

	if(rbratio < HDR_RBRATIO_THESHOLD)
	{
		atr_switch_gain = sensor->gain_switch2;
		gain_interval = sensor->gain_interval2;
	}
	else
	{
		atr_switch_gain = sensor->gain_switch;
		gain_interval = sensor->gain_interval;
	}
#endif
	if(N_gainBeforeAjust >=(atr_switch_gain + gain_interval))
	{
	        if(ATR_ON == sensor->hdrInfo.atr_on)
	        {
	             ispv1_hdr_set_ATR_switch(0);
		     if(sensor->hdrInfo.atr_over_expo_on)
			sensor->over_exposure_adjust(0,NULL);
#ifdef HDR_MOVIE_DEBUG_MODE
			char *filename = "/data/hdr-ov-debug-on.txt";
			ispv1_load_isp_setting(filename);
			sensor->hdrInfo.ae_target = sensor->ae_target_low;
#else
			sensor->hdrInfo.ae_target = HDR_AE_TARGET_LOW;
#endif
	        }
		wb_lmt = 1023;
		ae_sat  = 960;
		ae_result->N_short_gain 	= ae_result->N_gain;
		ae_result->N_short_shuter	= ae_result->N_shuter;

	}else if(N_gainBeforeAjust < (atr_switch_gain - gain_interval))
	{
	        if( ATR_OFF == sensor->hdrInfo.atr_on)
	        {
	            ispv1_hdr_set_ATR_switch(1);
#ifdef HDR_MOVIE_DEBUG_MODE
		     char *filename = "/data/hdr-ov-debug-off.txt";
		     ispv1_load_isp_setting(filename);
		sensor->hdrInfo.ae_target = sensor->ae_target_high;
#else
		sensor->hdrInfo.ae_target = HDR_AE_TARGET_HIGH;
#endif
	        }
	}
	if(_IS_DEBUG_AE)
	{
		print_info("N_gainBeforeAjust =%d,N_gain: %d, atr_switch_gain: %d,gain_interval: %d atr_on = %d ae_arithmatic_switch_gain = %d",N_gainBeforeAjust, N_gain,atr_switch_gain, gain_interval,sensor->hdrInfo.atr_on,hdr_movie_ae_ctrl->ae_arith_method);
		print_info("ae_sat = %08x,wb_lmt = %08x", ae_sat,wb_lmt);
	}
	if(sensor->set_lmt_sat)
	{
		sensor->set_lmt_sat(wb_lmt,ae_sat);
	}

	ae_result->N_wb_lmt	= wb_lmt;
	ae_result->N_ae_sat	= ae_sat;
}

 /*
  *************************************************************************
  * FunctionName: 	ispv1_hdr_set_ae_status;
  * Description 	: the function  tell the status of ae to isp.
  * Input			: the status of ae ,include  STEADY_STATUS ,ADJUST_STATUS,
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
void ispv1_hdr_set_ae_status(u8 status)
{

	 ispv1_write_isp_reg(REG_ISP_AE_STATUS, status&0x1);
}

 /*
  *************************************************************************
  * FunctionName: ispv1_isp_ae_switch;
  * Description 	: the function  control that the shutter and gain is writed by sensor or isp,if 1 ,isp write sensor, 0 driver ae will write.
  * Input			: 1 ,isp write sensor shutter and gain; 0 driver ae will write shutter and gain.
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************

  */
void ispv1_hdr_isp_ae_switch(u8 on)
{
	print_debug("enter %s", __func__);

	ispv1_write_isp_reg(REG_ISP_AE_SWITCH, on&0x1);
}

 /*
  *************************************************************************
  * FunctionName: ispv1_isp_ae_ctrl;
  * Description : the function that turn on or off the switch of isp, if value is 1 , the ae arithmatic of isp is used, otherwise ae of dirver is used.
  * Input			: 1 the isp's ae arithmatic . 0 . the driver of ae.
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
void ispv1_hdr_isp_ae_ctrl(u8 on)
{
      print_debug("enter %s", __func__);

      ispv1_write_isp_reg(REG_ISP_AE_CTRL, on&0x1);
}

 /*
  *************************************************************************
  * FunctionName: ispv1_hdr_get_lux_matrix;
  * Description : the function that get lux matrix from sensor.
  * Input			: the pointer of lux_stat_matrix_tbl is used for saving lux matrix value from sensor.
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
void ispv1_hdr_get_lux_matrix(lux_stat_matrix_tbl * lux_matrix_bbl)
{
   camera_sensor *sensor = this_ispdata->sensor;
   assert(lux_matrix_bbl);
   assert(sensor);
   if(sensor->get_hdr_lux_matrix)
   {
      sensor->get_hdr_lux_matrix(sensor->hdr_regs_y,lux_matrix_bbl);
   }

}

 /*
  *************************************************************************
  * FunctionName: ispv1_hdr_set_analog_gain_exposure;
  * Description 	: the function that set long shutter ,short shutter , long gain and short gain to sensor and isp
  * Input			: the struct of hdr_ae_algo_result include 1.long exposure. 2 short exposure. 3 long gain 4 short gain
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
void ispv1_hdr_set_analog_gain_exposure(hdr_ae_algo_result * ae_result)
{
	camera_sensor *sensor = this_ispdata->sensor;
	u16 isp_long_gain_db = 0;
	u16 isp_short_gain_db = 0;
	u8  digital_gain_h = 0;
	u8  digital_gain_l = 0;
	u16 long_gain_db = 0;
	u16 short_gain_db = 0;
	u32 expo_long = ae_result->N_shuter;
	u32 expo_short = ae_result->N_short_shuter;
	u16 global_gain = ae_result->N_gain;
	u16 gain_short = ae_result->N_short_gain;
	u16 N_gainBeforeAjust = ae_result->N_gainBeforeAjust;

	digital_gain_st digital_gain;
	if(_IS_DEBUG_AE)
	{
		print_info("N_gainBeforeAjust = %d,global_gain = %d ,expo_long = %d,gain_short = %d,expo_short = %d", N_gainBeforeAjust,global_gain,expo_long,gain_short,expo_short);
	}
	if(sensor->set_hdr_exposure_gain)
	{
		digital_gain_h = ((ae_result->N_digital >> 8)&0xff);
		digital_gain_l = (ae_result->N_digital &0xff);
		digital_gain.digital_gain_h = digital_gain_h;
		digital_gain.digital_gain_l = digital_gain_l;
		ispv1_set_digital_gain(&digital_gain);
		long_gain_db = hdr_gain_time_to_db(global_gain);
		short_gain_db = hdr_gain_time_to_db(gain_short);
		sensor->set_hdr_exposure_gain(expo_long, long_gain_db, expo_short, short_gain_db);
	}

	if(N_gainBeforeAjust != 0)
	isp_long_gain_db = 256 - (256 * 16) / N_gainBeforeAjust;
	if(gain_short != 0)
	 isp_short_gain_db = 256 - (256 * 16) / gain_short;

	 ispv1_write_isp_reg(REG_ISP_LONG_TIME_H, (expo_long >> 8)&0xff);
	 ispv1_write_isp_reg(REG_ISP_LONG_TIME_L, expo_long &0xff);
	 ispv1_write_isp_reg(REG_ISP_LONG_TIME_DS_H, (expo_long >> 8)&0xff);
	 ispv1_write_isp_reg(REG_ISP_LONG_TIME_DS_L, expo_long &0xff);
        ispv1_write_isp_reg(REG_ISP_LONG_GAIN, isp_long_gain_db&0xff);

	 ispv1_write_isp_reg(REG_ISP_SHORT_TIME_WHT_H, (expo_short >> 8)&0xff);
	 ispv1_write_isp_reg(REG_ISP_SHORT_TIME_WHT_L, expo_short &0xff);
	 ispv1_write_isp_reg(REG_ISP_SHORT_TIME_WHT_DS_H, (expo_short >> 8)&0xff);
	 ispv1_write_isp_reg(REG_ISP_SHORT_TIME_WHT_DS_L, expo_short &0xff);
        ispv1_write_isp_reg(REG_ISP_SHORT_GAIN, isp_long_gain_db&0xff);
}

 /*
  *************************************************************************
  * FunctionName: ispv1_hdr_set_ATR_switch
  * Description : the function that turn on or off atr functio for hdr movie
  * Input			: on 1 on, 0 off
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
void ispv1_hdr_set_ATR_switch(int on)
{
	camera_sensor *sensor = this_ispdata->sensor;
	print_info("enter %s ,switch = 0x%x", __func__,on);
       assert(sensor);
	if(sensor->set_ATR_switch)
	{
	   sensor->set_ATR_switch(on);
	}
}
 /*
  *************************************************************************
  * FunctionName: ispv1_hdr_ae_init;
  * Description : the function that init parameter for hdr movie
  * Input			: NA
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
int  ispv1_hdr_ae_init(void)
{
	hdr_movie_ae_ctrl = kmalloc(sizeof(ispv1_hdr_movie_ae_ctrl_s), GFP_KERNEL);

	if (!hdr_movie_ae_ctrl) {
		print_error("malloc is failed in %s function at line#%d\n", __func__, __LINE__);
		return -ENOMEM;
	}
	memset(hdr_movie_ae_ctrl, 0, sizeof(ispv1_hdr_movie_ae_ctrl_s));
	ispv1_hdr_parameter_init();
	return 0;
}
 /*
  *************************************************************************
  * FunctionName: ispv1_hdr_ae_exit;
  * Description : the function that is exit for hdr movie free resource.
  * Input			: NA
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
int ispv1_hdr_ae_exit(void)
{
       print_debug("enter %s", __func__);
	if(NULL != hdr_movie_ae_ctrl)
	{
		kfree(hdr_movie_ae_ctrl);
		hdr_movie_ae_ctrl = NULL;
	}
	return 0;
}
 void hdr_movie_stats_param_init(void)
 {
	hdr_movie_ae_ctrl->stats_min			= 8192;
	hdr_movie_ae_ctrl->stats_ave			= 0;
	hdr_movie_ae_ctrl->stats_max		= 0;
	hdr_movie_ae_ctrl->stats_diff		       = 0;
 }
 /*
  *************************************************************************
  * FunctionName: ispv1_hdr_parameter_init;
  * Description : the function that init struct 's number parameter.
  * Input			: NA
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
void ispv1_hdr_parameter_init()
{
	camera_sensor * sensor = this_ispdata->sensor;
	print_debug("enter %s",__func__);
	assert(this_ispdata);
	assert(sensor);
	hdr_movie_ae_ctrl->hdr_ae_ratio 		= sensor->ae_hdr_ratio;

	hdr_movie_ae_ctrl->ae_arith_method 	= HDR_AE_AVERAGE_MATH;

	hdr_movie_ae_ctrl->vts 				= sensor->frmsize_list[sensor->preview_frmsize_index].vts - 4;

	sensor->hdrInfo.atr_on     				= ATR_ON;

	hdr_movie_stats_param_init();
}
 /*
  *************************************************************************
  * FunctionName: ispv1_hdr_banding_step_change;
  * Description 	: the function that set banding step for hdr movie.
  * Input			: banding parameter{	CAMERA_ANTI_BANDING_50Hz,CAMERA_ANTI_BANDING_60Hz,}
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
u32 ispv1_hdr_banding_step_change(camera_anti_banding banding)
{
	u32 	hdr_banding_step = 0;
	camera_sensor * sensor = this_ispdata->sensor;
	if(_IS_DEBUG_AE)
	print_info("%s, anti-banding:%#x", __func__, banding);
	switch (banding) {
	case CAMERA_ANTI_BANDING_50Hz:
		hdr_banding_step	= sensor->frmsize_list[sensor->preview_frmsize_index].banding_step_50hz;
		break;
	case CAMERA_ANTI_BANDING_60Hz:
		hdr_banding_step  = sensor->frmsize_list[sensor->preview_frmsize_index].banding_step_60hz;
		break;
	default:
		hdr_banding_step	= sensor->frmsize_list[sensor->preview_frmsize_index].banding_step_50hz;
		print_error("%s, Unsupport anti-banding:%#x", __func__, banding);
		break;
	}
	return hdr_banding_step;
}

 /*
  *************************************************************************
  * FunctionName   : hdr_movie_over_exposure_process;
  * Description 	: the function that adjust min stats, ave stats,max stats for atr corve for overexposure..
  * Input			: NA
  * Output		: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */
static void hdr_movie_over_exposure_process(void)
{
	camera_sensor *sensor = this_ispdata->sensor;
	int frm_min = 0;
	int frm_max = 0;
	int frm_min_h = 0;
	int frm_min_l  = 0;
	int frm_ave_h = 0;
	int frm_ave_l  = 0;
	int frm_max_h = 0;
	int frm_max_l  = 0;

	int stats_diff_thl =  0;
	int stats_max_thl= 0;
	int stats_max = hdr_movie_ae_ctrl->stats_max;
	int stats_min	 = hdr_movie_ae_ctrl->stats_min;
	int stats_ave	 = hdr_movie_ae_ctrl->stats_ave;
	int stats_diff = hdr_movie_ae_ctrl->stats_diff;
	stats_hdr_frm stats_frm;
	frm_min 				= HDR_MOVIE_FRM_MIN_IN;
	frm_max 			= 959;
#ifdef HDR_MOVIE_DEBUG_MODE
	stats_diff_thl			= sensor->stats_diff_threshold;
	stats_max_thl			= sensor->stats_max_threshold;
#else
	stats_diff_thl			= HDR_MOVIE_STATS_DIFF_TH;
	stats_max_thl			= HDR_MOVIE_OVER_EXP_TH;
#endif

	if(_IS_DEBUG_AE)
	print_info("stats_diff = %d,stats_max= %d,stats_ave=%d,stats_min=%d,stats_diff_thl=%d,stats_max_thl=%d",stats_diff,stats_max,stats_ave,stats_min,stats_diff_thl,stats_max_thl);
	if(stats_diff < stats_diff_thl)
	{
		if(stats_max < stats_max_thl)
		{
			if(frm_min > stats_min)
				frm_min = stats_min;
				frm_min_h = frm_min / 256;
				frm_min_l = frm_min - frm_min_h * 256;

				frm_ave_h = stats_ave / 256;
				frm_ave_l = stats_ave - frm_ave_h * 256;

			if (frm_max < stats_max)
				frm_max = stats_max;
			frm_max_h = frm_max / 256;
		       frm_max_l = frm_max - frm_max_h * 256;

			stats_frm.frm_ave_h = frm_ave_h;
			stats_frm.frm_ave_l = frm_ave_l;
			stats_frm.frm_min_h = frm_min_h;
			stats_frm.frm_min_l = frm_min_l;
			stats_frm.frm_max_h = frm_max_h;
			stats_frm.frm_max_l = frm_max_l;
			sensor->over_exposure_adjust(1,&stats_frm);
	    	}
	    	else
	    	{
	    		if(sensor->hdrInfo.atr_over_expo_on)
				sensor->over_exposure_adjust(0,NULL);
		}
	}
	else
	{
		if(sensor->hdrInfo.atr_over_expo_on)
			sensor->over_exposure_adjust(0,NULL);
	}
}

int ispv1_awb_dynamic_ccm_gain()
{
	camera_sensor *sensor = this_ispdata->sensor;
	awb_gain_t  awb_gain;
	GETREG16(REG_ISP_AWB_ORI_GAIN_B, awb_gain.b_gain);
	GETREG16(REG_ISP_AWB_ORI_GAIN_R, awb_gain.r_gain);


	u32 current_rbratio = 0x100;


	if ((awb_gain.b_gain != 0) && (awb_gain.r_gain != 0)) {
		current_rbratio = 0x8000 / (awb_gain.b_gain * 0x100 / awb_gain.r_gain);
	}



	return current_rbratio;
}
/****************************************************************************
* FunctionName: ispv1_set_sensor_awb_gain;
* Description : NA;
* Input       : R,GR,GB,B gain from isp;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static void ispv1_set_sensor_awb_gain(void)
{
	u16 b_gain, gb_gain, gr_gain, r_gain;
       assert(this_ispdata->sensor);
	if (this_ispdata->sensor->set_awb_gain)
	{
		GETREG16(MANUAL_AWB_GAIN_B, b_gain);
		GETREG16(MANUAL_AWB_GAIN_GB, gb_gain);
		GETREG16(MANUAL_AWB_GAIN_GR, gr_gain);
		GETREG16(MANUAL_AWB_GAIN_R, r_gain);

		b_gain = 2 * b_gain;
		gb_gain = 2 * gb_gain;
		gr_gain = 2 * gr_gain;
		r_gain = 2 * r_gain;

		this_ispdata->sensor->set_awb_gain(b_gain, gb_gain, gr_gain, r_gain);
	}
}

int  ispv1_get_hdr_movie_ae_init_param(hdr_ae_constant_param * hdr_ae_param)
{
	camera_sensor *sensor = this_ispdata->sensor;
	u32 C_shuter = 0;
	u16 C_gain = 0;
	u16 C_gain_db = 0;
	C_shuter = sensor->get_exposure();
	C_gain_db = sensor->get_gain();
	sensor->get_hdr_movie_ae_param(hdr_ae_param);
	C_gain = hdr_gain_db_to_time(C_gain_db);// gain_db switch gain times
	#ifdef HDR_MOVIE_DEBUG_MODE
	char *hdrfilename = "/data/hdr_sensor_debug.txt";
	ispv1_load_sensor_setting(sensor,hdrfilename);
	hdr_ae_param->hdr_ae_target  = sensor->ae_target_high;
	sensor->hdrInfo.ae_target = sensor->ae_target_high;
	#else
	hdr_ae_param->hdr_ae_target  = HDR_AE_TARGET_HIGH;
	sensor->hdrInfo.ae_target = HDR_AE_TARGET_HIGH;
	#endif
	hdr_ae_param->default_gain 		= C_gain;
	hdr_ae_param->default_shutter	= C_shuter;
	return 0;
}
/****************************************************************************
* FunctionName	: ispv1_get_hdr_movie_ae_running_param;
* Description 		: Ae in the HAL get ae mean of stats lux and banding step by this interface from driver.;
* Input       		: the space of struct hdr_ae_volatile_param;
* Output      		: the value of ae mean and banding step;
* ReturnValue 	: NA;
* Other       		: NA;
***************************************************************************/
int ispv1_get_hdr_movie_ae_running_param(hdr_ae_volatile_param * output_param)
{

	camera_sensor *sensor = this_ispdata->sensor;
	lux_stat_matrix_tbl lux_matrix;
	int i = 0;
	int aemean = 0;
	int banding_step = 0;
	int ae_target = 0;
	unsigned int tmp[HDR_STATS_ARRAY_SIZE] = {0};
	u32 stats_sum = 0;
	if(this_ispdata->hdr_switch_on == false)
	{
		output_param->ae_mean = 1;
		output_param->banding_step = 1;
		output_param->hdr_ae_target = 1;
		output_param->sensor_vts = 1;
		return 0;
	}
	ae_target = sensor->hdrInfo.ae_target;
	ispv1_hdr_get_lux_matrix(&lux_matrix);
	hdr_movie_stats_param_init();
	for (i=0; i<HDR_STATS_ARRAY_SIZE; i++)
	{
		if(_IS_DEBUG_AE)
		  	print_info("i = %d,value = %d",i,lux_matrix.matrix_table[i]);
		tmp[i] = lux_matrix.matrix_table[i];
		stats_sum +=tmp[i];
		if(hdr_movie_ae_ctrl->stats_min > tmp[i])
		{
			hdr_movie_ae_ctrl->stats_min = tmp[i];
		}

		if(hdr_movie_ae_ctrl->stats_max < tmp[i])
		{
			hdr_movie_ae_ctrl->stats_max = tmp[i];
		}
	}
	hdr_movie_ae_ctrl->stats_ave = (stats_sum/HDR_STATS_ARRAY_SIZE);
	hdr_movie_ae_ctrl->stats_diff	= hdr_movie_ae_ctrl->stats_max- hdr_movie_ae_ctrl->stats_ave;

	if(HDR_AE_AVERAGE_MATH == hdr_movie_ae_ctrl->ae_arith_method)
	{
		aemean = ispv1_hdr_average_arith(ae_target,tmp);
	}
	else if(HDR_AE_WEIGHT_MATH == hdr_movie_ae_ctrl->ae_arith_method)
	{
		aemean = ispv1_hdr_weight_arith(ae_target,tmp);
	}

	if (aemean == 0)
	{
	    if(_IS_DEBUG_AE)
	        print_info("******stable!******\n");
		 ispv1_hdr_set_ae_status(1);
	}
	else
	{

	    if(_IS_DEBUG_AE)
	        print_info("******Not stable!******\n");
	         ispv1_hdr_set_ae_status(0);
	}

	banding_step = ispv1_hdr_banding_step_change(this_ispdata->anti_banding);

	if(NULL != output_param)
	{
		output_param->hdr_ae_target = sensor->hdrInfo.ae_target;
		output_param->ae_mean 		= aemean;
		output_param->banding_step 	= banding_step;
		output_param->sensor_vts	= sensor->frmsize_list[sensor->preview_frmsize_index].vts;
	}
	if(_IS_DEBUG_AE)
	{
		print_info("ae_mean: %d, ae_target: %d,C_mean: %d,banding_step = %d,sensor_vts = %d\n", aemean,ae_target, ae_target - aemean,banding_step
			,output_param->sensor_vts);
	}
#ifdef HDR_MOVIE_DEBUG_MODE
	sensor->hdrInfo.stats_diff  	= hdr_movie_ae_ctrl->stats_diff	;
	sensor->hdrInfo.stats_max  	= hdr_movie_ae_ctrl->stats_max;
#endif

	return 0;
}

  /**************************************************************************
  * FunctionName: ispv1_set_isp_default_reg;
  * Description : the function that turn off some registers for function that include
  * digital gain , hdr movie and so on.
  * Input		  : NA
  * Output		  : NA;
  * ReturnValue   : NA;
  * Other         :
  **************************************************************************
  */
void ispv1_set_isp_default_reg()
{
    print_info("enter %s", __func__);
	ispv1_write_isp_reg(0x1d9c6, 0x00);
	ispv1_write_isp_reg(0x1d9b0, 0x02);
	ispv1_write_isp_reg(0x1d9b1, 0x00);
	ispv1_write_isp_reg(0x1d9af, 0x00);
	ispv1_write_isp_reg(0x1d9c4, 0x00);
	ispv1_write_isp_reg(0x1d9c5, 0x00);

	ispv1_write_isp_reg(0x1d99f, 0x00);

}

/****************************************************************************
* FunctionName	: ispv1_set_hdr_movie_shutter_gain_to_sensor;
* Description 		: the function that set long and short gain and shutter from HAL and then set it to sensor.
				: otherwise adjust hdr cuver and points for over exposure and awb gain.
* Input       		: the value of long shutter ,long gain ,short shutter short gain for sensor.
* Output      		: NA
* ReturnValue 	: NA;
* Other       		: NA;
***************************************************************************/

int ispv1_set_hdr_movie_shutter_gain_to_sensor(hdr_ae_algo_result * ae_result)
{
	camera_sensor * sensor = this_ispdata->sensor;

	if(this_ispdata->hdr_switch_on == false)
		return 0;
	hdr_movie_adjust_process(ae_result);
#ifdef HDR_MOVIE_DEBUG_MODE
	sensor->hdrInfo.gain  		= ae_result->N_gain;
	sensor->hdrInfo.short_gain	= ae_result->N_short_gain;
	sensor->hdrInfo.expo		= ae_result->N_shuter;
	sensor->hdrInfo.short_expo		= ae_result->N_short_shuter;
	sensor->hdrInfo.gainBeforeAjust	= ae_result->N_gainBeforeAjust;
	sensor->hdrInfo.wb_lmt  		= ae_result->N_wb_lmt	;
	sensor->hdrInfo.ae_sat  		= ae_result->N_ae_sat;
	sensor->hdrInfo.N_digital_h  	=  ((ae_result->N_digital >> 8)&0xff);
	sensor->hdrInfo.N_digital_l    	= (ae_result->N_digital &0xff);
#endif

	ispv1_hdr_set_analog_gain_exposure(ae_result);

	if(sensor->hdrInfo.atr_on)
	{
		hdr_movie_over_exposure_process();
	}
        ispv1_set_sensor_awb_gain();
	return 0;
}
