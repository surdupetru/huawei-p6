/* ***************************************************************************************/
/* Filename:	k3_bq24161.c
*
* Discription:  this file monitor the battery charge state and send the information
*		to the battery monitor.
*
* Copyright: 	(C) 2011 huawei.
*
* revision history: 1.0
******************************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/wakelock.h>
#include <linux/usb/otg.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/power_supply.h>
#include <linux/power/k3_bq24161.h>
#include <linux/power/k3_bq27510.h>
#include <mach/gpio.h>
#include <mach/irqs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/mux.h>
#include <linux/usb/hiusb_android.h>
#include <linux/mutex.h>
#include <hsad/config_mgr.h>

static int		timer_fault;

struct charge_params {
	unsigned long		currentmA;
	unsigned long		voltagemV;
	unsigned long		term_currentmA;
	unsigned long		enable_iterm;
	bool			enable;
};

struct k3_bq24161_device_info {
	struct device		*dev;
	struct i2c_client	*client;
	struct charge_params	params;
	struct delayed_work	bq24161_charger_work;
	struct delayed_work	charge_work;
	struct work_struct	usb_work;

	unsigned short		status_reg;
	unsigned short		control_reg;
	unsigned short		voltage_reg;
	unsigned short		bqchip_version;
	unsigned short		current_reg;
	unsigned short		special_charger_reg;

	unsigned int		cin_limit;
	unsigned int		currentmA;
	unsigned int		voltagemV;
	unsigned int		max_currentmA;
	unsigned int		max_voltagemV;
	unsigned int		term_currentmA;
	unsigned int      	dppm_voltagemV;
	unsigned short		usb_supply_sel_reg;
	unsigned short		tmr_ts_reg;
	unsigned short		dppm_reg;
	bool           		enable_ce;
	bool          		hz_mode;

	bool          		cd_active;
	bool			cfg_params;
	bool			enable_iterm;
	bool			active;
    bool            battery_present;
    bool            calling_limit;

	int			charge_status;
	int			charger_source;

	unsigned long		event;

	struct notifier_block	nb;

	bool   			enable_low_chg;

	/*hand control charging process*/
	bool   			factory_flag;

	/**********ADD BY 00186176 begin****************/
	int 			gpio;
	struct iomux_block  	*piomux_block;
	struct block_config 	*pblock_config;
	/**********ADD BY 00186176 END****************/

#if BQ2416X_USE_WAKE_LOCK
	struct wake_lock 	charger_wake_lock;
#endif
};

/*exterm a bq27510 instance for reading battery info*/
extern struct k3_bq27510_device_info *g_battery_measure_by_bq27510_device;
/*extern a notifier list for charging notification*/
extern struct blocking_notifier_head notifier_list_bat;
extern u32 wakeup_timer_seconds;

struct k3_bq24161_i2c_client *k3_bq24161_client;

enum{
    BATTERY_HEALTH_TEMPERATURE_NORMAL = 0,
	BATTERY_HEALTH_TEMPERATURE_OVERLOW,
	BATTERY_HEALTH_TEMPERATURE_LOW,
	BATTERY_HEALTH_TEMPERATURE_HIGH,
	BATTERY_HEALTH_TEMPERATURE_OVERHIGH,
	BATTERY_HEALTH_TEMPERATURE_NORMAL_HIGH,
};

/**
 * k3_bq24161_write_block:
 * returns 0 if write successfully
 * This is API to check whether OMAP is waking up from device OFF mode.
 * There is no other status bit available for SW to read whether last state
 * entered was device OFF. To work around this, CORE PD, RFF context state
 * is used which is lost only when we hit device OFF state
 */
int k3_bq24161_write_block(struct k3_bq24161_device_info *di, u8 *value,
						u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[1];
	int ret = 0;

	*value		= reg;

	msg[0].addr	= di->client->addr;
	msg[0].flags	= 0;
	msg[0].buf	= value;
	msg[0].len	= num_bytes + 1;

	ret = i2c_transfer(di->client->adapter, msg, 1);

	/* i2c_transfer returns number of messages transferred */
	if (ret != 1) {
		dev_err(di->dev,
			"i2c_write failed to transfer all messages\n");
		if (ret < 0)
			return ret;
		else
			return -EIO;
	} else {
		return 0;
	}
}

static int k3_bq24161_read_block(struct k3_bq24161_device_info *di, u8 *value,
						u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[2];
	u8 buf = 0;
	int ret = 0;

	buf		= reg;

	msg[0].addr	= di->client->addr;
	msg[0].flags	= 0;
	msg[0].buf	= &buf;
	msg[0].len	= 1;

	msg[1].addr	= di->client->addr;
	msg[1].flags	= I2C_M_RD;
	msg[1].buf	= value;
	msg[1].len	= num_bytes;

	ret = i2c_transfer(di->client->adapter, msg, 2);

	/* i2c_transfer returns number of messages transferred */
	if (ret != 2) {
		dev_err(di->dev,
			"i2c_write failed to transfer all messages\n");
		if (ret < 0) {
			return ret;
		} else
			return -EIO;
	} else {
		return 0;
	}
}

static int k3_bq24161_write_byte(struct k3_bq24161_device_info *di, u8 value, u8 reg)
{
	/* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */
	u8 temp_buffer[2] = { 0 };

	/* offset 1 contains the data */
	temp_buffer[1] = value;
	return k3_bq24161_write_block(di, temp_buffer, reg, 1);
}

static int k3_bq24161_read_byte(struct k3_bq24161_device_info *di, u8 *value, u8 reg)
{
	return k3_bq24161_read_block(di, value, reg, 1);
}


