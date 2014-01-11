/*
 * arch/arm/mach-k3v2/board-tc45msu3.c
 *
 * Copyright (C) 2010 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */



#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <linux/clk.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/dma-mapping.h>
#include <linux/pda_power.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/amba/bus.h>
#include <linux/io.h>

#include <linux/i2c.h>
#include <linux/input.h>

#include <linux/power/bq27510_battery.h>
#include <linux/power/bq2419x_charger.h>
#include <linux/power/bq_bci_battery.h>
#include <linux/nct203.h>
#if 0
#include <linux/power/k3_bq24161.h>
#include <linux/power/k3_bq27510.h>
#include <linux/power/k3_battery_monitor.h>
#endif
#include <linux/mhl/mhl.h>

#include <linux/hkadc/hiadc_hal.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/system.h>
#include <asm/mach/arch.h>
#include <mach/hardware.h>
#include <mach/system.h>
#include <mach/irqs.h>
#include <mach/io.h>
#include <mach/gpio.h>
#include <mach/early-debug.h>
#include <mach/hisi_mem.h>
#include <mach/k3_keypad.h>
#include <mach/boardid.h>
#include <mach/atmel_mXT224E.h>
#include <mach/tps61310.h>

#include "board.h"
#include "clock.h"
#include "k3v2_clocks_init_data.h"
#include <mach/sound/tpa2028_spk_l.h>
#include <mach/sound/tpa2028_spk_r.h>
#include <mach/sound/tpa6132.h>
#include <mach/sound/spk_5vboost.h>
#include <mach/sound/es305.h>
#include <mach/sound/modemctl.h>
#include <mach/sound/tfa9887.h>

#include <linux/switch_usb.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/irq.h>
#include <hsad/config_debugfs.h>
#include <hsad/config_interface.h>
#include <linux/uart_output.h>
#include <linux/sc16is750_i2c.h>
#ifdef CONFIG_LEDS_K3_6421
#include <linux/led/k3-leds.h>
#endif

#ifdef CONFIG_ANDROID_K3_VIBRATOR
#include <linux/vibrator/k3_vibrator.h>
#endif

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif
#include <mach/ak6921af.h>
#ifdef CONFIG_TOUCHSCREEN_RMI4_SYNAPTICS_GENERIC
#include <linux/rmi.h>
#include <linux/interrupt.h>
#endif
#ifdef CONFIG_SYNAPTICS_TOUCH_KEY
#include "../../../drivers/huawei/device/touchkey/synaptics_SO340010.h"
#endif

#include <linux/pn544.h>
/* Cypress cyttsp4 start */
/* cyttsp */
#include <linux/cyttsp4_bus.h>
#include <linux/cyttsp4_core.h>
#include <linux/cyttsp4_btn.h>
#include <linux/cyttsp4_mt.h>

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_INCLUDE_FW
#include <linux/cyttsp4_img.h>

static struct cyttsp4_touch_firmware cyttsp4_firmware = {
	.img = cyttsp4_img,
	.size = ARRAY_SIZE(cyttsp4_img),
	.ver = cyttsp4_ver,
	.vsize = ARRAY_SIZE(cyttsp4_ver),
};
#else
static struct cyttsp4_touch_firmware cyttsp4_firmware = {
	.img = NULL,
	.size = 0,
	.ver = NULL,
	.vsize = 0,
};
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_AUTO_LOAD_TOUCH_PARAMS
#include <linux/cyttsp4_params.h>
static struct touch_settings cyttsp4_sett_param_regs = {
	.data = (uint8_t *)&cyttsp4_param_regs[0],
	.size = ARRAY_SIZE(cyttsp4_param_regs),
	.tag = 0,
};

static struct touch_settings cyttsp4_sett_param_size = {
	.data = (uint8_t *)&cyttsp4_param_size[0],
	.size = ARRAY_SIZE(cyttsp4_param_size),
	.tag = 0,
};
#else
static struct touch_settings cyttsp4_sett_param_regs = {
	.data = NULL,
	.size = 0,
	.tag = 0,
};

static struct touch_settings cyttsp4_sett_param_size = {
	.data = NULL,
	.size = 0,
	.tag = 0,
};
#endif

static struct cyttsp4_loader_platform_data _cyttsp4_loader_platform_data = {
	.fw = &cyttsp4_firmware,
	.param_regs = &cyttsp4_sett_param_regs,
	.param_size = &cyttsp4_sett_param_size,
	.flags = 1,
};


#define CYTTSP4_USE_I2C
/* #define CYTTSP4_USE_SPI */

#ifdef CYTTSP4_USE_I2C
#define CYTTSP4_I2C_NAME "cyttsp4_i2c_adapter"
#define CYTTSP4_I2C_TCH_ADR 0x1A
#define CYTTSP4_LDR_TCH_ADR 0x1A
#define CYTTSP4_I2C_IRQ_GPIO TOUCH_INT_PIN /* J6.9, C19, GPMC_AD14/GPIO_38 */
#define CYTTSP4_I2C_RST_GPIO TOUCH_RESET_PIN /* J6.10, D18, GPMC_AD13/GPIO_37 */
#endif

#ifdef CYTTSP4_USE_SPI
#define CYTTSP4_SPI_NAME "cyttsp4_spi_adapter"
/* Change GPIO numbers when using I2C and SPI at the same time
 * Following is possible alternative:
 * IRQ: J6.17, C18, GPMC_AD12/GPIO_36
 * RST: J6.24, D17, GPMC_AD11/GPIO_35
 */
#define CYTTSP4_SPI_IRQ_GPIO 38 /* J6.9, C19, GPMC_AD14/GPIO_38 */
#define CYTTSP4_SPI_RST_GPIO 37 /* J6.10, D18, GPMC_AD13/GPIO_37 */
#endif

/* Check GPIO numbers if both I2C and SPI are enabled */
#if defined(CYTTSP4_USE_I2C) && defined(CYTTSP4_USE_SPI)
#if CYTTSP4_I2C_IRQ_GPIO == CYTTSP4_SPI_IRQ_GPIO || \
	CYTTSP4_I2C_RST_GPIO == CYTTSP4_SPI_RST_GPIO
#error "GPIO numbers should be different when both I2C and SPI are on!"
#endif
#endif

#define CY_MAXX 720
#define CY_MAXY 1280
#define CY_MINX 0
#define CY_MINY 0

#define CY_ABS_MIN_X CY_MINX
#define CY_ABS_MIN_Y CY_MINY
#define CY_ABS_MAX_X CY_MAXX
#define CY_ABS_MAX_Y CY_MAXY
#define CY_ABS_MIN_P 0
#define CY_ABS_MAX_P 255
#define CY_ABS_MIN_W 0
#define CY_ABS_MAX_W 255

#define CY_ABS_MIN_T 0

#define CY_ABS_MAX_T 15

#define CY_IGNORE_VALUE 0xFFFF
/* Cypress cyttsp4 end */

//#define	GPIO_BT_EN					(GPIO_21_1)
//#define	GPIO_BT_RST					(GPIO_21_0)
//#define	GPIO_HOST_WAKEUP			(GPIO_20_6)
//#define	GPIO_DEV_WAKEUP				(GPIO_20_7)

//#define	REGULATOR_DEV_BLUETOOTH_NAME	"bt-io"

//#define	GPIO_BT_EN					(GPIO_21_1)   
//#define	GPIO_BT_RST					(GPIO_21_0)	
//#define	GPIO_HOST_WAKEUP			(GPIO_20_6) 
//#define	GPIO_DEV_WAKEUP				(GPIO_20_7) 
#define	GPIO_BT_EN					(GPIO_21_1)		
#include <linux/skbuff.h>
#include <linux/ti_wilink_st.h>
//#define	REGULATOR_DEV_BLUETOOTH_NAME	"bt-io"    

#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
#include <mach/extral_dynamic_dcdc.h>
#endif

#define NFC_NAME "pn544"

/* for framebuffer */
#ifdef CONFIG_LCD_TOSHIBA_MDW70
#define PLATFORM_DEVICE_LCD_NAME "mipi_toshiba_MDW70_V001"
#elif  defined(CONFIG_LCD_PANASONIC_VVX10F002A00)
#define PLATFORM_DEVICE_LCD_NAME "mipi_panasonic_VVX10F002A00"
#elif defined(CONFIG_LCD_CMI_OTM1280A)
#define PLATFORM_DEVICE_LCD_NAME "mipi_cmi_OTM1280A"
#elif defined(CONFIG_LCD_SAMSUNG_LMS350DF04)
#define PLATFORM_DEVICE_LCD_NAME "ldi_samsung_LMS350DF04"
#elif defined(CONFIG_LCD_SAMSUNG_S6E39A)
#define PLATFORM_DEVICE_LCD_NAME "mipi_samsung_S6E39A"
#elif defined(CONFIG_LCD_SHARP_LS035B3SX)
#define PLATFORM_DEVICE_LCD_NAME "mipi_sharp_LS035B3SX"
#elif defined(CONFIG_LCD_JDI_OTM1282B)
#define PLATFORM_DEVICE_LCD_NAME "mipi_jdi_OTM1282B"
#elif defined(CONFIG_LCD_CMI_PT045TN07)
#define PLATFORM_DEVICE_LCD_NAME "mipi_cmi_PT045TN07"
#elif defined(CONFIG_LCD_TOSHIBA_MDY90)
#define PLATFORM_DEVICE_LCD_NAME "mipi_toshiba_MDY90"
#elif defined(CONFIG_LCD_K3_FAKE)
#define PLATFORM_DEVICE_LCD_NAME "lcd_k3_fake_fb"
#else
#error "PLATFORM_DEVICE_LCD_NAME not defined"
#endif
/* Begin: Added by d59977 for BCM GPS */
#define GPIO_GPS_BCM_EN    (GPIO_18_7)
#define GPIO_GPS_BCM_RET   (GPIO_19_0)
/* End: Added by d59977 for BCM GPS */

/* Begin: Added by d59977 for BCM GPS */
#define GPIO_GPS_BCM_EN_NAME    "gpio_gps_bcm_enable"
#define GPIO_GPS_BCM_RET_NAME   "gpio_gps_bcm_rest"
/* End: Added by d59977 for BCM GPS */
#define TOUCH_INT_PIN  GPIO_19_5  /*GPIO_157*/
#define TOUCH_RESET_PIN GPIO_19_4

#define SYNA_NAME "rmi_i2c"
//add for touchkey
#define TOUCHKEY_INT GPIO_5_5	//45
#define TOUCHKEY_LED_ENABLE GPIO_1_0	//8

