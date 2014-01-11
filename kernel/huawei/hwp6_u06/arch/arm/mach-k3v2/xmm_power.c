/*
 * arch/arm/mach-k3v2/xmm_power.c
 *
 * Copyright (C) 2011 Hisilicon Co. Ltd.
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
 *
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/mux.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/random.h>
#include <linux/time.h>
#include <linux/compiler.h>

#include <mach/gpio.h>
#include <mach/platform.h>
#include <mach/xmm_power.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <linux/mutex.h>
#include <hsad/config_interface.h>
#include "mach/modem_boot.h"

MODULE_LICENSE("GPL");


#define ECHI_DEVICE_FILE        "/sys/devices/platform/hisik3-ehci/ehci_power"

#define MAX_ENUM_WAIT           1000  	/* mini sec */
#define MAX_ENUM_RETRY          15
#define MAX_HSIC_RESET_RETRY    3
#define MAX_MODEM_RESET_CHECK_TIMES   10
#define MAX_RESUME_RETRY_TIME   3

/* Enum of power state */
enum xmm_power_s_e {
	XMM_POW_S_OFF,
	XMM_POW_S_INIT_ON,
	XMM_POW_S_WAIT_READY,
	XMM_POW_S_INIT_USB_ON,
	XMM_POW_S_USB_ENUM_DONE,
	XMM_POW_S_USB_L0,
	XMM_POW_S_USB_L2,
	XMM_POW_S_USB_L2_TO_L0,
	XMM_POW_S_USB_L3,
};

static DEFINE_MUTEX(power_sem);
static enum xmm_power_s_e  xmm_curr_power_state;
static struct wake_lock xmm_wakelock;
static struct workqueue_struct *workqueue;
static struct work_struct l2_resume_work;
static struct work_struct l3_resume_work;
static struct work_struct modem_reset_work;
static wait_queue_head_t  xmm_wait_q;
static spinlock_t	  xmm_state_lock;
static unsigned long      xmm_last_suspend = 0;

static int modem_on_delay = 140;
static int usb_on_delay = 100;
static int reset_isr_register = 0;

static struct xmm_power_plat_data* xmm_driver_plat_data;

static struct iomux_block *xmm_iomux_block;
static struct block_config *xmm_iomux_config;

#define GPIO_CP_RESET           xmm_driver_plat_data->gpios[XMM_RESET].gpio
#define GPIO_CP_POWER_ON     	xmm_driver_plat_data->gpios[XMM_POWER_ON].gpio
#define GPIO_CP_PMU_RESET     	xmm_driver_plat_data->gpios[XMM_PMU_RESET].gpio
#define GPIO_HOST_ACTIVE     	xmm_driver_plat_data->gpios[XMM_HOST_ACTIVE].gpio
#define GPIO_HOST_WAKEUP        xmm_driver_plat_data->gpios[XMM_HOST_WAKEUP].gpio
#define GPIO_SLAVE_WAKEUP       xmm_driver_plat_data->gpios[XMM_SLAVE_WAKEUP].gpio
#define GPIO_POWER_IND          xmm_driver_plat_data->gpios[XMM_POWER_IND].gpio
#define GPIO_RESET_IND          xmm_driver_plat_data->gpios[XMM_RESET_IND].gpio
#define GPIO_TRIGGER_D          xmm_driver_plat_data->gpios[XMM_SIM_TRIGGER_D].gpio
#define GPIO_SIM_INI_CLK        xmm_driver_plat_data->gpios[XMM_SIM_INI_CLK].gpio

#define XMM_NETLINK         NETLINK_XMM
static struct sock *xmm_nlfd = NULL;
DEFINE_MUTEX(receive_sem);
static unsigned int xmm_rild_pid = 0;

struct xmm_nl_packet_msg {
	int xmm_event;
};

typedef enum Xmm_KnlMsgType {
    NETLINK_XMM_REG = 0,    /* send from rild to register the PID for netlink kernel socket */
    NETLINK_XMM_KER_MSG,    /* send from xmm to rild */
    NETLINK_XMM_UNREG       /* when rild exit send this type message to unregister */
} XMM_MSG_TYPE_EN;

enum _XMM_STATE_FLAG_ {
    XMM_STATE_FREE,  /* unsolicited power off modem, free monitor */
    XMM_STATE_OFF,
    XMM_STATE_POWER,
    XMM_STATE_READY,
    XMM_STATE_INVALID,
};

static int xmm_change_notify_event(int event)
{
	int ret;
	int size;
	unsigned char *old_tail;
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	struct xmm_nl_packet_msg *packet;

	mutex_lock(&receive_sem);
	if (!xmm_rild_pid || !xmm_nlfd ) {
		pr_err("xmm_power: %s, cannot notify event, g_rild_pid = %d \n",__func__,xmm_rild_pid);
		ret = -1;
		goto end;
	}
	size = NLMSG_SPACE(sizeof(struct xmm_nl_packet_msg));

	skb = alloc_skb(size, GFP_ATOMIC);
	if (!skb) {
		pr_info("xmm_power: %s, alloc skb fail\n",__func__);
		ret = -1;
		goto end;
	}
	old_tail = skb->tail;

	nlh = NLMSG_PUT(skb, 0, 0, NETLINK_XMM_KER_MSG, size-sizeof(*nlh));
	packet = NLMSG_DATA(nlh);
	memset(packet, 0, sizeof(struct xmm_nl_packet_msg));

	packet->xmm_event = event;

	nlh->nlmsg_len = skb->tail - old_tail;

	ret = netlink_unicast(xmm_nlfd, skb, xmm_rild_pid, MSG_DONTWAIT);
	goto end;

nlmsg_failure:
	ret = -1;
	if(skb)
		kfree_skb(skb);
end:
	mutex_unlock(&receive_sem);
	return ret;
}

