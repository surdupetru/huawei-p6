/*
 * Copyright (C) 2010 Trusted Logic S.A.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/pn544.h>
#include <linux/reboot.h>
#include <linux/hw_dev_dec.h>
#include <linux/wakelock.h>
#include <hsad/config_interface.h>
#define MAX_BUFFER_SIZE	512

//#define NFC_DEBUG_LOG

#ifdef NFC_DEBUG_LOG
#define NFC_DEBUG printk
#else
#define NFC_DEBUG  
#endif

struct pn544_dev	{
	wait_queue_head_t	 read_wq;
	struct mutex		 read_mutex;
	struct i2c_client	*client;
	struct miscdevice	pn544_device;
	unsigned int 		ven_gpio;
	unsigned int 		firm_gpio;
	unsigned int		irq_gpio;
	bool			      irq_enabled;
	spinlock_t		irq_enabled_lock;
};

static struct wake_lock wlock_read;

static void pn544_disable_irq(struct pn544_dev *pn544_dev)
{
	unsigned long flags;

	spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
	if (pn544_dev->irq_enabled) {
		disable_irq_nosync(pn544_dev->client->irq);
		pn544_dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);
}

static irqreturn_t pn544_dev_irq_handler(int irq, void *dev_id)
{
	struct pn544_dev *pn544_dev = dev_id;

	pn544_disable_irq(pn544_dev);

	/* Wake up waiting readers */
	wake_up(&pn544_dev->read_wq);

	return IRQ_HANDLED;
}

static ssize_t pn544_dev_read(struct file *filp, char __user *buf,
		size_t count, loff_t *offset)
{
	struct pn544_dev *pn544_dev = filp->private_data;
	char tmp[MAX_BUFFER_SIZE];
	int ret, i;
	int calc =  0;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	NFC_DEBUG("%s : reading %zu bytes.\n", __func__, count);

	mutex_lock(&pn544_dev->read_mutex);

	wake_lock_timeout(&wlock_read, 1 * HZ);
	NFC_DEBUG("%s : start to gpio_get_value with got value: %d\n", 
		__func__, gpio_get_value(pn544_dev->irq_gpio));
	if (!gpio_get_value(pn544_dev->irq_gpio)) {
		if (filp->f_flags & O_NONBLOCK) {
			NFC_DEBUG("flip->f_flags=%u\n",filp->f_flags);
			ret = -EAGAIN;
			goto fail;
		}
		
		pn544_dev->irq_enabled = true;
		NFC_DEBUG("%s : start to enable_irq.\n", __func__);
		enable_irq(pn544_dev->client->irq);
		NFC_DEBUG("%s : start to wait_event_interruptible.\n", __func__);
		ret = wait_event_interruptible(pn544_dev->read_wq,
				gpio_get_value(pn544_dev->irq_gpio));
		NFC_DEBUG("%s : end to pn544_disable_irq, with ret: %d.\n", __func__, ret);
		
		pn544_disable_irq(pn544_dev);

		if (ret)
			goto fail;

	}

	NFC_DEBUG("%s : start to i2c_master_recv.\n", __func__);

	/* Read data */
	while(calc <3)
        {
            calc ++;
	    ret = i2c_master_recv(pn544_dev->client, tmp, count);
	    if (ret < 0) 
	    {
	         pr_info("%s : read data try =%d returned %d\n", __func__,calc,ret);
	         msleep(10);
	         continue;
	    }
	    else
		break;
        }

	if (calc ==3) {
		pr_err("%s : i2c_master_recv returned %d\n", __func__, ret);
		ret = -EIO;
	}
	
	mutex_unlock(&pn544_dev->read_mutex);

	NFC_DEBUG("%s : end to i2c_master_recv, with the ret: %d.\n", __func__, ret);

	 for(i=0;i<count;i++)
	{
	       NFC_DEBUG("%s : read %x.\n", __func__, tmp[i]);
        }

	if (ret < 0) {
		pr_err("%s: i2c_master_recv returned %d\n", __func__, ret);
		return ret;
	}
	if (ret > count) {
		pr_err("%s: received too many bytes from i2c (%d)\n",
			__func__, ret);
		return -EIO;
	}
	if (copy_to_user(buf, tmp, ret)) {
		pr_err("%s : failed to copy to user space\n", __func__);
		return -EFAULT;
	}

	return ret;

fail:
        mutex_unlock(&pn544_dev->read_mutex);

	return ret;
}

