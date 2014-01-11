/*******************************************************************************
 * Copyright:   Copyright (c) 2011. Hisilicon Technologies, CO., LTD.
 * Version:     V200
 * Filename:    hiadc_hal.h
 * Description: hiadc_hal.h
 * History:     1.Created by 00186176 on 2011/08/02
*******************************************************************************/
#include <linux/semaphore.h>
#include <linux/spinlock_types.h>

#ifndef __K3_ADC_HAL_H
#define __K3_ADC_HAL_H

#define ADC_VOLTAGE_MOD1	(0x00)/*2.5V:HKADC_CONFIG bit[6...4]:000*/
#define ADC_VOLTAGE_MOD2	(0x30)/*3.0V:HKADC_CONFIG bit[6...4]:011*/
#define ADC_VOLTAGE_MOD3	(0x40)/*power-supply:HKADC_CONFIG bit[6...4]:100*/
#define ADC_VOLTAGE_MOD_2500MV    (0x00)/*2.5V:HKADC_CONFIG bit[6...4]:000*/
#define ADC_VOLTAGE_MOD_3000MV    (0x30)/*3.0V:HKADC_CONFIG bit[6...4]:011*/

#define ADC_ADCIN1			(0x00)/*HKADC_CONFIG bit[3...0]  1 channel*/
#define ADC_ADCIN2			(0x01)/*HKADC_CONFIG bit[3...0]  2 channel*/
#define ADC_ADCIN3			(0x02)/*HKADC_CONFIG bit[3...0]  3 channel*/
#define ADC_NC0				(0x03)/*HKADC_CONFIG bit[3...0]  nc channel*/
#define ADC_VBATMON			(0x04)/*HKADC_CONFIG bit[3...0]   VBATMON*/
#define ADC_VCOINMON		(0x05)/*HKADC_CONFIG bit[3...0]   VCOINMON*/
#define ADC_RTMP			(0x06)/*HKADC_CONFIG bit[3...0]    RTMP*/
#define ADC_PB_DETECT		(0x07)
#define ADC_NC1				(0X08)
#define ADC_NC2				(0X09)
#define ADC_500K			(0x0A)
#define ADC_NC3				(0X0B)
#define ADC_NC4				(0X0C)
#define ADC_NC5				(0X0D)
#define ADC_NC6				(0X0E)
#define ADC_NC7				(0X0F)

#define LDO_AUDIO_CTRL_ADDR		(0x36)/*LDO AUDIO address*/
#define VEST_LDO_AUDIO		(0x70)/*LDO power3.3V*/
#define DIS_ECO_AUDIO		(0x00)/*LDO ECO disadle*/
#define EN_LDO_AUDIO		(0x01)/*enable LDO*/
#define DIS_LDO_AUDIO		(0x00)

#define HKADC_CONFIG_ADDR	(0xF8)/*CONFIG base address*/
#define HKADC_SETUP_ADDR	(0xF9)/*SETUP base address*/
#define HKADC_CONV_START_ADDR	(0xFA)/*CONV_START base address*/
#define HKADC_DATA1_ADDR	(0xFB)/*DATA1 base address*/
#define HKADC_DATA2_ADDR	(0xFC)/*DATA2 base address*/
#define HKADC_BUSY_ADDR		(0xFD)/*BUSY base address*/
#define HKADC_RSV1_ADDR		(0xFE)/*RSV1 base address*/
#define HKADC_RSV2_ADDR		(0xFF)/*RSV2 base address*/
#define PMU_LOW_VOL_ADDR		(0x4D)
#define PMU_IRQ_2				(0x02)

#define OTHER_CTRL_REG_ADDR	(0x4E)/*OTHER_CTRL_REG base address*/

#define REG_RET_ADDR		(0x86)/*RET base address*/

#define HKAD_MAX_LVL		(6)

#define VOLTAGE_MOD1		(2500)/*reference voltage 2.5*100*/
#define VOLTAGE_MOD2		(3000)/*reference voltage 3.0*100*/
#define VOLTAGE_MOD3		(3300)/*reference voltage 3.3*100*/

