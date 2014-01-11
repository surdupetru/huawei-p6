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

#include "ldi_reg.h"


typedef union {
	struct {
		u32 hfp:12;
		u32 reserved0:8;
		u32 hbp:12;
	} bits;
	u32 ul32;
} LDI_HRZ_CTRL0;

typedef union {
	struct {
		u32 hsw:12;
		u32 reserved0:20;
	} bits;
	u32 ul32;
} LDI_HRZ_CTRL1;

typedef union {
	struct {
		u32 vfp:12;
		u32 reserved0:8;
		u32 vbp:12;
	} bits;
	u32 ul32;
} LDI_VRT_CTRL0;

typedef union {
	struct {
		u32 vsw:12;
		u32 reserved0:20;
	} bits;
	u32 ul32;
} LDI_VRT_CTRL1;


typedef union {
	struct {
		u32 vsync_plr:1;
		u32 hsync_plr:1;
		u32 pixel_clk_plr:1;
		u32 data_en_plr:1;
		u32 reserved0:28;
	} bits;
	u32 ul32;
} LDI_PLR_CTRL;

typedef union {
	struct {
		u32 hsize:12;
		u32 reserved0:8;
		u32 vsize:12;
	} bits;
	u32 ul32;
} LDI_DSP_SIZE;

typedef union {
	struct {
		u32 active_space:12;
		u32 reserved0:20;
	} bits;
	u32 ul32;
} LDI_3D_CTRL;

typedef union {
	struct {
		u32 frame_start_int_en:1;
		u32 frame_end_int_en:1;
		u32 edc_afifo_underflow_int_en:1;
		u32 vsync_int_en:1;
		u32 vbackporch_int_en:1;
		u32 vfrontporch_int_en:1;
		u32 vactive0_start_int_en:1;
		u32 vactive0_end_int_en:1;
		u32 vactive1_start_int_en:1;
		u32 vactive1_end_int_en:1;
		u32 reserved0:22;
	} bits;
	u32 ul32;
} LDI_INT_EN;

typedef union {
	struct {
		u32 ldi_en:1;
		u32 disp_mode_buf:1;
		u32 data_gate_en:1;
		u32 bpp:2;
		u32 wait_vsync_en:1;
		u32 colorbar_width:7;
		u32 bgr:1;
		u32 reserved0:18;
	} bits;
	u32 ul32;
} LDI_CTRL;

typedef union {
	struct {
		u32 frame_start_int:1;
		u32 frame_end_int:1;
		u32 edc_afifo_underflow_int:1;
		u32 vsync_int:1;
		u32 vbackporch_int:1;
		u32 vfrontporch_int:1;
		u32 vactive0_start_int:1;
		u32 vactive0_end_int:1;
		u32 vactive1_start_int:1;
		u32 vactive1_end_int:1;
		u32 reserved0:22;
	} bits;
	u32 ul32;
} LDI_ORG_INT;

typedef union {
	struct {
		u32 frame_start_msk_int:1;
		u32 frame_end_msk_int:1;
		u32 edc_afifo_underflow_msk_int:1;
		u32 vsync_msk_int:1;
		u32 vbackporch_msk_int:1;
		u32 vfrontporch_msk_int:1;
		u32 vactive0_start_msk_int:1;
		u32 vactive0_end_msk_int:1;
		u32 vactive1_start_msk_int:1;
		u32 vactive1_end_msk_int:1;
		u32 reserved0:22;
	} bits;
	u32 ul32;
} LDI_MSK_INT;

typedef union {
	struct {
		u32 frame_start_int_clr:1;
		u32 frame_end_int_clr:1;
		u32 edc_afifo_underflow_int_clr:1;
		u32 vsync_int_clr:1;
		u32 vbackporch_int_clr:1;
		u32 vfrontporch_int_clr:1;
		u32 vactive0_start_int_clr:1;
		u32 vactive0_end_int_clr:1;
		u32 vactive1_start_int_clr:1;
		u32 vactive1_end_int_clr:1;
		u32 reserved0:22;
	} bits;
	u32 ul32;
} LDI_INT_CLR;

typedef union {
	struct {
		u32 work_mode:1;
		u32 wback_en:1;
		u32 colorbar_en:1;
		u32 reserved0:29;
	} bits;
	u32 ul32;
} LDI_WORK_MODE;


