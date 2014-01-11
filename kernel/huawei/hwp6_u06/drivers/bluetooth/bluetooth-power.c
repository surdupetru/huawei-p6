/* Copyright (c) 2009-2010,	Code Aurora	Forum. All rights reserved.
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under	the	terms of the GNU General Public	License	version	2 and
 * only	version	2 as published by the Free Software	Foundation.
 *
 * This	program	is distributed in the hope that	it will	be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A	PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received	a copy of the GNU General Public License
 * along with this program;	if not,	write to the Free Software
 * Foundation, Inc., 51	Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/*
 * Bluetooth Power Switch Module
 * controls	power to external Bluetooth	device
 * with	interface to power management device
 */



#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <mach/gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mux.h>

struct bluepower_info {
	bool             previous;
	unsigned	     gpio_enable;
	unsigned		 gpio_rst;
#ifndef CONFIG_MACH_K3V2OEM1
	unsigned         gpio_bt_vdd;
#endif
	unsigned		 sleep_clk;
	unsigned		 normal_clk;
	unsigned		 pwr_count;
	struct regulator   *regulator;
	struct clk		   *bt_clk;
	struct rfkill    *rfkill;
	struct iomux_block	*gpio_blockTemp;
	struct block_config	*gpio_configTemp;
};

/*
*set the bluetooth vddio ldo voltage 1.8V
*/
#define BT_VDDIO_LDO_18V 1800000
/*
 *config	the	bt enable/reset/bt wakeup/host wakeup io
 */
#define BT_GPIO_IOMUX_BLOCK "block_btpwr"
#define	BT_SHUTDOWN	0


static int bluetooth_power_handle(struct bluepower_info *bt_info, int on)
{
	int	ret	= 0;
	pr_debug("%s,%d.\n", __func__, on);

	/* power on	the	bluetooth */
	if (on)	{
		if (bt_info->pwr_count == 0) {

			printk("%s, bluetooth power on +\n", __func__);

#ifdef CONFIG_MACH_K3V2OEM1

			pr_debug("set mux normal mode======>\n");

			/*SET NORMAL*/
			ret	= blockmux_set(bt_info->gpio_blockTemp,
					bt_info->gpio_configTemp, NORMAL);
			if (unlikely(ret)) {
				pr_err("%s:	SETERROR:gpio blockmux set err %d\r\n",
					__func__, ret);
				return ret;
			}

			pr_debug("enable bluetooth 32 clk======>\n");

			/*enable bt	sleep clk*/
			ret	= clk_enable(bt_info->bt_clk);
			if (unlikely(ret < 0)) {
				pr_err("%s : enable	clk	failed %d",
					__func__, ret);
				goto clk_err;
			}

//			pr_debug("enable regulator of LDO14======>\n");

			/*enable bt	VDDIO*/
/*			ret	= regulator_enable(bt_info->regulator);
			if (unlikely(ret < 0)) {
				pr_err("%s:	regulator enable failed	%d\n",
					__func__, ret);
				goto regulator_err;
			}
*/
#endif
			msleep(1);
			/*bt power on*/
			gpio_set_value(bt_info->gpio_enable, 1);
			msleep(5);
			/*bt rst on*/
			gpio_set_value(bt_info->gpio_rst, 1);
			msleep(5);
			bt_info->pwr_count = 1;

			printk("%s, bluetooth power on -.\n", __func__);

		}
	} else {
		/* power off the bluetooth */
		if (bt_info->pwr_count == 1) {

			printk("%s, bluetooth power off +.\n", __func__);

			gpio_set_value(bt_info->gpio_rst, 0);
			gpio_set_value(bt_info->gpio_enable, 0);

#ifdef CONFIG_MACH_K3V2OEM1

//			pr_debug("disable bluetooth regulator LDO14======>\n");

			/*disable VDDIO*/
/*			ret	= regulator_disable(bt_info->regulator);
			if (unlikely(ret < 0))
				pr_err("%s:	regulator disable failed %d\n",
					__func__, ret);

			pr_debug("disable bluetooth clk======>\n");
*/
			/*disable sleep clk*/
//			clk_disable(bt_info->bt_clk);

//			pr_debug("set mux lowpower mode======>\n");

			/*SET lowpower*/
			ret	= blockmux_set(bt_info->gpio_blockTemp,
					bt_info->gpio_configTemp, LOWPOWER);
			if (unlikely(ret)) {
				pr_err("%s:	SETERROR:gpio blockmux set err %d\r\n",
						__func__, ret);
			}
#endif

			bt_info->pwr_count = 0;

			printk("%s, bluetooth power off -.\n", __func__);

		}
	}
	return ret;

#ifdef	CONFIG_MACH_K3V2OEM1
regulator_err:
	clk_disable(bt_info->bt_clk);
clk_err:
	if (blockmux_set(bt_info->gpio_blockTemp,
			bt_info->gpio_configTemp, LOWPOWER))
		pr_err("%s:	blockmux_set err \n", __func__);

#endif

	return ret;
}


