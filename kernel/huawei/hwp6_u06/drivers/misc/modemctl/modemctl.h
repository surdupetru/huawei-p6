#ifndef MODEMCTL_H
#define MODEMCTL_H

#define MODEMCTL_NAME "modemctl"

#define MODEMCTL_IOCTL_MAGIC 'u'

#define MODEMCTL_SWITCH_TO_FIRST      _IOW(MODEMCTL_IOCTL_MAGIC, 0xC0, unsigned)
#define MODEMCTL_SWITCH_TO_SECOND     _IOW(MODEMCTL_IOCTL_MAGIC, 0xC1, unsigned)

struct modemctl_platform_data {
    uint32_t gpio_modemctl_en;
};

#endif //MODEMCTL_H