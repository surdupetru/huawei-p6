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

#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "k3_hdcp.h"
#include "k3_hdmi_hw.h"

#define LOG_TAG "hdmi-hdcp"
#include "k3_hdmi_log.h"

#define DELAY_REAUTHENTATION_REQ  200
hdcp_info hdcp_para = {0};

static void hdcp_sendcp_packet(bool on);
static u32 hdcp_mddc_read_rx(u32 NBytes, u32 addr, u8 * pData);

void hdcp_auto_ri_check ( bool bOn );
static u32 hdcp_is_in_hdmi_mode( void );
static bool hdcp_suspend_auto_richeck(bool state);
static void hdcp_anth_part3(void);

/******************************************************************************
* Function:       hdcp_set_auth_status
* Description:    set the status of the hdcp authen.
* Data Accessed:
* Data Updated:
* Input:          u8 status:
* Output:         NA
* Return:         NA
* Others:
*******************************************************************************/
static void hdcp_set_auth_status (u8 status)
{
    hdcp_para.auth_state = status;
    return;
}

u32 hdcp_get_auth_status (void)
{
    return hdcp_para.auth_state;
}

/***********************************************************************************
* Function:       hdcp_set_encryption
* Description:    enable or disable the encryption.
* Data Accessed:
* Data Updated:
* Input:          u8 onoff: true or false
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_set_encryption(u8 onoff)
{
    u32 regval = read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_CTRL);

    if (onoff) {
        write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_CTRL, regval | BIT_ENC_EN);
    } else {
        write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_CTRL, regval & ~BIT_ENC_EN);
    }

    return;
}

/***********************************************************************************
* Function:       hdcp_auto_ksvready_check
* Description:    enable or disable ksv list ready auto check.
* Data Accessed:
* Data Updated:
* Input:          bool bOn: true or false
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
void hdcp_auto_ksvready_check ( bool bOn )
{
    if ( bOn ) {
        write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR2_MASK,
                  ((read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INTR2_MASK )) | MASK_AUTO_KSV_READY ));
        write_reg(HDMI_CORE_SYS,  HDMI_CORE_SYS_RI_CMD,
                  ((read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_RI_CMD )) | BIT_RI_CMD_BCAP_EN));
    } else {
        write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR2_MASK,
                  ((read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INTR2_MASK )) & (~MASK_AUTO_KSV_READY)));
        write_reg(HDMI_CORE_SYS,  HDMI_CORE_SYS_RI_CMD,
                  ((read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_RI_CMD )) & (~BIT_RI_CMD_BCAP_EN)));
    }

    return;
}

/***********************************************************************************
* Function:       hdcp_convert_mddcstate_into_errcode
* Description:    convert statue to error code.
* Data Accessed:
* Data Updated:
* Input:          u8 * pErrSatus: mddc status
* Output:         u8 * pErrSatus: error code
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_convert_mddcstate_into_errcode( u32 * pErrSatus )
{
    BUG_ON(NULL == pErrSatus);

    if (!(*pErrSatus)) {
        logi("*pErrSatus is null\n");
        return;
    }

    if (*pErrSatus & BIT_MDDC_ST_I2C_LOW) {
        *pErrSatus = _MDDC_CAPTURED;
    } else if (*pErrSatus & BIT_MDDC_ST_NO_ACK) {
        *pErrSatus = _MDDC_NOACK;
    } else if (*pErrSatus & MASTER_FIFO_FULL) {
        *pErrSatus = _MDDC_FIFO_FULL;
    } else {
        *pErrSatus = IIC_OK;
    }

    return;
}

/***********************************************************************************
* Function:       hdcp_write_tx
* Description:    write the data .
* Data Accessed:
* Data Updated:
* Input:          u8 NBytes: write the lenth of the data
*                 u8 Add:    the offset address of write the data
*                 u8 * pData: the data to be write
* Output:         NA
* Return:         _IIC_NOACK or IIC_OK
* Others:
***********************************************************************************/
static u8 hdcp_write_tx(u8 NBytes, u16 Addr, u8 * pData)
{
    int i = 0;

    BUG_ON(NULL == pData);

    if ((NBytes > MAX_BYTE_ON_DDC) || ((NBytes + Addr) > MAX_DRESS_ONE_PAGE)) {
        loge(" CMD INVALID Reg or Size.\n");
        return _IIC_NOACK;
    }

    for (i = 0; i < NBytes; i++) {
        REG_FLD_MOD(HDMI_CORE_SYS, (Addr+4*i), pData[i],7, 0);
    }

    return IIC_OK;
}

/***********************************************************************************
* Function:       hdcp_read_tx
* Description:    read the data .
* Data Accessed:
* Data Updated:
* Input:          u8 NBytes: read the lenth of the data
*                 u8 Add:    the offset address of read the data
*                 u8 * pData: receive the data
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_read_tx(u8 NBytes, u8 Addr, u8 * pData)
{
    int i = 0;

    BUG_ON(NULL == pData);

    if ((NBytes > MAX_BYTE_ON_DDC) ||  ((NBytes + Addr) > MAX_DRESS_ONE_PAGE)) {
        loge(" CMD INVALID Reg or Size.\n");
        return ;
    }

    for (i = 0; i < NBytes; i++) {
        pData [i] = FLD_GET(read_reg(HDMI_CORE_SYS, (Addr+4*i)), 7, 0);
    }

    return ;
}

/***********************************************************************************
* Function:       hdcp_generate_an
* Description:    generate an, an is a 64 bit random number .
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_generate_an(void)
{
    u32 regval = read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_CTRL);

    /* Start AN Gen */
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_CTRL, regval & (~ BIT_AN_STOP));
    msleep(10);
    /* Stop AN Gen */
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_CTRL, regval | BIT_AN_STOP);
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_CTRL, regval | BIT_AN_STOP);
    return;
}

/***********************************************************************************
* Function:       hdcp_set_repeater_bit
* Description:    set the repeater or not.
* Data Accessed:
* Data Updated:
* Input:          u8 rxmode: true:repeater ,false:not repeater
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_set_repeater_bit( u8 rxmode )
{
    u32 regval = read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_CTRL);

    if (rxmode) {
        write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_CTRL, regval | BIT_RX_REPEATER );
    } else {
        write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_CTRL, regval & (~BIT_RX_REPEATER));
    }

    return;
}

/***********************************************************************************
* Function:       hdcp_release_cp_reset
* Description:    reset.
* Data Accessed:
* Data Updated:
* Input:          u8 rxmode: true:repeater ,false:not repeater
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_release_cp_reset( void )
{
    u32 regval = read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_CTRL);
    write_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_CTRL, regval | BIT_CP_RESET_N);

    return;
}

/***********************************************************************************
* Function:       hdcp_is_bksv_error
* Description:    bksv whether has error.
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:         error number
* Others:
***********************************************************************************/
static u32 hdcp_is_bksv_error(void)
{
    return read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_CTRL) & BIT_BKSV_ERROR;
}

/***********************************************************************************
* Function:       hdcp_richeck_interrupt_mask
* Description:    mask or not ri check interrupt.
* Data Accessed:
* Data Updated:
* Input:          bool benabled: true:mask, false:not mask
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_richeck_interrupt_mask(bool benabled)
{
    u32 regval = read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_INTR1_MASK);

    if (benabled) {
        write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_INTR1_MASK, regval | BIT_INT_Ri_CHECK);
    } else {
        write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_INTR1_MASK,  regval &(~ BIT_INT_Ri_CHECK));
    }

    return;
}

/***********************************************************************************
* Function:       hdcp_enable_hdmi_mode
* Description:    open or close hdmi mode.
* Data Accessed:
* Data Updated:
* Input:          bool benabled: true:open hdmi mode, false:close hdmi mode
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_enable_hdmi_mode(u8 benabled)
{
    u32 regval = read_reg(HDMI_CORE_AV,HDMI_CORE_AV_HDMI_CTRL);

    if (benabled) {
        write_reg( HDMI_CORE_AV,HDMI_CORE_AV_HDMI_CTRL, regval | BIT_TXHDMI_MODE);
    } else {
        write_reg( HDMI_CORE_AV,HDMI_CORE_AV_HDMI_CTRL, regval & (~BIT_TXHDMI_MODE));
    }

    return;
}

/***********************************************************************************
* Function:       hdcp_blank_video
* Description:    set vide blank
* Data Accessed:
* Data Updated:
* Input:          bool benabled : true:need blank, false: need not blank
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_blank_video(bool benabled)
{
    u32 regval = 0;
    u8 blankValue[3] = {0};

    regval = read_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_DATA_CTRL);
    if (benabled) {
        hdcp_write_tx(3, HDMI_CORE_SYS_VID_BLANK1, blankValue);
        write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_DATA_CTRL, regval | BIT_VID_BLANK);
    } else {
        write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_DATA_CTRL, regval & (~BIT_VID_BLANK));
    }
    return;
}

/***********************************************************************************
* Function:       hdcp_is_repeater
* Description:    device is a repeater or not.
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:         0x40: device is a repeater, other: device is not a repeater
* Others:
***********************************************************************************/
static bool hdcp_is_repeater(void)
{
#define MAX_CHECK_COUNT 3
    int i = 0;
    u8 regval = 0;
    for (; i < MAX_CHECK_COUNT; i++) {
        hdcp_mddc_read_rx(1, DDC_BCAPS_ADDR , &regval);
        if ((regval & DDC_BIT_REPEATER)) {
            break;
        } 
        mdelay(1);
    }
    logi("The hdcp device is %s\n", (regval & DDC_BIT_REPEATER) ? "Repeater" : "Sink");
    return (regval & DDC_BIT_REPEATER) ;
}

