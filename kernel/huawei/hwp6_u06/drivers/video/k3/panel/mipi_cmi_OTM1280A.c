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
static char regShiftData1[] = {
	0x00, 0x80,
};

static char regShiftData2[] = {
	0x00, 0x90,
};

static char regShiftData3[] = {
	0x00, 0xa0,
};

static char regShiftData4[] = {
	0x00, 0xb0,
};

static char regShiftData5[] = {
	0x00, 0xc0,
};

static char regShiftData6[] = {
	0x00, 0xd0,
};

static char regShiftData7[] = {
	0x00, 0xe0,
};

static char regShiftData8[] = {
	0x00, 0xf0,
};

static char regShiftData9[] = {
	0x00, 0xb3,
};

static char regShiftData10[] = {
	0x00, 0xa2,
};

static char regShiftData11[] = {
	0x00, 0xb4,
};

static char regShiftData12[] = {
	0x00, 0x00,
};

static char regShiftData13[] = {
	0x00, 0xb6,
};

static char regShiftData14[] = {
	0x00, 0xb8,
};

static char regShiftData15[] = {
	0x00, 0x94,
};

static char poweronData1[] = {
	0xff,
	0x12, 0x80, 0x01,
};

static char poweronData2[] = {
	0xff,
	0x12, 0x80,
};

static char poweronData3[] = {
	0xb3,
	0x38, 0x38,
};

static char poweronData4[] = {
	0xcb,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
};

static char poweronData5[] = {
	0xcb,
	0x00, 0xc0, 0xff, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
};

static char poweronData6[] = {
	0xcb,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
};

static char poweronData7[] = {
	0xcb,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

static char poweronData8[] = {
	0xcb,
	0x04, 0x00, 0x0f, 0x00, 0x00,
	0x00, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0xf4,
};

static char poweronData9[] = {
	0xcb,
	0xf4, 0xf4, 0x00, 0xf4, 0x08,
	0x04, 0x04, 0x04, 0x00, 0x00,
	0x00, 0x00, 0x00,
};

static char poweronData10[] = {
	0xcb,
	0x55, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x00, 0x00, 0x00,
};

static char poweronData11[] = {
	0xcb,
	0x00, 0x70, 0x01, 0x00, 0x00,
};

static char poweronData12[] = {
	0xcc,
	0x41, 0x42, 0x47, 0x48, 0x49,
	0x4a, 0x4b, 0x4c, 0x52, 0x55,
	0x43, 0x53, 0x65, 0x51, 0x4d,
	0x4e, 0x4f, 0x91, 0x8d, 0x8e,
	0x8f, 0x40, 0x40, 0x40, 0x40,
};

static char poweronData13[] = {
	0xcc,
	0x41, 0x42, 0x47, 0x48, 0x4c,
	0x4b, 0x4a, 0x49, 0x52, 0x55,
	0x43, 0x53, 0x65, 0x51, 0x4d,
	0x4e, 0x4f, 0x91, 0x8d, 0x8e,
	0x8f, 0x40, 0x40, 0x40, 0x40,
	0xff, 0xff, 0xff, 0x01,
};

static char poweronData14[] = {
	0xcc,
	0x41, 0x42, 0x47, 0x48, 0x49,
	0x4a, 0x4b, 0x4c, 0x52, 0x55,
	0x43, 0x53, 0x54, 0x51, 0x4d,
	0x4e, 0x4f, 0x91, 0x8d, 0x8e,
	0x8f, 0x40, 0x40, 0x40, 0x40,
};

static char poweronData15[] = {
	0xcc,
	0x41, 0x42, 0x47, 0x48, 0x4c,
	0x4b, 0x4a, 0x49, 0x52, 0x55,
	0x43, 0x53, 0x54, 0x51, 0x4d,
	0x4e, 0x4f, 0x91, 0x8d, 0x8e,
	0x8f, 0x40, 0x40, 0x40, 0x40,
	0xff, 0xff, 0xff, 0x01,
};

static char poweronData16[] = {
	0xc1,
	0x22, 0x00, 0x00, 0x00, 0x00,
};

static char poweronData17[] = {
	0xc0,
	0x00, 0x87, 0x00, 0x06, 0x0a,
	0x00, 0x87, 0x06, 0x0a, 0x00,
	0x00, 0x00,
};

static char poweronData18[] = {
	0xc0,
	0x00, 0x0a, 0x00, 0x14, 0x00,
	0x2a,
};

static char poweronData19[] = {
	0xc0,
	0x00, 0x03, 0x01, 0x01, 0x01,
	0x01, 0x1a, 0x03, 0x00, 0x02,
};

static char poweronData20[] = {
	0xc2,
	0x03, 0x02, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x22,
};

static char poweronData21[] = {
	0xc2,
	0x03, 0x00, 0xff, 0xff, 0x00,
	0x00, 0x00, 0x00, 0x22,
};

static char poweronData22[] = {
	0xc2,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00,
};

static char poweronData23[] = {
	0xc2,
	0xff, 0x00, 0xff, 0x00, 0x00,
	0x0a, 0x00, 0x0a,
};

static char poweronData24[] = {
	0xc2,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
};

static char poweronData25[] = {
	0xc2,
	0x84, 0x00, 0x10, 0x0d,
};

static char poweronData26[] = {
	0xc0,
	0x0f,
};

static char poweronData27[] = {
	0xc1,
	0xff,
};

static char poweronData28[] = {
	0xc0,
	0x54, 0x00,
};

static char poweronData29[] = {
	0xc5,
	0x20, 0x07, 0x00, 0xb0, 0xb0,
	0x00, 0x00, 0x00,
};

static char poweronData30[] = {
	0xc5,
	0x30, 0x85, 0x02, 0x88, 0x96,
	0x15, 0x00, 0x0c,
};

static char poweronData31[] = {
	0xd8,
	0x52, 0x00, 0x52, 0x00,
};

static char poweronData32[] = {
	0xd9,
	0x8f, 0x73, 0x80,
};

static char poweronData33[] = {
	0xc0,
	0x95,
};

static char poweronData34[] = {
	0xc0,
	0x05,
};

static char poweronData35[] = {
	0xf5,
	0x00, 0x00,
};

static char poweronData36[] = {
	0xb3,
	0x11,
};

static char poweronData37[] = {
	0xf5,
	0x00, 0x20,
};

static char poweronData38[] = {
	0xf5,
	0x0c, 0x12,
};

static char poweronData39[] = {
	0xf5,
	0x0a, 0x14, 0x06, 0x17,
};

static char poweronData40[] = {
	0xf5,
	0x0a, 0x14, 0x07, 0x14,
};

static char poweronData41[] = {
	0xf5,
	0x07, 0x16, 0x07, 0x14,
};

static char poweronData42[] = {
	0xf5,
	0x02, 0x12, 0x0a, 0x12, 0x07,
	0x12, 0x06, 0x12, 0x0b, 0x12,
	0x08, 0x12,
};

static char poweronData43[] = {
	0x35,
	0x00,
};

static char bl_level[] = {
	0x51,
	0x00,
};

static char bl_enable[] = {
	0x53,
	0x24,
};

static char soft_reset[] = {
	0x01,
};

/* exit sleep mode */
static char exit_sleep[] = {
	0x11,
};

/* set pixel off */
static char pixel_off[] = {
	0x22,
};

/* set pixel on */
static char pixel_on[] = {
	0x23,
};

/* set display off */
static char display_off[] = {
	0x28,
};

/* set display on */
static char display_on[] = {
	0x29,
};

static char set_address[] = {
	0x36,
	0xd0,
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
/*CABC in UI setting*/
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

/*CABC in video setting*/
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

static struct dsi_cmd_desc cmi_cabc_ui_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcUiData1), cabcUiData1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcUiData2), cabcUiData2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcUiData3), cabcUiData3}
};

