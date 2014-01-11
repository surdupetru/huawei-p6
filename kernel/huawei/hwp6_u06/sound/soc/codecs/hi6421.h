/*
* HI6421 ALSA Soc codec driver
*/

#ifndef HI6421_H
#define HI6421_H

#define HI6421_REG_AUDID_LDO        ( 0x36 )

#define HI6421_REG_CLK_CTL          ( 0x4F )

#define HI6421_REG_IRQ2             ( 0x02 )
#define HI6421_HDS_PLUG_IN_BIT                  ( 1 )
#define HI6421_HDS_PLUG_OUT_BIT                 ( 0 )

#define HI6421_REG_IRQ3             ( 0x03 )
#define     HI6421_HDS_BTN_RELEASE_BIT          ( 3 )
#define     HI6421_HDS_BTN_PRESS_BIT            ( 2 )

#define HI6421_REG_STATUS0          ( 0x07 )
#define     HI6421_PLUGCOMP                     ( 1 << 0 )

#define HI6421_REG_STATUS1          ( 0x08 )
#define     HI6421_HKCOMP                       ( 1 << 1 )

#define HI6421_REG_CIC_DMNS_CONFIG  ( 0x90 )
#define     HI6421_DMNS_CFG_BIT                 ( 6 )
#define     HI6421_ADCL_CIC_BIT                 ( 3 )
#define     HI6421_ADCR_CIC_BIT                 ( 0 )

#define HI6421_REG_DMNS_GAIN        ( 0x91 )

/* 92 : RO : NOISE LEVEL DETECTOR */

#define HI6421_REG_AHARM_GAIN       ( 0x93 )

/* 94 - 99 : AGCL CONFIGS */
#define HI6421_REG_AGC_C_L_CONFIG_1 ( 0x94 )
#define HI6421_REG_AGC_C_L_CONFIG_2 ( 0x95 )
#define HI6421_REG_AGC_C_L_CONFIG_3 ( 0x96 )
#define HI6421_REG_AGC_C_L_CONFIG_4 ( 0x97 )
#define HI6421_REG_AGC_C_L_CONFIG_5 ( 0x98 )
#define     HI6421_AGC_C_L_BYPASS_BIT            ( 7 )
#define HI6421_REG_AGC_C_L_GAIN     ( 0x99 )

/* 9A - 9F : AGCR CONFIGS */
#define HI6421_REG_AGC_C_R_CONFIG_1 ( 0x9A )
#define HI6421_REG_AGC_C_R_CONFIG_2 ( 0x9B )
#define HI6421_REG_AGC_C_R_CONFIG_3 ( 0x9C )
#define HI6421_REG_AGC_C_R_CONFIG_4 ( 0x9D )
#define HI6421_REG_AGC_C_R_CONFIG_5 ( 0x9E )
#define     HI6421_AGC_C_R_BYPASS_BIT            ( 7 )
#define HI6421_REG_AGC_C_R_GAIN     ( 0x9F )

/* A0 : ADCL DELAY CONFIG */

#define HI6421_REG_GAIN_FRACT       ( 0xA1 )
#define     HI6421_ADCL_POST_DELAY_BYPASS       ( 6 ) // NOT USE
#define     HI6421_AGC_C_L_GAIN_FRACT_BIT       ( 0 )
#define     HI6421_AGC_C_R_GAIN_FRACT_BIT       ( 1 )
#define     HI6421_MODEMDL_GAIN_FRACT_BIT       ( 4 )
/* V200 */
#define     HI6421_V200_APL_GAIN_FRACT_BIT      ( 2 )
#define     HI6421_V200_APR_GAIN_FRACT_BIT      ( 3 )
#define     HI6421_MODEMUL_GAIN_FRACT_BIT       ( 5 )
/* V220 */
#define     HI6421_MODEMUL_L_GAIN_FRACT_BIT     ( 5 )
#define     HI6421_MODEMUL_R_GAIN_FRACT_BIT     ( 7 )
#define     HI6421_V220_AGC_P_L_GAIN_FRACT_BIT  ( 2 )
#define     HI6421_V220_AGC_P_R_GAIN_FRACT_BIT  ( 3 )

