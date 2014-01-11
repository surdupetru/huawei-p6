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
#include <linux/rmi.h>

/**********************open or close the glove function**********************/
extern struct rmi_function_container *rmi_fc;
extern int rmi_f01_glove_switch_read(struct rmi_function_container *fc);
/**********************open or close the glove function**********************/


/*******************************************************************************
** Power ON Sequence(sleep mode to Normal mode)
*/
static char soft_reset[] = {
	0x01,
};

static char bl_level[] = {
	0x51,
	0x00,
};

static char bl_enable[] = {
	0x53,
	0x24,
};

static char te_enable[] = {
	0x35,
	0x00,
};

static char exit_sleep[] = {
	0x11,
};

static char normal_display_on[] = {
	0x13,
};

static char all_pixels_off[] = {
	0x22,
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

static char orise_shift_0x00[] = {
	0x00,
	0x00,
};

static char orise_sheift_0x80[] = {
	0x00,
	0x80, 
};

/*enable orise mode*/
static char enable_orise_command1[] = {
	0xFF,
	0x12, 0x82, 0x01,
};

static char enable_orise_command2[] = {
	0xFF,
	0x12, 0x82,
};

/*Disable per-charge*/
static char disable_per_charge [] = {
	0xA5,
	0x0C, 0x04, 0x01, 
};

/* Set VGL*/
static char set_vgl1[] = {
	0x00,
	0xB0, 
};

static char set_vgl2[] = {
	0xC5,
	0x92, 0xD6, 0xAF, 0xAF, 0x82,
	0x88, 0x44, 0x44, 0x40, 0x88,
};

/* Delay TE*/
static char Delay_TE[] = {
	0x44,
	0x01, 0x40,
};

/*----------------CABC Base Sequence---------------------*/
static char enable_orise_mode5[] = {
	0x00,
	0x00,
};

static char enable_orise_mode6[] = {
	0x00,
	0x90,
};

static char CABC_enable_setting[] = {
	0x59,
	0x03,
};

static char CABC_func_setting[] = {
	0xca,
	0xda, 0xff, 0xa6, 0xff, 0x80,
	0xff, 0x05, 0x03, 0x05, 0x03,
	0x05, 0x03,
};

static char CABC_disable_curve[] = {
	0xc6,
	0x00,
};

static char CABC_disable_setting[] = {
	0x59,
	0x00,
};

static char CABC_UI_MODE[] = {
	0x55,
	0x91,
};

static char CABC_UI_curve[] = {
	0xc6,
	0x10,
};

static char CABC_UI_curve_setting[] = {
	0xc7,
	0x90, 0x89, 0x89, 0x88, 0x88,
	0x98, 0x88, 0x88, 0x88, 0x88,
	0x88, 0x88, 0x87, 0x88, 0x87,
	0x88, 0x87, 0x78,
};

static char CABC_STILL_MODE[] = {
	0x55,
	0x92,
};

static char CABC_STILL_curve[] = {
	0xc6,
	0x11,
};

static char CABC_STILL_curve_setting[] = {
	0xc7,
	0xa0, 0x9a, 0x99, 0x99, 0x89,
	0x88, 0x88, 0x88, 0x88, 0x88,
	0x88, 0x78, 0x87, 0x78, 0x77,
	0x77, 0x77, 0x77,
};

static char CABC_VID_MODE[] = {
	0x55,
	0x93,
};

static char CABC_VID_curve[] = {
	0xc6,
	0x12,
};

static char CABC_VID_curve_setting[] = {
	0xc7,
	0xb0, 0xab, 0xaa, 0x99, 0x99,
	0x99, 0x89, 0x88, 0x88, 0x88,
	0x77, 0x77, 0x77, 0x77, 0x77,
	0x77, 0x77, 0x56,
};


static char ce_medium_on[] = {
	0x55,
	0x90,
};

static char ce_init_param1[] = {
    0xD4,
	0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
	0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
	0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
	0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
	0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
	0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
	0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
	0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
	0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
	0x00,0x40,0x00,0x41,0x00,0x40,0x00,0x41,0x00,0x40,
	0x00,0x42,0x00,0x40,0x00,0x43,0x00,0x40,0x00,0x44,
	0x00,0x40,0x00,0x45,0x00,0x40,0x00,0x46,0x00,0x40,
	0x00,0x47,0x00,0x40,0x00,0x47,0x00,0x40,0x00,0x48,
	0x00,0x40,0x00,0x49,0x00,0x40,0x00,0x4a,0x00,0x40,
	0x00,0x4b,0x00,0x40,0x00,0x4c,0x00,0x40,0x00,0x4d,
	0x00,0x40,0x00,0x4e,0x00,0x40,0x00,0x4f,0x00,0x40,
	0x00,0x50,0x00,0x40,0x00,0x52,0x00,0x40,0x00,0x53,
	0x00,0x40,0x00,0x54,0x00,0x40,0x00,0x55,0x00,0x40,
};

static char ce_init_param2[] = {
    0xD4,
	0x00,0x57,0x00,0x40,0x00,0x57,0x00,0x40,0x00,0x58,
	0x00,0x40,0x00,0x58,0x00,0x40,0x00,0x59,0x00,0x40,
	0x00,0x5a,0x00,0x40,0x00,0x5a,0x00,0x40,0x00,0x5b,
	0x00,0x40,0x00,0x5c,0x00,0x40,0x00,0x5c,0x00,0x40,
	0x00,0x5d,0x00,0x40,0x00,0x5e,0x00,0x40,0x00,0x5e,
	0x00,0x40,0x00,0x5f,0x00,0x40,0x00,0x60,0x00,0x40,
	0x00,0x60,0x00,0x40,0x00,0x5f,0x00,0x40,0x00,0x5f,
	0x00,0x40,0x00,0x5e,0x00,0x40,0x00,0x5d,0x00,0x40,
	0x00,0x5d,0x00,0x40,0x00,0x5c,0x00,0x40,0x00,0x5b,
	0x00,0x40,0x00,0x5b,0x00,0x40,0x00,0x5a,0x00,0x40,
	0x00,0x59,0x00,0x40,0x00,0x59,0x00,0x40,0x00,0x58,
	0x00,0x40,0x00,0x58,0x00,0x40,0x00,0x57,0x00,0x40,
	0x00,0x56,0x00,0x40,0x00,0x55,0x00,0x40,0x00,0x54,
	0x00,0x40,0x00,0x54,0x00,0x40,0x00,0x53,0x00,0x40,
	0x00,0x52,0x00,0x40,0x00,0x51,0x00,0x40,0x00,0x50,
	0x00,0x40,0x00,0x4f,0x00,0x40,0x00,0x4f,0x00,0x40,
	0x00,0x4e,0x00,0x40,0x00,0x4d,0x00,0x40,0x00,0x4c,
	0x00,0x40,0x00,0x4b,0x00,0x40,0x00,0x4a,0x00,0x40,
};

static char ce_init_param3[] = {
    0xD4,
	0x00,0x4a,0x00,0x40,0x00,0x4b,0x00,0x40,0x00,0x4c,
	0x00,0x40,0x00,0x4c,0x00,0x40,0x00,0x4d,0x00,0x40,
	0x00,0x4e,0x00,0x40,0x00,0x4f,0x00,0x40,0x00,0x50,
	0x00,0x40,0x00,0x51,0x00,0x40,0x00,0x51,0x00,0x40,
	0x00,0x52,0x00,0x40,0x00,0x53,0x00,0x40,0x00,0x54,
	0x00,0x40,0x00,0x55,0x00,0x40,0x00,0x56,0x00,0x40,
	0x00,0x57,0x00,0x40,0x00,0x57,0x00,0x40,0x00,0x58,
	0x00,0x40,0x00,0x58,0x00,0x40,0x00,0x59,0x00,0x40,
	0x00,0x5a,0x00,0x40,0x00,0x5a,0x00,0x40,0x00,0x5b,
	0x00,0x40,0x00,0x5c,0x00,0x40,0x00,0x5c,0x00,0x40,
	0x00,0x5d,0x00,0x40,0x00,0x5e,0x00,0x40,0x00,0x5e,
	0x00,0x40,0x00,0x5f,0x00,0x40,0x00,0x60,0x00,0x40,
	0x00,0x60,0x00,0x40,0x00,0x5f,0x00,0x40,0x00,0x5f,
	0x00,0x40,0x00,0x5e,0x00,0x40,0x00,0x5d,0x00,0x40,
	0x00,0x5d,0x00,0x40,0x00,0x5c,0x00,0x40,0x00,0x5b,
	0x00,0x40,0x00,0x5b,0x00,0x40,0x00,0x5a,0x00,0x40,
	0x00,0x59,0x00,0x40,0x00,0x59,0x00,0x40,0x00,0x58,
	0x00,0x40,0x00,0x58,0x00,0x40,0x00,0x57,0x00,0x40,
};

static char ce_init_param4[] = {
    0xD4,
	0x00,0x56,0x00,0x40,0x00,0x55,0x00,0x40,0x00,0x54,
	0x00,0x40,0x00,0x54,0x00,0x40,0x00,0x53,0x00,0x40,
	0x00,0x52,0x00,0x40,0x00,0x51,0x00,0x40,0x00,0x50,
	0x00,0x40,0x00,0x4f,0x00,0x40,0x00,0x4f,0x00,0x40,
	0x00,0x4e,0x00,0x40,0x00,0x4d,0x00,0x40,0x00,0x4c,
	0x00,0x40,0x00,0x4b,0x00,0x40,0x00,0x4a,0x00,0x40,
	0x00,0x4a,0x00,0x40,0x00,0x4a,0x00,0x40,0x00,0x4b,
	0x00,0x40,0x00,0x4c,0x00,0x40,0x00,0x4c,0x00,0x40,
	0x00,0x4d,0x00,0x40,0x00,0x4e,0x00,0x40,0x00,0x4e,
	0x00,0x40,0x00,0x4f,0x00,0x40,0x00,0x50,0x00,0x40,
	0x00,0x50,0x00,0x40,0x00,0x51,0x00,0x40,0x00,0x51,
	0x00,0x40,0x00,0x52,0x00,0x40,0x00,0x53,0x00,0x40,
	0x00,0x53,0x00,0x40,0x00,0x52,0x00,0x40,0x00,0x50,
	0x00,0x40,0x00,0x4f,0x00,0x40,0x00,0x4e,0x00,0x40,
	0x00,0x4c,0x00,0x40,0x00,0x4b,0x00,0x40,0x00,0x4a,
	0x00,0x40,0x00,0x49,0x00,0x40,0x00,0x47,0x00,0x40,
	0x00,0x46,0x00,0x40,0x00,0x45,0x00,0x40,0x00,0x44,
	0x00,0x40,0x00,0x42,0x00,0x40,0x00,0x41,0x00,0x40,
};

static char ce_init_param5[] = {
    0xD5,
	0x00,0x55,0x00,0x4b,0x00,0x54,0x00,0x4a,0x00,0x52,
	0x00,0x4a,0x00,0x51,0x00,0x4a,0x00,0x4f,0x00,0x49,
	0x00,0x4e,0x00,0x49,0x00,0x4c,0x00,0x49,0x00,0x4b,
	0x00,0x49,0x00,0x4a,0x00,0x48,0x00,0x48,0x00,0x48,
	0x00,0x47,0x00,0x48,0x00,0x45,0x00,0x47,0x00,0x44,
	0x00,0x47,0x00,0x42,0x00,0x47,0x00,0x41,0x00,0x47,
	0x00,0x40,0x00,0x46,0x00,0x40,0x00,0x46,0x00,0x40,
	0x00,0x46,0x00,0x40,0x00,0x46,0x00,0x40,0x00,0x46,
	0x00,0x40,0x00,0x46,0x00,0x40,0x00,0x46,0x00,0x40,
	0x00,0x45,0x00,0x40,0x00,0x45,0x00,0x41,0x00,0x45,
	0x00,0x42,0x00,0x45,0x00,0x42,0x00,0x45,0x00,0x43,
	0x00,0x45,0x00,0x43,0x00,0x45,0x00,0x44,0x00,0x44,
	0x00,0x44,0x00,0x44,0x00,0x45,0x00,0x45,0x00,0x46,
	0x00,0x45,0x00,0x46,0x00,0x45,0x00,0x47,0x00,0x45,
	0x00,0x47,0x00,0x45,0x00,0x48,0x00,0x46,0x00,0x48,
	0x00,0x46,0x00,0x49,0x00,0x46,0x00,0x4a,0x00,0x46,
	0x00,0x4b,0x00,0x46,0x00,0x4c,0x00,0x47,0x00,0x4d,
	0x00,0x47,0x00,0x4d,0x00,0x47,0x00,0x4e,0x00,0x47,
};

static char ce_init_param6[] = {
    0xD5,
	0x00,0x4f,0x00,0x48,0x00,0x4f,0x00,0x48,0x00,0x50,
	0x00,0x48,0x00,0x50,0x00,0x48,0x00,0x51,0x00,0x48,
	0x00,0x51,0x00,0x49,0x00,0x52,0x00,0x49,0x00,0x52,
	0x00,0x49,0x00,0x52,0x00,0x49,0x00,0x53,0x00,0x49,
	0x00,0x53,0x00,0x4a,0x00,0x54,0x00,0x4a,0x00,0x54,
	0x00,0x4a,0x00,0x55,0x00,0x4a,0x00,0x55,0x00,0x4b,
	0x00,0x55,0x00,0x4b,0x00,0x55,0x00,0x4a,0x00,0x54,
	0x00,0x4a,0x00,0x54,0x00,0x4a,0x00,0x54,0x00,0x4a,
	0x00,0x53,0x00,0x4a,0x00,0x53,0x00,0x49,0x00,0x52,
	0x00,0x49,0x00,0x52,0x00,0x49,0x00,0x51,0x00,0x49,
	0x00,0x51,0x00,0x48,0x00,0x51,0x00,0x48,0x00,0x50,
	0x00,0x48,0x00,0x50,0x00,0x48,0x00,0x4f,0x00,0x48,
	0x00,0x4f,0x00,0x47,0x00,0x4e,0x00,0x47,0x00,0x4e,
	0x00,0x47,0x00,0x4d,0x00,0x47,0x00,0x4d,0x00,0x46,
	0x00,0x4c,0x00,0x46,0x00,0x4b,0x00,0x46,0x00,0x4b,
	0x00,0x45,0x00,0x4a,0x00,0x45,0x00,0x4a,0x00,0x45,
	0x00,0x49,0x00,0x45,0x00,0x49,0x00,0x44,0x00,0x48,
	0x00,0x44,0x00,0x47,0x00,0x44,0x00,0x47,0x00,0x43,
};

static char ce_init_param7[] = {
    0xD5,
	0x00,0x47,0x00,0x43,0x00,0x47,0x00,0x44,0x00,0x48,
	0x00,0x44,0x00,0x48,0x00,0x44,0x00,0x49,0x00,0x44,
	0x00,0x49,0x00,0x45,0x00,0x4a,0x00,0x45,0x00,0x4b,
	0x00,0x45,0x00,0x4b,0x00,0x46,0x00,0x4c,0x00,0x46,
	0x00,0x4c,0x00,0x46,0x00,0x4d,0x00,0x46,0x00,0x4d,
	0x00,0x47,0x00,0x4e,0x00,0x47,0x00,0x4f,0x00,0x47,
	0x00,0x4f,0x00,0x48,0x00,0x4f,0x00,0x48,0x00,0x50,
	0x00,0x48,0x00,0x50,0x00,0x48,0x00,0x51,0x00,0x48,
	0x00,0x51,0x00,0x49,0x00,0x52,0x00,0x49,0x00,0x52,
	0x00,0x49,0x00,0x52,0x00,0x49,0x00,0x53,0x00,0x49,
	0x00,0x53,0x00,0x4a,0x00,0x54,0x00,0x4a,0x00,0x54,
	0x00,0x4a,0x00,0x55,0x00,0x4a,0x00,0x55,0x00,0x4b,
	0x00,0x55,0x00,0x4b,0x00,0x55,0x00,0x4a,0x00,0x54,
	0x00,0x4a,0x00,0x54,0x00,0x4a,0x00,0x54,0x00,0x4a,
	0x00,0x53,0x00,0x4a,0x00,0x53,0x00,0x49,0x00,0x52,
	0x00,0x49,0x00,0x52,0x00,0x49,0x00,0x51,0x00,0x49,
	0x00,0x51,0x00,0x48,0x00,0x51,0x00,0x48,0x00,0x50,
	0x00,0x48,0x00,0x50,0x00,0x48,0x00,0x4f,0x00,0x48,
};

static char ce_init_param8[] = {
    0xD5,
	0x00,0x4f,0x00,0x47,0x00,0x4e,0x00,0x47,0x00,0x4e,
	0x00,0x47,0x00,0x4d,0x00,0x47,0x00,0x4d,0x00,0x46,
	0x00,0x4c,0x00,0x46,0x00,0x4b,0x00,0x46,0x00,0x4b,
	0x00,0x45,0x00,0x4a,0x00,0x45,0x00,0x4a,0x00,0x45,
	0x00,0x49,0x00,0x45,0x00,0x49,0x00,0x44,0x00,0x48,
	0x00,0x44,0x00,0x47,0x00,0x44,0x00,0x47,0x00,0x43,
	0x00,0x47,0x00,0x43,0x00,0x47,0x00,0x43,0x00,0x47,
	0x00,0x44,0x00,0x48,0x00,0x44,0x00,0x48,0x00,0x44,
	0x00,0x49,0x00,0x44,0x00,0x49,0x00,0x45,0x00,0x49,
	0x00,0x45,0x00,0x4a,0x00,0x45,0x00,0x4a,0x00,0x45,
	0x00,0x4b,0x00,0x45,0x00,0x4b,0x00,0x46,0x00,0x4c,
	0x00,0x46,0x00,0x4c,0x00,0x46,0x00,0x4c,0x00,0x46,
	0x00,0x4d,0x00,0x46,0x00,0x4e,0x00,0x47,0x00,0x4e,
	0x00,0x47,0x00,0x4f,0x00,0x47,0x00,0x4f,0x00,0x48,
	0x00,0x50,0x00,0x48,0x00,0x50,0x00,0x48,0x00,0x51,
	0x00,0x48,0x00,0x51,0x00,0x49,0x00,0x52,0x00,0x49,
	0x00,0x53,0x00,0x49,0x00,0x53,0x00,0x4a,0x00,0x54,
	0x00,0x4a,0x00,0x54,0x00,0x4a,0x00,0x55,0x00,0x4a,
};


static struct dsi_cmd_desc jdi_display_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(bl_level), bl_level},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(te_enable), te_enable},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(enable_orise_command1), enable_orise_command1},
	{DTYPE_GEN_WRITE2, 0, 200, WAIT_TYPE_US,
		sizeof(orise_sheift_0x80), orise_sheift_0x80},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(enable_orise_command2), enable_orise_command2},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(orise_shift_0x00), orise_shift_0x00},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(ce_init_param1), ce_init_param1},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(ce_init_param2), ce_init_param2},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(ce_init_param3), ce_init_param3},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(ce_init_param4), ce_init_param4},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(orise_shift_0x00), orise_shift_0x00},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(ce_init_param5), ce_init_param5},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(ce_init_param6), ce_init_param6},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(ce_init_param7), ce_init_param7},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(ce_init_param8), ce_init_param8},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(orise_shift_0x00), orise_shift_0x00},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(ce_medium_on), ce_medium_on},
	{DTYPE_GEN_WRITE2, 0, 200, WAIT_TYPE_US,
		sizeof(orise_sheift_0x80), orise_sheift_0x80},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(disable_per_charge), disable_per_charge},	
	{DTYPE_GEN_WRITE2, 0, 200, WAIT_TYPE_US,
		sizeof(set_vgl1), set_vgl1},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(set_vgl2), set_vgl2},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(Delay_TE), Delay_TE},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 0, 10, WAIT_TYPE_MS,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc jdi_display_off_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(bl_level), bl_level},
	{DTYPE_DCS_WRITE, 0, 30, WAIT_TYPE_US,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(enter_sleep), enter_sleep},
};

