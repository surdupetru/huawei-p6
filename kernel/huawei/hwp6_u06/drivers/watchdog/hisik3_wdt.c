/*
 * drivers/char/watchdog/hisik3_wdt.c
 *
 * Watchdog driver for watchdog module present with HISIK3 chipset
 *
 * Copyright (C) 2012  HUAWEI
 * sunhaihuan<sunhaihuan@huawei.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/syscore_ops.h>
#include <linux/cpu.h>
#include <linux/err.h>

/* default timeout in seconds */
#define DEFAULT_TIMEOUT         (10)

#define MODULE_NAME             "k3v2_watchdog"

/* watchdog register offsets and masks */
#define WDTLOAD                 0x000
        #define LOAD_MIN        0x00000000
        #define LOAD_MAX        0xFFFFFFFF

#define WDTVALUE                0x004

#define WDTCONTROL              0x008
        #define WDT_INTEN       (1 << 0)
        #define WDT_RESEN       (1 << 1)

#define WDTINTCLR               0x00C
        #define WDT_ANYVALUE    0xFFFFFFFF

#define WDTRIS                  0x010
        #define WDT_WDOGRIS     (1 << 0)

#define WDTMIS                  0x014
        #define WDT_WDOGMIS     (1 << 0)

#define WDTLOCK                 0xC00
        #define WDTLOCK_ALLWEN  0x1ACCE551


 /**
 * struct hisik3_wdt: cortex a9 wdt device structure
 *
 * lock: spin lock protecting dev structure and io access
 * base: base address of wdt
 * clk: clock structure of wdt
 * pdev: platform device structure of wdt
 * boot_status: boot time status of wdt
 * status: current status of wdt
 * prescale: prescale value to be set for current timeout
 * load_val: load value to be set for current timeout
 * timeout: current programmed timeout
 */
 struct hisik3_wdt {
        spinlock_t                      lock;
        void __iomem                    *base;
        struct clk                      *clk;
        struct platform_device          *pdev;
//		struct delayed_work				k3_wdt_delayed_work;
        unsigned long                   boot_status;
        unsigned long                   status;
        #define WDT_BUSY                0
        #define WDT_CAN_BE_CLOSED       1
        unsigned int                    prescale;
        unsigned int                    load_val;
        unsigned int                    timeout;
 };

static void wdt_config(unsigned int timeout);
static void wdt_enable(void);
static void wdt_disable(void);

 /* local variables */
static DECLARE_WAIT_QUEUE_HEAD(wdt_queue);
 static struct hisik3_wdt *wdt;
 static int nowayout = WATCHDOG_NOWAYOUT;

static DEFINE_PER_CPU(struct task_struct *, k3wdt_kick_watchdog_task);
static int k3_wdt_kick_start_oncpu(int cpu);
static void k3_wdt_kick_stop_oncpu(int cpu);
static int k3_wdt_kick_start(void);
static void k3_wdt_kick_stop(void);
static struct notifier_block __cpuinitdata k3wdt_cpu_nfb ;
 /* binary search */
 static inline void bsearch(u32 *var, u32 var_start, u32 var_end,
                 const u64 param, const u32 timeout, u32 rate)
 {
         u64 tmp = 0;

         /* get the lowest var value that can satisfy our requirement */
         while (var_start < var_end) {
                 tmp = var_start;
                 tmp += var_end;
                 tmp = div_u64(tmp, 2);
                 if (timeout > div_u64((param + 1) * (tmp + 1), rate)) {
                         if (var_start == tmp)
                                 break;
                         else
                                 var_start = tmp;
                 } else {
                         if (var_end == tmp)
                                 break;
                         else
                                 var_end = tmp;
                 }
         }
         *var = tmp;
 }