static ssize_t pn544_dev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *offset)
{
	struct pn544_dev  *pn544_dev;
	char tmp[MAX_BUFFER_SIZE];
	int ret;
	int calc =  0;

	pn544_dev = filp->private_data;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (copy_from_user(tmp, buf, count)) {
		pr_err("%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}

	NFC_DEBUG("%s : writing %zu bytes.\n", __func__, count);
	/* Write data */
        while(calc <3)
        {
            calc ++;
	    ret = i2c_master_send(pn544_dev->client, tmp, count);
	    if (ret != count) 
	    {
	         pr_info("%s : send data try =%d returned %d\n", __func__,calc,ret);
		 msleep(10);
	         continue;
	    }
	    else
		break;
        }

	if (calc ==3) {
		pr_err("%s : i2c_master_send returned %d\n", __func__, ret);
		ret = -EIO;
	}

	return ret;
}

static int pn544_dev_open(struct inode *inode, struct file *filp)
{
	struct pn544_dev *pn544_dev = container_of(filp->private_data,
						struct pn544_dev,
						pn544_device);

    NFC_DEBUG("%s:enter pn544_dev_open\n", __func__);

    filp->private_data = pn544_dev;

    pr_info("%s : %d,%d\n", __func__, imajor(inode), iminor(inode));

    NFC_DEBUG("%s:exit pn544_dev_open\n", __func__);

	return 0;
}

static long pn544_dev_ioctl(struct file *filp,
			    unsigned int cmd, unsigned long arg)
{
	struct pn544_dev *pn544_dev = filp->private_data;

	switch (cmd) {
	case PN544_SET_PWR:
		if (arg == 2) {
			/* power on with firmware download (requires hw reset)
			 */
			pr_info("%s power on with firmware\n", __func__);
			gpio_set_value(pn544_dev->ven_gpio, 1);
			gpio_set_value(pn544_dev->firm_gpio, 1);
			msleep(20);
			gpio_set_value(pn544_dev->ven_gpio, 0);
			msleep(60);
			gpio_set_value(pn544_dev->ven_gpio, 1);
			msleep(20);
		} else if (arg == 1) {
			/* power on */
			pr_info("%s power on\n", __func__);
			gpio_set_value(pn544_dev->firm_gpio, 0);
			gpio_set_value(pn544_dev->ven_gpio, 1);
			msleep(20);
		} else  if (arg == 0) {
			/* power off */
			pr_info("%s power off\n", __func__);
			gpio_set_value(pn544_dev->firm_gpio, 0);
			gpio_set_value(pn544_dev->ven_gpio, 0);
			msleep(60);
		} else {
			pr_err("%s bad arg %lu\n", __func__, arg);
			return -EINVAL;
		}
		break;
	default:
		pr_err("%s bad ioctl %u\n", __func__, cmd);
		return -EINVAL;
	}

	return 0;
}

static const struct file_operations pn544_dev_fops = {
	.owner	= THIS_MODULE,
	.llseek	= no_llseek,
	.read	= pn544_dev_read,
	.write	= pn544_dev_write,
	.open	= pn544_dev_open,
	.unlocked_ioctl	= pn544_dev_ioctl,
	
};

static int check_pn544(struct i2c_client *client)
{
       int ret;
       int count =  0;
       int calc = 0;
       int check_pn544_result = 0;
       const char host_to_pn544[8] = {0x05, 0xF9, 0x04, 0x00, 0xC3, 0xe5};
       const char check_from_pn544[8] = {0x03,0xE6,0x17,0xA7};
       char pn544_to_host[8];
       const char cmd_fwdld_first[4] = {0x01, 0x00, 0x00};

       struct pn544_i2c_platform_data *platform_data;
       platform_data = client->dev.platform_data;

       NFC_DEBUG("------enter check pn544-----\n");
       for(count = 0; count < 3;count++)
       {
	       ret = i2c_master_send(client,  &host_to_pn544, 6);

               NFC_DEBUG("%s:enter pn544 I2C\n", __func__);

	       if (ret < 0)
	       {
	                pr_err("%s:pn544_i2c_write failed\n", __func__);
			msleep(10);
			continue;
		}
		pr_err("pn544_i2c_write: %x, %x, %x, %x, %x, %x \n", host_to_pn544[0],host_to_pn544[1],
		              host_to_pn544[2],host_to_pn544[3],host_to_pn544[4],host_to_pn544[5]);

		ret =  i2c_master_recv(client, &pn544_to_host, 4);

		if (ret < 0)
	       {
	                pr_err("%s:pn544_i2c_read failed\n", __func__);
			msleep(10);
			continue;
		}

		pr_err("pn544_i2c_read: %x,%x,%x,%x \n", pn544_to_host[0],pn544_to_host[1],
		pn544_to_host[2],pn544_to_host[3]);

		if(!((pn544_to_host[0]== check_from_pn544[0])&&(pn544_to_host[1]== check_from_pn544[1])
			&&(pn544_to_host[2]== check_from_pn544[2])&&(pn544_to_host[3]== check_from_pn544[3])))
		{
	                  pr_err("%s : pn544 device check failed.\n", __func__);
			  msleep(10);
			  ret = -ENODEV;
			  continue;
		}
		check_pn544_result = 1;
	        break;
        }

	if ((1 == check_pn544_result) ||(pn544_to_host[0] == 0x51))
	{
	        pr_err("%s, pn544 check successfully.\n", __func__);
		ret = 0;
		goto exit;
	}

	for(calc = 0; calc < 3;calc++)
       {
		pr_info("%s:enter fw download mode\n", __func__);
	        gpio_set_value(platform_data->ven_gpio, 1);
		gpio_set_value(platform_data->firm_gpio, 1);
		msleep(20);
		gpio_set_value(platform_data->ven_gpio, 0);
		msleep(60);
		gpio_set_value(platform_data->ven_gpio, 1);
		msleep(20);

                pr_info("%s:send fw download cmd for check pn544\n", __func__);
			
		ret = i2c_master_send(client,  &cmd_fwdld_first, 3);
		if (ret < 0)
	        {
		      pr_err("%s:pn544_i2c_write download cmd failed:%d.\n", __func__,calc);
		      continue;
		}
		pr_info("%s:exit firmware download for check pn544 successfully\n", __func__);
		gpio_set_value(platform_data->firm_gpio, 0);
		gpio_set_value(platform_data->ven_gpio, 1);
		msleep(20);
		gpio_set_value(platform_data->ven_gpio, 0);
		msleep(60);
		gpio_set_value(platform_data->ven_gpio, 1);
		msleep(20);
		break;

	}
	
exit:
	return ret;

}


static int ven_status = 0;
static ssize_t pn544_ven_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int retval = 0;
        NFC_DEBUG("%s: enter pn544_ven_store\n", __func__);
	if ('0' != buf[0]) {
		retval = gpio_direction_output(NFC_1V8EN,1);
	        if (retval) {
	            pr_err("%s: Failed to setup nfc_ven gpio %d. Code: %d.",
	                   __func__, NFC_1V8EN, retval);
	        }
                ven_status = 1;

	} else {
		retval = gpio_direction_output(NFC_1V8EN,0);
	        if (retval) {
	            pr_err("%s: Failed to setup nfc_ven gpio %d. Code: %d.",
	                   __func__, NFC_1V8EN, retval);
	        }
		ven_status = 0;
	}

    NFC_DEBUG("%s: exit pn544_ven_store\n", __func__);
	return count;
}

static ssize_t pn544_ven_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t count;
	count = sprintf(buf, "%d\n", ven_status);
	return count;
}