static struct dsi_cmd_desc cmi_cabc_vid_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcVidData1), cabcVidData1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcVidData2), cabcVidData2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(cabcVidData3), cabcVidData3}
};

static struct dsi_cmd_desc cmi_gamma25_cmds[] = {
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

static struct dsi_cmd_desc cmi_gamma22_cmds[] = {
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

static struct dsi_cmd_desc cmi_display_on_cmds1[] = {
	{DTYPE_DCS_WRITE, 0, 5, WAIT_TYPE_MS,
		sizeof(soft_reset), soft_reset},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(bl_level), bl_level},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE, 0, 150, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 0, 100, WAIT_TYPE_US,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc cmi_display_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData1), poweronData1},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData1), regShiftData1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData2), poweronData2},

	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData3), regShiftData3},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData3), poweronData3},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData1), regShiftData1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData4), poweronData4},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData2), regShiftData2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData5), poweronData5},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData3), regShiftData3},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData6), poweronData6},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData4), regShiftData4},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData7), poweronData7},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData5), regShiftData5},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData8), poweronData8},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData6), regShiftData6},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData9), poweronData9},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData7), regShiftData7},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData10), poweronData10},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData8), regShiftData8},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData11), poweronData11},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData1), regShiftData1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData12), poweronData12},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData3), regShiftData3},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData13), poweronData13},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData5), regShiftData5},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData14), poweronData14},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData7), regShiftData7},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData15), poweronData15},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData2), regShiftData2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData16), poweronData16},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData1), regShiftData1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData17), poweronData17},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData2), regShiftData2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData18), poweronData18},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData3), regShiftData3},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData19), poweronData19},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData1), regShiftData1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData20), poweronData20},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData2), regShiftData2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData21), poweronData21},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData4), regShiftData4},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData22), poweronData22},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData3), regShiftData3},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData23), poweronData23},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData5), regShiftData5},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData24), poweronData24},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData7), regShiftData7},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData25), poweronData25},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData9), regShiftData9},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData26), poweronData26},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData10), regShiftData10},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData27), poweronData27},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData11), regShiftData11},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData28), poweronData28},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData1), regShiftData1},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData29), poweronData29},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData2), regShiftData2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData30), poweronData30},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData12), regShiftData12},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData31), poweronData31},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData12), regShiftData12},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData32), poweronData32},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData5), regShiftData5},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData33), poweronData33},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData6), regShiftData6},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData34), poweronData34},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData13), regShiftData13},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData35), poweronData35},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData4), regShiftData4},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData36), poweronData36},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData4), regShiftData4},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData37), poweronData37},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData14), regShiftData14},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData38), poweronData38},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData15), regShiftData15},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData39), poweronData39},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData10), regShiftData10},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData40), poweronData40},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData2), regShiftData2},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData41), poweronData41},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(regShiftData3), regShiftData3},
	{DTYPE_GEN_LWRITE, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData42), poweronData42},
	{DTYPE_GEN_WRITE2, 0, 30, WAIT_TYPE_US,
		sizeof(poweronData43), poweronData43},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(set_address), set_address},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(bl_level), bl_level},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 0, 30, WAIT_TYPE_US,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc cmi_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 30, WAIT_TYPE_US,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
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
#define GPIO_LCD_POWER_NAME	"gpio_lcd_power"
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"
#define GPIO_LCD_ID0_NAME	"gpio_lcd_id0"
#define GPIO_LCD_ID1_NAME	"gpio_lcd_id1"

