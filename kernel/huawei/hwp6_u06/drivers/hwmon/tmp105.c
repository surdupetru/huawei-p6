/*
 * tmp105.c
 *
 * TMP105 temperature sensor driver
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/slab.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

/* Registers */
#define                TMP105_TEMP_REG         0x00
#define                TMP105_CONF_REG         0x01
#define                TMP105_TLOW_REG         0x02
#define                TMP105_THIGH_REG        0x03

/* Configuration register parameters */
#define                TMP105_CONF_SD          0x01
#define                TMP105_CONF_TM          0x02
#define                TMP105_CONF_POL         0x04
#define                TMP105_CONF_F0          0x08
#define                TMP105_CONF_F1          0x10
#define                TMP105_CONF_R0          0x20
#define                TMP105_CONF_R1          0x40
#define                TMP105_CONF_OS          0x80

#define        TMP105_I2C_ADDRESS      0x48

#define         MAX_TEMP               128
#define         MIN_TEMP               -55

/* Each client has this additional data */
struct tmp105_data {
       struct i2c_client *client;
       /* mutex for sysfs operations */
       struct mutex lock;
       struct device *hwmon_dev;
       s16 temp[3];
       unsigned long last_updated;
       u8 configuration_setting;
};

static const u8 tmp105_reg[] = {
       TMP105_TEMP_REG,
       TMP105_TLOW_REG,
       TMP105_THIGH_REG,
};

static void tmp105_init_client(struct i2c_client *client);

static signed long tmp105_reg_to_mC(s16 val)
{
       signed long temp_mC;
       if (val  & 0x800)
               val = val - 0x1000 ;
        temp_mC = (val * 64000) / 1024;
        return temp_mC;
}

static u16 tmp105_C_to_reg(signed long val)
{
       val =  (val * 1024) / 64000;
       if (val < 0)
               val = val + 0x1000;
       return (u16)val;
}

static s16 *tmp105_update_device(struct i2c_client *client,
                                               int  index)
{
       struct tmp105_data *data = i2c_get_clientdata(client);
       u8 tmp[2];

       mutex_lock(&data->lock);

       if (time_after(jiffies, data->last_updated + HZ/4)) {
               i2c_smbus_read_i2c_block_data(client,
                                               tmp105_reg[index], 2, tmp);
               data->temp[index] = ((tmp[0] << 4) | ((tmp[1] & 0xF0) >> 4));
               printk(KERN_INFO "Raw temperature: %u\n", data->temp[index]);
               data->last_updated = jiffies;
       }

       mutex_unlock(&data->lock);
       return data->temp[index] ;
}

static ssize_t show_temp_value(struct device *dev,
                              struct device_attribute *devattr, char *buf)
{
       struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
       struct i2c_client *client = to_i2c_client(dev);
       s16 temperature = tmp105_update_device(client , sda->index);
       signed long temp_in_mC;

       temp_in_mC = tmp105_reg_to_mC(temperature);

       return sprintf(buf, "%d\n", temp_in_mC);
}

static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_temp_value, NULL , 0);

static ssize_t tmp105_set_temp(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
       struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
       struct i2c_client *client = to_i2c_client(dev);
       struct tmp105_data *tmp105 = i2c_get_clientdata(client);
       signed long val;
       int status = 0;
       u16 temp;

       if ((strict_strtol(buf, 10, &val) < 0))
               return -EINVAL;

       SENSORS_LIMIT(val , MIN_TEMP , MAX_TEMP);

       mutex_lock(&tmp105->lock);

       temp = tmp105_C_to_reg(val);
       temp = ((temp & 0xFF0) >> 4) | ((temp & 0xF)<<12);

       status = i2c_smbus_write_word_data(client, tmp105_reg[sda->index],
                       temp);

       tmp105->temp[sda->index] = temp;
       mutex_unlock(&tmp105->lock);
       return status ? : count;
}

static SENSOR_DEVICE_ATTR(temp1_min, S_IWUSR | S_IRUGO, show_temp_value,
       tmp105_set_temp, 1);
static SENSOR_DEVICE_ATTR(temp1_max, S_IWUSR | S_IRUGO, show_temp_value,
       tmp105_set_temp, 2);

/* sysfs call */
static ssize_t set_configuration(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
       s32 status;
       struct i2c_client *client = to_i2c_client(dev);
       struct tmp105_data *data = i2c_get_clientdata(client);
       data->configuration_setting = simple_strtoul(buf, NULL, 10);
       /* I2C write to the configuration register */
       status = i2c_smbus_write_byte_data(client, TMP105_CONF_REG,
                       data->configuration_setting);
       return count;
}

