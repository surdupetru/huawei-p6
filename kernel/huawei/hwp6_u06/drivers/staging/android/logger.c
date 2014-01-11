/*
 * drivers/misc/logger.c
 *
 * A Logging Subsystem
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * Robert Love <rlove@google.com>
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

#include <linux/sched.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/time.h>
#include "logger.h"

#include <asm/ioctls.h>

#ifdef CONFIG_K3_LOG
#include <mach/hardware.h>
#include <mach/hilog.h>
#include <linux/rtc.h>
#include <asm/io.h>
#include <mach/hisi_mem.h>
#endif

#include <linux/mtd/nve_interface.h>

typedef enum android_LogPriority {
	ANDROID_LOG_UNKNOWN = 0,
	ANDROID_LOG_DEFAULT,    /* only for SetMinPriority() */
	ANDROID_LOG_VERBOSE,
	ANDROID_LOG_DEBUG,
	ANDROID_LOG_INFO,
	ANDROID_LOG_WARN,
	ANDROID_LOG_ERROR,
	ANDROID_LOG_FATAL,
	ANDROID_LOG_SILENT,     /* only for SetMinPriority(); must be last */
} android_LogPriority;


#define ANDROID_LOG_LEVEL               (ANDROID_LOG_INFO)
//huangwen 2012-09-05 begin
static int powerlogoff = 1;
//huangwen 2012-09-05 end

#define MAX_TAG_LEN  100
static int minor_of_events = 0;
static int minor_of_main = 0;
static int minor_of_power = 0;
static int logctl_nv = 0;
#define RETRY_COUNT 3

#ifndef CONFIG_K3_LOG
/*
 * struct logger_log - represents a specific log, such as 'main' or 'radio'
 *
 * This structure lives from module insertion until module removal, so it does
 * not need additional reference counting. The structure is protected by the
 * mutex 'mutex'.
 */
struct logger_log {
	unsigned char 		*buffer;/* the ring buffer itself */
	struct miscdevice	misc;	/* misc device representing the log */
	wait_queue_head_t	wq;	/* wait queue for readers */
	struct list_head	readers; /* this log's readers */
	struct mutex		mutex;	/* mutex protecting buffer */
	size_t			w_off;	/* current write head offset */
	size_t			head;	/* new readers start here */
	size_t			size;	/* size of the log */
};
#endif
/*
 * struct logger_reader - a logging device open for reading
 *
 * This object lives from open to release, so we don't need additional
 * reference counting. The structure is protected by log->mutex.
 */
struct logger_reader {
	struct logger_log	*log;	/* associated log */
	struct list_head	list;	/* entry in logger_log's list */
	size_t			r_off;	/* current read head offset */
};

/* logger_offset - returns index 'n' into the log via (optimized) modulus */
#define logger_offset(n)	((n) & (log->size - 1))

#ifdef CONFIG_K3_LOG
extern struct semaphore k3log_sema;
extern volatile log_buffer_head *log_buf_info;
static char prefix_buf[1024];
static ssize_t do_write_rbuf(struct logger_log *log, const struct iovec *iov,
							unsigned long nr_segs, struct logger_entry head);
#endif

/*
 * file_get_log - Given a file structure, return the associated log
 *
 * This isn't aesthetic. We have several goals:
 *
 * 	1) Need to quickly obtain the associated log during an I/O operation
 * 	2) Readers need to maintain state (logger_reader)
 * 	3) Writers need to be very fast (open() should be a near no-op)
 *
 * In the reader case, we can trivially go file->logger_reader->logger_log.
 * For a writer, we don't want to maintain a logger_reader, so we just go
 * file->logger_log. Thus what file->private_data points at depends on whether
 * or not the file was opened for reading. This function hides that dirtiness.
 */
static inline struct logger_log *file_get_log(struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct logger_reader *reader = file->private_data;
		return reader->log;
	} else
		return file->private_data;
}

