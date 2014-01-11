#ifndef __DWC_OTG_CORE_PARAM_H__
#define __DWC_OTG_CORE_PARAM_H__
/** @name OTG Core Parameters */
/** @{ */

/**
 * Specifies the OTG capabilities. The driver will automatically
 * detect the value for this parameter if none is specified.
 * 0 - HNP and SRP capable (default)
 * 1 - SRP Only capable
 * 2 - No HNP/SRP capable
 */
extern int dwc_otg_set_param_otg_cap(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_otg_cap(dwc_otg_core_if_t * core_if);
#define DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE 0
#define DWC_OTG_CAP_PARAM_SRP_ONLY_CAPABLE 1
#define DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE 2
#define dwc_param_otg_cap_default DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE

extern int dwc_otg_set_param_opt(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_opt(dwc_otg_core_if_t * core_if);
#define dwc_param_opt_default 1

/**
 * Specifies whether to use slave or DMA mode for accessing the data
 * FIFOs. The driver will automatically detect the value for this
 * parameter if none is specified.
 * 0 - Slave
 * 1 - DMA (default, if available)
 */
extern int dwc_otg_set_param_dma_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_dma_enable(dwc_otg_core_if_t * core_if);
#define DWC_PARAM_DMA_ENABLE 1
#define DWC_PARAM_DMA_DISABLE 0
#define dwc_param_dma_enable_default 1

/**
 * When DMA mode is enabled specifies whether to use
 * address DMA or DMA Descritor mode for accessing the data
 * FIFOs in device mode. The driver will automatically detect
 * the value for this parameter if none is specified.
 * 0 - address DMA
 * 1 - DMA Descriptor(default, if available)
 */
extern int dwc_otg_set_param_dma_desc_enable(dwc_otg_core_if_t * core_if,
					     int32_t val);
extern int32_t dwc_otg_get_param_dma_desc_enable(dwc_otg_core_if_t * core_if);

/*
 * m53980: bug with s/g dma xfer. When intrrupt (xfercmpl) occurs, dma desc
 * shows there are remain bytes. So disable it and enable buffer dma xfer
 */
#define DWC_PARAM_DMA_DESC_ENABLE 1
#define DWC_PARAM_DMA_DESC_DISABLE 0
#define dwc_param_dma_desc_enable_default DWC_PARAM_DMA_DESC_ENABLE

/** The DMA Burst size (applicable only for External DMA
 * Mode). 1, 4, 8 16, 32, 64, 128, 256 (default 32)
 */
extern int dwc_otg_set_param_dma_burst_size(dwc_otg_core_if_t * core_if,
					    int32_t val);
extern int32_t dwc_otg_get_param_dma_burst_size(dwc_otg_core_if_t * core_if);
#define dwc_param_dma_burst_size_default 32

/**
 * Specifies the maximum speed of operation in host and device mode.
 * The actual speed depends on the speed of the attached device and
 * the value of phy_type. The actual speed depends on the speed of the
 * attached device.
 * 0 - High Speed (default)
 * 1 - Full Speed
 */
extern int dwc_otg_set_param_speed(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_speed(dwc_otg_core_if_t * core_if);
#define dwc_param_speed_default 0
#define DWC_SPEED_PARAM_HIGH 0
#define DWC_SPEED_PARAM_FULL 1

/** Specifies whether low power mode is supported when attached
 *	to a Full Speed or Low Speed device in host mode.
 * 0 - Don't support low power mode (default)
 * 1 - Support low power mode
 */
extern int dwc_otg_set_param_host_support_fs_ls_low_power(dwc_otg_core_if_t *
							  core_if, int32_t val);
extern int32_t dwc_otg_get_param_host_support_fs_ls_low_power(dwc_otg_core_if_t
							      * core_if);
#define dwc_param_host_support_fs_ls_low_power_default 0

/** Specifies the PHY clock rate in low power mode when connected to a
 * Low Speed device in host mode. This parameter is applicable only if
 * HOST_SUPPORT_FS_LS_LOW_POWER is enabled. If PHY_TYPE is set to FS
 * then defaults to 6 MHZ otherwise 48 MHZ.
 *
 * 0 - 48 MHz
 * 1 - 6 MHz
 */
extern int dwc_otg_set_param_host_ls_low_power_phy_clk(dwc_otg_core_if_t *
						       core_if, int32_t val);
extern int32_t dwc_otg_get_param_host_ls_low_power_phy_clk(dwc_otg_core_if_t *
							   core_if);
#define dwc_param_host_ls_low_power_phy_clk_default 0
#define DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_48MHZ 0
#define DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_6MHZ 1

/**
 * 0 - Use cC FIFO size parameters
 * 1 - Allow dynamic FIFO sizing (default)
 */
extern int dwc_otg_set_param_enable_dynamic_fifo(dwc_otg_core_if_t * core_if,
						 int32_t val);
extern int32_t dwc_otg_get_param_enable_dynamic_fifo(dwc_otg_core_if_t *
						     core_if);
#define dwc_param_enable_dynamic_fifo_default 1

/** Total number of 4-byte words in the data FIFO memory. This
 * memory includes the Rx FIFO, non-periodic Tx FIFO, and periodic
 * Tx FIFOs.
 * 32 to 32768 (default 8192)
 * Note: The total FIFO memory depth in the FPGA configuration is 8192.
 */
extern int dwc_otg_set_param_data_fifo_size(dwc_otg_core_if_t * core_if,
					    int32_t val);
extern int32_t dwc_otg_get_param_data_fifo_size(dwc_otg_core_if_t * core_if);
#define dwc_param_data_fifo_size_default 8192

/** Number of 4-byte words in the Rx FIFO in device mode when dynamic
 * FIFO sizing is enabled.
 * 16 to 32768 (default 1064)
 */
extern int dwc_otg_set_param_dev_rx_fifo_size(dwc_otg_core_if_t * core_if,
					      int32_t val);
extern int32_t dwc_otg_get_param_dev_rx_fifo_size(dwc_otg_core_if_t * core_if);
#define dwc_param_dev_rx_fifo_size_default 1064

/** Number of 4-byte words in the non-periodic Tx FIFO in device mode
 * when dynamic FIFO sizing is enabled.
 * 16 to 32768 (default 1024)
 */
extern int dwc_otg_set_param_dev_nperio_tx_fifo_size(dwc_otg_core_if_t *
						     core_if, int32_t val);
extern int32_t dwc_otg_get_param_dev_nperio_tx_fifo_size(dwc_otg_core_if_t *
							 core_if);
#define dwc_param_dev_nperio_tx_fifo_size_default 1024

/** Number of 4-byte words in each of the periodic Tx FIFOs in device
 * mode when dynamic FIFO sizing is enabled.
 * 4 to 768 (default 256)
 */
extern int dwc_otg_set_param_dev_perio_tx_fifo_size(dwc_otg_core_if_t * core_if,
						    int32_t val, int fifo_num);
extern int32_t dwc_otg_get_param_dev_perio_tx_fifo_size(dwc_otg_core_if_t *
							core_if, int fifo_num);
#define dwc_param_dev_perio_tx_fifo_size_default 256

/** Number of 4-byte words in the Rx FIFO in host mode when dynamic
 * FIFO sizing is enabled.
 * 16 to 32768 (default 1024)
 */
extern int dwc_otg_set_param_host_rx_fifo_size(dwc_otg_core_if_t * core_if,
					       int32_t val);
extern int32_t dwc_otg_get_param_host_rx_fifo_size(dwc_otg_core_if_t * core_if);
#define dwc_param_host_rx_fifo_size_default 1024

/** Number of 4-byte words in the non-periodic Tx FIFO in host mode
 * when Dynamic FIFO sizing is enabled in the core.
 * 16 to 32768 (default 1024)
 */
extern int dwc_otg_set_param_host_nperio_tx_fifo_size(dwc_otg_core_if_t *
						      core_if, int32_t val);
extern int32_t dwc_otg_get_param_host_nperio_tx_fifo_size(dwc_otg_core_if_t *
							  core_if);
#define dwc_param_host_nperio_tx_fifo_size_default 1024

/** Number of 4-byte words in the host periodic Tx FIFO when dynamic
 * FIFO sizing is enabled.
 * 16 to 32768 (default 1024)
 */
extern int dwc_otg_set_param_host_perio_tx_fifo_size(dwc_otg_core_if_t *
						     core_if, int32_t val);
extern int32_t dwc_otg_get_param_host_perio_tx_fifo_size(dwc_otg_core_if_t *
							 core_if);
#define dwc_param_host_perio_tx_fifo_size_default 1024

/** The maximum transfer size supported in bytes.
 * 2047 to 65,535  (default 65,535)
 */
extern int dwc_otg_set_param_max_transfer_size(dwc_otg_core_if_t * core_if,
					       int32_t val);
extern int32_t dwc_otg_get_param_max_transfer_size(dwc_otg_core_if_t * core_if);
#define dwc_param_max_transfer_size_default 65535

/** The maximum number of packets in a transfer.
 * 15 to 511  (default 511)
 */
extern int dwc_otg_set_param_max_packet_count(dwc_otg_core_if_t * core_if,
					      int32_t val);
extern int32_t dwc_otg_get_param_max_packet_count(dwc_otg_core_if_t * core_if);
#define dwc_param_max_packet_count_default 511

/** The number of host channel registers to use.
 * 1 to 16 (default 12)
 * Note: The FPGA configuration supports a maximum of 12 host channels.
 */
extern int dwc_otg_set_param_host_channels(dwc_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t dwc_otg_get_param_host_channels(dwc_otg_core_if_t * core_if);
#define dwc_param_host_channels_default 12

/** The number of endpoints in addition to EP0 available for device
 * mode operations.
 * 1 to 15 (default 6 IN and OUT)
 * Note: The FPGA configuration supports a maximum of 6 IN and OUT
 * endpoints in addition to EP0.
 */
extern int dwc_otg_set_param_dev_endpoints(dwc_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t dwc_otg_get_param_dev_endpoints(dwc_otg_core_if_t * core_if);
#define dwc_param_dev_endpoints_default 6

/**
 * Specifies the type of PHY interface to use. By default, the driver
 * will automatically detect the phy_type.
 *
 * 0 - Full Speed PHY
 * 1 - UTMI+ (default)
 * 2 - ULPI
 */
extern int dwc_otg_set_param_phy_type(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_phy_type(dwc_otg_core_if_t * core_if);
#define DWC_PHY_TYPE_PARAM_FS 0
#define DWC_PHY_TYPE_PARAM_UTMI 1
#define DWC_PHY_TYPE_PARAM_ULPI 2
#define dwc_param_phy_type_default DWC_PHY_TYPE_PARAM_UTMI

/**
 * Specifies the UTMI+ Data Width. This parameter is
 * applicable for a PHY_TYPE of UTMI+ or ULPI. (For a ULPI
 * PHY_TYPE, this parameter indicates the data width between
 * the MAC and the ULPI Wrapper.) Also, this parameter is
 * applicable only if the OTG_HSPHY_WIDTH cC parameter was set
 * to "8 and 16 bits", meaning that the core has been
 * configured to work at either data path width.
 *
 * 8 or 16 bits (default 16)
 */
extern int dwc_otg_set_param_phy_utmi_width(dwc_otg_core_if_t * core_if,
					    int32_t val);
extern int32_t dwc_otg_get_param_phy_utmi_width(dwc_otg_core_if_t * core_if);
#define dwc_param_phy_utmi_width_default 8

/**
 * Specifies whether the ULPI operates at double or single
 * data rate. This parameter is only applicable if PHY_TYPE is
 * ULPI.
 *
 * 0 - single data rate ULPI interface with 8 bit wide data
 * bus (default)
 * 1 - double data rate ULPI interface with 4 bit wide data
 * bus
 */
extern int dwc_otg_set_param_phy_ulpi_ddr(dwc_otg_core_if_t * core_if,
					  int32_t val);
extern int32_t dwc_otg_get_param_phy_ulpi_ddr(dwc_otg_core_if_t * core_if);
#define dwc_param_phy_ulpi_ddr_default 0

/**
 * Specifies whether to use the internal or external supply to
 * drive the vbus with a ULPI phy.
 */
extern int dwc_otg_set_param_phy_ulpi_ext_vbus(dwc_otg_core_if_t * core_if,
					       int32_t val);
extern int32_t dwc_otg_get_param_phy_ulpi_ext_vbus(dwc_otg_core_if_t * core_if);
#define DWC_PHY_ULPI_INTERNAL_VBUS 0
#define DWC_PHY_ULPI_EXTERNAL_VBUS 1
#define dwc_param_phy_ulpi_ext_vbus_default DWC_PHY_ULPI_INTERNAL_VBUS

/**
 * Specifies whether to use the I2Cinterface for full speed PHY. This
 * parameter is only applicable if PHY_TYPE is FS.
 * 0 - No (default)
 * 1 - Yes
 */
extern int dwc_otg_set_param_i2c_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_i2c_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_i2c_enable_default 0

extern int dwc_otg_set_param_ulpi_fs_ls(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_ulpi_fs_ls(dwc_otg_core_if_t * core_if);
#define dwc_param_ulpi_fs_ls_default 0

extern int dwc_otg_set_param_ts_dline(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_ts_dline(dwc_otg_core_if_t * core_if);
#define dwc_param_ts_dline_default 0

/**
 * Specifies whether dedicated transmit FIFOs are
 * enabled for non periodic IN endpoints in device mode
 * 0 - No
 * 1 - Yes
 */
extern int dwc_otg_set_param_en_multiple_tx_fifo(dwc_otg_core_if_t * core_if,
						 int32_t val);
extern int32_t dwc_otg_get_param_en_multiple_tx_fifo(dwc_otg_core_if_t *
						     core_if);
#define dwc_param_en_multiple_tx_fifo_default 1

/** Number of 4-byte words in each of the Tx FIFOs in device
 * mode when dynamic FIFO sizing is enabled.
 * 4 to 768 (default 256)
 */
extern int dwc_otg_set_param_dev_tx_fifo_size(dwc_otg_core_if_t * core_if,
					      int fifo_num, int32_t val);
extern int32_t dwc_otg_get_param_dev_tx_fifo_size(dwc_otg_core_if_t * core_if,
						  int fifo_num);
#define dwc_param_dev_tx_fifo_size_default 256

/** Thresholding enable flag-
 * bit 0 - enable non-ISO Tx thresholding
 * bit 1 - enable ISO Tx thresholding
 * bit 2 - enable Rx thresholding
 */
extern int dwc_otg_set_param_thr_ctl(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_thr_ctl(dwc_otg_core_if_t * core_if, int fifo_num);
#define dwc_param_thr_ctl_default 0

/** Thresholding length for Tx
 * FIFOs in 32 bit DWORDs
 */
extern int dwc_otg_set_param_tx_thr_length(dwc_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t dwc_otg_get_tx_thr_length(dwc_otg_core_if_t * core_if);
#define dwc_param_tx_thr_length_default 64

/** Thresholding length for Rx
 *	FIFOs in 32 bit DWORDs
 */
extern int dwc_otg_set_param_rx_thr_length(dwc_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t dwc_otg_get_rx_thr_length(dwc_otg_core_if_t * core_if);
#define dwc_param_rx_thr_length_default 64

/**
 * Specifies whether LPM (Link Power Management) support is enabled
 */
extern int dwc_otg_set_param_lpm_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_lpm_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_lpm_enable_default 1

/**
 * Specifies whether PTI enhancement is enabled
 */
extern int dwc_otg_set_param_pti_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_pti_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_pti_enable_default 0

/**
 * Specifies whether MPI enhancement is enabled
 */
extern int dwc_otg_set_param_mpi_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_mpi_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_mpi_enable_default 0

/**
 * Specifies whether ADP capability is enabled
 */
extern int dwc_otg_set_param_adp_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_adp_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_adp_enable_default 0

/**
 * Specifies whether IC_USB capability is enabled
 */

extern int dwc_otg_set_param_ic_usb_cap(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_ic_usb_cap(dwc_otg_core_if_t * core_if);
#define dwc_param_ic_usb_cap_default 0

extern int dwc_otg_set_param_ahb_thr_ratio(dwc_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t dwc_otg_get_param_ahb_thr_ratio(dwc_otg_core_if_t * core_if);
#define dwc_param_ahb_thr_ratio_default 0

extern int dwc_otg_set_param_power_down(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_power_down(dwc_otg_core_if_t * core_if);
#define dwc_param_power_down_default 0

extern int dwc_otg_set_param_reload_ctl(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_reload_ctl(dwc_otg_core_if_t * core_if);
#define dwc_param_reload_ctl_default 0

extern int dwc_otg_set_param_otg_ver(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_otg_ver(dwc_otg_core_if_t * core_if);
/*
 * OTG Version (OTGVer)
 * Indicates the OTG revision.
 * 1'0: OTG Version 1.3. In this version the core supports Data line
 * pulsing and VBus pulsing for SRP.
 * 1'b1: OTG Version 2.0. In this version the core supports only Data
 * line pulsing for SRP.
 */
#define dwc_param_otg_ver_default 1


int dwc_otg_setup_params(dwc_otg_core_if_t * core_if);



#endif
