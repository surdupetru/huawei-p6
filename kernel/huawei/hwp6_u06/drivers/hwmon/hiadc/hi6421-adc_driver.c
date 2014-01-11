/*******************************************************************************
 * Copyright:   Copyright (c) 2011. Hisilicon Technologies, CO., LTD.
 * Version:     V200
 * Filename:    hiadc_driver.c
 * Description: hiadc_driver.c
 * History:     1.Created by 00186176 on 2011/08/02
*******************************************************************************/
#include <linux/hkadc/hiadc_hal.h>
#ifndef BOOT_MOD
#include <linux/hkadc/hiadc_linux.h>
#else
#include <linux/hkadc/hiadc_boot.h>
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <linux/delay.h>
#include <linux/semaphore.h>

extern struct adc_device *adc_dev;

static void k3_adc_bypass(void)
{
	unsigned char write_reg  = 0;
	/*configure SETPU register, BYPASS ADC, enable clock, choose BIAS, disable POWERUP, disable BUFF*/
	/*enable tc mode*/
	write_reg = (unsigned char)(HKADC_BYPASS_YES);
	k3_adc_reg_write(HKADC_SETUP_ADDR, write_reg, 0x0);
}

static void k3_adc_reset(void)
{
	unsigned char write_reg  = 0;

	write_reg     = 0x99;
	k3_adc_reg_write(REG_RET_ADDR, write_reg, 0x0);

	write_reg     = 0xAA;
	k3_adc_reg_write(REG_RET_ADDR, write_reg, 0x0);

	return;
}

static int k3_adc_wait_event(int clock)
{
	unsigned char   read_reg = 0;

	k3_adc_reg_read(HKADC_BUSY_ADDR, &read_reg, 1);
	if (!(HKADC_AD_BUSY_NO & read_reg)) {
		if (clock == HKADC_CK_SEL_FOUR)
			k3_adc_msleep_range(WAIT_TIME_RANGE_FOR_100K_MIN, WAIT_TIME_RANGE_FOR_100K_MAX);
		else if (clock == HKADC_CK_SEL_THREE)
			k3_adc_msleep_range(WAIT_TIME_RANGE_FOR_200K_MIN, WAIT_TIME_RANGE_FOR_200K_MAX);
		else if (clock == HKADC_CK_SEL_TWO)
			k3_adc_msleep_range(WAIT_TIME_RANGE_FOR_1M_MIN, WAIT_TIME_RANGE_FOR_1M_MAX);

		k3_adc_reg_read(HKADC_BUSY_ADDR, &read_reg, 1);

		if (!(HKADC_AD_BUSY_NO & read_reg)) {
			k3_adc_reset();
			k3_adc_bypass();
			k3_adc_print_error("HKADC BUSY!");
			return ADC_ERROR;
		} else {
			return ADC_SUCCESS;
		}
	}

	return ADC_SUCCESS;
}

static unsigned short k3_adc_calc_vol(unsigned short data, int ch)
{
	unsigned short rt = 0;
	int tmp_data = data;
	int mod = 0;
	struct adc_data *adc_table = NULL;

	adc_table = ((struct adc_dataEx *)(adc_dev->pdev->dev.platform_data))->data;
	if (!adc_table) {
		rt = ADC_ARG_ERROR;
		k3_adc_print_error("CLIENT IS NULL!");
		return rt;
	}

    if (ADC_VOLTAGE_MOD1 == adc_table[ch].vol){
        mod = VOLTAGE_MOD1;
    }else if (ADC_VOLTAGE_MOD2 == adc_table[ch].vol){
        mod = VOLTAGE_MOD2;
    }else{
        mod = VOLTAGE_MOD3;
    }

	rt = (unsigned short)(((tmp_data * mod) >> CLAC_SHIFT));

	return rt;
}

