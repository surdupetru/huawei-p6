/*
* HI6421 ALSA Soc codec driver
*/

#include <linux/init.h>
#include <linux/module.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#define DIGITAL_AUDIO_MIN_CHANNELS ( 1 )
#define DIGITAL_AUDIO_MAX_CHANNELS ( 8 )
#define DIGITAL_AUDIO_FORMATS     ( SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)
#define DIGITAL_AUDIO_RATES       ( SNDRV_PCM_RATE_8000_192000)

static inline unsigned int digital_audio_reg_read(struct snd_soc_codec *codec,
            unsigned int reg)
{
     return 0;
}

static inline int digital_audio_reg_write(struct snd_soc_codec *codec,
            unsigned int reg, unsigned int value)
{
    return 0;
}

static int digital_audio_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
    pr_info("%s : set codec rate %d %s\n", __FUNCTION__, params_rate(params),
        substream->stream == SNDRV_PCM_STREAM_PLAYBACK
            ? "PLAYBACK" : "CAPTURE");

    return 0;
}

struct snd_soc_dai_ops digital_audio_dai_ops = {
    .hw_params  = digital_audio_hw_params,
};

struct snd_soc_dai_driver digital_audio_dai = {
    .name = "digital-audio-dai",
    .playback = {
        .stream_name    = "Playback",
        .channels_min   = DIGITAL_AUDIO_MIN_CHANNELS,
        .channels_max   = DIGITAL_AUDIO_MAX_CHANNELS,
        .rates          = DIGITAL_AUDIO_RATES,
        .formats        = DIGITAL_AUDIO_FORMATS},
    .ops = &digital_audio_dai_ops,
};

static int digital_audio_codec_probe(struct snd_soc_codec *codec)
{
    return 0;
}
static int digital_audio_codec_remove(struct snd_soc_codec *codec)
{
    return 0;
}

static struct snd_soc_codec_driver digital_audio_dev = {
    .probe          = digital_audio_codec_probe,
    .remove         = digital_audio_codec_remove,
    .read           = digital_audio_reg_read,
    .write          = digital_audio_reg_write,
};

static int __devinit digital_audio_probe(struct platform_device *pdev)
{
    int ret = -ENODEV;

    ret = snd_soc_register_codec(&pdev->dev, &digital_audio_dev,
                                    &digital_audio_dai, 1);
    if (ret < 0) {
        pr_err("%s: snd_soc_register_codec failed\n", __FUNCTION__);
    }

    return ret;
}

static int __devexit digital_audio_remove(struct platform_device *pdev)
{
    snd_soc_unregister_codec(&pdev->dev);

    return 0;
}

static struct platform_driver digital_audio_driver = {
    .driver = {
        .name  = "digital-audio",
        .owner = THIS_MODULE,
    },
    .probe  = digital_audio_probe,
    .remove = __devexit_p(digital_audio_remove),
};

static int __init digital_audio_init(void)
{
    pr_info("%s\n",__FUNCTION__);
    return platform_driver_register(&digital_audio_driver);
}
module_init(digital_audio_init);

static void __exit digital_audio_exit(void)
{
    pr_info("%s\n",__FUNCTION__);
    platform_driver_unregister(&digital_audio_driver);
}
module_exit(digital_audio_exit);

MODULE_DESCRIPTION("ASoC hdmi audio driver");
MODULE_AUTHOR("c00166660");
MODULE_LICENSE("GPL");
