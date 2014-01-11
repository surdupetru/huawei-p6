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
#include <linux/workqueue.h>
#include <linux/freezer.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

#include "es305.h"
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif
#define LOG_TAG "ES305"

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

#define WAKEUP_IOMUX_BLOCK_NAME "block_audio_es305"

#define ES305_EACH_MSG_SIZE ( 32 )

/* msg format : 0x12345678 */
#define ES305_GET_BYTE0(msg) ((msg >> 24) & 0xFF)
#define ES305_GET_BYTE1(msg) ((msg >> 16) & 0xFF)
#define ES305_GET_BYTE2(msg) ((msg >> 8) & 0xFF)
#define ES305_GET_BYTE3(msg) (msg & 0xFF)

#define SLEEP_RANGE(x)       (usleep_range((x), (x + 500)))
#define SIZE_OF_ARRAY(x)     (sizeof(x) / sizeof(unsigned int))

/* status bit mask */
#define AGC_BIT ( 1 << 0 )
#define AEC_BIT ( 1 << 1 )
#define ANR_BIT ( 1 << 2 )

enum ES305_MODE {
    ES305_SLEEP,
    ES305_NORMAL,
};

enum ES305_POWER {
    ES305_POWER_OFF,
    ES305_POWER_ON,
};

struct audience_img {
    unsigned char *data;
    unsigned int img_size;
};

struct audience_img img;

static enum ES305_MODE   es305_mode = ES305_NORMAL;
static enum ES305_PATHID es305_current_pathid = ES305_PATH_NO_INIT;
static enum ES305_POWER  es305_power = ES305_POWER_OFF;
static unsigned char es305_status = AGC_BIT | AEC_BIT | ANR_BIT;

static struct i2c_client *this_client = NULL;
static struct es305_platform_data *pdata = NULL;
static struct mutex es305_lock;

static struct iomux_block  *es305_iomux_block = NULL;
static struct block_config *es305_block_config = NULL;

/* CLOCK 26M LDO_1 */
static struct regulator *g_regu_ldo1 = NULL;

/* ES305 VCC LDO_5 */
static struct regulator *g_regu_ldo5 = NULL;

extern void hi6421_lock_sysclk(bool);

static unsigned int path_playback[] = {
    0x80520074, /* BYPASS MODE PORT D to PORT B */
    0x80260092, /* ROUTE 146 : PORT A(64bits) <--> PORT C(32bits) */
};

static unsigned int path_first_nb_call_receiver[] = {
    0x80310000, /* CONFIG HANDSET PRESET */
};

static unsigned int path_first_nb_call_headset[] = {
    0x80310001, /* CONFIG HEADSET PRESET */
};

static unsigned int path_first_nb_call_headphone[] = {
    0x80310002, /* CONFIG HEADPHONE PRESET */
};

static unsigned int path_first_nb_call_speaker[] = {
    0x80310003, /* CONFIG HANDFREE PRESET */
};

static unsigned int path_first_nb_call_bt[] = {
    0x80310004, /* CONFIG BT PRESET */
};

/* cs chip call mode wide band  first modem */
static unsigned int path_first_wb_call_receiver[] = {
    0x80310006, /* CONFIG HANDSET PRESET */
};

static unsigned int path_first_wb_call_headset[] = {
    0x80310007, /* CONFIG HEADSET PRESET */
};

static unsigned int path_first_wb_call_headphone[] = {
    0x80310008, /* CONFIG HEADPHONE PRESET */
};

static unsigned int path_first_wb_call_speaker[] = {
    0x80310009, /* CONFIG HANDFREE PRESET */
};

static unsigned int path_first_wb_call_bt[] = {
    0x8031000A, /* CONFIG BT PRESET */
};

/* ASR */
static unsigned int path_asr[] = {
    0x80310012, 
};

/* cs chip call mode narrow band for second modem */
static unsigned int path_second_nb_call_receiver[] = {
    0x80310018, /* CONFIG HANDSET PRESET */
};

static unsigned int path_second_nb_call_headset[] = {
    0x80310019, /* CONFIG HEADSET PRESET */
};

static unsigned int path_second_nb_call_headphone[] = {
    0x8031001A, /* CONFIG HEADPHONE PRESET */
};

static unsigned int path_second_nb_call_speaker[] = {
    0x8031001B, /* CONFIG HANDFREE PRESET */
};

static unsigned int path_second_nb_call_bt[] = {
    0x8031001C, /* CONFIG BT PRESET */
};

