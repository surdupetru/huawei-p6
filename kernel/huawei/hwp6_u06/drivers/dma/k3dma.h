/*
 * History:
 * 2011-06-01 create this file
 */
#ifndef K3_DMA_REGS_H
#define K3_DMA_REGS_H

#include <mach/dma.h>

/*definition for DMAC registers*/
#define			INT_STAT_OFFSET			0x00
#define			INT_TC1_OFFSET			0x04
#define			INT_TC2_OFFSET			0x08
#define			INT_ERR1_OFFSET			0x0C
#define			INT_ERR2_OFFSET			0x10
#define			INT_ERR3_OFFSET			0x14
#define			INT_TC1_MASK_OFFSET		0x18
#define			INT_TC2_MASK_OFFSET		0x1C
#define			INT_ERR1_MASK_OFFSET	0x20
#define			INT_ERR2_MASK_OFFSET	0x24
#define			INT_ERR3_MASK_OFFSET	0x28
#define			INT_TC1_RAW_OFFSET		0x600
#define			INT_TC2_RAW_OFFSET		0x608
#define			INT_ERR1_RAW_OFFSET		0x610
#define			INT_ERR2_RAW_OFFSET		0x618
#define			INT_ERR3_RAW_OFFSET		0x620

#define			SREQ_OFFSET				0x660
#define			LSREQ_OFFSET			0x664
#define			BREQ_OFFSET				0x668
#define			LBREQ_OFFSET			0x66C
#define			FREQ_OFFSET				0x670
#define			LFREQ_OFFSET			0x674

#define			CH_PRI_OFFSET			0x688
#define			CH_STAT_OFFSET			0x690
#define			CTRL_OFFSET				0x698

#define			CX_CURR_CNT1_OFFSET(x)		(0x700+x*0x10)
#define			CX_CURR_CNT0_OFFSET(x)		(0x704+x*0x10)
#define			CX_CURR_SRC_ADDR_OFFSET(x)	(0x708+x*0x10)
#define			CX_CURR_DES_ADDR_OFFSET(x)	(0x70C+x*0x10)

/*Channel x base addr*/
#define			ch_regs(x)			(0x800 + (x) * 0x40)
#define			CX_LLI_OFFSET			0x00
#define			CX_BINDX_OFFSET			0x04
#define			CX_CINDX_OFFSET			0x08
#define			CX_CNT1_OFFSET			0x0C
#define			CX_CNT0_OFFSET			0x10
#define			CX_SRC_ADDR_OFFSET		0x14
#define			CX_DES_ADDR_OFFSET		0x18
#define			CX_CONFIG_OFFSET		0x1C
#define			CX_AXI_CONF_OFFSET		0x20

/*definition for channel config*/
#define			K3_DMA_FC_MEM2MEM			(0x0 << 2)
#define			K3_DMA_FC_MEM2PER			(0x1 << 2)
#define			K3_DMA_FC_PER2MEM			(0x2 << 2)

#define			K3_DMA_SRC_ADDR_MODE_INCR	(0x1 << 31)
#define			K3_DMA_SRC_ADDR_MODE_FIXED	(0x0 << 31)
#define			K3_DMA_DST_ADDR_MODE_INCR	(0x1 << 30)
#define			K3_DMA_DST_ADDR_MODE_FIXED	(0x0 << 30)

/*definition for others*/
#define			K3_DMA_SLAVE_BURST_SIZE_MAX	  16
#define			K3_DMA_SLAVE_BURST_UNDEFINED  0
#define			K3_DMA_MAX_NR_SLAVE_CHANNELS  32
#define			K3_DMA_BTSIZE_MAX			  0x2000
#define			K3_DMA_DEFAULT_CONFIG		  0xCFF33001
#define			K3_DMA_DEFAULT_AXI			  0x201201
#define			K3_DMA_ALIGN				  32
#define			K3_DMA_CONFIG_ENABLE		  0x1
#define			K3_DMA_LLI_MASK1              0xFFFFFFE0
#define			K3_DMA_LLI_MASK2			  0x02

/*definition for reg read and write*/
#define			dma_reg_write(reg, value)	writel(value, reg)
#define			dma_reg_read(reg)		readl(reg)

/*definition for channel state*/
enum k3dma_chan_state {
	K3DMA_CHAN_IDLE			= 0,
	K3DMA_CHAN_SUCCESS,
	K3DMA_CHAN_PAUSED,
	K3DMA_CHAN_ERROR,
};

