/*
 *
 * arch/arm/mach-k3v2/thermal_framework.h
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

#ifndef __LINUX_THERMAL_FRAMEWORK_H__
#define __LINUX_THERMAL_FRAMEWORK_H__

struct thermal_dev;

/**
 * struct thermal_dev_ops  - Structure for device operation call backs
 * @get_temp: A temp sensor call back to get the current temperature.
 *		temp is reported in milli degrees.
 * @process_temp: The governors call back for processing a domain temperature
 *
 */
struct thermal_dev_ops {
	/* Sensor call backs */
	int (*report_temp) (struct thermal_dev *);
	/* Governor call backs */
	int (*process_temp) (struct thermal_dev *temp_sensor, int temp);
};

/**
 * struct thermal_dev  - Structure for each thermal device.
 * @name: The name of the device that is registering to the framework
 * @domain_name: The temperature domain that the thermal device represents
 * @dev: Device node
 * @dev_ops: The device specific operations for the sensor, governor and cooling
 *           agents.
 * @node: The list node of the
 * @index: The index of the device created.
 * @current_temp: The current temperature reported for the specific domain
 *
 */
struct thermal_dev {
	const char	*name;
	const char	*domain_name;
	struct device	*dev;
	struct thermal_dev_ops *dev_ops;
	struct list_head node;
	int		index;
	int 		current_temp;

};

extern int thermal_request_temp(struct thermal_dev *tdev);
extern int thermal_sensor_set_temp(struct thermal_dev *tdev);

extern int thermal_sensor_dev_register(struct thermal_dev *tdev);
extern void thermal_sensor_dev_unregister(struct thermal_dev *tdev);
extern int thermal_governor_dev_register(struct thermal_dev *tdev);
extern void thermal_governor_dev_unregister(struct thermal_dev *tdev);
extern int getcalctemperature(int channel);

#endif /* __LINUX_THERMAL_FRAMEWORK_H__ */
