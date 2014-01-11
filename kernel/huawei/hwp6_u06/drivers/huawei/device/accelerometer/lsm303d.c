/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
*
* File Name          : lsm303d.h
* Authors            : MSH - C&I BU - Application Team
*		     : Matteo Dameno (matteo.dameno@st.com)
*		     : Denis Ciocca (denis.ciocca@st.com)
*		     : Both authors are willing to be considered the contact
*		     : and update points for the driver.
* Version            : V.2.0.0
* Date               : 2012/May/09
* Description        : LSM303D accelerometer sensor API
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
********************************************************************************
Version History.

Revision 2-0-0 2012/05/09
 first revision
*******************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/kernel.h>

#include "lsm303d.h"

#define	I2C_AUTO_INCREMENT	(0x80)

#define	ACC_G_MAX_POS		1495040	/** max positive value acc [ug] */
#define	ACC_G_MAX_NEG		1495770	/** max negative value acc [ug] */
#define	MAG_G_MAX_POS		983520	/** max positive value mag [ugauss] */
#define	MAG_G_MAX_NEG		983040	/** max negative value mag [ugauss] */

#define FUZZ			0
#define FLAT			0

/* Address registers */
#define REG_WHOAMI_ADDR		(0x0F)
#define REG_CNTRL0_ADDR		(0x1F)
#define REG_CNTRL1_ADDR		(0x20)
#define REG_CNTRL2_ADDR		(0x21)
#define REG_CNTRL3_ADDR		(0x22)
#define REG_CNTRL4_ADDR		(0x23)
#define REG_CNTRL5_ADDR		(0x24)
#define REG_CNTRL6_ADDR		(0x25)
#define REG_CNTRL7_ADDR		(0x26)

#define REG_ACC_DATA_ADDR	(0x28)
#define REG_MAG_DATA_ADDR	(0x08)

/* Sensitivity */
#define SENSITIVITY_ACC_2G	60	/**	ug/LSB	*/
#define SENSITIVITY_ACC_4G	120	/**	ug/LSB	*/
#define SENSITIVITY_ACC_8G	240	/**	ug/LSB	*/
#define SENSITIVITY_ACC_16G	730	/**	ug/LSB	*/

#define SENSITIVITY_MAG_2G	80	/**	ugauss/LSB	*/
#define SENSITIVITY_MAG_4G	160	/**	ugauss/LSB	*/
#define SENSITIVITY_MAG_8G	320	/**	ugauss/LSB	*/
#define SENSITIVITY_MAG_12G	480	/**	ugauss/LSB	*/

/* ODR */
#define ODR_ACC_MASK		(0XF0)	/* Mask for odr change on acc */
#define LSM303D_ACC_ODR_OFF	(0x00)  /* Power down */
#define LSM303D_ACC_ODR3_125	(0x10)  /* 3.25Hz output data rate */
#define LSM303D_ACC_ODR6_25	(0x20)  /* 6.25Hz output data rate */
#define LSM303D_ACC_ODR12_5	(0x30)  /* 12.5Hz output data rate */
#define LSM303D_ACC_ODR25	(0x40)  /* 25Hz output data rate */
#define LSM303D_ACC_ODR50	(0x50)  /* 50Hz output data rate */
#define LSM303D_ACC_ODR100	(0x60)  /* 100Hz output data rate */
#define LSM303D_ACC_ODR200	(0x70)  /* 200Hz output data rate */
#define LSM303D_ACC_ODR400	(0x80)  /* 400Hz output data rate */
#define LSM303D_ACC_ODR800	(0x90)  /* 800Hz output data rate */
#define LSM303D_ACC_ODR1600	(0xA0)  /* 1600Hz output data rate */

#define ODR_MAG_MASK		(0X1C)	/* Mask for odr change on mag */
#define LSM303D_MAG_ODR3_125	(0x00)  /* 3.25Hz output data rate */
#define LSM303D_MAG_ODR6_25	(0x04)  /* 6.25Hz output data rate */
#define LSM303D_MAG_ODR12_5	(0x08)  /* 12.5Hz output data rate */
#define LSM303D_MAG_ODR25	(0x0C)  /* 25Hz output data rate */
#define LSM303D_MAG_ODR50	(0x10)  /* 50Hz output data rate */
#define LSM303D_MAG_ODR100	(0x14)  /* 100Hz output data rate */

/* Magnetic sensor mode */
#define MSMS_MASK		(0x03)	/* Mask magnetic sensor mode */
#define POWEROFF_MAG		(0x02)	/* Power Down */
#define CONTINUOS_CONVERSION	(0x00)

/* Default values loaded in probe function */
#define WHOIAM_VALUE		(0x49)
#define REG_DEF_CNTRL0		(0x00)
#define REG_DEF_CNTRL1		(0x07)
#define REG_DEF_CNTRL2		(0x00)
#define REG_DEF_CNTRL3		(0x00)
#define REG_DEF_CNTRL4		(0x00)
#define REG_DEF_CNTRL5		(0x18)
#define REG_DEF_CNTRL6		(0x20)
#define REG_DEF_CNTRL7		(0x02)


struct {
	unsigned int cutoff_us;
	u8 value;
} lsm303d_acc_odr_table[] = {
		{    625, LSM303D_ACC_ODR1600 },
		{   1250, LSM303D_ACC_ODR800  },
		{   2500, LSM303D_ACC_ODR400  },
		{   5000, LSM303D_ACC_ODR200  },
		{  10000, LSM303D_ACC_ODR100  },
		{  20000, LSM303D_ACC_ODR50   },
		{  40000, LSM303D_ACC_ODR25   },
		{  80000, LSM303D_ACC_ODR12_5 },
		{ 160000, LSM303D_ACC_ODR6_25 },
		{ 320000, LSM303D_ACC_ODR3_125},
};

struct {
	unsigned int cutoff_us;
	u8 value;
} lsm303d_mag_odr_table[] = {
		{  10000, LSM303D_MAG_ODR100  },
		{  20000, LSM303D_MAG_ODR50   },
		{  40000, LSM303D_MAG_ODR25   },
		{  80000, LSM303D_MAG_ODR12_5 },
		{ 160000, LSM303D_MAG_ODR6_25 },
		{ 320000, LSM303D_MAG_ODR3_125},
};

struct lsm303d_status {
	struct i2c_client *client;
	struct lsm303d_acc_platform_data *pdata_acc;
	struct lsm303d_acc_platform_data *pdata_mag;

	struct mutex lock;
	struct delayed_work input_work_acc;
	struct delayed_work input_work_mag;

