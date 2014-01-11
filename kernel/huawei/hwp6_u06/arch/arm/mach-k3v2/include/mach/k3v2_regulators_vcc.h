/*
 * arch/arm/mach-k3v2/k3v2_regulators_vcc.h
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#ifndef __K3V2_REGULATORS_VCC_H_
#define __K3V2_REGULATORS_VCC_H_
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#define EN_REG 0xC0
#define DIS_REG 0xC4
#define EN_STAT_REG 0xC8
#define PWR_EN_REG 0xD0
#define PWR_DIS_REG 0xD4
#define PWR_STAT_REG 0xD8
#define PWR_ACK_REG 0xDC
#define SCCPURSTEN_REG 0x410
#define SCCPUCLKOFF_REG 0x404
#define SCCPURSTDIS_REG 0x414
/*soft reset disable register of edc1,isp,venc,vdec,bt*/
#define SCPERRSTDIS1_REG 0x090
/*soft reset enable register of edc1,isp,venc,vdec,bt*/
#define SCPERRSTEN1_REG 0x08c
#define SCPERRSTSTAT1_REG 0x094
#define SCCPURSTSTAT_REG 0x418
#define SCPERCTRL0_WFI_REG 0x200
#define SCCPUCOREEN_REG 0x0F4
#define SCCPUCOREDIS_REG 0x0F8
#define G3D_CLK_EN_REG 0x030
#define G3D_CLK_DISEN_REG 0x034

#define VCC_EDC1 0
#define VCC_ISP 1
#define VCC_VENC 2
#define VCC_VDEC 3
#define VCC_BT 4
#define VCC_CPU2 5
#define VCC_CPU3 6
#define VCC_G3D 7
#define G3D_CORE_RST_BIT 8
#define G3D_INTF_RST_BIT 9
#define LDI1_CLOCK 10
#define PLL2_CLOCK 11
/*edc1's rate, 18750000 64 division of pll2 1200M*/
#define EDC_RST_RATE_FROM_1080M (1080000000 >> 6) /*edc1's rate, 64 division of pll2 1200M*/
#define EDC_RST_RATE_FROM_1400M (1400000000 >> 6) /*edc1's rate, 64 division of pll2 1050M*/
struct vcc_resource {
		void __iomem	*base;
		struct resource	*res;
		struct regulator_dev	*rdev[8];
		unsigned int irq;
};

extern struct regulator_ops vcc_med_ops;
extern struct regulator_ops vcc_cpu23_ops;
extern struct regulator_ops vcc_edc1_ops;
extern struct regulator_ops vcc_g3d_ops;
/*extern struct regulator_ops vcc_ops;*/
extern struct regulator_desc vcc_regulator_desc[];
extern struct platform_device vcc_regulator_device;
#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
extern struct platform_device vcc_regulator_dcdc_gpu_device;
#endif
#endif
