/*
 *  mma8452.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor
 *
 *  Copyright (C) 2010-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/board_sensors.h>
#include "gs_mma8452.h"
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

struct mma8452_poll_data
{
   int poll_interval;
   int min_interval;
   int max_interval;

   int g_range;
};

struct gs_mma8452_data
{
  struct i2c_client *client;
  struct input_dev *input_dev;
  struct mma8452_status mma_status;
  struct hrtimer timer;
  struct work_struct work;
  struct mma8452_acc_platform_data data;
  struct mma8452_poll_data pdata;
  struct early_suspend early_suspend;
  struct mutex lock;                    /* reentrant protection for struct */
  atomic_t chip_enabled;
  atomic_t timer_flag;
  int on_before_suspend;
};

static int mma_delay=10;
static struct workqueue_struct *mma_wq;

static int16_t accl8452_data[3] = {0, 0, 0};

static int ACCHAL[8][3][3] =
{
   {{ 0, -1,  0}, { 1,  0,    0}, {0, 0,    1}},
   {{-1,  0,  0}, { 0, -1,    0}, {0, 0,    1}},
   {{ 0,  1,  0}, {-1,  0,    0}, {0, 0,    1}},
   {{ 1,  0,  0}, { 0,  1,    0}, {0, 0,    1}},

   {{ 0, -1,  0}, {-1,  0,    0}, {0, 0,  -1}},
   {{-1,  0,  0}, { 0,  1,    0}, {0, 0,  -1}},
   {{ 0,  1,  0}, { 1,  0,    0}, {0, 0,  -1}},
   {{ 1,  0,  0}, { 0, -1,    0}, {0, 0,  -1}},
};

