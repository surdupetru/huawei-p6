/*
 * kernel/drivers/input/touchscreen/synaptics_i2c_rmi4.c
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
/*#define DEBUG 1*/
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/synaptics_i2c_rmi4.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mux.h>
#include <mach/gpio.h>
#include <mach/platform.h>

#define BOARD_T0

/* Register: EGR_0 */
#define EGR_PINCH_REG		0
#define EGR_PINCH			(1 << 6)
#define EGR_PRESS_REG 		0
#define EGR_PRESS			(1 << 5)
#define EGR_FLICK_REG 		0
#define EGR_FLICK			(1 << 4)
#define EGR_EARLY_TAP_REG	0
#define EGR_EARLY_TAP		(1 << 3)
#define EGR_DOUBLE_TAP_REG	0
#define EGR_DOUBLE_TAP		(1 << 2)
#define EGR_TAP_AND_HOLD_REG	0
#define EGR_TAP_AND_HOLD	(1 << 1)
#define EGR_SINGLE_TAP_REG	0
#define EGR_SINGLE_TAP		(1 << 0)
/* Register: EGR_1 */
#define EGR_PALM_DETECT_REG	1
#define EGR_PALM_DETECT		(1 << 0)

#define MASK_7BIT		0x7F
#define MASK_5BIT		0x1F
#define MASK_4BIT		0x0F
#define MASK_3BIT		0x07
#define MASK_2BIT		0x03

#define PDT_START_SCAN_LOCATION	0xE9
#define PDT_END_SCAN_LOCATION	0x05
#define PDT_ENTRY_SIZE			6
#define BTN_F30					BTN_0
#define RMI4_NUMBER_OF_MAX_FINGERS	10

#define INTR_SRC_COUNT(x) (x & 7)

struct synaptics_function_descriptor {
	unsigned char query_base_addr;
	unsigned char cmd_base_addr;
	unsigned char ctrl_base_addr;
	unsigned char data_base_addr;
	unsigned char int_src_count;
	unsigned char func_num;
};

static struct synaptics_function_descriptor fd_01;
static struct synaptics_function_descriptor fd_11;
static struct synaptics_function_descriptor fd_34;
static int fake_synaptics_rmi4_probe(void);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_rmi4_early_suspend(struct early_suspend *h);
static void synaptics_rmi4_late_resume(struct early_suspend *h);
#endif

