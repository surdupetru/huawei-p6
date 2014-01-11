/**
    @copyright: Huawei Technologies Co., Ltd. 2012-2013. All rights reserved.
    
    @file: srecorder.h
    
    @brief: define the global resources for SRecorder
    
    @version: 1.0 
    
    @author: QiDechun ID: 216641
    
    @date: 2013-01-29
    
    @history:
*/

#ifndef SRECORDER_H
#define SRECORDER_H


/*----includes-----------------------------------------------------------------------*/


/*----c++ support--------------------------------------------------------------------*/


/*----export macroes-----------------------------------------------------------------*/

/*==================================================================*/
/*                                          define log dump switch begin                                                                          */
/*==================================================================*/
#ifdef CONFIG_DUMP_CRASH_TIME
#define DUMP_CRASH_TIME (1)
#else
#define DUMP_CRASH_TIME 1                   /* 导出死机的时间，这是SRecorder的基础抓包能力，该值必须为1 */
#endif

#ifdef CONFIG_DUMP_DMESG
#define DUMP_DMESG (1)
#else
#define DUMP_DMESG 1                             /* 导出dmesg信息获取，这是SRecorder的基础抓包能力，该值必须为1 */
#endif

#ifdef CONFIG_DUMP_CURRENT_PS_BACKTRACE
#define DUMP_CURRENT_PS_BACKTRACE (1)
#else
#define DUMP_CURRENT_PS_BACKTRACE 0 /* 导出当前正在运行进程调用栈，建议打开 */
#endif

#ifdef CONFIG_DUMP_ALLCPU_STACK
#define DUMP_ALLCPU_STACK (1)
#else
#define DUMP_ALLCPU_STACK 0                /* 导出所有CPU调用栈，可以关闭 */
#endif

#ifdef CONFIG_DUMP_ALLPS_INFO
#define DUMP_ALLPS_INFO (1)
#else
#define DUMP_ALLPS_INFO 0                     /* 导出所有进程信息，可以关闭 */
#endif

#ifdef CONFIG_DUMP_SYS_INFO
#define DUMP_SYS_INFO (1)
#else
#define DUMP_SYS_INFO 0                        /* 导出系统信息，可以关闭 */
#endif

#ifdef CONFIG_DUMP_SLAB_INFO
#define DUMP_SLAB_INFO (1)
#else
#define DUMP_SLAB_INFO 0                        /* 导出slab信息，可以关闭 */
#endif

#ifdef CONFIG_DUMP_MODEM_LOG
#define DUMP_MODEM_LOG (1)
#else
#ifdef CONFIG_ARCH_MSM
#define DUMP_MODEM_LOG 1                      /* 导出modem死机堆栈和运行日志，可以关闭 */
#else
#define DUMP_MODEM_LOG 0
#endif
#endif
/*==================================================================*/
/*                    define log dump switch end                                                                                                  */
/*==================================================================*/

#define CRASH_REASON_KEYWORD "===============crash reason time==============="
#define SYSINFO_KEYWORD "===============sys info==============="
#define DMESG_KEYWORD "===============dmesg==============="
#define ALL_CPU_CALL_STACK_KEYWORD "===============all cpu call stack==============="
#define ALL_PS_INFO_KEYWORD "===============all ps info==============="
#define CURRENT_PS_BACKTRACE_KEYWORD "===============current ps backtrace==============="
#define SLABINFO_KEYWORD "===============slabinfo==============="
#define MODEM_ERR_KEYWORD "===============modem err==============="
#define MODEM_ERR_F3_KEYWORD "===============modem err f3==============="

#ifndef GET_MIN
#define GET_MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif

#define SRECORDER_MAGIC_NUM (0x20122102)
#define INVALID_SRECORDER_MAGIC_NUM (0xffffffff)
#define ABNORMAL_RESET (1)
#define NORMAL_RESET (0)
#define CRC32_SEED_VALUE (0xFFFFFFFF)

#define KERNEL_TEXT_OFFSET (0x8000)
#define INITIAL_PAGE_TABLE_SIZE (0x4000)

#ifndef CONFIG_SRECORDER_LOG_BUF_LEN
#define CONFIG_SRECORDER_LOG_BUF_LEN 0x40000
#endif

#define SRECORDER_TIMER_EXPIRE_PERIOD (5) 


/*----export prototypes---------------------------------------------------------------*/

