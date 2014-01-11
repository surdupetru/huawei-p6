/* Copyright (c) 2008-2011, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/leds.h>
#include <linux/regulator/consumer.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <mach/boardid.h>

#include "k3_fb.h"
#include "edc_reg.h"
#include "ldi_reg.h"
#include "mipi_reg.h"
#include "sbl_reg.h"

#ifdef CONFIG_K3V2_TEMP_GOVERNOR
#include <linux/thermal_framework.h>
#define ADC_RTMP_FOR_SIM	0x02
#endif

#ifdef HDMI_DISPLAY
#include "hdmi/k3_hdmi.h"
#define ALWAYS_USE_BUFFER 1
#endif

#if defined(CONFIG_OVERLAY_COMPOSE)
#include <linux/proc_fs.h>

#if defined(CONFIG_CPU_FREQ_GOV_K3HOTPLUG)
#ifdef CONFIG_IPPS_SUPPORT
#include <linux/ipps.h>
#endif
#endif
extern int nct203_temp_report(void);

#define BUFFER_IS_USED(display_addr, addr, addr_size)  ((display_addr >= addr) && (display_addr < addr + addr_size))
#define OVC_BUFFER_SYNC_FILE "gralloc_overlay_state"
static DEFINE_SPINLOCK(gralloc_overlay_slock);
static DECLARE_WAIT_QUEUE_HEAD(gralloc_overlay_wait_queue);
#endif

#define K3_FB_MAX_DEV_LIST 32
#define MAX_FBI_LIST 32

static int k3_fb_resource_initialized;
static struct platform_device *pdev_list[K3_FB_MAX_DEV_LIST] = {0};

static int pdev_list_cnt;
static struct fb_info *fbi_list[MAX_FBI_LIST] = {0};
static int fbi_list_index;

static struct k3_fb_data_type *k3fd_list[MAX_FBI_LIST] = {0};
static int k3fd_list_index;

u32 k3fd_reg_base_edc0;
u32 k3fd_reg_base_edc1;
u32 k3fd_reg_base_pwm0;
u32 k3fd_reg_base_pwm1;

static u32 k3fd_irq_edc0;
static u32 k3fd_irq_edc1;
static u32 k3fd_irq_ldi0;
static u32 k3fd_irq_ldi1;
static bool hdmi_is_connected = false;

/* 0: g2d_clk_rate enable 1: 60MHz 2: 120MHz 3: 240MHz 4: 360MHz 5: 480MHz */
static int k3fb_debug_g2d_clk_rate = 0;
/* 0: not print g2d log 1: print g2d log */
static int k3fb_debug_g2d = 0;
/* 0: frc enable  1: frc disable 2: print frc log */
static int k3fb_debug_frc = 0;
/* 0: esd enable 1: esd disable */
static int k3fb_debug_esd = 0;
/* 0: not print vsync log 1: print vsync log */
static int k3fb_debug_vsync = 0;

static u32 k3_fb_pseudo_palette[16] = {
	0x00000000, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
};

#define MAX_BACKLIGHT_BRIGHTNESS 255

#define APICAL_INDOOR_UI       0x1
#define APICAL_OUTDOOR_UI      0x2   
#define APICAL_CAMERA_UI       0x3
DEFINE_SEMAPHORE(k3_fb_overlay_sem);
DEFINE_SEMAPHORE(k3_fb_backlight_sem);
DEFINE_SEMAPHORE(k3_fb_blank_sem);


/******************************************************************************
** FUNCTIONS PROTOTYPES
*/
static void k3_fb_set_backlight_cmd_mode(struct k3_fb_data_type *k3fd, u32 bkl_lvl);
static int k3_fb_blank_sub(int blank_mode, struct fb_info *info);
static int k3_fb_esd_set(struct k3_fb_data_type *k3fd);
static int k3_fb_esd_set_cmd_mode(struct fb_info *info);
static int k3fb_frc_get_fps(struct k3_fb_data_type *k3fd);

static void k3fb_te_inc(struct k3_fb_data_type *k3fd, bool te_should_enable, bool in_isr);
static void k3fb_te_dec(struct k3_fb_data_type *k3fd, bool te_should_disable, bool in_isr);


/******************************************************************************
** for HDMI
*/
static bool should_use_videobuf(struct fb_info *info)
{
#if !ALWAYS_USE_BUFFER
    int timing_code = info->var.reserved[3];
    if (timing_code == 32 || timing_code == 33 || timing_code == 34) {
        return true;
    }
    return false;
#else
    return true;
#endif
}

#if K3_FB_OVERLAY_USE_BUF
#define MAX_OVERLAY_BUF_NUM 3
typedef struct overlay_video_data {
	struct overlay_data data;
	bool is_set;
	u32 count;
} overlay_video_data;

typedef struct overlay_video_buf_ {
	overlay_video_data video_data_list[MAX_OVERLAY_BUF_NUM];
	struct work_struct play_work;
	u32 write_addr;
	u32 play_addr;
	u32 last_addr;
	bool exit_work;    
	struct workqueue_struct *play_wq;
	struct mutex overlay_mutex;
	bool is_init;
	bool is_video;
} overlay_video_buf;

static overlay_video_buf video_buf;

static void reset_video_buf(void)
{
	memset(&video_buf.video_data_list, 0, sizeof(video_buf.video_data_list));
	video_buf.write_addr = 0;
	video_buf.play_addr  = 0;
	video_buf.last_addr  = 0;
}

static void overlay_play_work_queue(struct work_struct *ws)
{
	struct fb_info *info = fbi_list[1];
	struct k3_fb_data_type *k3fd = NULL;
	overlay_video_data *play_data = NULL;
	u32 play_index = 0;
	int i = 0;
	int free_count = 0;
	int min_count  = 0;
	int null_count = 0;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	while (!video_buf.exit_work) {
		if (video_buf.is_video && hdmi_is_connected) {
			if (wait_event_interruptible_timeout(k3fd->frame_wq, k3fd->update_frame, HZ) <= 0) {
				k3fb_logw("wait_event_interruptible_timeout !\n");
				k3fd->update_frame = 0;
				continue;
			}
			k3fd->update_frame = 0;
		} else {
			msleep(80);
			continue;
		}
		
		mutex_lock(&video_buf.overlay_mutex);

		free_count = 0;
		min_count = 0;

		video_buf.play_addr = video_buf.write_addr;

		if (video_buf.is_video) {
			for (i = 0; i < MAX_OVERLAY_BUF_NUM; i++) {
				play_data = &video_buf.video_data_list[i];
				if (play_data->is_set) {
					if (0 == min_count || play_data->count < min_count) {
						min_count = play_data->count;
						play_index = i;
					}
				} else {
					free_count++;
				}
			}
		
			if (MAX_OVERLAY_BUF_NUM == free_count) {
				null_count++;
				if (null_count < 20) {
					k3fb_logw("video buff null 00000.\n");
				}
			} else {
				null_count = 0;
				play_data = &video_buf.video_data_list[play_index];

				if ((play_data->count == 1) && (free_count != 0)) {
					/*k3fb_logi("free_count:%d count:%d\n", free_count, play_data->count);*/
                } else {
					/*k3fb_logi("play index:%d count:%d\n", play_index, play_data->count);*/
    				video_buf.write_addr = play_data->data.src.phy_addr;
    				play_data->is_set = false;
				}
				edc_overlay_play(k3fd->fbi, &play_data->data);
			}
		}
		mutex_unlock(&video_buf.overlay_mutex);
	}
	return ;
}

static int overlay_play_work(struct k3_fb_data_type *k3fd)
{
	memset(&video_buf, 0, sizeof(video_buf));

	video_buf.play_wq = create_singlethread_workqueue("overlay_play_work"); 
	if (!(video_buf.play_wq)) {
		k3fb_loge("workqueue create failed !\n");
		return -1;
	}

	mutex_init(&video_buf.overlay_mutex);
	INIT_WORK(&video_buf.play_work, overlay_play_work_queue);

	video_buf.is_init = true;

	return 0;
}
#endif

int k3_fb1_blank(int blank_mode)
{
	int ret = 0;
	struct fb_info *info = fbi_list[1];
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	k3fb_logi("index=%d, enter!\n", k3fd->index);

	ret = k3_fb_blank_sub(blank_mode, info);
	if (ret != 0) {
		k3fb_loge("blank mode %d failed!\n", blank_mode);
	}

	if (blank_mode == FB_BLANK_UNBLANK) {
		k3_fb_set_backlight(k3fd, k3fd->bl_level);
	}

	k3fb_logi("index=%d, exit!\n", k3fd->index);

	return ret;
}

int k3fb_buf_isfree(int phys)
{
	int ret = 1;

#if K3_FB_OVERLAY_USE_BUF
	int i = 0;
	struct fb_info *info = fbi_list[1];
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	if (!k3fd->panel_power_on || phys == 0 || !video_buf.is_init || !hdmi_is_connected) {
		return ret;
	}

	mutex_lock(&video_buf.overlay_mutex);

	if (video_buf.write_addr == phys || video_buf.play_addr == phys) {
		/*k3fb_logi("addr:0x%x is playing!", phys); */
		ret = 0;
		goto done;
	}
	for (i = 0; i < MAX_OVERLAY_BUF_NUM; i++) {
		if (video_buf.video_data_list[i].is_set 
		   && (video_buf.video_data_list[i].data.src.phy_addr == phys)) {
			/*k3fb_logi("addr:0x%x in buf!", phys); */
			ret = 0;
			goto done;
		}
	}
	
done:
	mutex_unlock(&video_buf.overlay_mutex);
#endif
	return ret;
}

void k3fb_set_hdmi_state(bool is_connected)
{
	if (hdmi_is_connected == is_connected) {
		return;
	}
	k3fb_logi("hdmi_is_connected: %d is_connected: %d\n", hdmi_is_connected , is_connected);
	hdmi_is_connected = is_connected;
#if K3_FB_OVERLAY_USE_BUF
	if (video_buf.is_init) {
		mutex_lock(&video_buf.overlay_mutex);
		reset_video_buf();	
		mutex_unlock(&video_buf.overlay_mutex);

        if (hdmi_is_connected) {
            video_buf.exit_work = 0;
            queue_work(video_buf.play_wq, &video_buf.play_work);
        } else {
            video_buf.exit_work = 1;
        }
	}
#endif
	return;
}

struct fb_var_screeninfo * k3fb_get_fb_var(int index)
{
	struct fb_info *info = fbi_list[index];

	BUG_ON(info == NULL);

	return &info->var;
}

EXPORT_SYMBOL(k3_fb1_blank);
EXPORT_SYMBOL(k3fb_buf_isfree);
EXPORT_SYMBOL(k3fb_set_hdmi_state);
EXPORT_SYMBOL(k3fb_get_fb_var);


/******************************************************************************/
static int k3fb_frc_set_state(struct fb_info *info, unsigned long *argp)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;
	char buffer[4];

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	ret = copy_from_user(buffer, argp, sizeof(buffer));
	if (ret) {
		k3fb_loge("copy from user failed, ret = %d \n", ret);
		return -EFAULT;
	}

	/*  Different input values mean different application scenes as follow:
	  *  1) 1:1 or 1:0 mean Video is playing or stop playing;
	  *  2) 2:1 or 2:0 mean Game is playing or stop playing;
	  *  3) 3:1 or 3:0 mean Benchmark is runing or stop running.
	  *  4) 4:1 or 4:0 mean Webkit is running or stop running.
	  *  5) 5:1 or 5:0 mean Special Game is playing or stop playing.
         *  6) 7:1 or 7:0 mean game 30 fps playing or stop playing.
	  *  7) ......
	  */

	switch (buffer[0]-'0') {
	case 1:
	case 2:
	case 3:
	case 4:
    case 5:
    case 7:
		{
			if (buffer[2] == '0') {
				k3fd->frc_state &= ~(1 << (buffer[0] - '0'));
			} else {
				k3fd->frc_state |= (1 << (buffer[0] - '0'));
			}
		}
		break;

	default:
		break;
	}

	return 0;
}

static int k3fd_g2d_clk_rate_off;
static void k3_fb_set_g2d_clk_rate(int level)
{
	struct k3_fb_data_type *k3fd = NULL;
	int ret = 0;
	u32 rate = 0;

	k3fd = k3fd_list[0];
	BUG_ON(k3fd == NULL);

	if (!k3fd->panel_power_on) {
		return;
	}

	if (k3fb_debug_g2d_clk_rate > 0) {
		level = k3fb_debug_g2d_clk_rate;
	}

	switch (level) {
	case 1:
		rate = G2D_CLK_60MHz;
		break;
	case 2:
		rate = G2D_CLK_120MHz;
		break;
	case 3:
		rate = G2D_CLK_240MHz;
		break;
	case 4:
		rate = G2D_CLK_360MHz;
		break;
	case 5:
		rate = G2D_CLK_480MHz;
		break;
	default:
		rate = G2D_CLK_480MHz;
		break;
	}

	if (k3fd->g2d_clk) {
		ret = clk_set_rate(k3fd->g2d_clk,  rate);
		if (ret != 0) {
			k3fb_loge("%d, error = %d.\n", rate, ret);
		} else {
			if (k3fb_debug_g2d)
				k3fb_logi("%d.\n", rate);
		}
	}
}

static int k3fb_g2d_set_freq(struct fb_info *info, unsigned long *argp)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;
	char buffer[4];

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	ret = copy_from_user(buffer, argp, sizeof(buffer));
	if (ret) {
		k3fb_loge("copy from user failed, ret = %d \n", ret);
		return -EFAULT;
	}

	switch (buffer[0]-'0') {
	case 9:
	{
		if (k3fd->frc_state == K3_FB_FRC_BENCHMARK_PLAYING) {
			k3_fb_set_g2d_clk_rate(5);
			break;
		}

		if (buffer[2] == '0') {
			if (k3fd_g2d_clk_rate_off == 0) {
				k3fd_g2d_clk_rate_off = 1;
				k3_fb_set_g2d_clk_rate(5);
			} else {
				k3fd_g2d_clk_rate_off = 0;
			}
		}

		if (k3fd_g2d_clk_rate_off == 0)
			k3_fb_set_g2d_clk_rate(buffer[2]-'0');
	}
		break;
	default:
		break;
	}

	return 0;
}

int apical_flags = 0x0;
static int k3fb_sbl_set_value(struct fb_info *info, unsigned long *argp)
{
    int ret = 0;
    int val = 0;
    struct k3_fb_data_type *k3fd = NULL;

    int value_flags;
    u32 edc_base = 0;
    BUG_ON(info == NULL);
    k3fd = (struct k3_fb_data_type *)info->par;
    BUG_ON(k3fd == NULL);
    edc_base = k3fd->edc_base;

    if (copy_from_user(&val, argp, sizeof(val))) {
        k3fb_loge("copy from user failed!\n");
        return -EFAULT;
    }

    if (k3fd->panel_info.sbl_enable) {
        down(&k3_fb_blank_sem);
        k3fd->sbl_lsensor_value = val & 0xffff;
        k3fd->sbl_enable = (val >> 16) & 0x1;
        value_flags = (val >> 17) & 0x3;
                
        if (value_flags && k3fd->sbl_enable) {
            if (value_flags == APICAL_OUTDOOR_UI) {
                set_reg(edc_base + SBL_STRENGTH_MANUAL_OFFSET, 0x80, 8, 0);
                apical_flags = 0x80;
            } else if (value_flags == APICAL_CAMERA_UI) {
                set_reg(edc_base + SBL_STRENGTH_MANUAL_OFFSET, 0x20, 8, 0);
                apical_flags = 0x20;
            } else {
                set_reg(edc_base + SBL_STRENGTH_MANUAL_OFFSET, 0x0, 8, 0);
                apical_flags = 0x0;
            }
        }

		if (!k3fd->panel_power_on) {
			k3fb_logw("panel power is not on.\n");
			up(&k3_fb_blank_sem);
			return -ENODEV;;
		}

		sbl_ctrl_set(k3fd);
		up(&k3_fb_blank_sem);
	}

	return ret;
}


/*******************************************************************************
** for overlay compose
*/
#if defined(CONFIG_OVERLAY_COMPOSE)

static void k3_fb_overlay_compose_parse_buffer(char* buf, u32* para_h, u32* para_t)
{
    int length;
    int pos;
    char buf_head[20];
    char buf_tail[20];

    if (buf == NULL) {
        *para_h = 0;
        *para_t = 0;
        return;
    }
    memset(buf_head, 0, sizeof(buf_head));
    memset(buf_tail, 0, sizeof(buf_tail));

    /* buffer format => 12345:12345 */
    length = strlen(buf);
    pos = strstr(buf, ":") - buf;

    memcpy(buf_head, buf, pos);
    buf_head[pos] = '\0';
    memcpy(buf_tail, buf + pos + 1, length - pos);

    *para_h = simple_strtoul(buf_head, NULL, 10); /* decimal */
    *para_t = simple_strtoul(buf_tail, NULL, 10); /* decimal */
}

#if defined(EDC_CH_CHG_SUPPORT)
struct fb_info* k3_fb1_get_info(void)
{
    struct fb_info *info = fbi_list[1];
    BUG_ON(info == NULL);
    return info;
}

