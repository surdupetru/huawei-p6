/*
 * TI LM3642 driver 
 *
 *  Author: 	Zhoujie (zhou.jie1981@163.com)
 *  Date:  	2013/01/05
 *  Version:	1.0
 *  History:	2013/01/05      Frist add driver for LM3642 
 *  
 * ----------------------------------------------------------------------------
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 *
 */

#include <linux/mux.h>
#include <linux/kernel.h>
#include <mach/gpio.h>
#include <linux/i2c.h>
#include "../isp/k3_ispv1_io.h"
#include "../isp/sensor_common.h"
//#include <mach/lm3642.h>
#include <linux/delay.h>
#include <linux/time.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

#define LOG_TAG "K3_STROBE"

//#define DEBUG_DEBUG 1
#include "../isp/cam_log.h"


#define REG_ENABLE 				0x0a
#define REG_FLAGS   				0x0b
#define REG_FLASH_FEATURES  	0x08
#define REG_CURRENT_CONTROL 	0x09
#define REG_IVFM 				0x01
#define REG_TORCH_RAMP_TIME   0x06

/* reg_enable
     x x x x   x x x x
                      | |__mode bit 0
			  |__ _mode bit 1	
*/
#define MODE_STANDBY			0x00
#define MODE_INDICATOR			0x01
#define MODE_TORCH				0x02
#define MODE_FLASH				0x03
#define STROBE_PIN				0x20
#define TORCH_PIN				0x10
#define TX_PIN					0x40

#define FLASH_CURRENT_93MA75	0x00
#define FLASH_CURRENT_MIN    	FLASH_CURRENT_93MA75


#define STATE_BRIGHT_OFF       0x0
#define STATE_BRIGHT_LOW       0x09
#define STATE_BRIGHT_MEDIUM    0x12
#define STATE_BRIGHT_HIGH      0x1b
#define STATE_BRIGHT_LEFT_HIGH      0x18
#define STATE_BRIGHT_RIGHT_HIGH      0x03

#define TORCH_CURRENT_FOR_PREFLASH   3    //187.5ma
#define FLASH_CURRENT_FOR_FLASH          9    //937.5ma
#define THERMAL_PROTECT (1)

static camera_flashlight lm3642_intf;
struct i2c_client *lm3642_client;

static int brightness_level = 0;

#define I2C_BUS_NUM 0
#define PIN_STROBE0   65
#define  DEVICE_LM3642_NAME   "LM3642"
#define  DEVICE_LM3642_ADDR   0x63
static struct i2c_board_info lm3642_info = {
         .type = DEVICE_LM3642_NAME,
         .addr = DEVICE_LM3642_ADDR,
};

static ssize_t lm3642_led_torch_mode_get_brightness(struct device *dev, struct device_attribute *attr,char *buf);
static ssize_t lm3642_led_torch_mode_set_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

#ifdef THERMAL_PROTECT 
static DEFINE_SPINLOCK(led_stat_lock);
static bool thermal_protect_led_stat = true;
//static int led_configed_flag = 0;
static void thermal_protect_set_flash_stat(bool thermal_led_stat);
static bool thermal_protect_get_flash_stat(void);
static ssize_t thermal_protect_flash_led_state_get(struct device *dev, struct device_attribute *attr,char *buf);
static ssize_t thermal_protect_flash_led_state_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
#endif
int lm3642_reset(void);
static void lm3642_shutdown(struct i2c_client *client);
/*
 * record led select status
 * after reset, default LED2 enabled
 * led_select: x x x x x 1 1 1
 *                       | | |_ LED1 enable
 *                       | |___ LED2 enable 
 *                       |_____ LED3 enable
 */
static u8 led_select = 0x02;
static int reset_pin;
static int strobe0_pin;
static int strobe1_pin;

#define K3_FLASH_BLOCK	"block_isp_flash"
static struct iomux_block *gpio_block;
static struct block_config *block_conf;


