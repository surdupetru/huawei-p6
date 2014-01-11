/*
 * himci.c - hisilicon MMC Host driver
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mshci.h>
#include <linux/gpio.h>

#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/switch.h>

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/freezer.h>
#include <asm/dma.h>
#include <linux/io.h>
#include <mach/irqs.h>
#include <mach/mmc.h>
#include <linux/clk.h>
#include <asm/sizes.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <mach/hardware.h>
#include <linux/mux.h>
#include <mach/boardid.h>

#include <mach/early-debug.h>
#include <mach/gpio.h>
#include <linux/regulator/consumer.h>

#include "himci.h"
#include <linux/mux.h>
#include <hsad/config_interface.h>



/* parameters */
int trace_level = HIMCI_TRACE_LEVEL; /* 5 - the highest trace level */

#define SD_CLK_PIN "gpio_088"

#ifdef SD_FPGA_GPIO
#define GPIO_CD_DATA(__X__)	(*((unsigned volatile*)(__X__ + 0x3FC)))
#define GPIO_CD_DIR(__X__)	(*((unsigned volatile*)(__X__ + 0x400)))
#define GPIO_CD_IS(__X__)	(*((unsigned volatile*)(__X__ + 0x404)))
#define GPIO_CD_IBE(__X__)	(*((unsigned volatile*)(__X__ + 0x408)))
#define GPIO_CD_IE(__X__)	(*((unsigned volatile*)(__X__ + 0x410)))
#define GPIO_CD_RIS(__X__)	(*((unsigned volatile*)(__X__ + 0x414)))
#define GPIO_CD_IC(__X__)	(*((unsigned volatile*)(__X__ + 0x41C)))
#endif

#ifdef MODULE
module_param(trace_level, int, 0600);
MODULE_PARM_DESC(trace_level, "HIMCI_TRACE_LEVEL");
#endif

static spinlock_t mmc_tuning_lock;

#if 0
/* IOMG addr for SD */
#define    IOMG_047	   (IO_ADDRESS(REG_BASE_IOC) + 0x0BC)
#define    IOMG_048	   (IO_ADDRESS(REG_BASE_IOC) + 0x0C0)

/* IOCG addr for SD */
#define    IOCG_107	   (IO_ADDRESS(REG_BASE_IOC) + 0x98C)
#define    IOCG_108	   (IO_ADDRESS(REG_BASE_IOC) + 0x990)
#define    IOCG_109	   (IO_ADDRESS(REG_BASE_IOC) + 0x994)
#define    IOCG_110	   (IO_ADDRESS(REG_BASE_IOC) + 0x998)
#define    IOCG_111	   (IO_ADDRESS(REG_BASE_IOC) + 0x99C)
#define    IOCG_112	   (IO_ADDRESS(REG_BASE_IOC) + 0x9A0)

void dump_hw_info()
{
	printk(KERN_ERR ": ===========================================\n");

	printk(KERN_ERR " IOMG_047: 0 == %d\n", readl(IOMG_047));
	printk(KERN_ERR " IOMG_048: 0 == %d\n", readl(IOMG_048));

	printk(KERN_ERR " IOCG_107: 0x30 == 0x%x\n", readl(IOCG_107));
	printk(KERN_ERR " IOCG_108: 0x31 == 0x%x\n", readl(IOCG_108));
	printk(KERN_ERR " IOCG_109: 0x31 == 0x%x\n", readl(IOCG_109));
	printk(KERN_ERR " IOCG_110: 0x31 == 0x%x\n", readl(IOCG_110));
	printk(KERN_ERR " IOCG_111: 0x31 == 0x%x\n", readl(IOCG_111));
	printk(KERN_ERR " IOCG_112: 0x31 == 0x%x\n", readl(IOCG_112));

	printk(KERN_ERR ": -------------------------------------------\n");
}
#endif

static void mshci_hi_set_timing(struct himci_host *hi_host, int sam, int drv, int div)
{
	unsigned int clken_reg;
	unsigned int clken_bit;
	unsigned int sam_sel_reg;
	unsigned int sam_sel_bit;
	unsigned int drv_sel_reg;
	unsigned int drv_sel_bit;
	unsigned int div_reg;
	unsigned int div_bit;
	int i = 0;
	unsigned int temp_reg;
	unsigned long flags;

	switch (hi_host->pdev->id) {
		case 0:
			clken_reg = 0x1F8;
			clken_bit = 0;
			drv_sel_reg = 0x1F8;
			drv_sel_bit = 4;
			sam_sel_reg = 0x1F8;
			sam_sel_bit = 8;
			div_reg = 0x1F8;
			div_bit = 1;
			break;
		case 1:
			clken_reg = 0x1F8;
			clken_bit = 12;
			drv_sel_reg = 0x1F8;
			drv_sel_bit = 16;
			sam_sel_reg = 0x1F8;
			sam_sel_bit = 20;
			div_reg = 0x1F8;
			div_bit = 13;
			break;
		case 2:
			clken_reg = 0x1F8;
			clken_bit = 24;
			drv_sel_reg = 0x1F8;
			drv_sel_bit = 28;
			sam_sel_reg = 0x1FC;
			sam_sel_bit = 0;
			div_reg = 0x1F8;
			div_bit = 25;
			break;
		case 3:
			clken_reg = 0x1FC;
			clken_bit = 4;
			drv_sel_reg = 0x1FC;
			drv_sel_bit = 8;
			sam_sel_reg = 0x1FC;
			sam_sel_bit = 12;
			div_reg = 0x1FC;
			div_bit = 5;
			break;
		default:
			return;
	}


	spin_lock_irqsave(&mmc_tuning_lock,flags);

	/* disable clock */
	temp_reg = readl(IO_ADDRESS(REG_BASE_PCTRL + clken_reg));
	temp_reg &= ~(1<<clken_bit);
	writel(temp_reg, IO_ADDRESS(REG_BASE_PCTRL + clken_reg));

	temp_reg = readl(IO_ADDRESS(REG_BASE_PCTRL + sam_sel_reg));
	if (sam >= 0) {
		/* set sam delay */
		for(i = 0; i < 4; i++){
			if( sam % 2 ){
				temp_reg |= 1<<(sam_sel_bit + i);
			} else {
				temp_reg &= ~(1<<(sam_sel_bit + i));
			}
			sam = sam >> 1;
		}
	}
	writel(temp_reg, IO_ADDRESS(REG_BASE_PCTRL + sam_sel_reg));

	temp_reg = readl(IO_ADDRESS(REG_BASE_PCTRL + drv_sel_reg));
	if (drv >= 0) {
		/* set drv delay */
		for(i = 0; i < 4; i++){
			if( drv % 2 ){
				temp_reg |= 1<<(drv_sel_bit + i);
			} else {
				temp_reg &= ~(1<<(drv_sel_bit + i));
			}
			drv = drv >> 1;
		}
	}
	writel(temp_reg, IO_ADDRESS(REG_BASE_PCTRL + drv_sel_reg));

	temp_reg = readl(IO_ADDRESS(REG_BASE_PCTRL + drv_sel_reg));
	if (div >= 0) {
		/* set drv delay */
		for(i = 0; i < 3; i++){
			if( div % 2 ){
				temp_reg |= 1<<(div_bit + i);
			} else {
				temp_reg &= ~(1<<(div_bit + i));
			}
			div = div >> 1;
		}
	}
	writel(temp_reg, IO_ADDRESS(REG_BASE_PCTRL + drv_sel_reg));
	/* enable clock */
	temp_reg = readl(IO_ADDRESS(REG_BASE_PCTRL + clken_reg));
	temp_reg |= 1<<clken_bit;
	writel(temp_reg, IO_ADDRESS(REG_BASE_PCTRL + clken_reg));

	spin_unlock_irqrestore(&mmc_tuning_lock,flags);

}


