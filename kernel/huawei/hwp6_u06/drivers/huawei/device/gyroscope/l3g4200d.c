/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name		: l3g4200d_gyr_sysfs.c
* Authors		: MH - C&I BU - Application Team
*			: Carmine Iascone (carmine.iascone@st.com)
*			: Matteo Dameno (matteo.dameno@st.com)
*			: Both authors are willing to be considered the contact
*			: and update points for the driver.
* Version		: V 1.1 sysfs
* Date			: 2011/02/28
* Description		: L3G4200D digital output gyroscope sensor API
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
********************************************************************************
* REVISON HISTORY
*
* VERSION	| DATE		| AUTHORS		| DESCRIPTION
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include	<linux/earlysuspend.h>
#include "l3g4200d.h"
#include <linux/board_sensors.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif
#ifndef DEBUG
#define DEBUG
#endif
static struct workqueue_struct *gyro_wq;
static struct l3g4200d_data *gyro_temp; 

static const struct output_rate odr_table[] = {
	{	2,		GYRO_ODR800 | BW10},
	{	3,		GYRO_ODR400 | BW01},
	{	5,		GYRO_ODR200 | BW00},
	{	10,		GYRO_ODR100 | BW00},
};

static int l3g4200d_i2c_read(struct l3g4200d_data *gyro,
						u8 *buf, int len)
{
	int err = -1;

	struct i2c_msg msgs[] = {
		{
			.addr = gyro->client->addr,
			.flags = gyro->client->flags & I2C_M_TEN,
			.len = 1,
			.buf = buf,
		},
		{
			.addr = gyro->client->addr,
			.flags = (gyro->client->flags & I2C_M_TEN) | I2C_M_RD,
			.len = len,
			.buf = buf,
		},
	};

	err = i2c_transfer(gyro->client->adapter, msgs, 2);
	if (err != 2) {
		dev_err(&gyro->client->dev, "read transfer error: %d\n", err);
		return -EIO;
	}

	return 0;
}

static int l3g4200d_i2c_write(struct l3g4200d_data *gyro,
						u8 *buf, int len)
{
	int err = -1;
	struct i2c_msg msgs[] = {
		{
		 .addr = gyro->client->addr,
		 .flags = gyro->client->flags & I2C_M_TEN,
		 .len = len + 1,
		 .buf = buf,
		 },
	};

	err = i2c_transfer(gyro->client->adapter, msgs, 1);
	if (err != 1) {
		dev_err(&gyro->client->dev, "write transfer error: %d\n", err);
		return -EIO;
	}

	return 0;
}

static int l3g4200d_register_write(struct l3g4200d_data *gyro, u8 *buf,
		u8 reg_address, u8 new_value)
{

	/* Sets configuration register at reg_address
	 *  NOTE: this is a straight overwrite
	 */
	buf[0] = reg_address;
	buf[1] = new_value;

	return l3g4200d_i2c_write(gyro, buf, 1);
}

static int l3g4200d_register_read(struct l3g4200d_data *gyro, u8 *buf,
		u8 reg_address)
{
	buf[0] = reg_address;

	return l3g4200d_i2c_read(gyro, buf, 1);
}

static int l3g4200d_register_update(struct l3g4200d_data *gyro, u8 *buf,
		u8 reg_address, u8 mask, u8 new_bit_values)
{
	int err = -1;
	u8 init_val;
	u8 updated_val;

	err = l3g4200d_register_read(gyro, buf, reg_address);
	if (err < 0)
		return err;

	init_val = buf[0];
	updated_val = ((mask & new_bit_values) | ((~mask) & init_val));
	err = l3g4200d_register_write(gyro, buf, reg_address,
				updated_val);

	return err;
}

static int l3g4200d_update_fs_range(struct l3g4200d_data *gyro,
							u8 new_fs)
{
	int err = -1 ;
	u8 buf[2];

	buf[0] = CTRL_REG4;

	err = l3g4200d_register_update(gyro, buf, CTRL_REG4,
						FS_MASK, new_fs);

	if (err < 0) {
		dev_err(&gyro->client->dev, "%s : failed to update fs:0x%02x\n",
			 __func__, new_fs);
		return err;
	}

	gyro->resume_state[GYRO_RES_CTRL_REG4] =
		((FS_MASK & new_fs) |
		(~FS_MASK & gyro->resume_state[GYRO_RES_CTRL_REG4]));

	return err;
}

