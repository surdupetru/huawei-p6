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
#include <linux/debugfs.h>
#include <linux/uaccess.h>	   /* copy_from_user() */
#include <linux/platform_device.h>
#include <hsad/config_interface.h>

#include "modemctl.h"

#define LOG_TAG "modemctl"

#define PRINT_INFO  0
#define PRINT_WARN  1
#define PRINT_DEBUG 1
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

#ifdef CONFIG_DEBUG_FS
struct dentry *modem_control_debugfs;
#endif

#define IOMUX_BLOCK_NAME "block_modem_switch"

static struct modemctl_platform_data *pdata = NULL;
static struct mutex modemctl_lock;
static struct iomux_block *modemctl_iomux_block = NULL;
static struct block_config *modemctl_block_config = NULL;

#ifdef CONFIG_DEBUG_FS 
static ssize_t modem_control_read(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
    unsigned int value = 0;
	char buf[32];
    memset(buf,0,32);
    value = gpio_get_value(pdata->gpio_modemctl_en);
	sprintf(buf,"%d\n",value);
    return simple_read_from_buffer(user_buf, count, ppos, buf, strlen(buf));
}

static ssize_t modem_control_write(struct file *file,
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
    	ret = blockmux_set(modemctl_iomux_block, modemctl_block_config, NORMAL);
        if (0 > ret) {
            loge("%s: set iomux to gpio normal error", __FUNCTION__);
            goto err_exit;
        }
        gpio_set_value(pdata->gpio_modemctl_en, 1);
    }
    else{
        gpio_set_value(pdata->gpio_modemctl_en, 0);
        ret = blockmux_set(modemctl_iomux_block, modemctl_block_config, LOWPOWER);
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

static const struct file_operations modem_control_list_fops = {
       .read = modem_control_read,
       .write = modem_control_write,
	   .open =		default_open,
	   .llseek =	default_llseek,
};
#endif


static int modemctl_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int modemctl_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long modemctl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    mutex_lock(&modemctl_lock);
    switch (cmd) {
    case MODEMCTL_SWITCH_TO_FIRST:
        ret = blockmux_set(modemctl_iomux_block, modemctl_block_config, NORMAL);
        if (0 > ret) {
            loge("%s: set iomux to gpio normal error", __FUNCTION__);
            goto err_exit;
        }
        gpio_set_value(pdata->gpio_modemctl_en, 0);
        break;
    case MODEMCTL_SWITCH_TO_SECOND:
        gpio_set_value(pdata->gpio_modemctl_en, 1);
        ret = blockmux_set(modemctl_iomux_block, modemctl_block_config, LOWPOWER);
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
    mutex_unlock(&modemctl_lock);
    return ret;
}

static const struct file_operations modemctl_fops = {
    .owner          = THIS_MODULE,
    .open           = modemctl_open,
    .release        = modemctl_release,
    .unlocked_ioctl = modemctl_ioctl,
};

static struct miscdevice modemctl_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = MODEMCTL_NAME,
    .fops   = &modemctl_fops,
};


static int __devinit modemctl_probe(struct platform_device *pdev)
{
    int ret = -ENODEV;

    logi("%s", __FUNCTION__);

    pdata = pdev->dev.platform_data;
    if (NULL == pdata) {
        loge("%s: platform data is NULL", __FUNCTION__);
        return -EINVAL;
    }

    modemctl_iomux_block = iomux_get_block(IOMUX_BLOCK_NAME);
    if (!modemctl_iomux_block) {
        loge("%s: get iomux block error", __FUNCTION__);
        return -ENODEV;
    }

    modemctl_block_config = iomux_get_blockconfig(IOMUX_BLOCK_NAME);
    if (!modemctl_block_config) {
        loge("%s: get block config error", __FUNCTION__);
        return -ENODEV;
    }

    /* request gpio */
    ret = gpio_request(pdata->gpio_modemctl_en, MODEMCTL_NAME);
    if (0 > ret) {
        loge("%s: gpio request enable pin failed", __FUNCTION__);
        return ret;
    }

    /* set gpio output & set value low */
    ret = gpio_direction_output(pdata->gpio_modemctl_en, 0);
    if (0 > ret) {
        loge("%s: set gpio direction failed", __FUNCTION__);
        gpio_free(pdata->gpio_modemctl_en);
        return ret;
    }

    ret = misc_register(&modemctl_device);
    if (ret) {
        loge("%s: modemctl_device register failed", __FUNCTION__);
        gpio_free(pdata->gpio_modemctl_en);
        return ret;
    }

#ifdef CONFIG_DEBUG_FS  
    if (!debugfs_create_file("modem_control", 0644, NULL, NULL,
                            &modem_control_list_fops))
        pr_warn("Modem_switch: Failed to create modem_control debugfs file\n");
#endif

    return ret;
}

static int __devexit modemctl_remove(struct platform_device *pdev)
{
    logi("%s", __FUNCTION__);
    gpio_free(pdata->gpio_modemctl_en);
    return 0;
}

static struct platform_driver modemctl_driver = {
    .driver = {
        .name  = MODEMCTL_NAME,
        .owner = THIS_MODULE,
    },
    .probe  = modemctl_probe,
    .remove = __devexit_p(modemctl_remove),
};

static int __init modemctl_init(void)
{
    logi("%s", __FUNCTION__);

    if (!is_modem_switch_support()) {
        logi("%s don't support modem switch.", __FUNCTION__);
        return -1;
    }

    mutex_init(&modemctl_lock);
    return platform_driver_register(&modemctl_driver);
}

static void __exit modemctl_exit(void)
{
    logi("%s", __FUNCTION__);
    platform_driver_unregister(&modemctl_driver);
}

module_init(modemctl_init);
module_exit(modemctl_exit);

MODULE_DESCRIPTION("modemctl driver");
MODULE_LICENSE("GPL");
