/*
 * drivers/rtc/rtc-pl031.c
 *
 * Real Time Clock interface for ARM AMBA PrimeCell 031 RTC
 *
 * Author: Deepak Saxena <dsaxena@plexity.net>
 *
 * Copyright 2006 (c) MontaVista Software, Inc.
 *
 * Author: Mian Yousaf Kaukab <mian.yousaf.kaukab@stericsson.com>
 * Copyright 2010 (c) ST-Ericsson AB
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/amba/bus.h>
#include <linux/io.h>
#include <linux/bcd.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <mach/platform.h>

/* add for reboot in recovery charging mode */
#include <linux/reboot.h>
#include <linux/syscalls.h>
#include <linux/workqueue.h>
#include <linux/init.h>
#include <linux/kthread.h>

#define USE_PMU_RTC

/*
 * Register definitions
 */
#define	RTC_DR		0x00	/* Data read register */
#define	RTC_MR		0x04	/* Match register */
#define	RTC_LR		0x08	/* Data load register */
#define	RTC_CR		0x0c	/* Control register */
#define	RTC_IMSC	0x10	/* Interrupt mask and set register */
#define	RTC_RIS		0x14	/* Raw interrupt status register */
#define	RTC_MIS		0x18	/* Masked interrupt status register */
#define	RTC_ICR		0x1c	/* Interrupt clear register */
/* ST variants have additional timer functionality */
#define RTC_TDR		0x20	/* Timer data read register */
#define RTC_TLR		0x24	/* Timer data load register */
#define RTC_TCR		0x28	/* Timer control register */
#define RTC_YDR		0x30	/* Year data read register */
#define RTC_YMR		0x34	/* Year match register */
#define RTC_YLR		0x38	/* Year data load register */

#define RTC_CR_CWEN	(1 << 26)	/* Clockwatch enable bit */

#define RTC_TCR_EN	(1 << 1) /* Periodic timer enable bit */

/* Common bit definitions for Interrupt status and control registers */
#define RTC_BIT_AI	(1 << 0) /* Alarm interrupt bit */
#define RTC_BIT_PI	(1 << 1) /* Periodic interrupt bit. ST variants only. */

/* Common bit definations for ST v2 for reading/writing time */
#define RTC_SEC_SHIFT 0
#define RTC_SEC_MASK (0x3F << RTC_SEC_SHIFT) /* Second [0-59] */
#define RTC_MIN_SHIFT 6
#define RTC_MIN_MASK (0x3F << RTC_MIN_SHIFT) /* Minute [0-59] */
#define RTC_HOUR_SHIFT 12
#define RTC_HOUR_MASK (0x1F << RTC_HOUR_SHIFT) /* Hour [0-23] */
#define RTC_WDAY_SHIFT 17
#define RTC_WDAY_MASK (0x7 << RTC_WDAY_SHIFT) /* Day of Week [1-7] 1=Sunday */
#define RTC_MDAY_SHIFT 20
#define RTC_MDAY_MASK (0x1F << RTC_MDAY_SHIFT) /* Day of Month [1-31] */
#define RTC_MON_SHIFT 25
#define RTC_MON_MASK (0xF << RTC_MON_SHIFT) /* Month [1-12] 1=January */

#define RTC_TIMER_FREQ 32768

/*
 * Hi6421V200 RTC Register definitions
 */
#define	HI6421V200_RTCDR0		(0x58 << 2)	/* Data read register */
#define	HI6421V200_RTCDR1		(0x59 << 2)	/* Data read register */
#define	HI6421V200_RTCDR2		(0x5A << 2)	/* Data read register */
#define	HI6421V200_RTCDR3		(0x5B << 2)	/* Data read register */
#define	HI6421V200_RTCMR0		(0x5C << 2)	/* Match register */
#define	HI6421V200_RTCMR1		(0x5D << 2)	/* Match register */
#define	HI6421V200_RTCMR2		(0x5E << 2)	/* Match register */
#define	HI6421V200_RTCMR3		(0x5F << 2)	/* Match register */
#define	HI6421V200_RTCLR0		(0x60 << 2)	/* Data load register */
#define	HI6421V200_RTCLR1		(0x61 << 2)	/* Data load register */
#define	HI6421V200_RTCLR2		(0x62 << 2)	/* Data load register */
#define	HI6421V200_RTCLR3		(0x63 << 2)	/* Data load register */
#define HI6421V200_RTCCTRL		(0x64 << 2)	/* Control register */

