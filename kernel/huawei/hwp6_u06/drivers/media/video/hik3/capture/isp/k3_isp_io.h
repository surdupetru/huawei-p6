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

#ifndef __K3_ISP_IO_H__
#define __K3_ISP_IO_H__

#include "../camera.h"

typedef struct {
	char name[50];
	struct regulator *regulator;
	u32 vol_min;
	u32 vol_max;
} sensor_ldo_t;

/*
 * isp register struct
 * used for initialize isp registers
 */
struct isp_reg_t {
	u32 subaddr;
	u8 value;
	u8 mask;
};

typedef struct _isp_io_data {
	bool inited;
	bool csi_inited;
} isp_io_data_t;

struct _sensor_reg_t;
struct _camera_sensor;
struct _sensor_reg_t;
/* Sensor driver read/write I2C through ISP */
void k3_ispio_reset_i2c(struct i2c_t *i2c);
int k3_ispio_read_reg(i2c_index_t index, u8 i2c_addr, u16 reg, u16 *val, i2c_length length);
int k3_ispio_read_seq(i2c_index_t index, u8 i2c_addr, struct _sensor_reg_t *seq, u32 size, i2c_length length);
int k3_ispio_write_reg(i2c_index_t index, u8 i2c_addr, u16 reg, u16 val, i2c_length length, u8 mask);
int k3_ispio_write_seq(i2c_index_t index, u8 i2c_addr,
		       const struct _sensor_reg_t *buf, u32 size, i2c_length length, u8 mask);
int k3_ispio_write_group_seq(i2c_index_t index, u8 i2c_addr,
			     const struct _sensor_reg_t *buf, u32 size);
void k3_ispio_write_isp_seq(const struct isp_reg_t *seq, u32 size);
void k3_ispio_read_isp_seq(struct isp_reg_t *seq, u32 size);
void k3_ispio_update_flip(u8 flip_changed, u16 width, u16 height, pixel_order changed);
int k3_ispldo_power_sensor(camera_power_state power_state,  char *ldo_name);
int k3_ispgpio_power_sensor(struct _camera_sensor *sensor,
			    camera_power_state power_state);
int k3_ispio_i2c_ioconfig(struct _camera_sensor *sensor,
			  camera_power_state power_state);
int k3_ispio_ioconfig(struct _camera_sensor *sensor,
			  camera_power_state power_state);
int k3_ispgpio_reset_sensor(sensor_index_t sensor_index, camera_power_state power_state, electrical_valid_t reset_valid);
void k3_isp_io_enable_mclk(mclk_state state, sensor_index_t sensor_index);
int k3_ispio_power_init(char *ldo_name, u32 vol_min, u32 vol_max);
void k3_ispio_power_deinit(void);
int k3_ispio_init_csi(csi_index_t mipi_index, csi_lane_t mipi_lane_count, u8 lane_clk);
void k3_ispio_deinit_csi(csi_index_t mipi_index);

struct platform_device;
typedef struct _ispio_controller {
	int (*power_init) (char *ldo_name, u32 vol_min, u32 vol_max);
	void (*power_deinit) (void);
	int (*power_sensor) (camera_power_state power_state, char *ldo_name);
	int (*ispio_hw_init) (struct platform_device *pdev);
	int (*ispio_hw_deinit) (void);
	int (*init_csi) (csi_index_t csi_index, csi_lane_t mipi_lane_count, u8 lane_clk);
	void (*deinit_csi) (csi_index_t index);
	int (*ioconfig) (camera_power_state power_state, data_interface_t);
	int (*i2c_ioconfig) (camera_power_state power_state, data_interface_t);
} ispio_controller;

typedef struct _isp_sensor_reg_controller {
	/* Sensor driver read/write I2C through ISP */
	int (*isp_read_sensor_seq)(i2c_index_t index, u8 i2c_addr,struct _sensor_reg_t *buf, u32 size, i2c_length length);
	int (*isp_read_sensor_reg)(i2c_index_t index, u8 i2c_addr, u16 reg, u16 *val, i2c_length length);
	int (*isp_write_sensor_reg)(i2c_index_t index, u8 i2c_addr, u16 reg, u16 val, i2c_length length, u8 mask);
	int (*isp_write_sensor_seq)(i2c_index_t index, u8 i2c_addr,
					const struct _sensor_reg_t *reg, u32 size, i2c_length length, u8 mask);
	int (*isp_write_sensor_group_seq)(i2c_index_t index, u8 i2c_addr,
					const struct _sensor_reg_t *reg, u32 size);
	void (*isp_write_isp_seq) (const struct isp_reg_t *seq, u32 size);
	void (*isp_read_isp_seq) (struct isp_reg_t *seq, u32 size);

	void (*isp_update_flip)(u8 flip_change, u16 width, u16 height, pixel_order changed);

	void (*isp_power_sensor) (sensor_index_t sensor_index,
				  camera_power_state power_state,
				  electrical_valid_t pd_valid,
				  electrical_valid_t vcmpd_valid);
	void (*isp_reset_sensor) (sensor_index_t sensor_index, camera_power_state power_state,
				  electrical_valid_t reset_valid);
	void (*isp_enable_mclk) (mclk_state state, sensor_index_t sensor_index);
    void (*isp_sensor_reset_i2c)(struct i2c_t *i2c);
} isp_sensor_reg_controller;

ispio_controller *get_ispio_controller(void);
isp_sensor_reg_controller *get_isp_sensor_reg_controller(void);

#endif /*__K3_ISP_IO_H__ */

/********************** END **********************/