static u32 hdcp_is_ready(void)
{
    u8 regval = 0;

    hdcp_mddc_read_rx(1, DDC_BCAPS_ADDR , &regval);

    return regval & DDC_BIT_FIFO_READY;
}

/***********************************************************************************
* Function:       hdcp_are_r0s_match
* Description:    check if the r0s is equals.
* Data Accessed:
* Data Updated:
* Input:          u8 * pMatch: true or false
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static u8 hdcp_are_r0s_match(u8 * pMatch)
{
    u32 ret     = 0;
    u32 j       = 0;
    u8 r0rx[2]  = {0};
    u8 r0tx[2]  = {0};
    
    BUG_ON(NULL == pMatch);

    ret = hdcp_mddc_read_rx(2, DDC_Ri_ADDR, r0rx);
    if (ret) {
        loge("hdcp_mddc_read_rx failed, ret = %d\n",ret);
        return ret;
    }

    hdcp_read_tx(2, HDMI_CORE_SYS_HDCP_Ri_ADDR, r0tx);
    logi("rx_r0 = 0x%02x%02x, tx_r0 = 0x%02x%02x\n", r0rx[0],r0rx[1], r0tx[0],r0tx[1]);

    while ((j < 2) && (r0rx[j] == r0tx[j])) {
        j++;
    }

    if (j >= 2) {
        *pMatch = true;
    } else {
        *pMatch = false;
    }

    return ret;
}

/***********************************************************************************
* Function:       hdcp_make_copy_m0
* Description:
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_make_copy_m0(void)
{
    /* M0 readable over I2C */
    u32 regval = read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_SHA_CONTROL ) | BIT_M0_READ_EN;
    write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_SHA_CONTROL, regval);

    hdcp_read_tx(8, HDMI_CORE_SYS_HDCP_AN_ADDR, hdcp_para.an_tx);
    regval = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_SHA_CONTROL ) | ~BIT_M0_READ_EN;
    write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_SHA_CONTROL, regval);

    return;
}

/***********************************************************************************
* Function:       hdcp_check_device
* Description:    check the receiver device whether it is a hdcp device
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         u8 * pHDCP: true: hdcp device, false: not hdcp device
* Return:         0 or error code
* Others:
***********************************************************************************/
static bool hdcp_check_device(u8 * pKSV)
{
    u32 i     = 0;
    u32 j     = 0;
    u32 Ones  = 0;
    u8 Mask   = 0;

    BUG_ON(NULL == pKSV);

    for(i = 0; i < 5; i++) {
        Mask = 0x01;
        for(j = 0; j < 8; j++) {
            if(pKSV[i] & Mask) {
                Ones++;
            }
            Mask <<= 1;
        }
    }

    if (KSV_ONES_NUMBER == Ones) { 
        return true;
    } else {
        return false;
    }
}

static void hdcp_wait_tmds_stable()
{
    #define HDCP_CHECK_NUM 256
    u32 timeout = HDCP_CHECK_NUM;
    while ( !(read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_SYS_STAT) & 0x01) && (timeout--)) {
        msleep(1);
    }
//    logi("count:%d\n", HDCP_CHECK_NUM - timeout);
    return;
}

/***********************************************************************************
* Function:       hdcp_sendcp_packet
* Description:    set avi mute or not
* Data Accessed:
* Data Updated:
* Input:          bool on : true:mute, false: not mute
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_sendcp_packet(bool on)
{
    #define HDCP_CP_CHECK_NUM 64
    u32 timeoutCount = HDCP_CP_CHECK_NUM;
    u32 regval       = 0;
    bool is_hdmi = hdcp_is_in_hdmi_mode();
    logi("send cp to output video:%s\n", on ? "blank" : "normal");
    if(is_hdmi) {
        regval = read_reg( HDMI_CORE_AV,HDMI_CORE_AV_PB_CTRL2 );
        write_reg(HDMI_CORE_AV,HDMI_CORE_AV_PB_CTRL2, regval & (~BIT_CP_REPEAT));

        while (timeoutCount--) {
            if (!(read_reg( HDMI_CORE_AV,HDMI_CORE_AV_PB_CTRL2 )& BIT_CP_ENABLE)) {
                break;
            }
            msleep(1);
        }
    }
        //logi("check timeoutCount:%d\n", HDCP_CP_CHECK_NUM - timeoutCount);
    if (on) {
        write_reg(HDMI_CORE_AV, HDMI_CORE_AV_CP_BYTE1, BIT_CP_AVI_MUTE_SET);
    } else {
        write_reg(HDMI_CORE_AV, HDMI_CORE_AV_CP_BYTE1, BIT_CP_AVI_MUTE_CLEAR);
    }

    if(is_hdmi) {
        write_reg(HDMI_CORE_AV,HDMI_CORE_AV_PB_CTRL2, regval | BIT_CP_ENABLE | BIT_CP_REPEAT);
    }

    return;
}

/***********************************************************************************
* Function:       hdcp_mddc_init_read_from_fifo
* Description:    init to read fifo from mddc
* Data Accessed:
* Data Updated:
* Input:          u16 addr  : the offset address
*                 u16 NBytes: the lenth of the data
* Output:         NA
* Return:         0 or error code
* Others:
***********************************************************************************/
static u32 hdcp_mddc_init_read_from_fifo(u32 addr, u32 NBytes)
{
    tmpd_type tmpd;
    memset((void*)&tmpd, 0, sizeof(tmpd));

    tmpd.mddctype.slaveaddr = MDCC_SLAVE_ADDRESS;
    tmpd.mddctype.segment = 0;
    tmpd.mddctype.offset= addr;
    tmpd.mddctype.nbyteslsb = (u8)NBytes & 0xFF;
    tmpd.mddctype.nbytemsb = (u8)(NBytes & 0x300) >> 8;
    tmpd.mddctype.cmd = MASTER_CMD_SEQ_RD;

    REG_FLD_MOD(HDMI_CORE_SYS, HDMI_CORE_DDC_CMD, MASTER_CMD_CLEAR_FIFO, 3, 0);
    REG_FLD_MOD(HDMI_CORE_SYS, HDMI_CORE_DDC_CMD, MASTER_CMD_ABORT, 3, 0);

    /* Load Slave Address Register */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_ADDR, tmpd.mddctype.slaveaddr>> 1, 7, 1);

    /* Load Segment Address Register */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_SEGM, tmpd.mddctype.segment, 7, 0);

    /* Load offset Address Register */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_OFFSET, tmpd.mddctype.offset, 7, 0);

    /* Load Byte Count */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_COUNT1, tmpd.mddctype.nbyteslsb, 7, 0);
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_COUNT2, tmpd.mddctype.nbytemsb, 1, 0);

    /* Set DDC_CMD */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_CMD, tmpd.mddctype.cmd, 3, 0);

    /* HDMI_CORE_DDC_STATUS_BUS_LOW */
    if (1 == FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 6, 6)) {
        loge("I2C Bus Low?\n");
        return -EIO;
    }

    /* HDMI_CORE_DDC_STATUS_NO_ACK */
    if (1 == FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 5, 5)) {
        loge("I2C No Ack\n");
        return -EIO;
    }

    return 0;
}

/***********************************************************************************
* Function:       hdcp_read_mddc_fifo
* Description:    read the ddc fifo data
* Data Accessed:
* Data Updated:
* Input:          u8 ucLen : the lenth we want to get
*                 u8 * pucData: the data we get
* Output:         NA
* Return:         0 or error code
* Others:
***********************************************************************************/
static u32 hdcp_read_mddc_fifo(u32 ucLen, u8 *pucData)
{
    s32 retry   = 0;
    u32 ucIndex = 0;
    u32 ucRet   = 0;

    BUG_ON(NULL == pucData);

    while (retry < 20) {
        ucRet = read_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_FIFOCNT);
        if (0 == ucRet) {
            msleep(1);
            retry++;
            continue;
        }

        pucData[ucIndex] =  read_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_DATA);
        ucIndex ++;
        if (ucIndex >= ucLen) {
            break;
        }
    }

    if (ucIndex == ucLen) {
        return IIC_OK;
    } else {
        loge("Read MDDC_FIFO_ERR read %u/%u Byte.\n", ucIndex, ucLen);
        return _IIC_NOACK;
    }
}

