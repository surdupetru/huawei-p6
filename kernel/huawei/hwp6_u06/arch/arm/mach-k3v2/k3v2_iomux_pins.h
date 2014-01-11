#ifndef __MACH_K3V2_IOMUX_PINS_H
#define __MACH_K3V2_IOMUX_PINS_H
#include <mach/platform.h>
#include <mach/io.h>
#include <linux/mux.h>
#include "iomux.h"

/*board id can not be used precompile*/
#define V110_VERSION

extern struct iomux_ops iomux_pin_ops;

#define IOMUX_IOMG(_iomg, _iomg_name, _iomg_reg, _func_array)\
struct iomux_iomg _iomg = {\
	.name = _iomg_name,\
	.iomg_reg = (void __iomem	*)(IO_ADDRESS(REG_BASE_IOC) + _iomg_reg),\
	.regValue = _func_array,\
};

#define IOMUX_IOCG(_iocg, _iocg_name, _iocg_reg, _iocg_pud_mask, _iocg_drvstrength_mask)    \
struct iomux_iocg _iocg = {\
	.name = _iocg_name,\
	.iocg_reg = (void __iomem	*)(IO_ADDRESS(REG_BASE_IOC) + _iocg_reg),\
	.iocg_pullupdown_mask = _iocg_pud_mask,\
	.iocg_drivestrength_mask = _iocg_drvstrength_mask,\
};

#define IOMUX_PIN(_iomux_pin, _iomux_pin_name, _pin_func, _pin_pull_updown, \
_pin_drive_strength, _pin_iomg, _pin_iocg)    \
struct  iomux_pin _iomux_pin = {\
	.pin_name = _iomux_pin_name,\
	.pin_func  = _pin_func,\
	.pin_pull_updown = _pin_pull_updown,\
	.pin_drive_strength = _pin_drive_strength,\
	.ops  =  &iomux_pin_ops,\
	.pin_iomg = _pin_iomg,\
	.pin_iocg  = _pin_iocg,\
	.init = 0, \
};

#define PIN_TABLE(_pinname, _iomux_pin)	\
{\
	.pinname = _pinname,\
	.iomux_pin = _iomux_pin,\
}

/*define the iomg*/
int func_array1[] = {0, 1, RESERVE};
int func_array2[] = {0, 1, 2, RESERVE};
int func_array3[] = {0, 1, 2, 3, RESERVE};
int func_array4[] = {0, 1, 2, 3, 4, 5, 6, RESERVE};

IOMUX_IOMG(iomg0, "iomg0", 0x000, func_array1)
IOMUX_IOMG(iomg1, "iomg1", 0x004, func_array2)
IOMUX_IOMG(iomg2, "iomg2", 0x008, func_array1)
IOMUX_IOMG(iomg3, "iomg3", 0x00C, func_array2)
IOMUX_IOMG(iomg4, "iomg4", 0x010, func_array2)
IOMUX_IOMG(iomg5, "iomg5", 0x014, func_array2)
IOMUX_IOMG(iomg6, "iomg6", 0x018, func_array2)
IOMUX_IOMG(iomg7, "iomg7", 0x020, func_array3)
IOMUX_IOMG(iomg8, "iomg8", 0x024, func_array3)
IOMUX_IOMG(iomg9, "iomg9", 0x028, func_array3)
IOMUX_IOMG(iomg10, "iomg10", 0x02C, func_array3)
IOMUX_IOMG(iomg12, "iomg12", 0x030, func_array3)
IOMUX_IOMG(iomg13, "iomg13", 0x034, func_array3)
IOMUX_IOMG(iomg14, "iomg14", 0x038, func_array3)
IOMUX_IOMG(iomg15, "iomg15", 0x03C, func_array3)
IOMUX_IOMG(iomg16, "iomg16", 0x040, func_array4)
IOMUX_IOMG(iomg17, "iomg17", 0x044, func_array4)
IOMUX_IOMG(iomg18, "iomg18", 0x048, func_array4)
IOMUX_IOMG(iomg19, "iomg19", 0x04C, func_array4)
IOMUX_IOMG(iomg20, "iomg20", 0x050, func_array4)/*func2:i2c3_scl*/
IOMUX_IOMG(iomg21, "iomg21", 0x054, func_array2)/*func2:i2c3_sda*/
IOMUX_IOMG(iomg22, "iomg22", 0x058, func_array1)
IOMUX_IOMG(iomg23, "iomg23", 0x05C, func_array1)
IOMUX_IOMG(iomg24, "iomg24", 0x060, func_array3)
IOMUX_IOMG(iomg25, "iomg25", 0x064, func_array3)
IOMUX_IOMG(iomg26, "iomg26", 0x068, func_array2)/*func0:I2C2_SCL*/
IOMUX_IOMG(iomg27, "iomg27", 0x06C, func_array2)/*func0:I2C2_SDA*/
IOMUX_IOMG(iomg28, "iomg28", 0x070, func_array1)
IOMUX_IOMG(iomg29, "iomg29", 0x074, func_array1)
IOMUX_IOMG(iomg30, "iomg30", 0x078, func_array1)
IOMUX_IOMG(iomg31, "iomg31", 0x07C, func_array1)
IOMUX_IOMG(iomg32, "iomg32", 0x080, func_array1)
IOMUX_IOMG(iomg33, "iomg33", 0x084, func_array1)
IOMUX_IOMG(iomg34, "iomg34", 0x088, func_array1)
IOMUX_IOMG(iomg35, "iomg35", 0x08C, func_array2)/*func2:DSI0_TE0*/
IOMUX_IOMG(iomg36, "iomg36", 0x090, func_array2)/*func2:DSI1_TE0*/
IOMUX_IOMG(iomg37, "iomg37", 0x094, func_array1)
IOMUX_IOMG(iomg38, "iomg38", 0x098, func_array2)
IOMUX_IOMG(iomg39, "iomg39", 0x09C, func_array3)
IOMUX_IOMG(iomg40, "iomg40", 0x0A0, func_array2)
IOMUX_IOMG(iomg41, "iomg41", 0x0A4, func_array3)
IOMUX_IOMG(iomg42, "iomg42", 0x0A8, func_array1)
IOMUX_IOMG(iomg43, "iomg43", 0x0AC, func_array1)
IOMUX_IOMG(iomg44, "iomg44", 0x0B0, func_array1)
IOMUX_IOMG(iomg45, "iomg45", 0x0B4, func_array1)
IOMUX_IOMG(iomg46, "iomg46", 0x0B8, func_array1)
IOMUX_IOMG(iomg47, "iomg47", 0x0BC, func_array1)
IOMUX_IOMG(iomg48, "iomg48", 0x0C0, func_array1)
IOMUX_IOMG(iomg49, "iomg49", 0x0C4, func_array2)/*func2:hsi*/
IOMUX_IOMG(iomg50, "iomg50", 0x0C8, func_array2)/*func2:hsi*/
IOMUX_IOMG(iomg51, "iomg51", 0x0CC, func_array1)
IOMUX_IOMG(iomg52, "iomg52", 0x0D0, func_array1)
IOMUX_IOMG(iomg53, "iomg53", 0x0D4, func_array1)
IOMUX_IOMG(iomg54, "iomg54", 0x0D8, func_array1)
IOMUX_IOMG(iomg55, "iomg55", 0x0DC, func_array1)
IOMUX_IOMG(iomg56, "iomg56", 0x0E0, func_array1)
IOMUX_IOMG(iomg57, "iomg57", 0x0E4, func_array2)
IOMUX_IOMG(iomg58, "iomg58", 0x0E8, func_array1)
IOMUX_IOMG(iomg59, "iomg59", 0x0F0, func_array1)
IOMUX_IOMG(iomg60, "iomg60", 0x0F4, func_array1)
IOMUX_IOMG(iomg61, "iomg61", 0x0F8, func_array2)
IOMUX_IOMG(iomg62, "iomg62", 0x0FC, func_array2)
IOMUX_IOMG(iomg63, "iomg63", 0x100, func_array2)
IOMUX_IOMG(iomg64, "iomg64", 0x108, func_array2)
IOMUX_IOMG(iomg65, "iomg65", 0x10C, func_array3)
IOMUX_IOMG(iomg66, "iomg66", 0x110, func_array3)
IOMUX_IOMG(iomg67, "iomg67", 0x114, func_array3)
IOMUX_IOMG(iomg68, "iomg68", 0x118, func_array3)
IOMUX_IOMG(iomg69, "iomg69", 0x11C, func_array3)
IOMUX_IOMG(iomg70, "iomg70", 0x120, func_array3)
IOMUX_IOMG(iomg71, "iomg71", 0x124, func_array3)
IOMUX_IOMG(iomg72, "iomg72", 0x128, func_array3)
IOMUX_IOMG(iomg73, "iomg73", 0x12C, func_array3)
IOMUX_IOMG(iomg74, "iomg74", 0x130, func_array3)
IOMUX_IOMG(iomg75, "iomg75", 0x134, func_array3)
IOMUX_IOMG(iomg76, "iomg76", 0x138, func_array3)
IOMUX_IOMG(iomg77, "iomg77", 0x13C, func_array3)
IOMUX_IOMG(iomg78, "iomg78", 0x140, func_array3)
IOMUX_IOMG(iomg79, "iomg79", 0x144, func_array3)
IOMUX_IOMG(iomg80, "iomg80", 0x148, func_array3)
IOMUX_IOMG(iomg81, "iomg81", 0x14C, func_array3)
IOMUX_IOMG(iomg82, "iomg82", 0x154, func_array1)
IOMUX_IOMG(iomg83, "iomg83", 0x158, func_array1)
IOMUX_IOMG(iomg84, "iomg84", 0x15C, func_array1)
IOMUX_IOMG(iomg85, "iomg85", 0x160, func_array2)
IOMUX_IOMG(iomg86, "iomg86", 0x164, func_array2)
IOMUX_IOMG(iomg87, "iomg87", 0x168, func_array2)/*func0:uart4,func1:gpio*/
IOMUX_IOMG(iomg88, "iomg88", 0x16C, func_array2)/*func0:uart4,func1:gpio*/
/*there are no iomg89 and iomg91 in hi3620v110*/
IOMUX_IOMG(iomg89, "iomg89", 0x174, func_array1)
IOMUX_IOMG(iomg91, "iomg91", 0x17C, func_array1)
IOMUX_IOMG(iomg90, "iomg90", 0x178, func_array1)
IOMUX_IOMG(iomg92, "iomg92", 0x180, func_array1)
IOMUX_IOMG(iomg93, "iomg93", 0x170, func_array2)/*func0:uart4,func1:gpio*/
IOMUX_IOMG(iomg94, "iomg94", 0x01C, func_array3)
IOMUX_IOMG(iomg95, "iomg95", 0x0EC, func_array1)
IOMUX_IOMG(iomg96, "iomg96", 0x104, func_array2)
IOMUX_IOMG(iomg97, "iomg97", 0x150, func_array3)
IOMUX_IOMG(iomg98, "iomg98", 0x184, func_array2)/*func0:spi,func1:gpio,func2:hsi*/

