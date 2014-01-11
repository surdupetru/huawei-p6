PRODUCT_NAME=UEDGE
PRODUCT_BRAND=Huawei
#add macro to enable command type lcd
export USE_LCD_JDI_OTM1282B := true
#add macro to enable command type TP&TK
export USE_TP_TK_U9700L := true
#NOTICE! Do not config PRODUCT_DEVICE
export USE_EDGE_CAMERA_SETTINGS := true

export USE_AUDIO_RING_VOLUME_CHANGE := true

PRODUCT_LCD_DISPLAY=HD

ifeq ($(TARGET_VERSION_MODE),normal)
PRODUCT_PACKAGES += check_root
endif