static void kernel_xmm_receive(struct sk_buff *skb)
{
	pr_info("xmm_power: kernel_xmm_receive +\n");
	mutex_lock(&receive_sem);

	if(skb->len >= sizeof(struct nlmsghdr)) {
		struct nlmsghdr *nlh;
		nlh = (struct nlmsghdr *)skb->data;
		if((nlh->nlmsg_len >= sizeof(struct nlmsghdr))&& (skb->len >= nlh->nlmsg_len)) {
			if(nlh->nlmsg_type == NETLINK_XMM_REG) {
				xmm_rild_pid = nlh->nlmsg_pid;
				pr_info("xmm_power: %s, netlink receive reg packet %d ;\n",__func__,xmm_rild_pid);
			} else if(nlh->nlmsg_type == NETLINK_XMM_UNREG) {
				pr_info("xmm_power: %s, netlink NETLINK_TIMER_UNREG ;\n",__func__ );
				xmm_rild_pid = 0;
			}
		}
	}
	mutex_unlock(&receive_sem);
}

static void xmm_netlink_init(void)
{
	xmm_nlfd = netlink_kernel_create(&init_net,
	                               XMM_NETLINK, 0, kernel_xmm_receive, NULL, THIS_MODULE);
	if(!xmm_nlfd)
		pr_info("xmm_power: %s, netlink_kernel_create faile ;\n",__func__ );
	else
		pr_info("xmm_power: %s, netlink_kernel_create success ;\n",__func__ );

	return;
}

static void xmm_netlink_deinit(void)
{
	if(xmm_nlfd && xmm_nlfd->sk_socket)
		sock_release(xmm_nlfd->sk_socket);
}

unsigned long calc_usec_interval(struct timespec *start_tv, struct timespec *end_tv)
{
	return ((end_tv->tv_sec - start_tv->tv_sec) * 1000000)
		 + ((end_tv->tv_nsec - start_tv->tv_nsec) / 1000);
}

#if 0
static void accu_udelay(unsigned long usecs)
{
	struct timespec start_tv, curr_tv;
	getnstimeofday(&start_tv);
	while (1) {
		barrier();
		getnstimeofday(&curr_tv);
		if (calc_usec_interval(&start_tv, &curr_tv) >= usecs)
			break;
	}
}

#define accu_mdelay(msecs)   accu_udelay((msecs) * 1000)
#endif

#define accu_udelay(usecs)   udelay(usecs)
#define accu_mdelay(msecs)   mdelay(msecs)

static int gpio_lowpower_set(enum iomux_func newmode)
{
	int ret;
	ret = blockmux_set(xmm_iomux_block, xmm_iomux_config, newmode);
	if (ret) {
		pr_info("SETERROR:blockmux set err\r\n");
		return -1;
	}
	return 0;
}

static int get_ehci_connect_status(int* status)
{
	mm_segment_t oldfs;
	struct file *filp;
	char buf[4] = {0};
	int ret = 0;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(ECHI_DEVICE_FILE, O_RDWR, 0);

	if (!filp || IS_ERR(filp)) {
		pr_info("xmm_power: Invalid EHCI filp=%ld\n", PTR_ERR(filp));
		ret = -EINVAL;
		set_fs(oldfs);
		return ret;
	} else {
		filp->f_op->read(filp, buf, 1, &filp->f_pos);
	}

	if (sscanf(buf, "%d", status) != 1)
	{
		pr_info("xmm_power: Invalid result get from ehci\n");
		ret = -EINVAL;
	}

	filp_close(filp, NULL);
	set_fs(oldfs);

	return ret;
}

static bool is_echi_connected(void)
{
	int status = 0;
	int retry = 10;

	for (; retry > 0; retry--) {
		get_ehci_connect_status(&status);
		if (status == 1)
			return true;
		msleep(50);
	}

	return false;
}

/* Enable/disable USB host controller and HSIC phy */
static void enable_usb_hsic(bool enable)
{
	mm_segment_t oldfs;
	struct file *filp;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(ECHI_DEVICE_FILE, O_RDWR, 0);
	pr_info("xmm_power: EHCI filp=%ld\n", PTR_ERR(filp));

	if (!filp || IS_ERR(filp)) {
		/* Fs node not exit, register the ECHI device */
		/*
		if (enable && xmm_driver_plat_data &&
			xmm_driver_plat_data->echi_device){
			platform_device_register(xmm_driver_plat_data->echi_device);
		}
		*/
		pr_err("xmm_power: failed to access ECHI\n");
	} else {
		if (enable) {
			filp->f_op->write(filp, "1", 1, &filp->f_pos);
		}
		else {
			filp->f_op->write(filp, "0", 1, &filp->f_pos);
		}
		filp_close(filp, NULL);
	}
	set_fs(oldfs);
}

