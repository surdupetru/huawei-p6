/*
 * drivers/power/bq2419x_charger.c
 *
 * BQ24190/1/2/3/4 charging driver
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

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
#include <linux/power/bq2419x_charger.h>
#include <linux/power/bq27510_battery.h>
#include <linux/power/bq_bci_battery.h>
#include <mach/gpio.h>
#include <mach/irqs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/mux.h>
#include <linux/usb/hiusb_android.h>
#include <linux/mutex.h>
#include <hsad/config_mgr.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

struct charge_params {
    unsigned long       currentmA;
    unsigned long       voltagemV;
    unsigned long       term_currentmA;
    unsigned long       enable_iterm;
    bool                enable;
};

struct bq2419x_device_info {
    struct device        *dev;
    struct i2c_client    *client;
    struct charge_params  params;
    struct delayed_work   bq2419x_charger_work;
    struct delayed_work   bq2419x_usb_otg_work;
    struct work_struct    usb_work;

    unsigned int      wakelock_enabled;
    unsigned short    input_source_reg00;
    unsigned short    power_on_config_reg01;
    unsigned short    charge_current_reg02;
    unsigned short    prechrg_term_current_reg03;
    unsigned short    charge_voltage_reg04;
    unsigned short    term_timer_reg05;
    unsigned short    thermal_regulation_reg06;
    unsigned short    misc_operation_reg07;
    unsigned short    system_status_reg08;
    unsigned short    charger_fault_reg09;
    unsigned short    bqchip_version;

    unsigned int      max_currentmA;
    unsigned int      max_voltagemV;
    unsigned int      max_cin_currentmA;

    unsigned int    cin_dpmmV;
    unsigned int    cin_limit;
    unsigned int    chrg_config;
    unsigned int    sys_minmV;
    unsigned int    currentmA;
    unsigned int    prechrg_currentmA;
    unsigned int    term_currentmA;
    unsigned int    voltagemV;
    unsigned int    watchdog_timer;
    unsigned int    chrg_timer;
    unsigned int    bat_compohm;
    unsigned int    comp_vclampmV;

    bool    hz_mode;
    bool    boost_lim;
    bool    enable_low_chg;
    bool    cfg_params;
    bool    enable_iterm;
    bool    enable_timer;
    bool    enable_batfet;
    bool    cd_active;
    bool    factory_flag;
    bool    hot_enable_charge_flag;
    bool    calling_limit;
    bool    battery_present;
    bool    enable_dpdm;
    int     charge_status;
    int     charger_source;
    int     timer_fault;
    unsigned int    battery_temp_status;
    unsigned long           event;
    struct notifier_block   nb;

    int                      gpio;
    struct iomux_block       *piomux_block;
    struct block_config      *pblock_config;
    int     battery_voltage;
    int     temperature_cold;
    int     temperature_cool;
    int     temperature_warm;
    int     temperature_hot;
    bool    coldhot_charging_flag;
    bool    limit_charging_flag;
    bool    hot_limit_charging_flag;
    bool    battery_full;
    int     temperature_5;
    int     temperature_10;
    u32    charge_full_count;

    /* these parameters are for charging between 0C-5C & 5C-10C, 1-0.1*capacity...
       charge_in_temp_5 means the parameter for charging between 0C-5C. */
    unsigned int design_capacity;
    unsigned int charge_in_temp_5;
    unsigned int charge_in_temp_10;

    int  japan_charger;
    int  is_two_stage_charger;
    int  two_stage_charger_status;
    int  first_stage_voltage;
    int  second_stage_voltage;
    int  is_disable_cool_temperature_charger;
    int  high_temp_para;
};

/*extern a notifier list for charging notification*/
extern struct blocking_notifier_head notifier_list;

extern struct bq27510_device_info *g_battery_measure_by_bq27510_device;

static struct wake_lock chrg_lock;
extern u32 wakeup_timer_seconds;

/* backup the last cin_limit */
static unsigned int pre_cin_limit;

enum{
    BATTERY_HEALTH_TEMPERATURE_NORMAL = 0,
    BATTERY_HEALTH_TEMPERATURE_OVERLOW,
    BATTERY_HEALTH_TEMPERATURE_LOW,
    BATTERY_HEALTH_TEMPERATURE_NORMAL_HIGH,
    BATTERY_HEALTH_TEMPERATURE_HIGH,
    BATTERY_HEALTH_TEMPERATURE_OVERHIGH,
    BATTERY_HEALTH_TEMPERATURE_10,
    BATTERY_HEALTH_TEMPERATURE_5,
    BATTERY_HEALTH_TEMPERATURE_HIGH_CP1,
    BATTERY_HEALTH_TEMPERATURE_HIGH_CP2,
    BATTERY_HEALTH_TEMPERATURE_HIGH_CP3,
    BATTERY_HEALTH_TEMPERATURE_HIGH_CP4,
    BATTERY_HEALTH_TEMPERATURE_HIGH_CP5,
};
enum{
    NORMAL_TEMP_CONFIG = 0,  // (BATTERY_HEALTH_TEMPERATURE_NORMAL)
    NORMAL_HIGH_TEMP_CONFIG, // (BATTERY_HEALTH_TEMPERATURE_NORMAL_HIGH)
    HIGH_TEMP_CONFIG,        // (BATTERY_HEALTH_TEMPERATURE_HIGH)
    TEMP_CONFIG_10,
    TEMP_CONFIG_5,
    NORMAL_SECOND_STAGE,//for two charging stage
    NORMAL_HIGH_TEMP_CONFIG_CP1,
    NORMAL_HIGH_TEMP_CONFIG_CP2,
    NORMAL_HIGH_TEMP_CONFIG_CP3,
    NORMAL_HIGH_TEMP_CONFIG_CP4,
};

//tempture checkpoint
struct bq2419x_high_temp_cp{
    int cp1;
    int cp2;
    int cp3;
    int cp4;
    int cp5;
    int cp6;
};

enum{
    HIGH_TEMP_CP_U9701L = 0,
    HIGH_TEMP_CP_U9700L = 1,
};

struct bq2419x_high_temp_cp japan_temp_cp[] ={
    {40, 42, 43, 45, 53, 55}, //HIGH_TEMP_CP_U9701L
    {37, 39, 40, 42, 43, 45},//HIGH_TEMP_CP_U9700L
    {39, 41, 42, 45, 53, 55},//HIGH_TEMP_CP_U9700GVC China area
    {0},
};

static void bq2419x_get_boardid_japan_charge_parameter(struct bq2419x_device_info *di)
{
    bool ret = 0;

    ret = get_hw_config_int("charger/japan_charger", &di->japan_charger, NULL);
    ret&= get_hw_config_int("charger/is_two_stage_charger", &di->is_two_stage_charger, NULL);
    ret&= get_hw_config_int("charger/first_stage_voltage", &di->first_stage_voltage, NULL);
    ret&= get_hw_config_int("charger/second_stage_voltage", &di->second_stage_voltage, NULL);
    ret&= get_hw_config_int("charger/is_disable_cool_temperature_charger", &di->is_disable_cool_temperature_charger, NULL);
    ret&= get_hw_config_int("charger/high_temperature_checkpoint",&di->high_temp_para , NULL);

     if(!ret)
        dev_err(di->dev, " bq2419x get japan charge parameter from boardid fail \n");

     dev_info(di->dev, "%s, \n"
                         "japan_charger = %d \n"
                         "is_two_stage_charger = %d\n"
                         "first_stage_voltage = %d \n"
                         "second_stage_voltage = %d \n"
                         "is_disable_cool_temperature_charger = %d \n"
                         "high_temp_para = %d \n"
                         , __func__, di->japan_charger, di->is_two_stage_charger,di->first_stage_voltage,
                         di->second_stage_voltage, di->is_disable_cool_temperature_charger,di->high_temp_para);
     dev_info(di->dev, "\ncp1 = %d \n"
                         "cp2 = %d \n"
                         "cp3 = %d \n"
                         "cp4 = %d \n"
                         "cp5 = %d \n"
                         "cp6 = %d \n"
                         ,japan_temp_cp[di->high_temp_para].cp1,japan_temp_cp[di->high_temp_para].cp2,
                         japan_temp_cp[di->high_temp_para].cp3,japan_temp_cp[di->high_temp_para].cp4,
                         japan_temp_cp[di->high_temp_para].cp5,japan_temp_cp[di->high_temp_para].cp6);
     return;
}


static int bq2419x_get_boardid_charge_parameter(struct bq2419x_device_info *di)
{
    bool ret = 0;
    int battery_id_status = 0;

    battery_id_status = get_battery_id();

    switch(battery_id_status){
    case BAT_ID_10K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_10k", &di->max_voltagemV, NULL);
         ret&= get_hw_config_int("gas_gauge/charge_current_10k", &di->max_currentmA , NULL);
         ret&= get_hw_config_int("gas_gauge/max_cin_limit_10k", &di->max_cin_currentmA, NULL);
         break;
    case BAT_ID_22K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_22k", &di->max_voltagemV, NULL);
         ret&= get_hw_config_int("gas_gauge/charge_current_22k", &di->max_currentmA , NULL);
         ret&= get_hw_config_int("gas_gauge/max_cin_limit_22k", &di->max_cin_currentmA, NULL);
         break;
    case BAT_ID_39K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_39k", &di->max_voltagemV, NULL);
         ret&= get_hw_config_int("gas_gauge/charge_current_39k", &di->max_currentmA , NULL);
         ret&= get_hw_config_int("gas_gauge/max_cin_limit_39k", &di->max_cin_currentmA, NULL);
         break;
    case BAT_ID_68K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_68k", &di->max_voltagemV, NULL);
         ret&= get_hw_config_int("gas_gauge/charge_current_68k", &di->max_currentmA , NULL);
         ret&= get_hw_config_int("gas_gauge/max_cin_limit_68k", &di->max_cin_currentmA, NULL);
         break;
    case BAT_ID_110K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_110k", &di->max_voltagemV, NULL);
         ret&= get_hw_config_int("gas_gauge/charge_current_110k", &di->max_currentmA , NULL);
         ret&= get_hw_config_int("gas_gauge/max_cin_limit_110k", &di->max_cin_currentmA, NULL);
         break;
    case BAT_ID_200K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_200k", &di->max_voltagemV, NULL);
         ret&= get_hw_config_int("gas_gauge/charge_current_200k", &di->max_currentmA , NULL);
         ret&= get_hw_config_int("gas_gauge/max_cin_limit_200k", &di->max_cin_currentmA, NULL);
         break;
    case BAT_ID_470K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_470k", &di->max_voltagemV, NULL);
         ret&= get_hw_config_int("gas_gauge/charge_current_470k", &di->max_currentmA , NULL);
         ret&= get_hw_config_int("gas_gauge/max_cin_limit_470k", &di->max_cin_currentmA, NULL);
         break;
    case BAT_ID_2M_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_2m", &di->max_voltagemV, NULL);
         ret&= get_hw_config_int("gas_gauge/charge_current_2m", &di->max_currentmA , NULL);
         ret&= get_hw_config_int("gas_gauge/max_cin_limit_2m", &di->max_cin_currentmA, NULL);
         break;
    default:
         ret = get_hw_config_int("gas_gauge/charge_voltage",&di->max_voltagemV, NULL);
         ret&= get_hw_config_int("gas_gauge/charge_current", &di->max_currentmA , NULL);
         ret&=get_hw_config_int("gas_gauge/max_cin_limit", &di->max_cin_currentmA, NULL);
         break;
    }
    if(ret){
        return true;
    }else{
        dev_err(di->dev, " bq2419x get charge parameter from boardid fail \n");
        di->max_voltagemV = 4208;
        di->max_currentmA = 1000;
        di->max_cin_currentmA = 1200;
        return false;
    }
}