/*===========================================================================
  Function:       k3_bq24161_config_usb_supply_sel_reg
  Description:    config TMR_RST function to reset the watchdog
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void k3_bq24161_config_usb_supply_sel_reg(struct k3_bq24161_device_info *di)
{

	di->usb_supply_sel_reg = (TIMER_RST);
	k3_bq24161_write_byte(di, di->usb_supply_sel_reg, REG_STATUS_CONTROL);

	return;
}

/*===========================================================================
  Function:       k3_bq24161_config_status_reg
  Description:    Enable STAT pin output to show charge status
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void k3_bq24161_config_status_reg(struct k3_bq24161_device_info *di)
{

	k3_bq24161_config_usb_supply_sel_reg(di);

	k3_bq24161_write_byte(di, di->control_reg, REG_CONTROL_REGISTER);
	return;
}

/*===========================================================================
  Function:       k3_bq24161_config_control_reg
  Description:    config bq24161 control resister
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void k3_bq24161_config_control_reg(struct k3_bq24161_device_info *di)
{
	u8 Iin_limit = 0;

	if (di->cin_limit <= CIN_LIMIT_100)
		Iin_limit = 0;
	else if (di->cin_limit > CIN_LIMIT_100 && di->cin_limit <= CIN_LIMIT_150)
		Iin_limit = 1;
	else if (di->cin_limit > CIN_LIMIT_150 && di->cin_limit <= CIN_LIMIT_500)
		Iin_limit = 2;
	else if (di->cin_limit > CIN_LIMIT_500 && di->cin_limit <= CIN_LIMIT_800)
		Iin_limit = 3;
	else if (di->cin_limit > CIN_LIMIT_800 && di->cin_limit <= CIN_LIMIT_900)
		Iin_limit = 4;
	else if (di->cin_limit > CIN_LIMIT_900 && di->cin_limit <= CIN_LIMIT_1500)
		Iin_limit = 5;
	else
		Iin_limit = 6;

	di->control_reg = ((Iin_limit << INPUT_CURRENT_LIMIT_SHIFT)
				| (di->enable_iterm << ENABLE_ITERM_SHIFT) | ENABLE_STAT_PIN
				| (di->enable_ce << 1) | di->hz_mode);
	k3_bq24161_write_byte(di, di->control_reg, REG_CONTROL_REGISTER);

	return;
}


/*===========================================================================
  Function:       k3_bq24161_config_voltage_reg
  Description:    set Battery Regulation Voltage between 3.5V and 4.44V
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void k3_bq24161_config_voltage_reg(struct k3_bq24161_device_info *di)
{
	unsigned int voltagemV = 0;
	u8 Voreg = 0;

	voltagemV = di->voltagemV;
	if (voltagemV < LOW_VOL)
		voltagemV = LOW_VOL;
	else if (voltagemV > HIGH_VOL)
		voltagemV = HIGH_VOL;

	Voreg = (voltagemV - LOW_VOL)/VOL_STEP;
	di->voltage_reg = (Voreg << VOLTAGE_SHIFT);
	k3_bq24161_write_byte(di, di->voltage_reg, REG_BATTERY_VOLTAGE);
	return;
}


/*===========================================================================
  Function:       k3_bq24161_config_current_reg
  Description:    set Battery charger current(550 ~1500mA) and Termination current(50~350mA)
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void k3_bq24161_config_current_reg(struct k3_bq24161_device_info *di)
{
	unsigned int currentmA = 0;
	unsigned int term_currentmA = 0;
	u8 Vichrg = 0;
	u8 shift = 0;
	u8 Viterm = 0;

	currentmA = di->currentmA;
	term_currentmA = di->term_currentmA;

	if (currentmA < LOW_CURRENT)
		currentmA = LOW_CURRENT;

	if ((di->bqchip_version & BQ24161)) {
		shift = BQ24161_CURRENT_SHIFT;
		if (currentmA > HIGH_CURRENT)
			currentmA = HIGH_CURRENT;
	}

	if (term_currentmA > HIGH_TERM_CURRENT)
		term_currentmA = HIGH_TERM_CURRENT;

	Vichrg = (currentmA - LOW_CURRENT)/CURRENT_STEP;
	Viterm = (term_currentmA-TERM_CURRENT_OFFSET)/TERM_CURRENT_STEP;

	di->current_reg = (Vichrg << shift | Viterm);
	k3_bq24161_write_byte(di, di->current_reg, REG_BATTERY_CURRENT);
	return;
}


/*===========================================================================
  Function:       k3_bq24161_config_dppm_voltage_reg
  Description:    set USB input dppm voltage between 4.2V and 4.76V
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void k3_bq24161_config_dppm_voltage_reg(struct k3_bq24161_device_info *di,
			unsigned int dppm_voltagemV)
{
      u8 Vmreg;

	if (dppm_voltagemV < LOW_DPPM_VOLTAGE)
		dppm_voltagemV = LOW_DPPM_VOLTAGE;
	else if (dppm_voltagemV > HIGH_DPPM_VOLTAGE)
		dppm_voltagemV = HIGH_DPPM_VOLTAGE;

	Vmreg = (dppm_voltagemV - LOW_DPPM_VOLTAGE)/DPPM_VOLTAGE_STEP;

	di->dppm_reg = (Vmreg << USB_INPUT_DPPM_SHIFT);
	k3_bq24161_write_byte(di, di->dppm_reg,	REG_DPPM_VOLTAGE);

	return;
}


/*===========================================================================
  Function:       k3_bq24161_config_safety_reg
  Description:    enable TMR_PIN and set Safety Timer Time Limit = 6h
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void k3_bq24161_config_safety_reg(struct k3_bq24161_device_info *di)
{
	di->tmr_ts_reg = (ENABLE_TMR_PIN | TMR_X_9)  | di->enable_low_chg;
	k3_bq24161_write_byte(di, di->tmr_ts_reg, REG_SAFETY_TIMER);

	return;
}

static void k3_bq24161_calling_limit_ac_input_current(struct k3_bq24161_device_info *di)
{
    if (di->charger_source == POWER_SUPPLY_TYPE_MAINS)
    {
        if(di->calling_limit){
            di->cin_limit = CIN_LIMIT_800;
        }
        else {
            di->cin_limit = CURRENT_AC_LIMIT_IN;
        }
    } else {
        di->cin_limit = CURRENT_USB_LIMIT_IN;
	}
   return;
}

/*===========================================================================
  Function:       k3_bq24161_charge_status
  Description:    get bq24161 charge status
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void
k3_bq24161_charge_status(struct k3_bq24161_device_info *di)
{
    long int events = BQ24161_START_CHARGING;
    u8 read_reg[8] = {0};

	k3_bq24161_read_block(di, &read_reg[0], 0, 2);
	if((read_reg[1] & BQ24161_FAULT_VBUS_VUVLO) == BQ24161_FAULT_VBUS_OVP)
	{
        dev_err(di->dev, " POWER_SUPPLY_OVERVOLTAGE = %x\n",read_reg[1]);
        events = POWER_SUPPLY_OVERVOLTAGE;
        blocking_notifier_call_chain(&notifier_list_bat, events, NULL);
        return;
	}

    if ((read_reg[0] & BQ24161_VUSB_FAULT) == BQ24161_CHARGING_FROM_USB) {
        events = BQ24161_START_CHARGING;

    } else if (di->enable_ce == DISABLE_CE){
        dev_err(di->dev, " BQ24161_NOT_CHARGING \n");
		events = BQ24161_NOT_CHARGING;

	} else if((read_reg[0] & BQ24161_VUSB_FAULT) == BQ24161_CHARGE_DONE){
        if (!is_k3_bq27510_battery_full(g_battery_measure_by_bq27510_device)) {
            /*enable bq2416x charger high impedance mode*/
            di->hz_mode = 1;
            k3_bq24161_write_byte(di, di->control_reg | di->hz_mode, REG_CONTROL_REGISTER);
            /*disable bq2416x charger high impedance mode*/
            msleep(1000);
            di->hz_mode = 0;
            k3_bq24161_config_control_reg(di);
            events = BQ24161_START_CHARGING;
		} else {
            events = BQ24161_CHARGE_DONE;
		}
    } else {
        events=BQ24161_START_CHARGING;
    }

    if(di->charger_source == POWER_SUPPLY_TYPE_BATTERY){
        return;
	}

    blocking_notifier_call_chain(&notifier_list_bat, events, NULL);
    return;
}