extern unsigned int get_pd_charge_flag(void);
extern struct rtc_wkalrm poweroff_rtc_alarm;

static unsigned long rtc_base;

static unsigned long k3_rtc_base;

struct pl031_local {
	struct rtc_device *rtc;
	void __iomem      *base;
	u8 hw_designer;
	u8 hw_revision:4;
};

static void k3_pmu_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	unsigned long time;
	unsigned char temp_val;
	int err;

	err = rtc_valid_tm(tm);
	if (err != 0) {
		dev_err(dev, "set pmu rtc err.\n");
		return;
	}

	rtc_tm_to_time(tm, &time);

	/* stop PMU RTC */
	temp_val = 0;
	writel(temp_val, k3_rtc_base + HI6421V200_RTCCTRL);

	temp_val = time & 0xFF;
	writel(temp_val, k3_rtc_base + HI6421V200_RTCLR0);
	temp_val = (time >> 8) & 0xFF;
	writel(temp_val, k3_rtc_base + HI6421V200_RTCLR1);
	temp_val = (time >> 16) & 0xFF;
	writel(temp_val, k3_rtc_base + HI6421V200_RTCLR2);
	temp_val = (time >> 24) & 0xFF;
	writel(temp_val, k3_rtc_base + HI6421V200_RTCLR3);

	/* start PMU RTC */
	temp_val = 1;
	writel(temp_val, k3_rtc_base + HI6421V200_RTCCTRL);
}

/* FIXME: Alarm Open the Phone if shutdown. Record Alarm Time to PMU RTC RTCMR */
static void k3_pmu_rtc_setalarmtime(unsigned long time)
{
	unsigned char temp_val;

	temp_val = time & 0xFF;
	writel(temp_val, k3_rtc_base + HI6421V200_RTCMR0);
	temp_val = (time >> 8) & 0xFF;
	writel(temp_val, k3_rtc_base + HI6421V200_RTCMR1);
	temp_val = (time >> 16) & 0xFF;
	writel(temp_val, k3_rtc_base + HI6421V200_RTCMR2);
	temp_val = (time >> 24) & 0xFF;
	writel(temp_val, k3_rtc_base + HI6421V200_RTCMR3);

	return;
}

static void k3_pmu_rtc_readalarmtime(struct rtc_time *tm)
{
	unsigned long time;
	unsigned char temp_val;

	temp_val = readl(k3_rtc_base + HI6421V200_RTCMR0);
	time = temp_val;
	temp_val = readl(k3_rtc_base + HI6421V200_RTCMR1);
	time |= (temp_val << 8);
	temp_val = readl(k3_rtc_base + HI6421V200_RTCMR2);
	time |= (temp_val << 16);
	temp_val = readl(k3_rtc_base + HI6421V200_RTCMR3);
	time |= (temp_val << 24);

	rtc_time_to_tm(time, tm);

	return;
}

static void k3_pmu_rtc_readtime(struct rtc_time *tm)
{
	unsigned long time;
	unsigned char temp_val;

	temp_val = readl(k3_rtc_base + HI6421V200_RTCDR0);
	time = temp_val;
	temp_val = readl(k3_rtc_base + HI6421V200_RTCDR1);
	time |= (temp_val << 8);
	temp_val = readl(k3_rtc_base + HI6421V200_RTCDR2);
	time |= (temp_val << 16);
	temp_val = readl(k3_rtc_base + HI6421V200_RTCDR3);
	time |= (temp_val << 24);

	rtc_time_to_tm(time, tm);

	return;
}

static int pl031_alarm_irq_enable(struct device *dev,
	unsigned int enabled)
{
	struct pl031_local *ldata = dev_get_drvdata(dev);
	unsigned long imsc;

	/* Clear any pending alarm interrupts. */
	writel(RTC_BIT_AI, ldata->base + RTC_ICR);

	imsc = readl(ldata->base + RTC_IMSC);

	if (enabled == 1)
		writel(imsc | RTC_BIT_AI, ldata->base + RTC_IMSC);
	else
		writel(imsc & ~RTC_BIT_AI, ldata->base + RTC_IMSC);

	return 0;
}

