/*
 * kernel/driver/mfd/hi6421-irq.c
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/bitops.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/irqnr.h>
#include <mach/early-debug.h>
#include <linux/err.h>
#include <mach/gpio.h>
#define   PMU_IRQ1	(0x01 << 2)
#define   PMU_IRQ2	(0x02 << 2)
#define   PMU_IRQ3	(0x03 << 2)
#define   PMU_IRQM1	(0x04 << 2)
#define   PMU_IRQM2	(0x05 << 2)
#define   PMU_IRQM3	(0x06 << 2)
#define   OCP_STATUS1 (0x54 << 2)
#define   OCP_STATUS2 (0x55 << 2)
#define   OCP_STATUS3 (0x56 << 2)
#define   OCP_STATUS4 (0x57 << 2)
#define   IRQM2_BASE_OFFSET	(8)
#define   IRQM3_BASE_OFFSET	(16)
#define	LDO12_OCP   (0X01 << 4)
#define	BUCK2_OCP   (0X01 << 2)
#define	BUCK5_OCP   (0X01 << 5)
#define   MASK_RESERVED_BIT 0x3F
/* number of interrupt*/
#define HI6421_PMU_NR	24

#define MASK_REG1	192

struct pmu_irq_data{

		void __iomem	*base;
		unsigned		irq_base;
		unsigned		irq;
		struct resource	*res;
		struct clk *clk_pmu;
		spinlock_t		irq_lock;
		struct workqueue_struct *regulator_ocp_wq;
		struct work_struct regulator_ocp_wk;
};
/*
 * PMU IRQ
 */
static void pmu_irq_mask(struct irq_data *data)
{
	unsigned char reg1 = 0, reg2 = 0, reg3 = 0, offset = 0;
	unsigned irq = data->irq;
	struct pmu_irq_data *pmu_irq_data = irq_get_chip_data(irq);

	if (NULL == pmu_irq_data) {
		pr_err("%s %d irq_get_chip_data return NULL\n", __func__, __LINE__);
		return;
	}

	if ((irq < IRQ_PMU_NR_BASE) || (irq > NR_IRQS)) {
		printk("hi6421 pmu irq is error,please check irq=%d\n", irq);
		return;
	}

	spin_lock(&pmu_irq_data->irq_lock);

	if (((irq - IRQ_PMU_NR_BASE) >> 3) == 0) {
		offset = irq - IRQ_PMU_NR_BASE;
		reg1 = ioread32(pmu_irq_data->base + PMU_IRQM1);
		reg1 |= 1 << offset;
		iowrite32(reg1, pmu_irq_data->base + PMU_IRQM1);
	} else if (((irq - IRQ_PMU_NR_BASE) >> 3) == 1) {
		offset = irq - IRQ_PMU_NR_BASE - IRQM2_BASE_OFFSET;
		reg2 = ioread32(pmu_irq_data->base + PMU_IRQM2);
		reg2 |= 1 << offset;
		iowrite32(reg2, pmu_irq_data->base + PMU_IRQM2);
	} else {
		offset = irq - IRQ_PMU_NR_BASE - IRQM3_BASE_OFFSET;
		reg3 = ioread32(pmu_irq_data->base + PMU_IRQM3);
		reg3 |= 1 << offset;
		iowrite32(reg3, pmu_irq_data->base + PMU_IRQM3);
	}

	spin_unlock(&pmu_irq_data->irq_lock);
}
static void pmu_irq_unmask(struct irq_data *data)
{
	unsigned char reg1 = 0, reg2 = 0, reg3 = 0, offset = 0;
	unsigned irq = data->irq;
	struct pmu_irq_data *pmu_irq_data = irq_get_chip_data(irq);

	if (NULL == pmu_irq_data) {
		pr_err("%s %d irq_get_chip_data return NULL\n", __func__, __LINE__);
		return;
	}

	if ((irq < IRQ_PMU_NR_BASE) || (irq > NR_IRQS)) {
		printk("pmu irq is error,please check irq=%d\n", irq);
		return;
	}

	spin_lock(&pmu_irq_data->irq_lock);
	if (((irq - IRQ_PMU_NR_BASE) >> 3) == 0) {
		offset = irq - IRQ_PMU_NR_BASE;
		reg1 = ioread32(pmu_irq_data->base + PMU_IRQM1);
		reg1 &= ~(1 << offset);
		iowrite32(reg1, pmu_irq_data->base + PMU_IRQM1);
	} else if (((irq - IRQ_PMU_NR_BASE) >> 3) == 1) {
		offset = irq - IRQ_PMU_NR_BASE - IRQM2_BASE_OFFSET;
		reg2 = ioread32(pmu_irq_data->base + PMU_IRQM2);
		reg2 &= ~(1 << offset);
		iowrite32(reg2, pmu_irq_data->base + PMU_IRQM2);
	} else {
		offset = irq - IRQ_PMU_NR_BASE - IRQM3_BASE_OFFSET;
		reg3 = ioread32(pmu_irq_data->base + PMU_IRQM3);
		reg3 &= ~(1 << offset);
		iowrite32(reg3, pmu_irq_data->base + PMU_IRQM3);
	}
	spin_unlock(&pmu_irq_data->irq_lock);
}
static struct irq_chip pmu_irqchip = {
	.name		= "PMU",
	.irq_mask		= pmu_irq_mask,
	.irq_unmask	= pmu_irq_unmask,
};

