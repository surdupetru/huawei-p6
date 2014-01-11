/* 
 * Encoder device driver (kernel module)
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
--------------------------------------------------------------------------------
--
--  Abstract : 6280/7280/8270/8290 Encoder device driver (kernel module)
--
------------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <asm/pgtable.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

#include <linux/wait.h>
#include <linux/clk.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

#include "mach/platform.h"
#include "mach/irqs.h"
#include "mach/hisi_mem.h"

/* our own stuff */
#include "hx280enc.h"

#define MODULE_NAME "hx280enc"

#define ENCODER_CLOCK 280000000

#define HIENC8290_ERR(fmt, args...) \
    do {\
        printk(KERN_ERR MODULE_NAME ": " fmt "\n", ## args); \
    } while (0)

#define HIENC8290_WARN(fmt, args...) \
    do {\
        printk(KERN_WARNING MODULE_NAME ": " fmt "\n", ## args); \
    } while (0)

#define HIENC8290_INFO(fmt, args...) \
    do {\
        printk(KERN_INFO MODULE_NAME ": " fmt "\n", ## args); \
    } while (0)

#ifdef HI8290_DEBUG
#define HIENC8290_DEBUG(fmt, args...) \
    do {\
        printk(KERN_DEBUG MODULE_NAME ": " fmt "\n", ## args); \
    } while (0)
#else
#define HIENC8290_DEBUG(fmt, args...)
#endif

/* these could be module params in the future */
#define ENC_HW_ID1                  0x62800000
#define ENC_HW_ID2                  0x72800000
#define ENC_HW_ID3                  0x82700000
#define ENC_HW_ID4                  0x82900000
#define ENC_IRQ_REG_OFFSET          0x04

/* and this is our MAJOR; use 0 for dynamic allocation (recommended)*/
static int hx280enc_major;
static int g_enc_done;
static unsigned long virtbase;

/* here's all the must remember stuff */
typedef struct {
	struct semaphore sem;
	unsigned long iobaseaddr;
	unsigned int iosize;
	volatile u8 *hwregs;
	int open_count;
	unsigned int irq;
	struct semaphore busy_lock;
	struct fasync_struct *async_queue;
	wait_queue_head_t enc_done_queue;
	struct cdev cdev;
	struct device dev;
	struct clk *clock;
	struct regulator *reg;
} hx280enc_t;

static hx280enc_t hx280enc_data;    /* dynamic allocation? */

static int ReserveIO(void);
static void ReleaseIO(void);
static void ResetAsic(hx280enc_t *dev);
static irqreturn_t hx280enc_isr(int irq, void *dev_id);
extern int pmem_kalloc(const size_t size, const uint32_t id);
extern int pmem_kfree(const uint32_t id, const int32_t physaddr);

static int hx280enc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	int retval = 0;

	unsigned long pyhs_start = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long pyhs_end = pyhs_start + size;
	if (!(pyhs_start >= hisi_reserved_codec_phymem//not codec memory
			&& pyhs_end <= hisi_reserved_codec_phymem + HISI_MEM_CODEC_SIZE)
		&& !(pyhs_start >= hx280enc_data.iobaseaddr//not io address
			&& pyhs_end <= hx280enc_data.iobaseaddr + hx280enc_data.iosize)){
		printk(KERN_ERR "%s(%d) failed map:0x%lx-0x%lx\n", __FUNCTION__, __LINE__, pyhs_start, pyhs_end);
		return -EFAULT;
	}

	/* make buffers write-thru cacheable */

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_pfn_range(vma, start, vma->vm_pgoff, size, vma->vm_page_prot)) {
		retval = -ENOBUFS;
	}

	return retval;
}

static long hx280enc_ioctl(struct file *filp,
                           unsigned int cmd, unsigned long arg)
{
	int err = 0;
	int ret = 0;
	hx280enc_t *encdev;

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != HX280ENC_IOC_MAGIC) {
		return -ENOTTY;
	}

	if (_IOC_NR(cmd) > HX280ENC_IOC_MAXNR) {
		return -ENOTTY;
	}

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	}
	if (err) {
		HIENC8290_ERR("io ctrl access check failed\n");
		return -EFAULT;
	}

	encdev = (hx280enc_t *) (filp->private_data);
	if (NULL == encdev) {
		HIENC8290_ERR("get vout semphore failed\n");
		return -EINVAL;
	}

	/* make this multi process safe */
	if (down_interruptible(&encdev->sem)) {
		HIENC8290_ERR("get vout semphore failed\n");
		return -ERESTARTSYS;
	}

	switch (cmd) {
		case HX280ENC_IOCGHWOFFSET: {
			__put_user(hx280enc_data.iobaseaddr,
			           (unsigned long *)arg);
			break;
		}

		case HX280ENC_IOCGHWIOSIZE: {
			__put_user(hx280enc_data.iosize, (unsigned int *)arg);
			break;
		}

		case HX280ENC_IOC_CLI: {
			/*disable_irq(h5650enc_data.irq); */
			break;
		}

		case HX280ENC_IOC_STI: {
			/*enable_irq(h5650enc_data.irq); */
			break;
		}

		case HX280ENC_IOCHARDRESET: {
			/*
			 * reset the counter to 1, to allow unloading in case
			 * of problems. Use 1, not 0, because the invoking
			 * process has the device open.
			 */
			break;
		}

		case HX280ENC_IOCVIRTBASE: {
			virtbase = (unsigned long)arg;
			break;
		}

		case HX280ENC_IOCWAITDONE: {
			if (!wait_event_interruptible_timeout
			    (encdev->enc_done_queue, g_enc_done, HZ)) {
				g_enc_done = 0;
				err = -ETIME;
				printk("hx280enc: wait timeout\n");
				break;
			} else if (signal_pending(current)) {
				err = -ERESTARTSYS;
				printk("hx280enc: signal pending\n");
				break;
			}

			g_enc_done = 0;
			break;
		}
		case HX280ENC_IOC_CLOCK_ON:
#if 0
			ret = regulator_enable(hx280enc_data.reg);
			if ( ret ) {
				printk(KERN_ERR "hx280enc: regulator_enable failed\n");
			}

			ret = clk_enable(hx280enc_data.clock);
			if ( ret ) {
				printk(KERN_ERR "hx280enc: clk_enable failed\n");
			}
#endif
			break;

		case HX280ENC_IOC_CLOCK_OFF:
#if 0
			clk_disable(hx280enc_data.clock);
			regulator_disable( hx280enc_data.reg );
#endif
			break;

		default:
			break;
	}

	up(&encdev->sem);
	return err;
}

