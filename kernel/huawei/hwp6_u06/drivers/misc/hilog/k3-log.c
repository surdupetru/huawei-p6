/*
 * drivers/misc/hilog/k3log.c
 *
 * A Logging Subsystem
 *
 * Copyright (C) 2010- Hisilicon, Inc.
 *
 *
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
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/kernel.h>
#include <linux/suspend.h>
#include <linux/earlysuspend.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/rtc.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/early-debug.h>
#include <mach/hilog.h>
#include <linux/syscalls.h>
#include <mach/hisi_mem.h>

typedef struct {
		log_buffer_head *log_buf_info;
		unsigned char *log_buf_addr;
		unsigned long log_buf_size;
		unsigned long log_file_size;
		char *log_file_name;
} log_info_t;

volatile log_info_t log_info[LOG_MAX];
DEFINE_MUTEX(k3log_file_writer);

struct task_struct *kdumplog_task;

int hilog_loaded ; /*used to indicate printk can ioremap*/
EXPORT_SYMBOL(hilog_loaded);

#ifdef CONFIG_WAKELOCK
struct early_suspend hilog_early_suspend;
#endif

#define TMP_BUF_LEN                         (64)

extern volatile log_buffer_head *log_buf_info;
extern volatile unsigned char *res_log_buf;
extern struct semaphore k3log_sema;

#define HI_DECLARE_SEMAPHORE(name) \
	struct semaphore name = __SEMAPHORE_INITIALIZER(name, 1)

HI_DECLARE_SEMAPHORE(k3log_sys_sema);
EXPORT_SYMBOL(k3log_sys_sema);

static void flush_all_logs(void);
static int create_dumpfile(char *name, unsigned long file_size);

static void set_log_info(unsigned char index, volatile log_buffer_head *log_buf_info,
						void *log_buf_addr, unsigned long  log_buf_size,
						unsigned long log_file_size, char *log_file_name)
{
	log_info[index].log_buf_info = (log_buffer_head *)log_buf_info;
	log_info[index].log_buf_addr = (unsigned char *)log_buf_addr;
	log_info[index].log_buf_size = log_buf_size;
	log_info[index].log_file_size = log_file_size;
	log_info[index].log_file_name = log_file_name;
}

static int hisik3_log_probe(struct platform_device *pdev)
{
	pr_info("k3 log probed\n");

	return 0;
}

static int hisik3_log_remove(struct platform_device *pdev)
{
	pr_info("k3 log remove\n");
	flush_all_logs();
	return 0;
}

static int hisik3_log_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int hisik3_log_resume(struct platform_device *pdev)
{
	return 0;
}

static void hisik3_log_shutdown(struct platform_device *pdev)
{
	pr_info("k3 log shutdown\n");
	flush_all_logs();
	sys_sync();
	return;
}

static void log_lock(log_type type_id, unsigned long *flags)
{
	mutex_lock(&k3log_file_writer);
}

static void log_unlock(log_type type_id, unsigned long *flags)
{
	mutex_unlock(&k3log_file_writer);
}

