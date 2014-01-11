/*
 *  Hisilicon K3 soc camera v4l2 driver source file
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
#include <linux/err.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/stat.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "cam_dbg.h"
#include "cam_util.h"
/* #define DEBUG_DEBUG 1 */
#define LOG_TAG "K3_CAM_DBG"
#include "cam_log.h"
#include <mach/io_mutex.h>

#define DBG_BIG_ENDIAN(a) ((a)+3-2*((a)&0x3))

#define DBG_SETREG8(reg, value) \
	(((reg) >= 0x20000) ? \
	iowrite8_mutex((value), (cam_dbg_info.dbg_isp_base_addr + (reg))) : \
	iowrite8_mutex((value), (cam_dbg_info.dbg_isp_base_addr + DBG_BIG_ENDIAN(reg))))

#define DBG_GETREG8(reg) \
	(((reg) >= 0x20000) ?	\
	(*(volatile u8*)(cam_dbg_info.dbg_isp_base_addr + (reg))) : \
	(*(volatile u8*)(cam_dbg_info.dbg_isp_base_addr + DBG_BIG_ENDIAN (reg))))




static cam_dbg_info_t cam_dbg_info;

/*
 **************************************************************************
 * FunctionName: register_cam_dbg_info;
 * Description : NA;
 * Input       :
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void register_cam_dbg_info(int dbg_info_type, void *context)
{
	switch (dbg_info_type) {
	case DBG_INFO_ISP_BASE:
		{
			cam_dbg_info.dbg_isp_base_addr = (u32) context;
			print_info("cam_dbg_info.dbg_isp_base_addr=%#x", cam_dbg_info.dbg_isp_base_addr);
			break;
		}
	case DBG_INFO_CSI0_BASE:
		{
			cam_dbg_info.dbg_csi0_base_addr = (u32) context;
			print_info("cam_dbg_info.dbg_csi0_base_addr=%#x", cam_dbg_info.dbg_csi0_base_addr);
			break;
		}
	case DBG_INFO_CSI1_BASE:
		{
			cam_dbg_info.dbg_csi1_base_addr = (u32) context;
			print_info("cam_dbg_info.dbg_csi1_base_addr=%#x", cam_dbg_info.dbg_csi1_base_addr);
			break;
		}
	default:
		{
			print_error("%s invalid parameters", __func__);
			break;
		}
	}
}

/*
 **************************************************************************
 * FunctionName: dump_queue;
 * Description : NA;
 * Input       :
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void dump_queue(struct list_head *list_queue, u32 flag)
{
#ifdef DUMP_QUEUE
	camera_frame_buf *frame = NULL;
	struct list_head *pos = NULL;

	print_info("dump queue flag %d:", flag);
	list_for_each(pos, list_queue) {
		frame = list_entry(pos, camera_frame_buf, queue);
		if (frame)
			print_info("phyaddr:%#x, index:%d", frame->phyaddr, frame->index);
	}
#endif
}
/*
 **************************************************************************
 * FunctionName: dump_file;
 * Description : NA;
 * Input       :
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void dump_file(char *filename, u32 addr, u32 size)
{
#ifdef DUMP_FILE
	struct file *file1 = NULL;
	mm_segment_t old_fs;
	memset(&old_fs, 0x0, sizeof(old_fs));

	print_info("dumpfile %s with size %u", filename, size);
	if (filename == NULL) {
		print_error("dump file with NULL file name!");
		return;
	}
	file1 = filp_open(filename, O_CREAT | O_RDWR, 0644);
	if (IS_ERR(file1)) {
		print_error("error occured while opening file %s, exiting...\n", filename);
		return;
	} else {
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		file1->f_op->write(file1, (char *)addr, size, &file1->f_pos);
		set_fs(old_fs);
		filp_close(file1, NULL);
		return;
	}
#endif
}

/*
 **************************************************************************
 * FunctionName: dump_cmd_reg;
 * Description : NA;
 * Input       :
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void dump_cmd_reg(void)
{
#ifdef DUMP_CMD_REG
	int i = 0;

	/* command set register range : 0x1e900 ~ 0x1e9ff */
	for (i = 0x1e900; i < 0x1e9ff; i++)
		print_info("%#x=%#x", i, DBG_GETREG8(i));
	print_info("0x63901=%#x", DBG_GETREG8(0x63901));
	print_info("0x63902=%#x", DBG_GETREG8(0x63902));
	print_info("0x63903=%#x", DBG_GETREG8(0x63903));
	print_info("0x63904=%#x", DBG_GETREG8(0x63904));
	print_info("0x63905=%#x", DBG_GETREG8(0x63905));
	print_info("0x63906=%#x", DBG_GETREG8(0x63906));
	print_info("0x63907=%#x", DBG_GETREG8(0x63907));
	print_info("0x63908=%#x", DBG_GETREG8(0x63908));
	print_info("0x63909=%#x", DBG_GETREG8(0x63909));
	print_info("0x6390a=%#x", DBG_GETREG8(0x6390a));
#endif
}

