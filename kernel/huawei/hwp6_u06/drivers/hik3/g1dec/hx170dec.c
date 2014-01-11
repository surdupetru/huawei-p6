/* 
 * Decoder device driver (kernel module)
 *
 * Copyright (C) 2012 Google Finland Oy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
------------------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <linux/module.h>
/* needed for __init,__exit directives */
#include <linux/init.h>
/* needed for remap_pfn_range
    SetPageReserved
    ClearPageReserved
*/
#include <linux/mm.h>
/* obviously, for kmalloc */
#include <linux/slab.h>
/* for struct file_operations, register_chrdev() */
#include <linux/fs.h>
/* standard error codes */
#include <linux/errno.h>

#include <linux/moduleparam.h>
/* request_irq(), free_irq() */
#include <linux/interrupt.h>

/* needed for virt_to_phys() */
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <asm/irq.h>
#include <linux/platform_device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include "mach/platform.h"
#include "mach/irqs.h"
#include "mach/hardware.h"
#include "mach/early-debug.h"
#include "mach/hisi_mem.h"

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

/* our own stuff */
#include "hx170dec.h"

#define DECODER_CLOCK 200000000

/* Decoder interrupt register */
#define X170_INTERRUPT_REGISTER_DEC (1*4)
#define X170_INTERRUPT_REGISTER_PP (60*4)

#define HX_DEC_INTERRUPT_BIT        0x100
#define HX_PP_INTERRUPT_BIT         0x100

static const int DecHwId[] = { 0x8190, 0x8170, 0x9170, 0x9190, 0x6731 };

static u32 hx_pp_instance;
static u32 hx_dec_instance;

/* and this is our MAJOR; use 0 for dynamic allocation (recommended)*/
static int hx170dec_major;

/* here's all the must remember stuff */
typedef struct {
	char *buffer;
	unsigned long iobaseaddr;
	unsigned int iosize;
	volatile u8 *hwregs;
	int open_count;
	unsigned int irq;
	unsigned int done;
	wait_queue_head_t dec_queue;
	struct semaphore busy_lock;
	struct fasync_struct *async_queue_dec;
	struct fasync_struct *async_queue_pp;
	struct cdev cdev;
	struct device dev;
	struct clk *clock;
	struct regulator *reg;
} hx170dec_t;

static hx170dec_t hx170dec_data;    /* dynamic allocation? */

static int ReserveIO(void);
static void ReleaseIO(void);
static void ResetAsic(hx170dec_t *dev);

#ifdef HX170DEC_DEBUG
static void dump_regs(unsigned long data);
#endif

/* IRQ handler */
static irqreturn_t hx170dec_isr(int irq, void *dev_id);

/*------------------------------------------------------------------------------
    Function name   : hx170dec_ioctl
    Description     : communication method to/from the user space

    Return type     : int
------------------------------------------------------------------------------*/

static long hx170dec_ioctl(struct file *filp,
                           unsigned int cmd, unsigned long arg)
{
	int err = 0;
	int ret = 0;

	/*PDEBUG("ioctl cmd 0x%08ux\n", cmd); */
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != HX170DEC_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > HX170DEC_IOC_MAXNR)
		return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	switch (cmd) {
		case HX170DEC_IOC_CLI:
			disable_irq(hx170dec_data.irq);
			break;

		case HX170DEC_IOC_STI:
			enable_irq(hx170dec_data.irq);
			break;
		case HX170DEC_IOCGHWOFFSET:
			__put_user(hx170dec_data.iobaseaddr, (unsigned long *)arg);
			break;
		case HX170DEC_IOCGHWIOSIZE:
			__put_user(hx170dec_data.iosize, (unsigned int *)arg);
			break;
		case HX170DEC_PP_INSTANCE:
			filp->private_data = &hx_pp_instance;
			break;
		case HX170DEC_IOC_WAIT_DONE:
			/* timeout: arg second */
			if (!wait_event_interruptible_timeout(hx170dec_data.dec_queue,
			                                      hx170dec_data.done,
			                                      arg * HZ)) {
				printk(KERN_INFO "hx170dec wait event timeout\n");
				return -ETIME;
			} else if (signal_pending(current)) {
				printk(KERN_INFO "signal pending\n");
				hx170dec_data.done = 0;

				return -ERESTARTSYS;
			}

			hx170dec_data.done = 0;
			break;

		case HX170DEC_IOC_CLOCK_ON:
			ret = regulator_enable(hx170dec_data.reg);
			if ( ret ) {
				printk(KERN_ERR "hx170dec: regulator_enable failed\n");
				return ret;
			}

			ret = clk_enable(hx170dec_data.clock);
			if ( ret ) {
				/* Clk enable fail , power off*/
				printk(KERN_ERR "hx170dec: clk_enable failed\n");
				regulator_disable(hx170dec_data.reg);
				return ret;
			}
			udelay(1);
			break;

		case HX170DEC_IOC_CLOCK_OFF:
			regulator_disable(hx170dec_data.reg);
			clk_disable(hx170dec_data.clock);
			break;

		case HX170DEC_IOCHARDRESET:
			/*
			 * reset the counter to 1, to allow unloading in case
			 * of problems. Use 1, not 0, because the invoking
			 * process has the device open.
			 */
			module_put(THIS_MODULE);
			break;
		default:
			break;
	}
	return ret;
}