struct output_rate {
        unsigned long cutoff_us;
        unsigned char mask;
} mma8452_odr_table[] = {
        {    1250, 0x00 },              /* 800Hz  output data rate */
        {    2500, 0x08 },              /* 400Hz  output data rate */
        {    5000, 0x10 },              /* 200Hz  output data rate */
        {   10000, 0x18 },              /* 100Hz  output data rate */
        {   20000, 0x20 },              /* 50Hz   output data rate */
        {   80000, 0x28 },              /* 12.5Hz output data rate */
        {  160000, 0x30 },              /* 6.25Hz output data rate */
        {  640000, 0x38 },              /* 1.56Hz output data rate */
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mma8452_early_resume(struct early_suspend *h);
static void mma8452_early_suspend(struct early_suspend *h);
#endif

static int mma8452_adjust_position(struct gs_mma8452_data *mma,short *xyz)
{
   short rawdata[3],data[3];
   int i,j;
   int position = mma->mma_status.position ;
   if(position < 0 || position > 7 )
           position = 0;

   for(i=0;i<3;i++)
   rawdata[i]=xyz[i];

   for(i = 0; i < 3 ; i++)
   {
       data[i] = 0;
       for(j = 0; j < 3; j++)
       {
          data[i] += rawdata[j] * ACCHAL[position][i][j];
       }
   }

   for(i=0;i<3;i++)
   xyz[i]=data[i];

   return 0;
}

static int mma8452_read_data(struct gs_mma8452_data *mma,short *xyz)
{

    u8 tmp_data[MMA8452_BUF_SIZE];
    int ret;

    ret = i2c_smbus_read_i2c_block_data(mma->client,
                        MMA8452_OUT_X_MSB, MMA8452_BUF_SIZE, tmp_data);
    if (ret < MMA8452_BUF_SIZE)
    {
        dev_err(&mma->client->dev, "i2c block read failed\n");
        return -EIO;
    }

    xyz[0]= ((tmp_data[0] << 8) & 0xff00) | tmp_data[1];
    xyz[1]= ((tmp_data[2] << 8) & 0xff00) | tmp_data[3];
    xyz[2]= ((tmp_data[4] << 8) & 0xff00) | tmp_data[5];

    xyz[0]= xyz[0] >> 4;
    xyz[1]= xyz[1] >> 4;
    xyz[2]= xyz[2] >> 4;

    if (mma->mma_status.mode == MODE_4G)
    {
        xyz[0]=(xyz[0])<<1;
        xyz[1]=(xyz[1])<<1;
        xyz[2]=(xyz[2])<<1;
    } 
    else if (mma->mma_status.mode == MODE_8G)
    {
        xyz[0]=(xyz[0])<<2;
        xyz[1]=(xyz[1])<<2;
        xyz[2]=(xyz[2])<<2;
    }

    return 0;
}

static void report_abs(struct gs_mma8452_data *mma)
{
    short xyz[3];
    //if(mma->mma_status.active == MMA_STANDBY)
    if(atomic_read(&mma->chip_enabled) == 0)
    {
        goto out;
    }
    /*do{
        result = i2c_smbus_read_byte_data(mma->client,
                         MMA8452_STATUS);
        retry --;
        msleep(1);
    } while (!(result & MMA8452_STATUS_ZYXDR)&& retry > 0);
        if(retry == 0)
        goto out;*/

    if (mma8452_read_data(mma,xyz) != 0)
        goto out;
    mma8452_adjust_position(mma,xyz);
      
    accl8452_data[0] = xyz[0];
    accl8452_data[1] = xyz[1];
    accl8452_data[2] = xyz[2];
    input_report_abs(mma->input_dev, ABS_X,  xyz[0]);
    input_report_abs(mma->input_dev, ABS_Y,  xyz[1]);
    input_report_abs(mma->input_dev, ABS_Z,  xyz[2]);
   
    input_sync(mma->input_dev);
    
out:
    return;
}

static void mma8452_work_func(struct work_struct *work)
{
    int sesc = 0;
    unsigned long nsesc = 0;
    struct gs_mma8452_data *mma = container_of(work,struct gs_mma8452_data, work);

    mma_delay = mma->pdata.poll_interval;
    mma_delay = mma_delay - 1;
    sesc = mma_delay/1000;
    nsesc = (mma_delay%1000)*1000000;
    
    report_abs(mma);
    if(atomic_read(&mma->timer_flag))
    {
    	hrtimer_start(&mma->timer, ktime_set(sesc, nsesc), HRTIMER_MODE_REL);
    }
}

static enum hrtimer_restart mma8452_timer_func(struct hrtimer *timer)
{
    struct gs_mma8452_data *mma = container_of(timer, struct gs_mma8452_data, timer);
    queue_work(mma_wq, &mma->work);
    return HRTIMER_NORESTART;
}

static int mma8452_enable(struct gs_mma8452_data *mma)
{
    u8 val;
    int ret=0;
    //if(mma->mma_status.active == MMA_STANDBY)
    if(atomic_read(&mma->chip_enabled) == 0)
    {
        val = i2c_smbus_read_byte_data(mma->client,MMA8452_CTRL_REG1);
        ret = i2c_smbus_write_byte_data(mma->client, MMA8452_CTRL_REG1, val|0x01);      // set as ACTIVE_MODE
        if(!ret)
        {
            //mma->mma_status.active = MMA_ACTIVED;
            atomic_set(&mma->chip_enabled,1);
            atomic_set(&mma->timer_flag,1);
            dev_info(&mma->client->dev,"mma enable setting active \n");
        }
        hrtimer_start(&mma->timer, ktime_set(0, NORMAL_TM), HRTIMER_MODE_REL);
    }

    return ret;
 }

static int mma8452_disable(struct gs_mma8452_data *mma)
{
    u8 val;
    int ret=0;

    //if(mma->mma_status.active == MMA_ACTIVED)
    if(atomic_read(&mma->chip_enabled))
    {
        atomic_set(&mma->timer_flag, 0);
        //hrtimer_cancel(&mma->timer);
        cancel_work_sync(&mma->work);
        val = i2c_smbus_read_byte_data(mma->client,MMA8452_CTRL_REG1);
        ret = i2c_smbus_write_byte_data(mma->client, MMA8452_CTRL_REG1,val & 0xFE);         //  set as STANDBY_MODE
        if(!ret)
        {
            //mma->mma_status.active = MMA_STANDBY;
            atomic_set(&mma->chip_enabled,0);
            dev_info(&mma->client->dev,"mma enable setting inactive \n");
        }
    }
    return ret;
 }

static ssize_t mma8452_enable_show(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
    int enable;
    struct gs_mma8452_data *mma=dev_get_drvdata(dev);

    mutex_lock(&mma->lock);
    enable=atomic_read(&mma->chip_enabled);
    mutex_unlock(&mma->lock);
    return sprintf(buf, "%d\n", enable);
}

static ssize_t mma8452_enable_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    unsigned long enable;
    struct gs_mma8452_data *mma=dev_get_drvdata(dev);
    enable = simple_strtoul(buf, NULL, 10);

    mutex_lock(&mma->lock);
    enable = (enable > 0) ? 1 : 0;
    if(enable)
    {
       mma8452_enable(mma);
    }
    else
    {
        mma8452_disable(mma);
    }
    mutex_unlock(&mma->lock);
    return count;
}
static ssize_t mma8452_position_show(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
    int position = 0;
    struct gs_mma8452_data *mma=dev_get_drvdata(dev);
    mutex_lock(&mma->lock);
    position = mma->mma_status.position ;
    mutex_unlock(&mma->lock);
    return sprintf(buf, "%d\n", position);
}

