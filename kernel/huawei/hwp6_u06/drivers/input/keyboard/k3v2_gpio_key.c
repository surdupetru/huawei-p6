/*
 * Filename:kernel/drivers/input/keyboard/k3v2_gpio_key.c
 *
 * Discription:using gpio_137 realizing volume-up-key and gpio_138
 * realizing volumn-down-key instead of KPC in kernel, only support simple
 * key-press at currunt version, not support combo-keys.
 *
 * Copyright (C) 2011 Hisilicon
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *
 * Revision history:
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/mux.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <asm/irq.h>
#include <mach/k3_keypad.h>
#include <mach/irqs.h>
#include <mach/gpio.h>
#include <mach/platform.h>
#include <hsad/config_interface.h>

#define TRUE				(1)
#define FALSE				(0)
#define GPIO_KEY_RELEASE    (0)
#define GPIO_KEY_PRESS      (1)
#define GPIO_HIGH_VOLTAGE   (1)
#define GPIO_LOW_VOLTAGE    (0)

static unsigned int g_keyup      = 1;
static unsigned int g_keydown = 1;
static unsigned int g_camerafocus=1;
static unsigned int g_camera=1;

struct k3v2_gpio_key {
	struct input_dev *input_dev;
	struct delayed_work gpio_keyup_work;
	struct delayed_work gpio_keydown_work;
	struct delayed_work gpio_camera_work;
	struct delayed_work gpio_camerafocus_work;
	int    			volume_up_irq;      /*volumn up key irq.*/
	int    			volume_down_irq;    /*volumn down key irq.*/
	int				camera_irq;/*camera key irq.*/
	int				camera_focus_irq;/*camera_focus key irq.*/
};

static int k3v2_gpio_key_open(struct input_dev *dev)
{
	return 0;
}

static void k3v2_gpio_key_close(struct input_dev *dev)
{
	return;
}

static void k3v2_gpio_keyup_work(struct work_struct *work)
{
	struct k3v2_gpio_key *gpio_key = container_of(work,
		struct k3v2_gpio_key, gpio_keyup_work.work);

	unsigned int keyup_value = 0;
	unsigned int report_action = GPIO_KEY_RELEASE;

	keyup_value = gpio_get_value(GPIO_17_1);
	if (g_keyup == keyup_value)
		return;
	else
		g_keyup = keyup_value;

	/*judge key is pressed or released.*/
	if (keyup_value == GPIO_LOW_VOLTAGE)
		report_action = GPIO_KEY_PRESS;
	else if (keyup_value == GPIO_HIGH_VOLTAGE)
		report_action = GPIO_KEY_RELEASE;
	else {
		printk(KERN_ERR "[gpiokey][%s]invalid gpio key_value.\n", __FUNCTION__);
		return;
	}

	printk(KERN_DEBUG "[gpiokey]volumn key %u action %u\n", KEY_VOLUMEUP, report_action);
	input_report_key(gpio_key->input_dev, KEY_VOLUMEUP, report_action);
	input_sync(gpio_key->input_dev);

	return;
}

static void k3v2_gpio_keydown_work(struct work_struct *work)
{
	struct k3v2_gpio_key *gpio_key = container_of(work,
		struct k3v2_gpio_key, gpio_keydown_work.work);

	unsigned int keydown_value = 0;
	unsigned int report_action = GPIO_KEY_RELEASE;

	keydown_value = gpio_get_value(GPIO_17_2);
	if (g_keydown == keydown_value)
		return;
	else
		g_keydown = keydown_value;

	/*judge key is pressed or released.*/
	if (keydown_value == GPIO_LOW_VOLTAGE)
		report_action = GPIO_KEY_PRESS;
	else if (keydown_value == GPIO_HIGH_VOLTAGE)
		report_action = GPIO_KEY_RELEASE;
	else {
		printk(KERN_ERR "[gpiokey][%s]invalid gpio key_value.\n", __FUNCTION__);
		return;
	}

	printk(KERN_DEBUG "[gpiokey]volumn key %u action %u\n", KEY_VOLUMEDOWN, report_action);
	input_report_key(gpio_key->input_dev, KEY_VOLUMEDOWN, report_action);
	input_sync(gpio_key->input_dev);

	return;
}