/* V200 */
#define HI6421_APL_GAIN             ( 0xA2 )
#define HI6421_APR_GAIN             ( 0xA3 )
#define HI6421_MODEMD_GAIN          ( 0xA4 )
#define HI6421_MODEMU_GAIN          ( 0xA5 )

/* V220 */
#define HI6421_MODEMUL_R_GAIN       ( 0xA2 )
/* A3 RESERVED */
#define HI6421_MODEMDL_GAIN         ( 0xA4 )
#define HI6421_MODEMUL_L_GAIN       ( 0xA5 )

/* A6 - AD : VOICED EQ CONFIGS */
#define HI6421_MODEMDL_EQ_CONFIG    ( 0xA6 )
#define HI6421_MODEMDL_EQ_125HZ     ( 0xA7 )
#define HI6421_MODEMDL_EQ_250HZ     ( 0xA8 )
#define HI6421_MODEMDL_EQ_500HZ     ( 0xA9 )
#define HI6421_MODEMDL_EQ_1KHZ      ( 0xAA )
#define HI6421_MODEMDL_EQ_2KHZ      ( 0xAB )
#define HI6421_MODEMDL_EQ_HS        ( 0xAC )
#define HI6421_MODEMDL_EQ_LS        ( 0xAD )

/* V200 */
/* AE - B5 : VOICEU EQ CONFIGS */
#define HI6421_MODEMUL_EQ_CONFIG    ( 0xAE )
#define HI6421_MODEMUL_EQ_125HZ     ( 0xAF )
#define HI6421_MODEMUL_EQ_250HZ     ( 0xB0 )
#define HI6421_MODEMUL_EQ_500HZ     ( 0xB1 )
#define HI6421_MODEMUL_EQ_1KHZ      ( 0xB2 )
#define HI6421_MODEMUL_EQ_2KHZ      ( 0xB3 )
#define HI6421_MODEMUL_EQ_HS        ( 0xB4 )
#define HI6421_MODEMUL_EQ_LS        ( 0xB5 )

/* V220 */
/* AE - B3 : PLAYBACK AGCL CONFIGS */
#define HI6421_REG_AGC_P_L_CONFIG_1 ( 0xAE )
#define HI6421_REG_AGC_P_L_CONFIG_2 ( 0xAF )
#define HI6421_REG_AGC_P_L_CONFIG_3 ( 0xB0 )
#define HI6421_REG_AGC_P_L_CONFIG_4 ( 0xB1 )
#define HI6421_REG_AGC_P_L_CONFIG_5 ( 0xB2 )
#define     HI6421_AGC_P_L_BYPASS_BIT            ( 7 )

#define HI6421_REG_AGC_P_L_GAIN     ( 0xB3 )
/* B4 - B5 : RESERVED */

/* B6 : SW CLR REG */

#define HI6421_REG_EN_CFG_1         ( 0xB7 )
#define HI6421_REG_EN_CFG_2         ( 0xB8 )
#define HI6421_REG_EN_CFG_3         ( 0xB9 )
#define HI6421_REG_EN_CFG_4         ( 0xBA )

