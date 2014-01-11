/*
 * arch/arm/mach-k3v2/include/mach/irqs.h
 *
 * Copyright (c) Hisilicon Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MACH_K3V2_IRQS_H
#define __MACH_K3V2_IRQS_H

#include <asm/irq.h>

#define IRQ_LOCALTIMER	29
#define IRQ_LOCALWDOG	30

#define IRQ_GIC_START	32

#define IRQ_TIMER0		(IRQ_GIC_START + 0)
#define IRQ_TIMER1		(IRQ_GIC_START + 1)
#define IRQ_TIMER2		(IRQ_GIC_START + 2)
#define IRQ_TIMER3		(IRQ_GIC_START + 3)
#define IRQ_TIMER4		(IRQ_GIC_START + 4)
#define IRQ_TIMER5		(IRQ_GIC_START + 5)
#define IRQ_TIMER6		(IRQ_GIC_START + 6)
#define IRQ_TIMER7		(IRQ_GIC_START + 7)
#define IRQ_WDOG		(IRQ_GIC_START + 8)
#define IRQ_RTC			(IRQ_GIC_START + 9)
#define IRQ_KPC			(IRQ_GIC_START + 10)
#define IRQ_TBC			(IRQ_GIC_START + 11)
#define IRQ_DMAC		(IRQ_GIC_START + 12)
#define IRQ_SECENG		(IRQ_GIC_START + 13)
#define IRQ_NANDC		(IRQ_GIC_START + 14)
#define IRQ_L2CC		(IRQ_GIC_START + 15)
#define IRQ_MMC0		(IRQ_GIC_START + 16)
#define IRQ_MMC1		(IRQ_GIC_START + 17)
#define IRQ_MMC2		(IRQ_GIC_START + 18)
#define IRQ_MMC3		(IRQ_GIC_START + 19)
#define IRQ_UART0		(IRQ_GIC_START + 20)
#define IRQ_UART1		(IRQ_GIC_START + 21)

#if defined(CONFIG_CONNECTIVE_CONTROL_FPGA)
#define IRQ_UART2		(IRQ_GIC_START + 126)
#define IRQ_UART3		(IRQ_GIC_START + 127)
#else
#define IRQ_UART2		(IRQ_GIC_START + 22)
#define IRQ_UART3		(IRQ_GIC_START + 23)
#endif
#define IRQ_UART4		(IRQ_GIC_START + 24)
#define IRQ_SPI0		(IRQ_GIC_START + 25)
#define IRQ_SPI1		(IRQ_GIC_START + 26)
#define IRQ_SPI2		(IRQ_GIC_START + 27)
#define IRQ_I2C0		(IRQ_GIC_START + 28)
#define IRQ_I2C1		(IRQ_GIC_START + 29)
#define IRQ_SCI			(IRQ_GIC_START + 30)
#define IRQ_GPS			(IRQ_GIC_START + 31)
#define IRQ_G3D			(IRQ_GIC_START + 32)
#define IRQ_G2D			(IRQ_GIC_START + 33)


#define IRQ_VDEC		(IRQ_GIC_START + 35)
#define IRQ_VPP			(IRQ_GIC_START + 36)
#define IRQ_PGD			(IRQ_GIC_START + 37)
#define IRQ_EDC0		(IRQ_GIC_START + 38)
#define IRQ_LDI0		(IRQ_GIC_START + 39)
#define IRQ_MIPIDSI0		(IRQ_GIC_START + 40)
#define IRQ_EDC1		(IRQ_GIC_START + 41)
#define IRQ_LDI1		(IRQ_GIC_START + 42)
#define IRQ_MIPIDSI1		(IRQ_GIC_START + 43)
#define IRQ_HDMI		(IRQ_GIC_START + 44)
#define IRQ_ASP			(IRQ_GIC_START + 45)
#define IRQ_ASPSPDIF		(IRQ_GIC_START + 46)
#define IRQ_ISP			(IRQ_GIC_START + 47)
#define IRQ_MIPICSI0		(IRQ_GIC_START + 48)
#define IRQ_MIPICSI1		(IRQ_GIC_START + 49)
#if defined(CONFIG_MACH_K3V2OEM1)
#define IRQ_USB2HST0		(IRQ_GIC_START + 50)
#define IRQ_USB2HST1		(IRQ_GIC_START + 51)
#define IRQ_USB2DVC		(IRQ_GIC_START + 52)
#define IRQ_USBOTG		(IRQ_GIC_START + 52)
#define IRQ_VENC		(IRQ_GIC_START + 34)
#elif defined(CONFIG_MACH_TC45MSU3)
/* CONFIG K3 FPGA */
#define IRQ_USB2HST0		91
#define IRQ_USB2HST1		92
#define IRQ_USB2DVC		83
#define IRQ_VENC		(IRQ_GIC_START + 32)
#endif
#define IRQ_PCIEA		(IRQ_GIC_START + 53)
#define IRQ_PCIEB		(IRQ_GIC_START + 54)
#define IRQ_PCIEC		(IRQ_GIC_START + 55)
#define IRQ_PCIED		(IRQ_GIC_START + 56)
#define IRQ_MCU			(IRQ_GIC_START + 57)
#define IRQ_PMCTRL		(IRQ_GIC_START + 58)
#define IRQ_CHARGER		(IRQ_GIC_START + 59)
#define IRQ_AGPS_WAKEUP		(IRQ_GIC_START + 60)
#define IRQ_HSI			(IRQ_GIC_START + 61)
#define IRQ_I2C2		(IRQ_GIC_START + 62)
#define IRQ_I2C3		(IRQ_GIC_START + 63)
#define IRQ_GPIO0		(IRQ_GIC_START + 64)
#define IRQ_GPIO1		(IRQ_GIC_START + 65)
#define IRQ_GPIO2		(IRQ_GIC_START + 66)
#define IRQ_GPIO3		(IRQ_GIC_START + 67)
#define IRQ_GPIO4		(IRQ_GIC_START + 68)
#define IRQ_GPIO5		(IRQ_GIC_START + 69)
#define IRQ_GPIO6		(IRQ_GIC_START + 70)
#define IRQ_GPIO7		(IRQ_GIC_START + 71)
#define IRQ_GPIO8		(IRQ_GIC_START + 72)
#define IRQ_GPIO9		(IRQ_GIC_START + 73)
#define IRQ_GPIO10		(IRQ_GIC_START + 74)
#define IRQ_GPIO11		(IRQ_GIC_START + 75)
#define IRQ_GPIO12		(IRQ_GIC_START + 76)
#define IRQ_GPIO13		(IRQ_GIC_START + 77)
#define IRQ_GPIO14		(IRQ_GIC_START + 78)
#define IRQ_GPIO15		(IRQ_GIC_START + 79)
#define IRQ_GPIO16		(IRQ_GIC_START + 80)
#define IRQ_GPIO17		(IRQ_GIC_START + 81)
#define IRQ_GPIO18		(IRQ_GIC_START + 82)
#define IRQ_GPIO19		(IRQ_GIC_START + 83)
#if defined(CONFIG_CONNECTIVE_CONTROL_FPGA)
#define IRQ_GPIO20		(IRQ_GIC_START + 125)
#else
#define IRQ_GPIO20		(IRQ_GIC_START + 84)
#endif
#define IRQ_GPIO21		(IRQ_GIC_START + 85)
#define IRQ_TIMER8		(IRQ_GIC_START + 96)
#define IRQ_TIMER9		(IRQ_GIC_START + 97)
#define IRQ_TSENSOR0		(IRQ_GIC_START + 98)
#define IRQ_TSENSOR1		(IRQ_GIC_START + 99)
#define IRQ_DVFS_CPU		(IRQ_GIC_START + 100)
#define IRQ_PMIRQ0		(IRQ_GIC_START + 105)
#define IRQ_PMIRQ1		(IRQ_GIC_START + 106)
#define IRQ_PMIRQ2		(IRQ_GIC_START + 107)
#define IRQ_PMIRQ3		(IRQ_GIC_START + 108)
#define IRQ_CPU0IRQ		(IRQ_GIC_START + 112)
#define IRQ_CPU1IRQ		(IRQ_GIC_START + 113)
#define IRQ_CPU2IRQ		(IRQ_GIC_START + 114)
#define IRQ_CPU3IRQ		(IRQ_GIC_START + 115)
#define IRQ_CPU0FRQ		(IRQ_GIC_START + 116)
#define IRQ_CPU1FRQ		(IRQ_GIC_START + 117)
#define IRQ_CPU2FRQ		(IRQ_GIC_START + 118)
#define IRQ_CPU3FRQ		(IRQ_GIC_START + 119)
#define IRQ_COMMRX0		(IRQ_GIC_START + 120)
#define IRQ_COMMRX1		(IRQ_GIC_START + 121)
#define IRQ_COMMRX2		(IRQ_GIC_START + 122)
#define IRQ_COMMRX3		(IRQ_GIC_START + 123)
#define IRQ_COMMTX0		(IRQ_GIC_START + 124)
#define IRQ_COMMTX1		(IRQ_GIC_START + 125)
#define IRQ_COMMTX2		(IRQ_GIC_START + 126)
#define IRQ_COMMTX3		(IRQ_GIC_START + 127)

