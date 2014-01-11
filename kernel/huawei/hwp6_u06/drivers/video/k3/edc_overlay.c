/* Copyright (c) 2008-2010, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include <linux/fb.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <mach/boardid.h>

#include "k3_fb.h"
#include "edc_reg.h"
#include "ldi_reg.h"
#include "mipi_reg.h"
#include "sbl_reg.h"

/*--------------------VERTICLE--------------------*/
/* enlarge */
static s16 gfxcoefficient4_cubic[16][4] = {
	{0, 511, 0, 0, },
	{-37, 509, 42, -2, },
	{-64, 499, 86, -9, },
	{-82, 484, 129, -19, },
	{-94, 463, 174, -31, },
	{-98, 438, 217, -45, },
	{-98, 409, 260, -59, },
	{-92, 376, 300, -72, },
	{-83, 339, 339, -83, },
	{-72, 300, 376, -92, },
	{-59, 260, 409, -98, },
	{-45, 217, 438, -98, },
	{-31, 174, 463, -94, },
	{-19, 129, 484, -82, },
	{-9, 86, 499, -64, },
	{-2, 42, 509, -37, }
};

/* scaler down below 4/3 times */
static s16 gfxcoefficient4_lanczos2_6M_a15[16][4] = {
	{79, 383, 79, -29},
	{58, 384, 102, -32},
	{40, 380, 127, -35},
	{23, 374, 153, -38},
	{8, 363, 180, -39},
	{-6, 349, 208, -39},
	{-16, 331, 235, -38},
	{-25, 311, 262, -36},
	{-31, 287, 287, -31},
	{-36, 262, 311, -25},
	{-38, 235, 331, -16},
	{-39, 208, 349, -6},
	{-39, 180, 363, 8},
	{-38, 153, 374, 23},
	{-35, 127, 380, 40},
	{-32, 102, 384, 58}
};

/* scaler down below 2 times */
static s16 gfxcoefficient4_lanczos2_5M_a15[16][4] = {
	{103, 328, 103, -22},
	{85, 328, 121, -22},
	{69, 324, 141, -22},
	{53, 319, 161, -21},
	{40, 311, 181, -20},
	{27, 301, 201, -17},
	{16, 288, 221, -13},
	{7, 273, 240, -8},
	{-2, 258, 258, -2},
	{-8, 240, 273, 7},
	{-13, 221, 288, 16},
	{-17, 201, 301, 27},
	{-20, 181, 311, 40},
	{-21, 161, 319, 53},
	{-22, 141, 324, 69},
	{-22, 121, 328, 85}
};

/*--------------------HORIZONTAL--------------------*/
/* enlarge */
static s16 gfxcoefficient8_cubic[8][8] = {
	{0, 0, 0, 511, 0, 0, 0, 0, },
	{-3, 11, -41, 496, 61, -16, 4, 0, },
	{-4, 17, -63, 451, 138, -35, 9, -1, },
	{-4, 18, -70, 386, 222, -52, 14, -2, },
	{-3, 17, -65, 307, 307, -65, 17, -3, },
	{-2, 14, -52, 222, 386, -70, 18, -4, },
	{-1, 9, -35, 138, 451, -63, 17, -4, },
	{0, 4, -16, 61, 496, -41, 11, -3, }
};

/* scaler down */
static s16 gfxcoefficient8_lanczos2_8tap[8][8] = {
	{-16, 0, 145, 254, 145, 0, -16, 0},
	{-13, -9, 123, 252, 167, 11, -19, 0},
	{-10, -15, 101, 245, 188, 25, -21, -1},
	{-7, -19, 80, 236, 206, 41, -22, -3},
	{-5, -21, 60, 222, 222, 60, -21, -5},
	{-3, -22, 41, 206, 236, 80, -19, -7},
	{-1, -21, 25, 188, 245, 101, -15, -10},
	{0, -19, 11, 167, 252, 123, -9, -13}
};

static bool isAlphaRGBType(int format)
{
	switch (format) {
	case EDC_ARGB_1555:
	case EDC_ARGB_8888:
		return true;
	default:
		return false;
	}
	return false;
}

static int get_bytespp(int format)
{
	switch (format) {
	case EDC_ARGB_1555:
	case EDC_RGB_565:
		return 2;
	case EDC_XRGB_8888:
	case EDC_ARGB_8888:
		return 4;
	case EDC_YUYV_I:
	case EDC_UYVY_I:
	case EDC_YVYU_I:
	case EDC_VYUY_I:
	default:
		return 2;
	}
}

static bool is64BytesOddAlign(int type, u32 stride)
{
	if (type == FB_64BYTES_ODD_ALIGN_NONE) {
		return true;
	} else {
		return (((stride / 64) % 2 == 0) ? false : true);
	}
}

static bool isNeedDither(struct k3_fb_data_type *k3fd)
{
	BUG_ON(k3fd == NULL);

	if (k3fd->fb_imgType > EDC_ARGB_8888) {
		return false;
	}

	if (((k3fd->panel_info.bpp == EDC_OUT_RGB_565) && (k3fd->fb_imgType == EDC_RGB_565)) ||
		((k3fd->panel_info.bpp == EDC_OUT_RGB_888) && (k3fd->fb_imgType == EDC_ARGB_8888)) ||
		((k3fd->panel_info.bpp == EDC_OUT_RGB_888) && (k3fd->fb_imgType == EDC_XRGB_8888))) {
		return  false;
	}

	return true;
}

static u32 computeDisplayAddr(struct k3fb_img *img,
	u32 rotation, struct k3fb_rect *src)
{
	u32 addr = 0;
	u32 temp = 0;

	BUG_ON(img == NULL || src == NULL);

	switch (rotation) {
	case EDC_ROT_NOP:
		{
			addr = img->phy_addr + img->stride * src->y +
				src->x * get_bytespp(img->format);
		}
		break;
	case EDC_ROT_90:
		{
			addr = img->phy_addr + img->stride * src->y +
				src->x * get_bytespp(img->format);

			temp = src->w;
			src->w = src->h;
			src->h = temp;
		}
		break;
	case EDC_ROT_180:
		{
			addr =  img->phy_addr + img->stride * (src->y + src->h - 1) +
				src->x * get_bytespp(img->format);
		}
		break;
	case EDC_ROT_270:
		{
			addr =  img->phy_addr + img->stride * (src->y + src->h - 1) +
				src->x * get_bytespp(img->format);

			temp = src->w;
			src->w = src->h;
			src->h = temp;
		}
		break;
	default:
		k3fb_loge("Unsupported rotation(%d)!\n", rotation);
		break;
	}

	return addr;
}

static void edc_overlay_pipe_init4ch1(struct edc_overlay_pipe *pipe)
{
	BUG_ON(pipe == NULL);

	pipe->edc_ch_info.cap.ckey_enable = 0;
	pipe->edc_ch_info.ckeymin = 0xffffcc;
	pipe->edc_ch_info.ckeymax = 0xffffec;

	pipe->edc_ch_info.cap.alpha_enable = 1;
	pipe->edc_ch_info.alp_src = EDC_ALP_PIXEL;
	pipe->edc_ch_info.alpha0 = 0x7f;
	pipe->edc_ch_info.alpha1 = 0x7f;

	pipe->edc_ch_info.cap.filter_enable = 1;
	pipe->edc_ch_info.cap.csc_enable = 1;

	pipe->set_EDC_CHL_ADDR = set_EDC_CH1L_ADDR;
	pipe->set_EDC_CHR_ADDR = set_EDC_CH1R_ADDR;
	pipe->set_EDC_CH_CTL_ch_en = set_EDC_CH1_CTL_ch1_en;
	pipe->set_EDC_CH_CTL_secu_line = set_EDC_CH1_CTL_secu_line;
	pipe->set_EDC_CH_CTL_bgr = set_EDC_CH1_CTL_bgr;
	pipe->set_EDC_CH_CTL_pix_fmt = set_EDC_CH1_CTL_pix_fmt;
	pipe->set_EDC_CH_CTL_colork_en = set_EDC_CH1_CTL_colork_en;
	pipe->set_EDC_CH_COLORK_MIN = set_EDC_CH1_COLORK_MIN;
	pipe->set_EDC_CH_COLORK_MAX = set_EDC_CH1_COLORK_MAX;
	pipe->set_EDC_CH_XY = set_EDC_CH1_XY;
	pipe->set_EDC_CH_SIZE = set_EDC_CH1_SIZE;
	pipe->set_EDC_CH_STRIDE = set_EDC_CH1_STRIDE;

	pipe->set_EDC_CH_CSCIDC_csc_en = set_EDC_CH1_CSCIDC_csc_en;
	pipe->set_EDC_CH_CSCIDC = set_EDC_CH1_CSCIDC;
	pipe->set_EDC_CH_CSCODC = set_EDC_CH1_CSCODC;
	pipe->set_EDC_CH_CSCP0 = set_EDC_CH1_CSCP0;
	pipe->set_EDC_CH_CSCP1 = set_EDC_CH1_CSCP1;
	pipe->set_EDC_CH_CSCP2 = set_EDC_CH1_CSCP2;
	pipe->set_EDC_CH_CSCP3 = set_EDC_CH1_CSCP3;
	pipe->set_EDC_CH_CSCP4 = set_EDC_CH1_CSCP4;

#if defined(CONFIG_OVERLAY_COMPOSE)
	pipe->set_OVC_CH_CSCIDC = set_OVC_CH1_CSCIDC;
	pipe->set_OVC_CH_CSCODC = set_OVC_CH1_CSCODC;
	pipe->set_OVC_CH_CSCP0 = set_OVC_CH1_CSCP0;
	pipe->set_OVC_CH_CSCP1 = set_OVC_CH1_CSCP1;
	pipe->set_OVC_CH_CSCP2 = set_OVC_CH1_CSCP2;
	pipe->set_OVC_CH_CSCP3 = set_OVC_CH1_CSCP3;
	pipe->set_OVC_CH_CSCP4 = set_OVC_CH1_CSCP4;
#endif //CONFIG_OVERLAY_COMPOSE

}

static void edc_overlay_pipe_init4ch2(struct edc_overlay_pipe *pipe)
{
	BUG_ON(pipe == NULL);

	pipe->edc_ch_info.cap.ckey_enable = 0;
	pipe->edc_ch_info.ckeymin = 0xffffcc;
	pipe->edc_ch_info.ckeymax = 0xffffec;

	pipe->edc_ch_info.cap.alpha_enable = 1;
	pipe->edc_ch_info.alp_src = EDC_ALP_PIXEL;
	pipe->edc_ch_info.alpha0 = 0x7f;
	pipe->edc_ch_info.alpha1 = 0x7f;

	pipe->edc_ch_info.cap.filter_enable = 1;
	pipe->edc_ch_info.cap.csc_enable = 1;

	pipe->set_EDC_CHL_ADDR = set_EDC_CH2L_ADDR;
	pipe->set_EDC_CHR_ADDR = set_EDC_CH2R_ADDR;
	pipe->set_EDC_CH_CTL_ch_en = set_EDC_CH2_CTL_ch2_en;
	pipe->set_EDC_CH_CTL_secu_line = set_EDC_CH2_CTL_secu_line;
	pipe->set_EDC_CH_CTL_bgr = set_EDC_CH2_CTL_bgr;
	pipe->set_EDC_CH_CTL_pix_fmt = set_EDC_CH2_CTL_pix_fmt;
	pipe->set_EDC_CH_CTL_colork_en = set_EDC_CH2_CTL_colork_en;
	pipe->set_EDC_CH_COLORK_MIN = set_EDC_CH2_COLORK_MIN;
	pipe->set_EDC_CH_COLORK_MAX = set_EDC_CH2_COLORK_MAX;
	pipe->set_EDC_CH_XY = set_EDC_CH2_XY;
	pipe->set_EDC_CH_SIZE = set_EDC_CH2_SIZE;
	pipe->set_EDC_CH_STRIDE = set_EDC_CH2_STRIDE;

	pipe->set_EDC_CH_CSCIDC_csc_en = set_EDC_CH2_CSCIDC_csc_en;
	pipe->set_EDC_CH_CSCIDC = set_EDC_CH2_CSCIDC;
	pipe->set_EDC_CH_CSCODC = set_EDC_CH2_CSCODC;
	pipe->set_EDC_CH_CSCP0 = set_EDC_CH2_CSCP0;
	pipe->set_EDC_CH_CSCP1 = set_EDC_CH2_CSCP1;
	pipe->set_EDC_CH_CSCP2 = set_EDC_CH2_CSCP2;
	pipe->set_EDC_CH_CSCP3 = set_EDC_CH2_CSCP3;
	pipe->set_EDC_CH_CSCP4 = set_EDC_CH2_CSCP4;

#if defined(CONFIG_OVERLAY_COMPOSE)
	pipe->set_OVC_CH_CSCIDC = set_OVC_CH2_CSCIDC;
	pipe->set_OVC_CH_CSCODC = set_OVC_CH2_CSCODC;
	pipe->set_OVC_CH_CSCP0 = set_OVC_CH2_CSCP0;
	pipe->set_OVC_CH_CSCP1 = set_OVC_CH2_CSCP1;
	pipe->set_OVC_CH_CSCP2 = set_OVC_CH2_CSCP2;
	pipe->set_OVC_CH_CSCP3 = set_OVC_CH2_CSCP3;
	pipe->set_OVC_CH_CSCP4 = set_OVC_CH2_CSCP4;
#endif //CONFIG_OVERLAY_COMPOSE

}