/*define iocg*/
IOMUX_IOCG(iocg191, "iocg191", 0x800, 0x0000, 0x00F0)
IOMUX_IOCG(iocg192, "iocg192", 0x804, 0x0003, 0x00F0)
IOMUX_IOCG(iocg193, "iocg193", 0x808, 0x0000, 0x00F0)
IOMUX_IOCG(iocg0, "iocg0", 0x80C, 0x0003, 0x0000)
IOMUX_IOCG(iocg1, "iocg1", 0x810, 0x0003, 0x0000)
IOMUX_IOCG(iocg2, "iocg2", 0x814, 0x0003, 0x0000)
IOMUX_IOCG(iocg3, "iocg3", 0x818, 0x0003, 0x0000)
IOMUX_IOCG(iocg4, "iocg4", 0x81C, 0x0001, 0x0000)
IOMUX_IOCG(iocg5, "iocg5", 0x820, 0x0001, 0x0000)
IOMUX_IOCG(iocg6, "iocg6", 0x824, 0x0003, 0x0000)
IOMUX_IOCG(iocg7, "iocg7", 0x828, 0x0003, 0x0000)
IOMUX_IOCG(iocg8, "iocg8", 0x82C, 0x0003, 0x0000)
IOMUX_IOCG(iocg9, "iocg9", 0x830, 0x0003, 0x0000)
IOMUX_IOCG(iocg10, "iocg10", 0x834, 0x0003, 0x0000)
IOMUX_IOCG(iocg11, "iocg11", 0x838, 0x0003, 0x0000)
IOMUX_IOCG(iocg12, "iocg12", 0x83C, 0x0003, 0x00F0)
IOMUX_IOCG(iocg13, "iocg13", 0x840, 0x0003, 0x00F0)
IOMUX_IOCG(iocg14, "iocg14", 0x844, 0x0003, 0x00F0)
IOMUX_IOCG(iocg15, "iocg15", 0x848, 0x0003, 0x00F0)
IOMUX_IOCG(iocg16, "iocg16", 0x84C, 0x0003, 0x00F0)
IOMUX_IOCG(iocg17, "iocg17", 0x850, 0x0003, 0x00F0)
IOMUX_IOCG(iocg18, "iocg18", 0x854, 0x0003, 0x00F0)
IOMUX_IOCG(iocg19, "iocg19", 0x858, 0x0003, 0x00F0)
IOMUX_IOCG(iocg20, "iocg20", 0x85C, 0x0003, 0x0000)
IOMUX_IOCG(iocg21, "iocg21", 0x860, 0x0003, 0x0000)
IOMUX_IOCG(iocg22, "iocg22", 0x864, 0x0003, 0x0000)
IOMUX_IOCG(iocg23, "iocg23", 0x868, 0x0003, 0x0000)
IOMUX_IOCG(iocg24, "iocg24", 0x86C, 0x0003, 0x00F0)
IOMUX_IOCG(iocg25, "iocg25", 0x870, 0x0003, 0x00F0)
IOMUX_IOCG(iocg26, "iocg26", 0x874, 0x0003, 0x00F0)
IOMUX_IOCG(iocg27, "iocg27", 0x878, 0x0003, 0x00F0)
IOMUX_IOCG(iocg28, "iocg28", 0x87C, 0x0003, 0x00F0)
IOMUX_IOCG(iocg29, "iocg29", 0x880, 0x0003, 0x00F0)
IOMUX_IOCG(iocg30, "iocg30", 0x884, 0x0003, 0x00F0)
IOMUX_IOCG(iocg31, "iocg31", 0x888, 0x0003, 0x00F0)
IOMUX_IOCG(iocg32, "iocg32", 0x88C, 0x0003, 0x00F0)
IOMUX_IOCG(iocg33, "iocg33", 0x890, 0x0003, 0x00F0)
IOMUX_IOCG(iocg34, "iocg34", 0x894, 0x0003, 0x00F0)
IOMUX_IOCG(iocg35, "iocg35", 0x898, 0x0003, 0x00F0)
IOMUX_IOCG(iocg36, "iocg36", 0x89C, 0x0003, 0x00F0)
IOMUX_IOCG(iocg37, "iocg37", 0x8A0, 0x0003, 0x00F0)
IOMUX_IOCG(iocg38, "iocg38", 0x8A4, 0x0003, 0x00F0)
IOMUX_IOCG(iocg39, "iocg39", 0x8A8, 0x0003, 0x00F0)
IOMUX_IOCG(iocg40, "iocg40", 0x8AC, 0x0003, 0x00F0)
IOMUX_IOCG(iocg41, "iocg41", 0x8B0, 0x0003, 0x00F0)
IOMUX_IOCG(iocg54, "iocg54", 0x8B4, 0x0003, 0x0000)
IOMUX_IOCG(iocg55, "iocg55", 0x8B8, 0x0003, 0x0000)
IOMUX_IOCG(iocg56, "iocg56", 0x8BC, 0x0003, 0x0000)
IOMUX_IOCG(iocg57, "iocg57", 0x8C0, 0x0003, 0x0000)
IOMUX_IOCG(iocg58, "iocg58", 0x8C4, 0x0003, 0x0000)
IOMUX_IOCG(iocg59, "iocg59", 0x8C8, 0x0003, 0x0000)
IOMUX_IOCG(iocg60, "iocg60", 0x8CC, 0x0003, 0x0000)
IOMUX_IOCG(iocg61, "iocg61", 0x8D0, 0x0003, 0x0000)
IOMUX_IOCG(iocg62, "iocg62", 0x8D4, 0x0003, 0x0000)
IOMUX_IOCG(iocg63, "iocg63", 0x8D8, 0x0003, 0x0000)
IOMUX_IOCG(iocg64, "iocg64", 0x8DC, 0x0003, 0x0000)
IOMUX_IOCG(iocg65, "iocg65", 0x8E0, 0x0003, 0x0000)
IOMUX_IOCG(iocg66, "iocg66", 0x8E4, 0x0003, 0x0000)
IOMUX_IOCG(iocg67, "iocg67", 0x8E8, 0x0003, 0x0000)
IOMUX_IOCG(iocg68, "iocg68", 0x8EC, 0x0003, 0x0000)
IOMUX_IOCG(iocg69, "iocg69", 0x8F0, 0x0003, 0x0000)
IOMUX_IOCG(iocg70, "iocg70", 0x8F4, 0x0003, 0x0000)
IOMUX_IOCG(iocg71, "iocg71", 0x8F8, 0x0003, 0x0000)
IOMUX_IOCG(iocg72, "iocg72", 0x8FC, 0x0003, 0x0000)
IOMUX_IOCG(iocg73, "iocg73", 0x900, 0x0003, 0x0000)
IOMUX_IOCG(iocg74, "iocg74", 0x904, 0x0003, 0x0000)
IOMUX_IOCG(iocg75, "iocg75", 0x908, 0x0003, 0x0000)
IOMUX_IOCG(iocg76, "iocg76", 0x90C, 0x0003, 0x0000)
IOMUX_IOCG(iocg77, "iocg77", 0x910, 0x0003, 0x0000)
IOMUX_IOCG(iocg78, "iocg78", 0x914, 0x0003, 0x0000)
IOMUX_IOCG(iocg79, "iocg79", 0x918, 0x0003, 0x0000)
IOMUX_IOCG(iocg80, "iocg80", 0x91C, 0x0003, 0x0000)
IOMUX_IOCG(iocg81, "iocg81", 0x920, 0x0003, 0x0000)
IOMUX_IOCG(iocg82, "iocg82", 0x924, 0x0003, 0x0000)
IOMUX_IOCG(iocg83, "iocg83", 0x928, 0x0003, 0x00F0)
IOMUX_IOCG(iocg84, "iocg84", 0x92C, 0x0003, 0x00F0)
IOMUX_IOCG(iocg85, "iocg85", 0x930, 0x0003, 0x0000)
IOMUX_IOCG(iocg86, "iocg86", 0x934, 0x0003, 0x0000)
IOMUX_IOCG(iocg87, "iocg87", 0x938, 0x0003, 0x0000)
IOMUX_IOCG(iocg88, "iocg88", 0x93C, 0x0003, 0x0000)
IOMUX_IOCG(iocg89, "iocg89", 0x940, 0x0003, 0x0000)
IOMUX_IOCG(iocg90, "iocg90", 0x944, 0x0003, 0x0000)
IOMUX_IOCG(iocg91, "iocg91", 0x948, 0x0003, 0x0000)
IOMUX_IOCG(iocg92, "iocg92", 0x94C, 0x0003, 0x0000)
IOMUX_IOCG(iocg93, "iocg93", 0x950, 0x0003, 0x0000)
IOMUX_IOCG(iocg94, "iocg94", 0x954, 0x0003, 0x00F0)
IOMUX_IOCG(iocg95, "iocg95", 0x958, 0x0002, 0x0000)
IOMUX_IOCG(iocg194, "iocg194", 0x95C, 0x0000, 0x00F0)
IOMUX_IOCG(iocg96, "iocg96", 0x960, 0x0002, 0x0000)
IOMUX_IOCG(iocg97, "iocg97", 0x964, 0x0002, 0x0000)
IOMUX_IOCG(iocg98, "iocg98", 0x968, 0x0003, 0x00F0)
IOMUX_IOCG(iocg99, "iocg99", 0x96C, 0x0003, 0x00F0)
IOMUX_IOCG(iocg100, "iocg100", 0x970, 0x0003, 0x00F0)
IOMUX_IOCG(iocg101, "iocg101", 0x974, 0x0003, 0x00F0)
IOMUX_IOCG(iocg102, "iocg102", 0x978, 0x0003, 0x00F0)
IOMUX_IOCG(iocg103, "iocg103", 0x97C, 0x0003, 0x0000)
IOMUX_IOCG(iocg104, "iocg104", 0x980, 0x0003, 0x0000)
IOMUX_IOCG(iocg105, "iocg105", 0x984, 0x0003, 0x0000)
IOMUX_IOCG(iocg106, "iocg106", 0x988, 0x0003, 0x0000)
IOMUX_IOCG(iocg107, "iocg107", 0x98C, 0x0003, 0x00F0)
IOMUX_IOCG(iocg108, "iocg108", 0x990, 0x0003, 0x00F0)
IOMUX_IOCG(iocg109, "iocg109", 0x994, 0x0003, 0x00F0)
IOMUX_IOCG(iocg110, "iocg110", 0x998, 0x0003, 0x00F0)
IOMUX_IOCG(iocg111, "iocg111", 0x99C, 0x0003, 0x00F0)
IOMUX_IOCG(iocg112, "iocg112", 0x9A0, 0x0003, 0x00F0)
IOMUX_IOCG(iocg113, "iocg113", 0x9A4, 0x0003, 0x00F0)
IOMUX_IOCG(iocg114, "iocg114", 0x9A8, 0x0003, 0x00F0)
IOMUX_IOCG(iocg115, "iocg115", 0x9AC, 0x0003, 0x00F0)
IOMUX_IOCG(iocg116, "iocg116", 0x9B0, 0x0003, 0x00F0)
IOMUX_IOCG(iocg117, "iocg117", 0x9B4, 0x0003, 0x00F0)
IOMUX_IOCG(iocg118, "iocg118", 0x9B8, 0x0003, 0x00F0)
IOMUX_IOCG(iocg119, "iocg119", 0x9BC, 0x0003, 0x00F0)
IOMUX_IOCG(iocg120, "iocg120", 0x9C0, 0x0003, 0x00F0)
IOMUX_IOCG(iocg121, "iocg121", 0x9C4, 0x0003, 0x00F0)
IOMUX_IOCG(iocg122, "iocg122", 0x9C8, 0x0003, 0x00F0)
IOMUX_IOCG(iocg123, "iocg123", 0x9CC, 0x0003, 0x00F0)
IOMUX_IOCG(iocg124, "iocg124", 0x9D0, 0x0003, 0x00F0)
IOMUX_IOCG(iocg125, "iocg125", 0x9D4, 0x0003, 0x00F0)
IOMUX_IOCG(iocg126, "iocg126", 0x9D8, 0x0003, 0x00F0)
IOMUX_IOCG(iocg127, "iocg127", 0x9DC, 0x0003, 0x00F0)
IOMUX_IOCG(iocg128, "iocg128", 0x9E0, 0x0003, 0x00F0)
IOMUX_IOCG(iocg129, "iocg129", 0x9E4, 0x0003, 0x00F0)
IOMUX_IOCG(iocg130, "iocg130", 0x9E8, 0x0003, 0x00F0)
IOMUX_IOCG(iocg131, "iocg131", 0x9EC, 0x0003, 0x00F0)
IOMUX_IOCG(iocg132, "iocg132", 0x9F0, 0x0003, 0x00F0)
IOMUX_IOCG(iocg133, "iocg133", 0x9F4, 0x0003, 0x00F0)
IOMUX_IOCG(iocg134, "iocg134", 0x9F8, 0x0003, 0x00F0)
IOMUX_IOCG(iocg135, "iocg135", 0x9FC, 0x0003, 0x00F0)
IOMUX_IOCG(iocg136, "iocg136", 0xA00, 0x0003, 0x0000)
IOMUX_IOCG(iocg137, "iocg137", 0xA04, 0x0003, 0x0000)
IOMUX_IOCG(iocg138, "iocg138", 0xA08, 0x0003, 0x0000)
IOMUX_IOCG(iocg139, "iocg139", 0xA0C, 0x0003, 0x0000)
IOMUX_IOCG(iocg140, "iocg140", 0xA10, 0x0003, 0x0000)
IOMUX_IOCG(iocg141, "iocg141", 0xA14, 0x0003, 0x0000)
IOMUX_IOCG(iocg142, "iocg142", 0xA18, 0x0003, 0x0000)
IOMUX_IOCG(iocg143, "iocg143", 0xA1C, 0x0003, 0x0000)
IOMUX_IOCG(iocg144, "iocg144", 0xA20, 0x0003, 0x0000)
IOMUX_IOCG(iocg145, "iocg145", 0xA24, 0x0003, 0x0000)
IOMUX_IOCG(iocg146, "iocg146", 0xA28, 0x0003, 0x0000)
IOMUX_IOCG(iocg147, "iocg147", 0xA2C, 0x0003, 0x0000)
IOMUX_IOCG(iocg148, "iocg148", 0xA30, 0x0003, 0x0000)
IOMUX_IOCG(iocg149, "iocg149", 0xA34, 0x0003, 0x0000)
IOMUX_IOCG(iocg150, "iocg150", 0xA38, 0x0003, 0x0000)
IOMUX_IOCG(iocg151, "iocg151", 0xA3C, 0x0003, 0x0000)
IOMUX_IOCG(iocg152, "iocg152", 0xA40, 0x0003, 0x0000)
IOMUX_IOCG(iocg153, "iocg153", 0xA44, 0x0003, 0x0000)
IOMUX_IOCG(iocg154, "iocg154", 0xA48, 0x0003, 0x0000)
IOMUX_IOCG(iocg155, "iocg155", 0xA4C, 0x0003, 0x0000)
IOMUX_IOCG(iocg156, "iocg156", 0xA50, 0x0003, 0x0000)
IOMUX_IOCG(iocg157, "iocg157", 0xA5C, 0x0003, 0x0000)
IOMUX_IOCG(iocg158, "iocg158", 0xA58, 0x0003, 0x0000)
IOMUX_IOCG(iocg159, "iocg159", 0xA5C, 0x0003, 0x0000)
IOMUX_IOCG(iocg160, "iocg160", 0xA60, 0x0003, 0x0000)
IOMUX_IOCG(iocg161, "iocg161", 0xA64, 0x0003, 0x0000)
IOMUX_IOCG(iocg162, "iocg162", 0xA68, 0x0003, 0x0000)
IOMUX_IOCG(iocg163, "iocg163", 0xA6C, 0x0003, 0x0000)
IOMUX_IOCG(iocg164, "iocg164", 0xA70, 0x0003, 0x0000)
IOMUX_IOCG(iocg165, "iocg165", 0xA74, 0x0003, 0x0000)
IOMUX_IOCG(iocg166, "iocg166", 0xA78, 0x0003, 0x0000)
IOMUX_IOCG(iocg167, "iocg167", 0xA7C, 0x0003, 0x0000)
IOMUX_IOCG(iocg168, "iocg168", 0xA80, 0x0003, 0x00F0)
IOMUX_IOCG(iocg169, "iocg169", 0xA84, 0x0003, 0x00F0)
IOMUX_IOCG(iocg170, "iocg170", 0xA88, 0x0003, 0x0000)
IOMUX_IOCG(iocg171, "iocg171", 0xA8C, 0x0003, 0x0000)
IOMUX_IOCG(iocg172, "iocg172", 0xA90, 0x0003, 0x0000)
IOMUX_IOCG(iocg173, "iocg173", 0xA94, 0x0003, 0x00F0)
IOMUX_IOCG(iocg174, "iocg174", 0xA98, 0x0003, 0x0000)
IOMUX_IOCG(iocg175, "iocg175", 0xA9C, 0x0003, 0x00F0)
IOMUX_IOCG(iocg176, "iocg176", 0xAA0, 0x0003, 0x00F0)
IOMUX_IOCG(iocg177, "iocg177", 0xAA4, 0x0003, 0x0000)
IOMUX_IOCG(iocg178, "iocg178", 0xAA8, 0x0003, 0x0000)
IOMUX_IOCG(iocg179, "iocg179", 0xAAC, 0x0003, 0x0000)
IOMUX_IOCG(iocg180, "iocg180", 0xAB0, 0x0003, 0x0000)
IOMUX_IOCG(iocg181, "iocg181", 0xAB4, 0x0003, 0x00F0)
IOMUX_IOCG(iocg182, "iocg182", 0xAB8, 0x0003, 0x00F0)
IOMUX_IOCG(iocg183, "iocg183", 0xABC, 0x0003, 0x00F0)
IOMUX_IOCG(iocg184, "iocg184", 0xAC0, 0x0003, 0x00F0)
IOMUX_IOCG(iocg185, "iocg185", 0xAC4, 0x0003, 0x0000)
IOMUX_IOCG(iocg186, "iocg186", 0xAC8, 0x0003, 0x0000)
IOMUX_IOCG(iocg187, "iocg187", 0xACC, 0x0003, 0x0000)
IOMUX_IOCG(iocg188, "iocg188", 0xAD0, 0x0003, 0x0000)
IOMUX_IOCG(iocg189, "iocg189", 0xAD4, 0x0003, 0x0000)
IOMUX_IOCG(iocg190, "iocg190", 0xAD8, 0x0003, 0x00F0)

