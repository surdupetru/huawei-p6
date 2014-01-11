/*
 * linux/drivers/mmc/host/sys_ctrl.c - hisilicon GPIO and Sys control for \
 * MMC Host ccontroller \
 * This program is free software; you can redistribute it and/or modify \
 * it under the terms of the GNU General Public License version 2 as \
 * published by the Free Software Foundation. \
 */

#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include "sys_ctrl.h"


/* external function prototype */
extern void spi_hisipmu_init(void);
extern int spi_hisipmu_reg_read(unsigned char addr, unsigned char *readvalue);
extern int spi_hisipmu_reg_write(unsigned char addr, unsigned char writevalue);
extern int HiOALEnableWake(unsigned int  Irqnum);


#if 0
/* reset MMC host controler */
void sys_ctrl_reset_mmc_controller(void)
{
	unsigned int reg_value;
	unsigned long flags;

	local_irq_save(flags);

	reg_value = readl(SCTL_SC_RSTCTRL2);
	reg_value &= ~MMC1_SRST;
	writel(reg_value, SCTL_SC_RSTCTRL2);

	local_irq_restore(flags);

	/* m53980: maybe we can do some optimization */
	udelay(10);
}

/* de-reset MMC host controler */
void sys_ctrl_de_reset_mmc_controller(void)
{
	unsigned int reg_value;
	unsigned long flags;

	local_irq_save(flags);

	reg_value = readl(SCTL_SC_RSTCTRL2);
	reg_value |= MMC1_SRST;
	writel(reg_value, SCTL_SC_RSTCTRL2);

	local_irq_restore(flags);

	/* m53980:  maybe we can do some optimization */
	udelay(10);
}

/* config power */
void sys_ctrl_himci_power(unsigned int flag)
{
	/* m53980: maybe be deteled */
}

/* hardware initialization */
void sys_ctrl_himci_init(void)
{
	unsigned int tmp_reg;
	unsigned long flags;

	/* step1: set iocfg */
	gpio_set_normal(IOCFG_BASE);

	local_irq_save(flags);

	/* step2: set clk div */
	tmp_reg = readl(SCTL_SC_PERCTRL0);
	tmp_reg &= ~(MMC1_DIV << MMC1_DIV_BIT);
	tmp_reg |= (MMC1_DIV) << MMC1_DIV_BIT;
	writel(tmp_reg, SCTL_SC_PERCTRL0);

	/* step3: enable MMC host input clk */
	tmp_reg = readl(SCTL_SC_PEREN);;
	tmp_reg |= MMC1_CLK;
	writel(tmp_reg, SCTL_SC_PEREN);

	local_irq_restore(flags);

	/* step5: reset controller */
	msleep(2);
	sys_ctrl_reset_mmc_controller();

	/* step6: de-reset controller */
	msleep(2);
	sys_ctrl_de_reset_mmc_controller();

	local_irq_save(flags);

	/* reset periphctrl */
	tmp_reg = readl(SCTL_SC_RSTCTRL2);
	tmp_reg |= MMC1_PERIPHCTRL;
	writel(tmp_reg, SCTL_SC_RSTCTRL2);

	/* clear interrupts */
	tmp_reg = readl((IO_ADDRESS(REG_BASE_MMC1)+0x44));
	writel(tmp_reg, (IO_ADDRESS(REG_BASE_MMC1)+0x44));

	local_irq_restore(flags);
}

/* hardware deinitialization */
void sys_ctrl_himci_exit(void)
{
	unsigned int tmp_reg;
	unsigned long flags;

	/* himci reset */
	sys_ctrl_reset_mmc_controller();

	gpio_set_suspend(IOCFG_BASE);

	/* disable CLK */

	local_irq_save(flags);

	tmp_reg = readl(SCTL_SC_PERDIS);
	tmp_reg |= MMC1_CLK;
	writel(tmp_reg, SCTL_SC_PERDIS);

	local_irq_restore(flags);

}

