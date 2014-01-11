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
#include <linux/lcd_tuning.h>
#include <mach/platform.h>
#include <mach/gpio.h>
#include <mach/boardid.h>

#include "k3_fb.h"
#include "k3_fb_def.h"
#include "mipi_dsi.h"
#include "mipi_reg.h"


/*******************************************************************************
** Power ON Sequence(sleep mode to Normal mode)
*/

static char MCAP[] = {
	0xB0,
	0x00,
};

static char set_address[] = {
	0x36,
	0xC0,
};

static char bl_level[] = {
	0x51,
	0x00,
	0x00,
};

static char bl_enable[] = {
	0x53,
	0x24,
};

#if defined(CONFIG_OVERLAY_COMPOSE)
static char display_setting2[] = {
    0xc2,
    0x31, 0xF7, 0x80, 0x10, 0x08, 0x00, 0x00,
};

static char nvm_write[] = {
    0xD6, 0x01,
};
#endif

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

/* BASE Sequence */
static char MCAP_param1[] = {
	0xb0,
	0x04,
};

static char MCAP_param2[] = {
	0xb0,
	0x03,
};

/* CABC BASE Sequence */
static char cabc_base_param2[] = {
	0x53,
	0x2c,
};

/* R63311 inter setting */
static char cabc_inter_param[] = {
	0xb8,
	0x18, 0x80, 0x18, 0x18, 0xcf,
	0x1f, 0x00, 0x0c, 0x12, 0x6c,
	0x11, 0x6c, 0x12, 0x0c, 0x12,
	0xda, 0x6d, 0xff, 0xff, 0x10,
	0x67, 0xa3, 0xdb, 0xfb, 0xff,
};

/* CABC GUI Sequence */
static char cabc_ui_param[] = {
	0xba,
	0x00, 0x30, 0x04, 0x40, 0x9f,
	0x1f, 0xB0,
};

static char cabc_ui_on[] = {
	0x55,
	0x01,
};

/* CABC MOVIE Sequence */
static char cabc_video_param[] = {
	0xb9,
	0x00, 0x30, 0x18, 0x18, 0x9f,
	0x1f, 0x80,
};
static char cabc_video_on[] = {
	0x55,
	0x03,
};

/* CABC PWM Sequence */
static char cabc_pwm_param[] = {
	0xce,
	0x00, 0x07, 0x00, 0xc0, 0x40,
	0xb2, 0x02,
};

/* CE Sequence */
static char ce_param[] = {
	0xca,
	0x01, 0x80, 0xdc, 0x80, 0xec,
	0x80, 0x90, 0xa0, 0x08, 0x10,
	0x14, 0xab, 0x0a, 0x4a, 0x37,
	0xa0, 0x55, 0xf8, 0x0c, 0x10,
	0x20, 0x10, 0x3f, 0x3f, 0x19,
	0xd6, 0x10, 0x10, 0x3f, 0x3f,
	0x3f, 0x3f,
};

/* Sharpness Sequence */
static char sharpness_param[] = {
	0xdd,
	0x11, 0x93, /*ADST 150%->50%, AVST 100%->50%*/
};

static struct dsi_cmd_desc toshiba_cabc_ui_cmds[] = {
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(MCAP_param1), MCAP_param1},
	{DTYPE_DCS_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(cabc_base_param2), cabc_base_param2},
	{DTYPE_GEN_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(cabc_ui_param), cabc_ui_param},
	{DTYPE_GEN_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(cabc_inter_param), cabc_inter_param},
	{DTYPE_GEN_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(cabc_pwm_param), cabc_pwm_param},
	{DTYPE_DCS_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(cabc_ui_on), cabc_ui_on},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(MCAP_param2), MCAP_param2},
};

