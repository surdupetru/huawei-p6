/* File: mux_driver.c
 *
 * Portions derived from rfcomm.c, original header as follows:
 *
 * Copyright (C) 2000, 2001  Axis Communications AB
 *
 * Author: Mats Friden <mats.friden@axis.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Exceptionally, Axis Communications AB grants discretionary and
 * conditional permissions for additional use of the text contained
 * in the company's release of the AXIS OpenBT Stack under the
 * provisions set forth hereunder.
 *
 * Provided that, if you use the AXIS OpenBT Stack with other files,
 * that do not implement functionality as specified in the Bluetooth
 * System specification, to produce an executable, this does not by
 * itself cause the resulting executable to be covered by the GNU
 * General Public License. Your use of that executable is in no way
 * restricted on account of using the AXIS OpenBT Stack code with it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the provisions of the GNU
 * General Public License.
 *
 */

/*
 * Copyright (C) 2002-2004  Motorola
 * Copyright (C) 2006 Harald Welte <laforge@openezx.org>
 *  07/28/2002  Initial version
 *  11/18/2002  Second version
 *  04/21/2004  Add GPRS PROC
 *  09/28/2008  Porting to kernel 2.6 by Spreadtrum 
*/




#include <linux/module.h>
#include <linux/types.h>

#include <linux/kernel.h>

#include <linux/errno.h>
#include <linux/tty.h>
#include <linux/tty_ldisc.h>
#include <linux/tty_flip.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/major.h>
#include <linux/init.h>

#include <linux/proc_fs.h>

#include <linux/delay.h>

#include "sprd_ts0710.h"
#include "sprd_ts0710_mux.h"
#include "sprd_mux_buffer.h"
#include "sprd_transfer.h"
#include <linux/version.h>
//#include "sprd_version.h"

extern  void xmd_net_notify(int chno);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
typedef	struct mutex		MUTEX;
#define	INIT_MUTEX(m)		mutex_init(m)
#define	DOWN_TRYLOCK(m)		(!mutex_trylock(m))
#define	DOWN(m)			mutex_lock(m)
#define UP(m)			mutex_unlock(m)

#else
typedef	struct semaphore	MUTEX;
#define	INIT_MUTEX(m)		init_MUTEX(m)
#define	DOWN_TRYLOCK(m)		down_trylock(m)
#define	DOWN(m)			down(m)
#define UP(m)			up(m)
#endif

#define VT_CHANNEL (1)
#define STM_CHANNEL (5)

#define TS0710MUX_GPRS_SESSION_MAX 3
#define TS0710MUX_MAJOR 0
#define TS0710MUX_MINOR_START 0
#define NR_MUXS 17

#define TS0710MUX_TIME_OUT 250	/* 2500ms, for BP UART hardware flow control AP UART  */

#define TS0710MUX_IO_DLCI_FC_ON 0x54F2
#define TS0710MUX_IO_DLCI_FC_OFF 0x54F3
#define TS0710MUX_IO_FC_ON 0x54F4
#define TS0710MUX_IO_FC_OFF 0x54F5

#define TS0710MUX_MAX_BUF_SIZE 2048

#define TS0710MUX_SEND_BUF_OFFSET 10
#define TS0710MUX_SEND_BUF_SIZE (DEF_TS0710_MTU + TS0710MUX_SEND_BUF_OFFSET + 34)
#define TS0710MUX_RECV_BUF_SIZE TS0710MUX_SEND_BUF_SIZE

/*For BP UART problem Begin*/
#ifdef TS0710SEQ2
#define ACK_SPACE 0		/* 6 * 11(ACK frame size)  */
#else
#define ACK_SPACE 0		/* 6 * 7(ACK frame size)  */
#endif
/*For BP UART problem End*/

																																						    /*#define TS0710MUX_SERIAL_BUF_SIZE (DEF_TS0710_MTU + TS0710_MAX_HDR_SIZE) *//* For BP UART problem  */
#define TS0710MUX_SERIAL_BUF_SIZE (DEF_TS0710_MTU + TS0710_MAX_HDR_SIZE + ACK_SPACE)	/* For BP UART problem: ACK_SPACE  */

#define TS0710MUX_MAX_TOTAL_FRAME_SIZE (DEF_TS0710_MTU + TS0710_MAX_HDR_SIZE + FLAG_SIZE)
#define TS0710MUX_MAX_CHARS_IN_BUF 65535
#define TS0710MUX_THROTTLE_THRESHOLD DEF_TS0710_MTU

#define TEST_PATTERN_SIZE 250

#define CMDTAG 0x55
#define DATATAG 0xAA

#define ACK 0x4F		/*For BP UART problem */

/*For BP UART problem Begin*/
#ifdef TS0710SEQ2
#define FIRST_BP_SEQ_OFFSET 0	/*offset from start flag */
#define SECOND_BP_SEQ_OFFSET 0	/*offset from start flag */
#define FIRST_AP_SEQ_OFFSET 0	/*offset from start flag */
#define SECOND_AP_SEQ_OFFSET 0	/*offset from start flag */
#define SLIDE_BP_SEQ_OFFSET 0	/*offset from start flag */
#define SEQ_FIELD_SIZE 0
#else
#define SLIDE_BP_SEQ_OFFSET  1	/*offset from start flag */
#define SEQ_FIELD_SIZE    0	//1
#endif

#define ADDRESS_FIELD_OFFSET (1 + SEQ_FIELD_SIZE)	/*offset from start flag */
/*For BP UART problem End*/

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(v) (void)(v)
#endif

#define TS0710MUX_GPRS1_DLCI  3
#define TS0710MUX_GPRS2_DLCI  4
#define TS0710MUX_VT_DLCI         2

#define TS0710MUX_GPRS1_RECV_COUNT_IDX 0
#define TS0710MUX_GPRS1_SEND_COUNT_IDX 1
#define TS0710MUX_GPRS2_RECV_COUNT_IDX 2
#define TS0710MUX_GPRS2_SEND_COUNT_IDX 3
#define TS0710MUX_VT_RECV_COUNT_IDX 4
#define TS0710MUX_VT_SEND_COUNT_IDX 5

#define TS0710MUX_COUNT_MAX_IDX        5
#define TS0710MUX_COUNT_IDX_NUM (TS0710MUX_COUNT_MAX_IDX + 1)
#define RECV_RUNNING 0
#define NUM_X_BUF  100 //5

typedef struct {
	int recvBytes;
	int sentBytes;
} gprs_bytes;

typedef struct {
	__u8 cmdtty;
	__u8 datatty;
} dlci_tty;

typedef struct {
	volatile __u8 buf[TS0710MUX_SEND_BUF_SIZE];
	volatile __u8 *frame;
	unsigned long flags;
	volatile __u16 length;
	volatile __u8 filled;
	volatile __u8 dummy;	/* Allignment to 4*n bytes */
} mux_send_struct;

/* Bit number in flags of mux_send_struct */
#define BUF_BUSY 0

struct mux_recv_packet_tag {
	__u8 *data;
	__u32 length;
	struct mux_recv_packet_tag *next;
};
typedef struct mux_recv_packet_tag mux_recv_packet;

struct mux_recv_struct_tag {
	__u8 data[TS0710MUX_RECV_BUF_SIZE];
	__u32 length;
	__u32 total;
	mux_recv_packet *mux_packet;
	struct mux_recv_struct_tag *next;
	int no_tty;
	volatile __u8 post_unthrottle;
};
typedef struct mux_recv_struct_tag mux_recv_struct;

#ifdef min
#undef min
#define min(a,b)    ( (a)<(b) ? (a):(b) )
#endif
/*=========================================================*/
extern int vt_buffer_load(struct tty_struct *tty, const unsigned char *buf,
			  int count);

extern struct tty_driver *usb_for_mux_driver;
extern struct tty_struct *usb_for_mux_tty;
extern void (*serial_mux_dispatcher) (struct tty_struct * tty);
extern void (*serial_mux_sender) (void);
extern int write_data_to_modem_buf(const void *buf, int len);
extern void set_modem_state(modem_state state);

/*=========================================================*/


ssize_t file_proc_read(struct file *file, char *buf, size_t size,
		       loff_t * ppos);
ssize_t file_proc_write(struct file *file, const char *buf, size_t count,
			loff_t * ppos);

static int get_count(__u8 idx);
static int set_count(__u8 idx, int count);
static int add_count(__u8 idx, int count);

static int send_ua(ts0710_con * ts0710, __u8 dlci);
static int send_dm(ts0710_con * ts0710, __u8 dlci);
static int send_sabm(ts0710_con * ts0710, __u8 dlci);
static int send_disc(ts0710_con * ts0710, __u8 dlci);
static void queue_uih(mux_send_struct * send_info, __u16 len,
		      ts0710_con * ts0710, __u8 dlci);
static int send_pn_msg(ts0710_con * ts0710, __u8 prior, __u32 frame_size,
		       __u8 credit_flow, __u8 credits, __u8 dlci, __u8 cr);
static int send_nsc_msg(ts0710_con * ts0710, mcc_type cmd, __u8 cr);
static void set_uih_hdr(short_frame * uih_pkt, __u8 dlci, __u32 len, __u8 cr);

static __u32 crc_check(__u8 * data, __u32 length, __u8 check_sum);
static __u8 crc_calc(__u8 * data, __u32 length);
static void create_crctable(__u8 table[]);

static void mux_sched_send(void);
/*=========================================================*/

static volatile int mux_data_count[TS0710MUX_COUNT_IDX_NUM] =
    { 0, 0, 0, 0, 0, 0 };
static volatile int mux_data_count2[TS0710MUX_COUNT_IDX_NUM] =
    { 0, 0, 0, 0, 0, 0 };
static struct semaphore mux_data_count_mutex[TS0710MUX_COUNT_IDX_NUM];
static volatile __u8 post_recv_count_flag = 0;
static char mux_data[TS0710MUX_MAX_BUF_SIZE * 10];
struct mux_ringbuffer rbuf;

/*PROC file*/
struct proc_dir_entry *gprs_proc_file = NULL;


static __u8 tty2dlci[NR_MUXS] =
    { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 };

static dlci_tty dlci2tty[] = {
	{0, 0},			/* DLCI 0 */
	{0, 0},			/* DLCI 1 */
	{1, 1},			/* DLCI 2 */
	{2, 2},			/* DLCI 3 */
	{3, 3},			/* DLCI 4 */
	{4, 4},			/* DLCI 5 */
	{5, 5},			/* DLCI 6 */
	{6, 6},			/* DLCI 7 */
	{7, 7},			/* DLCI 8 */
	{8, 8},			/* DLCI 9 */
	{9, 9},			/* DLCI 10 */
	{10, 10},		/* DLCI 11 */
	{11, 11},		/* DLCI 12 */
	{12, 12},		/* DLCI 13 */
	{13, 13},		/* DLCI 14 */
	{14, 14},		/* DLCI 15 */
	{15, 15},		/* DLCI 16 */
	{16, 16},		/* DLCI 17 */
};

struct file_operations file_proc_operations = {
      read:file_proc_read,
      write:file_proc_write,
};

const static char version_string[]="SPRD_MUX_1.0.001";

static unsigned long mux_recv_flags = 0;
static mux_send_struct *mux_send_info[NR_MUXS];
static volatile __u8 mux_send_info_flags[NR_MUXS];
static volatile __u8 mux_send_info_idx = NR_MUXS;
static mux_recv_struct *mux_recv_info[NR_MUXS];
static volatile __u8 mux_recv_info_flags[NR_MUXS];
static mux_recv_struct *mux_recv_queue = NULL;
static struct tty_driver mux_driver;
void (*COMM_MUX_DISPATCHER) (struct tty_struct * tty);
void (*COMM_MUX_SENDER) (void);
static struct tty_struct **mux_table;
static volatile short int mux_tty[NR_MUXS];
static __u8 crctable[256];
static ts0710_con ts0710_connection;
static struct workqueue_struct *mux_work_queue;
static void receive_worker(struct work_struct *private_);
static void post_recv_worker(struct work_struct *private_);
static void send_worker(struct work_struct *private_);
static void net_send_worker(struct work_struct *private_);

struct s_data
{
      __u8   buf[DEF_TS0710_MTU];
      __u32 size;
};

struct net_send_buf
{
	struct s_data data[NUM_X_BUF];
	__u8 head;
	__u8 tail;
};
struct net_channel
{
    __u32 line;
    struct work_struct mux_net_send_work;
    struct net_send_buf mux_net_dbuf;
    spinlock_t lock;
};

static spinlock_t hsi_mem_lock;
static struct net_channel netdata_send_channel[3];

static DECLARE_WORK(mux_send_work, send_worker);
static DECLARE_WORK(mux_receive_work, receive_worker);
static DECLARE_WORK(mux_post_receive_work, post_recv_worker);
/*=========================================================*/
void init_q(int chno)
{
    int i;

    netdata_send_channel[chno].mux_net_dbuf.head  = 0;
    netdata_send_channel[chno].mux_net_dbuf.tail  = 0;

    for(i = 0;i<NUM_X_BUF;i++)
    {
     netdata_send_channel[chno].mux_net_dbuf.data[i].size = 0;
    }
}
struct s_data* read_q(int chno, char *buf, struct net_send_buf* q)
{
	struct s_data *data = NULL;
       int len;
	if (!q)
        {
		return NULL;
	  }
	spin_lock_bh(&netdata_send_channel[chno-2].lock);

	if (q->head == q->tail) {
		spin_unlock_bh(&netdata_send_channel[chno-2].lock);
		return NULL;
	}
	data = q->data + q->head;
      if(NULL == data)
        {
           printk("data is null\n");
           return 0;
        }
      memcpy(buf,&(data->buf[0]), data->size);
      len = data->size;
      data->size = 0;
	q->head = (q->head + 1) % NUM_X_BUF;

	spin_unlock_bh(&netdata_send_channel[chno-2].lock);

	return len;
}

/* Tail grows on writing in q. "q[tail]=data;tail++;" */
int write_q(struct net_send_buf* q, char *buf, int size, struct s_data **data)
{
	int temp = 0;

	if (!q) {
		return 0;
	}

	temp = (q->tail + 1) % NUM_X_BUF;

	if (temp != q->head) {
             memcpy(&(q->data[q->tail].buf[0]), buf, size);
		q->data[q->tail].size = size;
		if (data)
               {
			*data = q->data + q->tail;
		  }
		q->tail = temp;
	} else {
		return 0;
	}

      return q->tail > q->head ? q->tail - q->head:q->tail - q->head + NUM_X_BUF;
}

/*
static rpn_values rpn_val;
*/

static int valid_dlci(__u8 dlci)
{
	if ((dlci < TS0710_MAX_CHN) && (dlci > 0))
		return 1;
	else
		return 0;
}


#define PRINT_OUTPUT_PRINTK

#ifdef TS0710DEBUG

#ifdef PRINT_OUTPUT_PRINTK
#define TS0710_DEBUG(fmt, arg...) printk(KERN_INFO "MUX:%s "fmt, __FUNCTION__ , ## arg)
#else
#include "ezxlog.h"
static __u8 strDebug[256];
#define TS0710_DEBUG(fmt, arg...) ({ snprintf(strDebug, sizeof(strDebug), "MUX " __FUNCTION__ ": " fmt "\n" , ## arg); \
                                     /*printk("%s", strDebug)*/ezxlogk("MX", strDebug, strlen(strDebug)); })
#endif /* End #ifdef PRINT_OUTPUT_PRINTK */

#else
#define TS0710_DEBUG(fmt...)
#endif /* End #ifdef TS0710DEBUG */

#ifdef TS0710LOG
static unsigned char g_tbuf[TS0710MUX_MAX_BUF_SIZE];
#ifdef PRINT_OUTPUT_PRINTK
#define TS0710_LOG(fmt, arg...) printk(fmt, ## arg)
#define TS0710_PRINTK(fmt, arg...) printk(fmt, ## arg)
#else
#include "ezxlog.h"
static __u8 strLog[256];
#define TS0710_LOG(fmt, arg...) ({ snprintf(strLog, sizeof(strLog), fmt, ## arg); \
                                     /*printk("%s", strLog)*/ezxlogk("MX", strLog, strlen(strLog)); })
