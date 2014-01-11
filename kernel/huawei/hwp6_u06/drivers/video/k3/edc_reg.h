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

#ifndef EDC_REG_H
#define EDC_REG_H

#include "k3_edc.h"
#include "k3_fb_def.h"


#define EDC_ID_OFFSET				(0x0)
#define EDC_CH1L_ADDR_OFFSET		(0x4)
#define EDC_CH1R_ADDR_OFFSET		(0x8)
#define EDC_CH1_STRIDE_OFFSET		(0xC)
#define EDC_CH1_XY_OFFSET			(0x10)
#define EDC_CH1_SIZE_OFFSET			(0x14)
#define EDC_CH1_CTL_OFFSET			(0x18)
#define EDC_CH1_COLORK_MIN_OFFSET	(0x1C)
#define EDC_CH1_COLORK_MAX_OFFSET	(0x20)
#define EDC_CH2L_ADDR_OFFSET		(0x24)
#define EDC_CH2R_ADDR_OFFSET		(0x28)
#define EDC_CH2_STRIDE_OFFSET		(0x2C)
#define EDC_CH2_XY_OFFSET			(0x30)
#define EDC_CH2_SIZE_OFFSET			(0x34)
#define EDC_CH2_CTL_OFFSET			(0x38)
#define EDC_CH2_COLORK_MIN_OFFSET	(0x3C)
#define EDC_CH2_COLORK_MAX_OFFSET	(0x40)
#define EDC_CH12_OVLY_OFFSET		(0x44)
#define EDC_CH12_GLB_ALP_VAL_OFFSET	(0x48)
#define EDC_CH12_MSK_COLOR0_MIN_OFFSET	(0x4C)
#define EDC_CH12_MSK_COLOR0_MAX_OFFSET	(0x50)
#define EDC_CH12_MSK_COLOR1_MIN_OFFSET	(0x54)
#define EDC_CH12_MSK_COLOR1_MAX_OFFSET	(0x58)
#define EDC_CH12_MSK_COLOR2_MIN_OFFSET	(0x5C)
#define EDC_CH12_MSK_COLOR2_MAX_OFFSET	(0x60)
#define EDC_CH12_MSK_COLOR3_MIN_OFFSET	(0x64)
#define EDC_CH12_MSK_COLOR3_MAX_OFFSET	(0x68)
#define EDC_CRSL_ADDR_OFFSET		(0x6C)
#define EDC_CRSR_ADDR_OFFSET		(0x70)
#define EDC_CRS_STRIDE_OFFSET		(0x74)
#define EDC_CRS_XY_OFFSET			(0x78)
#define EDC_CRS_SIZE_OFFSET			(0x7C)
#define EDC_CRS_CTL_OFFSET			(0x80)
#define EDC_CRS_COLORK_MIN_OFFSET	(0x84)
#define EDC_CRS_COLORK_MAX_OFFSET	(0x88)
#define EDC_CRS_GLB_ALP_VAL_OFFSET	(0x8C)
#define EDC_DISP_SIZE_OFFSET		(0x90)
#define EDC_DISP_CTL_OFFSET			(0x94)
#define EDC_DISP_DPD_OFFSET			(0x98)
#define EDC_STS_OFFSET				(0x9C)
#define EDC_INTS_OFFSET				(0xA0)
#define EDC_INTE_OFFSET				(0xA4)
#define EDC_GAMCNT_CLR_OFFSET		(0xA8)
#define EDC_TILE_SIZE_VRT_OFFSET	(0xAC)
#define EDC_TILE_CROP_OFFSET		(0xB0)
#define EDC_TL_ENGIN0_ADDR_OFFSET	(0xB4)
#define EDC_TL_ENGIN1_ADDR_OFFSET	(0xB8)
#define EDC_TR_ENGIN0_ADDR_OFFSET	(0xBC)
#define EDC_TR_ENGIN1_ADDR_OFFSET	(0xC0)

#define EDC_GAMMA_OFFSET			(0x100)

