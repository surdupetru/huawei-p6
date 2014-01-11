/* Copyright (c) 2008-2011, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/pwm.h>
#include <mach/platform.h>
#include <mach/gpio.h>

#include "k3_fb.h"
#include "k3_fb_def.h"
#include "mipi_dsi.h"
#include "mipi_reg.h"


/*
** save power by controling cabc and color enhancement
*/
static char cabc_begin[] = {
	0xF3,
	0xA0,
};

static char cabc_on[] = {
	0x0E,
	0x0A,
};

static char cabc_off[] = {
	0x0E,
	0x02,
};


static struct dsi_cmd_desc panasonic_cabc_on_cmds[] = {
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(cabc_begin), cabc_begin},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(cabc_on), cabc_on},
	{DTYPE_GEN_WRITE, 0, 30, WAIT_TYPE_US,
		0, 0}
};

static struct dsi_cmd_desc panasonic_cabc_off_cmds[] = {
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(cabc_begin), cabc_begin},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(cabc_off), cabc_off},
	{DTYPE_GEN_WRITE, 0, 30, WAIT_TYPE_US,
		0, 0}
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME		"lcdio-vcc"
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcdanalog;

static struct vcc_desc panasonic_lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000},
};

static struct vcc_desc panasonic_lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc panasonic_lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc panasonic_lcd_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
	{DTYPE_VCC_DISABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
};

/*******************************************************************************
** LCD IOMUX
*/
#define IOMUX_LCD_NAME	"block_lcd"

static struct iomux_block **lcd_block;
static struct block_config **lcd_block_config;

static struct iomux_desc panasonic_lcd_iomux_init_cmds[] = {
	{DTYPE_IOMUX_GET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, 0},
};

static struct iomux_desc panasonic_lcd_iomux_normal_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, NORMAL},
};

static struct iomux_desc panasonic_lcd_iomux_lowpower_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, LOWPOWER},
};

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_LCD_POWER_NAME	"gpio_lcd_power"
#define GPIO_LCD_ID0_NAME	"gpio_lcd_id0"
#define GPIO_LCD_ID1_NAME	"gpio_lcd_id1"

#define GPIO_LCD_POWER	GPIO_21_3
#define GPIO_LCD_ID0	GPIO_16_7
#define GPIO_LCD_ID1	GPIO_17_0

static struct gpio_desc panasonic_lcd_gpio_request_cmds[] = {
	/* power */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 0},
	/* id0 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
};

static struct gpio_desc panasonic_lcd_gpio_free_cmds[] = {
	/* power */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 0},
	/* id0 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
};


static struct gpio_desc panasonic_lcd_gpio_normal_cmds[] = {
	/* id0 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
	/* power */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 1},
};

static struct gpio_desc panasonic_lcd_gpio_lowpower_cmds[] = {
	/* id0 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
	/* power */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 0},
};

/*******************************************************************************
** PWM IOMUX
*/
#define IOMUX_PWM_NAME	"block_lcd"

static struct iomux_block **pwm_block;
static struct block_config **pwm_block_config;

static struct iomux_desc panasonic_pwm_iomux_init_cmds[] = {
	{DTYPE_IOMUX_GET, IOMUX_PWM_NAME,
		(struct iomux_block **)&pwm_block, (struct block_config **)&pwm_block_config, 0},
};

static struct iomux_desc panasonic_pwm_iomux_normal_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_PWM_NAME,
		(struct iomux_block **)&pwm_block, (struct block_config **)&pwm_block_config, NORMAL},
};

static struct iomux_desc panasonic_pwm_iomux_lowpower_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_PWM_NAME,
		(struct iomux_block **)&pwm_block, (struct block_config **)&pwm_block_config, LOWPOWER},
};

/*******************************************************************************
** PWM GPIO
*/
#define GPIO_LCD_PWM0_NAME	"gpio_pwm0"
#define GPIO_LCD_PWM1_NAME	"gpio_pwm1"

#define GPIO_PWM0	GPIO_18_5
#define GPIO_PWM1	GPIO_18_6

