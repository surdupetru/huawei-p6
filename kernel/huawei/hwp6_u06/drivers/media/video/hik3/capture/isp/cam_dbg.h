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

#ifndef __CAM_DBG_H__
#define __CAM_DBG_H__

#include <linux/list.h>

/*#define DUMP_QUEUE */
/*#define DUMP_FILE */
/*#define DUMP_PREVIEW_FILE */
/*#define DUMP_SIZE_REG */
/*#define DUMP_CMD_REG */
/*#define REG_CSI_IRQ */

enum cam_dbg_info_type {
	DBG_INFO_ISP_BASE = 0,
	DBG_INFO_CSI0_BASE,
	DBG_INFO_CSI1_BASE,
};

typedef struct _cam_dbg_info_t {

	u32 dbg_isp_base_addr;
	u32 dbg_csi0_base_addr;
	u32 dbg_csi1_base_addr;

	/* FIXME: add infomation we need here */

} cam_dbg_info_t;

void register_cam_dbg_info(int dbg_info_type, void *context);
void dump_queue(struct list_head *list_queue, u32 flag);
void dump_file(char *filename, u32 addr, u32 size);
void dump_cmd_reg(void);
void dump_sensor_reg(u32 addr);
void dump_size_reg(void);
void dump_zoom_reg(void);
void dump_isp_reg(int start, int size);

#endif /*__CAM_DBG_H__ */

/********************** END ***********************/