/***********************************************************************************
* Function:       hdcp_get_mddc_fifo
* Description:    get the ddc fifo data
* Data Accessed:
* Data Updated:
* Input:          u8 NBytes : the lenth we want to get
*                 u8 * pData: the data we get
* Output:         NA
* Return:         0 or error code
* Others:
***********************************************************************************/
static u32 hdcp_get_mddc_fifo(u32 NBytes, u8 * pData)
{
    u32  timeoutCount = 0;
    int  errCode      = 0;

    BUG_ON(NULL == pData);

    timeoutCount = NBytes * 2;
    while ((read_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_FIFOCNT) < NBytes) && (timeoutCount--)) {
        msleep(1);
    }

    if (timeoutCount) {
        errCode = hdcp_read_mddc_fifo(NBytes, pData);
    } else {
        errCode =  _IIC_NOACK;
    }

    if (errCode != IIC_OK) {
        loge("timeout _IIC_NOACK\n");
    }

    return errCode;
}

/***********************************************************************************
* Function:       hdcp_read_mddc
* Description:    read the data from mddc.
* Data Accessed:
* Data Updated:
* Input:          mddc_type * pMddcCmd
* Output:         NA
* Return:         sucess:0, or error.
* Others:
***********************************************************************************/
static u32 hdcp_read_mddc(mddc_type * pMddcCmd)
{
    int i      = 0;
    int t      = 0;
    int ucRet  = 0;
    int size   = 0;
    int timer  = 0;
    u32 status = 0;
    u32 ret    = 0;
    
    BUG_ON(NULL == pMddcCmd);

    REG_FLD_MOD(HDMI_CORE_AV,HDMI_CORE_AV_DPD, 0x7, 2, 0);

    if (!hdcp_suspend_auto_richeck(true)) {
        loge("hdcp_suspend_auto_richeck failed, not use ddc, return.\n");
        return -1;
    }

    usleep_range(800, 1000);

    /* Clk SCL Devices */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_CMD, MASTER_CMD_CLOCK, 3, 0);

    /* Clear FIFO */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_CMD, MASTER_CMD_CLEAR_FIFO, 3, 0);

    while (FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 4, 4) != 0) {
        udelay(100);
        if (t++ > 10000) {
            loge("Failed to program DDC\n");
            return -ETIMEDOUT;
        }
    }

    /* Load Slave Address Register */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_ADDR, pMddcCmd->slaveaddr>> 1, 7, 1);

    /* Load Segment Address Register */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_SEGM, pMddcCmd->segment, 7, 0);

    /* Load offset Address Register */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_OFFSET, pMddcCmd->offset, 7, 0);

    /* Load Byte Count */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_COUNT1, pMddcCmd->nbyteslsb, 7, 0);
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_COUNT2, pMddcCmd->nbytemsb, 1, 0);

    /* Set DDC_CMD */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_CMD, pMddcCmd->cmd, 3, 0);

    /* HDMI_CORE_DDC_STATUS_BUS_LOW */
    if (1 == FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 6, 6)) {
        loge("I2C Bus Low?\n");
        return -EIO;
    }

    /* HDMI_CORE_DDC_STATUS_NO_ACK */
    if (1 == FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 5, 5)) {
        loge("I2C No Ack\n");
        return -EIO;
    }

    size = pMddcCmd->nbyteslsb + (pMddcCmd->nbytemsb << 8);

    while ((timer < 50) && (i != size) && (read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_SYS_STAT) & BIT_HPD_PIN)) {
        if (((1 == FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 4, 4))
                ||(0 == FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 2, 2)))
               && (i < size)) {

            ucRet = read_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_FIFOCNT);
            if (0 == ucRet ) {
                logd("MDDC_FIFO_CNT is 0.\n");
                msleep(1);
                timer = 0;
                continue;
            }

            if (0 == FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 2, 2)) {
                /* FIFO not empty */
                pMddcCmd->pdata[i] = FLD_GET(read_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_DATA), 7, 0);
                i++;
            }
        } else {
            //logi("wait for ddc process status\n");
            msleep(10);
            timer++;
        }
    }
    status = read_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_STATUS);
    hdcp_convert_mddcstate_into_errcode(&status);
    if (status) {
        /* can happen if Rx is clock stretching the SCL line. DDC bus unusable */
        if (_MDDC_NOACK == status) {
            /* mute audio and video */
            loge("_MDDC_NOACK, mute av\n");
            hdcp_sendcp_packet(true);
            hdcp_set_encryption(false);
            /* force re-authentication */
            hdcp_set_auth_status(REAUTHENTATION_REQ);
        } else {
            write_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_CMD,MASTER_CMD_ABORT);
            write_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_CMD,MASTER_CMD_CLOCK);
            write_reg(HDMI_CORE_SYS,MDDC_MANUAL_ADDR, 0);
        }
    }

    /* re-enable Auto Ri */
    hdcp_suspend_auto_richeck(false);

    return ret;
}

/***********************************************************************************
* Function:       hdcp_write_mddc
* Description:    write the data to mddc.
* Data Accessed:
* Data Updated:
* Input:          mddc_type * pMddcCmd
* Output:         NA
* Return:         sucess:0, or error.
* Others:
***********************************************************************************/
static u32 hdcp_write_mddc(mddc_type * pMddcCmd)
{
    u32  timeoutCount = 0;
    int  t            = 0;
    int  i            = 0;
    int  size         = 0;
    u32  status       = 0;
    u32  ret          = 0;

    BUG_ON(NULL == pMddcCmd);

    if (!hdcp_suspend_auto_richeck(true)) {
         loge("hdcp_suspend_auto_richeck failed, not use ddc, return.\n");
        return -1;
    }

    /* Clear FIFO */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_CMD, MASTER_CMD_CLEAR_FIFO, 3, 0);

    while (0 != FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 4, 4)) {
        udelay(1);
        if (t++ > 10000) {
            loge("Failed to program DDC\n");
            return -ETIMEDOUT;
        }
    }

    /* Load Slave Address Register */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_ADDR, pMddcCmd->slaveaddr>> 1, 7, 1);

    /* Load Segment Address Register */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_SEGM, pMddcCmd->segment, 7, 0);

    /* Load offset Address Register */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_OFFSET, pMddcCmd->offset, 7, 0);

    /* Load Byte Count */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_COUNT1, pMddcCmd->nbyteslsb, 7, 0);
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_COUNT2, pMddcCmd->nbytemsb, 1, 0);

    /* Set DDC_CMD */
    REG_FLD_MOD(HDMI_CORE_SYS,HDMI_CORE_DDC_CMD, pMddcCmd->cmd, 3, 0);

    status = read_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_STATUS);

    /* HDMI_CORE_DDC_STATUS_BUS_LOW */
    if (1 == FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 6, 6)) {
        loge("I2C Bus Low?\n");
        return -EIO;
    }

    /* HDMI_CORE_DDC_STATUS_NO_ACK */
    if (1 == FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 5, 5)) {
        loge("I2C No Ack\n");
        return -EIO;
    }

    size = pMddcCmd->nbyteslsb + (pMddcCmd->nbytemsb << 8);

    while ((0 == FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS), 3, 3))
           && (i < size)) {

        WR_REG_32(HDMI_CORE_SYS,HDMI_CORE_DDC_DATA, pMddcCmd->pdata[i++]);
        msleep(1);
    }

    timeoutCount = size;
    /* wait data has been written in ready */
    while (( read_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_STATUS) & BIT_MDDC_ST_IN_PROGR )
           && (timeoutCount--)) {
        /* wait data has been written in ready */
        logi(" wait data has been written in ready\n");
        msleep(1);
    }

    status = read_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_STATUS);
    hdcp_convert_mddcstate_into_errcode(&status);
    /* can happen if Rx is clock stretching the SCL line. DDC bus unusable */
    if (_MDDC_NOACK == status) {
        loge("status is no ack\n");
        /* mute audio and video */
        hdcp_sendcp_packet(true);
        hdcp_set_encryption(false);
        /* force re-authentication */
        hdcp_set_auth_status(REAUTHENTATION_REQ);
    } else {
        write_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_CMD,MASTER_CMD_ABORT);
        write_reg(HDMI_CORE_SYS,HDMI_CORE_DDC_CMD,MASTER_CMD_CLOCK);
        write_reg(HDMI_CORE_SYS,MDDC_MANUAL_ADDR, 0);
    }
    
    /* re-enable Auto Ri */
    hdcp_suspend_auto_richeck(false);

    return ret;
}

