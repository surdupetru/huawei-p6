/*
* Filename:	k3_battery_monitor.c
*
* Discription:  this file monitor the battery state, such as voltage, current, capacity,
*		charge state and so on from bq27510 and bq24161 and send these information
*		to the power_supply.
*
* Copyright: 	(C) 2011 huawei.
*
* revision history: 1.0
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/wakelock.h>
#include <linux/hkadc/hiadc_hal.h>
#include <linux/hkadc/hiadc_linux.h>
#include <linux/notifier.h>
#include <linux/power/k3_bq27510.h>
#include <linux/power/k3_bq24161.h>
#include <linux/power/k3_battery_monitor.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <mach/irqs.h>
#include <linux/usb/hiusb_android.h>

extern struct i2c_client *g_battery_measure_by_bq27510_i2c_client;
static struct k3_bq27510_device_info dev27510;
static struct wake_lock low_power_lock;

struct k3_battery_monitor_device_info {
	struct device		*dev;
	int			*battery_tmp_tbl;
	unsigned int		tblsize;

	int			bat_voltage;
	int			bat_temperature;
	int			bat_exist;
	int			bat_health;
	int			bat_capacity;
	int			bat_capacity_level;
	int			bat_technolog;
	int         bat_rm;
	int         bat_fcc;

	int			temperature;
	int			amb_temperature;
	int			voltage_uV;
	int			bk_voltage_uV;
	int			current_uA;
	int			current_avg_uA;
	int			temp_C;
	int			charge_status;
	int			vac_priority;
	int			charger_source;

	int			fuelgauge_mode;
	int			timer_n2;
	int			timer_n1;
	s32			charge_n1;
	s32			charge_n2;
	s16			cc_offset;
	u8			usb_online;
	u8			ac_online;
	u8			stat1;
	u8			status_int1;
	u8			status_int2;

	u8			watchdog_duration;
	u16			current_avg_interval;
	u16			monitoring_interval;
	short			bat_current;
	unsigned int		min_vbus;
	unsigned int		max_charger_voltagemV;
	unsigned int		max_charger_currentmA;
	unsigned int		charger_incurrentmA;
	unsigned int		charger_outcurrentmA;
	unsigned int		regulation_voltagemV;
	unsigned int		low_bat_voltagemV;
	unsigned int		termination_currentmA;
	unsigned long		usb_max_power;
	unsigned long		event;

	unsigned int		capacity;
	unsigned int		capacity_debounce_count;
	unsigned int		ac_last_refresh;
	unsigned int		prev_capacity;
	unsigned int		wakelock_enabled;
	/*defined a flag for sysfs open status*/
	unsigned int		sysfs_flags;

	struct power_supply	bat;
	struct power_supply	usb;
	struct power_supply	ac;
	struct power_supply	bk_bat;

	struct otg_transceiver	*otg;
	struct notifier_block	nb;

	struct workqueue_struct *k3_bat_irq_workqueue;
	struct delayed_work	k3_battery_monitor_work;
	struct delayed_work     vbat_low_int_work;
	int power_supply_status;
};

BLOCKING_NOTIFIER_HEAD(notifier_list_bat);
extern int getcalctemperature(int channel);

static int calc_capacity_from_voltage(void)
{
	int data 				= 20;
	int battery_voltage 			= 0;

	battery_voltage = k3_bq27510_battery_voltage(&dev27510);
	if (battery_voltage < BAT_VOL_3500)
		data = 5;
	else if (battery_voltage < BAT_VOL_3600 && battery_voltage >= BAT_VOL_3500)
		data = 20;
	else if (battery_voltage < BAT_VOL_3700 && battery_voltage >= BAT_VOL_3600)
		data = 50;
	else if (battery_voltage < BAT_VOL_3800 && battery_voltage >= BAT_VOL_3700)
		data = 75;
	else if (battery_voltage < BAT_VOL_3900 && battery_voltage >= BAT_VOL_3800)
		data = 90;
	else if (battery_voltage >= BAT_VOL_3900)
		data = 100;

	return data;
}

