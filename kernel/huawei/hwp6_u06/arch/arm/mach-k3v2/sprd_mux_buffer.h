/*
 *
 * mux_ringbuffer.h: ring buffer implementation for the mux driver
 *
 *
 */

#ifndef _MUX_RINGBUFFER_H_
#define _MUX_RINGBUFFER_H_

#include <linux/spinlock.h>
#include <linux/wait.h>

struct mux_ringbuffer {
	u8 *data;
	ssize_t size;
	ssize_t pread;
	ssize_t pwrite;
	int error;

	wait_queue_head_t queue;
	spinlock_t lock;
};

#define MUX_RINGBUFFER_PKTHDRSIZE 3

/*
** Notes:
** ------
** (1) For performance reasons read and write routines don't check buffer sizes
**     and/or number of bytes free/available. This has to be done before these
**     routines are called. For example:
**
**     *** write <buflen> bytes ***
**     free = mux_ringbuffer_free(rbuf);
**     if (free >= buflen)
**         count = mux_ringbuffer_write(rbuf, buffer, buflen);
**     else
**         ...
**
**     *** read min. 1000, max. <bufsize> bytes ***
**     avail = mux_ringbuffer_avail(rbuf);
**     if (avail >= 1000)
**         count = mux_ringbuffer_read(rbuf, buffer, min(avail, bufsize), 0);
**     else
**         ...
**
** (2) If there is exactly one reader and one writer, there is no need
**     to lock read or write operations.
**     Two or more readers must be locked against each other.
**     Flushing the buffer counts as a read operation.
**     Two or more writers must be locked against each other.
*/

/* initialize ring buffer, lock and queue */
extern void mux_ringbuffer_init(struct mux_ringbuffer *rbuf, void *data,
				size_t len);

/* test whether buffer is empty */
extern int mux_ringbuffer_empty(struct mux_ringbuffer *rbuf);

/* return the number of free bytes in the buffer */
extern ssize_t mux_ringbuffer_free(struct mux_ringbuffer *rbuf);

/* return the number of bytes waiting in the buffer */
extern ssize_t mux_ringbuffer_avail(struct mux_ringbuffer *rbuf);

/* read routines & macros */
/* ---------------------- */
/* flush buffer */
extern void mux_ringbuffer_flush(struct mux_ringbuffer *rbuf);

/* flush buffer protected by spinlock and wake-up waiting task(s) */
extern void mux_ringbuffer_flush_spinlock_wakeup(struct mux_ringbuffer *rbuf);

/* peek at byte <offs> in the buffer */
#define MUX_RINGBUFFER_PEEK(rbuf,offs)	\
			(rbuf)->data[((rbuf)->pread+(offs))%(rbuf)->size]

/* advance read ptr by <num> bytes */
#define MUX_RINGBUFFER_SKIP(rbuf,num)	\
			(rbuf)->pread=((rbuf)->pread+(num))%(rbuf)->size

/*
** read <len> bytes from ring buffer into <buf>
** <usermem> specifies whether <buf> resides in user space
** returns number of bytes transferred or -EFAULT
*/
extern ssize_t mux_ringbuffer_read(struct mux_ringbuffer *rbuf, u8 * buf,
				   size_t len, int usermem);

/* write routines & macros */
/* ----------------------- */
/* write single byte to ring buffer */
#define MUX_RINGBUFFER_WRITE_BYTE(rbuf,byte)	\
			{ (rbuf)->data[(rbuf)->pwrite]=(byte); \
			(rbuf)->pwrite=((rbuf)->pwrite+1)%(rbuf)->size; }
/*
** write <len> bytes to ring buffer
** <usermem> specifies whether <buf> resides in user space
** returns number of bytes transferred or -EFAULT
*/
extern ssize_t mux_ringbuffer_write(struct mux_ringbuffer *rbuf, const u8 * buf,
				    size_t len);

/**
 * Write a packet into the ringbuffer.
 *
 * <rbuf> Ringbuffer to write to.
 * <buf> Buffer to write.
 * <len> Length of buffer (currently limited to 65535 bytes max).
 * returns Number of bytes written, or -EFAULT, -ENOMEM, -EVINAL.
 */
extern ssize_t mux_ringbuffer_pkt_write(struct mux_ringbuffer *rbuf, u8 * buf,
					size_t len);

/**
 * Read from a packet in the ringbuffer. Note: unlike mux_ringbuffer_read(), this
 * does NOT update the read pointer in the ringbuffer. You must use
 * mux_ringbuffer_pkt_dispose() to mark a packet as no longer required.
 *
 * <rbuf> Ringbuffer concerned.
 * <idx> Packet index as returned by mux_ringbuffer_pkt_next().
 * <offset> Offset into packet to read from.
 * <buf> Destination buffer for data.
 * <len> Size of destination buffer.
 * <usermem> Set to 1 if <buf> is in userspace.
 * returns Number of bytes read, or -EFAULT.
 */
extern ssize_t mux_ringbuffer_pkt_read(struct mux_ringbuffer *rbuf, size_t idx,
				       int offset, u8 * buf, size_t len,
				       int usermem);

/**
 * Dispose of a packet in the ring buffer.
 *
 * <rbuf> Ring buffer concerned.
 * <idx> Packet index as returned by mux_ringbuffer_pkt_next().
 */
extern void mux_ringbuffer_pkt_dispose(struct mux_ringbuffer *rbuf, size_t idx);

/**
 * Get the index of the next packet in a ringbuffer.
 *
 * <rbuf> Ringbuffer concerned.
 * <idx> Previous packet index, or -1 to return the first packet index.
 * <pktlen> On success, will be updated to contain the length of the packet in bytes.
 * returns Packet index (if >=0), or -1 if no packets available.
 */
extern ssize_t mux_ringbuffer_pkt_next(struct mux_ringbuffer *rbuf, size_t idx,
				       size_t * pktlen);

#endif /* _MUX_RINGBUFFER_H_ */