	struct input_dev *input_dev_acc;
	struct input_dev *input_dev_mag;

	int hw_initialized;

	/* hw_working=-1 means not tested yet */
	int hw_working;

	atomic_t enabled_acc;
	atomic_t enabled_mag;

	int on_before_suspend;
	int use_smbus;

	u16 sensitivity_acc;
	u16 sensitivity_mag;
};

struct reg_rw {
	u8 address;
	u8 default_value;
	u8 resume_value;
};

struct reg_r {
	u8 address;
	u8 value;
};

static struct status_registers {
	struct reg_r who_am_i;
	struct reg_rw cntrl0;
	struct reg_rw cntrl1;
	struct reg_rw cntrl2;
	struct reg_rw cntrl3;
	struct reg_rw cntrl4;
	struct reg_rw cntrl5;
	struct reg_rw cntrl6;
	struct reg_rw cntrl7;
} status_registers = {
	.who_am_i.address=REG_WHOAMI_ADDR, .who_am_i.value=WHOIAM_VALUE,
	.cntrl0.address=REG_CNTRL0_ADDR, .cntrl0.default_value=REG_DEF_CNTRL0,
	.cntrl1.address=REG_CNTRL1_ADDR, .cntrl1.default_value=REG_DEF_CNTRL1,
	.cntrl2.address=REG_CNTRL2_ADDR, .cntrl2.default_value=REG_DEF_CNTRL2,
	.cntrl3.address=REG_CNTRL3_ADDR, .cntrl3.default_value=REG_DEF_CNTRL3,
	.cntrl4.address=REG_CNTRL4_ADDR, .cntrl4.default_value=REG_DEF_CNTRL4,
	.cntrl5.address=REG_CNTRL5_ADDR, .cntrl5.default_value=REG_DEF_CNTRL5,
	.cntrl6.address=REG_CNTRL6_ADDR, .cntrl6.default_value=REG_DEF_CNTRL6,
	.cntrl7.address=REG_CNTRL7_ADDR, .cntrl7.default_value=REG_DEF_CNTRL7,
};

static int lsm303d_i2c_read(struct lsm303d_status *stat, u8 *buf, int len)
{
	int ret;
	u8 reg = buf[0];
	u8 cmd = reg;
#ifdef DEBUG
	unsigned int ii;
#endif


	if (len > 1)
		cmd = (I2C_AUTO_INCREMENT | reg);
	if (stat->use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_read_byte_data(stat->client, cmd);
			buf[0] = ret & 0xff;
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_read_byte_data: ret=0x%02x, len:%d ,"
				"command=0x%02x, buf[0]=0x%02x\n",
				ret, len, cmd , buf[0]);
#endif
		} else if (len > 1) {
			ret = i2c_smbus_read_i2c_block_data(stat->client,
								cmd, len, buf);
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_read_i2c_block_data: ret:%d len:%d, "
				"command=0x%02x, ",
				ret, len, cmd);
			for (ii = 0; ii < len; ii++)
				printk(KERN_DEBUG "buf[%d]=0x%02x,",
								ii, buf[ii]);

			printk("\n");
#endif
		} else
			ret = -1;

		if (ret < 0) {
			dev_err(&stat->client->dev,
				"read transfer error: len:%d, command=0x%02x\n",
				len, cmd);
			return 0;
		}
		return len;
	}

	ret = i2c_master_send(stat->client, &cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
		return ret;

	return i2c_master_recv(stat->client, buf, len);
}

static int lsm303d_i2c_write(struct lsm303d_status *stat, u8 *buf, int len)
{
	int ret;
	u8 reg, value;
#ifdef DEBUG
	unsigned int ii;
#endif

	if (len > 1)
		buf[0] = (I2C_AUTO_INCREMENT | buf[0]);

	reg = buf[0];
	value = buf[1];

	if (stat->use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_write_byte_data(stat->client,
								reg, value);
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_write_byte_data: ret=%d, len:%d, "
				"command=0x%02x, value=0x%02x\n",
				ret, len, reg , value);
#endif
			return ret;
		} else if (len > 1) {
			ret = i2c_smbus_write_i2c_block_data(stat->client,
							reg, len, buf + 1);
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_write_i2c_block_data: ret=%d, "
				"len:%d, command=0x%02x, ",
				ret, len, reg);
			for (ii = 0; ii < (len + 1); ii++)
				printk(KERN_DEBUG "value[%d]=0x%02x,",
								ii, buf[ii]);

			printk("\n");
#endif
			return ret;
		}
	}

	ret = i2c_master_send(stat->client, buf, len+1);
	return (ret == len+1) ? 0 : ret;
}

static int lsm303d_hw_init(struct lsm303d_status *stat)
{
	int err = -1;
	u8 buf[1];

	pr_info("%s: hw init start\n", LSM303D_DEV_NAME);

	buf[0] = status_registers.who_am_i.address;
	err = lsm303d_i2c_read(stat, buf, 1);

	if (err < 0) {
		dev_warn(&stat->client->dev, "Error reading WHO_AM_I: is device"
		" available/working?\n");
		goto err_firstread;
	} else
		stat->hw_working = 1;

	if (buf[0] != status_registers.who_am_i.value) {
	dev_err(&stat->client->dev,
		"device unknown. Expected: 0x%02x,"
		" Replies: 0x%02x\n", status_registers.who_am_i.value, buf[0]);
		err = -1;
		goto err_unknown_device;
	}

	status_registers.cntrl1.resume_value =
					status_registers.cntrl1.default_value;
	status_registers.cntrl2.resume_value =
					status_registers.cntrl2.default_value;
	status_registers.cntrl3.resume_value =
					status_registers.cntrl3.default_value;
	status_registers.cntrl4.resume_value =
					status_registers.cntrl4.default_value;
	status_registers.cntrl5.resume_value =
					status_registers.cntrl5.default_value;
	status_registers.cntrl6.resume_value =
					status_registers.cntrl6.default_value;
	status_registers.cntrl7.resume_value =
					status_registers.cntrl7.default_value;

	stat->hw_initialized = 1;
	pr_info("%s: hw init done\n", LSM303D_DEV_NAME);

	return 0;

err_unknown_device:
err_firstread:
	stat->hw_working = 0;
	stat->hw_initialized = 0;
	return err;
}