static int lm3642_read_reg8(u8 reg, u8 *val);
static int lm3642_write_reg8(u8 reg, u8 val);
static int lm3642_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int lm3642_remove(struct i2c_client *client);
static int test_flash(void);
static void set_strobe0(u32 value);
static void dump_reg(void);
static int lm3642_turn_on(work_mode mode, flash_lum_level lum);
static int _lm3642_turn_on(work_mode mode, flash_lum_level lum);
static int lm3642_turn_off(void);
static int lm3642_camera_mode_flag = 0;
static int lm3642_init_flag = 0;
static int led_configed_flag = 0;
static char led_status = '0';

int lm3642_init(void);
void lm3642_exit(void);

static const struct i2c_device_id lm3642_id[] = {
	{DEVICE_LM3642_NAME, 0},
	{}
};

static struct i2c_driver lm3642_driver = {
	.driver = {
		   .name = DEVICE_LM3642_NAME,
		   },
	.probe = lm3642_probe,
	.remove = lm3642_remove,
	.shutdown = lm3642_shutdown,
	.id_table = lm3642_id,
};

static struct device_attribute lm3642_led=
    __ATTR(lightness, 0664, lm3642_led_torch_mode_get_brightness,
                        lm3642_led_torch_mode_set_brightness);
#ifdef THERMAL_PROTECT
static struct device_attribute thermal_protect_flash_led=
    __ATTR(thermal_protect_flash_led_state, 0664, thermal_protect_flash_led_state_get,
                        thermal_protect_flash_led_state_set);
#endif

/*
 **************************************************************************
 * FunctionName: lm3642_i2c_read;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int lm3642_i2c_read(char *rxData, int length)
{
	int ret = 0;
	struct i2c_msg msgs[] = {
		{
		 .addr = lm3642_client->addr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxData,
		 },
	};

	ret = i2c_transfer(lm3642_client->adapter, msgs, 1);
	if (ret < 0)
		print_error("%s: transfer error %d\n", __func__, ret);

	return ret;
}

/*
 **************************************************************************
 * FunctionName: lm3642_i2c_write;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int lm3642_i2c_write(char *txData, int length)
{
	int ret = 0;
	struct i2c_msg msg[] = {
		{
		 .addr = lm3642_client->addr,
		 .flags = 0,
		 .len = length,
		 .buf = txData,
		 },
	};
	ret = i2c_transfer(lm3642_client->adapter, msg, 1);
	if (ret < 0)
		print_error("%s: transfer error %d\n", __func__, ret);

	return ret;
}

/*
 **************************************************************************
 * FunctionName: lm3642_probe;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
 extern int register_torch_led(struct device_attribute *attr);
static int lm3642_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter;
	struct lm3642_platform_data *pdata = NULL;
	int ret = 0;
	u8 val = 0;
	print_debug("Enter %s", __FUNCTION__);

	adapter = client->adapter;
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA
				     | I2C_FUNC_SMBUS_WRITE_BYTE)) {
		return -EIO;
	}
	lm3642_client = client;

	lm3642_read_reg8(REG_FLASH_FEATURES,&val);
	if(val ==0)
	{
		lm3642_client = NULL;
      		return -1;
	}
	lm3642_write_reg8(REG_IVFM,0x00);

	if(register_torch_led(&lm3642_led))
	{
		print_error( "%s:Unable to create interface\n", __func__, &client->dev);
		return -ENOMEM;
	}

	#ifdef THERMAL_PROTECT 
	if(register_torch_led(&thermal_protect_flash_led))
	{
		print_error( "%s:Unable to create thermal_protect_flash_led interface\n", __func__);
		return -ENOMEM;
	}
	#endif

	/* get gpio block */
	gpio_block = iomux_get_block(K3_FLASH_BLOCK);
	if (!gpio_block) {
		print_error("%s: failed to get iomux %s\n", __func__, K3_FLASH_BLOCK);
		return -EINVAL;
	}

	/* get gpio config block */
	block_conf = iomux_get_blockconfig(K3_FLASH_BLOCK);
	if (!block_conf) {
		print_error("%s: failed to get iomux isp %s\n", __func__, K3_FLASH_BLOCK);
		return -EINVAL;
	}

	/* set gpio work mode */
	ret = blockmux_set(gpio_block, block_conf, NORMAL);
	if (ret != 0) {
		print_error("%s: failed to set iomux flash to gpio mode.\n", __func__);
		return -EINVAL;
	}

	strobe0_pin = PIN_STROBE0;

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    /* detect current device successful, set the flag as present */
        set_hw_dev_flag(DEV_I2C_TPS);
