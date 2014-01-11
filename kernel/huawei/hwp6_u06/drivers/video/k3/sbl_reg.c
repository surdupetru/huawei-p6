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

#include "sbl_reg.h"

typedef union {
	struct {
		u32 data_format:2;
		u32 reserved0:30;
	} bits;
	u32 ul32;
} SBL_CTRL_REG0;

typedef union {
	struct {
		u32 reserved0:6;
		u32 auto_pos:1;
		u32 auto_size:1;
		u32 reserved1:24;
	} bits;
	u32 ul32;
} SBL_CTRL_REG1;

typedef union {
	struct {
		u32 hs_pos_l:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_HS_POS_L;

typedef union {
	struct {
		u32 hs_pos_h:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_HS_POS_H;


typedef union {
	struct {
		u32 frame_width_l:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_FRAME_WIDTH_L;

typedef union {
	struct {
		u32 frame_width_h:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_FRAME_WIDTH_H;

typedef union {
	struct {
		u32 frame_height_l:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_FRAME_HEIGHT_L;

typedef union {
	struct {
		u32 frame_height_h:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_FRAME_HEIGHT_H;

typedef union {
	struct {
		u32 vs_pos_l:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_VS_POS_L;

typedef union {
	struct {
		u32 vs_pos_h:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_VS_POS_H;

typedef union {
	struct {
		u32 iridix_enable:1;
		u32 local_contrast_ctrl:2;
		u32 local_contrast_2d:1;
		u32 color_correction:1;
		u32 reserved0:27;
	} bits;
	u32 ul32;
} SBL_IRIDIX_CTRL0;

typedef union {
	struct {
		u32 iridix_ctrl_port:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_IRIDIX_CTRL1;

typedef union {
	struct {
		u32 variance_space:4;
		u32 variance_intensity:4;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_VARIANCE;

typedef union {
	struct {
		u32 slope_max:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_SLOPE_MAX;

typedef union {
	struct {
		u32 slope_min:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_SLOPE_MIN;

typedef union {
	struct {
		u32 black_level_l:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_BLACK_LEVEL_L;

typedef union {
	struct {
		u32 black_level_h:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_BLACK_LEVEL_H;

typedef union {
	struct {
		u32 white_level_l:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_WHITE_LEVEL_L;

typedef union {
	struct {
		u32 white_level_h:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_WHITE_LEVEL_H;

typedef union {
	struct {
		u32 dark_amp_limit:4;
		u32 bright_amp_limit:4;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_LIMIT_AMP;

typedef union {
	struct {
		u32 dither_mode:3;
		u32 reserved0:29;
	} bits;
	u32 ul32;
} SBL_DITHER;

typedef union {
	struct {
		u32 logo_left:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_LOGO_LEFT;

typedef union {
	struct {
		u32 logo_right:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_LOGO_RIGHT;

typedef union {
	struct {
		u32 dither_enable:1;
		u32 dither_amount:1;
		u32 shift_mode:1;
		u32 reserved0:4;
		u32 dither_bypass:1;
		u32 reserved1:24;
	} bits;
	u32 ul32;
} SBL_DITHER_CTRL;

typedef union {
	struct {
		u32 strength_sel:1;
		u32 reserved0:31;
	} bits;
	u32 ul32;
} SBL_STRENGTH_SEL;

typedef union {
	struct {
		u32 strength_limit:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_STRENGTH_LIMIT;

typedef union {
	struct {
		u32 strength_manual:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_STRENGTH_MANUAL;

typedef union {
	struct {
		u32 option_sel:2;
		u32 reserved0:30;
	} bits;
	u32 ul32;
} SBL_OPTION_SEL;

typedef union {
	struct {
		u32 ambient_light_l:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_AMBIENT_LIGHT_L;

typedef union {
	struct {
		u32 ambient_light_h:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_AMBIENT_LIGHT_H;

typedef union {
	struct {
		u32 bkl_level_l:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_BKL_LEVEL_L;

typedef union {
	struct {
		u32 bkl_level_h:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_BKL_LEVEL_H;

typedef union {
	struct {
		u32 bkl_max_l:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_BKL_MAX_L;

typedef union {
	struct {
		u32 bkl_max_h:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_BKL_MAX_H;

typedef union {
	struct {
		u32 calibration_a:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_CALIBRATION_A;

typedef union {
	struct {
		u32 calibration_b:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_CALIBRATION_B;

typedef union {
	struct {
		u32 drc_in_l:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_DRC_IN_L;

typedef union {
	struct {
		u32 drc_in_h:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_DRC_IN_H;

typedef union {
	struct {
		u32 t_filt_ctrl:8;
		u32 reserved0:24;
	} bits;
	u32 ul32;
} SBL_T_FILT_CTRL;

typedef union {
	struct {
		u32 start_calc:1;
		u32 reserved0:31;
	} bits;
	u32 ul32;
} SBL_START_CALC;

/******************************************************************************
** FUNCTIONS IMPLEMENTATIONS
*/

void set_SBL_CTRL_REG0_data_format(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_CTRL_REG0_OFFSET;
	SBL_CTRL_REG0 sbl_ctrl_reg0;

	sbl_ctrl_reg0.ul32 = inp32(addr);
	sbl_ctrl_reg0.bits.data_format = nVal;
	outp32(addr, sbl_ctrl_reg0.ul32);
}

void set_SBL_CTRL_REG1_auto_pos(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_CTRL_REG1_OFFSET;
	SBL_CTRL_REG1 sbl_ctrl_reg1;

	sbl_ctrl_reg1.ul32 = inp32(addr);
	sbl_ctrl_reg1.bits.auto_pos = nVal;
	outp32(addr, sbl_ctrl_reg1.ul32);
}

void set_SBL_CTRL_REG1_auto_size(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_CTRL_REG1_OFFSET;
	SBL_CTRL_REG1 sbl_ctrl_reg1;

	sbl_ctrl_reg1.ul32 = inp32(addr);
	sbl_ctrl_reg1.bits.auto_size = nVal;
	outp32(addr, sbl_ctrl_reg1.ul32);
}

void set_SBL_HS_POS_L_hs_pos_l(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_HS_POS_LOFFSET;
	SBL_HS_POS_L sbl_hs_pos_l;

	sbl_hs_pos_l.ul32 = inp32(addr);
	sbl_hs_pos_l.bits.hs_pos_l = nVal;
	outp32(addr, sbl_hs_pos_l.ul32);
}

void set_SBL_HS_POS_H_hs_pos_h(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_HS_POS_HOFFSET;
	SBL_HS_POS_H sbl_hs_pos_h;

	sbl_hs_pos_h.ul32 = inp32(addr);
	sbl_hs_pos_h.bits.hs_pos_h = nVal;
	outp32(addr, sbl_hs_pos_h.ul32);
}

void set_SBL_FRAME_WIDTH_L_frame_width_l(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_FRAME_WIDTH_LOFFSET;
	SBL_FRAME_WIDTH_L sbl_frame_width_l;

	sbl_frame_width_l.ul32 = inp32(addr);
	sbl_frame_width_l.bits.frame_width_l = nVal;
	outp32(addr, sbl_frame_width_l.ul32);
}

void set_SBL_FRAME_WIDTH_H_frame_width_h(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_FRAME_WIDTH_HOFFSET;
	SBL_FRAME_WIDTH_H sbl_frame_width_h;

	sbl_frame_width_h.ul32 = inp32(addr);
	sbl_frame_width_h.bits.frame_width_h = nVal;
	outp32(addr, sbl_frame_width_h.ul32);
}

void set_SBL_FRAME_HEIGHT_L_frame_height_l(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_FRAME_HEIGHT_LOFFSET;
	SBL_FRAME_HEIGHT_L sbl_frame_height_l;

	sbl_frame_height_l.ul32 = inp32(addr);
	sbl_frame_height_l.bits.frame_height_l = nVal;
	outp32(addr, sbl_frame_height_l.ul32);
}

void set_SBL_FRAME_HEIGHT_H_frame_height_h(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_FRAME_HEIGHT_HOFFSET;
	SBL_FRAME_HEIGHT_H sbl_frame_height_h;

	sbl_frame_height_h.ul32 = inp32(addr);
	sbl_frame_height_h.bits.frame_height_h = nVal;
	outp32(addr, sbl_frame_height_h.ul32);
}

void set_SBL_VS_POS_L_vs_pos_l(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_VS_POS_LOFFSET;
	SBL_VS_POS_L sbl_vs_pos_l;

	sbl_vs_pos_l.ul32 = inp32(addr);
	sbl_vs_pos_l.bits.vs_pos_l = nVal;
	outp32(addr, sbl_vs_pos_l.ul32);
}

void set_SBL_VS_POS_H_vs_pos_h(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_VS_POS_HOFFSET;
	SBL_VS_POS_H sbl_vs_pos_h;

	sbl_vs_pos_h.ul32 = inp32(addr);
	sbl_vs_pos_h.bits.vs_pos_h = nVal;
	outp32(addr, sbl_vs_pos_h.ul32);
}

void set_SBL_IRIDIX_CTRL0_iridix_enable(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_IRIDIX_CTRL0_OFFSET;
	SBL_IRIDIX_CTRL0 sbl_iridix_ctrl0;

	sbl_iridix_ctrl0.ul32 = inp32(addr);
	sbl_iridix_ctrl0.bits.iridix_enable = nVal;
	outp32(addr, sbl_iridix_ctrl0.ul32);
}

void set_SBL_IRIDIX_CTRL0_local_contrast_ctrl(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_IRIDIX_CTRL0_OFFSET;
	SBL_IRIDIX_CTRL0 sbl_iridix_ctrl0;

	sbl_iridix_ctrl0.ul32 = inp32(addr);
	sbl_iridix_ctrl0.bits.local_contrast_ctrl = nVal;
	outp32(addr, sbl_iridix_ctrl0.ul32);
}

void set_SBL_IRIDIX_CTRL0_local_contrast_2d(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_IRIDIX_CTRL0_OFFSET;
	SBL_IRIDIX_CTRL0 sbl_iridix_ctrl0;

	sbl_iridix_ctrl0.ul32 = inp32(addr);
	sbl_iridix_ctrl0.bits.local_contrast_2d = nVal;
	outp32(addr, sbl_iridix_ctrl0.ul32);
}

void set_SBL_IRIDIX_CTRL0_color_correction(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_IRIDIX_CTRL0_OFFSET;
	SBL_IRIDIX_CTRL0 sbl_iridix_ctrl0;

	sbl_iridix_ctrl0.ul32 = inp32(addr);
	sbl_iridix_ctrl0.bits.color_correction = nVal;
	outp32(addr, sbl_iridix_ctrl0.ul32);
}

void set_SBL_IRIDIX_CTRL1_iridix_ctrl_port(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_IRIDIX_CTRL1_OFFSET;
	SBL_IRIDIX_CTRL1 sbl_iridix_ctrl1;

	sbl_iridix_ctrl1.ul32 = inp32(addr);
	sbl_iridix_ctrl1.bits.iridix_ctrl_port = nVal;
	outp32(addr, sbl_iridix_ctrl1.ul32);
}

void set_SBL_VARIANCE_variance_space(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_VARIANCE_OFFSET;
	SBL_VARIANCE sbl_variance;

	sbl_variance.ul32 = inp32(addr);
	sbl_variance.bits.variance_space = nVal;
	outp32(addr, sbl_variance.ul32);
}

void set_SBL_VARIANCE_variance_intensity(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_VARIANCE_OFFSET;
	SBL_VARIANCE sbl_variance;

	sbl_variance.ul32 = inp32(addr);
	sbl_variance.bits.variance_intensity = nVal;
	outp32(addr, sbl_variance.ul32);
}

void set_SBL_SLOPE_MAX_slope_max(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_SLOPE_MAX_OFFSET;
	SBL_SLOPE_MAX sbl_slope_max;

	sbl_slope_max.ul32 = inp32(addr);
	sbl_slope_max.bits.slope_max = nVal;
	outp32(addr, sbl_slope_max.ul32);
}

void set_SBL_SLOPE_MIN_slope_min(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_SLOPE_MIN_OFFSET;
	SBL_SLOPE_MIN sbl_slope_min;

	sbl_slope_min.ul32 = inp32(addr);
	sbl_slope_min.bits.slope_min = nVal;
	outp32(addr, sbl_slope_min.ul32);
}

void set_SBL_BLACK_LEVEL_L_black_level_l(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_BLACK_LEVEL_LOFFSET;
	SBL_BLACK_LEVEL_L sbl_black_level_l;

	sbl_black_level_l.ul32 = inp32(addr);
	sbl_black_level_l.bits.black_level_l = nVal;
	outp32(addr, sbl_black_level_l.ul32);
}

void set_SBL_BLACK_LEVEL_H_black_level_h(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_BLACK_LEVEL_HOFFSET;
	SBL_BLACK_LEVEL_H sbl_black_level_h;

	sbl_black_level_h.ul32 = inp32(addr);
	sbl_black_level_h.bits.black_level_h = nVal;
	outp32(addr, sbl_black_level_h.ul32);
}

void set_SBL_WHITE_LEVEL_L_white_level_l(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_WHITE_LEVEL_LOFFSET;
	SBL_WHITE_LEVEL_L sbl_white_level_l;

	sbl_white_level_l.ul32 = inp32(addr);
	sbl_white_level_l.bits.white_level_l = nVal;
	outp32(addr, sbl_white_level_l.ul32);
}

void set_SBL_WHITE_LEVEL_H_white_level_h(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_WHITE_LEVEL_HOFFSET;
	SBL_WHITE_LEVEL_H sbl_white_level_h;

	sbl_white_level_h.ul32 = inp32(addr);
	sbl_white_level_h.bits.white_level_h = nVal;
	outp32(addr, sbl_white_level_h.ul32);
}

void set_SBL_LIMIT_AMP_dark_amp_limit(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_LIMIT_AMP_OFFSET;
	SBL_LIMIT_AMP sbl_limit_amp;

	sbl_limit_amp.ul32 = inp32(addr);
	sbl_limit_amp.bits.dark_amp_limit = nVal;
	outp32(addr, sbl_limit_amp.ul32);
}

void set_SBL_LIMIT_AMP_bright_amp_limit(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_LIMIT_AMP_OFFSET;
	SBL_LIMIT_AMP sbl_limit_amp;

	sbl_limit_amp.ul32 = inp32(addr);
	sbl_limit_amp.bits.bright_amp_limit = nVal;
	outp32(addr, sbl_limit_amp.ul32);
}

void set_SBL_DITHER_dither_mode(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_DITHER_OFFSET;
	SBL_DITHER sbl_dither;

	sbl_dither.ul32 = inp32(addr);
	sbl_dither.bits.dither_mode = nVal;
	outp32(addr, sbl_dither.ul32);
}

void set_SBL_LOGO_LEFT_logo_left(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_LOGO_LEFT_OFFSET;
	SBL_LOGO_LEFT sbl_logo_left;

	sbl_logo_left.ul32 = inp32(addr);
	sbl_logo_left.bits.logo_left = nVal;
	outp32(addr, sbl_logo_left.ul32);
}

void set_SBL_LOGO_RIGHT_logo_right(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_LOGO_RIGHT_OFFSET;
	SBL_LOGO_RIGHT sbl_logo_right;

	sbl_logo_right.ul32 = inp32(addr);
	sbl_logo_right.bits.logo_right = nVal;
	outp32(addr, sbl_logo_right.ul32);
}

void set_SBL_DITHER_CTRL_dither_enable(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_DITHER_CTRL_OFFSET;
	SBL_DITHER_CTRL sbl_dither_ctrl;

	sbl_dither_ctrl.ul32 = inp32(addr);
	sbl_dither_ctrl.bits.dither_enable = nVal;
	outp32(addr, sbl_dither_ctrl.ul32);
}

void set_SBL_DITHER_CTRL_dither_amount(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_DITHER_CTRL_OFFSET;
	SBL_DITHER_CTRL sbl_dither_ctrl;

	sbl_dither_ctrl.ul32 = inp32(addr);
	sbl_dither_ctrl.bits.dither_amount = nVal;
	outp32(addr, sbl_dither_ctrl.ul32);
}

void set_SBL_DITHER_CTRL_shift_mode(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_DITHER_CTRL_OFFSET;
	SBL_DITHER_CTRL sbl_dither_ctrl;

	sbl_dither_ctrl.ul32 = inp32(addr);
	sbl_dither_ctrl.bits.shift_mode = nVal;
	outp32(addr, sbl_dither_ctrl.ul32);
}

void set_SBL_DITHER_CTRL_dither_bypass(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_DITHER_CTRL_OFFSET;
	SBL_DITHER_CTRL sbl_dither_ctrl;

	sbl_dither_ctrl.ul32 = inp32(addr);
	sbl_dither_ctrl.bits.dither_bypass = nVal;
	outp32(addr, sbl_dither_ctrl.ul32);
}

void set_SBL_STRENGTH_SEL_strength_sel(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_STRENGTH_SEL_OFFSET;
	SBL_STRENGTH_SEL sbl_strength_sel;

	sbl_strength_sel.ul32 = inp32(addr);
	sbl_strength_sel.bits.strength_sel = nVal;
	outp32(addr, sbl_strength_sel.ul32);
}

void set_SBL_STRENGTH_LIMIT_strength_limit(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_STRENGTH_LIMIT_OFFSET;
	SBL_STRENGTH_LIMIT sbl_strength_limit;

	sbl_strength_limit.ul32 = inp32(addr);
	sbl_strength_limit.bits.strength_limit = nVal;
	outp32(addr, sbl_strength_limit.ul32);
}

void set_SBL_STRENGTH_MANUAL_strength_manual(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_STRENGTH_MANUAL_OFFSET;
	SBL_STRENGTH_MANUAL sbl_strength_manual;

	sbl_strength_manual.ul32 = inp32(addr);
	sbl_strength_manual.bits.strength_manual = nVal;
	outp32(addr, sbl_strength_manual.ul32);
}

void set_SBL_OPTION_SEL_option_sel(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_OPTION_SEL_OFFSET;
	SBL_OPTION_SEL sbl_option_sel;

	sbl_option_sel.ul32 = inp32(addr);
	sbl_option_sel.bits.option_sel = nVal;
	outp32(addr, sbl_option_sel.ul32);
}

void set_SBL_AMBIENT_LIGHT_L_ambient_light_l(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_AMBIENT_LIGHT_LOFFSET;
	SBL_AMBIENT_LIGHT_L sbl_ambient_light_l;

	sbl_ambient_light_l.ul32 = inp32(addr);
	sbl_ambient_light_l.bits.ambient_light_l = nVal;
	outp32(addr, sbl_ambient_light_l.ul32);
}

void set_SBL_AMBIENT_LIGHT_H_ambient_light_h(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_AMBIENT_LIGHT_HOFFSET;
	SBL_AMBIENT_LIGHT_H sbl_ambient_light_h;

	sbl_ambient_light_h.ul32 = inp32(addr);
	sbl_ambient_light_h.bits.ambient_light_h = nVal;
	outp32(addr, sbl_ambient_light_h.ul32);
}

void set_SBL_BKL_LEVEL_L_bkl_level_l(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_BKL_LEVEL_LOFFSET;
	SBL_BKL_LEVEL_L sbl_bkl_level_l;

	sbl_bkl_level_l.ul32 = inp32(addr);
	sbl_bkl_level_l.bits.bkl_level_l = nVal;
	outp32(addr, sbl_bkl_level_l.ul32);
}

void set_SBL_BKL_LEVEL_H_bkl_level_h(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_BKL_LEVEL_HOFFSET;
	SBL_BKL_LEVEL_H sbl_bkl_level_h;

	sbl_bkl_level_h.ul32 = inp32(addr);
	sbl_bkl_level_h.bits.bkl_level_h = nVal;
	outp32(addr, sbl_bkl_level_h.ul32);
}

void set_SBL_BKL_MAX_L_bkl_max_l(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_BKL_MAX_LOFFSET;
	SBL_BKL_MAX_L sbl_bkl_max_l;

	sbl_bkl_max_l.ul32 = inp32(addr);
	sbl_bkl_max_l.bits.bkl_max_l = nVal;
	outp32(addr, sbl_bkl_max_l.ul32);
}

void set_SBL_BKL_MAX_H_bkl_max_h(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_BKL_MAX_HOFFSET;
	SBL_BKL_MAX_H sbl_bkl_max_h;

	sbl_bkl_max_h.ul32 = inp32(addr);
	sbl_bkl_max_h.bits.bkl_max_h = nVal;
	outp32(addr, sbl_bkl_max_h.ul32);
}

void set_SBL_CALIBRATION_A_calibration_a(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_CALIBRATION_A_OFFSET;
	SBL_CALIBRATION_A sbl_calibration_a;

	sbl_calibration_a.ul32 = inp32(addr);
	sbl_calibration_a.bits.calibration_a = nVal;
	outp32(addr, sbl_calibration_a.ul32);
}

void set_SBL_CALIBRATION_B_calibration_b(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_CALIBRATION_B_OFFSET;
	SBL_CALIBRATION_B sbl_calibration_b;

	sbl_calibration_b.ul32 = inp32(addr);
	sbl_calibration_b.bits.calibration_b = nVal;
	outp32(addr, sbl_calibration_b.ul32);
}

void set_SBL_DRC_IN_L_drc_in_l(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_DRC_IN_LOFFSET;
	SBL_DRC_IN_L sbl_drc_in_l;

	sbl_drc_in_l.ul32 = inp32(addr);
	sbl_drc_in_l.bits.drc_in_l = nVal;
	outp32(addr, sbl_drc_in_l.ul32);
}

void set_SBL_DRC_IN_H_drc_in_h(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_DRC_IN_HOFFSET;
	SBL_DRC_IN_H sbl_drc_in_h;

	sbl_drc_in_h.ul32 = inp32(addr);
	sbl_drc_in_h.bits.drc_in_h = nVal;
	outp32(addr, sbl_drc_in_h.ul32);
}

void set_SBL_T_FILT_CTRL_t_filt_ctrl(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_T_FILT_CTRL_OFFSET;
	SBL_T_FILT_CTRL sbl_t_filt_ctrl;

	sbl_t_filt_ctrl.ul32 = inp32(addr);
	sbl_t_filt_ctrl.bits.t_filt_ctrl = nVal;
	outp32(addr, sbl_t_filt_ctrl.ul32);
}

void set_SBL_START_CALC_start_calc(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + SBL_START_CALC_OFFSET;
	SBL_START_CALC sbl_start_calc;

	sbl_start_calc.ul32 = inp32(addr);
	sbl_start_calc.bits.start_calc = nVal;
	outp32(addr, sbl_start_calc.ul32);
}