/* cs chip call mode wide band  second modem */
static unsigned int path_second_wb_call_receiver[] = {
    0x8031001E, /* CONFIG HANDSET PRESET */
};

static unsigned int path_second_wb_call_headset[] = {
    0x8031001F, /* CONFIG HEADSET PRESET */
};

static unsigned int path_second_wb_call_headphone[] = {
    0x80310020, /* CONFIG HEADPHONE PRESET */
};

static unsigned int path_second_wb_call_speaker[] = {
    0x80310021, /* CONFIG HANDFREE PRESET */
};

static unsigned int path_second_wb_call_bt[] = {
    0x80310022, /* CONFIG BT PRESET */
};

/* voip mode, support wide band only */
static unsigned int path_voip_receiver[] = {
    0x80520000, /* CANCEL BYPASS */
    0x8026008C, /* ROUTE 0x8C */
    0x8031000C,
};

static unsigned int path_voip_headset[] = {
    0x80520000, /* CANCEL BYPASS */
    0x8026008C, /* ROUTE 0x8C */
    0x8031000D,
};

static unsigned int path_voip_headphone[] = {
    0x80520000, /* CANCEL BYPASS */
    0x8026008C, /* ROUTE 0x8C */
    0x8031000E,
};

static unsigned int path_voip_speaker[] = {
    0x80520000, /* CANCEL BYPASS */
    0x8026008C, /* ROUTE 0x8C */
    0x8031000F,
};

static unsigned int path_voip_bt[] = {
    0x80520000, /* CANCEL BYPASS */
    0x8026008C, /* ROUTE 0x8C */
    0x80310010,
};

/* 3A CONFIG */
static unsigned int disable_agc[] = {
    0x80170004,
    0x80180000,
    0x80170028,
    0x80180000,
};

static unsigned int disable_aec[] = {
    0x80170003,
    0x80180000,
};

static unsigned int disable_anr[] = {
    0x8017000e,
    0x80180000,
    0x8017004b,
    0x80180000,
};

static int audience_i2c_read(char *rxData, int length)
{
    int ret = 0;
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

    {
        int i = 0;
        for (i = 0; i < length; i++)
            logi("%s: rx[%d] = %2x", __FUNCTION__, i, rxData[i]);
    }

    return ret;
}

static int audience_i2c_write(char *txData, int length)
{
    int ret = 0;
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

    {
        int i = 0;
        for (i = 0; i < length; i++)
            logi("%s: tx[%d] = %2x", __FUNCTION__, i, txData[i]);
    }

    return ret;
}

static void es305_poweron(void)
{
    int ret = 0;

    if (ES305_POWER_OFF == es305_power) {
        es305_power = ES305_POWER_ON;
        hi6421_lock_sysclk(true);

        ret = regulator_enable(g_regu_ldo1);
        if (0 > ret)
            pr_err("%s: regulator ldo1 enable failed\n", __FUNCTION__);

        regulator_set_optimum_mode(g_regu_ldo5, 10000);
        ret = regulator_enable(g_regu_ldo5);
        if (0 > ret)
            pr_err("%s: regulator ldo5 enable failed\n", __FUNCTION__);
    }
}

static void es305_poweroff(void)
{
    int ret = 0;

    if (ES305_POWER_ON == es305_power) {
        es305_power = ES305_POWER_OFF;
        ret = regulator_disable(g_regu_ldo5);
        if (0 > ret)
            pr_err("%s: regulator ldo5 disable failed\n", __FUNCTION__);

        ret = regulator_disable(g_regu_ldo1);
        if (0 > ret)
            pr_err("%s: regulator ldo1 disable failed\n", __FUNCTION__);

        hi6421_lock_sysclk(false);
    }
}

/* reset es305 chip */
static void es305_hw_reset(void)
{
    logi("%s", __FUNCTION__);

    gpio_set_value(pdata->gpio_es305_reset, 0);

    /* keep 4 cycles in clk (26M) */
    mdelay(AUDIENCE_RESET_STABLE_TIME);

    gpio_set_value(pdata->gpio_es305_reset, 1);
    es305_mode = ES305_NORMAL;
    es305_current_pathid = ES305_PATH_NO_INIT;
}