static int bq2419x_get_max_charge_voltage(struct bq2419x_device_info *di)
{
    bool ret = 0;
    int battery_id_status = 0;

    battery_id_status = get_battery_id();
    dev_info(di->dev, " battery_id_status = %d\n",battery_id_status);
    switch(battery_id_status){
    case BAT_ID_10K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_10k", &di->max_voltagemV, NULL);
        break;
    case BAT_ID_22K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_22k", &di->max_voltagemV, NULL);
        break;
    case BAT_ID_39K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_39k", &di->max_voltagemV, NULL);
        break;
    case BAT_ID_68K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_68k", &di->max_voltagemV, NULL);
        break;
    case BAT_ID_110K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_110k", &di->max_voltagemV, NULL);
        break;
    case BAT_ID_200K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_200k", &di->max_voltagemV, NULL);
        break;
    case BAT_ID_470K_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_470k", &di->max_voltagemV, NULL);
        break;
    case BAT_ID_2M_STATUS:
         ret = get_hw_config_int("gas_gauge/charge_voltage_2m", &di->max_voltagemV, NULL);
        break;
    default:
         ret = get_hw_config_int("gas_gauge/charge_voltage",&di->max_voltagemV, NULL);
        break;
    }
    if(ret){
        return true;
    }else{
        di->max_voltagemV = 4208;
        dev_err(di->dev, "boardid does not set limited charge voltage\n");
        return false;
    }
}