static struct dsi_cmd_desc jdi_cabc_cmds[] = {
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_enable_setting), CABC_enable_setting},	
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode6), enable_orise_mode6},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(CABC_func_setting), CABC_func_setting},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_UI_curve), CABC_UI_curve},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},		
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(CABC_UI_curve_setting), CABC_UI_curve_setting},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_STILL_curve), CABC_STILL_curve},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},		
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(CABC_STILL_curve_setting), CABC_STILL_curve_setting},	
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_VID_curve), CABC_VID_curve},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},		
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(CABC_VID_curve_setting), CABC_VID_curve_setting},	
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_disable_curve), CABC_disable_curve},	
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_disable_setting), CABC_disable_setting},		
};

static struct dsi_cmd_desc jdi_cabc_ui_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_UI_MODE), CABC_UI_MODE},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_UI_curve), CABC_UI_curve},
};

static struct dsi_cmd_desc jdi_cabc_still_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_STILL_MODE), CABC_STILL_MODE},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_STILL_curve), CABC_STILL_curve},			
};

static struct dsi_cmd_desc jdi_cabc_vid_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_VID_MODE), CABC_VID_MODE},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(CABC_VID_curve), CABC_VID_curve},		
};

/*
static struct dsi_cmd_desc jdi_ce_cmds[] = {
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode7), enable_orise_mode7},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(CE_param1), CE_param1},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode8), enable_orise_mode8},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(CE_param2), CE_param2},				
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode9), enable_orise_mode9},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(CE_param3), CE_param3},		
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode10), enable_orise_mode10},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(CE_param4), CE_param4},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode11), enable_orise_mode11},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(CE_param5), CE_param5},				
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode12), enable_orise_mode12},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(CE_param6), CE_param6},
	{DTYPE_GEN_WRITE2, 0, 100, WAIT_TYPE_US,
		sizeof(enable_orise_mode5), enable_orise_mode5},
};

static struct dsi_cmd_desc jdi_ce_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(CE_medium_on), CE_medium_on},	
};
*/
/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME		"lcdio-vcc"
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcdanalog;

