/* drivers/input/misc/cm3320.c - cm3320 color sensor driver
 *
 * Copyright (C) 2012 Capella Microsystems Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/gpio.h>
//#include <linux/lightsensor.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/mach-types.h>
//#include <linux/cm3320.h>
#include <asm/setup.h>
#include <linux/wakelock.h>
#include <linux/jiffies.h>
#include "cm3320.h"
#include <linux/board_sensors.h>
#include <hsad/config_interface.h>

#define CM3320_I2C_NAME "cm3320"

#define REL_RED		REL_X
#define REL_GREEN	REL_Y
#define REL_BLUE	REL_Z

#define	CM3320_W_CMD1_addr	0x20 >> 1
#define	CM3320_W_CMD2_addr	0x22 >> 1
#define	CM3320_W_CMD3_addr	0x24 >> 1
#define	CM3320_R_MSB_addr	0x21 >> 1
#define	CM3320_R_LSB_addr	0x23 >> 1

/*cm3320*/
/*for command 1 (20h) */
#define CM3320_CMD1_RD_SEL_R	0x00
#define CM3320_CMD1_RD_SEL_G	0x01
#define CM3320_CMD1_RD_SEL_B	0x02
#define CM3320_CMD1_CR			0x08
#define CM3320_CMD1_SD			0x80
#define CM3320_CMD1_DEFAULT		0x54

/*for command 2 (22h) */
#define CM3320_CMD2_DEFAULT		0x70

/*for command 3 (24h) */
#define CM3320_CMD3_B_IT_0_5T	0x00
#define CM3320_CMD3_B_IT_1T		0x01
#define CM3320_CMD3_B_IT_2T		0x02
#define CM3320_CMD3_B_IT_4T		0x03
#define CM3320_CMD3_G_IT_0_5T	0x00
#define CM3320_CMD3_G_IT_1T		0x04
#define CM3320_CMD3_G_IT_2T		0x08
#define CM3320_CMD3_G_IT_4T		0x0C
#define CM3320_CMD3_R_IT_0_5T	0x00
#define CM3320_CMD3_R_IT_1T		0x10
#define CM3320_CMD3_R_IT_2T		0x20
#define CM3320_CMD3_R_IT_4T		0x30
//-------define--light_source----begin
#define light_u30_coolwhite					1
#define light_Daylight						2
#define light_Fluorescence					3
#define light_Xenon							4
#define light_Incand_EHLS_Xenon_Horizon		5
#define light_Incand_EHLS					6
#define light_Horizon						7
//-------define--light_source----end

//add 20120906
#define	light_status_print()			;//printk("it is %s ",(lpi->light_status==1?"light_u30_coolwhite":(lpi->light_status==2?"light_Daylight":(lpi->light_status==3?"light_Fluorescence":(lpi->light_status==4?"light_Xenon":(lpi->light_status==5?"light_Incand_EHLS_Xenon_Horizon":(lpi->light_status==6?"light_Incand_EHLS":(lpi->light_status==7?"light_Horizon":"no record"))))))))
#define	light_data_print()				;//printk(" ,  luxValue = %d , Red= %d , green= %d ,blue = %d ,BG_Ratio= %d ,RG_RB_Ratio = %d ,RG_BG_Ratio =%d\n",luxValue,red,green,blue,BG_Ratio,RG_RB_Ratio,RG_BG_Ratio)
#define D(x...) 				pr_info(x)
#define I2C_RETRY_COUNT 		10
#define LS_POLLING_DELAY 		500
#define LS_PWR_ON				(1 << 0)
//add
#define RGB_DATA_SIZE 6
static uint8_t rgb_data[6] = {0, 0, 0, 0, 0, 0};
static uint16_t color_data[3];
static int color_value;
static int als_polling_count;
//add
extern int light_red;
extern int light_green;
extern int light_blue;
extern int light_red_bright;
extern int light_green_bright;
extern int light_blue_bright;
extern unsigned long  light_red_delay_on;
extern unsigned long light_red_delay_off;
extern unsigned long light_green_delay_on;
extern unsigned long light_green_delay_off;
extern unsigned long light_blue_delay_on;
extern unsigned long light_blue_delay_off;

static void report_do_work(struct work_struct *w);
static DECLARE_DELAYED_WORK(report_work, report_do_work);

struct cm3320_info {
	struct class *cm3320_class;
	struct device *ls_dev;
	struct input_dev *ls_input_dev;
	struct early_suspend early_suspend;
	struct i2c_client *i2c_client;
	struct workqueue_struct *lp_wq;
	int als_enable;
	int (*power)(int, uint8_t); /* power to the chip */
	int polling_delay;
	int light_status;
	uint16_t red[3];
	uint16_t green[3];
	uint16_t blue[3];
	uint16_t index;
};
struct cm3320_info *lp_info;
int enable_log = 0;
static struct mutex als_enable_mutex, als_disable_mutex, als_get_value_mutex;
static int lightsensor_enable(struct cm3320_info *lpi);
static int lightsensor_disable(struct cm3320_info *lpi);

