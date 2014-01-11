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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/pwm.h>
#include <linux/gpio.h>
#include <linux/leds.h>

#include <mach/gpio.h>
#include <mach/platform.h>
#include <mach/hisi_spi2.h>

#include "k3_fb.h"
#include "k3_fb_def.h"


#define TIME_PER_FRAME	16


/*******************************************************************************
** Power ON Sequence(sleep mode to Normal mode)
*/
static u8 powerOnData1[] = {
    0x74, 0x00, 0x07,
    0x76, 0x00, 0x00
};

/* Wait more than 10ms */
static u8 powerOnData2[] = {
    0x74, 0x00, 0x11,
    0x76, 0x22, 0x2F,
    0x74, 0x00, 0x12,
    0x76, 0x0F, 0x00,
    0x74, 0x00, 0x13,
    0x76, 0x7F, 0xD9,
    0x74, 0x00, 0x76,
    0x76, 0x22, 0x13,
    0x74, 0x00, 0x74,
    0x76, 0x00, 0x01,
    0x74, 0x00, 0x76,
    0x76, 0x00, 0x00,
    0x74, 0x00, 0x10,
    0x76, 0x56, 0x04
};

/* Wait more than 6frames */
static u8 powerOnData3[] = {
    0x74, 0x00, 0x12,
    0x76, 0x0C, 0x63
};

/* Wait more than 5frames */
static u8 powerOnData4[] = {
    0x74, 0x00, 0x01,
    0x76, 0x08, 0x3B, /* 0x08 or 0x0b */
    0x74, 0x00, 0x02,
    0x76, 0x03, 0x00,
    0x74, 0x00, 0x03,
    0x76, 0xC0, 0x40,
    0x74, 0x00, 0x08,
    0x76, 0x00, 0x0A,
    0x74, 0x00, 0x09,
    0x76, 0x00, 0x0F,
    0x74, 0x00, 0x76,
    0x76, 0x22, 0x13,
    0x74, 0x00, 0x0b,
    0x76, 0x33, 0x40,
    0x74, 0x00, 0x0c,
    0x76, 0x00, 0x20,
    0x74, 0x00, 0x1C,
    0x76, 0x77, 0x70,
    0x74, 0x00, 0x76,
    0x76, 0x00, 0x00,
    0x74, 0x00, 0x0D,
    0x76, 0x00, 0x00,
    0x74, 0x00, 0x0E,
    0x76, 0x05, 0x00,
    0x74, 0x00, 0x14,
    0x76, 0x00, 0x00,
    0x74, 0x00, 0x15,
    0x76, 0x08, 0x03,
    0x74, 0x00, 0x16,
    0x76, 0x00, 0x00,
    0x74, 0x00, 0x30,
    0x76, 0x00, 0x03,
    0x74, 0x00, 0x31,
    0x76, 0x07, 0x0F,
    0x74, 0x00, 0x32,
    0x76, 0x0D, 0x05,
    0x74, 0x00, 0x33,
    0x76, 0x04, 0x05,
    0x74, 0x00, 0x34,
    0x76, 0x09, 0x0D,
    0x74, 0x00, 0x35,
    0x76, 0x05, 0x01,
    0x74, 0x00, 0x36,
    0x76, 0x04, 0x00,
    0x74, 0x00, 0x37,
    0x76, 0x05, 0x04,
    0x74, 0x00, 0x38,
    0x76, 0x0C, 0x09,
    0x74, 0x00, 0x39,
    0x76, 0x01, 0x0C
};

/* wait NOTHING  */
static u8 powerOnData5[] = {
    0x74, 0x00, 0x07,
    0x76, 0x00, 0x01
};

/* Wait more than 2frame */
static u8 powerOnData6[] = {
    0x74, 0x00, 0x07,
    0x76, 0x01, 0x01
};