static int synaptics_rmi4_read_pdt(struct synaptics_rmi4 *ts)
{
	int ret = 0;
	int nFd = 0; /*number of functions*/
	int intr_count = 0;
	unsigned char data_length = 0;
	unsigned char fd_reg;
	unsigned char query[14];
	unsigned char *egr;

	struct i2c_msg fd_i2c_msg[2];
	struct i2c_msg query_i2c_msg[2];
	struct synaptics_function_descriptor fd;

	fd_i2c_msg[0].addr = ts->client->addr;
	fd_i2c_msg[0].flags = 0;
	fd_i2c_msg[0].buf = &fd_reg;
	fd_i2c_msg[0].len = 1;

	fd_i2c_msg[1].addr = ts->client->addr;
	fd_i2c_msg[1].flags = I2C_M_RD;
	fd_i2c_msg[1].buf = (unsigned char *)(&fd);
	fd_i2c_msg[1].len = PDT_ENTRY_SIZE;

	query_i2c_msg[0].addr = ts->client->addr;
	query_i2c_msg[0].flags = 0;
	query_i2c_msg[0].buf = &fd.query_base_addr;
	query_i2c_msg[0].len = 1;

	query_i2c_msg[1].addr = ts->client->addr;
	query_i2c_msg[1].flags = I2C_M_RD;
	query_i2c_msg[1].buf = query;
	query_i2c_msg[1].len = sizeof(query);

	ts->hasF11 = false;
	ts->hasF19 = false;
	ts->hasF30 = false;
	ts->data_reg = 0xff;
	ts->data_length = 0;

	for (fd_reg = PDT_START_SCAN_LOCATION; fd_reg >= PDT_END_SCAN_LOCATION;
			fd_reg -= PDT_ENTRY_SIZE) {
		ret = i2c_transfer(ts->client->adapter, fd_i2c_msg, 2);
		if (ret < 0) {
			dev_err(&ts->client->dev, "%s: I2C read failed querying RMI4 $%02X capabilities\n",
				__func__, ts->client->addr);
			return ret;
		}
		if (!fd.func_num) {
			/* End of PDT */
			ret = nFd;
			break;
		}

		++nFd;

		switch (fd.func_num) {
		/* Function $34: Flash Memory Management */
		case 0x34:
			fd_34.query_base_addr = fd.query_base_addr;
			fd_34.data_base_addr = fd.data_base_addr;

			/*Debug: */
			dev_dbg(&ts->client->dev, "%s: fd.34.query_base_addr = %x\n",
				__func__, fd_34.query_base_addr);
			dev_dbg(&ts->client->dev, "%s: fd.34.data_base_addr = %x\n",
				__func__, fd_34.data_base_addr);

			break;

		/* Function $01: RMI Devcice Control */
		case 0x01:
			ts->f01.data_offset = fd.data_base_addr;
			fd_01.query_base_addr = fd.query_base_addr;
			fd_01.data_base_addr = fd.data_base_addr;
			fd_01.cmd_base_addr = fd.cmd_base_addr;
			fd_01.ctrl_base_addr = fd.ctrl_base_addr;
			/*
			 * Can't determine data_length
			 * until whole PDT has been read to count interrupt sources
			 * and calculate number of interrupt status registers.
			 * Setting to 0 safely "ignores" for now.
			 */
			data_length = 0;

			/*Debug: */
			dev_dbg(&ts->client->dev, "%s: fd_01.query_base_addr = %x\n",
				__func__, fd_01.query_base_addr);
			dev_dbg(&ts->client->dev, "%s: fd_01.data_base_addr = %x\n",
				__func__, fd_01.data_base_addr);
			dev_dbg(&ts->client->dev, "%s: fd_01.cmd_base_addr = %x\n",
				__func__, fd_01.cmd_base_addr);
			dev_dbg(&ts->client->dev, "%s: fd_01.ctrl_base_addr = %x\n",
				__func__, fd_01.ctrl_base_addr);

			break;

		/* Function $11: 2D Sensors (Touchscreen) */
		case 0x11:
			fd_11.ctrl_base_addr = fd.ctrl_base_addr;
			ts->hasF11 = true;
			ts->f11.data_offset = fd.data_base_addr;
			ts->f11.interrupt_offset = intr_count / 8;
			ts->f11.interrupt_mask = ((1 << INTR_SRC_COUNT(fd.int_src_count)) - 1)
					<< (intr_count % 8);

			/* Read F11 query registers*/
			ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
			if (ret < 0) {
				dev_err(&ts->client->dev, "%s: failed to read F11 query registers\n", __func__);
				return ret;
			}

			ts->f11.points_supported = (query[1] & MASK_3BIT) + 1;
			if (ts->f11.points_supported == 6)
				ts->f11.points_supported = 10;

			ts->f11_fingers = kcalloc(ts->f11.points_supported,
				sizeof(*ts->f11_fingers), GFP_NOWAIT);

			ts->f11_has_relative = (query[1] >> 3) & 1;
			ts->f11_has_gestures = (query[1] >> 5) & 1;

			/* if the sensitivity adjust exist
			 * if f11_has_Sensitivity_Adjust=1, register map can set bit f11_2d_ctrl14
			 */
			ts->f11_has_Sensitivity_Adjust = (query[1] >> 6) & 1;
			/* egr point to F11_2D_Query7*/
			egr = &query[7];

			ts->hasEgrPinch = egr[EGR_PINCH_REG] & EGR_PINCH;
			ts->hasEgrPress = egr[EGR_PRESS_REG] & EGR_PRESS;
			ts->hasEgrFlick = egr[EGR_FLICK_REG] & EGR_FLICK;
			ts->hasEgrEarlyTap = egr[EGR_EARLY_TAP_REG] & EGR_EARLY_TAP;
			ts->hasEgrDoubleTap = egr[EGR_DOUBLE_TAP_REG] & EGR_DOUBLE_TAP;
			ts->hasEgrTapAndHold = egr[EGR_TAP_AND_HOLD_REG] & EGR_TAP_AND_HOLD;
			ts->hasEgrSingleTap = egr[EGR_SINGLE_TAP_REG] & EGR_SINGLE_TAP;
			ts->hasEgrPalmDetect = egr[EGR_PALM_DETECT_REG] & EGR_PALM_DETECT;/*(F11_2D_Query8, bit 0)*/

			/* Read f11 control register*/
			query_i2c_msg[0].buf = &fd.ctrl_base_addr;
			ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
			if (ret < 0)
				dev_err(&ts->client->dev, "Error reading F11 control registers\n");

			query_i2c_msg[0].buf = &fd.query_base_addr;

			ts->f11_max_x = ((query[7] & MASK_4BIT) << 8) | query[6];
			ts->f11_max_y = ((query[9] & MASK_4BIT) << 8) | query[8];

			ts->f11.data_length = data_length =
				/* finger status, four fingers per register */
				((ts->f11.points_supported + 3) / 4)
				/* absolute data, 5 per finger */
				+ 5 * ts->f11.points_supported
				/* two relative registers */
				+ (ts->f11_has_relative ? 2 : 0)
				/* F11_2D_Data8 is only present if the egr_0 register is non-zero. */
				+ (egr[0] ? 1 : 0)
				/* F11_2D_Data9 is only present if either egr_0 or egr_1 registers are non-zero. */
				+ ((egr[0] || egr[1]) ? 1 : 0)
				/* F11_2D_Data10 is only present if EGR_PINCH or EGR_FLICK of egr_0 reports as 1. */
				+ ((ts->hasEgrPinch || ts->hasEgrFlick) ? 1 : 0)
				/* F11_2D_Data11 and F11_2D_Data12 are only present if EGR_FLICK of egr_0 reports as 1. */
				+ (ts->hasEgrFlick ? 2 : 0);

			/*Debug: */
			dev_dbg(&ts->client->dev, "%s: fd_11.ctrl_base_addr = %x\n",
				__func__, fd_11.query_base_addr);
			dev_dbg(&ts->client->dev, "%s: fd_11.data_base_addr = %x\n",
				__func__, fd_11.data_base_addr);
			dev_dbg(&ts->client->dev, "%s: fd_11.cmd_base_addr = %x\n",
				__func__, fd_11.cmd_base_addr);
			dev_dbg(&ts->client->dev, "%s: fd_11.ctrl_base_addr = %x\n",
				__func__, fd_11.ctrl_base_addr);
			dev_dbg(&ts->client->dev, "%s: f11_max_x = %d\n",
				__func__, ts->f11_max_x);
			dev_dbg(&ts->client->dev, "%s: f11_max_y = %d\n",
				__func__, ts->f11_max_y);
			break;

		/* Function $30: LED/GPIO Control */
		case 0x30:
			ts->hasF30 = true;
			ts->f30.data_offset = fd.data_base_addr;
			ts->f30.interrupt_offset = intr_count / 8;
			ts->f30.interrupt_mask = ((1 < INTR_SRC_COUNT(fd.int_src_count)) - 1)
						<< (intr_count % 8);

			ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
			if (ret < 0) {
				dev_err(&ts->client->dev, "%s: failed to read F30 query registers\n", __func__);
				return ret;
			}

			ts->f30.points_supported = query[1] & MASK_5BIT;
			ts->f30.data_length = data_length = (ts->f30.points_supported + 7) / 8;
			break;

		default:
			goto pdt_next_iter;
		}

		/* Change to end address for comparison
		 *NOTE: make sure final value of ts->data_reg is subtracted */
		data_length += fd.data_base_addr;
		if (data_length > ts->data_length)
			ts->data_length = data_length;

		if (fd.data_base_addr < ts->data_reg)
			ts->data_reg = fd.data_base_addr;

pdt_next_iter:
		intr_count += INTR_SRC_COUNT(fd.int_src_count);
	}

	/* Now that PDT has been read,interrupt count determined,
	 * F01 data length can be determined.
	 */
	ts->f01.data_length = data_length = 1 + ((intr_count + 7) / 8);
	/* Change to end address for comparison
	 *NOTE: make sure final value of ts->data_reg is subtracted*/
	data_length += ts->f01.data_offset;
	if (data_length > ts->data_length)
		ts->data_length = data_length;

	/*Change data_length back from end address to length
	 *NOTE: make sure this was an address*/
	ts->data_length -= ts->data_reg;

	/*Change all data offsets to be relative to first register read */
	ts->f01.data_offset -= ts->data_reg;
	ts->f11.data_offset -= ts->data_reg;
	ts->f19.data_offset -= ts->data_reg;
	ts->f30.data_offset -= ts->data_reg;

	ts->data = kcalloc(ts->data_length, sizeof(*ts->data), GFP_NOWAIT);
	if (ts->data == NULL) {
		dev_err(&ts->client->dev, "%s: failed to allocate space for RMI4 data\n",
				__func__);
		ret = -ENOMEM;
	}

	/*Config i2c msg for later use*/
	ts->data_i2c_msg[0].addr = ts->client->addr;
	ts->data_i2c_msg[0].flags = 0;
	ts->data_i2c_msg[0].len = 1;
	ts->data_i2c_msg[0].buf = &ts->data_reg;

	ts->data_i2c_msg[1].addr = ts->client->addr;
	ts->data_i2c_msg[1].flags = I2C_M_RD;
	ts->data_i2c_msg[1].len = ts->data_length;
	ts->data_i2c_msg[1].buf = ts->data;

	dev_dbg(&ts->client->dev, "%s: RMI4 $%02X data read: $%02X + %d\n", __func__,
		ts->client->addr, ts->data_reg, ts->data_length);

	return ret;
}

