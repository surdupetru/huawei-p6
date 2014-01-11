#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/mux.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/random.h>
#include <linux/time.h>
#include <linux/compiler.h>

#include <mach/gpio.h>
#include <mach/platform.h>
#include "mach/modem_boot.h"
#include <mach/modem_boot_qsc6085.h>
#include <hsad/config_interface.h>


#define iomux_block_set_work_mode(block_name)   \
        blockmux_set(iomux_get_block(block_name),iomux_get_blockconfig(block_name),NORMAL)


//MODEM BOOT GPIO
#define GPIO_CP_POWER_ON        modem_driver_plat_data->gpios[MODEM_BOOT_POWER_ON].gpio
#define GPIO_CP_PMU_RESET       modem_driver_plat_data->gpios[MODEM_BOOT_PMU_RESET].gpio
#define GPIO_POWER_IND       modem_driver_plat_data->gpios[MODEM_BOOT_POWER_IND].gpio
#define GPIO_RESET_IND       modem_driver_plat_data->gpios[MODEM_BOOT_RESET_IND].gpio

//COMM GPIO
#define GPIO_DOWNLOAD_EN       modem_driver_plat_data->gpios[COMM_DOWNLOAD_EN].gpio
#define GPIO_WAKEUP_SLAVE       modem_driver_plat_data->gpios[COMM_WAKEUP_SLAVE].gpio
#define GPIO_WAKEUP_HOST       modem_driver_plat_data->gpios[COMM_WAKEUP_HOST].gpio
#define GPIO_CP_PSHOLD         modem_driver_plat_data->gpios[MODEM_BOOT_PSHOLD].gpio

//Rename modem indication GPIO
#define GPIO_CP_PM_INDICATION    GPIO_POWER_IND      //AP: GPIO_094_MODEM_POWER_IND; CP: VREG_MSME1;
#define GPIO_CP_SW_INDICATION    GPIO_RESET_IND      //AP: GPIO_098_M_RST_STATE; CP: GPIO_12;


static struct modem_qsc6085_platform_data *modem_driver_plat_data=NULL;
static int modem_curr_power_state;
static volatile int gpio_wake_host_state = 0;
static struct wake_lock wake_host_lock;
static struct modem_boot_device *modem_device = NULL;
static volatile int modem_in_state_change = 0;

/* Do CP power on */
static void modem_boot_power_on(void)
{
        gpio_set_value(GPIO_CP_PMU_RESET, 1);
        gpio_set_value(GPIO_CP_PSHOLD, 0);
        gpio_set_value(GPIO_CP_POWER_ON, 1);
        msleep(40);
}

/* Do CP power off */
static void modem_boot_power_off(void)
{
    gpio_set_value(GPIO_CP_POWER_ON, 0);
    gpio_set_value(GPIO_CP_PSHOLD, 1);
    msleep(20);
    gpio_set_value(GPIO_CP_PMU_RESET, 0);
    msleep(40);
}

static volatile int resume_flag = 0;
void set_first_resume_flag(void)
{
	resume_flag = 1;
}
void clear_resume_flag(void)
{
	printk("qsc6085 clear_resume_flag\n");
	resume_flag = 0;
	if(modem_driver_plat_data)
	  gpio_set_value(GPIO_WAKEUP_SLAVE, 0);
}
int get_resume_flag(void)
{
	return resume_flag;
}
bool modem_comm_registered_for_sleep(void)
{
    return ( (get_modem_type( MODEM_QSC6085 )>0)?true:false );
}
static atomic_t tty_driver_in_suspend = ATOMIC_INIT(0);
void modem_comm_set_gpio_for_suspend(void)
{
	printk(KERN_INFO"qsc6085 modem_comm_set_gpio_for_suspend\n");
	if(modem_driver_plat_data)
	  gpio_set_value(GPIO_WAKEUP_SLAVE, 1);
	atomic_set(&tty_driver_in_suspend, 1);
}
void modem_comm_set_gpio_for_resume(void)
{
	int val;
	if(!modem_driver_plat_data)
	   return;
	printk(KERN_INFO"qsc6085 modem_comm_set_gpio_for_resume\n");
	/*if modem has already wakeup, set wakeup_slave to 0,
	  else delay to the first time write to modem,
	  to keep modem still in sleep*/
	val = gpio_get_value(GPIO_WAKEUP_HOST);
	if(val == 0)
		{
		printk(KERN_INFO"qsc6085 wake_host low, set wake_salve low\n");
		if(modem_driver_plat_data)
			gpio_set_value(GPIO_WAKEUP_SLAVE, 0);
		}
	set_first_resume_flag();
	atomic_set(&tty_driver_in_suspend, 0);
}

