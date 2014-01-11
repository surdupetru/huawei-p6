/*
 *  nct203.h
 *
 *
 */
#ifndef _NCT203_H
#define _NCT203_H
#include <linux/types.h>

struct nct203_platform_data {
    bool    ext_range;
    u8      alert_limit;
    u8      throttling_limit;
    u8      hysteresis;
    u8      supported_hwrev;
    u8      conv_rate;
    void    (*alarm_fn)(bool raised);
    int     debug_temp;
};

#endif				/* _NCT203_H */
