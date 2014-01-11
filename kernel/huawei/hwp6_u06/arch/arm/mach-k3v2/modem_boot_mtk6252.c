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
#include <mach/modem_boot_mtk6252.h>
#include <hsad/config_interface.h>


#define iomux_block_set_work_mode(block_name)   \
        blockmux_set(iomux_get_block(block_name),iomux_get_blockconfig(block_name),NORMAL)


//MODEM BOOT GPIO
#define GPIO_CP_RESET           modem_driver_plat_data->gpios[MODEM_MTK_BOOT_RESET].gpio
#define GPIO_CP_POWER_ON        modem_driver_plat_data->gpios[MODEM_MTK_BOOT_POWER_ON].gpio
#define GPIO_CP_DOWNLOAD_EN         modem_driver_plat_data->gpios[MODEM_MTK_BOOT_DOWNLOAD_EN].gpio
#define GPIO_CP_SOFTWARE_STATE   modem_driver_plat_data->gpios[MODEM_MTK_BOOT_SOFTWARE_STATE].gpio
#define GPIO_POWER_IND       modem_driver_plat_data->gpios[MODEM_MTK_BOOT_POWER_IND].gpio
#define GPIO_RESET_IND       modem_driver_plat_data->gpios[MODEM_MTK_BOOT_RESET_IND].gpio

//COMM GPIO
#define GPIO_WAKEUP_SLAVE       modem_driver_plat_data->gpios[COMM_MTK_WAKEUP_SLAVE].gpio
#define GPIO_WAKEUP_HOST       modem_driver_plat_data->gpios[COMM_MTK_WAKEUP_HOST].gpio

//Rename modem indication GPIO
#define GPIO_CP_PM_INDICATION    GPIO_POWER_IND  //AP: GPIO_021_MTK_PWRON_STAT; CP: UCTS1_B,GPIO24;
#define GPIO_CP_SW_INDICATION    GPIO_CP_SOFTWARE_STATE  //AP: GPIO_026_SOFTWARE_STATE; CP: URTS1_B,GPIO25;

#define MODEM_BOOT_DOWNLOAD_EN    MODEM_MTK_BOOT_DOWNLOAD_EN


static struct modem_mtk6252_platform_data *modem_driver_plat_data=NULL;
static struct STRUCT_MODEM_BOOT modem_boot;


//schedule a work to send uevent
static void uevent_work_func(struct work_struct *data)
{
    struct STRUCT_MODEM_BOOT *my_boot=&modem_boot;
    unsigned long flags;

    while (my_boot->head != my_boot->tail) {
        kobject_uevent_env(&modem_boot.pdev->dev.kobj, KOBJ_CHANGE, &modem_boot.envp[my_boot->head][0] );
        dev_info(&modem_boot.pdev->dev, "%s. notify event[%d,%d] %s to userspace!\n",__func__,
                                my_boot->tail, my_boot->head, modem_boot.envp[my_boot->head][0]);

        spin_lock_irqsave(&modem_boot.lock, flags);
        modem_boot.envp[my_boot->head][0] = NULL;
        my_boot->head = (my_boot->head + 1) % QUEUE_NUM;
        spin_unlock_irqrestore(&modem_boot.lock, flags);
    }
}

/*
 Send the uevent notify message to userspace (rild)
*/
static int modem_monitor_uevent_notify(int event)
{
    struct STRUCT_MODEM_BOOT *my_boot=&modem_boot;
    int temp=0;

    temp = (my_boot->tail + 1) % QUEUE_NUM;
    if (temp != my_boot->head) {
        if(event>=MODEM_STATE_OFF && event<MODEM_STATE_INVALID)
            modem_boot.envp[my_boot->tail][0] = (char *)modem_state_str[event];
        else
            modem_boot.envp[my_boot->tail][0] = (char *)modem_state_str[MODEM_STATE_INVALID];
        modem_boot.envp[my_boot->tail][1] = NULL;

        my_boot->tail = temp;
    }

    schedule_work(&modem_boot.uevent_work);

    return ( (QUEUE_NUM+my_boot->head-my_boot->tail)%QUEUE_NUM );    //queue available room
}

