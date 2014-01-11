/*
 * synaptics_SO340010.c - SYNAPTICS SO340010 ONETOUCH SENSOR CONTROLER
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <asm/gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mux.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <hsad/config_interface.h>
#include "synaptics_SO340010.h"

/*macro definition begin*/
/*
static int touchkey_debug_mask = 0;
module_param_named(debug_mask, touchkey_debug_mask, int,
		S_IRUGO | S_IWUSR | S_IWGRP);

#define S340010_DBG(x...) do {\
	if (touchkey_debug_mask) \
		printk(KERN_DEBUG x);\
	} while (0)

	*/
#define PRESS_KEY                                    1
#define RELEASE_KEY                                0
#define KEYNUMBER                                   3
#define DEFAULT10_SENSITIVITY                0x8FAF
#define DEFAULT32_SENSITIVITY                0x8F8F 
#define DEFAULT10_SENSITIVITY_DCM                0x6800
#define DEFAULT32_SENSITIVITY_DCM                0x606A
#define SYNAPTICS_I2C_RETRY_TIMES    10
#define TRUE                                              1
#define FALSE                                            0

/*macro definition end*/

extern atomic_t touch_is_pressed;    
extern u32 time_finger_up;    
bool tk_flag_status; 

static atomic_t tk_led_available = ATOMIC_INIT(0);
/*function definition begin*/
#if 1
static int synaptics_resume(struct i2c_client *client);
static int synaptics_suspend(struct i2c_client *client, pm_message_t mesg);
static void synaptics_early_suspend(struct early_suspend *h);
static void synaptics_later_resume(struct early_suspend *h);
#endif
/*function definition end*/

/*variable definiton begin*/
int g_led_brightness = -1;
/*variable definiton end*/

/*struct area begin*/
struct so340010_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct workqueue_struct *synaptics_wq;
	struct work_struct work;
	struct early_suspend early_suspend;
	uint16_t key_matrix;
	unsigned short *keycodes;
	struct iomux_block *gpio_block;
	struct block_config *gpio_block_config;
	int key_port;
};
struct so340010_data *touchkey_driver = NULL;
/*struct area end*/
//static bool flag_tk_status = false;

