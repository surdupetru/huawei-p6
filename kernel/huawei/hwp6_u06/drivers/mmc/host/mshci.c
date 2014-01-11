/*
 * linux/drivers/mmc/host/mshci.c
 *
 * Mobile Storage Host Controller Interface driver
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Based on linux/drivers/mmc/host/sdhci.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/leds.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>

#include <linux/mmc/mshci.h>

#include <linux/clk.h>
#include <linux/workqueue.h>

#include "k3v2_mmc_raw.h"

#ifdef CONFIG_MMC_SD_HUB
#include <linux/mmc/mn66831hub_api.h>
#endif /*	CONFIG_MMC_SD_HUB	*/

#define DRIVER_NAME "mshci"

#define DBG(f, x...) \
	pr_debug(DRIVER_NAME " [%s()]: " f, __func__, ## x)
	
#ifdef CONFIG_MMC_SD_HUB
#define		SDHUB_DEVICE_ID		(SDHUB_DEVID0)

#define MMC_CMD_SDIO		(1 << 14)

static int flag_connection = 0 ;
#endif	/*	CONFIG_MMC_SD_HUB	*/

#define SDHC_CLK_ON 1
#define SDHC_CLK_OFF 0
#define HI_MMC_OPT_WAITBUSY

#define DMA_SG_NUM 256

#define DELAY_TIME     2000

#ifdef CONFIG_MMC_SD_HUB
void SDHUB_clock_enable( void );
void SDHUB_clock_disable( void );
#endif

static unsigned int debug_quirks;

static void mshci_prepare_data(struct mshci_host *, struct mmc_data *);
static void mshci_finish_data(struct mshci_host *);

static void mshci_send_command(struct mshci_host *, struct mmc_command *);
static void mshci_finish_command(struct mshci_host *);
static void mshci_set_clock(struct mshci_host *host, unsigned int clock);
static void mshci_fifo_init(struct mshci_host *host);
static void mshci_request(struct mmc_host *mmc, struct mmc_request *mrq);

static void mshci_dumpregs(struct mshci_host *host)
{
	printk(KERN_ERR DRIVER_NAME ": ============== REGISTER DUMP ==============\n");
	printk(KERN_ERR DRIVER_NAME ": MSHCI_CTRL:	  0x%08x\n",
		mshci_readl(host, MSHCI_CTRL));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_PWREN:	 0x%08x\n",
		mshci_readl(host, MSHCI_PWREN));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_CLKDIV:	0x%08x\n",
		mshci_readl(host, MSHCI_CLKDIV));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_CLKSRC:	0x%08x\n",
		mshci_readl(host, MSHCI_CLKSRC));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_CLKENA:	0x%08x\n",
		mshci_readl(host, MSHCI_CLKENA));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_TMOUT:	 0x%08x\n",
		mshci_readl(host, MSHCI_TMOUT));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_CTYPE:	 0x%08x\n",
		mshci_readl(host, MSHCI_CTYPE));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_BLKSIZ:	0x%08x\n",
		mshci_readl(host, MSHCI_BLKSIZ));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_BYTCNT:	0x%08x\n",
		mshci_readl(host, MSHCI_BYTCNT));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_INTMSK:	0x%08x\n",
		mshci_readl(host, MSHCI_INTMSK));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_CMDARG:	0x%08x\n",
		mshci_readl(host, MSHCI_CMDARG));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_CMD:	   0x%08x\n",
		mshci_readl(host, MSHCI_CMD));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_MINTSTS:   0x%08x\n",
		mshci_readl(host, MSHCI_MINTSTS));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_RINTSTS:   0x%08x\n",
		mshci_readl(host, MSHCI_RINTSTS));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_STATUS:	0x%08x\n",
		mshci_readl(host, MSHCI_STATUS));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_FIFOTH:	0x%08x\n",
		mshci_readl(host, MSHCI_FIFOTH));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_CDETECT:   0x%08x\n",
		mshci_readl(host, MSHCI_CDETECT));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_WRTPRT:	0x%08x\n",
		mshci_readl(host, MSHCI_WRTPRT));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_GPIO:	  0x%08x\n",
		mshci_readl(host, MSHCI_GPIO));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_TCBCNT:	0x%08x\n",
		mshci_readl(host, MSHCI_TCBCNT));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_TBBCNT:	0x%08x\n",
		mshci_readl(host, MSHCI_TBBCNT));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_DEBNCE:	0x%08x\n",
		mshci_readl(host, MSHCI_DEBNCE));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_USRID:	 0x%08x\n",
		mshci_readl(host, MSHCI_USRID));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_VERID:	 0x%08x\n",
		mshci_readl(host, MSHCI_VERID));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_HCON:	  0x%08x\n",
		mshci_readl(host, MSHCI_HCON));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_UHS_REG:   0x%08x\n",
		mshci_readl(host, MSHCI_UHS_REG));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_BMOD:	  0x%08x\n",
		mshci_readl(host, MSHCI_BMOD));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_PLDMND:	0x%08x\n",
		mshci_readl(host, MSHCI_PLDMND));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_DBADDR:	0x%08x\n",
		mshci_readl(host, MSHCI_DBADDR));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_IDSTS:	 0x%08x\n",
		mshci_readl(host, MSHCI_IDSTS));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_IDINTEN:   0x%08x\n",
		mshci_readl(host, MSHCI_IDINTEN));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_DSCADDR:   0x%08x\n",
		mshci_readl(host, MSHCI_DSCADDR));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_BUFADDR:   0x%08x\n",
		mshci_readl(host, MSHCI_BUFADDR));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_WAKEUPCON: 0x%08x\n",
		mshci_readl(host, MSHCI_WAKEUPCON));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_CLOCKCON:  0x%08x\n",
		mshci_readl(host, MSHCI_CLOCKCON));
	printk(KERN_ERR DRIVER_NAME ": MSHCI_CARDTHRCTl:  0x%08x\n",
		mshci_readl(host, MSHCI_CARDTHRCTl));
	/*
	printk(KERN_ERR DRIVER_NAME ": MSHCI_FIFODAT:   0x%08x\n",
		mshci_readl(host, MSHCI_FIFODAT));
	*/
	printk(KERN_ERR DRIVER_NAME ": ===========================================\n");
}

/* Low level functions */

static void mshci_clear_set_irqs(struct mshci_host *host, u32 clear, u32 set)
{
	u32 ier;

	ier = mshci_readl(host, MSHCI_INTMSK);
	ier &= ~clear;
	ier |= set;
	mshci_writel(host, ier, MSHCI_INTMSK);
}

static void mshci_unmask_irqs(struct mshci_host *host, u32 irqs)
{
	mshci_clear_set_irqs(host, 0, irqs);
}

static void mshci_mask_irqs(struct mshci_host *host, u32 irqs)
{
	mshci_clear_set_irqs(host, irqs, 0);
}

static void mshci_set_card_detection(struct mshci_host *host, bool enable)
{
	u32 irqs = INTMSK_CDETECT;

	if (enable)
		mshci_unmask_irqs(host, irqs);
	else
		mshci_mask_irqs(host, irqs);
}

static void mshci_enable_card_detection(struct mshci_host *host)
{
	mshci_set_card_detection(host, true);
}

static void mshci_disable_card_detection(struct mshci_host *host)
{
	mshci_set_card_detection(host, false);
}

static void mshci_reset_ciu(struct mshci_host *host)
{
	u32 timeout = 100;
	u32 ier;

	ier = mshci_readl(host, MSHCI_CTRL);
	ier |= CTRL_RESET;

	mshci_writel(host, ier, MSHCI_CTRL);
	while (mshci_readl(host, MSHCI_CTRL) & CTRL_RESET) {
		if (timeout == 0) {
			printk(KERN_ERR "%s: Reset CTRL never completed.\n",
				mmc_hostname(host->mmc));
			mshci_dumpregs(host);
			return;
		}
		timeout--;
		mdelay(1);
	}
}

static void mshci_reset_fifo(struct mshci_host *host)
{
	u32 timeout = 100;
	u32 ier;

	ier = mshci_readl(host, MSHCI_CTRL);
	ier |= FIFO_RESET;

	mshci_writel(host, ier, MSHCI_CTRL);
	while (mshci_readl(host, MSHCI_CTRL) & FIFO_RESET) {
		if (timeout == 0) {
			printk(KERN_ERR "%s: Reset FIFO never completed.\n",
				mmc_hostname(host->mmc));
			dump_stack();
			mshci_dumpregs(host);
			return;
		}
		timeout--;
		mdelay(1);
	}
}

static void mshci_reset_dma(struct mshci_host *host)
{
	u32 timeout = 100;
	u32 ier;

	ier = mshci_readl(host, MSHCI_CTRL);
	ier |= DMA_RESET;

	mshci_writel(host, ier, MSHCI_CTRL);
	while (mshci_readl(host, MSHCI_CTRL) & DMA_RESET) {
		if (timeout == 0) {
			printk(KERN_ERR "%s: Reset DMA never completed.\n",
				mmc_hostname(host->mmc));
			mshci_dumpregs(host);
			return;
		}
		timeout--;
		mdelay(1);
	}
}

static void mshci_reset_all(struct mshci_host *host)
{
	mshci_reset_ciu(host);
	mshci_reset_fifo(host);
	mshci_reset_dma(host);
}

static void mshci_init(struct mshci_host *host)
{
	mshci_reset_all(host);

	/* clear interrupt status */
	mshci_writel(host, INTMSK_ALL, MSHCI_RINTSTS);

	mshci_clear_set_irqs(host, INTMSK_ALL,
		INTMSK_CDETECT | INTMSK_RE |
		INTMSK_CDONE | INTMSK_DTO | INTMSK_TXDR | INTMSK_RXDR |
		INTMSK_RCRC | INTMSK_DCRC | INTMSK_RTO | INTMSK_DRTO |
		INTMSK_HTO | INTMSK_FRUN | INTMSK_HLE | INTMSK_SBE |
		INTMSK_EBE);
}

static void mshci_reinit(struct mshci_host *host)
{
	mshci_init(host);
	mshci_enable_card_detection(host);

	mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) & ~INT_ENABLE),
					MSHCI_CTRL);
}

/* Core functions */

static void mshci_read_block_pio(struct mshci_host *host)
{
	unsigned long flags;
	size_t fifo_cnt, len, chunk;
	u32 uninitialized_var(scratch);
	u8 *buf;

	DBG("PIO reading\n");

	fifo_cnt = (mshci_readl(host, MSHCI_STATUS) & FIFO_COUNT) >> 17;
	fifo_cnt *= FIFO_WIDTH;
	chunk = 0;

	local_irq_save(flags);

	while (fifo_cnt) {
		if (!sg_miter_next(&host->sg_miter))
			BUG();

		len = min(host->sg_miter.length, fifo_cnt);

		fifo_cnt -= len;
		host->sg_miter.consumed = len;

		buf = host->sg_miter.addr;

		while (len) {
			if (chunk == 0) {
				scratch = mshci_readl(host, MSHCI_FIFODAT);
				chunk = 4;
			}

			*buf = scratch & 0xFF;

			buf++;
			scratch >>= 8;
			chunk--;
			len--;
		}
	}

	sg_miter_stop(&host->sg_miter);

	local_irq_restore(flags);
}