static void wdt_default_init(unsigned int timeout)
{
	u64 psc = 0;
	u32 load = 0;
	u32 rate = 32000;/*wdt timer clock is 32k*/

	/* get appropriate value of psc and load */
	bsearch((u32 *)&load, LOAD_MIN, LOAD_MAX, psc, timeout, rate);

	wdt->prescale = psc;
	wdt->load_val = load;
	wdt->timeout = timeout;
}

static void wdt_default_config(void)
{
	spin_lock(&wdt->lock);

	/* unlock WDT register */
	writel(WDTLOCK_ALLWEN,wdt->base + WDTLOCK);

	writel(wdt->load_val, wdt->base + WDTLOAD);

	/* clear intr ok */
	writel(1,wdt->base + WDTINTCLR);

	/* relock WDT register */
	writel(0,wdt->base + WDTLOCK);

	spin_unlock(&wdt->lock);
}

//static void wdt_mond(struct work_struct *work)
//{
//	struct hisik3_wdt *wdt = container_of(work, struct hisik3_wdt, k3_wdt_delayed_work.work);

//	wdt_default_config();
//	schedule_delayed_work(&wdt->k3_wdt_delayed_work, msecs_to_jiffies(DEFAULT_TIMEOUT*1000));
//}
/*
 * This routine finds the most appropriate prescale and load value for input
 * timout value
 */
static void wdt_config(unsigned int timeout)
{
	wdt_default_init(timeout);

        spin_lock(&wdt->lock);

	/* unlock WDT register */
	writel(WDTLOCK_ALLWEN,wdt->base + WDTLOCK);

        writel(wdt->load_val,wdt->base + WDTLOAD);

	/* clear intr ok */
	writel(1,wdt->base + WDTINTCLR);

	/* relock WDT register */
	writel(0,wdt->base + WDTLOCK);

        spin_unlock(&wdt->lock);

}

static void wdt_enable(void)
{
        u32 val;

        spin_lock(&wdt->lock);

	/* unlock WDT register */
	writel(WDTLOCK_ALLWEN,wdt->base + WDTLOCK);

        /*enable WDT output reset signal and output interrupt signal*/
        val = WDT_INTEN | WDT_RESEN;
        writel(val, wdt->base + WDTCONTROL);

	/* relock WDT register */
	writel(0,wdt->base + WDTLOCK);

        spin_unlock(&wdt->lock);
}

void feed_watdog(void)
{
    u32 val;

    if (unlikely(NULL == wdt))
    {
        return;
    }

    if (spin_trylock(&wdt->lock))
    {
        /* unlock WDT register */
        writel(WDTLOCK_ALLWEN, wdt->base + WDTLOCK);

        /*enable WDT output reset signal and output interrupt signal*/
        val = WDT_INTEN | WDT_RESEN;
        writel(val, wdt->base + WDTCONTROL);

        /* relock WDT register */
        writel(0, wdt->base + WDTLOCK);

        spin_unlock(&wdt->lock);
    }
}
EXPROT_SYMBOL(feed_watdog);

static void wdt_disable(void)
{
        u32 val;
        spin_lock(&wdt->lock);

	/* unlock WDT register */
	writel(WDTLOCK_ALLWEN,wdt->base + WDTLOCK);

        /* disable WDT output reset signal and output interrupt signal */
        val = (~WDT_INTEN)&(~WDT_RESEN);
        writel(val,wdt->base + WDTCONTROL);

	/* relock WDT register */
	writel(0,wdt->base + WDTLOCK);

        spin_unlock(&wdt->lock);
}

static ssize_t hisik3_wdt_write(struct file *file, const char *data,
                size_t len, loff_t *ppos)
{
        if (len) {
                if (!nowayout) {
                        size_t i;

                        clear_bit(WDT_CAN_BE_CLOSED, &wdt->status);

                        for (i = 0; i != len; i++) {
                                char c;

                                if (get_user(c, data + i))
                                        return -EFAULT;
                                if (c == 'V') {
                                        set_bit(WDT_CAN_BE_CLOSED,
                                                        &wdt->status);
                                        break;
                                 }
                        }
                }
                wdt_enable();
        }
        return len;
}