/*pengchao add begin*/
atomic_t touch_is_pressed;
EXPORT_SYMBOL(touch_is_pressed);
u32 time_finger_up = 0;
EXPORT_SYMBOL(time_finger_up);
/*pengchao add end*/
//add for IRDA
#define IRDA_NAME "sc16is750_i2c"

#define SECRAM_RESET_ADDR	IO_ADDRESS(REG_BASE_PMUSPI + (0x87 << 2))
#define SECRAM_REST_FLAG_LEN	(0x8)
#define SECRAM_REST_INFO_LEN	(0x20)

#define RESET_COLD_FLAG		"coldboot"
#define RESET_WARM_FLAG		"warmboot"
#define SCTRL_SCSYSSTAT		0x004
#define SCTRL_SCPERRSTEN0	0x080
#define PMU_RST_CTRL		(0x035<<2)

static struct resource k3_adc_resources = {
	.start	= REG_BASE_PMUSPI,
	.end	= REG_BASE_PMUSPI + REG_PMUSPI_IOSIZE - 1,
	.flags	= IORESOURCE_MEM,
};

static struct adc_data hi6421_adc_table[] = {
	{
		.ch = ADC_ADCIN1,
		.vol = ADC_VOLTAGE_MOD1,
		.clock = HKADC_CK_SEL_TWO,
		.buffer = HKADC_BUFF_SEL_YES,
		.parameter = ADC_PARAMETER1,
	},
	{
		.ch = ADC_ADCIN2,
		.vol = ADC_VOLTAGE_MOD1,
		.clock = HKADC_CK_SEL_TWO,
		.buffer = HKADC_BUFF_SEL_YES,
		.parameter = ADC_PARAMETER1,
	},
	{
		.ch = ADC_ADCIN3,
		.vol = ADC_VOLTAGE_MOD3,
		.clock = HKADC_CK_SEL_TWO,
		.buffer = HKADC_BUFF_SEL_YES,
		.parameter = ADC_PARAMETER1,
	},
	{
		.ch = ADC_NC0,
		.vol = ADC_VOLTAGE_MOD1,
		.clock = HKADC_CK_SEL_TWO,
		.buffer = HKADC_BUFF_SEL_YES,
		.parameter = ADC_PARAMETER1,
	},
	{
		.ch = ADC_VBATMON,
		.vol = ADC_VOLTAGE_MOD1,
		.clock = HKADC_CK_SEL_TWO,
		.buffer = HKADC_BUFF_SEL_YES,
		.parameter = ADC_PARAMETER2,
	},
	{
		.ch = ADC_VCOINMON,
		.vol = ADC_VOLTAGE_MOD1,
		.clock = HKADC_CK_SEL_TWO,
		.buffer = HKADC_BUFF_SEL_YES,
		.parameter = ADC_PARAMETER2,
	},
	{
		.ch = ADC_RTMP,
		.vol = ADC_VOLTAGE_MOD1,
		.clock = HKADC_CK_SEL_TWO,
		.buffer = HKADC_BUFF_SEL_YES,
		.parameter = ADC_PARAMETER1,
	},
	{
		.ch = ADC_PB_DETECT,
		.vol = ADC_VOLTAGE_MOD2,
		.clock = HKADC_CK_SEL_TWO,
		.buffer = HKADC_BUFF_SEL_YES,
		.parameter = ADC_PARAMETER1,
	},
	{
		.ch = ADC_NC1,
		.vol = ADC_VOLTAGE_MOD1,
		.clock = HKADC_CK_SEL_TWO,
		.buffer = HKADC_BUFF_SEL_YES,
		.parameter = ADC_PARAMETER1,
	},
	{
		.ch = ADC_NC2,
		.vol = ADC_VOLTAGE_MOD1,
		.clock = HKADC_CK_SEL_TWO,
		.buffer = HKADC_BUFF_SEL_YES,
		.parameter = ADC_PARAMETER1,
	},
	{
		.ch = ADC_500K,
		.vol = ADC_VOLTAGE_MOD2,
		.clock = HKADC_CK_SEL_TWO,
		.buffer = HKADC_BUFF_SEL_YES,
		.parameter = ADC_PARAMETER1,
	},
};

static struct adc_dataEx hi6421_adc_tableEx = {
	.data = hi6421_adc_table,
	.sum = ARRAY_SIZE(hi6421_adc_table),
};

static struct platform_device hisik3_adc_device = {
	.name    = "k3adc",
	.id    = 0,
	.dev    = {
		.platform_data = &hi6421_adc_tableEx,
		.init_name = "hkadc",
	},
	.num_resources    = 1,
	.resource    =  &k3_adc_resources,
};

static struct platform_device hisik3_device_hwmon = {
	.name		= "k3-hwmon",
	.id		= -1,
};

#ifdef CONFIG_LEDS_K3_6421
/*k3_led begin*/
static struct k3_led_platform_data hi6421_leds = {
	.leds_size = K3_LEDS_MAX,
	.leds = {
		[0] = {
			.name = "red",
			.brightness = LED_OFF,
			.delay_on = 0,
			.delay_off = 0,
			.default_trigger = "timer",
		},
		[1] = {
			.name = "green",
			.brightness = LED_OFF,
			.delay_on = 0,
			.delay_off = 0,
			.default_trigger = "timer",
		},
		[2] {
			.name = "blue",
			.brightness = LED_OFF,
			.delay_on = 0,
			.delay_off = 0,
			.default_trigger = "timer",
		},
	},
};

static struct k3_led_platform_data hi6421_leds_phone = {
	.leds_size = K3_LEDS_MAX,
	.leds = {
		[0] = {
			.name = "green",
			.brightness = LED_OFF,
			.delay_on = 0,
			.delay_off = 0,
			.default_trigger = "timer",
		},
		[1] = {
			.name = "red",
			.brightness = LED_OFF,
			.delay_on = 0,
			.delay_off = 0,
			.default_trigger = "timer",
		},
		[2] {
			.name = "blue",
			.brightness = LED_OFF,
			.delay_on = 0,
			.delay_off = 0,
			.default_trigger = "timer",
		},
	},
};

static struct resource hi6421_led_resources = {
	.start		= REG_BASE_PMUSPI,
	.end			= REG_BASE_PMUSPI + REG_PMUSPI_IOSIZE - 1,
	.flags		= IORESOURCE_MEM,

};
static struct platform_device hi6421_led_device = {
	.name		= K3_LEDS,
	.id			= 0,
	.dev = {
		.platform_data = &hi6421_leds,
		.init_name = "hkled",
	},
	.num_resources		= 1,
	.resource       =  &hi6421_led_resources,
};
/*k3_led end*/
#endif


#ifdef CONFIG_ANDROID_K3_VIBRATOR
static struct k3_vibrator_platform_data hi6421_vibrator = {
	.low_freq  = PERIOD,
	.low_power = ISET_POWER,
	.mode  = SET_MODE,
	.high_freq = PERIOD_QUICK,
	.high_power = ISET_POWERSTRONG,
};

/*vibrator  begin*/
static struct resource hi6421_vibrator_resources = {
	.start		= REG_BASE_PMUSPI,
	.end		= REG_BASE_PMUSPI + REG_PMUSPI_IOSIZE - 1,
	.flags		= IORESOURCE_MEM,

};
static struct platform_device hi6421_vibrator_device = {
	.name		= K3_VIBRATOR,
	.id			= 0,
	.dev = {
		.platform_data = &hi6421_vibrator,
		.init_name = "hkvibrator",
	},
	.num_resources		= 1,
	.resource       =  &hi6421_vibrator_resources,
};
#endif

/*vibrator end*/