/*------------------------------------------------------------------------------
    Function name   : hx170dec_open
    Description     : open method

    Return type     : int
------------------------------------------------------------------------------*/

static int hx170dec_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

	if (down_interruptible(&(hx170dec_data.busy_lock)))
		return -EINTR;

	hx170dec_data.open_count++;

	filp->private_data = &hx_dec_instance;

	PDEBUG("dev opened\n");

	up(&(hx170dec_data.busy_lock));
	return 0;
}

/*------------------------------------------------------------------------------
    Function name   : hx170dec_fasync
    Description     : Method for signing up for a interrupt

    Return type     : int
------------------------------------------------------------------------------*/

static int hx170dec_fasync(int fd, struct file *filp, int mode)
{

	hx170dec_t *dev = &hx170dec_data;

	/*PDEBUG("dec %x pp %x this %x\n",
	 * &hx_dec_instance,
	 * &hx_pp_instance,
	 * (u32 *)filp->private_data); */

	/* select which interrupt this instance will sign up for */

	if (((u32 *) filp->private_data) == &hx_dec_instance) {
		/* decoder interrupt */
		PDEBUG("decoder fasync called %d %x %d %x\n",
		       fd, (u32) filp, mode, (u32)&dev->async_queue_dec);
		return fasync_helper(fd, filp, mode, &dev->async_queue_dec);
	} else {
		/* pp interrupt */
		PDEBUG("pp fasync called %d %x %d %x\n",
		       fd, (u32) filp, mode, (u32)&dev->async_queue_dec);
		return fasync_helper(fd, filp, mode, &dev->async_queue_pp);
	}
}

/*------------------------------------------------------------------------------
    Function name   : hx170dec_release
    Description     : Release driver

    Return type     : int
------------------------------------------------------------------------------*/

static int hx170dec_release(struct inode *inode, struct file *filp)
{
	if (down_interruptible(&(hx170dec_data.busy_lock)))
		return -EINTR;

	/* remove this filp from the asynchronusly notified filp's */
	/*hx170dec_fasync(-1, filp, 0); */

	if (--hx170dec_data.open_count <= 0) {
		hx170dec_data.done = 0;
		PDEBUG("dev closed\n");
	}

	up(&(hx170dec_data.busy_lock));

	return 0;
}

static int CheckHwId(hx170dec_t *dev)
{
	long int hwid;

	size_t numHw = sizeof(DecHwId) / sizeof(*DecHwId);

	hwid = readl(dev->hwregs);
	printk(KERN_INFO "hx170dec: HW ID=0x%08lx\n", hwid);

	hwid = (hwid >> 16) & 0xFFFF;   /* product version only */

	while (numHw--) {
		if (hwid == DecHwId[numHw]) {
			printk(KERN_INFO
			       "hx170dec: Compatible HW found at 0x%08lx\n",
			       dev->iobaseaddr);
			return 1;
		}
	}

	printk(KERN_INFO "hx170dec: No Compatible HW found at 0x%08lx\n",
	       dev->iobaseaddr);
	return 0;
}

