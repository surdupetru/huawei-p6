/*
 *
 * mux_ringbuffer.c: ring buffer implementation for the mux driver
 *
 * 
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <asm/uaccess.h>

#include "sprd_mux_buffer.h"

#define PKT_READY 0
#define PKT_DISPOSED 1

void mux_ringbuffer_init(struct mux_ringbuffer *rbuf, void *data, size_t len)
{
	rbuf->pread = rbuf->pwrite = 0;
	rbuf->data = data;
	rbuf->size = len;
	rbuf->error = 0;

	init_waitqueue_head(&rbuf->queue);

	spin_lock_init(&(rbuf->lock));
}

int mux_ringbuffer_empty(struct mux_ringbuffer *rbuf)
{
	return (rbuf->pread == rbuf->pwrite);
}

ssize_t mux_ringbuffer_free(struct mux_ringbuffer * rbuf)
{
	ssize_t free;

	free = rbuf->pread - rbuf->pwrite;
	if (free <= 0)
		free += rbuf->size;
	return free - 1;
}

ssize_t mux_ringbuffer_avail(struct mux_ringbuffer * rbuf)
{
	ssize_t avail;

	avail = rbuf->pwrite - rbuf->pread;
	if (avail < 0)
		avail += rbuf->size;
	return avail;
}

void mux_ringbuffer_flush(struct mux_ringbuffer *rbuf)
{
	rbuf->pread = rbuf->pwrite;
	rbuf->error = 0;
}

void mux_ringbuffer_flush_spinlock_wakeup(struct mux_ringbuffer *rbuf)
{
	unsigned long flags;

	spin_lock_irqsave(&rbuf->lock, flags);
	mux_ringbuffer_flush(rbuf);
	spin_unlock_irqrestore(&rbuf->lock, flags);

	wake_up(&rbuf->queue);
}

ssize_t mux_ringbuffer_read(struct mux_ringbuffer *rbuf, u8 * buf, size_t len,
			    int usermem)
{
	size_t todo = len;
	size_t split;

    spin_lock(&rbuf->lock);//added by zhouzhigang
	split = (rbuf->pread + len > rbuf->size) ? rbuf->size - rbuf->pread : 0;
	if (split > 0) {
		if (!usermem)
			memcpy(buf, rbuf->data + rbuf->pread, split);
		else if (copy_to_user(buf, rbuf->data + rbuf->pread, split))
			return -EFAULT;
		buf += split;
		todo -= split;
		rbuf->pread = 0;
	}
	if (!usermem)
		memcpy(buf, rbuf->data + rbuf->pread, todo);
	else if (copy_to_user(buf, rbuf->data + rbuf->pread, todo))
		return -EFAULT;

	rbuf->pread = (rbuf->pread + todo) % rbuf->size;
    spin_unlock(&rbuf->lock);//added by zhouzhigang
	return len;
}

ssize_t mux_ringbuffer_write(struct mux_ringbuffer * rbuf, const u8 * buf,
			     size_t len)
{
	size_t todo = len;
	size_t split;

    spin_lock(&rbuf->lock);//added by zhouzhigang
	split =
	    (rbuf->pwrite + len > rbuf->size) ? rbuf->size - rbuf->pwrite : 0;

	if (split > 0) {

        //added by zhouzhigang
		if((todo-split) > rbuf->pread)
		{
		   goto rbuf_overflow;
		}		
		memcpy(rbuf->data + rbuf->pwrite, buf, split);
		buf += split;
		todo -= split;
		rbuf->pwrite = 0;
	}
	else if((rbuf->pread > rbuf->pwrite)&&((rbuf->pwrite+len) > rbuf->pread))
	{
	   goto rbuf_overflow;
	}
	memcpy(rbuf->data + rbuf->pwrite, buf, todo);
	rbuf->pwrite = (rbuf->pwrite + todo) % rbuf->size;
    spin_unlock(&rbuf->lock);//added by zhouzhigang
	return len;

rbuf_overflow:
	spin_unlock(&rbuf->lock);//added by zhouzhigang
	printk("!!!!!!!rbuf overflow:%d,%d,%d\n",rbuf->pwrite, rbuf->pread, len);
	return 0;
}

ssize_t mux_ringbuffer_pkt_write(struct mux_ringbuffer * rbuf, u8 * buf,
				 size_t len)
{
	int status;
	ssize_t oldpwrite = rbuf->pwrite;

	MUX_RINGBUFFER_WRITE_BYTE(rbuf, len >> 8);
	MUX_RINGBUFFER_WRITE_BYTE(rbuf, len & 0xff);
	MUX_RINGBUFFER_WRITE_BYTE(rbuf, PKT_READY);
	status = mux_ringbuffer_write(rbuf, buf, len);

	if (status < 0)
		rbuf->pwrite = oldpwrite;
	return status;
}

ssize_t mux_ringbuffer_pkt_read(struct mux_ringbuffer * rbuf, size_t idx,
				int offset, u8 * buf, size_t len, int usermem)
{
	size_t todo;
	size_t split;
	size_t pktlen;

	pktlen = rbuf->data[idx] << 8;
	pktlen |= rbuf->data[(idx + 1) % rbuf->size];
	if (offset > pktlen)
		return -EINVAL;
	if ((offset + len) > pktlen)
		len = pktlen - offset;

	idx = (idx + MUX_RINGBUFFER_PKTHDRSIZE + offset) % rbuf->size;
	todo = len;
	split = ((idx + len) > rbuf->size) ? rbuf->size - idx : 0;
	if (split > 0) {
		if (!usermem)
			memcpy(buf, rbuf->data + idx, split);
		else if (copy_to_user(buf, rbuf->data + idx, split))
			return -EFAULT;
		buf += split;
		todo -= split;
		idx = 0;
	}
	if (!usermem)
		memcpy(buf, rbuf->data + idx, todo);
	else if (copy_to_user(buf, rbuf->data + idx, todo))
		return -EFAULT;

	return len;
}

void mux_ringbuffer_pkt_dispose(struct mux_ringbuffer *rbuf, size_t idx)
{
	size_t pktlen;

	rbuf->data[(idx + 2) % rbuf->size] = PKT_DISPOSED;

	// clean up disposed packets
	while (mux_ringbuffer_avail(rbuf) > MUX_RINGBUFFER_PKTHDRSIZE) {
		if (MUX_RINGBUFFER_PEEK(rbuf, 2) == PKT_DISPOSED) {
			pktlen = MUX_RINGBUFFER_PEEK(rbuf, 0) << 8;
			pktlen |= MUX_RINGBUFFER_PEEK(rbuf, 1);
			MUX_RINGBUFFER_SKIP(rbuf,
					    pktlen + MUX_RINGBUFFER_PKTHDRSIZE);
		} else {
			// first packet is not disposed, so we stop cleaning now
			break;
		}
	}
}

ssize_t mux_ringbuffer_pkt_next(struct mux_ringbuffer *rbuf, size_t idx,
				size_t * pktlen)
{
	int consumed;
	int curpktlen;
	int curpktstatus;

	if (idx == -1) {
		idx = rbuf->pread;
	} else {
		curpktlen = rbuf->data[idx] << 8;
		curpktlen |= rbuf->data[(idx + 1) % rbuf->size];
		idx =
		    (idx + curpktlen + MUX_RINGBUFFER_PKTHDRSIZE) % rbuf->size;
	}

	consumed = (idx - rbuf->pread) % rbuf->size;

	while ((mux_ringbuffer_avail(rbuf) - consumed) >
	       MUX_RINGBUFFER_PKTHDRSIZE) {

		curpktlen = rbuf->data[idx] << 8;
		curpktlen |= rbuf->data[(idx + 1) % rbuf->size];
		curpktstatus = rbuf->data[(idx + 2) % rbuf->size];

		if (curpktstatus == PKT_READY) {
			*pktlen = curpktlen;
			return idx;
		}

		consumed += curpktlen + MUX_RINGBUFFER_PKTHDRSIZE;
		idx =
		    (idx + curpktlen + MUX_RINGBUFFER_PKTHDRSIZE) % rbuf->size;
	}

	// no packets available
	return -1;
}

EXPORT_SYMBOL(mux_ringbuffer_init);
EXPORT_SYMBOL(mux_ringbuffer_empty);
EXPORT_SYMBOL(mux_ringbuffer_free);
EXPORT_SYMBOL(mux_ringbuffer_avail);
EXPORT_SYMBOL(mux_ringbuffer_flush_spinlock_wakeup);
EXPORT_SYMBOL(mux_ringbuffer_read);
EXPORT_SYMBOL(mux_ringbuffer_write);