#define TS0710_PRINTK(fmt, arg...) ({ printk(fmt, ## arg); \
                                      TS0710_LOG(fmt, ## arg); })
#endif /* End #ifdef PRINT_OUTPUT_PRINTK */

#else
#define TS0710_LOG(fmt...)
#define TS0710_PRINTK(fmt, arg...) printk(fmt, ## arg)
#endif /* End #ifdef TS0710LOG */

#ifdef TS0710DEBUG
static void TS0710_DEBUGHEX(__u8 * buf, int len)
{
	static unsigned char tbuf[TS0710MUX_MAX_BUF_SIZE];

	int i;
	int c;

	if (len <= 0) {
		return;
	}

	c = 0;
	for (i = 0; (i < len) && (c < (TS0710MUX_MAX_BUF_SIZE - 3)); i++) {
		sprintf(&tbuf[c], "%02x ", buf[i]);
		c += 3;
	}
	tbuf[c] = 0;

#ifdef PRINT_OUTPUT_PRINTK
	TS0710_DEBUG("%s", tbuf);
#else
	/*printk("%s\n", tbuf) */ ezxlogk("MX", tbuf, c);
#endif
}
static void TS0710_DEBUGSTR(__u8 * buf, int len)
{
	static unsigned char tbuf[TS0710MUX_MAX_BUF_SIZE];

	if (len <= 0) {
		return;
	}

	if (len > (TS0710MUX_MAX_BUF_SIZE - 1)) {
		len = (TS0710MUX_MAX_BUF_SIZE - 1);
	}

	memcpy(tbuf, buf, len);
	tbuf[len] = 0;

#ifdef PRINT_OUTPUT_PRINTK
	/* 0x00 byte in the string pointed by tbuf may truncate the print result */
	TS0710_DEBUG("%s", tbuf);
#else
	/*printk("%s\n", tbuf) */ ezxlogk("MX", tbuf, len);
#endif
}
#else
#define TS0710_DEBUGHEX(buf, len)
#define TS0710_DEBUGSTR(buf, len)
#endif /* End #ifdef TS0710DEBUG */

#ifdef TS0710LOG
static void TS0710_LOGSTR_FRAME(__u8 send, __u8 * data, int len)
{
	short_frame *short_pkt;
	long_frame *long_pkt;
	__u8 *uih_data_start;
	__u32 uih_len;
	__u8 dlci;
	int pos;

	if (len <= 0) {
		return;
	}

	pos = 0;
	if (send) {
		pos += sprintf(&g_tbuf[pos], "<");
		short_pkt = (short_frame *) (data + 1);	/*For BP UART problem */
	} else {
		/*For BP UART problem */
		/*pos += sprintf(&g_tbuf[pos], ">"); */
		pos += sprintf(&g_tbuf[pos], ">%d ", *(data + SLIDE_BP_SEQ_OFFSET));	/*For BP UART problem */

#ifdef TS0710SEQ2
		pos += sprintf(&g_tbuf[pos], "%02x %02x %02x %02x ", *(data + FIRST_BP_SEQ_OFFSET), *(data + SECOND_BP_SEQ_OFFSET), *(data + FIRST_AP_SEQ_OFFSET), *(data + SECOND_AP_SEQ_OFFSET));	/*For BP UART problem */
#endif

		short_pkt = (short_frame *) (data + ADDRESS_FIELD_OFFSET);	/*For BP UART problem */
	}

	/*For BP UART problem */
	/*short_pkt = (short_frame *)(data + 1); */

	dlci = short_pkt->h.addr.server_chn << 1 | short_pkt->h.addr.d;
	switch (CLR_PF(short_pkt->h.control)) {
	case SABM:
		pos += sprintf(&g_tbuf[pos], "C SABM %d ::", dlci);
		break;
	case UA:
		pos += sprintf(&g_tbuf[pos], "C UA %d ::", dlci);
		break;
	case DM:
		pos += sprintf(&g_tbuf[pos], "C DM %d ::", dlci);
		break;
	case DISC:
		pos += sprintf(&g_tbuf[pos], "C DISC %d ::", dlci);
		break;

		/*For BP UART problem Begin */
	case ACK:
		pos += sprintf(&g_tbuf[pos], "C ACK %d ", short_pkt->data[0]);

#ifdef TS0710SEQ2
		pos += sprintf(&g_tbuf[pos], "%02x %02x %02x %02x ", short_pkt->data[1], short_pkt->data[2], short_pkt->data[3], short_pkt->data[4]);	/*For BP UART problem */
#endif

		pos += sprintf(&g_tbuf[pos], "::");
		break;
		/*For BP UART problem End */

	case UIH:
		if (!dlci) {
			pos += sprintf(&g_tbuf[pos], "C MCC %d ::", dlci);
		} else {

			if ((short_pkt->h.length.ea) == 0) {
				long_pkt = (long_frame *) short_pkt;
				uih_len = GET_LONG_LENGTH(long_pkt->h.length);
				uih_data_start = long_pkt->h.data;
			} else {
				uih_len = short_pkt->h.length.len;
				uih_data_start = short_pkt->data;
			}
			//switch (*uih_data_start) {//jim:  not support
			switch ( /**uih_data_start*/ 0) {
			case CMDTAG:
				pos +=
				    sprintf(&g_tbuf[pos], "I %d A %d ::", dlci,
					    uih_len);
				break;
			case DATATAG:
			default:
				pos +=
				    sprintf(&g_tbuf[pos], "I %d D %d ::", dlci,
					    uih_len);
				break;
			}

		}
		break;
	default:
		pos += sprintf(&g_tbuf[pos], "N!!! %d ::", dlci);
		break;
	}

	if (len > (sizeof(g_tbuf) - pos - 1)) {
		len = (sizeof(g_tbuf) - pos - 1);
	}

	memcpy(&g_tbuf[pos], data, len);
	pos += len;
	g_tbuf[pos] = 0;

#ifdef PRINT_OUTPUT_PRINTK
	/* 0x00 byte in the string pointed by g_tbuf may truncate the print result */
	TS0710_LOG("%s\n", g_tbuf);
#else
	/*printk("%s\n", g_tbuf) */ ezxlogk("MX", g_tbuf, pos);
#endif
}
#else
#define TS0710_LOGSTR_FRAME(send, data, len)
#endif

#ifdef TS0710SIG
#define my_for_each_task(p) \
        for ((p) = current; ((p) = (p)->next_task) != current; )

static void TS0710_SIG2APLOGD(void)
{
	struct task_struct *p;
	static __u8 sig = 0;

	if (sig) {
		return;
	}

	read_lock(&tasklist_lock);
	my_for_each_task(p) {
		if (strncmp(p->comm, "aplogd", 6) == 0) {
			sig = 1;
			if (send_sig(SIGUSR2, p, 1) == 0) {
				TS0710_PRINTK
				    ("MUX: success to send SIGUSR2 to aplogd!\n");
			} else {
				TS0710_PRINTK
				    ("MUX: failure to send SIGUSR2 to aplogd!\n");
			}
			break;
		}
	}
	read_unlock(&tasklist_lock);

	if (!sig) {
		TS0710_PRINTK("MUX: not found aplogd!\n");
	}
}
#else
#define TS0710_SIG2APLOGD()
#endif

static int basic_write(ts0710_con * ts0710, __u8 * buf, int len)
{
	int res;
	UNUSED_PARAM(ts0710);
	//printk("basic_write\n");

	buf[0] = TS0710_BASIC_FLAG;
	buf[len + 1] = TS0710_BASIC_FLAG;

	TS0710_LOGSTR_FRAME(1, buf, len + 2);

	res = write_data_to_modem_buf(buf, len+2);

	if (res) {
		printk("MUX basic_write: Write Error!\n");
		return -1;
	}

	return len + 2;
}

/* Functions for the crc-check and calculation */

#define CRC_VALID 0xcf

static __u32 crc_check(__u8 * data, __u32 length, __u8 check_sum)
{
	__u8 fcs = 0xff;
	length = length - 1;
	while (length--) {
		fcs = crctable[fcs ^ *data++];
	}
	fcs = crctable[fcs ^ check_sum];
	TS0710_DEBUG("fcs : %d\n", fcs);
	TS0710_DEBUG(" crc_check :len:%d check_sum:%d fcs : %d\n", length,
		     check_sum, fcs);
	if (fcs == (uint) 0xcf) {	/*CRC_VALID) */
		TS0710_DEBUG("crc_check: CRC check OK\n");
		return 0;
	} else {
		TS0710_PRINTK("MUX crc_check: CRC check failed\n");
		return 1;
	}
}

/* Calculates the checksum according to the ts0710 specification */

static __u8 crc_calc(__u8 * data, __u32 length)
{
	__u8 fcs = 0xff;

	while (length--) {
		fcs = crctable[fcs ^ *data++];
	}

	return 0xff - fcs;
}

/* Calulates a reversed CRC table for the FCS check */

static void create_crctable(__u8 table[])
{
	int i, j;

	__u8 data;
	__u8 code_word = (__u8) 0xe0;
	__u8 sr = (__u8) 0;

	for (j = 0; j < 256; j++) {
		data = (__u8) j;

		for (i = 0; i < 8; i++) {
			if ((data & 0x1) ^ (sr & 0x1)) {
				sr >>= 1;
				sr ^= code_word;
			} else {
				sr >>= 1;
			}

			data >>= 1;
			sr &= 0xff;
		}

		table[j] = sr;
		sr = 0;
	}
}

static void ts0710_reset_dlci(__u8 j)
{
	if (j >= TS0710_MAX_CHN)
		return;

    printk("ts0710_reset_dlci");
	ts0710_connection.dlci[j].state = DISCONNECTED;
	ts0710_connection.dlci[j].flow_control = 0;
	ts0710_connection.dlci[j].mtu = DEF_TS0710_MTU;
	ts0710_connection.dlci[j].initiated = 0;
	ts0710_connection.dlci[j].initiator = 0;
	init_waitqueue_head(&ts0710_connection.dlci[j].open_wait);
	init_waitqueue_head(&ts0710_connection.dlci[j].close_wait);
}

static void ts0710_reset_con(void)
{
	__u8 j;

    printk("ts0710_reset_con\n");
	ts0710_connection.initiator = 0;
	ts0710_connection.mtu = DEF_TS0710_MTU + TS0710_MAX_HDR_SIZE;
	ts0710_connection.be_testing = 0;
	ts0710_connection.test_errs = 0;
	init_waitqueue_head(&ts0710_connection.test_wait);

	for (j = 0; j < TS0710_MAX_CHN; j++) {
		ts0710_reset_dlci(j);
	}
}

static void ts0710_init(void)
{
      printk("ts0710_init\n");
      create_crctable(crctable);

	ts0710_reset_con();
}

static void ts0710_upon_disconnect(void)
{
	ts0710_con *ts0710 = &ts0710_connection;
	__u8 j;

    printk("ts0710_upon_disconnect\n");
	for (j = 0; j < TS0710_MAX_CHN; j++) {
		ts0710->dlci[j].state = DISCONNECTED;
		wake_up_interruptible(&ts0710->dlci[j].open_wait);
		wake_up_interruptible(&ts0710->dlci[j].close_wait);
	}
	ts0710->be_testing = 0;
	wake_up_interruptible(&ts0710->test_wait);
	ts0710_reset_con();
}

/* Sending packet functions */

/* Creates a UA packet and puts it at the beginning of the pkt pointer */

static int send_ua(ts0710_con * ts0710, __u8 dlci)
{
	__u8 buf[sizeof(short_frame) + FCS_SIZE + FLAG_SIZE];
	short_frame *ua;

	TS0710_DEBUG("send_ua: Creating UA packet to DLCI %d\n", dlci);

	ua = (short_frame *) (buf + 1);
	ua->h.addr.ea = 1;
	ua->h.addr.cr = ((~(ts0710->initiator)) & 0x1);
	ua->h.addr.d = (dlci) & 0x1;
	ua->h.addr.server_chn = (dlci) >> 0x1;
	ua->h.control = SET_PF(UA);
	ua->h.length.ea = 1;
	ua->h.length.len = 0;
	ua->data[0] = crc_calc((__u8 *) ua, SHORT_CRC_CHECK);

	return basic_write(ts0710, buf, sizeof(short_frame) + FCS_SIZE);
}

/* Creates a DM packet and puts it at the beginning of the pkt pointer */

static int send_dm(ts0710_con * ts0710, __u8 dlci)
{
	__u8 buf[sizeof(short_frame) + FCS_SIZE + FLAG_SIZE];
	short_frame *dm;

	TS0710_DEBUG("send_dm: Creating DM packet to DLCI %d\n", dlci);

	dm = (short_frame *) (buf + 1);
	dm->h.addr.ea = 1;
	dm->h.addr.cr = ((~(ts0710->initiator)) & 0x1);
	dm->h.addr.d = dlci & 0x1;
	dm->h.addr.server_chn = dlci >> 0x1;
	dm->h.control = SET_PF(DM);
	dm->h.length.ea = 1;
	dm->h.length.len = 0;
	dm->data[0] = crc_calc((__u8 *) dm, SHORT_CRC_CHECK);

	return basic_write(ts0710, buf, sizeof(short_frame) + FCS_SIZE);
}

static int send_sabm(ts0710_con * ts0710, __u8 dlci)
{
	__u8 buf[sizeof(short_frame) + FCS_SIZE + FLAG_SIZE];
	short_frame *sabm;

	TS0710_DEBUG("send_sabm: Creating SABM packet to DLCI %d\n", dlci);

	sabm = (short_frame *) (buf + 1);
	sabm->h.addr.ea = 1;
	sabm->h.addr.cr = ((ts0710->initiator) & 0x1);
	sabm->h.addr.d = dlci & 0x1;
	sabm->h.addr.server_chn = dlci >> 0x1;
	sabm->h.control = SET_PF(SABM);
	sabm->h.length.ea = 1;
	sabm->h.length.len = 0;
	sabm->data[0] = crc_calc((__u8 *) sabm, SHORT_CRC_CHECK);

	return basic_write(ts0710, buf, sizeof(short_frame) + FCS_SIZE);
}

static int send_disc(ts0710_con * ts0710, __u8 dlci)
{
	__u8 buf[sizeof(short_frame) + FCS_SIZE + FLAG_SIZE];
	short_frame *disc;

	TS0710_DEBUG("send_disc: Creating DISC packet to DLCI %d\n", dlci);

	disc = (short_frame *) (buf + 1);
	disc->h.addr.ea = 1;
	disc->h.addr.cr = ((ts0710->initiator) & 0x1);
	disc->h.addr.d = dlci & 0x1;
	disc->h.addr.server_chn = dlci >> 0x1;
	disc->h.control = SET_PF(DISC);
	disc->h.length.ea = 1;
	disc->h.length.len = 0;
	disc->data[0] = crc_calc((__u8 *) disc, SHORT_CRC_CHECK);

	return basic_write(ts0710, buf, sizeof(short_frame) + FCS_SIZE);
}

static void queue_uih(mux_send_struct * send_info, __u16 len,
		      ts0710_con * ts0710, __u8 dlci)
{
	__u32 size;

	TS0710_DEBUG
	    ("queue_uih: Creating UIH packet with %d bytes data to DLCI %d\n",
	     len, dlci);

	if (len > SHORT_PAYLOAD_SIZE) {
		long_frame *l_pkt;

		size = sizeof(long_frame) + len + FCS_SIZE;
		l_pkt = (long_frame *) (send_info->frame - sizeof(long_frame));
		set_uih_hdr((void *)l_pkt, dlci, len, ts0710->initiator);
		l_pkt->data[len] = crc_calc((__u8 *) l_pkt, LONG_CRC_CHECK);
		send_info->frame = ((__u8 *) l_pkt) - 1;
	} else {
		short_frame *s_pkt;

		size = sizeof(short_frame) + len + FCS_SIZE;
		s_pkt =(short_frame *) (send_info->frame - sizeof(short_frame));
		set_uih_hdr((void *)s_pkt, dlci, len, ts0710->initiator);
		s_pkt->data[len] = crc_calc((__u8 *) s_pkt, SHORT_CRC_CHECK);
		send_info->frame = ((__u8 *) s_pkt) - 1;
	}
	send_info->length = size;
}

/* Multiplexer command packets functions */

/* Turns on the ts0710 flow control */

static int ts0710_fcon_msg(ts0710_con * ts0710, __u8 cr)
{
	__u8 buf[30];
	mcc_short_frame *mcc_pkt;
	short_frame *uih_pkt;
	__u32 size;

	size = sizeof(short_frame) + sizeof(mcc_short_frame) + FCS_SIZE;
	uih_pkt = (short_frame *) (buf + 1);
	set_uih_hdr(uih_pkt, CTRL_CHAN, sizeof(mcc_short_frame),
		    ts0710->initiator);
	uih_pkt->data[sizeof(mcc_short_frame)] =
	    crc_calc((__u8 *) uih_pkt, SHORT_CRC_CHECK);
	mcc_pkt = (mcc_short_frame *) (uih_pkt->data);

	mcc_pkt->h.type.ea = EA;
	mcc_pkt->h.type.cr = cr;
	mcc_pkt->h.type.type = FCON;
	mcc_pkt->h.length.ea = EA;
	mcc_pkt->h.length.len = 0;

	return basic_write(ts0710, buf, size);
}

/* Turns off the ts0710 flow control */

static int ts0710_fcoff_msg(ts0710_con * ts0710, __u8 cr)
{
	__u8 buf[30];
	mcc_short_frame *mcc_pkt;
	short_frame *uih_pkt;
	__u32 size;

	size = (sizeof(short_frame) + sizeof(mcc_short_frame) + FCS_SIZE);
	uih_pkt = (short_frame *) (buf + 1);
	set_uih_hdr(uih_pkt, CTRL_CHAN, sizeof(mcc_short_frame),
		    ts0710->initiator);
	uih_pkt->data[sizeof(mcc_short_frame)] =
	    crc_calc((__u8 *) uih_pkt, SHORT_CRC_CHECK);
	mcc_pkt = (mcc_short_frame *) (uih_pkt->data);

	mcc_pkt->h.type.ea = 1;
	mcc_pkt->h.type.cr = cr;
	mcc_pkt->h.type.type = FCOFF;
	mcc_pkt->h.length.ea = 1;
	mcc_pkt->h.length.len = 0;

	return basic_write(ts0710, buf, size);
}

/* Sends an PN-messages and sets the not negotiable parameters to their
   default values in ts0710 */

static int send_pn_msg(ts0710_con * ts0710, __u8 prior, __u32 frame_size,
		       __u8 credit_flow, __u8 credits, __u8 dlci, __u8 cr)
{
	__u8 buf[30];
	pn_msg *pn_pkt;
	__u32 size;
	TS0710_DEBUG
	    ("send_pn_msg: DLCI 0x%02x, prior:0x%02x, frame_size:%d, credit_flow:%x, credits:%d, cr:%x\n",
	     dlci, prior, frame_size, credit_flow, credits, cr);

	size = sizeof(pn_msg);
	pn_pkt = (pn_msg *) (buf + 1);

	set_uih_hdr((void *)pn_pkt, CTRL_CHAN,
		    size - (sizeof(short_frame) + FCS_SIZE), ts0710->initiator);
	pn_pkt->fcs = crc_calc((__u8 *) pn_pkt, SHORT_CRC_CHECK);

	pn_pkt->mcc_s_head.type.ea = 1;
	pn_pkt->mcc_s_head.type.cr = cr;
	pn_pkt->mcc_s_head.type.type = PN;
	pn_pkt->mcc_s_head.length.ea = 1;
	pn_pkt->mcc_s_head.length.len = 8;

	pn_pkt->res1 = 0;
	pn_pkt->res2 = 0;
	pn_pkt->dlci = dlci;
	pn_pkt->frame_type = 0;
	pn_pkt->credit_flow = credit_flow;
	pn_pkt->prior = prior;
	pn_pkt->ack_timer = 0;
	SET_PN_MSG_FRAME_SIZE(pn_pkt, frame_size);
	pn_pkt->credits = credits;
	pn_pkt->max_nbrof_retrans = 0;

	return basic_write(ts0710, buf, size);
}

/* Send a Not supported command - command, which needs 3 bytes */

static int send_nsc_msg(ts0710_con * ts0710, mcc_type cmd, __u8 cr)
{
	__u8 buf[30];
	nsc_msg *nsc_pkt;
	__u32 size;

	size = sizeof(nsc_msg);
	nsc_pkt = (nsc_msg *) (buf + 1);

	set_uih_hdr((void *)nsc_pkt, CTRL_CHAN,
		    sizeof(nsc_msg) - sizeof(short_frame) - FCS_SIZE,
		    ts0710->initiator);

	nsc_pkt->fcs = crc_calc((__u8 *) nsc_pkt, SHORT_CRC_CHECK);

	nsc_pkt->mcc_s_head.type.ea = 1;
	nsc_pkt->mcc_s_head.type.cr = cr;
	nsc_pkt->mcc_s_head.type.type = NSC;
	nsc_pkt->mcc_s_head.length.ea = 1;
	nsc_pkt->mcc_s_head.length.len = 1;

	nsc_pkt->command_type.ea = 1;
	nsc_pkt->command_type.cr = cmd.cr;
	nsc_pkt->command_type.type = cmd.type;

	return basic_write(ts0710, buf, size);
}

static int ts0710_msc_msg(ts0710_con * ts0710, __u8 value, __u8 cr, __u8 dlci)
{
	__u8 buf[30];
	msc_msg *msc_pkt;
	__u32 size;

	size = sizeof(msc_msg);
	msc_pkt = (msc_msg *) (buf + 1);

	set_uih_hdr((void *)msc_pkt, CTRL_CHAN,
		    sizeof(msc_msg) - sizeof(short_frame) - FCS_SIZE,
		    ts0710->initiator);

	msc_pkt->fcs = crc_calc((__u8 *) msc_pkt, SHORT_CRC_CHECK);

	msc_pkt->mcc_s_head.type.ea = 1;
	msc_pkt->mcc_s_head.type.cr = cr;
	msc_pkt->mcc_s_head.type.type = MSC;
	msc_pkt->mcc_s_head.length.ea = 1;
	msc_pkt->mcc_s_head.length.len = 2;

	msc_pkt->dlci.ea = 1;
	msc_pkt->dlci.cr = 1;
	msc_pkt->dlci.d = dlci & 1;
	msc_pkt->dlci.server_chn = (dlci >> 1) & 0x1f;

	msc_pkt->v24_sigs = value;

	return basic_write(ts0710, buf, size);
}

static int ts0710_test_msg(ts0710_con * ts0710, __u8 * test_pattern, __u32 len,
			   __u8 cr, __u8 * f_buf /*Frame buf */ )
{
	__u32 size;

	if (len > SHORT_PAYLOAD_SIZE) {
		long_frame *uih_pkt;
		mcc_long_frame *mcc_pkt;

		size =
		    (sizeof(long_frame) + sizeof(mcc_long_frame) + len +
		     FCS_SIZE);
		uih_pkt = (long_frame *) (f_buf + 1);

		set_uih_hdr((short_frame *) uih_pkt, CTRL_CHAN, len +
			    sizeof(mcc_long_frame), ts0710->initiator);
		uih_pkt->data[GET_LONG_LENGTH(uih_pkt->h.length)] =
		    crc_calc((__u8 *) uih_pkt, LONG_CRC_CHECK);
		mcc_pkt = (mcc_long_frame *) uih_pkt->data;

		mcc_pkt->h.type.ea = EA;
		/* cr tells whether it is a commmand (1) or a response (0) */
		mcc_pkt->h.type.cr = cr;
		mcc_pkt->h.type.type = TEST;
		SET_LONG_LENGTH(mcc_pkt->h.length, len);
		memcpy(mcc_pkt->value, test_pattern, len);
	} else if (len > (SHORT_PAYLOAD_SIZE - sizeof(mcc_short_frame))) {
		long_frame *uih_pkt;
		mcc_short_frame *mcc_pkt;

		/* Create long uih packet and short mcc packet */
		size =
		    (sizeof(long_frame) + sizeof(mcc_short_frame) + len +
		     FCS_SIZE);
		uih_pkt = (long_frame *) (f_buf + 1);

		set_uih_hdr((short_frame *) uih_pkt, CTRL_CHAN,
			    len + sizeof(mcc_short_frame), ts0710->initiator);
		uih_pkt->data[GET_LONG_LENGTH(uih_pkt->h.length)] =
		    crc_calc((__u8 *) uih_pkt, LONG_CRC_CHECK);
		mcc_pkt = (mcc_short_frame *) uih_pkt->data;

		mcc_pkt->h.type.ea = EA;
		mcc_pkt->h.type.cr = cr;
		mcc_pkt->h.type.type = TEST;
		mcc_pkt->h.length.ea = EA;
		mcc_pkt->h.length.len = len;
		memcpy(mcc_pkt->value, test_pattern, len);
	} else {
		short_frame *uih_pkt;
		mcc_short_frame *mcc_pkt;

		size =
		    (sizeof(short_frame) + sizeof(mcc_short_frame) + len +
		     FCS_SIZE);
		uih_pkt = (short_frame *) (f_buf + 1);

		set_uih_hdr((void *)uih_pkt, CTRL_CHAN, len
			    + sizeof(mcc_short_frame), ts0710->initiator);
		uih_pkt->data[uih_pkt->h.length.len] =
		    crc_calc((__u8 *) uih_pkt, SHORT_CRC_CHECK);
		mcc_pkt = (mcc_short_frame *) uih_pkt->data;

		mcc_pkt->h.type.ea = EA;
		mcc_pkt->h.type.cr = cr;
		mcc_pkt->h.type.type = TEST;
		mcc_pkt->h.length.ea = EA;
		mcc_pkt->h.length.len = len;
		memcpy(mcc_pkt->value, test_pattern, len);

	}
	return basic_write(ts0710, f_buf, size);
}

static void set_uih_hdr(short_frame * uih_pkt, __u8 dlci, __u32 len, __u8 cr)
{
	uih_pkt->h.addr.ea = 1;
	uih_pkt->h.addr.cr = cr;
	uih_pkt->h.addr.d = dlci & 0x1;
	uih_pkt->h.addr.server_chn = dlci >> 1;
	uih_pkt->h.control = CLR_PF(UIH);

	if (len > SHORT_PAYLOAD_SIZE) {
		SET_LONG_LENGTH(((long_frame *) uih_pkt)->h.length, len);
	} else {
		uih_pkt->h.length.ea = 1;
		uih_pkt->h.length.len = len;
	}
}

/* Parses a multiplexer control channel packet */

void process_mcc(__u8 * data, __u32 len, ts0710_con * ts0710, int longpkt)
{
	__u8 *tbuf = NULL;
	mcc_short_frame *mcc_short_pkt;
	int j;

	if (longpkt) {
		mcc_short_pkt =
		    (mcc_short_frame *) (((long_frame *) data)->data);
	} else {
		mcc_short_pkt =
		    (mcc_short_frame *) (((short_frame *) data)->data);
	}

	switch (mcc_short_pkt->h.type.type) {
	case TEST:
		if (mcc_short_pkt->h.type.cr == MCC_RSP) {
			TS0710_DEBUG("Received test command response\n");

			if (ts0710->be_testing) {
				if ((mcc_short_pkt->h.length.ea) == 0) {
					mcc_long_frame *mcc_long_pkt;
					mcc_long_pkt =
					    (mcc_long_frame *) mcc_short_pkt;
					if (GET_LONG_LENGTH
					    (mcc_long_pkt->h.length) !=
					    TEST_PATTERN_SIZE) {
						ts0710->test_errs =
						    TEST_PATTERN_SIZE;
						TS0710_DEBUG
						    ("Err: received test pattern is %d bytes long, not expected %d\n",
						     GET_LONG_LENGTH
						     (mcc_long_pkt->h.length),
						     TEST_PATTERN_SIZE);
					} else {
						ts0710->test_errs = 0;
						for (j = 0;
						     j < TEST_PATTERN_SIZE;
						     j++) {
							if (mcc_long_pkt->
							    value[j] !=
							    (j & 0xFF)) {
								(ts0710->
								 test_errs)++;
							}
						}
					}

				} else {

#if TEST_PATTERN_SIZE < 128
					if (mcc_short_pkt->h.length.len !=
					    TEST_PATTERN_SIZE) {
#endif

						ts0710->test_errs =
						    TEST_PATTERN_SIZE;
						TS0710_DEBUG
						    ("Err: received test pattern is %d bytes long, not expected %d\n",
						     mcc_short_pkt->h.length.
						     len, TEST_PATTERN_SIZE);

#if TEST_PATTERN_SIZE < 128
					} else {
						ts0710->test_errs = 0;
						for (j = 0;
						     j < TEST_PATTERN_SIZE;
						     j++) {
							if (mcc_short_pkt->
							    value[j] !=
							    (j & 0xFF)) {
								(ts0710->
								 test_errs)++;
							}
						}
					}
#endif

				}

				ts0710->be_testing = 0;	/* Clear the flag */
				wake_up_interruptible(&ts0710->test_wait);
			} else {
				TS0710_DEBUG
				    ("Err: shouldn't or late to get test cmd response\n");
			}
		} else {
			tbuf = (__u8 *) kmalloc(len + 32, GFP_ATOMIC);
			if (!tbuf) {
				break;
			}

			if ((mcc_short_pkt->h.length.ea) == 0) {
				mcc_long_frame *mcc_long_pkt;
				mcc_long_pkt = (mcc_long_frame *) mcc_short_pkt;
				ts0710_test_msg(ts0710, mcc_long_pkt->value,
						GET_LONG_LENGTH(mcc_long_pkt->h.
								length),
						MCC_RSP, tbuf);
			} else {
				ts0710_test_msg(ts0710, mcc_short_pkt->value,
						mcc_short_pkt->h.length.len,
						MCC_RSP, tbuf);
			}

			kfree(tbuf);
		}
		break;

	case FCON:		/*Flow control on command */
		TS0710_PRINTK
		    ("MUX Received Flow control(all channels) on command\n");
		if (mcc_short_pkt->h.type.cr == MCC_CMD) {
			ts0710->dlci[0].state = CONNECTED;
			ts0710_fcon_msg(ts0710, MCC_RSP);
			mux_sched_send();
		}
		break;

	case FCOFF:		/*Flow control off command */
		TS0710_PRINTK
		    ("MUX Received Flow control(all channels) off command\n");
		if (mcc_short_pkt->h.type.cr == MCC_CMD) {
			for (j = 0; j < TS0710_MAX_CHN; j++) {
				ts0710->dlci[j].state = FLOW_STOPPED;
			}
			ts0710_fcoff_msg(ts0710, MCC_RSP);
		}
		break;

	case MSC:		/*Modem status command */
		{
			__u8 dlci;
			__u8 v24_sigs;

			dlci = (mcc_short_pkt->value[0]) >> 2;
			v24_sigs = mcc_short_pkt->value[1];

			if ((ts0710->dlci[dlci].state != CONNECTED)
			    && (ts0710->dlci[dlci].state != FLOW_STOPPED)) {
				send_dm(ts0710, dlci);
				break;
			}
			if (mcc_short_pkt->h.type.cr == MCC_CMD) {
				TS0710_DEBUG("Received Modem status command\n");
				if (v24_sigs & 2) {
					if (ts0710->dlci[dlci].state ==
					    CONNECTED) {
						TS0710_LOG
						    ("MUX Received Flow off on dlci %d\n",
						     dlci);
						ts0710->dlci[dlci].state =
						    FLOW_STOPPED;
					}
				} else {
					if (ts0710->dlci[dlci].state ==
					    FLOW_STOPPED) {
						ts0710->dlci[dlci].state =
						    CONNECTED;
						TS0710_LOG
						    ("MUX Received Flow on on dlci %d\n",
						     dlci);
						mux_sched_send();
					}
				}

				ts0710_msc_msg(ts0710, v24_sigs, MCC_RSP, dlci);

			} else {
				TS0710_DEBUG
				    ("Received Modem status response\n");

				if (v24_sigs & 2) {
					TS0710_DEBUG("Flow stop accepted\n");
				}
			}
			break;
		}

	case PN:		/*DLC parameter negotiation */
		{
			__u8 dlci;
			__u16 frame_size;
			pn_msg *pn_pkt;

			pn_pkt = (pn_msg *) data;
			dlci = pn_pkt->dlci;
			frame_size = GET_PN_MSG_FRAME_SIZE(pn_pkt);
			TS0710_DEBUG
			    ("Received DLC parameter negotiation, PN\n");
			if (pn_pkt->mcc_s_head.type.cr == MCC_CMD) {
				TS0710_DEBUG("received PN command with:\n");
				TS0710_DEBUG("Frame size:%d\n", frame_size);

				frame_size =
				    min(frame_size, ts0710->dlci[dlci].mtu);
				send_pn_msg(ts0710, pn_pkt->prior, frame_size,
					    0, 0, dlci, MCC_RSP);
				ts0710->dlci[dlci].mtu = frame_size;
				TS0710_DEBUG("process_mcc : mtu set to %d\n",
					     ts0710->dlci[dlci].mtu);
			} else {
				TS0710_DEBUG("received PN response with:\n");
				TS0710_DEBUG("Frame size:%d\n", frame_size);

				frame_size =
				    min(frame_size, ts0710->dlci[dlci].mtu);
				ts0710->dlci[dlci].mtu = frame_size;

				TS0710_DEBUG
				    ("process_mcc : mtu set on dlci:%d to %d\n",
				     dlci, ts0710->dlci[dlci].mtu);

				if (ts0710->dlci[dlci].state == NEGOTIATING) {
					ts0710->dlci[dlci].state = CONNECTING;
					wake_up_interruptible(&ts0710->
							      dlci[dlci].
							      open_wait);
				}
			}
			break;
		}

	case NSC:		/*Non supported command resonse */
		TS0710_LOG("MUX Received Non supported command response\n");
		break;

	default:		/*Non supported command received */
		TS0710_LOG("MUX Received a non supported command\n");
		send_nsc_msg(ts0710, mcc_short_pkt->h.type, MCC_RSP);
		break;
	}
}

static mux_recv_packet *get_mux_recv_packet(__u32 size)
{
	mux_recv_packet *recv_packet;

	TS0710_DEBUG("Enter into get_mux_recv_packet");

	recv_packet =
	    (mux_recv_packet *) kmalloc(sizeof(mux_recv_packet), GFP_ATOMIC);
	if (!recv_packet) {
		return 0;
	}

	recv_packet->data = (__u8 *) kmalloc(size, GFP_ATOMIC);
	if (!(recv_packet->data)) {
		kfree(recv_packet);
		return 0;
	}
	recv_packet->length = 0;
	recv_packet->next = 0;
	return recv_packet;
}

static void free_mux_recv_packet(mux_recv_packet * recv_packet)
{
	TS0710_DEBUG("Enter into free_mux_recv_packet");

	if (!recv_packet) {
		return;
	}

	if (recv_packet->data) {
		kfree(recv_packet->data);
	}
	kfree(recv_packet);
}

static void free_mux_recv_struct(mux_recv_struct * recv_info)
{
	mux_recv_packet *recv_packet1, *recv_packet2;

	if (!recv_info) {
		return;
	}

	recv_packet1 = recv_info->mux_packet;
	while (recv_packet1) {
		recv_packet2 = recv_packet1->next;
		free_mux_recv_packet(recv_packet1);
		recv_packet1 = recv_packet2;
	}

	kfree(recv_info);
}

static inline void add_post_recv_queue(mux_recv_struct ** head,
				       mux_recv_struct * new_item)
{
	new_item->next = *head;
	*head = new_item;
}

static void ts0710_flow_on(__u8 dlci, ts0710_con * ts0710)
{
	int i;
	__u8 cmdtty;
	__u8 datatty;
	struct tty_struct *tty;
	mux_recv_struct *recv_info;

	if ((ts0710->dlci[0].state != CONNECTED)
	    && (ts0710->dlci[0].state != FLOW_STOPPED)) {
		return;
	} else if ((ts0710->dlci[dlci].state != CONNECTED)
		   && (ts0710->dlci[dlci].state != FLOW_STOPPED)) {
		return;
	}

	if (!(ts0710->dlci[dlci].flow_control)) {
		return;
	}

	cmdtty = dlci2tty[dlci].cmdtty;
	datatty = dlci2tty[dlci].datatty;

	if (cmdtty != datatty) {
		/* Check AT cmd tty */
		tty = mux_table[cmdtty];
		if (mux_tty[cmdtty] && tty) {
			if (test_bit(TTY_THROTTLED, &tty->flags)) {
				return;
			}
		}
		recv_info = mux_recv_info[cmdtty];
		if (mux_recv_info_flags[cmdtty] && recv_info) {
			if (recv_info->total) {
				return;
			}
		}

		/* Check data tty */
		tty = mux_table[datatty];
		if (mux_tty[datatty] && tty) {
			if (test_bit(TTY_THROTTLED, &tty->flags)) {
				return;
			}
		}
		recv_info = mux_recv_info[datatty];
		if (mux_recv_info_flags[datatty] && recv_info) {
			if (recv_info->total) {
				return;
			}
		}
	}

	for (i = 0; i < 3; i++) {
		if (ts0710_msc_msg(ts0710, EA | RTC | RTR | DV, MCC_CMD, dlci) <
		    0) {
			continue;
		} else {
			TS0710_LOG("MUX send Flow on on dlci %d\n", dlci);
			ts0710->dlci[dlci].flow_control = 0;
			break;
		}
	}
}

char netdatabuffer[2500];
extern int data_size;

void  getnetdata(char * data,int len)
{
	memcpy(netdatabuffer,data,len);
}
static void ts0710_flow_off(struct tty_struct *tty, __u8 dlci,
			    ts0710_con * ts0710)
{
	int i;

	if (test_and_set_bit(TTY_THROTTLED, &tty->flags)) {
		return;
	}

	if ((ts0710->dlci[0].state != CONNECTED)
	    && (ts0710->dlci[0].state != FLOW_STOPPED)) {
		return;
	} else if ((ts0710->dlci[dlci].state != CONNECTED)
		   && (ts0710->dlci[dlci].state != FLOW_STOPPED)) {
		return;
	}

	if (ts0710->dlci[dlci].flow_control) {
		return;
	}

	for (i = 0; i < 3; i++) {
		if (ts0710_msc_msg
		    (ts0710, EA | FC | RTC | RTR | DV, MCC_CMD, dlci) < 0) {
			continue;
		} else {
			TS0710_LOG("MUX send Flow off on dlci %d\n", dlci);
			ts0710->dlci[dlci].flow_control = 1;
			break;
		}
	}
}

int ts0710_recv_data(ts0710_con * ts0710, char *data, int len)
{
	short_frame *short_pkt;
	long_frame *long_pkt;
	__u8 *uih_data_start;
	__u32 uih_len;
	__u8 dlci;
	__u8 be_connecting;
#ifdef TS0710DEBUG
	unsigned long t;
#endif

	short_pkt = (short_frame *) data;

	dlci = short_pkt->h.addr.server_chn << 1 | short_pkt->h.addr.d;
	switch (CLR_PF(short_pkt->h.control)) {
	case SABM:
		printk("SABM-packet received\n");

		if (!dlci) {
			TS0710_DEBUG("server channel == 0\n");
			ts0710->dlci[0].state = CONNECTED;

			TS0710_DEBUG("sending back UA - control channel\n");
			send_ua(ts0710, dlci);
			wake_up_interruptible(&ts0710->dlci[0].open_wait);

		} else if (valid_dlci(dlci)) {

			TS0710_DEBUG("Incomming connect on channel %d\n", dlci);

			TS0710_DEBUG("sending UA, dlci %d\n", dlci);
			send_ua(ts0710, dlci);

			ts0710->dlci[dlci].state = CONNECTED;
			wake_up_interruptible(&ts0710->dlci[dlci].open_wait);

		} else {
			TS0710_DEBUG("invalid dlci %d, sending DM\n", dlci);
			send_dm(ts0710, dlci);
		}

		break;

	case UA:
		printk("UA packet received\n");

		if (!dlci) {
			TS0710_DEBUG("server channel == 0|dlci[0].state=%d\n",
				     ts0710->dlci[0].state);

			if (ts0710->dlci[0].state == CONNECTING) {
				ts0710->dlci[0].state = CONNECTED;
				wake_up_interruptible(&ts0710->dlci[0].
						      open_wait);
			} else if (ts0710->dlci[0].state == DISCONNECTING) {
				ts0710_upon_disconnect();
			} else {
				TS0710_DEBUG
				    (" Something wrong receiving UA packet\n");
			}
		} else if (valid_dlci(dlci)) {
			TS0710_DEBUG
			    ("Incomming UA on channel %d|dlci[%d].state=%d\n",
			     dlci, dlci, ts0710->dlci[dlci].state);

			if (ts0710->dlci[dlci].state == CONNECTING) {
				ts0710->dlci[dlci].state = CONNECTED;
				wake_up_interruptible(&ts0710->dlci[dlci].
						      open_wait);
			} else if (ts0710->dlci[dlci].state == DISCONNECTING) {
				ts0710->dlci[dlci].state = DISCONNECTED;
				wake_up_interruptible(&ts0710->dlci[dlci].
						      open_wait);
				wake_up_interruptible(&ts0710->dlci[dlci].
						      close_wait);
				ts0710_reset_dlci(dlci);
			} else {
				TS0710_DEBUG
				    (" Something wrong receiving UA packet\n");
			}
		} else {
			TS0710_DEBUG("invalid dlci %d\n", dlci);
		}

		break;

	case DM:
		printk("DM packet received\n");

		if (!dlci) {
			TS0710_DEBUG("server channel == 0\n");

			if (ts0710->dlci[0].state == CONNECTING) {
				be_connecting = 1;
			} else {
				be_connecting = 0;
			}
			ts0710_upon_disconnect();
			if (be_connecting) {
				ts0710->dlci[0].state = REJECTED;
			}
		} else if (valid_dlci(dlci)) {
			TS0710_DEBUG("Incomming DM on channel %d\n", dlci);

			if (ts0710->dlci[dlci].state == CONNECTING) {
				ts0710->dlci[dlci].state = REJECTED;
			} else {
				ts0710->dlci[dlci].state = DISCONNECTED;
			}
			wake_up_interruptible(&ts0710->dlci[dlci].open_wait);
			wake_up_interruptible(&ts0710->dlci[dlci].close_wait);
			ts0710_reset_dlci(dlci);
		} else {
			TS0710_DEBUG("invalid dlci %d\n", dlci);
		}

		break;

	case DISC:
		printk("DISC packet received\n");

		if (!dlci) {
			TS0710_DEBUG("server channel == 0\n");

			send_ua(ts0710, dlci);
			TS0710_DEBUG("DISC, sending back UA\n");

			ts0710_upon_disconnect();
		} else if (valid_dlci(dlci)) {
			TS0710_DEBUG("Incomming DISC on channel %d\n", dlci);

			send_ua(ts0710, dlci);
			TS0710_DEBUG("DISC, sending back UA\n");

			ts0710->dlci[dlci].state = DISCONNECTED;
			wake_up_interruptible(&ts0710->dlci[dlci].open_wait);
			wake_up_interruptible(&ts0710->dlci[dlci].close_wait);
			ts0710_reset_dlci(dlci);
		} else {
			TS0710_DEBUG("invalid dlci %d\n", dlci);
		}

		break;

	case UIH:
		TS0710_DEBUG("UIH packet received\n");

		if ((dlci >= TS0710_MAX_CHN)) {
			TS0710_DEBUG("invalid dlci %d\n", dlci);
			send_dm(ts0710, dlci);
			break;
		}

		if (GET_PF(short_pkt->h.control)) {
			TS0710_LOG
			    ("MUX Error %s: UIH packet with P/F set, discard it!\n",
			     __FUNCTION__);
			break;
		}

		if ((ts0710->dlci[dlci].state != CONNECTED)
		    && (ts0710->dlci[dlci].state != FLOW_STOPPED)) {
			TS0710_LOG
			    ("MUX Error %s: DLCI %d not connected, discard it!\n",
			     __FUNCTION__, dlci);
			send_dm(ts0710, dlci);
			break;
		}

		if ((short_pkt->h.length.ea) == 0) {
			TS0710_DEBUG("Long UIH packet received\n");
			long_pkt = (long_frame *) data;
			uih_len = GET_LONG_LENGTH(long_pkt->h.length);
			uih_data_start = long_pkt->h.data;
			TS0710_DEBUG("long packet length %d\n", uih_len);

		} else {
			TS0710_DEBUG("Short UIH pkt received\n");
			uih_len = short_pkt->h.length.len;
			uih_data_start = short_pkt->data;

		}
		//added by zhouzhigang
                if((dlci>=3)&&(dlci<=5))
		{
			getnetdata(uih_data_start,uih_len);
			data_size = uih_len;
			//printk("chno:%d, len:%d\n", dlci,uih_len); 
			xmd_net_notify(dlci-1);
			break;
		}
		
		if (dlci == 0) {
			TS0710_DEBUG("UIH on serv_channel 0\n");
			process_mcc(data, len, ts0710,
				    !(short_pkt->h.length.ea));
		} else if (valid_dlci(dlci)) {
			/* do tty dispatch */
			__u8 tty_idx;
			struct tty_struct *tty;
			__u8 queue_data;
			__u8 post_recv;
			__u8 flow_control;
			mux_recv_struct *recv_info;
			int recv_room;
			mux_recv_packet *recv_packet, *recv_packet2;

			TS0710_DEBUG("UIH on channel %d\n", dlci);

			if (uih_len > ts0710->dlci[dlci].mtu) {
				TS0710_PRINTK
				    ("MUX Error:  DLCI:%d, uih_len:%d is bigger than mtu:%d, discard data!\n",
				     dlci, uih_len, ts0710->dlci[dlci].mtu);
				break;
			}

			tty_idx = dlci2tty[dlci].datatty;	//jim : we see data and commmand as same
			tty = mux_table[tty_idx];
			if ((!mux_tty[tty_idx]) || (!tty)) {
				TS0710_PRINTK
				    ("MUX: No application waiting for, discard it! tty=%p,/dev/mux%d\n",
				     tty, tty_idx);
			} else {	/* Begin processing received data */
				if ((!mux_recv_info_flags[tty_idx])
				    || (!mux_recv_info[tty_idx])) {
					TS0710_PRINTK
					    ("MUX Error: No mux_recv_info, discard it! /dev/mux%d\n",
					     tty_idx);
					break;
				}

				recv_info = mux_recv_info[tty_idx];
				if (recv_info->total > 8192) {
					TS0710_PRINTK
					    ("MUX : discard data for tty_idx:%d, recv_info->total > 8192 \n",
					     tty_idx);
					break;
				}

				queue_data = 0;
				post_recv = 0;
				flow_control = 0;
				recv_room = 65535;
				if (tty->receive_room)
					recv_room = tty->receive_room;

				if (test_bit(TTY_THROTTLED, &tty->flags)) {
					queue_data = 1;
				} else {
					if (recv_info->total) {
						queue_data = 1;
						post_recv = 1;
					} else if (recv_room < uih_len) {
						queue_data = 1;
						flow_control = 1;
					}

					if ((recv_room -
					     (uih_len + recv_info->total)) <
					    ts0710->dlci[dlci].mtu) {
						flow_control = 1;
					}
				}

				if (!queue_data) {
					/* Put received data into read buffer of tty */
					TS0710_DEBUG
					    ("Put received data into read buffer of /dev/mux%d",
					     tty_idx);

#ifdef TS0710DEBUG
					t = jiffies;
#endif

					(tty->ldisc->ops->receive_buf) (tty,
								  uih_data_start,
								  NULL,
								  uih_len);

#ifdef TS0710DEBUG
					TS0710_DEBUG
					    ("tty->ldisc.receive_buf:%p,  take ticks: %lu",
					     tty->ldisc->ops->receive_buf,
					     (jiffies - t));
#endif

				} else {	/* Queue data */

					TS0710_DEBUG
					    ("Put received data into recv queue of /dev/mux%d",
					     tty_idx);

					if (recv_info->total) {
						/* recv_info is already linked into mux_recv_queue */

						recv_packet =
						    get_mux_recv_packet
						    (uih_len);
						if (!recv_packet) {
							TS0710_PRINTK
							    ("MUX %s: no memory\n",
							     __FUNCTION__);
							break;
						}

						memcpy(recv_packet->data,
						       uih_data_start, uih_len);
						recv_packet->length = uih_len;
						recv_info->total += uih_len;
						recv_packet->next = NULL;

						if (!(recv_info->mux_packet)) {
							recv_info->mux_packet =
							    recv_packet;
						} else {
							recv_packet2 =
							    recv_info->
							    mux_packet;
							while (recv_packet2->
							       next) {
								recv_packet2 =
								    recv_packet2->
								    next;
							}
							recv_packet2->next =
							    recv_packet;
						}	/* End if( !(recv_info->mux_packet) ) */
					} else {	/* recv_info->total == 0 */
						if (uih_len >
						    TS0710MUX_RECV_BUF_SIZE) {
							TS0710_PRINTK
							    ("MUX Error:  tty_idx:%d, uih_len == %d is too big\n",
							     tty_idx, uih_len);
							uih_len =
							    TS0710MUX_RECV_BUF_SIZE;
						}
						memcpy(recv_info->data,
						       uih_data_start, uih_len);
						recv_info->length = uih_len;
						recv_info->total = uih_len;

						add_post_recv_queue
						    (&mux_recv_queue,
						     recv_info);
					}	/* End recv_info->total == 0 */
				}	/* End Queue data */

				if (flow_control) {
					/* Do something for flow control */
					//printk("\n@jim@ ts0710_recv_data:ts0710_flow_off\n ");
					ts0710_flow_off(tty, dlci, ts0710);
				}
#if 0

				if (tty_idx ==
				    dlci2tty[TS0710MUX_GPRS1_DLCI].datatty) {
					if (add_count
					    (TS0710MUX_GPRS1_RECV_COUNT_IDX,
					     uih_len) < 0) {
						post_recv_count_flag = 1;
						post_recv = 1;
						mux_data_count2
						    [TS0710MUX_GPRS1_RECV_COUNT_IDX]
						    += uih_len;
					}
				} else if (tty_idx ==
					   dlci2tty[TS0710MUX_GPRS2_DLCI].
					   datatty) {
					if (add_count
					    (TS0710MUX_GPRS2_RECV_COUNT_IDX,
					     uih_len) < 0) {
						post_recv_count_flag = 1;
						post_recv = 1;
						mux_data_count2
						    [TS0710MUX_GPRS2_RECV_COUNT_IDX]
						    += uih_len;
					}
				} else if (tty_idx ==
					   dlci2tty[TS0710MUX_VT_DLCI].
					   datatty) {
					if (add_count
					    (TS0710MUX_VT_RECV_COUNT_IDX,
					     uih_len) < 0) {
						post_recv_count_flag = 1;
						post_recv = 1;
						mux_data_count2
						    [TS0710MUX_VT_RECV_COUNT_IDX]
						    += uih_len;
					}
				}
				if (post_recv) {

					//schedule_work(&post_recv_tqueue);
					//      queue_work(muxpost_receive_work_queue,
					//         &mux_post_receive_work);
				}
#endif

			}	/* End processing received data */
		} else {
			TS0710_DEBUG("invalid dlci %d\n", dlci);
		}

		break;

	default:
		TS0710_DEBUG("illegal packet\n");
		break;
	}
	return 0;
}

/* Close ts0710 channel */
static void ts0710_close_channel(__u8 dlci)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int try;
	unsigned long t;

	printk("ts0710_disc_command on channel %d\n", dlci);
	return ; //jim
	if ((ts0710->dlci[dlci].state == DISCONNECTED)
	    || (ts0710->dlci[dlci].state == REJECTED)) {
		return;
	} else if (ts0710->dlci[dlci].state == DISCONNECTING) {
		/* Reentry */
		return;
	} else {
		ts0710->dlci[dlci].state = DISCONNECTING;
		try = 3;
		while (try--) {
			t = jiffies;
			send_disc(ts0710, dlci);
			interruptible_sleep_on_timeout(&ts0710->dlci[dlci].
						       close_wait,
						       TS0710MUX_TIME_OUT);
			if (ts0710->dlci[dlci].state == DISCONNECTED) {
				break;
			} else if (signal_pending(current)) {
				TS0710_PRINTK
				    ("MUX DLCI %d Send DISC got signal!\n",
				     dlci);
				break;
			} else if ((jiffies - t) >= TS0710MUX_TIME_OUT) {
				TS0710_PRINTK
				    ("MUX DLCI %d Send DISC timeout!\n", dlci);
				continue;
			}
		}

		if (ts0710->dlci[dlci].state != DISCONNECTED) {
			if (dlci == 0) {	/* Control Channel */
				ts0710_upon_disconnect();
			} else {	/* Other Channel */
				ts0710->dlci[dlci].state = DISCONNECTED;
				wake_up_interruptible(&ts0710->dlci[dlci].
						      close_wait);
				ts0710_reset_dlci(dlci);
			}
		}
	}
}

