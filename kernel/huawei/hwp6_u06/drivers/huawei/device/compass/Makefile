#sensor unification for differ platform
# Makefile for the compass drivers.
#

# Each configuration option enables a list of files.
obj-$(CONFIG_HUAWEI_FEATURE_SENSORS_AKM8975)   += akm8975.o
obj-$(CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963)   += akm8963.o
obj-$(CONFIG_HUAWEI_FEATURE_SENSORS_YAMAHA_COMPASS)   += yas_ori_kernel_driver.o yas_mag_driver-yas530.o yas_mag_kernel_driver.o
ifeq ($(CONFIG_COMPASS_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

