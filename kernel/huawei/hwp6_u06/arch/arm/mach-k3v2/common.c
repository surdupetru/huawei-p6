/*
 * arch/arm/mach-k3v2/common.c
 *
 * Copyright (C) 2011 Hisilicon Co. Ltd.
 *
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
#include <linux/amba/bus.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/system.h>
#include <asm/pmu.h>
#include <asm/setup.h>
#include <mach/system.h>
#include <mach/platform.h>
#include <mach/lm.h>
#include <mach/irqs.h>
#include <mach/io.h>
#include <linux/amba/pl061.h>
#include <mach/early-debug.h>
#include <mach/mmc.h>
#include <mach/dma.h>
#include <linux/amba/serial.h>
#include <linux/i2c/designware.h>
#include <linux/amba/pl022.h>
#include <linux/spi/spi.h>
#include <mach/board-hi6421-regulator.h>
#include <mach/k3v2_regulators_vcc.h>
#include <linux/mux.h>
#include <linux/delay.h>
#ifdef CONFIG_MFD_HI6421_IRQ
#include <sound/hi6421_common.h>
#endif
#include <mach/usb_phy.h>
#include <mach/boardid.h>

#include <mach/product_feature_sel.h>

#ifdef CONFIG_MODEM_BOOT
#ifdef CONFIG_XMM_POWER
#include <mach/xmm_power.h>
#endif
#ifdef CONFIG_BALONG_POWER
#include <mach/balong_power.h>
#endif
#ifdef CONFIG_MODEM_BOOT_QSC6085
#include <mach/modem_boot_qsc6085.h>
#endif
#ifdef CONFIG_MODEM_BOOT_MTK6252
#include <mach/modem_boot_mtk6252.h>
#endif
#ifdef CONFIG_MODEM_BOOT_SPRD8803G
#include <mach/modem_boot_sprd8803g.h>
#endif
#endif

#include <mach/gpio.h>
#include <mach/hisi_mem.h>
#include <hsad/config_mgr.h>
#include <mach/debugled.h>

#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
#include "k3v2_dcdc_gpu.h"
#endif
#ifdef CONFIG_THERMAL_FRAMEWORK
#include <linux/temperature_sensor.h>
#endif

#ifdef CONFIG_MODEM_BOOT
const char* const modem_state_str[] = {
    "MODEM_STATE_OFF",
    "MODEM_STATE_POWER",
    "MODEM_STATE_READY",
    "MODEM_STATE_INVALID",  //Invalid case
};
#endif  //#ifdef CONFIG_MODEM_BOOT


void (*k3v2_reset)(char mode, const char *cmd);

DEFINE_SPINLOCK(io_lock);


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
#ifdef CONFIG_SPARSEMEM
	arm_add_memory(PLAT_PHYS_OFFSET, SZ_512M);
	arm_add_memory((PLAT_PHYS_OFFSET + SZ_512M), (HISI_BASE_MEMORY_SIZE - reserved_size - SZ_512M));
#else
	arm_add_memory(PLAT_PHYS_OFFSET, (HISI_BASE_MEMORY_SIZE - reserved_size));
#endif
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
                if (ep == NULL)
                        break;

		if (*ep == ',')
			p = ep + 1;
		else
			break;

		printk(KERN_INFO "k3v2_mem = %s\n", p);
	}

	return 0;
}
early_param("k3v2_mem", early_k3v2_mem);

extern void enable_foz(void);
void __init hisik3_init_cache(void)
{
#ifdef CONFIG_CACHE_L2X0
	void __iomem *p = (void *)IO_ADDRESS(REG_BASE_L2CC);
	edb_trace(1);

	writel_relaxed(L2X0_DYNAMIC_CLK_GATING_EN|L2X0_STNDBY_MODE_EN,
		p+L2X0_POWER_CTRL);

	/*
	 * 64K * 16 Way = 1024MB
	 */
	//l2x0_init(p, 0x76070000, 0x8B000fff);
	/*Round-robin, Full line for zero disable.*/
	l2x0_init(p, 0x3c070000, 0xc2000fff);
	/*Enable write full line for zeros mode in A9*/
	enable_foz();
#endif
}
early_initcall(hisik3_init_cache);

void common_block_set(char *block_name, enum iomux_func newmode)
{
	struct iomux_block *block_tmp = NULL;
	struct block_config *config_tmp = NULL;
	int ret = -1;

	block_tmp = iomux_get_block(block_name);
	if (!block_tmp) {
		pr_err("%s: iomux_get_block(%s) error.\n",
			__func__, block_name);
		return;
	}

	config_tmp = iomux_get_blockconfig(block_name);
	if (!config_tmp) {
		pr_err("%s: iomux_get_blockconfig(%s) error.\n",
			__func__, block_name);
		return;
	}

	ret = blockmux_set(block_tmp, config_tmp, newmode);
	if (ret) {
		pr_err("%s: blockmux_set error.\n", __func__);
		return;
	}
}

/* Define DMA parameters for UART */
K3_DEF_DMA_PARAM(UART0_RX);
K3_DEF_DMA_PARAM(UART0_TX);
K3_DEF_DMA_PARAM(UART1_RX);
K3_DEF_DMA_PARAM(UART1_TX);
K3_DEF_DMA_PARAM(UART2_RX);
K3_DEF_DMA_PARAM(UART2_TX);
K3_DEF_DMA_PARAM(UART3_RX);
K3_DEF_DMA_PARAM(UART3_TX);
K3_DEF_DMA_PARAM(UART4_RX);
K3_DEF_DMA_PARAM(UART4_TX);
K3_DEF_DMA_PARAM(I2C0_RX);
K3_DEF_DMA_PARAM(I2C0_TX);
K3_DEF_DMA_PARAM(I2C1_RX);
K3_DEF_DMA_PARAM(I2C1_TX);
K3_DEF_DMA_PARAM(I2C2_RX);
K3_DEF_DMA_PARAM(I2C2_TX);
K3_DEF_DMA_PARAM(I2C3_RX);
K3_DEF_DMA_PARAM(I2C3_TX);
K3_DEF_DMA_PARAM(SPI0_RX);
K3_DEF_DMA_PARAM(SPI0_TX);
K3_DEF_DMA_PARAM(SPI1_RX);
K3_DEF_DMA_PARAM(SPI1_TX);
K3_DEF_DMA_PARAM(SPI2_RX);
K3_DEF_DMA_PARAM(SPI2_TX);

#define UART0_BLOCK_NAME            "block_uart0"
#define UART1_BLOCK_NAME            "block_uart1"
#define UART2_BLOCK_NAME            "block_uart2"
#define UART3_BLOCK_NAME            "block_uart3"
#define UART4_BLOCK_NAME            "block_uart4"
#define I2C0_BLOCK_NAME             "block_i2c0"
#define I2C1_BLOCK_NAME             "block_i2c1"
#define I2C2_BLOCK_NAME             "block_i2c2"
#define I2C3_BLOCK_NAME             "block_i2c3"
#define SPI0_BLOCK_NAME             "block_spi0"
#define SPI1_BLOCK_NAME             "block_spi1"

/*need confirm by chenya*/
#define SPI0_0_CS_BLOCK_NAME        "block_spi0_0_cs"
#define SPI0_1_CS_BLOCK_NAME        "block_spi0_1_cs"
#define SPI0_2_CS_BLOCK_NAME        "block_spi0_2_cs"
#define SPI0_3_CS_BLOCK_NAME        "block_spi0_3_cs"

#define MODULE_FUNCS_DEFINE(dev_name)				\
static void dev_name##_init(void){				\
	common_block_set(dev_name##_BLOCK_NAME, NORMAL);	\
}								\
static void dev_name##_exit(void){				\
	common_block_set(dev_name##_BLOCK_NAME, LOWPOWER);	\
}

#define I2C0_SCL_GPIO		84
#define I2C0_SDA_GPIO		85
#define I2C1_SCL_GPIO		86
#define I2C1_SDA_GPIO		87
#define I2C2_SCL_GPIO		63
#define I2C2_SDA_GPIO		64
#define I2C3_SCL_GPIO		57
#define I2C3_SDA_GPIO		58

