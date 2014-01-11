#include <linux/uaccess.h>
#include <mach/gpio.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/crc-ccitt.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/hw_dev_dec.h>
#include <linux/hw_dev_array.h>
#include <hsad/config_interface.h>

#ifndef TYPEDEF_UINT8
typedef unsigned char	uint8;
#endif

#ifndef TYPEDEF_UINT16
typedef unsigned short	uint16;
#endif

#ifndef TYPEDEF_UINT32
typedef unsigned int	uint32;
#endif

#ifndef TYPEDEF_UINT64
typedef unsigned long long	uint64;
#endif

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

struct dev_flag_device{
    const char    *name;
    struct device    *dev;
    int        index;
};

static struct dev_flag_device dev_dct = {
    .name = "dev_flag",
    .index = 0,
};

static uint64 dev_flag_long = 0;

/* set device flag */
int set_hw_dev_flag( int dev_id )
{

    if( (dev_id >= 0) && ( dev_id < DEV_I2C_MAX ) )
    {
        dev_flag_long = dev_flag_long | (1 << dev_id);
    }
    else
    {
        return false;
    }
    return true;
}

static ssize_t dev_flag_show(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    return sprintf((char *)buf, "%llu\n",dev_flag_long);
}

static DEVICE_ATTR(dev_flag, S_IRUGO | S_IWUSR, dev_flag_show, NULL);

static int __devinit dev_dct_probe(struct platform_device *pdev)
{
    int ret = 0;
    int i,j;
    int type;
    struct class *myclass = class_create(THIS_MODULE, "dev_dct");
    dev_dct.dev = device_create(myclass, NULL, MKDEV(0, dev_dct.index), NULL, dev_dct.name);
    ret = device_create_file(dev_dct.dev, &dev_attr_dev_flag);

    for(i = 0;i<DEV_I2C_MAX;i++)
    {
        for(j = 0;j<sizeof(hw_dec_device_array)/sizeof(hw_dec_struct);j++)
        {
            if (hw_dec_device_array[j].devices_id == i)
            {
                type = get_I2C_devices(hw_dec_device_array[j].devices_name);
                if(!type || (type == -1))
                {
                    dev_flag_long = dev_flag_long | (1 << i);
                }
                break;
            }
            if(j == (sizeof(hw_dec_device_array)/sizeof(hw_dec_struct)-1))
            {
                dev_flag_long = dev_flag_long | (1 << i);
            }
        }

    }
    for(i = DEV_CONNECTIVITY_START;i <DEV_CONNECTIVITY_MAX;i++)
    {
        for(j = 0;j < sizeof(hw_dec_device_array)/sizeof(hw_dec_struct);j++)
        {
            if (hw_dec_device_array[j].devices_id == i)
            {
                type = get_connectivity_devices(hw_dec_device_array[j].devices_name);
                if(type)
                {
                    dev_flag_long = dev_flag_long | (1 << i);
                    break;
                }
            }
        }
    }

    return 0;
}

static struct platform_driver dev_dct_driver = {
	.driver	= {
		.name	= "hw-dev-detect",
	},
	.probe		= dev_dct_probe,
	.remove		= NULL,
};

static int __init hw_dev_dct_init(void)
{
    return platform_driver_register(&dev_dct_driver);
}

static void __exit hw_dev_dct_exit(void)
{
    platform_driver_unregister(&dev_dct_driver);
}

/* priority is 7s */
late_initcall_sync(hw_dev_dct_init);

module_exit(hw_dev_dct_exit);

MODULE_AUTHOR("sjm");
MODULE_DESCRIPTION("Device Detect Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dev_dct");