/*------------------------------------------------------------------------------
    Function name   : ReserveIO
    Description     : IO reserve

    Return type     : int
------------------------------------------------------------------------------*/
static int ReserveIO(void)
{
	if (!request_mem_region
	    (hx170dec_data.iobaseaddr, hx170dec_data.iosize, "hx170dec")) {
		printk(KERN_INFO "hx170dec: failed to reserve HW regs\n");
		return -EBUSY;
	}

	hx170dec_data.hwregs =
	    (volatile u8 *)ioremap_nocache(hx170dec_data.iobaseaddr,
	                                   hx170dec_data.iosize);

	if (hx170dec_data.hwregs == NULL) {
		printk(KERN_INFO "hx170dec: failed to ioremap HW regs\n");
		ReleaseIO();
		return -EBUSY;
	}

	/* check for correct HW */
	if (!CheckHwId(&hx170dec_data)) {
		ReleaseIO();
		return -EBUSY;
	}

	return 0;
}

/*------------------------------------------------------------------------------
    Function name   : releaseIO
    Description     : release

    Return type     : void
------------------------------------------------------------------------------*/

static void ReleaseIO(void)
{
	if (hx170dec_data.hwregs)
		iounmap((void *)hx170dec_data.hwregs);
	release_mem_region(hx170dec_data.iobaseaddr, hx170dec_data.iosize);
}

/*------------------------------------------------------------------------------
    Function name   : hx170dec_isr
    Description     : interrupt handler

    Return type     : irqreturn_t
------------------------------------------------------------------------------*/

static irqreturn_t hx170dec_isr(int irq, void *dev_id)
{
	unsigned int handled = 0;

	hx170dec_t *dev = (hx170dec_t *) dev_id;
	u32 irq_status_dec;
	u32 irq_status_pp;

	handled = 0;

	/* interrupt status register read */
	irq_status_dec = readl(dev->hwregs + X170_INTERRUPT_REGISTER_DEC);
	irq_status_pp = readl(dev->hwregs + X170_INTERRUPT_REGISTER_PP);

	if ((irq_status_dec & HX_DEC_INTERRUPT_BIT) ||
	    (irq_status_pp & HX_PP_INTERRUPT_BIT)) {
		if (irq_status_dec & HX_DEC_INTERRUPT_BIT) {
			/* clear dec IRQ */
			writel(irq_status_dec & (~HX_DEC_INTERRUPT_BIT),
			       dev->hwregs + X170_INTERRUPT_REGISTER_DEC);
			/* fasync kill for decoder instances */
			if (dev->async_queue_dec != NULL) {
				kill_fasync(&dev->async_queue_dec,
				            SIGIO, POLL_IN);
			} else {
				/*printk(KERN_WARNING "x170: IRQ received w/o
				 * anybody waiting for it!\n");
				 */
			}
			PDEBUG("decoder IRQ received!\n");
			hx170dec_data.done = 1;
			wake_up_interruptible(&hx170dec_data.dec_queue);
		}

		if (irq_status_pp & HX_PP_INTERRUPT_BIT) {
			/* clear pp IRQ */
			writel(irq_status_pp & (~HX_PP_INTERRUPT_BIT),
			       dev->hwregs + X170_INTERRUPT_REGISTER_PP);

			/* kill fasync for PP instances */
			if (dev->async_queue_pp != NULL) {
				kill_fasync(&dev->async_queue_pp,
				            SIGIO, POLL_IN);
			} else {
				printk(KERN_WARNING
				       "x170: IRQ received w/o anybody waiting for it!\n");
			}
			/*PDEBUG("pp IRQ received!\n"); */
			hx170dec_data.done = 1;
			wake_up_interruptible(&hx170dec_data.dec_queue);
		}

		handled = 1;
	} else {
		PDEBUG("IRQ received, but not x170's!\n");
	}

	return IRQ_RETVAL(handled);
}

/*------------------------------------------------------------------------------
    Function name   : ResetAsic
    Description     : reset asic

    Return type     :
------------------------------------------------------------------------------*/

void ResetAsic(hx170dec_t *dev)
{
	int i;

	writel(0, dev->hwregs + 0x04);

	for (i = 4; i < dev->iosize; i += 4)
		writel(0, dev->hwregs + i);
}

/*------------------------------------------------------------------------------
    Function name   : dump_regs
    Description     : Dump registers

    Return type     :
------------------------------------------------------------------------------*/
#ifdef HX170DEC_DEBUG
void dump_regs(unsigned long data)
{
	hx170dec_t *dev = (hx170dec_t *) data;
	int i;

	PDEBUG("Reg Dump Start\n");
	for (i = 0; i < dev->iosize; i += 4)
		PDEBUG("\toffset %02X = %08X\n", i, readl(dev->hwregs + i));

	PDEBUG("Reg Dump End\n");
}
#endif

