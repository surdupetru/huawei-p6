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

#ifndef SBL_REG_H
#define SBL_REG_H

#include "k3_edc.h"
#include "k3_fb_def.h"

#define REG_SBL_EN 0x40000000
/******************************************************************************
** SMART BACKLIGHT
*/
#define SBL_OFFSET				(0x1000)
#define SBL_CTRL_REG0_OFFSET		(SBL_OFFSET + 0x0)
#define SBL_CTRL_REG1_OFFSET		(SBL_OFFSET + 0x4)
#define SBL_HS_POS_LOFFSET		(SBL_OFFSET + 0x8)
#define SBL_HS_POS_HOFFSET		(SBL_OFFSET + 0xC)
#define SBL_FRAME_WIDTH_LOFFSET	(SBL_OFFSET + 0x10)
#define SBL_FRAME_WIDTH_HOFFSET		(SBL_OFFSET + 0x14)
#define SBL_FRAME_HEIGHT_LOFFSET		(SBL_OFFSET + 0x18)
#define SBL_FRAME_HEIGHT_HOFFSET		(SBL_OFFSET + 0x1C)
#define SBL_VS_POS_LOFFSET			(SBL_OFFSET + 0x20)
#define SBL_VS_POS_HOFFSET		(SBL_OFFSET + 0x24)
#define SBL_IRIDIX_CTRL0_OFFSET		(SBL_OFFSET + 0x400)
#define SBL_IRIDIX_CTRL1_OFFSET		(SBL_OFFSET + 0x404)
#define SBL_VARIANCE_OFFSET	(SBL_OFFSET + 0x40C)
#define SBL_SLOPE_MAX_OFFSET		(SBL_OFFSET + 0x410)
#define SBL_SLOPE_MIN_OFFSET		(SBL_OFFSET + 0x414)
#define SBL_BLACK_LEVEL_LOFFSET		(SBL_OFFSET + 0x418)
#define SBL_BLACK_LEVEL_HOFFSET		(SBL_OFFSET + 0x41C)
#define SBL_WHITE_LEVEL_LOFFSET		(SBL_OFFSET + 0x420)
#define SBL_WHITE_LEVEL_HOFFSET		(SBL_OFFSET + 0x424)
#define SBL_LIMIT_AMP_OFFSET		(SBL_OFFSET + 0x428)
#define SBL_DITHER_OFFSET		(SBL_OFFSET + 0x42C)
#define SBL_LOGO_LEFT_OFFSET		(SBL_OFFSET + 0x800)
#define SBL_LOGO_RIGHT_OFFSET		(SBL_OFFSET + 0x804)
#define SBL_DITHER_CTRL_OFFSET		(SBL_OFFSET + 0x840)
#define SBL_STRENGTH_SEL_OFFSET		(SBL_OFFSET + 0xC00)
#define SBL_STRENGTH_LIMIT_OFFSET		(SBL_OFFSET + 0xC28)
#define SBL_STRENGTH_MANUAL_OFFSET		(SBL_OFFSET + 0xC04)
#define SBL_OPTION_SEL_OFFSET		(SBL_OFFSET + 0xC08)
#define SBL_AMBIENT_LIGHT_LOFFSET		(SBL_OFFSET + 0xC10)
#define SBL_AMBIENT_LIGHT_HOFFSET		(SBL_OFFSET + 0xC14)
#define SBL_BKL_LEVEL_LOFFSET		(SBL_OFFSET + 0xC18)
#define SBL_BKL_LEVEL_HOFFSET		(SBL_OFFSET + 0xC1C)
#define SBL_BKL_MAX_LOFFSET		(SBL_OFFSET + 0xC20)
#define SBL_BKL_MAX_HOFFSET		(SBL_OFFSET + 0xC24)
#define SBL_CALIBRATION_A_OFFSET		(SBL_OFFSET + 0xC2C)
#define SBL_CALIBRATION_B_OFFSET		(SBL_OFFSET + 0xC30)
#define SBL_DRC_IN_LOFFSET		(SBL_OFFSET + 0xC38)
#define SBL_DRC_IN_HOFFSET		(SBL_OFFSET + 0xC3C)
#define SBL_T_FILT_CTRL_OFFSET		(SBL_OFFSET + 0xC34)