static int lsm303d_acc_device_power_off(struct lsm303d_status *stat)
{
	int err;
	u8 buf[2];

	buf[0] = status_registers.cntrl1.address;
	buf[1] = ((ODR_ACC_MASK & LSM303D_ACC_ODR_OFF) |
		((~ODR_ACC_MASK) & status_registers.cntrl1.resume_value));

	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		dev_err(&stat->client->dev, "accelerometer soft power off "
							"failed: %d\n", err);

	if (stat->pdata_acc->power_off) {
		stat->pdata_acc->power_off();
	}

	atomic_set(&stat->enabled_acc, 0);

	return 0;
}

static int lsm303d_mag_device_power_off(struct lsm303d_status *stat)
{
	int err;
	u8 buf[2];

	buf[0] = status_registers.cntrl7.address;
	buf[1] = ((MSMS_MASK & POWEROFF_MAG) |
		((~MSMS_MASK) & status_registers.cntrl7.resume_value));

	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		dev_err(&stat->client->dev, "magnetometer soft power off "
							"failed: %d\n", err);

	if (stat->pdata_mag->power_off) {
		stat->pdata_mag->power_off();
	}

	atomic_set(&stat->enabled_mag, 0);

	return 0;
}

static int lsm303d_acc_device_power_on(struct lsm303d_status *stat)
{
	int err = -1;
	u8 buf[5];

	if (stat->pdata_acc->power_on) {
		err = stat->pdata_acc->power_on();
		if (err < 0) {
			dev_err(&stat->client->dev,
				"accelerometer power_on failed: %d\n", err);
			return err;
		}
	}

	buf[0] = status_registers.cntrl0.address;
	buf[1] = status_registers.cntrl0.resume_value;
	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		goto err_resume_state;

	buf[0] = status_registers.cntrl1.address;
	buf[1] = status_registers.cntrl1.resume_value;
	buf[2] = status_registers.cntrl2.resume_value;
	buf[3] = status_registers.cntrl3.resume_value;
	buf[4] = status_registers.cntrl4.resume_value;
	err = lsm303d_i2c_write(stat, buf, 4);
	if (err < 0)
		goto err_resume_state;

	buf[0] = status_registers.cntrl7.address;
	buf[1] = status_registers.cntrl7.resume_value;
	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		goto err_resume_state;

	atomic_set(&stat->enabled_acc, 1);

	return 0;

err_resume_state:
	atomic_set(&stat->enabled_acc, 0);
	dev_err(&stat->client->dev, "accelerometer hw power on error "
				"0x%02x,0x%02x: %d\n", buf[0], buf[1], err);
	return err;
}

static int lsm303d_mag_device_power_on(struct lsm303d_status *stat)
{
	int err = -1;
	u8 buf[6];

	if (stat->pdata_mag->power_on) {
		err = stat->pdata_mag->power_on();
		if (err < 0) {
			dev_err(&stat->client->dev,
				"magnetometer power_on failed: %d\n", err);
			return err;
		}
	}

	buf[0] = status_registers.cntrl0.address;
	buf[1] = status_registers.cntrl0.resume_value;
	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		goto err_resume_state;

	buf[0] = status_registers.cntrl3.address;
	buf[1] = status_registers.cntrl3.resume_value;
	buf[2] = status_registers.cntrl4.resume_value;
	buf[3] = status_registers.cntrl5.resume_value;
	buf[4] = status_registers.cntrl6.resume_value;
	buf[5] = ((MSMS_MASK & CONTINUOS_CONVERSION) |
		((~MSMS_MASK) & status_registers.cntrl7.resume_value));


	err = lsm303d_i2c_write(stat, buf, 5);
	if (err < 0)
		goto err_resume_state;

	atomic_set(&stat->enabled_mag, 1);

	return 0;

err_resume_state:
	atomic_set(&stat->enabled_mag, 0);
	dev_err(&stat->client->dev, "magnetometer hw power on error "
				"0x%02x,0x%02x: %d\n", buf[0], buf[1], err);
	return err;
}

static int lsm303d_acc_update_fs_range(struct lsm303d_status *stat,
								u8 new_fs_range)
{
	int err=-1;

	u16 sensitivity;
	u8 updated_val;
	u8 buf[2];

	switch (new_fs_range) {
	case LSM303D_ACC_FS_2G:
		sensitivity = SENSITIVITY_ACC_2G;
		break;
	case LSM303D_ACC_FS_4G:
		sensitivity = SENSITIVITY_ACC_4G;
		break;
	case LSM303D_ACC_FS_8G:
		sensitivity = SENSITIVITY_ACC_8G;
		break;
	case LSM303D_ACC_FS_16G:
		sensitivity = SENSITIVITY_ACC_16G;
		break;
	default:
		dev_err(&stat->client->dev, "invalid accelerometer "
				"fs range requested: %u\n", new_fs_range);
		return -EINVAL;
	}

	buf[0] = status_registers.cntrl2.address;
	err = lsm303d_i2c_read(stat, buf, 1);
	if (err < 0)
		goto error;

	status_registers.cntrl2.resume_value = buf[0];
	updated_val = ((LSM303D_ACC_FS_MASK & new_fs_range) |
					((~LSM303D_ACC_FS_MASK) & buf[0]));
	buf[1] = updated_val;
	buf[0] = status_registers.cntrl2.address;

	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		goto error;
	status_registers.cntrl2.resume_value = updated_val;
	stat->sensitivity_acc = sensitivity;

	return err;

error:
	dev_err(&stat->client->dev, "update accelerometer fs range failed "
		"0x%02x,0x%02x: %d\n", buf[0], buf[1], err);
	return err;
}

static int lsm303d_mag_update_fs_range(struct lsm303d_status *stat,
								u8 new_fs_range)
{
	int err=-1;

	u16 sensitivity;
	u8 updated_val;
	u8 buf[2];

	switch (new_fs_range) {
	case LSM303D_MAG_FS_2G:
		sensitivity = SENSITIVITY_MAG_2G;
		break;
	case LSM303D_MAG_FS_4G:
		sensitivity = SENSITIVITY_MAG_4G;
		break;
	case LSM303D_MAG_FS_8G:
		sensitivity = SENSITIVITY_MAG_8G;
		break;
	case LSM303D_MAG_FS_12G:
		sensitivity = SENSITIVITY_MAG_12G;
		break;
	default:
		dev_err(&stat->client->dev, "invalid magnetometer "
				"fs range requested: %u\n", new_fs_range);
		return -EINVAL;
	}

	buf[0] = status_registers.cntrl6.address;
	err = lsm303d_i2c_read(stat, buf, 1);
	if (err < 0)
		goto error;

	status_registers.cntrl6.resume_value = buf[0];
	updated_val = (LSM303D_MAG_FS_MASK & new_fs_range);
	buf[1] = updated_val;
	buf[0] = status_registers.cntrl6.address;

	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		goto error;
	status_registers.cntrl6.resume_value = updated_val;
	stat->sensitivity_mag = sensitivity;

	return err;

error:
	dev_err(&stat->client->dev, "update magnetometer fs range failed "
		"0x%02x,0x%02x: %d\n", buf[0], buf[1], err);
	return err;
}