/******************************************************************************
** FUNCTIONS IMPLEMENTATIONS
*/

void set_LDI_HRZ_CTRL0_hfp(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_HRZ_CTRL0_OFFSET;
	LDI_HRZ_CTRL0 ldi_hrz_ctrl0;

	ldi_hrz_ctrl0.ul32 = inp32(addr);
	ldi_hrz_ctrl0.bits.hfp = nVal;
	outp32(addr, ldi_hrz_ctrl0.ul32);
}

void set_LDI_HRZ_CTRL0_hbp(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_HRZ_CTRL0_OFFSET;
	LDI_HRZ_CTRL0 ldi_hrz_ctrl0;

	ldi_hrz_ctrl0.ul32 = inp32(addr);
	ldi_hrz_ctrl0.bits.hbp = nVal;
	outp32(addr, ldi_hrz_ctrl0.ul32);
}

void set_LDI_HRZ_CTRL1_hsw(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_HRZ_CTRL1_OFFSET;
	LDI_HRZ_CTRL1 ldi_hrz_ctrl1;

	ldi_hrz_ctrl1.ul32 = inp32(addr);
	ldi_hrz_ctrl1.bits.hsw = (nVal > 0) ? nVal - 1 : 0;
	outp32(addr, ldi_hrz_ctrl1.ul32);
}

void set_LDI_VRT_CTRL0_vfp(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_VRT_CTRL0_OFFSET;
	LDI_VRT_CTRL0 ldi_vrt_ctrl0;

	ldi_vrt_ctrl0.ul32 = inp32(addr);
	ldi_vrt_ctrl0.bits.vfp = nVal;
	outp32(addr, ldi_vrt_ctrl0.ul32);
}

void set_LDI_VRT_CTRL0_vbp(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_VRT_CTRL0_OFFSET;
	LDI_VRT_CTRL0 ldi_vrt_ctrl0;

	ldi_vrt_ctrl0.ul32 = inp32(addr);
	ldi_vrt_ctrl0.bits.vbp = nVal;
	outp32(addr, ldi_vrt_ctrl0.ul32);
}

void set_LDI_VRT_CTRL1_vsw(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_VRT_CTRL1_OFFSET;
	LDI_VRT_CTRL1 ldi_vrt_ctrl1;

	ldi_vrt_ctrl1.ul32 = inp32(addr);
	ldi_vrt_ctrl1.bits.vsw = (nVal > 0) ? nVal - 1 : 0;
	outp32(addr, ldi_vrt_ctrl1.ul32);
}

void set_LDI_PLR_CTRL_vsync(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_PLR_CTRL_OFFSET;
	LDI_PLR_CTRL ldi_plr_ctrl;

	ldi_plr_ctrl.ul32 = inp32(addr);
	ldi_plr_ctrl.bits.vsync_plr = nVal;
	outp32(addr, ldi_plr_ctrl.ul32);
}

void set_LDI_PLR_CTRL_hsync(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_PLR_CTRL_OFFSET;
	LDI_PLR_CTRL ldi_plr_ctrl;

	ldi_plr_ctrl.ul32 = inp32(addr);
	ldi_plr_ctrl.bits.hsync_plr = nVal;
	outp32(addr, ldi_plr_ctrl.ul32);
}

void set_LDI_PLR_CTRL_pixel_clk(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_PLR_CTRL_OFFSET;
	LDI_PLR_CTRL ldi_plr_ctrl;

	ldi_plr_ctrl.ul32 = inp32(addr);
	ldi_plr_ctrl.bits.pixel_clk_plr = nVal;
	outp32(addr, ldi_plr_ctrl.ul32);
}

void set_LDI_PLR_CTRL_data_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_PLR_CTRL_OFFSET;
	LDI_PLR_CTRL ldi_plr_ctrl;

	ldi_plr_ctrl.ul32 = inp32(addr);
	ldi_plr_ctrl.bits.data_en_plr = nVal;
	outp32(addr, ldi_plr_ctrl.ul32);
}