/* log type */
typedef enum
{
    CRASH_REASON_TIME = 0, 
    SYS_INFO,
    DMESG,
    ALL_CPU_STACK,    
    ALL_PS_INFO,    
    CURRENT_PS_BACKTRACE,
    SLABINFO,
    MODEM_ERR,
    MODEM_ERR_F3,
    TYPE_MAX,
} srecorder_info_type_e;

/* header struct for SRecorder's reserved memory */
typedef struct __srecorder_reserved_mem_header_t
{
    unsigned long version; /* the version of the SRecorder */
    unsigned long magic_num; /* the default magic number is 0x20122102 */
    unsigned long data_length; /* log length except this header */

    /*===========================================================*/
    /*       allocate 6 members for abnormal reset begin         */
    /*===========================================================*/
    unsigned long reset_flag; /* system reset_flag, 1 - abnormal reset, 0 - normal reset */
    unsigned long log_buf; /* phys addr of __log_buf */
    unsigned long log_end; /* phys addr of log_end */
    unsigned long log_buf_len; /* phys addr of log_buf_len */
    unsigned long reserved_mem_size; /* SRecorder reserved mem max length */
    unsigned long crc32; /* crc32 check value */
    /*===========================================================*/
    /*       allocate 6 members for abnormal reset end           */
    /*===========================================================*/
    
    char reserved[24];
    
    /*the region following this struct is assigned for the data region*/
} srecorder_reserved_mem_header_t, *psrecorder_reserved_mem_header_t;

/* define the header struct for log */
typedef struct __info_header
{
    srecorder_info_type_e type;
    unsigned long crc32;
    unsigned long data_len;
} srecorder_info_header_t, *psrecorder_info_header_t;

typedef struct
{
    unsigned long srecorder_log_buf;
    unsigned long srecorder_log_buf_len;
#ifdef CONFIG_POWERCOLLAPSE
    unsigned long log_buf;
    unsigned long log_buf_len;
#endif
    unsigned long crc32;
} platform_special_reserved_mem_info_t;


/*----global variables-----------------------------------------------------------------*/


/*----export function prototypes--------------------------------------------------------*/

/**
    @function: void get_log_buf_info(unsigned long *plog_buf, unsigned long*plog_end, unsigned long*plog_buf_len)
    @brief: get kernel log ring buffer's start address,write index and length.
    
    @param: plog_buf save ring buffer's start address
    @param: plog_end save ring buffer's write index
    @param: plog_buf_len save ring buffer's length
    
    @return: none

    @note: 
*/
void get_log_buf_info(unsigned long *plog_buf, 
    unsigned long *plog_end, 
    unsigned long *plog_buf_len);


/**
    @function: void get_srecorder_log_buf_info(unsigned long *psrecorder_log_buf, unsigned long*psrecorder_log_buf_len)
    @brief: get the start address and length of SRecorder's reserved memory.

    @param: psrecorder_log_buf Save start address of SRecorder's reserved memory
    @param: psrecorder_log_buf_len Save length of SRecorder's reserved memory
    
    @return: none

    @note: 
*/
void get_srecorder_log_buf_info(
    unsigned long *psrecorder_log_buf, 
    unsigned long *psrecorder_log_buf_len);


/**
    @function: char* get_srecorder_temp_buf_addr(void)
    @brief: get address of the temp buffer saving the log captured before reboot.

    @param: none
    
    @return: none

    @note: 
*/
char* get_srecorder_temp_buf_addr(void);


/**
    @function: unsigned long srecorder_get_crc32(unsigned char const *pbuf, unsigned long data_len)
    @brief: do crc32 check

    @param: pbuf data tobe checked
    @param: data_len data length
    
    @return: crc32 value

    @note: 
*/
unsigned long srecorder_get_crc32(unsigned char const *pbuf, unsigned long data_len);


#ifdef CONFIG_SRECORDER_DUMP_LOG_TO_STORAGECARD
/**
    @function: char *alloc_buf_for_srecorder(unsigned long buf_len)
    @brief: allocate memory for SRecorder by vmalloc

    @param: size memory size of the buffer to be allocated
    
    @return: the start address of the buffer allocated

    @note: 
*/
char *alloc_buf_for_srecorder(unsigned long size);


/**
    @function: bool get_srecorder_log_buf(char *panic_reason, char **pbuf, unsigned long *plog_len)
    @brief: get SRecorder's valid log start addres and its length

    @param: panic_reason crash reason.
    @param: pbuf valid log buffer's start address.
    @param: plog_len save valid log length
    
    @return: true - successfully, false - failed

    @note: 
*/
bool get_srecorder_log_buf(char *panic_reason, char **pbuf, unsigned long *plog_len);
#endif


#endif /* SRECORDER_H */