/*
 * Convert Gregorian date to ST v2 RTC format.
 */
static int pl031_stv2_tm_to_time(struct device *dev,
				 struct rtc_time *tm, unsigned long *st_time,
	unsigned long *bcd_year)
{
	int year = tm->tm_year + 1900;
	int wday = tm->tm_wday;

	/* wday masking is not working in hardware so wday must be valid */
	if (wday < -1 || wday > 6) {
		dev_err(dev, "invalid wday value %d\n", tm->tm_wday);
		return -EINVAL;
	} else if (wday == -1) {
		/* wday is not provided, calculate it here */
		unsigned long time;
		struct rtc_time calc_tm;

		rtc_tm_to_time(tm, &time);
		rtc_time_to_tm(time, &calc_tm);
		wday = calc_tm.tm_wday;
	}

	*bcd_year = (bin2bcd(year % 100) | bin2bcd(year / 100) << 8);

	*st_time = ((tm->tm_mon + 1) << RTC_MON_SHIFT)
			|	(tm->tm_mday << RTC_MDAY_SHIFT)
			|	((wday + 1) << RTC_WDAY_SHIFT)
			|	(tm->tm_hour << RTC_HOUR_SHIFT)
			|	(tm->tm_min << RTC_MIN_SHIFT)
			|	(tm->tm_sec << RTC_SEC_SHIFT);

	return 0;
}

/*
 * Convert ST v2 RTC format to Gregorian date.
 */
static int pl031_stv2_time_to_tm(unsigned long st_time, unsigned long bcd_year,
	struct rtc_time *tm)
{
	tm->tm_year = bcd2bin(bcd_year) + (bcd2bin(bcd_year >> 8) * 100);
	tm->tm_mon  = ((st_time & RTC_MON_MASK) >> RTC_MON_SHIFT) - 1;
	tm->tm_mday = ((st_time & RTC_MDAY_MASK) >> RTC_MDAY_SHIFT);
	tm->tm_wday = ((st_time & RTC_WDAY_MASK) >> RTC_WDAY_SHIFT) - 1;
	tm->tm_hour = ((st_time & RTC_HOUR_MASK) >> RTC_HOUR_SHIFT);
	tm->tm_min  = ((st_time & RTC_MIN_MASK) >> RTC_MIN_SHIFT);
	tm->tm_sec  = ((st_time & RTC_SEC_MASK) >> RTC_SEC_SHIFT);

	tm->tm_yday = rtc_year_days(tm->tm_mday, tm->tm_mon, tm->tm_year);
	tm->tm_year -= 1900;

	return 0;
}

static int pl031_stv2_read_time(struct device *dev, struct rtc_time *tm)
{
	struct pl031_local *ldata = dev_get_drvdata(dev);

	pl031_stv2_time_to_tm(readl(ldata->base + RTC_DR),
			readl(ldata->base + RTC_YDR), tm);

	return 0;
}

static int pl031_stv2_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long time;
	unsigned long bcd_year;
	struct pl031_local *ldata = dev_get_drvdata(dev);
	int ret;

	ret = pl031_stv2_tm_to_time(dev, tm, &time, &bcd_year);
	if (ret == 0) {
		writel(bcd_year, ldata->base + RTC_YLR);
		writel(time, ldata->base + RTC_LR);
	}

	return ret;
}

static int pl031_stv2_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct pl031_local *ldata = dev_get_drvdata(dev);
	int ret;

	ret = pl031_stv2_time_to_tm(readl(ldata->base + RTC_MR),
			readl(ldata->base + RTC_YMR), &alarm->time);

	alarm->pending = readl(ldata->base + RTC_RIS) & RTC_BIT_AI;
	alarm->enabled = readl(ldata->base + RTC_IMSC) & RTC_BIT_AI;

	return ret;
}

