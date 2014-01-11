/*
 * Copyright (C) huawei company
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under	the	terms of the GNU General Public	License	version	2 as
 * published by	the	Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/rmi.h>
#include <linux/cyttsp4_core.h>
#include "touch_info.h"

#ifdef CONFIG_P2_TP_TK_CMD_FEATURE
extern int rmi_get_glove_switch(u8 *status);
extern int rmi_set_glove_switch(u8 status);
#endif
extern int cyttsp4_get_glove_switch(u8 *status);
extern int cyttsp4_set_glove_switch(u8 status);

static char *touch_chip_info = NULL;
int set_touch_chip_info(const char *chip_info)
{
	if(chip_info == NULL) {
		pr_err("touch_chip_info = %s\n", chip_info);
		return -EINVAL;
	}
	touch_chip_info = (char *)chip_info;
	return 0;
}
EXPORT_SYMBOL(set_touch_chip_info);

static struct platform_device touch_input_info = {
	.name = "huawei_touch",
	.id = -1,
};

static ssize_t show_touch_chip_info(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (dev == NULL) {
		pr_err("touch_chip info is null\n");
		return -EINVAL;
	}

	return sprintf(buf, "%s\n", touch_chip_info);
}
static DEVICE_ATTR(touch_chip_info, 0664,
				   show_touch_chip_info, NULL);

int tp_get_glove_switch(u8 *status)
{
	if(touch_chip_info == NULL) {
		pr_err("%s: touch_chip info is null\n", __func__);
		return -EINVAL;
	}
	if (status == NULL) {
		pr_err("%s: status is null\n", __func__);
		return -EINVAL;
	}

	if (0 == strcmp(touch_chip_info, TOUCH_INFO_RMI3250) ) {  //rmi 3250
		pr_info("rmi_get_glove_switch is called\n");
#ifdef CONFIG_P2_TP_TK_CMD_FEATURE
		return rmi_get_glove_switch(status);
#else
		return 0;
#endif
	} else if (0 == strcmp(touch_chip_info, TOUCH_INFO_RMI7020) ) {
		pr_info("touch_chip_info = TOUCH_INFO_RMI7020\n");
		return 0;
	} else if (0 == strcmp(touch_chip_info, TOUCH_INFO_CYPRESS) ) { //cypress
		pr_info("cyttsp4_get_glove_switch is called\n");
		return cyttsp4_get_glove_switch(status);
	} else {
		pr_err("%s: invalid touch_chip_info\n", __func__);
		return -EINVAL;
	}
}
int tp_set_glove_switch(u8 status)
{
	if( touch_chip_info == NULL) {
		pr_err("%s: touch_chip info is null\n", __func__);
		return -EINVAL;
	}

	if (0 == strcmp(touch_chip_info, TOUCH_INFO_RMI3250) ) {  //rmi 3250
		pr_info("rmi_set_glove_switch is called\n");
#ifdef CONFIG_P2_TP_TK_CMD_FEATURE
		return rmi_set_glove_switch(status);
#else
		return 0;
#endif
	} else if (0 == strcmp(touch_chip_info, TOUCH_INFO_RMI7020) ) { //rmi 7020
		pr_info("touch_chip_info = TOUCH_INFO_RMI7020\n");
		return 0;
	}else if (0 == strcmp(touch_chip_info, TOUCH_INFO_CYPRESS) ) { //cypress
		pr_info("cyttsp4_set_glove_switch is called\n");
		return cyttsp4_set_glove_switch(status);
	} else {
		pr_err("%s: invalid touch_chip_info\n", __func__);
		return -EINVAL;
	}
}

static ssize_t show_touch_glove(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u8 touch_glove_switch = 0;

	pr_info("%s called\n", __func__);

	if (dev == NULL) {
		pr_err("%s dev is null\n", __func__);
		return -EINVAL;
	}
	if (0 == tp_get_glove_switch(&touch_glove_switch)) {
		return sprintf(buf, "%d\n", touch_glove_switch);
	} else {
		pr_err("%s tp_get_glove_switch failed\n", __func__);
		return -EFAULT;
	}
}
static ssize_t store_touch_glove(struct device *dev,
				struct device_attribute *attr, char *buf, size_t count)
{
	u8 value;

	pr_info("%s called\n", __func__);

	if (dev == NULL) {
		pr_err("%s dev is null\n", __func__);
		return -EINVAL;
	}
	if (1 != sscanf(buf, "%d", &value)) {
		pr_err("%s: failed to store\n", __func__);
		return -EINVAL;
	}

	if ((value == 0)  || (value == 1)) {
		if ( tp_set_glove_switch(value) < 0) {
			pr_err("%s: tp_set_glove_switch failed\n", __func__);
			return -EFAULT;
		}
	} else {
		pr_err("%s, value illegal[value=%d]\n", __func__, value);
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR(touch_glove, 0664,
				   show_touch_glove, store_touch_glove);

static struct attribute *touch_input_attributes[] = {
	&dev_attr_touch_chip_info.attr,
	&dev_attr_touch_glove.attr,
	NULL
};
static const struct attribute_group touch_input = {
	.attrs = touch_input_attributes,
};
static int __init touch_input_info_init(void)
{
	int ret = 0;
	printk(KERN_INFO"[%s] ++", __func__);

	ret = platform_device_register(&touch_input_info);
	if (ret) {
		pr_err("%s: platform_device_register failed, ret:%d.\n",
				__func__, ret);
		goto REGISTER_ERR;
	}

	ret = sysfs_create_group(&touch_input_info.dev.kobj, &touch_input);
	if (ret) {
		pr_err("touch_input_info_init sysfs_create_group error ret =%d", ret);
		goto SYSFS_CREATE_CGOUP_ERR;
	}
	printk(KERN_INFO"[%s] --", __func__);

	return 0;
SYSFS_CREATE_CGOUP_ERR:
	platform_device_unregister(&touch_input_info);
REGISTER_ERR:
	return ret;

}

device_initcall(touch_input_info_init);
MODULE_DESCRIPTION("touch input info");
MODULE_AUTHOR("huawei driver group of k3v2");
MODULE_LICENSE("GPL");
