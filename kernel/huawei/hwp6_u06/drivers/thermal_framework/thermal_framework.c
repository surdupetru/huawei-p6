/*
 * drivers/thermal/thermal_framework.c
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>

#include <linux/thermal_framework.h>

static LIST_HEAD(thermal_domain_list);
static DEFINE_MUTEX(thermal_domain_list_lock);

static atomic_t device_count;

/**
 * DOC: Introduction
 * =================
 * The Thermal Framework is designed to be a central location to link
 * temperature sensor drivers, governors  together.
 * The principle is to have one temperature sensor to one governor to many
 * .  This model allows the governors to impart  policies
 * based on the available  for a specific domain.
 *
 * The temperature sensor device should register to the framework and report
 * the temperature of the current domain for which it reports a
 * temperature measurement.
 *
 * The governor is responsible for imparting the  for the specific
 * domain.  The governor will be given a list of  agents that it can call
 * to  domain.
 *
 * The  agent's primary responsibility is to perform an operation on the
 * device to  domain it is responsible for.
 *
 * The sensor, governor and the ing agents are linked in the framework
 * via the domain_name in the thermal_dev structure.
*/
/**
 * struct thermal_domain  - Structure that contains the pointers to the
 *		sensor, governor and a list of ing agents.
 * @domain_name: The domain that the thermal structure represents
 * @node: The list node of the thermal domain
 * @temp_sensor: The domains temperature sensor thermal device pointer.
 * @governor: The domain governor thermal device pointer.
 *
 */
struct thermal_domain {
	const char *domain_name;
	struct list_head node;
	struct thermal_dev *temp_sensor;
	struct thermal_dev *governor;
};

/**
 * thermal_sensor_set_temp() - External API to allow a sensor driver to set
 *				the current temperature for a domain
 * @tdev: The thermal device setting the temperature
 *
 * Returns 0 for a successfull call to a governor. ENODEV if a governor does not
 * exist.
 */
int thermal_sensor_set_temp(struct thermal_dev *tdev)
{
	struct thermal_domain *thermal_domain;
	int ret = -ENODEV;
	if (list_empty(&thermal_domain_list)) {
		pr_debug("%s: No governors registered\n", __func__);
		goto out;
	}
	list_for_each_entry(thermal_domain, &thermal_domain_list, node) {
		if (!strcmp(thermal_domain->domain_name, tdev->domain_name)) {
			if (thermal_domain->governor &&
			    thermal_domain->governor->dev_ops &&
			    thermal_domain->governor->dev_ops->process_temp) {
				thermal_domain->governor->dev_ops->process_temp(tdev, tdev->current_temp);
				ret = 0;
				goto out;
			} else {
				pr_debug("%s:Gov did not have right function\n",
					__func__);
				goto out;
			}

		}
		}
out:
	return ret;
}
EXPORT_SYMBOL_GPL(thermal_sensor_set_temp);

/**
 * thermal_request_temp() - Requests the thermal sensor to report it's current
 *			    temperature to the governor.
 *
 * @tdev: The agent requesting the updated temperature.
 *
 * Returns the current temperature of the current domain.
 * ENODEV if the temperature sensor does not exist.
 * EOPNOTSUPP if the temperature sensor does not support this API.
 */