#endif

	return 0;
}

/*
 **************************************************************************
 * FunctionName: lm3642_remove;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int lm3642_remove(struct i2c_client *client)
{
	print_debug("Enter %s", __FUNCTION__);
	lm3642_reset();
	lm3642_client->adapter = NULL;
	return 0;
}

static void lm3642_shutdown(struct i2c_client *client)
{
	print_debug("Enter %s", __FUNCTION__);
	lm3642_reset();
	lm3642_client->adapter = NULL;
	return;
}
/*
 **************************************************************************
 * FunctionName: lm3642_read_reg8;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int lm3642_read_reg8(u8 reg, u8 *val)
{
	int ret;
	u8 buf[1];
	print_debug("Enter Function:%s", __FUNCTION__);

	buf[0] = reg;
	ret = lm3642_i2c_write(buf, 1);
	if (ret < 0) {
		print_error("lm3642 read reg error(%d), reg=%x", ret, reg);
	}

	ret = lm3642_i2c_read(val, 1);
	if (ret < 0) {
		print_error("lm3642 read reg error(%d), reg=%x", ret, reg);
	}

	return ret;

}

/*
 **************************************************************************
 * FunctionName: lm3642_write_reg8;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int lm3642_write_reg8(u8 reg, u8 val)
{
	u8 buf[2];
	print_debug("Enter Function:%s", __FUNCTION__);

	buf[0] = reg;
	buf[1] = val;
	return lm3642_i2c_write(buf, 2);

}

int lm3642_led_torch_mode_on()
{
	int val,ret;

        lm3642_init(); 
        return 0;
}

static lm3642_led_torch_mode_off()
{
	int val,ret;

	if (lm3642_camera_mode_flag)
		return;

	lm3642_exit();

	return 0;
}
static ssize_t lm3642_led_torch_mode_get_brightness(struct device *dev, struct device_attribute *attr,char *buf)
{
        int ret;
        sprintf(buf,"brightness_level= %d\n",brightness_level);
        ret = strlen(buf)+1;
        return ret;
}


static ssize_t lm3642_led_torch_mode_set_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int val,ret;
	//static char status = '0';
	u8 level;
//#ifndef THERMAL_PROTECT
	//static  int led_configed_flag = 0;
//#endif

	if(led_status != buf[0])
	{
	    led_status = buf[0];
	}
	else
	{
	    return count;
	}

	if(kstrtou8(buf,0,&level))
	{
		printk("fail to recover str to U8 %s\n",__func__);
		return -1;
	}
	
    if (level==0)//close
    {
        ret = lm3642_led_torch_mode_off();
        if(ret != 0){
            print_error("lm3642_led_torch_mode_off error");
            return -1;
        }

#ifdef THERMAL_PROTECT
	spin_lock(&led_stat_lock);
#endif
	led_configed_flag = 0;
	brightness_level = 0;
#ifdef THERMAL_PROTECT
	spin_unlock(&led_stat_lock);
#endif
    }
    else 
    {
    		if(level>7) 
		{
			printk("Input the wrong number\n");
			return -1;
		}
/*
    	#ifdef THERMAL_PROTECT
		if(!thermal_protect_get_flash_stat())
		{
			print_info("temperature is too high,can't open the flash led");
			return -1;
		}
		#endif
*/
        if(led_configed_flag == 0)
        {
            ret = lm3642_led_torch_mode_on();
            if(ret != 0){
                print_error("lm3642_led_torch_mode_on error");
                return -1;
            }

			#ifdef THERMAL_PROTECT
			spin_lock(&led_stat_lock);
			#endif
            led_configed_flag = 1;
			#ifdef THERMAL_PROTECT
			spin_unlock(&led_stat_lock);
			#endif
		}

		
	        ret = _lm3642_turn_on(TORCH_MODE, level-1);
            if(ret < 0){
                print_error("set light level error write reg0 = %x",STATE_BRIGHT_LOW);
                return -1;
            	}

			#ifdef THERMAL_PROTECT
			spin_lock(&led_stat_lock);
			#endif
            		brightness_level = level;
			#ifdef THERMAL_PROTECT
			spin_unlock(&led_stat_lock);
			#endif
		}
		
    return count;
}


