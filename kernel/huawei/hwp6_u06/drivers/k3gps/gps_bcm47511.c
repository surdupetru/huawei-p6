#include <linux/amba/bus.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/mux.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/system.h>
#include <asm/pmu.h>
#include <mach/system.h>
#include <mach/platform.h>
#include <mach/lm.h>
#include <mach/irqs.h>
#include <mach/io.h>
#include <mach/early-debug.h>
#include <mach/gpio.h>



typedef struct gps_bcm_info {
	unsigned long       gpioid_en;
	unsigned long       gpioid_ret;
	struct iomux_block  *piomux_block;
	struct block_config *pblock_config;
	struct clk          *clk;
#ifndef CONFIG_MACH_K3V2OEM1
	unsigned long       gpioid_power;
#endif
} GPS_BCM_INFO;


static int __devinit k3_gps_bcm_probe(struct platform_device *pdev)
{
	GPS_BCM_INFO *gps_bcm;
	struct resource *res;
	int ret = 0;

	gps_bcm = kzalloc(sizeof(GPS_BCM_INFO), GFP_KERNEL);
	if (!gps_bcm) {
		dev_err(&pdev->dev, "Alloc memory failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, gps_bcm);

	/* Get enable gpio */
	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (!res) {
		dev_err(&pdev->dev, "Get enable gpio resourse failed\n");
		ret = -ENXIO;
		goto err_free;
	}
	gps_bcm->gpioid_en = res->start;

	ret = gpio_request(gps_bcm->gpioid_en, "gps_enbale");
	if (ret < 0) {
		dev_err(&pdev->dev,  "gpio_request failed, gpio=%lu, ret=%d\n", gps_bcm->gpioid_en, ret);
		goto err_free;
	}
	gpio_export(gps_bcm->gpioid_en, false);

	/* Get reset gpio */
	res = platform_get_resource(pdev, IORESOURCE_IO, 1);
	if (!res) {
		dev_err(&pdev->dev, "Get reset gpio resourse failed\n");
		ret = -ENXIO;
		goto err_free_gpio_en;
	}
	gps_bcm->gpioid_ret = res->start;

	ret = gpio_request(gps_bcm->gpioid_ret, "gps_reset");
	if (ret < 0) {
		dev_err(&pdev->dev,  "gpio_request failed, gpio=%lu, ret=%d\n", gps_bcm->gpioid_ret, ret);
		goto err_free_gpio_en;
	}
	gpio_export(gps_bcm->gpioid_ret, false);

#ifndef CONFIG_MACH_K3V2OEM1
	/* Get power gpio (VDDIO 1.8V), Only for FPGA */
	res = platform_get_resource(pdev, IORESOURCE_IO, 2);
	if (!res) {
		dev_err(&pdev->dev, "Get power gpio resourse failed\n");
		ret = -ENXIO;
		goto err_free_gpio_ret;
	}
	gps_bcm->gpioid_power = res->start;

	ret = gpio_request(gps_bcm->gpioid_power, "gps_power");
	if (ret < 0) {
		dev_err(&pdev->dev, "gpio_request failed, gpio=%lu, rc=%d\n", gps_bcm->gpioid_power, ret);
		gpio_free(gps_bcm->gpioid_en);
		goto err_free_gpio_ret;
	}

	/* Low GPS power, only for FPGA */
	gpio_direction_output(gps_bcm->gpioid_power, 0);

	/* High GPS power, only for FPGA */
	gpio_set_value(gps_bcm->gpioid_power, 1);
	dev_dbg(&pdev->dev,  "High power\n");
#else
	/* Set 32KC clock */
	gps_bcm->clk = clk_get(NULL, "clk_pmu32kc");
	if (IS_ERR(gps_bcm->clk)) {
		dev_err(&pdev->dev, "Get gps clk failed\n");
		ret = PTR_ERR(gps_bcm->clk);
		goto err_free_gpio_ret;
	}
	ret = clk_enable(gps_bcm->clk);
	if (ret) {
		dev_err(&pdev->dev, "Enable clk failed, ret=%d\n", ret);
		goto err_free_clk;
	}

	/* Set iomux NORMAL, If set iomux failed, we still go on */
	gps_bcm->piomux_block  = iomux_get_block("block_gps_boardcom");
	if (!gps_bcm->piomux_block)
		dev_err(&pdev->dev, "Get gps iomux_block failed\n");

	gps_bcm->pblock_config = iomux_get_blockconfig("block_gps_boardcom");
	if (!gps_bcm->pblock_config)
		dev_err(&pdev->dev, "Get gps block_config failed\n");

	if ((gps_bcm->piomux_block) && (gps_bcm->pblock_config)) {
		ret = blockmux_set(gps_bcm->piomux_block, gps_bcm->pblock_config, NORMAL);
		if (ret)
			dev_err(&pdev->dev, "Set gps iomux to NORMAL failed, ret=%d\n", ret);
	}
#endif

	/* Low Reset GPIO */
	gpio_direction_output(gps_bcm->gpioid_ret, 0);
	dev_dbg(&pdev->dev,  "Low reset\n");

	/* Low Enable GPIO */
	gpio_direction_output(gps_bcm->gpioid_en, 0);
	dev_dbg(&pdev->dev,  "Low enable\n");

	/* High Reset GPIO*/
	gpio_set_value(gps_bcm->gpioid_ret, 1);
	dev_dbg(&pdev->dev,  "High reset\n");

	return 0;

#ifdef CONFIG_MACH_K3V2OEM1
err_free_clk:
	clk_put(gps_bcm->clk);
#endif

err_free_gpio_ret:
	gpio_free(gps_bcm->gpioid_ret);

err_free_gpio_en:
	gpio_free(gps_bcm->gpioid_en);

err_free:
	kfree(gps_bcm);
	gps_bcm = NULL;
	return ret;
}


static int __devexit k3_gps_bcm_remove(struct platform_device *pdev)
{
	GPS_BCM_INFO *gps_bcm = platform_get_drvdata(pdev);
	int ret = 0;

	dev_dbg(&pdev->dev, "k3_gps_bcm_remove +\n");

	if (!gps_bcm) {
		dev_err(&pdev->dev, "gps_bcm is NULL\n");
		return 0;
	}

#ifdef CONFIG_MACH_K3V2OEM1
	if ((gps_bcm->piomux_block) && (gps_bcm->pblock_config)) {
		ret = blockmux_set(gps_bcm->piomux_block, gps_bcm->pblock_config, LOWPOWER);
		if (ret)
			dev_err(&pdev->dev, "Set gps iomux to LOWPOWER failed, ret=%d\n", ret);
	}

	clk_disable(gps_bcm->clk);
	clk_put(gps_bcm->clk);
#else
	gpio_free(gps_bcm->gpioid_power);
#endif

	gpio_free(gps_bcm->gpioid_en);
	gpio_free(gps_bcm->gpioid_ret);

	kfree(gps_bcm);
	gps_bcm = NULL;
	platform_set_drvdata(pdev, NULL);

	dev_dbg(&pdev->dev, "k3_gps_bcm_remove -\n");

	return ret;
}

static void K3_gps_bcm_shutdown(struct platform_device *pdev)
{
	GPS_BCM_INFO *gps_bcm = platform_get_drvdata(pdev);
#ifdef CONFIG_MACH_K3V2OEM1
	int ret = 0;
#endif

	if (!gps_bcm) {
		dev_err(&pdev->dev, "gps_bcm is NULL\n");
		return;
	}

	printk("[%s] +\n", __func__);

	gpio_set_value(gps_bcm->gpioid_en, 0);
	gpio_set_value(gps_bcm->gpioid_ret, 0);

#ifdef CONFIG_MACH_K3V2OEM1
	if ((gps_bcm->piomux_block) && (gps_bcm->pblock_config)) {
		ret = blockmux_set(gps_bcm->piomux_block, gps_bcm->pblock_config, LOWPOWER);
		if (ret)
			dev_err(&pdev->dev, "Set gps iomux to LOWPOWER failed, ret=%d\n", ret);
	}

	clk_disable(gps_bcm->clk);
	clk_put(gps_bcm->clk);
#else
	gpio_set_value(gps_bcm->gpioid_ret, 0);
	gpio_free(gps_bcm->gpioid_power);
#endif

	printk("[%s] -\n", __func__);
}


#ifdef CONFIG_PM
static int  k3_gps_bcm_suspend(struct platform_device *pdev, pm_message_t state)
{
	GPS_BCM_INFO *gps_bcm = platform_get_drvdata(pdev);

	if (!gps_bcm) {
		dev_err(&pdev->dev, "gps_bcm is NULL\n");
		return 0;
	}

	printk("[%s] +\n", __func__);

	gpio_set_value(gps_bcm->gpioid_en, 0);

	printk("[%s] -\n", __func__);
	return 0;
}
#else

#define k3_gps_bcm_suspend	NULL

#endif /* CONFIG_PM */

static struct platform_driver k3_gps_bcm_driver = {
	.driver	= {
		.name		= "k3_gps_bcm_47511",
		.owner		= THIS_MODULE,
	},
	.probe			= k3_gps_bcm_probe,
	.suspend		= k3_gps_bcm_suspend,
	.remove			= __devexit_p(k3_gps_bcm_remove),
	.shutdown		= K3_gps_bcm_shutdown,
};

static int __init k3_gps_bcm_init(void)
{
	return platform_driver_register(&k3_gps_bcm_driver);
}

static void __exit k3_gps_bcm_exit(void)
{
	platform_driver_unregister(&k3_gps_bcm_driver);
}


module_init(k3_gps_bcm_init);
module_exit(k3_gps_bcm_exit);

MODULE_AUTHOR("DRIVER_AUTHOR");
MODULE_DESCRIPTION("GPS Boardcom 47511 driver");
MODULE_LICENSE("GPL");