/*definition for channel error type*/
enum k3dma_error_type {
	K3DMA_NO_ERROR			= 0,
	K3DMA_ERROR_CONFIG,
	K3DMA_ERROR_TRANS,
};

/*--  Channels	--------------------------------------------------------*/
/**
 * struct k3dma_phy_chan - internal representation of hidmav300 channel
 * @chan_id: index of channel
 * @ch_regs: memory mapped register base
 * @serving: virtual channel which phy channel serving for
 */
struct k3dma_phy_chan {
	u32 chan_id;
	void __iomem			*ch_regs;
	struct k3dma_chan		*serving;

	spinlock_t				lock;
};

struct k3dma_desc_page {
	struct list_head   pg_node;
	unsigned char * page_link;
};

/**
 * struct k3dma_chan - internal representation of an hidmav300 slave channel
 * @chan_common: common dmaengine channel object members
 * @phychan: target phy channel
 * @at: running descriptor
 * @slave_config: store config data
 * @cd: store channel data from platform
 * @error_status: transmit error status information from irq handler
 *				  to tasklet (use atomic operations)
 * @tasklet: bottom half to finish transaction work
 * @state: to flag dma status
 * @lc: identifier for the last completed operation
 * @errcode: to flag error type
 * @error_status: occur error
 * @phychan_hold: to hold phy channel so others can not use
 * @pend_list: list of descriptors dmaengine is being running on
 * @free_list: list of descriptors usable by the channel
 * @descs_allocated: records the actual size of the descriptor pool
 * @cctl: channel control bit
 */
struct k3dma_chan {
	struct dma_chan				chan_common;
	struct k3dma_phy_chan		*phychan;
	struct k3dma_desc			*at;
	struct dma_slave_config		slave_config;
	struct tasklet_struct		tasklet;
	struct k3dma_channel_data		cd;
	struct list_head			free_list;
	struct list_head			pend_list;

	enum k3dma_chan_state		state;
	enum k3dma_error_type		errtype;

	u32				error_status;
	u32				phychan_hold;
	u32				descs_allocated;
	u32				cctl;
	u32				ref_count;
	dma_cookie_t	lc;
	spinlock_t		lock;
	struct list_head           page_list;
};


/*--  Controller  ------------------------------------------------------*/

/**
 * struct k3dma_device - internal representation of an hidmav300 Controller
 * @dma_common: common dmaengine dma_device object members
 * @regs: memory mapped register base
 * @peri_ctrl0_reg: control reg for acp and clock gate
 * @clk: dma controller clock
 * @all_chan_mask: all channels availlable in a mask
 * @dma_desc_pool: base of DMA descriptor region (DMA address)
 * @phy_chans: channels table to store k3dma_phy_chan structures
 * @dma_chans: channels table to store k3dma_chan structures
 */
struct k3dma_device {
	struct dma_device			dma_common;
	void __iomem				*regs;
	struct clk					*clk;
	struct clk					*clk_acp;
	struct dma_pool				*dma_desc_pool;
	struct k3dma_phy_chan		*phy_chans;
	struct k3dma_chan			*dma_chans;
	struct k3dma_platform_data	*pd;

	u32					all_chan_mask;
	bool				hasacp;

	spinlock_t			lock;
};



/*--  descriptors  -----------------------------------------------------*/

/* *
*@lli:next lli phys
*@bindx:offset for two-denmention trans
*@cindx:offset for three-denmention trans
*@cnt1:trans count
*@cnt0:trans count
*@saddr:lli source phys
*@daddr:lli dest phys
*@confog:config reg for trans
*/
struct k3dma_lli {
	u32				lli;
	u32				bindx;
	u32				cindx;
	u32				cnt1;
	u32				cnt0;
	u32				saddr;
	u32				daddr;
	u32				config;
};
/**
 * struct k3dma_desc - software descriptor
 * @k3dma_lli: hardware lli structure
 * @txd: support for the async_tx api
 * @tx_list: store descs for one trans
 * @desc_node: node on the channed descriptors list
 * @len: one lli bytecount
 * @total: total bytecounts for one trans
 */
struct k3dma_desc {
	/* FIRST values the hardware uses */
	struct k3dma_lli				lli;

	/* THEN values for driver housekeeping */
	struct list_head				tx_list;
	struct list_head				desc_node;
	struct dma_async_tx_descriptor	txd;

	u32 len;
	u32 total;
};
#endif