/* HI6421_REG_EN_CFG_1 */
#define     HI6421_DACL_EN_REG                  ( HI6421_REG_EN_CFG_1 )
#define     HI6421_DACL_EN_BIT                  ( 5 )
#define     HI6421_DACR_EN_REG                  ( HI6421_REG_EN_CFG_1 )
#define     HI6421_DACR_EN_BIT                  ( 4 )
#define     HI6421_DEEMPL_EN_REG                ( HI6421_REG_EN_CFG_1 )
#define     HI6421_DEEMPL_EN_BIT                ( 3 )
#define     HI6421_DEEMPR_EN_REG                ( HI6421_REG_EN_CFG_1 )
#define     HI6421_DEEMPR_EN_BIT                ( 2 )
#define     HI6421_AHARM_EN_REG                 ( HI6421_REG_EN_CFG_1 )
#define     HI6421_AHARM_EN_BIT                 ( 1 )
#define     HI6421_HBF1I_EN_REG                 ( HI6421_REG_EN_CFG_1 )
#define     HI6421_HBF1I_EN_BIT                 ( 0 )
/* V200 */
#define     HI6421_V200_AGC_C_L_EN_REG          ( HI6421_REG_EN_CFG_1 )
#define     HI6421_V200_AGC_C_L_EN_BIT          ( 7 )
#define     HI6421_V200_AP_GAIN_EN_REG          ( HI6421_REG_EN_CFG_1 )
#define     HI6421_V200_AP_GAIN_EN_BIT          ( 6 )
/* V220 */
#define     HI6421_V220_AGC_C_L_EN_REG          ( HI6421_REG_EN_CFG_1 )
#define     HI6421_V220_AGC_C_L_EN_BIT          ( 7 )
#define     HI6421_V220_AGC_C_R_EN_REG          ( HI6421_REG_EN_CFG_1 )
#define     HI6421_V220_AGC_C_R_EN_BIT          ( 6 )

/* HI6421_REG_EN_CFG_2 */
#define     HI6421_SRCIU_EN_REG                 ( HI6421_REG_EN_CFG_2 )
#define     HI6421_SRCIU_EN_BIT                 ( 5 )
#define     HI6421_HBF1DR_EN_REG                ( HI6421_REG_EN_CFG_2 )
#define     HI6421_HBF1DR_EN_BIT                ( 2 )
#define     HI6421_HBF1DL_EN_REG                ( HI6421_REG_EN_CFG_2 )
#define     HI6421_HBF1DL_EN_BIT                ( 1 )
/* V200 */
#define     HI6421_MODEM_UL_GAIN_EN_REG         ( HI6421_REG_EN_CFG_2 )
#define     HI6421_MODEM_UL_GAIN_EN_BIT         ( 6 )
#define     HI6421_HBFVD_EN_REG                 ( HI6421_REG_EN_CFG_2 )
#define     HI6421_HBFVD_EN_BIT                 ( 4 )
#define     HI6421_MODEM_UL_EQ_EN_REG           ( HI6421_REG_EN_CFG_2 )
#define     HI6421_MODEM_UL_EQ_EN_BIT           ( 3 )
#define     HI6421_V200_AGC_C_R_EN_REG          ( HI6421_REG_EN_CFG_2 )
#define     HI6421_V200_AGC_C_R_EN_BIT          ( 0 )
/* V220 */
#define     HI6421_V220_AGC_P_L_EN_REG          ( HI6421_REG_EN_CFG_2 )
#define     HI6421_V220_AGC_P_L_EN_BIT          ( 7 )
#define     HI6421_MODEM_UL_L_GAIN_EN_REG       ( HI6421_REG_EN_CFG_2 )
#define     HI6421_MODEM_UL_L_GAIN_EN_BIT       ( 6 )
#define     HI6421_MODEM_UL_L_HBFVD_EN_REG      ( HI6421_REG_EN_CFG_2 )
#define     HI6421_MODEM_UL_L_HBFVD_EN_BIT      ( 4 )
#define     HI6421_V220_AGC_P_R_EN_REG          ( HI6421_REG_EN_CFG_2 )
#define     HI6421_V220_AGC_P_R_EN_BIT          ( 0 )