/* Wait more than 2frame */
static u8 powerOnData7[] = {
    0x74, 0x00, 0x07,
    0x76, 0x01, 0x03
};

/*******************************************************************************
** Power OFF Sequence(Normal to power off)
*/
static u8 powerOffData1[] = {
    0x74, 0x00, 0x0b,
    0x76, 0x30, 0x00,
    0x74, 0x00, 0x07,
    0x76, 0x01, 0x02
};

/* Wait more than 2frame */
static u8 powerOffData2[] = {
    0x74, 0x00, 0x07,
    0x76, 0x00, 0x00
};

/* Wait more than 2frame */
static u8 powerOffData3[] = {
    0x74, 0x00, 0x12,
    0x76, 0x00, 0x00,
    0x74, 0x00, 0x10,
    0x76, 0x06, 0x00
};

/* wait 1s */
static u8 powerOffData4[] = {
    0x74, 0x00, 0x10,
    0x76, 0x06, 0x01
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME		"lcdio-vcc"
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcdanalog;

static struct vcc_desc samsung_lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000},
};

static struct vcc_desc samsung_lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc samsung_lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc samsung_lcd_vcc_disable_cmds[] = {
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

static struct iomux_desc samsung_lcd_iomux_init_cmds[] = {
	{DTYPE_IOMUX_GET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, 0},
};

static struct iomux_desc samsung_lcd_iomux_normal_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, NORMAL},
};

