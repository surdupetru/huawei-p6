/*
 *  Hisilicon K3 soc camera ISP driver source file
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
#include <mach/gpio.h>
#include "k3_isp.h"
#include "k3_isp_io.h"
#include "sensor_common.h"

#define DEBUG_DEBUG 0
#define LOG_TAG "K3_ISPIO"
#include "cam_log.h"

static ispio_controller *ispio_hw_ctl;
static isp_sensor_reg_controller *isp_sensor_reg_ctl;
static isp_io_data_t isp_io_data = { false, false };

/*
 **************************************************************************
 * FunctionName: k3_ispio_reset_i2c;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void k3_ispio_reset_i2c(struct i2c_t *i2c)
{
    isp_sensor_reg_ctl->isp_sensor_reset_i2c(i2c);
}
EXPORT_SYMBOL(k3_ispio_reset_i2c);

/*
 **************************************************************************
 * FunctionName: k3_ispio_read_reg;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispio_read_reg(i2c_index_t index, u8 i2c_addr, u16 reg, u16 *val, i2c_length length)
{
    return isp_sensor_reg_ctl->isp_read_sensor_reg(index, i2c_addr, reg, val, length);
}
EXPORT_SYMBOL(k3_ispio_read_reg);
/*
 **************************************************************************
 * FunctionName: k3_ispio_read_seq;
 * Description : The interface that call isp_read_sensor_seq function of ispv1 file;
 * Input       : the parameter is the same as the parameter of isp_read_sensor_seq;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispio_read_seq(i2c_index_t index, u8 i2c_addr,
		        struct _sensor_reg_t *seq, u32 size, i2c_length length)
{
	return isp_sensor_reg_ctl->isp_read_sensor_seq(index, i2c_addr, seq, size, length);
}

EXPORT_SYMBOL(k3_ispio_read_seq);

/*
 **************************************************************************
 * FunctionName: k3_ispio_write_reg;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispio_write_reg(i2c_index_t index, u8 i2c_addr, u16 reg, u16 val, i2c_length length, u8 mask)
{

    return isp_sensor_reg_ctl->isp_write_sensor_reg(index, i2c_addr, reg, val, length, mask);
}
EXPORT_SYMBOL(k3_ispio_write_reg);

/*
 **************************************************************************
 * FunctionName: k3_ispio_write_seq;
 * Description : write sequeuence to select I2C though isp command set;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispio_write_seq(i2c_index_t index, u8 i2c_addr,
		       const struct _sensor_reg_t *seq, u32 size, i2c_length length, u8 mask)
{
	return isp_sensor_reg_ctl->isp_write_sensor_seq(index, i2c_addr, seq, size, length, mask);
}
EXPORT_SYMBOL(k3_ispio_write_seq);

/*
 **************************************************************************
 * FunctionName: k3_ispio_write_group_seq;
 * Description : write sequeuence to select I2C though isp command set;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispio_write_group_seq(i2c_index_t index, u8 i2c_addr,
			     const struct _sensor_reg_t *reg, u32 size)
{
	return isp_sensor_reg_ctl->isp_write_sensor_group_seq(index, i2c_addr, reg, size);
}
EXPORT_SYMBOL(k3_ispio_write_group_seq);

/*
 **************************************************************************
 * FunctionName: k3_ispio_write_isp_seq;
 * Description : write sensor related sequeuence to isp;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void k3_ispio_write_isp_seq(const struct isp_reg_t *seq, u32 size)
{
	isp_sensor_reg_ctl->isp_write_isp_seq(seq, size);
}
EXPORT_SYMBOL(k3_ispio_write_isp_seq);

/*
 **************************************************************************
 * FunctionName: k3_ispio_read_isp_seq;
 * Description : read sensor related sequeuence to isp;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void k3_ispio_read_isp_seq(struct isp_reg_t *seq, u32 size)
{
	isp_sensor_reg_ctl->isp_read_isp_seq(seq, size);
}
EXPORT_SYMBOL(k3_ispio_read_isp_seq);
/*
 **************************************************************************
 * FunctionName: k3_ispio_update_flip;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void k3_ispio_update_flip(u8 flip_changed, u16 width, u16 height, pixel_order changed)
{
	isp_sensor_reg_ctl->isp_update_flip(flip_changed, width, height, changed);
}
EXPORT_SYMBOL(k3_ispio_update_flip);

/*
 **************************************************************************
 * FunctionName: k3_ispio_power_init;
 * Description : register ldo;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispio_power_init(char *ldo_name, u32 vol_min, u32 vol_max)
{
	return ispio_hw_ctl->power_init(ldo_name, vol_min, vol_max);
}
EXPORT_SYMBOL(k3_ispio_power_init);

/*
 **************************************************************************
 * FunctionName: k3_ispio_power_deinit;
 * Description : deregister ldo;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void k3_ispio_power_deinit(void)
{
	ispio_hw_ctl->power_deinit();
}
EXPORT_SYMBOL(k3_ispio_power_deinit);

/*
 **************************************************************************
 * FunctionName: k3_ispldo_power_sensor;
 * Description : Power on/off ldo
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispldo_power_sensor(camera_power_state power_state, char *ldo_name)
{
	int ret = 0;

	print_debug("enter %s", __func__);

	/*ldo */
	ret = ispio_hw_ctl->power_sensor(power_state, ldo_name);
	if (ret != 0) {
		print_error("%s: power sensor fail", __func__);
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL(k3_ispldo_power_sensor);

/*
 **************************************************************************
 * FunctionName: k3_ispio_power_sensor;
 * Description : Power on/off gpio;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispgpio_power_sensor(camera_sensor *sensor,
			    camera_power_state power_state)
{
	int ret = 0;

	print_debug("enter %s", __func__);

	/*gpio powerdown */
	isp_sensor_reg_ctl->isp_power_sensor(sensor->sensor_index, power_state,
					     sensor->power_conf.pd_valid, sensor->power_conf.vcmpd_valid);

	return ret;
}
EXPORT_SYMBOL(k3_ispgpio_power_sensor);