/**********ADD BY 00186176 begin****************/
/*this function is just used for test.*/
static int k3_battery_monitor_get_gpadc_conversion(struct k3_battery_monitor_device_info *di, int channel_no)
{
	unsigned char reserve = 0;
	int value = 0;
	int ret = 0;

	ret = k3_adc_open_channel(channel_no);
	if (ret < 0)
		dev_err(di->dev, "%s is error\n", __func__);

	value = k3_adc_get_value(channel_no, &reserve);

	ret = k3_adc_close_channal(channel_no);
	if (ret < 0)
		dev_err(di->dev, "%s is error\n", __func__);

	return value;
}
/**********ADD BY 00186176 END****************/

static enum power_supply_property k3_battery_monitor_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CAPACITY_RM,
	POWER_SUPPLY_PROP_CAPACITY_FCC,
	POWER_SUPPLY_PROP_TEMP_AMBIENT,
};

static enum power_supply_property k3_battery_monitor_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_HEALTH,
};

static enum power_supply_property k3_battery_monitor_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_HEALTH,
};

static enum power_supply_property k3_battery_monitor_bk_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};


static int modem_off = false;
int notify_mdm_off_to_pm(void)
{
	if (system_state != SYSTEM_RUNNING)
		return 0;
	if ((k3_bq27510_battery_voltage(&dev27510) < 3400)&&
		(get_charger_name() == CHARGER_REMOVED)){
        modem_off = true;
        return 1;
	}
	return 0;
}

/*===========================================================================
  Function:     k3_battery_monitor_capacity_changed
  Description:  find battery capacity change 1%
  Input:        struct k3_battery_monitor_device_info *
  Return:       0 : no changed
		1:  changed
===========================================================================*/
static int k3_battery_monitor_capacity_changed(struct k3_battery_monitor_device_info *di)
{
	/*di->capacity;*/
	int curr_capacity = 0;
	int curr_temperature = 0;
    int low_bat_flag = is_k3_bq27510_battery_reach_threshold(&dev27510);
	int ambient_temperature = 0;

	di->bat_exist = is_k3_bq27510_battery_exist(&dev27510);

	if (!di->bat_exist) {
		curr_capacity = calc_capacity_from_voltage();
		curr_temperature = 0;
	} else {
		curr_capacity = k3_bq27510_battery_capacity(&dev27510);
		curr_temperature = k3_bq27510_battery_temperature(&dev27510);
		ambient_temperature = getcalctemperature(0x02) * 10;
	}

   if((low_bat_flag & BQ27510_FLAG_LOCK) != BQ27510_FLAG_LOCK)
       wake_unlock(&low_power_lock);

       /* Debouncing of power on init. */
	if (di->capacity == -1) {
		di->capacity = curr_capacity;
		//di->bat_capacity = curr_capacity;
        di->prev_capacity = curr_capacity;
		di->capacity_debounce_count = 0;
		di->amb_temperature = ambient_temperature;

		return 1;
	}



	if (modem_off) {
        dev_info(di->dev," modem off so shut down AP and curr_capacity = %d \n", curr_capacity);
		curr_capacity = 0;
		return 1;
	}

		di->amb_temperature = ambient_temperature;

/*Only availability if the capacity changed*/
	if (curr_capacity != di->prev_capacity) {
        if (abs(di->prev_capacity -curr_capacity) >= 5){
            dev_info(di->dev,"prev_capacity = %d \n"
                             "curr_capacity = %d \n" "curr_voltage = %d \n",
            di->prev_capacity, curr_capacity,k3_bq27510_battery_voltage(&dev27510));
		}
		di->prev_capacity = curr_capacity;
		di->capacity_debounce_count = 0;
       if ((di->usb_online || di->ac_online) && curr_capacity == 100){
			di->charge_status = POWER_SUPPLY_STATUS_FULL;
	   }
	} else if (++di->capacity_debounce_count >= 4) {
       if ((di->usb_online || di->ac_online) && curr_capacity == 100)
			di->charge_status = POWER_SUPPLY_STATUS_FULL;

		di->capacity = curr_capacity;
		//di->bat_capacity = curr_capacity;
		di->temperature = curr_temperature;
		di->capacity_debounce_count = 0;
		return 1;
	}

	return 0;
}


