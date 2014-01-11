#ifndef _EXTRAL_DYNAMIC_DCDC_H
#define _EXTRAL_DYNAMIC_DCDC_H
#include <linux/i2c.h>

#include <linux/regulator/machine.h>

#include <mach/board-hi6421-regulator.h>

#define EXTRAL_DYNAMIC_DCDC_NAME        "extral_dynamic_dcdc"
#define EXTRAL_DYNAMIC_DCDC_I2C_ADDR    0x60

struct extral_dynamic_dcdc_platform_data{
	int enable_gpio;
	char io_block_name[20];
    struct regulator_init_data* regulator_data;
};


#endif

