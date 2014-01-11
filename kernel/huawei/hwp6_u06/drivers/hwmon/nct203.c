/*
 * Driver for NCT203, temperature monitoring device from ON Semiconductors
 *
 * Copyright (c) 2012, Huawei Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */


#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/gpio.h>

#include <linux/nct203.h>
#include <linux/hwmon.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

#define DRIVER_NAME "nct203"
#define DRIVER_NAME_SLAVE "nct203_slave"

/* Register Addresses */
#define LOCAL_TEMP_RD                  0x00
//#define EXT_HI_TEMP_RD                 0x01
//#define EXT_LO_TEMP_RD                 0x10
#define STATUS_RD                      0x02
#define CONFIG_RD                      0x03

#define CONFIG_WR                      0x09
#define CONV_RATE_WR                   0x0A
#define LOCAL_TEMP_HI_LIMIT_WR         0x0B
#define LOCAL_TEMP_HI_LIMIT_RD         0x05
//#define EXT_TEMP_HI_LIMIT_HI_BYTE      0x0D
//#define OFFSET_WR                      0x11
//#define EXT_THERM_LIMIT_WR             0x19
#define LOCAL_THERM_LIMIT_WR           0x20
#define THERM_HYSTERESIS_WR            0x21

/* Configuration Register Bits */
#define EXTENDED_RANGE_BIT             (0x1 << 2)
#define THERM2_BIT                     (0x1 << 5)
#define STANDBY_BIT                    (0x1 << 6)

/* Max Temperature Measurements */
#define EXTENDED_RANGE_OFFSET          64U
#define STANDARD_RANGE_MAX             127U
#define EXTENDED_RANGE_MAX             (150U + EXTENDED_RANGE_OFFSET)

#define NCT_CHECK_TIMES                2

static inline u8 temperature_to_value(bool extended, u8 temp);

struct nct203_data {
       struct device *hwmon_dev;
       struct work_struct work;
       struct i2c_client *client;
       struct mutex mutex;
       u8 config;
       void (*alarm_fn)(bool raised);
};

static struct nct203_data *report_data;

static ssize_t nct203_show_temp(struct device *dev,
       struct device_attribute *attr, char *buf)
{
       struct i2c_client *client = to_i2c_client(dev);
       signed int temp_value = 0;
       u8 data = 0;

       if (!dev || !buf || !attr)
               return -EINVAL;

       data = i2c_smbus_read_byte_data(client, LOCAL_TEMP_RD);
       if (data < 0) {
               dev_err(&client->dev, "%s: failed to read "
                       "temperature\n", __func__);
               return -EINVAL;
       }

       temp_value = (signed int)data;
       return sprintf(buf, "%d\n", temp_value);
}

int nct203_temp_report(void)
{
       signed int temp_value = 0;
       u8 data = 0;
       static int check_times = 0;
       static signed int store_value = 25;

       
	   if (report_data == NULL)
	   	  return temp_value;
          
       check_times ++;
       if (check_times <= NCT_CHECK_TIMES)
       {
           return store_value;
       }
         
       check_times = 0;
       data = i2c_smbus_read_byte_data(report_data->client, LOCAL_TEMP_RD);
       if (data < 0) {
               dev_err(report_data->client, "%s: failed to read "
                       "temperature\n", __func__);
               return -EINVAL;
       }

       temp_value = (signed int)data;
       store_value = temp_value;
       return temp_value;
}
EXPORT_SYMBOL(nct203_temp_report);

static ssize_t nct203_store_throttle_limit(struct device *dev,
       struct device_attribute *attr, const char *buf, size_t count)
{
       struct i2c_client *client = to_i2c_client(dev);
       struct nct203_platform_data *pdata = client->dev.platform_data;
       unsigned long res;
       u8 ext_limit;
       u8 value;
       int err;

       err = strict_strtoul(buf, 0, &res);
       if (err)
               return -EINVAL;

