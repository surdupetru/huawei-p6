/*
 * linux/drivers/mmc/host/mshci.h
 *
 * Mobile Storage Host Controller Interface driver
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Based on linux/drivers/mmc/host/sdhci.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifndef __MSHCI_H
#define __MSHCI_H

#include <linux/scatterlist.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/io.h>

/* MSHC Internal Registers */

#define MSHCI_CTRL	0x00	/* Control */
#define MSHCI_PWREN	0x04	/* Power-enable */
#define MSHCI_CLKDIV	0x08	/* Clock divider */
#define MSHCI_CLKSRC	0x0C	/* Clock source */
#define MSHCI_CLKENA	0x10	/* Clock enable */
#define MSHCI_TMOUT	0x14	/* Timeout */
#define MSHCI_CTYPE	0x18	/* Card type */
#define MSHCI_BLKSIZ	0x1C	/* Block Size */
#define MSHCI_BYTCNT	0x20	/* Byte count */
#define MSHCI_INTMSK	0x24	/* Interrupt Mask */
#define MSHCI_CMDARG	0x28	/* Command Argument */
#define MSHCI_CMD	0x2C	/* Command */
#define MSHCI_RESP0	0x30	/* Response 0 */
#define MSHCI_RESP1	0x34	/* Response 1 */
#define MSHCI_RESP2	0x38	/* Response 2 */
#define MSHCI_RESP3	0x3C	/* Response 3 */
#define MSHCI_MINTSTS	0x40	/* Masked interrupt status */
#define MSHCI_RINTSTS	0x44	/* Raw interrupt status */
#define MSHCI_STATUS	0x48	/* Status */
#define MSHCI_FIFOTH	0x4C	/* FIFO threshold */
#define MSHCI_CDETECT	0x50	/* Card detect */
#define MSHCI_WRTPRT	0x54	/* Write protect */
#define MSHCI_GPIO	0x58	/* General Purpose IO */
#define MSHCI_TCBCNT	0x5C	/* Transferred CIU byte count */
#define MSHCI_TBBCNT	0x60	/* Transferred host/DMA to/from byte count */
#define MSHCI_DEBNCE	0x64	/* Card detect debounce */
#define MSHCI_USRID	0x68	/* User ID */
#define MSHCI_VERID	0x6C	/* Version ID */
#define MSHCI_HCON	0x70	/* Hardware Configuration */
#define MSHCI_UHS_REG	0x74	/* UHS and DDR setting */
#define MSHCI_BMOD	0x80	/* Bus mode register */
#define MSHCI_PLDMND	0x84	/* Poll demand */
#define MSHCI_DBADDR	0x88	/* Descriptor list base address */
#define MSHCI_IDSTS	0x8C	/* Internal DMAC status */
#define MSHCI_IDINTEN	0x90	/* Internal DMAC interrupt enable */
#define MSHCI_DSCADDR	0x94	/* Current host descriptor address */
#define MSHCI_BUFADDR	0x98	/* Current host buffer address */
#define MSHCI_WAKEUPCON	0xA0	/* Wakeup control register */
#define MSHCI_CLOCKCON	0xA4	/* Clock (delay) control register */
#define MSHCI_CARDTHRCTl	0x100	/* Card Threshold Control Register */
#define MSHCI_FIFODAT	0x200	/* FIFO data read write */

/* Control Register MSHCI_CTRL(offset 0x00) */

#define CTRL_RESET	(0x1 << 0) /* Reset DWC_mobile_storage controller */
#define FIFO_RESET	(0x1 << 1) /* Reset FIFO */
#define DMA_RESET	(0x1 << 2) /* Reset DMA interface */
#define INT_ENABLE	(0x1 << 4) /* Global interrupt enable/disable bit */
#define DMA_ENABLE	(0x1 << 5) /* DMA transfer mode enable/disable bit */
#define ENABLE_IDMAC	(0x1 << 25)

