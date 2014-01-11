#
# Makefile for TI's shared transport driver based wl128x
# FM radio.
#

#ifeq ($(THIS_PRODUCT_VERSION_MODE),factory)
EXTRA_CFLAGS += -DFACTORY_WORK_AROUD
#endif
obj-$(CONFIG_RADIO_WL128X)	+= fm_drv.o
fm_drv-objs		:= fmdrv_common.o fmdrv_rx.o fmdrv_tx.o fmdrv_v4l2.o
