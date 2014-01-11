
#ifndef _CONFIG_GENERIC_STRUCT_H
#define _CONFIG_GENERIC_STRUCT_H

#include <hsad/configdata.h>

#ifdef CONFIG_HUAWEI_GPIO_UNITE
#include <mach/gpiomux.h>
#include <linux/hwgpio.h>
#include <linux/regulator/hw-uniregulator.h>

//module name list
#define GPIO_MODULE_NAME  "gpio"
#define PM_GPIO_MODULE_NAME  "pm gpio"
#endif
#define COMMON_MODULE_NAME  "common"
#define POWER_MODULE_NAME "power"

struct board_id_general_struct
{
	char	name[32];
	int		board_id;
	union{
	#ifdef CONFIG_HUAWEI_GPIO_UNITE
		struct gpio_config_type *gpio_ptr;
		struct pm_gpio_cfg_t *pm_gpio_ptr;
	#endif
		//struct hw_config_power_tree* power_tree_ptr;
		config_pair	*config_pair_ptr;
	}data_array;
	struct list_head list;
};
#endif