static struct vcc_desc jdi_lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000},
};

static struct vcc_desc jdi_lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc jdi_lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0},
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0},
};

static struct vcc_desc jdi_lcd_vcc_disable_cmds[] = {
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

static struct iomux_desc jdi_lcd_iomux_init_cmds[] = {
	{DTYPE_IOMUX_GET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, 0},
};

static struct iomux_desc jdi_lcd_iomux_normal_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, NORMAL},
};

static struct iomux_desc jdi_lcd_iomux_lowpower_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_LCD_NAME,
		(struct iomux_block **)&lcd_block, (struct block_config **)&lcd_block_config, LOWPOWER},
};

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_LCD_POWER_NAME	"gpio_lcd_power"
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"

#define GPIO_LCD_POWER	GPIO_21_3
#define GPIO_LCD_RESET	GPIO_0_3

static struct gpio_desc jdi_lcd_gpio_request_cmds[] = {
	/* power */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
};

static struct gpio_desc jdi_lcd_gpio_free_cmds[] = {
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
	/* power */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 0},
};

static struct gpio_desc jdi_lcd_gpio_normal_cmds[] = {
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
	/* power */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 1},
    /* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 1},
};

static struct gpio_desc jdi_lcd_gpio_lowpower_cmds[] = {
	/* power */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_POWER_NAME, GPIO_LCD_POWER, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_RESET_NAME, GPIO_LCD_RESET, 0},
};