/*
 **************************************************************************
 * FunctionName: lm3642_cfg_led;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void lm3642_cfg_led(int led, int status)
{
/*
	u8 val;
	print_debug("enter %s", __FUNCTION__);

	lm3642_read_reg8(FLASH_REGISTER5, &val);
	if (status == ENABLE) {
		val = val | (1 << led);
		lm3642_write_reg8(FLASH_REGISTER5, val);
	} else {
		val = val & (~(1 << led));
		lm3642_write_reg8(FLASH_REGISTER5, val);
	}

	led_select = val & 0x07;
*/
	return;
}

/*
 **************************************************************************
 * FunctionName: lm3642_cfg_trig;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void lm3642_cfg_trig(int trigger_mode)
{
#if 0
	u8 val;
	print_debug("enter %s", __FUNCTION__);

	lm3642_read_reg8(FLASH_REGISTER3, &val);
	if (trigger_mode == TRIG_MODE_LEVEL) {
		val = val & (~FLASH_STT_BIT);
		lm3642_write_reg8(FLASH_REGISTER3, val);
	} else if (trigger_mode == TRIG_MODE_EDGE) {
		val = val | FLASH_STT_BIT;
		lm3642_write_reg8(FLASH_REGISTER3, val);
	} else if (trigger_mode == TRIG_MODE_CMD) {
		/* do nothing */
	}
#endif
	return;
}

/*
 **************************************************************************
 * FunctionName: lm3642_init;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int lm3642_init(void)
{
	int ret;
	print_debug("Enter Function:%s", __FUNCTION__);

	if (lm3642_init_flag){
		ret = lm3642_led_torch_mode_off();
		if(ret != 0){
			print_error("lm3642_led_torch_mode_off error");
			return -1;
		}
		#ifdef THERMAL_PROTECT
		spin_lock(&led_stat_lock);
		#endif
		led_configed_flag = 0;
		brightness_level = 0;
		led_status = '0';
		#ifdef THERMAL_PROTECT
		spin_unlock(&led_stat_lock);
		#endif
		//return;
	}

	ret = gpio_request(strobe0_pin, NULL);
	if (ret) {
		print_error("failed to request strobe0 pin of flash ic");
		return -EIO;
	}

	lm3642_init_flag = 1;
	return 0;
}

/*
 **************************************************************************
 * FunctionName: lm3642_exit;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void lm3642_exit(void)
{
	int ret;
	print_debug("enter %s", __FUNCTION__);
	lm3642_turn_off();
	gpio_free(strobe0_pin);

	/* set gpio work mode */
	ret = blockmux_set(gpio_block, block_conf, LOWPOWER);
	if (ret != 0) {
		print_error("%s: failed to set iomux flash to gpio mode.\n", __func__);
		return;
	}

	#ifdef THERMAL_PROTECT
	spin_lock(&led_stat_lock);
	#endif
	lm3642_camera_mode_flag = 0;
	#ifdef THERMAL_PROTECT
	spin_unlock(&led_stat_lock);
	#endif
	lm3642_init_flag = 0;
	return;
}