/***********************************************************************************
* Function:       hdcp_mddc_write_rx
* Description:    write data to ddc.
* Data Accessed:
* Data Updated:
* Input:          u8 NBytes : the lenth to be write
*                 u8 addr   : the offset adress
*                 u8 * pData: the data to write
* Output:         NA
* Return:
* Others:
***********************************************************************************/
static u32 hdcp_mddc_write_rx(u32 NBytes, u32 addr, u8 * pData)
{
    tmpd_type tmpd;

    BUG_ON(NULL == pData);

    memset((void*)&tmpd, 0, sizeof(tmpd));

    tmpd.mddctype.slaveaddr = MDCC_SLAVE_ADDRESS;
    tmpd.mddctype.segment = 0;
    tmpd.mddctype.offset = addr;
    tmpd.mddctype.nbyteslsb = NBytes & 0xFF;
    tmpd.mddctype.nbytemsb = (NBytes & 0x300) >> 8;
    tmpd.mddctype.cmd = MASTER_CMD_SEQ_WR;
    tmpd.mddctype.pdata = pData;

    return hdcp_write_mddc(&tmpd.mddctype);
}

/***********************************************************************************
* Function:       hdcp_mddc_read_rx
* Description:    read data from ddc.
* Data Accessed:
* Data Updated:
* Input:          u8 NBytes : the lenth to be read
*                 u8 addr   : the offset adress
*                 u8 * pData: the data to read
* Output:         NA
* Return:
* Others:
***********************************************************************************/
static u32 hdcp_mddc_read_rx(u32 NBytes, u32 addr, u8 * pData)
{
    tmpd_type tmpd;
    memset((void*)&tmpd, 0, sizeof(tmpd));

    BUG_ON(NULL == pData);

    tmpd.mddctype.slaveaddr = MDCC_SLAVE_ADDRESS;
    tmpd.mddctype.segment = 0;
    tmpd.mddctype.offset = addr;
    tmpd.mddctype.nbyteslsb = NBytes & 0xFF;
    tmpd.mddctype.nbytemsb = (NBytes & 0x300) >> 8;
    tmpd.mddctype.cmd = MASTER_CMD_SEQ_RD;
    tmpd.mddctype.pdata = pData;

    return hdcp_read_mddc(&tmpd.mddctype);
}

static void hdcp_sha_init( sha_ctx *pshsInfo)
{
    BUG_ON(NULL == pshsInfo);

    /* Set the h-vars to their initial values */
    pshsInfo->digest[0] = h0init;
    pshsInfo->digest[1] = h1init;
    pshsInfo->digest[2] = h2init;
    pshsInfo->digest[3] = h3init;
    pshsInfo->digest[4] = h4init;

    logd("hdcp_sha_init:0x%x,0x%x,0x%x,0x%x\n", pshsInfo->digest[0], pshsInfo->digest[1], pshsInfo->digest[2], pshsInfo->digest[3]);

    /* Initialise bit count */
    pshsInfo->bytecounter = 0;
    pshsInfo->bstatusMXcounter = 0;

    return;
}

static u32 hdcp_sha_place_pad_bit_counter(sha_ctx * pSha)
{
    u8 i       = 0;
    u8 bytePos = 0;
    u32 status = 0;

    BUG_ON(NULL == pSha);

    /* get posstion in SHA HI_U16 */
    bytePos = (u8)pSha->bytecounter & 0x0003;
    /* get SHA's HI_U16 address */
    i = (u8)((pSha->bytecounter & 0x0003F) >> 2);

    switch (bytePos) {
        case 0: {
            pSha->data[i]|=0x80000000;
            break;
        }

        case 1: {
            pSha->data[i]|=0x00800000;
            break;
        }

        case 2: {
            pSha->data[i]|=0x00008000;
            break;
        }

        case 3: {
            pSha->data[i]|=0x00000080;
            break;
        }

        default: {
            logi("bytePos is more than 3\n");
        }
            
    }

    status = PADED;
    if (((pSha->bytecounter & 0x3f)>>2) < 14) {
        pSha->data[15] = pSha->bytecounter*8;
        status =  END_PLACED;
    }

    return status;
}

static void hdcp_sha_transform(sha_ctx *sha)
{
    u32 A    = 0; /* Local vars */
    u32 B    = 0;
    u32 C    = 0;
    u32 D    = 0;
    u32 E    = 0;
    u32 TEMP = 0;
    u32 i    = 0;
    u8 s     = 0;

    BUG_ON(NULL == sha);

    A = sha->digest[ 0 ];
    B = sha->digest[ 1 ];
    C = sha->digest[ 2 ];
    D = sha->digest[ 3 ];
    E = sha->digest[ 4 ];

    for (i = 0; i < 80; i++) {
        s = i & 15;
        if (i >= 16) {
            sha->data[s]= sha->data[(s+13)&15]^sha->data[(s+8)&15]^sha->data[(s+2)&15]^sha->data[s];
            sha->data[s]= (sha->data[s]<<1)|(sha->data[s]>>31);
        }

        TEMP = ((A<<5)|(A>>27)) + E + sha->data[s];
        if (i <= 19) {
            TEMP+=K1;
            TEMP+=f1(B,C,D);
        } else if (i <= 39) {
            TEMP+=K2;
            TEMP+=f2(B,C,D);
        } else if (i <= 59) {
            TEMP+=K3;
            TEMP+=f3(B,C,D);
        } else {
            TEMP+=K4;
            TEMP+=f4(B,C,D);
        }

        E = D;
        D = C;
        C = (B<<30)|(B>>2);
        B = A;
        A = TEMP;
    }

    /* Build message digest */
    sha->digest[ 0 ] += A;
    sha->digest[ 1 ] += B;
    sha->digest[ 2 ] += C;
    sha->digest[ 3 ] += D;
    sha->digest[ 4 ] += E;

    logd("Digest value:0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", sha->digest[ 0 ], sha->digest[ 1 ], sha->digest[ 2 ], sha->digest[ 3 ], sha->digest[ 4 ]);
    return;
}

/***********************************************************************************
* Function:       hdcp_sha_get_aksv_list
* Description:    if the receiver is a repeater,and receive the ready,so we call
*                 this function to get aksv list
* Data Accessed:
* Data Updated:
* Input:          sha_ctx  * pSha
*                 u16 i:
* Output:         NA
* Return:         0 or error code
* Others:
***********************************************************************************/
static u32 hdcp_sha_get_aksv_list(sha_ctx  * pSha, u32 i)
{
    u32 aux       = 0;
    u32 bufStatus = 0;
    u8  *pdata    = NULL;

    BUG_ON(NULL == pSha);

    /* clear data buffer */
    for(aux = 0; aux < MAX_BYTE_ON_DDC; aux++) {
        pSha->data[aux] = 0;
    }

    /* Check how many bytes remain to read
     * total number = KSV list (up to  5 * 127 = 635) +
     * BStatus (2 bytes) + MX (8 bytes)
     * Get how many bytes KSV list remain to receive */
    i = i - pSha->bytecounter;
    aux = 0;

    while (aux < MAX_BYTE_ON_DDC) {
        if (0 == i) {
            /* no bytes remain in */
            bufStatus = KSV_FINISHED;
            break;
        } else {
            if (i > 4) {
                hdcp_get_mddc_fifo(4, (u8 *)&pSha->data[aux]);
                pSha->bytecounter += 4;
                i-= 4;
                if (0 == i) {
                    /* no bytes remain in */
                    bufStatus = KSV_FINISHED;
                    break;
                }
            } else {
                hdcp_get_mddc_fifo(i, (u8 *)&pSha->data[aux]);
                pSha->bytecounter += i;
                bufStatus = KSV_FINISHED;
                break;
            }
        }

        aux++;
    }

    pdata = (u8 *)&(pSha->data[0]);

    /* lc added for HDCP Repeater */
    {
        u8 index_byte = 0;
        u8 index = 0;
        u8 tmp = 0;
        u8 *Data = NULL;
        u8 NBytes = 0;
        
        for(index_byte = 0; index_byte < MAX_BYTE_ON_DDC; index_byte++) {

            Data = (u8 *)&pSha->data[index_byte];
            NBytes = 4;
            logd("orignal: \n");
            for(index = 0; index < NBytes; index++) {
                logd("0x%02x, \n", Data[index]);
            }
            logd("\n");

            for(index = 0; index < NBytes / 2; index++) {
                tmp = Data[index];
                Data[index] = Data[NBytes - 1 - index];
                Data[NBytes - 1 - index] = tmp;
            }

            logd("new: \n");
            for(index = 0; index < NBytes; index++) {
                logd("0x%02x, \n", Data[index]);
            }
            logd("\n");
        }
    }

    return bufStatus;
}

