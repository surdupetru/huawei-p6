#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#include <mach/boardid.h>

#define BOARDID_NAME "boardid_dev"
#define BOARDID_IOCTL_MAGIC 'u'

#define BOARDID_READ_DATA    _IOR(BOARDID_IOCTL_MAGIC, 0x10, unsigned)

enum BOARDID_STATUS {
	BOARDID_CLOSE,
	BOARDID_OPENED,
};

static enum BOARDID_STATUS boardid_status = BOARDID_CLOSE;
static struct mutex boardid_lock;

extern void hi6421_check_pll(void);

static int boardid_open(struct inode *inode, struct file *file)
{
	mutex_lock(&boardid_lock);

	if (BOARDID_OPENED == boardid_status) {
		pr_err("%s: busy\n", __func__);
	    mutex_unlock(&boardid_lock);
		return -EBUSY;
	}
	boardid_status = BOARDID_OPENED;

	mutex_unlock(&boardid_lock);
	return 0;
}

static int boardid_release(struct inode *inode, struct file *file)
{
	mutex_lock(&boardid_lock);
	boardid_status = BOARDID_CLOSE;
	mutex_unlock(&boardid_lock);
	return 0;
}

static long boardid_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	unsigned int boardid = 0;
	int ret = 0;

	switch (cmd) {
    case BOARDID_READ_DATA:
    	boardid = get_boardid();
    	pr_info("%s: get boardid : %d\n", __func__, boardid);
    	if (copy_to_user(argp, &boardid, sizeof(int))){
        	pr_err("%s: copy_to_user error\n", __func__);
            ret = -EFAULT;
        }
    	break;
    default:
		pr_err("%s: invalid command %d\n", __func__, _IOC_NR(cmd));
    	break;
    }

    return ret;
}



static const struct file_operations boardid_fops = {
    .owner          = THIS_MODULE,
    .open           = boardid_open,
    .release        = boardid_release,
    .unlocked_ioctl = boardid_ioctl,
};

static struct miscdevice boardid_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = BOARDID_NAME,
    .fops   = &boardid_fops,
};

static int __devinit boardid_probe(struct platform_device *pdev)
{
    pr_info("%s\n",__func__);

    mutex_lock(&boardid_lock);
    boardid_status = BOARDID_CLOSE;
    mutex_unlock(&boardid_lock);

    return misc_register(&boardid_device);
}

static int __devexit boardid_remove(struct platform_device *pdev)
{
    pr_info("%s\n",__func__);
    return 0;
}

#ifdef CONFIG_PM_SLEEP
#define boardid_suspend NULL
static int boardid_resume(struct platform_device *dev)
{
    hi6421_check_pll();
    return 0;
}
#else
#define boardid_suspend NULL
#define boardid_resume NULL
#endif

static struct platform_driver boardid_driver = {
    .driver = {
        .name  = BOARDID_NAME,
        .owner = THIS_MODULE,
    },
    .probe  = boardid_probe,
    .remove = __devexit_p(boardid_remove),
    .suspend  = boardid_suspend,
    .resume   = boardid_resume,
};

static int __init boardid_init(void)
{
    pr_info("%s\n", __func__);
    mutex_init(&boardid_lock);
    return platform_driver_register(&boardid_driver);
}

static void __exit boardid_exit(void)
{
    pr_info("%s\n", __func__);
    platform_driver_unregister(&boardid_driver);
}

module_init(boardid_init);
module_exit(boardid_exit);

MODULE_DESCRIPTION("boardid_dev driver");
MODULE_LICENSE("GPL");
