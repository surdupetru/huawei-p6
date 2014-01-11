#ifndef __K3_ISPV1_HDR_MOVIE_AE_H__
#define __K3_ISPV1_HDR_MOVIE_AE_H__


#define _IS_DEBUG_AE 0

#define HDR_STATS_ARRAY_SIZE     				(16)
#define HDR_AE_TARGET_LOW						(40*4) // 8bit *4 = 10bit
#define HDR_AE_TARGET_HIGH					(56*4) // 8bit *4 = 10bit
#define HDR_AE_THRESHOLD						(10)
#define REG_ISP_AE_SWITCH                       			0x1d9a0
#define REG_ISP_LONG_TIME_H                   			0x1d9a1
#define REG_ISP_LONG_TIME_L                    		0x1d9a2
#define REG_ISP_LONG_TIME_DS_H             			0x1d9a3
#define REG_ISP_LONG_TIME_DS_L              		0x1d9a4
#define REG_ISP_LONG_GAIN                        		0x1d9a5
#define REG_ISP_SHORT_TIME_WHT_H         		0x1d9a6
#define REG_ISP_SHORT_TIME_WHT_L          		0x1d9a7
#define REG_ISP_SHORT_TIME_WHT_DS_H   		0x1d9a8
#define REG_ISP_SHORT_TIME_WHT_DS_L    		0x1d9a9
#define REG_ISP_SHORT_GAIN                       		0x1d9aa

#define REG_ISP_AE_HDR_RATIO                   		0x1d9ab

#define REG_ISP_AE_STATUS                         		0x1d9ac

#define REG_ISP_AE_CTRL                             		0x1d9ad

#define MAX_EXPOSURE_TABLE_SIZE   			 	500

#define Q16                       						 	16
#define AE_ADJUST_STEP            				 	13422 //0.0128*Q16


#define DEFAULT_OUT_NOISE 			 			0xA0
#define DEFAULT_OUT_MID              	 			0x800
#define DEFAULT_OUT_SAT      		 			0xFFF


#define DEFAULT_OFFSETD                    			0x0003
#define DEFAULT_OFFSETE                   				0x0004
#define DEFAULT_OFFSETF                    				0x0005

#define DEFAULT_SAT_LMT_8                			0x1DF8

#define HDR_ATR_SWITCH_GAIN_DEFAULT			100

#define GAIN_TABLE_LEN 							97

#define HDR_MOVIE_STATS_DIFF_TH 				200
#define HDR_MOVIE_OVER_EXP_TH 				400
#define HDR_MOVIE_FRM_MIN_IN 					60

#define HDR_RBRATIO_THESHOLD					130
#define HDR_ATR_SWITCH_OUTDOOR				250
#define HDR_ATR_INTERVAL_OUTDOOR				40
#define HDR_ATR_SWITCH_INDOOR				0
#define HDR_ATR_INTERVAL_INDOOR				0
typedef struct _hdr_ae_volatile_param
{
	int ae_mean;
	int sensor_vts;
	unsigned short  banding_step;
	unsigned short  hdr_ae_target;
}hdr_ae_volatile_param;

typedef enum{
	HDR_AE_AVERAGE_MATH = 0,
	HDR_AE_WEIGHT_MATH,
}ae_arithmatic_method;
typedef enum{
	HDR_MOVIE_NOT_SUPPORT = 0,
	HDR_MOVIE_SUPPORT = 1,
}hdr_movie_func;
typedef enum {
	HDR_MOVIE_OFF  = 0,
	HDR_MOVIE_ON,

} hdr_movie_switch;


typedef enum _hdr_ae_status{
  STEADY_STATUS = 0,
  ADJUST_STATUS,
} hdr_ae_status;

typedef enum _hdr_movie_ATR_status{
  ATR_OFF = 0,
  ATR_ON,
}hdr_movie_ATR_status;

typedef enum _camera_preview_mode
{
   HDR_MOVIE_PREVIEW_MODE = 0,
   NON_HDR_PREVIEW_MODE,
}camera_preview_mode;

typedef struct _ispv1_hdr_movie_ae_ctrl_s
{

	int 				hdr_ae_ratio ;

	int				hdr_atr_switch_gain;
	int 				ae_arith_method;
	u32 				banding_step;
	int 				vts;

	int				stats_min;
	int 				stats_ave;
	int 				stats_max;
	int				stats_diff;
}ispv1_hdr_movie_ae_ctrl_s;

void ispv1_hdr_get_lux_matrix(lux_stat_matrix_tbl * lux_matrix_tbl);

void ispv1_hdr_set_analog_gain_exposure(hdr_ae_algo_result * ae_result);

void ispv1_hdr_set_ae_status(u8 status);

void ispv1_hdr_isp_ae_switch(u8 on);


void ispv1_hdr_isp_ae_ctrl(u8 on);


void ispv1_hdr_set_ATR_switch(int on);

int 	ispv1_hdr_ae_init(void);

int   	ispv1_hdr_ae_exit(void);

void ispv1_hdr_parameter_init(void);

u32 	ispv1_hdr_banding_step_change(camera_anti_banding banding);

int ispv1_get_hdr_movie_ae_init_param(hdr_ae_constant_param * hdr_ae_param);

int ispv1_get_hdr_movie_ae_running_param(hdr_ae_volatile_param * output_param);

 int 	ispv1_set_hdr_movie_shutter_gain_to_sensor(hdr_ae_algo_result * ae_result);

int  ispv1_awb_dynamic_ccm_gain();

void ispv1_set_isp_default_reg();
#endif /*__K3_ISPV1_HDR_MOVIE_AE_H__ */
