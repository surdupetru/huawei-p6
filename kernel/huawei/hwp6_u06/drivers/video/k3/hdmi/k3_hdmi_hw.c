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

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/mux.h>
#include <linux/regulator/consumer.h>
#include <linux/mhl/mhl.h>

#include "edc_reg.h"
#include "ldi_reg.h"

#include "k3_hdmi.h"
#include "k3_edid.h"
#include "k3_hdmi_hw.h"
#include "k3_hdcp.h"

#define LOG_TAG "hdmi-hw"
#include "k3_hdmi_log.h"

extern hdmi_device hdmi;

hdmi_hw_res hw_res = {0};

void write_reg(u32 base, u32 idx, u32 val)
{
    writel(val, base + idx);
    return;
}

u32 read_reg(u32 base, u32 idx)
{
    return readl(base + idx);
}

/***********************************************************************************
* Function:       hw_core_blank_video
* Description:    set vide blank
* Data Accessed:
* Data Updated:
* Input:          bool blank : true:need blank, false: need not blank
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
void hw_core_blank_video(bool blank)
{
    u32 regval = 0;

    regval = read_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_DATA_CTRL);

    if (blank) {
        write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_VID_BLANK1, 0);
        write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_VID_BLANK1+4, 0);
        write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_VID_BLANK1+8, 0);
        write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_DATA_CTRL, regval | BIT_VID_BLANK);
    } else {
        write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_DATA_CTRL, regval & (~BIT_VID_BLANK));
    }

    return;
}

/******************************************************************************
* Function:       hw_core_ddc_edid
* Description:    get edid data from ddc
* Data Accessed:
* Data Updated:
* Input:          index of edid block
* Output:         edid data
* Return:
* Others:
*******************************************************************************/
int hw_core_ddc_edid(u8 *edid, int ext)
{
    u32 sts = HDMI_CORE_DDC_STATUS;
    u32 ins = HDMI_CORE_SYS;
    u32 offset  = 0;
    u32 i       = 0;
    u32 j       = 0;
    u32 l       = 0;
    char checksum = 0;
    
    BUG_ON(NULL == edid);

    /* Turn on CLK for DDC */
    REG_FLD_MOD(HDMI_CORE_AV, HDMI_CORE_AV_DPD, 0x7, 2, 0);

    /* Wait */
    mdelay(10);

    if (!ext) {
        /* Clk SCL Devices */
        REG_FLD_MOD(ins, HDMI_CORE_DDC_CMD, 0xA, 3, 0);

        msleep(10);
        while (1 == FLD_GET(read_reg(ins, sts), 4, 4)) {
            msleep(10);
            if (i > 200) {
                loge("Failed to program DDC\n");
                return -ETIMEDOUT;
            }
            i++;
        }
        
        i =0;
        /* Clear FIFO */
        REG_FLD_MOD(ins, HDMI_CORE_DDC_CMD, 0x9, 3, 0);

        while (1 == FLD_GET(read_reg(ins, sts), 4, 4)) {
            msleep(10);
            if (i > 200) {
                loge("Failed to program DDC\n");
                return -ETIMEDOUT;
            }
            i++;
        }
        i = 0;
    } else {
        if (ext%2 != 0) {
            offset = 0x80;
        }
    }

    /* Load Segment Address Register */
    REG_FLD_MOD(ins, HDMI_CORE_DDC_SEGM, ext/2, 7, 0);

    /* Load Slave Address Register */
    REG_FLD_MOD(ins, HDMI_CORE_DDC_ADDR, 0xA0 >> 1, 7, 1);

    /* Load Offset Address Register */
    REG_FLD_MOD(ins, HDMI_CORE_DDC_OFFSET, offset, 7, 0);
    /* Load Byte Count */
    REG_FLD_MOD(ins, HDMI_CORE_DDC_COUNT1, 0x80, 7, 0);
    REG_FLD_MOD(ins, HDMI_CORE_DDC_COUNT2, 0x0, 1, 0);
    /* Set DDC_CMD */

    if (ext) {
        REG_FLD_MOD(ins, HDMI_CORE_DDC_CMD, 0x4, 3, 0);
    } else {
        REG_FLD_MOD(ins, HDMI_CORE_DDC_CMD, 0x2, 3, 0);
    }

    /*
     * do not optimize this part of the code, seems
     * DDC bus needs some time to get stabilized
     */
    l = read_reg(ins, sts);

    if (1 == FLD_GET(l, 6, 6)) {
        loge("I2C Bus Low.\n");
        return -1;
    }

    if (1 == FLD_GET(l, 5, 5)) {
        loge("I2C No Ack.\n");
        return -1;
    }

    i = ext * HDMI_EDID_BLOCK_LENGTH;

    while (j != HDMI_EDID_BLOCK_LENGTH) {
        if (0 == FLD_GET(read_reg(ins, sts), 4, 4) || (!hdmi.connected)) {
            logi("break connected:%d\n", hdmi.connected);
            break;
        }

        if (0 == FLD_GET(read_reg(ins, sts), 2, 2)) {
            /* FIFO not empty */
            edid[i++] = FLD_GET( read_reg(ins, HDMI_CORE_DDC_DATA), 7, 0);
            j++;
        }
    }

    for (j = 0; j < HDMI_EDID_BLOCK_LENGTH; j++) {
        checksum += edid[j];
    }

    if (checksum != 0) {
        loge("E-EDID checksum failed!!\n");
        return -1;
    }

    return 0;
}

/******************************************************************************
* Function:       hw_core_read_edid
* Description:    read edid data
* Data Accessed:
* Data Updated:
* Input:          max length to read
* Output:         edid data
* Return:
* Others:
*******************************************************************************/
int hw_core_read_edid(u8 *edid, u16 max_length)
{
    int ret = 0;
    int n = 0;
    int i = 0;
    int max_ext_blocks = (max_length / HDMI_EDID_BLOCK_LENGTH) - 1;

    BUG_ON(NULL == edid);

    if (max_length < HDMI_EDID_BLOCK_LENGTH) {
        loge("input max length invald(valid: >=128): %d.\n", max_length);
        return -EINVAL;
    }

    /* read edid block 0 */
    ret = hw_core_ddc_edid(edid, 0);
    if (ret) {
        loge("hw core ddc edid fail: %d.\n", ret);
        return ret;
    }

    n = edid[0x7e];

    /*
     * README: need to comply with max_length set by the caller.
     * Better implementation should be to allocate necessary
     * memory to store EDID according to nb_block field found
     * in first block
     */
    if (n > max_ext_blocks) {
        n = max_ext_blocks;
    }

    for (i = 1; i <= n; i++) {
        /* read edid block 1 to n */
        ret = hw_core_ddc_edid(edid, i);
        if (ret) {
            loge("hw core ddc edid fail: %d.\n", ret);
            return ret;
        }
    }

    return ret;
}

/******************************************************************************
* Function:       hw_core_init
* Description:    init default parameter by deep color
* Data Accessed:
* Data Updated:
* Input:          deep color
* Output:         video and audio configure, avi, packet repeat
* Return:
* Others:
*******************************************************************************/
static void hw_core_init(hdmi_deep_color_mode deep_color,
                         hdmi_core_video_config *v_cfg,
                         hw_audio_configure *audio_cfg,
                         hdmi_core_infoframe_avi *avi,
                         hdmi_core_packet_enable_repeat *r_p)
{

    BUG_ON((NULL == v_cfg) || (NULL == audio_cfg) || (NULL == avi) || (NULL == r_p));

    IN_FUNCTION;

    /* video core */
    switch (deep_color) {
        case HDMI_DEEP_COLOR_30BIT:
            v_cfg->input_bus_wide = HDMI_INPUT_10BIT;
            v_cfg->output_dither_truncation = HDMI_OUTPUT_TRUNCATION_10BIT;
            v_cfg->deep_color_packet_enabled = HDMI_DEEP_COLOR_PACKECT_ENABLE;
            v_cfg->packet_mode = HDMI_PACKET_MODE_30BIT_PER_PIXEL;
            break;
        case HDMI_DEEP_COLOR_36BIT:
            v_cfg->input_bus_wide = HDMI_INPUT_12BIT;
            v_cfg->output_dither_truncation = HDMI_OUTPUT_TRUNCATION_12BIT;
            v_cfg->deep_color_packet_enabled = HDMI_DEEP_COLOR_PACKECT_ENABLE;
            v_cfg->packet_mode = HDMI_PACKET_MODE_36BIT_PER_PIXEL;
            break;
        case HDMI_DEEP_COLOR_24BIT:
        default:
            v_cfg->input_bus_wide = HDMI_INPUT_8BIT;
            v_cfg->output_dither_truncation = HDMI_OUTPUT_TRUNCATION_8BIT;
            v_cfg->deep_color_packet_enabled = HDMI_DEEP_COLOR_PACKECT_DISABLE;
            v_cfg->packet_mode = HDMI_PACKET_MODE_24BIT_PER_PIXEL;
            break;
    }

    v_cfg->hdmi_dvi = HDMI_HDMI;
    v_cfg->tclk_sel_clk_mult = FPLL10IDCK;
    /* audio core */
    audio_cfg->fs = FS_48000;
    audio_cfg->n = 6144;
    audio_cfg->layout = LAYOUT_2CH;
    audio_cfg->if_fs = IF_FS_NO;
    audio_cfg->if_channel_number = 2;
    audio_cfg->if_sample_size = IF_NO_PER_SAMPLE;
    audio_cfg->if_audio_channel_location = HDMI_CEA_CODE_00;
    audio_cfg->i2schst_max_word_length = I2S_CHST_WORD_MAX_20;
    audio_cfg->i2schst_word_length = I2S_CHST_WORD_16_BITS;
    audio_cfg->i2s_in_bit_length = I2S_IN_LENGTH_16;
    audio_cfg->i2s_justify = HDMI_AUDIO_JUSTIFY_LEFT;
    audio_cfg->aud_par_busclk = 0;
    audio_cfg->bsio = true;
    audio_cfg->cts_mode = CTS_MODE_HW;

    /* info frame */
    avi->db1y_rgb_yuv422_yuv444 = INFOFRAME_AVI_DB1Y_RGB;
    avi->db1a_active_format_off_on = INFOFRAME_AVI_DB1A_ACTIVE_FORMAT_OFF;
    avi->db1b_no_vert_hori_verthori = INFOFRAME_AVI_DB1B_NO;
    avi->db1s_0_1_2 = INFOFRAME_AVI_DB1S_0;
    avi->db2c_no_itu601_itu709_extented = INFOFRAME_AVI_DB2C_NO;
    avi->db2m_no_43_169 = INFOFRAME_AVI_DB2M_NO;
    avi->db2r_same_43_169_149 = INFOFRAME_AVI_DB2R_SAME;
    avi->db3itc_no_yes = INFOFRAME_AVI_DB3ITC_NO;
    avi->db3ec_xvyuv601_xvyuv709 = INFOFRAME_AVI_DB3EC_XVYUV601;
    avi->db3q_default_lr_fr = INFOFRAME_AVI_DB3Q_DEFAULT;
    avi->db3sc_no_hori_vert_horivert = INFOFRAME_AVI_DB3SC_NO;
    avi->db4vic_videocode = 0;
    avi->db5pr_no_2_3_4_5_6_7_8_9_10 = INFOFRAME_AVI_DB5PR_NO;
    avi->db6_7_lineendoftop = 0 ;
    avi->db8_9_linestartofbottom = 0;
    avi->db10_11_pixelendofleft = 0;
    avi->db12_13_pixelstartofright = 0;

    /* packet enable and repeat */
    r_p->audio_packet_ed                = PACKET_DISABLE;
    r_p->audio_packet_repeat            = PACKET_REPEAT_OFF;
    r_p->avi_info_frame_ed              = PACKET_DISABLE;
    r_p->avi_info_frame_repeat          = PACKET_REPEAT_OFF;
    r_p->general_control_packet_ed      = PACKET_DISABLE;
    r_p->general_control_packet_repeat  = PACKET_REPEAT_OFF;
    r_p->generic_packet_ed              = PACKET_DISABLE;
    r_p->generic_packet_repeat          = PACKET_REPEAT_OFF;
    r_p->mpeg_info_frame_ed             = PACKET_DISABLE;
    r_p->mpeg_info_frame_repeat         = PACKET_REPEAT_OFF;
    r_p->spd_info_frame_ed              = PACKET_DISABLE;
    r_p->spd_info_frame_repeat          = PACKET_REPEAT_OFF;

    OUT_FUNCTION;
    return;
}

