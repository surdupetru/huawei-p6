#include <linux/module.h>   
#include <linux/init.h>		
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/sc16is750_i2c.h>
#include <linux/i2c-dev.h>
#include <linux/fcntl.h>
#include <linux/i2c.h>
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <mach/platform.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <asm/current.h>
#include <linux/string.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/unistd.h>
#include <linux/spinlock.h>
#include <linux/mux.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

#ifndef IRDA_DEBUG_LOG
#define IRDA_DEBUG_LOG
#endif

#ifdef IRDA_DEBUG_LOG
#define IRDA_PRINTK printk
#else
#define IRDA_PRINTK  
#endif


/*Interrupt mode operation and Polled mode operation can be selected according to sc16is750 spec
enable this macro means Interrupt modem,while disable this macro means Polled mode.*/
#define IRQ_INTERRUPT_MODE

#define GPIO_IRDA_BLOCK_NAME "block_irda"

#define IOCTL_SET_BAUD_RATE  0xE9000000
#define IOCTL_GET_INFRARED_STATE  0xE9000001
#define IOCTL_SET_INFRARED_STATE  0xE9000002

typedef enum baudrate_type
{
    BAUD_RATE_NOT_SET,
    BAUD_RATE_9600,
    BAUD_RATE_19200,
    BAUD_RATE_38400,
    BAUD_RATE_57600,
    BAUD_RATE_115200,
    BAUD_RATE_76000  /*For Irda Romote Control 38KHz*/
}baudrate_type_e;

static int irda_curr_baudrate = BAUD_RATE_9600;

typedef enum irda_state
{
    IRDA_STATE_OFF,
    IRDA_STATE_ON
}irda_state_e;

int irda_power_state = IRDA_STATE_OFF; /*sc16is750 is power off by default*/


typedef enum enum_irda_mdm_state
{
    IRDA_STATE_BLOCK,
    IRDA_STATE_NONBLOCK
}enum_irda_mdm_state_e;

int irda_mdm_state_current = IRDA_STATE_NONBLOCK;


static int irda_irq;

static struct task_struct *intr_thread;

struct driver_data
{
	struct semaphore i2c_lock;
	struct cdev *i2c_cdev;
	bool s_flag;
	int WRITE_COUNT_FLAG;
	int WRITE_COUNT;
	struct i2c_client *dev_client;
	struct iomux_block *gpio_block;
	struct block_config *gpio_block_config;
};

struct driver_data *driverdata;

char *sent_data;



#define MAX_QUEUE_LEN    70

struct irda_receive_element
{
     char received_data[MAX_FIFO+1];
     int received_data_len;
};

struct irda_receive_element irda_queue_list[MAX_QUEUE_LEN];

int received_data_point = 0; /*next element to store received data in array irda_queue_list[MAX_QUEUE_LEN]*/
int fetch_data_point = 0; /*next element to fetch received data in array irda_queue_list[MAX_QUEUE_LEN]*/
static int overrun_counter = 0;//counter for overrun to print debug info

static struct class *irda_class;
static struct device *irda_dev;


/***************************** FUNCTION DECLARATIONS **************************************/
dev_t createdev(int, int);

int allocatebuffer_mem(void);
void freebuffer_mem(void);

static int __init i2c_sc16is750_init(void);
static void __exit i2c_sc16is750_exit(void);

static int sc16is750_probe(struct i2c_client *, const struct i2c_device_id *);
static int sc16is750_remove(struct i2c_client *);


static int sc16is750_state_on(baudrate_type_e init_bd);
static int sc16is750_state_off(void);
static int sc16is750_get_state(void);

static int i2c_irda_open(struct inode *inode, struct file *filp);
static int i2c_irda_release(struct inode *inode, struct file *filp);
static ssize_t i2c_irda_write(struct file *filp, const char * ch, size_t count, loff_t *offp);
static ssize_t i2c_irda_read(struct file *filp, char * ch, size_t count, loff_t *offp);
static long irda_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static int sc16is750_resume(struct i2c_client *pdev);
static int sc16is750_suspend(struct i2c_client *pdev, pm_message_t msg);

static int i2c_irda_set_baudrate(baudrate_type_e bd_type);
static int i2c_irda_init_register(void);

static ssize_t attr_irda_mmi_write(struct device *dev, struct device_attribute *attr,const char *buf, size_t size);
static ssize_t attr_irda_mmi_read(struct device *dev, struct device_attribute *attr,char *buf);
static ssize_t attr_irda_mdm_state_write(struct device *dev, struct device_attribute *attr,const char *buf, size_t size);
static ssize_t attr_irda_mdm_state_read(struct device *dev, struct device_attribute *attr,char *buf);


static ssize_t attr_irda_debug_test(struct device *dev, struct device_attribute *attr,char *buf);
static ssize_t attr_irda_suspend_test(struct device *dev, struct device_attribute *attr,char *buf);
static ssize_t attr_irda_resume_test(struct device *dev, struct device_attribute *attr,char *buf);
static ssize_t attr_irda_state_on_test(struct device *dev, struct device_attribute *attr,char *buf);
static ssize_t attr_irda_state_off_test(struct device *dev, struct device_attribute *attr,char *buf);
static ssize_t attr_irda_get_state_test(struct device *dev, struct device_attribute *attr,char *buf);


//Interrupt handled via a thread by this function
int handle_interrupt(void *data);

#ifdef IRQ_INTERRUPT_MODE
int interrupt_flag = false;
//Interrupt handler to trigger threading
irqreturn_t executethread(int irq, void *dev_id);
#endif
/***************************** FUNCTION DECLARATIONS **************************************/




static struct device_attribute irda_attr[] ={
	__ATTR(irda_mmi_test, 0664,attr_irda_mmi_read,attr_irda_mmi_write),
	__ATTR(irda_mdm_state, 0664,attr_irda_mdm_state_read,attr_irda_mdm_state_write),
	__ATTR(irda_debug_test, 0664,attr_irda_debug_test,attr_irda_debug_test),
	__ATTR(irda_suspend_test, 0664,attr_irda_suspend_test,attr_irda_suspend_test),
	__ATTR(irda_resume_test, 0664,attr_irda_resume_test,attr_irda_resume_test),
	__ATTR(irda_state_on_test, 0664,attr_irda_state_on_test,attr_irda_state_on_test),
	__ATTR(irda_state_off_test, 0664,attr_irda_state_off_test,attr_irda_state_off_test),
	__ATTR(irda_get_state_test, 0664,attr_irda_get_state_test,attr_irda_get_state_test),
};


