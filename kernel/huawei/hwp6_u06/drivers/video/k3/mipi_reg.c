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
 * SUBSTITUTE GOODS OR SERVICES;LOSS OF USE, DATA, OR PROFITS;OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "mipi_reg.h"


/**
 * @union
 * @brief Name: Controller Version Identification.
 */
typedef union {
	struct {
		u32 version:32;
	} bits;
	u32 ul32;
} MIPIDSI_VERSION;

/**
 * @union
 * @brief Name: Core power up.
 */
typedef union {
	struct {
		u32 shutdownz:1;
		u32 reserved0:31;
	} bits;
	u32 ul32;
} MIPIDSI_PWR_UP;

/**
 * @union
 * @brief Name: Number of Active Data Lanes.
 */
typedef union {
	struct {
		u32 tx_esc_clk_division:8;
		u32 to_clk_dividsion:8;
		u32 reserved0:16;
	} bits;
	u32 ul32;
} MIPIDSI_CLKMGR_CFG;

/**
 * @union
 * @brief Name: DPI interface configuration. Only available if DPI interface is
 *	selected during RTL configuration.
 */
typedef union {
	struct {
		u32 dpi_vid:2;
		u32 dpi_color_coding:3;
		u32 dataen_active_low:1;
		u32 vsync_active_low:1;
		u32 hsync_active_low:1;
		u32 shutd_active_low:1;
		u32 colorm_active_low:1;
		u32 en18_loosely:1;
		u32 reserved0:21;
	} bits;
	u32 ul32;
} MIPIDSI_DPI_CFG;

/**
 * @union
 * @brief Name: DBI interface configuration. Only available if DBI interface is
 *	selected during RTL configuration.
 */
typedef union {
	struct {
		u32 dbi_vid:2;
		u32 in_dbi_conf:4;
		u32 lut_size_conf:2;
		u32 partitioning_en:1;
		u32 out_dbi_conf:4;
		u32 reserved0:19;
	} bits;
	u32 ul32;
} MIPIDSI_DBI_CFG;

/**
 * @union
 * @brief Name: DBI command size configuration. Only available if DBI interface
 *	is selected during RTL configuration.
 */
typedef union {
	struct {
		u32 wr_cmd_size:16;
		u32 allowed_cmd_size:16;
	} bits;
	u32 ul32;
} MIPIDSI_DBI_CMDSIZE;

/**
 * @union
 * @brief Name: Packet handler configuration
 */
typedef union {
	struct {
		u32 en_eotp_tx:1;
		u32 en_eotn_rx:1;
		u32 en_bta:1;
		u32 en_ecc_rx:1;
		u32 en_crc_rx:1;
		u32 gen_vid_rx:2;
		u32 reserved0:25;
	} bits;
	u32 ul32;
} MIPIDSI_PCKHDL_CFG;

/**
 * @union
 * @brief Name: Video Mode Configuration. Only available if DPI interface is
 *	selected during RTL sconfiguration.
 */
typedef union {
	struct {
		u32 en_video_mode:1;
		u32 vid_mode_type:2;
		u32 en_lp_vsa:1;
		u32 en_lp_vbp:1;
		u32 en_lp_vfp:1;
		u32 en_lp_vact:1;
		u32 en_lp_hbp:1;
		u32 en_lp_hfp:1;
		u32 en_multi_pkt:1;
		u32 en_null_pkt:1;
		u32 frame_bta_ack:1;
		u32 reserved0:20;
	} bits;
	u32 ul32;
} MIPIDSI_VID_MODE_CFG;

/**
 * @union
 * @brief Name: Video packet configuration. Only available if DPI interface is
 *	selected during RTL configuration.
 */
typedef union {
	struct {
		u32 vid_pkt_size:11;
		u32 null_chunks:10;
		u32 null_pkt_size:10;
		u32 reserved0:1;
	} bits;
	u32 ul32;
} MIPIDSI_VID_PKT_CFG;

/**
 * @union
 * @brief Name: Command mode configuration
 */
typedef union {
	struct {
		u32 en_cmd_mode:1;
		u32 gen_sw_0p_tx:1;
		u32 gen_sw_1p_tx:1;
		u32 gen_sw_2p_tx:1;
		u32 gen_sr_0p_tx:1;
		u32 gen_sr_1p_tx:1;
		u32 gen_sr_2p_tx:1;
		u32 dcs_sw_0p_tx:1;
		u32 dcs_sw_1p_tx:1;
		u32 dcs_sw_2p_tx:1;
		u32 max_rd_pkt_size:1;
		u32 gen_lw_tx:1;
		u32 dcs_lw_tx:1;
		u32 en_ack_rqst:1;
		u32 en_tear_fx:1;
		u32 reserved0:17;
	} bits;
	u32 ul32;
} MIPIDSI_CMD_MODE_CFG;

/**
 * @union
 * @brief Name: Line timer configuration. Only available if DPI interface is
 *	selected during RTL configuration
 */
typedef union {
	struct {
		u32 hsa_time:9;
		u32 hbp_time:9;
		u32 hline_time:14;
	} bits;
	u32 ul32;
} MIPIDSI_TMR_LINE_CFG;