static int bluetooth_toggle_radio(void *data, bool blocked)
{
	int	ret	= 0;
	struct bluepower_info *bt_info = platform_get_drvdata(data);

	if (NULL == bt_info) {
		pr_err("%s:	bt_info is NULL \n", __func__);
		ret = -1;
		return ret;
	}

	if (bt_info->previous != blocked) {
		ret = bluetooth_power_handle(bt_info, (!blocked));
		bt_info->previous =	blocked;
	}

	return ret;
}

static const struct	rfkill_ops bluetooth_power_rfkill_ops = {
	.set_block = bluetooth_toggle_radio,
};

static int bluetooth_power_rfkill_probe(struct platform_device *pdev,
			struct bluepower_info *bt_info)
{
	int	ret	= 0;

	bt_info->rfkill = rfkill_alloc("bt_power", &pdev->dev,
			RFKILL_TYPE_BLUETOOTH, &bluetooth_power_rfkill_ops,
				pdev);
	if (!bt_info->rfkill) {
		dev_err(&pdev->dev,	"rfkill	allocate failed\n");
		return -ENOMEM;
	}
	/* force Bluetooth off during init to allow	for	user control */
	rfkill_init_sw_state(bt_info->rfkill, 1);
	bt_info->previous =	1;
	ret	= rfkill_register(bt_info->rfkill);
	if (ret) {
		dev_err(&pdev->dev,	"rfkill	register failed=%d\n", ret);
		goto rfkill_failed;
	}

	return ret;

rfkill_failed:
	rfkill_destroy(bt_info->rfkill);
	return ret;
}

static void	bluetooth_power_rfkill_remove(struct platform_device *pdev)
{

	struct bluepower_info *bt_info = platform_get_drvdata(pdev);

	if (NULL == bt_info) {
		dev_err(&pdev->dev,	"%s, bt_info is null \n", __func__);
		return;
	}

	if (bt_info->rfkill)
		rfkill_unregister(bt_info->rfkill);
	rfkill_destroy(bt_info->rfkill);

}


#ifdef CONFIG_MACH_K3V2OEM1
/*
*Initialize the bluetooth io mux
*/
static int	bt_io_init(struct platform_device *pdev,
					struct bluepower_info *bt_info)
{
	int	ret	= 0;

	bt_info->gpio_blockTemp	= iomux_get_block(BT_GPIO_IOMUX_BLOCK);
	if (IS_ERR(bt_info->gpio_blockTemp)) {
		dev_err(&pdev->dev,	"%s	: iomux_get_block gpio_blockTemp failed\n",
			__func__);
		ret = -ENXIO;
		return ret;
	}

	bt_info->gpio_configTemp = iomux_get_blockconfig(BT_GPIO_IOMUX_BLOCK);
	if (IS_ERR(bt_info->gpio_blockTemp)) {
		dev_err(&pdev->dev,	"%s	: iomux_blockconfig_lookup gpio_configTemp:\n",
			__func__);
		ret = -ENXIO;
		goto err_block_config;
	}

