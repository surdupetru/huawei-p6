/*******************************************************************************
 * Copyright:   Copyright (c) 2011. Hisilicon Technologies, CO., LTD.
 * Version:     V200
 * Filename:    k3-adc-hwmon.c
 * Description: k3-adc-hwmon.c
 * History:     1.Created by 00186176 on 2011/08/02
*******************************************************************************/

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/hkadc/hiadc_linux.h>
#include <linux/hkadc/hiadc_hal.h>
#include <linux/semaphore.h>

struct k3_hwmon_attr {
	struct sensor_device_attribute	in;
	struct sensor_device_attribute	label;
	char				in_name[12];
	char				label_name[12];
};

/**
 * struct k3_hwmon - ADC hwmon client information
 * @lock: Access lock to serialise the conversions.
 * @client: The client we registered with the k3 ADC core.
 * @hwmon_dev: The hwmon device we created.
 * @attr: The holders for the channel attributes.
*/
struct k3_hwmon {
	struct semaphore	lock;
	struct k3_adc_client	*client;
	struct device		*hwmon_dev;

	struct k3_hwmon_attr	attrs[8];
};

/**
 * k3_hwmon_read_ch - read a value from a given adc channel.
 * @dev: The device.
 * @hwmon: Our state.
 * @channel: The channel we're reading from.
 *
 * Read a value from the @channel with the proper locking and sleep until
 * either the read completes or we timeout awaiting the ADC core to get
 * back to us.
 */
static int k3_hwmon_read_ch(struct device *dev,
			     struct k3_hwmon *hwmon, int channel)
{
	int ret   = 0;
	int value = 0;

	ret = down_interruptible(&hwmon->lock);
	if (ret < 0) {
		up(&hwmon->lock);
		return ret;
	}

	dev_dbg(dev, "reading channel %d\n", channel);

	ret = k3_adc_open_channel(channel);
	if (ret < 0) {
		dev_err(dev, "%s is error\n", __func__);
		up(&hwmon->lock);
		return ret;
	}
	if (channel == ADC_RTMP)
		msleep(10);
	value = k3_adc_get_value(channel, 0);

	ret = k3_adc_close_channal(channel);
	if (ret < 0) {
		dev_err(dev, "%s is error\n", __func__);
		up(&hwmon->lock);
		return ret;
	}

	up(&hwmon->lock);

	return value;
}

/**
 * k3_hwmon_show_raw - show a conversion from the raw channel number.
 * @dev: The device that the attribute belongs to.
 * @attr: The attribute being read.
 * @buf: The result buffer.
 *
 * This show deals with the raw attribute, registered for each possible
 * ADC channel. This does a conversion and returns the raw (un-scaled)
 * value returned from the hardware.
 */
static ssize_t k3_hwmon_show_raw(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct k3_hwmon *adc = platform_get_drvdata(to_platform_device(dev));
	struct sensor_device_attribute *sa = to_sensor_dev_attr(attr);

	if (adc == NULL) {
		dev_err(dev, "adc is null\n");
		return -ENODEV;
	}

	int ret = k3_hwmon_read_ch(dev, adc, sa->index);

	return  (ret < 0) ? ret : snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

#define DEF_ADC_ATTR(x)	\
	static SENSOR_DEVICE_ATTR(adc##x##_raw, S_IRUGO, k3_hwmon_show_raw, NULL, x)

DEF_ADC_ATTR(0);
DEF_ADC_ATTR(1);
DEF_ADC_ATTR(2);
DEF_ADC_ATTR(3);
DEF_ADC_ATTR(4);
DEF_ADC_ATTR(5);
DEF_ADC_ATTR(6);
DEF_ADC_ATTR(7);

static struct attribute *k3_hwmon_attrs[9] = {
	&sensor_dev_attr_adc0_raw.dev_attr.attr,
	&sensor_dev_attr_adc1_raw.dev_attr.attr,
	&sensor_dev_attr_adc2_raw.dev_attr.attr,
	&sensor_dev_attr_adc3_raw.dev_attr.attr,
	&sensor_dev_attr_adc4_raw.dev_attr.attr,
	&sensor_dev_attr_adc5_raw.dev_attr.attr,
	&sensor_dev_attr_adc6_raw.dev_attr.attr,
	&sensor_dev_attr_adc7_raw.dev_attr.attr,
};

static struct attribute_group k3_hwmon_attrgroup = {
	.attrs	= k3_hwmon_attrs,
};

static inline int k3_hwmon_add_raw(struct device *dev)
{
	return sysfs_create_group(&dev->kobj, &k3_hwmon_attrgroup);
}

static inline void k3_hwmon_remove_raw(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &k3_hwmon_attrgroup);
}


static int __devinit k3_hwmon_probe(struct platform_device *dev)
{
	struct k3_hwmon *hwmon = NULL;
	int ret = 0;

	hwmon = kzalloc(sizeof(struct k3_hwmon), GFP_KERNEL);
	if (hwmon == NULL) {
		dev_err(&dev->dev, "no memory\n");
		return -ENOMEM;
	}

	platform_set_drvdata(dev, hwmon);

	sema_init(&hwmon->lock, 1);

	/* add attributes for our adc devices. */
	ret = k3_hwmon_add_raw(&dev->dev);
	if (ret)
		goto err_registered;

	/* register with the hwmon core */
	hwmon->hwmon_dev = hwmon_device_register(&dev->dev);
	if (IS_ERR(hwmon->hwmon_dev)) {
		dev_err(&dev->dev, "error registering with hwmon\n");
		ret = PTR_ERR(hwmon->hwmon_dev);
		goto err_raw_attribute;
	}
	return 0;

 err_raw_attribute:
	k3_hwmon_remove_raw(&dev->dev);

 err_registered:

	kfree(hwmon);
	return ret;
}

static int __devexit k3_hwmon_remove(struct platform_device *dev)
{
	struct k3_hwmon *hwmon = platform_get_drvdata(dev);
	if (hwmon == NULL) {
		dev_err(&dev->dev, "hwmon is null\n");
		return -ENODEV;
	}
	k3_hwmon_remove_raw(&dev->dev);

	hwmon_device_unregister(hwmon->hwmon_dev);

	return 0;
}

static struct platform_driver k3_hwmon_driver = {
	.driver	= {
		.name		= "k3-hwmon",
		.owner		= THIS_MODULE,
	},
	.probe		= k3_hwmon_probe,
	.remove		= __devexit_p(k3_hwmon_remove),
};

static int __init k3_hwmon_init(void)
{
	return platform_driver_register(&k3_hwmon_driver);
}

static void __exit k3_hwmon_exit(void)
{
	platform_driver_unregister(&k3_hwmon_driver);
}

module_init(k3_hwmon_init);
module_exit(k3_hwmon_exit);

MODULE_AUTHOR("Huawei Technologies Co.,Ltd");
MODULE_DESCRIPTION("k3 ADC HWMon driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:k3-hwmon");
