/*
 * Copyright (c) 2011 Hisilicon Technologies Co., Ltd. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <linux/hkadc/hiadc_linux.h>
#include <linux/hkadc/hiadc_hal.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/input.h>
#include <linux/time.h>
#include <linux/wakelock.h>
#include <linux/syscalls.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <mach/hardware.h>

#include <linux/thermal_framework.h>
#include <linux/temperature_sensor.h>

#define PRORATE_OF_INIT	1000 /*in order to resolve divisor less than zero*/
#define multiple				100
#define R1003				1500/*ohm*/
#define R1006				27400/*ohm*/
#define  RT1008				150000/*ohm*/
#define AVDD				33/*add to both of  hot resistance's voltage *  10 */
#define BITNUM				4096
#define CURRENT				1/*I * 10 */
#define NCP_XH103J_NUM		25
#define ADC_RTMP_FOR_AP	0x06
#define ADC_RTMP_FOR_SIM	0x02
#define NTC_NCP150K_NUM	25

#define ADC_AP_RISISTANCE_MAX_VALUE	27218
#define ADC_AP_RISISTANCE_MIN_VALUE	596
#define ADC_SIM_RISISTANCE_MAX_VALUE	572667
#define ADC_SIM_RISISTANCE_MIN_VALUE	3454
#define ADC_AP_RTMP_DEFAULT_VALUE	861/*base default value for ap temp*/
#define ADC_SIM_RTMP_DEFAULT_VALUE	1100/*base default value for sim temp*/

/*hi3620 IC temperature from 0 to 120*/
static int ncpxh103j_volt_to_temp[NCP_XH103J_NUM][2] = {
	{596, 120}, {671, 115}, {758, 110}, {858, 105}, {973, 100},
	{1109, 95}, {1268, 90}, {1452, 85}, {1668, 80}, {1924, 75},
	{2227, 70}, {2586, 65}, {3014, 60}, {3535, 55}, {4160, 50},
	{4916, 45}, {5833, 40}, {6947, 35}, {8314, 30}, {10500, 25},
	{12080, 20}, {14673, 15}, {17925, 10}, {22021, 5}, {27218, 0},
};

/*around SIM card temperature from 0 to 120*/
static int ncp150k_volt_to_temp[NTC_NCP150K_NUM][2] = {
	{3454, 120}, {4052, 115}, {4772, 110}, {5639, 105}, {6688, 100},
	{7966, 95}, {9520, 90}, {11428, 85}, {13776, 80}, {16679, 75},
	{20293, 70}, {24804, 65}, {30453, 60}, {37605, 55}, {46665, 50},
	{58240, 45}, {73090, 40}, {92293, 35}, {117281, 30}, {150000, 25},
	{193166, 20}, {250538, 15}, {327405, 10}, {431264, 5}, {572667, 0},
};
/****************************************************
  Function:       int get_ntc_temperature_volt_adc()
  Description:   get different channel ntc's ADC value.
  Input:channel:0x06 is ADC_RTMP; 0x02 is ADC_ADCIN3
  Output:         NA
  Return:         value    : different channel ADC value
  Others:          NA
*****************************************************/

static int get_ntc_temperature_volt_adc(int channel)
{
	int ret = 0, value = 0;

	if ((channel == ADC_RTMP_FOR_AP) || (channel == ADC_RTMP_FOR_SIM)) {
		ret = k3_adc_open_channel(channel);
		if (ret < 0) {
			pr_err("k3v2 adc open channel fail!\n");
			return ret;
		}
		if (channel == ADC_RTMP_FOR_AP)
			msleep(10);
		value = k3_adc_get_value(channel, NULL);
		if (value < 0)
			pr_err("error value!\n");

		ret = k3_adc_close_channal(channel);
		if (ret < 0) {
			return ret;
		}
	} else {
		pr_err("get_ntc_temperature_volt_adc fail!\n\r");
	}

	return value;
}


/****************************************************
  Function:       int change_volt_to_temp()
  Description: volt to temp
  Input:
  Output:         NA
  Return:         get temperature
  Others:          NA
*****************************************************/
static int change_volt_to_temp(int iResistance, int index_min, int index_max, int channel)
{
	int prorate = 0;
	int resistance_max = 0;
	int resistance_min = 0;
	int temper_top = 0;
	int temper_bottom = 0;
	int itemper = 0;

	switch (channel) {
	case ADC_RTMP_FOR_AP:
		resistance_min = ncpxh103j_volt_to_temp[index_min][0];
		resistance_max = ncpxh103j_volt_to_temp[index_max][0];
		temper_bottom = ncpxh103j_volt_to_temp[index_min][1];
		temper_top = ncpxh103j_volt_to_temp[index_max][1];
		break;
	case ADC_RTMP_FOR_SIM:
		resistance_min = ncp150k_volt_to_temp[index_min][0];
		resistance_max = ncp150k_volt_to_temp[index_max][0];
		temper_bottom = ncp150k_volt_to_temp[index_min][1];
		temper_top = ncp150k_volt_to_temp[index_max][1];
		break;
	default:
		pr_err("ADC channel is not exist!\n\r");
		break;
	}

	prorate =  ((resistance_max - iResistance) * PRORATE_OF_INIT) / (resistance_max - resistance_min);
	itemper = ((temper_bottom - temper_top) * prorate) / PRORATE_OF_INIT + temper_top;
	return itemper;
}