/* HI6421_REG_EN_CFG_3 */
#define     HI6421_DACV_EN_REG                  ( HI6421_REG_EN_CFG_3 )
#define     HI6421_DACV_EN_BIT                  ( 6 )
#define     HI6421_ADCR_EN_REG                  ( HI6421_REG_EN_CFG_3 )
#define     HI6421_ADCR_EN_BIT                  ( 5 )
#define     HI6421_ADCL_EN_REG                  ( HI6421_REG_EN_CFG_3 )
#define     HI6421_ADCL_EN_BIT                  ( 4 )
#define     HI6421_DMNS_EN_REG                  ( HI6421_REG_EN_CFG_3 )
#define     HI6421_DMNS_EN_BIT                  ( 3 )
#define     HI6421_SRCID_EN_REG                 ( HI6421_REG_EN_CFG_3 )
#define     HI6421_SRCID_EN_BIT                 ( 2 )
#define     HI6421_HBFVI_EN_REG                 ( HI6421_REG_EN_CFG_3 )
#define     HI6421_HBFVI_EN_BIT                 ( 1 )
#define     HI6421_MODEM_DL_EQ_EN_REG           ( HI6421_REG_EN_CFG_3 )
#define     HI6421_MODEM_DL_EQ_EN_BIT           ( 0 )

/* HI6421_REG_EN_CFG_4 */
#define     HI6421_AP_INTERFACE_EN_REG          ( HI6421_REG_EN_CFG_4 )
#define     HI6421_AP_INTERFACE_EN_BIT          ( 5 )
#define     HI6421_MODEM_DL_GAIN_EN_REG         ( HI6421_REG_EN_CFG_4 )
#define     HI6421_MODEM_DL_GAIN_EN_BIT         ( 4 )
#define     HI6421_BT_INTERFACE_EN_REG          ( HI6421_REG_EN_CFG_4 )
#define     HI6421_BT_INTERFACE_EN_BIT          ( 3 )
#define     HI6421_SRCID_I2_EN_REG              ( HI6421_REG_EN_CFG_4 )
#define     HI6421_SRCID_I2_EN_BIT              ( 2 )
#define     HI6421_SRCIU_I2_EN_REG              ( HI6421_REG_EN_CFG_4 )
#define     HI6421_SRCIU_I2_EN_BIT              ( 1 )
#define     HI6421_DIGMIC_EN_REG                ( HI6421_REG_EN_CFG_4 )
#define     HI6421_DIGMIC_EN_BIT                ( 0 )
/* V220 */
#define     HI6421_MODEM_UL_R_HBFVD_EN_REG      ( HI6421_REG_EN_CFG_4 )
#define     HI6421_MODEM_UL_R_HBFVD_EN_BIT      ( 7 )
#define     HI6421_MODEM_UL_R_GAIN_EN_REG       ( HI6421_REG_EN_CFG_4 )
#define     HI6421_MODEM_UL_R_GAIN_EN_BIT       ( 6 )

#define HI6421_REG_AP_CONFIG        ( 0xBB )
#define HI6421_REG_BT_CONFIG        ( 0xBC )
#define HI6421_REG_MODEM_CONFIG     ( 0xBD )

#define     HI6421_INTERFACE_HZ_BIT             ( 7 )
#define     HI6421_INTERFACE_DATABITS_BIT       ( 5 )
#define     HI6421_INTERFACE_DATATYPE_BIT       ( 4 )
#define     HI6421_INTERFACE_PORT_BIT           ( 3 )
#define     HI6421_INTERFACE_EDGE_BIT           ( 2 )
#define     HI6421_INTERFACE_FSP_BIT            ( 1 )

#define HI6421_REG_FREQ_CONFIG      ( 0xBE )
#define     HI6421_FREQ_AP_BIT                  ( 3 )
#define     HI6421_FREQ_AP_48KHZ                ( 0 << HI6421_FREQ_AP_BIT )
#define     HI6421_FREQ_AP_96KHZ                ( 1 << HI6421_FREQ_AP_BIT )
#define     HI6421_FREQ_AP_MASK                 ( 1 << HI6421_FREQ_AP_BIT )
#define     HI6421_FREQ_BT_BIT                  ( 1 )
#define     HI6421_FREQ_BT_8KHZ                 ( 0 << HI6421_FREQ_BT_BIT )
#define     HI6421_FREQ_BT_16KHZ                ( 1 << HI6421_FREQ_BT_BIT )
#define     HI6421_FREQ_BT_48KHZ                ( 2 << HI6421_FREQ_BT_BIT )
#define     HI6421_FREQ_BT_MASK                 ( 3 << HI6421_FREQ_BT_BIT )
#define     HI6421_FREQ_MODEM_BIT               ( 0 )
#define     HI6421_FREQ_MODEM_8KHZ              ( 0 << HI6421_FREQ_MODEM_BIT )
#define     HI6421_FREQ_MODEM_16KHZ             ( 1 << HI6421_FREQ_MODEM_BIT )
#define     HI6421_FREQ_MODEM_MASK              ( 1 << HI6421_FREQ_MODEM_BIT )

