/*

   This	program	is free	software; you can redistribute it and/or modify
   it under	the	terms of the GNU General Public	License	version	2 as
   published by	the	Free Software Foundation.

   You should have received	a copy of the GNU General Public License
   along with this program;	if not,	write to the Free Software
   Foundation, Inc., 59	Temple Place, Suite	330, Boston, MA	 02111-1307	 USA

   This	program	is distributed in the hope that	it will	be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A	PARTICULAR PURPOSE.	 See the GNU General Public	License
   for more	details.
 */

#include <linux/module.h>	/* kernel module definitions */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>

#include <linux/irq.h>
#include <linux/param.h>
#include <linux/bitops.h>
#include <linux/termios.h>
#include <linux/wakelock.h>
#include <mach/gpio.h>


#include <linux/mux.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>	/* event notifications */
#include "hci_uart.h"

/*
 * Defines
 */

#define BT_SLEEP_DBG
#ifndef	BT_SLEEP_DBG
#define	BT_DBG(fmt,	arg...)
#endif

#define	VERSION		"1.1"
#define	PROC_DIR	"bluetooth/sleep"
#define	CONFIG_HUAWEI_KERNEL
/*config the bt enable/reset/bt wakeup/host wakeup io*/
#define	BT_PM_IOMUX_BLOCK	"block_btpm"
#define BT_LPM_SUPPORT_BTLD 1
/*#define IRQ_WAKEUP*/

struct bluesleep_info {
	unsigned host_wake;
	unsigned ext_wake;
	int host_wake_irq;
	unsigned long flags; /*Global state flags */
	spinlock_t rw_lock; /*Lock for state transitions */
	struct semaphore bt_seam;
	struct tasklet_struct hostwake_task; /*Tasklet to respond to change in hostwake line */
	struct timer_list tx_timer; /* Transmission check timer */
	struct iomux_block	*blockTemp;
	struct block_config	*configTemp;
	struct proc_dir_entry *bluetooth_dir;
	struct proc_dir_entry *sleep_dir;
	struct hci_dev *bluesleep_hdev; /* global pointer to a single hci device. */
	struct uart_port *uport;
	struct wake_lock wake_lock;
};

/* work	function */
static void	bluesleep_sleep_work(struct	work_struct	*work);

/* work	queue */
DECLARE_DELAYED_WORK(sleep_workqueue, bluesleep_sleep_work);

/* Macros for handling sleep work */
#define	bluesleep_rx_busy() schedule_delayed_work(&sleep_workqueue,	0)
#define	bluesleep_tx_busy()	schedule_delayed_work(&sleep_workqueue,	0)
#define	bluesleep_rx_idle()	schedule_delayed_work(&sleep_workqueue,	0)
#define	bluesleep_tx_idle()	schedule_delayed_work(&sleep_workqueue,	0)

/* 1 second	timeout	*/
#define	TX_TIMER_INTERVAL	1

/* state variable names	and	bit	positions */
#define	BT_PROTO	0x01
#define	BT_TXDATA	0x02
#define	BT_ASLEEP	0x04

/*
 * Local function prototypes
 */

static int bluesleep_hci_event(struct notifier_block *this,
				unsigned long event, void *data);

/*
 * Global variables
 */

static struct bluesleep_info *bsi;

/**	Notifier block for HCI events */
struct notifier_block hci_event_nblock = {
	.notifier_call = bluesleep_hci_event,
};


/*
 * Local functions
 */
#ifndef BT_LPM_SUPPORT_BTLD
static void	hsuart_power(int on)
{
	if (NULL == bsi->uport) {
		BT_ERR("%s: bsi->uport is null\n", __func__);
		return;
	}

	if (NULL == bsi->uport->ops->set_mctrl) {
		BT_ERR("%s: bsi->uport->ops->set_mctrl is NULL\n", __func__);
		return;
	}
	if (on)	{
		BT_DBG("hsuart_power wake up ===>\n");
		/*hs_request_clock_on(bsi->uport);*/
		bsi->uport->ops->set_mctrl(bsi->uport, TIOCM_RTS);
	} else {
		BT_DBG("hsuart_power go	sleep ===>\n");
		bsi->uport->ops->set_mctrl(bsi->uport, 0);
		/*hs_request_clock_off(bsi->uport); */
	}
}
#else
static void	hsuart_power(int on) {}
#endif