static void dump_log(log_type type_id)
{
	mm_segment_t fs;
	unsigned long flags;
	unsigned long buf_write_len;
	unsigned long file_pointer;
	struct kstat file_stat;
	struct file *fp;
	volatile log_info_t *tmp_log_info = &log_info[type_id];
	unsigned long tmp_len;
	unsigned char *buf_read_addr;
	unsigned char *buf_write_addr;
	struct rtc_time cur_tm;
	char time_buf[64];
	unsigned long timestamp_len;
	int ret;
	int file_flag = O_RDWR;
	struct timespec now;

	fs = get_fs();
	set_fs(KERNEL_DS);

	ret = vfs_stat(tmp_log_info->log_file_name, &file_stat);

	if (ret < 0)
		file_flag |= O_CREAT;

	log_lock(type_id, &flags);

	buf_read_addr = (unsigned char *)tmp_log_info->log_buf_addr
		+ tmp_log_info->log_buf_info->raddr;

	buf_write_addr = (unsigned char *)tmp_log_info->log_buf_addr
		+ tmp_log_info->log_buf_info->waddr;

	if (buf_read_addr == buf_write_addr) {
		log_unlock(type_id, &flags);
		return;
	}

	fp = filp_open(tmp_log_info->log_file_name, file_flag, 0666);

	if (IS_ERR(fp)) {
		//pr_err("open log file error\n");
		log_unlock(type_id, &flags);
		return;
	}

	vfs_stat(tmp_log_info->log_file_name, &file_stat);

	if (file_stat.size == 0)
		file_pointer = sizeof(file_pointer);
	else {
		vfs_llseek(fp, 0, SEEK_SET);
		/*get the file_pointer at the head of file 4 byte*/
		if (vfs_read(fp, (char *)&file_pointer, sizeof(file_pointer), &fp->f_pos) < 0)
			file_pointer = sizeof(file_pointer);
	}

	buf_write_len = (buf_write_addr > buf_read_addr) ? (buf_write_addr-buf_read_addr)
					: (tmp_log_info->log_buf_size - (buf_read_addr-buf_write_addr));

	vfs_llseek(fp, file_pointer, SEEK_SET);

	now = current_kernel_time();
	rtc_time_to_tm(now.tv_sec, &cur_tm);
	timestamp_len = sprintf(time_buf, "%d-%d-%d %2d:%2d:%2d\n",
							cur_tm.tm_year+1900, cur_tm.tm_mon+1,
							cur_tm.tm_mday, cur_tm.tm_hour,
							cur_tm.tm_min, cur_tm.tm_sec);

	tmp_len = min(timestamp_len, tmp_log_info->log_file_size-file_pointer);

	vfs_write(fp, time_buf, tmp_len, &fp->f_pos);

	if (tmp_len != timestamp_len) {
		vfs_llseek(fp, sizeof(file_pointer), SEEK_SET);
		vfs_write(fp, time_buf+tmp_len, timestamp_len-tmp_len, &fp->f_pos);
	}

	file_pointer = ((file_pointer + timestamp_len) >= tmp_log_info->log_file_size) ?
					(file_pointer + timestamp_len + sizeof(file_pointer))
					% (tmp_log_info->log_file_size) : (file_pointer + timestamp_len);

	vfs_llseek(fp, file_pointer, SEEK_SET);

	if (buf_read_addr < buf_write_addr) {
		tmp_len = min(buf_write_len, tmp_log_info->log_file_size-file_pointer);

		vfs_write(fp, buf_read_addr, tmp_len, &fp->f_pos);

		if (tmp_len != buf_write_len) {
			vfs_llseek(fp, sizeof(file_pointer), SEEK_SET);
			vfs_write(fp, buf_read_addr+tmp_len, buf_write_len-tmp_len, &fp->f_pos);
		}
	} else {
		unsigned long a = tmp_log_info->log_buf_size - tmp_log_info->log_buf_info->raddr;
		unsigned long b = tmp_log_info->log_buf_info->waddr;
		unsigned long c = tmp_log_info->log_file_size - file_pointer;

		tmp_len = min(a, c);
		vfs_write(fp, buf_read_addr, tmp_len, &fp->f_pos);

		if (a < c) {
			tmp_len = min(b, c-a);
			vfs_write(fp, (const char *)tmp_log_info->log_buf_addr, tmp_len, &fp->f_pos);

			if (b > (c-a)) {
				tmp_len = b-(c-a);
				vfs_llseek(fp, sizeof(file_pointer), SEEK_SET);
				vfs_write(fp, (const char *)tmp_log_info->log_buf_addr+c-a, tmp_len, &fp->f_pos);
			}
		} else {
			vfs_llseek(fp, sizeof(file_pointer), SEEK_SET);
			vfs_write(fp, (const char *)tmp_log_info->log_buf_addr+tmp_len, a-c, &fp->f_pos);
			vfs_write(fp, (const char *)tmp_log_info->log_buf_addr, b, &fp->f_pos);
		}
	}

	file_pointer = ((file_pointer + buf_write_len) >= tmp_log_info->log_file_size) ?
					(file_pointer + buf_write_len + sizeof(file_pointer))
					% (tmp_log_info->log_file_size) : (file_pointer + buf_write_len);

	/*save the file_pointer at the head of file 4 byte*/
	vfs_llseek(fp, 0, SEEK_SET);
	vfs_write(fp, (char *)&file_pointer, sizeof(file_pointer), &fp->f_pos);

	set_fs(fs);

	if (fp)
		filp_close(fp, 0);

	tmp_log_info->log_buf_info->raddr = (tmp_log_info->log_buf_info->raddr + buf_write_len)
										% tmp_log_info->log_buf_size;

	log_unlock(type_id, &flags);
}

