/* drivers/misc/vibrator_k3.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: y36721
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
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/regulator/consumer.h>

#include "timed_output.h"
#include <linux/vibrator/k3_vibrator.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/clk.h>
#include <hsad/config_interface.h>
#ifdef CONFIG_ANDROID_K3_VIBRATOR_AUTO_CONTROL
#include <linux/hkadc/hiadc_hal.h>
#endif
#define PMU_LDO12_CTRL	                (0xfe2d4000 + (0x02C<<2))
#define HI6421_LDO_ENA					0x10		/* LDO_ENA */
#define HI6421_LDO_ENA_MASK				0x10		/* LDO_MASK */

struct k3_vibrator_data {
	struct timed_output_dev dev;
	struct k3_vibrator_platform_data *plat_data;
	struct hrtimer timer;
	struct mutex lock;
	struct clk *clk;
	void __iomem *k3_vibrator_base;
	int value;
	u8 freq;
	u8 power;
	u8 mode;
#ifdef CONFIG_ANDROID_K3_VIBRATOR_AUTO_CONTROL
	u8 battery_power;
#endif
};

static int motor_type=MOTOR_DEFAULT;

struct k3_vibrator_data *k3_vibrator_pdata;
static void k3_vibrator_onoff_handler(struct work_struct *data);
static struct workqueue_struct *done_queue;

static bool vib_reg_enabled=false;
static int k3_vibrator_vout_number;
static struct regulator *k3_vibrator_vout_reg=NULL;

#ifdef CONFIG_ANDROID_K3_VIBRATOR_AUTO_CONTROL
static unsigned long g_pre_set_time;
#define K3_VPH_PWR_ADC_CH   ADC_VBATMON
#endif

static DECLARE_WORK(done_work, k3_vibrator_onoff_handler);

static void k3_vibrator_reg_write(u8 vibrator_set, u32 vibrator_address)
{
	writel(vibrator_set, k3_vibrator_pdata->k3_vibrator_base + vibrator_address);
}

static void k3_vibrator_set_cfg(struct k3_vibrator_data *pdata, int value)
{
	if (value <= TIMEOUT_MORE) {
		pdata->freq = pdata->plat_data->high_freq;
		pdata->power = pdata->plat_data->high_power;
	} else {
		pdata->freq = pdata->plat_data->low_freq;
		pdata->power = pdata->plat_data->low_power;
	}

	return ;
}

static void k3_vibrator_onoff(int on)
{
	struct k3_vibrator_data *pdata = k3_vibrator_pdata;
	pdata->value = on;
	queue_work(done_queue, &done_work);
}

#ifdef CONFIG_ANDROID_K3_VIBRATOR_AUTO_CONTROL
static int k3_vibrator_get_vphpwr_vol(int *voltage)
{
	unsigned char reserve = 0;
	int value = 0;
	int ret = 0;

	/* open temperature channel */
	ret = k3_adc_open_channel(K3_VPH_PWR_ADC_CH);
	if (ret < 0) {
		pr_err("%s:open adc channel failed(ret=%d channel_no=%d)\n", __func__, ret, ADC_VBATMON);
		return ret;
	}
	/* mdelay(1); */
	/* get the temperature voltage value */
	value = k3_adc_get_value(K3_VPH_PWR_ADC_CH, &reserve);
	/* release the resource */
	ret = k3_adc_close_channal(K3_VPH_PWR_ADC_CH);
	if (ret < 0) {
		pr_err(KERN_ERR "%s:close adc channel failed(ret=%d channel_no=%d)\n", __func__, ret, ADC_VBATMON);
		return ret;
	}
	*voltage = value;

	return 0;
}

struct k3_vph_pwr_vol_vib_iset k3_vibrator_table_vbat[] = {
	{3650, 0xff},
	{3700, 0xfe},
	{3750, 0xfc},
	{3800, 0xfb},
	{3850, 0xf9},
	{3900, 0xf8},
	{3950, 0xf6},
	{4000, 0xf5},
	{4050, 0xf4},
	{4100, 0xf2},
	{4150, 0xf1},
	{4200, 0xef},
	{4250, 0xed},// for 4.35V new battery
	{4300, 0xec},
	{4350, 0xeb},
};

