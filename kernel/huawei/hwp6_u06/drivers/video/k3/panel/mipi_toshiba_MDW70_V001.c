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
static char powerOnData1[] = {
	0xB0,
	0x00,
};

/* CABC ON/OFF */
static char cabc_on[] = {
	0xBB,
	0x0D
};

static char cabc_off[] = {
	0xBB,
	0x04,
};

static char display_RGB_switch_order[] = {
	0xc6,
	0x14,
	0x00,
	0x08,  
	0x71,  
	0x00,  
	0x00,  
	0x00,  
	0x00,  
	0x00,  
	0x00,  
};

/* Color Enhancement */
static char color_enhance[] = {
    0xBD,
    0x60, 0x98, 0x60, 0xC0,
    0x90, 0xD0, 0xB0, 0x90,
    0x20, 0x00, 0x80,
};

static char powerOnData2[] = {
	0xB9,
	0x00, 0x00, 0xEA, 0x00
};

#if defined(CONFIG_OVERLAY_COMPOSE)
static char  blanktime[] = {
	0xC0,
	0x40, 0x02, 0x7F, 0x0E,
	0x07,
};
#endif

/* exit sleep mode */
static char exit_sleep[] = {
	0x11,
};

/* set display on */
static char display_on[] = {
	0x29,
};

/*******************************************************************************
** Power OFF Sequence(Normal to power off)
*/
static char enter_sleep[] = {
	0x10,
};

/*******************************************************************************
** Gamma Setting Sequence
*/
static char gamma25Data1[] = {
	0xc9, 0x0f, 0x14, 0x21, 0x2e, 0x32, 0x2e,
	0x3a, 0x45, 0x3f, 0x42, 0x52, 0x39, 0x33
};

static char gamma25Data2[] = {
	0xca, 0x30, 0x2b, 0x3e, 0x31, 0x2d, 0x31,
	0x25, 0x1a, 0x20, 0x1d, 0x0d, 0x06, 0x0c
};

static char gamma25Data3[] = {
	0xcb, 0x0f, 0x14, 0x21, 0x2e, 0x32, 0x2e,
	0x3a, 0x45, 0x3f, 0x42, 0x52, 0x39, 0x33
};

static char gamma25Data4[] = {
	0xcc, 0x30, 0x2b, 0x3e, 0x31, 0x2d, 0x31,
	0x25, 0x1a, 0x20, 0x1d, 0x0d, 0x06, 0x0c
};

static char gamma25Data5[] = {
	0xcd, 0x0f, 0x14, 0x21, 0x2e, 0x32, 0x2e,
	0x3a, 0x45, 0x3f, 0x42, 0x52, 0x39, 0x33
};

static char gamma25Data6[] = {
	0xce, 0x30, 0x2b, 0x3e, 0x31, 0x2d, 0x31,
	0x25, 0x1a, 0x20, 0x1d, 0x0d, 0x06, 0x0c
};

static char gamma22Data1[] = {
	0xc9, 0x0f, 0x10, 0x1a, 0x25, 0x28, 0x25,
	0x32, 0x3e, 0x38, 0x3b, 0x4d, 0x36, 0x33
};

static char gamma22Data2[] = {
	0xca, 0x30, 0x2f, 0x45, 0x3a, 0x37, 0x3a,
	0x2d, 0x21, 0x27, 0x24, 0x12, 0x09, 0x0c
};

static char gamma22Data3[] = {
	0xcb, 0x0f, 0x10, 0x1a, 0x25, 0x28, 0x25,
	0x32, 0x3e, 0x38, 0x3b, 0x4d, 0x36, 0x33
};

static char gamma22Data4[] = {
	0xcc, 0x30, 0x2f, 0x45, 0x3a, 0x37, 0x3a,
	0x2d, 0x21, 0x27, 0x24, 0x12, 0x09, 0x0c
};

static char gamma22Data5[] = {
	0xcd, 0x0f, 0x10, 0x1a, 0x25, 0x28, 0x25,
	0x32, 0x3e, 0x38, 0x3b, 0x4d, 0x36, 0x33
};

