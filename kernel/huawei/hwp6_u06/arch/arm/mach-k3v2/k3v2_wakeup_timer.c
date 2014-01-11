/*
 * arch/arm/mach-k3v2/k3v2_wakeup_timer.c
 *
 * Timer2 driver module present with k3v2 chipset
 *
 * Copyright (C) 2012  HUAWEI
 * zhangniangao<zhangniangao@huawei.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/hardware/arm_timer.h>
#include <linux/module.h>



#define MODULE_NAME                            "k3v2_wakeup_timer"


/*timing for 1 ms = 32768 */
#define WAKEUP_TIMER_FREQ_32K                  (32768)
/*timer_value = time * freq*/
#define WAKEUP_TIMER_SET_VALUE(x)              ((x) * (WAKEUP_TIMER_FREQ_32K / 1000))

/*timer2 register offsets and masks */
#define WAKEUP_TIMER_DEFAULT_LOAD              (0xFFFFFFFF)
/*max data of int32*/
#define MAX_DATA_OF_32BIT                      (0xFFFFFFFF)
/*max time that timer2 can be timing(milliseconds)*/
#define WAKEUP_TIMER_MAX_TIMING                ((MAX_DATA_OF_32BIT / WAKEUP_TIMER_FREQ_32K) * 1000)

#define SCTRL                                  (REG_BASE_SCTRL + 0x000)
#define TIMEREN_2_OV                           (1<<20)
#define TIMEREN_2_SEL                          (1<<19)
#define TIMEREN_FORCE_HIGH                     (1<<8)

u32 wakeup_timer_seconds;
u32 wakeup_timer_milliseconds;
static bool k3v2_timer_mode_enable;

 /*
 * struct k3v2_wakeup_timer
 *
 * lock: spin lock protecting dev structure and io access
 * base: base address of k3v2_wakeup_timer
 * clk: clock structure of k3v2_wakeup_timer
 * irq: irq number of k3v2_wakeup_timer
 * pdev: platform device structure of k3v2_wakeup_timer
 *
 */
struct k3v2_wakeup_timer {
    spinlock_t                lock;
    void __iomem              *base;
    struct clk                *clk;
    struct clk                *pclk;
    int                       irq;
    struct platform_device    *pdev;
 };

/* local variables */
static struct k3v2_wakeup_timer wakeup_timer;

/*local functions*/
static void k3v2_wakeup_timer_set_time(unsigned int time);
static void k3v2_wakeup_timer_init_config(void);
static int  k3v2_wakeup_timer_clk_get(void);
static int  k3v2_wakeup_timer_pclk_get(void);
static int  k3v2_wakeup_timer_clk_enable(void);
static int  k3v2_wakeup_timer_pclk_enable(void);
static void k3v2_wakeup_timer_clk_disable(void);
static void k3v2_pm_wakeup_on_timer(unsigned int seconds, unsigned int milliseconds);
static void k3v2_wakeup_timer_disable(void);

static void k3v2_wakeup_timer_set_time(unsigned int time)
{
    unsigned long ctrl;

    ctrl = readl(wakeup_timer.base + TIMER_CTRL);

    /*first disable timer*/
    ctrl &= (~TIMER_CTRL_ENABLE);
    writel(ctrl, wakeup_timer.base + TIMER_CTRL);

    /*set TIME_LOAD register together*/
    writel(time, wakeup_timer.base + TIMER_LOAD);

    /*then enable timer again*/
    ctrl |= (TIMER_CTRL_ENABLE | TIMER_CTRL_IE);

    writel(ctrl, wakeup_timer.base + TIMER_CTRL);
}


/*
*func: do timer initializtion
*
*description:
*1. set timer clk to 32.768KHz
*2. if mode is periodic and period is not the same as timer value, set bgload value
*3. set timer control: mode, 32bits, no prescale, interrupt enable, timer enable
*
*/
static void k3v2_wakeup_timer_init_config(void)
{
    unsigned long ctrl = 0;
    unsigned int reg = 0;

    /*1. timer2 select 32.768K. bit20:bit19=00, bit8=1,force timer0123 high*/
    reg  = readl(IO_ADDRESS(SCTRL));
    reg  &= ~(TIMEREN_2_SEL|TIMEREN_2_OV);
    reg |= TIMEREN_FORCE_HIGH;
    writel(reg, IO_ADDRESS(SCTRL));

    /*2. clear the interrupt */
    writel(1, wakeup_timer.base + TIMER_INTCLR);

    /*3. set timer2 control reg: 32bit, interrupt disable, timer_value, oneshot mode and disable wakeup_timer*/
    ctrl = TIMER_CTRL_32BIT;

    writel(WAKEUP_TIMER_DEFAULT_LOAD, wakeup_timer.base + TIMER_LOAD);

    ctrl |= TIMER_CTRL_ONESHOT;

    writel(ctrl, wakeup_timer.base + TIMER_CTRL);
}

