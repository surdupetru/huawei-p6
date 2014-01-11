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
#include <linux/synaptics_i2c_rmi4.h>

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
#include <mach/k3v2_keypad_backlight.h>
#include <linux/hkadc/hiadc_hal.h>

#include "board.h"
#include "clock.h"
#include "k3v2_clocks_init_data.h"


#define	GPIO_BT_EN					(GPIO_20_5)
#define	GPIO_BT_RST					(GPIO_20_4)
#define	GPIO_BT_VDD					(GPIO_21_6)


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

/* Added by d59977 for BCM GPS, for FPGA */
#define GPIO_GPS_BCM_EN    (GPIO_20_6)  /*166*/
#define GPIO_GPS_BCM_RET   (GPIO_20_7)	/*167*/
#define GPIO_GPS_BCM_POWER (GPIO_21_5)  /*173*/
/* End: Added by d59977 for BCM GPS */

/* Begin: Added by d59977 for BCM GPS, for FPGA */
#define GPIO_GPS_BCM_EN_NAME    "gpio_gps_bcm_enable"
#define GPIO_GPS_BCM_RET_NAME   "gpio_gps_bcm_rest"
#define GPIO_GPS_BCM_POWER_NAME "gpio_gps_bcm_power"
/* End: Added by d59977 for BCM GPS */

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
		.vol = ADC_VOLTAGE_MOD1,
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
	.id = 1,
	.dev = {
		.init_name = "k3_dev_lcd",
	},
};

/* Begin: Added by d59977 for BCM GPS, for FPGA */
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
	[2] = {
	.name  = GPIO_GPS_BCM_POWER_NAME,
	.start = GPIO_GPS_BCM_POWER,
	.end   = GPIO_GPS_BCM_POWER,
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

static struct resource bluepower_resources[] = {
	{
		.name	= "bt_gpio_enable",
		.start	= GPIO_BT_EN,
		.end	= GPIO_BT_EN,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "bt_gpio_rst",
		.start	= GPIO_BT_RST,
		.end	= GPIO_BT_RST,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "bt_vdd_enable",
		.start	= GPIO_BT_VDD,
		.end	= GPIO_BT_VDD,
		.flags	= IORESOURCE_IO,
	},

};

static struct platform_device btbcm_device = {
	.name =	"bt_power",
	.dev  =	{
		.platform_data = NULL,
	},
	.id	= -1,
	.num_resources	= ARRAY_SIZE(bluepower_resources),
	.resource	= bluepower_resources,

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
	},
	*/
};

/* Keypad device and platform data start. */
static const uint32_t default_keymap[] = {
	/*row, col, key*/
	KEY(0, 0, KEY_MENU),
	KEY(0, 1, KEY_UP),
        /* FIXME:For FPGA platform, using camera key replace power key */
	/*
	KEY(0, 1, KEY_VOLUMEUP),
	KEY(0, 2, KEY_CAMERA_FOCUS),
	*/
	KEY(0, 2, KEY_POWER),
	KEY(1, 0, KEY_BACK),
	KEY(1, 1, KEY_DOWN),
	/*
	KEY(1, 1, KEY_VOLUMEDOWN),
	KEY(1, 2, KEY_CAMERA),
	*/
	KEY(1, 2, KEY_POWER),
	KEY(2, 0, KEY_LEFT),
	/*
	KEY(2, 0, KEY_SEND),
	KEY(2, 1, KEY_END),
	*/
	KEY(2, 1, KEY_RIGHT),
	KEY(2, 2, DPAD_CENTER),
	/* TODO: add your keys below*/

	/*Used for software function, not physical connection!*/
	KEY(7, 7, KEY_HOME),
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

/*Keypad backlight*/
static struct platform_device hisik3_keypad_backlight_device = {
	.name = "k3_keypad_backlight",
};

/* TouchScreen start*/
static struct synaptics_rmi4_platform_data synaptics_ts_platform_data = 
{
	.irq				= GPIO_19_4,
	.irq_flag			= IRQF_TRIGGER_LOW,
	.flip_flags			= SYNAPTICS_NO_FLIP,
	.x_res				= LCD_X_720P,
	.y_res				= LCD_Y_720P,
	.y_all				= LCD_Y_ALL_720P,
	.fuzz_x				= 0,
	.fuzz_y				= 0,
	.fuzz_p				= 0,
	.fuzz_w				= 0,
	.reset_pin			= GPIO_19_3,
};

static ssize_t synaptics_virtual_keys_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	/*FIXME: change coordinates value for 720p screen*/
	return sprintf(buf,
	     __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":50:520:80:20"
        ":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":160:520:80:20"
        ":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":270:520:80:20"
        "\n");
}