#include <linux/mmc/mn66831hub_api.h>
#define		SDHUB_DEVICE_ID		(SDHUB_DEVID0)

static irqreturn_t mshci_hi_card_detect_gpio(int irq, void *data)
{
	struct mshci_host *ms_host = (struct mshci_host *)data;
	struct himci_host *hi_host = (struct himci_host *)(ms_host->private);
    int ret;

#ifdef SD_FPGA_GPIO
	hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "g00175134: CD Interrupt 0x%x\n",
					GPIO_CD_RIS(hi_host->gpioaddr));
	GPIO_CD_IC(hi_host->gpioaddr) = 0x1;
#endif


    hi_host->ocp_flag = 0;
#ifdef CONFIG_MMC_SD_HUB
    if (ms_host->ops->get_present_status) {
        ret = ms_host->ops->get_present_status(ms_host);  // ret=1 卡不在位；ret=0卡在位
    	if (!ret) {
    		SDHUB_Connect_hotplug( ms_host->mmc, SDHUB_DEVICE_ID );
        } else {
    		SDHUB_Disconnect( ms_host->mmc, SDHUB_DEVICE_ID );
        }        
    }
    printk("CPRM : %s : %s : ret = %d\n", mmc_hostname(ms_host->mmc), __func__, ret);
#else
    tasklet_schedule(&ms_host->card_tasklet);
#endif /* CONFIG_MMC_SD_HUB */

	return IRQ_HANDLED;
};

unsigned int mshci_hi_get_present_status(struct mshci_host *ms_host)
{
	struct himci_host * hi_host = (struct himci_host *)(ms_host->private);
	unsigned int status;
	if (ms_host->quirks & MSHCI_QUIRK_WLAN_DETECTION) {
		if (ms_host->flags & MSHCI_DEVICE_DEAD)
			return 1;
		else
			return 0;
	} else if (ms_host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
#ifdef SD_FPGA_GPIO
		status = GPIO_CD_DATA(hi_host->gpioaddr);
		hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "g00175134: gpio status = %d \n",
						status);
#else
		if (get_sd_detect_type() == GPIO_LOW_MEAN_DETECTED)
		{
			/* GPIO Low mean SD Card insert; GPIO High means no SD card */
			status = gpio_get_value(hi_host->plat->cd_gpio);
		} else if (get_sd_detect_type() == GPIO_HIGH_MEAN_DETECTED){
			/* GPIO High mean SD Card insert; GPIO Low means no SD card */
			status = !gpio_get_value(hi_host->plat->cd_gpio);
		} else {
			himci_error("get_sd_detect_type failed");
			/* GPIO High mean SD Card insert; GPIO Low means no SD card */
			status = !gpio_get_value(hi_host->plat->cd_gpio);
		}
#endif
		status |= hi_host->ocp_flag;
		return status;
	} else {
		return 0;
	}
}

/* set host timing config */
static void mshci_hi_update_timing(struct mshci_host *ms_host, int bsuspend)
{
	struct himci_host * hi_host = (struct himci_host *)(ms_host->private);
	unsigned int config_val;

	if ( get_chipid() == DI_CHIP_ID ) {
		if (bsuspend) {
			config_val = hi_host->suspend_timing_config;
		} else {
			hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "current signal voltage = %d",
							hi_host->old_sig_voltage);
			hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "current timing = %d",
							hi_host->old_timing);
			config_val = hi_host->timing_config[hi_host->old_timing + \
							hi_host->old_sig_voltage * (MMC_TIMING_UHS_DDR50 + 1)];
		}

		hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "current timing config = 0x%x",
						config_val);

		if (-1 == config_val) {
			himci_error("invalid config_val");
		}

		writel(0xF << (hi_host->pdev->id * 4),
					IO_ADDRESS(REG_BASE_SCTRL) + REG_SCPERDIS4);
		writel(config_val << (hi_host->pdev->id * 4),
					IO_ADDRESS(REG_BASE_SCTRL) + REG_SCPEREN4);

		hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "new reg val 0x%x",
					readl(IO_ADDRESS(REG_BASE_SCTRL) + REG_SCPERSTAT4));
	}
}

extern int k3v2_wifi_power(int on);

