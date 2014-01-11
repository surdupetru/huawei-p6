/*
 * kernel/driver/regulator/hi6421-regulator.c
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/hi6421/hi6421-regulator.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <mach/platform.h>
#include <mach/irqs.h>
#include <linux/interrupt.h>
#include <mach/early-debug.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <linux/time.h>
#include <hsad/config_interface.h>


#define SD_GPIO_DCDC GPIO_21_4
static const char *hi6421_regulator_get_name(struct regulator_dev *rdev);
static DEFINE_MUTEX(reg_lock_mutex);
#define rdev_err(rdev, fmt, ...)				\
	pr_err("%s: " fmt, hi6421_regulator_get_name(rdev), ##__VA_ARGS__)
#define rdev_info(rdev, fmt, ...)					\
	pr_info("%s: " fmt, hi6421_regulator_get_name(rdev), ##__VA_ARGS__)
#define lock(x) mutex_lock(x)
#define unlock(x)  mutex_unlock(x)
struct hi6421_regulator_data{

		void __iomem	*base;
		struct resource	*res;
		struct clk *clk_pmu;
		struct regulator_dev	*rdev[NUM_OF_HI6421_REGULATOR];
		struct timeval lastoff_time[NUM_OF_HI6421_REGULATOR];
		unsigned int irq;
		struct regulator_consumer_supply *consumer_supplies;
		int num_consumer_supplies;
};
/*Regulator ctrl register: ctrl ecc mode,nomal mode, disable and enable.*/
static const u16 regulator_ctrl_to_reg[NUM_OF_HI6421_REGULATOR] = {
	LDO0_CTRL_REG,
	LDO1_CTRL_REG,
	LDO2_CTRL_REG,
	LDO3_CTRL_REG,
	LDO4_CTRL_REG,
	LDO5_CTRL_REG,
	LDO6_CTRL_REG,
	LDO7_CTRL_REG,
	LDO8_CTRL_REG,
	LDO9_CTRL_REG,
	LDO10_CTRL_REG,
	LDO11_CTRL_REG,
	LDO12_CTRL_REG,
	LDO13_CTRL_REG,
	LDO14_CTRL_REG,
	LDO15_CTRL_REG,
	LDO16_CTRL_REG,
	LDO17_CTRL_REG,
	LDO18_CTRL_REG,
	LDO19_CTRL_REG,
	LDO20_CTRL_REG,
	LDO_AUDIO_CTRL_REG,
	BUCK0_CTRL_REG,
	BUCK1_CTRL_REG,
	BUCK2_CTRL_REG,
	BUCK3_CTRL_REG,
	BUCK4_CTRL_REG,
	BUCK5_CTRL_REG,
	HI6421_USB_HDMI_CHG_PUMP_CTRL_REG,/*donot used*/
	HI6421_USB_HDMI_CHG_PUMP_CTRL_REG,/*donot used*/
};

/*Regulator vset register: set volatage*/
static const u16 regulator_vset_to_reg[NUM_OF_HI6421_REGULATOR] = {
	LDO0_VSET_REG,
	LDO1_VSET_REG,
	LDO2_VSET_REG,
	LDO3_VSET_REG,
	LDO4_VSET_REG,
	LDO5_VSET_REG,
	LDO6_VSET_REG,
	LDO7_VSET_REG,
	LDO8_VSET_REG,
	LDO9_VSET_REG,
	LDO10_VSET_REG,
	LDO11_VSET_REG,
	LDO12_VSET_REG,
	LDO13_VSET_REG,
	LDO14_VSET_REG,
	LDO15_VSET_REG,
	LDO16_VSET_REG,
	LDO17_VSET_REG,
	LDO18_VSET_REG,
	LDO19_VSET_REG,
	LDO20_VSET_REG,
	LDO_AUDIO_VSET_REG,
	BUCK0_VSET_REG,
	BUCK1_VSET_REG,
	BUCK2_VSET_REG,
	BUCK3_VSET_REG,
	BUCK4_VSET_REG,
	BUCK5_VSET_REG,
	HI6421_USB_HDMI_CHG_PUMP_CTRL_REG,/*donot used*/
	HI6421_USB_HDMI_CHG_PUMP_CTRL_REG,/*donot used*/
};

static const u32 on_off_delay_us[NUM_OF_HI6421_REGULATOR] = {
       10000, 10000, 20000, 20000, 20000, 20000, 20000, 20000, 20000, 40000, 40000, /* LDO0 ~ 10 */
       40000, 40000,40000, 40000,40000, 40000,40000, 40000,40000, 40000, 40000, /* LDO_11 ~ LDO20 ,LDO_AUDIO */
       20000, 20000, 100, 20000, 20000, 20000, /* BUCK0 ~ 5*/
       0,0/*donot used*/
};