static struct kobj_attribute synaptics_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.synaptics_rmi4",
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

	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		ret = sysfs_create_group(properties_kobj, &synaptics_properties_attr_group);
	if (!properties_kobj || ret)
		pr_err("%s: failed to create board_properties!\n", __func__);
}
/* TouchScreen end*/

/* please add i2c bus 0 devices here */
static struct i2c_board_info hisik3_i2c_bus0_devs[]= { 
	/*TODO: add your device here*/

};

/* please add i2c bus 1 devices here */
static struct i2c_board_info hisik3_i2c_bus1_devs[]= {
	/* Synaptics Touchscreen*/
	[0]	=	{
		.type			= SYNAPTICS_RMI4_NAME,
		.addr			= SYNAPTICS_RMI4_I2C_ADDR,
		.flags 			= true, 	/* Multi-touch support*/
		.platform_data 	= &synaptics_ts_platform_data,
	},

	/*TODO: add your device here*/
};

/* please add platform device in the struct.*/
static struct platform_device *k3v2fpga_public_dev[] __initdata = {
	&hisik3_hi6421_irq_device,
	&hisik3_adc_device,
	&hisik3_camera_device,
	&hisik3_fake_camera_device,
	&hisik3_device_hwmon,
	&hisik3_keypad_device,
	&hisik3_keypad_backlight_device,
	&k3_lcd_device, 
	&k3_gps_bcm_device, 
	&btbcm_device,
};

extern void (*k3v2_reset)(char mode, const char *cmd);
static void tc45msu3__reset(char mode, const char *cmd)
{
	BUG();
}

static void k3v2_i2c_devices_init()
{
	/* Register devices on I2C Bus0 and Bus1*/
	i2c_register_board_info(0, hisik3_i2c_bus0_devs, ARRAY_SIZE(hisik3_i2c_bus0_devs));
	i2c_register_board_info(1, hisik3_i2c_bus1_devs, ARRAY_SIZE(hisik3_i2c_bus1_devs));
}

static void __init tc45msu3_init(void)
{
	edb_trace(1);
	k3v2_reset = tc45msu3__reset;
	k3v2_common_init();
	platform_add_devices(k3v2fpga_public_dev, ARRAY_SIZE(k3v2fpga_public_dev));
	
	k3v2_i2c_devices_init();
	synaptics_virtual_keys_init();

}

static void __init k3v2_early_init(void)
{
	int chip_id = 0;

	k3v2_init_clock();
	chip_id = get_chipid();
	if (chip_id == CS_CHIP_ID) {
		k3v2_clk_init_from_table(common_clk_init_table_cs);
	} else if (chip_id == DI_CHIP_ID) {
		k3v2_clk_init_from_table(common_clk_init_table_es);
	}
}

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
	char *endp;
	char *ep;

	k3v2_mem_setup();

	printk(KERN_INFO "k3v2_mem = %s\n", p);

	start = PLAT_PHYS_OFFSET + HISI_BASE_MEMORY_SIZE;
	while (*p != '\0') {
		size  = memparse(p, &endp);
		if (*endp == '@')
			start = memparse(endp + 1, &ep);

		arm_add_memory(start, size);

		printk("early_k3v2_mem start 0x%x size 0x%lx\n", start, size);

		if (*ep == ',')
			p = ep + 1;
		else
			break;

		printk(KERN_INFO "k3v2_mem = %s\n", p);
	}

	return 0;
}
early_param("k3v2_mem", early_k3v2_mem);

static void __init k3v2_map_io(void)
{
	k3v2_map_common_io();
}

MACHINE_START(TC45MSU3, "tc45msu3")
	.boot_params  = PLAT_PHYS_OFFSET + 0x00000100,
	.init_irq     = k3v2_gic_init_irq,
	.init_machine = tc45msu3_init,
	.map_io       = k3v2_map_io,
	.timer        = &k3v2_timer,
	.init_early   = k3v2_early_init,
MACHINE_END