/* Clock Enable Register MSHCI_CLKENA(offset 0x10) */

#define CLK_ENABLE	(0x1 << 0)
#define CLK_DISABLE	(0x0 << 0)

/* Interrupt Mask Register MSHCI_INTMSK(offset 0x24) */

#define SDIO_INT_ENABLE	(0x1 << 16)

/* Interrupt bits */

#define INTMSK_ALL	0xFFFFFFFF
#define INTMSK_CDETECT	(0x1 << 0)
#define INTMSK_RE	(0x1 << 1)
#define INTMSK_CDONE	(0x1 << 2)
#define INTMSK_DTO	(0x1 << 3)
#define INTMSK_TXDR	(0x1 << 4)
#define INTMSK_RXDR	(0x1 << 5)
#define INTMSK_RCRC	(0x1 << 6)
#define INTMSK_DCRC	(0x1 << 7)
#define INTMSK_RTO	(0x1 << 8)
#define INTMSK_DRTO	(0x1 << 9)
#define INTMSK_HTO	(0x1 << 10)
#define INTMSK_VOLT_SWITCH	(0x1 << 10)
#define INTMSK_FRUN	(0x1 << 11)
#define INTMSK_HLE	(0x1 << 12)
#define INTMSK_SBE	(0x1 << 13)
#define INTMSK_ACD	(0x1 << 14)
#define INTMSK_EBE	(0x1 << 15)
#define INTMSK_DMA	(INTMSK_ACD | INTMSK_RXDR | INTMSK_TXDR)

#define INT_SRC_IDMAC	(0x0)
#define INT_SRC_MINT	(0x1)

/* Command Register MSHCI_CMD(offset 0x2C) */

#define CMD_RESP_EXP_BIT	(0x1 << 6)
#define CMD_RESP_LENGTH_BIT	(0x1 << 7)
#define CMD_CHECK_CRC_BIT	(0x1 << 8)
#define CMD_DATA_EXP_BIT	(0x1 << 9)
#define CMD_RW_BIT		(0x1 << 10)
#define CMD_TRANSMODE_BIT	(0x1 << 11)
#define CMD_WAIT_PRV_DAT_BIT	(0x1 << 13)
#define CMD_STOP_ABORT_CMD	(0x1 << 14)
#define CMD_SEND_INITIALIZATION	(0x1 << 15)
#define CMD_SEND_CLK_ONLY	(0x1 << 21)
#define CMD_VOLT_SWITCH     (0x1 << 28)
#define CMD_USE_HOLD_REG    (0x1 << 29)
#define CMD_STRT_BIT		(0x1 << 31)
#define CMD_ONLY_CLK		(CMD_STRT_BIT | CMD_SEND_CLK_ONLY | \
						CMD_WAIT_PRV_DAT_BIT)

/* Raw Interrupt Register MSHCI_RINTSTS(offset 0x44) */

#define DATA_ERR	(INTMSK_EBE | INTMSK_SBE | INTMSK_HLE |		\
			 INTMSK_FRUN | INTMSK_EBE | INTMSK_DCRC)
#define DATA_TOUT	(INTMSK_HTO | INTMSK_DRTO)
#define DATA_STATUS	(DATA_ERR | DATA_TOUT | INTMSK_RXDR |		\
			 INTMSK_TXDR | INTMSK_DTO)
#define CMD_STATUS	(INTMSK_RTO | INTMSK_RCRC | INTMSK_CDONE |	\
			 INTMSK_RE)

/* Status Register MSHCI_STATUS(offset 0x48) */

#define FIFO_COUNT	(0x1FFF << 17)
#define FIFO_WIDTH	(0x4)

/* FIFO Threshold Watermark Register MSHCI_FIFOTH(offset 0x4C) */

#define TX_WMARK		(0xFFF << 0)
#define RX_WMARK		(0xFFF << 16)
#define MSIZE_MASK		(0x7 << 28)

