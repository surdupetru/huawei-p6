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

#include "edc_reg.h"

typedef union {
	struct {
		u32 id:32;
	} bits;
	u32 ul32;
} EDC_ID;

typedef union {
	struct {
		u32 ch1l_addr:32;
	} bits;
	u32 ul32;
} EDC_CH1L_ADDR;

typedef union {
	struct {
		u32 ch1r_addr:32;
	} bits;
	u32 ul32;
} EDC_CH1R_ADDR;

typedef union {
	struct {
		u32 ch1_ln_stride:14;
		u32 reserved0:18;
	} bits;
	u32 ul32;
} EDC_CH1_STRIDE;

typedef union {
	struct {
		u32 ch1_y:12;
		u32 reserved0:4;
		u32 ch1_x:12;
		u32 reserved1:4;
	} bits;
	u32 ul32;
} EDC_CH1_XY;

typedef union {
	struct {
		u32 ch1_size_vrt:12;
		u32 reserved0:4;
		u32 ch1_size_hrz:12;
		u32 reserved1:4;
	} bits;
	u32 ul32;
} EDC_CH1_SIZE;

typedef union {
	struct {
		u32 ch1_secu_line:12;
		u32 ch1_y_flip:1;
		u32 ch1_tile_fmt:2;
		u32 ch1_tile_en:1;
		u32 ch1_pix_fmt:3;
		u32 ch1_bgr:1;
		u32 ch1_colork_en:1;
		u32 ch1_rot:2;
		u32 reserved1:1;
		u32 ch1_en:1;
		u32 ch1_alp_premulti:1;
		u32 reserved2:6;
	} bits;
	u32 ul32;
} EDC_CH1_CTL;