void inline uart1_set_gpio_for_runtime_suspend(void)
{
	printk(KERN_INFO"qsc6085 uart1_set_gpio_for_runtime_suspend\n");
	if(modem_driver_plat_data)
	  gpio_set_value(GPIO_WAKEUP_SLAVE, 1);
}

void inline uart1_set_gpio_for_runtime_resume(void)
{
	int k,m;
	if(modem_driver_plat_data)
	{
		if(atomic_read(&tty_driver_in_suspend))
			return;
	}else
		return;
	printk(KERN_INFO"qsc6085 uart1_set_gpio_for_runtime_resume\n");
	if(modem_driver_plat_data)
	{
		gpio_set_value(GPIO_WAKEUP_SLAVE, 0);
		mdelay(1);
	}
}

static inline void modem_boot_change_state(int state)
{
    pr_info("modem_power: Power state change: %d -> %d", modem_curr_power_state, state);
    modem_curr_power_state = state;
}

static ssize_t modem_boot_get(struct device *dev,
                 struct device_attribute *attr,
                 char *buf)
{
    ssize_t ret = modem_curr_power_state;
    return sprintf(buf, "%d\n", ret);
}

static ssize_t modem_boot_set(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t count)
{
    int state;

    dev_info(dev, "Power PHY set to %s\n", buf);

    if (sscanf(buf, "%d", &state) != 1)
        return -EINVAL;

	modem_in_state_change = 1;
    if (state == POWER_SET_ON) {
        modem_boot_change_state(state);
        gpio_set_value(GPIO_DOWNLOAD_EN, 1);//set high is normal boot mode
        modem_boot_power_on();
    } else if (state == POWER_SET_OFF) {
        modem_boot_change_state(state);
        modem_boot_power_off();
        gpio_set_value(GPIO_DOWNLOAD_EN, 0);
    } else if (state == POWER_SET_DEBUGON) {
        modem_boot_change_state(state);
        modem_boot_power_on();
    } else if (state == POWER_SET_DEBUGOFF) {
        modem_boot_change_state(state);
        modem_boot_power_off();
    } else {
        dev_err(dev, "Power PHY error state. %s\n", buf);
    }
	modem_in_state_change = 0;
    return count;
}

static ssize_t modem_gpio_set(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t count)
{
    char name[128] = {0};
    int val;
    unsigned gpio;
    int i;

    if (!modem_driver_plat_data)
        return -EINVAL;

    if (sscanf(buf, "%s %d", name, &val) != 2)
        return -EINVAL;

    for (i=0; i<ARRAY_SIZE(modem_driver_plat_data->gpios); i++)
        if (strcmp(name, modem_driver_plat_data->gpios[i].label) == 0)
            break;

    if (i == ARRAY_SIZE(modem_driver_plat_data->gpios))
        return -EINVAL;
    else
        gpio = modem_driver_plat_data->gpios[i].gpio;

    dev_info(dev, "%s set to %d\n", name, val);

    gpio_set_value(gpio, val);

    return count;
}

static ssize_t modem_gpio_get(struct device *dev,
                 struct device_attribute *attr,
                 char *buf)
{
    char* str = buf;
    int i;

    if (!modem_driver_plat_data)
        return 0;

    for (i=0; i<ARRAY_SIZE(modem_driver_plat_data->gpios); i++) {
        str += sprintf(str, "%s %d\n", modem_driver_plat_data->gpios[i].label,
                gpio_get_value(modem_driver_plat_data->gpios[i].gpio));
       }
    return str - buf;
}

/*------------------------------ SYSFS "modem_state" ------------------------------*/
static ssize_t modem_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int mdm_state;
    int gpio_val=gpio_get_value(GPIO_CP_SW_INDICATION);

    if( GPIO_CP_SW_RDY==gpio_val )
        mdm_state = MODEM_STATE_READY;
    else
        mdm_state = MODEM_STATE_OFF;

    return ( snprintf(buf,PAGE_SIZE,"%s\n", modem_state_str[mdm_state]) );
}

static DEVICE_ATTR(state, S_IRUGO | S_IWUSR, modem_boot_get, modem_boot_set);
static DEVICE_ATTR(modem_state, S_IRUGO, modem_state_show, NULL);
/* gpio are for debug */
static DEVICE_ATTR(gpio, S_IRUGO | S_IWUSR, modem_gpio_get, modem_gpio_set);