#define IRQ_GPIO_BASE		(IRQ_GIC_START + 128)
#define IRQ_GPIO(n)		(IRQ_GPIO_BASE + n)
#define IRQ_GPIO159		(IRQ_GPIO_BASE + 159)
#define IRQ_GPIO_NR		(22 * 8)
#define IRQ_PMU_NR_BASE		(IRQ_GPIO_BASE + IRQ_GPIO_NR)
#define IRQ_PMU_NR		(24)

#define IRQ_ALARMON_RISING				(IRQ_PMU_NR_BASE + 0)
#define IRQ_OTMP_RISING					(IRQ_PMU_NR_BASE + 1)
#define IRQ_OCP_RISING					(IRQ_PMU_NR_BASE + 2)
#define IRQ_HRESETN_FALLING				(IRQ_PMU_NR_BASE + 3)
#define IRQ_POWER_KEY_LONG_PRESS_10S		(IRQ_PMU_NR_BASE + 4)
#define IRQ_POWER_KEY_LONG_PRESS_1S		(IRQ_PMU_NR_BASE + 5)
#define IRQ_POWER_KEY_RELEASE	(IRQ_PMU_NR_BASE + 6)
#define IRQ_POWER_KEY_PRESS	(IRQ_PMU_NR_BASE + 7)

