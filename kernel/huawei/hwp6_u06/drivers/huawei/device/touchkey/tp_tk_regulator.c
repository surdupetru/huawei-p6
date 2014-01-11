#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <hsad/config_interface.h>
#include <linux/mux.h>
#include <mach/gpio.h>
#include"tp_tk_regulator.h"

#define SYNAPTICS_VDD "ts-vdd"
#define SYNAPTICS_VBUS "ts-vbus"

#define TOUCHKEY_GPIO_BOLCK_NAME "block_touchkey"
#define TOUCHSCREEN_GPIO_BOLCK_NAME "block_touchscreen"

#define TOUCH_RESET_PIN GPIO_19_4

struct regulator *g_ts_vci = NULL;
struct regulator *g_ts_vddio = NULL;
struct regulator *g_tk_vdd = NULL;

struct iomux_block *g_TK_gpio_block=NULL;
struct iomux_block *g_TP_gpio_block = NULL;
struct block_config *g_TK_gpio_block_config = NULL;
struct block_config *g_TP_gpio_block_config = NULL;



//-----------------------------------------------------functions
int TP_VCI_GET(struct platform_device *pdev)
{   
    printk("%s entor\n",__func__);

    BUG_ON(pdev==NULL);

    g_ts_vci = regulator_get(&pdev->dev, SYNAPTICS_VDD);
    if (IS_ERR(g_ts_vci)) {
        printk( "%s: failed to get synaptics vdd\n", __func__);
        return  -EINVAL;
    }

    return 0;

}
int TP_VCI_PUT()
{  
    printk("%s entor\n",__func__);
    if (IS_ERR(g_ts_vci)) {
        printk( "%s: failed to get synaptics vdd\n", __func__);
        return  -EINVAL;
    }
    regulator_put(g_ts_vci);

    return 0;

}
int TP_VCI_ENABLE()
{
    int error;
    printk("%s entor\n",__func__);
    if (IS_ERR(g_ts_vci)) {
        printk( "%s: failed to get synaptics vdd\n", __func__);
        return  -EINVAL;
    }
    error = regulator_enable(g_ts_vci);
    if (error < 0) {
        printk( "%s: failed to enable synaptics vdd\n", __func__);
        return -EINVAL;
    }

    return 0;
}
int TP_VCI_DISABLE()
{
    int error;
    printk("%s entor\n",__func__);
    if (IS_ERR(g_ts_vci)) {
        printk( "%s: failed to get synaptics vdd\n", __func__);
        return  -EINVAL;
    }
    error = regulator_disable(g_ts_vci);
    if (error < 0) {
        printk( "%s: failed to disable synaptics vdd\n", __func__);
        return -EINVAL;
    }

    return 0;
}

/*-----------------------------------------------------TP_VDDIO---------------------  ------*/
int TP_VDDIO_GET(struct platform_device *pdev)
{
    printk("%s entor\n",__func__);

    BUG_ON(pdev==NULL);

    g_ts_vddio = regulator_get(&pdev->dev,SYNAPTICS_VBUS);
    if (IS_ERR(g_ts_vddio)) {
        printk( "%s: failed to get synaptics vbus\n", __func__);
        return -EINVAL;
    }
    return 0;
}
int TP_VDDIO_PUT()
{
    printk("%s entor\n",__func__);
    if (IS_ERR(g_ts_vddio)) {
        printk( "%s: failed to get synaptics vbus\n", __func__);
        return -EINVAL;
    }
    regulator_put(g_ts_vddio);
    return 0;
}
int TP_VDDIO_ENABLE()
{
    int error;
    printk("%s entor\n",__func__);
    if (IS_ERR(g_ts_vddio)) {
    	printk( "%s: failed to get synaptics vbus\n", __func__);
    	return -EINVAL;
    }
    error = regulator_set_voltage(g_ts_vddio,1800000,1800000);
    if(error < 0){
    	printk( "%s: failed to set synaptics vbus\n", __func__);
    	return -EINVAL;
    }
    error = regulator_enable(g_ts_vddio);
    if (error < 0) {
    	printk( "%s: failed to enable synaptics vbus\n", __func__);
    	return -EINVAL;
    }
    return 0;
}
int TP_VDDIO_DISABLE()
{
    int error;
    printk("%s entor\n",__func__);
    if (IS_ERR(g_ts_vddio)) {
    	printk( "%s: failed to get synaptics vbus\n", __func__);
    	return -EINVAL;
    }

    error = regulator_disable(g_ts_vddio);
    if (error < 0) {
    	printk( "%s: failed to enable synaptics vbus\n", __func__);
    	return -EINVAL;
    }
    return 0;
}