/******************************************************************************
* Function:       hw_core_powerdown_enable
* Description:    power off the core
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
static __attribute__ ((unused)) void hw_core_powerdown_enable(void)
{
    REG_FLD_MOD(HDMI_CORE_SYS, HDMI_CORE_CTRL1, 0x1, 0, 0);
    return;
}

/******************************************************************************
* Function:       hw_core_swreset_release
* Description:    Normal Operation
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_core_swreset_release(void)
{
    IN_FUNCTION;

    REG_FLD_MOD(HDMI_CORE_SYS, HDMI_CORE_SYS_SRST, 0x0, 0, 0);

    OUT_FUNCTION;
    return;
}

/******************************************************************************
* Function:       hw_core_swreset_assert
* Description:    Reset all sections, including audio FIFO, but not writable registers or HDCP
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_core_swreset_assert(void)
{
    IN_FUNCTION;

    REG_FLD_MOD(HDMI_CORE_SYS, HDMI_CORE_SYS_SRST, 0x1, 0, 0);

    OUT_FUNCTION;
    return;
}

/******************************************************************************
* Function:       hw_core_video_config
* Description:    set video config
* Data Accessed:
* Data Updated:
* Input:          video config
* Output:
* Return:
* Others:
*******************************************************************************/
static void hw_core_video_config(hdmi_core_video_config *cfg)
{
    u32 core_base_addr    = HDMI_CORE_SYS;
    u32 av_base_addr = HDMI_CORE_AV;
    u32 phy_addr     = HDMI_PHY_BASE;
    u32 val = 0;

    /* sys_ctrl1 default configuration not tunable */
    u32 ven  = 0;
    u32 hen  = 0;
    u32 bsel = 0;
    u32 edge = 0;

    BUG_ON(NULL == cfg);

    IN_FUNCTION;

    /* sys_ctrl1 default configuration not tunable */
    ven = HDMI_CORE_CTRL1_VEN_FOLLOWVSYNC;
    hen = HDMI_CORE_CTRL1_HEN_FOLLOWHSYNC;
    bsel = HDMI_CORE_CTRL1_BSEL_24BITBUS;
    edge = HDMI_CORE_CTRL1_EDGE_RISINGEDGE;

    /* sys_ctrl1 default configuration not tunable */
    val = read_reg(core_base_addr, HDMI_CORE_CTRL1);
    val = FLD_MOD(val, ven, 5, 5);
    val = FLD_MOD(val, hen, 4, 4);
    val = FLD_MOD(val, bsel, 2, 2);
    val = FLD_MOD(val, edge, 1, 1);
    /* PD bit has to be written to recieve the interrupts */
    val = FLD_MOD(val, 1, 0, 0);
    write_reg(core_base_addr, HDMI_CORE_CTRL1, val);

    REG_FLD_MOD(core_base_addr, HDMI_CORE_SYS_VID_ACEN, cfg->input_bus_wide, 7, 6);
    /* set phy deep color */
    REG_FLD_MOD(phy_addr, HDMI_PHY_TDMS_CTL3, cfg->input_bus_wide, 2, 1);

    /* Vid_Mode */
    val = read_reg(core_base_addr, HDMI_CORE_SYS_VID_MODE);
    /* dither truncation configuration */
    if (cfg->output_dither_truncation > HDMI_OUTPUT_TRUNCATION_12BIT) {
        val = FLD_MOD(val, cfg->output_dither_truncation - 3, 7, 6);
        val = FLD_MOD(val, 1, 5, 5);
    } else {
        val = FLD_MOD(val, cfg->output_dither_truncation, 7, 6);
        val = FLD_MOD(val, 0, 5, 5);
    }
    write_reg(core_base_addr, HDMI_CORE_SYS_VID_MODE, val);

    /* HDMI_CTRL */
    val = read_reg(av_base_addr, HDMI_CORE_AV_HDMI_CTRL);
    val = FLD_MOD(val, cfg->deep_color_packet_enabled, 6, 6);
    val = FLD_MOD(val, cfg->packet_mode, 5, 3);
    val = FLD_MOD(val, cfg->hdmi_dvi, 0, 0);
    write_reg(av_base_addr, HDMI_CORE_AV_HDMI_CTRL, val);

    /* TMDS_CTRL */
    REG_FLD_MOD(core_base_addr, HDMI_CORE_SYS_TMDS_CTRL, cfg->tclk_sel_clk_mult, 6, 5);

    OUT_FUNCTION;
    return;
}

/***********************************************************************************
* Function:       hw_core_mute_audio
* Description:    enable or disable audio mute.
* Data Accessed:
* Data Updated:
* Input:          bool benabled: true:enable audio mute, false:disable audio mute
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
void hw_core_mute_audio(bool benabled)
{
    u8 regval = read_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_DATA_CTRL);

    if (false == benabled) {
        write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_DATA_CTRL, regval & (~BIT_AUD_MUTE));
        write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_SRST, 0x00);
    } else {
        write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_DATA_CTRL, regval | BIT_AUD_MUTE);
        write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_SRST, 0x02);
    }

    return;
}

/******************************************************************************
* Function:       hw_core_audio_mode_enable
* Description:    Audio input stream enable
* Data Accessed:
* Data Updated:
* Input:          base address of HDMI IP Audio and video
* Output:
* Return:
* Others:
*******************************************************************************/
static void hw_core_audio_mode_enable(u32  instanceName)
{
    REG_FLD_MOD(instanceName, HDMI_CORE_AV_AUD_MODE, 1, 0, 0);
    hw_core_mute_audio(false);
    return;
}

