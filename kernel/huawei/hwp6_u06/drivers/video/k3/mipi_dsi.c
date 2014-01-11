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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/raid/pq.h>
#include <linux/clk.h>
#include <mach/boardid.h>

#include "k3_fb.h"
#include "mipi_reg.h"
#include "mipi_dsi.h"
#include "ldi_reg.h"


/* 10MHz */
#define	MAX_TX_ESC_CLK	(10)
#define	DSI_BURST_MODE	DSI_BURST_SYNC_PULSES_1
#define	DEFAULT_MIPI_CLK_RATE	(26 * 1000 * 1000)


static void get_dsi_phy_ctrl(u32 dsi_bit_clk,
	struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	u32 range = 0;
	u32 m_pll = 0;

	BUG_ON(phy_ctrl == NULL);
	range = dsi_bit_clk * 2;

	/* Step 1: Determine PLL Input divider ratio (N)
	  *  Refernce frequency is 26MHz, so N is set to 13 for easy to get any frequency we want.
	  */
#ifdef CONFIG_LCD_1080P
	phy_ctrl->n_pll = 0x1;
#else
	phy_ctrl->n_pll = 0xD;
#endif

	phy_ctrl->pll_unlocking_filter = 0xFF;

	/* Step 2: Calculate PLL loop divider ratio (M) */
	m_pll = range *  phy_ctrl->n_pll / (DEFAULT_MIPI_CLK_RATE / 1000000);
	phy_ctrl->m_pll_1 = (m_pll - 1) & 0x1F;
	phy_ctrl->m_pll_2 = ((m_pll - 1) >> 5) | 0x80;

	/* Step 3: Determine CP current and LPF ctrl*/
	if (m_pll <= 32) {
		phy_ctrl->cp_current = 0x6;
		phy_ctrl->lpf_ctrl = 0x10;
	} else if (m_pll > 32 && m_pll <= 64) {
		phy_ctrl->cp_current = 0x6;
		phy_ctrl->lpf_ctrl = 0x10;
	} else if (m_pll > 64 && m_pll <= 128) {
		phy_ctrl->cp_current = 0xC;
		phy_ctrl->lpf_ctrl = 0x08;
	} else if (m_pll > 128 && m_pll <= 256) {
		phy_ctrl->cp_current = 0x4;
		phy_ctrl->lpf_ctrl = 0x04;
	} else if (m_pll > 256 && m_pll <= 512) {
		phy_ctrl->cp_current = 0x0;
		phy_ctrl->lpf_ctrl = 0x01;
	} else if (m_pll > 512 && m_pll <= 768) {
		phy_ctrl->cp_current = 0x1;
		phy_ctrl->lpf_ctrl = 0x01;
	} else if (m_pll > 768 && m_pll <= 1000) {
		phy_ctrl->cp_current = 0x2;
		phy_ctrl->lpf_ctrl = 0x01;
	} else {
		k3fb_loge("Unsurport the value of PLL loop divider ratio (M) (%d)!\n", m_pll);
		return;
	}

	/* Bypass CP&LPF default values*/
	phy_ctrl->lpf_ctrl |= 0xC0;

	/* Step 4: N and M factors effective*/
	phy_ctrl->factors_effective = 0x33;

	/* Step 5: Determine lp2hs_time, hs2lp_time and hsfreqrange*/
	if (range <= 90) {
		phy_ctrl->lp2hs_time = 24;
		phy_ctrl->hs2lp_time = 14;
		phy_ctrl->hsfreqrange = 0x0;
	} else if (range > 90 && range <= 100) {
		phy_ctrl->lp2hs_time = 25;
		phy_ctrl->hs2lp_time = 14;
		phy_ctrl->hsfreqrange = 0x00000020; /* 0100000 */
	} else if (range > 100 && range <= 110) {
		phy_ctrl->lp2hs_time = 25;
		phy_ctrl->hs2lp_time = 14;
		phy_ctrl->hsfreqrange = 0x00000040; /* 1000000 */
	} else if (range > 110 && range <= 125) {
		phy_ctrl->lp2hs_time = 25;
		phy_ctrl->hs2lp_time = 14;
		phy_ctrl->hsfreqrange = 0x00000002; /* 0000010 */
	} else if (range > 125 && range <= 140) {
		phy_ctrl->lp2hs_time = 25;
		phy_ctrl->hs2lp_time = 14;
		phy_ctrl->hsfreqrange = 0x00000022; /* 0100010 */
	} else if (range > 140 && range <= 150) {
		phy_ctrl->lp2hs_time = 25;
		phy_ctrl->hs2lp_time = 14;
		phy_ctrl->hsfreqrange = 0x00000042; /* 1000010 */
	} else if (range > 150 && range <= 160) {
		phy_ctrl->lp2hs_time = 25;
		phy_ctrl->hs2lp_time = 14;
		phy_ctrl->hsfreqrange = 0x00000004; /* 0000100 */
	} else if (range > 160 && range <= 180) {
		phy_ctrl->lp2hs_time = 28;
		phy_ctrl->hs2lp_time = 16;
		phy_ctrl->hsfreqrange = 0x00000024; /* 0100100 */
	} else if (range > 180 && range <= 200) {
		phy_ctrl->lp2hs_time = 32;
		phy_ctrl->hs2lp_time = 16;
		phy_ctrl->hsfreqrange = 0x00000044; /* 1000100 */
	} else if (range > 200 && range <= 210) {
		phy_ctrl->lp2hs_time = 31;
		phy_ctrl->hs2lp_time = 16;
		phy_ctrl->hsfreqrange = 0x00000006; /* 0000110 */
	} else if (range > 210 && range <= 240) {
		phy_ctrl->lp2hs_time = 35;
		phy_ctrl->hs2lp_time = 17;
		phy_ctrl->hsfreqrange = 0x00000026; /* 0100110 */
	} else if (range > 240 && range <= 250) {
		phy_ctrl->lp2hs_time = 37;
		phy_ctrl->hs2lp_time = 18;
		phy_ctrl->hsfreqrange = 0x00000046;  /* 1000110 */
	} else if (range > 250 && range <= 270) {
		phy_ctrl->lp2hs_time = 37;
		phy_ctrl->hs2lp_time = 18;
		phy_ctrl->hsfreqrange = 0x00000008; /* 0001000 */
	} else if (range > 270 && range <= 300) {
		phy_ctrl->lp2hs_time = 39;
		phy_ctrl->hs2lp_time = 19;
		phy_ctrl->hsfreqrange = 0x00000028; /* 0101000 */
	} else if (range > 300 && range <= 330) {
		phy_ctrl->lp2hs_time = 44;
		phy_ctrl->hs2lp_time = 20;
		phy_ctrl->hsfreqrange = 0x00000008; /* 0001000 */
	} else if (range > 330 && range <= 360) {
		phy_ctrl->lp2hs_time = 47;
		phy_ctrl->hs2lp_time = 21;
		phy_ctrl->hsfreqrange = 0x0000002a; /* 0101010 */
	} else if (range > 360 && range <= 400) {
		phy_ctrl->lp2hs_time = 48;
		phy_ctrl->hs2lp_time = 21;
		phy_ctrl->hsfreqrange = 0x0000004a; /* 1001010 */
	} else if (range > 400 && range <= 450) {
		phy_ctrl->lp2hs_time = 54;
		phy_ctrl->hs2lp_time = 23;
		phy_ctrl->hsfreqrange = 0x0000000c; /* 0001100 */
	} else if (range > 450 && range <= 500) {
		phy_ctrl->lp2hs_time = 58;
		phy_ctrl->hs2lp_time = 25;
		phy_ctrl->hsfreqrange = 0x0000002c; /* 0101100 */
	} else if (range > 500 && range <= 550) {
		phy_ctrl->lp2hs_time = 62;
		phy_ctrl->hs2lp_time = 26;
		phy_ctrl->hsfreqrange = 0x0000000e; /* 0001110 */
	} else if (range > 550 && range <= 600) {
		phy_ctrl->lp2hs_time = 67;
		phy_ctrl->hs2lp_time = 28;
		phy_ctrl->hsfreqrange = 0x0000002e; /* 0101110 */
	} else if (range > 600 && range <= 650) {
		phy_ctrl->lp2hs_time = 72;
		phy_ctrl->hs2lp_time = 30;
		phy_ctrl->hsfreqrange = 0x00000010; /* 0010000 */
	} else if (range > 650 && range <= 700) {
		phy_ctrl->lp2hs_time = 76;
		phy_ctrl->hs2lp_time = 31;
		phy_ctrl->hsfreqrange = 0x00000030; /* 0110000 */
	} else if (range > 700 && range <= 750) {
		phy_ctrl->lp2hs_time = 81;
		phy_ctrl->hs2lp_time = 32;
		phy_ctrl->hsfreqrange = 0x00000012; /* 0010010 */
	} else if (range > 750 && range <= 800) {
		phy_ctrl->lp2hs_time = 86;
		phy_ctrl->hs2lp_time = 34;
		phy_ctrl->hsfreqrange = 0x00000032; /* 0110010 */
	} else if (range > 800 && range <= 850) {
		phy_ctrl->lp2hs_time = 89;
		phy_ctrl->hs2lp_time = 35;
		phy_ctrl->hsfreqrange = 0x00000014; /* 0010100 */
	} else if (range > 850 && range <= 900) {
		phy_ctrl->lp2hs_time = 95;
		phy_ctrl->hs2lp_time = 37;
		phy_ctrl->hsfreqrange = 0x00000034; /* 0110100 */
	} else if (range > 900 && range <= 950) {
		phy_ctrl->lp2hs_time = 99;
		phy_ctrl->hs2lp_time = 38;
		phy_ctrl->hsfreqrange = 0x00000054; /* 1010100 */
	} else if (range > 950 && range <= 1000) {
		phy_ctrl->lp2hs_time = 104;
		phy_ctrl->hs2lp_time = 40;
		phy_ctrl->hsfreqrange = 0x00000074; /* 1110100 */
	} else {
		k3fb_loge("Unsurport this range(%d)!\n", range);
		return;
	}

	phy_ctrl->lane_byte_clk = dsi_bit_clk / 4;
	phy_ctrl->clk_division = ((phy_ctrl->lane_byte_clk % MAX_TX_ESC_CLK) > 0) ?
		(phy_ctrl->lane_byte_clk / MAX_TX_ESC_CLK + 1) :
		(phy_ctrl->lane_byte_clk / MAX_TX_ESC_CLK);
	phy_ctrl->burst_mode = DSI_BURST_MODE;
}