static int synaptics_rmi4_work_func(struct work_struct *work)
{
	unsigned char finger_status = 0x00;
	unsigned char *finger_reg = NULL;
	unsigned char *interrupt = NULL;
	unsigned char reg = 0;
	int x[RMI4_NUMBER_OF_MAX_FINGERS];
	int y[RMI4_NUMBER_OF_MAX_FINGERS];
	int wx[RMI4_NUMBER_OF_MAX_FINGERS];
	int wy[RMI4_NUMBER_OF_MAX_FINGERS];
	int z[RMI4_NUMBER_OF_MAX_FINGERS];
	int ret = 0;
	int f = 0;
	/*int data_offset = 0;*/
	int touch_count = 0;

	struct synaptics_rmi4 *ts = container_of(work, struct synaptics_rmi4, work);
	struct synaptics_rmi4_platform_data *platdata = ts->client->dev.platform_data;

	ret = i2c_transfer(ts->client->adapter, ts->data_i2c_msg, 2);
	if (ret < 0) {
		dev_err(&ts->client->dev, "%s: i2c_transfer failed\n", __func__);
		return 0;
	}

	interrupt = &ts->data[ts->f01.data_offset + 1];

	if (ts->hasF11 && (interrupt[ts->f11.interrupt_offset] & ts->f11.interrupt_mask)) {

		unsigned char *f11_data = &ts->data[ts->f11.data_offset];
		unsigned char finger_status_reg = 0;
		unsigned char finger_status_reg_len = (ts->f11.points_supported + 3)  >> 2;

		for (f = 0; f < ts->f11.points_supported; ++f) {
			if (!(f % 4))
				finger_status_reg = f11_data[f >> 2];

			finger_status = (finger_status_reg >> ((f % 4)  << 1)) & MASK_2BIT;
			if (finger_status == 1 || finger_status == 2) {
				/* abs finger data offset */
				reg = finger_status_reg_len + 5 * f;
				/* abs finger data register */
				finger_reg = &f11_data[reg];

				x[touch_count] = (finger_reg[0] << 4) | (finger_reg[2] & MASK_4BIT);
				y[touch_count] = (finger_reg[1] << 4) | (finger_reg[2] >> 4);
				wx[touch_count] = finger_reg[3] & MASK_4BIT;
				wy[touch_count] = finger_reg[3] >> 4;
				z[touch_count] = finger_reg[4];

				/*the num is covert into physics num*/
				x[touch_count] = x[touch_count] * (platdata->x_res) / (ts->f11_max_x);
				y[touch_count] = y[touch_count] * (platdata->y_all) / (ts->f11_max_y);

				if (platdata->flip_flags & SYNAPTICS_FLIP_X)
					x[touch_count] = platdata->x_res - x[touch_count];
				if (platdata->flip_flags & SYNAPTICS_FLIP_Y)
					y[touch_count] = platdata->y_all - y[touch_count];

				/*dev_info(&ts->client->dev, "%s: finger%d: x = 0x%x, y = 0x%x, z = 0x%x\n",
					__func__, f, x[touch_count], y[touch_count], z[touch_count]);*/

				touch_count++;
				ts->f11_fingers[f].status = finger_status;


			}
		}

		/* report the data */
		if (touch_count) {
			input_report_abs(ts->input_dev, ABS_X, x[0]);
			input_report_abs(ts->input_dev, ABS_Y, y[0]);
			input_report_abs(ts->input_dev, ABS_PRESSURE, z[0]);
			input_report_abs(ts->input_dev, ABS_TOOL_WIDTH, max(wx[0], wy[0]));
			/*input_report_key(ts->input_dev, BTN_TOUCH, ts->f11_fingers[f].status);*/
			input_report_key(ts->input_dev, BTN_TOUCH,  1);

			if (ts->is_support_multi_touch) {
				for (f = 0; f != touch_count; f++) {
					input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, f);
					input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x[f]);
					input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y[f]);
					input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, z[f]);
					input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, max(wx[f], wy[f]));
					input_report_abs(ts->input_dev, ABS_MT_ORIENTATION, (wx[f] > wy[f] ? 1 : 0));
					/*input_report_key(ts->input_dev, BTN_TOUCH, ts->f11_fingers[f].status);*/
					input_mt_sync(ts->input_dev);
					/* Debug:*/
					/*dev_info(&ts->client->dev, "%s: 22222finger%d: x = 0x%x, y = 0x%x, z = 0x%x\n", __func__, f, x[f], y[f], z[f]);*/
				}
			}
		} else{
			input_report_key(ts->input_dev, BTN_TOUCH,  0);
			input_report_abs(ts->input_dev, ABS_PRESSURE, 0);
			input_mt_sync(ts->input_dev);
		}