void set_LDI_DSP_SIZE_hsize(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_DSP_SIZE_OFFSET;
	LDI_DSP_SIZE ldi_dsp_size;

	ldi_dsp_size.ul32 = inp32(addr);
	ldi_dsp_size.bits.hsize = (nVal > 0) ? nVal-1 : 0;
	outp32(addr, ldi_dsp_size.ul32);
}

void set_LDI_DSP_SIZE_vsize(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_DSP_SIZE_OFFSET;
	LDI_DSP_SIZE ldi_dsp_size;

	ldi_dsp_size.ul32 = inp32(addr);
	ldi_dsp_size.bits.vsize = (nVal > 0) ? nVal-1 : 0;
	outp32(addr, ldi_dsp_size.ul32);
}

void set_LDI_3D_CTRL_active_space(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_3D_CTRL_OFFSET;
	LDI_3D_CTRL ldi_3d_ctrl;

	ldi_3d_ctrl.ul32 = inp32(addr);
	ldi_3d_ctrl.bits.active_space = nVal;
	outp32(addr, ldi_3d_ctrl.ul32);
}

void set_LDI_CTRL_ldi_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_CTRL_OFFSET;
	LDI_CTRL ldi_ctrl;

	ldi_ctrl.ul32 = inp32(addr);
	ldi_ctrl.bits.ldi_en = nVal;
	outp32(addr, ldi_ctrl.ul32);
}

void set_LDI_CTRL_disp_mode(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_CTRL_OFFSET;
	LDI_CTRL ldi_ctrl;

	ldi_ctrl.ul32 = inp32(addr);
	ldi_ctrl.bits.disp_mode_buf = nVal;
	outp32(addr, ldi_ctrl.ul32);
}

void set_LDI_CTRL_data_gate_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_CTRL_OFFSET;
	LDI_CTRL ldi_ctrl;

	ldi_ctrl.ul32 = inp32(addr);
	ldi_ctrl.bits.data_gate_en = nVal;
	outp32(addr, ldi_ctrl.ul32);
}

void set_LDI_CTRL_bpp(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_CTRL_OFFSET;
	LDI_CTRL ldi_ctrl;

	ldi_ctrl.ul32 = inp32(addr);
	ldi_ctrl.bits.bpp = nVal;
	outp32(addr, ldi_ctrl.ul32);
}

void set_LDI_CTRL_wait_vsync_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_CTRL_OFFSET;
	LDI_CTRL ldi_ctrl;

	ldi_ctrl.ul32 = inp32(addr);
	ldi_ctrl.bits.wait_vsync_en = nVal;
	outp32(addr, ldi_ctrl.ul32);
}

void set_LDI_CTRL_corlorbar_width(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_CTRL_OFFSET;
	LDI_CTRL ldi_ctrl;

	ldi_ctrl.ul32 = inp32(addr);
	ldi_ctrl.bits.colorbar_width = (nVal > 0) ? nVal - 1 : 0;
	outp32(addr, ldi_ctrl.ul32);
}

void set_LDI_CTRL_bgr(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_CTRL_OFFSET;
	LDI_CTRL ldi_ctrl;

	ldi_ctrl.ul32 = inp32(addr);
	ldi_ctrl.bits.bgr = nVal;
	outp32(addr, ldi_ctrl.ul32);
}

void set_LDI_INT_CLR(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_INT_CLR_OFFSET;
	outp32(addr, nVal);
}

void set_LDI_WORK_MODE_work_mode(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_WORK_MODE_OFFSET;
	LDI_WORK_MODE ldi_work_mode;

	ldi_work_mode.ul32 = inp32(addr);
	ldi_work_mode.bits.work_mode = nVal;
	outp32(addr, ldi_work_mode.ul32);
}

void set_LDI_WORK_MODE_wback_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_WORK_MODE_OFFSET;
	LDI_WORK_MODE ldi_work_mode;

	ldi_work_mode.ul32 = inp32(addr);
	ldi_work_mode.bits.wback_en = nVal;
	outp32(addr, ldi_work_mode.ul32);
}

void set_LDI_WORK_MODE_colorbar_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + LDI_WORK_MODE_OFFSET;
	LDI_WORK_MODE ldi_work_mode;

	ldi_work_mode.ul32 = inp32(addr);
	ldi_work_mode.bits.colorbar_en = nVal;
	outp32(addr, ldi_work_mode.ul32);
}
