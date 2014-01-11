#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/mux.h>
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#include "tpa2028_spk_l.h"

#define LOG_TAG "TPA2028_L"

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

#define IOMUX_BLOCK_NAME "block_audio_spk"

enum TPA2028_STATUS {
    TPA2028_NO_SUSPEND = 0,
    TPA2028_SUSPEND = 1,
};

static struct mutex tpa2028_l_lock;
static enum TPA2028_STATUS tpa2028_status;
static struct tpa2028_l_platform_data *pdata = NULL;
static struct i2c_client *this_client;

static struct iomux_block *tpa2028l_iomux_block = NULL;
static struct block_config *tpa2028l_block_config = NULL;

static int tpa2028_l_i2c_read(unsigned char *rxData, int length)
{
    int ret = 0;
    struct i2c_msg msgs[] = {
        {
            .addr  = this_client->addr,
            .flags = I2C_M_RD,
            .len   = length,
            .buf   = rxData,
        },
    };

    ret = i2c_transfer(this_client->adapter, msgs, 1);
    if (ret < 0)
        loge("%s: transfer error %d", __FUNCTION__, ret);

    return ret;
}

static int tpa2028_l_i2c_write(unsigned char *txData, int length)
{
    int ret = 0;
    struct i2c_msg msg[] = {
        {
            .addr  = this_client->addr,
            .flags = 0,
            .len   = length,
            .buf   = txData,
        },
    };

    ret = i2c_transfer(this_client->adapter, msg, 1);
    if (ret < 0)
        loge("%s: transfer error %d", __FUNCTION__, ret);

    return ret;
}

static int tpa2028_l_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int tpa2028_l_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long tpa2028_l_ioctl(struct file *file,
                            unsigned int cmd,
                            unsigned long arg)
{
    long ret = 0;
    unsigned char w_buf[] = {0, 0};
    unsigned char r_buf[] = {0};
    void __user *argp = (void __user *)arg;

    logi("%s: cmd = %#x", __FUNCTION__, _IOC_NR(cmd));

    mutex_lock(&tpa2028_l_lock);
    switch (cmd) {
    case TPA2028_ENABLE:
        if (pdata->gpio_tpa2028_en) {
            ret = blockmux_set(tpa2028l_iomux_block, tpa2028l_block_config, NORMAL);
            if (0 > ret) {
                loge("%s: set iomux to gpio normal error", __FUNCTION__);
                goto err_exit;
            }
            gpio_set_value(pdata->gpio_tpa2028_en, 1);
        }
        ret = 0;
        break;
    case TPA2028_DISABLE:
        if (pdata->gpio_tpa2028_en) {
            gpio_set_value(pdata->gpio_tpa2028_en, 0);
            ret = blockmux_set(tpa2028l_iomux_block, tpa2028l_block_config, LOWPOWER);
            if (0 > ret) {
                loge("%s: set iomux to gpio lowpower error", __FUNCTION__);
            }
        }
        ret = 0;
        break;
    case TPA2028_SET_REG:
        if (copy_from_user(w_buf, argp, sizeof(w_buf))) {
            loge("%s: copy args form user error", __FUNCTION__);
            ret = -EFAULT;
            goto err_exit;
        }

        /* check register only 1-7 can be r/w */
        if (w_buf[0] < 1 || w_buf[0] > 7) {
            loge("%s: invalid reg 0x%02x", __FUNCTION__, w_buf[0]);
            ret = -EINVAL;
            goto err_exit;
        }

        logd("%s: set reg %#x %#x", __FUNCTION__, w_buf[0], w_buf[1]);

        if (0x1 == w_buf[0]) {
            if (0x40 & w_buf[1])
                tpa2028_status = TPA2028_NO_SUSPEND;
            else
                tpa2028_status = TPA2028_SUSPEND;
        }

        ret = tpa2028_l_i2c_write(w_buf, 2);
        break;
    case TPA2028_GET_REG:
        if (copy_from_user(r_buf, argp, sizeof(r_buf))) {
            loge("%s: copy args form user error", __FUNCTION__);
            ret = -EFAULT;
            goto err_exit;
        }

        /* check register only 1-7 can be r/w */
        if (r_buf[0] < 1 || r_buf[0] > 7) {
            loge("%s: invalid reg 0x%02x", __FUNCTION__, r_buf[0]);
            ret = -EINVAL;
            goto err_exit;
        }

        logd("%s: get reg %#x", __FUNCTION__, r_buf[0]);

        ret = tpa2028_l_i2c_write(r_buf, 1);
        if (ret < 0) {
            loge("%s: i2c write error", __FUNCTION__);
            ret = -EFAULT;
            goto err_exit;
        }
        ret = tpa2028_l_i2c_read(r_buf, 1);

        logd("%s: get value %#x", __FUNCTION__, r_buf[0]);

        if (copy_to_user(argp, &r_buf, 1)) {
            loge("%s: copy args to user error", __FUNCTION__);
            ret = -EFAULT;
        }
        break;
    default:
        loge("%s: invalid command %d", __FUNCTION__, _IOC_NR(cmd));
        ret = -EINVAL;
        break;
    }

err_exit:
    mutex_unlock(&tpa2028_l_lock);
    return ret;
}