/* EGR report not support*/
 #if 0
		/* f == ts->f11.points_supported */
		/* set f to offset after all absolute data */
		data_offset = (f + 3) / 4 + f * 5;
		if (ts->f11_has_relative) {
			/* NOTE: not reporting relative data, even if available */
			/* just skipping over relative data registers */
			data_offset += 2;
		}

		if (ts->hasEgrPalmDetect)  {
			input_report_key(ts->input_dev, BTN_DEAD,
				f11_data[data_offset + EGR_PALM_DETECT_REG] & EGR_PALM_DETECT);
		}

		if (ts->hasEgrFlick) {
			if (f11_data[data_offset + EGR_FLICK_REG] & EGR_FLICK) {
					input_report_rel(ts->input_dev, REL_X, f11_data[data_offset + 2]);
					input_report_rel(ts->input_dev, REL_Y, f11_data[data_offset + 3]);
			}
		}

		if (ts->hasEgrSingleTap) {
			input_report_key(ts->input_dev, BTN_TOUCH,
				f11_data[data_offset + EGR_SINGLE_TAP_REG] & EGR_SINGLE_TAP);
		}

		if (ts->hasEgrDoubleTap) {
			input_report_key(ts->input_dev, BTN_TOOL_DOUBLETAP,
				f11_data[data_offset + EGR_DOUBLE_TAP_REG] & EGR_DOUBLE_TAP);
		}
#endif

	} else  {

	#if 1
		/*if the irq is not for F11, then clear it*/
		int i = 0;
		for (i = 5; i > 0 ; i--) {
			/*read the irq Interrupt Status*/
			if (i2c_smbus_read_byte_data(ts->client, fd_01.data_base_addr+1) >= 0)
				break;
		}
	#endif
	}

#if 0
	if (ts->hasF19 && (interrupt[ts->f19.interrupt_offset] & ts->f19.interrupt_mask)) {
		int reg;
		int touch = 0;
		for (reg = 0; reg < ((ts->f19.points_supported + 7) / 8); reg++) {
			if (ts->data[ts->f19.data_offset + reg]) {
				touch = 1;
				break;
			}
		}
		input_report_key(ts->input_dev, BTN_DEAD, touch);
	}
#endif
	input_sync(ts->input_dev);

	if (ts->use_irq)
		enable_irq(ts->client->irq);

	return touch_count;
}

