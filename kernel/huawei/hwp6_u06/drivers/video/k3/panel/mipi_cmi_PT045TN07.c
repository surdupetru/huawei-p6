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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/pwm.h>
#include <mach/platform.h>
#include <mach/gpio.h>
#include <mach/boardid.h>

#include "k3_fb.h"
#include "k3_fb_def.h"
#include "mipi_dsi.h"
#include "mipi_reg.h"

#include <linux/lcd_tuning.h>


/*******************************************************************************
** Power ON Sequence(sleep mode to Normal mode)
*/
static char backlight_51[] = {
	0x51,
	0xFF,
};

static char backlight_53[] = {
	0x53,
	0x24,
};

static char data_ff[] = {
	0xFF,
	0xAA, 0x55, 0x25, 0x01 
};

static char data_f5[] = {
	0xF5,
	0x00, 0x00
};

/* set page0 */
static char data_page0[] = {
	0xF0,
	0x55, 0xAA, 0x52, 0x08, 0x00
};

static char data_page0_c7[] = {
        0xC7,
        0x70
};

static char data_page0_ca[] = {
	0xCA,
	0x00, 0x1B, 0x1B, 0x1B, 0x1B,
	0x1B, 0x1B, 0x02, 0x02, 0x00,
	0x00
};

static char data_page0_ba[] = {
	0xBA,
	0x05
};

static char data_page0_c9[] = {
	0xC9,
	0x41, 0x06, 0x0d, 0x3a, 0x17,
	0x00
};

static char data_page0_b1[] = {
	0xB1,
	0xFC, 0x06, 0x00
};

static char data_page0_bc[] = {
	0xBC,
	0x00, 0x00, 0x00
};

static char data_page0_b7[] = {
	0xB7,
	0x72, 0x22
};

static char data_page0_b8[] = {
	0xB8,
	0x01, 0x02, 0x02, 0x00
};

static char data_page0_bb[] = {
	0xBB,
	0x53, 0x03, 0x53
};

static char data_page0_bd[] = {
	0xBD,
	0x01, 0x69, 0x05, 0x05, 0x01
};

static char data_page0_b6[] = {
	0xB6,
	0x0B
};

static char data_page0_3a[] = {
	0x3A,
	0x77
};

static char data_page0_36[] = {
	0x36,
	0x00
};

/* set page1 */
static char data_page1[] = {
	0xF0,
	0x55, 0xAA, 0x52, 0x08, 0x01
};

static char data_page1_b0[] = {
	0xB0,
	0x0A
};

static char data_page1_b6[] = {
	0xB6,
	0x54
};

static char data_page1_b1[] = {
	0xB1,
	0x0A
};

static char data_page1_b7[] = {
	0xB7,
	0x34
};

static char data_page1_b2[] = {
	0xB2,
	0x00
};

static char data_page1_b8[] = {
	0xB8,
	0x30
};

static char data_page1_b3[] = {
	0xB3,
	0x0D
};

static char data_page1_b9[] = {
	0xB9,
	0x24
};

static char data_page1_b4[] = {
	0xB4,
	0x08
};

static char data_page1_ba[] = {
	0xBA,
	0x14
};

static char data_page1_bc[] = {
	0xBC,
	0x00, 0x78, 0x00
};

static char data_page1_bd[] = {
	0xBD,
	0x00, 0x78, 0x00
};

static char data_page1_36[] = {
	0x36,
	0x00
};

static char data_page1_35[] = {
	0x35,
	0x00
};

static char data_page1_44[] = {
	0x44,
	0x01, 0xE0
};

static char exit_sleep[] = {
	0x11,
};

static char display_on[] = {
	0x29,
};

/*******************************************************************************
** Power OFF Sequence(Normal to power off)
*/
static char display_off[] = {
	0x28,
};

static char enter_sleep[] = {
	0x10,
};