/**
 * @union
 * @brief Name: Vertical timing configuration. Only available if DPI interface
 *	is selected during RTL configuration.
 */
typedef union {
	struct {
		u32 vsa_lines:4;
		u32 vbp_lines:6;
		u32 vfp_lines:6;
		u32 v_active_lines:11;
		u32 reserved0:5;
	} bits;
	u32 ul32;
} MIPIDSI_VTIMING_CFG;

/**
 * @union
 * @brief Name: D-PHY timing configuration. Only available if DPI interface is
 *	selected during RTL configuration.
 */
typedef union {
	struct {
		u32 bta_time:15;
		u32 reserved0:1;
		u32 phy_lp2hs_time:8;
		u32 phy_hs2lp_time:8;
	} bits;
	u32 ul32;
} MIPIDSI_PHY_TMR_CFG;

/**
 * @union
 * @brief Name: Generic packet Header configuration. Only available if Generic
 *	interface is selected during RTL configuration.
 */
typedef union {
	struct {
		u32 gen_htype:8;
		u32 gen_hdata:16;
		u32 reserved0:8;
	} bits;
	u32 ul32;
} MIPIDSI_GEN_HDR;

/**
 * @union
 * @brief Name: Generic payload data in/out. Only available if Generic interface
 *	is selected during RTL configuration.
 */
typedef union {
	struct {
		u32 gen_pld_data:32;
	} bits;
	u32 ul32;
} MIPIDSI_GEN_PLD_DATA;

/**
 * @union
 * @brief Name: Command packet status
 */
typedef union {
	struct {
		u32 gen_cmd_empty:1;
		u32 gen_cmd_full:1;
		u32 gen_pld_w_empty:1;
		u32 gen_pld_wr_full:1;
		u32 gen_pld_r_empty:1;
		u32 gen_pld_r_full:1;
		u32 gen_rd_cmd_busy:1;
		u32 reserved0:1;
		u32 dbi_cmd_empty:1;
		u32 dbi_cmd_full:1;
		u32 dbi_pld_w_empty:1;
		u32 dbi_pld_w_full:1;
		u32 dbi_pld_r_empty:1;
		u32 dbi_pld_r_full:1;
		u32 dbi_rd_cmd_busy:1;
		u32 reserved1:17;
	} bits;
	u32 ul32;
} MIPIDSI_CMD_PKT_STATUS;

/**
 * @union
 * @brief Name: Time Out timers configuration.
 */
typedef union {
	struct {
		u32 hstx_to_cnt:16;
		u32 lprx_to_cnt:16;
	} bits;
	u32 ul32;
} MIPIDSI_TO_CNT_CFG;

/**
 * @union
 * @brief Name: Interrupt status register 0
 */
typedef union {
	struct {
		u32 ack_with_err_0:1;
		u32 ack_with_err_1:1;
		u32 ack_with_err_2:1;
		u32 ack_with_err_3:1;
		u32 ack_with_err_4:1;
		u32 ack_with_err_5:1;
		u32 ack_with_err_6:1;
		u32 ack_with_err_7:1;
		u32 ack_with_err_8:1;
		u32 ack_with_err_9:1;
		u32 ack_with_err_10:1;
		u32 ack_with_err_11:1;
		u32 ack_with_err_12:1;
		u32 ack_with_err_13:1;
		u32 ack_with_err_14:1;
		u32 ack_with_err_15:1;
		u32 dphy_errors_0:1;
		u32 dphy_errors_1:1;
		u32 dphy_errors_2:1;
		u32 dphy_errors_3:1;
		u32 dphy_errors_4:1;
		u32 reserved0:11;
	} bits;
	u32 ul32;
} MIPIDSI_ERROR_ST0;

/**
 * @union
 * @brief Name: Interrupt status register 1
 */
typedef union {
	struct {
		u32 to_hs_tx:1;
		u32 to_lp_rx:1;
		u32 ecc_single_err:1;
		u32 ecc_multi_err:1;
		u32 crc_err:1;
		u32 pkt_size_err:1;
		u32 eopt_err:1;
		u32 dpi_pld_wr_err:1;
		u32 gen_cmd_wr_err:1;
		u32 gen_pld_wr_err:1;
		u32 gen_pld_send_err:1;
		u32 gen_pld_rd_err:1;
		u32 gen_pld_recv_err:1;
		u32 dbi_cmd_wr_err:1;
		u32 dbi_pld_wr_err:1;
		u32 dbi_pld_rd_err:1;
		u32 dbi_pld_recv_err:1;
		u32 dbi_illegal_comm_err:1;
		u32 reserved0:14;
	} bits;
	u32 ul32;
} MIPIDSI_ERROR_ST1;

/**
 * @union
 * @brief Name: Masks Interrupt generation triggered by ERROR_ST0 register.
 */