#define HI6421_REG_BYPASS_CONFIG    ( 0xBF )
#define     HI6421_BYPASS_ADCL_HPF_BIT          ( 6 )
#define     HI6421_BYPASS_ADCR_HPF_BIT          ( 5 )
#define     HI6421_BYPASS_DACL_DEEMP_BIT        ( 2 )
#define     HI6421_BYPASS_DACR_DEEMP_BIT        ( 4 )
#define     HI6421_BYPASS_DACL_AHARM_BIT        ( 0 )
#define     HI6421_BYPASS_DACR_AHARM_BIT        ( 3 )
#define     HI6421_BYPASS_DACLR_MIX_BIT         ( 1 )

#define HI6421_REG_MUX_CONFIG       ( 0xC0 )
#define     HI6421_MUX_AGCLR_MIX_BIT            ( 6 )
#define     HI6421_MUX_BT_BIT                   ( 4 )
#define     HI6421_MUX_DENOISE_BIT              ( 3 )
#define     HI6421_MUX_VOICEU_BIT               ( 2 )
#define     HI6421_MUX_AGCR_BIT                 ( 1 )
#define     HI6421_MUX_AGCL_BIT                 ( 0 )
#define     HI6421_BT_INCALL_BIT                ( 5 )
#define     HI6421_BT_PLAYBACK_BIT              ( 4 )
/* MUX_VOICEU_BIT & MUX_AGCL_BIT CAN NOT SET 0 TOGETHER */

#define HI6421_DMIC_SDM_CFG         ( 0xC1 )
#define     HI6421_DACL_SDM_DITHER              ( 3 )
#define     HI6421_DACR_SDM_DITHER              ( 4 )
#define     HI6421_DACV_SDM_DITHER              ( 5 )

/* V200 */
/* C2 - C8 RO DEBUG REGISTERS */

/* V220 */
/* C2 - C7 PLAYBACK AGCR CONFIGS */
#define HI6421_REG_AGC_P_R_CONFIG_1 ( 0xC2 )
#define HI6421_REG_AGC_P_R_CONFIG_2 ( 0xC3 )
#define HI6421_REG_AGC_P_R_CONFIG_3 ( 0xC4 )
#define HI6421_REG_AGC_P_R_CONFIG_4 ( 0xC5 )
#define HI6421_REG_AGC_P_R_CONFIG_5 ( 0xC6 )
#define     HI6421_AGC_P_R_BYPASS_BIT            ( 7 )
#define HI6421_REG_AGC_P_R_GAIN     ( 0xC7 )
/* C8 : RESERVED */

#define HI6421_REG_CODECENA_1       ( 0xC9 )
#define HI6421_REG_CODECENA_2       ( 0xCA )
#define HI6421_REG_CODECENA_3       ( 0xCB )
#define HI6421_REG_CODECENA_4       ( 0xCC )

#define     HI6421_ADCL_PD_REG                  ( HI6421_REG_CODECENA_1 )
#define     HI6421_ADCL_PD_BIT                  ( 6 )
#define     HI6421_ADCR_PD_REG                  ( HI6421_REG_CODECENA_1 )
#define     HI6421_ADCR_PD_BIT                  ( 5 )
#define     HI6421_MAINPGA_PD_REG               ( HI6421_REG_CODECENA_1 )
#define     HI6421_MAINPGA_PD_BIT               ( 4 )
#define     HI6421_SUBPGA_PD_REG                ( HI6421_REG_CODECENA_1 )
#define     HI6421_SUBPGA_PD_BIT                ( 3 )
#define     HI6421_SIDEPGA_PD_REG               ( HI6421_REG_CODECENA_1 )
#define     HI6421_SIDEPGA_PD_BIT               ( 2 )
#define     HI6421_LINEINLPGA_PD_REG            ( HI6421_REG_CODECENA_1 )
#define     HI6421_LINEINLPGA_PD_BIT            ( 1 )
#define     HI6421_LINEINRPGA_PD_REG            ( HI6421_REG_CODECENA_1 )
#define     HI6421_LINEINRPGA_PD_BIT            ( 0 )