static struct resource hi6421_irq_resources[] = {
	{
		.start		= REG_BASE_PMUSPI,
		.end		= REG_BASE_PMUSPI + REG_PMUSPI_IOSIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= IRQ_GPIO159,
		.end		= IRQ_GPIO159,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device hisik3_hi6421_irq_device = {
	.name		= "hi6421-irq",
	.id			= 0,
	.dev.platform_data	= NULL,
	.num_resources		= ARRAY_SIZE(hi6421_irq_resources),
	.resource       =  hi6421_irq_resources,
};

static struct platform_device k3_lcd_device = {
	.name = PLATFORM_DEVICE_LCD_NAME,
	.id	= 1,
	.dev = {
		.init_name = "k3_dev_lcd",
	},
};

//usb switch C9800 default configuration
#define USB_SWITCH_CTRL_GPIO1	174		// SEL0 for C9800
#define USB_SWITCH_CTRL_GPIO2	42		// SEL1 for C9800
#define USB_SWITCH_CTRL_GPIO3	52		// OE for C9800
#define USB_SWITCH_CTRL_GPIO4	53		// SW for C9800
#define USB_MODEM_LOADSWITCH1	9
#define USB_MODEM_LOADSWITCH2	8
#define USB_SWITCH_INTERRUPT1_GPIO   99
#define USB_SWITCH_INTERRUPT2_GPIO   27

//C9800 deafult configuration
static struct usb_switch_platform_data usw_plat_data = {
		.name           = "usbsw",
		.enable			= true,
		.modem1_supported = true,
		.modem_loadswitch1 = USB_MODEM_LOADSWITCH1,
		.modem_int_gpio1   = USB_SWITCH_INTERRUPT1_GPIO,
		.modem2_supported = true,
		.modem_loadswitch2 = USB_MODEM_LOADSWITCH2,
		.modem_int_gpio2   = USB_SWITCH_INTERRUPT2_GPIO,
		.mhl_supported	= true,
		.control_gpio1	= USB_SWITCH_CTRL_GPIO1,
		.control_gpio2	= USB_SWITCH_CTRL_GPIO2,
		.control_gpio3	= USB_SWITCH_CTRL_GPIO3,
		.control_gpio4	= USB_SWITCH_CTRL_GPIO4,
		.control_gpio1_boot_ap_value = GPIO_LOW,
		.control_gpio2_boot_ap_value = GPIO_LOW,
		.control_gpio3_boot_ap_value = GPIO_HIGH,
		.control_gpio4_boot_ap_value = GPIO_HIGH,
		.control_gpio1_off_value = GPIO_HIGH,
		.control_gpio2_off_value = GPIO_HIGH,
		.control_gpio3_off_value = GPIO_HIGH,
		.control_gpio4_off_value = GPIO_HIGH,
		.control_gpio1_modem1_value = GPIO_HIGH,
		.control_gpio2_modem1_value = GPIO_LOW,
		.control_gpio3_modem1_value = GPIO_LOW,
		.control_gpio4_modem1_value = GPIO_LOW,
		.control_gpio1_modem2_value = GPIO_HIGH,
		.control_gpio2_modem2_value = GPIO_LOW,
		.control_gpio3_modem2_value = GPIO_LOW,
		.control_gpio4_modem2_value = GPIO_HIGH,
		.control_gpio1_mhl_value = GPIO_LOW,
		.control_gpio2_mhl_value = GPIO_HIGH,
		.control_gpio3_mhl_value = GPIO_HIGH,
		.control_gpio4_mhl_value = GPIO_HIGH,
		.irq_flags      = IRQ_TYPE_EDGE_RISING,
};

static struct platform_device usb_switch_device = {
	.name   = "switch-usb",
	.dev    = {
		.init_name = "switch-usb",
		.platform_data = &usw_plat_data,
	},
};


/* Begin: Added by d59977 for BCM GPS */
static struct resource k3_gps_bcm_resources[] = {
	[0] = {
	.name  = GPIO_GPS_BCM_EN_NAME,
	.start = GPIO_GPS_BCM_EN,
	.end   = GPIO_GPS_BCM_EN,
	.flags = IORESOURCE_IO,
	},
	[1] = {
	.name  = GPIO_GPS_BCM_RET_NAME,
	.start = GPIO_GPS_BCM_RET,
	.end   = GPIO_GPS_BCM_RET,
	.flags = IORESOURCE_IO,
	},
};

static struct platform_device k3_gps_bcm_device = {
	.name = "k3_gps_bcm_47511",
	.id	= 1,
	.dev = {
		.init_name = "gps_bcm_47511",
	},
	.num_resources = ARRAY_SIZE(k3_gps_bcm_resources),
	.resource = k3_gps_bcm_resources,
};
/* End: Added by d59977 for BCM GPS */



//static struct resource bluepower_resources[] = {
//	{
//		.name	= "bt_gpio_enable",
//		.start	= GPIO_BT_EN,
//		.end	= GPIO_BT_EN,
//		.flags	= IORESOURCE_IO,
//	},
//	{
//		.name	= "bt_gpio_rst",
//		.start	= GPIO_BT_RST,
//		.end	= GPIO_BT_RST,
//		.flags	= IORESOURCE_IO,
//	},
//};
//
//static struct platform_device btbcm_device = {
//	.name =	"bt_power",
//	.dev  =	{
//		.platform_data = NULL,
//		.init_name = REGULATOR_DEV_BLUETOOTH_NAME,
//	},
//	.id	= -1,
//	.num_resources	= ARRAY_SIZE(bluepower_resources),
//	.resource	= bluepower_resources,
//
//};
//
//static struct resource bluesleep_resources[] = {
//	{
//		.name	= "gpio_host_wake",
//		.start	= GPIO_HOST_WAKEUP,
//		.end	= GPIO_HOST_WAKEUP,
//		.flags	= IORESOURCE_IO,
//	},
//	{
//		.name	= "gpio_ext_wake",
//		.start	= GPIO_DEV_WAKEUP,
//		.end	= GPIO_DEV_WAKEUP,
//		.flags	= IORESOURCE_IO,
//	},
//};
//
//static struct platform_device bcm_bluesleep_device = {
//	.name =	"bluesleep",
//	.id		= -1,
//	.num_resources	= ARRAY_SIZE(bluesleep_resources),
//	.resource	= bluesleep_resources,
//};

static struct resource bluepower_resources[] = {
	{
		.name	= "bt_gpio_enable",
		.start	= GPIO_BT_EN,
		.end	= GPIO_BT_EN,
		.flags	= IORESOURCE_IO,
	},
};

static unsigned long retry_suspend;
static int plat_wlink_kim_suspend(struct platform_device *pdev, pm_message_t
        state)
{
    return 0;
}

static int plat_wlink_kim_resume(struct platform_device *pdev)
{
    return 0;
}

static struct wake_lock st_wk_lock;
/* Call the uart disable of serial driver */
static int plat_chip_asleep(void)
{
	int err = 0;

	printk(KERN_INFO"%s\n", __func__);

	wake_unlock(&st_wk_lock);
	return err;
}

/* Call the uart enable of serial driver */
static int plat_chip_awake(void)
{
	int err = 0;

	printk(KERN_INFO"%s\n", __func__);

	wake_lock(&st_wk_lock);
	return err;
}

static int plat_chip_disable(void)
{
	int err = 0;

	printk(KERN_INFO"%s\n", __func__);

	wake_unlock(&st_wk_lock);
	return err;
}

/* Call the uart enable of serial driver */
static int plat_chip_enable(void)
{
	int err = 0;

	printk(KERN_INFO"%s\n", __func__);
	
	wake_lock(&st_wk_lock);
	return err;
}

//static int plat_kim_chip_enable(struct kim_data_s *kim_data)
//{
//	printk(KERN_INFO"%s\n", __func__);
//	/* Configure BT nShutdown to HIGH state */
//	gpio_set_value(kim_data->nshutdown, GPIO_LOW);
//	mdelay(5);      /* FIXME: a proper toggle */
//	gpio_set_value(kim_data->nshutdown, GPIO_HIGH);
//	printk(KERN_INFO"%s, gpio-%ld high\n", __func__, kim_data->nshutdown);
//	mdelay(1000);
//	return 0;
//}
//
//static int plat_kim_chip_disable(struct kim_data_s *kim_data)
//{
//	printk(KERN_INFO"%s\n", __func__);
//	/* By default configure BT nShutdown to LOW state */
//	gpio_set_value(kim_data->nshutdown, GPIO_LOW);
//	mdelay(1);
//	gpio_set_value(kim_data->nshutdown, GPIO_HIGH);
//	mdelay(1);
//	gpio_set_value(kim_data->nshutdown, GPIO_LOW);
//
//	return 0;
//}
#define WILINK_UART_DEV_NAME "/dev/ttyAMA4"
#define	GPIO_HOST_WAKEUP			(GPIO_20_2)   

/*wl18xx BT, FM, GPS connectivity chip*/
struct ti_st_plat_data wilink_pdata = {
	.nshutdown_gpio = GPIO_BT_EN,
	.dev_name = WILINK_UART_DEV_NAME,
	.flow_cntrl = 1,
//	.baud_rate = 115200,
//	.baud_rate = 921600,
	.baud_rate = 3000000,
	.suspend = plat_wlink_kim_suspend,
  	.resume = plat_wlink_kim_resume,
  	.chip_asleep = plat_chip_asleep,
	.chip_awake  = plat_chip_awake,
	.chip_enable = plat_chip_enable,
	.chip_disable = plat_chip_disable,
};


static struct platform_device wl18xx_device = {
	.name = "kim",
	.id = -1,
	.dev.platform_data = &wilink_pdata,
	.num_resources	= ARRAY_SIZE(bluepower_resources),
	.resource	= bluepower_resources,
};


static struct platform_device btwilink_device = {
	.name = "btwilink",
	.id = -1,
};

static struct resource st_host_wake_resources[] = {
	{
		.name	= "host_wake",
		.start	= GPIO_HOST_WAKEUP,
		.end	= GPIO_HOST_WAKEUP,
		.flags	= IORESOURCE_IRQ |IORESOURCE_IRQ_HIGHEDGE ,
	},

};

static struct platform_device st_host_wake_device = {
	.name =	"st_host_wake",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(st_host_wake_resources),
	.resource	= st_host_wake_resources,
};

/*  camera resources */
static struct resource hisik3_camera_resources[] = {
	{
		.name		= "isp_base",
		.start		= REG_BASE_ISP,
		.end		= REG_BASE_ISP + REG_ISP_IOSIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.name		= "isp_irq",
		.start		= IRQ_ISP,
		.end		= IRQ_ISP,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.name		= "csi0_irq",
		.start		= IRQ_MIPICSI0,
		.end		= IRQ_MIPICSI0,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.name		= "csi1_irq",
		.start		= IRQ_MIPICSI1,
		.end		= IRQ_MIPICSI1,
		.flags		= IORESOURCE_IRQ,
	}
};

static struct platform_device hisik3_camera_device = {
	.id	= 0,
	.name	= "k3-camera-v4l2",
	.dev = {
		.init_name = "camera",
	},
	.resource	= hisik3_camera_resources,
	.num_resources	= ARRAY_SIZE(hisik3_camera_resources),
};

static struct platform_device hisik3_fake_camera_device = {
	.id	= 1,
	.name	= "k3-fake-camera-v4l2",
	.resource	= 0,
	.num_resources	= 0,
	/*
	.dev = {
		.release = camera_platform_release,
	}
	,*/
};

/* Keypad device and platform data start, use KPC realizing keypad. */
static const uint32_t default_keymap[] = {
	/*row, col, key*/
#if 0
	/* used for truly platform.*/
	KEY(0, 0, KEY_MENU),
	KEY(1, 0, KEY_SEND),
	KEY(2, 0, KEY_VOLUMEUP),

	KEY(0, 1, KEY_HOME),
	KEY(1, 1, KEY_END),
	KEY(2, 1, KEY_VOLUMEDOWN),

	KEY(0, 2, KEY_CAMERA_FOCUS),
	KEY(1, 2, KEY_CAMERA),
	KEY(2, 2, DPAD_CENTER),
#endif

	/*row, col, key*/
	/* used for debug only.*/
	KEY(0, 0, KEY_MENU),
	KEY(0, 1, KEY_BACK),

	KEY(1, 0, KEY_LEFT),
	KEY(1, 1, KEY_RIGHT),

	KEY(2, 0, KEY_UP),

	KEY(2, 1, KEY_DOWN),
	KEY(2, 2, DPAD_CENTER),

	KEY(0, 2, KEY_CAMERA_FOCUS),
	KEY(1, 2, KEY_CAMERA),

	/* TODO: add your keys below*/

	/*Used for software function, not physical connection!*/

};

static struct matrix_keymap_data hisik3_keymap_data = {
	.keymap = default_keymap,
	.keymap_size = ARRAY_SIZE(default_keymap),
};
static uint16_t long_func_key1[] = {KEY_BACK};
static uint16_t long_func_key2[] = {DPAD_CENTER, KEY_VOLUMEDOWN};

static struct keypad_remap_item remap_items[] = {
	{KEY_HOME, 1, 1000/*ms*/, long_func_key1},
	/*{KEY_A, 2, 500, long_func_key2},*/
	/*TODO: add your remap_item here*/
};

static struct keypad_remap keypad_long_remap = {
	.count = ARRAY_SIZE(remap_items),
	.items = remap_items,
};

static struct resource hisik3_keypad_resources[] = {
	[0] = {
		.start = REG_BASE_KPC,
		.end = REG_BASE_KPC + REG_KPC_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_KPC,
		.end = IRQ_KPC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct k3v2_keypad_platdata hisik3_keypad_platdata = {
	.keymap_data = &hisik3_keymap_data,
	.keypad_remap = &keypad_long_remap,
	.rows = 8,
	.cols = 8,
	.row_shift = 3,
};

static struct platform_device hisik3_keypad_device = {
	.name = "k3_keypad",
	.id = -1,
	.num_resources = ARRAY_SIZE(hisik3_keypad_resources),
	.resource = hisik3_keypad_resources,
	.dev.platform_data = &hisik3_keypad_platdata,
};

/* Keypad device and platform data start, use GPIO realizing keypad. */

static struct resource hisik3_gpio_keypad_resources[] = {
	[0] = {
		.start = REG_BASE_GPIO18,
		.end = REG_BASE_GPIO18 + REG_GPIO18_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_GPIO(GPIO_17_1),
		.end = IRQ_GPIO(GPIO_17_1),
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_GPIO(GPIO_17_2),
		.end = IRQ_GPIO(GPIO_17_2),
		.flags = IORESOURCE_IRQ,
	},
#ifndef CONFIG_MATE_NFC_GPIO
	[3] = {
		.start = IRQ_GPIO(GPIO_17_6),
		.end = IRQ_GPIO(GPIO_17_6),
		.flags = IORESOURCE_IRQ,
	},
	[4] = {
		.start = IRQ_GPIO(GPIO_17_7),
		.end = IRQ_GPIO(GPIO_17_7),
		.flags = IORESOURCE_IRQ,
	},
#endif
};

static struct platform_device hisik3_gpio_keypad_device = {
	.name = "k3v2_gpio_key",
	.id = -1,
	.num_resources = ARRAY_SIZE(hisik3_gpio_keypad_resources),
	.resource = hisik3_gpio_keypad_resources,
};

/*power-key*/
static struct resource hisik3_power_key_resources[] = {
	[0] = {
		.start = REG_BASE_PMUSPI,
		.end = REG_BASE_PMUSPI + REG_PMUSPI_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_POWER_KEY_PRESS,
		.end = IRQ_POWER_KEY_PRESS,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_POWER_KEY_RELEASE,
		.end = IRQ_POWER_KEY_RELEASE,
		.flags = IORESOURCE_IRQ,
	},
	[3] = {
		.start = IRQ_POWER_KEY_LONG_PRESS_1S,
		.end = IRQ_POWER_KEY_LONG_PRESS_1S,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device hisik3_power_key_device = {
	.name = "k3v2_power_key",
	.id = -1,
	.num_resources = ARRAY_SIZE(hisik3_power_key_resources),
	.resource = hisik3_power_key_resources,
};

/*watchdog added by s00212129*/
static struct resource  hisik3_watchdog_resources[] = {

         [0] = {
                 .start = REG_BASE_WD,
                 .end = REG_BASE_WD + REG_WD_IOSIZE -1,
                 .flags = IORESOURCE_MEM,
         },

         [1] = {
                 .start = IRQ_WDOG,
                 .end   = IRQ_WDOG,
                 .flags = IORESOURCE_IRQ,
         },
 };

 static struct platform_device  hisik3_watchdog_device = {
         .name = "k3v2_watchdog",
         .id = -1,
         .num_resources = ARRAY_SIZE(hisik3_watchdog_resources),
         .resource = hisik3_watchdog_resources,
 };
 /*end watchdog*/


/*Keypad backlight*/
static struct platform_device hisik3_keypad_backlight_device = {
	.name = "keyboard-backlight",
};

/* TouchScreen start*/
#ifdef CONFIG_TOUCH_INPUT_SYNAPTICS_RMI4
static struct synaptics_rmi4_platform_data synaptics_ts_platform_data = {
	.irq				= GPIO_19_5,
	.irq_flag			= IRQF_TRIGGER_LOW,
	.flip_flags			= SYNAPTICS_NO_FLIP,
	.x_res				= LCD_X_720P,
	.y_res				= LCD_Y_720P,
	.y_all				= LCD_Y_ALL_720P,
	.fuzz_x				= 0,
	.fuzz_y				= 0,
	.fuzz_p				= 0,
	.fuzz_w				= 0,
	.reset_pin			= GPIO_19_4,
};
#endif
#ifdef CONFIG_RMI4_VIRTUAL_BUTTON
static unsigned char syna_button_codes[] = {KEY_BACK,KEY_HOMEPAGE,KEY_MENU};
// virtualkey for syna driver, not used now
static struct rmi_f19_button_map syna_button_map = {
	.nbuttons = ARRAY_SIZE(syna_button_codes),
	.map = syna_button_codes,
};
static struct virtualbutton_map syna_virtualbutton_maps[] = {
	{
		.x			= 305,
		.y			= 1830,
		.width			= 155,
		.height			= 90,
		.code			= KEY_BACK,
	},
	{
		.x			= 615,
		.y			= 1830,
		.width			= 155,
		.height			= 90,
		.code			= KEY_HOMEPAGE,
	},
	{
		.x			= 925,
		.y			= 1830,
		.width			= 155,
		.height			= 90,
		.code			= KEY_MENU,
	},
};
static struct rmi_f11_virtualbutton_map syna_virtualbutton_map = {
	.buttons = ARRAY_SIZE(syna_virtualbutton_maps),
	.map = syna_virtualbutton_maps,
};
#else
static char buf_virtualkey[500];
static ssize_t  buf_vkey_size=0;
static ssize_t synaptics_virtual_keys_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
        memcpy( buf, buf_virtualkey, buf_vkey_size );
        return buf_vkey_size;
}

static struct kobj_attribute synaptics_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.synaptics",
		.mode = S_IRUGO,
	},
	.show = &synaptics_virtual_keys_show,
};

static struct attribute *synaptics_properties_attrs[] = {
	&synaptics_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group synaptics_properties_attr_group = {
	.attrs = synaptics_properties_attrs,
};

static void __init synaptics_virtual_keys_init(void)
{
	struct kobject *properties_kobj;
	int ret = 0;

	buf_vkey_size = sprintf(buf_virtualkey,
                __stringify(EV_KEY) ":" __stringify(KEY_HOMEPAGE)  ":305:1600:155:90"
                 ":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":615:1600:155:90"
                 ":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":925:1600:155:90"
                 "\n");
	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		ret = sysfs_create_group(properties_kobj,
			&synaptics_properties_attr_group);
	if (!properties_kobj || ret)
		pr_err("%s: failed to create board_properties!\n", __func__);
}
#endif
/* TouchScreen end*/

/* Cypress cyttsp4 start */

static int cyttsp4_xres(struct cyttsp4_core_platform_data *pdata,
		struct device *dev)
{
	int rst_gpio = pdata->rst_gpio;
	int rc = 0;