static void k3v2_gpio_camerafocus_work(struct work_struct * work)
{
	struct k3v2_gpio_key *gpio_key = container_of(work,
		struct k3v2_gpio_key, gpio_camerafocus_work.work);

	unsigned int keycamerafocus_value = 0;
	unsigned int report_action = GPIO_KEY_RELEASE;

	keycamerafocus_value = gpio_get_value(GPIO_17_6);

	if (g_camerafocus== keycamerafocus_value)
		return;
	else
		g_camerafocus= keycamerafocus_value;

	/*judge key is pressed or released.*/
	if (keycamerafocus_value == GPIO_LOW_VOLTAGE)
		report_action = GPIO_KEY_PRESS;
	else if (keycamerafocus_value == GPIO_HIGH_VOLTAGE)
		report_action = GPIO_KEY_RELEASE;
	else {
		printk(KERN_ERR "[gpiokey][%s]invalid gpio key_value.\n", __FUNCTION__);
		return;
	}

	printk(KERN_DEBUG "[gpiokey]camera focus key %u action %u\n", KEY_CAMERA_FOCUS, report_action);
	input_report_key(gpio_key->input_dev, KEY_CAMERA_FOCUS, report_action);
	input_sync(gpio_key->input_dev);

	return;
}

static void k3v2_gpio_camera_work(struct work_struct *work)
{
	struct k3v2_gpio_key *gpio_key = container_of(work,
		struct k3v2_gpio_key, gpio_camera_work.work);

	unsigned int keycamera_value = 0;
	unsigned int report_action = GPIO_KEY_RELEASE;

	keycamera_value = gpio_get_value(GPIO_17_7);

	if (g_camera== keycamera_value)
		return;
	else
		g_camera= keycamera_value;

	/*judge key is pressed or released.*/
	if (keycamera_value == GPIO_LOW_VOLTAGE)
		report_action = GPIO_KEY_PRESS;
	else if (keycamera_value == GPIO_HIGH_VOLTAGE)
		report_action = GPIO_KEY_RELEASE;
	else {
		printk(KERN_ERR "[gpiokey][%s]invalid gpio key_value.\n", __FUNCTION__);
		return;
	}

	printk(KERN_DEBUG "[gpiokey]camera key %u action %u\n", KEY_CAMERA, report_action);
	input_report_key(gpio_key->input_dev, KEY_CAMERA, report_action);
	input_sync(gpio_key->input_dev);

	return;
}


static irqreturn_t k3v2_gpio_key_irq_handler(int irq, void *dev_id)
{
	struct k3v2_gpio_key *gpio_key = (struct k3v2_gpio_key *)dev_id;

	switch (irq) {
	case IRQ_GPIO(GPIO_17_1):
		schedule_delayed_work(&(gpio_key->gpio_keyup_work), 0);
		break;

	case IRQ_GPIO(GPIO_17_2):
		schedule_delayed_work(&(gpio_key->gpio_keydown_work), 0);
		break;
	case IRQ_GPIO(GPIO_17_6):
		schedule_delayed_work(&(gpio_key->gpio_camerafocus_work), 0);
		break;
	case IRQ_GPIO(GPIO_17_7):
		schedule_delayed_work(&(gpio_key->gpio_camera_work), 0);
		break;
	default:
		printk(KERN_ERR "[gpiokey][%s]invalid irq %d!\n", __FUNCTION__, irq);
		break;
	}

	return IRQ_HANDLED;
}