static int hx280enc_open(struct inode *inode, struct file *filp)
{
	hx280enc_t *dev = &hx280enc_data;
	int ret = 0;
	
	/*clock on*/
	ret = regulator_enable(hx280enc_data.reg);
	if ( ret ) {
		printk(KERN_ERR "hx280enc: hx280enc_open regulator_enable failed\n");
	}


	ret = clk_enable(hx280enc_data.clock);
	if ( ret ) {
		printk(KERN_ERR "hx280enc: hx280enc_open clk_enable failed\n");
	}
	
	if (down_interruptible(&(hx280enc_data.busy_lock))) {
		return -EINTR;
	}

	filp->private_data = (void *)dev;

	HIENC8290_DEBUG("dev opened\n");

	up(&(hx280enc_data.busy_lock));
	return 0;
}

#if 0
static int hx280enc_fasync(int fd, struct file *filp, int mode)
{
	/*hx280enc_t *dev = (hx280enc_t *) filp->private_data;

	   HIENC8290_ERR("fasync called\n");

	   return fasync_helper(fd, filp, mode, &dev->async_queue); */
	return 0;
}
#endif

#ifdef HI8290_DEBUG
static void dump_regs(unsigned long data);
#endif

static int hx280enc_release(struct inode *inode, struct file *filp)
{
	hx280enc_t *dev = (hx280enc_t *) filp->private_data;

	if (down_interruptible(&(hx280enc_data.busy_lock))) {
		return -EINTR;
	}

	/* this is necessary if user process exited asynchronously */
	/*disable_irq(dev->irq); */
#ifdef HI8290_DEBUG
	dump_regs((unsigned long)dev);  /* dump the regs */
#endif

	/*y44207 deleted: Clock off state , Can't set register*/
	//ResetAsic(dev);     /* reset hardware */

	/* remove this filp from the asynchronusly notified filp's */
	/*hx280enc_fasync(-1, filp, 0); */

	HIENC8290_DEBUG("dev closed\n");

	up(&(hx280enc_data.busy_lock));

	/*clock off*/
	clk_disable(hx280enc_data.clock);
	regulator_disable( hx280enc_data.reg );
	
	return 0;
}

/* VFS methods */
static struct file_operations hx280enc_fops = {
mmap:
	hx280enc_mmap,
open :
	hx280enc_open,
release :
	hx280enc_release,
unlocked_ioctl :
	hx280enc_ioctl,
};

