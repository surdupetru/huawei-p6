
#include "dwc_otg_cil.h"
#include "dwc_os.h"
#include "dwc_otg_core_param.h"


#if 0
#define paramlog(format, arg...)    \
	do {                 \
		printk(KERN_INFO "[param]"format, ##arg); \
	} while(0)
#else
#define paramlog(format, arg...)
#endif


/* Checks if the parameter is outside of its valid range of values */
#define DWC_OTG_PARAM_TEST(_param_, _low_, _high_) \
		(((_param_) < (_low_)) || \
		((_param_) > (_high_)))

static int dwc_otg_param_initialized(int32_t val)
{
	return val != -1;
}

static void dwc_otg_set_uninitialized(int32_t * p, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		p[i] = -1;
	}
}

/* Parameter access functions */
int dwc_otg_set_param_otg_cap(dwc_otg_core_if_t * core_if, int32_t val)
{
	int valid;
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 2)) {
		DWC_WARN("Wrong value for otg_cap parameter\n");
		DWC_WARN("otg_cap parameter must be 0,1 or 2\n");
		retval = -DWC_E_INVALID;
		goto out;
	}

	valid = 1;
	switch (val) {
	case DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE:
		if (core_if->hwcfg2.b.op_mode != DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG)
			valid = 0;
		break;
	case DWC_OTG_CAP_PARAM_SRP_ONLY_CAPABLE:
		if ((core_if->hwcfg2.b.op_mode != DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG)
		    && (core_if->hwcfg2.b.op_mode != DWC_HWCFG2_OP_MODE_SRP_ONLY_CAPABLE_OTG)
		    && (core_if->hwcfg2.b.op_mode != DWC_HWCFG2_OP_MODE_SRP_CAPABLE_DEVICE)
		    && (core_if->hwcfg2.b.op_mode != DWC_HWCFG2_OP_MODE_SRP_CAPABLE_HOST)) {
			valid = 0;
		}
		break;
	case DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE:
		/* always valid */
		break;
	}

	if (!valid) {
		if (dwc_otg_param_initialized(core_if->core_params->otg_cap)) {
			DWC_ERROR("%d invalid for otg_cap paremter. Check HW configuration.\n", val);
		}
		val = (((core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG)
		      || (core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_SRP_ONLY_CAPABLE_OTG)
		      || (core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_SRP_CAPABLE_DEVICE)
		      || (core_if->hwcfg2.b.op_mode == DWC_HWCFG2_OP_MODE_SRP_CAPABLE_HOST)) ?
		     DWC_OTG_CAP_PARAM_SRP_ONLY_CAPABLE : DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->otg_cap = val;
	paramlog("otg_cap: %d\n", core_if->core_params->otg_cap);
out:
	return retval;
}

int32_t dwc_otg_get_param_otg_cap(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->otg_cap;
}

int dwc_otg_set_param_opt(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for opt parameter\n");
		return -DWC_E_INVALID;
	}
	core_if->core_params->opt = val;
	paramlog("opt: %d\n", core_if->core_params->opt);
	return 0;
}

int32_t dwc_otg_get_param_opt(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->opt;
}

int dwc_otg_set_param_dma_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for dma enable\n");
		return -DWC_E_INVALID;
	}

	if ((val == 1) && (core_if->hwcfg2.b.architecture == 0)) {
		if (dwc_otg_param_initialized(core_if->core_params->dma_enable)) {
			DWC_ERROR
			    ("%d invalid for dma_enable paremter. Check HW configuration.\n",
			     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->dma_enable = val;
	if (val == 0) {
		dwc_otg_set_param_dma_desc_enable(core_if, 0);
	}
	paramlog("dma_enable: %d\n", core_if->core_params->dma_enable);
	return retval;
}

int32_t dwc_otg_get_param_dma_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->dma_enable;
}

int dwc_otg_set_param_dma_desc_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for dma_enable\n");
		DWC_WARN("dma_desc_enable must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if ((val == 1)
	    && ((dwc_otg_get_param_dma_enable(core_if) == 0)
		|| (core_if->hwcfg4.b.desc_dma == 0))) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->dma_desc_enable)) {
			DWC_ERROR
			    ("%d invalid for dma_desc_enable paremter. Check HW configuration.\n",
			     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}
	core_if->core_params->dma_desc_enable = val;
	paramlog("dma_desc_enable: %d\n", core_if->core_params->dma_desc_enable);
	return retval;
}

int32_t dwc_otg_get_param_dma_desc_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->dma_desc_enable;
}

int dwc_otg_set_param_host_support_fs_ls_low_power(dwc_otg_core_if_t * core_if,
						   int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for host_support_fs_low_power\n");
		DWC_WARN("host_support_fs_low_power must be 0 or 1\n");
		return -DWC_E_INVALID;
	}
	core_if->core_params->host_support_fs_ls_low_power = val;
	paramlog("host_support_fs_ls_low_power: %d\n",
		core_if->core_params->host_support_fs_ls_low_power);
	return 0;
}

int32_t dwc_otg_get_param_host_support_fs_ls_low_power(dwc_otg_core_if_t *
						       core_if)
{
	return core_if->core_params->host_support_fs_ls_low_power;
}