#if defined(CONFIG_OVERLAY_COMPOSE)
static void edc_overlay_pipe_init4crs(struct edc_overlay_pipe *pipe)
{
	BUG_ON(pipe == NULL);

	pipe->edc_ch_info.cap.ckey_enable = 0;
	pipe->edc_ch_info.ckeymin = 0xffffcc;
	pipe->edc_ch_info.ckeymax = 0xffffec;

	pipe->edc_ch_info.cap.alpha_enable = 1;
	pipe->edc_ch_info.alp_src = EDC_ALP_PIXEL;
	pipe->edc_ch_info.alpha0 = 0x7f;
	pipe->edc_ch_info.alpha1 = 0x7f;

	pipe->edc_ch_info.cap.filter_enable = 0;
	pipe->edc_ch_info.cap.csc_enable = 0;

	pipe->set_EDC_CHL_ADDR = set_EDC_CRSL_ADDR;
	pipe->set_EDC_CHR_ADDR = set_EDC_CRSR_ADDR;
	pipe->set_EDC_CH_CTL_ch_en = set_EDC_CRS_CTL_en;
	pipe->set_EDC_CH_CTL_secu_line = set_EDC_CRS_CTL_secu_line;
	pipe->set_EDC_CH_CTL_bgr = set_EDC_CRS_CTL_bgr;
	pipe->set_EDC_CH_CTL_pix_fmt = set_EDC_CRS_CTL_pix_fmt;
	pipe->set_EDC_CH_CTL_colork_en = set_EDC_CRS_CTL_colork_en;
	pipe->set_EDC_CH_COLORK_MIN = set_EDC_CRS_COLORK_MIN;
	pipe->set_EDC_CH_COLORK_MAX = set_EDC_CRS_COLORK_MAX;
	pipe->set_EDC_CH_XY = set_EDC_CRS_XY;
	pipe->set_EDC_CH_SIZE = set_EDC_CRS_SIZE;
	pipe->set_EDC_CH_STRIDE = set_EDC_CRS_STRIDE;

	pipe->set_EDC_CH_CSCIDC_csc_en = NULL;
	pipe->set_EDC_CH_CSCIDC = NULL;
	pipe->set_EDC_CH_CSCODC = NULL;
	pipe->set_EDC_CH_CSCP0 = NULL;
	pipe->set_EDC_CH_CSCP1 = NULL;
	pipe->set_EDC_CH_CSCP2 = NULL;
	pipe->set_EDC_CH_CSCP3 = NULL;
	pipe->set_EDC_CH_CSCP4 = NULL;

	pipe->set_OVC_CH_CSCIDC = NULL;
	pipe->set_OVC_CH_CSCODC = NULL;
	pipe->set_OVC_CH_CSCP0 = NULL;
	pipe->set_OVC_CH_CSCP1 = NULL;
	pipe->set_OVC_CH_CSCP2 = NULL;
	pipe->set_OVC_CH_CSCP3 = NULL;
	pipe->set_OVC_CH_CSCP4 = NULL;

}
#endif //CONFIG_OVERLAY_COMPOSE

/* initialize infomation for 4 pipes of overlay*/
void edc_overlay_init(struct edc_overlay_ctrl *ctrl)
{
	struct edc_overlay_pipe *pipe = NULL;

	BUG_ON(ctrl == NULL);

	pipe = &ctrl->plist[0];
	pipe->pipe_type = OVERLAY_TYPE_CHCAP_ALL,
	pipe->pipe_num = OVERLAY_PIPE_EDC0_CH1,
	pipe->edc_base = k3fd_reg_base_edc0,
	edc_overlay_pipe_init4ch1(pipe);

	pipe = &ctrl->plist[1];
	pipe->pipe_type = OVERLAY_TYPE_CHCAP_PARTIAL,
	pipe->pipe_num = OVERLAY_PIPE_EDC0_CH2,
	pipe->edc_base = k3fd_reg_base_edc0,
	edc_overlay_pipe_init4ch2(pipe);

	pipe = &ctrl->plist[2];
	pipe->pipe_type = OVERLAY_TYPE_CHCAP_ALL,
	pipe->pipe_num = OVERLAY_PIPE_EDC1_CH1,
	pipe->edc_base = k3fd_reg_base_edc1,
	edc_overlay_pipe_init4ch1(pipe);

	pipe = &ctrl->plist[3];
	pipe->pipe_type = OVERLAY_TYPE_CHCAP_PARTIAL,
	pipe->pipe_num = OVERLAY_PIPE_EDC1_CH2,
	pipe->edc_base = k3fd_reg_base_edc1,
	edc_overlay_pipe_init4ch2(pipe);

#if defined(CONFIG_OVERLAY_COMPOSE)
	pipe = &ctrl->plist[OVERLAY_PIPE_EDC0_CURSOR];
	pipe->pipe_type = OVERLAY_TYPE_CHCAP_PARTIAL,
	pipe->pipe_num = OVERLAY_PIPE_EDC0_CURSOR,
	pipe->edc_base = k3fd_reg_base_edc0,
	edc_overlay_pipe_init4crs(pipe);
#endif //CONFIG_OVERLAY_COMPOSE
}

static struct edc_overlay_pipe *edc_overlay_ndx2pipe(struct fb_info *info, int ndx)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct edc_overlay_ctrl *ctrl = NULL;
	struct edc_overlay_pipe *pipe = NULL;

	BUG_ON(info == NULL || info->par == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;

	if (ndx < 0 || ndx >= OVERLAY_PIPE_MAX)
		return NULL;

	ctrl = &k3fd->ctrl;
	pipe = &ctrl->plist[ndx];

	return pipe;
}

int edc_overlay_get(struct fb_info *info, struct overlay_info *req)
{
	struct edc_overlay_pipe *pipe = NULL;

	BUG_ON(info == NULL || req == NULL);

	pipe = edc_overlay_ndx2pipe(info, req->id);
	if (pipe == NULL) {
		k3fb_loge("id=%d not able to get pipe!\n", req->id);
		return -ENODEV;
	}

	/* *req = pipe->req_info; */
	memcpy(req, &pipe->req_info, sizeof(struct overlay_info));

	return 0;
}

int edc_overlay_set(struct fb_info *info, struct overlay_info *req)
{
	struct edc_overlay_pipe *pipe = NULL;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(info == NULL || req == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	pipe = edc_overlay_ndx2pipe(info, req->id);
	if (pipe == NULL) {
		k3fb_loge("id=%d not able to get pipe!\n", req->id);
		return -ENODEV;
	}

	memcpy(&pipe->req_info, req, sizeof(struct overlay_info));
	pipe->req_info.is_pipe_used = 1;

	return 0;
}

int edc_overlay_unset(struct fb_info *info, int ndx)
{
	struct edc_overlay_pipe *pipe = NULL;
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;
	int i = 0;
	int j = 0;

	BUG_ON(info == NULL || info->par == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	BUG_ON(k3fd == NULL);

	pipe = edc_overlay_ndx2pipe(info, ndx);
	if (pipe == NULL) {
		k3fb_loge("id=%d not able to get pipe!\n", ndx);
		return -ENODEV;
	}

#if defined(CONFIG_OVERLAY_COMPOSE)
	//unset it in pan display
	if (pipe->req_info.is_overlay_compose) {
		/* pipe->req_info.is_overlay_compose = 0; */
		memset(&pipe->req_info, 0, sizeof(struct overlay_info));
		pipe->req_info.need_unset = 1;
		return 0;
	}
#endif

	edc_base = pipe->edc_base;
	down(&k3fd->sem);
	memset(&pipe->req_info, 0, sizeof(struct overlay_info));

	if (!k3fd->panel_power_on) {
		up(&k3fd->sem);
		k3fb_logw("id=%d has power down!\n", ndx);
		return 0;
	}

	k3_fb_clk_enable_cmd_mode(k3fd);

	pipe->set_EDC_CHL_ADDR(edc_base, 0x0);
	pipe->set_EDC_CHR_ADDR(edc_base, 0x0);
	pipe->set_EDC_CH_CTL_ch_en(edc_base, 0x0);
	pipe->set_EDC_CH_CTL_secu_line(edc_base, 0x0);
	pipe->set_EDC_CH_CTL_bgr(edc_base, 0x0);
	pipe->set_EDC_CH_CTL_pix_fmt(edc_base, 0x0);
	pipe->set_EDC_CH_CTL_colork_en(edc_base, 0x0);
	pipe->set_EDC_CH_COLORK_MIN(edc_base, 0x0);
	pipe->set_EDC_CH_COLORK_MAX(edc_base, 0x0);
	pipe->set_EDC_CH_XY(edc_base, 0x0, 0x0);
	pipe->set_EDC_CH_SIZE(edc_base, 0x0, 0x0);
	pipe->set_EDC_CH_STRIDE(edc_base, 0x0);

	if (pipe->set_EDC_CH_CSCIDC_csc_en) {
		pipe->set_EDC_CH_CSCIDC_csc_en(edc_base, 0x0);
	}

	if (pipe->set_EDC_CH_CSCIDC) {
		pipe->set_EDC_CH_CSCIDC(edc_base, 0x0);
	}
	if (pipe->set_EDC_CH_CSCODC) {
		pipe->set_EDC_CH_CSCODC(edc_base, 0x0);
	}
	if (pipe->set_EDC_CH_CSCP0) {
		pipe->set_EDC_CH_CSCP0(edc_base, 0x0);
	}
	if (pipe->set_EDC_CH_CSCP1) {
		pipe->set_EDC_CH_CSCP1(edc_base, 0x0);
	}
	if (pipe->set_EDC_CH_CSCP2) {
		pipe->set_EDC_CH_CSCP2(edc_base, 0x0);
	}
	if (pipe->set_EDC_CH_CSCP3) {
		pipe->set_EDC_CH_CSCP3(edc_base, 0x0);
	}
	if (pipe->set_EDC_CH_CSCP4) {
		pipe->set_EDC_CH_CSCP4(edc_base, 0x0);
	}

	if (pipe->pipe_type == OVERLAY_TYPE_CHCAP_ALL) {
		set_EDC_CH1_CTL_rot(edc_base, 0x0);
		set_EDC_CH1_SCL_IRES(edc_base, 0x0, 0x0);
		set_EDC_CH1_SCL_ORES(edc_base, 0x0, 0x0);

		set_EDC_CH1_SCL_HSP_hsc_en(edc_base, 0x0);
		set_EDC_CH1_SCL_HSP_hratio(edc_base, 0x0);
		set_EDC_CH1_SCL_HSP_hafir_en(edc_base, 0x0);
		set_EDC_CH1_SCL_HSP_hfir_en(edc_base, 0x0);
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 4; j++) {
				set_EDC_CH1_SCL_HPC(edc_base, i, j, 0x0);
			}
		}

		set_EDC_CH1_SCL_VSP_vsc_en(edc_base, 0x0);
		set_EDC_CH1_SCL_VSR_vratio(edc_base, 0x0);
		set_EDC_CH1_SCL_VSP_vafir_en(edc_base, 0x0);
		set_EDC_CH1_SCL_VSP_vfir_en(edc_base, 0x0);
		for (i = 0; i < 16; i++) {
			for (j = 0; j < 2; j++) {
				set_EDC_CH1_SCL_VPC(edc_base, i, j, 0x0);
			}
		}
	}

	set_EDC_CH12_OVLY_alp_blend_en(edc_base, 0x0);
	set_EDC_CH12_OVLY_alp_src(edc_base, 0x0);
	set_EDC_CH12_GLB_ALP_VAL(edc_base, 0x0, 0x0);
	set_EDC_CH12_OVLY_pix_alp_src(edc_base, 0x0);
	set_EDC_CH12_OVLY_ch1_alp_sel(edc_base, 0x0);
	set_EDC_CH12_OVLY_ch2_alp_sel(edc_base, 0x0);
#if defined(CONFIG_OVERLAY_COMPOSE)
	set_EDC_CRS_CTL_alp_blend_en(edc_base, 0x0);
	set_EDC_CRS_CTL_alp_src(edc_base, 0x0);
	set_EDC_CRS_CTL_under_alp_sel(edc_base, 0x0);
	set_EDC_CRS_CTL_alp_sel(edc_base, 0x0);
	set_EDC_CRS_GLB_ALP_VAL(edc_base, 0x0, 0x0);
#endif //CONFIG_OVERLAY_COMPOSE

	set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);

	k3_fb_clk_disable_cmd_mode(k3fd);

	up(&k3fd->sem);
	return 0;
}


