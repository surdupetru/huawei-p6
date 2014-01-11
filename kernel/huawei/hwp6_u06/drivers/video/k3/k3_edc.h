/* include/linux/k3_edc.h
 *
 * Copyright (c) 2008-2011, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef _K3_EDC_H_
#define _K3_EDC_H_

#include <linux/types.h>
#include <linux/fb.h>

#define K3FB_IOCTL_MAGIC 'm'
#define K3FB_CURSOR _IOW(K3FB_IOCTL_MAGIC, 130, struct fb_cursor)
#define K3FB_TIMING_SET _IOW(K3FB_IOCTL_MAGIC, 131, struct fb_var_screeninfo)
#define K3FB_VSYNC_INT_SET _IOW(K3FB_IOCTL_MAGIC, 132, unsigned int)

#define K3FB_OVERLAY_GET	_IOR(K3FB_IOCTL_MAGIC, 135, struct overlay_info)
#define K3FB_OVERLAY_SET	_IOWR(K3FB_IOCTL_MAGIC, 136, struct overlay_info)
#define K3FB_OVERLAY_UNSET	_IOW(K3FB_IOCTL_MAGIC, 137, unsigned int)
#define K3FB_OVERLAY_PLAY	_IOW(K3FB_IOCTL_MAGIC, 138, struct overlay_data)

#define K3FB_FRC_SET		_IOW(K3FB_IOCTL_MAGIC, 139, char *)
#define K3FB_G2D_SET_FREQ	_IOW(K3FB_IOCTL_MAGIC, 140, char *)
#define K3FB_SBL_SET_VALUE	_IOW(K3FB_IOCTL_MAGIC, 141, int)

#define K3FB_OVC_CHECK_DDR_FREQ _IOW(K3FB_IOCTL_MAGIC, 142, char *) /* CONFIG_OVERLAY_COMPOSE */

enum {
	K3_DISABLE = 0,
	K3_ENABLE,
};

enum {
	EDC_ARGB_1555 = 0,
	EDC_RGB_565,
	EDC_XRGB_8888,
	EDC_ARGB_8888,
	EDC_YUYV_I,
	EDC_UYVY_I,
	EDC_YVYU_I,
	EDC_VYUY_I,
};

enum {
	EDC_OUT_RGB_565 = 0,
	EDC_OUT_RGB_666,
	EDC_OUT_RGB_888,
};

enum {
	EDC_ROT_NOP = 0,
	EDC_ROT_180,
	EDC_ROT_90,
	EDC_ROT_270,

};

enum {
	EDC_RGB = 0,
	EDC_BGR,
};

enum {
	EDC_ENDIAN_LITTLE = 0,
	EDC_ENDIAN_BIG,
};

enum {
	EDC_BURST8 = 0,
	EDC_BURST16,
};

enum {
	EDC_CFG_OK_NO = 0,
	EDC_CFG_OK_YES,
};

enum {
	EDC_CFG_OK_EDC2 = 0,
	EDC_CFG_OK_EDC1,
};

enum {
	EDC_LINEAR = 0,
	EDC_TILE,
};

enum {
	EDC_TILE_FMT_TITLE = 0,
	EDC_TILE_FMT_SUPPER_TITLE,
	EDC_TILE_FMT_MULTI_TITLE,
	EDC_TILE_FMT_SUPPERMULTI_TITLE,
};

enum {
	EDC_ALP_GLOBAL = 0,
	EDC_ALP_PIXEL,
};

enum {
	EDC_PIX_ALP_SRC_CH2 = 0,
	EDC_PIX_ALP_SRC_CH1,
};

enum {
	EDC_ALP_MUL_COEFF_0 = 0,	/* alpha */
	EDC_ALP_MUL_COEFF_1,		/* 1-alpha */
	EDC_ALP_MUL_COEFF_2,		/* 0 */
	EDC_ALP_MUL_COEFF_3,		/* 1 */
};

enum {
	EDC_CH2_BOTTOM = 0,
	EDC_CH2_TOP,
};

enum {
	EDC_FRM_FMT_2D = 0,
	EDC_FRM_FMT_3D_SBS,
	EDC_FRM_FMT_3D_TTB,
	EDC_FRM_FMT_3D_CBC,
	EDC_FRM_FMT_3D_LBL,
	EDC_FRM_FMT_3D_QUINCUNX,
	EDC_FRM_FMT_3D_FBF,
};

enum {
	LDI_DISP_MODE_NOT_3D_FBF = 0,
	LDI_DISP_MODE_3D_FBF,
};

enum {
	LDI_TEST = 0,
	LDI_WORK,
};


/******************************************************************************/

enum {
	EDC_OVERLAY_ROTATION_DEG = 1,
	EDC_OVERLAY_DITHER = 3,
	EDC_OVERLAY_TRANSFORM = 4,
};

enum {
	EDC_LEFT_ADDR = 0,
	EDC_RIGHT_ADDR,
	EDC_ALL_ADDR,
};

struct k3fb_rect {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

struct k3fb_img {
	uint32_t type;
	uint32_t phy_addr;
	uint32_t actual_width;
	uint32_t actual_height;
	uint32_t format;
	uint32_t s3d_format;
	uint32_t stride;
	uint32_t width;
	uint32_t height;
	uint32_t is_video;
	/* CONFIG_OVERLAY_COMPOSE start */
	uint32_t bgr_fmt;
	int32_t blending;
	struct k3fb_rect partial_dst_rect; /* partial ovc */
	uint32_t partial_format;
	int32_t partial_blending; /* partial ovc */
	/* CONFIG_OVERLAY_COMPOSE end */
};

struct overlay_data {
	uint32_t id;
	uint32_t is_graphic;
	struct k3fb_img src;
};

struct overlay_info {
	struct k3fb_rect src_rect;
	struct k3fb_rect dst_rect;
	uint32_t id;
	uint32_t is_pipe_used;
	uint32_t rotation;
	uint32_t dither;
	uint32_t hd_enable;
	/* CONFIG_OVERLAY_COMPOSE start */
	uint32_t cfg_disable;
	uint32_t is_overlay_compose;
	uint32_t need_unset;
	/* CONFIG_OVERLAY_COMPOSE end */
};


#endif /*_K3_EDC_H_*/
