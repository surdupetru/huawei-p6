# Makefile for the gyroscope drivers.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_HUAWEI_FEATURE_GYROSCOPE_L3G4200DH)	+= l3g4200d.o

ifeq ($(CONFIG_GYROSCOPE_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif
