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

#include "k3_fb.h"
#include "k3_fb_def.h"
#include "mipi_dsi.h"


/*******************************************************************************
** Power ON Sequence(sleep mode to Normal mode)
*/
/*
** Initial condition (RESET="L")
**
** VCI ON Logic Voltage
** WAIT (10ms) For Power Stable
** VDD5 ON Analog Voltage
** WAIT (10ms) For Power Stable
** Reset release (Reset=H)
** WAIT MIN 5ms
*/

/* Sleep out */
static char exit_sleep[] = {
	0x11,
};
/* WAIT MIN 120ms*/

/* Register bank select */
static char powerOnData1[] = {
	0xB0,
	0x00,
};

static char powerOnData2[] = {
	0xE6,
	0x12, 0x04, 0x00, 0x05,
	0x07, 0x03, 0xFF, 0x10,
	0xFF, 0xFF,
};

static char powerOnData3[] = {
	0xC6,
	0x05,
};

/* Power ON sequence setting B */
static char powerOnData4[] = {
	0x99,
	0x2B, 0x51,
};

/* Power on sequence setting A */
static char powerOnData5[] = {
	0x98,
	0x01, 0x05, 0x06, 0x0A,
	0x18, 0x0E, 0x22, 0x23,
	0x24,
};

/* Power off sequence setting A */
static char powerOnData6[] = {
	0x9B,
	0x02, 0x06, 0x08, 0x0A,
	0x0C, 0x01,
};

/* Red Gamma Sets in Positive */
static char powerOnData7[] = {
	0xA2,
	0x00, 0x2D, 0x0E, 0x05,
	0xF9, 0x87, 0x66, 0x05,
};

/* Red Gamma Sets in Negative */
static char powerOnData8[] = {
	0xA3,
	0x00, 0x2D, 0x0E, 0x05,
	0xF9, 0x87, 0x66, 0x05,
};

/* Green Gamma Sets in Positive */
static char powerOnData9[] = {
	0xA4,
	0x04, 0x2D, 0x0E, 0x05,
	0xF9, 0x87, 0x66, 0x05,
};

/* Green Gamma Sets in Negative */
static char powerOnData10[] = {
	0xA5,
	0x04, 0x2D, 0x0E, 0x05,
	0xF9, 0x87, 0x66, 0x05,
};

/* Blue Gamma Sets in Positive */
static char powerOnData11[] = {
	0xA6,
	0x02, 0x30, 0x13, 0x46,
	0x2C, 0xA9, 0x76, 0x06,
};

/* Blue Gamma Sets in Negative */
static char powerOnData12[] = {
	0xA7,
	0x02, 0x30, 0x13, 0x46,
	0x2C, 0xA9, 0x76, 0x06,
};

/* SETVGMPM: This command set the voltage of VRMP ,VRMM. */
static char powerOnData13[] = {
	0xB4,
	0x68,
};

/* RBIAS1: */
static char powerOnData14[] = {
	0xB5,
	0x33, 0x03,
};

/* SELMODE: */
static char powerOnData15[] = {
	0xB6,
	0x02,
};

/* SET_DDVDHP: */
static char powerOnData16[] = {
	0xB7,
	0x08, 0x44, 0x06, 0x2E,
	0x00, 0x00, 0x30, 0x33,
};

/* SET_DDVDHM: */
static char powerOnData17[] = {
	0xB8,
	0x1F, 0x44, 0x10, 0x2E,
	0x1F, 0x00, 0x30, 0x33,
};

/* SET_VGH: */
static char powerOnData18[] = {
	0xB9,
	0x48, 0x11, 0x01, 0x00,
	0x30,
};

/* SET_VGL */
static char powerOnData19[] = {
	0xBA,
	0x4F, 0x11, 0x00, 0x00,
	0x30,
};

/* SET_VCL */
static char powerOnData20[] = {
	0xBB,
	0x11, 0x01, 0x00, 0x30,
};

/* TVBP */
static char powerOnData21[] = {
	0xBC,
	0x06,
};

/* THDEHBP */
static char powerOnData22[] = {
	0xBF,
	0x80,
};

static char powerOnData23[] = {
	0xB0,
	0x01,
};

/* If BANK(B1h) = 1, then this registers are able to be written or read. */
static char powerOnData24[] = {
	0xC0,
	0xC8,
};

static char powerOnData25[] = {
	0xC2,
	0x00,
};

static char powerOnData26[] = {
	0xC3,
	0x00,
};

static char powerOnData27[] = {
	0xC4,
	0x12,
};

static char powerOnData28[] = {
	0xC5,
	0x24,
};

static char powerOnData29[] = {
	0xC8,
	0x00,
};

/* Adjust the start position of SSD. */
static char powerOnData30[] = {
	0xCA,
	0x12,
};