static void hdcp_sha_place_bit_counter(sha_ctx * pSha)
{
    u32 i = 0;

    BUG_ON(NULL == pSha);

    /* clear data buffer */
    for(i = 0; i < MAX_BYTE_ON_DDC; i++) {
        pSha->data[i]=0;
    }

    pSha->data[15] = pSha->bytecounter*8;

    return;
}

/***********************************************************************************
* Function:       hdcp_sha_compare_vi
* Description:    check vi and vi', if they equal return true ,else return false
* Data Accessed:
* Data Updated:
* Input:          sha_ctx * pSha
* Output:         NA
* Return:         if they equal return true ,else return false
* Others:
***********************************************************************************/
static bool hdcp_sha_compare_vi(sha_ctx * pSha)
{
    u32 vprime  = 0;
    u32 i       = 0;
    bool result = true;

    BUG_ON(NULL == pSha);

    hdcp_mddc_init_read_from_fifo( DDC_V_ADDR, 20 );

    for (i = 0; i < 5; i++) {
        hdcp_get_mddc_fifo(4, (u8 *)&vprime);

        logd("pSha->Digest[%d]:0x%x, vprime:0x%x\n", i, pSha->digest[i], vprime);
        if (pSha->digest[i] != vprime) {
            result = false;
            break;
        }
    }
    logd("hdcp_sha_compare_vi is result 0x%x *****\n", result);

    return result;
}

static u32 hdcp_sha_get_byte_bstatus_mx(u32 addr)
{
    u8 data[2] = {0};

    if (addr < 2) {
        /* Read B Status */
        hdcp_mddc_read_rx(2, DDC_BSTATUS_ADDR, data);
        logd("BSTATUS:0x%x,0x%x\n", data[0], data[1]);
        return data[addr];
    } else {
        /* MX */
        return hdcp_para.an_tx[addr-2];
    }
}

static u32 hdcp_sha_get_bstatus_mx(sha_ctx * pSha)
{
    u8 i       = 0;
    u8 bytePos = 0;

    BUG_ON(NULL == pSha);

    /* New data frame */
    if (0 == (pSha->bytecounter & 0x0003F)) {
        /* clear data buffer */
        for (i = 0; i < MAX_BYTE_ON_DDC; i++) {
            pSha->data[i] = 0;
        }
    }

    /* get posstion in SHA HI_U16 */
    bytePos = (u8)pSha->bytecounter&0x0003;
    /* get SHA's HI_U16 address */
    i = (u8)((pSha->bytecounter&0x0003F)>>2);

    switch (bytePos) {
        case 1: {
            pSha->data[i] |= (hdcp_sha_get_byte_bstatus_mx(pSha->bstatusMXcounter)<<16);
            pSha->bstatusMXcounter++;
            pSha->bytecounter++;
            if (pSha->bstatusMXcounter != BSTATMX_SIZE) {
                pSha->data[i] |= (hdcp_sha_get_byte_bstatus_mx(pSha->bstatusMXcounter)<<8);
                pSha->bstatusMXcounter++;
                pSha->bytecounter++;
            }

            if (pSha->bstatusMXcounter != BSTATMX_SIZE) {
                pSha->data[i] |= hdcp_sha_get_byte_bstatus_mx(pSha->bstatusMXcounter);
                pSha->bstatusMXcounter++;
                pSha->bytecounter++;
            }
            break;
        }

        case 2: {
            pSha->data[i] |= (hdcp_sha_get_byte_bstatus_mx(pSha->bstatusMXcounter)<<8);
            pSha->bstatusMXcounter++;
            pSha->bytecounter++;
            if(pSha->bstatusMXcounter) {
                pSha->data[i] |= hdcp_sha_get_byte_bstatus_mx(pSha->bstatusMXcounter);
                pSha->bstatusMXcounter++;
                pSha->bytecounter++;
            }
            break;
        }

        case 3: {
            pSha->data[i] |= hdcp_sha_get_byte_bstatus_mx(pSha->bstatusMXcounter);
            pSha->bstatusMXcounter++;
            pSha->bytecounter++;
            break;
        }

    }

    if (bytePos != 0) {
        i++;
    }

    while (i < MAX_BYTE_ON_DDC) {
        if (BSTATMX_SIZE == pSha->bstatusMXcounter) {
            /* no bytes remain in */
            return BSTATMX_FINISHED;
        }

        pSha->data[i] |= (hdcp_sha_get_byte_bstatus_mx(pSha->bstatusMXcounter) << 24);
        pSha->bytecounter++;
        pSha->bstatusMXcounter++;
        if (BSTATMX_SIZE == pSha->bstatusMXcounter) {
            /* no bytes remain in */
            return BSTATMX_FINISHED;
        }

        pSha->data[i]|= (hdcp_sha_get_byte_bstatus_mx(pSha->bstatusMXcounter)<<16);
        pSha->bytecounter++;
        pSha->bstatusMXcounter++;
        if (BSTATMX_SIZE == pSha->bstatusMXcounter) {
            /* no bytes remain in */
            return BSTATMX_FINISHED;
        }

        pSha->data[i] |= (hdcp_sha_get_byte_bstatus_mx( pSha->bstatusMXcounter)<<8);
        pSha->bytecounter++;
        pSha->bstatusMXcounter++;
        if (BSTATMX_SIZE == pSha->bstatusMXcounter) {
            /* no bytes remain in */
            return BSTATMX_FINISHED;
        }

        pSha->data[i] |= hdcp_sha_get_byte_bstatus_mx( pSha->bstatusMXcounter);
        pSha->bytecounter ++;
        pSha->bstatusMXcounter++;

        i++;
    }

    return BSTATMX_PULLED;
}

/***********************************************************************************
* Function:       hdcp_sha_handler
* Description:    process SHA arithmatic
* Data Accessed:
* Data Updated:
* Input:          int status: only when the status is REQ_SHA_CALC,we call this function
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_sha_handler(int status)
{
    u32  bufStatus     = 0;
    u32  KSVSize       = 0;
    u8  BStatusReg[2] = {0};
    
    sha_ctx sha;
    memset((void*)&sha, 0, sizeof(sha));

    IN_FUNCTION;

    if (REQ_SHA_CALC != status ) {
        OUT_FUNCTION;
        return;
    }

    hdcp_auto_ri_check(false);

    /* Read Bstatus */
    memset((void *)BStatusReg, 0x00, 2);
    hdcp_mddc_read_rx(2, DDC_BSTATUS_ADDR, BStatusReg);

    /* Check MAX_DEVS_EXCEEDED and MAX_CASCADE_EXCEEDED */
    logd("BStatusReg[0]:0x%x, BStatusReg[1]:0x%x\n", BStatusReg[0], BStatusReg[1]);
    if ((BStatusReg[0] & 0x80) ||(BStatusReg[1] & 0x08)) {
        loge("Reach Max device/deep, Reauthenicatin!!!\n");
        hdcp_sendcp_packet(true);
        hdcp_set_encryption(false);
        goto error;
    }

    /* Get FIFO size (number of KSV) /// // Must be done BEFORE reading KSV FIFO!!! - 8/24/07 */
    KSVSize =  (BStatusReg[0]&0x7F)*5;
    logd("KSVSize(DEVICE_COUNT*5):%d\n", KSVSize);

    if (KSVSize) {
        logi("read KSV List\n");
        hdcp_mddc_init_read_from_fifo(DDC_KSV_FIFO_ADDR, KSVSize );
    }

    hdcp_sha_init(&sha);
    logd("After hdcp_sha_init sha->Digest:0x%x, 0x%x, 0x%x, 0x%x\n", sha.digest[0], sha.digest[1], sha.digest[2], sha.digest[3]);

    while (KSV_PULLED == hdcp_sha_get_aksv_list(&sha, KSVSize)) {
        hdcp_sha_transform(&sha);
    }

    /* Got here if less than 512 bits that were left in FIFO were just read into buffer, */
    if (BSTATMX_PULLED == hdcp_sha_get_bstatus_mx(&sha)) {
        /* meaning that the buffer has room for more data. Now attempt to copy  into the */
        hdcp_sha_transform(&sha);

        /* same buffer the 2 bytes of BStatus and 8 of MX. if BSTATMX_PULLED, it means that
         * the last KSV's + BSttus + MX exceeded 512 bit. Transform W/O padding.
         * Now get the leftovers of KSV BStatus and MX. Will never reach 14 DWords. Must pad. */

        hdcp_sha_get_bstatus_mx(&sha);
        /* Pad down to DWord 14. Store length in DWord 15. */
        hdcp_sha_place_pad_bit_counter(&sha);
        /* Hash this very last block after padding it. */
        hdcp_sha_transform(&sha);
    } else {
        bufStatus = hdcp_sha_place_pad_bit_counter(&sha);
        hdcp_sha_transform(&sha);
        if (bufStatus != END_PLACED) {
            hdcp_sha_place_bit_counter(&sha);
            hdcp_sha_transform(&sha);
        }
    }

    if (hdcp_sha_compare_vi(&sha)) {
        hdcp_anth_part3();
        logi("SI_SHAHandler ok, enter hdcp protocol 3\n");
    } else {
        loge("SI_SHAHandler(hdcp protocol 2) fail\n");
        goto error;
    }

    OUT_FUNCTION;
    return;