static const struct i2c_device_id sc16is750_id[] =
{
	{
		"sc16is750_i2c", 0
	},
	{ }
};

static struct i2c_driver i2cdev_driver =
{
	.driver =
		{
                     .owner	= THIS_MODULE,
			.name	= "sc16is750_i2c",
		},
	.probe = sc16is750_probe,
	.remove = sc16is750_remove,
	.suspend = sc16is750_suspend,
	.resume = sc16is750_resume,
	.id_table = sc16is750_id,
};

static struct file_operations i2c_fops =
{
	owner: THIS_MODULE,
	open: i2c_irda_open,
	release: i2c_irda_release,
	read: i2c_irda_read,
	write: i2c_irda_write,
	.unlocked_ioctl = irda_ioctl,
};

/*use camera gpio for trigger when debugging irda problems*/
#define IRDA_TRIGGER_GPIO    GPIO_13_7   
int trigger_gpio_init_status = false;

int sc16is750_set_trigger_gpio(int trigger)
{
#if 1
    int ret;
    if(trigger_gpio_init_status == false)
    {
        /*enable power config*/
        ret = gpio_request(IRDA_TRIGGER_GPIO, "irda_trigger_gpio");
        if (ret) {
            IRDA_PRINTK(KERN_ERR "IRDA request IRDA_TRIGGER_GPIO fail err=%d.\n", ret);
            return ret;
        }
        trigger_gpio_init_status = true;
    }

    ret = gpio_direction_output(IRDA_TRIGGER_GPIO,trigger);
    if (ret) {
        IRDA_PRINTK(KERN_ERR "IRDA set IRDA_TRIGGER_GPIO trigger=%d fail err=%d.\n", trigger, ret);
        return ret;
    }
    return 0;
#else
    return 0;
#endif
}


dev_t createdev(int MAJOR, int MINOR)
{
	dev_t dev;
	dev = MKDEV(MAJOR, MINOR);
	return dev;
}


int allocatebuffer_mem()
{
	/*2KB of buffer to copy the data from user to kernel space when data is being transmitted by the master (sc6is750).
	 The buffer size can always to changed but care should be taken with the size of data transmitted at once.*/
	sent_data = kzalloc(MAX_SENT, GFP_KERNEL);
	if(sent_data == NULL)
		return -ENOMEM;

	driverdata = kzalloc(sizeof(struct driver_data), GFP_KERNEL);
	if(driverdata == NULL)
    {
        kfree (sent_data);
        return -ENOMEM;
    }

	return 0;
}

void freebuffer_mem()
{
	kfree (sent_data);
}

static int create_sysfs_interfaces(struct device *dev)
{
       int i;
       for (i = 0; i < ARRAY_SIZE(irda_attr); i++)
	          if (device_create_file(dev, irda_attr + i))
		       goto error;
       return 0;
error:
	for ( ; i >= 0; i--)
		   device_remove_file(dev, irda_attr + i);
       IRDA_PRINTK(KERN_ERR "IRDA unable to create sysfs interface.\n" );
       return -1;
}

static int remove_sysfs_interfaces(struct device *dev)
{
       int i;
       for (i = 0; i < ARRAY_SIZE(irda_attr); i++)
	           device_remove_file(dev, irda_attr + i);
       return 0;
}

static int create_cdev_interface()
{
	irda_class = class_create(THIS_MODULE, "irda_class");
	if (IS_ERR(irda_class)) {
		IRDA_PRINTK(KERN_ERR "IRDA create class err=%ld.\n", PTR_ERR(irda_class));
              irda_class = NULL;
              return -1;
	}
	irda_dev = device_create(irda_class, NULL,
				     MKDEV(I2C_MAJOR_NUM,I2C_MINOR_NUM), NULL, "irda_device", 0);
	if (IS_ERR(irda_dev)) {
		IRDA_PRINTK(KERN_ERR "IRDA create device err=%ld.\n",PTR_ERR(irda_dev));
		irda_dev = NULL;
              return -1;
	}

       return 0;
}

int sc16is750_control_power_supply(int powerstate)
{
       int ret;
	ret = gpio_direction_output(IRDA_2V85EN,powerstate);
       if (ret) {
            IRDA_PRINTK(KERN_ERR "IRDA sc16is750_control_power_supply() powerstate=%d err=%d.\n", powerstate, ret);
            return -1;
        }
       return 0;
}

int sc16is750_reset(void)
{
        int ret = 0;

        ret = gpio_direction_output(IRDA_RESET,0);
        if (ret) {
            IRDA_PRINTK(KERN_ERR "IRDA sc16is750_reset() fail output lower err=%d.\n", ret);
            return -1;
        }

        msleep (100);
	 ret = gpio_direction_output(IRDA_RESET,1);
        if (ret) {
            IRDA_PRINTK(KERN_ERR "IRDA sc16is750_reset() fail output high err=%d.\n", ret);
            return -1;
       }
        
      return 0;
    
}



static int sc16is750_state_on(baudrate_type_e init_bd)
{

    IRDA_PRINTK(KERN_DEBUG "IRDA sc16is750_state_on() when irda_power_state=%d.\n", irda_power_state);

    if(IRDA_STATE_OFF == irda_power_state)
    {
        if(0 != blockmux_set(driverdata->gpio_block, driverdata->gpio_block_config, NORMAL) )
        {
            IRDA_PRINTK(KERN_ERR "IRDA blockmux_set() error when state_on.\n");
            return -1;
        }

       if(0 != sc16is750_control_power_supply(true) )
       {
            IRDA_PRINTK(KERN_ERR "IRDA poweron fail when state_on.\n");
            return -1;
       }
       
       if( 0 != sc16is750_reset() )
       {
            IRDA_PRINTK(KERN_ERR "IRDA reset error when state_on.\n");
            return -1;
       }

        enable_irq(irda_irq);

        if(0 != i2c_irda_set_baudrate(init_bd) )
        {
            IRDA_PRINTK(KERN_ERR "IRDA set baudrate 9600 error when state_on.\n");
            return -1;
        }

        if(0 != i2c_irda_init_register() )
        {
            IRDA_PRINTK(KERN_ERR "IRDA init register error when state_on.\n");
            return -1;
        }

        irda_power_state = IRDA_STATE_ON;

    }
    
    
    return 0;

}