#if defined(CONFIG_OVERLAY_COMPOSE)
#define EDC_BIT_MASK (0x1)
#define EDC_CFG_OK_BIT  (1)
#define CH1_ROTATE_BIT  (21)
#define CH1_EN_BIT  (24)
#define CH2_EN_BIT  (21)
#define CH2_FMT_BIT  (16)
#define CH2_FMT_MASK (0x7)
#define CRS_EN_BIT  (27)

#if defined(EDC_CH_CHG_SUPPORT)
enum {
    CH_CHG_DISABLE,
    CH_CHG_ENABLE
};
enum {
    CFG_SEL_EDC1,
    CFG_SEL_EDC0
};
#endif //EDC_CH_CHG_SUPPORT
volatile u32 edc_overlay_compose_get_cfg_ok(struct k3_fb_data_type *k3fd)
{
    volatile u32 edc_ctl;
    BUG_ON(k3fd == NULL);
    edc_ctl = inp32(k3fd->edc_base + EDC_DISP_CTL_OFFSET);
    return ((edc_ctl >> EDC_CFG_OK_BIT) & EDC_BIT_MASK);
}

volatile u32 edc_overlay_compose_reg_get_ch1_rotation(struct k3_fb_data_type *k3fd)
{
    volatile u32 ch1_ctl;
    BUG_ON(k3fd == NULL);
    ch1_ctl = inp32(k3fd->edc_base + EDC_CH1_CTL_OFFSET);
    return ((ch1_ctl >> CH1_ROTATE_BIT) & 0x3);
}

volatile u32 edc_overlay_compose_get_ch1_state(struct k3_fb_data_type *k3fd)
{
    volatile u32 ch1_ctl;
    BUG_ON(k3fd == NULL);
    ch1_ctl = inp32(k3fd->edc_base + EDC_CH1_CTL_OFFSET);
    return ((ch1_ctl >> CH1_EN_BIT) & EDC_BIT_MASK);
}

volatile u32 edc_overlay_compose_get_ch2_state(struct k3_fb_data_type *k3fd)
{
    volatile u32 ch2_ctl;
    BUG_ON(k3fd == NULL);
    ch2_ctl = inp32(k3fd->edc_base + EDC_CH2_CTL_OFFSET);
    return ((ch2_ctl >> CH2_EN_BIT) & EDC_BIT_MASK);
}

volatile u32 edc_overlay_compose_get_ch2_fmt(struct k3_fb_data_type *k3fd)
{
    volatile u32 ch2_ctl;
    BUG_ON(k3fd == NULL);
    ch2_ctl = inp32(k3fd->edc_base + EDC_CH2_CTL_OFFSET);
    return ((ch2_ctl >> CH2_FMT_BIT) & CH2_FMT_MASK);
}

volatile u32 edc_overlay_compose_get_crs_state(struct k3_fb_data_type *k3fd)
{
    volatile u32 crs_ctl;
    BUG_ON(k3fd == NULL);
    crs_ctl = inp32(k3fd->edc_base + EDC_CRS_CTL_OFFSET);
    return ((crs_ctl >> CRS_EN_BIT) & EDC_BIT_MASK);
}

u32 edc_overlay_compose_get_state(struct fb_info *info, u32 pipe_id)
{
    struct edc_overlay_pipe *pipe = NULL;
    pipe = edc_overlay_ndx2pipe(info, pipe_id);
    if (pipe == NULL) {
        k3fb_loge("id=%d not able to get pipe!", pipe_id);
        return 0;
    }
    return pipe->req_info.is_overlay_compose;
}

u32 edc_overlay_compose_get_cfg_disable(struct fb_info *info, u32 pipe_id)
{
    struct edc_overlay_pipe *pipe = NULL;
    pipe = edc_overlay_ndx2pipe(info, pipe_id);
    if (pipe == NULL) {
        k3fb_loge("id=%d not able to get pipe!", pipe_id);
        return 1;
    }
    return pipe->req_info.cfg_disable;
}

u32 edc_overlay_compose_get_ch1_rotation(struct fb_info *info, u32 pipe_id)
{
    struct edc_overlay_pipe *pipe = NULL;
    pipe = edc_overlay_ndx2pipe(info, pipe_id);
    if (pipe == NULL) {
        k3fb_loge("id=%d not able to get pipe!", pipe_id);
        return 0;
    }
    return pipe->req_info.rotation;
}


static void edc_overlay_compose_graphic_alpha_set(struct overlay_data *req, u32 edc_base)
{
    #define HWC_BLENDING_PREMULT (0x0105)
    #define HWC_BLENDING_COVERAGE (0x0405)

    int32_t blending;
    u32 pipe;
    /* Get perpixelAlpha enabling state. */
    bool perpixelAlpha;
    /* Get plane alpha value. */
    u32 planeAlpha;

    pipe = req->id;

    if (pipe == OVERLAY_PIPE_EDC0_CH1) {
        //opaque
        return;
    }

    blending =  req->src.blending;
    planeAlpha = blending >> 16;
    perpixelAlpha = isAlphaRGBType(req->src.format);

//    printk("---blending = %x\n", blending);
    switch (blending & 0xFFFF) {
        case HWC_BLENDING_PREMULT:
            if (pipe == OVERLAY_PIPE_EDC0_CURSOR) {
                set_EDC_CRS_CTL_alp_blend_en(edc_base, K3_ENABLE);
                if (perpixelAlpha) {
                    set_EDC_CRS_CTL_alp_src(edc_base, EDC_ALP_PIXEL);
                    set_EDC_CRS_CTL_under_alp_sel(edc_base, EDC_ALP_MUL_COEFF_1);
                    set_EDC_CRS_CTL_alp_sel(edc_base, EDC_ALP_MUL_COEFF_3);
                } else if (planeAlpha < 0xFF) {
                    set_EDC_CRS_CTL_alp_src(edc_base, EDC_ALP_GLOBAL);
                    set_EDC_CRS_GLB_ALP_VAL(edc_base, planeAlpha, 0x0);
                } else {
                    set_EDC_CRS_CTL_alp_blend_en(edc_base, K3_DISABLE);
                }
            }else {
                set_EDC_CH12_OVLY_alp_blend_en(edc_base, K3_ENABLE);
                if (perpixelAlpha) {
                    set_EDC_CH12_OVLY_alp_src(edc_base, EDC_ALP_PIXEL);
                    set_EDC_CH12_OVLY_ch2_top(edc_base, EDC_CH2_TOP);
                    set_EDC_CH12_OVLY_pix_alp_src(edc_base, EDC_PIX_ALP_SRC_CH2);
                    set_EDC_CH12_OVLY_ch1_alp_sel(edc_base, EDC_ALP_MUL_COEFF_1);
                    set_EDC_CH12_OVLY_ch2_alp_sel(edc_base, EDC_ALP_MUL_COEFF_3);
                } else if (planeAlpha < 0xFF) {
                    set_EDC_CH12_OVLY_alp_src(edc_base, EDC_ALP_GLOBAL);
                    set_EDC_CH12_GLB_ALP_VAL(edc_base, planeAlpha, 0x0);
                } else {
                    set_EDC_CH12_OVLY_alp_blend_en(edc_base, K3_DISABLE);
                }
            }
            break;
        case HWC_BLENDING_COVERAGE:
            if (pipe == OVERLAY_PIPE_EDC0_CURSOR) {
                if (perpixelAlpha) {
                    set_EDC_CRS_CTL_alp_blend_en(edc_base, K3_ENABLE);
                    set_EDC_CRS_CTL_alp_src(edc_base, EDC_ALP_PIXEL);
                    set_EDC_CRS_CTL_under_alp_sel(edc_base, EDC_ALP_MUL_COEFF_1);
                    set_EDC_CRS_CTL_alp_sel(edc_base, EDC_ALP_MUL_COEFF_0);
                } else if (planeAlpha < 0xFF) {
                    set_EDC_CRS_CTL_alp_src(edc_base, EDC_ALP_GLOBAL);
                    set_EDC_CRS_GLB_ALP_VAL(edc_base, planeAlpha, 0x0);
                } else {
                    set_EDC_CRS_CTL_alp_blend_en(edc_base, K3_DISABLE);
                }
            } else {
                set_EDC_CH12_OVLY_alp_blend_en(edc_base, K3_ENABLE);
                if (perpixelAlpha) {
                    set_EDC_CH12_OVLY_alp_src(edc_base, EDC_ALP_PIXEL);
                    set_EDC_CH12_OVLY_ch2_top(edc_base, EDC_CH2_TOP);
                    set_EDC_CH12_OVLY_pix_alp_src(edc_base, EDC_PIX_ALP_SRC_CH2);
                    set_EDC_CH12_OVLY_ch1_alp_sel(edc_base, EDC_ALP_MUL_COEFF_1);
                    set_EDC_CH12_OVLY_ch2_alp_sel(edc_base, EDC_ALP_MUL_COEFF_0);
                } else if (planeAlpha < 0xFF) {
                    set_EDC_CH12_OVLY_alp_src(edc_base, EDC_ALP_GLOBAL);
                    set_EDC_CH12_GLB_ALP_VAL(edc_base, planeAlpha, 0x0);
                } else {
                    set_EDC_CH12_OVLY_alp_blend_en(edc_base, K3_DISABLE);
                }
            }
            break;
        default:
            if (pipe == OVERLAY_PIPE_EDC0_CURSOR) {
                set_EDC_CRS_CTL_alp_blend_en(edc_base, K3_DISABLE);
            }else {
                set_EDC_CH12_OVLY_alp_blend_en(edc_base, K3_DISABLE);
            }
            break;
    }
}

static void edc_overlay_compose_partial_graphic_alpha_set(struct overlay_data *req, u32 edc_base)
{
    #define HWC_BLENDING_PREMULT (0x0105)
    #define HWC_BLENDING_COVERAGE (0x0405)

    int32_t blending;
    /* Get perpixelAlpha enabling state. */
    bool perpixelAlpha;
    /* Get plane alpha value. */
    u32 planeAlpha;

    blending =  req->src.partial_blending;
    perpixelAlpha = isAlphaRGBType(req->src.partial_format);
    planeAlpha = blending >> 16;
    /*printk("partial OVC:%d,0x%x \n", perpixelAlpha, blending );*/

    switch (blending & 0xFFFF) {
        case HWC_BLENDING_PREMULT:
            set_EDC_CH12_OVLY_alp_blend_en(edc_base, K3_ENABLE);
            if (perpixelAlpha) {
                set_EDC_CH12_OVLY_alp_src(edc_base, EDC_ALP_PIXEL);
                set_EDC_CH12_OVLY_ch2_top(edc_base, EDC_CH2_TOP);
                set_EDC_CH12_OVLY_pix_alp_src(edc_base, EDC_PIX_ALP_SRC_CH2);
                set_EDC_CH12_OVLY_ch1_alp_sel(edc_base, EDC_ALP_MUL_COEFF_1);
                set_EDC_CH12_OVLY_ch2_alp_sel(edc_base, EDC_ALP_MUL_COEFF_3);
            }else if (planeAlpha < 0xFF) {
                set_EDC_CH12_OVLY_alp_src(edc_base, EDC_ALP_GLOBAL);
                set_EDC_CH12_GLB_ALP_VAL(edc_base, planeAlpha, 0x0);
            } else {
                set_EDC_CH12_OVLY_alp_blend_en(edc_base, K3_DISABLE);
            }
            break;

        case HWC_BLENDING_COVERAGE:
            set_EDC_CH12_OVLY_alp_blend_en(edc_base, K3_ENABLE);
            if (perpixelAlpha) {
                set_EDC_CH12_OVLY_alp_src(edc_base, EDC_ALP_PIXEL);
                set_EDC_CH12_OVLY_ch2_top(edc_base, EDC_CH2_TOP);
                set_EDC_CH12_OVLY_pix_alp_src(edc_base, EDC_PIX_ALP_SRC_CH2);
                set_EDC_CH12_OVLY_ch1_alp_sel(edc_base, EDC_ALP_MUL_COEFF_1);
                set_EDC_CH12_OVLY_ch2_alp_sel(edc_base, EDC_ALP_MUL_COEFF_0);
            }else if (planeAlpha < 0xFF) {
                set_EDC_CH12_OVLY_alp_src(edc_base, EDC_ALP_GLOBAL);
                set_EDC_CH12_GLB_ALP_VAL(edc_base, planeAlpha, 0x0);
            } else {
                set_EDC_CH12_OVLY_alp_blend_en(edc_base, K3_DISABLE);
            }
            break;
        default:
            set_EDC_CH12_OVLY_alp_blend_en(edc_base, K3_DISABLE);
            break;
    }

}