int ts0710_open_channel(__u8 dlci)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int try;
	int retval;
	unsigned long t;

      printk("ts0710_open_channel dlci %d\n",dlci);
	retval = -ENODEV;
	if (dlci == 0) {	// control channel
		if ((ts0710->dlci[0].state == CONNECTED)
		    || (ts0710->dlci[0].state == FLOW_STOPPED)) {
			return 0;
		} else if (ts0710->dlci[0].state == CONNECTING) {
			/* Reentry */
			TS0710_PRINTK
			    ("MUX DLCI: 0, reentry to open DLCI 0, pid: %d, %s !\n",
			     current->pid, current->comm);
			try = 11;
			while (try--) {
				t = jiffies;
				interruptible_sleep_on_timeout(&ts0710->dlci[0].
							       open_wait,
							       TS0710MUX_TIME_OUT);
				if ((ts0710->dlci[0].state == CONNECTED)
				    || (ts0710->dlci[0].state == FLOW_STOPPED)) {
					retval = 0;
					break;
				} else if (ts0710->dlci[0].state == REJECTED) {
					retval = -EREJECTED;
					break;
				} else if (ts0710->dlci[0].state ==
					   DISCONNECTED) {
					break;
				} else if (signal_pending(current)) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Wait for connecting got signal!\n",
					     dlci);
					retval = -EAGAIN;
					break;
				} else if ((jiffies - t) >= TS0710MUX_TIME_OUT) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Wait for connecting timeout!\n",
					     dlci);
					continue;
				} else if (ts0710->dlci[0].state == CONNECTING) {
					continue;
				}
			}

			if (ts0710->dlci[0].state == CONNECTING) {
				ts0710->dlci[0].state = DISCONNECTED;
			}
		} else if ((ts0710->dlci[0].state != DISCONNECTED)
			   && (ts0710->dlci[0].state != REJECTED)) {
			TS0710_PRINTK("MUX DLCI:%d state is invalid!\n", dlci);
			return retval;
		} else {
			ts0710->initiator = 1;
			ts0710->dlci[0].state = CONNECTING;
			ts0710->dlci[0].initiator = 1;
			try = 10;
			while (try--) {
				t = jiffies;
				send_sabm(ts0710, 0);
				interruptible_sleep_on_timeout(&ts0710->dlci[0].
							       open_wait,
							       TS0710MUX_TIME_OUT);
				if ((ts0710->dlci[0].state == CONNECTED)
				    || (ts0710->dlci[0].state == FLOW_STOPPED)) {
					retval = 0;
					break;
				} else if (ts0710->dlci[0].state == REJECTED) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Send SABM got rejected!\n",
					     dlci);
					retval = -EREJECTED;
					break;
				} else if (signal_pending(current)) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Send SABM got signal!\n",
					     dlci);
					retval = -EAGAIN;
					break;
				} else if ((jiffies - t) >= TS0710MUX_TIME_OUT) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Send SABM timeout!\n",
					     dlci);
					continue;
				}
			}

			if (ts0710->dlci[0].state == CONNECTING) {
				ts0710->dlci[0].state = DISCONNECTED;
			}
			wake_up_interruptible(&ts0710->dlci[0].open_wait);
		}
	} else {		// other channel
		if ((ts0710->dlci[0].state != CONNECTED)
		    && (ts0710->dlci[0].state != FLOW_STOPPED)) {
			return retval;
		} else if ((ts0710->dlci[dlci].state == CONNECTED)
			   || (ts0710->dlci[dlci].state == FLOW_STOPPED)) {
			return 0;
		} else if ((ts0710->dlci[dlci].state == NEGOTIATING)
			   || (ts0710->dlci[dlci].state == CONNECTING)) {
			/* Reentry */
			try = 8;
			while (try--) {
				t = jiffies;
				interruptible_sleep_on_timeout(&ts0710->
							       dlci[dlci].
							       open_wait,
							       TS0710MUX_TIME_OUT);
				if ((ts0710->dlci[dlci].state == CONNECTED)
				    || (ts0710->dlci[dlci].state ==
					FLOW_STOPPED)) {
					retval = 0;
					break;
				} else if (ts0710->dlci[dlci].state == REJECTED) {
					retval = -EREJECTED;
					break;
				} else if (ts0710->dlci[dlci].state ==
					   DISCONNECTED) {
					break;
				} else if (signal_pending(current)) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Wait for connecting got signal!\n",
					     dlci);
					retval = -EAGAIN;
					break;
				} else if ((jiffies - t) >= TS0710MUX_TIME_OUT) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Wait for connecting timeout!\n",
					     dlci);
					continue;
				} else
				    if ((ts0710->dlci[dlci].state ==
					 NEGOTIATING)
					|| (ts0710->dlci[dlci].state ==
					    CONNECTING)) {
					continue;
				}
			}

			if ((ts0710->dlci[dlci].state == NEGOTIATING)
			    || (ts0710->dlci[dlci].state == CONNECTING)) {
				ts0710->dlci[dlci].state = DISCONNECTED;
			}
		} else if ((ts0710->dlci[dlci].state != DISCONNECTED)
			   && (ts0710->dlci[dlci].state != REJECTED)) {
			TS0710_PRINTK("MUX DLCI:%d state is invalid!\n", dlci);
			return retval;
		} else {
			ts0710->dlci[dlci].state = CONNECTING;
			ts0710->dlci[dlci].initiator = 1;
			if (ts0710->dlci[dlci].state == CONNECTING) {
				try = 3;
				while (try--) {
					t = jiffies;
					send_sabm(ts0710, dlci);
					interruptible_sleep_on_timeout(&ts0710->
								       dlci
								       [dlci].
								       open_wait,
								       TS0710MUX_TIME_OUT);
					if ((ts0710->dlci[dlci].state ==
					     CONNECTED)
					    || (ts0710->dlci[dlci].state ==
						FLOW_STOPPED)) {
						retval = 0;
						break;
					} else if (ts0710->dlci[dlci].state ==
						   REJECTED) {
						TS0710_PRINTK
						    ("MUX DLCI:%d Send SABM got rejected!\n",
						     dlci);
						retval = -EREJECTED;
						break;
					} else if (signal_pending(current)) {
						TS0710_PRINTK
						    ("MUX DLCI:%d Send SABM got signal!\n",
						     dlci);
						retval = -EAGAIN;
						break;
					} else if ((jiffies - t) >=
						   TS0710MUX_TIME_OUT) {
						TS0710_PRINTK
						    ("MUX DLCI:%d Send SABM timeout!\n",
						     dlci);
						continue;
					}
				}
			}

			if ((ts0710->dlci[dlci].state == NEGOTIATING)
			    || (ts0710->dlci[dlci].state == CONNECTING)) {
				ts0710->dlci[dlci].state = DISCONNECTED;
			}
			wake_up_interruptible(&ts0710->dlci[dlci].open_wait);
		}
	}
	return retval;
}