static int touchkey_gpio_block_init(struct device *dev, struct so340010_data *syp_tk)
{
	int ret = 0;

	/* get gpio block*/
	syp_tk->gpio_block = iomux_get_block(TOUCHKEY_GPIO_BOLCK_NAME);
	if (IS_ERR(syp_tk->gpio_block)) {
		dev_err(dev, "%s: failed to get gpio block,iomux_get_block failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* get gpio block config*/
	syp_tk->gpio_block_config = iomux_get_blockconfig(TOUCHKEY_GPIO_BOLCK_NAME);
	if (IS_ERR(syp_tk->gpio_block_config)) {
		dev_err(dev, "%s: failed to get gpio block config\n", __func__);
		ret = -EINVAL;
		goto err_block_config;
	}

	/* config gpio work mode*/
	ret = blockmux_set(syp_tk->gpio_block, syp_tk->gpio_block_config, NORMAL);
	if (ret) {
		dev_err(dev, "%s: failed to config gpio,blockmux_set failed\n", __func__);
		ret = -EINVAL;
		goto err_mux_set;
	}

	return ret;

err_mux_set:
	if (syp_tk->gpio_block_config)
		syp_tk->gpio_block_config = NULL;
err_block_config:
	if (syp_tk->gpio_block)
		syp_tk->gpio_block = NULL;

	return ret;

}

int i2c_synaptics_write(uint16_t address, uint16_t data)
{
    int retry = SYNAPTICS_I2C_RETRY_TIMES;
    int err = 0;
    uint8_t buf[4];
    struct i2c_msg msg[] = {
        {
            .addr = touchkey_driver->client->addr,
            .flags = 0,
            .len = 4,
            .buf = buf,
        }
    };
    buf[1] = address & 0xFF;
    buf[0] = (address >> 8) & 0xFF;
    buf[3] = data & 0xFF;
    buf[2] = (data >> 8) & 0xFF;

    while(retry--){
        err = i2c_transfer(touchkey_driver->client->adapter, msg, 1);
        if(err >= 0) {
			return 0;
    	}
	printk(KERN_DEBUG "[TouchKey][reg 0x%x][data 0x%x] %s %d i2c transfer error\n",
		address,data,__func__, __LINE__);
       mdelay(10);
    }
    return err;
}
EXPORT_SYMBOL(i2c_synaptics_write);

static int i2c_synaptics_read( uint16_t address, uint16_t *data)
{
    int retry = SYNAPTICS_I2C_RETRY_TIMES;
    int err;
    uint8_t addr[2];
    uint8_t read_data[2];
    struct i2c_msg msg[] = {
        {
            .addr = touchkey_driver->client->addr,
            .flags = 0,
            .len = 2,
            .buf = addr,
        },
        {
            .addr = touchkey_driver->client->addr,
            .flags = I2C_M_RD,
            .len = 2,
            .buf = read_data,
        }
    };
    addr[1] = address & 0xFF;
    addr[0] = (address >> 8) & 0xFF;

    while(retry--){
        err = i2c_transfer(touchkey_driver->client->adapter, msg, 2);
        if(err >= 0) {
			(*data) = read_data[0]&0xFF;
			(*data) = (((*data) << 8) & 0xFF00) | read_data[1];
			return 0;
	}
	printk(KERN_ERR "[TouchKey] %s %d i2c transfer error\n",
		__func__, __LINE__);
        mdelay(10);
    }
    return err;
}
/*enable power for SO340010*/
static int enable_power_for_device(struct synatics_touchkey_platform *pdata, struct i2c_client *client, bool enable)
{
	int  error = 0;

        
       if(enable)
	{ 
	     pdata->vbus = regulator_get(&client->dev, SYNAPTICS_SO340010_NAME);
             if (IS_ERR(pdata->vbus)) {
        	dev_err(&client->dev, "%s: failed to get synaptics vbus\n", __func__);
        	return -EINVAL;
            }
            error = regulator_set_voltage(pdata->vbus,2850000,2850000);
            if(error < 0){
            	dev_err(&client->dev, "%s: failed to set synaptics vbus\n", __func__);
            	return -EINVAL;
            }
            
            error = regulator_enable(pdata->vbus);
            if (error < 0) {
            	dev_err(&client->dev, "%s: failed to enable synaptics vbus\n", __func__);
            	return -EINVAL;
            }
            mdelay(100);
	} else {
	    if( NULL !=pdata->vbus) {
               error = regulator_set_voltage(pdata->vbus, 0, 2850000);
		 if(error < 0) {
            	     dev_err(&client->dev, "%s: failed to set synaptics vbus\n", __func__);
            	     return -EINVAL;
               }
               error = regulator_disable(pdata->vbus);
	        if(error < 0) {
            	     dev_err(&client->dev, "%s: failed to disable synaptics vbus\n", __func__);
            	     return -EINVAL;
               }
                regulator_put(pdata->vbus);
               pdata->vbus = NULL;
	    }
	}
	return error;
}

static void synaptics_work_func(struct work_struct *work)
{
    uint16_t reg_read = 0,status = 0;
	u32 t, deltT;
    int err;
    struct so340010_data *syp_tk =
        container_of(work, struct so340010_data, work);
    err = i2c_synaptics_read(GPIO_STATE, &reg_read);
    err = i2c_synaptics_read(BUTTON_STATE, &reg_read);
    if(err < 0) {
        printk("synaptics_work_func_read_err\n");
        goto out;
    }
    reg_read = reg_read & 0x000F;
    status = reg_read | syp_tk->key_matrix;
    printk("====[%s],[reg_read:%x],[status:%d],[key_matrix:%d]\n", __func__, reg_read, status, syp_tk->key_matrix);
	t = (u32)ktime_to_ms(ktime_get());    
	deltT = t - time_finger_up;      
	if( !( ((atomic_read(&touch_is_pressed) == 0) && (120 < deltT)) || tk_flag_status) )    
	{    
		printk("synaptics_work_func not input_sync\n");    
		goto out;    
	}    

	

    if(syp_tk->key_port==E_TOUCHKEY_KEY_PORT_U9700S0)/* S0, S1, S2*/
    {
        switch(status)
        {
            case 1:
                if(reg_read)
                input_report_key(syp_tk->input, syp_tk->keycodes[0], PRESS_KEY);
                else
                input_report_key(syp_tk->input, syp_tk->keycodes[0], RELEASE_KEY);
                break;
            case 2:
                if(reg_read)
                input_report_key(syp_tk->input, syp_tk->keycodes[1], PRESS_KEY);
                else
                input_report_key(syp_tk->input, syp_tk->keycodes[1], RELEASE_KEY);
                break;
            case 4:
                if(reg_read)
                input_report_key(syp_tk->input, syp_tk->keycodes[2], PRESS_KEY);
                else
                input_report_key(syp_tk->input, syp_tk->keycodes[2], RELEASE_KEY);
                break;
            case 8:
                default:
                printk(KERN_ERR "[E_TOUCHKEY_KEY_PORT_U9700S0] %s invalid key S0.\n",__func__);
                goto out;
        }
    }
    else if(syp_tk->key_port==E_TOUCHKEY_KEY_PORT_U9700S1)/* S1, S2, S3*/
    {
        switch(status)
        {
            case 1:
                printk(KERN_ERR "[E_TOUCHKEY_KEY_PORT_U9700S1] %s invalid key case 1.\n",__func__);
                break;
            case 2: //menu
                if(reg_read)
                input_report_key(syp_tk->input, syp_tk->keycodes[0], PRESS_KEY);
                else
                input_report_key(syp_tk->input, syp_tk->keycodes[0], RELEASE_KEY);
                break;
            case 4://home
                if(reg_read)
                input_report_key(syp_tk->input, syp_tk->keycodes[1], PRESS_KEY);
                else
                input_report_key(syp_tk->input, syp_tk->keycodes[1], RELEASE_KEY);
                break;
            case 8: //back
                if(reg_read)
                input_report_key(syp_tk->input, syp_tk->keycodes[2], PRESS_KEY);
                else
                input_report_key(syp_tk->input, syp_tk->keycodes[2], RELEASE_KEY);
                break;
            default:
                printk(KERN_ERR "[E_TOUCHKEY_KEY_PORT_U9700S1] %s invalid key S1.\n",__func__);
                goto out;
        }
    }
    else if(syp_tk->key_port==E_TOUCHKEY_KEY_PORT_U9701S1)
    {    
        switch(status)
        {
            case 1:
                printk(KERN_ERR "[E_TOUCHKEY_KEY_PORT_U9701S1] %s invalid key case 1.\n",__func__);
                break;
            case 2:
                if(reg_read)
                input_report_key(syp_tk->input, syp_tk->keycodes[1], PRESS_KEY);
                else
                input_report_key(syp_tk->input, syp_tk->keycodes[1], RELEASE_KEY);
                break;
            case 4:
                if(reg_read)
                input_report_key(syp_tk->input, syp_tk->keycodes[2], PRESS_KEY);
                else
                input_report_key(syp_tk->input, syp_tk->keycodes[2], RELEASE_KEY);
                break;
            case 8:
                if(reg_read)
                input_report_key(syp_tk->input, syp_tk->keycodes[0], PRESS_KEY);
                else
                input_report_key(syp_tk->input, syp_tk->keycodes[0], RELEASE_KEY);
                break;
            default:
                printk(KERN_ERR "[TouchKey] %s invalid key.\n",__func__);
                goto out;
        }

    }
    else
    {
        printk(KERN_ERR "[TouchKey] %s invalid key_port=%d.\n",__func__,syp_tk->key_port);
    }
    input_sync(syp_tk->input);
	if(reg_read)    
		tk_flag_status = true;    
	else    
		tk_flag_status = false;    

    syp_tk->key_matrix = reg_read;
    
out:
    enable_irq(syp_tk->client->irq);
}
static irqreturn_t synaptics_irq_handler(int irq, void *dev_id)
{
	struct so340010_data *spy_tk = dev_id;
    disable_irq_nosync(spy_tk->client->irq);
    queue_work(spy_tk->synaptics_wq, &spy_tk->work);
    return IRQ_HANDLED;
}

static int synaptics_set_initconfig(struct i2c_client *client, int key_port,int led_brightness)
{
    int err;
    uint16_t data;
    printk("synaptics_set_initconfig_____________enter\n");

    if(key_port==E_TOUCHKEY_KEY_PORT_U9700S0)
    {
        printk("synaptics_set_initconfig V3\n");
        err = i2c_synaptics_write(BUTTON_ENABLE , 0x0007);
        if(err < 0) {
            printk(KERN_ERR "[TouchKey] %s init BUTTON_ENABLE failed\n",__func__);
            return err;
        }
    }
    else if( (key_port==E_TOUCHKEY_KEY_PORT_U9700S1)
                ||(key_port==E_TOUCHKEY_KEY_PORT_U9701S1) )
    {
        printk("synaptics_set_initconfig V4\n");
        err = i2c_synaptics_write(BUTTON_ENABLE , 0x000E);
        if(err < 0) {
            printk(KERN_ERR "[TouchKey] %s init BUTTON_ENABLE failed\n",__func__);
            return err;
        }
    }
    else
    {
            printk(KERN_ERR "[TouchKey] %s failed,key_port=%d\n",__func__,key_port);
            return -1;
    }
	
    err = i2c_synaptics_write(GPIO_CONTROL , 0x0E0E);
    if(err < 0) {
        printk(KERN_ERR "[TouchKey] %s init GPIO_CONTROL failed\n",__func__);
        return err;
    }
    
    err = i2c_synaptics_write(INTERFERENCE_THRESHOLD , 0x0080);
    if(err < 0) {
        printk(KERN_ERR "[TouchKey] %s init SENSOR_PIN10_SENSITIVITY failed\n",__func__);
        return err;
    }	

    if( get_touchkey_sensitivity()==1 ) /*U9701L*/
    {
        err = i2c_synaptics_write(SENSOR_PIN10_SENSITIVITY , DEFAULT10_SENSITIVITY_DCM);
        if(err < 0) {
            printk(KERN_ERR "[TouchKey] %s init SENSOR_PIN10_SENSITIVITY_DCM failed\n",__func__);
            return err;
        }

        err = i2c_synaptics_write(SENSOR_PIN32_SENSITIVITY , DEFAULT32_SENSITIVITY_DCM);
        if(err < 0) {
            printk(KERN_ERR "[TouchKey] %s init SENSOR_PIN32_SENSITIVITY_DCM failed\n",__func__);
            return err;
        }
    }
    else if( get_touchkey_sensitivity()==0 )/*U9700L*/
    {
        err = i2c_synaptics_write(SENSOR_PIN10_SENSITIVITY , DEFAULT10_SENSITIVITY);
        if(err < 0) {
            printk(KERN_ERR "[TouchKey] %s init SENSOR_PIN10_SENSITIVITY failed\n",__func__);
            return err;
        }

        err = i2c_synaptics_write(SENSOR_PIN32_SENSITIVITY , DEFAULT32_SENSITIVITY);
        if(err < 0) {
            printk(KERN_ERR "[TouchKey] %s init SENSOR_PIN32_SENSITIVITY failed\n",__func__);
            return err;
        }
    }
    else
    {
        printk("[TouchKey] init SENSOR_PIN10_SENSITIVITY and SENSOR_PIN32_SENSITIVITY failed\n");
        return -1;
    }
	
    err = i2c_synaptics_write(BUTTON_TO_GPIO_MAPPING , 0x2000);
    if(err < 0) {
        printk(KERN_ERR "[TouchKey] %s init BUTTON_TO_GPIO_MAPPING failed\n",__func__);
        return err;
    }

    err = i2c_synaptics_write(TIMER_CONTROL , 0xD000);
    if(err < 0) {
        printk(KERN_ERR "[TouchKey] %s init TIMER_CONTROL failed\n",__func__);
        return err;
    }

    if(E_TOUCHKEY_KEY_BRIGHTNESS_11==led_brightness)
    {
        err = i2c_synaptics_write(LED_CONTROL1 , 0x8B8B);
        if(err < 0) {
            printk(KERN_ERR "[TouchKey] %s init LED_CONTROL1 failed\n",__func__);
            return err;
        }

        err = i2c_synaptics_write(LED_CONTROL2 , 0x8B8B);
        if(err < 0) {
            printk(KERN_ERR "[TouchKey] %s init LED_CONTROL2 failed\n",__func__);
            return err;
        }
    }
    else if(E_TOUCHKEY_KEY_BRIGHTNESS_26==led_brightness)
    {
        err = i2c_synaptics_write(LED_CONTROL1 , 0x9A9A);
        if(err < 0) {
            printk(KERN_ERR "[TouchKey] %s init LED_CONTROL1 failed\n",__func__);
            return err;
        }

        err = i2c_synaptics_write(LED_CONTROL2 , 0x9A9A);
        if(err < 0) {
            printk(KERN_ERR "[TouchKey] %s init LED_CONTROL2 failed\n",__func__);
            return err;
        }
    }
    else
    {
        printk(KERN_ERR "[TouchKey] %s failed, led_brightness = %d\n",__func__,led_brightness);
        return -1; 
    }
#if 0
    err = i2c_synaptics_write(LED_ENABLE, 0x000E);
    if(err < 0) {
        printk(KERN_ERR "[TouchKey] %s init LED_CONTROL2 failed\n",__func__);
        return err;
    }
#endif
	 err = i2c_synaptics_write(INTERFACE_CONFIGUARATION , 0x0007);
    if(err < 0) {
        printk(KERN_ERR "[TouchKey] %s init INTERFACE_CONFIGUARATION failed\n",__func__);
        return err;
    }
	
    err = i2c_synaptics_write(GENERAL_CONFIGURATION , 0x0060); //0060
    if(err < 0) {
        printk(KERN_ERR "[TouchKey] %s init GENERAL_CONFIGURATION failed\n",__func__);
        return err;
    }	
    err = i2c_synaptics_read(GPIO_STATE, &data);
	printk("i2c_synaptics_read     data      %d\n",data);
	printk("synaptics_set_initconfig_____________exit\n");

    return err;
}

/*add for check the details of registers begin */
static ssize_t synaptics_reg_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct so340010_data *synaptics = dev_get_drvdata(dev);
	u16 i,ret;
	int val;
	ssize_t count=0;
	uint16_t data = 0;
	uint16_t array_address[18] = {INTERFACE_CONFIGUARATION, GENERAL_CONFIGURATION, BUTTON_ENABLE,
		GPIO_CONTROL, INTERFERENCE_THRESHOLD, SENSOR_PIN10_SENSITIVITY, SENSOR_PIN32_SENSITIVITY,
		BUTTON_TO_GPIO_MAPPING, TIMER_CONTROL, LED_ENABLE, LED_EFFECT_PERIOD, LED_CONTROL1,
		LED_CONTROL2, GPIO_STATE, BUTTON_STATE, TIMER_STATE, PRESSURE_VALUES10, PRESSURE_VALUES32};
	disable_irq(synaptics->client->irq);
	for(i=0; i < 18; i++)
	{
		val = i2c_synaptics_read(array_address[i], &data);
		 ret = sprintf(buf, "register_address:[0x%04x]; val:[0x%04x] \n", array_address[i],data);
		 buf += ret;
		 count += ret;
	}
	enable_irq(synaptics->client->irq);
	return count;
}

static ssize_t synaptics_reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct so340010_data *synaptics = dev_get_drvdata(dev);
	int regs,value;
	int ret;
	if (2 != sscanf(buf, "0x%4x 0x%4x", &regs, &value)) {
		dev_err(&synaptics->client->dev, "failed to reg store\n");
		return -EINVAL;
	}
	disable_irq(synaptics->client->irq);
	ret = i2c_synaptics_write(regs, value);
	enable_irq(synaptics->client->irq);
	return count;
}