static void set_usb_hsic_suspend(bool suspend)
{
	mm_segment_t oldfs;
	struct file *filp;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(ECHI_DEVICE_FILE, O_RDWR, 0);

	if (!filp || IS_ERR(filp)) {
		pr_err("xmm_power: Can not open EHCI file. filp=%ld\n", PTR_ERR(filp));
	} else {
		if (suspend) {
			filp->f_op->write(filp, "2", 1, &filp->f_pos);
		}
		else {
			filp->f_op->write(filp, "3", 1, &filp->f_pos);
		}
		filp_close(filp, NULL);
	}
	set_fs(oldfs);
}

static void notify_usb_resume(void)
{
	mm_segment_t oldfs;
	struct file *filp;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open("/dev/ttyACM0", O_RDWR, 0);

	if (!filp || IS_ERR(filp)) {
		pr_err("xmm_power: Can not open tty file. filp=%ld\n", PTR_ERR(filp));
	} else {
		filp_close(filp, NULL);
	}
	set_fs(oldfs);
}

static void simcard_detect_enable(void)
{
	gpio_set_value(GPIO_TRIGGER_D, 1);
	gpio_set_value(GPIO_SIM_INI_CLK, 1);
	accu_udelay(1);
	gpio_set_value(GPIO_SIM_INI_CLK, 0);
	accu_udelay(1);
	gpio_set_value(GPIO_SIM_INI_CLK, 1);
	accu_udelay(1);
	gpio_set_value(GPIO_TRIGGER_D, 0);
}

/* Do CP power on */
static void xmm_power_on(void)
{
	gpio_set_value(GPIO_CP_RESET, 1);
	gpio_set_value(GPIO_CP_PMU_RESET, 1);
	accu_mdelay(1);
	gpio_set_value(GPIO_CP_POWER_ON, 1);
	accu_udelay(60);
	gpio_set_value(GPIO_CP_POWER_ON, 0);
}

/* Do CP power off */
static void xmm_power_off(void)
{
	gpio_set_value(GPIO_CP_RESET, 0);
	gpio_set_value(GPIO_CP_PMU_RESET, 0);
}

extern int notify_mdm_off_to_pm(void);
static int cp_shutdown;
static DEFINE_MUTEX(cp_shutdown_mutex);
static int cp_shutdown_get(void)
{
    int value=-1;

    mutex_lock(&cp_shutdown_mutex);
    value = cp_shutdown;
    mutex_unlock(&cp_shutdown_mutex);
    return value;
}
static ssize_t cp_shutdown_show(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
    char *_buf = buf;

    mutex_lock(&cp_shutdown_mutex);
    _buf += snprintf(_buf, PAGE_SIZE, "%d\n", cp_shutdown);

    mutex_unlock(&cp_shutdown_mutex);
    return (_buf - buf);
}
static ssize_t cp_shutdown_store(struct device *dev,struct device_attribute *attr,
                            const char *buf, size_t count)
{
    int value=0;

    if (sscanf(buf, "%d", &value) != 1)
        return -EINVAL;

    mutex_lock(&cp_shutdown_mutex);
    pr_info("\nXMD: %s. [cp_shutdown] %d -> %d;\n",__func__,cp_shutdown,value);
    cp_shutdown= value;
    mutex_unlock(&cp_shutdown_mutex);
    return count;
}
/* sysfs entries for "cp_shutdown" node control */
static DEVICE_ATTR(cp_shutdown, S_IRUGO | S_IWUSR, cp_shutdown_show, cp_shutdown_store);

static inline void xmm_power_change_state(enum xmm_power_s_e state)
{
	pr_info("xmm_power: state change %d->%d\n", xmm_curr_power_state, state);
	xmm_curr_power_state = state;
}

static void xmm_power_usb_on(void)
{
	gpio_set_value(GPIO_HOST_ACTIVE, 1);
	enable_usb_hsic(true);
	msleep(usb_on_delay);

	/* Pull down HOST_ACTIVE to reset CP's USB */
	gpio_set_value(GPIO_HOST_ACTIVE, 0);
	accu_mdelay(10);
	gpio_set_value(GPIO_HOST_ACTIVE, 1);
}

/* Check enumeration with CP's bootrom */
static int usb_enum_check1(void)
{
	int i;

	pr_info("xmm_power: Start usb enum check 1\n");

	for (i=0; i<MAX_ENUM_RETRY; i++) {
		msleep(40);
		accu_mdelay(i%6);
		/* Check ECHI connect state to see if the enumeration will be success */
		if (readl(IO_ADDRESS(REG_BASE_USB2EHCI) + 0x58) & 1) {
			pr_info("xmm_power: bootrom emun seems to be done after %d retrys\n", i);
			if (is_echi_connected()) {
				pr_info("xmm_power: bootrom emun done\n");
				return 0;
			}
			else {
				pr_info("xmm_power: connect signal detected but bootrom emun failed\n");
				return -1;
			}
		}
		else {
			gpio_set_value(GPIO_CP_PMU_RESET, 0);
			msleep(30);
			gpio_set_value(GPIO_CP_PMU_RESET, 1);
			accu_mdelay(1);
			gpio_set_value(GPIO_CP_POWER_ON, 1);
			accu_udelay(60);
			gpio_set_value(GPIO_CP_POWER_ON, 0);
		}
	}

	return -1;
}