/*===========================================================================
  Function:     k3_battery_monitor_charger_event
  Description:  Update charger present status for sysfs
  Input:        struct notifier_block *nb
		unsigned long event
		void *_data
  Return:       0 : successed
===========================================================================*/
static int k3_battery_monitor_charger_event(struct notifier_block *nb, unsigned long event,
				void *_data)
{
	int ret = 0;
	struct k3_battery_monitor_device_info *di = container_of(nb, struct k3_battery_monitor_device_info, nb);

	switch (event) {
	case BQ24161_START_USB_CHARGING:
		di->usb_online = 1;
		di->ac_online = 0;
		di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case BQ24161_START_AC_CHARGING:

		di->ac_online = 1;
		di->usb_online = 0;
		di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case BQ24161_STOP_CHARGING:

		di->usb_online = 0;
		di->ac_online = 0;
		di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
	    di->power_supply_status = POWER_SUPPLY_HEALTH_UNKNOWN;
		break;
	case BQ24161_START_CHARGING:
		di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case BQ24161_NOT_CHARGING:
		di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case BQ24161_CHARGE_DONE:
	    di->charge_status = POWER_SUPPLY_STATUS_FULL;
		di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;
		break;

	case POWER_SUPPLY_OVERVOLTAGE:
		di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		di->power_supply_status = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		break;
    case BATTERY_LOW_WARNING:
		/*used for low capacity interrupt.*/
		break;
    case BATTERY_LOW_SHUTDOWN:
        wake_lock(&low_power_lock);
        break;
	default:
		dev_err(di->dev, "%s defualt run.\n", __func__);
		break;
	}

	if (!di->bat_exist)
		di->capacity = calc_capacity_from_voltage();
	else
	    di->capacity = k3_bq27510_battery_capacity(&dev27510);

	if ((di->usb_online || di->ac_online) && di->capacity == 100)
		di->charge_status = POWER_SUPPLY_STATUS_FULL;

        power_supply_changed(&di->bat);
	return ret;
}

static void get_battery_info(struct k3_battery_monitor_device_info *di)
{
	//di->bat_temperature 	= di->temperature;
	//di->bat_voltage 	= k3_bq27510_battery_voltage(&dev27510);
	//di->bat_current 	= k3_bq27510_battery_current(&dev27510);
	//di->bat_capacity 	= di->capacity;
	di->bat_rm = k3_bq27510_battery_rm(&dev27510);
    di->bat_fcc = k3_bq27510_battery_fcc(&dev27510);

	if (!(di->bat_exist)) {
		di->bat_health 		= POWER_SUPPLY_HEALTH_UNKNOWN;
		di->bat_capacity_level 	= POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
		di->bat_technolog 	= POWER_SUPPLY_TECHNOLOGY_UNKNOWN;
		di->bat_temperature = 0;
	} else {
		di->bat_health 		= k3_bq27510_battery_health(&dev27510);
		di->bat_capacity_level 	= k3_bq27510_battery_capacity_level(&dev27510);
		di->bat_technolog 	= k3_bq27510_battery_technology(&dev27510);
		di->bat_temperature = k3_bq27510_battery_temperature(&dev27510);
	}
}

/*===========================================================================
  Function:       k3_battery_monitor_work
  Description:    battery monitor work
  Input:          struct work_struct *work
  Return:         NULL
===========================================================================*/
static void k3_battery_monitor_work(struct work_struct *work)
{
	struct k3_battery_monitor_device_info *di = container_of(work,
		struct k3_battery_monitor_device_info, k3_battery_monitor_work.work);

    get_battery_info(di);
	schedule_delayed_work(&di->k3_battery_monitor_work,
			msecs_to_jiffies(1000 * di->monitoring_interval));
	if (k3_battery_monitor_capacity_changed(di)){
	/*the capacity changed and updata the info of the battery*/
		power_supply_changed(&di->bat);
	}
}

#define to_k3_battery_monitor_ac_device_info(x) container_of((x), \
			struct k3_battery_monitor_device_info, ac);

/*===========================================================================
  Function:     k3_battery_monitor_ac_get_property
  Description:  monitor ac
  Input:        struct power_supply *psy
		enum power_supply_property psp
		union power_supply_propval *val
  Return:       0 :successed
		other:failed
===========================================================================*/
static int k3_battery_monitor_ac_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct k3_battery_monitor_device_info *di = to_k3_battery_monitor_ac_device_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = di->ac_online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/*VBAT VOLTAGE*/
		/**********ADD BY 00186176 begin****************/
		val->intval = k3_battery_monitor_get_gpadc_conversion(di, ADC_VBATMON);
		/**********ADD BY 00186176 END****************/
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = di->power_supply_status;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#define to_k3_battery_monitor_usb_device_info(x) container_of((x), \
		struct k3_battery_monitor_device_info, usb);