static DEVICE_ATTR(reg, 0664, synaptics_reg_show,
		   synaptics_reg_store);

static struct attribute *synaptics_attribute[] = {
    &dev_attr_reg.attr,
    NULL
};
static const struct attribute_group synaptics_attr_group = {
    .attrs = synaptics_attribute,
};
/*add for check the details of registers end */


static void synaptics_keypad_bl_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
    int ret;
    #ifdef CONFIG_P2_TP_TK_CMD_FEATURE
    uint16_t reg_brightness,reg_val;

    if((atomic_read(&tk_led_available) == 0))
    {
        dev_err(&touchkey_driver->client->dev, "%s failed ,tk_led_available==0\n",__func__);  
        return;
    }

    if(g_led_brightness<0)
    {
        dev_err(&touchkey_driver->client->dev, "%s failed ,g_led_brightness=%d\n",__func__,g_led_brightness);
        return;
    }

    reg_brightness = (value*g_led_brightness)/0xFF;

    if(reg_brightness==0)
    {
        ret = i2c_synaptics_write(LED_ENABLE, 0x0000);
        if (ret<0)
        {
            dev_err(&touchkey_driver->client->dev, "failed to disable led1 and led2\n");
        }
        return;
    }
    if(reg_brightness>g_led_brightness)
    {
        reg_brightness = g_led_brightness;
    }

    reg_brightness |= 0x80;
    reg_val = (reg_brightness<<8)+reg_brightness;
    printk("synaptics_keypad_bl_led_set: reg_val = 0x%x\n",reg_val);

    ret = i2c_synaptics_write(LED_CONTROL1 , reg_val);
    if(ret<0) {
        dev_err(&touchkey_driver->client->dev, "[TouchKey] synaptics_keypad_bl_led_set LED_CONTROL1 failed\n");
        return ;
    }
    ret = i2c_synaptics_write(LED_CONTROL2 , reg_val);
    if(ret<0) {
        dev_err(&touchkey_driver->client->dev, "[TouchKey] synaptics_keypad_bl_led_set LED_CONTROL2 failed\n");
        return ;
    }

    ret = i2c_synaptics_write(LED_ENABLE, 0x000E);
    if(ret<0) {
        dev_err(&touchkey_driver->client->dev, "[TouchKey] synaptics_keypad_bl_led_set LED_ENABLE failed\n");
        return ;
    }
    #else
    if (value)
    {
        ret = i2c_synaptics_write(LED_ENABLE, 0x0006);       // enable led1 and led2
        if (ret<0)
        {
            dev_err(&touchkey_driver->client->dev, "failed to enable led1 and led2\n");
        }
    }
    else
    {
        ret = i2c_synaptics_write(LED_ENABLE, 0x0000);
        if (ret<0)
        {
            dev_err(&touchkey_driver->client->dev, "failed to disable led1 and led2\n");
        }
    }
    #endif
}

