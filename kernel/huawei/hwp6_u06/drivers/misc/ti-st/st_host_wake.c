/*
 *  ST Host Wakeup Driver - 
	For protocols registered over Shared Transport
 *  Copyright (C) 2011-2012 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */
#define DEBUG
#include <linux/platform_device.h>
#include <net/bluetooth/bluetooth.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <linux/slab.h>

#include <linux/ti_wilink_st.h>
#include <linux/module.h>

#include <linux/mux.h>
#include <linux/wakelock.h>

struct iomux_block *btuart4_block;
struct block_config *btuart4_config;
static struct wake_lock wake_host_lock;
static spinlock_t irq_enable_lock; /*Lock for irq enable state transitions */
static bool suspend_flag = false;

#define BTUART4_GPIO_IOMUX_BLOCK "block_uart4"

/* Bluetooth Driver Version */
#define VERSION               "1.0"


/* state variable names and bit positions */
#define FLAG_RESET		0x00
#define ST_HOST_WAKE	0x01
#define IRQ_WAKE		0x02

#define CHANNEL_ACL		0x02
#define CHANNEL_EVT		0x04

struct st_host_wake_info {
	unsigned host_wake_irq;
};

/*
 * Global variables
 */
static char  dev_id[12] ="st_host_wake";
static struct st_host_wake_info *hwi;

/* module usage */


/** Global state flags */
static unsigned long flags;

/**
 * interrupt handler for hostwake
 * <code>HOST_WAKE</code> GPIO pin.
 * @param irq Not used.
 * @param dev_id Not used.
 */
static irqreturn_t st_host_wake_isr(int irq, void *dev_id)
{
	unsigned long irq_flags = 0;

	pr_info("%s", __func__);

	spin_lock_irqsave(&irq_enable_lock, irq_flags);
	if(suspend_flag)
	{
		suspend_flag = false;
		wake_lock_timeout(&wake_host_lock, (5 * HZ)); //wake for 5s.
	}
	spin_unlock_irqrestore(&irq_enable_lock, irq_flags);

	return IRQ_HANDLED;
}

/**
 * Handles ST registration events.
 * @param chan_id Channel id of the protocol registered. 0 for unregister case.
 * @param reg_state If the protocol registers or un-registers.
 * @return <code>NOTIFY_DONE</code>.
 */
void st_host_wake_notify(int chan_id, int reg_state)
{
	/* HOST_WAKE flag should be set only after all BT channels
	 * including CHANNEL_SCO is registered 
	 */
	if(chan_id == CHANNEL_ACL || chan_id == CHANNEL_EVT)
		return;

	switch(reg_state) {
		case ST_PROTO_REGISTERED:
			pr_info("Channel %d registered", chan_id);
			set_bit(ST_HOST_WAKE, &flags);
			pr_info("ST_HOST_WAKE set");
			break;
		
		case ST_PROTO_UNREGISTERED:
			pr_info("All channels un-registered", chan_id);
			if(hwi && test_bit(IRQ_WAKE, &flags)) {
				pr_info("disabling wake_irq after un-register");
#ifdef IRQ_WAKEUP
				ret = disable_irq_wake(hwi->host_wake_irq);
				if (ret < 0)    {
					pr_err("%s: couldn't disable ST_HOST_WAKE as wakeup interrupt %d", __func__, ret);
				}

#endif
				clear_bit(IRQ_WAKE, &flags);
			}
			clear_bit(ST_HOST_WAKE, &flags);
			pr_info("HOST_WAKE cleared");
			break;

		default:
			break;
	}

	return;
}
EXPORT_SYMBOL_GPL(st_host_wake_notify);

static int st_host_wake_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;

	pr_info("TI Host Wakeup Driver - Version %s", VERSION);

	hwi = kzalloc(sizeof(struct st_host_wake_info), GFP_KERNEL);
	if (!hwi)
		return -ENOMEM;

	res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
			"host_wake");
	if (!res) {
		pr_info("couldn't find host_wake irq\n");
		ret = -ENODEV;
		goto free_hwi;
	}

	btuart4_block = iomux_get_block(BTUART4_GPIO_IOMUX_BLOCK);
	if (IS_ERR(btuart4_block)) {
		pr_info("%s	: iomux_get_block btuart4_block failed\n", __func__);
		return -ENXIO;
	}
	btuart4_config = iomux_get_blockconfig(BTUART4_GPIO_IOMUX_BLOCK);
	if (IS_ERR(btuart4_config)) {
		pr_info("%s	: iomux_get_blockconfig btuart4_config:\n", __func__);
		return -ENXIO;
	}
	ret = blockmux_set(btuart4_block, btuart4_config, NORMAL);
	if (ret) {
		pr_info("%s	: btuart4_block blockmux set err %d\r\n", __func__, ret);
		return ret;
	}
	gpio_request(res->start, NULL);
	gpio_direction_input(res->start);

	hwi->host_wake_irq = gpio_to_irq(res->start);
	pr_info(" host_wake irq  %d",hwi->host_wake_irq);
	if (hwi->host_wake_irq < 0) {
		pr_info("couldn't find host_wake irq");
		ret = -ENODEV;
		goto free_hwi;
	}
	/* Initialize spinlock.*/
	spin_lock_init(&irq_enable_lock);

	wake_lock_init(&wake_host_lock, WAKE_LOCK_SUSPEND, "st_host_wake");

	if (res->flags & IORESOURCE_IRQ_LOWEDGE)
		ret = request_irq(hwi->host_wake_irq, st_host_wake_isr,
				 IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
				"bluetooth hostwake", dev_id);
	else
		ret = request_irq(hwi->host_wake_irq, st_host_wake_isr,
				  IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
				"bluetooth hostwake", dev_id);

	if (ret < 0) {
		pr_info("Couldn't acquire ST_HOST_WAKE IRQ");
		goto free_hwi;
	}

	clear_bit(ST_HOST_WAKE, &flags);
	clear_bit(IRQ_WAKE, &flags);
	goto finish;	