#define THRUPUT_MOD1		(5000)/*thruput 5K*/
#define THRUPUT_MOD2		(10000)/*thruput 10K*/
#define THRUPUT_MOD3		(50000)/*thruput 50K*/

#define AD_DIGITAL_DIV		(1024)/*ADC precision*/

#define AD_MAXSAMP			(10)
#define TC_MAXSAMP			(10)
#define DEFAULT_AD_SAMP		(3)
#define DEFAULT_TC_SAMP		(3)
#define AD_MINSAMP			(1)
#define TC_MINSAMP			(1)
#define AD_TIME_OUT			(1000000)/*ADC timeout*/
#define ADC_SAMP_NUM		(8)

/*register Mask bit*/
#define SETUP_INIT_MASK		(0x77)
#define SETUP_STOP_MASK		(0x00)
#define SETUP_AD_MASK		(0x00)
#define CONFIG_INIT_MASK	(0xFE)
#define CONFIG_AD_MASK		(0x00)

#define HKADC_BIAS_SEL_ONE	(0x00)/*HKADC_SETUP bit[7,6]  ,  BIAS 0.25u*/
#define HKADC_BIAS_SEL_TWO	(0x40)/*HKADC_SETUP bit[7,6]  ,  BIAS 0.5u*/
#define HKADC_BIAS_SEL_THREE	(0x80)/*HKADC_SETUP bit[7,6]  ,  BIAS 0.75u*/
#define HKADC_BIAS_SEL_FOUR	(0xC0)/*HKADC_SETUP bit[7,6]  ,  BIAS 1u*/

#define ADC_DATE_PRE_TIME	(5000)/*thruput*/

#define HKADC_CK_SEL_ONE	(0x00)/*HKADC_SETUP bit[5,4]  ,  no clock*/
#define HKADC_CK_SEL_TWO	(0x10)/*HKADC_SETUP bit[5,4]  ,  clock 1MEG*/
#define HKADC_CK_SEL_THREE	(0x20)/*HKADC_SETUP bit[5,4]  ,  clock 200k*/
#define HKADC_CK_SEL_FOUR	(0x30)/*HKADC_SETUP bit[5,4]  ,  clock 100k*/
#define HKADC_CK_SEL_100K    (0x30)/*HKADC_SETUP bit[5,4]  ,  clock 100k*/

#define HKADC_BYPASS_NO		(0x00)/*working HKADC_SETUP bit[3]:0*/
#define HKADC_BYPASS_YES	(0x08)/*bypass HKADC_SETUP bit[3]:1*/

#define HKADC_POWERUP_NO	(0x00)/*Power up HKADC HKADC_SETUP bit[2]:0*/
#define HKADC_POWERUP_YES	(0x04)/*Power up HKADC HKADC_SETUP bit[2]:1*/

#define HKADC_CONVERT_ON_CONFIG_WR_NO	(0x00)/*HKADC HKADC_SETUP bit[1]:0  disable CONVERT ON CONFIG WR*/
#define HKADC_CONVERT_ON_CONFIG_WR_YES	(0x02)/*HKADC HKADC_SETUP bit[1]:1  enable CONVERT ON CONFIG WR*/

#define HKADC_BUFF_SEL_NO	(0x00)/*HKADC_SETUP bit[0]:0 disable BUFF SEL*/
#define HKADC_BUFF_SEL_YES	(0x01)/*HKADC_SETUP bit[0]:1 enable BUFF SEL*/

#define HKADC_STARTEN_NO	(0x00)/*disable start_en HKADC_CONV bit[1]:0*/
#define HKADC_STARTEN_YES	(0x02)/*enable start_en HKADC_CONV bit[1]:1*/

#define HKADC_START_NO		(0x00)/*disable start HKADC_CONV bit[0]:0*/
#define HKADC_START_YES		(0x01)/*enable start HKADC_CONV bit[0]:1*/

