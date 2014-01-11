/*
 *  arch/arm/mach-k3v2/include/mach/io.h
 *
 *  Copyright (C) 2011 Hisilicon Co. Ltd.
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
#ifndef __MACH_K3V2_IO_H
#define __MACH_K3V2_IO_H

#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/platform.h>

#define IO_SPACE_LIMIT 0xffffffff

#define __io(a)	((void __iomem *) a)
#define __mem_pci(a)	(a)

#define IO_TO_VIRT_BETWEEN(p, name)	(((p) >= (REG_BASE_##name)) && ((p) < ((REG_BASE_##name) + (REG_##name##_IOSIZE))))
#define IO_TO_VIRT_XLATE(p, name)	(((p) - (REG_BASE_##name) + (REG_BASE_##name##_VIRT)))

#define IO_TO_VIRT_PRE(n) ( \
	IO_TO_VIRT_BETWEEN((n), USB2DVC) ?	\
		IO_TO_VIRT_XLATE((n), USB2DVC) : \
	IO_TO_VIRT_BETWEEN((n), USB2OHCI) ?	\
		IO_TO_VIRT_XLATE((n), USB2OHCI) : \
	IO_TO_VIRT_BETWEEN((n), USB2EHCI) ?	\
		IO_TO_VIRT_XLATE((n), USB2EHCI) : \
	IO_TO_VIRT_BETWEEN((n), NANDC_CFG) ?	\
		IO_TO_VIRT_XLATE((n), NANDC_CFG) : \
	IO_TO_VIRT_BETWEEN((n), NAND) ?	\
		IO_TO_VIRT_XLATE((n), NAND) : \
	IO_TO_VIRT_BETWEEN((n), PCIE_CFG) ?	\
		IO_TO_VIRT_XLATE((n), PCIE_CFG) : \
	IO_TO_VIRT_BETWEEN((n), MMC3) ?	\
		IO_TO_VIRT_XLATE((n), MMC3) : \
	IO_TO_VIRT_BETWEEN((n), MMC2) ?	\
		IO_TO_VIRT_XLATE((n), MMC2) : \
	IO_TO_VIRT_BETWEEN((n), MMC1) ?	\
		IO_TO_VIRT_XLATE((n), MMC1) : \
	IO_TO_VIRT_BETWEEN((n), MMC0) ?	\
		IO_TO_VIRT_XLATE((n), MMC0) : \
	IO_TO_VIRT_BETWEEN((n), VENC) ?	\
		IO_TO_VIRT_XLATE((n), VENC) : \
	IO_TO_VIRT_BETWEEN((n), VDEC) ?	\
		IO_TO_VIRT_XLATE((n), VDEC) : \
	IO_TO_VIRT_BETWEEN((n), G2D) ?	\
		IO_TO_VIRT_XLATE((n), G2D) : \
	IO_TO_VIRT_BETWEEN((n), G3D) ?	\
		IO_TO_VIRT_XLATE((n), G3D) : \
	IO_TO_VIRT_BETWEEN((n), G2D) ?	\
		IO_TO_VIRT_XLATE((n), G2D) : \
	IO_TO_VIRT_BETWEEN((n), CA9dbgAPB) ?	\
		IO_TO_VIRT_XLATE((n), CA9dbgAPB) : \
	IO_TO_VIRT_BETWEEN((n), SECENG) ?	\
		IO_TO_VIRT_XLATE((n), SECENG) : \
	IO_TO_VIRT_BETWEEN((n), MCU) ?	\
		IO_TO_VIRT_XLATE((n), MCU) : \
	IO_TO_VIRT_BETWEEN((n), DMAC) ?	\
		IO_TO_VIRT_XLATE((n), DMAC) : \
	IO_TO_VIRT_BETWEEN((n), DDRC_CFG) ?	\
		IO_TO_VIRT_XLATE((n), DDRC_CFG) : \
	IO_TO_VIRT_BETWEEN((n), PMUSPI) ?	\
		IO_TO_VIRT_XLATE((n), PMUSPI) : \
	IO_TO_VIRT_BETWEEN((n), GPS) ?	\
		IO_TO_VIRT_XLATE((n), GPS) : \
	IO_TO_VIRT_BETWEEN((n), SCI) ?	\
		IO_TO_VIRT_XLATE((n), SCI) : \
	IO_TO_VIRT_BETWEEN((n), I2C1) ?	\
		IO_TO_VIRT_XLATE((n), I2C1) : \
	IO_TO_VIRT_BETWEEN((n), I2C0) ?	\
		IO_TO_VIRT_XLATE((n), I2C0) : \
	IO_TO_VIRT_BETWEEN((n), SPI2) ?	\
		IO_TO_VIRT_XLATE((n), SPI2) : \
	IO_TO_VIRT_BETWEEN((n), SPI1) ?	\
		IO_TO_VIRT_XLATE((n), SPI1) : \
	IO_TO_VIRT_BETWEEN((n), SPI0) ?	\
		IO_TO_VIRT_XLATE((n), SPI0) : \
	IO_TO_VIRT_BETWEEN((n), UART4) ?	\
		IO_TO_VIRT_XLATE((n), UART4) : \
	IO_TO_VIRT_BETWEEN((n), UART3) ?	\
		IO_TO_VIRT_XLATE((n), UART3) : \
	IO_TO_VIRT_BETWEEN((n), UART2) ?	\
		IO_TO_VIRT_XLATE((n), UART2) : \
	IO_TO_VIRT_BETWEEN((n), UART1) ?	\
		IO_TO_VIRT_XLATE((n), UART1) : \
	IO_TO_VIRT_BETWEEN((n), UART0) ?	\
		IO_TO_VIRT_XLATE((n), UART0) : \
	IO_TO_VIRT_BETWEEN((n), HDMI_EFC) ?	\
		IO_TO_VIRT_XLATE((n), HDMI_EFC) : \
	IO_TO_VIRT_BETWEEN((n), EFUSEC) ?	\
		IO_TO_VIRT_XLATE((n), EFUSEC) : \
	IO_TO_VIRT_BETWEEN((n), PCTRL) ?	\
		IO_TO_VIRT_XLATE((n), PCTRL) : \
	IO_TO_VIRT_BETWEEN((n), PMCTRL) ?	\
		IO_TO_VIRT_XLATE((n), PMCTRL) : \
	IO_TO_VIRT_BETWEEN((n), TBC) ?	\
		IO_TO_VIRT_XLATE((n), TBC) : \
	IO_TO_VIRT_BETWEEN((n), PWM1) ?	\
		IO_TO_VIRT_XLATE((n), PWM1) : \
	IO_TO_VIRT_BETWEEN((n), PWM0) ?	\
		IO_TO_VIRT_XLATE((n), PWM0) : \
	IO_TO_VIRT_BETWEEN((n), WD) ?	\
		IO_TO_VIRT_XLATE((n), WD) : \
	IO_TO_VIRT_BETWEEN((n), TIMER4) ?	\
		IO_TO_VIRT_XLATE((n), TIMER4) : \
	IO_TO_VIRT_BETWEEN((n), TIMER3) ?	\
		IO_TO_VIRT_XLATE((n), TIMER3) : \
	IO_TO_VIRT_BETWEEN((n), TIMER2) ?	\
		IO_TO_VIRT_XLATE((n), TIMER2) : \
	IO_TO_VIRT_BETWEEN((n), TZPC) ?	\
		IO_TO_VIRT_XLATE((n), TZPC) : \
	IO_TO_VIRT_BETWEEN((n), GPIO21) ?	\
		IO_TO_VIRT_XLATE((n), GPIO21) : \
	IO_TO_VIRT_BETWEEN((n), GPIO20) ?	\
		IO_TO_VIRT_XLATE((n), GPIO20) : \
	IO_TO_VIRT_BETWEEN((n), GPIO19) ?	\
		IO_TO_VIRT_XLATE((n), GPIO19) : \
	IO_TO_VIRT_BETWEEN((n), GPIO18) ?	\
		IO_TO_VIRT_XLATE((n), GPIO18) : \
	IO_TO_VIRT_BETWEEN((n), GPIO17) ?	\
		IO_TO_VIRT_XLATE((n), GPIO17) : \
	IO_TO_VIRT_BETWEEN((n), GPIO16) ?	\
		IO_TO_VIRT_XLATE((n), GPIO16) : \
	IO_TO_VIRT_BETWEEN((n), GPIO15) ?	\
		IO_TO_VIRT_XLATE((n), GPIO15) : \
	IO_TO_VIRT_BETWEEN((n), GPIO14) ?	\
		IO_TO_VIRT_XLATE((n), GPIO14) : \
	IO_TO_VIRT_BETWEEN((n), GPIO13) ?	\
		IO_TO_VIRT_XLATE((n), GPIO13) : \
	IO_TO_VIRT_BETWEEN((n), GPIO12) ?	\
		IO_TO_VIRT_XLATE((n), GPIO12) : \
	IO_TO_VIRT_BETWEEN((n), GPIO11) ?	\
		IO_TO_VIRT_XLATE((n), GPIO11) : \
	IO_TO_VIRT_BETWEEN((n), GPIO10) ?	\
		IO_TO_VIRT_XLATE((n), GPIO10) : \
	IO_TO_VIRT_BETWEEN((n), GPIO9) ?	\
		IO_TO_VIRT_XLATE((n), GPIO9) : \
	IO_TO_VIRT_BETWEEN((n), GPIO8) ?	\
		IO_TO_VIRT_XLATE((n), GPIO8) : \
	IO_TO_VIRT_BETWEEN((n), GPIO7) ?	\
		IO_TO_VIRT_XLATE((n), GPIO7) : \
	IO_TO_VIRT_BETWEEN((n), GPIO6) ?	\
		IO_TO_VIRT_XLATE((n), GPIO6) : \
	IO_TO_VIRT_BETWEEN((n), GPIO5) ?	\
		IO_TO_VIRT_XLATE((n), GPIO5) : \
	IO_TO_VIRT_BETWEEN((n), GPIO4) ?	\
		IO_TO_VIRT_XLATE((n), GPIO4) : \
	IO_TO_VIRT_BETWEEN((n), GPIO3) ?	\
		IO_TO_VIRT_XLATE((n), GPIO3) : \
	IO_TO_VIRT_BETWEEN((n), GPIO2) ?	\
		IO_TO_VIRT_XLATE((n), GPIO2) : \
	IO_TO_VIRT_BETWEEN((n), GPIO1) ?	\
		IO_TO_VIRT_XLATE((n), GPIO1) : \
	IO_TO_VIRT_BETWEEN((n), GPIO0) ?	\
		IO_TO_VIRT_XLATE((n), GPIO0) : \
	IO_TO_VIRT_BETWEEN((n), KPC) ?	\
		IO_TO_VIRT_XLATE((n), KPC) : \
	IO_TO_VIRT_BETWEEN((n), RTC) ?	\
		IO_TO_VIRT_XLATE((n), RTC) : \
	IO_TO_VIRT_BETWEEN((n), IOC) ?	\
		IO_TO_VIRT_XLATE((n), IOC) : \
	IO_TO_VIRT_BETWEEN((n), SCTRL) ?	\
		IO_TO_VIRT_XLATE((n), SCTRL) : \
	IO_TO_VIRT_BETWEEN((n), TIMER1) ?	\
		IO_TO_VIRT_XLATE((n), TIMER1) : \
	IO_TO_VIRT_BETWEEN((n), TIMER0) ?	\
		IO_TO_VIRT_XLATE((n), TIMER0) : \
	IO_TO_VIRT_BETWEEN((n), L2CC) ?	\
		IO_TO_VIRT_XLATE((n), L2CC) : \
	IO_TO_VIRT_BETWEEN((n), A9PER) ?	\
		IO_TO_VIRT_XLATE((n), A9PER) : \
	IO_TO_VIRT_BETWEEN((n), VPP) ?	\
		IO_TO_VIRT_XLATE((n), VPP) : \
	IO_TO_VIRT_BETWEEN((n), EDC1) ?	\
		IO_TO_VIRT_XLATE((n), EDC1) : \
	IO_TO_VIRT_BETWEEN((n), HDMI) ?	\
		IO_TO_VIRT_XLATE((n), HDMI) : \
	IO_TO_VIRT_BETWEEN((n), EDC0) ?	\
		IO_TO_VIRT_XLATE((n), EDC0) : \
	IO_TO_VIRT_BETWEEN((n), ASP) ?	\
		IO_TO_VIRT_XLATE((n), ASP) : \
	IO_TO_VIRT_BETWEEN((n), ISP) ?	\
		IO_TO_VIRT_XLATE((n), ISP) : \
	IO_TO_VIRT_BETWEEN((n), SECRAM) ?	\
		IO_TO_VIRT_XLATE((n), SECRAM) : \
	IO_TO_VIRT_BETWEEN((n), I2C2) ?	\
		IO_TO_VIRT_XLATE((n), I2C2) : \
	IO_TO_VIRT_BETWEEN((n), I2C3) ?	\
		IO_TO_VIRT_XLATE((n), I2C3) : \
	IO_TO_VIRT_BETWEEN((n), MIPIHSI) ?	\
		IO_TO_VIRT_XLATE((n), MIPIHSI) : \
	0)

#ifdef CONFIG_PCI
#define IO_TO_VIRT(n)  ( \
	IO_TO_VIRT_BETWEEN((n), PCIE) ?	\
		IO_TO_VIRT_XLATE((n), PCIE) : \
	IO_TO_VIRT_PRE(n))
#else
#define IO_TO_VIRT(n) IO_TO_VIRT_PRE(n)
#endif

#ifndef __ASSEMBLER__

#define __arch_ioremap(p, s, t)	k3v2_ioremap(p, s, t)
#define __arch_iounmap(v)	k3v2_iounmap(v)

extern void __iomem *gic_cpu_base_addr;
extern void __iomem *gic_dist_base_addr;

void k3v2_init_cache(void);
void k3v2_gic_init_irq(void);
void k3v2_map_common_io(void);
void k3v2_common_init(void);
void __iomem *k3v2_ioremap(unsigned long phys, size_t size, unsigned int type);
void k3v2_iounmap(void __iomem *addr);
/*#define IO_ADDRESS(n) ((void __iomem *) IO_TO_VIRT(n))*/

#endif/*__ASSEMBLER__*/

#endif/*__MACH_K3V2_IO_H*/