/* Get the register of DR2_ISET */
/* @test_voltage: virtual VBAT voltage if true, else get real VBAT voltage */
static int k3_vibrator_get_iset_value(int test_voltage)
{
	int iset_val = k3_vibrator_pdata->power;
	int vph_pwr_vol = 0;
	int array_len = 0;
	int count = 0;
	struct k3_vph_pwr_vol_vib_iset *pArray = NULL;

	/* NOTES: test_voltage is test use,we remove the debug interface now,
	 * but for capacity,we still reserve it.
	 */
	if (!test_voltage) {
		if (k3_vibrator_get_vphpwr_vol(&vph_pwr_vol)) {
			pr_err("%s:get vph-pwr failed,iset_val = %d\n", __func__, iset_val);
			return iset_val;
		}
		/* Vibrator voltage default on 2.8V */
		pArray = k3_vibrator_table_vbat;

		array_len = ARRAY_SIZE(k3_vibrator_table_vbat);
		if (vph_pwr_vol < pArray->vph_voltage) {
			iset_val = pArray->vreg_value;
		} else if (vph_pwr_vol > (pArray + array_len - 1)->vph_voltage) {
			iset_val = (pArray + array_len - 1)->vreg_value;
		} else {
			count = (vph_pwr_vol - pArray->vph_voltage)/((pArray+1)->vph_voltage - pArray->vph_voltage);
			if (count < (array_len - 1))
				iset_val = (pArray + count)->vreg_value;
			else
				iset_val = (pArray + array_len - 1)->vreg_value;
		}
	}
	/* pr_debug("%s: Battery voltage = %d, count = %d,iset_val = 0x%02x\n", __func__, vph_pwr_vol, count, iset_val); */

	return iset_val;
}
#endif

void k3_vibrator_regulator_enable(void)
{
	int error;

	if (true == vib_reg_enabled)
		return ;

	BUG_ON(IS_ERR(k3_vibrator_vout_reg));

	error = regulator_enable(k3_vibrator_vout_reg);
	if (error < 0) {
		pr_err( "%s: failed to enable edge vibrator vdd\n", __func__);
		return ;
	}

	vib_reg_enabled = true;
	return ;
}

void k3_vibrator_regulator_disable(void)
{
	int error;

	if (false == vib_reg_enabled)
		return ;

	BUG_ON(IS_ERR(k3_vibrator_vout_reg));

	error = regulator_disable(k3_vibrator_vout_reg);
	if (error < 0) {
		pr_err( "%s: failed to disable edge vibrator vdd\n", __func__);
		return ;
	}

	vib_reg_enabled = false;
	return ;
}

static void k3_vibrator_onoff_handler(struct work_struct *data)
{
	struct k3_vibrator_data *pdata = k3_vibrator_pdata;
	int on = pdata->value;
	int ret = 0;
	u32 tmp;
	mutex_lock(&pdata->lock);
	ret = clk_enable(pdata->clk);
	if (ret) {
		pr_err("pmu clock enable failed,ret:%d\n", ret);
		mutex_unlock(&pdata->lock);
		return ;
	}

	if (on) {
#ifdef CONFIG_ANDROID_K3_VIBRATOR_AUTO_CONTROL
		k3_vibrator_reg_write(pdata->battery_power, DR2_ISET);
#else
		k3_vibrator_reg_write(pdata->power, DR2_ISET);
#endif
		switch(motor_type)
		{
			case MOTOR_LDO:
				k3_vibrator_regulator_enable();
				break;
			default:
				k3_vibrator_reg_write(pdata->freq | pdata->mode, DR2_CTRL);
				break;
		}
	} else {
		switch(motor_type)
		{
			case MOTOR_LDO:
				k3_vibrator_regulator_disable();
				break;
			default:
				k3_vibrator_reg_write(DR2_DISABLE, DR2_CTRL);
				break;
		}
	}
	clk_disable(pdata->clk);
	mutex_unlock(&pdata->lock);
	return ;
}

static enum hrtimer_restart k3_vibrator_timer_func(struct hrtimer *timer)
{
	k3_vibrator_onoff(0);

	return HRTIMER_NORESTART;
}

