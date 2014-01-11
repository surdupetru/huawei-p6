#ifndef _LINUX_BQ24161_H
#define _LINUX_BQ24161_H

#define	BQ24161_EVENT_UNKOWN				(0x00)
#define	BQ24161_NOT_CHARGING				(0x10)
#define	BQ24161_START_CHARGING				(0x20)
#define	BQ24161_DEFAULT_USB_CHARGING		(0x10)
#define	BQ24161_START_AC_CHARGING			(0x30)
#define	BQ24161_START_USB_CHARGING			(0x40)
#define	BQ24161_CHARGE_DONE					(0x50)
#define	BQ24161_STOP_CHARGING				(0x60)
#define POWER_SUPPLY_STATE_FAULT            (0x70)
#define	BQ24161_NO_CHARGER_SOURCE			(0x00)

#define POWER_SUPPLY_OVERVOLTAGE            (0x80)
#define POWER_SUPPLY_WEAKSOURCE             (0x90)

#define BATTERY_LOW_WARNING      (0xA1)
#define BATTERY_LOW_SHUTDOWN     (0xA2)

#define	BQ24161_FAULT_THERMAL_SHUTDOWN		(0x71)
#define	BQ24161_FAULT_BATTERY_TEMPERATURE	(0x72)
#define	BQ24161_FAULT_WATCHDOG_TIMER		(0x73)
#define	BQ24161_FAULT_SAFETY_TIMER			(0x74)
#define	BQ24161_FAULT_AC_SUPPLY				(0x75)
#define	BQ24161_FAULT_USB_SUPPLY			(0x76)
#define	BQ24161_FAULT_BATTERY				(0x77)

#define	BQ24161_FAULT_VAC_OVP				(0x40)
#define	BQ24161_FAULT_VAC_WEAK				(0x80)
#define	BQ24161_FAULT_VAC_VUVLO				(0xC0)

#define	BQ24161_FAULT_VBUS_OVP				(0x10)
#define	BQ24161_FAULT_VBUS_WEAK				(0x20)
#define	BQ24161_FAULT_VBUS_VUVLO			(0x30)

#define	BQ24161_FAULT_BAT_OVP				(0x02)
#define	BQ24161_FAULT_NO_BATTERY			(0x04)

#define	BQ24161_FAULT_TS_COLD_OR_HOT		(0x02)
#define	BQ24161_FAULT_TS_COLD_AND_COOL		(0x04)
#define	BQ24161_FAULT_TS_WARM_AND_HOT		(0x06)
/* not a bq generated event,we use this to reset the
 * the timer from the twl driver.
 */
#define	BQ24161_RESET_TIMER					(0x38)

#define	I2C_ADDR_BQ24161					(0x6B)

#define	MAX_CHARGE_CURRENT					(1000)
#define	MAX_CHARGE_VOLTAGE					(4200)

#define	BQ24161_GPIO_074					(GPIO_9_2)

/* BQ24160 / BQ24161 / BQ24162 */
/* Status/Control Register */
#define	REG_STATUS_CONTROL					(0x00)
/* write "1" to reset the watchdog*/
#define	TIMER_RST							(1 << 7)
/* USB has precedence when both supplies are connected*/
#define	ENABLE_USB_SUPPLY_SEL				(1 << 3)
/*display charger is not charging*/
#define	BQ24161_USB_READY					(0x20)
 /*display charger is charging from USB interface*/
#define	BQ24161_CHARGING_FROM_USB			(0x40)
#define	ENABLE_ITERM						(1)
#define	DISABLE_ITERM						(0)
#define	DISABLE_CE							(1)
#define	ENABLE_CE							(0)
#define	BQ24161_VUSB_SUPPLY_FAULT			(0x30)
#define	BQ24161_VUSB_FAULT					(0x70)
#define	DISABLE_LOW_CHG						(0)

/* Battery/ Supply Status Register */
#define	REG_BATTERY_AND_SUPPLY_STATUS		(0x01)

/* Control Register */
#define	REG_CONTROL_REGISTER				(0x02)
#define	INPUT_CURRENT_LIMIT_SHIFT			(4)
/* Enable charge current termination*/
#define	ENABLE_ITERM_SHIFT					(2)
/* Enable STAT output to show charge status*/
#define	ENABLE_STAT_PIN						(1 << 3)
/* Charger enabled (default 0 for bq24160, 1 for bq24161/2)*/
#define	ENABLE_ICE_PIN						(0x7D)

/* Control/Battery Voltage Register */
#define	REG_BATTERY_VOLTAGE					(0x03)
#define	VOLTAGE_SHIFT						(2)

/* Vender/Part/Revision Register */
#define	REG_PART_REVISION					(0x04)
#define BQ24161_VERSION_MSK                 (0x18)