/* Check enumeration with CP's system-img */
static int usb_enum_check2(void)
{
	int i;

	pr_info("xmm_power: Start usb enum check 2\n");

	for (i=0; i<MAX_ENUM_RETRY; i++) {
		accu_mdelay(20 + (i%3));
		/* Check ECHI connect state to see if the enumeration will be success */
		if (readl(IO_ADDRESS(REG_BASE_USB2EHCI) + 0x58) & 1) {
			pr_info("xmm_power: system emun seems to be done after %d retrys\n", i);
			if (is_echi_connected()) {
				pr_info("xmm_power: system emun done\n");
				xmm_power_change_state(XMM_POW_S_USB_ENUM_DONE);
				return 0;
			}
			else {
				pr_info("xmm_power: connect signal detected but system emun failed\n");
				return -1;
			}
		}
		else {
			gpio_set_value(GPIO_HOST_ACTIVE, 0);
			accu_mdelay(10);
			gpio_set_value(GPIO_HOST_ACTIVE, 1);
		}
	}

	return -1;
}


/* Check enumeration with CP's system-img */
static int usb_enum_check3(void)
{
	int i;

	pr_info("xmm_power: Start usb enum check 3\n");

	for (i=0; i<MAX_ENUM_RETRY; i++) {
		accu_mdelay(30 + (i%3));
		/* Check ECHI connect state to see if the enumeration will be success */
		if ((readl(IO_ADDRESS(REG_BASE_USB2EHCI) + 0x58) & 1)) {
			pr_info("xmm_power: resume emun seems to be done after %d retrys\n", i);
			if (is_echi_connected()) {
				pr_info("xmm_power: resume emun done\n");
				gpio_set_value(GPIO_HOST_ACTIVE, 1);
				gpio_set_value(GPIO_SLAVE_WAKEUP, 0);
				return 0;
			}
			else {
				pr_info("xmm_power: connect signal detected but resume emun failed\n");
				return -1;
			}
		}
		else {
			gpio_set_value(GPIO_HOST_ACTIVE, 1);
			gpio_set_value(GPIO_SLAVE_WAKEUP, 0);
			accu_mdelay(10);
			gpio_set_value(GPIO_HOST_ACTIVE, 0);
			accu_mdelay(20);
			gpio_set_value(GPIO_SLAVE_WAKEUP, 1);
		}
	}

	return -1;
}


/* Resume from L2 state */
static void xmm_power_l2_resume(struct work_struct *work)
{
	pr_info("xmm_power: resume from L2\n");
	msleep(10);
	notify_usb_resume();
}

/* Resume from L3 state */
static void xmm_power_l3_resume(struct work_struct *work)
{
	int i;
	for (i=0; i<MAX_HSIC_RESET_RETRY; i++) {
		if (xmm_curr_power_state != XMM_POW_S_USB_L3) {
			pr_info("xmm_power: resume not in L3 state %d\n",xmm_curr_power_state);
			return;
		}

		enable_usb_hsic(true);
		msleep(10);
		if (gpio_get_value(GPIO_HOST_WAKEUP)) {
			/* AP wakeup CP */
			accu_mdelay(5);
			gpio_set_value(GPIO_SLAVE_WAKEUP, 1);
		}

		if (usb_enum_check3() != 0) {
			enable_usb_hsic(false);
			gpio_set_value(GPIO_SLAVE_WAKEUP, 0);
			msleep(10);
		}
		else {
			xmm_power_change_state(XMM_POW_S_USB_L0);
			break;
		}
	}

	if (i == MAX_HSIC_RESET_RETRY) {
		pr_err("xmm_power: Resume enumeration failed.\n");
		wake_unlock(&xmm_wakelock);
	}
}

static void xmm_power_reset_work(struct work_struct *work)
{
	/* TODO: add pm func here */
	int i = 0;
	while(i++ < MAX_MODEM_RESET_CHECK_TIMES) {
		msleep(10);
		if (gpio_get_value(GPIO_RESET_IND) == 1) {
			pr_notice("xmm_power: modem software reset\n");
			break;
		}
	}

	if (i >= MAX_MODEM_RESET_CHECK_TIMES)
		pr_notice("xmm_power: modem hardware reset\n");

	if (cp_shutdown_get() == false){
		notify_mdm_off_to_pm();
		pr_info("notify modem hardware shutdown to PM");
	}

	xmm_change_notify_event(XMM_STATE_OFF);
}

static irqreturn_t modem_reset_isr(int irq, void *dev_id)
{
	struct device* dev = (struct device*)dev_id;
	dev_info(dev, "modem_reset_isr get !!\n");

	if (gpio_get_value(GPIO_RESET_IND) == 0) {
		dev_info(dev, "modem reset abnorm, notice ril.\n");
		queue_work(workqueue, &modem_reset_work);
	}

	return IRQ_HANDLED;
}

