/*
 *arch/arm/mach-k3c2/regulator_vcc.c
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <mach/platform.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <mach/k3v2_regulators_vcc.h>
#include <mach/boardid.h>
#include <linux/ipps.h>

static DEFINE_MUTEX(reg_lock_mutex);
#define lock(x) mutex_lock(x)
#define unlock(x)  mutex_unlock(x)

unsigned int ip_reset_value[] = {
	[VCC_EDC1] = 0x01 << 10,/*edc1 soft reset, bit 10*/
	[VCC_ISP] = 0x01 << 12,/*isp soft reset bit 12*/
	[VCC_VENC] = 0x01 << 4,/*venc soft reset bit 4*/
	[VCC_VDEC] = 0x01 << 5,/*vdec soft reset bit 5*/
	[VCC_BT] = 0x01 << 18,/*bt soft reset bit 18*/
	/*cpu2 core rst bit 2,and debug rst bit 14*/
	[VCC_CPU2] = 0x01 << 2 | 0x01 << 14,
	/*cpu3 core rst bit 3,and debug rst bit 15*/
	[VCC_CPU3] = 0x01 << 3 | 0x01 << 15,
	[VCC_G3D] = (0x01 << 1) | (0x01) | (0x01 << 13) | (0x01 << 14),/*g3d soft rest bit 1 0 13 14*/
	[G3D_CORE_RST_BIT] = (0x01) | (0x01 << 13) | (0x01 << 14),/*g3d soft rest bit 0 13 14*/
	[G3D_INTF_RST_BIT] = 0x01 << 1,/*g3d bus rest bit 1*/
};

/*clocks'name of media sub system*/
char *clock_name[] = {
	[VCC_EDC1] = "clk_edc1",
	[VCC_ISP] = "clk_isp",
	[VCC_VENC] = "clk_venc",
	[VCC_VDEC] = "clk_vdec",
	[VCC_BT] = "clk_bt",
	[VCC_G3D] = "clk_g3d",
};

/*clocks of media sub system*/
struct clk *med_clock[] = {
	[VCC_EDC1] = NULL,
	[VCC_ISP] = NULL,
	[VCC_VENC] = NULL,
	[VCC_VDEC] = NULL,
	[VCC_BT] = NULL,
	[VCC_G3D] = NULL,
	[LDI1_CLOCK] = NULL,
	[PLL2_CLOCK] = NULL,
};

static void ippsclient_add(struct ipps_device *device)
{

}

static void ippsclient_remove(struct ipps_device *device)
{

}

static struct ipps_client vcc_ipps_client = {
	.name   = "vcc-regulator",
	.add    = ippsclient_add,
	.remove = ippsclient_remove
};

static void write_med_enable_softrst_register(void __iomem *base_addr,
		int regulator_id)
{
	writel(ip_reset_value[regulator_id], base_addr + SCPERRSTEN1_REG);
}

static void write_cpu23_enable_softrst_register(void __iomem *base_addr,
		int regulator_id)
{
	writel(ip_reset_value[regulator_id], base_addr + SCCPURSTEN_REG);
}

static void write_med_disable_softrst_register(void __iomem *base_addr,
		int regulator_id)
{
	writel(ip_reset_value[regulator_id], base_addr + SCPERRSTDIS1_REG);
}

static void write_cpu23_disable_softrst_register(void __iomem *base_addr,
		int regulator_id)
{
	writel(ip_reset_value[regulator_id], base_addr + SCCPURSTDIS_REG);
}

static void open_mtcmos(void __iomem *base_addr, int regulator_id)
{
	int bit_mask;
	bit_mask = 0x01 << regulator_id;
	writel(bit_mask, base_addr + PWR_EN_REG);
	udelay(100);
}

static void close_mtcmos(void __iomem *base_addr, int regulator_id)
{
	int bit_mask;
	bit_mask = 0x01 << regulator_id;
	writel(bit_mask, base_addr + PWR_DIS_REG);
	udelay(100);
}

static void disable_softrst(void __iomem *base_addr, int regulator_id)
{
	if ((regulator_id == VCC_CPU3) || (regulator_id == VCC_CPU2))
		write_cpu23_disable_softrst_register(base_addr, regulator_id);
	else
		write_med_disable_softrst_register(base_addr, regulator_id);
}