int dwc_otg_set_param_enable_dynamic_fifo(dwc_otg_core_if_t * core_if,
					  int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for enable_dynamic_fifo\n");
		DWC_WARN("enable_dynamic_fifo must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if ((val == 1) && (core_if->hwcfg2.b.dynamic_fifo == 0)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->enable_dynamic_fifo)) {
			DWC_ERROR
			    ("%d invalid for enable_dynamic_fifo paremter. Check HW configuration.\n",
			     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}
	core_if->core_params->enable_dynamic_fifo = val;
	paramlog("enable_dynamic_fifo: %d\n", core_if->core_params->enable_dynamic_fifo);
	return retval;
}

int32_t dwc_otg_get_param_enable_dynamic_fifo(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->enable_dynamic_fifo;
}

int dwc_otg_set_param_data_fifo_size(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 32, 32768)) {
		DWC_WARN("Wrong value for data_fifo_size\n");
		DWC_WARN("data_fifo_size must be 32-32768\n");
		return -DWC_E_INVALID;
	}

	if (val > core_if->hwcfg3.b.dfifo_depth) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->data_fifo_size)) {
			DWC_ERROR
			    ("%d invalid for data_fifo_size parameter. Check HW configuration.\n",
			     val);
		}
		val = core_if->hwcfg3.b.dfifo_depth;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->data_fifo_size = val;
	paramlog("data_fifo_size: %d\n", core_if->core_params->data_fifo_size);
	return retval;
}

int32_t dwc_otg_get_param_data_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->data_fifo_size;
}

int dwc_otg_set_param_dev_rx_fifo_size(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 16, 32768)) {
		DWC_WARN("Wrong value for dev_rx_fifo_size\n");
		DWC_WARN("dev_rx_fifo_size must be 16-32768\n");
		return -DWC_E_INVALID;
	}

	if (val > DWC_READ_REG32(&core_if->core_global_regs->grxfsiz)) {
		if (dwc_otg_param_initialized(core_if->core_params->dev_rx_fifo_size)) {
		DWC_WARN("%d invalid for dev_rx_fifo_size parameter\n", val);
		}
		val = DWC_READ_REG32(&core_if->core_global_regs->grxfsiz);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->dev_rx_fifo_size = val;
	paramlog("dev_rx_fifo_size: %d\n", core_if->core_params->dev_rx_fifo_size);
	return retval;
}

int32_t dwc_otg_get_param_dev_rx_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->dev_rx_fifo_size;
}

int dwc_otg_set_param_dev_nperio_tx_fifo_size(dwc_otg_core_if_t * core_if,
					      int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 16, 32768)) {
		DWC_WARN("Wrong value for dev_nperio_tx_fifo\n");
		DWC_WARN("dev_nperio_tx_fifo must be 16-32768\n");
		return -DWC_E_INVALID;
	}

	if (val > (DWC_READ_REG32(&core_if->core_global_regs->gnptxfsiz) >> 16)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->dev_nperio_tx_fifo_size)) {
			DWC_ERROR
			    ("%d invalid for dev_nperio_tx_fifo_size. Check HW configuration.\n",
			     val);
		}
		val =
		    (DWC_READ_REG32(&core_if->core_global_regs->gnptxfsiz) >>
		     16);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->dev_nperio_tx_fifo_size = val;
	paramlog("dev_nperio_tx_fifo_size: %d\n",
		core_if->core_params->dev_nperio_tx_fifo_size);
	return retval;
}

int32_t dwc_otg_get_param_dev_nperio_tx_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->dev_nperio_tx_fifo_size;
}

int dwc_otg_set_param_host_rx_fifo_size(dwc_otg_core_if_t * core_if,
					int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 16, 32768)) {
		DWC_WARN("Wrong value for host_rx_fifo_size\n");
		DWC_WARN("host_rx_fifo_size must be 16-32768\n");
		return -DWC_E_INVALID;
	}

	if (val > DWC_READ_REG32(&core_if->core_global_regs->grxfsiz)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->host_rx_fifo_size)) {
			DWC_ERROR
			    ("%d invalid for host_rx_fifo_size. Check HW configuration.\n",
			     val);
		}
		val = DWC_READ_REG32(&core_if->core_global_regs->grxfsiz);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->host_rx_fifo_size = val;
	paramlog("host_rx_fifo_size: %d\n",
		core_if->core_params->host_rx_fifo_size);
	return retval;

}

int32_t dwc_otg_get_param_host_rx_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->host_rx_fifo_size;
}

int dwc_otg_set_param_host_nperio_tx_fifo_size(dwc_otg_core_if_t * core_if,
					       int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 16, 32768)) {
		DWC_WARN("Wrong value for host_nperio_tx_fifo_size\n");
		DWC_WARN("host_nperio_tx_fifo_size must be 16-32768\n");
		return -DWC_E_INVALID;
	}

	if (val > (DWC_READ_REG32(&core_if->core_global_regs->gnptxfsiz) >> 16)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->host_nperio_tx_fifo_size)) {
			DWC_ERROR
			    ("%d invalid for host_nperio_tx_fifo_size. Check HW configuration.\n",
			     val);
		}
		val =
		    (DWC_READ_REG32(&core_if->core_global_regs->gnptxfsiz) >>
		     16);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->host_nperio_tx_fifo_size = val;
	paramlog("host_nperio_tx_fifo_size: %d\n",
		core_if->core_params->host_nperio_tx_fifo_size);
	return retval;
}