static const u32 on_enable_delay_us[NUM_OF_HI6421_REGULATOR] = {
	250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, /* LDO0 ~ 10 */
	250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, /* LDO_11 ~ LDO20 ,LDO_AUDIO */
	300, 300, 250, 250, 250, 250, /* BUCK0 ~ 5*/
	0,0 /*donot used*/
};


/*all regulator voltage in mv,the voltage for the regulator within the range*/
static const u16 regulator_vset_table[NUM_OF_HI6421_REGULATOR][NUM_OF_HI6421_REGULATOR_VSET] = {
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},	/*ldo0 voltage in mv*/
	{1700, 1800, 1900, 2000,},
	{1050, 1100, 1150, 1200, 1250, 1300, 1350, 1400},
	{1050, 1100, 1150, 1200, 1250, 1300, 1350, 1400},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},	/*ldo5 voltage in mv*/
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2600, 2700, 2850, 3000, 3300},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},	/*ldo10 voltage in mv*/
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2600, 2700, 2850, 3000, 3300},	/*ldo15 voltage in mv*/
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},
	{1500, 1800, 2400, 2500, 2600, 2700, 2850, 3000},	/*ldo20 voltage in mv*/
	{2800, 2850, 2900, 2950, 3000, 3100, 3200, 3300},	/*ldoAUDIO voltage in mv*/
	{0, 0, 0, 0, 0, 0, 0, 0},				/*buck0 buck1 buck2 has 128 step,need handle special ,so set all 0*/
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{700, 800, 900, 950, 1050, 1100, 1150, 1200},		/*buck3 voltage in mv*/
	{1150, 1200, 1250, 1350, 1700, 1800, 1900, 2000},	/*buck4 voltage in mv*/
	{1150, 1200, 1250, 1350, 1600, 1700, 1800, 1900},	/*buck5 voltage in mv*/
	{0, 0, 0, 0, 0, 0, 0, 0},		/*donot used*/
	{0, 0, 0, 0, 0, 0, 0, 0},		/*donot used*/
};
/*this function is get regulator name in order to trace*/
static const char *hi6421_regulator_get_name(struct regulator_dev *rdev)
{
	if (rdev == NULL) {
		return "Hi6421-Regulator";
	}
	if (rdev->constraints && rdev->constraints->name)
		return rdev->constraints->name;
	else if (rdev->desc->name)
		return rdev->desc->name;
	return "error";
}
static int hi6421_regulator_set_bits(struct hi6421_regulator_data *hi6421_regulator_data, u16 reg, u16 mask, u16 val)
{
	u32 tmp;
	lock(&reg_lock_mutex);
	tmp = readl(hi6421_regulator_data->base + reg);
	tmp = (tmp & ~mask) | (val & mask) ;
	writel(tmp, hi6421_regulator_data->base + reg);
	unlock(&reg_lock_mutex);
	return 0;
}
static u32 hi6421_reg_read(struct hi6421_regulator_data *hi6421_regulator_data, u16 reg)
{
	u32 tmp;
	tmp = readl(hi6421_regulator_data->base + reg);
	return tmp;
}
static int hi6421_regulator_is_enabled(struct regulator_dev *dev)
{
	int  regulator_id;
	u32 val;
	struct hi6421_regulator_data *hi6421_regulator_data = rdev_get_drvdata(dev);
	regulator_id =  rdev_get_id(dev);
	val = hi6421_reg_read(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id]);
	/*LDO0-LDO20(HI6421_LDO_ENA_MASK),
	LDO_AUDIO(HI6421_LDOAUDIO_ENA_MASK),
	BUCK0-BUCK5(HI6421_BUCK_ENA_MASK)  have differ eable mask!*/
	if (regulator_id <= HI6421_LDO20) {
		return (val & HI6421_LDO_ENA_MASK) != 0;
	} else if (regulator_id == HI6421_LDOAUDIO) {
		return (val & HI6421_LDOAUDIO_ENA_MASK) != 0;
	} else {
		return (val & HI6421_BUCK_ENA_MASK) != 0;
	}
}
static int hi6421_regulator_enable(struct regulator_dev *dev)
{
	int  regulator_id, ret;
	struct timeval tv;
	u32 diff;

	struct hi6421_regulator_data *hi6421_regulator_data = rdev_get_drvdata(dev);
	regulator_id =  rdev_get_id(dev);
	/*LDO0-LDO20(HI6421_LDO_ENA_MASK),LDO_AUDIO(HI6421_LDOAUDIO_ENA_MASK),
	BUCK0-BUCK5(HI6421_BUCK_ENA_MASK)  have differ eable mask!*/

	#ifdef HI6421_REGULATOR_DEBUG
	rdev_info(dev, "will be enabled\n");
	#endif

        do_gettimeofday(&tv);
        diff = (tv.tv_sec - hi6421_regulator_data->lastoff_time[regulator_id].tv_sec) * USEC_PER_SEC
                        + tv.tv_usec - hi6421_regulator_data->lastoff_time[regulator_id].tv_usec;
        if (diff < on_off_delay_us[regulator_id]) {
                msleep((on_off_delay_us[regulator_id] - diff + 999 )/1000);
        }

	if (regulator_id == HI6421_LDO12) {
		ret = gpio_request(SD_GPIO_DCDC,  "sdcard_dcdc");
		if (ret < 0) {
			rdev_err(NULL, "hi6421 regulator gpio_request failed,please check!\n");
			return ret;
		}
		gpio_direction_output(SD_GPIO_DCDC, 1);
	}
	if (regulator_id <= HI6421_LDO20) {
		hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_LDO_ENA_MASK, HI6421_LDO_ENA);
	} else if (regulator_id == HI6421_LDOAUDIO) {
		hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_LDOAUDIO_ENA_MASK, HI6421_LDOAUDIO_ENA);
	} else {
		hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_BUCK_ENA_MASK, HI6421_BUCK_ENA);
	}

	udelay(on_enable_delay_us[regulator_id]);

	return 0;
}
static int hi6421_regulator_disable(struct regulator_dev *dev)
{
	int  regulator_id;

	struct hi6421_regulator_data *hi6421_regulator_data = rdev_get_drvdata(dev);
	regulator_id =  rdev_get_id(dev);
	/*LDO0-LDO20(HI6421_LDO_ENA_MASK),LDO_AUDIO(HI6421_LDOAUDIO_ENA_MASK),
	BUCK0-BUCK5(HI6421_BUCK_ENA_MASK)  have differ eable mask!*/
	#ifdef HI6421_REGULATOR_DEBUG
	rdev_info(dev, "will be disabled\n");
	#endif

	/*buck2 voltage must be set as 1.1v when gpu power off*/
	if (regulator_id == HI6421_BUCK2) {
		/*(1100000 - 700000) / HI6421_BUCK_SPECIAL(7087) = 56*/
		hi6421_regulator_set_bits(hi6421_regulator_data, regulator_vset_to_reg[regulator_id], HI6421_BUCK012_VSEL_MASK, 56);
	}

#ifndef CONFIG_K3_REGULATOR_ALWAYS_ON
	if (regulator_id <= HI6421_LDO20) {
		hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_LDO_ENA_MASK, 0);
	} else if (regulator_id == HI6421_LDOAUDIO) {
		hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_LDOAUDIO_ENA_MASK, 0);
	} else {
		hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_BUCK_ENA_MASK, 0);
	}
	if (regulator_id == HI6421_LDO0) {
		writel(0, hi6421_regulator_data->base + PMU_REST_REG);
	}