static int ReserveIO(void)
{   
	long int hwid;

	if (!request_mem_region
	    (hx280enc_data.iobaseaddr, hx280enc_data.iosize, "hx280enc")) {
		HIENC8290_ERR("hx280enc: failed to reserve HW regs\n");
		return -EBUSY;
	}

	hx280enc_data.hwregs =
	    (volatile u8 *)ioremap_nocache(hx280enc_data.iobaseaddr,
	                                   hx280enc_data.iosize);

	if (hx280enc_data.hwregs == NULL) {
		HIENC8290_ERR("hx280enc: failed to ioremap HW regs\n");
		ReleaseIO();
		return -EBUSY;
	}

	hwid = readl(hx280enc_data.hwregs);

	/* check for encoder HW ID */
	if ((((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID1 >> 16) & 0xFFFF)) &&
	    (((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID2 >> 16) & 0xFFFF)) &&
	    (((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID3 >> 16) & 0xFFFF)) &&
	    (((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID4 >> 16) & 0xFFFF))) {
		HIENC8290_ERR("hx280enc: HW not found at 0x%08lx\n",
		              hx280enc_data.iobaseaddr);
#ifdef HX280ENC_DEBUG
		dump_regs((unsigned long)&hx280enc_data);
#endif
		ReleaseIO();
		return -EBUSY;
	} else {
		HIENC8290_ERR
		("hx280enc: Valid HW found at base <0x%08lx> with ID <0x%08lx>\n",
		 hx280enc_data.iobaseaddr, hwid);
	}

	return 0;

}

static void ReleaseIO(void)
{
	if (hx280enc_data.hwregs) {
		iounmap((void *)hx280enc_data.hwregs);
		hx280enc_data.hwregs = 0;
	}
	release_mem_region(hx280enc_data.iobaseaddr, hx280enc_data.iosize);
}

static irqreturn_t hx280enc_isr(int irq, void *dev_id)
{
	hx280enc_t *dev = (hx280enc_t *) dev_id;
	u32 irq_status;

	irq_status = readl(dev->hwregs + ENC_IRQ_REG_OFFSET);

	if (irq_status & 0x01) {

		/* clear enc IRQ and slice ready interrupt bit */
		writel(irq_status & (~0x101), dev->hwregs + ENC_IRQ_REG_OFFSET);

		/* Handle slice ready interrupts. The reference implementation
		 * doesn't signal slice ready interrupts to EWL.
		 * The EWL will poll the slices ready register value. */
		if ((irq_status & 0x1FE) == 0x100) {
			HIENC8290_ERR("Slice ready IRQ handled!\n");
			return IRQ_HANDLED;
		}

		/* All other interrupts will be signaled to EWL. */
		if (dev->async_queue) {
			kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
		} else {
			HIENC8290_DEBUG
			("hx280enc: IRQ received w/o anybody waiting for it!\n");
		}

		g_enc_done = 1;
		wake_up_interruptible(&dev->enc_done_queue);

		HIENC8290_DEBUG("IRQ handled!\n");
		return IRQ_HANDLED;
	} else {
		HIENC8290_DEBUG("IRQ received, but NOT handled!\n");
		return IRQ_NONE;
	}

}

void ResetAsic(hx280enc_t *dev)
{   
	int i;

	writel(0, dev->hwregs + 0x38);

	for (i = 4; i < dev->iosize; i += 4) {
		writel(0, dev->hwregs + i);
	}
}

#ifdef HX280ENC_DEBUG
void dump_regs(unsigned long data)
{
	hx280enc_t  *dev = (hx280enc_t *) data;
	int i;

	HIENC8290_DEBUG("Reg Dump Start\n");
	for (i = 0; i < dev->iosize; i += 4) {
		HIENC8290_DEBUG("\toffset %02X = %08X\n", i,
		                readl(dev->hwregs + i));
	}
	HIENC8290_DEBUG("Reg Dump End\n");
}
#endif

static void hx280enc_dev_release(struct device *dev)
{
	return;
}

static struct device_attribute dec_attrs[] = {
	__ATTR(name, S_IRUGO, NULL, NULL),
	__ATTR(index, S_IRUGO, NULL, NULL),
	__ATTR_NULL
};

static struct class enc_class = {
		.name = "hx280enc",
		.dev_attrs = dec_attrs,
	};