static int ts0710_exec_test_cmd(void)
{
	ts0710_con *ts0710 = &ts0710_connection;
	__u8 *f_buf;		/* Frame buffer */
	__u8 *d_buf;		/* Data buffer */
	int retval = -EFAULT;
	int j;
	unsigned long t;

	if (ts0710->be_testing) {
		/* Reentry */
		t = jiffies;
		interruptible_sleep_on_timeout(&ts0710->test_wait,
					       3 * TS0710MUX_TIME_OUT);
		if (ts0710->be_testing == 0) {
			if (ts0710->test_errs == 0) {
				retval = 0;
			} else {
				retval = -EFAULT;
			}
		} else if (signal_pending(current)) {
			TS0710_DEBUG
			    ("Wait for Test_cmd response got signal!\n");
			retval = -EAGAIN;
		} else if ((jiffies - t) >= 3 * TS0710MUX_TIME_OUT) {
			TS0710_DEBUG("Wait for Test_cmd response timeout!\n");
			retval = -EFAULT;
		}
	} else {
		ts0710->be_testing = 1;	/* Set the flag */

		f_buf = (__u8 *) kmalloc(TEST_PATTERN_SIZE + 32, GFP_KERNEL);
		d_buf = (__u8 *) kmalloc(TEST_PATTERN_SIZE + 32, GFP_KERNEL);
		if ((!f_buf) || (!d_buf)) {
			if (f_buf) {
				kfree(f_buf);
			}
			if (d_buf) {
				kfree(d_buf);
			}

			ts0710->be_testing = 0;	/* Clear the flag */
			ts0710->test_errs = TEST_PATTERN_SIZE;
			wake_up_interruptible(&ts0710->test_wait);
			return -ENOMEM;
		}

		for (j = 0; j < TEST_PATTERN_SIZE; j++) {
			d_buf[j] = j & 0xFF;
		}

		t = jiffies;
		ts0710_test_msg(ts0710, d_buf, TEST_PATTERN_SIZE, MCC_CMD,
				f_buf);
		interruptible_sleep_on_timeout(&ts0710->test_wait,
					       2 * TS0710MUX_TIME_OUT);
		if (ts0710->be_testing == 0) {
			if (ts0710->test_errs == 0) {
				retval = 0;
			} else {
				retval = -EFAULT;
			}
		} else if (signal_pending(current)) {
			TS0710_DEBUG("Send Test_cmd got signal!\n");
			retval = -EAGAIN;
		} else if ((jiffies - t) >= 2 * TS0710MUX_TIME_OUT) {
			TS0710_DEBUG("Send Test_cmd timeout!\n");
			ts0710->test_errs = TEST_PATTERN_SIZE;
			retval = -EFAULT;
		}

		ts0710->be_testing = 0;	/* Clear the flag */
		wake_up_interruptible(&ts0710->test_wait);

		/* Release buffer */
		if (f_buf) {
			kfree(f_buf);
		}
		if (d_buf) {
			kfree(d_buf);
		}
	}

	return retval;
}