/*===========================================================================
  Function:     k3_battery_monitor_usb_get_property
  Description:  monitor usb
  Input:        struct power_supply *psy
		enum power_supply_property psp
		union power_supply_propval *val
  Return:       0 :successed
		other:failed
===========================================================================*/
static int k3_battery_monitor_usb_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct k3_battery_monitor_device_info *di = to_k3_battery_monitor_usb_device_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = di->usb_online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/*VBAT VOLTAGE*/
		/**********ADD BY 00186176 begin****************/
		val->intval = k3_battery_monitor_get_gpadc_conversion(di, ADC_VBATMON);
		/**********ADD BY 00186176 END****************/
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = di->power_supply_status;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#define to_k3_battery_monitor_bk_device_info(x) container_of((x), \
		struct k3_battery_monitor_device_info, bk_bat);

/*===========================================================================
  Function:     k3_battery_monitor_bk_get_property
  Description:  monitor bk
  Input:        struct power_supply *psy
		enum power_supply_property psp
		union power_supply_propval *val
  Return:       0 :successed
		other:failed
===========================================================================*/
static int k3_battery_monitor_bk_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct k3_battery_monitor_device_info *di = to_k3_battery_monitor_bk_device_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/*Use gpadc channel for measuring bk battery voltage*/
		/**********ADD BY 00186176 begin****************/
		val->intval = k3_battery_monitor_get_gpadc_conversion(di, ADC_VBATMON);
		/**********ADD BY 00186176 END****************/
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#define to_k3_battery_monitor_device_info(x) container_of((x), \
			struct k3_battery_monitor_device_info, bat);

/*===========================================================================
  Function:     k3_battery_monitor_get_property
  Description:  BQ27510 device driver manage battery para and provide interfaces to sysfs
  Input:        struct power_supply *psy
		enum power_supply_property psp
		union power_supply_propval *val
  Return:       0 :successed
		other:failed
===========================================================================*/
static int k3_battery_monitor_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct k3_battery_monitor_device_info *di;

	di = to_k3_battery_monitor_device_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = di->charge_status;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        di->bat_voltage = k3_bq27510_battery_voltage(&dev27510);
		val->intval = di->bat_voltage * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	    di->bat_current = k3_bq27510_battery_current(&dev27510);
		val->intval = di->bat_current;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = di->bat_temperature * 10;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		val->intval = di->amb_temperature;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = di->bat_exist;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = di->bat_health;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
                if (modem_off) {
                    val->intval = 0;
                }else {
                      val->intval = di->capacity;
		}
		break;

	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		val->intval = di->bat_capacity_level;
		break;

	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = di->bat_technolog;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_RM:
		val->intval = di->bat_rm;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_FCC:
		val->intval = di->bat_fcc;
		break;

	default:
		dev_err(di->dev, "%s defualt run.\n", __func__);
		return -EINVAL;
	}

	return 0;
}

int k3_battery_monitor_register_notifier(struct notifier_block *nb,
				unsigned int events)
{
	return blocking_notifier_chain_register(&notifier_list_bat, nb);
}
EXPORT_SYMBOL_GPL(k3_battery_monitor_register_notifier);

int k3_battery_monitor_unregister_notifier(struct notifier_block *nb,
				unsigned int events)
{
	return blocking_notifier_chain_unregister(&notifier_list_bat, nb);
}
EXPORT_SYMBOL_GPL(k3_battery_monitor_unregister_notifier);

static char *k3_battery_monitor_supplied_to[] = {
	"k3_battery_monitor",
};

