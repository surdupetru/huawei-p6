/*
 *  hi3620_hi6421.c
 *  ALSA SoC
 *  cpu-dai   : Hi3620
 *  codec-dai : Hi6421
 */

#include <linux/clk.h>
#include <linux/kernel.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/jack.h>

static struct platform_device *hi3620_hi6421_snd_device;

static struct snd_soc_dai_link hi3620_hi6421_dai_link[] = {
    {
        /* dai link name*/
        .name           = "Hi3620_Hi6421",
        /* stream name same as name */
        .stream_name    = "Hi3620_Hi6421",
        /* codec(hi6421) device name ,see in hi6421.c */
        .codec_name     = "hi6421-codec",
        /* cpu(k3v2:asp) dai name(device name), see in hi3620-pcm.c */
        .cpu_dai_name   = "hi3620-mm",
        /* codec dai name, see in struct snd_soc_dai_driver in hi6421.c */
        .codec_dai_name = "hi6421-dai",
        /* platform(k3v2:asp) device name, see in hi3620-pcm.c */
        .platform_name  = "hi3620-asp",
    },
    {
        .name           = "MODEM_HI6421",
        .stream_name    = "MODEM_HI6421",
        .codec_name     = "hi6421-codec",
        .cpu_dai_name   = "hi3620-modem",
        .codec_dai_name = "hi6421-dai",
        .platform_name  = "hi3620-asp",
    },
    {
        .name           = "FM_HI6421",
        .stream_name    = "FM_HI6421",
        .codec_name     = "hi6421-codec",
        .cpu_dai_name   = "hi3620-fm",
        .codec_dai_name = "hi6421-dai",
        .platform_name  = "hi3620-asp",
    },
    {
        .name           = "BT_HI6421",
        .stream_name    = "BT_HI6421",
        .codec_name     = "hi6421-codec",
        .cpu_dai_name   = "hi3620-bt",
        .codec_dai_name = "hi6421-dai",
        .platform_name  = "hi3620-asp",
    },
    {
        .name           = "DIGITAL_HI6421",
        .stream_name    = "DIGITAL_HI6421",
        .codec_name     = "digital-audio",
        .cpu_dai_name   = "hi3620-digital",
        .codec_dai_name = "digital-audio-dai",
        .platform_name  = "hi3620-aspdigital",
    },
};

/* Audio machine driver */
static struct snd_soc_card snd_soc_hi3620_hi6421 = {
    /* sound card name, can see all sound cards in /proc/asound/cards */
    .name       = "HI3620_HI6421_CARD",
    .dai_link   = hi3620_hi6421_dai_link,
    .num_links  = ARRAY_SIZE(hi3620_hi6421_dai_link),
};

static int hi3620_hi6421_probe(struct platform_device *pdev)
{
    int ret = 0;

    pr_info("%s\n",__FUNCTION__);

    /* alloc soc sound card memory*/
    hi3620_hi6421_snd_device = platform_device_alloc("soc-audio", -1);
    if (!hi3620_hi6421_snd_device) {
        pr_err("%s : Unable to alloc memory for soc sound card\n",__FUNCTION__);
        return -ENOMEM;
    }

    snd_soc_hi3620_hi6421.dev = &pdev->dev;
    platform_set_drvdata(hi3620_hi6421_snd_device, &snd_soc_hi3620_hi6421);

    /* register soc sound card */
    ret = platform_device_add(hi3620_hi6421_snd_device);
    if (ret) {
        pr_err("%s : Unable to register sound card : %s\n",
                    __FUNCTION__, snd_soc_hi3620_hi6421.name);
        platform_device_put(hi3620_hi6421_snd_device);
    }

    return ret;
}

static int hi3620_hi6421_remove(struct platform_device *pdev)
{
    pr_info("%s\n",__FUNCTION__);
    platform_device_unregister(hi3620_hi6421_snd_device);
    return 0;
}

static struct platform_driver hi3620_hi6421_driver = {
    .probe  = hi3620_hi6421_probe,
    .remove = hi3620_hi6421_remove,
    .driver = {
        .name    = "hi3620_hi6421",
        .owner   = THIS_MODULE,
    },
};

static int __init hi3620_hi6421_soc_init(void)
{
    pr_info("%s\n",__FUNCTION__);
    return platform_driver_register(&hi3620_hi6421_driver);
}

static void __exit hi3620_hi6421_soc_exit(void)
{
    pr_info("%s\n",__FUNCTION__);
    platform_driver_unregister(&hi3620_hi6421_driver);
}

module_init(hi3620_hi6421_soc_init);
module_exit(hi3620_hi6421_soc_exit);

/* Module information */
MODULE_AUTHOR("C00166660");
MODULE_DESCRIPTION("Hi3620_Hi6421 ALSA SoC audio driver");
MODULE_LICENSE("GPL");