/*
 **************************************************************************
 * FunctionName: k3_ispio_i2c_ioconfig;
 * Description : i2c io config;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispio_i2c_ioconfig(camera_sensor *sensor, camera_power_state power_state)
{

	print_debug("enter %s", __func__);

	return ispio_hw_ctl->i2c_ioconfig(power_state, sensor->interface_type);
}
EXPORT_SYMBOL(k3_ispio_i2c_ioconfig);

/*
 **************************************************************************
 * FunctionName: k3_ispio_ioconfig;
 * Description : io config;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispio_ioconfig(camera_sensor *sensor, camera_power_state power_state)
{

	print_debug("enter %s", __func__);

	return ispio_hw_ctl->ioconfig(power_state, sensor->interface_type);
}
EXPORT_SYMBOL(k3_ispio_ioconfig);

/*
 **************************************************************************
 * FunctionName: k3_ispgpio_reset_sensor;
 * Description : reset sensor through gpio;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispgpio_reset_sensor(sensor_index_t sensor_index, camera_power_state power_state, electrical_valid_t reset_valid)
{
	if (sensor_index >= CAMERA_SENSOR_MAX || reset_valid > HIGH_VALID) {
		print_error("cur_type %d or reset_valid %d error", sensor_index, reset_valid);
		return -EINVAL;
	}

	/*gpio reset */
	isp_sensor_reg_ctl->isp_reset_sensor(sensor_index, power_state, reset_valid);
	return 0;
}
EXPORT_SYMBOL(k3_ispgpio_reset_sensor);

/*
 **************************************************************************
 * FunctionName: k3_isp_io_enable_mclk;
 * Description : enable or disable mclk;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void k3_isp_io_enable_mclk(mclk_state state, sensor_index_t sensor_index)
{
	isp_sensor_reg_ctl->isp_enable_mclk(state, sensor_index);
	return ;
}
EXPORT_SYMBOL(k3_isp_io_enable_mclk);

/*
 **************************************************************************
 * FunctionName: k3_ispio_init_csi;
 * Description :
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispio_init_csi(csi_index_t mipi_index, csi_lane_t mipi_lane_count, u8 lane_clk)
{
	int ret = 0;

	if (isp_io_data.csi_inited)
		return ret;

	/* enter LP11 state
	   {    SETREG8(ISP_REG_BASE+0x63602, 0x30);
	   SETREG8(ISP_REG_BASE+0x63603, 0x00);
	   SETREG8(ISP_REG_BASE+0x63605, 0x03);
	   SETREG8(ISP_REG_BASE+0x63609, 0x37);
	   } */

	ret = ispio_hw_ctl->init_csi(mipi_index, mipi_lane_count, lane_clk);

	/*exit LP11 state
	 *SETREG8(ISP_REG_BASE+0x63602, 0x30);
	 *SETREG8(ISP_REG_BASE+0x63603, 0x00);
	 *SETREG8(ISP_REG_BASE+0x63605, 0x00);
	 *SETREG8(ISP_REG_BASE+0x63609, 0x37);
	 */
	if (0 == ret)
		isp_io_data.csi_inited = true;

	return ret;
}
EXPORT_SYMBOL(k3_ispio_init_csi);

/*
 **************************************************************************
 * FunctionName: k3_ispio_deinit_csi;
 * Description :
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void k3_ispio_deinit_csi(csi_index_t mipi_index)
{
	if (isp_io_data.csi_inited == false)
		return;

	ispio_hw_ctl->deinit_csi(mipi_index);
	isp_io_data.csi_inited = false;
}
EXPORT_SYMBOL(k3_ispio_deinit_csi);

/*
 **************************************************************************
 * FunctionName: k3_ispio_init;
 * Description : init ispio;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
int k3_ispio_init(struct platform_device *pdev)
{
	int ret = 0;
	print_debug("enter %s", __func__);
	if (isp_io_data.inited) {
		print_error("%s ispio not already inited", __func__);
		return -EINVAL;
	}

	isp_sensor_reg_ctl = get_isp_sensor_reg_controller();
	ispio_hw_ctl = get_ispio_controller();
	ret = ispio_hw_ctl->ispio_hw_init(pdev);
	if (ret == 0)
		isp_io_data.inited = true;

	return ret;
}

/*
 **************************************************************************
 * FunctionName: k3_ispio_deinit;
 * Description : deinit ispio;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
void k3_ispio_deinit()
{
	if (isp_io_data.inited) {
		ispio_hw_ctl->ispio_hw_deinit();
		isp_io_data.inited = false;
	}
}

/*********************** END *************************/