/*
 * get_entry_len - Grabs the length of the payload of the next entry starting
 * from 'off'.
 *
 * Caller needs to hold log->mutex.
 */
static __u32 get_entry_len(struct logger_log *log, size_t off)
{
	__u16 val;

	switch (log->size - off) {
	case 1:
		memcpy(&val, log->buffer + off, 1);
		memcpy(((char *) &val) + 1, log->buffer, 1);
		break;
	default:
		memcpy(&val, log->buffer + off, 2);
	}

	return sizeof(struct logger_entry) + val;
}

/*
 * do_read_log_to_user - reads exactly 'count' bytes from 'log' into the
 * user-space buffer 'buf'. Returns 'count' on success.
 *
 * Caller must hold log->mutex.
 */
static ssize_t do_read_log_to_user(struct logger_log *log,
				   struct logger_reader *reader,
				   char __user *buf,
				   size_t count)
{
	size_t len;

	/*
	 * We read from the log in two disjoint operations. First, we read from
	 * the current read head offset up to 'count' bytes or to the end of
	 * the log, whichever comes first.
	 */
	len = min(count, log->size - reader->r_off);
	if (copy_to_user(buf, log->buffer + reader->r_off, len))
		return -EFAULT;

	/*
	 * Second, we read any remaining bytes, starting back at the head of
	 * the log.
	 */
	if (count != len)
		if (copy_to_user(buf + len, log->buffer, count - len))
			return -EFAULT;

	reader->r_off = logger_offset(reader->r_off + count);

	return count;
}

/*
 * logger_read - our log's read() method
 *
 * Behavior:
 *
 * 	- O_NONBLOCK works
 * 	- If there are no log entries to read, blocks until log is written to
 * 	- Atomically reads exactly one log entry
 *
 * Optimal read size is LOGGER_ENTRY_MAX_LEN. Will set errno to EINVAL if read
 * buffer is insufficient to hold next entry.
 */
static ssize_t logger_read(struct file *file, char __user *buf,
			   size_t count, loff_t *pos)
{
	struct logger_reader *reader = file->private_data;
	struct logger_log *log = reader->log;
	ssize_t ret;
	DEFINE_WAIT(wait);

start:
	while (1) {
		mutex_lock(&log->mutex);

		prepare_to_wait(&log->wq, &wait, TASK_INTERRUPTIBLE);

		ret = (log->w_off == reader->r_off);
		mutex_unlock(&log->mutex);
		if (!ret)
			break;

		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			ret = -EINTR;
			break;
		}

		schedule();
	}

	finish_wait(&log->wq, &wait);
	if (ret)
		return ret;

	mutex_lock(&log->mutex);

	/* is there still something to read or did we race? */
	if (unlikely(log->w_off == reader->r_off)) {
		mutex_unlock(&log->mutex);
		goto start;
	}

	/* get the size of the next entry */
	ret = get_entry_len(log, reader->r_off);
	if (count < ret) {
		ret = -EINVAL;
		goto out;
	}

	/* get exactly one entry from the log */
	ret = do_read_log_to_user(log, reader, buf, ret);

out:
	mutex_unlock(&log->mutex);

	return ret;
}

/*
 * get_next_entry - return the offset of the first valid entry at least 'len'
 * bytes after 'off'.
 *
 * Caller must hold log->mutex.
 */
static size_t get_next_entry(struct logger_log *log, size_t off, size_t len)
{
	size_t count = 0;

	do {
		size_t nr = get_entry_len(log, off);
		off = logger_offset(off + nr);
		count += nr;
	} while (count < len);

	return off;
}

/*
 * clock_interval - is a < c < b in mod-space? Put another way, does the line
 * from a to b cross c?
 */
static inline int clock_interval(size_t a, size_t b, size_t c)
{
	if (b < a) {
		if (a < c || b >= c)
			return 1;
	} else {
		if (a < c && b >= c)
			return 1;
	}

	return 0;
}