int32_t als_kadc;

static int I2C_RxData(uint16_t slaveAddr, uint8_t *rxData, int length)
{
	uint8_t loop_i;

	struct i2c_msg msgs[] = {
		{
		 .addr = slaveAddr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxData,
		 },
	};

	for (loop_i = 0; loop_i < I2C_RETRY_COUNT; loop_i++) {

		if (i2c_transfer(lp_info->i2c_client->adapter, msgs, 1) > 0)
			break;

		msleep(10);
	}
	if (loop_i >= I2C_RETRY_COUNT) {
		printk(KERN_ERR "[ERR][CM3320 error] %s retry over %d\n",
			__func__, I2C_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

static int I2C_TxData(uint16_t slaveAddr, uint8_t *txData, int length)
{
	uint8_t loop_i;

	struct i2c_msg msg[] = {
		{
		 .addr = slaveAddr,
		 .flags = 0,
		 .len = length,
		 .buf = txData,
		 },
	};

	for (loop_i = 0; loop_i < I2C_RETRY_COUNT; loop_i++) {
		if (i2c_transfer(lp_info->i2c_client->adapter, msg, 1) > 0)
			break;

		msleep(10);
	}

	if (loop_i >= I2C_RETRY_COUNT) {
		printk(KERN_ERR "[ERR][CM3320 error] %s retry over %d\n",
			__func__, I2C_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

static int cm3320_I2C_Read_Byte(uint16_t slaveAddr, uint8_t *pdata)
{
	uint8_t buffer = 0;
	int ret = 0;

	if (pdata == NULL)
		return -EFAULT;

	ret = I2C_RxData(slaveAddr, &buffer, 1);
	if (ret < 0) {
		pr_err(
			"[ERR][CM3320 error]%s: I2C_RxData fail, slave addr: 0x%x\n",
			__func__, slaveAddr);
		return ret;
	}

	*pdata = buffer;
#if 0
	/* Debug use */
	printk(KERN_DEBUG "[CM3320] %s: I2C_RxData[0x%x] = 0x%x\n",
		__func__, slaveAddr, buffer);
#endif
	return ret;
}

static int cm3320_I2C_Write_Byte(uint16_t SlaveAddress,
				uint8_t data)
{
	char buffer[2];
	int ret = 0;
#if 0
	/* Debug use */
	printk(KERN_DEBUG
	"[CM3320] %s: cm3320_I2C_Write_Byte[0x%x, 0x%x, 0x%x]\n",
		__func__, SlaveAddress, cmd, data);
#endif
	buffer[0] = data;

	ret = I2C_TxData(SlaveAddress, buffer, 1);
	if (ret < 0) {
		pr_err("[ERR][CM3320 error]%s: I2C_TxData fail\n", __func__);
		return -EIO;
	}

	return ret;
}

static int cm3320_read_light_data(uint8_t color, uint16_t *light_data)
{
	uint8_t	lsb, msb;
	int ret = 0;

	// Select the color of light data
	ret = cm3320_I2C_Write_Byte(CM3320_W_CMD1_addr, CM3320_CMD1_DEFAULT | color);
	if (ret < 0) {
		pr_err(
			"[LS][CM3320 error]%s: cm3320_I2C_Write_Byte fail\n",
			__func__);
		return -EIO;
	}

	/* Read ALS data: LSB */
	ret = cm3320_I2C_Read_Byte(CM3320_R_LSB_addr, &lsb);
	if (ret < 0) {
		pr_err(
			"[LS][CM3320 error]%s: cm3320_I2C_Read_Byte LSB fail\n",
			__func__);
		return -EIO;
	}

	/* Read ALS data: MSB */
	ret = cm3320_I2C_Read_Byte(CM3320_R_MSB_addr, &msb);
	if (ret < 0) {
		pr_err(
			"[LS][CM3320 error]%s: cm3320_I2C_Read_Byte MSB fail\n",
			__func__);
		return -EIO;
	}

	*light_data = (uint16_t)msb;
	*light_data <<= 8;
	*light_data |= (uint16_t)lsb;
#if 0
	D("[LS][CM3320] %s: %d light data = 0x%X\n",
		__func__, color, *light_data);
#endif
	return ret;
}

static int get_cm3320_rgb_value(struct cm3320_info *lpi)
{
	int ret = 0;

	if (lpi == NULL)
		return -EFAULT;

	lpi->index = (lpi->index + 1) % 3;

	ret = cm3320_read_light_data(CM3320_CMD1_RD_SEL_R, &(lpi->red[lpi->index]));
	if (ret < 0) {
		pr_err(
			"[LS][CM3320 error]%s: cm3320_read_light_data for RED fail\n",
			__func__);
		return -EIO;
	}

	ret = cm3320_read_light_data(CM3320_CMD1_RD_SEL_G, &(lpi->green[lpi->index]));
	if (ret < 0) {
		pr_err(
			"[LS][CM3320 error]%s: cm3320_read_light_data for GREEN fail\n",
			__func__);
		return -EIO;
	}

	ret = cm3320_read_light_data(CM3320_CMD1_RD_SEL_B, &(lpi->blue[lpi->index]));
	if (ret < 0) {
		pr_err(
			"[LS][CM3320 error]%s: cm3320_read_light_data for BLUE fail\n",
			__func__);
		return -EIO;
	}

	return ret;
}

static void report_lsensor_input_event(struct cm3320_info *lpi, bool resume)
{/*when resume need report a data, so the paramerter need to quick reponse*/
	int ret = 0;
	uint16_t red, green, blue;
	//uint16_t CCTI,CCT,luxValue;
	int CCTI,CCT,luxValue,gain;
	int BG_Ratio;
	int RG_Ratio;
	int RB_Ratio;
	int lux_factor, Rf,Gf,Bf;
	int Hysteretic_A,Hysteretic_B,Hysteretic_C,Hysteretic_D1,Hysteretic_D2,Hysteretic_E;
	int Hysteretic_B1,Hysteretic_B2,Hysteretic_B3;
	static int RG_RB_Ratio, RG_BG_Ratio;// why is  static?
	int sensor_tp_type;

	mutex_lock(&als_get_value_mutex);

	ret = get_cm3320_rgb_value(lpi);
	sensor_tp_type = get_sensor_tp_type();
	// Calculate the average of red, green and blue light data
	red = (lpi->red[0] + lpi->red[1] + lpi->red[2]) / 3;
	green = (lpi->green[0] + lpi->green[1] + lpi->green[2]) / 3;
	blue = (lpi->blue[0] + lpi->blue[1] + lpi->blue[2]) / 3;
	//printk("it is for looking :  Red= %d , green= %d ,blue = %d \n",red,green,blue);
//********remove the interference from RGB LED********begin******//
	if( (sensor_tp_type == 0) || (sensor_tp_type == 1) )
	{
		if(light_red_bright == 2 && light_green_bright == 2)//the led is red + green
		{

			red   = red   <30? 0:(red-30);
			green = green <35? 0:(green-35);
			blue  = blue  <8? 0:(blue-8);
		}
		else if(light_red_bright == 2)//the led is green
		{

			red   = red   <6? 0:(red-6);
			green = green <20? 0:(green-20);
			blue  = blue  <4? 0:(blue-4);
		}
		else if(light_green_bright == 2)//the led is red
		{

			red   = red   <21? 0:(red-21);
			green = green <16? 0:(green-16);
			blue  = blue  <6? 0:(blue-6);
		}
	}
//********remove the interference from RGB LED********end******//
	color_data[0] = red;
	color_data[1] = green;
	color_data[2] = blue;
	//printk(" it is removed LED : Red= %d , green= %d ,blue = %d \n",red,green,blue);
	// Calculate CCTi ,for caculate *1000;
     //   CCTI = 1000*(red - blue)/green;
	if(blue <=0)
	    blue =1;
	if (red <=0)
		red  =1;

    if(sensor_tp_type == 0)
	{
		if(green ==0)
		{
		    luxValue=0;
		}
		else if (green>65534)
			luxValue=10000;
		else
		{
			BG_Ratio = 1000* blue / green;
			RG_Ratio = 1000* red  / green;
			RB_Ratio = 1000* red  / blue;
			//CCTI = 1000*(red - blue)/green;
			RG_RB_Ratio = RG_Ratio * RB_Ratio /1000;
			RG_BG_Ratio = RG_Ratio * BG_Ratio /1000;
			//-----judge----old light status------begin
			switch(lpi->light_status)
			{
				case light_u30_coolwhite:
						Hysteretic_A = 25;
						Hysteretic_B1= 50;
						Hysteretic_B2= 50;
						Hysteretic_B3= 50;
						Hysteretic_C = 25;
						break;
				case light_Daylight:
						Hysteretic_A =-25;
						Hysteretic_B1= 50;
						Hysteretic_B2= 50;
						Hysteretic_B3= 50;
						Hysteretic_C =-25;
						break;
				case light_Fluorescence:
						Hysteretic_A =-25;
						Hysteretic_B1= 50;
						Hysteretic_B2= 50;
						Hysteretic_B3= 50;
						Hysteretic_C = 25;
						break;
				case light_Xenon:
						Hysteretic_A =-25;
						Hysteretic_B1=-50;
						Hysteretic_B2= 50;
						Hysteretic_B3= 50;
						Hysteretic_C = 25;
						break;
				case light_Incand_EHLS_Xenon_Horizon:
						Hysteretic_A =-25;
						Hysteretic_B1=-50;
						Hysteretic_B2=-50;
						Hysteretic_B3= 50;
						Hysteretic_C = 25;
						break;
				case light_Incand_EHLS:
						Hysteretic_A =-25;
						Hysteretic_B1=-50;
						Hysteretic_B2=-50;
						Hysteretic_B3=-50;
						Hysteretic_C = 25;
						break;
				default ://light_Daylight
						Hysteretic_A =-25;
						Hysteretic_B1= 50;
						Hysteretic_B2= 50;
						Hysteretic_B3= 50;
						Hysteretic_C =-25;
						break;
			}
			//-----judge----old light status------end
			if(BG_Ratio < 525+Hysteretic_A) // U30,coolwhite
			{
				luxValue = green *1490/1000;
				lpi->light_status=light_u30_coolwhite;
				light_status_print();light_data_print();
			}
			else
			{
				if(RG_RB_Ratio < 1890 + Hysteretic_B1) //it is Daylight or fluorescence
				{
					if(RG_BG_Ratio < 560 + Hysteretic_C)//  it is fluorescence
					{
						luxValue = green *1490/1000;
						lpi->light_status=light_Fluorescence;
						light_status_print();light_data_print();
					}
					else						//it is Daylight
					{
						luxValue = green * 950/1000 ;
						lpi->light_status=light_Daylight;
						light_status_print();light_data_print();
					}
				}
				else if ((RG_RB_Ratio >= 1890 + Hysteretic_B1)&&(RG_RB_Ratio <= 2340 + Hysteretic_B2))//it is Xenon
				{
					luxValue = green * 1050/1000 ;
					lpi->light_status=light_Xenon;
					light_status_print();light_data_print();
				}
				else if ((RG_RB_Ratio > 2340 + Hysteretic_B2)&&(RG_RB_Ratio <= 2600 + Hysteretic_B3))//it may be EHLS and incand and Xenon,,can't be dis````
				{
					luxValue = green * 700/1000;//include Xenon ,need to bigger
					lpi->light_status=light_Incand_EHLS_Xenon_Horizon;
					light_status_print();light_data_print();
				}
				else //if (RG_RB_Ratio > 2600 + Hysteretic_B3 )//&&(RG_RB_Ratio <= 2820 )// it is incand or EHLS
				{
					luxValue = green * 580/1000;
					lpi->light_status=light_Incand_EHLS;
					light_status_print();light_data_print();
				}
			}
		}
       }
	else if(sensor_tp_type == 1)
	{
		if(green <= 0)
		    luxValue=0;
		else if (green>65534)
			luxValue=10000;
		else
		{
			BG_Ratio = 1000* blue / green;
			RG_Ratio = 1000* red  / green;
			RB_Ratio = 1000* red  / blue;
			RG_RB_Ratio = RG_Ratio * RB_Ratio /1000;
			RG_BG_Ratio = RG_Ratio * BG_Ratio /1000;
			//-----judge----old light status------begin
			switch(lpi->light_status)
			{
				case light_u30_coolwhite:
						Hysteretic_A = 25;
						Hysteretic_B = 50;
						Hysteretic_C =-25;
						Hysteretic_D1= 10;
						Hysteretic_D2= 10;
						Hysteretic_E =-50;
						break;
				case light_Daylight:
						Hysteretic_A =-25;
						Hysteretic_B = 50;
						Hysteretic_C =-25;
						Hysteretic_D1= 10;
						Hysteretic_D2= 10;
						Hysteretic_E =-50;
						break;
				case light_Fluorescence:
						Hysteretic_A =-25;
						Hysteretic_B = 50;
						Hysteretic_C = 25;
						Hysteretic_D1= 10;
						Hysteretic_D2= 10;
						Hysteretic_E =-50;
						break;
				case light_Xenon:
						Hysteretic_A =-25;
						Hysteretic_B =-50;
						Hysteretic_C =-25;
						Hysteretic_D1= 10;
						Hysteretic_D2= 10;
						Hysteretic_E =-50;
						break;
				case light_Incand_EHLS_Xenon_Horizon:
						Hysteretic_A =-25;
						Hysteretic_B =-50;
						Hysteretic_C =-25;
						Hysteretic_D1=-10;
						Hysteretic_D2= 10;
						Hysteretic_E =-50;
						break;
				case light_Incand_EHLS:
						Hysteretic_A =-25;
						Hysteretic_B =-50;
						Hysteretic_C =-25;
						Hysteretic_D1=-10;
						Hysteretic_D2=-10;
						Hysteretic_E =-50;
						break;
				case light_Horizon:
						Hysteretic_A =-25;
						Hysteretic_B =-50;
						Hysteretic_C =-25;
						Hysteretic_D1=-10;
						Hysteretic_D2=-10;
						Hysteretic_E = 50;
						break;
				default ://coolwhite or u30
						Hysteretic_A = 25;
						Hysteretic_B = 50;
						Hysteretic_C =-25;
						Hysteretic_D1= 10;
						Hysteretic_D2= 10;
						Hysteretic_E =-50;	
						break;
			}
			//-----judge----old light status------end
			if(BG_Ratio < 525+Hysteretic_A) // U30,coolwhite  +/-25
			{
				luxValue = green *1220/1000;
				lpi->light_status=light_u30_coolwhite;
				light_status_print();light_data_print();
			}
			else
			{
				if(RG_RB_Ratio < 2100 + Hysteretic_B) //it is Daylight or fluorescence  +/-50
				{
					if(RG_BG_Ratio < 625 + Hysteretic_C)//  it is fluorescence +/-25
					{
						luxValue = green *1340/1000;
						lpi->light_status=light_Fluorescence;
						light_status_print();light_data_print();
					}
					else						//it is Daylight   +/-25
					{
						luxValue = green * 830/1000 ;
						lpi->light_status=light_Daylight;
						light_status_print();light_data_print();
					}
				}
				else    //it may be EHLS and incand and Xenon and horizon,,can't be dis````  +/-50
				{
					if (RG_BG_Ratio <= 880 + Hysteretic_D1)//it is Xenon +/-10
					{
						luxValue = green * 970/1000 ;
						lpi->light_status=light_Xenon;
						light_status_print();light_data_print();
					}
					else if ((RG_BG_Ratio > 880 + Hysteretic_D1)&&(RG_BG_Ratio <= 920 + Hysteretic_D2))//it may be EHLS and incand and Xenon,,can't be dis````//  +/-10
					{
						luxValue = green * 780/1000;//include Xenon ,need to bigger
						lpi->light_status=light_Incand_EHLS_Xenon_Horizon;
						light_status_print();light_data_print();
					}
					else //if (RG_BG_Ratio > 920 + Hysteretic_D2) // it is incand or EHLS or horizon  +/-10
					{
						if(RG_RB_Ratio < 3100 + Hysteretic_E)//  it is Horizon +/-50
						{
							luxValue = green *410/1000;
							lpi->light_status=light_Horizon;
							light_status_print();light_data_print();
						}
						else						//it is Incand_EHLS  +/-50
						{
							luxValue = green * 480/1000 ;
							lpi->light_status=light_Incand_EHLS;
							light_status_print();light_data_print();
						}
					}
				}

		}

		}
	}
	else
	{
		if(green <= 0)
		    luxValue=0;
		else if (green>65534)
			luxValue=10000;
		else
		{
			BG_Ratio = 1000* blue / green;
			RG_Ratio = 1000* red  / green;
			RG_BG_Ratio = RG_Ratio * BG_Ratio /1000;
			if(RG_BG_Ratio<400)
				gain=2837;
			else if(RG_BG_Ratio<500)
				gain=(500-RG_BG_Ratio)*693/100+2143;
			else if(RG_BG_Ratio<650)
				gain=2143;
			else if(RG_BG_Ratio<740)
				gain=(740-RG_BG_Ratio)*657/100+1487;
			else if(RG_BG_Ratio<950)
				gain=1487;
			else
				gain=987;
			luxValue=green*gain/1000;
		}
	}
	if (luxValue >10000)
		luxValue=10000;
	if (luxValue < 0)
		luxValue=0;
#if 0
	input_report_rel(lpi->ls_input_dev, REL_RED, red);
	input_report_rel(lpi->ls_input_dev, REL_GREEN, green);
	input_report_rel(lpi->ls_input_dev, REL_BLUE, blue);
	input_sync(lpi->ls_input_dev);
#endif
	color_value = CCT;
	if( als_polling_count <5 )
	{
		if(luxValue == 10000)
			luxValue = luxValue - als_polling_count%2;
		else
			luxValue = luxValue + als_polling_count%2;
		als_polling_count++;
	}
	if ( als_polling_count >2 )
	{
	input_report_abs(lpi->ls_input_dev, ABS_MISC, luxValue); // report the lux level
	input_sync(lpi->ls_input_dev);
	}
	mutex_unlock(&als_get_value_mutex);
}

static void report_do_work(struct work_struct *work)
{
	struct cm3320_info *lpi = lp_info;

	if (enable_log)
		D("[CM3320] %s\n", __func__);

	report_lsensor_input_event(lpi, 0);
	if ( als_polling_count >2 )
	{	queue_delayed_work(lpi->lp_wq, &report_work, lpi->polling_delay);}
	else
	{	queue_delayed_work(lpi->lp_wq, &report_work, 200);}
}

static int als_power(int enable)
{
	struct cm3320_info *lpi = lp_info;

	if (lpi->power)
		lpi->power(LS_PWR_ON, 1);

	return 0;
}
//add
static ssize_t cm3320_show_enable_als_sensor(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm3320_info *lpi = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", lpi->als_enable);
}

static ssize_t cm3320_store_enable_als_sensor(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm3320_info *lpi = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	unsigned long flags;
	als_polling_count = 0;
	printk("%s: enable als sensor ( val = %ld)\n", __func__, val);

	if ((val != 0) && (val != 1))
	{
		dev_dbg(dev,"%s: enable als sensor=%ld\n", __func__, val);
		return count;
	}

	if(val == 1) {
		lightsensor_enable(lpi);
	}
	else{
		lightsensor_disable(lpi);
	}
	return count;
}

static DEVICE_ATTR(enable_als_sensor, S_IWUSR| S_IRUGO,
						cm3320_show_enable_als_sensor, cm3320_store_enable_als_sensor);


static ssize_t cm3320_show_als_poll_delay(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm3320_info *lpi  = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", lpi->polling_delay);	// return in micro-second
}

static ssize_t cm3320_store_als_poll_delay(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t count)
{

       struct i2c_client *client = to_i2c_client(dev);
	struct cm3320_info *lpi  = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;
	int poll_delay=0;

	if (val<200000)
		val = 200000;	// minimum 200ms

	lpi->polling_delay =  msecs_to_jiffies(val/1000);	// convert us => ms

	return count;

}

static DEVICE_ATTR(als_poll_delay, S_IWUSR|S_IRUGO,
						cm3320_show_als_poll_delay, cm3320_store_als_poll_delay);

static ssize_t cm3320_show_als_rgb_value(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm3320_info *lpi  = i2c_get_clientdata(client);
#if 1

	rgb_data[0] = (uint8_t)color_data[0] ;
	rgb_data[1] = (uint8_t)(color_data[0] >> 8) ;
	rgb_data[2] = (uint8_t)color_data[1] ;
	rgb_data[3] = (uint8_t)(color_data[1] >> 8) ;
	rgb_data[4] = (uint8_t)color_data[2] ;
	rgb_data[5] = (uint8_t)(color_data[2] >> 8) ;

	memcpy(buf, rgb_data, RGB_DATA_SIZE);
	return RGB_DATA_SIZE;

#endif

}


static DEVICE_ATTR(als_rgb_value, S_IWUSR|S_IRUGO,
						cm3320_show_als_rgb_value, NULL);

static ssize_t cm3320_show_als_color_value(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm3320_info *lpi  = i2c_get_clientdata(client);
	return sprintf(buf, "%d\n", color_value);

}


static DEVICE_ATTR(als_color_value, S_IWUSR|S_IRUGO,
						cm3320_show_als_color_value, NULL);

static struct attribute *cm3320_attributes[] = {
	&dev_attr_enable_als_sensor.attr,
	&dev_attr_als_poll_delay.attr,
	&dev_attr_als_rgb_value.attr,
	&dev_attr_als_color_value.attr,
	NULL};
static const struct attribute_group cm3320_attr_group = {
	.attrs = cm3320_attributes,
	};
//end
static int lightsensor_enable(struct cm3320_info *lpi)
{
	int ret = 0;
	uint8_t cmd = 0;

	mutex_lock(&als_enable_mutex);
	D("[LS][CM3320] %s\n", __func__);

	cmd = ( CM3320_CMD1_DEFAULT | CM3320_CMD1_CR);
	ret = cm3320_I2C_Write_Byte(CM3320_W_CMD1_addr, cmd);
	if (ret < 0)
		pr_err(
		"[LS][CM3320 error]%s: set auto light sensor fail\n",
		__func__);
	else {
		//msleep(600);/*wait for 600 ms for the first report*/
		lpi->als_enable = 1;
	}
	queue_delayed_work(lpi->lp_wq, &report_work, 200);
	lpi->als_enable = 1;
	mutex_unlock(&als_enable_mutex);

	return ret;
}

static int lightsensor_disable(struct cm3320_info *lpi)
{
	int ret = 0;
	char cmd = 0;

	mutex_lock(&als_disable_mutex);

	D("[LS][CM3320] %s\n", __func__);

	cmd = (CM3320_CMD1_DEFAULT | CM3320_CMD1_CR | CM3320_CMD1_SD);
	ret = cm3320_I2C_Write_Byte(CM3320_W_CMD1_addr, cmd);
	if (ret < 0)
		pr_err("[LS][CM3320 error]%s: disable auto light sensor fail\n",
			__func__);
	else {
		lpi->als_enable = 0;
	}
	cancel_delayed_work(&report_work);
	lpi->als_enable = 0;
	mutex_unlock(&als_disable_mutex);

	return ret;
}

#if 0

static long lightsensor_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int rc, val;
	struct cm3320_info *lpi = lp_info;

	/*D("[CM3320] %s cmd %d\n", __func__, _IOC_NR(cmd));*/

	switch (cmd) {
	case LIGHTSENSOR_IOCTL_ENABLE:
		if (get_user(val, (unsigned long __user *)arg)) {
			rc = -EFAULT;
			break;
		}
		D("[LS][CM3320] %s LIGHTSENSOR_IOCTL_ENABLE, value = %d\n",
			__func__, val);
		rc = val ? lightsensor_enable(lpi) : lightsensor_disable(lpi);
		break;
	case LIGHTSENSOR_IOCTL_GET_ENABLED:
		val = lpi->als_enable;
		D("[LS][CM3320] %s LIGHTSENSOR_IOCTL_GET_ENABLED, enabled %d\n",
			__func__, val);
		rc = put_user(val, (unsigned long __user *)arg);
		break;
	default:
		pr_err("[LS][CM3320 error]%s: invalid cmd %d\n",
			__func__, _IOC_NR(cmd));
		rc = -EINVAL;
	}

	return rc;
}

#endif
static int lightsensor_setup(struct cm3320_info *lpi)
{
	int ret;

	lpi->ls_input_dev = input_allocate_device();
	if (!lpi->ls_input_dev) {
		pr_err(
			"[LS][CM3320 error]%s: could not allocate ls input device\n",
			__func__);
		return -ENOMEM;
	}
	//lpi->ls_input_dev->name = "cm3320-ls";
//add 20120618
	lpi->ls_input_dev->name = "light sensor";
	lpi->ls_input_dev->id.bustype = BUS_I2C;
	lpi->ls_input_dev->dev.parent = &lpi->i2c_client->dev;
//end add
	input_set_drvdata(lpi->ls_input_dev, lpi);
	input_set_capability(lpi->ls_input_dev, EV_REL, REL_RED);
	input_set_capability(lpi->ls_input_dev, EV_REL, REL_GREEN);
	input_set_capability(lpi->ls_input_dev, EV_REL, REL_BLUE);

	set_bit(EV_ABS, lpi->ls_input_dev->evbit);
	input_set_abs_params(lpi->ls_input_dev, ABS_MISC, 0, 10000, 0, 0);

	ret = input_register_device(lpi->ls_input_dev);
	if (ret < 0) {
		pr_err("[LS][CM3320 error]%s: can not register ls input device\n",
				__func__);
		goto err_free_ls_input_device;
	}
//del
#if 0
	ret = misc_register(&lightsensor_misc);
	if (ret < 0) {
		pr_err("[LS][CM3320 error]%s: can not register ls misc device\n",
				__func__);
		goto err_unregister_ls_input_device;
	}
#endif

	return ret;

err_unregister_ls_input_device:
	input_unregister_device(lpi->ls_input_dev);
err_free_ls_input_device:
	input_free_device(lpi->ls_input_dev);
	return ret;
}

static int cm3320_setup(struct cm3320_info *lpi)
{
	int ret = 0;
	int RED,GREEN,BLUE;

	als_power(1);
	msleep(5);

	// Initialize command register 1
	ret = cm3320_I2C_Write_Byte(CM3320_W_CMD1_addr, CM3320_CMD1_DEFAULT | CM3320_CMD1_CR | CM3320_CMD1_SD);
	if(ret < 0)
		return ret;

	// Initialize command register 2
	ret = cm3320_I2C_Write_Byte(CM3320_W_CMD2_addr, CM3320_CMD2_DEFAULT);
	if(ret < 0)
		return ret;

	// Initialize command register 3
	ret = cm3320_I2C_Write_Byte(CM3320_W_CMD3_addr, CM3320_CMD3_B_IT_2T | CM3320_CMD3_G_IT_2T | CM3320_CMD3_R_IT_2T);

	msleep(20);

	// Get initial RED light data
	ret = cm3320_read_light_data(RED, &(lpi->red[0]));
	if (ret < 0) {
		pr_err(
			"[LS][CM3320 error]%s: cm3320_read_light_data for RED fail\n",
			__func__);
		return -EIO;
	}
	lpi->red[1] = lpi->red[0];
	lpi->red[2] = lpi->red[0];

	// Get initial GREEN light data
	ret = cm3320_read_light_data(GREEN, &(lpi->green[0]));
	if (ret < 0) {
		pr_err(
			"[LS][CM3320 error]%s: cm3320_read_light_data for GREEN fail\n",
			__func__);
		return -EIO;
	}
	lpi->green[1] = lpi->green[0];
	lpi->green[2] = lpi->green[0];

	// Get initial BLUE light data
	ret = cm3320_read_light_data(BLUE, &(lpi->blue[0]));
	if (ret < 0) {
		pr_err(
			"[LS][CM3320 error]%s: cm3320_read_light_data for BLUE fail\n",
			__func__);
		return -EIO;
	}
	lpi->blue[1] = lpi->blue[0];
	lpi->blue[2] = lpi->blue[0];

	lpi->index = 0;

	return ret;
}


static void cm3320_early_suspend(struct early_suspend *h)
{
	struct cm3320_info *lpi = lp_info;

	D("[LS][CM3320] %s\n", __func__);

	if (lpi->als_enable)
		lightsensor_disable(lpi);
}

static void cm3320_late_resume(struct early_suspend *h)
{
	struct cm3320_info *lpi = lp_info;

	D("[LS][CM3320] %s\n", __func__);

	if (!lpi->als_enable)
		lightsensor_enable(lpi);
}

static int cm3320_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct cm3320_info *lpi;
	struct cm3320_platform_data *pdata;

	D("[CM3320] %s\n", __func__);

	lpi = kzalloc(sizeof(struct cm3320_info), GFP_KERNEL);
	if (!lpi)
		return -ENOMEM;

	//add test
	if(true == get_rgb_sensor_type()){
		ret = set_sensor_chip_info(ALS, "CAPELLA CM3320 ");
              if (ret) {
                      dev_err(&client->dev, "set_sensor_chip_info error\n");
              }
		printk("%s:now we use rgb_sensor for lux !\n",__func__);
	}
	else{
		return ret;
	}

	/*D("[CM3320] %s: client->irq = %d\n", __func__, client->irq);*/

	lpi->i2c_client = client;
#if 0
	pdata = client->dev.platform_data;
	if (!pdata) {
		pr_err("[CM3320 error]%s: Assign platform_data error!!\n",
			__func__);
		ret = -EBUSY;
		goto err_platform_data_null;
	}
#endif
//del 20120618
	//lpi->irq = client->irq;

	i2c_set_clientdata(client, lpi);
	//lpi->power = pdata->power;

	lpi->polling_delay = msecs_to_jiffies(LS_POLLING_DELAY);

	lp_info = lpi;

	mutex_init(&als_enable_mutex);
	mutex_init(&als_disable_mutex);
	mutex_init(&als_get_value_mutex);


	lpi->lp_wq = create_singlethread_workqueue("cm3320_wq");
	if (!lpi->lp_wq) {
		pr_err("[CM3320 error]%s: can't create workqueue\n", __func__);
		ret = -ENOMEM;
		goto err_create_singlethread_workqueue;
	}

	ret = cm3320_setup(lpi);
	if (ret < 0) {
		pr_err("[ERR][CM3320 error]%s: cm3320_setup error!\n", __func__);
		goto err_cm3320_setup;
	}
	ret = lightsensor_setup(lpi);
	if (ret < 0) {
		pr_err("[LS][CM3320 error]%s: lightsensor_setup error!!\n",
			__func__);
		goto err_lightsensor_setup;
	}

	lpi->cm3320_class = class_create(THIS_MODULE, "optical_sensors");
	if (IS_ERR(lpi->cm3320_class)) {
		ret = PTR_ERR(lpi->cm3320_class);
		lpi->cm3320_class = NULL;
		goto err_create_class;
	}

	lpi->ls_dev = device_create(lpi->cm3320_class,
				NULL, 0, "%s", "lightsensor");
	if (unlikely(IS_ERR(lpi->ls_dev))) {
		ret = PTR_ERR(lpi->ls_dev);
		lpi->ls_dev = NULL;
		goto err_create_ls_device;
	}

	/* register the attributes */
//del
#if 0
	ret = device_create_file(lpi->ls_dev, &dev_attr_ls_adc);
	if (ret)
		goto err_create_ls_device_file;

	/* register the attributes */
	ret = device_create_file(lpi->ls_dev, &dev_attr_ls_auto);
	if (ret)
		goto err_create_ls_device_file;
#endif
//add sysfs creat
/* Register sysfs hooks */
	ret = sysfs_create_group(&client->dev.kobj, &cm3320_attr_group);
	if (ret)
		goto err_create_ls_device;
    if(true == get_rgb_sensor_type()) {
	    ret = set_sensor_input(ALS, lpi->ls_input_dev->dev.kobj.name);
	    if (ret) {
		   sysfs_remove_group(&client->dev.kobj, &cm3320_attr_group);
		   printk("%s, set_sensor_input als error: %s\n",
		          __func__, lpi->ls_input_dev->name);
		   goto err_create_ls_device;
	    }
    }
//end add
	lpi->early_suspend.level =
			EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	lpi->early_suspend.suspend = cm3320_early_suspend;
	lpi->early_suspend.resume = cm3320_late_resume;
	register_early_suspend(&lpi->early_suspend);

	lpi->als_enable = 0;
	D("[CM3320] %s: Probe success!\n", __func__);

	return ret;

//err_create_ls_device_file:
//	device_unregister(lpi->ls_dev);
err_create_ls_device:
	class_destroy(lpi->cm3320_class);
err_create_class:
	input_unregister_device(lpi->ls_input_dev);
	input_free_device(lpi->ls_input_dev);
err_lightsensor_setup:
err_cm3320_setup:
	destroy_workqueue(lpi->lp_wq);
	mutex_destroy(&als_enable_mutex);
	mutex_destroy(&als_disable_mutex);
	mutex_destroy(&als_get_value_mutex);
err_create_singlethread_workqueue:
//err_platform_data_null:
//	kfree(lpi);
	return ret;
}

static const struct i2c_device_id cm3320_i2c_id[] = {
	{CM3320_I2C_NAME, 0},
	{}
};

static struct i2c_driver cm3320_driver = {
	.id_table = cm3320_i2c_id,
	.probe = cm3320_probe,
	.driver = {
	.name = CM3320_I2C_NAME,
	.owner = THIS_MODULE,
	},
};

static int __init cm3320_init(void)
{
	return i2c_add_driver(&cm3320_driver);
}

static void __exit cm3320_exit(void)
{
	i2c_del_driver(&cm3320_driver);
}

module_init(cm3320_init);
module_exit(cm3320_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RGB sensor device driver for CM3320");
MODULE_AUTHOR("Capella Microsystems");


