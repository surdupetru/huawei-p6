##############################################################################
#
#    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
#
#    The material in this file is confidential and contains trade secrets
#    of Vivante Corporation. This is proprietary information owned by
#    Vivante Corporation. No part of this work may be disclosed,
#    reproduced, copied, transmitted, or used in any way for any purpose,
#    without the express written permission of Vivante Corporation.
#
##############################################################################


#
# Linux build file for kernel HAL driver.
#

AQROOT := $(PWD)/drivers/misc/gpu_drv
AQARCH := $(AQROOT)/arch/XAQ2

include $(AQROOT)/config

DRIVER_OUT_DIR = hal/driver
KERNEL_DIR ?= $(TOOL_DIR)/kernel

OS_KERNEL_DIR   := hal/os/linux/kernel
ARCH_KERNEL_DIR := arch/$(notdir $(AQARCH))/hal/kernel
ARCH_VG_KERNEL_DIR := arch/$(notdir $(AQVGARCH))/hal/kernel
HAL_KERNEL_DIR  := hal/kernel

EXTRA_CFLAGS += -Werror -fno-pic

OBJS := $(OS_KERNEL_DIR)/gc_hal_kernel_device.o \
        $(OS_KERNEL_DIR)/gc_hal_kernel_driver.o \
        $(OS_KERNEL_DIR)/gc_hal_kernel_linux.o \
        $(OS_KERNEL_DIR)/gc_hal_kernel_math.o \
        $(OS_KERNEL_DIR)/gc_hal_kernel_os.o \
        $(OS_KERNEL_DIR)/gc_hal_kernel_debugfs.o

ifeq ($(USE_3D_VG), 1)

OBJS += $(HAL_KERNEL_DIR)/gc_hal_kernel.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_command.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_db.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_debug.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_event.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_heap.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_mmu.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_video_memory.o

OBJS += $(ARCH_KERNEL_DIR)/gc_hal_kernel_context.o \
        $(ARCH_KERNEL_DIR)/gc_hal_kernel_hardware.o

ifeq ($(VIVANTE_ENABLE_VG), 1)
OBJS +=\
          $(HAL_KERNEL_DIR)/gc_hal_kernel_vg.o\
          $(HAL_KERNEL_DIR)/gc_hal_kernel_command_vg.o\
          $(HAL_KERNEL_DIR)/gc_hal_kernel_interrupt_vg.o\
          $(HAL_KERNEL_DIR)/gc_hal_kernel_mmu_vg.o\
          $(ARCH_VG_KERNEL_DIR)/gc_hal_kernel_hardware_command_vg.o\
          $(ARCH_VG_KERNEL_DIR)/gc_hal_kernel_hardware_vg.o
endif
else

OBJS += $(HAL_KERNEL_DIR)/gc_hal_kernel.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_command.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_heap.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_interrupt.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_mmu.o \
        $(HAL_KERNEL_DIR)/gc_hal_kernel_video_memory.o \
        $(OS_KERNEL_DIR)/gc_hal_kernel_debug.o

OBJS += $(ARCH_KERNEL_DIR)/gc_hal_kernel_hardware.o \
        $(ARCH_KERNEL_DIR)/gc_hal_kernel_hardware_command.o

endif

ifeq ($(KERNELRELEASE), )

.PHONY: all clean install

# Define targets.
all:
	@mkdir -p $(DRIVER_OUT_DIR)
	@make V=$(V) ARCH=$(ARCH_TYPE) -C $(KERNEL_DIR) SUBDIRS=`pwd` modules

clean:
	@rm -rf $(OBJS)
	@rm -rf $(DRIVER_OUT_DIR)
	@rm -rf modules.order Module.symvers
	@find $(AQROOT) -name ".gc_*.cmd" | xargs rm -f

install: all
	@mkdir -p $(SDK_DIR)/drivers
	@cp $(DRIVER_OUT_DIR)/galcore.ko $(SDK_DIR)/drivers

else


EXTRA_CFLAGS += -DLINUX -DDRIVER

ifeq ($(ENUM_WORKAROUND), 1)
EXTRA_CFLAGS += -DENUM_WORKAROUND=1
else
EXTRA_CFLAGS += -DENUM_WORKAROUND=0
endif

ifeq ($(FLAREON),1)
EXTRA_CFLAGS += -DFLAREON
endif

ifeq ($(DEBUG), 1)
EXTRA_CFLAGS += -DDBG=1 -DDEBUG -D_DEBUG
else
EXTRA_CFLAGS += -DDBG=0
endif

#ifeq ($(NO_DMA_COHERENT), 1)
EXTRA_CFLAGS += -DNO_DMA_COHERENT
#endif

ifeq ($(CONFIG_DOVE_GPU), 1)
EXTRA_CFLAGS += -DCONFIG_DOVE_GPU=1
endif

ifeq ($(USE_POWER_MANAGEMENT), 1)
EXTRA_CFLAGS += -DgcdPOWER_MANAGEMENT=1
else
EXTRA_CFLAGS += -DgcdPOWER_MANAGEMENT=0
endif

ifeq ($(USE_OPENCL), 1)
EXTRA_CFLAGS += -DgcdUSE_OPENCL=1
else
EXTRA_CFLAGS += -DgcdUSE_OPENCL=0
endif