int32_t dwc_otg_get_param_host_nperio_tx_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->host_nperio_tx_fifo_size;
}

int dwc_otg_set_param_host_perio_tx_fifo_size(dwc_otg_core_if_t * core_if,
					      int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 16, 32768)) {
		DWC_WARN("Wrong value for host_perio_tx_fifo_size\n");
		DWC_WARN("host_perio_tx_fifo_size must be 16-32768\n");
		return -DWC_E_INVALID;
	}

	if (val >
	    ((DWC_READ_REG32(&core_if->core_global_regs->hptxfsiz) >> 16))) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->host_perio_tx_fifo_size)) {
			DWC_ERROR
			    ("%d invalid for host_perio_tx_fifo_size. Check HW configuration.\n",
			     val);
		}
		val =
		    (DWC_READ_REG32(&core_if->core_global_regs->hptxfsiz) >>
		     16);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->host_perio_tx_fifo_size = val;
	paramlog("host_perio_tx_fifo_size: %d\n",
		core_if->core_params->host_perio_tx_fifo_size);
	return retval;
}

int32_t dwc_otg_get_param_host_perio_tx_fifo_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->host_perio_tx_fifo_size;
}

int dwc_otg_set_param_max_transfer_size(dwc_otg_core_if_t * core_if,
					int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 2047, 524288)) {
		DWC_WARN("Wrong value for max_transfer_size\n");
		DWC_WARN("max_transfer_size must be 2047-524288\n");
		return -DWC_E_INVALID;
	}

	if (val >= (1 << (core_if->hwcfg3.b.xfer_size_cntr_width + 11))) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->max_transfer_size)) {
			DWC_ERROR
			    ("%d invalid for max_transfer_size. Check HW configuration.\n",
			     val);
		}
		val =
		    ((1 << (core_if->hwcfg3.b.packet_size_cntr_width + 11)) -
		     1);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->max_transfer_size = val;
	paramlog("max_transfer_size: %d\n",
		core_if->core_params->max_transfer_size);
	return retval;
}

int32_t dwc_otg_get_param_max_transfer_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->max_transfer_size;
}

int dwc_otg_set_param_max_packet_count(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 15, 511)) {
		DWC_WARN("Wrong value for max_packet_count\n");
		DWC_WARN("max_packet_count must be 15-511\n");
		return -DWC_E_INVALID;
	}

	if (val > (1 << (core_if->hwcfg3.b.packet_size_cntr_width + 4))) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->max_packet_count)) {
			DWC_ERROR
			    ("%d invalid for max_packet_count. Check HW configuration.\n",
			     val);
		}
		val =
		    ((1 << (core_if->hwcfg3.b.packet_size_cntr_width + 4)) - 1);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->max_packet_count = val;
	paramlog("max_packet_count: %d\n",
		core_if->core_params->max_packet_count);
	return retval;
}

int32_t dwc_otg_get_param_max_packet_count(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->max_packet_count;
}

int dwc_otg_set_param_host_channels(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 1, 16)) {
		DWC_WARN("Wrong value for host_channels\n");
		DWC_WARN("host_channels must be 1-16\n");
		return -DWC_E_INVALID;
	}

	if (val > (core_if->hwcfg2.b.num_host_chan + 1)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->host_channels)) {
			DWC_ERROR
			    ("%d invalid for host_channels. Check HW configurations.\n",
			     val);
		}
		val = (core_if->hwcfg2.b.num_host_chan + 1);
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->host_channels = val;
	paramlog("host_channels: %d\n", core_if->core_params->host_channels);
	return retval;
}

int32_t dwc_otg_get_param_host_channels(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->host_channels;
}

int dwc_otg_set_param_dev_endpoints(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 1, 15)) {
		DWC_WARN("Wrong value for dev_endpoints\n");
		DWC_WARN("dev_endpoints must be 1-15\n");
		return -DWC_E_INVALID;
	}

	if (val > (core_if->hwcfg2.b.num_dev_ep)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->dev_endpoints)) {
			DWC_ERROR
			    ("%d invalid for dev_endpoints. Check HW configurations.\n",
			     val);
		}
		val = core_if->hwcfg2.b.num_dev_ep;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->dev_endpoints = val;
	paramlog("dev_endpoints: %d\n", core_if->core_params->dev_endpoints);
	return retval;
}

int32_t dwc_otg_get_param_dev_endpoints(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->dev_endpoints;
}