error:

    hdcp_set_auth_status(REAUTHENTATION_REQ);
    if (hdcp_para.hdcp_notify) {
        hdcp_para.hdcp_notify(HDCP_AUTH_FAILED);
    }
    hdcp_handle_work(0, DELAY_REAUTHENTATION_REQ);

    OUT_FUNCTION;
    return;
}

/***********************************************************************************
* Function:       hdcp_read_aksvlist_timeout
* Description:    this is a 5s timer to receive aksv list ready,
*                 if it is more than 5s,we think the auth failed.
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:
* Return:        true or false
* Others:
***********************************************************************************/
static u8 hdcp_read_aksvlist_timeout(void)
{
    u32 tick_to_jiffies = 0;
    u32 jiffies_now = jiffies;

    tick_to_jiffies = msecs_to_jiffies(5000);
    //logi("time out tick jifies:%d now:%d ori:%d\n", tick_to_jiffies, jiffies_now, hdcp_para.jiffies_ori);
    if (jiffies_now > hdcp_para.jiffies_ori) {
        if (tick_to_jiffies < (jiffies_now - hdcp_para.jiffies_ori)) {
            /* >= 5, we will think timeout is occur!!¡¡*/
            loge("\n****Timeout(%d, %d, diff:0x%x ms) in get KSVlist, ReAuthentication***\n", (u32)jiffies_now,
                 (u32)hdcp_para.jiffies_ori, (u32)(jiffies_now - hdcp_para.jiffies_ori));
            return true;
        }
    } else {
        if (tick_to_jiffies < jiffies_now) {
            /* >= 5, we will think timeout is occur!!¡¡*/
            loge("\n****Timeout(%d, %d, diff:0x%x ms) in get KSVlist, ReAuthentication***\n", (u32)jiffies_now,
                 (u32)hdcp_para.jiffies_ori, (u32)(jiffies_now - hdcp_para.jiffies_ori));
            return true;
        }
    }

    return false;
}

/***********************************************************************************
* Function:       hdcp_disable
* Description:    disable the hdcp auth
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:
* Others:
***********************************************************************************/
static void hdcp_disable(void)
{
    logi("\n");
    hdcp_auto_ri_check( false );
    hdcp_auto_ksvready_check( false );
    hdcp_sendcp_packet(false);
    hdcp_set_encryption(false);
    hdcp_blank_video(false);
    hw_core_mute_audio(false);
    return;
}

static void hdcp_process_hpd_status(void)
{
    if (AUTHENTICATED == hdcp_para.auth_state) {
        hdcp_sendcp_packet(true);
        /* Must turn encryption off when AVMUTE */
        hdcp_set_encryption(false);

        hdcp_auto_ri_check ( false );
    }

    return;
}

/***********************************************************************************
* Function:       hdcp_auto_ri_check
* Description:    enable or disable ri auto check.
* Data Accessed:
* Data Updated:
* Input:          bool benabled: true:enable ri auto check, false:disable ri auto check
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
void hdcp_auto_ri_check ( bool bOn )
{
    if (bOn) {
        /* Turn on the following Auto Ri interrupts
         * OFF bit 7 Ri reading was not done within one frame
         * OFF bit 6 Ri did not change between frame 127 and 0
         * ON  bit 5 Ri did not match during 2nd frame test (0)
         * ON  bit 4 Ri did not match during 1st frame test (127) */
        write_reg(HDMI_CORE_SYS,  HDMI_CORE_SYS_INTR3,
                      ((read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR3_MASK )) | MASK_AUTO_RI_9134_SPECIFIC ));
        write_reg(HDMI_CORE_SYS,  HDMI_CORE_SYS_INTR3_MASK,
                  ((read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR3_MASK )) | MASK_AUTO_RI_9134_SPECIFIC ));
        write_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_RI_CMD,
                  ((read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_RI_CMD )) | BIT_RI_CMD_RI_EN));
    } else {
        write_reg(HDMI_CORE_SYS,  HDMI_CORE_SYS_INTR3_MASK,
                  ((read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR3_MASK )) & (~MASK_AUTO_RI_9134_SPECIFIC)));
        write_reg(HDMI_CORE_SYS,  HDMI_CORE_SYS_RI_CMD,
                  ((read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_RI_CMD )) & (~BIT_RI_CMD_RI_EN)));
    }

    return;
}

/***********************************************************************************
* Function:       hdcp_is_in_hdmi_mode
* Description:    whether the hdcp in hdmi mode.
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:         0:not in hdmi mode, 1:in hdmi mode
* Others:
***********************************************************************************/
static u32 hdcp_is_in_hdmi_mode( void )
{
    return BIT_TXHDMI_MODE & read_reg(HDMI_CORE_AV,HDMI_CORE_AV_HDMI_CTRL);
}

/***********************************************************************************
* Function:       hdcp_auto_rifailure_handle
* Description:    Checks if Link integraty is failed.
* Data Accessed:
* Data Updated:
* Input:          Ri Errors Interrupts
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
static void hdcp_auto_rifailure_handle( u8 bRiFIntr )
{
    if (bRiFIntr & MASK_AUTO_RI_9134_SPECIFIC) {
        hdcp_auto_ri_check( false );
        hdcp_auto_ksvready_check( false );
        hdcp_sendcp_packet(true);
        hdcp_set_encryption(false);
    }

    return;
}

/***********************************************************************************
* Function:       hdcp_suspend_auto_richeck
* Description:    Suspend Auto Ri in order to allow FW access MDDC bus.
* Data Accessed:
* Data Updated:
* Input:          bool state
* Output:         NA
* Return:         true if bus available,false if timed-out.
* Others:
***********************************************************************************/
static bool hdcp_suspend_auto_richeck(bool state)
{
    u32 timeout            = 10;
    static u8 oldRistate   = 0;
    static u8 oldRiCommand = 0;
    volatile u8 regval     = 0;

    if (true == state) {
        /* Save original Auto Ri state */
        oldRiCommand = read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_RI_CMD_ADDR ) & 0x01;
        /* disable Auto Ri */
        hdcp_auto_ri_check( false );

        /* wait for HW to release MDDC bus */
        while (--timeout) {
            regval = read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_RI_STAT );
            if (!(regval & 0x01)) {
                break;
            }
            msleep(1);
        }

        /* MDDC bus not relinquished. */
        if (!timeout && oldRiCommand) {
            /* enable Auto Ri Check */
            hdcp_auto_ri_check( true );
            return false;
        }

        /* Save MDDC bus status */
        oldRistate = read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_HDCP_RI_STAT ) & 0x01;
        return true;
    } else {
        if ((true == oldRistate) && ( true == oldRiCommand )) {
            /* re-enable Auto Ri */
            logi("hdcp_auto_ri_check\n");
            hdcp_auto_ri_check( true );
        }/* If Auto Ri was enabled before it was suspended */
    }
    return true;
}