static int lsm303d_acc_update_odr(struct lsm303d_status *stat,
						unsigned int poll_interval_us)
{
	int err = -1;
	u8 config[2];
	int i;

	for (i = ARRAY_SIZE(lsm303d_acc_odr_table) - 1; i >= 0; i--) {
		if ((lsm303d_acc_odr_table[i].cutoff_us <= poll_interval_us)
								|| (i == 0))
			break;
	}

	config[1] = ((ODR_ACC_MASK & lsm303d_acc_odr_table[i].value) |
		((~ODR_ACC_MASK) & status_registers.cntrl1.resume_value));

	if (atomic_read(&stat->enabled_acc)) {
		config[0] = status_registers.cntrl1.address;
		err = lsm303d_i2c_write(stat, config, 1);
		if (err < 0)
			goto error;
		status_registers.cntrl1.resume_value = config[1];
	}

	return err;

error:
	dev_err(&stat->client->dev, "update accelerometer odr failed "
			"0x%02x,0x%02x: %d\n", config[0], config[1], err);

	return err;
}

static int lsm303d_mag_update_odr(struct lsm303d_status *stat,
						unsigned int poll_interval_us)
{
	int err = -1;
	u8 config[2];
	int i;

	for (i = ARRAY_SIZE(lsm303d_mag_odr_table) - 1; i >= 0; i--) {
		if ((lsm303d_mag_odr_table[i].cutoff_us <= poll_interval_us)
								|| (i == 0))
			break;
	}

	config[1] = ((ODR_MAG_MASK & lsm303d_mag_odr_table[i].value) |
		((~ODR_MAG_MASK) & status_registers.cntrl5.resume_value));

	if (atomic_read(&stat->enabled_mag)) {
		config[0] = status_registers.cntrl5.address;
		err = lsm303d_i2c_write(stat, config, 1);
		if (err < 0)
			goto error;
		status_registers.cntrl5.resume_value = config[1];
	}

	return err;

error:
	dev_err(&stat->client->dev, "update magnetometer odr failed "
			"0x%02x,0x%02x: %d\n", config[0], config[1], err);

	return err;
}

static int lsm303d_validate_polling(unsigned int *min_interval,
					unsigned int *poll_interval,
					unsigned int min, u8 *axis_map_x,
					u8 *axis_map_y, u8 *axis_map_z,
					struct i2c_client *client)
{
	*min_interval = max(min, *min_interval);
	*poll_interval = max(*poll_interval, *min_interval);

	if (*axis_map_x > 2 || *axis_map_y > 2 || *axis_map_z > 2) {
		dev_err(&client->dev, "invalid axis_map value "
		"x:%u y:%u z%u\n", *axis_map_x, *axis_map_y, *axis_map_z);
		return -EINVAL;
	}

	return 0;
}

static int lsm303d_validate_negate(u8 *negate_x, u8 *negate_y, u8 *negate_z,
						struct i2c_client *client)
{
	if (*negate_x > 1 || *negate_y > 1 || *negate_z > 1) {
		dev_err(&client->dev, "invalid negate value "
			"x:%u y:%u z:%u\n", *negate_x, *negate_y, *negate_z);
		return -EINVAL;
	}
	return 0;
}

static int lsm303d_acc_validate_pdata(struct lsm303d_status *stat)
{
	int res=-1;
	res=lsm303d_validate_polling(&stat->pdata_acc->min_interval,
				&stat->pdata_acc->poll_interval,
				(unsigned int)LSM303D_ACC_MIN_POLL_PERIOD_US,
				&stat->pdata_acc->axis_map_x,
				&stat->pdata_acc->axis_map_y,
				&stat->pdata_acc->axis_map_z,
				stat->client);
	if(res<0)
		return -EINVAL;

	res=lsm303d_validate_negate(&stat->pdata_acc->negate_x,
				&stat->pdata_acc->negate_y,
				&stat->pdata_acc->negate_z,
				stat->client);
	if(res<0)
		return -EINVAL;

	return 0;
}

static int lsm303d_mag_validate_pdata(struct lsm303d_status *stat)
{
	int res=-1;
	res=lsm303d_validate_polling(&stat->pdata_mag->min_interval,
				&stat->pdata_mag->poll_interval,
				(unsigned int)LSM303D_MAG_MIN_POLL_PERIOD_US,
				&stat->pdata_mag->axis_map_x,
				&stat->pdata_mag->axis_map_y,
				&stat->pdata_mag->axis_map_z,
				stat->client);
	if(res<0)
		return -EINVAL;

	res=lsm303d_validate_negate(&stat->pdata_mag->negate_x,
				&stat->pdata_mag->negate_y,
				&stat->pdata_mag->negate_z,
				stat->client);
	if(res<0)
		return -EINVAL;

	return 0;
}

static int lsm303d_acc_enable(struct lsm303d_status *stat)
{
	int err;
	if (!atomic_cmpxchg(&stat->enabled_acc, 0, 1)) {
		err = lsm303d_acc_device_power_on(stat);
		if (err < 0) {
			atomic_set(&stat->enabled_acc, 0);
			return err;
		}
		schedule_delayed_work(&stat->input_work_acc,
			usecs_to_jiffies(stat->pdata_acc->poll_interval));
	}

	return 0;
}

static int lsm303d_acc_disable(struct lsm303d_status *stat)
{
	if (atomic_cmpxchg(&stat->enabled_acc, 1, 0)) {
		cancel_delayed_work_sync(&stat->input_work_acc);
		lsm303d_acc_device_power_off(stat);
	}

	return 0;
}

static int lsm303d_mag_enable(struct lsm303d_status *stat)
{
	int err;

	if (!atomic_cmpxchg(&stat->enabled_mag, 0, 1)) {
		err = lsm303d_mag_device_power_on(stat);
		if (err < 0) {
			atomic_set(&stat->enabled_mag, 0);
			return err;
		}
		schedule_delayed_work(&stat->input_work_mag,
			usecs_to_jiffies(stat->pdata_mag->poll_interval));
	}

	return 0;
}