static enum hrtimer_restart synaptics_rmi4_timer_func(struct hrtimer *timer)
{
	struct synaptics_rmi4 *ts = container_of(timer, struct synaptics_rmi4, timer);
	queue_work(ts->synaptics_wq, &ts->work);
	hrtimer_start(&ts->timer, ktime_set(0, 12 * NSEC_PER_MSEC), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

irqreturn_t synaptics_rmi4_irq_handler(int irq, void *dev_id)
{
	struct synaptics_rmi4 *ts = (struct synaptics_rmi4 *)dev_id;
	disable_irq_nosync(ts->client->irq);
	queue_work(ts->synaptics_wq, &ts->work);
	return IRQ_HANDLED;
}

/*set the touch sensor on*/
static int synaptics_rmi4_power(struct i2c_client *client, bool on)
{
	int ret = 0;
	if (on) {
		/*sensor on*/
	#if 0
		ret = i2c_smbus_read_byte_data(client, fd_01.ctrl_base_addr);
		if (ret < 0) {
			dev_err(&client->dev, "%s: failed to read f01 ctrl regisger\n", __func__);
			return ret;
		}

		ret &= 0xFC;
		ret = i2c_smbus_write_byte_data(client, fd_01.ctrl_base_addr, ret);
	#endif
		ret = i2c_smbus_write_byte_data(client, fd_01.ctrl_base_addr, 0x80);
		if (ret < 0) {
			dev_err(&client->dev, "%s: synaptics sensor can not wake up\n", __func__);
			return ret;
		}

		/*touchscreen reset*/
		ret = i2c_smbus_write_byte_data(client, fd_01.cmd_base_addr, 0x01);
		if (ret < 0) {
			dev_err(&client->dev, "%s: synaptics chip can not reset\n", __func__);
			return ret;
		}

		/* wait for device reset; */
		msleep(200);

	} else {

		/* set touchscreen to deep sleep mode*/
		ret = i2c_smbus_read_byte_data(client, fd_01.ctrl_base_addr);
		if (ret < 0) {
			dev_err(&client->dev, "%s: failed to read f01 ctrl regisger\n", __func__);
			return ret;
		}

		ret = (ret & 0xFC) | 0x1;
		ret = i2c_smbus_write_byte_data(client, fd_01.ctrl_base_addr, ret);
		if (ret < 0) {
			dev_err(&client->dev, "%s: synaptics touch can not enter sleep mode\n", __func__);
			return ret;
		}
	}

	return 0;
}

static int synaptics_rmi4_config_input_dev(struct i2c_client *client)
{
	struct synaptics_rmi4 *ts = i2c_get_clientdata(client);
	struct synaptics_rmi4_platform_data *platdata = client->dev.platform_data;
	int ret = 0;
	/*int i = 0;*/

	ts->input_dev->name = "synaptics";
	ts->input_dev->phys = client->name;
	dev_set_drvdata(&(ts->input_dev->dev), ts);

	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_REL, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(ABS_X, ts->input_dev->absbit);
	set_bit(ABS_Y, ts->input_dev->absbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(ABS_PRESSURE, ts->input_dev->absbit);
	set_bit(ABS_TOOL_WIDTH, ts->input_dev->absbit);

	input_set_abs_params(ts->input_dev, ABS_X, 0,
			platdata->x_res, platdata->fuzz_x, 0);
	input_set_abs_params(ts->input_dev, ABS_Y, 0,
			platdata->y_res, platdata->fuzz_y, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE,
			0, 255, platdata->fuzz_p, 0);
	input_set_abs_params(ts->input_dev, ABS_TOOL_WIDTH,
			0, 15, platdata->fuzz_w, 0);

	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0,
			ts->f11.points_supported, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0,
			platdata->x_res, platdata->fuzz_x, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0,
			platdata->y_res, platdata->fuzz_y, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0,
			255, platdata->fuzz_w, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0,
			15, platdata->fuzz_p, 0);

	/*Fix me: The app will not recognize the BTN_DEAD*/
	/*if (ts->hasEgrPalmDetect)
		set_bit(BTN_DEAD, ts->input_dev->keybit);

	if (ts->hasEgrFlick) {
		set_bit(REL_X, ts->input_dev->relbit);
		set_bit(REL_Y, ts->input_dev->relbit);
	}

	if (ts->hasEgrSingleTap)
		set_bit(BTN_TOUCH, ts->input_dev->keybit);

	if (ts->hasEgrDoubleTap)
		set_bit(BTN_TOOL_DOUBLETAP, ts->input_dev->keybit);

	if (ts->hasF19)
		set_bit(BTN_DEAD, ts->input_dev->keybit);

	if (ts->hasF30) {
		for (i = 0; i < ts->f30.points_supported; ++i) {
			set_bit(BTN_F30 + i, ts->input_dev->keybit);
		}
	}*/

	ret = input_register_device(ts->input_dev);
	if (ret) {
		dev_err(&client->dev, "%s: Unable to register %s input device\n", __func__,
				ts->input_dev->name);
		return ret;
	}

	return 0;
}

static int synaptics_rmi4_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct synaptics_rmi4_platform_data *platdata = NULL;
	struct synaptics_rmi4 *ts = NULL;
	int i = 0;
	int ret = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "%s: failed to check i2c function!\n", __func__);
		ret = -EIO;
		goto err_check_i2c_func;
	}

	platdata = client->dev.platform_data;
	if (!platdata) {
		dev_err(&client->dev, "%s: the platform data is NULL!\n", __func__);
		ret = -ENOMEM;
		goto err_get_platform_data;
	}

	ts = kzalloc(sizeof(struct synaptics_rmi4), GFP_KERNEL);
	if (!ts) {
		dev_err(&client->dev, "%s: failed to kzalloc synaptics_rmi4!\n", __func__);
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	ts->synaptics_wq = create_singlethread_workqueue(SYNAPTICS_WQ_NAME);
	if (!ts->synaptics_wq) {
		dev_err(&client->dev, "%s: failed to create workqueue\n", __func__);
		ret = -ENOMEM;
		goto err_creat_workqueue;
	}

	INIT_WORK(&ts->work, synaptics_rmi4_work_func);

	ts->client = client;
	ts->client->dev.init_name = SYNAPTICS_INIT_NAME;
	ts->client->irq = gpio_to_irq(platdata->irq);
	ts->is_support_multi_touch = client->flags;
	i2c_set_clientdata(client, ts);

	/* VDD power on */
	ts->vdd = regulator_get(&client->dev, SYNAPTICS_VDD);
	if (IS_ERR(ts->vdd)) {
		dev_err(&client->dev, "%s: failed to get synaptics vdd\n", __func__);
		ret = -EINVAL;
		goto err_get_vdd;
	}

	ret = regulator_enable(ts->vdd);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to enable synaptics vdd\n", __func__);
		ret = -EINVAL;
		goto err_enable_vdd;
	}

	/* VBUS power on */
	ts->vbus = regulator_get(&client->dev, SYNAPTICS_VBUS);
	if (IS_ERR(ts->vbus)) {
		dev_err(&client->dev, "%s: failed to get synaptics vbus\n", __func__);
		ret = -EINVAL;
		goto err_get_vbus;
	}
	ret = regulator_enable(ts->vbus);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to enable synaptics vbus\n", __func__);
		ret = -EINVAL;
		goto err_enable_vbus;
	}

	/* get gpio block*/
	ts->gpio_block = iomux_get_block(SYNAPTICS_GPIO_BLOCK_NAME);
	if (!ts->gpio_block) {
		dev_err(&client->dev, "%s: failed to get gpio block\n", __func__);
		ret = -EINVAL;
		goto err_config_gpio;
	}

	/* get gpio block config*/
	ts->gpio_block_config = iomux_get_blockconfig(SYNAPTICS_GPIO_BLOCK_NAME);
	if (!ts->gpio_block_config) {
		dev_err(&client->dev, "%s: failed to get gpio block config\n", __func__);
		ret =  -EINVAL;
		goto err_config_gpio;
	}

	/* config gpio work mode*/
	ret = blockmux_set(ts->gpio_block, ts->gpio_block_config, NORMAL);
	if (ret) {
		dev_err(&client->dev, "%s: failed to config gpio\n", __func__);
		ret =  -EINVAL;
		goto err_config_gpio;
	}
	ret = gpio_request(platdata->reset_pin, client->name);
	if (ret) {
		dev_err(&client->dev, "%s: failed to request gpio for reset\n", __func__);
		ret = -EIO;
		goto err_request_reset_pin;
	}

