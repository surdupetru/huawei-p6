/*******************************************************************************
 * Copyright:   Copyright (c) 2011. Hisilicon Technologies, CO., LTD.
 * Version:     V200
 * Filename:    adc_os.c
 * Description: adc_os.c
 * History:     1.Created by 00186176 on 2011/08/02
*******************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <linux/delay.h>
#include <mach/early-debug.h>
#include <linux/semaphore.h>
#include <linux/hkadc/hiadc_hal.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
/*OS define*/
#include <linux/hkadc/hiadc_linux.h>

struct adc_device *adc_dev;
EXPORT_SYMBOL_GPL(adc_dev);
static struct semaphore	 hi_lock;
void k3_adc_print_error(char *s)
{
	if (!s) {
		printk("string is error!\n");
	}
	printk("%s\n", s);
}

void k3_adc_usleep(int us)
{
	udelay(us);
}

void k3_adc_msleep(int ms)
{
	msleep(ms);
}

void k3_adc_msleep_range(int ms_min, int ms_max)
{
	usleep_range(ms_min, ms_max);
}

void k3_adc_lock(void)
{
	down(&hi_lock);
}

void k3_adc_release(void)
{
	up(&hi_lock);
}

void k3_adc_reg_read(unsigned char addr, unsigned char *read_value, unsigned int count)
{
	unsigned char reg_addr = addr;
	unsigned int reg_addr_new = addr;
	unsigned int txcnt = 0;
	unsigned long flags = 0;

	if (read_value == NULL || count == 0) {
		k3_adc_print_error("INPUT ERROR.\r\n");
		return;
	}

	if ((reg_addr + count - 1) > 0xFF) {
		k3_adc_print_error("INPUT ERROR.\r\n");
		return;
	}

	spin_lock_irqsave(&adc_dev->reg_lock, flags);
	while (txcnt < count) {
		reg_addr_new = reg_addr<<ADC_SHITF;
		*read_value = ioread32(adc_dev->regs + reg_addr_new);

		read_value++;
		reg_addr++;
		txcnt++;
	}

	spin_unlock_irqrestore(&adc_dev->reg_lock, flags);
}

void k3_adc_reg_write(unsigned char addr, unsigned char write_value, unsigned char mask)
{
	unsigned char read_data      = 0;
	unsigned int reg_addr_new    = addr<<ADC_SHITF;
	unsigned long flags = 0;

	/*judge parameter legality*/
	if ((addr) > 0xFF) {
		k3_adc_print_error("INPUT ERROR.\r\n");
		return;
	}

	spin_lock_irqsave(&adc_dev->reg_lock, flags);

	if (0 != mask) {
		read_data = ioread32(adc_dev->regs + reg_addr_new);
		read_data  &= mask;
		write_value &= ~(mask);
		write_value |= read_data;
	}

	iowrite32(write_value, adc_dev->regs + reg_addr_new);

	spin_unlock_irqrestore(&adc_dev->reg_lock, flags);
}

void k3_adc_set_pmu(unsigned char addr, unsigned char write_data, unsigned char mask)
{
	int ret = 0;

	ret = clk_enable(adc_dev->clk);
	if (ret) {
		clk_disable(adc_dev->clk);
		k3_adc_print_error("hi6421 pmu clk enable failed\n");
		return;
	}

	k3_adc_reg_write(addr, write_data, mask);
	clk_disable(adc_dev->clk);
}

void k3_adc_get_pmu(unsigned char addr, unsigned char *read_data)
{
	int ret = 0;

	ret = clk_enable(adc_dev->clk);
	if (ret) {
		clk_disable(adc_dev->clk);
		k3_adc_print_error("hi6421 pmu clk enable failed\n");
		return;
	}

	k3_adc_reg_read(addr, read_data, 1);

	clk_disable(adc_dev->clk);
}

void k3_adc_power(int flag)
{
	unsigned char write_reg  = 0;
	int ret = 0;

	ret = clk_enable(adc_dev->clk);
	if (ret) {
		k3_adc_print_error("hi6421 pmu clk enable failed\n");
		return;
	}

	if (ENABLE_ADC == flag) {
		/*enable LDO_AUDIO*/
		ret = regulator_enable(adc_dev->regu);
		if (ret < 0) {
			k3_adc_print_error("error regulator for adc client\n");
			clk_disable(adc_dev->clk);
			return;
		}

		ret = regulator_set_optimum_mode(adc_dev->regu, ADO_AUDIO_CURRENT);
		if (ret < 0) {
			k3_adc_print_error("regulator_set_optimum_mode is error\n");
			regulator_disable(adc_dev->regu);
			clk_disable(adc_dev->clk);
			return;
		}

		ret = regulator_set_voltage(adc_dev->regu, LDO_AUDIO_VOL, LDO_AUDIO_VOL);
		if (ret < 0) {
			k3_adc_print_error("regulator_set_voltage is error\n");
			regulator_disable(adc_dev->regu);
			clk_disable(adc_dev->clk);
			return;
		}
	} else {
		/*disable LDO_AUDIO*/
		write_reg = (unsigned char)(HKADC_BYPASS_YES);
		k3_adc_reg_write(HKADC_SETUP_ADDR, write_reg, 0xF7);

		regulator_disable(adc_dev->regu);
	}

	clk_disable(adc_dev->clk);

	return;
}