static irqreturn_t k3v2_wakeup_timer_interrupt(int irq, void *dev_id)
{
    spin_lock(&wakeup_timer.lock);

    if(false == k3v2_timer_mode_enable)
    {
        /*timer is disabled already*/
        goto out;
    }

    if ((readl(wakeup_timer.base + TIMER_RIS)) & 0x1) {
        /* clear the interrupt */
        writel(1, wakeup_timer.base + TIMER_INTCLR);
    }

out:
    spin_unlock(&wakeup_timer.lock);

    return IRQ_HANDLED;
}


/*
*func: used for wakeup on timer for S/R
*
*decs: set timer2 count value = (seconds*1000+milliseconds)*32.768 ms
*mode is used for user to set timer work in periodic or oneshot mode
*mode: 0 for periodic
*          1 for oneshot
*/
static void k3v2_pm_wakeup_on_timer(unsigned int seconds, unsigned int milliseconds)
{
    unsigned int set_time;
    unsigned long flags;

    if (!seconds && !milliseconds)
    {
        printk("hisk3_pm_wakeup_on_timer: input time error!\n");
        return;
    }

    /*change time to milliseconds format*/
    set_time = 1000 * seconds + milliseconds;

    if (WAKEUP_TIMER_MAX_TIMING < set_time)
    {
        printk("hisk3_pm_wakeup_on_timer: input timing overflow!\n");
        return;
    }

    spin_lock_irqsave(&wakeup_timer.lock, flags);
    /*enable clk*/
    if(k3v2_wakeup_timer_clk_enable())
    {
        printk("hisk3_pm_wakeup_on_timer: clk enable error!\n");
        spin_unlock_irqrestore(&wakeup_timer.lock, flags);
        return;
    }

    /*enable pclk*/
    if(k3v2_wakeup_timer_pclk_enable())
    {
        printk("hisk3_pm_wakeup_on_timer: pclk enable error!\n");
        spin_unlock_irqrestore(&wakeup_timer.lock, flags);
        return;
    }

    /*add for the case ICS4.0 system changed the timer clk to 6.5MHz
    here changed back to 32.768KHz.
    */
    k3v2_wakeup_timer_init_config();

    k3v2_wakeup_timer_set_time((unsigned int)WAKEUP_TIMER_SET_VALUE(set_time));

    /*set timer mode flag to be TRUE*/
    k3v2_timer_mode_enable = true;

    spin_unlock_irqrestore(&wakeup_timer.lock, flags);

    pr_info("PM: Resume timer in %u.%03u secs\n",seconds, milliseconds);
}


static void k3v2_wakeup_timer_disable(void)
{
    unsigned long ctrl, flags;

    spin_lock_irqsave(&wakeup_timer.lock, flags);

    /* clear the interrupt */
    if ((readl(wakeup_timer.base + TIMER_RIS)) & 0x1) {
        writel(1, wakeup_timer.base + TIMER_INTCLR);
    }

    ctrl = readl(wakeup_timer.base + TIMER_CTRL);

    ctrl &= ~(TIMER_CTRL_ENABLE | TIMER_CTRL_IE);

    writel(ctrl, wakeup_timer.base + TIMER_CTRL);

    /*disbale clk*/
    k3v2_wakeup_timer_clk_disable();

    /*set timer mode flag to be FALSE*/
    k3v2_timer_mode_enable = false;

    spin_unlock_irqrestore(&wakeup_timer.lock, flags);
}


static int k3v2_wakeup_timer_clk_get(void)
{
    int ret = 0;

    /*get timer clk*/
    wakeup_timer.clk = clk_get(NULL,"clk_timer1");
    if (IS_ERR(wakeup_timer.clk)) {
        printk("k3v2_wakeup_timer_clk_get: clk not found\n");
        ret = PTR_ERR(wakeup_timer.clk);
        return ret;
    }

    return ret;
}

static int k3v2_wakeup_timer_pclk_get(void)
{
    int ret = 0;

    /*get timer pclk*/
    wakeup_timer.pclk = clk_get(NULL,"clk_pclktimer1");
    if (IS_ERR(wakeup_timer.pclk)) {
        printk("k3v2_wakeup_timer_clk_get: pclk not found\n");
        ret = PTR_ERR(wakeup_timer.pclk);
    }

    return ret;
}

static int k3v2_wakeup_timer_clk_enable(void)
{
    int ret = 0;

    ret = clk_enable(wakeup_timer.clk);
    if (ret) {
        printk("k3v2_wakeup_timer_clk_enable: clk enable fail\n");
        return ret;
    }

    return ret;
}

static int k3v2_wakeup_timer_pclk_enable(void)
{
    int ret = 0;

    ret = clk_enable(wakeup_timer.pclk);
    if (ret) {
        printk("k3v2_wakeup_timer_clk_enable: pclk enable fail\n");
    }

    return ret;
}

static void k3v2_wakeup_timer_clk_disable(void)
{
    clk_disable(wakeup_timer.clk);

    clk_disable(wakeup_timer.pclk);
}

static void k3v2_wakeup_timer_clk_put(void)
{
    clk_put(wakeup_timer.clk);

    clk_put(wakeup_timer.pclk);
}