static void hi6421_irq_handler(unsigned irq, struct irq_desc *desc)
{
	unsigned int i = 0, reg = 0, reg1 = 0, reg2 = 0, reg3 = 0;
	unsigned int pwrkey = 0;
	struct pmu_irq_data *pmu_irq_data = irq_get_handler_data(irq);

	if (NULL == pmu_irq_data) {
		pr_err("%s %d irq_get_handler_data return NULL\n", __func__, __LINE__);
		return;
	}

	desc->irq_data.chip->irq_disable(&desc->irq_data);

	/*clean interrupt*/
	reg1 = ioread32(pmu_irq_data->base + PMU_IRQ1);
	if (reg1) {
		iowrite32(reg1, pmu_irq_data->base + PMU_IRQ1);
	}

	reg2 = ioread32(pmu_irq_data->base + PMU_IRQ2);
	if (reg2) {
		iowrite32(reg2, pmu_irq_data->base + PMU_IRQ2);
	}

	reg3 = ioread32(pmu_irq_data->base + PMU_IRQ3);
	if (reg3) {
		iowrite32(reg3, pmu_irq_data->base + PMU_IRQ3);
	}

	reg = reg1 | (reg2 << 8) | (reg3 << 16);

	pwrkey = 0x01 << (IRQ_POWER_KEY_PRESS -IRQ_PMU_NR_BASE) |
			0x01 << (IRQ_POWER_KEY_RELEASE -IRQ_PMU_NR_BASE);

	if ((reg & pwrkey) == pwrkey) {
		generic_handle_irq(IRQ_POWER_KEY_PRESS);
		generic_handle_irq(IRQ_POWER_KEY_RELEASE);
		reg &= (~pwrkey);
	}


	if (reg) {
		for (i = 0; i < HI6421_PMU_NR; i++) {
			if (reg & (1 << i)) {
				/* handle interrupt service */
				generic_handle_irq(IRQ_PMU_NR_BASE + i);
			}
		}
	}
	desc->irq_data.chip->irq_enable(&desc->irq_data);
}

extern struct regulator *regulator_get(struct device *dev, const char *id);
extern int regulator_force_disable(struct regulator *regulator);

static void inquiry_hi6421_ocp_reg(struct work_struct *work)
{
	struct pmu_irq_data *pmu_irq_data = container_of(work, struct pmu_irq_data, regulator_ocp_wk);
	u8 buck0_buck5_ocp_status, ldo0_ldo7_ocp_status, ldo8_ldo15_ocp_status, ldo16_ldo20_ocp_status;
	struct regulator *LDO12;
	int i;
	u32 ocp_status;

	buck0_buck5_ocp_status = ioread8(pmu_irq_data->base + OCP_STATUS1);
	ldo0_ldo7_ocp_status = ioread8(pmu_irq_data->base + OCP_STATUS2);
	ldo8_ldo15_ocp_status = ioread8(pmu_irq_data->base + OCP_STATUS3);
	ldo16_ldo20_ocp_status = ioread8(pmu_irq_data->base + OCP_STATUS4);

	buck0_buck5_ocp_status &= MASK_RESERVED_BIT;
	ldo16_ldo20_ocp_status &= MASK_RESERVED_BIT;

	ocp_status = buck0_buck5_ocp_status | (ldo0_ldo7_ocp_status << 8) |
				(ldo8_ldo15_ocp_status << 16) | (ldo16_ldo20_ocp_status << 24);

	for (i = 0; i < 32; i++) {
		if (ocp_status & (0x01 << i)) {
			if (i < 8) {
				printk("hi6421 regulator buck%d ocp happen!\n\r", i);
			} else {
				printk("hi6421 regulator ldo%d ocp happen!\n\r", (i - 8));
			}
		}
	}

	if (ldo8_ldo15_ocp_status & LDO12_OCP) {
		pr_info("*******************LDO12 OCP****************\n\r");
		LDO12 = regulator_get(NULL,"test-vcc");
		if (LDO12 == NULL) {
			pr_err("NULL pointer!\n\r");
		} else {
			regulator_force_disable(LDO12);
		}
	}

	//cleanout BUCK2 AND LDO12 OCP happen
	ocp_status &= (~BUCK2_OCP);
	ocp_status &= (~BUCK5_OCP);
	ocp_status &= (~(LDO12_OCP <<16));

	if (ocp_status) {
		//reset system
		BUG_ON(1);
	}
}