/*
 * GPIO_CP_PM_INDICATION hard irq
*/
static irqreturn_t mdm_pm_state_isr(int irq, void *dev_id)
{
    struct STRUCT_MODEM_BOOT *my_boot=dev_id;
    unsigned long flags;
    int mdm_state=-1;
    int gpio_val=gpio_get_value(GPIO_CP_PM_INDICATION);

    if( dev_id==NULL ) {
        pr_err_once("MODEM# %s. IRQ handler for GPIO_CP_PM_INDICATION failed\n",__func__);
        return IRQ_NONE;
    }
    dev_info(&my_boot->pdev->dev, "MODEM# GPIO_CP_PM_INDICATION interrupt: %d;\n",gpio_val);

    spin_lock_irqsave(&my_boot->lock, flags);
    if( GPIO_CP_PM_RDY==gpio_val ) {
        my_boot->mdm_state = mdm_state = MODEM_STATE_POWER;
        modem_monitor_uevent_notify(mdm_state);
        spin_unlock_irqrestore(&my_boot->lock, flags);

        dev_dbg(&my_boot->pdev->dev, "MODEM# %s. mdm_state.%d -> Modem PMU power on\n",__func__,mdm_state );
    } else {
        my_boot->mdm_state = mdm_state = MODEM_STATE_OFF;
        modem_monitor_uevent_notify(mdm_state);
        spin_unlock_irqrestore(&my_boot->lock, flags);

        dev_dbg(&my_boot->pdev->dev, "MODEM# %s. mdm_state.%d -> Modem PMU power off\n",__func__,mdm_state);
        /* modem abnormal power off handing */
    }

    return IRQ_HANDLED;
}
/*
 * GPIO_CP_SW_INDICATION hard irq
*/
static irqreturn_t mdm_sw_state_isr(int irq, void *dev_id)
{
    struct STRUCT_MODEM_BOOT *my_boot=dev_id;
    unsigned long flags;
    int mdm_state=-1;
    int gpio_val=gpio_get_value(GPIO_CP_SW_INDICATION);

    if( dev_id==NULL ) {
        pr_err_once("MODEM# %s. IRQ handler for GPIO_CP_SW_INDICATION failed\n",__func__);
        return IRQ_NONE;
    }
    dev_info(&my_boot->pdev->dev, "MODEM# GPIO_CP_SW_INDICATION interrupt: %d;\n",gpio_val);

    spin_lock_irqsave(&my_boot->lock, flags);
    if( GPIO_CP_SW_RDY==gpio_val ) {
        if( my_boot->mdm_state==MODEM_STATE_OFF ) {
            mdm_state = MODEM_STATE_OFF;
            modem_monitor_uevent_notify(mdm_state);
            spin_unlock_irqrestore(&my_boot->lock, flags);

            dev_dbg(&my_boot->pdev->dev, "MODEM# %s. mdm_state.%d -> CP software down\n",__func__, mdm_state );
        } else {
            my_boot->mdm_state = mdm_state = MODEM_STATE_READY;  //Todo, will call wake_up_mdm_ready_ack();
            modem_monitor_uevent_notify(mdm_state);
            spin_unlock_irqrestore(&my_boot->lock, flags);

            dev_dbg(&my_boot->pdev->dev, "MODEM# %s. mdm_state.%d -> CP software ready\n", __func__, mdm_state);
        }
    } else {
        if( my_boot->mdm_state==MODEM_STATE_READY ) {
            my_boot->mdm_state = mdm_state = MODEM_STATE_POWER;
            modem_monitor_uevent_notify(mdm_state);
            spin_unlock_irqrestore(&my_boot->lock, flags);

            dev_dbg(&my_boot->pdev->dev, "MODEM# %s. mdm_state.%d -> CP not ready, notify!\n", __func__, mdm_state );
        } else {
            my_boot->mdm_state = mdm_state = MODEM_STATE_POWER;
            modem_monitor_uevent_notify(mdm_state);
            spin_unlock_irqrestore(&my_boot->lock, flags);

            dev_dbg(&my_boot->pdev->dev, "MODEM# %s. mdm_state.%d -> CP software up\n", __func__, mdm_state );
        }
    }

    return IRQ_HANDLED;
}

#define modem_comm_registered_for_sleep  uart3_comm_registered_for_sleep
#define dev_dbg  dev_info

bool modem_comm_registered_for_sleep(void)
{
    return ( (get_modem_type( MODEM_MTK6252 )>0)?true:false );
}
EXPORT_SYMBOL_GPL(modem_comm_registered_for_sleep);