static irqreturn_t host_wakeup_isr(int irq, void *dev_id)
{
    int val = gpio_get_value(GPIO_WAKEUP_HOST);
    if (val)
        printk(KERN_INFO"qsc6085 Host wakeup rising[%d]\n", gpio_wake_host_state);
    else
    {
        printk(KERN_INFO"qsc6085 Host wakeup falling[%d]\n", gpio_wake_host_state);
        if(!atomic_read(&tty_driver_in_suspend) && gpio_get_value(GPIO_WAKEUP_SLAVE))
        {
        printk(KERN_INFO"qsc6085 irq not in suspend mode,set gpio for resume\n");
        if(modem_driver_plat_data)
           gpio_set_value(GPIO_WAKEUP_SLAVE, 0);
        }
        wake_lock_timeout(&wake_host_lock, 2 * HZ);
    }
    gpio_wake_host_state = val;
   return IRQ_HANDLED;
}

//schedule a work to send uevent
static void uevent_work_func(struct work_struct *data)
{
	char *reset[2]       = { "MODEM_STATE=0", NULL };
	char *power_on[2]    = { "MODEM_STATE=1", NULL };
	char **uevent_envp = NULL;
	unsigned long flags;
	struct modem_boot_device *mdevice =  container_of(data, struct modem_boot_device, work);
	spin_lock_irqsave(&mdevice->lock, flags);
	uevent_envp = (mdevice->modem_state == MODEM_STATE_POWERON) ? power_on : reset;
	spin_unlock_irqrestore(&mdevice->lock, flags);
	kobject_uevent_env(&mdevice->device->kobj, KOBJ_CHANGE, uevent_envp);
}

static irqreturn_t cp_sw_indication_isr(int irq, void *dev_id)
{
	int val;
	unsigned long flags;
	struct modem_boot_device * mdevice = (struct modem_boot_device *)dev_id;
	if(mdevice == NULL)
	{
		printk(KERN_INFO"ERR: cp_sw_indication_isr , dev_id is NULL\n");
		return IRQ_HANDLED;
	}
	if( modem_in_state_change)
	{
		printk(KERN_INFO"qsc6085 cp_sw_indication_isr during modem state set, return\n");
		return IRQ_HANDLED;
	}
	val = gpio_get_value(GPIO_CP_SW_INDICATION);
	spin_lock_irqsave(&mdevice->lock, flags);
	if (val)
	{
		mdevice->modem_state = MODEM_STATE_POWERON;
	}
	else
	{
		mdevice->modem_state = MODEM_STATE_RESET;
		wake_lock_timeout(&wake_host_lock, 5 * HZ);
	}
	schedule_work(&mdevice->work);
	spin_unlock_irqrestore(&mdevice->lock, flags);
	printk(KERN_INFO"qsc6085 GPIO_CP_SW_INDICATION %s\n" , (val ? "rising": "falling"));
	return IRQ_HANDLED;
}
static int modem_boot_probe(struct platform_device *pdev)
{
    int status = -1;

    dev_info(&pdev->dev, "%s. modem_boot_probe\n",__func__);

    modem_driver_plat_data = pdev->dev.platform_data;
        //iomux_block_set_work_mode( modem_driver_plat_data->block_name );  //Todo later
    if (modem_driver_plat_data) {
        status = gpio_request_array(modem_driver_plat_data->gpios, MODEM_GPIO_NUM);
        if (status) {
            dev_err(&pdev->dev, "%s. GPIO request failed.%d\n",__func__,status);
            goto error;
        }
    }

    /* sysfs entries for IO control */
    status = device_create_file(&(pdev->dev), &dev_attr_state);
    if (status) {
        dev_err(&pdev->dev, "Failed to create sysfs entry\n");
        goto error;
    }

    /* sysfs entries for modem state indication node "modem_state" */
    status = device_create_file(&(pdev->dev), &dev_attr_modem_state);
    if (status) {
        dev_err(&pdev->dev, "Failed to create sysfs entry modem_state\n");
        goto error;
    }

    status = device_create_file(&(pdev->dev), &dev_attr_gpio);
    if (status) {
        dev_err(&pdev->dev, "Failed to create sysfs entry\n");
        goto error;
    }

        dev_info(&pdev->dev, "%s. Call modem_boot_set(&pdev->dev,NULL,\"2\",1) now\n", __func__);
        status = modem_boot_set(&pdev->dev,NULL,"2",1); //power off modem at the init time;
        if( status!=1 ) {
            dev_err(&pdev->dev, "Failed to call modem_boot_set(&pdev->dev,NULL,\"2\",1)\n");
        }
        //register irq handler
        wake_lock_init(&wake_host_lock, WAKE_LOCK_SUSPEND, "qsc6085");
		/* Register IRQ for HOST_WAKEUP gpio */
		status = request_irq(gpio_to_irq(GPIO_WAKEUP_HOST),
							host_wakeup_isr,
							IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND,
							"HOST_WAKEUP_IRQ",
							&(pdev->dev));
		printk(KERN_INFO"request irq for GPIO_WAKE_HOST num=%d, ret=%d\n" , GPIO_WAKEUP_HOST,status);
		if (status) {
			printk(KERN_ERR"Request irq for host wakeup\n");
		}
		modem_device = kmalloc(sizeof(struct modem_boot_device), GFP_KERNEL);
		memset(modem_device,0,sizeof(struct modem_boot_device));
		modem_device->device = &(pdev->dev);
		modem_device->modem_state = MODEM_STATE_RESET;
		spin_lock_init(&modem_device->lock);
		INIT_WORK(&modem_device->work, uevent_work_func);
		//Register IRQ for GPIO_CP_SW_INDICATION
		status = request_irq(gpio_to_irq(GPIO_CP_SW_INDICATION),
							cp_sw_indication_isr,
							IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND,
							"GPIO_CP_SW_INDICATION",
							modem_device);
		printk(KERN_INFO"request irq for GPIO_CP_SW_INDICATION num=%d, ret=%d\n" , GPIO_CP_SW_INDICATION,status);
		if (status) {
			printk(KERN_ERR"Request irq for GPIO_CP_SW_INDICATION fail\n");
		}

    return 0;

error:
    if (modem_driver_plat_data) {
        gpio_free_array(modem_driver_plat_data->gpios, MODEM_GPIO_NUM);
    }
    device_remove_file(&(pdev->dev), &dev_attr_state);
    device_remove_file(&(pdev->dev), &dev_attr_gpio);
	if(modem_device)
		kfree(modem_device);
	modem_device = NULL;

    return status;
}