static int __devinit k3v2_gpio_key_probe(struct platform_device* pdev)
{
	struct k3v2_gpio_key *gpio_key = NULL;
	struct input_dev *input_dev = NULL;
	int err =0;
        unsigned int camera_focus_key;
        camera_focus_key = get_camera_focus_key();

	if (NULL == pdev) {
		printk(KERN_ERR "[gpiokey]parameter error!\n");
		err = -EINVAL;
		return err;
	}

	gpio_key = kzalloc(sizeof(struct k3v2_gpio_key), GFP_KERNEL);
	if (!gpio_key) {
		dev_err(&pdev->dev, "Failed to allocate struct k3v2_gpio_key!\n");
		err = -ENOMEM;
		return err;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&pdev->dev, "Failed to allocate struct input_dev!\n");
		err = -ENOMEM;
		goto err_alloc_input_device;
	}

	input_dev->name = pdev->name;
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &pdev->dev;
	input_set_drvdata(input_dev, gpio_key);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(KEY_VOLUMEUP, input_dev->keybit);
	set_bit(KEY_VOLUMEDOWN, input_dev->keybit);
	set_bit(KEY_CAMERA_FOCUS, input_dev->keybit);
	set_bit(KEY_CAMERA, input_dev->keybit);
	input_dev->open = k3v2_gpio_key_open;
	input_dev->close = k3v2_gpio_key_close;

	gpio_key->input_dev = input_dev;

	/*initial work before we use it.*/
	INIT_DELAYED_WORK(&(gpio_key->gpio_keyup_work), k3v2_gpio_keyup_work);
	INIT_DELAYED_WORK(&(gpio_key->gpio_keydown_work), k3v2_gpio_keydown_work);
	INIT_DELAYED_WORK(&(gpio_key->gpio_camerafocus_work), k3v2_gpio_camerafocus_work);
	INIT_DELAYED_WORK(&(gpio_key->gpio_camera_work), k3v2_gpio_camera_work);

	/*get volume-up-key irq.*/
	gpio_key->volume_up_irq = platform_get_irq(pdev, 0);
	if (gpio_key->volume_up_irq < 0) {
		dev_err(&pdev->dev, "Failed to get gpio key press irq!\n");
		err = gpio_key->volume_up_irq;
		goto err_get_irq;
	}

	/*
	 * support failing irq that means volume-up-key is pressed,
	 * and rising irq which means volume-up-key is released.
	 */
	err = request_irq(gpio_key->volume_up_irq, k3v2_gpio_key_irq_handler, IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, pdev->name, gpio_key);
	if (err) {
		dev_err(&pdev->dev, "Failed to request press interupt handler!\n");
		goto err_request_irq;
	}

	/*get volume-down-key irq.*/
	gpio_key->volume_down_irq = platform_get_irq(pdev, 1);
	if (gpio_key->volume_down_irq < 0) {
		dev_err(&pdev->dev, "Failed to get gpio key release irq!\n");
		err = gpio_key->volume_down_irq;
		goto err_get_irq;
	}

	/*
	 * support failing irq that means volume-down-key is pressed,
	 * and rising irq which means volume-down-key is released.
	 */
	err = request_irq(gpio_key->volume_down_irq, k3v2_gpio_key_irq_handler, IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, pdev->name, gpio_key);
	if (err) {
		dev_err(&pdev->dev, "Failed to request release interupt handler!\n");
		goto err_request_irq;
	}

        if(camera_focus_key) {
		/*get camera-focus-key irq.*/
		gpio_key->camera_focus_irq = platform_get_irq(pdev, 2);
		if (gpio_key->camera_focus_irq < 0) {
			dev_err(&pdev->dev, "Failed to get gpio camera focus irq!\n");
			err = gpio_key->camera_focus_irq;
			goto err_get_irq;
		}

		/*
		 * support failing irq that means camera-focus-key is pressed,
		 * and rising irq which means camera-focus-key is released.
		 */
		err = request_irq(gpio_key->camera_focus_irq, k3v2_gpio_key_irq_handler, IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, pdev->name, gpio_key);
		if (err) {
			dev_err(&pdev->dev, "Failed to request camera focus interupt handler!\n");
			goto err_request_irq;
		}

		/*get camera-key irq.*/
		gpio_key->camera_irq = platform_get_irq(pdev, 3);
		if (gpio_key->camera_irq < 0) {
			dev_err(&pdev->dev, "Failed to get gpio camera irq!\n");
			err = gpio_key->camera_irq;
			goto err_get_irq;
		}

		/*
		 * support failing irq that means camera-key is pressed,
		 * and rising irq which means camera-key is released.
		 */
		err = request_irq(gpio_key->camera_irq, k3v2_gpio_key_irq_handler, IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, pdev->name, gpio_key);
		if (err) {
			dev_err(&pdev->dev, "Failed to request camera interupt handler!\n");
			goto err_request_irq;
		}
        }

	err = input_register_device(gpio_key->input_dev);
	if (err) {
		dev_err(&pdev->dev, "Failed to register input device!\n");
		goto err_register_device;
	}

	device_init_wakeup(&pdev->dev, TRUE);
	platform_set_drvdata(pdev, gpio_key);

	dev_info(&pdev->dev, "k3v2 gpio key driver probes successfully!\n");

	return 0;

