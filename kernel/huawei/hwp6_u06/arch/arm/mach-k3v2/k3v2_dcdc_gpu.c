/*
 * Copyright (c) 2011 Hisilicon Technologies Co., Ltd. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <mach/platform.h>
#include <mach/irqs.h>
#include <linux/ipps.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include "k3v2_dcdc_gpu.h"

#include <mach/product_feature_sel.h>	

static DEFINE_MUTEX(reg_lock_mutex);
#define lock(x) mutex_lock(x)
#define unlock(x)  mutex_unlock(x)
struct ipps_gpu *g_ipps_gpu = NULL;
struct gpu_policy_info *g_ipps_policy = NULL;

static unsigned int freq_to_index(unsigned int freq)
{
	unsigned int index = 0;

	while (gpu_profile_dcdc[index].freq != 0) {
		if (gpu_profile_dcdc[index].freq >= freq)
			break;
		index++;
	}

	if (index && gpu_profile_dcdc[index].freq == 0)
		index--;

	return index;
}

static u32 computer_gpu_target_profile(u32 count, struct ipps_gpu *ipps_gpu)
{
	u32 cur_profile = 0;
	u32 up_threshold = 0;
	u32 down_threshold = 0;
	u32 target_profile = 0;

	cur_profile = ipps_gpu->curr_profile;
	target_profile = ipps_gpu->curr_profile;
	up_threshold = gpu_profile_dcdc[cur_profile].freq * DEFAULT_GPUDVFS_PERIOD * g_ipps_policy[cur_profile].up_threshold / 100;
	down_threshold = gpu_profile_dcdc[cur_profile].freq * DEFAULT_GPUDVFS_PERIOD * g_ipps_policy[cur_profile].down_threshold / 100;
	if ((count < down_threshold) && (cur_profile > GPU_PROFILE0)) {
		ipps_gpu->downtimes++;
		ipps_gpu->uptimes = 0;
		if (ipps_gpu->downtimes >= g_ipps_policy[cur_profile].downtimes) {
			target_profile--;
			ipps_gpu->downtimes = 0;
		}
	} else if ((count > up_threshold) && (cur_profile < (PROFILE_END - 1))) {
		ipps_gpu->downtimes = 0;
		ipps_gpu->uptimes++;
		if ((ipps_gpu->uptimes >= g_ipps_policy[cur_profile].uptimes) && (g_ipps_policy[cur_profile].up_threshold < 100)) {
			target_profile++;
			ipps_gpu->uptimes = 0;
		}
	} else {
		ipps_gpu->uptimes = 0;
		ipps_gpu->downtimes = 0;
	}

	return target_profile;
}

static u32 gpu_temp_monitor(u32 profile, struct ipps_gpu *ipps_gpu)
{
	u32 val;
	u32 target = profile;

	val = readl(TEMP_VAL_REG);
	if (val > TEMP_HIGH) {
		if (ipps_gpu->temp_enable == TEMP_MON_DISABLE) {
			ipps_gpu->temp_enable = TEMP_MON_ENABLE;
		}
	} else if (val < TEMP_LOW) {
		if (ipps_gpu->temp_enable == TEMP_MON_ENABLE) {
			ipps_gpu->temp_enable = TEMP_MON_DISABLE;
		}
	}

	if(ipps_gpu->temp_enable == TEMP_MON_DISABLE) {
		return target;
	}

	if (target > ipps_gpu->gpu.freq.safe) {
		target = ipps_gpu->gpu.freq.safe;
	}

	return target;
}

static void gpu_check_dvfs_done(void)
{
	int val = 0;
	int i = 500;

	val = readl(DVFS_DONE_REG);
	while (!(val & G3D_DVFS_INTR_STATUS) && i) {
		val = readl(DVFS_DONE_REG);
		udelay(1);
		i--;
	}
}

static void change_gpu_profile(u32 target_profile, struct ipps_gpu *ipps_gpu)
{
	int ret = 0;

	if (target_profile == ipps_gpu->curr_profile)
		return;

	/*if profile up,voltage is set firstly*/
	if (target_profile > ipps_gpu->curr_profile) {
		regulator_set_voltage(ipps_gpu->regulator_dcdc, gpu_profile_dcdc[target_profile].dcdc_vol, gpu_profile_dcdc[target_profile].dcdc_vol);
		udelay(300);
	}