static struct led_classdev synaptics_kp_bl_led = {
	.name                   = "synaptics-backlight-tk",
	.brightness_set		= synaptics_keypad_bl_led_set,
	.brightness		= LED_OFF,
};
static int __devinit synaptics_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
    struct so340010_data *syp_tk;
    struct synatics_touchkey_platform *pdata;
    int err,i;

    if (get_so340010_enable() == false) {
        dev_info(&client->dev, "so340010 disable, exit!\n");
	 err = -ENODEV;
        goto err_check_functionality_failed;
    }

    pdata = client->dev.platform_data;
	printk("synaptics_probe_in\n");
    client->dev.init_name = S340010_IO;
	#if 0
    err = enable_power_for_device(pdata, client,TRUE);
    if(err < 0) {
        dev_err(&client->dev, "%s adapter not supported\n",
        dev_driver_string(&client->adapter->dev));
        err = -ENODEV;
        goto err_check_functionality_failed;
	}
	#endif
	 tk_flag_status = false;
/* Check functionality */
    msleep(10);
    err = i2c_check_functionality(client->adapter,
        I2C_FUNC_I2C);
    if (!err) {
        dev_err(&client->dev, "%s adapter not supported\n",
        dev_driver_string(&client->adapter->dev));
        err = -ENODEV;
        goto err_check_functionality_failed;
	}
