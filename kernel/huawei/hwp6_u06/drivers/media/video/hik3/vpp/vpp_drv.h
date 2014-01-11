
#ifndef __VPP_DRV_H_
#define __VPP_DRV_H_

#include <linux/ioctl.h>   

#include "vpp_inter.h"
#include "vpp_hal.h"
#include "vpp_drv_ext.h"

/* Use 'k' as magic number */
#define VPP_IOC_MAGIC  'k'

#define VPP_STARTCONFIG            _IOWR(VPP_IOC_MAGIC, 1, VPP_CONFIG_S)
#define VPP_IOC_MAXNR 2

#endif