static void request_reset_isr(struct device *dev)
{
	int status = 0;

	if (reset_isr_register == 1)
		return;

	/* Register IRQ for MODEM_RESET gpio */
	status = request_irq(gpio_to_irq(GPIO_RESET_IND),
	                     modem_reset_isr,
	                     IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND,
	                     "MODEM_RESET_IRQ",
	                     dev);
	if (status) {
		reset_isr_register = 0;
		dev_err(dev, "Request irq for modem reset failed\n");
	}else
		reset_isr_register = 1;
}

static void free_reset_isr(struct device *dev)
{
	if (reset_isr_register == 1)
		free_irq(gpio_to_irq(GPIO_RESET_IND),dev);

	reset_isr_register = 0;
}

static ssize_t xmm_power_get(struct device *dev,
                 struct device_attribute *attr,
                 char *buf)
{
    ssize_t ret = xmm_curr_power_state;

    printk("~~~~~~~~~~~~~~~~~~~~~~ ret=%d\n", ret);
    return sprintf(buf, "%d\n", ret);
}

static ssize_t xmm_power_set(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int state;
	int i;
	unsigned long flags;
	struct xmm_power_plat_data *plat = dev->platform_data;
	unsigned long tio;

	dev_info(dev, "Power set to %s\n", buf);

	if (sscanf(buf, "%d", &state) != 1)
		return -EINVAL;

	mutex_lock(&power_sem);
	if (state == POWER_SET_ON) {
		xmm_power_change_state(XMM_POW_S_INIT_ON);
		free_reset_isr(dev);
		cancel_work_sync(&l3_resume_work);

		if (plat && plat->flashless) {
			/* For flashless, enable USB to enumerate with CP's bootrom */
			enable_usb_hsic(true);
			msleep(usb_on_delay);
		}
		else {
			xmm_power_change_state(XMM_POW_S_WAIT_READY);
		}

		simcard_detect_enable();
		xmm_power_on();
		msleep(20);
		gpio_lowpower_set(NORMAL);
		gpio_set_value(GPIO_HOST_ACTIVE, 1);

		for (i=0; i<MAX_HSIC_RESET_RETRY; i++){
			if (usb_enum_check1() == 0)
				break;
			enable_usb_hsic(false);
			msleep(10);
			enable_usb_hsic(true);
			msleep(usb_on_delay);
		}

		if (i == MAX_HSIC_RESET_RETRY){
			/* Fail to enumarte, make caller to know */
			dev_err(dev, "boot rom enum retry failed\n");
			mutex_unlock(&power_sem);
			return -1;
		}
	}
	else if (state == POWER_SET_OFF){
		free_reset_isr(dev);
		spin_lock_irqsave(&xmm_state_lock, flags);
		if (xmm_curr_power_state == XMM_POW_S_USB_L0)
			wake_unlock(&xmm_wakelock);

		xmm_power_change_state(XMM_POW_S_OFF);
		spin_unlock_irqrestore(&xmm_state_lock, flags);

		enable_usb_hsic(false);
		gpio_set_value(GPIO_HOST_ACTIVE, 0);
		gpio_set_value(GPIO_SLAVE_WAKEUP, 0);
		xmm_power_off();
		gpio_lowpower_set(LOWPOWER);
	}
	else if (state == POWER_SET_DL_FINISH){
		tio = msecs_to_jiffies(5000);
		xmm_power_change_state(XMM_POW_S_WAIT_READY);
		enable_usb_hsic(false);
		gpio_set_value(GPIO_HOST_ACTIVE, 0);
		tio = wait_event_timeout(xmm_wait_q,
					(xmm_curr_power_state == XMM_POW_S_INIT_USB_ON),
					tio);
		if (tio == 0) {
			dev_err(dev, "Wait modem system on timeout\n");
			mutex_unlock(&power_sem);
			return -1;
		}

		for (i=0; i<MAX_HSIC_RESET_RETRY; i++){
			xmm_power_usb_on();
			if (usb_enum_check2() == 0)
				break;

			enable_usb_hsic(false);
			msleep(10);
		}

		if (i == MAX_HSIC_RESET_RETRY) {
			/* Failed to enumarte, make caller to know */
			dev_err(dev, "system enum retry failed\n");
			mutex_unlock(&power_sem);
			return 0;
		}
		request_reset_isr(dev);
	}
	else if (state == POWER_SET_DEBUGON){
		free_reset_isr(dev);
		xmm_power_on();
		gpio_lowpower_set(NORMAL);
	}
	else if (state == POWER_SET_DEBUGOFF){
		free_reset_isr(dev);
		xmm_power_off();
		gpio_lowpower_set(LOWPOWER);
	}
#if 1 /* Command for debug use */
	else if (state == 5){ /* L0 to L2 */
		xmm_power_change_state(XMM_POW_S_USB_L2);
		set_usb_hsic_suspend(true);
	}
	else if (state == 6){ /* L2 to L0 */
		gpio_set_value(GPIO_SLAVE_WAKEUP, 1);
	}
	else if (state == 7){ /* to L3 */
		enable_usb_hsic(false);
		gpio_set_value(GPIO_HOST_ACTIVE, 0);
		xmm_power_change_state(XMM_POW_S_USB_L3);
	}
	else if (state == 8){ /* L3 to L0 */
		enable_usb_hsic(true);
		accu_mdelay(5);
		gpio_set_value(GPIO_SLAVE_WAKEUP, 1);

		if (usb_enum_check3() == 0) {
			xmm_power_change_state(XMM_POW_S_USB_L0);
		}
	}
	else if (state == 9){
		pr_info("0\n");
		accu_mdelay(1);
		pr_info("1\n");
		accu_mdelay(2);
		pr_info("2\n");
		accu_mdelay(3);
		pr_info("3\n");
		accu_mdelay(4);
		pr_info("4\n");
		accu_mdelay(5);
		pr_info("5\n");
		accu_mdelay(10);
		pr_info("10\n");
		accu_mdelay(20);
		pr_info("20\n");
	}
#endif
	mutex_unlock(&power_sem);
	return count;
}