/* Adjust the interval of SSD. */
static char powerOnData31[] = {
	0xCC,
	0x12,
};

/* Blanking Period, Partial non-display area setting. */
static char powerOnData32[] = {
	0xD4,
	0x00,
};

static char powerOnData33[] = {
	0xDC,
	0x20,
};

/* VALGO */
static char powerOnData34[] = {
	0x96,
	0x01,
};

#if 0
/* RDDCOLMODE
  * 0x55:16bits 0x66:18bits 0x77:24bits */
static char color_mode_16bits[] = {
	0x0C,
	0x55,
};

static char color_mode_18bits[] = {
	0x0C,
	0x66,
};
#endif

static char powerOnData35[] = {
	0x0C,
	0x77,
};

/* MADCTL */
static char powerOnData36[] = {
	0x36,
	0x03,
};

/* Display On */
static char display_on[] = {
	0x29,
};

/*******************************************************************************
** Power OFF Sequence(Normal to power off)
*/
/* Display Off */
static char display_off[] = {
	0x28,
};

/* Sleep In */
static char enter_sleep[] = {
	0x10,
};
/* WAIT MIN 120ms For Power Down */

/*
** Reset release (Reset=L)
** WAIT (10ms)
** VDD5OFF Analog Voltage
** WAIT (10ms) For Power Stable
** VCI OFF Logic Voltage
*/

static struct dsi_cmd_desc sharp_display_on_cmds[] = {
	{DTYPE_GEN_WRITE1, 0, 120, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},

	{DTYPE_GEN_WRITE2, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData1), powerOnData1},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData2), powerOnData2},
	{DTYPE_GEN_WRITE2, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData3), powerOnData3},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData4), powerOnData4},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData5), powerOnData5},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData6), powerOnData6},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData7), powerOnData7},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData8), powerOnData8},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData9), powerOnData9},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData10), powerOnData10},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData11), powerOnData11},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData12), powerOnData12},
	{DTYPE_GEN_WRITE2, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData13), powerOnData13},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData14), powerOnData14},
	{DTYPE_GEN_WRITE2, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData15), powerOnData15},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData16), powerOnData16},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData17), powerOnData17},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData18), powerOnData18},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData19), powerOnData19},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData20), powerOnData20},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData21), powerOnData21},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData22), powerOnData22},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData23), powerOnData23},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData24), powerOnData24},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData25), powerOnData25},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData26), powerOnData26},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData27), powerOnData27},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData28), powerOnData28},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData29), powerOnData29},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData30), powerOnData30},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData31), powerOnData31},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData32), powerOnData32},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData33), powerOnData33},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData34), powerOnData34},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData35), powerOnData35},
	{DTYPE_GEN_LWRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(powerOnData36), powerOnData36},

	{DTYPE_GEN_WRITE1, 0, 1, WAIT_TYPE_MS,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sharp_display_off_cmds[] = {
	{DTYPE_GEN_WRITE1, 0, 1, WAIT_TYPE_MS,
		sizeof(display_off), display_off},
	{DTYPE_GEN_WRITE1, 0, 120, WAIT_TYPE_MS,
		sizeof(enter_sleep), enter_sleep}
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME		"lcdio-vcc"
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcdanalog;

static struct vcc_desc sharp_lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000},
};

static struct vcc_desc sharp_lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc sharp_lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc sharp_lcd_vcc_disable_cmds[] = {
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

static struct iomux_desc sharp_lcd_iomux_init_cmds[] = {
	{DTYPE_IOMUX_GET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, 0},
};

static struct iomux_desc sharp_lcd_iomux_normal_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, NORMAL},
};

