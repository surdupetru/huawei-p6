#ifndef _UART_OUTPUT_H
#define _UART_OUTPUT_H

/*
 * UART Output Control debugfs header
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2012 Wei Du <weidu.du@huawei.com>
 */

extern int  uart_output_debugfs_init(void);
extern bool if_uart_output_enabled(void);

#endif	/* !_UART_OUTPUT_H */