static ssize_t xmm_gpio_set(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	char name[128] = {0};
	int val;
	unsigned gpio;
	int i;

	if (!xmm_driver_plat_data)
		return -EINVAL;

	if (sscanf(buf, "%s %d", name, &val) != 2)
		return -EINVAL;

	for (i=0; i<ARRAY_SIZE(xmm_driver_plat_data->gpios); i++)
		if (strcmp(name, xmm_driver_plat_data->gpios[i].label) == 0)
			break;

	if (i == ARRAY_SIZE(xmm_driver_plat_data->gpios))
		return -EINVAL;
	else
		gpio = xmm_driver_plat_data->gpios[i].gpio;

	dev_info(dev, "%s set to %d\n", name, val);

	gpio_set_value(gpio, val);

	return count;
}

static ssize_t xmm_gpio_get(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	char* str = buf;
	int i;

	if (!xmm_driver_plat_data)
		return 0;

	for (i=0; i<ARRAY_SIZE(xmm_driver_plat_data->gpios); i++)
		str += sprintf(str, "%s %d\n", xmm_driver_plat_data->gpios[i].label,
				gpio_get_value(xmm_driver_plat_data->gpios[i].gpio));

	return str - buf;
}

static ssize_t xmm_delay_set(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int delay1;
	int delay2;

	if (sscanf(buf, "%d %d", &delay1, &delay2) != 2) {
		dev_err(dev, "Invalid input\n");
		return -EINVAL;
	}

	if (delay1 < 0 || delay1 > 10000 || delay2 < 0 || delay2 > 10000) {
		dev_err(dev, "Invalid delay value input\n");
		return -EINVAL;
	}

	modem_on_delay = delay1;
	usb_on_delay = delay2;
	dev_info(dev, "modem_on_delay: %d, usb_on_delay: %d\n", modem_on_delay, usb_on_delay);

	return count;
}

/*------------------------------ SYSFS "modem_state" ------------------------------*/
static ssize_t modem_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if( gpio_get_value(GPIO_RESET_IND)==0 )
        return ( snprintf(buf,PAGE_SIZE,"%s\n", modem_state_str[MODEM_STATE_OFF]) );
    else {
        if( reset_isr_register==1 )
            return ( snprintf(buf,PAGE_SIZE,"%s\n", modem_state_str[MODEM_STATE_READY]) );
        else
            return ( snprintf(buf,PAGE_SIZE,"%s\n", modem_state_str[MODEM_STATE_POWER]) );
    }
}


static DEVICE_ATTR(modem_state, S_IRUGO, modem_state_show, NULL);
static DEVICE_ATTR(state, S_IRUGO | S_IWUSR, xmm_power_get, xmm_power_set);
/* gpio and delay are for debug */
static DEVICE_ATTR(gpio, S_IRUGO | S_IWUSR, xmm_gpio_get, xmm_gpio_set);
static DEVICE_ATTR(delay, S_IRUGO | S_IWUSR, NULL, xmm_delay_set);


static irqreturn_t host_wakeup_isr(int irq, void *dev_id)
{
	struct device* dev = (struct device*)dev_id;
	int val = gpio_get_value(GPIO_HOST_WAKEUP);
	unsigned long flags;

	if (val)
		dev_dbg(dev, "Host wakeup rising.\n");
	else
		dev_dbg(dev, "Host wakeup falling.\n");

	spin_lock_irqsave(&xmm_state_lock, flags);
	switch (xmm_curr_power_state) {
	case XMM_POW_S_WAIT_READY:
		if (!val) {
			xmm_power_change_state(XMM_POW_S_INIT_USB_ON);
			wake_up(&xmm_wait_q);
		}
		break;

	case XMM_POW_S_USB_ENUM_DONE:
		if (val) {
			xmm_power_change_state(XMM_POW_S_USB_L0);
			wake_lock(&xmm_wakelock);
		}
		break;

	case XMM_POW_S_USB_L2:
		if (!val) {
			xmm_power_change_state(XMM_POW_S_USB_L2_TO_L0);
			wake_lock(&xmm_wakelock);

			if (gpio_get_value(GPIO_SLAVE_WAKEUP) == 0) {
				/* CP wakeup AP, must do something to make USB host know it */
				queue_work(workqueue, &l2_resume_work);
			}
		}
		break;

	case XMM_POW_S_USB_L2_TO_L0:
		if (val) {
			xmm_power_change_state(XMM_POW_S_USB_L0);
			/* If AP wakeup CP, set SLAVE_WAKEUP to low to complete */
			if (gpio_get_value(GPIO_SLAVE_WAKEUP))
				gpio_set_value(GPIO_SLAVE_WAKEUP, 0);
		}
		break;

	case XMM_POW_S_USB_L3:
		if (!val && !wake_lock_active(&xmm_wakelock))
			/* Wakeup by CP, wakelock the system */
			wake_lock(&xmm_wakelock);
		break;

	case XMM_POW_S_USB_L0:
		if (!val) {
			dev_notice(dev, "Receive HOST_WAKEUP at L0, workaround\n");
		}
		break;

	default:
		break;
	}

	spin_unlock_irqrestore(&xmm_state_lock, flags);
	return IRQ_HANDLED;
}