static void k3_fb_overlay_compose_edc1_poweroff(struct k3_fb_data_type *k3fd)
{
    if (k3fd->ch_chg_power_off) {
        k3fd->ch_chg_power_off = false;
        overlay_compose_edc1_power_off();
    }
}
#endif //EDC_CH_CHG_SUPPORT

#if defined(CONFIG_CPU_FREQ_GOV_K3HOTPLUG)

#ifdef CONFIG_IPPS_SUPPORT
static void k3_fb_ippsclient_add(struct ipps_device *device)
{
}

static void k3_fb_ippsclient_remove(struct ipps_device *device)
{
}

static struct ipps_client k3_fb_ipps_client = {
    .name   = "k3_fb_ddr_ipps",
    .add    = k3_fb_ippsclient_add,
    .remove = k3_fb_ippsclient_remove
};

static u32 k3_fb_get_current_ddr_freq(void)
{
    /*
        return value:
        58000, 120000, 360000,450000
    */
    u32 freq = 0;

    ipps_get_current_freq(&k3_fb_ipps_client, IPPS_OBJ_DDR, &freq);
    return freq;
}
#endif /* CONFIG_IPPS_SUPPORT */

static void k3_fb_overlay_compose_ddr_workqueue(struct work_struct *ws)
{
    unsigned long flags;
    struct k3_fb_data_type *k3fd = NULL;
    bool need_request = false;
    u32 curr_ddr_freq;

    k3fd  = container_of(ws, struct k3_fb_data_type, ovc_ddr_work);
    BUG_ON(k3fd == NULL);

#ifdef CONFIG_IPPS_SUPPORT
    curr_ddr_freq = k3_fb_get_current_ddr_freq();
#endif

    spin_lock_irqsave(&gralloc_overlay_slock, flags);
    /* printk("ddr_workqueue: old_min = %d, new_min = %d , curr freq = %d \n", k3fd->ddr_min_freq_saved, k3fd->ddr_min_freq, curr_ddr_freq); */
    if (k3fd->ddr_min_freq_saved !=  k3fd->ddr_min_freq) {
        if (k3fd->ddr_min_freq_saved > k3fd->ddr_min_freq) {
            curr_ddr_freq = 0;
        }
        need_request = true;
        k3fd->ddr_min_freq_saved = k3fd->ddr_min_freq;
    }

    k3fd->ddr_curr_freq = curr_ddr_freq;
    spin_unlock_irqrestore(&gralloc_overlay_slock, flags);

    if (need_request) {
        pm_qos_update_request(&k3fd->ovc_ddrminprofile, k3fd->ddr_min_freq_saved);
    }
}

static int k3_fb_overlay_compose_create_ddr_workqueue(struct k3_fb_data_type *k3fd)
{
    k3fd->ovc_ddr_wq = create_singlethread_workqueue("ovc_ddr_freq_workqueue");
    if (!k3fd->ovc_ddr_wq) {
        k3fb_loge("failed to create ovc ddr workqueue!\n");
        return -1;
    }

    INIT_WORK(&k3fd->ovc_ddr_work, k3_fb_overlay_compose_ddr_workqueue);
    return 0;
}

static int k3_fb_overlay_compose_destroy_ddr_workqueue(struct k3_fb_data_type *k3fd)
{
    if (k3fd->ovc_ddr_wq) {
        destroy_workqueue(k3fd->ovc_ddr_wq);
        k3fd->ovc_ddr_wq = NULL;
    }
    return 0;
}

#if defined(CONFIG_LCD_1080P)
/* 1080p */
#define MIN_FULL_OVC_ROTATE (360000)
#define MIN_FULL_OVC_2 (360000)
#define MIN_FULL_OVC_2_ROTATE (360000)
#define MIN_FULL_OVC_3 (360000)
#define MIN_FULL_OVC_3_ROTATE (450000)
#else
/* k3v2 actual 720p */
#define MIN_FULL_OVC_ROTATE (120000)
#define MIN_FULL_OVC_2 (120000)
#define MIN_FULL_OVC_2_ROTATE (360000)
#define MIN_FULL_OVC_3 (120000)
#define MIN_FULL_OVC_3_ROTATE (360000)
#endif

#if 0
/* 720p theory */
#define MIN_FULL_OVC_ROTATE (120000)
#define MIN_FULL_OVC_2 (61000)
#define MIN_FULL_OVC_2_ROTATE (180000)
#define MIN_FULL_OVC_3 (92000)
#define MIN_FULL_OVC_3_ROTATE (240000)
#endif

static u32 k3_fb_overlay_compose_get_target_ddr_min_freq(u32 count, u32 rotate)
{
    u32 min_ddr_freq = 0;
    switch (count) {
        case 1:
            min_ddr_freq = (rotate == 0) ? 0 : MIN_FULL_OVC_ROTATE;
            break;
        case 2:
            min_ddr_freq = (rotate == 0) ? MIN_FULL_OVC_2 : MIN_FULL_OVC_2_ROTATE;
            break;
        case 3:
            min_ddr_freq = (rotate == 0) ? MIN_FULL_OVC_3 : MIN_FULL_OVC_3_ROTATE;
            break;
        default:
            min_ddr_freq = 0;
            break;
    }
    return min_ddr_freq;
}

static int k3_fb_overlay_compose_check_ddr_min_freq(struct k3_fb_data_type *k3fd, u32 count, u32 rotate)
{
    /* 0: overlay enable
       -1: overlay disbale
    */
    unsigned long flags;
    u32 min_ddr_freq = 0;
    int ret = -1;
    bool need_qw = false;

    BUG_ON(k3fd->ovc_ddr_wq == NULL);

    min_ddr_freq = k3_fb_overlay_compose_get_target_ddr_min_freq(count, rotate);

    spin_lock_irqsave(&gralloc_overlay_slock, flags);
    /* printk("INCREASE: target ddr = %d, curr ddr = %d \n", min_ddr_freq, k3fd->ddr_min_freq); */
    if (min_ddr_freq > k3fd->ddr_min_freq) {
        need_qw = true;
        k3fd->ddr_min_freq = min_ddr_freq;
        queue_work(k3fd->ovc_ddr_wq, &k3fd->ovc_ddr_work);
    }

    /* printk("%s: k3_fb_get_current_ddr_freq = %d \n", __func__, k3fd->ddr_curr_freq); */
    if (k3fd->ddr_curr_freq >= min_ddr_freq) {
        ret = 0;
    } else {
        if (!need_qw) {
            queue_work(k3fd->ovc_ddr_wq, &k3fd->ovc_ddr_work);
        }
        k3fd->ovc_ddr_failed = 1;
    }
    spin_unlock_irqrestore(&gralloc_overlay_slock, flags);

    return ret;
}

#endif /* CONFIG_CPU_FREQ_GOV_K3HOTPLUG */

void k3_fb_overlay_compose_data_clear(struct k3_fb_data_type *k3fd)
{
    int i;

    k3fd->ovc_type = OVC_NONE;
    k3fd->ovc_ch_count = 0;

#if defined(EDC_CH_CHG_SUPPORT)
    k3fd->ch_chg_flag = false;
    k3fd->ch_chg_power_off = false;
#endif /* EDC_CH_CHG_SUPPORT */
    k3fd->ovc_idle_flag = false;

    for (i = 0; i< MAX_EDC_CHANNEL; i++) {
        if (k3fd->ovc_wait_state[i]) {
            /* wake up buffer sync before suspend */
            k3fd->ovc_wait_state[i] = false;
            k3fd->ovc_wait_flag[i] = true;
            wake_up_interruptible(&gralloc_overlay_wait_queue);
        } else {
            k3fd->ovc_wait_flag[i] = false;
            k3fd->ovc_lock_addr[i] = 0;
            k3fd->ovc_lock_size[i] = 0;
        }

        k3fd->ovc_ch_gonna_display_addr[i] = 0;
        k3fd->ovc_ch_display_addr[i] = 0;
        memset(&k3fd->ovc_req[i], 0, sizeof(k3fd->ovc_req[i]));
    }

#if defined(CONFIG_CPU_FREQ_GOV_K3HOTPLUG)
    if (k3fd->ddr_min_freq > 0) {
        k3fd->ddr_min_freq = 0;
        pm_qos_update_request(&k3fd->ovc_ddrminprofile, k3fd->ddr_min_freq);
    }
    k3fd->ddr_min_freq_saved = 0;
    k3fd->ddr_curr_freq = 0;
    k3fd->ovc_ddr_failed = 0;
    k3fd->ovcIdleCount = 0;
#endif /* CONFIG_CPU_FREQ_GOV_K3HOTPLUG */
}

static void k3_fb_gralloc_overlay_ch_process(struct k3_fb_data_type *k3fd)
{
    volatile bool cfg_ok;
    volatile bool ch1_en;
    volatile bool ch2_en;
    volatile bool crs_en;
    volatile u32 ch_addr[MAX_EDC_CHANNEL];
    int i;
#if defined(CONFIG_CPU_FREQ_GOV_K3HOTPLUG)
    volatile u32 ch1_rotate;
    u32 min_ddr_freq;
    bool addr_changed = false;
#endif /* CONFIG_CPU_FREQ_GOV_K3HOTPLUG */

    spin_lock(&gralloc_overlay_slock);

    cfg_ok = edc_overlay_compose_get_cfg_ok(k3fd);
    /* edc will load data when cfg_ok reset to 0 */
    if (cfg_ok) {
        /* printk("k3_fb_gralloc_overlay_ch_process error----\n"); */
        spin_unlock(&gralloc_overlay_slock);
        return;
    }

    ch1_en = edc_overlay_compose_get_ch1_state(k3fd);
    ch2_en = edc_overlay_compose_get_ch2_state(k3fd);
    crs_en = edc_overlay_compose_get_crs_state(k3fd);

    memset((void*)ch_addr, 0, sizeof(ch_addr));
    if (ch1_en) {
        ch_addr[0] = inp32(k3fd->edc_base + EDC_CH1L_ADDR_OFFSET);
    }

    if (ch2_en) {
        ch_addr[1] = inp32(k3fd->edc_base + EDC_CH2L_ADDR_OFFSET);
    }

    if (crs_en) {
        ch_addr[2] = inp32(k3fd->edc_base + EDC_CRSL_ADDR_OFFSET);
    }

#if defined(EDC_CH_CHG_SUPPORT)
    k3_fb_overlay_compose_edc1_poweroff(k3fd);
#endif
    k3_fb_gralloc_overlay_restore_display_addr(k3fd);

    for (i=0; i<MAX_EDC_CHANNEL; i++) {
        if (ch_addr[i] != k3fd->ovc_ch_display_addr[i]) {
    #if defined(CONFIG_CPU_FREQ_GOV_K3HOTPLUG)
            addr_changed = true;
    #endif
            if (k3fd->ovc_ch_display_addr[i] != 0 && k3fd->ovc_lock_addr[i] != 0
                && !BUFFER_IS_USED(ch_addr[i], k3fd->ovc_lock_addr[i], k3fd->ovc_lock_size[i])) {
                k3fd->ovc_wait_flag[i] = true;
                wake_up_interruptible(&gralloc_overlay_wait_queue);
            }
        }
        k3fd->ovc_ch_display_addr[i] = ch_addr[i];
    }

#if defined(CONFIG_CPU_FREQ_GOV_K3HOTPLUG)
    ch1_rotate = edc_overlay_compose_reg_get_ch1_rotation(k3fd);
    min_ddr_freq = k3_fb_overlay_compose_get_target_ddr_min_freq(ch1_en + ch2_en + crs_en, ch1_rotate);

    /* printk("DECREASE: target ddr = %d, curr ddr = %d k3fd->ovc_ddr_failed=%d, addr_changed=%d \n", min_ddr_freq, k3fd->ddr_min_freq, k3fd->ovc_ddr_failed, addr_changed); */
    if (min_ddr_freq < k3fd->ddr_min_freq) {
        if (k3fd->ovc_ddr_failed && addr_changed) {
            k3fd->ovc_ddr_failed = 0;
        } else if (k3fd->ovc_ddr_failed == 0) {
            k3fd->ddr_min_freq = min_ddr_freq;
            if (k3fd->ovc_ddr_wq) {
                queue_work(k3fd->ovc_ddr_wq, &k3fd->ovc_ddr_work);
            }
        }
    }
#endif /* CONFIG_CPU_FREQ_GOV_K3HOTPLUG */

    spin_unlock(&gralloc_overlay_slock);

}

static int k3_fb_gralloc_overlay_buffer_is_busy(struct k3_fb_data_type *k3fd, u32 addr, u32 size)
{
    int i;

    for (i=0; i<MAX_EDC_CHANNEL; i++) {
        if (BUFFER_IS_USED(k3fd->ovc_ch_display_addr[i], addr, size) || BUFFER_IS_USED(k3fd->ovc_ch_gonna_display_addr[i], addr, size)) {
            k3fd->ovc_lock_addr[i] = addr;
            k3fd->ovc_lock_size[i] = size;
            return i;
        }
    }
    return -1;
}

static int k3_fb_gralloc_overlay_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char buf[32];
    unsigned long buf_len;
    struct k3_fb_data_type *k3fd = NULL;
    unsigned long flags;
    u32 buf_addr;
    u32 buf_size;
    int curr = 0;

    k3fd = k3fd_list[0];
    BUG_ON(k3fd == NULL);

    buf_len = min_t(unsigned long, count, sizeof(buf) - 1);
    if (copy_from_user(buf, buffer, buf_len)) {
        printk("%s:copy_from_user Error\n", __func__);
        return 0;
    }

    buf[buf_len] = '\0';
    k3_fb_overlay_compose_parse_buffer(buf, &buf_addr, &buf_size);
    //printk("parse buffer:addr=0x%x, size=%d\n", buf_addr, buf_size);

    spin_lock_irqsave(&gralloc_overlay_slock, flags);
    curr = k3_fb_gralloc_overlay_buffer_is_busy(k3fd, buf_addr, buf_size);
    spin_unlock_irqrestore(&gralloc_overlay_slock, flags);
    if (curr >= 0) {
        k3fd->ovc_wait_state[curr] = true;

        wait_event_interruptible_timeout(gralloc_overlay_wait_queue, k3fd->ovc_wait_flag[curr], 2 * HZ);
        //wait_event_interruptible(gralloc_overlay_wait_queue, k3fd->ovc_wait_flag[curr]);

        k3fd->ovc_wait_state[curr] = false;
        k3fd->ovc_wait_flag[curr] = false;
        k3fd->ovc_lock_addr[curr] = 0;
        k3fd->ovc_lock_size[curr] = 0;
    }
    return 0;
}

void k3_fb_gralloc_overlay_save_display_addr(struct k3_fb_data_type *k3fd, int ch, u32 addr)
{
    int index = 0;
    if (ch == OVERLAY_PIPE_EDC0_CURSOR) {
        index = MAX_EDC_CHANNEL - 1;
    } else {
        index = ch;
    }
    BUG_ON(index > MAX_EDC_CHANNEL - 1);
    k3fd->ovc_ch_gonna_display_addr[index] = addr;
}

void k3_fb_gralloc_overlay_restore_display_addr(struct k3_fb_data_type *k3fd)
{
    int i;
    for (i = 0; i < MAX_EDC_CHANNEL; i++) {
        k3fd->ovc_ch_gonna_display_addr[i] = 0;
    }
}

/* overlay compose ilde will switch to g2d */
static DECLARE_WAIT_QUEUE_HEAD(overlay_idle_wait_queue);

static void k3_fb_overlay_idle_notice(struct k3_fb_data_type *k3fd)
{
    u32 count;
    const u32 idle_ch_count = 1;
    bool need_notice = false;
    volatile u32 ch1_en = edc_overlay_compose_get_ch1_state(k3fd);
    volatile u32 ch1_rotate = edc_overlay_compose_reg_get_ch1_rotation(k3fd);
    volatile u32 ch2_en = edc_overlay_compose_get_ch2_state(k3fd);
    volatile u32 ch2_fmt = edc_overlay_compose_get_ch2_fmt(k3fd);
    volatile u32 crs_en = edc_overlay_compose_get_crs_state(k3fd);

    count = ch1_en + ch2_en + crs_en;
    if (count > idle_ch_count) {
        /* 2 or 3 channels working need to notice */
        need_notice = true;
    } else if (count == idle_ch_count && ch1_rotate != 0) {
        /* 1 rotate ch need to notice for ddr */
        need_notice = true;
    }

    /*g2d compose && fb fmt is rgb565*/
    if(ch2_en && (1 == count)  && (ch2_fmt == 0x1)){
        need_notice = true;
    }

    if (need_notice) {
        k3fd->ovc_idle_flag = true;
        wake_up_interruptible(&overlay_idle_wait_queue);
    }
}

static ssize_t k3_fb_overlay_idle_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct k3_fb_data_type *k3fd = NULL;
    int ret = 0;

    k3fd = k3fd_list[0];
    BUG_ON(k3fd == NULL);

   ret = wait_event_interruptible(overlay_idle_wait_queue, k3fd->ovc_idle_flag);
   k3fd->ovc_idle_flag = false;
   return 0;
}