static int lsm303d_mag_disable(struct lsm303d_status *stat)
{
	if (atomic_cmpxchg(&stat->enabled_mag, 1, 0)) {
		cancel_delayed_work_sync(&stat->input_work_mag);
		lsm303d_mag_device_power_off(stat);
	}

	return 0;
}

static void lsm303d_acc_input_cleanup(struct lsm303d_status *stat)
{
	input_unregister_device(stat->input_dev_acc);
	input_free_device(stat->input_dev_acc);
}

static void lsm303d_mag_input_cleanup(struct lsm303d_status *stat)
{
	input_unregister_device(stat->input_dev_mag);
	input_free_device(stat->input_dev_mag);
}

static ssize_t attr_get_polling_rate_acc(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	unsigned int val;
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	mutex_lock(&stat->lock);
	val = stat->pdata_acc->poll_interval;
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%u\n", val);
}

static ssize_t attr_get_polling_rate_mag(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	unsigned int val;
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	mutex_lock(&stat->lock);
	val = stat->pdata_mag->poll_interval;
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%u\n", val);
}

static ssize_t attr_set_polling_rate_acc(struct device *dev,
				     	struct device_attribute *attr,
				     	const char *buf, size_t size)
{
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long interval_us;

	if (strict_strtoul(buf, 10, &interval_us))
		return -EINVAL;
	if (!interval_us)
		return -EINVAL;
	interval_us = (unsigned int)max((unsigned int)interval_us,
						stat->pdata_acc->min_interval);
	mutex_lock(&stat->lock);
	stat->pdata_acc->poll_interval = (unsigned int)interval_us;
	lsm303d_acc_update_odr(stat, interval_us);
	mutex_unlock(&stat->lock);
	return size;
}

static ssize_t attr_set_polling_rate_mag(struct device *dev,
				     	struct device_attribute *attr,
				     	const char *buf, size_t size)
{
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long interval_us;

	if (strict_strtoul(buf, 10, &interval_us))
		return -EINVAL;
	if (!interval_us)
		return -EINVAL;
	interval_us = (unsigned int)max((unsigned int)interval_us,
						stat->pdata_mag->min_interval);
	mutex_lock(&stat->lock);
	stat->pdata_mag->poll_interval = (unsigned int)interval_us;
	lsm303d_mag_update_odr(stat, interval_us);
	mutex_unlock(&stat->lock);
	return size;
}