#ifdef WITH_G3D_600M_PROF
	/*
	 *if profile change to 600M profile which uses hdmipll,
	 *set the rate of hdmipll and enable, and not bypass.
	 */
	if (target_profile == G3D_PLLSW_PROF_ID) {
		ret = clk_enable(ipps_gpu->clk_hdmipll);
		if (ret)
			WARN(1, "K3V2 DCDC GPU:failed to enable clock,ret=%d.\n", ret);
	}
#endif

	/*profile clock param is set*/
	if (readl(ipps_gpu->pmctrl_base + G3D_CLK_SEL2)) {
		writel(gpu_profile_dcdc[target_profile].gpu_clk_profile, ipps_gpu->pmctrl_base + G3D_CLK_PROFILE);
	} else {
		writel((gpu_profile_dcdc[target_profile].gpu_clk_profile | G3D_CHAN_MASK), ipps_gpu->pmctrl_base + G3D_CLK_PROFILE);
	}

	/*clear the intr status of dvfs done*/
	writel(0x2, ipps_gpu->pmctrl_base + G3D_DVFS_CLR);
	/*
	 *if profile down, start dvfs, check dvfs done and set voltage,
	 *if profile up,start dvfs, check dvfs done,voltage was set at the beginning.
	 */
	if (target_profile < ipps_gpu->curr_profile) {
		writel(0x0, ipps_gpu->pmctrl_base + G3D_DVFS_START);
		gpu_check_dvfs_done();
	    regulator_set_voltage(ipps_gpu->regulator_dcdc, gpu_profile_dcdc[target_profile].dcdc_vol, gpu_profile_dcdc[target_profile].dcdc_vol);
	} else {
		writel(0x1, ipps_gpu->pmctrl_base + G3D_DVFS_START);
		gpu_check_dvfs_done();
	}
#ifdef WITH_G3D_600M_PROF
	/*if profile changed from 600M profile which uses hdmipll, hdmipll must be disabled*/
	if (ipps_gpu->curr_profile == G3D_PLLSW_PROF_ID) {
		clk_disable(ipps_gpu->clk_hdmipll);
	}
#endif
	ipps_gpu->curr_profile = target_profile;
}

static void start_gpu_dvfs(struct ipps_gpu *ipps_gpu)
{
	u32 gpu_count = 0;
	u32 target_profile = 0;

	/*step1:get gpu mon count */
	gpu_count = readl(G3D_MON_COUNTER);
	/*step2:clr count*/
	writel(0, G3D_MON_COUNTER);
	/*step3:computer target profile*/
	target_profile = computer_gpu_target_profile(gpu_count, ipps_gpu);
	lock(&reg_lock_mutex);
	if (target_profile < ipps_gpu->gpu.freq.min) {
		target_profile = ipps_gpu->gpu.freq.min;
	} else if (target_profile > ipps_gpu->gpu.freq.max) {
		target_profile = ipps_gpu->gpu.freq.max;
	}
	/*step4:check temp monitor*/
	target_profile = gpu_temp_monitor(target_profile, ipps_gpu);
	/*step5:change profile*/
	if (target_profile != ipps_gpu->curr_profile) {
		change_gpu_profile(target_profile, ipps_gpu);
	}
	unlock(&reg_lock_mutex);
  
}

static void enable_gpu_dvfs(struct ipps_gpu *ipps_gpu)
{
	writel(1, ipps_gpu->pmctrl_base + G3D_DVFS_ENABLE);
}

