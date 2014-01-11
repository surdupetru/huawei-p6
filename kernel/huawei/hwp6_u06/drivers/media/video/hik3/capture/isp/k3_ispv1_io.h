/*
 *  Hisilicon K3 soc camera ISP driver header file
 *
 *  CopyRight (C) Hisilicon Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __K3_ISPV1_IO__
#define __K3_ISPV1_IO__
#include <linux/mux.h>
#include "k3_isp_io.h"
#define SENSOR_LDO_COUNT  4
#define IOMUX_DVP_NAME "block_isp_dvp"
#define IOMUX_ISP_NAME "block_isp"
#define IOMUX_I2C_NAME "block_isp_i2c"
#define IOMUX_RST_NAME "block_isp_reset"

typedef struct _isp_io_hw {

	struct platform_device *pdev;

	u32 csi0_irq;
	u32 csi1_irq;

	struct clk *phy1_clk;
	struct clk *phy2_clk;

	u32 csi0_base;
	u32 csi1_base;

	sensor_ldo_t ldo[SENSOR_LDO_COUNT];

	u32 ldo_count;

	struct iomux_block *block_i2c;
	struct iomux_block *block_isp;
	struct iomux_block *block_dvp;
	struct iomux_block *block_rst;
	struct block_config *block_conf_i2c;
	struct block_config *block_conf_isp;
	struct block_config *block_conf_dvp;
	struct block_config *block_conf_rst;

} ispio_hw_t;

#endif /* __K3_ISPV1_IO__ */