static int k3_vibrator_get_time(struct timed_output_dev *dev)
{
	struct k3_vibrator_data *pdata =
			container_of(dev, struct k3_vibrator_data, dev);
	if (hrtimer_active(&pdata->timer)) {
		ktime_t r = hrtimer_get_remaining(&pdata->timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	} else
		return 0;
}

static void k3_vibrator_enable(struct timed_output_dev *dev, int value)
{
	struct k3_vibrator_data *pdata = container_of(dev, struct k3_vibrator_data, dev);
#ifdef CONFIG_ANDROID_K3_VIBRATOR_AUTO_CONTROL
	static int set_count;
#endif

	if (value < 0) {
		pr_err("error:vibrator_enable value:%d is negative\n", value);
		return;
	}
	/* cancel previous timer */
	if (hrtimer_active(&pdata->timer))
		hrtimer_cancel(&pdata->timer);

	if (value > 0) {
#ifdef CONFIG_ANDROID_K3_VIBRATOR_AUTO_CONTROL
		if (time_after(jiffies, g_pre_set_time+60*HZ)) {
			g_pre_set_time = jiffies;
			set_count = 0;
		}
		if (set_count == 0)
			pdata->battery_power = k3_vibrator_get_iset_value(0);

		set_count = (set_count+1)%50;
#endif

		if (value < TIMEOUT_MIN)
			value = TIMEOUT_MIN;
		k3_vibrator_set_cfg(pdata, value);
		k3_vibrator_onoff(1);
		hrtimer_start(&pdata->timer,
			ktime_set(value / 1000, (value % 1000) * 1000000),
			HRTIMER_MODE_REL);
	} else {
		k3_vibrator_onoff(0);
	}
}

int k3_vibrator_get_vout(struct platform_device *pdev){
	char req_reg_name[32]={0};
	int min_voltage=0;
	int max_voltage=0;

	k3_vibrator_vout_number = get_vibrator_vout_number();

	if (-1 == k3_vibrator_vout_number){
		pr_err( "%s: ldo motor get vibrator vout fail\n", __func__);
		return -EPERM;
	}

	snprintf(req_reg_name, sizeof(req_reg_name),
		"vdd-vib-ldo%d", k3_vibrator_vout_number);
	k3_vibrator_vout_reg =
	regulator_get(&pdev->dev, req_reg_name);

	if (IS_ERR(k3_vibrator_vout_reg)){
	pr_err( "%s: k3_vibrator_vout_reg error\n", __func__);
		return -EPERM;
	}

	min_voltage = get_vibrator_vout_min_voltage();
	max_voltage = get_vibrator_vout_max_voltage();

	if (-1 == min_voltage || -1 == max_voltage) return 0;

	if (regulator_set_voltage(k3_vibrator_vout_reg, min_voltage, max_voltage)){
		pr_err( "%s: vibrator set voltage error\n", __func__);
		return -EPERM;
	}

	return 0;
}

static int k3_vibrator_probe(struct platform_device *pdev)
{
	struct k3_vibrator_data *p_data;
	struct resource *res;
	int ret = 0;

	if (MOTOR_LDO == motor_type){
		ret = k3_vibrator_get_vout(pdev);
		if (ret) {
			dev_err(&pdev->dev, "failed to get vib vout\n");
			return ret;
		}
	}

	p_data = kzalloc(sizeof(struct k3_vibrator_data), GFP_KERNEL);
	if (p_data == NULL) {
		dev_err(&pdev->dev, "failed to allocate vibrator_device\n");
		return -ENOMEM;
	}

	/*get the clock with con_id = "clk_pmuspi"*/
	p_data->clk = clk_get(NULL, "clk_pmuspi");
	if (p_data->clk == NULL) {
		dev_err(&pdev->dev, "pmu clock get failed\n");
		ret = -ENODEV;
		goto err;
	}

	/* get base_addres */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to find registers\n");
		ret = -ENXIO;
		goto err_clk;
	}
	p_data->k3_vibrator_base = ioremap(res->start, resource_size(res));
	if (p_data->k3_vibrator_base == 0) {
		dev_err(&pdev->dev, "failed to map registers\n");
		ret = -ENXIO;
		goto err_clk;
	}

	/* init timer */
	hrtimer_init(&p_data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	p_data->timer.function = k3_vibrator_timer_func;

	/* init lock */
	mutex_init(&p_data->lock);

	p_data->plat_data = pdev->dev.platform_data;

	if (NULL == p_data->plat_data) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "failed get platform_data.\n");
		goto err_remap;
	}

	p_data->mode = p_data->plat_data->mode;
	p_data->freq = p_data->plat_data->low_freq;
	p_data->power = p_data->plat_data->low_power;

	p_data->dev.name = K3_VIBRATOR;
	p_data->dev.get_time = k3_vibrator_get_time;
	p_data->dev.enable = k3_vibrator_enable;

	ret = timed_output_dev_register(&p_data->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to regist dev\n");
		goto err_remap;
	}
	platform_set_drvdata(pdev, p_data);

	k3_vibrator_pdata = p_data;

	/* create a single thread workquene */
	done_queue = create_workqueue("done_queue");
	if (!done_queue) {
		dev_err(&pdev->dev, "failed to creat workqueue\n");
		ret = -ENOMEM;
		goto err_regis;
	}

#ifdef CONFIG_ANDROID_K3_VIBRATOR_AUTO_CONTROL
	g_pre_set_time = jiffies ;
	p_data->battery_power = k3_vibrator_get_iset_value(0);
	/* Bugfix:there are risk if not enable clk */
	ret = clk_enable(p_data->clk);
	if (ret) {
		pr_err("pmu clock enable failed,ret:%d\n", ret);
		goto err_regis;
	}

	k3_vibrator_reg_write(p_data->battery_power, DR2_ISET);
	clk_disable(p_data->clk);
	dev_info(&pdev->dev, "Vibrator auto control with the battery voltage change feature enable\n");