typedef union {
	struct {
		u32 ack_with_err_0:1;
		u32 ack_with_err_1:1;
		u32 ack_with_err_2:1;
		u32 ack_with_err_3:1;
		u32 ack_with_err_4:1;
		u32 ack_with_err_5:1;
		u32 ack_with_err_6:1;
		u32 ack_with_err_7:1;
		u32 ack_with_err_8:1;
		u32 ack_with_err_9:1;
		u32 ack_with_err_10:1;
		u32 ack_with_err_11:1;
		u32 ack_with_err_12:1;
		u32 ack_with_err_13:1;
		u32 ack_with_err_14:1;
		u32 ack_with_err_15:1;
		u32 dphy_errors_0:1;
		u32 dphy_errors_1:1;
		u32 dphy_errors_2:1;
		u32 dphy_errors_3:1;
		u32 dphy_errors_4:1;
		u32 reserved0:11;
	} bits;
	u32 ul32;
} MIPIDSI_ERROR_MSK0;

/**
 * @union
 * @brief Name: Masks Interrupt generation triggered by ERROR_ST1 register.
 */
typedef union {
	struct {
		u32 to_hs_tx:1;
		u32 to_lp_rx:1;
		u32 ecc_single_err:1;
		u32 ecc_multi_err:1;
		u32 crc_err:1;
		u32 pkt_size_err:1;
		u32 eopt_err:1;
		u32 dpi_pld_wr_err:1;
		u32 gen_cmd_wr_err:1;
		u32 gen_pld_wr_err:1;
		u32 gen_pld_send_err:1;
		u32 gen_pld_rd_err:1;
		u32 gen_pld_recv_err:1;
		u32 dbi_cmd_wr_err:1;
		u32 dbi_pld_wr_err:1;
		u32 dbi_pld_rd_err:1;
		u32 dbi_pld_recv_err:1;
		u32 dbi_illegal_comm_err:1;
		u32 reserved0:14;
	} bits;
	u32 ul32;
} MIPIDSI_ERROR_MSK1;

/**
 * @union
 * @brief Name: D-PHY reset control.
 */
typedef union {
	struct {
		u32 phy_shutdownz:1;
		u32 phy_rstz:1;
		u32 phy_enableclk:1;
		u32 reserved0:29;
	} bits;
	u32 ul32;
} MIPIDSI_PHY_RSTZ;

/**
 * @union
 * @brief Name: D-PHY interface configuration
 */
typedef union {
	struct {
		u32 n_lanes:2;
		u32 phy_stop_wait_time:8;
		u32 reserved0:22;
	} bits;
	u32 ul32;
} MIPIDSI_PHY_IF_CFG;

/**
 * @union
 * @brief Name: D-PHY PPI interface control.
 */
typedef union {
	struct {
		u32 phy_txrequestclkhs:1;
		u32 phy_txrequlpsclk:1;
		u32 phy_txexitulpsclk:1;
		u32 phy_txrequlpslan:1;
		u32 phy_txexitulpslan:1;
		u32 phy_tx_tiggers:4;
		u32 reserved0:23;
	} bits;
	u32 ul32;
} MIPIDSI_PHY_IF_CTRL;

/**
 * @union
 * @brief Name: D-PHY PPI status interface.
 */
typedef union {
	struct {
		u32 phylock:1;
		u32 phydirection:1;
		u32 phystopstateclklane:1;
		u32 phyrxulpsclknot:1;
		u32 phystopstate0lane:1;
		u32 ulpsactivenot0lane:1;
		u32 rxulpsesc0lane:1;
		u32 phystopstate1lane:1;
		u32 ulpsactivenot1lane:1;
		u32 phystopstate2lane:1;
		u32 ulpsactivenot2lane:1;
		u32 phystopstate3lane:1;
		u32 ulpsactivenot3lane:1;
		u32 reserved0:19;
	} bits;
	u32 ul32;
} MIPIDSI_PHY_STATUS;

/**
 * @union
 * @brief Name: D-PHY Test interface control 0.
 */
typedef union {
	struct {
		u32 phy_testclr:1;
		u32 phy_testclk:1;
		u32 reserved0:30;
	} bits;
	u32 ul32;
} MIPIDSI_PHY_TST_CTRL0;

/**
 * @union
 * @brief Name: D-PHY Test interface control 1.
 */
typedef union {
	struct {
		u32 phy_testdin:8;
		u32 phy_testdout:8;
		u32 phy_testen:1;
		u32 reserved0:15;
	} bits;
	u32 ul32;
} MIPIDSI_PHY_TST_CTRL1;

/**
 * @union
 * @brief Name: DSI TE CTRL.
 */
typedef union {
	struct {
		u32 dsi_te_hard_en:1;
		u32 dsi_te0_pin_p:1;
		u32 dsi_te1_pin_p:1;
		u32 dsi_te_hard_sel:1;
		u32 dsi_te_pin_hd_sel:1;
		u32 dsi_te_mask_en:1;
		u32 dsi_te_mask_dis:4;
		u32 dsi_te_mask_und:4;
		u32 dsi_te_pin_en:1;
		u32 reserved:17;
	} bits;
	u32 ul32;
} MIPIDSI_TE_CTRL;

/**
 * @union
 * @brief Name: EDPI CFG.
 */
typedef union {
	struct {
		u32 edpi_allowed_cmd_size:16;
		u32 edpi_en:1;
		u32 reserved:15;
	} bits;
	u32 ul32;
} MIPIDSI_EDPI_CFG;

