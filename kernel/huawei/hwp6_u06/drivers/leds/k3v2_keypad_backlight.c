/*
 * Filename:kernel/drivers/leds/k3v2_keypad_backlight.c
 *
 * Discription:realize keypad backlight.
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

#include <asm/io.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <asm-generic/errno-base.h>
#include <mach/k3v2_keypad_backlight.h>

#define TRUE	(1)
#define FALSE	(0)

struct k3v2_led {
	struct led_classdev *cdev;
	bool                 bOpenBacklight;
	unsigned char        brightness;
};

/*
 * Function name:k3v2_backlight_ctrl.
 * Discription: complement backlight control by means of writing given value
 * to DR1 control register.
 */
int k3v2_backlight_ctrl(struct k3v2_led *pLed)
{
	if (pLed->bOpenBacklight) {
		writel((DR1_ENABLE | BRIGHTNESS_10), DR1_CTRL);
		writel(BRIGHTNESS_ON_LEVEL7 << BRIGHTNESS_ON_BIT, DR1_TIM_CONT1);
	} else {
		writel(DR1_DISABLE, DR1_CTRL);
		writel(BRIGHTNESS_OFF_LEVEL7, DR1_TIM_CONT1);
	}

	return 0;
}

int k3v2_open_backlight(struct k3v2_led *pLed, unsigned char brightness)
{
	pLed->bOpenBacklight = TRUE;
	pLed->brightness = brightness;

	return k3v2_backlight_ctrl(pLed);
}

int k3v2_close_backlight(struct k3v2_led *pLed)
{
	pLed->bOpenBacklight = FALSE;
	pLed->brightness = BACKLIGHT_MIN_BRIGHTNESS;

	return k3v2_backlight_ctrl(pLed);
}

static DEFINE_MUTEX(k3keyboard_backlight_lock);

/*
 * Function name:k3v2_set_backlight_level.
 * Discription: set keypad backlight brightness according to level value.
 * level value may be 0,50,or 100, corresponding to backlight-off, half-brightness,
 * and maximum brightness by meas of writing 0x0,0x3,and 0x7 to DR1 control register.
 */
static void k3v2_set_backlight_level(struct k3v2_led *pLed, uint8_t level)
{
	if (0 == level)
		k3v2_close_backlight(pLed);
	else
		k3v2_open_backlight(pLed, level);
}

/*
 * Function name:k3v2_keypad_brightness_set.
 * Discription:set keypad brightness, andriod system appliciton will call this
 * function when detectiong keypad is pressed, and this function will call
 * k3v2_set_backlight_level to realize really backlight-setting.
 */
static void k3v2_keypad_brightness_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	struct k3v2_led *pLed     = container_of(led_cdev, struct k3v2_led, cdev);
	int              newValue = 0;
	if (!pLed) {
		pr_err("Error! k3v2_led is NULL!\n");
		return;
	}

	pr_debug("k3v2 keypad brightness set: value = %d\n", value);

	/* the app's max brightness is 255, caculate the percent for use.
	 * newValue is the percent of brightness,maybe one of them:0,50,or 100.
	 */
	newValue = (value * 100) / 255;
	newValue &= 0xFF;

	mutex_lock(&k3keyboard_backlight_lock);
	k3v2_set_backlight_level(pLed, newValue);
	mutex_unlock(&k3keyboard_backlight_lock);
}

static int k3v2_keypad_backlight_probe(struct platform_device *pdev)
{
	struct k3v2_led     *k3v2_led;
	struct led_classdev *cled;
	int                  ret;

	k3v2_led = kzalloc(sizeof(struct k3v2_led), GFP_KERNEL);

	if(k3v2_led == NULL){
		dev_err(&pdev->dev,"failed to allocate k3v2_led device!\n");
		return -EINVAL;
	}

	cled = kzalloc(sizeof(struct led_classdev), GFP_KERNEL);

	if(cled == NULL){
		dev_err(&pdev->dev,"failed to allocate cled device!\n");
		kfree(k3v2_led);
		return -EINVAL;
	}


	cled->name = pdev->name;
	cled->brightness = APP_MAX_BRIGHTNESS;
	cled->brightness_set = k3v2_keypad_brightness_set;
	k3v2_led->cdev = cled;
	k3v2_led->bOpenBacklight = FALSE;
	k3v2_led->brightness = BACKLIGHT_MIN_BRIGHTNESS;
	platform_set_drvdata(pdev, k3v2_led);

	ret = led_classdev_register(&pdev->dev, k3v2_led->cdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "Error! Failed to register led_classdev!\n");
		kfree(cled);
		kfree(k3v2_led);
		return ret;
	}

	dev_dbg(&pdev->dev, "k3v2 keypad backlight driver probes successfully!\n");
	return 0;
}

static int k3v2_keypad_backlight_remove(struct platform_device *pdev)
{
	struct k3v2_led *led = platform_get_drvdata(pdev);

	if(led == NULL){
		dev_dbg(&pdev->dev, "get invalid led pointer\n");
		return -EINVAL;
	}

	led_classdev_unregister(led->cdev);
	kfree(led);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int k3v2_keypad_backlight_suspend(struct platform_device *pdev)
{
	struct k3v2_led *led = platform_get_drvdata(pdev);

	if(led == NULL){
		dev_dbg(&pdev->dev, "get invalid led pointer\n");
		return -EINVAL;
	}

	mutex_lock(&k3keyboard_backlight_lock);
	k3v2_close_backlight(led);
	mutex_unlock(&k3keyboard_backlight_lock);
	return 0;
}

static int k3v2_keypad_backlight_resume(struct platform_device *pdev)
{
	//struct k3v2_led *led = platform_get_drvdata(pdev);
	//k3v2_open_backlight(led, BACKLIGHT_MAX_BRIGHTNESS);

	return 0;
}

#endif

static struct platform_driver k3v2_keyboard_backlight_driver = {
	.probe          = k3v2_keypad_backlight_probe,
	.remove         = __devexit_p(k3v2_keypad_backlight_remove),
	.driver         = {
		.name           = "keyboard-backlight",
		.owner          = THIS_MODULE,
	},
#ifdef CONFIG_PM
	.suspend = k3v2_keypad_backlight_suspend,
	.resume = k3v2_keypad_backlight_resume,
#endif
};


static int __init k3v2_keypad_backlight_init(void)
{
	pr_info("k3v2 keypad backlight init!\n");
	return platform_driver_register(&k3v2_keyboard_backlight_driver);
}

static void __exit k3v2_keypad_backlight_exit(void)
{
	platform_driver_unregister(&k3v2_keyboard_backlight_driver);
}
module_init(k3v2_keypad_backlight_init);
module_exit(k3v2_keypad_backlight_exit);
MODULE_AUTHOR("Hisilicon k3 driver group");
MODULE_DESCRIPTION("k3v2 Led backlight driver");
MODULE_LICENSE("GPL");