extern  int hisik3_ehci_bus_post_suspend_register( struct device *dev, void (*fn)(void) );
extern  int hisik3_ehci_bus_pre_resume_register( struct device *dev, void (*fn)(void) );
static int xmm_power_probe(struct platform_device *pdev)
{
	int status = -1;

	dev_dbg(&pdev->dev, "xmm_power_probe\n");

	xmm_driver_plat_data = pdev->dev.platform_data;

	if (xmm_driver_plat_data) {
		status = gpio_request_array(xmm_driver_plat_data->gpios, XMM_GPIO_NUM);
		if (status) {
			dev_err(&pdev->dev, "GPIO request failed.\n");
			goto error;
		}
	}

	xmm_iomux_block = iomux_get_block(xmm_driver_plat_data->block_name);
	if (!xmm_iomux_block) {
		dev_err(&pdev->dev, "iomux_get_block failed.\n");
		goto error;
	}

	xmm_iomux_config = iomux_get_blockconfig(xmm_driver_plat_data->block_name);
	if (!xmm_iomux_config) {
		dev_err(&pdev->dev, "iomux_get_blockconfig failed.\n");
		goto error;
	}

	/* Register IRQ for HOST_WAKEUP gpio */
	status = request_irq(gpio_to_irq(GPIO_HOST_WAKEUP),
				host_wakeup_isr,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND,
				"HOST_WAKEUP_IRQ",
				&pdev->dev);
	if (status) {
		dev_err(&pdev->dev, "Request irq for host wakeup\n");
		goto error;
	}

	/* sysfs entries for IO control */
	status = device_create_file(&(pdev->dev), &dev_attr_state);
	if (status) {
		dev_err(&pdev->dev, "Failed to create sysfs entry\n");
		goto error;
	}

	status = device_create_file(&(pdev->dev), &dev_attr_gpio);
	if (status) {
		dev_err(&pdev->dev, "Failed to create sysfs entry\n");
		goto error;
	}

        status = device_create_file(&(pdev->dev), &dev_attr_cp_shutdown);
	if (status) {
	        dev_err(&pdev->dev,"XMD: BOOT error creating sysfs entry cp_shutdown\n");
	        goto error;
	}
	
	status = device_create_file(&(pdev->dev), &dev_attr_delay);
	if (status) {
		dev_err(&pdev->dev, "Failed to create sysfs entry\n");
		goto error;
	}

	status = device_create_file(&(pdev->dev), &dev_attr_modem_state);
	if (status) {
		dev_err(&pdev->dev, "Failed to create sysfs modem_state entry\n");
		goto error;
	}

    //Register hisik3_ehci_bus_post_suspend and hisik3_ehci_bus_pre_resume function for dynamic compatible with multi modem
    status = hisik3_ehci_bus_post_suspend_register( NULL, xmm_power_runtime_idle );
    if (status)
        dev_info(&pdev->dev, "Warning: Register hisik3_ehci_bus_post_suspend fn failed!\n");

    status = hisik3_ehci_bus_pre_resume_register( NULL, xmm_power_runtime_resume );
    if (status)
        dev_info(&pdev->dev, "Warning: Register hisik3_ehci_bus_pre_resume fn failed!\n");

	/* Initialize works */
	workqueue = create_singlethread_workqueue("xmm_power_workqueue");
	if (!workqueue) {
		dev_err(&pdev->dev, "Create workqueue failed\n");
		status = -1;
		goto error;
	}

	INIT_WORK(&l2_resume_work, xmm_power_l2_resume);
	INIT_WORK(&l3_resume_work, xmm_power_l3_resume);
	INIT_WORK(&modem_reset_work, xmm_power_reset_work);

	init_waitqueue_head(&xmm_wait_q);

	/* This wakelock will be used to arrest system sleeping when USB is in L0 state */
	wake_lock_init(&xmm_wakelock, WAKE_LOCK_SUSPEND, "xmm_power");

	xmm_curr_power_state = XMM_POW_S_OFF;

	return 0;

error:
	if (xmm_driver_plat_data) {
		gpio_free_array(xmm_driver_plat_data->gpios, XMM_GPIO_NUM);
	}

	device_remove_file(&(pdev->dev), &dev_attr_state);
	device_remove_file(&(pdev->dev), &dev_attr_gpio);
	device_remove_file(&(pdev->dev), &dev_attr_delay);

	return status;
}

static int xmm_power_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "xmm_power_remove\n");

	wake_lock_destroy(&xmm_wakelock);

	destroy_workqueue(workqueue);

	device_remove_file(&(pdev->dev), &dev_attr_state);
	device_remove_file(&(pdev->dev), &dev_attr_gpio);
	device_remove_file(&(pdev->dev), &dev_attr_delay);

	if (xmm_driver_plat_data) {
		gpio_free_array(xmm_driver_plat_data->gpios, XMM_GPIO_NUM);
	}

	return 0;
}