/* Modifed for U9510 T0 board*/
#ifdef BOARD_T0
	#define PMU_LDO13_CTRL           IO_ADDRESS(REG_BASE_PMUSPI + (0x02D<<2))
	#define PMU_POWER_ON              0x16

	writel(PMU_POWER_ON, PMU_LDO13_CTRL);
	msleep(5);

	ret = gpio_request(GPIO_7_5, "GPIO_061");
	if (ret < 0)
		dev_err(&client->dev, "%s: gpio request GPIO_061 error\n", __func__);

	ret = gpio_direction_output(GPIO_7_5, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s: set GPIO_061 output 1 error\n", __func__);

	/*ret = gpio_get_value(GPIO_7_5);*/
	/*dev_err(&client->dev, "%s: config gpio061:  0x%x\n", __func__, ret);*/
#endif

	/* reset device*/
	gpio_direction_output(platdata->reset_pin, 0);
	msleep(20);
	gpio_direction_output(platdata->reset_pin, 1);

	/* read Page Discrption Table */
	msleep(2);

	ret = synaptics_rmi4_read_pdt(ts);
	if (ret <= 0) {
		if (ret == 0) {
			dev_err(&client->dev, "%s: empty PDT\n", __func__);
		} else {
			dev_err(&client->dev, "%s: Error identifying device (%d)\n", __func__, ret);
			fake_synaptics_rmi4_probe();
		}
		ret = -ENODEV;
		goto err_read_pdt;
	}
	/* if the function 11 is not exist, do power on and read pdt again */
	if (!ts->hasF11) {
		ret = synaptics_rmi4_power(client, true);
		if (ret < 0) {
			dev_err(&client->dev, "%s: failed to power on device.\n", __func__);
			goto err_power_failed;
		}
		/*Read pdt again*/
		ret = synaptics_rmi4_read_pdt(ts);
		if (ret <= 0) {
			if (0 == ret) {
				dev_err(&client->dev, "%s: empty PDT\n", __func__);
			} else {
				dev_err(&client->dev, "%s: Error identifying device (%d)\n", __func__, ret);
				fake_synaptics_rmi4_probe();
			}
			ret = -ENODEV;
			goto err_read_pdt;
		}

		if (!ts->hasF11)
			goto err_read_pdt;
	}

#if 0
	/*use control base to set touchscreen wakeup*/
	ret = i2c_smbus_read_byte_data(client, fd_01.ctrl_base_addr);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read fd_01 ctrl register!\n", __func__);
		ret = -EIO;
		goto err_read_device;
	}

	ret &= 0xFC;
	ret = i2c_smbus_write_byte_data(client, fd_01.ctrl_base_addr, ret);
#endif
	ret = i2c_smbus_write_byte_data(client, fd_01.ctrl_base_addr, 0x0);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to write fd_01 ctrl register! \n", __func__);
		ret = -EIO;
		goto err_write_device;
	}
	mdelay(50);

#if 0
	/* set Reporting Mode as continuum */
	ret = i2c_smbus_read_byte_data(client, fd_11.ctrl_base_addr);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read fd_11 ctrl register!\n", __func__);
		ret = -EIO;
		goto err_read_device;
	}

	ret &= 0xF8;
	ret = i2c_smbus_write_byte_data(client, fd_11.ctrl_base_addr, ret);

	ret = i2c_smbus_write_byte_data(client, fd_11.ctrl_base_addr, 0x1);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to write fd_11 ctrl register!\n", __func__);
		ret = -EIO;
		goto err_write_device;
	}
#endif
	/* allocate input device used to report */
	ts->input_dev = input_allocate_device();
	if (!ts->input_dev) {
		dev_err(&client->dev, "%s: failed to allocate input device.\n", __func__);
		ret = -ENOMEM;
		goto err_alloc_input_dev;
	}

	/* config input device parameter */
	ret = synaptics_rmi4_config_input_dev(client);
	if (ret) {
		dev_err(&client->dev, "%s: failed to register input device.\n", __func__);
		ret = -ENODEV;
		goto err_config_input_dev;
	}

	/* request gpio pin for irq */
	ret = gpio_request(platdata->irq, client->name);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to request gpio for irq\n", __func__);
		ret = -EIO;
		goto err_request_irq_pin;
	}