#define SBL_START_CALC_OFFSET		(SBL_OFFSET + 0xC40)
/******************************************************************************
** FUNCTIONS PROTOTYPES
*/
void set_SBL_CTRL_REG0_data_format(u32 edc_base, u32 nVal);
void set_SBL_CTRL_REG1_auto_pos(u32 edc_base, u32 nVal);
void set_SBL_CTRL_REG1_auto_size(u32 edc_base, u32 nVal);
void set_SBL_HS_POS_L_hs_pos_l(u32 edc_base, u32 nVal);
void set_SBL_HS_POS_H_hs_pos_h(u32 edc_base, u32 nVal);
void set_SBL_FRAME_WIDTH_L_frame_width_l(u32 edc_base, u32 nVal);
void set_SBL_FRAME_WIDTH_H_frame_width_h(u32 edc_base, u32 nVal);
void set_SBL_FRAME_HEIGHT_L_frame_height_l(u32 edc_base, u32 nVal);
void set_SBL_FRAME_HEIGHT_H_frame_height_h(u32 edc_base, u32 nVal);
void set_SBL_VS_POS_L_vs_pos_l(u32 edc_base, u32 nVal);
void set_SBL_VS_POS_H_vs_pos_h(u32 edc_base, u32 nVal);
void set_SBL_IRIDIX_CTRL0_iridix_enable(u32 edc_base, u32 nVal);
void set_SBL_IRIDIX_CTRL0_local_contrast_ctrl(u32 edc_base, u32 nVal);
void set_SBL_IRIDIX_CTRL0_local_contrast_2d(u32 edc_base, u32 nVal);
void set_SBL_IRIDIX_CTRL0_color_correction(u32 edc_base, u32 nVal);
void set_SBL_IRIDIX_CTRL1_iridix_ctrl_port(u32 edc_base, u32 nVal);
void set_SBL_VARIANCE_variance_space(u32 edc_base, u32 nVal);
void set_SBL_VARIANCE_variance_intensity(u32 edc_base, u32 nVal);
void set_SBL_SLOPE_MAX_slope_max(u32 edc_base, u32 nVal);
void set_SBL_SLOPE_MIN_slope_min(u32 edc_base, u32 nVal);
void set_SBL_BLACK_LEVEL_L_black_level_l(u32 edc_base, u32 nVal);
void set_SBL_BLACK_LEVEL_H_black_level_h(u32 edc_base, u32 nVal);
void set_SBL_WHITE_LEVEL_L_white_level_l(u32 edc_base, u32 nVal);
void set_SBL_WHITE_LEVEL_H_white_level_h(u32 edc_base, u32 nVal);
void set_SBL_LIMIT_AMP_dark_amp_limit(u32 edc_base, u32 nVal);
void set_SBL_LIMIT_AMP_bright_amp_limit(u32 edc_base, u32 nVal);
void set_SBL_DITHER_dither_mode(u32 edc_base, u32 nVal);
void set_SBL_LOGO_LEFT_logo_left(u32 edc_base, u32 nVal);
void set_SBL_LOGO_RIGHT_logo_right(u32 edc_base, u32 nVal);
void set_SBL_DITHER_CTRL_dither_enable(u32 edc_base, u32 nVal);
void set_SBL_DITHER_CTRL_dither_amount(u32 edc_base, u32 nVal);
void set_SBL_DITHER_CTRL_shift_mode(u32 edc_base, u32 nVal);
void set_SBL_DITHER_CTRL_dither_bypass(u32 edc_base, u32 nVal);
void set_SBL_STRENGTH_SEL_strength_sel(u32 edc_base, u32 nVal);
void set_SBL_STRENGTH_LIMIT_strength_limit(u32 edc_base, u32 nVal);
void set_SBL_STRENGTH_MANUAL_strength_manual(u32 edc_base, u32 nVal);
void set_SBL_OPTION_SEL_option_sel(u32 edc_base, u32 nVal);
void set_SBL_AMBIENT_LIGHT_L_ambient_light_l(u32 edc_base, u32 nVal);
void set_SBL_AMBIENT_LIGHT_H_ambient_light_h(u32 edc_base, u32 nVal);
void set_SBL_BKL_LEVEL_L_bkl_level_l(u32 edc_base, u32 nVal);
void set_SBL_BKL_LEVEL_H_bkl_level_h(u32 edc_base, u32 nVal);
void set_SBL_BKL_MAX_L_bkl_max_l(u32 edc_base, u32 nVal);
void set_SBL_BKL_MAX_H_bkl_max_h(u32 edc_base, u32 nVal);
void set_SBL_CALIBRATION_A_calibration_a(u32 edc_base, u32 nVal);
void set_SBL_CALIBRATION_B_calibration_b(u32 edc_base, u32 nVal);
void set_SBL_DRC_IN_L_drc_in_l(u32 edc_base, u32 nVal);
void set_SBL_DRC_IN_H_drc_in_h(u32 edc_base, u32 nVal);
void set_SBL_T_FILT_CTRL_t_filt_ctrl(u32 edc_base, u32 nVal);
void set_SBL_START_CALC_start_calc(u32 edc_base, u32 nVal);



#endif  /* __SBL_REG_H__ */