/*-----------------------------------------------------TK_VCI---------------------  ------*/
int TK_VCI_GET(struct platform_device *pdev)
{
    int tk_regulator_vout;
    
    printk("%s entor\n",__func__);

    BUG_ON(pdev==NULL);

    tk_regulator_vout = get_touchkey_regulator_vout();

    if(E_TOUCHKEY_REGULATOR_VOUT17==tk_regulator_vout )
    {
        printk("%s so340010_V17\n",__func__);
        g_tk_vdd = regulator_get(&pdev->dev,"so340010_V17");
        if (IS_ERR(g_tk_vdd)) {
           printk( "%s: failed to get synaptics_tk vdd\n", __func__);
           return -EINVAL;
        }
    }
    else if(E_TOUCHKEY_REGULATOR_VOUT13==tk_regulator_vout)
    {
        printk("%s so340010_V13\n",__func__);
        g_tk_vdd = regulator_get(&pdev->dev,"so340010_V13");
        if (IS_ERR(g_tk_vdd)) {
           printk( "%s: failed to get synaptics_tk vdd\n", __func__);
           return -EINVAL;
        }
    }
    else
    {
        printk( "%s:get_touchkey_regulator_vout failed tk_regulator_vout=%d\n", __func__,tk_regulator_vout);
        return -EINVAL;
    }

    return 0;
}
int TK_VCI_PUT()
{
    printk("%s entor\n",__func__);
    if (IS_ERR(g_tk_vdd)) {
        printk( "%s: failed to get synaptics vbus\n", __func__);
        return -EINVAL;
    }
    regulator_put(g_tk_vdd);

    return 0;
}
int TK_VCI_ENABLE()
{
    int error;
    printk("%s entor\n",__func__);
    if (IS_ERR(g_tk_vdd)) {
       printk("%s: failed to get synaptics_tk vdd\n", __func__);
       return -EINVAL;
    }    
    
    error = regulator_set_voltage(g_tk_vdd,2850000,2850000);
    if(error < 0){
        printk("%s: failed to set synaptics_tk vdd\n", __func__);
        return -EINVAL;
    }
    error = regulator_enable(g_tk_vdd);
    if (error < 0) {
        printk("%s: failed to enable synaptics_tk vdd\n", __func__);
        return -EINVAL;
    }
    
    return 0;
}
int TK_VCI_DISABLE()
{
    int error;
    printk("%s entor\n",__func__);
    if (IS_ERR(g_tk_vdd)) {
       printk("%s: failed to get synaptics_tk vdd\n", __func__);
       return -EINVAL;
    }    
    
    error = regulator_disable(g_tk_vdd);
    if (error < 0) {
        printk( "%s: failed to disable synaptics_tk vdd\n", __func__);
        return -EINVAL;
    }

    return 0;
}