static int k3_bq24161_check_battery_temperature_threshold(void)
{
    int battery_temperature = 0;

    battery_temperature = k3_bq27510_battery_temperature(g_battery_measure_by_bq27510_device);

    if (battery_temperature < BQ24161_COLD_BATTERY_THRESHOLD) {
        return BATTERY_HEALTH_TEMPERATURE_OVERLOW;

	} else if((battery_temperature >= BQ24161_COLD_BATTERY_THRESHOLD)
      && (battery_temperature <  BQ24161_COOL_BATTERY_THRESHOLD)){
        return BATTERY_HEALTH_TEMPERATURE_LOW;

	} else if ((battery_temperature >= BQ24161_COOL_BATTERY_THRESHOLD)
	  && (battery_temperature < (BQ24161_WARM_BATTERY_THRESHOLD - TEMPERATURE_OFFSET))){
        return BATTERY_HEALTH_TEMPERATURE_NORMAL;

	} else if ((battery_temperature >= (BQ24161_WARM_BATTERY_THRESHOLD - TEMPERATURE_OFFSET))
	  && (battery_temperature < BQ24161_WARM_BATTERY_THRESHOLD)){
        return BATTERY_HEALTH_TEMPERATURE_NORMAL_HIGH;

	} else if ((battery_temperature >= BQ24161_WARM_BATTERY_THRESHOLD)
	  && (battery_temperature < BQ24161_HOT_BATTERY_THRESHOLD)){
        return BATTERY_HEALTH_TEMPERATURE_HIGH;

	} else if (battery_temperature >= BQ24161_HOT_BATTERY_THRESHOLD){
        return BATTERY_HEALTH_TEMPERATURE_OVERHIGH;

	} else {
		return BATTERY_HEALTH_TEMPERATURE_NORMAL;
	}
}

/*===========================================================================
  Function:       k3_bq24161_low_current_charge
  Description:    small current charging(100mA) when low battery voltage or low battery temprature
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void
k3_bq24161_low_current_charge(struct k3_bq24161_device_info *di)
{
	int battery_voltage = 0;
    int battery_temp_status = 0;

   if(!di->battery_present)
       return;

	battery_voltage = k3_bq27510_battery_voltage(g_battery_measure_by_bq27510_device);
    battery_temp_status = k3_bq24161_check_battery_temperature_threshold();

	switch (battery_temp_status) {
	case BATTERY_HEALTH_TEMPERATURE_OVERLOW:
        di->enable_ce = DISABLE_CE;
        k3_bq24161_calling_limit_ac_input_current(di);
         break;
	case BATTERY_HEALTH_TEMPERATURE_LOW:
	     /*enable low charge,100mA charging*/
        di->enable_low_chg = ENABLE_LOW_CHG;
        k3_bq24161_config_safety_reg(di);
        k3_bq24161_calling_limit_ac_input_current(di);
        if (battery_voltage < BQ24161_LOW_TEMP_TERM_VOLTAGE) {
            di->enable_ce = ENABLE_CE;
        } else {
           di->enable_ce = DISABLE_CE;
        }
	     break;
    case BATTERY_HEALTH_TEMPERATURE_NORMAL:
        k3_bq24161_calling_limit_ac_input_current(di);
        if (battery_voltage < BQ24161_PRECONDITIONING_BATVOLT_THRESHOLD) {
	        /*enable low charge,100mA charging*/
            di->enable_low_chg = ENABLE_LOW_CHG;
        } else {
            di->enable_low_chg = DISABLE_LOW_CHG;
        }
        k3_bq24161_config_safety_reg(di);
		 di->enable_ce = ENABLE_CE;
	     break;
	case BATTERY_HEALTH_TEMPERATURE_HIGH:
        if (battery_voltage < BQ24161_PRECONDITIONING_BATVOLT_THRESHOLD) {
	        /*enable low charge,100mA charging*/
            di->enable_low_chg = ENABLE_LOW_CHG;
        } else {
            di->enable_low_chg = DISABLE_LOW_CHG;
        }
        k3_bq24161_config_safety_reg(di);
        di->enable_ce = ENABLE_CE;
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS)
            di->cin_limit = CIN_LIMIT_800;
        else
            di->cin_limit = CURRENT_USB_LIMIT_IN;
	    break;
	case BATTERY_HEALTH_TEMPERATURE_OVERHIGH:
        di->enable_ce = DISABLE_CE;
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS)
            di->cin_limit = CIN_LIMIT_800;
        else
            di->cin_limit = CURRENT_USB_LIMIT_IN;
	    break;
	case BATTERY_HEALTH_TEMPERATURE_NORMAL_HIGH:
        if (battery_voltage < BQ24161_PRECONDITIONING_BATVOLT_THRESHOLD) {
	        /*enable low charge,100mA charging*/
            di->enable_low_chg = ENABLE_LOW_CHG;
        } else {
            di->enable_low_chg = DISABLE_LOW_CHG;
        }
        k3_bq24161_config_safety_reg(di);
		di->enable_ce = ENABLE_CE;
        break;
	default:
			break;
	}
    di->enable_ce = (di->enable_ce | di->factory_flag);
    k3_bq24161_config_control_reg(di);
    k3_bq24161_charge_status(di);
}