/**
 * @return 1 if	the	Host can go to sleep, 0 otherwise.
 */
static inline int bluesleep_can_sleep(void)
{
#if BT_LPM_SUPPORT_BTLD
    /* check if gpio ext_wake and gpio host_wake are both deasserted */
	return gpio_get_value(bsi->ext_wake) &&
		gpio_get_value(bsi->host_wake);
#else
    /* check if gpio ext_wake and gpio host_wake are both deasserted */
	return gpio_get_value(bsi->ext_wake) &&
		gpio_get_value(bsi->host_wake) &&
		(bsi->uport	!= NULL);
#endif
}
void bluesleep_sleep_wakeup(void)
{
	if (test_bit(BT_ASLEEP,	&bsi->flags)) {
		printk("bt waking up...\n");
		/* Start the timer */
		mod_timer(&bsi->tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
		gpio_set_value(bsi->ext_wake, 0);
		clear_bit(BT_ASLEEP, &bsi->flags);
		wake_lock(&bsi->wake_lock);
		/*Activating UART */
		hsuart_power(1);
	}
#ifdef	CONFIG_HUAWEI_KERNEL
	else {
	/*Tx idle, Rx	busy, we must also make	host_wake asserted,	that is	low
	* 1 means bt chip can sleep, in bluesleep.c
	*/
	/* Here we depend on the status of gpio ext_wake, for stability	*/
		if (1 == gpio_get_value(bsi->ext_wake)) {
			BT_DBG("bluesleep_sleep_wakeup wakeup bt chip\n");
			gpio_set_value(bsi->ext_wake, 0);
			mod_timer(&bsi->tx_timer, jiffies +	(TX_TIMER_INTERVAL * HZ));
		}
	}
#endif

}

/**
 * @brief@	main sleep work	handling function which	update the flags
 * and activate	and	deactivate UART	,check FIFO.
 */
static void	bluesleep_sleep_work(struct	work_struct	*work)
{
#ifndef BT_LPM_SUPPORT_BTLD
	if (NULL == bsi->uport) {
		BT_ERR("%s: bsi->uport is null\n", __func__);
		return;
	}
	if (NULL == bsi->uport->ops->tx_empty) {
		BT_ERR("%s: bsi->uport->ops->tx_empty is NULL\n", __func__);
		return ;
	}
#endif
	if (bluesleep_can_sleep()) {
		if (test_bit(BT_ASLEEP,	&bsi->flags)) {
			BT_DBG("already	asleep");
			return;
		}
#ifndef BT_LPM_SUPPORT_BTLD
		if (bsi->uport->ops->tx_empty(bsi->uport)) {
			printk("bt going to sleep...\n");
			set_bit(BT_ASLEEP, &bsi->flags);
			/*Deactivating UART	*/
			hsuart_power(0);
			wake_lock_timeout(&bsi->wake_lock, HZ/2);

		} else {
			mod_timer(&bsi->tx_timer, jiffies + (TX_TIMER_INTERVAL	* HZ));
			return;
		}
#else
			printk("bt going to sleep...\n");
			set_bit(BT_ASLEEP, &bsi->flags);
			wake_lock_timeout(&bsi->wake_lock, HZ/2);
#endif
	} else {
		bluesleep_sleep_wakeup();
	}
}
/**
 * A tasklet function that runs	in tasklet context and reads the value
 * of the HOST_WAKE	GPIO pin and further defer the work.
 * @param data Not used.
 */
static void	bluesleep_hostwake_task(unsigned long data)
{
	struct bluesleep_info *bs_info = (struct bluesleep_info *)data;
	if (!bs_info) {
		BT_ERR("%s: bs_info is NULL\n", __func__);
		return ;
	}
	BT_DBG("hostwake line change");

	spin_lock(&bs_info->rw_lock);

	if (!gpio_get_value(bs_info->host_wake))
		bluesleep_rx_busy();
	else
		bluesleep_rx_idle();

	spin_unlock(&bs_info->rw_lock);
}

/**
 * Handles proper timer	action when	outgoing data is delivered to the
 * HCI line	discipline.	Sets BT_TXDATA.
 */
static void	bluesleep_outgoing_data(struct bluesleep_info *bs_info)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&bs_info->rw_lock, irq_flags);
	/* log data	passing	by */
	set_bit(BT_TXDATA, &bs_info->flags);

	/* if the tx side is sleeping... */
	if (gpio_get_value(bs_info->ext_wake)) {

		BT_DBG("tx was sleeping");
		bluesleep_sleep_wakeup();
	}

	spin_unlock_irqrestore(&bs_info->rw_lock, irq_flags);
}