int dwc_otg_set_param_phy_type(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	int valid = 0;

	if (DWC_OTG_PARAM_TEST(val, 0, 2)) {
		DWC_WARN("Wrong value for phy_type\n");
		DWC_WARN("phy_type must be 0,1 or 2\n");
		return -DWC_E_INVALID;
	}
#ifndef NO_FS_PHY_HW_CHECKS
	if ((val == DWC_PHY_TYPE_PARAM_UTMI) &&
	    ((core_if->hwcfg2.b.hs_phy_type == 1) ||
	     (core_if->hwcfg2.b.hs_phy_type == 3))) {
		valid = 1;
	} else if ((val == DWC_PHY_TYPE_PARAM_ULPI) &&
		   ((core_if->hwcfg2.b.hs_phy_type == 2) ||
		    (core_if->hwcfg2.b.hs_phy_type == 3))) {
		valid = 1;
	} else if ((val == DWC_PHY_TYPE_PARAM_FS) &&
		   (core_if->hwcfg2.b.fs_phy_type == 1)) {
		valid = 1;
	}
	if (!valid) {
		if (dwc_otg_param_initialized(core_if->core_params->phy_type)) {
			DWC_ERROR
			    ("%d invalid for phy_type. Check HW configurations.\n",
			     val);
		}
		if (core_if->hwcfg2.b.hs_phy_type) {
			if ((core_if->hwcfg2.b.hs_phy_type == 3) ||
			    (core_if->hwcfg2.b.hs_phy_type == 1)) {
				val = DWC_PHY_TYPE_PARAM_UTMI;
			} else {
				val = DWC_PHY_TYPE_PARAM_ULPI;
			}
		}
		retval = -DWC_E_INVALID;
	}
#endif
	core_if->core_params->phy_type = val;
	paramlog("phy_type: %d\n", core_if->core_params->phy_type);
	return retval;
}

int32_t dwc_otg_get_param_phy_type(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->phy_type;
}

int dwc_otg_set_param_speed(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for speed parameter\n");
		DWC_WARN("max_speed parameter must be 0 or 1\n");
		return -DWC_E_INVALID;
	}
	if ((val == 0)
	    && dwc_otg_get_param_phy_type(core_if) == DWC_PHY_TYPE_PARAM_FS) {
		if (dwc_otg_param_initialized(core_if->core_params->speed)) {
			DWC_ERROR
			    ("%d invalid for speed paremter. Check HW configuration.\n",
			     val);
		}
		val =
		    (dwc_otg_get_param_phy_type(core_if) ==
		     DWC_PHY_TYPE_PARAM_FS ? 1 : 0);
		retval = -DWC_E_INVALID;
	}
	core_if->core_params->speed = val;
	paramlog("speed: %d\n", core_if->core_params->speed);
	return retval;
}

int32_t dwc_otg_get_param_speed(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->speed;
}

int dwc_otg_set_param_host_ls_low_power_phy_clk(dwc_otg_core_if_t * core_if,
						int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN
		    ("Wrong value for host_ls_low_power_phy_clk parameter\n");
		DWC_WARN("host_ls_low_power_phy_clk must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if ((val == DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_48MHZ)
	    && (dwc_otg_get_param_phy_type(core_if) == DWC_PHY_TYPE_PARAM_FS)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->host_ls_low_power_phy_clk)) {
			DWC_ERROR
			    ("%d invalid for host_ls_low_power_phy_clk. Check HW configuration.\n",
			     val);
		}
		val =
		    (dwc_otg_get_param_phy_type(core_if) ==
		     DWC_PHY_TYPE_PARAM_FS) ?
		    DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_6MHZ :
		    DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_48MHZ;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->host_ls_low_power_phy_clk = val;
	paramlog("host_ls_low_power_phy_clk: %d\n",
		core_if->core_params->host_ls_low_power_phy_clk);
	return retval;
}

int32_t dwc_otg_get_param_host_ls_low_power_phy_clk(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->host_ls_low_power_phy_clk;
}

int dwc_otg_set_param_phy_ulpi_ddr(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for phy_ulpi_ddr\n");
		DWC_WARN("phy_upli_ddr must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params->phy_ulpi_ddr = val;
	paramlog("phy_ulpi_ddr: %d\n", core_if->core_params->phy_ulpi_ddr);
	return 0;
}

int32_t dwc_otg_get_param_phy_ulpi_ddr(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->phy_ulpi_ddr;
}

int dwc_otg_set_param_phy_ulpi_ext_vbus(dwc_otg_core_if_t * core_if,
					int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong valaue for phy_ulpi_ext_vbus\n");
		DWC_WARN("phy_ulpi_ext_vbus must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params->phy_ulpi_ext_vbus = val;
	paramlog("phy_ulpi_ext_vbus: %d\n",
		core_if->core_params->phy_ulpi_ext_vbus);
	return 0;
}

int32_t dwc_otg_get_param_phy_ulpi_ext_vbus(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->phy_ulpi_ext_vbus;
}

int dwc_otg_set_param_phy_utmi_width(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 8, 8) && DWC_OTG_PARAM_TEST(val, 16, 16)) {
		DWC_WARN("Wrong valaue for phy_utmi_width\n");
		DWC_WARN("phy_utmi_width must be 8 or 16\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params->phy_utmi_width = val;
	paramlog("phy_utmi_width: %d\n",
		core_if->core_params->phy_utmi_width);
	return 0;
}

int32_t dwc_otg_get_param_phy_utmi_width(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->phy_utmi_width;
}

int dwc_otg_set_param_ulpi_fs_ls(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong valaue for ulpi_fs_ls\n");
		DWC_WARN("ulpi_fs_ls must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params->ulpi_fs_ls = val;
	paramlog("ulpi_fs_ls: %d\n", core_if->core_params->ulpi_fs_ls);
	return 0;
}

int32_t dwc_otg_get_param_ulpi_fs_ls(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->ulpi_fs_ls;
}

int dwc_otg_set_param_ts_dline(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong valaue for ts_dline\n");
		DWC_WARN("ts_dline must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params->ts_dline = val;
	paramlog("ts_dline: %d\n", core_if->core_params->ts_dline);
	return 0;
}

int32_t dwc_otg_get_param_ts_dline(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->ts_dline;
}

int dwc_otg_set_param_i2c_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong valaue for i2c_enable\n");
		DWC_WARN("i2c_enable must be 0 or 1\n");
		return -DWC_E_INVALID;
	}
#ifndef NO_FS_PHY_HW_CHECK
	if (val == 1 && core_if->hwcfg3.b.i2c == 0) {
		if (dwc_otg_param_initialized(core_if->core_params->i2c_enable)) {
			DWC_ERROR
			    ("%d invalid for i2c_enable. Check HW configuration.\n",
			     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}
#endif

	core_if->core_params->i2c_enable = val;
	paramlog("i2c_enable: %d\n", core_if->core_params->i2c_enable);
	return retval;
}

int32_t dwc_otg_get_param_i2c_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->i2c_enable;
}

int dwc_otg_set_param_dev_perio_tx_fifo_size(dwc_otg_core_if_t * core_if,
					     int32_t val, int fifo_num)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 4, 768)) {
		DWC_WARN("Wrong value for dev_perio_tx_fifo_size\n");
		DWC_WARN("dev_perio_tx_fifo_size must be 4-768\n");
		return -DWC_E_INVALID;
	}

	if (val > (DWC_READ_REG32(&core_if->core_global_regs->dtxfsiz[fifo_num]))) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->dev_perio_tx_fifo_size[fifo_num])) {
			DWC_ERROR
			    ("`%d' invalid for parameter `dev_perio_fifo_size_%d'. Check HW configuration.\n",
			     val, fifo_num);
		}
		val = (DWC_READ_REG32(&core_if->core_global_regs->dtxfsiz[fifo_num]));
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->dev_perio_tx_fifo_size[fifo_num] = val;
	paramlog("dev_perio_tx_fifo_size[%d]: 0x%08x\n",
		fifo_num, core_if->core_params->dev_perio_tx_fifo_size[fifo_num]);
	return retval;
}