/****************************************************************************
  Function:       int get_ap_ntc_temperature(int iResistance)
  Description:    dichotomy
  Input:          NA
  Output:         NA
  Return:         return temperature
  Others:         NA
*****************************************************************************/
static int get_ntc_temperature(int iResistance, int channel, int **volt_to_temperature)
{
	int iLow = 0;
	int iUpper = 0 ;
	int temperature = 0;
	int iMid = 0;
	int *temp_idex = (int *)volt_to_temperature;

	if (ADC_RTMP_FOR_AP == channel) {
		iUpper = sizeof(ncpxh103j_volt_to_temp) / sizeof(ncpxh103j_volt_to_temp[0][0]) / 2;
	} else if (ADC_RTMP_FOR_SIM == channel) {
		iUpper = sizeof(ncp150k_volt_to_temp) / sizeof(ncp150k_volt_to_temp[0][0]) / 2;
	}

	if (iResistance < *(temp_idex + 2 * 0 + 0)) {
		temperature = *(temp_idex + 2 * 0 + 1);
		return temperature;
	} else if (iResistance > *(temp_idex + 2 * (iUpper - 1) + 0)) {
		temperature = *(temp_idex + 2 * (iUpper - 1) + 1);
		return temperature;
	}

	while (iLow <= iUpper) {
		iMid = (iLow + iUpper) / 2;
		if (*(temp_idex + 2 * iMid + 0) > iResistance) {
			if (*(temp_idex + 2 * (iMid - 1) + 0) < iResistance) {
				temperature = change_volt_to_temp(iResistance,
						(iMid - 1), iMid, channel);
				return temperature;
			}
			iUpper = iMid - 1;
		} else if (*(temp_idex + 2 * iMid + 0) < iResistance) {
			if (*(temp_idex + 2 * (iMid + 1) + 0) > iResistance) {
				temperature = change_volt_to_temp(iResistance,
					iMid, (iMid + 1), channel);
			return temperature;
			}
			iLow = iMid + 1;
		} else {
			iUpper = iMid;
			temperature = *(temp_idex + 2 * iMid + 1);
			break;
		}
	}
	return temperature;
}

/****************************************************************************
  Function:       int getcalctemperature(int BatteryTemperature)
  Description:    get temperature value
  Calls:          NA
  Data Accessed:  NA
  Data Updated:   NA
  Input:          NA
  Output:         NA
  Return:         vol to temp
  Others:         NA
*****************************************************************************/
int getcalctemperature(int channel)
{
	int tempdata = 0;
	int Volt = 0;
	int Resistance1 = 0;
	int Resistance2 = 0;
	int adc_value = 0;

	switch (channel) {
	case ADC_RTMP_FOR_AP:
		adc_value = get_ntc_temperature_volt_adc(channel);
		if (adc_value <= 0) {
			pr_err("AP get ADC value is fail,adc_value[%d]!\n\r", adc_value);
			adc_value = ADC_AP_RTMP_DEFAULT_VALUE;/*if read error, return default value*/
		}
		Volt = (AVDD * adc_value * multiple) / BITNUM;
		Resistance1 = (Volt / CURRENT * 10) - R1003;
		Resistance2 = (Resistance1 * R1006) / (R1006 - Resistance1);
		tempdata = get_ntc_temperature(Resistance2, channel, ncpxh103j_volt_to_temp);
		break;
	case ADC_RTMP_FOR_SIM:
		adc_value = get_ntc_temperature_volt_adc(channel);
		if (adc_value <= 0) {
			pr_err("SIM get ADC value is fail,adc_value[%d]!\n\r", adc_value);
			adc_value = ADC_SIM_RTMP_DEFAULT_VALUE;/*if read error, return default value*/
		}
		Volt = (AVDD * adc_value * multiple) / BITNUM;
		Resistance1 = (Volt * RT1008) / (AVDD * multiple - Volt);
		tempdata = get_ntc_temperature(Resistance1, channel, ncp150k_volt_to_temp);
		break;
	default:
		pr_err("ADC channel is not exist!\n\r");
		break;
	}

	return tempdata;
}