static void edc_overlay_compose_ch_csc_unset(struct edc_overlay_pipe *pipe, u32 edc_base)
{
    if (pipe == NULL) {
        k3fb_loge("invalid pipe!");
        return;
    }

    if (pipe->set_EDC_CH_CSCIDC_csc_en) {
        pipe->set_EDC_CH_CSCIDC_csc_en(edc_base, 0x0);
    }
    if (pipe->set_OVC_CH_CSCIDC) {
        pipe->set_OVC_CH_CSCIDC(edc_base, 0x0, 0x0, 0x0);
    }
    if (pipe->set_OVC_CH_CSCODC) {
        pipe->set_OVC_CH_CSCODC(edc_base, 0x0, 0x0, 0x0);
    }
    if (pipe->set_OVC_CH_CSCP0) {
        pipe->set_OVC_CH_CSCP0(edc_base, 0x0, 0x0);
    }
    if (pipe->set_OVC_CH_CSCP1) {
        pipe->set_OVC_CH_CSCP1(edc_base, 0x0, 0x0);
    }
    if (pipe->set_OVC_CH_CSCP2) {
        pipe->set_OVC_CH_CSCP2(edc_base, 0x0, 0x0);
    }
    if (pipe->set_OVC_CH_CSCP3) {
        pipe->set_OVC_CH_CSCP3(edc_base, 0x0, 0x0);
    }
    if (pipe->set_OVC_CH_CSCP4) {
        pipe->set_OVC_CH_CSCP4(edc_base, 0x0);
    }
}

void edc_overlay_compose_pre_unset(struct fb_info *info, int ndx)
{
    struct edc_overlay_pipe *pipe = NULL;

    BUG_ON(info == NULL);
    pipe = edc_overlay_ndx2pipe(info, ndx);
    if (pipe == NULL) {
        k3fb_loge("id=%d not able to get pipe!\n", ndx);
        return;
    }

    //unset it in cfg ok
    if (pipe->req_info.is_overlay_compose) {
        /* pipe->req_info.is_overlay_compose = 0; */
        memset(&pipe->req_info, 0, sizeof(struct overlay_info));
        pipe->req_info.need_unset = 1;
    }
}

static int edc_overlay_compose_pipe_unset(struct fb_info *info)
{
    struct edc_overlay_pipe *pipe = NULL;
    u32 edc_base = 0;
    int ch_id;

    BUG_ON(info == NULL || info->par == NULL);
    for (ch_id = 0; ch_id < OVERLAY_PIPE_MAX; ch_id++) {
        pipe = edc_overlay_ndx2pipe(info, ch_id);
        if (pipe == NULL) {
            k3fb_loge("id=%d not able to get pipe!", ch_id);
            return -ENODEV;
        }
        edc_base = pipe->edc_base;
        if (pipe->req_info.need_unset) {
            pipe->req_info.need_unset = 0;
            /* memset(&pipe->req_info, 0, sizeof(struct overlay_info)); */
            pipe->set_EDC_CHL_ADDR(edc_base, 0x0);
            pipe->set_EDC_CHR_ADDR(edc_base, 0x0);
            pipe->set_EDC_CH_XY(edc_base, 0x0, 0x0);
            pipe->set_EDC_CH_SIZE(edc_base, 0x0, 0x0);
            pipe->set_EDC_CH_STRIDE(edc_base, 0x0);
            pipe->set_EDC_CH_CTL_pix_fmt(edc_base, 0x0);
            pipe->set_EDC_CH_CTL_bgr(edc_base, 0x0);
            pipe->set_EDC_CH_CTL_colork_en(edc_base, 0x0);

            pipe->set_EDC_CH_CTL_ch_en(edc_base, 0x0);

            if (pipe->pipe_type == OVERLAY_TYPE_CHCAP_ALL) {
                set_EDC_CH1_CTL_rot(edc_base, 0);
                //notice: only reset the enable reg of scaler when fs
                set_EDC_CH1_SCL_HSP_hsc_en(edc_base, K3_DISABLE);
                set_EDC_CH1_SCL_VSP_vsc_en(edc_base, K3_DISABLE);
            }

            edc_overlay_compose_ch_csc_unset(pipe, edc_base);
            if (ch_id == OVERLAY_PIPE_EDC0_CURSOR) {
                set_EDC_CRS_CTL_alp_blend_en(edc_base, 0x0);
                set_EDC_CRS_CTL_alp_src(edc_base, 0x0);
                set_EDC_CRS_GLB_ALP_VAL(edc_base, 0x0, 0x0);
                set_EDC_CRS_CTL_under_alp_sel(edc_base, 0x0);
                set_EDC_CRS_CTL_alp_sel(edc_base, 0x0);
            } else {
                set_EDC_CH12_OVLY_alp_blend_en(edc_base, 0x0);
                set_EDC_CH12_OVLY_alp_src(edc_base, 0x0);
                set_EDC_CH12_GLB_ALP_VAL(edc_base, 0x0, 0x0);
                set_EDC_CH12_OVLY_pix_alp_src(edc_base, 0x0);
                set_EDC_CH12_OVLY_ch1_alp_sel(edc_base, 0x0);
                set_EDC_CH12_OVLY_ch2_alp_sel(edc_base, 0x0);
            }
        }
    }

    return 0;
}

static int edc_overlay_compose_g2d_unset(struct fb_info *info)
{
    struct k3_fb_data_type *k3fd = NULL;
    struct edc_overlay_pipe *pipe = NULL;
    u32 edc_base = 0;
    int ch_id;

    BUG_ON(info == NULL || info->par == NULL);
    k3fd = (struct k3_fb_data_type *)info->par;

    /* reset ovc state */
    for (ch_id = 0; ch_id < OVERLAY_PIPE_MAX; ch_id++) {
        pipe = edc_overlay_ndx2pipe(info, ch_id);
        if (pipe == NULL) {
            k3fb_loge("id=%d not able to get ovc pipe!", ch_id);
            return -ENODEV;
        }
        edc_base = pipe->edc_base;
        if (pipe->req_info.need_unset) {
            pipe->req_info.need_unset = 0;
        }
    }

    /* g2d state */
    ch_id = k3fd->graphic_ch; /* graphic channel */
    pipe = edc_overlay_ndx2pipe(info, ch_id);
    if (pipe == NULL) {
        k3fb_loge("id=%d not able to get 2d pipe!", ch_id);
        return -ENODEV;
    }
    edc_base = pipe->edc_base;

    pipe->set_EDC_CHL_ADDR(edc_base, 0x0);
    pipe->set_EDC_CHR_ADDR(edc_base, 0x0);
    pipe->set_EDC_CH_XY(edc_base, 0x0, 0x0);
    pipe->set_EDC_CH_SIZE(edc_base, 0x0, 0x0);
    pipe->set_EDC_CH_STRIDE(edc_base, 0x0);
    pipe->set_EDC_CH_CTL_pix_fmt(edc_base, 0x0);
    pipe->set_EDC_CH_CTL_bgr(edc_base, 0x0);
    pipe->set_EDC_CH_CTL_colork_en(edc_base, 0x0);
    pipe->set_EDC_CH_CTL_ch_en(edc_base, 0x0);
    return 0;
}

int edc_overlay_compose_pipe_unset_previous(struct fb_info *info, u32 previous_type, u32 previous_count)
{
    struct k3_fb_data_type *k3fd = NULL;
    bool unset_flag = false;

    BUG_ON(info == NULL || info->par == NULL);
    k3fd = (struct k3_fb_data_type *)info->par;

    unset_flag = (previous_type != k3fd->ovc_type) || (previous_count != k3fd->ovc_ch_count);
    if (!unset_flag) {
        return 0;
    }

    switch (previous_type) {
        case OVC_FULL:
            edc_overlay_compose_pipe_unset(info);
            break;
        case OVC_PARTIAL:
            edc_overlay_compose_pipe_unset(info);
            edc_overlay_compose_g2d_unset(info);
            break;
        case OVC_NONE:
            edc_overlay_compose_g2d_unset(info);
            break;
        default:
            break;
    }
    return 0;
}

#if defined(EDC_CH_CHG_SUPPORT)
static int overlay_compose_get_edc1_hw(struct clk **edc_clk, struct regulator **edc_vcc)
{
    *edc_clk = clk_get(NULL, CLK_EDC1_NAME);
    if (IS_ERR(*edc_clk)) {
        k3fb_loge("failed to get edc1_clk!\n");
        return -EINVAL;
    }

    *edc_vcc = regulator_get(NULL,  VCC_EDC1_NAME);
    if (IS_ERR(*edc_vcc)) {
        k3fb_loge("failed to get edc1-vcc regulator\n");
        return -EINVAL;
    }
    return 0;
}

static void overlay_compose_edc1_power_on(struct k3_fb_data_type *k3fd)
{
    struct clk *edc1_clk;
    struct regulator *edc1_vcc;

    if (overlay_compose_get_edc1_hw(&edc1_clk, &edc1_vcc) < 0) {
        k3fb_loge("failed to get edc1 para.\n");
    }

    /* edc1 vcc */
    if (regulator_enable(edc1_vcc) != 0) {
        k3fb_loge("failed to enable edc-vcc regulator.\n");
    }

    /* edc clock enable */
    if (clk_enable(edc1_clk) != 0) {
        k3fb_loge("failed to enable edc1 clock.\n");
    }
    /* Note: call clk_set_rate after clk_set_rate, set edc clock rate to normal value */
    //edc1 clk uses edc0's rate !
    if (clk_set_rate(edc1_clk, k3fd->panel_info.clk_rate * 12 / 10) != 0) {
        k3fb_loge("failed to set edc1 clk rate(%d).\n", k3fd->panel_info.clk_rate * 12 / 10);
    }

}

void overlay_compose_edc1_power_off(void)
{
    struct clk *edc1_clk;
    struct regulator *edc1_vcc;

    if (overlay_compose_get_edc1_hw(&edc1_clk, &edc1_vcc) < 0) {
        k3fb_loge("failed to get edc1 para.\n");
    }
    /* edc clock gating */
    clk_disable(edc1_clk);

    /* edc1 vcc */
    if (regulator_disable(edc1_vcc) != 0) {
        k3fb_loge("failed to disable edc-vcc regulator.\n");
    }
}

int edc_overlay_compose_ch_chg_disable(struct k3_fb_data_type *k3fd, struct fb_info *info)
{
    //edc0
    struct edc_overlay_pipe *edc0_pipe = NULL;
    u32 edc0_base = 0;

    //edc1
    struct fb_info* info1 = k3_fb1_get_info();
    struct edc_overlay_pipe *edc1_pipe = NULL;
    u32 edc1_base = 0;


//edc0
    BUG_ON(info == NULL || info->par == NULL);

    if (!k3fd->ch_chg_flag) {
        return 0;
    }
    k3fd->ch_chg_flag = false;
    k3fd->ch_chg_power_off = true;


    edc0_pipe = edc_overlay_ndx2pipe(info, OVERLAY_PIPE_EDC0_CH2);
    if (edc0_pipe == NULL) {
        k3fb_loge("OVERLAY_PIPE_EDC0_CH2 not able to get pipe!");
        return -ENODEV;
    }
    edc0_base = edc0_pipe->edc_base;

//edc1
    BUG_ON(info1 == NULL || info1->par == NULL);
    edc1_pipe = edc_overlay_ndx2pipe(info1, OVERLAY_PIPE_EDC1_CH1);
    if (edc1_pipe == NULL) {
        k3fb_loge("OVERLAY_PIPE_EDC1_CH1 not able to get pipe!");
        return -ENODEV;
    }
    edc1_base = edc1_pipe->edc_base;

    edc1_pipe->set_EDC_CH_CTL_ch_en(edc1_base, K3_DISABLE);
    set_EDC_DISP_CTL_ch_chg(edc0_base, CH_CHG_DISABLE);
    set_EDC_DISP_CTL_cfg_ok_sel(edc0_base, CFG_SEL_EDC1);

    return 0;
}

static int edc_overlay_compose_ch_chg_enable(struct k3_fb_data_type *k3fd)
{
    if (k3fd->ch_chg_flag) {
        return 0;
    }
    k3fd->ch_chg_flag = true;
    overlay_compose_edc1_power_on(k3fd);
    return 0;
}
#endif //EDC_CH_CHG_SUPPORT