static void mshci_write_block_pio(struct mshci_host *host)
{
	unsigned long flags;
	size_t fifo_cnt, len, chunk;
	u32 scratch;
	u8 *buf;

	DBG("PIO writing\n");

	fifo_cnt = 8;

	fifo_cnt *= FIFO_WIDTH;
	chunk = 0;
	scratch = 0;

	local_irq_save(flags);

	while (fifo_cnt) {
		if (!sg_miter_next(&host->sg_miter)) {

			/*
			 * Even though transfer is complete,
			 * TXDR interrupt occurs again.
			 * So, it has to check that it has really
			 * no next sg buffer or just DTO interrupt
			 * has not occured yet.
			 */

			if ((host->data->blocks * host->data->blksz) ==
					host->data_transfered)
				/* transfer done but DTO not yet */
				break;
			BUG();
		}
		len = min(host->sg_miter.length, fifo_cnt);

		fifo_cnt -= len;
		host->sg_miter.consumed = len;
		host->data_transfered += len;

		buf = (host->sg_miter.addr);

		while (len) {
			scratch |= (u32)*buf << (chunk * 8);

			buf++;
			chunk++;
			len--;

			if ((chunk == 4) || ((len == 0) && (fifo_cnt == 0))) {
				mshci_writel(host, scratch, MSHCI_FIFODAT);
				chunk = 0;
				scratch = 0;
			}
		}
	}

	sg_miter_stop(&host->sg_miter);

	local_irq_restore(flags);
}

static void mshci_transfer_pio(struct mshci_host *host)
{
	BUG_ON(!host->data);

	if (host->blocks == 0)
		return;

	if (host->data->flags & MMC_DATA_READ)
		mshci_read_block_pio(host);
	else
		mshci_write_block_pio(host);

	DBG("PIO transfer complete.\n");
}

static void mshci_set_mdma_desc(u8 *desc_vir, u8 *desc_phy,
				u32 des0, u32 des1, u32 des2)
{
	((struct mshci_idmac *)(desc_vir))->des0 = des0;
	((struct mshci_idmac *)(desc_vir))->des1 = des1;
	((struct mshci_idmac *)(desc_vir))->des2 = des2;
	((struct mshci_idmac *)(desc_vir))->des3 = (u32)desc_phy +
					sizeof(struct mshci_idmac);
}

static int mshci_mdma_table_pre(struct mshci_host *host,
				struct mmc_data *data)
{
	int direction;

	u8 *desc_vir, *desc_phy;
	dma_addr_t addr;
	int len;

	struct scatterlist *sg;
	int i;
	u32 des_flag;
	u32 size_idmac = sizeof(struct mshci_idmac);

	if (data->flags & MMC_DATA_READ)
		direction = DMA_FROM_DEVICE;
	else
		direction = DMA_TO_DEVICE;

	host->sg_count = dma_map_sg(mmc_dev(host->mmc),
		data->sg, data->sg_len, direction);
	if (host->sg_count == 0)
		goto fail;

	desc_vir = host->idma_desc;

	/* to know phy address */
	host->idma_addr = dma_map_single(mmc_dev(host->mmc),
				host->idma_desc,
				DMA_SG_NUM * size_idmac,
				DMA_TO_DEVICE);
	if (dma_mapping_error(mmc_dev(host->mmc), host->idma_addr))
		goto unmap_entries;
	BUG_ON(host->idma_addr & 0x3);

	desc_phy = (u8 *)host->idma_addr;

	for_each_sg(data->sg, sg, host->sg_count, i) {
		addr = sg_dma_address(sg);
		len = sg_dma_len(sg);

		/* tran, valid */
		des_flag = (MSHCI_IDMAC_OWN | MSHCI_IDMAC_CH);
		des_flag |= (i == 0) ? MSHCI_IDMAC_FS : 0;

		mshci_set_mdma_desc(desc_vir, desc_phy, des_flag, len, addr);
		desc_vir += size_idmac;
		desc_phy += size_idmac;

		/* If this triggers then we have a calculation bug somewhere */
		WARN_ON((desc_vir - host->idma_desc) > DMA_SG_NUM * size_idmac);
	}

	/* Add a terminating flag */
	((struct mshci_idmac *)(desc_vir-size_idmac))->des0 |= MSHCI_IDMAC_LD;

	/* It has to dma map again to resync vir data to phy data */
	host->idma_addr = dma_map_single(mmc_dev(host->mmc),
				host->idma_desc,
				DMA_SG_NUM * size_idmac,
				DMA_TO_DEVICE);
	if (dma_mapping_error(mmc_dev(host->mmc), host->idma_addr))
		goto unmap_entries;
	BUG_ON(host->idma_addr & 0x3);

	return 0;

unmap_entries:
	dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len, direction);
fail:
	return -EINVAL;
}

static void mshci_idma_table_post(struct mshci_host *host,
				  struct mmc_data *data)
{
	int direction;

	if (data->flags & MMC_DATA_READ)
		direction = DMA_FROM_DEVICE;
	else
		direction = DMA_TO_DEVICE;

	dma_unmap_single(mmc_dev(host->mmc), host->idma_addr,
			 DMA_SG_NUM * sizeof(struct mshci_idmac), DMA_TO_DEVICE);

	dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len, direction);
}

static u32 mshci_calc_timeout(struct mshci_host *host, struct mmc_data *data)
{
	/* this value SHOULD be optimized */
	return 0xffffffff;
}

static void mshci_set_transfer_irqs(struct mshci_host *host)
{
	u32 uhs_reg;
	u32 dma_irqs = INTMSK_DMA;
	u32 pio_irqs = INTMSK_TXDR | INTMSK_RXDR;

	uhs_reg = mshci_readl(host, MSHCI_UHS_REG);

	if (host->flags & MSHCI_REQ_USE_DMA) {
		/* Next codes are the W/A for DDR */
		if ((uhs_reg & (0x1 << 16))
			&& (host->data->flags & MMC_DATA_WRITE))
			dma_irqs |= INTMSK_DCRC;

		/* clear interrupts for PIO */
		mshci_clear_set_irqs(host, dma_irqs, 0);
	} else {
		/* Next codes are the W/A for DDR */
		if ((uhs_reg & (0x1 << 16))
			&& (host->data->flags & MMC_DATA_WRITE))
			mshci_clear_set_irqs(host, INTMSK_DCRC, pio_irqs);
		else
			mshci_clear_set_irqs(host, 0, pio_irqs);
	}
}

static void mshci_prepare_data(struct mshci_host *host, struct mmc_data *data)
{
	u32 count;
#ifndef HI_MMC_OPT_WAITBUSY
	u32 ret;
#endif
	WARN_ON(host->data);

	if (data == NULL)
		return;

	BUG_ON(data->blksz * data->blocks > (host->mmc->max_req_size *
					host->mmc->max_segs));
	BUG_ON(data->blksz > host->mmc->max_blk_size);
	BUG_ON(data->blocks > 400000);

	host->data = data;
	host->data_early = 0;

	count = mshci_calc_timeout(host, data);
	mshci_writel(host, count, MSHCI_TMOUT);

	mshci_reset_fifo(host);

	if (host->flags & (MSHCI_USE_IDMA))
		host->flags |= MSHCI_REQ_USE_DMA;

	/*
	 * FIXME: This doesn't account for merging when mapping the
	 * scatterlist.
	 */
	if (host->flags & MSHCI_REQ_USE_DMA) {
		/*
		 *  mshc's IDMAC can't transfer data that is not aligned
		 * or has length not divided by 4 byte.
		 */
		int i;
		struct scatterlist *sg;

			for_each_sg(data->sg, sg, data->sg_len, i) {
				if (sg->length & 0x3) {
					DBG("Reverting to PIO because of "
						"transfer size (%d)\n",
						sg->length);
					host->flags &= ~MSHCI_REQ_USE_DMA;
					break;
			} else if (sg->offset & 0x3) {
				DBG("Reverting to PIO because of "
					"bad alignment\n");
				host->flags &= ~MSHCI_REQ_USE_DMA;
				break;
			}
		}
	}

#ifndef HI_MMC_OPT_WAITBUSY
	/* remove to mshci_send_command for optimised */
	if (host->flags & MSHCI_REQ_USE_DMA) {
		ret = mshci_mdma_table_pre(host, data);
		if (ret) {
			/*
			 * This only happens when someone fed
			 * us an invalid request.
			 */
			WARN_ON(1);
			host->flags &= ~MSHCI_REQ_USE_DMA;
		} else {
			mshci_writel(host, host->idma_addr,
				MSHCI_DBADDR);
		}
	}
#endif

	if (host->flags & MSHCI_REQ_USE_DMA) {
		/* enable DMA, IDMA interrupts and IDMAC */
		mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) |
					ENABLE_IDMAC|DMA_ENABLE), MSHCI_CTRL);
		mshci_writel(host, (mshci_readl(host, MSHCI_BMOD) |
					(BMOD_IDMAC_ENABLE|BMOD_IDMAC_FB)),
					MSHCI_BMOD);
		mshci_writel(host, INTMSK_IDMAC_ERROR, MSHCI_IDINTEN);
	}

	if (!(host->flags & MSHCI_REQ_USE_DMA)) {
		int flags;

		flags = SG_MITER_ATOMIC;
		if (host->data->flags & MMC_DATA_READ)
			flags |= SG_MITER_TO_SG;
		else
			flags |= SG_MITER_FROM_SG;

		sg_miter_start(&host->sg_miter, data->sg, data->sg_len, flags);
		host->blocks = data->blocks;

		printk(KERN_ERR "it starts transfer on PIO\n");
	}
	/* set transfered data as 0. this value only uses for PIO write */
	host->data_transfered = 0;
	mshci_set_transfer_irqs(host);

	mshci_writel(host, data->blksz, MSHCI_BLKSIZ);
	if(data->blksz <= 512)
		mshci_writel(host, data->blksz<<16|0x1, MSHCI_CARDTHRCTl);
	else
		mshci_writel(host, 512<<16|0x1, MSHCI_CARDTHRCTl);

	mshci_writel(host, (data->blocks * data->blksz), MSHCI_BYTCNT);
}

static u32 mshci_set_transfer_mode(struct mshci_host *host,
				   struct mmc_data *data)
{
	u32 ret = 0;

	if (data == NULL)
		return ret;

	WARN_ON(!host->data);

	/* this cmd has data to transmit */
	ret |= CMD_DATA_EXP_BIT;

	if (data->flags & MMC_DATA_WRITE)
		ret |= CMD_RW_BIT;
	if (data->flags & MMC_DATA_STREAM)
		ret |= CMD_TRANSMODE_BIT;

	return ret;
}

static void mshci_finish_data(struct mshci_host *host)
{
	struct mmc_data *data;

	BUG_ON(!host->data);

	data = host->data;
	host->data = NULL;

	if (host->flags & MSHCI_REQ_USE_DMA) {
		/* mshci_idma_table_post(host, data); */
		host->flags |= MSHCI_POST_UNMAP;
		/* disable IDMAC and DMA interrupt */
		mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) &
				~(DMA_ENABLE|ENABLE_IDMAC)), MSHCI_CTRL);

		/* mask all interrupt source of IDMAC */
		mshci_writel(host, 0x0, MSHCI_IDINTEN);
	}

	if (data->error) {
		mshci_reset_dma(host);
		data->bytes_xfered = 0;
	} else
		data->bytes_xfered = data->blksz * data->blocks;

	if (data->stop)
		mshci_send_command(host, data->stop);
	else
		tasklet_schedule(&host->finish_tasklet);
}