static int l3g4200d_selftest(struct l3g4200d_data *gyro, u8 enable)
{
	int err = -1;
	u8 buf[2] = {0x00, 0x00};
	char reg_address, mask, bit_values;

	reg_address = CTRL_REG4;
	mask = SELFTEST_MASK;
    /*st code logic error*/
	if (enable > 0) {
		bit_values = L3G4200D_SELFTEST_EN_POS;
	}else {
		bit_values = L3G4200D_SELFTEST_DIS;
	}

	mutex_lock(&gyro->lock);
	err = l3g4200d_register_update(gyro, buf, reg_address,
		mask, bit_values);
	if (err < 0) {
		dev_err(&gyro->client->dev, "%s : failed to update:0x%02x\n",
			      __func__, bit_values);
		mutex_unlock(&gyro->lock);
		return err;
	}
	gyro->selftest_enabled = enable;
	mutex_unlock(&gyro->lock);
	//gyro->resume_state[GYRO_RES_CTRL_REG4] = ((mask & bit_values) |
	//		(~mask & gyro->resume_state[GYRO_RES_CTRL_REG4]));

/*
	if (atomic_read(&gyro->enabled)) {
		mutex_lock(&gyro->lock);
		err = l3g4200d_register_update(gyro, buf, reg_address,
				mask, bit_values);
		if (err < 0) {
			dev_err(&gyro->client->dev, "%s : failed to update:0x%02x\n",
				 __func__, bit_values);
			mutex_unlock(&gyro->lock);
			return err;
		}
		gyro->selftest_enabled = enable;
		mutex_unlock(&gyro->lock);

		gyro->resume_state[GYRO_RES_CTRL_REG4] = ((mask & bit_values) |
			(~mask & gyro->resume_state[GYRO_RES_CTRL_REG4]));
	}
*/
	return err;
}

static int l3g4200d_update_odr(struct l3g4200d_data *gyro,
				int poll_interval)
{
	int err = -1;
	int i;
	u8 config[2];

	for (i = ARRAY_SIZE(odr_table) - 1; i >= 0; i--) {
		if (odr_table[i].poll_rate_ms <= poll_interval)
			break;
	}
	if (i < 0)
		i = 0;

	config[1] = odr_table[i].mask;
	config[1] |= (ENABLE_ALL_AXES + PM_NORMAL);

	/* If device is currently enabled, we need to write new
	*  configuration out to it
	*/
	if (atomic_read(&gyro->enabled)) {
		config[0] = CTRL_REG1;
		err = l3g4200d_i2c_write(gyro, config, 1);
		if (err < 0)
			return err;
		gyro->resume_state[GYRO_RES_CTRL_REG1] = config[1];
	}
	return err;
}

/* gyroscope data readout */
static int l3g4200d_get_data(struct l3g4200d_data *gyro,
					struct l3g4200d_triple *data)
{
	int err;
	unsigned char gyro_out[6];
	/* y,p,r hardware data */
	s16 hw_d[3] = { 0 };

	gyro_out[0] = (AUTO_INCREMENT | AXISDATA_REG);
	err = l3g4200d_i2c_read(gyro, gyro_out, 6);
	if (err < 0) {
		dev_err(&gyro->client->dev, "%s l3g4200d_get_data failed err= %d\n",
					L3G4200D_GYR_DEV_NAME, err);
		return err;
	}

	hw_d[0] = (s16) (((gyro_out[1]) << 8) | gyro_out[0]);
	hw_d[1] = (s16) (((gyro_out[3]) << 8) | gyro_out[2]);
	hw_d[2] = (s16) (((gyro_out[5]) << 8) | gyro_out[4]);

	data->x = ((gyro->pdata->negate_x) ? (-hw_d[gyro->pdata->axis_map_x])
		   : (hw_d[gyro->pdata->axis_map_x]));
	data->y = ((gyro->pdata->negate_y) ? (-hw_d[gyro->pdata->axis_map_y])
		   : (hw_d[gyro->pdata->axis_map_y]));
	data->z = ((gyro->pdata->negate_z) ? (-hw_d[gyro->pdata->axis_map_z])
		   : (hw_d[gyro->pdata->axis_map_z]));

	dev_dbg(&gyro->client->dev, "gyro_out: y = %d x = %d z= %d\n",
		data->y, data->x, data->z);

	return err;
}

static void l3g4200d_report_values(struct l3g4200d_data *l3g,
						struct l3g4200d_triple *data)
{
	input_report_abs(l3g->input_dev, ABS_X, data->x);
	input_report_abs(l3g->input_dev, ABS_Y, data->y);
	input_report_abs(l3g->input_dev, ABS_Z, data->z);
	input_sync(l3g->input_dev);
}

static int l3g4200d_hw_init(struct l3g4200d_data *gyro)
{
	int err = -1;
	u8 buf[6];

	dev_dbg(&gyro->client->dev, "%s hw init\n", L3G4200D_GYR_DEV_NAME);

	buf[0] = (AUTO_INCREMENT | CTRL_REG1);
	buf[1] = gyro->resume_state[GYRO_RES_CTRL_REG1];
	buf[2] = gyro->resume_state[GYRO_RES_CTRL_REG2];
	buf[3] = gyro->resume_state[GYRO_RES_CTRL_REG3];
	buf[4] = gyro->resume_state[GYRO_RES_CTRL_REG4];
	buf[5] = gyro->resume_state[GYRO_RES_CTRL_REG5];

	err = l3g4200d_i2c_write(gyro, buf, 5);
	if (err < 0) {
		dev_err(&gyro->client->dev, "%s l3g4200d_hw_init failed err= %d\n",
					L3G4200D_GYR_DEV_NAME, err);
		return err;
	}

	gyro->hw_initialized = 1;

	return err;
}