static struct gpio_desc panasonic_pwm_gpio_request_cmds[] = {
	/* pwm0 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM0_NAME, GPIO_PWM0, 0},
	/* pwm1 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};

static struct gpio_desc panasonic_pwm_gpio_free_cmds[] = {
	/* pwm0 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM0_NAME, GPIO_PWM0, 0},
	/* pwm1 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};

static struct gpio_desc panasonic_pwm_gpio_normal_cmds[] = {
	/* pwm1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};

static struct gpio_desc panasonic_pwm_gpio_lowpower_cmds[] = {
	/* pwm0 */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_PWM0_NAME, GPIO_PWM0, 0},
	/* pwm1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};


static volatile bool g_display_on;
static struct k3_fb_panel_data panasonic_panel_data;


/*******************************************************************************
**
*/
static ssize_t panasonic_lcd_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct k3_panel_info *pinfo = NULL;

	pinfo = panasonic_panel_data.panel_info;
	sprintf(buf, "Panasonic_VVX10F002A00 10.1'TFT %d x %d\n",
		pinfo->xres, pinfo->yres);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(lcd_info, S_IRUGO, panasonic_lcd_info_show, NULL);

static struct attribute *panasonic_attrs[] = {
	&dev_attr_lcd_info,
	NULL,
};

static struct attribute_group panasonic_attr_group = {
	.attrs = panasonic_attrs,
};

static int panasonic_sysfs_init(struct platform_device *pdev)
{
	int ret = 0;

	ret = sysfs_create_group(&pdev->dev.kobj, &panasonic_attr_group);
	if (ret) {
		k3fb_loge("create sysfs file failed!\n");
		return ret;
	}

	return 0;
}

static void panasonic_sysfs_deinit(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &panasonic_attr_group);
}


/*******************************************************************************
**
*/
static int panasonic_pwm_on(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

	/* pwm iomux normal */
	iomux_cmds_tx(panasonic_pwm_iomux_normal_cmds, \
		ARRAY_SIZE(panasonic_pwm_iomux_normal_cmds));

	/* pwm gpio request */
	gpio_cmds_tx(panasonic_pwm_gpio_request_cmds, \
		ARRAY_SIZE(panasonic_pwm_gpio_request_cmds));
	/* pwm gpio normal */
	gpio_cmds_tx(panasonic_pwm_gpio_normal_cmds, \
		ARRAY_SIZE(panasonic_pwm_gpio_normal_cmds));

	/* backlight on */
	pwm_set_backlight(k3fd, &g_display_on);

	return 0;
}

static int panasonic_pwm_off(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

	/* backlight off */
	pwm_set_backlight(k3fd, &g_display_on);

	/* pwm gpio lowpower */
	gpio_cmds_tx(panasonic_pwm_gpio_lowpower_cmds, \
		ARRAY_SIZE(panasonic_pwm_gpio_lowpower_cmds));
	/* pwm gpio free */
	gpio_cmds_tx(panasonic_pwm_gpio_free_cmds, \
		ARRAY_SIZE(panasonic_pwm_gpio_free_cmds));

	/* pwm iomux lowpower */
	iomux_cmds_tx(panasonic_pwm_iomux_lowpower_cmds, \
		ARRAY_SIZE(panasonic_pwm_iomux_lowpower_cmds));

	return 0;
}

static void panasonic_disp_on(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	pinfo = &(k3fd->panel_info);

	/* lcd iomux normal */
	iomux_cmds_tx(panasonic_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(panasonic_lcd_iomux_normal_cmds));

	/* lcd gpio request */ 
	gpio_cmds_tx(panasonic_lcd_gpio_request_cmds, \
		ARRAY_SIZE(panasonic_lcd_gpio_request_cmds));
	/* lcd gpio normal */
	gpio_cmds_tx(panasonic_lcd_gpio_normal_cmds, \
		ARRAY_SIZE(panasonic_lcd_gpio_normal_cmds));
}

