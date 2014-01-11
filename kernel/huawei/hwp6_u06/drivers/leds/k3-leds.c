/*
 * LEDs driver for k3
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/led/k3-leds.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <hsad/config_mgr.h>

int light_red = 0;
int light_green = 0;
int light_blue = 0;
int light_red_bright = 0;
int light_green_bright = 0;
int light_blue_bright = 0;
int rising_and_falling = 0x33; //default Value, means 1s to rising and falling
int start_time = DR_START_DEL_512;
unsigned long light_red_delay_on = 0;
unsigned long light_red_delay_off = 0;
unsigned long light_green_delay_on = 0;
unsigned long light_green_delay_off = 0;
unsigned long light_blue_delay_on = 0;
unsigned long light_blue_delay_off = 0;
static struct k3_led_drv_data *k3_led_pdata;

/* read register  */
static unsigned long k3_led_reg_read(u32 led_address)
{
	return readl(k3_led_pdata->k3_led_base + led_address);
}
/* write register  */
static void k3_led_reg_write(u8 led_set, u32 led_address)
{
	writel(led_set, k3_led_pdata->k3_led_base + led_address);
}
/* set led off  */
static void k3_led_set_disable(u8 id)
{
	unsigned long led_k3_dr_ctl;

	led_k3_dr_ctl = k3_led_reg_read(DR_LED_CTRL);

	switch (id) {

	case K3_LED0:
		led_k3_dr_ctl &= DR3_DISABLE;
                light_red_bright = 0;
		break;

	case K3_LED1:
		led_k3_dr_ctl &= DR4_DISABLE;
                light_green_bright = 0;
		break;

	case K3_LED2:
		led_k3_dr_ctl &= DR5_DISABLE;
                light_blue_bright = 0;
		break;

	default:
		break;
	}

	k3_led_reg_write(led_k3_dr_ctl, DR_LED_CTRL);
}
static void k3_led_set_reg_write(struct led_set_config *bightness_config)
{
	/* config current  */
	k3_led_reg_write(bightness_config->brightness_set, bightness_config->k3_led_iset_address);
	/* start_delay  */
	k3_led_reg_write(start_time, bightness_config->k3_led_start_address);
	/* delay_on  */
	k3_led_reg_write(DR_DELAY_ON, bightness_config->k3_led_tim_address);
	/* enable */
	k3_led_reg_write(bightness_config->led_k3_dr_ctl, DR_LED_CTRL);
	/* output enable */
	k3_led_reg_write(bightness_config->led_k3_dr_out_ctl, DR_OUT_CTRL);
	/*set rising and falling*/
	k3_led_reg_write(rising_and_falling,bightness_config->k3_led_tim1_address );
}

/* set led half brightness or full brightness  */
static void k3_led_set_select_led(u8 brightness_set, u8 id)
{
	unsigned long led_k3_dr_ctl;
	unsigned long led_k3_dr_out_ctl;
	struct led_set_config bightness_config[K3_LEDS_MAX];

	led_k3_dr_ctl = k3_led_reg_read(DR_LED_CTRL);
	led_k3_dr_out_ctl = k3_led_reg_read(DR_OUT_CTRL);

	switch (id) {

	case K3_LED0:
		bightness_config[id].brightness_set = brightness_set;
		bightness_config[id].k3_led_iset_address = DR3_ISET;
		bightness_config[id].k3_led_start_address = DR3_START_DEL;
		bightness_config[id].k3_led_tim_address = DR3_TIM_CONF0;
		bightness_config[id].led_k3_dr_ctl = led_k3_dr_ctl | DR3_ENABLE;
		bightness_config[id].led_k3_dr_out_ctl = led_k3_dr_out_ctl & DR3_OUT_ENABLE;
		bightness_config[id].k3_led_tim1_address = DR3_TIM_CONF1;
                light_red = id;
                light_red_bright = brightness_set;
		break;

	case K3_LED1:
		bightness_config[id].brightness_set = brightness_set;
		bightness_config[id].k3_led_iset_address = DR4_ISET;
		bightness_config[id].k3_led_start_address = DR4_START_DEL;
		bightness_config[id].k3_led_tim_address = DR4_TIM_CONF0;
		bightness_config[id].led_k3_dr_ctl = led_k3_dr_ctl | DR4_ENABLE;
		bightness_config[id].led_k3_dr_out_ctl = led_k3_dr_out_ctl & DR4_OUT_ENABLE;
		bightness_config[id].k3_led_tim1_address = DR4_TIM_CONF1;
                light_green = id;
                light_green_bright = brightness_set;
		break;

	case K3_LED2:
		bightness_config[id].brightness_set = brightness_set;
		bightness_config[id].k3_led_iset_address = DR5_ISET;
		bightness_config[id].k3_led_start_address = DR5_START_DEL;
		bightness_config[id].k3_led_tim_address = DR5_TIM_CONF0;
		bightness_config[id].led_k3_dr_ctl = led_k3_dr_ctl | DR5_ENABLE;
		bightness_config[id].led_k3_dr_out_ctl = led_k3_dr_out_ctl & DR5_OUT_ENABLE;
		bightness_config[id].k3_led_tim1_address = DR5_TIM_CONF1;
                light_blue = id;
                light_blue_bright = brightness_set;
		break;

	default:
		break;
	}
	k3_led_set_reg_write(&bightness_config[id]);
}

