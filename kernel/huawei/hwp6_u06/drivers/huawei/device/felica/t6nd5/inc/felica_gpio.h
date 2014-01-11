/*
 * @file felica_gpio.h
 * @brief Local header file of FeliCa GPIO driver
 *
 * @date 2012/03/16
 *
 *
 * Copyright(C) 2012 NTT DATA MSE CORPORATION. All right reserved.
 */


#ifndef __DV_FELICA_GPIO_LOCAL_H__
#define __DV_FELICA_GPIO_LOCAL_H__

/*
 * include extra header
 */
#include <linux/sched.h>					/* jiffies    */
#include <mach/gpio.h>

#define FELICA_GPIO_BOLCK_NAME "block_felica"
/*
 * The FeliCa terminal condition to acquire and to set.
 */
#define FELICA_OUT_L				(0x00)	/* condition L */
#define FELICA_OUT_H				(0x01)	/* condition H */
#define FELICA_OUT_RFS_L			(0x01)	/* RFS condition L */
#define FELICA_OUT_RFS_H			(0x00)	/* RFS condition H */

#define FELICA_OUT_SIZE				(1)		/* size of to acquire and to set */

/*
 * The Felica interrupt condition to acquire.
 */
#define FELICA_EDGE_L				(0x00)	/* changed condition from H to L */
#define FELICA_EDGE_H				(0x01)	/* changed condition from L to H */
#define FELICA_EDGE_NON				(0xFF)	/* non changed */
#define FELICA_EDGE_OUT_SIZE (2)			/* size of to acquire and to set for EDGE */

/*
 * Identification information of FeliCa device drivers
 */
#define FELICA_DEV_NAME				"felica"			/* Device Name        */
#define FELICA_DEV_NAME_CTRL_PON	"felica_pon"		/* PON Terminal Ctrl  */
#define FELICA_DEV_NAME_CTRL_CEN	"felica_cen"		/* CEN Terminal Ctrl  */
#define FELICA_DEV_NAME_CTRL_RFS	"felica_rfs"		/* RFS Terminal Ctrl  */
#define FELICA_DEV_NAME_CTRL_ITR	"felica_int"		/* ITR Terminal Ctrl  */
#define FELICA_DEV_NAME_CTRL_DEBUG	"felica_ctrl_dbg"	/* Ctrl debug         */
#define FELICA_DEV_NAME_UART		"felica"			/* Commnunication     */
#define FELICA_DEV_NAME_UART_DEBUG	"felica_comm_dbg"	/* Comm deubg         */
#define FELICA_DEV_NAME_RWS			"felica_rws"		/* RWS                */
#define FELICA_DEV_NAME_CFG			"felica_cfg"		/* CFG                */

#define FELICA_DEV_NUM				(1)
#define FELICA_DEV_ITR_MAX_OPEN_NUM	(1)

/*
 * Minor number
 */
#define FELICA_CTRL_MINOR_NO_PON	(0)					/* PON Terminal Ctrl  */
#define FELICA_CTRL_MINOR_NO_CEN	(1)					/* CEN Terminal Ctrl  */
#define FELICA_CTRL_MINOR_NO_RFS	(2)					/* RFS Terminal Ctrl  */
#define FELICA_CTRL_MINOR_NO_ITR	(3)					/* ITR Terminal Ctrl  */
#define FELICA_CTRL_MINOR_NO_DEBUG	(4)					/* Ctrl debug         */
#define FELICA_UART_MINOR_NO_UART	(0)					/* Commnunication     */
#define FELICA_RWS_MINOR_NO_RWS		(0)					/* RWS                */

/*
 * Interrupts device name
 */
#define FELICA_CTRL_ITR_STR_RFS		"felica_ctrl_rfs"	/* RFS interrupt */
#define FELICA_CTRL_ITR_STR_INT		"felica_ctrl_int"	/* INT interrupt */

/*
 * GPIO paramater
 */