static irqreturn_t hi6421_regulator_current_over_support(int irq, void *data)
{
	struct pmu_irq_data *pmu_irq_data = data;
	queue_work(pmu_irq_data->regulator_ocp_wq, &pmu_irq_data->regulator_ocp_wk);
	return IRQ_HANDLED;
}

static int __init hi6421_irq_probe(struct platform_device *pdev)
{
	int ret;
	unsigned int i, irq, reg1, update_reg1;
	struct pmu_irq_data  *pmu_irq_data;
	struct irq_desc *desc, *gpio_desc;
	irq = platform_get_irq(pdev, 0);
	pmu_irq_data = kzalloc(sizeof(*pmu_irq_data), GFP_KERNEL);
	if (pmu_irq_data == NULL) {
		printk("hi6421 pmu irq kzalloc is failed,please check!\n");
		return -ENOMEM;
	}

	/*init spin lock*/
	spin_lock_init(&pmu_irq_data->irq_lock);

	pmu_irq_data->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (NULL == pmu_irq_data->res) {
		ret = -ENOMEM;
		printk("hi6421 pmu irq platform_get_resource err,please check!\n");
		goto free_mem;
	}

	pmu_irq_data->base = ioremap(pmu_irq_data->res->start, resource_size(pmu_irq_data->res));
	if (pmu_irq_data->base == NULL) {
		ret = -ENOMEM;
		printk("hi6421 pmu irq no base adder,please check!\n");
		goto free_mem;
	}
	/*get clock*/
	pmu_irq_data->clk_pmu = clk_get(NULL, "clk_pmuspi");
	if (pmu_irq_data->clk_pmu == NULL) {
		printk("hi6421 irq pmu clock get failed,please check!\n");
		ret = -ENXIO;
		goto err_iounmap;
	}
	/*enable the clock*/
	ret = clk_enable(pmu_irq_data->clk_pmu);
	if (ret) {
		printk("hi6421 irq pmu enable clk failed,please check!\n");
		goto err_clk_put;
	}

	reg1 = ioread32(pmu_irq_data->base + PMU_IRQ1);
	update_reg1 = MASK_REG1 & reg1;
	if (update_reg1) {
		iowrite32(update_reg1, pmu_irq_data->base + PMU_IRQ1);
		printk("k3v2 power key clean interrupt reg1=%x update_reg1=%x for boot\n", reg1, update_reg1);
	}

	/*handle interrupt in PMU,fromIRQ_ALARMON_RISING to  IRQ_RESERVED_PMU*/
	for (i = IRQ_ALARMON_RISING; i <= IRQ_RESERVED_PMU; i++) {
		irq_set_chip(i, &pmu_irqchip);
		irq_set_handler(i, handle_simple_irq);
		set_irq_flags(i, IRQF_VALID);
		irq_set_chip_data(i, pmu_irq_data);
		/* init mask pmu interrupt*/
		desc = irq_to_desc(i);
		desc->irq_data.chip->irq_mask(&desc->irq_data);
	}
	/*init and set GPIO19_7*/
	pmu_irq_data->irq = irq;
	ret = gpio_request(GPIO_19_7, "pmuirq");
	if (ret) {
		printk("hi6421-irq probe can't request gpio for data.\n");
		goto err_disable_clk;
	}
	gpio_direction_input(GPIO_19_7);
	gpio_desc = irq_to_desc(irq);
	irq_set_handler_data(irq, pmu_irq_data);
	irq_set_chained_handler(irq, hi6421_irq_handler);
	irq_set_irq_type(irq, IRQ_TYPE_LEVEL_LOW);
	gpio_desc->irq_data.chip->irq_enable(&gpio_desc->irq_data);

	pmu_irq_data->regulator_ocp_wq = create_singlethread_workqueue("hi6421_regulator_ocp");
	INIT_WORK(&pmu_irq_data->regulator_ocp_wk, (void *)inquiry_hi6421_ocp_reg);
	/*register regulator ocp interrupt*/
	ret = request_irq(IRQ_OCP_RISING, hi6421_regulator_current_over_support, IRQF_DISABLED, "hi6421-irq", pmu_irq_data);
	if (ret) {
		printk("hi6421 irq register ocp interrupt failed!\n");
		goto err_gpio_free;
	}

	/*set pmu_irq_data in order to get at last*/
	platform_set_drvdata(pdev, pmu_irq_data);
	return 0;
err_gpio_free:
	gpio_free(GPIO_19_7);
err_disable_clk:
	clk_disable(pmu_irq_data->clk_pmu);
err_clk_put:
	clk_put(pmu_irq_data->clk_pmu);
err_iounmap:
	iounmap(pmu_irq_data->base);
free_mem:
	kfree(pmu_irq_data);
	return ret;
}
static  int hi6421_irq_remove(struct platform_device *pdev)
{
	unsigned int i;
	struct pmu_irq_data *pmu_irq_data;
	/*get pmu_irq_data*/
	pmu_irq_data = platform_get_drvdata(pdev);
	if (NULL == pmu_irq_data) {
		pr_err("%s %d platform_get_drvdata NULL\n", __func__, __LINE__);
		return -1;
	}

	/*release map phy_addr and virtual_addr*/
	iounmap(pmu_irq_data->base);
	/*disable clock*/
	if (!IS_ERR(pmu_irq_data->clk_pmu)) {
		clk_disable(pmu_irq_data->clk_pmu);
		clk_put(pmu_irq_data->clk_pmu);
	}

	/*release memory*/
	kfree(pmu_irq_data);
	gpio_free(GPIO_19_7);

	/* cleanup irqchip */
	for (i = IRQ_ALARMON_RISING; i <= IRQ_RESERVED_PMU; i++) {
		irq_set_chip_and_handler(i, NULL, NULL);
	}

	return 0;
}
#ifdef CONFIG_PM
static int hi6421_irq_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct pmu_irq_data *pmu_irq_data;

	printk("hi6421 pmu irq entry suspend successfully");

	/*get pmu_irq_data*/
	pmu_irq_data = platform_get_drvdata(pdev);
	if (NULL == pmu_irq_data) {
		pr_err("%s %d platform_get_drvdata NULL\n", __func__, __LINE__);
		return -1;
	}

	/*disable clock*/
	if (!IS_ERR(pmu_irq_data->clk_pmu)) {
		clk_disable(pmu_irq_data->clk_pmu);
	}

	return 0;
}
static int hi6421_irq_resume(struct platform_device *pdev)
{
	int ret = 0;
	struct pmu_irq_data *pmu_irq_data;

	printk("hi6421 pmu irq entry resume successfully");
	/*get pmu_irq_data*/
	pmu_irq_data = platform_get_drvdata(pdev);
	if (NULL == pmu_irq_data) {
		pr_err("%s %d platform_get_drvdata NULL\n", __func__, __LINE__);
		return -1;
	}

	/*disable clock*/
	if (!IS_ERR(pmu_irq_data->clk_pmu)) {
		ret = clk_enable(pmu_irq_data->clk_pmu);
		if (ret)
			printk("hi6421_irq_resume enable clk failed,please check.\n");
	}
	return ret;
}
#endif
static struct platform_driver hi6421_irq_driver = {
	.probe = hi6421_irq_probe,
	.remove = hi6421_irq_remove,
	#ifdef CONFIG_PM
	.suspend = hi6421_irq_suspend,
	.resume = hi6421_irq_resume,
	#endif
	.driver		= {
		.name	= "hi6421-irq",
	},
};
static int __init hi6421_irq_init(void)
{
	int ret = 0;
	ret = platform_driver_register(&hi6421_irq_driver);
	if (ret != 0)
		printk("Failed to register hi6421_pmu_irq_driver driver: %d\n", ret);
	return ret;
}
static void __exit hi6421_irq_exit(void)
{
	platform_driver_unregister(&hi6421_irq_driver);
}
module_exit(hi6421_irq_exit);
subsys_initcall(hi6421_irq_init);
MODULE_LICENSE("GPL");