/******************************************************************************
** FUNCTIONS IMPLEMENTATIONS
*/

void set_MIPIDSI_PWR_UP(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PWR_UP_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_PWR_UP_shutdownz(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PWR_UP_OFFSET;
	MIPIDSI_PWR_UP mipidsi_pwr_up;

	mipidsi_pwr_up.ul32 = inp32(addr);
	mipidsi_pwr_up.bits.shutdownz = nVal;
	outp32(addr, mipidsi_pwr_up.ul32);
}

void set_MIPIDSI_CLKMGR_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CLKMGR_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_DPI_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_DPI_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_DPI_CFG_dpi_vid(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_DPI_CFG_OFFSET;
	MIPIDSI_DPI_CFG mipidsi_dpi_cfg;

	mipidsi_dpi_cfg.ul32 = inp32(addr);
	mipidsi_dpi_cfg.bits.dpi_vid = nVal;
	outp32(addr, mipidsi_dpi_cfg.ul32);
}

void set_MIPIDSI_DPI_CFG_dpi_color_coding(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_DPI_CFG_OFFSET;
	MIPIDSI_DPI_CFG mipidsi_dpi_cfg;

	mipidsi_dpi_cfg.ul32 = inp32(addr);
	mipidsi_dpi_cfg.bits.dpi_color_coding = nVal;
	outp32(addr, mipidsi_dpi_cfg.ul32);
}

void set_MIPIDSI_DPI_CFG_dataen_active_low(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_DPI_CFG_OFFSET;
	MIPIDSI_DPI_CFG mipidsi_dpi_cfg;

	mipidsi_dpi_cfg.ul32 = inp32(addr);
	mipidsi_dpi_cfg.bits.dataen_active_low = nVal;
	outp32(addr, mipidsi_dpi_cfg.ul32);
}

void set_MIPIDSI_DPI_CFG_vsync_active_low(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_DPI_CFG_OFFSET;
	MIPIDSI_DPI_CFG mipidsi_dpi_cfg;

	mipidsi_dpi_cfg.ul32 = inp32(addr);
	mipidsi_dpi_cfg.bits.vsync_active_low = nVal;
	outp32(addr, mipidsi_dpi_cfg.ul32);
}

void set_MIPIDSI_DPI_CFG_hsync_active_low(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_DPI_CFG_OFFSET;
	MIPIDSI_DPI_CFG mipidsi_dpi_cfg;

	mipidsi_dpi_cfg.ul32 = inp32(addr);
	mipidsi_dpi_cfg.bits.hsync_active_low = nVal;
	outp32(addr, mipidsi_dpi_cfg.ul32);
}

void set_MIPIDSI_DPI_CFG_shutd_active_low(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_DPI_CFG_OFFSET;
	MIPIDSI_DPI_CFG mipidsi_dpi_cfg;

	mipidsi_dpi_cfg.ul32 = inp32(addr);
	mipidsi_dpi_cfg.bits.shutd_active_low = nVal;
	outp32(addr, mipidsi_dpi_cfg.ul32);
}

void set_MIPIDSI_DPI_CFG_colorm_active_low(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_DPI_CFG_OFFSET;
	MIPIDSI_DPI_CFG mipidsi_dpi_cfg;

	mipidsi_dpi_cfg.ul32 = inp32(addr);
	mipidsi_dpi_cfg.bits.colorm_active_low = nVal;
	outp32(addr, mipidsi_dpi_cfg.ul32);
}

void set_MIPIDSI_DPI_CFG_en18_loosely(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_DPI_CFG_OFFSET;
	MIPIDSI_DPI_CFG mipidsi_dpi_cfg;

	mipidsi_dpi_cfg.ul32 = inp32(addr);
	mipidsi_dpi_cfg.bits.en18_loosely = nVal;
	outp32(addr, mipidsi_dpi_cfg.ul32);
}

void set_MIPIDSI_DBI_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_DBI_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_DBI_CMDSIZE(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_DBI_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_PCKHDL_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PCKHDL_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_PCKHDL_CFG_en_eotp_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PCKHDL_CFG_OFFSET;
	MIPIDSI_PCKHDL_CFG mipidsi_pckhdl_cfg;

	mipidsi_pckhdl_cfg.ul32 = inp32(addr);
	mipidsi_pckhdl_cfg.bits.en_eotp_tx = nVal;
	outp32(addr, mipidsi_pckhdl_cfg.ul32);
}

void set_MIPIDSI_PCKHDL_CFG_en_eotn_rx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PCKHDL_CFG_OFFSET;
	MIPIDSI_PCKHDL_CFG mipidsi_pckhdl_cfg;

	mipidsi_pckhdl_cfg.ul32 = inp32(addr);
	mipidsi_pckhdl_cfg.bits.en_eotn_rx = nVal;
	outp32(addr, mipidsi_pckhdl_cfg.ul32);
}

void set_MIPIDSI_PCKHDL_CFG_en_bta(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PCKHDL_CFG_OFFSET;
	MIPIDSI_PCKHDL_CFG mipidsi_pckhdl_cfg;

	mipidsi_pckhdl_cfg.ul32 = inp32(addr);
	mipidsi_pckhdl_cfg.bits.en_bta = nVal;
	outp32(addr, mipidsi_pckhdl_cfg.ul32);
}