static void l3g4200d_device_power_off(struct l3g4200d_data *dev_data)
{
	int err;
	u8 buf[2];
	buf[0] = CTRL_REG1;
	buf[1] = PM_OFF;

	dev_dbg(&dev_data->client->dev, "%s power off\n", L3G4200D_GYR_DEV_NAME);

	err = l3g4200d_i2c_write(dev_data, buf, 1);
	if (err < 0)
		dev_err(&dev_data->client->dev, "soft power off failed\n");

	if (dev_data->pdata->power_off) {
		dev_data->pdata->power_off();
		dev_data->hw_initialized = 0;
	}

	if (dev_data->hw_initialized)
		dev_data->hw_initialized = 0;

}

static int l3g4200d_device_power_on(struct l3g4200d_data *dev_data)
{
	int err;
	if (dev_data->pdata->power_on) {
		err = dev_data->pdata->power_on();
		if (err < 0)
			return err;
	}
	if (!dev_data->hw_initialized) {
		err = l3g4200d_hw_init(dev_data);
		if (err < 0) {
			l3g4200d_device_power_off(dev_data);
			return err;
		}
	}
	return 0;
}

static int l3g4200d_enable(struct l3g4200d_data *dev_data)
{
	int err;
	printk("[%s] +\n", __func__);
	if (!atomic_cmpxchg(&dev_data->enabled, 0, 1)) {

		err = l3g4200d_device_power_on(dev_data);
		if (err < 0) {
			atomic_set(&dev_data->enabled, 0);
			return err;
		}

		msleep(400);
		hrtimer_start(&dev_data->timer, ktime_set(0, 10000000), HRTIMER_MODE_REL);

	}
	printk("[%s] -\n", __func__);
	return 0;
}

static int l3g4200d_disable(struct l3g4200d_data *dev_data)
{
	printk("[%s] +\n", __func__);
	if (atomic_cmpxchg(&dev_data->enabled, 1, 0)) {
		hrtimer_cancel(&dev_data->timer);
		cancel_work_sync(&dev_data->work);
		l3g4200d_device_power_off(dev_data);
	}
	printk("[%s] -\n", __func__);
	return 0;
}

static ssize_t attr_polling_rate_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int val;
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	mutex_lock(&gyro->lock);
	val = gyro->pdata->poll_interval;
	mutex_unlock(&gyro->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_polling_rate_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	unsigned long interval_ms = 0;

	if (strict_strtoul(buf, 10, &interval_ms))
		return -EINVAL;
	if (!interval_ms)
		return -EINVAL;

	mutex_lock(&gyro->lock);
	gyro->pdata->poll_interval = interval_ms;
	l3g4200d_update_odr(gyro, interval_ms);
	mutex_unlock(&gyro->lock);

	return size;
}

static ssize_t attr_range_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	int range = 0;
	char val = 0;
	mutex_lock(&gyro->lock);
	val = gyro->pdata->fs_range;
	switch (val) {
	case L3G4200D_GYR_FS_250DPS:
		range = 250;
		break;
	case L3G4200D_GYR_FS_500DPS:
		range = 500;
		break;
	case L3G4200D_GYR_FS_2000DPS:
		range = 2000;
		break;
	}
	mutex_unlock(&gyro->lock);
	return sprintf(buf, "%d\n", range);
}

static ssize_t attr_range_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	unsigned long val = 0;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	mutex_lock(&gyro->lock);
	gyro->pdata->fs_range = val;
	l3g4200d_update_fs_range(gyro, val);
	mutex_unlock(&gyro->lock);

	return size;
}

static ssize_t attr_enable_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	int val = atomic_read(&gyro->enabled);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_enable_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	if (val)
		l3g4200d_enable(gyro);
	else
		l3g4200d_disable(gyro);

	return size;
}