static int sc16is750_state_off(void)
{

    int ret;

    IRDA_PRINTK(KERN_DEBUG "IRDA sc16is750_state_off() when irda_power_state=%d.\n",irda_power_state);

    if(IRDA_STATE_ON == irda_power_state)
    {
        irda_power_state = IRDA_STATE_OFF;
        
        memset(irda_queue_list, 0x0, sizeof(struct irda_receive_element)*MAX_QUEUE_LEN );
        received_data_point = 0;
        fetch_data_point = 0;

        gpio_direction_input(IRDA_2V85EN);//config IRDA_2V85EN input mode
        //gpio_direction_input(IRDA_SHDN);//config IRDA_SHDN input mode
        gpio_direction_input(IRDA_RESET);//config IRDA_RESET input mode
        //gpio_direction_input(IRDA_IRQ);//config IRDA_IRQ input mode

        ret = blockmux_set(driverdata->gpio_block, driverdata->gpio_block_config, LOWPOWER);
        if (ret) {
            IRDA_PRINTK(KERN_ERR "IRDA sc16is750_state_off() blockmux_set error.\n");
            return -1;
        } 

        disable_irq_nosync(irda_irq);
    }

    
    return 0;
}


static int sc16is750_get_state(void)
{
    IRDA_PRINTK(KERN_DEBUG "IRDA sc16is750_get_state() = %d.\n",irda_power_state);
    return irda_power_state;
}



static int sc16is750_resume(struct i2c_client *pdev)
{
        int ret;

        if(IRDA_STATE_ON == irda_power_state)
        {
            ret = blockmux_set(driverdata->gpio_block, driverdata->gpio_block_config, NORMAL);
            if (ret) {
                IRDA_PRINTK(KERN_ERR "IRDA blockmux_set error when resume.\n");
                return -1;
            }
           if(0 != sc16is750_control_power_supply(true) )
           {
                IRDA_PRINTK(KERN_ERR "IRDA poweron error when resume.\n");
                return -1;
           }
           if( 0 != sc16is750_reset() )
           {
                IRDA_PRINTK(KERN_ERR "IRDA reset error when resume.\n");
                return -1;
           }
            if(0 != i2c_irda_set_baudrate(irda_curr_baudrate) )
            {
                return -1;
            }

            if(0 != i2c_irda_init_register() ) 
            {
                return -1;
            }

            enable_irq(irda_irq);

        }

        IRDA_PRINTK(KERN_DEBUG "IRDA resume success when irda_power_state=%d.\n", irda_power_state);
        return 0;

}


static int sc16is750_suspend(struct i2c_client *pdev, pm_message_t msg)
{
        int ret;

        if(IRDA_STATE_ON == irda_power_state)
        {
            disable_irq_nosync(irda_irq);
        
            gpio_direction_input(IRDA_2V85EN);//config IRDA_2V85EN input mode
            //gpio_direction_input(IRDA_SHDN);//config IRDA_SHDN input mode
            gpio_direction_input(IRDA_RESET);//config IRDA_RESET input mode
            //gpio_direction_input(IRDA_IRQ);//config IRDA_IRQ input mode

            ret = blockmux_set(driverdata->gpio_block, driverdata->gpio_block_config, LOWPOWER);
            if (ret) {
                IRDA_PRINTK(KERN_ERR "IRDA blockmux_set error when suspend.\n");
                return -1;
            }
        }
        
        IRDA_PRINTK(KERN_DEBUG "IRDA suspend success when irda_power_state=%d.\n", irda_power_state);
        
        return 0;
        
}



static int i2c_irda_set_baudrate(baudrate_type_e bd_type)
{
    u_char write_to_reg[2];

    //Set LCR[7] = 1 to allow access of DLL/DLH and set DLL = 0xA9 (UART baud rate : 9600 by default)
    write_to_reg[0] = LCR;
    write_to_reg[1] = SET_BIT(7);
    if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
        return -1;
    
    if(BAUD_RATE_19200 == bd_type)
    {
        write_to_reg[0] = DLL;
        write_to_reg[1] = SET_BIT(2) | SET_BIT(4) | SET_BIT(6);
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
            return -1;

        write_to_reg[0] = DLH;
        write_to_reg[1] = 0x0;
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
            return -1;
        
        irda_curr_baudrate = BAUD_RATE_19200;
        IRDA_PRINTK(KERN_DEBUG "IRDA i2c_irda_set_baudrate() 19200 success.\n");
    }
    else if(BAUD_RATE_38400 == bd_type)
    {
        write_to_reg[0] = DLL;
        write_to_reg[1] = SET_BIT(1)|SET_BIT(3)|SET_BIT(5);
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
            return -1;

        write_to_reg[0] = DLH;
        write_to_reg[1] = 0x0;
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
            return -1;     
        
        irda_curr_baudrate = BAUD_RATE_38400;
        IRDA_PRINTK(KERN_DEBUG "IRDA i2c_irda_set_baudrate() 38400 success.\n");
    }
    else if(BAUD_RATE_57600 == bd_type)
    {
        write_to_reg[0] = DLL;
        write_to_reg[1] = SET_BIT(2)|SET_BIT(3)|SET_BIT(4);
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
            return -1;

        write_to_reg[0] = DLH;
        write_to_reg[1] = 0x0;
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
            return -1;
        
        irda_curr_baudrate = BAUD_RATE_57600;
        IRDA_PRINTK(KERN_DEBUG "IRDA i2c_irda_set_baudrate() 57600 success.\n");
    }
    else if(BAUD_RATE_115200 == bd_type)
    {
        write_to_reg[0] = DLL;
        write_to_reg[1] = SET_BIT(1)|SET_BIT(2)|SET_BIT(3);
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
            return -1;

        write_to_reg[0] = DLH;
        write_to_reg[1] = 0x0;
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
            return -1;
        
        irda_curr_baudrate = BAUD_RATE_115200;
        IRDA_PRINTK(KERN_DEBUG "IRDA i2c_irda_set_baudrate() 115200 success.\n");
    }
    else if(BAUD_RATE_76000 == bd_type)
    {

        write_to_reg[0] = MCR;
#if 1
        write_to_reg[1] = 0;//config for uart mode
#else
        write_to_reg[1] = SET_BIT(6);//config for irda mode
#endif
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
            return -1;

        write_to_reg[0] = DLL;
#if 0
        //divisor = 21,actual baud rate is 77380bps
        write_to_reg[1] = SET_BIT(0)|SET_BIT(2)|SET_BIT(4);
#else
        //divisor = 22,actual baud rate is 73863bps
        write_to_reg[1] = SET_BIT(1)|SET_BIT(2)|SET_BIT(4);
#endif
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
            return -1;

        write_to_reg[0] = DLH;
        write_to_reg[1] = 0x0;
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
            return -1;
        
        irda_curr_baudrate = BAUD_RATE_76000;
        IRDA_PRINTK(KERN_DEBUG "IRDA i2c_irda_set_baudrate() 76000 success.\n");
    }
    else/*use default BAUD_RATE_9600*/
    {
        write_to_reg[0] = DLL;
        write_to_reg[1] = SET_BIT(0)|SET_BIT(3)|SET_BIT(5)|SET_BIT(7);
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
        return -1;

        write_to_reg[0] = DLH;
        write_to_reg[1] = 0x0;
        if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
        return -1;
        
        irda_curr_baudrate = BAUD_RATE_9600;
        IRDA_PRINTK(KERN_DEBUG "IRDA i2c_irda_set_baudrate() 9600 success.\n");
    }

    return 0;
}


