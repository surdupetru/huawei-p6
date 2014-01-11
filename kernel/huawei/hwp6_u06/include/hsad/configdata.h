

#ifndef _CONFIG_DATA_H
#define _CONFIG_DATA_H


#include <linux/kernel.h>

#define BOARD_ID_MAX 0x7

#define DISABLE 	0x00
#define ENABLE  	0xFF

#define ISDEBUG  	(DISABLE)

#if (ISDEBUG == ENABLE)
#define HW_CONFIG_DEBUG(fmt, arg...) printk("[HW_CONFIG]: fun= %s: line=%d: " fmt, __FUNCTION__, __LINE__, ##arg)
#else
#define  HW_CONFIG_DEBUG(fmt, arg...)
#endif


typedef enum _config_data_type {
    E_CONFIG_DATA_TYPE_INT = 0,
    E_CONFIG_DATA_TYPE_ENUM,
    E_CONFIG_DATA_TYPE_BOOL,
    E_CONFIG_DATA_TYPE_STRING,
    E_CONFIG_DATA_MAX
} config_data_type;

typedef struct _config_pair {
    const char* key;
    const unsigned int data; /* data:*/
    const config_data_type type; /* type*/
} config_pair;

char * strncpy(char *,const char *, __kernel_size_t);

#endif