static ssize_t attr_get_selftest(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int val;
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	mutex_lock(&gyro->lock);
	val = gyro->selftest_enabled;
	mutex_unlock(&gyro->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_set_selftest(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	unsigned long val;
	int err;
	int data;
	struct l3g4200d_triple data_out;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	if (ENABLE_SELFTEST == val) {
		gyro->resume_state[GYRO_RES_CTRL_REG1] = 0x6F;
		gyro->resume_state[GYRO_RES_CTRL_REG2] = 0x00;
		gyro->resume_state[GYRO_RES_CTRL_REG3] = 0x00;
		gyro->resume_state[GYRO_RES_CTRL_REG4] = 0xA0;
		gyro->resume_state[GYRO_RES_CTRL_REG5] = 0x02;

		err = l3g4200d_device_power_on(gyro);

		l3g4200d_selftest(gyro, val);
		/* mdelay(800);*/
		msleep(800);

		mutex_lock(&gyro->lock);
		err = l3g4200d_get_data(gyro, &data_out);
		mutex_unlock(&gyro->lock);
		if (err < 0) {
			dev_err(&gyro->client->dev, "%s get_gyroscope_data failed err= %d\n",
						L3G4200D_GYR_DEV_NAME, err);
			return err;
		}

		/*self_test value*/
		data = abs(data_out.x);
		if ((data > MAX_VAL) || (data < MIN_VAL)) {
			dev_err(&gyro->client->dev, "selftest_gyro-------MAX-MIN--X-==%d\n", data);
			return ERR_SELFTEST;
		}
		data = abs(data_out.y);
		if ((data > MAX_VAL) || (data < MIN_VAL)) {
			dev_err(&gyro->client->dev, "selftest_gyro-------MAX-MIN--Y-==%d\n", data);
			return ERR_SELFTEST;
		}
		data = abs(data_out.z);
		if ((data > MAX_VAL) || (data < MIN_VAL)) {
			dev_err(&gyro->client->dev, "selftest_gyro-------MAX-MIN--Z-==%d\n", data);
			return ERR_SELFTEST;
		}
		return TRUE_SELFTEST;
	} else {
		l3g4200d_selftest(gyro, val);
		l3g4200d_device_power_off(gyro);
		dev_err(&gyro->client->dev, "selftest_gyro-------out\n");
		return DISABLE_SELFTEST;
	}

}

 int set_selftest(int value)
{
	int err, result;
	int data;
	struct l3g4200d_triple data_out;

         u8 old_ctrl_reg_state[5];
	if(gyro_temp == NULL ||gyro_temp->pdata == NULL) return DISABLE_SELFTEST;
	if (ENABLE_SELFTEST == value) {
		old_ctrl_reg_state[0] = gyro_temp->resume_state[GYRO_RES_CTRL_REG1] ;
		old_ctrl_reg_state[1] = gyro_temp->resume_state[GYRO_RES_CTRL_REG2] ;
		old_ctrl_reg_state[2] = gyro_temp->resume_state[GYRO_RES_CTRL_REG3] ;
		old_ctrl_reg_state[3] = gyro_temp->resume_state[GYRO_RES_CTRL_REG4] ;
		old_ctrl_reg_state[4] = gyro_temp->resume_state[GYRO_RES_CTRL_REG5] ;

		gyro_temp->resume_state[GYRO_RES_CTRL_REG1] = 0x6F;
		gyro_temp->resume_state[GYRO_RES_CTRL_REG2] = 0x00;
		gyro_temp->resume_state[GYRO_RES_CTRL_REG3] = 0x00;
		gyro_temp->resume_state[GYRO_RES_CTRL_REG4] = 0xA0;
		gyro_temp->resume_state[GYRO_RES_CTRL_REG5] = 0x02;

		err = l3g4200d_device_power_on(gyro_temp);
	          if(err){
	                     dev_err(&gyro_temp->client->dev, "gyroscope power on failed err= %d\n", err);
				result = ERR_SELFTEST;
				goto TEST_RESULT;
	          }
		l3g4200d_selftest(gyro_temp, 1);
		msleep(800);

		mutex_lock(&gyro_temp->lock);
		err = l3g4200d_get_data(gyro_temp, &data_out);
		mutex_unlock(&gyro_temp->lock);
		if (err < 0) {
			dev_err(&gyro_temp->client->dev, "%s get_gyroscope_data failed err= %d\n",
						L3G4200D_GYR_DEV_NAME, err);
			result = ERR_SELFTEST;
			goto TEST_RESULT;
		}

		/*self_test value*/
		data = abs(data_out.x);
		if ((data > MAX_VAL) || (data < MIN_VAL)) {
			dev_err(&gyro_temp->client->dev, "selftest_gyro-------MAX-MIN--X-==%d\n", data);
			result = ERR_SELFTEST;
			goto TEST_RESULT;
		}
		data = abs(data_out.y);
		if ((data > MAX_VAL) || (data < MIN_VAL)) {
			dev_err(&gyro_temp->client->dev, "selftest_gyro-------MAX-MIN--Y-==%d\n", data);
			result = ERR_SELFTEST;
			goto TEST_RESULT;
		}
		data = abs(data_out.z);
		if ((data > MAX_VAL) || (data < MIN_VAL)) {
			dev_err(&gyro_temp->client->dev, "selftest_gyro-------MAX-MIN--Z-==%d\n", data);
			result = ERR_SELFTEST;
			goto TEST_RESULT;
		}
		result = TRUE_SELFTEST;
TEST_RESULT:
		if(result == TRUE_SELFTEST){
			dev_info(&gyro_temp->client->dev, "gyro self test ok---------\n");
			err = set_gyro_selfTest_result(GYRO, "1");
			if (err) {
				dev_err(&gyro_temp->client->dev, " gyro set self test result  fail\n");
			}
		}else {
			err = set_gyro_selfTest_result(GYRO, "0");
			if (err) {
				dev_err(&gyro_temp->client->dev, "gyro set self test result	fail\n");
			}
			dev_err(&gyro_temp->client->dev, "gyro self test fail\n");
		}
		msleep(50);

		gyro_temp->resume_state[GYRO_RES_CTRL_REG1] = old_ctrl_reg_state[0];
		gyro_temp->resume_state[GYRO_RES_CTRL_REG2] = old_ctrl_reg_state[1];
		gyro_temp->resume_state[GYRO_RES_CTRL_REG3] = old_ctrl_reg_state[2];
		gyro_temp->resume_state[GYRO_RES_CTRL_REG4] = old_ctrl_reg_state[3];
		gyro_temp->resume_state[GYRO_RES_CTRL_REG5] = old_ctrl_reg_state[4];

		err = l3g4200d_hw_init(gyro_temp);
		if (err < 0) {
			dev_err(&gyro_temp->client->dev, "gyro set self test reg resume fail\n");
		}
		if (0==atomic_read(&gyro_temp->enabled)){
			gyro_temp->hw_initialized = 0;
		}

		dev_info(&gyro_temp->client->dev, "selftest_gyro-------out\n");
		return DISABLE_SELFTEST;
	}

}


static ssize_t l3g4200d_self_test(struct l3g4200d_data *gyro)
{
       int err, result;
	int data;
	struct l3g4200d_triple data_out;

       gyro->resume_state[GYRO_RES_CTRL_REG1] = 0x6F;
	gyro->resume_state[GYRO_RES_CTRL_REG2] = 0x00;
	gyro->resume_state[GYRO_RES_CTRL_REG3] = 0x00;
	gyro->resume_state[GYRO_RES_CTRL_REG4] = 0xA0;
	gyro->resume_state[GYRO_RES_CTRL_REG5] = 0x02;

	err = l3g4200d_device_power_on(gyro);
          if(err){
                     dev_err(&gyro->client->dev, "gyroscope power on failed err= %d\n", err);
			result = ERR_SELFTEST;
			goto TEST_RESULT;
          }
	l3g4200d_selftest(gyro, 1);
	msleep(800);

	mutex_lock(&gyro->lock);
	err = l3g4200d_get_data(gyro, &data_out);
	mutex_unlock(&gyro->lock);
	if (err < 0) {
		dev_err(&gyro->client->dev, "%s get_gyroscope_data failed err= %d\n",
					L3G4200D_GYR_DEV_NAME, err);
		result = ERR_SELFTEST;
		goto TEST_RESULT;
	}

	/*self_test value*/
	data = abs(data_out.x);
	if ((data > MAX_VAL) || (data < MIN_VAL)) {
		dev_err(&gyro->client->dev, "selftest_gyro-------MAX-MIN--X-==%d\n", data);
		result = ERR_SELFTEST;
		goto TEST_RESULT;
	}
	data = abs(data_out.y);
	if ((data > MAX_VAL) || (data < MIN_VAL)) {
		dev_err(&gyro->client->dev, "selftest_gyro-------MAX-MIN--Y-==%d\n", data);
		result = ERR_SELFTEST;
		goto TEST_RESULT;
	}
	data = abs(data_out.z);
	if ((data > MAX_VAL) || (data < MIN_VAL)) {
		dev_err(&gyro->client->dev, "selftest_gyro-------MAX-MIN--Z-==%d\n", data);
		result = ERR_SELFTEST;
		goto TEST_RESULT;
	}
	result = TRUE_SELFTEST;
TEST_RESULT:
	if(result == TRUE_SELFTEST){
		dev_info(&gyro->client->dev, "gyro self test ok---------\n");
		err = set_gyro_selfTest_result(GYRO, "1");
		if (err) {
			dev_err(&gyro->client->dev, " gyro set self test result  fail\n");
		}
	}else {
	       err = set_gyro_selfTest_result(GYRO, "0");
		if (err) {
			dev_err(&gyro->client->dev, "gyro set self test result  fail\n");
		}
		dev_err(&gyro->client->dev, "gyro self test fail\n");
	}
	l3g4200d_selftest(gyro, 0);
	l3g4200d_device_power_off(gyro);
	return 0;
}

#ifdef DEBUG
static ssize_t attr_reg_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	int rc;
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	u8 x[2];
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
	mutex_lock(&gyro->lock);
	x[0] = gyro->reg_addr;
	mutex_unlock(&gyro->lock);
	x[1] = val;
	rc = l3g4200d_i2c_write(gyro, x, 1);
	if (rc < 0)
		return rc;

	return size;
}

static ssize_t attr_reg_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	ssize_t ret;
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	int rc;
	u8 data;

	mutex_lock(&gyro->lock);
	data = gyro->reg_addr;
	mutex_unlock(&gyro->lock);
	rc = l3g4200d_i2c_read(gyro, &data, 1);
	if (rc < 0)
		return rc;

	ret = sprintf(buf, "0x%02x\n", data);
	return ret;
}

