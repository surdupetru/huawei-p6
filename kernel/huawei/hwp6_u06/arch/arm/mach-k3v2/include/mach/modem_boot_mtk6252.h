#ifndef _MODEM_BOOT_MTK6252_H
#define _MODEM_BOOT_MTK6252_H

#include <linux/platform_device.h>
#include <linux/gpio.h>

#define MODEM_MTK6252   "mtk6252"
#define MODEM_DEVICE_BOOT(type)   type"_boot"

enum modem_mtk6252_gpio_index {
//MODEM_MTK6252 BOOT GPIO
        MODEM_MTK_BOOT_RESET,
        MODEM_MTK_BOOT_POWER_ON,
        MODEM_MTK_BOOT_DOWNLOAD_EN,
        MODEM_MTK_BOOT_SOFTWARE_STATE,
        MODEM_MTK_BOOT_POWER_IND,
        MODEM_MTK_BOOT_RESET_IND,
//COMM GPIO
        COMM_MTK_WAKEUP_SLAVE,
        COMM_MTK_WAKEUP_HOST,
//MODEM GPIO NUM
        MODEM_MTK_GPIO_NUM,
};

struct modem_mtk6252_platform_data {
	bool                    flashless;
	struct gpio             gpios[MODEM_MTK_GPIO_NUM];
	char                   *block_name;
};

#define MTK6252_WAKE_LOCK_TIMEOUT_MS   (2*1000)

/* The state that can be set through FS interface */
#define POWER_SET_DEBUGOFF     	0    /* Only turn off the modem, for debug USE */
#define POWER_SET_DEBUGON      	1    /* Only turn on the modem, for debug USE */
#define POWER_SET_OFF       	2
#define POWER_SET_ON       	3


#endif /*_MODEM_BOOT_MTK6252_H*/