#define EDC_CH1_CSC_OFFSET			(0x200)
#define EDC_CH1_CSCIDC_OFFSET		(EDC_CH1_CSC_OFFSET + 0x0)
#define EDC_CH1_CSCODC_OFFSET		(EDC_CH1_CSC_OFFSET + 0x4)
#define EDC_CH1_CSCP0_OFFSET		(EDC_CH1_CSC_OFFSET + 0x8)
#define EDC_CH1_CSCP1_OFFSET		(EDC_CH1_CSC_OFFSET + 0xC)
#define EDC_CH1_CSCP2_OFFSET		(EDC_CH1_CSC_OFFSET + 0x10)
#define EDC_CH1_CSCP3_OFFSET		(EDC_CH1_CSC_OFFSET + 0x14)
#define EDC_CH1_CSCP4_OFFSET		(EDC_CH1_CSC_OFFSET + 0x18)

#define EDC_CH2_CSC_OFFSET			(0x300)
#define EDC_CH2_CSCIDC_OFFSET		(EDC_CH2_CSC_OFFSET + 0x0)
#define EDC_CH2_CSCODC_OFFSET		(EDC_CH2_CSC_OFFSET + 0x4)
#define EDC_CH2_CSCP0_OFFSET		(EDC_CH2_CSC_OFFSET + 0x8)
#define EDC_CH2_CSCP1_OFFSET		(EDC_CH2_CSC_OFFSET + 0xC)
#define EDC_CH2_CSCP2_OFFSET		(EDC_CH2_CSC_OFFSET + 0x10)
#define EDC_CH2_CSCP3_OFFSET		(EDC_CH2_CSC_OFFSET + 0x14)
#define EDC_CH2_CSCP4_OFFSET		(EDC_CH2_CSC_OFFSET + 0x18)

#define EDC_OUT_CSC_OFFSET			(0x400)
#define EDC_OUT_CSCIDC_OFFSET		(EDC_OUT_CSC_OFFSET + 0x0)
#define EDC_OUT_CSCODC_OFFSET		(EDC_OUT_CSC_OFFSET + 0x4)
#define EDC_OUT_CSCP0_OFFSET		(EDC_OUT_CSC_OFFSET + 0x8)
#define EDC_OUT_CSCP1_OFFSET		(EDC_OUT_CSC_OFFSET + 0xC)
#define EDC_OUT_CSCP2_OFFSET		(EDC_OUT_CSC_OFFSET + 0x10)
#define EDC_OUT_CSCP3_OFFSET		(EDC_OUT_CSC_OFFSET + 0x14)
#define EDC_OUT_CSCP4_OFFSET		(EDC_OUT_CSC_OFFSET + 0x18)

#define EDC_SCL_OFFSET				(0x500)
#define EDC_CH1_SCL_HSP_OFFSET		(EDC_SCL_OFFSET + 0x0)
#define EDC_CH1_SCL_HOFFSET_OFFSET	(EDC_SCL_OFFSET + 0x4)
#define EDC_CH1_SCL_VSP_OFFSET		(EDC_SCL_OFFSET + 0x8)
#define EDC_CH1_SCL_VSR_OFFSET		(EDC_SCL_OFFSET + 0xC)
#define EDC_CH1_SCL_VOFFSET_OFFSET	(EDC_SCL_OFFSET + 0x10)
#define EDC_CH1_SCL_ORES_OFFSET		(EDC_SCL_OFFSET + 0x14)
#define EDC_CH1_SCL_IRES_OFFSET		(EDC_SCL_OFFSET + 0x18)

#define EDC_CH1_SCL_HPC_OFFSET		(0x600)
#define EDC_CH1_SCL_VPC_OFFSET		(0x700)


/******************************************************************************
** FUNCTIONS PROTOTYPES
*/