int32_t dwc_otg_get_param_dev_perio_tx_fifo_size(dwc_otg_core_if_t * core_if,
						 int fifo_num)
{
	return core_if->core_params->dev_perio_tx_fifo_size[fifo_num];
}

int dwc_otg_set_param_en_multiple_tx_fifo(dwc_otg_core_if_t * core_if,
					  int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong valaue for en_multiple_tx_fifo,\n");
		DWC_WARN("en_multiple_tx_fifo must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if (val == 1 && core_if->hwcfg4.b.ded_fifo_en == 0) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->en_multiple_tx_fifo)) {
			DWC_ERROR
			    ("%d invalid for parameter en_multiple_tx_fifo. Check HW configuration.\n",
			     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->en_multiple_tx_fifo = val;
	paramlog("en_multiple_tx_fifo: %d\n", core_if->core_params->en_multiple_tx_fifo);
	return retval;
}

int32_t dwc_otg_get_param_en_multiple_tx_fifo(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->en_multiple_tx_fifo;
}

int dwc_otg_set_param_dev_tx_fifo_size(dwc_otg_core_if_t * core_if, int32_t val,
				       int fifo_num)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 4, 768)) {
		DWC_WARN("Wrong value for dev_tx_fifo_size\n");
		DWC_WARN("dev_tx_fifo_size must be 4-768\n");
		return -DWC_E_INVALID;
	}

	if (val >
	    (DWC_READ_REG32(&core_if->core_global_regs->dtxfsiz[fifo_num]))) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->dev_tx_fifo_size[fifo_num])) {
			DWC_ERROR
			    ("`%d' invalid for parameter `dev_tx_fifo_size_%d'. Check HW configuration.\n",
			     val, fifo_num);
		}
		val = (DWC_READ_REG32(&core_if->core_global_regs->dtxfsiz[fifo_num]));
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->dev_tx_fifo_size[fifo_num] = val;
	paramlog("dev_tx_fifo_size[%d]: 0x%08x\n",
		fifo_num, core_if->core_params->dev_tx_fifo_size[fifo_num]);
	return retval;
}

int32_t dwc_otg_get_param_dev_tx_fifo_size(dwc_otg_core_if_t * core_if,
					   int fifo_num)
{
	return core_if->core_params->dev_tx_fifo_size[fifo_num];
}

int dwc_otg_set_param_thr_ctl(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 0, 7)) {
		DWC_WARN("Wrong value for thr_ctl\n");
		DWC_WARN("thr_ctl must be 0-7\n");
		return -DWC_E_INVALID;
	}

	if ((val != 0) &&
	    (!dwc_otg_get_param_dma_enable(core_if) ||
	     !core_if->hwcfg4.b.ded_fifo_en)) {
		if (dwc_otg_param_initialized(core_if->core_params->thr_ctl)) {
			DWC_ERROR
			    ("%d invalid for parameter thr_ctl. Check HW configuration.\n",
			     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->thr_ctl = val;
	paramlog("thr_ctl: %d\n", core_if->core_params->thr_ctl);
	return retval;
}

int32_t dwc_otg_get_param_thr_ctl(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->thr_ctl;
}

int dwc_otg_set_param_lpm_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("Wrong value for lpm_enable\n");
		DWC_WARN("lpm_enable must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if (val && !core_if->hwcfg3.b.otg_lpm_en) {
		if (dwc_otg_param_initialized(core_if->core_params->lpm_enable)) {
			DWC_ERROR
			    ("%d invalid for parameter lpm_enable. Check HW configuration.\n",
			     val);
		}
		val = 0;
		retval = -DWC_E_INVALID;
	}

	core_if->core_params->lpm_enable = val;
	paramlog("lpm_enable: %d\n", core_if->core_params->lpm_enable);
	return retval;
}

int32_t dwc_otg_get_param_lpm_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->lpm_enable;
}