int uart3_comm_set_gpio_for_suspend(void)
{
    struct STRUCT_MODEM_BOOT *my_boot=&modem_boot;

    if (modem_driver_plat_data) {
        if( gpio_get_value(GPIO_WAKEUP_SLAVE)==GPIO_WAKEUP_SLAVE_ACTIVE ) {
            gpio_set_value(GPIO_WAKEUP_SLAVE, GPIO_WAKEUP_SLAVE_INACTIVE);
            dev_info(&my_boot->pdev->dev, "[%s]- %d -> %d\n",__func__, GPIO_WAKEUP_SLAVE_ACTIVE, GPIO_WAKEUP_SLAVE_INACTIVE);
        }

        if( GPIO_WAKEUP_HOST_ACTIVE==gpio_get_value(GPIO_WAKEUP_HOST) ) {   //my_boot->mdm_wakeup.cawake_value
            dev_err(&my_boot->pdev->dev, "[%s]* GPIO_WAKEUP_HOST ACTIVE now, expect inactive!\n",__func__);
        }
    }

    return 0;
}
EXPORT_SYMBOL_GPL(uart3_comm_set_gpio_for_suspend);

int uart3_comm_set_gpio_for_resume(void)
{
    return 0;
}
EXPORT_SYMBOL_GPL(uart3_comm_set_gpio_for_resume);

int uart3_set_gpio_for_runtime_suspend(void)
{
    struct STRUCT_MODEM_BOOT *my_boot=&modem_boot;

    if(modem_driver_plat_data) {
        if( gpio_get_value(GPIO_WAKEUP_SLAVE)==GPIO_WAKEUP_SLAVE_ACTIVE ) {
            gpio_set_value(GPIO_WAKEUP_SLAVE, GPIO_WAKEUP_SLAVE_INACTIVE);
            dev_dbg(&my_boot->pdev->dev, "[%s]- %d -> %d\n",__func__, GPIO_WAKEUP_SLAVE_ACTIVE, GPIO_WAKEUP_SLAVE_INACTIVE);
        }
    }

    return 0;
}
EXPORT_SYMBOL_GPL(uart3_set_gpio_for_runtime_suspend);

int uart3_set_gpio_for_runtime_resume(void)
{
    struct STRUCT_MODEM_BOOT *my_boot=&modem_boot;

    if( modem_driver_plat_data ) {
        if( gpio_get_value(GPIO_WAKEUP_SLAVE)==GPIO_WAKEUP_SLAVE_INACTIVE ) {
            gpio_set_value(GPIO_WAKEUP_SLAVE, GPIO_WAKEUP_SLAVE_ACTIVE);
            dev_dbg(&my_boot->pdev->dev, "[%s]+ %d -> %d\n",__func__, GPIO_WAKEUP_SLAVE_INACTIVE, GPIO_WAKEUP_SLAVE_ACTIVE);
        }
    }

    return 0;
}
EXPORT_SYMBOL_GPL(uart3_set_gpio_for_runtime_resume);

static irqreturn_t mdm_wakeup_host_isr(int irq, void *dev_id)
{
    struct STRUCT_MODEM_BOOT *my_boot=dev_id;
    int val = gpio_get_value(GPIO_WAKEUP_HOST);

    if( dev_id==NULL ) {
        pr_err_once("MODEM# %s. IRQ handler for GPIO_WAKEUP_HOST failed\n",__func__);
        return IRQ_NONE;
    }

    if (GPIO_WAKEUP_HOST_ACTIVE==val) {
        dev_info(&my_boot->pdev->dev, "[%s] Wakeup host active[%d->%d]\n", __func__,my_boot->mdm_wakeup.cawake_value,val);
        wake_lock(&my_boot->mdm_wakeup.wake_host_lock);
    } else {
        dev_info(&my_boot->pdev->dev, "[%s] Wakeup host inactive[%d->%d]\n", __func__,my_boot->mdm_wakeup.cawake_value,val);
        wake_unlock(&my_boot->mdm_wakeup.wake_host_lock);
    }
    my_boot->mdm_wakeup.cawake_value = val;

    return IRQ_HANDLED;
}


