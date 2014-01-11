/*
 * Copyright 2010 HUAWEI Tech. Co., Ltd.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/module.h>
#include <mach/system.h>
#include <mach/platform.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>


#include <mach/gpio.h>

#include <mach/extral_dynamic_dcdc.h>
#include <hsad/config_interface.h>
//#include <mach/product_feature_sel.h>

#define READ_ID_RETRY_NUM  3

#define REG_SET0    0x00
#define REG_SET1    0x01
#define REG_SET2    0x02
#define REG_SET3    0x03
#define REG_CTRL    0x04
#define REG_TEMP    0x05
#define REG_RMPCTRL 0x06
#define REG_ID0     0x08
#define REG_ID1     0x09

#define BIT2  (1<<2)
#define BIT3  (1<<3)
#define BIT4  (1<<4)
#define BIT7  (1<<7)
#define CHID_TPS62360   0
#define CHID_TPS62361B  1
#define CHID_TPS62362   2
#define CHID_TPS62366A  3


#define TPS62360_VOL_MIN    770000
#define TPS62360_VOL_MAX    1400000
#define TPS62361B_VOL_MIN   500000
#define TPS62361B_VOL_MAX   1770000
#define TPS62362_VOL_MIN    770000
#define TPS62362_VOL_MAX    1400000
#define TPS62366A_VOL_MIN    500000
#define TPS62366A_VOL_MAX    1770000
#define VOLTAGE_STEP        10000

#define TPS6236x_MODE_PFM_PWM   0
#define TPS6236x_MODE_FORCE_PWM 1

static int g_moule_init_finish = 0;
static int g_reg_set = REG_SET0;
static int g_chiptype = CHID_TPS62360;
static struct i2c_client * g_tps6236x_i2c_client = NULL;


enum {
	DEBUG_DUG = 1U << 0,
	DEBUG_INFO = 1U << 1,
	DEBUG_WARING = 1U << 2,
	DEBUG_ERR = 1U << 3,
};

static int debug_mask = DEBUG_WARING | DEBUG_ERR;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

typedef struct vcc_extral_dynamic_dcdc_dev_info{
	struct regulator_dev	*rdev;
	unsigned int en_gpio;
};


static int tps6236x_ic_check_init(void)
{
    return g_moule_init_finish;
}

static int tps6236x_sel_reg_set(int vsel0, int vsel1)
{
	if(!vsel0 && !vsel1)        // vsel0 = 0, vsel1 = 0
		g_reg_set = REG_SET0;
	else if(vsel0 && !vsel1)    // vsel0 = 1, vsel1 = 0
		g_reg_set = REG_SET1;
	else if(!vsel0 && vsel1)    // vsel0 = 0, vsel1 = 1
		g_reg_set = REG_SET2;
	else                        // vsel0 = 1, vsel1 = 1
		g_reg_set = REG_SET3;
	return g_reg_set;
}

static int tps6236x_get_chip_type(int chipid)
{
	if(chipid & BIT2)
		return CHID_TPS62361B;
	else if(chipid & BIT3)
		return CHID_TPS62362;
	else if(chipid & BIT4)
	{

		return CHID_TPS62366A;
	}
	else
		return CHID_TPS62360;
}

struct i2c_client* tps6236x_get_i2c_client(void)
{
	return g_tps6236x_i2c_client;
}


static int tps6236x_i2c_read_byte(u8 reg, u8* pdata)
{
	int err = 0;
	struct i2c_client *client = NULL;
	client = tps6236x_get_i2c_client();

	if(!client)
	{
		if (debug_mask & DEBUG_ERR)
			printk(KERN_ERR "%s: i2c client is NULL \n",__func__);
		return -EINVAL;
	}
	err = i2c_smbus_read_byte_data(client,reg);
	if(err < 0)
	{
		if (debug_mask & DEBUG_ERR)
			printk(KERN_ERR "tps6236x read reg[0x%02x] failed return %d\n",reg,err);
		return err;
	}
	else
	{
		*pdata = (u8)err;
	}
	return 0;
}

static int tps6236x_i2c_write_byte(u8 reg, u8 data)
{
	int err = 0;
	struct i2c_client *client = NULL;
	client = tps6236x_get_i2c_client();

	if(!client)
	{
		if (debug_mask & DEBUG_ERR)
			printk(KERN_ERR "%s: i2c client is NULL \n",__func__);
		return -EINVAL;
	}

	err = i2c_smbus_write_byte_data(client, reg, data);
	if(err < 0)
	{
		if (debug_mask & DEBUG_ERR)
			printk(KERN_ERR "tps6236x write reg[0x%02x] failed return %d\n",reg,err);
	}
	return err;
}

static int tps6236x_set_dcdc_work_mode(int mode)
{
	int ret = 0;
	unsigned char reg_value = 0;
	ret = tps6236x_i2c_read_byte(g_reg_set,&reg_value);
	if(ret)
	{
		return  -EINVAL;
	}

	if(TPS6236x_MODE_PFM_PWM == mode)
	{
		reg_value &= 0x7f;
	}
	else
	{
		reg_value |= 0x80;
	}
	ret = tps6236x_i2c_write_byte(g_reg_set,reg_value);
	return ret;

}

static int tps6236x_get_dcdc_work_mode(int* mode)
{
	int ret = 0;
	unsigned char reg_value = 0;
	ret = tps6236x_i2c_read_byte(g_reg_set,&reg_value);
	if(ret)
	{
		return  -EINVAL;
	}

	if(reg_value & BIT7)
		*mode = TPS6236x_MODE_FORCE_PWM;
	else
		*mode = TPS6236x_MODE_PFM_PWM;

	return 0;

}

static int tps6236x_set_voltage(int voltage)
{
	int ret = 0;
	int set_step = 0;
	unsigned char reg_value = 0;

	if (debug_mask & DEBUG_DUG)
		printk(KERN_ERR "%s: CHIP[%d] set voltage %d \n",__func__,g_chiptype,voltage);
	switch(g_chiptype)
	{
		case CHID_TPS62360:
			if((voltage < TPS62360_VOL_MIN) || (voltage > TPS62360_VOL_MAX))
			{
				ret = -EINVAL;
				break;
			}
			set_step = (voltage - TPS62360_VOL_MIN)/VOLTAGE_STEP;
			ret = tps6236x_i2c_read_byte(g_reg_set,&reg_value);
			if(!ret)
			{
				reg_value &= 0x80;
				reg_value |= set_step;
				ret = tps6236x_i2c_write_byte(g_reg_set,reg_value);
				if(ret)
				{
					if (debug_mask & DEBUG_ERR)
						printk(KERN_ERR "%s: CHIP[%d] write reg[%d] val[%d] err return %d \n",
									__func__,g_chiptype,g_reg_set,reg_value,ret);
				}
			}
			else
			{
				if (debug_mask & DEBUG_ERR)
					printk(KERN_ERR "%s: CHIP[%d] read reg[%d] val[%d] err return %d \n",
								__func__,g_chiptype,g_reg_set,reg_value,ret);
			}
			break;
		case CHID_TPS62361B:
			if((voltage < TPS62361B_VOL_MIN) || (voltage > TPS62361B_VOL_MAX))
			{
				ret = -EINVAL;
				break;
			}
			set_step = (voltage - TPS62361B_VOL_MIN)/VOLTAGE_STEP;
			ret = tps6236x_i2c_read_byte(g_reg_set,&reg_value);
			if(!ret)
			{
				reg_value &= 0x80;
				reg_value |= set_step;
				ret = tps6236x_i2c_write_byte(g_reg_set,reg_value);
				if(ret)
				{
					if (debug_mask & DEBUG_ERR)
						printk(KERN_ERR "%s: CHIP[%d] write reg[%d] val[%d] err return %d \n",
									__func__,g_chiptype,g_reg_set,reg_value,ret);
				}
			}
			else
			{
				if (debug_mask & DEBUG_ERR)
					printk(KERN_ERR "%s: CHIP[%d] read reg[%d] val[%d] err return %d \n",
								__func__,g_chiptype,g_reg_set,reg_value,ret);
			}
			break;
		case CHID_TPS62362:
			if((voltage < TPS62362_VOL_MIN) || (voltage > TPS62362_VOL_MAX))
			{
				ret = -EINVAL;
				break;
			}
			set_step = (voltage - TPS62362_VOL_MIN)/VOLTAGE_STEP;
			ret = tps6236x_i2c_read_byte(g_reg_set,&reg_value);
			if(!ret)
			{
				reg_value &= 0x80;
				reg_value |= set_step;
				ret = tps6236x_i2c_write_byte(g_reg_set,reg_value);
				if(ret)
				{
					if (debug_mask & DEBUG_ERR)
						printk(KERN_ERR "%s: CHIP[%d] write reg[%d] val[%d] err return %d \n",
									__func__,g_chiptype,g_reg_set,reg_value,ret);
				}
			}
			else
			{
				if (debug_mask & DEBUG_ERR)
					printk(KERN_ERR "%s: CHIP[%d] read reg[%d] val[%d] err return %d \n",
								__func__,g_chiptype,g_reg_set,reg_value,ret);
			}
			break;
	    case CHID_TPS62366A:
			if((voltage < TPS62366A_VOL_MIN) || (voltage > TPS62366A_VOL_MAX))
			{
				ret = -EINVAL;
				break;
			}
			set_step = (voltage - TPS62366A_VOL_MIN)/VOLTAGE_STEP;
			ret = tps6236x_i2c_read_byte(g_reg_set,&reg_value);
			if(!ret)
			{
				reg_value &= 0x80;
				reg_value |= set_step;
				ret = tps6236x_i2c_write_byte(g_reg_set,reg_value);

				if(ret)
				{
					if (debug_mask & DEBUG_ERR)
						printk(KERN_ERR "%s: CHIP[%d] write reg[%d] val[%d] err return %d \n",
									__func__,g_chiptype,g_reg_set,reg_value,ret);
				}
			}
			else
			{
				if (debug_mask & DEBUG_ERR)
					printk(KERN_ERR "%s: CHIP[%d] read reg[%d] val[%d] err return %d \n",
								__func__,g_chiptype,g_reg_set,reg_value,ret);
			}
			break;
		default:
			if (debug_mask & DEBUG_ERR)
				printk(KERN_ERR "%s: err chipid type %d \n",__func__,g_chiptype);
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int tps6236x_get_voltage(int* voltage)
{
	int ret = 0;
	unsigned char reg_value = 0;

	switch(g_chiptype)
	{
		case CHID_TPS62360:
			ret = tps6236x_i2c_read_byte(g_reg_set,&reg_value);
			if(!ret)
			{
				reg_value &= 0x3f;
				*voltage = TPS62360_VOL_MIN + reg_value * VOLTAGE_STEP ;
				if (debug_mask & DEBUG_DUG)
					printk(KERN_ERR "%s: CHIP[%d] voltage = %d \n",
								__func__,g_chiptype,*voltage);
			}
			else
			{
				if (debug_mask & DEBUG_ERR)
					printk(KERN_ERR "%s: CHIP[%d] read reg[%d] val[%d] err return %d \n",
								__func__,g_chiptype,g_reg_set,reg_value,ret);
			}
			break;

		case CHID_TPS62362:
			ret = tps6236x_i2c_read_byte(g_reg_set,&reg_value);
			if(!ret)
			{
				reg_value &= 0x3f;
				*voltage = TPS62362_VOL_MIN + reg_value * VOLTAGE_STEP ;
				if (debug_mask & DEBUG_DUG)
					printk(KERN_ERR "%s: CHIP[%d] voltage = %d \n",
								__func__,g_chiptype,*voltage);
			}
			else
			{
				if (debug_mask & DEBUG_ERR)
					printk(KERN_ERR "%s: CHIP[%d] read reg[%d] val[%d] err return %d \n",
								__func__,g_chiptype,g_reg_set,reg_value,ret);
			}
			break;
		case CHID_TPS62366A:
			ret = tps6236x_i2c_read_byte(g_reg_set,&reg_value);
			if(!ret)
			{
				reg_value &= 0x7f;
				*voltage = TPS62366A_VOL_MIN + reg_value * VOLTAGE_STEP ;
				if (debug_mask & DEBUG_DUG)
					printk(KERN_ERR "%s: CHIP[%d] voltage = %d \n",
								__func__,g_chiptype,*voltage);
			}
			else
			{
				if (debug_mask & DEBUG_ERR)
					printk(KERN_ERR "%s: CHIP[%d] read reg[%d] val[%d] err return %d \n",
								__func__,g_chiptype,g_reg_set,reg_value,ret);
			}
			break;
		case CHID_TPS62361B:
		default: // default CHID_TPS62361B
			ret = tps6236x_i2c_read_byte(g_reg_set,&reg_value);
			if(!ret)
			{
				reg_value &= 0x7f;
				*voltage = TPS62361B_VOL_MIN + reg_value * VOLTAGE_STEP ;
				if (debug_mask & DEBUG_DUG)
					printk(KERN_ERR "%s: CHIP[%d] voltage = %d \n",
								__func__,g_chiptype,*voltage);
			}
			else
			{
				if (debug_mask & DEBUG_ERR)
					printk(KERN_ERR "%s: CHIP[%d] read reg[%d] val[%d] err return %d \n",
								__func__,g_chiptype,g_reg_set,reg_value,ret);
			}
			break;
	}

	return ret;
}

static int tps6236x_set_power_state(int enable)
{
	struct i2c_client *client = NULL;
	struct extral_dynamic_dcdc_platform_data*  platdata     = NULL;
	int ret = 0;

	client = tps6236x_get_i2c_client();
	platdata = client->dev.platform_data;
	ret = gpio_request(platdata->enable_gpio,"extral_dynamic_dcdc_enable");
	if(ret)
	{
		if (debug_mask & DEBUG_ERR)
			printk(KERN_ERR "%s: request gpio[%d] failed return %d \n",
						__func__,platdata->enable_gpio,ret);
		return ret;
	}
	if(enable)
	{
		gpio_direction_output(platdata->enable_gpio,1);
	}
	else
	{
		gpio_direction_output(platdata->enable_gpio,0);
	}

	gpio_free(platdata->enable_gpio);
	mdelay(1);
	return 0;
}

static int tps6236x_get_power_state(int* enable)
{
	struct i2c_client *client = NULL;
	struct extral_dynamic_dcdc_platform_data*  platdata     = NULL;
	int ret = 0;

	client = tps6236x_get_i2c_client();
	platdata = client->dev.platform_data;
	ret = gpio_request(platdata->enable_gpio,"extral_dynamic_dcdc_enable");
	if(ret)
	{
		if (debug_mask & DEBUG_ERR)
			printk(KERN_ERR "%s: request gpio[%d] failed return %d \n",
						__func__,platdata->enable_gpio,ret);
		return ret;
	}

	*enable = gpio_get_value(platdata->enable_gpio);

	gpio_free(platdata->enable_gpio);
	return 0;
}


static int vcc_extral_dynamic_dcdc_is_enabled(struct regulator_dev *rdev)
{
	int is_enabled = 0;
	int ret = 0;

	if (debug_mask & DEBUG_DUG)
		printk(KERN_ERR "%s: entered!\n",__func__);
		
	if(!tps6236x_ic_check_init())
		return 1;
	
	ret = tps6236x_get_power_state(&is_enabled);

	if (debug_mask & DEBUG_INFO)
		printk(KERN_ERR "%s: ret = %d, is_enabled = %d\n",__func__,ret,is_enabled);
	if(ret)
		return -EIO;
	return is_enabled;
}

static int vcc_extral_dynamic_dcdc_enable(struct regulator_dev *rdev)
{
	if (debug_mask & DEBUG_DUG)
		printk(KERN_ERR "%s: entered!\n",__func__);
	if(!tps6236x_ic_check_init())
		return 0;
	return tps6236x_set_power_state(1);
}

static int vcc_extral_dynamic_dcdc_disable(struct regulator_dev *rdev)
{
	if (debug_mask & DEBUG_DUG)
		printk(KERN_ERR "%s: entered!\n",__func__);
	if(!tps6236x_ic_check_init())
		return 0;
	return tps6236x_set_power_state(0);
}

static int vcc_extral_dynamic_dcdc_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV,
			    unsigned int *selector)
{
	int dcdc_max_uV = 0;
	int dcdc_min_uV = 0;
	int vsel = 0, uVsel = 0;

	if (debug_mask & DEBUG_INFO)
		printk(KERN_ERR "%s: g_chiptype = %d,min_uV = %d, max_uV = %d!\n",__func__,g_chiptype,min_uV,max_uV);
	
	switch(g_chiptype)
	{
		case CHID_TPS62360:
			dcdc_max_uV = TPS62360_VOL_MAX;
			dcdc_min_uV = TPS62360_VOL_MIN;
			break;
		case CHID_TPS62361B:
			dcdc_max_uV = TPS62361B_VOL_MAX;
			dcdc_min_uV = TPS62361B_VOL_MIN;
			break;
		case CHID_TPS62362:
			dcdc_max_uV = TPS62362_VOL_MAX;
			dcdc_min_uV = TPS62362_VOL_MIN;
			break;
		case CHID_TPS62366A:
			dcdc_max_uV = TPS62366A_VOL_MAX;
			dcdc_min_uV = TPS62366A_VOL_MIN;
			break;
		default:
			dcdc_max_uV = TPS62361B_VOL_MAX;
			dcdc_min_uV = TPS62361B_VOL_MIN;
			break;
	}
	if((max_uV < dcdc_min_uV) || (min_uV > dcdc_max_uV))
	{
		if (debug_mask & DEBUG_ERR)
			printk(KERN_ERR "%s: input error max voltage less than the min valtage\n", __func__);
		return -EINVAL;
	}
	if(max_uV > dcdc_max_uV)
	{
		vsel = (dcdc_max_uV - dcdc_min_uV) / VOLTAGE_STEP;
		uVsel = dcdc_max_uV;
	}
	else
	{
		vsel = (max_uV - dcdc_min_uV) / VOLTAGE_STEP;
		uVsel = max_uV;
	}
	*selector = vsel;

	if(!tps6236x_ic_check_init())
		return 0;
	tps6236x_set_voltage(uVsel);
	return 0;
}

static int vcc_extral_dynamic_dcdc_get_voltage(struct regulator_dev * rdev)
{
	int ret = 0, voltage = 0;

	if (debug_mask & DEBUG_INFO)
		printk(KERN_ERR "%s: entered!\n",__func__);
	if(!tps6236x_ic_check_init())
	{
		voltage = 1160000;
		return 0;
	}
	ret = tps6236x_get_voltage(&voltage);
	if(ret)
	voltage = -EINVAL;

	if (debug_mask & DEBUG_INFO)
		printk(KERN_ERR "%s: ret = %d, voltage = %d!\n",__func__,ret,voltage);
	return voltage;
}

static int vcc_extral_dynamic_dcdc_set_mode(struct regulator_dev *rdev, unsigned int mode)
{
	int dcdc_mode = 0;
	if (debug_mask & DEBUG_INFO)
		printk(KERN_ERR "%s: entered! mode = %d\n",__func__,mode);
	if(!tps6236x_ic_check_init())
		return 0;
	return tps6236x_set_dcdc_work_mode(dcdc_mode);
}

static int vcc_extral_dynamic_dcdc_get_mode(struct regulator_dev *rdev)
{
	int dcdc_mode = 0;
	if (debug_mask & DEBUG_INFO)
		printk(KERN_ERR "%s: entered!\n",__func__);
	if(!tps6236x_ic_check_init())
		return 0;

	tps6236x_get_dcdc_work_mode(&dcdc_mode);
	if (debug_mask & DEBUG_INFO)
		printk(KERN_ERR "%s: mode = %d!\n",__func__,dcdc_mode);
	return dcdc_mode;
}

struct regulator_ops vcc_extral_dynamic_dcdc_ops = {
	.is_enabled         = vcc_extral_dynamic_dcdc_is_enabled,
	.enable             = vcc_extral_dynamic_dcdc_enable,
	.disable            = vcc_extral_dynamic_dcdc_disable,
	.set_voltage        = vcc_extral_dynamic_dcdc_set_voltage,
	.get_voltage        = vcc_extral_dynamic_dcdc_get_voltage,
	.set_mode           = vcc_extral_dynamic_dcdc_set_mode,
	.get_mode           = vcc_extral_dynamic_dcdc_get_mode,
};


struct regulator_desc extral_dynamic_dcdc_regulator_desc = {
//	.name = EXTRAL_DYNAMIC_DCDC_NAME,
	.name = "VCC_EXTRAL_DYNAMIC_DCDC",
	.id = 0,
	.ops = &vcc_extral_dynamic_dcdc_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
};

static int tps6236x_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	unsigned char chip_id = 0;
	int icount = 0;
	unsigned int max_voltage = 0, min_voltage = 0;
	struct extral_dynamic_dcdc_platform_data*  platdata     = NULL;
	struct vcc_extral_dynamic_dcdc_dev_info* di = NULL;


	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		if (debug_mask & DEBUG_ERR)
			dev_err(&client->dev, "%s: failed to check i2c function!\n", __func__);
		ret = -EIO;
		goto err_check_i2c_func;
	}
	

	platdata = client->dev.platform_data;
	if (!platdata) {
		ret = -EINVAL;
		goto err_get_platform_data;
	}

	di = kzalloc(sizeof(struct vcc_extral_dynamic_dcdc_dev_info), GFP_KERNEL);
	if (di == NULL) {
		if (debug_mask & DEBUG_ERR)
			pr_err("Regulator vcc extral dcdc: kzalloc mem fail, please check!\n");
		ret = -ENOMEM;
		goto err_alloc_dev_data;
	}
	


	client->dev.init_name = EXTRAL_DYNAMIC_DCDC_NAME;
	g_tps6236x_i2c_client = client;
	for(icount = 0; icount < READ_ID_RETRY_NUM; icount++)
	{
		ret = tps6236x_i2c_read_byte(REG_ID0, &chip_id);
		if (debug_mask & DEBUG_DUG)
			printk(KERN_ERR "%s: chip_id = 0x%02x \n",__func__,chip_id);
		if(!ret)
		{
			break;
		}
		mdelay(10);
	}
	if(ret)
		goto err_get_chipid;

	g_chiptype = tps6236x_get_chip_type(chip_id);
	g_reg_set = tps6236x_sel_reg_set(1,0);  // s10 vsel0 is high and vsel1 is high


	switch(g_chiptype)
	{   
		case CHID_TPS62360:
			max_voltage = TPS62360_VOL_MAX;
			min_voltage = TPS62360_VOL_MIN;
			break;
		case CHID_TPS62361B:
			max_voltage = TPS62361B_VOL_MAX;
			min_voltage = TPS62361B_VOL_MIN;
			break;
		case CHID_TPS62362:
			max_voltage = TPS62362_VOL_MAX;
			min_voltage = TPS62362_VOL_MIN;
			break;
		case CHID_TPS62366A:
			max_voltage = TPS62366A_VOL_MAX;
			min_voltage = TPS62366A_VOL_MIN;
			break;
		default:
			max_voltage = TPS62361B_VOL_MAX;
			min_voltage = TPS62361B_VOL_MIN;
			break;
	}
	platdata->regulator_data->constraints.max_uV = max_voltage;
	platdata->regulator_data->constraints.min_uV= min_voltage;
	di->rdev = regulator_register(&extral_dynamic_dcdc_regulator_desc, &client->dev,
	platdata->regulator_data, di);
	if (IS_ERR(di->rdev)) {
		if (debug_mask & DEBUG_ERR)
			printk(KERN_ERR "%s: register regulator extral dynamic dcdc failed \n",__func__);
		goto err_register_regulator;
	}

	i2c_set_clientdata(client, di);

	ret = tps6236x_i2c_write_byte(REG_RMPCTRL, 0x62);
	if (ret) {
		WARN(1, "tps6236x set RMPCTRL err!\n", ret);
		goto err_check_i2c_func;
	}
	g_moule_init_finish = 1;
#if 1
	{
		ret = tps6236x_set_power_state(1);
		if(ret)
			printk(KERN_ERR "------------enable tps6236x failed!\n");
		else
			printk(KERN_ERR "************enable tps6236x succeed!\n");

		//add for test voltage
		int voltage = 1160000;
		//   for(icount = 0; icount < 128; icount++)
		{
			voltage += 10*icount;
			ret = tps6236x_set_voltage(voltage);
			if(ret)
			printk(KERN_ERR "------------SET %d vol failed!\n",voltage);
			else
			printk(KERN_ERR "************SET %d vol succeed!\n",voltage);
			//   mdelay(1000);
			//   icount++;
		}
	}
#endif
	if (debug_mask & DEBUG_INFO)
		printk(KERN_ERR "%s: probe succeed!!\n", __func__);
	return 0;

err_register_regulator:
err_get_chipid:
	kfree(di);
	di = NULL;
err_alloc_dev_data:
err_get_platform_data:
err_check_i2c_func:
	if (debug_mask & DEBUG_ERR)
		printk(KERN_ERR "%s: probe failed!!\n", __func__);
	return ret;
}

static int tps6236x_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id tps6236x_id[] = {
	{EXTRAL_DYNAMIC_DCDC_NAME, 0 },
	{ }
};

static struct i2c_driver tps6236x_driver = {
	.probe		= tps6236x_probe,
	.remove		= tps6236x_remove,
	.id_table	= tps6236x_id,
	.driver = {
		.name	= EXTRAL_DYNAMIC_DCDC_NAME,
	},
};


static int __devinit tps6236x_init(void)
{
    if(0 == get_gpu_dcdc_supply())
    {
        return -1;
    }
	return i2c_add_driver(&tps6236x_driver);
}

static void __exit tps6236x_exit (void)
{
	i2c_del_driver(&tps6236x_driver);
}

subsys_initcall(tps6236x_init);
module_exit(tps6236x_exit);

MODULE_AUTHOR("Hisilicon K3");
MODULE_DESCRIPTION("I2C tps6236x Driver");
MODULE_LICENSE("GPL");