/*===========================================================================
  Function:       k3_bq24161_open_inner_fet
  Description:    VPH_PWR connected to VBATT begin
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
void k3_bq24161_open_inner_fet(struct k3_bq24161_device_info *di)
{
	u8 en_nobatop = 0;

	k3_bq24161_read_byte(di, &en_nobatop, REG_BATTERY_AND_SUPPLY_STATUS);

	if (di->battery_present) {
		di->enable_iterm = ENABLE_ITERM;
		en_nobatop = en_nobatop & (~EN_NOBATOP);
	} else {
		di->enable_iterm = DISABLE_ITERM;
		en_nobatop = en_nobatop | EN_NOBATOP;
	}

	k3_bq24161_config_control_reg(di);
	k3_bq24161_write_byte(di, en_nobatop, REG_BATTERY_AND_SUPPLY_STATUS);
}

/*===========================================================================
  Function:       k3_bq24161_start_500mA_charger
  Description:    start 500mA charge
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void k3_bq24161_start_500mA_charger(struct k3_bq24161_device_info *di)
{
	long int  events = BQ24161_START_USB_CHARGING;

#if BQ2416X_USE_WAKE_LOCK
	/* hold system to enter into suspend when charging */
	wake_lock(&di->charger_wake_lock);
#endif

	/*if the charger is NON_STANDARD AC, the charge parameters are same of USB, only the
	type is AC*/
	//if (CHARGER_TYPE_NON_STANDARD == di->event)
	//	events = BQ24161_START_AC_CHARGING;

	/*set gpio_074 low level for CD pin to enable bq24161 IC*/
	gpio_set_value(di->gpio, 0);

	blocking_notifier_call_chain(&notifier_list_bat, events, NULL);

    /*enable charger*/
	di->enable_ce = ENABLE_CE;
    /*enable charge current termination*/
	di->enable_iterm = ENABLE_ITERM;

    di->calling_limit = 0;
	di->factory_flag = 0;
	di->hz_mode = 0;

	di->charger_source = POWER_SUPPLY_TYPE_USB;
	di->charge_status = POWER_SUPPLY_STATUS_CHARGING;

	di->cin_limit = CURRENT_USB_LIMIT_IN;
	di->currentmA = CURRENT_USB_CHARGE_IN;
    di->dppm_voltagemV = VOLT_DPPM_ADJUST_USB;

	k3_bq24161_config_control_reg(di);
	k3_bq24161_config_voltage_reg(di);
	k3_bq24161_config_current_reg(di);
	k3_bq24161_config_dppm_voltage_reg(di, di->dppm_voltagemV);

	schedule_delayed_work(&di->bq24161_charger_work, msecs_to_jiffies(0));

	dev_info(di->dev, "%s, ---->START CHARGING, \n"
				"battery current = %d mA\n"
				"battery voltage = %d mV\n"
				, __func__, di->currentmA, di->voltagemV);

    di->battery_present = is_k3_bq27510_battery_exist(g_battery_measure_by_bq27510_device);
	if (!di->battery_present) {
		dev_dbg(di->dev, "BATTERY NOT DETECTED!\n");

		events = BQ24161_NOT_CHARGING;
		blocking_notifier_call_chain(&notifier_list_bat, events, NULL);

		di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	}
}

/*===========================================================================
  Function:       k3_bq24161_start_BCUSB_charger
  Description:    start BCUSB charge
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void k3_bq24161_start_BCUSB_charger(struct k3_bq24161_device_info *di)
{
	long int  events = BQ24161_START_USB_CHARGING;

#if BQ2416X_USE_WAKE_LOCK
	/* hold system to enter into suspend when charging */
	wake_lock(&di->charger_wake_lock);
#endif

	/*set gpio_074 low level for CD pin to enable bq24161 IC*/
	gpio_set_value(di->gpio, 0);

	blocking_notifier_call_chain(&notifier_list_bat, events, NULL);

	/*enable charger*/
	di->enable_ce = ENABLE_CE;
	/*enable charge current termination*/
	di->enable_iterm = ENABLE_ITERM;

    di->calling_limit = 0;
	di->factory_flag = 0;
	di->hz_mode = 0;

	di->charger_source = POWER_SUPPLY_TYPE_MAINS;
	di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
	di->cin_limit = CURRENT_AC_LIMIT_IN;
	di->currentmA = di->max_currentmA ;
    di->dppm_voltagemV = VOLT_DPPM_ADJUST;

	k3_bq24161_config_control_reg(di);
	k3_bq24161_config_voltage_reg(di);
	k3_bq24161_config_current_reg(di);
	k3_bq24161_config_dppm_voltage_reg(di, di->dppm_voltagemV);

	schedule_delayed_work(&di->bq24161_charger_work, msecs_to_jiffies(0));

	dev_info(di->dev, "%s, ---->START CHARGING, \n"
				"battery current = %d mA\n"
				"battery voltage = %d mV\n"
				, __func__, di->currentmA, di->voltagemV);

    di->battery_present = is_k3_bq27510_battery_exist(g_battery_measure_by_bq27510_device);
	if (!di->battery_present) {
		dev_warn(di->dev, "BATTERY NOT DETECTED!\n");
		events = BQ24161_NOT_CHARGING;
		blocking_notifier_call_chain(&notifier_list_bat, events, NULL);

		di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	}
}

/*===========================================================================
  Function:       k3_bq24161_start_ac_charger
  Description:    start AC charge
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void k3_bq24161_start_ac_charger(struct k3_bq24161_device_info *di)
{
	long int  events = BQ24161_START_AC_CHARGING;

#if BQ2416X_USE_WAKE_LOCK
	/* hold system to enter into suspend when charging */
	wake_lock(&di->charger_wake_lock);