static int k3_fb_overlay_compose_wait_event(struct k3_fb_data_type *k3fd)
{
    if (wait_event_interruptible_timeout(k3fd->frame_wq, k3fd->update_frame, HZ / 10) <= 0) {
        k3fb_logw("wait_event_interruptible_timeout !\n");
        k3fd->update_frame = 0;
        return -ETIME;
    }
    k3fd->update_frame = 0;
    return 0;
}

/*overlaylib queuebuffer*/
static int k3_fb_overlay_compose_play(struct fb_info *info, struct overlay_data req)
{
    struct k3_fb_data_type *k3fd = NULL;
    int i;
    int ret = 0;
    int index = 0;
    uint32_t cfg_disable = 0;
    unsigned long flags;
    int wait_ret = 0;
    bool cmd_mode;
    u32 pre_type;
    u32 pre_count;

    BUG_ON(info == NULL);
    k3fd = (struct k3_fb_data_type *)info->par;
    BUG_ON(k3fd == NULL);

    if (req.id == OVERLAY_PIPE_EDC0_CURSOR) {
        index = MAX_EDC_CHANNEL - 1;
    } else {
        index = req.id;
    }

    BUG_ON(index > MAX_EDC_CHANNEL - 1);
    memcpy(&k3fd->ovc_req[index], &req, sizeof(req));
    cfg_disable = edc_overlay_compose_get_cfg_disable(info, req.id);

    if (!cfg_disable) {
        pre_type = k3fd->ovc_type;
        k3fd->ovc_type = OVC_FULL;
        pre_count = k3fd->ovc_ch_count;
        k3fd->ovc_ch_count = index + 1;

        cmd_mode = (k3fd->panel_info.type == PANEL_MIPI_CMD);
        if (cmd_mode) {
		k3_fb_clk_enable_cmd_mode(k3fd);
        } else {
            //normal mode
            wait_ret = k3_fb_overlay_compose_wait_event(k3fd);
            if (wait_ret < 0) {
                return wait_ret;
            }
        }

        /*down(&k3fd->sem);*/
        spin_lock_irqsave(&gralloc_overlay_slock, flags);
        /*unset g2d when new frame is overlay*/
        edc_overlay_compose_pipe_unset_previous(info, pre_type, pre_count);
        for (i = 0; i <= index; i++) {
            ret = edc_overlay_compose_play(info, &k3fd->ovc_req[i]);
            memset(&k3fd->ovc_req[i], 0, sizeof(k3fd->ovc_req[i]));
        }
        spin_unlock_irqrestore(&gralloc_overlay_slock, flags);
        /*up(&k3fd->sem);*/

        if (cmd_mode) {
		// Clear idle count
		k3fd->ovcIdleCount = 0;

		k3fb_te_inc(k3fd, true, false);

		if (k3fd->frc_state != K3_FB_FRC_IDLE_PLAYING)
			k3fd->use_fake_vsync = k3fb_frc_get_fps(k3fd) < K3_FB_FRC_NORMAL_FPS ? true : false;

            wait_ret = k3_fb_overlay_compose_wait_event(k3fd);
            if (wait_ret < 0) {
                if (k3fd->panel_info.esd_enable) {
                    if (k3_fb_blank_sub(FB_BLANK_POWERDOWN, info) != 0) {
                        k3fb_loge("blank mode %d failed!\n", FB_BLANK_POWERDOWN);
                    }
                    if (k3_fb_blank_sub(FB_BLANK_UNBLANK, info) != 0) {
                        k3fb_loge("blank mode %d failed!\n", FB_BLANK_UNBLANK);
                    }
                    k3fb_loge("ESD recover!\n");
                }

		k3fb_te_dec(k3fd, false, false);
                return wait_ret;
            }

	    k3fb_te_dec(k3fd, false, false);
        }
    }

    if (k3fd->temp_wq)
        queue_work(k3fd->temp_wq, &k3fd->temp_work);

    return ret;
}

/* overlay compose partial */
static int k3_fb_overlay_compose_partial_state(struct fb_info *info)
{
    int i;
    int partial_ovc = 0;
    u32 is_ovc;
    /* notice: 1 ch partial ovc currently, 2 chs is under-construction. */
    for (i = 0; i < MAX_EDC_CHANNEL - 1; i++) {
        /* crs donot support ovc partial, check ch1,ch2 */
        is_ovc = edc_overlay_compose_get_state(info, i);
        if (!is_ovc) {
            break;
        }
        partial_ovc ++;
    }
    return partial_ovc;
}

static int k3_fb_overlay_compose_partial_play(struct fb_var_screeninfo *var, struct fb_info *info, int partial_ovc)
{
    struct k3_fb_data_type *k3fd = NULL;
    int ret = 0;
    int wait_ret = 0;
    unsigned long flags;
    int i;
    bool cmd_mode;
    u32 pre_type;
    u32 pre_count;

    BUG_ON(info == NULL);
    k3fd = (struct k3_fb_data_type *)info->par;
    BUG_ON(k3fd == NULL);

    pre_type = k3fd->ovc_type;
    k3fd->ovc_type = OVC_PARTIAL;
    pre_count = k3fd->ovc_ch_count;
    k3fd->ovc_ch_count = partial_ovc + 1;

    cmd_mode = (k3fd->panel_info.type == PANEL_MIPI_CMD);
    if (cmd_mode) {
	k3_fb_clk_enable_cmd_mode(k3fd);
    } else {
        wait_ret = k3_fb_overlay_compose_wait_event(k3fd);
        if (wait_ret < 0) {
            return wait_ret;
        }
    }

    BUG_ON(partial_ovc > MAX_EDC_CHANNEL - 1);

    spin_lock_irqsave(&gralloc_overlay_slock, flags);
    /*unset crs overlay if necessary*/
    edc_overlay_compose_pipe_unset_previous(info, pre_type, pre_count);
#if defined(EDC_CH_CHG_SUPPORT)
    edc_overlay_compose_ch_chg_disable(k3fd, info);
#endif /* EDC_CH_CHG_SUPPORT */
    for (i = 0; i < partial_ovc; i++) {
        ret = edc_overlay_compose_play(info, &k3fd->ovc_req[i]);
    }

    ret = edc_overlay_compose_pan_display(var, info, &k3fd->ovc_req[partial_ovc - 1]);
    spin_unlock_irqrestore(&gralloc_overlay_slock, flags);

    if (cmd_mode) {

	// Clear idle count
	k3fd->ovcIdleCount = 0;

	k3fb_te_inc(k3fd, true, false);

	if (k3fd->frc_state != K3_FB_FRC_IDLE_PLAYING)
		k3fd->use_fake_vsync = k3fb_frc_get_fps(k3fd) < K3_FB_FRC_NORMAL_FPS ? true : false;

        wait_ret = k3_fb_overlay_compose_wait_event(k3fd);
        if (wait_ret < 0) {
            if (k3fd->panel_info.esd_enable) {
                if (k3_fb_blank_sub(FB_BLANK_POWERDOWN, info) != 0) {
                    k3fb_loge("blank mode %d failed!\n", FB_BLANK_POWERDOWN);
                }
                if (k3_fb_blank_sub(FB_BLANK_UNBLANK, info) != 0) {
                    k3fb_loge("blank mode %d failed!\n", FB_BLANK_UNBLANK);
                }
                k3fb_loge("ESD recover!\n");
            }

	    k3fb_te_dec(k3fd, false, false);
            return wait_ret;
        }

	k3fb_te_dec(k3fd, false, false);
    }

    if (k3fd->temp_wq)
        queue_work(k3fd->temp_wq, &k3fd->temp_work);

    return ret;
}

static int k3fb_ovc_check_ddr_freq(struct fb_info *info, unsigned long *argp)
{
    char buf[8];
    int buf_len;
    int ret = 0;
    struct k3_fb_data_type *k3fd = NULL;
    u32 hwc_count;
    u32 hwc_rotate;

    BUG_ON(info == NULL);
    k3fd = (struct k3_fb_data_type *)info->par;
    BUG_ON(k3fd == NULL);

    if (argp == NULL) {
        return -EFAULT;
    }
    buf_len = min(strlen((char *)argp), sizeof(buf) - 1);
    ret = copy_from_user(buf, argp, buf_len);
    if (ret) {
        k3fb_loge("copy from user failed \n");
        return -EFAULT;
    }

    buf[buf_len] = '\0';
#if defined(CONFIG_CPU_FREQ_GOV_K3HOTPLUG)
    k3_fb_overlay_compose_parse_buffer(buf, &hwc_count, &hwc_rotate);
    /* printk("%s: count = %d, rotate = %d \n", __func__, hwc_count, hwc_rotate); */
    ret = k3_fb_overlay_compose_check_ddr_min_freq(k3fd, hwc_count, hwc_rotate);
#endif /* CONFIG_CPU_FREQ_GOV_K3HOTPLUG */

    return ret;
}


static int k3_fb_overlay_compose_init(struct k3_fb_data_type *k3fd)
{
    int err = 0;

    struct proc_dir_entry *ent;
    /* Creating read/write "gralloc overlay" entry, rwx:111 */
    ent = create_proc_entry(OVC_BUFFER_SYNC_FILE, 0222, NULL);
    if (ent == NULL) {
        k3fb_loge("Unable to create /proc/gralloc_overlay_state entry\n");
        err = -1;
        return err;
    }
    ent->write_proc = k3_fb_gralloc_overlay_write_proc;

#if defined(CONFIG_CPU_FREQ_GOV_K3HOTPLUG)

    pm_qos_add_request(&k3fd->ovc_ddrminprofile, PM_QOS_DDR_MIN_PROFILE, PM_QOS_DDR_MINPROFILE_DEFAULT_VALUE);
#ifdef CONFIG_IPPS_SUPPORT
    err = ipps_register_client(&k3_fb_ipps_client);
    if (err != 0) {
        k3fb_loge("k3_fb_ddr_ipps register client failed, please check!");
        return err;
    }
#endif /* CONFIG_IPPS_SUPPORT */
    k3_fb_overlay_compose_create_ddr_workqueue(k3fd);

#endif /* CONFIG_CPU_FREQ_GOV_K3HOTPLUG */

    return err;
}

static void k3_fb_overlay_compose_deinit(struct k3_fb_data_type *k3fd)
{
    remove_proc_entry(OVC_BUFFER_SYNC_FILE, NULL);

#if defined(CONFIG_CPU_FREQ_GOV_K3HOTPLUG)
    pm_qos_remove_request(&k3fd->ovc_ddrminprofile);
#ifdef CONFIG_IPPS_SUPPORT
    ipps_unregister_client(&k3_fb_ipps_client);
#endif
    k3_fb_overlay_compose_destroy_ddr_workqueue(k3fd);

#endif /* CONFIG_CPU_FREQ_GOV_K3HOTPLUG */

}

static void k3_fb_overlay_compose_reset_pipe(struct fb_info *info)
{
    int i;

    k3fb_logi("\n");
    for (i = OVERLAY_PIPE_EDC0_CH1; i < OVERLAY_PIPE_MAX; i++) {
        if (i == OVERLAY_PIPE_EDC1_CH1 || i == OVERLAY_PIPE_EDC1_CH2) {
            continue;
        }
        edc_overlay_compose_pre_unset(info, i);
    }
}

static DEVICE_ATTR(overlay_idle_state, (S_IRUGO | S_IWUSR | S_IWGRP),
	k3_fb_overlay_idle_show, 0);
#endif //CONFIG_OVERLAY_COMPOSE


/*******************************************************************************
** for debug
*/
static ssize_t k3_fb_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	sprintf(buf,
		"debug_g2d=%d\n"\
		"debug_g2d_clk_rate=%d\n"\
		"debug_frc=%d\n"\
		"debug_esd=%d\n"\
		"debug_vsync=%d\n",
		k3fb_debug_g2d,
		k3fb_debug_g2d_clk_rate,
		k3fb_debug_frc,
		k3fb_debug_esd,
		k3fb_debug_vsync);

	return strlen(buf);
}

static ssize_t k3_fb_debug_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
	int ret = 0;

	if (strstr(buf, "debug_g2d_clk_rate")) {
		ret = sscanf(buf, "debug_g2d_clk_rate=%d\n", &k3fb_debug_g2d_clk_rate);
		if ((ret != 0) && (k3fb_debug_g2d_clk_rate > 0)) {
			k3_fb_set_g2d_clk_rate(k3fb_debug_g2d_clk_rate);
		}
	} else if (strstr(buf, "debug_g2d")) {
		ret = sscanf(buf, "debug_g2d=%d\n", &k3fb_debug_g2d);
	} else if (strstr(buf, "debug_frc")) {
		ret = sscanf(buf, "debug_frc=%d\n", &k3fb_debug_frc);
	} else if (strstr(buf, "debug_esd")){
		ret = sscanf(buf, "debug_esd=%d\n", &k3fb_debug_esd);
	} else if (strstr(buf, "debug_vsync")) {
		ret = sscanf(buf, "debug_vsync=%d\n", &k3fb_debug_vsync);
	} else {
		k3fb_loge("Error input\n");
	}

	return size;
}

static DEVICE_ATTR(debug, (S_IRUGO | S_IWUSR | S_IWGRP),
	k3_fb_debug_show, k3_fb_debug_store);


/*******************************************************************************
**
*/
static struct attribute *k3_fb_attrs[] = {
#if defined(CONFIG_OVERLAY_COMPOSE)
	&dev_attr_overlay_idle_state.attr,
#endif
	&dev_attr_debug.attr,
	NULL
};

static struct attribute_group k3_fb_attr_group = {
	.attrs = k3_fb_attrs,
};

static int k3_fb_sysfs_create(struct platform_device *pdev)
{
	int ret = 0;

	ret = sysfs_create_group(&pdev->dev.kobj, &k3_fb_attr_group);
	if (ret) {
		k3fb_loge("create sysfs file failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

static void k3_fb_sysfs_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &k3_fb_attr_group);
}

static void sbl_workqueue(struct work_struct *ws)
{
	static unsigned int ALold;
	unsigned int res;
	int m = 3;
	u32 lsensor_h = 0;
	u32 lsensor_l = 0;
	u32 tmp_sbl = 0;
	struct k3_fb_data_type *k3fd = NULL;

	k3fd = container_of(ws, struct k3_fb_data_type, sbl_work);
	BUG_ON(k3fd == NULL);

	down(&k3_fb_blank_sem);
	if (!k3fd->panel_power_on) {
		up(&k3_fb_blank_sem);
		return;
	}

	k3_fb_clk_enable_cmd_mode(k3fd);

	tmp_sbl = inp32(k3fd->edc_base + EDC_DISP_DPD_OFFSET);
	if ((tmp_sbl & REG_SBL_EN) == REG_SBL_EN) {
		res = ((ALold << m) + ((int)k3fd->sbl_lsensor_value << 14) - ALold) >> m;
		if (res != ALold) {
			ALold = res;
			lsensor_h = ((res >> 14) >> 8) & 0xff;
			lsensor_l = (res >> 14) & 0xff;
			set_SBL_AMBIENT_LIGHT_L_ambient_light_l(k3fd->edc_base, lsensor_l);
			set_SBL_AMBIENT_LIGHT_H_ambient_light_h(k3fd->edc_base, lsensor_h);
		}
	}

	k3_fb_clk_disable_cmd_mode(k3fd);

	up(&k3_fb_blank_sem);

}

static int init_sbl_workqueue(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

	k3fd->sbl_wq = create_singlethread_workqueue("sbl_workqueue");
	if (!k3fd->sbl_wq) {
		k3fb_loge("failed to create sbl workqueue!\n");
		return -1;
	}

	INIT_WORK(&k3fd->sbl_work, sbl_workqueue);

	return 0;
}


static void temp_workqueue(struct work_struct *ws)
{
	struct k3_fb_data_type *k3fd = NULL;

	k3fd = container_of(ws, struct k3_fb_data_type, temp_work);
	BUG_ON(k3fd == NULL);

	if (!k3fd->panel_power_on) {
		return;
	}

    k3fd->ambient_temperature = nct203_temp_report();

}

static int init_temperature_workqueue(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

	k3fd->temp_wq = create_singlethread_workqueue("temp_workqueue");
	if (!k3fd->temp_wq) {
		k3fb_loge("failed to create sbl workqueue!\n");
		return -1;
	}

	INIT_WORK(&k3fd->temp_work, temp_workqueue);

	return 0;
}

static void frame_end_workqueue(struct work_struct *ws)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_fb_panel_data *pdata = NULL;
	struct fb_info *info = NULL;
	bool is_timeout = true;
	int delay_count = 0;
	u32 tmp = 0;
	int ret = 0;

	k3fd = container_of(ws, struct k3_fb_data_type, frame_end_work);
	BUG_ON(k3fd == NULL);
	pdata = (struct k3_fb_panel_data *)k3fd->pdev->dev.platform_data;

	info =  fbi_list[0];
	BUG_ON(info == NULL);

	down(&k3_fb_blank_sem);
	if (!k3fd->panel_power_on) {
		goto error;
	}

	udelay(120);
	if (k3fd->bl_level != k3fd->bl_level_cmd) {
		k3_fb_set_backlight_cmd_mode(k3fd, k3fd->bl_level);
	}

	if (k3fd->panel_info.esd_enable) {
		if (k3fb_debug_esd)
			goto error;

		if (k3fd->esd_hrtimer_enable) {
			/* check dsi stop state */
			while (1) {
				tmp = inp32(k3fd->edc_base + MIPIDSI_PHY_STATUS_OFFSET);
				if (((tmp & 0xA90) == 0xA90) || delay_count > 100) {
					is_timeout = (delay_count > 100) ? true : false;
					delay_count = 0;
					break;
				} else {
					udelay(1);
					++delay_count;
				}
			}

			if (is_timeout) {
				k3fb_loge("ESD check dsi stop state timeout\n");
				goto error;
			}

			/* disable generate High Speed clock */
			set_reg(k3fd->edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET, 0x0, 1, 0);
			/* check panel power status*/
			ret = pdata->check_esd(k3fd->pdev);
		        //k3fb_loge("--------ESD READ = 0x%x--------------\n", ret);
			if (ret != 0x00) {
				k3fd->esd_recover = true;
			}


			/* enable generate High Speed clock */
			set_reg(k3fd->edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET, 0x1, 1, 0);

			k3fd->esd_hrtimer_enable = false;
		}
	}

error:
	k3_fb_clk_disable_cmd_mode(k3fd);

	up(&k3_fb_blank_sem);
}