static void disable_gpu_dvfs(struct ipps_gpu *ipps_gpu)
{
	writel(0, ipps_gpu->pmctrl_base + G3D_DVFS_ENABLE);
}
static void do_gpu_dvfs(struct work_struct *work)
{
	int delay = usecs_to_jiffies(DEFAULT_GPUDVFS_PERIOD * 1000);
	delay -= jiffies % delay;

	if (g_ipps_gpu) {
		start_gpu_dvfs(g_ipps_gpu);
		queue_delayed_work_on(0, g_ipps_gpu->gpu_dvfs_wq, &g_ipps_gpu->k_gpu_work, delay);
	} else {
		WARN(1, "K3V2 DCDC GPU:g_ipps_gpu is null.\n");
	}
}

static inline void gpu_dvfs_init(void)
{
	if (g_ipps_gpu)
		INIT_DELAYED_WORK(&g_ipps_gpu->k_gpu_work, do_gpu_dvfs);
	else
		WARN(1, "K3V2 DCDC GPU:g_ipps_gpu is null.\n");
}

int start_gpu_monitor(struct ipps_gpu *ipps_gpu)
{
	ipps_gpu->curr_profile = GPU_PROFILE0;

	if (ipps_gpu->gpu_dvfs_wq) {
		enable_gpu_dvfs(ipps_gpu);
		queue_delayed_work_on(0, ipps_gpu->gpu_dvfs_wq, &ipps_gpu->k_gpu_work, usecs_to_jiffies(DEFAULT_GPUDVFS_PERIOD * 1000));
	}
	else{
		WARN(1, "K3V2 DCDC GPU:k_gpu_work is null.\n");
	}

	return 0;
}

int  stop_gpu_monitor(struct ipps_gpu *ipps_gpu)
{
	if (ipps_gpu->gpu_dvfs_wq) {
		cancel_delayed_work_sync(&ipps_gpu->k_gpu_work);

#ifdef WITH_G3D_600M_PROF
		if (ipps_gpu->curr_profile == G3D_PLLSW_PROF_ID) {
			clk_disable(ipps_gpu->clk_hdmipll);
		}
#endif

		ipps_gpu->curr_profile = GPU_PROFILE0;
		disable_gpu_dvfs(ipps_gpu);
	}

	return 0;
}

int k3v2_dcdc_gpu_cmd(struct device *dev,
			enum ipps_cmd_type cmd, unsigned int object, void *data)
{
	int ret = 0;
	struct ipps_gpu *ipps_gpu = g_ipps_gpu;

	if  (!(object & IPPS_OBJ_GPU)) {
		pr_err("K3V2 DCDC GPU:object is not matched with cmd.\n");
		ret = -EINVAL;
		goto out;
	}

	if (!data) {
		pr_err("K3V2 DCDC GPU:input param is invalid.\n");
		ret = -EINVAL;
		goto out;
	}

	if (!ipps_gpu) {
		pr_err("K3V2 DCDC GPU:dev private data is NULL.\n");
		ret = -EINVAL;
		goto out;
	}