/**
 * Handles HCI device events.
 * @param this Not used.
 * @param event	The	event that occurred.
 * @param data The HCI device associated with the event.
 * @return <code>NOTIFY_DONE</code>.
 */
static int bluesleep_hci_event(struct notifier_block *this,
				unsigned long event, void *data)
{
	struct hci_dev *hdev = (struct hci_dev *) data;
	struct hci_uart	*hu;
	struct uart_state *state;

	if (!hdev) {
		BT_ERR("%s: bluesleep_hci_event hci_dev null\n",
				__func__);
		return NOTIFY_DONE;
	}

	switch (event) {
	case HCI_DEV_REG:
		if (!bsi->bluesleep_hdev) {
			bsi->bluesleep_hdev = hdev;
			hu	= (struct hci_uart *) hdev->driver_data;
			state =	(struct	uart_state *) hu->tty->driver_data;
			bsi->uport = state->uart_port;
		}
		break;

	case HCI_DEV_UNREG:
		bsi->bluesleep_hdev = NULL;
		bsi->uport = NULL;
		break;

	case HCI_DEV_WRITE:
		bluesleep_outgoing_data(bsi);
		break;

	default:
		break;
	}

	return NOTIFY_DONE;
}

/**
 * Handles transmission	timer expiration.
 * @param data Not used.
 */
static void	bluesleep_tx_timer_expire(unsigned long	data)
{
	unsigned long irq_flags = 0;
	struct bluesleep_info *bs_info = (struct bluesleep_info *)data;

	if (NULL == bs_info) {
		BT_ERR("%s: bluesleep_info is null\n", __func__);
		return;
	}

	spin_lock_irqsave(&bs_info->rw_lock, irq_flags);

	BT_DBG("Tx timer expired");

	/* were	we silent during the last timeout? */
	if (!test_bit(BT_TXDATA, &bs_info->flags)) {
		BT_DBG("Tx has been	idle");
		gpio_set_value(bs_info->ext_wake, 1);
		bluesleep_tx_idle();
	} else {
		BT_DBG("Tx data	during last	period");
		mod_timer(&bs_info->tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));
	}

	/* clear the incoming data flag	*/
#ifndef BT_LPM_SUPPORT_BTLD
	clear_bit(BT_TXDATA, &bs_info->flags);
#endif
	spin_unlock_irqrestore(&bs_info->rw_lock, irq_flags);
}

/**
 * Schedules a tasklet to run when receiving an	interrupt on the
 * <code>HOST_WAKE</code> GPIO pin.
 * @param irq Not used.
 * @param dev_id Not used.
 */
static irqreturn_t bluesleep_hostwake_isr(int irq, void	*dev_id)
{
	/* schedule	a tasklet to handle	the	change
	* in the host wake	line
	*/
	struct bluesleep_info *bs_info = (struct bluesleep_info *)dev_id;

	BT_DBG("bluesleep_hostwake_isr===>>\n");
	tasklet_schedule(&bs_info->hostwake_task);
	return IRQ_HANDLED;
}

/**
 * Starts the Sleep-Mode Protocol on the Host.
 * @return On success, 0. On error,	-1,	and	<code>errno</code> is set
 * appropriately.
 */