static void mipi_init(struct k3_fb_data_type *k3fd)
{
	u32 edc_base = 0;
	u32 hline_time = 0;
	u32 hsa_time = 0;
	u32 hbp_time = 0;
	u32 pixel_clk = 0;
	unsigned long dw_jiffies = 0;
	struct mipi_dsi_phy_ctrl phy_ctrl = {0};
	u32 tmp = 0;
	bool is_ready = false;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(k3fd == NULL);

	pinfo = &(k3fd->panel_info);
	edc_base = k3fd->edc_base;
	get_dsi_phy_ctrl(pinfo->mipi.dsi_bit_clk, &phy_ctrl);

	/* config TE */
	if (pinfo->type == PANEL_MIPI_CMD) {
		set_reg(edc_base + MIPIDSI_CMD_MOD_CTRL_OFFSET, 0x1, 32, 0);
		set_reg(edc_base + MIPIDSI_TE_CTRL_OFFSET, 0x4021, 32, 0);
		set_reg(edc_base + MIPIDSI_TE_HS_NUM_OFFSET, 0x0, 32, 0);
		set_reg(edc_base + MIPIDSI_TE_HS_WD_OFFSET, 0x8002, 32, 0);
		set_reg(edc_base + MIPIDSI_TE_VS_WD_OFFSET, 0x1001, 32, 0);
	}

	/*--------------configuring the DPI packet transmission----------------*/
	/*
	** 1. Global configuration
	** Configure Register PHY_IF_CFG with the correct number of lanes
	** to be used by the controller.
	*/
	set_MIPIDSI_PHY_IF_CFG_n_lanes(edc_base, pinfo->mipi.lane_nums);

	/*
	** 2. Configure the DPI Interface:
	** This defines how the DPI interface interacts with the controller.
	*/
	set_MIPIDSI_DPI_CFG_dpi_vid(edc_base, pinfo->mipi.vc);
	set_MIPIDSI_DPI_CFG_dpi_color_coding(edc_base, pinfo->mipi.color_mode);
	set_MIPIDSI_DPI_CFG_hsync_active_low(edc_base, pinfo->ldi.hsync_plr);
	set_MIPIDSI_DPI_CFG_vsync_active_low(edc_base, pinfo->ldi.vsync_plr);
	set_MIPIDSI_DPI_CFG_dataen_active_low(edc_base, pinfo->ldi.data_en_plr);
	set_MIPIDSI_DPI_CFG_shutd_active_low(edc_base, 0);
	set_MIPIDSI_DPI_CFG_colorm_active_low(edc_base, 0);
	if (pinfo->bpp == EDC_OUT_RGB_666) {
		set_MIPIDSI_DPI_CFG_en18_loosely(edc_base, 1);
	}

	/*
	** 3. Select the Video Transmission Mode:
	** This defines how the processor requires the video line to be
	** transported through the DSI link.
	*/
	set_MIPIDSI_VID_MODE_CFG_en_lp_vsa(edc_base, 1);
	set_MIPIDSI_VID_MODE_CFG_en_lp_vbp(edc_base, 1);
	set_MIPIDSI_VID_MODE_CFG_en_lp_vfp(edc_base, 1);
	set_MIPIDSI_VID_MODE_CFG_en_lp_vact(edc_base, 1);
	set_MIPIDSI_VID_MODE_CFG_en_lp_hbp(edc_base, 1);
	set_MIPIDSI_VID_MODE_CFG_en_lp_hfp(edc_base, 1);

	set_MIPIDSI_VID_MODE_CFG_frame_bta_ack(edc_base, 0);
	set_MIPIDSI_VID_MODE_CFG_vid_mode_type(edc_base, phy_ctrl.burst_mode);

	set_MIPIDSI_VID_PKT_CFG_vid_pkt_size(edc_base, pinfo->xres);

	/* for dsi read */
	set_MIPIDSI_PCKHDL_CFG_en_bta(edc_base, 1);

	/*
	** 4. Define the DPI Horizontal timing configuration:
	**
	** Hsa_time = HSA*(PCLK period/Clk Lane Byte Period);
	** Hbp_time = HBP*(PCLK period/Clk Lane Byte Period);
	** Hline_time = (HSA+HBP+HACT+HFP)*(PCLK period/Clk Lane Byte Period);
	*/
	pixel_clk = pinfo->clk_rate / 1000000;
	hsa_time = pinfo->ldi.h_pulse_width * phy_ctrl.lane_byte_clk / pixel_clk;
	hbp_time = pinfo->ldi.h_back_porch * phy_ctrl.lane_byte_clk / pixel_clk;
	hline_time = (pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
		pinfo->xres + pinfo->ldi.h_front_porch) *
		phy_ctrl.lane_byte_clk / pixel_clk;
	set_MIPIDSI_TMR_LINE_CFG_hsa_time(edc_base, hsa_time);
	set_MIPIDSI_TMR_LINE_CFG_hbp_time(edc_base, hbp_time);
	set_MIPIDSI_TMR_LINE_CFG_hline_time(edc_base, hline_time);

	/*
	** 5. Define the Vertical line configuration:
	*/
	if (pinfo->ldi.v_pulse_width > 15)
		pinfo->ldi.v_pulse_width = 15;
	set_MIPIDSI_VTIMING_CFG_vsa_lines(edc_base, pinfo->ldi.v_pulse_width);
	set_MIPIDSI_VTIMING_CFG_vbp_lines(edc_base, pinfo->ldi.v_back_porch);
	set_MIPIDSI_VTIMING_CFG_vfp_lines(edc_base, pinfo->ldi.v_front_porch);
	set_MIPIDSI_VTIMING_CFG_v_active_lines(edc_base, pinfo->yres);

	/* Configure core's phy parameters */
	set_MIPIDSI_PHY_TMR_CFG_bta_time(edc_base, 4095);
	set_MIPIDSI_PHY_TMR_CFG_phy_lp2hs_time(edc_base, phy_ctrl.lp2hs_time);
	set_MIPIDSI_PHY_TMR_CFG_phy_hs2lp_time(edc_base, phy_ctrl.hs2lp_time);

	/*------------DSI and D-PHY Initialization-----------------*/
	/* 1. Waking up Core */
	set_MIPIDSI_PWR_UP_shutdownz(edc_base, 1);

	/*
	** 3. Configure the TX_ESC clock frequency to a frequency lower than 20 MHz
	** that is the maximum allowed frequency for D-PHY ESCAPE mode.
	*/
	if (pinfo->type == PANEL_MIPI_CMD) {
		set_MIPIDSI_CLKMGR_CFG(edc_base, 5);
	} else {
		set_MIPIDSI_CLKMGR_CFG(edc_base, phy_ctrl.clk_division);
	}

	/*
	** 4. Configure the DPHY PLL clock frequency through the TEST Interface to
	** operate at XX Hz,
	*/

	/* Write CP current */
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, 0x00010011);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, phy_ctrl.cp_current);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);

	/* Write LPF Control */
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, 0x00010012);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, phy_ctrl.lpf_ctrl);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);

	/*Configured N and M factors effective*/
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, 0x00010019);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, phy_ctrl.factors_effective);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);

	/* Write N Pll */
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, 0x00010017);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, phy_ctrl.n_pll - 1);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);

	/* Write M Pll part 1 */
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, 0x00010018);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, phy_ctrl.m_pll_1);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);

	/* Write M Pll part 2 */
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, 0x00010018);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, phy_ctrl.m_pll_2);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);

	/* Set hsfreqrange */
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, 0x00010044);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, phy_ctrl.hsfreqrange);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);

	/* Set PLL unlocking filter */
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, 0x00010016);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);
	set_MIPIDSI_PHY_TST_CTRL1(edc_base, phy_ctrl.pll_unlocking_filter);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000002);
	set_MIPIDSI_PHY_TST_CTRL0(edc_base, 0x00000000);

	set_MIPIDSI_PHY_RSTZ(edc_base, 0x00000007);

	/* Enable EDPI, ALLOWED_CMD_SIZE = 720*/
	if (pinfo->type == PANEL_MIPI_CMD) {
		set_MIPIDSI_EDPI_CFG_edpi_allowed_cmd_size(edc_base, pinfo->xres);
		set_MIPIDSI_EDPI_CFG_edpi_en(edc_base, K3_ENABLE);
	}

	is_ready = false;
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(edc_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((tmp & 0x00000001) == 0x00000001) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (!is_ready) {
		k3fb_logi("phylock is not ready.\n");
	}

	is_ready = false;
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(edc_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((tmp & 0x00000004) == 0x00000004) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (!is_ready) {
		k3fb_logi("phystopstateclklane is not ready.\n");
	}
}

static int mipi_dsi_on(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);
	edc_base = k3fd->edc_base;

	/* set LCD init step before LCD on*/
	k3fd->panel_info.lcd_init_step = LCD_INIT_POWER_ON;
	ret = panel_next_on(pdev);

	/* mipi dphy clock enable */
	ret = clk_enable(k3fd->mipi_dphy_clk);
	if (ret != 0) {
		k3fb_loge("failed to enable mipi_dphy_clk, error=%d!\n", ret);
		return ret;
	}

	/* dsi pixel on */
	set_reg(edc_base + LDI_HDMI_DSI_GT, 0x0, 1, 0);
	/* mipi init */
	mipi_init(k3fd);

	/* switch to command mode */
	set_reg(edc_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x0, 1, 0);
	set_reg(edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1fff, 13, 0);
	/* disable generate High Speed clock */
	set_reg(edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET, 0x0, 1, 0);

	/* MIPI LP Command mode */
	ret = panel_next_on(pdev);

	/* To ensure that LCD init commands were sent correctly. */
	msleep(10);

	set_reg(edc_base + MIPIDSI_PWR_UP_OFFSET, 0x0, 1, 0);

	if (k3fd->panel_info.type == PANEL_MIPI_VIDEO) {
		/* switch to video mode */
		set_reg(edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 1, 0);
		set_reg(edc_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x1, 1, 0);
	}

	if (k3fd->panel_info.type == PANEL_MIPI_CMD) {
		/* switch to cmd mode */
		set_reg(edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 13, 0);
	}

#ifdef CONFIG_LCD_PANASONIC_VVX10F002A00
	msleep(50);
#endif

	/* enable generate High Speed clock */
	set_reg(edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET, 0x1, 1, 0);

	set_reg(edc_base + MIPIDSI_PWR_UP_OFFSET, 0x1, 1, 0);

	if (k3fd->panel_info.type == PANEL_MIPI_CMD) {
		/* Eable EOTp after switching to HS*/
		set_MIPIDSI_PCKHDL_CFG_en_eotp_tx(edc_base, 0x00000001);
	}

	/* MIPI HS Video/Command mode */
	ret = panel_next_on(pdev);

#ifdef CONFIG_LCD_TOSHIBA_MDY90
	msleep(70);
#endif

	return ret;
}