#define MODULE_RESET_DEFINE(dev_name)					\
static void dev_name##_reset(void){					\
	int ret;							\
									\
	ret = gpio_request(dev_name##_SCL_GPIO, "I2C_SCL_GPIO");	\
        if (ret < 0) {							\
		printk(KERN_ERR "%s, gpio_request %d with"		\
			" ret: %d.\n", __func__, dev_name##_SCL_GPIO, ret);\
		goto Done;						\
        }								\
									\
	ret = gpio_direction_output(dev_name##_SCL_GPIO, 0);		\
	if (ret < 0) {							\
		printk(KERN_ERR "%s, gpio_direction_output %d with"	\
			" ret: %d.\n", __func__, dev_name##_SCL_GPIO, ret);\
		goto err_2;						\
        }								\
        udelay(5);							\
        gpio_set_value(dev_name##_SCL_GPIO, 1);				\
									\
	ret = gpio_request(dev_name##_SDA_GPIO, "I2C_SDA_GPIO");	\
        if (ret < 0) {							\
		printk(KERN_ERR "%s, gpio_request %d with"		\
			" ret: %d.\n", __func__, dev_name##_SDA_GPIO, ret);	\
		goto err_1;						\
        }								\
									\
	ret = gpio_direction_output(dev_name##_SDA_GPIO, 1);		\
	if (ret < 0) {							\
		printk(KERN_ERR "%s, gpio_direction_output %d with"	\
			" ret: %d.\n",  __func__, dev_name##_SDA_GPIO, ret);\
		goto err_1;						\
        }								\
									\
	udelay(5);							\
err_1:									\
	gpio_free(dev_name##_SCL_GPIO);					\
err_2:									\
	gpio_free(dev_name##_SDA_GPIO);					\
Done:									\
	return ret;							\
}


/* I2C delay sda 300ns */
#define I2C0_ENABLE_DELAY_SDA				0x00010001
#define I2C0_DISABLE_DELAY_SDA				0x00010000
#define I2C1_ENABLE_DELAY_SDA				0x00020002
#define I2C1_DISABLE_DELAY_SDA				0x00020000
#define I2C2_ENABLE_DELAY_SDA				0x00100010
#define I2C2_DISABLE_DELAY_SDA				0x00100000
#define I2C3_ENABLE_DELAY_SDA				0x00200020
#define I2C3_DISABLE_DELAY_SDA				0x00200000

#define I2C0_SET_DELAY_ADDR					(IO_ADDRESS(REG_BASE_PCTRL)+0x00)
#define I2C1_SET_DELAY_ADDR					(IO_ADDRESS(REG_BASE_PCTRL)+0x00)
#define I2C2_SET_DELAY_ADDR					(IO_ADDRESS(REG_BASE_PCTRL)+0x0C)
#define I2C3_SET_DELAY_ADDR					(IO_ADDRESS(REG_BASE_PCTRL)+0x0C)
#define MODULE_FUNCS_DELAY_SDA_DEFINE(dev_name)				\
static void dev_name##_delay_sda(int enable){				\
	if(enable)							\
		writel(dev_name##_ENABLE_DELAY_SDA, dev_name##_SET_DELAY_ADDR);	\
	else								\
		writel(dev_name##_DISABLE_DELAY_SDA, dev_name##_SET_DELAY_ADDR);\
}

/* I2C reset controller */
#define I2C0_RESET_CONTROLLER               0x01000000
#define I2C1_RESET_CONTROLLER               0x02000000
#define I2C2_RESET_CONTROLLER               0x10000000
#define I2C3_RESET_CONTROLLER               0x20000000
#define I2C_RESET_CONTROLLER_TIMEOUT		10

#define MODULE_FUNCS_RESET_CONTROLLER_DEFINE(dev_name)              \
static void dev_name##_reset_controller(){                  \
    u32 status;\
    u32 timeout = I2C_RESET_CONTROLLER_TIMEOUT;\
    writel(dev_name##_RESET_CONTROLLER, IO_ADDRESS(REG_BASE_SCTRL) + 0x98);     \
    status = readl(IO_ADDRESS(REG_BASE_SCTRL) + 0xA0) & dev_name##_RESET_CONTROLLER;\
    while(!status && timeout)                              \
    {                                       \
        udelay(1);                              \
        status = readl(IO_ADDRESS(REG_BASE_SCTRL) + 0xA0) & dev_name##_RESET_CONTROLLER;\
        timeout--;\
    }                                       \
    udelay(1);              \
    timeout = I2C_RESET_CONTROLLER_TIMEOUT;                                        \
    writel(dev_name##_RESET_CONTROLLER, IO_ADDRESS(REG_BASE_SCTRL) + 0x9C);     \
    status = readl(IO_ADDRESS(REG_BASE_SCTRL) + 0xA0) & dev_name##_RESET_CONTROLLER;\
    while(status && timeout)                               \
    {                                       \
        udelay(1);                              \
        status = readl(IO_ADDRESS(REG_BASE_SCTRL) + 0xA0) & dev_name##_RESET_CONTROLLER;\
	timeout--;\
    }                                       \
}

MODULE_FUNCS_DEFINE(UART0)
MODULE_FUNCS_DEFINE(UART1)
MODULE_FUNCS_DEFINE(UART2)
MODULE_FUNCS_DEFINE(UART3)
//MODULE_FUNCS_DEFINE(UART4)
MODULE_FUNCS_DELAY_SDA_DEFINE(I2C0)
MODULE_FUNCS_DELAY_SDA_DEFINE(I2C1)
MODULE_FUNCS_DELAY_SDA_DEFINE(I2C2)
MODULE_FUNCS_DELAY_SDA_DEFINE(I2C3)
MODULE_FUNCS_RESET_CONTROLLER_DEFINE(I2C0)
MODULE_FUNCS_RESET_CONTROLLER_DEFINE(I2C1)
MODULE_FUNCS_RESET_CONTROLLER_DEFINE(I2C2)
MODULE_FUNCS_RESET_CONTROLLER_DEFINE(I2C3)
MODULE_FUNCS_DEFINE(I2C0)
MODULE_FUNCS_DEFINE(I2C1)
MODULE_FUNCS_DEFINE(I2C2)
MODULE_FUNCS_DEFINE(I2C3)
MODULE_RESET_DEFINE(I2C0)
MODULE_RESET_DEFINE(I2C1)
MODULE_RESET_DEFINE(I2C2)
MODULE_RESET_DEFINE(I2C3)
MODULE_FUNCS_DEFINE(SPI0)
MODULE_FUNCS_DEFINE(SPI1)

/*need confirm by chenya*/
MODULE_FUNCS_DEFINE(SPI0_0_CS)
MODULE_FUNCS_DEFINE(SPI0_1_CS)
MODULE_FUNCS_DEFINE(SPI0_2_CS)
MODULE_FUNCS_DEFINE(SPI0_3_CS)

#define UART0_dma_filter  NULL
#define UART1_dma_filter  k3_def_dma_filter
#define UART2_dma_filter  k3_def_dma_filter
#define UART3_dma_filter  k3_def_dma_filter
#define UART4_dma_filter  k3_def_dma_filter
#define UART4_exit        NULL    
static void UART4_init(void)  
{
	common_block_set(UART4_BLOCK_NAME, NORMAL);  
}

#define K3_UART_PLAT_DATA(dev_name, flag)				\
	{								\
		.dma_filter = dev_name##_dma_filter,			\
		.dma_rx_param = &K3_DMA_PARAM_NAME(dev_name##_RX),	\
		.dma_tx_param = &K3_DMA_PARAM_NAME(dev_name##_TX),	\
		.irq_flags = flag,					\
		.init = dev_name##_init,				\
		.exit = dev_name##_exit,				\
	}

static struct amba_pl011_data uart_plat_data[] = {
	K3_UART_PLAT_DATA(UART0, 0),
	K3_UART_PLAT_DATA(UART1, 0),
	K3_UART_PLAT_DATA(UART2, 0),
	K3_UART_PLAT_DATA(UART3, 0),
	K3_UART_PLAT_DATA(UART4, 0),
};

/*
 * AMBA SSP (SPI)
 */
#define SEL_SPI0_NO_CS	(0x003c0000)
#define SEL_SPI0_0_CS	(0x003c0004)
#define SEL_SPI0_1_CS	(0x003c0008)
#define SEL_SPI0_2_CS	(0x003c0010)
#define SEL_SPI0_3_CS	(0x003c0020)

static void k3v2_spidev0_cs_set(u32 control)
{
	pr_debug("k3v2_spidev0_cs_set: control = %d\n", control);
	if (control == 0)
		writel(SEL_SPI0_0_CS, IO_ADDRESS(REG_BASE_PCTRL) + 0x00);
	else
		writel(SEL_SPI0_NO_CS, IO_ADDRESS(REG_BASE_PCTRL) + 0x00);
}

static void k3v2_spidev1_cs_set(u32 control)
{
	pr_debug("k3v2_spidev1_cs_set: control = %d\n", control);
	if (control == 0)
		writel(SEL_SPI0_1_CS, IO_ADDRESS(REG_BASE_PCTRL) + 0x00);
	else
		writel(SEL_SPI0_NO_CS, IO_ADDRESS(REG_BASE_PCTRL) + 0x00);
}

static void k3v2_spidev2_cs_set(u32 control)
{
	pr_debug("k3v2_spidev2_cs_set: control = %d\n", control);
	if (control == 0)
		writel(SEL_SPI0_2_CS, IO_ADDRESS(REG_BASE_PCTRL) + 0x00);
	else
		writel(SEL_SPI0_NO_CS, IO_ADDRESS(REG_BASE_PCTRL) + 0x00);
}

static void k3v2_spidev3_cs_set(u32 control)
{
	pr_debug("k3v2_spidev3_cs_set: control = %d\n", control);
	if (control == 0)
		writel(SEL_SPI0_3_CS, IO_ADDRESS(REG_BASE_PCTRL) + 0x00);
	else
		writel(SEL_SPI0_NO_CS, IO_ADDRESS(REG_BASE_PCTRL) + 0x00);
}

static void k3v2_spidev4_cs_set(u32 control)
{
	pr_debug("k3v2_spidev4_cs_set: control = %d\n", control);
}

#define PL022_CONFIG_CHIP(id)						\
static struct pl022_config_chip spidev##id##_chip_info = {		\
	.com_mode		= DMA_TRANSFER,				\
	.iface			= SSP_INTERFACE_MOTOROLA_SPI,		\
	.hierarchy		= SSP_MASTER,				\
	.slave_tx_disable	= 0,					\
	.rx_lev_trig		= SSP_RX_16_OR_MORE_ELEM,		\
	.tx_lev_trig		= SSP_TX_16_OR_MORE_EMPTY_LOC,		\
	.ctrl_len		= SSP_BITS_8,				\
	.wait_state		= SSP_MWIRE_WAIT_ZERO,			\
	.duplex			= SSP_MICROWIRE_CHANNEL_FULL_DUPLEX,	\
	.cs_control		= k3v2_spidev##id##_cs_set,		\
}

PL022_CONFIG_CHIP(0);
PL022_CONFIG_CHIP(1);
PL022_CONFIG_CHIP(2);
PL022_CONFIG_CHIP(3);

PL022_CONFIG_CHIP(4);
#define K3_SPI_PLAT_DATA(dev_name, num_id, num_cs, en_dma)		\
	{								\
		.bus_id = num_id,					\
		.num_chipselect = num_cs,				\
		.enable_dma = en_dma,					\
		.dma_filter = k3_def_dma_filter,			\
		.dma_rx_param = &K3_DMA_PARAM_NAME(dev_name##_RX),	\
		.dma_tx_param = &K3_DMA_PARAM_NAME(dev_name##_TX),	\
		.init = dev_name##_init,				\
		.exit = dev_name##_exit,				\
	}

static struct pl022_ssp_controller spi_plat_data[] = {
	K3_SPI_PLAT_DATA(SPI0, 0, 4, 1),
	K3_SPI_PLAT_DATA(SPI1, 1, 1, 1),
};


/* spi_devs driver registration */
static int k3v2_spi_board_register(void)
{
	static struct spi_board_info info[] = {
		[0] = {
			.modalias = "spi_dev0",
			.max_speed_hz = 13000000,
			.bus_num = 0,
			.chip_select = 0,
			.controller_data = &spidev0_chip_info,
		},
		[1] = {
			.modalias = "spi_dev1",
			.max_speed_hz = 13000000,
			.bus_num = 0,
			.chip_select = 1,
			.controller_data = &spidev1_chip_info,
		},
		[2] = {
			.modalias = "spi_dev2",
			.max_speed_hz = 13000000,
			.bus_num = 0,
			.chip_select = 2,
			.controller_data = &spidev2_chip_info,
		},
		[3] = {
			.modalias = "spi_dev3",
			.max_speed_hz = 13000000,
			.bus_num = 0,
			.chip_select = 3,
			.controller_data = &spidev3_chip_info,
		},
	};
	return spi_register_board_info(info, ARRAY_SIZE(info));
}

static int k3v2_spi1_board_register(void)
{
	static struct spi_board_info info[] = {

        [0] = {
            .modalias = "sprd_modem_spi3.0",
            .max_speed_hz   = 13000000,
            .bus_num           = 1,
            .chip_select         = 0,
            .controller_data = &spidev4_chip_info,
              },

	};

	return spi_register_board_info(info, ARRAY_SIZE(info));
}

#define AMBA_DEVICE(name, busid, base, plat)			\
static struct amba_device name##_device = {			\
	.dev		= {					\
		.coherent_dma_mask = ~0,			\
		.init_name = busid,				\
		.platform_data = plat,				\
	},							\
	.res		= {					\
		.start	= REG_BASE_##base,			\
		.end	= (REG_BASE_##base) + SZ_4K - 1,	\
		.flags	= IORESOURCE_MEM,			\
	},							\
	.dma_mask	= ~0,					\
	.irq		= { IRQ_##base, NO_IRQ },		\
}

AMBA_DEVICE(uart0, "amba-uart.0", UART0, &uart_plat_data[0]);
AMBA_DEVICE(uart1, "amba-uart.1", UART1, &uart_plat_data[1]);
AMBA_DEVICE(uart2, "amba-uart.2", UART2, &uart_plat_data[2]);
AMBA_DEVICE(uart3, "amba-uart.3", UART3, &uart_plat_data[3]);
AMBA_DEVICE(uart4, "amba-uart.4", UART4, &uart_plat_data[4]);
AMBA_DEVICE(spi0, "dev:ssp0", SPI0, &spi_plat_data[0]);
AMBA_DEVICE(spi1, "dev:ssp1", SPI1, &spi_plat_data[1]);

/* USB EHCI Host Controller */
static struct resource hisik3_usb_ehci_resource[] = {
	[0] = {
		.start = REG_BASE_USB2EHCI,
		.end   = REG_BASE_USB2EHCI + REG_USB2EHCI_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_USB2HST0,
		.end   = IRQ_USB2HST0,
		.flags = IORESOURCE_IRQ,
	}
};
static struct hisik3_ehci_platform_data hisik3_usbh_data = {
	.operating_mode = HISIK3_USB_HOST,
	.phy_config = NULL,
	.power_down_on_bus_suspend = 0,
#if  defined( CONFIG_XMM_POWER) && defined( CONFIG_BALONG_POWER)
	.post_suspend = NULL,
	.pre_resume = NULL,
#else
#ifdef CONFIG_XMM_POWER
	.post_suspend = xmm_power_runtime_idle,
	.pre_resume = xmm_power_runtime_resume,
#endif
#ifdef CONFIG_BALONG_POWER
	.post_suspend = balong_power_runtime_idle,
	.pre_resume = balong_power_runtime_resume,
#endif
#endif
};

static u64 hisik3_usb_ehci_dmamask = (0xFFFFFFFFUL);
static struct platform_device hisik3_usb_ehci = {
	.name		  = "hisik3-ehci",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(hisik3_usb_ehci_resource),
	.resource	  = hisik3_usb_ehci_resource,
	.dev              = {
		.dma_mask = &hisik3_usb_ehci_dmamask,
		.coherent_dma_mask = (0xFFFFFFFFUL),
		.platform_data		= &hisik3_usbh_data,
	}
};

/* USB OHCI Host Controller */
static struct resource hisi_usb_ohci_resource[] = {
	[0] = {
		.start = REG_BASE_USB2OHCI,
		.end   = REG_BASE_USB2OHCI + REG_USB2OHCI_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_USB2HST1,
		.end   = IRQ_USB2HST1,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 hisi_usb_ohci_dmamask = (0xFFFFFFFFUL);
static struct platform_device hisik3_usb_ohci = {
	.name		  = "hisik3-ohci",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(hisi_usb_ohci_resource),
	.resource	  = hisi_usb_ohci_resource,
	.dev              = {
		.dma_mask = &hisi_usb_ohci_dmamask,
		.coherent_dma_mask = (0xFFFFFFFFUL),
	}
};

/* USB OTG, Device only. */
static struct lm_device hisik3_usb_otg = {
	.dev            = {
		.init_name = "hisik3-usb-otg",
	},
	.resource.start = REG_BASE_USB2DVC,
	.resource.end = REG_BASE_USB2DVC + REG_USB2DVC_IOSIZE - 1,
	.resource.flags = IORESOURCE_MEM,
	.irq = IRQ_USB2DVC,
	.id = -1,
};

static struct amba_device rtc_device = {
	.dev = {
		.init_name = "pl061-rtc",
	},
	.res = {
		.start  = REG_BASE_RTC,
		.end    = REG_BASE_RTC + SZ_4K - 1,
		.flags  = IORESOURCE_MEM,
	},
	.irq = { IRQ_RTC, NO_IRQ },
	.periphid = 0x00041031,
};

static struct lm_device *hisi_lm_dev[] __initdata = {
	&hisik3_usb_otg,
};

static struct resource hisik3_fb_resources[] = {
	[0] = {
		.name = "reg_base_edc0",
		.start = REG_BASE_EDC0,
		.end = REG_BASE_EDC0 + REG_EDC0_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "reg_base_edc1",
		.start = REG_BASE_EDC1,
		.end = REG_BASE_EDC1 + REG_EDC1_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.name = "reg_base_pwm0",
		.start = REG_BASE_PWM0,
		.end = REG_BASE_PWM0 + REG_PWM0_IOSIZE-1,
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.name = "irq_edc0",
		.start = IRQ_EDC0,
		.end = IRQ_EDC0,
		.flags = IORESOURCE_IRQ,
	},
	[4] = {
		.name = "irq_edc1",
		.start = IRQ_EDC1,
		.end = IRQ_EDC1,
		.flags = IORESOURCE_IRQ,
	},
	[5] = {
		.name = "irq_ldi0",
		.start = IRQ_LDI0,
		.end = IRQ_LDI0,
		.flags = IORESOURCE_IRQ,
	},
	[6] = {
		.name = "irq_ldi1",
		.start = IRQ_LDI1,
		.end = IRQ_LDI1,
		.flags	 = IORESOURCE_IRQ,
	},
};

static struct resource hisik3_hdmi_resources[] = {
    [0] = {
        .name = "irq_hdmi",
        .start = IRQ_HDMI,
        .end = IRQ_HDMI,
        .flags = IORESOURCE_IRQ,
    },
    [1] = {
        .name = "reg_hdmi",
        .start = REG_BASE_HDMI,
        .end = REG_BASE_HDMI + REG_HDMI_IOSIZE - 1,
        .flags = IORESOURCE_MEM,
    }
};

static struct platform_device hisik3_hdmi_device = {
    .name = "k3_hdmi",
    .id = 0,
    .num_resources = ARRAY_SIZE(hisik3_hdmi_resources),
    .resource = hisik3_hdmi_resources,
};

#ifdef CONFIG_SECENGDEV
/*seceng driver begin. added by z00212134*/
static struct resource  hisik3_seceng_resources[] = {
	[0] = {
		.start = REG_BASE_SECENG,
		.end = REG_BASE_SECENG + REG_SECENG_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device  hisik3_seceng_device = {
	.name = "k3v2_seceng",
	.id = -1,
	.num_resources = ARRAY_SIZE(hisik3_seceng_resources),
	.resource = hisik3_seceng_resources,
};
 /*seceng driver end. added by z00212134*/
#endif

static struct platform_device hisik3_fb_device = {
	.name	= "k3_fb",
	.id	= 0,
	.num_resources	= ARRAY_SIZE(hisik3_fb_resources),
	.resource	= hisik3_fb_resources,
};

static struct resource hisik3_gpu_resources[] = {
#ifdef CONFIG_G2D
	{
		.name	= "g2d_irq",
		.start	= IRQ_G2D,
		.end	= IRQ_G2D,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "g2d_base",
		.start	= REG_BASE_G2D,
		.end	= REG_BASE_G2D + REG_G2D_IOSIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
#endif
#ifdef CONFIG_G3D
	{
		.name	= "g3d_irq",
		.start	= IRQ_G3D,
		.end	= IRQ_G3D,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "g3d_base",
		.start	= REG_BASE_G3D,
		.end	= REG_BASE_G3D + REG_G3D_IOSIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
#endif
};

static struct platform_device hisik3_gpu_device = {
	.name	= "galcore",
	.id	= 0,
	.num_resources	= ARRAY_SIZE(hisik3_gpu_resources),
	.resource	= &hisik3_gpu_resources,
};

static struct resource hx170dec_resources[] = {
	[0] = {
	       .start = REG_BASE_VDEC,
	       .end = REG_BASE_VDEC + REG_VDEC_IOSIZE - 1,
	       .flags = IORESOURCE_MEM,
	       },

	[1] = {
	       .start = IRQ_VDEC,
	       .end = IRQ_VDEC,
	       .flags = IORESOURCE_IRQ,
	       }
};

static struct platform_device hisik3_hx170dec_device = {
	.name = "hx170dec",
	.id = 0,
	.resource = hx170dec_resources,
	.num_resources = 2,
};


static struct resource hx280enc_resources[] = {
	[0] = {
	       .start = REG_BASE_VENC,
	       .end = REG_BASE_VENC + REG_VENC_IOSIZE - 1,
	       .flags = IORESOURCE_MEM,
	       },

	[1] = {
	       .start = IRQ_VENC,
	       .end = IRQ_VENC,
	       .flags = IORESOURCE_IRQ,
	       }
};

static struct platform_device hisik3_hx280enc_device = {
	.name = "hx280enc",
	.id = 0,
	.resource = hx280enc_resources,
	.num_resources = 2,
};

static struct resource asp_resources[] = {
	{
		.name       = "asp_irq",
		.start      = IRQ_ASP,
		.end        = IRQ_ASP,
		.flags      = IORESOURCE_IRQ,
	},
	{
		.name       = "asp_base",
		.start      = REG_BASE_ASP,
		.end        = REG_BASE_ASP + REG_ASP_IOSIZE - 1,
		.flags      = IORESOURCE_MEM,
	},
};

static struct platform_device hisik3_asp_device = {
	.name = "hi3620-asp",
	.id	= -1,
	.num_resources	= ARRAY_SIZE(asp_resources),
	.resource	= &asp_resources,
};

static struct resource aspdigital_resources[] = {
    {
        .name       = "asp_irq",
        .start      = IRQ_ASP,
        .end        = IRQ_ASP,
        .flags      = IORESOURCE_IRQ,
    },
    {
        .name       = "asp_base",
        .start      = REG_BASE_ASP,
        .end        = REG_BASE_ASP + REG_ASP_IOSIZE - 1,
        .flags      = IORESOURCE_MEM,
    },
};

static struct platform_device hisik3_aspdigital_device = {
    .name = "hi3620-aspdigital",
    .id = -1,
    .num_resources = ARRAY_SIZE(aspdigital_resources),
    .resource = &aspdigital_resources,
};

#ifdef CONFIG_MFD_HI6421_IRQ
static struct hi6421_codec_platform_data hi6421_codec_plat_data = {
	.irq[0] = IRQ_HEADSET_PLUG_OUT,
	.irq[1] = IRQ_HEADSET_PLUG_IN,
	.irq[2] = IRQ_BSD_BTN_PRESS_DOWN,
	.irq[3] = IRQ_BSD_BTN_PRESS_UP,
};
#endif

static struct resource hisik3_pmuspi_resources[] = {
	{
		.name	= "hi6421_base",
		.start	= REG_BASE_PMUSPI,
		.end	= REG_BASE_PMUSPI + REG_PMUSPI_IOSIZE - 1,
		.flags	= IORESOURCE_MEM,
	}
};

static struct platform_device hisik3_hi6421_codec_device = {
	.name	= "hi6421-codec",
	.id	= -1,
	.num_resources	= ARRAY_SIZE(hisik3_pmuspi_resources),
	.resource	= &hisik3_pmuspi_resources,
#ifdef CONFIG_MFD_HI6421_IRQ
	.dev = {
		.platform_data	= &hi6421_codec_plat_data,
	},
#endif
};

static struct platform_device hisik3_digital_audio_device = {
	.name	= "digital-audio",
	.id	= -1,
};


static u64 audio_card_dmamask = DMA_BIT_MASK(32);

static struct platform_device hisik3_hi6421_device = {
	.name = "hi3620_hi6421",
	.id   = -1,
	.dev = {
		.dma_mask	= &audio_card_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

static struct resource ipps2_resources[] = {
	{
		.start = REG_BASE_MCU,
		.end = REG_BASE_MCU + REG_MCU_IOSIZE -1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = REG_BASE_SCTRL,
		.end = REG_BASE_SCTRL + REG_SCTRL_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_MCU,
		.end = IRQ_MCU,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device ipps2_device = {
	.id = 0,
	.name = "ipps-v2",
	.resource = ipps2_resources,
	.num_resources = ARRAY_SIZE(ipps2_resources),
};

#ifdef CONFIG_K3V2_WAKEUP_TIMER
static struct resource k3v2_wakeup_timer_resources [] = {
	[0] = {
	       .start = REG_BASE_TIMER1,
	       .end = REG_BASE_TIMER1 + REG_TIMER1_IOSIZE - 1,
	       .flags = IORESOURCE_MEM,
	       },

	[1] = {
	       .start = IRQ_TIMER2,
	       .end = IRQ_TIMER2,
	       .flags = IORESOURCE_IRQ,
	       }
};
static struct platform_device k3v2_wakeup_timer_device = {
	.name = "k3v2_wakeup_timer",
	.id	= -1,
	.num_resources	= ARRAY_SIZE(k3v2_wakeup_timer_resources),
	.resource	= &k3v2_wakeup_timer_resources,
};
#endif

#ifdef CONFIG_THERMAL_FRAMEWORK
#ifdef CONFIG_K3V2_AP_SENSOR
static struct ap_temp_sensor_pdata ap_temp_plat_data = {
	.name = "ap_temp_sensor",
};

static struct platform_device ap_temp_device = {
	.id = 0,
	.name = "ap_temp_sensor",
	.dev = {
		.platform_data	= &ap_temp_plat_data,
	},
};
#endif
#ifdef CONFIG_K3V2_SIM_SENSOR
static struct sim_temp_sensor_pdata sim_temp_plat_data = {
	.name = "sim_temp_sensor",
};

static struct platform_device sim_temp_device = {
	.id = 1,
	.name = "sim_temp_sensor",
	.dev = {
		.platform_data	= &sim_temp_plat_data,
	},
};
#endif
#ifdef CONFIG_K3V2_CPU_TEMP_SENSOR
static struct k3v2_temp_sensor_pdata k3v2_temp_plat_data = {
	.name = "k3v2_temp_sensor",
};

static struct resource k3v2_temp_resources[] = {
	{
		.start = REG_BASE_PCTRL,
		.end = REG_BASE_PCTRL + REG_PCTRL_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device k3v2_temp_device = {
	.id = 2,
	.name = "k3v2_temp_sensor",
	.dev = {
		.platform_data = &k3v2_temp_plat_data,
	},
	.resource = k3v2_temp_resources,
	.num_resources = ARRAY_SIZE(k3v2_temp_resources),

};
#endif
#ifdef CONFIG_SENSORS_NCT203
static struct nct203_temp_sensor_pdata nct203_temp_plat_data = {
	.name = "nct203_temp_sensor",
};

static struct platform_device nct203_temp_device = {
	.id = 3,
	.name = "nct203_temp_sensor",
	.dev = {
		.platform_data	= &nct203_temp_plat_data,
	},
};
#endif
#ifdef CONFIG_THERMAL_CONFIG
static struct thermal_config_name_pdata thermal_config_plat_data = {
	.name = "thermal_config_name",
};

static struct platform_device thermal_config_device = {
	.id = 4,
	.name = "thermal_config_name",
	.dev = {
		.platform_data	= &thermal_config_plat_data,
	},
};
#endif
#endif
#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
struct plat_data dev_plat_data = {
	.obj = IPPS_OBJ_GPU,
	.cur_profile = GPU_PROFILE0,
	.mode = IPPS_DISABLE,
	.policy = 0x0,/*0x0:powersave, 0x10:normal, 0x20:performance*/
	.min = GPU_PROFILE0,
#ifdef WITH_G3D_600M_PROF
	.max = GPU_PROFILE5,
#else
	.max = GPU_PROFILE4,
#endif
	.safe = GPU_PROFILE1,
};

static struct resource k3v2_dcdc_gpu_resources[] = {
	{
		.start = REG_BASE_PMCTRL,
		.end = REG_BASE_PMCTRL + REG_PMCTRL_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device k3v2_dcdc_gpu_device = {
	.id = 0,
	.name = "k3v2-dcdc-gpu",
	.resource = k3v2_dcdc_gpu_resources,
	.num_resources = ARRAY_SIZE(k3v2_dcdc_gpu_resources),
	.dev = {
		.platform_data	= &dev_plat_data,
	},
};
#endif
#define PL061_GPIO_PLATFORM_DATA(id)	\
static struct pl061_platform_data gpio##id##_plat_data = {	\
	.gpio_base	= (id * 8),	\
	.irq_base	= IRQ_GPIO_BASE + (id * 8),	\
}
PL061_GPIO_PLATFORM_DATA(0);
PL061_GPIO_PLATFORM_DATA(1);
PL061_GPIO_PLATFORM_DATA(2);
PL061_GPIO_PLATFORM_DATA(3);
PL061_GPIO_PLATFORM_DATA(4);
PL061_GPIO_PLATFORM_DATA(5);
PL061_GPIO_PLATFORM_DATA(6);
PL061_GPIO_PLATFORM_DATA(7);
PL061_GPIO_PLATFORM_DATA(8);
PL061_GPIO_PLATFORM_DATA(9);
PL061_GPIO_PLATFORM_DATA(10);
PL061_GPIO_PLATFORM_DATA(11);
PL061_GPIO_PLATFORM_DATA(12);
PL061_GPIO_PLATFORM_DATA(13);
PL061_GPIO_PLATFORM_DATA(14);
PL061_GPIO_PLATFORM_DATA(15);
PL061_GPIO_PLATFORM_DATA(16);
PL061_GPIO_PLATFORM_DATA(17);
PL061_GPIO_PLATFORM_DATA(18);
PL061_GPIO_PLATFORM_DATA(19);
PL061_GPIO_PLATFORM_DATA(20);
PL061_GPIO_PLATFORM_DATA(21);

AMBA_DEVICE(gpio0, "gpio0", GPIO0, &gpio0_plat_data);
AMBA_DEVICE(gpio1, "gpio1", GPIO1, &gpio1_plat_data);
AMBA_DEVICE(gpio2, "gpio2", GPIO2, &gpio2_plat_data);
AMBA_DEVICE(gpio3, "gpio3", GPIO3, &gpio3_plat_data);
AMBA_DEVICE(gpio4, "gpio4", GPIO4, &gpio4_plat_data);
AMBA_DEVICE(gpio5, "gpio5", GPIO5, &gpio5_plat_data);
AMBA_DEVICE(gpio6, "gpio6", GPIO6, &gpio6_plat_data);
AMBA_DEVICE(gpio7, "gpio7", GPIO7, &gpio7_plat_data);
AMBA_DEVICE(gpio8, "gpio8", GPIO8, &gpio8_plat_data);
AMBA_DEVICE(gpio9, "gpio9", GPIO9, &gpio9_plat_data);
AMBA_DEVICE(gpio10, "gpio10", GPIO10, &gpio10_plat_data);
AMBA_DEVICE(gpio11, "gpio11", GPIO11, &gpio11_plat_data);
AMBA_DEVICE(gpio12, "gpio12", GPIO12, &gpio12_plat_data);
AMBA_DEVICE(gpio13, "gpio13", GPIO13, &gpio13_plat_data);
AMBA_DEVICE(gpio14, "gpio14", GPIO14, &gpio14_plat_data);
AMBA_DEVICE(gpio15, "gpio15", GPIO15, &gpio15_plat_data);
AMBA_DEVICE(gpio16, "gpio16", GPIO16, &gpio16_plat_data);
AMBA_DEVICE(gpio17, "gpio17", GPIO17, &gpio17_plat_data);
AMBA_DEVICE(gpio18, "gpio18", GPIO18, &gpio18_plat_data);
AMBA_DEVICE(gpio19, "gpio19", GPIO19, &gpio19_plat_data);
AMBA_DEVICE(gpio20, "gpio20", GPIO20, &gpio20_plat_data);
AMBA_DEVICE(gpio21, "gpio21", GPIO21, &gpio21_plat_data);

static struct amba_device *amba_devs[] __initdata = {
	&uart0_device,
	&uart1_device,
	&uart2_device,
	&uart3_device,
	&uart4_device,
	&spi0_device,
	&spi1_device,
	&rtc_device,
	&gpio0_device,
	&gpio1_device,
	&gpio2_device,
	&gpio3_device,
	&gpio4_device,
	&gpio5_device,
	&gpio6_device,
	&gpio7_device,
	&gpio8_device,
	&gpio9_device,
	&gpio10_device,
	&gpio11_device,
	&gpio12_device,
	&gpio13_device,
	&gpio14_device,
	&gpio15_device,
	&gpio16_device,
	&gpio17_device,
	&gpio18_device,
	&gpio19_device,
	&gpio20_device,
	&gpio21_device,
};

static struct i2c_dw_data i2c0_plat_data = {
	.dma_filter = k3_def_dma_filter,
	.dma_rx_param = &K3_DMA_PARAM_NAME(I2C0_RX),
	.dma_tx_param = &K3_DMA_PARAM_NAME(I2C0_TX),
	.init = I2C0_init,
	.exit = I2C0_exit,
	.reset	= I2C0_reset,
	.delay_sda = I2C0_delay_sda,
	.reset_controller = I2C0_reset_controller,
};

static struct i2c_dw_data i2c1_plat_data = {
	.dma_filter = k3_def_dma_filter,
	.dma_rx_param = &K3_DMA_PARAM_NAME(I2C1_RX),
	.dma_tx_param = &K3_DMA_PARAM_NAME(I2C1_TX),
	.init = I2C1_init,
	.exit = I2C1_exit,
	.reset = I2C1_reset,
	.delay_sda = NULL,
	.reset_controller = I2C1_reset_controller,
};

static struct i2c_dw_data i2c2_plat_data = {
	.dma_filter = k3_def_dma_filter,
	.dma_rx_param = &K3_DMA_PARAM_NAME(I2C2_RX),
	.dma_tx_param = &K3_DMA_PARAM_NAME(I2C2_TX),
	.init = I2C2_init,
	.exit = I2C2_exit,
	.reset	=  I2C2_reset,
	.delay_sda = I2C2_delay_sda,
	.reset_controller = I2C2_reset_controller,
};

static struct i2c_dw_data i2c3_plat_data = {
	.dma_filter = k3_def_dma_filter,
	.dma_rx_param = &K3_DMA_PARAM_NAME(I2C3_RX),
	.dma_tx_param = &K3_DMA_PARAM_NAME(I2C3_TX),
	.init = I2C3_init,
	.exit = I2C3_exit,
	.reset = I2C3_reset,
	.delay_sda = NULL,
	.reset_controller = I2C3_reset_controller,
};

static struct resource i2c_resource0[] = {
	[0] = {
		.start	= REG_BASE_I2C0,
		.end	= REG_BASE_I2C0 + REG_I2C0_IOSIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_I2C0,
		.end    = IRQ_I2C0,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct resource i2c_resource1[] = {
	[0] = {
		.start	= REG_BASE_I2C1,
		.end	= REG_BASE_I2C1 + REG_I2C1_IOSIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_I2C1,
		.end    = IRQ_I2C1,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct resource i2c_resource2[] = {
	[0] = {
		.start	= REG_BASE_I2C2,
		.end	= REG_BASE_I2C2 + REG_I2C2_IOSIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_I2C2,
		.end    = IRQ_I2C2,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct resource i2c_resource3[] = {
	[0] = {
		.start	= REG_BASE_I2C3,
		.end	= REG_BASE_I2C3 + REG_I2C3_IOSIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_I2C3,
		.end    = IRQ_I2C3,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device hisik3_i2c_device0 = {
	.name		= "i2c_designware",
	.id		= 0,
	.resource	= i2c_resource0,
	.num_resources	= ARRAY_SIZE(i2c_resource0),
	.dev = {
		.platform_data = &i2c0_plat_data,
		.init_name = "dw-i2c.0",
	},
};

static struct platform_device hisik3_i2c_device1 = {
	.name		= "i2c_designware",
	.id		= 1,
	.resource	= i2c_resource1,
	.num_resources	= ARRAY_SIZE(i2c_resource1),
	.dev = {
		.platform_data = &i2c1_plat_data,
		.init_name = "dw-i2c.1",
	},
};

static struct platform_device hisik3_i2c_device2 = {
	.name		= "i2c_designware",
	.id		= 2,
	.resource	= i2c_resource2,
	.num_resources	= ARRAY_SIZE(i2c_resource2),
	.dev = {
		.platform_data = &i2c2_plat_data,
		.init_name = "dw-i2c.2",
	},
};

static struct platform_device hisik3_i2c_device3 = {
	.name		= "i2c_designware",
	.id		= 3,
	.resource	= i2c_resource3,
	.num_resources	= ARRAY_SIZE(i2c_resource3),
	.dev = {
		.platform_data = &i2c3_plat_data,
		.init_name = "dw-i2c.3",
	},
};

struct resource hisik3_bat_resources[] = {
	[0] = {
		.start  = IRQ_VBUS_COMP_VBAT_RISING,
		.name   = NULL,
		.flags  = IORESOURCE_IRQ,
	} ,
	[1] = {
		.start  = IRQ_VBUS_COMP_VBAT_FALLING,
		.name   = NULL,
		.flags  = IORESOURCE_IRQ,
	} ,
	[2] = {
		.start  = IRQ_VBATLOW_RISING,
		.name   = NULL,
		.flags  = IORESOURCE_IRQ,
	} ,
	[3] = {
		.start  = IRQ_VBUS5P5_RISING,
		.name   = NULL,
		.flags  = IORESOURCE_IRQ,
	} ,
	[4] = {
		.start  = IRQ_CHGIN_OK1,
		.name   = NULL,
		.flags  = IORESOURCE_IRQ,
	} ,
	[5] = {
		.start  = IRQ_BATT_OK,
		.name   = NULL,
		.flags  = IORESOURCE_IRQ,
	} ,
	[6] = {
		.start  = IRQ_COUL_RISING,
		.name   = NULL,
		.flags  = IORESOURCE_IRQ,
	} ,
	[7] = {
		.start  = IRQ_GPIO(5),
		.name   = NULL,
		.flags  = IORESOURCE_IRQ,
	} ,
};

static struct platform_device hisik3_battery_device = {
	.name	= "k3-battery",
	.id	= 1,
	.resource	= hisik3_bat_resources,
	.num_resources	= ARRAY_SIZE(hisik3_bat_resources),
};

/* mmc0 resource : mmc0 controller & interrupt */
static struct resource mmc0_resources[] = {
	[0] = {
		.start          = REG_BASE_MMC0,
		.end            = REG_BASE_MMC0 + REG_MMC0_IOSIZE - 1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = IRQ_MMC0,
		.end            = IRQ_MMC0,
		.flags          = IORESOURCE_IRQ,
	},
};

/* mmc1 resource : mmc1 controller & interrupt */
static struct resource mmc1_resources[] = {
	[0] = {
		.start          = REG_BASE_MMC1,
		.end            = REG_BASE_MMC1 + REG_MMC1_IOSIZE - 1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = IRQ_MMC1,
		.end            = IRQ_MMC1,
		.flags          = IORESOURCE_IRQ,
	},
};

/* mmc2 resource : mmc2 controller & interrupt */
static struct resource mmc2_resources[] = {
	[0] = {
		.start          = REG_BASE_MMC2,
		.end            = REG_BASE_MMC2 + REG_MMC2_IOSIZE - 1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = IRQ_MMC2,
		.end            = IRQ_MMC2,
		.flags          = IORESOURCE_IRQ,
	},
};


/* mmc2 resource : mmc2 controller & interrupt */
static struct resource mmc3_resources[] = {
	[0] = {
		.start          = REG_BASE_MMC3,
		.end            = REG_BASE_MMC3 + REG_MMC3_IOSIZE - 1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = IRQ_MMC3,
		.end            = IRQ_MMC3,
		.flags          = IORESOURCE_IRQ,
	},
};


/* mmc0 timing config */
static int mmc0_timing_config[2][6] = {
	{
		0x9, 0x9, 0x9, 0x8, -1, 0x8
	},
	{
		0xf, 0xf, 0xf, 0xe, -1, 0xe
	},
};

/* mmc1 timing config */
static int mmc1_timing_config[2][6] = {
	{
		0x8, 0x8, 0x8, 0x8, -1, 0x8
	},
	{
		0x7, 0x7, 0x7, 0x7, -1, 0x7
	},
};

/* mmc2 timing config*/
static int mmc3_timing_config[2][6] = {
	{
		0x9, 0x9, 0x9, -1, -1, -1
	},
	{
		0xf, 0xf, 0xf, -1, -1, -1
	},
};

/* mmc0 tuning init configs */
static int mmc0_init_tuning_config[7][TUNING_INIT_CONFIG_NUM] = {
/* bus_clk, div, drv_sel, sam_sel_max, sam_sel_min, input_clk */
	{
		180000000, 6, 6, 13, 13, 25000000		/* init: 400k */
	},
	{
		180000000, 6, 4, 13, 13, 25000000		/* 0: SDR12 */
	},
	{
		360000000, 6, 4, 2, 0, 50000000		/* 1: */
	},
	{
		360000000, 6, 4, 2, 0, 50000000		/* 2: SDR25 */
	},
	{
		720000000, 6, 1, 9, 4, 100000000		/* 3: SDR50 */
	},
	{
		-1, -1, -1, -1, -1, -1					/* 4: SDR104 */
	},
	{
		360000000, 7, 1, 3, 0, 50000000		/* 5: DDR50 */
	},
};

static int mmc1_init_tuning_config[7][TUNING_INIT_CONFIG_NUM] = {
/* bus_clk, div, drv_sel, sam_sel_max, sam_sel_min, input_clk */
	{
		26000000, 1, 1, 3, 3, 13000000			/* init: 400k */
	},
	{
		26000000, 1, 1, 3, 3, 13000000			/* 0: MMC_LEGACY */
	},
	{
		360000000, 6, 3, 3, 1, 50000000		/* 1: MMC_HS*/
	},
	{
		26000000, 1, 1, 3, 3, 13000000			/* 2: SDR25 */
	},
	{
		360000000, 6, 3, 3, 1, 50000000		/* 3: SDR50 */
	},
	{
		-1, -1, -1, -1, -1, -1					/* 4: SDR104 */
	},
	{
		720000000, 6, 4, 8, 4, 100000000		/* 5: DDR50 */
	},
};

static int mmc2_init_tuning_config[7][TUNING_INIT_CONFIG_NUM] = {
/* bus_clk, div, drv_sel, sam_sel_max, sam_sel_min, input_clk */
	{
		180000000, 6, 6, 13, 13, 25000000		/* init: 400k */
	},
	{
		180000000, 6, 6, 13, 13, 25000000		/* 0: MMC_LEGACY */
	},
	{
		360000000, 6, 6, 4, 2, 50000000		/* 1: MMC_HS */
	},
	{
		360000000, 6, 6, 4, 2, 50000000		/* 2: SDR25 */
	},
	{
		-1, -1, -1, -1, -1, -1					/* 3: SDR50 */
	},
	{
		-1, -1, -1, -1, -1, -1					/* 4: SDR104 */
	},
	{
		-1, -1, -1, -1, -1, -1					/* 5: DDR50 */
	},
};

static int mmc3_init_tuning_config[7][TUNING_INIT_CONFIG_NUM] = {
/* bus_clk, div, drv_sel, sam_sel_max, sam_sel_min, input_clk */
	{
		180000000, 6, 6, 3, 3, 25000000		/* init: 400k */
	},
	{
		180000000, 6, 6, 3, 3, 25000000		/* 0: MMC_LEGACY */
	},
	{
		360000000, 6, 5, 7, 2, 50000000		/* 1: MMC_HS */
	},
	{
		360000000, 6, 5, 7, 2, 50000000		/* 2: SDR25 */
	},
	{
		-1, -1, -1, -1, -1, -1					/* 3: SDR50 */
	},
	{
		-1, -1, -1, -1, -1, -1					/* 4: SDR104 */
	},
	{
		-1, -1, -1, -1, -1, -1					/* 5: DDR50 */
	},
};

/* emmc platform data */
static struct hisik3_mmc_platform_data emmc_plat_data = {
	.ocr_mask = MMC_VDD_28_29,
#if defined(CONFIG_MACH_K3V2OEM1)
	.caps = MMC_CAP_8_BIT_DATA | MMC_CAP_MMC_HIGHSPEED |
		MMC_CAP_CLOCK_GATING | MMC_CAP_1_8V_DDR | MMC_CAP_UHS_DDR50,
#else
	.caps = MMC_CAP_8_BIT_DATA | MMC_CAP_MMC_HIGHSPEED |
		MMC_CAP_CLOCK_GATING,
#endif
	.quirks = MSHCI_QUIRK_BROKEN_CARD_DETECTION |
		MSHCI_QUIRK_ALWAYS_WRITABLE |
		MSHCI_QUIRK_BROKEN_PRESENT_BIT |
		MSHCI_QUIRK_TUNING_ENABLE,
	.ext_cd_init = NULL,
	.ext_cd_cleanup = NULL,
	.set_power = NULL,
	.clk_name = "clk_mmc1",
	.iomux_name = "block_emmc",
	.reg_name = NULL,
	.signal_reg_name = NULL,
	.timing_config = &mmc1_timing_config[0][0],
	.init_tuning_config = &mmc1_init_tuning_config[0][0],
	.suspend_timing_config = 0xf,
	.default_signal_voltage = MMC_SIGNAL_VOLTAGE_180,
	.allow_switch_signal_voltage = 0,
};

/* emmc device */
static struct platform_device hisik3_emmc_device = {
	.name           = "hi_mci",
	.id             = 1,
#if defined(CONFIG_MACH_K3V2OEM1)
	.num_resources  = ARRAY_SIZE(mmc1_resources),
	.resource       = mmc1_resources,
#else
	.num_resources  = ARRAY_SIZE(mmc0_resources),
	.resource       = mmc0_resources,
#endif
	.dev = {
		.coherent_dma_mask      = 0xffffffff,
		.platform_data          = &emmc_plat_data,
	},
};

static struct hisik3_mmc_platform_data sd_plat_data = {
	.ocr_mask = MMC_VDD_28_29,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED |
		MMC_CAP_MMC_HIGHSPEED | MMC_CAP_CLOCK_GATING |
		MMC_CAP_1_8V_DDR | MMC_CAP_UHS_SDR12 |
		MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 |
		MMC_CAP_DRIVER_TYPE_A,
#if 0
	.quirks = MSHCI_QUIRK_BROKEN_CARD_DETECTION |
		MSHCI_QUIRK_ALWAYS_WRITABLE |
		MSHCI_QUIRK_BROKEN_PRESENT_BIT,
#endif
	.quirks = MSHCI_QUIRK_ALWAYS_WRITABLE |
		MSHCI_QUIRK_EXTERNAL_CARD_DETECTION |
		MSHCI_QUIRK_TUNING_ENABLE,
#if defined(CONFIG_MACH_K3V2OEM1)
	.cd_gpio = 83,
#else
	.cd_gpio = 2,
#endif
	.ext_cd_init = NULL,
	.ext_cd_cleanup = NULL,
	.set_power = NULL,
	.clk_name = "clk_sd",
	.iomux_name = "block_sd",
	.reg_name = "sd-vcc",
	.signal_reg_name = "sdio-vcc",
	.timing_config = &mmc0_timing_config[0][0],
	.init_tuning_config = &mmc0_init_tuning_config[0][0],
	.suspend_timing_config = 0x9,
	.default_signal_voltage = MMC_SIGNAL_VOLTAGE_330,
	.allow_switch_signal_voltage = 1,
};

/* sd device */
static struct platform_device hisik3_sd_device = {
	.name           = "hi_mci",
	.id             = 0,
#if defined(CONFIG_MACH_K3V2OEM1)
	.num_resources  = ARRAY_SIZE(mmc0_resources),
	.resource       = mmc0_resources,
#else
	.num_resources  = ARRAY_SIZE(mmc1_resources),
	.resource       = mmc1_resources,
#endif
	.dev = {
		.coherent_dma_mask      = 0xffffffff,
		.platform_data          = &sd_plat_data,
	},
};

static void (*sdio_notify_func)(struct platform_device *,
						   int state);

static int sdio_cd_init(void (*notify_func)(struct platform_device *,
						   int state))
{
	sdio_notify_func = notify_func;
	return 0;
}

static int sdio_cd_cleanup(void (*notify_func)(struct platform_device *,
						      int state))
{
	sdio_notify_func = NULL;
	return 0;
}

//z00186406 add to config wifi's capability begin
static struct hisik3_mmc_platform_data sdio_plat_data = {
	.ocr_mask = MMC_VDD_165_195,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_POWER_OFF_CARD
                    | MMC_CAP_NONREMOVABLE | MMC_CAP_SD_HIGHSPEED,
	.quirks = MSHCI_QUIRK_TI_WL18XX,
	.ext_cd_init = NULL,
	.ext_cd_cleanup = NULL,
	.set_power = NULL,
	.clk_name = "clk_mmc3",
	.iomux_name = "block_sdio",
	.reg_name = NULL,
	.signal_reg_name = NULL,
	.timing_config = &mmc3_timing_config[0][0],
	.init_tuning_config = &mmc3_init_tuning_config[0][0],
	.suspend_timing_config = 0x9,
	.default_signal_voltage = MMC_SIGNAL_VOLTAGE_180,
	.allow_switch_signal_voltage = 0,
};
//z00186406 add to config wifi's capability end

/* sdio device */
static struct platform_device hisik3_sdio_device = {
	.name           = "hi_mci",
	.id             = 3,
#if defined(CONFIG_MACH_K3V2OEM1)
	.num_resources  = ARRAY_SIZE(mmc3_resources),
	.resource       = mmc3_resources,
#else
    .num_resources  = ARRAY_SIZE(mmc2_resources),
	.resource       = mmc2_resources,
#endif
	.dev = {
		.coherent_dma_mask      = 0xffffffff,
		.platform_data          = &sdio_plat_data,
	},
};

#ifdef CONFIG_XMM_POWER
/* device to manage power of XMM6020 */
static struct xmm_power_plat_data  xmm_plat_data = {
	.flashless = true,
	.gpios = {
		[XMM_RESET]	   = {95, GPIOF_OUT_INIT_LOW,  "xmm_rst"},
		[XMM_POWER_ON]	   = {96, GPIOF_OUT_INIT_LOW, "xmm_pow"},
		[XMM_PMU_RESET]    = {97, GPIOF_OUT_INIT_LOW, "xmm_pmu_rst"},
		[XMM_HOST_ACTIVE]  = {115, GPIOF_OUT_INIT_LOW, "host_act"},
		[XMM_HOST_WAKEUP]  = {113, GPIOF_IN, "host_wake"},
		[XMM_SLAVE_WAKEUP] = {114, GPIOF_OUT_INIT_LOW, "slave_wake"},
		[XMM_POWER_IND]    = {94, GPIOF_IN, "power_ind"},
		[XMM_RESET_IND]    = {98, GPIOF_IN, "reset_ind"},
		[XMM_SIM_TRIGGER_D]= {107, GPIOF_OUT_INIT_LOW, "sim_trigger_d"},
		[XMM_SIM_INI_CLK]  = {108, GPIOF_OUT_INIT_HIGH, "sim_ini_clk"},

	},
	.echi_device = &hisik3_usb_ehci,
	.block_name = "block_modem",
};

static struct platform_device xmm_power_device = {
    .name       = "xmm_power",
    .id = -1,
    .dev = {
        .platform_data = &xmm_plat_data,
    },
};
#endif

#ifdef CONFIG_BALONG_POWER
static struct balong_reset_ind_reg balong_reset_reg = {
    .base_addr = REG_BASE_GPIO12,
    .gpioic = 0x41C,
};

/* device to manage power of balong modem */
static struct balong_power_plat_data  balong_plat_data = {
    .flashless = true,
    .gpios = {
        [BALONG_POWER_ON]      = {96, GPIOF_OUT_INIT_LOW, "balong_pow"},    //PWR_HOLD
        [BALONG_PMU_RESET]    = {97, GPIOF_IN, "balong_pmu_rst"},   //AP_RST_MDM, for AP it is output signal, but we need configurate it to input;
        [BALONG_HOST_ACTIVE]  = {115, GPIOF_OUT_INIT_LOW, "host_act"},  //HOST_ACTIVE
        [BALONG_HOST_WAKEUP]  = {113, GPIOF_IN, "host_wake"},  //HOST_WAKEUP
        [BALONG_SLAVE_WAKEUP] = {114, GPIOF_OUT_INIT_LOW, "slave_wake"},  //SLAVE_WAKEUP
        [BALONG_POWER_IND]    = {94, GPIOF_IN, "power_ind"},    //MDM_PWROK_AP
        [BALONG_RESET_IND]    = {98, GPIOF_IN, "reset_ind"},    //MDM_PWROK_AP_2, MDM_PMU_RSTOUT_N later than MDM_PWROK_AP;
        [BALONG_SIM_PNP_IND]    = {54, GPIOF_IN, "sim_pnp"},    //GPIO_054_SIM_PNP;
        [BALONG_SUSPEND_REQUEST]  = {116, GPIOF_OUT_INIT_LOW, "Suspend_Request"}, //SUSPEND_REQUEST
        [BALONG_SLAVE_ACTIVE] = {62, GPIOF_IN, "SLAVE_ACTIVE"},   //MODEM_STATUS
    },
    .echi_device = &hisik3_usb_ehci,
    .block_name = "block_modem",
    .reset_reg = &balong_reset_reg,
};

static struct platform_device balong_power_device = {
    .name       = "balong_power",
    .id = -1,
    .dev = {
        .platform_data = &balong_plat_data,
    },
};
#endif

#ifdef CONFIG_MODEM_BOOT_QSC6085
/* device to manage power of QSC6085 modem */
static struct modem_qsc6085_platform_data  modem_qsc6085_plat_data = {
    .flashless = false,
    .gpios = {
        [COMM_WAKEUP_SLAVE]        = {95, GPIOF_OUT_INIT_LOW, "comm_wakeup_slave"},     //GPIO_095_WAKEUP_SLAVE
        [MODEM_BOOT_POWER_ON]      = {96, GPIOF_OUT_INIT_LOW, "modem_boot_power_on"},   //GPIO_094_MODEM_POWER_IND
        [MODEM_BOOT_PMU_RESET]    = {97, GPIOF_OUT_INIT_HIGH, "modem_boot_pmu_reset"},  //GPIO_097_M_RESET_PWRDWN_N
        [MODEM_BOOT_POWER_IND]    = {94, GPIOF_IN, "modem_boot_power_ind"}, //GPIO_094_MODEM_POWER_IND
        [MODEM_BOOT_RESET_IND]    = {98, GPIOF_IN, "modem_boot_reset_ind"}, //GPIO_096_M_ON1

        [COMM_DOWNLOAD_EN]  = {114, GPIOF_OUT_INIT_LOW, "comm_download_en"},   //GPIO_114_DOWNLOAD_EN
        [COMM_WAKEUP_HOST]  = {113, GPIOF_IN, "comm_wakeup_host"},  //GPIO_113_WAKE_H
        [MODEM_SIM_CARD_SWITCH]   = {44, GPIOF_OUT_INIT_LOW, "modem_sim_card_switch"},
        [MODEM_SIM_CARD_DETECT]   = {54, GPIOF_IN, "modem_sim_card_detect"},
        [MODEM_BOOT_PSHOLD]   	  = {6, GPIOF_OUT_INIT_LOW, "modem_boot_pshold"},
    },
    .block_name = "block_modem",
};

static struct platform_device modem_device_qsc6085_boot = {
    .name       = MODEM_DEVICE_BOOT(MODEM_QSC6085),    //"qsc6085_boot",
    .id = -1,
    .dev = {
        .platform_data = &modem_qsc6085_plat_data,
    },
};
#endif

#ifdef CONFIG_MODEM_BOOT_MTK6252
/* device to manage power of MTK6252 modem */
static struct modem_mtk6252_platform_data  modem_mtk6252_plat_data = {
    .flashless = false,
    .gpios = {
        [MODEM_MTK_BOOT_RESET]     = {22, GPIOF_OUT_INIT_HIGH,  "modem_boot_reset"},     //GPIO_022_MTK_RESET
        [MODEM_MTK_BOOT_POWER_ON]      = {20, GPIOF_OUT_INIT_LOW, "modem_boot_power_on"},   //GPIO_020_MTK_PWRON
        [MODEM_MTK_BOOT_DOWNLOAD_EN]      = {71, GPIOF_OUT_INIT_LOW, "modem_boot_download_en"},   //GPIO_071_MTK_DOWNLOAD_EN
        [MODEM_MTK_BOOT_SOFTWARE_STATE]      = {26, GPIOF_IN, "modem_boot_software_state"},   //GPIO_026_SOFTWARE_STATE
        [MODEM_MTK_BOOT_POWER_IND]    = {21, GPIOF_IN, "modem_boot_power_ind"},     //GPIO_021_MTK_PWRON_STAT
        [MODEM_MTK_BOOT_RESET_IND]    = {23, GPIOF_IN, "modem_boot_reset_ind"},     //GPIO_023_MTK_RESET_STAT

        [COMM_MTK_WAKEUP_SLAVE]  = {24, GPIOF_OUT_INIT_LOW, "comm_wakeup_slave"},          //GPIO_024_AP_WAKEUP_MTK
        [COMM_MTK_WAKEUP_HOST] = {25, GPIOF_IN, "comm_wakeup_host"},   //GPIO_025_MTK_WAKE_AP
    },
    .block_name = "block_modem",
};

static struct platform_device modem_device_mtk6252_boot = {
    .name       = MODEM_DEVICE_BOOT(MODEM_MTK6252),    //"mtk6252_boot",
    .id = -1,
    .dev = {
        .platform_data = &modem_mtk6252_plat_data,
    },
};
#endif

#ifdef CONFIG_MODEM_BOOT_SPRD8803G

//flashless platform data
static struct modem_sprd8803g_platform_data  modem_sprd8803g_plat_data = {

    .gpios_flashless = {
        [MODEM_SPRD_APCP_RTS]     = {94, GPIOF_OUT_INIT_LOW, "GPIO_APCP_RTS"},
        [MODEM_SPRD_CPAP_RDY]     = {24, GPIOF_IN, "GPIO_CPAP_RDY"},
        [MODEM_SPRD_CPAP_RTS]     = {21, GPIOF_IN, "GPIO_CPAP_RTS"},
        [MODEM_SPRD_APCP_RDY]     = {25, GPIOF_OUT_INIT_LOW, "GPIO_APCP_RDY"},
        [MODEM_SPRD_APCP_FLAG]     = {23, GPIOF_OUT_INIT_LOW, "GPIO_APCP_FLAG"},
        [MODEM_SPRD_CPAP_FLAG]     = {107, GPIOF_IN, "GPIO_CPAP_FLAG"},
        [MODEM_SPRD_CPAP_DINT]     = {99, GPIOF_IN, "GPIO_CPAP_DINT"},
        [MODEM_SPRD_APCP_NBOOT]     = {95, GPIOF_IN, "GPIO_APCP_NBOOT"},
        [MODEM_SPRD_APCP_RSTN]     = {97, GPIOF_OUT_INIT_LOW, "GPIO_APCP_RSTN"},
        [MODEM_SPRD_APCP_PWD]     = {96, GPIOF_OUT_INIT_LOW, "GPIO_APCP_PWD"},

        [MODEM_SPRD_CPAP_FLAG2]     = {106, GPIOF_IN, "GPIO_CPAP_FLAG2"},//modem crash or watchdog reset flag
        [MODEM_SPRD_APCP_RESEND]     = {22, GPIOF_OUT_INIT_LOW, "GPIO_APCP_RESEND"},
        [MODEM_SPRD_CPAP_RESEND]     = {98, GPIOF_IN, "GPIO_CPAP_RESEND"},
        [MODEM_SPRD_APCP_USBSW_OUT] = {0xff,GPIOF_IN, "UNUSED"},
        [MODEM_SPRD_CPAP_USBSW_IRQ] = {0xff,GPIOF_IN, "UNUSED"},
    },
	 .gpios = {
        [MODEM_SPRD_APCP_RTS]     = {24, GPIOF_OUT_INIT_LOW, "GPIO_APCP_RTS"},
        [MODEM_SPRD_CPAP_RDY]     = {21, GPIOF_IN, "GPIO_CPAP_RDY"},
        [MODEM_SPRD_CPAP_RTS]     = {25, GPIOF_IN, "GPIO_CPAP_RTS"},
        [MODEM_SPRD_APCP_RDY]     = {98, GPIOF_OUT_INIT_LOW, "GPIO_APCP_RDY"},
        [MODEM_SPRD_APCP_FLAG]     = {23, GPIOF_OUT_INIT_LOW, "GPIO_APCP_FLAG"},
        [MODEM_SPRD_APCP_USBSW_OUT]     = {22, GPIOF_OUT_INIT_LOW, "GPIO_APCP_USBSW_OUT"},
        [MODEM_SPRD_CPAP_FLAG]     = {94, GPIOF_IN, "GPIO_CPAP_FLAG"},
        [MODEM_SPRD_CPAP_USBSW_IRQ]     = {20, GPIOF_IN, "GPIO_CPAP_USBSW_IRQ"},
        [MODEM_SPRD_CPAP_DINT]     = {99, GPIOF_IN, "GPIO_CPAP_DINT"},
        [MODEM_SPRD_APCP_NBOOT]     = {95, GPIOF_OUT_INIT_LOW, "GPIO_APCP_NBOOT"},
        [MODEM_SPRD_APCP_RSTN]     = {97, GPIOF_OUT_INIT_LOW, "GPIO_APCP_RSTN"},
        [MODEM_SPRD_APCP_PWD]     = {96, GPIOF_OUT_INIT_LOW, "GPIO_APCP_PWD"},
        [MODEM_SPRD_CPAP_FLAG2] =  {0xff,GPIOF_IN, "UNUSED"},
        [MODEM_SPRD_APCP_RESEND] =  {0xff,GPIOF_IN, "UNUSED"},
        [MODEM_SPRD_CPAP_RESEND] =  {0xff,GPIOF_IN, "UNUSED"},
     },
    .block_name = "block_sprd_modem",
    .block_spi1_name = "block_spi1",
    .block_gpio_name = "block_sprd_gpio_modem",
    .block_spi1_gpio_name = "block_spi1_gpio",
        	
};


static struct platform_device modem_device_sprd8803g_boot = {
    .name       = MODEM_DEVICE_BOOT(MODEM_SPRD8803G),
    .id = -1,
    .dev = {
        .platform_data = &modem_sprd8803g_plat_data,
    },
};
#endif

int hi_sdio_detectcard_to_core(int val)
{
	if (sdio_notify_func == NULL)
		return -1;
	sdio_notify_func(&hisik3_sdio_device, val);
	return 0;
}
EXPORT_SYMBOL(hi_sdio_detectcard_to_core);
void hi_sdio_set_power(int val)
{
	if (sdio_plat_data.set_power == NULL) {
		printk("sdio set power is null\n");
		return;
	}

	sdio_plat_data.set_power(&hisik3_sdio_device, val);
	return;
}
EXPORT_SYMBOL(hi_sdio_set_power);

static struct platform_device *hisik3_power_dev[] __initdata = {
	&hi6421_regulator_device,
	&vcc_regulator_device,
};

#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
static struct platform_device *hisik3_power_dcdc_dev[] __initdata = {
	&hi6421_regulator_device,
	&vcc_regulator_dcdc_gpu_device,
};
#endif

/* please add platform device in the struct.*/
static struct platform_device *hisik3_public_dev[] __initdata = {
#ifdef CONFIG_K3V2_WAKEUP_TIMER
	&k3v2_wakeup_timer_device,
#endif
#if defined(CONFIG_MMC_EMMC_DEVICE)
	&hisik3_emmc_device,
#endif
#if defined(CONFIG_MMC_SD_DEVICE)
	&hisik3_sd_device,
#endif
#if defined(CONFIG_MMC_SDIO_DEVICE)
	&hisik3_sdio_device,
#endif
	&hisik3_usb_ehci,
#ifdef CONFIG_XMM_POWER
	&xmm_power_device,
#endif
#ifdef CONFIG_BALONG_POWER
	&balong_power_device,
#endif
#ifdef CONFIG_MODEM_BOOT_QSC6085
	&modem_device_qsc6085_boot,
#endif
#ifdef CONFIG_MODEM_BOOT_MTK6252
	&modem_device_mtk6252_boot,
#endif
#ifdef CONFIG_MODEM_BOOT_SPRD8803G
	&modem_device_sprd8803g_boot,
#endif
	&hisik3_usb_ohci,
	&hisik3_i2c_device0,
	&hisik3_i2c_device1,
	&hisik3_i2c_device2,
	&hisik3_i2c_device3,
	&ipps2_device,

#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
	&k3v2_dcdc_gpu_device,
#endif

	&hisik3_fb_device,
	&hisik3_gpu_device,
	&hisik3_hx170dec_device,
#if defined(CONFIG_8290_ENC)
	&hisik3_hx280enc_device,
#endif
	/* &hisik3_keypad_backlight_device, */
	&hisik3_asp_device,
	&hisik3_aspdigital_device,
	&hisik3_hi6421_codec_device,
	&hisik3_digital_audio_device,
	&hisik3_hi6421_device,
	&hisik3_battery_device,
	&hisik3_hdmi_device,
#ifdef CONFIG_SECENGDEV
/*seceng driver begin. added by z00212134*/
	&hisik3_seceng_device,
/*seceng driver end. added by z00212134*/
#endif
#ifdef CONFIG_HAS_EXTRAL_DCDC
    &extral_dcdc_regulator_device,
#endif
#ifdef CONFIG_THERMAL_FRAMEWORK
#ifdef CONFIG_K3V2_AP_SENSOR
	&ap_temp_device,
#endif
#ifdef CONFIG_K3V2_SIM_SENSOR
	&sim_temp_device,
#endif
#ifdef CONFIG_K3V2_CPU_TEMP_SENSOR
	&k3v2_temp_device,
#endif
#ifdef CONFIG_SENSORS_NCT203
	&nct203_temp_device,
#endif
#ifdef CONFIG_THERMAL_CONFIG
	&thermal_config_device
#endif
#endif
};
int pd_charge_flag;
/**
 * parse powerdown charge cmdline which is passed from fastoot. *
 * Format : pd_charge=0 or 1             *
 */
static int __init early_parse_pdcharge_cmdline(char * p)
{
	char pd_charge[HEX_STRING_MAX + 1];
	char *endptr = NULL;

	memset(pd_charge, 0, HEX_STRING_MAX + 1);

	memcpy(pd_charge, p, HEX_STRING_MAX);
	pd_charge[HEX_STRING_MAX] = '\0';

	pd_charge_flag = simple_strtoull(pd_charge, &endptr, TRANSFER_BASE);

	printk("power down charge p:%s, pd_charge_flag :%d\n", p, pd_charge_flag);

	return 0;
}
early_param("pd_charge", early_parse_pdcharge_cmdline);


unsigned int get_pd_charge_flag(void)
{
	return pd_charge_flag;
}
EXPORT_SYMBOL(get_pd_charge_flag);

/*c00218647+20121015 add for normal_reset = 0 or abnormal_reset = 1  begin*/
static unsigned int resetmode_normal = RESETMODE_FLAG_NORMAL;

static int __init early_parse_resetmode_cmdline(char * p)
{
	if (p) {
		if ((!strcmp(p,"press10s,"))||
			(!strcmp(p,"press1s,"))||
			(!strcmp(p,"ColdReset,"))||
			(!strcmp(p,"PoweroffAlarm,"))) {
			resetmode_normal = RESETMODE_FLAG_NORMAL;
			printk("resetmode is %s, resetmode_normal = %d\n", p, resetmode_normal);
		}
		else if ((!strcmp(p,"PanicReset,"))||(!strcmp(p,"AbnormalReset,"))) {
			resetmode_normal = RESETMODE_FLAG_ABNORMAL;
			printk("resetmode is %s resetmode_normal = %d\n", p, resetmode_normal);
		}
		else
			printk("Judge resetmode error! \n");

	}

	return 0;
}

early_param("normal_reset_type", early_parse_resetmode_cmdline);
unsigned int resetmode_is_normal(void)
{
	return resetmode_normal;
}

EXPORT_SYMBOL(resetmode_is_normal);
/*c00218647+20121015 add for two reset causes: normal_reset = 0 or abnormal_reset = 1 end*/

int hpm_vol_val;
int get_str_len(char *str)
{
	int count = 0;
	char *p = str;

	while ((*p != ',') && (*p != '\0'))	{
		count ++;
		p++;
	}

	return count;
}

static int __init early_parse_hpm_cmdline(char *p)
{
	char tmpbuf[HEX_STRING_MAX + 1];
	char *endptr = NULL;
	int str_len;

	memset(tmpbuf, 0, HEX_STRING_MAX + 1);

	str_len = get_str_len(p);
	memcpy(tmpbuf, p, str_len);
	tmpbuf[str_len] = '\0';
	hpm_vol_val = simple_strtoull(tmpbuf, &endptr, 10);

	printk(KERN_INFO "hpm_value = %d.\n", hpm_vol_val);

	return 0;
}
early_param("hpm_value", early_parse_hpm_cmdline);

int get_hpm_vol_val()
{
	return hpm_vol_val;
}
EXPORT_SYMBOL(get_hpm_vol_val);

int cpu_max_freq;

static int __init early_parse_cpumaxfreq_cmdline(char *p)
{
	char tmpbuf[HEX_STRING_MAX + 1];
	char *endptr = NULL;
	int str_len;

	memset(tmpbuf, 0, HEX_STRING_MAX + 1);

	str_len = get_str_len(p);
	memcpy(tmpbuf, p, str_len);
	tmpbuf[str_len] = '\0';
	cpu_max_freq = simple_strtoull(tmpbuf, &endptr, 10);

	printk(KERN_INFO "cpu_maxfreq = %d.\n", cpu_max_freq);

	return 0;
}
early_param("cpu_maxfreq", early_parse_cpumaxfreq_cmdline);

int get_cpu_max_freq()
{
	return cpu_max_freq;
}
EXPORT_SYMBOL(get_cpu_max_freq);


 int enter_recovery_flag;
/**
* parse boot_into_recovery cmdline which is passed from boot_recovery() of boot.c *
* Format : boot_into_recovery_flag=0 or 1             *
*/
static int __init early_parse_enterrecovery_cmdline(char * p)
{
    char enter_recovery[HEX_STRING_MAX + 1];
    char *endptr = NULL;

    memset(enter_recovery, 0, HEX_STRING_MAX + 1);

    memcpy(enter_recovery, p, HEX_STRING_MAX);
    enter_recovery[HEX_STRING_MAX] = '\0';

    enter_recovery_flag = simple_strtoull(enter_recovery, &endptr, TRANSFER_BASE);

    printk("enter recovery p:%s, enter_recovery_flag :%d\n", p, enter_recovery_flag);

    return 0;
}
early_param("enter_recovery", early_parse_enterrecovery_cmdline);

unsigned int get_boot_into_recovery_flag(void)
{
    return enter_recovery_flag;
}
EXPORT_SYMBOL(get_boot_into_recovery_flag);

int logctl_flag;
static int __init early_parse_logctl_cmdline(char * p)
{
    char logctl[HEX_STRING_MAX + 1];
    char *endptr = NULL;

    memset(logctl, 0, HEX_STRING_MAX + 1);

    memcpy(logctl, p, HEX_STRING_MAX);

    logctl[HEX_STRING_MAX] = '\0';

    logctl_flag = simple_strtoull(logctl, &endptr, TRANSFER_BASE);

    printk("logctl p:%s, logctl :%d\n", p, logctl_flag);

    return 0;

}

early_param("setup_logctl", early_parse_logctl_cmdline);

unsigned int get_logctl_value(void)
{
    return logctl_flag;
}

EXPORT_SYMBOL(get_logctl_value);

static unsigned int runmode_factory = RUNMODE_FLAG_NORMAL;

static int __init early_parse_runmode_cmdline(char * p)
{
	if (p) {
		if (!strcmp(p,"factory"))
			runmode_factory = RUNMODE_FLAG_FACTORY;

		printk("runmode is %s, runmode_factory = %d\n", p, runmode_factory);
	}

	return 0;
}

early_param("androidboot.swtype", early_parse_runmode_cmdline);

/* the function interface to check factory/normal mode in kernel */
unsigned int runmode_is_factory(void)
{
	return runmode_factory;
}

EXPORT_SYMBOL(runmode_is_factory);


boardid_parameter boardid_source;

/**
 * parse boaridid cmdline which is passed from fastoot. *
 * Format : BoardID=chip_id,pmu_id,board_id             *
 */
static int __init early_parse_boardid_cmdline(char *p)
{
	char board[HEX_STRING_MAX + 1];
	char chip [HEX_STRING_MAX + 1];
	char pmu  [HEX_STRING_MAX + 1];
	char *endptr = NULL;

	memset(board, 0, HEX_STRING_MAX + 1);
	memset(chip, 0, HEX_STRING_MAX + 1);
	memset(pmu, 0, HEX_STRING_MAX + 1);

	memcpy(chip, p, HEX_STRING_MAX);
	chip[HEX_STRING_MAX] = '\0';

	boardid_source.chip_id = simple_strtoull(chip, &endptr, TRANSFER_BASE);

	/* skip next ',' symbol */
	p += strlen(chip)+1;
	memcpy(pmu, p, HEX_STRING_MAX);
	pmu[HEX_STRING_MAX] = '\0';

	boardid_source.pmu_id = simple_strtoull(pmu, &endptr, TRANSFER_BASE);

	p += strlen(pmu)+1;
	memcpy(board, p, HEX_STRING_MAX);
	board[HEX_STRING_MAX] = '\0';

	boardid_source.board_id = simple_strtoull(board, &endptr, TRANSFER_BASE);

	printk(KERN_INFO "[bdid]boardid = 0x%x.\n", boardid_source.board_id);

	/* create hw attribute config data base on boardid */
	if(select_hw_config(boardid_source.board_id))
            printk(KERN_ERR "select_hw_config eroor\n");
	return 0;
}
early_param("boardid", early_parse_boardid_cmdline);


unsigned int get_boardid(void)
{
	return boardid_source.board_id;
}
EXPORT_SYMBOL(get_boardid);

unsigned int get_chipid(void)
{
	return boardid_source.chip_id;
}
EXPORT_SYMBOL(get_chipid);

unsigned int get_pmuid(void)
{
	return boardid_source.pmu_id;
}
EXPORT_SYMBOL(get_pmuid);



static int hw_version1 = 0;
static int hw_version2 = 0;
static int hw_min_version1 = 0;
static int hw_min_version2 = 0;
static int __init early_parse_hwversion_cmdline(char *p)
{
	char hw_version1_str[HEX_STRING_MAX + 1];
	char hw_version2_str [HEX_STRING_MAX + 1];
	char hw_min_version1_str[HEX_STRING_MAX + 1];
	char hw_min_version2_str[HEX_STRING_MAX + 1];
	char *endptr = NULL;

	memset(hw_version1_str, 0, HEX_STRING_MAX + 1);
	memset(hw_version2_str, 0, HEX_STRING_MAX + 1);
	memset(hw_min_version1_str, 0, HEX_STRING_MAX + 1);
	memset(hw_min_version2_str, 0, HEX_STRING_MAX + 1);
	
	memcpy(hw_version1_str, p, HEX_STRING_MAX);
	hw_version1_str[HEX_STRING_MAX] = '\0';

	hw_version1 = simple_strtoull(hw_version1_str, &endptr, TRANSFER_BASE);

	/* skip next ',' symbol */
	p += strlen(hw_version1_str)+1;
	memcpy(hw_version2_str, p, HEX_STRING_MAX);
	hw_version2_str[HEX_STRING_MAX] = '\0';
	
	hw_version2 = simple_strtoull(hw_version2_str, &endptr, TRANSFER_BASE);
	
	printk(KERN_INFO "hw_version1 = 0x%x. hw_version2 = 0x%x\n", hw_version1,hw_version2);
	
	/* skip next ',' symbol */
	p += strlen(hw_version2_str)+1;
	memcpy(hw_min_version1_str, p, HEX_STRING_MAX);
	hw_min_version1_str[HEX_STRING_MAX] = '\0';
	
	hw_min_version1 = simple_strtoull(hw_min_version1_str, &endptr, TRANSFER_BASE);
	
	/* skip next ',' symbol */
	p += strlen(hw_min_version1_str)+1;
	memcpy(hw_min_version2_str, p, HEX_STRING_MAX);
	hw_min_version2_str[HEX_STRING_MAX] = '\0';
	
	hw_min_version2 = simple_strtoull(hw_min_version2_str, &endptr, TRANSFER_BASE);
	
	printk(KERN_INFO "hw_min_version1 = 0x%x. hw_min_version2 = 0x%x\n", hw_min_version1,hw_min_version2);
	
	init_product_feature_array();
	
	return 0;
}
early_param("hw_version", early_parse_hwversion_cmdline);

void get_hwversion_num(int* version1, int* version2, int* min_version1,int* min_version2)
{
	*version1 = hw_version1;
	*version2 = hw_version2;
	*min_version1 = hw_min_version1;
	*min_version2 = hw_min_version2;
}
EXPORT_SYMBOL(get_hwversion_num);


void __init hisik3_amba_init(void)
{
	int i;

	edb_trace(1);

	k3v2_spi_board_register();
	k3v2_spi1_board_register();

	for (i = 0; i < ARRAY_SIZE(amba_devs); i++) {
		struct amba_device *d = amba_devs[i];
		amba_device_register(d, &iomem_resource);
	}
	edb_trace(1);
}

void __init hisik3_lm_init(void)
{
	struct lm_device *d = NULL;
	int i;

	edb_trace(1);

	for (i = 0; i < ARRAY_SIZE(hisi_lm_dev); i++) {
		d = hisi_lm_dev[i];
		lm_device_register(d);
	}
	edb_trace(1);
}
void __init hisik3_platform_init(void)
{
    if(get_product_feature(PROD_FEATURE_GPU_DCDC_SUPPLY))
	{
		#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
		platform_add_devices(hisik3_power_dcdc_dev, ARRAY_SIZE(hisik3_power_dcdc_dev));
		#endif
	}
	else
	{
		platform_add_devices(hisik3_power_dev, ARRAY_SIZE(hisik3_power_dev));
	}

	/* platform devices were addded here. */
	platform_add_devices(hisik3_public_dev, ARRAY_SIZE(hisik3_public_dev));

	/* add other API if you need. */
	hisik3_lm_init();
}

void __init k3v2_common_init(void)
{
	edb_trace(1);
	//hisik3_init_cache();
	hisik3_amba_init();
	hisik3_platform_init();

	DEBUG_LED_INIT();
}

static struct resource  A9_pmu_resource[] = {
	[0] = {
		.start = IRQ_PMIRQ0,
		.end   = IRQ_PMIRQ0,
		.flags = IORESOURCE_IRQ,
	},
#if (CONFIG_NR_CPUS >= 2)
	[1] = {
		.start = IRQ_PMIRQ1,
		.end   = IRQ_PMIRQ1,
		.flags = IORESOURCE_IRQ,
	},
#endif

#if (CONFIG_NR_CPUS >= 3)
	[2] = {
		.start = IRQ_PMIRQ2,
		.end   = IRQ_PMIRQ2,
		.flags = IORESOURCE_IRQ,
	},
#endif

#if (CONFIG_NR_CPUS >= 4)
	[3] = {
		.start = IRQ_PMIRQ3,
		.end   = IRQ_PMIRQ3,
		.flags = IORESOURCE_IRQ,
	},
#endif
};

static struct platform_device A9_pmu_device = {
	.name		= "arm-pmu",
	.id		= ARM_PMU_DEVICE_CPU,
	.resource	= &A9_pmu_resource,
	.num_resources	= ARRAY_SIZE(A9_pmu_resource),
};

static int __init hisi_pmu_init(void)
{
	int ret = 0;
	ret = platform_device_register(&A9_pmu_device);
	return ret;
};
arch_initcall(hisi_pmu_init);