static int k3_adc_convert(unsigned short *buf_out, int flag)
{
	int     rt                  = ADC_SUCCESS;
	/*ADC data*/
	unsigned char  read_reg_adc[2] = {0, 0};
	unsigned char  write_reg     = 0;
	unsigned short tmp_data      = 0;
	unsigned short sum = 0;

	if (!buf_out) {
		rt = ADC_ARG_ERROR;
		k3_adc_print_error("BUF IS OUT!");
		return rt;
	}

	/*configure HKADC_STARTEN18,1.*/
	write_reg = (unsigned char)(HKADC_STARTEN_YES | HKADC_START_NO);
	k3_adc_reg_write(HKADC_CONV_START_ADDR, write_reg, 0x0);

	k3_adc_usleep(10);

	/*disable HKADC_START*/
	write_reg = (unsigned char)(HKADC_START_YES | HKADC_STARTEN_YES);
	k3_adc_reg_write(HKADC_CONV_START_ADDR, write_reg, 0x0);

	/*wait adc complete*/
	if (ADC_SUCCESS != k3_adc_wait_event(flag)) {
		rt = ADC_ERROR;
		return rt;
	}

	k3_adc_reg_read(HKADC_DATA1_ADDR, read_reg_adc, 2);
	tmp_data = read_reg_adc[0] << 4;
	tmp_data = tmp_data | ((0xf)&read_reg_adc[1]);
	sum = sum + tmp_data;

	buf_out[0] = sum;
	return rt;
}

static int k3_adc_is_valid_ch(unsigned int ch)
{
	int ret = ADC_SUCCESS;

	switch (ch) {
	case ADC_VBATMON:
	case ADC_VCOINMON:
	case ADC_NC0:
	case ADC_RTMP:
	case ADC_PB_DETECT:
	case ADC_500K:
		break;

	case ADC_ADCIN1:
	case ADC_ADCIN2:
	case ADC_ADCIN3:
		break;

	default:
		ret = ADC_CMD_ERROR;
		break;
	}
	return ret;
}

int k3_adc_open_channel(int ch)
{
	unsigned char   write_reg = 0;
	struct adc_data *adc_table = NULL;
	int rt = ADC_SUCCESS;

	if (k3_adc_is_valid_ch(ch)) {
		rt = ADC_CMD_ERROR;
		k3_adc_print_error("CHANGNEL IS ERROR!");
		return rt;
	}

	k3_adc_lock();

	if (!adc_dev) {
		rt = ADC_ARG_ERROR;
		k3_adc_print_error("adc_dev IS NULL!");
		k3_adc_release();
		return rt;
	}

	if (!adc_dev->pdev->dev.platform_data) {
		rt = ADC_ARG_ERROR;
		k3_adc_print_error("adc_dev IS NULL!");
		k3_adc_release();
		return rt;
	}

	adc_table = ((struct adc_dataEx *)(adc_dev->pdev->dev.platform_data))->data;

	if (!adc_table) {
		rt = ADC_ARG_ERROR;
		k3_adc_print_error("adc_table IS NULL!");
		k3_adc_release();
		return rt;
	}

	rt = clk_enable(adc_dev->clk);
	if (rt) {
		k3_adc_print_error("hi6421 pmu clk enable failed\n");
		rt = ADC_ARG_ERROR;
		k3_adc_release();
		return rt;
	}

	if (!(adc_table[ch].count) && (ch == ADC_RTMP)) {
		write_reg = (unsigned char)(EN_RTMP);
		k3_adc_reg_write(OTHER_CTRL_REG_ADDR, write_reg, ~(0x04));
	}

	adc_table[ch].count++;

	clk_disable(adc_dev->clk);

	k3_adc_release();

	return rt;
}

int k3_adc_close_channal(int ch)
{
	unsigned char   write_reg = 0;
	struct adc_data *adc_table = NULL;
	int rt = ADC_SUCCESS;



	if (k3_adc_is_valid_ch(ch)) {
		rt = ADC_CMD_ERROR;
		k3_adc_print_error("CHANGNEL IS ERROR!");
		return rt;
	}
	k3_adc_lock();
	if (!adc_dev) {
		rt = ADC_ARG_ERROR;
		k3_adc_print_error("adc_dev IS NULL!");
		k3_adc_release();
		return rt;
	}

	if (!adc_dev->pdev->dev.platform_data) {
		rt = ADC_ARG_ERROR;
		k3_adc_print_error("adc_dev IS NULL!");
		k3_adc_release();
		return rt;
	}

	adc_table = ((struct adc_dataEx *)(adc_dev->pdev->dev.platform_data))->data;

	if (!adc_table) {
		rt = ADC_ARG_ERROR;
		k3_adc_print_error("adc_table IS NULL!");
		k3_adc_release();
		return rt;
	}

	rt = clk_enable(adc_dev->clk);
	if (rt) {
		k3_adc_print_error("hi6421 pmu clk enable failed\n");
		rt = ADC_ARG_ERROR;
		k3_adc_release();
		return rt;
	}

	adc_table[ch].count--;

	if (adc_table[ch].count < 0)
		adc_table[ch].count = 0;

	if (!(adc_table[ch].count) && (ch == ADC_RTMP)) {
		write_reg = (unsigned char)(DIS_RTMP);
		k3_adc_reg_write(OTHER_CTRL_REG_ADDR, write_reg, ~(0x04));
	}

	clk_disable(adc_dev->clk);

	k3_adc_release();

	return rt;
}