#endif

	/*set gpio_074 low level for CD pin to enable bq24161 IC*/
	gpio_set_value(di->gpio, 0);

	blocking_notifier_call_chain(&notifier_list_bat, events, NULL);

	/*enable charger*/
	di->enable_ce = ENABLE_CE;

	/*enable charge current termination*/
	di->enable_iterm = ENABLE_ITERM;
    di->calling_limit = 0;
	di->factory_flag = 0;
	di->hz_mode = 0;

	di->charger_source = POWER_SUPPLY_TYPE_MAINS;
	di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
	di->cin_limit = CURRENT_AC_LIMIT_IN;
	di->currentmA = di->max_currentmA ;
    di->dppm_voltagemV = VOLT_DPPM_ADJUST;

	k3_bq24161_config_control_reg(di);
	k3_bq24161_config_voltage_reg(di);
	k3_bq24161_config_current_reg(di);
	k3_bq24161_config_dppm_voltage_reg(di, di->dppm_voltagemV);

	schedule_delayed_work(&di->bq24161_charger_work, msecs_to_jiffies(0));

	dev_info(di->dev, "%s, ---->START CHARGING, \n"
				"battery current = %d mA\n"
				"battery voltage = %d mV\n"
				, __func__, di->currentmA, di->voltagemV);

    di->battery_present = is_k3_bq27510_battery_exist(g_battery_measure_by_bq27510_device);
	if (!di->battery_present) {
		dev_warn(di->dev, "BATTERY NOT DETECTED!\n");
		events = BQ24161_NOT_CHARGING;
		blocking_notifier_call_chain(&notifier_list_bat, events, NULL);

		di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	}
}

/*===========================================================================
  Function:       k3_bq24161_stop_charger
  Description:    STOP charge
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void k3_bq24161_stop_charger(struct k3_bq24161_device_info *di)
{
	long int  events  = BQ24161_STOP_CHARGING;

	dev_info(di->dev, "%s,---->STOP CHARGING\n", __func__);
        gpio_set_value(di->gpio, 1);

	di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
	di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
    di->calling_limit = 0;
	di->factory_flag = 0;
	di->hz_mode = 0;

	cancel_delayed_work_sync(&di->bq24161_charger_work);

	blocking_notifier_call_chain(&notifier_list_bat, events, NULL);

	/*set gpio_074 high level for CD pin to disable bq24161 IC */
	gpio_set_value(di->gpio, 1);

        msleep(1000);
#if BQ2416X_USE_WAKE_LOCK
	wake_unlock(&di->charger_wake_lock);
#endif

}

/*===========================================================================
  Function:       k3_bq24161_charger_update_status
  Description:    updata charger status
  Input:          struct k3_bq24161_device_info *di
  Return:         NULL
===========================================================================*/
static void
k3_bq24161_charger_update_status(struct k3_bq24161_device_info *di)
{
	u8 read_reg[8] = {0};

	timer_fault = 0;

	k3_bq24161_read_block(di, &read_reg[0], 0, 8);

	if ((read_reg[0] & BQ24161_VUSB_FAULT) == 0x50)
		dev_dbg(di->dev, "CHARGE DONE\n");

	if ((read_reg[0] & 0x7) == 0x4)
		timer_fault = 1;

	if (read_reg[0] & 0x7) {
		di->cfg_params = 1;
		dev_err(di->dev, "CHARGER STATUS %x\n", read_reg[0]);
	}

	if ((timer_fault == 1) || (di->cfg_params == 1)) {
		k3_bq24161_write_byte(di, di->control_reg, REG_CONTROL_REGISTER);
		k3_bq24161_write_byte(di, di->voltage_reg, REG_BATTERY_VOLTAGE);
		k3_bq24161_write_byte(di, di->current_reg, REG_BATTERY_CURRENT);
		k3_bq24161_write_byte(di, di->dppm_reg, REG_DPPM_VOLTAGE);
		k3_bq24161_config_safety_reg(di);
		di->cfg_params = 0;
	}

	return;
}

/*===========================================================================
  Function:       k3_bq24161_charger_work
  Description:    start charge work
  Input:          struct work_struct *work
  Return:         NULL
===========================================================================*/
static void k3_bq24161_charger_work(struct work_struct *work)
{
	struct k3_bq24161_device_info *di = container_of(work,
		struct k3_bq24161_device_info, bq24161_charger_work.work);

	di->battery_present = is_k3_bq27510_battery_exist(g_battery_measure_by_bq27510_device);

	k3_bq24161_open_inner_fet(di);

	k3_bq24161_low_current_charge(di);

	k3_bq24161_charger_update_status(di);

	/* reset 32 second timer */
	k3_bq24161_config_status_reg(di);

	/* arrange next 30 seconds charger work */
	schedule_delayed_work(&di->bq24161_charger_work, msecs_to_jiffies(25000));
}


/* for /sys interface */
static ssize_t k3_bq24161_set_enable_itermination(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int status = count;
	long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
		return -EINVAL;
	di->enable_iterm = val;
	k3_bq24161_config_control_reg(di);

	return status;
}

/* for /sys interface */
static ssize_t k3_bq24161_show_enable_itermination(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	unsigned long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	val = di->enable_iterm;
	return sprintf(buf, "%lu\n", val);
}

/* for /sys interface */
static ssize_t k3_bq24161_set_cin_limit(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int status = count;
	long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	if ((strict_strtol(buf, 10, &val) < 0) || (val < 100)
					|| (val > di->max_currentmA))
		return -EINVAL;
	di->cin_limit = val;
	k3_bq24161_config_control_reg(di);

	return status;
}

/* for /sys interface */
static ssize_t k3_bq24161_show_cin_limit(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	unsigned long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	val = di->cin_limit;
	return sprintf(buf, "%lu\n", val);
}

/* for /sys interface */
static ssize_t k3_bq24161_set_regulation_voltage(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int status = count;
	long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	if ((strict_strtol(buf, 10, &val) < 0) || (val < LOW_VOL)
					|| (val > di->max_voltagemV))
		return -EINVAL;
	di->voltagemV = val;
	k3_bq24161_config_voltage_reg(di);

	return status;
}

/* for /sys interface */
static ssize_t k3_bq24161_show_regulation_voltage(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	unsigned long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	val = di->voltagemV;
	return sprintf(buf, "%lu\n", val);
}

/* for /sys interface */
static ssize_t k3_bq24161_set_charge_current(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int status = count;
	long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	if ((strict_strtol(buf, 10, &val) < 0) || (val < LOW_CURRENT)
					|| (val > di->max_currentmA))
		return -EINVAL;
	di->currentmA = val;
	k3_bq24161_config_current_reg(di);

	return status;
}

/* for /sys interface */
static ssize_t k3_bq24161_show_charge_current(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	unsigned long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	val = di->currentmA;
	return sprintf(buf, "%lu\n", val);
}

