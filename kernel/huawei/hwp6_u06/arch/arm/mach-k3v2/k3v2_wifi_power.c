/*
 * Copyright (C) 2009 Texas	Instruments
 *
 * Author: Pradeep Gurumath	<pradeepgurumath@ti.com>
 * This	program	is free	software; you can redistribute it and/or modify
 * it under	the	terms of the GNU General Public	License	version	2 as
 * published by	the	Free Software Foundation.
 */

/* linux/arch/arm/mach-k3v2/k3v2_wifi_power.c
 */

/*=========================================================================
 *
 * histoty
 *
 *=========================================================================
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>
#include <linux/random.h>
#include <linux/skbuff.h>
#include <generated/mach-types.h>
#include <linux/wifi_tiwlan.h>
#include <asm/mach-types.h>
#include <linux/mux.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/clk.h>
#include <linux/mtd/nve_interface.h>
#include <linux/ctype.h>
#include <linux/wl12xx.h>


#include "k3v2_wifi_power.h"

int k3v2_wifi_power(bool power_on);

static struct wl12xx_platform_data k3v2_wlan_data __initdata = {
	.irq = 0,
	.board_ref_clock = WL12XX_REFCLOCK_26,
	.board_tcxo_clock = WL12XX_TCXOCLOCK_26,
	//z00186406 add to enable outband irq begin
	.platform_quirks  = WL12XX_PLATFORM_QUIRK_EDGE_IRQ,
	//z00186406 add to enable outband irq end
};

struct wifi_host_s {
        struct regulator *vdd;
        struct clk *clk;
        struct iomux_block *block;
        struct block_config *config;
};

struct wifi_host_s *wifi_host;

//#define  K3V2_WIFI_PM_ERR_FLAG_CLEAN_DEBUG

#ifdef K3V2_WIFI_PM_ERR_FLAG_CLEAN_DEBUG
#define wifi_db_printk(format, arg...)   printk(KERN_INFO "WiFi DEBUG: "format, ##arg)
#else
#define wifi_db_printk(format, arg...)
#endif
/*Add for clean wifi card device's runtime pm error flag. */
static struct device * wifi_sdio_card_dev_ptr = NULL;

/**************************************************************
Description      : Called to set the dev pointer of WiFi SDIO card , in 
				the WiFi driver probe function.
Prototype        : int k3v2_set_wifi_card_dev_ptr(struct device *  dev_ptr)
Input  Param    :
Output  Param  :
Return  Value   :
***************************************************************/
int k3v2_set_wifi_card_dev_ptr(struct device *  dev_ptr){
	wifi_db_printk("k3v2_set_wifi_card_dev_ptr enter.\n");
	if(NULL == dev_ptr){
		wifi_db_printk("k3v2_set_wifi_card_dev_ptr NULL point error.\n");
		return -1;
	}

	wifi_sdio_card_dev_ptr = dev_ptr;

	return 0;
}
EXPORT_SYMBOL(k3v2_set_wifi_card_dev_ptr);

/**************************************************************
Description      : If wifi card dev's power.runtime_error flag is not 0, it will block the wifi 
				card to power on. Need clean it before wifi start. We think the wifi's first
				start after the phone power up is always ok. and the program will record the 
				card dev pointer in the driver probe function at the first wifi start up process.
				
Prototype        : void k3v2_clean_card_runtime_error_flag(void)
Input  Param    :
Output  Param  :
Return  Value   :
***************************************************************/
void k3v2_clean_card_pm_error_flag(void){
	wifi_db_printk("k3v2_clean_card_runtime_error_flag enter.\n");	
	if(NULL == wifi_sdio_card_dev_ptr){
		wifi_db_printk("wifi card dev ptr is not set now.\n");
		return;
	}
	
	if(wifi_sdio_card_dev_ptr->power.runtime_error){
		printk(KERN_ERR "WiFi card's runtime_error flag: %d, serious error happen, clean it now! \n", 
				wifi_sdio_card_dev_ptr->power.runtime_error);
		wifi_sdio_card_dev_ptr->power.runtime_error = 0;
	}else{
		wifi_db_printk(KERN_ERR "WiFi card's runtime_error flag is 0, normal.\n");	
	}
}
EXPORT_SYMBOL(k3v2_clean_card_pm_error_flag);
static int __init k3v2_wifi_init(void)
{
	int ret = 0;

	printk("TI WL18XX: %s \n", __func__);

        wifi_host = kzalloc(sizeof(struct wifi_host_s), GFP_KERNEL);
        wifi_host->clk = clk_get(NULL, "clk_pmu32kb");
        //delete the regulator LDO14 that wifi not used
        wifi_host->block = iomux_get_block("block_wifi");
        wifi_host->config = iomux_get_blockconfig("block_wifi");
        blockmux_set(wifi_host->block, wifi_host->config, LOWPOWER);

	/* set power gpio */
	ret = gpio_request(K3V2_WIFI_POWER_GPIO, NULL);
	if (ret < 0) {
		pr_err("%s: gpio_request failed, ret:%d.\n", __func__,
			K3V2_WIFI_POWER_GPIO);
		goto err_power_gpio_request;
	}
	gpio_direction_output(K3V2_WIFI_POWER_GPIO, 0);

	//z00186406 add to request out band irq, and convert gpio number to irq number begin
	ret = gpio_request(K3V2_WIFI_IRQ_GPIO, NULL);
	if(ret < 0) {
		pr_err( "%s: gpio_request failed, ret:%d.\n", __func__,
			K3V2_WIFI_IRQ_GPIO );
		goto err_irq_gpio_request;
	}
	gpio_direction_input(K3V2_WIFI_IRQ_GPIO);
	k3v2_wlan_data.irq = gpio_to_irq(K3V2_WIFI_IRQ_GPIO);
	//z00186406 add to request out band irq, and convert gpio number to irq number end

	wl12xx_set_platform_data(&k3v2_wlan_data);

	return 0;

//z00186406 add enable out band irq begin
err_irq_gpio_request:
       gpio_free(K3V2_WIFI_IRQ_GPIO);
//z00186406 add enable out band irq end

err_power_gpio_request:
	gpio_free(K3V2_WIFI_POWER_GPIO);
	return ret;
}
device_initcall(k3v2_wifi_init);

int k3v2_wifi_power(bool power_on)
{
        printk("TI WL18XX: Powering %s WiFi\n", (power_on ? "on" : "off"));

        if (power_on) {
                clk_enable(wifi_host->clk);
                gpio_set_value(K3V2_WIFI_POWER_GPIO, 1);
                mdelay(15);
                gpio_set_value(K3V2_WIFI_POWER_GPIO, 0);
                mdelay(3);
                gpio_set_value(K3V2_WIFI_POWER_GPIO, 1);
                mdelay(70);
        } else {
                gpio_set_value(K3V2_WIFI_POWER_GPIO, 0);
                clk_disable(wifi_host->clk);
        }

        return 0;
}
EXPORT_SYMBOL(k3v2_wifi_power);