#define  IRQ_HEADSET_PLUG_OUT				(IRQ_PMU_NR_BASE + 8)
#define  IRQ_HEADSET_PLUG_IN				(IRQ_PMU_NR_BASE + 9)
#define  IRQ_COUL_RISING				(IRQ_PMU_NR_BASE + 10)
#define  IRQ_VBUS_COMP_VBAT_RISING			(IRQ_PMU_NR_BASE + 11)
#define  IRQ_VBUS_COMP_VBAT_FALLING			(IRQ_PMU_NR_BASE + 12)
#define  IRQ_VBATLOW_RISING				(IRQ_PMU_NR_BASE + 13)
#define  IRQ_VBUS5P5_RISING				(IRQ_PMU_NR_BASE + 14)
#define  IRQ_CHGIN_OK1					(IRQ_PMU_NR_BASE + 15)

#define  IRQ_CHGIN_OK3					(IRQ_PMU_NR_BASE + 16)
#define  IRQ_CHGIN_OK2					(IRQ_PMU_NR_BASE + 17)
#define  IRQ_BSD_BTN_PRESS_DOWN				(IRQ_PMU_NR_BASE + 18)
#define  IRQ_BSD_BTN_PRESS_UP			(IRQ_PMU_NR_BASE + 19)
#define  IRQ_BATT_OK					(IRQ_PMU_NR_BASE + 20)
#define  IRQ_RESET_EN					(IRQ_PMU_NR_BASE + 21)
#define  IRQ_ANA_RSV0_2D				(IRQ_PMU_NR_BASE + 22)
#define  IRQ_RESERVED_PMU				(IRQ_PMU_NR_BASE + 23)

#define NR_IRQS             (IRQ_PMU_NR_BASE + IRQ_PMU_NR)

#ifdef CONFIG_HOTPLUG_CPU

extern int k3v2_irqaffinity_register(unsigned int irq, int cpu);
extern void k3v2_irqaffinity_unregister(unsigned int irq);

#else

static inline int k3v2_irqaffinity_register(unsigned int irq, int cpu)
{
	return 0;
}

static inline void k3v2_irqaffinity_unregister(unsigned int irq) {}

#endif

typedef struct k3v2_irqaffinity_array {
	unsigned int cpu;
	unsigned int irq;
} k3v2_irqaffinity_array;

#endif