static int hx170dec_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	int retval = 0;

	unsigned long pyhs_start = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long pyhs_end = pyhs_start + size;
	if(!(pyhs_start >= hisi_reserved_codec_phymem//not codec memory
			&& pyhs_end <= hisi_reserved_codec_phymem + HISI_MEM_CODEC_SIZE)
		&& !(pyhs_start >= hx170dec_data.iobaseaddr//not io address
			&& pyhs_end <= hx170dec_data.iobaseaddr + hx170dec_data.iosize)) {
		printk(KERN_ERR "%s(%d) failed map:0x%lx-0x%lx\n", __FUNCTION__, __LINE__, pyhs_start, pyhs_end);
		return -EFAULT;
	}

	/* make buffers write-thru cacheable */

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_pfn_range(vma, start, vma->vm_pgoff, size, vma->vm_page_prot))
		retval = -ENOBUFS;

	return retval;
}

/* VFS methods */
static const struct file_operations hx170dec_fops = {
open:
	hx170dec_open,
release :
	hx170dec_release,
unlocked_ioctl :
	hx170dec_ioctl,
fasync :
	hx170dec_fasync,
mmap :
	hx170dec_mmap,
};

static void hx170dec_dev_release(struct device *dev)
{
	return;
}

static ssize_t show_index(struct device *cd,
                          struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0");
}

static ssize_t show_name(struct device *cd,
                         struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "hx170dec\n");
}

static struct device_attribute dec_attrs[] = {
	__ATTR(name, S_IRUGO, show_index, NULL),
	__ATTR(index, S_IRUGO, show_name, NULL),
	__ATTR_NULL
};

static struct class dec_class = {
		.name = "hx170dec",
		.dev_attrs = dec_attrs,
	};

/*------------------------------------------------------------------------------
    Function name   : hx170dec_dev_probe
    Description     : probe hx170 dec device

    Return type     : int
------------------------------------------------------------------------------*/