/* Chip is valid and active. Allocate structure */
    syp_tk = kzalloc(sizeof(struct so340010_data), GFP_KERNEL);
    if (syp_tk == NULL)
    {
        printk(KERN_ERR"%s: allocate synaptics_data failed\n", __func__);
        err = -ENOMEM;
        goto err_alloc_data_failed;
    }
    touchkey_driver = syp_tk;
/*queue work for synaptics So340010 chip*/
    syp_tk->synaptics_wq = create_singlethread_workqueue("synaptics_tk_wq");
    if (!syp_tk->synaptics_wq) 
    {
        printk(KERN_ERR"%s: create workqueue failed\n", __func__);
        err = -ENOMEM;
        goto err_cread_wq_failed;
    }
    INIT_WORK(&syp_tk->work, synaptics_work_func);
    syp_tk->client = client;
    i2c_set_clientdata(client, syp_tk);
    syp_tk->key_matrix =  0 ;
    syp_tk->keycodes = pdata->get_key_map();

    err = touchkey_gpio_block_init(&client->dev, syp_tk);
    if(err<0)
    {
        dev_err(&client->dev, "%s: failed to config gpio mode\n", __func__);
        goto err_request_gpio_failed;
    }

    if(pdata->touchkey_gpio_config) {
	 pdata->touchkey_gpio_config(TK_GPIO_PROBE);
        client->irq = gpio_to_irq(pdata->attn_gpio);
        if (client->irq < 0) {
            dev_err(&client->dev, "irq gpio reguest failed\n");
            err = -ENODEV;
            goto err_request_gpio_failed;
        }
    } else {
        dev_err(&client->dev, "no irq config func\n");
        err = -ENODEV;
        goto err_request_gpio_failed;
    }
    syp_tk->key_port = get_touchkey_key_port();
    if(syp_tk->key_port<0)
    {
        dev_err(&client->dev, "%s : get_touchkey_key_port failed\n",__func__);
        err = -EIO;
        goto err_init_config_failed;
    }
    g_led_brightness = get_touchkey_led_brightness();
    if(g_led_brightness<0)
    {
        dev_err(&client->dev, "%s : get_touchkey_led_brightness failed\n",__func__);
        err = -EIO;
        goto err_init_config_failed;
    }
    err = synaptics_set_initconfig(client,syp_tk->key_port,g_led_brightness);
    if (err < 0)
    {
        printk(KERN_ERR "%s: Failed to set register value!\n",__func__);
        err = -EIO;
        goto err_init_config_failed;
    }
    syp_tk->input = input_allocate_device();
    if (!syp_tk->input)
    {
        dev_err(&client->dev, "insufficient memory\n");
        err = -ENOMEM;
        goto err_input_dev_alloc_failed;
    }
    dev_set_drvdata(&syp_tk->input->dev, syp_tk);
    syp_tk->input->name = SYNAPTICS_SO340010_NAME;
    syp_tk->input->id.bustype = BUS_I2C;
    syp_tk->input->keycode = syp_tk->keycodes;
    syp_tk->input->keycodesize = sizeof(syp_tk->keycodes[0]);
    syp_tk->input->keycodemax = KEYNUMBER;
    __set_bit(EV_KEY, syp_tk->input->evbit);
    __clear_bit(EV_REP, syp_tk->input->evbit);
    for (i = 0; i < KEYNUMBER; i++)
    {
        input_set_capability(syp_tk->input, EV_KEY,
        syp_tk->keycodes[i]);
    }
    __clear_bit(KEY_RESERVED, syp_tk->input->keybit);
    err = input_register_device(syp_tk->input);
    if (err)
    {
    dev_err(&client->dev,"Failed to register input device\n");
    err = -ENOMEM;
    goto err_input_register_device_failed;
    }
    err = request_irq(client->irq, synaptics_irq_handler,
    IRQF_TRIGGER_LOW, SYNAPTICS_SO340010_NAME, syp_tk);// IRQF_TRIGGER_LOW // | IRQF_TRIGGER_RISING
    if (err) 
    {
        dev_err(&client->dev,"failed to allocate irq %d\n", client->irq);
        err = EIO;
        goto err_request_irq_failed;
    }