static int pl031_stv2_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct pl031_local *ldata = dev_get_drvdata(dev);
	unsigned long time;
	unsigned long bcd_year;
	int ret;

	/* At the moment, we can only deal with non-wildcarded alarm times. */
	ret = rtc_valid_tm(&alarm->time);
	if (ret == 0) {
		ret = pl031_stv2_tm_to_time(dev, &alarm->time,
					    &time, &bcd_year);
		if (ret == 0) {
			writel(bcd_year, ldata->base + RTC_YMR);
			writel(time, ldata->base + RTC_MR);

			pl031_alarm_irq_enable(dev, alarm->enabled);
		}
	}

	return ret;
}

/* add a workqueue for reboot */
static int oem_rtc_reboot_thread(void *u)
{
	printk("Entering Rebooting Causeed by Alarm...\n");
	emergency_remount();
	sys_sync();
	kernel_restart("oem_rtc");

	/* should not be here */
	panic("oem_rtc");
	return 0;
}
void oem_rtc_reboot(unsigned long t)
{
	kernel_thread(oem_rtc_reboot_thread, NULL, CLONE_FS | CLONE_FILES);
}
static DECLARE_TASKLET(oem_rtc_reboot_tasklet, oem_rtc_reboot, 0);

static irqreturn_t pl031_interrupt(int irq, void *dev_id)
{
#ifndef USE_PMU_RTC
	struct pl031_local *ldata = dev_id;
	unsigned long rtcmis;
	unsigned long events = 0;

	rtcmis = readl(ldata->base + RTC_MIS);
	if (rtcmis) {
		writel(rtcmis, ldata->base + RTC_ICR);

		if (rtcmis & RTC_BIT_AI)
			events |= (RTC_AF | RTC_IRQF);

		/* Timer interrupt is only available in ST variants */
		if ((rtcmis & RTC_BIT_PI) &&
			(ldata->hw_designer == AMBA_VENDOR_ST))
			events |= (RTC_PF | RTC_IRQF);

		rtc_update_irq(ldata->rtc, 1, events);

		return IRQ_HANDLED;
	}
#endif
	if(unlikely(get_pd_charge_flag())) {
		tasklet_schedule(&oem_rtc_reboot_tasklet);
	}

	return IRQ_NONE;
}

static int pl031_read_time(struct device *dev, struct rtc_time *tm)
{
#ifndef USE_PMU_RTC
	struct pl031_local *ldata = dev_get_drvdata(dev);

	rtc_time_to_tm(readl(ldata->base + RTC_DR), tm);
#else
        k3_pmu_rtc_readtime(tm);
#endif
	return 0;
}

static int pl031_set_time(struct device *dev, struct rtc_time *tm)
{
#ifndef USE_PMU_RTC
	unsigned long time;
	struct pl031_local *ldata = dev_get_drvdata(dev);
#endif
	int err;

	err = rtc_valid_tm(tm);
	if (err != 0) {
		return err;
	}
#ifndef USE_PMU_RTC
	err = rtc_tm_to_time(tm, &time);

    if (err == 0)
	    writel(time, ldata->base + RTC_LR);
#else
	k3_pmu_rtc_settime(dev, tm);
#endif

	return 0;
}

static int pl031_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
#ifndef USE_PMU_RTC
	struct pl031_local *ldata = dev_get_drvdata(dev);

	rtc_time_to_tm(readl(ldata->base + RTC_MR), &alarm->time);

	alarm->pending = readl(ldata->base + RTC_RIS) & RTC_BIT_AI;
	alarm->enabled = readl(ldata->base + RTC_IMSC) & RTC_BIT_AI;
#else
        k3_pmu_rtc_readalarmtime(&alarm->time);
#endif

	return 0;
}

static int pl031_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
#ifndef USE_PMU_RTC
	struct pl031_local *ldata = dev_get_drvdata(dev);
#endif
	unsigned long time;
	int ret;

	/* At the moment, we can only deal with non-wildcarded alarm times. */
	ret = rtc_valid_tm(&alarm->time);
	if (ret == 0) {
		ret = rtc_tm_to_time(&alarm->time, &time);
		if (ret == 0) {
#ifndef USE_PMU_RTC
			writel(time, ldata->base + RTC_MR);
			pl031_alarm_irq_enable(dev, alarm->enabled);
#endif
			k3_pmu_rtc_setalarmtime(time);
		}
	}

	return ret;
}