static void flush_all_logs(void)
{
	int i;

	for (i = 0; i < LOG_MAX; i++) {
		if (i == LOG_EVENTS)
			continue;

		dump_log(i);
	}
}

static void dump_all_logs(void)
{
	int i = 0;
	unsigned long flags = 0;
	unsigned long buf_reader = 0;
	unsigned long buf_writer = 0;
	unsigned long buf_write_len = 0;

	for (i = 0; i < LOG_MAX; i++) {
		if (i == LOG_EVENTS)
			continue;

		log_lock(i, &flags);
		buf_reader = log_info[i].log_buf_info->raddr;
		buf_writer = log_info[i].log_buf_info->waddr;
		log_unlock(i, &flags);

		buf_write_len = (buf_writer > buf_reader) ? (buf_writer - buf_reader)
						: (log_info->log_buf_size - (buf_reader - buf_writer));

		if (buf_write_len >= (log_info->log_buf_size >> 1))
			dump_log(i);
	}
}

static int kdumplog(void *p)
{
	int ret = 0;
	int i;

	for (i = 0; i < LOG_MAX; i++) {
		ret = create_dumpfile(log_info[i].log_file_name, log_info[i].log_file_size);

		if (ret) {
			pr_err("dumplog create file error i=%d,ret=%d\n", i , ret);
			return ret;
		}
	}

	while (!kthread_should_stop()) {
		ret = down_timeout(&k3log_sema, msecs_to_jiffies(DUMP_LOG_TIME_OUT*1000));
		if (down_interruptible(&k3log_sys_sema) < 0) {
			pr_err("acquire the semaphore error!\n");
		}

		if (ret < 0) {
			flush_all_logs();
		} else {
			dump_all_logs();
		}
		up(&k3log_sys_sema);
	}
	return 0;
}

static int create_dumpfile(char *name, unsigned long file_size)
{
	unsigned long file_pointer = sizeof(file_pointer);
	char tmpbuf[TMP_BUF_LEN];
	unsigned long tmp_file_len;
	mm_segment_t fs;
	struct kstat file_stat;
	struct file *fp;
	int ret;
	int i = 0;

	fs = get_fs();
	set_fs(KERNEL_DS);

	/* wait for file system(/data/dumplog) ready, at most 2 mins */
	do {
		ret = vfs_stat(K3_LOG_DIR, &file_stat);

		if (ret < 0)
				msleep(K3_SLEEP_TIME);
			else
				break;

			i++;
	} while (i < K3_LOOP_TIMES);

	if (ret < 0)
		return -1;

	ret = vfs_stat(name, &file_stat);

	if (ret < 0) {
		memset(tmpbuf, 0xFF, TMP_BUF_LEN);
		fp = filp_open(name, O_RDWR | O_CREAT, 0666);

		if (IS_ERR(fp)) {
			//pr_err("open log file error\n");
			return (int)(PTR_ERR(fp));
		}

		/* write file pointer */
		vfs_llseek(fp, 0, SEEK_SET);
		vfs_write(fp, (char *)&file_pointer, sizeof(file_pointer), &fp->f_pos);

		/* reserve log file */
		tmp_file_len = file_size;

		while (tmp_file_len > 0) {
			if (tmp_file_len >= TMP_BUF_LEN) {
				if (vfs_write(fp, tmpbuf, TMP_BUF_LEN, &fp->f_pos) < 0)
					break;

				tmp_file_len -= TMP_BUF_LEN;
			} else {
				if (vfs_write(fp, tmpbuf, tmp_file_len, &fp->f_pos) < 0)
					break;

				tmp_file_len = 0;
			}
		}

		if (fp)
			filp_close(fp, 0);
	}

	set_fs(fs);

	return 0;
}

static void k3log_release(struct device *dev)
{
	return;
}

