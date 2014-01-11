/*
 * Synaptics_SO0340010.h - head file for SO0340010
*/

#ifndef _TOUCHKEY_PLATFORM_CONFIG_H
#define _TOUCHKEY_PLATFORM_CONFIG_H


#define SYNAPTICS_SO340010_NAME "so340010" 
#define S340010_IO "skp_tk"

/* CONFIGURATION REGISTERS  R/W */
#define    INTERFACE_CONFIGUARATION     0x0000
#define    GENERAL_CONFIGURATION           0x0001
#define    BUTTON_ENABLE                           0x0004
#define    GPIO_CONTROL                              0x000E
#define    INTERFERENCE_THRESHOLD         0x000F
#define    SENSOR_PIN10_SENSITIVITY        0x0010
#define    SENSOR_PIN32_SENSITIVITY        0x0011
#define    BUTTON_TO_GPIO_MAPPING         0x001E
#define    TIMER_CONTROL                            0x001F
#define    LED_ENABLE                                   0x0022
#define    LED_EFFECT_PERIOD                     0x0023
#define    LED_CONTROL1                               0x0024
#define    LED_CONTROL2                               0x0025


/* DATA REGISTERS     R ONLY */
#define    GPIO_STATE                                    0x0108
#define    BUTTON_STATE                               0x0109
#define    TIMER_STATE                                  0x010B
#define    PRESSURE_VALUES10                      0x010C
#define    PRESSURE_VALUES32                      0x010D

/* RESET REGISTER      W ONLY */
#define    RESET                                               0x0300


#define TOUCHKEY_GPIO_BOLCK_NAME "block_touchkey"

/* Platform Structure for SO0340010 */
struct synatics_touchkey_platform{
    int attn_gpio;
    struct regulator *vbus;
    unsigned short* (*get_key_map)(void);
//    int (*touchkey_reset)(void);/* touch reset */
    int (*touchkey_gpio_config)(int enable);
};

typedef enum tk_gpio_setup_cfg_tag{
    TK_GPIO_PROBE = 1,
    TK_GPIO_RESUME = 2,
    TK_GPIO_SUSPEND  = 3,
}tk_gpio_setup_cfg;
#endif