/* Do CP power on */
static void modem_boot_power_on(void)
{
    if( modem_boot.mdm_wakeup.irq_flag==false ) {
        enable_irq( gpio_to_irq(GPIO_CP_PM_INDICATION) );
        enable_irq( gpio_to_irq(GPIO_CP_SW_INDICATION) );
        enable_irq( gpio_to_irq(GPIO_WAKEUP_HOST) );
        modem_boot.mdm_wakeup.irq_flag = true;
    }

    gpio_set_value(GPIO_CP_RESET, 0);   //inverse logcic, set MTK_RESETB high;
    gpio_set_value(GPIO_CP_POWER_ON, 1);    //inverse logcic, Pull down G_KEY_PWRON
    msleep(1000);  //250+20 is not enough, 300ms work fine, but MTK recommend 1000ms for safe;
    gpio_set_value(GPIO_CP_POWER_ON, 0);    //inverse logcic, Pull up G_KEY_PWRON
}

/* Do CP power off */
static void modem_boot_power_off(void)
{
    if( modem_boot.mdm_wakeup.irq_flag==true ) {
        disable_irq( gpio_to_irq(GPIO_CP_PM_INDICATION) );
        disable_irq( gpio_to_irq(GPIO_CP_SW_INDICATION) );
        disable_irq( gpio_to_irq(GPIO_WAKEUP_HOST) );
        modem_boot.mdm_wakeup.irq_flag = false;
    }
    wake_unlock(&modem_boot.mdm_wakeup.wake_host_lock);

    gpio_set_value(GPIO_CP_POWER_ON, 0);   //Remove power on condition;
    gpio_set_value(GPIO_CP_RESET, 0);   //inverse logcic, MTK_RESETB high level trigger hardware power off;
    msleep(200);
    gpio_set_value(GPIO_CP_RESET, 1);
}

static DEFINE_MUTEX(boot_state_mutex);

static inline void modem_boot_change_state(int state)
{
    pr_info("modem_power: Power state change: %d -> %d", modem_boot.control_state, state);
    modem_boot.control_state = state;
}

static ssize_t modem_boot_get(struct device *dev,
                 struct device_attribute *attr,
                 char *buf)
{
    ssize_t ret=-1;

    mutex_lock(&boot_state_mutex);
    ret = modem_boot.control_state;
    mutex_unlock(&boot_state_mutex);

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

    mutex_lock(&boot_state_mutex);

    if (state == POWER_SET_ON) {
        gpio_set_value(GPIO_CP_DOWNLOAD_EN, 0); //Disable USB download;
        msleep(20);
        modem_boot_change_state(state);
        modem_boot_power_on();
        gpio_set_value(GPIO_WAKEUP_SLAVE, 1);   //Enable GPIO_WAKEUP_SLAVE;
    } else if (state == POWER_SET_OFF) {
        gpio_set_value(GPIO_WAKEUP_SLAVE, 0);   //Disable GPIO_WAKEUP_SLAVE;
        gpio_set_value(GPIO_CP_DOWNLOAD_EN, 0);
        msleep(20);
        modem_boot_change_state(state);
        modem_boot_power_off();
    } else if (state == POWER_SET_DEBUGON) {
        gpio_set_value(GPIO_CP_DOWNLOAD_EN, 1); //Enable USB download;
        msleep(20);
        modem_boot_change_state(state);
        modem_boot_power_on();
    } else if (state == POWER_SET_DEBUGOFF) {
        gpio_set_value(GPIO_CP_DOWNLOAD_EN, 0);
        msleep(20);
        modem_boot_change_state(state);
        modem_boot_power_off();
    } else {
        dev_err(dev, "Power PHY error state. %s\n", buf);
    }

    mutex_unlock(&boot_state_mutex);

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
    int mdm_state=modem_boot.mdm_state;

    if( mdm_state<0 || mdm_state>=MODEM_STATE_INVALID )
        return ( snprintf(buf,PAGE_SIZE,"%s\n", modem_state_str[MODEM_STATE_INVALID]) );
    else
        return ( snprintf(buf, PAGE_SIZE, "%s\n",modem_state_str[mdm_state]) );
}

static DEVICE_ATTR(state, S_IRUGO | S_IWUSR, modem_boot_get, modem_boot_set);
static DEVICE_ATTR(modem_state, S_IRUGO, modem_state_show, NULL);
/* gpio are for debug */
static DEVICE_ATTR(gpio, S_IRUGO | S_IWUSR, modem_gpio_get, modem_gpio_set);