static int mipi_dsi_off(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;
	u32 edc_base = 0;

	BUG_ON(pdev == NULL);

	k3fd = platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	edc_base = k3fd->edc_base;

	ret = panel_next_off(pdev);

	set_reg(edc_base + MIPIDSI_PWR_UP_OFFSET, 0x0, 1, 0);
	/* switch to command mode */
	set_reg(edc_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x0, 1, 0);
	set_reg(edc_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1fff, 13, 0);
	/* disable generate High Speed clock */
	set_reg(edc_base + MIPIDSI_PHY_IF_CTRL_OFFSET, 0x0, 1, 0);
	set_reg(edc_base + MIPIDSI_PWR_UP_OFFSET, 0x1, 1, 0);

	/* shutdown d_phy */
	set_reg(edc_base +  MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 3, 0);

	/* mipi dphy clk gating */
	clk_disable(k3fd->mipi_dphy_clk);

	return ret;
}

static int mipi_dsi_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);
	k3fd = platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	k3fb_logi("index=%d, enter!\n", k3fd->index);

	if (!IS_ERR(k3fd->mipi_dphy_clk)) {
		clk_put(k3fd->mipi_dphy_clk);
	}

	ret = panel_next_remove(pdev);

	k3fb_logi("index=%d, exit!\n", k3fd->index);
	
	return ret;
}