/*
 * fix_up_readers - walk the list of all readers and "fix up" any who were
 * lapped by the writer; also do the same for the default "start head".
 * We do this by "pulling forward" the readers and start head to the first
 * entry after the new write head.
 *
 * The caller needs to hold log->mutex.
 */
static void fix_up_readers(struct logger_log *log, size_t len)
{
	size_t old = log->w_off;
	size_t new = logger_offset(old + len);
	struct logger_reader *reader;

	if (clock_interval(old, new, log->head))
		log->head = get_next_entry(log, log->head, len);

	list_for_each_entry(reader, &log->readers, list)
		if (clock_interval(old, new, reader->r_off))
			reader->r_off = get_next_entry(log, reader->r_off, len);
}

/*
 * do_write_log - writes 'len' bytes from 'buf' to 'log'
 *
 * The caller needs to hold log->mutex.
 */
static void do_write_log(struct logger_log *log, const void *buf, size_t count)
{
	size_t len;

	len = min(count, log->size - log->w_off);
	memcpy(log->buffer + log->w_off, buf, len);

	if (count != len)
		memcpy(log->buffer, buf + len, count - len);

	log->w_off = logger_offset(log->w_off + count);

}

/*
 * do_write_log_user - writes 'len' bytes from the user-space buffer 'buf' to
 * the log 'log'
 *
 * The caller needs to hold log->mutex.
 *
 * Returns 'count' on success, negative error code on failure.
 */
static ssize_t do_write_log_from_user(struct logger_log *log,
				      const void __user *buf, size_t count)
{
	size_t len;

	len = min(count, log->size - log->w_off);
	if (len && copy_from_user(log->buffer + log->w_off, buf, len))
		return -EFAULT;

	if (count != len)
		if (copy_from_user(log->buffer, buf + len, count - len))
			return -EFAULT;

	log->w_off = logger_offset(log->w_off + count);

	return count;
}



static void get_log_entry(const struct iovec *iov, unsigned long nr_segs, char *priority, char *tag, int taglen)
{
    int minlen = 0;
    int index = 0;

    if ((NULL == iov) || (NULL == priority) || (NULL == tag))
    {
        return;
    }

    while (nr_segs-- > 0)
    {
        if (NULL == iov->iov_base)
        {
            return;
        }

        if (0 == index)
        {
            if (1 != iov->iov_len)
            {
                *priority = 0;
            }
            else
            {
                if (copy_from_user(priority, iov->iov_base, iov->iov_len))
                {
                    printk("copy_from_user error, can't get the log priority\n");
                    return;
                }
            }
        }
        else if (1 == index)
        {
            minlen = iov->iov_len > taglen ? taglen : iov->iov_len;
            if (copy_from_user(tag, iov->iov_base, minlen))
            {
                printk("copy_from_user error, can't get the log tag\n");
                return;
            }
        }
        else
        {
            break;
        }

        index++;
        iov++;
    }

    return;
}

//read logctl state value from nv
static int read_logctl_state_from_nv()
{
	int ret;
	struct nve_info_user user_info;

	user_info.nv_operation = 1;
	user_info.nv_number = 301;
	user_info.valid_size = 1;
	strcpy(user_info.nv_name, "LOGCTL");
	if (ret = nve_direct_access(&user_info))
	{
		printk(KERN_ERR "nve_direct_access read error(%d)\n", ret);
		return -1;
	}

	ret = (int)user_info.nv_data[0];

	return ret;
}

/*
 * logger_aio_write - our write method, implementing support for write(),
 * writev(), and aio_write(). Writes are our fast path, and we try to optimize
 * them above all else.
 */
