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
#include <linux/mux.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>	   /* copy_from_user() */
#include <linux/debugfs.h>
#include <linux/platform_device.h>

#include "tpa6132.h"

#define LOG_TAG "TPA6132"

#define PRINT_INFO  0
#define PRINT_WARN  0
#define PRINT_DEBUG 0
#define PRINT_ERR   1

#if PRINT_INFO
#define logi(fmt, ...) printk("[" LOG_TAG "][I]" fmt "\n", ##__VA_ARGS__)
#else
#define logi(fmt, ...)
#endif

#if PRINT_WARN
#define logw(fmt, ...) printk("[" LOG_TAG "][W]" fmt "\n", ##__VA_ARGS__)
#else
#define logw(fmt, ...)
#endif

#if PRINT_DEBUG
#define logd(fmt, ...) printk("[" LOG_TAG "][D]" fmt "\n", ##__VA_ARGS__)
#else
#define logd(fmt, ...)
#endif

#if PRINT_ERR
#define loge(fmt, ...) printk("[" LOG_TAG "][E]" fmt "\n", ##__VA_ARGS__)
#else
#define loge(fmt, ...)
#endif

#define IOMUX_BLOCK_NAME "block_audio_eph"

static struct tpa6132_platform_data *pdata = NULL;
static struct mutex tpa6132_lock;
static struct iomux_block *tpa6132_iomux_block = NULL;
static struct block_config *tpa6132_block_config = NULL;
#ifdef CONFIG_DEBUG_FS
struct dentry *tpa6132_debugfs;
static int tpa6132_read(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
    unsigned int value = 0;
	char buf[32];
    memset(buf,0,32);
    value = gpio_get_value(pdata->gpio_tpa6132_en);
	sprintf(buf,"%d\n",value);
    return simple_read_from_buffer(user_buf, count, ppos, buf, strlen(buf));
}
static int tpa6132_write(struct file *file,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
    int ret=0;
    char buf[32];
    size_t buf_size;
    unsigned int value;
    memset(buf,0,32);
    buf_size = min(count, (sizeof(buf)-1));
    if (copy_from_user(buf, user_buf, buf_size))
	    return -EFAULT;
    kstrtoint(buf, 10, &value);
    if(value){
    	ret = blockmux_set(tpa6132_iomux_block, tpa6132_block_config, NORMAL);
        if (0 > ret) {
            loge("%s: set iomux to gpio normal error", __FUNCTION__);
            goto err_exit;
        }
        gpio_set_value(pdata->gpio_tpa6132_en, 1);
    }
    else{
        gpio_set_value(pdata->gpio_tpa6132_en, 0);
        ret = blockmux_set(tpa6132_iomux_block, tpa6132_block_config, LOWPOWER);
        if (0 > ret) {
            loge("%s: set iomux to gpio lowpower error", __FUNCTION__);
            goto err_exit;
        }
    }
err_exit:
    return buf_size;     
}
static int default_open(struct inode *inode, struct file *file)
{
	if (inode->i_private)
		file->private_data = inode->i_private;
	return 0;
}
static const struct file_operations tpa6132_list_fops = {
	.read =     tpa6132_read,
	.write =    tpa6132_write,
	.open =		default_open,
	.llseek =	default_llseek,
};
#endif

static int tpa6132_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int tpa6132_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long tpa6132_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    mutex_lock(&tpa6132_lock);
    switch (cmd) {
    case TPA6132_ENABLE:
        ret = blockmux_set(tpa6132_iomux_block, tpa6132_block_config, NORMAL);
        if (0 > ret) {
            loge("%s: set iomux to gpio normal error", __FUNCTION__);
            goto err_exit;
        }
        gpio_set_value(pdata->gpio_tpa6132_en, 1);
        break;
    case TPA6132_DISABLE:
        gpio_set_value(pdata->gpio_tpa6132_en, 0);
        ret = blockmux_set(tpa6132_iomux_block, tpa6132_block_config, LOWPOWER);
        if (0 > ret) {
            loge("%s: set iomux to gpio lowpower error", __FUNCTION__);
            goto err_exit;
        }
        break;
    default:
        loge("%s: invalid command %d", __FUNCTION__, _IOC_NR(cmd));
        ret = -EINVAL;
        break;
    }

err_exit:
    mutex_unlock(&tpa6132_lock);
    return ret;
}

static const struct file_operations tpa6132_fops = {
    .owner          = THIS_MODULE,
    .open           = tpa6132_open,
    .release        = tpa6132_release,
    .unlocked_ioctl = tpa6132_ioctl,
};

static struct miscdevice tpa6132_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = TPA6132_NAME,
    .fops   = &tpa6132_fops,
};

static int __devinit tpa6132_probe(struct platform_device *pdev)
{
    int ret = -ENODEV;

    logi("%s", __FUNCTION__);

    pdata = pdev->dev.platform_data;
    if (NULL == pdata) {
        loge("%s: platform data is NULL", __FUNCTION__);
        return -EINVAL;
    }

    tpa6132_iomux_block = iomux_get_block(IOMUX_BLOCK_NAME);
    if (!tpa6132_iomux_block) {
        loge("%s: get iomux block error", __FUNCTION__);
        return -ENODEV;
    }

    tpa6132_block_config = iomux_get_blockconfig(IOMUX_BLOCK_NAME);
    if (!tpa6132_block_config) {
        loge("%s: get block config error", __FUNCTION__);
        return -ENODEV;
    }

    /* request gpio */
    ret = gpio_request(pdata->gpio_tpa6132_en, TPA6132_NAME);
    if (0 > ret) {
        loge("%s: gpio request enable pin failed", __FUNCTION__);
        return ret;
    }

    /* set gpio output & set value low */
    ret = gpio_direction_output(pdata->gpio_tpa6132_en, 0);
    if (0 > ret) {
        loge("%s: set gpio direction failed", __FUNCTION__);
        gpio_free(pdata->gpio_tpa6132_en);
        return ret;
    }

    ret = misc_register(&tpa6132_device);
    if (ret) {
        loge("%s: tpa6132_device register failed", __FUNCTION__);
        gpio_free(pdata->gpio_tpa6132_en);
        return ret;
    }
#ifdef CONFIG_DEBUG_FS  
	if (!debugfs_create_file("tpa6132", 0644, NULL, NULL,
				 &tpa6132_list_fops))
		pr_warn("PA: Failed to create tpa6132 debugfs file\n");
#endif
    return ret;
}

static int __devexit tpa6132_remove(struct platform_device *pdev)
{
    logi("%s", __FUNCTION__);
    gpio_free(pdata->gpio_tpa6132_en);
    return 0;
}

static struct platform_driver tpa6132_driver = {
    .driver = {
        .name  = TPA6132_NAME,
        .owner = THIS_MODULE,
    },
    .probe  = tpa6132_probe,
    .remove = __devexit_p(tpa6132_remove),
};

static int __init tpa6132_init(void)
{
    logi("%s", __FUNCTION__);
    mutex_init(&tpa6132_lock);
    return platform_driver_register(&tpa6132_driver);
}

static void __exit tpa6132_exit(void)
{
    logi("%s", __FUNCTION__);
    platform_driver_unregister(&tpa6132_driver);
}

module_init(tpa6132_init);
module_exit(tpa6132_exit);

MODULE_DESCRIPTION("tpa6132 driver");
MODULE_LICENSE("GPL");