static ssize_t mma8452_position_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    int  position;
    struct gs_mma8452_data *mma=dev_get_drvdata(dev);
    position = simple_strtoul(buf, NULL, 10);
    mutex_lock(&mma->lock);
    mma->mma_status.position = position;
    mutex_unlock(&mma->lock);
    return count;
}

int mma8452_update_odr(struct gs_mma8452_data *mma, int poll_interval_ms)
{
    int err = -1;
    int i;
    u8 val;
    unsigned long interval;

    interval=poll_interval_ms*1000;

    for (i = ARRAY_SIZE(mma8452_odr_table) - 1; i >= 0; i--)
    {
        if (mma8452_odr_table[i].cutoff_us <= interval)
            break;
    }
    if(i<0)
       i=0;

   //if(mma->mma_status.active ==MMA_ACTIVED)
   if(atomic_read(&mma->chip_enabled))
   {
       hrtimer_cancel(&mma->timer);
   }

   val = i2c_smbus_read_byte_data(mma->client, MMA8452_CTRL_REG1);
   if (val & 0x01)
   {                                                        /*if the chip was actived,set it as standby*/
        val &= 0xc6;
        err = i2c_smbus_write_byte_data(mma->client, MMA8452_CTRL_REG1, val);
        if (err)
        {
             dev_err(&mma->client->dev, "%s: disable write error.\n", __func__);
             goto error;
        }
   }

    val&=0xC7;
    val|= mma8452_odr_table[i].mask;
    err = i2c_smbus_write_byte_data(mma->client, MMA8452_CTRL_REG1,val);
    if(err<0)
    {
        dev_err(&mma->client->dev, "%s:  odr write error.\n", __func__);
        goto error;
    }

    //if(mma->mma_status.active == MMA_ACTIVED)
    if(atomic_read(&mma->chip_enabled))
    {
        val=i2c_smbus_read_byte_data(mma->client, MMA8452_CTRL_REG1);
        err=i2c_smbus_write_byte_data(mma->client, MMA8452_CTRL_REG1, val | 0x01);
        if(err)
        {
            dev_err(&mma->client->dev, "%s:  enable write error.\n", __func__);
            goto error;
        }

        hrtimer_start(&mma->timer, ktime_set(0, NORMAL_TM), HRTIMER_MODE_REL);
    }

    return err;

error:
    dev_err(&mma->client->dev, "update odr failed %d\n",err);
    return err;
}

