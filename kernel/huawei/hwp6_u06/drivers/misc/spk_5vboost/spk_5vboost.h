#ifndef SPK_5VBOOST_H
#define SPK_5VBOOST_H

#define SPK_5VBOOST_NAME        "spk_5vboost"

#define SPK_5VBOOST_IOCTL_MAGIC 'u'

#define SPK_5VBOOST_ENABLE      _IOW(SPK_5VBOOST_IOCTL_MAGIC, 0xC0, unsigned)
#define SPK_5VBOOST_DISABLE     _IOW(SPK_5VBOOST_IOCTL_MAGIC, 0xC1, unsigned)

struct spk_5vboost_platform_data {
    uint32_t gpio_5vboost_en;
};

#endif /* SPK_5VBOOST_H */