ssize_t logger_aio_write(struct kiocb *iocb, const struct iovec *iov,
			 unsigned long nr_segs, loff_t ppos)
{
	struct logger_log *log = file_get_log(iocb->ki_filp);
	size_t orig = log->w_off;
	struct logger_entry header;
	struct timespec now;
	ssize_t ret = 0;
        static int retry = 0;

        char priority = 0;
        char tag[MAX_TAG_LEN] = {0};
        get_log_entry(iov, nr_segs, &priority, tag, sizeof(tag));
        tag[sizeof(tag) - 1] = 0;

        if(retry < RETRY_COUNT)
        {
              logctl_nv = read_logctl_state_from_nv();
              printk("%s, logctl_nv=%d\n", __FUNCTION__, logctl_nv);
              if(logctl_nv >= 0)
              {
                   retry = RETRY_COUNT;
              }
              else
                   retry++;
        }

        /* if log device is events or main which its priority is more than ANDROID_LOG_INFO, we also pass it */
        if (logctl_nv == 1 || minor_of_events == log->misc.minor
            || ((minor_of_main == log->misc.minor) && (priority >= ANDROID_LOG_INFO))
            ||minor_of_power == log->misc.minor)
        {
            /* log it */
        }
        else
        {
            return -1;
        }

	now = current_kernel_time();

	header.pid = current->tgid;
	header.tid = current->pid;
	header.sec = now.tv_sec;
	header.nsec = now.tv_nsec;
	header.len = min_t(size_t, iocb->ki_left, LOGGER_ENTRY_MAX_PAYLOAD);

	/* null writes succeed, return zero */
	if (unlikely(!header.len))
		return 0;

	mutex_lock(&log->mutex);

	/*
	 * Fix up any readers, pulling them forward to the first readable
	 * entry after (what will be) the new write offset. We do this now
	 * because if we partially fail, we can end up with clobbered log
	 * entries that encroach on readable buffer.
	 */
	fix_up_readers(log, sizeof(struct logger_entry) + header.len);

#ifdef CONFIG_K3_LOG
	do_write_rbuf(log, iov, nr_segs, header);
#endif

	do_write_log(log, &header, sizeof(struct logger_entry));

	while (nr_segs-- > 0) {
		size_t len;
		ssize_t nr;

		/* figure out how much of this vector we can keep */
		len = min_t(size_t, iov->iov_len, header.len - ret);

		/* write out this segment's payload */
		nr = do_write_log_from_user(log, iov->iov_base, len);
		if (unlikely(nr < 0)) {
			log->w_off = orig;
			mutex_unlock(&log->mutex);
			return nr;
		}

		iov++;
		ret += nr;
	}

	mutex_unlock(&log->mutex);

	/* wake up any blocked readers */
	wake_up_interruptible(&log->wq);

	return ret;
}

static struct logger_log *get_log_from_minor(int);

/*
 * logger_open - the log's open() file operation
 *
 * Note how near a no-op this is in the write-only case. Keep it that way!
 */
static int logger_open(struct inode *inode, struct file *file)
{
	struct logger_log *log;
	int ret;

	ret = nonseekable_open(inode, file);
	if (ret)
		return ret;

	log = get_log_from_minor(MINOR(inode->i_rdev));
	if (!log)
		return -ENODEV;

	if (file->f_mode & FMODE_READ) {
		struct logger_reader *reader;

		reader = kmalloc(sizeof(struct logger_reader), GFP_KERNEL);
		if (!reader)
			return -ENOMEM;

		reader->log = log;
		INIT_LIST_HEAD(&reader->list);

		mutex_lock(&log->mutex);
		reader->r_off = log->head;
		list_add_tail(&reader->list, &log->readers);
		mutex_unlock(&log->mutex);

		file->private_data = reader;
	} else
		file->private_data = log;

	return 0;
}

/*
 * logger_release - the log's release file operation
 *
 * Note this is a total no-op in the write-only case. Keep it that way!
 */
static int logger_release(struct inode *ignored, struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct logger_reader *reader = file->private_data;
		struct logger_log *log = reader->log;

		mutex_lock(&log->mutex);
		list_del(&reader->list);
		mutex_unlock(&log->mutex);

		kfree(reader);
	}

	return 0;
}