void set_EDC_CH1L_ADDR(u32 edc_base, u32 nVal);
void set_EDC_CH1R_ADDR(u32 edc_base, u32 nVal);
void set_EDC_CH1_STRIDE(u32 edc_base, u32 nVal);
void set_EDC_CH1_XY(u32 edc_base, u32 ch1_x, u32 ch1_y);
void set_EDC_CH1_SIZE(u32 edc_base, u32 ch1_size_hrz, u32 ch1_size_vrt);
void set_EDC_CH1_CTL_secu_line(u32 edc_base, u32 nVal);
void set_EDC_CH1_CTL_y_flip(u32 edc_base, u32 nVal);
void set_EDC_CH1_CTL_tile_fmt(u32 edc_base, u32 nVal);
void set_EDC_CH1_CTL_tile_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_CTL_pix_fmt(u32 edc_base, u32 nVal);
void set_EDC_CH1_CTL_bgr(u32 edc_base, u32 nVal);
void set_EDC_CH1_CTL_colork_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_CTL_rot(u32 edc_base, u32 nVal);
void set_EDC_CH1_CTL_ch1_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_CTL_alp_premulti(u32 edc_base, u32 nVal);

void set_EDC_CH1_COLORK_MIN(u32 edc_base, u32 nVal);
void set_EDC_CH1_COLORK_MAX(u32 edc_base, u32 nVal);

void set_EDC_CH2L_ADDR(u32 edc_base, u32 nVal);
void set_EDC_CH2R_ADDR(u32 edc_base, u32 nVal);
void set_EDC_CH2_STRIDE(u32 edc_base, u32 nVal);
void set_EDC_CH2_XY(u32 edc_base, u32 ch2_x, u32 ch2_y);
void set_EDC_CH2_SIZE(u32 edc_base, u32 ch2_size_hrz, u32 ch2_size_vrt);
void set_EDC_CH2_CTL_secu_line(u32 edc_base, u32 nVal);
void set_EDC_CH2_CTL_pix_fmt(u32 edc_base, u32 nVal);
void set_EDC_CH2_CTL_bgr(u32 edc_base, u32 nVal);
void set_EDC_CH2_CTL_colork_en(u32 edc_base, u32 nVal);
void set_EDC_CH2_CTL_ch2_en(u32 edc_base, u32 nVal);
void set_EDC_CH2_COLORK_MIN(u32 edc_base, u32 nVal);
void set_EDC_CH2_COLORK_MAX(u32 edc_base, u32 nVal);

void set_EDC_CH12_OVLY_alp_blend_mskc3_en(u32 edc_base, u32 nVal);
void set_EDC_CH12_OVLY_alp_blend_mskc2_en(u32 edc_base, u32 nVal);
void set_EDC_CH12_OVLY_alp_blend_mskc1_en(u32 edc_base, u32 nVal);
void set_EDC_CH12_OVLY_alp_blend_mskc0_en(u32 edc_base, u32 nVal);
void set_EDC_CH12_OVLY_alp_blend_mskc_src(u32 edc_base, u32 nVal);
void set_EDC_CH12_OVLY_ch2_alp_sel(u32 edc_base, u32 nVal);
void set_EDC_CH12_OVLY_ch1_alp_sel(u32 edc_base, u32 nVal);
void set_EDC_CH12_OVLY_pix_alp_src(u32 edc_base, u32 nVal);
void set_EDC_CH12_OVLY_alp_src(u32 edc_base, u32 nVal);
void set_EDC_CH12_OVLY_alp_blend_en(u32 edc_base, u32 nVal);
void set_EDC_CH12_OVLY_ch2_top(u32 edc_base, u32 nVal);
void set_EDC_CH12_GLB_ALP_VAL(u32 edc_base, u32 ch12_alp_value0,
    u32 ch12_alp_value1);
void set_EDC_CH12_MSK_COLOR0_MIN(u32 edc_base, u32 nVal);
void set_EDC_CH12_MSK_COLOR0_MAX(u32 edc_base, u32 nVal);
void set_EDC_CH12_MSK_COLOR1_MIN(u32 edc_base, u32 nVal);
void set_EDC_CH12_MSK_COLOR1_MAX(u32 edc_base, u32 nVal);
void set_EDC_CH12_MSK_COLOR2_MIN(u32 edc_base, u32 nVal);
void set_EDC_CH12_MSK_COLOR2_MAX(u32 edc_base, u32 nVal);
void set_EDC_CH12_MSK_COLOR3_MIN(u32 edc_base, u32 nVal);
void set_EDC_CH12_MSK_COLOR3_MAX(u32 edc_base, u32 nVal);