//ps:need add leds ports

/*add for led control begin*/
     err = led_classdev_register(&client->dev, &synaptics_kp_bl_led);  
     if (err)
     {
         dev_err(&client->dev, "unable to register led class driver\n");
	 err = -ENOMEM;
	 goto err_intput_reg;
     }
/*add for led control end*/

/*add for check the details of registers begin */
     err = sysfs_create_group(&client->dev.kobj, &synaptics_attr_group);
/*add for check the details of registers end */
#ifdef CONFIG_HAS_EARLYSUSPEND
    syp_tk->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING + 1;
    syp_tk->early_suspend.suspend = synaptics_early_suspend;
    syp_tk->early_suspend.resume = synaptics_later_resume;
    register_early_suspend(&syp_tk->early_suspend);
#endif

    atomic_set(&tk_led_available, 1);
    printk(KERN_DEBUG "synaptics-tk is successfully probed");
return 0;

err_intput_reg:
err_request_irq_failed:
	input_unregister_device(syp_tk->input);
err_input_register_device_failed:
	input_free_device(syp_tk->input);
err_init_config_failed:
err_input_dev_alloc_failed:
    if(pdata->touchkey_gpio_config)
    {
        pdata->touchkey_gpio_config(TK_GPIO_SUSPEND);
    }