#endif
	return 0;

err_regis:
	timed_output_dev_unregister(&p_data->dev);
err_remap:
	iounmap(p_data->k3_vibrator_base);
err_clk:
	clk_put(p_data->clk);
	p_data->clk = NULL;
err:
	kfree(p_data);
	p_data = NULL;
	return ret;
}

static int k3_vibrator_remove(struct platform_device *pdev)
{
	struct k3_vibrator_data *pdata = platform_get_drvdata(pdev);

	if (pdata == NULL) {
		dev_err(&pdev->dev, "%s:pdata is NULL\n", __func__);
		return -ENODEV;
	}

	if (hrtimer_active(&pdata->timer))
		hrtimer_cancel(&pdata->timer);

	timed_output_dev_unregister(&pdata->dev);

	clk_put(pdata->clk);
	iounmap(pdata->k3_vibrator_base);
	kfree(pdata);
	pdata = NULL;
	destroy_workqueue(done_queue);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static void k3_vibrator_shutdown(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_vibrator_data *pdata = platform_get_drvdata(pdev);
	if (pdata == NULL) {
		dev_err(&pdev->dev, "%s:pdata is NULL\n", __func__);
		return;
	}
	printk("[%s] +\n", __func__);

	if (hrtimer_active(&pdata->timer))
		hrtimer_cancel(&pdata->timer);


	ret = clk_enable(pdata->clk);
	if (ret) {
		dev_err(&pdev->dev, "pmu clock enable failed,ret:%d\n", ret);
	} else {
		k3_vibrator_reg_write(DR2_DISABLE, DR2_CTRL);
		clk_disable(pdata->clk);
	}

	clk_put(pdata->clk);

	printk("[%s] -\n", __func__);
	return ;
}

#ifdef CONFIG_PM
static int  k3_vibrator_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct k3_vibrator_data *pdata = platform_get_drvdata(pdev);
	int ret;
	if (pdata == NULL) {
		dev_err(&pdev->dev, "%s:pdata is NULL\n", __func__);
		return -ENODEV;
	}
	printk("[%s] +\n", __func__);
	if (!mutex_trylock(&pdata->lock)) {
		dev_err(&pdev->dev, "%s: mutex_trylock.\n", __func__);
		return -EAGAIN;
	}
	if (hrtimer_active(&pdata->timer)) {
		ret = clk_enable(pdata->clk);
		if (ret) {
			pr_err("pmu clock enable failed,ret:%d\n", ret);
			mutex_unlock(&pdata->lock);
			return ret;
		}
		hrtimer_cancel(&pdata->timer);
		k3_vibrator_reg_write(DR2_DISABLE, DR2_CTRL);
		clk_disable(pdata->clk);
	}
	printk("[%s] -\n", __func__);
	return 0;
}

static int k3_vibrator_resume(struct platform_device *pdev)
{
	struct k3_vibrator_data *pdata = platform_get_drvdata(pdev);
	if (pdata == NULL) {
		dev_err(&pdev->dev, "%s:pdata is NULL\n", __func__);
		return -ENODEV;
	}
	printk("[%s] +\n", __func__);
	mutex_unlock(&pdata->lock);
	printk("[%s] -\n", __func__);
	return 0;
}
#endif

static struct platform_driver k3_vibrator_driver = {
	.probe  = k3_vibrator_probe,
	.remove = k3_vibrator_remove,
	.shutdown	= k3_vibrator_shutdown,
#ifdef CONFIG_PM
	.suspend	= k3_vibrator_suspend,
	.resume		= k3_vibrator_resume,
#endif
	.driver = {
		.name   = K3_VIBRATOR,
		.owner  = THIS_MODULE,
	},
};

static int __init k3_vibrator_init(void)
{
    motor_type = get_motor_board_type();
    switch(motor_type)
    {
        case MOTOR_DEFAULT:
        case MOTOR_LDO:
            return platform_driver_register(&k3_vibrator_driver);
        default:
            return -EPERM;
    }
}

static void __exit k3_vibrator_exit(void)
{
    switch(motor_type)
    {
        case MOTOR_DEFAULT:
            platform_driver_unregister(&k3_vibrator_driver);
            break;
        case MOTOR_LDO:
            k3_vibrator_vout_number = -1;
            if (!IS_ERR(k3_vibrator_vout_reg))
                regulator_put(k3_vibrator_vout_reg);

            platform_driver_unregister(&k3_vibrator_driver);
            break;
        default:
            break;
    }
}

module_init(k3_vibrator_init);
module_exit(k3_vibrator_exit);

MODULE_AUTHOR("skf57909");
MODULE_DESCRIPTION(" k3 vibrator driver");
MODULE_LICENSE("GPL");