/*******************************************************************************
** TP VCC
*/
#define VCC_TPVCI_NAME		"ts-vdd"
#define VCC_TPVDDIO_NAME	"ts-vbus"

static struct regulator *vcc_tpvci;
static struct regulator *vcc_tpvddio;

static struct vcc_desc jdi_tp_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_TPVCI_NAME, &vcc_tpvci, 0, 0},
	{DTYPE_VCC_GET, VCC_TPVDDIO_NAME, &vcc_tpvddio, 0, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_TPVCI_NAME, &vcc_tpvddio, 1800000, 1800000},
};

static struct vcc_desc jdi_tp_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_TPVCI_NAME, &vcc_tpvci, 0, 0},
	{DTYPE_VCC_PUT, VCC_TPVDDIO_NAME, &vcc_tpvddio, 0, 0},
};

static struct vcc_desc jdi_tp_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_TPVCI_NAME, &vcc_tpvci, 0, 0},
	{DTYPE_VCC_ENABLE, VCC_TPVDDIO_NAME, &vcc_tpvddio, 0, 0},
};

static struct vcc_desc jdi_tp_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, VCC_TPVDDIO_NAME, &vcc_tpvddio, 0, 0},
	{DTYPE_VCC_DISABLE, VCC_TPVCI_NAME, &vcc_tpvci, 0, 0},
};