static const struct watchdog_info ident = {
        .options = WDIOF_CARDRESET | WDIOF_MAGICCLOSE |
                WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
        .identity = MODULE_NAME,
};


static long hisik3_wdt_ioctl(struct file *file, unsigned int cmd,
                 unsigned long arg)
{
        int ret = -ENOTTY;
        int timeout;
        unsigned int temp = 0x0;

        switch (cmd) {
        case WDIOC_GETSUPPORT:
                ret = copy_to_user((struct watchdog_info *)arg, &ident,
                                sizeof(ident)) ? -EFAULT : 0;
                break;

        case WDIOC_GETSTATUS:
                ret = put_user(0, (int *)arg);
                break;
        /*  do not implement this feature now
        case WDIOC_GETBOOTSTATUS:
                ret = put_user(wdt->boot_status, (int *)arg);
                break;
        */

        case WDIOC_KEEPALIVE:
                wdt_enable();
                ret = 0;
                break;

        case WDIOC_SETTIMEOUT:
                ret = get_user(timeout, (int *)arg);
                if (ret)
                        break;
                dev_warn(&wdt->pdev->dev,"SETTIMEOUT : %d\n",timeout);
                wdt_config(timeout);
                wdt_enable();
                break;
        case WDIOC_GETTIMEOUT:
                ret = put_user(wdt->timeout, (int *)arg);
                break;

        case WDIOC_GETTEMP:
                temp = readl(wdt->base + WDTVALUE);
		/*dev_warn(&wdt->pdev->dev, "temp value = %02x",  temp);*/
		ret = put_user(temp, (unsigned int *)arg);
		break;

	default:
		break;

	}
        return ret;
}


 static int hisik3_wdt_open(struct inode *inode, struct file *file)
 {
         int ret = 0;
         if (test_and_set_bit(WDT_BUSY, &wdt->status))
                 return -EBUSY;
         ret = clk_enable(wdt->clk);
         if (ret) {
                 dev_err(&wdt->pdev->dev, "clock enable fail");
                 goto err;
         }

         wdt_enable();
         /* can not be closed, once enabled */
         clear_bit(WDT_CAN_BE_CLOSED, &wdt->status);
         return nonseekable_open(inode, file);

 err:
         dev_warn(&wdt->pdev->dev,"open watchdog file node failed\n");

         clear_bit(WDT_BUSY, &wdt->status);
         return ret;
 }


static int hisik3_wdt_release(struct inode *inode, struct file *file)
{
        if (!test_bit(WDT_CAN_BE_CLOSED, &wdt->status)) {
                clear_bit(WDT_BUSY, &wdt->status);
                dev_warn(&wdt->pdev->dev, "Device closed unexpectedly\n");
                return 0;
        }

        wdt_disable();
        clk_disable(wdt->clk);
        clear_bit(WDT_BUSY, &wdt->status);

        return 0;
}

static const struct file_operations hisik3_wdt_fops = {
        .owner = THIS_MODULE,
        .llseek = no_llseek,
        .write = hisik3_wdt_write,
        .unlocked_ioctl = hisik3_wdt_ioctl,
        .open = hisik3_wdt_open,
        .release = hisik3_wdt_release,
};

static struct miscdevice hisik3_wdt_miscdev = {
        .minor = WATCHDOG_MINOR,
        .name = "watchdog",
        .fops = &hisik3_wdt_fops,
};