static int bq2419x_write_block(struct bq2419x_device_info *di, u8 *value,
                               u8 reg, unsigned num_bytes)
{
    struct i2c_msg msg[1];
    int ret = 0;

    *value = reg;

    msg[0].addr = di->client->addr;
    msg[0].flags = 0;
    msg[0].buf = value;
    msg[0].len = num_bytes + 1;

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

static int bq2419x_read_block(struct bq2419x_device_info *di, u8 *value,
                            u8 reg, unsigned num_bytes)
{
    struct i2c_msg msg[2];
    u8 buf = 0;
    int ret = 0;

    buf = reg;

    msg[0].addr = di->client->addr;
    msg[0].flags = 0;
    msg[0].buf = &buf;
    msg[0].len = 1;

    msg[1].addr = di->client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = value;
    msg[1].len = num_bytes;

    ret = i2c_transfer(di->client->adapter, msg, 2);

    /* i2c_transfer returns number of messages transferred */
    if (ret != 2) {
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

static int bq2419x_write_byte(struct bq2419x_device_info *di, u8 value, u8 reg)
{
    /* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */
    u8 temp_buffer[2] = { 0 };

    /* offset 1 contains the data */
    temp_buffer[1] = value;
    return bq2419x_write_block(di, temp_buffer, reg, 1);
}

static int bq2419x_read_byte(struct bq2419x_device_info *di, u8 *value, u8 reg)
{
    return bq2419x_read_block(di, value, reg, 1);
}

static void bq2419x_config_input_source_reg(struct bq2419x_device_info *di)
{
      unsigned int vindpm = 0;
      u8 Vdpm = 0;
      u8 Iin_limit = 0;

      vindpm = di->cin_dpmmV;

      if(vindpm < VINDPM_MIN_3880)
          vindpm = VINDPM_MIN_3880;
      else if (vindpm > VINDPM_MAX_5080)
          vindpm = VINDPM_MAX_5080;

      if (di->cin_limit <= IINLIM_100)
          Iin_limit = 0;
      else if (di->cin_limit > IINLIM_100 && di->cin_limit <= IINLIM_150)
          Iin_limit = 1;
      else if (di->cin_limit > IINLIM_150 && di->cin_limit <= IINLIM_500)
          Iin_limit = 2;
      else if (di->cin_limit > IINLIM_500 && di->cin_limit <= IINLIM_900)
          Iin_limit = 3;
      else if (di->cin_limit > IINLIM_900 && di->cin_limit <= IINLIM_1200)
          Iin_limit = 4;
      else if (di->cin_limit > IINLIM_1200 && di->cin_limit <= IINLIM_1500)
          Iin_limit = 5;
      else if (di->cin_limit > IINLIM_1500 && di->cin_limit <= IINLIM_2000)
          Iin_limit = 6;
      else if (di->cin_limit > IINLIM_2000 && di->cin_limit <= IINLIM_3000)
          Iin_limit = 7;
      else
          Iin_limit = 4;

      Vdpm = (vindpm -VINDPM_MIN_3880)/VINDPM_STEP_80;

      di->input_source_reg00 = (di->hz_mode << BQ2419x_EN_HIZ_SHIFT)
                        | (Vdpm << BQ2419x_VINDPM_SHIFT) |Iin_limit;

      bq2419x_write_byte(di, di->input_source_reg00, INPUT_SOURCE_REG00);
      return;
}

static void bq2419x_config_power_on_reg(struct bq2419x_device_info *di)
{
      unsigned int sys_min = 0;
      u8 Sysmin = 0;

      sys_min = di->sys_minmV;

      if(sys_min < SYS_MIN_MIN_3000)
          sys_min = SYS_MIN_MIN_3000;
      else if (sys_min > SYS_MIN_MAX_3700)
          sys_min = SYS_MIN_MAX_3700;

      Sysmin = (sys_min -SYS_MIN_MIN_3000)/SYS_MIN_STEP_100;

      di->power_on_config_reg01 = WATCHDOG_TIMER_RST
           | (di->chrg_config << BQ2419x_EN_CHARGER_SHIFT)
           | (Sysmin << BQ2419x_SYS_MIN_SHIFT) | di->boost_lim;

       bq2419x_write_byte(di, di->power_on_config_reg01, POWER_ON_CONFIG_REG01);
       return;
}

static void bq2419x_config_current_reg(struct bq2419x_device_info *di)
{
    unsigned int currentmA = 0;
    u8 Vichrg = 0;

    currentmA = di->currentmA;
    /* if currentmA is below ICHG_MIN, we can set the ICHG to 5*currentmA and
       set the FORCE_20PCT in REG02 to make the true current 20% of the ICHG*/
    if (currentmA < ICHG_MIN) {
        currentmA = currentmA * 5;
        di->enable_low_chg = EN_FORCE_20PCT;
    } else {
        di->enable_low_chg = DIS_FORCE_20PCT;
    }
    if (currentmA < ICHG_MIN)
        currentmA = ICHG_MIN;
    else if(currentmA > ICHG_MAX)
        currentmA = ICHG_MAX;
    Vichrg = (currentmA - ICHG_MIN)/ICHG_STEP_64;

    di->charge_current_reg02 = (Vichrg << BQ2419x_ICHG_SHIFT) | di->enable_low_chg;

     bq2419x_write_byte(di, di->charge_current_reg02, CHARGE_CURRENT_REG02);
     return;
}

static void bq2419x_config_prechg_term_current_reg(struct bq2419x_device_info *di)
{
    unsigned int precurrentmA = 0;
    unsigned int term_currentmA = 0;
    u8 Prechg = 0;
    u8 Viterm = 0;

    precurrentmA = di->prechrg_currentmA;
    term_currentmA = di->term_currentmA;

    if(precurrentmA < IPRECHRG_MIN_128)
        precurrentmA = IPRECHRG_MIN_128;
    if(term_currentmA < ITERM_MIN_128)
        term_currentmA = ITERM_MIN_128;

    if((di->bqchip_version & BQ24192I)) {
        if(precurrentmA > IPRECHRG_640)
            precurrentmA = IPRECHRG_640;
     }

    if ((di->bqchip_version & (BQ24190|BQ24191|BQ24192))) {
        if (precurrentmA > IPRECHRG_MAX_2048)
            precurrentmA = IPRECHRG_MAX_2048;
    }

    if (term_currentmA > ITERM_MAX_2048)
        term_currentmA = ITERM_MAX_2048;

    Prechg = (precurrentmA - IPRECHRG_MIN_128)/IPRECHRG_STEP_128;
    Viterm = (term_currentmA-ITERM_MIN_128)/ITERM_STEP_128;

    di->prechrg_term_current_reg03 = (Prechg <<  BQ2419x_IPRECHRG_SHIFT| Viterm);
    bq2419x_write_byte(di, di->prechrg_term_current_reg03, PRECHARGE_TERM_CURRENT_REG03);
    return;
}

static void bq2419x_config_voltage_reg(struct bq2419x_device_info *di)
{
    unsigned int voltagemV = 0;
    u8 Voreg = 0;

    voltagemV = di->voltagemV;
    if (voltagemV < VCHARGE_MIN_3504)
        voltagemV = VCHARGE_MIN_3504;
    else if (voltagemV > VCHARGE_MAX_4400)
        voltagemV = VCHARGE_MAX_4400;

    Voreg = (voltagemV - VCHARGE_MIN_3504)/VCHARGE_STEP_16;

    di->charge_voltage_reg04 = (Voreg << BQ2419x_VCHARGE_SHIFT) | BATLOWV_3000 |VRECHG_100;
    bq2419x_write_byte(di, di->charge_voltage_reg04, CHARGE_VOLTAGE_REG04);
    return;
}

static void bq2419x_config_term_timer_reg(struct bq2419x_device_info *di)
{
    di->term_timer_reg05 = (di->enable_iterm << BQ2419x_EN_TERM_SHIFT)
        | di->watchdog_timer | (di->enable_timer << BQ2419x_EN_TIMER_SHIFT)
        | di->chrg_timer | JEITA_ISET;

    bq2419x_write_byte(di, di->term_timer_reg05, CHARGE_TERM_TIMER_REG05);
    return;
}

static void bq2419x_config_thernal_regulation_reg(struct bq2419x_device_info *di)
{
    unsigned int batcomp_ohm = 0;
    unsigned int vclampmV = 0;
    u8 Batcomp = 0;
    u8 Vclamp = 0;

    batcomp_ohm = di->bat_compohm;
    vclampmV = di->comp_vclampmV;

    if(batcomp_ohm > BAT_COMP_MAX_70)
        batcomp_ohm = BAT_COMP_MAX_70;

    if(vclampmV > VCLAMP_MAX_112)
        vclampmV = VCLAMP_MAX_112;

    Batcomp = batcomp_ohm/BAT_COMP_STEP_10;
    Vclamp = vclampmV/VCLAMP_STEP_16;

    di->thermal_regulation_reg06 = (Batcomp << BQ2419x_BAT_COMP_SHIFT)
                           |(Vclamp << BQ2419x_VCLAMP_SHIFT) |TREG_120;

    bq2419x_write_byte(di, di->thermal_regulation_reg06, THERMAL_REGUALTION_REG06);
    return;
}

static void bq2419x_config_misc_operation_reg(struct bq2419x_device_info *di)
{
    di->misc_operation_reg07 = (di->enable_dpdm << BQ2419x_DPDM_EN_SHIFT)
          | TMR2X_EN |(di->enable_batfet<< BQ2419x_BATFET_EN_SHIFT)
          | CHRG_FAULT_INT_EN |BAT_FAULT_INT_EN;

    bq2419x_write_byte(di, di->misc_operation_reg07, MISC_OPERATION_REG07);
    return;
}

/*deal with poor charger when capacity is more than 90% ,if hardware does not
 use REGN for usb_int,pls delete the follow fuction*/
static void bq2419x_reset_vindpm(struct bq2419x_device_info *di)
{
    int battery_capacity = 0;

    if ((di->charger_source == POWER_SUPPLY_TYPE_MAINS)&&(di->cin_dpmmV != VINDPM_4600)){
         battery_capacity = bq27510_battery_capacity(g_battery_measure_by_bq27510_device);
         if(battery_capacity > CAPACITY_LEVEL_HIGH_THRESHOLD){
             di->cin_dpmmV = VINDPM_4600;
             bq2419x_config_input_source_reg(di);
         }
     }
     return;
}

/*0 = temperature less than 42,1 = temperature more than 42 and less 45,2 more than 45 */
static void bq2419x_calling_limit_ac_input_current(struct bq2419x_device_info *di,int flag)
{
    if (di->charger_source == POWER_SUPPLY_TYPE_MAINS)
    {
       switch(flag){
       case NORMAL_TEMP_CONFIG:
           if(di->calling_limit){
               di->cin_limit = IINLIM_900;
               di->currentmA = ICHG_820;
           } else {
               di->cin_limit = di->max_cin_currentmA;
               di->currentmA = di->max_currentmA;
           }
           break;
       case NORMAL_HIGH_TEMP_CONFIG:
           if(di->calling_limit){
               di->cin_limit = IINLIM_900;
               di->currentmA = ICHG_820;
           } else {
               if(di->cin_limit == di->max_cin_currentmA)
                   di->currentmA =  di->max_currentmA;
               di->cin_limit = di->cin_limit;
               di->currentmA = di->currentmA;
           }

           break;
       case HIGH_TEMP_CONFIG:
           if(di->calling_limit){
               di->cin_limit = IINLIM_900;
               di->currentmA = ICHG_820;
           } else {
               di->cin_limit = IINLIM_900;
               di->currentmA = ICHG_820;
           }
           break;
       case TEMP_CONFIG_5:
           if(di->calling_limit){
               di->cin_limit = IINLIM_900;
               di->currentmA = di->design_capacity / 10 * di->charge_in_temp_5;
           } else {
               di->cin_limit = di->max_cin_currentmA;
               /* battery whose max_voltage is above 4.35V is easy to broken
                  when the temperature is below 10¡æ.
                  So we need set the Current below 0.x * Capacity. */
               di->currentmA = di->design_capacity / 10 * di->charge_in_temp_5;
           }
           break;
       case TEMP_CONFIG_10:
           if(di->calling_limit){
               di->cin_limit = IINLIM_900;
               di->currentmA = di->design_capacity / 10 * di->charge_in_temp_10;
           } else {
               di->cin_limit = di->max_cin_currentmA;
               di->currentmA = di->design_capacity / 10 * di->charge_in_temp_10;
           }
           break;
        case NORMAL_SECOND_STAGE:
            if(di->calling_limit){
               di->cin_limit = IINLIM_900;
               di->currentmA = ICHG_820;
           } else {
               di->cin_limit = IINLIM_1200;
               di->currentmA = ICHG_1024;
           }
           break;
       case NORMAL_HIGH_TEMP_CONFIG_CP1:
             if(di->calling_limit){
               di->cin_limit = di->cin_limit;
               di->currentmA = di->currentmA;
           } else {
               di->cin_limit = di->cin_limit;
               di->currentmA = di->currentmA;
           }
           break;
       case NORMAL_HIGH_TEMP_CONFIG_CP2:
            if(di->calling_limit){
               di->cin_limit = IINLIM_900;
               di->currentmA = ICHG_820;
           } else {
               di->cin_limit = IINLIM_900;
               di->currentmA = ICHG_820;
           }
           break;
       case NORMAL_HIGH_TEMP_CONFIG_CP3:
            if(di->calling_limit){
               di->cin_limit = di->cin_limit;
               di->currentmA = di->currentmA;
           } else {
               di->cin_limit = di->cin_limit;
               di->currentmA = di->currentmA;
           }
           break;
       case NORMAL_HIGH_TEMP_CONFIG_CP4:
            if(di->calling_limit){
               di->cin_limit = IINLIM_500;
               di->currentmA = ICHG_MIN;
           } else {
               di->cin_limit = IINLIM_500;
               di->currentmA = ICHG_MIN;
           }
        }
    }
   return;
}

static void bq2419x_config_limit_temperature_parameter(struct bq2419x_device_info *di)
{
    di->coldhot_charging_flag = 0;//DIS_CHARGER;
    di->limit_charging_flag = 1;//EN_CHARGER;
    di->hot_limit_charging_flag = 1;//enable charge limit at first
    di->temperature_cold = BQ2419x_COLD_BATTERY_THRESHOLD;
    di->temperature_cool = BQ2419x_COOL_BATTERY_THRESHOLD;
    di->temperature_warm = BQ2419x_WARM_BATTERY_THRESHOLD;
    di->temperature_hot  = BQ2419x_HOT_BATTERY_THRESHOLD;
    di->temperature_5    = BQ2419x_BATTERY_THRESHOLD_5;
    di->temperature_10   = BQ2419x_BATTERY_THRESHOLD_10;
    return;
}

static int bq2419x_check_battery_temperature_japan_threshold(struct bq2419x_device_info *di)
{
    int battery_temperature = 0;

    battery_temperature = bq27510_battery_temperature(g_battery_measure_by_bq27510_device);

    if (battery_temperature < BQ2419x_COLD_BATTERY_THRESHOLD) {
        return BATTERY_HEALTH_TEMPERATURE_OVERLOW;

    } else if((battery_temperature >= BQ2419x_COLD_BATTERY_THRESHOLD)
        && (battery_temperature <  BQ2419x_COOL_BATTERY_THRESHOLD)){
        return BATTERY_HEALTH_TEMPERATURE_LOW;

    } else if ((battery_temperature >= BQ2419x_COOL_BATTERY_THRESHOLD)
        && (battery_temperature < BQ2419x_BATTERY_THRESHOLD_5)){
       return BATTERY_HEALTH_TEMPERATURE_5;

    } else if ((battery_temperature >= BQ2419x_BATTERY_THRESHOLD_5)
        && (battery_temperature < BQ2419x_BATTERY_THRESHOLD_10)){
       return BATTERY_HEALTH_TEMPERATURE_10;

    } else if ((battery_temperature >= BQ2419x_BATTERY_THRESHOLD_10)
        && (battery_temperature < japan_temp_cp[di->high_temp_para].cp1)){
       return BATTERY_HEALTH_TEMPERATURE_NORMAL;

    } else if ((battery_temperature >= japan_temp_cp[di->high_temp_para].cp1)
        && (battery_temperature < japan_temp_cp[di->high_temp_para].cp2)){
        return BATTERY_HEALTH_TEMPERATURE_HIGH_CP1;

    } else if ((battery_temperature >= japan_temp_cp[di->high_temp_para].cp2)
        && (battery_temperature < japan_temp_cp[di->high_temp_para].cp3)){
        return BATTERY_HEALTH_TEMPERATURE_HIGH_CP2;

    } else if ((battery_temperature >= japan_temp_cp[di->high_temp_para].cp3)
        && (battery_temperature < japan_temp_cp[di->high_temp_para].cp4)){
        return BATTERY_HEALTH_TEMPERATURE_HIGH_CP3;

    } else if ((battery_temperature >= japan_temp_cp[di->high_temp_para].cp4)
        && (battery_temperature < japan_temp_cp[di->high_temp_para].cp5)){
        return BATTERY_HEALTH_TEMPERATURE_HIGH_CP4;

    } else if ((battery_temperature >= japan_temp_cp[di->high_temp_para].cp5)
        && (battery_temperature < japan_temp_cp[di->high_temp_para].cp6)){
        return BATTERY_HEALTH_TEMPERATURE_HIGH_CP5;

    }else if (battery_temperature >=  japan_temp_cp[di->high_temp_para].cp6){
       return BATTERY_HEALTH_TEMPERATURE_OVERHIGH;

    } else {
       return BATTERY_HEALTH_TEMPERATURE_NORMAL;
    }
}

static int bq2419x_check_battery_temperature_threshold(struct bq2419x_device_info *di)
{
    int battery_temperature = 0;

    battery_temperature = bq27510_battery_temperature(g_battery_measure_by_bq27510_device);

    if (battery_temperature < di->temperature_cold) {
        return BATTERY_HEALTH_TEMPERATURE_OVERLOW;

    } else if((battery_temperature >= di->temperature_cold)
        && (battery_temperature <  di->temperature_cool)){
        return BATTERY_HEALTH_TEMPERATURE_LOW;

    } else if ((battery_temperature >= di->temperature_cool)
        && (battery_temperature < di->temperature_5)){
       return BATTERY_HEALTH_TEMPERATURE_5;

    } else if ((battery_temperature >= di->temperature_5)
        && (battery_temperature < di->temperature_10)){
       return BATTERY_HEALTH_TEMPERATURE_10;

    } else if ((battery_temperature >= di->temperature_10)
        && (battery_temperature < (di->temperature_warm - BQ2419x_TEMPERATURE_OFFSET))){
       return BATTERY_HEALTH_TEMPERATURE_NORMAL;

    } else if ((battery_temperature >= (di->temperature_warm - BQ2419x_TEMPERATURE_OFFSET))
        && (battery_temperature < di->temperature_warm)){
        return BATTERY_HEALTH_TEMPERATURE_NORMAL_HIGH;

    } else if ((battery_temperature >= di->temperature_warm)
        && (battery_temperature < di->temperature_hot)){
        return BATTERY_HEALTH_TEMPERATURE_HIGH;

    } else if (battery_temperature >= di->temperature_hot){
       return BATTERY_HEALTH_TEMPERATURE_OVERHIGH;

    } else {
       return BATTERY_HEALTH_TEMPERATURE_NORMAL;
    }
}

/* get hot area temperature by nct203. */
static int get_hot_temperature()
{
    extern int nct203_temp_report(void);
    return nct203_temp_report();
}

static void bq2419x_monitor_battery_ntc_japan_charging(struct bq2419x_device_info *di)
{
    int battery_status = 0;
    long int events = VCHRG_START_CHARGING_EVENT;
    u8 read_reg = 0;
    bq2419x_read_byte(di, &read_reg, SYS_STATUS_REG08);
    if(!di->battery_present)
        return;
    int battery_temperature = bq27510_battery_temperature(g_battery_measure_by_bq27510_device);
    di->battery_voltage = bq27510_battery_voltage(g_battery_measure_by_bq27510_device);
    bq2419x_reset_vindpm(di);

    dev_info(di->dev,
        "%s: Status:\n"
        "battery temp is %d , ntc temp is %d , \n"
        "battery_voltage is %d , pre status is %d , \n"
        "di->limit_charging_flag is %d , di->hot_limit_charging_flag is %d.\n",
        __FUNCTION__, battery_temperature, get_hot_temperature(), di->battery_voltage,
        di->battery_temp_status, di->limit_charging_flag, di->hot_limit_charging_flag);

    /* We will not limit the charge current in two cases:
       1. User set limit_charging_flag to no limit(0) by Projectmenu
       2. TemperaturMoniter2 find nct203 is not high, set the hot_limit_charging_flag to no limit*/
    if(!di->limit_charging_flag
        || (!di->hot_limit_charging_flag && battery_temperature >= BQ2419x_BATTERY_THRESHOLD_10)){
        di->chrg_config = EN_CHARGER & di->factory_flag;

        /* if do not limit the current, set the current as BATTERY_HEALTH_TEMPERATURE_NORMAL */
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if((di->is_two_stage_charger== 1)
                &&( (di->battery_voltage >= di->first_stage_voltage)
                        || ((di->battery_voltage >= di->second_stage_voltage)
                        &&(di->two_stage_charger_status  == TWO_STAGE_CHARGE_SECOND_STAGE)))){
                 bq2419x_calling_limit_ac_input_current(di,NORMAL_SECOND_STAGE);
                 di->two_stage_charger_status  = TWO_STAGE_CHARGE_SECOND_STAGE;
            }else if(di->battery_voltage > BQ2419x_NORNAL_ICHRG_VOLTAGE){
                 bq2419x_calling_limit_ac_input_current(di,NORMAL_TEMP_CONFIG);
                 di->two_stage_charger_status  = TWO_STAGE_CHARGE_FIRST_STAGE;
            } else {
                 di->currentmA = ICHG_820;
            }
            bq2419x_config_input_source_reg(di);
            bq2419x_config_current_reg(di);
        }

        di->battery_temp_status = BATTERY_HEALTH_TEMPERATURE_NORMAL;

        bq2419x_config_power_on_reg(di);

        if(di->chrg_config){
            events = VCHRG_START_CHARGING_EVENT;
        } else {
            events = VCHRG_NOT_CHARGING_EVENT;
        }
        if ((read_reg & BQ2419x_CHGR_STAT_CHAEGE_DONE) == BQ2419x_CHGR_STAT_CHAEGE_DONE){
            return;
        }
        blocking_notifier_call_chain(&notifier_list, events, NULL);
        return;
    }

    battery_status = bq2419x_check_battery_temperature_japan_threshold(di);

    dev_info(di->dev, "%s, \n"
                         "di->battery_voltage = %d \n"
                         "battery_status = %d\n"
                         "di->two_stage_charger_status  = %d \n"
                          , __func__, di->battery_voltage, battery_status,di->two_stage_charger_status);

    switch (battery_status) {
    case BATTERY_HEALTH_TEMPERATURE_OVERLOW:
        di->chrg_config = DIS_CHARGER | di->coldhot_charging_flag;
        break;

    case BATTERY_HEALTH_TEMPERATURE_LOW:
        if (di->battery_voltage <= BQ2419x_LOW_TEMP_TERM_VOLTAGE)
            di->chrg_config = EN_CHARGER;
        else if (di->battery_voltage > BQ2419x_LOW_TEMP_NOT_CHARGING_VOLTAGE)
            di->chrg_config = DIS_CHARGER;
        else
            di->chrg_config =di->chrg_config;

        if(di->battery_temp_status != battery_status){
            di->currentmA = ICHG_LOW_TEMP;
            bq2419x_config_current_reg(di);
        }
        if (di->is_disable_cool_temperature_charger == 1){
            di->chrg_config = DIS_CHARGER;
        }
        break;

    case BATTERY_HEALTH_TEMPERATURE_5:
        di->chrg_config = EN_CHARGER;
       if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
           if (di->battery_temp_status != battery_status){
               bq2419x_calling_limit_ac_input_current(di,TEMP_CONFIG_5);
               bq2419x_config_input_source_reg(di);
               bq2419x_config_current_reg(di);
           }
        }
        break;

    case BATTERY_HEALTH_TEMPERATURE_10:
        di->chrg_config = EN_CHARGER;
       if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
           if (di->battery_temp_status != battery_status){
               bq2419x_calling_limit_ac_input_current(di,TEMP_CONFIG_10);
               bq2419x_config_input_source_reg(di);
               bq2419x_config_current_reg(di);
           }
        }
        break;

    case BATTERY_HEALTH_TEMPERATURE_NORMAL:
        di->chrg_config = EN_CHARGER;
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if((di->is_two_stage_charger== 1)
                &&( (di->battery_voltage >= di->first_stage_voltage)
                        || ((di->battery_voltage >= di->second_stage_voltage)
                        &&(di->two_stage_charger_status  == TWO_STAGE_CHARGE_SECOND_STAGE)))){
                 bq2419x_calling_limit_ac_input_current(di,NORMAL_SECOND_STAGE);
                 di->two_stage_charger_status  = TWO_STAGE_CHARGE_SECOND_STAGE;
            }else if(di->battery_voltage > BQ2419x_NORNAL_ICHRG_VOLTAGE){
                 bq2419x_calling_limit_ac_input_current(di,NORMAL_TEMP_CONFIG);
                 di->two_stage_charger_status  = TWO_STAGE_CHARGE_FIRST_STAGE;
            } else {
                 di->currentmA = ICHG_820;
            }
            bq2419x_config_input_source_reg(di);
            bq2419x_config_current_reg(di);
       }
        break;

    case BATTERY_HEALTH_TEMPERATURE_HIGH_CP1:
        di->chrg_config = EN_CHARGER;
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if (di->battery_temp_status != battery_status){
                bq2419x_calling_limit_ac_input_current(di,NORMAL_HIGH_TEMP_CONFIG_CP1);
                bq2419x_config_input_source_reg(di);
                bq2419x_config_current_reg(di);
            }
        }
        break;

     case BATTERY_HEALTH_TEMPERATURE_HIGH_CP2:
        di->chrg_config = EN_CHARGER;
        if(di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if (di->battery_temp_status != battery_status){
                bq2419x_calling_limit_ac_input_current(di,NORMAL_HIGH_TEMP_CONFIG_CP2);
                bq2419x_config_input_source_reg(di);
                bq2419x_config_current_reg(di);
            }
        }
        break;

      case BATTERY_HEALTH_TEMPERATURE_HIGH_CP3:
        di->chrg_config = EN_CHARGER;
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if (di->battery_temp_status != battery_status){
                bq2419x_calling_limit_ac_input_current(di,NORMAL_HIGH_TEMP_CONFIG_CP3);
                bq2419x_config_input_source_reg(di);
                bq2419x_config_current_reg(di);
            }
        }
        break;

     case BATTERY_HEALTH_TEMPERATURE_HIGH_CP4:
        di->chrg_config = EN_CHARGER;
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if (di->battery_temp_status != battery_status){
                bq2419x_calling_limit_ac_input_current(di,NORMAL_HIGH_TEMP_CONFIG_CP4);
                bq2419x_config_input_source_reg(di);
                bq2419x_config_current_reg(di);
            }
        }
        break;

     case BATTERY_HEALTH_TEMPERATURE_HIGH_CP5:
        di->chrg_config = di->chrg_config;
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if ((di->battery_temp_status != battery_status) && (di->chrg_config == EN_CHARGER)){
                bq2419x_calling_limit_ac_input_current(di,NORMAL_HIGH_TEMP_CONFIG_CP4);
                bq2419x_config_input_source_reg(di);
                bq2419x_config_current_reg(di);
            }
        }
        break;

    case BATTERY_HEALTH_TEMPERATURE_OVERHIGH:
        di->chrg_config = DIS_CHARGER | di->coldhot_charging_flag;
        break;
    default:
        break;
    }
    di->battery_temp_status = battery_status;
    di->chrg_config = di->chrg_config & di->factory_flag;

    if(!di->hot_enable_charge_flag){
        if(di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if(di->cin_limit > IINLIM_700){
                pre_cin_limit = di->cin_limit;
                di->cin_limit = IINLIM_700;
                bq2419x_config_input_source_reg(di);
            }
        }
        /* reset battery pre status for ntc set right current after exit hot disable charge */
        di->battery_temp_status = -1;
        dev_info(di->dev, "the hot area temperature is %d,"
            " it has stopped charing and depress the input source \n",get_hot_temperature());
    }
    if(di->chrg_config){
        events = VCHRG_START_CHARGING_EVENT;
    } else {
        events = VCHRG_NOT_CHARGING_EVENT;
    }
    bq2419x_config_power_on_reg(di);

    if ((read_reg & BQ2419x_CHGR_STAT_CHAEGE_DONE) == BQ2419x_CHGR_STAT_CHAEGE_DONE){
       return;
    }
    blocking_notifier_call_chain(&notifier_list, events, NULL);
    return;
}