/* set registers of dr3,4,5 */
static int k3_led_set(struct k3_led_data *led, u8 brightness)
{
	int ret = 0;
	u8 id = led->id;
	struct k3_led_drv_data *data = k3_led_pdata;

	if (brightness != LED_OFF && brightness != LED_HALF && brightness != LED_FULL) {
		pr_err("k3_led_set brightness:%d is error\n", brightness);
		ret = -EINVAL;
		return ret;
	}

	mutex_lock(&data->lock);
	ret = clk_enable(data->clk);
	if (ret) {
		pr_err("pmu clock enable failed,ret:%d\n", ret);
		mutex_unlock(&data->lock);
		return ret;
	}

	switch (id) {

	case K3_LED0:
	case K3_LED1:
	case K3_LED2:
		if (brightness == LED_OFF) {
			/* set led off */
			k3_led_set_disable(id);
		} else if (brightness == LED_HALF) {
			/* set led half brightness */
			k3_led_set_select_led(DR_BRIGHTNESS_HALF, id);
		} else {
			/* set led full brightness */
			k3_led_set_select_led(DR_BRIGHTNESS_FULL, id);
		}
		break;
	default:
		pr_err("k3_led_set id:%d is error\n", id);
		ret = -EINVAL;
		break;
	}

	clk_disable(data->clk);
	mutex_unlock(&data->lock);
	return ret;
}
static void k3_led_set_blink_reg_write(u8 id, u8 set_time)
{
	u32 led_address = DR3_TIM_CONF0;
	u32 led_change_address = DR3_TIM_CONF1;
	u8 set_chang_time = 0x00;

	switch (id) {

	case K3_LED0:
		led_address = DR3_TIM_CONF0;
		led_change_address = DR3_TIM_CONF1;
		break;

	case K3_LED1:
		led_address = DR4_TIM_CONF0;
		led_change_address = DR4_TIM_CONF1;
		break;

	case K3_LED2:
		led_address = DR5_TIM_CONF0;
		led_change_address = DR5_TIM_CONF1;
		break;

	default:
		break;
	}

	k3_led_reg_write(set_time, led_address);
	k3_led_reg_write(set_chang_time, led_change_address);

}
/* get the set time in area */
static u8 k3_led_get_time(unsigned long delay, u8 flag)
{
	u8 set_time = 0;
	if (delay <= DEL_500)
		set_time = DR_DEL01;
	else if (delay <= DEL_1000)
		set_time = DR_DEL02;
	else if (delay <= DEL_2000)
		set_time = DR_DEL03;
	else if (delay <= DEL_4000)
		set_time = DR_DEL04;
	else if (delay <= DEL_6000)
		set_time = DR_DEL05;
	else if (delay <= DEL_8000)
		set_time = DR_DEL06;
	else if (delay <= DEL_12000)
		set_time = DR_DEL07;
	else if (delay <= DEL_14000)
		set_time = DR_DEL08;
	else
		set_time = DR_DEL09;

	if (flag)
		return set_time << 4;
	else
		return set_time;
}