static struct iomux_desc samsung_lcd_iomux_lowpower_cmds[] = {
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

static struct gpio_desc samsung_lcd_gpio_request_cmds[] = {
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

static struct gpio_desc samsung_lcd_gpio_free_cmds[] = {
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

static struct gpio_desc samsung_lcd_gpio_normal_cmds[] = {
	/* id0 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
	/* power */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 1},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
};

static struct gpio_desc samsung_lcd_gpio_lowpower_cmds[] = {
	/* id0 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
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

static struct iomux_desc samsung_pwm_iomux_init_cmds[] = {
	{DTYPE_IOMUX_GET, IOMUX_PWM_NAME,
		(struct iomux_block **)&pwm_block, (struct block_config **)&pwm_block_config, 0},
};

static struct iomux_desc samsung_pwm_iomux_normal_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_PWM_NAME,
		(struct iomux_block **)&pwm_block, (struct block_config **)&pwm_block_config, NORMAL},
};

static struct iomux_desc samsung_pwm_iomux_lowpower_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_PWM_NAME,
		(struct iomux_block **)&pwm_block, (struct block_config **)&pwm_block_config, LOWPOWER},
};

/*******************************************************************************
** LCD PWM GPIO
*/
#define GPIO_LCD_PWM0_NAME	"gpio_pwm0"
#define GPIO_LCD_PWM1_NAME	"gpio_pwm1"

#define GPIO_PWM0	GPIO_18_5
#define GPIO_PWM1	GPIO_18_6

static struct gpio_desc samsung_pwm_gpio_request_cmds[] = {
	/* pwm0 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM0_NAME, GPIO_PWM0, 0},
	/* pwm1 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};

static struct gpio_desc samsung_pwm_gpio_free_cmds[] = {
	/* pwm0 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM0_NAME, GPIO_PWM0, 0},
	/* pwm1 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};

static struct gpio_desc samsung_pwm_gpio_normal_cmds[] = {
	/* pwm1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};

static struct gpio_desc samsung_pwm_gpio_lowpower_cmds[] = {
	/* pwm0 */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_PWM0_NAME, GPIO_PWM0, 0},
	/* pwm1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_PWM1_NAME, GPIO_PWM1, 0},
};


static volatile bool g_display_on;
static struct k3_fb_panel_data samsung_panel_data;


/*******************************************************************************
**
*/
#ifdef CONFIG_MACH_TC45MSU3
extern int CspiDataExchange(PCSPI_XCH_PKT_T pXchPkt);
static bool spiSendData(u8 *data, u32 length, u8 bitcount)
{
	CSPI_XCH_PKT_T xferpack;
	CSPI_BUSCONFIG_T cspiPara;

	cspiPara.bitcount = bitcount - 1;
	cspiPara.scr = 9;
	cspiPara.cpsdvsr = 2;
	cspiPara.burst_num = 3;
	cspiPara.spo = 1;
	cspiPara.sph = 1;

	xferpack.pBusCnfg = &cspiPara;
	xferpack.pTxBuf = data;
	xferpack.pRxBuf = NULL;
	xferpack.xchCnt = length;
	xferpack.xchEvent = NULL;

	xferpack.spiDevice  = CS1;  /* use LCD cs as test */

	if (CspiDataExchange(&xferpack) == -1) {
		k3fb_loge("CspiDataExchange failed!\n");
		return false;
	}

	return true;
}
#else
static bool spiSendData(u8 *data, u32 length, u8 bitcount)
{
	return true;
}
#endif

static void set_spi_cs(void)
{
	outp32(IO_ADDRESS(0xFCA09000), 0xFFFF003C);
}


/*******************************************************************************
**
*/
static int samsung_pwm_on(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

	/* pwm iomux normal */
	iomux_cmds_tx(samsung_pwm_iomux_normal_cmds, \
		ARRAY_SIZE(samsung_pwm_iomux_normal_cmds));

	/* pwm gpio request */
	gpio_cmds_tx(samsung_pwm_gpio_request_cmds, \
		ARRAY_SIZE(samsung_pwm_gpio_request_cmds));
	/* pwm gpio normal */
	gpio_cmds_tx(samsung_pwm_gpio_normal_cmds, \
		ARRAY_SIZE(samsung_pwm_gpio_normal_cmds));

	/* backlight on */
	pwm_set_backlight(k3fd, &g_display_on);

	return 0;
}

static int samsung_pwm_off(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

	/* backlight off */
	pwm_set_backlight(k3fd, &g_display_on);

	/* pwm gpio lowpower */
	gpio_cmds_tx(samsung_pwm_gpio_lowpower_cmds, \
		ARRAY_SIZE(samsung_pwm_gpio_lowpower_cmds));
	/* pwm gpio free */
	gpio_cmds_tx(samsung_pwm_gpio_free_cmds, \
		ARRAY_SIZE(samsung_pwm_gpio_free_cmds));

	/* pwm iomux lowpower */
	iomux_cmds_tx(samsung_pwm_iomux_lowpower_cmds, \
		ARRAY_SIZE(samsung_pwm_iomux_lowpower_cmds));

	return 0;
}

static void samsung_disp_on(struct k3_fb_data_type *k3fd)
{
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	pinfo = &(k3fd->panel_info);

	/* lcd iomux normal */
	iomux_cmds_tx(samsung_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(samsung_lcd_iomux_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(samsung_lcd_gpio_request_cmds, \
		ARRAY_SIZE(samsung_lcd_gpio_request_cmds));
	/* lcd gpio normal */
	gpio_cmds_tx(samsung_lcd_gpio_normal_cmds, \
		ARRAY_SIZE(samsung_lcd_gpio_normal_cmds));

	/* lcd display on sequence */
	set_spi_cs();

	spiSendData(powerOnData1, sizeof(powerOnData1), 8);
	mdelay(10);
	spiSendData(powerOnData2, sizeof(powerOnData2), 8);
	mdelay(6 * TIME_PER_FRAME);
	spiSendData(powerOnData3, sizeof(powerOnData3), 8);
	mdelay(5 * TIME_PER_FRAME);
	spiSendData(powerOnData4, sizeof(powerOnData4), 8);
	mdelay(10);
	spiSendData(powerOnData5, sizeof(powerOnData5), 8);
	mdelay(2 * TIME_PER_FRAME);
	spiSendData(powerOnData6, sizeof(powerOnData6), 8);
	mdelay(2 * TIME_PER_FRAME);
	spiSendData(powerOnData7, sizeof(powerOnData7), 8);
}

static void samsung_disp_off(struct k3_fb_data_type *k3fd)
{
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	pinfo = &(k3fd->panel_info);

	/* lcd display off sequence */
	spiSendData(powerOffData1, sizeof(powerOffData1), 8);
	mdelay(2 * TIME_PER_FRAME);
	spiSendData(powerOffData2, sizeof(powerOffData2), 8);
	mdelay(2 * TIME_PER_FRAME);
	spiSendData(powerOffData3, sizeof(powerOffData3), 8);
	mdelay(10);
	spiSendData(powerOffData4, sizeof(powerOffData4), 8);
	mdelay(10);

	/* lcd gpio lowpower */
	gpio_cmds_tx(samsung_lcd_gpio_lowpower_cmds, \
		ARRAY_SIZE(samsung_lcd_gpio_lowpower_cmds));
	/* lcd gpio free */
	gpio_cmds_tx(samsung_lcd_gpio_free_cmds, \
		ARRAY_SIZE(samsung_lcd_gpio_free_cmds));

	/* lcd iomux lowpower */
	iomux_cmds_tx(samsung_lcd_iomux_lowpower_cmds, \
		ARRAY_SIZE(samsung_lcd_iomux_lowpower_cmds));

	/* lcd vcc disable */
	vcc_cmds_tx(NULL, samsung_lcd_vcc_disable_cmds, \
		ARRAY_SIZE(samsung_lcd_vcc_disable_cmds));
}

static int ldi_samsung_panel_on(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	pinfo = &(k3fd->panel_info);
	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		/* lcd vcc enable */
		vcc_cmds_tx(NULL, samsung_lcd_vcc_enable_cmds, \
			ARRAY_SIZE(samsung_lcd_vcc_enable_cmds));

		pinfo->lcd_init_step = LCD_INIT_LDI_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_LDI_SEND_SEQUENCE) {
		if (!g_display_on) {
			/* lcd display on */
			samsung_disp_on(k3fd);
			g_display_on = true;
			if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
				/* backlight on */
				samsung_pwm_on(k3fd);
			}
		}
	} else {
		k3fb_loge("failed to init lcd!\n");
	}

	return 0;
}

static int ldi_samsung_panel_off(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	if (g_display_on) {
		g_display_on = false;
		if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
			/* backlight off */
			samsung_pwm_off(k3fd);
		}
		/* lcd display off */
		samsung_disp_off(k3fd);
	}