/***********************************************************************************
* Function:       hdcp_anth_part1
* Description:    hdcp anthentication part 1
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:
* Others:
***********************************************************************************/
static int hdcp_anth_part1(void)
{
#define CHECK_IS_STOPED if (!hdcp_para.bhdcpstart) {err = -1; goto error;}
    u32 err           = 0;
    u8 an_ksv_data[8] = {0};
    u8 isR0Match      = false;
    bool isHdcpDev    = false;
    
    IN_FUNCTION;
    logi("hdcp part1 begin\n");

    hdcp_release_cp_reset();

    hdcp_set_repeater_bit(hdcp_is_repeater());
    
    CHECK_IS_STOPED;
    
    hdcp_generate_an();
    /* Read AN */
    hdcp_read_tx(8, HDMI_CORE_SYS_HDCP_AN_ADDR, an_ksv_data);
    /* Write AN to RX */
    err = hdcp_mddc_write_rx(8, DDC_AN_ADDR, an_ksv_data);
    if (err) {
        loge("write an failed\n");
        err = -1;
        goto error;
    }
    logi("write an sucess.\n");

    CHECK_IS_STOPED;

    /* Exchange KSVs values
     * read AKSV from TX */
    memset(an_ksv_data, 0, sizeof(an_ksv_data));
    hdcp_read_tx(5, HDMI_CORE_SYS_HDCP_AKSV_ADDR, an_ksv_data);
    isHdcpDev = hdcp_check_device(an_ksv_data);
    if (false == isHdcpDev) {
        loge(" the device is not surport hdcp,AKSV is error,isHdcpDev = %d\n",isHdcpDev);
        err = -1;
        goto error;
    }

    CHECK_IS_STOPED;

    /* write AKSV into RX */
    err = hdcp_mddc_write_rx(5, DDC_AKSV_ADDR, an_ksv_data);
    if (err) {
        loge("write aksv failed\n");
        err = -1;
        goto error;
    }
    logi("write aksv sucess.\n");

    memset(an_ksv_data, 0, sizeof(an_ksv_data));
    /* read BKSV from RX */
    err = hdcp_mddc_read_rx(5, DDC_BKSV_ADDR , an_ksv_data);
    if(err) {
        loge("read bksv failed\n");
        err = -1;
        goto error;
    }

    CHECK_IS_STOPED;

    isHdcpDev = hdcp_check_device(an_ksv_data);
    if (false == isHdcpDev) {
        loge(" the device is not surport hdcp,BKSV is error,isHdcpDev = %d\n",isHdcpDev);
        err = -1;
        goto error;
    }
    logi("read bksv sucess.\n");

	//recheck repeater
    hdcp_set_repeater_bit(hdcp_is_repeater());

    /* Write BKSV into TX */
    hdcp_write_tx(5, HDMI_CORE_SYS_HDCP_BKSV_ADDR, an_ksv_data);
    if (false == hdcp_para.bhdcpstart) {
        err = -1;
        goto error;
    }
    
    if (hdcp_is_bksv_error()) {
        loge(" BKSV_ERROR\n");
        err = -1;
        goto error;
    }

    /* Delay for R0 calculation */
    msleep(100);

    CHECK_IS_STOPED;

    err = hdcp_are_r0s_match(&isR0Match);
    if (err) {
        loge(" AreR0sMatch erroror:NO_ACK_FROM_HDCP_DEV\n");
        goto error;
    }
    logi("hdcp part1 check r0 is match: %d\n", isR0Match);

    if (!isR0Match) {
        loge("r0 is unmatch.\n");
        err = -1;
        goto error;
    } else {
        logi("r0 is match.\n");
    }

error:
    OUT_FUNCTION;
    return err;
}

/***********************************************************************************
* Function:       hdcp_anth_part2
* Description:    hdcp anthentication part 2,when the repeater is 1,we call this function
*                 this function is enable the ksvready interrupt.
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:
* Others:
***********************************************************************************/
static void hdcp_anth_part2(void)
{
#define KSV_READY_TIMEOUT 5000
#define KSV_READY_CHECK_INTERVAL 100
    int i = 0;
    u32 tick_to_jiffies = msecs_to_jiffies(5000);

    IN_FUNCTION;
    logi("hdcp part2 begin\n");

    hdcp_para.jiffies_ori = jiffies;   /* clock */
    hdcp_set_auth_status(REPEATER_AUTH_REQ);

    hdcp_make_copy_m0();
    
    /* enable ksv ready auto check interrupt */
    hdcp_sendcp_packet(false);
    hdcp_auto_ksvready_check ( true );

    hdcp_para.is_checking_ksv = true;
    for (i = 1; i <= (KSV_READY_TIMEOUT/KSV_READY_CHECK_INTERVAL) && hdcp_para.is_checking_ksv; i++) {
        msleep(KSV_READY_CHECK_INTERVAL);
	    logi("check ksv fifo is ready time:%d\n", i);
	    if (hdcp_is_ready()) {
	  	    hdcp_para.is_checking_ksv = false;
	  	    logi("ksv fifo is ready\n");
            break;
	    }
	    if ((u32)(jiffies - hdcp_para.jiffies_ori) > tick_to_jiffies) {
	  	    logi("ksv fifo check timeout \n");
            break;
	    }
    }
    if (hdcp_para.is_checking_ksv) {
        hdcp_para.is_checking_ksv = false;
        logi("ksv ready check fail, retry...\n");
        hdcp_auto_ri_check( false );
        hdcp_auto_ksvready_check( false );
        hdcp_sendcp_packet(true);
        hdcp_set_encryption(false);

        if (hdcp_para.wait_anth_stop) {
            hdcp_set_auth_status(NO_HDCP);
        } else {
            hdcp_set_auth_status(REAUTHENTATION_REQ);
            if (hdcp_para.hdcp_notify) {
                hdcp_para.hdcp_notify(HDCP_AUTH_FAILED);
            }
    
            hdcp_handle_work(0, DELAY_REAUTHENTATION_REQ);
        }
    } else {
        logi("ksv ready check ok, time %dms\n", i*KSV_READY_CHECK_INTERVAL);
    }
    OUT_FUNCTION;

    return;
}

/***********************************************************************************
* Function:       hdcp_anth_part3
* Description:    hdcp anthentication part 3,enable the ri auto check interrupt
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:
* Others:
***********************************************************************************/
static void hdcp_anth_part3(void)
{
    IN_FUNCTION;
    logi("hdcp part3 begin\n");

    hdcp_auto_ri_check ( true );

    hdcp_sendcp_packet(false);
    hdcp_blank_video(false);
    if (!hdcp_is_in_hdmi_mode()) {
        hw_core_mute_audio(true);
    } else {
        hw_core_mute_audio(false);
    }

    hdcp_set_auth_status(AUTHENTICATED);
    if (hdcp_para.hdcp_notify) {
        hdcp_para.hdcp_notify(HDCP_AUTH_SUCCESS);
    }

    OUT_FUNCTION;

    return;
}

/***********************************************************************************
* Function:       hdcp_authentication
* Description:    begin to hdcp anthentication,the auth has three part
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:
* Others:
***********************************************************************************/
static void hdcp_authentication( void )
{
    int ret = 0;

    IN_FUNCTION;

    ret =  hdcp_anth_part1();
    if (ret) {
        loge("auth part1 fail\n");
        goto error;
    }

    /* enable encryption */
    hdcp_set_encryption(true);

    if (hdcp_is_repeater()) {
        hdcp_anth_part2();
    } else {
        hdcp_anth_part3();
    }

    OUT_FUNCTION;
    return;

error:
    hdcp_set_auth_status(REAUTHENTATION_REQ);
    if (hdcp_para.hdcp_notify) {
        hdcp_para.hdcp_notify(HDCP_AUTH_FAILED);
    }
    hdcp_handle_work(0, DELAY_REAUTHENTATION_REQ);

    OUT_FUNCTION;

    return ;
}

static void hdcp_reathentication( void )
{
    u32 uRsen         = 0;
    int is_hdmi       = 0;

    IN_FUNCTION;

    uRsen = read_reg(HDMI_CORE_SYS,HDMI_CORE_SYS_SYS_STAT) & BIT_RSEN;
    if (uRsen != BIT_RSEN) {
        loge("no receiver connected\n");
        OUT_FUNCTION;
        return;
    }

    hdcp_disable();

    hdcp_wait_tmds_stable();

    is_hdmi = hdcp_is_in_hdmi_mode();

    logi("begin reathentication mode = %s\n", is_hdmi ? "HDMI" : "DVI");

    //set dvi mode
    hdcp_enable_hdmi_mode(is_hdmi);
    REG_FLD_MOD(HDMI_CORE_SYS, 0xCF*4, is_hdmi, 4, 4);

    hdcp_blank_video(true);
    hdcp_sendcp_packet(true);
    hdcp_set_encryption(false);        /* Must turn encryption off when AVMUTE */

    logd(" Reload KSV from OTP\n");
    write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_EPCM,0x00);
    msleep(10);
    write_reg( HDMI_CORE_SYS,HDMI_CORE_SYS_EPCM,0x20);

    hdcp_authentication();
    //move to part3

    OUT_FUNCTION;
    return;
}

