/* linux/drivers/dma/k3dma.c
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/clk.h>
#include <linux/dmapool.h>
#include <linux/delay.h>
#include "k3dma.h"

static unsigned int init_desc_len = (sizeof(struct k3dma_desc) /K3_DMA_ALIGN + 1)  * K3_DMA_ALIGN;

/* ---------------------------declare funcs------------------------- */
static dma_cookie_t k3dma_tx_submit(struct dma_async_tx_descriptor *tx);
static inline struct k3dma_device *to_k3dma(struct dma_device *ddev);
static void k3dma_unmap_buffers(struct k3dma_desc *desc);
static struct device *chan2dev(struct dma_chan *chan);

static void k3dma_dumpregs(struct k3dma_device *k3dma, int chan_id)
{
		struct k3dma_phy_chan *ch = &k3dma->phy_chans[chan_id];

		printk(KERN_INFO DEVICE_NAME ": ============== REGISTER DUMP ==============\n");
		printk(KERN_INFO DEVICE_NAME ": INT_STAT_OFFSET:            0x%x\n",
			dma_reg_read(k3dma->regs + INT_STAT_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": INT_TC1_OFFSET:             0x%x\n",
			dma_reg_read(k3dma->regs + INT_TC1_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": INT_ERR1_OFFSET:            0x%x\n",
			dma_reg_read(k3dma->regs + INT_ERR1_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": INT_ERR2_OFFSET:            0x%x\n",
			dma_reg_read(k3dma->regs + INT_ERR2_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": INT_TC1_RAW_OFFSET:         0x%x\n",
			dma_reg_read(k3dma->regs + INT_TC1_RAW_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": INT_ERR1_RAW_OFFSET:        0x%x\n",
			dma_reg_read(k3dma->regs + INT_ERR1_RAW_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": INT_ERR2_RAW_OFFSET:        0x%x\n",
			dma_reg_read(k3dma->regs + INT_ERR2_RAW_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": CX_CURR_CNT1_OFFSET:        0x%x\n",
			dma_reg_read(k3dma->regs + CX_CURR_CNT1_OFFSET(chan_id)));
		printk(KERN_INFO DEVICE_NAME ": CX_CURR_CNT0_OFFSET:        0x%x\n",
			dma_reg_read(k3dma->regs + CX_CURR_CNT0_OFFSET(chan_id)));
		printk(KERN_INFO DEVICE_NAME ": CX_CURR_SRC_ADDR_OFFSET:    0x%x\n",
			dma_reg_read(k3dma->regs + CX_CURR_SRC_ADDR_OFFSET(chan_id)));
		printk(KERN_INFO DEVICE_NAME ": CX_CURR_DES_ADDR_OFFSET:    0x%x\n",
			dma_reg_read(k3dma->regs + CX_CURR_DES_ADDR_OFFSET(chan_id)));
		printk(KERN_INFO DEVICE_NAME ": CX_LLI_OFFSET:              0x%x\n",
			dma_reg_read(ch->ch_regs + CX_LLI_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": CX_SRC_ADDR_OFFSET:         0x%x\n",
			dma_reg_read(ch->ch_regs + CX_SRC_ADDR_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": CX_DES_ADDR_OFFSET:         0x%x\n",
			dma_reg_read(ch->ch_regs + CX_DES_ADDR_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": CX_BINDX_OFFSET:            0x%x\n",
			dma_reg_read(ch->ch_regs + CX_BINDX_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": CX_CINDX_OFFSET:            0x%x\n",
			dma_reg_read(ch->ch_regs + CX_CINDX_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": CX_CNT1_OFFSET:             0x%x\n",
			dma_reg_read(ch->ch_regs + CX_CNT1_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": CX_CNT0_OFFSET:             0x%x\n",
			dma_reg_read(ch->ch_regs + CX_CNT0_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": CX_CONFIG_OFFSET:           0x%x\n",
			dma_reg_read(ch->ch_regs + CX_CONFIG_OFFSET));
		printk(KERN_INFO DEVICE_NAME ": CX_AXI_CONF_OFFSET:         0x%x\n",
			dma_reg_read(ch->ch_regs + CX_AXI_CONF_OFFSET));
}
/*---------------------dma_ops:dma regs operation--------------------*/
static int k3dma_phy_channel_busy(struct k3dma_phy_chan *ch)
{
	u32 val = dma_reg_read(ch->ch_regs + CX_CONFIG_OFFSET) & 0x1;
	return val;
}

static void k3dma_setup_irq(struct k3dma_device *k3dma, int on)
{
	if (on) {
		dma_reg_write(k3dma->regs + INT_TC1_MASK_OFFSET,    0xFFFF);
		dma_reg_write(k3dma->regs + INT_ERR1_MASK_OFFSET,   0xFFFF);
		dma_reg_write(k3dma->regs + INT_ERR2_MASK_OFFSET,   0xFFFF);
	} else {
		dma_reg_write(k3dma->regs + INT_TC1_MASK_OFFSET,    0x0);
		dma_reg_write(k3dma->regs + INT_TC2_MASK_OFFSET,    0x0);
		dma_reg_write(k3dma->regs + INT_ERR1_MASK_OFFSET,   0x0);
		dma_reg_write(k3dma->regs + INT_ERR2_MASK_OFFSET,   0x0);
		dma_reg_write(k3dma->regs + INT_ERR3_MASK_OFFSET,   0x0);
	}
}

static void k3dma_clear_all_irq(struct k3dma_device *k3dma)
{
	dma_reg_write(k3dma->regs + INT_TC1_RAW_OFFSET,		0xFFFF);
	dma_reg_write(k3dma->regs + INT_TC2_RAW_OFFSET,		0xFFFF);
	dma_reg_write(k3dma->regs + INT_ERR1_RAW_OFFSET,	0xFFFF);
	dma_reg_write(k3dma->regs + INT_ERR2_RAW_OFFSET,	0xFFFF);
	dma_reg_write(k3dma->regs + INT_ERR3_RAW_OFFSET,	0xFFFF);
}

static void k3dma_interrupt_off(struct k3dma_device *k3dma)
{
	k3dma_setup_irq(k3dma, 0);
	k3dma_clear_all_irq(k3dma);
}

static void k3dma_interrupt_on(struct k3dma_device *k3dma)
{
	k3dma_setup_irq(k3dma, 1);
}

/*-------------------private interface----------------------------*/
static inline struct k3dma_desc *
txd_to_k3dma_desc(struct dma_async_tx_descriptor *txd)
{
	return container_of(txd, struct k3dma_desc, txd);
}

static void set_desc_eol(struct k3dma_desc *desc)
{
	desc->lli.lli = 0;
}

static inline struct k3dma_chan *to_k3dma_chan(struct dma_chan *dchan)
{
	return container_of(dchan, struct k3dma_chan, chan_common);
}

static inline struct k3dma_device *to_k3dma(struct dma_device *ddev)
{
	return container_of(ddev, struct k3dma_device, dma_common);
}

static struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}

static void k3dma_init_descriptor(struct dma_chan *chan,
					struct k3dma_desc *desc)
{
	INIT_LIST_HEAD(&desc->tx_list);
	dma_async_tx_descriptor_init(&desc->txd, chan);
	/* txd.flags will be overwritten in prep functions */
	desc->txd.flags = DMA_CTRL_ACK;
	desc->txd.tx_submit = k3dma_tx_submit;
	desc->txd.phys = virt_to_dma(NULL, &desc->lli);
}

/**
 * k3dma_desc_put - move a descriptor, including any children, to the free list
 * @k3chan: channel we work on
 * @desc: descriptor, at the head of a chain, to move to free list
 */
static void k3dma_desc_put(struct k3dma_chan *k3chan, struct k3dma_desc *desc)
{
	unsigned long flags;
	if (desc) {
		spin_lock_irqsave(&k3chan->lock, flags);
		list_splice_init(&desc->tx_list, &k3chan->free_list);
		list_add(&desc->desc_node, &k3chan->free_list);
		spin_unlock_irqrestore(&k3chan->lock, flags);
	}
}

/**
 * k3dma_desc_get - get an unused descriptor from free_list
 * @k3chan: channel we want a new descriptor for
 */
static struct k3dma_desc *k3dma_desc_get(struct k3dma_chan *k3chan)
{
	struct k3dma_desc *desc, *_desc;
	struct k3dma_desc *ret = NULL;
	unsigned int i = 0;
	unsigned long flags;
	LIST_HEAD(tmp_list);

	spin_lock_irqsave(&k3chan->lock, flags);
	list_for_each_entry_safe(desc, _desc, &k3chan->free_list, desc_node) {
		i++;
		if (async_tx_test_ack(&desc->txd)) {
			list_del(&desc->desc_node);
			ret = desc;
			break;
		}
		dev_dbg(chan2dev(&k3chan->chan_common),
				"desc %p not ACKed\n", desc);
	}
	spin_unlock_irqrestore(&k3chan->lock, flags);
	dev_dbg(chan2dev(&k3chan->chan_common),
		"scanned %u descriptors on freelist\n", i);

	if (!ret) {
		unsigned int init_nr_desc_per_channel = PAGE_SIZE / init_desc_len;
		struct dma_chan *chan = &k3chan->chan_common;
		struct k3dma_desc_page* k3_desc_pg = (struct k3dma_desc_page*)kmalloc(sizeof(struct k3dma_desc_page),GFP_ATOMIC);
		if (!k3_desc_pg) {
			dev_err(chan2dev(&k3chan->chan_common),
				"no memery for desc\n");
			return ret;
		}
		memset(k3_desc_pg, 0, sizeof(struct k3dma_desc_page));

		k3_desc_pg->page_link = (unsigned char *) __get_free_page(GFP_ATOMIC);
		if (!k3_desc_pg->page_link) {
			dev_err(chan2dev(&k3chan->chan_common),
				"no memery for page link\n");
			kfree(k3_desc_pg);
			return ret;
		}
		memset(k3_desc_pg->page_link, 0, PAGE_SIZE);
		for (i = 0; i < init_nr_desc_per_channel; i++) {
			desc = (struct k3dma_desc*)((unsigned char*)k3_desc_pg->page_link + init_desc_len * i);
			k3dma_init_descriptor(chan, desc);
			list_add_tail(&desc->desc_node, &tmp_list);
		}
		spin_lock_irqsave(&k3chan->lock, flags);
		k3chan->descs_allocated += i;
		list_splice(&tmp_list, &k3chan->free_list);
		ret = desc;
		list_del(&desc->desc_node);
		list_add(&k3_desc_pg->pg_node, &k3chan->page_list);
		spin_unlock_irqrestore(&k3chan->lock, flags);
	}

	return ret;
}

static struct k3dma_phy_chan *
k3dma_find_phy_channel(struct k3dma_device *k3dma,
			  struct k3dma_chan *k3chan)
{
	struct k3dma_phy_chan *ch = NULL;
	unsigned long flags;
	int i;

	for (i = 0; i < k3dma->pd->nr_phy_channels; i++) {
		ch = &k3dma->phy_chans[i];

		spin_lock_irqsave(&ch->lock, flags);
		if (!ch->serving) {
			ch->serving = k3chan;
			spin_unlock_irqrestore(&ch->lock, flags);
			break;
		}
		spin_unlock_irqrestore(&ch->lock, flags);

	}

	if (i == k3dma->pd->nr_phy_channels) {
		/* No physical channel available */
		return NULL;
	}

	return ch;
}

static int k3dma_get_phy_channel(struct k3dma_chan *k3chan)
{
	struct k3dma_device *k3dma = to_k3dma(k3chan->chan_common.device);
	struct k3dma_phy_chan *ch;
	unsigned long flags;

	/* Check if we already have a channel */
	spin_lock_irqsave(&k3chan->lock, flags);
	if (k3chan->phychan) {
		k3chan->phychan_hold++;
		spin_unlock_irqrestore(&k3chan->lock, flags);
		return 0;
	}

	ch = k3dma_find_phy_channel(k3dma, k3chan);
	if (!ch) {
		/* No physical channel available*/
		dev_warn(chan2dev(&k3chan->chan_common),
			"%s no phy channel avaiable\n", __func__);
		spin_unlock_irqrestore(&k3chan->lock, flags);
		return -EBUSY;
	}

	k3chan->phychan_hold++;
	k3chan->phychan = ch;
	spin_unlock_irqrestore(&k3chan->lock, flags);

	return 0;
}


static void k3dma_put_phy_channel(struct k3dma_chan *k3chan)
{
	struct k3dma_phy_chan *ch = k3chan->phychan;
	unsigned long flags;
	if (!list_empty(&k3chan->pend_list) || (k3chan->phychan_hold))
		return;

	if (ch) {
		spin_lock_irqsave(&ch->lock, flags);
		ch->serving = NULL;
		spin_unlock_irqrestore(&ch->lock, flags);
	}

	k3chan->phychan = NULL;
}

static void k3dma_pause_phy_chan(struct k3dma_phy_chan *ch)
{
	u32 val;
	int timeout;

	if (ch) {
		/* Set the HALT bit and wait for the channel stop */
		val = dma_reg_read(ch->ch_regs + CX_CONFIG_OFFSET);
		val &= ~K3_DMA_CONFIG_ENABLE;
		dma_reg_write(ch->ch_regs + CX_CONFIG_OFFSET, val);

		/* Wait for channel inactive */
		for (timeout = 2000; timeout > 0; timeout--) {
			if (!k3dma_phy_channel_busy(ch))
				break;
			udelay(1);
		}

		if (timeout == 0)
			printk(KERN_ERR DEVICE_NAME ":channel%u timeout waiting for pause, timeout:%d\n",
				ch->chan_id, timeout);
	} else{
		printk(KERN_ERR DEVICE_NAME ":channel is null, no phy channel to pause\n");
	}
}

static void k3dma_terminate_phy_chan(struct k3dma_device *k3dma,
	struct k3dma_phy_chan *ch)
{
	int mask = 0;
	if (ch) {
		mask = 0x1 << ch->chan_id;

		k3dma_pause_phy_chan(ch);

		/*clear irq*/
		dma_reg_write(k3dma->regs + INT_TC1_RAW_OFFSET, mask);
		dma_reg_write(k3dma->regs + INT_TC2_RAW_OFFSET, mask);
		dma_reg_write(k3dma->regs + INT_ERR1_RAW_OFFSET, mask);
		dma_reg_write(k3dma->regs + INT_ERR2_RAW_OFFSET, mask);
		dma_reg_write(k3dma->regs + INT_ERR3_RAW_OFFSET, mask);
	} else{
		printk(KERN_ERR DEVICE_NAME ":channel is null, no phy channel to terminate\n");
	}
}

/**
 * This function to get the number of remaining
 * bytes in the currently active transaction
 * Note:The channel should be paused when calling this
 */
static u32 k3dma_getbytes_chan(struct k3dma_chan *k3chan)
{
	struct k3dma_device *k3dma = to_k3dma(k3chan->chan_common.device);
	struct k3dma_phy_chan *ch	= NULL;
	struct k3dma_desc *desc	= NULL;
	struct k3dma_desc *child	= NULL;
	struct dma_slave_config *slave_config = NULL;
	unsigned long flags;
	u32 bytes = 0;
	u32 source = 0;
	u32 dest = 0;
	u32 num = 0;

	ch = k3chan->phychan;
	desc = k3chan->at;
	slave_config = &k3chan->slave_config;

	/*
	 * Follow the LLIs to get the number of remaining
	 * bytes in the currently active transaction.
	 */
	if (ch && desc) {
		/* First get the remaining bytes in the active transfer */
		spin_lock_irqsave(&k3dma->lock, flags);
		if (k3dma_phy_channel_busy(ch)) {
			spin_unlock_irqrestore(&k3dma->lock, flags);
			return 0;
		}

		bytes = dma_reg_read(k3dma->regs
			+ CX_CURR_CNT0_OFFSET(ch->chan_id));
		source = dma_reg_read(k3dma->regs
			+ CX_CURR_SRC_ADDR_OFFSET(ch->chan_id));
		dest = dma_reg_read(k3dma->regs
			+ CX_CURR_DES_ADDR_OFFSET(ch->chan_id));
		spin_unlock_irqrestore(&k3dma->lock, flags);

		num  = 0;
		list_for_each_entry(child, &desc->tx_list, desc_node) {
			if (slave_config->direction == DMA_TO_DEVICE) {
				if ((source >= desc->lli.saddr)
					&& (source <= (desc->lli.saddr + desc->len))) {
					bytes += child->len;
					continue;
				}

				if ((source >= child->lli.saddr)
					&& (source <= (child->lli.saddr + child->len))) {
					num++;
					continue;
				}

				if (num)
					bytes = bytes + child->len;
			} else if (slave_config->direction == DMA_FROM_DEVICE) {
				if ((dest >= desc->lli.daddr)
					&& (dest <= (desc->lli.daddr + desc->len))) {
					continue;
				}

				if ((dest >= child->lli.daddr)
					&& (dest <= (child->lli.daddr + child->len))) {
					num++;
					continue;
				}

				if (num)
					bytes = bytes + child->len;
			} else if (slave_config->direction == DMA_NONE) {
				if ((source >= desc->lli.saddr)
					&& (source <= (desc->lli.saddr + desc->len))) {
					continue;
				}

				if ((source >= child->lli.saddr)
					&& (source <= (child->lli.saddr + child->len))) {
					num++;
					continue;
				}

				if (num)
					bytes = bytes + child->len;
			} else{
				dev_err(k3dma->dma_common.dev,
					"%s not correct direction\n", __func__);
				return 0;
			}
		}
	}

	/* Sum up all queued transactions */
	if (!list_empty(&k3chan->pend_list)) {
		struct k3dma_desc *txdi;
		list_for_each_entry(txdi, &k3chan->pend_list, desc_node)
			bytes += txdi->total;
	}

	return bytes;
}



/**
 * k3dma_dostart - starts the DMA engine for real
 * @k3chan: the channel we want to start
 * @first: first descriptor in the list we want to begin with
 *
 * Called with atchan->lock held and bh disabled
 */
static void k3dma_dostart(struct k3dma_chan *k3chan, struct k3dma_desc *first)
{
	struct k3dma_phy_chan *ch = k3chan->phychan;
	struct k3dma_device *k3dma = to_k3dma(k3chan->chan_common.device);
	struct k3dma_lli *lli = &first->lli;
	int ret = 0;

	k3chan->at = first;
	BUG_ON(k3dma_phy_channel_busy(ch));

	if (k3chan->ref_count == 0) {
		if (k3dma->hasacp)
			ret = clk_enable(k3dma->clk_acp);
			if (ret) {
				dev_err(k3dma->dma_common.dev, "%s: clock enable fail", __func__);
			}
	}
	k3chan->ref_count++;
	/*config chan register*/
	dma_reg_write(ch->ch_regs + CX_LLI_OFFSET,		lli->lli);
	dma_reg_write(ch->ch_regs + CX_BINDX_OFFSET,	lli->bindx);
	dma_reg_write(ch->ch_regs + CX_CINDX_OFFSET,	lli->cindx);
	dma_reg_write(ch->ch_regs + CX_CNT1_OFFSET,		lli->cnt1);
	dma_reg_write(ch->ch_regs + CX_CNT0_OFFSET,		lli->cnt0);
	dma_reg_write(ch->ch_regs + CX_SRC_ADDR_OFFSET,	lli->saddr);
	dma_reg_write(ch->ch_regs + CX_DES_ADDR_OFFSET,	lli->daddr);
	dma_reg_write(ch->ch_regs + CX_AXI_CONF_OFFSET,	K3_DMA_DEFAULT_AXI);
	dma_reg_write(ch->ch_regs + CX_CONFIG_OFFSET,	lli->config);
}

static void
k3dma_chain_complete(struct k3dma_chan *k3chan, struct k3dma_desc *desc)
{
	struct dma_async_tx_descriptor	*txd = &desc->txd;
	dma_async_tx_callback callback = txd->callback;
	void *callback_param = txd->callback_param;
	unsigned long flags;

	/* unmap dma addresses */
	if (k3chan->slave_config.direction == DMA_NONE)
		k3dma_unmap_buffers(desc);

	spin_lock_irqsave(&k3chan->lock, flags);
	list_splice_init(&desc->tx_list, &k3chan->free_list);
	list_add(&desc->desc_node, &k3chan->free_list);
	spin_unlock_irqrestore(&k3chan->lock, flags);

	/* Callback to signal completion */
	if (callback)
		callback(callback_param);
}

/**
 * k3dma_cleanup_descriptors - cleanup up finished descriptors in pend_list
 * @k3chan: channel to be cleaned up
 *
 * Called with k3chan->lock held and bh disabled
 */
static void k3dma_cleanup_descriptors(struct k3dma_chan *k3chan)
{
	struct k3dma_desc	*desc, *_desc;

	dev_dbg(chan2dev(&k3chan->chan_common), "cleanup descriptors\n");

	list_for_each_entry_safe(desc, _desc, &k3chan->pend_list, desc_node) {
		list_splice_init(&desc->tx_list, &k3chan->free_list);
		list_add(&desc->desc_node, &k3chan->free_list);
	}

	INIT_LIST_HEAD(&k3chan->pend_list);
}

/**
 * k3dma_handle_error - handle errors reported by DMA controller
 * @k3chan: channel where error occurs
 *
 * Called with k3chan->lock held and bh disabled
 */
static void k3dma_handle_error(struct k3dma_chan *k3chan)
{
	struct k3dma_device *k3dma = to_k3dma(k3chan->chan_common.device);
	struct k3dma_phy_chan *ch = k3chan->phychan;

	if (k3chan->errtype == K3DMA_ERROR_CONFIG)
		dev_err(chan2dev(&k3chan->chan_common),
			"k3dma config error, peri index:%d\n", k3chan->cd.peri);

	if (k3chan->errtype == K3DMA_ERROR_TRANS)
		dev_err(chan2dev(&k3chan->chan_common),
			"k3dma trans error, peri index:%d\n", k3chan->cd.peri);

	k3chan->errtype = K3DMA_NO_ERROR;

	if (ch) {
		k3dma_dumpregs(k3dma, ch->chan_id);
		k3dma_terminate_phy_chan(k3dma, ch);
	}
}

static void k3dma_unmap_buffers(struct k3dma_desc *desc)
{
	struct device *dev = desc->txd.chan->device->dev;

	if (!(desc->txd.flags & DMA_COMPL_SKIP_SRC_UNMAP)) {
		if (desc->txd.flags & DMA_COMPL_SRC_UNMAP_SINGLE)
			dma_unmap_single(dev, desc->lli.saddr, desc->len,
				DMA_TO_DEVICE);
		else
			dma_unmap_page(dev, desc->lli.saddr, desc->len,
				DMA_TO_DEVICE);
	}
	if (!(desc->txd.flags & DMA_COMPL_SKIP_DEST_UNMAP)) {
		if (desc->txd.flags & DMA_COMPL_DEST_UNMAP_SINGLE)
			dma_unmap_single(dev, desc->lli.daddr, desc->len,
				DMA_FROM_DEVICE);
		else
			dma_unmap_page(dev, desc->lli.daddr, desc->len,
				DMA_FROM_DEVICE);
	}
}

/*--  IRQ & Tasklet  ---------------------------------------------------*/
static void k3dma_tasklet(unsigned long data)
{
	struct k3dma_chan *k3chan = (struct k3dma_chan *)data;
	struct k3dma_device *k3dma = to_k3dma(k3chan->chan_common.device);
	unsigned long flags;
	struct k3dma_desc *desc = NULL;


	spin_lock_irqsave(&k3chan->lock, flags);
	desc = k3chan->at;
	k3chan->at = NULL;

	if (desc) {
		/* Update last completed */
		k3chan->lc = desc->txd.cookie;
	}

	/* handle error */
	if (k3chan->errtype != K3DMA_NO_ERROR)
		k3dma_handle_error(k3chan);

	/* If a new descriptor is queued, set it up k3chan->at is NULL here */
	if (!list_empty(&k3chan->pend_list)) {
		struct k3dma_desc *next;

		next = list_first_entry(&k3chan->pend_list,
					struct k3dma_desc,
					desc_node);
		list_del(&next->desc_node);

		k3dma_dostart(k3chan, next);
	} else if (k3chan->phychan_hold) {
		/*
		 * This channel is still in use - we have a new txd being
		 * prepared and will soon be queued.  Don't give up the
		 * physical channel.
		 */
	} else {

		/*
		 * No more jobs, so free up the physical channel
		 * Free any allocated signal on slave transfers too
		 */
		k3dma_put_phy_channel(k3chan);
	}

	if (k3chan->ref_count > 0) {
		k3chan->ref_count--;

		if (k3chan->ref_count == 0) {
			if (k3dma->hasacp)
				clk_disable(k3dma->clk_acp);
		}
	}

	spin_unlock_irqrestore(&k3chan->lock, flags);

	if (desc)
		k3dma_chain_complete(k3chan, desc);

	return;
}
static irqreturn_t k3dma_interrupt(int irq, void *dev_id)
{
	struct k3dma_device		*k3dma = (struct k3dma_device *)dev_id;
	struct k3dma_chan	*k3chan;
	struct k3dma_phy_chan	*ch;
	u32			i;
	u32 mask	= 0;
	u32 mask_tc1	= 0;
	u32 mask_err1	= 0;
	u32 mask_err2	= 0;
	u32 ret		= 0;

	int stat = dma_reg_read(k3dma->regs + INT_STAT_OFFSET);
	int tc1  = dma_reg_read(k3dma->regs + INT_TC1_OFFSET);
	int err1 = dma_reg_read(k3dma->regs + INT_ERR1_OFFSET);
	int err2 = dma_reg_read(k3dma->regs + INT_ERR2_OFFSET);
	for (i = 0; i < k3dma->pd->nr_phy_channels; i++) {
		mask = 0x1 << i;

		/*deal interrupt*/
		if (stat & mask) {
			ch = &k3dma->phy_chans[i];
			spin_lock(&ch->lock);
			k3chan = ch->serving;
			if (NULL == k3chan){
				spin_unlock(&ch->lock);
				continue;
			}
			spin_unlock(&ch->lock);
			if (likely(tc1 & mask)) {
				mask_tc1 |= 0x1 << i;
				k3chan->state = K3DMA_CHAN_SUCCESS;
			}

			if (unlikely(err1 & mask)) {
				mask_err1 |= 0x1 << i;
				k3chan->state = K3DMA_CHAN_ERROR;
				k3chan->errtype = K3DMA_ERROR_CONFIG;
			}

			if (unlikely(err2 & mask)) {
				mask_err2 |= 0x1 << i;
				k3chan->state = K3DMA_CHAN_ERROR;
				k3chan->errtype = K3DMA_ERROR_TRANS;
			}

			tasklet_schedule(&k3chan->tasklet);
		}
	}

	dma_reg_write(k3dma->regs + INT_TC1_RAW_OFFSET, mask_tc1);
	dma_reg_write(k3dma->regs + INT_ERR1_RAW_OFFSET, mask_err1);
	dma_reg_write(k3dma->regs + INT_ERR2_RAW_OFFSET, mask_err2);

	ret = mask_tc1 | mask_err1 | mask_err2;
	return ret ? IRQ_HANDLED : IRQ_NONE;
}


/*
 * Fills in one LLI for a certain transfer descriptor and advance the counter
 */
static void k3dma_fill_lli_for_desc(struct k3dma_desc *desc,
	int src, int dst, int len, u32 cctl)
{
	desc->lli.saddr = src;
	desc->lli.daddr = dst;
	desc->lli.cindx = 0;
	desc->lli.bindx = 0;
	desc->lli.cnt1 = 0;
	desc->lli.cnt0 = len;
	desc->lli.config = cctl;
}

static struct k3dma_desc *k3dma_fill_llis_for_desc(struct k3dma_chan *k3chan,
	int src, int dst, int len, u32 cctl, struct k3dma_desc **last)
{
	int remainder = len ;
	int bytes_transferred = 0;
	int cnt = 0;
	int source = src;
	int dest = dst;
	struct k3dma_desc *first = NULL;
	struct k3dma_desc *prev = NULL;
	struct k3dma_desc *desc = NULL;

	remainder = len;
	while (remainder > 0) {
		if (remainder < K3_DMA_BTSIZE_MAX)
			cnt = remainder;
		else
			cnt = K3_DMA_BTSIZE_MAX;

		desc = k3dma_desc_get(k3chan);
		if (!desc) {
			dev_err(chan2dev(&k3chan->chan_common),
				"%s no memory for desc\n", __func__);
			goto err_desc_get;
		}

		if (k3chan->slave_config.direction == DMA_TO_DEVICE) {
			source = src + bytes_transferred;
		} else if (k3chan->slave_config.direction == DMA_FROM_DEVICE) {
			dest = dst + bytes_transferred;
		} else if (k3chan->slave_config.direction == DMA_NONE) {
			source = src + bytes_transferred;
			dest = dst + bytes_transferred;
		} else {
			dev_err(chan2dev(&k3chan->chan_common),
				"%s not correct direction\n", __func__);
			goto err_desc_get;
		}

		k3dma_fill_lli_for_desc(desc, source, dest, cnt, cctl);
		desc->len = cnt;
		desc->txd.cookie = 0;
		async_tx_ack(&desc->txd);

		if (!first) {
			first = desc;
		} else {
			/* to make lli 256-bit align */
			prev->lli.lli = (desc->txd.phys & K3_DMA_LLI_MASK1)
				| K3_DMA_LLI_MASK2;

			/* insert the link descriptor to the end of list */
			list_add_tail(&desc->desc_node,
					&first->tx_list);
		}
		prev = desc;

		/* refresh values of remainder and bytes_transferred*/
		bytes_transferred += cnt;
		remainder -= cnt;
	}

	first->total = len;

	/* set end-of-link to the last link descriptor of list*/
	set_desc_eol(desc);

	if (last)
		*last = desc;
	return first;
err_desc_get:
	k3dma_desc_put(k3chan, first);
	return NULL;
}

static int k3dma_get_trans_bitwidth_cctl(u32 addr_width)
{
	int dma_bit_width = addr_width;
	int dma_bit_width_cctl = -ENXIO;

	switch (dma_bit_width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		dma_bit_width_cctl = 0;
		break;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		dma_bit_width_cctl = 1;
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		dma_bit_width_cctl = 2;
		break;
	case DMA_SLAVE_BUSWIDTH_8_BYTES:
		dma_bit_width_cctl = 3;
		break;
	default:
		dma_bit_width_cctl = -ENXIO;
		printk(KERN_ERR DEVICE_NAME ":not supported slave bitwidth\n");
		break;
	}
	return dma_bit_width_cctl;
}
static int k3dma_get_trans_burstlength_cctl(u32 maxburst)
{
	int dma_burst_length = maxburst;

	if ((dma_burst_length == 0) || (dma_burst_length > K3_DMA_SLAVE_BURST_SIZE_MAX)) {
		printk(KERN_ERR DEVICE_NAME ":not supported slave burstlength\n");
		return -ENXIO;
	}

	dma_burst_length -= 1;
	return dma_burst_length;
}
static u32 k3dma_get_trans_cctl(struct k3dma_chan *k3chan)
{
	struct dma_slave_config	*slave_config = &k3chan->slave_config;
	enum dma_data_direction direction = slave_config->direction;
	int config = 0;

	int reg_burst = 0;
	int reg_width = 0;
	int mem_burst = 0;
	int mem_width = 0;

	if (direction == DMA_TO_DEVICE) {
		reg_width = slave_config->dst_addr_width;
		reg_burst = slave_config->dst_maxburst;

		reg_width = k3dma_get_trans_bitwidth_cctl(reg_width);
		if (reg_width < 0)
			return 0;

		if (slave_config->src_addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED) {
			mem_width = reg_width;
		} else{
			mem_width = k3dma_get_trans_bitwidth_cctl(slave_config->src_addr_width);
			if (mem_width < 0)
				return 0;
		}

		reg_burst = k3dma_get_trans_burstlength_cctl(reg_burst);
		if (reg_burst < 0)
			return 0;

		if (slave_config->src_maxburst == K3_DMA_SLAVE_BURST_UNDEFINED) {
			mem_burst = reg_burst;
		} else{
			mem_burst = k3dma_get_trans_burstlength_cctl(slave_config->src_maxburst);
			if (mem_burst < 0)
				return 0;
		}

		config |= K3_DMA_SRC_ADDR_MODE_INCR
			| K3_DMA_DST_ADDR_MODE_FIXED
			|(mem_burst << 24)
			|(reg_burst << 20)
			|(mem_width << 16)
			|(reg_width << 12)
			| K3_DMA_FC_MEM2PER
			|K3_DMA_CONFIG_ENABLE;
	} else if (direction == DMA_FROM_DEVICE) {
		reg_width = slave_config->src_addr_width;
		reg_burst = slave_config->src_maxburst;

		reg_width = k3dma_get_trans_bitwidth_cctl(reg_width);
		if (reg_width < 0)
			return 0;
		if (slave_config->dst_addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED) {
			mem_width = reg_width;
		} else{
			mem_width = k3dma_get_trans_bitwidth_cctl(slave_config->dst_addr_width);
			if (mem_width < 0)
				return 0;
		}

		reg_burst = k3dma_get_trans_burstlength_cctl(reg_burst);
		if (reg_burst < 0)
			return 0;
		if (slave_config->dst_maxburst == K3_DMA_SLAVE_BURST_UNDEFINED) {
			mem_burst = reg_burst;
		} else{
			mem_burst = k3dma_get_trans_burstlength_cctl(slave_config->dst_maxburst);
			if (mem_burst < 0)
				return 0;
		}

		config |= K3_DMA_SRC_ADDR_MODE_FIXED
			| K3_DMA_DST_ADDR_MODE_INCR
			|(reg_burst << 24)
			|(mem_burst << 20)
			|(reg_width << 16)
			|(mem_width << 12)
			| K3_DMA_FC_MEM2PER
			| K3_DMA_CONFIG_ENABLE;
	} else{
		printk(KERN_ERR DEVICE_NAME ":not supported slave direction\n");
		return 0;
	}

	return config;
}

/*--  DMA Engine API  --------------------------------------------------*/

/**
 * k3dma_tx_submit - set the prepared txdriptor(s) to be executed by the engine
 * @tx: txdriptor at the head of the transaction chain
 */
static dma_cookie_t k3dma_tx_submit(struct dma_async_tx_descriptor *tx)
{
	struct k3dma_desc		*desc = txd_to_k3dma_desc(tx);
	struct k3dma_chan	*k3chan = to_k3dma_chan(tx->chan);
	unsigned long flags;

	spin_lock_irqsave(&k3chan->lock, flags);
	BUG_ON(!k3chan->phychan);

	k3chan->chan_common.cookie += 1;
	if (k3chan->chan_common.cookie < 0)
		k3chan->chan_common.cookie = 1;
	tx->cookie = k3chan->chan_common.cookie;

	/* Put this onto the pending list */
	list_add_tail(&desc->desc_node, &k3chan->pend_list);
	k3chan->phychan_hold--;
	spin_unlock_irqrestore(&k3chan->lock, flags);

	return tx->cookie;
}

static void k3dma_desc_chain(struct k3dma_desc **first, struct k3dma_desc **prev,
			   struct k3dma_desc *desc, struct k3dma_desc *last)
{
	struct k3dma_desc		*child, *_child;
	if (!(*first)) {
		*first = desc;
	} else {
		/* to make lli 256-bit align */
		(*prev)->lli.lli = (desc->txd.phys & K3_DMA_LLI_MASK1)
			| K3_DMA_LLI_MASK2;

		/* insert the link descriptor to the end of list */
		list_add_tail(&desc->desc_node,
			&(*first)->tx_list);

		list_for_each_entry_safe(child, _child, &desc->tx_list, desc_node) {
			list_del(&child->desc_node);
			list_add_tail(&child->desc_node,
				&(*first)->tx_list);
		}
	}
	*prev = last;
}
/**
 * k3dma_prep_dma_memcpy - prepare a memcpy operation
 * @chan: the channel to prepare operation on
 * @dest: operation virtual destination address
 * @src: operation virtual source address
 * @len: operation length
 * @flags: tx txdriptor status flags
 */
static struct dma_async_tx_descriptor *
k3dma_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
		size_t len, unsigned long flags)
{
	struct k3dma_chan	*k3chan = to_k3dma_chan(chan);
	struct k3dma_desc				*first	= NULL;
	u32 config = 0;
	int ret = 0;

	dev_dbg(chan2dev(chan), "%s d0x%x s0x%x l0x%zx f0x%lx\n",
			__func__, dest, src, len, flags);

	if (unlikely(!len || (len >= SZ_1G))) {
		dev_err(chan2dev(chan),
			"%s  length is zero or above 1G!\n", __func__);
		return NULL;
	}

	/*get phy channels*/
	ret = k3dma_get_phy_channel(k3chan);
	if (ret)
		return NULL;

	/**fill llis*/
	config = K3_DMA_DEFAULT_CONFIG;
	first = k3dma_fill_llis_for_desc(k3chan, src, dest, len, config, NULL);
	if (!first)
		goto err_desc_get;

	first->txd.cookie = -EBUSY;
	first->total = len;


	first->txd.flags = flags; /* client is in control of this ack */
	return &first->txd;

err_desc_get:
	spin_lock_irqsave(&k3chan->lock, flags);
	k3chan->phychan_hold--;
	k3dma_put_phy_channel(k3chan);
	spin_unlock_irqrestore(&k3chan->lock, flags);
	return NULL;
}

/**
 * k3dma_prep_slave_sg - prepare txdriptors for a DMA_SLAVE transaction
 * @chan: DMA channel
 * @sgl: scatterlist to transfer to/from
 * @sg_len: number of entries in @scatterlist
 * @direction: DMA direction
 * @flags: tx txdriptor status flags
 */
static struct dma_async_tx_descriptor *
k3dma_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_data_direction direction,
		unsigned long flags)
{
	struct k3dma_chan *k3chan = to_k3dma_chan(chan);
	struct dma_slave_config	*slave_config = &k3chan->slave_config;
	struct k3dma_desc		*first	= NULL;
	struct k3dma_desc		*prev	= NULL;
	struct k3dma_desc		*child	= NULL;
	u32			config	= 0;
	dma_addr_t		reg;
	unsigned int		i;
	struct scatterlist	*sg;
	size_t			total_len = 0;
	int ret = 0;

	if (unlikely((slave_config->direction == DMA_NONE) || !sg_len)) {
		dev_err(chan2dev(chan),
			"k3dma_prep_slave_sg: slave is null or length is zero!\n");
		return NULL;
	}

	config = k3chan->cctl;
	if (config == 0) {
		dev_err(chan2dev(chan),
			"k3dma_prep_slave_sg: slave cctl is zero!\n");
		return NULL;
	}

	/*get phy channels*/
	ret = k3dma_get_phy_channel(k3chan);
	if (ret)
		return NULL;

	switch (direction) {
	case DMA_TO_DEVICE:
		reg = slave_config->dst_addr;
		for_each_sg(sgl, sg, sg_len, i) {
			struct k3dma_desc	*desc = NULL;
			struct k3dma_desc	*last = NULL;
			u32		len;
			u32		mem;

			if (unlikely(NULL == sg)) {
				dev_err(chan2dev(chan),
					"%s  sg is null pointer!\n",
					__func__);
				goto err_desc_get;
			}

			mem = sg_dma_address(sg);
			len = sg_dma_len(sg);
			if (unlikely(!len || (len >= SZ_1G))) {
				dev_err(chan2dev(chan),
					"%s  length is zero or above 1G!\n",
					__func__);
				goto err_desc_get;
			}

			desc = k3dma_fill_llis_for_desc(k3chan, mem, reg, len,
				config, &last);
			if (!desc)
				goto err_desc_get;

			k3dma_desc_chain(&first, &prev, desc, last);
			total_len += desc->total;
		}
		break;
	case DMA_FROM_DEVICE:
		reg = slave_config->src_addr;
		for_each_sg(sgl, sg, sg_len, i) {
			struct k3dma_desc	*desc = NULL;
			struct k3dma_desc  *last = NULL;
			u32		len;
			u32		mem;

			if (unlikely(NULL == sg)) {
				dev_err(chan2dev(chan),
					"%s  sg is null pointer!\n",
					__func__);
				goto err_desc_get;
			}

			mem = sg_dma_address(sg);
			len = sg_dma_len(sg);
			if (unlikely(!len || (len >= SZ_1G))) {
				dev_err(chan2dev(chan),
					"%s  length is zero or above 1G!\n",
					__func__);
				goto err_desc_get;
			}

			desc = k3dma_fill_llis_for_desc(k3chan, reg, mem, len,
				config, &last);
			if (!desc)
				goto err_desc_get;

			k3dma_desc_chain(&first, &prev, desc, last);
			total_len += desc->total;
		}
		break;
	default:
		dev_err(chan2dev(chan),
			"%s dma direction not correct\n", __func__);
		return NULL;
	}

	/* First descriptor of the chain embedds additional information */
	first->txd.cookie = -EBUSY;
	first->total = total_len;

	/* last link descriptor of list is responsible of flags */
	first->txd.flags = flags; /* client is in control of this ack */
	dev_dbg(chan2dev(chan),
		"first: len:0x%x, lli:0x%x, addr:0x%x, src:0x%x, dst:0x%x, cnt0:0x%x, config:0x%x\n",
			first->len, first->lli.lli,
			first->txd.phys, first->lli.saddr,
			first->lli.daddr, first->lli.cnt0,
			first->lli.config);

	list_for_each_entry(child, &first->tx_list, desc_node) {
		dev_dbg(chan2dev(chan),
			"child: lli:0x%x, addr:0x%x, src:0x%x, dst:0x%x, cnt0:0x%x, config:0x%x\n",
				child->lli.lli, child->txd.phys,
				child->lli.saddr, child->lli.daddr,
				child->lli.cnt0, child->lli.config);
	}

	return &first->txd;

err_desc_get:
	spin_lock_irqsave(&k3chan->lock, flags);
	k3chan->phychan_hold--;
	k3dma_put_phy_channel(k3chan);
	spin_unlock_irqrestore(&k3chan->lock, flags);
	k3dma_desc_put(k3chan, first);
	return NULL;
}

static int k3dma_set_runtime_config(struct dma_chan *chan,
				  struct dma_slave_config *config)
{
	struct k3dma_chan *k3chan = to_k3dma_chan(chan);
	u32 cctl;
	if (NULL == config) {
		dev_err(chan2dev(chan),
			"%s: slave config is null\n", __func__);
		return -1;
	}

	memcpy(&k3chan->slave_config, config, sizeof(struct dma_slave_config));

	cctl = k3dma_get_trans_cctl(k3chan);
	if (cctl == 0) {
		dev_err(chan2dev(chan), "cctl: 0x%x\n", cctl);
		return -1;
	}
	cctl |= k3chan->cd.peri << 4;
	k3chan->cctl = cctl;
	dev_dbg(chan2dev(chan), "cctl: 0x%x\n", cctl);

	return 0;
}

static int k3dma_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd,
			   unsigned long arg)
{
	struct k3dma_chan *k3chan = to_k3dma_chan(chan);
	struct k3dma_device *k3dma = to_k3dma(k3chan->chan_common.device);
	struct k3dma_desc *desc = NULL;
	unsigned long flags;
	int ret = 0;

	/* set channel runtime config */
	if (cmd == DMA_SLAVE_CONFIG) {
		return k3dma_set_runtime_config(chan,
				(struct dma_slave_config *)arg);
	}

	/*
	 * Anything succeeds on channels with no physical allocation and
	 * no queued transfers.
	 */
	spin_lock_irqsave(&k3chan->lock, flags);
	if (!k3chan->phychan && !k3chan->at) {
		spin_unlock_irqrestore(&k3chan->lock, flags);
		return 0;
	}

	desc = k3chan->at;
	switch (cmd) {
	case DMA_TERMINATE_ALL:
		k3chan->state = K3DMA_CHAN_IDLE;
		/* teminate physical */
		if (k3chan->phychan) {
			k3dma_terminate_phy_chan(k3dma, k3chan->phychan);
			k3dma_put_phy_channel(k3chan);
		}

		/* move desc to freelist */
		if (desc) {
			list_splice_init(&desc->tx_list, &k3chan->free_list);
			list_add(&desc->desc_node, &k3chan->free_list);
		}
		/* Dequeue jobs and free LLIs */
		if (!list_empty(&k3chan->pend_list))
			k3dma_cleanup_descriptors(k3chan);

		k3chan->at = NULL;
		if (k3chan->ref_count > 0) {
			k3chan->ref_count = 0;
			if (k3dma->hasacp){
				clk_disable(k3dma->clk_acp);
			}
		}

		break;
	case DMA_PAUSE:
		k3dma_pause_phy_chan(k3chan->phychan);
		k3chan->state = K3DMA_CHAN_PAUSED;
		break;
	case DMA_RESUME:
		ret = -ENXIO;
		break;
	default:
		/* Unknown command */
		ret = -ENXIO;
		break;
	}

	spin_unlock_irqrestore(&k3chan->lock, flags);

	return ret;
}

/**
 * k3dma_tx_status - poll for transaction completion
 * @chan: DMA channel
 * @cookie: transaction identifier to check status of
 * @txstate: if not %NULL updated with transaction state
 *
 * If @txstate is passed in, upon return it reflect the driver
 * internal state and can be used with dma_async_is_complete() to check
 * the status of multiple cookies without re-checking hardware state.
 */
static enum dma_status
k3dma_tx_status(struct dma_chan *chan,
		dma_cookie_t cookie,
		struct dma_tx_state *txstate)
{
	struct k3dma_chan *k3chan = to_k3dma_chan(chan);
	dma_cookie_t last_used;
	dma_cookie_t last_complete;
	enum dma_status ret;
	u32 bytesleft = 0;
	unsigned long flags;

	spin_lock_irqsave(&k3chan->lock, flags);
	if (unlikely(k3chan->state == K3DMA_CHAN_ERROR)) {
		spin_unlock_irqrestore(&k3chan->lock, flags);
		return DMA_ERROR;
	}

	last_used = k3chan->chan_common.cookie;
	last_complete = k3chan->lc;

	ret = dma_async_is_complete(cookie, last_complete, last_used);
	if (ret == DMA_SUCCESS) {
		dma_set_tx_state(txstate, last_complete, last_used, 0);
		spin_unlock_irqrestore(&k3chan->lock, flags);
		return ret;
	}

	/*
	 * This cookie not complete yet
	 */
	last_used = k3chan->chan_common.cookie;
	last_complete = k3chan->lc;

	/* Get number of bytes left in the active transactions and queue */
	if (k3chan->state == K3DMA_CHAN_PAUSED) {
		bytesleft = k3dma_getbytes_chan(k3chan);
		dma_set_tx_state(txstate, last_complete, last_used,
			 bytesleft);
		spin_unlock_irqrestore(&k3chan->lock, flags);
		return DMA_PAUSED;
	}
	spin_unlock_irqrestore(&k3chan->lock, flags);

	/* Whether waiting or running, we're in progress */
	return DMA_IN_PROGRESS;
}
/**
 * k3dma_issue_pending - try to finish work
 * @chan: target DMA channel
 */
static void k3dma_issue_pending(struct dma_chan *chan)
{
	struct k3dma_chan	*k3chan = to_k3dma_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&k3chan->lock, flags);
	/* Something is already active, or we're waiting for a channel... */
	if (k3chan->at) {
		spin_unlock_irqrestore(&k3chan->lock, flags);
		return;
	}

	/* Take the first element in the queue and execute it */
	if (!list_empty(&k3chan->pend_list)) {
		struct k3dma_desc *next;

		next = list_first_entry(&k3chan->pend_list,
					struct k3dma_desc,
					desc_node);
		list_del(&next->desc_node);

		k3dma_dostart(k3chan, next);
	}

	spin_unlock_irqrestore(&k3chan->lock, flags);
}


/**
 * k3dma_alloc_chan_resources - allocate resources for DMA channel
 * @chan: allocate descriptor resources for this channel
 * @client: current client requesting the channel be ready for requests
 *
 * return - the number of allocated descriptors
 */
static int k3dma_alloc_chan_resources(struct dma_chan *chan)
{
	struct k3dma_chan	*k3chan = to_k3dma_chan(chan);
	struct k3dma_desc_page* k3_desc_pg = NULL;
	struct k3dma_desc		*desc;
	int			i;
	unsigned long flags;
	unsigned int init_nr_desc_per_channel = PAGE_SIZE / init_desc_len;

	LIST_HEAD(tmp_list);

	dev_dbg(chan2dev(chan), "alloc_chan_resources\n");

	/* have we already been set up?
	 * reconfigure channel but no need to reallocate descriptors */
	if ((!list_empty(&k3chan->free_list)) && (!list_empty(&k3chan->page_list)))
		return k3chan->descs_allocated;

	/* Allocate descriptors */
	k3_desc_pg = (struct k3dma_desc_page*)kmalloc(sizeof(struct k3dma_desc_page),GFP_KERNEL);
	if (!k3_desc_pg) {
		dev_err(chan2dev(&k3chan->chan_common),
			"no memery for desc\n");
		return 0;
	}
	memset(k3_desc_pg, 0, sizeof(struct k3dma_desc_page));

	k3_desc_pg->page_link = (unsigned char *) __get_free_page(GFP_KERNEL);
	if (!k3_desc_pg->page_link) {
		dev_err(chan2dev(&k3chan->chan_common),
			"no memery for page link\n");
		kfree(k3_desc_pg);
		return 0;
	}
	memset(k3_desc_pg->page_link, 0, PAGE_SIZE);

	for (i = 0; i < init_nr_desc_per_channel; i++) {
		desc = (struct k3dma_desc*)((unsigned char*)k3_desc_pg->page_link + init_desc_len * i);
		k3dma_init_descriptor(chan, desc);
		list_add_tail(&desc->desc_node, &tmp_list);
	}

	spin_lock_irqsave(&k3chan->lock, flags);
	k3chan->descs_allocated = i;
	list_splice(&tmp_list, &k3chan->free_list);
	list_add(&k3_desc_pg->pg_node, &k3chan->page_list);
	k3chan->lc = chan->cookie = 1;
	spin_unlock_irqrestore(&k3chan->lock, flags);

	dev_dbg(chan2dev(chan),
		"alloc_chan_resources: allocated %d descriptors\n",
		k3chan->descs_allocated);

	return k3chan->descs_allocated;
}

/**
 * k3dma_free_chan_resources - free all channel resources
 * @chan: DMA channel
 */
static void k3dma_free_chan_resources(struct dma_chan *chan)
{
	struct k3dma_chan	*k3chan = to_k3dma_chan(chan);
	struct k3dma_desc		*desc, *_desc;
	struct k3dma_desc_page    *k3chan_page,*_k3chan_page;
	unsigned long flags;
	LIST_HEAD(list);

	dev_dbg(chan2dev(chan), "free_chan_resources: (descs allocated=%u)\n",
		k3chan->descs_allocated);

	/* ASSERT:channel is idle */
	k3dma_control(chan, DMA_TERMINATE_ALL, 0);
	list_for_each_entry_safe(desc, _desc, &k3chan->free_list, desc_node) {
		dev_dbg(chan2dev(chan), "  freeing descriptor %p\n", desc);
		list_del(&desc->desc_node);
	}

	list_for_each_entry_safe(k3chan_page, _k3chan_page, &k3chan->page_list, pg_node) {
		list_del(&k3chan_page->pg_node);
		free_page((unsigned long)k3chan_page->page_link);
		kfree(k3chan_page);
	}

	spin_lock_irqsave(&k3chan->lock, flags);
	list_splice_init(&k3chan->free_list, &list);
	list_splice_init(&k3chan->page_list, &list);
	k3chan->descs_allocated = 0;
	spin_unlock_irqrestore(&k3chan->lock, flags);

	dev_dbg(chan2dev(chan), "free_chan_resources: done\n");

	return;
}
/*
 * Initialise the DMAC memcpy/slave channels.
 * Make a local wrapper to hold required data
 */
static void k3dma_init_virtual_channels(struct k3dma_device *k3dma)
{
	int i = 0;

	INIT_LIST_HEAD(&k3dma->dma_common.channels);
	for (i = 0; i < k3dma->pd->nr_slave_channels; i++,
		k3dma->dma_common.chancnt++) {
		struct k3dma_chan	*k3chan = &k3dma->dma_chans[i];

		k3chan->chan_common.device = &k3dma->dma_common;
		k3chan->chan_common.cookie = k3chan->lc = 0;
		k3chan->chan_common.chan_id = i;
		list_add_tail(&k3chan->chan_common.device_node,
				&k3dma->dma_common.channels);

		spin_lock_init(&k3chan->lock);
		k3chan->at = NULL;
		k3chan->phychan = NULL;
		k3chan->phychan_hold = 0;
		memset(&k3chan->slave_config, 0, sizeof(struct dma_slave_config));
		k3chan->slave_config.direction = DMA_NONE;
		k3chan->cd = k3dma->pd->cd[i];
		k3chan->errtype = K3DMA_NO_ERROR;
		k3chan->state = K3DMA_CHAN_IDLE;
		k3chan->descs_allocated = 0;
		k3chan->cctl = 0;
		k3chan->ref_count = 0;
		k3chan->lc = 0;

		INIT_LIST_HEAD(&k3chan->free_list);
		INIT_LIST_HEAD(&k3chan->pend_list);
		INIT_LIST_HEAD(&k3chan->page_list);

		tasklet_init(&k3chan->tasklet, k3dma_tasklet,
				(unsigned long)k3chan);
	}

	return;

}

static void k3dma_deinit_virtual_channels(struct k3dma_device *k3dma)
{
	struct dma_chan		*chan, *_chan;
	struct k3dma_desc_page    *k3chan_page,*_k3chan_page;

	list_for_each_entry_safe(chan, _chan, &k3dma->dma_common.channels,
			device_node) {
		struct k3dma_chan	*k3chan = to_k3dma_chan(chan);

		/* Disable interrupts */
		tasklet_disable(&k3chan->tasklet);
		tasklet_kill(&k3chan->tasklet);
		k3chan->phychan = NULL;
		k3chan->at = NULL;
		list_for_each_entry_safe(k3chan_page, _k3chan_page, &k3chan->page_list, pg_node) {
			list_del(&k3chan_page->pg_node);
			free_page((unsigned long)k3chan_page->page_link);
			kfree(k3chan_page);
		}
		list_del(&chan->device_node);
	}

}

static void k3dma_init_phy_channels(struct k3dma_device *k3dma)
{
	int i = 0;

	for (i = 0; i < k3dma->pd->nr_phy_channels; i++) {
		struct k3dma_phy_chan *phychan = &k3dma->phy_chans[i];

		phychan->chan_id = i;
		phychan->ch_regs = k3dma->regs + ch_regs(i);
		phychan->serving = NULL;
		spin_lock_init(&phychan->lock);
	}

	return;
}

static void k3dma_deinit_phy_channels(struct k3dma_device *k3dma)
{
	int i = 0;

	for (i = 0; i < k3dma->pd->nr_phy_channels; i++) {
		struct k3dma_phy_chan *phychan = &k3dma->phy_chans[i];
		phychan->ch_regs = NULL;
		phychan->serving = NULL;
	}

	return;
}

/*--  Module Management  -----------------------------------------------*/
static int k3dma_probe(struct platform_device *pdev)
{
	struct k3dma_platform_data	*pdata;
	struct resource			*io;
	struct k3dma_device			*k3dma;
	size_t				size;
	int				irq;
	int				err;

	/* get DMA Controller parameters from platform */
	pdata = pdev->dev.platform_data;
	if (!pdata || pdata->nr_slave_channels > K3_DMA_MAX_NR_SLAVE_CHANNELS) {
		dev_err(&pdev->dev, "%s: failed to get slave channels\n",
			__func__);
		return -EINVAL;
	}

	io = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (!io) {
		dev_err(&pdev->dev, "%s: failed to get dma resource\n",
			__func__);
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "%s: failed to get dma irq\n",
			__func__);
		return irq;
	}

	size = sizeof(struct k3dma_device);
	k3dma = kzalloc(size, GFP_KERNEL);
	if (!k3dma) {
		dev_err(&pdev->dev, "%s: failed to allocate k3dma memory\n",
			__func__);
		return -ENOMEM;
	}

	dma_cap_set(DMA_MEMCPY,	pdata->cap_mask);
	dma_cap_set(DMA_SLAVE,	pdata->cap_mask);
	dma_cap_set(DMA_PRIVATE, pdata->cap_mask);

	/* discover transaction capabilites from the platform data */
	k3dma->dma_common.cap_mask = pdata->cap_mask;
	k3dma->all_chan_mask = (1 << pdata->nr_slave_channels) - 1;
	k3dma->hasacp = pdata->hasacp;
	k3dma->pd = pdata;

	size = io->end - io->start + 1;
	if (!request_mem_region(io->start, size, pdev->dev.driver->name)) {
		dev_err(&pdev->dev, "%s: k3dma request_mem_region failed\n",
			__func__);
		err = -EBUSY;
		goto err_kfree;
	}

	k3dma->regs = ioremap(io->start, size);
	if (!k3dma->regs) {
		dev_err(&pdev->dev, "%s: k3dma ioremap failed\n",
			__func__);
		err = -ENOMEM;
		goto err_release_r;
	}

	/*get clk*/
	k3dma->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(k3dma->clk)) {
		dev_err(&pdev->dev, "%s: k3dma clk_get failed\n",
			__func__);
		err = PTR_ERR(k3dma->clk);
		goto err_clk;
	}

	/*set clock gate*/
	if (!k3dma->pd->clkgate_enable(1)) {
		dev_err(&pdev->dev, "%s: k3dma clkgate_enable failed\n",
			__func__);
		err = EINVAL;
		goto err_clkgate_enable;
	}

	/*choose acp*/
	if (k3dma->hasacp) {
		if (!k3dma->pd->acp_enable(1)) {
			dev_err(&pdev->dev, "%s: k3dma acp_enable failed\n",
				__func__);
			err = EINVAL;
			goto err_acp_enable;
		}

		k3dma->clk_acp = k3dma->pd->clkacp_get();
		if (!k3dma->clk_acp) {
			dev_err(&pdev->dev, "%s: k3dma clkacp_get failed\n",
				__func__);
			err = EINVAL;
			goto err_acp_enable;
		}
	} else {
		if (!k3dma->pd->acp_enable(0)) {
			dev_err(&pdev->dev, "%s: k3dma acp_disable failed\n",
				__func__);
			err = EINVAL;
			goto err_acp_enable;
		}
	}

	err = clk_enable(k3dma->clk);
	if (err) {
		dev_err(k3dma->dma_common.dev, "%s: clock enable fail", __func__);
		goto err_clk_enable;
	}

	/* force dma off, just in case */
	k3dma_interrupt_off(k3dma);
	err = request_irq(irq, k3dma_interrupt, 0, "k3dma", k3dma);
	if (err) {
		dev_err(&pdev->dev, "%s: k3dma request_irq failed, err:%d\n",
			__func__, err);
		goto err_irq;
	}

	platform_set_drvdata(pdev, k3dma);

	spin_lock_init(&k3dma->lock);

	/*initialize phy channels*/
	k3dma->phy_chans = kzalloc((pdata->nr_phy_channels
		* sizeof(struct k3dma_phy_chan)),
		GFP_KERNEL);
	if (!k3dma->phy_chans) {
		err = -ENOMEM;
		dev_err(&pdev->dev, "%s: k3dma allocate physical channel holders failed\n",
			__func__);
		goto out_no_phychans;
	}
	k3dma_init_phy_channels(k3dma);

	/*initialize virtual channels*/
	k3dma->dma_chans = kzalloc((pdata->nr_slave_channels
		* sizeof(struct k3dma_chan)), GFP_KERNEL);
	if (!k3dma->dma_chans) {
		err = -ENOMEM;
		dev_err(&pdev->dev, "%s: k3dma allocate virtual channel holders failed\n",
			__func__);
		goto out_no_virtualchans;
	}
	k3dma_init_virtual_channels(k3dma);

	/* set base routines */
	k3dma->dma_common.device_alloc_chan_resources = k3dma_alloc_chan_resources;
	k3dma->dma_common.device_free_chan_resources = k3dma_free_chan_resources;
	k3dma->dma_common.device_tx_status = k3dma_tx_status;
	k3dma->dma_common.device_issue_pending = k3dma_issue_pending;
	k3dma->dma_common.dev = &pdev->dev;

	/* set prep routines based on capability */
	if (dma_has_cap(DMA_MEMCPY, k3dma->dma_common.cap_mask))
		k3dma->dma_common.device_prep_dma_memcpy = k3dma_prep_dma_memcpy;

	if (dma_has_cap(DMA_SLAVE, k3dma->dma_common.cap_mask)) {
		k3dma->dma_common.device_prep_slave_sg = k3dma_prep_slave_sg;
		k3dma->dma_common.device_control = k3dma_control;
	}

	dev_info(&pdev->dev, "k3dmacv300 Controller ( %s%s), %d channels\n",
		dma_has_cap(DMA_MEMCPY, k3dma->dma_common.cap_mask)
			? "cpy " : "",
		dma_has_cap(DMA_SLAVE, k3dma->dma_common.cap_mask)
			? "slave " : "",
		k3dma->dma_common.chancnt);

	/*set dma priority*/
	dma_reg_write(k3dma->regs + CH_PRI_OFFSET, 0x0);

	/*force dma on*/
	k3dma_interrupt_on(k3dma);

	/*register dma device*/
	dma_async_device_register(&k3dma->dma_common);
	return 0;
out_no_virtualchans:
	kfree(k3dma->phy_chans);
	k3dma->phy_chans = NULL;
out_no_phychans:
	platform_set_drvdata(pdev, NULL);
	free_irq(platform_get_irq(pdev, 0), k3dma);
err_irq:
	clk_disable(k3dma->clk);
err_clk_enable:
	if (k3dma->hasacp)
		clk_put(k3dma->clk_acp);
err_acp_enable:
	k3dma->pd->clkgate_enable(0);
err_clkgate_enable:
	clk_put(k3dma->clk);
err_clk:
	iounmap(k3dma->regs);
	k3dma->regs = NULL;
err_release_r:
	release_mem_region(io->start, io->end - io->start + 1);
err_kfree:
	kfree(k3dma);
	k3dma = NULL;
	return err;

}

static void k3dma_shutdown(struct platform_device *pdev)
{
	struct k3dma_device	*k3dma = platform_get_drvdata(pdev);

	if (k3dma == NULL) {
		dev_err(&pdev->dev, "%s: k3dma is null\n", __func__);
		return;
	}

	printk("[%s] +\n", __func__);

	k3dma_interrupt_off(k3dma);

	printk("[%s] -\n", __func__);
}

static int k3dma_remove(struct platform_device *pdev)
{
	struct k3dma_device		*k3dma;
	struct resource		*io;

	k3dma = platform_get_drvdata(pdev);

	if (k3dma == NULL) {
		dev_err(&pdev->dev, "%s: k3dma is null\n", __func__);
		return -ENODEV;
	}

	/*force dma off*/
	k3dma_interrupt_off(k3dma);
	clk_disable(k3dma->clk);

	/*unregister dma common device*/
	dma_async_device_unregister(&k3dma->dma_common);

	clk_put(k3dma->clk);
	if (k3dma->hasacp)
		clk_put(k3dma->clk_acp);

	platform_set_drvdata(pdev, NULL);
	free_irq(platform_get_irq(pdev, 0), k3dma);

	/*deinit phychans*/
	k3dma_deinit_phy_channels(k3dma);
	kfree(k3dma->phy_chans);
	k3dma->phy_chans = NULL;

	/*deinit virchans*/
	k3dma_deinit_virtual_channels(k3dma);
	kfree(k3dma->dma_chans);
	k3dma->dma_chans = NULL;

	 /*unmap regs*/
	iounmap(k3dma->regs);
	k3dma->regs = NULL;
	io = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (io)
		release_mem_region(io->start, io->end - io->start + 1);

	/*kfree dma*/
	kfree(k3dma);
	k3dma = NULL;

	return 0;
}

#ifdef CONFIG_PM
static int k3dma_suspend_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct k3dma_device		*k3dma = platform_get_drvdata(pdev);
	u32 state = 0;

	if (k3dma == NULL) {
		dev_err(&pdev->dev, "%s: k3dma is null\n", __func__);
		return -ENODEV;
	}

	printk("[%s] +\n", __func__);

	/*channel is active, return -1*/
	state = dma_reg_read(k3dma->regs + CH_STAT_OFFSET);
	if (state) {
		dev_warn(k3dma->dma_common.dev,
			"chan is busy, so not to enter suspend state\n");
		return -1;
	}
	clk_disable(k3dma->clk);

	printk("[%s] -\n", __func__);

	return 0;
}

static int k3dma_resume_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct k3dma_device *k3dma = platform_get_drvdata(pdev);
	int ret = 0;

	if (k3dma == NULL) {
		dev_err(&pdev->dev, "%s: k3dma is null\n", __func__);
		return -ENODEV;
	}

	printk("[%s] +\n", __func__);

	ret = clk_enable(k3dma->clk);
	if (ret) {
		dev_err(k3dma->dma_common.dev, "%s: clock enable fail", __func__);
	}

	k3dma_interrupt_on(k3dma);
	dma_reg_write(k3dma->regs + CH_PRI_OFFSET, 0x0);

	printk("[%s] -\n", __func__);

	return 0;
}
static const struct dev_pm_ops k3dma_dev_pm_ops = {
	.suspend_noirq = k3dma_suspend_noirq,
	.resume_noirq = k3dma_resume_noirq,
};
#endif

static struct platform_driver k3dma_driver = {
	.probe = k3dma_probe,
	.shutdown = k3dma_shutdown,
	.remove = k3dma_remove,
	.driver = {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &k3dma_dev_pm_ops,
#endif
	},
};

static int __init dmac_module_init(void)
{
	int retval;

	retval = platform_driver_register(&k3dma_driver);
	if (retval) {
		printk(KERN_ERR "k3dma platform driver register failed\n");
		return retval;
	}

	return retval;
}

static void __exit dmac_module_exit(void)
{
	platform_driver_unregister(&k3dma_driver);
}

subsys_initcall(dmac_module_init);

MODULE_AUTHOR("k3<k3dma@huawei.com>");
MODULE_DESCRIPTION("Driver for K3DMA");
MODULE_LICENSE("GPL");