static int __devinit k3_battery_monitor_probe(struct platform_device *pdev)
{
	struct k3_battery_monitor_platform_data *pdata = NULL;
	struct k3_battery_monitor_device_info *di = NULL;
	int low_bat_flag;
	int ret = 0;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_dbg(&pdev->dev, "di is NULL\n");
		return -ENOMEM;
	}

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_dbg(&pdev->dev, "platform_data not available\n");
		ret = -EINVAL;
		goto err_pdata;
	}

	/* Get I2C client - bq27510 */
	if (g_battery_measure_by_bq27510_i2c_client) {
		dev27510.client = g_battery_measure_by_bq27510_i2c_client;
	} else {
		dev_err(&pdev->dev, "%s failed, check bq27510 module installed...\n", __func__);
		ret = -EINVAL;
		goto err_pdata;
	}

	if (pdata->monitoring_interval == 0) {
		di->monitoring_interval = 10;
		di->current_avg_interval = 10;
	} else {
		di->monitoring_interval = pdata->monitoring_interval;
		di->current_avg_interval = pdata->monitoring_interval;
	}

	di->max_charger_currentmA = pdata->max_charger_currentmA;
	di->max_charger_voltagemV = pdata->max_bat_voltagemV;
	di->termination_currentmA = pdata->termination_currentmA;
	di->regulation_voltagemV = pdata->max_bat_voltagemV;
	di->low_bat_voltagemV = pdata->low_bat_voltagemV;
	di->battery_tmp_tbl = pdata->battery_tmp_tbl;
	di->tblsize = pdata->tblsize;

	di->dev = &pdev->dev;

	di->bat.name = "Battery";
	di->bat.supplied_to = k3_battery_monitor_supplied_to;
	di->bat.num_supplicants = ARRAY_SIZE(k3_battery_monitor_supplied_to);
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = k3_battery_monitor_props;
	di->bat.num_properties = ARRAY_SIZE(k3_battery_monitor_props);
	di->bat.get_property = k3_battery_monitor_get_property;
	di->bat_health = POWER_SUPPLY_HEALTH_GOOD;
    di->temperature = 0;
	di->bat_exist = is_k3_bq27510_battery_exist(&dev27510);

	di->usb.name = "USB";
	di->usb.type = POWER_SUPPLY_TYPE_USB;
	di->usb.properties = k3_battery_monitor_usb_props;
	di->usb.num_properties = ARRAY_SIZE(k3_battery_monitor_usb_props);
	di->usb.get_property = k3_battery_monitor_usb_get_property;
	di->power_supply_status = POWER_SUPPLY_HEALTH_GOOD;

	di->ac.name = "Mains";
	di->ac.type = POWER_SUPPLY_TYPE_MAINS;
	di->ac.properties = k3_battery_monitor_ac_props;
	di->ac.num_properties = ARRAY_SIZE(k3_battery_monitor_ac_props);
	di->ac.get_property = k3_battery_monitor_ac_get_property;

	di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;

	di->bk_bat.name = "k3_bk_battery";
	di->bk_bat.type = POWER_SUPPLY_TYPE_UPS;
	di->bk_bat.properties = k3_battery_monitor_bk_props;
	di->bk_bat.num_properties = ARRAY_SIZE(k3_battery_monitor_bk_props);
	di->bk_bat.get_property = k3_battery_monitor_bk_get_property;

	di->vac_priority = 2;
	di->capacity = -1;
	di->capacity_debounce_count = 0;
	di->ac_last_refresh = jiffies;


	get_battery_info(di);

	platform_set_drvdata(pdev, di);

    wake_lock_init(&low_power_lock, WAKE_LOCK_SUSPEND, "low_power_wake_lock");

    low_bat_flag = is_k3_bq27510_battery_reach_threshold(&dev27510);
    if(( low_bat_flag & BQ27510_FLAG_LOCK ) == BQ27510_FLAG_LOCK) {
        wake_lock(&low_power_lock);
    }

	ret = power_supply_register(&pdev->dev, &di->bat);
	if (ret) {
		dev_err(&pdev->dev, "failed to register main battery\n");
		goto batt_failed;
	}

	ret = power_supply_register(&pdev->dev, &di->usb);
	if (ret) {
		dev_err(&pdev->dev, "failed to register usb power supply\n");
		goto usb_failed;
	}

	ret = power_supply_register(&pdev->dev, &di->ac);
	if (ret) {
		dev_err(&pdev->dev, "failed to register ac power supply\n");
		goto ac_failed;
	}

	ret = power_supply_register(&pdev->dev, &di->bk_bat);
	if (ret) {
		dev_err(&pdev->dev, "failed to register backup battery\n");
		goto bk_batt_failed;
	}

	INIT_DELAYED_WORK(&di->k3_battery_monitor_work,
				k3_battery_monitor_work);

	schedule_delayed_work(&di->k3_battery_monitor_work, 0);

	di->charge_n1 = 0;
	di->timer_n1 = 0;
	di->sysfs_flags = 0;

	di->nb.notifier_call = k3_battery_monitor_charger_event;
	k3_battery_monitor_register_notifier(&di->nb, 1);

	return ret;