free_hwi:
	kfree(hwi);
finish:
	return ret;
	
}

static int st_host_wake_remove(struct platform_device *pdev)
{
	int ret;
	unsigned long irq_flags = 0;

	pr_info("%s", __func__);

	if (hwi->host_wake_irq) {
#ifdef IRQ_WAKEUP
		ret = disable_irq_wake(hwi->host_wake_irq);
		if (ret < 0)	{
			pr_err("%s: couldn't disable ST_HOST_WAKE as wakeup interrupt %d", __func__, ret);
		}

#endif
		free_irq(hwi->host_wake_irq, dev_id);
	}
	kfree(hwi);
	return 0;

}

static int st_host_wake_resume(struct platform_device *pdev)
{
	int ret;
	unsigned long irq_flags = 0;

	pr_err("%s", __func__);

	if (test_bit(ST_HOST_WAKE, &flags) && test_bit(IRQ_WAKE, &flags)) {
		ret = blockmux_set(btuart4_block, btuart4_config, NORMAL);
		if (ret) {
			pr_info("%s: btuart4_block blockmux set err %d\r\n", __func__, ret);
			return ret;
		}

#ifdef IRQ_WAKEUP
		ret = disable_irq_wake(hwi->host_wake_irq);
		if (ret < 0)	{
			pr_err("%s: couldn't disable ST_HOST_WAKE as wakeup interrupt %d", __func__, ret);
		}
#endif
		clear_bit(IRQ_WAKE, &flags);
	}

	return 0;
}

int st_host_wake_suspend(void)
{
	int ret;
	unsigned long irq_flags = 0;

	pr_info("%s", __func__);

	if (test_bit(ST_HOST_WAKE, &flags) && (!test_bit(IRQ_WAKE, &flags))) {

		//enable_irq(hwi->host_wake_irq);
		ret = blockmux_set(btuart4_block, btuart4_config, LOWPOWER);
		if (ret) {
			pr_info("%s	: btuart4_block blockmux set err %d\r\n", __func__, ret);
			return ret;
		}

		spin_lock_irqsave(&irq_enable_lock, irq_flags);
		suspend_flag = true;
		spin_unlock_irqrestore(&irq_enable_lock, irq_flags);

#ifdef IRQ_WAKEUP
		ret = enable_irq_wake(hwi->host_wake_irq);
		if (ret < 0)	{
			pr_err("%s: couldn't enable	ST_HOST_WAKE as	wakeup interrupt %d", __func__, ret);
		}

#endif
		set_bit(IRQ_WAKE, &flags);
		pr_info("enabled the host_wake irq");
	}

	return 0;

}

static struct platform_driver st_host_wake_driver = {
	.probe = st_host_wake_probe,
	.remove = st_host_wake_remove,
	.suspend = st_host_wake_suspend,
	.resume = st_host_wake_resume,
	.driver = {
		.name = "st_host_wake",
		.owner = THIS_MODULE,
	},
};

/* ------- Module Init/Exit interfaces ------ */
static int __init st_host_wake_init(void)
{
	int retval;

	retval = platform_driver_register(&st_host_wake_driver);
	if (retval)
		goto fail;

	if (hwi == NULL)
		return 0;

	flags = FLAG_RESET; /* clear all status bits */

	return 0;
fail:
	return retval;


}

static void __exit st_host_wake_exit(void)
{

	if (hwi == NULL)
		return;
	platform_driver_unregister(&st_host_wake_driver);
}

module_init(st_host_wake_init);
module_exit(st_host_wake_exit);

/* ------ Module Info ------ */

MODULE_AUTHOR("Deepa Dhakappa<xdeepadh@ti.com>");
MODULE_DESCRIPTION("TI ST Host Wakeup Driver[Ver %s]" VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