static int init_frame_end_workqueue(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

	k3fd->frame_end_wq = create_singlethread_workqueue("frame_end_workqueue");
	if (!k3fd->frame_end_wq) {
		k3fb_loge("failed to create frame end workqueue!\n");
		return -1;
	}

	INIT_WORK(&k3fd->frame_end_work, frame_end_workqueue);

	return 0;
}

static int k3fb_vsync_timestamp_changed(struct k3_fb_data_type *k3fd,
	ktime_t prev_timestamp)
{
	BUG_ON(k3fd == NULL);
	return !ktime_equal(prev_timestamp, k3fd->vsync_info.timestamp);
}

static int k3fb_wait_for_vsync_thread(void *data)
{
	struct k3_fb_data_type *k3fd = (struct k3_fb_data_type *)data;
	ktime_t prev_timestamp;
	int ret = 0;

	struct sched_param param = {.sched_priority = 99};
	sched_setscheduler(current, SCHED_RR, &param);

	while (!kthread_should_stop()) {
		prev_timestamp = k3fd->vsync_info.timestamp;
		ret = wait_event_interruptible_timeout(
					k3fd->vsync_info.wait,
					k3fb_vsync_timestamp_changed(k3fd, prev_timestamp) &&
					k3fd->vsync_info.active,
					msecs_to_jiffies(VSYNC_TIMEOUT_MSEC));

		if (ret > 0) {
			char *envp[2];
			char buf[64];
			snprintf(buf, sizeof(buf), "VSYNC=%llu",
					ktime_to_ns(k3fd->vsync_info.timestamp));
			envp[0] = buf;
			envp[1] = NULL;
			kobject_uevent_env(&k3fd->pdev->dev.kobj, KOBJ_CHANGE, envp);

			if (k3fb_debug_vsync) {
				k3fb_logi("%s\n", buf);
			}
		}
	}

	return 0;
}


static void k3fb_te_inc(struct k3_fb_data_type *k3fd, bool te_should_enable, bool in_isr)
{
	struct k3_fb_panel_data *pdata = NULL;
	unsigned long flag;

	BUG_ON(k3fd == NULL);
	pdata = dev_get_platdata(&k3fd->pdev->dev);
	BUG_ON(pdata == NULL);

	if (in_isr) {
		spin_lock(&k3fd->vsync_info.irq_lock);
	} else {
		spin_lock_irqsave(&k3fd->vsync_info.irq_lock, flag);
	}

	k3fd->vsync_info.te_refcount++;

	if (te_should_enable)
		set_MIPIDSI_TE_CTRL_te_mask_en(k3fd->edc_base, 0);

	if (in_isr) {
		spin_unlock(&k3fd->vsync_info.irq_lock);
	} else {
		spin_unlock_irqrestore(&k3fd->vsync_info.irq_lock, flag);
	}
}

static void k3fb_te_dec(struct k3_fb_data_type *k3fd, bool te_should_disable, bool in_isr)
{
	struct k3_fb_panel_data *pdata = NULL;
	unsigned long flag;

	BUG_ON(k3fd == NULL);
	pdata = dev_get_platdata(&k3fd->pdev->dev);
	BUG_ON(pdata == NULL);

	if (in_isr) {
		spin_lock(&k3fd->vsync_info.irq_lock);
	} else {
		spin_lock_irqsave(&k3fd->vsync_info.irq_lock, flag);
	}

	if (k3fd->vsync_info.te_refcount > 0)
		k3fd->vsync_info.te_refcount--;

	if (k3fd->vsync_info.te_refcount <= 0 && te_should_disable) {
		set_MIPIDSI_TE_CTRL_te_mask_en(k3fd->edc_base, 1);
	}

	if (in_isr) {
		spin_unlock(&k3fd->vsync_info.irq_lock);
	} else {
		spin_unlock_irqrestore(&k3fd->vsync_info.irq_lock, flag);
	}

}


/******************************************************************************/

static int k3fb_frc_get_fps(struct k3_fb_data_type *k3fd)
{
	int fps = 0;
	BUG_ON(k3fd == NULL);

	if (k3fd->panel_info.type == PANEL_MIPI_CMD) {
		switch (k3fd->frc_state) {
		case K3_FB_FRC_GAME_PLAYING:
		case K3_FB_FRC_GAME_PLAYING | K3_FB_FRC_IDLE_PLAYING:
			fps = K3_FB_FRC_GAME_FPS;
			break;
		case K3_FB_FRC_SPECIAL_GAME_PLAYING:
		case K3_FB_FRC_SPECIAL_GAME_PLAYING | K3_FB_FRC_IDLE_PLAYING:
			fps = K3_FB_FRC_SPECIAL_GAME_FPS;
			break;
                   
        case K3_FB_FRC_WEBKIT_PLAYING:  
        case K3_FB_FRC_WEBKIT_PLAYING | K3_FB_FRC_IDLE_PLAYING:  
             fps = K3_FB_FRC_WEBKIT_FPS;
            break;  

        case K3_FB_FRC_GAME_30_PLAYING:
        case K3_FB_FRC_GAME_30_PLAYING | K3_FB_FRC_IDLE_PLAYING:
             fps = K3_FB_FRC_GAME_30_FPS;
             break;
     
         default:
			fps = K3_FB_FRC_NORMAL_FPS;
			break;
		}
	} else {
		switch (k3fd->frc_state) {
		case K3_FB_FRC_IDLE_PLAYING:
			fps = K3_FB_FRC_IDLE_FPS;
			break;
		case K3_FB_FRC_GAME_PLAYING:
		case K3_FB_FRC_GAME_PLAYING | K3_FB_FRC_IDLE_PLAYING:
			fps = K3_FB_FRC_GAME_FPS;
		    break;
		case K3_FB_FRC_SPECIAL_GAME_PLAYING:
		case K3_FB_FRC_SPECIAL_GAME_PLAYING | K3_FB_FRC_IDLE_PLAYING:
			fps = K3_FB_FRC_SPECIAL_GAME_FPS;
			break;
		case K3_FB_FRC_VIDEO_IN_GAME:
		case K3_FB_FRC_VIDEO_IN_GAME | K3_FB_FRC_IDLE_PLAYING:
			fps = K3_FB_FRC_VIDEO_FPS;
			break;
		case K3_FB_FRC_BENCHMARK_PLAYING:
		case K3_FB_FRC_BENCHMARK_PLAYING | K3_FB_FRC_IDLE_PLAYING:
			fps = K3_FB_FRC_BENCHMARK_FPS;
			break;
		case K3_FB_FRC_WEBKIT_PLAYING:
		case K3_FB_FRC_WEBKIT_PLAYING | K3_FB_FRC_IDLE_PLAYING:
			fps = K3_FB_FRC_WEBKIT_FPS;
			break;
        case K3_FB_FRC_GAME_30_PLAYING:
        case K3_FB_FRC_GAME_30_PLAYING | K3_FB_FRC_IDLE_PLAYING:
             fps = K3_FB_FRC_GAME_30_FPS;
            break;
		case K3_FB_FRC_NONE_PLAYING:
		default:
			fps = K3_FB_FRC_NORMAL_FPS;
			break;
		}
	}

	return fps;
}

static bool k3fb_frc_prepare(struct k3_fb_data_type *k3fd)
{
	int fps = K3_FB_FRC_NORMAL_FPS;
	static u32 addr_old;
	u32 addr_new;
#if defined(CONFIG_OVERLAY_COMPOSE)
	/* for ch1 */
	static u32 ch1_addr_old;
	volatile u32 ch1_addr_new;
	/* for crs */
	static u32 crs_addr_old;
	volatile u32 crs_addr_new;
	static u32 frc_special_count;
#endif /* CONFIG_OVERLAY_COMPOSE */

	if (get_boot_into_recovery_flag() || time_after((k3fd->frc_timestamp  + HZ * 3), jiffies)) {
		return false;
	}

	if (k3fb_debug_frc == 1)
		return false;

	addr_new = inp32(k3fd->edc_base + EDC_CH2L_ADDR_OFFSET);
#if defined(CONFIG_OVERLAY_COMPOSE)
	ch1_addr_new = inp32(k3fd->edc_base + EDC_CH1L_ADDR_OFFSET);
	crs_addr_new = inp32(k3fd->edc_base + EDC_CRSL_ADDR_OFFSET);
#endif /* CONFIG_OVERLAY_COMPOSE */

	fps = k3fb_frc_get_fps(k3fd);
	if (fps > k3fd->panel_info.frame_rate) {
		k3fd->frc_threshold_count = 0;
		addr_old = addr_new;
	#if defined(CONFIG_OVERLAY_COMPOSE)
		ch1_addr_old= ch1_addr_new;
		crs_addr_old = crs_addr_new;
		frc_special_count = 0;
	#endif /* CONFIG_OVERLAY_COMPOSE */

		return true;
	} else if (fps < k3fd->panel_info.frame_rate) {

	#if defined(CONFIG_OVERLAY_COMPOSE)
		frc_special_count = 0;
	#endif /* CONFIG_OVERLAY_COMPOSE */

		if ((addr_new == addr_old)
	#if defined(CONFIG_OVERLAY_COMPOSE)
		&& (ch1_addr_new == ch1_addr_old) && (crs_addr_new == crs_addr_old)
	#endif /* CONFIG_OVERLAY_COMPOSE */
		) {
			k3fd->frc_threshold_count++;

			if (k3fd->frc_threshold_count >= K3_FB_FRC_THRESHOLD) {
				k3fd->frc_threshold_count = 0;
				return true;
			}
		} else {
			k3fd->frc_threshold_count = 0;
			addr_old = addr_new;
			return true;
		}
	}
#if defined(CONFIG_OVERLAY_COMPOSE)
	else {
		/* for special scene idle to g2d*/
		if (((k3fd->frc_state == K3_FB_FRC_WEBKIT_PLAYING)
			|| (k3fd->frc_state == K3_FB_FRC_GAME_PLAYING)
			|| (k3fd->frc_state == K3_FB_FRC_SPECIAL_GAME_PLAYING))
			&& ((addr_new == addr_old) && (ch1_addr_new == ch1_addr_old) && (crs_addr_new == crs_addr_old))) {
			frc_special_count ++;
			if (frc_special_count >= K3_FB_FRC_THRESHOLD) {
				k3_fb_overlay_idle_notice(k3fd);
				frc_special_count = 0;
			}
		} else {
			frc_special_count = 0;
		}
	}
#endif /* CONFIG_OVERLAY_COMPOSE */

	addr_old = addr_new;
#if defined(CONFIG_OVERLAY_COMPOSE)
	ch1_addr_old= ch1_addr_new;
	crs_addr_old = crs_addr_new;
#endif /* CONFIG_OVERLAY_COMPOSE */

	return false;
}

static int k3fb_frc_set(struct k3_fb_data_type *k3fd)
{
	int delay_count = 0;
	bool is_timeout = false;
	int tmp = 0;
	int fps = K3_FB_FRC_NORMAL_FPS;
	struct k3_fb_panel_data *pdata = NULL;

	pdata = (struct k3_fb_panel_data *)k3fd->pdev->dev.platform_data;
	if ((!pdata) || (!pdata->set_frc)) {
		k3fb_loge("no panel operation detected!\n");
		return 0;
	}

	/* wait dsi lane stop for 100us*/
	while (1) {
		tmp = inp32(k3fd->edc_base + MIPIDSI_PHY_STATUS_OFFSET);
		if (((tmp & 0xA90) == 0xA90) || delay_count > 100) {
			is_timeout = (delay_count > 100) ? true : false;
			delay_count = 0;
			break;
		} else {
			udelay(1);
			++delay_count;
		}
	}

	if (is_timeout) {
		k3fb_loge("wait dsi lane stop timeout.\n");
		return -1;
	}

	fps = k3fb_frc_get_fps(k3fd);
	if (fps == k3fd->panel_info.frame_rate) {
		/*k3fb_logi("no need to set frc.\n");*/
		return 0;
	}

	if (pdata->set_frc(k3fd->pdev,  fps) != 0) {
		k3fb_loge("set frc failed!\n");
		return -1;
	} else {
		if (k3fb_debug_frc)
			k3fb_logi("now fps will be %d.\n", fps);
	}

#if defined(CONFIG_OVERLAY_COMPOSE)
	if (k3fd->frc_state == K3_FB_FRC_IDLE_PLAYING
		&& (k3fd->panel_info.frame_rate == K3_FB_FRC_IDLE_FPS)) {
		k3_fb_overlay_idle_notice(k3fd);
	}
#endif /* CONFIG_OVERLAY_COMPOSE */
	return 0;
}


/******************************************************************************
** handle ESD
*/

static bool k3fb_esd_prepare(struct k3_fb_data_type *k3fd)
{
	if (time_after((k3fd->esd_timestamp  + HZ * 3), jiffies)) {
		return false;
	}

	if (k3fb_debug_esd)
		return false;

	if (++k3fd->esd_frame_count >= K3_FB_ESD_THRESHOLD) {
		k3fd->esd_frame_count = 0;
		return true;
	} else {
		return false;
	}
}