static void mshci_clock_onoff(struct mshci_host *host, bool val)
{
	u32 loop_count;
	u32 reg = 0;

	loop_count = 0x100000;

	if (val) {
        /* use internal clock gating */
        if (host->clock_gate) {
            mshci_writel(host, (0x1<<0 | 0x1<<16), MSHCI_CLKENA);
        }
        else
			mshci_writel(host, (0x1<<0), MSHCI_CLKENA);

		mshci_writel(host, 0, MSHCI_CMD);
		mshci_writel(host,INTMSK_HLE,MSHCI_RINTSTS);
		mshci_writel(host, CMD_ONLY_CLK, MSHCI_CMD);
		do {
			if (!(mshci_readl(host, MSHCI_CMD) & CMD_STRT_BIT))
				break;
			loop_count--;
		} while (loop_count);
	} else {
		mshci_writel(host, (0x0<<0), MSHCI_CLKENA);
		mshci_writel(host, 0, MSHCI_CMD);
		mshci_writel(host,INTMSK_HLE,MSHCI_RINTSTS);
		mshci_writel(host, CMD_ONLY_CLK, MSHCI_CMD);
		do {
			if (!(mshci_readl(host, MSHCI_CMD) & CMD_STRT_BIT))
				break;
			loop_count--;
		} while (loop_count);
	}

	reg = mshci_readl(host, MSHCI_RINTSTS);
	if (reg & INTMSK_HLE) {
		printk(KERN_ERR "%s: HLE in Clock %s. RINTSTS = 0x%x\n "
				, mmc_hostname(host->mmc), val ? "ON" : "OFF", reg);
		mshci_writel(host, 0, MSHCI_CMD);
		mshci_writel(host,INTMSK_HLE,MSHCI_RINTSTS);
	} else if (loop_count == 0) {
		printk(KERN_ERR "%s: Clock %s has been failed.\n ",
				mmc_hostname(host->mmc), val ? "ON" : "OFF");
	}

	mshci_writel(host, 0, MSHCI_CMD);
}

static void mshci_send_command(struct mshci_host *host, struct mmc_command *cmd)
{
	int flags, ret;
	int ctype_flags, uhs_flags;


	WARN_ON(host->cmd);

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		del_timer(&host->clock_timer);
		if ((host->pclk != NULL) && (host->clk_ref_counter == CLK_DISABLED)) {
#ifdef CONFIG_MMC_SD_HUB
			if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
		        SDHUB_clock_enable();
		        }
#endif            
			if (clk_enable(host->pclk))
				printk(KERN_ERR "%s:%s:clk_enable pclk failed\n",
					mmc_hostname(host->mmc), __func__);
			host->clk_ref_counter = CLK_ENABLED;
		}
		if (host->clock_to_restore != 0 && host->clock == 0)
			mshci_set_clock(host, host->clock_to_restore);
	}

	/* disable interrupt before issuing cmd to the card. */
	mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) & ~INT_ENABLE),
					MSHCI_CTRL);

	mod_timer(&host->timer, jiffies + 10 * HZ);

	host->cmd = cmd;

	mshci_prepare_data(host, cmd->data);

	mshci_writel(host, cmd->arg, MSHCI_CMDARG);

	flags = mshci_set_transfer_mode(host, cmd->data);

	if ((cmd->flags & MMC_RSP_136) && (cmd->flags & MMC_RSP_BUSY)) {
		printk(KERN_ERR "%s: Unsupported response type!\n",
			mmc_hostname(host->mmc));
		cmd->error = -EINVAL;
		tasklet_schedule(&host->finish_tasklet);
		return;
	}

	if (cmd->flags & MMC_RSP_PRESENT) {
		flags |= CMD_RESP_EXP_BIT;
		if (cmd->flags & MMC_RSP_136)
			flags |= CMD_RESP_LENGTH_BIT;
	}
	if (cmd->flags & MMC_RSP_CRC)
		flags |= CMD_CHECK_CRC_BIT;

#if defined(CONFIG_MACH_K3V2OEM1)
	flags |= (cmd->opcode | CMD_STRT_BIT | CMD_WAIT_PRV_DAT_BIT | CMD_USE_HOLD_REG);
#else
	flags |= (cmd->opcode | CMD_STRT_BIT | CMD_WAIT_PRV_DAT_BIT);
#endif

	if (MMC_STOP_TRANSMISSION == cmd->opcode) {
		flags &= ~CMD_WAIT_PRV_DAT_BIT;
		flags |= CMD_STOP_ABORT_CMD;
	}

	if (host->flags & MSHCI_CMD_SEND_INIT) {
		flags |= CMD_SEND_INITIALIZATION;
		host->flags &= ~MSHCI_CMD_SEND_INIT;
	}

	/* 
	 * m53980:
	 * this cmd is cmd11 for voltage switch
	 * disable clock low power first, then send the cmd
	 */
	if (cmd->flags & MMC_CMD_VOLT_SWITCH) {
		int loop_count;
		flags |= CMD_VOLT_SWITCH;

		/* disable clock low power */
		mshci_writel(host, (0x1<<0), MSHCI_CLKENA);
		mshci_writel(host, CMD_ONLY_CLK | CMD_VOLT_SWITCH, MSHCI_CMD);
		loop_count = 0x100000;
		do {
			if (!(mshci_readl(host, MSHCI_CMD) & CMD_STRT_BIT))
				break;
			loop_count--;
		} while (loop_count);
		if (!loop_count) 
			printk(KERN_ERR "%s: disable clk low power failed in volt_switch\n"
				, mmc_hostname(host->mmc));

	}

	ret = mshci_readl(host, MSHCI_CMD);
	if (ret & CMD_STRT_BIT) {
		printk(KERN_ERR "CMD busy. current cmd %d. last cmd reg 0x%x\n",
			cmd->opcode, ret);
		ctype_flags = mshci_readl(host, MSHCI_CTYPE);
		uhs_flags = mshci_readl(host, MSHCI_UHS_REG);
		mshci_reinit(host);
		host->clock_to_restore = host->clock;
		host->clock = 0;
		mshci_set_clock(host, host->clock_to_restore);
		mshci_writel(host, ctype_flags, MSHCI_CTYPE);
		mshci_writel(host, uhs_flags, MSHCI_UHS_REG);
	}

	mshci_writel(host, flags, MSHCI_CMD);

	/* enable interrupt upon it sends a command to the card. */
	mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) | INT_ENABLE),
					MSHCI_CTRL);
}

static void mshci_finish_command(struct mshci_host *host)
{
	int i;

	BUG_ON(host->cmd == NULL);

	if (host->cmd->flags & MMC_RSP_PRESENT) {
		if (host->cmd->flags & MMC_RSP_136) {
			/* response data are overturned */
			for (i = 0; i < 4; i++) {
				host->cmd->resp[0] = mshci_readl(host,
								MSHCI_RESP3);
				host->cmd->resp[1] = mshci_readl(host,
								MSHCI_RESP2);
				host->cmd->resp[2] = mshci_readl(host,
								MSHCI_RESP1);
				host->cmd->resp[3] = mshci_readl(host,
								MSHCI_RESP0);
			}
		} else {
			host->cmd->resp[0] = mshci_readl(host, MSHCI_RESP0);
		}
	}

	host->cmd->error = 0;

	/* if data interrupt occurs earlier than command interrupt */
	if (host->data && host->data_early)
		mshci_finish_data(host);

	if (!host->cmd->data)
		tasklet_schedule(&host->finish_tasklet);

	host->cmd = NULL;
}

static void mshci_set_clock(struct mshci_host *host, unsigned int clock)
{
	int div;
	u32 loop_count, reg;

	if (clock == host->clock)
		return;

	/* before changing clock. clock needs to be off */
	mshci_clock_onoff(host, CLK_DISABLE);

	/* printk(KERN_INFO "%s: Changing clock %d\n", mmc_hostname(host->mmc), clock); */

	if (clock == 0)
		goto out;

	if (clock >= host->max_clk) {
		div = 0;
	} else {
		for (div = 1; div < 255; div++) {
			if ((host->max_clk / (div<<1)) <= clock)
				break;
		}
	}

	/* printk(KERN_INFO "%s: clock source: %d, div: %d\n", mmc_hostname(host->mmc),
			host->max_clk, div);
	*/

	loop_count = 0x100000;

	mshci_writel(host, div, MSHCI_CLKDIV);
	mshci_writel(host, 0, MSHCI_CLKSRC);
	mshci_writel(host, 0, MSHCI_CMD);
	mshci_writel(host,INTMSK_HLE,MSHCI_RINTSTS);
	mshci_writel(host, CMD_ONLY_CLK, MSHCI_CMD);

	do {
		if (!(mshci_readl(host, MSHCI_CMD) & CMD_STRT_BIT))
			break;
		loop_count--;
	} while (loop_count);

	reg = mshci_readl(host, MSHCI_RINTSTS);
	if (reg & INTMSK_HLE) {
		printk(KERN_ERR "%s: HLE in Changing clock. RINTSTS = 0x%x\n "
				, mmc_hostname(host->mmc), reg);
		mshci_writel(host, 0, MSHCI_CMD);
		mshci_writel(host,INTMSK_HLE,MSHCI_RINTSTS);
	} else if (loop_count == 0) {
		printk(KERN_ERR "%s: Changing clock has been failed.\n "
				, mmc_hostname(host->mmc));
	}

	mshci_writel(host, 0, MSHCI_CMD);

	mshci_clock_onoff(host, CLK_ENABLE);

out:
	host->clock = clock;
}

static void mshci_set_power(struct mshci_host *host, unsigned short power)
{
	u8 pwr = power;

	if (power == (unsigned short)-1)
		pwr = 0;

	//if (host->pwr == pwr)
	//	return;

	host->pwr = pwr;

	if (pwr == 0)
		mshci_writel(host, 0, MSHCI_PWREN);
	else
		mshci_writel(host, 0x1, MSHCI_PWREN);
}


static void mmc_tuning_wait_done(struct mmc_request *mrq)
{
	complete(mrq->done_data);
}

static int mshci_execute_app_command(struct mmc_host *mmc)
{
	struct mmc_command app_cmd = {0};
	struct mmc_request app_mrq = {0};
	struct mshci_host *host;

	DECLARE_COMPLETION_ONSTACK(app_cmd_complete);

	host = mmc_priv(mmc);
	app_cmd.opcode = MMC_APP_CMD;
	if (mmc->card) {
		app_cmd.arg = mmc->card->rca << 16;
		app_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	} else {
		app_cmd.arg = host->tuning_rca << 16;
		app_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_BCR;
	}

	app_cmd.retries = 0;

	app_mrq.cmd = &app_cmd;
	app_cmd.data = NULL;

	app_mrq.done_data = &app_cmd_complete;
	app_mrq.done = mmc_tuning_wait_done;

	app_cmd.error = 0;
	app_cmd.mrq = &app_mrq;

	mshci_request(mmc, &app_mrq);

	wait_for_completion(&app_cmd_complete);

	return app_cmd.error;
}

/* set pattern at partition:"-" */
#define EMMC_PATTERN_ADDRESS (384*2)