/*
 **************************************************************************
 * FunctionName: dump_sensor_reg;
 * Description : NA;
 * Input       :
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void dump_sensor_reg(u32 addr)
{
	DBG_SETREG8(0x63602, (addr >> 8) & 0xff);
	DBG_SETREG8(0x63603, addr & 0xff);
	DBG_SETREG8(0x63609, 0x33);
	msleep(10);
	DBG_SETREG8(0x63609, 0xf9);
	msleep(4);
	print_info("dump sensor: 0x%0x 0x%0x", addr, DBG_GETREG8(0x63608));
}

/*
 **************************************************************************
 * FunctionName: dump_size_reg;
 * Description : NA;
 * Input       :
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void dump_size_reg(void)
{
#ifdef DUMP_SIZE_REG
	print_info("enter %s", __func__);
	print_info("0x63b34 is 0x%0x", DBG_GETREG8(0x63b34));

	print_info("=write img width=");
	print_info("0x63b36 is 0x%0x", DBG_GETREG8(0x63b36));
	print_info("0x63b37 is 0x%0x", DBG_GETREG8(0x63b37));
	print_info("=write img height=");
	print_info("0x63b3a is 0x%0x", DBG_GETREG8(0x63b3a));
	print_info("0x63b3b is 0x%0x", DBG_GETREG8(0x63b3b));
	print_info("=write mem width=");
	print_info("0x63b38 is 0x%0x", DBG_GETREG8(0x63b38));
	print_info("0x63b39 is 0x%0x", DBG_GETREG8(0x63b39));

	print_info("=isp input width=");
	print_info("0x65010 is 0x%0x", DBG_GETREG8(0x65010));
	print_info("0x65011 is 0x%0x", DBG_GETREG8(0x65011));
	print_info("=isp input height=");
	print_info("0x65012 is 0x%0x", DBG_GETREG8(0x65012));
	print_info("0x65013 is 0x%0x", DBG_GETREG8(0x65013));

	print_info("=raw scale down=");
	print_info("0x65000 is 0x%0x", DBG_GETREG8(0x65000));
	print_info("0x65080 is 0x%0x", DBG_GETREG8(0x65080));
	print_info("=raw scale output size=");
	print_info("0x65032 is 0x%0x", DBG_GETREG8(0x65032));
	print_info("0x65033 is 0x%0x", DBG_GETREG8(0x65033));
	print_info("0x65034 is 0x%0x", DBG_GETREG8(0x65034));
	print_info("0x65035 is 0x%0x", DBG_GETREG8(0x65035));

	print_info("=yuv scale down switch=");
	print_info("0x65002 is 0x%0x", DBG_GETREG8(0x65002));
	print_info("=yuv dcw=");
	print_info("0x65023 is 0x%0x", DBG_GETREG8(0x65023));
	print_info("=yuv scale down=");
	print_info("0x65024 is 0x%0x", DBG_GETREG8(0x65024));
	print_info("0x65025 is 0x%0x", DBG_GETREG8(0x65025));

	print_info("=yuv scale up=");
	print_info("0x65026 is 0x%0x", DBG_GETREG8(0x65026));
	print_info("0x65027 is 0x%0x", DBG_GETREG8(0x65027));
	print_info("0x65028 is 0x%0x", DBG_GETREG8(0x65028));
	print_info("0x65029 is 0x%0x", DBG_GETREG8(0x65029));

	print_info("yuv scale out size");
	print_info("0x65014 is 0x%0x", DBG_GETREG8(0x65014));
	print_info("0x65015 is 0x%0x", DBG_GETREG8(0x65015));
	print_info("0x65016 is 0x%0x", DBG_GETREG8(0x65016));
	print_info("0x65017 is 0x%0x", DBG_GETREG8(0x65017));

	print_info("=yuv crop size=");
	print_info("0x650f4 is 0x%0x", DBG_GETREG8(0x650f4));
	print_info("0x650f5 is 0x%0x", DBG_GETREG8(0x650f5));
	print_info("0x650f6 is 0x%0x", DBG_GETREG8(0x650f6));
	print_info("0x650f7 is 0x%0x", DBG_GETREG8(0x650f7));

	print_info("=image vsize for 3d mode=");
	print_info("0x63106 is 0x%0x", DBG_GETREG8(0x63106));
	print_info("0x63107 is 0x%0x", DBG_GETREG8(0x63107));
	print_info("= IDI size=");
	print_info("0x63c04 is 0x%0x", DBG_GETREG8(0x63c04));
	print_info("0x63c05 is 0x%0x", DBG_GETREG8(0x63c05));
	print_info("0x63c06 is 0x%0x", DBG_GETREG8(0x63c06));
	print_info("0x63c07 is 0x%0x", DBG_GETREG8(0x63c07));
#endif
}

void dump_isp_reg(int start, int size)
{
	int reg_addr;

	for (reg_addr = start; reg_addr < (start + size); reg_addr++) {
		if ((reg_addr % 16) == 0)
			printk("0x%5x:", reg_addr);

		printk("0x%2x ", DBG_GETREG8(reg_addr));

		if (((reg_addr + 1) % 16) == 0)
			printk("\n");
	}
}

/************************* END ******************************/