#if 0
	/* Clear touch screen interrupt status register before irq_handler ready
	 * let tp release irq pin
	 */
	if (ts->use_irq) {
		for (i = 5; i > 0 ; i--) {
			/*read the irq Interrupt Status*/
			if (i2c_smbus_read_byte_data(client, fd_01.data_base_addr + 1) >= 0)
				break;
		}
	}
#endif

	/* set irq handler */
	ret = request_irq(ts->client->irq, synaptics_rmi4_irq_handler,
			IRQF_DISABLED | IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND/*platdata->irq_flag*/, client->name, ts);
	if (!ret) {
		dev_info(&client->dev, "%s: Synaptics RMI4 device %s work in interrupt mode\n",
				__func__, client->name);
		ts->use_irq = true;
		/*enable_irq(ts->client->irq);*/
	} else {
		ts->use_irq = false;
	}

	if (!ts->use_irq) {
		dev_info(&client->dev, "%s: Synaptics RMI4 device %s in polling mode\n",
				__func__, client->name);
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = synaptics_rmi4_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
#if 0
	/* Clear touch screen interrupt status register */
	for (i = 5; i > 0 ; i--) {
		/*read the irq Interrupt Status*/
		if (i2c_smbus_read_byte_data(ts->client, fd_01.data_base_addr + 1) >= 0)
			break;
	}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_rmi4_early_suspend;
	ts->early_suspend.resume = synaptics_rmi4_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	dev_info(&client->dev, "%s: probing for Synaptics RMI4 device %s at $%02X...\n",
			__func__, client->name, client->addr);

	ret = i2c_smbus_write_byte_data(client, fd_01.ctrl_base_addr + 1, 0x4);
	if (ret < 0) {
		dev_info(&client->dev, "%s: failed to write fd_01 ctrl register! \n", __func__);
		goto err_write_data;
	}

	return 0;
err_write_data:
	gpio_free(platdata->irq);
err_request_irq_pin:
err_config_input_dev:
	input_free_device(ts->input_dev);
err_alloc_input_dev:
err_write_device:
/*err_read_device:*/
err_power_failed:
err_read_pdt:
	gpio_free(platdata->reset_pin);
err_request_reset_pin:
	ret = blockmux_set(ts->gpio_block, ts->gpio_block_config, LOWPOWER);
	if (ret) {
		pr_err("%s: failed to set gpio_block to LOWPOWER\n", __func__);
	}
err_config_gpio:
	regulator_disable(ts->vbus);
err_enable_vbus:
	regulator_put(ts->vbus);
err_get_vbus:
	regulator_disable(ts->vdd);
err_enable_vdd:
	regulator_put(ts->vdd);
err_get_vdd:
	destroy_workqueue(ts->synaptics_wq);
err_creat_workqueue:
	kfree(ts);
err_alloc_data_failed:
err_get_platform_data:
err_check_i2c_func:

	return ret;
}


static int synaptics_rmi4_remove(struct i2c_client *client)
{
	struct synaptics_rmi4 *ts = i2c_get_clientdata(client);
	struct synaptics_rmi4_platform_data *platdata = client->dev.platform_data;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif

	if (ts->use_irq)
		free_irq(platdata->irq, ts);
	else
		hrtimer_cancel(&ts->timer);

	input_unregister_device(ts->input_dev);

	regulator_disable(ts->vdd);
	regulator_put(ts->vdd);
	regulator_disable(ts->vbus);
	regulator_put(ts->vbus);

	if (ts->synaptics_wq)
		destroy_workqueue(ts->synaptics_wq);

	kfree(ts);
	return 0;
}

static int synaptics_rmi4_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct synaptics_rmi4 *ts = i2c_get_clientdata(client);
	int ret = 0;
	pr_info("[%s]: +\n", __func__);

	/* if use interrupt disable the irq ,else disable timer */
	if (ts->use_irq)
		disable_irq_nosync(client->irq);
	else
		hrtimer_cancel(&ts->timer);
#if 0
	/* disable interrupt*/
	ret = i2c_smbus_write_byte_data(client, fd_01.ctrl_base_addr + 1, 0x0);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to disable interrrupt\n", __func__);
		return -EIO;
	}
#endif
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq) {
		/* if work was pending disable-count is now 2 */
		dev_err(&client->dev, "%s: can't cancel the work, so enable the irq\n", __func__);
	}
	flush_workqueue(ts->synaptics_wq);

	/* Read fd01 ctrl register */
#if 0
	ret = i2c_smbus_read_byte_data(client, fd_01.ctrl_base_addr);
	ret = (ret & 0xFC) | 0x1;
#endif
	/* Write back to set ts into sleep mode*/
	ret = i2c_smbus_write_byte_data(client, fd_01.ctrl_base_addr, 0x01);
	if (ret < 0) {
		dev_err(&client->dev, "%s: the touch can't get into deep sleep\n", __func__);
		return -EIO;
	}

	/* config gpio lowpower work mode*/
	ret = blockmux_set(ts->gpio_block, ts->gpio_block_config, LOWPOWER);
	if (ret != 0) {
		dev_err(&client->dev, "%s: failed to config gpio lowpower mode\n", __func__);
		return -EINVAL;
	}

	/* disable regulator*/
	/*regulator_disable(ts->vbus);*/
	/*regulator_disable(ts->vdd);*/

	/*dev_info(&client->dev, "[TouchScreen] synaptics_rmi4_touch suspend successfully!\n");*/
	pr_info("[%s]: -\n", __func__);
	return 0;
}