static char gamma22Data6[] = {
	0xce, 0x30, 0x2f, 0x45, 0x3a, 0x37, 0x3a,
	0x2d, 0x21, 0x27, 0x24, 0x12, 0x09, 0x0c
};

/*******************************************************************************
** CABC Setting Sequence
*/
/* CABC in UI setting */
static char cabcUiData1[] = {
	0xb7,
	0x18, 0x00, 0x18, 0x18, 0x0c, 0x10, 0x5c,
	0x10, 0xac, 0x10, 0x0c, 0x10, 0x00, 0x10
};

static char cabcUiData2[] = {
	0xb8,
	0xf8, 0xda, 0x6d, 0xfb, 0xff, 0xff,
	0xcf, 0x1f, 0xc0, 0xd3, 0xe3, 0xf1, 0xff
};

static char cabcUiData3[] = {
	0xBE,
	0xff, 0x0f, 0x02, 0x02, 0x04,
	0x04, 0x00, 0x5d
};

/* CABC in video setting */
static char cabcVidData1[] = {
	0xb7,
	0x18, 0x00, 0x18, 0x18, 0x0c, 0x13, 0x5c,
	0x13, 0xac, 0x13, 0x0c, 0x13, 0x00, 0x10
};

static char cabcVidData2[] = {
	0xb8,
	0xf8, 0xda, 0x6d, 0xfb, 0xff, 0xff,
	0xcf, 0x1f, 0x67, 0x89, 0xaf, 0xd6, 0xff
};

static char cabcVidData3[] = {
	0xBE,
	0xff, 0x0f, 0x00, 0x18, 0x04,
	0x40, 0x00, 0x5d
};

static struct dsi_cmd_desc toshiba_cabc_ui_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcUiData1), cabcUiData1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcUiData2), cabcUiData2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcUiData3), cabcUiData3}
};

static struct dsi_cmd_desc toshiba_cabc_vid_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcVidData1), cabcVidData1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcVidData2), cabcVidData2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcVidData3), cabcVidData3}
};

static struct dsi_cmd_desc toshiba_gamma25_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma25Data1), gamma25Data1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma25Data2), gamma25Data2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma25Data3), gamma25Data3},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma25Data4), gamma25Data4},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma25Data5), gamma25Data5},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma25Data6), gamma25Data6},
};

static struct dsi_cmd_desc toshiba_gamma22_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma22Data1), gamma22Data1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma22Data2), gamma22Data2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma22Data3), gamma22Data3},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma22Data4), gamma22Data4},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma22Data5), gamma22Data5},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(gamma22Data6), gamma22Data6},
};

static struct dsi_cmd_desc toshiba_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(powerOnData1), powerOnData1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(powerOnData2), powerOnData2},
#if defined(CONFIG_OVERLAY_COMPOSE)
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(blanktime), blanktime},
#endif
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(display_RGB_switch_order), display_RGB_switch_order},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(cabc_on), cabc_on},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(color_enhance), color_enhance},
	{DTYPE_DCS_WRITE, 0, 30, WAIT_TYPE_US,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc toshiba_display_off_cmds[] = {
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

static struct vcc_desc toshiba_lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000},
};

static struct vcc_desc toshiba_lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc toshiba_lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc toshiba_lcd_vcc_disable_cmds[] = {
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

static struct iomux_desc toshiba_lcd_iomux_init_cmds[] = {
	{DTYPE_IOMUX_GET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, 0},
};

static struct iomux_desc toshiba_lcd_iomux_normal_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, NORMAL},
};

static struct iomux_desc toshiba_lcd_iomux_lowpower_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, LOWPOWER},
};

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_LCD_POWER_NAME	"gpio_lcd_power"
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"
#define GPIO_LCD_ID0_NAME	"gpio_lcd_id0"
#define GPIO_LCD_ID1_NAME	"gpio_lcd_id1"

#define GPIO_LCD_POWER	GPIO_21_3
#define GPIO_LCD_RESET	GPIO_0_3
#define GPIO_LCD_ID0	GPIO_16_7
#define GPIO_LCD_ID1	GPIO_17_0

static struct gpio_desc toshiba_lcd_gpio_request_cmds[] = {
	/* power */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 0},
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

