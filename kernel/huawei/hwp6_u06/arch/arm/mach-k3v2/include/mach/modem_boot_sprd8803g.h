#ifndef _MODEM_BOOT_SPRD8803G_H
#define _MODEM_BOOT_SPRD8803G_H

#include <linux/platform_device.h>
#include <linux/gpio.h>

#define MODEM_SPRD8803G   "sprd8803g"
#define MODEM_DEVICE_BOOT(type)   type"_boot"

enum modem_sprd8803g_gpio_index
{
    MODEM_SPRD_APCP_RTS,
    MODEM_SPRD_CPAP_RDY,
    MODEM_SPRD_CPAP_RTS,
    MODEM_SPRD_APCP_RDY,
    MODEM_SPRD_APCP_FLAG,
    MODEM_SPRD_CPAP_FLAG,
    MODEM_SPRD_CPAP_DINT,
    MODEM_SPRD_APCP_NBOOT,
    MODEM_SPRD_APCP_RSTN,
    MODEM_SPRD_APCP_PWD,
    MODEM_SPRD_CPAP_FLAG2,
    MODEM_SPRD_APCP_RESEND,
    MODEM_SPRD_CPAP_RESEND,
    MODEM_SPRD_APCP_USBSW_OUT,
    MODEM_SPRD_CPAP_USBSW_IRQ,
    MODEM_SPRD_GPIO_NUM,
};



struct modem_sprd8803g_platform_data
{
    bool                    flashless;
    struct gpio             gpios[MODEM_SPRD_GPIO_NUM];
    struct gpio             gpios_flashless[MODEM_SPRD_GPIO_NUM];
    char                   *block_name;
    char                   *block_spi1_name;
    char                   *block_gpio_name;
    char                   *block_spi1_gpio_name;
};

#define GPIO_CP_PM_INIT  0
#define GPIO_CP_PM_RDY  ( !(GPIO_CP_PM_INIT) )

#define GPIO_CP_SW_INIT  0
#define GPIO_CP_SW_RDY  ( !(GPIO_CP_SW_INIT) )

#define GPIO_CP_SYS_INIT  0
#define GPIO_CP_SYS_RDY  ( !(GPIO_CP_SYS_INIT) )

#define SPRD8803G_WAKE_LOCK_TIMEOUT_MS   (2*1000)

/* The state that can be set through FS interface */
#define POWER_SET_DEBUGOFF      0    /* Only turn off the modem, for debug USE */
#define POWER_SET_DEBUGON       1    /* Only turn on the modem, for debug USE */
#define POWER_SET_OFF           2
#define POWER_SET_ON        3
#define POWER_INVALID       4
#endif /*_MODEM_BOOT_SPRD8803G_H*/