/* The different configuration registers are set here. By default the FIFO is enabled and the UART data frame is set to 8N1
   format. Also the Tx/Rx is interrupt driven where interrupt occurs according to the trigger level set in TLR. By default,
   the hardware flow control is not set.
   Note: Please do not try to modify anyy of the register values directly from user end. The driver takes care of them and any 
   change may result in some serious errors*/
static int i2c_irda_init_register(void)
{
    	u_char write_to_reg[2];
        
	//Setting EFR[4] = 1 and MCR[6] = 1 to enable use of enchanced functions
	write_to_reg[0] = LCR;
	write_to_reg[1] = 0xBF; //LCR should be 0xBF to allow access of EFR register
	if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
		return -1;

	write_to_reg[0] = EFR;
	write_to_reg[1] = SET_BIT(4);
	if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
		return -1;

	//LCR[7] is set to '0' and UART data frame is set to 8N1 (default)
	write_to_reg[0] = LCR;
	write_to_reg[1] = SET_BIT(0) | SET_BIT(1); 
	if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
		return -1;

	write_to_reg[0] = MCR;
	write_to_reg[1] = SET_BIT(6);
	if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
		return -1;

	//To exploit the usefulness of FIFO, the Tx/Rx is done using interrupt with FIFO enabled.
	write_to_reg[0] = FCR;
	write_to_reg[1] = SET_BIT(0) | SET_BIT(1) | SET_BIT(2) | SET_BIT(6) ; //FIFO-Reset & Enabled,set trigger level 16 bytes
	if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
		return -1;		
		
#ifdef IRQ_INTERRUPT_MODE
	write_to_reg[0] = IER;
	write_to_reg[1] = SET_BIT(0) | SET_BIT(2);//Enabel Line Status Interrupt, RHR interrupt
	if (i2c_master_send(driverdata->dev_client, write_to_reg, 2) != 2)
		return -1;
#endif
		
        /*clear the receive buffer to init status*/
        memset(irda_queue_list, 0x0, sizeof(struct irda_receive_element)*MAX_QUEUE_LEN );
        received_data_point = 0;
        fetch_data_point = 0;

       return 0;

}


/* The function is invoked when ever 'intr_thread' is waken up, at the time of interrupt arrival. 
   It reads out the Interrupt Identification Register (IIR) to identify the interrupt source. It
   handles 5 different interrupts. 
   @Line Status Interrupt 	: On interrupt due to any error in received UART frame, the actual error
                          	  condition is found by LSR and the user is informed.
   @Receive Holding Interrupt	:
   @Receiver Time-Out Interrupt	: Both these interrupts suggests the arrival of a new data in the Rx
                                  FIFO. The data is read out and is stored in a buffer of size 2KB (2048)
                                  characater. If there is an overflow, the previous data is all lost and the
				  new data is stored at the start location. So it is adviced to read out the 
                                  buffer before any overflow occurs. See ioctl() and read() command.
   @Transmit Holding Interrupt	: THR interrupt is used to send the data to Tx FIFO of sc16is750. The data is 
                                  sent in chunks of MAX_TX_FIFO. 
   @Input pin (GPIO) Interrupt	: If the interrupt is enabled for the acknowledgement of change of state of an
                                  input pin, then this interrupt will occur. It informs the user about the present
                                  state of all pins.
*/
#ifdef IRQ_INTERRUPT_MODE
int handle_interrupt(void *data)
{
    u_char buf[2], iir_value[2];
    int size,i;

    IRDA_PRINTK(KERN_DEBUG "IRDA handle_interrupt() thread enter for interrupt mode.\n");

    while(1){

        //Read out the Interrupt Identification Register
        buf[0] = IIR;
        i2c_master_send(driverdata->dev_client,buf,1);
        i2c_master_recv(driverdata->dev_client, iir_value, 1);
cases:
        //Receiver Line Status Interrupt, RHR interrupt and Receiver time-out interrupt
        if(GET_BIT(iir_value[0], 2))
        {
            down(&driverdata->i2c_lock);
            if(GET_BIT(iir_value[0], 1))
            {
                buf[0] = LSR;
                i2c_master_send(driverdata->dev_client, buf, 1);
                i2c_master_recv(driverdata->dev_client, buf, 1);
                if(GET_BIT(buf[0], 1))
                {
                    IRDA_PRINTK(KERN_ERR "IRDA overrun error occurs, LSR=0x%02X.\n", buf[0]);
                    overrun_counter++;
                    //sc16is750_set_trigger_gpio(0);
                }
                if(GET_BIT(buf[0], 2))
                    IRDA_PRINTK(KERN_DEBUG "LSR Interrupt: Parity error\n");
                if(GET_BIT(buf[0], 3))
                    IRDA_PRINTK(KERN_DEBUG "LSR Interrupt: Framing error\n");
                if(GET_BIT(buf[0], 4))
                    IRDA_PRINTK(KERN_DEBUG "LSR Interrupt: Break error\n");
            }

            buf[0] = RXLVL;
            i2c_master_send(driverdata->dev_client, buf, 1);
            i2c_master_recv(driverdata->dev_client, buf, 1);
            size = (int)buf[0];
	    if (unlikely(buf[0] > MAX_FIFO)) {
		    printk("%s error size=%d, set it to %d\n", __FUNCTION__, size, MAX_FIFO);
		    size = MAX_FIFO;
	    }

            buf[0] = RHR;
            i2c_master_send(driverdata->dev_client, buf, 1);

            i2c_master_recv(driverdata->dev_client, irda_queue_list[received_data_point].received_data, size);
            irda_queue_list[received_data_point].received_data_len = size;

#if 0
            for(i=0;i<size;i++)
            {
                IRDA_PRINTK(KERN_DEBUG "0x%02X",k_buffer[i]);
            }
            IRDA_PRINTK(KERN_DEBUG "IRDA handle_interrupt() %d bytes had been received.\n", size);
#endif

            if(++received_data_point == MAX_QUEUE_LEN)
            received_data_point = 0;

            up(&driverdata->i2c_lock);

        }

        interrupt_flag = false;
        buf[0] = IIR;
        i2c_master_send(driverdata->dev_client,buf,1);
        i2c_master_recv(driverdata->dev_client, iir_value, 1);

        //Put this thread to sleep if there is no interrupt
        set_current_state(TASK_UNINTERRUPTIBLE);
        if( (iir_value[0]& 0x01) || (IRDA_STATE_OFF == irda_power_state) )
        {
            if(!interrupt_flag)
                schedule();
        }
        else
            goto cases;
    }

    return 0;

}

