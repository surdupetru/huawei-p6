/* Copyright (c) 2008-2011, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *	 * Redistributions of source code must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *	 * Redistributions in binary form must reproduce the above
 *	   copyright notice, this list of conditions and the following
 *	   disclaimer in the documentation and/or other materials provided
 *	   with the distribution.
 *	 * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *	   contributors may be used to endorse or promote products derived
 *	   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef K3_FB_H
#define K3_FB_H

#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/spinlock.h>
#include <mach/platform.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <mach/hisi_mem.h>

#include "k3_fb_def.h"
#include "k3_fb_panel.h"
#include "edc_overlay.h"
#ifdef CONFIG_OVERLAY_COMPOSE
#  ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
#include <linux/pm_qos_params.h>
#  endif
#endif


#define K3_FB_OVERLAY_USE_BUF 1


/* G2D clk rate */
#define G2D_CLK_60MHz	(60 * 1000 * 1000)
#define G2D_CLK_120MHz	(120 * 1000 * 1000)
#define G2D_CLK_240MHz	(240 * 1000 * 1000)
#define G2D_CLK_360MHz	(360 * 1000 * 1000)
#define G2D_CLK_480MHz	(480 * 1000 * 1000)

#ifdef CONFIG_LCD_TOSHIBA_MDY90
/* temprature for FRC: using fake vsync */
#define EVT_TEM_LEVEL_MAX	4

#define EVT_TEM_NORM		50
#define EVT_TEM_ABOVE_NORM	55
#define EVT_TEM_HIGH		60
#define EVT_TEM_THRD		67

#define FAKE_FPS_TEM_NORM_FPS		60
#define FAKE_FPS_TEM_ABOVE_NORM_FPS	45
#define FAKE_FPS_TEM_HIGH			36
#define FAKE_FPS_TEM_THRD			30

#define FAKE_FPS_SPECIAL_GAME		45

/* FRC: using hw vsync */
#define K3_FB_FRC_NORMAL_FPS	60
#define K3_FB_FRC_IDLE_FPS		45
#define K3_FB_FRC_VIDEO_FPS		45
#define K3_FB_FRC_WEBKIT_FPS	45
#define K3_FB_FRC_BENCHMARK_FPS	60
#define K3_FB_FRC_GAME_FPS		45
#define K3_FB_FRC_SPECIAL_GAME_FPS	60
#define K3_FB_FRC_GAME_30_FPS           30

/* SBL */
#define SBL_BKL_STEP	5
#define SBL_REDUCE_VALUE(x)	((x) * 80 / 100)
#elif defined(CONFIG_LCD_CMI_OTM1280A)
/* temprature for FRC: using fake vsync */
#define EVT_TEM_LEVEL_MAX	4

#define EVT_TEM_NORM		50
#define EVT_TEM_ABOVE_NORM	55
#define EVT_TEM_HIGH		60
#define EVT_TEM_THRD		67

#define FAKE_FPS_TEM_NORM_FPS		60
#define FAKE_FPS_TEM_ABOVE_NORM_FPS	45
#define FAKE_FPS_TEM_HIGH			36
#define FAKE_FPS_TEM_THRD			30

#define FAKE_FPS_SPECIAL_GAME		45

/* FRC: using hw vsync */
#define K3_FB_FRC_NORMAL_FPS	60
#define K3_FB_FRC_IDLE_FPS	45
#define K3_FB_FRC_VIDEO_FPS	45
#define K3_FB_FRC_WEBKIT_FPS	45
#define K3_FB_FRC_BENCHMARK_FPS	60
#define K3_FB_FRC_GAME_FPS	45
#define K3_FB_FRC_SPECIAL_GAME_FPS  60

/* SBL */
#define SBL_BKL_STEP	5
#define SBL_REDUCE_VALUE(x)	((x) * 70 / 100)
#else
/* temprature for FRC: using fake vsync */
#define EVT_TEM_LEVEL_MAX	4

#define EVT_TEM_NORM		50
#define EVT_TEM_ABOVE_NORM	55
#define EVT_TEM_HIGH		60
#define EVT_TEM_THRD		67

#define FAKE_FPS_TEM_NORM_FPS		60
#define FAKE_FPS_TEM_ABOVE_NORM_FPS	45
#define FAKE_FPS_TEM_HIGH			36
#define FAKE_FPS_TEM_THRD			30

#define FAKE_FPS_SPECIAL_GAME		45