int xmm_power_suspend(struct platform_device *pdev, pm_message_t state)
{
	unsigned long flags;
	dev_info(&pdev->dev, "suspend+\n");

	spin_lock_irqsave(&xmm_state_lock, flags);
	if (xmm_curr_power_state == XMM_POW_S_USB_L2) {
		xmm_power_change_state(XMM_POW_S_USB_L3);
		spin_unlock_irqrestore(&xmm_state_lock, flags);
		enable_usb_hsic(false);
		gpio_set_value(GPIO_HOST_ACTIVE, 0);
	}
	else if (xmm_curr_power_state == XMM_POW_S_USB_L3){
		spin_unlock_irqrestore(&xmm_state_lock, flags);
		dev_warn(&pdev->dev, "already in L3 state\n");
	}
	else {
		spin_unlock_irqrestore(&xmm_state_lock, flags);
		if (xmm_curr_power_state != XMM_POW_S_OFF) {
			dev_warn(&pdev->dev, "suspend at unexpected state %d\n", xmm_curr_power_state);
			return -1;
		}
	}

	dev_info(&pdev->dev, "suspend-\n");

	return 0;
}

int xmm_power_resume(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "resume+\n");
	if (xmm_curr_power_state == XMM_POW_S_USB_L3) {
		if (!wake_lock_active(&xmm_wakelock))
			wake_lock(&xmm_wakelock);
		queue_work(workqueue, &l3_resume_work);
	}
	else if (xmm_curr_power_state != XMM_POW_S_OFF) {
		dev_info(&pdev->dev, "resume at unexpected state %d\n", xmm_curr_power_state);
	}
	dev_info(&pdev->dev, "resume-\n");

	return 0;
}

static struct platform_driver xmm_power_driver = {
	.probe = xmm_power_probe,
	.remove = xmm_power_remove,
	.suspend = xmm_power_suspend,
	.resume = xmm_power_resume,
	.driver = {
		.name ="xmm_power",
		.owner = THIS_MODULE,
	},
};


void xmm_power_runtime_idle()
{
	xmm_power_change_state(XMM_POW_S_USB_L2);
	wake_unlock(&xmm_wakelock);
	xmm_last_suspend = jiffies;
}
EXPORT_SYMBOL_GPL(xmm_power_runtime_idle);


void xmm_power_runtime_resume()
{
      int i = 0;
	if (xmm_curr_power_state == XMM_POW_S_USB_L2
	|| xmm_curr_power_state == XMM_POW_S_USB_L2_TO_L0) {
		if (gpio_get_value(GPIO_HOST_WAKEUP)) {
			/* It's AP wakeup CP */
			unsigned long least_time = xmm_last_suspend + msecs_to_jiffies(100);
			unsigned long tio;

			/* If resume just after suspending, the modem may fail
			   to resume, check this case and delay for awhile */
			while (time_before_eq(jiffies, least_time)) {
				if (gpio_get_value(GPIO_HOST_WAKEUP) == 0) {
					pr_info("xmm_power: CP want to wakeup AP too\n");
					return;
				}
				else
					pr_info("xmm_power: suspending delayed\n");

				msleep(10);
			}

			while(i < MAX_RESUME_RETRY_TIME) {
				gpio_set_value(GPIO_SLAVE_WAKEUP, 1);
				tio = jiffies + msecs_to_jiffies(200);
				while (time_before_eq(jiffies, tio)) {
					msleep(10);
					if (gpio_get_value(GPIO_HOST_WAKEUP) == 0)
						break;
				}

				if (gpio_get_value(GPIO_HOST_WAKEUP) != 0) {
					pr_err("xmm_power: Wait for resume USB timeout, retry\n");
					gpio_set_value(GPIO_SLAVE_WAKEUP, 0);
					msleep(10);
				}
				else {
					break;
				}
				i++;
			}

			if(i == MAX_RESUME_RETRY_TIME) {
				pr_err("xmm_power: Wait for resume USB timeout, No Method to Resolve\n");
			}
		}
	}
	else {
		pr_err("xmm_power: Invalid state %d for runtime resume.\n",
			xmm_curr_power_state);
	}
}
EXPORT_SYMBOL_GPL(xmm_power_runtime_resume);


static int __init xmm_power_init(void)
{
    int ret = -1;

        ret = get_modem_type( "xmm6260" );
        pr_info("MODEM#%s: get_modem_type \"%s\". %d\n","xmm_power","xmm6260",ret);
        if( ret<=0 ) {
            pr_err("MODEM#%s: modem \"%s\" not support on this board. %d\n","xmm_power","xmm6260",ret);
            return( -1 );
        }

	platform_driver_register(&xmm_power_driver);
	spin_lock_init(&xmm_state_lock);
	xmm_netlink_init();
	return 0;
}

static void __exit xmm_power_exit(void)
{
	platform_driver_unregister(&xmm_power_driver);
	xmm_netlink_deinit();
}

module_init(xmm_power_init);
module_exit(xmm_power_exit);