static ssize_t attr_addr_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;

	mutex_lock(&gyro->lock);

	gyro->reg_addr = val;

	mutex_unlock(&gyro->lock);

	return size;
}
#endif /* DEBUG */

static struct device_attribute attributes[] = {
	__ATTR(pollrate_ms, 0664, attr_polling_rate_show,
						attr_polling_rate_store),
	__ATTR(range, 0664, attr_range_show, attr_range_store),
	__ATTR(enable_device, 0664, attr_enable_show, attr_enable_store),
	__ATTR(enable_selftest, 0664, attr_get_selftest, attr_set_selftest),
#ifdef DEBUG
	__ATTR(reg_value, 0600, attr_reg_get, attr_reg_set),
	__ATTR(reg_addr, 0200, NULL, attr_addr_set),
#endif
};

static int create_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto error;
	return 0;

error:
	for ( ; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return -1;
}

static int remove_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);
	return 0;
}
#if 0
static void l3g4200d_input_poll_func(struct input_polled_dev *dev)
{
	struct l3g4200d_data *gyro = dev->private;

	struct l3g4200d_triple data_out;

	int err;

	/* dev_data = container_of((struct delayed_work *)work,
				 struct l3g4200d_data, input_work); */

	mutex_lock(&gyro->lock);
	err = l3g4200d_get_data(gyro, &data_out);
	if (err < 0)
		dev_err(&gyro->client->dev, "get_gyroscope_data failed\n");
	else
		l3g4200d_report_values(gyro, &data_out);

	mutex_unlock(&gyro->lock);

}
#endif