static void bq2419x_monitor_battery_ntc_charging(struct bq2419x_device_info *di)
{
    if(di->japan_charger){
        return bq2419x_monitor_battery_ntc_japan_charging(di);
    }

    int battery_status = 0;
    long int events = VCHRG_START_CHARGING_EVENT;
    u8 read_reg = 0;
    bq2419x_read_byte(di, &read_reg, SYS_STATUS_REG08);
    if(!di->battery_present)
        return;
    int battery_temperature = bq27510_battery_temperature(g_battery_measure_by_bq27510_device);
    di->battery_voltage = bq27510_battery_voltage(g_battery_measure_by_bq27510_device);
    bq2419x_reset_vindpm(di);

    dev_info(di->dev,
        "%s: Status:\n"
        "battery temp is %d , ntc temp is %d , \n"
        "battery_voltage is %d , pre status is %d , \n"
        "di->limit_charging_flag is %d , di->hot_limit_charging_flag is %d.\n",
        __FUNCTION__, battery_temperature, get_hot_temperature(), di->battery_voltage,
        di->battery_temp_status, di->limit_charging_flag, di->hot_limit_charging_flag);

    /*
     * We will not limit the charge current in two cases:
     * 1. User set limit_charging_flag to no limit(0) by Projectmenu
     * 2. TemperaturMoniter2 find nct203 is below 48, set the hot_limit_charging_flag to 0
     */
    if(!di->limit_charging_flag
        || (!di->hot_limit_charging_flag && battery_temperature >= BQ2419x_BATTERY_THRESHOLD_10)){
        di->chrg_config = EN_CHARGER & di->factory_flag;

        /* if do not limit the current, set the current as BATTERY_HEALTH_TEMPERATURE_NORMAL */
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            bq2419x_calling_limit_ac_input_current(di,NORMAL_TEMP_CONFIG);
            bq2419x_config_input_source_reg(di);
            bq2419x_config_current_reg(di);
        }

        di->battery_temp_status = BATTERY_HEALTH_TEMPERATURE_NORMAL;
        bq2419x_config_power_on_reg(di);
        if(di->chrg_config){
            events = VCHRG_START_CHARGING_EVENT;
        } else {
            events = VCHRG_NOT_CHARGING_EVENT;
        }
        if ((read_reg & BQ2419x_CHGR_STAT_CHAEGE_DONE) == BQ2419x_CHGR_STAT_CHAEGE_DONE){
            return;
        }
        blocking_notifier_call_chain(&notifier_list, events, NULL);
        return;
    }

    battery_status = bq2419x_check_battery_temperature_threshold(di);

    switch (battery_status) {
    case BATTERY_HEALTH_TEMPERATURE_OVERLOW:
        di->chrg_config = DIS_CHARGER | di->coldhot_charging_flag;
        break;
    case BATTERY_HEALTH_TEMPERATURE_LOW:
        if (di->battery_voltage <= BQ2419x_LOW_TEMP_TERM_VOLTAGE)
            di->chrg_config = EN_CHARGER;
        else if (di->battery_voltage > BQ2419x_LOW_TEMP_NOT_CHARGING_VOLTAGE)
            di->chrg_config = DIS_CHARGER;
        else
            di->chrg_config =di->chrg_config;

        if(di->battery_temp_status != battery_status){
            di->currentmA = ICHG_LOW_TEMP;
            bq2419x_config_current_reg(di);
       }
        break;
    case BATTERY_HEALTH_TEMPERATURE_5:
        di->chrg_config = EN_CHARGER;
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
           if (di->battery_temp_status != battery_status){
               bq2419x_calling_limit_ac_input_current(di,TEMP_CONFIG_5);
               bq2419x_config_input_source_reg(di);
               bq2419x_config_current_reg(di);
           }
        }
        break;
    case BATTERY_HEALTH_TEMPERATURE_10:
        di->chrg_config = EN_CHARGER;
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
           if (di->battery_temp_status != battery_status){
               bq2419x_calling_limit_ac_input_current(di,TEMP_CONFIG_10);
               bq2419x_config_input_source_reg(di);
               bq2419x_config_current_reg(di);
           }
        }
        break;
    case BATTERY_HEALTH_TEMPERATURE_NORMAL:
        di->chrg_config = EN_CHARGER;
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if(di->battery_voltage > BQ2419x_NORNAL_ICHRG_VOLTAGE){
                 bq2419x_calling_limit_ac_input_current(di,NORMAL_TEMP_CONFIG);
            } else {
                 di->currentmA = ICHG_820;
            }
            bq2419x_config_input_source_reg(di);
            bq2419x_config_current_reg(di);
       }
        break;
    case BATTERY_HEALTH_TEMPERATURE_NORMAL_HIGH:
        di->chrg_config = EN_CHARGER;
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if (di->battery_temp_status != battery_status){
               bq2419x_calling_limit_ac_input_current(di,NORMAL_HIGH_TEMP_CONFIG);
                bq2419x_config_input_source_reg(di);
                bq2419x_config_current_reg(di);
            }
        }

        break;
    case BATTERY_HEALTH_TEMPERATURE_HIGH:
        di->chrg_config = EN_CHARGER;
        if(di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if (di->battery_temp_status != battery_status){
                bq2419x_calling_limit_ac_input_current(di,HIGH_TEMP_CONFIG);
                bq2419x_config_input_source_reg(di);
                bq2419x_config_current_reg(di);
            }
        }
        break;
    case BATTERY_HEALTH_TEMPERATURE_OVERHIGH:
        di->chrg_config = DIS_CHARGER | di->coldhot_charging_flag;
        break;
    default:
        break;
    }

    di->battery_temp_status = battery_status;

    di->chrg_config = di->chrg_config & di->factory_flag;

    if(!di->hot_enable_charge_flag){
        if(di->charger_source == POWER_SUPPLY_TYPE_MAINS){
            if(di->cin_limit > IINLIM_700){
                pre_cin_limit = di->cin_limit;
                di->cin_limit = IINLIM_700;
                bq2419x_config_input_source_reg(di);
            }
        }
        /* reset battery pre status for ntc set right current after exit hot disable charge */
        di->battery_temp_status = -1;
        dev_info(di->dev, "the hot area temperature is %d,"
            " it has stopped charing and depress the input source \n",get_hot_temperature());
    }
    if(di->chrg_config){
        events = VCHRG_START_CHARGING_EVENT;
    } else {
        events = VCHRG_NOT_CHARGING_EVENT;
    }
    bq2419x_config_power_on_reg(di);

    if ((read_reg & BQ2419x_CHGR_STAT_CHAEGE_DONE) == BQ2419x_CHGR_STAT_CHAEGE_DONE){
       return;
    }
    blocking_notifier_call_chain(&notifier_list, events, NULL);
    return;
}