static void panasonic_disp_off(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	pinfo = &(k3fd->panel_info);

	/* lcd gpio lowpower */
	gpio_cmds_tx(panasonic_lcd_gpio_lowpower_cmds, \
		ARRAY_SIZE(panasonic_lcd_gpio_lowpower_cmds));
	/* lcd gpio free */
	gpio_cmds_tx(panasonic_lcd_gpio_free_cmds, \
		ARRAY_SIZE(panasonic_lcd_gpio_free_cmds));

	/* lcd iomux lowpower */
	iomux_cmds_tx(panasonic_lcd_iomux_lowpower_cmds, \
		ARRAY_SIZE(panasonic_lcd_iomux_lowpower_cmds));

	/* lcd vcc disable */
	vcc_cmds_tx(NULL, panasonic_lcd_vcc_disable_cmds, \
		ARRAY_SIZE(panasonic_lcd_vcc_disable_cmds));
}

static int mipi_panasonic_panel_on(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	pinfo = &(k3fd->panel_info);
	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		/* lcd vcc enable */
		vcc_cmds_tx(NULL, panasonic_lcd_vcc_enable_cmds, \
			ARRAY_SIZE(panasonic_lcd_vcc_enable_cmds));

		pinfo->lcd_init_step = 	LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		if (!g_display_on) {
			/* lcd display on */
			panasonic_disp_on(k3fd);
		}
		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
		if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
			/* backlight on */
			panasonic_pwm_on(k3fd);
		}

		g_display_on = true;
	} else {
		k3fb_loge("failed to init lcd!\n");
	}

	return 0;
}

static int mipi_panasonic_panel_off(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	if (g_display_on) {
		g_display_on = false;
		if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
			/* backlight off */
			panasonic_pwm_off(k3fd);
		}
		/* lcd display off */
		panasonic_disp_off(k3fd);
	}

	return 0;
}

static int mipi_panasonic_panel_remove(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;
	
	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	k3fb_logd("index=%d, enter!\n", k3fd->index);

	if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		/* pwm clock */
		pwm_clk_put(&(k3fd->panel_info));
	}

	/* lcd vcc finit */
	vcc_cmds_tx(pdev, panasonic_lcd_vcc_finit_cmds, \
		ARRAY_SIZE(panasonic_lcd_vcc_finit_cmds));

	panasonic_sysfs_deinit(pdev);

	k3fb_logd("index=%d, exit!\n", k3fd->index);

	return 0;
}

static int mipi_panasonic_panel_set_backlight(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	return pwm_set_backlight(k3fd, &g_display_on);
}