static struct iomux_desc sharp_lcd_iomux_lowpower_cmds[] = {
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

static struct gpio_desc sharp_lcd_gpio_request_cmds[] = {
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

static struct gpio_desc sharp_lcd_gpio_free_cmds[] = {
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

static struct gpio_desc sharp_lcd_gpio_normal_cmds[] = {
	/* id0 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
	/* power */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 1},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
};

static struct gpio_desc sharp_lcd_gpio_lowpower_cmds[] = {
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

/*******************************************************************************
** PWM IOMUX
*/
#define IOMUX_PWM_NAME	"block_lcd"

static struct iomux_block **pwm_block;
static struct block_config **pwm_block_config;

static struct iomux_desc sharp_pwm_iomux_init_cmds[] = {
	{DTYPE_IOMUX_GET, IOMUX_PWM_NAME,
		(struct iomux_block **)&pwm_block, (struct block_config **)&pwm_block_config, 0},
};

static struct iomux_desc sharp_pwm_iomux_normal_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_PWM_NAME,
		(struct iomux_block **)&pwm_block, (struct block_config **)&pwm_block_config, NORMAL},
};

static struct iomux_desc sharp_pwm_iomux_lowpower_cmds[] = {
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

static struct gpio_desc sharp_pwm_gpio_request_cmds[] = {
	/* pwm0 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM0_NAME, GPIO_PWM0, 0},
	/* pwm1 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};

static struct gpio_desc sharp_pwm_gpio_free_cmds[] = {
	/* pwm0 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM0_NAME, GPIO_PWM0, 0},
	/* pwm1 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};

static struct gpio_desc sharp_pwm_gpio_normal_cmds[] = {
	/* pwm1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};

static struct gpio_desc sharp_pwm_gpio_lowpower_cmds[] = {
	/* pwm0 */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_PWM0_NAME, GPIO_PWM0, 0},
	/* pwm1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};

static volatile bool g_display_on;
static struct k3_fb_panel_data sharp_panel_data;


/*******************************************************************************
**
*/
static int sharp_pwm_on(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

	/* pwm iomux normal */
	iomux_cmds_tx(sharp_pwm_iomux_normal_cmds, \
		ARRAY_SIZE(sharp_pwm_iomux_normal_cmds));

	/* pwm gpio request */
	gpio_cmds_tx(sharp_pwm_gpio_request_cmds, \
		ARRAY_SIZE(sharp_pwm_gpio_request_cmds));
	/* pwm gpio free */
	gpio_cmds_tx(sharp_pwm_gpio_normal_cmds, \
		ARRAY_SIZE(sharp_pwm_gpio_normal_cmds));

	/* backlight on */
	pwm_set_backlight(k3fd, &g_display_on);

	return 0;
}

static int sharp_pwm_off(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

	/* backlight off */
	pwm_set_backlight(k3fd, &g_display_on);

	/* pwm gpio lowpower */
	gpio_cmds_tx(sharp_pwm_gpio_lowpower_cmds, \
		ARRAY_SIZE(sharp_pwm_gpio_lowpower_cmds));
	/* pwm gpio free */
	gpio_cmds_tx(sharp_pwm_gpio_free_cmds, \
		ARRAY_SIZE(sharp_pwm_gpio_free_cmds));

	/* pwm iomux lowpower */
	iomux_cmds_tx(sharp_pwm_iomux_lowpower_cmds, \
		ARRAY_SIZE(sharp_pwm_iomux_lowpower_cmds));

	return 0;
}

static void sharp_disp_on(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	pinfo = &(k3fd->panel_info);

	/* lcd iomux normal */
	iomux_cmds_tx(sharp_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(sharp_lcd_iomux_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(sharp_lcd_gpio_request_cmds, \
		ARRAY_SIZE(sharp_lcd_gpio_request_cmds));
	/* lcd gpio normal */
	gpio_cmds_tx(sharp_lcd_gpio_normal_cmds, \
		ARRAY_SIZE(sharp_lcd_gpio_normal_cmds));

	/* lcd display on sequence */
	mipi_dsi_cmds_tx(sharp_display_on_cmds, \
		ARRAY_SIZE(sharp_display_on_cmds), edc_base);
}

static void sharp_disp_off(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	pinfo = &(k3fd->panel_info);

	/* lcd display off sequence */
	mipi_dsi_cmds_tx(sharp_display_off_cmds, \
		ARRAY_SIZE(sharp_display_off_cmds), edc_base);

	/* lcd gpio lowpower */
	gpio_cmds_tx(sharp_lcd_gpio_lowpower_cmds, \
		ARRAY_SIZE(sharp_lcd_gpio_lowpower_cmds));
	/* lcd gpio free */
	gpio_cmds_tx(sharp_lcd_gpio_free_cmds, \
		ARRAY_SIZE(sharp_lcd_gpio_free_cmds));

	/* lcd iomux lowpower */
	iomux_cmds_tx(sharp_lcd_iomux_lowpower_cmds, \
		ARRAY_SIZE(sharp_lcd_iomux_lowpower_cmds));

	/* lcd vcc disable */
	vcc_cmds_tx(NULL, sharp_lcd_vcc_disable_cmds, \
		ARRAY_SIZE(sharp_lcd_vcc_disable_cmds));
}

static int mipi_sharp_panel_on(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	pinfo = &(k3fd->panel_info);
	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		/* lcd vcc enable */
		vcc_cmds_tx(NULL, sharp_lcd_vcc_enable_cmds, \
			ARRAY_SIZE(sharp_lcd_vcc_enable_cmds));

		pinfo->lcd_init_step = 	LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		if (!g_display_on) {
			/* lcd display on */
			sharp_disp_on(k3fd);
		}
		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
		if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
			/* backlight on */
			sharp_pwm_on(k3fd);
		}

		g_display_on = true;
	} else {
		k3fb_loge("failed to init lcd!\n");
	}

	return 0;
}

static int mipi_sharp_panel_off(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	if (g_display_on) {
		g_display_on = false;
		if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
			/* backlight off */
			sharp_pwm_off(k3fd);
		}
		/* lcd display off */
		sharp_disp_off(k3fd);
	}