void set_MIPIDSI_PCKHDL_CFG_en_ecc_rx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PCKHDL_CFG_OFFSET;
	MIPIDSI_PCKHDL_CFG mipidsi_pckhdl_cfg;

	mipidsi_pckhdl_cfg.ul32 = inp32(addr);
	mipidsi_pckhdl_cfg.bits.en_ecc_rx = nVal;
	outp32(addr, mipidsi_pckhdl_cfg.ul32);
}

void set_MIPIDSI_PCKHDL_CFG_en_crc_rx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PCKHDL_CFG_OFFSET;
	MIPIDSI_PCKHDL_CFG mipidsi_pckhdl_cfg;

	mipidsi_pckhdl_cfg.ul32 = inp32(addr);
	mipidsi_pckhdl_cfg.bits.en_crc_rx = nVal;
	outp32(addr, mipidsi_pckhdl_cfg.ul32);
}

void set_MIPIDSI_PCKHDL_CFG_gen_vid_rx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PCKHDL_CFG_OFFSET;
	MIPIDSI_PCKHDL_CFG mipidsi_pckhdl_cfg;

	mipidsi_pckhdl_cfg.ul32 = inp32(addr);
	mipidsi_pckhdl_cfg.bits.gen_vid_rx = nVal;
	outp32(addr, mipidsi_pckhdl_cfg.ul32);
}

void set_MIPIDSI_VID_MODE_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_VID_MODE_CFG_en_lp_vsa(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	MIPIDSI_VID_MODE_CFG mipidsi_vid_mode_cfg;

	mipidsi_vid_mode_cfg.ul32 = inp32(addr);
	mipidsi_vid_mode_cfg.bits.en_lp_vsa = nVal;
	outp32(addr, mipidsi_vid_mode_cfg.ul32);
}

void set_MIPIDSI_VID_MODE_CFG_en_lp_vbp(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	MIPIDSI_VID_MODE_CFG mipidsi_vid_mode_cfg;

	mipidsi_vid_mode_cfg.ul32 = inp32(addr);
	mipidsi_vid_mode_cfg.bits.en_lp_vbp = nVal;
	outp32(addr, mipidsi_vid_mode_cfg.ul32);
}

void set_MIPIDSI_VID_MODE_CFG_en_lp_vfp(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	MIPIDSI_VID_MODE_CFG mipidsi_vid_mode_cfg;

	mipidsi_vid_mode_cfg.ul32 = inp32(addr);
	mipidsi_vid_mode_cfg.bits.en_lp_vfp = nVal;
	outp32(addr, mipidsi_vid_mode_cfg.ul32);
}

void set_MIPIDSI_VID_MODE_CFG_en_lp_vact(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	MIPIDSI_VID_MODE_CFG mipidsi_vid_mode_cfg;

	mipidsi_vid_mode_cfg.ul32 = inp32(addr);
	mipidsi_vid_mode_cfg.bits.en_lp_vact = nVal;
	outp32(addr, mipidsi_vid_mode_cfg.ul32);
}

void set_MIPIDSI_VID_MODE_CFG_en_lp_hbp(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	MIPIDSI_VID_MODE_CFG mipidsi_vid_mode_cfg;

	mipidsi_vid_mode_cfg.ul32 = inp32(addr);
	mipidsi_vid_mode_cfg.bits.en_lp_hbp = nVal;
	outp32(addr, mipidsi_vid_mode_cfg.ul32);
}

void set_MIPIDSI_VID_MODE_CFG_en_lp_hfp(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	MIPIDSI_VID_MODE_CFG mipidsi_vid_mode_cfg;

	mipidsi_vid_mode_cfg.ul32 = inp32(addr);
	mipidsi_vid_mode_cfg.bits.en_lp_hfp = nVal;
	outp32(addr, mipidsi_vid_mode_cfg.ul32);
}

void set_MIPIDSI_VID_MODE_CFG_frame_bta_ack(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	MIPIDSI_VID_MODE_CFG mipidsi_vid_mode_cfg;

	mipidsi_vid_mode_cfg.ul32 = inp32(addr);
	mipidsi_vid_mode_cfg.bits.frame_bta_ack = nVal;
	outp32(addr, mipidsi_vid_mode_cfg.ul32);
}

void set_MIPIDSI_VID_MODE_CFG_en_video_mode(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	MIPIDSI_VID_MODE_CFG mipidsi_vid_mode_cfg;

	mipidsi_vid_mode_cfg.ul32 = inp32(addr);
	mipidsi_vid_mode_cfg.bits.en_video_mode = nVal;
	outp32(addr, mipidsi_vid_mode_cfg.ul32);
}

void set_MIPIDSI_VID_MODE_CFG_vid_mode_type(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	MIPIDSI_VID_MODE_CFG mipidsi_vid_mode_cfg;

	mipidsi_vid_mode_cfg.ul32 = inp32(addr);
	mipidsi_vid_mode_cfg.bits.vid_mode_type = nVal;
	outp32(addr, mipidsi_vid_mode_cfg.ul32);
}

