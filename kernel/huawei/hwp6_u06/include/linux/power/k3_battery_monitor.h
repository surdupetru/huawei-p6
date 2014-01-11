#ifndef _LINUX_BATTERY_MONITOR_H
#define _LINUX_BATTERY_MONITOR_H

#define	BATTERY_CAPACITY_FULL			(100)
#define	BATTERY_MONITOR_INTERVAL		(20)
#define	BATTERY_LOW_CAPACITY			(15)

#define	LOW_VOL_IRQ_NUM					(13)
#define	LOW_BAT_VOL_MASK_1				(0x3B)
#define	LOW_BAT_VOL_MASK_2				(0x39)
#define	LOW_BAT_VOL_MASK_3				(0x38)
#define	LOW_BAT_VOL_MASK_4				(0x37)
#define	LOW_BAT_VOL_MASK_5				(0x3D)

struct k3_battery_monitor_platform_data {
	int *battery_tmp_tbl;
	unsigned int tblsize;

	unsigned int monitoring_interval;

	unsigned int max_charger_currentmA;
	unsigned int max_charger_voltagemV;
	unsigned int termination_currentmA;

	unsigned int max_bat_voltagemV;
	unsigned int low_bat_voltagemV;
};
#endif