static struct gpio_desc toshiba_lcd_gpio_free_cmds[] = {
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
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

static struct gpio_desc toshiba_lcd_gpio_normal_cmds[] = {
	/* id0 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
	/* power */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 1},
};

static struct gpio_desc toshiba_lcd_gpio_lowpower_cmds[] = {
	/* id0 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
	/* power */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
};


static volatile bool g_display_on;
static struct k3_fb_panel_data toshiba_panel_data;


/*******************************************************************************
**
*/
#define TRUE_MIPI  1
#define FALSE_MIPI 0

static struct lcd_tuning_dev *p_tuning_dev = NULL;
static int cabc_mode = 1;	 /*allow application to set cabc mode to ui mode */
static int isCabcVidMode = FALSE_MIPI;

static u32 square_point_6(u32 x)
{
	unsigned long t = x * x * x;
	unsigned long t0 = 0;
	u32 i = 0, j = 255, k = 0;

	while(j - i > 1)
	{
		k = (i + j) / 2;
		t0 = k * k * k * k * k;
		if(t0 < t){
		    i = k;
		}
		else if(t0 > t){
		    j = k;
		}
		else{
		    return k;
		}
	}

	return k;
}
/* END:   Added by huohua, 2012/6/12 */

static int toshiba_set_gamma(struct lcd_tuning_dev *ltd, enum lcd_gamma gamma)
{
	int ret = 0;
	u32 edc_base = 0;
	struct platform_device *pdev = NULL;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(ltd == NULL);
	pdev = (struct platform_device *)(ltd->data);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	edc_base = k3fd->edc_base;

	switch (gamma) {
	case GAMMA25:
		mipi_dsi_cmds_tx(toshiba_gamma25_cmds, \
			ARRAY_SIZE(toshiba_gamma25_cmds), edc_base);
		break;
	case GAMMA22:
		mipi_dsi_cmds_tx(toshiba_gamma22_cmds, \
			ARRAY_SIZE(toshiba_gamma22_cmds), edc_base);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int toshiba_set_cabc(struct lcd_tuning_dev *ltd, enum  tft_cabc cabc)
{
	int ret = 0;
	u32 edc_base = 0;
	struct platform_device *pdev = NULL;
	struct k3_fb_data_type *k3fd = NULL;
	u32 bl_level = 0;
    /* END:   Added by huohua, 2012/6/12 */

	BUG_ON(ltd == NULL);
	pdev = (struct platform_device *)(ltd->data);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	edc_base = k3fd->edc_base;

	bl_level = ( k3fd->bl_level * square_point_6(k3fd->bl_level) * 100 ) / 2779;
	if(bl_level > 255){
		bl_level = 255;
	}
	/* END:   Added by huohua, 2012/6/12 */

	if (cabc_mode == 2)
		cabc = CABC_VID;

	switch (cabc) {
	case CABC_UI:
		cabcUiData3[1] = (bl_level >> 4) & 0x0f;
		cabcUiData3[2] = bl_level & 0x0f;
	    /* END:   Modified by huohua, 2012/6/12 */
		mipi_dsi_cmds_tx(toshiba_cabc_ui_cmds, \
			ARRAY_SIZE(toshiba_cabc_ui_cmds), edc_base);
		isCabcVidMode = FALSE_MIPI;
		break;
	case CABC_VID:
		cabcVidData3[1] = (bl_level >> 4) & 0x0f;
		cabcVidData3[2] = bl_level & 0x0f;
	    /* END:   Modified by huohua, 2012/6/12 */
		mipi_dsi_cmds_tx(toshiba_cabc_vid_cmds, \
			ARRAY_SIZE(toshiba_cabc_vid_cmds), edc_base);
		isCabcVidMode = TRUE_MIPI;
		break;
	case CABC_OFF:
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static struct lcd_tuning_ops sp_tuning_ops = {
	.set_gamma = toshiba_set_gamma,
	.set_cabc = toshiba_set_cabc,
};

static ssize_t toshiba_lcd_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct k3_panel_info *pinfo = NULL;

	pinfo = toshiba_panel_data.panel_info;
	sprintf(buf, "Toshiba_MDW70 4.5'TFT %d x %d\n",
		pinfo->xres, pinfo->yres);
	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t show_cabc_mode(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", cabc_mode);
}

static ssize_t store_cabc_mode(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
	unsigned long val = 0;

	ret = strict_strtoul(buf, 0, &val);
	if (ret)
		return ret;

	if (val == 1) {
		/* allow application to set cabc mode to ui mode */
		cabc_mode = 1;
		toshiba_set_cabc(p_tuning_dev, CABC_UI);
	} else if (val == 2) {
		/* force cabc mode to video mode */
		cabc_mode = 2;
		toshiba_set_cabc(p_tuning_dev, CABC_VID);
	}

	return sprintf(buf, "%d\n", cabc_mode);
}

static DEVICE_ATTR(lcd_info, S_IRUGO, toshiba_lcd_info_show, NULL);
static DEVICE_ATTR(cabc_mode, 0644, show_cabc_mode, store_cabc_mode);

static struct attribute *toshiba_attrs[] = {
	&dev_attr_lcd_info,
	&dev_attr_cabc_mode,
	NULL,
};

static struct attribute_group toshiba_attr_group = {
	.attrs = toshiba_attrs,
};

static int toshiba_sysfs_init(struct platform_device *pdev)
{
	int ret = 0;

	ret = sysfs_create_group(&pdev->dev.kobj, &toshiba_attr_group);
	if (ret) {
		k3fb_loge("create sysfs file failed!\n");
		return ret;
	}

	return 0;
}

static void toshiba_sysfs_deinit(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &toshiba_attr_group);
}


/*******************************************************************************
**
*/
static void toshiba_disp_on(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	pinfo = &(k3fd->panel_info);

	/* lcd iomux normal */
	iomux_cmds_tx(toshiba_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(toshiba_lcd_iomux_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(toshiba_lcd_gpio_request_cmds, \
		ARRAY_SIZE(toshiba_lcd_gpio_request_cmds));
	/* lcd gpio normal */
	gpio_cmds_tx(toshiba_lcd_gpio_normal_cmds, \
		ARRAY_SIZE(toshiba_lcd_gpio_normal_cmds));

	/* lcd display on sequence */
	mipi_dsi_cmds_tx(toshiba_display_on_cmds, \
		ARRAY_SIZE(toshiba_display_on_cmds), edc_base);

	if (TRUE_MIPI == isCabcVidMode) {
		cabcVidData3[1] = 0x00;
		cabcVidData3[2] = 0x00;
		mipi_dsi_cmds_tx(toshiba_cabc_vid_cmds, \
			ARRAY_SIZE(toshiba_cabc_vid_cmds), edc_base);
	} else {
		/* Default is UI mode. */
		cabcUiData3[1] = 0x00;
		cabcUiData3[2] = 0x00;
		mipi_dsi_cmds_tx(toshiba_cabc_ui_cmds, \
			ARRAY_SIZE(toshiba_cabc_ui_cmds), edc_base);
	}
}

static void toshiba_disp_off(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	pinfo = &(k3fd->panel_info);

	/* lcd display off sequence */
	mipi_dsi_cmds_tx(toshiba_display_off_cmds, \
		ARRAY_SIZE(toshiba_display_off_cmds), edc_base);

	/* lcd gpio lowpower */
	gpio_cmds_tx(toshiba_lcd_gpio_lowpower_cmds, \
		ARRAY_SIZE(toshiba_lcd_gpio_lowpower_cmds));
	/* lcd gpio free */
	gpio_cmds_tx(toshiba_lcd_gpio_free_cmds, \
		ARRAY_SIZE(toshiba_lcd_gpio_free_cmds));

	/* lcd iomux lowpower */
	iomux_cmds_tx(toshiba_lcd_iomux_lowpower_cmds, \
		ARRAY_SIZE(toshiba_lcd_iomux_lowpower_cmds));

	/* lcd vcc disable */
	vcc_cmds_tx(NULL, toshiba_lcd_vcc_disable_cmds, \
		ARRAY_SIZE(toshiba_lcd_vcc_disable_cmds));
}

static int mipi_toshiba_panel_on(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	pinfo = &(k3fd->panel_info);
	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		/* lcd vcc enable */
		vcc_cmds_tx(NULL, toshiba_lcd_vcc_enable_cmds, \
			ARRAY_SIZE(toshiba_lcd_vcc_enable_cmds));

		pinfo->lcd_init_step = 	LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		if (!g_display_on) {
			/* lcd display on */
			toshiba_disp_on(k3fd);
		}
		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {

		g_display_on = true;
	} else {
		k3fb_loge("failed to init lcd!\n");
	}

	return 0;
}

static int mipi_toshiba_panel_off(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	if (g_display_on) {
		g_display_on = false;
		/* lcd display off */
		toshiba_disp_off(k3fd);
	}

	return 0;
}

static int mipi_toshiba_panel_remove(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	k3fb_logd("index=%d, enter!\n", k3fd->index);

	/* lcd vcc finit */
	vcc_cmds_tx(pdev, toshiba_lcd_vcc_finit_cmds, \
		ARRAY_SIZE(toshiba_lcd_vcc_finit_cmds));

	toshiba_sysfs_deinit(pdev);

	k3fb_logd("index=%d, exit!\n", k3fd->index);

	return 0;
}

static int mipi_toshiba_panel_set_backlight(struct platform_device *pdev)
{
	int ret = 0;
	u32 edc_base = 0;
	u32 bl_h = 0;
	u32 bl_l = 0;
	u32 bl_level = 0;
    /* END:   Added by huohua, 2012/6/12 */
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;

	//Let the backlight brightness to adjust way by the curve transformation,
	//to better meet the human visual characteristics.
	 //Y = (X / 255) ^ 1.6 * 255
	bl_level = ( k3fd->bl_level * square_point_6(k3fd->bl_level) * 100 ) / 2779;
	if(bl_level > 255){
		bl_level = 255;
	}
	bl_h = (((bl_level >> 4) & 0x0f) << 8) & 0x00FF00;
	bl_l = ((bl_level & 0x0f) << 16) & 0xFF0000;
	/* END:   Modified by huohua, 2012/6/12 */

	if (TRUE_MIPI == isCabcVidMode) {
		outp32(edc_base + MIPIDSI_GEN_PLD_DATA_OFFSET, (bl_h | bl_l | 0x000000be));
		outp32(edc_base + MIPIDSI_GEN_PLD_DATA_OFFSET, 0x400418);
	} else {
		outp32(edc_base + MIPIDSI_GEN_PLD_DATA_OFFSET, (bl_h | bl_l | 0x020000be));
		outp32(edc_base + MIPIDSI_GEN_PLD_DATA_OFFSET, 0x040402);
	}
	outp32(edc_base + MIPIDSI_GEN_PLD_DATA_OFFSET, 0x5d);
	outp32(edc_base + MIPIDSI_GEN_HDR_OFFSET, 0x929);

	return ret;
}

static int mipi_toshiba_panel_set_fastboot(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	/* lcd vcc enable */
	vcc_cmds_tx(pdev, toshiba_lcd_vcc_enable_cmds, \
		ARRAY_SIZE(toshiba_lcd_vcc_enable_cmds));

	/* lcd iomux normal */
	iomux_cmds_tx(toshiba_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(toshiba_lcd_iomux_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(toshiba_lcd_gpio_request_cmds, \
		ARRAY_SIZE(toshiba_lcd_gpio_request_cmds));

	g_display_on = true;

	return 0;
}

static int mipi_toshiba_panel_set_cabc(struct platform_device *pdev, int value)
{
	u32 edc_base = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;

	if (value) {
		outp32(edc_base + MIPIDSI_GEN_HDR_OFFSET, 0x0dbb23);
	} else {
		outp32(edc_base + MIPIDSI_GEN_HDR_OFFSET, 0x0cbb23);
	}

	return 0;
}

static struct k3_panel_info toshiba_panel_info = {0};
static struct k3_fb_panel_data toshiba_panel_data = {
	.panel_info = &toshiba_panel_info,
	.on = mipi_toshiba_panel_on,
	.off = mipi_toshiba_panel_off,
	.remove = mipi_toshiba_panel_remove,
	.set_backlight = mipi_toshiba_panel_set_backlight,
	.set_fastboot = mipi_toshiba_panel_set_fastboot,
	.set_cabc = mipi_toshiba_panel_set_cabc,
};


/*******************************************************************************
**
*/
static int __devinit toshiba_probe(struct platform_device *pdev)
{
	struct k3_panel_info *pinfo = NULL;
	struct platform_device *reg_pdev = NULL;
	struct lcd_properities lcd_props;

	g_display_on = false;

	pinfo = toshiba_panel_data.panel_info;
	/* init lcd panel info */
	pinfo->xres = 720;
	pinfo->yres = 1280;
	pinfo->width = 55;
	pinfo->height = 98;
	pinfo->type = PANEL_MIPI_VIDEO;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = EDC_OUT_RGB_888;
	pinfo->s3d_frm = EDC_FRM_FMT_2D;
	pinfo->bgr_fmt = EDC_RGB;
	pinfo->bl_set_type = BL_SET_BY_MIPI;
	pinfo->bl_min = 1;
	pinfo->bl_max = 100;

	pinfo->frc_enable = 1;
	pinfo->esd_enable = 1;
	pinfo->sbl_enable = 1;

	pinfo->sbl.bl_max = 0xff;
	pinfo->sbl.cal_a = 0x0f;
	pinfo->sbl.cal_b = 0xd8;
	pinfo->sbl.str_limit = 0x40;

	pinfo->ldi.h_back_porch = 53;
#if defined(CONFIG_OVERLAY_COMPOSE)
	pinfo->ldi.h_front_porch = 134;
#else
	pinfo->ldi.h_front_porch = 133;
#endif
	pinfo->ldi.h_pulse_width = 67;
#if defined(CONFIG_OVERLAY_COMPOSE)
	pinfo->ldi.v_back_porch = 12;
	pinfo->ldi.v_front_porch = 7;
	pinfo->ldi.v_pulse_width = 1;
#else
	pinfo->ldi.v_back_porch = 4;
	pinfo->ldi.v_front_porch = 15;
	pinfo->ldi.v_pulse_width = 2;
#endif
	pinfo->ldi.hsync_plr = 1;
	pinfo->ldi.vsync_plr = 1;
	pinfo->ldi.pixelclk_plr = 0;
	pinfo->ldi.data_en_plr = 0;
	pinfo->ldi.disp_mode = LDI_DISP_MODE_NOT_3D_FBF;

	/* Note: must init here */
	pinfo->frame_rate = 60;
	pinfo->clk_rate = 76000000;

	pinfo->mipi.lane_nums = DSI_4_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.dsi_bit_clk = 241;

	/* lcd vcc init */
	vcc_cmds_tx(pdev, toshiba_lcd_vcc_init_cmds, \
		ARRAY_SIZE(toshiba_lcd_vcc_init_cmds));

	/* lcd iomux init */
	iomux_cmds_tx(toshiba_lcd_iomux_init_cmds, \
		ARRAY_SIZE(toshiba_lcd_iomux_init_cmds));

	/* alloc panel device data */
	if (platform_device_add_data(pdev, &toshiba_panel_data,
		sizeof(struct k3_fb_panel_data))) {
		k3fb_loge("failed to platform_device_add_data!\n");
		platform_device_put(pdev);
		return -ENOMEM;
	}

	reg_pdev = k3_fb_add_device(pdev);

	/* for cabc */
	lcd_props.type = TFT;
	lcd_props.default_gamma = GAMMA25;
	p_tuning_dev = lcd_tuning_dev_register(&lcd_props, &sp_tuning_ops, (void *)reg_pdev);
	if (IS_ERR(p_tuning_dev)) {
		p_tuning_dev = NULL;
		k3fb_loge("lcd_tuning_dev_register failed!\n");
		return -1;
	}

	toshiba_sysfs_init(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe = toshiba_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_toshiba_MDW70_V001",
	},
};

static int __init mipi_toshiba_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		k3fb_loge("not able to register the driver, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(mipi_toshiba_panel_init);