static int es305_sw_reset(void)
{
    int ret = 0;
    unsigned char msgbuf[4];

    logi("%s", __FUNCTION__);

    msgbuf[0] = ES305_GET_BYTE0(AUDIENCE_MSG_RESET_IMMEDIATE);
    msgbuf[1] = ES305_GET_BYTE1(AUDIENCE_MSG_RESET_IMMEDIATE);
    msgbuf[2] = ES305_GET_BYTE2(AUDIENCE_MSG_RESET_IMMEDIATE);
    msgbuf[3] = ES305_GET_BYTE3(AUDIENCE_MSG_RESET_IMMEDIATE);

    ret = audience_i2c_write(msgbuf, 4);

    if (!ret) {
        msleep(AUDIENCE_SW_RESET_TIME);
        return -AUDIENCE_SW_RESET_OK;
    } else {
        loge("%s: software reset error %d", __FUNCTION__, ret);
    }

    return -AUDIENCE_SW_RESET_ERROR;
}

static int es305_sync(void)
{
    int ret = 0;
    int retry = POLLING_RETRY_TIMES;
    unsigned char w_buf[4];
    unsigned char r_buf[4] = {0,0,0,0};

    logi("%s", __FUNCTION__);

    w_buf[0] = ES305_GET_BYTE0(AUDIENCE_MSG_SYNC);
    w_buf[1] = ES305_GET_BYTE1(AUDIENCE_MSG_SYNC);
    w_buf[2] = ES305_GET_BYTE2(AUDIENCE_MSG_SYNC);
    w_buf[3] = ES305_GET_BYTE3(AUDIENCE_MSG_SYNC);

    while (retry--) {
        ret = audience_i2c_write(w_buf, 4);
        if (0 > ret) {
            loge("%s : write sync msg error", __FUNCTION__);
            continue;
        }

        SLEEP_RANGE(AUDIENCE_SYNC_TIME);

        ret = audience_i2c_read(r_buf, 4);
        if (0 > ret) {
            loge("%s : read sync ack error", __FUNCTION__);
            continue;
        }

        if ((w_buf[0] == r_buf[0]) &&
            (w_buf[1] == r_buf[1]) &&
            (w_buf[2] == r_buf[2]) &&
            (w_buf[3] == r_buf[3])) {
            return 0;
        }
    }

    return -1;
}

static int es305_check_cmd_ack(unsigned int msg)
{
    int ret = 0;
    unsigned char msgbuf[4] = {0,0,0,0};

    logi("%s", __FUNCTION__);

    ret = audience_i2c_read(msgbuf, 4);
    if (0 > ret) {
        loge("%s: i2c read error %d", __FUNCTION__, ret);
        return -EINVAL;
    }

    logd("%s: read ack : 0x%02x%02x%02x%02x", __FUNCTION__,
            msgbuf[0], msgbuf[1], msgbuf[2], msgbuf[3]);

    if ((ES305_GET_BYTE0(msg) == msgbuf[0]) &&
        (ES305_GET_BYTE1(msg) == msgbuf[1])) {
        return 0;
    } else if (AUDIENCE_ACK_ERROR == msgbuf[0] &&
               AUDIENCE_ACK_ERROR == msgbuf[1]) {
        loge("%s: cmd error", __FUNCTION__);
        return -EINVAL;
    } else if (AUDIENCE_ACK_NONE == msgbuf[0] &&
               AUDIENCE_ACK_NONE == msgbuf[1]) {
        loge("%s: not ready", __FUNCTION__);
        return -EBUSY;
    } else {
        loge("%s: cmd/ack mismatch", __FUNCTION__);
        return -EBUSY;
    }
}

static int es305_send_msg(unsigned int msg)
{
    int ret = 0;
    int retry = POLLING_RETRY_TIMES;
    unsigned char msgbuf[4];

    logd("%s: send msg : %#x", __FUNCTION__, msg);

    if (AUDIENCE_MSG_RESET_IMMEDIATE == msg) {
        logi("%s: send msg : reset immediate", __FUNCTION__);
        return es305_sw_reset();
    }

    if (AUDIENCE_MSG_SYNC == msg) {
        logi("%s: send msg : sync", __FUNCTION__);
        return es305_sync();
    }

    msgbuf[0] = ES305_GET_BYTE0(msg);
    msgbuf[1] = ES305_GET_BYTE1(msg);
    msgbuf[2] = ES305_GET_BYTE2(msg);
    msgbuf[3] = ES305_GET_BYTE3(msg);

    if(AUDIENCE_MSG_SLEEP_MODE == msg) {
        es305_mode = ES305_SLEEP;
        ret = audience_i2c_write(msgbuf, 4);
        if (0 > ret) {
            loge("%s: i2c write error %d", __FUNCTION__, ret);
            ret = es305_sync();
            /* sync error means es305 sleep; otherwise retry*/
            if (0 > ret) {
                msleep(AUDIENCE_SLEEP_TIME);
                return 0;
            } else {
                return audience_i2c_write(msgbuf, 4);
            }
        }
        msleep(AUDIENCE_SLEEP_TIME);
        return 0;
    }

    while (retry--) {
        ret = audience_i2c_write(msgbuf, 4);
        if (0 > ret) {
            loge("%s: i2c write error %d", __FUNCTION__, ret);
            continue;
        }

        /* polling mode */
        SLEEP_RANGE(AUDIENCE_POLLING_TIME);

        ret = es305_check_cmd_ack(msg);

        if (!ret)
            break;
        else
            loge("%s: send msg %#x error, %d times left",
                    __FUNCTION__, msg, retry);
    }

    if (0 > retry && 0 > ret) {
        loge("%s: send msg %#x error", __FUNCTION__, msg);
        return -1;
    }

    return ret;
}