static int mshci_execute_tuning(struct mmc_host *mmc)
{
	struct mshci_host *host;
	int type;
	struct mmc_command cmd = {0};
	struct mmc_command stop_cmd = {0};
	struct mmc_request mrq = {0};
	struct mmc_request stop_mrq = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;
	void *data_buf;
	unsigned len = 512;
	unsigned acmd = 0; /* if tuning cmd is acmd, set 1 */
	int ret = 0;
	int err = 0;

	DECLARE_COMPLETION_ONSTACK(tuning_complete);

	host = mmc_priv(mmc);

	printk(KERN_ERR "%s: mshci_execute_tuning ++\n", mmc_hostname(host->mmc));

	data_buf = kmalloc(len, GFP_KERNEL);
	if (data_buf == NULL)
		return -ENOMEM;

	mrq.sbc = NULL;
	mrq.cmd = &cmd;
	mrq.data = &data;
	mrq.stop = NULL;
	mrq.done_data = &tuning_complete;
	mrq.done = mmc_tuning_wait_done;

	cmd.retries = 0;
	cmd.error = 0;
	cmd.data = mrq.data;
	cmd.mrq = &mrq;

	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;
	data.error = 0;
	data.stop = mrq.stop;
	data.mrq = &mrq;

	stop_mrq.sbc = NULL;
	stop_mrq.cmd = &stop_cmd;
	stop_mrq.data = NULL;
	stop_mrq.stop = NULL;
	stop_mrq.done_data = &tuning_complete;
	stop_mrq.done = mmc_tuning_wait_done;

	stop_cmd.retries = 0;
	stop_cmd.error = 0;
	stop_cmd.data = NULL;
	stop_cmd.mrq = &stop_mrq;
	stop_cmd.opcode = MMC_STOP_TRANSMISSION;
	stop_cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;


	if (mmc->card) {
		type = mmc->card->type;
	} else {
		printk(KERN_WARNING "%s: tuning: mmc->card == NULL\n",
							mmc_hostname(host->mmc));

		if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
			type = MMC_TYPE_SD;
		} else if (host->quirks & MSHCI_QUIRK_BROKEN_CARD_DETECTION) {
			type = MMC_TYPE_MMC;
		} else {
			type = MMC_TYPE_SDIO;
		}
	}

	printk(KERN_WARNING "%s: tuning type = %d\n",
						mmc_hostname(host->mmc), type);

	switch (type) {
	case MMC_TYPE_MMC:
		/* read the pattern for tuning */
		cmd.opcode = MMC_READ_SINGLE_BLOCK;
		if (1 == host->hw_mmc_id)
			cmd.arg = EMMC_PATTERN_ADDRESS; /* set the pattern block address */
		else
			cmd.arg = 0; 					/* MMC card read frist block */

		cmd.flags = MMC_RSP_R1 | MMC_RSP_CRC | MMC_CMD_ADTC;

		data.blksz = 512;
		break;

	case MMC_TYPE_SD:
		/* FIXME: Because CMD19 is optional for SDR50, don't use CMD19 for tuning SDR50*/
		if ((MMC_TIMING_UHS_SDR104 == mmc->ios.timing) ||
			(MMC_TIMING_UHS_SDR50 == mmc->ios.timing)) {
			cmd.opcode = MMC_SEND_TUNING_BLOCK;
			cmd.arg = 0;
			cmd.flags = MMC_RSP_R1 | MMC_RSP_CRC | MMC_CMD_ADTC;

			data.blksz = 64;
		} else {
#if 0
			/* use acmd13 for tuning */
			acmd = 1;
			cmd.opcode = SD_APP_SD_STATUS;

			data.blksz = 64;
#else
			/* read the first block for tuning */
			cmd.opcode = MMC_READ_SINGLE_BLOCK;
			data.blksz = 512;
#endif
			cmd.arg = 0;
			cmd.flags = MMC_RSP_R1 | MMC_RSP_CRC | MMC_CMD_ADTC;
		}
		break;

	case MMC_TYPE_SDIO: /* FIXME: need confirm */
		cmd.opcode = SD_IO_RW_EXTENDED;
		cmd.arg = 0x200004;
		cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;

		data.blksz = 4;
		data.timeout_ns = 1000000000;
		data.timeout_clks = 0;
		break;

	case MMC_TYPE_SD_COMBO:
		printk(KERN_INFO "%s:tuning: no rule to tuning MMC_TYPE_SD_COMBO\n",
						mmc_hostname(host->mmc));
		goto out;
	default:
		printk(KERN_INFO "%s:tuning: undefined card type\n",
						mmc_hostname(host->mmc));
		goto out;
	}

	if (data.blksz > len)
		data.blksz = len;

	sg_init_one(&sg, data_buf, data.blocks * data.blksz);

	/* this timeout setting may not be called */
	if (mmc->card) {
		mmc_set_data_timeout(&data, mmc->card);
	}

	host->flags |= MSHCI_IN_TUNING;
	host->flags &= ~MSHCI_TUNING_DONE;

	while (1) {
		if ((!host->ops->tuning_find_condition) || (!host->ops->tuning_set_current_state)) {
			printk(KERN_ERR "%s:tuning: no tuning find condition method\n",
							mmc_hostname(host->mmc));
			return 0;
		}
		ret = host->ops->tuning_find_condition(host);
		if (1 != ret)
			break;
		cmd.error = 0;
		data.error = 0;
		if (acmd) {
			err = mshci_execute_app_command(mmc);
			if (err) {
				if (-ENOMEDIUM == err) {
					printk(KERN_ERR "%s:tuning: send app command no card\n",
									mmc_hostname(host->mmc));
					break;
				}

				host->ops->tuning_set_current_state(host, 0);
				mshci_request(mmc, &stop_mrq);
				wait_for_completion(&tuning_complete);

				continue;
			}
		}
		mshci_request(mmc, &mrq);

		wait_for_completion(&tuning_complete);

		if (cmd.error != 0 || data.error != 0) {
			host->ops->tuning_set_current_state(host, 0);
			if (-ENOMEDIUM == cmd.error) {
				printk(KERN_ERR "%s:tuning: tuning no card\n",
										mmc_hostname(host->mmc));
				ret = -ENOMEDIUM;
				break;
			}
			mshci_request(mmc, &stop_mrq);
			wait_for_completion(&tuning_complete);
		} else {
			host->ops->tuning_set_current_state(host, 1);
		}
	}

	host->flags &= ~MSHCI_IN_TUNING;
	if (!ret) {
		host->flags |= MSHCI_TUNING_DONE;
	}

	host->tuning_move_start = 1;

	printk(KERN_ERR "%s: mshci_execute_tuning --\n", mmc_hostname(host->mmc));

out:
	kfree(data_buf);
	return ret;
}


/* MMC callbacks */
static void mshci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct mshci_host *host;
	bool present;
	unsigned long flags;
	unsigned long timeout;
	int ret;
	/*2012-02-11-liuchang(l00198597)-added-begin*/
	u32 intmask;
	/*2012-02-11-liuchang(l00198597)-added-end*/
	host = mmc_priv(mmc);
	if (host->quirks & MSHCI_QUIRK_WLAN_DETECTION) {
		if (mmc->ios.power_mode == MMC_POWER_OFF)
			printk(KERN_ERR "receive wifi cmd on off state:0x%x\n", mrq->cmd->opcode);
	}

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			del_timer_sync(&host->clock_timer);
		}
	}

	spin_lock_irqsave(&host->lock, flags);

	WARN_ON(host->mrq != NULL);

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if ((host->pclk != NULL) && (host->clk_ref_counter == CLK_DISABLED)) {
#ifdef CONFIG_MMC_SD_HUB
		    if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
		        SDHUB_clock_enable();
		    }
#endif
			if (clk_enable(host->pclk))
				printk(KERN_ERR "%s:%s:clk_enable pclk failed\n",
					mmc_hostname(host->mmc), __func__);
			host->clk_ref_counter = CLK_ENABLED;
		}
	}
	host->working = MMC_HOST_BUSY;
	host->mrq = mrq;
	/* If polling, assume that the card is always present. */
	if (host->quirks & MSHCI_QUIRK_BROKEN_CARD_DETECTION ||
			host->quirks & MSHCI_QUIRK_BROKEN_PRESENT_BIT)
		present = true;
	else if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION &&
			host->ops->get_present_status)
		present = !(host->ops->get_present_status(host));
	else
		present = !(mshci_readl(host, MSHCI_CDETECT) & CARD_PRESENT);

	if (!present || host->flags & MSHCI_DEVICE_DEAD ||
			(host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION &&
			host->flags & MSHCI_CARD_NEED_ENUM)) {
		host->flags &= ~MSHCI_TUNING_DONE;
		if(host->mrq && host->mrq->cmd)
			host->mrq->cmd->error = -ENOMEDIUM;
		tasklet_schedule(&host->finish_tasklet);
	} else {
		/* Wait max 2s */
		timeout = 200000;

		#ifdef HI_MMC_OPT_WAITBUSY
		/* copy from mshci_prepare_data for optimised */
		if (mrq->cmd->data) {
			if (host->flags & (MSHCI_USE_IDMA))
				host->flags |= MSHCI_REQ_USE_DMA;

			if (host->flags & MSHCI_REQ_USE_DMA) {
				spin_unlock_irqrestore(&host->lock, flags);
				ret = mshci_mdma_table_pre(host, mrq->cmd->data);
				spin_lock_irqsave(&host->lock, flags);
				if (ret) {
					/*
					 * This only happens when someone fed
					 * us an invalid request.
					 */
					WARN_ON(1);
					host->flags &= ~MSHCI_REQ_USE_DMA;
				} else {
					mshci_writel(host, host->idma_addr,
						MSHCI_DBADDR);
				/*2012-02-11-liuchang(l00198597)-added-begin*/
					intmask = mshci_readl(host, MSHCI_IDSTS);
					intmask = ((intmask >>13) & 0xf);
					if(1 == intmask)
					mshci_writel(host, 0x1,MSHCI_PLDMND);
				/*2012-02-11-liuchang(l00198597)-added-end*/
				}
			}
		}
		#endif
		mmiowb();
		spin_unlock_irqrestore(&host->lock, flags);
		/*
		 * We shouldn't wait for data inihibit for stop commands,
		 * even though they might use busy signaling
		 */
		while (mshci_readl(host, MSHCI_STATUS) & (1 << 9)) {
			if (timeout == 0) {
				printk(KERN_ERR "%s: Controller never released "
					"data0.\n", mmc_hostname(host->mmc));
				mshci_dumpregs(host);
				mrq->cmd->error = -EIO;
				tasklet_schedule(&host->finish_tasklet);
				return;
			}
			timeout--;
			udelay(10);
		}

		spin_lock_irqsave(&host->lock, flags);

		if (host->quirks & MSHCI_QUIRK_BROKEN_CARD_DETECTION ||
				host->quirks & MSHCI_QUIRK_BROKEN_PRESENT_BIT)
			present = true;
		else if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION &&
				host->ops->get_present_status)
			present = !(host->ops->get_present_status(host));
		else
			present = !(mshci_readl(host, MSHCI_CDETECT) & CARD_PRESENT);

		if (!present || host->flags & MSHCI_DEVICE_DEAD||
			(host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION &&
			host->flags & MSHCI_CARD_NEED_ENUM)) {
			host->flags &= ~MSHCI_TUNING_DONE;
			if(host->mrq && host->mrq->cmd)
				host->mrq->cmd->error = -ENOMEDIUM;
			tasklet_schedule(&host->finish_tasklet);
		} else {
			mshci_send_command(host, mrq->cmd);
		}
	}
	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);
}

static void mshci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct mshci_host *host;
	unsigned long flags;

	host = mmc_priv(mmc);
	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			del_timer_sync(&host->clock_timer);
		}
	}

	spin_lock_irqsave(&host->lock, flags);

	host->working = MMC_HOST_BUSY;
	if ((host->flags & MSHCI_DEVICE_DEAD)&&(!(host->quirks & MSHCI_QUIRK_WLAN_DETECTION)))
		goto out;

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if ((host->pclk != NULL) && (host->clk_ref_counter == CLK_DISABLED)) {
#ifdef CONFIG_MMC_SD_HUB
			if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
		        SDHUB_clock_enable();
		        }