#define     HI6421_MIXERINL_PD_REG              ( HI6421_REG_CODECENA_2 )
#define     HI6421_MIXERINL_PD_BIT              ( 7 )
#define     HI6421_MIXERINR_PD_REG              ( HI6421_REG_CODECENA_2 )
#define     HI6421_MIXERINR_PD_BIT              ( 6 )
#define     HI6421_SUBMICBIAS_PD_REG            ( HI6421_REG_CODECENA_2 )
#define     HI6421_SUBMICBIAS_PD_BIT            ( 5 )
#define     HI6421_MAINMICBIAS_PD_REG           ( HI6421_REG_CODECENA_2 )
#define     HI6421_MAINMICBIAS_PD_BIT           ( 4 )
#define     HI6421_PLL_PD_REG                   ( HI6421_REG_CODECENA_2 )
#define     HI6421_PLL_PD_BIT                   ( 3 )
#define     HI6421_IBIAS_PD_REG                 ( HI6421_REG_CODECENA_2 )
#define     HI6421_IBIAS_PD_BIT                 ( 2 )
#define     HI6421_VREF_SEL_REG                 ( HI6421_REG_CODECENA_2 )
#define     HI6421_VREF_SEL_BIT                 ( 0 )
            /* sel 5k : PLL STABLE TIME :20ms */

#define     HI6421_DACL_PD_REG                  ( HI6421_REG_CODECENA_3 )
#define     HI6421_DACL_PD_BIT                  ( 7 )
#define     HI6421_DACR_PD_REG                  ( HI6421_REG_CODECENA_3 )
#define     HI6421_DACR_PD_BIT                  ( 6 )
#define     HI6421_DACV_PD_REG                  ( HI6421_REG_CODECENA_3 )
#define     HI6421_DACV_PD_BIT                  ( 5 )
#define     HI6421_MIXEROUTL_PD_REG             ( HI6421_REG_CODECENA_3 )
#define     HI6421_MIXEROUTL_PD_BIT             ( 4 )
#define     HI6421_MIXEROUTR_PD_REG             ( HI6421_REG_CODECENA_3 )
#define     HI6421_MIXEROUTR_PD_BIT             ( 3 )
#define     HI6421_MIXERLINEOUTL_PD_REG         ( HI6421_REG_CODECENA_3 )
#define     HI6421_MIXERLINEOUTL_PD_BIT         ( 2 )
#define     HI6421_MIXERLINEOUTR_PD_REG         ( HI6421_REG_CODECENA_3 )
#define     HI6421_MIXERLINEOUTR_PD_BIT         ( 1 )
#define     HI6421_MIXERMONO_PD_REG             ( HI6421_REG_CODECENA_3 )
#define     HI6421_MIXERMONO_PD_BIT             ( 0 )

#define     HI6421_LINEOUTL_PD_REG              ( HI6421_REG_CODECENA_4 )
#define     HI6421_LINEOUTL_PD_BIT              ( 6 )
#define     HI6421_LINEOUTR_PD_REG              ( HI6421_REG_CODECENA_4 )
#define     HI6421_LINEOUTR_PD_BIT              ( 5 )
#define     HI6421_HEADSETL_PD_REG              ( HI6421_REG_CODECENA_4 )
#define     HI6421_HEADSETL_PD_BIT              ( 4 )
#define     HI6421_HEADSETR_PD_REG              ( HI6421_REG_CODECENA_4 )
#define     HI6421_HEADSETR_PD_BIT              ( 3 )
#define     HI6421_VMID_PD_REG                  ( HI6421_REG_CODECENA_4 )
#define     HI6421_VMID_PD_BIT                  ( 2 )
#define     HI6421_EARPIECE_PD_REG              ( HI6421_REG_CODECENA_4 )
#define     HI6421_EARPIECE_PD_BIT              ( 1 )
#define     HI6421_CLASSD_PD_REG                ( HI6421_REG_CODECENA_4 )
#define     HI6421_CLASSD_PD_BIT                ( 0 )