static int mipi_dsi_set_backlight(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	ret = panel_next_set_backlight(pdev);

	return ret;
}

static int mipi_dsi_set_timing(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_panel_info *pinfo = NULL;
	u32 edc_base = 0;
	u32 hline_time = 0;
	u32 hsa_time = 0;
	u32 hbp_time = 0;
	u32 pixel_clk = 0;
	struct mipi_dsi_phy_ctrl phy_ctrl = {0};

	BUG_ON(pdev == NULL);

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	pinfo = &(k3fd->panel_info);
	edc_base = k3fd->edc_base;
	get_dsi_phy_ctrl(pinfo->mipi.dsi_bit_clk, &phy_ctrl);

	set_MIPIDSI_VID_PKT_CFG_vid_pkt_size(edc_base, pinfo->xres);

	pixel_clk = pinfo->clk_rate / 1000000;
	hsa_time = pinfo->ldi.h_pulse_width * phy_ctrl.lane_byte_clk / pixel_clk;
	hbp_time = pinfo->ldi.h_back_porch * phy_ctrl.lane_byte_clk / pixel_clk;
	hline_time = (pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
		pinfo->xres + pinfo->ldi.h_front_porch) *
		phy_ctrl.lane_byte_clk / pixel_clk;
	set_MIPIDSI_TMR_LINE_CFG_hsa_time(edc_base, hsa_time);
	set_MIPIDSI_TMR_LINE_CFG_hbp_time(edc_base, hbp_time);
	set_MIPIDSI_TMR_LINE_CFG_hline_time(edc_base, hline_time);

	if (pinfo->ldi.v_pulse_width > 15)
		pinfo->ldi.v_pulse_width = 15;
	set_MIPIDSI_VTIMING_CFG_vsa_lines(edc_base, pinfo->ldi.v_pulse_width);
	set_MIPIDSI_VTIMING_CFG_vbp_lines(edc_base, pinfo->ldi.v_back_porch);
	set_MIPIDSI_VTIMING_CFG_vfp_lines(edc_base, pinfo->ldi.v_front_porch);
	set_MIPIDSI_VTIMING_CFG_v_active_lines(edc_base, pinfo->yres);

	ret = panel_next_set_timing(pdev);

	return ret;
}