ifneq ($(USE_PLATFORM_DRIVER), 0)
EXTRA_CFLAGS += -DUSE_PLATFORM_DRIVER=1
else
EXTRA_CFLAGS += -DUSE_PLATFORM_DRIVER=0
endif

ifeq ($(USE_PROFILER), 1)
EXTRA_CFLAGS += -DVIVANTE_PROFILER=1
else
EXTRA_CFLAGS += -DVIVANTE_PROFILER=0
endif

ifeq ($(ANDROID), 1)
EXTRA_CFLAGS += -DANDROID=1
endif

ifeq ($(ENABLE_GPU_CLOCK_BY_DRIVER), 1)
EXTRA_CFLAGS += -DENABLE_GPU_CLOCK_BY_DRIVER=1
else
EXTRA_CFLAGS += -DENABLE_GPU_CLOCK_BY_DRIVER=0
endif

ifeq ($(USE_NEW_LINUX_SIGNAL), 1)
EXTRA_CFLAGS += -DUSE_NEW_LINUX_SIGNAL=1
else
EXTRA_CFLAGS += -DUSE_NEW_LINUX_SIGNAL=0
endif

ifeq ($(NO_USER_DIRECT_ACCESS_FROM_KERNEL), 1)
EXTRA_CFLAGS += -DNO_USER_DIRECT_ACCESS_FROM_KERNEL=1
else
EXTRA_CFLAGS += -DNO_USER_DIRECT_ACCESS_FROM_KERNEL=0
endif

ifeq ($(FORCE_ALL_VIDEO_MEMORY_CACHED), 1)
EXTRA_CFLAGS += -DgcdPAGED_MEMORY_CACHEABLE=1
else
EXTRA_CFLAGS += -DgcdPAGED_MEMORY_CACHEABLE=0
endif

ifeq ($(NONPAGED_MEMORY_CACHEABLE), 1)
EXTRA_CFLAGS += -DgcdNONPAGED_MEMORY_CACHEABLE=1
else
EXTRA_CFLAGS += -DgcdNONPAGED_MEMORY_CACHEABLE=0
endif

ifeq ($(NONPAGED_MEMORY_BUFFERABLE), 1)
EXTRA_CFLAGS += -DgcdNONPAGED_MEMORY_BUFFERABLE=1
else
EXTRA_CFLAGS += -DgcdNONPAGED_MEMORY_BUFFERABLE=0
endif

ifeq ($(CACHE_FUNCTION_UNIMPLEMENTED), 1)
EXTRA_CFLAGS += -DgcdCACHE_FUNCTION_UNIMPLEMENTED=1
else
EXTRA_CFLAGS += -DgcdCACHE_FUNCTION_UNIMPLEMENTED=0
endif

ifeq ($(SUPPORT_SWAP_RECTANGLE), 1)
EXTRA_CFLAGS += -DgcdSUPPORT_SWAP_RECTANGLE=1
else
EXTRA_CFLAGS += -DgcdSUPPORT_SWAP_RECTANGLE=0
endif

ifeq ($(VIVANTE_ENABLE_VG), 1)
EXTRA_CFLAGS += -DgcdENABLE_VG=1
else
EXTRA_CFLAGS += -DgcdENABLE_VG=0
endif

ifeq ($(CONFIG_SMP), y)
EXTRA_CFLAGS += -DgcdSMP=1
else
EXTRA_CFLAGS += -DgcdSMP=0
endif

ifeq ($(VIVANTE_NO_3D),1)
EXTRA_CFLAGS += -DVIVANTE_NO_3D
endif

ifeq ($(ENABLE_OUTER_CACHE_PATCH), 1)
EXTRA_CFLAGS += -DgcdENABLE_OUTER_CACHE_PATCH=1
else
EXTRA_CFLAGS += -DgcdENABLE_OUTER_CACHE_PATCH=0
endif

ifeq ($(USE_BANK_ALIGNMENT), 1)
    EXTRA_CFLAGS += -DgcdENABLE_BANK_ALIGNMENT=1
    ifneq ($(BANK_BIT_START), 0)
	        ifneq ($(BANK_BIT_END), 0)
	            EXTRA_CFLAGS += -DgcdBANK_BIT_START=$(BANK_BIT_START)
	            EXTRA_CFLAGS += -DgcdBANK_BIT_END=$(BANK_BIT_END)
	        endif
    endif

    ifneq ($(BANK_CHANNEL_BIT), 0)
        EXTRA_CFLAGS += -DgcdBANK_CHANNEL_BIT=$(BANK_CHANNEL_BIT)
    endif
endif

EXTRA_CFLAGS += -I$(AQROOT)/hal/kernel/inc
EXTRA_CFLAGS += -I$(AQROOT)/hal/kernel
EXTRA_CFLAGS += -I$(AQARCH)/hal/kernel
EXTRA_CFLAGS += -I$(AQROOT)/hal/os/linux/kernel

ifeq ($(VIVANTE_ENABLE_VG), 1)
EXTRA_CFLAGS += -I$(AQVGARCH)/hal/kernel
endif

obj-y = galcore.o

galcore-objs  = $(OBJS)

endif
