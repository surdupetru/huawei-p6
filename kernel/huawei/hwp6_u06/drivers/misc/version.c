#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/hkadc/hiadc_hal.h>
#include <mach/gpio.h>
#include <mach/k3_version.h>
#include <linux/stat.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

static struct s10_gpio_version_platdata  *version_gpio_pdata;
static struct kobject *kobj = NULL;
/* save the whole version string */
char hw_version[HW_VERSION_LEN + 1] ;
/* gpio1 represents GPIO12(u4pin);gpio2 represents GPIO13(u5pin) */
#ifndef CONFIG_GET_HW_VERSION_FROM_CMDLINE
static int gpio1;
static int gpio2;
#endif
/* we can control the print switch by /sys/module/version_s10/parameters/version_dbg_switch */
static int version_debug_mask = 1;
module_param_named(version_dbg_switch, version_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define VERSION_DBG(x...) do {\
if (version_debug_mask) \
        printk(KERN_DEBUG x);\
} while (0);

/* hardware minor version string definition */
static const char *minor_version[] = 
{
"ESVA",
"CSVA",
"CSVB",
"CSVD",
"Unknown"
};
/* hardware major version range */
static const struct adc_mv_to_versionidx_map_type mv_to_versionidx_map[]  = 
{
    {  0,   0 }, 
    {  381,   0 }, 
    {  807,   0 }, 
    {  1185,   0 }, 
    {  1573,   0 }, 
    {  1973,   0 }, 
    {  2395,   0 }, 
    {  3300,   0 }, 
    {  0,   381 }, 
    {  381,   381 }, 
    {  807,   381 }, 
    {  1185,   381 }, 
    {  1573,   381 }, 
    {  1973,   381 }, 
    {  2395,   381 }, 
    {  3300,   381 }, 
    {  0,   807 }, 
    {  381,   807 }, 
    {  807,   807 }, 
    {  1185,   807 }, 
    {  1573,   807 }, 
    {  1973,   807 }, 
    {  2395,   807 }, 
    {  3300,   807 }, 
    {  0,   1185 }, 
    {  381,   1185 }, 
    {  807,   1185 }, 
    {  1185,   1185 }, 
    {  1573,   1185 }, 
    {  1973,   1185 }, 
    {  2395,   1185 }, 
    {  3300,   1185 }, 
    {  0,   1573 }, 
    {  381,   1573 }, 
    {  807,   1573 }, 
    {  1185,   1573 }, 
    {  1573,   1573 }, 
    {  1973,   1573 }, 
    {  2395,   1573 }, 
    {  3300,   1573 }, 
    {  0,   1973 }, 
    {  381,   1973 }, 
    {  807,   1973 }, 
    {  1185,   1973 }, 
    {  1573,   1973 }, 
    {  1973,   1973 }, 
    {  2395,   1973 }, 
    {  3300,   1973 }, 
    {  0,   2395 }, 
    {  381,   2395 },
    {  807,   2395 },
    {  1185,   2395 },
    {  1573,   2395 },
    {  1973,   2395 },
    {  2395,   2395 },
    {  3300,   2395 },
    {  0,   3300 },
    {  381,   3300 },
    {  807,   3300 },
    {  1185,   3300 },
    {  1573,   3300 },
    {  1973,   3300 },
    {  2395,   3300 },
    {  3300,   3300 },    
    {  -1    ,  -1     }
};

#if 0   // old hardwrae major version
/* hardware major version string definition */
static const char* major_version[] =
{
     "STEN101uV0001", "STEN101uV0002", "STEN101uV0003", "STEN101uV0004", "STEN101uV0005", "STEN101uV0006", "STEN102uV0001", "STEN102uV0002",
     "STEN102uV0003", "STEN102uV0004", "STEN102uV0005", "STEN102uV0006", "STEN101wV0001", "STEN101wV0002", "STEN101wV0003", "STEN101wV0004",
     "STEN101wV0005", "STEN101wV0006", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD",
     "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD",
     "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD",
     "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD",
     "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TEMP0001", "TEMP0002",
     "TEMP0003", "TEMP0004", "TEMP0005", "TEMP0006", "TEMP0007", "TEMP0008", "TEMP0009", "TEMP0010","Unknown"
};
#else   // new hardwrae major version
static const char* major_version[] =
{
    "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD",
    "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD",
    "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD", "TBD",
    "TBD", "TBD", "TBD", "TBD", "STEN101wfAPV0004", "STEN101wfAPV0003", "STEN101wfAPV0002", "STEN101wfAPV0001",
    "TBD", "TBD", "TBD", "TBD", "STEN101LAPV0004", "STEN101LAPV0003", "STEN101LAPV0002", "STEN101LAPV0001",
    "TBD", "TBD", "TBD", "TBD", "STEN101wAPV0004", "STEN101wAPV0003", "STEN101wAPV0002", "STEN101wAPV0001",
    "TBD", "TBD", "TBD", "TBD", "STEN102uAPV0004", "STEN102uAPV0003", "STEN102uAPV0002", "STEN102uAPV0001",
    "TBD", "TBD", "TBD", "TBD", "STEN101uAPV0004", "STEN101uAPV0003", "STEN101uAPV0002", "STEN101uAPV0001",

};
#endif

/* definition of attribute in sys filesystem */
static ssize_t hw_version_attr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", hw_version);
}
static struct kobj_attribute hw_version_attribute =
     __ATTR(hw_version, 0444, hw_version_attr_show, NULL);

