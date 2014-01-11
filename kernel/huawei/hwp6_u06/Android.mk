#Android makefile to build kernel as a part of Android Build

#ifeq ($(TARGET_PREBUILT_KERNEL),)

KERNEL_OUT := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ
KERNEL_CONFIG := $(KERNEL_OUT)/.config
TARGET_PREBUILT_KERNEL := $(KERNEL_OUT)/arch/arm/boot/zImage

KERNEL_ARCH_ARM_CONFIGS := kernel/arch/arm/configs
KERNEL_GEN_CONFIG_FILE := huawei_k3v2oem1_$(HW_PRODUCT)_defconfig
KERNEL_GEN_CONFIG_PATH := $(KERNEL_ARCH_ARM_CONFIGS)/$(KERNEL_GEN_CONFIG_FILE)

KERNEL_COMMON_DEFCONFIG := $(KERNEL_ARCH_ARM_CONFIGS)/$(KERNEL_DEFCONFIG)
KERNEL_PRODUCT_CONFIGS  := device/hisi/k3v2oem1/product_spec/kernel_config/$(HW_PRODUCT)
KERNEL_DEBUG_CONFIGS := $(KERNEL_ARCH_ARM_CONFIGS)/debug

$(shell cd device/hisi/customize/hsad;./xml2complete.sh > /dev/null)
$(shell cd kernel/drivers/huawei/hsad;./xml2code.sh)

#add debug-kernel-config file generation rules by z00175161
ifeq ($(TARGET_BUILD_TYPE),release)
  KERNEL_DEBUG_CONFIGFILE := $(KERNEL_COMMON_DEFCONFIG)
  
$(KERNEL_DEBUG_CONFIGFILE):FORCE
	echo "do nothing"  

else
  KERNEL_DEBUG_CONFIGFILE :=  $(KERNEL_ARCH_ARM_CONFIGS)/huawei_k3v2oem1_$(HW_PRODUCT)_debug_defconfig
  
$(KERNEL_DEBUG_CONFIGFILE):FORCE
	$(shell device/hisi/k3v2oem1/kernel-config.sh -f $(KERNEL_COMMON_DEFCONFIG) -d $(KERNEL_DEBUG_CONFIGS) -o $(KERNEL_DEBUG_CONFIGFILE))
endif

ifeq ($(KERNEL_DEBUG_CONFIGFILE),$(KERNEL_COMMON_DEFCONFIG))
KERNEL_TOBECLEAN_CONFIGFILE := 
else
KERNEL_TOBECLEAN_CONFIGFILE := $(KERNEL_DEBUG_CONFIGFILE)
endif


$(KERNEL_OUT):
	mkdir -p $(KERNEL_OUT)

$(KERNEL_GEN_CONFIG_PATH): $(KERNEL_DEBUG_CONFIGFILE)
	$(shell device/hisi/k3v2oem1/kernel-config.sh -f $(KERNEL_DEBUG_CONFIGFILE) -d $(KERNEL_PRODUCT_CONFIGS) -o $(KERNEL_GEN_CONFIG_PATH))

$(KERNEL_CONFIG): $(KERNEL_OUT) $(KERNEL_GEN_CONFIG_PATH)
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi- $(KERNEL_GEN_CONFIG_FILE)
	@rm -frv $(KERNEL_GEN_CONFIG_PATH)
	@rm -frv $(KERNEL_TOBECLEAN_CONFIGFILE)


$(TARGET_PREBUILT_KERNEL): $(KERNEL_CONFIG)
	$(hide) $(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi- -j 18
	$(hide) $(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi- -j 18 zImage

kernelconfig: $(KERNEL_OUT) $(KERNEL_GEN_CONFIG_PATH)
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi- $(KERNEL_GEN_CONFIG_FILE) menuconfig
	@rm -frv $(KERNEL_GEN_CONFIG_PATH)