err_request_gpio_failed:
    destroy_workqueue(syp_tk->synaptics_wq);
err_cread_wq_failed:
    touchkey_driver = NULL;
    kfree(syp_tk);
err_alloc_data_failed:
err_check_functionality_failed:
    //enable_power_for_device(pdata, client,FALSE);
    return err;
}

static int __devexit synaptics_remove(struct i2c_client *client)
{
    struct so340010_data *syp_tk;
    struct synatics_touchkey_platform *pdata;
    pdata = client->dev.platform_data;
    syp_tk = i2c_get_clientdata(client);
    cancel_work_sync(&syp_tk->work);
    destroy_workqueue(syp_tk->synaptics_wq);
    unregister_early_suspend(&syp_tk->early_suspend);
    /* Release IRQ so no queue will be scheduled */
    if (client->irq)
    	free_irq(client->irq, syp_tk);
    input_unregister_device(syp_tk->input);
    //enable_power_for_device(pdata, client,FALSE);
    touchkey_driver = NULL;
    kfree(syp_tk);
    return 0;
}

static int synaptics_suspend(struct i2c_client *client,pm_message_t mesg)
{
    int ret;
    struct synatics_touchkey_platform *pdata = client->dev.platform_data;
    struct so340010_data *syp_tk = i2c_get_clientdata(client);

    atomic_set(&tk_led_available, 0);
/* if use interrupt disable the irq ,else disable timer */ 
	tk_flag_status = false;
    disable_irq_nosync(client->irq);
    ret = cancel_work_sync(&syp_tk->work);    
    if (ret) /* if work was pending disable-count is now 2 */
    {   
       enable_irq(client->irq);
       printk(KERN_ERR "synaptics_suspend: can't cancel the work ,so enable the irq \n");
    }
#if 0
//we shoulden't control the led in driver
    ret = i2c_synaptics_write(LED_ENABLE, 0x0000);
    if(ret < 0)
    {
    	printk(KERN_ERR "synaptics_suspend: the touch can't close led \n");
    }
#endif
    ret = i2c_synaptics_write(GENERAL_CONFIGURATION, 0x0080); //use control base to set tp sleep
    if(ret < 0)
    {
    	printk(KERN_ERR "synaptics_suspend: the touch can't get into sleep \n");
    }

    if(pdata->touchkey_gpio_config)
    {
        pdata->touchkey_gpio_config(TK_GPIO_SUSPEND);
    }

    ret = blockmux_set(syp_tk->gpio_block, syp_tk->gpio_block_config, LOWPOWER);
    if (ret < 0) {
        printk(KERN_ERR "[TouchKey] %s: failed to config gpio\n", __func__);
    }

    return 0;
}
#if 1
static int synaptics_resume(struct i2c_client *client)
{
    int ret;
    struct synatics_touchkey_platform *pdata = client->dev.platform_data;
    struct so340010_data *syp_tk = i2c_get_clientdata(client);

    if(pdata->touchkey_gpio_config)
    {
        pdata->touchkey_gpio_config(TK_GPIO_RESUME);
    }    
    ret = blockmux_set(syp_tk->gpio_block, syp_tk->gpio_block_config, NORMAL);
    if (ret<0) {
        printk(KERN_ERR "%s: failed to config gpio\n", __func__);
    }

    ret = synaptics_set_initconfig(client,syp_tk->key_port,g_led_brightness);
    if (ret < 0)
    {
        printk(KERN_ERR "%s: Failed to set register value!\n",__func__);
        ret = -EIO;
        return ret;
    }

    ret = i2c_synaptics_write(GENERAL_CONFIGURATION, 0x0060); //use control base to wake up tp
    if(ret < 0)
    {
      printk(KERN_ERR "synaptics_resume: the touch can't resume! \n");
    }
#if 0
//we shoulden't control the led in driver
    #ifdef CONFIG_P2_TP_TK_CMD_FEATURE
    ret = i2c_synaptics_write(LED_ENABLE, 0x000E);
    if(ret < 0)
    {
        printk(KERN_ERR "synaptics_resume: the touch can't open led \n");
    }
    #endif
#endif
    mdelay(10);
    enable_irq(client->irq);
    atomic_set(&tk_led_available, 1);
    printk(KERN_ERR "synaptics_touchkey is resume!\n");
    return 0;
}
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_early_suspend(struct early_suspend *h)
{
    struct so340010_data *syp_tk;
    syp_tk = container_of(h, struct so340010_data, early_suspend);
    synaptics_suspend(syp_tk->client,PMSG_SUSPEND);
}
static void synaptics_later_resume(struct early_suspend *h)
{
    struct so340010_data *syp_tk;
    syp_tk = container_of(h, struct so340010_data, early_suspend);
    synaptics_resume(syp_tk->client);
}
#endif