static struct attribute* version_attributes[] =
{
     &hw_version_attribute.attr,
     NULL
};
static struct attribute_group version_attr_group =
{
     .attrs = version_attributes,
};

#ifndef CONFIG_GET_HW_VERSION_FROM_CMDLINE

/* getting the minor version by gpio12 and gpio13 */
static int hw_minor_version_get(void)
{
    int gpio_val1;
    int gpio_val2;
    int index = 0;

    /* get the gpio12 and gpio13's value */
    gpio_val1 = gpio_get_value(gpio1);
    gpio_val2 = gpio_get_value(gpio2);

    /* 00 ->ESVA, 01 ->CSVA,  10 ->CSVB,  11 ->CSVD */
    VERSION_DBG("%s:gpio_val1=%d, gpio_val2=%d\n", __func__, gpio_val1, gpio_val2);
    if (!gpio_val1 && !gpio_val2)
    {
        index = 0;
    }
    else if (gpio_val1 == 1 && !gpio_val2)
    {
        index = 1;
    }
	else if (!gpio_val1 && gpio_val2 == 1)
    {
        index = 2;
    }
    else if (gpio_val1 ==1 && gpio_val2 == 1)
    {
        index = 3;
    }
    else
    {
        index = -1;
    }
    return index;
	
}

/* getting the major version by channel number */
static int  hw_major_version_get(int channel_no)
{
    unsigned char reserve = 0;
    int value = 0;    
    int ret = 0;

    /* open adc channel */
    ret = k3_adc_open_channel(channel_no);
    if (ret < 0)
    {
        printk(KERN_ERR "%s:open adc channel failed(ret=%d channel_no=%d)\n", __func__, ret, channel_no);
	 return ret;
    }
    /* get the volatge value by adc channel */
    value = k3_adc_get_value(channel_no, &reserve);
    /* close the adc channel */
    ret = k3_adc_close_channal(channel_no);
    if (ret < 0)
    {
        printk(KERN_ERR "%s:close adc channel failed(ret=%d channel_no=%d)\n", __func__, ret, channel_no);
	 return ret;
    }
    return value;
}


static int hw_version_get(void)
{
    int version1, version2, index=0;
    int minor_veridx;
    memset(hw_version, 0, strlen(hw_version));

    /* getting in1 and in2 channels' voltage value  */
    version1 = hw_major_version_get(ADC_ADCIN1);
    version2 = hw_major_version_get(ADC_ADCIN2);
    VERSION_DBG("%s:version1=%d version2=%d\n", __func__, version1, version2);
    if (version1 < 0 || version2 < 0)
    {
        printk(KERN_ERR "%s:get major version failed(ver1=%d,ver2=%d)\n",__func__, version1, version2);
        return -1;
    }
    /* lookup the string index by the voltage values */
    for (index = 0; index < VERSION_NUM; index++)
    {
        if ((version1 >= (mv_to_versionidx_map[index].mV1 - VOL_TOLERANCE)) &&
	      (version1 <= (mv_to_versionidx_map[index].mV1 + VOL_TOLERANCE)) &&
	      (version2 >= (mv_to_versionidx_map[index].mV2 - VOL_TOLERANCE)) &&
	      (version2 <= (mv_to_versionidx_map[index].mV2 + VOL_TOLERANCE)))
        {
            break;
        }
    }
    strcpy(hw_version, major_version[index]);
    minor_veridx = hw_minor_version_get();
    if (minor_veridx < 0)
    {
        printk(KERN_ERR "%s:get minor version failed.\n",__func__);
        return -1;
    }
    /* connect the major and minor string */
    strcat(hw_version, minor_version[minor_veridx]);
    VERSION_DBG("%s:version is %s\n", __func__, hw_version);
    return 0; 
}
#else