static ssize_t nfc_product_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int product = 0;
	ssize_t value;

	product = get_nfc_product();
	value = sprintf(buf, "%d\n", product);

	return value;
}
static ssize_t nfc_module_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int boardid = 0;

	boardid = get_nfc_module();
	return sprintf(buf, "%d\n", boardid);
}

static struct device_attribute pn544_attr[] ={
	__ATTR(nfc_ven, 0664, pn544_ven_show, pn544_ven_store),
	__ATTR(nfc_product, 0664, nfc_product_show, NULL),
	__ATTR(nfc_module, 0664, nfc_module_show, NULL),
};

static int create_sysfs_interfaces(struct device *dev)
{
       int i;
       for (i = 0; i < ARRAY_SIZE(pn544_attr); i++)
	          if (device_create_file(dev, pn544_attr + i))
		       goto error;
       return 0;
error:
	for ( ; i >= 0; i--)
		   device_remove_file(dev, pn544_attr + i);
       pr_err(KERN_ERR "pn544 unable to create sysfs interface.\n" );
       return -1;
}

static int remove_sysfs_interfaces(struct device *dev)
{
       int i;
       for (i = 0; i < ARRAY_SIZE(pn544_attr); i++)
	           device_remove_file(dev, pn544_attr + i);
       return 0;
}