static int synaptics_rmi4_resume(struct i2c_client *client)
{
	/* open the resume function for the touchscreen of synaptics */
	struct synaptics_rmi4 *ts = i2c_get_clientdata(client);
	struct synaptics_rmi4_platform_data *platdata = client->dev.platform_data;
	int ret = 0;
	pr_info("[%s]: +\n", __func__);
#if 0
	/* VDD power on */
	ret = regulator_enable(ts->vdd);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to enable synaptics vdd!\n", __func__);
		return -EIO;
	}

	/* VBUS power on */
	ret = regulator_enable(ts->vbus);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to enable synaptics vbus!\n", __func__);
		return -EIO;
	}
#endif
	/* config gpio normal work mode*/
	ret = blockmux_set(ts->gpio_block, ts->gpio_block_config, NORMAL);
	if (ret != 0) {
		dev_err(&client->dev, "%s: failed to config gpio lowpower mode\n", __func__);
		return -EINVAL;
	}
#if 0
	/* set Reporting Mode as continuum */
	ret = i2c_smbus_read_byte_data(client, fd_11.ctrl_base_addr);
	dev_dbg(&client->dev,  "%s: fd_11.ctrl_base_addr = %d\n", __func__, fd_11.ctrl_base_addr);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read fd_11 ctrl register!\n", __func__);
		ret = -EIO;
	}

	ret &= 0xF8;
	ret = i2c_smbus_write_byte_data(client, fd_11.ctrl_base_addr, ret);
	dev_dbg(&client->dev,  "%s: fd_11.ctrl_base_addr = %d\n", __func__, fd_11.ctrl_base_addr);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to write fd_11 ctrl register!\n", __func__);
		ret = -EIO;
	}
#endif

#if 0
	/*use control base to set touchscreen wakeup*/
	ret = i2c_smbus_read_byte_data(client, fd_01.ctrl_base_addr);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read fd_01 ctrl register!\n", __func__);
		return -EIO;
	}
	ret &= 0xFC;
#endif
	ret = i2c_smbus_write_byte_data(client, fd_01.ctrl_base_addr, 0x00);
	if (ret < 0) {
		dev_err(&client->dev, "%s: the touch can't resume!\n", __func__);
		return -EIO;
	}

	mdelay(50);

	/* reset gpio is not used, set output low*/
	/*gpio_direction_output(platdata->reset_pin, 0);*/
#if 0
	/* enable interrupt*/
	ret = i2c_smbus_write_byte_data(client, fd_01.ctrl_base_addr + 1, 0xFF);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to enable interrrupt\n", __func__);
		return -EIO;
	}
#endif

	if (ts->use_irq)
		enable_irq(client->irq);
	else
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

	/*dev_info(&client->dev, "[TouchScreen]synaptics_rmi4_touch resume successfully!\n");*/
	pr_info("[%s]: -\n", __func__);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_rmi4_early_suspend(struct early_suspend *h)
{
	struct synaptics_rmi4 *ts = container_of(h, struct synaptics_rmi4, early_suspend);
	synaptics_rmi4_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_rmi4_late_resume(struct early_suspend *h)
{
	struct synaptics_rmi4 *ts = container_of(h, struct synaptics_rmi4, early_suspend);
	synaptics_rmi4_resume(ts->client);
}
#endif

static const struct i2c_device_id synaptics_ts_id[] = {
	{ SYNAPTICS_RMI4_NAME, 0 },
	{ }
};

static struct i2c_driver synaptics_rmi4_driver = {
	.probe		= synaptics_rmi4_probe,
	.remove		= synaptics_rmi4_remove,
#if 1
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= synaptics_rmi4_suspend,
	.resume		= synaptics_rmi4_resume,
#endif
#endif

	.id_table	= synaptics_ts_id,
	.driver = {
		.name	= SYNAPTICS_RMI4_NAME,
	},
};

static unsigned char fake_input_name[] = "fake_synaptics";
static unsigned char fake_phy_name[] = "fake_ts";
static int fake_synaptics_rmi4_probe(void)
{
	struct synaptics_rmi4 *ts = NULL;
	int ret = 0;

	printk("%s: kzalloc synaptics_rmi4!\n", __func__);
	ts = kzalloc(sizeof(struct synaptics_rmi4), GFP_KERNEL);
	if (!ts) {
		printk("%s: failed to kzalloc synaptics_rmi4!\n", __func__);
		ret = -ENOMEM;
		goto alloc_data_failed;
	}

	/* allocate input device used to report */
	ts->input_dev = input_allocate_device();
	if (!ts->input_dev) {
		printk("%s: failed to allocate input device.\n", __func__);
		ret = -ENOMEM;
		goto alloc_input_dev_failed;
	}

	ts->input_dev->name = fake_input_name;
	ts->input_dev->phys = fake_phy_name;
	dev_set_drvdata(&(ts->input_dev->dev), ts);

	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_REL, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(ABS_X, ts->input_dev->absbit);
	set_bit(ABS_Y, ts->input_dev->absbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(ABS_PRESSURE, ts->input_dev->absbit);
	set_bit(ABS_TOOL_WIDTH, ts->input_dev->absbit);

	input_set_abs_params(ts->input_dev, ABS_X, 0, 1080, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_Y, 0, 1920, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 1, 0, 0);

	ret = input_register_device(ts->input_dev);
	if (ret) {
		printk("%s: Unable to register %s input device\n", __func__,
				ts->input_dev->name);
		ret = -1;
	}


alloc_input_dev_failed:
	kfree(ts);
alloc_data_failed:
	return ret;
}

static int __devinit synaptics_rmi4_init(void)
{
	return i2c_add_driver(&synaptics_rmi4_driver);
}

static void __exit synaptics_rmi4_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_driver);
}

module_init(synaptics_rmi4_init);
module_exit(synaptics_rmi4_exit);

MODULE_AUTHOR("Hisilicon K3");
MODULE_DESCRIPTION("Synaptics RMI4 Driver");
MODULE_LICENSE("GPL");