static int modem_boot_probe(struct platform_device *pdev)
{
    int status = -1;

    dev_info(&pdev->dev, "%s. modem_boot_probe\n",__func__);
    modem_boot.pdev = pdev;
    modem_driver_plat_data = pdev->dev.platform_data;
        //iomux_block_set_work_mode( modem_driver_plat_data->block_name );  //Todo this later;
    if (modem_driver_plat_data) {
        status = gpio_request_array(modem_driver_plat_data->gpios, MODEM_MTK_GPIO_NUM);
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

    spin_lock_init( &modem_boot.lock );
    modem_boot.tail = modem_boot.head = 0;
    INIT_WORK(&modem_boot.uevent_work, uevent_work_func);

//Modem state GPIO indication
    //GPIO_CP_PM_INDICATION
    gpio_direction_input(GPIO_CP_PM_INDICATION);
    /* GPIO_CP_PM_INDICATION interrupt registration */
    if (request_irq(gpio_to_irq(GPIO_CP_PM_INDICATION), mdm_pm_state_isr,
            IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "mdm_pm_state_isr", &modem_boot) < 0) {
        dev_err(&pdev->dev, "MODEM# ERROR requesting GPIO_CP_PM_INDICATION IRQ\n");
    }
    enable_irq_wake( gpio_to_irq(GPIO_CP_PM_INDICATION) );

    //GPIO_CP_SW_INDICATION
    gpio_direction_input(GPIO_CP_SW_INDICATION);
    /* GPIO_CP_SW_INDICATION interrupt registration */
    if (request_irq(gpio_to_irq(GPIO_CP_SW_INDICATION), mdm_sw_state_isr,
            IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "mdm_sw_state_isr", &modem_boot) < 0) {
        dev_err(&pdev->dev, "MODEM# ERROR requesting GPIO_CP_SW_INDICATION IRQ\n");
    }
    enable_irq_wake( gpio_to_irq(GPIO_CP_SW_INDICATION) );

//Modem GPIO sleep wakeup
    //wake_lock for wakeup
    wake_lock_init(&modem_boot.mdm_wakeup.wake_host_lock, WAKE_LOCK_SUSPEND, MODEM_MTK6252);
   /* Register IRQ for HOST_WAKEUP gpio */
   if (request_irq(gpio_to_irq(GPIO_WAKEUP_HOST), mdm_wakeup_host_isr,
            IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
            "mdm_wakeup_host_isr", &modem_boot) < 0) {
       dev_err(&pdev->dev, "MODEM# ERROR requesting GPIO_WAKEUP_HOST IRQ\n");
       modem_boot.mdm_wakeup.irq_flag = -1;
   } else
       modem_boot.mdm_wakeup.irq_flag = true;

    dev_info(&pdev->dev, "%s. Call modem_boot_set(&pdev->dev,NULL,\"2\",1) now\n", __func__);
    status = modem_boot_set(&pdev->dev,NULL,"2",1); //power off modem at the init time;
    if( status!=1 ) {
        dev_err(&pdev->dev, "Failed to call modem_boot_set(&pdev->dev,NULL,\"2\",1)\n");
    }

    return 0;

error:
    if (modem_driver_plat_data) {
        gpio_free_array(modem_driver_plat_data->gpios, MODEM_MTK_GPIO_NUM);
    }
    device_remove_file(&(pdev->dev), &dev_attr_state);
    device_remove_file(&(pdev->dev), &dev_attr_gpio);

    return status;
}

static int modem_boot_remove(struct platform_device *pdev)
{
    dev_dbg(&pdev->dev, "modem_boot_remove\n");

    device_remove_file(&(pdev->dev), &dev_attr_state);
    device_remove_file(&(pdev->dev), &dev_attr_gpio);
    if (modem_driver_plat_data) {
        gpio_free_array(modem_driver_plat_data->gpios, MODEM_MTK_GPIO_NUM);
    }

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
        .name = MODEM_DEVICE_BOOT(MODEM_MTK6252),    //"mtk6252_boot",
        .owner = THIS_MODULE,
    },
};

static int __init modem_boot_init(void)
{
    int ret = -1;

        ret = get_modem_type( MODEM_MTK6252 );
        pr_info("MODEM#%s: get_modem_type \"%s\". %d\n",MODEM_DEVICE_BOOT(MODEM_MTK6252),MODEM_MTK6252,ret);
        if( ret<=0 ) {
            pr_err("MODEM#%s: modem \"%s\" not support on this board. %d\n",MODEM_DEVICE_BOOT(MODEM_MTK6252),MODEM_MTK6252,ret);
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

