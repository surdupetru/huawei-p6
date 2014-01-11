/******************************************************************************

  Copyright (C), 2001-2011, Huawei Tech. Co., Ltd.

******************************************************************************
  File Name     : hifb_main.c
  Version       : Initial Draft
  Author        : w54130
  Created       : 2007/5/25
  Last Modified :
  Description   : framebuffer external function
  History       :
  1.Date        : 2007/5/25
    Author      : w54130
    Modification: Created file

  2.Date        : 2007/11/07
    Author      : w54130
    Modification:AE6D02359(modified function hifb_check_var_xbpp and hifb_check_var_1632)

  3.Date        : 2008/01/23
    Author      : w54130
    Modification: AE5D02630(add check for xoffset and yoffset in hifb_check_var_xbpp and hifb_check_var_1632)

  4.Date        : 2008/01/23
    Author      : w54130
    Modification: AE5D02631(copy temporary varible to user point agrp in FBIOGET_DEFLICKER_HIFB)

  5.Date        : 2008/06/26
    Author      : s37678
    Modification: fix the defect AE5D02966
    

******************************************************************************/

#include <linux/ctype.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <linux/slab.h>
#include <linux/mm.h>

#include <linux/fb.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/early-debug.h>
#include <linux/platform_device.h>
#include <mach/hisi_mem.h>

#define HIFB_DEF_WIDTH      (720)
#define HIFB_DEF_HEIGHT     (1280)
#define HIFB_DEF_DEPTH      (32)

#define HIFB_DEF_STRIDE     (HIFB_DEF_WIDTH*HIFB_DEF_DEPTH/8)

/* default fix information */
static struct fb_fix_screeninfo gsthifb_def_fix =
{
    .id          = "hifb",
    .type        = FB_TYPE_PACKED_PIXELS,
    .visual      = FB_VISUAL_TRUECOLOR,
    .xpanstep    =                     1,
    .ypanstep    =                     1,
    .ywrapstep   =                     0,
    .line_length = HIFB_DEF_STRIDE,
    .accel       = FB_ACCEL_NONE,
    .mmio_len    =                     0,
    .mmio_start  =                     0,
};

/* default variable information */
static struct fb_var_screeninfo gsthifb_def_var =
{
    .xres			= HIFB_DEF_WIDTH,
    .yres			= HIFB_DEF_HEIGHT,
    .xres_virtual	= HIFB_DEF_WIDTH,
    .yres_virtual	= HIFB_DEF_HEIGHT*3,   /* 3 buffers */
    .bits_per_pixel = HIFB_DEF_DEPTH,
    .red    		= {16, 8, 0},
    .green  		= {8, 8, 0},
    .blue   		= {0, 8, 0},
    .transp 		= {24, 8, 0},
    .activate		= FB_ACTIVATE_VBL,
    .width			= -1,
    .height			= -1,
    .pixclock		= -1, /* pixel clock in ps (pico seconds) */
    .left_margin	= -1, /* time from sync to picture	*/
    .right_margin	= -1, /* time from picture to sync	*/
    .upper_margin	= -1, /* time from sync to picture	*/
    .lower_margin	= -1,
    .hsync_len		= -1, /* length of horizontal sync	*/
    .vsync_len		= -1, /* length of vertical sync	*/
};

/******************************************************************************
 Function        : hifb_check_var
 Description     : check if the paramter for framebuffer is supported.
 Data Accessed   :
 Data Updated    :
 Output          : None
 Input           : struct fb_var_screeninfo *var
                   struct fb_info *info
 Return          : return 0, if the paramter is supported, otherwise,return error
                   code.
 Others          : 0
******************************************************************************/
static int hifb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	return 0;
}

/******************************************************************************
 Function        : hifb_set_par
 Description     : set the variable parmater and make it use
 Data Accessed   :
 Data Updated    :
 Output          : None
 Input           : struct fb_info *info
 Return          : return 0
 Others          : 0
******************************************************************************/
static int hifb_set_par(struct fb_info *info)
{
    return 0;
}

/******************************************************************************
 Function        : hifb_pan_display
 Description     : pan display.
 Data Accessed   :
 Data Updated    :
 Output          : None
 Input           : struct fb_var_screeninfo *var
 Return          : return 0
 Others          : 0
******************************************************************************/
static int hifb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    return 0;
}

/******************************************************************************
 Function        : hifb_ioctl
 Description     : set the colorkey or alpha for overlay
 Data Accessed   :
 Data Updated    :
 Output          : None
 Input           : struct inode *inode
                   struct file *file
                   unsigned int cmd
                   unsigned long arg
                   struct fb_info *info
 Return          : return 0 if succeed, otherwise return error code
 Others          : 0
******************************************************************************/
/* This is a Hisi Driver BUG, DONOT match prototype in fb.h, 2010/02/23 */

/*static int hifb_ioctl(struct inode *inode, struct file *file,
                      struct fb_info *info, unsigned long arg,
                      unsigned int cmd)*/  //svn original