#define GPIO_LCD_POWER	GPIO_21_3
#define GPIO_LCD_RESET	GPIO_0_3
#define GPIO_LCD_ID0	GPIO_16_7
#define GPIO_LCD_ID1	GPIO_17_0

static struct gpio_desc cmi_lcd_gpio_request_cmds[] = {
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

static struct gpio_desc cmi_lcd_gpio_free_cmds[] = {
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

static struct gpio_desc cmi_lcd_gpio_normal_cmds[] = {
	/* id0 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
	/* reset */
	/* {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1}, */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 50,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
	/* power */
	/* {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 120,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 1}, */
};

static struct gpio_desc cmi_lcd_gpio_lowpower_cmds[] = {
	/* id0 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID0_NAME, GPIO_LCD_ID0, 0},
	/* id1 */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID1_NAME, GPIO_LCD_ID1, 0},
	/* power */
	/* {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 0}, */
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
static int cabc_mode = 1;	 /* allow application to set cabc mode to ui mode */


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
	sprintf(buf, "CMI_OTM1280A 4.5'TFT %d x %d\n",
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

	if(val == 1) {
		/* allow application to set cabc mode to ui mode */
		cabc_mode = 1;
		cmi_set_cabc(p_tuning_dev, CABC_UI);
	} else if (val == 2) {
		/* force cabc mode to video mode */
		cabc_mode = 2;
		cmi_set_cabc(p_tuning_dev, CABC_VID);
	}

	return sprintf(buf, "%d\n", cabc_mode);
}

static DEVICE_ATTR(lcd_info, S_IRUGO, cmi_lcd_info_show, NULL);
static DEVICE_ATTR(cabc_mode, 0644, show_cabc_mode, store_cabc_mode);

static struct attribute *cmi_attrs[] = {
	&dev_attr_lcd_info,
	&dev_attr_cabc_mode,
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

	/* lcd gpio normal */
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
	int level = 0;

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;

	/*
	** Our eyes are more sensitive to small brightness.
	** So we adjust the brightness of lcd following iphone4 
	** Y=(X/255)^1.6*255
	*/
	level = (k3fd->bl_level * square_point_six(k3fd->bl_level) * 100) / 2779;
	if (level > 255)
		level = 255;

	outp32(edc_base + MIPIDSI_GEN_HDR_OFFSET,
		(k3fd->bl_level << 16) | (0x51 << 8) | 0x15);

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

static int mipi_cmi_panel_check_esd(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	outp32(k3fd->edc_base + MIPIDSI_GEN_HDR_OFFSET, 0x0A << 8 | 0x06);
	/* return  inp32(k3fd->edc_base + MIPIDSI_GEN_PLD_DATA_OFFSET); */

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
	.check_esd = mipi_cmi_panel_check_esd,
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
	pinfo->xres = 720;
	pinfo->yres = 1280;
	pinfo->width = 55;
	pinfo->height = 98;
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

	pinfo->ldi.h_back_porch = 43;
	pinfo->ldi.h_front_porch = 80;
	pinfo->ldi.h_pulse_width = 57;
	pinfo->ldi.v_back_porch = 4;
	pinfo->ldi.v_front_porch = 15;
	pinfo->ldi.v_pulse_width = 2;

	pinfo->ldi.hsync_plr = 1;
	pinfo->ldi.vsync_plr = 0;
	pinfo->ldi.pixelclk_plr = 1;
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

	/* for cabc */
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
		.name = "mipi_cmi_OTM1280A",
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