/******************************************************************************
* Function:       hw_core_audio_config
* Description:    set audio config
* Data Accessed:
* Data Updated:
* Input:          audio config
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_configure_audio(u32 base_addr, hw_audio_configure *audio_cfg)
{
    u32 sd3_en      = 0;
    u32 sd2_en      = 0;
    u32 sd1_en      = 0;
    u32 sd0_en      = 0;
    u32 aud_par_en  = 0;
    u32 spdif_en    = 0;
    u32 val         = 0;

    u32 dbyte1 = 0;
    u32 dbyte2 = 0;
    u32 dbyte4 = 0;
    u32 chsum  = 0;

    u32 size0  = 0;
    u32 size1  = 0;
    u8 acr_en = 1;

    BUG_ON(NULL == audio_cfg);

    IN_FUNCTION;

    /*1. Mute the audio sent to the receiver, as shown in Table 3.*/
    /*2. (Set 0x72:0x0D[2] to 0, 0x72:0x0D[1] to 1, and 0x7A:0xDF[0] to 0).*/
    REG_FLD_MOD(HDMI_CORE_SYS, HDMI_CORE_SYS_DATA_CTRL, 1, 2, 1);
    REG_FLD_MOD(HDMI_CORE_AV, HDMI_CORE_AV_CP_BYTE1, 0, 0, 0);

    /*3. Disable the audio input stream by setting 0x7A:0x14[0] to 0.*/
    REG_FLD_MOD(HDMI_CORE_AV, HDMI_CORE_AV_AUD_MODE, 0, 7, 0);

    /*4. Set all audio mode registers to the new audio mode as needed.*/

    /* CTS_MODE */
    WR_REG_32(base_addr, HDMI_CORE_AV_ACR_CTRL,
              /* MCLK_EN (0: Mclk is not used) */
              (0x1 << 2) |
              /* Set ACR packets while audio is not present */
              (acr_en << 1) |
              /* CTS Source Select (1:SW, 0:HW) */
              (audio_cfg->cts_mode << 0));

    REG_FLD_MOD(base_addr, HDMI_CORE_AV_FREQ_SVAL, 1, 2, 0);
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_N_SVAL1, audio_cfg->n, 7, 0);
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_N_SVAL2, audio_cfg->n >> 8, 7, 0);
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_N_SVAL3, audio_cfg->n >> 16, 7, 0);
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_CTS_SVAL1, audio_cfg->cts, 7, 0);
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_CTS_SVAL2, audio_cfg->cts >> 8, 7, 0);
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_CTS_SVAL3, audio_cfg->cts >> 16, 7, 0);

    /* number of channel */
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_HDMI_CTRL, audio_cfg->layout, 2, 1);
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_AUD_PAR_BUSCLK_1,
                audio_cfg->aud_par_busclk, 7, 0);
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_AUD_PAR_BUSCLK_2,
                (audio_cfg->aud_par_busclk >> 8), 7, 0);
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_AUD_PAR_BUSCLK_3,
                (audio_cfg->aud_par_busclk >> 16), 7, 0);
    /* FS_OVERRIDE = 1 because // input is used */
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_SPDIF_CTRL, 0, 1, 1);
    /* refer to table171 p122 in func core spec*/
    REG_FLD_MOD(base_addr, HDMI_CORE_AV_I2S_CHST4, audio_cfg->fs, 3, 0);

    /*
     * audio config is mainly due to wrapper hardware connection
     * and so are fixed (hardware) I2S deserializer is by-passed
     * so I2S configuration is not needed (I2S don't care).
     * Wrapper is directly connected at the I2S deserialiser
     * output level so some registers call I2S and need to be
     * programmed to configure this parallel bus, there configuration
     * is also fixed and due to the hardware connection (I2S hardware)
     */
    if (1 == audio_cfg->audiotype) {
        /* PCM  */
        WR_REG_32(base_addr, HDMI_CORE_AV_ASRC,
                  (0x1 << 4) | /* HBR_SPR_MASK */
                  (0x0 << 1) | /*  0 = Down-sample 2-to-1 when SRC_EN is set to 1; 1 = Down-sample 4-to-1 when SRC_EN is set to 1 */
                  (0x0 << 0)); /*  Audio sample rate conversion disable */

        WR_REG_32(base_addr, HDMI_CORE_AV_I2S_IN_CTRL,
                  (0 << 7) |    /* HBRA_ON */
                  (1 << 6) |    /* SCK_EDGE Sample clock is rising */
                  (0 << 5) |    /* CBIT_ORDER */
                  (0 << 4) |    /* VBit, 0x0=PCM, 0x1=compressed */
                  (0 << 3) |    /* I2S_WS, 0xdon't care */
                  (audio_cfg->i2s_justify << 2) | /* I2S_JUST*/
                  (0 << 1) |    /* I2S_DIR, 0xdon't care */
                  (0));         /* I2S_SHIFT, 0x0 don't care */
    } else if (audio_cfg->bsio == true) {
        /* high bit compressed */
        WR_REG_32(base_addr, HDMI_CORE_AV_ASRC, 0x0); /* HBR_SPR_MASK,Audio sample rate conversion disable */
        WR_REG_32(base_addr, HDMI_CORE_AV_I2S_IN_CTRL,
                  (1 << 7) |    /* HBRA_ON */
                  (1 << 6) |    /* SCK_EDGE Sample clock is rising */
                  (1 << 5) |    /* CBIT_ORDER */
                  (1 << 4) |    /* VBit, 0x0=PCM, 0x1=compressed */
                  (0 << 3) |    /* I2S_WS, 0xdon't care */
                  (audio_cfg->i2s_justify << 2) | /* I2S_JUST*/
                  (0 << 1) |    /* I2S_DIR, 0xdon't care */
                  (0));         /* I2S_SHIFT, 0x0 don't care */
    } else {
        /* low bit compressed */
        WR_REG_32(base_addr, HDMI_CORE_AV_ASRC, 0x0); /* HBR_SPR_MASK,Audio sample rate conversion disable */
        WR_REG_32(base_addr, HDMI_CORE_AV_I2S_IN_CTRL,
                  (0 << 7) |    /* HBRA_ON */
                  (1 << 6) |    /* SCK_EDGE Sample clock is rising */
                  (0 << 5) |    /* CBIT_ORDER */
                  (1 << 4) |    /* VBit, 0x0=PCM, 0x1=compressed */
                  (0 << 3) |    /* I2S_WS, 0xdon't care */
                  (audio_cfg->i2s_justify << 2) | /* I2S_JUST*/
                  (0 << 1) |    /* I2S_DIR, 0xdon't care */
                  (0));        /* I2S_SHIFT, 0x0 don't care */
    }

    /* write audio FS_ORIG and i2s word length */
    val = read_reg(base_addr, HDMI_CORE_AV_I2S_CHST5);
    val = FLD_MOD(val, audio_cfg->fs, 7, 4); /* FS_ORIG */
    val = FLD_MOD(val, audio_cfg->i2schst_word_length, 3, 1);
    val = FLD_MOD(val, audio_cfg->i2schst_max_word_length, 0, 0);
    WR_REG_32(base_addr, HDMI_CORE_AV_I2S_CHST5, val);

    REG_FLD_MOD(base_addr, HDMI_CORE_AV_I2S_IN_LEN,
                audio_cfg->i2s_in_bit_length, 3, 0);

    /* channel enable depend of the layout */
    if (false == audio_cfg->bsio) {
        /* SPDIF */
        spdif_en   = 1;
        aud_par_en = 0;
        sd3_en = 0;
        sd2_en = 0;
        sd1_en = 0;
        sd0_en = 0;
    } else if (LAYOUT_2CH == audio_cfg->layout) {
        /* sio */
        spdif_en   = 0;
        aud_par_en = 1;
        sd3_en = 0;
        sd2_en = 0;
        sd1_en = 0;
        sd0_en = 1;
    } else if (LAYOUT_8CH == audio_cfg->layout) {
        /* 8 channel */
        spdif_en   = 0;
        aud_par_en = 1;
        sd3_en = 1;
        sd2_en = 1;
        sd1_en = 1;
        sd0_en = 1;
    }

    val = read_reg(base_addr, HDMI_CORE_AV_AUD_MODE);
    val = FLD_MOD(val, sd3_en, 7, 7);       /* sd3_en */
    val = FLD_MOD(val, sd2_en, 6, 6);       /* sd2_en */
    val = FLD_MOD(val, sd1_en, 5, 5);       /* sd1_en */
    val = FLD_MOD(val, sd0_en, 4, 4);       /* sd0_en */
    val = FLD_MOD(val, 0, 3, 3);            /* dsd_en */
    val = FLD_MOD(val, aud_par_en, 2, 2);   /* aud_par_en */
    val = FLD_MOD(val, spdif_en, 1, 1);     /* spdif_en */
    WR_REG_32(base_addr, HDMI_CORE_AV_AUD_MODE, val);

    /* Audio info frame setting refer to CEA-861-d spec p75 */
    /* 0x0 because on HDMI CT must be = 0 / -1 because 1 is for 2 channel */
    //dbyte1 = (audio_cfg->audiotype << 4) + (audio_cfg->if_channel_number - 1);
    dbyte1 = 0; //for mhl Certification, must be 0;

    dbyte2 = (audio_cfg->if_fs << 2) + audio_cfg->if_sample_size;
    /* channel location according to CEA spec */
    dbyte4 = audio_cfg->if_audio_channel_location;

    chsum = (0x100 - 0x84 - 0x01 - 0x0A - dbyte1 - dbyte2 - dbyte4);

    WR_REG_32(base_addr, HDMI_CORE_AV_AUDIO_TYPE, 0x084);
    WR_REG_32(base_addr, HDMI_CORE_AV_AUDIO_VERS, 0x001);
    WR_REG_32(base_addr, HDMI_CORE_AV_AUDIO_LEN, 0x00A);
    /* don't care on VMP */
    WR_REG_32(base_addr, HDMI_CORE_AV_AUDIO_CHSUM, chsum);

    size0 = HDMI_CORE_AV_AUDIO_DBYTE;
    size1 = HDMI_CORE_AV_AUDIO_DBYTE_ELSIZE;
    write_reg(base_addr, (size0 + 0 * size1), dbyte1);
    write_reg(base_addr, (size0 + 1 * size1), dbyte2);
    write_reg(base_addr, (size0 + 2 * size1), 0x000);
    write_reg(base_addr, (size0 + 3 * size1), dbyte4);
    write_reg(base_addr, (size0 + 4 * size1), 0x000);
    write_reg(base_addr, (size0 + 5 * size1), 0x000);
    write_reg(base_addr, (size0 + 6 * size1), 0x000);
    write_reg(base_addr, (size0 + 7 * size1), 0x000);
    write_reg(base_addr, (size0 + 8 * size1), 0x000);
    write_reg(base_addr, (size0 + 9 * size1), 0x000);

    /*5. Wait 6 ms.*/
    mdelay(6);

    /*6. Enable the audio input stream by setting 0x7A:0x14[0] to 1.*/
    REG_FLD_MOD(HDMI_CORE_AV, HDMI_CORE_AV_AUD_MODE, 1, 0, 0);

    /*7. Unmute the audio sent to the receiver. (Set 0x72:0x0D[2] to 0, 0x72:0x0D[1] to 0, and 0x7A:0xDF[0] to 0).*/
    REG_FLD_MOD(HDMI_CORE_SYS, HDMI_CORE_SYS_DATA_CTRL, 0, 2, 1);
    REG_FLD_MOD(HDMI_CORE_AV, HDMI_CORE_AV_CP_BYTE1, 0, 0, 0);

    OUT_FUNCTION;
    return;
}