static int hx280enc_dev_probe(struct platform_device *pdev)
{
	struct resource *res;
	int result;
	dev_t dev;
	int devno;
	int ret = 0;

	hx280enc_data.clock = NULL;
	hx280enc_data.clock = clk_get(NULL, "clk_venc");

	if (IS_ERR(hx280enc_data.clock)) {
		printk(KERN_ERR "hx280enc: hx280enc_dev_probe get enc clock failed\n");
		ret = PTR_ERR( hx280enc_data.clock );
		return ret;
	}

	hx280enc_data.reg = NULL;
	hx280enc_data.reg= regulator_get( NULL ,"vcc_venc");

	if (IS_ERR(hx280enc_data.reg)) {
		printk(KERN_ERR "hx280enc: hx280enc_dev_probe get enc regulator failed\n");
		ret = PTR_ERR( hx280enc_data.reg );
		return ret;
	}

	/*clock on*/
	ret = regulator_enable(hx280enc_data.reg);
	if ( ret ) {
		printk(KERN_ERR "hx280enc: hx280enc_dev_probe regulator_enable failed\n");
	}

	ret = clk_set_rate( hx280enc_data.clock , ENCODER_CLOCK);
	if ( ret ) {
		printk(KERN_ERR "hx280enc: hx280enc_dev_probe clk_set_rate failed\n");
	}

	ret = clk_enable(hx280enc_data.clock);
	if ( ret ) {
		printk(KERN_ERR "hx280enc: hx280enc_dev_probe clk_enable failed\n");
	}


	/* get io mem resource */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (NULL == res) {
		HIENC8290_ERR("hx280enc: get io mem resource failed\n");
		return -ENODEV;
	}
	hx280enc_data.iobaseaddr = res->start;
	hx280enc_data.iosize = res->end - res->start + 1;

	/* get irq resource */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	hx280enc_data.irq = res->start;

	/* request irq */
	result = request_irq(hx280enc_data.irq, hx280enc_isr, IRQF_DISABLED, "hx280enc",
	                     (void *)&hx280enc_data);

	if (result < 0) {
		HIENC8290_ERR("hx280enc: request irq failed\n");
		ReleaseIO();
		return result;
	}

	/* get kernel virtual address */
	result = ReserveIO();
	if (result < 0) {
		ReleaseIO();
		free_irq(hx280enc_data.irq, &hx280enc_data);
		return result;
	}

	/* reset hardware */
	ResetAsic(&hx280enc_data);

	/* dymamic alloc major */
	result = alloc_chrdev_region(&dev, 0, 1, "hx280enc");
	if (result < 0) {
		HIENC8290_ERR("hx280enc: unable to get major %d\n",
		              hx280enc_major);
		ReleaseIO();
		free_irq(hx280enc_data.irq, &hx280enc_data);
		return result;
	}
	hx280enc_major = MAJOR(dev);

	/* init semphore */
	sema_init(&(hx280enc_data.sem) , 1);

	/* init wait queue */
	init_waitqueue_head(&(hx280enc_data.enc_done_queue));

	/* register cdev */
	devno = MKDEV(hx280enc_major, 0);
	cdev_init(&hx280enc_data.cdev, &hx280enc_fops);
	hx280enc_data.cdev.owner = THIS_MODULE;
	result = cdev_add(&hx280enc_data.cdev, devno, 1);
	if (result < 0) {
		HIENC8290_ERR("hx280enc: add cdev failed\n");
		free_irq(hx280enc_data.irq, &hx280enc_data);
		ReleaseIO();
		return result;
	}

	memset(&hx280enc_data.dev, 0, sizeof(hx280enc_data.dev));

	class_register(&enc_class);

	/*hx280enc_data.dev.driver_data = &hx280enc_data; */
	hx280enc_data.dev.class = &enc_class;
	dev_set_name(&hx280enc_data.dev, "%s", "hx280enc");
	hx280enc_data.dev.devt = MKDEV(hx280enc_major, 0);
	hx280enc_data.dev.release = hx280enc_dev_release;
	result = device_register(&hx280enc_data.dev);

	HIENC8290_DEBUG("register driver ok,major is %d,result %d\n",
	                hx280enc_major, result);

	/*clock off*/
	clk_disable(hx280enc_data.clock);
	regulator_disable( hx280enc_data.reg );

	return 0;
}
static struct platform_driver hx280enc_driver = {
	.driver = {
		.name = "hx280enc",
	},
	.probe = hx280enc_dev_probe,
 };

int __init hx280enc_init(void)
{
	int result;

	HIENC8290_DEBUG("hx280enc_ init\n");

	result = platform_driver_register(&hx280enc_driver);

	sema_init(&(hx280enc_data.busy_lock) , 1);

	return result;
}

void __exit hx280enc_cleanup(void)
{
	/* free the encoder IRQ */
	free_irq(hx280enc_data.irq, &hx280enc_data);
	cdev_del(&hx280enc_data.cdev);
	device_unregister(&hx280enc_data.dev);
	ReleaseIO();

	platform_driver_unregister(&hx280enc_driver);

	clk_put(hx280enc_data.clock);
	regulator_put(hx280enc_data.reg);

	return;
}

module_init(hx280enc_init);
module_exit(hx280enc_cleanup);

/* module description */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hantro Products Oy");
MODULE_DESCRIPTION("hx280enc - driver module for Hantro 8090 encoder");