int dwc_otg_set_param_tx_thr_length(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 8, 128)) {
		DWC_WARN("Wrong valaue for tx_thr_length\n");
		DWC_WARN("tx_thr_length must be 8 - 128\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params->tx_thr_length = val;
	paramlog("tx_thr_length: %d\n", core_if->core_params->tx_thr_length);
	return 0;
}

int32_t dwc_otg_get_param_tx_thr_length(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->tx_thr_length;
}

int dwc_otg_set_param_rx_thr_length(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 8, 128)) {
		DWC_WARN("Wrong valaue for rx_thr_length\n");
		DWC_WARN("rx_thr_length must be 8 - 128\n");
		return -DWC_E_INVALID;
	}

	core_if->core_params->rx_thr_length = val;
	paramlog("rx_thr_length: %d\n", core_if->core_params->rx_thr_length);
	return 0;
}

int32_t dwc_otg_get_param_rx_thr_length(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->rx_thr_length;
}

int dwc_otg_set_param_dma_burst_size(dwc_otg_core_if_t * core_if, int32_t val)
{
	if (DWC_OTG_PARAM_TEST(val, 1, 1) &&
	    DWC_OTG_PARAM_TEST(val, 4, 4) &&
	    DWC_OTG_PARAM_TEST(val, 8, 8) &&
	    DWC_OTG_PARAM_TEST(val, 16, 16) &&
	    DWC_OTG_PARAM_TEST(val, 32, 32) &&
	    DWC_OTG_PARAM_TEST(val, 64, 64) &&
	    DWC_OTG_PARAM_TEST(val, 128, 128) &&
	    DWC_OTG_PARAM_TEST(val, 256, 256)) {
		DWC_WARN("`%d' invalid for parameter `dma_burst_size'\n", val);
		return -DWC_E_INVALID;
	}
	core_if->core_params->dma_burst_size = val;
	paramlog("dma_burst_size: %d\n", core_if->core_params->dma_burst_size);
	return 0;
}

int32_t dwc_otg_get_param_dma_burst_size(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->dma_burst_size;
}

int dwc_otg_set_param_pti_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("`%d' invalid for parameter `pti_enable'\n", val);
		return -DWC_E_INVALID;
	}
	if (val && (core_if->snpsid < OTG_CORE_REV_2_72a)) {
		if (dwc_otg_param_initialized(core_if->core_params->pti_enable)) {
			DWC_ERROR
			    ("%d invalid for parameter pti_enable. Check HW configuration.\n",
			     val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}
	core_if->core_params->pti_enable = val;
	paramlog("pti_enable: %d\n", core_if->core_params->pti_enable);
	return retval;
}

int32_t dwc_otg_get_param_pti_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->pti_enable;
}

int dwc_otg_set_param_mpi_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("`%d' invalid for parameter `mpi_enable'\n", val);
		return -DWC_E_INVALID;
	}
	if (val && (core_if->hwcfg2.b.multi_proc_int == 0)) {
		if (dwc_otg_param_initialized(core_if->core_params->mpi_enable)) {
			DWC_ERROR
			    ("%d invalid for parameter mpi_enable. Check HW configuration.\n",
			     val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}
	core_if->core_params->mpi_enable = val;
	paramlog("mpi_enable: %d\n", core_if->core_params->mpi_enable);
	return retval;
}

int32_t dwc_otg_get_param_mpi_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->mpi_enable;
}

int dwc_otg_set_param_adp_enable(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("`%d' invalid for parameter `adp_enable'\n", val);
		return -DWC_E_INVALID;
	}
	if (val && (core_if->hwcfg3.b.adp_supp == 0)) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->adp_supp_enable)) {
			DWC_ERROR
			    ("%d invalid for parameter adp_enable. Check HW configuration.\n",
			     val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}
	core_if->core_params->adp_supp_enable = val;
	paramlog("adp_supp_enable: %d\n", core_if->core_params->adp_supp_enable);
	return retval;
}

int32_t dwc_otg_get_param_adp_enable(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->adp_supp_enable;
}