static int mipi_dsi_set_frc(struct platform_device *pdev, int target_fps)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_panel_info *pinfo = NULL;
	u32 hline_time = 0;
	u32 pixel_clk = 0;
	u32 vertical_timing = 0;
	u32 horizontal_timing = 0;
	u32 h_front_porch = 0;
	int ret = 0;

	BUG_ON(pdev == NULL);
	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	pinfo = &(k3fd->panel_info);

	/* calculate new HFP based on target_fps */
	vertical_timing = pinfo->yres + pinfo->ldi.v_back_porch
		+ pinfo->ldi.v_front_porch + pinfo->ldi.v_pulse_width;
	horizontal_timing = pinfo->clk_rate / (vertical_timing * target_fps);

	/* new HFP*/
	/*
	k3fd->panel_info.ldi.h_front_porch = horizontal_timing - k3fd->panel_info.xres
		-k3fd->panel_info.ldi.h_back_porch - k3fd->panel_info.ldi.h_pulse_width;
	*/
	h_front_porch = horizontal_timing - pinfo->xres
		-pinfo->ldi.h_back_porch - pinfo->ldi.h_pulse_width;

	pixel_clk = pinfo->clk_rate / 1000000;
	/* update hline_time */
	hline_time = (pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
		pinfo->xres + h_front_porch) * 
		(pinfo->mipi.dsi_bit_clk / 4) / pixel_clk;

	pinfo->frame_rate = target_fps;

	/* reset dsi core */
	set_MIPIDSI_PWR_UP(k3fd->edc_base, 0);
	set_MIPIDSI_TMR_LINE_CFG_hline_time(k3fd->edc_base, hline_time);
	set_LDI_HRZ_CTRL0_hfp(k3fd->edc_base, h_front_porch);
	/* power on dsi core */
	set_MIPIDSI_PWR_UP(k3fd->edc_base, 1);

	return ret;
}

