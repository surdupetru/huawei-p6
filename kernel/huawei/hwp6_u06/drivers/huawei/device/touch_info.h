#ifndef _TOUCH_INFO_H
#define _TOUCH_INFO_H

#define TOUCH_INFO_RMI3250 "synaptics_3250"
#define TOUCH_INFO_RMI7020 "synaptics_7020"
#define TOUCH_INFO_CYPRESS "cypress_CY8CTMA463"

int set_touch_chip_info(const char *chip_info);

#endif