static void k3log_early_suspend(struct early_suspend *h)
{
	pr_info("k3log_early_suspend+\n");
	pr_info("k3log_early_suspend-\n");

	return;
}

static void k3log_late_resume(struct early_suspend *h)
{
	pr_info("k3log_late_resume+\n");
	pr_info("k3log_late_resume-\n");

	return;
}

static struct platform_driver hisik3_log_driver = {
	.probe          = hisik3_log_probe,
	.remove         = hisik3_log_remove,
	.suspend		= hisik3_log_suspend,
	.resume			= hisik3_log_resume,
	.shutdown       = hisik3_log_shutdown,
	.driver         = {
		.name           = "hisik3_log",
		.owner          = THIS_MODULE,
	},
};

static struct platform_device hisik3_log = {
	.name           = "hisik3_log",
	.dev = {
		.release = k3log_release
	},
};

static int __init k3log_init(void)
{
	int rc = 0;
	int i = 0;

	hilog_loaded  = 1;
	pr_info("k3log_init set hilog_loaded = 1\n");

	rc = platform_device_register(&hisik3_log);

	if (rc) {
		pr_err("k3 log init failed\n");
		return rc;
	} else {
		rc = platform_driver_register(&hisik3_log_driver);

		if (rc) {
			platform_device_unregister(&hisik3_log);
			return rc;
		}

		set_log_info(LOG_KERNEL,
					log_buf_info,
					res_log_buf,
					KERNEL_LOG_BUF_LEN,
					KERNEL_LOG_FILE_LEN,
					KERNEL_LOG_FILE_NAME);

		set_log_info(LOG_MAIN,
					log_buf_info+1,
					ioremap_nocache(K3_MAIN_LOG_BASE, MAIN_LOG_BUF_LEN),
					MAIN_LOG_BUF_LEN,
					MAIN_LOG_FILE_LEN,
					MAIN_LOG_FILE_NAME);

		set_log_info(LOG_EVENTS,
					log_buf_info+2,
					ioremap_nocache(K3_EVENTS_LOG_BASE, EVENTS_LOG_BUF_LEN),
					EVENTS_LOG_BUF_LEN,
					EVENTS_LOG_FILE_LEN,
					EVENTS_LOG_FILE_NAME);

		set_log_info(LOG_RADIO,
					log_buf_info+3,
					ioremap_nocache(K3_RADIO_LOG_BASE, RADIO_LOG_BUF_LEN),
					RADIO_LOG_BUF_LEN,
					RADIO_LOG_FILE_LEN,
					RADIO_LOG_FILE_NAME);

		set_log_info(LOG_SYSTEM,
					log_buf_info+4,
					ioremap_nocache(K3_SYSTEM_LOG_BASE, SYSTEM_LOG_BUF_LEN),
					SYSTEM_LOG_BUF_LEN,
					SYSTEM_LOG_FILE_LEN,
					SYSTEM_LOG_FILE_NAME);

		for (i = 0; i < LOG_MAX; i++) {
			if (!log_info[i].log_buf_info || !log_info[i].log_buf_addr)
			return -EFAULT;
		}

		kdumplog_task = kthread_create(kdumplog, 0, "kdumplogd");

		if (IS_ERR(kdumplog_task)) {
			pr_err("k3 log create thread failed\n");
			return (int)(PTR_ERR(kdumplog_task));
		}

#ifdef CONFIG_WAKELOCK
		hilog_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
		hilog_early_suspend.suspend = k3log_early_suspend;
		hilog_early_suspend.resume = k3log_late_resume;
		register_early_suspend(&hilog_early_suspend);
#endif

		pr_info("k3 log init ok\n");

		wake_up_process(kdumplog_task);
	}

	return rc;
}

static void __exit k3log_exit(void)
{
	up(&k3log_sema);
	kthread_stop(kdumplog_task);

#ifdef CONFIG_WAKELOCK
	unregister_early_suspend(&hilog_early_suspend);
#endif

	platform_driver_unregister(&hisik3_log_driver);
	platform_device_unregister(&hisik3_log);
}

pure_initcall(k3log_init);
module_exit(k3log_exit);
MODULE_LICENSE("GPL");