int thermal_request_temp(struct thermal_dev *tdev)
{
	struct thermal_domain *thermal_domain;
	int ret = -ENODEV;

	if (list_empty(&thermal_domain_list)) {
		pr_debug("%s: No thermal sensors registered\n", __func__);
		return ret;
	}

	mutex_lock(&thermal_domain_list_lock);
	list_for_each_entry(thermal_domain, &thermal_domain_list, node) {
		if (!strcmp(thermal_domain->domain_name, tdev->domain_name))
			goto report;
		else {
			pr_err("%s:Cannot find domain_name for %s\n",
				__func__, thermal_domain->domain_name);
			mutex_unlock(&thermal_domain_list_lock);
			return ret;
		}
	}
report:
	mutex_unlock(&thermal_domain_list_lock);
	if (tdev->dev_ops &&
	    tdev->dev_ops->report_temp) {
			ret = tdev->dev_ops->report_temp(tdev);
	} else {
		pr_err("%s:Getting temp is not supported for domain %s\n",
			__func__, thermal_domain->domain_name);
		ret = -EOPNOTSUPP;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(thermal_request_temp);


/**
 * thermal_init_thermal_state() - Initialize the domain thermal state machine.
 *		Once all the thermal devices are available in the domain.
 *		Force the thermal sensor for the domain to report it's current
 *		temperature.
 *
 * @tdev: The thermal device that just registered
 *
 * Returns -ENODEV for empty lists and 0 for success
 */
static int thermal_init_thermal_state(struct thermal_dev *tdev)
{
	struct thermal_domain *domain;

	if (list_empty(&thermal_domain_list)) {
		pr_err("%s: No domain devices registered\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&thermal_domain_list_lock);
	list_for_each_entry(domain, &thermal_domain_list, node) {
		if (!strcmp(domain->domain_name, tdev->domain_name))
			goto check_domain;
	}

check_domain:
	mutex_unlock(&thermal_domain_list_lock);
	if (domain->temp_sensor && domain->governor)
		thermal_request_temp(tdev);
	else
		pr_debug("%s:Not all components registered for %s domain\n",
			__func__, domain->domain_name);
	return 0;
}

/**
 * thermal_governor_dev_register() - Registration call for thermal domain governors
 *
 * @tdev: The thermal governor device structure.
 *
 * Returns 0 for a successful registration of the governor to a domain
 * ENOMEM if the domain could not be created.
 *
 */
int thermal_governor_dev_register(struct thermal_dev *tdev)
{
	struct thermal_domain *therm_dom;
	struct thermal_domain *domain;
	tdev->index = atomic_inc_return(&device_count);
	if (list_empty(&thermal_domain_list)) {
		goto init_governor;
	} else {
		mutex_lock(&thermal_domain_list_lock);
		list_for_each_entry(domain, &thermal_domain_list, node) {
			pr_debug("%s:Found %s %s\n", __func__,
				domain->domain_name, tdev->domain_name);
			if (!strcmp(domain->domain_name, tdev->domain_name)) {
				pr_info("%s:Found %s\n", __func__,
					domain->domain_name);
				domain->governor = tdev;
				goto out;
			}
		}
		mutex_unlock(&thermal_domain_list_lock);
	}

init_governor:
	therm_dom = kzalloc(sizeof(struct thermal_domain), GFP_KERNEL);
	if (!therm_dom) {
		pr_err("%s:Cannot allocate domain memory\n", __func__);
		return -ENOMEM;
	}

	therm_dom->governor = tdev;
	therm_dom->domain_name = tdev->domain_name;
	pr_info("%s:Adding %s governor\n", __func__, tdev->name);
	mutex_lock(&thermal_domain_list_lock);
	list_add(&therm_dom->node, &thermal_domain_list);
out:
	mutex_unlock(&thermal_domain_list_lock);
	thermal_init_thermal_state(tdev);

	return 0;
}
EXPORT_SYMBOL_GPL(thermal_governor_dev_register);

/**
 * thermal_governor_dev_unregister() - Unregistration call for thermal domain
 *	governors.
 *
 * @tdev: The thermal governor device structure.
 *
 */
void thermal_governor_dev_unregister(struct thermal_dev *tdev)
{
	struct thermal_domain *domain;

	mutex_lock(&thermal_domain_list_lock);

	list_for_each_entry(domain, &thermal_domain_list, node) {
		if (!strcmp(domain->domain_name, tdev->domain_name)) {
			kfree(domain->governor);
			domain->governor = NULL;
		}
	}

	mutex_unlock(&thermal_domain_list_lock);
	return;
}
EXPORT_SYMBOL_GPL(thermal_governor_dev_unregister);

/**
 * thermal_sensor_dev_register() - Registration call for temperature sensors
 *
 * @tdev: The temperature device structure.
 *
 * Returns 0 for a successful registration of the temp sensor to a domain
 * ENOMEM if the domain could not be created.
 */
int thermal_sensor_dev_register(struct thermal_dev *tdev)
{
	struct thermal_domain *therm_dom;
	struct thermal_domain *domain;
	tdev->index = atomic_inc_return(&device_count);
	if (list_empty(&thermal_domain_list)) {
		pr_debug("%s:Need to init the %s domain\n",
			__func__, tdev->domain_name);
		goto init_sensor;
	} else {
		mutex_lock(&thermal_domain_list_lock);
		list_for_each_entry(domain, &thermal_domain_list, node) {
			pr_info("%s:Found %s %s\n", __func__,
				domain->domain_name, tdev->domain_name);
			if (!strcmp(domain->domain_name, tdev->domain_name)) {
				pr_info("%s:Adding %s sensor\n",
					__func__, tdev->name);
				domain->temp_sensor = tdev;
				goto out;
			}
		}
		mutex_unlock(&thermal_domain_list_lock);
	}

init_sensor:
	therm_dom = kzalloc(sizeof(struct thermal_domain),
		GFP_KERNEL);
	if (!therm_dom) {
		pr_err("%s:Cannot allocate domain memory\n",
			__func__);
		return -ENOMEM;
	}
	therm_dom->temp_sensor = tdev;
	therm_dom->domain_name = tdev->name;
	pr_debug("%s:Adding %s sensor\n", __func__, tdev->name);
	mutex_lock(&thermal_domain_list_lock);
	list_add(&therm_dom->node, &thermal_domain_list);
out:
	mutex_unlock(&thermal_domain_list_lock);
	thermal_init_thermal_state(tdev);
	return 0;
}
EXPORT_SYMBOL_GPL(thermal_sensor_dev_register);

/**
 * thermal_sensor_dev_unregister() - Unregistration call for  temperature sensors
 *
 * @tdev: The temperature device structure.
 *
 */
void thermal_sensor_dev_unregister(struct thermal_dev *tdev)
{
	struct thermal_domain *domain;

	mutex_lock(&thermal_domain_list_lock);

	list_for_each_entry(domain, &thermal_domain_list, node) {
		if (!strcmp(domain->domain_name, tdev->domain_name)) {
			kfree(domain->temp_sensor);
			domain->temp_sensor = NULL;
		}
	}
	mutex_unlock(&thermal_domain_list_lock);
	return;
}
EXPORT_SYMBOL_GPL(thermal_sensor_dev_unregister);

static int __init thermal_framework_init(void)
{
	return 0;
}

static void __exit thermal_framework_exit(void)
{
	return;
}

module_init(thermal_framework_init);
module_exit(thermal_framework_exit);

MODULE_AUTHOR("Brand  <xuezhiliang@huawei.com>");
MODULE_DESCRIPTION("Thermal Framework driver");
MODULE_LICENSE("GPL");
