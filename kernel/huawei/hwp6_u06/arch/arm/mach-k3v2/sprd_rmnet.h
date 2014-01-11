
#ifndef __NIHAO_H__
#define __NIHAO_H__

#include <linux/sched.h>

#define MAX_SMD_NET 3

typedef enum _HSI_CH_USER_
{
	XMD_TTY,
	XMD_NET,
} XMD_CH_USER;

struct xmd_ch_info {
	int id;
	char name[32];
	int chno;
	XMD_CH_USER user;
	void *priv;
	int open_count;
	spinlock_t lock;
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[1]))
#endif
enum {
//Common level segment
	DYNADBG_EMERG         = 1U << 0,	//pr_emerg, default Y;
	DYNADBG_ALERT         = 1U << 1,	//pr_alert, default Y;
	DYNADBG_CRIT          = 1U << 2,	//pr_crit, default N;
	DYNADBG_ERR           = 1U << 3,	//pr_err, default Y;
	DYNADBG_WARN          = 1U << 4,	//pr_warn, default N;
	DYNADBG_NOTICE        = 1U << 5,	//pr_notice, default Y;
	DYNADBG_INFO          = 1U << 6,	//pr_info, default N;
	DYNADBG_DEBUG         = 1U << 7,	//pr_debug, default N;

//Special level segment
	DYNADBG_OPEN_CLOSE    = 1U << 8,	//default Y;
	DYNADBG_READ          = 1U << 9,	//default Y;
	DYNADBG_WRITE         = 1U << 10,	//default Y;
	DYNADBG_INIT_EXIT     = 1U << 11,	//default Y;
	DYNADBG_RX            = 1U << 12,	//default Y;
	DYNADBG_TX            = 1U << 13,	//default Y;
	DYNADBG_EVENT         = 1U << 14,	//default Y;
	DYNADBG_THREADS       = 1U << 15,	//default Y;

//Module segment
	DYNADBG_XMD_CMM_EN   = 1U << 20, //default Y;
	DYNADBG_XMD_BOOT_EN  = 1U << 21, //default Y;
	DYNADBG_XMD_TTY_EN   = 1U << 22, //default Y;
	DYNADBG_XMD_NET_EN   = 1U <<23, //default Y;
	DYNADBG_HSI_MCM_EN   = 1U << 24, //default Y;
	DYNADBG_HSI_LL_EN    = 1U << 25, //default Y;
	DYNADBG_HSI_IF_EN    = 1U << 26, //default Y;
	DYNADBG_HSI_PHL_EN   = 1U << 27, //default Y;

//Private segment, unrestricted to DYNADBG_GLOBAL_EN;
	DYNADBG_VERIFY_EN     = 1U << 29,   //default N;
	DYNADBG_PROBE_EN      = 1U << 30,   //default N;
//Global segment
	DYNADBG_GLOBAL_EN     = 1U << 31, //default Y;
};
//XMD_CMM, XMD_BOOT, XMD_TTY, XMD_NET, HSI_MCM, HSI_LL, HSI_IF, HSI_PHL;

extern uint32_t dynamic_debug_mask;
#define dynamic_debug(mask, x...) \
    do { \
        if( ( (dynamic_debug_mask)&((mask)|(DYNADBG_GLOBAL_EN)) ) == ((mask)|(DYNADBG_GLOBAL_EN)) ) \
            printk(KERN_INFO x); \
    } while (0)

#define dynamic_verify(mask, x...) \
    do { \
        if( ( (dynamic_debug_mask)&((mask)|(DYNADBG_VERIFY_EN)) ) == ((mask)|(DYNADBG_VERIFY_EN)) ) \
            printk(KERN_INFO x); \
    } while (0)



#endif /* __NIHAO_H__ */