       ext_limit = (u8)res;

       /* External Temperature Throttling limit */
       value = temperature_to_value(pdata->ext_range, ext_limit);
       err = i2c_smbus_write_byte_data(client,
                                       LOCAL_THERM_LIMIT_WR, value);
       if (err < 0)
               return -EINVAL;

       pdata->throttling_limit = ext_limit;
       return count;
}

static ssize_t nct203_show_throttle_limit(struct device *dev,
       struct device_attribute *attr, char *buf)
{
       struct i2c_client *client = to_i2c_client(dev);
       struct nct203_platform_data *pdata = client->dev.platform_data;
       u8 ext_limit = 0;

       if (!dev || !buf || !attr)
               return -EINVAL;

       ext_limit = pdata->throttling_limit;

       return sprintf(buf, "%d\n", ext_limit);
}

static ssize_t nct203_store_alert_limit(struct device *dev,
       struct device_attribute *attr, const char *buf, size_t count)
{
       struct i2c_client *client = to_i2c_client(dev);
       struct nct203_platform_data *pdata = client->dev.platform_data;
       unsigned long res;
       u8 alert_limit;
       u8 value;
       int err;

       err = strict_strtoul(buf, 0, &res);
       if (err)
               return -EINVAL;

       alert_limit = (u8)res;

       /* External Temperature Throttling limit */
       value = temperature_to_value(pdata->ext_range, alert_limit);
       err = i2c_smbus_write_byte_data(client,
                                       LOCAL_TEMP_HI_LIMIT_WR, value);
       if (err < 0)
               return -EINVAL;

       pdata->alert_limit = alert_limit;
       return count;
}

static ssize_t nct203_show_alert_limit(struct device *dev,
       struct device_attribute *attr, char *buf)
{
       struct i2c_client *client = to_i2c_client(dev);
       struct nct203_platform_data *pdata = client->dev.platform_data;
       u8 alert_limit = 0;

       if (!dev || !buf || !attr)
               return -EINVAL;

       alert_limit = pdata->alert_limit;

       return sprintf(buf, "%d\n", alert_limit);

}
static ssize_t nct203_read_debug_temp(struct device *dev,
       struct device_attribute *attr, const char *buf, size_t count)
{
       struct i2c_client *client = to_i2c_client(dev);
       struct nct203_platform_data *pdata = client->dev.platform_data;
       unsigned long res;
       int  debug_temp;
       int err;

       err = strict_strtoul(buf, 0, &res);
       if (err)
               return -EINVAL;

       debug_temp = (int)res;

       pdata->debug_temp = debug_temp;
       return count;
}

static ssize_t nct203_write_debug_temp(struct device *dev,
       struct device_attribute *attr, char *buf)
{
       struct i2c_client *client = to_i2c_client(dev);
       struct nct203_platform_data *pdata = client->dev.platform_data;
       int debug_temp = 0;

       if (!dev || !buf || !attr)
               return -EINVAL;

       debug_temp = pdata->debug_temp;

       return sprintf(buf, "%d\n", debug_temp);

}

static DEVICE_ATTR(temp, S_IRUGO, nct203_show_temp, NULL);
static DEVICE_ATTR(throttle_limit, S_IRUGO | S_IWUSR,
                  nct203_show_throttle_limit,
                  nct203_store_throttle_limit);
static DEVICE_ATTR(alert_limit, S_IRUGO | S_IWUSR,
                  nct203_show_alert_limit,
                  nct203_store_alert_limit);
static DEVICE_ATTR(debug_temp, S_IRUGO | S_IWUSR,
                  nct203_write_debug_temp,
                  nct203_read_debug_temp);

static struct attribute *nct203_attributes[] = {
       &dev_attr_temp.attr,
       &dev_attr_throttle_limit.attr,
       &dev_attr_alert_limit.attr,
       &dev_attr_debug_temp.attr,
       NULL
};