static int bluesleep_start(struct bluesleep_info *bs_info)
{
	int	retval = 0;

	down(&bs_info->bt_seam);
	if (test_bit(BT_PROTO, &bs_info->flags))	{
		BT_ERR("%s: bluesleep already start\n", __func__);
		up(&bs_info->bt_seam);
		return retval;
	}

	printk("%s, bluetooth power manage unit start +.\n", __func__);

#ifdef	CONFIG_MACH_K3V2OEM1
	/*SET NORMAL*/
	retval = blockmux_set(bs_info->blockTemp, bs_info->configTemp, NORMAL);
	if (unlikely(retval)) {
		BT_ERR("%s: gpio blockmux set err %d\n",
			__func__, retval);
		up(&bs_info->bt_seam);
		return retval;
	}

#endif

#ifndef	CONFIG_HUAWEI_KERNEL
	retval = request_irq(bs_info->host_wake_irq, bluesleep_hostwake_isr,
			IRQF_DISABLED |	IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND,
				"bluetooth hostwake", bs_info);
#else
	retval = request_irq(bs_info->host_wake_irq, bluesleep_hostwake_isr,
			IRQF_DISABLED |	IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING
				| IRQF_NO_SUSPEND, "bluetooth hostwake", bs_info);
#endif
	if (retval	< 0) {
		BT_ERR("%s: couldn't acquire BT_HOST_WAKE IRQ", __func__);
		goto err_request_irq;
	}

#ifdef IRQ_WAKEUP
	retval = enable_irq_wake(bs_info->host_wake_irq);
	if (retval < 0)	{
		BT_ERR("%s: couldn't enable	BT_HOST_WAKE as	wakeup interrupt %d", __func__, retval);
		goto err_en_irqwake;
	}
#endif

	/* assert BT_WAKE */
	gpio_set_value(bs_info->ext_wake, 0);

	/* start the timer */
	mod_timer(&bs_info->tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));

	set_bit(BT_PROTO, &bs_info->flags);

	wake_lock(&bsi->wake_lock);

	up(&bs_info->bt_seam);
	printk("%s, bluetooth power manage unit start -.\n", __func__);

	return retval;

#ifdef IRQ_WAKEUP
err_en_irqwake:
	free_irq(bs_info->host_wake_irq, bs_info);
#endif
err_request_irq:
#ifdef	CONFIG_MACH_K3V2OEM1
	if (blockmux_set(bs_info->blockTemp, bs_info->configTemp, LOWPOWER))
		BT_ERR("%s: blockmux_set failed. \n", __func__);
#endif

	up(&bs_info->bt_seam);
	return retval;
}

/**
 * Stops the Sleep-Mode	Protocol on	the	Host.
 */
static void	bluesleep_stop(struct bluesleep_info *bs_info)
{

#ifdef CONFIG_MACH_K3V2OEM1
	int	retval = 0;
#endif

	down(&bs_info->bt_seam);

	if (!test_bit(BT_PROTO,	&bs_info->flags)) {
		up(&bs_info->bt_seam);
		BT_ERR("%s: bluesleep already stop\n", __func__);
		return;
	}

	printk("%s, bluetooth power manage unit stop +.\n", __func__);

	/* assert BT_WAKE */
	gpio_set_value(bs_info->ext_wake, 0);

	cancel_delayed_work_sync(&sleep_workqueue);
	del_timer_sync(&bs_info->tx_timer);

	if (test_bit(BT_ASLEEP,	&bs_info->flags)) {
		clear_bit(BT_ASLEEP, &bs_info->flags);
		hsuart_power(1);
	}


#ifdef IRQ_WAKEUP
	if (disable_irq_wake(bs_info->host_wake_irq))
		BT_ERR("%s: couldn't disable hostwake IRQ wakeup mode\n", __func__);
#endif
	if (bs_info->host_wake_irq)
		free_irq(bs_info->host_wake_irq, bs_info);

#ifdef	CONFIG_MACH_K3V2OEM1
	/*SET LOWPOWER*/
	retval = blockmux_set(bs_info->blockTemp,
					bs_info->configTemp, LOWPOWER);
	if (unlikely(retval))
		BT_ERR("%s:	gpio blockmux set err %d\n",
			__func__, retval);
#endif

	clear_bit(BT_PROTO,	&bs_info->flags);

	wake_unlock(&bsi->wake_lock);

	up(&bs_info->bt_seam);
	printk("%s, bluetooth power manage unit stop -.\n", __func__);


}
/**
 * Read	the	<code>BT_WAKE</code> GPIO pin value	via	the	proc interface.
 * When	this function returns, <code>page</code> will contain a	1 if the
 * pin is high,	0 otherwise.
 * @param page Buffer for writing data.
 * @param start	Not	used.
 * @param offset Not used.
 * @param count	Not	used.
 * @param eof Whether or not there is more data	to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluepower_read_proc_btwake(char *page, char **start,	off_t offset,
					int	count, int *eof, void *data)
{
	struct bluesleep_info *bs_info = (struct bluesleep_info *)data;

	*eof = 1;
	return sprintf(page, "btwake:%u\n",
				gpio_get_value(bs_info->ext_wake));
}
#ifdef BT_LPM_SUPPORT_BTLD
static int bluesleep_ap_wakeup_bt(struct file *file, const	char *buffer,
					unsigned long count, void *data)
{
	char buf;
	struct bluesleep_info *bs_info = (struct bluesleep_info *)data;
	if (count < 1 || (NULL == bs_info))
		return -EINVAL;


	if (copy_from_user(&buf, buffer, sizeof(buf)))
		return -EFAULT;

	if (buf == '0') {
		spin_lock(&bs_info->rw_lock);
		clear_bit(BT_TXDATA, &bs_info->flags);
		spin_unlock(&bs_info->rw_lock);
	} else
			bluesleep_outgoing_data(bs_info);

	return count;
}

static int bluesleep_read_bt_state(char *page,	char **start,
			off_t	offset,	int	count, int *eof, void *data)
{
	*eof = 1;
	return sprintf(page, "unsupported to read\n");
}
#endif
/**
 * Write the <code>BT_WAKE</code> GPIO pin value via the proc interface.
 * @param file Not used.
 * @param buffer The buffer	to read	from.
 * @param count	The	number of bytes	to be written.
 * @param data Not used.
 * @return On success, the number of bytes written.	On error, -1, and
 * <code>errno</code> is set appropriately.
 */
