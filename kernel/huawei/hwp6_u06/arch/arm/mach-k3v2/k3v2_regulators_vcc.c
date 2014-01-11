/*
 * arch/arm/mach-k3v2/k3v2_regulators_vcc.c
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#include <mach/system.h>
#include <mach/platform.h>
#include <linux/platform_device.h>
#include <mach/k3v2_regulators_vcc.h>

static struct regulator_consumer_supply vcc_g3d_consumers[] = {
	REGULATOR_SUPPLY("vcc_g3d", NULL),
};
static struct regulator_consumer_supply vcc_cpu3_consumers[] = {
	REGULATOR_SUPPLY("vcc_cpu3", NULL),
};
static struct regulator_consumer_supply vcc_cpu2_consumers[] = {
	REGULATOR_SUPPLY("vcc_cpu2", NULL),
};
static struct regulator_consumer_supply vcc_bt_consumers[] = {
	REGULATOR_SUPPLY("vcc_bt", "bt"),
};
static struct regulator_consumer_supply vcc_vdec_consumers[] = {
	REGULATOR_SUPPLY("vcc_vdec", NULL),
};
static struct regulator_consumer_supply vcc_venc_consumers[] = {
	REGULATOR_SUPPLY("vcc_venc", NULL),
};
static struct regulator_consumer_supply vcc_isp_consumers[] = {
	REGULATOR_SUPPLY("vcc_isp", "camera"),
};
static struct regulator_consumer_supply vcc_edc1_consumers[] = {
	REGULATOR_SUPPLY("vcc_edc1", NULL),
};

static struct regulator_init_data k3v2_vcc_regulators[] = {
		[VCC_EDC1] = {
			.constraints = {
				.name = "VCC_EDC1",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_edc1_consumers),
			.consumer_supplies = vcc_edc1_consumers,
		},
		[VCC_ISP] = {
			.constraints = {
				.name = "VCC_ISP",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_isp_consumers),
			.consumer_supplies = vcc_isp_consumers,
		},
		[VCC_VENC] = {
			.constraints = {
				.name = "VCC_VENC",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_venc_consumers),
			.consumer_supplies = vcc_venc_consumers,
		},
		[VCC_VDEC] = {
			.constraints = {
				.name = "VCC_VDEC",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_vdec_consumers),
			.consumer_supplies = vcc_vdec_consumers,
		},
		[VCC_BT] = {
			.constraints = {
				.name = "VCC_BT",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_bt_consumers),
			.consumer_supplies = vcc_bt_consumers,
		},
		[VCC_CPU2] = {
			.constraints = {
				.name = "VCC_CPU2",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_cpu2_consumers),
			.consumer_supplies = vcc_cpu2_consumers,
		},
		[VCC_CPU3] = {
			.constraints = {
				.name = "VCC_CPU3",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_cpu3_consumers),
			.consumer_supplies = vcc_cpu3_consumers,
		},
		[VCC_G3D] = {
			.constraints = {
				.name = "VCC_G3D",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_g3d_consumers),
			.consumer_supplies = vcc_g3d_consumers,
			.supply_regulator = "BUCK2",
		},
};

#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
static struct regulator_init_data k3v2_vcc_dcdc_gpu_regulators[] = {
		[VCC_EDC1] = {
			.constraints = {
				.name = "VCC_EDC1",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_edc1_consumers),
			.consumer_supplies = vcc_edc1_consumers,
		},
		[VCC_ISP] = {
			.constraints = {
				.name = "VCC_ISP",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_isp_consumers),
			.consumer_supplies = vcc_isp_consumers,
		},
		[VCC_VENC] = {
			.constraints = {
				.name = "VCC_VENC",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_venc_consumers),
			.consumer_supplies = vcc_venc_consumers,
		},
		[VCC_VDEC] = {
			.constraints = {
				.name = "VCC_VDEC",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_vdec_consumers),
			.consumer_supplies = vcc_vdec_consumers,
		},
		[VCC_BT] = {
			.constraints = {
				.name = "VCC_BT",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_bt_consumers),
			.consumer_supplies = vcc_bt_consumers,
		},
		[VCC_CPU2] = {
			.constraints = {
				.name = "VCC_CPU2",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_cpu2_consumers),
			.consumer_supplies = vcc_cpu2_consumers,
		},
		[VCC_CPU3] = {
			.constraints = {
				.name = "VCC_CPU3",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_cpu3_consumers),
			.consumer_supplies = vcc_cpu3_consumers,
		},
		[VCC_G3D] = {
			.constraints = {
				.name = "VCC_G3D",
				.valid_ops_mask = REGULATOR_CHANGE_STATUS,
				.always_on = 0,
			},
			.num_consumer_supplies = ARRAY_SIZE(vcc_g3d_consumers),
			.consumer_supplies = vcc_g3d_consumers,
			.supply_regulator = "VCC_EXTRAL_DYNAMIC_DCDC",
		},
};
#endif
struct regulator_desc vcc_regulator_desc[] = {
	[VCC_EDC1] = {
		.name = "VCC_EDC1",
		.id = VCC_EDC1,
		.ops = &vcc_edc1_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[VCC_ISP] = {
		.name = "VCC_ISP",
		.id = VCC_ISP,
		.ops = &vcc_med_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[VCC_VENC] = {
		.name = "VCC_VENC",
		.id = VCC_VENC,
		.ops = &vcc_med_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[VCC_VDEC] = {
		.name = "VCC_VDEC",
		.id = VCC_VDEC,
		.ops = &vcc_med_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[VCC_BT] = {
		.name = "VCC_BT",
		.id = VCC_BT,
		.ops = &vcc_med_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[VCC_CPU2] = {
		.name = "VCC_CPU2",
		.id = VCC_CPU2,
		.ops = &vcc_cpu23_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[VCC_CPU3] = {
		.name = "VCC_CPU3",
		.id = VCC_CPU3,
		.ops = &vcc_cpu23_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	[VCC_G3D] = {
		.name = "VCC_G3D",
		.id = VCC_G3D,
		.ops = &vcc_g3d_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
};

static struct resource vcc_regulator_resources[] = {
	{
		.start = REG_BASE_SCTRL,
		.end = REG_BASE_SCTRL + REG_SCTRL_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device vcc_regulator_device = {
	.name =  "vcc-regulator",
	.id	= 0,
	.dev  = {
		.platform_data	= k3v2_vcc_regulators,
	},
	.num_resources = ARRAY_SIZE(vcc_regulator_resources),
	.resource = vcc_regulator_resources,
};

#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
struct platform_device vcc_regulator_dcdc_gpu_device = {
	.name =  "vcc-regulator",
	.id	= 0,
	.dev  = {
		.platform_data	= k3v2_vcc_dcdc_gpu_regulators,
	},
	.num_resources = ARRAY_SIZE(vcc_regulator_resources),
	.resource = vcc_regulator_resources,
};
#endif  