/******************************************************************************
* Function:       hw_core_audio_infoframe_avi
* Description:    set avi info
* Data Accessed:
* Data Updated:
* Input:          avi info
* Output:
* Return:
* Others:
*******************************************************************************/
static void hw_core_audio_infoframe_avi(hdmi_core_infoframe_avi info_avi)
{
    u32 offset      = 0;
    u32 dbyte       = 0;
    u32 dbyte_size  = 0;
    u32 val         = 0;
    u32 sum         = 0;
    u32 checksum    = 0;

    IN_FUNCTION;
    
    dbyte = HDMI_CORE_AV_AVI_DBYTE;
    dbyte_size = HDMI_CORE_AV_AVI_DBYTE_ELSIZE;
    /* info frame video */
    sum += 0x82 + 0x002 + 0x00D;
    write_reg(HDMI_CORE_AV, HDMI_CORE_AV_AVI_TYPE, 0x082);
    write_reg(HDMI_CORE_AV, HDMI_CORE_AV_AVI_VERS, 0x002);
    write_reg(HDMI_CORE_AV, HDMI_CORE_AV_AVI_LEN, 0x00D);

    offset = dbyte + (0 * dbyte_size);
    val = (info_avi.db1y_rgb_yuv422_yuv444 << 5) |
          (info_avi.db1a_active_format_off_on << 4) |
          (info_avi.db1b_no_vert_hori_verthori << 2) |
          (info_avi.db1s_0_1_2);
    write_reg(HDMI_CORE_AV, offset, val);
    sum += val;

    offset = dbyte + (1 * dbyte_size);
    val = (info_avi.db2c_no_itu601_itu709_extented << 6) |
          (info_avi.db2m_no_43_169 << 4) |
          (info_avi.db2r_same_43_169_149);
    write_reg(HDMI_CORE_AV, offset, val);
    sum += val;

    offset = dbyte + (2 * dbyte_size);
    val = (info_avi.db3itc_no_yes << 7) |
          (info_avi.db3ec_xvyuv601_xvyuv709 << 4) |
          (info_avi.db3q_default_lr_fr << 2) |
          (info_avi.db3sc_no_hori_vert_horivert);
    write_reg(HDMI_CORE_AV, offset, val);
    sum += val;

    offset = dbyte + (3 * dbyte_size);
    write_reg(HDMI_CORE_AV, offset, info_avi.db4vic_videocode);
    sum += info_avi.db4vic_videocode;

    offset = dbyte + (4 * dbyte_size);
    val = info_avi.db5pr_no_2_3_4_5_6_7_8_9_10;
    write_reg(HDMI_CORE_AV, offset, val);
    sum += val;

    offset = dbyte + (5 * dbyte_size);
    val = info_avi.db6_7_lineendoftop & 0x00FF;
    write_reg(HDMI_CORE_AV, offset, val);
    sum += val;

    offset = dbyte + (6 * dbyte_size);
    val = ((info_avi.db6_7_lineendoftop >> 8) & 0x00FF);
    write_reg(HDMI_CORE_AV, offset, val);
    sum += val;

    offset = dbyte + (7 * dbyte_size);
    val = info_avi.db8_9_linestartofbottom & 0x00FF;
    write_reg(HDMI_CORE_AV, offset, val);
    sum += val;

    offset = dbyte + (8 * dbyte_size);
    val = ((info_avi.db8_9_linestartofbottom >> 8) & 0x00FF);
    write_reg(HDMI_CORE_AV, offset, val);
    sum += val;

    offset = dbyte + (9 * dbyte_size);
    val = info_avi.db10_11_pixelendofleft & 0x00FF;
    write_reg(HDMI_CORE_AV, offset, val);
    sum += val;

    offset = dbyte + (10 * dbyte_size);
    val = ((info_avi.db10_11_pixelendofleft >> 8) & 0x00FF);
    write_reg(HDMI_CORE_AV, offset, val);
    sum += val;

    offset = dbyte + (11 * dbyte_size);
    val = info_avi.db12_13_pixelstartofright & 0x00FF;
    write_reg(HDMI_CORE_AV, offset , val);
    sum += val;

    offset = dbyte + (12 * dbyte_size);
    val = ((info_avi.db12_13_pixelstartofright >> 8) & 0x00FF);
    write_reg(HDMI_CORE_AV, offset, val);
    sum += val;

    checksum = 0x100 - sum;
    write_reg(HDMI_CORE_AV, HDMI_CORE_AV_AVI_CHSUM, checksum);

    OUT_FUNCTION;
    return;
}

/******************************************************************************
* Function:       hw_configure_lrfr
* Description:    set lrfr config
* Data Accessed:
* Data Updated:
* Input:          lr fr config
* Output:
* Return:
* Others:
*******************************************************************************/
int hw_configure_lrfr(hdmi_range lr_fr)
{
    u32 val = 0;

    switch (lr_fr) {
            /*
             * Setting the AVI infroframe to respective limited range
             * 0 if for limited range 1 for full range
             */
        case HDMI_LIMITED_RANGE:
            hdmi.avi_param.db3q_default_lr_fr = INFOFRAME_AVI_DB3Q_LR;
            hw_core_audio_infoframe_avi(hdmi.avi_param);

            val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_VID_ACEN);
            val = FLD_MOD(val, 1, 3, 3);
            write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_VID_ACEN, val);
            break;
        case HDMI_FULL_RANGE:
            if (INFOFRAME_AVI_DB1Y_YUV422 == hdmi.avi_param.db1y_rgb_yuv422_yuv444) {
                loge("It is only limited range for YUV\n");
                return -1;
            }

            hdmi.avi_param.db3q_default_lr_fr = INFOFRAME_AVI_DB3Q_FR;
            hw_core_audio_infoframe_avi(hdmi.avi_param);

            val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_VID_MODE);
            val = FLD_MOD(val, 1, 4, 4);
            write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_VID_MODE, val);
            break;
        default:
            BUG_ON(NULL == NULL);
            break;
    }

    return 0;
}

/******************************************************************************
* Function:       hw_core_vsi_infoframe
* Description:    set 3d info frame by hdmi_s3d_config
* Data Accessed:
* Data Updated:
* Input:          base address and hdmi_s3d_config
* Output:
* Return:
* Others:
*******************************************************************************/
static void hw_core_vsi_infoframe(u32 base_addr, hdmi_s3d_config info_s3d)
{
    int i      = 0;
    u32 offset = 0;
    u8  sum    = 0;

    /* For side-by-side(HALF) we need to specify subsampling in 3D_ext_data */
    int length = info_s3d.structure > 0x07 ? 6 : 5;

    u8 info_frame_packet[] = {
        0x81,                       /*Vendor-Specific InfoFrame*/
        0x01,                       /*InfoFrame version number per CEA-861-D*/
        length,                     /*InfoFrame length, excluding checksum and header*/
        0x00,                       /*Checksum*/
        0x03, 0x0C, 0x00,           /*24-bit IEEE Registration Ident*/
        (HDMI_2D == info_s3d.structure) ? 0 : 0x40, /*3D format indication preset, 3D_Struct follows*/
        info_s3d.structure << 4,    /*3D_Struct, no 3D_Meta*/
        info_s3d.s3d_ext_data << 4, /*3D_Ext_Data*/
    };
    
    /*Adding packet header and checksum length*/
    length += 4;

    /*Checksum is packet_header+checksum+infoframe_length = 0*/
    for (i = 0; i < length; i++) {
        sum += info_frame_packet[i];
    }
    info_frame_packet[3] = 0x100 - sum;

    offset = HDMI_CORE_AV_GEN_DBYTE;
    for (i = 0; i < length; i++) {
        write_reg(base_addr, offset, info_frame_packet[i]);
        offset += HDMI_CORE_AV_GEN_DBYTE_ELSIZE;
    }

    return;
}

/******************************************************************************
* Function:       hw_core_av_packet_config
* Description:    set audio and video packet config
* Data Accessed:
* Data Updated:
* Input:          base address and hdmi_core_packet_enable_repeat
* Output:
* Return:
* Others:
*******************************************************************************/
static void hw_core_av_packet_config(u32 base_addr, hdmi_core_packet_enable_repeat r_p)
{
    IN_FUNCTION;
    
    /* Turn on CLK for DDC */
    REG_FLD_MOD(HDMI_CORE_AV, HDMI_CORE_AV_DPD, 0x1, 2, 2);
    mdelay(5);
    /* enable/repeat the infoframe */
    write_reg(base_addr, HDMI_CORE_AV_PB_CTRL1,
              (r_p.mpeg_info_frame_ed << 7) |
              (r_p.mpeg_info_frame_repeat << 6) |
              (r_p.audio_packet_ed << 5) |
              (r_p.audio_packet_repeat << 4) |
              (r_p.spd_info_frame_ed << 3) |
              (r_p.spd_info_frame_repeat << 2) |
              (r_p.avi_info_frame_ed << 1) |
              (r_p.avi_info_frame_repeat));

    /* enable/repeat the packet */
    write_reg(base_addr, HDMI_CORE_AV_PB_CTRL2,
              (r_p.general_control_packet_ed << 3) |
              (r_p.general_control_packet_repeat << 2) |
              (r_p.generic_packet_ed << 1) |
              (r_p.generic_packet_repeat));

    OUT_FUNCTION;
    return;
}