void set_MIPIDSI_VID_MODE_CFG_en_multi_pkt(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	MIPIDSI_VID_MODE_CFG mipidsi_vid_mode_cfg;

	mipidsi_vid_mode_cfg.ul32 = inp32(addr);
	mipidsi_vid_mode_cfg.bits.en_multi_pkt = nVal;
	outp32(addr, mipidsi_vid_mode_cfg.ul32);
}

void set_MIPIDSI_VID_MODE_CFG_en_null_pkt(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_MODE_CFG_OFFSET;
	MIPIDSI_VID_MODE_CFG mipidsi_vid_mode_cfg;

	mipidsi_vid_mode_cfg.ul32 = inp32(addr);
	mipidsi_vid_mode_cfg.bits.en_null_pkt = nVal;
	outp32(addr, mipidsi_vid_mode_cfg.ul32);
}

void set_MIPIDSI_VID_PKT_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_PKT_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_VID_PKT_CFG_vid_pkt_size(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_PKT_CFG_OFFSET;
	MIPIDSI_VID_PKT_CFG mipidsi_vid_pkt_cfg;

	mipidsi_vid_pkt_cfg.ul32 = inp32(addr);
	mipidsi_vid_pkt_cfg.bits.vid_pkt_size = nVal;
	outp32(addr, mipidsi_vid_pkt_cfg.ul32);
}

void set_MIPIDSI_VID_PKT_CFG_null_chunks(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_PKT_CFG_OFFSET;
	MIPIDSI_VID_PKT_CFG mipidsi_vid_pkt_cfg;

	mipidsi_vid_pkt_cfg.ul32 = inp32(addr);
	mipidsi_vid_pkt_cfg.bits.null_chunks = nVal;
	outp32(addr, mipidsi_vid_pkt_cfg.ul32);
}