static int k3_fb_esd_set(struct k3_fb_data_type *k3fd)
{
	bool is_timeout = true;
	int delay_count = 0;
	u32 tmp = 0;

	/* check payload write empty */
	while (1) {
		tmp = inp32(k3fd->edc_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
		if (((tmp & 0x00000005) == 0x00000005) || delay_count > 100) {
			is_timeout = (delay_count > 100) ? true : false;
			delay_count = 0;
			break;
		} else {
			udelay(1);
			++delay_count;
		}
	}

	if (is_timeout) {
		k3fb_loge("ESD check payload write empty timeout.\n");
		return -EBUSY;
	}

	/* check dsi stop state */
	while (1) {
		tmp = inp32(k3fd->edc_base + MIPIDSI_PHY_STATUS_OFFSET);
		if (((tmp & 0xA90) == 0xA90) || delay_count > 100) {
			is_timeout = (delay_count > 100) ? true : false;
			delay_count = 0;
			break;
		} else {
			udelay(1);
			++delay_count;
		}
	};

	if (is_timeout) {
		k3fb_loge("ESD check dsi stop state timeout.\n");
		return -EBUSY;
	}

	/* reset DSI */
	set_reg(k3fd->edc_base + MIPIDSI_PWR_UP_OFFSET, 0x0, 1, 0);

	/* check clock lane stop state */
	while (1) {
		tmp = inp32(k3fd->edc_base + MIPIDSI_PHY_STATUS_OFFSET);
		if (((tmp & 0x04) == 0x04) || delay_count > 500) {
			is_timeout = (delay_count > 500) ? true : false;
			delay_count = 0;
			break;
		} else {
			udelay(1);
			++delay_count;
		}
	};

	if (is_timeout) {
		k3fb_loge("ESD check clock lane stop state timeout.\n");
		/* Should't return. MUST power on DSI */
	}

	/* power on DSI */
	set_reg(k3fd->edc_base + MIPIDSI_PWR_UP_OFFSET, 0x1, 1, 0);

	return 0;
}

static enum hrtimer_restart k3fb_cmd_esd(struct hrtimer *timer)
{
	struct k3_fb_data_type *k3fd = NULL;

	k3fd  = container_of(timer, struct k3_fb_data_type, esd_hrtimer);
	BUG_ON(k3fd == NULL);

	k3fd->esd_hrtimer_enable = true;
	hrtimer_start(&k3fd->esd_hrtimer, ktime_set(0, NSEC_PER_SEC), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

static int get_fps_for_temperature(struct k3_fb_data_type *k3fd)
{
	static int last_tem_level = 4;

	static int fps_temp[4] = {
		FAKE_FPS_TEM_NORM_FPS,
		FAKE_FPS_TEM_ABOVE_NORM_FPS,
		FAKE_FPS_TEM_HIGH,
		FAKE_FPS_TEM_THRD,
	};

	int app_fps = 0;
	int temp_fps = 0;
	int temp = 0;

	BUG_ON(k3fd == NULL);

	temp = k3fd->ambient_temperature;
	app_fps = k3fb_frc_get_fps(k3fd);

	/* adjust temperature to LEVEL */
	if (temp <= EVT_TEM_NORM) {
		temp = 0;
	} else if (temp < EVT_TEM_ABOVE_NORM) {
		temp = 2;
	} else if (temp < EVT_TEM_HIGH) {
		temp = 2;
	} else if (temp < EVT_TEM_THRD) {
		temp = 3;
	} else {
		temp = 3;
	}

	temp_fps = fps_temp[temp];

	/* if temperature up/nochange in current app frame rate, slow down the frame rate. */
	if (temp > 0 && temp >= last_tem_level) {
		last_tem_level = temp;
		if (temp_fps >= app_fps) {
			temp++;
			if (temp > 3) {
				temp = 3;
			}
			temp_fps = fps_temp[temp];
		}
	} else {
		last_tem_level = temp;
	}

	/* Benchmark mode */
	if (k3fd->frc_state & K3_FB_FRC_BENCHMARK_PLAYING) {
		temp_fps = K3_FB_FRC_BENCHMARK_FPS;
	} else if (k3fd->frc_state == K3_FB_FRC_NONE_PLAYING) {
		temp_fps = K3_FB_FRC_NORMAL_FPS;
	}

	return temp_fps;
}

static enum hrtimer_restart k3fb_fake_vsync(struct hrtimer *timer)
{
	struct k3_fb_data_type *k3fd = NULL;
	int fps = 0;
	int fps_for_temp = 60;

	k3fd = container_of(timer, struct k3_fb_data_type, fake_vsync_hrtimer);
	BUG_ON(k3fd == NULL);

	if (!k3fd->panel_power_on)
		goto error;

	if (k3fd->panel_info.frc_enable) {
		fps = k3fb_frc_get_fps(k3fd);
		if (k3fd->frc_state & K3_FB_FRC_SPECIAL_GAME_PLAYING) {
			fps = FAKE_FPS_SPECIAL_GAME;
		}
		fps_for_temp = get_fps_for_temperature(k3fd);

		/*k3fb_loge("---ambient_temperature=%d, fps=%d, fps_for_temp=%d ---\n",
			k3fd->ambient_temperature, fps, fps_for_temp);*/

		fps = (fps_for_temp >= fps) ? fps : fps_for_temp;
	} else {
		fps = K3_FB_FRC_NORMAL_FPS;
	}

	/*k3fb_loge("---fps=%d, active=%d---\n", fps, k3fd->vsync_info.active);*/
	if (k3fd->use_fake_vsync && k3fd->vsync_info.active && k3fd->vsync_info.thread) {
		k3fd->vsync_info.timestamp = ktime_get();
		wake_up_interruptible_all(&k3fd->vsync_info.wait);
	}

#if defined(CONFIG_OVERLAY_COMPOSE)
	/*for overlay idle detect (only cmd display mode)*/
	if(!k3fd->vsync_info.active && k3fd->panel_info.type == PANEL_MIPI_CMD){
		if(k3fd->ovcIdleCount < K3_FB_FRC_THRESHOLD){
			k3fd->ovcIdleCount++;
			if( k3fd->ovcIdleCount == K3_FB_FRC_THRESHOLD){
				u32 min_ddr_freq;

				k3fb_te_dec(k3fd, true, true);

				min_ddr_freq = k3_fb_overlay_compose_get_target_ddr_min_freq(0, 0);

				spin_lock(&gralloc_overlay_slock);
				if (min_ddr_freq < k3fd->ddr_min_freq && k3fd->ovc_ddr_wq){
					k3fd->ddr_min_freq = min_ddr_freq;
					queue_work(k3fd->ovc_ddr_wq, &k3fd->ovc_ddr_work);
				}
				spin_unlock(&gralloc_overlay_slock);
			}
		}
	}else{
		k3fd->ovcIdleCount = 0;
	}
#endif

error:
	hrtimer_start(&k3fd->fake_vsync_hrtimer, ktime_set(0, NSEC_PER_SEC / fps), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

/******************************************************************************
** handle isr
*/

static int edc_isr_cmd_mode(struct k3_fb_data_type *k3fd, u32 ints)
{
	/*
	** check interrupt
	** 0x80 for bas_stat_int
	** 0x40 for bas_end_int
	*/
	if ((ints & 0x40) == 0x40) {
		if (k3fd->cmd_mode_refresh) {
			k3fd->cmd_mode_refresh = false;

			k3fd->cmd_bl_can_set = true;
			if (k3fd->frame_end_wq)
				queue_work(k3fd->frame_end_wq, &k3fd->frame_end_work);
		}
	}

	if ((ints & 0x80) == 0x80) {
		if (!k3fd->cmd_mode_refresh) {
			k3fd->cmd_mode_refresh = true;

		if (k3fd->index == 0) {
			if (!k3fd->use_fake_vsync && k3fd->vsync_info.active && k3fd->vsync_info.thread) {
				k3fd->vsync_info.timestamp = ktime_get();
				wake_up_interruptible_all(&k3fd->vsync_info.wait);
			}
		}

		if (k3fd->panel_info.sbl_enable && k3fd->sbl_wq)
			queue_work(k3fd->sbl_wq, &k3fd->sbl_work);

		#if defined(CONFIG_OVERLAY_COMPOSE)
			k3_fb_gralloc_overlay_ch_process(k3fd);

		#endif

			k3fd->update_frame = 1;
			wake_up_interruptible(&k3fd->frame_wq);
		}
	}

	return IRQ_HANDLED;
}

static irqreturn_t edc_isr_video_mode(struct k3_fb_data_type *k3fd, u32 ints)
{
	/*
	** check interrupt
	** 0x80 for bas_stat_int
	** 0x40 for bas_end_int
	*/
	if ((ints & 0x80) == 0x80) {
	#if defined(CONFIG_OVERLAY_COMPOSE)
	        k3_fb_gralloc_overlay_ch_process(k3fd);
	#endif
		k3fd->update_frame = 1;
		wake_up_interruptible(&k3fd->frame_wq);

		if (k3fd->index == 0) {

			if (!k3fd->use_fake_vsync && k3fd->vsync_info.active && k3fd->vsync_info.thread) {
				k3fd->vsync_info.timestamp = ktime_get();
				wake_up_interruptible_all(&k3fd->vsync_info.wait);
			}

			if (k3fd->panel_info.sbl_enable && k3fd->sbl_wq)
				queue_work(k3fd->sbl_wq, &k3fd->sbl_work);

			if (k3fd->panel_info.frc_enable) {
				if (k3fb_frc_prepare(k3fd)) {
					k3fd->ldi_int_type |= FB_LDI_INT_TYPE_FRC;
				}
			}

			if (k3fd->panel_info.esd_enable) {
				if (k3fb_esd_prepare(k3fd)) {
					k3fd->ldi_int_type |= FB_LDI_INT_TYPE_ESD;
				}
			}

			if (k3fd->ldi_int_type != FB_LDI_INT_TYPE_NONE) {
				/* clear ldi interrupt */
				outp32(k3fd->edc_base + LDI_INT_CLR_OFFSET, 0xFFFFFFFF);

				/* enable vfrontporch_end_int */
				set_reg(k3fd->edc_base + LDI_INT_EN_OFFSET, 0x1, 1, 10);
				/* disable ldi */
				set_reg(k3fd->edc_base + LDI_CTRL_OFFSET, 0x0, 1, 0);
			}
		}
	}

	return IRQ_HANDLED;
}

static irqreturn_t edc_isr(int irq, void *data)
{
	struct k3_fb_data_type *k3fd = NULL;
	u32 tmp = 0;
	int ret = IRQ_HANDLED;

	k3fd = (struct k3_fb_data_type *)data;
	BUG_ON(k3fd == NULL);

	/* check edc_afifo_underflow_int interrupt */
	tmp = inp32(k3fd->edc_base + LDI_ORG_INT_OFFSET);
	if ((tmp & 0x4) == 0x4) {
		set_reg(k3fd->edc_base + LDI_INT_CLR_OFFSET, 0x1, 1, 2);
		k3fb_logw("edc_afifo_underflow_int !!!\n");
	}

	tmp = inp32(k3fd->edc_base + EDC_INTS_OFFSET);
	outp32(k3fd->edc_base + EDC_INTS_OFFSET, 0x0);

	if (k3fd->panel_info.type == PANEL_MIPI_CMD)
		ret = edc_isr_cmd_mode(k3fd, tmp);
	else
		ret = edc_isr_video_mode(k3fd, tmp);

	return ret;
}

static irqreturn_t ldi_isr(int irq, void *data)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_fb_panel_data *pdata = NULL;
	int ret = 0;
	u32 ints = 0;

	k3fd = (struct k3_fb_data_type *)data;
	BUG_ON(k3fd == NULL);
	pdata = (struct k3_fb_panel_data *)k3fd->pdev->dev.platform_data;
	BUG_ON(pdata == NULL);

	/* ldi interrupt state */
	ints = inp32(k3fd->edc_base + LDI_ORG_INT_OFFSET);
	/* clear ldi interrupt */
	outp32(k3fd->edc_base + LDI_INT_CLR_OFFSET, 0xFFFFFFFF);

	if (k3fd->index == 0) {
		/* check vfrontporch_end_int interrupt*/
		if ((ints & 0x400) == 0x400) {
			if (k3fd->panel_info.frc_enable &&
				(k3fd->ldi_int_type & FB_LDI_INT_TYPE_FRC)) {
				ret = k3fb_frc_set(k3fd);
				if (ret < 0) {
					k3fb_loge("failed to set frc.\n");
				}
			} else if (k3fd->panel_info.esd_enable &&
				(k3fd->ldi_int_type & FB_LDI_INT_TYPE_ESD)) {
				ret = k3_fb_esd_set(k3fd);
				if (ret < 0) {
					k3fb_loge("failed to set esd.\n");
				}
			}
		}/* else {
			k3fb_loge("interrupt(0x%x) is not used!\n", ints);
		}*/

		/* disable vfrontporch_end_int */
		set_reg(k3fd->edc_base + LDI_INT_EN_OFFSET, 0x0, 1, 10);
		/* enable ldi */
		set_reg(k3fd->edc_base + LDI_CTRL_OFFSET, 0x1, 1, 0);

		k3fd->ldi_int_type = FB_LDI_INT_TYPE_NONE;
	}

	return IRQ_HANDLED;
}


/******************************************************************************/

static int k3fb_overlay_get(struct fb_info *info, void __user *p)
{
	int ret = 0;
	struct overlay_info req;

	BUG_ON(info == NULL);

	if (copy_from_user(&req, p, sizeof(req))) {
		k3fb_loge("copy from user failed!\n");
		return -EFAULT;
	}

	ret = edc_overlay_get(info, &req);
	if (ret) {
		k3fb_loge("edc_overlay_get ioctl failed!\n");
		return ret;
	}

	if (copy_to_user(p, &req, sizeof(req))) {
		k3fb_loge("copy2user failed!\n");
		return -EFAULT;
	}

	return ret;
}

static int k3fb_overlay_set(struct fb_info *info, void __user *p)
{
	int ret = 0;
	struct overlay_info req;

	BUG_ON(info == NULL);

	if (copy_from_user(&req, p, sizeof(req))) {
		k3fb_loge("copy from user failed!\n");
		return -EFAULT;
	}

	ret = edc_overlay_set(info, &req);
	if (ret) {
		k3fb_loge("k3fb_overlay_set ioctl failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

static int k3fb_overlay_unset(struct fb_info *info, unsigned long *argp)
{
	int ret = 0;
	int ndx = 0;

	BUG_ON(info == NULL);

	ret = copy_from_user(&ndx, argp, sizeof(ndx));
	if (ret) {
		k3fb_loge("copy from user failed!\n");
		return ret;
	}

#if K3_FB_OVERLAY_USE_BUF
	if (video_buf.is_init && video_buf.is_video) {
		video_buf.is_video = false;
		mutex_lock(&video_buf.overlay_mutex);
		reset_video_buf();
		mutex_unlock(&video_buf.overlay_mutex);
	}
#endif

	return edc_overlay_unset(info, ndx);
}

static int k3fb_overlay_play(struct fb_info *info, unsigned long *argp)
{
	int ret = 0;
	struct overlay_data req;
	struct k3_fb_data_type *k3fd = NULL;

#if K3_FB_OVERLAY_USE_BUF
	static int count = 0;
	overlay_video_data *video_data = NULL;
	u32 video_index = 0;
	int i = 0;
	int min_count = 0;
	int set_count = 0;
#endif

	BUG_ON(info == NULL);

	ret = copy_from_user(&req, argp, sizeof(req));
	if (ret) {
		k3fb_loge("copy from user failed!\n");
		return ret;
	}

	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

#if K3_FB_OVERLAY_USE_BUF
	if ((k3fd->index == 1) && hdmi_is_connected && (req.src.is_video == 1) && should_use_videobuf(info)) {

		mutex_lock(&video_buf.overlay_mutex);
		if (!video_buf.is_video) {
			k3fb_logi("begin play video.\n");
		}
		video_buf.is_video = true;
		count ++;

		if (video_buf.last_addr == req.src.phy_addr) {
			mutex_unlock(&video_buf.overlay_mutex);
			k3fb_logi("same buf phys:0x%x.\n", req.src.phy_addr);
			return 0;
		}

		for (i = 0; i < MAX_OVERLAY_BUF_NUM; i++) {
			video_data = &video_buf.video_data_list[i];
			if (video_data->is_set) {
				set_count++;
			}

			if (0 == i || video_data->count < min_count) {
				min_count = video_data->count;
				video_index = i;
			}
		}
		
		if (MAX_OVERLAY_BUF_NUM == set_count) {
			k3fb_logw("video buff full @@@@@.\n");
		}

		video_data = &video_buf.video_data_list[video_index];
		memcpy(&video_data->data, &req, sizeof(req));
		video_data->is_set = true;
		video_data->count = count;

		video_buf.last_addr = req.src.phy_addr;
		mutex_unlock(&video_buf.overlay_mutex);
		return 0;
	} else if((k3fd->index == 1) && video_buf.is_video){ 
		k3fb_logi("exit play video req.is_video:%d buf.is_video:%d is_connected:%d\n",
			req.src.is_video, video_buf.is_video, hdmi_is_connected);

		video_buf.is_video = false;
		mutex_lock(&video_buf.overlay_mutex);
		count = 0;
		reset_video_buf();
		mutex_unlock(&video_buf.overlay_mutex);
	} 
#endif

#if defined(CONFIG_OVERLAY_COMPOSE)
	if (edc_overlay_compose_get_state(info, req.id)) {
		return k3_fb_overlay_compose_play(info, req);
	}
#endif //CONFIG_OVERLAY_COMPOSE

	if (k3fd->index == 1 && hdmi_is_connected && (!req.src.is_video || !should_use_videobuf(info))) {
		if (wait_event_interruptible_timeout(k3fd->frame_wq, k3fd->update_frame, HZ / 10) <= 0) {
			k3fb_logw("wait_event_interruptible_timeout !\n");
			k3fd->update_frame = 0;
			return -ETIME;
		}
		k3fd->update_frame = 0;
	}

	return edc_overlay_play(info, &req);
}

static int k3fb_vsync_int_set(struct fb_info *info, unsigned long *argp)
{
	int ret = 0;
	int vsync = 0;
	bool cur_vsync_event = false;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	ret = copy_from_user(&vsync, argp, sizeof(vsync));
	if (ret) {
		k3fb_loge("copy from user failed!\n");
		return ret;
	}

	cur_vsync_event = (vsync == 1) ? true : false;

	if (k3fd->vsync_info.active != cur_vsync_event) {
		k3fd->vsync_info.active = cur_vsync_event;

		if (k3fd->panel_info.type == PANEL_MIPI_CMD) {
			k3_fb_clk_enable_cmd_mode(k3fd);
			if (!k3fd->vsync_info.active) {
				k3fb_te_dec(k3fd, false, false);
			} else {
				k3fd->use_fake_vsync = true;

				// Clear idle count
				k3fd->ovcIdleCount = 0;

				k3fb_te_inc(k3fd, false, false);
			}
			k3_fb_clk_disable_cmd_mode(k3fd);
		}
		if (k3fd->vsync_info.active) {
			if (k3fd->frc_state == K3_FB_FRC_IDLE_PLAYING) {
				k3fd->frc_state = K3_FB_FRC_NONE_PLAYING;
				k3fd->frc_threshold_count = 0;
			}
		} else {
			if (k3fd->frc_state == K3_FB_FRC_NONE_PLAYING)
				k3fd->frc_state = K3_FB_FRC_IDLE_PLAYING;
		}
	}

	return ret;
}

static int k3fb_set_timing(struct fb_info *info, unsigned long *argp)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_fb_panel_data *pdata = NULL;
	struct fb_var_screeninfo var;
	u32 edc_base = 0;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	edc_base = k3fd->edc_base;

	pdata = (struct k3_fb_panel_data *)k3fd->pdev->dev.platform_data;
	if ((!pdata) || (!pdata->set_timing)) {
		k3fb_loge("no panel operation detected!\n");
		return -ENODEV;
	}

	if (!k3fd->panel_power_on) {
		k3fb_loge("panel power off!\n");
		return -EPERM;
	}

	ret = copy_from_user(&var, argp, sizeof(var));
	if (ret) {
		k3fb_loge("copy from user failed!\n");
		return ret;
	}

	memcpy(&info->var, &var, sizeof(var));
	k3fd->panel_info.xres = var.xres;
	k3fd->panel_info.yres = var.yres;

	k3fd->panel_info.clk_rate = (var.pixclock == 0) ? k3fd->panel_info.clk_rate : var.pixclock;
	k3fd->panel_info.ldi.h_front_porch = var.right_margin;
	k3fd->panel_info.ldi.h_back_porch = var.left_margin;
	k3fd->panel_info.ldi.h_pulse_width = var.hsync_len;
	k3fd->panel_info.ldi.v_front_porch = var.lower_margin;
	k3fd->panel_info.ldi.v_back_porch = var.upper_margin;
	k3fd->panel_info.ldi.v_pulse_width = var.vsync_len;
#ifdef HDMI_DISPLAY
	k3fd->panel_info.ldi.vsync_plr = hdmi_get_vsync_bycode(var.reserved[3]);
	k3fd->panel_info.ldi.hsync_plr = hdmi_get_hsync_bycode(var.reserved[3]);
#endif

	/* Note: call clk_set_rate after clk_set_rate, set edc clock rate to normal value */
	if (clk_set_rate(k3fd->edc_clk, EDC_CLK_RATE_GET(k3fd->panel_info.clk_rate)) != 0) {
		k3fb_loge("failed to set edc clk rate(%d).\n", EDC_CLK_RATE_GET(k3fd->panel_info.clk_rate));
	}

	set_EDC_DISP_SIZE(edc_base, k3fd->panel_info.xres, k3fd->panel_info.yres);

	if (pdata->set_timing(k3fd->pdev) != 0) {
		k3fb_loge("set timing failed!\n");
	}

	return 0;
}


/******************************************************************************/
void k3_fb_clk_enable_cmd_mode(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

#if 0
	if (k3fd->panel_info.type == PANEL_MIPI_CMD) {
		/* enable edc clk */
		if (clk_enable(k3fd->edc_clk) != 0) {
			k3fb_loge("failed to enable edc clk!\n");
		}

		/*enable ldi clk*/
		if (clk_enable(k3fd->ldi_clk) != 0) {
			k3fb_loge("failed to enable ldi clk!\n");
		}
	}
#endif
}

void k3_fb_clk_disable_cmd_mode(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

#if 0
	if (k3fd->panel_info.type == PANEL_MIPI_CMD) {
		/* disable ldi clk */
		clk_disable(k3fd->ldi_clk);
		/* disable edc clk */
		clk_disable(k3fd->edc_clk);
	}
#endif
}

static int k3_fb_esd_set_cmd_mode(struct fb_info *info)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	return ret;
}

static void k3_fb_set_backlight_cmd_mode(struct k3_fb_data_type *k3fd, u32 bkl_lvl)
{
	struct k3_fb_panel_data *pdata = NULL;
	int ret = 0;
	
	BUG_ON(k3fd == NULL);

	if (!k3fd->panel_power_on) {
		return;
	}

	pdata = (struct k3_fb_panel_data *)k3fd->pdev->dev.platform_data;
	if ((!pdata) || (!pdata->set_backlight)) {
		k3fb_loge("no panel operation detected!\n");
		return;
	}

	if (k3fd->cmd_mode_refresh)
		return;

	k3fd->bl_level = bkl_lvl;
	ret = pdata->set_backlight(k3fd->pdev);
	if (ret == 0) {
		k3fd->bl_level_cmd = k3fd->bl_level;
	} else {
		k3fb_loge("failed to set backlight.\n");
	}
}

void k3_fb_set_backlight(struct k3_fb_data_type *k3fd, u32 bkl_lvl)
{
	struct k3_fb_panel_data *pdata = NULL;
	int ret = 0;
	
	BUG_ON(k3fd == NULL);

	if (!k3fd->panel_power_on) {
		return;
	}

	pdata = (struct k3_fb_panel_data *)k3fd->pdev->dev.platform_data;
	if ((!pdata) || (!pdata->set_backlight)) {
		k3fb_loge("no panel operation detected!\n");
		return;
	}

	if (k3fd->panel_info.type == PANEL_MIPI_CMD) {
		if (k3fd->cmd_mode_refresh)
			return;
	}

	down(&k3_fb_backlight_sem);

	k3_fb_clk_enable_cmd_mode(k3fd);

	k3fd->bl_level = bkl_lvl;
	ret = pdata->set_backlight(k3fd->pdev);
	if (ret == 0) {
		k3fd->bl_level_cmd = k3fd->bl_level;
	} else {
		k3fb_loge("failed to set backlight.\n");
	}

	k3_fb_clk_disable_cmd_mode(k3fd);

	up(&k3_fb_backlight_sem);
}

static int k3_fb_blank_sub(int blank_mode, struct fb_info *info)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_fb_panel_data *pdata = NULL;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	pdata = (struct k3_fb_panel_data *)k3fd->pdev->dev.platform_data;
	if ((!pdata) || (!pdata->on) || (!pdata->off)) {
		k3fb_loge("no panel operation detected!\n");
		return -ENODEV;
	}

	down(&k3_fb_blank_sem);

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		if (!k3fd->panel_power_on) {
			edc_fb_resume(info);
			ret = pdata->on(k3fd->pdev);
			if (ret != 0) {
				k3fb_loge("failed to turn on sub devices!\n");
			} else {
				if (k3fd->panel_info.sbl_enable)
					sbl_ctrl_resume(k3fd);

				k3fd->panel_power_on = true;
				if (k3fd->panel_info.type != PANEL_MIPI_CMD)
					k3_fb_set_backlight(k3fd, k3fd->bl_level);

				k3_fb_clk_disable_cmd_mode(k3fd);

				if (k3fd->panel_info.type == PANEL_MIPI_CMD) {
					if (k3fd->panel_info.esd_enable) {
						k3fd->esd_hrtimer_enable = false;
						hrtimer_restart(&k3fd->esd_hrtimer);
					}
				}

				if (k3fd->index == 0) {
					hrtimer_restart(&k3fd->fake_vsync_hrtimer);
				}

				/* enable edc irq */
				if (k3fd->edc_irq)
					enable_irq(k3fd->edc_irq);
				/* enable ldi irq */
				if (k3fd->ldi_irq)
					enable_irq(k3fd->ldi_irq);
			}
		}
		break;

	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
	case FB_BLANK_POWERDOWN:
	default:
		if (k3fd->panel_power_on) {
			bool curr_pwr_state = false;

			curr_pwr_state = k3fd->panel_power_on;
			k3fd->panel_power_on = false;

			/* disable edc irq */
			if (k3fd->edc_irq)
				disable_irq(k3fd->edc_irq);
			/* disable ldi irq*/
			if (k3fd->ldi_irq)
				disable_irq(k3fd->ldi_irq);

			if (k3fd->panel_info.type == PANEL_MIPI_CMD) {
				if (!k3fd->cmd_mode_refresh ) {
					k3_fb_clk_enable_cmd_mode(k3fd);
				}

				k3fd->cmd_mode_refresh = false;
				k3fd->cmd_bl_can_set = false;
				k3fd->bl_level_cmd = 0;

				if (k3fd->panel_info.esd_enable) {
					k3fd->esd_hrtimer_enable = false;
					hrtimer_cancel(&k3fd->esd_hrtimer);
				}
			}

			if (k3fd->index == 0) {
				k3fd->use_fake_vsync = false;
				hrtimer_cancel(&k3fd->fake_vsync_hrtimer);
				k3fd->ambient_temperature = 0;
			}

			/* Set frame rate back to 60 */
			if (k3fd->panel_info.frc_enable) {
				k3fd->frc_state = K3_FB_FRC_NONE_PLAYING;
				k3fb_frc_set(k3fd);
			}

			ret = pdata->off(k3fd->pdev);
			if (ret != 0) {
				k3fb_loge("failed to turn off sub devices!\n");
				k3fd->panel_power_on = curr_pwr_state;
			} else {
				edc_fb_suspend(info);
			}
		}
		break;
	}

	up(&k3_fb_blank_sem);

	return ret;
}

u32 k3_fb_line_length(u32 xres, int bpp)
{
#ifdef CONFIG_FB_STRIDE_64BYTES_ODD_ALIGN
	u32 stride = ALIGN_UP(xres * bpp, 64);
	return (((stride / 64) % 2 == 0) ? (stride + 64) : stride);
#else
	return xres * bpp;
#endif
}


/******************************************************************************/

static int k3_fb_open(struct fb_info *info, int user)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

#ifdef CONFIG_FASTBOOT_DISP_ENABLE
	if (k3fd->index == 0) {
		if (k3fd->ref_cnt == 1) {
			memset(info->screen_base, 0x0, info->fix.smem_len);
		}
	}
#endif

	if (!k3fd->ref_cnt) {
		k3fb_logi("index=%d, enter!\n", k3fd->index);
		ret = k3_fb_blank_sub(FB_BLANK_UNBLANK, info);
		if (ret != 0) {
			k3fb_loge("can't turn on display!\n");
			return ret;
		}
		k3fb_logi("index=%d, exit!\n", k3fd->index);
	}

	k3fd->ref_cnt++;

	return ret;
}

static int k3_fb_release(struct fb_info *info, int user)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	if (!k3fd->ref_cnt) {
		k3fb_loge("try to close unopened fb %d!\n", k3fd->index);
		return -EINVAL;
	}

	k3fd->ref_cnt--;

	if (!k3fd->ref_cnt) {
		k3fb_logi("index=%d, enter!\n", k3fd->index);
		ret = k3_fb_blank_sub(FB_BLANK_POWERDOWN, info);
		if (ret != 0) {
			k3fb_loge("can't turn off display!\n");
			return ret;
		}
		k3fb_logi("index=%d, exit!\n", k3fd->index);
	}
#if defined(CONFIG_OVERLAY_COMPOSE)
	if (k3fd->index == 0) {
		if (k3fd->ref_cnt == 1) {
			k3_fb_overlay_compose_reset_pipe(info);
		}
	}
#endif /* CONFIG_OVERLAY_COMPOSE */


	return ret;
}

static int k3_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;
	/*u32 max_xres = 0;
	u32 max_yres = 0;*/

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	if (var->rotate != FB_ROTATE_UR) {
		k3fb_loge("rotate %d!\n", var->rotate);
		return -EINVAL;
	}

	if (var->grayscale != info->var.grayscale) {
		k3fb_loge("grayscale %d!\n", var->grayscale);
		return -EINVAL;
	}

	switch (var->bits_per_pixel) {
	/* RGB565 RGBA5551 RGBX5551*/
	case 16:
		{
			if (var->blue.offset == 0) {
				if (var->red.offset == 10) {
					if ((var->transp.offset != 15) ||
						(var->green.length != 5) ||
						((var->transp.length != 1) && (var->transp.length != 0))) {
						k3fb_loge("not match  RGBA5551 and RGBX5551!\n");
						return -EINVAL;
					}
				} else if (var->red.offset == 11) {
					if ((var->transp.offset != 0) ||
						(var->green.length != 6) ||
						(var->transp.length != 0)) {
						k3fb_loge("not match  RGB565!\n");
						return -EINVAL;
					}
				} else {
					k3fb_loge("not match  RGB565, RGBA5551 or RGBX5551!\n");
					return -EINVAL;
				}
			} else if (var->blue.offset == 10) {
				if ((var->red.offset != 0) ||
					(var->transp.offset != 15) ||
					(var->green.length != 5) ||
					((var->transp.length != 1) && (var->transp.length != 0))) {
					k3fb_loge("1 not match  BGRA5551 and BGRX5551!\n");
					return -EINVAL;
				}
			} else if (var->blue.offset == 11) {
				if ((var->red.offset != 0) ||
					(var->transp.offset != 0) ||
					(var->green.length != 6) ||
					(var->transp.length != 0)) {
					k3fb_loge("not match  BGR565!\n");
					return -EINVAL;
				}
			} else {
				k3fb_loge("2 not match  RGB565, RGBA5551 or RGBX5551!\n");
				return -EINVAL;
			}

			/* Check the common values for RGB565, RGBA5551 and RGBX5551 */
			if ((var->green.offset != 5) ||
				(var->blue.length != 5) ||
				(var->red.length != 5) ||
				(var->blue.msb_right != 0) ||
				(var->green.msb_right != 0) ||
				(var->red.msb_right != 0) ||
				(var->transp.msb_right != 0)) {
				k3fb_loge("3 not match  RGB565, RGBA5551 or RGBX5551!\n");
				return -EINVAL;
			}
		}
		break;
	/* RGBA8888 RGBX8888*/
	case 32:
		{
			if (var->blue.offset == 0) {
				if (var->red.offset != 16) {
					k3fb_loge("not match EDC_RGB, bpp=32!\n");
					return -EINVAL;
				}
			} else if (var->blue.offset == 16) {
				if (var->red.offset != 0) {
					k3fb_loge("not match EDC_BGR, bpp=32!\n");
					return -EINVAL;
				}
			} else {
				k3fb_loge("1 not match RGBA8888 or RGBX8888!\n");
				return -EINVAL;
			}

			/* Check the common values for RGBA8888 and RGBX8888 */
			if ((var->green.offset != 8) ||
				(var->transp.offset != 24) ||
				(var->blue.length != 8) ||
				(var->green.length != 8) ||
				(var->red.length != 8) ||
				((var->transp.length != 8) && (var->transp.length != 0)) ||
				(var->blue.msb_right != 0) ||
				(var->green.msb_right != 0) ||
				(var->red.msb_right != 0) ||
				(var->transp.msb_right != 0)) {
				k3fb_loge("2 not match RGBA8888 or RGBX8888!\n");
				return -EINVAL;
			}
		}
		break;
	default:
		{
			k3fb_loge("bits_per_pixel=%d not supported!\n", var->bits_per_pixel);
			return -EINVAL;
		}
		break;
	}

	if ((var->xres_virtual < K3_FB_MIN_WIDTH) || (var->yres_virtual < K3_FB_MIN_HEIGHT)) {
		k3fb_loge("xres_virtual=%d yres_virtual=%d out of range!", var->xres_virtual, var->yres_virtual);
		return -EINVAL;
	}

	/*max_xres = MIN(var->xres_virtual, K3_FB_MAX_WIDTH);
	max_yres = MIN(var->yres_virtual, K3_FB_MAX_HEIGHT);
	if ((var->xres < K3_FB_MIN_WIDTH) ||
		(var->yres < K3_FB_MIN_HEIGHT) ||
		(var->xres > max_xres) ||
		(var->yres > max_yres)) {
		k3fb_loge("xres=%d yres=%d out of range!\n", var->xres, var->yres);
		return -EINVAL;
	}*/

	if (var->xoffset > (var->xres_virtual - var->xres)) {
		k3fb_loge("xoffset=%d out of range!\n", var->xoffset);
		return -EINVAL;
	}

	if (var->yoffset > (var->yres_virtual - var->yres)) {
		k3fb_loge("yoffset=%d out of range!\n", var->yoffset);
		return -EINVAL;
	}

	if (info->fix.smem_len < (k3_fb_line_length(var->xres_virtual, (var->bits_per_pixel >> 3))  * var->yres)) {
		k3fb_loge("smem_len=%d out of range!\n", info->fix.smem_len);
		return -EINVAL;
	}

	return ret;
}