/******************************************************************************
* Function:       hw_configure_acr
* Description:    config n and cts by pclk, fs and deep_color
* Data Accessed:
* Data Updated:
* Input:          pixel clock, audio fs
* Output:
* Return:
* Others:
*******************************************************************************/
int hw_configure_acr(u32 pclk, hdmi_core_fs audio_fs)
{
    u32 fs          = 0;
    u32 n           = 0;
    u32 cts         = 0;

    IN_FUNCTION;

    switch (audio_fs) {
        case FS_32000:
            fs = 32000;
            n = 4096;            
            break;
        case FS_44100:
            fs = 44100;
            n = 6272;
            break;
        case FS_48000:
            fs = 48000;
            n = 6144;            
            break;
        case FS_96000:
            fs = 96000;
            n = 12288;
            break;
        case FS_176400:
            fs = 176400;
            n = 25088;
            break;
        case FS_192000:
            fs = 192000;
            n = 24576;
            break;
        case FS_88200:        
        case FS_NOT_INDICATED:
        default:
            loge("fs is invalid: %d.\n", audio_fs);
            OUT_FUNCTION;
            return -EINVAL;
    }

    /* Process CTS */
    cts = (pclk * (n / 128) * 10) / (fs / 100);

    hdmi.audio_core_cfg.n = n;
    hdmi.audio_core_cfg.cts = cts;

    OUT_FUNCTION;
    return 0;
}

void hw_enable_tmds()
{
    /*tmds_oe enable*/
    REG_FLD_MOD(hw_res.base_core, HDMI_CORE_SYS_TMDS_CTRL4, 1, 4, 4);
    /*0xFA205804[5]= 0 phy output enable*/
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_TDMS_CTL2, 1, 5, 5);
    return;
}

void hw_disable_tmds()
{
    /*0xFA205804[5]= 0 phy output disable*/
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_TDMS_CTL2, 0, 5, 5);
    /*tmds_oe disable*/
    REG_FLD_MOD(hw_res.base_core, HDMI_CORE_SYS_TMDS_CTRL4, 0, 4, 4);
    return;
}

/******************************************************************************
* Function:       hw_enable
* Description:    config hdmi by hdmi config or default
* Data Accessed:
* Data Updated:
* Input:          hdmi configs
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_enable(hdmi_config *cfg)
{
    u32 av_base_addr = HDMI_CORE_AV;

    /* s3d param */
    hdmi_s3d_config s3d_param = {0};

    /* HDMI core */
    hdmi_core_video_config v_core_cfg = {0};
    hdmi_core_packet_enable_repeat repeat_param = {0};

    BUG_ON(NULL == cfg);
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
    {
        int hdmi_ddr_freq = HDMI_PM_QOS_DDR_MIN_FREQ;
        if (hdmi.has_request_ddr) {
            pm_qos_remove_request(&hdmi.qos_request);
        }

        if ((HDMI_EDID_EX_VIDEO_1920x1080p_50Hz_16_9 == hdmi.code || HDMI_EDID_EX_VIDEO_1920x1080p_60Hz_16_9 == hdmi.code)
            && HDMI_CODE_TYPE_CEA == hdmi.mode) {
            hdmi_ddr_freq = HDMI_1080P50_60_DDR_MIN_FREQ;
        }
        pm_qos_add_request(&hdmi.qos_request, PM_QOS_DDR_MIN_PROFILE, hdmi_ddr_freq);
        hdmi.has_request_ddr = true;
        logi("hw_enable set ddr freq:%d\n", hdmi_ddr_freq);
    }
#endif

    IN_FUNCTION;
    hw_core_init(cfg->deep_color, &v_core_cfg,
                 &hdmi.audio_core_cfg,
                 &hdmi.avi_param,
                 &repeat_param);

    /****************************** CORE *******************************/
    /************* configure core video part ********************************/
    
    // Disable transmission of AVI InfoFrames during re-configuration
    write_reg(av_base_addr, HDMI_CORE_AV_PB_CTRL1, 0);
    write_reg(av_base_addr, HDMI_CORE_AV_PB_CTRL2, 0);

    hw_disable_tmds();

    /* set software reset in the core */
    hw_core_swreset_assert();
    v_core_cfg.hdmi_dvi = cfg->hdmi_dvi;

    hw_core_video_config(&v_core_cfg);

    if(HDMI_HDMI == cfg->hdmi_dvi) {
        hdmi.audiochanged = true;
        hw_core_audio_mode_enable(av_base_addr);
        hdmi_audio_set_param(hdmi.sample_freq, hdmi.sample_size, hdmi.bsio, hdmi.layout, hdmi.audiotype);
        /* config n and cts by pixel_clock and fs */
        hw_configure_acr(hdmi.timings->pixel_clock, hdmi.audio_core_cfg.fs);
        /* set audio config to hdmi device */
        hw_configure_audio(HDMI_CORE_AV, &hdmi.audio_core_cfg);
        hdmi.audiochanged = false;
    }
    
    /* configure packet */
    /* info frame video see doc CEA861-D page 65 */
    hdmi.avi_param.db1y_rgb_yuv422_yuv444 = INFOFRAME_AVI_DB1Y_RGB;
    hdmi.avi_param.db1a_active_format_off_on = INFOFRAME_AVI_DB1A_ACTIVE_FORMAT_OFF;
    hdmi.avi_param.db1b_no_vert_hori_verthori = INFOFRAME_AVI_DB1B_NO;
    hdmi.avi_param.db1s_0_1_2 = INFOFRAME_AVI_DB1S_0;
    hdmi.avi_param.db2c_no_itu601_itu709_extented = INFOFRAME_AVI_DB2C_NO;

    /* Support AR in AVI infoframe */
    switch (cfg->video_format) {
            /* 16:9 */
        case HDMI_EDID_EX_VIDEO_720x480p_60Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1280x720p_60Hz_16_9:
        case HDMI_EDID_EX_VIDEO_720x240p_60Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1920x1080i_60Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1920x1080p_60Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1440x480p_60Hz_16_9:
        case HDMI_EDID_EX_VIDEO_720x576p_50Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1280x720p_50Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1920x1080i_50Hz_16_9:
        case HDMI_EDID_EX_VIDEO_720x288p_50Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1440x576p_50Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1920x1080p_50Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1920x1080p_24Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1920x1080p_25Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1920x1080p_30Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1280x720p_100Hz_16_9:
        case HDMI_EDID_EX_VIDEO_720x576p_100Hz_16_9:
        case HDMI_EDID_EX_VIDEO_1280x720p_120Hz_16_9:
        case HDMI_EDID_EX_VIDEO_720x480p_120Hz_16_9:
        case HDMI_EDID_EX_VIDEO_720x576p_200Hz_16_9:
        case HDMI_EDID_EX_VIDEO_720x480p_240Hz_16_9:
            hdmi.avi_param.db2m_no_43_169 = INFOFRAME_AVI_DB2M_169;
            break;
            /* 4:3 */
        case HDMI_EDID_EX_VIDEO_640x480p_60Hz_4_3:
        case HDMI_EDID_EX_VIDEO_720x480p_60Hz_4_3:
        case HDMI_EDID_EX_VIDEO_720x480i_60Hz_4_3:
        case HDMI_EDID_EX_VIDEO_720x576p_50Hz_4_3:
        case HDMI_EDID_EX_VIDEO_720x576i_50Hz_4_3:
        case HDMI_EDID_EX_VIDEO_1440x576p_50Hz_4_3:
        case HDMI_EDID_EX_VIDEO_720x240p_60Hz_4_3:
        case HDMI_EDID_EX_VIDEO_1440x480p_60Hz_4_3:
        case HDMI_EDID_EX_VIDEO_720x288p_50Hz_4_3:
        case HDMI_EDID_EX_VIDEO_720x576p_100Hz_4_3:
        case HDMI_EDID_EX_VIDEO_720x480p_120Hz_4_3:
        case HDMI_EDID_EX_VIDEO_720x576p_200Hz_4_3:
        case HDMI_EDID_EX_VIDEO_720x480p_240Hz_4_3:
            hdmi.avi_param.db2m_no_43_169 = INFOFRAME_AVI_DB2M_43;
            break;
        default:
            hdmi.avi_param.db2m_no_43_169 = INFOFRAME_AVI_DB2M_NO;
            break;
    }

    hdmi.avi_param.db2r_same_43_169_149 = INFOFRAME_AVI_DB2R_SAME;
    hdmi.avi_param.db3itc_no_yes = INFOFRAME_AVI_DB3ITC_NO;
    hdmi.avi_param.db3ec_xvyuv601_xvyuv709 = INFOFRAME_AVI_DB3EC_XVYUV601;
    hdmi.avi_param.db3sc_no_hori_vert_horivert = INFOFRAME_AVI_DB3SC_NO;
    hdmi.avi_param.db4vic_videocode = cfg->video_format;
    hdmi.avi_param.db5pr_no_2_3_4_5_6_7_8_9_10 = INFOFRAME_AVI_DB5PR_NO;
    hdmi.avi_param.db6_7_lineendoftop = 0;
    hdmi.avi_param.db8_9_linestartofbottom = 0;
    hdmi.avi_param.db10_11_pixelendofleft = 0;
    hdmi.avi_param.db12_13_pixelstartofright = 0;

    hw_core_audio_infoframe_avi(hdmi.avi_param);

    s3d_param.structure = cfg->s3d_structure;
    s3d_param.s3d_ext_data = cfg->subsamp_pos;
    hw_core_vsi_infoframe(av_base_addr, s3d_param);

    if (HDMI_HDMI == cfg->hdmi_dvi) {
        /* HDMI mode */

        /* enable/repeat the infoframe */
        repeat_param.avi_info_frame_ed = PACKET_ENABLE;
        repeat_param.avi_info_frame_repeat = PACKET_REPEAT_ON;

        repeat_param.audio_packet_ed = PACKET_ENABLE;
        repeat_param.audio_packet_repeat = PACKET_REPEAT_ON;

        repeat_param.spd_info_frame_ed = PACKET_DISABLE;//cfg->supports_ai;
        repeat_param.spd_info_frame_repeat = PACKET_REPEAT_OFF;//cfg->supports_ai;

        /* enable/repeat the vendor specific infoframe */
        repeat_param.generic_packet_ed = PACKET_ENABLE;
        repeat_param.generic_packet_repeat = PACKET_REPEAT_ON;
    } else {
        /* DVI mode */

        /* enable/repeat the infoframe */
        repeat_param.avi_info_frame_ed = PACKET_DISABLE;
        repeat_param.avi_info_frame_repeat = PACKET_REPEAT_OFF;

        repeat_param.audio_packet_ed = PACKET_DISABLE;
        repeat_param.audio_packet_repeat = PACKET_REPEAT_OFF;

        repeat_param.spd_info_frame_ed = PACKET_DISABLE;
        repeat_param.spd_info_frame_repeat = PACKET_REPEAT_OFF;

        /* enable/repeat the vendor specific infoframe */
        repeat_param.generic_packet_ed = PACKET_DISABLE;
        repeat_param.generic_packet_repeat = PACKET_REPEAT_OFF;
    }