/* When an interrupt occurs, the thread called 'intr_thread' is waken up. The thread then
   read out the interrupt identification register to decide which operation to perform.
   For details on the operations, see the 'handle_interrupt' function'  
*/
irqreturn_t executethread(int irq, void *dev_id)
{
       interrupt_flag = true;
       if(IRDA_STATE_ON == irda_power_state)
	    wake_up_process(intr_thread);
       //IRDA_PRINTK(KERN_DEBUG "IRDA executethread() enter irda_power_state=%d.\n",irda_power_state);
	return IRQ_HANDLED;
}


#else /*else for polled mode*/

int handle_interrupt(void *data)
{
    u_char buf[2], iir_value[2];
    unsigned char size = 0;
    int i;

    IRDA_PRINTK(KERN_DEBUG "IRDA handle_interrupt() thread enter for polled mode.\n");

    while(1)
    {
        down(&driverdata->i2c_lock);   

        buf[0] = LSR;
        i2c_master_send(driverdata->dev_client, buf, 1);
        i2c_master_recv(driverdata->dev_client, buf, 1);
        if( !GET_BIT( buf[0], 0) )
        {
            up(&driverdata->i2c_lock);
            continue;
        }

        //overrun case, serial error
        if( GET_BIT(buf[0], 1) )
        {
            IRDA_PRINTK(KERN_ERR "IRDA overrun error occurs, LSR=0x%02X.\n", buf[0]);
            overrun_counter++;
            //sc16is750_set_trigger_gpio(0);
        }

        buf[0] = RXLVL;
        i2c_master_send(driverdata->dev_client, buf, 1);
        i2c_master_recv(driverdata->dev_client, buf, 1);
        size = (unsigned char)buf[0];
        //size = size & 0x7F;//bit 7 of RXLVL is not used,and clear to zero

        if(size <= 0 || size > MAX_FIFO)
        {
            if( size > MAX_FIFO )
                IRDA_PRINTK(KERN_ERR "IRDA read RXLVL FIFO error size=%d.\n",size);
            up(&driverdata->i2c_lock);
            continue;
        }

        buf[0] = RHR;
        i2c_master_send(driverdata->dev_client, buf, 1);


        i2c_master_recv(driverdata->dev_client, irda_queue_list[received_data_point].received_data, size);
        irda_queue_list[received_data_point].received_data_len = size;

#if 0
        for(i=0;i<size;i++)
        {
            IRDA_PRINTK(KERN_DEBUG "0x%02X",k_buffer[i]);
        }
        IRDA_PRINTK(KERN_DEBUG "IRDA handle_interrupt() %d bytes had been received.\n", size);
#endif

        if(++received_data_point == MAX_QUEUE_LEN)
            received_data_point = 0;

        up(&driverdata->i2c_lock);
    }

    return 0;
}

#endif


static int i2c_irda_open(struct inode *inode, struct file *filp)
{	

        if(irda_mdm_state_current != IRDA_STATE_NONBLOCK)
        {
            IRDA_PRINTK(KERN_ERR "IRDA i2c_irda_open() error when blocked.\n");
            return -1;
        }
            

        if(IRDA_STATE_OFF == irda_power_state)
        {
            if(0 != blockmux_set(driverdata->gpio_block, driverdata->gpio_block_config, NORMAL) )
            {
                IRDA_PRINTK(KERN_ERR "IRDA blockmux_set() error when open.\n");
                return -1;
            }

           if(0 != sc16is750_control_power_supply(true) )
           {
                IRDA_PRINTK(KERN_ERR "IRDA poweron fail when open.\n");
                return -1;
           }
           
           if( 0 != sc16is750_reset() )
           {
                IRDA_PRINTK(KERN_ERR "IRDA reset error when open.\n");
                return -1;
           }
           
            enable_irq(irda_irq);
            
        }

        if(0 != i2c_irda_set_baudrate(BAUD_RATE_9600) )
        {
            IRDA_PRINTK(KERN_ERR "IRDA set baudrate 9600 error when open.\n");
            
            if( 0 != sc16is750_reset() )
            {
                IRDA_PRINTK(KERN_ERR "IRDA reset error when reset baudrate 9600.\n");
                return -1;
            }

            if(0 != i2c_irda_set_baudrate(BAUD_RATE_9600) )
            {
                IRDA_PRINTK(KERN_ERR "IRDA reset baudrate 9600 error again when open.\n");
                return -1;
            }
            else
            {
                IRDA_PRINTK(KERN_ERR "IRDA reset baudrate 9600 success when open.\n");
            }
                
        }

        if(0 != i2c_irda_init_register() )
        {
            IRDA_PRINTK(KERN_ERR "IRDA init register error when open.\n");
            return -1;
        }

        irda_power_state = IRDA_STATE_ON;

        IRDA_PRINTK(KERN_DEBUG "IRDA i2c_irda_open() success.\n");
        
        return 0;
}



static int i2c_irda_release(struct inode *inode, struct file *filp)
{

    int ret;

    IRDA_PRINTK(KERN_DEBUG "IRDA i2c_irda_release() when irda_power_state=%d.\n",irda_power_state);

    if(IRDA_STATE_ON == irda_power_state)
    {
        irda_power_state = IRDA_STATE_OFF;
        
        memset(irda_queue_list, 0x0, sizeof(struct irda_receive_element)*MAX_QUEUE_LEN );
        received_data_point = 0;
        fetch_data_point = 0;

        gpio_direction_input(IRDA_2V85EN);//config IRDA_2V85EN input mode
        //gpio_direction_input(IRDA_SHDN);//config IRDA_SHDN input mode
        gpio_direction_input(IRDA_RESET);//config IRDA_RESET input mode
        //gpio_direction_input(IRDA_IRQ);//config IRDA_IRQ input mode

        ret = blockmux_set(driverdata->gpio_block, driverdata->gpio_block_config, LOWPOWER);
        if (ret) {
            IRDA_PRINTK(KERN_ERR "IRDA blockmux_set() error when release.\n");
            return -1;
        } 

        disable_irq_nosync(irda_irq);
        
    }

    return 0;
}