	switch (cmd) {
	case IPPS_GET_FREQS_TABLE:
	{
		int index = 0;
		struct cpufreq_frequency_table *table = (struct cpufreq_frequency_table *)data;
		while (gpu_profile_dcdc[index].freq != 0) {
			table->index = index;
			table->frequency = gpu_profile_dcdc[index].freq;
			table++;
			index++;
		}
		table->index = index;
		table->frequency = CPUFREQ_TABLE_END;
		break;
	}
	case IPPS_GET_PARAM:
	{
		lock(&reg_lock_mutex);
		struct ipps_param *param = (struct ipps_param *) data;
		param->gpu.max_freq = gpu_profile_dcdc[ipps_gpu->gpu.freq.max].freq;
		param->gpu.min_freq = gpu_profile_dcdc[ipps_gpu->gpu.freq.min].freq;
		param->gpu.safe_freq = gpu_profile_dcdc[ipps_gpu->gpu.freq.safe].freq;
		unlock(&reg_lock_mutex);
		break;
	}
	case IPPS_SET_PARAM:
	{
		lock(&reg_lock_mutex);
		struct ipps_param *param = (struct ipps_param *) data;
		if (param->gpu.block_freq != 0) {
			ipps_gpu->gpu.freq.min = freq_to_index(param->gpu.block_freq);
			ipps_gpu->gpu.freq.max = ipps_gpu->gpu.freq.min;
		} else {
			ipps_gpu->gpu.freq.min = freq_to_index(param->gpu.min_freq);
			ipps_gpu->gpu.freq.max = freq_to_index(param->gpu.max_freq);
		}
		ipps_gpu->gpu.freq.safe = freq_to_index(param->gpu.safe_freq);
		unlock(&reg_lock_mutex);
		break;
	}
	case IPPS_GET_CUR_FREQ:
		*(unsigned int *)data = gpu_profile_dcdc[ipps_gpu->curr_profile].freq;
		break;
	case IPPS_GET_CUR_POLICY:
		*(unsigned int *)data = ipps_gpu->curr_policy;
		break;
	case IPPS_SET_CUR_POLICY:
		lock(&reg_lock_mutex);
		ipps_gpu->curr_policy = *(unsigned int *)data >> 4;
		if (((POWERSAVE_POLICY <= ipps_gpu->curr_policy) && (ipps_gpu->curr_policy <=SPEC0B_POLICY)) ||
			(ipps_gpu->curr_policy == NORMAL_POLICY) || (ipps_gpu->curr_policy == PERF_POLICY)) {
			g_ipps_policy = policy_table[ipps_gpu->curr_policy];
		} else {
			ret = -EINVAL;
			WARN(1, "K3V2 DCDC GPU:policy is invalid.\n");
		}
		unlock(&reg_lock_mutex);
		break;
	case IPPS_GET_MODE:
		*(unsigned int *)data = ipps_gpu->curr_mode;
		break;
	case IPPS_SET_MODE:
		if (*(unsigned int *)data == IPPS_ENABLE) {
			start_gpu_monitor(ipps_gpu);
			ipps_gpu->curr_mode = IPPS_ENABLE;
		} else if (*(unsigned int *)data == IPPS_DISABLE) {
			stop_gpu_monitor(ipps_gpu);
			ipps_gpu->curr_mode = IPPS_DISABLE;
		} else {
			WARN(1, "K3V2 DCDC GPU:mode is error.\n");
			ret = -EINVAL;
			goto out;
		}
		break;
	default:
		dev_warn(&ipps_gpu->pdev->dev, "K3V2 DCDC GPU:input cmd is error.\n");
		ret = -EINVAL;
		goto out;
	}

out:
	return ret;
}

static ssize_t gpu_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct ipps_gpu *ipps_gpu = g_ipps_gpu;

	return snprintf(buf, 240,
			"clk:%x, vol:%x, min_profile:%d     max_profile:%d    current_profile:%d   current_policy:%d\n",
			readl(ipps_gpu->pmctrl_base + G3D_CLK_PROFILE),
			readl(ipps_gpu->pmctrl_base + G3D_VOL_PROFILE),
			ipps_gpu->gpu.freq.min, ipps_gpu->gpu.freq.max,
			ipps_gpu->curr_profile,
			ipps_gpu->curr_policy);
}

static ssize_t gpu_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct ipps_gpu *ipps_gpu = g_ipps_gpu;
	if (buf != NULL && strlen(buf)) {
		switch (*buf) {
		case 'G':
			{
				int cmd;
				if (sscanf(buf, "G %d", &cmd)) {
					if (cmd == 1) {
						start_gpu_monitor(ipps_gpu);
					} else if (cmd == 0) {
						stop_gpu_monitor(ipps_gpu);
					}
				}
			}
			break;
		case 'I':
			{
				int gpuprofile;
				if (sscanf(buf, "I %d", &gpuprofile) && gpuprofile >= 0\
					&& gpuprofile <= (PROFILE_END - 1)) {
					change_gpu_profile(gpuprofile, ipps_gpu);
				}
			}
			break;
		default:
			printk("Cmd %s parse error!", buf);
			break;
		}
	}

	return count;
}
static DEVICE_ATTR(gpu, 0664, gpu_show, gpu_store);
static struct attribute *k3_dcdc_gpu_attr[] = {
	&dev_attr_gpu.attr,
	NULL,
};

