/*
 * linux/arch/arm/mach-k3v2/k3v2_wifi_power.h
 *
 * Copyright (C) 2010 Texas Instruments Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _K3V2_WIFI_POWER_H
#define _K3V2_WIFI_POWER_H

#include <mach/gpio.h>

#define PREALLOC_WLAN_NUMBER_OF_SECTIONS (4)
#define PREALLOC_WLAN_NUMBER_OF_BUFFERS  (160)
#define PREALLOC_WLAN_SECTION_HEADER     (24)
#define WLAN_SKB_BUF_MIN                 (4096)
#define WLAN_SKB_BUF_MAX                 (8192)
#define WLAN_SKB_BUF_NUM                 (16)
#define WLAN_SECTION_SIZE_0     (PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_1     (PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_2     (PREALLOC_WLAN_NUMBER_OF_BUFFERS * 512)
#define WLAN_SECTION_SIZE_3     (PREALLOC_WLAN_NUMBER_OF_BUFFERS * 1024)
#define WLAN_MAC_ADDR_MAXLENGTH 6
#define WLAN_MAC_NV_NUMBER      193
#define WLAN_MAC_NV_NAME        "MACWLAN"
#define WLAN_MAC_NV_VALID_SIZE  20

#if defined(CONFIG_MACH_K3V2OEM1)
#define K3V2_WIFI_IRQ_GPIO      GPIO_0_4
#define K3V2_WIFI_POWER_GPIO    GPIO_0_2
#define K3V2_WIFI_VDD_VOLTAGE   1800000
#else
#define K3V2_WIFI_IRQ_GPIO      GPIO_20_0
#define K3V2_WIFI_POWER_GPIO    GPIO_20_1
#define K3V2_WIFI_VDD_GPIO      GPIO_21_6
#endif

extern int hi_sdio_detectcard_to_core(int val);
extern void hi_sdio_set_power(int val);

#endif /*_K3V2_WIFI_POWER_H*/