/*
 * logger_poll - the log's poll file operation, for poll/select/epoll
 *
 * Note we always return POLLOUT, because you can always write() to the log.
 * Note also that, strictly speaking, a return value of POLLIN does not
 * guarantee that the log is readable without blocking, as there is a small
 * chance that the writer can lap the reader in the interim between poll()
 * returning and the read() request.
 */
static unsigned int logger_poll(struct file *file, poll_table *wait)
{
	struct logger_reader *reader;
	struct logger_log *log;
	unsigned int ret = POLLOUT | POLLWRNORM;

	if (!(file->f_mode & FMODE_READ))
		return ret;

	reader = file->private_data;
	log = reader->log;

	poll_wait(file, &log->wq, wait);

	mutex_lock(&log->mutex);
	if (log->w_off != reader->r_off)
		ret |= POLLIN | POLLRDNORM;
	mutex_unlock(&log->mutex);

	return ret;
}

static long logger_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct logger_log *log = file_get_log(file);
	struct logger_reader *reader;
	long ret = -ENOTTY;

	mutex_lock(&log->mutex);

	switch (cmd) {
	case LOGGER_GET_LOG_BUF_SIZE:
		ret = log->size;
		break;
	case LOGGER_GET_LOG_LEN:
		if (!(file->f_mode & FMODE_READ)) {
			ret = -EBADF;
			break;
		}
		reader = file->private_data;
		if (log->w_off >= reader->r_off)
			ret = log->w_off - reader->r_off;
		else
			ret = (log->size - reader->r_off) + log->w_off;
		break;
	case LOGGER_GET_NEXT_ENTRY_LEN:
		if (!(file->f_mode & FMODE_READ)) {
			ret = -EBADF;
			break;
		}
		reader = file->private_data;
		if (log->w_off != reader->r_off)
			ret = get_entry_len(log, reader->r_off);
		else
			ret = 0;
		break;
	case LOGGER_FLUSH_LOG:
		if (!(file->f_mode & FMODE_WRITE)) {
			ret = -EBADF;
			break;
		}
		list_for_each_entry(reader, &log->readers, list)
			reader->r_off = log->w_off;
		log->head = log->w_off;
		ret = 0;
		break;
	}

	mutex_unlock(&log->mutex);

	return ret;
}

static const struct file_operations logger_fops = {
	.owner = THIS_MODULE,
	.read = logger_read,
	.aio_write = logger_aio_write,
	.poll = logger_poll,
	.unlocked_ioctl = logger_ioctl,
	.compat_ioctl = logger_ioctl,
	.open = logger_open,
	.release = logger_release,
};

/*
 * Defines a log structure with name 'NAME' and a size of 'SIZE' bytes, which
 * must be a power of two, greater than LOGGER_ENTRY_MAX_LEN, and less than
 * LONG_MAX minus LOGGER_ENTRY_MAX_LEN.
 */
#define DEFINE_LOGGER_DEVICE(VAR, NAME, SIZE) \
static unsigned char _buf_ ## VAR[SIZE]; \
static struct logger_log VAR = { \
	.buffer = _buf_ ## VAR, \
	.misc = { \
		.minor = MISC_DYNAMIC_MINOR, \
		.name = NAME, \
		.fops = &logger_fops, \
		.parent = NULL, \
	}, \
	.wq = __WAIT_QUEUE_HEAD_INITIALIZER(VAR .wq), \
	.readers = LIST_HEAD_INIT(VAR .readers), \
	.mutex = __MUTEX_INITIALIZER(VAR .mutex), \
	.w_off = 0, \
	.head = 0, \
	.size = SIZE, \
};

