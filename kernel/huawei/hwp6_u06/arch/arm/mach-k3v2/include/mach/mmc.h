#ifndef ASMARM_MACH_MMC_H
#define ASMARM_MACH_MMC_H

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mshci.h>

#define TUNING_INIT_CONFIG_NUM 6

struct hisik3_mmc_platform_data {
	/* available voltages */
	unsigned int ocr_mask;
	/* available capabilities */
	unsigned long caps;
	/* clock name */
	char *clk_name;
	/* deviations from spec. */
	unsigned int quirks;
	/* external detect card gpio */
	unsigned int cd_gpio;
	/* iomux name */
	char *iomux_name;
	/* regulator name */
	char *reg_name;	
	/* signal voltage regulator name */
	char *signal_reg_name;
	int (*ext_cd_init)(void (*notify_func)(struct platform_device *,
		int state));
	int (*ext_cd_cleanup)(void (*notify_func)(struct platform_device *,
		int state));
	int (*set_power)(struct platform_device *, int val);
	int *timing_config;
	int *init_tuning_config;
	int suspend_timing_config;
	int allow_switch_signal_voltage;
	int default_signal_voltage;
};

#endif