static void l3g4200d_input_work_func(struct work_struct *work)
{
	struct l3g4200d_data *gyro;
	struct l3g4200d_triple data_out;
	int err;
	gyro = container_of((struct work_struct *)work,
				struct l3g4200d_data, work);

	mutex_lock(&gyro->lock);
	err = l3g4200d_get_data(gyro, &data_out);
	if (err < 0)
		dev_err(&gyro->client->dev, "get_gyroscope_data failed\n");
	else
		l3g4200d_report_values(gyro, &data_out);

	/*schedule_delayed_work(&gyro->input_work, msecs_to_jiffies(
            gyro->pdata->poll_interval));*/

	mutex_unlock(&gyro->lock);

}
static enum hrtimer_restart gry_timer_func(struct hrtimer *timer)
{
	//struct lis3dh_acc_data *acc = container_of(timer, struct lis3dh_acc_data, timer);
	struct l3g4200d_data *gyro = container_of(timer, struct l3g4200d_data, timer);
	queue_work(gyro_wq, &gyro->work);
	hrtimer_start(&gyro->timer, ktime_set(0, 10000000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}
#if 0
int l3g4200d_input_open(struct input_dev *input)
{
	struct l3g4200d_data *gyro = input_get_drvdata(input);

	return l3g4200d_enable(gyro);
}

void l3g4200d_input_close(struct input_dev *dev)
{
	struct l3g4200d_data *gyro = input_get_drvdata(dev);

	l3g4200d_disable(gyro);
}
#endif
static int l3g4200d_validate_pdata(struct l3g4200d_data *gyro)
{
	gyro->pdata->poll_interval = max(gyro->pdata->poll_interval,
			gyro->pdata->min_interval);

	if (gyro->pdata->axis_map_x > 2 ||
	    gyro->pdata->axis_map_y > 2 ||
	    gyro->pdata->axis_map_z > 2) {
		dev_err(&gyro->client->dev,
			"invalid axis_map value x:%u y:%u z%u\n",
			gyro->pdata->axis_map_x,
			gyro->pdata->axis_map_y,
			gyro->pdata->axis_map_z);
		return -EINVAL;
	}

	/* Only allow 0 and 1 for negation boolean flag */
	if (gyro->pdata->negate_x > 1 ||
	    gyro->pdata->negate_y > 1 ||
	    gyro->pdata->negate_z > 1) {
		dev_err(&gyro->client->dev,
			"invalid negate value x:%u y:%u z:%u\n",
			gyro->pdata->negate_x,
			gyro->pdata->negate_y,
			gyro->pdata->negate_z);
		return -EINVAL;
	}

	/* Enforce minimum polling interval */
	if (gyro->pdata->poll_interval < gyro->pdata->min_interval) {
		dev_err(&gyro->client->dev,
			"minimum poll interval violated\n");
		return -EINVAL;
	}
	return 0;
}


static int l3g4200d_input_init(struct l3g4200d_data *gyro)
{
	int err = -1;

	dev_dbg(&gyro->client->dev, "l3g4200d_input_init---START---");

	//INIT_DELAYED_WORK(&gyro->input_work,l3g4200d_input_work_func);
	INIT_WORK(&gyro->work, l3g4200d_input_work_func);
	gyro->input_dev = input_allocate_device();
	if (!gyro->input_dev) {
		err = -ENOMEM;
		dev_err(&gyro->client->dev,
			"input device allocate failed\n");
		goto err0;
	}

	/*gyro->input_dev->open = l3g4200d_input_open;*/
	/*gyro->input_dev->close = l3g4200d_input_close;*/
	gyro->input_dev->open = NULL;
	gyro->input_dev->close = NULL;
	gyro->input_dev->name = GYRO_INPUT_DEV_NAME;
	gyro->input_dev->id.bustype = BUS_I2C;
	gyro->input_dev->dev.parent = &gyro->client->dev;

	input_set_drvdata(gyro->input_dev, gyro);

	set_bit(EV_ABS, gyro->input_dev->evbit);

	input_set_abs_params(gyro->input_dev, ABS_X, -FS_MAX, FS_MAX, FUZZ, FLAT);
	input_set_abs_params(gyro->input_dev, ABS_Y, -FS_MAX, FS_MAX, FUZZ, FLAT);
	input_set_abs_params(gyro->input_dev, ABS_Z, -FS_MAX, FS_MAX, FUZZ, FLAT);

	err = input_register_device(gyro->input_dev);
	if (err) {
		dev_err(&gyro->client->dev,
				"unable to register input device %s\n",
				gyro->input_dev->name);
		goto err1;
	}
	hrtimer_init(&gyro->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	gyro->timer.function = gry_timer_func;

	gyro_wq = create_singlethread_workqueue("gyro_wq");

	if (!gyro_wq)
		return -ENOMEM;
#ifdef CONFIG_HUAWEI_SENSORS_INPUT_INFO
	err = set_sensor_input(GYRO, gyro->input_dev->dev.kobj.name);
	if (err) {
		input_unregister_device(gyro->input_dev);
		dev_err(&gyro->client->dev,
				"set_sensor_input %s\n",
				gyro->input_dev->name);
		goto err1;
	}
#endif
	return 0;

err1:
	input_set_drvdata(gyro->input_dev, NULL);
	input_free_device(gyro->input_dev);
err0:
	return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static int l3g4200d_early_suspend(struct early_suspend *h)
{
	struct l3g4200d_data *gyro = container_of(h, struct l3g4200d_data, early_suspend);
	struct i2c_client *client = gyro->client;
	dev_dbg(&client->dev, "l3g4200d_suspend\n");

	/* TO DO */
	gyro->on_before_suspend = atomic_read(&gyro->enabled);

	l3g4200d_disable(gyro);
	//printk("[%s] --------------\n", __func__);
	return 0;

}

static int l3g4200d_later_resume(struct early_suspend *h)
{
	struct l3g4200d_data *gyro = container_of(h, struct l3g4200d_data, early_suspend);
	struct i2c_client *client = gyro->client;
	dev_dbg(&client->dev, "l3g4200d_resume\n");

	/* TO DO */
	if (gyro->on_before_suspend)
		if (l3g4200d_enable(gyro)) {
			dev_err(&client->dev, "%s: l3g4200d_enable failed\n",
				__func__);
		}
	//printk("[%s] ----------------\n", __func__);
	return 0;
}
#endif 

static void l3g4200d_input_cleanup(struct l3g4200d_data *gyro)
{
	input_set_drvdata(gyro->input_dev, NULL);
	input_unregister_device(gyro->input_dev);
	input_free_device(gyro->input_dev);
}

static int l3g4200d_probe(struct i2c_client *client,
					const struct i2c_device_id *devid)
{
	struct l3g4200d_data *gyro;
	int err = -1;
	u8 chipid = 0;
	printk("[%s] +\n", __func__);
	dev_dbg(&client->dev, "%s: probe start.\n", L3G4200D_GYR_DEV_NAME);

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		goto err0;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "client not i2c capable:1\n");
		err = -ENODEV;
		goto err0;
	}

	gyro = kzalloc(sizeof(*gyro), GFP_KERNEL);
	if (gyro == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto err0;
	}
	gyro_temp = gyro;
	mutex_init(&gyro->lock);
	mutex_lock(&gyro->lock);
	gyro->client = client;

	chipid = WHO_AM_I;
	err = l3g4200d_i2c_read(gyro, &chipid, sizeof(chipid));
	if (err < 0) {
		dev_err(&client->dev, "%s hw l3g4200d_i2c_read err= %d\n",
					L3G4200D_GYR_DEV_NAME, err);
		goto err1;
	}

	if (chipid != WHOAMI_L3G4200D) {
		dev_err(&client->dev,
			"device unknown. Expected: 0x%x, Replies: 0x%x\n", WHOAMI_L3G4200D, chipid);
		err = -ENODEV;
		goto err1;
	}

	dev_err(&client->dev, "Read l3g4200d chip ok, ID is 0x%x\n", chipid);

	err = set_sensor_chip_info(GYRO, "ST L3G4200D");
	if (err) {
		dev_err(&client->dev, "set_sensor_chip_info error\n");
	}

	gyro->pdata = kzalloc(sizeof(*gyro->pdata), GFP_KERNEL);
	if (gyro->pdata == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for pdata: %d\n", err);
		err = -ENOMEM;
		goto err1;
	}
	memcpy(gyro->pdata, client->dev.platform_data,
						sizeof(*gyro->pdata));

	err = l3g4200d_validate_pdata(gyro);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto err1_1;
	}

	i2c_set_clientdata(client, gyro);

	if (gyro->pdata->init) {
		err = gyro->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err1_2;
		}
	}

