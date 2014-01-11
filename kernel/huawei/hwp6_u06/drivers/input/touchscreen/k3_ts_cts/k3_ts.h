#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/usb.h>
#include <asm/io.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>

#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <asm/irq.h>


#ifndef __K3_TS_SPI_H__
#define __K3_TS_SPI_H__
/*
typedef enum spistatus
{
    SSP_TXEMPTY = 0,
        SSP_TXNOTFULL,
        SSP_RXNOTEMPTY,
        SSP_RXFULL,
        SSP_BUSY
}spi_status;

*/

 /*#define SYSREG_BASE      0x20040000

 #define SYS_PER_CTRL0      0x1c
 #define SYS_PER_CTRL1      0x20
 #define SYS_PER_CTRL2      0x4c
 #define SYS_PER_CTRL5      0x58
 #define SYS_PER_DIS        0x28
 #define SYS_PER_EN         0x24
 #define SYS_PER_RST2       0x48*/

void ts_i2c_init(void);


#endif
#ifndef __K3_TS_H__
#define __K3_TS_H__

#define K3_TS_DRV_NAME "hisik3_touchscreen"


#define K3_TS_TRACE_LEVEL 10

#define hisik3_ts_trace(level, msg...) do { \
        if ((level) >= K3_TS_TRACE_LEVEL) { \
                printk("hisik3_ts_trace:%s:%d: ", __FUNCTION__, __LINE__); \
                printk(msg); \
                printk("\n"); \
        } \
} while (0)

struct hisik3_ts_dev {
    struct input_dev *input;
//    struct timer_list timer;
    int irq;
    unsigned addr;
    char * name;
#ifdef CONFIG_ANDROID_POWER
    struct android_early_suspend early_suspend;
#endif
};

typedef struct hisik3_ts_info_tag
{
    int x;
    int y;
    int num;
}hisik3_ts_info, *phisik3_ts_info;

#define REG_HKADC_SETUP     0x59
#define REG_HKADC_CONV_START    0x5a
#define REG_HKADC_CONFIG    0x5b
#define REG_HKADC_STATUS    0x5c
#define REG_HKADC_DATAX1    0x5d
#define REG_HKADC_DATAX2    0x5e
#define REG_HKADC_DATAY1    0x5f
#define REG_HKADC_DATAY2    0x60

#if 0
#define REG_IOMG030         0x20041078
#define REG_IOCG098         0x20041988
#endif

#endif