void mshci_hi_set_ios(struct mshci_host *ms_host, struct mmc_ios *ios)
{
	struct himci_host * hi_host = (struct himci_host *)(ms_host->private);
	int ret = -1;

	hi_host_trace(HIMCI_TRACE_GEN_API, "++");

	himci_assert(ios);
	himci_assert(hi_host);

	hi_host_trace(HIMCI_TRACE_GEN_INFO, "ios->power_mode = %d ", ios->power_mode);
	hi_host_trace(HIMCI_TRACE_GEN_INFO, "ios->clock = %d ", ios->clock);
	hi_host_trace(HIMCI_TRACE_GEN_INFO, "ios->bus_width = %d ", ios->bus_width);
	hi_host_trace(HIMCI_TRACE_GEN_INFO, "ios->timing = %d ", ios->timing);

	/* process power */
	if (hi_host->old_power_mode != ios->power_mode) {
		switch (ios->power_mode) {
		case MMC_POWER_OFF:

		    hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "set io to lowpower");

		    ret = blockmux_set(hi_host->piomux_block, hi_host->pblock_config, LOWPOWER);
		    if (ret) {
				himci_error("failed to blockmux_set");
		    }

			if (hi_host->signal_vcc) {
				regulator_set_mode(hi_host->signal_vcc, REGULATOR_MODE_IDLE);
		    }

#ifdef CONFIG_MMC_SD_HUB
		    if( !(ms_host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) ) {
		        if (hi_host->vcc) {
			   	    regulator_disable(hi_host->vcc);
		        }
		    } else {
		        printk("CPRM : mshci_hi_set_ios() : MMC_POWER_OFF\n");
		    }
#else
		    if (hi_host->vcc) {
		        regulator_disable(hi_host->vcc);
		    }
#endif

		    //z00186406 add to power on wifi begin
                    if (ms_host->quirks & MSHCI_QUIRK_TI_WL18XX) {
			k3v2_wifi_power(false);
		    }
                    //z00186406 add to power on wifi end
			break;
		case MMC_POWER_UP:
		    hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "set io to normal");
                    
                    //z00186406 add to power off wifi begin
                    if (ms_host->quirks & MSHCI_QUIRK_TI_WL18XX) {
			k3v2_wifi_power(true);
		    }
                    //z00186406 add to power off wifi end

#ifdef CONFIG_MMC_SD_HUB
            if( !(ms_host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) ) {                
    		    if (hi_host->vcc) {
    				ret = regulator_set_voltage(hi_host->vcc, 2850000, 2850000);
    				if (ret != 0) {
    					himci_error("failed to regulator_set_voltage");
    				}

    				ret = regulator_enable(hi_host->vcc);
    				if (ret) {
    					himci_error("failed to regulator_enable");
    				}
    		    }
            } else {
                printk("CPRM : mshci_hi_set_ios() : MMC_POWER_UP\n");
            }
#else
                 if (hi_host->vcc) {
                     ret = regulator_set_voltage(hi_host->vcc, 2850000, 2850000);
                     if (ret != 0) {
                           himci_error("failed to regulator_set_voltage");
                     }
                     ret = regulator_enable(hi_host->vcc);
                     if (ret) {
                           himci_error("failed to regulator_enable");
                     }
                 }
#endif
			if (hi_host->signal_vcc) {
				ret = regulator_set_voltage(hi_host->signal_vcc, 2600000, 2600000);
				if (ret != 0) {
					himci_error("failed to regulator_set_voltage");
				}
				regulator_set_mode(hi_host->signal_vcc, REGULATOR_MODE_NORMAL);
		    }

		    ret = blockmux_set(hi_host->piomux_block, hi_host->pblock_config, NORMAL);
		    if (ret) {
				himci_error("failed to blockmux_set");
		    }

			break;
		case MMC_POWER_ON:
			break;
		default:
			himci_error("unknown power supply mode");
			break;
		}
		hi_host->old_power_mode = ios->power_mode;
	}

	/* process timing */
	if (hi_host->old_timing != ios->timing) {

		hi_host->old_timing = ios->timing;

		if ( get_chipid() == DI_CHIP_ID ) {
			mshci_hi_update_timing(ms_host, 0);

			switch (ios->timing) {
			case MMC_TIMING_LEGACY:
				if (hi_host->pdev->id == 1) {
					/* 2 division, 40M */
					writel((0x1<<6) | (0x7<<22),
						IO_ADDRESS(REG_BASE_SCTRL) + REG_SCCLKDIV2);
					ms_host->max_clk = 40*1000*1000;
					ms_host->clock++;
				} else if (hi_host->pdev->id == 0) {
					/* 2 division, 40M */
					writel((0x0<<5) | (0x1<<21),
						IO_ADDRESS(REG_BASE_SCTRL) + REG_SCCLKDIV2);
					ms_host->max_clk = 40*1000*1000;
					ms_host->clock++;
				}
				hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "MMC_TIMING_LEGACY");
				break;
			case MMC_TIMING_UHS_DDR50:
				if (hi_host->pdev->id == 1) {
					/* 1 division, 80M */
					writel((0x0<<6) | (0x7<<22),
						IO_ADDRESS(REG_BASE_SCTRL) + REG_SCCLKDIV2);
					ms_host->max_clk = 80*1000*1000;
					ms_host->clock++;
				} else {
#if 0
					/*
					 * m53980:
					 * debug purpose.
					 * change clock via sctrl configuration
					 */
					printk("clk div a:0x%x\n", readl(IO_ADDRESS(REG_BASE_SCTRL) + REG_SCCLKDIV2));
					writel((0x7)|(0xF<<16), IO_ADDRESS(REG_BASE_SCTRL) + REG_SCCLKDIV2);
					printk("clk div b:0x%x\n", readl(IO_ADDRESS(REG_BASE_SCTRL) + REG_SCCLKDIV2));
#endif
				}
				hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "MMC_TIMING_UHS_DDR50");
				break;
			case MMC_TIMING_UHS_SDR50:
				if (hi_host->pdev->id == 0) {
					writel((0x1<<5) | (0x1<<21),
						IO_ADDRESS(REG_BASE_SCTRL) + REG_SCCLKDIV2);
					ms_host->max_clk = 80*1000*1000;
					ms_host->clock++;
				}
				hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "MMC_TIMING_UHS_SDR50");
				break;
			default:
				break;
			}
		} else {
			ret = clk_set_rate(hi_host->pclk,hi_host->init_tuning_config[0 + (ios->timing + 1) * TUNING_INIT_CONFIG_NUM]);
			if (ret) {
				himci_error("failed to clk_set_rate");
			}
			hi_host->tuning_init_sample =
					(hi_host->init_tuning_config[3 + (ios->timing + 1) * TUNING_INIT_CONFIG_NUM] +
					hi_host->init_tuning_config[4 + (ios->timing + 1) * TUNING_INIT_CONFIG_NUM]) / 2;
			mshci_hi_set_timing(hi_host,
				hi_host->tuning_init_sample,
				hi_host->init_tuning_config[2 + (ios->timing + 1) * TUNING_INIT_CONFIG_NUM],
				hi_host->init_tuning_config[1 + (ios->timing + 1) * TUNING_INIT_CONFIG_NUM]);

			ms_host->max_clk = hi_host->init_tuning_config[5 + (ios->timing + 1) * TUNING_INIT_CONFIG_NUM];

			ms_host->clock++;
		}
	}

	hi_host_trace(HIMCI_TRACE_GEN_API, "--");
}

static unsigned int mshci_hi_get_max_clk(struct mshci_host *ms_host)
{
	if ( get_chipid() == CS_CHIP_ID )
		return MMC_CCLK_MAX_50M;
	else
		return MMC_CCLK_MAX;
}

static int mshci_hi_start_signal_voltage_switch(struct mshci_host *ms_host,
				struct mmc_ios *ios)
{
	struct himci_host *hi_host = (struct himci_host *)(ms_host->private);
	int ret =0;
	struct iomux_pin *pin_temp = NULL;

	hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "++");

	himci_assert(ios);
	himci_assert(hi_host);

	hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "old_sig_voltage = %d ",
					hi_host->old_sig_voltage);
	hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "new_sig_voltage = %d ",
					ios->signal_voltage);
	hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "allow_switch_signal_voltage = %d ",
					hi_host->allow_switch_signal_voltage);

	if (hi_host->allow_switch_signal_voltage &&
		(hi_host->old_sig_voltage != ios->signal_voltage)) {
		switch (ios->signal_voltage) {
		case MMC_SIGNAL_VOLTAGE_330:
			/* alter driver 6mA */
			hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "alter driver 6mA");
			pin_temp = iomux_get_pin(SD_CLK_PIN);
			if (pin_temp) {
				ret = pinmux_setdrivestrength(pin_temp, LEVEL2);
				if (ret < 0)
					himci_error("pinmux_setdrivestrength error");
			} else {
				himci_error("failed to get iomux pin");
			}

			hi_host->old_sig_voltage = ios->signal_voltage;
			/* if there is signal vcc, set vcc to signal voltage */
		    if (hi_host->signal_vcc) {
				ret = regulator_set_voltage(hi_host->signal_vcc, 2600000, 2600000);
				if (ret != 0) {
					himci_error("failed to regulator_set_voltage");
				}
		    }

			/* update time config*/
			mshci_hi_update_timing(ms_host, 0);
			break;
		case MMC_SIGNAL_VOLTAGE_180:
			/* alter driver 8mA */
			hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "alter driver 8mA");
			pin_temp = iomux_get_pin(SD_CLK_PIN);
			if (pin_temp) {
#ifdef CONFIG_MMC_SD_HUB
				ret = pinmux_setdrivestrength(pin_temp, LEVEL0);
#else
                ret = pinmux_setdrivestrength(pin_temp, LEVEL3);
#endif
				if (ret < 0)
					himci_error("pinmux_setdrivestrength error");
			} else {
				himci_error("failed to get iomux pin");
			}

			hi_host->old_sig_voltage = ios->signal_voltage;
			/* if there is signal vcc, set vcc to signal voltage */
		    if (hi_host->signal_vcc) {
				ret = regulator_set_voltage(hi_host->signal_vcc, 1800000, 1800000);
				if (ret != 0) {
					himci_error("failed to regulator_set_voltage");
				}
		    }
			/* update time config*/
			mshci_hi_update_timing(ms_host, 0);
			break;

		case MMC_SIGNAL_VOLTAGE_120:
			/* FIXME */
			/* 1.20v is not support */
			himci_error("1.20V is not supported");
			break;
		default:
			himci_error("unknown signal voltage");
			break;
		}
	}

	hi_host_trace(HIMCI_TRACE_SIGNIFICANT, "--");

	return 0;
}

static void mshci_hi_tuning_clear_flags(struct himci_host *hi_host)
{
	hi_host->tuning_sample_flag = 0;
}

static void mshci_hi_tuning_set_flags(struct himci_host *hi_host, int sample, int ok)
{
	if (ok)
		hi_host->tuning_sample_flag |= (1 << sample);
	else
		hi_host->tuning_sample_flag &= ~(1 << sample);
}

static int mshci_hi_tuning_get_flags(struct himci_host *hi_host, int sample)
{
	if (hi_host->tuning_sample_flag & (1 << sample))
		return 1;
	else
		return 0;
}

/* By tuning, find the best timing condition */
/* return:	1 -- tuning is not finished. And this function should be called again */
/*			0 -- Tuning successfully. If this function be called again, another round of tuning would be start */
/*			-1 -- Tuning failed. Maybe slow down the clock and call this function again */
static int mshci_hi_tuning_find_condition(struct mshci_host *ms_host)
{
	struct himci_host *hi_host;
	int sample_min, sample_max;
	/*int begin, end;*/
	int i, j;
	int ret = 0;
	int mask,mask_lenth;

	hi_host = mshci_priv(ms_host);

	if (hi_host->init_tuning_config[4 + (ms_host->mmc->ios.timing + 1) * TUNING_INIT_CONFIG_NUM] ==
		hi_host->init_tuning_config[3 + (ms_host->mmc->ios.timing + 1) * TUNING_INIT_CONFIG_NUM]) {
		hi_host->tuning_init_sample =
			(hi_host->init_tuning_config[4 + (ms_host->mmc->ios.timing + 1) * TUNING_INIT_CONFIG_NUM] +
			hi_host->init_tuning_config[3 + (ms_host->mmc->ios.timing + 1) * TUNING_INIT_CONFIG_NUM]) / 2;
		mshci_hi_set_timing(hi_host, hi_host->tuning_init_sample, -1, -1);
		hi_host_trace(HIMCI_TRACE_SIGNIFICANT,
			"no need tuning: timing is %d, tuning sample = %d",
			ms_host->mmc->ios.timing, hi_host->tuning_init_sample);
		return 0;
	}
	if (-1 == hi_host->tuning_current_sample) {

		mshci_hi_tuning_clear_flags(hi_host);

		/* set the first sam del as the min_sam_del */
		hi_host->tuning_current_sample =
			hi_host->init_tuning_config[4 + (ms_host->mmc->ios.timing + 1) * TUNING_INIT_CONFIG_NUM];
		/* a trick for next "++" */
		hi_host->tuning_current_sample--;
	}

	if (hi_host->tuning_current_sample >=
			hi_host->init_tuning_config[3 + (ms_host->mmc->ios.timing + 1) * TUNING_INIT_CONFIG_NUM]) {
		/* tuning finish, select the best sam_del */

		/* set sam del to -1, for next tuning */
		hi_host->tuning_current_sample = -1;

		sample_min =
			hi_host->init_tuning_config[4 + (ms_host->mmc->ios.timing + 1) * TUNING_INIT_CONFIG_NUM];
		sample_max =
			hi_host->init_tuning_config[3 + (ms_host->mmc->ios.timing + 1) * TUNING_INIT_CONFIG_NUM];

#if 0
		begin = -1;
		end = -1;
		for (i = sample_min; i <= sample_max; i++) {
			if (-1 == begin) {
				/* no begin, find begin */
				if (mshci_hi_tuning_get_flags(hi_host, i)) {
					begin = i;
				} else {
					continue;
				}
			} else if (-1 == end) {
				/* no end, find end */
				if (mshci_hi_tuning_get_flags(hi_host, i)) {
					if (i == sample_max) {
						end = sample_max;
					} else {
						continue;
					}
				} else {
					end = i - 1;
				}
			} else {
				/* has begin & end, break */
				break;
			}
		}

		if (-1 == begin) {
			hi_host->tuning_init_sample = (sample_min + sample_max) / 2;
			hi_host_trace(HIMCI_TRACE_SIGNIFICANT,
				"tuning err: no good sam_del, timing is %d, tuning_flag = 0x%x",
				ms_host->mmc->ios.timing, hi_host->tuning_sample_flag);
			ret = -1;
		} else if (-1 == end) {
			hi_host->tuning_init_sample = (begin + sample_max) / 2;
			hi_host_trace(HIMCI_TRACE_SIGNIFICANT,
					"tuning err: no end! arithmetic error, timing is %d, begin = %d; tuning_flag = 0x%x",
					ms_host->mmc->ios.timing, begin, hi_host->tuning_sample_flag);
			ret = -1;
		} else {
			hi_host->tuning_init_sample = (begin + end) / 2;
			hi_host_trace(HIMCI_TRACE_SIGNIFICANT,
				"tuning OK: timing is %d, tuning sample = %d, tuning_flag = 0x%x",
				ms_host->mmc->ios.timing, hi_host->tuning_init_sample, hi_host->tuning_sample_flag);
			ret = 0;
		}
#else
		hi_host->tuning_init_sample = -1;
		for (mask_lenth = (((sample_max - sample_min) >> 1) << 1) + 1;
			mask_lenth >= 1; mask_lenth -= 2) {

				mask = (1 << mask_lenth) - 1;
				for (i = (sample_min + sample_max - mask_lenth + 1) / 2, j = 1;
					(i <= sample_max - mask_lenth + 1) && (i >= sample_min);
					i = ((sample_min + sample_max - mask_lenth + 1) / 2) + ((j % 2) ? -1 : 1) * (j / 2)) {
					if ((hi_host->tuning_sample_flag & (mask << i)) == (mask << i)) {
						hi_host->tuning_init_sample = i + mask_lenth / 2 ;
						break;
					}

					j++;
				}

				if (hi_host->tuning_init_sample != -1) {
					hi_host_trace(HIMCI_TRACE_SIGNIFICANT,
						"tuning OK: timing is %d, tuning sample = %d, tuning_flag = 0x%x",
						ms_host->mmc->ios.timing, hi_host->tuning_init_sample, hi_host->tuning_sample_flag);
					ret = 0;
					break;
				}
		}

		if (-1 == hi_host->tuning_init_sample) {
			hi_host->tuning_init_sample = (sample_min + sample_max) / 2;
			hi_host_trace(HIMCI_TRACE_SIGNIFICANT,
				"tuning err: no good sam_del, timing is %d, tuning_flag = 0x%x",
				ms_host->mmc->ios.timing, hi_host->tuning_sample_flag);
			ret = -1;
		}
#endif

		mshci_hi_set_timing(hi_host, hi_host->tuning_init_sample, -1, -1);
		return ret;
	} else {
		hi_host->tuning_current_sample++;
		mshci_hi_set_timing(hi_host, hi_host->tuning_current_sample, -1, -1);
		return 1;
	}
	return 0;
}

