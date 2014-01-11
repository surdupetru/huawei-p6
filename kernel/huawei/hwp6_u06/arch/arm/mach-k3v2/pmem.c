/*
 *  linux/arch/arm/mach-hi3620/pmem.c
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
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/io.h>
#include <linux/ata_platform.h>
#include <linux/amba/mmci.h>

#include <asm/clkdev.h>
#include <asm/system.h>
#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/leds.h>
#include <asm/mach-types.h>
#include <asm/hardware/arm_timer.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include <asm/hardware/gic.h>

#include <mach/platform.h>
#include <mach/irqs.h>
#include <mach/early-debug.h>
#include <linux/android_pmem.h>
#include <linux/bootmem.h>
#include <mach/hisi_mem.h>

static struct android_pmem_platform_data android_pmem_camera_pdata = {
	.name = "camera_pmem",
	.type = PMEM_TYPE_K3,
	.cached = 0,
};

static struct platform_device android_pmem_camera_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_camera_pdata},
};

static struct android_pmem_platform_data android_pmem_gralloc_pdata = {
	.name = "gralloc_pmem",
	.type = PMEM_TYPE_K3,
	.cached = 0,
};

static struct platform_device android_pmem_gralloc_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_gralloc_pdata},
};

#if defined(CONFIG_OVERLAY_COMPOSE)
static struct android_pmem_platform_data android_pmem_overlay_pdata = {
	.name = "overlay_pmem",
	.type = PMEM_TYPE_K3,
	.cached = 1,
};

static struct platform_device android_pmem_overlay_device = {
	.name = "android_pmem",
	.id = 3,
	.dev = { .platform_data = &android_pmem_overlay_pdata},
};
#endif

static struct platform_device *pmem_devs[] __initdata = {
	&android_pmem_camera_device,
	&android_pmem_gralloc_device,
#if defined(CONFIG_OVERLAY_COMPOSE)
	&android_pmem_overlay_device,
#endif
};

unsigned long hisi_reserved_gpu_phymem;
unsigned long hisi_reserved_fb_phymem;
unsigned long hisi_reserved_codec_phymem;
unsigned long hisi_reserved_dumplog_phymem;
unsigned long hisi_reserved_camera_phymem;
unsigned long hisi_reserved_vpp_phymem;
EXPORT_SYMBOL(hisi_reserved_gpu_phymem);
EXPORT_SYMBOL(hisi_reserved_fb_phymem);
EXPORT_SYMBOL(hisi_reserved_codec_phymem);
EXPORT_SYMBOL(hisi_reserved_dumplog_phymem);
EXPORT_SYMBOL(hisi_reserved_camera_phymem);
EXPORT_SYMBOL(hisi_reserved_vpp_phymem);

#define HISI_LCD_SIZE_NAME 10
struct hisi_reserved_media_memory {
	unsigned char lcd_name[HISI_LCD_SIZE_NAME];
	unsigned long gpu_size;
	unsigned long codec_size;
	unsigned long camera_size;
	unsigned long gralloc_size;
	unsigned long vpp_size;
#if defined(CONFIG_OVERLAY_COMPOSE)
	unsigned long overlay_size;
#endif
	unsigned long fb_size;
};

static struct hisi_reserved_media_memory hisi_media_mem_array[] = {
	[0] = {
		.lcd_name = "hvga",
		.gpu_size = HISI_MEM_GPU_SIZE,
		.codec_size = HISI_MEM_CODEC_SIZE,
		.camera_size = HISI_PMEM_CAMERA_SIZE,
		.gralloc_size = HISI_PMEM_GRALLOC_SIZE,
		.vpp_size = HISI_MEM_VPP_SIZE,
	#if defined(CONFIG_OVERLAY_COMPOSE)
		.overlay_size = HISI_PMEM_OVERLAY_SIZE,
	#endif
		.fb_size = HISI_MEM_FB_SIZE,
	},
	[1] = {
		.lcd_name = "vga",
		.gpu_size = HISI_MEM_GPU_SIZE,
		.codec_size = HISI_MEM_CODEC_SIZE,
		.camera_size = HISI_PMEM_CAMERA_SIZE,
		.gralloc_size = HISI_PMEM_GRALLOC_SIZE,
		.vpp_size = HISI_MEM_VPP_SIZE,
	#if defined(CONFIG_OVERLAY_COMPOSE)
		.overlay_size = HISI_PMEM_OVERLAY_SIZE,
	#endif
		.fb_size = HISI_MEM_FB_SIZE,
	},
	[2] = {
		.lcd_name = "dvga",
		.gpu_size = HISI_MEM_GPU_SIZE,
		.codec_size = HISI_MEM_CODEC_SIZE,
		.camera_size = HISI_PMEM_CAMERA_SIZE,
		.gralloc_size = HISI_PMEM_GRALLOC_SIZE,
		.vpp_size = HISI_MEM_VPP_SIZE,
	#if defined(CONFIG_OVERLAY_COMPOSE)
		.overlay_size = HISI_PMEM_OVERLAY_SIZE,
	#endif
		.fb_size = HISI_MEM_FB_SIZE,
	},
	[3] = {
		.lcd_name = "720p",
		.gpu_size = HISI_MEM_GPU_SIZE,
		.codec_size = HISI_MEM_CODEC_SIZE,
		.camera_size = HISI_PMEM_CAMERA_SIZE,
		.gralloc_size = HISI_PMEM_GRALLOC_SIZE,
		.vpp_size = HISI_MEM_VPP_SIZE,
	#if defined(CONFIG_OVERLAY_COMPOSE)
		.overlay_size = HISI_PMEM_OVERLAY_SIZE,
	#endif
		.fb_size = HISI_MEM_FB_SIZE,
	},
	[4] = {
		.lcd_name = "1080p",
		.gpu_size = HISI_MEM_GPU_SIZE,
		.codec_size = HISI_MEM_CODEC_SIZE,
		.camera_size = HISI_PMEM_CAMERA_SIZE,
		.gralloc_size = HISI_PMEM_GRALLOC_SIZE,
		.vpp_size = HISI_MEM_VPP_SIZE,
	#if defined(CONFIG_OVERLAY_COMPOSE)
		.overlay_size = HISI_PMEM_OVERLAY_SIZE,
	#endif
		.fb_size = HISI_MEM_FB_SIZE,
	},
};

static struct hisi_reserved_media_memory hisi_media_mem = {
	.lcd_name = "720p",
	.gpu_size = HISI_MEM_GPU_SIZE,
	.codec_size = HISI_MEM_CODEC_SIZE,
	.camera_size = HISI_PMEM_CAMERA_SIZE,
	.gralloc_size = HISI_PMEM_GRALLOC_SIZE,
	.vpp_size = HISI_MEM_VPP_SIZE,
#if defined(CONFIG_OVERLAY_COMPOSE)
	.overlay_size = HISI_PMEM_OVERLAY_SIZE,
#endif
	.fb_size = HISI_MEM_FB_SIZE,
};

/*
	|<--------------------------------------------512MB------------------------------------------->|
	|_______system memory______________________gpu___codec___camera__gralloc_dumplog__16M reserved_|
	|                                       |       |       |       |       |       |              |
	|                                       |       |       |       |       |       |              |
	|                                       |       |       |       |       |       |              |
	|_______________________________________|_______|_______|_______|_______|_____ _|______________|
     0x0000000                                                                                   0x20000000
1. GPU + display + vpp （Gralloc分配）   150M    
         --phy  --ioremap ---> virt  (no needd register to pmem)    -----> 利用参数传递物理地址 hisi_reserved_media_phymem

2. jpeg decoder input/output  + jpeg encoder output  30M   
   video decoder ref + video encoder output
         --phy ---> ioremap --->virt (不需要注册成pmem)   ----> 利用参数传递物理地址 hisi_reserved_codec_phymem
 
3. camera capture            48MB   
         -- pmem ---> pem-ioctl -- app map/unmap   (需要注册成pmem)

  
4. video decoder display (Gralloc分配，保留一帧)  20M   
   camera preview + Thumbnail（Gralloc分配）
         ---pmem -- > pmem-ioctl --- app map/unmap  (需要注册成pmem)

5. dumplog memory			2M
*/
unsigned long hisi_get_reserve_mem_size(void)
{
	unsigned long reserved = SZ_2M;
	unsigned long section_size = 1<<SECTION_SIZE_BITS;

	reserved += hisi_media_mem.gpu_size;
	reserved += hisi_media_mem.codec_size;
	reserved += hisi_media_mem.camera_size;
	reserved += hisi_media_mem.gralloc_size;
	reserved += hisi_media_mem.vpp_size;
#if defined(CONFIG_OVERLAY_COMPOSE)
	reserved += hisi_media_mem.overlay_size;
#endif
	reserved += hisi_media_mem.fb_size;
	reserved += HISI_PMEM_DUMPLOG_SIZE;
	reserved = (reserved & 0xFFF00000) + SZ_1M;
	printk("ori reserved = 0x%x\n",reserved);
	#ifdef CONFIG_SPARSEMEM
	reserved = ((reserved/section_size) + 1) * section_size;
	#endif
	printk("reserved = 0x%x\n",reserved);
	return reserved;
}