static struct dsi_cmd_desc toshiba_cabc_video_cmds[] = {
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(MCAP_param1), MCAP_param1},
//	{DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_MS,
//		sizeof(cabc_base_param1), cabc_base_param1},
	{DTYPE_DCS_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(cabc_base_param2), cabc_base_param2},
	{DTYPE_GEN_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(cabc_video_param), cabc_video_param},
	{DTYPE_GEN_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(cabc_inter_param), cabc_inter_param},
	{DTYPE_GEN_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(cabc_pwm_param), cabc_pwm_param},
	{DTYPE_DCS_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(cabc_video_on), cabc_video_on},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(MCAP_param2), MCAP_param2},
};

static struct dsi_cmd_desc toshiba_sharpness_cmds[] = {
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(MCAP_param1), MCAP_param1},
	{DTYPE_GEN_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(sharpness_param), sharpness_param},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(MCAP_param2), MCAP_param2},
};

static struct dsi_cmd_desc toshiba_ce_cmds[] = {
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(MCAP_param1), MCAP_param1},
	{DTYPE_GEN_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(ce_param), ce_param},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(MCAP_param2), MCAP_param2},
};

static struct dsi_cmd_desc toshiba_display_on_cmd[] = {
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(MCAP), MCAP},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(set_address), set_address},
	{DTYPE_DCS_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(bl_level), bl_level},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
#if defined(CONFIG_OVERLAY_COMPOSE)
	{DTYPE_GEN_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(display_setting2), display_setting2},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(nvm_write), nvm_write},
#endif
	{DTYPE_DCS_WRITE, 0, 5, WAIT_TYPE_MS,
		sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc toshiba_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 20, WAIT_TYPE_MS,
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
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"
#define GPIO_LCD_POWER_NAME	"gpio_lcd_power"
#define GPIO_LCD_ID0_NAME	"gpio_lcd_id0"
#define GPIO_LCD_ID1_NAME	"gpio_lcd_id1"

#define GPIO_LCD_POWER	GPIO_21_3
#define GPIO_LCD_RESET	GPIO_0_3
#define GPIO_LCD_ID0	GPIO_16_6
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
	/* power */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 1},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
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
static struct lcd_tuning_dev *p_tuning_dev = NULL;

static int toshiba_set_gamma(struct lcd_tuning_dev *ltd, enum lcd_gamma gamma)
{
	/* Fix me: Implement it */
	return 0;
}

static int toshiba_set_cabc(struct lcd_tuning_dev *ltd, enum  tft_cabc cabc)
{
	int ret = 0;
	u32 edc_base = 0;
	struct platform_device *pdev = NULL;
	struct k3_fb_data_type *k3fd = NULL;

	return 0; /*will re-implement it later*/

	BUG_ON(ltd == NULL);
	pdev = (struct platform_device *)(ltd->data);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	edc_base = k3fd->edc_base;

/*	switch (cabc) {
	case CABC_UI:
		mipi_dsi_cmds_tx(toshiba_cabc_ui_cmds, \
			ARRAY_SIZE(toshiba_cabc_ui_cmds), edc_base);
		break;
	case CABC_VID:
		mipi_dsi_cmds_tx(toshiba_cabc_video_cmds, \
			ARRAY_SIZE(toshiba_cabc_video_cmds), edc_base);
		break;
	case CABC_OFF:
		break;
	default:
		ret = -1;
		break;
	}*/

	return ret;
}

/*wugao 00190753 modified for color temperature begin*/
static unsigned int g_csc_value[9];
static unsigned int g_is_csc_set;
static struct semaphore ct_sem;

static void toshiba_store_ct_cscValue(unsigned int csc_value[])
{
    down(&ct_sem);
    g_csc_value [0] = csc_value[0];
    g_csc_value [1] = csc_value[1];
    g_csc_value [2] = csc_value[2];
    g_csc_value [3] = csc_value[3];
    g_csc_value [4] = csc_value[4];
    g_csc_value [5] = csc_value[5];
    g_csc_value [6] = csc_value[6];
    g_csc_value [7] = csc_value[7];
    g_csc_value [8] = csc_value[8];
    g_is_csc_set = 1;
    up(&ct_sem);

    return;
}