/*
 **************************************************************************
 * FunctionName: dump_reg;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void dump_reg(void)
{
	u8 val;
	int i;
	print_debug("enter %s", __FUNCTION__);
#if 0
	for (i = 0; i < 8; i++) {
		lm3642_read_reg8(i, &val);
		print_info("read reg 0x%x = 0x%x", i, val);
		msleep(100);
	}
	#endif
}

/*
 **************************************************************************
 * FunctionName: set_strobe0;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void set_strobe0(u32 value)
{
	if (value == 0)
		gpio_direction_output(strobe0_pin, 0);
	else
		gpio_direction_output(strobe0_pin, 1);

	/* for debug
	 *
	 * value = gpio_get_value(strobe0_pin);
	 * print_debug("===gpio[%d] value:%d", strobe0_pin, value);
	 */

}

/*
 **************************************************************************
 * FunctionName: test_flash;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int test_flash(void)
{
	print_debug("enter %s", __FUNCTION__);

	lm3642_turn_on(TORCH_MODE, 0);
	msleep(4000);
	dump_reg();
	lm3642_turn_off();
	msleep(1000);
	dump_reg();

	lm3642_turn_on(FLASH_MODE, 1);
	msleep(1000);
	dump_reg();
	lm3642_turn_off();
	dump_reg();

	return 0;
}

/*
 **************************************************************************
 * FunctionName: lm3642_reset;
 * Description : software reset;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
 static int lm3642_turn_off(void);
int lm3642_reset(void)
{
	print_error("enter %s", __FUNCTION__);

	//gpio_direction_output(reset_pin, 0);	
	lm3642_turn_off();
	return 0;
}

/*
 **************************************************************************
 * FunctionName: lm3642_turn_on;
 * Description : turn on flashlight;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
 static int _lm3642_turn_on(work_mode mode, flash_lum_level lum)
{
	u8 val;
	u8 temp;
	u8 enable=0;

	print_debug("enter %s", __FUNCTION__);
/*
	#ifdef THERMAL_PROTECT 
	if(!thermal_protect_get_flash_stat())
	{
		print_info("temperature is too high,can't open the flash led");
		return -1;
	}
	#endif
*/
	lm3642_read_reg8(REG_FLAGS,&val);//clear error flag,resume chip
	lm3642_read_reg8(REG_CURRENT_CONTROL, &val);	

	if (mode == FLASH_MODE) 
	{		
		temp = lum;		
		val = (val&0xf0)|temp;
		enable = MODE_FLASH|TX_PIN;
		print_debug("start FLASH_MODE");		
	} else if (mode == TORCH_MODE) {	

		val = (lum << 4) + (val&0x0f);
		enable = MODE_TORCH;
		print_debug("start TORCH_MODE");		
	}

	lm3642_write_reg8(REG_CURRENT_CONTROL, val);
	lm3642_write_reg8(REG_ENABLE, enable);

	#ifdef THERMAL_PROTECT
	spin_lock(&led_stat_lock);
	#endif

	#ifdef THERMAL_PROTECT
	spin_unlock(&led_stat_lock);
	#endif

	return 0;
}

static int lm3642_turn_on(work_mode mode, flash_lum_level lum)
{
	int ret;

	if(lum>LUM_LEVEL_MAX)
	{
		print_error("Unsupport lum_level:%d", lum);
		return -1;
	}

	 if (mode == TORCH_MODE) 
	 {		
		lum = TORCH_CURRENT_FOR_PREFLASH;
	 }
	 else
	 {
	 	
	 	lum = FLASH_CURRENT_FOR_FLASH;
	 }

	ret = _lm3642_turn_on(mode,lum);
	if(ret)
	{
		return -1;
	}
	lm3642_camera_mode_flag = 1;
	return ret;
}

