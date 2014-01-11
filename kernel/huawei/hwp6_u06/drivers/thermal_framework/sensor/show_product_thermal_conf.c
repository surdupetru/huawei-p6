


#include <linux/err.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/stddef.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <linux/thermal_framework.h>
#include <linux/temperature_sensor.h>
#include <hsad/config_mgr.h>

#define CONF_NAME_MAX   20
#define DEFAULT_CONF_NAME "thermal.conf"
struct thermal_config {
	struct platform_device *pdev;
	struct device *dev;
	struct thermal_dev *therm_fw;
};
static int get_product_thermal_confname(struct thermal_dev *tdev)
{
//     struct platform_device *pdev = to_platform_device(tdev->dev);
//     struct thermal_config *thermal_conf = platform_get_drvdata(pdev);
	return 0;
}
/*
 * sysfs hook functions
 */
static int read_thermal_conf_name(struct device *dev,
                                           struct device_attribute *devattr, char *buf)
{
 //      struct platform_device *pdev = to_platform_device(dev);
	char config_name[CONF_NAME_MAX] = {0};
	int ret;
	ret = get_hw_config_string("product_thermal_type/config_name",config_name,CONF_NAME_MAX,NULL);
	if (ret)
           return sprintf(buf, "%s", config_name);
       else
           return sprintf(buf, "%s", DEFAULT_CONF_NAME);
}

static DEVICE_ATTR(conf_name_input, S_IRUGO, read_thermal_conf_name, NULL);

static struct attribute *thermal_conf_name_attributes[] = {
	&dev_attr_conf_name_input.attr,
	NULL
};

static const struct attribute_group thermal_conf_name_group = {
	.attrs =thermal_conf_name_attributes,
};


static struct thermal_dev_ops thermal_conf_ops = {
	.report_temp = get_product_thermal_confname,
};

static int __devinit thermal_config_probe(struct platform_device *pdev)
{
	struct thermal_config_name_pdata *pdata = pdev->dev.platform_data;
	struct thermal_config *thermal_conf;
	int ret = 0;

	if (!pdata) 
        {
             dev_err(&pdev->dev, "%s: platform data missing\n", __func__);
             return -EINVAL;
	 }

	thermal_conf = kzalloc(sizeof(struct thermal_config), GFP_KERNEL);
	if (!thermal_conf)
           return -ENOMEM;

	thermal_conf->pdev = pdev;
	thermal_conf->dev = &pdev->dev;

	kobject_uevent(&pdev->dev.kobj, KOBJ_ADD);
	platform_set_drvdata(pdev, thermal_conf);

/*	thermal_conf->therm_fw = kzalloc(sizeof(struct thermal_dev), GFP_KERNEL);
	if (NULL == thermal_conf->therm_fw) {
              dev_err(&pdev->dev, "%s:Cannot alloc memory for thermal fw\n", __func__);
              ret = -ENOMEM;
              goto therm_fw_alloc_err;
	}

	thermal_conf->therm_fw->name = "thermal_conf";
	thermal_conf->therm_fw->domain_name = "cpu";
	thermal_conf->therm_fw->dev = thermal_conf->dev;
	thermal_conf->therm_fw->dev_ops = &thermal_conf_ops;
	thermal_sensor_dev_register(thermal_conf->therm_fw);  */

	ret = sysfs_create_group(&pdev->dev.kobj,&thermal_conf_name_group);
	if (ret) 
        {
              dev_err(&pdev->dev, "could not create sysfs files\n");
              goto sysfs_create_err;
	}

//	dev_info(&pdev->dev, "%s : '%s'\n", thermal_conf->therm_fw->name,pdata->name);

	return ret;

sysfs_create_err:
	thermal_sensor_dev_unregister(thermal_conf->therm_fw);
//	kfree(thermal_conf->therm_fw);
	platform_set_drvdata(pdev, NULL);
therm_fw_alloc_err:
	kfree(thermal_conf);
	return ret;
}

static int __devexit thermal_config_remove(struct platform_device *pdev)
{
	struct thermal_config *thermal_conf = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &thermal_conf_name_group);
	thermal_sensor_dev_unregister(thermal_conf->therm_fw);
//	kfree(thermal_conf->therm_fw);
	kobject_uevent(&thermal_conf->dev->kobj, KOBJ_REMOVE);
	platform_set_drvdata(pdev, NULL);
	kfree(thermal_conf);

	return 0;
}

static struct platform_driver thermal_config_driver = {
	.probe = thermal_config_probe,
	.remove = thermal_config_remove,
	.driver = {
            .name = "thermal_config_name",
	},
};

int __init thermal_config_name_init(void)
{
	return platform_driver_register(&thermal_config_driver);
}

static void __exit thermal_config_name_exit(void)
{
	platform_driver_unregister(&thermal_config_driver);
}

module_init(thermal_config_name_init);
module_exit(thermal_config_name_exit);


MODULE_LICENSE("GPL");