/* for /sys interface */
static ssize_t k3_bq24161_set_termination_current(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int status = count;
	long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > HIGH_TERM_CURRENT))
		return -EINVAL;
	di->term_currentmA = val;
	k3_bq24161_config_current_reg(di);

	return status;
}

/* for /sys interface */
static ssize_t k3_bq24161_show_termination_current(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	unsigned long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	val = di->term_currentmA;
	return sprintf(buf, "%lu\n", val);
}

/* for /sys interface */
static ssize_t k3_bq24161_set_dppm_voltage(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int status = count;
	long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > HIGH_DPPM_VOLTAGE))
		return -EINVAL;
	di->dppm_voltagemV = val;
	k3_bq24161_config_dppm_voltage_reg(di, di->dppm_voltagemV);

	return status;
}

/* for /sys interface */
static ssize_t k3_bq24161_show_dppm_voltage(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	unsigned long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	val = di->dppm_voltagemV;
	return sprintf(buf, "%lu\n", val);
}

/*
 * for /sys interface
 * force to enable/disable charger
 * set 1 --- enable_charger; 0 --- disable charger
 */
static ssize_t k3_bq24161_set_enable_charger(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int status = count;
	long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);
	if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
		return -EINVAL;
	di->enable_ce = val ^ 0x1;
	k3_bq24161_config_control_reg(di);
	di->factory_flag = di->enable_ce;
	k3_bq24161_charge_status(di);

	return status;
}

/* for /sys interface */
static ssize_t k3_bq24161_show_enable_charger(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	unsigned long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	val = di->enable_ce ^ 0x1;
	return sprintf(buf, "%lu\n", val);
}

/* for /sys interface
 * set 1 --- hz_mode ; 0 --- not hz_mode
 */
static ssize_t k3_bq24161_set_enable_hz_mode(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int status = count;
	long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
		return -EINVAL;
	di->hz_mode = val;
	k3_bq24161_config_control_reg(di);

	return status;
}

/* for /sys interface */
static ssize_t k3_bq24161_show_enable_hz_mode(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	unsigned long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	val = di->hz_mode;
	return sprintf(buf, "%lu\n", val);
}

/* for /sys interface
 * set 1 --- enable bq24161 IC; 0 --- disable bq24161 IC
 */
static ssize_t k3_bq24161_set_enable_cd(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int status = count;
	long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
		return -EINVAL;
	di->cd_active = val ^ 0x1;
	gpio_set_value(di->gpio, di->cd_active);
	return status;
}
static ssize_t k3_bq24161_show_enable_cd(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	unsigned long val = 0;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	val = di->cd_active ^ 0x1;
	return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2416x_show_chargelog(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
        int i = 0;
        u8   read_reg[8] = {0};
        u8   buf_temp[20] = {0};
        struct k3_bq24161_device_info *di = dev_get_drvdata(dev);
        k3_bq24161_read_block(di, &read_reg[0], 0, 8);
        for(i=0;i<8;i++)
        {
            sprintf(buf_temp,"0x%-8.2x",read_reg[i]);
            strcat(buf,buf_temp);
        }
        strcat(buf,"\n");
	return strlen(buf);
}

/* for /sys interface
 * virtual charger plugout
 */

static ssize_t k3_battery_set_usb_plug_out(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int status = count;
	long val = -1;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	if (!di)
		return -EINVAL;

	if ((strict_strtol(buf, 1, &val) < 0) || (val < 0))
		return -EINVAL;

	if ((0 == val) &&
		 (POWER_SUPPLY_STATUS_DISCHARGING != di->charge_status))
		k3_bq24161_stop_charger(di);

	return status;
}

static ssize_t k3_battery_set_usb_plug_in(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int status = count;
	long val = -1;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	if (!di)
		return 0;

	if ((strict_strtol(buf, 1, &val) < 0) || (val < 0))
		return -EINVAL;

	if ((0 == val) &&
		 (POWER_SUPPLY_STATUS_DISCHARGING == di->charge_status))
		k3_bq24161_start_500mA_charger(di);

	return status;
}

static ssize_t k3_bq24161_set_calling_limit(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	long val;
	int status = count;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);
	if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
		return -EINVAL;

	di->calling_limit = val;
	if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
		if(di->calling_limit){
			di->cin_limit = CIN_LIMIT_800;
			k3_bq24161_config_control_reg(di);
			dev_info(di->dev,"calling_limit_current = %d\n", di->cin_limit);
		}
	}
	else{
		di->calling_limit = 0;
	}

	return status;
}

static ssize_t k3_bq24161_show_calling_limit(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	unsigned long val;
	struct k3_bq24161_device_info *di = dev_get_drvdata(dev);

	val = di->calling_limit;

	return sprintf(buf, "%lu\n", val);
}


static DEVICE_ATTR(enable_cd, S_IWUSR | S_IRUGO,
				k3_bq24161_show_enable_cd,
				k3_bq24161_set_enable_cd);

static DEVICE_ATTR(enable_itermination, S_IWUSR | S_IRUGO,
				k3_bq24161_show_enable_itermination,
				k3_bq24161_set_enable_itermination);

static DEVICE_ATTR(cin_limit, S_IWUSR | S_IRUGO,
				k3_bq24161_show_cin_limit,
				k3_bq24161_set_cin_limit);

static DEVICE_ATTR(regulation_voltage, S_IWUSR | S_IRUGO,
				k3_bq24161_show_regulation_voltage,
				k3_bq24161_set_regulation_voltage);

static DEVICE_ATTR(charge_current, S_IWUSR | S_IRUGO,
				k3_bq24161_show_charge_current,
				k3_bq24161_set_charge_current);

static DEVICE_ATTR(termination_current, S_IWUSR | S_IRUGO,
				k3_bq24161_show_termination_current,
				k3_bq24161_set_termination_current);

static DEVICE_ATTR(enable_charger, S_IWUSR | S_IRUGO,
				k3_bq24161_show_enable_charger,
				k3_bq24161_set_enable_charger);

static DEVICE_ATTR(enable_hz_mode, S_IWUSR | S_IRUGO,
				k3_bq24161_show_enable_hz_mode,
				k3_bq24161_set_enable_hz_mode);

static DEVICE_ATTR(dppm_voltage, S_IWUSR | S_IRUGO,
				k3_bq24161_show_dppm_voltage,
				k3_bq24161_set_dppm_voltage);

