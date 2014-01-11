/* Copyright (c) 2011, Huawei Technology Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <asm/io.h>
#include <linux/mux.h>
#include <mach/ak6921af.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <hsad/config_interface.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <hsad/config_interface.h>

static struct i2c_client *ak6921af_client = NULL;

static int ak6921af_i2c_read(struct i2c_client *client, uint8_t cmd, uint8_t *data, uint8_t length)
{
	int retry;
	uint8_t addr[1];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = addr, /*i2c address*/
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		}
	};
	addr[0] = cmd & 0xFF;

	for (retry = 0; retry < AK6921AF_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2)/* read two times,because msg[2] */
			break;
		mdelay(10);
	}
	if (retry == AK6921AF_I2C_RETRY_TIMES) {
		printk(KERN_ERR "i2c_read_block retry over %d\n",
			AK6921AF_I2C_RETRY_TIMES);
		return -EIO;
	}
	return 0;

}

static int ak6921af_i2c_write(struct i2c_client *client, uint8_t cmd, uint8_t *data, uint8_t length)
{
	int retry, loop_i;
	uint8_t buf[length + 1];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	buf[0] = cmd & 0xFF;

	for (loop_i = 0; loop_i < length; loop_i++)
		buf[loop_i + 1] = data[loop_i];

	for (retry = 0; retry < AK6921AF_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		mdelay(10);
	}

	if (retry == AK6921AF_I2C_RETRY_TIMES) {
		printk(KERN_ERR "i2c_write_block retry over %d\n",
			AK6921AF_I2C_RETRY_TIMES);
		return -EIO;
	}
	return 0;    
}

int ak6921af_write_eeprom(int value)
{
    uint8_t data;
    
    data = 1;
    if (ak6921af_client == NULL) {
        return AK6921AF_NG;    
    }
    if (ak6921af_i2c_write(ak6921af_client, CMD_REG_EEPROM_SWITCH, &data, 1) < 0) {
        pr_err("change to eeprom mode error, %s\n", __func__);
        return AK6921AF_NG;
    }
    
    if (ak6921af_i2c_write(ak6921af_client, CMD_WRITE_ENABLE, NULL, 0) < 0) {
        pr_err("eeprom write enable error, %s\n", __func__);
        return AK6921AF_NG;
    }
    
    data = ((uint8_t)value) & 0x1;
    if (ak6921af_i2c_write(ak6921af_client, CMD_OUTPUT, &data, 1) < 0) {
        pr_err("write out reg to set eeprom error, %s\n", __func__);
        return AK6921AF_NG;
    }
    mdelay(5);
    
    if (ak6921af_i2c_write(ak6921af_client, CMD_WRITE_DISABLE, NULL, 0) < 0) {
        pr_err("eeprom write disable error, %s\n", __func__);
        return AK6921AF_NG;
    }
    
    data = 0;
    if (ak6921af_i2c_write(ak6921af_client, CMD_REG_EEPROM_SWITCH, &data, 1) < 0) {
        pr_err("change to register mode, %s\n", __func__);
        return AK6921AF_NG;    
    }
    return AK6921AF_OK;
}
EXPORT_SYMBOL(ak6921af_write_eeprom);

int ak6921af_read_eeprom(void)
{
    uint8_t data, result; 
    
    data = 1;
    if (ak6921af_client == NULL) {
        return AK6921AF_NG;    
    }
    if (ak6921af_i2c_write(ak6921af_client, CMD_REG_EEPROM_SWITCH, &data, 1) < 0) {
        pr_err("change to eeprom mode error, %s\n", __func__);
        return AK6921AF_NG; 
    }
    
    result = 0;    
    if (ak6921af_i2c_read(ak6921af_client, CMD_OUTPUT, &result, 1) < 0) {
        pr_err("read out reg to read eeprom error, %s\n", __func__);
        return AK6921AF_NG; 
    }
    result = result & 0x1;
    //printk("ak6921af eeprom value:0x%x\n", result);
    
    data = 0;
    if (ak6921af_i2c_write(ak6921af_client, CMD_REG_EEPROM_SWITCH, &data, 1) < 0) {
        pr_err("change to register mode error, %s\n", __func__);
        return AK6921AF_NG;
    }
    
    return result;
}
EXPORT_SYMBOL(ak6921af_read_eeprom);
    
static int ak6921af_i2c_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    int ret = 0;    
    ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (!ret)
	{
		pr_err("i2c_check_functionality failed\n");
		goto probe_failure;
	}
    ak6921af_client = client;    
    return ret;
probe_failure:
	printk("ak6921af_i2c_probe failed! ret = %d\n", ret);
	return AK6921AF_NG;
}

static int ak6921af_i2c_remove(struct i2c_client *client)
{    
    return 0;
}
#if 0
static int ak6921af_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
    return 0;
}

static int ak6921af_i2c_resume(struct i2c_client *client)
{   
    return 0;
}
#endif
static struct i2c_device_id ak6921af_i2c_idtable[] = {
    {AK6921AF_NAME, 0},
};

static struct i2c_driver ak6921af_i2c_driver = {
    .driver = {
        .name = AK6921AF_NAME,
    },
    .id_table 	= ak6921af_i2c_idtable,
    .probe     = ak6921af_i2c_probe,
    .remove 	= __devexit_p(ak6921af_i2c_remove),
#if 0
    .suspend	= ak6921af_i2c_suspend,
    .resume 	= ak6921af_i2c_resume,
#endif
};

static int __init ak6921af_init_module(void)
{
    if(!is_felica_exist())
		return -ENODEV;
    return i2c_add_driver(&ak6921af_i2c_driver);	
}

static void __exit ak6921af_exit_module(void)
{ 
    i2c_del_driver(&ak6921af_i2c_driver);    
}

module_init(ak6921af_init_module);
module_exit(ak6921af_exit_module);
MODULE_DESCRIPTION("ak6921af driver");
MODULE_LICENSE("GPL v2");