static long irda_ioctl(struct file *filp, unsigned int cmd,unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    int set_baud = 0;
    
    switch (cmd) {
    case IOCTL_SET_BAUD_RATE:
        if (argp == NULL) {
            IRDA_PRINTK(KERN_DEBUG "IRDA irda_ioctl() set baudrate invalid argument.\n");
            return -1;
        }
        get_user(set_baud, (int *)(argp));
        if(0 != i2c_irda_set_baudrate(set_baud) )
        {
            IRDA_PRINTK(KERN_ERR "IRDA irda_ioctl() set baudrate=%d fail.\n", set_baud);
            return -1;
        }
        if(0 != i2c_irda_init_register() )
        {
            IRDA_PRINTK(KERN_ERR "IRDA irda_ioctl() init register fail.\n");
            return -1;
        }
    break;
    
    case IOCTL_GET_INFRARED_STATE:
        return sc16is750_get_state();
    break;

    case IOCTL_SET_INFRARED_STATE:
        if( arg == IRDA_STATE_ON )
        {
            sc16is750_state_on(BAUD_RATE_9600);
        }
        else if(arg == IRDA_STATE_OFF)
        {
            sc16is750_state_off();
        }
        else
        {
            IRDA_PRINTK(KERN_DEBUG "IRDA irda_ioctl() set IRDA_STATE invalid argument.\n");
        }
    break;

    default:
        IRDA_PRINTK(KERN_ERR "IRDA Illegal command operation. cmd[ 0x%04x ].\n", cmd);
        return -1;
    break;
    
    }

    return 0;
}



/* The write() call will write to the registers of the sc16is750. In case the data has to be written
   to the Tx FIFO, the data is to copied into a buffer (limited by size MAX_SENT) and THR interrupt
   is enabled. This will wake up the 'intr_thread' and the data transfer will start in chunks of 
   MAX_TX_FIFO.
*/
static ssize_t i2c_irda_write(struct file *filp, const char  __user *ch, size_t count, loff_t *offp)
{
    u_char  write_to_reg[2];
    u_char  buffer[2];
    u_char txbuffer[MAX_TX_FIFO + 1];
    int availwritebyte = 0;
    char lsr_buffer[2];
    struct i2c_msg irda_msg_lsr[] = {
        {
            .addr = 0,
            .flags = 0,
            .len = 1,
            .buf = 0,
        },
        {
            .addr = 0,
            .flags = I2C_M_RD,
            .len = 1,
            .buf = 0,
        }
    };
    
    lsr_buffer[0] = LSR;
    lsr_buffer[1] = 0;
    irda_msg_lsr[0].addr = driverdata->dev_client->addr;
    irda_msg_lsr[1].addr = driverdata->dev_client->addr;
    irda_msg_lsr[0].buf = lsr_buffer;
    irda_msg_lsr[1].buf = buffer;

    //IRDA_PRINTK(KERN_DEBUG "IRDA i2c_irda_write() count=%d enter.\n",count);
    //sc16is750_set_trigger_gpio(1);

    down(&driverdata->i2c_lock);
    if (count > MAX_SENT)
        count = MAX_SENT;
    if (copy_from_user(sent_data, ch,count))
        return -EFAULT;

    //Writing to THR  Registers
    driverdata->WRITE_COUNT = count;
    driverdata->WRITE_COUNT_FLAG = 0;

    write_to_reg[0] = EFCR;
    i2c_master_send(driverdata->dev_client, write_to_reg, 1);
    i2c_master_recv(driverdata->dev_client, write_to_reg, 1);
    write_to_reg[1] = write_to_reg[0] | SET_BIT(1);//set EFCR[1] = 1 to disable rx
    write_to_reg[0] = EFCR;
    i2c_master_send(driverdata->dev_client, write_to_reg, 2);

    do
    {
        buffer[0] = TXLVL;
        i2c_master_send(driverdata->dev_client, buffer, 1);
        i2c_master_recv(driverdata->dev_client, buffer, 1);
        availwritebyte = buffer[0];

        if(availwritebyte ==0)
        {
            continue;
        }
        else
        {                  
            txbuffer[0] = THR;
            if(driverdata->WRITE_COUNT < availwritebyte)
                availwritebyte = driverdata->WRITE_COUNT;

            memcpy(txbuffer + 1, sent_data + driverdata->WRITE_COUNT_FLAG , availwritebyte);
            driverdata->WRITE_COUNT_FLAG += availwritebyte;
            driverdata->WRITE_COUNT -= availwritebyte;
            i2c_master_send(driverdata->dev_client, txbuffer , availwritebyte + 1);
        }
    }while(driverdata->WRITE_COUNT);

    write_to_reg[1] = write_to_reg[0] & CLEAR_BIT(1); //set EFCR[1] = 1 to enable rx

    read_ISR:
    buffer[0] = LSR;
    i2c_transfer(driverdata->dev_client->adapter, irda_msg_lsr, 2);
    if(GET_BIT(buffer[0], 6))
    {
        i2c_master_send(driverdata->dev_client, write_to_reg, 2);
        //sc16is750_set_trigger_gpio(0);
        IRDA_PRINTK(KERN_DEBUG "IRDA i2c_irda_write() count=%d done.\n",count);
        up(&driverdata->i2c_lock);
    }
    else
        goto read_ISR;

    return count;
}