static int __devinit k3v2_wakeup_timer_probe(struct platform_device *pdev)
{
    int irq, ret = 0;
    struct resource *res;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        ret = -ENOENT;
        dev_warn(&pdev->dev, "memory resource not defined\n");
        goto err;
    }

    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        dev_err(&pdev->dev, "no irq resource?\n");
        goto err;
    }

    if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
        dev_warn(&pdev->dev, "failed to get memory region resource\n");
        ret = -ENOENT;
        goto err;
    }

    if(k3v2_wakeup_timer_clk_get())
    {
        dev_warn(&pdev->dev, "clk get failed\n");
        goto err_clk_get;
    }

    if(k3v2_wakeup_timer_pclk_get())
    {
        dev_warn(&pdev->dev, "pclk get failed\n");
        goto err_pclk_get;
    }

    wakeup_timer.base = ioremap(res->start, resource_size(res));
    if (!wakeup_timer.base) {
        ret = -ENOMEM;
        dev_warn(&pdev->dev, "ioremap fail\n");
        goto err_ioremap;
    }

    spin_lock_init(&wakeup_timer.lock);

    if(k3v2_wakeup_timer_clk_enable())
    {
        dev_warn(&pdev->dev, "clk enable failed\n");
        goto err_clk_enable;
    }

    if(k3v2_wakeup_timer_pclk_enable())
    {
        dev_warn(&pdev->dev, "pclk enable failed\n");
        goto err_pclk_enable;
    }

    wakeup_timer.pdev = pdev;
    wakeup_timer.irq = irq;

    /*do timer init configs: disable timer ,mask interrupt, clear interrupt and set clk to 32.768KHz*/
    k3v2_wakeup_timer_init_config();

    k3v2_timer_mode_enable = false;

    /*register timer2 interrupt*/
    if(request_irq(wakeup_timer.irq, k3v2_wakeup_timer_interrupt, IRQF_NO_SUSPEND, MODULE_NAME, NULL))
    {
        dev_warn(&pdev->dev,"request irq for timer2 error\n");
        goto err_irq;
    }

    /*when init config finished, disable the clk and pclk for timer and enable them when needed.*/
    k3v2_wakeup_timer_clk_disable();

    dev_warn(&pdev->dev,"probing has been finished\n");

    return 0;

err_irq:
    clk_disable(wakeup_timer.pclk);
err_pclk_enable:
    clk_disable(wakeup_timer.clk);
err_clk_enable:
    iounmap(wakeup_timer.base);
err_ioremap:
    clk_put(wakeup_timer.pclk);
err_pclk_get:
    clk_put(wakeup_timer.clk);
err_clk_get:
    wakeup_timer.clk = NULL;
    wakeup_timer.pclk = NULL;
    release_mem_region(res->start, resource_size(res));
err:
    dev_warn(&pdev->dev, "probe failed!\n");
    return ret;
}

static int __devexit k3v2_wakeup_timer_remove(struct platform_device *pdev)
{
    struct resource *res;

    iounmap(wakeup_timer.base);
    k3v2_wakeup_timer_clk_disable();
    k3v2_wakeup_timer_clk_put();
    free_irq(wakeup_timer.irq, NULL);
    wakeup_timer.clk = NULL;
    wakeup_timer.pclk = NULL;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res)
        release_mem_region(res->start, resource_size(res));

    return 0;
}

static  int k3v2_wakeup_timer_suspend(struct platform_device *pdev, pm_message_t state)
{
    /* Wakeup timer from suspend */
    if (wakeup_timer_seconds || wakeup_timer_milliseconds){
        k3v2_pm_wakeup_on_timer(wakeup_timer_seconds,wakeup_timer_milliseconds);
    }
    return 0;
}

static  int k3v2_wakeup_timer_resume(struct platform_device *pdev)
{
    if(k3v2_timer_mode_enable){
        k3v2_wakeup_timer_disable();
        dev_info(&pdev->dev, "k3v2_wakeup_timer_disable\n");
    }

    return 0;
}

static struct platform_driver hisik3v2_timer2_driver = {
    .driver = {
        .name = MODULE_NAME,
        .owner  = THIS_MODULE,
    },
    .probe = k3v2_wakeup_timer_probe,
    .remove = __devexit_p(k3v2_wakeup_timer_remove),
    .suspend = k3v2_wakeup_timer_suspend,
    .resume = k3v2_wakeup_timer_resume,
};


static int __init hisik3v2_timer2_init(void)
{
    return platform_driver_register(&hisik3v2_timer2_driver);
}
arch_initcall(hisik3v2_timer2_init);

static void __exit hisik3v2_timer2_exit(void)
{
    platform_driver_unregister(&hisik3v2_timer2_driver);
}
module_exit(hisik3v2_timer2_exit);


MODULE_AUTHOR("zhangniangao@huawei.com>");
MODULE_DESCRIPTION("K3V2 WAKEUP TIMER DRIVER");
MODULE_LICENSE("GPL");