/* config of  dr3,4,5 delay_on and delay_off registers */
static int k3_led_set_blink(struct led_classdev *led_ldev,
	unsigned long *delay_on, unsigned long *delay_off)
{
	struct k3_led_data *led_dat =
		container_of(led_ldev, struct k3_led_data, ldev);

	struct k3_led_drv_data *data = k3_led_pdata;
	u8 id = led_dat->id;
	int ret = 0;
	u8 set_time_on = 0;
	u8 set_time_off = 0;

	if ((*delay_on == 0) && (*delay_off == 0))
		return ret;
        if (id == 0){
            light_red_delay_on = *delay_on;
            light_red_delay_off = *delay_off;
        }
        if (id == 1){
            light_green_delay_on = *delay_on;
            light_green_delay_off = *delay_off;
        }
        if (id == 2){
            light_blue_delay_on = *delay_on;
            light_blue_delay_off = *delay_off;
        }
	mutex_lock(&data->lock);
	ret = clk_enable(data->clk);
	if (ret) {
		pr_err("pmu clock enable failed,ret:%d\n", ret);
		mutex_unlock(&data->lock);
		return ret;
	}

	switch (id) {

	case K3_LED0:
	case K3_LED1:
	case K3_LED2:
		led_ldev->blink_delay_on =  *delay_on;
		led_ldev->blink_delay_off = *delay_off;

		set_time_on = k3_led_get_time(*delay_on, DELAY_ON);
		set_time_off = k3_led_get_time(*delay_off, DELAY_OFF);
		k3_led_set_blink_reg_write(id, set_time_on | set_time_off);
		break;
	default:
		pr_err("k3_led_set_blink id:%d is error\n", id);
		ret = -EINVAL;
		break;
	}

	clk_disable(data->clk);
	mutex_unlock(&data->lock);
	return ret;
}

/* set brightness of dr3,4,5 lights*/
static void  k3_led_set_brightness(struct led_classdev *led_ldev,
				      enum led_brightness brightness)
{
	struct k3_led_data *led =
		container_of(led_ldev, struct k3_led_data, ldev);

	led->status.brightness = brightness;
	k3_led_set(led, led->status.brightness);
}

/* config lights */
static int k3_led_configure(struct platform_device *pdev,
				struct k3_led_drv_data *data,
				struct k3_led_platform_data *pdata)
{
	int i;
	int ret = 0;

	for (i = 0; i < pdata->leds_size; i++) {
		struct k3_led *pled = &pdata->leds[i];
		struct k3_led_data *led = &data->leds[i];
		led->id = i;

		led->status.brightness = pled->brightness;
		led->status.delay_on = pled->delay_on;
		led->status.delay_off = pled->delay_off;
		led->ldev.name = pled->name;
		led->ldev.default_trigger = pled->default_trigger;
		led->ldev.max_brightness = LED_FULL;
		led->ldev.blink_set = k3_led_set_blink;
		led->ldev.brightness_set = k3_led_set_brightness;
		//led->ldev.flags = LED_CORE_SUSPENDRESUME;

		ret = led_classdev_register(&pdev->dev, &led->ldev);
		if (ret < 0) {
			dev_err(&pdev->dev,
					"couldn't register LED %s\n",
					led->ldev.name);
			goto exit;
		}
#if 0
		/* to expose the default value to userspace */
		led->ldev.brightness = led->status.brightness;

		/* Set the default led status */
		ret = k3_led_set(led, led->status.brightness);
		if (ret < 0) {
			dev_err(&pdev->dev,
					"%s couldn't set STATUS %d\n",
					led->ldev.name, led->status.brightness);
			goto exit;
		}
#endif
	}
	return 0;

exit:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--) {
			led_classdev_unregister(&data->leds[i].ldev);
		}
	}
	return ret;
}
static int __devinit k3_led_probe(struct platform_device *pdev)
{
	struct k3_led_platform_data *pdata = pdev->dev.platform_data;
	struct k3_led_drv_data *data;
	struct resource *res;
	int ret = 0 ;

	if (pdata == NULL) {
		dev_err(&pdev->dev, "no platform data\n");
		return -EINVAL;
	}

	data = kzalloc(sizeof(struct k3_led_drv_data), GFP_KERNEL);
	if (!data) {
		dev_err(&pdev->dev, "failed to allocate led_device\n");
		return -ENOMEM;
	}

	/* get the clock with con_id = "clk_pmuspi" */
	data->clk = clk_get(NULL, "clk_pmuspi");
	if (data->clk == NULL) {
		dev_err(&pdev->dev, "pmu clock get failed\n");
		ret = -ENODEV;
		goto err;
	}

	/* get base_address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to find registers\n");
		ret = -ENXIO;
		goto err_clk;
	}
	data->k3_led_base = ioremap(res->start, resource_size(res));
	if (data->k3_led_base == 0) {
		dev_err(&pdev->dev, "failed to map registers\n");
		ret = -ENXIO;
		goto err_clk;
	}

	mutex_init(&data->lock);

	platform_set_drvdata(pdev, data);
	k3_led_pdata = data;
	/*config lights*/
	ret = k3_led_configure(pdev, data, pdata);
	if (ret < 0)
		goto err_remap;


	ret = get_hw_config_int("sensor/rising_and_falling", &rising_and_falling, NULL);
	if(!ret)
		dev_err(&pdev->dev, "failed to get rising and falling value from boardid\n");
	if(0 == rising_and_falling)
		start_time = 0x00;

	return 0;

err_remap:
	iounmap(data->k3_led_base);
err_clk:
	clk_put(data->clk);
	data->clk = NULL;
err:
	kfree(data);
	data = NULL;
	return ret;
}