static void mshci_hi_tuning_set_current_state(struct mshci_host *ms_host, int ok)
{
	struct himci_host *hi_host;
	hi_host = mshci_priv(ms_host);

	mshci_hi_tuning_set_flags(hi_host, hi_host->tuning_current_sample, ok);
}

static int mshci_hi_tuning_move(struct mshci_host *ms_host, int start)
{
	struct himci_host *hi_host;
	hi_host = mshci_priv(ms_host);
	int sample_min, sample_max;
	int loop;

	sample_min = hi_host->init_tuning_config[4 + (ms_host->mmc->ios.timing + 1) * TUNING_INIT_CONFIG_NUM];
	sample_max = hi_host->init_tuning_config[3 + (ms_host->mmc->ios.timing + 1) * TUNING_INIT_CONFIG_NUM];

	if (start) {
		hi_host->tuning_move_count = 0;
	}

	for (loop = 0; loop < 2; loop++) {
		hi_host->tuning_move_count++;
		hi_host->tuning_move_sample =
			hi_host->tuning_init_sample +
			((hi_host->tuning_move_count % 2) ? 1 : -1) * (hi_host->tuning_move_count / 2);

		if ((hi_host->tuning_move_sample > sample_max) ||
			(hi_host->tuning_move_sample < sample_min)) {
			continue;
		} else {
			break;
		}
	}

	if ((hi_host->tuning_move_sample > sample_max) ||
		(hi_host->tuning_move_sample < sample_min)) {
		mshci_hi_set_timing(hi_host, hi_host->tuning_init_sample, -1, -1);
		printk(KERN_ERR "id = %d, tuning move to init del_sel %d\n",
			hi_host->pdev->id, hi_host->tuning_init_sample);
		return 0;
	} else {
		mshci_hi_set_timing(hi_host, hi_host->tuning_move_sample, -1, -1);
		printk(KERN_ERR "id = %d, tuning move to current del_sel %d\n",
			hi_host->pdev->id, hi_host->tuning_move_sample);
		return 1;
	}
}

static struct mshci_ops mshci_hi_ops = {
	.get_max_clock		= mshci_hi_get_max_clk,
	.get_min_clock		= NULL,
	.get_present_status = mshci_hi_get_present_status,  /* return: 0 -- present; 1 -- not present */
	.set_ios			= mshci_hi_set_ios,
	.start_signal_voltage_switch
						= mshci_hi_start_signal_voltage_switch,
	.set_clock		    = NULL,

	.tuning_find_condition
						= mshci_hi_tuning_find_condition,
	.tuning_set_current_state
						= mshci_hi_tuning_set_current_state,
	.tuning_move		= mshci_hi_tuning_move,

};