static int es305_check_wakeup(void)
{
    int ret = 0;
    int temp = 0;
    int retry = POLLING_RETRY_TIMES;

    logi("%s", __FUNCTION__);

    if (ES305_SLEEP == es305_mode) {
        ret = blockmux_set(es305_iomux_block, es305_block_config, NORMAL);
        if (0 > ret) {
            loge("%s: set iomux to gpio normal error", __FUNCTION__);
            return ret;
        }

        ret = gpio_direction_output(pdata->gpio_es305_wakeup, 0);
        if (0 > ret) {
            loge("%s: request wakeup gpio direction failed", __FUNCTION__);
            goto err_free_gpio;
        }

        do {
            gpio_set_value(pdata->gpio_es305_wakeup, 1);
            SLEEP_RANGE(AUDIENCE_WAKEUP_H_TIME);
            gpio_set_value(pdata->gpio_es305_wakeup, 0);
            SLEEP_RANGE(AUDIENCE_WAKEUP_L_TIME);
            ret = es305_sync();
        } while ((0 > ret) && --retry);

        if (0 > ret) {
            loge("%s: sync failed", __FUNCTION__);
            goto err_sync;
        }
        es305_mode = ES305_NORMAL;

err_sync:
        ret = gpio_direction_input(pdata->gpio_es305_wakeup);
        if (0 > ret)
            loge("%s: set gpio input mode error", __FUNCTION__);

err_free_gpio:
        temp = blockmux_set(es305_iomux_block, es305_block_config, LOWPOWER);
        if (0 > temp)
            loge("%s: set iomux to gpio normal error", __FUNCTION__);
    }

    return ret;
}

static ssize_t es305_i2c_download(struct file *file, struct es305_img *img)
{
    struct audience_img *p_img = file->private_data;
    int ret = 0;
    int remaining = 0;
    int retry = RETRY_TIMES;
    unsigned char *curdata = NULL;
    char buf[2];

    logi("%s", __FUNCTION__);

    if (img->img_size >= ES305_MAX_FW_SIZE) {
        loge("%s: invalid es305 image size %d", __FUNCTION__, img->img_size);
        return -EINVAL;
    }

    p_img->data = kmalloc(img->img_size, GFP_KERNEL);/* fix me */
    if (!p_img->data) {
        loge("%s: out of memory", __FUNCTION__);
        return -ENOMEM;
    }

    p_img->img_size = img->img_size;
    if (copy_from_user(p_img->data, img->buf, img->img_size)) {
        loge("%s: copy from user failed", __FUNCTION__);
        kfree(p_img->data);
        return -EFAULT;
    }

    while (retry--) {
        /* hardware reset */
        es305_hw_reset();

        /* delay before send I2C command */
        msleep(AUDIENCE_I2C_ACCESS_DELAY_TIME);

        /* send boot command to es305 */
        buf[0] = AUDIENCE_MSG_BOOT_BYTE1;
        buf[1] = AUDIENCE_MSG_BOOT_BYTE2;

        ret = audience_i2c_write(buf, 2);
        if (0 > ret) {
            loge("%s: send boot command(%d retries left)", __FUNCTION__, retry);
            continue;
        }

        /* wait for i2c ack */
        mdelay(AUDIENCE_BOOT_ACK_TIME);
        ret = audience_i2c_read(buf, 1);
        if (0 > ret) {
            loge("%s: i2c read error(%d retries left)", __FUNCTION__, retry);
            continue;
        }

        if (AUDIENCE_MSG_BOOT_ACK != buf[0]) {
            loge("%s: not a boot ack(%d retries left)", __FUNCTION__, retry);
            continue;
        }

        remaining = p_img->img_size / ES305_EACH_MSG_SIZE;
        curdata = p_img->data;

        logi("%s: starting to load image (%d passes)...", __FUNCTION__,
                remaining + !!(p_img->img_size % ES305_EACH_MSG_SIZE));

        /* send image */
        for (; remaining; remaining--, curdata += ES305_EACH_MSG_SIZE) {
            ret = audience_i2c_write(curdata, ES305_EACH_MSG_SIZE);
            if (0 > ret)
                break;
        }

        if (ret >= 0 && (p_img->img_size % ES305_EACH_MSG_SIZE))
            ret = audience_i2c_write(curdata, p_img->img_size % ES305_EACH_MSG_SIZE);

        if (0 > ret) {
            loge("%s: firmware load error : %d(%d retries left)",
                    __FUNCTION__, ret, retry);
            continue;
        }

        /* Delay time before issue a Sync Cmd */
        msleep(AUDIENCE_BOOT_SYNC_TIME);

        logi("%s: firmware loaded successfully", __FUNCTION__);

        ret = es305_sync();
        if (0 > ret) {
            loge("%s: sync command error %d(%d retries left)",
                __FUNCTION__, ret, retry);
            continue;
        } else {
            logi("%s: sync successfully", __FUNCTION__);
            break;
        }
    }

    if (0 > retry) {
        loge("%s: initialized failed", __FUNCTION__);
        ret = -EINVAL;
    }

    kfree(p_img->data);
    return ret;
}