/* enable and disable ldo of edc1 including rst operation is special*/
static void edc1_enable_softrst(void __iomem *base_addr, int regulator_id)
{
	int ret = 0;
	long rate = 0;

	/*EDC and ldi1 clock enable*/
	if (IS_ERR(med_clock[regulator_id]) || IS_ERR(med_clock[LDI1_CLOCK])\
		|| IS_ERR(med_clock[PLL2_CLOCK]))
		WARN(1, "Regulator vcc:clock get failed\r\n");
	ret = clk_enable(med_clock[regulator_id]);
	if (ret)
		WARN(1, "Regulator vcc:failed to enable edc clock\r\n");
	ret = clk_enable(med_clock[LDI1_CLOCK]);
	if (ret)
		WARN(1, "Regulator vcc:failed to enable ldi1 clock\r\n");

	/*EDC reset*/
	write_med_enable_softrst_register(base_addr, regulator_id);
	/*config EDC core clock to 64 division*/
	rate = clk_get_rate(med_clock[PLL2_CLOCK]);
	if (rate == 0) {
		WARN(1, "Regulator vcc:PLL2 clock's rate get failed.\r\n");
	}

	ret = clk_set_rate(med_clock[regulator_id], rate >> 6);
	if (ret)
		WARN(1, "Regulator vcc:failed to set rate for edc\r\n");

	/*EDC clock disable*/
	clk_disable(med_clock[regulator_id]);
	clk_disable(med_clock[LDI1_CLOCK]);
}

static void g3d_enable_softrst(void __iomem *base_addr, int regulator_id)
{
	/*enable rst including core and intf for 32 cycle*/
	write_med_enable_softrst_register(base_addr, regulator_id);
	/*delay circle*/
	udelay(5);/*more than 50 cycles of 30M HZ clock*/
}

static void enable_softrst(void __iomem *base_addr, int regulator_id)
{
	switch (regulator_id) {
	case VCC_EDC1:
		edc1_enable_softrst(base_addr, regulator_id);
		break;
	case VCC_ISP:
	case VCC_VENC:
	case VCC_VDEC:
	case VCC_BT:
		write_med_enable_softrst_register(base_addr, regulator_id);
		break;
	case VCC_CPU3:
	case VCC_CPU2:
		write_cpu23_enable_softrst_register(base_addr, regulator_id);
		break;
	case VCC_G3D:
		g3d_enable_softrst(base_addr, regulator_id);
		break;
	default:
		WARN(1, "Regulator vcc:regulator id error\r\n");
	}
}

static void enable_iso(void __iomem *base_addr, int regulator_id)
{
	writel(0x01 << regulator_id, base_addr + EN_REG);
	udelay(5);
}

static void disable_iso(void __iomem *base_addr, int regulator_id)
{
	writel(0x01 << regulator_id, base_addr + DIS_REG);
	udelay(5);
}

/*set wfi mask*/
static void set_wfi(void __iomem *base_addr, int regulator_id)
{
	int val = 0;
	int bit_mask = 0;

	if (regulator_id == VCC_CPU2)
		bit_mask = 0x01 << 30;
	else if (regulator_id == VCC_CPU3)
		bit_mask = 0x01 << 31;

	val = readl(base_addr + SCPERCTRL0_WFI_REG);
	val |= bit_mask;
	writel(val, base_addr + SCPERCTRL0_WFI_REG);
}

/*clear wfi*/
static void clear_wfi(void __iomem *base_addr, int regulator_id)
{
	int val = 0;
	int bit_mask = 0;

	if (regulator_id == VCC_CPU2)
		bit_mask = 0x01 << 30;
	else if (regulator_id == VCC_CPU3)
		bit_mask = 0x01 << 31;

	val = readl(base_addr + SCPERCTRL0_WFI_REG);
	val &= ~bit_mask;
	writel(val, base_addr + SCPERCTRL0_WFI_REG);
}

