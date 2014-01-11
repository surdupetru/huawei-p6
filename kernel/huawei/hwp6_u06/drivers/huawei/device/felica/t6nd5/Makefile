#
# Makefile for FeliCa control driver.
#

## Defined compiling option
EXTRA_CFLAGS :=
EXTRA_CFLAGS += -Idrivers/felica/inc

# target module name
DRV_NAME_CTRL = felica_gpio

obj-$(CONFIG_HUAWEI_FEATURE_FELICA_T6ND5)   := $(DRV_NAME_CTRL).o
$(DRV_NAME_CTRL)-objs = \
        src/felica_gpio.o 

# target module name
DRV_NAME_COMM = felica_uart

obj-$(CONFIG_HUAWEI_FEATURE_FELICA_T6ND5)   += $(DRV_NAME_COMM).o
$(DRV_NAME_COMM)-objs = \
        src/felica_uart.o 

# target module name
DRV_NAME_RWS = felica_rws

obj-$(CONFIG_HUAWEI_FEATURE_FELICA_T6ND5)   += $(DRV_NAME_RWS).o
$(DRV_NAME_RWS)-objs = \
        src/felica_rws.o 

## End of Makefile
