/*
 * smarter gpio handling functions header
 *
 * Copyright (C) 2012 Huawei Technology, Limited.
 * Author: Wei Du
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
#ifndef __DRIVERS_SWITCH_SMART_GPIO_H__
#define __DRIVERS_SWITCH_SMART_GPIO_H__

#define UNAVAILABLE_PIN						-1
#define SMART_GPIO_NO_TOUCH					0x10

static inline void smart_gpio_request(int gpio_num, char *gpio_name, int *ret)
{
	int local_ret;

	if ( (*ret != 0)
	   || (gpio_num == UNAVAILABLE_PIN)
	   )
	{
		// previous error or unavailable gpio
		return;
	}

	local_ret = gpio_request(gpio_num, gpio_name);
	if (local_ret < 0) {
		pr_err("%s: gpio_request %d return"
			"err: %d.\n", __func__, gpio_num, local_ret);
		*ret = local_ret;
	}
}

static inline void smart_gpio_direction_output(int gpio_num, unsigned gpio_value, int *ret)
{
	int local_ret;

	if ( (*ret != 0)
	   || (gpio_num == UNAVAILABLE_PIN)
	   )
	{
		// previous error or unavailable gpio
		return;
	}

	local_ret = gpio_direction_output(gpio_num, gpio_value);
	if (local_ret < 0) {
		pr_err("%s: gpio_direction_output %d return"
			"err: %d.\n", __func__, gpio_num, local_ret);
		*ret = local_ret;
	}
}

static inline void smart_gpio_set_value(int gpio_num, unsigned gpio_value)
{
	if ((gpio_num == UNAVAILABLE_PIN) || (gpio_value == SMART_GPIO_NO_TOUCH))
	{
		return;
	}

	gpio_set_value(gpio_num, gpio_value);
}

static inline int smart_gpio_get_value(int gpio_num)
{
	if (gpio_num == UNAVAILABLE_PIN)
	{
		return SMART_GPIO_NO_TOUCH;
	}

	return gpio_get_value(gpio_num);
}

static inline void smart_gpio_free(int gpio_num)
{
	if (gpio_num == UNAVAILABLE_PIN)
	{
		// unavailable gpio
		return;
	}

	gpio_free(gpio_num);
}

#endif /* __DRIVERS_SWITCH_SMART_GPIO_H__ */