	return 0;
}

static int ldi_samsung_panel_remove(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	/*BUG_ON(k3fd == NULL);*/
	if (!k3fd) {
		return 0;
	}

	if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		/* pwm clock */
		pwm_clk_put(&(k3fd->panel_info));
	}

	/* lcd vcc finit */
	vcc_cmds_tx(pdev, samsung_lcd_vcc_finit_cmds, \
		ARRAY_SIZE(samsung_lcd_vcc_finit_cmds));

	return 0;
}

static int ldi_samsung_panel_set_backlight(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	return pwm_set_backlight(k3fd, &g_display_on);
}

static int ldi_samsung_panel_set_fastboot(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	/* lcd vcc enable */
	vcc_cmds_tx(pdev, samsung_lcd_vcc_enable_cmds, \
		ARRAY_SIZE(samsung_lcd_vcc_enable_cmds));

	/* lcd iomux normal */
	iomux_cmds_tx(samsung_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(samsung_lcd_iomux_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(samsung_lcd_gpio_request_cmds, \
		ARRAY_SIZE(samsung_lcd_gpio_request_cmds));

	if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		/* pwm iomux normal */
		iomux_cmds_tx(samsung_pwm_iomux_normal_cmds, \
			ARRAY_SIZE(samsung_pwm_iomux_normal_cmds));

		/* pwm gpio request */
		gpio_cmds_tx(samsung_pwm_gpio_request_cmds, \
			ARRAY_SIZE(samsung_pwm_gpio_request_cmds));
	}

	g_display_on = true;

	return 0;
}

static struct k3_panel_info samsung_panel_info = {0};
static struct k3_fb_panel_data samsung_panel_data = {
	.panel_info = &samsung_panel_info,
	.on = ldi_samsung_panel_on,
	.off = ldi_samsung_panel_off,
	.remove = ldi_samsung_panel_remove,
	.set_backlight = ldi_samsung_panel_set_backlight,
	.set_fastboot = ldi_samsung_panel_set_fastboot,
};

static int __devinit samsung_probe(struct platform_device *pdev)
{
	struct k3_panel_info *pinfo = NULL;

	g_display_on = false;

	pinfo = samsung_panel_data.panel_info;
	/* init lcd info */
	pinfo->xres = 320;
	pinfo->yres = 480;
	pinfo->width = 55;
	pinfo->height = 98;
	pinfo->type = PANEL_LDI;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = EDC_OUT_RGB_888;
	pinfo->s3d_frm = EDC_FRM_FMT_2D;
	pinfo->bgr_fmt = EDC_RGB;
	pinfo->bl_set_type = BL_SET_BY_PWM;
	pinfo->bl_min = 1;
	pinfo->bl_max = 100;

	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->sbl_enable = 0;

	pinfo->sbl.bl_max = 0xff;
	pinfo->sbl.cal_a = 0x0f;
	pinfo->sbl.cal_b = 0xd8;
	pinfo->sbl.str_limit = 0x40;

	pinfo->ldi.h_back_porch = 11;
	pinfo->ldi.h_front_porch = 4;
	pinfo->ldi.h_pulse_width = 4;
	pinfo->ldi.v_back_porch = 10;  /* 8 */
	pinfo->ldi.v_front_porch = 4;
	pinfo->ldi.v_pulse_width = 2;
	pinfo->ldi.hsync_plr = 1;
	pinfo->ldi.vsync_plr = 1;
	pinfo->ldi.pixelclk_plr = 0;
	pinfo->ldi.data_en_plr = 1;
	pinfo->ldi.disp_mode = LDI_DISP_MODE_NOT_3D_FBF;

	/* Note: must init here */
	pinfo->frame_rate = 60;
	pinfo->clk_rate = 21000000;

	/* lcd vcc init */
	vcc_cmds_tx(pdev, samsung_lcd_vcc_init_cmds, \
		ARRAY_SIZE(samsung_lcd_vcc_init_cmds));

	/* lcd iomux init */
	iomux_cmds_tx(samsung_lcd_iomux_init_cmds, \
		ARRAY_SIZE(samsung_lcd_iomux_init_cmds));

	if (pinfo->bl_set_type & BL_SET_BY_PWM) {
		pinfo->pwm.name = CLK_PWM0_NAME;
		pinfo->pwm.clk_rate = DEFAULT_PWM_CLK_RATE;

		/* pwm clock */
		pwm_clk_get(pinfo);

		/* pwm iomux init */
		iomux_cmds_tx(samsung_pwm_iomux_init_cmds, \
			ARRAY_SIZE(samsung_pwm_iomux_init_cmds));
	}

	/* alloc panel device data */
	if (platform_device_add_data(pdev, &samsung_panel_data,
		sizeof(struct k3_fb_panel_data))) {
		k3fb_loge("failed to platform_device_add_data!\n");
		platform_device_put(pdev);
		return -ENOMEM;
	}

	k3_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe = samsung_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "ldi_samsung_LMS350DF04",
	},
};

static int __init ldi_samsung_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		k3fb_loge("not able to register the driver, error=%d.\n", ret);
		return ret;
	}

	return ret;
}

module_init(ldi_samsung_panel_init);
