/*
 * K3V2 Temperature sensor driver file
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 * Author: J Keerthy <j-keerthy@ti.com>
 * Author: Moiz Sonasath <m-sonasath@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
*/
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/stddef.h>
#include <linux/sysfs.h>
#include <linux/err.h>
#include <linux/reboot.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/thermal_framework.h>
#include <linux/temperature_sensor.h>


#define REPORT_DELAY_MS					1000
#define K3V2_CPU_TEMP_ADC_DDR_OFFSET	0xEC
#define K3V2_TEMP_START_VALUE				-40	/* -40 deg C */
#define K3V2_TEMP_END_VALUE				160	/* 160 deg C */
#define K3V2_ADC_START_VALUE				0
#define K3V2_ADC_END_VALUE				255
#define DRIVER_NAME  						temp

struct k3v2_temp_sensor {
	void __iomem		*tempio;
	struct resource	*res;
	struct platform_device	*pdev;
	struct device		*dev;
	struct thermal_dev		*therm_fw;
};


/*
 * Temperature values in milli degrees celsius ADC code values from 530 to 923
 */

static int adc_to_temp_conversion(int adc_val)
{
	int temp = 0;

	if ((0 > adc_val) || (adc_val > 255)) {
		pr_info("adc_to_temp_conversion failed \n\r ");
		return temp;
	}
	/*temp = (adc_val * (160 - (-40)  + 1))/(255 - 0 + 1);*/
	temp = (adc_val * (K3V2_TEMP_END_VALUE - K3V2_TEMP_START_VALUE  + 1))
			/ (K3V2_ADC_END_VALUE - K3V2_ADC_START_VALUE + 1);
	/*temp  = temp -40 -1*/
	temp = temp + K3V2_TEMP_START_VALUE - 1;
	return temp;
}


static int k3v2_read_current_temp(struct k3v2_temp_sensor *temp_sensor)
{
	int temp = 0;
	int temp_adc = 0;
	temp_adc = readl(temp_sensor->tempio + K3V2_CPU_TEMP_ADC_DDR_OFFSET);
	temp = adc_to_temp_conversion(temp_adc);

	return temp;
}

static int k3v2_get_temp(struct thermal_dev *tdev)
{
	struct platform_device *pdev = to_platform_device(tdev->dev);
	struct k3v2_temp_sensor *temp_sensor = platform_get_drvdata(pdev);

	temp_sensor->therm_fw->current_temp =
			k3v2_read_current_temp(temp_sensor);

	return temp_sensor->therm_fw->current_temp;
}

static int k3v2_report_temp(struct thermal_dev *tdev)
{
	struct platform_device *pdev = to_platform_device(tdev->dev);
	struct k3v2_temp_sensor *temp_sensor = platform_get_drvdata(pdev);
	int ret;

	temp_sensor->therm_fw->current_temp =
			k3v2_read_current_temp(temp_sensor);
	if (temp_sensor->therm_fw->current_temp != -EINVAL) {
		ret = thermal_sensor_set_temp(temp_sensor->therm_fw);
		if (ret == -ENODEV)
			pr_err("%s:thermal_sensor_set_temp reports error\n",
				__func__);
	}

	return temp_sensor->therm_fw->current_temp;
}

static int k3v2_temp_sensor_read_temp(struct device *dev,
				      struct device_attribute *devattr,
				      char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct k3v2_temp_sensor *temp_sensor = platform_get_drvdata(pdev);
	int temp = 0;

	temp = k3v2_read_current_temp(temp_sensor);

	return sprintf(buf, "%d\n", temp);
}




static DEVICE_ATTR(temp1_input, S_IRUGO, k3v2_temp_sensor_read_temp,
			  NULL);

static struct attribute *k3v2_temp_sensor_attributes[] = {
	&dev_attr_temp1_input.attr,
	NULL
};

static const struct attribute_group k3v2_temp_sensor_group = {
	.attrs = k3v2_temp_sensor_attributes,
};


static struct thermal_dev_ops k3v2_sensor_ops = {
	.report_temp = k3v2_get_temp,
};