/* DW DMA Mutiple Transaction Size */
#define MSIZE_8			(2 << 28)
#define FIFO_TH			(8)


/*
 * Card Detect Register MSHCI_CDETECT(offset 0x50)
 * It assumes there is only one SD slot
 */
#define CARD_PRESENT		(0x1 << 0)

/*
 * Write Protect Register MSHCI_WRTPRT(offset 0x54)
 * It assumes there is only one SD slot
 */
#define WRTPRT_ON		(0x1 << 0)

/* Bus Mode Register MSHCI_BMOD(offset 0x80) */

#define BMOD_IDMAC_RESET	(0x1 << 0)
#define BMOD_IDMAC_FB		(0x1 << 1)
#define BMOD_IDMAC_ENABLE	(0x1 << 7)

/* Hardware Configuration  Register MSHCI_IDSTS(offset 0x8c) */

#define IDSTS_CES		(0x1 << 5)
#define IDSTS_DU		(0x1 << 4)
#define IDSTS_FBE		(0x1 << 2)

struct mshci_ops;

struct mshci_idmac {
	u32	des0;
	u32	des1;
	u32	des2;
	u32	des3;
#define MSHCI_IDMAC_OWN	(1 << 31)
#define MSHCI_IDMAC_CH		(1 << 4)
#define MSHCI_IDMAC_FS		(1 << 3)
#define MSHCI_IDMAC_LD		(1 << 2)
#define INTMSK_IDMAC_ERROR	(0x214)
};

struct mshci_host {
	/* Data set by hardware interface driver */
	const char		*hw_name;	/* Hardware bus name */
	int				hw_mmc_id;	/* Hardware mmc id */
	unsigned int		quirks;		/* Deviations from spec. */

/* Controller has no write-protect pin connected with SD card */

#define MSHCI_QUIRK_NO_WP_BIT			(1 << 0)
#define MSHCI_QUIRK_BROKEN_CARD_DETECTION	(1 << 1)
#define MSHCI_QUIRK_ALWAYS_WRITABLE		(1 << 2)
#define MSHCI_QUIRK_BROKEN_PRESENT_BIT		(1 << 3)
#define MSHCI_QUIRK_EXTERNAL_CARD_DETECTION		(1 << 4)
#define MSHCI_QUIRK_WLAN_DETECTION		(1 << 5)
#define MSHCI_QUIRK_TUNING_ENABLE		(1 << 6)
#define MSHCI_QUIRK_TI_WL18XX   (1 << 7 )       //z00186406 add to give ti wl18xx a quirk

	int			irq;		/* Device IRQ */
	void __iomem		*ioaddr;	/* Mapped address */

	const struct mshci_ops	*ops;		/* Low level hw interface */

	/* Internal data */
	struct mmc_host		*mmc;		/* MMC structure */
	u64			dma_mask;	/* custom DMA mask */

	spinlock_t		lock;		/* Mutex */

	int			flags;		/* Host attributes */
#define MSHCI_USE_IDMA		(1 << 1)	/* Host is ADMA capable */
#define MSHCI_REQ_USE_DMA	(1 << 2)	/* Use DMA for this req. */
#define MSHCI_DEVICE_DEAD	(1 << 3)	/* Device unresponsive */
#define MSHCI_CARD_NEED_ENUM	(1 << 4)	/* movable card need re-enum */
#define MSHCI_IN_TUNING		(1 << 5)	/* Host is doing tuning */
#define MSHCI_TUNING_DONE	(1 << 6)	/* Host initialization tuning done */
#define MSHCI_POST_UNMAP	(1 << 8)	/* Post numap flag */
#define MSHCI_CMD_SEND_INIT	(1 << 9)	/* the next command should send initialization */
	unsigned int		tuning_rca;	/* card rca for tuning */

	int					tuning_move_start;	/* tuning move start flag */