extern void get_hwversion_num(int* version1, int* version2,int* minversion1, int* minversion2);
static int minversion_translate(int gpio_val1, int gpio_val2)
{
	int index = 0;
	if (!gpio_val1 && !gpio_val2)
    {
        index = 0;
    }
    else if (gpio_val1 == 1 && !gpio_val2)
    {
        index = 1;
    }
	else if (!gpio_val1 && gpio_val2 == 1)
    {
        index = 2;
    }
    else if (gpio_val1 ==1 && gpio_val2 == 1)
    {
        index = 3;
    }
    else
    {
        index = -1;
    }
    return index;
}
static int hw_version_get(void)
{
	int hw_version1 = 0, hw_version2 = 0, index=0;
	int hw_min_version1 = 0, hw_min_version2 = 0;
	int minor_veridx;
	memset(hw_version, 0, strlen(hw_version));

	get_hwversion_num(&hw_version1,&hw_version2,&hw_min_version1,&hw_min_version2);
	VERSION_DBG("%s:version1=%d version2=%d,hw_min_version1=%d,hw_min_version2=%d\n",
				 __func__, hw_version1, hw_version2,hw_min_version1,hw_min_version2);
	if (hw_version1 < 0 || hw_version2 < 0)
	{
		printk(KERN_ERR "%s:get major version failed(ver1=%d,ver2=%d)\n",__func__, hw_version1, hw_version2);
		return -1;
	}
	/* lookup the string index by the voltage values */
	for (index = 0; index < VERSION_NUM; index++)
	{
		if ((hw_version1 >= (mv_to_versionidx_map[index].mV1 - VOL_TOLERANCE)) &&
			(hw_version1 <= (mv_to_versionidx_map[index].mV1 + VOL_TOLERANCE)) &&
			(hw_version2 >= (mv_to_versionidx_map[index].mV2 - VOL_TOLERANCE)) &&
			(hw_version2 <= (mv_to_versionidx_map[index].mV2 + VOL_TOLERANCE)))
		{
			break;
		}
	}
	strcpy(hw_version, major_version[index]);
	minor_veridx = minversion_translate(hw_min_version1,hw_min_version2);
	if (minor_veridx < 0)
	{
		printk(KERN_ERR "%s:get minor version failed.\n",__func__);
		return -1;
	}
	/* connect the major and minor string */
	strcat(hw_version, minor_version[minor_veridx]);
	VERSION_DBG("%s:version is %s\n", __func__, hw_version);
	return 0; 
}
#endif

/* release the gpio resource and set the lowpower mode */
static int  k3v2_version_release(void)
{
    int err = 0;
	
    if (NULL == version_gpio_pdata)
    {
        printk(KERN_ERR "[gpioversion]platform data error!\n");
        err = -EINVAL;
        return err;
    }
#ifndef CONFIG_GET_HW_VERSION_FROM_CMDLINE
    /* set the special lowpower work mode by the exit function */
    if (version_gpio_pdata->exit)
    {
        version_gpio_pdata->exit();
    }
    gpio_free(version_gpio_pdata->gpio1);
    gpio_free(version_gpio_pdata->gpio2);
#endif
    return 0;
    
}
/* initialization the gpio configuration */
static int __devinit k3v2_version_probe(struct platform_device* pdev)
{
    int err = 0;

    VERSION_DBG("k3v2_version_probe probe starting....\n");
    if (NULL == pdev) {
        printk(KERN_ERR "[gpioversion]parameter error!\n");
        err = -EINVAL;
        return err;
    }
#ifndef CONFIG_GET_HW_VERSION_FROM_CMDLINE
    version_gpio_pdata = pdev->dev.platform_data;
    if (NULL == version_gpio_pdata)
    {
        printk(KERN_ERR "[gpioversion]platform data error!\n");
        err = -EINVAL;
        return err;
    }

    /* set the special gpio work mode by the init function */
    if (version_gpio_pdata->init)
    {
        version_gpio_pdata->init();
    }
    /* get the gpio12 and gpio13 number */
    gpio1 = version_gpio_pdata->gpio1;
    gpio2 = version_gpio_pdata->gpio2;
    /* request gpio12 */
    err = gpio_request(version_gpio_pdata->gpio1, "gpio1");
    if (err)
    {
        printk(KERN_ERR "[gpioversion]gpio_request gpio12 error!\n");
        return err;
    }
    /* set the gpio deirction to be inputable */
    err = gpio_direction_input(version_gpio_pdata->gpio1);
    if (err)
    {
        printk(KERN_ERR "[gpioversion]gpio_direction_input gpio12 error!\n");
	 gpio_free(version_gpio_pdata->gpio1);
	 return err;
    }
    /* request gpio13 */
    err = gpio_request(version_gpio_pdata->gpio2, "gpio2");
    if (err)
    {
        printk(KERN_ERR "[gpioversion]gpio_request gpio13 error!\n");
        gpio_free(version_gpio_pdata->gpio1);
        return err;
    }
    /* set the gpio deirction to be inputable */
    err = gpio_direction_input(version_gpio_pdata->gpio2);
    if (err)
    {
        printk(KERN_ERR "[gpioversion]gpio_direction_input gpio12 error!\n");
	 gpio_free(version_gpio_pdata->gpio1);
	 gpio_free(version_gpio_pdata->gpio2);
	 return err;
    }
#endif
    VERSION_DBG("k3v2_version_probe probe finish\n");
    return err;
    
}
#ifdef CONFIG_PM
int k3v2_version_suspend(struct platform_device *pdev, pm_message_t state)	
{
	dev_info(&pdev->dev, "suspend+\n");
#ifndef CONFIG_GET_HW_VERSION_FROM_CMDLINE
	if (version_gpio_pdata->exit)
	{
		version_gpio_pdata->exit();
	}
#endif
	return 0;
}