static struct attribute_group k3_dcdc_gpu_attr_group = {
	.attrs = k3_dcdc_gpu_attr,
};



int k3v2_dcdc_gpu_probe(struct platform_device *pdev)
{
	int err = 0;
	struct plat_data *ipps_init_data = NULL;
	struct resource *pmctrl_res = NULL;
	struct device *dev = NULL;
	struct ipps_gpu *ipps_gpu = NULL;

#ifdef CONFIG_EXTRAL_DYNAMIC_DCDC
	if(!get_product_feature(PROD_FEATURE_GPU_DCDC_SUPPLY)) {
		dev_err(&pdev->dev, "hardware version is invalid.\n");
		return -EINVAL;
	}
#endif	

	dev = &pdev->dev;
	ipps_init_data = pdev->dev.platform_data;
	if (!ipps_init_data) {
		dev_err(&pdev->dev, "failed to get driver data.\n");
		return -EINVAL;
	}
    
	pmctrl_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!pmctrl_res) {
		dev_err(&pdev->dev, "failed to get pm I/O memory.\n");
		return -ENXIO;
	}

	ipps_gpu = kzalloc(sizeof(struct ipps_gpu), GFP_KERNEL);
	if (!ipps_gpu) {
		dev_err(&pdev->dev, "failed to kzalloc memory for dcdc gpu.\n");
		return -ENOMEM;
	}

	ipps_gpu->pmctrl_base = ioremap(pmctrl_res->start, resource_size(pmctrl_res));
	if (!ipps_gpu->pmctrl_base) {
		dev_err(&pdev->dev, "failed to remap pm_base I/O memory.\n");
		err = -ENXIO;
		goto err_free_mem;
	}

	ipps_gpu->regulator_dcdc = regulator_get(NULL, "gpu-vcc");
	if (IS_ERR(ipps_gpu->regulator_dcdc)) {
		dev_err(&pdev->dev, "regulator_get gpu dcdc failed.\n");
		err = PTR_ERR(ipps_gpu->regulator_dcdc);
		goto err_iounmap_pmsctrl_base;
	}

	/*get gpu dcdc*/
	ipps_gpu->clk_hdmipll = clk_get(NULL, "clk_pll4");
	if (IS_ERR(ipps_gpu->clk_hdmipll)) {
		dev_err(&pdev->dev, "hdmipll clock get failed.\n");
		err = PTR_ERR(ipps_gpu->clk_hdmipll);
		goto err_regulator_put;
	}

	ipps_gpu->curr_profile = ipps_init_data->cur_profile;
	ipps_gpu->curr_mode = ipps_init_data->mode;
	ipps_gpu->curr_policy = ipps_init_data->policy;
	ipps_gpu->gpu.freq.max = ipps_init_data->max;
	ipps_gpu->gpu.freq.min = ipps_init_data->min;
	ipps_gpu->gpu.freq.safe = ipps_init_data->safe;
	ipps_gpu->temp_enable = TEMP_MON_DISABLE;
	ipps_gpu->uptimes = 0;
	ipps_gpu->downtimes = 0;
	ipps_gpu->idev = ipps_alloc_device(sizeof(struct ipps_device));
	if (NULL == ipps_gpu->idev) {
		dev_err(&pdev->dev, "failed to alloc idev mem!\n");
		err = PTR_ERR(ipps_gpu->idev);
		goto err_regulator_put;
	}
	ipps_gpu->idev->command = k3v2_dcdc_gpu_cmd;
	ipps_gpu->idev->object = ipps_init_data->obj;
	ipps_gpu->idev->dev = dev;

	g_ipps_gpu = ipps_gpu;
	if (((POWERSAVE_POLICY <= ipps_gpu->curr_policy) && (ipps_gpu->curr_policy <=SPEC0B_POLICY)) ||
			(ipps_gpu->curr_policy == NORMAL_POLICY) || (ipps_gpu->curr_policy == PERF_POLICY)) {
		g_ipps_policy = policy_table[ipps_gpu->curr_policy];
	} else {
		err = -EINVAL;
		WARN(1, "K3V2 DCDC GPU:policy is invalid.\n");
		goto err_free_idev;
	}

	/*
	 *ipps_register_device() uses g_ipps_gpu pointer
	 *to excute client->add()
	 */
	err = ipps_register_device(ipps_gpu->idev);
	if (err) {
		dev_err(&pdev->dev, "ipps device register failed!\n");
		goto err_free_idev;
	}

	err = sysfs_create_group(&dev->kobj, &k3_dcdc_gpu_attr_group);
	if (err) {
		dev_err(dev, "failed to creat sysfs file\n");
		goto err_idev_unregister;
	}