static ssize_t i2c_irda_write_loop(const char  __user *ch, size_t count)
{
	u_char txbuffer[MAX_TX_FIFO + 1];
	if (count > MAX_SENT)
		count = MAX_SENT;

       memset(sent_data, 0x0, MAX_SENT);

	strncpy(sent_data, ch, count);
   
	//Writing to THR  Registers
	driverdata->WRITE_COUNT = count - 1;
	driverdata->WRITE_COUNT_FLAG = 0;

until_write:
	txbuffer[0] = THR;
	if ((driverdata->WRITE_COUNT <= MAX_TX_FIFO) && (driverdata->WRITE_COUNT != 0))
	{
		 memcpy(txbuffer + 1, sent_data + driverdata->WRITE_COUNT_FLAG , driverdata->WRITE_COUNT);
		 i2c_master_send(driverdata->dev_client, txbuffer , driverdata->WRITE_COUNT + 1);
		 driverdata->WRITE_COUNT_FLAG += driverdata->WRITE_COUNT;
		 driverdata->WRITE_COUNT = 0;

	 }
	 else if(driverdata->WRITE_COUNT > MAX_TX_FIFO)
	 {
		 memcpy(txbuffer + 1, sent_data + driverdata->WRITE_COUNT_FLAG , MAX_TX_FIFO);
		 driverdata->WRITE_COUNT_FLAG += MAX_TX_FIFO;
		 driverdata->WRITE_COUNT -= MAX_TX_FIFO;
		 i2c_master_send(driverdata->dev_client, txbuffer , MAX_TX_FIFO + 1);
		 goto until_write;
	 }

	return count;

}


static ssize_t i2c_irda_read(struct file *filp, char * ch, size_t count, loff_t *offp)
{
      int need_copy_size = 0;
      int tempbuffer_copy[MAX_FIFO + 1];

      if(fetch_data_point == received_data_point)
        return 0;

      if(count >= irda_queue_list[fetch_data_point].received_data_len)
      {
            need_copy_size = irda_queue_list[fetch_data_point].received_data_len;
            copy_to_user(ch,irda_queue_list[fetch_data_point].received_data, irda_queue_list[fetch_data_point].received_data_len);

            if(++fetch_data_point == MAX_QUEUE_LEN)
                fetch_data_point = 0;

            return need_copy_size;
      }
      else
      {
            memcpy(tempbuffer_copy,irda_queue_list[fetch_data_point].received_data + count, irda_queue_list[fetch_data_point].received_data_len - count);
            copy_to_user(ch,irda_queue_list[fetch_data_point].received_data, count);

            memcpy(irda_queue_list[fetch_data_point].received_data, tempbuffer_copy, irda_queue_list[fetch_data_point].received_data_len - count);
            irda_queue_list[fetch_data_point].received_data_len -= count;

            return count;
      }
      
}



static ssize_t attr_irda_mmi_write(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
        int ret;
	 struct inode *inode = NULL;
        struct file *filp = NULL;

	 ret = i2c_irda_open(inode, filp);
	 if(ret){
               IRDA_PRINTK(KERN_ERR "IRDA %s: Failed to open irda.\n", __func__);
               return ret;
	 }
     
        ret = i2c_irda_write_loop(buf, strlen(buf)+1);
	 if(!ret){
               IRDA_PRINTK(KERN_ERR "IRDA %s: Failed to write data to irda.\n", __func__);
               return ret;
	 }

        IRDA_PRINTK(KERN_DEBUG "IRDA %s: write:[%s] size=%d buf[0]=0x%02X  buf[1]=0x%02X  buf[2]=0x%02X\n",__func__,buf,strlen(buf)+1,buf[0],buf[1],buf[2]);

	 return ret;
}

static ssize_t attr_irda_mmi_read(struct device *dev, struct device_attribute *attr,
				char *buf)
{
        int ret, ret0,i;
	 struct inode *inode = NULL;
        struct file *filp = NULL;
        char readbuf[MAX_FIFO+1];/*assume mmi test can read no more than MAX_FIFO bytes data*/

        memset(readbuf, 0x0, MAX_FIFO + 1);

        for(i = 0; i < received_data_point; i++)
            strcat(readbuf, irda_queue_list[i].received_data);

	 ret = sprintf(buf, "%s\n", readbuf);

	 ret0 = i2c_irda_release(inode, filp);
	 if(ret0){
               return ret0;
	 }
     
	 return ret;
}