/*enable cpu core*/
static void enable_cpu23_core(void __iomem *base_addr, int regulator_id)
{
	int bit_mask = 0;

	if (regulator_id == VCC_CPU2)
		bit_mask = 0x01 << 2;
	else if (regulator_id == VCC_CPU3)
		bit_mask = 0x01 << 3;

	writel(bit_mask, base_addr + SCCPUCOREEN_REG);
}

/*disable cpu core*/
static void disable_cpu23_core(void __iomem *base_addr, int regulator_id)
{
	int bit_mask = 0;

	if (regulator_id == VCC_CPU2)
		bit_mask = 0x01 << 2;
	else if (regulator_id == VCC_CPU3)
		bit_mask = 0x01 << 3;

	writel(bit_mask, base_addr + SCCPUCOREDIS_REG);
}

/*cpu2 & cpu3 vcc*/
static int vcc_cpu23_is_enabled(struct regulator_dev *dev)
{
	u32 val = 0;
	u32 bit_mask = 0;
	int chip_id = 0;
	int  regulator_id;
	void __iomem *base_addr = NULL;
	struct vcc_resource *vcc_resource_data = NULL;

	regulator_id =  rdev_get_id(dev);
	vcc_resource_data = rdev_get_drvdata(dev);
	base_addr = vcc_resource_data->base;
	bit_mask = 0x01 << regulator_id;

	lock(&reg_lock_mutex);
	chip_id = get_chipid();
	if (chip_id == CS_CHIP_ID) {
		val = readl(base_addr + PWR_ACK_REG) & (bit_mask << 1);/*the final status*/
		val = val >> (regulator_id + 1);
	} else if (chip_id == DI_CHIP_ID) {
		val = readl(base_addr + PWR_ACK_REG) & bit_mask;/*the final status*/
		val = val >> regulator_id;
	}
	unlock(&reg_lock_mutex);

	return val;
}
static int vcc_cpu23_enable(struct regulator_dev *dev)
{
	int  regulator_id;
	void __iomem *base_addr = NULL;
	struct vcc_resource *vcc_resource_data;

	regulator_id =  rdev_get_id(dev);
	vcc_resource_data = rdev_get_drvdata(dev);
	if (!vcc_resource_data)
		return -1;
	base_addr = vcc_resource_data->base;

	lock(&reg_lock_mutex);
	/*step 1:mtcmos*/
	open_mtcmos(base_addr, regulator_id);
	/*step 2:enable cpu core*/
	enable_cpu23_core(base_addr, regulator_id);
	/*step 3:clear rst then enable again*/
	disable_softrst(base_addr, regulator_id);
	enable_softrst(base_addr, regulator_id);
	/*step 4:iso disable*/
	disable_iso(base_addr, regulator_id);
	/*step 5:clear wfi*/
	clear_wfi(base_addr, regulator_id);
	/*step 6:disable rst*/
	disable_softrst(base_addr, regulator_id);
	unlock(&reg_lock_mutex);
	return 0;
}

static int vcc_cpu23_disable(struct regulator_dev *dev)
{
	int  regulator_id;
	void __iomem *base_addr = NULL;
	struct vcc_resource *vcc_resource_data;

	regulator_id =  rdev_get_id(dev);
	vcc_resource_data = rdev_get_drvdata(dev);
	if (!vcc_resource_data)
		return -1;
	base_addr = vcc_resource_data->base;

	lock(&reg_lock_mutex);
	/*step 1:set wfi mask*/
	set_wfi(base_addr, regulator_id);
	/*step 2:disable cpu core before rst*/
	disable_cpu23_core(base_addr, regulator_id);
	/*step 3:enable iso*/
	enable_iso(base_addr, regulator_id);
	/*step 4:soft reset*/
	enable_softrst(base_addr, regulator_id);
	/*step 5:close mtcmos*/
	close_mtcmos(base_addr, regulator_id);
	unlock(&reg_lock_mutex);
	return 0;
}