/*******************************************************************************
** TP IOMUX
*/
#define IOMUX_TP_NAME	"block_touchscreen"

static struct iomux_block **tp_block;
static struct block_config **tp_block_config;

static struct iomux_desc jdi_tp_iomux_init_cmds[] = {
	{DTYPE_IOMUX_GET, IOMUX_TP_NAME,
		(struct iomux_block **)&tp_block, (struct block_config **)&tp_block_config, 0},
};

static struct iomux_desc jdi_tp_iomux_normal_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_TP_NAME,
		(struct iomux_block **)&tp_block, (struct block_config **)&tp_block_config, NORMAL},
};

static struct iomux_desc jdi_tp_iomux_lowpower_cmds[] = {
	{DTYPE_IOMUX_SET, IOMUX_TP_NAME,
		(struct iomux_block **)&tp_block, (struct block_config **)&tp_block_config, LOWPOWER},
};

/*******************************************************************************
** TP GPIO
*/
#define GPIO_TP_RESET_NAME	"gpio_tp_reset"

#define GPIO_TP_RESET	GPIO_19_4

static struct gpio_desc jdi_tp_gpio_request_cmds[] = {
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_TP_RESET_NAME, GPIO_TP_RESET, 0},
};

static struct gpio_desc jdi_tp_gpio_free_cmds[] = {
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_TP_RESET_NAME, GPIO_TP_RESET, 0},
};

static struct gpio_desc jdi_tp_gpio_normal_cmds[] = {
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_TP_RESET_NAME, GPIO_TP_RESET, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_TP_RESET_NAME, GPIO_TP_RESET, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_TP_RESET_NAME, GPIO_TP_RESET, 1},
};

static struct gpio_desc jdi_tp_gpio_lowpower_cmds[] = {
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_TP_RESET_NAME, GPIO_TP_RESET, 0},
};

static volatile bool g_display_on;
static struct k3_fb_panel_data jdi_panel_data;
static volatile bool backlight_log_once;


/*******************************************************************************
**
*/
static struct lcd_tuning_dev *p_tuning_dev = NULL;
static int cabc_mode = 1; /* allow application to set cabc mode to ui mode */

/* y=pow(x,0.6),x=[0,255]
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
*/
static int jdi_set_gamma(struct lcd_tuning_dev *ltd, enum lcd_gamma gamma)
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

	return ret;
}