static int k3_fb_set_par(struct fb_info *info)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct fb_var_screeninfo *var = NULL;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	var = &info->var;
	switch (var->bits_per_pixel) {
	case 16:
		{
			if (var->blue.offset == 0) {
				k3fd->fb_bgrFmt = EDC_RGB;
				if (var->red.offset == 11)
					k3fd->fb_imgType = EDC_RGB_565;
				else
					k3fd->fb_imgType = EDC_ARGB_1555;
			} else {
				k3fd->fb_bgrFmt = EDC_BGR;
				if (var->blue.offset == 11)
					k3fd->fb_imgType = EDC_RGB_565;
				else
					k3fd->fb_imgType = EDC_ARGB_1555;
			}
		}
		break;
	case 32:
		{
			if (var->blue.offset == 0) {
				k3fd->fb_bgrFmt = EDC_RGB;
			} else {
				k3fd->fb_bgrFmt = EDC_BGR;
			}

			if (var->transp.length == 8)
				k3fd->fb_imgType = EDC_ARGB_8888;
			else
				k3fd->fb_imgType = EDC_XRGB_8888;
		}
		break;
	default:
		k3fb_loge("bits_per_pixel=%d not supported!\n", var->bits_per_pixel);
		return -EINVAL;
	}

    /*line_length will not change width when fb_imgType changing. For FB format Change dynamicly*/
	/*k3fd->fbi->fix.line_length = k3_fb_line_length(var->xres_virtual, var->bits_per_pixel >> 3);*/

	if (info->fix.xpanstep)
		info->var.xoffset = (var->xoffset / info->fix.xpanstep) * info->fix.xpanstep;

	if (info->fix.ypanstep)
		info->var.yoffset = (var->yoffset / info->fix.ypanstep) * info->fix.ypanstep;

	return 0;
}