static int k3_adc_probe(struct platform_device *pdev)
{
	struct	adc_device *adc		= NULL;
	struct	resource *regs		= NULL;
	struct	adc_data *adc_table	= NULL;
	int		ret = 0;
	int		i = 0;
	int		sum = 0;

	adc = kzalloc(sizeof(struct adc_device), GFP_KERNEL);
	if (adc == NULL) {
		dev_err(&pdev->dev, "failed to allocate adc_device\n");
		ret = -ENOMEM;
		return ret;
	}

	/*get the clock with con_id = "clk_pmuspi"*/
	adc->clk = clk_get(NULL, "clk_pmuspi");
	if (adc->clk == NULL) {
		k3_adc_print_error("hi6421 pmu clock get failed\n");
		ret = -ENXIO;
		goto err_pmu_clk;
	}

	adc->regu = regulator_get(&pdev->dev, "hkadc-vcc");
	if (IS_ERR(adc->regu)) {
		k3_adc_print_error("regulator_get failed\n");
		ret = -ENXIO;
		goto err_clk;
	}

	sema_init(&hi_lock, 1);
	spin_lock_init(&adc->reg_lock);
	adc->pdev = pdev;

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		k3_adc_print_error("failed to find registers\n");
		ret = -ENXIO;
		goto err_reg;
	}

	adc->regs = ioremap(regs->start, resource_size(regs));
	if (!adc->regs) {
		k3_adc_print_error("failed to map registers\n");
		ret = -ENXIO;
		goto err_reg;
	}

	sum = ((struct adc_dataEx *)(adc->pdev->dev.platform_data))->sum;
	adc_table = ((struct adc_dataEx *)(adc->pdev->dev.platform_data))->data;

	for (i = 0; i < sum; i++)
		adc_table[i].count = 0;

	dev_info(&pdev->dev, "attached adc driver\n");

	platform_set_drvdata(pdev, adc);

	adc_dev = adc;

	k3_adc_power(ENABLE_ADC);

	return 0;
err_reg:
	regulator_put(adc->regu);
err_clk:
	clk_put(adc->clk);
	adc->clk = NULL;
err_pmu_clk:
	kfree(adc);
	return ret;
}

#ifdef CONFIG_PM
static int  k3_adc_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*LDO*/
	k3_adc_lock();
	k3_adc_power(DISABLE_ADC);

	dev_info(&pdev->dev, "HKADC suspend successfully\n");

	return 0;
}

static int k3_adc_resume(struct platform_device *pdev)
{
	k3_adc_power(ENABLE_ADC);
	k3_adc_release();

	dev_info(&pdev->dev, "HKADC resume successfully\n");

	return 0;
}
#endif

static int __devexit k3_adc_remove(struct platform_device *pdev)
{
	struct adc_device *adc = platform_get_drvdata(pdev);

	if (adc == NULL) {
		dev_err(&pdev->dev, "adc is null\n");
		return -ENODEV;
	}
	k3_adc_lock();
	k3_adc_power(DISABLE_ADC);
	iounmap(adc->regs);

	clk_put(adc->clk);
	regulator_put(adc->regu);
	kfree(adc);
	platform_set_drvdata(pdev, NULL);
	k3_adc_release();
	return 0;
}

static void k3_adc_shutdown(struct platform_device *pdev)
{
	struct adc_device *adc = platform_get_drvdata(pdev);
	if (adc == NULL) {
		dev_err(&pdev->dev, "adc is null\n");
		return ;
	}

	k3_adc_print_error("k3_adc_shutdown+.\r\n");

	k3_adc_lock();

	k3_adc_power(DISABLE_ADC);

	iounmap(adc->regs);

	clk_put(adc->clk);

	regulator_put(adc->regu);

	kfree(adc);

	adc = NULL;
	adc_dev = NULL;
	k3_adc_release();

	k3_adc_print_error("k3_adc_shutdown-.\r\n");

	return;
}

static struct platform_driver hi6421_adc_driver = {
	.probe      = k3_adc_probe,
#ifdef CONFIG_PM
	.suspend   = k3_adc_suspend,
	.resume    = k3_adc_resume,
#endif
	.remove   = __devexit_p(k3_adc_remove),
	.shutdown  = k3_adc_shutdown,
	.driver      = {
		.owner  = THIS_MODULE,
		.name   = "k3adc",
	},
};

static int __init k3_adc_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&hi6421_adc_driver);
	if (ret)
		k3_adc_print_error("failed to add adc driver\n");

	return ret;
}

static void __exit k3_adc_exit(void)
{
	platform_driver_unregister(&hi6421_adc_driver);
}

arch_initcall(k3_adc_init);
module_exit(k3_adc_exit);

MODULE_AUTHOR("Huawei Technologies Co.,Ltd");
MODULE_DESCRIPTION("K3 ADC Device Driver");
MODULE_LICENSE("GPL");
