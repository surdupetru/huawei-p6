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

#include "spk_5vboost.h"

#define LOG_TAG "SPK_5VBOOST"

#define PRINT_INFO  1
#define PRINT_WARN  1
#define PRINT_DEBUG 1
#define PRINT_ERR   1
#ifdef CONFIG_DEBUG_FS
struct dentry *spa_5vboost_debugfs;
#endif

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

#define IOMUX_BLOCK_NAME "block_audio_spk"

static struct mutex spk_5vboost_lock;
static struct spk_5vboost_platform_data *pdata = NULL;

static struct iomux_block *spk_5vboost_iomux_block = NULL;
static struct block_config *spk_5vboost_block_config = NULL;

static int spk_5vboost_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int spk_5vboost_release(struct inode *inode, struct file *file)
{
    return 0;
}
#ifdef CONFIG_DEBUG_FS 
static int spk_5vboost_read(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
    unsigned int value = 0;
	char buf[32];
    memset(buf,0,32);
    value = gpio_get_value(pdata->gpio_5vboost_en);
	sprintf(buf,"%d\n",value);
    return simple_read_from_buffer(user_buf, count, ppos, buf, strlen(buf));
}
static int spk_5vboost_write(struct file *file,
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
    	ret = blockmux_set(spk_5vboost_iomux_block, spk_5vboost_block_config, NORMAL);
        if (0 > ret) {
            loge("%s: set iomux to gpio normal error", __FUNCTION__);
            goto err_exit;
        }
        gpio_set_value(pdata->gpio_5vboost_en, 1);
    }
    else{
        gpio_set_value(pdata->gpio_5vboost_en, 0);
        ret = blockmux_set(spk_5vboost_iomux_block, spk_5vboost_block_config, LOWPOWER);
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
static const struct file_operations spk_5vboost_list_fops = {
	.read =     spk_5vboost_read,
	.write =    spk_5vboost_write,
	.open =		default_open,
	.llseek =	default_llseek,
};
#endif

static long spk_5vboost_ioctl(struct file *file,
                            unsigned int cmd,
                            unsigned long arg)
{
    int ret = 0;

    mutex_lock(&spk_5vboost_lock);
    switch (cmd) {
    case SPK_5VBOOST_ENABLE:
        ret = blockmux_set(spk_5vboost_iomux_block, spk_5vboost_block_config, NORMAL);
        if (0 > ret) {
            loge("%s: set iomux to gpio normal error", __FUNCTION__);
            goto err_exit;
        }
        gpio_set_value(pdata->gpio_5vboost_en, 1);
        break;
    case SPK_5VBOOST_DISABLE:
        gpio_set_value(pdata->gpio_5vboost_en, 0);
        ret = blockmux_set(spk_5vboost_iomux_block, spk_5vboost_block_config, LOWPOWER);
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
    mutex_unlock(&spk_5vboost_lock);
    return ret;
}

static const struct file_operations spk_5vboost_fops = {
    .owner          = THIS_MODULE,
    .open           = spk_5vboost_open,
    .release        = spk_5vboost_release,
    .unlocked_ioctl = spk_5vboost_ioctl,
};

static struct miscdevice spk_5vboost_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = SPK_5VBOOST_NAME,
    .fops   = &spk_5vboost_fops,
};

static int __devinit spk_5vboost_probe(struct platform_device *pdev)
{
    int ret = -ENODEV;

    logi("%s", __FUNCTION__);

    pdata = pdev->dev.platform_data;
    if (NULL == pdata) {
        loge("%s: platform data is NULL", __FUNCTION__);
        return -EINVAL;
    }

    spk_5vboost_iomux_block = iomux_get_block(IOMUX_BLOCK_NAME);
    if (!spk_5vboost_iomux_block) {
        loge("%s: get iomux block error", __FUNCTION__);
        return -ENODEV;
    }

    spk_5vboost_block_config = iomux_get_blockconfig(IOMUX_BLOCK_NAME);
    if (!spk_5vboost_block_config) {
        loge("%s: get block config error", __FUNCTION__);
        return -ENODEV;
    }

    /* request gpio */
    ret = gpio_request(pdata->gpio_5vboost_en, SPK_5VBOOST_NAME);
    if (0 > ret) {
        loge("%s: gpio request enable pin failed", __FUNCTION__);
        return ret;
    }

    /* set gpio output & set value low */
    ret = gpio_direction_output(pdata->gpio_5vboost_en, 0);
    if (0 > ret) {
        loge("%s: set gpio direction failed", __FUNCTION__);
        gpio_free(pdata->gpio_5vboost_en);
        return ret;
    }

    ret = misc_register(&spk_5vboost_device);
    if (ret) {
        loge("%s: spk_5vboost_device register failed", __FUNCTION__);
        gpio_free(pdata->gpio_5vboost_en);
        return ret;
    }
#ifdef CONFIG_DEBUG_FS  
	if (!debugfs_create_file("spa_5vboost", 0644, NULL, NULL,
				 &spk_5vboost_list_fops))
		pr_warn("PA: Failed to create spa_5vboost debugfs file\n");
#endif

    return ret;
}

static int __devexit spk_5vboost_remove(struct platform_device *pdev)
{
    logi("%s", __FUNCTION__);
    gpio_free(pdata->gpio_5vboost_en);
    return 0;
}

static struct platform_driver spk_5vboost_driver = {
    .driver = {
        .name  = SPK_5VBOOST_NAME,
        .owner = THIS_MODULE,
    },
    .probe  = spk_5vboost_probe,
    .remove = __devexit_p(spk_5vboost_remove),
};

static int __init spk_5vboost_init(void)
{
    logi("%s", __FUNCTION__);
    mutex_init(&spk_5vboost_lock);
    return platform_driver_register(&spk_5vboost_driver);
}

static void __exit spk_5vboost_exit(void)
{
    logi("%s", __FUNCTION__);
    platform_driver_unregister(&spk_5vboost_driver);
}

module_init(spk_5vboost_init);
module_exit(spk_5vboost_exit);

MODULE_DESCRIPTION("spk_5vboost driver");
MODULE_LICENSE("GPL");