#if HDMI_FOR_CERTIFICATION
    if (cfg->has_vcdb) {
        int ret = hw_configure_lrfr(HDMI_LIMITED_RANGE);
        if (ret) {
            loge("hw_configure_lrfr fail, ret: %d.\n", ret);
        }
    }
#endif

    hw_core_av_packet_config(av_base_addr, repeat_param);
    /*must delay here, otherwise display error*/
    mdelay(200);

    REG_FLD_MOD(av_base_addr, HDMI_CORE_AV_HDMI_CTRL, cfg->hdmi_dvi, 0, 0);

    /* release software reset in the core */
    hw_core_swreset_release();

    hw_enable_tmds();

    OUT_FUNCTION;
    
    return;
}

void hw_init_irqs(void)
{
    /* init intr POLARITY# = 0: INT pin assertion level high */
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INTR1_MASK,0x0);
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INTR2_MASK,0x0);
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INTR3_MASK,0x0);
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INTR4_MASK,0x0);

    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INT_CNTRL,0x0);
    
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INTR1,0xFF);
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INTR2,0xFF);
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INTR3,0xFF);
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INTR4,0x0F);
    return;
}

/******************************************************************************
* Function:       hw_enable_irqs
* Description:    enable or disable hdmi irq
* Data Accessed:
* Data Updated:
* Input:          true to enable irq, false to disable irq
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_enable_irqs(bool enable)
{
    static bool irq_isdisabled = false;

    logd("irq_isdisabled:%d, enable:%d. \n", irq_isdisabled, enable);

    if((!enable) && (!irq_isdisabled)) {
        logi("disable irq.\n");
        disable_irq(hw_res.irq);
        irq_isdisabled = true;
    }
    hw_init_irqs();

    write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR1_MASK, enable? (BIT_INTR1_HPD | BIT_INTR1_RSEN) : 0x00);
    write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR2_MASK, enable? BIT_INTR2_TCLK_STBL : 0x00);

    if((enable) && (irq_isdisabled)) {
        logi("enable irq.\n");
        enable_irq(hw_res.irq);
        irq_isdisabled = false;
    }

    return;
}

inline void dump_intr_status( int irq_id, u32 intr_val)
{
    #define DUMP_INTR_STATUS(intr_val, intr_bit, bit) \
    do {\
        if (intr_val & intr_bit) {\
            loge("              %d %s\n", bit,#intr_bit);\
        }\
    }while(0);

    if ((1 == irq_id) && intr_val) {
        loge(" ================= irq1 ==================== \n");
        DUMP_INTR_STATUS(intr_val, BIT_INTR1_SOFT, 7);
        DUMP_INTR_STATUS(intr_val, BIT_INTR1_HPD, 6);
        DUMP_INTR_STATUS(intr_val, BIT_INTR1_RSEN, 5);
        DUMP_INTR_STATUS(intr_val, BIT_INTR1_DROP_SAMPLE, 4);
        DUMP_INTR_STATUS(intr_val, BIT_INTR1_BI_PHASE_ERR, 3);
        DUMP_INTR_STATUS(intr_val, BIT_INTR1_RI_128, 2);
        DUMP_INTR_STATUS(intr_val, BIT_INTR1_OVER_RUN, 1);
        DUMP_INTR_STATUS(intr_val, BIT_INTR1_UNDER_RUN, 0);
        loge(" ================= irq1 ==================== \n\n");
    }

    if ((2 == irq_id) && intr_val) {
        loge(" ================= irq2 ==================== \n");
        DUMP_INTR_STATUS(intr_val, BIT_INTR2_BCAP_DONE, 7);
        DUMP_INTR_STATUS(intr_val, BIT_INTR2_SPDIF_PAR, 6);
        DUMP_INTR_STATUS(intr_val, BIT_INTR2_ENC_DIS, 5);
        DUMP_INTR_STATUS(intr_val, BIT_INTR2_PREAM_ERR, 4);
        DUMP_INTR_STATUS(intr_val, BIT_INTR2_CTS_CHG, 3);
        DUMP_INTR_STATUS(intr_val, BIT_INTR2_ACR_OVR, 2);
        DUMP_INTR_STATUS(intr_val, BIT_INTR2_TCLK_STBL, 1);
        DUMP_INTR_STATUS(intr_val, BIT_INTR2_VSYNC_REC, 0);
        loge(" ================= irq2 ==================== \n\n");
    }

    if ((3 == irq_id) && intr_val) {
        loge(" ================= irq3 ==================== \n");
        DUMP_INTR_STATUS(intr_val, BIT_INTR3_RI_ERR3, 7);
        DUMP_INTR_STATUS(intr_val, BIT_INTR3_RI_ERR2, 6);
        DUMP_INTR_STATUS(intr_val, BIT_INTR3_RI_ERR1, 5);
        DUMP_INTR_STATUS(intr_val, BIT_INTR3_RI_ERR0, 4);
        DUMP_INTR_STATUS(intr_val, BIT_INTR3_DDC_CMD_DONE, 3);
        DUMP_INTR_STATUS(intr_val, BIT_INTR3_DDC_FIFO_HALF, 2);
        DUMP_INTR_STATUS(intr_val, BIT_INTR3_DDC_FIFO_FULL, 1);
        DUMP_INTR_STATUS(intr_val, BIT_INTR3_DDC_FIFO_EMPTY, 0);
        loge(" ================= irq3 ==================== \n\n");
    }

    if ((4 == irq_id) && intr_val) {
        loge(" ================= irq4 ==================== \n");
        DUMP_INTR_STATUS(intr_val, BIT_INTR4_CEC, 3);
        DUMP_INTR_STATUS(intr_val, BIT_INTR4_DSD_INVALID, 0);
        loge(" ================= irq4 ==================== \n\n");
    }

    return;
}
/******************************************************************************
* Function:       hw_get_hdmi_state
* Description:    get irq state by register state
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:         irq state
* Others:
*******************************************************************************/
int hw_get_hdmi_state(void)
{
    u32 set         = 0;
    u32 hpd_intr    = 0;
    u32 core_state  = 0;
    u32 intr2       = 0;
    u32 intr3       = 0;
    u32 intr4       = 0;
    int hdmi_state  = 0;

    static volatile int first_hpd = 0;
    static volatile int laststatus = 0;

    core_state = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR_STATE);
    if (!(core_state & BIT_INTR)) {
        logd("this is a fake irq.\n");
        return 0;
    }

    hpd_intr = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR1);
    intr2 = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR2);
    intr3 = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR3);
    intr4 = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR4);

    /* write to flush */
    write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR1, hpd_intr);
    write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR2, intr2);
    write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR3, intr3);
    write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR4, intr4);
    set = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_SYS_STAT);

    logd("core_state: 0x%x, sys_stat: 0x%x, hpd_intr: 0x%x, intr2: 0x%x, intr3: 0x%x, intr4: 0x%x.\n"
         , core_state, set, hpd_intr, intr2, intr3, intr4);

#if DEBUG_LEVEL
    dump_intr_status(1, hpd_intr);
    dump_intr_status(2, intr2);
    dump_intr_status(3, intr3);
    dump_intr_status(4, intr4);
#endif

    if ((hpd_intr & (BIT_INTR1_HPD | BIT_INTR1_RSEN)) ||
        (intr2 & BIT_INTR2_TCLK_STBL)) {
        if  ((set & BIT_HDMI_PSTABLE) == 0) {/*power on when HDMI is connceted, HPD will toggle*/
            /*tmp state, discard*/
            logd("set %x laststatus %x\n", set, laststatus);
        } else if  ((set & (BIT_RSEN | BIT_HPD_PIN)) == (BIT_RSEN | BIT_HPD_PIN)) {
            logd("connect, \n");
            hdmi_state = HDMI_CONNECT;
            if (0 == first_hpd) {
                hdmi_state |= HDMI_FIRST_HPD;
                first_hpd++;
                logd("first hpd\n");
            }
        } else {
            logd("Disconnect\n");
            first_hpd = 0;
            hdmi_state = HDMI_DISCONNECT;
        }
        laststatus = set;
    }

#if USE_HDCP
    if (intr2 & HDMI_IP_CORE_SYSTEM_INTR2_BCAP) {
        logd("BCAP\n");
        hdmi_state |= HDMI_BCAP;
        if (REPEATER_AUTH_REQ == hdcp_get_auth_status()) {
            hdcp_auto_ksvready_check( false );
        }
    }

    if (intr3 & HDMI_IP_CORE_SYSTEM_INTR3_RI_ERR) {
        logd("ri err\n");
        hdmi_state |= HDMI_RI_ERR;
    }

    if (intr3 & MASK_AUTO_RI_9134_SPECIFIC) {
        logd("9134 ri err\n");
        hdmi_state |= HDMI_RI_ERR;
    }