static int __devinit k3v2_temp_sensor_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct k3v2_temp_sensor_pdata *pdata = pdev->dev.platform_data;
	struct k3v2_temp_sensor *temp_sensor;
	struct resource *res;

	int ret = 0;

	if (!pdata) {
		dev_err(&pdev->dev, "%s: platform data missing\n", __func__);
		return -EINVAL;
	}

	temp_sensor = kzalloc(sizeof(struct k3v2_temp_sensor), GFP_KERNEL);
	if (!temp_sensor) {
		dev_err(dev, "k3v2_temp_sensor apply memory space fail\n\r");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "cannot find register resource\n");
		ret = -EINVAL;
		goto clk_get_err;
	}

	temp_sensor->res = request_mem_region(res->start, resource_size(res), dev_name(dev));
	if (!temp_sensor->res) {
		dev_err(dev, "cannot reserve registers\n");
		ret = -ENOENT;
		goto clk_get_err;
	}

	temp_sensor->tempio = ioremap(res->start, resource_size(res));
	if (!temp_sensor->tempio) {
		dev_err(dev, "cannot map registers\n");
		ret = -ENXIO;
		goto err_mmio_res;
	}

	temp_sensor->pdev = pdev;
	temp_sensor->dev = &pdev->dev;

	kobject_uevent(&pdev->dev.kobj, KOBJ_ADD);
	platform_set_drvdata(pdev, temp_sensor);

	temp_sensor->therm_fw = kzalloc(sizeof(struct thermal_dev), GFP_KERNEL);
	if (NULL == temp_sensor->therm_fw) {
		dev_err(&pdev->dev, "%s:Cannot alloc memory for thermal fw\n",
			__func__);
		ret = -ENOMEM;
		goto err_mmio;
	}

	temp_sensor->therm_fw->name = "k3v2_ondie_sensor";
	temp_sensor->therm_fw->domain_name = "cpu";
	temp_sensor->therm_fw->dev = temp_sensor->dev;
	temp_sensor->therm_fw->dev_ops = &k3v2_sensor_ops;
	thermal_sensor_dev_register(temp_sensor->therm_fw);

	ret = sysfs_create_group(&pdev->dev.kobj,
				 &k3v2_temp_sensor_group);
	if (ret) {
		dev_err(&pdev->dev, "could not create sysfs files\n");
		goto sysfs_create_err;
	}
	k3v2_report_temp(temp_sensor->therm_fw);
	dev_info(&pdev->dev, "%s : '%s'\n", temp_sensor->therm_fw->name,
			pdata->name);

	return ret;

sysfs_create_err:
	thermal_sensor_dev_unregister(temp_sensor->therm_fw);
	kfree(temp_sensor->therm_fw);
	platform_set_drvdata(pdev, NULL);
err_mmio:
	iounmap(temp_sensor->tempio);
err_mmio_res:
	release_resource(temp_sensor->res);
	kfree(temp_sensor->res);
clk_get_err:
	pm_runtime_disable(&pdev->dev);
	kfree(temp_sensor);

	return ret;
}

static int __devexit k3v2_temp_sensor_remove(struct platform_device *pdev)
{
	struct k3v2_temp_sensor *temp_sensor = platform_get_drvdata(pdev);

	sysfs_remove_group(&temp_sensor->dev->kobj, &k3v2_temp_sensor_group);
	thermal_sensor_dev_unregister(temp_sensor->therm_fw);
	kfree(temp_sensor->therm_fw);
	kobject_uevent(&temp_sensor->dev->kobj, KOBJ_REMOVE);
	platform_set_drvdata(pdev, NULL);
	iounmap(temp_sensor->tempio);
	kfree(temp_sensor);
	return 0;
}

static struct platform_driver k3v2_temp_sensor_driver = {
	.probe = k3v2_temp_sensor_probe,
	.remove = k3v2_temp_sensor_remove,
	.driver = {
		.name = "k3v2_temp_sensor",
	},
};

int __init k3v2_temp_sensor_init(void)
{
	return platform_driver_register(&k3v2_temp_sensor_driver);
}

static void __exit k3v2_temp_sensor_exit(void)
{
	platform_driver_unregister(&k3v2_temp_sensor_driver);
}

module_init(k3v2_temp_sensor_init);
module_exit(k3v2_temp_sensor_exit);

MODULE_DESCRIPTION("K3V2 Temperature Sensor Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("Texas Instruments Inc");
