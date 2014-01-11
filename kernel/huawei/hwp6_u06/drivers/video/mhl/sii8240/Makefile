#
# SiI8240 Linux Driver
# 
# Copyright (C) 2011-2012 Silicon Image Inc.
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation version 2.
# 
# This program is distributed .as is. WITHOUT ANY WARRANTY of any
# kind, whether express or implied; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.  See the
# GNU General Public License for more details.
# 

#
# Makefile for the Silicon Image 8240 MHL TX device driver
#
# example invocations:	
#	For regular Linux builds
#	make ARCH=arm CROSS_COMPILE=arm-angstrom-linux-gnueabi- sii8240_clean sii8240_debug
#	make ARCH=arm CROSS_COMPILE=arm-angstrom-linux-gnueabi- sii8240_clean sii8240_release
#	make ARCH=arm CROSS_COMPILE=arm-angstrom-linux-gnueabi- sii8240_clean sii8240_clean
#
#	For Android driver builds - Specify different tool-chain and kernel revision
#	export PATH=~/rowboat-android/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin:$PATH
#	make ARCH=arm KERNELPATH=~/rowboat-android/kernel CROSS_COMPILE=arm-eabi- sii8240_clean sii8240_debug
#	make ARCH=arm KERNELPATH=~/rowboat-android/kernel CROSS_COMPILE=arm-eabi- sii8240_clean sii8240_release
#	make ARCH=arm KERNELPATH=~/rowboat-android/kernel CROSS_COMPILE=arm-eabi- sii8240_clean sii8240_clean

ifeq ($(CONFIG_K3_HDMI),y)
EXTRA_CFLAGS += -DK3_HDMI
EXTRA_CFLAGS += -Idrivers/video/k3/hdmi/
endif

BUILDNUM = 0
ifneq ($(KERNELRELEASE),)
# kbuild part of makefile

ccflags-y := -I$(src)/include
ccflags-y += -I$(src)/platform
ccflags-y += -I$(src)/platform/api
ccflags-y += -I$(src)/platform/hal
ccflags-y += -I$(src)/component/mhl2_tx
ccflags-y += -I$(src)/component/hdmi_tx_lite_mhl2
ccflags-y += -I$(src)/driver/cra_drv
ccflags-y += -I$(src)/driver/sii8240_drv
ccflags-y += -I$(src)/driver/hdmi_tx_lite_drv_8240
ccflags-y += -I$(src)/driver/mhl2_tx_drv
ccflags-y += -I$(src)/linux_kernel/kernel_module
ccflags-y += -I$(src)/platform/include

ccflags-y += -DMAKE_8240_DRIVER 
ccflags-y += -DINTERRUPT_DRIVEN_EDID
ccflags-y += -DREQUEST_EDID_ATOMICALLY

ccflags-y += -DBUILDNUM=$(BUILDNUM)

ccflags-y += -DC99_VA_ARG_SUPPORT 
#ccflags-$(CONFIG_DEBUG_DRIVER) += -DDEBUG
ccflags-y += -DDEBUG

obj-$(CONFIG_MHL_SII8240) += sii8240drv.o
#----------------- App layer --------------------------------------
sii8240drv-objs  += linux_kernel/kernel_module/mhl_linuxdrv_main.o
sii8240drv-objs  += linux_kernel/public/mhl_linuxdrv_ioctl.o
sii8240drv-objs  += linux_kernel/public/mhl_linuxdrv_open_close.o
#sii8240drv-objs  += application/board.o
#----------------- Component layer --------------------------------
sii8240drv-objs  += component/mhl2_tx/si_mhl_tx.o
sii8240drv-objs  += component/mhl2_tx/si_mhl_tx_rcp_table.o
sii8240drv-objs  += component/hdmi_tx_lite_mhl2/si_hdmi_tx_lite.o
#----------------- Platform layer ---------------------------------
sii8240drv-objs  += platform/osal/si_osdebug.o
sii8240drv-objs  += platform/osal/sii_osal_linux_timer.o
sii8240drv-objs  += platform/hal/sii_hal_linux_init.o
sii8240drv-objs  += platform/hal/sii_hal_linux_i2c.o
sii8240drv-objs  += platform/hal/sii_hal_linux_isr.o
sii8240drv-objs  += platform/hal/sii_hal_linux_timer.o
sii8240drv-objs  += platform/hal/sii_hal_linux_gpio.o
sii8240drv-objs  += platform/hal/sii_hal_linux_switcher.o
#----------------- Driver layer --------------------------------
sii8240drv-objs  += driver/cra_drv/si_cra.o
sii8240drv-objs  += driver/sii8240_drv/si_cra_cfg.o
sii8240drv-objs  += driver/sii8240_drv/si_drv_mhl_tx.o
sii8240drv-objs  += driver/hdmi_tx_lite_drv_8240/si_drv_hdmi_tx_lite_edid.o
sii8240drv-objs  += driver/hdmi_tx_lite_drv_8240/si_drv_hdmi_tx_lite_hdcp.o
sii8240drv-objs  += driver/hdmi_tx_lite_drv_8240/si_drv_hdmi_tx_lite_hdmi.o
sii8240drv-objs  += driver/hdmi_tx_lite_drv_8240/si_drv_infoframe.o
else

# Normal Makefile

# If a specific kernel is not specified, default to 2.6.36
ifneq ($(KERNELPATH),)
KERNELDIR=$(KERNELPATH)
else
KERNELDIR=~/src/linux-2.6.36
endif
ARCH=arm

PWD := $(shell pwd)


.PHONY: sii8240_clean


sii8240_release:
	make -C $(KERNELDIR) M=$(PWD) CONFIG_SII8240_MHL_TX=m modules
	$(CROSS_COMPILE)strip --strip-debug sii8240drv.ko

sii8240_debug:
	make -C $(KERNELDIR) M=$(PWD) CONFIG_SII8240_MHL_TX=m CONFIG_DEBUG_DRIVER=y modules

sii8240_clean:
	make -C $(KERNELDIR) M=$(PWD) CONFIG_SII8240_MHL_TX=m clean
endif