#endif

    return hdmi_state;
}

/******************************************************************************
* Function:       hw_re_detect
* Description:    get cable connect state by register state
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:         state of cable connect
* Others:
*******************************************************************************/
int hw_rx_detect(void)
{
    int hpd_state   = 0;
    int loop        = 0;

    do {
        /* read connect state from register */
        hpd_state = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_SYS_STAT);
        if ((hpd_state & (BIT_RSEN | BIT_HPD_PIN)) == (BIT_RSEN | BIT_HPD_PIN)) {
            return HDMI_CONNECT;
        }
        loop ++;
    } while (loop < 100);

    return HDMI_DISCONNECT;
}

/******************************************************************************
* Function:       hw_phy_power_on
* Description:    turn on hdmi phy, may depend on tmds later
* Data Accessed:
* Data Updated:
* Input:          
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_phy_power_on(void)
{
    IN_FUNCTION;
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_TDMS_CTL2, 1, 5, 5);
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_TDMS_CTL3, 1, 0, 0);
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_BIST_INSTRL, 1, 4, 4);

    /*
    0   8'b1101_0100   8'hD4 or 8'hD1 will do.
    1   8'b0011_1xx1   Red colored bit should be 1'b1, so that external resistor value is used for the output swing value.
    3   8'b1100_0000   DVI encoder of phy used,Red colored bit should be 1'b1, so that external resistor value is used for the output swing value.
    */    
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_TDMS_CTL1, 1, 7, 7);    
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_TDMS_CTL2, 3, 4, 3);
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_BIST_CNTL, 3, 7, 6);
    mdelay(5);
    
    OUT_FUNCTION;
    return;
}

/******************************************************************************
* Function:       hw_phy_power_off
* Description:    turn off hdmi phy
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_phy_power_off(void)
{
    IN_FUNCTION;
    
    /*0xFA205804[5]= 0 output disable*/
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_TDMS_CTL2, 0, 5, 5);
    /*0xFA205808[0]= 0 phy power down*/
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_TDMS_CTL3, 0, 0, 0);
    /*0xFA205818[4]= 0 close osc_cec_clk*/
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_BIST_INSTRL, 0, 4, 4);

    OUT_FUNCTION;
    
    return;
}

/******************************************************************************
* Function:       hw_phy_configure_aclk_dig
* Description:    config phy aclk dig
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_phy_configure_aclk_dig(int factor)
{
    int aclk_factor = ACLK_MULT_FACTOR_1;
    switch(factor){
        case 1:
            aclk_factor = ACLK_MULT_FACTOR_1; 
            break;
        case 2:
            aclk_factor = ACLK_MULT_FACTOR_2; 
            break;
        case 3:
            aclk_factor = ACLK_MULT_FACTOR_3; 
            break;
        case 4:
            aclk_factor = ACLK_MULT_FACTOR_4; 
            break;
        case 5:
            aclk_factor = ACLK_MULT_FACTOR_5; 
            break;
        case 6:
            aclk_factor = ACLK_MULT_FACTOR_6; 
            break;
        case 10:
            aclk_factor = ACLK_MULT_FACTOR_10; 
            break;
        default:
            loge("factory %d is incorrect!!\n",factor);
            break;            
    }
    
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_BIST_CNTL, aclk_factor >> 2, BIT_ACLK_COUNT2, BIT_ACLK_COUNT2);
    REG_FLD_MOD(hw_res.base_phy, HDMI_PHY_TDMS_CNTL9, aclk_factor, BIT_ACLK_COUNT1, BIT_ACLK_COUNT0);

    logd("set factor:%d factor_reg:%d\n",factor,aclk_factor);
    
    return;
}

/******************************************************************************
* Function:       hw_core_power_on
* Description:    turn on hdmi device
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_core_power_on(void)
{
    IN_FUNCTION;

#if HDMI_CHIP_VER

    if (!hdmi.in_reset) {

#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
        pm_qos_add_request(&hdmi.qos_request, PM_QOS_DDR_MIN_PROFILE, HDMI_PM_QOS_DDR_MIN_FREQ);
        logd("set ddr freq:%d\n", HDMI_PM_QOS_DDR_MIN_FREQ);
#endif

        if (regulator_enable(hw_res.edc_vcc) != 0) {
            loge("failed to enable edc-vcc regulator.\n");
        }
        /*enable hdmi clock*/
        hw_enable_clocks();
    
        /*set io mux hdmi mode*/
        hw_set_iomux(NORMAL);

        if(!hw_support_mhl()) {
            if (regulator_enable(hw_res.charge_pump) != 0) {
                loge("failed to enable charge_pump regulator.\n");
            }
        }

        mdelay(5);
    }

    /*init irq*/
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INT_CNTRL,0x0);

    /* hdmi core power on */
    REG_FLD_MOD(HDMI_CORE_SYS, HDMI_CORE_CTRL1, 0x1, 0, 0);

    REG_FLD_MOD(HDMI_CORE_AV, HDMI_CORE_AV_DPD, 0x0, 1, 1);

    hw_core_mute_audio(true);

#else
    logd("in hw_core_power_on ........\n");
    /* LDO Power on */
    hisik3_gpio_set_direction(GPIO_0_1, EGPIO_DIR_OUTPUT);
    hisik3_gpio_setvalue(GPIO_0_1, EGPIO_DATA_HIGH);
    msleep(100);

    /* HDMI PHY reset */
    hisik3_gpio_set_direction(GPIO_0_0, EGPIO_DIR_OUTPUT);
    hisik3_gpio_setvalue(GPIO_0_0, EGPIO_DATA_HIGH);
    msleep(10);

    hisik3_gpio_set_direction(GPIO_0_0, EGPIO_DIR_OUTPUT);
    hisik3_gpio_setvalue(GPIO_0_0, EGPIO_DATA_LOW);
    msleep(10);

    hisik3_gpio_set_direction(GPIO_0_0, EGPIO_DIR_OUTPUT);
    hisik3_gpio_setvalue(GPIO_0_0, EGPIO_DATA_HIGH);
    msleep(10);

    /* DDC Power on */
    hisik3_gpio_set_direction(GPIO_0_2, EGPIO_DIR_OUTPUT);
    hisik3_gpio_setvalue(GPIO_0_2, EGPIO_DATA_HIGH);
    msleep(10);

    /* hdmi par sel */
    hisik3_gpio_set_direction(GPIO_0_3, EGPIO_DIR_OUTPUT);
    hisik3_gpio_setvalue(GPIO_0_3, EGPIO_DATA_LOW);
    msleep(10);
#endif

    hdmi.state = HDMI_DISPLAY_ACTIVE;

    OUT_FUNCTION;

    return ;
}

/******************************************************************************
* Function:       hw_core_power_off
* Description:    turn off hdmi device
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
void  hw_core_power_off(void)
{
#if HDMI_CHIP_VER
    int ret = 0;
#endif  

    IN_FUNCTION;

#if HDMI_CHIP_VER

    /* Turn off DDC */
     REG_FLD_MOD(HDMI_CORE_AV, HDMI_CORE_AV_DPD, 0x0, 2, 1);

    /* Turn off core */
    REG_FLD_MOD(HDMI_CORE_SYS, HDMI_CORE_CTRL1, 0x0, 0, 0);

    if(!hdmi.in_reset) {
        /* en_chg_pump_int off */
        if(!hw_support_mhl()) {
            regulator_disable(hw_res.charge_pump);
        }
        /*set io mux low power*/
        ret = hw_set_iomux(LOWPOWER);
        if (ret) {
            loge("io mux set fail: %d.\n", ret);
        }
    
        /*disable hdmi clocks*/
        hw_disable_clocks();
        mdelay(2);

        /*disable edc1 regulator*/
        regulator_disable(hw_res.edc_vcc);
        mdelay(2);

#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
        if (hdmi.has_request_ddr) {
            hdmi.has_request_ddr = false;
        pm_qos_remove_request(&hdmi.qos_request);
        logd("remove qos request\n");
        }
#endif

    } else {
        msleep(100);
    }

#else
    hisik3_gpio_set_direction(GPIO_0_0, EGPIO_DIR_OUTPUT);
    hisik3_gpio_setvalue(GPIO_0_0, EGPIO_DATA_LOW);
    msleep(10);

    /* LDO Power on */
    hisik3_gpio_set_direction(GPIO_0_1, EGPIO_DIR_OUTPUT);
    hisik3_gpio_setvalue(GPIO_0_1, EGPIO_DATA_LOW);
    msleep(10);

    /* DDC Power on */
    hisik3_gpio_set_direction(GPIO_0_2, EGPIO_DIR_OUTPUT);
    hisik3_gpio_setvalue(GPIO_0_2, EGPIO_DATA_LOW);
    msleep(10);
#endif

    return;
}

/******************************************************************************
* Function:       hw_set_clk_rate
* Description:    set clocks rate
* Data Accessed:
* Data Updated:
* Input:          clock index, rate
* Output:
* Return:
* Others:
*******************************************************************************/
int hw_set_clk_rate(u32 clks, u32 rate){
    int ret = -1;
    if (clks & HDMI_CLK_LDI1) {
        ret = clk_set_rate(hw_res.clk_ldi1, rate);
        if(ret){
            loge("enable ldi1 clock error %d\n",ret);
        }
    }
    if (clks & HDMI_CLK_EDC1) {
        ret = clk_set_rate(hw_res.clk_edc1, rate*12/10);
        if(ret){
            loge("enable ldi1 clock error %d\n",ret);
        }
    }
    return ret;
}