#define HKADC_AD_BUSY_NO	(0x01)/*HKADC BUSY*/

#define HKADC_RETRY_COUNT	(0x10)

#define EN_VBATMON			(0x08)/*enable VBATMON*/
#define DIS_VBATMON			(0x00)/*disable VBATMON*/
#define EN_RTMP				(0x04)/*enable RTMP*/
#define DIS_RTMP			(0x00)/*disable RTMP*/
#define EN_CHG_PUMP_INT		(0x02)/*open chg_pump*/
#define DIS_CHG_PUMP_INT	(0x00)/*close chg_pump*/
#define EN_TMP_DET			(0x01)/*enable temperature*/
#define DIS_TMP_DET			(0x00)/*disable temperature*/

#define ADC_SUCCESS			(0)/*HKADC SUCCESS*/
#define ADC_ERROR			(-1)/*HKADC ERROR*/
#define ADC_CMD_ERROR		(-2)/*HKADC CMD ERROR*/
#define ADC_ARG_ERROR		(-3)/*HKADC ARG ERROR*/
#define ADC_BUG_ERROR		(-4)/*HKADC BUG ERROR*/
#define ADC_OPERATE_ERROR	(-5)
#define ENABLE_ADC			(1)
#define DISABLE_ADC			(0)

#define ADC_SEQ				(1)
#define ADC_ONE				(0)

#define SAMPLING_NUM		(1)
#define SAMPLING_TIME_PRE	(1)/*sampling frequency*/

#define ADC_WRITE_COUNT		(1)/*HKADC write count*/

#define TEST_NUM_PRE		(5)
#define MODIFY_COEFFICIENT1	(1700)
#define MODIFY_COEFFICIENT2	(1630)

#define LDO_AUDIO_VOL		(3300000)
#define ADO_AUDIO_CURRENT	(100000)

#define ADC_SHITF			(2)
#define CLAC_SHIFT			(12)

#define ADC_PARAMETER1		(1)
#define ADC_PARAMETER2		(2)

#define	WAIT_TIME_RANGE_FOR_1M_MIN	(100)
#define	WAIT_TIME_RANGE_FOR_1M_MAX	(200)
#define	WAIT_TIME_RANGE_FOR_200K_MIN	(200)
#define	WAIT_TIME_RANGE_FOR_200K_MAX	(400)
#define	WAIT_TIME_RANGE_FOR_100K_MIN	(400)
#define	WAIT_TIME_RANGE_FOR_100K_MAX	(600)

struct adc_device {
	struct platform_device	*pdev;
	struct clk		*clk;
	struct regulator	*regu;
	void __iomem		*regs;
	//struct semaphore	 lock;
	spinlock_t	reg_lock;
};


struct k3_adc_client {
	unsigned int		 nr_samples;
	int			 result;
	unsigned char		 channel;
	int clock;
	int vol;
	int buffer_set;
	int parameter;
	struct adc_device *adc;
};

struct adc_data {
	int ch;
	int vol;
	int clock;
	int buffer;
	int parameter;
	int count;
};

struct adc_dataEx {
    struct adc_data *data;
    int sum;
};

int k3_adc_close_channal(int ch);
int k3_adc_open_channel(int ch);
int k3_adc_get_value(unsigned int ch, unsigned char *reserve);
void k3_adc_power(int flag);

void k3_adc_lock(void);
void k3_adc_release(void);
void k3_adc_reg_read(unsigned char addr, unsigned char *read_value, unsigned int count);
void k3_adc_reg_write(unsigned char addr, unsigned char write_value, unsigned char mask);
void k3_adc_msleep(int ms);
void k3_adc_msleep_range(int ms_min, int ms_max);
void k3_adc_usleep(int us);
void k3_adc_print_error(char *s);
void k3_adc_set_pmu(unsigned char addr, unsigned char write_reg,unsigned char mask);
void k3_adc_get_pmu(unsigned char addr, unsigned char *read_reg);
#endif