void set_MIPIDSI_VID_PKT_CFG_null_pkt_size(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VID_PKT_CFG_OFFSET;
	MIPIDSI_VID_PKT_CFG mipidsi_vid_pkt_cfg;

	mipidsi_vid_pkt_cfg.ul32 = inp32(addr);
	mipidsi_vid_pkt_cfg.bits.null_pkt_size = nVal;
	outp32(addr, mipidsi_vid_pkt_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_CMD_MODE_CFG_en_cmd_mode(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.en_cmd_mode = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_gen_sw_0p_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.gen_sw_0p_tx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_gen_sw_1p_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.gen_sw_1p_tx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_gen_sw_2p_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.gen_sw_2p_tx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_gen_sr_0p_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.gen_sr_0p_tx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_gen_sr_1p_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.gen_sr_1p_tx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_gen_sr_2p_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.gen_sr_2p_tx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_dcs_sw_0p_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.dcs_sw_0p_tx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_dcs_sw_1p_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.dcs_sw_1p_tx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_dcs_sr_0p_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.dcs_sw_2p_tx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_max_rd_pkt_size(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.max_rd_pkt_size = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_gen_lw_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.gen_lw_tx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_dcs_lw_tx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.dcs_lw_tx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_en_ack_rqst(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.en_ack_rqst = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_CMD_MODE_CFG_en_tear_fx(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET;
	MIPIDSI_CMD_MODE_CFG mipidsi_cmd_mode_cfg;

	mipidsi_cmd_mode_cfg.ul32 = inp32(addr);
	mipidsi_cmd_mode_cfg.bits.en_tear_fx = nVal;
	outp32(addr, mipidsi_cmd_mode_cfg.ul32);
}

void set_MIPIDSI_TMR_LINE_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_TMR_LINE_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_TMR_LINE_CFG_hsa_time(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_TMR_LINE_CFG_OFFSET;
	MIPIDSI_TMR_LINE_CFG mipidsi_tmr_line_cfg;

	mipidsi_tmr_line_cfg.ul32 = inp32(addr);
	mipidsi_tmr_line_cfg.bits.hsa_time = nVal;
	outp32(addr, mipidsi_tmr_line_cfg.ul32);
}

void set_MIPIDSI_TMR_LINE_CFG_hbp_time(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_TMR_LINE_CFG_OFFSET;
	MIPIDSI_TMR_LINE_CFG mipidsi_tmr_line_cfg;

	mipidsi_tmr_line_cfg.ul32 = inp32(addr);
	mipidsi_tmr_line_cfg.bits.hbp_time = nVal;
	outp32(addr, mipidsi_tmr_line_cfg.ul32);
}

void set_MIPIDSI_TMR_LINE_CFG_hline_time(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_TMR_LINE_CFG_OFFSET;
	MIPIDSI_TMR_LINE_CFG mipidsi_tmr_line_cfg;

	mipidsi_tmr_line_cfg.ul32 = inp32(addr);
	mipidsi_tmr_line_cfg.bits.hline_time = nVal;
	outp32(addr, mipidsi_tmr_line_cfg.ul32);
}

void set_MIPIDSI_VTIMING_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VTIMING_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_VTIMING_CFG_vsa_lines(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VTIMING_CFG_OFFSET;
	MIPIDSI_VTIMING_CFG mipidsi_vtiming_cfg;

	mipidsi_vtiming_cfg.ul32 = inp32(addr);
	mipidsi_vtiming_cfg.bits.vsa_lines = nVal;
	outp32(addr, mipidsi_vtiming_cfg.ul32);
}

void set_MIPIDSI_VTIMING_CFG_vbp_lines(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VTIMING_CFG_OFFSET;
	MIPIDSI_VTIMING_CFG mipidsi_vtiming_cfg;

	mipidsi_vtiming_cfg.ul32 = inp32(addr);
	mipidsi_vtiming_cfg.bits.vbp_lines = nVal;
	outp32(addr, mipidsi_vtiming_cfg.ul32);
}

void set_MIPIDSI_VTIMING_CFG_vfp_lines(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VTIMING_CFG_OFFSET;
	MIPIDSI_VTIMING_CFG mipidsi_vtiming_cfg;

	mipidsi_vtiming_cfg.ul32 = inp32(addr);
	mipidsi_vtiming_cfg.bits.vfp_lines = nVal;
	outp32(addr, mipidsi_vtiming_cfg.ul32);
}

void set_MIPIDSI_VTIMING_CFG_v_active_lines(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_VTIMING_CFG_OFFSET;
	MIPIDSI_VTIMING_CFG mipidsi_vtiming_cfg;

	mipidsi_vtiming_cfg.ul32 = inp32(addr);
	mipidsi_vtiming_cfg.bits.v_active_lines = nVal;
	outp32(addr, mipidsi_vtiming_cfg.ul32);
}

void set_MIPIDSI_PHY_TMR_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_TMR_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_PHY_TMR_CFG_bta_time(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_TMR_CFG_OFFSET;
	MIPIDSI_PHY_TMR_CFG mipidsi_phy_tmr_cfg;

	mipidsi_phy_tmr_cfg.ul32 = inp32(addr);
	mipidsi_phy_tmr_cfg.bits.bta_time = nVal;
	outp32(addr, mipidsi_phy_tmr_cfg.ul32);
}

void set_MIPIDSI_PHY_TMR_CFG_phy_lp2hs_time(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_TMR_CFG_OFFSET;
	MIPIDSI_PHY_TMR_CFG mipidsi_phy_tmr_cfg;

	mipidsi_phy_tmr_cfg.ul32 = inp32(addr);
	mipidsi_phy_tmr_cfg.bits.phy_lp2hs_time = nVal;
	outp32(addr, mipidsi_phy_tmr_cfg.ul32);
}

void set_MIPIDSI_PHY_TMR_CFG_phy_hs2lp_time(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_TMR_CFG_OFFSET;
	MIPIDSI_PHY_TMR_CFG mipidsi_phy_tmr_cfg;

	mipidsi_phy_tmr_cfg.ul32 = inp32(addr);
	mipidsi_phy_tmr_cfg.bits.phy_hs2lp_time = nVal;
	outp32(addr, mipidsi_phy_tmr_cfg.ul32);
}

void set_MIPIDSI_GEN_HDR(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_GEN_HDR_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_GEN_PLD_DATA(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_GEN_PLD_DATA_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_TO_CNT_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_TO_CNT_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_PHY_RSTZ(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_RSTZ_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_PHY_RSTZ_shutdownz(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_RSTZ_OFFSET;
	MIPIDSI_PHY_RSTZ mipidsi_phy_rstz;

	mipidsi_phy_rstz.ul32 = inp32(addr);
	mipidsi_phy_rstz.bits.phy_shutdownz = nVal;
	outp32(addr, mipidsi_phy_rstz.ul32);
}

void set_MIPIDSI_PHY_RSTZ_rstz(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_RSTZ_OFFSET;
	MIPIDSI_PHY_RSTZ mipidsi_phy_rstz;

	mipidsi_phy_rstz.ul32 = inp32(addr);
	mipidsi_phy_rstz.bits.phy_rstz = nVal;
	outp32(addr, mipidsi_phy_rstz.ul32);
}

void set_MIPIDSI_PHY_RSTZ_enableclk(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_RSTZ_OFFSET;
	MIPIDSI_PHY_RSTZ mipidsi_phy_rstz;

	mipidsi_phy_rstz.ul32 = inp32(addr);
	mipidsi_phy_rstz.bits.phy_enableclk = nVal;
	outp32(addr, mipidsi_phy_rstz.ul32);
}

void set_MIPIDSI_PHY_IF_CFG(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_IF_CFG_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_PHY_IF_CFG_n_lanes(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_IF_CFG_OFFSET;
	MIPIDSI_PHY_IF_CFG mipidsi_phy_if_cfg;

	mipidsi_phy_if_cfg.ul32 = inp32(addr);
	mipidsi_phy_if_cfg.bits.n_lanes = nVal;
	outp32(addr, mipidsi_phy_if_cfg.ul32);
}

void set_MIPIDSI_PHY_IF_CFG_phy_stop_wait_time(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_IF_CFG_OFFSET;
	MIPIDSI_PHY_IF_CFG mipidsi_phy_if_cfg;

	mipidsi_phy_if_cfg.ul32 = inp32(addr);
	mipidsi_phy_if_cfg.bits.phy_stop_wait_time = nVal;
	outp32(addr, mipidsi_phy_if_cfg.ul32);
}

void set_MIPIDSI_PHY_IF_CTRL(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_PHY_IF_CTRL_txrequestclkhs(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET;
	MIPIDSI_PHY_IF_CTRL mipidsi_phy_if_ctrl;

	mipidsi_phy_if_ctrl.ul32 = inp32(addr);
	mipidsi_phy_if_ctrl.bits.phy_txrequestclkhs = nVal;
	outp32(addr, mipidsi_phy_if_ctrl.ul32);
}

void set_MIPIDSI_PHY_IF_CTRL_txrequlpsclk(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET;
	MIPIDSI_PHY_IF_CTRL mipidsi_phy_if_ctrl;

	mipidsi_phy_if_ctrl.ul32 = inp32(addr);
	mipidsi_phy_if_ctrl.bits.phy_txrequlpsclk = nVal;
	outp32(addr, mipidsi_phy_if_ctrl.ul32);
}

void set_MIPIDSI_PHY_IF_CTRL_txexitulpsclk(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET;
	MIPIDSI_PHY_IF_CTRL mipidsi_phy_if_ctrl;

	mipidsi_phy_if_ctrl.ul32 = inp32(addr);
	mipidsi_phy_if_ctrl.bits.phy_txexitulpsclk = nVal;
	outp32(addr, mipidsi_phy_if_ctrl.ul32);
}

void set_MIPIDSI_PHY_IF_CTRL_txrequlpslan(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET;
	MIPIDSI_PHY_IF_CTRL mipidsi_phy_if_ctrl;

	mipidsi_phy_if_ctrl.ul32 = inp32(addr);
	mipidsi_phy_if_ctrl.bits.phy_txrequlpslan = nVal;
	outp32(addr, mipidsi_phy_if_ctrl.ul32);
}

void set_MIPIDSI_PHY_IF_CTRL_txexitulpslan(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET;
	MIPIDSI_PHY_IF_CTRL mipidsi_phy_if_ctrl;

	mipidsi_phy_if_ctrl.ul32 = inp32(addr);
	mipidsi_phy_if_ctrl.bits.phy_txexitulpslan = nVal;
	outp32(addr, mipidsi_phy_if_ctrl.ul32);
}

void set_MIPIDSI_PHY_IF_CTRL_tx_tiggers(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET;
	MIPIDSI_PHY_IF_CTRL mipidsi_phy_if_ctrl;

	mipidsi_phy_if_ctrl.ul32 = inp32(addr);
	mipidsi_phy_if_ctrl.bits.phy_tx_tiggers = nVal;
	outp32(addr, mipidsi_phy_if_ctrl.ul32);
}

void set_MIPIDSI_PHY_TST_CTRL0(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_TST_CTRL0_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_PHY_TST_CTRL1(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_PHY_TST_CTRL1_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_TE_CTRL(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_TE_CTRL_OFFSET;
	outp32(addr, nVal);
}

void set_MIPIDSI_TE_CTRL_te_hard_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_TE_CTRL_OFFSET;
	MIPIDSI_TE_CTRL mipidsi_te_ctrl;

	mipidsi_te_ctrl.ul32 = inp32(addr);
	mipidsi_te_ctrl.bits.dsi_te_hard_en = nVal;
	outp32(addr, mipidsi_te_ctrl.ul32);
}

void set_MIPIDSI_TE_CTRL_te_mask_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_TE_CTRL_OFFSET;
	MIPIDSI_TE_CTRL mipidsi_te_ctrl;

	mipidsi_te_ctrl.ul32 = inp32(addr);
	mipidsi_te_ctrl.bits.dsi_te_mask_en = nVal;
	outp32(addr, mipidsi_te_ctrl.ul32);
}

void set_MIPIDSI_TE_CTRL_te_pin_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_TE_CTRL_OFFSET;
	MIPIDSI_TE_CTRL mipidsi_te_ctrl;

	mipidsi_te_ctrl.ul32 = inp32(addr);
	mipidsi_te_ctrl.bits.dsi_te_pin_en = nVal;
	outp32(addr, mipidsi_te_ctrl.ul32);
}

void set_MIPIDSI_EDPI_CFG_edpi_en(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_EDPI_CFG_OFFSET;
	MIPIDSI_EDPI_CFG mipidsi_edpi_cfg;

	mipidsi_edpi_cfg.ul32 = inp32(addr);
	mipidsi_edpi_cfg.bits.edpi_en = nVal;
	outp32(addr, mipidsi_edpi_cfg.ul32);
}

void set_MIPIDSI_EDPI_CFG_edpi_allowed_cmd_size(u32 edc_base, u32 nVal)
{
	u32 addr = edc_base + MIPIDSI_EDPI_CFG_OFFSET;
	MIPIDSI_EDPI_CFG mipidsi_edpi_cfg;

	mipidsi_edpi_cfg.ul32 = inp32(addr);
	mipidsi_edpi_cfg.bits.edpi_allowed_cmd_size = nVal;
	outp32(addr, mipidsi_edpi_cfg.ul32);
}