static int toshiba_set_ct_cscValue(struct k3_fb_data_type *k3fd)
{
     u32 edc_base = 0;
    edc_base = k3fd->edc_base;
    down(&ct_sem);
    if(1 == g_is_csc_set)
    {

        set_reg(edc_base + 0x400, 0x1, 1, 27);

        set_reg(edc_base + 0x408, g_csc_value[0], 13, 0);
        set_reg(edc_base + 0x408, g_csc_value[1], 13, 16);
        set_reg(edc_base + 0x40C, g_csc_value[2], 13, 0);
        set_reg(edc_base + 0x40C, g_csc_value[3], 13, 16);
        set_reg(edc_base + 0x410, g_csc_value[4], 13, 0);
        set_reg(edc_base + 0x410, g_csc_value[5], 13, 16);
        set_reg(edc_base + 0x414, g_csc_value[6], 13, 0);
        set_reg(edc_base + 0x414, g_csc_value[7], 13, 16);
        set_reg(edc_base + 0x418, g_csc_value[8], 13, 0);
    }
    up(&ct_sem);


     return 0;
}

/*wugao 00190753 modified for color temperature end*/

/* BEGIN: Added by wugao 00190753*/

static int toshiba_set_color_temperature(struct lcd_tuning_dev *ltd, unsigned int csc_value[])
{
	/*wugao 00190753 modified for color temperature begin*/
    int flag = 0;
    struct platform_device *pdev;
    struct k3_fb_data_type *k3fd;

    if (ltd == NULL)
    {
        return -1;
    }
    pdev  = (struct platform_device *)(ltd->data);
    k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);

    if (k3fd == NULL)
    {
        return -1;
    }

    toshiba_store_ct_cscValue(csc_value);
    flag = toshiba_set_ct_cscValue(k3fd);
    return flag;
	/*wugao 00190753 modified for color temperature end*/
}

/* END:   add by wugao 00190753*/

static struct lcd_tuning_ops sp_tuning_ops = {
	.set_gamma = toshiba_set_gamma,
	.set_cabc = toshiba_set_cabc,
	.set_color_temperature = toshiba_set_color_temperature
};

static ssize_t toshiba_lcd_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct k3_panel_info *pinfo = NULL;

	pinfo = toshiba_panel_data.panel_info;
	sprintf(buf, "Toshiba 5.0' FHD TFT %d x %d\n",
		pinfo->xres, pinfo->yres);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(lcd_info, S_IRUGO, toshiba_lcd_info_show, NULL);

bool sbl_low_power_mode = false;
static ssize_t toshiba_sbl_low_power_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	if(sbl_low_power_mode)
	{
		sprintf(buf, "Toshiba 5.0' FHD TFT sbl low power mode is enable.\n");
	}
	else
	{
		sprintf(buf, "Toshiba 5.0' FHD TFT sbl low power mode is disable.\n");
	}
	ret = strlen(buf) + 1;
	return ret;
}

static ssize_t toshiba_sbl_low_power_mode_store(struct device *dev,
			     struct device_attribute *devattr,
			     const char *buf, size_t count)
{
	long m_sbl_low_power_mode = simple_strtol(buf, NULL, 10) != 0;
	sbl_low_power_mode =(bool)m_sbl_low_power_mode;
	return count;
}

static DEVICE_ATTR(sbl_low_power_mode, 0664,
	toshiba_sbl_low_power_mode_show, toshiba_sbl_low_power_mode_store);

