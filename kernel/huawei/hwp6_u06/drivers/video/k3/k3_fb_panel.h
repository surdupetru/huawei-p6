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

#ifndef K3_FB_PANEL_H
#define K3_FB_PANEL_H

#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/pwm.h>
#include <linux/mux.h>
#include <mach/gpio.h>

#include "k3_fb_def.h"


/* panel type list */
#define PANEL_NO			0xffff	/* No Panel */
#define PANEL_LDI			1	/* internal LCDC type */
#define PANEL_HDMI		2	/* HDMI TV */
#define PANEL_MIPI_VIDEO	3	/* MIPI */
#define PANEL_MIPI_CMD	4	/* MIPI */
#define PANEL_DP			5	/* Display Port */
#define PANEL_MIPI2RGB	6	/* MIPI to RGB */

/* device name */
#define DEV_NAME_LDI				"ldi"
#define DEV_NAME_HDMI			"hdmi"
#define DEV_NAME_DP				"displayport"
#define DEV_NAME_MIPI2RGB		"mipi2rgb"
#define DEV_NAME_MIPIDSI			"mipi_dsi"
#define DEV_NAME_FB				"k3_fb"
#define DEV_NAME_LCD_BKL		"lcd_backlight0"

/* clk name */
#define CLK_PWM0_NAME	"clk_pwm0"
#define CLK_MIPI_DPHY0_NAME	"clk_dphy0"
#define CLK_MIPI_DPHY1_NAME	"clk_dphy2"
#define CLK_LDI0_NAME	"clk_ldi0"
#define CLK_LDI1_NAME	"clk_ldi1"
#define CLK_EDC0_NAME	"clk_edc0"
#define CLK_EDC0_RST_NAME	"clk_edc0_rst"
#define CLK_EDC1_NAME	"clk_edc1"
#define CLK_G2D_NAME	"clk_g2d"

/* irq name */
#define IRQ_LDI0_NAME	"irq_ldi0"
#define IRQ_LDI1_NAME	"irq_ldi1"
#define IRQ_EDC0_NAME	"irq_edc0"
#define IRQ_EDC1_NAME	"irq_edc1"

/* reg base name */
#define REG_BASE_PWM0_NAME	"reg_base_pwm0"
#define REG_BASE_EDC0_NAME	"reg_base_edc0"
#define REG_BASE_EDC1_NAME	"reg_base_edc1"

/* vcc name */
#define VCC_EDC1_NAME		"vcc_edc1"

/* default pwm clk */
#define DEFAULT_PWM_CLK_RATE	(13 * 1000000)


/* LCD init step */
enum {
	LCD_INIT_NONE = 0,
	LCD_INIT_POWER_ON,
	LCD_INIT_LDI_SEND_SEQUENCE,
	LCD_INIT_MIPI_LP_SEND_SEQUENCE,
	LCD_INIT_MIPI_HS_SEND_SEQUENCE,
};

/* resource desc */
struct resource_desc {
	u32 flag;
	char *name;
	u32 *value;
};

/* dtype for vcc */
enum {
	DTYPE_VCC_GET,
	DTYPE_VCC_PUT,
	DTYPE_VCC_ENABLE,
	DTYPE_VCC_DISABLE,
	DTYPE_VCC_SET_VOLTAGE,
};

/* vcc desc */
struct vcc_desc {
	int dtype;
	char *id;
	struct regulator **regulator;
	int min_uV;
	int max_uV;
};

/* dtype for iomux */
enum {
	DTYPE_IOMUX_GET,
	DTYPE_IOMUX_SET,
};

/* iomux desc */
struct iomux_desc {
	int dtype;
	char *name;
	struct iomux_block **block;
	struct block_config **block_config;
	/*enum iomux_func mode;*/
	int mode;
};

/* dtype for gpio */
enum {
	DTYPE_GPIO_REQUEST,
	DTYPE_GPIO_FREE,
	DTYPE_GPIO_INPUT,
	DTYPE_GPIO_OUTPUT,
};

/* gpio desc */
struct gpio_desc {
	int dtype;
	int waittype;
	int wait;
	char *label;
	u32 gpio;
	int value;
};

typedef struct panel_id_s {
	u32 id;
	u32 type;
} panel_id_type;

