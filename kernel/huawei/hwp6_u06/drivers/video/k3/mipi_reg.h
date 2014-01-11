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

#ifndef MIPI_REG_H
#define MIPI_REG_H

#include "k3_edc.h"
#include "k3_fb_def.h"


#define MIPIDSI_OFFSET				(0x900)
#define MIPIDSI_OFFSET_2			(0x800)
#define MIPIDSI_VERSION_OFFSET		(MIPIDSI_OFFSET + 0x0)
#define MIPIDSI_PWR_UP_OFFSET		(MIPIDSI_OFFSET + 0x4)
#define MIPIDSI_CLKMGR_CFG_OFFSET	(MIPIDSI_OFFSET + 0x8)
#define MIPIDSI_DPI_CFG_OFFSET		(MIPIDSI_OFFSET + 0xC)
#define MIPIDSI_DBI_CFG_OFFSET		(MIPIDSI_OFFSET + 0x10)
#define MIPIDSI_DBI_CMDSIZE_OFFSET	(MIPIDSI_OFFSET + 0x14)
#define MIPIDSI_PCKHDL_CFG_OFFSET	(MIPIDSI_OFFSET + 0x18)
#define MIPIDSI_VID_MODE_CFG_OFFSET	(MIPIDSI_OFFSET + 0x1C)
#define MIPIDSI_VID_PKT_CFG_OFFSET	(MIPIDSI_OFFSET + 0x20)
#define MIPIDSI_CMD_MODE_CFG_OFFSET	(MIPIDSI_OFFSET + 0x24)
#define MIPIDSI_TMR_LINE_CFG_OFFSET	(MIPIDSI_OFFSET + 0x28)
#define MIPIDSI_VTIMING_CFG_OFFSET	(MIPIDSI_OFFSET + 0x2C)
#define MIPIDSI_PHY_TMR_CFG_OFFSET	(MIPIDSI_OFFSET + 0x30)
#define MIPIDSI_GEN_HDR_OFFSET		(MIPIDSI_OFFSET + 0x34)
#define MIPIDSI_GEN_PLD_DATA_OFFSET	(MIPIDSI_OFFSET + 0x38)
#define MIPIDSI_CMD_PKT_STATUS_OFFSET	(MIPIDSI_OFFSET + 0x3C)

#define MIPIDSI_TO_CNT_CFG_OFFSET	(MIPIDSI_OFFSET + 0x40)
#define MIPIDSI_ERROR_ST0_OFFSET	(MIPIDSI_OFFSET + 0x44)
#define MIPIDSI_ERROR_ST1_OFFSET	(MIPIDSI_OFFSET + 0x48)
#define MIPIDSI_ERROR_MSK0_OFFSET	(MIPIDSI_OFFSET + 0x4C)
#define MIPIDSI_ERROR_MSK1_OFFSET	(MIPIDSI_OFFSET + 0x50)
#define MIPIDSI_PHY_RSTZ_OFFSET		(MIPIDSI_OFFSET + 0x54)
#define MIPIDSI_PHY_IF_CFG_OFFSET	(MIPIDSI_OFFSET + 0x58)
#define MIPIDSI_PHY_IF_CTRL_OFFSET	(MIPIDSI_OFFSET + 0x5C)
#define MIPIDSI_PHY_STATUS_OFFSET	(MIPIDSI_OFFSET + 0x60)
#define MIPIDSI_PHY_TST_CTRL0_OFFSET	(MIPIDSI_OFFSET + 0x64)
#define MIPIDSI_PHY_TST_CTRL1_OFFSET	(MIPIDSI_OFFSET + 0x68)
#define MIPIDSI_EDPI_CFG_OFFSET		(MIPIDSI_OFFSET + 0x6C)
#define MIPIDSI_CMD_MOD_CTRL_OFFSET		(MIPIDSI_OFFSET_2 + 0x3C)
#define MIPIDSI_TE_CTRL_OFFSET			(MIPIDSI_OFFSET_2 + 0x40)
#define MIPIDSI_TE_HS_NUM_OFFSET		(MIPIDSI_OFFSET_2 + 0x44)
#define MIPIDSI_TE_HS_WD_OFFSET			(MIPIDSI_OFFSET_2 + 0x48)
#define MIPIDSI_TE_VS_WD_OFFSET			(MIPIDSI_OFFSET_2 + 0x4C)

/******************************************************************************
** FUNCTIONS PROTOTYPES
*/