static int hx170dec_dev_probe(struct platform_device *pdev)
{
	struct resource *res;
	int result;
	dev_t dev;
	int devno;
	int ret = 0;

	hx170dec_data.clock = NULL;
	hx170dec_data.clock = clk_get(NULL, "clk_vdec");

	if (IS_ERR(hx170dec_data.clock)) {
		printk(KERN_ERR "hx170dec: hx170dec_dev_probe get dec clock failed\n");
		ret = PTR_ERR( hx170dec_data.clock );
		return ret;
	}

	hx170dec_data.reg = NULL;
	hx170dec_data.reg= regulator_get( NULL ,"vcc_vdec");
	if (IS_ERR(hx170dec_data.reg)) {
		printk(KERN_ERR "hx170dec: hx170dec_dev_probe get dec regulator failed\n");
		ret = PTR_ERR( hx170dec_data.reg );
		return ret;
	}

	/*Clock on*/
	ret = regulator_enable(hx170dec_data.reg);
	if ( ret ) {
		printk(KERN_ERR "hx170dec: hx170dec_dev_probe regulator_enable failed\n");
	}

	ret = clk_set_rate( hx170dec_data.clock , DECODER_CLOCK);
	if ( ret ) {
		printk(KERN_ERR "hx170dec: hx170dec_dev_probe clk_set_rate failed\n");
	}

	ret = clk_enable(hx170dec_data.clock);
	if ( ret ) {
		printk(KERN_ERR "hx170dec: hx170dec_dev_probe clk_enable failed\n");
	}

	/* get io mem resource */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (NULL == res) {
		printk(KERN_ERR "hx170dec: get io mem resource failed\n");
		return -ENODEV;
	}
	hx170dec_data.iobaseaddr = res->start;
	hx170dec_data.iosize = res->end - res->start + 1;

	/* get irq resource */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	hx170dec_data.irq = res->start;

	/* get kernel virtual address */
	result = ReserveIO();
	if (result < 0) {
		ReleaseIO();
		return result;
	}

	/* reset hardware */
	ResetAsic(&hx170dec_data);

	/* request irq */
	result = request_irq(hx170dec_data.irq, hx170dec_isr,
	                     IRQF_DISABLED | IRQF_SHARED, "hx170dec",
	                     (void *)&hx170dec_data);
	if (result < 0) {
		printk(KERN_ERR "hx170dec: request irq failed\n");
		ReleaseIO();
		return result;
	}

	/* dymamic alloc major */
	result = alloc_chrdev_region(&dev, 0, 1, "hx170dec");
	if (result < 0) {
		printk(KERN_INFO "hx170dec: unable to get major %d\n",
		       hx170dec_major);
		ReleaseIO();
		return result;
	}
	hx170dec_major = MAJOR(dev);

	printk(KERN_INFO "hx170dec_major is %d\n", hx170dec_major);

	/* register cdev */
	devno = MKDEV(hx170dec_major, 0);
	cdev_init(&hx170dec_data.cdev, &hx170dec_fops);
	hx170dec_data.cdev.owner = THIS_MODULE;
	result = cdev_add(&hx170dec_data.cdev, devno, 1);
	if (result < 0) {
		printk(KERN_ERR "hx170dec: add cdev failed\n");
		free_irq(hx170dec_data.irq, &hx170dec_data);
		ReleaseIO();
		return result;
	}

	/* register sysfs */
	result = class_register(&dec_class);
	if (result < 0) {
		printk(KERN_ERR "hx170dec: add cdev failed\n");
		free_irq(hx170dec_data.irq, &hx170dec_data);
		ReleaseIO();
		cdev_del(&hx170dec_data.cdev);
		return result;
	}
	/*hx170dec_data.dev.driver_data = &hx170dec_data; */
	hx170dec_data.dev.class = &dec_class;
	hx170dec_data.dev.devt = MKDEV(hx170dec_major, 0);
	hx170dec_data.dev.release = hx170dec_dev_release;
	dev_set_name(&hx170dec_data.dev, "%s", "hx170dec");
	result = device_register(&hx170dec_data.dev);
	if (result < 0) {
		printk(KERN_ERR "hx170dec: add cdev failed\n");
		free_irq(hx170dec_data.irq, &hx170dec_data);
		ReleaseIO();
		cdev_del(&hx170dec_data.cdev);
		return result;
	}

	init_waitqueue_head(&hx170dec_data.dec_queue);

	/*Clock off*/
	clk_disable(hx170dec_data.clock);
	regulator_disable(hx170dec_data.reg);

	printk(KERN_ERR "hx170dec: add cdev OK\n");
	return 0;
}


static struct platform_driver hx170dec_driver = {
	.driver = {
		.name = "hx170dec",
	},
	.probe = hx170dec_dev_probe,
 };

/*------------------------------------------------------------------------------
    Function name   : hx170dec_init
    Description     : Initialize the driver

    Return type     : int
------------------------------------------------------------------------------*/
int __init hx170dec_init(void)
{
	int result;

	result = platform_driver_register(&hx170dec_driver);

	sema_init(&(hx170dec_data.busy_lock), 1);

	return result;
}

/*------------------------------------------------------------------------------
    Function name   : hx170dec_cleanup
    Description     : clean up

    Return type     : int
------------------------------------------------------------------------------*/

void __exit hx170dec_cleanup(void)
{
	hx170dec_t *dev = (hx170dec_t *)&hx170dec_data;

	/* y44207 revived : Clock off state , Can't write register*/
#if 0
	/* clear dec IRQ */
	writel(0, hx170dec_data.hwregs + X170_INTERRUPT_REGISTER_DEC);
	/* clear pp IRQ */
	writel(0, hx170dec_data.hwregs + X170_INTERRUPT_REGISTER_PP);
#endif

#ifdef HX170DEC_DEBUG
	dump_regs((unsigned long)dev);  /* dump the regs */
#endif

	/* free the encoder IRQ */
	free_irq(dev->irq, (void *)&hx170dec_data);
	cdev_del(&dev->cdev);
	ReleaseIO();
	platform_driver_unregister(&hx170dec_driver);

	clk_put(hx170dec_data.clock);
	regulator_put(hx170dec_data.reg);

	printk(KERN_INFO "hx170dec: module removed\n");
	return;
}

module_init(hx170dec_init);
module_exit(hx170dec_cleanup);

/* module description */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hantro Products Oy");
MODULE_DESCRIPTION("hx170dec - driver module for Hantro x170 decoder");