static void mshci_hi_notify_change(struct platform_device *dev, int state)
{
	struct mshci_host *host = platform_get_drvdata(dev);
	unsigned long flags;

	if (host) {
		spin_lock_irqsave(&host->lock, flags);
		if (state) {
			dev_info(&dev->dev, "card inserted.\n");
			host->flags &= ~MSHCI_DEVICE_DEAD;
		} else {
			dev_info(&dev->dev, "card removed.\n");
			host->flags |= MSHCI_DEVICE_DEAD;
		}
		tasklet_schedule(&host->card_tasklet);
		spin_unlock_irqrestore(&host->lock, flags);
	}
}
static void mshci_hi_sdio_set_power(struct platform_device *dev, int val)
{
	struct mshci_host *host = platform_get_drvdata(dev);
	struct himci_host * hi_host = (struct himci_host *)(host->private);
	int ret = -1;
	u32 loop_count = 1000; /* wait 10S */
	u32 i = 0;

	himci_assert(host);
	himci_assert(hi_host);

	for (i = 0; i < loop_count; i++) {
		if (MMC_HOST_BUSY == host->working || host->mrq) {
			msleep(10);
		} else {
			break;
		}
	}

	if (val) {
		printk("%s:val=%d, set io to normal mode\n", __func__, val);
		host->mmc->ios.power_mode = MMC_POWER_UP;
		host->mmc->ios.timing = MMC_TIMING_LEGACY;
		host->mmc->ios.bus_width = MMC_BUS_WIDTH_1;
		host->mmc->ios.clock = 0;
		host->mmc->ops->set_ios(host->mmc, &host->mmc->ios);

		ret = blockmux_set(hi_host->piomux_block, hi_host->pblock_config, NORMAL);
		if (ret) {
			himci_error("failed to blockmux_set");
		}
		msleep(10);

		host->mmc->ios.power_mode = MMC_POWER_ON;
		host->mmc->ios.clock = 400000;
		host->mmc->ops->set_ios(host->mmc, &host->mmc->ios);

	} else {
		printk("%s:val=%d, set io to lowpower mode\n", __func__, val);
		host->mmc->ios.clock = 0;
		host->mmc->ios.power_mode = MMC_POWER_OFF;
		host->mmc->ios.bus_width = MMC_BUS_WIDTH_1;
		host->mmc->ios.timing = MMC_TIMING_LEGACY;

		ret = blockmux_set(hi_host->piomux_block, hi_host->pblock_config, LOWPOWER);
		if (ret) {
			himci_error("failed to blockmux_set");
		}
	}
}


static int mshci_hi_disable_voltage(struct notifier_block *nb,
				    unsigned long event,
				    void *ignored)
{
	struct himci_host *hi_host = container_of(nb, struct himci_host, nb);
	struct mshci_host *ms_host = hi_host->ms_host;

	if ((event & REGULATOR_EVENT_FORCE_DISABLE) == REGULATOR_EVENT_FORCE_DISABLE) {
		hi_host->ocp_flag = 1;
		printk(KERN_ERR "%s ocp_flag = %d, event:0x%x\n", __func__, hi_host->ocp_flag, event);
		tasklet_schedule(&ms_host->card_tasklet);
	}

	return NOTIFY_OK;
}