static DEVICE_ATTR(chargelog, S_IWUSR | S_IRUGO,
				bq2416x_show_chargelog,
				NULL);

static DEVICE_ATTR(usb_plug_out, S_IWUSR | S_IRUGO,
				NULL,
				k3_battery_set_usb_plug_out);
static DEVICE_ATTR(usb_plug_in, S_IWUSR | S_IRUGO,
				NULL,
				k3_battery_set_usb_plug_in);

static DEVICE_ATTR(calling_limit, S_IWUSR | S_IRUGO,
				k3_bq24161_show_calling_limit,
				k3_bq24161_set_calling_limit);

static struct attribute *k3_bq24161_attributes[] = {
	&dev_attr_enable_itermination.attr,
	&dev_attr_cin_limit.attr,
	&dev_attr_regulation_voltage.attr,
	&dev_attr_charge_current.attr,
	&dev_attr_termination_current.attr,
	&dev_attr_dppm_voltage.attr,
	&dev_attr_enable_charger.attr,
	&dev_attr_enable_hz_mode.attr,
	&dev_attr_enable_cd.attr,
	&dev_attr_chargelog.attr,
	&dev_attr_usb_plug_out.attr,
	&dev_attr_usb_plug_in.attr,
	&dev_attr_calling_limit.attr,
	NULL,
};

static const struct attribute_group k3_bq24161_attr_group = {
	.attrs = k3_bq24161_attributes,
};

static void k3_bq24161_usb_charger_work(struct work_struct *work)
{
	struct k3_bq24161_device_info *di =
		container_of(work, struct k3_bq24161_device_info, usb_work);


	switch (di->event) {
	case CHARGER_TYPE_USB:
		dev_info(di->dev, "case = CHARGER_TYPE_USB -> \n");
		k3_bq24161_start_500mA_charger(di);
		break;
	case CHARGER_TYPE_NON_STANDARD:
		dev_info(di->dev, "case = CHARGER_TYPE_NON_STANDARD -> \n");
		k3_bq24161_start_500mA_charger(di);
		break;
	case CHARGER_TYPE_BC_USB:
		dev_info(di->dev, "case = CHARGER_TYPE_BC_USB -> \n");
		k3_bq24161_start_BCUSB_charger(di);
		break;
	case CHARGER_TYPE_STANDARD:
		dev_info(di->dev, "case = CHARGER_TYPE_STANDARD\n");
		k3_bq24161_start_ac_charger(di);
		break;
	case CHARGER_REMOVED:
		dev_info(di->dev, "case = USB_EVENT_NONE\n");
		k3_bq24161_stop_charger(di);
		break;
	default:
		dev_err(di->dev, "case = default\n");
		break;
	}

}

static int k3_bq24161_usb_notifier_call(struct notifier_block *nb,
		unsigned long event, void *data)
{
	struct k3_bq24161_device_info *di =
		container_of(nb, struct k3_bq24161_device_info, nb);

	di->event = event;

	schedule_work(&di->usb_work);
	return NOTIFY_OK;
}
static int bq2416x_get_max_charge_voltage(struct k3_bq24161_device_info *di)
{
	bool ret = 0;

	ret = get_hw_config_int("gas_gauge/charge_voltage", &di->max_voltagemV , NULL);
	if (ret) {
		if (di->max_voltagemV < 4200) {
			di->max_voltagemV = 4200;
		}
		return true;
    }
    else{
        dev_err(di->dev, " bq2416x_get_max_charge_voltage from boardid fail \n");
        return false;
    }
}

static int bq2416x_get_max_charge_current(struct k3_bq24161_device_info *di)
{
	bool ret = 0;
	ret = get_hw_config_int("gas_gauge/charge_current", &di->max_currentmA , NULL);
	if (ret) {
		if(di->max_currentmA < 1000) {
			di->max_currentmA = 1000;
		}
		return true;
	}
	else {
		dev_err(di->dev, " bq2416x_get_max_charge_current from boardid fail \n");
		return false;
	}
}
static int __devinit k3_bq24161_charger_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	u8 read_reg = 0;
	int ret = 0;
	enum usb_charger_type plugin_stat = CHARGER_REMOVED;
	struct k3_bq24161_device_info *di = NULL;
	struct k3_bq24161_platform_data *pdata = NULL;
	/*enum plugin_status plugin_stat;*/

	pdata = client->dev.platform_data;
	if (!pdata) {
		dev_err(&client->dev, "pdata is NULL!\n");
		return -ENOMEM;
	}

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(&client->dev, "di is NULL!\n");
		return -ENOMEM;
	}

	di->dev = &client->dev;
	di->client = client;

	i2c_set_clientdata(client, di);

	ret = k3_bq24161_read_byte(di, &read_reg, REG_PART_REVISION);

	if (ret < 0) {
		dev_err(&client->dev, "chip not present at address %x\n",
								client->addr);
		ret = -EINVAL;
		goto err_kfree;
	}

	if (((read_reg & BQ24161_VERSION_MSK) == 0x00)
		&& (client->addr == I2C_ADDR_BQ24161))
		di->bqchip_version = BQ24161;

	if (di->bqchip_version == 0) {
		dev_err(&client->dev, "unknown bq chip\n");
		dev_err(&client->dev, "Chip address %x", client->addr);
		dev_err(&client->dev, "bq chip version reg value %x", read_reg);
		ret = -EINVAL;
		goto err_kfree;
	}
    ret = bq2416x_get_max_charge_voltage(di);
    if(!ret){
	    di->max_voltagemV = pdata->max_charger_voltagemV;
	}

    ret = bq2416x_get_max_charge_current(di);
    if(!ret){
	    di->max_currentmA = pdata->max_charger_currentmA;
	}

	di->voltagemV = di->max_voltagemV;
	di->currentmA = CURRENT_USB_CHARGE_IN ;
	di->term_currentmA = CURRENT_TERM_CHARGE_IN;
	di->dppm_voltagemV = VOLT_DPPM_ADJUST;
	di->cin_limit = CURRENT_USB_LIMIT_IN;
	di->gpio = pdata->gpio;

	 /* Set iomux normal */