#define HI6421_REG_LINEINLPGA       ( 0xCD )
#define HI6421_REG_LINEINRPGA       ( 0xCE )

#define     HI6421_LINEINPGA_GAIN_BIT           ( 1 )
#define     HI6421_LINEINPGA_MUTE_BIT           ( 0 )

#define HI6421_REG_SUBPGA           ( 0xCF )
#define     HI6421_SUBPGA_BOOST_BIT             ( 5 )
#define     HI6421_SUBPGA_GAIN_BIT              ( 1 )
#define     HI6421_SUBPGA_MUTE_BIT              ( 0 )

#define HI6421_REG_MAINPGA          ( 0xD0 )
#define     HI6421_MAINPGA_SEL_BIT              ( 6 )
#define     HI6421_MAINPGA_BOOST_BIT            ( 5 )
#define     HI6421_MAINPGA_GAIN_BIT             ( 1 )
#define     HI6421_MAINPGA_MUTE_BIT             ( 0 )

#define HI6421_REG_SIDEPGA          ( 0xD1 )
#define     HI6421_SIDEPGA_GAIN_BIT             ( 1 )
#define     HI6421_SIDEPGA_MUTE_BIT             ( 0 )

#define HI6421_REG_MIXINL           ( 0xD2 )
#define HI6421_REG_MIXINR           ( 0xD3 )

/* D4 : ADCL CONTROL : ELECTRIC CURRENT & VOLTAGE */
/* D5 : ADCR CONTROL : ELECTRIC CURRENT & VOLTAGE */
/* D6 : DAC CONTROL : ELECTRIC CURRENT */

#define HI6421_REG_MONOMIX          ( 0xD7 )
#define HI6421_REG_MIXOUTL          ( 0xD8 )
#define HI6421_REG_MIXOUTR          ( 0xD9 )

#define     HI6421_MIXER_DACL_BIT               ( 6 )
#define     HI6421_MIXER_DACV_BIT               ( 5 )
#define     HI6421_MIXER_DACR_BIT               ( 4 )
#define     HI6421_MIXER_LINEINL_BIT            ( 3 )
#define     HI6421_MIXER_LINEINR_BIT            ( 2 )
#define     HI6421_MIXER_MAINMIC_BIT            ( 1 )
#define     HI6421_MIXER_SIDETONE_BIT           ( 0 )
#define     HI6421_MIXER_SUBMIC_BIT             ( 0 )

#define HI6421_REG_LINEOUTMIXL      ( 0xDA )
#define HI6421_REG_LINEOUTMIXR      ( 0XDC )

#define     HI6421_LINEOUTMIXER_MONO_BIT        ( 0 )
#define     HI6421_LINEOUTMIXER_DACL_BIT        ( 1 )
#define     HI6421_LINEOUTMIXER_DACR_BIT        ( 2 )

#define HI6421_REG_LINEOUTLPGA      ( 0xDB )
#define HI6421_REG_LINEOUTRPGA      ( 0xDD )

#define     HI6421_LINEOUTPGA_GAIN_BIT          ( 1 )
#define     HI6421_LINEOUTPGA_MUTE_BIT          ( 0 )

/* DE : HEADSET CONTROL : ELECTRIC CURRENT & VOLTAGE */
#define HI6421_REG_HEADSET_CONFIG   ( 0xDE )

#define HI6421_REG_HEADSETPGAL      ( 0xDF )
#define HI6421_REG_HEADSETPGAR      ( 0xE0 )