#endif                     
			if (clk_enable(host->pclk))
				printk(KERN_ERR "%s:%s:clk_enable pclk failed\n",
					mmc_hostname(host->mmc), __func__);
			host->clk_ref_counter = CLK_ENABLED;
		}
	}


	if (ios->power_mode == MMC_POWER_OFF){
		mshci_reinit(host);
		if(host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION){
			host->flags &= ~MSHCI_CARD_NEED_ENUM;
		}
	}

	if (ios->power_mode == MMC_POWER_UP) {
		host->flags |= MSHCI_CMD_SEND_INIT;
	}
	/* 
	 * m53980:
	 * A trick to change clock source.
	 * Pls refer to the imlementation of function mshci_hi_set_ios in himci.c.
	 */
	mshci_set_clock(host, ios->clock);

	spin_unlock_irqrestore(&host->lock, flags);
	if (host->ops->set_ios)
		host->ops->set_ios(host, ios);
	spin_lock_irqsave(&host->lock, flags);

	mmc->f_min = host->max_clk / 510;
	mmc->f_max = host->max_clk;	

	if (ios->power_mode == MMC_POWER_OFF)
		mshci_set_power(host, -1);
	else
		//mshci_set_power(host, ios->vdd);
		mshci_set_power(host, 1);

	if (ios->bus_width == MMC_BUS_WIDTH_8) {
		mshci_writel(host, (0x1<<16), MSHCI_CTYPE);
		//mshci_writel(host, 0, MSHCI_UHS_REG);
	} else if (ios->bus_width == MMC_BUS_WIDTH_4) {
		mshci_writel(host, (0x1<<0), MSHCI_CTYPE);
		//mshci_writel(host, 0, MSHCI_UHS_REG);
	} else if (ios->bus_width == MMC_BUS_WIDTH_8_DDR) {
		mshci_writel(host, (0x1<<16), MSHCI_CTYPE);
		//mshci_writel(host, (0x1<<16), MSHCI_UHS_REG);
	} else if (ios->bus_width == MMC_BUS_WIDTH_4_DDR) {
		mshci_writel(host, (0x1<<0), MSHCI_CTYPE);
		//mshci_writel(host, (0x1<<16), MSHCI_UHS_REG);
	} else {
		mshci_writel(host, 0, MSHCI_CTYPE);
		//mshci_writel(host, 0, MSHCI_UHS_REG);
	}

	if (MMC_TIMING_UHS_DDR50 == ios->timing) {
		mshci_writel(host, (0x1<<16), MSHCI_UHS_REG);
	} else {
		mshci_writel(host, 0, MSHCI_UHS_REG);
	}

out:
	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			mod_timer(&host->clock_timer, jiffies + msecs_to_jiffies(DELAY_TIME));
		}
	}

	mmiowb();
	host->working = MMC_HOST_FREE;
	spin_unlock_irqrestore(&host->lock, flags);
}

static int mshci_get_ro(struct mmc_host *mmc)
{
	struct mshci_host *host;
	unsigned long flags;
	int wrtprt;

	host = mmc_priv(mmc);

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			del_timer_sync(&host->clock_timer);
		}
	}

	spin_lock_irqsave(&host->lock, flags);
	host->working = MMC_HOST_BUSY;
	if (host->quirks & MSHCI_QUIRK_ALWAYS_WRITABLE)
		wrtprt = 0;
	else if (host->quirks & MSHCI_QUIRK_NO_WP_BIT)
		wrtprt = host->ops->get_ro(mmc) ? 0 : WRTPRT_ON;
	else if (host->flags & MSHCI_DEVICE_DEAD)
		wrtprt = 0;
	else {
		if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
			if ((host->pclk != NULL) && (host->clk_ref_counter == CLK_DISABLED)) {
#ifdef CONFIG_MMC_SD_HUB
			if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
		        SDHUB_clock_enable();
		        }
#endif         
                
				if (clk_enable(host->pclk))
					printk(KERN_ERR "%s:%s:clk_enable pclk failed\n",
						mmc_hostname(host->mmc), __func__);
				host->clk_ref_counter = CLK_ENABLED;
			}
		}

		wrtprt = mshci_readl(host, MSHCI_WRTPRT);

		if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
			if (&host->clock_timer != NULL) {
				mod_timer(&host->clock_timer, jiffies + msecs_to_jiffies(DELAY_TIME));
			}
		}
	}
	host->working = MMC_HOST_FREE;
	spin_unlock_irqrestore(&host->lock, flags);

	return wrtprt & WRTPRT_ON;
}

static void mshci_delete_timer(struct work_struct *work)
{
	struct mshci_host *host =
                container_of(work, struct mshci_host, mshci_delete_timer_work);

	if (host && &host->clock_timer)
		del_timer_sync(&host->clock_timer);
}

static void mshci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct mshci_host *host;
	unsigned long flags;

	host = mmc_priv(mmc);

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			schedule_work(&host->mshci_delete_timer_work);
		}
	}
	spin_lock_irqsave(&host->lock, flags);

	host->working = MMC_HOST_BUSY;
	if (host->flags & MSHCI_DEVICE_DEAD)
		goto out;

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if ((host->pclk != NULL) && (host->clk_ref_counter == CLK_DISABLED)) {
#ifdef CONFIG_MMC_SD_HUB
			if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
		        SDHUB_clock_enable();
		        }
#endif                     
			if (clk_enable(host->pclk))
				printk(KERN_ERR "%s:%s:clk_enable pclk failed\n",
					mmc_hostname(host->mmc), __func__);
			host->clk_ref_counter = CLK_ENABLED;
		}
	}

	if (enable) {
		mshci_unmask_irqs(host, SDIO_INT_ENABLE);
		/* enable interrupt */
		mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) | INT_ENABLE),
					MSHCI_CTRL);
	}
	else
		mshci_mask_irqs(host, SDIO_INT_ENABLE);

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			mod_timer(&host->clock_timer, jiffies + msecs_to_jiffies(DELAY_TIME));
		}
	}
out:
	mmiowb();
	host->working = MMC_HOST_FREE;
	spin_unlock_irqrestore(&host->lock, flags);
}

static int mshci_get_cd(struct mmc_host *mmc)
{
	struct mshci_host *host;
    int ret;
	host = mmc_priv(mmc);
#ifdef CONFIG_MMC_SD_HUB
    if(host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
        printk("CPRM : %s : In %s flag_connection = %d\n", mmc_hostname(host->mmc), __func__, flag_connection);
    	if( flag_connection == 0 ) {
            if (host->ops->get_present_status) {
                ret = host->ops->get_present_status(host);  // ret=1 卡不在位；ret=0卡在位
             	if (!ret) {
            		SDHUB_Connect( mmc, SDHUB_DEVICE_ID );
                } else {
                    if (SDHUB_Set_LowPower_Mode()) {
        				printk(KERN_ERR "%s:SDHUB_Set_LowPower_Mode Failed\n", __func__);
                    }                        
                }                
            }
    		flag_connection = 1 ;
    	}
        return !ret;
    }
#endif /* CONFIG_MMC_SD_HUB */	

	if ((host->quirks & MSHCI_QUIRK_WLAN_DETECTION)
		&& (host->flags & MSHCI_DEVICE_DEAD))
		return 0;
	if (host->ops->get_present_status)
		return !host->ops->get_present_status(host);
	else
		return 1;
}

static int mshci_start_signal_voltage_switch(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct mshci_host *host;
	unsigned long loop_count = 0x100000;
	int ret = -1;
	int intrs;
	unsigned long flags;
	
	host = mmc_priv(mmc);

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			del_timer_sync(&host->clock_timer);
		}
		if ((host->pclk != NULL) && (host->clk_ref_counter == CLK_DISABLED)) {
#ifdef CONFIG_MMC_SD_HUB
			if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
		        SDHUB_clock_enable();
		        }
#endif                     
			if (clk_enable(host->pclk))
				printk(KERN_ERR "%s:%s:clk_enable pclk failed\n",
					mmc_hostname(host->mmc), __func__);
			host->clk_ref_counter = CLK_ENABLED;
		}
	}
	spin_lock_irqsave(&host->lock, flags);
	host->working = MMC_HOST_BUSY;
	spin_unlock_irqrestore(&host->lock, flags);
	/* 
	 * m53980:
	 * A way to handle voltage switch.
	 * This is the second half.
	 * Mobile storage IP hava a unique implementation to voltage switch,
	 * it will generate interrupt INTMSK_VOLT_SWITCH first, generate cd 
	 * interrupt after voltage switch successfully.
	 * So in this second half, we stop clock, switch voltage, start clock and handle
	 * cd interrtup.
	 * The first half is inplemented in function mshci_request and mshci_irq
	 */

	/* voltage switch only happen to MMC_SIGNAL_VOLTAGE_180 */
	if (ios->signal_voltage != MMC_SIGNAL_VOLTAGE_330) {

		/* disable interrupt upon voltage switch. handle interrupt here 
		   and DO NOT triggle irq */
		mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) & ~INT_ENABLE),
					MSHCI_CTRL);

		/* stop clock */
		mshci_writel(host, (0x0<<0), MSHCI_CLKENA);
		mshci_writel(host, CMD_ONLY_CLK | CMD_VOLT_SWITCH, MSHCI_CMD);
		do {
			if (!(mshci_readl(host, MSHCI_CMD) & CMD_STRT_BIT))
				break;
			loop_count--;
		} while (loop_count);

		if (!loop_count) 
			printk(KERN_ERR "%s: disable clock failed in voltage_switch\n"
				, mmc_hostname(host->mmc));	

		mmiowb();
	}
	
	if (host->ops->start_signal_voltage_switch)
		ret = host->ops->start_signal_voltage_switch(host, ios);

	usleep_range(5000, 5500);

	if (ios->signal_voltage != MMC_SIGNAL_VOLTAGE_330) {

		/* voltage switch cont. */

		/* start clock */
		mshci_writel(host, (0x1<<0), MSHCI_CLKENA);
		mshci_writel(host, CMD_ONLY_CLK | CMD_VOLT_SWITCH, MSHCI_CMD);
		loop_count = 0x100000;
		do {
			if (!(mshci_readl(host, MSHCI_CMD) & CMD_STRT_BIT))
				break;
			loop_count--;
		} while (loop_count);

		if (!loop_count) 
			printk(KERN_ERR "%s: enable clock failed in voltage_switch\n"
				, mmc_hostname(host->mmc));

		/* poll cd interrupt */
		loop_count = 0x100000;
		do {
			intrs = mshci_readl(host, MSHCI_RINTSTS); 
			if (intrs & INTMSK_CDONE) {
				printk(KERN_ERR "%s: cd 0x%x in voltage_switch\n",
					 mmc_hostname(host->mmc), intrs);	
				mshci_writel(host, intrs, MSHCI_RINTSTS);
				break;
			}
			loop_count--;
		} while (loop_count);		

		if (!loop_count) 
			printk(KERN_ERR "%s: poll cd failed in voltage_switch\n"
				, mmc_hostname(host->mmc));
				
		/* enable interrupt */
		mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) | INT_ENABLE),
					MSHCI_CTRL);			
	
		mmiowb();
	}
	spin_lock_irqsave(&host->lock, flags);
	host->working = MMC_HOST_FREE;
	spin_unlock_irqrestore(&host->lock, flags);
	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			mod_timer(&host->clock_timer, jiffies + msecs_to_jiffies(DELAY_TIME));
		}
	}

	return ret;
}

static struct mmc_host_ops mshci_ops = {
	.request	= mshci_request,
	.set_ios	= mshci_set_ios,
	.get_ro		= mshci_get_ro,
	.enable_sdio_irq = mshci_enable_sdio_irq,
	.get_cd     = mshci_get_cd,
	.start_signal_voltage_switch = mshci_start_signal_voltage_switch,
	.panic_probe = raw_mmc_panic_probe,
	.panic_write = raw_mmc_panic_write,
	.panic_erase = raw_mmc_panic_erase,
	.execute_tuning = mshci_execute_tuning,
};

static struct mmc_host_ops no_tuning_mshci_ops = {
	.request	= mshci_request,
	.set_ios	= mshci_set_ios,
	.get_ro		= mshci_get_ro,
	.enable_sdio_irq = mshci_enable_sdio_irq,
	.get_cd     = mshci_get_cd,
	.start_signal_voltage_switch = mshci_start_signal_voltage_switch,
	.panic_probe = raw_mmc_panic_probe,
	.panic_write = raw_mmc_panic_write,
	.panic_erase = raw_mmc_panic_erase,
	.execute_tuning = NULL,
};

/* Tasklets */

static void mshci_tasklet_card(unsigned long param)
{
	struct mshci_host *host;
	unsigned long flags;
	unsigned int present;

	unsigned int count;

	host = (struct mshci_host *)param;


	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			del_timer_sync(&host->clock_timer);
		}
	}

	spin_lock_irqsave(&host->lock, flags);
	host->working = MMC_HOST_BUSY;
	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if ((host->pclk != NULL) && (host->clk_ref_counter == CLK_DISABLED)) {
#ifdef CONFIG_MMC_SD_HUB
			if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
		        SDHUB_clock_enable();
		        }