/***********************************************************************************
* Function:           hdcp_work_queue
* Description:
* Data Accessed:
* Data Updated:
* Input:
* Output:
* Return:
* Others:
***********************************************************************************/
static void hdcp_work_queue(struct work_struct *ws)
{
    int state = 0;
    hdcp_work_info *work = NULL;
    bool wait_anth_stop = hdcp_para.wait_anth_stop;

    BUG_ON(NULL == ws);
    
    IN_FUNCTION;

    work = container_of(ws, hdcp_work_info, work.work);
    mutex_lock(&hdcp_para.lock);
    if (false == wait_anth_stop) {
        mutex_lock(hdcp_para.videolock);
    }
    state = work->state;
    logi("hdcp status = %d, irq state= %d\n",hdcp_para.auth_state, state);

    if (!hdcp_get_start()) {
        logi("edid not read complete\n");
        hdcp_set_auth_status(NO_HDCP); 
        goto done;
    }
    
    REG_FLD_MOD(HDMI_CORE_AV,HDMI_CORE_AV_DPD, 0x7, 2, 0);

    if (!hdcp_para.enable) {
        logi("***********force HDCP stop\n");
        hdcp_disable(); 
        goto done;
    }
    
    if (state & HDMI_RI_ERR) {
        logi("state is HDMI_RI_ERR\n");
        hdcp_auto_rifailure_handle( MASK_AUTO_RI_9134_SPECIFIC );
        hdcp_set_auth_status(REAUTHENTATION_REQ);
        msleep(DELAY_REAUTHENTATION_REQ);
    } else if (state & HDMI_BCAP) {
        logi("state is HDMI_BCAP\n");
        hdcp_para.is_checking_ksv = false;
        if (REPEATER_AUTH_REQ == hdcp_para.auth_state) {
            hdcp_set_auth_status(REQ_SHA_CALC);
        }
    } else if ( HDMI_CONNECT == state ) {
        if ((hdcp_para.auth_state != NO_HDCP) &&
            (hdcp_para.auth_state != AUTHENTICATED)) {
            goto done;
        }

        logi("state is HDMI_CONNECT\n");
        
        hdcp_process_hpd_status();
        hdcp_set_auth_status(REQ_AUTHENTICATION);
    } else if (state & HDMI_DISCONNECT) {
        logi("state is HDMI_DISCONNECT\n");
        hdcp_set_auth_status(NO_HDCP); 
        goto done;
    }

    if (false == hdcp_para.bhdcpstart) {
        hdcp_set_auth_status(NO_HDCP); 
        goto done;
    }
    
    if (REPEATER_AUTH_REQ == hdcp_para.auth_state) {
        logi("in REPEATER_AUTH_REQ, wait ksv fifo ready\n");
    } else if (REQ_SHA_CALC == hdcp_para.auth_state) {

        if (true == hdcp_read_aksvlist_timeout()) {
            loge("hdcp timeout to read KSVList\n");
            hdcp_auto_ri_check( false );
            hdcp_auto_ksvready_check( false );
            hdcp_sendcp_packet(true);
            hdcp_set_encryption(false);

            hdcp_set_auth_status(REAUTHENTATION_REQ);
            if (hdcp_para.hdcp_notify) {
                hdcp_para.hdcp_notify(HDCP_AUTH_FAILED);
            }

            hdcp_handle_work(0, DELAY_REAUTHENTATION_REQ);
            goto done;
        }
        
        if (false == hdcp_para.bhdcpstart) {
            hdcp_set_auth_status(NO_HDCP); 
            goto done;
        }
        
        if ( REQ_SHA_CALC == hdcp_para.auth_state ) {
            logi("hdcp part2 stat sha_calc\n");
            hdcp_sha_handler(hdcp_para.auth_state);
        }
    } else if (NO_HDCP == hdcp_para.auth_state ) {
        hdcp_disable();
    } else if(hdcp_para.auth_state != AUTHENTICATED) {
        if (hdcp_para.enable && hdcp_para.bhdcpstart ) {
            hdcp_reathentication();  
        } else {
            hdcp_disable();
        }
    }
    
done:
    kfree(work);
    if (false == wait_anth_stop) {
        mutex_unlock(hdcp_para.videolock);
    }
    mutex_unlock(&hdcp_para.lock);
    OUT_FUNCTION;
    
    return;
}

/***********************************************************************************
* Function:         hdcp_handle_work
* Description:      this function is process the interrupt and auth
* Data Accessed:
* Data Updated:
* Input:            int r: the interrupt
* Output:           NA
* Return:           NA
* Others:
***********************************************************************************/
void hdcp_handle_work(u32 irq_state, int delay)
{
    hdcp_work_info *work = NULL;

    BUG_ON( NULL == hdcp_para.hdcp_wq);

    IN_FUNCTION;

    if ((true == hdcp_para.wait_anth_stop) && (0 == irq_state)) {
        OUT_FUNCTION;
        return;
    }

    work = kmalloc(sizeof(*work), GFP_ATOMIC);
    if (work) {
        INIT_DELAYED_WORK(&work->work, hdcp_work_queue);
        work->state = irq_state;
        queue_delayed_work(hdcp_para.hdcp_wq, &work->work,msecs_to_jiffies(delay));
    } else {
        loge( "Cannot allocate memory to create work\n");
    }
    
    OUT_FUNCTION;
    
    return;
}

/***********************************************************************************
* Function:       hdcp_set_enable
* Description:    set hdcp enable
* Data Accessed:
* Data Updated:
* Input:          bool enable, true enable, false disable
* Output:         NA
* Return:
* Others:
***********************************************************************************/
void hdcp_set_enable(bool enable)
{
    IN_FUNCTION;
    logi(" enable:%d\n", enable);

    if (hdcp_para.enable == enable) {
        logi("the enable equals enable\n");
        OUT_FUNCTION;
        return;
    }

    hdcp_para.enable = enable;

    mutex_lock(&hdcp_para.lock);
    if (enable) {
        hdcp_set_auth_status(REAUTHENTATION_REQ);
    } else {
        hdcp_set_auth_status(NO_HDCP);
    }

    hdcp_handle_work(0,0);
    mutex_unlock(&hdcp_para.lock);
    
    OUT_FUNCTION;
    return;
}

/***********************************************************************************
* Function:       hdcp_get_enable
* Description:    this function is to get whether the hdcp is enable
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:         true or false, true means enable ,false means disable
* Others:
***********************************************************************************/
bool hdcp_get_enable(void)
{
    IN_FUNCTION;
    
    return hdcp_para.enable;
}

/***********************************************************************************
* Function:       hdcp_init
* Description:    init hdcp, init work queue
* Data Accessed:
* Data Updated:
* Input:         struct platform_device *pdev: device
* Output:        NA
* Return:        NA
* Others:
***********************************************************************************/
void hdcp_init(void (*hdcp_notify)(char *result), struct mutex *videolock)
{
    IN_FUNCTION;
    
    memset(&hdcp_para, 0, sizeof(hdcp_info));

    hdcp_para.hdcp_wq = create_singlethread_workqueue("hdcp_wq");

    hdcp_para.hdcp_notify = hdcp_notify;

    hdcp_para.enable = true;

    hdcp_para.videolock = videolock;

    mutex_init(&hdcp_para.lock);
    
    OUT_FUNCTION;
    
    return;
}

/***********************************************************************************
* Function:       hdcp_exit
* Description:    release resource
* Data Accessed:
* Data Updated:
* Input:          NA
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
void hdcp_exit(void)
{
    IN_FUNCTION;
    
    if (hdcp_para.hdcp_wq) {
        destroy_workqueue(hdcp_para.hdcp_wq);
    }

    hdcp_para.hdcp_notify = NULL;
    
    OUT_FUNCTION;
    
    return;
}

/***********************************************************************************
* Function:       hdcp_set_start
* Description:    set edid read compelete or not
* Data Accessed:
* Data Updated:
* Input:          bool bhasRead: true:has read; false: not read
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
void hdcp_set_start(bool bstart)
{
    IN_FUNCTION;
    logi("hdcp process state %d.\n", bstart);
    hdcp_para.bhdcpstart = bstart;
    if (false == bstart) {
        hdcp_disable();
    }
    
    OUT_FUNCTION;
    
    return;
}

/***********************************************************************************
* Function:       hdcp_get_start
* Description:    get edid read compelete or not
* Data Accessed:
* Data Updated:
* Input:          bool bhasRead: true:has read; false: not read
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
bool hdcp_get_start(void)
{
    IN_FUNCTION;
    OUT_FUNCTION;
    
    return hdcp_para.bhdcpstart;
}

/***********************************************************************************
* Function:       hdcp_wait_anth_stop
* Description:    wait anth_stop
* Data Accessed:
* Data Updated:
* Input:          bool bhasRead: true:has read; false: not read
* Output:         NA
* Return:         NA
* Others:
***********************************************************************************/
void hdcp_wait_anth_stop(void)
{
    int times = 0;
    hdcp_para.bhdcpstart = false;
    hdcp_para.wait_anth_stop = true;

    logi("hdcp stop state:%d\n", hdcp_para.auth_state);
    for (; times < 500; times++) {
        if ((NO_HDCP == hdcp_para.auth_state) || (AUTHENTICATED == hdcp_para.auth_state)) {
            break;
        }
        msleep(10);
    }
    hdcp_disable();
    hdcp_para.wait_anth_stop = false;
    if (50 == times) {
        loge("hdcp_wait_anth_stop failed\n");
    }
}