/*-----------------------------------------------------------------------------*/
static int set_iomux_init(char *block_name,struct iomux_block **gpio_block,struct block_config **gpio_block_config)
{
    int ret = 0;
    
    BUG_ON(block_name==NULL);
    BUG_ON(gpio_block==NULL);
    BUG_ON(gpio_block_config==NULL);

    /* get gpio block*/
    *gpio_block = iomux_get_block(block_name);
    if (IS_ERR(*gpio_block)) {
        printk("%s: failed to get gpio block,iomux_get_block failed\n", __func__);
        ret = -EINVAL;
        return ret;
    }

    /* get gpio block config*/
    *gpio_block_config = iomux_get_blockconfig(block_name);
    if (IS_ERR(*gpio_block_config)) {
        printk("%s: failed to get gpio block config\n", __func__);
        ret = -EINVAL;
        *gpio_block = NULL;
        return ret;
    }

    return 0;

}
static int set_iomux_normal(struct iomux_block *gpio_block,struct block_config *gpio_block_config)
{
    int ret;
    
    BUG_ON(gpio_block_config==NULL);
    BUG_ON(gpio_block==NULL);
    
    ret = blockmux_set(gpio_block, gpio_block_config, NORMAL);
    if (ret<0) {
        printk(KERN_ERR "%s: failed to config gpio\n", __func__);
        return ret;
    }

    return 0;
}
static int set_iomux_lowpower(struct iomux_block *gpio_block,struct block_config *gpio_block_config)
{
    int ret;
    
    BUG_ON(gpio_block_config==NULL);
    BUG_ON(gpio_block==NULL);
    
    ret = blockmux_set(gpio_block, gpio_block_config, LOWPOWER);
    if (ret<0) {
        printk(KERN_ERR "%s: failed to config gpio\n", __func__);
        return ret;
    }

    return 0;
}

int TK_set_iomux_init(void)
{
    printk("%s entor\n",__func__);
    return set_iomux_init(TOUCHKEY_GPIO_BOLCK_NAME,&g_TK_gpio_block,&g_TK_gpio_block_config);
}
int TK_set_iomux_normal(void)
{
    printk("%s entor\n",__func__);
    return set_iomux_normal(g_TK_gpio_block,g_TK_gpio_block_config);
}
int TK_set_iomux_lowpower(void)
{
    printk("%s entor\n",__func__);
    return set_iomux_lowpower(g_TK_gpio_block,g_TK_gpio_block_config);
}

int TP_set_iomux_init(void)
{
    printk("%s entor\n",__func__);
    return set_iomux_init(TOUCHSCREEN_GPIO_BOLCK_NAME,&g_TP_gpio_block,&g_TP_gpio_block_config);
}

int TP_set_iomux_normal(void)
{
    printk("%s entor\n",__func__);
    return set_iomux_normal(g_TP_gpio_block,g_TP_gpio_block_config);
}
int TP_set_iomux_lowpower(void)
{
    printk("%s entor\n",__func__);
    return set_iomux_lowpower(g_TP_gpio_block,g_TP_gpio_block_config);
}


int TP_set_gpio_config_normal(void)
{
    int retval;
    printk("%s entor\n",__func__);
    retval = gpio_request(TOUCH_RESET_PIN, "rmi4_reset");
    if (retval) {
        pr_err("%s: Failed to get reset gpio %d. Code: %d.",
               __func__, TOUCH_RESET_PIN, retval);
        return retval;
    }
    retval = gpio_direction_output(TOUCH_RESET_PIN,1);
    if (retval) {
        pr_err("%s: Failed to setup reset gpio %d.1 Code: %d.",
               __func__, TOUCH_RESET_PIN, retval);
    }
    mdelay(10);
    retval = gpio_direction_output(TOUCH_RESET_PIN,0);
    if (retval) {
        pr_err("%s: Failed to setup reset gpio %d.1 Code: %d.",
               __func__, TOUCH_RESET_PIN, retval);
    }
    mdelay(10);
    retval = gpio_direction_output(TOUCH_RESET_PIN,1);
    if (retval) {
        pr_err("%s: Failed to setup reset gpio %d.1 Code: %d.",
               __func__, TOUCH_RESET_PIN, retval);
    }
    gpio_free(TOUCH_RESET_PIN);
    return 0;
}
int TP_set_gpio_config_lowpower(void)
{
    int retval;
    printk("%s entor\n",__func__);

    retval = gpio_request(TOUCH_RESET_PIN, "rmi4_reset");
    if (retval) {
        pr_err("%s: Failed to get reset gpio %d. Code: %d.",
               __func__, TOUCH_RESET_PIN, retval);
        return retval;
    }    
    retval = gpio_direction_output(TOUCH_RESET_PIN,0);
    if (retval) 
    {
        pr_err("%s: Failed to setup reset gpio %d. Code: %d.",
                        __func__, TOUCH_RESET_PIN, retval);
    }

    gpio_free(TOUCH_RESET_PIN);
    return retval;
}