typedef union {
	struct {
		u32 ch1_colork_min:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH1_COLORK_MIN;

typedef union {
	struct {
		u32 ch1_colork_max:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH1_COLORK_MAX;

typedef union {
	struct {
		u32 ch2l_addr:32;
	} bits;
	u32 ul32;
} EDC_CH2L_ADDR;

typedef union {
	struct {
		u32 ch2r_addr:32;
	} bits;
	u32 ul32;
} EDC_CH2R_ADDR;

typedef union {
	struct {
		u32 ch2_ln_stride:14;
		u32 reserved0:18;
	} bits;
	u32 ul32;
} EDC_CH2_STRIDE;

typedef union {
	struct {
		u32 ch2_y:12;
		u32 reserved0:4;
		u32 ch2_x:12;
		u32 reserved1:4;
	} bits;
	u32 ul32;
} EDC_CH2_XY;

typedef union {
	struct {
		u32 ch2_size_vrt:12;
		u32 reserved0:4;
		u32 ch2_size_hrz:12;
		u32 reserved1:4;
	} bits;
	u32 ul32;
} EDC_CH2_SIZE;

typedef union {
	struct {
		u32 ch2_secu_line:12;
		u32 reserved0:4;
		u32 ch2_pix_fmt:3;
		u32 ch2_bgr:1;
		u32 ch2_colork_en:1;
		u32 ch2_en:1;
		u32 reserved1:10;
	} bits;
	u32 ul32;
} EDC_CH2_CTL;

typedef union {
	struct {
		u32 ch2_colork_min:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH2_COLORK_MIN;

typedef union {
	struct {
		u32 ch2_colork_max:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH2_COLORK_MAX;

typedef union {
	struct {
		u32 ch12_mskc3_en:1;
		u32 ch12_mskc2_en:1;
		u32 ch12_mskc1_en:1;
		u32 ch12_mskc0_en:1;
		u32 ch12_blend_mskc_src:1;
		u32 ch2_alp_sel:2;
		u32 ch1_alp_sel:2;
		u32 ch12_pix_alp_src:1;
		u32 ch12_alp_mode:1;
		u32 ch12_blend_en:1;
		u32 ch2_top:1;
		u32 reserved0:19;
	} bits;
	u32 ul32;
} EDC_CH12_OVLY;

typedef union {
	struct {
		u32 ch12_alp_value0:8;
		u32 ch12_alp_value1:8;
		u32 reserved0:16;
	} bits;
	u32 ul32;
} EDC_CH12_GLB_ALP_VAL;

typedef union {
	struct {
		u32 ch12_mskc0_min:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH12_MSK_COLOR0_MIN;

typedef union {
	struct {
		u32 ch12_mskc0_max:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH12_MSK_COLOR0_MAX;

typedef union {
	struct {
		u32 ch12_mskc1_min:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH12_MSK_COLOR1_MIN;

typedef union {
	struct {
		u32 ch12_mskc1_max:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH12_MSK_COLOR1_MAX;

typedef union {
	struct {
		u32 ch12_mskc2_min:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH12_MSK_COLOR2_MIN;

typedef union {
	struct {
		u32 ch12_mskc2_max:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH12_MSK_COLOR2_MAX;

typedef union {
	struct {
		u32 ch12_mskc3_min:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH12_MSK_COLOR3_MIN;

typedef union {
	struct {
		u32 ch12_mskc3_max:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH12_MSK_COLOR3_MAX;

typedef union {
	struct {
		u32 crsl_addr:32;
	} bits;
	u32 ul32;
} EDC_CRSL_ADDR;

typedef union {
	struct {
		u32 crsr_addr:32;
	} bits;
	u32 ul32;
} EDC_CRSR_ADDR;

typedef union {
	struct {
		u32 crs_ln_stride:14;
		u32 reserved0:18;
	} bits;
	u32 ul32;
} EDC_CRS_STRIDE;

typedef union {
	struct {
		u32 crs_y:12;
		u32 reserved0:4;
		u32 crs_x:12;
		u32 reserved1:4;
	} bits;
	u32 ul32;
} EDC_CRS_XY;

typedef union {
	struct {
		u32 crs_size_vrt:12;
		u32 reserved0:4;
		u32 crs_size_hrz:12;
		u32 reserved1:4;
	} bits;
	u32 ul32;
} EDC_CRS_SIZE;

typedef union {
	struct {
		u32 crs_secu_line:12;
		u32 crs_alp_sel:2;
		u32 under_alp_sel:2;
		u32 reserved0:1;
		u32 crs_pix_fmt:2;
		u32 crs_bgr:1;
		u32 crs_colork_en:1;
		u32 reserved1:1;
		u32 crs_alp_mode:1;
		u32 crs_blend_en:1;
		u32 reserved2:3;
		u32 crs_en:1;
		u32 reserved3:4;
	} bits;
	u32 ul32;
} EDC_CRS_CTL;

typedef union {
	struct {
		u32 crs_colork_min:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CRS_COLORK_MIN;

typedef union {
	struct {
		u32 crs_colork_max:24;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CRS_COLORK_MAX;

typedef union {
	struct {
		u32 crs_alp_value0:8;
		u32 crs_alp_value1:8;
		u32 reserved0:16;
	} bits;
	u32 ul32;
} EDC_CRS_GLB_ALP_VAL;

typedef union {
	struct {
		u32 disp_size_vrt:12;
		u32 reserved0:4;
		u32 disp_size_hrz:12;
		u32 reserved1:4;
	} bits;
	u32 ul32;
} EDC_DISP_SIZE;

typedef union {
	struct {
		u32 edc_cfg_ok_sel:1;
		u32 edc_cfg_ok:1;
		u32 d3d_right_first:1;
		u32 disp_frm_fmt:3;
		u32 disp_pix_fmt:2;
		u32 dither_en:1;
		u32 disp_endian:1;
		u32 edc_en:1;
		u32 unflow_lev:12;
		u32 gamma_en:1;
		u32 ch_chg:1;
		u32 nrot_burst16:1;
		u32 outstding_dep:4;
		u32 frm_end_start:1;
		u32 crg_gt_en:1;
	} bits;
	u32 ul32;
} EDC_DISP_CTL;

typedef union {
	struct {
		u32 disp_dpd:24;
		u32 reserved0:4;
		u32 sbl_recover_bypass:1;
		u32 sbl_clk_gt:1;
		u32 sbl_en:1;
		u32 outstd_ctl_disable:1;
	} bits;
	u32 ul32;
} EDC_DISP_DPD;

typedef union {
	struct {
		u32 ovly_fifo_fill_lev:12;
		u32 reserved0:18;
		u32 bas_vsync:1;
		u32 outstding_zero:1;
	} bits;
	u32 ul32;
} EDC_STS;

typedef union {
	struct {
		u32 crsr_end_int:1;
		u32 crsl_end_int:1;
		u32 ch2r_end_int:1;
		u32 ch2l_end_int:1;
		u32 ch1r_end_int:1;
		u32 ch1l_end_int:1;
		u32 bas_end_int:1;
		u32 bas_stat_int:1;
		u32 crs_err_int:1;
		u32 ch2_line_int:1;
		u32 ch1_line_int:1;
		u32 crsr_err_int:1;
		u32 crsl_err_int:1;
		u32 ch2r_err_int:1;
		u32 ch2l_err_int:1;
		u32 ch1r_err_int:1;
		u32 ch1l_err_int:1;
		u32 unflow_int:1;
		u32 reserved0:14;
	} bits;
	u32 ul32;
} EDC_INTS;

typedef union {
	struct {
		u32 crsr_end_int_msk:1;
		u32 crsl_end_int_msk:1;
		u32 ch2r_end_int_msk:1;
		u32 ch2l_end_int_msk:1;
		u32 ch1r_end_int_msk:1;
		u32 ch1l_end_int_msk:1;
		u32 bas_end_int_msk:1;
		u32 bas_stat_int_msk:1;
		u32 crs_err_int_msk:1;
		u32 ch2_line_int_msk:1;
		u32 ch1_line_int_msk:1;
		u32 crsr_err_int_msk:1;
		u32 crsl_err_int_msk:1;
		u32 ch2r_err_int_msk:1;
		u32 ch2l_err_int_msk:1;
		u32 ch1r_err_int_msk:1;
		u32 ch1l_err_int_msk:1;
		u32 unflow_int_msk:1;
		u32 reserved0:14;
	} bits;
	u32 ul32;
} EDC_INTE;

typedef union {
	struct {
		u32 gamma_b:10;
		u32 gamma_g:10;
		u32 gamma_r:10;
		u32 reserved0:2;
	} bits;
	u32 ul32;
} EDC_GAMMA;

typedef union {
	struct {
		u32 ch1_cscidc_v:9;
		u32 ch1_cscidc_u:9;
		u32 ch1_cscidc_y:9;
		u32 ch1_csc_en:1;
		u32 reserved0:4;
	} bits;
	u32 ul32;
} EDC_CH1_CSCIDC;

typedef union {
	struct {
		u32 ch1_cscodc_b:9;
		u32 ch1_cscodc_g:9;
		u32 ch1_cscodc_r:9;
		u32 reserved0:5;
	} bits;
	u32 ul32;
} EDC_CH1_CSCODC;

typedef union {
	struct {
		u32 ch1_csc_00p:13;
		u32 reserved0:3;
		u32 ch1_csc_01p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_CH1_CSCP0;

typedef union {
	struct {
		u32 ch1_csc_02p:13;
		u32 reserved0:3;
		u32 ch1_csc_10p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_CH1_CSCP1;

typedef union {
	struct {
		u32 ch1_csc_11p:13;
		u32 reserved0:3;
		u32 ch1_csc_12p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_CH1_CSCP2;

typedef union {
	struct {
		u32 ch1_csc_20p:13;
		u32 reserved0:3;
		u32 ch1_csc_21p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_CH1_CSCP3;

typedef union {
	struct {
		u32 ch1_csc_22p:13;
		u32 reserved0:19;
	} bits;
	u32 ul32;
} EDC_CH1_CSCP4;

typedef union {
	struct {
		u32 ch2_cscidc_v:9;
		u32 ch2_cscidc_u:9;
		u32 ch2_cscidc_y:9;
		u32 ch2_csc_en:1;
		u32 reserved0:4;
	} bits;
	u32 ul32;
} EDC_CH2_CSCIDC;

typedef union {
	struct {
		u32 ch2_cscodc_b:9;
		u32 ch2_cscodc_g:9;
		u32 ch2_cscodc_r:9;
		u32 reserved0:5;
	} bits;
	u32 ul32;
} EDC_CH2_CSCODC;

typedef union {
	struct {
		u32 ch2_csc_00p:13;
		u32 reserved0:3;
		u32 ch2_csc_01p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_CH2_CSCP0;

typedef union {
	struct {
		u32 ch2_csc_02p:13;
		u32 reserved0:3;
		u32 ch2_csc_10p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_CH2_CSCP1;

typedef union {
	struct {
		u32 ch2_csc_11p:13;
		u32 reserved0:3;
		u32 ch2_csc_12p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_CH2_CSCP2;

typedef union {
	struct {
		u32 ch2_csc_20p:13;
		u32 reserved0:3;
		u32 ch2_csc_21p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_CH2_CSCP3;

typedef union {
	struct {
		u32 ch2_csc_22p:13;
		u32 reserved0:19;
	} bits;
	u32 ul32;
} EDC_CH2_CSCP4;

typedef union {
	struct {
		u32 out_cscidc_v:9;
		u32 out_cscidc_u:9;
		u32 out_cscidc_y:9;
		u32 out_csc_en:1;
		u32 reserved0:4;
	} bits;
	u32 ul32;
} EDC_OUT_CSCIDC;

typedef union {
	struct {
		u32 out_cscodc_b:9;
		u32 out_cscodc_g:9;
		u32 out_cscodc_r:9;
		u32 reserved0:5;
	} bits;
	u32 ul32;
} EDC_OUT_CSCODC;

typedef union {
	struct {
		u32 out_csc_00p:13;
		u32 reserved0:3;
		u32 out_csc_01p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_OUT_CSCP0;

typedef union {
	struct {
		u32 out_csc_02p:13;
		u32 reserved0:3;
		u32 out_csc_10p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_OUT_CSCP1;

typedef union {
	struct {
		u32 out_csc_11p:13;
		u32 reserved0:3;
		u32 out_csc_12p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_OUT_CSCP2;

typedef union {
	struct {
		u32 out_csc_20p:13;
		u32 reserved0:3;
		u32 out_csc_21p:13;
		u32 reserved1:3;
	} bits;
	u32 ul32;
} EDC_OUT_CSCP3;

typedef union {
	struct {
		u32 out_csc_22p:13;
		u32 reserved0:19;
	} bits;
	u32 ul32;
} EDC_OUT_CSCP4;

typedef union {
	struct {
		u32 hratio:16;
		u32 reserved0:3;
		u32 hfir_order:1;
		u32 reserved1:3;
		u32 hafir_en:1;
		u32 hfir_en:1;
		u32 reserved2:3;
		u32 hchmid_en:1;
		u32 hlmid_en:1;
		u32 hamid_en:1;
		u32 hsc_en:1;
	} bits;
	u32 ul32;
} EDC_CH1_SCL_HSP;

typedef union {
	struct {
		u32 hor_coffset:16;
		u32 hor_loffset:16;
	} bits;
	u32 ul32;
} EDC_CH1_SCL_HOFFSET;

typedef union {
	struct {
		u32 reserved0:23;
		u32 vafir_en:1;
		u32 vfir_en:1;
		u32 reserved1:2;
		u32 vsc_luma_tap:1;
		u32 vchmid_en:1;
		u32 vlmid_en:1;
		u32 vamid_en:1;
		u32 vsc_en:1;
	} bits;
	u32 ul32;
} EDC_CH1_SCL_VSP;

typedef union {
	struct {
		u32 vratio:16;
		u32 reserved0:16;
	} bits;
	u32 ul32;
} EDC_CH1_SCL_VSR;

typedef union {
	struct {
		u32 vbtm_offset:16;
		u32 vtp_offset:16;
	} bits;
	u32 ul32;
} EDC_CH1_SCL_VOFFSET;

typedef union {
	struct {
		u32 ow:12;
		u32 oh:12;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH1_SCL_ORES;

typedef union {
	struct {
		u32 iw:12;
		u32 ih:12;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} EDC_CH1_SCL_IRES;

typedef union {
	struct {
		u32 hlcoefn1:10;
		u32 reserved0:6;
		u32 hlcoefn2:10;
		u32 reserved1:6;
	} bits;
	u32 ul32;
} EDC_CH1_SCL_HPC;

typedef union {
	struct {
		u32 vlcoefn1:10;
		u32 reserved0:6;
		u32 vlcoefn2:10;
		u32 reserved1:6;
	} bits;
	u32 ul32;
} EDC_CH1_SCL_VPC;


/******************************************************************************
** FUNCTIONS IMPLEMENTATIONS
*/

void set_EDC_CH1L_ADDR(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1L_ADDR_OFFSET;
	EDC_CH1L_ADDR edc_ch1l_addr;

	edc_ch1l_addr.ul32 = inp32(addr);
	edc_ch1l_addr.bits.ch1l_addr = nVal;
	outp32(addr, edc_ch1l_addr.ul32);
}

void set_EDC_CH1R_ADDR(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1R_ADDR_OFFSET;
	EDC_CH1R_ADDR edc_ch1r_addr;

	edc_ch1r_addr.ul32 = inp32(addr);
	edc_ch1r_addr.bits.ch1r_addr = nVal;
	outp32(addr, edc_ch1r_addr.ul32);
}

void set_EDC_CH1_STRIDE(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_STRIDE_OFFSET;
	EDC_CH1_STRIDE edc_ch1_stride;

	edc_ch1_stride.ul32 = inp32(addr);
	edc_ch1_stride.bits.ch1_ln_stride = nVal;
	outp32(addr, edc_ch1_stride.ul32);
}

void set_EDC_CH1_XY(u32 edc_base, u32 ch1_x, u32 ch1_y)
{
	u32 addr = edc_base + EDC_CH1_XY_OFFSET;
	EDC_CH1_XY edc_ch1_xy;

	edc_ch1_xy.ul32 = inp32(addr);
	edc_ch1_xy.bits.ch1_x = ch1_x;
	edc_ch1_xy.bits.ch1_y = ch1_y;
	outp32(addr, edc_ch1_xy.ul32);
}

void set_EDC_CH1_SIZE(u32 edc_base, u32 ch1_size_hrz, u32 ch1_size_vrt)
{
	u32 addr = edc_base + EDC_CH1_SIZE_OFFSET;
	EDC_CH1_SIZE edc_ch1_size;

	edc_ch1_size.ul32 = inp32(addr);
	edc_ch1_size.bits.ch1_size_hrz = (ch1_size_hrz > 0) ? ch1_size_hrz-1 : 0;
	edc_ch1_size.bits.ch1_size_vrt = (ch1_size_vrt > 0) ? ch1_size_vrt-1 : 0;
	outp32(addr, edc_ch1_size.ul32);
}

void set_EDC_CH1_CTL_secu_line(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CTL_OFFSET;
	EDC_CH1_CTL edc_ch1_ctl;

	edc_ch1_ctl.ul32 = inp32(addr);
	edc_ch1_ctl.bits.ch1_secu_line = (nVal > 0) ? nVal-1 : 0;
	outp32(addr, edc_ch1_ctl.ul32);
}

void set_EDC_CH1_CTL_y_flip(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CTL_OFFSET;
	EDC_CH1_CTL edc_ch1_ctl;

	edc_ch1_ctl.ul32 = inp32(addr);
	edc_ch1_ctl.bits.ch1_y_flip = nVal;
	outp32(addr, edc_ch1_ctl.ul32);
}

void set_EDC_CH1_CTL_tile_fmt(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CTL_OFFSET;
	EDC_CH1_CTL edc_ch1_ctl;

	edc_ch1_ctl.ul32 = inp32(addr);
	edc_ch1_ctl.bits.ch1_tile_fmt = nVal;
	outp32(addr, edc_ch1_ctl.ul32);
}

void set_EDC_CH1_CTL_tile_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CTL_OFFSET;
	EDC_CH1_CTL edc_ch1_ctl;

	edc_ch1_ctl.ul32 = inp32(addr);
	edc_ch1_ctl.bits.ch1_tile_en = nVal;
	outp32(addr, edc_ch1_ctl.ul32);
}

void set_EDC_CH1_CTL_pix_fmt(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CTL_OFFSET;
	EDC_CH1_CTL edc_ch1_ctl;

	edc_ch1_ctl.ul32 = inp32(addr);
	edc_ch1_ctl.bits.ch1_pix_fmt = nVal;
	outp32(addr, edc_ch1_ctl.ul32);
}

void set_EDC_CH1_CTL_bgr(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CTL_OFFSET;
	EDC_CH1_CTL edc_ch1_ctl;

	edc_ch1_ctl.ul32 = inp32(addr);
	edc_ch1_ctl.bits.ch1_bgr = nVal;
	outp32(addr, edc_ch1_ctl.ul32);
}

void set_EDC_CH1_CTL_colork_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CTL_OFFSET;
	EDC_CH1_CTL edc_ch1_ctl;

	edc_ch1_ctl.ul32 = inp32(addr);
	edc_ch1_ctl.bits.ch1_colork_en = nVal;
	outp32(addr, edc_ch1_ctl.ul32);
}

void set_EDC_CH1_CTL_rot(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CTL_OFFSET;
	EDC_CH1_CTL edc_ch1_ctl;

	edc_ch1_ctl.ul32 = inp32(addr);
	edc_ch1_ctl.bits.ch1_rot = nVal;
	outp32(addr, edc_ch1_ctl.ul32);
}

void set_EDC_CH1_CTL_ch1_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CTL_OFFSET;
	EDC_CH1_CTL edc_ch1_ctl;

	edc_ch1_ctl.ul32 = inp32(addr);
	edc_ch1_ctl.bits.ch1_en = nVal;
	outp32(addr, edc_ch1_ctl.ul32);
}

void set_EDC_CH1_CTL_alp_premulti(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CTL_OFFSET;
	EDC_CH1_CTL edc_ch1_ctl;

	edc_ch1_ctl.ul32 = inp32(addr);
	edc_ch1_ctl.bits.ch1_alp_premulti = nVal;
	outp32(addr, edc_ch1_ctl.ul32);
}

void set_EDC_CH1_COLORK_MIN(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_COLORK_MIN_OFFSET;
	EDC_CH1_COLORK_MIN edc_ch1_colork_min;

	edc_ch1_colork_min.ul32 = inp32(addr);
	edc_ch1_colork_min.bits.ch1_colork_min = nVal;
	outp32(addr, edc_ch1_colork_min.ul32);
}

void set_EDC_CH1_COLORK_MAX(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_COLORK_MAX_OFFSET;
	EDC_CH1_COLORK_MAX edc_ch1_colork_max;

	edc_ch1_colork_max.ul32 = inp32(addr);
	edc_ch1_colork_max.bits.ch1_colork_max = nVal;
	outp32(addr, edc_ch1_colork_max.ul32);
}

void set_EDC_CH2L_ADDR(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2L_ADDR_OFFSET;
	EDC_CH2L_ADDR edc_ch2l_addr;

	edc_ch2l_addr.ul32 = inp32(addr);
	edc_ch2l_addr.bits.ch2l_addr = nVal;
	outp32(addr, edc_ch2l_addr.ul32);
}

void set_EDC_CH2R_ADDR(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2R_ADDR_OFFSET;
	EDC_CH2R_ADDR edc_ch2r_addr;

	edc_ch2r_addr.ul32 = inp32(addr);
	edc_ch2r_addr.bits.ch2r_addr = nVal;
	outp32(addr, edc_ch2r_addr.ul32);
}

void set_EDC_CH2_STRIDE(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_STRIDE_OFFSET;
	EDC_CH2_STRIDE edc_ch2_stride;

	edc_ch2_stride.ul32 = inp32(addr);
	edc_ch2_stride.bits.ch2_ln_stride = nVal;
	outp32(addr, edc_ch2_stride.ul32);
}

void set_EDC_CH2_XY(u32 edc_base, u32 ch2_x, u32 ch2_y)
{
	u32 addr = edc_base + EDC_CH2_XY_OFFSET;
	EDC_CH2_XY edc_ch2_xy;

	edc_ch2_xy.ul32 = inp32(addr);
	edc_ch2_xy.bits.ch2_x = ch2_x;
	edc_ch2_xy.bits.ch2_y = ch2_y;
	outp32(addr, edc_ch2_xy.ul32);
}

void set_EDC_CH2_SIZE(u32 edc_base, u32 ch2_size_hrz, u32 ch2_size_vrt)
{
	u32 addr = edc_base + EDC_CH2_SIZE_OFFSET;
	EDC_CH2_SIZE edc_ch2_size;

	edc_ch2_size.ul32 = inp32(addr);
	edc_ch2_size.bits.ch2_size_hrz = (ch2_size_hrz > 0) ? ch2_size_hrz-1 : 0;
	edc_ch2_size.bits.ch2_size_vrt = (ch2_size_vrt > 0) ? ch2_size_vrt-1 : 0;
	outp32(addr, edc_ch2_size.ul32);
}

void set_EDC_CH2_CTL_secu_line(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CTL_OFFSET;
	EDC_CH2_CTL edc_ch2_ctl;

	edc_ch2_ctl.ul32 = inp32(addr);
	edc_ch2_ctl.bits.ch2_secu_line = (nVal > 0) ? nVal-1 : 0;
	outp32(addr, edc_ch2_ctl.ul32);
}

void set_EDC_CH2_CTL_pix_fmt(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CTL_OFFSET;
	EDC_CH2_CTL edc_ch2_ctl;

	edc_ch2_ctl.ul32 = inp32(addr);
	edc_ch2_ctl.bits.ch2_pix_fmt = nVal;
	outp32(addr, edc_ch2_ctl.ul32);
}

void set_EDC_CH2_CTL_bgr(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CTL_OFFSET;
	EDC_CH2_CTL edc_ch2_ctl;

	edc_ch2_ctl.ul32 = inp32(addr);
	edc_ch2_ctl.bits.ch2_bgr = nVal;
	outp32(addr, edc_ch2_ctl.ul32);
}

void set_EDC_CH2_CTL_colork_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CTL_OFFSET;
	EDC_CH2_CTL edc_ch2_ctl;

	edc_ch2_ctl.ul32 = inp32(addr);
	edc_ch2_ctl.bits.ch2_colork_en = nVal;
	outp32(addr, edc_ch2_ctl.ul32);
}

void set_EDC_CH2_CTL_ch2_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CTL_OFFSET;
	EDC_CH2_CTL edc_ch2_ctl;

	edc_ch2_ctl.ul32 = inp32(addr);
	edc_ch2_ctl.bits.ch2_en = nVal;
	outp32(addr, edc_ch2_ctl.ul32);
}

void set_EDC_CH2_COLORK_MIN(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_COLORK_MIN_OFFSET;
	EDC_CH2_COLORK_MIN edc_ch2_colork_min;

	edc_ch2_colork_min.ul32 = inp32(addr);
	edc_ch2_colork_min.bits.ch2_colork_min = nVal;
	outp32(addr, edc_ch2_colork_min.ul32);
}

void set_EDC_CH2_COLORK_MAX(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_COLORK_MAX_OFFSET;
	EDC_CH2_COLORK_MAX edc_ch2_colork_max;

	edc_ch2_colork_max.ul32 = inp32(addr);
	edc_ch2_colork_max.bits.ch2_colork_max = nVal;
	outp32(addr, edc_ch2_colork_max.ul32);
}

void set_EDC_CH12_OVLY_alp_blend_mskc3_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_OVLY_OFFSET;
	EDC_CH12_OVLY edc_ch12_ovly;

	edc_ch12_ovly.ul32 = inp32(addr);
	edc_ch12_ovly.bits.ch12_mskc3_en = nVal;
	outp32(addr, edc_ch12_ovly.ul32);
}

void set_EDC_CH12_OVLY_alp_blend_mskc2_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_OVLY_OFFSET;
	EDC_CH12_OVLY edc_ch12_ovly;

	edc_ch12_ovly.ul32 = inp32(addr);
	edc_ch12_ovly.bits.ch12_mskc2_en = nVal;
	outp32(addr, edc_ch12_ovly.ul32);
}

void set_EDC_CH12_OVLY_alp_blend_mskc1_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_OVLY_OFFSET;
	EDC_CH12_OVLY edc_ch12_ovly;

	edc_ch12_ovly.ul32 = inp32(addr);
	edc_ch12_ovly.bits.ch12_mskc1_en = nVal;
	outp32(addr, edc_ch12_ovly.ul32);
}

void set_EDC_CH12_OVLY_alp_blend_mskc0_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_OVLY_OFFSET;
	EDC_CH12_OVLY edc_ch12_ovly;

	edc_ch12_ovly.ul32 = inp32(addr);
	edc_ch12_ovly.bits.ch12_mskc0_en = nVal;
	outp32(addr, edc_ch12_ovly.ul32);
}

void set_EDC_CH12_OVLY_alp_blend_mskc_src(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_OVLY_OFFSET;
	EDC_CH12_OVLY edc_ch12_ovly;

	edc_ch12_ovly.ul32 = inp32(addr);
	edc_ch12_ovly.bits.ch12_blend_mskc_src = nVal;
	outp32(addr, edc_ch12_ovly.ul32);
}

void set_EDC_CH12_OVLY_ch2_alp_sel(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_OVLY_OFFSET;
	EDC_CH12_OVLY edc_ch12_ovly;

	edc_ch12_ovly.ul32 = inp32(addr);
	edc_ch12_ovly.bits.ch2_alp_sel = nVal;
	outp32(addr, edc_ch12_ovly.ul32);
}

void set_EDC_CH12_OVLY_ch1_alp_sel(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_OVLY_OFFSET;
	EDC_CH12_OVLY edc_ch12_ovly;

	edc_ch12_ovly.ul32 = inp32(addr);
	edc_ch12_ovly.bits.ch1_alp_sel = nVal;
	outp32(addr, edc_ch12_ovly.ul32);
}

void set_EDC_CH12_OVLY_pix_alp_src(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_OVLY_OFFSET;
	EDC_CH12_OVLY edc_ch12_ovly;

	edc_ch12_ovly.ul32 = inp32(addr);
	edc_ch12_ovly.bits.ch12_pix_alp_src = nVal;
	outp32(addr, edc_ch12_ovly.ul32);
}

void set_EDC_CH12_OVLY_alp_src(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_OVLY_OFFSET;
	EDC_CH12_OVLY edc_ch12_ovly;

	edc_ch12_ovly.ul32 = inp32(addr);
	edc_ch12_ovly.bits.ch12_alp_mode = nVal;
	outp32(addr, edc_ch12_ovly.ul32);
}

void set_EDC_CH12_OVLY_alp_blend_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_OVLY_OFFSET;
	EDC_CH12_OVLY edc_ch12_ovly;

	edc_ch12_ovly.ul32 = inp32(addr);
	edc_ch12_ovly.bits.ch12_blend_en = nVal;
	outp32(addr, edc_ch12_ovly.ul32);
}

void set_EDC_CH12_OVLY_ch2_top(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_OVLY_OFFSET;
	EDC_CH12_OVLY edc_ch12_ovly;

	edc_ch12_ovly.ul32 = inp32(addr);
	edc_ch12_ovly.bits.ch2_top = nVal;
	outp32(addr, edc_ch12_ovly.ul32);
}

void set_EDC_CH12_GLB_ALP_VAL(u32 edc_base,
	u32 ch12_alp_value0, u32 ch12_alp_value1)
{
	u32 addr = edc_base + EDC_CH12_GLB_ALP_VAL_OFFSET;
	EDC_CH12_GLB_ALP_VAL edc_ch12_glb_alp_val;

	edc_ch12_glb_alp_val.ul32 = inp32(addr);
	edc_ch12_glb_alp_val.bits.ch12_alp_value0 = ch12_alp_value0;
	edc_ch12_glb_alp_val.bits.ch12_alp_value1 = ch12_alp_value1;
	outp32(addr, edc_ch12_glb_alp_val.ul32);
}

void set_EDC_CH12_MSK_COLOR0_MIN(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_MSK_COLOR0_MIN_OFFSET;
	EDC_CH12_MSK_COLOR0_MIN edc_ch12_msk_color0_min;

	edc_ch12_msk_color0_min.ul32 = inp32(addr);
	edc_ch12_msk_color0_min.bits.ch12_mskc0_min = nVal;
	outp32(addr, edc_ch12_msk_color0_min.ul32);
}

void set_EDC_CH12_MSK_COLOR0_MAX(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_MSK_COLOR0_MAX_OFFSET;
	EDC_CH12_MSK_COLOR0_MAX edc_ch12_msk_color0_max;

	edc_ch12_msk_color0_max.ul32 = inp32(addr);
	edc_ch12_msk_color0_max.bits.ch12_mskc0_max = nVal;
	outp32(addr, edc_ch12_msk_color0_max.ul32);
}

void set_EDC_CH12_MSK_COLOR1_MIN(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_MSK_COLOR1_MIN_OFFSET;
	EDC_CH12_MSK_COLOR1_MIN edc_ch12_msk_color1_min;

	edc_ch12_msk_color1_min.ul32 = inp32(addr);
	edc_ch12_msk_color1_min.bits.ch12_mskc1_min = nVal;
	outp32(addr, edc_ch12_msk_color1_min.ul32);
}

void set_EDC_CH12_MSK_COLOR1_MAX(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_MSK_COLOR1_MAX_OFFSET;
	EDC_CH12_MSK_COLOR1_MAX edc_ch12_msk_color1_max;

	edc_ch12_msk_color1_max.ul32 = inp32(addr);
	edc_ch12_msk_color1_max.bits.ch12_mskc1_max = nVal;
   outp32(addr, edc_ch12_msk_color1_max.ul32);
}

void set_EDC_CH12_MSK_COLOR2_MIN(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_MSK_COLOR2_MIN_OFFSET;
	EDC_CH12_MSK_COLOR2_MIN edc_ch12_msk_color2_min;

	edc_ch12_msk_color2_min.ul32 = inp32(addr);
	edc_ch12_msk_color2_min.bits.ch12_mskc2_min = nVal;
	outp32(addr, edc_ch12_msk_color2_min.ul32);
}

void set_EDC_CH12_MSK_COLOR2_MAX(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_MSK_COLOR2_MAX_OFFSET;
	EDC_CH12_MSK_COLOR2_MAX edc_ch12_msk_color2_max;

	edc_ch12_msk_color2_max.ul32 = inp32(addr);
	edc_ch12_msk_color2_max.bits.ch12_mskc2_max = nVal;
	outp32(addr, edc_ch12_msk_color2_max.ul32);
}

void set_EDC_CH12_MSK_COLOR3_MIN(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_MSK_COLOR3_MIN_OFFSET;
	EDC_CH12_MSK_COLOR3_MIN edc_ch12_msk_color3_min;

	edc_ch12_msk_color3_min.ul32 = inp32(addr);
	edc_ch12_msk_color3_min.bits.ch12_mskc3_min = nVal;
	outp32(addr, edc_ch12_msk_color3_min.ul32);
}

void set_EDC_CH12_MSK_COLOR3_MAX(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH12_MSK_COLOR3_MAX_OFFSET;
	EDC_CH12_MSK_COLOR3_MAX edc_ch12_msk_color3_max;

	edc_ch12_msk_color3_max.ul32 = inp32(addr);
	edc_ch12_msk_color3_max.bits.ch12_mskc3_max = nVal;
	outp32(addr, edc_ch12_msk_color3_max.ul32);
}

void set_EDC_CRSL_ADDR(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRSL_ADDR_OFFSET;
	EDC_CRSL_ADDR edc_crsl_addr;

	edc_crsl_addr.ul32 = inp32(addr);
	edc_crsl_addr.bits.crsl_addr = nVal;
	outp32(addr, edc_crsl_addr.ul32);
}

void set_EDC_CRSR_ADDR(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRSR_ADDR_OFFSET;
	EDC_CRSR_ADDR edc_crsr_addr;

	edc_crsr_addr.ul32 = inp32(addr);
	edc_crsr_addr.bits.crsr_addr = nVal;
	outp32(addr, edc_crsr_addr.ul32);
}

void set_EDC_CRS_STRIDE(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_STRIDE_OFFSET;
	EDC_CRS_STRIDE edc_crs_stride;

	edc_crs_stride.ul32 = inp32(addr);
	edc_crs_stride.bits.crs_ln_stride = nVal;
	outp32(addr, edc_crs_stride.ul32);
}

void set_EDC_CRS_XY(u32 edc_base, u32 crs_x, u32 crs_y)
{
	u32 addr = edc_base + EDC_CRS_XY_OFFSET;
	EDC_CRS_XY edc_crs_xy;

	edc_crs_xy.ul32 = inp32(addr);
	edc_crs_xy.bits.crs_x = crs_x;
	edc_crs_xy.bits.crs_y = crs_y;
	outp32(addr, edc_crs_xy.ul32);
}

void set_EDC_CRS_SIZE(u32 edc_base, u32 crs_size_hrz, u32 crs_size_vrt)
{
	u32 addr = edc_base + EDC_CRS_SIZE_OFFSET;
	EDC_CRS_SIZE edc_crs_size;

	edc_crs_size.ul32 = inp32(addr);
	edc_crs_size.bits.crs_size_hrz = (crs_size_hrz > 0) ? crs_size_hrz-1 : 0;
	edc_crs_size.bits.crs_size_vrt = (crs_size_vrt > 0) ? crs_size_vrt-1 : 0;
	outp32(addr, edc_crs_size.ul32);
}

void set_EDC_CRS_CTL_secu_line(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_CTL_OFFSET;
	EDC_CRS_CTL edc_crs_ctl;

	edc_crs_ctl.ul32 = inp32(addr);
	edc_crs_ctl.bits.crs_secu_line = (nVal > 0) ? nVal-1 : 0;
	outp32(addr, edc_crs_ctl.ul32);
}

void set_EDC_CRS_CTL_alp_sel(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_CTL_OFFSET;
	EDC_CRS_CTL edc_crs_ctl;

	edc_crs_ctl.ul32 = inp32(addr);
	edc_crs_ctl.bits.crs_alp_sel = nVal;
	outp32(addr, edc_crs_ctl.ul32);
}

void set_EDC_CRS_CTL_under_alp_sel(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_CTL_OFFSET;
	EDC_CRS_CTL edc_crs_ctl;

	edc_crs_ctl.ul32 = inp32(addr);
	edc_crs_ctl.bits.under_alp_sel = nVal;
	outp32(addr, edc_crs_ctl.ul32);
}

void set_EDC_CRS_CTL_pix_fmt(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_CTL_OFFSET;
	EDC_CRS_CTL edc_crs_ctl;

	edc_crs_ctl.ul32 = inp32(addr);
	edc_crs_ctl.bits.crs_pix_fmt = nVal;
	outp32(addr, edc_crs_ctl.ul32);
}

void set_EDC_CRS_CTL_bgr(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_CTL_OFFSET;
	EDC_CRS_CTL edc_crs_ctl;

	edc_crs_ctl.ul32 = inp32(addr);
	edc_crs_ctl.bits.crs_bgr = nVal;
	outp32(addr, edc_crs_ctl.ul32);
}

void set_EDC_CRS_CTL_colork_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_CTL_OFFSET;
	EDC_CRS_CTL edc_crs_ctl;

	edc_crs_ctl.ul32 = inp32(addr);
	edc_crs_ctl.bits.crs_colork_en = nVal;
	outp32(addr, edc_crs_ctl.ul32);
}

void set_EDC_CRS_CTL_alp_src(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_CTL_OFFSET;
	EDC_CRS_CTL edc_crs_ctl;

	edc_crs_ctl.ul32 = inp32(addr);
	edc_crs_ctl.bits.crs_alp_mode = nVal;
	outp32(addr, edc_crs_ctl.ul32);
}

void set_EDC_CRS_CTL_alp_blend_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_CTL_OFFSET;
	EDC_CRS_CTL edc_crs_ctl;

	edc_crs_ctl.ul32 = inp32(addr);
	edc_crs_ctl.bits.crs_blend_en = nVal;
	outp32(addr, edc_crs_ctl.ul32);
}

void set_EDC_CRS_CTL_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_CTL_OFFSET;
	EDC_CRS_CTL edc_crs_ctl;

	edc_crs_ctl.ul32 = inp32(addr);
	edc_crs_ctl.bits.crs_en = nVal;
	outp32(addr, edc_crs_ctl.ul32);
}

void set_EDC_CRS_COLORK_MIN(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_COLORK_MIN_OFFSET;
	EDC_CRS_COLORK_MIN edc_crs_colork_min;

	edc_crs_colork_min.ul32 = inp32(addr);
	edc_crs_colork_min.bits.crs_colork_min = nVal;
	outp32(addr, edc_crs_colork_min.ul32);
}

void set_EDC_CRS_COLORK_MAX(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CRS_COLORK_MAX_OFFSET;
	EDC_CRS_COLORK_MAX edc_crs_colork_max;

	edc_crs_colork_max.ul32 = inp32(addr);
	edc_crs_colork_max.bits.crs_colork_max = nVal;
	outp32(addr, edc_crs_colork_max.ul32);
}

void set_EDC_CRS_GLB_ALP_VAL(u32 edc_base,
	u32 crs_alp_value0, u32 crs_alp_value1)
{
	u32 addr = edc_base + EDC_CRS_GLB_ALP_VAL_OFFSET;
	EDC_CRS_GLB_ALP_VAL edc_crs_glb_alp_val;

	edc_crs_glb_alp_val.ul32 = inp32(addr);
	edc_crs_glb_alp_val.bits.crs_alp_value0 = crs_alp_value0;
	edc_crs_glb_alp_val.bits.crs_alp_value1 = crs_alp_value1;
	outp32(addr, edc_crs_glb_alp_val.ul32);
}

void set_EDC_DISP_SIZE(u32 edc_base, u32 disp_size_hrz, u32 disp_size_vrt)
{
	u32 addr = edc_base + EDC_DISP_SIZE_OFFSET;
	EDC_DISP_SIZE edc_disp_size;

	edc_disp_size.ul32 = inp32(addr);
	edc_disp_size.bits.disp_size_hrz = (disp_size_hrz > 0) ? disp_size_hrz-1 : 0;
	edc_disp_size.bits.disp_size_vrt = (disp_size_vrt > 0) ? disp_size_vrt-1 : 0;
	outp32(addr, edc_disp_size.ul32);
}

void set_EDC_DISP_CTL_cfg_ok(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.edc_cfg_ok = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_cfg_ok_sel(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.edc_cfg_ok_sel = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_d3d_rf(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.d3d_right_first = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_frm_fmt(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.disp_frm_fmt = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_pix_fmt(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.disp_pix_fmt = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_dither_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.dither_en = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_endian(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.disp_endian = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_gamma_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.gamma_en = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_ch_chg(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.ch_chg = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_nrot_burst(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.nrot_burst16 = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_unflow_lev(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.unflow_lev = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_outstding_dep(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.outstding_dep = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_frm_end_start(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.frm_end_start = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_crg_gt_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.crg_gt_en = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_CTL_edc_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_CTL_OFFSET;
	EDC_DISP_CTL edc_disp_ctl;

	edc_disp_ctl.ul32 = inp32(addr);
	edc_disp_ctl.bits.edc_en = nVal;
	outp32(addr, edc_disp_ctl.ul32);
}

void set_EDC_DISP_DPD_disp_dpd(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_DPD_OFFSET;
	EDC_DISP_DPD edc_disp_dpd;

	edc_disp_dpd.ul32 = inp32(addr);
	edc_disp_dpd.bits.disp_dpd = nVal;
	outp32(addr, edc_disp_dpd.ul32);
}

void set_EDC_DISP_DPD_sbl_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_DISP_DPD_OFFSET;
	EDC_DISP_DPD edc_disp_dpd;

	edc_disp_dpd.ul32 = inp32(addr);
	edc_disp_dpd.bits.sbl_en = nVal;
	outp32(addr, edc_disp_dpd.ul32);
}

void set_EDC_INTE(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_INTE_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_INTS(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_INTS_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH1_CSCIDC_csc_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CSCIDC_OFFSET;
	EDC_CH1_CSCIDC edc_ch1_cscidc;

	edc_ch1_cscidc.ul32 = inp32(addr);
	edc_ch1_cscidc.bits.ch1_csc_en = nVal;
	outp32(addr, edc_ch1_cscidc.ul32);
}

void set_EDC_CH1_CSCIDC(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CSCIDC_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH1_CSCODC(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CSCODC_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH1_CSCP0(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CSCP0_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH1_CSCP1(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CSCP1_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH1_CSCP2(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CSCP2_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH1_CSCP3(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CSCP3_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH1_CSCP4(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CSCP4_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH2_CSCIDC_csc_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CSCIDC_OFFSET;
	EDC_CH2_CSCIDC edc_ch2_cscidc;

	edc_ch2_cscidc.ul32 = inp32(addr);
	edc_ch2_cscidc.bits.ch2_csc_en = nVal;
	outp32(addr, edc_ch2_cscidc.ul32);
}

void set_EDC_CH2_CSCIDC(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CSCIDC_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH2_CSCODC(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CSCODC_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH2_CSCP0(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CSCP0_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH2_CSCP1(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CSCP1_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH2_CSCP2(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CSCP2_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH2_CSCP3(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CSCP3_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH2_CSCP4(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CSCP4_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_OUT_CSC_csc_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_OUT_CSCIDC_OFFSET;
	EDC_OUT_CSCIDC edc_out_cscidc;

	edc_out_cscidc.ul32 = inp32(addr);
	edc_out_cscidc.bits.out_csc_en = nVal;
	outp32(addr, edc_out_cscidc.ul32);
}

void set_EDC_OUT_CSCIDC(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_OUT_CSCIDC_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_OUT_CSCODC(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_OUT_CSCODC_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_OUT_CSCP0(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_OUT_CSCP0_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_OUT_CSCP1(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_OUT_CSCP1_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_OUT_CSCP2(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_OUT_CSCP2_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_OUT_CSCP3(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_OUT_CSCP3_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_OUT_CSCP4(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_OUT_CSCP4_OFFSET;
	outp32(addr, nVal);
}

void set_EDC_CH1_SCL_HSP_hratio(u32 edc_base, u32 nVal)
{

	u32 addr = edc_base + EDC_CH1_SCL_HSP_OFFSET;
	EDC_CH1_SCL_HSP edc_ch1_scl_hsp;

	edc_ch1_scl_hsp.ul32 = inp32(addr);
	edc_ch1_scl_hsp.bits.hratio = nVal;
	outp32(addr, edc_ch1_scl_hsp.ul32);
}

void set_EDC_CH1_SCL_HSP_hfir_order(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_HSP_OFFSET;
	EDC_CH1_SCL_HSP edc_ch1_scl_hsp;

	edc_ch1_scl_hsp.ul32 = inp32(addr);
	edc_ch1_scl_hsp.bits.hfir_order = nVal;
	outp32(addr, edc_ch1_scl_hsp.ul32);
}

void set_EDC_CH1_SCL_HSP_hafir_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_HSP_OFFSET;
	EDC_CH1_SCL_HSP edc_ch1_scl_hsp;

	edc_ch1_scl_hsp.ul32 = inp32(addr);
	edc_ch1_scl_hsp.bits.hafir_en = nVal;
	outp32(addr, edc_ch1_scl_hsp.ul32);
}

void set_EDC_CH1_SCL_HSP_hfir_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_HSP_OFFSET;
	EDC_CH1_SCL_HSP edc_ch1_scl_hsp;

	edc_ch1_scl_hsp.ul32 = inp32(addr);
	edc_ch1_scl_hsp.bits.hfir_en = nVal;
	outp32(addr, edc_ch1_scl_hsp.ul32);
}

void set_EDC_CH1_SCL_HSP_hchmid_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_HSP_OFFSET;
	EDC_CH1_SCL_HSP edc_ch1_scl_hsp;

	edc_ch1_scl_hsp.ul32 = inp32(addr);
	edc_ch1_scl_hsp.bits.hchmid_en = nVal;
	outp32(addr, edc_ch1_scl_hsp.ul32);
}

void set_EDC_CH1_SCL_HSP_hlmid_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_HSP_OFFSET;
	EDC_CH1_SCL_HSP edc_ch1_scl_hsp;

	edc_ch1_scl_hsp.ul32 = inp32(addr);
	edc_ch1_scl_hsp.bits.hlmid_en = nVal;
	outp32(addr, edc_ch1_scl_hsp.ul32);
}

void set_EDC_CH1_SCL_HSP_hamid_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_HSP_OFFSET;
	EDC_CH1_SCL_HSP edc_ch1_scl_hsp;

	edc_ch1_scl_hsp.ul32 = inp32(addr);
	edc_ch1_scl_hsp.bits.hamid_en = nVal;
	outp32(addr, edc_ch1_scl_hsp.ul32);
}

void set_EDC_CH1_SCL_HSP_hsc_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_HSP_OFFSET;
	EDC_CH1_SCL_HSP edc_ch1_scl_hsp;

	edc_ch1_scl_hsp.ul32 = inp32(addr);
	edc_ch1_scl_hsp.bits.hsc_en = nVal;
	outp32(addr, edc_ch1_scl_hsp.ul32);
}

void set_EDC_CH1_SCL_VSP_vafir_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_VSP_OFFSET;
	EDC_CH1_SCL_VSP edc_ch1_scl_vsp;

	edc_ch1_scl_vsp.ul32 = inp32(addr);
	edc_ch1_scl_vsp.bits.vafir_en = nVal;
	outp32(addr, edc_ch1_scl_vsp.ul32);
}

void set_EDC_CH1_SCL_VSP_vfir_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_VSP_OFFSET;
	EDC_CH1_SCL_VSP edc_ch1_scl_vsp;

	edc_ch1_scl_vsp.ul32 = inp32(addr);
	edc_ch1_scl_vsp.bits.vfir_en = nVal;
	outp32(addr, edc_ch1_scl_vsp.ul32);
}

void set_EDC_CH1_SCL_VSP_vsc_luma_tap(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_VSP_OFFSET;
	EDC_CH1_SCL_VSP edc_ch1_scl_vsp;

	edc_ch1_scl_vsp.ul32 = inp32(addr);
	edc_ch1_scl_vsp.bits.vsc_luma_tap = nVal;
	outp32(addr, edc_ch1_scl_vsp.ul32);
}

void set_EDC_CH1_SCL_VSP_vchmid_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_VSP_OFFSET;
	EDC_CH1_SCL_VSP edc_ch1_scl_vsp;

	edc_ch1_scl_vsp.ul32 = inp32(addr);
	edc_ch1_scl_vsp.bits.vchmid_en = nVal;
	outp32(addr, edc_ch1_scl_vsp.ul32);
}

void set_EDC_CH1_SCL_VSP_vlmid_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_VSP_OFFSET;
	EDC_CH1_SCL_VSP edc_ch1_scl_vsp;

	edc_ch1_scl_vsp.ul32 = inp32(addr);
	edc_ch1_scl_vsp.bits.vlmid_en = nVal;
	outp32(addr, edc_ch1_scl_vsp.ul32);
}

void set_EDC_CH1_SCL_VSP_vamid_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_VSP_OFFSET;
	EDC_CH1_SCL_VSP edc_ch1_scl_vsp;

	edc_ch1_scl_vsp.ul32 = inp32(addr);
	edc_ch1_scl_vsp.bits.vamid_en = nVal;
	outp32(addr, edc_ch1_scl_vsp.ul32);
}

void set_EDC_CH1_SCL_VSP_vsc_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_VSP_OFFSET;
	EDC_CH1_SCL_VSP edc_ch1_scl_vsp;

	edc_ch1_scl_vsp.ul32 = inp32(addr);
	edc_ch1_scl_vsp.bits.vsc_en = nVal;
	outp32(addr, edc_ch1_scl_vsp.ul32);
}

void set_EDC_CH1_SCL_VSR_vratio(u32 edc_base, u32 vratio)
{
	u32 addr = edc_base + EDC_CH1_SCL_VSR_OFFSET;
	EDC_CH1_SCL_VSR edc_ch1_scl_vsr;

	edc_ch1_scl_vsr.ul32 = inp32(addr);
	edc_ch1_scl_vsr.bits.vratio = vratio;
	outp32(addr, edc_ch1_scl_vsr.ul32);
}

void set_EDC_CH1_SCL_HOFFSET(u32 edc_base, u32 hor_coffset , u32 hor_loffset)
{
	u32 addr = edc_base + EDC_CH1_SCL_HOFFSET_OFFSET;
	EDC_CH1_SCL_HOFFSET edc_ch1_scl_hoffset;

	edc_ch1_scl_hoffset.ul32 = inp32(addr);
	edc_ch1_scl_hoffset.bits.hor_coffset = hor_coffset;
	edc_ch1_scl_hoffset.bits.hor_loffset = hor_loffset;
	outp32(addr, edc_ch1_scl_hoffset.ul32);
}

void set_EDC_CH1_SCL_VOFFSET(u32 edc_base, u32 vbtm_offset, u32 vtp_offset)
{
	u32 addr = edc_base + EDC_CH1_SCL_VOFFSET_OFFSET;
	EDC_CH1_SCL_VOFFSET edc_ch1_scl_voffset;

	edc_ch1_scl_voffset.ul32 = inp32(addr);
	edc_ch1_scl_voffset.bits.vbtm_offset = vbtm_offset;
	edc_ch1_scl_voffset.bits.vtp_offset = vtp_offset;
	outp32(addr, edc_ch1_scl_voffset.ul32);
}

void set_EDC_CH1_SCL_ORES(u32 edc_base, u32 ow, u32 oh)
{
	u32 addr = edc_base + EDC_CH1_SCL_ORES_OFFSET;
	EDC_CH1_SCL_ORES edc_ch1_scl_ores;

	edc_ch1_scl_ores.ul32 = inp32(addr);
	edc_ch1_scl_ores.bits.ow = (ow > 0) ? ow-1 : 0;
	edc_ch1_scl_ores.bits.oh = (oh > 0) ? oh-1 : 0;
	outp32(addr, edc_ch1_scl_ores.ul32);
}

void set_EDC_CH1_SCL_IRES(u32 edc_base, u32 iw, u32 ih)
{
	u32 addr = edc_base + EDC_CH1_SCL_IRES_OFFSET;
	EDC_CH1_SCL_IRES edc_ch1_scl_ires;

	edc_ch1_scl_ires.ul32 = inp32(addr);
	edc_ch1_scl_ires.bits.iw = (iw > 0) ? iw-1 : 0;
	edc_ch1_scl_ires.bits.ih = (ih > 0) ? ih-1 : 0;
	outp32(addr, edc_ch1_scl_ires.ul32);
}

void set_EDC_CH1_SCL_HPC(u32 edc_base, u32 i, u32 j, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_HPC_OFFSET + i * 0x10 + j * 0x4;
	outp32(addr, nVal);
}

void set_EDC_CH1_SCL_VPC(u32 edc_base, u32 i, u32 j, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_SCL_VPC_OFFSET + i * 0x8 + j * 0x4;
	outp32(addr, nVal);
}

#if defined(CONFIG_OVERLAY_COMPOSE)
void set_OVC_CH1_CSCIDC_csc_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH1_CSCIDC_OFFSET;
	EDC_CH1_CSCIDC edc_ch1_cscidc;

	edc_ch1_cscidc.ul32 = inp32(addr);
	edc_ch1_cscidc.bits.ch1_csc_en = nVal;
	outp32(addr, edc_ch1_cscidc.ul32);
}

void set_OVC_CH1_CSCIDC(u32 edc_base, u32 cscidc_y, u32 cscidc_u, u32 cscidc_v)
{
	u32 addr = edc_base + EDC_CH1_CSCIDC_OFFSET;

	EDC_CH1_CSCIDC edc_ch1_cscidc;
	edc_ch1_cscidc.ul32 = inp32(addr);
	edc_ch1_cscidc.bits.ch1_cscidc_y = cscidc_y;
	edc_ch1_cscidc.bits.ch1_cscidc_u = cscidc_u;
	edc_ch1_cscidc.bits.ch1_cscidc_v = cscidc_v;
	outp32(addr, edc_ch1_cscidc.ul32);
}

void set_OVC_CH1_CSCODC(u32 edc_base, u32 cscodc_r, u32 cscodc_g, u32 cscodc_b)
{
	u32 addr = edc_base + EDC_CH1_CSCODC_OFFSET;
	EDC_CH1_CSCODC edc_ch1_cscodc;
	edc_ch1_cscodc.ul32 = inp32(addr);
	edc_ch1_cscodc.bits.ch1_cscodc_r = cscodc_r;
	edc_ch1_cscodc.bits.ch1_cscodc_g = cscodc_g;
	edc_ch1_cscodc.bits.ch1_cscodc_b = cscodc_b;
	outp32(addr, edc_ch1_cscodc.ul32);
}

void set_OVC_CH1_CSCP0(u32 edc_base, u32 csc_01p, u32 csc_00p)
{
	u32 addr = edc_base + EDC_CH1_CSCP0_OFFSET;
	EDC_CH1_CSCP0 edc_ch1_cscp0;
	edc_ch1_cscp0.ul32 = inp32(addr);
	edc_ch1_cscp0.bits.ch1_csc_01p = csc_01p;
	edc_ch1_cscp0.bits.ch1_csc_00p = csc_00p;
	outp32(addr, edc_ch1_cscp0.ul32);
}

void set_OVC_CH1_CSCP1(u32 edc_base, u32 csc_10p, u32 csc_02p)
{
	u32 addr = edc_base + EDC_CH1_CSCP1_OFFSET;
	EDC_CH1_CSCP1 edc_ch1_cscp1;
	edc_ch1_cscp1.ul32 = inp32(addr);
	edc_ch1_cscp1.bits.ch1_csc_10p = csc_10p;
	edc_ch1_cscp1.bits.ch1_csc_02p = csc_02p;
	outp32(addr, edc_ch1_cscp1.ul32);
}

void set_OVC_CH1_CSCP2(u32 edc_base, u32 csc_12p, u32 csc_11p)
{
	u32 addr = edc_base + EDC_CH1_CSCP2_OFFSET;

	EDC_CH1_CSCP2 edc_ch1_cscp2;
	edc_ch1_cscp2.ul32 = inp32(addr);
	edc_ch1_cscp2.bits.ch1_csc_12p = csc_12p;
	edc_ch1_cscp2.bits.ch1_csc_11p = csc_11p;
	outp32(addr, edc_ch1_cscp2.ul32);
}

void set_OVC_CH1_CSCP3(u32 edc_base, u32 csc_21p, u32 csc_20p)
{
	u32 addr = edc_base + EDC_CH1_CSCP3_OFFSET;

	EDC_CH1_CSCP3 edc_ch1_cscp3;
	edc_ch1_cscp3.ul32 = inp32(addr);
	edc_ch1_cscp3.bits.ch1_csc_21p = csc_21p;
	edc_ch1_cscp3.bits.ch1_csc_20p = csc_20p;
	outp32(addr, edc_ch1_cscp3.ul32);
}

void set_OVC_CH1_CSCP4(u32 edc_base, u32 csc_22p)
{
	u32 addr = edc_base + EDC_CH1_CSCP4_OFFSET;

	EDC_CH1_CSCP4 edc_ch1_cscp4;
	edc_ch1_cscp4.ul32 = inp32(addr);
	edc_ch1_cscp4.bits.ch1_csc_22p = csc_22p;
	outp32(addr, edc_ch1_cscp4.ul32);
}

void set_OVC_CH2_CSCIDC_csc_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + EDC_CH2_CSCIDC_OFFSET;
	EDC_CH2_CSCIDC edc_ch2_cscidc;

	edc_ch2_cscidc.ul32 = inp32(addr);
	edc_ch2_cscidc.bits.ch2_csc_en = nVal;
	outp32(addr, edc_ch2_cscidc.ul32);
}

void set_OVC_CH2_CSCIDC(u32 edc_base, u32 cscidc_y, u32 cscidc_u, u32 cscidc_v)
{
	u32 addr = edc_base + EDC_CH2_CSCIDC_OFFSET;

	EDC_CH2_CSCIDC edc_ch2_cscidc;
	edc_ch2_cscidc.ul32 = inp32(addr);
	edc_ch2_cscidc.bits.ch2_cscidc_y = cscidc_y;
	edc_ch2_cscidc.bits.ch2_cscidc_u = cscidc_u;
	edc_ch2_cscidc.bits.ch2_cscidc_v = cscidc_v;
	outp32(addr, edc_ch2_cscidc.ul32);
}

void set_OVC_CH2_CSCODC(u32 edc_base, u32 cscodc_r, u32 cscodc_g, u32 cscodc_b)
{
	u32 addr = edc_base + EDC_CH2_CSCODC_OFFSET;
	EDC_CH2_CSCODC edc_ch2_cscodc;
	edc_ch2_cscodc.ul32 = inp32(addr);
	edc_ch2_cscodc.bits.ch2_cscodc_r = cscodc_r;
	edc_ch2_cscodc.bits.ch2_cscodc_g = cscodc_g;
	edc_ch2_cscodc.bits.ch2_cscodc_b = cscodc_b;
	outp32(addr, edc_ch2_cscodc.ul32);
}

void set_OVC_CH2_CSCP0(u32 edc_base, u32 csc_01p, u32 csc_00p)
{
	u32 addr = edc_base + EDC_CH2_CSCP0_OFFSET;

	EDC_CH2_CSCP0 edc_ch2_cscp0;
	edc_ch2_cscp0.ul32 = inp32(addr);
	edc_ch2_cscp0.bits.ch2_csc_01p = csc_01p;
	edc_ch2_cscp0.bits.ch2_csc_00p = csc_00p;
	outp32(addr, edc_ch2_cscp0.ul32);
}

void set_OVC_CH2_CSCP1(u32 edc_base, u32 csc_10p, u32 csc_02p)
{
	u32 addr = edc_base + EDC_CH2_CSCP1_OFFSET;

	EDC_CH2_CSCP1 edc_ch2_cscp1;
	edc_ch2_cscp1.ul32 = inp32(addr);
	edc_ch2_cscp1.bits.ch2_csc_10p = csc_10p;
	edc_ch2_cscp1.bits.ch2_csc_02p = csc_02p;
	outp32(addr, edc_ch2_cscp1.ul32);
}

void set_OVC_CH2_CSCP2(u32 edc_base, u32 csc_12p, u32 csc_11p)
{
	u32 addr = edc_base + EDC_CH2_CSCP2_OFFSET;

	EDC_CH2_CSCP2 edc_ch2_cscp2;
	edc_ch2_cscp2.ul32 = inp32(addr);
	edc_ch2_cscp2.bits.ch2_csc_12p = csc_12p;
	edc_ch2_cscp2.bits.ch2_csc_11p = csc_11p;
	outp32(addr, edc_ch2_cscp2.ul32);
}

void set_OVC_CH2_CSCP3(u32 edc_base, u32 csc_21p, u32 csc_20p)
{
	u32 addr = edc_base + EDC_CH2_CSCP3_OFFSET;

	EDC_CH2_CSCP3 edc_ch2_cscp3;
	edc_ch2_cscp3.ul32 = inp32(addr);
	edc_ch2_cscp3.bits.ch2_csc_21p = csc_21p;
	edc_ch2_cscp3.bits.ch2_csc_20p = csc_20p;
	outp32(addr, edc_ch2_cscp3.ul32);
}

void set_OVC_CH2_CSCP4(u32 edc_base, u32 csc_22p)
{
	u32 addr = edc_base + EDC_CH2_CSCP4_OFFSET;
	EDC_CH2_CSCP4 edc_ch2_cscp4;
	edc_ch2_cscp4.ul32 = inp32(addr);
	edc_ch2_cscp4.bits.ch2_csc_22p = csc_22p;
	outp32(addr, edc_ch2_cscp4.ul32);
}
#endif

void dump_EDC_Reg(u32 edc_base, u32 size)
{
	int i = 0;

	printk("\nedc reg sizeof: %d", size);
	for (i = 0; i < size; i++) {
		if (i % 4 == 0)
			printk("\n");
		printk("0x%.8x ",  inp32(edc_base + i*0x4));
	}
}
