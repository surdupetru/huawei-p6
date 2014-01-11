/*
 *  linux/arch/arm/mach-k3v2/io.c
 *
 *  Copyright (C) 1999 - 2003 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
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

#include <linux/module.h>
#include <linux/init.h>
#include <asm/mach/map.h>
#include <mach/io.h>
#include <mach/early-debug.h>

#define REG_IOMAP(name) { \
	.virtual =   (REG_BASE_##name##_VIRT),	\
	.pfn     =    __phys_to_pfn(REG_BASE_##name), \
	.length  =    REG_##name##_IOSIZE, \
	.type    =    MT_DEVICE	\
}
#define REG_MMMAP(name) { \
	.virtual =   (REG_BASE_##name##_VIRT),	\
	.pfn     =    __phys_to_pfn(REG_BASE_##name), \
	.length  =    REG_##name##_IOSIZE, \
	.type    =    MT_MEMORY \
}

static struct map_desc hisik3_io_desc[] __initdata = {
	REG_IOMAP(G2D),
	REG_IOMAP(G3D),
	REG_IOMAP(CA9dbgAPB),
	REG_IOMAP(USB2DVC),
	REG_IOMAP(USB2EHCI),
	REG_IOMAP(USB2OHCI),
	REG_IOMAP(NAND),
	REG_IOMAP(NANDC_CFG),
	REG_IOMAP(PCIE_CFG),
#ifdef CONFIG_MACH_TC45MSU3
	REG_IOMAP(USB2),
	REG_IOMAP(USB1),
	REG_IOMAP(USB0),
	REG_IOMAP(GPIO22),
#endif
	REG_IOMAP(VENC),
	REG_IOMAP(MMC3),
	REG_IOMAP(MMC2),
	REG_IOMAP(MMC1),
	REG_IOMAP(MMC0),
	REG_IOMAP(VDEC),
	REG_IOMAP(SECENG),
	REG_IOMAP(MCU),
	REG_IOMAP(DMAC),
	REG_IOMAP(DDRC_CFG),
	REG_IOMAP(PMUSPI),
	REG_IOMAP(GPS),
	REG_IOMAP(SCI),
	REG_IOMAP(I2C3),
	REG_IOMAP(I2C2),
	REG_IOMAP(I2C1),
	REG_IOMAP(I2C0),
	REG_IOMAP(SPI2),
	REG_IOMAP(SPI1),
	REG_IOMAP(SPI0),
	REG_IOMAP(UART4),
	REG_IOMAP(UART3),
	REG_IOMAP(UART2),
	REG_IOMAP(UART1),
	REG_IOMAP(UART0),
	REG_IOMAP(HDMI_EFC),
	REG_IOMAP(EFUSEC),
	REG_IOMAP(PCTRL),
	REG_IOMAP(PMCTRL),
	REG_IOMAP(TBC),
	REG_IOMAP(PWM1),
	REG_IOMAP(PWM0),
	REG_IOMAP(WD),
	REG_IOMAP(TIMER4),
	REG_IOMAP(TIMER3),
	REG_IOMAP(TIMER2),
	REG_IOMAP(TZPC),
	REG_IOMAP(GPIO0),
	REG_IOMAP(GPIO1),
	REG_IOMAP(GPIO2),
	REG_IOMAP(GPIO3),
	REG_IOMAP(GPIO4),
	REG_IOMAP(GPIO5),
	REG_IOMAP(GPIO6),
	REG_IOMAP(GPIO7),
	REG_IOMAP(GPIO8),
	REG_IOMAP(GPIO9),
	REG_IOMAP(GPIO10),
	REG_IOMAP(GPIO11),
	REG_IOMAP(GPIO12),
	REG_IOMAP(GPIO13),
	REG_IOMAP(GPIO14),
	REG_IOMAP(GPIO15),
	REG_IOMAP(GPIO16),
	REG_IOMAP(GPIO17),
	REG_IOMAP(GPIO18),
	REG_IOMAP(GPIO19),
	REG_IOMAP(GPIO20),
	REG_IOMAP(GPIO21),
	REG_IOMAP(KPC),
	REG_IOMAP(RTC),
	REG_IOMAP(IOC),
	REG_IOMAP(SCTRL),
	REG_IOMAP(TIMER1),
	REG_IOMAP(TIMER0),
	REG_IOMAP(A9PER),
	REG_IOMAP(L2CC),
	REG_IOMAP(VPP),
	REG_IOMAP(EDC1),
	REG_IOMAP(EDC0),
	REG_IOMAP(ASP),
	REG_IOMAP(ISP),
	REG_MMMAP(SECRAM),
#ifdef CONFIG_PCI
	REG_IOMAP(PCIE),
#endif
	REG_IOMAP(HDMI),
	REG_IOMAP(MIPIHSI),
};

void __init k3v2_map_common_io(void)
{
	int i = 0;

	iotable_init(hisik3_io_desc, ARRAY_SIZE(hisik3_io_desc));

	edb_init();
	edb_trace(1);

	/* edb info phy -> vir when io mapped */
	for (i = 0; i < ARRAY_SIZE(hisik3_io_desc); i++) {
		edb_putstr(" V: ");     edb_puthex(hisik3_io_desc[i].virtual);
		edb_putstr(" P: ");     edb_puthex(hisik3_io_desc[i].pfn);
		edb_putstr(" S: ");     edb_puthex(hisik3_io_desc[i].length);
		edb_putstr(" T: ");     edb_putul(hisik3_io_desc[i].type);
		edb_putstr("\n");

		if (hisik3_io_desc[i].virtual == IO_ADDRESS(~0)) {
			edb_putstr("<Bad IoTable> P: ");
			edb_puthex(hisik3_io_desc[i].pfn);
			edb_putstr(" V: ");
			edb_puthex(hisik3_io_desc[i].virtual);
			edb_putstr("\n");
		}
	}

#ifdef CONFIG_HAVE_ARM_TWD
	#include <asm/smp_twd.h>
	twd_base = IO_ADDRESS(REG_BASE_A9PER + 0x600);
#endif
	return;
}

/*
 * Intercept ioremap() requests for addresses in our fixed mapping regions.
 */
void __iomem *
k3v2_ioremap(unsigned long phys_addr, size_t size, unsigned int mtype)
{
	void __iomem *v;

	if (phys_addr > 0xF0000000)
		v = (void *) IO_ADDRESS(phys_addr);
	else
		v = __arm_ioremap(phys_addr, size, mtype);

	/*
	 * If the physical address was not physical memory or statically
	 * mapped, there's nothing we can do to map it safely.
	 */
	BUG_ON(v == NULL);

	return v;
}
EXPORT_SYMBOL(k3v2_ioremap);

void k3v2_iounmap(void __iomem *addr)
{
	unsigned long virt = (unsigned long)addr;

	if (virt >= VMALLOC_START && virt < VMALLOC_END)
		__iounmap(addr);

	else if (virt < VMALLOC_START)
		WARN(1, "arch ioumap addr 0x%lx is not in VMALLOC area!\n",
			(unsigned long) addr);
}
EXPORT_SYMBOL(k3v2_iounmap);
