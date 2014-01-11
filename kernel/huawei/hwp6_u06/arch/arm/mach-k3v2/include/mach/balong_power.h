#ifndef _BALONG_POWER_H
#define _BALONG_POWER_H

#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/types.h>

#define MODEM_BALONG   "balong"
#define MODEM_DEVICE_BOOT(type)   type"_power"   //type"_power"

enum balong_gpio_index {
    BALONG_POWER_ON,
    BALONG_PMU_RESET,
    BALONG_HOST_ACTIVE,
    BALONG_HOST_WAKEUP,
    BALONG_SLAVE_WAKEUP,
    BALONG_POWER_IND,
    BALONG_RESET_IND,
    BALONG_SIM_PNP_IND,
    BALONG_SUSPEND_REQUEST,
    BALONG_SLAVE_ACTIVE,
    BALONG_GPIO_NUM,
};

struct balong_reset_ind_reg {
    resource_size_t base_addr;
    resource_size_t gpioic;
};

struct balong_power_plat_data {
    bool                    flashless;
    struct gpio             gpios[BALONG_GPIO_NUM];
    struct platform_device* echi_device;
    char                   *block_name;
    struct balong_reset_ind_reg *reset_reg;
};


/* The state that can be set through FS interface */
#define POWER_SET_DEBUGOFF      0    /* Only turn off the modem, for debug USE */
#define POWER_SET_DEBUGON       1    /* Only turn on the modem, for debug USE */
#define POWER_SET_OFF           2
#define POWER_SET_ON        3
#define POWER_SET_HSIC_RESET     4   /* when finish download modem image reset HSIC */
#define POWER_SET_ACTIVE    5

/* For usb to call for bus suspend/resume */
extern void balong_power_runtime_idle(void);
extern void balong_power_runtime_resume(void);


#endif /*_balong_POWER_H*/