static int es305_send_msg_by_array(unsigned int msgs[], int size)
{
    int ret = 0;
    int i = 0;

    logi("%s : size = %d", __FUNCTION__, size);

    while (i < size) {
        ret = es305_send_msg(msgs[i]);
        if(0 > ret) {
            loge("set msg by array failed");
            return ret;
        }
        i++;
    }

    return 0;
}

static int es305_set_pathid(enum ES305_PATHID pathid)
{
    int ret = 0;

    logi("%s : pathid = %d", __FUNCTION__, pathid);

    /* check chip is waken up */
    ret = es305_check_wakeup();
    if (0 > ret) {
        loge("%s: can not wake up es305", __FUNCTION__);
        return ret;
    }

    switch(pathid) {
    case ES305_PATH_BYPASS:
        ret = es305_send_msg_by_array(path_playback,
                                      SIZE_OF_ARRAY(path_playback));
        break;
/* call narrow band */
    case ES305_PATH_FIRST_NB_CALL_RECEIVER:
        ret = es305_send_msg_by_array(path_first_nb_call_receiver,
                                      SIZE_OF_ARRAY(path_first_nb_call_receiver));
        break;
    case ES305_PATH_FIRST_NB_CALL_HEADSET:
        ret = es305_send_msg_by_array(path_first_nb_call_headset,
                                      SIZE_OF_ARRAY(path_first_nb_call_headset));
        break;
    case ES305_PATH_FIRST_NB_CALL_HEADPHONE:
        ret = es305_send_msg_by_array(path_first_nb_call_headphone,
                                      SIZE_OF_ARRAY(path_first_nb_call_headphone));
        break;
    case ES305_PATH_FIRST_NB_CALL_SPEAKER:
        ret = es305_send_msg_by_array(path_first_nb_call_speaker,
                                      SIZE_OF_ARRAY(path_first_nb_call_speaker));
        break;
    case ES305_PATH_FIRST_NB_CALL_BT:
        ret = es305_send_msg_by_array(path_first_nb_call_bt,
                                      SIZE_OF_ARRAY(path_first_nb_call_bt));
        break;
/* call wide band */
    case ES305_PATH_FIRST_WB_CALL_RECEIVER:
        ret = es305_send_msg_by_array(path_first_wb_call_receiver,
                                      SIZE_OF_ARRAY(path_first_wb_call_receiver));
        break;
    case ES305_PATH_FIRST_WB_CALL_HEADSET:
        ret = es305_send_msg_by_array(path_first_wb_call_headset,
                                      SIZE_OF_ARRAY(path_first_wb_call_headset));
        break;
    case ES305_PATH_FIRST_WB_CALL_HEADPHONE:
        ret = es305_send_msg_by_array(path_first_wb_call_headphone,
                                      SIZE_OF_ARRAY(path_first_wb_call_headphone));
        break;
    case ES305_PATH_FIRST_WB_CALL_SPEAKER:
        ret = es305_send_msg_by_array(path_first_wb_call_speaker,
                                      SIZE_OF_ARRAY(path_first_wb_call_speaker));
        break;
    case ES305_PATH_FIRST_WB_CALL_BT:
        ret = es305_send_msg_by_array(path_first_wb_call_bt,
                                      SIZE_OF_ARRAY(path_first_wb_call_bt));
        break;
/* narrow band call for second modem*/
    case ES305_PATH_SECOND_NB_CALL_RECEIVER:
        ret = es305_send_msg_by_array(path_second_nb_call_receiver,
                                      SIZE_OF_ARRAY(path_second_nb_call_receiver));
        break;
    case ES305_PATH_SECOND_NB_CALL_HEADSET:
        ret = es305_send_msg_by_array(path_second_nb_call_headset,
                                      SIZE_OF_ARRAY(path_second_nb_call_headset));
        break;
    case ES305_PATH_SECOND_NB_CALL_HEADPHONE:
        ret = es305_send_msg_by_array(path_second_nb_call_headphone,
                                      SIZE_OF_ARRAY(path_second_nb_call_headphone));
        break;
    case ES305_PATH_SECOND_NB_CALL_SPEAKER:
        ret = es305_send_msg_by_array(path_second_nb_call_speaker,
                                      SIZE_OF_ARRAY(path_second_nb_call_speaker));
        break;
    case ES305_PATH_SECOND_NB_CALL_BT:
        ret = es305_send_msg_by_array(path_second_nb_call_bt,
                                      SIZE_OF_ARRAY(path_second_nb_call_bt));
        break;
/* wide band call for second modem*/
    case ES305_PATH_SECOND_WB_CALL_RECEIVER:
        ret = es305_send_msg_by_array(path_second_wb_call_receiver,
                                      SIZE_OF_ARRAY(path_second_wb_call_receiver));
        break;
    case ES305_PATH_SECOND_WB_CALL_HEADSET:
        ret = es305_send_msg_by_array(path_second_wb_call_headset,
                                      SIZE_OF_ARRAY(path_second_wb_call_headset));
        break;
    case ES305_PATH_SECOND_WB_CALL_HEADPHONE:
        ret = es305_send_msg_by_array(path_second_wb_call_headphone,
                                      SIZE_OF_ARRAY(path_second_wb_call_headphone));
        break;
    case ES305_PATH_SECOND_WB_CALL_SPEAKER:
        ret = es305_send_msg_by_array(path_second_wb_call_speaker,
                                      SIZE_OF_ARRAY(path_second_wb_call_speaker));
        break;
    case ES305_PATH_SECOND_WB_CALL_BT:
        ret = es305_send_msg_by_array(path_second_wb_call_bt,
                                      SIZE_OF_ARRAY(path_second_wb_call_bt));
        break;
/* voip mode */
    case ES305_PATH_VOIP_RECEIVER:
        ret = es305_send_msg_by_array(path_voip_receiver,
                                      SIZE_OF_ARRAY(path_voip_receiver));
        break;
    case ES305_PATH_VOIP_HEADSET:
        ret = es305_send_msg_by_array(path_voip_headset,
                                      SIZE_OF_ARRAY(path_voip_headset));
        break;
    case ES305_PATH_VOIP_HEADPHONE:
        ret = es305_send_msg_by_array(path_voip_headphone,
                                      SIZE_OF_ARRAY(path_voip_headphone));
        break;
    case ES305_PATH_VOIP_SPEAKER:
        ret = es305_send_msg_by_array(path_voip_speaker,
                                      SIZE_OF_ARRAY(path_voip_speaker));
        break;
    case ES305_PATH_VOIP_BT:
        ret = es305_send_msg_by_array(path_voip_bt,
                                      SIZE_OF_ARRAY(path_voip_bt));
        break;
    default:
        loge("%s: invalid pathid : %d", __FUNCTION__, pathid);
        return -EINVAL;
    }

    if (ES305_PATH_BYPASS == pathid) {
        es305_poweroff();
    } else {
        es305_poweron();
    }

    if (0 <= ret) {
        logi("%s: change to path : %d", __FUNCTION__, pathid);
        es305_current_pathid = pathid;
    }

    return ret;
}