static ssize_t mma8452_get_polling_rate(struct device *dev,
                     struct device_attribute *attr,
                     char *buf)
{
    int val;
    struct gs_mma8452_data *mma = dev_get_drvdata(dev);
    mutex_lock(&mma->lock);
    val = mma->pdata.poll_interval;
    mutex_unlock(&mma->lock);
    return sprintf(buf, "%d\n", val);

    return 0;
}

static ssize_t mma8452_set_polling_rate(struct device *dev,
                     struct device_attribute *attr,
                     const char *buf, size_t size)
{

    struct gs_mma8452_data *mma = dev_get_drvdata(dev);
    unsigned long interval_ms;
    if (strict_strtoul(buf, 10, &interval_ms))
        return -EINVAL;
    if (!interval_ms)
        return -EINVAL;
    mutex_lock(&mma->lock);
    if(interval_ms < 10) interval_ms = 10;
    mma->pdata.poll_interval = interval_ms;
    mma8452_update_odr(mma,interval_ms);
    mutex_unlock(&mma->lock);

    return size;
}

int mma8452_update_g_range(struct gs_mma8452_data *mma, u8 new_g_range)
{
    int err=-1;
    u8 mode;
    switch (new_g_range)
       {
        case 2:
            mode = MODE_2G;
            break;
        case 4:
            mode = MODE_4G;
            break;
        case 8:
            mode = MODE_8G;
             break;
        default:
            dev_err(&mma->client->dev, "invalid g range requested: %d\n",
                new_g_range);
        return -EINVAL;
    }
    
    mma->mma_status.mode=mode;
    err = i2c_smbus_write_byte_data(mma->client, MMA8452_XYZ_DATA_CFG,mode);              
    if (err < 0)
    {
        goto error;
    }
    mdelay(MODE_CHANGE_DELAY_MS);

    return err;
error:
    dev_err(&mma->client->dev, "update g range failed  %d\n", err);
    return err;
}

static ssize_t mma8452_get_range(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
    int val;
    struct gs_mma8452_data *mma = dev_get_drvdata(dev);
    mutex_lock(&mma->lock);
    val = mma->pdata.g_range ;
    mutex_unlock(&mma->lock);
    return sprintf(buf, "%d\n", val);
}

static ssize_t mma8452_set_range(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t size)
{
    struct gs_mma8452_data *mma = dev_get_drvdata(dev);
    unsigned long val;
    u8 range;
    val=simple_strtoul(buf,NULL,10);
    switch(val)
    {
        case 2:
            range=2;
            break;
        case 4:
            range=4;
            break;
        case 8:
            range=8;
            break;
        default:
            dev_err(&mma->client->dev, "invalid g range seted: %lu\n",val);
            return -EINVAL;
    }
    mutex_lock(&mma->lock);
    mma->pdata.g_range = val;
    mma8452_update_g_range(mma, range);
    mutex_unlock(&mma->lock);
    return size;
}

static ssize_t mma8452_get_accl_data(struct device *dev,
                    struct device_attribute *attr,    char *buf)
{
    struct gs_mma8452_data *mma = dev_get_drvdata(dev);
    mutex_lock(&mma->lock);
    *((int16_t *)&buf[0]) = accl8452_data[0];
    *((int16_t *)&buf[2]) = accl8452_data[1];
    *((int16_t *)&buf[4]) = accl8452_data[2];
    mutex_unlock(&mma->lock);
    //printk("buf1=%d,buf2=%d,buf3=%d", buf[0],buf[1],buf[2]);
    return ACCL_DATA_SIZE;
}