	gpio_set_value(rst_gpio, 1);
	msleep(20);
	gpio_set_value(rst_gpio, 0);
	msleep(40);
	gpio_set_value(rst_gpio, 1);
	msleep(20);
	dev_info(dev,
		"%s: RESET CYTTSP gpio=%d r=%d\n", __func__,
		pdata->rst_gpio, rc);
	return rc;
}

static int cyttsp4_init(struct cyttsp4_core_platform_data *pdata,
		int on, struct device *dev)
{
	int rst_gpio = pdata->rst_gpio;
	int irq_gpio = pdata->irq_gpio;
	int rc = 0;

	if (on) {
		rc = gpio_request(rst_gpio, NULL);
		if (rc < 0) {
			gpio_free(rst_gpio);
			rc = gpio_request(rst_gpio, NULL);
		}
		if (rc < 0) {
			dev_err(dev,
				"%s: Fail request gpio=%d\n", __func__,
				rst_gpio);
		} else {
			rc = gpio_direction_output(rst_gpio, 1);
			if (rc < 0) {
				pr_err("%s: Fail set output gpio=%d\n",
					__func__, rst_gpio);
				gpio_free(rst_gpio);
			} else {
				rc = gpio_request(irq_gpio, NULL);
				if (rc < 0) {
					gpio_free(irq_gpio);
					rc = gpio_request(irq_gpio,
						NULL);
				}
				if (rc < 0) {
					dev_err(dev,
						"%s: Fail request gpio=%d\n",
						__func__, irq_gpio);
					gpio_free(rst_gpio);
				} else {
					gpio_direction_input(irq_gpio);
				}
			}
		}
	} else {
		gpio_free(rst_gpio);
		gpio_free(irq_gpio);
	}

	dev_info(dev,
		"%s: INIT CYTTSP RST gpio=%d and IRQ gpio=%d r=%d\n",
		__func__, rst_gpio, irq_gpio, rc);
	return rc;
}

static int cyttsp4_wakeup(struct cyttsp4_core_platform_data *pdata,
		struct device *dev, atomic_t *ignore_irq)
{
	int irq_gpio = pdata->irq_gpio;
	int rc = 0;

	if (ignore_irq)
		atomic_set(ignore_irq, 1);
	rc = gpio_direction_output(irq_gpio, 0);
	if (rc < 0) {
		if (ignore_irq)
			atomic_set(ignore_irq, 0);
		dev_err(dev,
			"%s: Fail set output gpio=%d\n",
			__func__, irq_gpio);
	} else {
		udelay(2000);
		rc = gpio_direction_input(irq_gpio);
		if (ignore_irq)
			atomic_set(ignore_irq, 0);
		if (rc < 0) {
			dev_err(dev,
				"%s: Fail set input gpio=%d\n",
				__func__, irq_gpio);
		}
	}