static int __devinit hi_mci_probe(struct platform_device *pdev)
{
	struct mshci_host *ms_host = NULL;
	struct himci_host *hi_host = NULL;
	struct hisik3_mmc_platform_data *plat = NULL;
	struct resource *memres = NULL;
	int ret = 0, irq;
	int err;
	unsigned long flags;

	himci_trace(HIMCI_TRACE_GEN_API, "++");

	himci_assert(pdev);

	plat = pdev->dev.platform_data;

	himci_trace(HIMCI_TRACE_SIGNIFICANT, "id:%d", pdev->id);

	/* sd control, if disabled, do not probe*/
	if (0 == pdev->id && get_sd_enable() == false){
		printk(KERN_INFO "%s sd not use\n", __func__);
		return -ENOENT;
	}

	/* must have platform data */
	if (!plat) {
		himci_error("Platform data not available");
		return -ENOENT;
	}

	irq = platform_get_irq(pdev, 0);
	memres = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!irq || !memres) {
		himci_error("resource error");
		return -ENOENT;
	}

	himci_trace(HIMCI_TRACE_SIGNIFICANT, "irq:%d,start:0x%x,size:0x%x", irq, \
										memres->start, resource_size(memres));

	ms_host = mshci_alloc_host(&pdev->dev, sizeof(struct himci_host));
	if (IS_ERR(ms_host)) {
		himci_error("mshci_alloc_host() failed\n");
		return PTR_ERR(ms_host);
	}

	hi_host = mshci_priv(ms_host);
	hi_host->ms_host = ms_host;
	hi_host->pdev = pdev;
	hi_host->dev = &pdev->dev;
	hi_host->plat = plat;

	platform_set_drvdata(pdev, ms_host);

	if ( get_chipid() == CS_CHIP_ID )
		hi_host->init_tuning_config = plat->init_tuning_config;

	/* set emmc pclk */
	hi_host->pclk = clk_get(&pdev->dev, plat->clk_name);

        ms_host->pclk = NULL;
	
	//z00186406 add to enable wlan's host controller begin
        if(plat->quirks){
                if((plat->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) ||
                     (plat->quirks & MSHCI_QUIRK_WLAN_DETECTION) ||
                     (plat->quirks & MSHCI_QUIRK_TI_WL18XX)){
                        ms_host->clk_ref_counter = CLK_DISABLED;
                }
                //z00186406 add, to enable sdio clock for TI wifi end
                else{
                        ms_host->clk_ref_counter = CLK_ENABLED;
                }
        }
	//z00186406 add to enable wlan's host controller end

	if (IS_ERR(hi_host->pclk)) {
		himci_error("clk_get fail!");
		ret = PTR_ERR(hi_host->pclk);
		goto err_io_clk;
	}

	ms_host->pclk = hi_host->pclk;

	if ( get_chipid() == CS_CHIP_ID ) {
		ret = clk_set_rate(hi_host->pclk, hi_host->init_tuning_config[0]);
		if (ret) {
			himci_error("failed to clk_set_rate");
		}

		hi_host->tuning_current_sample = -1;
		hi_host->tuning_init_sample = (hi_host->init_tuning_config[3] +
									hi_host->init_tuning_config[4]) / 2;
		mshci_hi_set_timing(hi_host,
				hi_host->tuning_init_sample,
				hi_host->init_tuning_config[2],
				hi_host->init_tuning_config[1]);
	}
	else
	{
		ret = clk_set_rate(hi_host->pclk, MMC_CCLK_MAX);
		if (ret) {
			himci_error("failed to clk_set_rate");
		}
	}



        if(ms_host->clk_ref_counter == CLK_DISABLED){
                ret = clk_enable(hi_host->pclk);
                ms_host->clk_ref_counter = CLK_ENABLED;
                if (ret) {
                        himci_error("clk_enable failed");
                        ret = -ENOENT;
                        goto err_no_busclks;
                }
        }

	ms_host->ioaddr = ioremap_nocache(memres->start, resource_size(memres));
	if (!ms_host->ioaddr) {
		himci_error("ioremap_nocache failed");
		ret = -ENXIO;
		goto err_req_regs;
	}

	ms_host->hw_name = "hisi_k3v2_mmc";
	ms_host->hw_mmc_id = hi_host->pdev->id;
	ms_host->ops = &mshci_hi_ops;
	ms_host->quirks = 0;
	ms_host->irq = irq;

	/* Setup quirks for the controller */

	if (plat->quirks) {
		ms_host->quirks |= plat->quirks;
	}

	if (plat->caps & MMC_CAP_CLOCK_GATING) {
		/* there is no reason not to use interral clock gating */
		ms_host->mmc->caps |= plat->caps;
		ms_host->mmc->caps |= MMC_CAP_CLOCK_GATING;
		ms_host->clock_gate = 1;
    } else {
		ms_host->mmc->caps |= plat->caps;
		ms_host->clock_gate = 0;
	}

	/* sandisk card need clock longer than spec ask */
	if (ms_host->hw_mmc_id == 0)
		ms_host->clock_gate = 0;

	
	//z00186406 add to enable wifi suspending begin
	if(ms_host->quirks & MSHCI_QUIRK_TI_WL18XX) {
            ms_host->mmc->pm_caps |= MMC_PM_KEEP_POWER | MMC_PM_WAKE_SDIO_IRQ;
        }
	//z00186406 add to enable wifi suspending end
	
	if (plat->ocr_mask)
		ms_host->mmc->ocr_avail |= plat->ocr_mask;

	hi_host->piomux_block = iomux_get_block(plat->iomux_name);
	hi_host->pblock_config = iomux_get_blockconfig(plat->iomux_name);

	if (plat->reg_name) {
		himci_trace(HIMCI_TRACE_SIGNIFICANT, "devname : %s, regname: %s",
					dev_name(hi_host->dev), plat->reg_name);
		hi_host->vcc = regulator_get(hi_host->dev, plat->reg_name);
		if (!IS_ERR(hi_host->vcc)) {
			/*
			 * Setup a notifier block to update this if another device
			 * causes the voltage to change
			 */
			hi_host->nb.notifier_call = &mshci_hi_disable_voltage;
			ret = regulator_register_notifier(hi_host->vcc, &hi_host->nb);
			if (ret) {
				dev_err(&pdev->dev,
					"regulator notifier request failed\n");
			}

			hi_host->ocp_flag = 0;
		} else {
			dev_err(&pdev->dev, "regulator_get() failed\n");
			hi_host->vcc = NULL;
		}
	}

	if (plat->signal_reg_name) {
		himci_trace(HIMCI_TRACE_SIGNIFICANT, "devname : %s, signal regname: %s",
						dev_name(hi_host->dev), plat->signal_reg_name);
		hi_host->signal_vcc = regulator_get(hi_host->dev, plat->signal_reg_name);
		if (IS_ERR(hi_host->signal_vcc)) {
			dev_err(&pdev->dev, "regulator_get() failed\n");
			hi_host->signal_vcc = NULL;
		}
	}

	hi_host->old_sig_voltage = plat->default_signal_voltage;
	hi_host->old_timing = MMC_TIMING_UHS_DDR50;
	hi_host->timing_config = plat->timing_config;
	hi_host->allow_switch_signal_voltage = plat->allow_switch_signal_voltage;
	hi_host->suspend_timing_config = plat->suspend_timing_config;

	if (ms_host->quirks & MSHCI_QUIRK_WLAN_DETECTION) {
		ms_host->flags |= MSHCI_DEVICE_DEAD;
	}

	ret = mshci_add_host(ms_host);
	if (ret) {
		dev_err(&pdev->dev, "mshci_add_host() failed\n");
		goto err_add_host;
	}

	if (ms_host->quirks & MSHCI_QUIRK_WLAN_DETECTION) {
		if (plat->ext_cd_init)
			plat->ext_cd_init(&mshci_hi_notify_change);
		plat->set_power = mshci_hi_sdio_set_power;
	}

	if (ms_host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
#ifdef SD_FPGA_GPIO
		hi_host->gpioaddr = ioremap_nocache(0xFE10F000, SZ_4K);

		if (!hi_host->gpioaddr) {
			himci_trace(HIMCI_TRACE_SIGNIFICANT, "g00175134: gpioaddr is null\n");
		}
		GPIO_CD_DIR(hi_host->gpioaddr) = 0x7C;
		GPIO_CD_IS(hi_host->gpioaddr) = 0x0;
		GPIO_CD_IBE(hi_host->gpioaddr) = 0x1;
		GPIO_CD_IC(hi_host->gpioaddr) = 0xFF;
		GPIO_CD_IE(hi_host->gpioaddr) = 0x1;

		err = request_irq(19+32, mshci_hi_card_detect_gpio,
				/*IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,*/
				IRQF_SHARED,
				mmc_hostname(ms_host->mmc), ms_host);
		if (err) {
			dev_warn(mmc_dev(ms_host->mmc), "request gpio irq error\n");
			goto no_card_detect_irq;
		}

#else
		err = gpio_request_one(plat->cd_gpio, GPIOF_IN, "ESDHC_CD");
		if (err) {
			dev_warn(mmc_dev(ms_host->mmc),
				"no card-detect pin available!\n");
			goto no_card_detect_pin;
		}

		err = request_irq(gpio_to_irq(plat->cd_gpio), mshci_hi_card_detect_gpio,
				 IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				 mmc_hostname(ms_host->mmc), ms_host);
		if (err) {
			dev_warn(mmc_dev(ms_host->mmc), "request gpio irq error\n");
			goto no_card_detect_irq;
		}
#endif
	}

	return 0;

no_card_detect_irq:
	gpio_free(plat->cd_gpio);
no_card_detect_pin:
	plat->cd_gpio = err;
err_add_host:
	iounmap(ms_host->ioaddr);
	ms_host->ioaddr = NULL;
err_req_regs:
	spin_lock_irqsave(&ms_host->lock, flags);
	if(ms_host->clk_ref_counter == CLK_ENABLED){
		clk_disable(hi_host->pclk);
		ms_host->clk_ref_counter = CLK_DISABLED;
	}
	spin_unlock_irqrestore(&ms_host->lock, flags);

err_no_busclks:
	clk_put(hi_host->pclk);

err_io_clk:
	mshci_free_host(ms_host);

	return ret;
}