void set_MIPIDSI_PWR_UP(u32 edc_base, u32 nVal);
void set_MIPIDSI_PWR_UP_shutdownz(u32 edc_base, u32 nVal);
void set_MIPIDSI_CLKMGR_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_DPI_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_DPI_CFG_dpi_vid(u32 edc_base, u32 nVal);
void set_MIPIDSI_DPI_CFG_dpi_color_coding(u32 edc_base, u32 nVal);
void set_MIPIDSI_DPI_CFG_dataen_active_low(u32 edc_base, u32 nVal);
void set_MIPIDSI_DPI_CFG_vsync_active_low(u32 edc_base, u32 nVal);
void set_MIPIDSI_DPI_CFG_hsync_active_low(u32 edc_base, u32 nVal);
void set_MIPIDSI_DPI_CFG_shutd_active_low(u32 edc_base, u32 nVal);
void set_MIPIDSI_DPI_CFG_colorm_active_low(u32 edc_base, u32 nVal);
void set_MIPIDSI_DPI_CFG_en18_loosely(u32 edc_base, u32 nVal);
void set_MIPIDSI_DBI_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_DBI_CMDSIZE(u32 edc_base, u32 nVal);
void set_MIPIDSI_PCKHDL_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_PCKHDL_CFG_en_eotp_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_PCKHDL_CFG_en_eotn_rx(u32 edc_base, u32 nVal);
void set_MIPIDSI_PCKHDL_CFG_en_bta(u32 edc_base, u32 nVal);
void set_MIPIDSI_PCKHDL_CFG_en_ecc_rx(u32 edc_base, u32 nVal);
void set_MIPIDSI_PCKHDL_CFG_en_crc_rx(u32 edc_base, u32 nVal);
void set_MIPIDSI_PCKHDL_CFG_gen_vid_rx(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG_en_lp_vsa(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG_en_lp_vbp(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG_en_lp_vfp(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG_en_lp_vact(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG_en_lp_hbp(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG_en_lp_hfp(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG_frame_bta_ack(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG_en_video_mode(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG_vid_mode_type(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG_en_multi_pkt(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_MODE_CFG_en_null_pkt(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_PKT_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_PKT_CFG_vid_pkt_size(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_PKT_CFG_null_chunks(u32 edc_base, u32 nVal);
void set_MIPIDSI_VID_PKT_CFG_null_pkt_size(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_en_cmd_mode(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_gen_sw_0p_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_gen_sw_1p_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_gen_sw_2p_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_gen_sr_0p_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_gen_sr_1p_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_gen_sr_2p_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_dcs_sw_0p_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_dcs_sw_1p_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_dcs_sr_0p_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_max_rd_pkt_size(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_gen_lw_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_dcs_lw_tx(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_en_ack_rqst(u32 edc_base, u32 nVal);
void set_MIPIDSI_CMD_MODE_CFG_en_tear_fx(u32 edc_base, u32 nVal);
void set_MIPIDSI_TMR_LINE_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_TMR_LINE_CFG_hsa_time(u32 edc_base, u32 nVal);
void set_MIPIDSI_TMR_LINE_CFG_hbp_time(u32 edc_base, u32 nVal);
void set_MIPIDSI_TMR_LINE_CFG_hline_time(u32 edc_base, u32 nVal);
void set_MIPIDSI_VTIMING_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_VTIMING_CFG_vsa_lines(u32 edc_base, u32 nVal);
void set_MIPIDSI_VTIMING_CFG_vbp_lines(u32 edc_base, u32 nVal);
void set_MIPIDSI_VTIMING_CFG_vfp_lines(u32 edc_base, u32 nVal);
void set_MIPIDSI_VTIMING_CFG_v_active_lines(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_TMR_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_TMR_CFG_bta_time(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_TMR_CFG_phy_lp2hs_time(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_TMR_CFG_phy_hs2lp_time(u32 edc_base, u32 nVal);
void set_MIPIDSI_GEN_HDR(u32 edc_base, u32 nVal);
void set_MIPIDSI_GEN_PLD_DATA(u32 edc_base, u32 nVal);
void set_MIPIDSI_TO_CNT_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_RSTZ(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_RSTZ_shutdownz(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_RSTZ_rstz(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_RSTZ_enableclk(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_IF_CFG(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_IF_CFG_n_lanes(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_IF_CFG_phy_stop_wait_time(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_IF_CTRL(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_IF_CTRL_txrequestclkhs(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_IF_CTRL_txrequlpsclk(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_IF_CTRL_txexitulpsclk(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_IF_CTRL_txrequlpslan(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_IF_CTRL_txexitulpslan(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_IF_CTRL_tx_tiggers(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_TST_CTRL0(u32 edc_base, u32 nVal);
void set_MIPIDSI_PHY_TST_CTRL1(u32 edc_base, u32 nVal);
void set_MIPIDSI_TE_CTRL(u32 edc_base, u32 nVal);
void set_MIPIDSI_TE_CTRL_te_hard_en(u32 edc_base, u32 nVal);
void set_MIPIDSI_TE_CTRL_te_mask_en(u32 edc_base, u32 nVal);
void set_MIPIDSI_TE_CTRL_te_pin_en(u32 edc_base, u32 nVal);
void set_MIPIDSI_EDPI_CFG_edpi_en(u32 edc_base, u32 nVal);
void set_MIPIDSI_EDPI_CFG_edpi_allowed_cmd_size(u32 edc_base, u32 nVal);

#endif  /* MIPI_REG_H */