#if 1
	ipps_gpu->gpu_dvfs_wq = create_singlethread_workqueue("gpudvfs");
	if (ipps_gpu->gpu_dvfs_wq == NULL) {
		dev_err(&pdev->dev, "create_workqueue failed for gpu dcdc\n");
		err = PTR_ERR(ipps_gpu->gpu_dvfs_wq);
		goto err_remove_sysfs_group;
	}
#endif
	mutex_init(&reg_lock_mutex);
	gpu_dvfs_init();
	return 0;
err_remove_sysfs_group:
	sysfs_remove_group(&pdev->dev.kobj, &k3_dcdc_gpu_attr_group);
err_idev_unregister:
	ipps_unregister_device(ipps_gpu->idev);
err_free_idev:
	ipps_dealloc_device(ipps_gpu->idev);
err_regulator_put:
	regulator_put(ipps_gpu->regulator_dcdc);
err_iounmap_pmsctrl_base:
	iounmap(ipps_gpu->pmctrl_base);
err_free_mem:
	kfree(ipps_gpu);

	return err;
}

static int k3v2_dcdc_gpu_remove(struct platform_device *pdev)
{
	struct ipps_gpu *ipps_gpu = g_ipps_gpu;
    printk("enter %s \n", __func__);
	destroy_workqueue(ipps_gpu->gpu_dvfs_wq);
	sysfs_remove_group(&pdev->dev.kobj, &k3_dcdc_gpu_attr_group);
	regulator_put(ipps_gpu->regulator_dcdc);
	clk_put(ipps_gpu->clk_hdmipll);
	iounmap(ipps_gpu->pmctrl_base);
	ipps_unregister_device(ipps_gpu->idev);
	ipps_dealloc_device(ipps_gpu->idev);
	kfree(ipps_gpu);
	g_ipps_gpu = NULL;
	g_ipps_policy = NULL;
	dev_info(&pdev->dev, "K3V2 DCDC GPU:k3v2_dcdc_gpu_remove.\n");

	return 0;
}

static struct platform_driver k3v2_dcdc_gpu_driver = {
	.probe		= k3v2_dcdc_gpu_probe,
	.remove		= __devexit_p(k3v2_dcdc_gpu_remove),
	.suspend = NULL,
	.resume = NULL,
	.driver = {
	.name = MODULE_NAME,
	.owner = THIS_MODULE,
	},
};

static int __init k3v2_dcdc_gpu_init(void)
{
	return platform_driver_register(&k3v2_dcdc_gpu_driver);
}

void __exit k3v2_dcdc_gpu_exit(void)
{
	platform_driver_unregister(&k3v2_dcdc_gpu_driver);
}

module_init(k3v2_dcdc_gpu_init);
module_exit(k3v2_dcdc_gpu_exit);