static int bluepower_write_proc_btwake(struct file *file, const	char *buffer,
					unsigned long count, void *data)
{
	char buf;
	struct bluesleep_info *bs_info = (struct bluesleep_info *)data;

	if ((count < 1) || (NULL == bs_info))
		return -EINVAL;


	if (copy_from_user(&buf, buffer, sizeof(buf)))
		return -EFAULT;

	if (buf == '0') {
		gpio_set_value(bs_info->ext_wake, 0);
	} else if (buf == '1') {
		gpio_set_value(bs_info->ext_wake, 1);
	} else {
		BT_ERR("%s: bluepower_write_proc_btwake error %d\n",
			__func__, buf);
		return -EINVAL;
	}

	return count;
}

/**
 * Read	the	<code>BT_HOST_WAKE</code> GPIO pin value via the proc interface.
 * When	this function returns, <code>page</code> will contain a	1 if the pin
 * is high,	0 otherwise.
 * @param page Buffer for writing data.
 * @param start	Not	used.
 * @param offset Not used.
 * @param count	Not	used.
 * @param eof Whether or not there is more data	to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluepower_read_proc_hostwake(char *page,	char **start,
			off_t	offset,	int	count, int *eof, void *data)
{
	struct bluesleep_info *bs_info = (struct bluesleep_info *)data;

	*eof = 1;
	return sprintf(page, "hostwake:	%u\n",
				gpio_get_value(bs_info->host_wake));
}


/**
 * Read	the	low-power status of	the	Host via the proc interface.
 * When	this function returns, <code>page</code> contains a	1 if the Host
 * is asleep, 0	otherwise.
 * @param page Buffer for writing data.
 * @param start	Not	used.
 * @param offset Not used.
 * @param count	Not	used.
 * @param eof Whether or not there is more data	to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluesleep_read_proc_asleep(char *page, char **start,	off_t offset,
					int	count, int *eof, void *data)
{
	unsigned int asleep;
	struct bluesleep_info *bs_info = (struct bluesleep_info *)data;

	asleep = test_bit(BT_ASLEEP, &bs_info->flags) ? 1 : 0;
	*eof = 1;
	return sprintf(page, "asleep: %u\n", asleep);
}

/**
 * Read	the	low-power protocol being used by the Host via the proc interface.
 * When	this function returns, <code>page</code> will contain a	1 if the Host
 * is using	the	Sleep Mode Protocol, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start	Not	used.
 * @param offset Not used.
 * @param count	Not	used.
 * @param eof Whether or not there is more data	to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluesleep_read_proc_proto(char *page, char **start, off_t offset,
					int	count, int *eof, void *data)
{
	unsigned int proto;
	struct bluesleep_info *bs_info = (struct bluesleep_info *)data;

	proto =	test_bit(BT_PROTO, &bs_info->flags) ? 1 : 0;
	*eof = 1;
	return sprintf(page, "%u\n", proto);
}

/**
 * Modify the low-power	protocol used by the Host via the proc interface.
 * @param file Not used.
 * @param buffer The buffer	to read	from.
 * @param count	The	number of bytes	to be written.
 * @param data Not used.
 * @return On success, the number of bytes written.	On error, -1, and
 * <code>errno</code> is set appropriately.
 */