static void bq2419x_start_usb_charger(struct bq2419x_device_info *di)
{
    long int  events = VCHRG_START_USB_CHARGING_EVENT;

    di->wakelock_enabled = 1;
    if (di->wakelock_enabled){
        wake_lock(&chrg_lock);
    }

    gpio_set_value(ENABLE_BQ2419x_CHARGER, 0);

    blocking_notifier_call_chain(&notifier_list, events, NULL);
    di->charger_source = POWER_SUPPLY_TYPE_USB;
    di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
    bq2419x_get_max_charge_voltage(di);
    di->cin_dpmmV = VINDPM_4520;
    di->cin_limit = IINLIM_500;
    pre_cin_limit = IINLIM_500;

    di->voltagemV = di->max_voltagemV;
    di->currentmA = ICHG_MIN ;
    di->term_currentmA = ITERM_MIN_128;
    di->watchdog_timer = WATCHDOG_40;
    di->chrg_timer = CHG_TIMER_12;

    di->hz_mode = DIS_HIZ;
    di->chrg_config = EN_CHARGER;
    di->boost_lim = BOOST_LIM_500;
    di->enable_low_chg = DIS_FORCE_20PCT;
    di->enable_iterm = DIS_TERM;
    di->enable_timer = EN_TIMER;
    di->enable_batfet = EN_BATFET;
    di->factory_flag = EN_CHARGER;
    di->calling_limit = 0;
    di->battery_temp_status = -1;
    di->enable_dpdm = DPDM_EN;
    di->charge_full_count = 0;

    bq2419x_config_power_on_reg(di);
    msleep(500);
    bq2419x_config_input_source_reg(di);
    bq2419x_config_current_reg(di);
    bq2419x_config_prechg_term_current_reg(di);
    bq2419x_config_voltage_reg(di);
    bq2419x_config_term_timer_reg(di);
    bq2419x_config_thernal_regulation_reg(di);
    bq2419x_config_misc_operation_reg(di);
    schedule_delayed_work(&di->bq2419x_charger_work, msecs_to_jiffies(0));

    dev_info(di->dev, "%s, ---->START USB CHARGING, \n"
                         "battery current = %d mA\n"
                         "battery voltage = %d mV\n"
                         "cin_limit_current = %d mA\n"
                         , __func__, di->currentmA, di->voltagemV,di->cin_limit);

    di->battery_present = is_bq27510_battery_exist(g_battery_measure_by_bq27510_device);
    if (!di->battery_present) {
            dev_dbg(di->dev, "BATTERY NOT DETECTED!\n");
            events = VCHRG_NOT_CHARGING_EVENT;
            blocking_notifier_call_chain(&notifier_list, events, NULL);
            di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
    }

    return;
}

static void bq2419x_start_ac_charger(struct bq2419x_device_info *di)
{
    long int  events = VCHRG_START_AC_CHARGING_EVENT;

    di->wakelock_enabled = 1;
    if (di->wakelock_enabled){
        wake_lock(&chrg_lock);
    }

    gpio_set_value(ENABLE_BQ2419x_CHARGER, 0);

    blocking_notifier_call_chain(&notifier_list, events, NULL);
    di->charger_source = POWER_SUPPLY_TYPE_MAINS;
    di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
    bq2419x_get_max_charge_voltage(di);
    di->cin_dpmmV = VINDPM_4520;
    di->cin_limit = di->max_cin_currentmA;
    pre_cin_limit = di->max_cin_currentmA;
    di->voltagemV = di->max_voltagemV;
    di->currentmA = di->max_currentmA;
    di->term_currentmA = ITERM_MIN_128;
    di->watchdog_timer = WATCHDOG_40;
    di->chrg_timer = CHG_TIMER_8;

    di->hz_mode = DIS_HIZ;
    di->chrg_config = EN_CHARGER;
    di->boost_lim = BOOST_LIM_500;
    di->enable_low_chg = DIS_FORCE_20PCT;
    di->enable_iterm = DIS_TERM;
    di->enable_timer = EN_TIMER;
    di->enable_batfet = EN_BATFET;
    di->factory_flag = EN_CHARGER;
    di->enable_dpdm = DPDM_DIS;
    di->calling_limit = 0;
    di->battery_temp_status = -1;
    di->charge_full_count = 0;

    bq2419x_config_power_on_reg(di);
    msleep(500);
    bq2419x_config_input_source_reg(di);
    bq2419x_config_current_reg(di);
    bq2419x_config_prechg_term_current_reg(di);
    bq2419x_config_voltage_reg(di);
    bq2419x_config_term_timer_reg(di);
    bq2419x_config_thernal_regulation_reg(di);
    bq2419x_config_misc_operation_reg(di);
    schedule_delayed_work(&di->bq2419x_charger_work, msecs_to_jiffies(0));

    dev_info(di->dev, "%s, ---->START AC CHARGING, \n"
                         "battery current = %d mA\n"
                         "battery voltage = %d mV\n"
                         "cin_limit_current = %d mA\n"
                         , __func__, di->currentmA, di->voltagemV,di->cin_limit);

    di->battery_present = is_bq27510_battery_exist(g_battery_measure_by_bq27510_device);
    if (!di->battery_present) {
            dev_dbg(di->dev, "BATTERY NOT DETECTED!\n");
            events = VCHRG_NOT_CHARGING_EVENT;
            blocking_notifier_call_chain(&notifier_list, events, NULL);
            di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
    }

    return;
}

static void bq2419x_start_usb_otg(struct bq2419x_device_info *di)
{
    dev_info(di->dev,"%s,---->USB_EVENT_OTG_ID<----\n", __func__);

    if(di->charger_source != POWER_SUPPLY_TYPE_BATTERY){
        gpio_set_value(ENABLE_BQ2419x_CHARGER, 1);
        return;
    }

    di->wakelock_enabled = 1;
    if (di->wakelock_enabled){
        wake_lock(&chrg_lock);
    }

    gpio_set_value(ENABLE_BQ2419x_CHARGER, 0);

    di->hz_mode = DIS_HIZ;
    di->chrg_config = EN_CHARGER_OTG;
    di->boost_lim = BOOST_LIM_500;

    bq2419x_config_power_on_reg(di);
    bq2419x_config_input_source_reg(di);

    schedule_delayed_work(&di->bq2419x_usb_otg_work, msecs_to_jiffies(0));

    return;
}

static void bq2419x_stop_charger(struct bq2419x_device_info *di)
{
    long int  events  = VCHRG_STOP_CHARGING_EVENT;

    if (!wake_lock_active(&chrg_lock)){
        wake_lock(&chrg_lock);
    }

    /*set gpio_074 high level for CE pin to disable bq2419x IC */
    gpio_set_value(ENABLE_BQ2419x_CHARGER, 1);

    cancel_delayed_work_sync(&di->bq2419x_charger_work);
    blocking_notifier_call_chain(&notifier_list, events, NULL);

    dev_info(di->dev,"%s,---->STOP CHARGING\n", __func__);
    di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
    di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;

    di->enable_batfet = EN_BATFET;
    di->calling_limit = 0;
    di->factory_flag = 0;
    di->hz_mode = DIS_HIZ;
    di->battery_temp_status = -1;
    di->chrg_config = DIS_CHARGER;
    di->enable_dpdm = DPDM_DIS;
    di->charge_full_count = 0;

    bq2419x_config_power_on_reg(di);
    bq2419x_config_input_source_reg(di);
    bq2419x_config_misc_operation_reg(di);


    cancel_delayed_work_sync(&di->bq2419x_usb_otg_work);

    /*set gpio_074 high level for CE pin to disable bq2419x IC */
    gpio_set_value(ENABLE_BQ2419x_CHARGER, 1);
    msleep(1000);

    di->wakelock_enabled = 1;
    if (di->wakelock_enabled)
        wake_unlock(&chrg_lock);

    wakeup_timer_seconds = 0;
   return;
}

static void bq2419x_check_bq27510_charge_full(struct bq2419x_device_info *di)
{
    if(di->battery_present){

        if(di->battery_full){
            di->enable_iterm = EN_TERM;
            di->charge_full_count++;
            if(di->charge_full_count >= 20){
               di->charge_full_count = 20;
            }
        }else{
            di->enable_iterm = DIS_TERM;
        }
    }else{
        di->enable_iterm = EN_TERM;
    }

    bq2419x_config_term_timer_reg(di);

    return;
}