static int pl031_remove(struct amba_device *adev)
{
	struct pl031_local *ldata = dev_get_drvdata(&adev->dev);

	amba_set_drvdata(adev, NULL);
	free_irq(adev->irq[0], ldata->rtc);
	rtc_device_unregister(ldata->rtc);
	iounmap(ldata->base);

	kfree(ldata);
	amba_release_regions(adev);

	return 0;
}

static int pl031_probe(struct amba_device *adev, const struct amba_id *id)
{
	int ret;
	struct pl031_local *ldata;
	struct rtc_class_ops *ops = id->data;
	struct rtc_time alarm_tm;
	unsigned long time = 0;
	unsigned long alarm_time = 0;

	ret = amba_request_regions(adev, NULL);
	if (ret)
		goto err_req;

	ldata = kzalloc(sizeof(struct pl031_local), GFP_KERNEL);
	if (!ldata) {
		ret = -ENOMEM;
		goto out;
	}

	ldata->base = ioremap(adev->res.start, resource_size(&adev->res));

	if (!ldata->base) {
		ret = -ENOMEM;
		goto out_no_remap;
	}

	k3_rtc_base = IO_ADDRESS(REG_BASE_PMUSPI);

	amba_set_drvdata(adev, ldata);

	ldata->hw_designer = amba_manf(adev);
	ldata->hw_revision = amba_rev(adev);

	dev_dbg(&adev->dev, "designer ID = 0x%02x\n", ldata->hw_designer);
	dev_dbg(&adev->dev, "revision = 0x%01x\n", ldata->hw_revision);

	/* Enable the clockwatch on ST Variants */
	if ((ldata->hw_designer == AMBA_VENDOR_ST) &&
	    (ldata->hw_revision > 1))
		writel(readl(ldata->base + RTC_CR) | RTC_CR_CWEN,
		       ldata->base + RTC_CR);

	ldata->rtc = rtc_device_register("pl031", &adev->dev, ops,
					THIS_MODULE);
	if (IS_ERR(ldata->rtc)) {
		ret = PTR_ERR(ldata->rtc);
		goto out_no_rtc;
	}

#ifndef USE_PMU_RTC
	if (request_irq(adev->irq[0], pl031_interrupt,
#else
	if (request_irq(IRQ_ALARMON_RISING, pl031_interrupt,
#endif
			IRQF_DISABLED, "rtc-pl031", ldata)) {
		ret = -EIO;
		goto out_no_irq;
	}

	/* record rtc_base addr */
	rtc_base = (unsigned long) ldata->base;

	{
		unsigned long rtccr_val = 0;
		struct rtc_time tm;

		memset(&tm, 0, sizeof(struct rtc_time));

		/* read PMU RTC (battery there!) */
		rtccr_val = readl(k3_rtc_base + HI6421V200_RTCCTRL);
		/* PMU RTC not set */
		if (rtccr_val == 0) {
			tm.tm_year = 111;
			tm.tm_mon = 0;
			tm.tm_mday = 1;
			k3_pmu_rtc_settime(&adev->dev, &tm);
		} else{
			k3_pmu_rtc_readtime(&tm);
		}
		rtc_tm_to_time(&tm, &time);

		dev_info(&adev->dev, "rtc: year %d mon %d day %d hour %d min %d sec %d time 0x%lx\n",
		 tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, time);

		writel(1, ldata->base + RTC_CR);
		writel(time, ldata->base + RTC_LR);
	}

	/* clear IRQ before enable */
	writel(1, ldata->base + RTC_ICR);
	/* enable IRQ */
	writel(1, ldata->base + RTC_IMSC);
	k3_pmu_rtc_readalarmtime(&alarm_tm);
	printk(KERN_DEBUG "Last alarm %d-%d-%d %d:%d:%d\n",
	       alarm_tm.tm_year, alarm_tm.tm_mon,
	       alarm_tm.tm_mday, alarm_tm.tm_hour,
	       alarm_tm.tm_min, alarm_tm.tm_sec);

	rtc_tm_to_time(&alarm_tm, &alarm_time);

	if (unlikely(get_pd_charge_flag() && (alarm_time > time))) {
		poweroff_rtc_alarm.enabled = 1;
		memcpy(&poweroff_rtc_alarm.time, &alarm_tm, sizeof(struct rtc_time));
		printk(KERN_DEBUG "set to poweroff alarm %d-%d-%d %d:%d:%d\n",
			poweroff_rtc_alarm.time.tm_year,
			poweroff_rtc_alarm.time.tm_mon,
			poweroff_rtc_alarm.time.tm_mday,
			poweroff_rtc_alarm.time.tm_hour,
			poweroff_rtc_alarm.time.tm_min,
			poweroff_rtc_alarm.time.tm_sec);
	}

	return 0;

out_no_irq:
	rtc_device_unregister(ldata->rtc);
out_no_rtc:
	iounmap(ldata->base);
	amba_set_drvdata(adev, NULL);
out_no_remap:
	kfree(ldata);
out:
	amba_release_regions(adev);
err_req:

	return ret;
}

static void pl031_shutdown(struct amba_device *adev)
{
	int rc;
	struct pl031_local *ldata = dev_get_drvdata(&adev->dev);
	unsigned long imsc;

	if(unlikely(get_pd_charge_flag()))
		return;

	/* clear IRQ before shutdown */
	writel(1, ldata->base + RTC_ICR);
	if (poweroff_rtc_alarm.enabled) {
		rc = pl031_set_alarm(&(adev->dev), &poweroff_rtc_alarm);
		if (rc < 0)
			printk(KERN_ERR "Set poweroff alarm FAIL!\n");
		else
			printk(KERN_INFO "Set poweroff alarm Success!\n");
	} else {
		writel(0, ldata->base + RTC_MR);

		k3_pmu_rtc_setalarmtime(0);
		/* disable irq */
		imsc = readl(ldata->base + RTC_IMSC);
		writel(imsc & ~RTC_BIT_AI, ldata->base + RTC_IMSC);
	}

}

/* Operations for the original ARM version */
static struct rtc_class_ops arm_pl031_ops = {
	.read_time = pl031_read_time,
	.set_time = pl031_set_time,
	.read_alarm = pl031_read_alarm,
	.set_alarm = pl031_set_alarm,
	.alarm_irq_enable = pl031_alarm_irq_enable,
};

/* The First ST derivative */
static struct rtc_class_ops stv1_pl031_ops = {
	.read_time = pl031_read_time,
	.set_time = pl031_set_time,
	.read_alarm = pl031_read_alarm,
	.set_alarm = pl031_set_alarm,
	.alarm_irq_enable = pl031_alarm_irq_enable,
};

/* And the second ST derivative */
static struct rtc_class_ops stv2_pl031_ops = {
	.read_time = pl031_stv2_read_time,
	.set_time = pl031_stv2_set_time,
	.read_alarm = pl031_stv2_read_alarm,
	.set_alarm = pl031_stv2_set_alarm,
	.alarm_irq_enable = pl031_alarm_irq_enable,
};

static struct amba_id pl031_ids[] = {
	{
		.id = 0x00041031,
		.mask = 0x000fffff,
		.data = &arm_pl031_ops,
	},
	/* ST Micro variants */
	{
		.id = 0x00180031,
		.mask = 0x00ffffff,
		.data = &stv1_pl031_ops,
	},
	{
		.id = 0x00280031,
		.mask = 0x00ffffff,
		.data = &stv2_pl031_ops,
	},
	{0, 0},
};

static struct amba_driver pl031_driver = {
	.drv = {
		.name = "rtc-pl031",
	},
	.id_table = pl031_ids,
	.probe = pl031_probe,
	.remove = pl031_remove,
	.shutdown = pl031_shutdown,
};

static int __init pl031_init(void)
{
	/* using tasklet to replace workqueue here,
	 * because flush_scheduled_work() will flush itself.
	 */
	return amba_driver_register(&pl031_driver);
}

static void __exit pl031_exit(void)
{
	amba_driver_unregister(&pl031_driver);
}

module_init(pl031_init);
module_exit(pl031_exit);

MODULE_AUTHOR("Deepak Saxena <dsaxena@plexity.net");
MODULE_DESCRIPTION("ARM AMBA PL031 RTC Driver");
MODULE_LICENSE("GPL");