	memset(gyro->resume_state, 0, ARRAY_SIZE(gyro->resume_state));

	gyro->resume_state[GYRO_RES_CTRL_REG1] = 0x07;
	gyro->resume_state[GYRO_RES_CTRL_REG2] = 0x00;
	gyro->resume_state[GYRO_RES_CTRL_REG3] = 0x00;
	gyro->resume_state[GYRO_RES_CTRL_REG4] = 0x00;
	gyro->resume_state[GYRO_RES_CTRL_REG5] = 0x00;

	err = l3g4200d_device_power_on(gyro);
	if (err < 0) {
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err2;
	}

	atomic_set(&gyro->enabled, 1);

	err = l3g4200d_update_fs_range(gyro, gyro->pdata->fs_range);
	if (err < 0) {
		dev_err(&client->dev, "update_fs_range failed\n");
		goto err3;
	}

	err = l3g4200d_update_odr(gyro, gyro->pdata->poll_interval);
	if (err < 0) {
		dev_err(&client->dev, "update_odr failed\n");
		goto err3;
	}

	err = l3g4200d_input_init(gyro);
	if (err < 0)
		goto err3;

	err = create_sysfs_interfaces(&client->dev);
	if (err < 0) {
		dev_err(&client->dev,
			"%s device register failed\n", L3G4200D_GYR_DEV_NAME);
		goto err4;
	}