//zhouzhigang
static void mux_sched_send(void)
{
	queue_work(mux_work_queue, &mux_send_work);
}

//zhouzhigang
#if 1				//CONFIG_TS0710_MUX_SPI

// Returns 1 if found, 0 otherwise. needle must be null-terminated.
// strstr might not work because WebBox sends garbage before the first OKread
int findInBuf(unsigned char *buf, int len, char *needle)
{
	int i;
	int needleMatchedPos = 0;

	if (needle[0] == '\0') {
		return 1;
	}

	for (i = 0; i < len; i++) {
		if (needle[needleMatchedPos] == buf[i]) {
			needleMatchedPos++;
			if (needle[needleMatchedPos] == '\0') {
				// Entire needle was found
				return 1;
			}
		} else {
			needleMatchedPos = 0;
		}
	}
	return 0;
}

static int cmux_mode = 0;
int is_cmux_mode(void)
{
	return cmux_mode;
}

#endif

/****************************
 * TTY driver routines
*****************************/

//EXPORT_SYMBOL(is_cmux_mode);
static void mux_close(struct tty_struct *tty, struct file *filp)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int line;
	__u8 dlci;
	__u8 cmdtty;
	__u8 datatty;

    printk("mux_close   line %d\n",tty->index);
    if (1 == tty->index || 5 == tty->index || tty->index>14)
       {
           printk("mux_close we modify it");
           return 0;
       }

    UNUSED_PARAM(filp);

	if(cmux_mode)
	{
	     ts0710_init();
           cmux_mode = 0;
	}
       return 0;

	if (!tty) {
		return;
	}
	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		return;
	}
	if (mux_tty[line] > 0)
		mux_tty[line]--;

	dlci = tty2dlci[line];
	cmdtty = dlci2tty[dlci].cmdtty;
	datatty = dlci2tty[dlci].datatty;
	if ((mux_tty[cmdtty] == 0) && (mux_tty[datatty] == 0)) {
		if (dlci == 1) {
			ts0710_close_channel(0);
			TS0710_PRINTK
			    ("MUX mux_close: tapisrv might be down!!! Close DLCI 1\n");
			TS0710_SIG2APLOGD();
		}
		ts0710_close_channel(dlci);
	}

	if (mux_tty[line] == 0) {
		if ((mux_send_info_flags[line])
		    && (mux_send_info[line])
		    /*&& (mux_send_info[line]->filled == 0) */
		    ) {
			mux_send_info_flags[line] = 0;
			kfree(mux_send_info[line]);
			mux_send_info[line] = 0;
			TS0710_DEBUG("Free mux_send_info for /dev/mux%d", line);
		}

		if ((mux_recv_info_flags[line])
		    && (mux_recv_info[line])
		    && (mux_recv_info[line]->total == 0)) {
			mux_recv_info_flags[line] = 0;
			free_mux_recv_struct(mux_recv_info[line]);
			mux_recv_info[line] = 0;
			TS0710_DEBUG("Free mux_recv_info for /dev/mux%d", line);
		}

		ts0710_flow_on(dlci, ts0710);
		//schedule_work(&post_recv_tqueue);
		queue_work(mux_work_queue, &mux_post_receive_work);

		wake_up_interruptible(&tty->read_wait);
		wake_up_interruptible(&tty->write_wait);
		tty->packet = 0;
	}
}