static int k3_fb_pan_display_cmd_mode(struct fb_var_screeninfo *var, 
	struct fb_info *info)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	k3_fb_clk_enable_cmd_mode(k3fd);

	ret = edc_fb_pan_display(var, info, k3fd->graphic_ch);
	if (ret != 0) {
		k3fb_loge("edc_fb_pan_display err!\n");
	}

	// Clear idle count
	k3fd->ovcIdleCount = 0;

	k3fb_te_inc(k3fd, true, false);

	if (k3fd->frc_state != K3_FB_FRC_IDLE_PLAYING)
		k3fd->use_fake_vsync = k3fb_frc_get_fps(k3fd) < K3_FB_FRC_NORMAL_FPS ? true : false;

#if K3_FB_OVERLAY_USE_BUF 
	if (!hdmi_is_connected || video_buf.is_video) {
#else
	if (!hdmi_is_connected) {
#endif
		if ((wait_event_interruptible_timeout(k3fd->frame_wq, k3fd->update_frame, HZ / 10) <= 0)||(k3fd->esd_recover == true)) {
			if (k3fd->panel_info.esd_enable) {
				if (k3_fb_blank_sub(FB_BLANK_POWERDOWN, info) != 0) {
					k3fb_loge("blank mode %d failed!\n", FB_BLANK_POWERDOWN);
				}
				if (k3_fb_blank_sub(FB_BLANK_UNBLANK, info) != 0) {
					k3fb_loge("blank mode %d failed!\n", FB_BLANK_UNBLANK);
				}
				k3fb_loge("ESD recover!\n");
				k3fd->esd_recover = false;
			}
			k3fb_logw("wait_event_interruptible_timeout !\n");
			k3fb_te_dec(k3fd, false, false);
			k3fd->update_frame = 0;
			return -ETIME;
		}
		k3fd->update_frame = 0;
	}

	k3fb_te_dec(k3fd, false, false);

	return ret;
}

static int k3_fb_pan_display_video_mode(struct fb_var_screeninfo *var, 
	struct fb_info *info)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

#if K3_FB_OVERLAY_USE_BUF 
	if (!hdmi_is_connected || video_buf.is_video) {
#else
	if (!hdmi_is_connected) {
#endif
		if (wait_event_interruptible_timeout(k3fd->frame_wq, k3fd->update_frame, HZ / 10) <= 0) {
			k3fb_logw("wait_event_interruptible_timeout !\n");
			k3fd->update_frame = 0;
			return -ETIME;
		}
		k3fd->update_frame = 0;
	}

	ret = edc_fb_pan_display(var, info, k3fd->graphic_ch);
	if (ret != 0) {
		k3fb_logw("edc_fb_pan_display err!\n");
	}

	return ret;
}

static int k3_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;
#if defined(CONFIG_OVERLAY_COMPOSE)
	int ovc_partial;
#endif

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	if (!k3fd->panel_power_on) {
		return -EPERM;
	}

#if defined(CONFIG_OVERLAY_COMPOSE)
	ovc_partial = k3_fb_overlay_compose_partial_state(info);
	if (ovc_partial) {
		return k3_fb_overlay_compose_partial_play(var, info, ovc_partial);
	}

#endif /* CONFIG_OVERLAY_COMPOSE */

	if (k3fd->panel_info.type == PANEL_MIPI_CMD)
		ret = k3_fb_pan_display_cmd_mode(var, info);
	else
		ret = k3_fb_pan_display_video_mode(var, info);

	if (k3fd->temp_wq)
		queue_work(k3fd->temp_wq, &k3fd->temp_work);

	return ret;
}

static int k3_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = 0;

	BUG_ON(info == NULL);


	down(&k3_fb_overlay_sem);
	switch (cmd) {
	case K3FB_OVERLAY_GET:
		ret = k3fb_overlay_get(info, argp);
		break;
	case K3FB_OVERLAY_SET:
		ret = k3fb_overlay_set(info, argp);
		break;
	case K3FB_OVERLAY_UNSET:
		ret = k3fb_overlay_unset(info, argp);
		break;
	case K3FB_OVERLAY_PLAY:
		ret = k3fb_overlay_play(info, argp);
		break;
	case K3FB_VSYNC_INT_SET:
		ret = k3fb_vsync_int_set(info, argp);
		break;
	case K3FB_TIMING_SET:
		ret = k3fb_set_timing(info, argp);
		break;
	case K3FB_FRC_SET:
		ret = k3fb_frc_set_state(info, argp);
		break;
	case K3FB_G2D_SET_FREQ:
		ret = k3fb_g2d_set_freq(info, argp);
		break;
	case K3FB_SBL_SET_VALUE:
		ret = k3fb_sbl_set_value(info, argp);
		break;
#if defined(CONFIG_OVERLAY_COMPOSE)
	case K3FB_OVC_CHECK_DDR_FREQ:
		ret = k3fb_ovc_check_ddr_freq(info, argp);
		break;
#endif /* CONFIG_OVERLAY_COMPOSE */
	default:
		k3fb_loge("unknown ioctl (cmd=%d) received!\n", cmd);
		ret = -EINVAL;
		break;
	}

	up(&k3_fb_overlay_sem);
	return ret;
}

static int k3_fb_blank(int blank_mode, struct fb_info *info)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(info == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	k3fb_logd("index=%d, enter!\n", k3fd->index);

	ret = k3_fb_blank_sub(blank_mode, info);
	if (ret != 0) {
		k3fb_loge("blank mode %d failed!\n", blank_mode);
	}

	k3fb_logd("index=%d, exit!\n", k3fd->index);

	return ret;
}


/******************************************************************************/

static struct fb_ops k3_fb_ops = {
	.owner = THIS_MODULE,
	.fb_open = k3_fb_open,
	.fb_release = k3_fb_release,
	.fb_read = NULL,
	.fb_write = NULL,
	.fb_cursor = NULL,
	.fb_check_var = k3_fb_check_var,  /* vinfo check */
	.fb_set_par = k3_fb_set_par,  /* set the video mode according to info->var */
	.fb_setcolreg = NULL,  /* set color register */
	.fb_blank = k3_fb_blank, /*blank display */
	.fb_pan_display = k3_fb_pan_display,  /* pan display */
	.fb_fillrect = NULL,  /* Draws a rectangle */
	.fb_copyarea = NULL,  /* Copy data from area to another */
	.fb_imageblit = NULL,  /* Draws a image to the display */
	.fb_rotate = NULL,
	.fb_sync = NULL,  /* wait for blit idle, optional */
	.fb_ioctl = k3_fb_ioctl,  /* perform fb specific ioctl (optional) */
	.fb_mmap = NULL,
};

static void k3_fb_set_bl_brightness(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct k3_fb_data_type *k3fd = dev_get_drvdata(led_cdev->dev->parent);
	int bl_lvl;

	if (value > MAX_BACKLIGHT_BRIGHTNESS)
		value = MAX_BACKLIGHT_BRIGHTNESS;

	bl_lvl = value;
	k3fd->bl_level = value;
	k3fd->bl_level_sbl = value;

	if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		/*
		** This maps android backlight level 0 to 255 into
		** driver backlight level 0 to bl_max with rounding
		** bl_lvl = (2 * value * k3fd->panel_info.bl_max + MAX_BACKLIGHT_BRIGHTNESS)
		** (2 * MAX_BACKLIGHT_BRIGHTNESS);
		*/
		bl_lvl = (value * k3fd->panel_info.bl_max) / MAX_BACKLIGHT_BRIGHTNESS;
		bl_lvl &= 0xFF;

		if (!bl_lvl && value)
			bl_lvl = 1;

		k3fd->bl_level = bl_lvl;
		k3fd->bl_level_sbl = bl_lvl;
	}

	if ((k3fd->panel_info.type == PANEL_MIPI_CMD)
		&& (!k3fd->cmd_bl_can_set))
			return;

	if (k3fd->panel_info.sbl_enable)
		sbl_bkl_set(k3fd, bl_lvl);
	k3_fb_set_backlight(k3fd, k3fd->bl_level);
}

static struct led_classdev backlight_led = {
	.name = DEV_NAME_LCD_BKL,
	.brightness = MAX_BACKLIGHT_BRIGHTNESS,
	.brightness_set = k3_fb_set_bl_brightness,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void k3fb_early_suspend(struct early_suspend *h)
{
	struct k3_fb_data_type *k3fd = container_of(h, struct k3_fb_data_type, early_suspend);

	BUG_ON(k3fd == NULL);

	k3fb_logi("index=%d, enter!\n", k3fd->index);

//	msleep(500);

	if (k3fd->index == 0) {
		if (k3_fb_blank_sub(FB_BLANK_POWERDOWN, k3fd->fbi) != 0) {
			k3fb_loge("failed to early suspend!\n");
		}
	} else if (k3fd->panel_power_on) {
		edc_fb_disable(k3fd->fbi);
	}

	k3fb_logi("index=%d, exit!\n", k3fd->index);
}

static void k3fb_late_resume(struct early_suspend *h)
{
	struct k3_fb_data_type *k3fd = container_of(h, struct k3_fb_data_type, early_suspend);

	BUG_ON(k3fd == NULL);

	k3fb_logi("index=%d, enter!\n", k3fd->index);

	if (k3fd->index == 0) {
		if (k3_fb_blank_sub(FB_BLANK_UNBLANK, k3fd->fbi) != 0) {
			k3fb_loge("failed to late resume!\n");
		}
	} else if (k3fd->panel_power_on) {
		edc_fb_enable(k3fd->fbi);
	}

	k3fb_logi("index=%d, exit!\n", k3fd->index);
}
#endif

static int k3_fb_init_par(struct k3_fb_data_type *k3fd, int pixel_fmt)
{
	struct fb_info *fbi = NULL;
	struct fb_var_screeninfo *var = NULL;
	struct fb_fix_screeninfo *fix = NULL;
	int bpp = 0;

	BUG_ON(k3fd == NULL);

	fbi = k3fd->fbi;
	fix = &fbi->fix;
	var = &fbi->var;

	fix->type_aux = 0;	/* if type == FB_TYPE_INTERLEAVED_PLANES */
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->visual = FB_VISUAL_TRUECOLOR;	/* True Color */
	fix->xpanstep = 1;
	fix->ypanstep = 1;
	fix->ywrapstep = 0;  /* No support */
	fix->mmio_start = 0;  /* No MMIO Address */
	fix->mmio_len = 0;	/* No MMIO Address */
	fix->accel = FB_ACCEL_NONE;  /* No hardware accelerator */

	var->xoffset = 0;  /* Offset from virtual to visible */
	var->yoffset = 0;  /* resolution */
	var->grayscale = 0;  /* No graylevels */
	var->nonstd = 0;  /* standard pixel format */
	/*var->activate = FB_ACTIVATE_NOW;*/
	var->activate = FB_ACTIVATE_VBL;  /* activate it at vsync */
	var->height = k3fd->panel_info.height;  /* height of picture in mm */
	var->width = k3fd->panel_info.width;  /* width of picture in mm */
	var->accel_flags = 0;  /* acceleration flags */
	var->sync = 0;	 /* see FB_SYNC_* */
	var->rotate = 0;   /* angle we rotate counter clockwise */
	var->vmode = FB_VMODE_NONINTERLACED;

	switch (pixel_fmt) {
	case IMG_PIXEL_FORMAT_RGBX_5551:
		var->blue.offset = 0;
		var->green.offset = 5;
		var->red.offset = 10;
		var->transp.offset = 15;
		var->blue.length = 5;
		var->green.length = 5;
		var->red.length = 5;
		var->transp.length = 0;
		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;
		bpp = 2;
		k3fd->fb_imgType = EDC_ARGB_1555;
		break;
	case IMG_PIXEL_FORMAT_RGBA_5551:
		var->blue.offset = 0;
		var->green.offset = 5;
		var->red.offset = 10;
		var->transp.offset = 15;
		var->blue.length = 5;
		var->green.length = 5;
		var->red.length = 5;
		var->transp.length = 1;
		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;
		bpp = 2;
		k3fd->fb_imgType = EDC_ARGB_1555;
		break;
	case IMG_PIXEL_FORMAT_RGB_565:
		var->blue.offset = 0;
		var->green.offset = 5;
		var->red.offset = 11;
		var->transp.offset = 0;
		var->blue.length = 5;
		var->green.length = 6;
		var->red.length = 5;
		var->transp.length = 0;
		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;
		bpp = 2;
		k3fd->fb_imgType = EDC_RGB_565;
		break;
	case IMG_PIXEL_FORMAT_RGBX_8888:
		var->blue.offset = 0;
		var->green.offset = 8;
		var->red.offset = 16;
		var->transp.offset = 24;
		var->blue.length = 8;
		var->green.length = 8;
		var->red.length = 8;
		var->transp.length = 0;
		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;
		bpp = 4;
		k3fd->fb_imgType = EDC_XRGB_8888;
		break;
	case IMG_PIXEL_FORMAT_RGBA_8888:
		var->blue.offset = 0;
		var->green.offset = 8;
		var->red.offset = 16;
		var->transp.offset = 24;
		var->blue.length = 8;
		var->green.length = 8;
		var->red.length = 8;
		var->transp.length = 8;
		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;
		bpp = 4;
		k3fd->fb_imgType = EDC_ARGB_8888;
		break;
	default:
		k3fb_loge("index=%d, unkown image type!\n", k3fd->index);
		return -EINVAL;
	}

	var->xres = k3fd->panel_info.xres;
	var->yres = k3fd->panel_info.yres;
	var->xres_virtual = k3fd->panel_info.xres;
	var->yres_virtual = k3fd->panel_info.yres * K3_NUM_FRAMEBUFFERS;
	var->bits_per_pixel = bpp * 8;

	fix->line_length = k3_fb_line_length(var->xres_virtual, bpp);
	fix->smem_len = roundup(fix->line_length * var->yres_virtual, PAGE_SIZE);

	/* id field for fb app */
	snprintf(fix->id, sizeof(fix->id), "k3fb%d", k3fd->index);

	fbi->fbops = &k3_fb_ops;
	fbi->flags = FBINFO_FLAG_DEFAULT; /* FBINFO_HWACCEL_YPAN | FBINFO_HWACCEL_XPAN */
	fbi->pseudo_palette = k3_fb_pseudo_palette;

	if (k3fd->index == 0) {
		fbi->fix.smem_start = (int32_t)K3_FB_PA;
		fbi->screen_size = fix->smem_len;
		fbi->screen_base = ioremap(fbi->fix.smem_start, fbi->fix.smem_len);
		/*memset(fbi->screen_base, 0x0, fbi->fix.smem_len);*/
	} else {
		fbi->fix.smem_start = fbi_list[0]->fix.smem_start;
		fbi->screen_size = fbi_list[0]->screen_size;
		fbi->screen_base = fbi_list[0]->screen_base;
	}

	return 0;
}

static int k3_fb_register(struct k3_fb_data_type *k3fd)
{
	int ret = 0;
#ifdef CONFIG_FASTBOOT_DISP_ENABLE
	struct k3_fb_panel_data *pdata = NULL;
#endif
	struct fb_info *fbi = NULL;
	struct fb_fix_screeninfo *fix = NULL;

	BUG_ON(k3fd == NULL);

	/* fb info initialization */
	fbi = k3fd->fbi;
	fix = &fbi->fix;

	ret = k3_fb_init_par(k3fd, IMG_PIXEL_FORMAT_RGBA_8888);
	if (ret != 0) {
		k3fb_loge("index=%d, k3_fb_init_par failed!\n", k3fd->index);
		return ret;
	}

	/* init edc overlay, only intialize one time */
	edc_overlay_init(&k3fd->ctrl);

	k3fd->fb_bgrFmt = K3FB_DEFAULT_BGR_FORMAT;
	k3fd->ref_cnt = 0;
	k3fd->panel_power_on = false;
	sema_init(&k3fd->sem, 1);
	if (!(k3fd->panel_info.bl_set_type & BL_SET_BY_PWM)) {
		k3fd->bl_level = 102;
	} else {
		k3fd->bl_level = 40;
	}
	k3fd->ldi_int_type = FB_LDI_INT_TYPE_NONE;
	k3fd->cmd_mode_refresh = false;
	k3fd->cmd_bl_can_set = false;
	k3fd->ambient_temperature = 0;

	k3fd->update_frame = 0;
	init_waitqueue_head(&k3fd->frame_wq);

	/* for primary lcd */
	if (k3fd->index == 0) {
		if (k3fd->panel_info.sbl_enable) {
			init_sbl_workqueue(k3fd);
		}

		if (k3fd->panel_info.frc_enable) {
			k3fd->frc_threshold_count = 0;
			k3fd->frc_state = K3_FB_FRC_NONE_PLAYING;
			k3fd->frc_timestamp = jiffies;
		}

		/* for vsync */
		memset(&(k3fd->vsync_info), 0, sizeof(k3fd->vsync_info));

		init_waitqueue_head(&k3fd->vsync_info.wait);
		k3fd->vsync_info.thread = kthread_run(k3fb_wait_for_vsync_thread, k3fd, "k3fb-vsync");
		if (IS_ERR(k3fd->vsync_info.thread)) {
			k3fb_loge("failed to run vsync thread!\n");
			k3fd->vsync_info.thread = NULL;
		}

		spin_lock_init(&k3fd->vsync_info.irq_lock);
		k3fd->use_fake_vsync = false;

		/* for temperature obtaining*/
		init_temperature_workqueue(k3fd);

		/* hrtimer for fake vsync timing */
		hrtimer_init(&k3fd->fake_vsync_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		k3fd->fake_vsync_hrtimer.function = k3fb_fake_vsync;
		hrtimer_start(&k3fd->fake_vsync_hrtimer, ktime_set(0, NSEC_PER_SEC / 60), HRTIMER_MODE_REL);

		if (k3fd->panel_info.esd_enable) {
			if (k3fd->panel_info.type == PANEL_MIPI_CMD) {
				init_frame_end_workqueue(k3fd);

				/* hrtimer for ESD timing */
				hrtimer_init(&k3fd->esd_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
				k3fd->esd_hrtimer.function = k3fb_cmd_esd;
				k3fd->esd_hrtimer_enable = false;
				hrtimer_start(&k3fd->esd_hrtimer, ktime_set(0, NSEC_PER_SEC), HRTIMER_MODE_REL);
			} else {
				k3fd->esd_timestamp = jiffies;
				k3fd->esd_frame_count = 0;
			}
		}
	} else if (k3fd->index == 1) {
	#if K3_FB_OVERLAY_USE_BUF
		overlay_play_work(k3fd);
	#endif
	} else {
		k3fb_loge("fb%d not support now!\n", k3fd->index);
	}

#ifdef CONFIG_FASTBOOT_DISP_ENABLE
	if (k3fd->index == 0) {
		pdata = (struct k3_fb_panel_data *)k3fd->pdev->dev.platform_data;
		if (pdata && pdata->set_fastboot) {
			pdata->set_fastboot(k3fd->pdev);
		}

		if (k3fd->panel_info.type == PANEL_MIPI_CMD) {
			set_MIPIDSI_TE_CTRL_te_mask_en(k3fd->edc_base, 1);

		}

		set_EDC_INTE(k3fd->edc_base, 0xFFFFFFFF);
		set_EDC_INTS(k3fd->edc_base, 0x0);
		set_EDC_INTE(k3fd->edc_base, 0xFFFFFF3F);

		k3fd->ref_cnt++;
		k3fd->panel_power_on = true;

		k3_fb_clk_disable_cmd_mode(k3fd);
	}
#endif

	/* request edc irq */
	snprintf(k3fd->edc_irq_name, sizeof(k3fd->edc_irq_name), "%s_edc", fix->id);
	ret = request_irq(k3fd->edc_irq, edc_isr, IRQF_SHARED, k3fd->edc_irq_name, (void *)k3fd);
	if (ret != 0) {
		k3fb_loge("index=%d unable to request edc irq\n", k3fd->index);
	}

	if (k3fd->index == 1)
		disable_irq(k3fd->edc_irq);

	/* register edc_irq to core 1 */
	k3v2_irqaffinity_register(k3fd->edc_irq, 1);

	/* request ldi irq */
	snprintf(k3fd->ldi_irq_name, sizeof(k3fd->ldi_irq_name), "%s_ldi", fix->id);
	ret = request_irq(k3fd->ldi_irq, ldi_isr, IRQF_SHARED, k3fd->ldi_irq_name, (void *)k3fd);
	if (ret != 0) {
		k3fb_loge("index=%d unable to request ldi irq\n", k3fd->index);
	}

	if (k3fd->index == 1)
		disable_irq(k3fd->ldi_irq);

	/* register ldi_irq to core 1 */
	k3v2_irqaffinity_register(k3fd->ldi_irq, 1);

#ifdef CONFIG_HAS_EARLYSUSPEND
	k3fd->early_suspend.suspend = k3fb_early_suspend;
	k3fd->early_suspend.resume = k3fb_late_resume;
	k3fd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 2;
	register_early_suspend(&k3fd->early_suspend);
#endif

	/* register framebuffer */
	if (register_framebuffer(fbi) < 0) {
		k3fb_loge("not able to register framebuffer %d!\n", k3fd->index);
		return -EPERM;
	}

	return ret;
}

/******************************************************************************/
static int k3_fb_probe(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct resource *res = 0;
	int ret = 0;

	if ((pdev->id == 0) && (pdev->num_resources > 0)) {
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, IRQ_EDC0_NAME);
		if (!res) {
			k3fb_loge("failed to get irq_edc0 resource.\n");
			return -ENXIO;
		}
		k3fd_irq_edc0 = res->start;

		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, IRQ_EDC1_NAME);
		if (!res) {
			k3fb_loge("failed to get irq_edc1 resource.\n");
			return -ENXIO;
		}
		k3fd_irq_edc1 = res->start;

		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, IRQ_LDI0_NAME);
		if (!res) {
			k3fb_loge("failed to get irq_ldi0 resource.\n");
			return -ENXIO;
		}
		k3fd_irq_ldi0 = res->start;

		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, IRQ_LDI1_NAME);
		if (!res) {
			k3fb_loge("failed to get irq_ldi1 resource.\n");
			return -ENXIO;
		}
		k3fd_irq_ldi1 = res->start;

		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, REG_BASE_EDC0_NAME);
		if (!res) {
			k3fb_loge("failed to get reg_base_edc0 resource.\n");
			return -ENXIO;
		}
		k3fd_reg_base_edc0 = IO_ADDRESS(res->start);

		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, REG_BASE_EDC1_NAME);
		if (!res) {
			k3fb_loge("failed to get reg_base_edc1 resource.\n");
			return -ENXIO;
		}
		k3fd_reg_base_edc1 = IO_ADDRESS(res->start);

		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, REG_BASE_PWM0_NAME);
		if (!res) {
			k3fb_loge("failed to get reg_base_pwm0 resource.\n");
			return -ENXIO;
		}
		k3fd_reg_base_pwm0 = IO_ADDRESS(res->start);

		k3_fb_resource_initialized = 1;
		return 0;
	}

	if (!k3_fb_resource_initialized) {
		k3fb_loge("fb resource not initialized!\n");
		return -EPERM;
	}

	if (pdev_list_cnt >= K3_FB_MAX_DEV_LIST) {
		k3fb_loge("too many fb devices!\n");
		return -ENOMEM;
	}

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	/* edc clock */
	if (k3fd->index == 0) {
		k3fd->edc_clk_rst= clk_get(NULL, CLK_EDC0_RST_NAME);
		if (IS_ERR(k3fd->edc_clk_rst)) {
			k3fb_loge("failed to get edc_clk_rst!\n");
			return PTR_ERR(k3fd->edc_clk_rst);
		}

		k3fd->edc_clk = clk_get(NULL, CLK_EDC0_NAME);

	#if defined(CONFIG_OVERLAY_COMPOSE)
		k3_fb_overlay_compose_init(k3fd);
	#endif

	} else if (k3fd->index == 1) {
		/* edc1 vcc */
		k3fd->edc_vcc = regulator_get(NULL,  VCC_EDC1_NAME);
		if (IS_ERR(k3fd->edc_vcc)) {
			k3fb_loge("failed to get edc1-vcc regulator!\n");
			return PTR_ERR(k3fd->edc_vcc);
		}

		k3fd->edc_clk = clk_get(NULL, CLK_EDC1_NAME);
	} else {
		k3fb_loge("fb%d not support now!\n", k3fd->index);
		return -ENXIO;
	}

	if (IS_ERR(k3fd->edc_clk)) {
		k3fb_loge("failed to get edc_clk!\n");
		return PTR_ERR(k3fd->edc_clk);
	}

	ret = clk_set_rate(k3fd->edc_clk,  EDC_CLK_RATE_GET(k3fd->panel_info.clk_rate));
	if (ret != 0) {
		k3fb_loge("failed to set edc clk rate(%d).\n", EDC_CLK_RATE_GET(k3fd->panel_info.clk_rate));
		/*return ret;*/
	}