/*sub system vcc of medias*/
static int vcc_med_is_enabled(struct regulator_dev *dev)
{
	long val = 0;
	unsigned long bit_mask;
	int  regulator_id;
	void __iomem *base_addr = NULL;
	struct vcc_resource *vcc_resource_data;

	regulator_id =  rdev_get_id(dev);
	vcc_resource_data = rdev_get_drvdata(dev);
	base_addr = vcc_resource_data->base;
	bit_mask = 1 << regulator_id;

	lock(&reg_lock_mutex);
	if (regulator_id != VCC_EDC1) {
		val = readl(base_addr + PWR_ACK_REG) & bit_mask;/*the final status*/
		val = val >> regulator_id;
	}
	unlock(&reg_lock_mutex);
	return val;
}

static int vcc_med_enabled(struct regulator_dev *dev)
{
	int ret = 0;
	u32 bit_mask;
	int  regulator_id;
	void __iomem *base_addr = NULL;
	struct vcc_resource *vcc_resource_data;

	regulator_id =  rdev_get_id(dev);
	vcc_resource_data = rdev_get_drvdata(dev);
	base_addr = vcc_resource_data->base;
	bit_mask = 0x01 << regulator_id;

	lock(&reg_lock_mutex);
	/*step1:mtcmos*/
	/*g3d don't need to set mtcmos*/
	open_mtcmos(base_addr, regulator_id);
	/*step2: soft reset enable*/
	enable_softrst(base_addr, regulator_id);
	/*step 3:enable clock*/
	if (IS_ERR(med_clock[regulator_id])) {
		pr_err("Regulator vcc:clock get failed\r\n");
		goto err;
	}
	ret = clk_enable(med_clock[regulator_id]);
	if (ret) {
		pr_err("Regulator vcc:clock enable failed\r\n");
		goto err;
	}
	udelay(5);
	/*step 4:iso disable*/
	disable_iso(base_addr, regulator_id);
	/*step 5:reset disable*/
	disable_softrst(base_addr, regulator_id);
	clk_disable(med_clock[regulator_id]);
	unlock(&reg_lock_mutex);
	return 0;
err:
	unlock(&reg_lock_mutex);
	return -1;
}

static int vcc_med_disabled(struct regulator_dev *dev)
{
	int  regulator_id;
	int ret;
	void __iomem *base_addr = NULL;
	struct vcc_resource *vcc_resource_data;

	regulator_id =  rdev_get_id(dev);
	vcc_resource_data = rdev_get_drvdata(dev);
	base_addr = vcc_resource_data->base;

	lock(&reg_lock_mutex);
	if (IS_ERR(med_clock[regulator_id])) {
		pr_err("Regulator vcc:clock get failed\r\n");
		goto err;
	}
	ret = clk_enable(med_clock[regulator_id]);
	if (ret) {
		pr_err("Regulator vcc:clock enable failed\r\n");
		goto err;
	}
	/*step 1:soft reset enable*/
	enable_softrst(base_addr, regulator_id);
	udelay(5);
	/*step 2:close ip clock*/
	clk_disable(med_clock[regulator_id]);
	udelay(1);
	/*step 3:iso enable*/
	enable_iso(base_addr, regulator_id);
	/*step 4:close mtcmos*/
	close_mtcmos(base_addr, regulator_id);

	unlock(&reg_lock_mutex);
	return 0;
err:
	unlock(&reg_lock_mutex);
	return -1;
}

static int vcc_g3d_is_enabled(struct regulator_dev *dev)
{
	return 0;
}
static int vcc_g3d_enabled(struct regulator_dev *dev)
{
	int ret = 0;
	u32 bit_mask;
	int  regulator_id;
	void __iomem *base_addr = NULL;
	struct vcc_resource *vcc_resource_data;
	u32 uippsmode = 0;

	regulator_id =  rdev_get_id(dev);
	vcc_resource_data = rdev_get_drvdata(dev);
	base_addr = vcc_resource_data->base;
	bit_mask = 0x01 << regulator_id;

	lock(&reg_lock_mutex);
	/*step1:g3d clock*/
	if (IS_ERR(med_clock[regulator_id])) {
		pr_err("Regulator vcc:clock get failed\r\n");
		goto err;
	}
	ret = clk_enable(med_clock[regulator_id]);
	if (ret) {
		pr_err("Regulator vcc:clock enable failed\r\n");
		goto err;
	}
	/*step2: soft reset enable*/
	enable_softrst(base_addr, regulator_id);
	udelay(5);
	/*step 3:iso disable*/
	disable_iso(base_addr, regulator_id);
	/*step 4:reset disable*/
	/*disable core rst*/
	write_med_disable_softrst_register(base_addr, G3D_CORE_RST_BIT);
	udelay(1);
	/*disable intf rst*/
	write_med_disable_softrst_register(base_addr, G3D_INTF_RST_BIT);
	/*keep 128 cycles*/
	udelay(20);/*more than 150 cycles(more than 128 cycles) of 32M HZ clock*/

	/*step 5: notify ipps start gpu dvfs*/
	#ifdef CONFIG_IPPS_SUPPORT
	uippsmode = IPPS_ENABLE;
	ipps_set_mode(&vcc_ipps_client, IPPS_OBJ_GPU, &uippsmode);
	#endif
	unlock(&reg_lock_mutex);

	return 0;
err:
	unlock(&reg_lock_mutex);
	return -1;
}