static void mux_throttle(struct tty_struct *tty)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int line;
	int i;
	__u8 dlci;

	if (!tty) {
		return;
	}

	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		return;
	}

	TS0710_DEBUG("Enter into %s, minor number is: %d\n", __FUNCTION__,
		     line);

	dlci = tty2dlci[line];
	if ((ts0710->dlci[0].state != CONNECTED)
	    && (ts0710->dlci[0].state != FLOW_STOPPED)) {
		return;
	} else if ((ts0710->dlci[dlci].state != CONNECTED)
		   && (ts0710->dlci[dlci].state != FLOW_STOPPED)) {
		return;
	}

	if (ts0710->dlci[dlci].flow_control) {
		return;
	}

	for (i = 0; i < 3; i++) {
		if (ts0710_msc_msg
		    (ts0710, EA | FC | RTC | RTR | DV, MCC_CMD, dlci) < 0) {
			continue;
		} else {
			TS0710_LOG("MUX Send Flow off on dlci %d\n", dlci);
			ts0710->dlci[dlci].flow_control = 1;
			break;
		}
	}
}

static void mux_unthrottle(struct tty_struct *tty)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int line;
	__u8 dlci;
	mux_recv_struct *recv_info;

	if (!tty) {
		return;
	}
	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		return;
	}

	if ((!mux_recv_info_flags[line]) || (!mux_recv_info[line])) {
		return;
	}

	TS0710_DEBUG("Enter into %s, minor number is: %d\n", __FUNCTION__,
		     line);

	recv_info = mux_recv_info[line];
	dlci = tty2dlci[line];

	if (recv_info->total) {
		recv_info->post_unthrottle = 1;
		//schedule_work(&post_recv_tqueue);
		queue_work(mux_work_queue, &mux_post_receive_work);

	} else {
		ts0710_flow_on(dlci, ts0710);
	}
}

static int mux_chars_in_buffer(struct tty_struct *tty)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int retval;
	int line;
	__u8 dlci;
	mux_send_struct *send_info;

	retval = TS0710MUX_MAX_CHARS_IN_BUF;
	if (!tty) {
		goto out;
	}
	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		goto out;
	}

	dlci = tty2dlci[line];
	if (ts0710->dlci[0].state == FLOW_STOPPED) {
		TS0710_DEBUG
		    ("Flow stopped on all channels, returning MAX chars in buffer\n");
		goto out;
	} else if (ts0710->dlci[dlci].state == FLOW_STOPPED) {
		TS0710_DEBUG("Flow stopped, returning MAX chars in buffer\n");
		goto out;
	} else if (ts0710->dlci[dlci].state != CONNECTED) {
		TS0710_DEBUG("DLCI %d not connected\n", dlci);
		goto out;
	}

	if (!(mux_send_info_flags[line])) {
		goto out;
	}
	send_info = mux_send_info[line];
	if (!send_info) {
		goto out;
	}
	if (send_info->filled) {
		goto out;
	}

	retval = 0;

      out:
	return retval;
}

extern int get_mux_free();

int mux_net_write(int chno, void *data, int len)
{
      __u16 c;
      static struct s_data *d = NULL;
      int n = 0;
      int ret = 0;

      ret = get_mux_free();
      if(ret < len)
      {
          printk("get_mux_free:%d,len:%d", ret, len);
          return -EBUSY;
       }
	if (!data)
        {
		return -ENOMEM;
	  }
      c = min(len, (DEF_TS0710_MTU - 1));
	if (c <= 0)
        {
		return 0;
        }
      netdata_send_channel[chno-2].line = chno;
	n = write_q(&(netdata_send_channel[chno-2].mux_net_dbuf), data, c, &d);

      if (n == 0)
        {
           queue_work(mux_work_queue, &(netdata_send_channel[chno-2].mux_net_send_work));
           printk("netdata_send_channel[%d-2].mux_net_dbuf full\n ",chno);
           ret = -EBUSY;
         } else{
	     queue_work(mux_work_queue, &(netdata_send_channel[chno-2].mux_net_send_work));
	     ret = 0;
	   }
        return ret;
}
static void net_send_worker(struct work_struct *private_)
{

	ts0710_con *ts0710 = &ts0710_connection;
	int line;
      __u16 c;
	__u8 dlci;
	__u8 *d_buf;
	__u8 post_recv;
	mux_send_struct *send_info;
	int ret;
      int len;

	struct net_channel *ch = (struct net_channel *) container_of(private_,
                                                                                              struct net_channel, mux_net_send_work);    
	line = ch->line;
	if ((line < 0) || (line >= 5))
         {
            return ;
         }

	dlci = tty2dlci[line];
	if (ts0710->dlci[0].state == FLOW_STOPPED)
        {
		TS0710_DEBUG("Flow stopped on all channels, returning zero /dev/mux%d\n", line);
		return ;
	  } else if (ts0710->dlci[dlci].state == FLOW_STOPPED)
	  {
		TS0710_DEBUG("Flow stopped, returning zero /dev/mux%d\n", line);
		return ;
	   } else if (ts0710->dlci[dlci].state == CONNECTED)
	      {
              if (!(mux_send_info_flags[line]))
                {
			TS0710_PRINTK("MUX Error: mux_write: mux_send_info_flags[%d] == 0\n", line);
			return ;
	          }

             send_info = mux_send_info[line];

             if (!send_info)
                {
			TS0710_PRINTK("MUX Error: mux_write: mux_send_info[%d] == 0\n", line);
			return ;
		    }

		d_buf = ((__u8 *) send_info->buf) + TS0710MUX_SEND_BUF_OFFSET;

             while ((len = read_q(line, &d_buf[0], &(netdata_send_channel[line-2].mux_net_dbuf)))!=0)
                {
                    send_info->frame =d_buf;
                    queue_uih(send_info, len, ts0710, dlci);
                    send_info->filled = 1;
                    ret=basic_write(ts0710, (__u8 *) send_info->frame, send_info->length);
                    if(ret < 0)
                    {
                       printk("wait_event_timeout ret %d\n",ret);
			    msleep(100);
                    }
                    send_info->length = 0;
                    send_info->filled = 0;
                    clear_bit(BUF_BUSY, &send_info->flags);
                 }
             return;
	       } else {
		  TS0710_PRINTK("MUX mux_write: DLCI %d not connected\n", dlci);
		  return ;
             }
}

/**
 * mux channel write
 */
int mux_channel_write(struct tty_struct *tty,
		      const unsigned char *buf, int count)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int line;
	__u8 dlci;
	mux_send_struct *send_info;
	__u8 *d_buf;
	__u16 c;
	//__u8 post_recv;

	if (count <= 0) {
		return 0;
	}

	if (!tty) {
		return 0;
	}

	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS))
		return -ENODEV;

	dlci = tty2dlci[line];
	if (ts0710->dlci[0].state == FLOW_STOPPED) {
		TS0710_DEBUG
		    ("Flow stopped on all channels, returning zero /dev/mux%d\n",
		     line);
		return 0;
	} else if (ts0710->dlci[dlci].state == FLOW_STOPPED) {
		TS0710_DEBUG("Flow stopped, returning zero /dev/mux%d\n", line);
		return 0;
	} else if (ts0710->dlci[dlci].state == CONNECTED) {

		if (!(mux_send_info_flags[line])) {
			TS0710_PRINTK
			    ("MUX Error: mux_write: mux_send_info_flags[%d] == 0\n",
			     line);
			return -ENODEV;
		}
		send_info = mux_send_info[line];
		if (!send_info) {
			TS0710_PRINTK
			    ("MUX Error: mux_write: mux_send_info[%d] == 0\n",
			     line);
			return -ENODEV;
		}

		c = min(count, (ts0710->dlci[dlci].mtu - 1));

		if (c <= 0) {
			return 0;
		}

		if (test_and_set_bit(BUF_BUSY, &send_info->flags))
			return 0;

		if (send_info->filled) {
			clear_bit(BUF_BUSY, &send_info->flags);
			return 0;
		}

		d_buf = ((__u8 *) send_info->buf) + TS0710MUX_SEND_BUF_OFFSET;
		memcpy(&d_buf[0], buf, c);	//jim : no tag byte  
	

		TS0710_DEBUG("Prepare to send %d bytes from /dev/mux%d", c,
			     line);
		TS0710_DEBUGHEX(d_buf, c);

		send_info->frame = d_buf;
		queue_uih(send_info, c, ts0710, dlci);
		send_info->filled = 1;

		clear_bit(BUF_BUSY, &send_info->flags);

#if 0
		post_recv = 0;

		if (dlci == TS0710MUX_GPRS1_DLCI) {
			if (add_count(TS0710MUX_GPRS1_SEND_COUNT_IDX, c) < 0) {
				post_recv_count_flag = 1;
				post_recv = 1;
				mux_data_count2[TS0710MUX_GPRS1_SEND_COUNT_IDX]
				    += c;
			}
		} else if (dlci == TS0710MUX_GPRS2_DLCI) {
			if (add_count(TS0710MUX_GPRS2_SEND_COUNT_IDX, c) < 0) {
				post_recv_count_flag = 1;
				post_recv = 1;
				mux_data_count2[TS0710MUX_GPRS2_SEND_COUNT_IDX]
				    += c;
			}
		} else if (dlci == TS0710MUX_VT_DLCI) {
			if (add_count(TS0710MUX_VT_SEND_COUNT_IDX, c) < 0) {
				post_recv_count_flag = 1;
				post_recv = 1;
				mux_data_count2[TS0710MUX_VT_SEND_COUNT_IDX]
				    += c;
			}
		}

		if (post_recv)
			//      schedule_work(&post_recv_tqueue);
			queue_work(muxpost_receive_work_queue,
				   &mux_post_receive_work);

#endif
        ///zhouzhigang
		mux_sched_send();
		return c;
	} else {
		TS0710_PRINTK("MUX mux_write: DLCI %d not connected\n", dlci);
		return -EDISCONNECTED;
	}

}



static char stm_data[TS0710MUX_MAX_BUF_SIZE * 10 ];
struct mux_ringbuffer stmbuf;
static char stm_tmp_buf[4096];
static inline int scan_ordinary(const unsigned char *buf, int count)
{
	int i, c;

	for (i = 0; i < count; ++i) {
		c = buf[i];
		if (c == 0x7e/*PPP_FLAG*/ ){
			break;
		}
	}
	return i;
}

static int mux_write(struct tty_struct *tty,
		     const unsigned char *buf, int count)
{
	int ret;
	int line;

	if (count <= 0) {
		return -1;
	}

	if (!tty) {
		return -1;
	}

	line = tty->index;

	if ((line < 0) || (line >= NR_MUXS))
		return -ENODEV;
	/**
	 * if video call channel, then wake up kernel thread for sending data,
	 * others nornal mux buffer write
	 */
	//printk("\nThe mux line = %d,count = %d,\n",line,count);


	/*if (line == VT_CHANNEL) {
		//zhouzhigang
		//ret = vt_buffer_load(tty, buf, count);
	} else if(line == STM_CHANNEL){
		
		ret = stm_buffer_load(tty, buf, count);
	}else {
		ret = mux_channel_write(tty, buf, count);
	}*/
    ret = mux_channel_write(tty, buf, count);
	return ret;
}

static int mux_write_room(struct tty_struct *tty)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int retval;
	int line;
	__u8 dlci;
	mux_send_struct *send_info;

	retval = 0;
	if (!tty) {
		goto out;
	}
	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		goto out;
	}

	dlci = tty2dlci[line];
	if (ts0710->dlci[0].state == FLOW_STOPPED) {
		TS0710_DEBUG("Flow stopped on all channels, returning ZERO\n");
		goto out;
	} else if (ts0710->dlci[dlci].state == FLOW_STOPPED) {
		TS0710_DEBUG("Flow stopped, returning ZERO\n");
		goto out;
	} else if (ts0710->dlci[dlci].state != CONNECTED) {
		TS0710_DEBUG("DLCI %d not connected\n", dlci);
		goto out;
	}

	if (!(mux_send_info_flags[line])) {
		goto out;
	}
	send_info = mux_send_info[line];
	if (!send_info) {
		goto out;
	}
	if (send_info->filled) {
		goto out;
	}

	retval = ts0710->dlci[dlci].mtu - 1;

      out:
	return retval;
}

static int mux_ioctl(struct tty_struct *tty, struct file *file,
		     unsigned int cmd, unsigned long arg)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int line;
	__u8 dlci;

	UNUSED_PARAM(file);
	UNUSED_PARAM(arg);

	if (!tty) {
		return -EIO;
	}
	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		return -ENODEV;
	}

	dlci = tty2dlci[line];
	switch (cmd) {
	case TS0710MUX_IO_MSC_HANGUP:
		if (ts0710_msc_msg(ts0710, EA | RTR | DV, MCC_CMD, dlci) < 0) {
			return -EAGAIN;
		} else {
			return 0;
		}

	case TS0710MUX_IO_TEST_CMD:
		return ts0710_exec_test_cmd();
	default:
		break;
	}
	return -ENOIOCTLCMD;
}

static void mux_flush_buffer(struct tty_struct *tty)
{
	int line;

	if (!tty) {
		return;
	}

	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		return;
	}

	TS0710_PRINTK("MUX %s: line is:%d\n", __FUNCTION__, line);
	if (line == STM_CHANNEL) mux_ringbuffer_flush(&stmbuf);
	if ((mux_send_info_flags[line])
	    && (mux_send_info[line])
	    && (mux_send_info[line]->filled)) {

		mux_send_info[line]->filled = 0;
	}

	wake_up_interruptible(&tty->write_wait);
#ifdef SERIAL_HAVE_POLL_WAIT
	wake_up_interruptible(&tty->poll_wait);
#endif
	if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
	    tty->ldisc->ops->write_wakeup) {
		(tty->ldisc->ops->write_wakeup) (tty);
	}

}

static int mux_open(struct tty_struct *tty, struct file *filp)
{
	int retval;
	int line;
	__u8 dlci;
	__u8 cmdtty;
	__u8 datatty;
	mux_send_struct *send_info;
	mux_recv_struct *recv_info;
	
	UNUSED_PARAM(filp);

	retval = -ENODEV;

	if (!tty) {
		goto out;
	}

	printk("\nmux_open,index = %d\n", tty->index);

	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		goto out;
	}
#ifdef TS0710SERVER
	/* do nothing as a server */
	mux_tty[line]++;
	retval = 0;