static int k3wdt_kick_threadfunc(void *data)
{
	while(1)
	{
		printk(KERN_ERR "[%d] kick k3 dog\n",(int)data);
		wdt_default_config();

		if (kthread_should_stop())
		{
			printk(KERN_ERR "exit kick dog thread\n");
			break;
		}
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(DEFAULT_TIMEOUT *HZ);
		set_current_state(TASK_RUNNING);
	}
	return 0;
}
static int __devinit hisik3_wdt_probe(struct platform_device *pdev)
{
        int ret = 0;
        struct resource *res;

        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!res) {
                ret = -ENOENT;
                dev_warn(&pdev->dev, "WDT memory resource not defined\n");
                goto err;
        }

        if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
                dev_warn(&pdev->dev, "WDT failed to get memory region resource\n");
                ret = -ENOENT;
                goto err;
        }

        wdt = kzalloc(sizeof(*wdt), GFP_KERNEL);
        if (!wdt) {
                dev_warn(&pdev->dev, "WDT kzalloc failed\n");
                ret = -ENOMEM;
                goto err_kzalloc;
        }

        wdt->clk = clk_get(NULL,"clk_wd");
        if (IS_ERR(wdt->clk)) {
                dev_warn(&pdev->dev, "WDT clock not found\n");
                ret = PTR_ERR(wdt->clk);
                goto err_clk_get;
        }

        wdt->base = ioremap(res->start, resource_size(res));
        if (!wdt->base) {
                ret = -ENOMEM;
                dev_warn(&pdev->dev, "WDT ioremap fail\n");
                goto err_ioremap;
        }
        spin_lock_init(&wdt->lock);
        /* This checks if system booted after watchdog reset or not */
        ret = clk_enable(wdt->clk);
	if (ret) {
		dev_warn(&pdev->dev, "clock enable fail");
		goto err_clk_enable;
	}

	wdt->pdev = pdev;
	wdt_default_init(DEFAULT_TIMEOUT);
	wdt_default_config();

//	INIT_DELAYED_WORK(&wdt->k3_wdt_delayed_work, wdt_mond);
//	schedule_delayed_work(&wdt->k3_wdt_delayed_work, 0);
	ret = k3_wdt_kick_start();
	if(ret)
		goto err_create_thread;
	register_cpu_notifier((struct notifier_block *)&k3wdt_cpu_nfb);

        ret = misc_register(&hisik3_wdt_miscdev);
        if (ret < 0) {
                dev_warn(&pdev->dev, "WDT cannot register misc device\n");
                goto err_misc_register;
        }

	wdt_enable();

        dev_warn(&pdev->dev,"WDT probing has been finished\n");
        return 0;

err_misc_register:
	unregister_cpu_notifier((struct notifier_block *)&k3wdt_cpu_nfb);
err_create_thread:
	k3_wdt_kick_stop();
	clk_disable(wdt->clk);
err_clk_enable:
        iounmap(wdt->base);
err_ioremap:
        clk_put(wdt->clk);
err_clk_get:
        kfree(wdt);
        wdt = NULL;
err_kzalloc:
        release_mem_region(res->start, resource_size(res));
err:
	dev_warn(&pdev->dev, "WDT probe failed!!!\n");
        return ret;
}

static int __devexit hisik3_wdt_remove(struct platform_device *pdev)
{
        struct resource *res;
		unregister_cpu_notifier((struct notifier_block *)&k3wdt_cpu_nfb);
		k3_wdt_kick_stop();

        misc_deregister(&hisik3_wdt_miscdev);
        iounmap(wdt->base);
        clk_put(wdt->clk);
        kfree(wdt);
	wdt = NULL;

        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (res)
                release_mem_region(res->start, resource_size(res));

        return 0;
}

#ifdef CONFIG_PM

static int hisik3_wdt_suspend(struct platform_device *dev, pm_message_t pm)
{
	printk(KERN_INFO "[%s] +\n", __func__);

//	cancel_delayed_work(&wdt->k3_wdt_delayed_work);
	wdt_disable();
	clk_disable(wdt->clk);

	printk(KERN_INFO "[%s] -\n", __func__);
	return 0;
}