	l3g4200d_device_power_off(gyro);

	/* As default, do not report information */
	atomic_set(&gyro->enabled, 0);
#ifdef CONFIG_HAS_EARLYSUSPEND
	gyro->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	gyro->early_suspend.suspend = l3g4200d_early_suspend;
	gyro->early_suspend.resume = l3g4200d_later_resume;
	register_early_suspend(&gyro->early_suspend);
#endif
	mutex_unlock(&gyro->lock);

	dev_dbg(&client->dev, "%s probed: device created successfully\n",
							L3G4200D_GYR_DEV_NAME);
	printk("[%s] -\n", __func__);
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    /* detect current device successful, set the flag as present */
        set_hw_dev_flag(DEV_I2C_GYROSCOPE);
#endif

         //l3g4200d_self_test(gyro);
	return 0;

err4:
	l3g4200d_input_cleanup(gyro);
err3:
	l3g4200d_device_power_off(gyro);
err2:
	if (gyro->pdata->exit)
		gyro->pdata->exit();
err1_2:
	i2c_set_clientdata(client, NULL);
err1_1:
	kfree(gyro->pdata);
	gyro->pdata = NULL;
err1:
	mutex_unlock(&gyro->lock);
	kfree(gyro);
	gyro = NULL;
	gyro_temp = NULL;
err0:
	dev_err(&client->dev, "%s: Driver Initialization failed\n",
							L3G4200D_GYR_DEV_NAME);
	return err;
}

static int l3g4200d_remove(struct i2c_client *client)
{
	struct l3g4200d_data *gyro = i2c_get_clientdata(client);
	printk("[%s] +\n", __func__);
	dev_dbg(&client->dev, "L3G4200D driver removing\n");

	l3g4200d_input_cleanup(gyro);
	l3g4200d_disable(gyro);
	remove_sysfs_interfaces(&client->dev);

	if (gyro->pdata->exit)
		gyro->pdata->exit();
	hrtimer_cancel(&gyro->timer);
	if(gyro_wq)
		destroy_workqueue(gyro_wq);
	kfree(gyro->pdata);
	gyro->pdata = NULL;
	kfree(gyro);
	gyro = NULL;

	i2c_set_clientdata(client, NULL);
	printk("[%s] -\n", __func__);
	return 0;
}

static void l3g4200d_shutdown(struct i2c_client *client)
{
	struct l3g4200d_data *gyro = i2c_get_clientdata(client);

	printk("[%s] +\n", __func__);

	l3g4200d_disable(gyro);

	if (gyro->pdata->exit)
		gyro->pdata->exit();

	printk("[%s],-\n", __func__);

}

static const struct i2c_device_id l3g4200d_id[] = {
	{ L3G4200D_GYR_DEV_NAME , 0 },
	{},
};

MODULE_DEVICE_TABLE(i2c, l3g4200d_id);

static struct i2c_driver l3g4200d_driver = {
	.driver = {
			.owner = THIS_MODULE,
			.name = L3G4200D_GYR_DEV_NAME,
	},
	.probe = l3g4200d_probe,
	.remove = __devexit_p(l3g4200d_remove),
	.shutdown = l3g4200d_shutdown,
	.id_table = l3g4200d_id,
};

static int __init l3g4200d_init(void)
{
	return i2c_add_driver(&l3g4200d_driver);
}

static void __exit l3g4200d_exit(void)
{
	i2c_del_driver(&l3g4200d_driver);
	return;
}

module_init(l3g4200d_init);
module_exit(l3g4200d_exit);

MODULE_DESCRIPTION("l3g4200d digital gyroscope sysfs driver");
MODULE_AUTHOR("Matteo Dameno, Carmine Iascone, STMicroelectronics");
MODULE_LICENSE("GPL");