#ifdef CONFIG_GPIO_BAT
	if (!di->piomux_block)
		di->piomux_block  = iomux_get_block("block_charger");

	if (!di->pblock_config)
		di->pblock_config = iomux_get_blockconfig("block_charger");

	ret = blockmux_set(di->piomux_block, di->pblock_config, NORMAL);
	if (ret) {
		dev_err(&client->dev, "blockmux_set NORMAL failed, ret=%d\n", ret);
		goto err_kfree;
	}
#endif

    /*set gpio_074 to control CD pin to disable/enable bq24161 IC*/
	ret = gpio_request(di->gpio, "gpio_074_cd");
	if (ret) {
		dev_err(&client->dev, "could not request irq\n");
		ret = -ENOMEM;
		goto err_io;
	}

	 /* set charger CD pin to low level and enable it to supply power normally*/
	gpio_direction_output(di->gpio, 0);

	di->enable_low_chg = DISABLE_LOW_CHG;

	/*enable charge current termination*/
	di->enable_iterm = ENABLE_ITERM;

	di->factory_flag = 0;

	di->enable_ce = ENABLE_CE;
	di->hz_mode = 0;
	di->cd_active = 0;

#if BQ2416X_USE_WAKE_LOCK
	wake_lock_init(&di->charger_wake_lock, WAKE_LOCK_SUSPEND, "charger_wake_lock");
#endif

	INIT_DELAYED_WORK(&di->bq24161_charger_work,
				k3_bq24161_charger_work);

	INIT_WORK(&di->usb_work, k3_bq24161_usb_charger_work);

	di->active = 0;
	di->params.enable = 1;
	di->cfg_params = 1;
	/*di->enable_iterm = 1;*/

	k3_bq24161_config_control_reg(di);
	k3_bq24161_config_voltage_reg(di);
	k3_bq24161_config_current_reg(di);
	k3_bq24161_config_dppm_voltage_reg(di, di->dppm_voltagemV);
	k3_bq24161_config_safety_reg(di);

	ret = sysfs_create_group(&client->dev.kobj, &k3_bq24161_attr_group);
	if (ret) {
		dev_err(&client->dev, "could not create sysfs files\n");
		goto err_gpio;
	}

	di->nb.notifier_call = k3_bq24161_usb_notifier_call;

	ret = hiusb_charger_registe_notifier(&di->nb);
	if (ret < 0) {
		dev_err(&client->dev, "hiusb_charger_registe_notifier failed\n");
		goto err_sysfs;
	}

	plugin_stat = get_charger_name();
	if ((CHARGER_TYPE_USB == plugin_stat) || (CHARGER_TYPE_NON_STANDARD == plugin_stat)) {
		di->event = plugin_stat;
	} else if (CHARGER_TYPE_BC_USB == plugin_stat) {
		di->event = CHARGER_TYPE_BC_USB;
	} else if (CHARGER_TYPE_STANDARD == plugin_stat) {
		di->event = CHARGER_TYPE_STANDARD;
	} else {
		di->event = CHARGER_REMOVED;
	}
    schedule_work(&di->usb_work);

	return 0;

err_sysfs:
	sysfs_remove_group(&client->dev.kobj, &k3_bq24161_attr_group);
err_gpio:
	gpio_free(di->gpio);
err_io:
#ifdef CONFIG_GPIO_BAT
	if (blockmux_set(di->piomux_block, di->pblock_config, LOWPOWER))
		dev_err(&client->dev, "blockmux_set LOWPOWER failed\n");
#endif

err_kfree:
	#if BQ2416X_USE_WAKE_LOCK
	wake_lock_destroy(&di->charger_wake_lock);
	#endif
	kfree(di);
	di = NULL;

	return ret;
}

static int __devexit k3_bq24161_charger_remove(struct i2c_client *client)
{
	struct k3_bq24161_device_info *di = i2c_get_clientdata(client);

	hiusb_charger_unregiste_notifier(&di->nb);

	sysfs_remove_group(&client->dev.kobj, &k3_bq24161_attr_group);

	cancel_delayed_work(&di->bq24161_charger_work);
	flush_scheduled_work();

#if BQ2416X_USE_WAKE_LOCK
	wake_lock_destroy(&di->charger_wake_lock);
#endif

	gpio_free(di->gpio);

#ifdef CONFIG_GPIO_BAT
	if (blockmux_set(di->piomux_block, di->pblock_config, LOWPOWER))
		dev_err(&client->dev, "blockmux_set LOWPOWER failed\n");
	/*blockmux_set(di->piomux_block_interrupt, di->pblock_config_interrupt, LOWPOWER);*/
#endif

	kfree(di);

	return 0;
}

static void k3_bq24161_charger_shutdown(struct i2c_client *client)
{
	struct k3_bq24161_device_info *di = i2c_get_clientdata(client);

	cancel_delayed_work(&di->bq24161_charger_work);
	flush_scheduled_work();

#ifdef CONFIG_GPIO_BAT
	if (blockmux_set(di->piomux_block, di->pblock_config, LOWPOWER))
		dev_err(&client->dev, "blockmux_set LOWPOWER failed\n");
#endif

	return;
}

static const struct i2c_device_id bq24161_id[] = {
	{ "k3_bq24161_charger", 0 },
	{},
};

#ifdef CONFIG_PM
static int k3_bq24161_charger_suspend(struct i2c_client *client,
	pm_message_t state)
{
	return 0;
}

static int k3_bq24161_charger_resume(struct i2c_client *client)
{
	return 0;
}
#else
#define k3_bq24161_charger_suspend	NULL
#define k3_bq24161_charger_resume	NULL
#endif

MODULE_DEVICE_TABLE(i2c, bq24161);

static struct i2c_driver k3_bq24161_charger_driver = {
	.probe		= k3_bq24161_charger_probe,
	.remove		= __devexit_p(k3_bq24161_charger_remove),
	.suspend	= k3_bq24161_charger_suspend,
	.resume		= k3_bq24161_charger_resume,
	.shutdown	= k3_bq24161_charger_shutdown,
	.id_table		= bq24161_id,

	.driver		= {
		.owner = THIS_MODULE,
		.name	= "k3_bq24161_charger",
	},
};

static int __init k3_bq24161_charger_init(void)
{
	return i2c_add_driver(&k3_bq24161_charger_driver);
}
module_init(k3_bq24161_charger_init);

static void __exit k3_bq24161_charger_exit(void)
{
	i2c_del_driver(&k3_bq24161_charger_driver);
}
module_exit(k3_bq24161_charger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HUAWEI Inc");