/* FRC: using hw vsync */
#define K3_FB_FRC_NORMAL_FPS	60
#define K3_FB_FRC_IDLE_FPS		30
#define K3_FB_FRC_VIDEO_FPS		60
#define K3_FB_FRC_WEBKIT_FPS	45
#define K3_FB_FRC_BENCHMARK_FPS	67
#define K3_FB_FRC_GAME_FPS		45
#define K3_FB_FRC_SPECIAL_GAME_FPS  60
#define K3_FB_FRC_GAME_30_FPS           30


/* SBL */
#define SBL_BKL_STEP	5
#define SBL_REDUCE_VALUE(x)     ((x) * 80 / 100)
#endif

/* FRC threshold */
#define K3_FB_FRC_THRESHOLD	6

/* ESD threshold */
#define K3_FB_ESD_THRESHOLD	45

/* display resolution limited */
#define K3_FB_MIN_WIDTH	32
#define K3_FB_MIN_HEIGHT	32
#define K3_FB_MAX_WIDTH	1200
#define K3_FB_MAX_HEIGHT	1920

/* frame buffer physical addr */
#define K3_FB_PA	HISI_FRAME_BUFFER_BASE
#define K3_NUM_FRAMEBUFFERS	4

/* EDC */
#define EDC_CH_SECU_LINE	11
#define EDC_CLK_RATE_GET(x)	((x) * 12 / 10)

#define K3FB_DEFAULT_BGR_FORMAT	EDC_RGB

/* for MIPI Command LCD */
#define CLK_SWITCH	0

/* for Vsync*/
#define VSYNC_TIMEOUT_MSEC 100

/*
** stride 64 bytes odd align
** vivante gpu: stride must be 16pixels align
*/
#define CONFIG_FB_STRIDE_64BYTES_ODD_ALIGN	1

#if defined(CONFIG_OVERLAY_COMPOSE)
#define MAX_EDC_CHANNEL (3)

enum {
    OVC_NONE,
    OVC_PARTIAL,
    OVC_FULL,
};

/* channel change flow */
/* #define EDC_CH_CHG_SUPPORT */
#endif


enum {
	LCD_LANDSCAPE = 0,
	LCD_PORTRAIT,
};

/* set backlight by pwm or mipi ... */
enum {
	BL_SET_BY_NONE = 0,
	BL_SET_BY_PWM = 0x1,
	BL_SET_BY_MIPI = 0x2,
};

enum {
	IMG_PIXEL_FORMAT_RGB_565 = 1,
	IMG_PIXEL_FORMAT_RGBA_5551,
	IMG_PIXEL_FORMAT_RGBX_5551,
	IMG_PIXEL_FORMAT_RGBA_8888,
	IMG_PIXEL_FORMAT_RGBX_8888,
};

enum {
	FB_64BYTES_ODD_ALIGN_NONE = 0,
	FB_64BYTES_ODD_ALIGN_UI,
	FB_64BYTES_ODD_ALIGN_VIDEO,
};

enum {
	FB_LDI_INT_TYPE_NONE = 0x0,
	FB_LDI_INT_TYPE_FRC = 0x1,
	FB_LDI_INT_TYPE_ESD = 0x2,
};

/* frc state definition */
enum {
	K3_FB_FRC_NONE_PLAYING = 0x0,
	K3_FB_FRC_VIDEO_PLAYING = 0x2,
	K3_FB_FRC_GAME_PLAYING = 0x4,
	K3_FB_FRC_VIDEO_IN_GAME = 0x6,
	K3_FB_FRC_BENCHMARK_PLAYING = 0x8,
	K3_FB_FRC_WEBKIT_PLAYING =0x10,
	K3_FB_FRC_SPECIAL_GAME_PLAYING = 0x20,
	K3_FB_FRC_IDLE_PLAYING = 0x40,
    K3_FB_FRC_GAME_30_PLAYING = 0x80,
};


/**
 * struct k3fb_vsync - vsync information+
 * @wait:              a queue for processes waiting for vsync
 * @timestamp:         the time of the last vsync interrupt
 * @active:            whether userspace is requesting vsync uevents
 * @thread:            uevent-generating thread
 */
struct k3fb_vsync {
	wait_queue_head_t wait;
	ktime_t timestamp;
	bool active;
	int te_refcount;
	spinlock_t irq_lock;
	struct task_struct *thread;
};

struct k3_fb_data_type {
	u32 index;
	u32 ref_cnt;
	u32 bl_level;
	u32 bl_level_sbl;
	u32 bl_level_cmd;
	u32 fb_imgType;
	u32 fb_bgrFmt;
	u32 edc_base;
	u32 pwm_base;
	u32 edc_irq;
	u32 ldi_irq;
	s32 ldi_int_type;