#define FELICA_GPIO_PORT_PON			(GPIO_0_6)								/* correspondence for hsv */
#define FELICA_GPIO_PORT_RFS			(GPIO_16_6)								/* correspondence for hsv */
#define FELICA_GPIO_PORT_INT			(GPIO_17_4)								/* correspondence for hsv */
#define FELICA_GPIO_STR_RFS				"felica_rfs"				/* RFS labe name */
#define FELICA_GPIO_STR_INT				"felica_int"				/* INT labe name */
#define FELICA_GPIO_STR_PON				"felica_pon"				/* PON labe name */
#define FELICA_GPIO_STR_HVDD        "felica_hvdd"
/*
 * Timer
 */
#define FELICA_TIMER_CEN_TERMINAL_WAIT	((20 * HZ) / 1000 + 1)
#define FELICA_TIMER_PON_TERMINAL_WAIT	((30 * HZ) / 1000 + 1)

/*
 * The FeliCa terminal condition to acquire and to set for Internal.
 */
#define DFD_OUT_L	(0x00)								/* condition L */
#define DFD_OUT_H	(0x01)								/* condition H */

/*
 * Interrupt kinds
 */
enum{
	DFD_ITR_TYPE_RFS = 0,								/* RFS Interrupt      */
	DFD_ITR_TYPE_INT,									/* INT Interrupt      */
	DFD_ITR_TYPE_NUM									/* Interrupt type num */
};

/*
 * Cleanup parameters
 */
enum{
	DFD_CLUP_NONE = 0,
	DFD_CLUP_UNREG_CDEV_PON,
	DFD_CLUP_CDEV_DEL_PON,
	DFD_CLUP_UNREG_CDEV_CEN,
	DFD_CLUP_CDEV_DEL_CEN,
	DFD_CLUP_UNREG_CDEV_RFS,
	DFD_CLUP_CDEV_DEL_RFS,
	DFD_CLUP_UNREG_CDEV_ITR,
	DFD_CLUP_CDEV_DEL_ITR,
	DFD_CLUP_CLASS_DESTORY,
	DFD_CLUP_DEV_DESTORY_PON,
	DFD_CLUP_DEV_DESTORY_CEN,
	DFD_CLUP_DEV_DESTORY_RFS,
	DFD_CLUP_DEV_DESTORY_ITR,
	DFD_CLUP_DEV_DESTORY_UART,
	DFD_CLUP_DEV_DESTORY_RWS,
	DFD_CLUP_DEV_DESTORY_CFG,
	DFD_CLUP_ALL
};

/*
 * Wait Timer
 */
struct FELICA_TIMER {
	struct timer_list Timer;
	wait_queue_head_t wait;
};

/*
 * Device control data structure(PON terminal)
 */
struct FELICA_CTRLDEV_PON_INFO {
	/* Timeout control information */
	struct FELICA_TIMER PON_Timer;				/* PON terminal control timer */
};

/*
 * Device control data structure(CEN terminal)
 */
struct FELICA_CTRLDEV_CEN_INFO {
	/* Timeout control information */
	struct FELICA_TIMER PON_Timer;				/* PON terminal control timer */
	struct FELICA_TIMER CEN_Timer;				/* CEN terminal control timer */
};

/*
 * Device control data structure(ITR terminal)
 */
struct FELICA_CTRLDEV_ITR_INFO {
	/* ITR result information */
	unsigned char INT_Flag;						/* INT interrupts */
	unsigned char RFS_Flag;						/* RFS interrupts */
	unsigned char reserve1[2];					/* reserved       */

	/* wait queue */
	wait_queue_head_t RcvWaitQ;
};


/*
 * Device control data structure
 */
struct FELICA_DEV_INFO {
	unsigned int w_cnt;							/* write open counter  */
	unsigned int r_cnt;							/* read open counter   */
	void * Info;								/* interal saving data */
};


struct FELICA_ITR_INFO {
	unsigned int irq;
	irq_handler_t handler;
	unsigned long flags;
	const char *name;
	void *dev;
};

#endif /* __DV_FELICA_GPIO_LOCAL_H__ */