#endif                     
			if (clk_enable(host->pclk))
				printk(KERN_ERR "%s:%s:clk_enable pclk failed\n",
					mmc_hostname(host->mmc), __func__);
			host->clk_ref_counter = CLK_ENABLED;
		}
	}

	if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION &&
			host->ops->get_present_status)
		present = !(host->ops->get_present_status(host));
	else if (host->quirks & MSHCI_QUIRK_WLAN_DETECTION)
		/* SDIO set MSHCI_QUIRK_WLAN_DETECTION to
			use MSHCI_DEVICE_DEAD notify the change */
		present = 1;
	else
		present = !(mshci_readl(host, MSHCI_CDETECT) & CARD_PRESENT);

	if (!present) {
		host->flags &= ~MSHCI_TUNING_DONE;
	}

	if (host->mrq) {
		if ((!present) || (host->flags & MSHCI_DEVICE_DEAD)) {
			printk(KERN_ERR "%s: Card removed during transfer!\n",
				mmc_hostname(host->mmc));
			printk(KERN_ERR "%s: Resetting controller.\n",
				mmc_hostname(host->mmc));

			host->mrq->cmd->error = -ENOMEDIUM;
			tasklet_schedule(&host->finish_tasklet);
		}
	}

	if((host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) && (!present)){

		host->flags |= MSHCI_CARD_NEED_ENUM;

		mshci_init(host);

		/* set bus mode register for IDMAC */
		if (host->flags & MSHCI_USE_IDMA) {
			mshci_writel(host, BMOD_IDMAC_RESET, MSHCI_BMOD);
			count = 100;
			while ((mshci_readl(host, MSHCI_BMOD) & BMOD_IDMAC_RESET)
				&& --count)
				; /* nothing to do */
			mshci_writel(host, (mshci_readl(host, MSHCI_BMOD) |
					(BMOD_IDMAC_ENABLE|BMOD_IDMAC_FB)), MSHCI_BMOD);
			/* clear idmac interrupts */
			mshci_writel(host, 0x3FF, MSHCI_IDSTS);
		}
	}

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			mod_timer(&host->clock_timer, jiffies + msecs_to_jiffies(DELAY_TIME));
		}
	}

	host->working = MMC_HOST_FREE;
	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);

	mmc_detect_change(host->mmc, msecs_to_jiffies(200));
}

static void mshci_tasklet_finish(unsigned long param)
{
	struct mshci_host *host;
	unsigned long flags;
	struct mmc_request *mrq;
	u32 count = 100;

	host = (struct mshci_host *)param;

	spin_lock_irqsave(&host->lock, flags);

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		/* clear interrupt when clk disable */
		if (CLK_DISABLED == host->clk_ref_counter) {

			host->working = MMC_HOST_BUSY; /* set work busy */
#ifdef CONFIG_MMC_SD_HUB
			if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
		        SDHUB_clock_enable();
		        }
#endif         
			if (clk_enable(host->pclk))
				printk(KERN_ERR "%s:%s:clk_enable pclk failed\n",
					mmc_hostname(host->mmc), __func__);

			host->clk_ref_counter = CLK_ENABLED;

			mshci_writel(host, mshci_readl(host, MSHCI_MINTSTS), MSHCI_RINTSTS);
			mshci_writel(host, mshci_readl(host, MSHCI_IDSTS), MSHCI_IDSTS);

			if (host->clock != 0) {
				mod_timer(&host->clock_timer,
					  jiffies + msecs_to_jiffies(DELAY_TIME));
			}

			host->working = MMC_HOST_FREE;

			mmiowb();
			spin_unlock_irqrestore(&host->lock, flags);

			return;
		}
	}

	del_timer(&host->timer);

	mrq = host->mrq;

	/*
	 * The controller needs a reset of internal state machines
	 * upon error conditions.
	 */
	if (!(host->flags & MSHCI_DEVICE_DEAD) && (mrq) &&
		((mrq->cmd && mrq->cmd->error) ||
		 (mrq->data && (mrq->data->error ||
		  (mrq->data->stop && mrq->data->stop->error))))) {

		if (host->flags & MSHCI_USE_IDMA) {
			mshci_writel(host, BMOD_IDMAC_RESET, MSHCI_BMOD);
			count = 100;
			while ((mshci_readl(host, MSHCI_BMOD) & BMOD_IDMAC_RESET)
				&& --count)
				; /* nothing to do */

			mshci_writel(host, (mshci_readl(host, MSHCI_BMOD) |
					(BMOD_IDMAC_ENABLE|BMOD_IDMAC_FB)), MSHCI_BMOD);

			/* clear idmac interrupts */
			mshci_writel(host, 0x3FF, MSHCI_IDSTS);
		}

		/*
		 * Spec says we should do both at the same time,
		 * but Ricoh controllers do not like that
		 */
		mshci_reset_fifo(host);
	}

	if (!(host->flags & MSHCI_DEVICE_DEAD) && mrq && mrq->cmd &&
		!mrq->cmd->error && (SD_SEND_RELATIVE_ADDR == mrq->cmd->opcode)) {
		if (0 == mrq->cmd->arg) {
			host->tuning_rca = mrq->cmd->resp[0] >> 16;
		} else {
			host->tuning_rca = mrq->cmd->arg >> 16;
		}
	}

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		/* Disable the clock for power saving */
		if (host->clock != 0) {
			mod_timer(&host->clock_timer,
				  jiffies + msecs_to_jiffies(DELAY_TIME));
		}
	}

	host->mrq = NULL;
	host->cmd = NULL;
	host->data = NULL;
	host->working = MMC_HOST_FREE;
	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);

	if (host->flags & MSHCI_POST_UNMAP) {
		host->flags &= ~MSHCI_POST_UNMAP;

		if (mrq && mrq->cmd)
			mshci_idma_table_post(host, mrq->cmd->data);
		else
			printk(KERN_ERR "%s: idma table post error", mmc_hostname(host->mmc));
	}

	if (mrq && mrq->cmd && mrq->cmd->error == -ENOMEDIUM) {
		mrq->cmd->retries = 0;
		host->flags &= ~MSHCI_TUNING_DONE;
	}

	if ((host->quirks & MSHCI_QUIRK_TUNING_ENABLE) && host->ops->tuning_move) {
		if ((!(host->flags & MSHCI_DEVICE_DEAD)) && (host->flags & MSHCI_TUNING_DONE) &&
		mrq && mrq->cmd &&
		((mrq->cmd->error && mrq->cmd->error != -ETIMEDOUT && mrq->cmd->error != -ENOMEDIUM) ||
		(mrq->cmd->data && ((mrq->cmd->data->error && mrq->cmd->data->error != -ETIMEDOUT) ||
		(mrq->cmd->data->stop && mrq->cmd->data->stop->error &&	mrq->cmd->data->stop->error != ETIMEDOUT))))) {
			printk(KERN_ERR "%s: move tuning del_sel, start=%d, cmd=%d, arg=0x%x\n",
					mmc_hostname(host->mmc), host->tuning_move_start, mrq->cmd->opcode, mrq->cmd->arg);
			/* req error, need move del_sel */
			if (host->tuning_move_start != -1) {
				if (host->ops->tuning_move(host, host->tuning_move_start)) {
					host->tuning_move_start = 0;
					mrq->cmd->retries++;

					if (mrq->cmd->data && mrq->cmd->data->error) {
						mrq->cmd->data->error = 0;
					}
					if (mrq->cmd->data && mrq->cmd->data->stop && mrq->cmd->data->stop->error) {
						mrq->cmd->data->stop->error = 0;
					}
					if (!mrq->cmd->error) {
						mrq->cmd->error = -EILSEQ;
					}
				} else {
					host->tuning_move_start = -1;
				}
			}
		} else {
			host->tuning_move_start = 1;
		}
	}
	if (mrq) {
		if (host->flags & MSHCI_IN_TUNING) {
			if(mrq->done)
				mrq->done(mrq);
		} else {
			if (mrq->cmd && mrq->cmd->error == -ETIMEDOUT &&
				mrq->cmd->opcode == MMC_SEND_OP_COND) {
				mrq->cmd->error = 0;
			}
			if(mrq->cmd)
				mmc_request_done(host->mmc, mrq);
		}
	}
}

static void mshci_timeout_timer(unsigned long data)
{
	struct mshci_host *host;
	unsigned long flags;

	host = (struct mshci_host *)data;

	spin_lock_irqsave(&host->lock, flags);

	if (host->mrq) {
		printk(KERN_ERR "%s: Timeout waiting for hardware "
			"interrupt.\n", mmc_hostname(host->mmc));
		printk(KERN_ERR "GIC DUMP:\n");
		printk(KERN_ERR "0xFC001100	0x%x\n", readl(IO_ADDRESS(0xFC001100)));
		printk(KERN_ERR "0xFC001104     0x%x\n", readl(IO_ADDRESS(0xFC001104)));
		printk(KERN_ERR "0xFC001200     0x%x\n", readl(IO_ADDRESS(0xFC001200)));
		printk(KERN_ERR "0xFC001204     0x%x\n", readl(IO_ADDRESS(0xFC001204)));
		printk(KERN_ERR "0xFC001300     0x%x\n", readl(IO_ADDRESS(0xFC001300)));
		printk(KERN_ERR "0xFC001304     0x%x\n", readl(IO_ADDRESS(0xFC001304)));
		mshci_dumpregs(host);

		if (host->data) {
			host->data->error = -ETIMEDOUT;
			mshci_finish_data(host);
		} else {
			if (host->cmd)
				host->cmd->error = -ETIMEDOUT;
			else
				host->mrq->cmd->error = -ETIMEDOUT;

			tasklet_schedule(&host->finish_tasklet);
		}
	}

	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);
}

static void mshci_clock_gate_timer(unsigned long data)
{
	struct mshci_host *host;
	unsigned long flags;

	host = (struct mshci_host *)data;

	spin_lock_irqsave(&host->lock, flags);

	/*
	 * if data line is busy or cmd, data and mrq exist,
	 * don't turn clock off
	 */
	if ((mshci_readl(host, MSHCI_STATUS) & (1 << 9))
		|| host->cmd || host->data || host->mrq || MMC_HOST_BUSY == host->working) {
		mod_timer(&host->clock_timer, jiffies + msecs_to_jiffies(DELAY_TIME));
	} else {
		host->clock_to_restore = host->clock;
		mshci_set_clock(host, 0);

		if (host->pclk != NULL && host->clk_ref_counter == CLK_ENABLED) {
			mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) & ~INT_ENABLE),
					MSHCI_CTRL);
			clk_disable(host->pclk);
			host->clk_ref_counter = CLK_DISABLED;
#ifdef CONFIG_MMC_SD_HUB
			if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
		        SDHUB_clock_disable();
		    }
#endif
		}
	}

	spin_unlock_irqrestore(&host->lock, flags);
}

/* Interrupt handling */

static void mshci_cmd_irq(struct mshci_host *host, u32 intmask)
{
	BUG_ON(intmask == 0);

	if (!host->cmd) {
		printk(KERN_ERR "%s: Got command interrupt 0x%08x even "
			"though no command operation was in progress.\n",
			mmc_hostname(host->mmc), (unsigned)intmask);
		return;
	}

	if (intmask & INTMSK_RTO) {
		printk(KERN_ERR "%s: CMD %d(arg=0x%x) REP time out\n",
						mmc_hostname(host->mmc), host->cmd->opcode, host->cmd->arg);
		host->cmd->error = -ETIMEDOUT;
	} else if (intmask & INTMSK_RCRC) {
		printk(KERN_ERR "%s: CMD %d(arg=0x%x) REP CRC error\n",
						mmc_hostname(host->mmc), host->cmd->opcode, host->cmd->arg);
		host->cmd->error = -EILSEQ;
	} else if (intmask & INTMSK_RE) {
		printk(KERN_ERR "%s: CMD %d(arg=0x%x) REP error\n",
						mmc_hostname(host->mmc), host->cmd->opcode, host->cmd->arg);
		host->cmd->error = -EILSEQ;
	}

	if (host->cmd->error) {
		tasklet_schedule(&host->finish_tasklet);
		return;
	}

	if (intmask & INTMSK_CDONE)
		mshci_finish_command(host);
}