static ssize_t attr_get_enable_acc(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	int val = (int)atomic_read(&stat->enabled_acc);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_get_enable_mag(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	int val = (int)atomic_read(&stat->enabled_mag);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_set_enable_acc(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	if (val)
		lsm303d_acc_enable(stat);
	else
		lsm303d_acc_disable(stat);

	return size;
}

static ssize_t attr_set_enable_mag(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	if (val)
		lsm303d_mag_enable(stat);
	else
		lsm303d_mag_disable(stat);

	return size;
}

static ssize_t attr_get_range_acc(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	u8 val;
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	int range = 2;
	mutex_lock(&stat->lock);
	val = stat->pdata_acc->fs_range ;
	switch (val) {
	case LSM303D_ACC_FS_2G:
		range = 2;
		break;
	case LSM303D_ACC_FS_4G:
		range = 4;
		break;
	case LSM303D_ACC_FS_8G:
		range = 8;
		break;
	case LSM303D_ACC_FS_16G:
		range = 16;
		break;
	}
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%d\n", range);
}

static ssize_t attr_get_range_mag(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	u8 val;
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	int range = 2;
	mutex_lock(&stat->lock);
	val = stat->pdata_mag->fs_range ;
	switch (val) {
	case LSM303D_MAG_FS_2G:
		range = 2;
		break;
	case LSM303D_MAG_FS_4G:
		range = 4;
		break;
	case LSM303D_MAG_FS_8G:
		range = 8;
		break;
	case LSM303D_MAG_FS_12G:
		range = 12;
		break;
	}
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%d\n", range);
}

static ssize_t attr_set_range_acc(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long val;
	u8 range;
	int err;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	switch (val) {
	case 2:
		range = LSM303D_ACC_FS_2G;
		break;
	case 4:
		range = LSM303D_ACC_FS_4G;
		break;
	case 8:
		range = LSM303D_ACC_FS_8G;
		break;
	case 16:
		range = LSM303D_ACC_FS_16G;
		break;
	default:
		dev_err(&stat->client->dev, "accelerometer invalid range "
					"request: %lu, discarded\n", val);
		return -EINVAL;
	}
	mutex_lock(&stat->lock);
	err = lsm303d_acc_update_fs_range(stat, range);
	if (err < 0) {
		mutex_unlock(&stat->lock);
		return err;
	}
	stat->pdata_acc->fs_range = range;
	mutex_unlock(&stat->lock);
	dev_info(&stat->client->dev, "accelerometer range set to:"
							" %lu g\n", val);

	return size;
}

static ssize_t attr_set_range_mag(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long val;
	u8 range;
	int err;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	switch (val) {
	case 2:
		range = LSM303D_MAG_FS_2G;
		break;
	case 4:
		range = LSM303D_MAG_FS_4G;
		break;
	case 8:
		range = LSM303D_MAG_FS_8G;
		break;
	case 16:
		range = LSM303D_MAG_FS_12G;
		break;
	default:
		dev_err(&stat->client->dev, "magnetometer invalid range "
					"request: %lu, discarded\n", val);
		return -EINVAL;
	}
	mutex_lock(&stat->lock);
	err = lsm303d_mag_update_fs_range(stat, range);
	if (err < 0) {
		mutex_unlock(&stat->lock);
		return err;
	}
	stat->pdata_mag->fs_range = range;
	mutex_unlock(&stat->lock);
	dev_info(&stat->client->dev, "magnetometer range set to:"
							" %lu g\n", val);

	return size;
}

static struct device_attribute attributes[] = {
	__ATTR(pollrate_acc_us, 0664, attr_get_polling_rate_acc,
						attr_set_polling_rate_acc),
	__ATTR(pollrate_mag_us, 0664, attr_get_polling_rate_mag,
						attr_set_polling_rate_mag),
	__ATTR(enable_device_acc, 0664, attr_get_enable_acc,
						attr_set_enable_acc),
	__ATTR(enable_device_mag, 0664, attr_get_enable_mag,
						attr_set_enable_mag),
	__ATTR(range_acc, 0664, attr_get_range_acc, attr_set_range_acc),
	__ATTR(range_mag, 0664, attr_get_range_mag, attr_set_range_mag),
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

int lsm303d_acc_input_open(struct input_dev *input)
{
	struct lsm303d_status *stat = input_get_drvdata(input);

	return lsm303d_acc_enable(stat);
}

void lsm303d_acc_input_close(struct input_dev *dev)
{
	struct lsm303d_status *stat = input_get_drvdata(dev);

	lsm303d_acc_disable(stat);
}

int lsm303d_mag_input_open(struct input_dev *input)
{
	struct lsm303d_status *stat = input_get_drvdata(input);

	return lsm303d_mag_enable(stat);
}

void lsm303d_mag_input_close(struct input_dev *dev)
{
	struct lsm303d_status *stat = input_get_drvdata(dev);

	lsm303d_mag_disable(stat);
}

static int lsm303d_acc_get_data(struct lsm303d_status *stat, int *xyz)
{
	int err = -1;
	u8 acc_data[6];
	s32 hw_d[3] = { 0 };

	acc_data[0] = (REG_ACC_DATA_ADDR);
	err = lsm303d_i2c_read(stat, acc_data, 6);
	if (err < 0)
		return err;

	hw_d[0] = ((s32)( (s16)((acc_data[1] << 8) | (acc_data[0]))));
	hw_d[1] = ((s32)( (s16)((acc_data[3] << 8) | (acc_data[2]))));
	hw_d[2] = ((s32)( (s16)((acc_data[4] << 8) | (acc_data[4]))));

#ifdef DEBUG
	pr_debug("%s read x=%X %X(regH regL), x=%d(dec) [ug]\n",
		LSM303D_ACC_DEV_NAME, acc_data[1], acc_data[0], hw_d[0]);
	pr_debug("%s read x=%X %X(regH regL), x=%d(dec) [ug]\n",
		LSM303D_ACC_DEV_NAME, acc_data[3], acc_data[0], hw_d[2]);
	pr_debug("%s read x=%X %X(regH regL), x=%d(dec) [ug]\n",
		LSM303D_ACC_DEV_NAME, acc_data[5], acc_data[0], hw_d[4]);
#endif

	hw_d[0] = hw_d[0] * stat->sensitivity_acc;
	hw_d[1] = hw_d[1] * stat->sensitivity_acc;
	hw_d[2] = hw_d[2] * stat->sensitivity_acc;

	xyz[0] = ((stat->pdata_acc->negate_x) ?
				(-hw_d[stat->pdata_acc->axis_map_x])
		   			: (hw_d[stat->pdata_acc->axis_map_x]));
	xyz[1] = ((stat->pdata_acc->negate_y) ?
				(-hw_d[stat->pdata_acc->axis_map_y])
		   			: (hw_d[stat->pdata_acc->axis_map_y]));
	xyz[2] = ((stat->pdata_acc->negate_z) ?
				(-hw_d[stat->pdata_acc->axis_map_z])
		   			: (hw_d[stat->pdata_acc->axis_map_z]));

	return err;
}

static int lsm303d_mag_get_data(struct lsm303d_status *stat, int *xyz)
{
	int err = -1;
	u8 mag_data[6];
	s32 hw_d[3] = { 0 };

	mag_data[0] = (REG_MAG_DATA_ADDR);
	err = lsm303d_i2c_read(stat, mag_data, 6);
	if (err < 0)
		return err;

	hw_d[0] = ((s32)( (s16)((mag_data[1] << 8) | (mag_data[0]))));
	hw_d[1] = ((s32)( (s16)((mag_data[3] << 8) | (mag_data[2]))));
	hw_d[2] = ((s32)( (s16)((mag_data[4] << 8) | (mag_data[4]))));

#ifdef DEBUG
	pr_debug("%s read x=%X %X(regH regL), x=%d(dec) [ug]\n",
		LSM303D_MAG_DEV_NAME, mag_data[1], mag_data[0], hw_d[0]);
	pr_debug("%s read x=%X %X(regH regL), x=%d(dec) [ug]\n",
		LSM303D_MAG_DEV_NAME, mag_data[3], mag_data[0], hw_d[2]);
	pr_debug("%s read x=%X %X(regH regL), x=%d(dec) [ug]\n",
		LSM303D_MAG_DEV_NAME, mag_data[5], mag_data[0], hw_d[4]);
#endif

	hw_d[0] = hw_d[0] * stat->sensitivity_mag;
	hw_d[1] = hw_d[1] * stat->sensitivity_mag;
	hw_d[2] = hw_d[2] * stat->sensitivity_mag;

	xyz[0] = ((stat->pdata_mag->negate_x) ?
				(-hw_d[stat->pdata_mag->axis_map_x])
		   			: (hw_d[stat->pdata_mag->axis_map_x]));
	xyz[1] = ((stat->pdata_mag->negate_y) ?
				(-hw_d[stat->pdata_mag->axis_map_y])
		   			: (hw_d[stat->pdata_mag->axis_map_y]));
	xyz[2] = ((stat->pdata_mag->negate_z) ?
				(-hw_d[stat->pdata_mag->axis_map_z])
		   			: (hw_d[stat->pdata_mag->axis_map_z]));

	return err;
}

static void lsm303d_acc_report_values(struct lsm303d_status *stat, int *xyz)
{
	input_report_abs(stat->input_dev_acc, ABS_X, xyz[0]);
	input_report_abs(stat->input_dev_acc, ABS_Y, xyz[1]);
	input_report_abs(stat->input_dev_acc, ABS_Z, xyz[2]);
	input_sync(stat->input_dev_acc);
}

static void lsm303d_mag_report_values(struct lsm303d_status *stat, int *xyz)
{
	input_report_abs(stat->input_dev_mag, ABS_X, xyz[0]);
	input_report_abs(stat->input_dev_mag, ABS_Y, xyz[1]);
	input_report_abs(stat->input_dev_mag, ABS_Z, xyz[2]);
	input_sync(stat->input_dev_mag);
}

static void lsm303d_acc_input_work_func(struct work_struct *work)
{
	struct lsm303d_status *stat;
	int xyz[3] = { 0 };
	int err;

	stat = container_of((struct delayed_work *)work,
			struct lsm303d_status, input_work_acc);

	mutex_lock(&stat->lock);
	err = lsm303d_acc_get_data(stat, xyz);
	if (err < 0)
		dev_err(&stat->client->dev, "get_accelerometer_data failed\n");
	else
		lsm303d_acc_report_values(stat, xyz);

		schedule_delayed_work(&stat->input_work_acc, usecs_to_jiffies(
			stat->pdata_acc->poll_interval));
	mutex_unlock(&stat->lock);
}

static void lsm303d_mag_input_work_func(struct work_struct *work)
{
	struct lsm303d_status *stat;
	int xyz[3] = { 0 };
	int err;

	stat = container_of((struct delayed_work *)work,
			struct lsm303d_status, input_work_mag);

	mutex_lock(&stat->lock);
	err = lsm303d_mag_get_data(stat, xyz);
	if (err < 0)
		dev_err(&stat->client->dev, "get_magnetometer_data failed\n");
	else
		lsm303d_mag_report_values(stat, xyz);

		schedule_delayed_work(&stat->input_work_mag, usecs_to_jiffies(
			stat->pdata_mag->poll_interval));
	mutex_unlock(&stat->lock);
}

static int lsm303d_acc_input_init(struct lsm303d_status *stat)
{
	int err;

	INIT_DELAYED_WORK(&stat->input_work_acc, lsm303d_acc_input_work_func);
	stat->input_dev_acc = input_allocate_device();
	if (!stat->input_dev_acc) {
		err = -ENOMEM;
		dev_err(&stat->client->dev, "accelerometer "
					"input device allocation failed\n");
		goto err0;
	}

	stat->input_dev_acc->open = lsm303d_acc_input_open;
	stat->input_dev_acc->close = lsm303d_acc_input_close;
	stat->input_dev_acc->name = LSM303D_ACC_DEV_NAME;
	stat->input_dev_acc->id.bustype = BUS_I2C;
	stat->input_dev_acc->dev.parent = &stat->client->dev;

	input_set_drvdata(stat->input_dev_acc, stat);

	set_bit(EV_ABS, stat->input_dev_acc->evbit);

	input_set_abs_params(stat->input_dev_acc, ABS_X,
					-ACC_G_MAX_NEG, ACC_G_MAX_POS, FUZZ, FLAT);
	input_set_abs_params(stat->input_dev_acc, ABS_Y,
					-ACC_G_MAX_NEG, ACC_G_MAX_POS, FUZZ, FLAT);
	input_set_abs_params(stat->input_dev_acc, ABS_Z,
					-ACC_G_MAX_NEG, ACC_G_MAX_POS, FUZZ, FLAT);

	err = input_register_device(stat->input_dev_acc);
	if (err) {
		dev_err(&stat->client->dev,
			"unable to register accelerometer input device %s\n",
				stat->input_dev_acc->name);
		goto err1;
	}

	return 0;

err1:
	input_free_device(stat->input_dev_acc);
err0:
	return err;
}

static int lsm303d_mag_input_init(struct lsm303d_status *stat)
{
	int err;

	INIT_DELAYED_WORK(&stat->input_work_mag, lsm303d_mag_input_work_func);
	stat->input_dev_mag = input_allocate_device();
	if (!stat->input_dev_mag) {
		err = -ENOMEM;
		dev_err(&stat->client->dev, "magnetometer "
					"input device allocation failed\n");
		goto err0;
	}

	stat->input_dev_mag->open = lsm303d_mag_input_open;
	stat->input_dev_mag->close = lsm303d_mag_input_close;
	stat->input_dev_mag->name = LSM303D_MAG_DEV_NAME;
	stat->input_dev_mag->id.bustype = BUS_I2C;
	stat->input_dev_mag->dev.parent = &stat->client->dev;

	input_set_drvdata(stat->input_dev_mag, stat);

	set_bit(EV_ABS, stat->input_dev_mag->evbit);

	input_set_abs_params(stat->input_dev_mag, ABS_X,
					-MAG_G_MAX_NEG, MAG_G_MAX_POS, FUZZ, FLAT);
	input_set_abs_params(stat->input_dev_mag, ABS_Y,
					-MAG_G_MAX_NEG, MAG_G_MAX_POS, FUZZ, FLAT);
	input_set_abs_params(stat->input_dev_mag, ABS_Z,
					-MAG_G_MAX_NEG, MAG_G_MAX_POS, FUZZ, FLAT);

	err = input_register_device(stat->input_dev_mag);
	if (err) {
		dev_err(&stat->client->dev,
			"unable to register magnetometer input device %s\n",
				stat->input_dev_mag->name);
		goto err1;
	}

	return 0;

err1:
	input_free_device(stat->input_dev_mag);
err0:
	return err;
}

static void lsm303d_input_cleanup(struct lsm303d_status *stat)
{
	input_unregister_device(stat->input_dev_acc);
	input_free_device(stat->input_dev_acc);

	input_unregister_device(stat->input_dev_mag);
	input_free_device(stat->input_dev_mag);
}

static int lsm303d_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct lsm303d_status *stat;

	u32 smbus_func = I2C_FUNC_SMBUS_BYTE_DATA |
			I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_I2C_BLOCK;

	int err = -1;
	dev_info(&client->dev, "probe start.\n");
	stat = kzalloc(sizeof(struct lsm303d_status), GFP_KERNEL);
	if (stat == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for module data: "
					"%d\n", err);
		goto exit_check_functionality_failed;
	}

	stat->use_smbus = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_warn(&client->dev, "client not i2c capable\n");
		if (i2c_check_functionality(client->adapter, smbus_func)){
			stat->use_smbus = 1;
			dev_warn(&client->dev, "client using SMBUS\n");
		} else {
			err = -ENODEV;
			dev_err(&client->dev, "client nor SMBUS capable\n");
			goto exit_check_functionality_failed;
		}
	}

	mutex_init(&stat->lock);
	mutex_lock(&stat->lock);

	stat->client = client;
	i2c_set_clientdata(client, stat);

	stat->pdata_acc = kmalloc(sizeof(*stat->pdata_acc), GFP_KERNEL);
	stat->pdata_mag = kmalloc(sizeof(*stat->pdata_mag), GFP_KERNEL);
	if ((stat->pdata_acc == NULL)||(stat->pdata_mag == NULL)) {
		err = -ENOMEM;
		dev_err(&client->dev,
			"failed to allocate memory for pdata: %d\n", err);
		goto err_mutexunlock;
	}

	if (client->dev.platform_data) {
		struct lsm303d_main_platform_data *tmp;
		tmp = kzalloc(sizeof(struct lsm303d_main_platform_data),
								GFP_KERNEL);
		if(tmp == NULL)
			goto exit_kfree_pdata;
		memcpy(tmp, client->dev.platform_data, sizeof(*tmp));
		if(tmp->pdata_acc == NULL) {
			kfree(tmp);
			dev_err(&client->dev, "platform acc data is null\n");
			goto exit_kfree_pdata;
		} else {
			memcpy(stat->pdata_acc, tmp->pdata_acc,
						sizeof(*stat->pdata_acc));
		}
		if(tmp->pdata_mag == NULL) {
			kfree(tmp);
			dev_err(&client->dev, "platform mag data is null\n");
			goto exit_kfree_pdata;
		} else {
			memcpy(stat->pdata_mag, tmp->pdata_mag,
						sizeof(*stat->pdata_mag));
		}
		kfree(tmp);
	}
	else{
		dev_err(&client->dev, "platform data is null\n");
		goto exit_kfree_pdata;
	}
	err = lsm303d_acc_validate_pdata(stat);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data for "
							"accelerometer \n");
		goto exit_kfree_pdata;
	}

	err = lsm303d_mag_validate_pdata(stat);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data for "
							"magnetometer\n");
		goto exit_kfree_pdata;
	}

	if (stat->pdata_acc->init) {
		err = stat->pdata_acc->init();
		if (err < 0) {
			dev_err(&client->dev, "accelerometer init failed: "
								"%d\n", err);
			goto err_pdata_acc_init;
		}
	}
	if (stat->pdata_mag->init) {
		err = stat->pdata_mag->init();
		if (err < 0) {
			dev_err(&client->dev, "magnetometer init failed: "
								"%d\n", err);
			goto err_pdata_mag_init;
		}
	}

	err = lsm303d_hw_init(stat);
	if (err < 0) {
		dev_err(&client->dev, "hw init failed: %d\n", err);
		goto err_hw_init;
	}

	err = lsm303d_acc_device_power_on(stat);
	if (err < 0) {
		dev_err(&client->dev, "accelerometer power on failed: "
								"%d\n", err);
		goto err_pdata_init;
	}
	err = lsm303d_mag_device_power_on(stat);
	if (err < 0) {
		dev_err(&client->dev, "magnetometer power on failed: "
								"%d\n", err);
		goto err_pdata_init;
	}

	err = lsm303d_acc_update_fs_range(stat, stat->pdata_acc->fs_range);
	if (err < 0) {
		dev_err(&client->dev, "update_fs_range on accelerometer "
								"failed\n");
		goto  err_power_off_acc;
	}

	err = lsm303d_mag_update_fs_range(stat, stat->pdata_mag->fs_range);
	if (err < 0) {
		dev_err(&client->dev, "update_fs_range on magnetometer "
								"failed\n");
		goto  err_power_off_mag;
	}

	err = lsm303d_acc_update_odr(stat, stat->pdata_acc->poll_interval);
	if (err < 0) {
		dev_err(&client->dev, "update_odr on accelerometer failed\n");
		goto  err_power_off;
	}

	err = lsm303d_mag_update_odr(stat, stat->pdata_mag->poll_interval);
	if (err < 0) {
		dev_err(&client->dev, "update_odr on magnetometer failed\n");
		goto  err_power_off;
	}

	err = lsm303d_acc_input_init(stat);
	if (err < 0) {
		dev_err(&client->dev, "accelerometer input init failed\n");
		goto err_power_off;
	}

	err = lsm303d_mag_input_init(stat);
	if (err < 0) {
		dev_err(&client->dev, "magnetometer input init failed\n");
		goto err_power_off;
	}

	err = create_sysfs_interfaces(&client->dev);
	if (err < 0) {
		dev_err(&client->dev,
		   "device LSM303D_DEV_NAME sysfs register failed\n");
		goto err_input_cleanup;
	}

	lsm303d_acc_device_power_off(stat);
	lsm303d_mag_device_power_off(stat);

	mutex_unlock(&stat->lock);
	dev_info(&client->dev, "%s: probed\n", LSM303D_DEV_NAME);
	return 0;