int edc_overlay_compose_play(struct fb_info *info, struct overlay_data *req)
{
    struct edc_overlay_pipe *pipe = NULL;
    struct k3_fb_data_type *k3fd = NULL;
    struct overlay_info *ov_info = NULL;
    struct k3fb_rect src;
    /*struct k3fb_rect logic_rect;*/
    struct k3fb_rect dst;  /* physic */
    u32 edc0_base = 0;
    struct edc_overlay_pipe *edc0_pipe = NULL;

    u32 edc_base = 0;

    int i = 0;
    int j = 0;
    s16 stSCLHorzCoef[8][8] = {{0},};
    s16 stSCLVertCoef[16][4] = {{0},};
    u32 scl_ratio_w = 0;
    u32 scl_ratio_h = 0;
    u32 addr = 0;

    #define MAX_EDC_W (1920)
    #define MAX_EDC_H (1200)
    u32 max_w;
    u32 max_h;

    bool is_rotate = false;
    u32 cfg_disable;

#if defined(EDC_CH_CHG_SUPPORT)
    //edc1
    struct k3_fb_data_type *k3fd1 = NULL;
    struct fb_info* info1 = k3_fb1_get_info();
    struct edc_overlay_pipe *edc1_pipe = NULL;
    u32 edc1_base = 0;
    bool ch_chg = false;
#endif //EDC_CH_CHG_SUPPORT

    BUG_ON(info == NULL || info->par == NULL || req == NULL);

#ifdef DEBUG_ON
    printk(
       "\nDUMP overlay play:\n"
       "type=%d\n"
       "phy_addr=%x\n"
       "actual_width=%d\n"
       "actual_height=%d\n"
       "format=%d\n"
       "s3d_format=%d\n"
       "stride=%d\n"
       "width=%d\n"
       "height=%d\n"
       "id=%d\n"
       "is_graphic=%d\n",
       req->src.type,
       req->src.phy_addr,
       req->src.actual_width,
       req->src.actual_height,
       req->src.format,
       req->src.s3d_format,
       req->src.stride,
       req->src.width,
       req->src.height,
       req->id,
       req->is_graphic);
#endif

    k3fd = (struct k3_fb_data_type *)info->par;
    edc0_pipe = edc_overlay_ndx2pipe(info, req->id);
    if (edc0_pipe == NULL) {
        k3fb_loge("id=%d not able to get pipe!", req->id);
        return -ENODEV;
    }
    //set to edc0
    pipe = edc0_pipe;

    /* stride 64byte odd align */
    if (!is64BytesOddAlign(FB_64BYTES_ODD_ALIGN_NONE, req->src.stride)) {
        k3fb_logw("stride NOT 64 bytes odd aligned, %x\n", req->src.stride);
        return -EINVAL;
    }

    if (pipe->req_info.dst_rect.w == 0 ||
        pipe->req_info.dst_rect.h == 0 ||
        pipe->req_info.src_rect.w == 0 ||
        pipe->req_info.src_rect.h == 0) {
        k3fb_logw("invalid width or height! dst(%d,%d), src(%d,%d)",
            pipe->req_info.dst_rect.w, pipe->req_info.dst_rect.h,
            pipe->req_info.src_rect.w, pipe->req_info.src_rect.h);
        return -EINVAL;
    }

    edc0_base = pipe->edc_base;
    //set to edc0
    edc_base = edc0_base;

    ov_info = &pipe->req_info;

    cfg_disable =  pipe->req_info.cfg_disable;
#ifdef DEBUG_ON
    printk("cfg_disable=%d\n",cfg_disable);
#endif

    is_rotate = ((ov_info->rotation == EDC_ROT_90) || (ov_info->rotation == EDC_ROT_270));
    if (k3fd->panel_info.orientation == LCD_LANDSCAPE) {
        max_w = is_rotate ? MAX_EDC_H : MAX_EDC_W;
        max_h = is_rotate ? MAX_EDC_W : MAX_EDC_H;
    }
    else {
        max_w = is_rotate ? MAX_EDC_W : MAX_EDC_H;
        max_h = is_rotate ? MAX_EDC_H : MAX_EDC_W;
    }
    src.x = ov_info->src_rect.x;
    src.y = ov_info->src_rect.y;
    if (ov_info->src_rect.w > max_w) {
        ov_info->src_rect.w = max_w;
    }
    src.w = ov_info->src_rect.w;

    if (ov_info->src_rect.h > max_h) {
        ov_info->src_rect.h = max_h;
    }
    src.h = ov_info->src_rect.h;

    dst = ov_info->dst_rect;

#ifdef DEBUG_ON
    printk("scr(%d,%d,%d,%d) ==> dst(%d,%d,%d,%d)\n",src.x,src.y,src.w,src.h, dst.x,dst.y,dst.w,dst.h);
#endif

    /*computeDisplayRegion(&(req->src), k3fd->panel_info.orientation,
        &(ov_info->dst_rect), &logic_rect);
    logic2physicCoordinate(ov_info->rotation, &logic_rect, &dst);*/
    addr = computeDisplayAddr(&(req->src), ov_info->rotation, &src);
    //128bit(16byte) align
    if (addr & 0x0f) {
        k3fb_logw("addr NOT 64 bytes aligned, %x\n", addr);
        return -EINVAL;
    }

    if (!ov_info->is_pipe_used) {
        return 0;
    }

    if (!k3fd->panel_power_on) {
        return 0;
    }

    /* check edc_afifo_underflow_int interrupt */
    if ((inp32(edc_base + LDI_ORG_INT_OFFSET) & 0x4) == 0x4) {
        set_reg(edc_base + LDI_INT_CLR_OFFSET, 0x1, 1, 2);
        pr_notice("k3fb, %s: edc_afifo_underflow_int !!!\n", __func__);
    }

#if defined(EDC_CH_CHG_SUPPORT)
    //channel change flow
    if (ov_info->rotation) {
        if (!cfg_disable) {
            if (req->id == OVERLAY_PIPE_EDC0_CH2) {
                //replace edc1ch2
                ch_chg = true;
                edc_overlay_compose_ch_chg_enable(k3fd);
            } else {
                edc_overlay_compose_ch_chg_disable(k3fd, info);
            }
        }
    } else {
        edc_overlay_compose_ch_chg_disable(k3fd, info);
    }

    if (ch_chg) {
        pipe->set_EDC_CH_CTL_ch_en(edc_base, K3_DISABLE);
        set_EDC_DISP_CTL_ch_chg(edc_base, CH_CHG_ENABLE);
        set_EDC_DISP_CTL_cfg_ok_sel(edc_base, CFG_SEL_EDC0);

        BUG_ON(info1 == NULL || info1->par == NULL);
        k3fd1 = (struct k3_fb_data_type *)info1->par;
        BUG_ON(k3fd1 == NULL);
        edc1_pipe = edc_overlay_ndx2pipe(info1, OVERLAY_PIPE_EDC1_CH1);
        if (edc1_pipe == NULL) {
            k3fb_loge("OVERLAY_PIPE_EDC1_CH1 not able to get pipe!");
            return -ENODEV;
        }
        edc1_base = edc1_pipe->edc_base;

        //set to edc1
        pipe = edc1_pipe;
        edc_base = edc1_base;

        pipe->set_EDC_CH_CTL_ch_en(edc_base, K3_ENABLE);
    }
#endif //EDC_CH_CHG_SUPPORT

    if (req->src.type == EDC_RIGHT_ADDR) {
        pipe->set_EDC_CHR_ADDR(edc_base, addr);
    } else if (req->src.type == EDC_LEFT_ADDR) {
        pipe->set_EDC_CHL_ADDR(edc_base, addr);
    } else {
        pipe->set_EDC_CHL_ADDR(edc_base, addr);
        pipe->set_EDC_CHR_ADDR(edc_base, addr);
    }
    pipe->set_EDC_CH_XY(edc_base, dst.x, dst.y);
    pipe->set_EDC_CH_SIZE(edc_base, ov_info->src_rect.w, ov_info->src_rect.h);
    pipe->set_EDC_CH_STRIDE(edc_base, req->src.stride);
    pipe->set_EDC_CH_CTL_pix_fmt(edc_base, req->src.format);
    pipe->set_EDC_CH_CTL_bgr(edc_base, req->src.bgr_fmt);
    pipe->set_EDC_CH_CTL_ch_en(edc_base, K3_ENABLE);

    pipe->set_EDC_CH_CTL_colork_en(edc_base, pipe->edc_ch_info.cap.ckey_enable);
    if (pipe->edc_ch_info.cap.ckey_enable) {
        pipe->set_EDC_CH_COLORK_MIN(edc_base, pipe->edc_ch_info.ckeymin);
        pipe->set_EDC_CH_COLORK_MAX(edc_base, pipe->edc_ch_info.ckeymax);
    }

    if (pipe->pipe_type == OVERLAY_TYPE_CHCAP_ALL) {
        set_EDC_CH1_CTL_rot(edc_base, ov_info->rotation);

        if (src.w == dst.w && src.h == dst.h) {
            set_EDC_CH1_SCL_HSP_hsc_en(edc_base, K3_DISABLE);
            set_EDC_CH1_SCL_VSP_vsc_en(edc_base, K3_DISABLE);
        } else {
            set_EDC_CH1_SCL_HSP_hsc_en(edc_base, K3_ENABLE);
            set_EDC_CH1_SCL_VSP_vsc_en(edc_base, K3_ENABLE);

            set_EDC_CH1_SCL_IRES(edc_base, src.w, src.h);
            set_EDC_CH1_SCL_ORES(edc_base, dst.w, dst.h);

            scl_ratio_w = (src.w << 12) / dst.w;
            scl_ratio_h = (src.h << 12) / dst.h;
            /*
			if ((scl_ratio_w > ((10<<12)/7)) && (scl_ratio_h < (1<<12))) {
				scl_ratio_w = ((10<<12)/7);
			} else if ((scl_ratio_h > ((10<<12)/7)) && (scl_ratio_w < (1<<12))) {
				scl_ratio_h = ((10<<12)/7);
			}
            */
            set_EDC_CH1_SCL_HSP_hratio(edc_base, scl_ratio_w);

            set_EDC_CH1_SCL_HSP_hafir_en(edc_base, pipe->edc_ch_info.cap.filter_enable);
            set_EDC_CH1_SCL_HSP_hfir_en(edc_base, pipe->edc_ch_info.cap.filter_enable);
            if (pipe->edc_ch_info.cap.filter_enable) {
                if (dst.w / src.w >= 1) {
                    memcpy(stSCLHorzCoef, gfxcoefficient8_cubic, sizeof(gfxcoefficient8_cubic));
                } else if ((dst.w / src.w == 0) && (((dst.w % src.w) << 12) / src.w) >= 0x800)  {
                    memcpy(stSCLHorzCoef, gfxcoefficient8_lanczos2_8tap, sizeof(gfxcoefficient8_lanczos2_8tap));
                }

                for (i = 0; i < 8; i++) {
                    for (j = 0; j < 4; j++) {
                        set_EDC_CH1_SCL_HPC(edc_base, i, j, (stSCLHorzCoef[i][j*2+1] << 16) |
                        (stSCLHorzCoef[i][j*2] & 0xFFFF));
                    }
                }
            }

            set_EDC_CH1_SCL_VSR_vratio(edc_base, scl_ratio_h);
            set_EDC_CH1_SCL_VSP_vafir_en(edc_base, pipe->edc_ch_info.cap.filter_enable);
            set_EDC_CH1_SCL_VSP_vfir_en(edc_base, pipe->edc_ch_info.cap.filter_enable);
            if (pipe->edc_ch_info.cap.filter_enable) {
                if (dst.h / src.h >= 1) {
                    memcpy(stSCLVertCoef, gfxcoefficient4_cubic, sizeof(gfxcoefficient4_cubic));
                } else if ((dst.h / src.h == 0) && (((dst.h % src.h) << 12) / src.h >= 0xC00)) {
                    memcpy(stSCLVertCoef, gfxcoefficient4_lanczos2_6M_a15, sizeof(gfxcoefficient4_lanczos2_6M_a15));
                } else if ((dst.h / src.h == 0) && (((dst.h % src.h) << 12) / src.h >= 0x800) &&
                    (((dst.h % src.h) << 12) / src.h <= 0xC00)) {
                    memcpy(stSCLVertCoef, gfxcoefficient4_lanczos2_5M_a15, sizeof(gfxcoefficient4_lanczos2_5M_a15));
                }

                for (i = 0; i < 16; i++) {
                    for (j = 0; j < 2; j++) {
                        set_EDC_CH1_SCL_VPC(edc_base, i, j, (stSCLVertCoef[i][j*2+1] << 16) |
                        (stSCLVertCoef[i][j*2] & 0xFFFF));
                    }
                }
            }
        }
    }

#if defined(EDC_CH_CHG_SUPPORT)
    if (ch_chg) {
        //set to edc0
        pipe = edc0_pipe;
        edc_base = edc0_base;

	pipe->set_EDC_CH_XY(edc_base, dst.x, dst.y);
	pipe->set_EDC_CH_SIZE(edc_base, src.w, src.h); //edc0
	pipe->set_EDC_CH_CTL_pix_fmt(edc_base, req->src.format); //edc0
    }
#endif //EDC_CH_CHG_SUPPORT

    if (!req->is_graphic) {
        if (pipe->set_EDC_CH_CSCIDC_csc_en) {
            pipe->set_EDC_CH_CSCIDC_csc_en(edc_base, pipe->edc_ch_info.cap.csc_enable);
        }
        if (pipe->edc_ch_info.cap.csc_enable) {
#if 0
        /* 709 for HD */
            pipe->set_OVC_CH_CSCIDC(edc_base, 0x1f0, 0x180, 0x180);
            pipe->set_OVC_CH_CSCODC(edc_base, 0x0, 0x0, 0x0);
            pipe->set_OVC_CH_CSCP0(edc_base, 0x0, 0x129);
            pipe->set_OVC_CH_CSCP1(edc_base, 0x129, 0x1cb);
            pipe->set_OVC_CH_CSCP2(edc_base, 0x1f78, 0x1fca);
            pipe->set_OVC_CH_CSCP3(edc_base, 0x21c, 0x129);
            pipe->set_OVC_CH_CSCP4(edc_base, 0x0);
#else
            /* 601 for SD */
            pipe->set_OVC_CH_CSCIDC(edc_base, 0x1f0, 0x180, 0x180);
            pipe->set_OVC_CH_CSCODC(edc_base, 0x0, 0x0, 0x0);
            pipe->set_OVC_CH_CSCP0(edc_base, 0x0, 0x129);
            pipe->set_OVC_CH_CSCP1(edc_base, 0x129, 0x198);
            pipe->set_OVC_CH_CSCP2(edc_base, 0x1f30, 0x1f9c);
            pipe->set_OVC_CH_CSCP3(edc_base, 0x204, 0x129);
            pipe->set_OVC_CH_CSCP4(edc_base, 0x0);
#endif
        }
    }else {
        edc_overlay_compose_ch_csc_unset(pipe, edc_base);
    }

    edc_overlay_compose_graphic_alpha_set(req, edc_base);
    k3_fb_gralloc_overlay_save_display_addr(k3fd, req->id, addr);

    if (!cfg_disable) {
        set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);
    }
    return 0;
}