#if 0
//just for case that the phone didn't close the LED:insert charger then power off the phone
static void synaptics_shutdown(struct i2c_client *client)
{
    struct so340010_data *syp_tk = i2c_get_clientdata(client);
    struct synatics_touchkey_platform *pdata = client->dev.platform_data;
    int ret;
    ret = i2c_synaptics_write(LED_ENABLE, 0x0000);//led off
    if(ret)
        printk(KERN_DEBUG "synaptics-tk is  shutdown");
    if(pdata->touchkey_gpio_config)
    {
        pdata->touchkey_gpio_config(TK_GPIO_SUSPEND);
    }
} 
#endif

static const struct i2c_device_id synaptics_idtable[] = {
	{ SYNAPTICS_SO340010_NAME, 0, },
	{ }
};

MODULE_DEVICE_TABLE(i2c, synaptics_idtable);

static struct i2c_driver synaptics_driver = {
	.driver = {
		.name	= SYNAPTICS_SO340010_NAME,
		.owner  = THIS_MODULE,
	},
	.id_table	= synaptics_idtable,
	.probe		= synaptics_probe,
	.remove		= synaptics_remove,
//	.shutdown      = synaptics_shutdown,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = synaptics_suspend,
	.resume =  synaptics_resume,
#endif
};

static int __init synaptics_init(void)
{
	printk(KERN_INFO "Synaptics SO003410 init:\n");
	return i2c_add_driver(&synaptics_driver);
}
module_init(synaptics_init);

static void __exit synaptics_cleanup(void)
{
	i2c_del_driver(&synaptics_driver);
}
module_exit(synaptics_cleanup);


MODULE_AUTHOR("Willow.Wang willow.wanglei@huawei.com PengChao pengchao.peng@huawei.com");
MODULE_DESCRIPTION("Driver for SO340010 Touch Sensor");
MODULE_LICENSE("GPL");