/* turn ldo2 on */
void sys_ctrl_ldo_on(void)
{
	ANLDO_CTRL_t sd_Ldo;
	sd_Ldo.bEnLDO     =  1; /* enable LDO */
	sd_Ldo.bPowerSave =  0; /* no power saving */
	sd_Ldo.bSetOper   =  1; /* set */
	sd_Ldo.LDO_Volt   =  LDO23_VOLT_30; /* set voltage */
	HiPmuCtl(IOCTL_PMU_LDO2_OPER, (unsigned char *)&sd_Ldo, sizeof(sd_Ldo));

	/* wait */
	msleep(2);
}


/* turn ldo2 off */
void sys_ctrl_ldo_off(void)
{

	ANLDO_CTRL_t sd_Ldo;

	sd_Ldo.bEnLDO     =  0;     /* disable LDO */
	sd_Ldo.bPowerSave =  0;     /* no power saving */
	sd_Ldo.bSetOper   =  1;     /* set */
	sd_Ldo.LDO_Volt   =  LDO23_VOLT_30; /* set voltage */
	HiPmuCtl(IOCTL_PMU_LDO2_OPER, (unsigned char *)&sd_Ldo, sizeof(sd_Ldo));

}

/* detect card status */
unsigned int sys_ctrl_himci_card_detect(void)
{
	unsigned int card_status[3];
	unsigned int i = 0;
	unsigned int status;


	hisik3_gpio_set_direction(GPIO_5_5, EGPIO_DIR_INPUT);

	gpio_d3_detect(IOCFG_BASE);

	/* get sample per 5 ms */
	for (i = 0; i < 3; i++) {
		msleep(5);
		card_status[i] = hisik3_gpio_getvalue(GPIO_5_5);
	}

	if ((card_status[0] != card_status[1]) || \
		(card_status[0] != card_status[2])) {
		status = CARD_DITHERING;
	} else if (card_status[0] == EGPIO_DATA_HIGH) {
		/* card is inserted */
		status = CARD_PLUGED;
	} else {
		/* card is removed */
		status = CARD_EJECTED;
	}

	gpiod3_detect_done(IOCFG_BASE);

	msleep(5);

	return status;
}

/* get readonly status */
unsigned int sys_ctrl_himci_card_readonly(void)
{

	/* always return false */

	unsigned int card_value = 0;

	return card_value;

}

/* detect in suspend */
unsigned int sys_ctrl_detect_enable_wake(unsigned int cardstatus)
{
	gpio_d3_detect(IOCFG_BASE);

	/* gpio input & level triggle & single */
	hisik3_gpio_set_direction(GPIO_5_5, EGPIO_DIR_INPUT);
	hisik3_gpio_setsense(GPIO_5_5, EGPIO_INT_LEVEL);
	hisik3_gpio_setedge(GPIO_5_5, EGPIO_EDGE_SINGLE);

	/* triggle levev depends on card status */
	if (0 == cardstatus) {
		/* wait card removed */
		hisik3_gpio_setevent(GPIO_5_5, EGPIO_EVENT_LOW);
	} else {
		/* wait card inserted */
		hisik3_gpio_setevent(GPIO_5_5, EGPIO_EVENT_HIGH);
	}

	/* set wake interrupt source */
	HiOALEnableWake(GPIO_5_5);

	/* enable interrupt */
	hisik3_gpio_IntrEnable(GPIO_5_5);

	return 0;
}

/* disable detect in resume */
unsigned int sys_ctrl_detect_disable_wake(void)
{
    hisik3_gpio_IntrDisable(GPIO_5_5);

    return 0;
}



/* get d3 interrupt */
unsigned int sys_ctrl_detect_get_interrupt(void)
{
    return hisik3_gpio_Get_IntrStats(GPIO_5_5);
}

/* response to detect interrupt */
void sys_ctrl_detect_done_interrupt(void)
{
    hisik3_gpio_IntrClr(GPIO_5_5);
    hisik3_gpio_IntrDisable(GPIO_5_5);
}
#endif