static const struct attribute_group nct203_attr_group = {
       .attrs = nct203_attributes,
};

static void nct203_enable(struct i2c_client *client)
{
       struct nct203_data *data = i2c_get_clientdata(client);

       i2c_smbus_write_byte_data(client, CONFIG_WR,
                                 data->config & ~STANDBY_BIT);
}

static void nct203_disable(struct i2c_client *client)
{
       struct nct203_data *data = i2c_get_clientdata(client);
       u8 value = 0;

       i2c_smbus_write_byte_data(client, CONFIG_WR,
                                 data->config | STANDBY_BIT);

        value = i2c_smbus_read_byte_data(client, CONFIG_WR);
        printk("[%s] value :%d\n", __func__,value);
        if( value < 0 || !(value&STANDBY_BIT))
        {
            i2c_smbus_write_byte_data(client, CONFIG_WR,
                                 data->config | STANDBY_BIT);
        }

}


static void nct203_work_func(struct work_struct *work)
{
       struct nct203_data *data =
               container_of(work, struct nct203_data, work);
       int irq = data->client->irq;

       mutex_lock(&data->mutex);

       if (data->alarm_fn) {
               /* Therm2 line is active low */
               data->alarm_fn(!gpio_get_value(irq_to_gpio(irq)));
       }

       mutex_unlock(&data->mutex);
}

static irqreturn_t nct203_irq(int irq, void *dev_id)
{
       struct nct203_data *data = dev_id;
       schedule_work(&data->work);

       return IRQ_HANDLED;
}

static inline u8 value_to_temperature(bool extended, u8 value)
{
       return extended ? (u8)(value - EXTENDED_RANGE_OFFSET) : value;
}

static inline u8 temperature_to_value(bool extended, u8 temp)
{
       return extended ? (u8)(temp + EXTENDED_RANGE_OFFSET) : temp;
}

static int __devinit nct203_configure_sensor(struct nct203_data* data)
{
       struct i2c_client *client           = data->client;
       struct nct203_platform_data *pdata = client->dev.platform_data;
       u8 value;
       int err;

       if (!pdata || !pdata->supported_hwrev)
               return -ENODEV;

       /*
        * Initial Configuration - device is placed in standby and
        * ALERT/THERM2 pin is configured as alert
        */
       data->config = value = pdata->ext_range ?
               (STANDBY_BIT | EXTENDED_RANGE_BIT) :
               (STANDBY_BIT);

       err = i2c_smbus_write_byte_data(client, CONFIG_WR, value);
       if (err < 0)
               goto error;

       /* Temperature conversion rate */
       err = i2c_smbus_write_byte_data(client, CONV_RATE_WR, pdata->conv_rate);
       if (err < 0)
               goto error;

       /* Local temperature h/w shutdown limit */
       value = temperature_to_value(pdata->ext_range,
                                    pdata->throttling_limit);
       err = i2c_smbus_write_byte_data(client, LOCAL_THERM_LIMIT_WR, value);
       if (err < 0)
               goto error;

       /* Local Temperature alert limit */
       value = temperature_to_value(pdata->ext_range,
                                    pdata->alert_limit);
       value = pdata->ext_range ? EXTENDED_RANGE_MAX : STANDARD_RANGE_MAX;
       err = i2c_smbus_write_byte_data(client, LOCAL_TEMP_HI_LIMIT_WR, value);
       if (err < 0)
               goto error;

       /* THERM hysteresis */
       err = i2c_smbus_write_byte_data(client, THERM_HYSTERESIS_WR,
                                       pdata->hysteresis);
       if (err < 0)
               goto error;

       data->alarm_fn = pdata->alarm_fn;
       return 0;
error:
       return err;
}

static int __devinit nct203_configure_irq(struct nct203_data *data)
{
       INIT_WORK(&data->work, nct203_work_func);

       return request_irq(data->client->irq, nct203_irq, IRQF_TRIGGER_RISING |
                               IRQF_TRIGGER_FALLING, DRIVER_NAME, data);
}