static int bluesleep_write_proc_proto(struct file *file, const char	*buffer,
					unsigned long count, void *data)
{
	char proto = 0;
	struct bluesleep_info *bs_info = (struct bluesleep_info *)data;

    if ((count < 1) || (NULL == bs_info))
		return -EINVAL;

	if (copy_from_user(&proto, buffer, sizeof(proto)))
		return -EFAULT;

	if (proto == '0')
		bluesleep_stop(bs_info);
	else if (proto == '1')
		bluesleep_start(bs_info);
	else
		BT_ERR("bluesleep_write_proc_proto error %d\n",
			proto);

	/* claim that we wrote everything */
	return count;
}

#ifdef CONFIG_MACH_K3V2OEM1

/* Initialize the bluetooth io mux  */
static int	bluesleep_io_init(struct bluesleep_info *bs_info)
{

	int	ret	= 0;

	bs_info->blockTemp	= iomux_get_block(BT_PM_IOMUX_BLOCK);
	if (IS_ERR(bs_info->blockTemp)) {
		BT_ERR("%s:	iomux_get_block	blockTemp failed\n", __func__);
		ret = -ENXIO;
		return ret;
	}

	bs_info->configTemp = iomux_get_blockconfig(BT_PM_IOMUX_BLOCK);
	if (IS_ERR(bs_info->configTemp)) {
		BT_ERR("%s:iomux_get_blockconfig failed\n", __func__);
		ret = -ENXIO;
		goto err_block_config;
	}

	ret	= blockmux_set(bs_info->blockTemp,
				bs_info->configTemp, LOWPOWER);
	if (ret) {
		BT_ERR("%s: gpio_blockTemp blockmux err	%d\n",
			__func__, ret);
		goto err_mux_set;
	}

	return ret;

err_mux_set:
	if (bs_info->configTemp)
		bs_info->configTemp = NULL;
err_block_config:
	if (bs_info->blockTemp)
		bs_info->blockTemp	= NULL;

	return ret;

}

#endif


static int create_bluesleep_proc_sys(struct bluesleep_info *bs_info)
{

	int	retval = 0;
	struct proc_dir_entry *ent;

	bs_info->bluetooth_dir = proc_mkdir("bluetooth", NULL);
	if (bs_info->bluetooth_dir == NULL) {
		BT_ERR("%s: unable to create /proc/bluetooth directory", __func__);
		retval = -ENOMEM;
		return retval;
	}

	bs_info->sleep_dir = proc_mkdir("sleep", bs_info->bluetooth_dir);
	if (bs_info->sleep_dir == NULL) {
		BT_ERR("%s: unable to create /proc/%s directory", __func__, PROC_DIR);
		retval = -ENOMEM;
		goto err_proc_sleep;
	}

	/* Creating read/write "btwake" entry */
	ent = create_proc_entry("btwake", 0, bs_info->sleep_dir);
	if (ent == NULL) {
		BT_ERR("%s: unable to create /proc/%s/btwake entry", __func__, PROC_DIR);
		retval = -ENOMEM;
		goto err_proc_btwake;
	}

	ent->read_proc = bluepower_read_proc_btwake;
	ent->write_proc = bluepower_write_proc_btwake;
	ent->data = bs_info;

	/* read only proc entries */
	if (create_proc_read_entry("hostwake", 0, bs_info->sleep_dir,
			bluepower_read_proc_hostwake, bs_info) == NULL) {
		BT_ERR("%s: unable to create /proc/%s/hostwake entry", __func__, PROC_DIR);
		retval = -ENOMEM;
		goto err_proc_hostwake;
	}

	/* read/write proc entries */
	ent = create_proc_entry("proto", 0, bs_info->sleep_dir);
	if (ent == NULL) {
		BT_ERR("%s: unable to create /proc/%s/proto entry", __func__, PROC_DIR);
		retval = -ENOMEM;
		goto err_proc_proto;
	}
	ent->read_proc = bluesleep_read_proc_proto;
	ent->write_proc = bluesleep_write_proc_proto;
	ent->data = bs_info;

	/* read only proc entries */
	if (create_proc_read_entry("asleep", 0,
			bs_info->sleep_dir, bluesleep_read_proc_asleep,
			bs_info) == NULL) {
		BT_ERR("%s: unable to create /proc/%s/asleep entry", __func__, PROC_DIR);
		retval = -ENOMEM;
		goto err_proc_asleep;
	}
#ifdef BT_LPM_SUPPORT_BTLD
	ent = create_proc_entry("btwrite", S_IFREG | S_IRUGO, bs_info->sleep_dir);
	if (ent == NULL) {
		BT_ERR("%s: unable to create /proc/%s/btwrite entry", __func__, PROC_DIR);
		retval = -ENOMEM;
		goto err_proc_btwrite;
	}

	ent->read_proc = bluesleep_read_bt_state;
	ent->write_proc = bluesleep_ap_wakeup_bt;
	ent->data = bs_info;
#endif
	return retval;
#ifdef BT_LPM_SUPPORT_BTLD
err_proc_btwrite:
	remove_proc_entry("asleep", bs_info->sleep_dir);
#endif
err_proc_asleep:
	remove_proc_entry("proto", bs_info->sleep_dir);
err_proc_proto:
	remove_proc_entry("hostwake", bs_info->sleep_dir);
err_proc_hostwake:
	remove_proc_entry("btwake",	bs_info->sleep_dir);
err_proc_btwake:
	remove_proc_entry("sleep", bs_info->bluetooth_dir);
err_proc_sleep:
    remove_proc_entry("bluetooth", NULL);

	return retval;


}