static void bq2419x_charger_done_release_wakelock(struct bq2419x_device_info *di)
{
    if(di->charger_source == POWER_SUPPLY_TYPE_MAINS){
        if(!di->battery_present)
            return;
        if(di->battery_full){
            if (wake_lock_active(&chrg_lock)){
                if(di->charge_full_count >= 20){
                    wake_unlock(&chrg_lock);
                    dev_err(di->dev, "ac charge done wakelock release\n");
                }
            }
        }else{
            if (!wake_lock_active(&chrg_lock)){
                wake_lock(&chrg_lock);
                dev_err(di->dev, "ac recharge wakelock add again\n");
            }
        }
    }
    return;
}

static void bq2419x_config_status_reg(struct bq2419x_device_info *di)
{
    di->power_on_config_reg01 = di->power_on_config_reg01 |WATCHDOG_TIMER_RST;
    bq2419x_write_byte(di, di->power_on_config_reg01, POWER_ON_CONFIG_REG01);
    return;
}

static void
bq2419x_charger_update_status(struct bq2419x_device_info *di)
{
    long int  events  = 0;
    u8 read_reg[11] = {0};

    di->timer_fault = 0;
    bq2419x_read_block(di, &read_reg[0], 0, 11);

    if ((read_reg[8] & BQ2419x_CHGR_STAT_CHAEGE_DONE) == BQ2419x_CHGR_STAT_CHAEGE_DONE){
        dev_dbg(di->dev, "CHARGE DONE\n");
        events  = VCHRG_CHARGE_DONE_EVENT;
        blocking_notifier_call_chain(&notifier_list, events, NULL);
    }

    if ((read_reg[8] & BQ2419x_PG_STAT) == BQ2419x_NOT_PG_STAT){
        di->cfg_params = 1;
        dev_info(di->dev, "not power good\n");
    }

    if ((read_reg[9] & BQ2419x_POWER_SUPPLY_OVP) == BQ2419x_POWER_SUPPLY_OVP){
        dev_err(di->dev, "POWER_SUPPLY_OVERVOLTAGE = %x\n", read_reg[9]);
        events  = VCHRG_POWER_SUPPLY_OVERVOLTAGE;
        blocking_notifier_call_chain(&notifier_list, events, NULL);
    }

    if ((read_reg[9] & BQ2419x_WATCHDOG_FAULT) == BQ2419x_WATCHDOG_FAULT)
        di->timer_fault = 1;

    if (read_reg[9] & 0xFF) {
        di->cfg_params = 1;
        dev_err(di->dev, "CHARGER STATUS %x\n", read_reg[9]);
    }

    if ((read_reg[9] & BQ2419x_BAT_FAULT_OVP) == BQ2419x_BAT_FAULT_OVP) {
        di->hz_mode = EN_HIZ;
        bq2419x_config_input_source_reg(di);
        bq2419x_write_byte(di, di->charge_voltage_reg04, CHARGE_VOLTAGE_REG04);
        dev_err(di->dev, "BATTERY OVP = %x\n", read_reg[9]);
        msleep(700);
        di->hz_mode = DIS_HIZ;
        bq2419x_config_input_source_reg(di);
    }

    if ((di->timer_fault == 1) || (di->cfg_params == 1)) {
        bq2419x_write_byte(di, di->input_source_reg00, INPUT_SOURCE_REG00);
        bq2419x_write_byte(di, di->power_on_config_reg01, POWER_ON_CONFIG_REG01);
        bq2419x_write_byte(di, di->charge_current_reg02, CHARGE_CURRENT_REG02);
        bq2419x_write_byte(di, di->prechrg_term_current_reg03, PRECHARGE_TERM_CURRENT_REG03);
        bq2419x_write_byte(di, di->charge_voltage_reg04, CHARGE_VOLTAGE_REG04);
        bq2419x_write_byte(di, di->term_timer_reg05, CHARGE_TERM_TIMER_REG05);
        bq2419x_write_byte(di, di->thermal_regulation_reg06, THERMAL_REGUALTION_REG06);
        di->cfg_params = 0;
    }

    /* reset 30 second timer */
    bq2419x_config_status_reg(di);
}

static void bq2419x_charger_work(struct work_struct *work)
{
    struct bq2419x_device_info *di = container_of(work,
                 struct bq2419x_device_info, bq2419x_charger_work.work);

    di->battery_present = is_bq27510_battery_exist(g_battery_measure_by_bq27510_device);

    di->battery_full = (is_bq27510_battery_full(g_battery_measure_by_bq27510_device)
        &&(bq27510_battery_current(g_battery_measure_by_bq27510_device) <= 100));

    bq2419x_monitor_battery_ntc_charging(di);

    bq2419x_check_bq27510_charge_full(di);

    bq2419x_charger_update_status(di);

    bq2419x_charger_done_release_wakelock(di);

    schedule_delayed_work(&di->bq2419x_charger_work,
                 msecs_to_jiffies(BQ2419x_WATCHDOG_TIMEOUT));
}

static void bq2419x_usb_otg_work(struct work_struct *work)
{
    struct bq2419x_device_info *di = container_of(work,
        struct bq2419x_device_info, bq2419x_usb_otg_work.work);
    u8 read_reg[11] = {0};

    bq2419x_read_block(di, &read_reg[0], 0, 10);
    if(read_reg[9] & BQ2419x_OTG_FAULT){
        dev_err(di->dev, "VBUS overloaded in OTG, or VBUS OVP = %x\n", read_reg[9]);
     }

        /* reset 30 second timer */
    bq2419x_config_power_on_reg(di);

    schedule_delayed_work(&di->bq2419x_usb_otg_work,
                 msecs_to_jiffies(BQ2419x_WATCHDOG_TIMEOUT));
}

static void bq2419x_usb_charger_work(struct work_struct *work)
{
    struct bq2419x_device_info	*di =
              container_of(work, struct bq2419x_device_info, usb_work);

    switch (di->event) {
    case CHARGER_TYPE_USB:
        dev_info(di->dev, "case = CHARGER_TYPE_USB-> \n");
        bq2419x_start_usb_charger(di);
        break;
    case CHARGER_TYPE_NON_STANDARD:
        dev_info(di->dev, "case = CHARGER_TYPE_NON_STANDARD -> \n");
        bq2419x_start_usb_charger(di);
        break;
    case CHARGER_TYPE_BC_USB:
        dev_info(di->dev, "case = CHARGER_TYPE_BC_USB -> \n");
        bq2419x_start_ac_charger(di);
        break;
    case CHARGER_TYPE_STANDARD:
        dev_info(di->dev, "case = CHARGER_TYPE_STANDARD\n");
        bq2419x_start_ac_charger(di);
        break;
    case CHARGER_REMOVED:
        dev_info(di->dev, "case = USB_EVENT_NONE\n");
        bq2419x_stop_charger(di);
        break;
    case USB_EVENT_OTG_ID:
        bq2419x_start_usb_otg(di);
        break;
    default:
        break;
    }
}

static int bq2419x_usb_notifier_call(struct notifier_block *nb,
                        unsigned long event, void *data)
{
    struct bq2419x_device_info *di =
          container_of(nb, struct bq2419x_device_info, nb);

    di->event = event;
    switch (event) {
    case CHARGER_TYPE_USB:
        break;
    case CHARGER_TYPE_NON_STANDARD:
        break;
    case CHARGER_TYPE_BC_USB:
        break;
    case CHARGER_TYPE_STANDARD:
        break;
    case CHARGER_REMOVED:
        break;
    case USB_EVENT_OTG_ID:
        break;
    default:
        return NOTIFY_OK;
    }

    schedule_work(&di->usb_work);
    return NOTIFY_OK;
}


/*
* set 1 --- hz_mode ; 0 --- not hz_mode
*/
static ssize_t bq2419x_set_enable_hz_mode(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
        return -EINVAL;

    di->hz_mode= val;
    bq2419x_config_input_source_reg(di);

    return status;
}

static ssize_t bq2419x_show_enable_hz_mode(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->hz_mode;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_dppm_voltage(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < VINDPM_MIN_3880)
                      || (val > VINDPM_MAX_5080))
         return -EINVAL;

    di->cin_dpmmV = val;
    bq2419x_config_input_source_reg(di);

    return status;
}

static ssize_t bq2419x_show_dppm_voltage(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->cin_dpmmV;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_cin_limit(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < IINLIM_100)
                || (val > IINLIM_3000))
        return -EINVAL;

    di->cin_limit = val;
    bq2419x_config_input_source_reg(di);

    return status;
}

static ssize_t bq2419x_show_cin_limit(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->cin_limit;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_regulation_voltage(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < VCHARGE_MIN_3504)
                     || (val > VCHARGE_MAX_4400))
        return -EINVAL;

    di->voltagemV = val;
    bq2419x_config_voltage_reg(di);

    return status;
}

static ssize_t bq2419x_show_regulation_voltage(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->voltagemV;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_charge_current(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < ICHG_MIN)
                   || (val > ICHG_3000))
        return -EINVAL;

    di->currentmA = val;
    bq2419x_config_current_reg(di);

    return status;
}

static ssize_t bq2419x_show_charge_current(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->currentmA;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_precharge_current(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > IPRECHRG_MAX_2048))
    return -EINVAL;

    di->prechrg_currentmA = val;
    bq2419x_config_prechg_term_current_reg(di);

    return status;
}

static ssize_t bq2419x_show_precharge_current(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->prechrg_currentmA;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_termination_current(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > ITERM_MAX_2048))
        return -EINVAL;

    di->term_currentmA = val;
    bq2419x_config_prechg_term_current_reg(di);

    return status;
}