	dev_info(dev,
		"%s: WAKEUP CYTTSP gpio=%d r=%d\n", __func__,
		irq_gpio, rc);
	return rc;
}

static int cyttsp4_sleep(struct cyttsp4_core_platform_data *pdata,
		struct device *dev, atomic_t *ignore_irq)
{
	return 0;
}

static int cyttsp4_power(struct cyttsp4_core_platform_data *pdata,
		int on, struct device *dev, atomic_t *ignore_irq)
{
	if (on)
		return cyttsp4_wakeup(pdata, dev, ignore_irq);

	return cyttsp4_sleep(pdata, dev, ignore_irq);
}

static int cyttsp4_irq_stat(struct cyttsp4_core_platform_data *pdata,
		struct device *dev)
{
	return gpio_get_value(pdata->irq_gpio);
}

/* Button to keycode conversion */
static u16 cyttsp4_btn_keys[] = {
	/* use this table to map buttons to keycodes (see input.h) */
	KEY_HOME,		/* 102 */
	KEY_MENU,		/* 139 */
	KEY_BACK,		/* 158 */
	KEY_SEARCH,		/* 217 */
	KEY_VOLUMEDOWN,		/* 114 */
	KEY_VOLUMEUP,		/* 115 */
	KEY_CAMERA,		/* 212 */
	KEY_POWER		/* 116 */
};

static struct touch_settings cyttsp4_sett_btn_keys = {
	.data = (uint8_t *)&cyttsp4_btn_keys[0],
	.size = ARRAY_SIZE(cyttsp4_btn_keys),
	.tag = 0,
};

static struct cyttsp4_core_platform_data _cyttsp4_core_platform_data = {
	.irq_gpio = CYTTSP4_I2C_IRQ_GPIO,
	.rst_gpio = CYTTSP4_I2C_RST_GPIO,
	.use_configure_sensitivity = 1,
	.xres = cyttsp4_xres,
	.init = cyttsp4_init,
	.power = cyttsp4_power,
	.irq_stat = cyttsp4_irq_stat,
	.sett = {
		NULL,	/* Reserved */
		NULL,	/* Command Registers */
		NULL,	/* Touch Report */
		NULL,	/* Cypress Data Record */
		NULL,	/* Test Record */
		NULL,	/* Panel Configuration Record */
		NULL, /* &cyttsp4_sett_param_regs, */
		NULL, /* &cyttsp4_sett_param_size, */
		NULL,	/* Reserved */
		NULL,	/* Reserved */
		NULL,	/* Operational Configuration Record */
		NULL, /* &cyttsp4_sett_ddata, *//* Design Data Record */
		NULL, /* &cyttsp4_sett_mdata, *//* Manufacturing Data Record */
		NULL,	/* Config and Test Registers */
		&cyttsp4_sett_btn_keys,	/* button-to-keycode table */
	},
	.loader_pdata = &_cyttsp4_loader_platform_data,
};


static struct cyttsp4_core_info cyttsp4_core_info __initdata = {
	.name = CYTTSP4_CORE_NAME,
	.id = "main_ttsp_core",
	.adap_id = CYTTSP4_I2C_NAME,
	.platform_data = &_cyttsp4_core_platform_data,
};


static const uint16_t cyttsp4_abs[] = {
	ABS_MT_POSITION_X, CY_ABS_MIN_X, CY_ABS_MAX_X, 0, 0,
	ABS_MT_POSITION_Y, CY_ABS_MIN_Y, CY_ABS_MAX_Y, 0, 0,
	ABS_MT_PRESSURE, CY_ABS_MIN_P, CY_ABS_MAX_P, 0, 0,
	CY_IGNORE_VALUE, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
	ABS_MT_TRACKING_ID, CY_ABS_MIN_T, CY_ABS_MAX_T, 0, 0,
	ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0,
	ABS_MT_TOUCH_MINOR, 0, 255, 0, 0,
	ABS_MT_ORIENTATION, -128, 127, 0, 0,
};

struct touch_framework cyttsp4_framework = {
	.abs = (uint16_t *)&cyttsp4_abs[0],
	.size = ARRAY_SIZE(cyttsp4_abs),
	.enable_vkeys = 0,
};

static struct cyttsp4_mt_platform_data _cyttsp4_mt_platform_data = {
	.frmwrk = &cyttsp4_framework,
	.flags = 0x00, //0x38,
	.inp_dev_name = CYTTSP4_MT_NAME,
};

struct cyttsp4_device_info cyttsp4_mt_info __initdata = {
	.name = CYTTSP4_MT_NAME,
	.core_id = "main_ttsp_core",
	.platform_data = &_cyttsp4_mt_platform_data,
};

static struct cyttsp4_btn_platform_data _cyttsp4_btn_platform_data = {
	.inp_dev_name = CYTTSP4_BTN_NAME,
};

struct cyttsp4_device_info cyttsp4_btn_info __initdata = {
	.name = CYTTSP4_BTN_NAME,
	.core_id = "main_ttsp_core",
	.platform_data = &_cyttsp4_btn_platform_data,
};

#ifdef CYTTSP4_VIRTUAL_KEYS
static ssize_t cyttps4_virtualkeys_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
		__stringify(EV_KEY) ":"
		__stringify(KEY_BACK) ":1360:90:160:180"
		":" __stringify(EV_KEY) ":"
		__stringify(KEY_MENU) ":1360:270:160:180"
		":" __stringify(EV_KEY) ":"
		__stringify(KEY_HOME) ":1360:450:160:180"
		":" __stringify(EV_KEY) ":"
		__stringify(KEY_SEARCH) ":1360:630:160:180"
		"\n");
}

static struct kobj_attribute cyttsp4_virtualkeys_attr = {
	.attr = {
		.name = "virtualkeys.cyttsp4_mt",
		.mode = S_IRUGO,
	},
	.show = &cyttps4_virtualkeys_show,
};

static struct attribute *cyttsp4_properties_attrs[] = {
	&cyttsp4_virtualkeys_attr.attr,
	NULL
};

static struct attribute_group cyttsp4_properties_attr_group = {
	.attrs = cyttsp4_properties_attrs,
};
#endif

static void  huawei_cyttsp4_init(void)
{
	/* Register core and devices */
	cyttsp4_register_core_device(&cyttsp4_core_info);
	cyttsp4_register_device(&cyttsp4_mt_info);
	cyttsp4_register_device(&cyttsp4_btn_info);
}


/* Cypress cyttsp4 end */

/* Atmel mXT224E Touchscreen start*/
static struct atmel_i2c_platform_data atmel_tp_platform_data = {
	.version = 0x10,
	.source = 0,
	.abs_x_min = 0,
	.abs_x_max = 719,
	.abs_y_min = 0,
	.abs_y_max = 1279,
	.abs_pressure_min = 0,
	/*.abs_pressure_max = 15,*/
	.abs_pressure_max = 255,
	.abs_width_min = 0,
	.abs_width_max = 255,
	/*.abs_area_min = 0,*/
	/*.abs_area_max = 255,*/
	.gpio_irq = GPIO_19_5,
	.gpio_reset = GPIO_19_4,
	.power = NULL,
	.config_T6 = {
		0, 0, 0, 0, 0,
		0
	},
	.config_T7 = {
		32, 12, 25
	},
	.config_T8 = {
		24, 0, 1, 10, 0,
		0, 5, 60, 10, 192
	},
	.config_T9 = {
		139, 0, 0, 19, 11,
		0, 32, 66, 2, 3,
		0, 5, 2, 47, 10,
		15, 22, 10, 106, 5,/*XRANGE = 1386*/
		207, 2, 0, 0, 2,/* YRANGE = 719*/
		2, 161, 40, 183, 64,
		30, 20, 0, 0, 1
	},
	.config_T15 = {
		0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		0
	},
	.config_T19 = {
		0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		0
	},
	.config_T23 = {
		0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		0, 0, 0, 0, 0
	},
	.config_T25 = {
		0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		0, 0, 0, 0
	},
	.config_T40 = {
		0, 0, 0, 0, 0
	},
	.config_T42 = {
		3, 40, 40, 80, 128,
		0, 0, 0
	},
	.config_T46 = {
		0, 3, 32, 32, 0,
		0, 0, 0, 0
	},
	.config_T47 = {
		0, 20, 50, 5, 2,
		40, 40, 180, 0, 100
	},
	.config_T48 = {
		1, 4, 10, 0, 0,
		0, 0, 0, 1, 1,
		0, 0, 0, 6, 6,
		0, 0, 63, 6, 64,
		10, 0, 20, 5, 0,
		38, 0, 20, 0, 0,
		0, 0, 0, 0, 0,
		40, 2, 2, 2, 32,
		10, 12, 20, 241, 251,
		0, 0, 191, 40, 183,
		64, 30, 15, 0
	},
	.object_crc = {
		0xFD, 0x3B, 0x8D
	},/*CRC*/
	.cable_config = {
		70, 30, 32, 32
	},
	.cable_config_T7 = {
		32, 16, 25
	},
	.cable_config_T8 = {
		24, 0, 5, 5, 0,
		0, 5, 60, 10, 192
	},
	.cable_config_T9 = {
		139, 0, 0, 19, 11,
		0, 32, 66, 2, 3,
		0, 5, 2, 64, 10,
		12, 20, 10, 106, 5,/*XRANGE = 1386*/
		207, 2, 0, 0, 0,/* YRANGE = 719*/
		0, 161, 40, 183, 64,
		30, 20, 0, 0, 0
	},
	.cable_config_T46 = {
		0, 3, 40, 40, 0,
		0, 0, 0, 0
	},