#ifdef CONFIG_K3_LOG
DEFINE_LOGGER_DEVICE(log_main, LOGGER_LOG_MAIN, MAIN_LOG_BUF_LEN)
DEFINE_LOGGER_DEVICE(log_events, LOGGER_LOG_EVENTS, EVENTS_LOG_BUF_LEN)
DEFINE_LOGGER_DEVICE(log_radio, LOGGER_LOG_RADIO, RADIO_LOG_BUF_LEN)
DEFINE_LOGGER_DEVICE(log_system, LOGGER_LOG_SYSTEM, SYSTEM_LOG_BUF_LEN)
DEFINE_LOGGER_DEVICE(log_exception, LOGGER_LOG_EXCEPTION, EXCEPTION_LOG_BUF_LEN)
DEFINE_LOGGER_DEVICE(log_power, LOGGER_LOG_POWER, POWER_LOG_BUF_LEN)

EXPORT_SYMBOL(log_main);
EXPORT_SYMBOL(log_events);
EXPORT_SYMBOL(log_radio);
EXPORT_SYMBOL(log_system);
EXPORT_SYMBOL(log_exception);
EXPORT_SYMBOL(log_power);//begin:modified by dangjian
#else
DEFINE_LOGGER_DEVICE(log_main, LOGGER_LOG_MAIN, 256*1024)
DEFINE_LOGGER_DEVICE(log_events, LOGGER_LOG_EVENTS, 256*1024)
DEFINE_LOGGER_DEVICE(log_radio, LOGGER_LOG_RADIO, 256*1024)
DEFINE_LOGGER_DEVICE(log_system, LOGGER_LOG_SYSTEM, 256*1024)
DEFINE_LOGGER_DEVICE(log_exception, LOGGER_LOG_EXCEPTION, 16*1024)
DEFINE_LOGGER_DEVICE(log_power, LOGGER_LOG_POWER, 256*1024)
#endif

static struct logger_log *get_log_from_minor(int minor)
{
	if (log_main.misc.minor == minor)
		return &log_main;
	if (log_events.misc.minor == minor)
		return &log_events;
	if (log_radio.misc.minor == minor)
		return &log_radio;
	if (log_system.misc.minor == minor)
		return &log_system;
	if (log_exception.misc.minor == minor)
		return &log_exception;
		
    if (log_power.misc.minor == minor)
        return &log_power;

	return NULL;
}


#define LOG_CTL_ON 1
#define LOG_CTL_OFF 0
extern unsigned int get_logctl_value(void);


static int __init init_log(struct logger_log *log)
{
	int ret;

	ret = misc_register(&log->misc);
	if (unlikely(ret)) {
		printk(KERN_ERR "logger: failed to register misc "
		       "device for log '%s'!\n", log->misc.name);
		return ret;
	}

	printk(KERN_INFO "logger: created %luK log '%s'\n",
	       (unsigned long) log->size >> 10, log->misc.name);

	return 0;
}