static struct dsi_cmd_desc cmi_display_on_cmds[] = {
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_ff), data_ff},
        {DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
                sizeof(data_f5), data_f5},

        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0), data_page0},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_c7), data_page0_c7},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_ca), data_page0_ca},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_ba), data_page0_ba},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_c9), data_page0_c9},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_b1), data_page0_b1},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_bc), data_page0_bc},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_b7), data_page0_b7},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_b8), data_page0_b8},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_bb), data_page0_bb},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_bd), data_page0_bd},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_b6), data_page0_b6},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_3a), data_page0_3a},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page0_36), data_page0_36},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1), data_page1},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_b0), data_page1_b0},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_b6), data_page1_b6},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_b1), data_page1_b1},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_b7), data_page1_b7},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_b2), data_page1_b2},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_b8), data_page1_b8},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_b3), data_page1_b3},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_b9), data_page1_b9},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_b4), data_page1_b4},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_ba), data_page1_ba},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_bc), data_page1_bc},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_bd), data_page1_bd},
        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_36), data_page1_36},

        {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_35), data_page1_35},
        {DTYPE_GEN_LWRITE, 0, 3000, WAIT_TYPE_US,
                sizeof(data_page1_44), data_page1_44},

       {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
                sizeof(backlight_51), backlight_51},

      {DTYPE_GEN_WRITE2, 0, 3000, WAIT_TYPE_US,
              sizeof(backlight_53), backlight_53},

        {DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
                sizeof(exit_sleep), exit_sleep},

        {DTYPE_DCS_WRITE, 0, 5, WAIT_TYPE_MS,
                sizeof(display_on), display_on},
};

static struct dsi_cmd_desc cmi_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 5, WAIT_TYPE_MS,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 0, 80, WAIT_TYPE_MS,
		sizeof(enter_sleep), enter_sleep}
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME		"lcdio-vcc"
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcdanalog;

static struct vcc_desc cmi_lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000},
};

static struct vcc_desc cmi_lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc cmi_lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc cmi_lcd_vcc_disable_cmds[] = {
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

static struct iomux_desc cmi_lcd_iomux_init_cmds[] = {
	{DTYPE_IOMUX_GET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, 0},
};

static struct iomux_desc cmi_lcd_iomux_normal_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, NORMAL},
};

static struct iomux_desc cmi_lcd_iomux_lowpower_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, LOWPOWER},
};

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"
#define GPIO_LCD_ID0_NAME	"gpio_lcd_id0"
#define GPIO_LCD_ID1_NAME	"gpio_lcd_id1"

#define GPIO_LCD_RESET	GPIO_0_3
#define GPIO_LCD_ID0	GPIO_16_7
#define GPIO_LCD_ID1	GPIO_17_0

static struct gpio_desc cmi_lcd_gpio_request_cmds[] = {
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
	/* id0 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
};

static struct gpio_desc cmi_lcd_gpio_free_cmds[] = {
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
	/* id0 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
};

static struct gpio_desc cmi_lcd_gpio_normal_cmds[] = {
	/* id0 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 50,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
};

static struct gpio_desc cmi_lcd_gpio_lowpower_cmds[] = {
	/* id0 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
};


static volatile bool g_display_on;
static struct k3_fb_panel_data cmi_panel_data;


/*******************************************************************************
**
*/
static struct lcd_tuning_dev *p_tuning_dev = NULL;

/*y=pow(x,0.6),x=[0,255]*/
static u32 square_point_six(u32 x)
{
	unsigned long t = x * x * x;
	int i = 0, j = 255, k = 0;
	unsigned long t0 = 0;

	while (j - i > 1) {
		k = (i + j) / 2;
		t0 = k * k * k * k * k;
		if (t0 < t)
			i = k;
		else if (t0 > t)
			j = k;
		else
			return k;
	}

	return k;
}

static int cmi_set_gamma(struct lcd_tuning_dev *ltd, enum lcd_gamma gamma)
{
	int ret = 0;
	struct platform_device *pdev = NULL;
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;

	BUG_ON(ltd == NULL);
	pdev = (struct platform_device *)(ltd->data);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	edc_base = k3fd->edc_base;

	/* Fix me: Implement it */

	return ret;
}

static int cmi_set_cabc(struct lcd_tuning_dev *ltd, enum  tft_cabc cabc)
{
	int ret = 0;
	struct platform_device *pdev = NULL;
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;

	BUG_ON(ltd == NULL);
	pdev = (struct platform_device *)(ltd->data);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	edc_base = k3fd->edc_base;

	/* Fix me: Implement it */

	return ret;
}

static struct lcd_tuning_ops sp_tuning_ops = {
	.set_gamma = cmi_set_gamma,
	.set_cabc = cmi_set_cabc,
};

static ssize_t cmi_lcd_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct k3_panel_info *pinfo = NULL;