static ssize_t mma8452_get_accl_info(struct device *dev,
                 struct device_attribute *attr, char *buf)
{
    ssize_t count;
    count = sprintf(buf, "FREESCALE MMA8452");
    return count;
}
static struct device_attribute attributes[] =
    {
        __ATTR(enable, S_IWUSR | S_IRUGO,
                   mma8452_enable_show, mma8452_enable_store),
        __ATTR(position, S_IWUSR | S_IRUGO,
                   mma8452_position_show, mma8452_position_store),
        __ATTR(pollrate_ms, 0664, mma8452_get_polling_rate, mma8452_set_polling_rate),
       __ATTR(range, 0664, mma8452_get_range, mma8452_set_range),
     __ATTR(accl_data, 0664, mma8452_get_accl_data, NULL),

     __ATTR(accl_info, 0664, mma8452_get_accl_info, NULL),
};
static int mma8452_hw_init(struct gs_mma8452_data *mma)
{
    int result;
    mma->pdata.poll_interval=POLL_INTERVAL;
    mma->pdata.min_interval =POLL_INTERVAL_MIN;
    mma->pdata.max_interval =POLL_INTERVAL_MAX;

    result=mma8452_update_odr(mma, mma->pdata.poll_interval);
    if (result < 0)
          dev_err(&mma->client->dev,"error when mma8452_update_odr init mma8452:(%d)",result);

    mma->pdata.g_range=2;
    result=mma8452_update_g_range(mma, mma->pdata.g_range);
    if(result<0)
          dev_err(&mma->client->dev,"error when mma8452_update_g_range init :(%d)",result);

    //mma->mma_status.active=MMA_STANDBY;
    atomic_set(&mma->chip_enabled,0);

    dev_info(&mma->client->dev, "mma8452_hw_init: init chip success! result=%d\n", result);
    return result;
}

 int mma8452_input_open(struct input_dev *input)
{
    struct gs_mma8452_data *mma = input_get_drvdata(input);
    return mma8452_enable(mma);
}

void mma8452_input_close(struct input_dev *dev)
{
    struct gs_mma8452_data *mma = input_get_drvdata(dev);
    mma8452_disable(mma);
}

static int mma8452_input_init(struct gs_mma8452_data *mma)
{
    int err;
    INIT_WORK(&mma->work,mma8452_work_func);

    mma->input_dev=input_allocate_device();
    if(!mma->input_dev)
    {
        err=-ENOMEM;
        dev_err(&mma->client->dev,"intput device allocate failed\n");
        goto err0;
    }
   
    mma->input_dev->open  = NULL;
    mma->input_dev->close = NULL;
    mma->input_dev->name = ACCL_INPUT_DEV_NAME;
    //acc->input->name = "accelerometer";
    mma->input_dev->id.bustype = BUS_I2C;
    mma->input_dev->dev.parent = &mma->client->dev;

    input_set_drvdata(mma->input_dev, mma);

    mma->input_dev->evbit[0]=BIT_MASK(EV_ABS);

    input_set_abs_params(mma->input_dev, ABS_X, -8192, 8191, INPUT_FUZZ, INPUT_FLAT);
    input_set_abs_params(mma->input_dev, ABS_Y, -8192, 8191, INPUT_FUZZ, INPUT_FLAT);
    input_set_abs_params(mma->input_dev, ABS_Z, -8192, 8191, INPUT_FUZZ, INPUT_FLAT);

    err = input_register_device(mma->input_dev);
    if (err)
    {
        dev_err(&mma->client->dev,
                "unable to register input device %s\n",
                mma->input_dev->name);
        goto err1;
    }
	err = set_sensor_input(ACC, mma->input_dev->dev.kobj.name);
	if (err) {
		dev_err(&mma->client->dev, "%s set_sensor_input failed!\n", __func__);
		goto err1;
	}
    hrtimer_init(&mma->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    mma->timer.function = mma8452_timer_func;
    mma_wq = create_singlethread_workqueue("mma_wq");
    if (!mma_wq)
        return -ENOMEM;
    dev_info(&mma->client->dev, "mma8452_input_init: init input_dev success! result=%d\n", err);
    return 0;

err1:
    input_free_device(mma->input_dev);
err0:
    return err;
}

static int create_sysfs_interfaces(struct device *dev)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(attributes); i++)
        if (device_create_file(dev, attributes + i))          //  create sysfs attribute file for device
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

