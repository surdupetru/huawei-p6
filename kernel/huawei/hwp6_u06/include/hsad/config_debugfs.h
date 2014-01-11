
#ifndef _CONFIG_DEBUGFS_H
#define _CONFIG_DEBUGFS_H
#include <mach/boardid.h>

#ifdef CONFIG_HUAWEI_GPIO_UNITE
struct gpio_config_str
{
	char* func;
	char* drv;
	char* pull;
	char* dir;
};

struct pm_gpio_str
{
	char* direction;
	char* output_buffer;
	char* output_value;
	char* pull;
	char* vin_sel;
	char* out_strength;
	char* function;
	char* inv_int_pol;
	char* disable_pin;
};
#endif

extern int config_debugfs_init(void);
#endif