#endif
	if (regulator_id == HI6421_LDO12) {
		gpio_direction_output(SD_GPIO_DCDC, 0);
		gpio_free(SD_GPIO_DCDC);
	}
	do_gettimeofday(&hi6421_regulator_data->lastoff_time[regulator_id]);
	return 0;
}
static int hi6421_regulator_list_voltage(struct regulator_dev *dev,
				   unsigned selector)
{
	int  regulator_id;
	regulator_id =  rdev_get_id(dev);
	/* BUCK0,BUCK1,BUCK2,vset has 128step,uv=700000+vsel*7100 need handle special*/
	if (regulator_id == HI6421_BUCK0 || regulator_id == HI6421_BUCK1 || regulator_id == HI6421_BUCK2) {
		return HI6421_BUCK012_MIN_VOLTAGE + selector*HI6421_BUCK_SPECIAL;
	} else {
		if (selector >= NUM_OF_HI6421_REGULATOR_VSET) {
			rdev_err(dev, "selector is %d,error,please input new\n", selector);
			return -EINVAL;
		} else
		return regulator_vset_table[regulator_id][selector]*1000;
	}
}
static int hi6421_regulator_get_voltage(struct regulator_dev *dev)
{
	u32 val;
	int  regulator_id;
	struct hi6421_regulator_data *hi6421_regulator_data = rdev_get_drvdata(dev);
	regulator_id =  rdev_get_id(dev);
	val = hi6421_reg_read(hi6421_regulator_data, regulator_vset_to_reg[regulator_id]);
	/*LDO1(HI6421_LDO1_VSEL_MASK);LDO0,LDO2-LDO20(HI6421_LDO_OTHERS_VSEL_MASK);
	LDO_AUDIO(HI6421_LDOAUDIO_VSEL_MASK);BUCK0-BUCK2(HI6421_BUCK012_VSEL_MASK)
	BUCK3-BUCK5(HI6421_BUCK345_VSEL_MASK)  have differ voltage mask!*/
	if (regulator_id == HI6421_LDO1) {
		val &= HI6421_LDO1_VSEL_MASK;
	} else if (regulator_id == HI6421_BUCK0 || regulator_id == HI6421_BUCK1 || regulator_id == HI6421_BUCK2) {
		val &= HI6421_BUCK012_VSEL_MASK;
		return HI6421_BUCK012_MIN_VOLTAGE + val*HI6421_BUCK_SPECIAL;
	} else if (regulator_id == HI6421_BUCK3 || regulator_id == HI6421_BUCK4 || regulator_id == HI6421_BUCK5) {
		val &= HI6421_BUCK345_VSEL_MASK;
	} else if (regulator_id == HI6421_LDOAUDIO) {
		val &= HI6421_LDOAUDIO_VSEL_MASK;
		val = val >> 4;
	} else {
		val &= HI6421_LDO_OTHERS_VSEL_MASK;
	}
	return hi6421_regulator_list_voltage(dev, val);
}
static int hi6421_regulator_set_voltage(struct regulator_dev *dev,
				  int min_uV, int max_uV, unsigned *selector)
{
	struct hi6421_regulator_data *hi6421_regulator_data = rdev_get_drvdata(dev);
	int regulator_id = rdev_get_id(dev);
	int vsel = 0;
	/* BUCK0,BUCK1,BUCK2,vset has 128step,uv=700000+vsel*7100 need handle special*/
	if (regulator_id == HI6421_BUCK0 || regulator_id == HI6421_BUCK1 || regulator_id == HI6421_BUCK2) {
		if (max_uV < HI6421_BUCK012_MIN_VOLTAGE) {
			rdev_info(dev, "input error max voltage\n");
			return -EINVAL;
		}
		vsel = (max_uV - HI6421_BUCK012_MIN_VOLTAGE) / HI6421_BUCK_SPECIAL;
		*selector = vsel;
		#ifdef HI6421_REGULATOR_DEBUG
		rdev_info(dev, "set voltage step is %d\n", vsel);
		#endif
		return hi6421_regulator_set_bits(hi6421_regulator_data, regulator_vset_to_reg[regulator_id], HI6421_BUCK012_VSEL_MASK, vsel);
	} else {
		/*other regulator, vset has 8 step, refer to the table "regulator_vset_table"*/
		for (vsel = 0; vsel < NUM_OF_HI6421_REGULATOR_VSET; vsel++) {
			int mV = regulator_vset_table[regulator_id][vsel];
			int uV = mV * 1000;
			/* Break at the first in-range value */
			if (min_uV <= uV && uV <= max_uV)
				break;
		}
		#ifdef HI6421_REGULATOR_DEBUG
		rdev_info(dev, "set voltage step is %d\n", vsel);
		#endif
		if (vsel == NUM_OF_HI6421_REGULATOR_VSET) {
			rdev_err(dev, "regulator no support this voltage\n");
			return -EINVAL;
		}
		*selector = vsel;
		/* write to the register in case we found a match */
		/*LDO1(HI6421_LDO1_VSEL_MASK);LDO0,LDO2-LDO20(HI6421_LDO_OTHERS_VSEL_MASK);
		LDO_AUDIO(HI6421_LDOAUDIO_VSEL_MASK);BUCK0-BUCK2(HI6421_BUCK012_VSEL_MASK)
		BUCK3-BUCK5(HI6421_BUCK345_VSEL_MASK)  have differ voltage mask!*/
		if (regulator_id == HI6421_LDO1) {
			return hi6421_regulator_set_bits(hi6421_regulator_data, regulator_vset_to_reg[regulator_id], HI6421_LDO1_VSEL_MASK, vsel);
		} else if (regulator_id == HI6421_LDOAUDIO) {
			return hi6421_regulator_set_bits(hi6421_regulator_data, regulator_vset_to_reg[regulator_id], HI6421_LDOAUDIO_VSEL_MASK, (vsel << 4));
		} else if (regulator_id == HI6421_BUCK3 || regulator_id == HI6421_BUCK4 || regulator_id == HI6421_BUCK5) {
			return hi6421_regulator_set_bits(hi6421_regulator_data, regulator_vset_to_reg[regulator_id], HI6421_BUCK345_VSEL_MASK, vsel);
		} else {
			return hi6421_regulator_set_bits(hi6421_regulator_data, regulator_vset_to_reg[regulator_id], HI6421_LDO_OTHERS_VSEL_MASK, vsel);
		}
	}
}
static unsigned int hi6421_regulator_get_mode(struct regulator_dev *dev)
{
	u32 val;
	struct hi6421_regulator_data *hi6421_regulator_data = rdev_get_drvdata(dev);
	int regulator_id = rdev_get_id(dev);
	val = hi6421_reg_read(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id]);
	/*LDO0-LDO20(HI6421_LDO_ENTRY_ECO_MODE); BUCK0-BUCK5(HI6421_BUCK_ENTRY_ECO_MODE) ;
	 LDO_AUDIO(HI6421_LDOAUDIO_ENTRY_ECO_MODE) has different eco mode mask*/
	if ((regulator_id >= HI6421_BUCK0) && (regulator_id <= HI6421_BUCK5)) {
		if (val & HI6421_BUCK_ENTRY_ECO_MODE)
			return REGULATOR_MODE_IDLE;
		else
			return REGULATOR_MODE_NORMAL;
	} else if (regulator_id == HI6421_LDOAUDIO) {
		if (val & HI6421_LDOAUDIO_ENTRY_ECO_MODE)
			return REGULATOR_MODE_IDLE;
		else
			return REGULATOR_MODE_NORMAL;
	} else {
		if (val & HI6421_LDO_ENTRY_ECO_MODE)
			return REGULATOR_MODE_IDLE;
		else
			return REGULATOR_MODE_NORMAL;
	}
}
static int hi6421_regulator_set_mode(struct regulator_dev *dev, unsigned int mode)
{
	struct hi6421_regulator_data *hi6421_regulator_data = rdev_get_drvdata(dev);
	int regulator_id = rdev_get_id(dev);
	/*LDO0-LDO20(HI6421_LDO_ENTRY_ECO_MODE); BUCK0-BUCK5(HI6421_BUCK_ENTRY_ECO_MODE) ;
	LDO_AUDIO(HI6421_LDOAUDIO_ENTRY_ECO_MODE) has different eco mode mask*/
	switch (mode) {
	case REGULATOR_MODE_NORMAL:
		#ifdef HI6421_REGULATOR_DEBUG
		rdev_info(dev, "will be set normal mode\n");
		#endif
		if ((regulator_id >= HI6421_BUCK0) && (regulator_id <= HI6421_BUCK5)) {
			hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_BUCK_ENTRY_ECO_MODE, 0);
		} else  if (regulator_id == HI6421_LDOAUDIO) {
			hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_LDOAUDIO_ENTRY_ECO_MODE, 0);
		} else {
			hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_LDO_ENTRY_ECO_MODE, 0);
		}
		break;
	case REGULATOR_MODE_IDLE:
		rdev_err(dev, "will be set eco mode:regulator_id[%d]\n", regulator_id);
		if ((regulator_id >= HI6421_BUCK0) && (regulator_id <= HI6421_BUCK5)) {
			hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_BUCK_ENTRY_ECO_MODE, HI6421_BUCK_ENTRY_ECO_MODE);
		} else if (regulator_id == HI6421_LDOAUDIO) {
			hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_LDOAUDIO_ENTRY_ECO_MODE, HI6421_LDOAUDIO_ENTRY_ECO_MODE);
		} else {
			hi6421_regulator_set_bits(hi6421_regulator_data, regulator_ctrl_to_reg[regulator_id], HI6421_LDO_ENTRY_ECO_MODE, HI6421_LDO_ENTRY_ECO_MODE);
		}
		break;
	default:
		rdev_err(dev, "no support the mode\n");
		return -EINVAL;
	}
	return 0;
}
unsigned int hi6421_regulator_get_optimum_mode(struct regulator_dev *dev, int input_uV,
					  int output_uV, int load_uA)
{
	int regulator_id = rdev_get_id(dev);

	if (load_uA == 0) {
		return REGULATOR_MODE_NORMAL;
	}

	switch (regulator_id) {
	case HI6421_LDO0:
	case HI6421_LDO2:
	case HI6421_LDO3:
	case HI6421_LDO4:
	case HI6421_LDO5:
	case HI6421_LDO6:
	case HI6421_LDO8:
	case HI6421_LDO9:
	case HI6421_LDO10:
	case HI6421_LDO11:
	case HI6421_LDO12:
	case HI6421_LDO14:
	case HI6421_LDO16:
	case HI6421_LDO17:
	case HI6421_LDO15:
	case HI6421_LDO13:
	case HI6421_LDO18:
	case HI6421_LDO19:
	case HI6421_LDO20:
		if (load_uA > REGULATOR_ECO_UA_1) {
			return REGULATOR_MODE_NORMAL;
		} else {
			return REGULATOR_MODE_IDLE;
		}
	case HI6421_LDO1:
	case HI6421_LDO7:
	case HI6421_LDOAUDIO:
		if (load_uA > REGULATOR_ECO_UA_2) {
			return REGULATOR_MODE_NORMAL;
		} else {
			return REGULATOR_MODE_IDLE;
		}
	default:
		return REGULATOR_MODE_NORMAL;
	}
}
static struct regulator_ops hi6421_ldo_ops = {
	.is_enabled = hi6421_regulator_is_enabled,
	.enable = hi6421_regulator_enable,
	.disable = hi6421_regulator_disable,
	.list_voltage = hi6421_regulator_list_voltage,
	.get_voltage = hi6421_regulator_get_voltage,
	.set_voltage = hi6421_regulator_set_voltage,
	.get_mode = hi6421_regulator_get_mode,
	.set_mode = hi6421_regulator_set_mode,
	.get_optimum_mode = hi6421_regulator_get_optimum_mode,
};