void set_EDC_CRSL_ADDR(u32 edc_base, u32 nVal);
void set_EDC_CRSR_ADDR(u32 edc_base, u32 nVal);
void set_EDC_CRS_STRIDE(u32 edc_base, u32 nVal);
void set_EDC_CRS_XY(u32 edc_base, u32 crs_x, u32 crs_y);
void set_EDC_CRS_SIZE(u32 edc_base, u32 crs_size_hrz, u32 crs_size_vrt);
void set_EDC_CRS_CTL_secu_line(u32 edc_base, u32 nVal);
void set_EDC_CRS_CTL_alp_sel(u32 edc_base, u32 nVal);
void set_EDC_CRS_CTL_under_alp_sel(u32 edc_base, u32 nVal);
void set_EDC_CRS_CTL_pix_fmt(u32 edc_base, u32 nVal);
void set_EDC_CRS_CTL_bgr(u32 edc_base, u32 nVal);
void set_EDC_CRS_CTL_colork_en(u32 edc_base, u32 nVal);
void set_EDC_CRS_CTL_alp_src(u32 edc_base, u32 nVal);
void set_EDC_CRS_CTL_alp_blend_en(u32 edc_base, u32 nVal);
void set_EDC_CRS_CTL_en(u32 edc_base, u32 nVal);
void set_EDC_CRS_COLORK_MIN(u32 edc_base, u32 nVal);
void set_EDC_CRS_COLORK_MAX(u32 edc_base, u32 nVal);
void set_EDC_CRS_GLB_ALP_VAL(u32 edc_base, u32 crs_alp_value0,
    u32 crs_alp_value1);