int edc_overlay_compose_pan_display(struct fb_var_screeninfo *var, struct fb_info *info, struct overlay_data *req)
{
    struct edc_overlay_pipe *pipe = NULL;
    struct k3_fb_data_type *k3fd = NULL;
    u32 edc_base = 0;
    u32 display_addr = 0;
    int pipe_id;
    u32 pan_x;
    u32 pan_y;
    u32 pan_w;
    u32 pan_h;

    BUG_ON(var == NULL || info == NULL || info->par == NULL || req == NULL);
    k3fd = (struct k3_fb_data_type *)info->par;

    pipe_id = k3fd->graphic_ch;
    pipe = edc_overlay_ndx2pipe(info, pipe_id);
    if (pipe == NULL) {
        k3fb_loge("id=%d not able to get pipe!\n", pipe_id);
        return -ENODEV;
    }

    pan_x = req->src.partial_dst_rect.x;
    pan_y = req->src.partial_dst_rect.y;
    pan_w = req->src.partial_dst_rect.w;
    pan_h = req->src.partial_dst_rect.h;


    edc_base = pipe->edc_base;
    display_addr = info->fix.smem_start + info->fix.line_length * var->yoffset
        + var->xoffset * (var->bits_per_pixel >> 3);

    /* stride 64byte odd align */
    if (!is64BytesOddAlign(FB_64BYTES_ODD_ALIGN_NONE, info->fix.line_length)) {
        k3fb_logw("stride NOT 64 bytes odd aligned, %x!\n", info->fix.line_length);
        return -EINVAL;
    }

    if (!k3fd->panel_power_on) {
        return 0;
    }

    display_addr += pan_y * info->fix.line_length + pan_x * (var->bits_per_pixel >> 3);
    if (display_addr & 0x0F) {
        k3fb_logw("buf addr NOT 16 bytes aligned, %x!\n", display_addr);
        return -EINVAL;
    }

    if (k3fd->panel_info.s3d_frm != EDC_FRM_FMT_2D) {
        pipe->set_EDC_CHR_ADDR(edc_base, display_addr);
    }
    pipe->set_EDC_CHL_ADDR(edc_base, display_addr);
    pipe->set_EDC_CH_STRIDE(edc_base, info->fix.line_length);
    pipe->set_EDC_CH_XY(edc_base, pan_x, pan_y);
    pipe->set_EDC_CH_SIZE(edc_base, pan_w, pan_h);
    pipe->set_EDC_CH_CTL_pix_fmt(edc_base, k3fd->fb_imgType);
    pipe->set_EDC_CH_CTL_colork_en(edc_base, pipe->edc_ch_info.cap.ckey_enable);
    if (pipe->edc_ch_info.cap.ckey_enable) {
        pipe->set_EDC_CH_COLORK_MIN(edc_base, pipe->edc_ch_info.ckeymin);
        pipe->set_EDC_CH_COLORK_MAX(edc_base, pipe->edc_ch_info.ckeymax);
    }
    pipe->set_EDC_CH_CTL_bgr(edc_base, k3fd->panel_info.bgr_fmt);
    set_EDC_CH12_OVLY_ch2_top(edc_base, EDC_CH2_TOP);
    pipe->set_EDC_CH_CTL_ch_en(edc_base, K3_ENABLE);
    edc_overlay_compose_partial_graphic_alpha_set(req, edc_base);

    set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);
    return 0;
}
#endif //CONFIG_OVERLAY_COMPOSE

int edc_overlay_play(struct fb_info *info, struct overlay_data *req)
{
	struct edc_overlay_pipe *pipe = NULL;
	struct k3_fb_data_type *k3fd = NULL;
	struct overlay_info *ov_info = NULL;
	struct k3fb_rect src;
	struct k3fb_rect dst;
	u32 edc_base = 0;
	int i = 0;
	int j = 0;
	s16 stSCLHorzCoef[8][8] = {{0},};
	s16 stSCLVertCoef[16][4] = {{0},};
	u32 scl_ratio_w = 0;
	u32 scl_ratio_h = 0;
	u32 addr = 0;

	BUG_ON(info == NULL || info->par == NULL || req == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	pipe = edc_overlay_ndx2pipe(info, req->id);
	if (pipe == NULL) {
		k3fb_loge("id=%d not able to get pipe!\n", req->id);
		return -ENODEV;
	}

	/* 16 byte align */
	if (req->src.phy_addr & 0x0F) {
		k3fb_logw("buf NOT 16 bytes aligned, %x.\n", req->src.phy_addr);
		return -EINVAL;
	}

	/* stride 64byte odd align */
	if (!is64BytesOddAlign(FB_64BYTES_ODD_ALIGN_NONE, req->src.stride)) {
		k3fb_logw("stride NOT 64 bytes odd aligned, %x.\n", req->src.stride);
		return -EINVAL;
	}

	if (req->src.actual_width == 0 ||
		req->src.actual_height == 0 ||
		pipe->req_info.dst_rect.w == 0 ||
		pipe->req_info.dst_rect.h == 0) {
		k3fb_logw("invalid width or height!img_actual(%d,%d), dst(%d,%d).\n",
			req->src.actual_width, req->src.actual_height,
			pipe->req_info.dst_rect.w, pipe->req_info.dst_rect.h);
		return -EINVAL;
	}

	edc_base = pipe->edc_base;
	ov_info = &pipe->req_info;

	src.x = 0;
	src.y = 0;
	src.w = req->src.actual_width;
	src.h = req->src.actual_height;
	dst = ov_info->dst_rect;

	addr = computeDisplayAddr(&(req->src), ov_info->rotation, &src);

	if (!ov_info->is_pipe_used)
		return 0;

	down(&k3fd->sem);
	if (!k3fd->panel_power_on) {
		up(&k3fd->sem);
		return 0;
	}

	/* check edc_afifo_underflow_int interrupt */
	if ((inp32(edc_base + LDI_ORG_INT_OFFSET) & 0x4) == 0x4) {
		set_reg(edc_base + LDI_INT_CLR_OFFSET, 0x1, 1, 2);
		k3fb_logw("edc_afifo_underflow_int !!!\n");
	}

	if (req->src.type == EDC_RIGHT_ADDR) {
		pipe->set_EDC_CHR_ADDR(edc_base, addr);
	} else if (req->src.type == EDC_LEFT_ADDR) {
		pipe->set_EDC_CHL_ADDR(edc_base, addr);
	} else {
		pipe->set_EDC_CHL_ADDR(edc_base, addr);
		pipe->set_EDC_CHR_ADDR(edc_base, addr);
	}
	pipe->set_EDC_CH_XY(edc_base, dst.x, dst.y);
	pipe->set_EDC_CH_SIZE(edc_base, req->src.actual_width, req->src.actual_height);
	pipe->set_EDC_CH_STRIDE(edc_base, req->src.stride);
	pipe->set_EDC_CH_CTL_pix_fmt(edc_base, req->src.format);
	pipe->set_EDC_CH_CTL_bgr(edc_base, k3fd->panel_info.bgr_fmt);
	pipe->set_EDC_CH_CTL_ch_en(edc_base, K3_ENABLE);

	pipe->set_EDC_CH_CTL_colork_en(edc_base, pipe->edc_ch_info.cap.ckey_enable);
	if (pipe->edc_ch_info.cap.ckey_enable) {
		pipe->set_EDC_CH_COLORK_MIN(edc_base, pipe->edc_ch_info.ckeymin);
		pipe->set_EDC_CH_COLORK_MAX(edc_base, pipe->edc_ch_info.ckeymax);
	}

	if (pipe->pipe_type == OVERLAY_TYPE_CHCAP_ALL) {
		set_EDC_CH1_CTL_rot(edc_base, ov_info->rotation);

		if (src.w == dst.w && src.h == dst.h) {
			set_EDC_CH1_SCL_HSP_hsc_en(edc_base, K3_DISABLE);
			set_EDC_CH1_SCL_VSP_vsc_en(edc_base, K3_DISABLE);
		} else {
			set_EDC_CH1_SCL_HSP_hsc_en(edc_base, K3_ENABLE);
			set_EDC_CH1_SCL_VSP_vsc_en(edc_base, K3_ENABLE);

			set_EDC_CH1_SCL_IRES(edc_base, src.w, src.h);
			set_EDC_CH1_SCL_ORES(edc_base, dst.w, dst.h);

			scl_ratio_w = (src.w << 12) / dst.w;
			scl_ratio_h = (src.h << 12) / dst.h;
            /*
			if ((scl_ratio_w > ((10<<12)/7)) && (scl_ratio_h < (1<<12))) {
				scl_ratio_w = ((10<<12)/7);
			} else if ((scl_ratio_h > ((10<<12)/7)) && (scl_ratio_w < (1<<12))) {
				scl_ratio_h = ((10<<12)/7);
			}
            */
			set_EDC_CH1_SCL_HSP_hratio(edc_base, scl_ratio_w);

			set_EDC_CH1_SCL_HSP_hafir_en(edc_base, pipe->edc_ch_info.cap.filter_enable);
			set_EDC_CH1_SCL_HSP_hfir_en(edc_base, pipe->edc_ch_info.cap.filter_enable);
			if (pipe->edc_ch_info.cap.filter_enable) {
				if (dst.w / src.w >= 1) {
					memcpy(stSCLHorzCoef, gfxcoefficient8_cubic, sizeof(gfxcoefficient8_cubic));
				} else if ((dst.w / src.w == 0) && (((dst.w % src.w) << 12) / src.w) >= 0x800)  {
					memcpy(stSCLHorzCoef, gfxcoefficient8_lanczos2_8tap, sizeof(gfxcoefficient8_lanczos2_8tap));
				}

				for (i = 0; i < 8; i++) {
					for (j = 0; j < 4; j++) {
						set_EDC_CH1_SCL_HPC(edc_base, i, j, (stSCLHorzCoef[i][j*2+1] << 16) |
							(stSCLHorzCoef[i][j*2] & 0xFFFF));
					}
				}
			}

			set_EDC_CH1_SCL_VSR_vratio(edc_base, scl_ratio_h);
			set_EDC_CH1_SCL_VSP_vafir_en(edc_base, pipe->edc_ch_info.cap.filter_enable);
			set_EDC_CH1_SCL_VSP_vfir_en(edc_base, pipe->edc_ch_info.cap.filter_enable);
			if (pipe->edc_ch_info.cap.filter_enable) {
				if (dst.h / src.h >= 1) {
					memcpy(stSCLVertCoef, gfxcoefficient4_cubic, sizeof(gfxcoefficient4_cubic));
				} else if ((dst.h / src.h == 0) && (((dst.h % src.h) << 12) / src.h >= 0xC00)) {
					memcpy(stSCLVertCoef, gfxcoefficient4_lanczos2_6M_a15, sizeof(gfxcoefficient4_lanczos2_6M_a15));
				} else if ((dst.h / src.h == 0) && (((dst.h % src.h) << 12) / src.h >= 0x800) &&
					(((dst.h % src.h) << 12) / src.h <= 0xC00)) {
					memcpy(stSCLVertCoef, gfxcoefficient4_lanczos2_5M_a15, sizeof(gfxcoefficient4_lanczos2_5M_a15));
				}

				for (i = 0; i < 16; i++) {
					for (j = 0; j < 2; j++) {
						set_EDC_CH1_SCL_VPC(edc_base, i, j, (stSCLVertCoef[i][j*2+1] << 16) |
							(stSCLVertCoef[i][j*2] & 0xFFFF));
					}
				}
			}
		}
	}

	if (!req->is_graphic) {
		pipe->set_EDC_CH_CSCIDC_csc_en(edc_base, pipe->edc_ch_info.cap.csc_enable);
		if (pipe->edc_ch_info.cap.csc_enable) {
		#if 1
			/* 709 for HD */
			pipe->set_EDC_CH_CSCIDC(edc_base, 0x0fc30180);
			pipe->set_EDC_CH_CSCODC(edc_base, 0x00000000);
			pipe->set_EDC_CH_CSCP0(edc_base, 0x00000100);
			pipe->set_EDC_CH_CSCP1(edc_base, 0x0100018a);
			pipe->set_EDC_CH_CSCP2(edc_base, 0x1f8b1fd2);
			pipe->set_EDC_CH_CSCP3(edc_base, 0x01d00100);
			pipe->set_EDC_CH_CSCP4(edc_base, 0x00000000);
		#else
			/* 601 for SD */
			pipe->set_EDC_CH_CSCIDC(edc_base, 0x0fc30180);
			pipe->set_EDC_CH_CSCODC(edc_base, 0x00000000);
			pipe->set_EDC_CH_CSCP0(edc_base, 0x00000100);
			pipe->set_EDC_CH_CSCP1(edc_base, 0x0100015e);
			pipe->set_EDC_CH_CSCP2(edc_base, 0x1faa1f4e);
			pipe->set_EDC_CH_CSCP3(edc_base, 0x01bb0100);
			pipe->set_EDC_CH_CSCP4(edc_base, 0x00000000);
		#endif
		}

		if (isAlphaRGBType(k3fd->fb_imgType)) {
			set_EDC_CH12_OVLY_alp_blend_en(edc_base, pipe->edc_ch_info.cap.alpha_enable);
			if (pipe->edc_ch_info.cap.alpha_enable) {
				set_EDC_CH12_OVLY_alp_src(edc_base, pipe->edc_ch_info.alp_src);
				if (pipe->edc_ch_info.alp_src == EDC_ALP_GLOBAL) {
					set_EDC_CH12_GLB_ALP_VAL(edc_base, pipe->edc_ch_info.alpha0, pipe->edc_ch_info.alpha1);
				} else {
					if (k3fd->graphic_ch == OVERLAY_PIPE_EDC0_CH2 ||
						k3fd->graphic_ch == OVERLAY_PIPE_EDC1_CH2) {
						set_EDC_CH12_OVLY_pix_alp_src(edc_base, EDC_PIX_ALP_SRC_CH2);
						set_EDC_CH12_OVLY_ch1_alp_sel(edc_base, EDC_ALP_MUL_COEFF_1);
						set_EDC_CH12_OVLY_ch2_alp_sel(edc_base, EDC_ALP_MUL_COEFF_0);
					} else {
						set_EDC_CH12_OVLY_pix_alp_src(edc_base, EDC_PIX_ALP_SRC_CH1);
						set_EDC_CH12_OVLY_ch1_alp_sel(edc_base, EDC_ALP_MUL_COEFF_0);
						set_EDC_CH12_OVLY_ch2_alp_sel(edc_base, EDC_ALP_MUL_COEFF_1);
					}
				}
			}
		} else {
			set_EDC_CH12_OVLY_alp_blend_en(edc_base, pipe->edc_ch_info.cap.alpha_enable);
			if (pipe->edc_ch_info.cap.alpha_enable) {
				set_EDC_CH12_OVLY_alp_src(edc_base, EDC_ALP_GLOBAL);
				set_EDC_CH12_GLB_ALP_VAL(edc_base, pipe->edc_ch_info.alpha0, pipe->edc_ch_info.alpha1);
			}
		}
	}
	set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);
	up(&k3fd->sem);

	return 0;
}