static int es305_set_ns(unsigned char ns)
{
    int ret = 0;

    logi("%s", __FUNCTION__);

    if (0 != ns) {
        ret = es305_set_pathid(es305_current_pathid);
        if (0 > ret) {
            loge("%s: set ns on error", __FUNCTION__);
            return ret;
        }
    } else {
        ret = es305_send_msg(0x8017004B);
        if (0 > ret) {
            loge("%s: set ns off error", __FUNCTION__);
            return ret;
        }
        ret = es305_send_msg(0x80180000);
        if (0 > ret) {
            loge("%s: set ns off error", __FUNCTION__);
            return ret;
        }
        ret = es305_send_msg(0x8017000E);
        if (0 > ret) {
            loge("%s: set ns off error", __FUNCTION__);
            return ret;
        }
        ret = es305_send_msg(0x80180000);
        if (0 > ret) {
            loge("%s: set ns off error", __FUNCTION__);
            return ret;
        }
    }

    return ret;
}

static int es305_set_status(unsigned char status)
{
    int ret = 0;
    logi("%s, status = %x", __FUNCTION__, status);

    if (status == es305_status) {
        logi("%s : same status", __FUNCTION__);
        return 0;
    }

    es305_status = status;

    /* set 3A on */
    es305_set_pathid(es305_current_pathid);

    if (0 == (AGC_BIT & status)) {
        ret = es305_send_msg_by_array(disable_agc, SIZE_OF_ARRAY(disable_agc));
        if (0 > ret)
            loge("%s : disable agc error", __FUNCTION__);
    }

    if (0 == (AEC_BIT & status)) {
        ret = es305_send_msg_by_array(disable_aec, SIZE_OF_ARRAY(disable_aec));
        if (0 > ret)
            loge("%s : disable aec error", __FUNCTION__);
    }

    if (0 == (ANR_BIT & status)) {
        ret = es305_send_msg_by_array(disable_anr, SIZE_OF_ARRAY(disable_anr));
        if (0 > ret)
            loge("%s : disable anr error", __FUNCTION__);
    }

    return 0;
}

