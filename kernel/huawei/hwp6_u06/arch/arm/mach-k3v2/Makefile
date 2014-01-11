#
# Makefile for the linux kernel.
#

obj-y				:= common.o lm.o k3v2_clocks_ops.o k3v2_clocks_cs.o k3v2_clocks_es.o k3v2_clocks_init_data_es.o k3v2_clocks_init_data_cs.o
obj-y				+= timer.o io.o irq.o clock.o pmem.o k3v2_irq_affinity.o
obj-y                           += k3v2_regulators_vcc.o regulators_vcc.o
obj-y                           += k3v2_iomux.o iomux.o
obj-y				+= product_feature_sel.o
obj-$(CONFIG_PCI)		+= pcie.o 
obj-$(CONFIG_ARCH_PROVIDES_UDELAY) += delay.o
obj-$(CONFIG_MACH_TC45MSU3)	+= board-tc45msu3.o
obj-$(CONFIG_MACH_K3V2OEM1)	+= board-k3v2oem1.o
obj-$(CONFIG_MACH_K3V2OEM2)	+= board-k3v2oem2.o
obj-$(CONFIG_SMP)		+= platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)	+= hotplug.o
obj-$(CONFIG_LOCAL_TIMERS)	+= localtimer.o
obj-$(CONFIG_K3V2_DMA)          += dma.o
obj-$(CONFIG_PM)		+= hilpm-cpugodp.o misc_assembly.o pm.o
obj-$(CONFIG_USB_SUPPORT)	+= usb_phy.o
obj-$(CONFIG_CPU_FREQ)		+= cpu-k3v2.o
obj-$(CONFIG_IPPS_SUPPORT)	+= ipps-core.o ipps2.o
obj-$(CONFIG_EXTRAL_DYNAMIC_DCDC) += k3v2_dcdc_gpu_profile.o k3v2_dcdc_gpu.o
obj-$(CONFIG_PM)		+= lowpmregs.o
obj-$(CONFIG_XMM_POWER)         += xmm_power.o
obj-$(CONFIG_WIFI_DEVICE)	+= k3v2_wifi_power.o
obj-$(CONFIG_XMM_RMNET)         += xmd_rmnet.o
obj-$(CONFIG_DEBUG_LED)		+= debugled.o
obj-$(CONFIG_CPU_IDLE)         += k3v2_cpuidle.o k3v2_cpu_enteridle.o
obj-$(CONFIG_K3V2_WAKEUP_TIMER)     += k3v2_wakeup_timer.o

obj-$(CONFIG_BALONG_POWER)         += balong_power.o
obj-$(CONFIG_BALONG_RMNET)         += balong_rmnet.o
obj-$(CONFIG_MODEM_BOOT_MTK6252)         += modem_boot_mtk6252.o
obj-$(CONFIG_MODEM_BOOT_QSC6085)         += modem_boot_qsc6085.o
obj-$(CONFIG_MODEM_BOOT_SPRD8803G)         += modem_boot_sprd8803g.o sprd_transfer.o sprd_spi_drv.o sprd_ts0710_mux.o sprd_mux_buffer.o sprd_rmnet.o
obj-$(CONFIG_SUPPORT_B3750000_BITRATE) += k3v2_clocks_cs_60M.o k3v2_clocks_init_data_cs_60M.o

clean-files :=  k3v2_iomux_blocks.h
$(obj)/iomux.o $(MODVERDIR)/$(obj)/iomux.ver: $(obj)/k3v2_iomux_blocks.h
$(obj)/k3v2_iomux_blocks.h: $(src)/gen_io_cfg.pl $(src)/iomux.xls
	$(PERL) -s $^ $@

ifdef CONFIG_IPPS_PARAM_ALTERNATIVE
# Files generated that shall be removed upon make clean
clean-files :=	ipps_para.h

$(obj)/ipps2.o $(MODVERDIR)/$(obj)/ipps2.ver: $(obj)/ipps_para.h

ifdef CONFIG_FHD_LCD_PANEL
$(obj)/ipps_para.h: $(src)/ipps_para_gen.pl $(src)/ipps_para_FHD.xls
	$(PERL) -s $^ $@
else
$(obj)/ipps_para.h: $(src)/ipps_para_gen.pl $(src)/ipps_para.xls
	$(PERL) -s $^ $@
endif

endif