static int __devinit nct203_probe(struct i2c_client *client,
                                  const struct i2c_device_id *id)
{
       struct nct203_data *data;
       int err;

       data = kzalloc(sizeof(struct nct203_data), GFP_KERNEL);

       if (!data)
               return -ENOMEM;

       data->client = client;
       i2c_set_clientdata(client, data);
       mutex_init(&data->mutex);

       err = nct203_configure_sensor(data);   /* sensor is in standby */
       if (err < 0)
            goto error;

       data->hwmon_dev = hwmon_device_register(&client->dev);
       if (IS_ERR(data->hwmon_dev)) {
               err = PTR_ERR(data->hwmon_dev);
               dev_err(&client->dev, "%s: hwmon_device_register "
                       "failed\n", __func__);
               goto error;
       }

       /* register sysfs hooks */
       err = sysfs_create_group(&client->dev.kobj, &nct203_attr_group);
       if (err < 0)
               goto fail_sys;

        if(!strcmp(id->name,DRIVER_NAME))
        {
            nct203_enable(client);         /* sensor is running */

            report_data = data;
        }
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
       /* detect current device successful, set the flag as present */
       set_hw_dev_flag(DEV_I2C_NTC);
#endif
       return 0;

fail_sys:
       hwmon_device_unregister(data->hwmon_dev);
error:
       kfree(data);
       return err;
}

static int __devexit nct203_remove(struct i2c_client *client)
{
       struct nct203_data *data = i2c_get_clientdata(client);
       hwmon_device_unregister(data->hwmon_dev);
       sysfs_remove_group(&client->dev.kobj, &nct203_attr_group);
       kfree(data);

       return 0;
}

#ifdef CONFIG_PM
static int nct203_suspend(struct i2c_client *client, pm_message_t state)
{
       nct203_disable(client);

       return 0;
}

static int nct203_resume(struct i2c_client *client)
{
       nct203_enable(client);

       return 0;
}
#endif

static const struct i2c_device_id nct203_id[] = {
       { DRIVER_NAME, 0 },
       { }
};
MODULE_DEVICE_TABLE(i2c, nct203_id);

static const struct i2c_device_id nct203_slave_id[] = {
       { DRIVER_NAME_SLAVE, 0 },
       { }
};
MODULE_DEVICE_TABLE(i2c, nct203_slave_id);

static struct i2c_driver nct203_driver = {
       .driver = {
               .name   = DRIVER_NAME,
       },
       .probe          = nct203_probe,
       .remove         = __devexit_p(nct203_remove),
       .id_table       = nct203_id,
#ifdef CONFIG_PM
       .suspend        = nct203_suspend,
       .resume         = nct203_resume,
#endif
};

static struct i2c_driver nct203_slave_driver = {
       .driver = {
               .name   = DRIVER_NAME_SLAVE,
       },
       .probe          = nct203_probe,
       .remove         = __devexit_p(nct203_remove),
       .id_table       = nct203_slave_id,
#if 1//CONFIG_PM
//       .suspend        = nct203_suspend,
//       .resume         = nct203_resume,
#endif
};

static int __init nct203_init(void)
{
       return i2c_add_driver(&nct203_driver);
}

static int __init nct203_slave_init(void)
{
       return i2c_add_driver(&nct203_slave_driver);
}

static void __exit nct203_exit(void)
{
       i2c_del_driver(&nct203_driver);
}

static void __exit nct203_slave_exit(void)
{
       i2c_del_driver(&nct203_slave_driver);
}

MODULE_DESCRIPTION("Temperature sensor driver for OnSemi nct203");
MODULE_LICENSE("GPL");

module_init(nct203_init);
module_exit(nct203_exit);
module_init(nct203_slave_init);
module_exit(nct203_slave_exit);