	.cable_config_T48 = {
		1, 128, 114, 0, 0,
		0, 0, 0, 1, 7,
		0, 0, 0, 6, 6,
		0, 0, 63, 6, 64,
		10, 0, 20, 5, 0,
		38, 0, 20, 0, 0,
		0, 0, 0, 0, 0,
		40, 2, 2, 2, 32,
		10, 12, 20, 241, 251,
		0, 0, 191, 40, 183,
		64, 30, 15, 0
	},
	.noise_config = {
		70, 3, 35
	},
	.filter_level = {
		0, 0, 539, 539
	},
	.GCAF_level = {
		8, 16, 24, 32, 40
	},
	.ATCH_NOR = {
		0, 0, 5, 66, 10,
		192
	},
	.ATCH_NOR_20S = {
		0, 0, 255, 1, 0,
		0
	},
};
/* Atmel mXT224E Touchscreen end*/
#ifdef CONFIG_SYNAPTICS_TOUCH_KEY
static int synaptics_touchkey_gpio_setup(int configure)
{
    int retval=0;
    int led_gpio_enable;
    u16 attn_gpio_number = TOUCHKEY_INT;
    u16 tk_led_gpio_number = TOUCHKEY_LED_ENABLE;

    led_gpio_enable = get_touchkey_led_gpio_enable();

    if (configure==TK_GPIO_PROBE) 
    {
        retval = gpio_request(attn_gpio_number, "syp_tk_attn");
        if (retval) {
            pr_err("%s: Failed to get attn gpio %d. Code: %d.",
                   __func__, attn_gpio_number, retval);
            return retval;
        }
        retval = gpio_direction_input(attn_gpio_number);
        if (retval) {
            pr_err("%s: Failed to setup attn gpio %d. Code: %d.",
                   __func__, attn_gpio_number, retval);
            gpio_free(attn_gpio_number);
        }
        
        if(E_TOUCHKEY_KEY_LED_GPIO_CONTROL==led_gpio_enable)
        {
            printk("synaptics_touchkey_gpio_setup: set gpio %d\n",tk_led_gpio_number);
            retval = gpio_request(tk_led_gpio_number, "syp_tk_led");
            if (retval) {
                pr_err("%s: Failed to get tk_led gpio %d. Code: %d.",
                       __func__, tk_led_gpio_number, retval);
                return retval;
            }
            retval = gpio_direction_output(tk_led_gpio_number,1);
            if (retval) {
                pr_err("%s: Failed to setup tk_led gpio %d. Code: %d.",
                       __func__, tk_led_gpio_number, retval);
                gpio_free(tk_led_gpio_number);
            }
        }
        else if(E_TOUCHKEY_KEY_LED_GPIO_NONE==led_gpio_enable)
        {
            pr_info("%s: E_TOUCHKEY_KEY_LED_GPIO_NONE\n",__func__);
        }
        else
        {
            pr_err("%s: led_gpio_enable=%d\n",__func__,led_gpio_enable);
        }

    } 
    else if(configure==TK_GPIO_RESUME) 
    {
        printk("%s(TK_GPIO_RESUME) entor\n",__func__);
        if(E_TOUCHKEY_KEY_LED_GPIO_CONTROL==led_gpio_enable)
        {
            retval = gpio_direction_output(tk_led_gpio_number,1);
            if (retval) {
                pr_err("%s: Failed to setup tk_led gpio %d. Code: %d.",
                       __func__, tk_led_gpio_number, retval);
                gpio_free(tk_led_gpio_number);
            }
        }
    }
    else if(configure==TK_GPIO_SUSPEND) 
    {
        printk("%s(TK_GPIO_SUSPEND) entor\n",__func__);
        if(E_TOUCHKEY_KEY_LED_GPIO_CONTROL==led_gpio_enable)
        {
            retval = gpio_direction_output(tk_led_gpio_number,0);
            if (retval) {
                pr_err("%s: Failed to setup tk_led gpio %d. Code: %d.",
                       __func__, tk_led_gpio_number, retval);
                gpio_free(tk_led_gpio_number);
            }
        }
    }
    else
    {
        pr_warn("%s: No way to deconfigure gpio %d.",
               __func__, attn_gpio_number);
    }
    return retval;
};

static unsigned short syn_touchkey_keycode[] =
{
	KEY_MENU,//139
	KEY_HOMEPAGE,//172	
	KEY_BACK,//158
};
unsigned short* get_key_map(void)
{
	return syn_touchkey_keycode;
}
static struct synatics_touchkey_platform syp_touchkey = {
	.attn_gpio = TOUCHKEY_INT,
    .get_key_map = get_key_map,
    .touchkey_gpio_config = synaptics_touchkey_gpio_setup,
};
#endif

#ifdef CONFIG_TOUCHSCREEN_RMI4_SYNAPTICS_GENERIC
struct syna_gpio_data {
    u16 attn_gpio_number;
    u16 reset_gpio_number;
    char* attn_gpio_name;
    char* reset_gpio_name;
};
static int synaptics_touchpad_gpio_setup(void *gpio_data, bool configure)
{
    int retval=0;
    struct syna_gpio_data *data = gpio_data;

    if (configure) {
        retval = gpio_request(data->attn_gpio_number, "rmi4_attn");
        if (retval) {
            pr_err("%s: Failed to get attn gpio %d. Code: %d.",
                   __func__, data->attn_gpio_number, retval);
            return retval;
        }
        retval = gpio_direction_input(data->attn_gpio_number);
        if (retval) {
            pr_err("%s: Failed to setup attn gpio %d. Code: %d.",
                   __func__, data->attn_gpio_number, retval);
            gpio_free(data->attn_gpio_number);
        }
#ifndef CONFIG_P2_TP_TK_CMD_FEATURE
        retval = gpio_request(data->reset_gpio_number, "rmi4_reset");
        if (retval) {
            pr_err("%s: Failed to get reset gpio %d. Code: %d.",
                   __func__, data->reset_gpio_number, retval);
            return retval;
        }
        retval = gpio_direction_output(data->reset_gpio_number,0);
        if (retval) {
            pr_err("%s: Failed to setup reset gpio %d. Code: %d.",
                   __func__, data->reset_gpio_number, retval);
            gpio_free(data->reset_gpio_number);
        }
        msleep (10);
        retval = gpio_direction_output(data->reset_gpio_number,1);
        if (retval) {
            pr_err("%s: Failed to setup reset gpio %d. Code: %d.",
                   __func__, data->reset_gpio_number, retval);
            gpio_free(data->reset_gpio_number);
        }
        msleep(10);
        gpio_free(data->reset_gpio_number);
#endif
    } else {
        pr_warn("%s: No way to deconfigure gpio %d and %d.",
               __func__, data->attn_gpio_number,data->reset_gpio_number);
    }
    return retval;
};
static struct syna_gpio_data rmi4_gpiodata = {
	    .attn_gpio_number = TOUCH_INT_PIN,
	    .reset_gpio_number = TOUCH_RESET_PIN,
	    .attn_gpio_name = "rmi4_attn",
	    .reset_gpio_name = "rmi4_reset",
	};
static struct rmi_device_platform_data syna_platformdata ={
    .driver_name = "rmi_generic",
    .sensor_name = "RMI4",
    .attn_gpio = TOUCH_INT_PIN,
    .attn_polarity = 0,
    .level_triggered = true,
    .gpio_data = &rmi4_gpiodata,
    .gpio_config = synaptics_touchpad_gpio_setup,
    .reset_delay_ms = 100,
#ifdef CONFIG_P2_TP_TK_CMD_FEATURE 
    .axis_align = {
    .flip_x = false,
    .flip_y = false,},
#else
    .axis_align = {
    .flip_x = false,
    .flip_y = true,},
#endif
#ifdef CONFIG_RMI4_VIRTUAL_BUTTON
    .button_map = &syna_button_map,
    .virtualbutton_map = &syna_virtualbutton_map,
#endif
};
#endif

/*NFC pn544 gpio setup*/
#ifdef CONFIG_MATE_NFC_GPIO
static int nfc_gpio_setup (void *gpio_data, bool configure) {
        int retval = 0;

	 retval = gpio_request(NFC_CLK_REQ, "nfc_clk_req");
        if (retval) {
            pr_err("%s: Failed to get nfc_clk_req gpio %d. Code: %d.",
                   __func__, NFC_CLK_REQ, retval);
            return retval;
        }

        retval = gpio_direction_input(NFC_CLK_REQ);
        if (retval) {
            pr_err("%s: Failed to setup nfc_clk_req gpio %d. Code: %d.",
                   __func__, NFC_CLK_REQ, retval);
            gpio_free(NFC_CLK_REQ);
        }

        retval = gpio_request(NFC_FIRM_DLOAD, "nfc_firm");
        if (retval) {
            pr_err("%s: Failed to get nfc_firm gpio %d. Code: %d.",
                   __func__, NFC_FIRM_DLOAD, retval);
            return retval;
        }

        retval = gpio_direction_output(NFC_FIRM_DLOAD,0);
        if (retval) {
            pr_err("%s: Failed to setup nfc_firm gpio %d. Code: %d.",
                   __func__, NFC_FIRM_DLOAD, retval);
            gpio_free(NFC_FIRM_DLOAD);
        }


        retval = gpio_request(NFC_1V8EN, "nfc_ven");
        if (retval) {
            pr_err("%s: Failed to get nfc_ven gpio %d. Code: %d.",
                   __func__, NFC_1V8EN, retval);
            return retval;
        }

        retval = gpio_direction_output(NFC_1V8EN,0);
        if (retval) {
            pr_err("%s: Failed to setup nfc_ven gpio %d. Code: %d.",
                   __func__, NFC_1V8EN, retval);
            gpio_free(NFC_1V8EN);
        }

        msleep (20);
	retval = gpio_direction_output(NFC_1V8EN,1);
        if (retval) {
            pr_err("%s: Failed to setup nfc_ven gpio %d. Code: %d.",
                   __func__, NFC_1V8EN, retval);
            gpio_free(NFC_1V8EN);
        }
        msleep (10);

      return retval;
}


static struct pn544_i2c_platform_data pn544_platform_data = {   

	.irq_gpio = NFC_IRQ_MATE,
	.ven_gpio = NFC_1V8EN,	
	.firm_gpio = NFC_FIRM_DLOAD,    
       .gpio_config = nfc_gpio_setup,
};


#else
static int nfc_gpio_setup (void *gpio_data, bool configure) {
        int retval = 0;
        pr_err("%s: enter P2 GPIO config\n", __func__);
	    retval = gpio_request(NFC_CLK_REQ, "nfc_clk_req");
        if (retval) {
            pr_err("%s: Failed to get nfc_clk_req gpio %d. Code: %d.",
                   __func__, NFC_CLK_REQ, retval);
            return retval;
        }

        retval = gpio_direction_input(NFC_CLK_REQ);
        if (retval) {
            pr_err("%s: Failed to setup nfc_clk_req gpio %d. Code: %d.",
                   __func__, NFC_CLK_REQ, retval);
            gpio_free(NFC_CLK_REQ);
        }

        retval = gpio_request(NFC_FIRM_DLOAD, "nfc_firm");
        if (retval) {
            pr_err("%s: Failed to get nfc_firm gpio %d. Code: %d.",
                   __func__, NFC_FIRM_DLOAD, retval);
            return retval;
        }

        retval = gpio_direction_output(NFC_FIRM_DLOAD,0);
        if (retval) {
            pr_err("%s: Failed to setup nfc_firm gpio %d. Code: %d.",
                   __func__, NFC_FIRM_DLOAD, retval);
            gpio_free(NFC_FIRM_DLOAD);
        }


        retval = gpio_request(NFC_1V8EN, "nfc_ven");
        if (retval) {
            pr_err("%s: Failed to get nfc_ven gpio %d. Code: %d.",
                   __func__, NFC_1V8EN, retval);
            return retval;
        }

	    retval = gpio_direction_output(NFC_1V8EN,0);
        if (retval) {
            pr_err("%s: Failed to setup nfc_ven gpio %d. Code: %d.",
                   __func__, NFC_1V8EN, retval);
            gpio_free(NFC_1V8EN);
        }

        msleep (20);
	    retval = gpio_direction_output(NFC_1V8EN,1);
        if (retval) {
            pr_err("%s: Failed to setup nfc_ven gpio %d. Code: %d.",
                   __func__, NFC_1V8EN, retval);
            gpio_free(NFC_1V8EN);
        }
        msleep (10);
         return retval;
}


static struct pn544_i2c_platform_data pn544_platform_data = {   

