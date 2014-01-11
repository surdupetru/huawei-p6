#ifndef _MODEM_BOOT_QSC6085_H
#define _MODEM_BOOT_QSC6085_H

#include <linux/platform_device.h>
#include <linux/gpio.h>

#define MODEM_QSC6085   "qsc6085"
#define MODEM_DEVICE_BOOT(type)   type"_boot"

enum modem_qsc6085_gpio_index {
//MODEM BOOT GPIO
	MODEM_BOOT_POWER_ON,
	MODEM_BOOT_PMU_RESET,
        MODEM_BOOT_POWER_IND,
	MODEM_BOOT_RESET_IND,
//COMM GPIO
	COMM_DOWNLOAD_EN,
	COMM_WAKEUP_SLAVE,
	COMM_WAKEUP_HOST,
//SIM Card
	MODEM_SIM_CARD_SWITCH,
	MODEM_SIM_CARD_DETECT,
	MODEM_BOOT_PSHOLD,
//MODEM GPIO NUM
	MODEM_GPIO_NUM,
};

struct modem_qsc6085_platform_data {
	bool                    flashless;
	struct gpio             gpios[MODEM_GPIO_NUM];
	char                   *block_name;
};

struct modem_boot_device{
	struct device			*device;
	struct work_struct		work;
	int						modem_state;
	spinlock_t				lock;
};

/* The state that can be set through FS interface */
#define POWER_SET_DEBUGOFF     	0    /* Only turn off the modem, for debug USE */
#define POWER_SET_DEBUGON      	1    /* Only turn on the modem, for debug USE */
#define POWER_SET_OFF       	2
#define POWER_SET_ON       	3

#define GPIO_CP_SW_INIT  0
#define GPIO_CP_SW_RDY  ( !(GPIO_CP_SW_INIT) )

#define MODEM_STATE_RESET    0
#define MODEM_STATE_POWERON  1
#endif /*_MODEM_BOOT_QSC6085_H*/