	pinfo = cmi_panel_data.panel_info;
	sprintf(buf, "CHIMEI_Innolux 4.5'TFT %d x %d\n",
		pinfo->xres, pinfo->yres);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(lcd_info, S_IRUGO, cmi_lcd_info_show, NULL);
static struct attribute *cmi_attrs[] = {
	&dev_attr_lcd_info,
	NULL,
};

static struct attribute_group cmi_attr_group = {
	.attrs = cmi_attrs,
};

static int cmi_sysfs_init(struct platform_device *pdev)
{
	int ret = 0;

	ret = sysfs_create_group(&pdev->dev.kobj, &cmi_attr_group);
	if (ret) {
		k3fb_loge("create sysfs file failed!\n");
		return ret;
	}

	return 0;
}

static void cmi_sysfs_deinit(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &cmi_attr_group);
}


/*******************************************************************************
**
*/
static void cmi_disp_on(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	pinfo = &(k3fd->panel_info);

	/* lcd iomux normal */
	iomux_cmds_tx(cmi_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(cmi_lcd_iomux_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(cmi_lcd_gpio_request_cmds, \
		ARRAY_SIZE(cmi_lcd_gpio_request_cmds));
	/* lcd gpio normal */
	gpio_cmds_tx(cmi_lcd_gpio_normal_cmds, \
		ARRAY_SIZE(cmi_lcd_gpio_normal_cmds));

	/* lcd display on sequence */
	mipi_dsi_cmds_tx(cmi_display_on_cmds, \
		ARRAY_SIZE(cmi_display_on_cmds), edc_base);
}

static void cmi_disp_off(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	pinfo = &(k3fd->panel_info);

	/* lcd display off sequence */
	mipi_dsi_cmds_tx(cmi_display_off_cmds, \
		ARRAY_SIZE(cmi_display_off_cmds), edc_base);

	/* lcd gpio lowpower */
	gpio_cmds_tx(cmi_lcd_gpio_lowpower_cmds, \
		ARRAY_SIZE(cmi_lcd_gpio_lowpower_cmds));
	/* lcd gpio free */
	gpio_cmds_tx(cmi_lcd_gpio_free_cmds, \
		ARRAY_SIZE(cmi_lcd_gpio_free_cmds));

	/* lcd iomux lowpower */
	iomux_cmds_tx(cmi_lcd_iomux_lowpower_cmds, \
		ARRAY_SIZE(cmi_lcd_iomux_lowpower_cmds));

	/* lcd vcc disable */
	vcc_cmds_tx(NULL, cmi_lcd_vcc_disable_cmds, \
		ARRAY_SIZE(cmi_lcd_vcc_disable_cmds));
}

static int mipi_cmi_panel_on(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	pinfo = &(k3fd->panel_info);
	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		/* lcd vcc enable */
		vcc_cmds_tx(NULL, cmi_lcd_vcc_enable_cmds, \
			ARRAY_SIZE(cmi_lcd_vcc_enable_cmds));

		pinfo->lcd_init_step = 	LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		if (!g_display_on) {
			/* lcd display on */
			cmi_disp_on(k3fd);
		}
		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {

		g_display_on = true;
	} else {
		k3fb_loge("failed to init lcd!\n");
	}

	return 0;
}

static int mipi_cmi_panel_off(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	if (g_display_on) {
		g_display_on = false;
		/* lcd display off */
		cmi_disp_off(k3fd);
	}

	return 0;
}

static int mipi_cmi_panel_remove(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	/*BUG_ON(k3fd == NULL);*/
	if (!k3fd) {
		return 0;
	}

	/* lcd vcc finit */
	vcc_cmds_tx(pdev, cmi_lcd_vcc_finit_cmds, \
		ARRAY_SIZE(cmi_lcd_vcc_finit_cmds));

	cmi_sysfs_deinit(pdev);

	return 0;
}

static int mipi_cmi_panel_set_backlight(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;
	u32 level = 0;

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;

	/*Our eyes are more sensitive to small brightness.
	So we adjust the brightness of lcd following iphone4 */
	/* Y=(X/255)^1.6*255 */
	level = (k3fd->bl_level * square_point_six(k3fd->bl_level) * 100) / 2779;
	if (level > 255)
		level = 255;

	outp32(edc_base + MIPIDSI_GEN_HDR_OFFSET, (level << 16) | (0x51 << 8) | 0x15);

	return ret;
}

static int mipi_cmi_panel_set_fastboot(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	/* lcd vcc enable */
	vcc_cmds_tx(pdev, cmi_lcd_vcc_enable_cmds, \
		ARRAY_SIZE(cmi_lcd_vcc_enable_cmds));

	/* lcd iomux normal */
	iomux_cmds_tx(cmi_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(cmi_lcd_iomux_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(cmi_lcd_gpio_request_cmds, \
		ARRAY_SIZE(cmi_lcd_gpio_request_cmds));

	g_display_on = true;

	return 0;
}

static int mipi_cmi_panel_set_cabc(struct platform_device *pdev, int value)
{
	u32 edc_base = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;

	/* Fix me: Implement it */

	return 0;
}

static struct k3_panel_info cmi_panel_info = {0};
static struct k3_fb_panel_data cmi_panel_data = {
	.panel_info = &cmi_panel_info,
	.on = mipi_cmi_panel_on,
	.off = mipi_cmi_panel_off,
	.remove = mipi_cmi_panel_remove,
	.set_backlight = mipi_cmi_panel_set_backlight,
	.set_fastboot = mipi_cmi_panel_set_fastboot,
	.set_cabc = mipi_cmi_panel_set_cabc,
};


/*******************************************************************************
**
*/
static int __devinit cmi_probe(struct platform_device *pdev)
{
	struct k3_panel_info *pinfo = NULL;
	struct platform_device *reg_pdev = NULL;
	struct lcd_properities lcd_props;

	g_display_on = false;

	pinfo = cmi_panel_data.panel_info;
	/* init lcd panel info */
	pinfo->xres = 540;
	pinfo->yres = 960;
	pinfo->type = PANEL_MIPI_CMD;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = EDC_OUT_RGB_888;
	pinfo->s3d_frm = EDC_FRM_FMT_2D;
	pinfo->bgr_fmt = EDC_RGB;
	pinfo->bl_set_type = BL_SET_BY_MIPI;
	pinfo->bl_min = 1;
	pinfo->bl_max = 100;

	pinfo->frc_enable = 1;
	pinfo->esd_enable = 1;
	pinfo->sbl_enable = 0;

	pinfo->sbl.bl_max = 0xff;
	pinfo->sbl.cal_a = 0x0f;
	pinfo->sbl.cal_b = 0xd8;
	pinfo->sbl.str_limit = 0x40;

	pinfo->ldi.h_back_porch = 16;
	pinfo->ldi.h_front_porch = 16;
	pinfo->ldi.h_pulse_width = 2;
	pinfo->ldi.v_back_porch = 2;
	pinfo->ldi.v_front_porch = 2;
	pinfo->ldi.v_pulse_width = 2;

	pinfo->ldi.hsync_plr = 1;
	pinfo->ldi.vsync_plr = 0;
	pinfo->ldi.pixelclk_plr = 1;
	pinfo->ldi.data_en_plr = 0;

	pinfo->ldi.disp_mode = LDI_DISP_MODE_NOT_3D_FBF;

	/* Note: must init here */
	pinfo->frame_rate = 60;
	pinfo->clk_rate = 32000000;

	pinfo->mipi.lane_nums = DSI_2_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.dsi_bit_clk = 216;

	/* lcd vcc init */
	vcc_cmds_tx(pdev, cmi_lcd_vcc_init_cmds, \
		ARRAY_SIZE(cmi_lcd_vcc_init_cmds));

	/* lcd iomux init */
	iomux_cmds_tx(cmi_lcd_iomux_init_cmds, \
		ARRAY_SIZE(cmi_lcd_iomux_init_cmds));

	/* alloc panel device data */
	if (platform_device_add_data(pdev, &cmi_panel_data,
		sizeof(struct k3_fb_panel_data))) {
		k3fb_loge("platform_device_add_data failed!\n");
		platform_device_put(pdev);
		return -ENOMEM;
	}

	reg_pdev = k3_fb_add_device(pdev);
	lcd_props.type = TFT;
	lcd_props.default_gamma = GAMMA25;

	p_tuning_dev = lcd_tuning_dev_register(&lcd_props, &sp_tuning_ops, (void *)reg_pdev);
	if (IS_ERR(p_tuning_dev)) {
		k3fb_loge("lcd_tuning_dev_register failed!\n");
		return -1;
	}

	cmi_sysfs_init(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe = cmi_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_cmi_PT045TN07",
	},
};

static int __init mipi_cmi_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		k3fb_loge("not able to register the driver\n");
		return ret;
	}

	return ret;
}

module_init(mipi_cmi_panel_init);