	char edc_irq_name[64];
	char ldi_irq_name[64];

	panel_id_type panel;
	struct k3_panel_info panel_info;
	bool panel_power_on;

	struct fb_info *fbi;
	u32 graphic_ch;

	wait_queue_head_t frame_wq;
	s32 update_frame;
	struct work_struct frame_end_work;
	struct workqueue_struct *frame_end_wq;

	bool cmd_mode_refresh;
	bool cmd_bl_can_set;

	struct edc_overlay_ctrl ctrl;
	struct platform_device *pdev;
	struct semaphore sem;
	struct clk *edc_clk;
	struct clk *edc_clk_rst;
	struct clk *ldi_clk;
	struct clk *mipi_dphy_clk;
	struct clk *g2d_clk;
	struct regulator *edc_vcc;

	struct k3fb_vsync vsync_info;
	bool use_fake_vsync;
	struct hrtimer fake_vsync_hrtimer;

	int ambient_temperature;

	/* for sbl */
	u32 sbl_enable;
	u32 sbl_lsensor_value;
	struct work_struct sbl_work;
	struct workqueue_struct *sbl_wq;

	/* for temperature obtaining*/
	struct work_struct temp_work;
	struct workqueue_struct *temp_wq;

	/* for frc */
	s32 frc_state;
	s32 frc_threshold_count;
	unsigned long frc_timestamp;

	/* for esd */
	unsigned long esd_timestamp;
	s32 esd_frame_count;
	bool esd_recover;

	struct hrtimer esd_hrtimer;
	bool esd_hrtimer_enable;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

#if defined(CONFIG_OVERLAY_COMPOSE)
	/* type: full/partial/none */
	u32 ovc_type;
	u32 ovc_ch_count;
	/* ch chg state */
#  if defined(EDC_CH_CHG_SUPPORT)
	bool ch_chg_flag;
	bool ch_chg_power_off;
#  endif
	/* idle switch to g2d */
	bool ovc_idle_flag;
	/* ovc buffer sync: ch1,ch2,crs */
	bool ovc_wait_state[MAX_EDC_CHANNEL];
	bool ovc_wait_flag[MAX_EDC_CHANNEL];
	u32 ovc_lock_addr[MAX_EDC_CHANNEL];
	u32 ovc_lock_size[MAX_EDC_CHANNEL];
	u32 ovc_ch_gonna_display_addr[MAX_EDC_CHANNEL];
	u32 ovc_ch_display_addr[MAX_EDC_CHANNEL];
	/* overlay play cfg_ok once */
	struct overlay_data ovc_req[MAX_EDC_CHANNEL];
#  if defined(CONFIG_CPU_FREQ_GOV_K3HOTPLUG)
	u32 ddr_min_freq;
	u32 ddr_min_freq_saved;
	u32 ddr_curr_freq;
	u32 ovc_ddr_failed;
	struct pm_qos_request_list ovc_ddrminprofile;
	struct work_struct ovc_ddr_work;
	struct workqueue_struct *ovc_ddr_wq;
	u32 ovcIdleCount;
#  endif
#endif

};


/******************************************************************************
** FUNCTIONS PROTOTYPES
*/

extern u32 k3fd_reg_base_edc0;
extern u32 k3fd_reg_base_edc1;
extern u32 k3fd_reg_base_pwm0;
extern u32 k3fd_reg_base_pwm1;

void k3_fb_set_backlight(struct k3_fb_data_type *k3fd, u32 bkl_lvl);
struct platform_device *k3_fb_add_device(struct platform_device *pdev);
void k3_fb_clk_enable_cmd_mode(struct k3_fb_data_type *k3fd);
void k3_fb_clk_disable_cmd_mode(struct k3_fb_data_type *k3fd);

int k3_fb1_blank(int blank_mode);
int k3fb_buf_isfree(int phys);
void k3fb_set_hdmi_state(bool is_connected);
struct fb_var_screeninfo * k3fb_get_fb_var(int index);

#if defined(CONFIG_OVERLAY_COMPOSE)
void k3_fb_gralloc_overlay_save_display_addr(struct k3_fb_data_type *k3fd, int ch, u32 addr);
void k3_fb_gralloc_overlay_restore_display_addr(struct k3_fb_data_type *k3fd);
#if defined(EDC_CH_CHG_SUPPORT)
struct fb_info* k3_fb1_get_info(void);
#endif //EDC_CH_CHG_SUPPORT
void k3_fb_overlay_compose_data_clear(struct k3_fb_data_type *k3fd);

#endif //CONFIG_OVERLAY_COMPOSE

#endif /* K3_FB_H */