/*-----------------------------ES----------------------------------*/
/*define pins*/
IOMUX_PIN(gpio_002, "efuse_sel", FUNC0, PULLDOWN, LEVEL2, &iomg0, &iocg6)
IOMUX_PIN(gpio_003, "efuse_csb", FUNC0, PULLDOWN, LEVEL2, &iomg0, &iocg7)
IOMUX_PIN(gpio_004, "efuse_sclk", FUNC0, PULLDOWN, LEVEL2, &iomg0, &iocg8)
IOMUX_PIN(gpio_005, "efuse_pgm", FUNC0, PULLDOWN, LEVEL2, &iomg0, &iocg9)
IOMUX_PIN(gpio_006, "efuse_din", FUNC0, PULLDOWN, LEVEL2, &iomg1, &iocg10)
IOMUX_PIN(gpio_007, "efuse_dout", FUNC0, PULLDOWN, RESERVE, &iomg2, &iocg11)
IOMUX_PIN(gpio_008, "nand_ale", FUNC0, NOPULL, LEVEL2, &iomg3, &iocg12)
IOMUX_PIN(gpio_009, "nand_cle", FUNC0, NOPULL, LEVEL2, &iomg3, &iocg13)
IOMUX_PIN(gpio_010, "nand_re_n", FUNC0, NOPULL, LEVEL2, &iomg3, &iocg14)
IOMUX_PIN(gpio_011, "nand_we_n", FUNC0, NOPULL, LEVEL2, &iomg3, &iocg15)
IOMUX_PIN(gpio_012, "nand_cs0_n", FUNC0, NOPULL, LEVEL2, &iomg3, &iocg16)
IOMUX_PIN(gpio_013, "nand_cs1_n", FUNC0, NOPULL, LEVEL2, &iomg4, &iocg17)
IOMUX_PIN(gpio_014, "nand_cs2_n", FUNC0, NOPULL, LEVEL2, &iomg5, &iocg18)
IOMUX_PIN(gpio_015, "nand_cs3_n", FUNC0, NOPULL, LEVEL2, &iomg6, &iocg19)
IOMUX_PIN(gpio_016, "nand_busy0_n", FUNC0, PULLUP, LEVEL2, &iomg94, &iocg20)
IOMUX_PIN(gpio_017, "nand_busy1_n", FUNC0, PULLUP, LEVEL2, &iomg7, &iocg21)
IOMUX_PIN(gpio_018, "nand_busy2_n", FUNC0, PULLUP, LEVEL2, &iomg8, &iocg22)
IOMUX_PIN(gpio_019, "nand_busy3_n", FUNC0, PULLUP, LEVEL2, &iomg9, &iocg23)
IOMUX_PIN(gpio_020, "nand_data0", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg24)
IOMUX_PIN(gpio_021, "nand_data1", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg25)
IOMUX_PIN(gpio_022, "nand_data2", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg26)
IOMUX_PIN(gpio_023, "nand_data3", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg27)
IOMUX_PIN(gpio_024, "nand_data4", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg28)
IOMUX_PIN(gpio_025, "nand_data5", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg29)
IOMUX_PIN(gpio_026, "nand_data6", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg30)
IOMUX_PIN(gpio_027, "nand_data7", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg31)
IOMUX_PIN(gpio_028, "nand_data8", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg32)
IOMUX_PIN(gpio_029, "nand_data9", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg33)
IOMUX_PIN(gpio_030, "nand_data10", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg34)
IOMUX_PIN(gpio_031, "nand_data11", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg35)
IOMUX_PIN(gpio_032, "nand_data12", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg36)
IOMUX_PIN(gpio_033, "nand_data13", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg37)
IOMUX_PIN(gpio_034, "nand_data14", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg38)
IOMUX_PIN(gpio_035, "nand_data15", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg39)
IOMUX_PIN(gpio_036, "emmc_cmd", FUNC0, PULLDOWN, LEVEL2, &iomg12, &iocg40)
IOMUX_PIN(gpio_037, "emmc_clk", FUNC0, PULLDOWN, LEVEL2, &iomg12, &iocg41)
IOMUX_PIN(gpio_038, "hdmi_scl", FUNC1, PULLDOWN, LEVEL2, &iomg13, &iocg54)
IOMUX_PIN(gpio_039, "hdmi_sda", FUNC1, PULLDOWN, LEVEL2, &iomg13, &iocg55)
IOMUX_PIN(gpio_040, "hdmi_cec", FUNC1, PULLDOWN, LEVEL2, &iomg14, &iocg56)
IOMUX_PIN(gpio_041, "hdmi_hpd", FUNC1, PULLDOWN, LEVEL2, &iomg15, &iocg57)
IOMUX_PIN(gpio_042, "cam_data0", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg58)
IOMUX_PIN(gpio_043, "cam_data1", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg59)
IOMUX_PIN(gpio_044, "cam_data2", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg60)
IOMUX_PIN(gpio_045, "cam_data3", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg61)
IOMUX_PIN(gpio_046, "cam_data4", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg62)
IOMUX_PIN(gpio_047, "cam_data5", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg63)
IOMUX_PIN(gpio_048, "cam_data6", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg64)
IOMUX_PIN(gpio_049, "cam_data7", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg65)
IOMUX_PIN(gpio_050, "cam_data8", FUNC1, PULLDOWN, LEVEL2, &iomg17, &iocg66)
IOMUX_PIN(gpio_051, "cam_data9", FUNC1, PULLDOWN, LEVEL2, &iomg17, &iocg67)
IOMUX_PIN(gpio_052, "cam_hysnc", FUNC1, PULLDOWN, LEVEL2, &iomg18, &iocg68)
IOMUX_PIN(gpio_053, "cam_pclk", FUNC1, PULLDOWN, LEVEL2, &iomg18, &iocg69)
IOMUX_PIN(gpio_054, "cam_vsync", FUNC1, PULLDOWN, LEVEL2, &iomg18, &iocg70)
IOMUX_PIN(gpio_055, "isp_scl0", FUNC1, PULLDOWN, LEVEL2, &iomg19, &iocg71)
IOMUX_PIN(gpio_056, "isp_sda0", FUNC1, PULLDOWN, LEVEL2, &iomg19, &iocg72)
IOMUX_PIN(gpio_057, "isp_scl1", FUNC1, PULLDOWN, LEVEL2, &iomg20, &iocg73)
IOMUX_PIN(gpio_058, "isp_sda1", FUNC1, PULLDOWN, LEVEL2, &iomg21, &iocg74)
IOMUX_PIN(gpio_059, "isp_resetb0", FUNC1, PULLDOWN, LEVEL2, &iomg22, &iocg75)
IOMUX_PIN(gpio_060, "isp_resetb1", FUNC1, PULLDOWN, LEVEL2, &iomg23, &iocg76)
IOMUX_PIN(gpio_061, "isp_fsin0_i", FUNC1, PULLDOWN, LEVEL2,  &iomg24, &iocg77)
IOMUX_PIN(gpio_063, "isp_fsin0_o", FUNC1, PULLDOWN, LEVEL2,  &iomg26, &iocg79)
IOMUX_PIN(gpio_062, "isp_fsin1_i", FUNC1, PULLDOWN, LEVEL2,  &iomg25, &iocg78)
IOMUX_PIN(gpio_064, "isp_fsn1_o", FUNC1, PULLDOWN, LEVEL2,  &iomg27, &iocg80)
IOMUX_PIN(gpio_065, "isp_strobe0", FUNC1, PULLDOWN, LEVEL2,  &iomg28, &iocg81)
IOMUX_PIN(gpio_066, "isp_strobe1", FUNC1, PULLDOWN, LEVEL2,  &iomg29, &iocg82)
IOMUX_PIN(gpio_067, "isp_cclk0", FUNC1, PULLDOWN, LEVEL2, &iomg30, &iocg83)
IOMUX_PIN(gpio_068, "isp_cclk2", FUNC1, PULLDOWN, LEVEL2, &iomg31, &iocg84)
IOMUX_PIN(gpio_069, "isp_gpio0", FUNC1, PULLDOWN, LEVEL2, &iomg32, &iocg85)
IOMUX_PIN(gpio_070, "isp_gpio1", FUNC1, PULLDOWN, LEVEL2, &iomg33, &iocg86)
IOMUX_PIN(gpio_071, "isp_gpio2", FUNC1, PULLDOWN, LEVEL2, &iomg34, &iocg87)
IOMUX_PIN(gpio_072, "isp_gpio3", FUNC1, PULLDOWN, LEVEL2, &iomg35, &iocg88)
IOMUX_PIN(gpio_073, "isp_gpio4", FUNC1, PULLDOWN, LEVEL2, &iomg36, &iocg89)
IOMUX_PIN(gpio_074, "isp_gpio5", FUNC1, PULLDOWN, LEVEL2, &iomg37, &iocg90)
IOMUX_PIN(gpio_075, "isp_gpio6", FUNC1, PULLDOWN, LEVEL2, &iomg38, &iocg91)
IOMUX_PIN(gpio_076, "isp_gpio7", FUNC3, PULLDOWN, LEVEL2, &iomg39, &iocg92)
IOMUX_PIN(gpio_077, "isp_gpio8", FUNC1, PULLDOWN, RESERVE, &iomg40, &iocg93)
IOMUX_PIN(gpio_078, "isp_gpio9", FUNC1, PULLDOWN, LEVEL2, &iomg41, &iocg94)
IOMUX_PIN(gpio_079, "pcm_di", FUNC1, PULLDOWN, LEVEL2, &iomg43, &iocg98)
IOMUX_PIN(gpio_080, "pcm_do", FUNC1, PULLDOWN, LEVEL2, &iomg43, &iocg99)
IOMUX_PIN(gpio_081, "pcm_xclk", FUNC1, PULLDOWN, LEVEL2, &iomg43, &iocg100)
IOMUX_PIN(gpio_082, "usb1_drv_vbus", FUNC1, PULLDOWN, LEVEL2, &iomg43, &iocg101)
IOMUX_PIN(gpio_083, "spdif", FUNC1, PULLDOWN, LEVEL2, &iomg44, &iocg102)
IOMUX_PIN(gpio_084, "i2c0_scl", FUNC1, PULLDOWN, LEVEL2, &iomg45, &iocg103)
IOMUX_PIN(gpio_085, "i2c0_sda", FUNC1, PULLDOWN, LEVEL2, &iomg45, &iocg104)
IOMUX_PIN(gpio_086, "i2c1_scl", FUNC1, PULLDOWN, LEVEL2, &iomg46, &iocg105)
IOMUX_PIN(gpio_087, "i2c1_sda", FUNC1, PULLDOWN, LEVEL2, &iomg46, &iocg106)
IOMUX_PIN(gpio_088, "sd_clk", FUNC1, PULLDOWN, LEVEL2, &iomg47, &iocg107)
IOMUX_PIN(gpio_089, "sd_cmd", FUNC1, PULLDOWN, LEVEL2, &iomg47, &iocg108)
IOMUX_PIN(gpio_090, "sd_data0", FUNC1, PULLDOWN, LEVEL2, &iomg47, &iocg109)
IOMUX_PIN(gpio_091, "sd_data1", FUNC1, PULLDOWN, LEVEL2, &iomg47, &iocg110)
IOMUX_PIN(gpio_092, "sd_data2", FUNC1, PULLDOWN, LEVEL2, &iomg47, &iocg111)
IOMUX_PIN(gpio_093, "sd_data3", FUNC1, PULLDOWN, LEVEL2, &iomg48, &iocg112)
IOMUX_PIN(gpio_094, "sdio0_clk", FUNC1, PULLDOWN, LEVEL2, &iomg49, &iocg113)
IOMUX_PIN(gpio_095, "sdio0_cmd", FUNC1, PULLDOWN, LEVEL2, &iomg49, &iocg114)
IOMUX_PIN(gpio_096, "sdio0_data0", FUNC1, PULLDOWN, LEVEL2, &iomg49, &iocg115)
IOMUX_PIN(gpio_097, "sdio0_data1", FUNC1, PULLDOWN, LEVEL2, &iomg50, &iocg116)
IOMUX_PIN(gpio_098, "sdio0_data2", FUNC1, PULLDOWN, LEVEL2, &iomg49, &iocg117)
IOMUX_PIN(gpio_099, "sdio0_data3", FUNC1, PULLDOWN, LEVEL2, &iomg49, &iocg118)
IOMUX_PIN(gpio_100, "sdio1_clk", FUNC1, PULLDOWN, LEVEL2, &iomg51, &iocg119)
IOMUX_PIN(gpio_101, "sdio1_cmd", FUNC1, PULLDOWN, LEVEL2, &iomg51, &iocg120)
IOMUX_PIN(gpio_102, "sdio1_data0", FUNC1, PULLDOWN, LEVEL2, &iomg51, &iocg121)
IOMUX_PIN(gpio_103, "sdio1_data1", FUNC1, PULLDOWN, LEVEL2, &iomg52, &iocg122)
IOMUX_PIN(gpio_104, "sdio1_data2", FUNC1, PULLDOWN, LEVEL2, &iomg51, &iocg123)
IOMUX_PIN(gpio_105, "sdio1_data3", FUNC1, PULLDOWN, LEVEL2, &iomg51, &iocg124)
IOMUX_PIN(gpio_106, "spi0_clk", FUNC1, PULLDOWN, LEVEL2, &iomg53, &iocg125)
IOMUX_PIN(gpio_107, "spi0_di", FUNC1, PULLDOWN, LEVEL2, &iomg53, &iocg126)
IOMUX_PIN(gpio_108, "spi0_do", FUNC1, PULLDOWN, LEVEL2, &iomg53, &iocg127)
IOMUX_PIN(gpio_109, "spi0_cs0_n", FUNC1, PULLDOWN, LEVEL2, &iomg54, &iocg128)
IOMUX_PIN(gpio_110, "spi0_cs1_n", FUNC1, PULLDOWN, LEVEL2, &iomg55, &iocg129)
IOMUX_PIN(gpio_111, "spi0_cs2_n", FUNC1, PULLDOWN, LEVEL2, &iomg56, &iocg130)
IOMUX_PIN(gpio_112, "spi0_cs3_n", FUNC1, PULLDOWN, LEVEL2, &iomg57, &iocg131)
IOMUX_PIN(gpio_113, "spi1_clk", FUNC1, PULLDOWN, LEVEL2, &iomg58, &iocg132)
IOMUX_PIN(gpio_114, "spi1_di", FUNC1, PULLDOWN, LEVEL2, &iomg58, &iocg133)
IOMUX_PIN(gpio_115, "spi1_do", FUNC1, PULLDOWN, LEVEL2, &iomg58, &iocg134)
IOMUX_PIN(gpio_116, "spi1_cs_n", FUNC1, PULLDOWN, LEVEL2, &iomg95, &iocg135)
IOMUX_PIN(gpio_117, "uart0_cts_n", FUNC1, PULLDOWN, LEVEL2, &iomg59, &iocg136)
IOMUX_PIN(gpio_118, "uart0_rts_n", FUNC1, PULLDOWN, LEVEL2, &iomg59, &iocg137)
IOMUX_PIN(gpio_119, "uart0_rxd", FUNC1, PULLDOWN, LEVEL2, &iomg60, &iocg138)
IOMUX_PIN(gpio_120, "uart0_txd", FUNC1, PULLDOWN, LEVEL2, &iomg60, &iocg139)
IOMUX_PIN(gpio_121, "uart1_cts_n", FUNC1, PULLDOWN, LEVEL2, &iomg61, &iocg140)
IOMUX_PIN(gpio_122, "uart1_rts_n", FUNC1, PULLDOWN, LEVEL2, &iomg61, &iocg141)
IOMUX_PIN(gpio_123, "uart1_rxd", FUNC1, PULLDOWN, LEVEL2, &iomg62, &iocg142)
IOMUX_PIN(gpio_124, "uart1_txd", FUNC1, PULLDOWN, LEVEL2, &iomg62, &iocg143)
IOMUX_PIN(gpio_125, "usim_clk", FUNC1, PULLDOWN, LEVEL2, &iomg63, &iocg144)
IOMUX_PIN(gpio_126, "usim_data", FUNC1, PULLDOWN, LEVEL2, &iomg63, &iocg145)
IOMUX_PIN(gpio_127, "usim_rst", FUNC1, PULLDOWN, LEVEL2, &iomg96, &iocg146)
IOMUX_PIN(gpio_128, "onewire", FUNC1, PULLDOWN, LEVEL2, &iomg64, &iocg147)
IOMUX_PIN(gpio_129, "keypad_out0", FUNC1, PULLDOWN, LEVEL2, &iomg65, &iocg148)
IOMUX_PIN(gpio_130, "keypad_out1", FUNC1, PULLDOWN, LEVEL2, &iomg66, &iocg149)
IOMUX_PIN(gpio_131, "keypad_out2", FUNC1, PULLDOWN, LEVEL2, &iomg67, &iocg150)
IOMUX_PIN(gpio_132, "keypad_out3", FUNC1, PULLDOWN, LEVEL2, &iomg68, &iocg151)
IOMUX_PIN(gpio_133, "keypad_out4", FUNC1, PULLDOWN, LEVEL2, &iomg69, &iocg152)
IOMUX_PIN(gpio_134, "keypad_out5", FUNC0, PULLDOWN, LEVEL2, &iomg70, &iocg153)
IOMUX_PIN(gpio_135, "keypad_out6", FUNC1, PULLDOWN, LEVEL2, &iomg71, &iocg154)
IOMUX_PIN(gpio_136, "keypad_out7", FUNC0, NOPULL, RESERVE, &iomg72, &iocg155)
IOMUX_PIN(gpio_137, "keypad_in0", FUNC1, PULLDOWN, LEVEL2, &iomg73, &iocg156)
IOMUX_PIN(gpio_138, "keypad_in1", FUNC1, PULLDOWN, LEVEL2, &iomg74, &iocg157)
IOMUX_PIN(gpio_139, "keypad_in2", FUNC1, PULLDOWN, LEVEL2, &iomg75, &iocg158)
IOMUX_PIN(gpio_140, "keypad_in3", FUNC1, PULLDOWN, LEVEL2, &iomg76, &iocg159)
IOMUX_PIN(gpio_141, "keypad_in4", FUNC1, PULLUP, LEVEL2, &iomg77, &iocg160)
IOMUX_PIN(gpio_142, "keypad_in5", FUNC1, PULLUP, LEVEL2, &iomg78, &iocg161)
IOMUX_PIN(gpio_143, "keypad_in6", FUNC1, PULLUP, LEVEL2, &iomg79, &iocg162)
IOMUX_PIN(gpio_144, "keypad_in7", FUNC1, PULLUP, LEVEL2, &iomg80, &iocg163)
IOMUX_PIN(gpio_145, "tbc_left", FUNC1, PULLDOWN, LEVEL2, &iomg81, &iocg164)
IOMUX_PIN(gpio_146, "tbc_right", FUNC1, PULLDOWN, LEVEL2, &iomg81, &iocg165)
IOMUX_PIN(gpio_147, "tbc_up", FUNC1, PULLDOWN, LEVEL2, &iomg97, &iocg166)
IOMUX_PIN(gpio_148, "tbc_down", FUNC1, PULLDOWN, LEVEL2, &iomg97, &iocg167)
IOMUX_PIN(gpio_149, "pwm_out0", FUNC1, PULLDOWN, LEVEL2, &iomg82, &iocg168)
IOMUX_PIN(gpio_150, "pwm_out1", FUNC1, PULLDOWN, LEVEL2, &iomg83, &iocg169)
IOMUX_PIN(gpio_151, "gps_sign", FUNC1, PULLDOWN, LEVEL2, &iomg84, &iocg170)
IOMUX_PIN(gpio_152, "gps_acqclk", FUNC1, PULLDOWN, LEVEL2, &iomg84, &iocg171)
IOMUX_PIN(gpio_153, "gps_pd", FUNC1, PULLDOWN, LEVEL2, &iomg84, &iocg172)
IOMUX_PIN(gpio_154, "gps_spi_clk", FUNC1, PULLDOWN, LEVEL2, &iomg85, &iocg173)
IOMUX_PIN(gpio_155, "gps_spi_di", FUNC1, PULLDOWN, LEVEL2, &iomg85, &iocg174)
IOMUX_PIN(gpio_156, "gpio_156", RESERVE, PULLDOWN, LEVEL2, NULL, &iocg0)
IOMUX_PIN(gpio_157, "gpio_157", RESERVE, PULLDOWN, LEVEL2, NULL, &iocg1)
IOMUX_PIN(gpio_158, "gpio_158", RESERVE, PULLUP, RESERVE, NULL, &iocg2)
IOMUX_PIN(gpio_159, "gpio_159", RESERVE, PULLUP, RESERVE, NULL, &iocg3)
IOMUX_PIN(gpio_160, "gps_spi_do", FUNC1, PULLDOWN, LEVEL2, &iomg86, &iocg175)
IOMUX_PIN(gpio_161, "ps_spi_en_n", FUNC1, PULLDOWN, LEVEL2, &iomg86, &iocg176)
IOMUX_PIN(gpio_162, "bt_bdata1", FUNC1, PULLDOWN, LEVEL2, &iomg87, &iocg177)
IOMUX_PIN(gpio_164, "bt_bpktctl", FUNC1, PULLDOWN, LEVEL2, &iomg88, &iocg179)
IOMUX_PIN(gpio_163, "bt_brclk", FUNC1, PULLDOWN, LEVEL2, &iomg87, &iocg178)
IOMUX_PIN(gpio_165, "rf_rst_n", FUNC1, PULLDOWN, LEVEL2, &iomg93, &iocg180)
IOMUX_PIN(gpio_166, "bt_spi_clk", FUNC1, PULLDOWN, LEVEL2, &iomg89, &iocg181)
IOMUX_PIN(gpio_167, "bt_spi_csn", FUNC1, PULLDOWN, LEVEL2, &iomg89, &iocg182)
IOMUX_PIN(gpio_168, "bt_spi_data", FUNC1, PULLDOWN, LEVEL2, &iomg89, &iocg183)
IOMUX_PIN(gpio_169, "bt_enable_rm", FUNC1, PULLDOWN, LEVEL2, &iomg89, &iocg184)
IOMUX_PIN(gpio_170, "bt_26M_in", FUNC1, PULLDOWN, LEVEL2, &iomg89, &iocg185)
IOMUX_PIN(gpio_171, "rftcxo_pwr", FUNC1, PULLDOWN, LEVEL2, &iomg90, &iocg186)
IOMUX_PIN(gpio_172, "coex_btpriority", FUNC1, PULLDOWN, LEVEL2, &iomg91, &iocg187)
IOMUX_PIN(gpio_173, "coex_btactive", FUNC1, PULLDOWN, LEVEL2, &iomg91, &iocg188)
IOMUX_PIN(gpio_174, "coex_wlactive", FUNC1, PULLDOWN, LEVEL2, &iomg91, &iocg189)
IOMUX_PIN(gpio_175, "clk_out0", FUNC0, PULLDOWN, LEVEL2, &iomg92, &iocg190)
/*--------------------------------CS--------------------------------------*/
IOMUX_PIN(gpio_002_cs, "efuse_sel", FUNC0, PULLDOWN, LEVEL2, &iomg0, &iocg6)
IOMUX_PIN(gpio_003_cs, "efuse_csb", FUNC0, PULLDOWN, LEVEL2, &iomg0, &iocg7)
IOMUX_PIN(gpio_004_cs, "efuse_sclk", FUNC0, PULLDOWN, LEVEL2, &iomg0, &iocg8)
IOMUX_PIN(gpio_005_cs, "efuse_pgm", FUNC0, PULLDOWN, LEVEL2, &iomg0, &iocg9)
IOMUX_PIN(gpio_006_cs, "efuse_din", FUNC0, PULLDOWN, LEVEL2, &iomg1, &iocg10)
IOMUX_PIN(gpio_007_cs, "efuse_dout", FUNC0, PULLDOWN, RESERVE, &iomg2, &iocg11)
IOMUX_PIN(gpio_008_cs, "nand_ale", FUNC0, NOPULL, LEVEL2, &iomg3, &iocg12)
IOMUX_PIN(gpio_009_cs, "nand_cle", FUNC0, NOPULL, LEVEL2, &iomg3, &iocg13)
IOMUX_PIN(gpio_010_cs, "nand_re_n", FUNC0, NOPULL, LEVEL2, &iomg3, &iocg14)
IOMUX_PIN(gpio_011_cs, "nand_we_n", FUNC0, NOPULL, LEVEL2, &iomg3, &iocg15)
IOMUX_PIN(gpio_012_cs, "nand_cs0_n", FUNC0, NOPULL, LEVEL2, &iomg3, &iocg16)
IOMUX_PIN(gpio_013_cs, "nand_cs1_n", FUNC0, NOPULL, LEVEL2, &iomg4, &iocg17)
IOMUX_PIN(gpio_014_cs, "nand_cs2_n", FUNC0, NOPULL, LEVEL2, &iomg5, &iocg18)
IOMUX_PIN(gpio_015_cs, "nand_cs3_n", FUNC0, NOPULL, LEVEL2, &iomg6, &iocg19)
IOMUX_PIN(gpio_016_cs, "nand_busy0_n", FUNC0, PULLUP, LEVEL2, &iomg94, &iocg20)
IOMUX_PIN(gpio_017_cs, "nand_busy1_n", FUNC0, PULLUP, LEVEL2, &iomg7, &iocg21)
IOMUX_PIN(gpio_018_cs, "nand_busy2_n", FUNC0, PULLUP, LEVEL2, &iomg8, &iocg22)
IOMUX_PIN(gpio_019_cs, "nand_busy3_n", FUNC0, PULLUP, LEVEL2, &iomg9, &iocg23)
IOMUX_PIN(gpio_020_cs, "nand_data0", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg24)
IOMUX_PIN(gpio_021_cs, "nand_data1", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg25)
IOMUX_PIN(gpio_022_cs, "nand_data2", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg26)
IOMUX_PIN(gpio_023_cs, "nand_data3", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg27)
IOMUX_PIN(gpio_024_cs, "nand_data4", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg28)
IOMUX_PIN(gpio_025_cs, "nand_data5", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg29)
IOMUX_PIN(gpio_026_cs, "nand_data6", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg30)
IOMUX_PIN(gpio_027_cs, "nand_data7", FUNC0, PULLDOWN, LEVEL2, &iomg3, &iocg31)
IOMUX_PIN(gpio_028_cs, "nand_data8", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg32)
IOMUX_PIN(gpio_029_cs, "nand_data9", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg33)
IOMUX_PIN(gpio_030_cs, "nand_data10", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg34)
IOMUX_PIN(gpio_031_cs, "nand_data11", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg35)
IOMUX_PIN(gpio_032_cs, "nand_data12", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg36)
IOMUX_PIN(gpio_033_cs, "nand_data13", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg37)
IOMUX_PIN(gpio_034_cs, "nand_data14", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg38)
IOMUX_PIN(gpio_035_cs, "nand_data15", FUNC0, PULLDOWN, LEVEL2, &iomg10, &iocg39)
IOMUX_PIN(gpio_036_cs, "emmc_cmd", FUNC0, PULLDOWN, LEVEL2, &iomg12, &iocg40)
IOMUX_PIN(gpio_037_cs, "emmc_clk", FUNC0, PULLDOWN, LEVEL2, &iomg12, &iocg41)
IOMUX_PIN(gpio_038_cs, "hdmi_scl", FUNC1, PULLDOWN, LEVEL2, &iomg13, &iocg54)
IOMUX_PIN(gpio_039_cs, "hdmi_sda", FUNC1, PULLDOWN, LEVEL2, &iomg13, &iocg55)
IOMUX_PIN(gpio_040_cs, "hdmi_cec", FUNC1, PULLDOWN, LEVEL2, &iomg14, &iocg56)
IOMUX_PIN(gpio_041_cs, "hdmi_hpd", FUNC1, PULLDOWN, LEVEL2, &iomg15, &iocg57)
IOMUX_PIN(gpio_042_cs, "cam_data0", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg58)
IOMUX_PIN(gpio_043_cs, "cam_data1", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg59)
IOMUX_PIN(gpio_044_cs, "cam_data2", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg60)
IOMUX_PIN(gpio_045_cs, "cam_data3", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg61)
IOMUX_PIN(gpio_046_cs, "cam_data4", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg62)
IOMUX_PIN(gpio_047_cs, "cam_data5", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg63)
IOMUX_PIN(gpio_048_cs, "cam_data6", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg64)
IOMUX_PIN(gpio_049_cs, "cam_data7", FUNC1, PULLDOWN, LEVEL2, &iomg16, &iocg65)
IOMUX_PIN(gpio_050_cs, "cam_data8", FUNC1, PULLDOWN, LEVEL2, &iomg17, &iocg66)
IOMUX_PIN(gpio_051_cs, "cam_data9", FUNC1, PULLDOWN, LEVEL2, &iomg17, &iocg67)
IOMUX_PIN(gpio_052_cs, "cam_hysnc", FUNC1, PULLDOWN, LEVEL2, &iomg18, &iocg68)
IOMUX_PIN(gpio_053_cs, "cam_pclk", FUNC1, PULLDOWN, LEVEL2, &iomg18, &iocg69)
IOMUX_PIN(gpio_054_cs, "cam_vsync", FUNC1, PULLDOWN, LEVEL2, &iomg18, &iocg70)
IOMUX_PIN(gpio_055_cs, "isp_scl0", FUNC1, NOPULL, LEVEL2, &iomg19, &iocg71)
IOMUX_PIN(gpio_056_cs, "isp_sda0", FUNC1, NOPULL, LEVEL2, &iomg19, &iocg72)
IOMUX_PIN(gpio_057_cs, "isp_scl1", FUNC1, PULLDOWN, LEVEL2, &iomg20, &iocg73)
IOMUX_PIN(gpio_058_cs, "isp_sda1", FUNC1, PULLDOWN, LEVEL2, &iomg21, &iocg74)
IOMUX_PIN(gpio_059_cs, "isp_resetb0", FUNC1, PULLDOWN, LEVEL2, &iomg22, &iocg75)
IOMUX_PIN(gpio_060_cs, "isp_resetb1", FUNC1, PULLDOWN, LEVEL2, &iomg23, &iocg76)
IOMUX_PIN(gpio_061_cs, "isp_fsin0", FUNC1, PULLDOWN, LEVEL2,  &iomg24, &iocg77)
IOMUX_PIN(gpio_062_cs, "isp_fsin1", FUNC1, PULLDOWN, LEVEL2,  &iomg25, &iocg78)
IOMUX_PIN(gpio_063_cs, "i2c2_scl", FUNC1, PULLDOWN, LEVEL2,  &iomg26, &iocg79)
IOMUX_PIN(gpio_064_cs, "i2c2_sda", FUNC1, PULLDOWN, LEVEL2,  &iomg27, &iocg80)
IOMUX_PIN(gpio_065_cs, "isp_strobe0", FUNC1, PULLDOWN, LEVEL2,  &iomg28, &iocg81)
IOMUX_PIN(gpio_066_cs, "isp_strobe1", FUNC1, PULLDOWN, LEVEL2,  &iomg29, &iocg82)
IOMUX_PIN(gpio_067_cs, "isp_cclk0", FUNC1, PULLDOWN, LEVEL2, &iomg30, &iocg83)
IOMUX_PIN(gpio_068_cs, "isp_cclk2", FUNC1, PULLDOWN, LEVEL2, &iomg31, &iocg84)
IOMUX_PIN(gpio_069_cs, "isp_gpio0", FUNC1, PULLDOWN, LEVEL2, &iomg32, &iocg85)
IOMUX_PIN(gpio_070_cs, "isp_gpio1", FUNC1, PULLDOWN, LEVEL2, &iomg33, &iocg86)
IOMUX_PIN(gpio_071_cs, "isp_gpio2", FUNC1, PULLDOWN, LEVEL2, &iomg34, &iocg87)
IOMUX_PIN(gpio_072_cs, "isp_gpio3", FUNC1, PULLDOWN, LEVEL2, &iomg35, &iocg88)
IOMUX_PIN(gpio_073_cs, "isp_gpio4", FUNC1, PULLDOWN, LEVEL2, &iomg36, &iocg89)
IOMUX_PIN(gpio_074_cs, "isp_gpio5", FUNC1, PULLDOWN, LEVEL2, &iomg37, &iocg90)
IOMUX_PIN(gpio_075_cs, "isp_gpio6", FUNC1, PULLDOWN, LEVEL2, &iomg38, &iocg91)
IOMUX_PIN(gpio_076_cs, "isp_gpio7", FUNC1, PULLDOWN, LEVEL2, &iomg39, &iocg92)
IOMUX_PIN(gpio_077_cs, "isp_gpio8", FUNC1, PULLDOWN, RESERVE, &iomg40, &iocg93)
IOMUX_PIN(gpio_078_cs, "isp_gpio9", FUNC1, PULLDOWN, LEVEL2, &iomg41, &iocg94)
IOMUX_PIN(gpio_079_cs, "pcm_di", FUNC0, PULLDOWN, LEVEL2, NULL, &iocg98)
IOMUX_PIN(gpio_080_cs, "pcm_do", FUNC0, PULLDOWN, LEVEL2, NULL, &iocg99)
IOMUX_PIN(gpio_081_cs, "pcm_xclk", FUNC0, PULLDOWN, LEVEL2, NULL, &iocg100)
IOMUX_PIN(gpio_082_cs, "usb1_drv_vbus", FUNC0, PULLDOWN, LEVEL2, &iomg43, &iocg101)
IOMUX_PIN(gpio_083_cs, "spdif", FUNC0, PULLDOWN, LEVEL2, &iomg44, &iocg102)
IOMUX_PIN(gpio_084_cs, "i2c0_scl", FUNC1, PULLDOWN, LEVEL2, &iomg45, &iocg103)
IOMUX_PIN(gpio_085_cs, "i2c0_sda", FUNC1, PULLDOWN, LEVEL2, &iomg45, &iocg104)
IOMUX_PIN(gpio_086_cs, "i2c1_scl", FUNC1, PULLDOWN, LEVEL2, &iomg46, &iocg105)
IOMUX_PIN(gpio_087_cs, "i2c1_sda", FUNC1, PULLDOWN, LEVEL2, &iomg46, &iocg106)
IOMUX_PIN(gpio_088_cs, "sd_clk", FUNC1, PULLDOWN, LEVEL2, &iomg47, &iocg107)
IOMUX_PIN(gpio_089_cs, "sd_cmd", FUNC1, PULLDOWN, LEVEL2, &iomg47, &iocg108)
IOMUX_PIN(gpio_090_cs, "sd_data0", FUNC1, PULLDOWN, LEVEL2, &iomg47, &iocg109)
IOMUX_PIN(gpio_091_cs, "sd_data1", FUNC1, PULLDOWN, LEVEL2, &iomg47, &iocg110)
IOMUX_PIN(gpio_092_cs, "sd_data2", FUNC1, PULLDOWN, LEVEL2, &iomg47, &iocg111)
IOMUX_PIN(gpio_093_cs, "sd_data3", FUNC1, PULLDOWN, LEVEL2, &iomg48, &iocg112)
IOMUX_PIN(gpio_094_cs, "sdio0_clk", FUNC1, PULLDOWN, LEVEL2, &iomg49, &iocg113)
IOMUX_PIN(gpio_095_cs, "sdio0_cmd", FUNC1, PULLDOWN, LEVEL2, &iomg49, &iocg114)
IOMUX_PIN(gpio_096_cs, "sdio0_data0", FUNC1, PULLDOWN, LEVEL2, &iomg49, &iocg115)
IOMUX_PIN(gpio_097_cs, "sdio0_data1", FUNC1, PULLDOWN, LEVEL2, &iomg50, &iocg116)
IOMUX_PIN(gpio_098_cs, "sdio0_data2", FUNC1, PULLDOWN, LEVEL2, &iomg49, &iocg117)
IOMUX_PIN(gpio_099_cs, "sdio0_data3", FUNC1, PULLDOWN, LEVEL2, &iomg49, &iocg118)
IOMUX_PIN(gpio_100_cs, "sdio1_clk", FUNC1, PULLDOWN, LEVEL2, &iomg51, &iocg119)
IOMUX_PIN(gpio_101_cs, "sdio1_cmd", FUNC1, PULLDOWN, LEVEL2, &iomg51, &iocg120)
IOMUX_PIN(gpio_102_cs, "sdio1_data0", FUNC1, PULLDOWN, LEVEL2, &iomg51, &iocg121)
IOMUX_PIN(gpio_103_cs, "sdio1_data1", FUNC1, PULLDOWN, LEVEL2, &iomg52, &iocg122)
IOMUX_PIN(gpio_104_cs, "sdio1_data2", FUNC1, PULLDOWN, LEVEL2, &iomg51, &iocg123)
IOMUX_PIN(gpio_105_cs, "sdio1_data3", FUNC1, PULLDOWN, LEVEL2, &iomg51, &iocg124)
IOMUX_PIN(gpio_106_cs, "spi0_clk", FUNC1, PULLDOWN, LEVEL2, &iomg53, &iocg125)
IOMUX_PIN(gpio_107_cs, "spi0_di", FUNC1, PULLDOWN, LEVEL2, &iomg53, &iocg126)
IOMUX_PIN(gpio_108_cs, "spi0_do", FUNC1, PULLDOWN, LEVEL2, &iomg53, &iocg127)
IOMUX_PIN(gpio_109_cs, "spi0_cs0_n", FUNC1, PULLDOWN, LEVEL2, &iomg54, &iocg128)
IOMUX_PIN(gpio_110_cs, "spi0_cs1_n", FUNC1, PULLDOWN, LEVEL2, &iomg55, &iocg129)
IOMUX_PIN(gpio_111_cs, "spi0_cs2_n", FUNC1, PULLDOWN, LEVEL2, &iomg56, &iocg130)
IOMUX_PIN(gpio_112_cs, "spi0_cs3_n", FUNC1, PULLDOWN, LEVEL2, &iomg57, &iocg131)
IOMUX_PIN(gpio_113_cs, "spi1_clk", FUNC1, PULLDOWN, LEVEL2, &iomg98, &iocg132)
IOMUX_PIN(gpio_114_cs, "spi1_di", FUNC1, PULLDOWN, LEVEL2, &iomg98, &iocg133)
IOMUX_PIN(gpio_115_cs, "spi1_do", FUNC1, PULLDOWN, LEVEL2, &iomg58, &iocg134)
IOMUX_PIN(gpio_116_cs, "spi1_cs_n", FUNC1, PULLDOWN, LEVEL2, &iomg95, &iocg135)
IOMUX_PIN(gpio_117_cs, "uart0_cts_n", FUNC1, PULLDOWN, LEVEL2, &iomg59, &iocg136)
IOMUX_PIN(gpio_118_cs, "uart0_rts_n", FUNC1, PULLDOWN, LEVEL2, &iomg59, &iocg137)
IOMUX_PIN(gpio_119_cs, "uart0_rxd", FUNC1, PULLDOWN, LEVEL2, &iomg60, &iocg138)
IOMUX_PIN(gpio_120_cs, "uart0_txd", FUNC1, PULLDOWN, LEVEL2, &iomg60, &iocg139)
IOMUX_PIN(gpio_121_cs, "uart1_cts_n", FUNC1, PULLDOWN, LEVEL2, &iomg61, &iocg140)
IOMUX_PIN(gpio_122_cs, "uart1_rts_n", FUNC1, PULLDOWN, LEVEL2, &iomg61, &iocg141)
IOMUX_PIN(gpio_123_cs, "uart1_rxd", FUNC1, PULLDOWN, LEVEL2, &iomg62, &iocg142)
IOMUX_PIN(gpio_124_cs, "uart1_txd", FUNC1, PULLDOWN, LEVEL2, &iomg62, &iocg143)
IOMUX_PIN(gpio_125_cs, "usim_clk", FUNC1, PULLDOWN, LEVEL2, &iomg63, &iocg144)
IOMUX_PIN(gpio_126_cs, "usim_data", FUNC1, PULLDOWN, LEVEL2, &iomg63, &iocg145)
IOMUX_PIN(gpio_127_cs, "usim_rst", FUNC1, PULLDOWN, LEVEL2, &iomg96, &iocg146)
IOMUX_PIN(gpio_128_cs, "onewire", FUNC1, PULLDOWN, LEVEL2, &iomg64, &iocg147)
IOMUX_PIN(gpio_129_cs, "keypad_out0", FUNC1, PULLDOWN, LEVEL2, &iomg65, &iocg148)
IOMUX_PIN(gpio_130_cs, "keypad_out1", FUNC1, PULLDOWN, LEVEL2, &iomg66, &iocg149)
IOMUX_PIN(gpio_131_cs, "keypad_out2", FUNC1, PULLDOWN, LEVEL2, &iomg67, &iocg150)
IOMUX_PIN(gpio_132_cs, "keypad_out3", FUNC1, PULLDOWN, LEVEL2, &iomg68, &iocg151)
IOMUX_PIN(gpio_133_cs, "keypad_out4", FUNC1, PULLDOWN, LEVEL2, &iomg69, &iocg152)
IOMUX_PIN(gpio_134_cs, "keypad_out5", FUNC0, PULLDOWN, LEVEL2, &iomg70, &iocg153)
IOMUX_PIN(gpio_135_cs, "keypad_out6", FUNC1, PULLDOWN, LEVEL2, &iomg71, &iocg154)
IOMUX_PIN(gpio_136_cs, "keypad_out7", FUNC0, NOPULL, RESERVE, &iomg72, &iocg155)
IOMUX_PIN(gpio_137_cs, "keypad_in0", FUNC1, PULLDOWN, LEVEL2, &iomg73, &iocg156)
IOMUX_PIN(gpio_138_cs, "keypad_in1", FUNC1, PULLDOWN, LEVEL2, &iomg74, &iocg157)
IOMUX_PIN(gpio_139_cs, "keypad_in2", FUNC1, PULLDOWN, LEVEL2, &iomg75, &iocg158)
IOMUX_PIN(gpio_140_cs, "keypad_in3", FUNC1, PULLDOWN, LEVEL2, &iomg76, &iocg159)
IOMUX_PIN(gpio_141_cs, "keypad_in4", FUNC1, PULLUP, LEVEL2, &iomg77, &iocg160)
IOMUX_PIN(gpio_142_cs, "keypad_in5", FUNC1, PULLUP, LEVEL2, &iomg78, &iocg161)
IOMUX_PIN(gpio_143_cs, "keypad_in6", FUNC1, PULLUP, LEVEL2, &iomg79, &iocg162)
IOMUX_PIN(gpio_144_cs, "keypad_in7", FUNC1, PULLUP, LEVEL2, &iomg80, &iocg163)
IOMUX_PIN(gpio_145_cs, "tbc_left", FUNC1, PULLDOWN, LEVEL2, &iomg81, &iocg164)
IOMUX_PIN(gpio_146_cs, "tbc_right", FUNC1, PULLDOWN, LEVEL2, &iomg81, &iocg165)
IOMUX_PIN(gpio_147_cs, "tbc_up", FUNC1, PULLDOWN, LEVEL2, &iomg97, &iocg166)
IOMUX_PIN(gpio_148_cs, "tbc_down", FUNC1, PULLDOWN, LEVEL2, &iomg97, &iocg167)
IOMUX_PIN(gpio_149_cs, "pwm_out0", FUNC1, PULLDOWN, LEVEL2, &iomg82, &iocg168)
IOMUX_PIN(gpio_150_cs, "pwm_out1", FUNC1, PULLDOWN, LEVEL2, &iomg83, &iocg169)
IOMUX_PIN(gpio_151_cs, "gps_sign", FUNC1, PULLDOWN, LEVEL2, &iomg84, &iocg170)
IOMUX_PIN(gpio_152_cs, "gps_acqclk", FUNC1, PULLDOWN, LEVEL2, &iomg84, &iocg171)
IOMUX_PIN(gpio_153_cs, "gps_pd", FUNC1, PULLDOWN, LEVEL2, &iomg84, &iocg172)
IOMUX_PIN(gpio_154_cs, "gps_spi_clk", FUNC1, PULLDOWN, LEVEL2, &iomg85, &iocg173)
IOMUX_PIN(gpio_155_cs, "gps_spi_di", FUNC1, PULLDOWN, LEVEL2, &iomg85, &iocg174)
IOMUX_PIN(gpio_156_cs, "gpio_156", RESERVE, PULLDOWN, LEVEL2, NULL, &iocg0)
IOMUX_PIN(gpio_157_cs, "gpio_157", RESERVE, PULLDOWN, LEVEL2, NULL, &iocg1)
IOMUX_PIN(gpio_158_cs, "gpio_158", RESERVE, PULLUP, RESERVE, NULL, &iocg2)
IOMUX_PIN(gpio_159_cs, "gpio_159", RESERVE, PULLUP, RESERVE, NULL, &iocg3)
IOMUX_PIN(gpio_160_cs, "gps_spi_do", FUNC1, PULLDOWN, LEVEL2, &iomg86, &iocg175)
IOMUX_PIN(gpio_161_cs, "ps_spi_en_n", FUNC1, PULLDOWN, LEVEL2, &iomg86, &iocg176)
IOMUX_PIN(gpio_162_cs, "bt_bdata1", FUNC1, PULLDOWN, LEVEL2, &iomg87, &iocg177)
IOMUX_PIN(gpio_164_cs, "bt_bpktctl", FUNC1, PULLDOWN, LEVEL2, &iomg88, &iocg179)
IOMUX_PIN(gpio_163_cs, "bt_brclk", FUNC1, PULLDOWN, LEVEL2, &iomg87, &iocg178)
IOMUX_PIN(gpio_165_cs, "rf_rst_n", FUNC1, PULLDOWN, LEVEL2, &iomg93, &iocg180)
IOMUX_PIN(gpio_166_cs, "bt_spi_clk", RESERVE, PULLDOWN, LEVEL2, &iomg89, &iocg181)
IOMUX_PIN(gpio_167_cs, "bt_spi_csn", RESERVE, PULLDOWN, LEVEL2, &iomg89, &iocg182)
IOMUX_PIN(gpio_168_cs, "bt_spi_data", RESERVE, PULLDOWN, LEVEL2, &iomg89, &iocg183)
IOMUX_PIN(gpio_169_cs, "bt_enable_rm", RESERVE, PULLDOWN, LEVEL2, &iomg89, &iocg184)
IOMUX_PIN(gpio_170_cs, "bt_26M_in", RESERVE, PULLDOWN, LEVEL2, &iomg89, &iocg185)
IOMUX_PIN(gpio_171_cs, "rftcxo_pwr", FUNC1, PULLDOWN, LEVEL2, &iomg90, &iocg186)
IOMUX_PIN(gpio_172_cs, "coex_btpriority", RESERVE, PULLDOWN, RESERVE, &iomg91, &iocg187)
IOMUX_PIN(gpio_173_cs, "coex_btactive", RESERVE, PULLDOWN, RESERVE, &iomg91, &iocg188)
IOMUX_PIN(gpio_174_cs, "coex_wlactive", RESERVE, PULLDOWN, RESERVE, &iomg91, &iocg189)
IOMUX_PIN(gpio_175_cs, "clk_out0", FUNC0, PULLDOWN, LEVEL2, &iomg92, &iocg190)
/*cs pins are defined specially*/