	.irq_gpio = NFC_IRQ_P2,
	.ven_gpio = NFC_1V8EN,	
	.firm_gpio = NFC_FIRM_DLOAD,    
    .gpio_config = nfc_gpio_setup,
};

#endif


/* Audience */

static struct es305_platform_data audience_platform_data = {
	.gpio_es305_wakeup  = GPIO_16_0,/* 128 */
	.gpio_es305_reset   = GPIO_18_1,/* 145 */
};

/* TPA2028_SPK_L */
static struct tpa2028_l_platform_data tpa2028_l_pdata = {
	.gpio_tpa2028_en    = GPIO_17_3,/* 139 */
};

/* TPA2028_SPK_R */
static struct tpa2028_r_platform_data tpa2028_r_pdata = {
    //.gpio_tpa2028_en    = GPIO_14_5,/* 117 */
};

/* TPA6132 */
static struct tpa6132_platform_data tpa6132_pdata = {
	.gpio_tpa6132_en    = GPIO_21_0,/* 168 */
};

static struct platform_device tpa6132_device = {
	.name    = TPA6132_NAME,
	.id      = 0,
	.dev     = {
		.platform_data = &tpa6132_pdata,
	},
};

/* SPK_5VBOOST */
static struct spk_5vboost_platform_data spk_5vboost_pdata = {
	.gpio_5vboost_en    = GPIO_17_3,/* 139 */
};

static struct platform_device spk_5vboost_device = {
	.name    = SPK_5VBOOST_NAME,
	.id      = 0,
	.dev     = {
		.platform_data = &spk_5vboost_pdata,
	},
};

static struct tps61310_platform_data tps61310_platform_data =
{
	.reset_pin			= GPIO_9_4,
	.strobe0			= GPIO_8_1,
	.strobe1			= GPIO_8_2,
};

static struct platform_device boardid_dev ={
    .name    = "boardid_dev",
    .id      = 0,
};

//modem audio path switch control
static struct modemctl_platform_data modem_switch_pdata = {
	.gpio_modemctl_en    = GPIO_13_2,/* 106 */
};
static struct platform_device modem_switch_device = {
	.name    = MODEMCTL_NAME,
	.id      = 0,
	.dev     = {
		.platform_data = &modem_switch_pdata,
	},
};
static int hi6421_batt_table[] = {
	/* adc code for temperature in degree C */
	929, 925, /* -2 ,-1 */
	920, 917, 912, 908, 904, 899, 895, 890, 885, 880, /* 00 - 09 */
	875, 869, 864, 858, 853, 847, 841, 835, 829, 823, /* 10 - 19 */
	816, 810, 804, 797, 790, 783, 776, 769, 762, 755, /* 20 - 29 */
	748, 740, 732, 725, 718, 710, 703, 695, 687, 679, /* 30 - 39 */
	671, 663, 655, 647, 639, 631, 623, 615, 607, 599, /* 40 - 49 */
	591, 583, 575, 567, 559, 551, 543, 535, 527, 519, /* 50 - 59 */
	511, 504, 496 /* 60 - 62 */
};


static struct bq_bci_platform_data hi6421_bci_data = {
    .termination_currentmA = 100,
    .monitoring_interval   = 10,
    .max_charger_currentmA = 1200,
    .max_charger_voltagemV = 4200,
    .max_bat_voltagemV     = 4200,
    .low_bat_voltagemV     = 3300,
    .battery_tmp_tbl       = hi6421_batt_table,
    .tblsize     = ARRAY_SIZE(hi6421_batt_table),
};

static struct resource hisik3_battery_resources[] = {
    [0] = {
        .start  = IRQ_VBATLOW_RISING,
        .name   = NULL,
        .flags  = IORESOURCE_IRQ,
    } ,
};

static struct platform_device hisik3_battery_monitor = {
    .name  = "bq_bci_battery",
    .id = 1,
    .resource = hisik3_battery_resources,
    .num_resources = ARRAY_SIZE(hisik3_battery_resources),
    .dev = {
        .platform_data = &hi6421_bci_data,
    },
};

static struct bq2419x_platform_data bq2419x_data =
{
    .max_charger_currentmA = 1000,
    .max_charger_voltagemV = 4200,
    .max_cin_limit_currentmA = 1200,
    .gpio = ENABLE_BQ2419x_CHARGER,
};
static struct nct203_platform_data nct203_data = {
    .alert_limit   = 0x55,
    .throttling_limit   = 0x55,
    .hysteresis             = 0x0A,
    .supported_hwrev        = true,
    .conv_rate              = 0x08,
};
#if 0
static struct k3_battery_monitor_platform_data hi6421_bci_data = {
	.termination_currentmA		=CIN_LIMIT_100,
	.monitoring_interval		= MONITOR_TIME,
	.max_charger_currentmA		= HIGH_CURRENT,
	.max_charger_voltagemV		= HIGH_VOL,
	.max_bat_voltagemV		= BAT_FULL_VOL,
	.low_bat_voltagemV		= BAT_SLOW_VOL,
	.battery_tmp_tbl		= hi6421_batt_table,
	.tblsize			= ARRAY_SIZE(hi6421_batt_table),
};

static struct resource hisik3_battery_resources[] = {
	[0] = {
		.start  = IRQ_VBATLOW_RISING,
		.name   = NULL,
		.flags  = IORESOURCE_IRQ,
	} ,
};

static struct platform_device hisik3_battery_monitor = {
	.name	= "k3_battery_monitor",
	.id	= 1,
	.resource	= hisik3_battery_resources,
	.num_resources	= ARRAY_SIZE(hisik3_battery_resources),
	.dev = {
		.platform_data	= &hi6421_bci_data,
	},
};

static struct k3_bq24161_platform_data k3_bq24161_data =
{
	.max_charger_currentmA = 1000,
	.max_charger_voltagemV = 4200,
	.gpio = BQ24161_GPIO_074,
};
#endif

#ifdef MHL_SII9244
static struct mhl_platform_data k3_mhl_data =
{
	.gpio_reset 	= MHL_GPIO_RESET,
	.gpio_wake_up	= MHL_GPIO_WAKE_UP,
	.gpio_int		= MHL_GPIO_INT,
};
#endif

/* please add i2c bus 0 devices here */
static struct i2c_board_info hisik3_i2c_bus0_devs[]= {
	/* camera tps61310 light */
#ifdef CONFIG_HIK3_CAMERA_FLASH
	[0]	=	{
		.type			= K3_FLASH_NAME,
		.addr			= K3_FLASH_I2C_ADDR,
		.platform_data		= &tps61310_platform_data,
	},
#endif
	[1]	=	{
		.type			= "nct203",
		.addr			= 0x4C,
		.platform_data 	= &nct203_data,
		.irq				= GPIO_5_6,
	},

	/*TODO: add your device here*/
};

static struct i2c_board_info hisik3_i2c_bus0_tfa9887[] = {
   [0] =   {
		.type			= TFA9887_NAME,
		.addr			= TFA9887_I2C_ADDR,
		.flags 			= true,
	},

	/*TODO: add your device here*/
};
static struct i2c_board_info hisik3_i2c_bus1_tpa2028_l[]= {
	[0] =   {
		.type			= TPA2028_L_NAME,
		.addr			= TPA2028_I2C_ADDR,
		.flags 			= true,
		.platform_data 	= &tpa2028_l_pdata,
	},