static ssize_t bq2419x_show_termination_current(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->term_currentmA;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_enable_itermination(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
        return -EINVAL;

    di->enable_iterm = val;
    bq2419x_config_term_timer_reg(di);

    return status;
}

static ssize_t bq2419x_show_enable_itermination(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->enable_iterm;
    return sprintf(buf, "%lu\n", val);
}

/*
 * Add a sysfs to set the parameter hot_enable_charge_flag.
 * If framework call to disable charge because ntc(hot area) temperature is over 53,
 * set hot_enable_charge_flag to 0. Then schedule bq2419x_monitor_battery_ntc_charging,
 * it will set input source to 100 and disable charge by setting chrg_config to disable.
 * If set hot_enable_charge_flag to 0, it means the temperature is too high.
 * Just stop charging can't make the temperature falling down,
 * so it limits input source to 100mA together.
 */
static ssize_t bq2419x_set_hot_enable_charger(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    long int events = VCHRG_START_CHARGING_EVENT;
    u8 read_reg = 0;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
        return -EINVAL;

    di->hot_enable_charge_flag = val;

    if(di->hot_enable_charge_flag)
    {
        /* if enable charge by TemperatureMoniter2, recovery the cin_limit. */
		if (di->charger_source == POWER_SUPPLY_TYPE_MAINS)
			di->cin_limit = pre_cin_limit;
        dev_info(di->dev, "%s: hot area temperature is %d, enable charge.\n",
            __FUNCTION__, get_hot_temperature());
    }
    else
        dev_info(di->dev, "%s: hot area temperature is %d, disable charge.\n",
            __FUNCTION__, get_hot_temperature());

    bq2419x_monitor_battery_ntc_charging(di);

    return status;
}

static ssize_t bq2419x_show_hot_enable_charger(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->hot_enable_charge_flag ;
    return sprintf(buf, "%lu\n", val);
}
/* set 1 --- enable_charger; 0 --- disable charger */
static ssize_t bq2419x_set_enable_charger(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    long int events = VCHRG_START_CHARGING_EVENT;
    u8 read_reg = 0;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
        return -EINVAL;

    di->chrg_config = val;
    di->factory_flag = val;
    if(di->factory_flag){
        events = VCHRG_START_CHARGING_EVENT;
    } else {
        events = VCHRG_NOT_CHARGING_EVENT;
    }
    bq2419x_config_power_on_reg(di);

    bq2419x_read_byte(di, &read_reg, SYS_STATUS_REG08);
    if ((read_reg & BQ2419x_CHGR_STAT_CHAEGE_DONE) == BQ2419x_CHGR_STAT_CHAEGE_DONE){
        return status;
    }
    blocking_notifier_call_chain(&notifier_list, events, NULL);
    return status;
}

static ssize_t bq2419x_show_enable_charger(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->chrg_config ;
    return sprintf(buf, "%lu\n", val);
}

/* set 1 --- enable_batfet; 0 --- disable batfet */
static ssize_t bq2419x_set_enable_batfet(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
        return -EINVAL;

    di->enable_batfet = val ^ 0x01;
    bq2419x_config_misc_operation_reg(di);

    return status;
}

static ssize_t bq2419x_show_enable_batfet(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->enable_batfet ^ 0x01;
    return sprintf(buf, "%lu\n", val);
}

/*
* set 1 --- enable bq24192 IC; 0 --- disable bq24192 IC
*
*/
static ssize_t bq2419x_set_enable_cd(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
        return -EINVAL;

    di->cd_active =val ^ 0x1;
    gpio_set_value(ENABLE_BQ2419x_CHARGER, di->cd_active);
    return status;
}

static ssize_t bq2419x_show_enable_cd(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->cd_active ^ 0x1;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_charging(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if (strncmp(buf, "startac", 7) == 0) {
        if (di->charger_source == POWER_SUPPLY_TYPE_USB)
            bq2419x_stop_charger(di);
        bq2419x_start_ac_charger(di);
    } else if (strncmp(buf, "startusb", 8) == 0) {
        if (di->charger_source == POWER_SUPPLY_TYPE_MAINS)
            bq2419x_stop_charger(di);
        bq2419x_start_usb_charger(di);
    } else if (strncmp(buf, "stop" , 4) == 0) {
        bq2419x_stop_charger(di);
    } else if (strncmp(buf, "otg" , 3) == 0) {
        if (di->charger_source == POWER_SUPPLY_TYPE_BATTERY){
            bq2419x_stop_charger(di);
            bq2419x_start_usb_otg(di);
        } else{
            return -EINVAL;
        }
    } else
        return -EINVAL;

    return status;
}

static ssize_t bq2419x_set_wakelock_enable(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
        return -EINVAL;

    if ((val) && (di->charger_source != POWER_SUPPLY_TYPE_BATTERY))
        wake_lock(&chrg_lock);
    else
        wake_unlock(&chrg_lock);

    di->wakelock_enabled = val;
    return status;
}

static ssize_t bq2419x_show_wakelock_enable(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned int val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->wakelock_enabled;
    return sprintf(buf, "%u\n", val);
}

static ssize_t bq2419x_show_chargelog(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    int i = 0;
    u8 read_reg[11] = {0};
    u8 buf_temp[26] = {0};
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    bq2419x_read_block(di, &read_reg[0], 0, 11);
    for(i=0;i<11;i++)
    {
        snprintf(buf_temp,26,"0x%-8.2x",read_reg[i]);
        strcat(buf,buf_temp);
    }
    strcat(buf,"\n");
   return strlen(buf);
}

static ssize_t bq2419x_set_calling_limit(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
       return -EINVAL;

    di->calling_limit = val;
    if (di->charger_source == POWER_SUPPLY_TYPE_MAINS)
    {
        if(di->calling_limit){
            di->cin_limit = IINLIM_900;
            di->currentmA = ICHG_820;
            bq2419x_config_input_source_reg(di);
            bq2419x_config_current_reg(di);
            dev_info(di->dev,"calling_limit_current = %d\n", di->cin_limit);
        } else {
            di->battery_temp_status = -1;
            di->cin_limit = di->max_cin_currentmA;
            di->currentmA = di->max_currentmA ;
            bq2419x_config_input_source_reg(di);
            bq2419x_config_current_reg(di);
        }
    } else {
        di->calling_limit = 0;
    }

    return status;
}

static ssize_t bq2419x_show_calling_limit(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->calling_limit;

    return sprintf(buf, "%lu\n", val);
}
static ssize_t bq2419x_set_compesation_resistor(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 70))
        return -EINVAL;

    di->bat_compohm = val;
    bq2419x_config_thernal_regulation_reg(di);

    return status;
}

static ssize_t bq2419x_show_compesation_resistor(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->bat_compohm;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_compesation_voltage(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 112))
        return -EINVAL;

    di->comp_vclampmV = val;
    bq2419x_config_thernal_regulation_reg(di);

    return status;
}

static ssize_t bq2419x_show_compesation_voltage(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->comp_vclampmV;
    return sprintf(buf, "%lu\n", val);
}

/*
 * Add a sysfs to set the parameter hot_limit_charging_flag.
 * If framework call to limit current because ntc(hot area) temperature is over 48C,
 * set hot_limit_charging_flag to 1.
 * Then schedule bq2419x_monitor_battery_ntc_charging to set current to no limit status.
 */
static ssize_t bq2419x_set_hot_limit_temp_charging(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
        return -EINVAL;

    di->hot_limit_charging_flag = val;

    if(di->hot_limit_charging_flag)
        dev_info(di->dev, "%s: hot area temperature is %d, start limit charging.\n",
            __FUNCTION__, get_hot_temperature());
    else
        dev_info(di->dev, "%s: hot area temperature is %d, stop limit charging.\n",
            __FUNCTION__, get_hot_temperature());

    bq2419x_monitor_battery_ntc_charging(di);

    return status;
}

static ssize_t bq2419x_show_hot_limit_temp_charging(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->hot_limit_charging_flag;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_limit_temp_charging(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
        return -EINVAL;

    di->limit_charging_flag = val;

    bq2419x_monitor_battery_ntc_charging(di);

    return status;
}

static ssize_t bq2419x_show_limit_temp_charging(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->limit_charging_flag;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_coldhot_charging(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
        return -EINVAL;

    di->coldhot_charging_flag = val;
    di->limit_charging_flag = 1;
    di->hot_limit_charging_flag = 1;
    bq2419x_monitor_battery_ntc_charging(di);
    return status;
}

static ssize_t bq2419x_show_coldhot_charging(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    unsigned long val;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    val = di->coldhot_charging_flag;
    return sprintf(buf, "%lu\n", val);
}

static ssize_t bq2419x_set_temperature_parameter(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < 40)
                   || (val > 70))
        return -EINVAL;

     di->temperature_warm = val;
     di->temperature_hot  = val + 5;

    bq2419x_monitor_battery_ntc_charging(di);

    return status;
}

static ssize_t bq2419x_show_temperature_parameter(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    int read_reg[6] = {0};
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    read_reg[0] = di->temperature_cold;
    read_reg[1] = di->temperature_cool;
    read_reg[2] = di->temperature_5;
    read_reg[3] = di->temperature_10;
    read_reg[4] = di->temperature_warm;
    read_reg[5] = di->temperature_hot;

    sprintf(buf,"%-9d  %-9d  %-9d  %-9d  %-9d  %-9d",
    read_reg[0],read_reg[1],read_reg[2],read_reg[3],read_reg[4],read_reg[5]);

    return strlen(buf);
}

static ssize_t bq2419x_set_current(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    long val;
    int status = count;
    struct bq2419x_device_info *di = dev_get_drvdata(dev);

    if ((strict_strtol(buf, 10, &val) < 0) || (val < ICHG_MIN)
                   || (val > ICHG_3000))
        return -EINVAL;

    di->max_currentmA = val;
    di->max_cin_currentmA = val;

    return status;
}

static DEVICE_ATTR(enable_hz_mode, S_IWUSR | S_IRUGO,
                bq2419x_show_enable_hz_mode,
                bq2419x_set_enable_hz_mode);
static DEVICE_ATTR(dppm_voltage, S_IWUSR | S_IRUGO,
                bq2419x_show_dppm_voltage,
                bq2419x_set_dppm_voltage);
static DEVICE_ATTR(cin_limit, S_IWUSR | S_IRUGO,
                bq2419x_show_cin_limit,
                bq2419x_set_cin_limit);
static DEVICE_ATTR(regulation_voltage, S_IWUSR | S_IRUGO,
                bq2419x_show_regulation_voltage,
                bq2419x_set_regulation_voltage);
static DEVICE_ATTR(charge_current, S_IWUSR | S_IRUGO,
                bq2419x_show_charge_current,
                bq2419x_set_charge_current);
static DEVICE_ATTR(termination_current, S_IWUSR | S_IRUGO,
                bq2419x_show_termination_current,
                bq2419x_set_termination_current);
static DEVICE_ATTR(precharge_current, S_IWUSR | S_IRUGO,
                bq2419x_show_precharge_current,
                bq2419x_set_precharge_current);
static DEVICE_ATTR(enable_itermination, S_IWUSR | S_IRUGO,
                bq2419x_show_enable_itermination,
                bq2419x_set_enable_itermination);
static DEVICE_ATTR(hot_enable_charger, S_IWUSR | S_IRUGO,
                bq2419x_show_hot_enable_charger,
                bq2419x_set_hot_enable_charger);
static DEVICE_ATTR(enable_charger, S_IWUSR | S_IRUGO,
                bq2419x_show_enable_charger,
                bq2419x_set_enable_charger);
static DEVICE_ATTR(enable_batfet, S_IWUSR | S_IRUGO,
                bq2419x_show_enable_batfet,
                bq2419x_set_enable_batfet);
static DEVICE_ATTR(enable_cd, S_IWUSR | S_IRUGO,
                bq2419x_show_enable_cd,
                bq2419x_set_enable_cd);
static DEVICE_ATTR(charging, S_IWUSR | S_IRUGO,
                NULL,
                bq2419x_set_charging);
static DEVICE_ATTR(wakelock_enable, S_IWUSR | S_IRUGO,
                bq2419x_show_wakelock_enable,
                bq2419x_set_wakelock_enable);
static DEVICE_ATTR(chargelog, S_IWUSR | S_IRUGO,
                bq2419x_show_chargelog,
                NULL);
static DEVICE_ATTR(calling_limit, S_IWUSR | S_IRUGO,
                bq2419x_show_calling_limit,
                bq2419x_set_calling_limit);
static DEVICE_ATTR(compesation_resistor, S_IWUSR | S_IRUGO,
                bq2419x_show_compesation_resistor,
                bq2419x_set_compesation_resistor);
static DEVICE_ATTR(compesation_voltage, S_IWUSR | S_IRUGO,
                bq2419x_show_compesation_voltage,
                bq2419x_set_compesation_voltage);
static DEVICE_ATTR(hot_limit_charging, S_IWUSR | S_IRUGO,
                bq2419x_show_hot_limit_temp_charging,
                bq2419x_set_hot_limit_temp_charging);
static DEVICE_ATTR(limit_charging, S_IWUSR | S_IRUGO,
                bq2419x_show_limit_temp_charging,
                bq2419x_set_limit_temp_charging);
static DEVICE_ATTR(coldhot_charging, S_IWUSR | S_IRUGO,
                bq2419x_show_coldhot_charging,
                bq2419x_set_coldhot_charging);
static DEVICE_ATTR(temperature_parameter, S_IWUSR | S_IRUGO,
                bq2419x_show_temperature_parameter,
                bq2419x_set_temperature_parameter);
static DEVICE_ATTR(set_current, S_IWUSR | S_IRUGO,
                NULL,
                bq2419x_set_current);