/*non gpio pins*/
IOMUX_PIN(BOOT_MODE0, "boot_mode0", RESERVE, PULLUP, RESERVE, NULL, &iocg4)
IOMUX_PIN(BOOT_MODE1, "boot_mode1", RESERVE, PULLUP, RESERVE, NULL, &iocg5)
IOMUX_PIN(PMU_SPI_CLK, "pmu_spi_clk", RESERVE, RESERVE, LEVEL2, NULL, &iocg191)
IOMUX_PIN(PMU_SPI_DATA, "pmu_spi_data", RESERVE, PULLDOWN, LEVEL2, NULL, &iocg192)
IOMUX_PIN(PMU_SPI_CS_N, "pmu_spi_cs_n", RESERVE, RESERVE, LEVEL2, NULL, &iocg193)
IOMUX_PIN(I2S_DI, "i2s_di", RESERVE, PULLDOWN, LEVEL2, NULL, &iocg95)
IOMUX_PIN(I2S_DO, "i2s_do", FUNC0, RESERVE, LEVEL2, &iomg42, &iocg194)
IOMUX_PIN(I2S_XCLK, "i2s_xclk", RESERVE, PULLDOWN, LEVEL2, NULL, &iocg96)
IOMUX_PIN(I2S_XFS, "i2s_xfs", RESERVE, PULLDOWN, LEVEL2, NULL, &iocg97)