int dwc_otg_set_param_ic_usb_cap(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("`%d' invalid for parameter `ic_usb_cap'\n", val);
		DWC_WARN("ic_usb_cap must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if (val && (core_if->hwcfg2.b.otg_enable_ic_usb == 0)) {
		if (dwc_otg_param_initialized(core_if->core_params->ic_usb_cap)) {
			DWC_ERROR
			    ("%d invalid for parameter ic_usb_cap. Check HW configuration.\n",
			     val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}
	core_if->core_params->ic_usb_cap = val;
	paramlog("ic_usb_cap: %d\n", core_if->core_params->ic_usb_cap);
	return retval;
}

int32_t dwc_otg_get_param_ic_usb_cap(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->ic_usb_cap;
}

int dwc_otg_set_param_ahb_thr_ratio(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	int valid = 1;

	if (DWC_OTG_PARAM_TEST(val, 0, 3)) {
		DWC_WARN("`%d' invalid for parameter `ahb_thr_ratio'\n", val);
		DWC_WARN("ahb_thr_ratio must be 0 - 3\n");
		return -DWC_E_INVALID;
	}

	if (val
	    && (core_if->snpsid < OTG_CORE_REV_2_81a
		|| !dwc_otg_get_param_thr_ctl(core_if))) {
		valid = 0;
	} else if (val
		   && ((dwc_otg_get_param_tx_thr_length(core_if) / (1 << val)) <
		       4)) {
		valid = 0;
	}
	if (valid == 0) {
		if (dwc_otg_param_initialized
		    (core_if->core_params->ahb_thr_ratio)) {
			DWC_ERROR
			    ("%d invalid for parameter ahb_thr_ratio. Check HW configuration.\n",
			     val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}

	core_if->core_params->ahb_thr_ratio = val;
	paramlog("ahb_thr_ratio: %d\n", core_if->core_params->ahb_thr_ratio);
	return retval;
}

int32_t dwc_otg_get_param_ahb_thr_ratio(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->ahb_thr_ratio;
}

int dwc_otg_set_param_power_down(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	int valid = 1;

	if (DWC_OTG_PARAM_TEST(val, 0, 2)) {
		DWC_WARN("`%d' invalid for parameter `power_down'\n", val);
		DWC_WARN("power_down must be 0 - 2\n");
		return -DWC_E_INVALID;
	}

	if ((val == 2) && (core_if->snpsid < OTG_CORE_REV_2_91a)) {
		valid = 0;
	}
	if (valid == 0) {
		if (dwc_otg_param_initialized(core_if->core_params->power_down)) {
			DWC_ERROR
			    ("%d invalid for parameter power_down. Check HW configuration.\n",
			     val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}
	core_if->core_params->power_down = val;
	paramlog("power_down: %d\n", core_if->core_params->power_down);
	return retval;
}

int32_t dwc_otg_get_param_power_down(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->power_down;
}

int dwc_otg_set_param_reload_ctl(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;
	int valid = 1;

	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("`%d' invalid for parameter `reload_ctl'\n", val);
		DWC_WARN("reload_ctl must be 0 or 1\n");
		return -DWC_E_INVALID;
	}

	if ((val == 1) && (core_if->snpsid < OTG_CORE_REV_2_92a)) {
		valid = 0;
	}
	if (valid == 0) {
		if (dwc_otg_param_initialized(core_if->core_params->reload_ctl)) {
			DWC_ERROR("%d invalid for parameter reload_ctl."
				  "Check HW configuration.\n", val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}
	core_if->core_params->reload_ctl = val;
	paramlog("reload_ctl: %d\n", core_if->core_params->reload_ctl);
	return retval;
}

int32_t dwc_otg_get_param_reload_ctl(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->reload_ctl;
}

int dwc_otg_set_param_otg_ver(dwc_otg_core_if_t * core_if, int32_t val)
{
	int retval = 0;

	if (DWC_OTG_PARAM_TEST(val, 0, 1)) {
		DWC_WARN("`%d' invalid for parameter `otg_ver'\n", val);
		DWC_WARN
		    ("otg_ver must be 0(for OTG 1.3 support) or 1(for OTG 2.0 support)\n");
		return -DWC_E_INVALID;
	}

	if (val && (core_if->hwcfg3.b.otg_ver_support == 0)) {
		if (dwc_otg_param_initialized(core_if->core_params->otg_ver)) {
			DWC_ERROR
			    ("%d invalid for parameter otg_ver. Check HW configuration.\n",
			     val);
		}
		retval = -DWC_E_INVALID;
		val = 0;
	}
	core_if->core_params->otg_ver = val;
	paramlog("otg_ver: %d\n", core_if->core_params->otg_ver);
	return retval;
}

int32_t dwc_otg_get_param_otg_ver(dwc_otg_core_if_t * core_if)
{
	return core_if->core_params->otg_ver;
}

int dwc_otg_setup_params(dwc_otg_core_if_t * core_if)
{
	int i;

	printk("[%s]+\n", __func__);

	core_if->core_params = DWC_ALLOC(sizeof(*core_if->core_params));
	if (!core_if->core_params) {
		return -DWC_E_NO_MEMORY;
	}
	dwc_otg_set_uninitialized((int32_t *) core_if->core_params,
				  sizeof(*core_if->core_params) /
				  sizeof(int32_t));

	//dwc_otg_set_param_otg_cap(core_if, dwc_param_otg_cap_default);
	dwc_otg_set_param_otg_cap(core_if, DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE);

	dwc_otg_set_param_dma_enable(core_if, dwc_param_dma_enable_default);
	//dwc_otg_set_param_dma_enable(core_if, DWC_PARAM_DMA_DISABLE);

	//dwc_otg_set_param_dma_desc_enable(core_if, dwc_param_dma_desc_enable_default);
	dwc_otg_set_param_dma_desc_enable(core_if, DWC_PARAM_DMA_DESC_DISABLE);

	dwc_otg_set_param_opt(core_if, dwc_param_opt_default);

	// inter DMA did not use this parametre
	dwc_otg_set_param_dma_burst_size(core_if, dwc_param_dma_burst_size_default);
	dwc_otg_set_param_host_support_fs_ls_low_power(core_if, dwc_param_host_support_fs_ls_low_power_default);
	dwc_otg_set_param_enable_dynamic_fifo(core_if, dwc_param_enable_dynamic_fifo_default);
	dwc_otg_set_param_data_fifo_size(core_if, dwc_param_data_fifo_size_default);
	dwc_otg_set_param_dev_rx_fifo_size(core_if, dwc_param_dev_rx_fifo_size_default);
	dwc_otg_set_param_dev_nperio_tx_fifo_size(core_if, dwc_param_dev_nperio_tx_fifo_size_default);
	dwc_otg_set_param_host_rx_fifo_size(core_if, dwc_param_host_rx_fifo_size_default);
	dwc_otg_set_param_host_nperio_tx_fifo_size(core_if, dwc_param_host_nperio_tx_fifo_size_default);
	dwc_otg_set_param_host_perio_tx_fifo_size(core_if, dwc_param_host_perio_tx_fifo_size_default);
	dwc_otg_set_param_max_transfer_size(core_if, dwc_param_max_transfer_size_default);
	dwc_otg_set_param_max_packet_count(core_if, dwc_param_max_packet_count_default);

	//dwc_otg_set_param_host_channels(core_if, dwc_param_host_channels_default);
	dwc_otg_set_param_host_channels(core_if, 16);

	dwc_otg_set_param_dev_endpoints(core_if, dwc_param_dev_endpoints_default);
	dwc_otg_set_param_phy_type(core_if, dwc_param_phy_type_default);

	dwc_otg_set_param_speed(core_if, dwc_param_speed_default);
	//dwc_otg_set_param_speed(core_if, DWC_SPEED_PARAM_FULL);

	dwc_otg_set_param_host_ls_low_power_phy_clk(core_if, dwc_param_host_ls_low_power_phy_clk_default);
	dwc_otg_set_param_phy_ulpi_ddr(core_if, dwc_param_phy_ulpi_ddr_default);
	dwc_otg_set_param_phy_ulpi_ext_vbus(core_if, dwc_param_phy_ulpi_ext_vbus_default);
	dwc_otg_set_param_phy_utmi_width(core_if, dwc_param_phy_utmi_width_default);
	dwc_otg_set_param_ts_dline(core_if, dwc_param_ts_dline_default);
	dwc_otg_set_param_i2c_enable(core_if, dwc_param_i2c_enable_default);
	dwc_otg_set_param_ulpi_fs_ls(core_if, dwc_param_ulpi_fs_ls_default);
	dwc_otg_set_param_en_multiple_tx_fifo(core_if, dwc_param_en_multiple_tx_fifo_default);

	for (i = 0; i < 15; i++) {
		dwc_otg_set_param_dev_perio_tx_fifo_size(core_if,
			dwc_param_dev_perio_tx_fifo_size_default, i);
	}

#if 0
	for (i = 0; i < 15; i++) {
		dwc_otg_set_param_dev_tx_fifo_size(core_if,
			dwc_param_dev_tx_fifo_size_default, i);
	}
#else
	//FIXME:a fixed fifo cfg. Can modify for different application.
	dwc_otg_set_param_dev_tx_fifo_size(core_if, dwc_param_dev_tx_fifo_size_default, 0);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, dwc_param_dev_tx_fifo_size_default, 1);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, dwc_param_dev_tx_fifo_size_default, 2);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, dwc_param_dev_tx_fifo_size_default, 3);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, 0x20, 4);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, 0x20, 5);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, 0x20, 6);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, 0x20, 7);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, 0x20, 8);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, 0x20, 9);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, 0x20, 10);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, 0x20, 11);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, 0x20, 12);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, 0x10, 13);
	dwc_otg_set_param_dev_tx_fifo_size(core_if, 0x10, 14);
#endif

	dwc_otg_set_param_thr_ctl(core_if, dwc_param_thr_ctl_default);
	dwc_otg_set_param_mpi_enable(core_if, dwc_param_mpi_enable_default);
	dwc_otg_set_param_pti_enable(core_if, dwc_param_pti_enable_default);
	dwc_otg_set_param_lpm_enable(core_if, dwc_param_lpm_enable_default);
	dwc_otg_set_param_ic_usb_cap(core_if, dwc_param_ic_usb_cap_default);
	dwc_otg_set_param_tx_thr_length(core_if, dwc_param_tx_thr_length_default);
	dwc_otg_set_param_rx_thr_length(core_if, dwc_param_rx_thr_length_default);
	dwc_otg_set_param_ahb_thr_ratio(core_if, dwc_param_ahb_thr_ratio_default);
	dwc_otg_set_param_power_down(core_if, dwc_param_power_down_default);
	dwc_otg_set_param_reload_ctl(core_if, dwc_param_reload_ctl_default);
	dwc_otg_set_param_otg_ver(core_if, dwc_param_otg_ver_default);
	dwc_otg_set_param_adp_enable(core_if, dwc_param_adp_enable_default);

	printk("[%s]-\n", __func__);
	return 0;
}