static int hi6421_chg_pump_is_enabled(struct regulator_dev *dev)
{
	int  regulator_id;
	u32 val;
	struct hi6421_regulator_data *hi6421_regulator_data = rdev_get_drvdata(dev);
	regulator_id =  rdev_get_id(dev);
	val = hi6421_reg_read(hi6421_regulator_data, (u16)HI6421_USB_HDMI_CHG_PUMP_CTRL_REG);
	if (regulator_id == HI6421_USB_CHG_PUMP) {
		return (val & HI6421_USB_CHG_PUMP_ENA_MASK) != 0;
	} else if (regulator_id == HI6421_HDMI_CHG_PUMP) {
		return (val & HI6421_HDMI_CHG_PUMP_ENA_MASK) != 0;
	} else {
		rdev_err(NULL, "there is no chg pump\n");
		return -EINVAL;
	}

}
static int hi6421_chg_pump_enable(struct regulator_dev *dev)
{
	int  regulator_id;
	struct hi6421_regulator_data *hi6421_regulator_data = rdev_get_drvdata(dev);
	regulator_id =  rdev_get_id(dev);
	#ifdef HI6421_REGULATOR_DEBUG
	rdev_info(dev, "will be enabled\n");
	#endif
	if (regulator_id == HI6421_USB_CHG_PUMP) {
		hi6421_regulator_set_bits(hi6421_regulator_data, (u16)HI6421_USB_HDMI_CHG_PUMP_CTRL_REG,
			HI6421_USB_CHG_PUMP_ENA_MASK, HI6421_USB_CHG_PUMP_ENA_MASK);
	} else if (regulator_id == HI6421_HDMI_CHG_PUMP) {
		hi6421_regulator_set_bits(hi6421_regulator_data, (u16)HI6421_USB_HDMI_CHG_PUMP_CTRL_REG,
			HI6421_HDMI_CHG_PUMP_ENA_MASK, HI6421_HDMI_CHG_PUMP_ENA_MASK);
	} else {
		rdev_err(NULL, "can't enable, because there is no chg pump\n");
		return -EINVAL;
	}
	return 0;
}
static int hi6421_chg_pump_disable(struct regulator_dev *dev)
{
	int  regulator_id;
	struct hi6421_regulator_data *hi6421_regulator_data = rdev_get_drvdata(dev);
	regulator_id =  rdev_get_id(dev);
	#ifdef HI6421_REGULATOR_DEBUG
	rdev_info(dev, "will be disabled\n");
	#endif

#ifndef CONFIG_K3_REGULATOR_ALWAYS_ON
	if (regulator_id == HI6421_USB_CHG_PUMP) {
		hi6421_regulator_set_bits(hi6421_regulator_data, (u16)HI6421_USB_HDMI_CHG_PUMP_CTRL_REG,
			HI6421_USB_CHG_PUMP_ENA_MASK, 0);
	} else if (regulator_id == HI6421_HDMI_CHG_PUMP) {
		hi6421_regulator_set_bits(hi6421_regulator_data, (u16)HI6421_USB_HDMI_CHG_PUMP_CTRL_REG,
			HI6421_HDMI_CHG_PUMP_ENA_MASK, 0);
	} else {
		rdev_err(NULL, "can't disable, because there is no chg pump\n");
		return -EINVAL;
	}
#endif
	return 0;
}
static struct regulator_ops hi6421_chg_pump_ops = {
	.is_enabled = hi6421_chg_pump_is_enabled,
	.enable = hi6421_chg_pump_enable,
	.disable = hi6421_chg_pump_disable,
};