struct iomux_pin_table pins_table[] = {
	PIN_TABLE("gpio_003", &gpio_003), PIN_TABLE("gpio_004", &gpio_004), PIN_TABLE("gpio_005", &gpio_005), PIN_TABLE("gpio_006", &gpio_006),\
	PIN_TABLE("gpio_007", &gpio_007), PIN_TABLE("gpio_008", &gpio_008), PIN_TABLE("gpio_009", &gpio_009), PIN_TABLE("gpio_010", &gpio_010),\
	PIN_TABLE("gpio_011", &gpio_011), PIN_TABLE("gpio_012", &gpio_012), PIN_TABLE("gpio_013", &gpio_013), PIN_TABLE("gpio_014", &gpio_014),\
	PIN_TABLE("gpio_015", &gpio_015), PIN_TABLE("gpio_016", &gpio_016), PIN_TABLE("gpio_017", &gpio_017), PIN_TABLE("gpio_018", &gpio_018),\
	PIN_TABLE("gpio_019", &gpio_019), PIN_TABLE("gpio_020", &gpio_020), PIN_TABLE("gpio_021", &gpio_021), PIN_TABLE("gpio_022", &gpio_022),\
	PIN_TABLE("gpio_023", &gpio_023), PIN_TABLE("gpio_024", &gpio_024), PIN_TABLE("gpio_025", &gpio_025), PIN_TABLE("gpio_026", &gpio_026),\
	PIN_TABLE("gpio_027", &gpio_027), PIN_TABLE("gpio_028", &gpio_028), PIN_TABLE("gpio_029", &gpio_029), PIN_TABLE("gpio_030", &gpio_030),\
	PIN_TABLE("gpio_031", &gpio_031), PIN_TABLE("gpio_032", &gpio_032), PIN_TABLE("gpio_033", &gpio_033), PIN_TABLE("gpio_034", &gpio_034),\
	PIN_TABLE("gpio_035", &gpio_035), PIN_TABLE("gpio_036", &gpio_036), PIN_TABLE("gpio_037", &gpio_037), PIN_TABLE("gpio_038", &gpio_038),\
	PIN_TABLE("gpio_039", &gpio_039), PIN_TABLE("gpio_040", &gpio_040), PIN_TABLE("gpio_041", &gpio_041), PIN_TABLE("gpio_042", &gpio_042),\
	PIN_TABLE("gpio_043", &gpio_043), PIN_TABLE("gpio_044", &gpio_044), PIN_TABLE("gpio_045", &gpio_045), PIN_TABLE("gpio_046", &gpio_046),\
	PIN_TABLE("gpio_047", &gpio_047), PIN_TABLE("gpio_048", &gpio_048), PIN_TABLE("gpio_049", &gpio_049), PIN_TABLE("gpio_050", &gpio_050),\
	PIN_TABLE("gpio_051", &gpio_051), PIN_TABLE("gpio_052", &gpio_052), PIN_TABLE("gpio_053", &gpio_053), PIN_TABLE("gpio_054", &gpio_054),\
	PIN_TABLE("gpio_055", &gpio_055), PIN_TABLE("gpio_056", &gpio_056), PIN_TABLE("gpio_057", &gpio_057), PIN_TABLE("gpio_058", &gpio_058),\
	PIN_TABLE("gpio_059", &gpio_059), PIN_TABLE("gpio_060", &gpio_060), PIN_TABLE("gpio_061", &gpio_061), PIN_TABLE("gpio_062", &gpio_062),\
	PIN_TABLE("gpio_063", &gpio_063), PIN_TABLE("gpio_064", &gpio_064), PIN_TABLE("gpio_065", &gpio_065), PIN_TABLE("gpio_066", &gpio_066),\
	PIN_TABLE("gpio_067", &gpio_067), PIN_TABLE("gpio_068", &gpio_068), PIN_TABLE("gpio_069", &gpio_069), PIN_TABLE("gpio_070", &gpio_070),\
	PIN_TABLE("gpio_071", &gpio_071), PIN_TABLE("gpio_072", &gpio_072), PIN_TABLE("gpio_073", &gpio_073), PIN_TABLE("gpio_074", &gpio_074),\
	PIN_TABLE("gpio_075", &gpio_075), PIN_TABLE("gpio_076", &gpio_076), PIN_TABLE("gpio_077", &gpio_077), PIN_TABLE("gpio_078", &gpio_078),\
	PIN_TABLE("gpio_079", &gpio_079), PIN_TABLE("gpio_080", &gpio_080), PIN_TABLE("gpio_081", &gpio_081), PIN_TABLE("gpio_082", &gpio_082),\
	PIN_TABLE("gpio_083", &gpio_083), PIN_TABLE("gpio_084", &gpio_084), PIN_TABLE("gpio_085", &gpio_085), PIN_TABLE("gpio_086", &gpio_086),\
	PIN_TABLE("gpio_087", &gpio_087), PIN_TABLE("gpio_088", &gpio_088), PIN_TABLE("gpio_089", &gpio_089), PIN_TABLE("gpio_090", &gpio_090),\
	PIN_TABLE("gpio_091", &gpio_091), PIN_TABLE("gpio_092", &gpio_092), PIN_TABLE("gpio_093", &gpio_093), PIN_TABLE("gpio_094", &gpio_094),\
	PIN_TABLE("gpio_095", &gpio_095), PIN_TABLE("gpio_096", &gpio_096), PIN_TABLE("gpio_097", &gpio_097), PIN_TABLE("gpio_098", &gpio_098),\
	PIN_TABLE("gpio_099", &gpio_099), PIN_TABLE("gpio_100", &gpio_100), PIN_TABLE("gpio_101", &gpio_101), PIN_TABLE("gpio_102", &gpio_102),\
	PIN_TABLE("gpio_103", &gpio_103), PIN_TABLE("gpio_104", &gpio_104), PIN_TABLE("gpio_105", &gpio_105), PIN_TABLE("gpio_106", &gpio_106),\
	PIN_TABLE("gpio_107", &gpio_107), PIN_TABLE("gpio_108", &gpio_108), PIN_TABLE("gpio_109", &gpio_109), PIN_TABLE("gpio_110", &gpio_110),\
	PIN_TABLE("gpio_111", &gpio_111), PIN_TABLE("gpio_112", &gpio_112), PIN_TABLE("gpio_113", &gpio_113), PIN_TABLE("gpio_114", &gpio_114),\
	PIN_TABLE("gpio_115", &gpio_115), PIN_TABLE("gpio_116", &gpio_116), PIN_TABLE("gpio_117", &gpio_117), PIN_TABLE("gpio_118", &gpio_118),\
	PIN_TABLE("gpio_119", &gpio_119), PIN_TABLE("gpio_120", &gpio_120), PIN_TABLE("gpio_121", &gpio_121), PIN_TABLE("gpio_122", &gpio_122),\
	PIN_TABLE("gpio_123", &gpio_123), PIN_TABLE("gpio_124", &gpio_124), PIN_TABLE("gpio_125", &gpio_125), PIN_TABLE("gpio_126", &gpio_126),\
	PIN_TABLE("gpio_127", &gpio_127), PIN_TABLE("gpio_128", &gpio_128), PIN_TABLE("gpio_129", &gpio_129), PIN_TABLE("gpio_130", &gpio_130),\
	PIN_TABLE("gpio_131", &gpio_131), PIN_TABLE("gpio_132", &gpio_132), PIN_TABLE("gpio_133", &gpio_133), PIN_TABLE("gpio_134", &gpio_134),\
	PIN_TABLE("gpio_135", &gpio_135), PIN_TABLE("gpio_136", &gpio_136), PIN_TABLE("gpio_137", &gpio_137), PIN_TABLE("gpio_138", &gpio_138),\
	PIN_TABLE("gpio_139", &gpio_139), PIN_TABLE("gpio_140", &gpio_140), PIN_TABLE("gpio_141", &gpio_141), PIN_TABLE("gpio_142", &gpio_142),\
	PIN_TABLE("gpio_143", &gpio_143), PIN_TABLE("gpio_144", &gpio_144), PIN_TABLE("gpio_145", &gpio_145), PIN_TABLE("gpio_146", &gpio_146),\
	PIN_TABLE("gpio_147", &gpio_147), PIN_TABLE("gpio_148", &gpio_148), PIN_TABLE("gpio_149", &gpio_149), PIN_TABLE("gpio_150", &gpio_150),\
	PIN_TABLE("gpio_151", &gpio_151), PIN_TABLE("gpio_152", &gpio_152), PIN_TABLE("gpio_153", &gpio_153), PIN_TABLE("gpio_154", &gpio_154),\
	PIN_TABLE("gpio_155", &gpio_155), PIN_TABLE("gpio_156", &gpio_156), PIN_TABLE("gpio_157", &gpio_157), PIN_TABLE("gpio_158", &gpio_158),\
	PIN_TABLE("gpio_159", &gpio_159), PIN_TABLE("gpio_160", &gpio_160), PIN_TABLE("gpio_161", &gpio_161), PIN_TABLE("gpio_162", &gpio_162),\
	PIN_TABLE("gpio_163", &gpio_163), PIN_TABLE("gpio_164", &gpio_164), PIN_TABLE("gpio_165", &gpio_165), PIN_TABLE("gpio_166", &gpio_166),\
	PIN_TABLE("gpio_167", &gpio_167), PIN_TABLE("gpio_168", &gpio_168), PIN_TABLE("gpio_169", &gpio_169), PIN_TABLE("gpio_170", &gpio_170),\
	PIN_TABLE("gpio_171", &gpio_171), PIN_TABLE("gpio_172", &gpio_172), PIN_TABLE("gpio_173", &gpio_173), PIN_TABLE("gpio_174", &gpio_174),\
	PIN_TABLE("gpio_175", &gpio_175),\
	PIN_TABLE("BOOT_MODE0", &BOOT_MODE0), PIN_TABLE("BOOT_MODE1", &BOOT_MODE1), PIN_TABLE("PMU_SPI_CLK", &PMU_SPI_CLK),\
	PIN_TABLE("PMU_SPI_DATA", &PMU_SPI_DATA), PIN_TABLE("PMU_SPI_CS_N", &PMU_SPI_CS_N), PIN_TABLE("I2S_DI", &I2S_DI),\
	PIN_TABLE("I2S_DO", &I2S_DO), PIN_TABLE("I2S_XCLK", &I2S_XCLK), PIN_TABLE("I2S_XFS", &I2S_XFS),\
	PIN_TABLE(NULL, NULL), \
};

