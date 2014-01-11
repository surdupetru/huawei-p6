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

#ifndef __K3_HDCP_H__
#define __K3_HDCP_H__

#define MDCC_SLAVE_ADDRESS  0x74
#define MAX_DRESS_ONE_PAGE  0x3FCul
#define MAX_BYTE_ON_DDC     16
#define KSV_ONES_NUMBER     20

#define DDC_BKSV_ADDR       0x00    /* 5 bytes, Read only (RO), receiver KSV */
#define DDC_Ri_ADDR         0x08    /* Ack from receiver RO */
#define DDC_AKSV_ADDR       0x10    /* 5 bytes WO, transmitter KSV */
#define DDC_AN_ADDR         0x18    /* 8 bytes WO, random value. */
#define DDC_V_ADDR          0x20    /* 20 bytes RO */
#define DDC_RSVD_ADDR       0x34    /* 12 byte RO */
#define DDC_BCAPS_ADDR      0x40   /* Capabilities Status byte RO */

#define DDC_BIT_HDMI_CAP    0x80
#define DDC_BIT_REPEATER    0x40
#define DDC_BIT_FIFO_READY  0x20

#define DDC_BSTATUS_ADDR         0x41
#define DDC_BSTATUS_1_ADDR       0x41
#define DDC_BSTATUS_2_ADDR       0x42
#define DDC_KSV_FIFO_ADDR        0x43

#define _DVI_  0
#define _HDMI_ 0x01

#define HDCP_AUTH_SUCCESS   "HDCP_STATE=hdcp_auth_success"
#define HDCP_AUTH_FAILED    "HDCP_STATE=hdcp_auth_failed"

/* Command Codes */
#define MASTER_CMD_ABORT        0x0f
#define MASTER_CMD_CLEAR_FIFO   0x09
#define MASTER_CMD_CLOCK        0x0a
#define MASTER_CMD_CUR_RD       0x00
#define MASTER_CMD_SEQ_RD       0x02
#define MASTER_CMD_ENH_RD       0x04
#define MASTER_CMD_SEQ_WR       0x06

#define MASTER_FIFO_WR_USE      0x01
#define MASTER_FIFO_RD_USE      0x02
#define MASTER_FIFO_EMPTY       0x04
#define MASTER_FIFO_FULL        0x08
#define MASTER_DDC_BUSY         0x10
#define MASTER_DDC_NOACK        0x20
#define MASTER_DDC_STUCK        0x40
#define MASTER_DDC_RSVD         0x80

#define NO_HDCP                         0x00
#define NO_DEVICE_WITH_HDCP_SLAVE_ADDR  0x01
#define BKSV_ERROR                      0x02
#define R0s_ARE_MISSMATCH               0x03
#define RIs_ARE_MISSMATCH               0x04
#define REAUTHENTATION_REQ              0x05
#define REQ_AUTHENTICATION              0x06
#define NO_ACK_FROM_HDCP_DEV            0x07
#define NO_RSEN                         0x08
#define AUTHENTICATED                   0x09
#define REPEATER_AUTH_REQ               0x0A
#define REQ_SHA_CALC                    0x0B
#define REQ_SHA_HW_CALC                 0x0C /* 9032 added */
#define FAILED_ViERROR                  0x0D /* 9032 moved */

#define _IIC_NOACK      2
#define _MDDC_CAPTURED  3
#define _MDDC_NOACK     4
#define _MDDC_FIFO_FULL 5
#define IIC_OK          0

#define MDDC_MANUAL_ADDR           0x3B0                /* Register Offsets */

#define h0init  0x67452301L
#define h1init  0xEFCDAB89L
#define h2init  0x98BADCFEL
#define h3init  0x10325476L
#define h4init  0xC3D2E1F0L

/* The SHS Constants */

#define K1  0x5A827999L                                 /* Rounds  0-19 */
#define K2  0x6ED9EBA1L                                 /* Rounds 20-39 */
#define K3  0x8F1BBCDCL                                 /* Rounds 40-59 */
#define K4  0xCA62C1D6L                                 /* Rounds 60-79 */

#define f1(x,y,z)   ( z ^ ( x & ( y ^ z ) ) )           /* Rounds  0-19 */
#define f2(x,y,z)   ( x ^ y ^ z )                       /* Rounds 20-39 */
#define f3(x,y,z)   ( ( x & y ) | ( z & ( x | y ) ) )   /* Rounds 40-59 */
#define f4(x,y,z)   ( x ^ y ^ z )                       /* Rounds 60-79 */

#define KSV_PULLED   0
#define KSV_FINISHED 1

// Buffer status
#define PADED      1
#define END_PLACED 2

#define BSTATMX_PULLED   0
#define BSTATMX_FINISHED 1
#define BSTATMX_SIZE     10

typedef struct _sha_ctx {
    u32 digest[5];
    u32 data[MAX_BYTE_ON_DDC];
    u16 bytecounter;
    u8 bstatusMXcounter;
} sha_ctx;

typedef struct _mddc_type {
    u32 slaveaddr;
    u32 segment;
    u32 offset;
    u32 nbyteslsb;
    u32 nbytemsb;
    u32 dummy;
    u32 cmd;
    u8 * pdata;
    u8 data[6];
} mddc_type;

typedef union _tmpd_type {
    u8 funcres[16];
    mddc_type mddctype;
} tmpd_type;

typedef struct _hdcp_work_info {
    struct delayed_work work;    /* work struct */
    u32 state;                  /* current work state */
} hdcp_work_info;

typedef struct _hdcp_info {
    bool enable;
    bool bhdcpstart;
    bool wait_anth_stop;
	bool is_checking_ksv;
    u32  auth_state;
    u8  an_tx[8];
    u32 jiffies_ori;

    struct mutex lock;
    struct workqueue_struct *hdcp_wq;
    struct mutex *videolock;

    void (*hdcp_notify)(char *result);
} hdcp_info;

void hdcp_init(void (*hdcp_notify)(char *result), struct mutex *videolock);
void hdcp_exit(void);

void hdcp_set_enable(bool enable);
bool hdcp_get_enable(void);

void hdcp_handle_work(u32 irq_state, int delay);

void hdcp_set_start(bool bstart);
bool hdcp_get_start(void);

void hdcp_auto_ksvready_check ( bool bOn );
u32 hdcp_get_auth_status (void);
void hdcp_wait_anth_stop(void);
#endif