/* Battery Termination/Fast Charge Current Register */
#define	REG_BATTERY_CURRENT					(0x05)
/* fast charge current shift bit*/
#define	BQ24161_CURRENT_SHIFT				(3)

/* VIN-DPM Voltage/ DPPM Status Register */
#define	REG_DPPM_VOLTAGE					(0x06)
/* USB input VIN-DPM voltage shift bit */
#define	USB_INPUT_DPPM_SHIFT				(3)

#define	CURRENT_USB_LIMIT_IN				(500)
#define	CURRENT_USB_CHARGE_IN				(550)
#define	CURRENT_AC_LIMIT_IN					(1000)
#define	CURRENT_TERM_CHARGE_IN				(50)
#define	VOLT_DPPM_ADJUST					(4200)
#define VOLT_DPPM_ADJUST_USB                (4520)

/* Safety Timer/NTC Monitor Register */
#define	REG_SAFETY_TIMER					(0x07)
/* Timer slowed by 2x when in thermal regulation*/
#define	ENABLE_TMR_PIN						(1 << 7)
/* 6 hour fast charge*/
#define	TMR_X_6								(1 << 5)
/* 7 hour fast charge*/
#define	TMR_X_9								(1 << 6)
/* TS function enabled*/
#define	ENABLE_TS_PIN						(1 << 3)

/*preconditioning current is 100mA below 3.0V*/
#define	ENABLE_LOW_CHG						(1 << 0)

/*(-10 ) battery temperature is -10 degree*/
#define	BQ24161_COLD_BATTERY_THRESHOLD		(-10)
 /*(0 ) battery temperature is 0 degree*/
#define	BQ24161_COOL_BATTERY_THRESHOLD		(0)
 /*( 40 ) battery temperature is 40 degree*/
#define	BQ24161_WARM_BATTERY_THRESHOLD		(40)
 /*( 50 ) battery temperature is 50 degree*/
#define	BQ24161_HOT_BATTERY_THRESHOLD		(50)
 /*( 3 ) battery temperature offset is 3 degree*/
#define	TEMPERATURE_OFFSET                  (3)
 /*(3.0V) battery preconditioning voltage is 3.0V*/

#define	BQ24161_PRECONDITIONING_BATVOLT_THRESHOLD		(3000)
 /*(3.2V) battery startup voltage is 3.0V*/
#define	BQ24161_STARTUP_BATVOLT_THRESHOLD	(3200)
 /*low temperature charge termination voltage*/
#define	BQ24161_LOW_TEMP_TERM_VOLTAGE		(4000)

#define	BQ24161								(1 << 1)

#define	BQ2416X_USE_WAKE_LOCK				(1)
/* Enable no battery operation*/
#define	EN_NOBATOP							(1 << 0)

#define	DRIVER_NAME							"bq24161"

#define	CIN_LIMIT_100							(100)
#define	CIN_LIMIT_150							(150)
#define	CIN_LIMIT_500							(500)
#define	CIN_LIMIT_800							(800)
#define	CIN_LIMIT_900							(900)
#define	CIN_LIMIT_1500							(1500)

#define	BAT_FULL_VOL							(4200)
#define	BAT_SLOW_VOL							(3300)
#define	LOW_VOL									(3500)
#define	HIGH_VOL								(4440)
#define	VOL_STEP								(20)

#define	LOW_CURRENT								(550)
#define	HIGH_CURRENT							(1500)
#define	HIGH_TERM_CURRENT						(350)
#define	CURRENT_STEP							(75)
#define	TERM_CURRENT_OFFSET						(50)
#define	TERM_CURRENT_STEP						(50)

#define	LOW_DPPM_VOLTAGE						(4200)
#define	HIGH_DPPM_VOLTAGE						(4760)
#define	DPPM_VOLTAGE_STEP						(80)
#define	MONITOR_TIME							(10)

enum plugin_status {
	/* no charger plugin */
	PLUGIN_DEVICE_NONE,
	/* plugin usb charger */
	PLUGIN_USB_CHARGER,
	/* plugin ac charger */
	PLUGIN_AC_CHARGER,
};

struct k3_bq24161_platform_data {
	int max_charger_currentmA;
	int max_charger_voltagemV;
	int termination_currentmA;
	int gpio;
};

struct k3_bq24161_i2c_client {
	int addr;
	struct i2c_client *client;
	int id;
	int (*init_client)(struct k3_bq24161_i2c_client *i2c);
	char *name;
};

int k3_bq24161_register_notifier(struct notifier_block *nb, unsigned int events);
int k3_bq24161_unregister_notifier(struct notifier_block *nb, unsigned int events);

#endif