static void mshci_data_irq(struct mshci_host *host, u32 intmask, u8 intr_src)
{
	BUG_ON(intmask == 0);

	if (!host->data) {
		/*
		 * The "data complete" interrupt is also used to
		 * indicate that a busy state has ended. See comment
		 * above in mshci_cmd_irq().
		 */
		if (host->cmd && (host->cmd->flags & MMC_RSP_BUSY)) {
			if (intmask & INTMSK_DTO) {
				mshci_finish_command(host);
				return;
			}
		}

		printk(KERN_ERR "%s: Got data interrupt 0x%08x from %s "
			"even though no data operation was in progress.\n",
			mmc_hostname(host->mmc), (unsigned)intmask,
			intr_src ? "MINT" : "IDMAC");
		return;
	}
	if (intr_src == INT_SRC_MINT) {
		if (intmask & INTMSK_HTO) {
			printk(KERN_ERR "%s: Host timeout error\n",
						mmc_hostname(host->mmc));
			host->data->error = -ETIMEDOUT;
		} else if (intmask & INTMSK_DRTO) {
			printk(KERN_ERR "%s: Data read timeout error\n",
						mmc_hostname(host->mmc));
			host->data->error = -ETIMEDOUT;
		} else if (intmask & INTMSK_SBE) {
			printk(KERN_ERR "%s: Start bit error\n",
						mmc_hostname(host->mmc));
			host->data->error = -EIO;
		} else if (intmask & INTMSK_EBE) {
			printk(KERN_ERR "%s: Endbit/Write no CRC error\n",
						mmc_hostname(host->mmc));
			host->data->error = -EIO;
		} else if (intmask & INTMSK_DCRC) {
			printk(KERN_ERR "%s: Data CRC error\n",
						mmc_hostname(host->mmc));
			host->data->error = -EIO;
		} else if (intmask & INTMSK_FRUN) {
			printk(KERN_ERR "%s: FIFO underrun/overrun error\n",
						mmc_hostname(host->mmc));
			host->data->error = -EIO;
		}
	} else {
		if (intmask & IDSTS_FBE) {
			printk(KERN_ERR "%s: Fatal Bus error on DMA\n",
					mmc_hostname(host->mmc));
			host->data->error = -EIO;
		} else if (intmask & IDSTS_CES) {
			printk(KERN_ERR "%s: Card error on DMA\n",
					mmc_hostname(host->mmc));
			host->data->error = -EIO;
		} else if (intmask & IDSTS_DU) {
			printk(KERN_ERR "%s: Description error on DMA\n",
					mmc_hostname(host->mmc));
			host->data->error = -EIO;
		}
	}

	if (host->data->error) {
		mshci_finish_data(host);
	} else {
		if (!(host->flags & MSHCI_REQ_USE_DMA) &&
				(((host->data->flags & MMC_DATA_READ) &&
				(intmask & (INTMSK_RXDR | INTMSK_DTO))) ||
				((host->data->flags & MMC_DATA_WRITE) &&
					(intmask & (INTMSK_TXDR)))))
			mshci_transfer_pio(host);

		if (intmask & INTMSK_DTO) {
			if (host->cmd) {
				/*
				 * Data managed to finish before the
				 * command completed. Make sure we do
				 * things in the proper order.
				 */
				host->data_early = 1;
			} else {
				mshci_finish_data(host);
			}
		}
	}
}

static irqreturn_t mshci_irq(int irq, void *dev_id)
{
	irqreturn_t result;
	struct mshci_host *host = dev_id;
	u32 intmask;
	int cardint = 0;
	int timeout = 0x10000;

	spin_lock(&host->lock);

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (CLK_DISABLED == host->clk_ref_counter) {
			printk(KERN_ERR "%s: Got interrupt even though clk disable\n",
				mmc_hostname(host->mmc));
			tasklet_schedule(&host->finish_tasklet);
			mmiowb();
			spin_unlock(&host->lock);
			return IRQ_HANDLED;
		}
	}

	intmask = mshci_readl(host, MSHCI_MINTSTS);

	if (!intmask || intmask == 0xffffffff) {
		/* check if there is a interrupt for IDMAC  */
		intmask = mshci_readl(host, MSHCI_IDSTS);
		if (intmask) {
			mshci_writel(host, intmask, MSHCI_IDSTS);
			mshci_data_irq(host, intmask, INT_SRC_IDMAC);
			result = IRQ_HANDLED;
			goto out;
			}
		result = IRQ_NONE;
		goto out;
	}
	DBG("*** %s got interrupt: 0x%08x\n", mmc_hostname(host->mmc), intmask);

	mshci_writel(host, intmask, MSHCI_RINTSTS);

	if (intmask & (INTMSK_CDETECT))
		tasklet_schedule(&host->card_tasklet);

	intmask &= ~INTMSK_CDETECT;

	if (intmask & CMD_STATUS) {
		if (!(intmask & INTMSK_CDONE) && (intmask & INTMSK_RTO)) {
			/*
			 * when a error about command timeout occurs,
			 * cmd done intr comes together.
			 * cmd done intr comes later than error intr.
			 * so, it has to wait for cmd done intr.
			 */
			while (--timeout && !(mshci_readl(host, MSHCI_MINTSTS)
				& INTMSK_CDONE))
				; /* Nothing to do */
			if (!timeout)
				printk(KERN_ERR"*** %s time out for\
					CDONE intr\n",
					mmc_hostname(host->mmc));
			else
				mshci_writel(host, INTMSK_CDONE,
					MSHCI_RINTSTS);
			mshci_cmd_irq(host, intmask & CMD_STATUS);
		} else {
			mshci_cmd_irq(host, intmask & CMD_STATUS);
		}
	}

	/* 
	 * m53980:
	 * A way to handle voltage switch.
	 * This is the first half.
	 * Mobile storage IP hava a unique implementation to voltage switch,
	 * it will generate interrupt INTMSK_VOLT_SWITCH first, generate cd 
	 * interrupt after voltage switch successfully.
	 * So in this first half, we just send cmd11 and handle INTMSK_VOLT_SWITCH
	 * In the second half, we stop clock, switch voltage, start clock and handle
	 * cd interrtup.
	 * The second half is inplemented in function mshci_start_signal_voltage_switch
	 */
	if (host->cmd && (host->cmd->flags & MMC_CMD_VOLT_SWITCH) 
		&& (intmask & INTMSK_VOLT_SWITCH)) {
		mshci_finish_command(host);
		intmask &= ~INTMSK_VOLT_SWITCH; 
	}

	if (intmask & DATA_STATUS) {
		if (!(intmask & INTMSK_DTO) && (intmask & INTMSK_DRTO)) {
			/*
			 * when a error about data timout occurs,
			 * DTO intr comes together.
			 * DTO intr comes later than error intr.
			 * so, it has to wait for DTO intr.
			 */
			while (--timeout && !(mshci_readl(host, MSHCI_MINTSTS)
				& INTMSK_DTO))
				; /* Nothing to do */
			if (!timeout)
				printk(KERN_ERR"*** %s time out for\
					CDONE intr\n",
					mmc_hostname(host->mmc));
			else
				mshci_writel(host, INTMSK_DTO,
					MSHCI_RINTSTS);
			mshci_data_irq(host, intmask & DATA_STATUS,
							INT_SRC_MINT);
		} else {
			mshci_data_irq(host, intmask & DATA_STATUS,
							INT_SRC_MINT);
		}
	}

	intmask &= ~(CMD_STATUS | DATA_STATUS);

	if (intmask & SDIO_INT_ENABLE)
		cardint = 1;

	intmask &= ~SDIO_INT_ENABLE;

	if (intmask) {
		printk(KERN_ERR "%s: Unexpected interrupt 0x%08x.\n",
			mmc_hostname(host->mmc), intmask);
		mshci_dumpregs(host);
	}

	result = IRQ_HANDLED;

	mmiowb();
out:
	spin_unlock(&host->lock);

	/* We have to delay this as it calls back into the driver */
	if (cardint) {
		if ((host->mmc) && (host->mmc->sdio_irq_thread)) {
			mmc_signal_sdio_irq(host->mmc);
		} else {
			printk(KERN_ERR "[%s] NULL pointer sdio_irq_thread (irq.no: %d)\n", __FUNCTION__, irq);
		}
	}

	return result;
}

/* Suspend and Resume */

#ifdef CONFIG_PM

int mshci_suspend_host(struct mshci_host *host, pm_message_t state)
{
	int ret;

	mshci_disable_card_detection(host);
	
	//z00186406 add, to enable TI wifi suspend/resume functionality,  begin
	if ((host->quirks & (MSHCI_QUIRK_WLAN_DETECTION | MSHCI_QUIRK_TI_WL18XX)) == 0) {
	//z00186406 add, to enable TI wifi suspend/resume functionality,  end
		ret = mmc_suspend_host(host->mmc);
		if (ret)
			return ret;
	}

	free_irq(host->irq, host);

	return 0;
}
EXPORT_SYMBOL_GPL(mshci_suspend_host);

static void mshci_resume_init(struct mshci_host *host);
int mshci_resume_host(struct mshci_host *host)
{
	int ret;
	unsigned long flags;

	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			del_timer_sync(&host->clock_timer);
		}
	}

	spin_lock_irqsave(&host->lock, flags);
	if (host->clk_ref_counter == CLK_DISABLED) {
#ifdef CONFIG_MMC_SD_HUB
			if (host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION) {
		        SDHUB_clock_enable();
		        }
#endif         
        
		ret = clk_enable(host->pclk);
		host->clk_ref_counter = CLK_ENABLED;
		if (ret) {
			printk(KERN_ERR "%s:%s:clk_enable pclk failed\n",
					mmc_hostname(host->mmc), __func__);
			ret = -ENOENT;
		}
	}
	spin_unlock_irqrestore(&host->lock, flags);

	if (host->flags & (MSHCI_USE_IDMA)) {
		if (host->ops->enable_dma)
			host->ops->enable_dma(host);
	}

	mshci_resume_init(host);

	ret = request_irq(host->irq, mshci_irq, IRQF_SHARED,
			  mmc_hostname(host->mmc), host);
	if (ret)
		goto out;
	mmiowb();
	
	//z00186406 add, to enable TI wifi suspend/resume functionality,  begin
	if ((host->quirks & (MSHCI_QUIRK_WLAN_DETECTION | MSHCI_QUIRK_TI_WL18XX)) == 0) {
	//z00186406 add, to enable TI wifi suspend/resume functionality,  end
		ret = mmc_resume_host(host->mmc);
		if (ret)
			goto out;
	}

	host->flags &= ~MSHCI_IN_TUNING;
	host->flags &= ~MSHCI_TUNING_DONE;

	mshci_enable_card_detection(host);

out:
	if (host->mmc->caps & MMC_CAP_CLOCK_GATING) {
		if (&host->clock_timer != NULL) {
			mod_timer(&host->clock_timer, jiffies + msecs_to_jiffies(DELAY_TIME));
		}
	}

	return ret;
}
EXPORT_SYMBOL_GPL(mshci_resume_host);

#endif /* CONFIG_PM */

/* Device allocation and registration */

struct mshci_host *mshci_alloc_host(struct device *dev,
					size_t priv_size)
{
	struct mmc_host *mmc;
	struct mshci_host *host;

	WARN_ON(dev == NULL);

	mmc = mmc_alloc_host(sizeof(struct mshci_host) + priv_size, dev);
	if (!mmc)
		return ERR_PTR(-ENOMEM);

	host = mmc_priv(mmc);
	host->mmc = mmc;

	return host;
}