static ssize_t show_configuration(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
       struct i2c_client *client = to_i2c_client(dev);
       struct tmp105_data *data = i2c_get_clientdata(client);
       u8 tmp;
       i2c_smbus_read_i2c_block_data(client, TMP105_CONF_REG, 1, &tmp);
       data->configuration_setting = tmp;
       return sprintf(buf, "%u\n", data->configuration_setting);
}
static DEVICE_ATTR(configuration, S_IWUSR | S_IRUGO, show_configuration,
               set_configuration);


static struct attribute *tmp105_attributes[] = {
       &dev_attr_configuration.attr,
       &sensor_dev_attr_temp1_input.dev_attr.attr,
       &sensor_dev_attr_temp1_min.dev_attr.attr,
       &sensor_dev_attr_temp1_max.dev_attr.attr,
       NULL
};

static const struct attribute_group tmp105_attr_group = {
       .attrs = tmp105_attributes,
};

static int tmp105_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
       struct tmp105_data *tmp105_data;
       int err;

       if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
               dev_dbg(&client->dev, "adapter doesn't support I2C\n");
               return -ENODEV;
       }

       tmp105_data = kzalloc(sizeof(struct tmp105_data), GFP_KERNEL);
       if (!tmp105_data) {
               err = -ENOMEM;
               goto exit;
       }
       tmp105_data->client = client;

       i2c_set_clientdata(client, tmp105_data);
       mutex_init(&tmp105_data->lock);

       /* Initialize the TMP105 chip */
       tmp105_init_client(client);

       /* Register sysfs hooks */
       err = sysfs_create_group(&client->dev.kobj, &tmp105_attr_group);
       if (err)
               goto exit_free;
       tmp105_data->hwmon_dev = hwmon_device_register(&client->dev);
       if (IS_ERR(tmp105_data->hwmon_dev)) {
               err = PTR_ERR(tmp105_data->hwmon_dev);
               tmp105_data->hwmon_dev = NULL;
               goto exit_remove;
       }
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    /* detect current device successful, set the flag as present */
        set_hw_dev_flag(DEV_I2C_NTC);
#endif
       return 0;

exit_remove:
       sysfs_remove_group(&client->dev.kobj, &tmp105_attr_group);
exit_free:
       i2c_set_clientdata(client, NULL);
       kfree(tmp105_data);
exit:
       return err;
}

static int tmp105_remove(struct i2c_client *client)
{
       struct tmp105_data *tmp105 = i2c_get_clientdata(client);
       hwmon_device_unregister(tmp105->hwmon_dev);

       sysfs_remove_group(&client->dev.kobj, &tmp105_attr_group);
       i2c_set_clientdata(client, NULL);
       kfree(tmp105);
       return 0;
}

/* Called when we have found a new TMP105. */
static void tmp105_init_client(struct i2c_client *client)
{
       struct tmp105_data *data = i2c_get_clientdata(client);
       data->last_updated = jiffies - HZ;
       mutex_init(&data->lock);
}

static const struct i2c_device_id tmp105_id[] = {
       { "tmp105", 0 },
       { }
};

#ifdef CONFIG_PM
static int tmp105_suspend(struct device *dev)
{
       struct i2c_client *client = to_i2c_client(dev);
       u8 config_reg;
       i2c_smbus_read_i2c_block_data(client, TMP105_CONF_REG, 1, &config_reg);
       config_reg = config_reg | TMP105_CONF_SD;
       i2c_smbus_write_byte_data(client, TMP105_CONF_REG, TMP105_CONF_SD);
       return 0;
}

static int tmp105_resume(struct device *dev)
{
       struct i2c_client *client = to_i2c_client(dev);
       u8 config_reg;
       i2c_smbus_read_i2c_block_data(client, TMP105_CONF_REG, 1, &config_reg);
       config_reg = config_reg & ~TMP105_CONF_SD;
       i2c_smbus_write_byte_data(client, TMP105_CONF_REG, TMP105_CONF_SD);
}

static struct dev_pm_ops tmp105_dev_pm_ops = {
       .suspend = tmp105_suspend,
       .resume = tmp105_resume,
};

#define TMP105_DEV_PM_OPS (&tmp105_dev_pm_ops)
#else
#define TMP105_DEV_PM_OPS NULL
#endif /* CONFIG_PM */


static struct i2c_driver tmp105_driver = {
       .driver = {
               .name   = "tmp105",
               .owner = THIS_MODULE,
               .pm = TMP105_DEV_PM_OPS,
       },
       .probe          = tmp105_probe,
       .remove         = tmp105_remove,
       .id_table       = tmp105_id,
       .class = I2C_CLASS_HWMON,
};

static int __init tmp105_init(void)
{
       return i2c_add_driver(&tmp105_driver);
}

static void __exit tmp105_exit(void)
{
       i2c_del_driver(&tmp105_driver);
}

MODULE_DESCRIPTION("TMP105 driver");
MODULE_LICENSE("GPL");

module_init(tmp105_init);
module_exit(tmp105_exit);