int k3_adc_get_value(unsigned int ch, unsigned char *reserve)
{
	int             rt       = ADC_SUCCESS;
	unsigned char   write_reg = 0;
	unsigned short  buf[2]   = {0, 0};
	unsigned short  adc_value   = 0;
	struct adc_data *adc_table = NULL;



	if (k3_adc_is_valid_ch(ch)) {
		rt = ADC_CMD_ERROR;
		k3_adc_print_error("CHANGNEL IS ERROR!");
		return rt;
	}

	k3_adc_lock();
	if (!adc_dev) {
		rt = ADC_ARG_ERROR;
		k3_adc_print_error("adc_dev IS NULL!");
		k3_adc_release();
		return rt;
	}

	adc_table = ((struct adc_dataEx *)(adc_dev->pdev->dev.platform_data))->data;

	if (!adc_table) {
		rt = ADC_ARG_ERROR;
		k3_adc_print_error("adc_dev->pdev IS NULL!");
		k3_adc_release();
		return rt;
	}

	rt = clk_enable(adc_dev->clk);
	if (rt) {
		k3_adc_print_error("hi6421 pmu clk enable failed\n");
		rt = ADC_ARG_ERROR;
		k3_adc_release();
		return rt;
	}

	/*choose hkadc voltage*/
    if (ADC_VOLTAGE_MOD1 == adc_table[ch].vol) {
        write_reg = (unsigned char)(ADC_VOLTAGE_MOD1 | ch);
    } else if (ADC_VOLTAGE_MOD2 == adc_table[ch].vol){
        write_reg = (unsigned char)(ADC_VOLTAGE_MOD2 | ch);
    }else{
        write_reg = (unsigned char)(ADC_VOLTAGE_MOD3 | ch);
    }

	k3_adc_reg_write(HKADC_CONFIG_ADDR, write_reg, 0x0);

	/*choose hkadc bias current ,clock.bypass HKADC*/
	write_reg = (unsigned char)(HKADC_BYPASS_YES | HKADC_BIAS_SEL_FOUR | adc_table[ch].clock);
	k3_adc_reg_write(HKADC_SETUP_ADDR, write_reg, 0x0);

	/*disable bypass HKADC,open BUFF,powerup HKADC,disable convert*/
	write_reg = (unsigned char)(HKADC_BIAS_SEL_FOUR | adc_table[ch].clock | HKADC_BYPASS_NO | HKADC_POWERUP_YES |
					HKADC_CONVERT_ON_CONFIG_WR_NO|adc_table[ch].buffer);
	k3_adc_reg_write(HKADC_SETUP_ADDR, write_reg, 0x0);

	/*enable convert*/
       write_reg = (unsigned char)(HKADC_BIAS_SEL_FOUR | adc_table[ch].clock | HKADC_BYPASS_NO | HKADC_POWERUP_YES |
					HKADC_CONVERT_ON_CONFIG_WR_YES | adc_table[ch].buffer);
	k3_adc_reg_write(HKADC_SETUP_ADDR, write_reg, 0x0);

	rt = k3_adc_convert(buf, adc_table[ch].clock);
	if (ADC_SUCCESS != rt) {
		k3_adc_print_error("HKADC OUTTIME!");
		k3_adc_bypass();
		clk_disable(adc_dev->clk);
		k3_adc_release();
		return rt;
	}

	adc_value = k3_adc_calc_vol(buf[0], ch);
	buf[0] = adc_value * adc_table[ch].parameter;
	adc_value = buf[0];

	k3_adc_bypass();
	clk_disable(adc_dev->clk);
	k3_adc_release();

	return adc_value;
}