static int __devexit hi_mci_remove(struct platform_device *pdev)
{
	struct mshci_host *ms_host =  platform_get_drvdata(pdev);
	struct himci_host *hi_host = mshci_priv(ms_host);
	struct hisik3_mmc_platform_data *plat = pdev->dev.platform_data;

	unsigned long flags;

	if (ms_host->quirks & MSHCI_QUIRK_WLAN_DETECTION) {
		if (plat->ext_cd_init)
			plat->ext_cd_cleanup(&mshci_hi_notify_change);
	}
#ifdef CONFIG_MMC_SD_HUB
    mshci_disconnect(ms_host);
#endif
	mshci_remove_host(ms_host, 1);

	iounmap(ms_host->ioaddr);
	ms_host->ioaddr = NULL;


	spin_lock_irqsave(&ms_host->lock, flags);
	if(ms_host->clk_ref_counter == CLK_ENABLED){
		clk_disable(hi_host->pclk);
		ms_host->clk_ref_counter = CLK_DISABLED;
	}
	spin_unlock_irqrestore(&ms_host->lock, flags);


	clk_put(hi_host->pclk);
	if (hi_host->vcc) {
		regulator_unregister_notifier(hi_host->vcc, &hi_host->nb);
		regulator_disable(hi_host->vcc);
		regulator_put(hi_host->vcc);
	}

	if (hi_host->signal_vcc) {
		regulator_put(hi_host->signal_vcc);
	}

	mshci_free_host(ms_host);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
#ifdef CONFIG_WIFI_GPIO_OPTIMIZE
#define MMC_ID_WIFI 3
#define SDIO_BLOCK "block_sdio"
static struct iomux_block *sdio_block = NULL;
static struct block_config *sdio_block_config  = NULL;

int config_wifi_sdio_interface(enum iomux_func mode)
{
	int ret = 0;

	if(sdio_block == NULL){
		sdio_block = iomux_get_block(SDIO_BLOCK);
		if(sdio_block == NULL){
			printk("get sdio block error\n");
			return -1;
		}
	}

	if(sdio_block_config == NULL){
		sdio_block_config = iomux_get_blockconfig(SDIO_BLOCK);
		if(sdio_block_config == NULL){
			printk("get sdio config block error\n");
			return -1;
        }
	}

	ret = blockmux_set(sdio_block, sdio_block_config, mode);
	if(ret < 0){
		printk("set sdio config error:%d\n", ret);
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL(config_wifi_sdio_interface);
#endif

static int hi_mci_suspend(struct platform_device *dev, pm_message_t pm)
{
	struct mshci_host *ms_host = platform_get_drvdata(dev);
	struct himci_host *hi_host = mshci_priv(ms_host);
	int ret = 0;
	unsigned long flags;
	
	//z00186406 add, to enable TI wifi suspend/resume functionality,  begin
	if (ms_host->quirks & (MSHCI_QUIRK_WLAN_DETECTION | MSHCI_QUIRK_TI_WL18XX)) {
	//z00186406 add, to enable TI wifi suspend/resume functionality, end
		/* sdio power off */
		if (ms_host->mmc->ios.power_mode != MMC_POWER_OFF) {
			ret = mmc_sdio_suspend_ext(ms_host->mmc);
			if (ret) {
				printk("%s, sdio suspend error\n", __func__);
				return ret;
			}
			
			//z00186406 add,  while suspend let wifi to keep power, begin
			ms_host->mmc->ios.power_mode = MMC_POWER_ON;
			//z00186406 add,  while suspend let wifi to keep power, end
			hi_host->old_timing = 0;
			hi_host->old_sig_voltage = 0;
		}
	}
	mshci_suspend_host(ms_host, pm);

	spin_lock_irqsave(&ms_host->lock, flags);
	if(ms_host->clk_ref_counter == CLK_ENABLED){
		clk_disable(hi_host->pclk);
		ms_host->clk_ref_counter = CLK_DISABLED;
	}
	spin_unlock_irqrestore(&ms_host->lock, flags);

	mshci_hi_update_timing(ms_host, 1);

#ifdef CONFIG_WIFI_GPIO_OPTIMIZE
	if(MMC_ID_WIFI == dev->id){
		config_wifi_sdio_interface(LOWPOWER);
	}
#endif
	return 0;
}

static int hi_mci_resume(struct platform_device *dev)
{
	struct mshci_host *ms_host = platform_get_drvdata(dev);
	struct himci_host *hi_host = mshci_priv(ms_host);
	int ret = 0;
	unsigned long flags;

#ifdef CONFIG_WIFI_GPIO_OPTIMIZE
	if(MMC_ID_WIFI == dev->id){
		config_wifi_sdio_interface(NORMAL);
	}
#endif

	if ( get_chipid() == CS_CHIP_ID )
	{
		ret = clk_set_rate(hi_host->pclk, hi_host->init_tuning_config[0]);
		if (ret) {
			himci_error("failed to clk_set_rate");
		}

		hi_host->tuning_current_sample = -1;
		hi_host->tuning_init_sample = (hi_host->init_tuning_config[3] +
									hi_host->init_tuning_config[4]) / 2;
		mshci_hi_set_timing(hi_host,
				hi_host->tuning_init_sample,
				hi_host->init_tuning_config[2],
				hi_host->init_tuning_config[1]);
	}
	else
	{
		ret = clk_set_rate(hi_host->pclk, MMC_CCLK_MAX);
		if (ret) {
			himci_error("failed to clk_set_rate");
		}
	}

	mshci_resume_host(ms_host);
	
	//z00186406 add, to enable TI wifi suspend/resume functionality,  begin 
	if (ms_host->quirks & (MSHCI_QUIRK_WLAN_DETECTION | MSHCI_QUIRK_TI_WL18XX)) {
	//z00186406 add, to enable TI wifi suspend/resume functionality, end
		if (ms_host->mmc->ios.power_mode != MMC_POWER_OFF) {
			ret = mmc_sdio_resume_ext(ms_host->mmc);
			if (ret) {
				printk("%s, sdio resume error\n", __func__);
				return ret;
			}
		}
		//z00186406, mask power keep flag to let sdio_card re_intialize, begin
		ms_host->mmc->pm_flags &= ~MMC_PM_KEEP_POWER;
		//z00186406, mask power keep flag to let sdio_card re_intialize, end
	}
	return ret;
}

#else
#define hi_mci_suspend  NULL
#define hi_mci_resume   NULL
#endif

static struct platform_driver hi_mci_driver = {
	.probe		= hi_mci_probe,
	.remove		= hi_mci_remove,
	.suspend	= hi_mci_suspend,
	.resume		= hi_mci_resume,
	.driver		= {
		.name	  = "hi_mci",
	},
};

static int __init hi_mci_init(void)
{
	spin_lock_init(&mmc_tuning_lock);
	return platform_driver_register(&hi_mci_driver);
}

static void __exit hi_mci_exit(void)
{
	platform_driver_unregister(&hi_mci_driver);
}

module_init(hi_mci_init);
module_exit(hi_mci_exit);

#ifdef MODULE
MODULE_AUTHOR("Hisilicon Driver Group");
MODULE_DESCRIPTION("MMC/SD driver for the Hisilicon MMC/SD Host Controller");
MODULE_LICENSE("GPL");
#endif
