/* Copyright (c) 2008-2011, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include "k3_fb_panel.h"
#include "edc_overlay.h"


void set_reg(u32 addr, u32 val, u8 bw, u8 bs)
{
	u32 mask = (1 << bw) - 1;
	u32 tmp = inp32(addr);
	tmp &= ~(mask << bs);
	outp32(addr, tmp | ((val & mask) << bs));
}

int resource_cmds_tx(struct platform_device *pdev,
	struct resource_desc *cmds, int cnt)
{
	struct resource *res = NULL;
	struct resource_desc *cm = NULL;
	int i = 0;

	BUG_ON(pdev == NULL);
	cm = cmds;

	for (i = 0; i < cnt; i++) {
		if (cm == NULL) {
			k3fb_loge("cm(%d) is null!\n", i);
			continue;
		}

		BUG_ON(cm->name == NULL);

		res = platform_get_resource_byname(pdev, cm->flag, cm->name);
		if (!res) {
			k3fb_loge("failed to get %s resource!\n",
				(cm->name == NULL) ? "NULL" : cm->name);
		} else {
			*(cm->value) = res->start;
			cm++;
		}
	}

	return cnt;
}

int vcc_cmds_tx(struct platform_device *pdev, struct vcc_desc *cmds, int cnt)
{
	struct vcc_desc *cm = NULL;
	int i = 0;

	cm = cmds;

	for (i = 0; i < cnt; i++) {
		if (cm == NULL) {
			k3fb_loge("cm(%d) is null!\n", i);
			continue;
		}
		if (cm->dtype == DTYPE_VCC_GET) {
			BUG_ON(pdev == NULL);

			*(cm->regulator) = regulator_get(&pdev->dev, cm->id);
			if (IS_ERR(*(cm->regulator))) {
				k3fb_loge("failed to get %s regulator!\n", cm->id);
			}
		} else if (cm->dtype == DTYPE_VCC_PUT) {
			if (!IS_ERR(*(cm->regulator))) {
				regulator_put(*(cm->regulator));
			}
		} else if (cm->dtype == DTYPE_VCC_ENABLE) {
			if (!IS_ERR(*(cm->regulator))) {
				if (regulator_enable(*(cm->regulator)) != 0) {
					k3fb_loge("failed to enable %s regulator!\n",
						(cm->id == NULL) ? "NULL" : cm->id);
				}
			}
		} else if (cm->dtype == DTYPE_VCC_DISABLE) {
			if (!IS_ERR(*(cm->regulator))) {
				if (regulator_disable(*(cm->regulator)) != 0) {
					k3fb_loge("failed to disable %s regulator!\n",
						(cm->id == NULL) ? "NULL" : cm->id);
				}
			}
		} else if (cm->dtype == DTYPE_VCC_SET_VOLTAGE) {
			if (!IS_ERR(*(cm->regulator))) {
				if (regulator_set_voltage(*(cm->regulator), cm->min_uV, cm->max_uV) != 0) {
					k3fb_loge("failed to set %s regulator voltage!\n",
						(cm->id == NULL) ? "NULL" : cm->id);
				}
			}
		} else {
			k3fb_loge("dtype=%x NOT supported\n", cm->dtype);
		}

		cm++;
	}

	return cnt;
}

int iomux_cmds_tx(struct iomux_desc *cmds, int cnt)
{
	struct iomux_desc *cm = NULL;
	int i = 0;

	cm = cmds;

	for (i = 0; i < cnt; i++) {
		if (cm == NULL) {
			k3fb_loge("cm(%d) is null!\n", i);
			continue;
		}

		BUG_ON(cm->name == NULL);

		if (cm->dtype == DTYPE_IOMUX_GET) {
			if (cm->name == NULL) {
				k3fb_loge("Block name is NULL!\n");
			}
			*(cm->block) = iomux_get_block(cm->name);
			if (*(cm->block) == NULL) {
				k3fb_loge("failed to iomux_get_block, name=%s!\n",
					(cm->name == NULL) ? "NULL" : cm->name);

				continue;
			}

			*(cm->block_config) = iomux_get_blockconfig(cm->name);
			if (*(cm->block_config) == NULL) {
				k3fb_loge("failed to iomux_get_blockconfig, name=%s!\n",
					(cm->name == NULL) ? "NULL" : cm->name);

				continue;
			}
		} else if (cm->dtype == DTYPE_IOMUX_SET) {
			if (blockmux_set(*(cm->block), *(cm->block_config), cm->mode) != 0) {
				k3fb_loge("failed to blockmux_set, name=%s!\n",
					(cm->name == NULL) ? "NULL" : cm->name);

				continue;
			}
		}

		cm++;
	}

	return cnt;
}

int gpio_cmds_tx(struct gpio_desc *cmds, int cnt)
{
	struct gpio_desc *cm = NULL;
	int i = 0;

	cm = cmds;

	for (i = 0; i < cnt; i++) {
		if (cm == NULL) {
			k3fb_loge("cm(%d) is null!\n", i);
			continue;
		}
		if (!gpio_is_valid(cm->gpio)) {
			k3fb_loge("gpio invalid, dtype=%d, lable=%s, gpio=%d!\n",
				cm->dtype, (cm->label == NULL) ? "NULL" : cm->label, cm->gpio);

			continue;
		}

		if (cm->dtype == DTYPE_GPIO_INPUT) {
			if (gpio_direction_input(cm->gpio) != 0) {
				k3fb_loge("failed to gpio_direction_input, lable=%s, gpio=%d!\n",
					(cm->label == NULL) ? "NULL" : cm->label, cm->gpio);
			}
		} else if (cm->dtype == DTYPE_GPIO_OUTPUT) {
			if (gpio_direction_output(cm->gpio, cm->value) != 0) {
				k3fb_loge("failed to gpio_direction_output, label%s, gpio=%d!\n",
					(cm->label == NULL) ? "NULL" : cm->label, cm->gpio);
			}
		} else if (cm->dtype == DTYPE_GPIO_REQUEST) {
			if (gpio_request(cm->gpio, cm->label) != 0) {
				k3fb_loge("failed to gpio_request, lable=%s, gpio=%d!\n",
					(cm->label == NULL) ? "NULL" : cm->label, cm->gpio);
			}
		} else if (cm->dtype == DTYPE_GPIO_FREE) {
			gpio_free(cm->gpio);
		} else {
			k3fb_loge("dtype=%x NOT supported\n", cm->dtype);
		}

		if (cm->wait) {
			if (cm->waittype == WAIT_TYPE_US)
				udelay(cm->wait);
			else if (cm->waittype == WAIT_TYPE_MS)
				mdelay(cm->wait);
			else
				mdelay(cm->wait * 1000);
		}
		cm++;
	}

	return cnt;
}

int panel_next_on(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_panel_data *pdata = NULL;
	struct k3_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	BUG_ON(pdev == NULL);

	pdata = (struct k3_fb_panel_data *)pdev->dev.platform_data;
	if (pdata) {
		next_pdev = pdata->next;
		if (next_pdev) {
			next_pdata = (struct k3_fb_panel_data *)next_pdev->dev.platform_data;
			if ((next_pdata) && (next_pdata->on))
				ret = next_pdata->on(next_pdev);
		}
	}

	return ret;
}

int panel_next_off(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_panel_data *pdata = NULL;
	struct k3_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	BUG_ON(pdev == NULL);

	pdata = (struct k3_fb_panel_data *)pdev->dev.platform_data;
	if (pdata) {
		next_pdev = pdata->next;
		if (next_pdev) {
			next_pdata = (struct k3_fb_panel_data *)next_pdev->dev.platform_data;
			if ((next_pdata) && (next_pdata->off))
				ret = next_pdata->off(next_pdev);
		}
	}

	return ret;
}

int panel_next_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_panel_data *pdata = NULL;
	struct k3_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	BUG_ON(pdev == NULL);

	pdata = (struct k3_fb_panel_data *)pdev->dev.platform_data;
	if (pdata) {
		next_pdev = pdata->next;
		if (next_pdev) {
			next_pdata = (struct k3_fb_panel_data *)next_pdev->dev.platform_data;
			if ((next_pdata) && (next_pdata->remove))
				ret = next_pdata->remove(next_pdev);
		}
	}

	return ret;
}

int panel_next_set_backlight(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_panel_data *pdata = NULL;
	struct k3_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	BUG_ON(pdev == NULL);

	pdata = (struct k3_fb_panel_data *)pdev->dev.platform_data;
	if (pdata) {
		next_pdev = pdata->next;
		if (next_pdev) {
			next_pdata = (struct k3_fb_panel_data *)next_pdev->dev.platform_data;
			if ((next_pdata) && (next_pdata->set_backlight))
				ret = next_pdata->set_backlight(next_pdev);
		}
	}

	return ret;
}

int panel_next_set_timing(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_panel_data *pdata = NULL;
	struct k3_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	BUG_ON(pdev == NULL);

	pdata = (struct k3_fb_panel_data *)pdev->dev.platform_data;
	if (pdata) {
		next_pdev = pdata->next;
		if (next_pdev) {
			next_pdata = (struct k3_fb_panel_data *)next_pdev->dev.platform_data;
			if ((next_pdata) && (next_pdata->set_timing))
				ret = next_pdata->set_timing(next_pdev);
		}
	}

	return ret;
}

int panel_next_set_frc(struct platform_device *pdev, int target_fps)
{
	int ret = 0;
	struct k3_fb_panel_data *pdata = NULL;
	struct k3_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	BUG_ON(pdev == NULL);

	pdata = (struct k3_fb_panel_data *)pdev->dev.platform_data;
	if (pdata) {
		next_pdev = pdata->next;
		if (next_pdev) {
			next_pdata = (struct k3_fb_panel_data *)next_pdev->dev.platform_data;
			if ((next_pdata) && (next_pdata->set_frc))
				ret = next_pdata->set_frc(next_pdev, target_fps);
		}
	}

	return ret;
}

int panel_next_check_esd(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_fb_panel_data *pdata = NULL;
	struct k3_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	BUG_ON(pdev == NULL);

	pdata = (struct k3_fb_panel_data *)pdev->dev.platform_data;
	if (pdata) {
		next_pdev = pdata->next;
		if (next_pdev) {
			next_pdata = (struct k3_fb_panel_data *)next_pdev->dev.platform_data;
			if ((next_pdata) && (next_pdata->check_esd))
				ret = next_pdata->check_esd(next_pdev);
		}
	}

	return ret;
}

struct platform_device *k3_fb_device_alloc(struct k3_fb_panel_data *pdata,
	u32 type, u32 index, u32 *graphic_ch)
{
	struct platform_device *this_dev = NULL;
	char dev_name[16] = {0};

	BUG_ON(pdata == NULL);

	switch (type) {
	case PANEL_HDMI:
		snprintf(dev_name, sizeof(dev_name), DEV_NAME_LDI);
		*graphic_ch = OVERLAY_PIPE_EDC1_CH1;
		break;
	case PANEL_DP:
		snprintf(dev_name, sizeof(dev_name), DEV_NAME_DP);
		*graphic_ch = OVERLAY_PIPE_EDC1_CH1;
		break;
	case PANEL_LDI:
		snprintf(dev_name, sizeof(dev_name), DEV_NAME_LDI);
		*graphic_ch = OVERLAY_PIPE_EDC0_CH2;
		break;
	case PANEL_MIPI_VIDEO:
	case PANEL_MIPI_CMD:
		snprintf(dev_name, sizeof(dev_name), DEV_NAME_MIPIDSI);
		*graphic_ch = OVERLAY_PIPE_EDC0_CH2;
		break;
	default:
		k3fb_loge("invalid panel type = %d!\n", type);
		return NULL;
	}

	pdata->next = NULL;

	this_dev = platform_device_alloc(dev_name, index + 1);
	if (this_dev) {
		if (platform_device_add_data(this_dev, pdata, sizeof(struct k3_fb_panel_data))) {
			k3fb_loge("failed to platform_device_add_data!\n");
			platform_device_put(this_dev);
			return NULL;
		}
	}

	return this_dev;
}

void  k3_fb_device_free(struct platform_device *pdev)
{
	BUG_ON(pdev == NULL);
	platform_device_put(pdev);
}