/*
 **************************************************************************
 * FunctionName: lm3642_turn_off;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int lm3642_turn_off(void)
{
	u8 val;
	
	print_debug("enter %s", __FUNCTION__);

	#ifdef THERMAL_PROTECT
	spin_lock(&led_stat_lock);
	#endif
	lm3642_camera_mode_flag = 0;
	#ifdef THERMAL_PROTECT
	spin_unlock(&led_stat_lock);
	#endif

	lm3642_read_reg8(REG_FLAGS,&val);
	lm3642_write_reg8(REG_ENABLE, MODE_STANDBY);	

	return 0;
}

#ifdef THERMAL_PROTECT 
static bool thermal_protect_get_flash_stat(void)
{
	return thermal_protect_led_stat;
}

static void thermal_protect_set_flash_stat(bool thermal_led_stat)
{
	thermal_protect_led_stat = thermal_led_stat;
}

static ssize_t thermal_protect_flash_led_state_get(struct device *dev, struct device_attribute *attr,char *buf)
{
    int ret;
    sprintf(buf,"thermal_protect_led_stat = %d\n",thermal_protect_led_stat);
    ret = strlen(buf)+1;
    return ret;
}

static ssize_t thermal_protect_flash_led_state_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	
    if (buf[0] == '0')
    {
    	if(led_configed_flag)
    	{
		    ret = lm3642_led_torch_mode_off();
		    if(ret != 0){
		        print_error("lm3642_led_torch_mode_off error");
		        return -1;
		    }
			spin_lock(&led_stat_lock);
		    led_configed_flag = 0;
		    brightness_level = 0;
			spin_unlock(&led_stat_lock);
    	}

		if(lm3642_camera_mode_flag)
		{
			lm3642_turn_off();
		}
		
		thermal_protect_led_stat = false;
    }
	else if (buf[0] == '1')
	{
		thermal_protect_led_stat = true;
	}
	else 
	{
		printk("Input the wrong number\n");
		return -1;
	}
	return count;
}
#endif

/*
 **************************************************************************
 * FunctionName: lm3642_set_default;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void lm3642_set_default(void)
{
	print_debug("enter %s", __FUNCTION__);

	lm3642_intf.init		= lm3642_init;
	lm3642_intf.exit		= lm3642_exit;
	lm3642_intf.reset		= lm3642_reset;
	lm3642_intf.turn_on		= lm3642_turn_on;
	lm3642_intf.turn_off		= lm3642_turn_off;
	lm3642_intf.test		= test_flash;
	lm3642_intf.type		= LED_FLASH;
}

/*
 **************************************************************************
 * FunctionName: lm3642_module_init;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int __init lm3642_module_init(void)
{
	int ret;

	struct i2c_adapter  *Pr_i2c_adapter=NULL;	

	Pr_i2c_adapter = i2c_get_adapter(I2C_BUS_NUM);
	if(Pr_i2c_adapter ==NULL)
	{
		printk("%s can't get number %d  i2c_adapter!!!!\n",__func__,I2C_BUS_NUM);
		return -1;
	}
	
	lm3642_client = i2c_new_device(Pr_i2c_adapter,&lm3642_info);
	if(lm3642_client == NULL)
	{
		printk("%s can't add device on number %d  i2c_adapter!!!!\n",__func__,I2C_BUS_NUM);
		return -1;
	}
	
	print_debug("enter %s", __FUNCTION__);
	i2c_add_driver(&lm3642_driver);


	if(lm3642_client == NULL)
	{
		printk("%s can't add driver on number %d  i2c_adapter!!!!\n",__func__,I2C_BUS_NUM);
		return -1;
	}

	lm3642_set_default();
	register_camera_flash(&lm3642_intf);
	#ifdef THERMAL_PROTECT 
	thermal_protect_set_flash_stat(true);
	#endif
	return 0;
}

/*
 **************************************************************************
 * FunctionName: lm3642_module_deinit;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void __exit lm3642_module_deinit(void)
{
	print_debug("enter %s", __FUNCTION__);
	i2c_del_driver(&lm3642_driver);

	unregister_camera_flash(&lm3642_intf);
	return;
}


module_init(lm3642_module_init);
module_exit(lm3642_module_deinit);
MODULE_AUTHOR("Jiezhou");
MODULE_DESCRIPTION("lm3642 Flash Driver");
MODULE_LICENSE("GPL");

/********************************** END ***************************************/