err_input_cleanup:
	lsm303d_input_cleanup(stat);
err_power_off:
err_power_off_mag:
	lsm303d_mag_device_power_off(stat);
err_power_off_acc:
	lsm303d_acc_device_power_off(stat);
err_hw_init:
err_pdata_init:
err_pdata_mag_init:
	if (stat->pdata_mag->exit)
		stat->pdata_mag->exit();
err_pdata_acc_init:
	if (stat->pdata_acc->exit)
		stat->pdata_acc->exit();
exit_kfree_pdata:
	kfree(stat->pdata_acc);
	kfree(stat->pdata_mag);
err_mutexunlock:
	mutex_unlock(&stat->lock);
	kfree(stat);
exit_check_functionality_failed:
	pr_err("%s: Driver Init failed\n", LSM303D_DEV_NAME);
	return err;
}

static int __devexit lsm303d_remove(struct i2c_client *client)
{
	struct lsm303d_status *stat = i2c_get_clientdata(client);

	lsm303d_acc_disable(stat);
	lsm303d_mag_disable(stat);
	lsm303d_acc_input_cleanup(stat);
	lsm303d_mag_input_cleanup(stat);

	remove_sysfs_interfaces(&client->dev);

	if (stat->pdata_acc->exit)
		stat->pdata_acc->exit();

	if (stat->pdata_mag->exit)
		stat->pdata_mag->exit();

	kfree(stat->pdata_acc);
	kfree(stat->pdata_mag);
	kfree(stat);
	return 0;
}

static const struct i2c_device_id lsm303d_id[]
					= { { LSM303D_DEV_NAME, 0 }, { }, };

MODULE_DEVICE_TABLE(i2c, lsm303d_id);

static struct i2c_driver lsm303d_driver = {
	.driver = {
			.owner = THIS_MODULE,
			.name = LSM303D_DEV_NAME,
		  },
	.probe = lsm303d_probe,
	.remove = __devexit_p(lsm303d_remove),
	.id_table = lsm303d_id,
};

static int __init lsm303d_init(void)
{
	pr_info("%s driver: init\n", LSM303D_DEV_NAME);
	return i2c_add_driver(&lsm303d_driver);
}

static void __exit lsm303d_exit(void)
{
	pr_info("%s driver exit\n", LSM303D_DEV_NAME);
	i2c_del_driver(&lsm303d_driver);
}

module_init(lsm303d_init);
module_exit(lsm303d_exit);

MODULE_DESCRIPTION("lsm303d accelerometer and magnetometer driver");
MODULE_AUTHOR("Matteo Dameno, Denis Ciocca, STMicroelectronics");
MODULE_LICENSE("GPL");