struct iomux_pin_table pins_table_cs[] = {
	PIN_TABLE("gpio_003", &gpio_003_cs), PIN_TABLE("gpio_004", &gpio_004_cs), PIN_TABLE("gpio_005", &gpio_005_cs), PIN_TABLE("gpio_006", &gpio_006_cs),\
	PIN_TABLE("gpio_007", &gpio_007_cs), PIN_TABLE("gpio_008", &gpio_008_cs), PIN_TABLE("gpio_009", &gpio_009_cs), PIN_TABLE("gpio_010", &gpio_010_cs),\
	PIN_TABLE("gpio_011", &gpio_011_cs), PIN_TABLE("gpio_012", &gpio_012_cs), PIN_TABLE("gpio_013", &gpio_013_cs), PIN_TABLE("gpio_014", &gpio_014_cs),\
	PIN_TABLE("gpio_015", &gpio_015_cs), PIN_TABLE("gpio_016", &gpio_016_cs), PIN_TABLE("gpio_017", &gpio_017_cs), PIN_TABLE("gpio_018", &gpio_018_cs),\
	PIN_TABLE("gpio_019", &gpio_019_cs), PIN_TABLE("gpio_020", &gpio_020_cs), PIN_TABLE("gpio_021", &gpio_021_cs), PIN_TABLE("gpio_022", &gpio_022_cs),\
	PIN_TABLE("gpio_023", &gpio_023_cs), PIN_TABLE("gpio_024", &gpio_024_cs), PIN_TABLE("gpio_025", &gpio_025_cs), PIN_TABLE("gpio_026", &gpio_026_cs),\
	PIN_TABLE("gpio_027", &gpio_027_cs), PIN_TABLE("gpio_028", &gpio_028_cs), PIN_TABLE("gpio_029", &gpio_029_cs), PIN_TABLE("gpio_030", &gpio_030_cs),\
	PIN_TABLE("gpio_031", &gpio_031_cs), PIN_TABLE("gpio_032", &gpio_032_cs), PIN_TABLE("gpio_033", &gpio_033_cs), PIN_TABLE("gpio_034", &gpio_034_cs),\
	PIN_TABLE("gpio_035", &gpio_035_cs), PIN_TABLE("gpio_036", &gpio_036_cs), PIN_TABLE("gpio_037", &gpio_037_cs), PIN_TABLE("gpio_038", &gpio_038_cs),\
	PIN_TABLE("gpio_039", &gpio_039_cs), PIN_TABLE("gpio_040", &gpio_040_cs), PIN_TABLE("gpio_041", &gpio_041_cs), PIN_TABLE("gpio_042", &gpio_042_cs),\
	PIN_TABLE("gpio_043", &gpio_043_cs), PIN_TABLE("gpio_044", &gpio_044_cs), PIN_TABLE("gpio_045", &gpio_045_cs), PIN_TABLE("gpio_046", &gpio_046_cs),\
	PIN_TABLE("gpio_047", &gpio_047_cs), PIN_TABLE("gpio_048", &gpio_048_cs), PIN_TABLE("gpio_049", &gpio_049_cs), PIN_TABLE("gpio_050", &gpio_050_cs),\
	PIN_TABLE("gpio_051", &gpio_051_cs), PIN_TABLE("gpio_052", &gpio_052_cs), PIN_TABLE("gpio_053", &gpio_053_cs), PIN_TABLE("gpio_054", &gpio_054_cs),\
	PIN_TABLE("gpio_055", &gpio_055_cs), PIN_TABLE("gpio_056", &gpio_056_cs), PIN_TABLE("gpio_057", &gpio_057_cs), PIN_TABLE("gpio_058", &gpio_058_cs),\
	PIN_TABLE("gpio_059", &gpio_059_cs), PIN_TABLE("gpio_060", &gpio_060_cs), PIN_TABLE("gpio_061", &gpio_061_cs), PIN_TABLE("gpio_062", &gpio_062_cs),\
	PIN_TABLE("gpio_063", &gpio_063_cs), PIN_TABLE("gpio_064", &gpio_064_cs), PIN_TABLE("gpio_065", &gpio_065_cs), PIN_TABLE("gpio_066", &gpio_066_cs),\
	PIN_TABLE("gpio_067", &gpio_067_cs), PIN_TABLE("gpio_068", &gpio_068_cs), PIN_TABLE("gpio_069", &gpio_069_cs), PIN_TABLE("gpio_070", &gpio_070_cs),\
	PIN_TABLE("gpio_071", &gpio_071_cs), PIN_TABLE("gpio_072", &gpio_072_cs), PIN_TABLE("gpio_073", &gpio_073_cs), PIN_TABLE("gpio_074", &gpio_074_cs),\
	PIN_TABLE("gpio_075", &gpio_075_cs), PIN_TABLE("gpio_076", &gpio_076_cs), PIN_TABLE("gpio_077", &gpio_077_cs), PIN_TABLE("gpio_078", &gpio_078_cs),\
	PIN_TABLE("gpio_079", &gpio_079_cs), PIN_TABLE("gpio_080", &gpio_080_cs), PIN_TABLE("gpio_081", &gpio_081_cs), PIN_TABLE("gpio_082", &gpio_082_cs),\
	PIN_TABLE("gpio_083", &gpio_083_cs), PIN_TABLE("gpio_084", &gpio_084_cs), PIN_TABLE("gpio_085", &gpio_085_cs), PIN_TABLE("gpio_086", &gpio_086_cs),\
	PIN_TABLE("gpio_087", &gpio_087_cs), PIN_TABLE("gpio_088", &gpio_088_cs), PIN_TABLE("gpio_089", &gpio_089_cs), PIN_TABLE("gpio_090", &gpio_090_cs),\
	PIN_TABLE("gpio_091", &gpio_091_cs), PIN_TABLE("gpio_092", &gpio_092_cs), PIN_TABLE("gpio_093", &gpio_093_cs), PIN_TABLE("gpio_094", &gpio_094_cs),\
	PIN_TABLE("gpio_095", &gpio_095_cs), PIN_TABLE("gpio_096", &gpio_096_cs), PIN_TABLE("gpio_097", &gpio_097_cs), PIN_TABLE("gpio_098", &gpio_098_cs),\
	PIN_TABLE("gpio_099", &gpio_099_cs), PIN_TABLE("gpio_100", &gpio_100_cs), PIN_TABLE("gpio_101", &gpio_101_cs), PIN_TABLE("gpio_102", &gpio_102_cs),\
	PIN_TABLE("gpio_103", &gpio_103_cs), PIN_TABLE("gpio_104", &gpio_104_cs), PIN_TABLE("gpio_105", &gpio_105_cs), PIN_TABLE("gpio_106", &gpio_106_cs),\
	PIN_TABLE("gpio_107", &gpio_107_cs), PIN_TABLE("gpio_108", &gpio_108_cs), PIN_TABLE("gpio_109", &gpio_109_cs), PIN_TABLE("gpio_110", &gpio_110_cs),\
	PIN_TABLE("gpio_111", &gpio_111_cs), PIN_TABLE("gpio_112", &gpio_112_cs), PIN_TABLE("gpio_113", &gpio_113_cs), PIN_TABLE("gpio_114", &gpio_114_cs),\
	PIN_TABLE("gpio_115", &gpio_115_cs), PIN_TABLE("gpio_116", &gpio_116_cs), PIN_TABLE("gpio_117", &gpio_117_cs), PIN_TABLE("gpio_118", &gpio_118_cs),\
	PIN_TABLE("gpio_119", &gpio_119_cs), PIN_TABLE("gpio_120", &gpio_120_cs), PIN_TABLE("gpio_121", &gpio_121_cs), PIN_TABLE("gpio_122", &gpio_122_cs),\
	PIN_TABLE("gpio_123", &gpio_123_cs), PIN_TABLE("gpio_124", &gpio_124_cs), PIN_TABLE("gpio_125", &gpio_125_cs), PIN_TABLE("gpio_126", &gpio_126_cs),\
	PIN_TABLE("gpio_127", &gpio_127_cs), PIN_TABLE("gpio_128", &gpio_128_cs), PIN_TABLE("gpio_129", &gpio_129_cs), PIN_TABLE("gpio_130", &gpio_130_cs),\
	PIN_TABLE("gpio_131", &gpio_131_cs), PIN_TABLE("gpio_132", &gpio_132_cs), PIN_TABLE("gpio_133", &gpio_133_cs), PIN_TABLE("gpio_134", &gpio_134_cs),\
	PIN_TABLE("gpio_135", &gpio_135_cs), PIN_TABLE("gpio_136", &gpio_136_cs), PIN_TABLE("gpio_137", &gpio_137_cs), PIN_TABLE("gpio_138", &gpio_138_cs),\
	PIN_TABLE("gpio_139", &gpio_139_cs), PIN_TABLE("gpio_140", &gpio_140_cs), PIN_TABLE("gpio_141", &gpio_141_cs), PIN_TABLE("gpio_142", &gpio_142_cs),\
	PIN_TABLE("gpio_143", &gpio_143_cs), PIN_TABLE("gpio_144", &gpio_144_cs), PIN_TABLE("gpio_145", &gpio_145_cs), PIN_TABLE("gpio_146", &gpio_146_cs),\
	PIN_TABLE("gpio_147", &gpio_147_cs), PIN_TABLE("gpio_148", &gpio_148_cs), PIN_TABLE("gpio_149", &gpio_149_cs), PIN_TABLE("gpio_150", &gpio_150_cs),\
	PIN_TABLE("gpio_151", &gpio_151_cs), PIN_TABLE("gpio_152", &gpio_152_cs), PIN_TABLE("gpio_153", &gpio_153_cs), PIN_TABLE("gpio_154", &gpio_154_cs),\
	PIN_TABLE("gpio_155", &gpio_155_cs), PIN_TABLE("gpio_156", &gpio_156_cs), PIN_TABLE("gpio_157", &gpio_157_cs), PIN_TABLE("gpio_158", &gpio_158_cs),\
	PIN_TABLE("gpio_159", &gpio_159_cs), PIN_TABLE("gpio_160", &gpio_160_cs), PIN_TABLE("gpio_161", &gpio_161_cs), PIN_TABLE("gpio_162", &gpio_162_cs),\
	PIN_TABLE("gpio_163", &gpio_163_cs), PIN_TABLE("gpio_164", &gpio_164_cs), PIN_TABLE("gpio_165", &gpio_165_cs), PIN_TABLE("gpio_166", &gpio_166_cs),\
	PIN_TABLE("gpio_167", &gpio_167_cs), PIN_TABLE("gpio_168", &gpio_168_cs), PIN_TABLE("gpio_169", &gpio_169_cs),\
	PIN_TABLE("gpio_170", &gpio_170_cs), PIN_TABLE("gpio_171", &gpio_171_cs), PIN_TABLE("gpio_172", &gpio_172_cs),\
	PIN_TABLE("gpio_173", &gpio_173_cs), PIN_TABLE("gpio_174", &gpio_174_cs), PIN_TABLE("gpio_175", &gpio_175_cs),\
	PIN_TABLE("BOOT_MODE0", &BOOT_MODE0), PIN_TABLE("BOOT_MODE1", &BOOT_MODE1), PIN_TABLE("PMU_SPI_CLK", &PMU_SPI_CLK),\
	PIN_TABLE("PMU_SPI_DATA", &PMU_SPI_DATA), PIN_TABLE("PMU_SPI_CS_N", &PMU_SPI_CS_N), PIN_TABLE("I2S_DI", &I2S_DI),\
	PIN_TABLE("I2S_DO", &I2S_DO), PIN_TABLE("I2S_XCLK", &I2S_XCLK), PIN_TABLE("I2S_XFS", &I2S_XFS),\
	PIN_TABLE(NULL, NULL), \
};


#endif