	ret	= blockmux_set(bt_info->gpio_blockTemp,
			bt_info->gpio_configTemp, LOWPOWER);
	if (ret) {
		dev_err(&pdev->dev,	"%s	: gpio_blockTemp blockmux set err %d\r\n",
			__func__, ret);
		goto err_mux_set;
	}

	return ret;


err_mux_set:
	if (bt_info->gpio_configTemp)
		bt_info->gpio_configTemp = NULL;
err_block_config:
	if (bt_info->gpio_blockTemp)
		bt_info->gpio_blockTemp	= NULL;

	return ret;
}

#endif

static int __devinit bt_power_probe(struct platform_device *pdev)
{
	int	ret	= 0;
	struct resource	*res;
	struct bluepower_info *bt_info;

	dev_dbg(&pdev->dev,	"%s\n",	__func__);

	bt_info	= kzalloc(sizeof(struct	bluepower_info), GFP_KERNEL);
	if (!bt_info) {
		dev_err(&pdev->dev,	"couldn't kzalloc bluepower_info.\n");
		return -ENOMEM;
	}

#ifdef	CONFIG_MACH_K3V2OEM1
	ret = bt_io_init(pdev, bt_info);
	if (ret) {
		dev_err(&pdev->dev,	"bt io init failed.%d\n", ret);
		goto free_bpi;
	}
#endif
	res	= platform_get_resource_byname(pdev, IORESOURCE_IO,
					   "bt_gpio_enable");
	if (!res) {
		dev_err(&pdev->dev,	"couldn't find bt_gpio_enable gpio\n");
		ret	= -ENODEV;
		goto free_bpi;
	}

	bt_info->gpio_enable = res->start;

	ret	= gpio_request(bt_info->gpio_enable, "bt_gpio_enable");
	if (ret) {
		dev_err(&pdev->dev,	"couldn't request bt_gpio_enable gpio*****\n");
		goto free_bpi;
	}

	ret	= gpio_direction_output(bt_info->gpio_enable, 0);
	if (ret) {
		dev_err(&pdev->dev,	"couldn't gpio_direction_output	enable gpio\n");
		goto free_bt_enable;
	}

	res	= platform_get_resource_byname(pdev, IORESOURCE_IO,
					   "bt_gpio_rst");
	if (!res) {
		dev_err(&pdev->dev,	"couldn't find bt_gpio_rst gpio\n");
		ret	= -ENODEV;
		goto free_bt_enable;
	}

	bt_info->gpio_rst =	res->start;

	ret	= gpio_request(bt_info->gpio_rst, "bt_gpio_rst");
	if (ret) {
		dev_err(&pdev->dev,	"couldn't request bt_gpio_rst gpio\n");
		goto free_bt_enable;
	}

	ret	= gpio_direction_output(bt_info->gpio_rst, 0);
	if (ret) {
		dev_err(&pdev->dev,	"couldn't gpio_direction_output	rst	gpio\n");
		goto free_bt_reset;
	}
#ifndef CONFIG_MACH_K3V2OEM1
	res	= platform_get_resource_byname(pdev, IORESOURCE_IO,
				"bt_vdd_enable");
	if (!res) {
		dev_err(&pdev->dev, "couldn't find bt_vdd_enable gpio\n");
		ret	= -ENODEV;
		goto free_bt_reset;
	}
	bt_info->gpio_bt_vdd = res->start;

	ret	= gpio_request(bt_info->gpio_bt_vdd, "bt_vdd_enable");
	if (ret) {
		dev_err(&pdev->dev, "couldn't request bt_vdd_enable	gpio\n");
		} else {
		ret	= gpio_direction_output(bt_info->gpio_bt_vdd, 1);
		if (ret) {
			dev_err(&pdev->dev, "couldn't gpio_direction_output fpga bt vdd gpio\n");
			goto free_bt_gpio_vdd;
		}
	}
#else

	/* get bt32kb clock*/
	bt_info->bt_clk	= clk_get(NULL,	"clk_pmu32kb");
	if (IS_ERR(bt_info->bt_clk)) {
		dev_err(&pdev->dev,	"%s	: Could	not	get	clk_pmu32kb\n",
			__func__);
		ret	= -ENOMEM;
		goto free_bt_reset;
	}

	/* get bluetooth LDO14 */
/*	bt_info->regulator = regulator_get(&pdev->dev, "bt-io-vcc");
	if (IS_ERR(bt_info->regulator))	{
		dev_err(&pdev->dev,	"%s: Could not get regulator : bt-io-vcc\n",
				__func__);
		ret	= -ENOMEM;
		goto free_bt_clk;
	}
*/
	/* set bluetooth vddio voltage to 1.8V */
/*	ret	= regulator_set_voltage(bt_info->regulator,
			BT_VDDIO_LDO_18V, BT_VDDIO_LDO_18V);
	if (ret) {
		dev_err(&pdev->dev,	"%s: regulator_set_voltage err %d\n",
			__func__, ret);
		goto free_bt_regulator;
	}
*/
#endif

	platform_set_drvdata(pdev, bt_info);
	ret	= bluetooth_power_rfkill_probe(pdev, bt_info);
	if (ret) {
		dev_err(&pdev->dev,	"bluetooth_power_rfkill_probe failed, ret:%d\n",
			ret);
		goto free_bt_res;
	}
	return ret;

free_bt_res:
	platform_set_drvdata(pdev, NULL);
#ifdef CONFIG_MACH_K3V2OEM1
free_bt_regulator:
	regulator_put(bt_info->regulator);
free_bt_clk:
	clk_put(bt_info->bt_clk);
#else
free_bt_gpio_vdd:
	gpio_free(bt_info->gpio_bt_vdd);
#endif
free_bt_reset:
	gpio_free(bt_info->gpio_rst);
free_bt_enable:
	gpio_free(bt_info->gpio_enable);
free_bpi:
	kfree(bt_info);
	bt_info	= NULL;

	return ret;
}