static int __devinit bluesleep_probe(struct platform_device *pdev)
{
	int	ret = 0;
	struct resource	*res = NULL;

	bsi	= kzalloc(sizeof(struct	bluesleep_info), GFP_KERNEL);
	if (!bsi) {
		BT_ERR("%s malloc bluesleep_info failed\n",
					__func__);
		ret = -ENOMEM;
		return ret;
	}

	platform_set_drvdata(pdev, bsi);

#ifdef	CONFIG_MACH_K3V2OEM1
	ret = bluesleep_io_init(bsi);
	if (ret) {
		BT_ERR("%s: bt io init failed%d\n", __func__, ret);
		goto free_bsi;
	}
#endif

	res	= platform_get_resource_byname(pdev, IORESOURCE_IO,
				"gpio_host_wake");
	if (!res) {
		BT_ERR("%s: couldn't find host_wake gpio\n", __func__);
		ret	= -ENODEV;
		goto free_bsi;
	}
	bsi->host_wake = res->start;

	ret	= gpio_request(bsi->host_wake, "bt_host_wake");
	if (ret) {
		BT_ERR("%s: bt_host_wake gpio_request failed\n", __func__);
		goto free_bsi;
	}

	ret	= gpio_direction_input(bsi->host_wake);
	if (ret) {
		BT_ERR("%s: bt_host_wake gpio_direction_input failed\n", __func__);
		goto free_bt_host_wake;
	}

	res	= platform_get_resource_byname(pdev, IORESOURCE_IO,
				"gpio_ext_wake");
	if (!res) {
		BT_ERR("%s: couldn't find ext_wake gpio\n", __func__);
		ret	= -ENODEV;
		goto free_bt_host_wake;
	}

	bsi->ext_wake =	res->start;

	ret	= gpio_request(bsi->ext_wake, "bt_ext_wake");
	if (ret) {
		BT_ERR("%s: gpio_request bt_ext_wake failed\n", __func__);
		goto free_bt_host_wake;
	}

	/* assert bt wake */
	ret	= gpio_direction_output(bsi->ext_wake, 0);
	if (ret) {
		BT_ERR("%s: bt_ext_wake gpio_direction_input failed\n", __func__);
		goto free_bt_ext_wake;
	}

	bsi->host_wake_irq = gpio_to_irq(bsi->host_wake);
	if (bsi->host_wake_irq < 0)	{
		BT_ERR("%s: couldn't find host_wake	irq\n", __func__);
		ret	= -ENODEV;
		goto free_bt_ext_wake;
	}

	ret = create_bluesleep_proc_sys(bsi);
	if (ret) {
		BT_ERR("%s: create_bluesleep_proc_sys %d\n",
			__func__, ret);
		goto free_bt_ext_wake;
	}

	/* Initialize spinlock.*/
	spin_lock_init(&bsi->rw_lock);

	/* Initialize semaphore.*/
	sema_init(&bsi->bt_seam, 1);

	/* Initialize bluetooth wakelock*/
	wake_lock_init(&bsi->wake_lock, WAKE_LOCK_SUSPEND, "bluesleep");

	/* Initialize timer	*/
	init_timer(&bsi->tx_timer);
	bsi->tx_timer.function = bluesleep_tx_timer_expire;
	bsi->tx_timer.data = (unsigned long)bsi;

	/* initialize host wake	tasklet	*/
	tasklet_init(&bsi->hostwake_task,
			bluesleep_hostwake_task, (unsigned long)bsi);

	hci_register_notifier(&hci_event_nblock);

	return ret;

free_bt_ext_wake:
	gpio_free(bsi->ext_wake);
free_bt_host_wake:
	gpio_free(bsi->host_wake);
free_bsi:
	platform_set_drvdata(pdev, NULL);
	kfree(bsi);
	bsi = NULL;

	return ret;
}

