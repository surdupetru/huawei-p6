#ifndef _SYS_CTRL_H_
#define _SYS_CTRL_H_

#define MMC_CLK           40000000
#define MMC_CCLK_MAX      20000000
#define MMC_CCLK_MIN      200000

#define ENABLE            1
#define DISABLE           0
#define POWER_ON          0
#define POWER_OFF         1

#define DMA_CHANNEL       4
#define DMA_ONCE_LENGTH   0xFF0
#define DMA_BURST_SIZE    8

#define CARD_PLUGED       0
#define CARD_EJECTED      1
#define CARD_DITHERING    2

#define CARD_READ_ONLY    0


/* Base address of SD card register */
#define HI_MCI_BASE                0xFE100000
#define HI_MCI_IO_SIZE             0x00001000
#define HI_MCI_INTR                48

/* Base address of system control register */
#define SCTL_BASE                   REG_BASE_SCTL
#define SCTL_SC_RSTCTRL2            IO_ADDRESS(SCTL_BASE + 0x48)

/* 0:soft reset  1:undo soft reset */
#define MMC1_SRST                    (1<<15)
#define MMC1_PERIPHCTRL              (1<<30)

#define SCTL_SC_PEREN               IO_ADDRESS(SCTL_BASE + 0x24)
#define SCTL_SC_PERDIS              IO_ADDRESS(SCTL_BASE + 0x28)

/* 0: no care  1:enable clock */
#define MMC1_CLK                    (1<<16)

#define SCTL_SC_PERCTRL0            IO_ADDRESS(SCTL_BASE + 0x1c)

/* mmc1 div coefficient */
#define MMC1_DIV                    3
#define MMC1_DIV_BIT                2

#define IOCFG_BASE                  IO_ADDRESS(REG_BASE_IOCFG)

#if 0     /* 8mA driver */
#define gpio_set_normal(x) do {     \
	writel(0x00, x + 0x0050);       \
	writel(0x14, x + 0x0940);       \
	writel(0x17, x + 0x0944);       \
	writel(0x17, x + 0x0948);       \
	writel(0x17, x + 0x094c);       \
	writel(0x17, x + 0x0950);       \
	writel(0x17, x + 0x0954);       \
} while (0)

#define gpio_set_suspend(x) do {    \
	writel(0x14, x + 0x0944);       \
	writel(0x14, x + 0x0948);       \
	writel(0x14, x + 0x094c);       \
	writel(0x14, x + 0x0950);       \
	writel(0x14, x + 0x0954);       \
} while (0)

#define gpio_set_resume(x) do {     \
	writel(0x17, x + 0x0944);       \
	writel(0x17, x + 0x0948);       \
	writel(0x17, x + 0x094c);       \
	writel(0x17, x + 0x0950);       \
	writel(0x17, x + 0x0954);       \
} while (0)
#endif

#if 1   /* 4mA driver */

#define gpio_set_normal(x) do {      \
	writel(0x00, x + 0x0050);        \
	writel(0x04, x + 0x0940);        \
	writel(0x07, x + 0x0944);        \
	writel(0x07, x + 0x0948);        \
	writel(0x07, x + 0x094c);        \
	writel(0x07, x + 0x0950);        \
	writel(0x07, x + 0x0954);        \
} while (0)

#define gpio_set_suspend(x) do {     \
	writel(0x04, x + 0x0944);        \
	writel(0x04, x + 0x0948);        \
	writel(0x04, x + 0x094c);        \
	writel(0x04, x + 0x0950);        \
	writel(0x04, x + 0x0954);        \
} while (0)

#define gpio_set_resume(x) do {     \
	writel(0x07, x + 0x0944);       \
	writel(0x07, x + 0x0948);       \
	writel(0x07, x + 0x094c);       \
	writel(0x07, x + 0x0950);       \
	writel(0x07, x + 0x0954);       \
} while (0)

#endif

#define gpio_d3_detect(x) do {      \
	writel(0x1, x + 0x0050);        \
	writel(0x4, x + 0x0954);        \
} while (0)

#define gpiod3_detect_done(x) do {  \
	writel(0x0, x + 0x0050);        \
	writel(0x7, x + 0x0954);        \
} while (0)


/* function prototypes */
void sys_ctrl_reset_mmc_controller(void);
void sys_ctrl_de_reset_mmc_controller(void);
void sys_ctrl_himci_init(void);
void sys_ctrl_himci_exit(void);
void sys_ctrl_ldo_on(void);
void sys_ctrl_ldo_off(void);
void sys_ctrl_himci_power(unsigned int flag);
unsigned int sys_ctrl_himci_card_detect(void);
unsigned int sys_ctrl_himci_card_readonly(void);
unsigned int sys_ctrl_detect_enable_wake(unsigned int cardstatus);
unsigned int sys_ctrl_detect_get_interrupt(void);
void sys_ctrl_detect_done_interrupt(void);
unsigned int sys_ctrl_detect_disable_wake(void);

#endif


