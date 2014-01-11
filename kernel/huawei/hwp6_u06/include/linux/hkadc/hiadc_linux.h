/*******************************************************************************
 * Copyright:   Copyright (c) 2011. Hisilicon Technologies, CO., LTD.
 * Version:     V200
 * Filename:    hiadc_linux.h
 * Description: hiadc_linux.h
 * History:     1.Created by 00186176 on 2011/08/02
*******************************************************************************/

#ifndef __K3_ADC_H
#define __K3_ADC_H

#define ADC_ERR_MSG(printf_msg)		\	do {\		printk(KERN_ERR "%s,%d", __FILE__, __LINE__);\		printk printf_msg ;\	} 		while (0)
#ifdef DEBUG
#define ADC_INF_MSG(printf_msg)				printk printf_msg
#else
#define ADC_INF_MSG(printf_msg)
#endif

#define adc_dbg(_adc, msg...)				dev_dbg(&(_adc)->pdev->dev, msg)

#endif