static int nfc_ven_low_beforepwd(struct notifier_block *this, unsigned long code,
								void *unused)
{
        int retval = 0;
	    NFC_DEBUG("%s: enter  nfc_ven_low_beforepwd\n", __func__);
	    retval = gpio_direction_output(NFC_1V8EN,0);
        if (retval) {
            pr_err("%s: Failed to setup nfc_ven gpio %d. Code: %d.",
                   __func__, NFC_1V8EN, retval);
            gpio_free(NFC_1V8EN);
        }
        return retval;
}

static struct notifier_block nfc_ven_low_notifier = {
	.notifier_call =	nfc_ven_low_beforepwd,
};
static int pn544_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret;

	struct pn544_i2c_platform_data *platform_data;

    struct pn544_dev *pn544_dev;

    NFC_DEBUG("%s: enter pn544 probe\n", __func__);

	platform_data = client->dev.platform_data;

	if (platform_data == NULL) {
		pr_err("%s : nfc probe fail\n", __func__);
		return  -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s : need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}

        /* IRQ_GPIO requset*/
	ret = gpio_request(platform_data->irq_gpio, "nfc_int");
	if (ret)
		return  -ENODEV;

	/*nfc gpio config*/
	if (platform_data->gpio_config) {
		dev_info(&client->dev, "Configuring GPIOs.\n");
		ret = platform_data->gpio_config(platform_data->gpio_data, true);
		if (ret< 0) {
			dev_err(&client->dev, "Failed to configure GPIOs, code: %d.\n",ret);
			return ret;
		}
		dev_info(&client->dev, "Done with GPIO configuration.\n");
	}

	ret = create_sysfs_interfaces(&client->dev);
	if (ret < 0) {
		return ret;
	}

#if 0
	ret = gpio_request(platform_data->ven_gpio, "nfc_ven");
	if (ret)
		goto err_ven;
	ret = gpio_request(platform_data->firm_gpio, "nfc_firm");
	if (ret)
		goto err_firm;