#else
	mux_tty[line]++;
	dlci = tty2dlci[line];

	if (dlci == 1) {
		if (cmux_mode == 0) {
			char buffer[256];
			int i = 0;
			memset(buffer, 0, 256);
			mux_ringbuffer_flush(&rbuf);

            //zhouzhigang
           /* if(!power_on){
			    set_modem_state(MODEM_POWER_ON);
				power_on = 1;
                return -EIO;
			}
			else
			    msleep(6000);*/

			while (1) {
				int c = mux_ringbuffer_avail(&rbuf);
				if (c > 0) {
					if (c > 256)
						c = 256;
					mux_ringbuffer_read(&rbuf, buffer, c, 0);
					if (findInBuf(buffer, 256, "OK"))
						break;
					if (findInBuf(buffer, 256, "ERROR")) {
						printk
						    ("\n wrong modem state !!!!\n");
						break;
					}
				} else {
				    printk("\n start send at:<\n");
					write_data_to_modem_buf("at\r", strlen("at\r"));
					msleep(1000);
					++i;
				}
				if (i > 3) {
					printk("\n wrong modem state !!!!\n");
					//break;
					return -EIO;
				}
			}

            printk("\n start send omux tx buf:<\n");
			write_data_to_modem_buf("at+cmux=0\r", strlen("at+cmux=0\r"));			
			msleep(1000);
			printk("\n cmux receive>\n");
			//empty ringbuffer
			cmux_mode = 1;
			mux_ringbuffer_flush(&rbuf);
		}

		/* Open server channel 0 first */
		if ((retval = ts0710_open_channel(0)) != 0) {
			TS0710_PRINTK("MUX: Can't connect server channel 0!\n");
			ts0710_init();

			mux_tty[line]--;
			goto out;
		}
	}

	/* Allocate memory first. As soon as connection has been established, MUX may receive */
	if (mux_send_info_flags[line] == 0) {
		send_info =
		    (mux_send_struct *) kmalloc(sizeof(mux_send_struct),
						GFP_KERNEL);
		if (!send_info) {
			retval = -ENOMEM;

			mux_tty[line]--;
			goto out;
		}
		send_info->length = 0;
		send_info->flags = 0;
		send_info->filled = 0;
		mux_send_info[line] = send_info;
		mux_send_info_flags[line] = 1;
		TS0710_DEBUG("Allocate mux_send_info for /dev/mux%d", line);
	}

	if (mux_recv_info_flags[line] == 0) {
		recv_info =
		    (mux_recv_struct *) kmalloc(sizeof(mux_recv_struct),
						GFP_KERNEL);
		if (!recv_info) {
			mux_send_info_flags[line] = 0;
			kfree(mux_send_info[line]);
			mux_send_info[line] = 0;
			TS0710_DEBUG("Free mux_send_info for /dev/mux%d", line);
			retval = -ENOMEM;

			mux_tty[line]--;
			goto out;
		}
		recv_info->length = 0;
		recv_info->total = 0;
		recv_info->mux_packet = 0;
		recv_info->next = 0;
		recv_info->no_tty = line;
		recv_info->post_unthrottle = 0;
		mux_recv_info[line] = recv_info;
		mux_recv_info_flags[line] = 1;
		TS0710_DEBUG("Allocate mux_recv_info for /dev/mux%d", line);
	}

	/* Now establish DLCI connection */
	cmdtty = dlci2tty[dlci].cmdtty;
	datatty = dlci2tty[dlci].datatty;
	if ((mux_tty[cmdtty] > 0) || (mux_tty[datatty] > 0)) {
		if ((retval = ts0710_open_channel(dlci)) != 0) {
			TS0710_PRINTK("MUX: Can't connected channel %d!\n",
				      dlci);
			ts0710_reset_dlci(dlci);

			mux_send_info_flags[line] = 0;
			kfree(mux_send_info[line]);
			mux_send_info[line] = 0;
			TS0710_DEBUG("Free mux_send_info for /dev/mux%d", line);

			mux_recv_info_flags[line] = 0;
			free_mux_recv_struct(mux_recv_info[line]);
			mux_recv_info[line] = 0;
			TS0710_DEBUG("Free mux_recv_info for /dev/mux%d", line);

			mux_tty[line]--;
			goto out;
		}
	}
	retval = 0;
#endif
      out:
	return retval;
}

static inline int rt_policy(int policy)
{
	if (unlikely(policy == SCHED_FIFO) || unlikely(policy == SCHED_RR))
		return 1;
	return 0;
}

static inline int task_has_rt_policy(struct task_struct *p)
{
	return rt_policy(p->policy);
}

/* mux dispatcher, call from serial.c receiver_chars() */
void mux_dispatcher(struct tty_struct *tty)
{
	UNUSED_PARAM(tty);

	if (queue_work(mux_work_queue, &mux_receive_work) == 0) {
		printk("MUX:rec work pending!\n");
	}

}

/*For BP UART problem Begin*/
#ifdef TS0710SEQ2
static int send_ack(ts0710_con * ts0710, __u8 seq_num, __u8 bp_seq1,
		    __u8 bp_seq2)
#else
static int send_ack(ts0710_con * ts0710, __u8 seq_num)
#endif
{
	__u8 buf[20];
	short_frame *ack;

#ifdef TS0710SEQ2
	static __u16 ack_seq = 0;
#endif
	return 0;		//jim
	ack = (short_frame *) (buf + 1);
	ack->h.addr.ea = 1;
	ack->h.addr.cr = ((ts0710->initiator) & 0x1);
	ack->h.addr.d = 0;
	ack->h.addr.server_chn = 0;
	ack->h.control = ACK;
	ack->h.length.ea = 1;

#ifdef TS0710SEQ2
	ack->h.length.len = 5;
	ack->data[0] = seq_num;
	ack->data[1] = bp_seq1;
	ack->data[2] = bp_seq2;
	ack->data[3] = (ack_seq & 0xFF);
	ack->data[4] = (ack_seq >> 8) & 0xFF;
	ack_seq++;
	ack->data[5] = crc_calc((__u8 *) ack, SHORT_CRC_CHECK);
#else
	ack->h.length.len = 1;
	ack->data[0] = seq_num;
	ack->data[1] = crc_calc((__u8 *) ack, SHORT_CRC_CHECK);
#endif

	return basic_write(ts0710, buf,
			   (sizeof(short_frame) + FCS_SIZE +
			    ack->h.length.len));
}

/*For BP UART problem End*/

//int mux_set_thread_pro(int pro)
//{
//
//	struct sched_param s = {.sched_priority = MAX_RT_PRIO - pro };
//
//	/* just for this write, set us real-time */
//	if (!task_has_rt_policy(current)) {
//		cap_raise(current->cap_effective, CAP_SYS_NICE);
//		sched_setscheduler(current, SCHED_RR, &s);
//	}
//	return 0;
//}

static void receive_worker(struct work_struct *private_)
{
	int count;
	static unsigned char tbuf[TS0710MUX_MAX_BUF_SIZE];
	static unsigned char *tbuf_ptr = &tbuf[0];
	static unsigned char *start_flag = 0;
	unsigned char *search, *to, *from;
	short_frame *short_pkt;
	long_frame *long_pkt;
	static int framelen = -1;
	/*For BP UART problem Begin */
	static __u8 expect_seq = 0;
	__u32 crc_error;
	__u8 *uih_data_start;
	__u32 uih_len;
	/*For BP UART problem End */

	//UNUSED_PARAM(private_);

	//mux_set_thread_pro(95);

	//printk("receive_worker\n");

	count = mux_ringbuffer_avail(&rbuf);

	if (count < 0){
		return;
	}


	if (count == 0) {
		if (work_pending(&mux_receive_work)) {
			queue_work(mux_work_queue, &mux_receive_work);
		}

		return;
	}

	if (count > (TS0710MUX_MAX_BUF_SIZE - (tbuf_ptr - tbuf))) {

		count = (TS0710MUX_MAX_BUF_SIZE - (tbuf_ptr - tbuf));
		queue_work(mux_work_queue, &mux_receive_work);

	}

	mux_ringbuffer_read(&rbuf, tbuf_ptr, count, 0);
	tbuf_ptr += count;

	if (test_and_set_bit(RECV_RUNNING, &mux_recv_flags)) {
		queue_work(mux_work_queue, &mux_receive_work);
		return;
	}
	if ((start_flag != 0) && (framelen != -1)) {
		if ((tbuf_ptr - start_flag) < framelen) {
			clear_bit(RECV_RUNNING, &mux_recv_flags);
			if (work_pending(&mux_receive_work)) {
				queue_work(mux_work_queue,
					   &mux_receive_work);
			}
			return;
		}
	}

	search = &tbuf[0];
	while (1) {
		if (start_flag == 0) {	/* Frame Start Flag not found */
			framelen = -1;
			while (search < tbuf_ptr) {
				if (*search == TS0710_BASIC_FLAG) {

					start_flag = search;
					break;
				}
#ifdef TS0710LOG
				else {
					TS0710_LOG(">S %02x %c\n", *search,
						   *search);
				}
#endif

				search++;
			}

			if (start_flag == 0) {
				tbuf_ptr = &tbuf[0];
				break;
			}
		} else {	/* Frame Start Flag found */
			/* 1 start flag + 1 address + 1 control + 1 or 2 length + lengths data + 1 FCS + 1 end flag */
			/* For BP UART problem 1 start flag + 1 seq_num + 1 address + ...... */
			/*if( (framelen == -1) && ((tbuf_ptr - start_flag) > TS0710_MAX_HDR_SIZE) ) */
			if ((framelen == -1) && ((tbuf_ptr - start_flag) > (TS0710_MAX_HDR_SIZE + SEQ_FIELD_SIZE))) {	/*For BP UART problem */
				/*short_pkt = (short_frame *) (start_flag + 1); */
				short_pkt = (short_frame *) (start_flag + ADDRESS_FIELD_OFFSET);	/*For BP UART problem */
				if (short_pkt->h.length.ea == 1) {	/* short frame */
					/*framelen = TS0710_MAX_HDR_SIZE + short_pkt->h.length.len + 1; */
					framelen = TS0710_MAX_HDR_SIZE + short_pkt->h.length.len + 1 + SEQ_FIELD_SIZE;	/*For BP UART problem */
				} else {	/* long frame */
					/*long_pkt = (long_frame *) (start_flag + 1); */
					long_pkt = (long_frame *) (start_flag + ADDRESS_FIELD_OFFSET);	/*For BP UART problem */
					/*framelen = TS0710_MAX_HDR_SIZE + GET_LONG_LENGTH( long_pkt->h.length ) + 2; */
					framelen = TS0710_MAX_HDR_SIZE + GET_LONG_LENGTH(long_pkt->h.length) + 2 + SEQ_FIELD_SIZE;	/*For BP UART problem */
				}

				/*if( framelen > TS0710MUX_MAX_TOTAL_FRAME_SIZE ) { */
				if (framelen > (TS0710MUX_MAX_TOTAL_FRAME_SIZE + SEQ_FIELD_SIZE)) {	/*For BP UART problem */
					TS0710_LOGSTR_FRAME(0, start_flag,
							    (tbuf_ptr -
							     start_flag));
					TS0710_PRINTK
					    ("MUX Error: %s: frame length:%d is bigger than Max total frame size:%d\n",
		 /*__FUNCTION__, framelen, TS0710MUX_MAX_TOTAL_FRAME_SIZE);*/
					     __FUNCTION__, framelen, (TS0710MUX_MAX_TOTAL_FRAME_SIZE + SEQ_FIELD_SIZE));	/*For BP UART problem */
					search = start_flag + 1;
					start_flag = 0;
					framelen = -1;
					continue;
				}
			}

			if ((framelen != -1)
			    && ((tbuf_ptr - start_flag) >= framelen)) {
				if (*(start_flag + framelen - 1) == TS0710_BASIC_FLAG) {	/* OK, We got one frame */

					/*For BP UART problem Begin */
					TS0710_LOGSTR_FRAME(0, start_flag,
							    framelen);
		//			TS0710_DEBUGHEX(start_flag, framelen);

					short_pkt =
					    (short_frame *) (start_flag +
							     ADDRESS_FIELD_OFFSET);
					if ((short_pkt->h.length.ea) == 0) {
						long_pkt =
						    (long_frame *) (start_flag +
								    ADDRESS_FIELD_OFFSET);
						uih_len =
						    GET_LONG_LENGTH(long_pkt->h.
								    length);
						uih_data_start =
						    long_pkt->h.data;

						crc_error =
						    crc_check((__u8
							       *) (start_flag +
								   SLIDE_BP_SEQ_OFFSET),
							      LONG_CRC_CHECK +
							      1,
							      *(uih_data_start +
								uih_len));
					} else {
						uih_len =
						    short_pkt->h.length.len;
						uih_data_start =
						    short_pkt->data;

						crc_error =
						    crc_check((__u8
							       *) (start_flag +
								   SLIDE_BP_SEQ_OFFSET),
							      SHORT_CRC_CHECK +
							      1,
							      *(uih_data_start +
								uih_len));
					}

					if (!crc_error) {
						if (	/*expect_seq ==
							 *(start_flag +
							 SLIDE_BP_SEQ_OFFSET)*/
							   1) {
							expect_seq =
							    *(start_flag +
							      SLIDE_BP_SEQ_OFFSET);
							expect_seq++;
							if (expect_seq >= 4) {
								expect_seq = 0;
							}
#ifdef TS0710SEQ2
							send_ack
							    (&ts0710_connection,
							     expect_seq,
							     *(start_flag +
							       FIRST_BP_SEQ_OFFSET),
							     *(start_flag +
							       SECOND_BP_SEQ_OFFSET));
#else
							send_ack
							    (&ts0710_connection,
							     expect_seq);
#endif

							ts0710_recv_data
							    (&ts0710_connection,
							     start_flag +
							     ADDRESS_FIELD_OFFSET,
							     framelen - 2 -
							     SEQ_FIELD_SIZE);
						} else {

#ifdef TS0710DEBUG
							if (*
							    (start_flag +
							     SLIDE_BP_SEQ_OFFSET)
!= 0x9F) {
#endif

								TS0710_LOG
								    ("MUX sequence number %d is not expected %d, discard data!\n",
								     *
								     (start_flag
								      +
								      SLIDE_BP_SEQ_OFFSET),
								     expect_seq);

#ifdef TS0710SEQ2
								send_ack
								    (&ts0710_connection,
								     expect_seq,
								     *
								     (start_flag
								      +
								      FIRST_BP_SEQ_OFFSET),
								     *
								     (start_flag
								      +
								      SECOND_BP_SEQ_OFFSET));
#else
								send_ack
								    (&ts0710_connection,
								     expect_seq);
#endif

#ifdef TS0710DEBUG
							} else {
								*(uih_data_start
								  + uih_len) =
						     0;
								TS0710_PRINTK
								    ("MUX bp log: %s\n",
								     uih_data_start);
							}
#endif

						}
					} else {	/* crc_error */
						search = start_flag + 1;
						start_flag = 0;
						framelen = -1;
						continue;
					}	/*End if(!crc_error) */

					search = start_flag + framelen;
				} else {
					TS0710_LOGSTR_FRAME(0, start_flag,
							    framelen);
		//			TS0710_DEBUGHEX(start_flag, framelen);
					TS0710_PRINTK
					    ("MUX: Lost synchronization!\n");
					search = start_flag + 1;
				}

				start_flag = 0;
				framelen = -1;
				continue;
			}

			if (start_flag != &tbuf[0]) {

				to = tbuf;
				from = start_flag;
				count = tbuf_ptr - start_flag;
				while (count--) {
					*to++ = *from++;
				}

				tbuf_ptr -= (start_flag - tbuf);
				start_flag = tbuf;

			}
			break;
		}		/* End Frame Start Flag found */
	}			/* End while(1) */

	clear_bit(RECV_RUNNING, &mux_recv_flags);
	if (work_pending(&mux_receive_work)) {
		queue_work(mux_work_queue, &mux_receive_work);
	}

}