static int mipi_dsi_check_esd(struct platform_device *pdev)
{
	BUG_ON(pdev == NULL);
	return panel_next_check_esd(pdev);
}

static int mipi_dsi_probe(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct platform_device *ldi_dev = NULL;
	struct k3_fb_panel_data *pdata = NULL;
	int ret = 0;

	k3fd = (struct k3_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	/* mipi dphy clock */
	if (k3fd->index == 0) {
		k3fd->mipi_dphy_clk = clk_get(NULL, CLK_MIPI_DPHY0_NAME);
	} else if (k3fd->index == 1) {
		k3fd->mipi_dphy_clk = clk_get(NULL, CLK_MIPI_DPHY1_NAME);
	} else {
		k3fb_loge("fb%d not support now!\n", k3fd->index);
		return EINVAL;
	}

	if (IS_ERR(k3fd->mipi_dphy_clk)) {
		k3fb_loge("failed to get mipi_dphy_clk!\n");
		return PTR_ERR(k3fd->mipi_dphy_clk);
	}

#if 0
	/* set mipi dphy clock rate */
	ret = clk_set_rate(k3fd->mipi_dphy_clk, DEFAULT_MIPI_CLK_RATE);
	if (ret != 0) {
		k3fb_loge("failed to set mipi dphy clk rate, error=%d!\n", DEFAULT_MIPI_CLK_RATE);
		/*return ret;*/
	}
#endif

	/* alloc ldi device */
	ldi_dev = platform_device_alloc(DEV_NAME_LDI, pdev->id);
	if (!ldi_dev) {
		k3fb_loge("ldi platform_device_alloc failed!\n");
		return -ENOMEM;
	}

	/* link to the latest pdev */
	k3fd->pdev = ldi_dev;

	/* alloc panel device data */
	if (platform_device_add_data(ldi_dev, pdev->dev.platform_data,
		sizeof(struct k3_fb_panel_data))) {
		k3fb_loge("failed to platform_device_add_data, error=%d!\n", ret);
		platform_device_put(ldi_dev);
		return -ENOMEM;
	}

	/* data chain */
	pdata = (struct k3_fb_panel_data *)ldi_dev->dev.platform_data;
	pdata->on = mipi_dsi_on;
	pdata->off = mipi_dsi_off;
	pdata->remove = mipi_dsi_remove;
	pdata->set_backlight = mipi_dsi_set_backlight;
	pdata->set_timing = mipi_dsi_set_timing;
	pdata->set_frc = mipi_dsi_set_frc;
	pdata->check_esd = mipi_dsi_check_esd;
	pdata->next = pdev;

	/* get/set panel info */
	memcpy(&k3fd->panel_info, pdata->panel_info, sizeof(struct k3_panel_info));

	/* set driver data */
	platform_set_drvdata(ldi_dev, k3fd);
	/* register in ldi driver */
	ret = platform_device_add(ldi_dev);
	if (ret) {
		k3fb_loge("failed to platform_device_add, error=%d!\n", ret);
		platform_device_put(ldi_dev);
		return ret;
	}

	return ret;
}

static struct platform_driver this_driver = {
	.probe = mipi_dsi_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = DEV_NAME_MIPIDSI,
		},
};

static int __init mipi_dsi_driver_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		k3fb_loge("not able to register the driver, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(mipi_dsi_driver_init);
