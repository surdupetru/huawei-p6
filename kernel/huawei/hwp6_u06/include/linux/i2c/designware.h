/*
 * designware.h - designware I2C driver header file
 *
 * Copyright (c) 2011
 */

#ifndef __DESIGNWARE_H_
#define __DESIGNWARE_H_

#define ENABLE_DELAY_SDA		1
#define DISABLE_DELAY_SDA		0

struct dma_chan;
struct i2c_dw_data {
	bool (*dma_filter)(struct dma_chan *chan, void *filter_param);
	void *dma_rx_param;
	void *dma_tx_param;
#ifdef CONFIG_ARCH_K3V2
	void (*init) (void);
	void (*exit) (void);
	void (*reset) (void);
	void (*delay_sda) (int enable);
	void (*reset_controller) (void);
#endif
};

#endif /* __DESIGNWARE_H_ */