static int __devexit bluesleep_remove(struct platform_device *pdev)
{

	struct bluesleep_info *bs_info = platform_get_drvdata(pdev);

	if (NULL == bs_info) {
		dev_err(&pdev->dev,	"%s, bs_info is null \n", __func__);
		return -1;
	}

	/* assert bt wake */
	gpio_set_value(bs_info->ext_wake, 0);

	hci_unregister_notifier(&hci_event_nblock);

	if (test_bit(BT_PROTO, &bs_info->flags)) {
#ifdef IRQ_WAKEUP
		if (disable_irq_wake(bs_info->host_wake_irq))
			BT_ERR("%s: couldn't disable hostwake IRQ wakeup mode\n", __func__);
#endif
		if (bs_info->host_wake_irq)
			free_irq(bs_info->host_wake_irq, bs_info);

		del_timer_sync(&bs_info->tx_timer);
		if (test_bit(BT_ASLEEP,	&bs_info->flags))
			hsuart_power(1);
	}

	gpio_free(bs_info->host_wake);
	gpio_free(bs_info->ext_wake);

#ifdef	CONFIG_MACH_K3V2OEM1
	/*SET LOWPOWER*/
	if (blockmux_set(bs_info->blockTemp, bs_info->configTemp, LOWPOWER))
		BT_ERR("%s: gpio blockmux set err\n",
			__func__);
#endif
#ifdef BT_LPM_SUPPORT_BTLD
	remove_proc_entry("btwrite", bs_info->sleep_dir);
#endif
	remove_proc_entry("asleep",	bs_info->sleep_dir);
	remove_proc_entry("proto", bs_info->sleep_dir);
	remove_proc_entry("hostwake", bs_info->sleep_dir);
	remove_proc_entry("btwake",	bs_info->sleep_dir);
	remove_proc_entry("sleep", bs_info->bluetooth_dir);
	remove_proc_entry("bluetooth", 0);

	wake_lock_destroy(&bsi->wake_lock);

	kfree(bs_info);
	bs_info = NULL;

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static void	bluesleep_shutdown(struct platform_device *pdev)
{
	struct bluesleep_info *bs_info = platform_get_drvdata(pdev);

	if (NULL == bs_info) {
		BT_ERR("%s: bs_info is null\n", __func__);
		return;
	}

	printk("[%s],+\n", __func__);

#ifdef	CONFIG_MACH_K3V2OEM1
	/*SET LOWPOWER*/
	if (blockmux_set(bs_info->blockTemp, bs_info->configTemp, LOWPOWER))
		BT_ERR("%s:	SETERROR:gpio blockmux set err\n",
			__func__);
#endif

	printk("[%s],-\n", __func__);

	return;
}


static struct platform_driver bluesleep_driver = {
	.probe  = bluesleep_probe,
	.remove	= __devexit_p(bluesleep_remove),
	.shutdown  = bluesleep_shutdown,
	.driver	= {
		.name =	"bluesleep",
		.owner = THIS_MODULE,
	},
};
/**
 * Initializes the module.
 * @return On success, 0. On error,	-1,	and	<code>errno</code> is set
 * appropriately.
 */
static int __init bluesleep_init(void)
{
	int	retval = 0;
	retval	= platform_driver_register(&bluesleep_driver);
	if (retval) {
		BT_ERR("%s:	platform_driver_register failed %d\n",
				__func__, retval);
	}
	return retval;
}

/**
 * Cleans up the module.
 */
static void	__exit bluesleep_exit(void)
{
	platform_driver_unregister(&bluesleep_driver);
}

module_init(bluesleep_init);
module_exit(bluesleep_exit);

MODULE_DESCRIPTION("Bluetooth Sleep Mode Driver	ver	%s " VERSION);
MODULE_LICENSE("GPL");