#ifdef CONFIG_G2D
	/*G2D clock*/
	k3fd->g2d_clk = clk_get(NULL, CLK_G2D_NAME);
	if (IS_ERR(k3fd->g2d_clk)) {
		k3fb_loge("failed to get g2d_clk!\n");
		return PTR_ERR(k3fd->g2d_clk);
	}
#endif

	/* fb register */
	ret = k3_fb_register(k3fd);
	if (ret != 0) {
		k3fb_loge("fb register failed!\n");
		return ret;
	}

	if (k3fd->index == 0) {
		/* fb sysfs create */
		k3_fb_sysfs_create(pdev);

		/* android supports only one lcd-backlight/lcd for now */
		if (led_classdev_register(&pdev->dev, &backlight_led))
			k3fb_loge("led_classdev_register failed!\n");
	}

	pdev_list[pdev_list_cnt++] = pdev;

	return 0;
}

static int k3_fb_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_fb_panel_data *pdata = NULL;

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	if (!k3fd) {
		return 0;
	}

	k3fb_logi("index=%d, enter!\n", k3fd->index);

	pdata = (struct k3_fb_panel_data *)k3fd->pdev->dev.platform_data;	
	if (!pdata) {
		k3fb_loge("k3_fb_panel_data is null!\n");
		return -ENODEV;
	}

	ret = k3_fb_blank_sub(FB_BLANK_POWERDOWN, k3fd->fbi);
	if (ret != 0) {
		k3fb_loge("can't stop the device %d!\n", k3fd->index);
	}

	if (pdata->remove) {
		ret = pdata->remove(k3fd->pdev);
		if (ret != 0) {
			k3fb_loge("no panel operation remove detected!\n");
		}
	}

	/* put g2d clock*/
	if (!IS_ERR(k3fd->g2d_clk)) {
		clk_put(k3fd->g2d_clk);
	}

	/* put edc clock */
	if (!IS_ERR(k3fd->edc_clk)) {
		clk_put(k3fd->edc_clk);
	}

	if (k3fd->index == 0) {

	#if defined(CONFIG_OVERLAY_COMPOSE)
		k3_fb_overlay_compose_deinit(k3fd);
	#endif

		/* put g2d clock*/
		if (!IS_ERR(k3fd->g2d_clk)) {
			clk_put(k3fd->g2d_clk);
		}

		/* put edc rst clock */
		if (!IS_ERR(k3fd->edc_clk_rst)) {
			clk_put(k3fd->edc_clk_rst);
		}

		if (k3fd->panel_info.sbl_enable && k3fd->sbl_wq) {
			destroy_workqueue(k3fd->sbl_wq);
			k3fd->sbl_wq = NULL;
		}

		if (k3fd->frame_end_wq) {
			destroy_workqueue(k3fd->frame_end_wq);
			k3fd->frame_end_wq = NULL;
		}

		if (k3fd->temp_wq) {
			destroy_workqueue(k3fd->temp_wq);
			k3fd->temp_wq = NULL;
		}

		if (k3fd->vsync_info.thread)
			kthread_stop(k3fd->vsync_info.thread);

		k3_fb_sysfs_remove(pdev);

		led_classdev_unregister(&backlight_led);
	} else if (k3fd->index == 1) {
		/* put edc vcc */
		if (!IS_ERR(k3fd->edc_vcc)) {
			regulator_put(k3fd->edc_vcc);
		}

	#if K3_FB_OVERLAY_USE_BUF
		if (video_buf.play_wq) {
			video_buf.exit_work = 1;
			destroy_workqueue(video_buf.play_wq);
		}
	#endif
	} else {
		k3fb_loge("fb%d not support now!\n", k3fd->index);
		return EINVAL;
	}

	/* remove /dev/fb* */
	ret = unregister_framebuffer(k3fd->fbi);
	if (ret != 0) {
		k3fb_loge("can't unregister framebuffer %d!\n", k3fd->index);
	}

	k3fb_logi("index=%d, exit!\n", k3fd->index);

	return ret;
}

static void k3_fb_shutdown(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	/* BUG_ON(k3fd == NULL); */
	if (!k3fd) {
		return;
	}

	k3fb_logi("index=%d, enter!\n", k3fd->index);

	ret = k3_fb_blank_sub(FB_BLANK_POWERDOWN, k3fd->fbi);
	if (ret != 0) {
		k3fb_loge("can't stop the device %d\n", k3fd->index);
	}
	
	k3fb_logi("index=%d, exit!\n", k3fd->index);
}

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static int k3_fb_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	/* BUG_ON(k3fd == NULL); */
	if (!k3fd) {
		return 0;
	}

	k3fb_logi("index=%d, enter!\n", k3fd->index);

	ret = k3_fb_blank_sub(FB_BLANK_POWERDOWN, k3fd->fbi);
	if (ret != 0) {
		k3fb_loge("failed to suspend, error=%d!\n", ret);
		fb_set_suspend(k3fd->fbi, FBINFO_STATE_RUNNING);
	} else {
		pdev->dev.power.power_state = state;
	}

	k3fb_logi("index=%d, exit!\n", k3fd->index);

	return ret;
}

static int k3_fb_resume(struct platform_device *pdev)
{
	/* This resume function is called when interrupt is enabled. */
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	/* BUG_ON(k3fd == NULL); */
	if (!k3fd) {
		return 0;
	}

	k3fb_logi("index=%d, enter!\n", k3fd->index);

	ret = k3_fb_blank_sub(FB_BLANK_UNBLANK, k3fd->fbi);
	if (ret != 0) {
		k3fb_loge("failed to resume, error=%d!\n", ret);
	}
	pdev->dev.power.power_state = PMSG_ON;
	fb_set_suspend(k3fd->fbi, FBINFO_STATE_RUNNING);

	k3fb_logi("index=%d, exit!\n", k3fd->index);

	return ret;
}
#else
#define k3_fb_suspend NULL
#define k3_fb_resume NULL
#endif


/******************************************************************************/

static struct platform_driver k3_fb_driver = {
	.probe = k3_fb_probe,
	.remove = k3_fb_remove,
	.suspend = k3_fb_suspend,
	.resume = k3_fb_resume,
	.shutdown = k3_fb_shutdown,
	.driver = {
		.name = DEV_NAME_FB,
		},
};

struct platform_device *k3_fb_add_device(struct platform_device *pdev)
{
	struct k3_fb_panel_data *pdata = NULL;
	struct platform_device *this_dev = NULL;
	struct fb_info *fbi = NULL;
	struct k3_fb_data_type *k3fd = NULL;
	u32 type = 0, id = 0, graphic_ch = 0;

	BUG_ON(pdev == NULL);

	pdata = pdev->dev.platform_data;
	BUG_ON(pdata == NULL);

	if (fbi_list_index >= MAX_FBI_LIST) {
		k3fb_loge("no more framebuffer info list!\n");
		return NULL;
	}

	/* alloc panel device data */
	id = pdev->id;
	type = pdata->panel_info->type;
	this_dev = k3_fb_device_alloc(pdata, type, fbi_list_index, &graphic_ch);
	if (!this_dev) {
		k3fb_loge("failed to k3_fb_device_alloc!\n");
		return NULL;
	}

	/* alloc framebuffer info + par data */
	fbi = framebuffer_alloc(sizeof(struct k3_fb_data_type), NULL);
	if (fbi == NULL) {
		k3fb_loge("can't alloca framebuffer info data!\n");
		/*platform_device_put(this_dev);*/
		k3_fb_device_free(this_dev);
		return NULL;
	}

	k3fd = (struct k3_fb_data_type *)fbi->par;
	k3fd->fbi = fbi;
	k3fd->panel.type = type;
	k3fd->panel.id = id;
	k3fd->graphic_ch = graphic_ch;
	k3fd->index = fbi_list_index;
	if (k3fd->index == 0) {
		k3fd->edc_base = k3fd_reg_base_edc0;
		k3fd->pwm_base = k3fd_reg_base_pwm0;
		k3fd->edc_irq = k3fd_irq_edc0;
		k3fd->ldi_irq = k3fd_irq_ldi0;
	} else if (k3fd->index == 1) {
		k3fd->edc_base = k3fd_reg_base_edc1;
		k3fd->pwm_base = k3fd_reg_base_pwm1;
		k3fd->edc_irq = k3fd_irq_edc1;
		k3fd->ldi_irq = k3fd_irq_ldi1;
	} else {
		k3fb_loge("fb%d not support now!\n", k3fd->index);
		return NULL;
	}

	/* link to the latest pdev */
	k3fd->pdev = this_dev;

	k3fd_list[k3fd_list_index++] = k3fd;
	fbi_list[fbi_list_index++] = fbi;

	 /* get/set panel info */
	memcpy(&k3fd->panel_info, pdata->panel_info, sizeof(struct k3_panel_info));

	/* set driver data */
	platform_set_drvdata(this_dev, k3fd);

	if (platform_device_add(this_dev)) {
		k3fb_loge("failed to platform_device_add!\n");
		/*platform_device_put(this_dev);*/
		framebuffer_release(fbi);
		k3_fb_device_free(this_dev);
		fbi_list_index--;
		return NULL;
	}

	return this_dev;
}
EXPORT_SYMBOL(k3_fb_add_device);

int __init k3_fb_init(void)
{
	int ret = -ENODEV;

	ret = platform_driver_register(&k3_fb_driver);
	if (ret) {
		k3fb_loge("not able to register the driver, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(k3_fb_init);
