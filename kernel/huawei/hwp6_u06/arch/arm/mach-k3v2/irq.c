/*
 *  linux/arch/arm/mach-k3v2/io.c
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

#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/time.h>
#include <linux/io.h>
#include <asm/smp_twd.h>
#include <asm/clkdev.h>
#include <asm/hardware/gic.h>
#include <asm/system.h>
#include <mach/hisi_cortex-a9.h>
#include <mach/early-debug.h>
#include <mach/irqs.h>
#include <mach/io.h>

/*
 * Map io address
 */
void __init k3v2_gic_init_irq(void)
{
	edb_putstr("gic_init_irq\n");

	/* board GIC, primary */
	gic_cpu_base_addr = (void __iomem *)IO_ADDRESS(REG_CPU_A9GIC_BASE);

	gic_dist_base_addr = (void __iomem *)IO_ADDRESS(REG_CPU_A9GICDIST_BASE);

	WARN_ON(gic_cpu_base_addr == NULL);
	WARN_ON(gic_dist_base_addr == NULL);

	/* core tile GIC, primary */
	gic_init(0, IRQ_LOCALTIMER,
		(void __iomem *) IO_ADDRESS(REG_CPU_A9GICDIST_BASE),
		(void __iomem *) gic_cpu_base_addr);
}