static void mshci_fifo_init(struct mshci_host *host)
{
	int fifo_val, fifo_depth, fifo_threshold;

	fifo_val = mshci_readl(host, MSHCI_FIFOTH);
	fifo_depth = ((fifo_val & RX_WMARK)>>16)+1+(fifo_val & TX_WMARK);

	fifo_threshold = FIFO_TH;
	host->fifo_threshold = fifo_threshold;
	host->fifo_depth = fifo_depth;

	fifo_val &= ~(RX_WMARK | TX_WMARK | MSIZE_MASK);

	fifo_val |= (fifo_threshold | ((fifo_threshold - 1) << 16));
	fifo_val |= MSIZE_8;

	mshci_writel(host, fifo_val, MSHCI_FIFOTH);

	printk(KERN_INFO "%s: FIFO WMARK FOR RX 0x%x WX 0x%x.\n",
		mmc_hostname(host->mmc), ((fifo_val & RX_WMARK) >> 16) + 1,
		fifo_val & TX_WMARK);
}

static void mshci_resume_init(struct mshci_host *host)
{
	int count;

	mshci_init(host);

	mshci_fifo_init(host);

	/* set debounce filter value */
	mshci_writel(host, 0xfffff, MSHCI_DEBNCE);

	/* clear card type. set 1bit mode */
	mshci_writel(host, 0x0, MSHCI_CTYPE);

	/* set bus mode register for IDMAC */
	if (host->flags & MSHCI_USE_IDMA) {
		mshci_writel(host, BMOD_IDMAC_RESET, MSHCI_BMOD);
		count = 100;
		while ((mshci_readl(host, MSHCI_BMOD) & BMOD_IDMAC_RESET)
			&& --count)
			; /* nothing to do */

		mshci_writel(host, (mshci_readl(host, MSHCI_BMOD) |
				(BMOD_IDMAC_ENABLE|BMOD_IDMAC_FB)), MSHCI_BMOD);
	}
}

EXPORT_SYMBOL_GPL(mshci_alloc_host);

int mshci_add_host(struct mshci_host *host)
{
	struct mmc_host *mmc;
	int ret, count;

	WARN_ON(host == NULL);
	if (host == NULL)
		return -EINVAL;

	mmc = host->mmc;

	if (debug_quirks)
		host->quirks = debug_quirks;
	host->working = MMC_HOST_BUSY;
	mshci_reset_all(host);

	host->version = mshci_readl(host, MSHCI_VERID);

	host->flags &= ~MSHCI_IN_TUNING;
	host->flags &= ~MSHCI_TUNING_DONE;
	host->flags |= MSHCI_CMD_SEND_INIT;

	/* there are no reasons not to use DMA */
	host->flags |= MSHCI_USE_IDMA;

	if (host->flags & MSHCI_USE_IDMA) {
		/* We need to allocate descriptors for all sg entries
		 * DMA_SG_NUM transfer for each of those entries. */
		host->idma_desc = kmalloc(DMA_SG_NUM * sizeof(struct mshci_idmac),
					GFP_KERNEL);
		if (!host->idma_desc) {
			kfree(host->idma_desc);
			printk(KERN_WARNING "%s: Unable to allocate IDMA "
				"buffers. Falling back to standard DMA.\n",
				mmc_hostname(mmc));
			host->flags &= ~MSHCI_USE_IDMA;
		}
	}

	/*
	 * If we use DMA, then it's up to the caller to set the DMA
	 * mask, but PIO does not need the hw shim so we set a new
	 * mask here in that case.
	 */
	if (!(host->flags & (MSHCI_USE_IDMA))) {
		host->dma_mask = DMA_BIT_MASK(64);
		mmc_dev(host->mmc)->dma_mask = &host->dma_mask;
	}

	printk(KERN_INFO "%s: Version ID 0x%x.\n",
		mmc_hostname(host->mmc), host->version);

	host->max_clk = 0;

	if (host->max_clk == 0) {
		if (!host->ops->get_max_clock) {
			printk(KERN_ERR
				   "%s: Hardware doesn't specify base clock "
				   "frequency.\n", mmc_hostname(mmc));
			return -ENODEV;
		}
		host->max_clk = host->ops->get_max_clock(host);
	}

	/* Set host parameters */
	if (host->ops->get_ro)
		mshci_ops.get_ro = host->ops->get_ro;

	if (!(host->quirks & MSHCI_QUIRK_TUNING_ENABLE)) {
		mmc->ops = &no_tuning_mshci_ops;
	} else {
		mmc->ops = &mshci_ops;
	}

	mmc->f_min = host->max_clk / 510;
	mmc->f_max = host->max_clk;
	mmc->caps |= MMC_CAP_SDIO_IRQ;

	mmc->caps |= MMC_CAP_4_BIT_DATA;

	/*mmc->ocr_avail = 0;*/
	/*mmc->ocr_avail |= MMC_VDD_32_33|MMC_VDD_33_34;*/
	/*mmc->ocr_avail |= MMC_VDD_29_30|MMC_VDD_30_31;*/

	if (mmc->ocr_avail == 0) {
		printk(KERN_ERR "%s: Hardware doesn't report any "
			"support voltages.\n", mmc_hostname(mmc));
		return -ENODEV;
	}

	spin_lock_init(&host->lock);

	/* Maximum number of segments */
	mmc->max_segs = DMA_SG_NUM;

	/*
	 * Maximum number of sectors in one transfer. Limited by DMA boundary
	 * size (4KiB).
	 * Limited by CPU I/O boundry size (0xfffff000 KiB)
	 */

	/*
	 * to prevent starvation of a process that want to access SD device
	 * it should limit size that transfer at one time.
	 */
	mmc->max_req_size = 0x80000;

	/*
	 * Maximum segment size. Could be one segment with the maximum number
	 * of bytes. When doing hardware scatter/gather, each entry cannot
	 * be larger than 4 KiB though.
	 */
	if (host->flags & MSHCI_USE_IDMA)
		mmc->max_seg_size = 0x1000;
	else
		mmc->max_seg_size = mmc->max_req_size;

	/*
	 * from SD spec 2.0 and MMC spec 4.2, block size has been
	 * fixed to 512 byte
	 */
	mmc->max_blk_size = 0;

	mmc->max_blk_size = 512 << mmc->max_blk_size;

	/* Maximum block count */
	mmc->max_blk_count = 0xffff;

	/* Init tasklets */
	tasklet_init(&host->card_tasklet,
		mshci_tasklet_card, (unsigned long)host);
	tasklet_init(&host->finish_tasklet,
		mshci_tasklet_finish, (unsigned long)host);

	setup_timer(&host->timer, mshci_timeout_timer, (unsigned long)host);
	if (host->mmc->caps & MMC_CAP_CLOCK_GATING)
		setup_timer(&host->clock_timer, mshci_clock_gate_timer,
				(unsigned long)host);

	mshci_init(host);

	mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) & ~INT_ENABLE),
					MSHCI_CTRL);

	mshci_fifo_init(host);

	/* set debounce filter value */
	mshci_writel(host, 0xfffff, MSHCI_DEBNCE);

	/* clear card type. set 1bit mode */
	mshci_writel(host, 0x0, MSHCI_CTYPE);

	/* set bus mode register for IDMAC */
	if (host->flags & MSHCI_USE_IDMA) {
		mshci_writel(host, BMOD_IDMAC_RESET, MSHCI_BMOD);
		count = 100;
		while ((mshci_readl(host, MSHCI_BMOD) & BMOD_IDMAC_RESET)
			&& --count)
			; /* nothing to do */

		mshci_writel(host, (mshci_readl(host, MSHCI_BMOD) |
				(BMOD_IDMAC_ENABLE|BMOD_IDMAC_FB)), MSHCI_BMOD);

		/* clear idmac interrupts */
		mshci_writel(host, 0x3FF, MSHCI_IDSTS);
	}
#ifdef CONFIG_MMC_DEBUG
	mshci_dumpregs(host);
#endif

	if(host->quirks & MSHCI_QUIRK_EXTERNAL_CARD_DETECTION){
		host->flags &= ~MSHCI_CARD_NEED_ENUM;
	}

	ret = request_irq(host->irq, mshci_irq, IRQF_SHARED,
		mmc_hostname(mmc), host);
	if (ret)
		goto untasklet;

	mmiowb();

	mmc_add_host(mmc);

	printk(KERN_INFO "%s: MSHCI controller on %s [%s] using %s\n",
		mmc_hostname(mmc), host->hw_name, dev_name(mmc_dev(mmc)),
		(host->flags & MSHCI_USE_IDMA) ? "IDMA" : "PIO");

	mshci_enable_card_detection(host);
	host->working = MMC_HOST_FREE;

	INIT_WORK(&host->mshci_delete_timer_work, mshci_delete_timer);

	return 0;

untasklet:
	tasklet_kill(&host->card_tasklet);
	tasklet_kill(&host->finish_tasklet);
	host->working = MMC_HOST_FREE;

	return ret;
}
EXPORT_SYMBOL_GPL(mshci_add_host);

void mshci_remove_host(struct mshci_host *host, int dead)
{
	unsigned long flags;

	if (dead) {
		spin_lock_irqsave(&host->lock, flags);

		host->flags |= MSHCI_DEVICE_DEAD;

		if (host->mrq) {
			printk(KERN_ERR "%s: Controller removed during "
				" transfer!\n", mmc_hostname(host->mmc));

			host->mrq->cmd->error = -ENOMEDIUM;
			host->flags &= ~MSHCI_IN_TUNING;
			host->flags &= ~MSHCI_TUNING_DONE;
			tasklet_schedule(&host->finish_tasklet);
		}

		spin_unlock_irqrestore(&host->lock, flags);
	}

	mshci_disable_card_detection(host);

	mmc_remove_host(host->mmc);

	if (!dead)
		mshci_reset_all(host);

	free_irq(host->irq, host);

	del_timer_sync(&host->timer);
	if (host->mmc->caps & MMC_CAP_CLOCK_GATING)
		del_timer_sync(&host->clock_timer);

	tasklet_kill(&host->card_tasklet);
	tasklet_kill(&host->finish_tasklet);

	kfree(host->idma_desc);

	host->idma_desc = NULL;
	host->align_buffer = NULL;
}
EXPORT_SYMBOL_GPL(mshci_remove_host);

void mshci_free_host(struct mshci_host *host)
{
	mmc_free_host(host->mmc);
}
EXPORT_SYMBOL_GPL(mshci_free_host);

void mshci_disconnect(struct mshci_host *host)
{
#ifdef CONFIG_MMC_SD_HUB
	if( flag_connection == 1 ) {
		SDHUB_Disconnect( host->mmc, SDHUB_DEVICE_ID );
		flag_connection = 0 ;
	}
#endif /* CONFIG_MMC_SD_HUB */
}
EXPORT_SYMBOL_GPL(mshci_disconnect);


/* Driver init and exit */

static int __init mshci_drv_init(void)
{
	printk(KERN_INFO DRIVER_NAME
		": Mobile Storage Host Controller Interface driver\n");
	printk(KERN_INFO DRIVER_NAME ": Copyright(c) Pierre Ossman\n");

	return 0;
}

static void __exit mshci_drv_exit(void)
{
}

module_init(mshci_drv_init);
module_exit(mshci_drv_exit);

module_param(debug_quirks, uint, 0444);

MODULE_AUTHOR("Hyunsung Jang <hs79.jang@samsung.com>");
MODULE_AUTHOR("Hyuk Lee <hyuk1.lee@samsung.com>");
MODULE_DESCRIPTION("Mobile Storage Host Controller Interface core driver");
MODULE_LICENSE("GPL");

MODULE_PARM_DESC(debug_quirks, "Force certain quirks.");