static ssize_t attr_irda_mdm_state_write(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{

        if(!strncmp(buf,"0\n",size))
            irda_mdm_state_current = IRDA_STATE_BLOCK;
        else if(!strncmp(buf,"1\n",size))
            irda_mdm_state_current = IRDA_STATE_NONBLOCK;
        else
        {
            IRDA_PRINTK(KERN_DEBUG "IRDA attr_irda_mdm_state_write() invalid argument error.\n");
            return -1;
        }
        
        IRDA_PRINTK(KERN_DEBUG "IRDA attr_irda_mdm_state_write() buf=%s size=%d irda_mdm_state_current=%d\n",buf,size,irda_mdm_state_current);
        
	return size;
}

static ssize_t attr_irda_mdm_state_read(struct device *dev, struct device_attribute *attr,
				char *buf)
{
        int ret;
	ret = sprintf(buf, "%d\n", irda_mdm_state_current);
	return ret;
}


static ssize_t attr_irda_debug_test(struct device *dev, struct device_attribute *attr,
				char *buf)
{
    u_char bufs[2];        

    //Read out the Interrupt Identification Register       
    bufs[0] = IIR;
    i2c_master_send(driverdata->dev_client,bufs,1);        
    i2c_master_recv(driverdata->dev_client, bufs, 1);        
    IRDA_PRINTK(KERN_DEBUG "IRDA Register: IIR =0x%02X\n", bufs[0]);    

    
    bufs[0] = LSR;        
    i2c_master_send(driverdata->dev_client, bufs, 1);        
    i2c_master_recv(driverdata->dev_client, bufs, 1);        
    IRDA_PRINTK(KERN_DEBUG "IRDA Register: LSR =0x%02X\n", bufs[0]);   

    
    bufs[0] = RXLVL;        
    i2c_master_send(driverdata->dev_client, bufs, 1);
    i2c_master_recv(driverdata->dev_client, bufs, 1);
    IRDA_PRINTK(KERN_DEBUG "IRDA Register: RXLVL =0x%02X\n", bufs[0]);


    IRDA_PRINTK(KERN_DEBUG "IRDA Register: OVERRUN COUNT = %d\n", overrun_counter);    

    return 0;
    
}


static ssize_t attr_irda_suspend_test(struct device *dev, struct device_attribute *attr,
				char *buf)
{
    pm_message_t t;
    i2cdev_driver.suspend(0, t);
    return 0;
}

static ssize_t attr_irda_resume_test(struct device *dev, struct device_attribute *attr,
				char *buf)
{
    i2cdev_driver.resume(0);
    return 0;
}

static ssize_t attr_irda_state_on_test(struct device *dev, struct device_attribute *attr,
				char *buf)
{
    sc16is750_state_on(BAUD_RATE_9600);
    return 0;
}

static ssize_t attr_irda_state_off_test(struct device *dev, struct device_attribute *attr,
				char *buf)
{
    sc16is750_state_off();
    return 0;
}

static ssize_t attr_irda_get_state_test(struct device *dev, struct device_attribute *attr,
				char *buf)
{
    sc16is750_get_state();
    return 0;
}

static int sc16is750_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;

	driverdata->dev_client = client;
       
	ret = create_cdev_interface();
	if (ret < 0) {
            return ret;
        }
    
	ret = create_sysfs_interfaces(&client->dev);
	if (ret < 0) {
		return ret;
	}

	  ret = gpio_request(IRDA_2V85EN, "irda_power");
        if (ret) {
            IRDA_PRINTK(KERN_ERR "IRDA gpio request IRDA_2V85EN fail.\n");
            return ret;
        }

        ret = gpio_request(IRDA_RESET, "irda_reset");
        if (ret) {
            IRDA_PRINTK(KERN_ERR "IRDA gpio request IRDA_RESET fail.\n");
            return ret;
        }

        ret = gpio_request(IRDA_SHDN, "irda_shdn");
        if (ret) {
            IRDA_PRINTK(KERN_ERR "IRDA gpio request IRDA_SHDN fail.\n");
            return ret;
        }
        
        
        driverdata->gpio_block = iomux_get_block(GPIO_IRDA_BLOCK_NAME);
        if (!driverdata->gpio_block) {
            IRDA_PRINTK(KERN_ERR "IRDA iomux_get_block() error when probe.\n");
            return -1;
        }
        
        driverdata->gpio_block_config= iomux_get_blockconfig(GPIO_IRDA_BLOCK_NAME);
        if (!driverdata->gpio_block_config) {
            IRDA_PRINTK(KERN_ERR "IRDA iomux_get_blockconfig() error when probe.\n");
            return -1;
        }

        gpio_direction_input(IRDA_2V85EN);//config IRDA_2V85EN input mode
        gpio_direction_input(IRDA_SHDN);//config IRDA_SHDN input mode
        gpio_direction_input(IRDA_RESET);//config IRDA_RESET input mode
        //gpio_direction_input(IRDA_IRQ);//config IRDA_IRQ input mode
        ret = blockmux_set(driverdata->gpio_block, driverdata->gpio_block_config, LOWPOWER);
        if (ret) {
            IRDA_PRINTK(KERN_ERR "IRDA blockmux_set() error when probe.\n");
            return -1;
        }

	sema_init(&driverdata->i2c_lock, 1);

#ifdef IRQ_INTERRUPT_MODE
	//If all successful, create a thread to control the sc16is750 interrupt
	intr_thread = kthread_create(handle_interrupt, NULL, "IrdaThread");
	intr_thread->state = TASK_UNINTERRUPTIBLE;
	intr_thread->rt_priority = 99;//set highest priority to make a real time task 

	irda_irq = gpio_to_irq(IRDA_IRQ);
	ret = request_irq(irda_irq, executethread, IRQF_DISABLED|IRQ_TYPE_EDGE_FALLING, "irda_handler", NULL);
	if(ret)
	{
		IRDA_PRINTK(KERN_ERR "IRDA can't acquire Interrupt IRDA_IRQ\n");
		driverdata->s_flag = false;
		return ret;
	}

       disable_irq_nosync(irda_irq);
#else
	//If all successful, create a thread to control the sc16is750 interrupt
	intr_thread = kthread_create(handle_interrupt, NULL, "IrdaThread");
	intr_thread->state = TASK_UNINTERRUPTIBLE;
       intr_thread->rt_priority = 99;//set highest priority to make a real time task 
       wake_up_process(intr_thread);
#endif

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    /* detect current device successful, set the flag as present */
        set_hw_dev_flag(DEV_I2C_IRDA);
#endif

       IRDA_PRINTK(KERN_DEBUG "IRDA sc16is750_probe() success.\n");

	return 0;
}


static int sc16is750_remove(struct i2c_client *client)
{
	free_irq(irda_irq, NULL);
	remove_sysfs_interfaces(&client->dev);
       IRDA_PRINTK(KERN_DEBUG "IRDA sc16is750_remove() success.\n");
	return 0;
}

static int __init i2c_sc16is750_init()
{
	int ret;
	dev_t dev;

	dev = createdev(I2C_MAJOR_NUM,I2C_MINOR_NUM);
	ret = register_chrdev_region(dev, 1, "sc750i2c");
	if(ret)
	{
		IRDA_PRINTK(KERN_ERR "IRDA error allocating chrdev sc750i2c.\n");
		return ret ;
	}

	if(allocatebuffer_mem())
	{
		IRDA_PRINTK(KERN_ERR "IRDA memory allocation for buffer failed\n");
		goto unregister;
	}
    
	driverdata->s_flag= true;
	driverdata->i2c_cdev = cdev_alloc();
	driverdata->i2c_cdev->owner = THIS_MODULE;
	driverdata->i2c_cdev->ops = &i2c_fops;

       if((ret = cdev_add(driverdata->i2c_cdev, dev, 1)))
	{
		IRDA_PRINTK(KERN_ERR "IRDA device registration failure\n");
		goto free_mem;
	}

	if((ret = i2c_add_driver(&i2cdev_driver)))
       {
		IRDA_PRINTK(KERN_ERR "IRDA i2c add driver failure\n");
		goto dev_del;
       }   
		

	if(driverdata->s_flag == false)
	{
		ret = -1;
		goto remove_driver;
	}
    
	IRDA_PRINTK(KERN_DEBUG "IRDA i2c_sc16is750_init() success.\n");

	return 0;

remove_driver:
	i2c_del_driver(&i2cdev_driver);
dev_del:
	cdev_del(driverdata->i2c_cdev);
free_mem:
	freebuffer_mem();
unregister:
	unregister_chrdev_region(createdev(I2C_MAJOR_NUM, I2C_MINOR_NUM), 1);
	return ret;
}


static void __exit i2c_sc16is750_exit()
{
	i2c_del_driver(&i2cdev_driver);
	freebuffer_mem();
	cdev_del(driverdata->i2c_cdev);
	unregister_chrdev_region(createdev(I2C_MAJOR_NUM, I2C_MINOR_NUM), 1);
	IRDA_PRINTK (KERN_DEBUG "IRDA i2c_sc16is750_exit() success.\n");
}


module_init(i2c_sc16is750_init);
module_exit(i2c_sc16is750_exit);

/****************************** MODULE DESCRIPTION ****************************************/

MODULE_AUTHOR("Malay Jajodia - Nxp Semiconductors");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C-UART Bridge driver for SC16IS750");