static int  mma8452_probe(struct i2c_client *client ,
                                  const struct i2c_device_id *id)
{
    int err=-1,client_id;
    struct i2c_adapter *adapter;
    struct gs_mma8452_data *mma;
    struct mma8452_acc_platform_data *devpd =client->dev.platform_data;
    pr_info("%s: probe start.\n", MMA8452_ACC_DEV_NAME);

    if (!devpd)
    {
        dev_err(&client->dev, "No platfrom data!\n");
        err=-ENODEV;
        goto err_get_power_fail;
    }
    /*get power*/
    if(devpd->power_on)
    {
        err = devpd->power_on(&client->dev);
        if(err < 0)
        {
            dev_err(&client->dev, "mma8452_acc_probe: get power fail! result=%d\n", err);
            goto err_get_power_fail;
        }
        dev_dbg(&client->dev,"mma8452_acc_probe: get power success! \n");
    }
    dev_info(&client->dev, "mma8452_acc_probe: get power success! result=%d\n", err);

    /*check functionality*/
    adapter = to_i2c_adapter(client->dev.parent);
    err = i2c_check_functionality(adapter,
                     I2C_FUNC_SMBUS_BYTE |
                     I2C_FUNC_SMBUS_BYTE_DATA);
    if (!err)
    {
        dev_err(&client->dev, "client not i2c capable\n");
        err=-ENODEV;
        goto err_out;
    }

    msleep(5);

    client_id = i2c_smbus_read_byte_data(client, MMA8452_WHO_AM_I);     // MMA8452_WHO_AM_I=3A
    if (client_id != MMA8452_ID)
    {
        dev_err(&client->dev,
            "read chip ID 0x%x is not equal to 0x%x or 0x%x!\n",
            err, MMA8452_ID, MMA8452_ID);
        err = -EINVAL;
        goto err_out;
    }
	dev_info(&client->dev, "Read mma8452 chip ok, ID is 0x%x\n", client_id);
	err = set_sensor_chip_info(ACC, "FREESCALE MMA8452");
	if (err) {
		dev_err(&client->dev, "set_sensor_chip_info error \n");
	}
    /*allocate memory*/
    mma=kzalloc(sizeof(struct gs_mma8452_data), GFP_KERNEL);
    if (!mma)
    {
        err = -ENOMEM;
        dev_err(&client->dev,
                "failed to allocate memory for module data: "
                    "%d\n",err);
        goto err_out;
    }
	err =  i2c_smbus_write_byte_data(client, MMA8452_CTRL_REG3, 0x2);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed to selects the polarity of the interrupt signal\n", __func__);
		goto err_out;
	}
    mutex_init(&mma->lock);
    mma->client=client;
    i2c_set_clientdata(client,mma);
    /* Initialize the MMA8452 chip */
    err = mma8452_hw_init(mma);      // MOD_2G     0
    if (err<0)
    {
        dev_err(&client->dev,
            "error mma8452_hw_init init chip failed:(%d)\n", err);
        goto err_init;
    }

    /*Initialize the input device */
    err=mma8452_input_init(mma);
    if(err<0)
    {
        dev_err(&client->dev,"input init failed \n");
        goto err_init;
    }

    err = create_sysfs_interfaces(&client->dev);
    if (err < 0)
    {
        dev_err(&client->dev, "device MMA8452_ACC_DEV_NAME sysfs register failed\n");
        goto err_create_sysfs;
    }
    dev_info(&client->dev, "create_sysfs_interfaces: create interfaces success! ");
    mma->mma_status.position = devpd->config_mxc_mma_position;

    #ifdef CONFIG_HAS_EARLYSUSPEND
    mma->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    mma->early_suspend.suspend = mma8452_early_suspend;
    mma->early_suspend.resume = mma8452_early_resume;
    register_early_suspend(&mma->early_suspend);
    #endif
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
        /* detect current device successful, set the flag as present */
    set_hw_dev_flag(DEV_I2C_G_SENSOR);