static int __devexit k3_led_remove(struct platform_device *pdev)
{
	struct k3_led_platform_data *pdata = pdev->dev.platform_data;
	struct k3_led_drv_data *data = platform_get_drvdata(pdev);
	int i;

	if (data == NULL) {
		dev_err(&pdev->dev, "%s:data is NULL\n", __func__);
		return -ENODEV;
	}

	for (i = 0; i < pdata->leds_size; i++) {
		led_classdev_unregister(&data->leds[i].ldev);
	}

	clk_put(data->clk);
	iounmap(data->k3_led_base);
	kfree(data);
	data = NULL;
	platform_set_drvdata(pdev, NULL);
	return 0;
}
static void k3_led_shutdown(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_led_drv_data *data = platform_get_drvdata(pdev);

	if (data == NULL) {
		dev_err(&pdev->dev, "%s:data is NULL\n", __func__);
		return;
	}
	printk("[%s] +\n", __func__);

	ret = clk_enable(data->clk);
	if (ret) {
		dev_err(&pdev->dev, "pmu clock enable failed,ret:%d\n", ret);
	} else {
		k3_led_reg_write(DR_CONTR_DISABLE, DR_LED_CTRL);
		clk_disable(data->clk);
	}

	clk_put(data->clk);

	printk("[%s] -\n", __func__);

	return ;
}

#ifdef CONFIG_PM
static int  k3_led_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct k3_led_drv_data *data = platform_get_drvdata(pdev);
	if (data == NULL) {
		dev_err(&pdev->dev, "%s:data is NULL\n", __func__);
		return -ENODEV;
	}
	printk("[%s] +\n", __func__);

	if (!mutex_trylock(&data->lock)) {
		dev_err(&pdev->dev, "%s: mutex_trylock.\n", __func__);
		return -EAGAIN;
	}
	printk("[%s] -\n", __func__);
	return 0;
}
static int k3_led_resume(struct platform_device *pdev)
{

	struct k3_led_drv_data *data = platform_get_drvdata(pdev);
	if (data == NULL) {
		dev_err(&pdev->dev, "%s:data is NULL\n", __func__);
		return -ENODEV;
	}
	printk("[%s] +\n", __func__);
	mutex_unlock(&data->lock);
	printk("[%s] -\n", __func__);
	return 0;
}
#endif

static struct platform_driver k3_led_driver = {
	.probe		= k3_led_probe,
	.remove		= __devexit_p(k3_led_remove),
	.shutdown	= k3_led_shutdown,
#ifdef CONFIG_PM
	.suspend	= k3_led_suspend,
	.resume		= k3_led_resume,
#endif
	.driver		= {
		.name	= K3_LEDS,
		.owner	= THIS_MODULE,
	},
};

MODULE_ALIAS("platform:k3-leds");
static int __init k3_led_init(void)
{
	return platform_driver_register(&k3_led_driver);
}

static void __exit k3_led_exit(void)
{
	platform_driver_unregister(&k3_led_driver);
}

module_init(k3_led_init);
module_exit(k3_led_exit);

MODULE_AUTHOR("skf57909");
MODULE_DESCRIPTION("k3 LED driver");
MODULE_LICENSE("GPL");