static int jdi_set_cabc(struct lcd_tuning_dev *ltd, enum  tft_cabc cabc)
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

	switch (cabc) 
	{
		case CABC_UI:
			
			mipi_dsi_cmds_tx(jdi_cabc_ui_on_cmds, \
				ARRAY_SIZE(jdi_cabc_ui_on_cmds), edc_base);
			break;
		case CABC_VID:
		
			mipi_dsi_cmds_tx(jdi_cabc_vid_on_cmds, \
				ARRAY_SIZE(jdi_cabc_vid_on_cmds), edc_base);
			break;
		case CABC_OFF:
			break;
		default:
			ret = -1;
	}

	return ret;
}

static unsigned int g_csc_value[9];
static unsigned int g_is_csc_set;
static struct semaphore ct_sem;
unsigned int lcd_product_id = 0xff;

static void jdi_store_ct_cscValue(unsigned int csc_value[])
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

static int jdi_set_ct_cscValue(struct k3_fb_data_type *k3fd)
{
    u32 edc_base = 0;
    edc_base = k3fd->edc_base;
    down(&ct_sem);
    if (1 == g_is_csc_set) {
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

static int jdi_set_color_temperature(struct lcd_tuning_dev *ltd, unsigned int csc_value[])
{
    int flag = 0;
    struct platform_device *pdev;
    struct k3_fb_data_type *k3fd;

    if (ltd == NULL) {
        return -1;
    }
    pdev = (struct platform_device *)(ltd->data);
    k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);

    if (k3fd == NULL) {
        return -1;
    }

    jdi_store_ct_cscValue(csc_value);
    flag = jdi_set_ct_cscValue(k3fd);
    return flag;
}

static struct lcd_tuning_ops sp_tuning_ops = {
	.set_gamma = jdi_set_gamma,
	.set_cabc = jdi_set_cabc,
	.set_color_temperature = jdi_set_color_temperature,
};

static ssize_t jdi_lcd_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct k3_panel_info *pinfo = NULL;

	pinfo = jdi_panel_data.panel_info;
	sprintf(buf, "jdi_OTM1282B 4.7' CMD TFT %d x %d\n",
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
		jdi_set_cabc(p_tuning_dev, CABC_UI);
	} else if (val == 2) {
		/* force cabc mode to video mode */
		cabc_mode = 2;
		jdi_set_cabc(p_tuning_dev, CABC_VID);
	}

	return sprintf(buf, "%d\n", cabc_mode);
}

static DEVICE_ATTR(lcd_info, S_IRUGO, jdi_lcd_info_show, NULL);
static DEVICE_ATTR(cabc_mode, 0644, show_cabc_mode, store_cabc_mode);

static struct attribute *jdi_attrs[] = {
	&dev_attr_lcd_info,
	&dev_attr_cabc_mode,
	NULL,
};

static struct attribute_group jdi_attr_group = {
	.attrs = jdi_attrs,
};

static int jdi_sysfs_init(struct platform_device *pdev)
{
	int ret = 0;

	ret = sysfs_create_group(&pdev->dev.kobj, &jdi_attr_group);
	if (ret) {
		k3fb_loge("create sysfs file failed!\n");
		return ret;
	}

	return 0;
}

static void jdi_sysfs_deinit(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &jdi_attr_group);
}