#endif
    pr_info("%s: probe success.\n", MMA8452_ACC_DEV_NAME);
    return 0;

err_create_sysfs:
       remove_sysfs_interfaces(&client->dev);
       input_unregister_device(mma->input_dev);
       input_free_device(mma->input_dev);
err_init:
    kfree(mma);
err_out:
    if(devpd->power_off)
          devpd->power_off();
err_get_power_fail:
    return err;
}

static int __devexit mma8452_remove(struct i2c_client *client)
{
    int ret;
    struct gs_mma8452_data *mma=i2c_get_clientdata(client);
    struct mma8452_acc_platform_data *devpd = client->dev.platform_data;
    ret = mma8452_disable(mma);

    input_unregister_device(mma->input_dev);
    input_free_device(mma->input_dev);

    remove_sysfs_interfaces(&client->dev);

    kfree(mma);
    if(devpd->power_off)
           devpd->power_off();
    return ret;
}

static int mma8452_suspend(struct device *dev)
{
    struct gs_mma8452_data *mma = dev_get_drvdata(dev);
    int ret = 0;
    mma->on_before_suspend = atomic_read(&mma->chip_enabled);
    if (mma->on_before_suspend)
    {
	ret = mma8452_disable(mma);
    }
    return ret;
}

static int mma8452_resume(struct device *dev)
{
    struct gs_mma8452_data *mma = dev_get_drvdata(dev);
    if (mma->on_before_suspend)
    {
        mma8452_enable(mma);
    }
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mma8452_early_resume(struct early_suspend *h)
{
   struct gs_mma8452_data *mma=container_of(h,struct gs_mma8452_data,early_suspend);
   mma8452_resume(&mma->client->dev);
   dev_info(&mma->client->dev,"mma8452 has resumed\n");
}

static void mma8452_early_suspend(struct early_suspend *h)
{
   struct gs_mma8452_data *mma=container_of(h,struct gs_mma8452_data,early_suspend);
   mma8452_suspend(&mma->client->dev);
   dev_info(&mma->client->dev,"mma8452 has suspend\n");
}
#endif

static const struct i2c_device_id mma8452_id[] = {
    {"mma8452", 0},
};

MODULE_DEVICE_TABLE(i2c, mma8452_id);

//static SIMPLE_DEV_PM_OPS(mma8452_pm_ops, mma8452_suspend, mma8452_resume);
static struct i2c_driver mma8452_driver = 
{
    .driver = 
    {
        .name = "mma8452",
        .owner = THIS_MODULE,
        //.pm = &mma8452_pm_ops,
           
    },
    .probe = mma8452_probe,
    .remove = __devexit_p(mma8452_remove),
    .id_table = mma8452_id,
};

static int __init mma8452_init(void)
{
    /* register driver */
    int res;
    printk(KERN_INFO "%s accelerometer driver:init\n",
     MMA8452_ACC_DEV_NAME);
    res = i2c_add_driver(&mma8452_driver);
    if (res < 0) 
    {
        printk(KERN_INFO "add mma8452 i2c driver failed\n");
        return -ENODEV;
    }
    return res;
}

static void __exit mma8452_exit(void)
{
    i2c_del_driver(&mma8452_driver);
    if(mma_wq)
        destroy_workqueue(mma_wq);
    return;
}

module_init(mma8452_init);
module_exit(mma8452_exit);

MODULE_AUTHOR("Chen Gang <gang.chen@freescale.com>");
MODULE_DESCRIPTION("MMA8452 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.3.2");