static struct attribute *bq2419x_attributes[] = {
    &dev_attr_enable_hz_mode.attr,
    &dev_attr_dppm_voltage.attr,
    &dev_attr_cin_limit.attr,
    &dev_attr_regulation_voltage.attr,
    &dev_attr_charge_current.attr,
    &dev_attr_precharge_current.attr,
    &dev_attr_termination_current.attr,
    &dev_attr_enable_itermination.attr,
    &dev_attr_hot_enable_charger.attr,
    &dev_attr_enable_charger.attr,
    &dev_attr_enable_batfet.attr,
    &dev_attr_enable_cd.attr,
    &dev_attr_charging.attr,
    &dev_attr_wakelock_enable.attr,
    &dev_attr_chargelog.attr,
    &dev_attr_calling_limit.attr,
    &dev_attr_compesation_resistor.attr,
    &dev_attr_compesation_voltage.attr,
    &dev_attr_hot_limit_charging.attr,
    &dev_attr_limit_charging.attr,
    &dev_attr_coldhot_charging.attr,
    &dev_attr_temperature_parameter.attr,
    &dev_attr_set_current.attr,
    NULL,
};

static const struct attribute_group bq2419x_attr_group = {
    .attrs = bq2419x_attributes,
};

static int __devinit bq2419x_charger_probe(struct i2c_client *client,
                           const struct i2c_device_id *id)
{
    struct bq2419x_device_info *di;
    struct bq2419x_platform_data *pdata = client->dev.platform_data;
    enum usb_charger_type plugin_stat = CHARGER_REMOVED;
    int ret = 0;
    u8 read_reg = 0;

    di = kzalloc(sizeof(*di), GFP_KERNEL);
    if (!di) {
        dev_err(&client->dev, "bq2419x_device_info is NULL!\n");
        return -ENOMEM;
    }

    di->dev = &client->dev;
    di->client = client;

    i2c_set_clientdata(client, di);

    ret = bq2419x_read_byte(di, &read_reg, PART_REVISION_REG0A);

    if (ret < 0) {
        dev_err(&client->dev, "chip not present at address %x\n",
                     client->addr);
        ret = -EINVAL;
        goto err_kfree;
    }

    if (((read_reg & 0x38) == BQ2419x || (read_reg & 0x38) == BQ24192)
         && (client->addr == 0x6B))
        di->bqchip_version = BQ24192;

    if (di->bqchip_version == 0) {
        dev_dbg(&client->dev, "unknown bq chip\n");
        dev_dbg(&client->dev, "Chip address %x", client->addr);
        dev_dbg(&client->dev, "bq chip version reg value %x", read_reg);
        ret = -EINVAL;
        goto err_kfree;
    }

     ret = bq2419x_get_boardid_charge_parameter(di);
     if(!ret){
        di->max_voltagemV = pdata->max_charger_voltagemV;
        di->max_currentmA = pdata->max_charger_currentmA;
        di->max_cin_currentmA = pdata->max_cin_limit_currentmA;
     }

    di->gpio = pdata->gpio;//ENABLE_BQ2419x_CHARGER;

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
    ret = gpio_request(ENABLE_BQ2419x_CHARGER, "gpio_074_cd");
    if (ret) {
          dev_err(&client->dev, "could not request irq\n");
          ret = -ENOMEM;
          goto err_io;
    }
     gpio_direction_output(ENABLE_BQ2419x_CHARGER, 0);

    di->voltagemV = di->max_voltagemV;
    di->currentmA = ICHG_MIN ;
    di->cin_dpmmV = VINDPM_4360;
    di->cin_limit = IINLIM_500;
    pre_cin_limit = IINLIM_500;
    di->sys_minmV = SYS_MIN_3500;
    di->prechrg_currentmA = IPRECHRG_256;
    di->term_currentmA = ITERM_MIN_128;
    di->watchdog_timer = WATCHDOG_40;
    di->chrg_timer = CHG_TIMER_8;
    di->bat_compohm = BAT_COMP_40;
    di->comp_vclampmV = VCLAMP_48;

    di->hz_mode = DIS_HIZ;
    di->chrg_config = EN_CHARGER;
    di->boost_lim = BOOST_LIM_500;
    di->enable_low_chg = DIS_FORCE_20PCT;
    di->enable_iterm = EN_TERM;
    di->enable_timer = EN_TIMER;
    di->enable_batfet = EN_BATFET;
    di->factory_flag = 0;
    di->hot_enable_charge_flag = 1;
    di->cd_active = 0;
    di->params.enable = 1;
    di->cfg_params = 1;
    di->enable_dpdm = DPDM_DIS;
    di->battery_full = 0;
    di->charge_full_count = 0;

    di->japan_charger = 0;
    di->is_two_stage_charger = 0;
    di->two_stage_charger_status = TWO_STAGE_CHARGE_FIRST_STAGE;
    di->first_stage_voltage = 4200;
    di->second_stage_voltage = 4100;
    di->is_disable_cool_temperature_charger = 0;
    di->high_temp_para =HIGH_TEMP_CP_U9701L;

    bq2419x_get_boardid_japan_charge_parameter(di);

    bq2419x_config_power_on_reg(di);
    bq2419x_config_current_reg(di);
    bq2419x_config_prechg_term_current_reg(di);
    bq2419x_config_voltage_reg(di);
    bq2419x_config_term_timer_reg(di);
    bq2419x_config_thernal_regulation_reg(di);
    bq2419x_config_limit_temperature_parameter(di);

    wake_lock_init(&chrg_lock, WAKE_LOCK_SUSPEND, "bq2419x_chrg_wakelock");

    /* Configuration parameters for 0C-10C, set the default to the parameters,
       and get the values from boardid files */
    di->design_capacity = BQ2419x_DEFAULT_CAPACITY;
    di->charge_in_temp_5 = DEFAULT_CHARGE_PARAM_LOW_TEMP;
    di->charge_in_temp_10 = DEFAULT_CHARGE_PARAM_LOW_TEMP;
    bq27510_get_gasgauge_normal_capacity(&(di->design_capacity));
    bq27510_get_gasgauge_param_temperature(&(di->charge_in_temp_5),
        &(di->charge_in_temp_10));
    dev_info(di->dev, "%s: capacity is %d, charge_in_temp_5 is %d,"
           "charge_in_temp_10 is %d.\n",__FUNCTION__,di->design_capacity,
           di->charge_in_temp_5,di->charge_in_temp_10);

    INIT_DELAYED_WORK_DEFERRABLE(&di->bq2419x_charger_work,
        bq2419x_charger_work);

    INIT_DELAYED_WORK_DEFERRABLE(&di->bq2419x_usb_otg_work,
        bq2419x_usb_otg_work);

    INIT_WORK(&di->usb_work, bq2419x_usb_charger_work);

    ret = sysfs_create_group(&client->dev.kobj, &bq2419x_attr_group);
    if (ret) {
        dev_dbg(&client->dev, "could not create sysfs files\n");
        goto err_sysfs;
    }

    di->nb.notifier_call = bq2419x_usb_notifier_call;

    ret = hiusb_charger_registe_notifier(&di->nb);
    if (ret < 0) {
       dev_err(&client->dev, "hiusb_charger_registe_notifier failed\n");
       goto err_hiusb;
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

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    /* detect current device successful, set the flag as present */
    set_hw_dev_flag(DEV_I2C_CHARGER);
#endif

    return 0;

err_hiusb:
    sysfs_remove_group(&client->dev.kobj, &bq2419x_attr_group);
err_sysfs:
    gpio_free(di->gpio);
    wake_lock_destroy(&chrg_lock);
err_io:
#ifdef CONFIG_GPIO_BAT
    if (blockmux_set(di->piomux_block, di->pblock_config, LOWPOWER))
    dev_err(&client->dev, "blockmux_set LOWPOWER failed\n");
#endif

err_kfree:
    kfree(di);
    di = NULL;

    return ret;
}

static int __devexit bq2419x_charger_remove(struct i2c_client *client)
{
    struct bq2419x_device_info *di = i2c_get_clientdata(client);

    hiusb_charger_unregiste_notifier(&di->nb);
    sysfs_remove_group(&client->dev.kobj, &bq2419x_attr_group);
    wake_lock_destroy(&chrg_lock);

    cancel_delayed_work(&di->bq2419x_charger_work);
    flush_scheduled_work();
    cancel_delayed_work_sync(&di->bq2419x_usb_otg_work);
    gpio_free(di->gpio);
#ifdef CONFIG_GPIO_BAT
    if (blockmux_set(di->piomux_block, di->pblock_config, LOWPOWER))
    dev_err(&client->dev, "blockmux_set LOWPOWER failed\n");
          /*blockmux_set(di->piomux_block_interrupt, di->pblock_config_interrupt, LOWPOWER);*/
#endif

    kfree(di);

    return 0;
}

static void bq2419x_charger_shutdown(struct i2c_client *client)
{
    struct bq2419x_device_info *di = i2c_get_clientdata(client);


    cancel_delayed_work(&di->bq2419x_charger_work);
    flush_scheduled_work();
    cancel_delayed_work_sync(&di->bq2419x_usb_otg_work);
#ifdef CONFIG_GPIO_BAT
    if (blockmux_set(di->piomux_block, di->pblock_config, LOWPOWER))
    dev_err(&client->dev, "blockmux_set LOWPOWER failed\n");
#endif

   return;
}

static const struct i2c_device_id bq2419x_id[] = {
    { "bq2419x_charger", 0 },
    {},
};

#ifdef CONFIG_PM
static int bq2419x_charger_suspend(struct i2c_client *client,
              pm_message_t state)
{
    struct bq2419x_device_info *di = i2c_get_clientdata(client);

    if(di->charger_source == POWER_SUPPLY_TYPE_MAINS){
        if(di->battery_full){
            if (!wake_lock_active(&chrg_lock)){
                cancel_delayed_work(&di->bq2419x_charger_work);
                if((wakeup_timer_seconds > 300) || !wakeup_timer_seconds)
                    wakeup_timer_seconds = 300;
            }
        }
    }

    bq2419x_config_power_on_reg(di);
    return 0;
}

static int bq2419x_charger_resume(struct i2c_client *client)
{
    struct bq2419x_device_info *di = i2c_get_clientdata(client);

    if(di->charger_source == POWER_SUPPLY_TYPE_MAINS){
        bq2419x_config_voltage_reg(di);
        schedule_delayed_work(&di->bq2419x_charger_work, msecs_to_jiffies(0));
    }

    bq2419x_config_power_on_reg(di);
   return 0;
}
#else
#define bq2419x_charger_suspend       NULL
#define bq2419x_charger_resume        NULL
#endif /* CONFIG_PM */

MODULE_DEVICE_TABLE(i2c, bq24192);
static struct i2c_driver bq2419x_charger_driver = {
    .probe = bq2419x_charger_probe,
    .remove = __devexit_p(bq2419x_charger_remove),
    .suspend = bq2419x_charger_suspend,
    .resume = bq2419x_charger_resume,
    .shutdown = bq2419x_charger_shutdown,
    .id_table = bq2419x_id,
    .driver = {
         .owner = THIS_MODULE,
         .name = "bq2419x_charger",
    },
};

static int __init bq2419x_charger_init(void)
{
    return i2c_add_driver(&bq2419x_charger_driver);
}
module_init(bq2419x_charger_init);

static void __exit bq2419x_charger_exit(void)
{
    i2c_del_driver(&bq2419x_charger_driver);
}
module_exit(bq2419x_charger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HW Inc");
