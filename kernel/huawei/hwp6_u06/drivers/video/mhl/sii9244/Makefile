# Makefile for MHL SiI9244 chip drivers.

ifeq ($(CONFIG_K3_HDMI),y)
EXTRA_CFLAGS += -DK3_HDMI
EXTRA_CFLAGS += -Idrivers/video/k3/hdmi/
endif

#define cbus id switcher defaut open for k3 and omap platform
EXTRA_CFLAGS += -DCBUS_ID_SWITCHER_DEFAULT_OPEN

#Share DP and DN Switcher with USB

EXTRA_CFLAGS += -DSHARE_SWITCHER_WITH_USB

ccflags-y := -I$(src)/include

#obj-$(CONFIG_MHL_SII9244)	+= si_drv_mhl_tx.o
#obj-$(CONFIG_MHL_SII9244)	+= si_mhl_tx.o
#obj-$(CONFIG_MHL_SII9244)	+= si_mhl_tx_rcp_table.o
#obj-$(CONFIG_MHL_SII9244)	+= sii_reg_access.o
#obj-$(CONFIG_MHL_SII9244)	+= sii_9244_api.o

obj-$(CONFIG_MHL_SII9244)	+= sii_9244_driver.o
sii_9244_driver-objs				:= sii_9244_api.o si_drv_mhl_tx.o \
										si_mhl_tx.o sii_reg_access.o \
										si_mhl_tx_rcp_table.o

#ccflags-$(CONFIG_I2C_DEBUG_BUS) := -DDEBUG