#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/mux.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>

#include "tfa9887.h"

#define LOG_TAG "TFA9887"

#define PRINT_INFO  0
#define PRINT_WARN  1
#define PRINT_DEBUG 1
#define PRINT_ERR   1

#if PRINT_INFO
#define logi(fmt, ...) printk("[" LOG_TAG "][I]" fmt "\n", ##__VA_ARGS__)
#else
#define logi(fmt, ...)
#endif

#if PRINT_DEBUG
#define logd(fmt, ...) printk("[" LOG_TAG "][D]" fmt "\n", ##__VA_ARGS__)
#else
#define logd(fmt, ...)
#endif

#if PRINT_WARN
#define logw(fmt, ...) printk("[" LOG_TAG "][W]" fmt "\n", ##__VA_ARGS__)
#else
#define logw(fmt, ...)
#endif

#if PRINT_ERR
#define loge(fmt, ...) printk("[" LOG_TAG "][E]" fmt "\n", ##__VA_ARGS__)
#else
#define loge(fmt, ...)
#endif


static struct i2c_client *this_client = NULL;
static struct mutex tfa9887_lock;

static int tfa9887_i2c_read(char *rxData, int length)
{
    int ret = 0;
	int i = 0;
    struct i2c_msg msg[] = {
        {
            .addr  = this_client->addr,
            .flags = I2C_M_RD,
            .len   = length,
            .buf   = rxData,
        },
    };

    ret = i2c_transfer(this_client->adapter, msg, 1);
    if (0 > ret) {
        loge("%s: transfer error %d", __FUNCTION__, ret);
        return ret;
    }

    for (i = 0; i < length; i++)
        logi("%s: rx[%d] = %2x", __FUNCTION__, i, rxData[i]);

    return ret;
}

static int tfa9887_i2c_write(char *txData, int length)
{
    int ret = 0;
	int i = 0;
    struct i2c_msg msg[] = {
        {
            .addr = this_client->addr,
            .flags = 0,
            .len = length,
            .buf = txData,
        },
    };

    ret = i2c_transfer(this_client->adapter, msg, 1);
    if (0 > ret) {
        loge("%s: transfer error %d", __FUNCTION__, ret);
        return ret;
    }

    for (i = 0; i < length; i++)
        logi("%s: tx[%d] = %2x", __FUNCTION__, i, txData[i]);

    return ret;
}

static int tfa9887_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int tfa9887_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t tfa9887_read(struct file *file, char __user *buf,
         size_t nbytes, loff_t *pos)
{
    int retry = POLLING_RETRY_TIMES;
	int ret = 0;
	void *kern_buf;
	kern_buf = kmalloc(nbytes, GFP_KERNEL);
	if (!kern_buf) {
         loge(KERN_INFO "Failed to allocate buffer\n");
         return -ENOMEM;
    }
	
    ret = tfa9887_i2c_read((char *)kern_buf, nbytes);
    if (0 > ret) {
        loge("%s: i2c read error %d", __FUNCTION__, ret);
	    kfree(kern_buf);
        return -1;
    }
	
	if (copy_to_user((void  __user *)buf, kern_buf,  nbytes)) {
         kfree(kern_buf);
         return -EFAULT;
    }
	kfree(kern_buf);
    return ret;
}

static ssize_t tfa9887_write(struct file *file,
		const char __user *buf, size_t nbytes, loff_t *ppos)
{
    int ret=0;
	int retry = POLLING_RETRY_TIMES;
    void *kern_buf;
	
	kern_buf = kmalloc(nbytes, GFP_KERNEL);
	if (!kern_buf) {
         loge(KERN_INFO "Failed to allocate buffer\n");
         return -ENOMEM;
    }

    if (copy_from_user(kern_buf, (void  __user *)buf, nbytes)) {
         kfree(kern_buf);
         return -EFAULT;
    }
	
    ret = tfa9887_i2c_write((char *)kern_buf, nbytes);
    if (0 > ret) {
        loge("%s: i2c write error %d", __FUNCTION__, ret);
		ret = -1;
    }

	kfree(kern_buf);
    return ret;     
}


static long tfa9887_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    //void __user *argp = (void __user *)arg;
	int ret = 0;
    mutex_lock(&tfa9887_lock);
    switch (cmd) {
    case I2C_SLAVE:
        break;
    default:
        loge("%s: invalid command %d", __FUNCTION__, _IOC_NR(cmd));
        ret = -EINVAL;
        break;
    }

    mutex_unlock(&tfa9887_lock);
    return ret;
}
	
static const struct file_operations tfa9887_fops = {
    .owner          = THIS_MODULE,
    .open           = tfa9887_open,
    .release        = tfa9887_release,
	.read           = tfa9887_read,
	.write          = tfa9887_write,
    .unlocked_ioctl = tfa9887_ioctl,
};

static struct miscdevice tfa9887_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = TFA9887_NAME,
    .fops   = &tfa9887_fops,
};

static int tfa9887_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int ret = 0;

    this_client = client;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        loge("%s: i2c check functionality error", __FUNCTION__);
        ret = -ENODEV;
    }

    ret = misc_register(&tfa9887_device);
    if (ret)
        loge("%s: tfa9887_device register failed (%d)", __FUNCTION__, ret);
/*
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    // detect current device successful, set the flag as present 
    set_hw_dev_flag(DEV_I2C_AUDIENCE);
#endif
*/
    return ret;
}

static int tfa9887_remove(struct i2c_client *client)
{
    misc_deregister(&tfa9887_device);
    return 0;
}

static const struct i2c_device_id tfa9887_id[] = {
    { TFA9887_NAME, 0 },
    { }
};

static struct i2c_driver tfa9887_driver = {
    .probe      = tfa9887_probe,
    .remove     = tfa9887_remove,
    .id_table   = tfa9887_id,
    .driver     = {
        .name = TFA9887_NAME,
    },
};

static int __init tfa9887_init(void)
{
    logi("%s", __FUNCTION__);
    mutex_init(&tfa9887_lock);
    return i2c_add_driver(&tfa9887_driver);
}

static void __exit tfa9887_exit(void)
{
    i2c_del_driver(&tfa9887_driver);
}

module_init(tfa9887_init);
module_exit(tfa9887_exit);

MODULE_DESCRIPTION("TFA9887 I2C driver");
MODULE_LICENSE("GPL");