int edc_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info, int id)
{
	struct edc_overlay_pipe *pipe = NULL;
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;
	u32 display_addr = 0;
#if defined(CONFIG_OVERLAY_COMPOSE)
	u32 pre_type;
	u32 pre_count;
#endif

	BUG_ON(var == NULL || info == NULL || info->par == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;

#if defined(CONFIG_OVERLAY_COMPOSE)
	pre_type = k3fd->ovc_type;
	k3fd->ovc_type = OVC_NONE;
	pre_count = k3fd->ovc_ch_count;
	k3fd->ovc_ch_count = 1;
#endif /* CONFIG_OVERLAY_COMPOSE */

	pipe = edc_overlay_ndx2pipe(info, id);
	if (pipe == NULL) {
		k3fb_loge("id=%d not able to get pipe!\n", id);
		return -ENODEV;
	}

	edc_base = pipe->edc_base;
	display_addr = info->fix.smem_start + info->fix.line_length * var->yoffset
		+ var->xoffset * (var->bits_per_pixel >> 3);

	if (display_addr & 0x0F) {
		k3fb_logw("buf addr NOT 16 bytes aligned, %x!\n", display_addr);
		return -EINVAL;
	}

	/* stride 64byte odd align */
	if (!is64BytesOddAlign(FB_64BYTES_ODD_ALIGN_NONE, info->fix.line_length)) {
		k3fb_logw("stride NOT 64 bytes odd aligned, %x!\n", info->fix.line_length);
		return -EINVAL;
	}

	down(&k3fd->sem);
	if (!k3fd->panel_power_on) {
		up(&k3fd->sem);
		return 0;
	}

#if defined(CONFIG_OVERLAY_COMPOSE)
	edc_overlay_compose_pipe_unset_previous(info, pre_type, pre_count);
#if defined(EDC_CH_CHG_SUPPORT)
	edc_overlay_compose_ch_chg_disable(k3fd, info);
#endif //EDC_CH_CHG_SUPPORT
	pipe->set_EDC_CH_CTL_bgr(edc_base, k3fd->panel_info.bgr_fmt);
	set_EDC_CH12_OVLY_ch2_top(edc_base, EDC_CH2_TOP);
#endif //CONFIG_OVERLAY_COMPOSE

	if (k3fd->panel_info.s3d_frm != EDC_FRM_FMT_2D) {
		pipe->set_EDC_CHR_ADDR(edc_base, display_addr);
	}
	pipe->set_EDC_CHL_ADDR(edc_base, display_addr);
	pipe->set_EDC_CH_STRIDE(edc_base, info->fix.line_length);
	pipe->set_EDC_CH_XY(edc_base, 0, 0);
	pipe->set_EDC_CH_SIZE(edc_base, k3fd->panel_info.xres, k3fd->panel_info.yres);
	pipe->set_EDC_CH_CTL_pix_fmt(edc_base, k3fd->fb_imgType);
	pipe->set_EDC_CH_CTL_colork_en(edc_base, pipe->edc_ch_info.cap.ckey_enable);
	if (pipe->edc_ch_info.cap.ckey_enable) {
		pipe->set_EDC_CH_COLORK_MIN(edc_base, pipe->edc_ch_info.ckeymin);
		pipe->set_EDC_CH_COLORK_MAX(edc_base, pipe->edc_ch_info.ckeymax);
	}
	pipe->set_EDC_CH_CTL_ch_en(edc_base, K3_ENABLE);
	set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);
	up(&k3fd->sem);

	return 0;
}

int edc_fb_suspend(struct fb_info *info)
{
	struct edc_overlay_pipe *pipe = NULL;
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;
	unsigned long dw_jiffies = 0;
	u32 tmp = 0;

	BUG_ON(info == NULL || info->par == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	pipe = edc_overlay_ndx2pipe(info, k3fd->graphic_ch);
	if (pipe == NULL) {
		k3fb_loge("id=%d not able to get pipe!\n", k3fd->graphic_ch);
		return -ENODEV;
	}

	edc_base = pipe->edc_base;

	down(&k3fd->sem);
	pipe->set_EDC_CH_CTL_ch_en(edc_base, K3_DISABLE);
	/* mask edc int and clear int state */
	set_EDC_INTE(edc_base, 0xFFFFFFFF);
	set_EDC_INTS(edc_base, 0x0);
	/* disable edc */
	set_EDC_DISP_CTL_edc_en(edc_base, K3_DISABLE);
	if (k3fd->panel_info.sbl_enable) {
		/* disable sbl */
		set_EDC_DISP_DPD_sbl_en(edc_base, K3_DISABLE);
	}
	/* edc cfg ok */
	set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);

	/* check outstanding */
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(edc_base + EDC_STS_OFFSET);
		if ((tmp & 0x80000000) == 0x80000000) {
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	/* edc clock gating */
	clk_disable(k3fd->edc_clk);
	if (k3fd->index == 0) {
		/* edc clock rst gating*/
		clk_disable(k3fd->edc_clk_rst);
	} else if (k3fd->index == 1) {
		/* edc1 vcc */
		if (regulator_disable(k3fd->edc_vcc) != 0) {
			k3fb_loge("failed to disable edc-vcc regulator.\n");
		}
	} else {
		k3fb_loge("fb%d not support now!\n", k3fd->index);
	}

#if defined(CONFIG_OVERLAY_COMPOSE)
	k3_fb_overlay_compose_data_clear(k3fd);
#endif

	up(&k3fd->sem);

	return 0;
}

int edc_fb_resume(struct fb_info *info)
{
	struct edc_overlay_pipe *pipe = NULL;
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;

	BUG_ON(info == NULL || info->par == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	pipe = edc_overlay_ndx2pipe(info, k3fd->graphic_ch);
	if (pipe == NULL) {
		k3fb_loge("id=%d not able to get pipe!\n", k3fd->graphic_ch);
		return -ENODEV;
	}

	edc_base = k3fd->edc_base;

	down(&k3fd->sem);

	if (k3fd->index == 0) {
		/* enable edc clk rst */
		if (clk_enable(k3fd->edc_clk_rst) != 0) {
			k3fb_loge("failed to enable edc clock rst.\n");
		}
	} else if (k3fd->index == 1) {
		/* edc1 vcc */
		if (regulator_enable(k3fd->edc_vcc) != 0) {
			k3fb_loge("failed to enable edc-vcc regulator.\n");
		}
	} else {
		k3fb_loge("fb%d not support now!\n", k3fd->index);
	}

	/* enable edc clk */
	if (clk_enable(k3fd->edc_clk) != 0) {
		k3fb_loge("failed to enable edc clock.\n");
	}

	/* Note: call clk_set_rate after clk_set_rate, set edc clock rate to normal value */
	if (clk_set_rate(k3fd->edc_clk, EDC_CLK_RATE_GET(k3fd->panel_info.clk_rate)) != 0) {
		k3fb_loge("failed to set edc clk rate(%d).\n", EDC_CLK_RATE_GET(k3fd->panel_info.clk_rate));
	}

	/* edc init */
	pipe->set_EDC_CH_CTL_secu_line(edc_base, EDC_CH_SECU_LINE);
	pipe->set_EDC_CH_CTL_bgr(edc_base, k3fd->panel_info.bgr_fmt);
	set_EDC_INTE(k3fd->edc_base, 0xFFFFFFFF);
	set_EDC_INTS(k3fd->edc_base, 0x0);
	set_EDC_INTE(k3fd->edc_base, 0xFFFFFF3F);
	set_EDC_DISP_DPD_disp_dpd(edc_base, 0x0);
	set_EDC_DISP_SIZE(edc_base, k3fd->panel_info.xres, k3fd->panel_info.yres);
	set_EDC_DISP_CTL_pix_fmt(edc_base, k3fd->panel_info.bpp);
	set_EDC_DISP_CTL_frm_fmt(edc_base, k3fd->panel_info.s3d_frm);
	set_EDC_DISP_CTL_endian(edc_base, EDC_ENDIAN_LITTLE);
	if (isNeedDither(k3fd)) {
		set_EDC_DISP_CTL_dither_en(edc_base, 1);
	} else {
		set_EDC_DISP_CTL_dither_en(edc_base, 0);
	}
	set_EDC_DISP_CTL_nrot_burst(edc_base, EDC_BURST8);
	set_EDC_DISP_CTL_crg_gt_en(edc_base, K3_ENABLE);

#if defined(CONFIG_OVERLAY_COMPOSE)
	set_EDC_DISP_CTL_outstding_dep(edc_base, 15);
#else
	set_EDC_DISP_CTL_outstding_dep(edc_base, 8);
#endif

	/*Set unflow_lev = 3072 for edc0 and unflow_lev = 640 for edc1*/
	if (k3fd->index == 0) {
		set_EDC_DISP_CTL_unflow_lev(edc_base, 0xC00);
	} else {
		set_EDC_DISP_CTL_unflow_lev(edc_base, 0x280);
	}

	set_EDC_DISP_CTL_edc_en(edc_base, K3_ENABLE);
	set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);

	up(&k3fd->sem);

	/* This spinlock will be UNLOCK in mipi_dsi_on */
	if (k3fd->index == 0) {
		if (k3fd->panel_info.frc_enable) {
			k3fd->panel_info.frame_rate = 60;
			k3fd->frc_threshold_count = 0;
			k3fd->frc_state = K3_FB_FRC_NONE_PLAYING;
			k3fd->frc_timestamp = jiffies;
		}

		if (k3fd->panel_info.esd_enable) {
			k3fd->esd_timestamp = jiffies;
			k3fd->esd_frame_count = 0;
		}
	}

	return 0;
}
extern bool sbl_low_power_mode;
int sbl_bkl_set(struct k3_fb_data_type *k3fd, u32 value)
{
	u32 tmp = 0;

	BUG_ON(k3fd == NULL);

	if (!k3fd->panel_power_on)
		return 0;

	tmp = inp32(k3fd->edc_base + EDC_DISP_DPD_OFFSET);
	if ((tmp & REG_SBL_EN) == REG_SBL_EN) {
		if(sbl_low_power_mode)
			k3fd->bl_level =  SBL_REDUCE_VALUE(value);
		else
			k3fd->bl_level = value;
		set_SBL_BKL_LEVEL_L_bkl_level_l(k3fd->edc_base, k3fd->bl_level);
	}

	return 0;
}

int sbl_ctrl_set(struct k3_fb_data_type *k3fd)
{
	u32 tmp = 0;
	u32 edc_base = 0;
	u32 bkl_value = 0;
	struct k3_fb_panel_data *pdata = NULL;

	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;

	pdata = (struct k3_fb_panel_data *)k3fd->pdev->dev.platform_data;

	tmp = inp32(edc_base + EDC_DISP_DPD_OFFSET);
	if (K3_DISABLE == k3fd->sbl_enable) {
		if ((tmp & REG_SBL_EN) == REG_SBL_EN) {
			set_EDC_DISP_DPD_sbl_en(edc_base, K3_DISABLE);
			set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);
			
			if (pdata && pdata->set_cabc) {
				pdata->set_cabc(k3fd->pdev, K3_ENABLE);
			}
			k3_fb_set_backlight(k3fd, k3fd->bl_level_sbl);
		}
	} else {
		if ((tmp & REG_SBL_EN) != REG_SBL_EN) {
			if(sbl_low_power_mode)
				bkl_value =  SBL_REDUCE_VALUE(k3fd->bl_level_sbl);
			else
				bkl_value = k3fd->bl_level_sbl;
			set_SBL_BKL_LEVEL_L_bkl_level_l(edc_base, bkl_value);
            if ((k3fd->panel_info.type != PANEL_MIPI_CMD)
              || (k3fd->cmd_bl_can_set))
			    k3_fb_set_backlight(k3fd, bkl_value);
			set_EDC_DISP_DPD_sbl_en(edc_base, K3_ENABLE);
			set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);
			
			outp32(edc_base + MIPIDSI_GEN_HDR_OFFSET, 0x0cbb23);
			if (pdata && pdata->set_cabc) {
				pdata->set_cabc(k3fd->pdev, K3_DISABLE);
			}
		}
	}
	return 0;
}
static unsigned short apical_rgb_table[] = {
	0x20, 0xff, 0x0f,
	0x1f, 0xb3, 0x0f,
	0x1e, 0x64, 0x0f,
	0x1d, 0x13, 0x0f,
	0x1c, 0xc0, 0x0e,
	0x1b, 0x6b, 0x0e,
	0x1a, 0x13, 0x0e,
	0x19, 0xb9, 0x0d,
	0x18, 0x5c, 0x0d,
	0x17, 0xfd, 0x0c,
	0x16, 0x9b, 0x0c,
	0x15, 0x36, 0x0c,
	0x14, 0xce, 0x0b,
	0x13, 0x63, 0x0b,
	0x12, 0xf4, 0x0a,
	0x11, 0x82, 0x0a,
	0x10, 0x0c, 0x0a,
	0x0f, 0x93, 0x09,
	0x0e, 0x15, 0x09,
	0x0d, 0x93, 0x08,
	0x0c, 0x0d, 0x08,
	0x0b, 0x82, 0x07,
	0x0a, 0xf3, 0x06,
	0x09, 0x5e, 0x06,
	0x08, 0xc3, 0x05,
	0x07, 0x23, 0x05,
	0x06, 0x7c, 0x04,
	0x05, 0xcf, 0x03,
	0x04, 0x1c, 0x03,
	0x03, 0x61, 0x02,
	0x02, 0x9e, 0x01,
	0x01, 0xd3, 0x00,
	0x00, 0x00, 0x00,
};