#endif

	pn544_dev = kzalloc(sizeof(*pn544_dev), GFP_KERNEL);
	if (pn544_dev == NULL) {
		dev_err(&client->dev,
				"failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}

        /*check pn544 device: if not, don't add pn544 driver*/
#if 1
        ret = check_pn544(client);
	if(ret < 0){
              pr_err("%s: pn544 probe failed \n",__func__);
	      goto err_exit;
	}

	ret = gpio_direction_output(NFC_1V8EN,0);
	        if (ret) {
	            pr_err("%s: Failed to setup nfc_ven gpio %d. Code: %d.",
	                   __func__, NFC_1V8EN, ret);
	        }
#endif

	pn544_dev->irq_gpio = platform_data->irq_gpio;
	pn544_dev->ven_gpio  = platform_data->ven_gpio;
	pn544_dev->firm_gpio  = platform_data->firm_gpio;
	pn544_dev->client   = client;

	/* init mutex and queues */
	init_waitqueue_head(&pn544_dev->read_wq);
	mutex_init(&pn544_dev->read_mutex);
	spin_lock_init(&pn544_dev->irq_enabled_lock);

	pn544_dev->pn544_device.minor = MISC_DYNAMIC_MINOR;
	pn544_dev->pn544_device.name = "pn544";
	pn544_dev->pn544_device.fops = &pn544_dev_fops;

	ret = misc_register(&pn544_dev->pn544_device);
	if (ret) {
		pr_err("%s : misc_register failed\n", __FILE__);
		goto err_misc_register;
	}

	/* Initialize wakelock*/
	wake_lock_init(&wlock_read, WAKE_LOCK_SUSPEND, "nfc_read");

	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */


	NFC_DEBUG("%s : requesting IRQ %d\n", __func__, client->irq);
	pn544_dev->irq_enabled = true;

	client->irq = gpio_to_irq(client->irq);
	
	ret = request_irq(client->irq, pn544_dev_irq_handler,
			  IRQF_TRIGGER_HIGH, client->name, pn544_dev);
	if (ret) {
		dev_err(&client->dev, "request_irq failed\n");
		goto err_request_irq_failed;
	}
	pn544_disable_irq(pn544_dev);
	i2c_set_clientdata(client, pn544_dev);

	ret = register_reboot_notifier(&nfc_ven_low_notifier);
	if (ret != 0) {
		pr_info(KERN_ERR "cannot register reboot notifier (err=%d)\n",
			ret);
		goto err_request_irq_failed;
	}

        NFC_DEBUG("%s: NFC probe end\n", __func__);

	set_hw_dev_flag(DEV_I2C_NFC);

	return 0;

err_request_irq_failed:
	wake_lock_destroy(&wlock_read);
	misc_deregister(&pn544_dev->pn544_device);
err_misc_register:
	mutex_destroy(&pn544_dev->read_mutex);
	kfree(pn544_dev);
err_exit:
	gpio_free(platform_data->irq_gpio);
	gpio_free(platform_data->ven_gpio);
	gpio_free(platform_data->firm_gpio);
	device_remove_file(&client->dev, pn544_attr);
	device_remove_file(&client->dev, pn544_attr+1);

	return ret;
}



static int pn544_remove(struct i2c_client *client)
{
	struct pn544_dev *pn544_dev;

    unregister_reboot_notifier(&nfc_ven_low_notifier);
    pn544_dev = i2c_get_clientdata(client);
	free_irq(client->irq, pn544_dev);
	wake_lock_destroy(&wlock_read);
	misc_deregister(&pn544_dev->pn544_device);
	mutex_destroy(&pn544_dev->read_mutex);
	gpio_free(pn544_dev->irq_gpio);
	gpio_free(pn544_dev->ven_gpio);
	gpio_free(pn544_dev->firm_gpio);
	remove_sysfs_interfaces(&client->dev);
	kfree(pn544_dev);

	return 0;
}

static const struct i2c_device_id pn544_id[] = {
	{ "pn544", 0 },
	{ }
};

static struct i2c_driver pn544_driver = {
	.id_table	= pn544_id,
	.probe		= pn544_probe,
	.remove		= pn544_remove,
	.driver		= {
	.owner	    = THIS_MODULE,
	.name	    = "pn544",
	},
};

/*
 * module load/unload record keeping
 */

static int __init pn544_dev_init(void)
{
	pr_info("Loading pn544 driver\n");
	return i2c_add_driver(&pn544_driver);
}
module_init(pn544_dev_init);

static void __exit pn544_dev_exit(void)
{
	pr_info("Unloading pn544 driver\n");
	i2c_del_driver(&pn544_driver);
}
module_exit(pn544_dev_exit);

MODULE_AUTHOR("Sylvain Fonteneau");
MODULE_DESCRIPTION("NFC PN544 driver");
MODULE_LICENSE("GPL");