struct ldi_panel_info {
	u32 h_back_porch;
	u32 h_front_porch;
	u32 h_pulse_width;

	/*
	** note: vbp > 8 if used overlay compose, 
	** also lcd vbp > 8 in lcd power on sequence
	*/
	u32 v_back_porch;
	u32 v_front_porch;
	u32 v_pulse_width;

	u8 hsync_plr;
	u8 vsync_plr;
	u8 pixelclk_plr;
	u8 data_en_plr;
	u8 disp_mode;
};

/* DSI PHY configuration */
struct mipi_dsi_phy_ctrl {
	u32 burst_mode;
	u32 lane_byte_clk;
	u32 clk_division;
	u32 lp2hs_time;
	u32 hs2lp_time;
	u32 hsfreqrange;
	u32 pll_unlocking_filter;
	u32 cp_current;
	u32 lpf_ctrl;
	u32 n_pll;
	u32 m_pll_1;
	u32 m_pll_2;
	u32 factors_effective;
};

struct mipi_panel_info {
	u8 vc;
	u8 lane_nums;
	u8 color_mode;
	u32 dsi_bit_clk; /* clock lane(p/n) */
	/*u32 dsi_pclk_rate;*/
};

struct sbl_panel_info{
	u32 bl_max;
	u32 cal_a;
	u32 cal_b;
	u32 str_limit;
};

struct pwm_panel_info {
	struct clk *clk;
	u32 clk_rate;
	char *name;
};

struct k3_panel_info {
	u32 type;
	u32 xres;
	u32 yres;
	u32 width;
	u32 height;
	u8 bpp;
	u8 orientation;
	u8 s3d_frm;
	u8 bgr_fmt;
	u8 frame_rate;
	u8 bl_set_type;
	u32 bl_min;
	u32 bl_max;
	u32 clk_rate;

	u8 lcd_init_step;

	u8 sbl_enable;
	u8 frc_enable;
	u8 esd_enable;

	struct pwm_panel_info pwm;
	struct ldi_panel_info ldi;
	struct mipi_panel_info mipi;
	struct sbl_panel_info sbl;
};

struct k3_fb_data_type;

struct k3_fb_panel_data {
	struct k3_panel_info *panel_info;

	/* function entry chain */
	int (*on) (struct platform_device *pdev);
	int (*off) (struct platform_device *pdev);
	int (*remove) (struct platform_device *pdev);
	int (*set_backlight) (struct platform_device *pdev);
	int (*set_timing) (struct platform_device *pdev);
	int (*set_fastboot) (struct platform_device *pdev);
	int (*set_frc)(struct platform_device *pdev, int target_fps);
	int (*set_cabc)(struct platform_device *pdev, int value);
	int (*check_esd) (struct platform_device *pdev);
	struct platform_device *next;
};


/*******************************************************************************
** FUNCTIONS PROTOTYPES
*/

void set_reg(u32 addr, u32 val, u8 bw, u8 bs);

int resource_cmds_tx(struct platform_device *pdev,
	struct resource_desc *cmds, int cnt);
int vcc_cmds_tx(struct platform_device *pdev, struct vcc_desc *cmds, int cnt);
int iomux_cmds_tx(struct iomux_desc *cmds, int cnt);
int gpio_cmds_tx(struct gpio_desc *cmds, int cnt);

struct platform_device *k3_fb_device_alloc(struct k3_fb_panel_data *pdata,
	u32 type, u32 index, u32 *graphic_ch);
void k3_fb_device_free(struct platform_device *pdev);

int panel_next_on(struct platform_device *pdev);
int panel_next_off(struct platform_device *pdev);
int panel_next_remove(struct platform_device *pdev);
int panel_next_set_backlight(struct platform_device *pdev);
int panel_next_set_timing(struct platform_device *pdev);
int panel_next_set_frc(struct platform_device *pdev, int target_fps);
int panel_next_check_esd(struct platform_device *pdev);

int pwm_set_backlight(struct k3_fb_data_type *k3fd, volatile bool *display_on);
int pwm_clk_get(struct k3_panel_info *pinfo);
void pwm_clk_put(struct k3_panel_info *pinfo);


#endif /* K3_FB_PANEL_H */