static void post_recv_worker(struct work_struct *private_)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int tty_idx;
	struct tty_struct *tty;
	__u8 post_recv;
	__u8 flow_control;
	__u8 dlci;
	mux_recv_struct *recv_info, *recv_info2, *post_recv_q;
	int recv_room;
	mux_recv_packet *recv_packet, *recv_packet2;
	__u8 j;

	UNUSED_PARAM(private_);
	//mux_set_thread_pro(90);
	printk("post_recv_worker\n");

	if (test_and_set_bit(RECV_RUNNING, &mux_recv_flags)) {
		//schedule_work(&post_recv_tqueue);
		queue_work(mux_work_queue, &mux_post_receive_work);

		return;
	}

	TS0710_DEBUG("Enter into post_recv_worker");

	post_recv = 0;
	if (!mux_recv_queue) {
		goto out;
	}

	post_recv_q = NULL;
	recv_info2 = mux_recv_queue;
	while ((recv_info = recv_info2)) {
		recv_info2 = recv_info->next;

		if (!(recv_info->total)) {
			TS0710_PRINTK
			    ("MUX Error: %s: Should not get here, recv_info->total == 0 \n",
			     __FUNCTION__);
			continue;
		}

		tty_idx = recv_info->no_tty;
		dlci = tty2dlci[tty_idx];
		tty = mux_table[tty_idx];
		if ((!mux_tty[tty_idx]) || (!tty)) {
			TS0710_PRINTK
			    ("MUX: No application waiting for, free recv_info! tty_idx:%d\n",
			     tty_idx);
			mux_recv_info_flags[tty_idx] = 0;
			free_mux_recv_struct(mux_recv_info[tty_idx]);
			mux_recv_info[tty_idx] = 0;
			ts0710_flow_on(dlci, ts0710);
			continue;
		}

		TS0710_DEBUG("/dev/mux%d recv_info->total is: %d", tty_idx,
			     recv_info->total);

		if (test_bit(TTY_THROTTLED, &tty->flags)) {
			add_post_recv_queue(&post_recv_q, recv_info);
			continue;
		}
		flow_control = 0;
		recv_packet2 = recv_info->mux_packet;
		while (recv_info->total) {
			recv_room = 65535;
			if (tty->receive_room)
				recv_room = tty->receive_room;

			if (recv_info->length) {
				if (recv_room < recv_info->length) {
					flow_control = 1;
					break;
				}

				/* Put queued data into read buffer of tty */
				TS0710_DEBUG
				    ("Put queued recv data into read buffer of /dev/mux%d\n",
				     tty_idx);
	//			TS0710_DEBUGHEX(recv_info->data,recv_info->length);
				(tty->ldisc->ops->receive_buf) (tty, recv_info->data,
							  NULL,
							  recv_info->length);
				recv_info->total -= recv_info->length;
				recv_info->length = 0;
			} else {	/* recv_info->length == 0 */
				if ((recv_packet = recv_packet2)) {
					recv_packet2 = recv_packet->next;

					if (recv_room < recv_packet->length) {
						flow_control = 1;
						recv_info->mux_packet =
						    recv_packet;
						break;
					}

					/* Put queued data into read buffer of tty */
					TS0710_DEBUG
					    ("Put queued recv data into read buffer2 of /dev/mux%d \n",
					     tty_idx);
		//			TS0710_DEBUGHEX(recv_packet->data,recv_packet->length);
					(tty->ldisc->ops->receive_buf) (tty,
								  recv_packet->
								  data, NULL,
								  recv_packet->
								  length);
					recv_info->total -= recv_packet->length;
					free_mux_recv_packet(recv_packet);
				} else {
					TS0710_PRINTK
					    ("MUX Error: %s: Should not get here, recv_info->total is:%u \n",
					     __FUNCTION__, recv_info->total);
				}
			}	/* End recv_info->length == 0 */
		}		/* End while( recv_info->total ) */

		if (!(recv_info->total)) {
			/* Important clear */
			recv_info->mux_packet = 0;

			if (recv_info->post_unthrottle) {
				/* Do something for post_unthrottle */
				ts0710_flow_on(dlci, ts0710);
				recv_info->post_unthrottle = 0;
			}
		} else {
			add_post_recv_queue(&post_recv_q, recv_info);

			if (flow_control) {
				/* Do something for flow control */
				if (recv_info->post_unthrottle) {
					set_bit(TTY_THROTTLED, &tty->flags);
					recv_info->post_unthrottle = 0;
				} else {
					ts0710_flow_off(tty, dlci, ts0710);
				}
			}	/* End if( flow_control ) */
		}
	}			/* End while( (recv_info = recv_info2) ) */

	mux_recv_queue = post_recv_q;

      out:
	if (post_recv_count_flag) {
		post_recv_count_flag = 0;
		for (j = 0; j < TS0710MUX_COUNT_IDX_NUM; j++) {
			if (mux_data_count2[j] > 0) {
				if (add_count(j, mux_data_count2[j]) == 0) {
					mux_data_count2[j] = 0;
				} else {
					post_recv_count_flag = 1;
					post_recv = 1;
				}
			}
		}		/* End for (j = 0; j < TS0710MUX_COUNT_IDX_NUM; j++) */
	}
	/* End if( post_recv_count_flag ) */
	if (post_recv)
		queue_work(mux_work_queue, &mux_post_receive_work);

	clear_bit(RECV_RUNNING, &mux_recv_flags);
}

/* mux sender, call from serial.c transmit_chars() */
void mux_sender(void)
{
	mux_send_struct *send_info;
	int chars;
	__u8 idx;

	//chars = omux_chars_in_tx_buffer();
	//if (!chars) {
		/* chars == 0 */
		TS0710_LOG("<[]\n");
		mux_sched_send();
		return;
	//}

	idx = mux_send_info_idx;
	if ((idx < NR_MUXS) && (mux_send_info_flags[idx])) {
		send_info = mux_send_info[idx];
		if ((send_info)
		    && (send_info->filled)
		    && !chars ) {

			mux_sched_send();
		}
	}
}
//EXPORT_SYMBOL(mux_sender);
static void send_worker(struct work_struct *private_)
{
	ts0710_con *ts0710 = &ts0710_connection;
	__u8 j;
	mux_send_struct *send_info;
	//int chars;
	struct tty_struct *tty;
	__u8 dlci;
	static int first_in = 1;

	UNUSED_PARAM(private_);

	TS0710_DEBUG("Enter into send_worker");
	//printk("send_worker\n");

	mux_send_info_idx = NR_MUXS;

	if (ts0710->dlci[0].state == FLOW_STOPPED) {
		TS0710_DEBUG("Flow stopped on all channels\n");
		return;
	}

    //zhouzhigang
    if(first_in)
    {
      // mux_set_thread_pro(80);
       set_user_nice(current, -10);
	   first_in = 0;
	   printk("send_worker:set pro to 80\n");
    }

	for (j = 0; j < NR_MUXS; j++) {
        if(j==2||j==3||j==4)
            continue;
		if (!(mux_send_info_flags[j])) {
			continue;
		}

		send_info = mux_send_info[j];
		if (!send_info) {
			continue;
		}

		if (!(send_info->filled)) {
			continue;
		}

		dlci = tty2dlci[j];
		if (ts0710->dlci[dlci].state == FLOW_STOPPED) {
			TS0710_DEBUG("Flow stopped on channel DLCI: %d\n",
				     dlci);
			continue;
		} else if (ts0710->dlci[dlci].state != CONNECTED) {
			TS0710_DEBUG("DLCI %d not connected\n", dlci);
			send_info->filled = 0;
			continue;
		}
			TS0710_DEBUG("Send queued UIH for /dev/mux%d", j);
			//printk("MUX:write to spi,for /dev/mux%d start\n",j);
			basic_write(ts0710, (__u8 *) send_info->frame,
				    send_info->length);
			//printk("MUX:write to spi,for /dev/mux%d end\n",j);
			send_info->length = 0;
			send_info->filled = 0;
		//} else {
		//	mux_send_info_idx = j;
		//	break;
		//}
	}			/* End for() loop */

	/* Queue UIH data to be transmitted */
	for (j = 0; j < NR_MUXS; j++) {
        if(j==2||j==3||j==4)
            continue;
		if (!(mux_send_info_flags[j])) {
			continue;
		}

		send_info = mux_send_info[j];
		if (!send_info) {
			continue;
		}

		if (send_info->filled) {
			continue;
		}

		/* Now queue UIH data to send_info->buf */

		if (!mux_tty[j]) {
			continue;
		}

		tty = mux_table[j];
		if (!tty) {
			continue;
		}

		dlci = tty2dlci[j];
		if (ts0710->dlci[dlci].state == FLOW_STOPPED) {
			TS0710_DEBUG("Flow stopped on channel DLCI: %d\n",
				     dlci);
			continue;
		} else if (ts0710->dlci[dlci].state != CONNECTED) {
			TS0710_DEBUG("DLCI %d not connected\n", dlci);
			continue;
		}

		if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP))
		    && tty->ldisc->ops->write_wakeup) {
			(tty->ldisc->ops->write_wakeup) (tty);
		}
		wake_up_interruptible(&tty->write_wait);

#ifdef SERIAL_HAVE_POLL_WAIT
		wake_up_interruptible(&tty->poll_wait);
#endif

		if (send_info->filled) {
			if (j < mux_send_info_idx) {
				mux_send_info_idx = j;
			}
		}
	}			/* End for() loop */
 TS0710_DEBUG("Out send_worker");
}

static int get_count(__u8 idx)
{
	int ret;

	if (idx > TS0710MUX_COUNT_MAX_IDX) {
		TS0710_PRINTK("MUX get_count: invalid idx: %d!\n", idx);
		return -1;
	}

	down(&mux_data_count_mutex[idx]);
	ret = mux_data_count[idx];
	up(&mux_data_count_mutex[idx]);

	return ret;
}

static int set_count(__u8 idx, int count)
{
	if (idx > TS0710MUX_COUNT_MAX_IDX) {
		TS0710_PRINTK("MUX set_count: invalid idx: %d!\n", idx);
		return -1;
	}
	if (count < 0) {
		TS0710_PRINTK("MUX set_count: invalid count: %d!\n", count);
		return -1;
	}

	down(&mux_data_count_mutex[idx]);
	mux_data_count[idx] = count;
	up(&mux_data_count_mutex[idx]);

	return 0;
}

static int add_count(__u8 idx, int count)
{
	if (idx > TS0710MUX_COUNT_MAX_IDX) {
		TS0710_PRINTK("MUX add_count: invalid idx: %d!\n", idx);
		return -1;
	}
	if (count <= 0) {
		TS0710_PRINTK("MUX add_count: invalid count: %d!\n", count);
		return -1;
	}

	if (down_trylock(&mux_data_count_mutex[idx]))
		return -1;
	mux_data_count[idx] += count;
	up(&mux_data_count_mutex[idx]);

	return 0;
}

ssize_t file_proc_read(struct file * file, char *buf, size_t size,
		       loff_t * ppos)
{
	gprs_bytes gprsData[TS0710MUX_GPRS_SESSION_MAX];
	int bufLen = sizeof(gprs_bytes) * TS0710MUX_GPRS_SESSION_MAX;
	int error = 0;
	UNUSED_PARAM(file);
	UNUSED_PARAM(size);
	UNUSED_PARAM(ppos);

	gprsData[0].recvBytes = get_count(TS0710MUX_GPRS1_RECV_COUNT_IDX);
	gprsData[0].sentBytes = get_count(TS0710MUX_GPRS1_SEND_COUNT_IDX);
	gprsData[TS0710MUX_GPRS_SESSION_MAX - 1].recvBytes =
	    get_count(TS0710MUX_GPRS2_RECV_COUNT_IDX);
	gprsData[TS0710MUX_GPRS_SESSION_MAX - 1].sentBytes =
	    get_count(TS0710MUX_GPRS2_SEND_COUNT_IDX);

	error = copy_to_user(buf, gprsData, bufLen);
	if (error)
		return -1;
	return bufLen;
}

ssize_t file_proc_write(struct file * file, const char *buf, size_t count,
			loff_t * ppos)
{
	gprs_bytes gprsData[TS0710MUX_GPRS_SESSION_MAX];
	int bufLen = sizeof(gprs_bytes) * TS0710MUX_GPRS_SESSION_MAX;

	UNUSED_PARAM(file);
	UNUSED_PARAM(count);
	UNUSED_PARAM(ppos);

	memset(gprsData, 0, bufLen);

	if (copy_from_user(gprsData, buf, bufLen))
		return -1;

	set_count(TS0710MUX_GPRS1_RECV_COUNT_IDX, gprsData[0].recvBytes);
	set_count(TS0710MUX_GPRS1_SEND_COUNT_IDX, gprsData[0].sentBytes);
	set_count(TS0710MUX_GPRS2_RECV_COUNT_IDX,
		  gprsData[TS0710MUX_GPRS_SESSION_MAX - 1].recvBytes);
	set_count(TS0710MUX_GPRS2_SEND_COUNT_IDX,
		  gprsData[TS0710MUX_GPRS_SESSION_MAX - 1].sentBytes);

	return bufLen;
}

static void gprs_proc_init(void)
{
	gprs_proc_file =
	    create_proc_entry("gprsbytes", S_IRUSR | S_IWUSR, NULL);
	gprs_proc_file->proc_fops = &file_proc_operations;
}

static void gprs_proc_exit(void)
{
	remove_proc_entry("gprsbytes", gprs_proc_file);
}

int ts0710_rx_handler_buf(unsigned char *buf, ssize_t len)
{
	int retval;
	unsigned long flags;

	//local_irq_save(flags);
	retval = mux_ringbuffer_write(&rbuf, buf, len);
	//local_irq_restore(flags);

	if (1 == cmux_mode) {
		queue_work(mux_work_queue, &mux_receive_work);
	}
	return retval;
}


extern int omux_mux_vt_init(void);
extern int omux_mux_vt_exit(void);

static const struct tty_operations rs_ops = {
	.open = mux_open,
	.close = mux_close,
	.write = mux_write,
	.write_room = mux_write_room,
	.flush_buffer=mux_flush_buffer,
	.chars_in_buffer = mux_chars_in_buffer,
	.ioctl = mux_ioctl,
	.throttle = mux_throttle,
	.unthrottle = mux_unthrottle,
};


static int __init mux_init(void)
{
	unsigned int i,j;
	printk("\n SPRD MODEM MUX VERSION: %s\n", version_string);
	ts0710_init();

	mux_ringbuffer_init(&rbuf, mux_data, TS0710MUX_MAX_BUF_SIZE * 10);
	mux_ringbuffer_init(&stmbuf, stm_data, TS0710MUX_MAX_BUF_SIZE*10 );

	for (j = 0; j < NR_MUXS; j++) {
		mux_send_info_flags[j] = 0;
		mux_send_info[j] = 0;
		mux_recv_info_flags[j] = 0;
		mux_recv_info[j] = 0;
	}
	mux_send_info_idx = NR_MUXS;
	mux_recv_queue = NULL;
	mux_recv_flags = 0;

	for (j = 0; j < TS0710MUX_COUNT_IDX_NUM; j++) {
		mux_data_count[j] = 0;
		mux_data_count2[j] = 0;
		//INIT_MUTEX(&mux_data_count_mutex[j]);
		sema_init(&mux_data_count_mutex[j], 1);
	}
	post_recv_count_flag = 0;

	memset(&mux_driver, 0, sizeof(struct tty_driver));
	memset(&mux_tty, 0, sizeof(mux_tty));
	
	kref_init(&mux_driver.kref);
	mux_driver.magic = TTY_DRIVER_MAGIC;
    mux_driver.driver_name = "ts0710mux_drv";
	mux_driver.name = "ts0710mux";
	mux_driver.major = TS0710MUX_MAJOR;
	mux_driver.minor_start = TS0710MUX_MINOR_START;
	mux_driver.num = NR_MUXS;
	mux_driver.type = TTY_DRIVER_TYPE_SERIAL;
	mux_driver.subtype = SERIAL_TYPE_NORMAL;
	mux_driver.init_termios = tty_std_termios;
	mux_driver.init_termios.c_iflag = 0;
	mux_driver.init_termios.c_oflag = 0;
	mux_driver.init_termios.c_cflag = B38400 | CS8 | CREAD;
	mux_driver.init_termios.c_lflag = 0;
	mux_driver.flags = TTY_DRIVER_RESET_TERMIOS | TTY_DRIVER_REAL_RAW;
	mux_driver.other = NULL;
	
	tty_set_operations(&mux_driver, &rs_ops);
	
	if (tty_register_driver(&mux_driver)){
        printk("modem err:tty_register_driver fail!\n");
		return -1;
	};
		
	mux_table = mux_driver.ttys;
	COMM_MUX_DISPATCHER = mux_dispatcher;
	COMM_MUX_SENDER = mux_sender;
	//zhouzhigang
	//omux_mux_vt_init();
	gprs_proc_init();

    for(i=0;i<3;i++)
       {
        INIT_WORK(&(netdata_send_channel[i].mux_net_send_work), net_send_worker);
        init_q(i);
       }

	printk("<Du Wei> mux probe success\n");
    //zhouzhigang
	mux_work_queue = create_singlethread_workqueue("mux_work");
	return 0;
}

static void __exit mux_exit(void)
{
	int j;

	COMM_MUX_DISPATCHER = NULL;
	COMM_MUX_SENDER = NULL;
	//zhouzhigang
	//omux_mux_vt_exit();
	gprs_proc_exit();

	mux_send_info_idx = NR_MUXS;
	mux_recv_queue = NULL;
	for (j = 0; j < NR_MUXS; j++) {
		if ((mux_send_info_flags[j]) && (mux_send_info[j])) {
			kfree(mux_send_info[j]);
		}
		mux_send_info_flags[j] = 0;
		mux_send_info[j] = 0;

		if ((mux_recv_info_flags[j]) && (mux_recv_info[j])) {
			free_mux_recv_struct(mux_recv_info[j]);
		}
		mux_recv_info_flags[j] = 0;
		mux_recv_info[j] = 0;
	}
	
	destroy_workqueue(mux_work_queue);
	
	if (tty_unregister_driver(&mux_driver))
		panic("Couldn't unregister mux driver");
}

EXPORT_SYMBOL(ts0710_rx_handler_buf);
EXPORT_SYMBOL(is_cmux_mode);

module_init(mux_init);
module_exit(mux_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harald Welte <laforge@openezx.org>");
MODULE_DESCRIPTION("GSM TS 07.10 Multiplexer");