static int es305_open(struct inode *inode, struct file *file)
{
    struct audience_img *p_img = &img;

    mutex_lock(&es305_lock);
    file->private_data = p_img;
    p_img->img_size = 0;
    p_img->data = NULL;
    mutex_unlock(&es305_lock);
    return 0;
}

static int es305_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long es305_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    struct es305_img img;
    int ret = 0;
    int pathid = 0;
    char msg[4];
    unsigned char status;
    unsigned char ns;
	
    mutex_lock(&es305_lock);
    switch (cmd) {
    case ES305_CMD_RESET:
        es305_hw_reset();
        break;
    case ES305_CMD_SYNC:
        ret = es305_sync();
        break;
    case ES305_CMD_SLEEP:
        if (ES305_SLEEP != es305_mode)
            ret = es305_send_msg(AUDIENCE_MSG_SLEEP_MODE);
        break;
    case ES305_I2C_DOWNLOAD:
        img.buf = 0;
        img.img_size = 0;
        if (copy_from_user(&img, argp, sizeof(img))) {
            loge("%s: get image error", __FUNCTION__);
            ret = -EFAULT;
            goto err_exit;
        }
        ret = es305_i2c_download(file, &img);
        break;
    case ES305_SET_PATHID:
        if (copy_from_user(&pathid, argp, sizeof(pathid))) {
            loge("%s: get pathid from user error", __FUNCTION__);
            ret = -EFAULT;
            goto err_exit;
        }
        ret = es305_set_pathid(pathid);
        break;
    case ES305_SET_STATUS:
        if (copy_from_user(&status, argp, sizeof(status))) {
            loge("%s: get ns from user error", __FUNCTION__);
            ret = -EFAULT;
            goto err_exit;
        }
        ret = es305_set_status(status);
        break;
    case ES305_SET_NS:
        if (copy_from_user(&ns, argp, sizeof(ns))) {
            loge("%s: get ns from user error", __FUNCTION__);
            ret = -EFAULT;
            goto err_exit;
        }
        ret = es305_set_ns(ns);
        break;
    case ES305_WRITE_MSG:
        ret = es305_check_wakeup();
        if (0 > ret) {
            loge("%s: cannot wakeup es305", __FUNCTION__);
            goto err_exit;
        }

        if (copy_from_user(msg, argp, sizeof(msg))) {
            loge("%s: copy_from_user error", __FUNCTION__);
            ret = -EFAULT;
            goto err_exit;
        }
        ret = audience_i2c_write(msg, 4);
        break;
    case ES305_READ_DATA:
        ret = es305_check_wakeup();
        if (0 > ret) {
            loge("%s: cannot wakeup es305", __FUNCTION__);
            goto err_exit;
        }

        ret = audience_i2c_read(msg, 4);
        if (copy_to_user(argp, &msg, 4)){
            loge("%s: copy_to_user error", __FUNCTION__);
            ret = -EFAULT;
            goto err_exit;
        }
        break;
    default:
        loge("%s: invalid command %d", __FUNCTION__, _IOC_NR(cmd));
        ret = -EINVAL;
        break;
    }