static int __devexit bt_power_remove(struct	platform_device	*pdev)
{
	struct bluepower_info *bt_info = platform_get_drvdata(pdev);

	if (NULL == bt_info) {
		dev_err(&pdev->dev,	"%s, bt_info is null \n", __func__);
		return -1;
	}

	dev_dbg(&pdev->dev,	"%s\n",	__func__);

	bluetooth_power_handle(bt_info, BT_SHUTDOWN);

	gpio_free(bt_info->gpio_enable);
	gpio_free(bt_info->gpio_rst);

#ifdef CONFIG_MACH_K3V2OEM1
	if (!IS_ERR((bt_info->regulator)))
		regulator_put(bt_info->regulator);
	if (!IS_ERR(bt_info->bt_clk))
		clk_put(bt_info->bt_clk);
#else
	gpio_free(bt_info->gpio_bt_vdd);
#endif

	bluetooth_power_rfkill_remove(pdev);

	kfree(bt_info);
	bt_info	= NULL;

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static void	bt_power_shutdown(struct platform_device *pdev)
{
	struct bluepower_info *bt_info = platform_get_drvdata(pdev);

	if (NULL == bt_info) {
		dev_err(&pdev->dev,	"%s, bt_info is null \n", __func__);
		return;
	}

	printk("[%s],+\n", __func__);

	dev_dbg(&pdev->dev,	"%s\n",	__func__);

	bluetooth_power_handle(bt_info, BT_SHUTDOWN);

	printk("[%s],-\n", __func__);

	return;
}

static struct platform_driver bt_power_driver =	{
	.probe	   = bt_power_probe,
	.remove	   = __devexit_p(bt_power_remove),
	.shutdown  = bt_power_shutdown,
	.driver	   = {
	.name  = "bt_power",
	.owner = THIS_MODULE,
	},
};

static int __init bluetooth_power_init(void)
{
	int	ret;
	ret	= platform_driver_register(&bt_power_driver);
	return ret;
}

static void	__exit bluetooth_power_exit(void)
{
	platform_driver_unregister(&bt_power_driver);
}

MODULE_LICENSE("GPL	v2");
MODULE_DESCRIPTION("hisi Bluetooth power control driver");
MODULE_VERSION("1.40");

module_init(bluetooth_power_init);
module_exit(bluetooth_power_exit);