err_register_device:
	free_irq(gpio_key->volume_up_irq, gpio_key);
	free_irq(gpio_key->volume_down_irq, gpio_key);
	free_irq(gpio_key->camera_focus_irq, gpio_key);
	free_irq(gpio_key->camera_irq, gpio_key);
err_request_irq:
err_get_irq:
	input_free_device(input_dev);
err_alloc_input_device:
	kfree(gpio_key);
	gpio_key = NULL;
	pr_info(KERN_ERR "[gpiokey]K3v2 gpio key probe failed! ret = %d.\n", err);
	return err;
}

static int __devexit k3v2_gpio_key_remove(struct platform_device* pdev)
{
	struct k3v2_gpio_key *gpio_key = platform_get_drvdata(pdev);

	if (gpio_key == NULL) {
		printk(KERN_ERR "get invalid gpio_key pointer\n");
		return -EINVAL;
	}

	free_irq(gpio_key->volume_up_irq, gpio_key);
	free_irq(gpio_key->volume_down_irq, gpio_key);
	free_irq(gpio_key->camera_focus_irq, gpio_key);
	free_irq(gpio_key->camera_irq, gpio_key);

	cancel_delayed_work(&(gpio_key->gpio_keyup_work));
	cancel_delayed_work(&(gpio_key->gpio_keydown_work));
	cancel_delayed_work(&(gpio_key->gpio_camerafocus_work));
	cancel_delayed_work(&(gpio_key->gpio_camera_work));

	input_unregister_device(gpio_key->input_dev);
	platform_set_drvdata(pdev, NULL);
	kfree(gpio_key);
	gpio_key = NULL;
	return 0;
}

#ifdef CONFIG_PM
static int k3v2_gpio_key_suspend(struct platform_device *pdev, pm_message_t state)
{
	pr_info("[gpiokey][%s]suspend successfully\n", __FUNCTION__);
	return 0;
}

static int k3v2_gpio_key_resume(struct platform_device *pdev)
{
	pr_info("[gpiokey][%s]resume successfully\n", __FUNCTION__);
	return 0;
}
#endif

struct platform_driver k3v2_gpio_key_driver = {
	.probe = k3v2_gpio_key_probe,
	.remove = __devexit_p(k3v2_gpio_key_remove),
	.driver = {
		.name = "k3v2_gpio_key",
		.owner = THIS_MODULE,
	},
#ifdef CONFIG_PM
	.suspend = k3v2_gpio_key_suspend,
	.resume = k3v2_gpio_key_resume,
#endif
};

static int __init k3v2_gpio_key_init(void)
{
	pr_info(KERN_INFO "[gpiokey]k3v2 gpio key init!\n");

	return platform_driver_register(&k3v2_gpio_key_driver);
}

static void __exit k3v2_gpio_key_exit(void)
{
	pr_info(KERN_INFO "[gpiokey]k3v2 gpio key exit!\n");
	platform_driver_unregister(&k3v2_gpio_key_driver);
}

module_init(k3v2_gpio_key_init);
module_exit(k3v2_gpio_key_exit);
MODULE_AUTHOR("Hisilicon K3 Driver Group");
MODULE_DESCRIPTION("K3v2 keypad platform driver");
MODULE_LICENSE("GPL");