bk_batt_failed:
	power_supply_unregister(&di->ac);
ac_failed:
	power_supply_unregister(&di->usb);
usb_failed:
	cancel_delayed_work(&di->k3_battery_monitor_work);
	power_supply_unregister(&di->bat);
batt_failed:
    wake_lock_destroy(&low_power_lock);
	platform_set_drvdata(pdev, NULL);
err_pdata:
	kfree(di);
	/**********ADD BY 00186176 begin****************/
	di = NULL;
	/**********ADD BY 00186176 END****************/

	return ret;
}

static int __devexit k3_battery_monitor_remove(struct platform_device *pdev)
{
	struct k3_battery_monitor_device_info *di = platform_get_drvdata(pdev);

	if (di == NULL) {
		dev_err(&pdev->dev, "di is NULL!\n");
		return -ENODEV;
	}
	cancel_delayed_work(&di->k3_battery_monitor_work);
	k3_battery_monitor_unregister_notifier(&di->nb, 1);

	flush_scheduled_work();

	power_supply_unregister(&di->bat);
	power_supply_unregister(&di->usb);
	power_supply_unregister(&di->ac);
	power_supply_unregister(&di->bk_bat);
    wake_lock_destroy(&low_power_lock);
	platform_set_drvdata(pdev, NULL);

	kfree(di);
	/**********ADD BY 00186176 begin****************/
	di = NULL;
	/**********ADD BY 00186176 END****************/

	return 0;
}

/**********ADD BY 00186176 begin****************/
static void k3_battery_monitor_shutdown(struct platform_device *pdev)
{
	struct k3_battery_monitor_device_info *di = platform_get_drvdata(pdev);

	if (di == NULL) {
		dev_err(&pdev->dev, "di is NULL!\n");
		return ;
	}

	printk("[%s] +\n", __func__);

	cancel_delayed_work(&di->k3_battery_monitor_work);
	flush_scheduled_work();
    wake_lock_destroy(&low_power_lock);

	return;

}
/**********ADD BY 00186176 END****************/

#ifdef CONFIG_PM
static int k3_battery_monitor_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	struct k3_battery_monitor_device_info *di = platform_get_drvdata(pdev);

	if (di == NULL) {
		dev_err(&pdev->dev, "di is NULL!\n");
		return -ENODEV;
	}

	dev_dbg(&pdev->dev, "k3_battery_monitor_suspend  +\n");

	cancel_delayed_work(&di->k3_battery_monitor_work);

	dev_dbg(&pdev->dev, "k3_battery_monitor_suspend  -\n");

	return 0;
}

static int k3_battery_monitor_resume(struct platform_device *pdev)
{
	struct k3_battery_monitor_device_info *di = platform_get_drvdata(pdev);

	if (di == NULL) {
		dev_err(&pdev->dev, "di is NULL!\n");
		return -ENODEV;
	}

	dev_dbg(&pdev->dev, "k3_battery_monitor_suspend  +\n");

	schedule_delayed_work(&di->k3_battery_monitor_work, 0);

	dev_dbg(&pdev->dev, "k3_battery_monitor_suspend  -\n");

	return 0;
}
#else
#define k3_battery_monitor_suspend	NULL
#define k3_battery_monitor_resume	NULL
#endif

static struct platform_driver k3_battery_monitor_driver = {
	.probe		= k3_battery_monitor_probe,
	.remove		= __devexit_p(k3_battery_monitor_remove),
	.suspend	= k3_battery_monitor_suspend,
	.resume		= k3_battery_monitor_resume,
	.shutdown	= k3_battery_monitor_shutdown,
	.driver		= {
	.name		= "k3_battery_monitor",
	},
};

static int __init k3_battery_monitor_init(void)
{
	return platform_driver_register(&k3_battery_monitor_driver);
}
module_init(k3_battery_monitor_init);

static void __exit k3_battery_monitor_exit(void)
{
	platform_driver_unregister(&k3_battery_monitor_driver);
}
module_exit(k3_battery_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:k3_bq_battery");
MODULE_AUTHOR("HUAWEI Inc");