	/*TODO: add your device here*/
};

/* please add i2c bus 1 devices here */
static struct i2c_board_info hisik3_i2c_bus1_devs[]= {
	[0]	=	{
		.type = "bq2419x_charger",
		.addr = I2C_ADDR_BQ24192,
		.platform_data = &bq2419x_data,
		.irq = GPIO_0_5,
	},
#if 0
	[0]	=	{
		.type			= "k3_bq24161_charger",
		.addr			= I2C_ADDR_BQ24161,
		.platform_data 	= &k3_bq24161_data,
		.irq				= GPIO_0_5,
	},
#endif
	[1]	=	{
		.type			= "bq27510-battery",
		.addr			= I2C_ADDR_BQ27510,
		.platform_data 	= NULL,
		.irq				= GPIO_21_2,
	},
#ifdef MHL_SII8240
        [2]	=	{
        	.type			= "Sil-8240",
        	.addr			= 0x3B,
        	.flags                     = I2C_CLIENT_WAKE,
        	.irq                        = GPIO_10_0,
	},

#elif defined(MHL_SII9244)
	[2]	=	{
		.type			= "mhl_Sii9244_page0",
		.addr			= MHL_SII9244_PAGE0_ADDR,
		.platform_data 		= &k3_mhl_data,
	},
	[3]	=	{
		.type			= "mhl_Sii9244_page1",
		.addr			= MHL_SII9244_PAGE1_ADDR,
		.platform_data 		= NULL,
	},
	[4]	=	{
		.type			= "mhl_Sii9244_page2",
		.addr			= MHL_SII9244_PAGE2_ADDR,
		.platform_data 		= NULL,
	},
	[5]	=	{
		.type			= "mhl_Sii9244_cbus",
		.addr			= MHL_SII9244_CBUS_ADDR,
		.platform_data 		= NULL,
	},
#else
/*TODO: add your device here*/
#endif
      /*add nfc i2c*/
#ifdef CONFIG_MATE_NFC_GPIO
	[6]	=	{
		.type			= NFC_NAME,
		.addr			= 0x28,
		.platform_data 	= &pn544_platform_data,
		.irq                        = GPIO_17_6,
       },
#else
	[6]	=	{
		.type			= NFC_NAME,
		.addr			= 0x28,
		.platform_data 	= &pn544_platform_data,
		.irq                        = GPIO_17_4,
       },
#endif
	[7]	=	{
		.type			= "nct203_slave",
		.addr			= 0x4C,
		.platform_data 	= &nct203_data,
		.irq				= GPIO_5_7,
		},
    /* felica cen ak6921af */
#ifdef CONFIG_HUAWEI_FEATURE_FELICA_AK6921AF    
	{
		.type			= AK6921AF_NAME, 
		.addr			= AK6921AF_I2C_ADDR,		
	},
#endif

};

static struct i2c_board_info hisik3_i2c_tp_devs[]= {
	/* Synaptics Touchscreen*/
#if 0
	[0] = {
		.type		= SYNAPTICS_RMI4_NAME,
		.addr		= SYNAPTICS_RMI4_I2C_ADDR,
		/* Multi-touch support*/
		.flags 		= true,
		.platform_data 	= &synaptics_ts_platform_data,
	},
#endif

#ifdef CONFIG_TOUCHSCREEN_RMI4_SYNAPTICS_GENERIC
	/*Synaptics Touchscreen*/
	[0]	=	{
		.type			= SYNA_NAME,
		.addr			= 0x70,
		.flags 			= true,
		.platform_data 	= &syna_platformdata,
	},
#endif
#ifdef CONFIG_SYNAPTICS_TOUCH_KEY
	[1]	=	{
        .type			= SYNAPTICS_SO340010_NAME,
		.addr			= 0x2C,
		.flags 			= true,
		.platform_data 	= &syp_touchkey,
	},
#endif
#if 0
	/* Atmel mXT224E touchscreen*/
	[1] = {
		.type			= ATMEL_MXT224E_NAME,
		.addr			= 0x4A,
		.platform_data	= &atmel_tp_platform_data,
	},

#endif
	/*TODO: add your device here*/

//CYTTSP4_USE_I2C
	[2] = {
		I2C_BOARD_INFO(CYTTSP4_I2C_NAME, CYTTSP4_I2C_TCH_ADR),
		.irq =  CYTTSP4_I2C_IRQ_GPIO,
		.platform_data = CYTTSP4_I2C_NAME,
	},
};

#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC                                        
static struct extral_dynamic_dcdc_platform_data tps6236x_platform_date ={
    .enable_gpio    = EXTRAL_DYNAMIC_DCDC_EN,                            
    .regulator_data = &extral_dynamic_dcdc_regulator,                    
};                                                                       
#endif
                                                                   
static struct i2c_board_info hisik3_i2c_bus3_devs[]= {
	[0]	=	{
        .type			= ES305_NAME,
		.addr			= 0x3E,
		.flags 			= true,
		.platform_data 	= &audience_platform_data,
	},
	#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
	[1] = {
		.type			= EXTRAL_DYNAMIC_DCDC_NAME,
		.addr			= EXTRAL_DYNAMIC_DCDC_I2C_ADDR,
		.platform_data	= &tps6236x_platform_date,
	},
	#endif

	/*TODO: add your device here*/
	[2]	=	{
        .type			= IRDA_NAME,
		.addr			= 0x4D,
		.flags 			= true,
		.platform_data 	= NULL,
	},
};

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
static struct platform_device huawei_device_detect = {
	.name = "hw-dev-detect",
	.id   =-1,
};
#endif

/* please add platform device in the struct.*/
static struct platform_device *k3v2oem1_public_dev[] __initdata = {
	&hisik3_hi6421_irq_device,
	&hisik3_adc_device,
#ifdef CONFIG_LEDS_K3_6421
	&hi6421_led_device,
#endif

#ifdef CONFIG_ANDROID_K3_VIBRATOR
	&hi6421_vibrator_device,
#endif
	&hisik3_camera_device,
	&hisik3_fake_camera_device,
	&hisik3_device_hwmon,
	&hisik3_keypad_device,
	&hisik3_keypad_backlight_device,
	&k3_lcd_device, 
	//&k3_gps_bcm_device, 
	&hisik3_battery_monitor,
//	&btbcm_device,
//	&bcm_bluesleep_device,
	&wl18xx_device,
	&st_host_wake_device,
	&btwilink_device,
	&hisik3_power_key_device,
	&tpa6132_device,
	//&spk_5vboost_device,
	&usb_switch_device,
	&boardid_dev,
         &hisik3_watchdog_device,
    &modem_switch_device,
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    &huawei_device_detect,
#endif
};

static void k3v2_i2c_devices_init(void)
{
	const char str_tpa2028[] = "TPA2028";
	const char str_tfa9887[] = "TFA9887";
	/* Register devices on I2C Bus0 and Bus1*/
	i2c_register_board_info(0, hisik3_i2c_bus0_devs,
					ARRAY_SIZE(hisik3_i2c_bus0_devs));
	i2c_register_board_info(1, hisik3_i2c_bus1_devs,
					ARRAY_SIZE(hisik3_i2c_bus1_devs));

	i2c_register_board_info(2, hisik3_i2c_tp_devs,
					ARRAY_SIZE(hisik3_i2c_tp_devs));

	i2c_register_board_info(3, hisik3_i2c_bus3_devs,
					ARRAY_SIZE(hisik3_i2c_bus3_devs));
	if(get_spk_pa(str_tpa2028) == true) {
		i2c_register_board_info(1, hisik3_i2c_bus1_tpa2028_l,
						ARRAY_SIZE(hisik3_i2c_bus1_tpa2028_l));
    }
	if(get_spk_pa(str_tfa9887) == true) {
	    int bus_id;
	    if(product_fullname("CEDGED") || product_fullname("UEDGE") || product_fullname("UEDGE_G")){
		    bus_id = 0;
		}else{
		    bus_id = 3;
		}
	    i2c_register_board_info(bus_id, hisik3_i2c_bus0_tfa9887,
					    ARRAY_SIZE(hisik3_i2c_bus0_tfa9887));
    }
}



static void __init k3v2oem1_init(void)
{
	unsigned int  index = 0;
	unsigned int  board_type;
	const char str_boost5v[] = "BOOST5V";
	edb_trace(1);
	k3v2_common_init();
	/*
	 * providing two ways of realizing keypad, one is KPC, the other is GPIO.
	 * depending on current boardid, use corresponding register device.
	 * boardid=0 means board, choose hisik3_keypad_device which is realized by KPC.
	 * boardid=1 means phone, choose hisi_gpiokeypad_device which is realized by GPIO.
	 */
	board_type = get_board_type();
	switch (board_type) {
	case E_BOARD_TYPE_U9510:
#ifdef CONFIG_LEDS_K3_6421
		hi6421_led_device.dev.platform_data = &hi6421_leds_phone;
#endif
		for( index =0; index <  ARRAY_SIZE(k3v2oem1_public_dev); index++ ) {
			if ( (struct platform_device *)(&hisik3_keypad_device) == (struct platform_device *)(k3v2oem1_public_dev[index]) ) {
				k3v2oem1_public_dev[index] = &hisik3_gpio_keypad_device;
				break;
			}
		}
		break;
	case E_BOARD_TYPE_PLATFORM:
		break;
	default:
		break;
	}

	wake_lock_init(&st_wk_lock, WAKE_LOCK_SUSPEND, "st_wake_lock");

	platform_add_devices(k3v2oem1_public_dev, ARRAY_SIZE(k3v2oem1_public_dev));

	if(get_spk_pa(str_boost5v) == true) {
		platform_device_register(&spk_5vboost_device);
	}
	k3v2_i2c_devices_init();
#ifndef CONFIG_RMI4_VIRTUAL_BUTTON
	synaptics_virtual_keys_init();
#endif
	/* Cypress touchscreen init */
	huawei_cyttsp4_init();
    
#ifdef CONFIG_DEBUG_FS
	config_debugfs_init();
	uart_output_debugfs_init();
#endif
}

static void __init k3v2_early_init(void)
{
	int chip_id = 0;
	k3v2_init_clock();
	chip_id = get_chipid();
	if (chip_id == CS_CHIP_ID) {
#ifdef CONFIG_SUPPORT_B3750000_BITRATE
	    if(get_if_use_3p75M_uart_clock())
		{
		printk(KERN_INFO"k3v2_early_init oem2, use 60M periperal clock\n");
		k3v2_clk_init_from_table(common_clk_init_table_cs_60M);
		}
		else
#endif
		k3v2_clk_init_from_table(common_clk_init_table_cs);
	} else if (chip_id == DI_CHIP_ID) {
		k3v2_clk_init_from_table(common_clk_init_table_es);
	}
}

#if 0
static void k3v2_mem_setup(void)
{
	unsigned long reserved_size;

	printk(KERN_INFO "k3v2_mem_setup\n");

	/*
	   Memory reserved for Graphic/ Dcode/EnCode
	*/
	reserved_size = hisi_get_reserve_mem_size();

	/*
	 * Memory configuration with SPARSEMEM enabled on  (see
	 * asm/mach/memory.h for more information).
	 */
	arm_add_memory(PLAT_PHYS_OFFSET, (HISI_BASE_MEMORY_SIZE - reserved_size));

	return;
}

/*
 * k3v2_mem=size1@start1[,size2@start2][,...]
 * size means memory size which larger than 512M
 */
static int __init early_k3v2_mem(char *p)
{
	unsigned long size;
	phys_addr_t start;
	char *endp = NULL;
	char *ep = NULL;

	k3v2_mem_setup();

	printk(KERN_INFO "k3v2_mem = %s\n", p);

	start = PLAT_PHYS_OFFSET + HISI_BASE_MEMORY_SIZE;
	while (*p != '\0') {
		size  = memparse(p, &endp);
		if (*endp == '@')
			start = memparse(endp + 1, &ep);

		/* oem ec1 1G memory based */
		if ((start == SZ_512M)) {
			if (size < SZ_512M)
				size = 0;
			else
				size -= SZ_512M;
		}

		arm_add_memory(start, size);

		printk(KERN_INFO "early_k3v2_mem start 0x%x size 0x%lx\n", start, size);

		if (*ep == ',')
			p = ep + 1;
		else
			break;

		printk(KERN_INFO "k3v2_mem = %s\n", p);
	}

	return 0;
}
early_param("k3v2_mem", early_k3v2_mem);
#endif

static void __init k3v2_map_io(void)
{
	printk("k3v2oem2 map io\n");
	k3v2_map_common_io();
}

MACHINE_START(K3V2OEM2, "k3v2oem1")
	.boot_params	= PLAT_PHYS_OFFSET + 0x00000100,
	.init_irq       = k3v2_gic_init_irq,
	.init_machine   = k3v2oem1_init,
	.map_io         = k3v2_map_io,
	.timer          = &k3v2_timer,
	.init_early 	= k3v2_early_init,
MACHINE_END