extern int apical_flags;
int sbl_ctrl_resume(struct k3_fb_data_type *k3fd)
{
	u32 tmp = 0;
	u32 edc_base = 0;
	u32 frame_width_l = 0;
	u32 frame_width_h = 0;
	u32 frame_height_l = 0;
	u32 frame_height_h = 0;

	unsigned int count = 0;
	unsigned int table_size;
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;
	
	frame_width_l = (k3fd->panel_info.xres) & 0xff;
	frame_width_h = (k3fd->panel_info.xres >> 8) & 0xff;
	frame_height_l = (k3fd->panel_info.yres) & 0xff;
	frame_height_h = (k3fd->panel_info.yres >> 8) & 0xff;

	set_SBL_FRAME_WIDTH_L_frame_width_l(edc_base, frame_width_l);
	set_SBL_FRAME_WIDTH_H_frame_width_h(edc_base, frame_width_h);
	set_SBL_FRAME_HEIGHT_L_frame_height_l(edc_base, frame_height_l);
	set_SBL_FRAME_HEIGHT_H_frame_height_h(edc_base, frame_height_h);

	set_reg(edc_base + SBL_CTRL_REG0_OFFSET, 0x0b, 8, 0);
	set_reg(edc_base + SBL_CTRL_REG1_OFFSET, 0x22, 8, 0);
	set_reg(edc_base + SBL_HS_POS_LOFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_HS_POS_HOFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_VS_POS_LOFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_VS_POS_HOFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_IRIDIX_CTRL0_OFFSET, 0x07, 8, 0);
	set_reg(edc_base + SBL_IRIDIX_CTRL1_OFFSET, 0x46, 8, 0);
	set_reg(edc_base + SBL_VARIANCE_OFFSET, 0x41, 8, 0);
#if 0
	set_reg(edc_base + SBL_SLOPE_MAX_OFFSET, 0x30, 8, 0);
#else
	set_reg(edc_base + SBL_SLOPE_MAX_OFFSET, 0x3c, 8, 0);
#endif
	set_reg(edc_base + SBL_SLOPE_MIN_OFFSET, 0x80, 8, 0);
#if 0
	set_reg(edc_base + SBL_BLACK_LEVEL_LOFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_BLACK_LEVEL_HOFFSET, 0x0, 8, 0);
#else
	set_reg(edc_base + SBL_BLACK_LEVEL_LOFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_BLACK_LEVEL_HOFFSET, 0x0, 8, 0);
#endif
	set_reg(edc_base + SBL_WHITE_LEVEL_LOFFSET, 0xff, 8, 0);
	set_reg(edc_base + SBL_WHITE_LEVEL_HOFFSET, 0x03, 8, 0);
	set_reg(edc_base + SBL_LIMIT_AMP_OFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_DITHER_OFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_LOGO_LEFT_OFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_LOGO_RIGHT_OFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_DITHER_CTRL_OFFSET, 0x03, 8, 0);
#if 0
	set_reg(edc_base + SBL_STRENGTH_SEL_OFFSET, 0x0, 8, 0);
#else
	set_reg(edc_base + SBL_STRENGTH_SEL_OFFSET, 0x01, 8, 0);
#endif
	set_reg(edc_base + SBL_STRENGTH_LIMIT_OFFSET, k3fd->panel_info.sbl.str_limit, 8, 0);
	if(apical_flags == 0x80)
		set_reg(edc_base + SBL_STRENGTH_MANUAL_OFFSET, 0x80, 8, 0);
	else if(apical_flags == 0x20)
		set_reg(edc_base + SBL_STRENGTH_MANUAL_OFFSET, 0x20, 8, 0);
	else if(apical_flags == 0x0)
		set_reg(edc_base + SBL_STRENGTH_MANUAL_OFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_OPTION_SEL_OFFSET, 0x02, 8, 0);

	set_reg(edc_base + SBL_BKL_MAX_LOFFSET, k3fd->panel_info.sbl.bl_max, 8, 0);
	set_reg(edc_base + SBL_BKL_MAX_HOFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_CALIBRATION_A_OFFSET, k3fd->panel_info.sbl.cal_a, 8, 0);
	set_reg(edc_base + SBL_CALIBRATION_B_OFFSET, k3fd->panel_info.sbl.cal_b, 8, 0);
	set_reg(edc_base + SBL_DRC_IN_LOFFSET, 0x87, 8, 0);
	set_reg(edc_base + SBL_DRC_IN_HOFFSET, 0xba, 8, 0);
	set_reg(edc_base + SBL_T_FILT_CTRL_OFFSET, 0x0, 8, 0);

	set_reg(edc_base + SBL_BKL_LEVEL_HOFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_AMBIENT_LIGHT_LOFFSET, 0x0, 8, 0);
	set_reg(edc_base + SBL_AMBIENT_LIGHT_HOFFSET, 0x2, 8, 0);
	set_reg(edc_base + SBL_START_CALC_OFFSET, 0x01, 8, 0);

    table_size = sizeof(apical_rgb_table)/sizeof(unsigned short);
	while(count < table_size)
	{
		//printk("%s:apical_rgb_table: 0x%x, 0x%x, 0x%x\n", __func__, apical_rgb_table[count], apical_rgb_table[count+1], apical_rgb_table[count+2]);
		set_reg(edc_base + SBL_OFFSET + 0x480, apical_rgb_table[count], 8, 0);
		set_reg(edc_base + SBL_OFFSET + 0x484, apical_rgb_table[count+1], 8, 0);
		set_reg(edc_base + SBL_OFFSET + 0x488, apical_rgb_table[count+2], 8, 0);
		count += 3;
	}
	tmp = inp32(edc_base + EDC_DISP_DPD_OFFSET);
	if ((K3_ENABLE == k3fd->sbl_enable) && ((tmp & REG_SBL_EN) != REG_SBL_EN)) {
		if(sbl_low_power_mode)
			k3fd->bl_level =  SBL_REDUCE_VALUE(k3fd->bl_level_sbl);
		else
			k3fd->bl_level = k3fd->bl_level_sbl;
		set_SBL_BKL_LEVEL_L_bkl_level_l(edc_base, k3fd->bl_level);
		set_EDC_DISP_DPD_sbl_en(edc_base, K3_ENABLE);
		set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);
		outp32(edc_base + MIPIDSI_GEN_HDR_OFFSET, 0x0cbb23);
	}

	return 0;
}

int edc_fb_disable(struct fb_info *info)
{
	struct edc_overlay_pipe *pipe = NULL;
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;

	BUG_ON(info == NULL || info->par == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	pipe = edc_overlay_ndx2pipe(info, k3fd->graphic_ch);
	if (pipe == NULL) {
		k3fb_loge("id=%d not able to get pipe!\n", k3fd->graphic_ch);
		return -ENODEV;
	}

	edc_base = pipe->edc_base;

	down(&k3fd->sem);
	pipe->set_EDC_CH_CTL_ch_en(edc_base, K3_DISABLE);
	/* disable edc */
	set_EDC_DISP_CTL_edc_en(edc_base, K3_DISABLE);
	/* edc cfg ok */
	set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);

	up(&k3fd->sem);

	return 0;
}

int edc_fb_enable(struct fb_info *info)
{
	struct edc_overlay_pipe *pipe = NULL;
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;

	BUG_ON(info == NULL || info->par == NULL);
	k3fd = (struct k3_fb_data_type *)info->par;
	pipe = edc_overlay_ndx2pipe(info, k3fd->graphic_ch);
	if (pipe == NULL) {
		k3fb_loge("id=%d not able to get pipe!\n", k3fd->graphic_ch);
		return -ENODEV;
	}

	edc_base = pipe->edc_base;

	down(&k3fd->sem);

	pipe->set_EDC_CH_CTL_ch_en(edc_base, K3_DISABLE);
	/* disable edc */
	set_EDC_DISP_CTL_edc_en(edc_base, K3_ENABLE);
	/* edc cfg ok */
	set_EDC_DISP_CTL_cfg_ok(edc_base, EDC_CFG_OK_YES);

	up(&k3fd->sem);

	return 0;
}