static int __init logger_init(void)
{
	int ret;

	ret = init_log(&log_main);
	if (unlikely(ret))
		goto out;

	ret = init_log(&log_events);
	if (unlikely(ret))
		goto out;

	ret = init_log(&log_radio);
	if (unlikely(ret))
		goto out;

	ret = init_log(&log_system);
	if (unlikely(ret))
		goto out;

    ret = init_log(&log_exception);
    if (unlikely(ret))
        goto out;

    minor_of_events = log_events.misc.minor;
    minor_of_main = log_main.misc.minor;
    printk("%s, minor_of_events=%d\n", __FUNCTION__, minor_of_events);
    printk("%s, minor_of_main=%d\n", __FUNCTION__, minor_of_main);

#ifdef CONFIG_K3_LOG
	log_main.log_buf_info = (volatile log_buffer_head *)log_buf_info + 1;
	log_main.rbuf = (volatile unsigned char *)ioremap_nocache(K3_MAIN_LOG_BASE, MAIN_LOG_BUF_LEN);
	log_events.log_buf_info = (volatile log_buffer_head *)log_buf_info + 2;
	log_events.rbuf = (volatile unsigned char *)ioremap_nocache(K3_EVENTS_LOG_BASE, EVENTS_LOG_BUF_LEN);
	log_radio.log_buf_info = (volatile log_buffer_head *)log_buf_info + 3;
	log_radio.rbuf = (volatile unsigned char *)ioremap_nocache(K3_RADIO_LOG_BASE, RADIO_LOG_BUF_LEN);
	log_system.log_buf_info = (volatile log_buffer_head *)log_buf_info + 4;
	log_system.rbuf = (volatile unsigned char *)ioremap_nocache(K3_SYSTEM_LOG_BASE, SYSTEM_LOG_BUF_LEN);
	log_exception.log_buf_info = (volatile log_buffer_head *)log_buf_info + 5;
	log_exception.rbuf = (volatile unsigned char *)ioremap_nocache(K3_EXCEPTION_LOG_BASE, EXCEPTION_LOG_BUF_LEN);

	if (!log_main.log_buf_info || !log_main.rbuf || !log_events.log_buf_info
		|| !log_events.rbuf || !log_radio.log_buf_info || !log_radio.rbuf
		|| !log_system.log_buf_info || !log_system.rbuf
		|| !log_exception.log_buf_info || !log_exception.rbuf ) {
		/* init error, the system may be crashed, no need to unmap. */
		ret = -EFAULT;
	}
#endif

out:
    //huangwen 2012-09-05 begin
    powerlogoff = 0;
    ret = init_log(&log_power);
    minor_of_power = log_power.misc.minor;
    printk("%s, minor_of_power=%d\n", __FUNCTION__, minor_of_power);
    powerlogoff = 1;
    if (unlikely(ret)) {
        //do nothing
    } else {
#ifdef CONFIG_K3_LOG
        log_power.log_buf_info = (volatile log_buffer_head *)log_buf_info + 6;
        log_power.rbuf = (volatile unsigned char *)ioremap_nocache(K3_POWER_LOG_BASE, POWER_LOG_BUF_LEN);
        if (!log_power.log_buf_info || !log_power.rbuf) {
            /* init error, the system may be crashed, no need to unmap. */
            ret = -EFAULT;
        }
#endif
    }
    //huangwen 2012-09-05 end
    return ret;
}
device_initcall(logger_init);

#ifdef CONFIG_K3_LOG
static char filterPriToChar (android_LogPriority pri)
{
	switch (pri) {
	case ANDROID_LOG_VERBOSE:		return 'V';
	case ANDROID_LOG_DEBUG:			return 'D';
	case ANDROID_LOG_INFO:			return 'I';
	case ANDROID_LOG_WARN:			return 'W';
	case ANDROID_LOG_ERROR:			return 'E';
	case ANDROID_LOG_FATAL:			return 'F';
	case ANDROID_LOG_SILENT:		return 'S';

	case ANDROID_LOG_DEFAULT:
	case ANDROID_LOG_UNKNOWN:
	default:						return '?';
	}
}