static struct attribute *toshiba_attrs[] = {
	&dev_attr_lcd_info.attr,
	&dev_attr_sbl_low_power_mode.attr,
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

static void toshiba_disp_on(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	pinfo = &(k3fd->panel_info);

	/*wugao 00190753 modified for color temperature begin*/
    toshiba_set_ct_cscValue(k3fd);
	/*wugao 00190753 modified for color temperature end*/

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
	mipi_dsi_cmds_tx(toshiba_display_on_cmd, \
		ARRAY_SIZE(toshiba_display_on_cmd), edc_base);

	mipi_dsi_cmds_tx(toshiba_sharpness_cmds, \
		ARRAY_SIZE(toshiba_sharpness_cmds), edc_base);

	mipi_dsi_cmds_tx(toshiba_ce_cmds, \
		ARRAY_SIZE(toshiba_ce_cmds), edc_base);
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

	/* lcd vcc finit */
	vcc_cmds_tx(pdev, toshiba_lcd_vcc_finit_cmds, \
		ARRAY_SIZE(toshiba_lcd_vcc_finit_cmds));

	toshiba_sysfs_deinit(pdev);

	return 0;
}

static int mipi_toshiba_panel_set_backlight(struct platform_device *pdev)
{
	u32 edc_base = 0;
	struct k3_fb_data_type *k3fd = NULL;
	char level[3] = { 0x51, 0x00, };
	struct dsi_cmd_desc cabc_pwm[] = {{DTYPE_DCS_LWRITE, 0, 100, WAIT_TYPE_US,
		sizeof(level), level}};

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;

	//backlight may turn off when bl_level is below 6.
	if(k3fd->bl_level < 6 && k3fd->bl_level !=0)
		k3fd->bl_level = 6;
	level[2] = (char)k3fd->bl_level;

	if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		return pwm_set_backlight(k3fd, &g_display_on);
	}
	else {
		mipi_dsi_cmds_tx(cabc_pwm, ARRAY_SIZE(cabc_pwm), edc_base);
		return 0;
	}
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

	/* Fix me: Implement it */

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
	pinfo->xres = 1080;
	pinfo->yres = 1920;
	pinfo->width = 62;
	pinfo->height = 110;
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
	pinfo->sbl.cal_a = 0x08;
	pinfo->sbl.cal_b = 0xd8;
	pinfo->sbl.str_limit = 0x40;

	pinfo->ldi.h_back_porch = 40;
#if defined(CONFIG_OVERLAY_COMPOSE)
	pinfo->ldi.h_front_porch = 81;
#else
	pinfo->ldi.h_front_porch = 90;
#endif
	pinfo->ldi.h_pulse_width = 30;
#if defined(CONFIG_OVERLAY_COMPOSE)
	pinfo->ldi.v_back_porch = 12;
#else
	pinfo->ldi.v_back_porch = 4;
#endif
	pinfo->ldi.v_front_porch = 8;
	pinfo->ldi.v_pulse_width = 4;

	pinfo->ldi.hsync_plr = 1;
	pinfo->ldi.vsync_plr = 1;
	pinfo->ldi.pixelclk_plr = 0;
	pinfo->ldi.data_en_plr = 0;
	pinfo->ldi.disp_mode = LDI_DISP_MODE_NOT_3D_FBF;

	/* Note: must init here */
	pinfo->frame_rate = 60;
	pinfo->clk_rate = 144000000;
	pinfo->mipi.lane_nums = DSI_4_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.dsi_bit_clk = 481;

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

	/* wugao 00190753 add for color temperature begin*/
	sema_init(&ct_sem, 1);
	g_csc_value[0] = 0;
	g_csc_value[1] = 0;
	g_csc_value[2] = 0;
	g_csc_value[3] = 0;
	g_csc_value[4] = 0;
	g_csc_value[5] = 0;
	g_csc_value[6] = 0;
	g_csc_value[7] = 0;
	g_csc_value[8] = 0;
	g_is_csc_set = 0;
	/* wugao 00190753 add for color temperature begin*/

	p_tuning_dev = lcd_tuning_dev_register(&lcd_props, &sp_tuning_ops, (void *)reg_pdev);
	if (IS_ERR(p_tuning_dev)) {
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
		.name = "mipi_toshiba_MDY90",
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