unsigned long hisi_get_reserve_gpu_mem_size(void)
{
	return hisi_media_mem.gpu_size;
}

/* lcd_density=hvga/vga/dvga and so on */
static int __init early_k3v2_lcd_density(char *p)
{
	int mach = 0;
	int i;

	printk("k3v2 lcd density = %s\n", p);
	for (i = 0; i < ARRAY_SIZE(hisi_media_mem_array); i++) {
		if (!strncmp(p, hisi_media_mem_array[i].lcd_name, strlen(p))) {
			mach = 1;
			break;
		}
	}

	if (mach) {
		hisi_media_mem.gpu_size = hisi_media_mem_array[i].gpu_size;
		hisi_media_mem.codec_size = hisi_media_mem_array[i].codec_size;
		hisi_media_mem.camera_size = hisi_media_mem_array[i].camera_size;
		hisi_media_mem.gralloc_size = hisi_media_mem_array[i].gralloc_size;
		hisi_media_mem.vpp_size = hisi_media_mem_array[i].vpp_size;
	#if defined(CONFIG_OVERLAY_COMPOSE)
		hisi_media_mem.overlay_size = hisi_media_mem_array[i].overlay_size;
	#endif
		hisi_media_mem.fb_size = hisi_media_mem_array[i].fb_size;
		memcpy(hisi_media_mem.lcd_name, p, strlen(p) + 1);

		printk("lcd %s gpu %luMB codec %luMB camera %luMB gralloc %luMB vpp %luMB"
			#if defined(CONFIG_OVERLAY_COMPOSE)
				" overlay %luMB"
			#endif
				" fb %luMB\n",
				hisi_media_mem.lcd_name,
				hisi_media_mem.gpu_size / SZ_1M,
				hisi_media_mem.codec_size / SZ_1M,
				hisi_media_mem.camera_size / SZ_1M,
				hisi_media_mem.gralloc_size / SZ_1M,
				hisi_media_mem.vpp_size / SZ_1M,
			#if defined(CONFIG_OVERLAY_COMPOSE)
				hisi_media_mem.overlay_size / SZ_1M,
			#endif
				hisi_media_mem.fb_size / SZ_1M
		);
	} else {
		printk("lcd %s not found!\n", p);
	}

	return 0;
}
early_param("k3v2_lcd_density", early_k3v2_lcd_density);