static struct regulator_desc regulators[] = {
	[HI6421_LDO0] = {
		.name = "LDO0",
		.id = HI6421_LDO0,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO1] = {
		.name = "LDO1",
		.id = HI6421_LDO1,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 4,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO2] = {
		.name = "LDO2",
		.id = HI6421_LDO2,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO3] = {
		.name = "LDO3",
		.id = HI6421_LDO3,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO4] = {
		.name = "LDO4",
		.id = HI6421_LDO4,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO5] = {
		.name = "LDO5",
		.id = HI6421_LDO5,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO6] = {
		.name = "LDO6",
		.id = HI6421_LDO6,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO7] = {
		.name = "LDO7",
		.id = HI6421_LDO7,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO8] = {
		.name = "LDO8",
		.id = HI6421_LDO8,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO9] = {
		.name = "LDO9",
		.id = HI6421_LDO9,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO10] = {
		.name = "LDO10",
		.id = HI6421_LDO10,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO11] = {
		.name = "LDO11",
		.id = HI6421_LDO11,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO12] = {
		.name = "LDO12",
		.id = HI6421_LDO12,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO13] = {
		.name = "LDO13",
		.id = HI6421_LDO13,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO14] = {
		.name = "LDO14",
		.id = HI6421_LDO14,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO15] = {
		.name = "LDO15",
		.id = HI6421_LDO15,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO16] = {
		.name = "LDO16",
		.id = HI6421_LDO16,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO17] = {
		.name = "LDO17",
		.id = HI6421_LDO17,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO18] = {
		.name = "LDO18",
		.id = HI6421_LDO18,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO19] = {
		.name = "LDO19",
		.id = HI6421_LDO19,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDO20] = {
		.name = "LDO20",
		.id = HI6421_LDO20,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_LDOAUDIO] = {
		.name = "LDO_AUDIO",
		.id = HI6421_LDOAUDIO,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_BUCK0] = {
		.name = "BUCK0",
		.id = HI6421_BUCK0,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 128,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_BUCK1] = {
		.name = "BUCK1",
		.id = HI6421_BUCK1,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 128,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_BUCK2] = {
		.name = "BUCK2",
		.id = HI6421_BUCK2,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 128,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_BUCK3] = {
		.name = "BUCK3",
		.id = HI6421_BUCK3,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_BUCK4] = {
		.name = "BUCK4",
		.id = HI6421_BUCK4,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_BUCK5] = {
		.name = "BUCK5",
		.id = HI6421_BUCK5,
		.ops = &hi6421_ldo_ops,
		.n_voltages = 8,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_USB_CHG_PUMP] = {
		.name = "USB_CHG_PUMP",
		.id = HI6421_USB_CHG_PUMP,
		.ops = &hi6421_chg_pump_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[HI6421_HDMI_CHG_PUMP] = {
		.name = "HDMI_CHG_PUMP",
		.id = HI6421_HDMI_CHG_PUMP,
		.ops = &hi6421_chg_pump_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
};
static __devinit int hi6421_regulator_probe(struct platform_device *pdev)
{
	int ret, regulator_id, reg_ocp;
	int irq;
	int oclk;
	bool pmu_out26m_enable=false;
	struct hi6421_regulator_data  *hi6421_regulator_data;
	struct regulator_init_data *regulator_init_data = pdev->dev.platform_data;

	hi6421_regulator_data = kzalloc(sizeof(*hi6421_regulator_data), GFP_KERNEL);
	if (hi6421_regulator_data == NULL) {
		rdev_err(NULL, "hi6421 regulator kzalloc mem fail,please check!\n");
		return -ENOMEM;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = -ENXIO;
		rdev_err(NULL, "failed to get IRQ\n");
		goto err_free_mem;
	}

	hi6421_regulator_data->irq = (unsigned int)irq;
	hi6421_regulator_data->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (NULL == hi6421_regulator_data->res) {
		ret = -ENOMEM;
		rdev_err(NULL, "hi6421 regulator platform_get_resource err ,please check: ret=%d!\n", ret);
		goto err_free_mem;
	}

	hi6421_regulator_data->base = ioremap(hi6421_regulator_data->res->start, resource_size(hi6421_regulator_data->res));
	if (hi6421_regulator_data->base == NULL) {
		ret = -ENOMEM;
		rdev_err(NULL, "hi6421 regulator no base adder,please check: ret=%d!\n", ret);
		goto err_free_mem;
	}

	pmu_out26m_enable = get_pmu_out26m_enable();
	if (pmu_out26m_enable){
		/*irda enable hi6421 clk_26M*/
		oclk=  readl(hi6421_regulator_data->base+TCXO_CTRL);

		oclk |= HI6421_CLK_26_EN;

		writel(oclk,hi6421_regulator_data->base+TCXO_CTRL);
	}

	/*get clock*/
	hi6421_regulator_data->clk_pmu = clk_get(NULL, "clk_pmuspi");
	if (IS_ERR(hi6421_regulator_data->clk_pmu)) {
		rdev_err(NULL, "hi6421 regulator clock get failed,please check!\n");
		ret = PTR_ERR(hi6421_regulator_data->clk_pmu);
		goto err_iounmap;
	}
	/*enable the clock*/
	ret = clk_enable(hi6421_regulator_data->clk_pmu);
	if (ret) {
		rdev_err(NULL, "hi6421 regulator clk_enable failed,please check!\n");
		goto err_clk_put;
	}

	/*register all regulator*/
	for (regulator_id = 0; regulator_id < ARRAY_SIZE(regulators); regulator_id++) {
		hi6421_regulator_data->rdev[regulator_id] = regulator_register(&regulators[regulator_id], &pdev->dev,
			regulator_init_data + regulator_id, hi6421_regulator_data);
		if (IS_ERR(hi6421_regulator_data->rdev[regulator_id])) {
			rdev_err(hi6421_regulator_data->rdev[regulator_id], "regulator register faile\n");
			ret = PTR_ERR(hi6421_regulator_data->rdev[regulator_id]);
			goto err_disable_clk;
		}
	}
	/*if there is a ldo current over,support ocp qudou 8ms*/
	reg_ocp = readl(hi6421_regulator_data->base + OCP_CURRENT_PROTECT_REG);
	reg_ocp = (reg_ocp & 0xF0) | OCP_CURRENT_PROTECT_MASK;
	writel(reg_ocp, hi6421_regulator_data->base + OCP_CURRENT_PROTECT_REG);
	platform_set_drvdata(pdev, hi6421_regulator_data);
	return 0;
err_disable_clk:
	clk_disable(hi6421_regulator_data->clk_pmu);
err_clk_put:
	clk_put(hi6421_regulator_data->clk_pmu);
err_iounmap:
	iounmap(hi6421_regulator_data->base);
err_free_mem:
	kfree(hi6421_regulator_data);
	return ret;
}
static __devexit int hi6421_regulator_remove(struct platform_device *pdev)
{
	int regulator_id;
	struct hi6421_regulator_data *hi6421_regulator_data;

	hi6421_regulator_data = platform_get_drvdata(pdev);
	if (NULL == hi6421_regulator_data) {
		pr_err("%s %d, platform_get_drvdata is NULL\n", __func__, __LINE__);
		return -1;
	}

	/*release map phy_addr and virtual_addr*/
	if (hi6421_regulator_data->base) {
		iounmap(hi6421_regulator_data->base);
	}
	/*disable clock*/
	if (!IS_ERR(hi6421_regulator_data->clk_pmu)) {
		clk_disable(hi6421_regulator_data->clk_pmu);
		clk_put(hi6421_regulator_data->clk_pmu);
	}
	for (regulator_id = 0; regulator_id < ARRAY_SIZE(regulators); regulator_id++) {
		if (!IS_ERR(hi6421_regulator_data->rdev[regulator_id])) {
			regulator_unregister(hi6421_regulator_data->rdev[regulator_id]);
		}
	}
	/*release memory*/
	kfree(hi6421_regulator_data);
	return 0;
}
#ifdef CONFIG_PM
static int hi6421_regulator_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct hi6421_regulator_data *hi6421_regulator_data;

	rdev_info(NULL, "hi6421 regulator entry suspend successfully");

	hi6421_regulator_data = platform_get_drvdata(pdev);
	if (NULL == hi6421_regulator_data) {
		pr_err("%s %d platform_get_drvdata NULL\n", __func__, __LINE__);
		return -1;
	}

	/*disable clock*/
	if (!IS_ERR(hi6421_regulator_data->clk_pmu)) {
		clk_disable(hi6421_regulator_data->clk_pmu);
	}

	return 0;
}
static int hi6421_regulator_resume(struct platform_device *pdev)
{
	int ret = 0;
	struct hi6421_regulator_data *hi6421_regulator_data;

	rdev_info(NULL, "hi6421 regulator entry resume successfully");

	hi6421_regulator_data = platform_get_drvdata(pdev);
	if (NULL == hi6421_regulator_data) {
		pr_err("%s %d platform_get_drvdata NULL\n", __func__, __LINE__);
		return -1;
	}

	if (!IS_ERR(hi6421_regulator_data->clk_pmu)) {
		ret = clk_enable(hi6421_regulator_data->clk_pmu);
		if (ret)
			rdev_err(NULL, "hi6421_regulator_resume enable clk failed,please check!\n");
	}

	return ret;
}
#endif
static struct platform_driver hi6421_ldo_driver = {
	.probe = hi6421_regulator_probe,
	.remove = hi6421_regulator_remove,
	#ifdef CONFIG_PM
	.suspend = hi6421_regulator_suspend,
	.resume = hi6421_regulator_resume,
	#endif
	.driver		= {
		.name	= "hi6421-regulator",
	},
};
static int __init hi6421_regulator_init(void)
{
	return platform_driver_register(&hi6421_ldo_driver);
}
static void __exit hi6421_regulator_exit(void)
{
	platform_driver_unregister(&hi6421_ldo_driver);
}
core_initcall(hi6421_regulator_init);
module_exit(hi6421_regulator_exit);
MODULE_LICENSE("GPL");