static const struct file_operations tpa2028_spk_l_fops = {
    .owner          = THIS_MODULE,
    .open           = tpa2028_l_open,
    .release        = tpa2028_l_release,
    .unlocked_ioctl = tpa2028_l_ioctl,
};

static struct miscdevice tpa2028_l_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = TPA2028_L_NAME,
    .fops   = &tpa2028_spk_l_fops,
};

static int tpa2028_l_probe(struct i2c_client *client,
                           const struct i2c_device_id *id)
{
    int ret = 0;

    logi("%s", __FUNCTION__);

    pdata = client->dev.platform_data;
    if (NULL == pdata) {
        loge("%s: platform data is NULL", __FUNCTION__);
        return -EINVAL;
    }

    this_client = client;

    if (pdata->gpio_tpa2028_en) {
        logd("%s: gpio_tpa2028_en is not null", __FUNCTION__);
        tpa2028l_iomux_block = iomux_get_block(IOMUX_BLOCK_NAME);
        if (!tpa2028l_iomux_block) {
            loge("%s: get iomux block error", __FUNCTION__);
            return -ENODEV;
        }

        tpa2028l_block_config = iomux_get_blockconfig(IOMUX_BLOCK_NAME);
        if (!tpa2028l_block_config) {
            loge("%s: get block config error", __FUNCTION__);
            return -ENODEV;
        }

        /* request gpio */
        ret = gpio_request(pdata->gpio_tpa2028_en, TPA2028_L_NAME);
        if (ret < 0) {
            loge("%s: gpio request enable pin failed", __FUNCTION__);
            return ret;
        }

        /* set gpio output & set value low */
        ret = gpio_direction_output(pdata->gpio_tpa2028_en, 0);
        if (ret < 0) {
            loge("%s: request enable gpio direction failed", __FUNCTION__);
            goto err_free_gpio;
        }
    } else {
        logd("%s: gpio_tpa2028_en is null", __FUNCTION__);
    }

    ret = misc_register(&tpa2028_l_device);
    if (ret) {
        loge("%s: tpa2028_l_device register failed", __FUNCTION__);
        goto err_free_gpio;
    }

    return 0;

err_free_gpio :
    gpio_free(pdata->gpio_tpa2028_en);
    return ret;
}

static int tpa2028_l_remove(struct i2c_client *client)
{
    struct tpa2028_l_platform_data *platform_data = i2c_get_clientdata(client);
    misc_deregister(&tpa2028_l_device);
    kfree(platform_data);
    return 0;
}

#ifdef CONFIG_PM_SLEEP
static int tpa2028_l_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret = 0;

    logi("%s : suspend", __FUNCTION__);

    mutex_lock(&tpa2028_l_lock);
    if (TPA2028_SUSPEND == tpa2028_status) {
        if (pdata->gpio_tpa2028_en) {
            logi("%s : set gpio low", __FUNCTION__);
            gpio_set_value(pdata->gpio_tpa2028_en, 0);
            ret = blockmux_set(tpa2028l_iomux_block, tpa2028l_block_config, LOWPOWER);
            if (0 > ret) {
                loge("%s: set iomux to gpio lowpower error", __FUNCTION__);
            }
        }
    }
    mutex_unlock(&tpa2028_l_lock);
    return 0;
}

static int tpa2028_l_resume(struct i2c_client *client)
{
    int ret = 0;
    unsigned char w_buf[2] = {0x1, 0x83};

    logi("%s : resume", __FUNCTION__);

    mutex_lock(&tpa2028_l_lock);
    if (TPA2028_SUSPEND == tpa2028_status) {
        if (pdata->gpio_tpa2028_en) {
            logi("%s : set gpio high", __FUNCTION__);
            ret = blockmux_set(tpa2028l_iomux_block, tpa2028l_block_config, NORMAL);
            if (0 > ret) {
                loge("%s: set iomux to gpio normal error", __FUNCTION__);
            }
            gpio_set_value(pdata->gpio_tpa2028_en, 1);
        }

        /* disable output */
        msleep(10);
        ret = tpa2028_l_i2c_write(w_buf, 2);
    }
    mutex_unlock(&tpa2028_l_lock);
    return 0;
}
#else
#define tpa2028_l_suspend NULL
#define tpa2028_l_resume NULL
#endif

static const struct i2c_device_id tpa2028_l_id[] = {
    { TPA2028_L_NAME, 0 },
};

static struct i2c_driver tpa2028_spk_l_driver = {
    .probe    = tpa2028_l_probe,
    .remove   = tpa2028_l_remove,
    .suspend  = tpa2028_l_suspend,
    .resume   = tpa2028_l_resume,
    .id_table = tpa2028_l_id,
    .driver   = {
        .name = TPA2028_L_NAME,
    },
};

static int __init tpa2028_spk_l_init(void)
{
    logi("%s", __FUNCTION__);
    mutex_init(&tpa2028_l_lock);
    return i2c_add_driver(&tpa2028_spk_l_driver);
}

static void __exit tpa2028_spk_l_exit(void)
{
    logi("%s", __FUNCTION__);
    i2c_del_driver(&tpa2028_spk_l_driver);
}

module_init(tpa2028_spk_l_init);
module_exit(tpa2028_spk_l_exit);

MODULE_DESCRIPTION("tpa2028_spk_l driver");
MODULE_LICENSE("GPL");