static int hifb_ioctl(struct fb_info *info,unsigned int cmd, 
                        unsigned long arg)
{
    return 0;
}


/******************************************************************************
 Function        : hifb_open
 Description     : open the framebuffer and using the default parameter to set the layer
 Data Accessed   :
 Data Updated    :
 Output          : None
                   struct fb_info *info
 Return          : return 0
 Others          : 0
******************************************************************************/
static int hifb_open(struct fb_info *info, int user)
{
	static int initflags =0;

    if(!initflags++)
    {
        info->fix.smem_start = (int32_t)HISI_FRAME_BUFFER_BASE;
        info->screen_base = ioremap(info->fix.smem_start, (info->fix.smem_len));
        info->screen_size = info->fix.smem_len;
        memset(info->screen_base, 0x0, info->fix.smem_len);
    }

    return 0;
}

/******************************************************************************
 Function        : hifb_release
 Description     : open the framebuffer and disable the layer
 Data Accessed   :
 Data Updated    :
 Output          : None
                   struct fb_info *info
 Return          : return 0 if succeed, otherwise return -EINVAL
 Others          : 0
******************************************************************************/
static int hifb_release (struct fb_info *info, int user)
{
    return 0;
}

static struct fb_ops gsthifb_ops =
{
    .owner			= THIS_MODULE,/*lint !e43*//*lint !e110*//*lint !e133*/
    .fb_open		= hifb_open,
    .fb_release		= hifb_release,
    .fb_check_var	= hifb_check_var,
    .fb_set_par		= hifb_set_par,
    .fb_pan_display = hifb_pan_display,
    .fb_ioctl		= hifb_ioctl,
};

static int hi_framebuffer_probe(struct platform_device *pdev)
{
	int ret = 0;
    struct fb_info * info = NULL;

    /* Creates a new frame buffer info structure. reserves hifb_par for driver private data (info->par) */
    info = framebuffer_alloc(0, NULL);
    if (!info){
    	printk("framebuffer alloc failed\n");
        return -ENOMEM;
    }

    /* initialize the fix screen info */
    info->fix = gsthifb_def_fix;

    info->fix.smem_len = gsthifb_def_var.xres_virtual*gsthifb_def_var.yres_virtual*gsthifb_def_var.bits_per_pixel/8;
    info->screen_size = info->fix.smem_len;

    /* initialize the variable info */
    info->var = gsthifb_def_var;

    /* initialize file operations */
    info->fbops = &gsthifb_ops;
    info->flags = FBINFO_FLAG_DEFAULT | FBINFO_HWACCEL_YPAN | FBINFO_HWACCEL_XPAN;

    if (register_framebuffer(info) < 0){
        printk("failed to register_framebuffer!\n");
        ret = -EINVAL;
        goto ERR;
    }

    /*printk("succeed in registering the fb%d: %s frame buffer device, info->screen_size=%lu\n", info->node, info->fix.id, info->screen_size);*/

    return 0;
    
ERR:
	return ret;
}

static int hi_framebuffer_remove(void)
{   
    return 0;
}

static int hi_framebuffer_suspend(void)
{
    return 0;
}

static int hi_framebuffer_resume(void)
{
    return 0;
}

static void hi_framebuffer_shutdown(void)
{
    return;
}

/******************************************************************************
 Function        : hifb_init
 Description     : initialize framebuffer. the function is called when
                   loading the moudel
 Data Accessed   :
 Data Updated    :
 Output          : None
 Input           : void
 Return          : 0, if success; otherwise, return error code
 Others          : 0
******************************************************************************/

static struct platform_device k3framebuffer_device = {
        .name           = "k3-fakefb",
};

static struct platform_driver k3framebuffer_driver = {
	.probe          = hi_framebuffer_probe,
	.remove         = hi_framebuffer_remove,
	.suspend 		= hi_framebuffer_suspend,
	.resume			= hi_framebuffer_resume,
	.shutdown       = hi_framebuffer_shutdown,
	.driver         = {
		.name           = "k3-fakefb",
		.owner          = THIS_MODULE,
	},
};

static int __init hifb_init(void)/*lint !e31*/
{
	int rc;

	rc = platform_device_register(&k3framebuffer_device);
	if(rc)  
		printk(KERN_ERR "k3 fake framebuffer init failed\n");
	else
		printk(KERN_ERR "k3 fake framebuffer init ok\n");

	rc = platform_driver_register(&k3framebuffer_driver);
	
	return rc;
}

/******************************************************************************
 Function        : hifb_cleanup
 Description     : cleanup the resource when exiting the framebuffer
                   module
 Data Accessed   :
 Data Updated    :
 Output          : None
 Input           : void
 Return          : static
 Others          : 0
******************************************************************************/


static void __exit hifb_exit(void)
{
    platform_driver_unregister(&k3framebuffer_driver);
    platform_device_unregister(&k3framebuffer_device);
}

module_init(hifb_init);