/******************************************************************************
* Function:       enable_clk
* Description:    enable some clocks
* Data Accessed:
* Data Updated:
* Input:          clock index
* Output:
* Return:
* Others:
*******************************************************************************/
static void enable_clk(u32 clks)
{
    int ret = 0;

    IN_FUNCTION;

    logd("enable clk 0x%x. \n", clks);
    
    if (clks & HDMI_CLK_EDC1) {
        ret = clk_enable(hw_res.clk_edc1);
        if(ret){
            loge("enable edc1 clock error %d\n",ret);
        }
    }
    
    if (clks & HDMI_CLK_LDI1) {
        ret = clk_enable(hw_res.clk_ldi1);
        if(ret){
            loge("enable ldi1 clock error %d\n",ret);
        }
    }

    if (clks & HDMI_CLK) {
    	ret = clk_enable(hw_res.clk_pclk_hdmi);
        if (ret) {
            loge("enable pclk hdmi clock error %d\n",ret);
        }

        ret = clk_enable(hw_res.clk_hdmi);
        if (ret) {
            loge("enable hdmi clock error %d\n",ret);
        }
    }

#if defined(CONFIG_HDMI_CEC_ENABLE)
    if (clks & HDMI_CLK_CEC) {
        ret = clk_enable(hw_res.clk_cec);
        if(ret){
            loge("enable cec clock error %d\n",ret);
        }
    }
#endif
    OUT_FUNCTION;

    return;
}

/******************************************************************************
* Function:       disable_clk
* Description:    disable some clocks
* Data Accessed:
* Data Updated:
* Input:          clock index
* Output:
* Return:
* Others:
*******************************************************************************/
static void disable_clk(u32 clks)
{
    IN_FUNCTION;

    logi("disable clk 0x%x. \n", clks);

    if (clks & HDMI_CLK) {
        clk_disable(hw_res.clk_hdmi);
        clk_disable(hw_res.clk_pclk_hdmi);
    }    
    
    if (clks & HDMI_CLK_LDI1) {
        clk_disable(hw_res.clk_ldi1);
    }
    
    if (clks & HDMI_CLK_EDC1) {
        clk_disable(hw_res.clk_edc1);
    }

#if defined(CONFIG_HDMI_CEC_ENABLE)
    if (clks & HDMI_CLK_CEC) {
        clk_disable(hw_res.clk_cec);
    }
#endif

    OUT_FUNCTION;
    return;
}

/******************************************************************************
* Function:       hw_enable_clocks
* Description:    enadle all clocks
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_enable_clocks(void)
{
    #define HDMI_APB_CLK_RATE 96000000
    struct clk * clk_apb = NULL;

    enable_clk(HDMI_CLK | HDMI_CLK_EDC1 | HDMI_CLK_LDI1 | HDMI_CLK_CEC);

    clk_apb = clk_get(NULL, HDMI_APB_CLK_NAME);
    if (IS_ERR(clk_apb)) {            
        loge("hdmi, %s: failed to get APB clock\n", __func__);
        return;
    }

    if (clk_set_rate(clk_apb, HDMI_APB_CLK_RATE)) {
        loge("set APB clock error\n");        
    }
    clk_put(clk_apb);
    
    return;
}

/******************************************************************************
* Function:       hw_disable_clocks
* Description:    disable all clocks
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_disable_clocks(void)
{
    disable_clk(HDMI_CLK | HDMI_CLK_EDC1 | HDMI_CLK_LDI1);
    return;
}

#if HDMI_CHIP_VER
/******************************************************************************
* Function:
* Description:
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
int hw_set_iomux(int mode)
{
    int ret = 0;
    IN_FUNCTION;

    ret = blockmux_set(hw_res.iomux_block, hw_res.iomux_block_config, mode);
    if (ret) {
        loge("failed to set iomux to %d mode.\n", mode);
    }

    OUT_FUNCTION;

    return ret;
}
#endif

/******************************************************************************
* Function:       hw_get_irq
* Description:    get object of irq
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
int hw_get_irq()
{
    return hw_res.irq;
}

/******************************************************************************
* Function:       hw_free_resources
* Description:    free resources, irq, clk.
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
*******************************************************************************/
void hw_free_resources(void)
{

    IN_FUNCTION;

    if (hw_res.irq && hdmi.irq_is_init) {
        free_irq(hw_res.irq, (void *)0);
    }

    if (hw_res.clk_hdmi) {
        clk_put(hw_res.clk_hdmi);
        hw_res.clk_hdmi = NULL;
    }

    
    if (hw_res.clk_edc1) {
        clk_put(hw_res.clk_edc1);
        hw_res.clk_edc1 = NULL;
    }

    if (hw_res.clk_ldi1) {
        clk_put(hw_res.clk_ldi1);
        hw_res.clk_ldi1 = NULL;
    }

#if defined(CONFIG_HDMI_CEC_ENABLE)
    if (hw_res.clk_cec) {
        clk_put(hw_res.clk_cec);
        hw_res.clk_cec = NULL;
    }
#endif

if (hw_res.edc_vcc) {
        regulator_put(hw_res.edc_vcc);
        hw_res.edc_vcc = NULL;
    }
    
    if (hw_res.charge_pump) {
        regulator_put(hw_res.charge_pump);
        hw_res.charge_pump = NULL;
    }

    OUT_FUNCTION;

    return;
}

/******************************************************************************
* Function:       hw_get_resources
* Description:    get resources, irq, clk, and config base address
* Data Accessed:
* Data Updated:
* Input:          platform device
* Output:
* Return:
* Others:
*******************************************************************************/
int hw_get_resources(struct platform_device *pdev)
{
    int ret = 0;
    struct resource *res = 0;

    BUG_ON(NULL == pdev);

    IN_FUNCTION;

    memset(&hw_res, 0, sizeof(hdmi_hw_res));

    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, HDMI_REG_NAME);
    if (!res) {
        loge("get hdmi reg resource fail.\n");
        OUT_FUNCTION;
        return -ENXIO;
    }

    hw_res.base_core = (u32) IO_ADDRESS(res->start);
    hw_res.base_core_av = hw_res.base_core + HDMI_AV_REG_OFFSET;
    hw_res.base_phy = hw_res.base_core + HDMI_PHY_REG_OFFSET;
    hw_res.base_cec = hw_res.base_core + HDMI_CEC_REG_OFFSET;

    logi("base core: 0x%x\n", (int)hw_res.base_core);

#if HDMI_CHIP_VER
    hw_res.iomux_block = iomux_get_block(HDMI_IOMUX_NAME);
    if (!hw_res.iomux_block) {
        loge("get hdmi clock fail.\n");
        ret = PTR_ERR(hw_res.iomux_block);
        goto err;
    }

    hw_res.iomux_block_config = iomux_get_blockconfig(HDMI_IOMUX_NAME);
    if (!hw_res.iomux_block_config) {
        loge("get hdmi clock config fail.\n");
        ret = PTR_ERR(hw_res.iomux_block_config);
        goto err;
    }
#endif

    hw_res.clk_hdmi = clk_get(NULL, HDMI_CLK_NAME);
    if (IS_ERR(hw_res.clk_hdmi)) {
        loge("get hdmi clk fail.\n");
        ret = PTR_ERR(hw_res.clk_hdmi);
        hw_res.clk_hdmi = NULL;
        goto err;
    }
    
    res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, HDMI_IRQ_NAME);
    if (!res) {
        loge("get hdmi irq fail.\n");
        ret = -ENXIO;
        goto err;
    }

    hw_res.irq = res->start;

    hw_res.edc_vcc = regulator_get(NULL,  HDMI_EDC1_VCC_NAME);
    if (IS_ERR(hw_res.edc_vcc)) {
        hw_res.edc_vcc = NULL;
        loge("hdmi, %s: failed to get edc1-vcc regulator\n", __func__);
        goto err;
    }
    
    hw_res.charge_pump = regulator_get(NULL,  HDMI_CHARGE_PUMP_NAME);
    if (IS_ERR(hw_res.charge_pump)) {
        hw_res.charge_pump = NULL;
        loge("hdmi, %s: failed to get charge_pump regulator\n", __func__);
        goto err;
    }

    hw_res.clk_pclk_hdmi = clk_get(NULL,  "clk_pclk_hdmi");
    if (IS_ERR(hw_res.clk_pclk_hdmi)) {
        hw_res.clk_pclk_hdmi = NULL;
        loge("hdmi, %s: failed to get pclk hdmi clock\n", __func__);
        goto err;
    }

    hw_res.clk_edc1 = clk_get(NULL,  HDMI_EDC1_CLK_NAME);
    if (IS_ERR(hw_res.clk_edc1)) {
        hw_res.clk_edc1 = NULL;
        loge("hdmi, %s: failed to get edc1 clock\n", __func__);
        goto err;
    }
    
    hw_res.clk_ldi1 = clk_get(NULL,  HDMI_LDI1_CLK_NAME);
    if (IS_ERR(hw_res.clk_ldi1)) {
        hw_res.clk_ldi1 = NULL;
        loge("hdmi, %s: failed to get ldi1 clock\n", __func__);
        goto err;
    }

#if defined(CONFIG_HDMI_CEC_ENABLE)
    hw_res.clk_cec = clk_get(NULL,  HDMI_CEC_CLK_NAME);
    if (IS_ERR(hw_res.clk_cec)) {
        hw_res.clk_cec = NULL;
        loge("hdmi, %s: failed to get cec clock\n", __func__);
        goto err;
    }
#endif

    OUT_FUNCTION;
    return 0;

err:
    hw_free_resources();
    return ret;
}

bool hw_support_mhl()
{
#if ENABLE_MOCK_HDMI_TO_MHL
    return false;
#endif
#ifdef SUPPORT_MHL
    return true;
#else
    logi("this device don't support mhl.\n");
    return false;
#endif    
}