	unsigned int		version;	/* SDHCI spec. version */

	struct clk              *pclk;          /*s00212129 emmc bus clock*/
	int                     clk_ref_counter;/*s00212129 emmc access reference counter*/
#define CLK_ENABLED             (1<<0)
#define CLK_DISABLED            (0<<0)

	unsigned int		max_clk;	/* Max possible freq (MHz) */
	unsigned int		timeout_clk;	/* Timeout freq (KHz) */

    unsigned int        clock_gate;
	unsigned int		clock;		/* Current clock (MHz) */
	unsigned int		clock_to_restore; /* Saved clock for dynamic clock gating (MHz) */
	u8			pwr;		/* Current voltage */

	struct mmc_request	*mrq;		/* Current request */
	struct mmc_command	*cmd;		/* Current command */
	struct mmc_data		*data;		/* Current data request */
	unsigned int		data_early:1;	/* Data finished before cmd */

	struct sg_mapping_iter	sg_miter;	/* SG state for PIO */
	unsigned int		blocks;		/* remaining PIO blocks */

	int			sg_count;	/* Mapped sg entries */

	u8			*idma_desc;	/* ADMA descriptor table */
	u8			*align_buffer;	/* Bounce buffer */

	dma_addr_t		idma_addr;	/* Mapped ADMA descr. table */
	dma_addr_t		align_addr;	/* Mapped bounce buffer */

	struct tasklet_struct	card_tasklet;	/* Tasklet structures */
	struct tasklet_struct	finish_tasklet;

	struct timer_list	timer;		/* Timer for timeouts */
	struct timer_list	clock_timer;	/* Timer for clock gating */

	u32			fifo_depth;
	u32			fifo_threshold;
	u32			data_transfered;
	u32			working;
#define MMC_HOST_BUSY            (1<<0)
#define MMC_HOST_FREE            (0<<0)
	struct work_struct       mshci_delete_timer_work;
	unsigned long		private[0] ____cacheline_aligned;
};

struct mshci_ops {
	void	(*set_clock)(struct mshci_host *host, unsigned int clock);

	int		(*enable_dma)(struct mshci_host *host);
	unsigned int	(*get_max_clock)(struct mshci_host *host);
	unsigned int	(*get_min_clock)(struct mshci_host *host);
	unsigned int	(*get_timeout_clock)(struct mshci_host *host);
	void		(*set_ios)(struct mshci_host *host,
				   struct mmc_ios *ios);
	int		(*get_ro)(struct mmc_host *mmc);
	void		(*init_issue_cmd)(struct mshci_host *host);
    unsigned int	(*get_present_status)(struct mshci_host *host);
    int (*start_signal_voltage_switch)(struct mshci_host *host, struct mmc_ios *ios);
	int		(*tuning_find_condition)(struct mshci_host *host);
	void	(*tuning_set_current_state)(struct mshci_host *host, int ok);
	int		(*tuning_move)(struct mshci_host *host, int start);
};

static inline void mshci_writel(struct mshci_host *host, u32 val, int reg)
{
	writel(val, host->ioaddr + reg);
}

static inline u32 mshci_readl(struct mshci_host *host, int reg)
{
	return readl(host->ioaddr + reg);
}

extern struct mshci_host *mshci_alloc_host(struct device *dev,
					   size_t priv_size);
extern void mshci_free_host(struct mshci_host *host);
extern void mshci_disconnect(struct mshci_host *host);

static inline void *mshci_priv(struct mshci_host *host)
{
	return (void *)host->private;
}

extern int mshci_add_host(struct mshci_host *host);
extern void mshci_remove_host(struct mshci_host *host, int dead);

#ifdef CONFIG_PM
extern int mshci_suspend_host(struct mshci_host *host, pm_message_t state);
extern int mshci_resume_host(struct mshci_host *host);
#endif

#endif /* __MSHCI_H */
