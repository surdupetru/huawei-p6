#ifndef _HI_MEM_INCLUDE_H_
#define _HI_MEM_INCLUDE_H_

#include <linux/mm.h>


extern unsigned long hisi_reserved_codec_phymem;
extern unsigned long hisi_reserved_gpu_phymem;
extern unsigned long hisi_reserved_fb_phymem;
extern unsigned long hisi_reserved_dumplog_phymem;
extern unsigned long hisi_reserved_camera_phymem;
extern unsigned long hisi_reserved_vpp_phymem;


#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align)  ((x) & ~((align)-1))
#endif
#ifndef STRIDE_ALIGN
#define STRIDE_ALIGN(stride)	\
	((((stride) / 64) % 2 == 0) ? ((stride) + 64) : (stride))
#endif

#ifdef CONFIG_LCD_TOSHIBA_MDW70
#define LCD_XRES	(720)
#define LCD_YRES	(1280)
#elif defined(CONFIG_LCD_PANASONIC_VVX10F002A00)
#define LCD_XRES	(1920)
#define LCD_YRES	(1200)
#elif defined(CONFIG_LCD_CMI_OTM1280A)
#define LCD_XRES	(720)
#define LCD_YRES	(1280)
#elif defined(CONFIG_LCD_SAMSUNG_S6E39A)
#define LCD_XRES	(540)
#define LCD_YRES	(960)
#elif defined(CONFIG_LCD_SAMSUNG_LMS350DF04)
#define LCD_XRES	(320)
#define LCD_YRES	(480)
#elif defined(CONFIG_LCD_SHARP_LS035B3SX)
#define LCD_XRES	(640)
#define LCD_YRES	(960)
#elif defined(CONFIG_LCD_JDI_OTM1282B)
#define LCD_XRES	(720)
#define LCD_YRES	(1280)
#elif defined(CONFIG_LCD_CMI_PT045TN07)
#define LCD_XRES	(540)
#define LCD_YRES	(960)
#elif defined(CONFIG_LCD_TOSHIBA_MDY90)
#define LCD_XRES	(1080)
#define LCD_YRES	(1920)
#elif defined(CONFIG_LCD_K3_FAKE)
#define LCD_XRES	(720)
#define LCD_YRES	(1280)
#else
#error "LCD_XRES and LCD_YRES not defined"
#endif
#define LCD_STRIDE	(STRIDE_ALIGN(ALIGN_UP((LCD_XRES * 4), 64)))
#define NUM_FRAME_BUFFERS  4


#ifdef CONFIG_MACH_TC45MSU3
#define HISI_BASE_MEMORY_SIZE	(SZ_512M)
#else
#define HISI_BASE_MEMORY_SIZE	(SZ_1G)
#endif

#define HISI_MEM_GPU_SIZE	(HIGPU_BUF_SIZE)
/* mem for framebuffer, stride must be 64 bytes odd align */
#define HISI_MEM_FB_SIZE	PAGE_ALIGN(LCD_YRES * LCD_STRIDE * NUM_FRAME_BUFFERS)
#define HISI_PMEM_CAMERA_SIZE	(4 * SZ_1K)
#if defined CONFIG_PMEM_ALTERNATIVES && CONFIG_PMEM_CODEC_SIZE != 0
#define HISI_MEM_CODEC_SIZE (CONFIG_PMEM_CODEC_SIZE * SZ_1M)
#else
#define HISI_MEM_CODEC_SIZE	(49 * SZ_1M)
#endif
#define HISI_MEM_VPP_SIZE	(5 * SZ_1M)
#if defined CONFIG_PMEM_ALTERNATIVES && CONFIG_PMEM_GRALLOC_SIZE != 0
#define HISI_PMEM_GRALLOC_SIZE (CONFIG_PMEM_GRALLOC_SIZE * SZ_1M)
#else
#if defined(CONFIG_LCD_1080P)
#define HISI_PMEM_GRALLOC_SIZE  (133 * SZ_1M)
#else
#define HISI_PMEM_GRALLOC_SIZE	(135 * SZ_1M)
#endif
#endif
#define HISI_PMEM_DUMPLOG_SIZE	(2 * SZ_1M)

/* pmem for overlay */
#if defined(CONFIG_OVERLAY_COMPOSE)
#define NUM_BUFFERS_ONE_LAYER	(3)
#define NUM_LAYERS	(5)
#define HISI_PMEM_OVERLAY_SIZE	PAGE_ALIGN(LCD_YRES * LCD_STRIDE * NUM_BUFFERS_ONE_LAYER * NUM_LAYERS)
#else
#define HISI_PMEM_OVERLAY_SIZE	(0)
#endif

/* NOTE: for HDMI preserve mem: alloc from GPU BUF */
#define HDMI_XRES	(1440)
#define HDMI_YRES	(1050)
#define NUM_HDMI_BUFFERS	(3)
#define HISI_HDMI_BUFFER_SIZE	PAGE_ALIGN(HDMI_XRES * HDMI_YRES * 4 *NUM_HDMI_BUFFERS)

/* temp */
#define CAMERA_PREVIEW_BUF_BASE (hisi_reserved_camera_phymem)
#define CAMERA_PREVIEW_BUF_SIZ	(hisi_reserved_media_phymem)

/* GPU */
#define HIGPU_BUF_BASE	(hisi_reserved_gpu_phymem)
#if defined CONFIG_PMEM_ALTERNATIVES && CONFIG_PMEM_HIGPU_BUF_SIZE != 0
#define HIGPU_BUF_SIZE (CONFIG_PMEM_HIGPU_BUF_SIZE * SZ_1M + HISI_HDMI_BUFFER_SIZE)
#else
#define HIGPU_BUF_SIZE	(128 * SZ_1M + HISI_HDMI_BUFFER_SIZE)
#endif

/* framebuffer */
#define HISI_FRAME_BUFFER_BASE    (hisi_reserved_fb_phymem)


void __init hisi_allocate_memory_regions(void);
unsigned long hisi_get_reserve_mem_size(void);
unsigned long hisi_get_reserve_gpu_mem_size(void);


#endif /* end _HI_MEM_INCLUDE_H_ */