static int hisik3_wdt_resume(struct platform_device *dev)
{
	int ret = 0;

	printk(KERN_INFO "[%s] +\n", __func__);

	ret = clk_enable(wdt->clk);
	if (ret) {
		dev_warn(&wdt->pdev->dev, "clock enable fail");
	} else {
		wdt_enable();
//		schedule_delayed_work(&wdt->k3_wdt_delayed_work, 0);
	}

	printk(KERN_INFO "[%s] -\n", __func__);

	return 0;
}

#else

#define hisik3_wdt_suspend  NULL
#define hisik3_wdt_resume   NULL

#endif
static int k3_wdt_kick_start_oncpu(int cpu)
{
	int err = 0;
	struct task_struct *p = per_cpu(k3wdt_kick_watchdog_task, cpu);
	if (!p) {
		p = kthread_create(k3wdt_kick_threadfunc, (void *)(unsigned long)cpu, "k3wdt_kicktask/%d", cpu);
		if (IS_ERR(p)) {
			printk(KERN_ERR "softlockup watchdog for %i failed\n", cpu);
			err = PTR_ERR(p);
			goto out;
		}
		kthread_bind(p, cpu);
		per_cpu(k3wdt_kick_watchdog_task, cpu) = p;
		wake_up_process(p);
	}
out:
	return err;
}

static void k3_wdt_kick_stop_oncpu(int cpu)
{
	struct task_struct *p = per_cpu(k3wdt_kick_watchdog_task, cpu);
	if (p) {
		per_cpu(k3wdt_kick_watchdog_task, cpu) = NULL;
		kthread_stop(p);
	}
}

static int k3_wdt_kick_start(void)
{
	int cpu = 0;
	int ret = 0;
	for_each_online_cpu(cpu)
		ret |= k3_wdt_kick_start_oncpu(cpu);
	return ret;
}

static void k3_wdt_kick_stop(void)
{
	int cpu;
	for_each_online_cpu(cpu)
		k3_wdt_kick_stop_oncpu(cpu);
}

/*
 * Create/destroy watchdog threads as CPUs come and go:
 */
static int __cpuinit
cpu_callback(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	int hotcpu = (unsigned long)hcpu;

	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
		break;
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
		k3_wdt_kick_start_oncpu(hotcpu);
		break;
#ifdef CONFIG_HOTPLUG_CPU
	case CPU_UP_CANCELED:
	case CPU_UP_CANCELED_FROZEN:
		k3_wdt_kick_stop_oncpu(hotcpu);
		break;
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		k3_wdt_kick_stop_oncpu(hotcpu);
		break;
#endif /* CONFIG_HOTPLUG_CPU */
	}

	/*
	 * hardlockup and softlockup are not important enough
	 * to block cpu bring up.  Just always succeed and
	 * rely on printk output to flag problems.
	 */
	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata k3wdt_cpu_nfb = {
	.notifier_call = cpu_callback
};

static struct platform_driver hisik3v2_wdt_driver = {
        .driver = {
                .name = MODULE_NAME,
                .owner  = THIS_MODULE,
        },
        .probe = hisik3_wdt_probe,
        .remove = __devexit_p(hisik3_wdt_remove),
};

static struct syscore_ops wdt_syscore_ops = {
	.suspend = hisik3_wdt_suspend,
	.resume =  hisik3_wdt_resume ,
};

static int __init hisik3v2_wdt_init(void)
{
	register_syscore_ops(&wdt_syscore_ops);
        return platform_driver_register(&hisik3v2_wdt_driver);
}
module_init(hisik3v2_wdt_init);

static void __exit hisik3v2_wdt_exit(void)
{
	unregister_syscore_ops(&wdt_syscore_ops);
        platform_driver_unregister(&hisik3v2_wdt_driver);
}

module_exit(hisik3v2_wdt_exit);

module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout,
                "Set to 1 to keep watchdog running after device release");

MODULE_AUTHOR("sunhaihuan @ huawei.com>");
MODULE_DESCRIPTION("HISIK3 Watchdog Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
