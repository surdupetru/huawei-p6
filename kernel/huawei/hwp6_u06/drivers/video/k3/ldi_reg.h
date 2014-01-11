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

#ifndef LDI_REG_H
#define LDI_REG_H

#include "k3_edc.h"
#include "k3_fb_def.h"


#define LDI_OFFSET				(0x800)
#define LDI_HRZ_CTRL0_OFFSET	(LDI_OFFSET + 0x0)
#define LDI_HRZ_CTRL1_OFFSET	(LDI_OFFSET + 0x4)
#define LDI_VRT_CTRL0_OFFSET	(LDI_OFFSET + 0x8)
#define LDI_VRT_CTRL1_OFFSET	(LDI_OFFSET + 0xC)
#define LDI_PLR_CTRL_OFFSET		(LDI_OFFSET + 0x10)
#define LDI_DSP_SIZE_OFFSET		(LDI_OFFSET + 0x14)
#define LDI_3D_CTRL_OFFSET		(LDI_OFFSET + 0x18)
#define LDI_INT_EN_OFFSET		(LDI_OFFSET + 0x1C)
#define LDI_CTRL_OFFSET			(LDI_OFFSET + 0x20)
#define LDI_ORG_INT_OFFSET		(LDI_OFFSET + 0x24)
#define LDI_MSK_INT_OFFSET		(LDI_OFFSET + 0x28)
#define LDI_INT_CLR_OFFSET		(LDI_OFFSET + 0x2C)
#define LDI_WORK_MODE_OFFSET	(LDI_OFFSET + 0x30)
#define LDI_HDMI_DSI_GT			(LDI_OFFSET + 0x34)
#define LDI_DE_SPACE_LOW		(LDI_OFFSET + 0x38)


/******************************************************************************
** FUNCTIONS PROTOTYPES
*/

void set_LDI_HRZ_CTRL0_hfp(u32 edc_base, u32 nVal);
void set_LDI_HRZ_CTRL0_hbp(u32 edc_base, u32 nVal);
void set_LDI_HRZ_CTRL1_hsw(u32 edc_base, u32 nVal);
void set_LDI_VRT_CTRL0_vfp(u32 edc_base, u32 nVal);
void set_LDI_VRT_CTRL0_vbp(u32 edc_base, u32 nVal);
void set_LDI_VRT_CTRL1_vsw(u32 edc_base, u32 nVal);
void set_LDI_PLR_CTRL_vsync(u32 edc_base, u32 nVal);
void set_LDI_PLR_CTRL_hsync(u32 edc_base, u32 nVal);
void set_LDI_PLR_CTRL_pixel_clk(u32 edc_base, u32 nVal);
void set_LDI_PLR_CTRL_data_en(u32 edc_base, u32 nVal);
void set_LDI_DSP_SIZE_hsize(u32 edc_base, u32 nVal);
void set_LDI_DSP_SIZE_vsize(u32 edc_base, u32 nVal);
void set_LDI_3D_CTRL_active_space(u32 edc_base, u32 nVal);
void set_LDI_CTRL_ldi_en(u32 edc_base, u32 nVal);
void set_LDI_CTRL_disp_mode(u32 edc_base, u32 nVal);
void set_LDI_CTRL_data_gate_en(u32 edc_base, u32 nVal);
void set_LDI_CTRL_bpp(u32 edc_base, u32 nVal);
void set_LDI_CTRL_wait_vsync_en(u32 edc_base, u32 nVal);
void set_LDI_CTRL_corlorbar_width(u32 edc_base, u32 nVal);
void set_LDI_CTRL_bgr(u32 edc_base, u32 nVal);
void set_LDI_INT_CLR(u32 edc_base, u32 nVal);
void set_LDI_WORK_MODE_work_mode(u32 edc_base, u32 nVal);
void set_LDI_WORK_MODE_wback_en(u32 edc_base, u32 nVal);
void set_LDI_WORK_MODE_colorbar_en(u32 edc_base, u32 nVal);


#endif  /* __LDI_REG_H__ */