void __init k3v2_allocate_memory_regions(void)
{
	unsigned long reserved_base = PLAT_PHYS_OFFSET + HISI_BASE_MEMORY_SIZE;
	unsigned long reserved_size;
	unsigned long size;

	edb_trace(1);

	reserved_size = hisi_get_reserve_mem_size();

	reserved_base -= (reserved_size - SZ_1M);

	/* GPU memory */
	size = hisi_media_mem.gpu_size;
	hisi_reserved_gpu_phymem = reserved_base;
	printk("pmem allocating %lu bytes at (%lx physical) for gpu "
		"pmem area\n", size, reserved_base);

	reserved_base += size;

	/* CODEC memory */
	size = hisi_media_mem.codec_size;
	/*Revived by y44207 ,V200 64 byte align*/
	hisi_reserved_codec_phymem = reserved_base; 
	printk("codec pmem allocating %lu bytes at (%lx physical) for codec "
		"pcodec area\n", size, reserved_base);
	reserved_base += size;
	/* VPP memory */
	size = hisi_media_mem.vpp_size;
	hisi_reserved_vpp_phymem = reserved_base; 
	printk("pmem allocating %lu bytes at (%lx physical) for vpp "
		"pmem area\n", size, reserved_base);

	reserved_base += size;

	/* CAMERA memory pmem */
	size = hisi_media_mem.camera_size;
	if (size) {
		android_pmem_camera_pdata.start = reserved_base;
		android_pmem_camera_pdata.size  = size;
		hisi_reserved_camera_phymem = android_pmem_camera_pdata.start;
		printk("camera pmem allocating %lu bytes at (%lx physical) for camera pic\n",
			size, reserved_base);
	}

	reserved_base += size;

	/* video decoder display && camera preview + Thumbnail */
	size = hisi_media_mem.gralloc_size;
	if (size) {
		android_pmem_gralloc_pdata.start = reserved_base;
		android_pmem_gralloc_pdata.size = size;
		printk("camera pmem allocating %lu bytes at (%lx physical) for gralloc "
			"pmem area\n", size, reserved_base);
	}

	reserved_base += size;

#if defined(CONFIG_OVERLAY_COMPOSE)
	size = hisi_media_mem.overlay_size;
	if (size) {
		android_pmem_overlay_pdata.start = reserved_base;
		android_pmem_overlay_pdata.size = size;
		printk("overlay pmem allocating %lu bytes at (%lx physical) for overlay "
			"pmem area\n", size, reserved_base);
	}

	reserved_base += size;
#endif

	/*FB memory*/
	size = hisi_media_mem.fb_size;
	hisi_reserved_fb_phymem = reserved_base;
	printk("pmem allocating %lu bytes at (%lx physical) for fb "
		"pmem area\n",size,reserved_base);

	reserved_base += size;

	/* dumplog memory */
	size = HISI_PMEM_DUMPLOG_SIZE;
	/*Revived by y44207 ,V200 64 byte align*/
	hisi_reserved_dumplog_phymem = reserved_base; 
	printk("dumplog pmem allocating %lu bytes at (%lx physical) for dumplog"
		"area\n", size, reserved_base);

	reserved_base += size;
}

static int __init k3v2_pmem_setup(char *str)
{
	k3v2_allocate_memory_regions();

	return 1;
}
__setup("k3v2_pmem", k3v2_pmem_setup);

static int __init k3v2_pmem_init(void)
{
	platform_add_devices(pmem_devs, ARRAY_SIZE(pmem_devs));

	return 0;
}
arch_initcall(k3v2_pmem_init);