void set_EDC_DISP_SIZE(u32 edc_base, u32 disp_size_hrz, u32 disp_size_vrt);
void set_EDC_DISP_CTL_cfg_ok(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_cfg_ok_sel(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_d3d_rf(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_frm_fmt(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_pix_fmt(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_dither_en(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_endian(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_gamma_en(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_ch_chg(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_nrot_burst(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_unflow_lev(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_outstding_dep(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_frm_end_start(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_crg_gt_en(u32 edc_base, u32 nVal);
void set_EDC_DISP_CTL_edc_en(u32 edc_base, u32 nVal);
void set_EDC_DISP_DPD_disp_dpd(u32 edc_base, u32 nVal);
void set_EDC_DISP_DPD_sbl_en(u32 edc_base, u32 nVal);
void set_EDC_INTS(u32 edc_base, u32 nVal);
void set_EDC_INTE(u32 edc_base, u32 nVal);

void set_EDC_CH1_CSCIDC_csc_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_CSCIDC(u32 edc_base, u32 nVal);
void set_EDC_CH1_CSCODC(u32 edc_base, u32 nVal);
void set_EDC_CH1_CSCP0(u32 edc_base, u32 nVal);
void set_EDC_CH1_CSCP1(u32 edc_base, u32 nVal);
void set_EDC_CH1_CSCP2(u32 edc_base, u32 nVal);
void set_EDC_CH1_CSCP3(u32 edc_base, u32 nVal);
void set_EDC_CH1_CSCP4(u32 edc_base, u32 nVal);

void set_EDC_CH2_CSCIDC_csc_en(u32 edc_base, u32 nVal);
void set_EDC_CH2_CSCIDC(u32 edc_base, u32 nVal);
void set_EDC_CH2_CSCODC(u32 edc_base, u32 nVal);
void set_EDC_CH2_CSCP0(u32 edc_base, u32 nVal);
void set_EDC_CH2_CSCP1(u32 edc_base, u32 nVal);
void set_EDC_CH2_CSCP2(u32 edc_base, u32 nVal);
void set_EDC_CH2_CSCP3(u32 edc_base, u32 nVal);
void set_EDC_CH2_CSCP4(u32 edc_base, u32 nVal);

void set_EDC_OUT_CSCIDC_csc_en(u32 edc_base, u32 nVal);
void set_EDC_OUT_CSCIDC(u32 edc_base, u32 nVal);
void set_EDC_OUT_CSCODC(u32 edc_base, u32 nVal);
void set_EDC_OUT_CSCP0(u32 edc_base, u32 nVal);
void set_EDC_OUT_CSCP1(u32 edc_base, u32 nVal);
void set_EDC_OUT_CSCP2(u32 edc_base, u32 nVal);
void set_EDC_OUT_CSCP3(u32 edc_base, u32 nVal);
void set_EDC_OUT_CSCP4(u32 edc_base, u32 nVal);

void set_EDC_CH1_SCL_HSP_hratio(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_HSP_hfir_order(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_HSP_hafir_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_HSP_hfir_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_HSP_hchmid_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_HSP_hlmid_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_HSP_hamid_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_HSP_hsc_en(u32 edc_base, u32 nVal);

void set_EDC_CH1_SCL_VSR_vratio(u32 edc_base, u32 vratio);
void set_EDC_CH1_SCL_VSP_vafir_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_VSP_vfir_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_VSP_vsc_luma_tap(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_VSP_vchmid_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_VSP_vlmid_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_VSP_vamid_en(u32 edc_base, u32 nVal);
void set_EDC_CH1_SCL_VSP_vsc_en(u32 edc_base, u32 nVal);

void set_EDC_CH1_SCL_HOFFSET(u32 edc_base, u32 hor_coffset , u32 hor_loffset);
void set_EDC_CH1_SCL_VOFFSET(u32 edc_base, u32 vbtm_offset, u32 vtp_offset);
void set_EDC_CH1_SCL_ORES(u32 edc_base, u32 ow, u32 oh);
void set_EDC_CH1_SCL_IRES(u32 edc_base, u32 iw, u32 ih);

void set_EDC_CH1_SCL_HPC(u32 edc_base, u32 i, u32 j, u32 nVal);
void set_EDC_CH1_SCL_VPC(u32 edc_base, u32 i, u32 j, u32 nVal);

#if defined(CONFIG_OVERLAY_COMPOSE)
void set_OVC_CH1_CSCIDC_csc_en(u32 edc_base, u32 nVal);
void set_OVC_CH1_CSCIDC(u32 edc_base, u32 cscidc_y, u32 cscidc_u, u32 cscidc_v);
void set_OVC_CH1_CSCODC(u32 edc_base, u32 cscodc_r, u32 cscodc_g, u32 cscodc_b);
void set_OVC_CH1_CSCP0(u32 edc_base, u32 csc_01p, u32 csc_00p);
void set_OVC_CH1_CSCP1(u32 edc_base, u32 csc_10p, u32 csc_02p);
void set_OVC_CH1_CSCP2(u32 edc_base, u32 csc_12p, u32 csc_11p);
void set_OVC_CH1_CSCP3(u32 edc_base, u32 csc_21p, u32 csc_20p);
void set_OVC_CH1_CSCP4(u32 edc_base, u32 csc_22p);

void set_OVC_CH2_CSCIDC_csc_en(u32 edc_base, u32 nVal);
void set_OVC_CH2_CSCIDC(u32 edc_base, u32 cscidc_y, u32 cscidc_u, u32 cscidc_v);
void set_OVC_CH2_CSCODC(u32 edc_base, u32 cscodc_r, u32 cscodc_g, u32 cscodc_b);
void set_OVC_CH2_CSCP0(u32 edc_base, u32 csc_01p, u32 csc_00p);
void set_OVC_CH2_CSCP1(u32 edc_base, u32 csc_10p, u32 csc_02p);
void set_OVC_CH2_CSCP2(u32 edc_base, u32 csc_12p, u32 csc_11p);
void set_OVC_CH2_CSCP3(u32 edc_base, u32 csc_21p, u32 csc_20p);
void set_OVC_CH2_CSCP4(u32 edc_base, u32 csc_22p);
#endif

void dump_EDC_Reg(u32 edc_base, u32 size);


#endif  /* EDC_REG_H */