static int add_prefix_to_rbuf(struct logger_log *log, struct iovec **iov,
							unsigned long nr_segs, struct logger_entry head)
{
	char prichar;
	unsigned long prefix_len;
	size_t len;
	size_t ret_len = 0;
	struct rtc_time cur_tm;
	int proc = 0;

	if (log == &log_events) {
		return ret_len;
	} else {
		len = min_t(size_t, (*iov)->iov_len, sizeof(int));
		if(len && copy_from_user((void *)&proc, (void *)(*iov)->iov_base, len)){
			return -EFAULT;
		}else {
			if (((proc < ANDROID_LOG_LEVEL) && (log != &log_radio))
				|| (proc == ANDROID_LOG_SILENT)
				|| ((proc < ANDROID_LOG_DEBUG) && (log == &log_radio)))
				return -EFAULT;
		}

		/* figure out how much of this vector we can keep */
		prichar = filterPriToChar(proc);

		rtc_time_to_tm(head.sec + 3600*8, &cur_tm);

		prefix_len = (unsigned int)snprintf(prefix_buf, sizeof(prefix_buf),
						"%d-%d-%d %2d:%2d:%2d.%03d %c/",
						cur_tm.tm_year+1900, cur_tm.tm_mon+1, cur_tm.tm_mday,
						cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec,
						head.nsec/1000000, prichar);

		(*iov)++;

		if(nr_segs > 2){
			//ret_len = min_t(size_t, (*iov)->iov_len-1, head.len-1);
			ret_len = min_t(size_t, (*iov)->iov_len, head.len);

			len = min_t(size_t, (*iov)->iov_len-1, sizeof(prefix_buf) - prefix_len);
			if(len && copy_from_user((void *)(prefix_buf + prefix_len), (void *)(*iov)->iov_base, len)){
				return -EFAULT;
			}

		prefix_len += len;

		len = (unsigned int)snprintf(prefix_buf + prefix_len, sizeof(prefix_buf)-prefix_len,
								"(%5d): ", head.pid);

			prefix_len += len;
			(*iov)++;
		}

		len = (unsigned int)snprintf(prefix_buf + prefix_len, sizeof(prefix_buf)-prefix_len,
									"(%5d): ", head.pid);

		prefix_len += len;

		//ret_len += 1;
	}

	len = min(prefix_len, log->size - log->log_buf_info->waddr);
	memcpy((void *)log->rbuf + log->log_buf_info->waddr, prefix_buf, len);

	if (prefix_len != len)
		memcpy((void *)log->rbuf, prefix_buf + len, prefix_len - len);

	log->log_buf_info->waddr = logger_offset(log->log_buf_info->waddr + prefix_len);

	return ret_len;
}

static ssize_t do_write_rbuf_from_user(struct logger_log *log,
					const void __user *buf, size_t count, unsigned long nr_segs)
{
	size_t len;
	size_t free_len;

	free_len = log->size - log->log_buf_info->waddr;
	len = min(count, free_len);
	if (len && copy_from_user((void *)log->rbuf + log->log_buf_info->waddr, buf, len))
		return -EFAULT;

	if (count != len)
		if (copy_from_user((void *)log->rbuf, buf + len, count - len))
			return -EFAULT;

	log->log_buf_info->waddr = logger_offset(log->log_buf_info->waddr + count);

	return count;
}

static void add_suffix_to_rbuf(struct logger_log *log)
{
	if (log->log_buf_info->waddr == 0)
		return;

	if (log->rbuf[log->log_buf_info->waddr-1] != 0x0A) {
		log->rbuf[log->log_buf_info->waddr] = 0x0A;
		log->log_buf_info->waddr = logger_offset(log->log_buf_info->waddr + 1);
	}
}

static ssize_t do_write_rbuf(struct logger_log *log, const struct iovec *iov,
								unsigned long nr_segs, struct logger_entry head)
{
	struct iovec * tmp_iov = (struct iovec *)iov;
	size_t orig_rbuf = 0;
	ssize_t nr;
	size_t len;
	size_t prefix_len;

	if (log == &log_events)
		return -EFAULT;

	orig_rbuf = log->log_buf_info->waddr;

	if (log->log_buf_info->waddr < log->log_buf_info->raddr) {
		if ((log->log_buf_info->waddr + log->size - log->log_buf_info->raddr) >= (log->size >> 1))
			up(&k3log_sema);
	} else {
		if ((log->log_buf_info->waddr - log->log_buf_info->raddr) >= (log->size >> 1))
			up(&k3log_sema);
	}

	prefix_len = add_prefix_to_rbuf(log, &tmp_iov, nr_segs, head);

	if (prefix_len < 0) {
		log->log_buf_info->waddr = orig_rbuf;
		return -EFAULT;
	}

	if(nr_segs > 1){
		len = min_t(size_t, tmp_iov->iov_len-1, head.len-prefix_len);

		nr = do_write_rbuf_from_user(log, tmp_iov->iov_base, len, nr_segs);

		if (unlikely(nr < 0)) {
			log->log_buf_info->waddr = orig_rbuf;
			return nr;
		}
	}

	add_suffix_to_rbuf(log);

	return 0;
}
#endif

