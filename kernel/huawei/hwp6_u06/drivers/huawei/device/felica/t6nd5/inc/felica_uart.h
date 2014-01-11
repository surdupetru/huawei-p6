/**
 * @file felica_uart.h
 * @brief FeliCa driver header file
 *
 * @date 2012/03/16
 *
 *
 * Copyright(C) 2012 NTT DATA MSE CORPORATION. All right reserved.
 */

#ifndef __DV_FELICA_UART_LOCAL_H__
#define __DV_FELICA_UART_LOCAL_H__

/* ========================================================================== */
/* define                                                                     */
/* ========================================================================== */
#define FELICA_UART_MINOR_NO_UART	(0)
#define FELICA_DEV_NUM				(1)

#define TTY_FILE_PATH				"/dev/ttyAMA3"
#define UART_TTY_READBUFF_SIZE		4095
#define UART_TTY_WRITEBUFF_SIZE		260

#endif /* __DV_FELICA_UART_LOCAL_H__ */