int k3v2_version_resume(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "resume+\n");
#ifndef CONFIG_GET_HW_VERSION_FROM_CMDLINE
	if (version_gpio_pdata->init)
	{
		version_gpio_pdata->init();
	}
#endif
	return 0;
}
#endif

struct platform_driver k3v2_version_driver = {
    .probe = k3v2_version_probe,
#ifndef CONFIG_GET_HW_VERSION_FROM_CMDLINE
#ifdef CONFIG_PM
    .suspend = k3v2_version_suspend,
    .resume = k3v2_version_resume,
#endif
#endif
    .driver = {
	            .name = "k3v2_version",
		     .owner = THIS_MODULE,
    },
};

static int __init hw_version_init(void)
{
   int ret = 0;
   int retry = 0;

   VERSION_DBG("%s:start........\n", __func__);
   ret = platform_driver_register(&k3v2_version_driver);
   if (ret < 0)
   {
       printk(KERN_ERR "%s:register k3v2_version_driver failed(ret = %d)\n", __func__, ret);
	goto out;
   }

    /* crate the node in the sys filesystem.The users can access it by /sys/version/hw_version */
    kobj = kobject_create_and_add("version", NULL);
    if (kobj == NULL) 
    {
        printk(KERN_ERR "%s:create kobject failed\n", __func__);
        platform_driver_unregister(&k3v2_version_driver);
        return -1;
    }
    
    if (sysfs_create_group(kobj, &version_attr_group)) 
    {
        printk(KERN_ERR "%s:create sysfs group failed\n", __func__);
        kobject_put(kobj);
        platform_driver_unregister(&k3v2_version_driver);
        return -1;
    } 

    /* we retry 5 times.if retrying for 5 times,we record as error. */
    for (retry = 0; retry < VERSION_GET_RETRY; retry++)
    {
        ret = hw_version_get();
        if (ret < 0)
        {
            printk(KERN_ERR "%s:create hw_version_get failed(ret=%d, retry=%d)\n", __func__, ret, retry);
	     msleep(1);
	     continue;
        }
	 else
	 {
	     break;
	 }
    }
    /* If retrying times is 5, we think getting version is error. */
    if (VERSION_GET_RETRY == retry)
    {
        printk(KERN_ERR "%s: retrying times is %d\n", __func__, retry);
        kobject_put(kobj);
        platform_driver_unregister(&k3v2_version_driver);
	 goto version_get_failed;
    }
version_get_failed:		
    /* The hardware version will be not changed when the system is running.So 
      * we just only get the hardware version once by hkadc.After getting the version, 
      * we can release the gpio resource.
      */
    ret = k3v2_version_release();
out:
    return ret;
}

static void __exit hw_version_exit (void)
{
    platform_driver_unregister(&k3v2_version_driver);
    sysfs_remove_group(kobj, &version_attr_group);
}
#ifndef CONFIG_GET_HW_VERSION_FROM_CMDLINE
fs_initcall(hw_version_init);
#else
arch_initcall(hw_version_init);
//subsys_initcall(hw_version_init);
#endif
module_exit(hw_version_exit);

MODULE_DESCRIPTION("hardware version for s10");
MODULE_LICENSE("GPL");