err_exit:
    mutex_unlock(&es305_lock);
    return ret;
}

static const struct file_operations es305_fops = {
    .owner          = THIS_MODULE,
    .open           = es305_open,
    .release        = es305_release,
    .unlocked_ioctl = es305_ioctl,
};

static struct miscdevice es305_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = ES305_NAME,
    .fops   = &es305_fops,
};

static int es305_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int ret = 0;

    pdata = client->dev.platform_data;

    if (pdata == NULL) {
        loge("%s: platform data is NULL", __FUNCTION__);
        return -ENOMEM;
    }

    this_client = client;

    es305_iomux_block = iomux_get_block(WAKEUP_IOMUX_BLOCK_NAME);
    if (!es305_iomux_block) {
        loge("%s: get iomux block error", __FUNCTION__);
        return -ENODEV;
    }

    es305_block_config = iomux_get_blockconfig(WAKEUP_IOMUX_BLOCK_NAME);
    if (!es305_block_config) {
        loge("%s: get blockconfig error", __FUNCTION__);
        return -ENODEV;
    }

    /* get ldo_1 */
    g_regu_ldo1 = regulator_get(NULL, "clock-26M-vcc");
    if (IS_ERR(g_regu_ldo1)) {
        pr_err("%s: Could not get regulator : clock-26M-vcc\n", __FUNCTION__);
        return -ENODEV;
    }

    /* get ldo_5 */
    g_regu_ldo5 = regulator_get(NULL, "es305-vcc");
    if (IS_ERR(g_regu_ldo5)) {
        pr_err("%s: Could not get regulator : es305-vcc\n", __FUNCTION__);
        goto err_put_ldo1;
    }

    ret = gpio_request(pdata->gpio_es305_wakeup, "es305");
    if (0 > ret) {
        loge("%s: gpio request wakeup pin failed", __FUNCTION__);
        goto err_put_ldos;
    }

    ret = gpio_request(pdata->gpio_es305_reset, "es305");
    if (0 > ret) {
        loge("%s: gpio request reset pin failed", __FUNCTION__);
        goto err_free_gpio;
    }

    ret = gpio_direction_output(pdata->gpio_es305_reset, 0);
    if (0 > ret) {
        loge("%s: request reset gpio direction failed", __FUNCTION__);
        goto err_free_gpio_all;
    }

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        loge("%s: i2c check functionality error", __FUNCTION__);
        ret = -ENODEV;
        goto err_free_gpio_all;
    }

    ret = misc_register(&es305_device);
    if (ret) {
        loge("%s: es305_device register failed (%d)", __FUNCTION__, ret);
        goto err_free_gpio_all;
    }

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    /* detect current device successful, set the flag as present */
    set_hw_dev_flag(DEV_I2C_AUDIENCE);
#endif
    return 0;

err_free_gpio_all:
    gpio_free(pdata->gpio_es305_reset);
err_free_gpio:
    gpio_free(pdata->gpio_es305_wakeup);
err_put_ldos:
    regulator_put(g_regu_ldo5);
err_put_ldo1:
    regulator_put(g_regu_ldo1);

    return ret;
}

static int es305_remove(struct i2c_client *client)
{
    struct es305_platform_data *p_data = i2c_get_clientdata(client);
    misc_deregister(&es305_device);
    kfree(p_data);
    return 0;
}

static int es305_suspend(struct i2c_client *client, pm_message_t mesg)
{
    return 0;
}

static int es305_resume(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id es305_id[] = {
    { ES305_NAME, 0 },
    { }
};

static struct i2c_driver es305_driver = {
    .probe      = es305_probe,
    .remove     = es305_remove,
    .suspend    = es305_suspend,
    .resume     = es305_resume,
    .id_table   = es305_id,
    .driver     = {
        .name = ES305_NAME,
    },
};

static int __init es305_init(void)
{
    logi("%s", __FUNCTION__);
    mutex_init(&es305_lock);
    return i2c_add_driver(&es305_driver);
}

static void __exit es305_exit(void)
{
    i2c_del_driver(&es305_driver);
}

module_init(es305_init);
module_exit(es305_exit);

MODULE_DESCRIPTION("AUDIENCE ES305 VOICE PROCESSOR DRIVER");
MODULE_LICENSE("GPL");