static int modem_boot_remove(struct platform_device *pdev)
{
    dev_dbg(&pdev->dev, "modem_boot_remove\n");

    device_remove_file(&(pdev->dev), &dev_attr_state);
    device_remove_file(&(pdev->dev), &dev_attr_gpio);
    if (modem_driver_plat_data) {
        gpio_free_array(modem_driver_plat_data->gpios, MODEM_GPIO_NUM);
    }
	if(modem_device)
		kfree(modem_device);
	modem_device = NULL;

    return 0;
}

static void modem_boot_shutdown(struct platform_device *pdev)
{
    dev_dbg(&pdev->dev, "modem_boot_shutdown\n");

    dev_info(&pdev->dev, "%s. Call modem_boot_set(&pdev->dev,NULL,\"2\",1) now\n", __func__);
    if( modem_boot_set(&pdev->dev,NULL,"2",1)!=1 ) {    //power off modem at the init time;
        dev_err(&pdev->dev, "Failed to call modem_boot_set(&pdev->dev,NULL,\"2\",1)\n");
    }
}

static struct platform_driver modem_boot_driver = {
    .probe = modem_boot_probe,
    .remove = modem_boot_remove,
    .shutdown = modem_boot_shutdown,
    .driver = {
        .name = MODEM_DEVICE_BOOT(MODEM_QSC6085),
        .owner = THIS_MODULE,
    },
};

static int __init modem_boot_init(void)
{
    int ret = -1;

        ret = get_modem_type( MODEM_QSC6085 );
        pr_info("MODEM#%s: get_modem_type \"%s\". %d\n",MODEM_DEVICE_BOOT(MODEM_QSC6085),MODEM_QSC6085,ret);
        if( ret<=0 ) {
            pr_err("MODEM#%s: modem \"%s\" not support on this board. %d\n",MODEM_DEVICE_BOOT(MODEM_QSC6085),MODEM_QSC6085,ret);
            return( -1 );
        }
    platform_driver_register(&modem_boot_driver);
    return 0;
}

static void __exit modem_boot_exit(void)
{
    platform_driver_unregister(&modem_boot_driver);
}

module_init(modem_boot_init);
module_exit(modem_boot_exit);
MODULE_LICENSE("GPL");

