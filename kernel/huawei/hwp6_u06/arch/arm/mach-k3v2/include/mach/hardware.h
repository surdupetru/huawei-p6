#ifndef __MACH_K3V2_HARDWARE_H
#define __MACH_K3V2_HARDWARE_H

#include <mach/platform.h>
#include <mach/io.h>

#ifdef MACH_TC45MSU3
#define IO_ADDRESS(addr) ((((addr) & 0xF0000000) == 0xF0000000) ? (addr) : 0)
#else
#define IO_ADDRESS(addr) IO_TO_VIRT(addr)
#endif

#ifdef CONFIG_PCI
#define pcibios_assign_all_busses() (0)
#define PCIBIOS_MIN_IO (0)
#define PCIBIOS_MIN_MEM (0)
#endif

#endif
