/* 
 * drivers/omux/ipc/modem_dev.h
 *
 * Open MUX's SPI driver.
 *
 * Copyright (C) 2008 SPRD, Inc.
 * Author: Zhongli Zhang<zhongli.zhang@spreadtrum.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MODEM_DEV_H
#define _MODEM_DEV_H
#include <linux/sched.h>
#include <linux/gpio.h>
#define MODEM_DEBUG

#define MODEM_PACKET_HEADER_MAGIC_NUM 	0xA85A
#define MODEM_PACKET_TAIL_MAGIC_NUM	0x54A5
#define MODEM_PACKET_TYPE_FIRST		0x55
#define MODEM_PACKET_TYPE_AFTER		0xFF

#define MODEM_PACKET_SIZE		64

#define SPI_STATUS_IDLE			0x00
#define SPI_STATUS_INITIALIZED 		0x01
#define SPI_STATUS_SEND_DATA		0x02
#define SPI_STATUS_RECEIVE_DATA		0x04

enum {
	ERR_NONE = 0,
	ERR_HEAD,
	ERR_TAIL,
	ERR_DATA,
	ERR_TYPE,
	ERR_CRC,
	ERR_LEN,
	ERR_STATE,
	ERR_UNKNOWN
};

/*modem data packet header*/
struct modem_packet_header_t {
	u16 magic_num;		/*0xA85A */
	u8 packet_type;		/*0x55 or 0xFF */
	u8 reserved;
};

/*modem data packet information*/
struct modem_packet_inf_t {
	u16 real_data_len;	/*total valid data length */
	u16 raw_data_len;	/*total data length to send or receive include invalid filling bytes */
};

/*modem data packet tail information*/
struct modem_packet_tail_t {
	u16 reserved;
	u16 magic_num;
};

/*data must be aligned at 4 bytes*/
struct modem_packet_t {
	struct modem_packet_header_t header;
	struct modem_packet_inf_t inf;
	u32 reserved;
	u8 data[48];
	struct modem_packet_tail_t tail;
};

/*modem data packet, first packet or raw data packet*/
union modem_data_u {
	u8 data[64];
	struct modem_packet_t packet;
};
#if 0
struct modem_dev_t {
	union {
		u8 *buf_8;
		u16 *buf_16;
		u32 *buf_32;
	} rx_buf;

	union {
		u8 *buf_8;
		u16 *buf_16;
		u32 *buf_32;
	} tx_buf;

	u8 *data_buf;
	u16 data_len;

	unsigned char rbuf[64];
	unsigned char tbuf[64];

	volatile int state;

	struct ssp_dev *ssp_dev;

	wait_queue_head_t spi_read_wq;
	wait_queue_head_t spi_write_wq;

	int ap_tx;

	volatile int modem_rdy;
	volatile int modem_rts;
	volatile int modem_wakeup;
	volatile int modem_sleep;
	volatile int modem_life;

	int modem_rts_irq;
	int modem_rdy_irq;
	int modem_wakeup_irq;
	int modem_sleep_irq;
	int modem_life_irq;

	volatile unsigned long start_jif;
	volatile unsigned long end_jif;

#ifdef MODEM_DEBUG
	u32 total_packet;
	u32 err_packet;
#endif
};
#endif
typedef struct{
	unsigned int  num;
    char *name;
}sprd_modem_ctrl_gpio;


#if 0
typedef enum
{
      MODEM_OFF,
      MODEM_POWER_ON,
      MODEM_OK,
      MODEM_NBOOT,
      MODEM_RESET,
      MODEM_FACTORY_MODE,
      MODEM_AP_POWEROFF,
      MODEM_CTA_RESET,
      MODEM_STATE_MAX
}modem_state;
#else
typedef enum
{
      MODEM_OFF = 0,
      MODEM_POWER_ON = 1,
      MODEM_OK = 2,
      MODEM_NBOOT = 3,
      MODEM_RESET = 4,
      MODEM_FACTORY_MODE,
      MODEM_AP_POWEROFF = 0,
      MODEM_CTA_RESET = 4,
      MODEM_STATE_MAX
}modem_state;
#endif
#endif