#define     HI6421_HEADSETPGA_GAIN_BIT          ( 1 )
#define     HI6421_HEADSETPGA_MUTE_BIT          ( 0 )

#define HI6421_REG_EARPIECEPGA      ( 0xE1 )
#define     HI6421_EARPIECEPGA_GAIN_BIT         ( 1 )
#define     HI6421_EARPIECEPGA_MUTE_BIT         ( 0 )

#define HI6421_REG_CLASSDPGA        ( 0xE2 )
#define     HI6421_CLASSDPGA_GAIN_BIT           ( 2 )
#define     HI6421_CLASSDPGA_MUTE_BIT           ( 1 )
#define     HI6421_CLASSDPGA_DELAY_MUTE_BIT     ( 0 )

/* E3 & E4 : CLASSD CONTROLS */
/* E5 & E6 : ZERO CROSS CONTROLS */
#define HI6421_REG_ZEROCROSS_1      ( 0xE5 )
#define HI6421_REG_ZEROCROSS_2      ( 0xE6 )

/* E7 - EB : PLL CONTROLS */
#define HI6421_REG_PLL_1            ( 0xE7 )
#define HI6421_REG_PLL_2            ( 0xE8 )
#define HI6421_REG_PLL_3            ( 0xE9 )
#define HI6421_REG_PLL_4            ( 0xEA )
#define HI6421_REG_PLL_5            ( 0xEB )

#define     HI6421_PLL_CONFIG_VALUE_1           ( 0xB0 )
#define     HI6421_PLL_CONFIG_VALUE_2           ( 0x7D )
#define     HI6421_PLL_CONFIG_VALUE_3           ( 0x1F )
#define     HI6421_PLL_CONFIG_VALUE_4           ( 0x87 )
#define     HI6421_PLL_CONFIG_VALUE_5           ( 0x00 )

#define HI6421_REG_MICBIAS          ( 0xEC )
#define     HI6421_MICBIAS_ECO_BIT              ( 4 )
#define     HI6421_MICBIAS_HS_MIC_EN_BIT        ( 3 )
#define     HI6421_MICBIAS_MAINMIC_EN_BIT       ( 2 )
#define     HI6421_MICBIAS_SW_GAIN_EN_BIT       ( 1 )
#define     HI6421_MICBIAS_SWM_EN_BIT           ( 0 )

/* ED : MICBIAS CONTROLS : VOLTAGE */
#define HI6421_REG_MICBIAS_CONFIG   ( 0xED )
/* EE - F0 : ELECTRIC CURRENT CONTROLS */
#define HI6421_REG_CURRENT_CONFIG_1 ( 0xEE )
#define HI6421_REG_CURRENT_CONFIG_2 ( 0xEF )
#define HI6421_REG_CURRENT_CONFIG_3 ( 0xF0 )
/* F1 : ANALOG LOOP */

#define HI6421_REG_HSD              ( 0xF2 )
#define     HI6421_HS_FAST_DISCHARGE_BIT        ( 3 )
#define     HI6421_HSD_ENABLE_BIT               ( 2 )
#define     HI6421_HSD_INVERT                   ( 1 )
#define     HI6421_HSD_PD_BIT                   ( 0 )

#define HI6421_CODEC_REG_FIRST          HI6421_REG_CIC_DMNS_CONFIG
#define HI6421_CODEC_REG_LAST           HI6421_REG_HSD

#define HI6421_CODEC_REG_RWLOOP1_FIRST  ( 0x93 )
#define HI6421_CODEC_REG_RWLOOP1_LAST   ( 0xC1 )

#define HI6421_CODEC_REG_RWLOOP2_FIRST  HI6421_REG_LINEINLPGA
#define HI6421_CODEC_REG_RWLOOP2_LAST   HI6421_CODEC_REG_LAST

#define HI6421_REG_PLL_RO_CTL           ( 0xF6 )
#define     HI6421_PLL_VALID_VALUE_MIN          ( 0x40 )

#endif
