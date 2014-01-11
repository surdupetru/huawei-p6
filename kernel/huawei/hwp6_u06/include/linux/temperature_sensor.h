/*
 *
 * arch/arm/mach-k3v2/temperature_sensor.h
 *
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef __ARCH_ARM_PLAT_K3V2_INCLUDE_PLAT_TEMPERATURE_SENSOR_H
#define __ARCH_ARM_PLAT_K3V2_INCLUDE_PLAT_TEMPERATURE_SENSOR_H

struct k3v2_temp_sensor_pdata {
	char *name;
};

struct sim_temp_sensor_pdata{
	char *name;
};

struct ap_temp_sensor_pdata{
	char *name;
};

struct nct203_temp_sensor_pdata{
	char *name;
};
struct thermal_config_name_pdata{
	char *name;
};
#endif