	return 0;
}

static int mipi_sharp_panel_remove(struct platform_device *pdev)
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

	/* lcd vcc  finit */
	vcc_cmds_tx(pdev, sharp_lcd_vcc_finit_cmds, \
		ARRAY_SIZE(sharp_lcd_vcc_finit_cmds));

	k3fb_logd("index=%d, exit!\n", k3fd->index);

	return 0;
}

static int mipi_sharp_panel_set_backlight(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	return pwm_set_backlight(k3fd, &g_display_on);
}

static int mipi_sharp_panel_set_fastboot(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	/* lcd vcc enable */
	vcc_cmds_tx(pdev, sharp_lcd_vcc_enable_cmds, \
		ARRAY_SIZE(sharp_lcd_vcc_enable_cmds));

	/* lcd iomux normal */
	iomux_cmds_tx(sharp_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(sharp_lcd_iomux_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(sharp_lcd_gpio_request_cmds, \
		ARRAY_SIZE(sharp_lcd_gpio_request_cmds));

	if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		/* pwm iomux normal */
		iomux_cmds_tx(sharp_pwm_iomux_normal_cmds, \
			ARRAY_SIZE(sharp_pwm_iomux_normal_cmds));

		/* pwm gpio request */
		gpio_cmds_tx(sharp_pwm_gpio_request_cmds, \
			ARRAY_SIZE(sharp_pwm_gpio_request_cmds));
	}

	g_display_on = true;

	return 0;
}

static struct k3_panel_info sharp_panel_info = {0};
static struct k3_fb_panel_data sharp_panel_data = {
	.panel_info = &sharp_panel_info,
	.on = mipi_sharp_panel_on,
	.off = mipi_sharp_panel_off,
	.remove = mipi_sharp_panel_remove,
	.set_backlight = mipi_sharp_panel_set_backlight,
	.set_fastboot = mipi_sharp_panel_set_fastboot,
};


/*******************************************************************************
**
*/
static int __devinit sharp_probe(struct platform_device *pdev)
{
	struct k3_panel_info *pinfo = NULL;

	g_display_on = false;

	pinfo = sharp_panel_data.panel_info;
	/* init lcd panel info */
	pinfo->xres = 640;
	pinfo->yres = 960;
	pinfo->width = 55;
	pinfo->height = 98;
	pinfo->type = PANEL_MIPI_VIDEO;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = EDC_OUT_RGB_888;
	pinfo->s3d_frm = EDC_FRM_FMT_2D;
	pinfo->bgr_fmt = EDC_RGB;
	pinfo->bl_set_type = BL_SET_BY_PWM;
	pinfo->bl_min = 1;
	pinfo->bl_max = 100;

	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->sbl_enable = 1;

	pinfo->sbl.bl_max = 0xff;
	pinfo->sbl.cal_a = 0x0f;
	pinfo->sbl.cal_b = 0xd8;
	pinfo->sbl.str_limit = 0x40;

	pinfo->ldi.h_back_porch = 4;
	pinfo->ldi.h_front_porch = 18;
	pinfo->ldi.h_pulse_width = 4;
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
	pinfo->clk_rate = 20000000;

	pinfo->mipi.lane_nums = DSI_2_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_3;
	pinfo->mipi.vc = 0;
	pinfo->mipi.dsi_bit_clk = 300;

	/* lcd vcc init */
	vcc_cmds_tx(pdev, sharp_lcd_vcc_init_cmds, \
		ARRAY_SIZE(sharp_lcd_vcc_init_cmds));

	/* lcd iomux init */
	iomux_cmds_tx(sharp_lcd_iomux_init_cmds, \
		ARRAY_SIZE(sharp_lcd_iomux_init_cmds));

	if (pinfo->bl_set_type & BL_SET_BY_PWM) {
		pinfo->pwm.name = CLK_PWM0_NAME,
		pinfo->pwm.clk_rate = DEFAULT_PWM_CLK_RATE,

		/* pwm clock */
		pwm_clk_get(pinfo);

		/* pwm iomux init */
		iomux_cmds_tx(sharp_pwm_iomux_init_cmds, \
			ARRAY_SIZE(sharp_pwm_iomux_init_cmds));
	}

	/* alloc panel device data */
	if (platform_device_add_data(pdev, &sharp_panel_data,
		sizeof(struct k3_fb_panel_data))) {
		k3fb_loge("failed to platform_device_add_data!\n");
		platform_device_put(pdev);
		return -ENOMEM;
	}

	k3_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe = sharp_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_sharp_LS035B3SX",
	},
};

static int __init mipi_sharp_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		k3fb_loge("not able to register the driver, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(mipi_sharp_panel_init);