static int vcc_g3d_disabled(struct regulator_dev *dev)
{
	int  regulator_id;
	void __iomem *base_addr = NULL;
	struct vcc_resource *vcc_resource_data;
	u32 uippsmode = 0;

	regulator_id =  rdev_get_id(dev);
	vcc_resource_data = rdev_get_drvdata(dev);
	base_addr = vcc_resource_data->base;

	lock(&reg_lock_mutex);
	/*step 1:stop dvfs avs*/
	#ifdef CONFIG_IPPS_SUPPORT
	uippsmode = IPPS_DISABLE;
	ipps_set_mode(&vcc_ipps_client, IPPS_OBJ_GPU, &uippsmode);
	#endif
	/*step 2:soft reset enable*/
	write_med_enable_softrst_register(base_addr, regulator_id);
	udelay(5);/*50 cycles of 30M HZ clock*/
	/*step 3:close ip clock*/
	clk_disable(med_clock[regulator_id]);
	udelay(5);
	/*step 4:iso enable*/
	enable_iso(base_addr, regulator_id);
	/*step 5:close mtcmos*/
	unlock(&reg_lock_mutex);

	return 0;
}

/*enable and disable function of edc1 are not include rst operation*/
static int vcc_edc1_med_enabled(struct regulator_dev *dev)
{
	int  regulator_id;
	void __iomem *base_addr = NULL;
	struct vcc_resource *vcc_resource_data;

	regulator_id =  rdev_get_id(dev);
	vcc_resource_data = rdev_get_drvdata(dev);
	base_addr = vcc_resource_data->base;

	lock(&reg_lock_mutex);
	/*step 1:mtcmos*/
	open_mtcmos(base_addr, regulator_id);
	udelay(5);
	/*step 2:reset disable*/
	enable_softrst(base_addr, regulator_id);
	udelay(5);
	/*step 3:iso disable*/
	disable_iso(base_addr, regulator_id);
	/*step 4:reset disable*/
	disable_softrst(base_addr, regulator_id);
	udelay(5);
	unlock(&reg_lock_mutex);
	return 0;
}

static int vcc_edc1_med_disabled(struct regulator_dev *dev)
{
	u32 bit_mask;
	int  regulator_id;
	void __iomem *base_addr = NULL;
	struct vcc_resource *vcc_resource_data;

	regulator_id =  rdev_get_id(dev);
	vcc_resource_data = rdev_get_drvdata(dev);
	base_addr = vcc_resource_data->base;
	bit_mask = 1 << regulator_id;

	lock(&reg_lock_mutex);
	/*step 1:iso enable*/
	enable_iso(base_addr, regulator_id);
	/*step 2:close mtcmos*/
	close_mtcmos(base_addr, regulator_id);

	unlock(&reg_lock_mutex);
	return 0;
}

struct regulator_ops vcc_med_ops = {
	.is_enabled = vcc_med_is_enabled,
	.enable = vcc_med_enabled,
	.disable = vcc_med_disabled,
};

/*edc1 power on/off are not include rst setting*/
struct regulator_ops vcc_edc1_ops = {
	.is_enabled = vcc_med_is_enabled,
	.enable = vcc_edc1_med_enabled,
	.disable = vcc_edc1_med_disabled,
};

struct regulator_ops vcc_cpu23_ops = {
	.is_enabled = vcc_cpu23_is_enabled,
	.enable = vcc_cpu23_enable,
	.disable = vcc_cpu23_disable,
};

struct regulator_ops vcc_g3d_ops = {
	.is_enabled = vcc_g3d_is_enabled,
	.enable = vcc_g3d_enabled,
	.disable = vcc_g3d_disabled,
};
static __devinit int vcc_regulator_probe(struct platform_device *pdev)
{
	int ret, regulator_id;
	struct vcc_resource  *vcc_resource_data;

	/*get initialized data*/
	struct regulator_init_data *regulator_init_data = pdev->dev.platform_data;

	/*get mem data*/
	vcc_resource_data = kzalloc(sizeof(*vcc_resource_data), GFP_KERNEL);
	if (vcc_resource_data == NULL) {
		pr_err("Regulator vcc:regulator kzalloc mem fail, please check!\n");
		return -ENOMEM;
	}

	vcc_resource_data->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	vcc_resource_data->base = ioremap(vcc_resource_data->res->start,
		resource_size(vcc_resource_data->res));
	if (vcc_resource_data->base == NULL) {
		ret = -ENOMEM;
		goto release_region;
	}
	/*clock is not enabled here*/

	/*register all regulator*/
	for (regulator_id = VCC_EDC1; regulator_id <= VCC_G3D; regulator_id++) {
		vcc_resource_data->rdev[regulator_id] =
			regulator_register(&vcc_regulator_desc[regulator_id], &pdev->dev,
				regulator_init_data + regulator_id, vcc_resource_data);
		if (IS_ERR(vcc_resource_data->rdev[regulator_id])) {
			ret = PTR_ERR(vcc_resource_data->rdev[regulator_id]);
			goto free_mem;
		}
		med_clock[regulator_id] = clk_get(NULL, clock_name[regulator_id]);
	}

	/*edc1 rst need ldi1 clock,this is special*/
	med_clock[LDI1_CLOCK] = clk_get(NULL, "clk_ldi1");
	med_clock[PLL2_CLOCK] = clk_get(NULL, "clk_pll2");

	platform_set_drvdata(pdev, vcc_resource_data);
	return 0;
release_region:
	release_mem_region(vcc_resource_data->res->start, resource_size(vcc_resource_data->res));
free_mem:
	iounmap(vcc_resource_data->base);
	kfree(vcc_resource_data);
	return ret;
}
static __devexit int vcc_regulator_remove(struct platform_device *pdev)
{
	int regulator_id;
	struct vcc_resource *vcc_resource_data;
	vcc_resource_data = platform_get_drvdata(pdev);
	/*release map phy_addr and virtual_addr*/
	if (vcc_resource_data->base)
		iounmap(vcc_resource_data->base);

	for (regulator_id = VCC_EDC1; regulator_id <= VCC_G3D; regulator_id++) {
		if (!IS_ERR(vcc_resource_data->rdev[regulator_id])) {
			regulator_unregister(vcc_resource_data->rdev[regulator_id]);
			clk_put(med_clock[regulator_id]);
			med_clock[regulator_id] = NULL;
		}
	}
	med_clock[LDI1_CLOCK] = NULL;
	/*release memory*/
	kfree(vcc_resource_data);
	return 0;
}

static struct platform_driver vcc_regulator_driver = {
	.probe = vcc_regulator_probe,
	.remove = vcc_regulator_remove,
	.driver		= {
		.name	= "vcc-regulator",
	},
};

static int __init vcc_regulator_init(void)
{
	int ret = 0;
#ifdef CONFIG_IPPS_SUPPORT
	ret = ipps_register_client(&vcc_ipps_client);
	if (ret != 0) {
		printk("regulator vcc register client failed, please check!");
		return ret;
	}
#endif
	return platform_driver_register(&vcc_regulator_driver);
}

static void __exit vcc_regulator_exit(void)
{
	platform_driver_unregister(&vcc_regulator_driver);
}
#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
fs_initcall(vcc_regulator_init);
#else
core_initcall(vcc_regulator_init);
#endif
module_exit(vcc_regulator_exit);
MODULE_LICENSE("GPL");
