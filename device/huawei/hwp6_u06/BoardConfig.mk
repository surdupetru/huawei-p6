BOARD_VENDOR := Huawei

USE_CAMERA_STUB := false

# Audio
TARGET_PROVIDES_LIBAUDIO := true

# inherit from the proprietary version
include vendor/huawei/hwp6_u06/BoardConfigVendor.mk

TARGET_ARCH := arm
TARGET_NO_BOOTLOADER := true
TARGET_BOARD_PLATFORM := k3v2oem1
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
ARCH_ARM_HAVE_TLS_REGISTER := true
TARGET_ARCH_VARIANT_CPU := cortex-a9
TARGET_CPU_VARIANT := cortex-a9
TARGET_ARCH_VARIANT_FPU := neon

ARCH_ARM_HAVE_32_BYTE_CACHE_LINES := true
ARCH_ARM_USE_NON_NEON_MEMCPY := true

NEED_WORKAROUND_CORTEX_A9_745320 := true

TARGET_BOOTLOADER_BOARD_NAME := hwp6_u06
BOARD_VENDOR_PLATFORM := k3v2oem1

#TARGET_USE_KRAIT_BIONIC_OPTIMIZATION := true
#TARGET_USE_KRAIT_PLD_SET := true
#TARGET_KRAIT_BIONIC_PLDOFFS := 10
#TARGET_KRAIT_BIONIC_PLDTHRESH := 10
#TARGET_KRAIT_BIONIC_BBTHRESH := 64
#TARGET_KRAIT_BIONIC_PLDSIZE := 64

# Webkit
ENABLE_WEBGL := true
TARGET_FORCE_CPU_UPLOAD := true

# Wifi
USES_TI_MAC80211 := true
ifdef USES_TI_MAC80211
WPA_SUPPLICANT_VERSION           := VER_0_8_X_TI
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_wl12xx
BOARD_HOSTAPD_DRIVER             := NL80211
BOARD_HOSTAPD_PRIVATE_LIB        := lib_driver_cmd_wl12xx
PRODUCT_WIRELESS_TOOLS           := true
BOARD_WLAN_DEVICE                := wl12xx_mac80211
BOARD_SOFTAP_DEVICE              := wl12xx_mac80211
WIFI_DRIVER_MODULE_PATH          := "/system/lib/modules/wlcore_sdio.ko"
WIFI_DRIVER_MODULE_NAME          := "wlcore_sdio"
WIFI_FIRMWARE_LOADER             := ""
COMMON_GLOBAL_CFLAGS += -DUSES_TI_MAC80211
endif


# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_WPAN_DEVICE :=true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/huawei/hwp6_u06/bluetooth

#TARGET_PREBUILT_KERNEL := device/huawei/hwp6_u06/kernel
TARGET_KERNEL_SOURCE := kernel/huawei/hwp6_u06
TARGET_KERNEL_CONFIG := cm_k3v2oem1_defconfig

# Camera
#COMMON_GLOBAL_CFLAGS += -DQCOM_BSP_CAMERA_ABI_HACK

# adb has root
ADDITIONAL_DEFAULT_PROPERTIES += ro.secure=0
ADDITIONAL_DEFAULT_PROPERTIES += ro.allow.mock.location=1
ADDITIONAL_DEFAULT_PROPERTIES += persist.sys.usb.config=modem,nmea,mass_storage,diag,default_mode

# Graphics
BOARD_EGL_CFG := device/huawei/hwp6_u06/prebuilt/lib/egl/egl.cfg
USE_OPENGL_RENDERER := true

# Kernel
BOARD_KERNEL_CMDLINE := vmalloc=512M k3v2_pmem=1 mmcparts=mmcblk0:p1(xloader),p3(nvme),p4(misc),p5(splash),p6(oeminfo),p7(reserved1),p8(reserved2),p9(splash2),p10(recovery2),p11(recovery),p12(boot),p13(modemimage),p14(modemnvm1),p15(modemnvm2),p16(system),p17(cache),p18(cust),p19(userdata);mmcblk1:p1(ext_sdcard)
BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_PAGESIZE := 2048

# fix this up by examining /proc/mtd on a running device
BOARD_BOOTIMAGE_PARTITION_SIZE := 8388608
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 16777216
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1610612736
BOARD_USERDATAIMAGE_PARTITION_SIZE := 5255462912


# USB mass storage
TARGET_USE_CUSTOM_LUN_FILE_PATH := "/sys/devices/virtual/android_usb/android0/f_mass_storage/lun/file"
BOARD_MTP_DEVICE := "/dev/mtp"
BOARD_VOLD_MAX_PARTITIONS := 19
BOARD_VOLD_EMMC_SHARES_DEV_MAJOR := true

# Recovery
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_TOUCH_RECOVERY := 
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_RECOVERY_PIXEL_FORMAT := "RGB_8888"
BOARD_HAS_SDCARD_INTERNAL := true
BOARD_HAS_LOCKED_BOOTLOADER := true
RECOVERY_FSTAB_VERSION := 2
BOARD_FLASH_BLOCK_SIZE := 131072
BOARD_HAS_LARGE_FILESYSTEM := true
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/huawei/hwp6_u06/recovery/recovery_keys.c
TARGET_RECOVERY_FSTAB := device/huawei/hwp6_u06/recovery/recovery.fstab
BOARD_USE_CUSTOM_RECOVERY_FONT := \"roboto_15x24.h\"

BOARD_RECOVERY_SWIPE := true

#TWRP
HAVE_SELINUX := false

TW_MAX_BRIGHTNESS := 255
DEVICE_RESOLUTION := 720x1280
RECOVERY_GRAPHICS_USE_LINELENGTH := true

TW_ALWAYS_RMRF := true
TW_NEVER_UMOUNT_SYSTEM := true

TW_CUSTOM_BATTERY_PATH := "/sys/devices/platform/bq_bci_battery.1/Battery"

RECOVERY_SDCARD_ON_DATA := true 
TW_HAS_NO_RECOVERY_PARTITION := true
TW_FLASH_FROM_STORAGE := true
TW_EXTERNAL_STORAGE_PATH := "/external_sd"
TW_EXTERNAL_STORAGE_MOUNT_POINT := "external_sd"
TW_DEFAULT_EXTERNAL_STORAGE := true

TW_INTERNAL_STORAGE_PATH := "/data/share"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
TW_INCLUDE_JB_CRYPTO := true
TW_CRYPTO_FS_TYPE := "ext4"
TW_BRIGHTNESS_PATH := /sys/class/leds/lcd_backlight0/brightness
TW_CRYPTO_REAL_BLKDEV := "/dev/block/platform/hi_mci.1/by-name/userdata"
TW_CRYPTO_MNT_POINT := "/data"