/*******************************************************************************
**
*/
static void jdi_disp_on(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	struct k3_panel_info *pinfo = NULL;
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	pinfo = &(k3fd->panel_info);
	jdi_set_ct_cscValue(k3fd);

	/* tp iomux normal */
	iomux_cmds_tx(jdi_tp_iomux_normal_cmds, \
		ARRAY_SIZE(jdi_tp_iomux_normal_cmds));

	/* tp gpio request */
	gpio_cmds_tx(jdi_tp_gpio_request_cmds, \
		ARRAY_SIZE(jdi_tp_gpio_request_cmds));

	/* tp gpio normal */
	gpio_cmds_tx(jdi_tp_gpio_normal_cmds, \
		ARRAY_SIZE(jdi_tp_gpio_normal_cmds));

	/* lcd iomux normal */
	iomux_cmds_tx(jdi_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(jdi_lcd_iomux_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(jdi_lcd_gpio_request_cmds, \
		ARRAY_SIZE(jdi_lcd_gpio_request_cmds));

	/* lcd gpio normal */
	gpio_cmds_tx(jdi_lcd_gpio_normal_cmds, \
		ARRAY_SIZE(jdi_lcd_gpio_normal_cmds));

	/* lcd display on sequence */
	mipi_dsi_cmds_tx(jdi_display_on_cmds, \
		ARRAY_SIZE(jdi_display_on_cmds), edc_base);

	 /*enable TP's irq which was disabled in jdi_disp_off.*/
	enable_irq(gpio_to_irq(GPIO_19_5));

	/* Enable CABC */
	mipi_dsi_cmds_tx(jdi_cabc_cmds, \
		ARRAY_SIZE(jdi_cabc_cmds), edc_base);
	mipi_dsi_cmds_tx(jdi_cabc_ui_on_cmds, \
		ARRAY_SIZE(jdi_cabc_ui_on_cmds), edc_base);
/*
	mipi_dsi_cmds_tx(jdi_ce_cmds, \
		ARRAY_SIZE(jdi_ce_cmds), edc_base);

	mipi_dsi_cmds_tx(jdi_ce_on_cmds, \
		ARRAY_SIZE(jdi_ce_on_cmds), edc_base);

	mipi_dsi_cmds_tx(jdi_vdd_cmds, \
		ARRAY_SIZE(jdi_vdd_cmds), edc_base);
*/
    printk("---display on\n");
}

static void jdi_disp_off(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	int retval = 0;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	pinfo = &(k3fd->panel_info);

	/* lcd display off sequence */
	mipi_dsi_cmds_tx(jdi_display_off_cmds,
		ARRAY_SIZE(jdi_display_off_cmds), edc_base);

	if(rmi_fc != NULL){

		retval = rmi_f01_glove_switch_read(rmi_fc);
		if (retval < 0)
			dev_err(&rmi_fc->dev,
				"Failed to switch mode between finger and glove. Code: %d.\n",
				retval);
	}
	/*
	GPIO_19_5 is TP's interrupt GPIO, It is upload by ldo14 which will be closed here,
	so this irq should be disabled at first and then enable it in mipi_jdi_panel_on.
	*/

	disable_irq(gpio_to_irq(GPIO_19_5));

	/* lcd gpio request */
	gpio_cmds_tx(jdi_lcd_gpio_lowpower_cmds, \
		ARRAY_SIZE(jdi_lcd_gpio_lowpower_cmds));

	/* lcd gpio free */
	gpio_cmds_tx(jdi_lcd_gpio_free_cmds, \
		ARRAY_SIZE(jdi_lcd_gpio_free_cmds));

	/* lcd iomux lowpower */
	iomux_cmds_tx(jdi_lcd_iomux_lowpower_cmds, \
		ARRAY_SIZE(jdi_lcd_iomux_lowpower_cmds));

	/* tp gpio request */
	printk("%s	enter:tp gpio request, free and iomux lowpower\n",__func__);
	gpio_cmds_tx(jdi_tp_gpio_lowpower_cmds, \
		ARRAY_SIZE(jdi_tp_gpio_lowpower_cmds));

	/* tp gpio free */
	gpio_cmds_tx(jdi_tp_gpio_free_cmds, \
		ARRAY_SIZE(jdi_tp_gpio_free_cmds));

	/* tp iomux lowpower */
	iomux_cmds_tx(jdi_tp_iomux_lowpower_cmds, \
		ARRAY_SIZE(jdi_tp_iomux_lowpower_cmds));

	/* lcd vcc disable */
	vcc_cmds_tx(NULL, jdi_lcd_vcc_disable_cmds, \
		ARRAY_SIZE(jdi_lcd_vcc_disable_cmds));

	/* tp vcc disable */
	vcc_cmds_tx(NULL, jdi_tp_vcc_disable_cmds, \
		ARRAY_SIZE(jdi_tp_vcc_disable_cmds));

}

static int skip_esd_once = true;
static int mipi_jdi_panel_on(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(pdev == NULL);
	
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	pinfo = &(k3fd->panel_info);
	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		/* tp vcc enable */
		vcc_cmds_tx(NULL, jdi_tp_vcc_enable_cmds, \
			ARRAY_SIZE(jdi_tp_vcc_enable_cmds));

		/* lcd vcc enable */
		vcc_cmds_tx(NULL, jdi_lcd_vcc_enable_cmds, \
			ARRAY_SIZE(jdi_lcd_vcc_enable_cmds));

		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		if (!g_display_on) {
			/* lcd display on */
			jdi_disp_on(k3fd);
			skip_esd_once = true;
		}
		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {

        g_display_on = true;
        backlight_log_once = true;
	} else {
		k3fb_loge("failed to init lcd!\n");
	}

	return 0;
}

static int mipi_jdi_panel_off(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	if (g_display_on) {
		g_display_on = false;
		/* lcd display off */
		jdi_disp_off(k3fd);
		skip_esd_once = false;
	}

	return 0;
}

static int mipi_jdi_panel_remove(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	/*BUG_ON(k3fd == NULL);*/
	if (!k3fd) {
		return 0;
	}

	/* lcd vcc finit */
	vcc_cmds_tx(pdev, jdi_lcd_vcc_finit_cmds, \
		ARRAY_SIZE(jdi_lcd_vcc_finit_cmds));

	/* tp vcc finit */
	vcc_cmds_tx(pdev, jdi_tp_vcc_finit_cmds, \
		ARRAY_SIZE(jdi_tp_vcc_finit_cmds));

	jdi_sysfs_deinit(pdev);

	return 0;
}

static int mipi_jdi_panel_set_backlight(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;
	u32 level = 0;

	char bl_level_adjust[2] = {
		0x51,
		0x00,
	};

	struct dsi_cmd_desc  jdi_bl_level_adjust[] = {
		{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust},		
	};

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;

	/*Our eyes are more sensitive to small brightness.
	So we adjust the brightness of lcd following iphone4 */
    level = k3fd->bl_level;

	if (level > 255)
		level = 255;

    //backlight may turn off when bl_level is below 6.
    if (level < 6 && level != 0)
    {
        level = 6;
    }

#ifdef CONFIG_MATE_USE_TI_BACKLIGHT_IC
    if (level >= 28 && level <= 34)
    {
        level = 35;
    }
#endif

	bl_level_adjust[1] = level;

    if (backlight_log_once) {
        backlight_log_once = false;
        printk("%s: set backlight to %d\n", __func__, level);
    }
	mipi_dsi_cmds_tx(jdi_bl_level_adjust, \
		ARRAY_SIZE(jdi_bl_level_adjust), edc_base);

	return 0;
}

static int mipi_jdi_panel_set_fastboot(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	/* tp vcc enable */
	vcc_cmds_tx(pdev, jdi_tp_vcc_enable_cmds, \
		ARRAY_SIZE(jdi_tp_vcc_enable_cmds));

	/* tp iomux normal */
	iomux_cmds_tx(jdi_tp_iomux_normal_cmds, \
		ARRAY_SIZE(jdi_lcd_iomux_normal_cmds));

	/* tp gpio request */
	gpio_cmds_tx(jdi_tp_gpio_request_cmds, \
		ARRAY_SIZE(jdi_tp_gpio_request_cmds));

	/* lcd vcc enable */
	vcc_cmds_tx(pdev, jdi_lcd_vcc_enable_cmds, \
		ARRAY_SIZE(jdi_lcd_vcc_enable_cmds));

	/* lcd iomux normal */
	iomux_cmds_tx(jdi_lcd_iomux_normal_cmds, \
		ARRAY_SIZE(jdi_lcd_iomux_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(jdi_lcd_gpio_request_cmds, \
		ARRAY_SIZE(jdi_lcd_gpio_request_cmds));

	g_display_on = true;

	return 0;
}

static int mipi_jdi_panel_set_cabc(struct platform_device *pdev, int value)
{
	u32 edc_base = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;

#if 0
	if (value) {
		outp32(edc_base + MIPIDSI_GEN_HDR_OFFSET, 0x0dbb23);
	} else {
		outp32(edc_base + MIPIDSI_GEN_HDR_OFFSET, 0x0cbb23);
	}
#endif

	return 0;
}

static int mipi_jdi_panel_check_esd(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

    if (skip_esd_once) {
        skip_esd_once = false;
        outp32(k3fd->edc_base + MIPIDSI_GEN_HDR_OFFSET, 0xAC << 8 | 0x06);
        inp32(k3fd->edc_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
        return 0;
    }

	/* read pwm */
	outp32(k3fd->edc_base + MIPIDSI_GEN_HDR_OFFSET, 0xAC << 8 | 0x06);

	return inp32(k3fd->edc_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
}

static struct k3_panel_info jdi_panel_info = {0};
static struct k3_fb_panel_data jdi_panel_data = {
	.panel_info = &jdi_panel_info,
	.on = mipi_jdi_panel_on,
	.off = mipi_jdi_panel_off,
	.remove = mipi_jdi_panel_remove,
	.set_backlight = mipi_jdi_panel_set_backlight,
	.set_fastboot = mipi_jdi_panel_set_fastboot,
	.set_cabc = mipi_jdi_panel_set_cabc,
	.check_esd = mipi_jdi_panel_check_esd,
};

static int __devinit jdi_probe(struct platform_device *pdev)
{
	struct k3_panel_info *pinfo = NULL;
	struct platform_device *reg_pdev = NULL;
	struct lcd_properities lcd_props;
    struct k3_fb_data_type *k3fd = NULL;

	g_display_on = false;

	pinfo = jdi_panel_data.panel_info;
	/* init lcd panel info */
	pinfo->xres = 720;
	pinfo->yres = 1280;
	pinfo->width = 58;
	pinfo->height = 103;
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
	pinfo->sbl_enable = 1;
	pinfo->sbl.bl_max = 0xff;
	pinfo->sbl.cal_a = 0x08;
	pinfo->sbl.cal_b = 0xd8;
	pinfo->sbl.str_limit = 0x40;

	pinfo->ldi.h_back_porch = 43;
	pinfo->ldi.h_front_porch = 80;
	pinfo->ldi.h_pulse_width = 57;
	pinfo->ldi.v_back_porch = 12;
	pinfo->ldi.v_front_porch = 14;
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

	/* tp vcc init */
	vcc_cmds_tx(pdev, jdi_tp_vcc_init_cmds, \
		ARRAY_SIZE(jdi_tp_vcc_init_cmds));

	/* lcd vcc init */
	vcc_cmds_tx(pdev, jdi_lcd_vcc_init_cmds, \
		ARRAY_SIZE(jdi_lcd_vcc_init_cmds));

	/* tp iomux init */
	iomux_cmds_tx(jdi_tp_iomux_init_cmds, \
		ARRAY_SIZE(jdi_tp_iomux_init_cmds));

	/* lcd iomux init */
	iomux_cmds_tx(jdi_lcd_iomux_init_cmds, \
		ARRAY_SIZE(jdi_lcd_iomux_init_cmds));

	/* alloc panel device data */
	if (platform_device_add_data(pdev, &jdi_panel_data,
		sizeof(struct k3_fb_panel_data))) {
		k3fb_loge("platform_device_add_data failed!\n");
		platform_device_put(pdev);
		return -ENOMEM;
	}

	reg_pdev = k3_fb_add_device(pdev);
    k3fd = (struct k3_fb_data_type *)platform_get_drvdata(reg_pdev);
    BUG_ON(k3fd == NULL);

    /* read product id */
    outp32(k3fd->edc_base + MIPIDSI_GEN_HDR_OFFSET, 0xDA << 8 | 0x06);
    udelay(150);
    lcd_product_id = inp32(k3fd->edc_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
    printk("lcd product id is 0x%x\n", lcd_product_id);

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
	/* for cabc */
	lcd_props.type = TFT;
	lcd_props.default_gamma = GAMMA25;
	p_tuning_dev = lcd_tuning_dev_register(&lcd_props, &sp_tuning_ops, (void *)reg_pdev);
	if (IS_ERR(p_tuning_dev)) {
		k3fb_loge("lcd_tuning_dev_register failed!\n");
		return -1;
	}

	jdi_sysfs_init(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe = jdi_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_jdi_OTM1282B",
	},
};

static int __init mipi_jdi_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		k3fb_loge("not able to register the driver\n");
		return ret;
	}

	return ret;
}

module_init(mipi_jdi_panel_init);