static int mipi_panasonic_panel_set_fastboot(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	/* lcd vcc enable */
	vcc_cmds_tx(pdev, panasonic_lcd_vcc_enable_cmds, \
		ARRAY_SIZE(panasonic_lcd_vcc_enable_cmds));

	/* lcd iomux normal */
	iomux_cmds_tx(panasonic_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(panasonic_lcd_iomux_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(panasonic_lcd_gpio_request_cmds, \
		ARRAY_SIZE(panasonic_lcd_gpio_request_cmds));

	if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		/* pwm iomux normal */
		iomux_cmds_tx(panasonic_pwm_iomux_normal_cmds, \
			ARRAY_SIZE(panasonic_pwm_iomux_normal_cmds));

		/* pwm gpio request */
		gpio_cmds_tx(panasonic_pwm_gpio_request_cmds, \
			ARRAY_SIZE(panasonic_pwm_gpio_request_cmds));
	}

	g_display_on = true;

	return 0;
}

static int mipi_panasonic_panel_set_cabc(struct platform_device *pdev, int value)
{
	u32 edc_base = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;

	if (value) {
		mipi_dsi_cmds_tx(panasonic_cabc_on_cmds, \
			ARRAY_SIZE(panasonic_cabc_on_cmds), edc_base);
	} else {
		mipi_dsi_cmds_tx(panasonic_cabc_off_cmds, \
			ARRAY_SIZE(panasonic_cabc_off_cmds), edc_base);
	}

	return 0;
}

static struct k3_panel_info panasonic_panel_info = {0};
static struct k3_fb_panel_data panasonic_panel_data = {
	.panel_info = &panasonic_panel_info,
	.on = mipi_panasonic_panel_on,
	.off = mipi_panasonic_panel_off,
	.remove = mipi_panasonic_panel_remove,
	.set_backlight = mipi_panasonic_panel_set_backlight,
	.set_fastboot = mipi_panasonic_panel_set_fastboot,
	.set_cabc = mipi_panasonic_panel_set_cabc,
};


/*******************************************************************************
**
*/
static int __devinit panasonic_probe(struct platform_device *pdev)
{
	struct k3_panel_info *pinfo = NULL;

	g_display_on = false;

	pinfo = panasonic_panel_data.panel_info;
	/* init lcd panel info */
	pinfo->xres = 1920;
	pinfo->yres = 1200;
	pinfo->width = 217;
	pinfo->height = 136;
	pinfo->type = PANEL_MIPI_VIDEO;
	pinfo->orientation = LCD_LANDSCAPE;
	pinfo->bpp = EDC_OUT_RGB_888;
	pinfo->s3d_frm = EDC_FRM_FMT_2D;
	pinfo->bgr_fmt = EDC_RGB;
	pinfo->bl_set_type = BL_SET_BY_PWM;
	pinfo->bl_min = 1;
	pinfo->bl_max = 100;

	pinfo->frc_enable = 1;
	pinfo->esd_enable = 1;
	pinfo->sbl_enable = 1;

	pinfo->sbl.bl_max = 0x64;
	pinfo->sbl.cal_a = 0x18;
	pinfo->sbl.cal_b = 0xd8;
	pinfo->sbl.str_limit = 0x50;
	
	pinfo->ldi.h_back_porch = 49;
	pinfo->ldi.h_front_porch = 89;
	pinfo->ldi.h_pulse_width = 5;
	pinfo->ldi.v_back_porch = 4;
	pinfo->ldi.v_front_porch = 6;
	pinfo->ldi.v_pulse_width = 2;
	pinfo->ldi.hsync_plr = 1;
	pinfo->ldi.vsync_plr = 1;
	pinfo->ldi.pixelclk_plr = 0;
	pinfo->ldi.data_en_plr = 0;
	pinfo->ldi.disp_mode = LDI_DISP_MODE_NOT_3D_FBF;

	/* Note: must init here */
	pinfo->frame_rate = 60;
	pinfo->clk_rate = 150000000;

	pinfo->mipi.lane_nums = DSI_4_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.dsi_bit_clk = 482;

	/* lcd vcc init */
	vcc_cmds_tx(pdev, panasonic_lcd_vcc_init_cmds, \
		ARRAY_SIZE(panasonic_lcd_vcc_init_cmds));

	/* lcd iomux init */
	iomux_cmds_tx(panasonic_lcd_iomux_init_cmds, \
		ARRAY_SIZE(panasonic_lcd_iomux_init_cmds));

	if (pinfo->bl_set_type & BL_SET_BY_PWM) {
		pinfo->pwm.name = CLK_PWM0_NAME;
		pinfo->pwm.clk_rate = DEFAULT_PWM_CLK_RATE;

		/* pwm clock */
		pwm_clk_get(pinfo);

		/* pwm iomux init */
		iomux_cmds_tx(panasonic_pwm_iomux_init_cmds, \
			ARRAY_SIZE(panasonic_pwm_iomux_init_cmds));
	}

	/* alloc panel device data */
	if (platform_device_add_data(pdev, &panasonic_panel_data,
			sizeof(struct k3_fb_panel_data))) {
		k3fb_loge("failed to platform_device_add_data!\n");
		platform_device_put(pdev);
		return -ENOMEM;
	}

	k3_fb_add_device(pdev);

	panasonic_sysfs_init(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe = panasonic_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_panasonic_VVX10F002A00",
	},
};

static int __init mipi_panasonic_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		k3fb_loge("not able to register the driver, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(mipi_panasonic_panel_init);
