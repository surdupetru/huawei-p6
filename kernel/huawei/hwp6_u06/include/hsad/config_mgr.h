

#ifndef CONFIG_MGR_H
#define CONFIG_MGR_H

#include <linux/types.h>

//#include "../../../arch/arm/mach-msm/include/mach/gpiomux.h"
/*
*get module struct pointer ,such as gpio,common
*/
extern struct board_id_general_struct * get_board_id_general_struct(char * module_name);


extern bool select_hw_config(unsigned int boot_id);
extern bool get_hw_config(const char* key, char* pbuf, size_t count, unsigned int *ptype);
extern bool get_hw_config_string(const char* key, char* pbuf, size_t count, unsigned int *ptype);
extern bool get_hw_config_int(const char* key, unsigned int* pbuf, unsigned int *ptype);
extern bool get_hw_config_bool(const char* key, bool* pbuf, unsigned int *ptype);
extern bool get_hw_config_enum(const char* key, char* pbuf, size_t count, unsigned int *ptype);

extern int g_current_board_id;
#endif
