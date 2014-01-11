#ifndef _LINUX_BQ_BCI_BATTERY_H
#define _LINUX_BQ_BCI_BATTERY_H


#define VCHRG_POWER_NONE_EVENT         (0x00)
#define VCHRG_NOT_CHARGING_EVENT       (0x10)
#define VCHRG_START_CHARGING_EVENT     (0x20)
#define VCHRG_START_AC_CHARGING_EVENT  (0x30)
#define VCHRG_START_USB_CHARGING_EVENT (0x40)
#define VCHRG_CHARGE_DONE_EVENT        (0x50)
#define VCHRG_STOP_CHARGING_EVENT      (0x60)
#define VCHRG_POWER_SUPPLY_OVERVOLTAGE (0x80)
#define VCHRG_POWER_SUPPLY_WEAKSOURCE  (0x90)
#define BATTERY_LOW_WARNING      0xA1
#define BATTERY_LOW_SHUTDOWN     0xA2

#define BATTERY_CAPACITY_FULL     (100)
#define BATTERY_MONITOR_INTERVAL  (10)
#define BATTERY_LOW_CAPACITY      (15)

#define LOW_VOL_IRQ_NUM           (13)
#define LOW_BAT_VOL_MASK_1        (0x3B)
#define LOW_BAT_VOL_MASK_2        (0x39)
#define LOW_BAT_VOL_MASK_3        (0x38)
#define LOW_BAT_VOL_MASK_4        (0x37)
#define LOW_BAT_VOL_MASK_5        (0x3D)


struct bq_bci_platform_data {
    int *battery_tmp_tbl;
    unsigned int tblsize;

    unsigned int monitoring_interval;

    unsigned int max_charger_currentmA;
    unsigned int max_charger_voltagemV;
    unsigned int termination_currentmA;

   unsigned int max_bat_voltagemV;
   unsigned int low_bat_voltagemV;
};

enum plugin_status {
    /* no charger plugin */
    PLUGIN_DEVICE_NONE,
   /* plugin usb charger */
    PLUGIN_USB_CHARGER,
   /* plugin ac charger */
   PLUGIN_AC_CHARGER,
};

int bq_register_notifier(struct notifier_block *nb,
                unsigned int events);
int bq_unregister_notifier(struct notifier_block *nb,
                unsigned int events);
#endif
