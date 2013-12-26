$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

# The gps config appropriate for this device
$(call inherit-product, device/common/gps/gps_us_supl.mk)

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

$(call inherit-product-if-exists, vendor/huawei/hwp6_u06/hwp6_u06-vendor.mk)

LOCAL_PATH := device/huawei/hwp6_u06

DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay


ifeq ($(TARGET_PREBUILT_KERNEL),)
	LOCAL_KERNEL := $(LOCAL_PATH)/kernel
else
	LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_COPY_FILES += \
    $(LOCAL_KERNEL):kernel

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/root/ueventd.k3v2oem1.rc:root/ueventd.k3v2oem1.rc \
    $(LOCAL_PATH)/root/fstab.k3v2oem1:root/fstab.k3v2oem1 \
    $(LOCAL_PATH)/recovery/postrecoveryboot.sh:recovery/root/sbin/postrecoveryboot.sh \
    $(LOCAL_PATH)/root/ueventd.k3v2oem1.rc:recovery/root/ueventd.k3v2oem1.rc \
    $(LOCAL_PATH)/root/fstab.k3v2oem1:recovery/root/fstab.k3v2oem1


PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml

PRODUCT_AAPT_CONFIG := normal hdpi xhdpi xxhdpi
PRODUCT_AAPT_PREF_CONFIG := xhdpi

PRODUCT_LOCALES += en_US

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_C9800D_ce_cs.bin:system/etc/audio/audience/audience_C9800D_ce_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_C9800D_default_cs.bin:system/etc/audio/audience/audience_C9800D_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_CEDGE_ce_cs.bin:system/etc/audio/audience/audience_CEDGE_ce_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_CEDGE_default_cs.bin:system/etc/audio/audience/audience_CEDGE_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_default_cs.bin:system/etc/audio/audience/audience_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_k3v2oem1_default_cs.bin:system/etc/audio/audience/audience_k3v2oem1_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_T9800L_ce_cs.bin:system/etc/audio/audience/audience_T9800L_ce_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_T9800L_default_cs.bin:system/etc/audio/audience/audience_T9800L_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_T9900_ce_cs.bin:system/etc/audio/audience/audience_T9900_ce_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_T9900_default_cs.bin:system/etc/audio/audience/audience_T9900_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9700L_ce_cs.bin:system/etc/audio/audience/audience_U9700L_ce_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9700L_default_cs.bin:system/etc/audio/audience/audience_U9700L_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9701G_ce_cs.bin:system/etc/audio/audience/audience_U9701G_ce_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9701G_default_cs.bin:system/etc/audio/audience/audience_U9701G_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9701L_ce_cs.bin:system/etc/audio/audience/audience_U9701L_ce_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9701L_default_cs.bin:system/etc/audio/audience/audience_U9701L_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9800D_ce_cs.bin:system/etc/audio/audience/audience_U9800D_ce_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9800D_default_cs.bin:system/etc/audio/audience/audience_U9800D_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9900_ce_cs.bin:system/etc/audio/audience/audience_U9900_ce_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9900_default_cs.bin:system/etc/audio/audience/audience_U9900_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9900L_ce_cs.bin:system/etc/audio/audience/audience_U9900L_ce_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/audience/audience_U9900L_default_cs.bin:system/etc/audio/audience/audience_U9900L_default_cs.bin \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_C9800D_ce_ADL.dat:system/etc/audio/codec/asound_C9800D_ce_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_C9800D_default_ADL.dat:system/etc/audio/codec/asound_C9800D_default_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_CEDGE_ce_ADL.dat:system/etc/audio/codec/asound_CEDGE_ce_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_CEDGE_default_ADL.dat:system/etc/audio/codec/asound_CEDGE_default_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_default_ADL.dat:system/etc/audio/codec/asound_default_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_k3v2oem1_default_ADL.dat:system/etc/audio/codec/asound_k3v2oem1_default_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_T9800L_cs_ADL.dat:system/etc/audio/codec/asound_T9800L_cs_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_T9800L_default_ADL.dat:system/etc/audio/codec/asound_T9800L_default_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_T9900_ce_ADL.dat:system/etc/audio/codec/asound_T9900_ce_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_T9900_default_ADL.dat:system/etc/audio/codec/asound_T9900_default_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_TEDGE_ce_NDL.dat:system/etc/audio/codec/asound_TEDGE_ce_NDL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_TEDGE_default_NDL.dat:system/etc/audio/codec/asound_TEDGE_default_NDL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9700L_ce_ADL.dat:system/etc/audio/codec/asound_U9700L_ce_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9700L_default_ADL.dat:system/etc/audio/codec/asound_U9700L_default_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9701G_ce_NSL.dat:system/etc/audio/codec/asound_U9701G_ce_NSL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9701G_default_NSL.dat:system/etc/audio/codec/asound_U9701G_default_NSL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9701L_ce_ASL.dat:system/etc/audio/codec/asound_U9701L_ce_ASL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9701L_default_ADL.dat:system/etc/audio/codec/asound_U9701L_default_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9701L_default_ASL.dat:system/etc/audio/codec/asound_U9701L_default_ASL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9800D_ce_ADL.dat:system/etc/audio/codec/asound_U9800D_ce_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9800D_default_ADL.dat:system/etc/audio/codec/asound_U9800D_default_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9900_ce_NDL.dat:system/etc/audio/codec/asound_U9900_ce_NDL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9900_default_NDL.dat:system/etc/audio/codec/asound_U9900_default_NDL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9900L_ce_ADL.dat:system/etc/audio/codec/asound_U9900L_ce_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_U9900L_default_ADL.dat:system/etc/audio/codec/asound_U9900L_default_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_UEDGE_ce_NDL.dat:system/etc/audio/codec/asound_UEDGE_ce_NDL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/codec/asound_UEDGE_default_NDL.dat:system/etc/audio/codec/asound_UEDGE_default_NDL.dat \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_capture.txt:system/etc/audio/fir_filter/fir_coef_capture.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_capture_C9800D_default.txt:system/etc/audio/fir_filter/fir_coef_capture_C9800D_default.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_capture_CEDGE_default.txt:system/etc/audio/fir_filter/fir_coef_capture_CEDGE_default.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_capture_T9800L_default.txt:system/etc/audio/fir_filter/fir_coef_capture_T9800L_default.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_capture_TEDGE_default.txt:system/etc/audio/fir_filter/fir_coef_capture_TEDGE_default.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_capture_U9701G_default.txt:system/etc/audio/fir_filter/fir_coef_capture_U9701G_default.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_capture_U9800D_default.txt:system/etc/audio/fir_filter/fir_coef_capture_U9800D_default.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_capture_UEDGE_default.txt:system/etc/audio/fir_filter/fir_coef_capture_UEDGE_default.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_speaker.txt:system/etc/audio/fir_filter/fir_coef_speaker.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_speaker_C9800D_default.txt:system/etc/audio/fir_filter/fir_coef_speaker_C9800D_default.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_speaker_T9800L_default.txt:system/etc/audio/fir_filter/fir_coef_speaker_T9800L_default.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_speaker_U9701G_default.txt:system/etc/audio/fir_filter/fir_coef_speaker_U9701G_default.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/fir_filter/fir_coef_speaker_U9701L_default.txt:system/etc/audio/fir_filter/fir_coef_speaker_U9701L_default.txt \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_C9800D.xml:system/etc/audio/stereo_enhancement/stereo_C9800D.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_CEDGE.xml:system/etc/audio/stereo_enhancement/stereo_CEDGE.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_T9800L.xml:system/etc/audio/stereo_enhancement/stereo_T9800L.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_T9900.xml:system/etc/audio/stereo_enhancement/stereo_T9900.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_TEDGE.xml:system/etc/audio/stereo_enhancement/stereo_TEDGE.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_U9700L.xml:system/etc/audio/stereo_enhancement/stereo_U9700L.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_U9701G.xml:system/etc/audio/stereo_enhancement/stereo_U9701G.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_U9701L.xml:system/etc/audio/stereo_enhancement/stereo_U9701L.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_U9800D.xml:system/etc/audio/stereo_enhancement/stereo_U9800D.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_U9900.xml:system/etc/audio/stereo_enhancement/stereo_U9900.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_U9900L.xml:system/etc/audio/stereo_enhancement/stereo_U9900L.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/stereo_enhancement/stereo_UEDGE.xml:system/etc/audio/stereo_enhancement/stereo_UEDGE.xml \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/fm/fm.eq:system/etc/audio/tfa9887/CEDGE/ce/fm/fm.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/fm/fm.preset:system/etc/audio/tfa9887/CEDGE/ce/fm/fm.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/incall_nb/incall_nb.eq:system/etc/audio/tfa9887/CEDGE/ce/incall_nb/incall_nb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/incall_nb/incall_nb.preset:system/etc/audio/tfa9887/CEDGE/ce/incall_nb/incall_nb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/incall_wb/incall_wb.eq:system/etc/audio/tfa9887/CEDGE/ce/incall_wb/incall_wb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/incall_wb/incall_wb.preset:system/etc/audio/tfa9887/CEDGE/ce/incall_wb/incall_wb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/music/music.eq:system/etc/audio/tfa9887/CEDGE/ce/music/music.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/music/music.preset:system/etc/audio/tfa9887/CEDGE/ce/music/music.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/ringtone/ringtone.eq:system/etc/audio/tfa9887/CEDGE/ce/ringtone/ringtone.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/ringtone/ringtone.preset:system/etc/audio/tfa9887/CEDGE/ce/ringtone/ringtone.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.eq:system/etc/audio/tfa9887/CEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.preset:system/etc/audio/tfa9887/CEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/voip/voip.eq:system/etc/audio/tfa9887/CEDGE/ce/voip/voip.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/ce/voip/voip.preset:system/etc/audio/tfa9887/CEDGE/ce/voip/voip.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/fm/fm.eq:system/etc/audio/tfa9887/CEDGE/normal/fm/fm.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/fm/fm.preset:system/etc/audio/tfa9887/CEDGE/normal/fm/fm.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/incall_nb/incall_nb.eq:system/etc/audio/tfa9887/CEDGE/normal/incall_nb/incall_nb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/incall_nb/incall_nb.preset:system/etc/audio/tfa9887/CEDGE/normal/incall_nb/incall_nb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/incall_wb/incall_wb.eq:system/etc/audio/tfa9887/CEDGE/normal/incall_wb/incall_wb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/incall_wb/incall_wb.preset:system/etc/audio/tfa9887/CEDGE/normal/incall_wb/incall_wb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/music/music.eq:system/etc/audio/tfa9887/CEDGE/normal/music/music.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/music/music.preset:system/etc/audio/tfa9887/CEDGE/normal/music/music.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/ringtone/ringtone.eq:system/etc/audio/tfa9887/CEDGE/normal/ringtone/ringtone.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/ringtone/ringtone.preset:system/etc/audio/tfa9887/CEDGE/normal/ringtone/ringtone.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.eq:system/etc/audio/tfa9887/CEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.preset:system/etc/audio/tfa9887/CEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/voip/voip.eq:system/etc/audio/tfa9887/CEDGE/normal/voip/voip.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/normal/voip/voip.preset:system/etc/audio/tfa9887/CEDGE/normal/voip/voip.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/CEDGE/speaker_model.speaker:system/etc/audio/tfa9887/CEDGE/speaker_model.speaker \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/fm/fm.eq:system/etc/audio/tfa9887/default/ce/fm/fm.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/fm/fm.preset:system/etc/audio/tfa9887/default/ce/fm/fm.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/incall_nb/incall_nb.eq:system/etc/audio/tfa9887/default/ce/incall_nb/incall_nb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/incall_nb/incall_nb.preset:system/etc/audio/tfa9887/default/ce/incall_nb/incall_nb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/incall_wb/incall_wb.eq:system/etc/audio/tfa9887/default/ce/incall_wb/incall_wb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/incall_wb/incall_wb.preset:system/etc/audio/tfa9887/default/ce/incall_wb/incall_wb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/music/music.eq:system/etc/audio/tfa9887/default/ce/music/music.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/music/music.preset:system/etc/audio/tfa9887/default/ce/music/music.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/ringtone/ringtone.eq:system/etc/audio/tfa9887/default/ce/ringtone/ringtone.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/ringtone/ringtone.preset:system/etc/audio/tfa9887/default/ce/ringtone/ringtone.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/ringtone_hs_spk/ringtone_hs_spk.eq:system/etc/audio/tfa9887/default/ce/ringtone_hs_spk/ringtone_hs_spk.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/ringtone_hs_spk/ringtone_hs_spk.preset:system/etc/audio/tfa9887/default/ce/ringtone_hs_spk/ringtone_hs_spk.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/voip/voip.eq:system/etc/audio/tfa9887/default/ce/voip/voip.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/ce/voip/voip.preset:system/etc/audio/tfa9887/default/ce/voip/voip.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/fm/fm.eq:system/etc/audio/tfa9887/default/normal/fm/fm.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/fm/fm.preset:system/etc/audio/tfa9887/default/normal/fm/fm.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/incall_nb/incall_nb.eq:system/etc/audio/tfa9887/default/normal/incall_nb/incall_nb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/incall_nb/incall_nb.preset:system/etc/audio/tfa9887/default/normal/incall_nb/incall_nb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/incall_wb/incall_wb.eq:system/etc/audio/tfa9887/default/normal/incall_wb/incall_wb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/incall_wb/incall_wb.preset:system/etc/audio/tfa9887/default/normal/incall_wb/incall_wb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/music/music.eq:system/etc/audio/tfa9887/default/normal/music/music.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/music/music.preset:system/etc/audio/tfa9887/default/normal/music/music.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/ringtone/ringtone.eq:system/etc/audio/tfa9887/default/normal/ringtone/ringtone.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/ringtone/ringtone.preset:system/etc/audio/tfa9887/default/normal/ringtone/ringtone.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/ringtone_hs_spk/ringtone_hs_spk.eq:system/etc/audio/tfa9887/default/normal/ringtone_hs_spk/ringtone_hs_spk.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/ringtone_hs_spk/ringtone_hs_spk.preset:system/etc/audio/tfa9887/default/normal/ringtone_hs_spk/ringtone_hs_spk.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/voip/voip.eq:system/etc/audio/tfa9887/default/normal/voip/voip.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/normal/voip/voip.preset:system/etc/audio/tfa9887/default/normal/voip/voip.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/default/speaker_model.speaker:system/etc/audio/tfa9887/default/speaker_model.speaker \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/fm/fm.eq:system/etc/audio/tfa9887/TEDGE/ce/fm/fm.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/fm/fm.preset:system/etc/audio/tfa9887/TEDGE/ce/fm/fm.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/incall_nb/incall_nb.eq:system/etc/audio/tfa9887/TEDGE/ce/incall_nb/incall_nb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/incall_nb/incall_nb.preset:system/etc/audio/tfa9887/TEDGE/ce/incall_nb/incall_nb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/incall_wb/incall_wb.eq:system/etc/audio/tfa9887/TEDGE/ce/incall_wb/incall_wb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/incall_wb/incall_wb.preset:system/etc/audio/tfa9887/TEDGE/ce/incall_wb/incall_wb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/music/music.eq:system/etc/audio/tfa9887/TEDGE/ce/music/music.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/music/music.preset:system/etc/audio/tfa9887/TEDGE/ce/music/music.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/ringtone/ringtone.eq:system/etc/audio/tfa9887/TEDGE/ce/ringtone/ringtone.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/ringtone/ringtone.preset:system/etc/audio/tfa9887/TEDGE/ce/ringtone/ringtone.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.eq:system/etc/audio/tfa9887/TEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.preset:system/etc/audio/tfa9887/TEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/voip/voip.eq:system/etc/audio/tfa9887/TEDGE/ce/voip/voip.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/ce/voip/voip.preset:system/etc/audio/tfa9887/TEDGE/ce/voip/voip.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/fm/fm.eq:system/etc/audio/tfa9887/TEDGE/normal/fm/fm.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/fm/fm.preset:system/etc/audio/tfa9887/TEDGE/normal/fm/fm.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/incall_nb/incall_nb.eq:system/etc/audio/tfa9887/TEDGE/normal/incall_nb/incall_nb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/incall_nb/incall_nb.preset:system/etc/audio/tfa9887/TEDGE/normal/incall_nb/incall_nb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/incall_wb/incall_wb.eq:system/etc/audio/tfa9887/TEDGE/normal/incall_wb/incall_wb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/incall_wb/incall_wb.preset:system/etc/audio/tfa9887/TEDGE/normal/incall_wb/incall_wb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/music/music.eq:system/etc/audio/tfa9887/TEDGE/normal/music/music.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/music/music.preset:system/etc/audio/tfa9887/TEDGE/normal/music/music.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/ringtone/ringtone.eq:system/etc/audio/tfa9887/TEDGE/normal/ringtone/ringtone.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/ringtone/ringtone.preset:system/etc/audio/tfa9887/TEDGE/normal/ringtone/ringtone.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.eq:system/etc/audio/tfa9887/TEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.preset:system/etc/audio/tfa9887/TEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/voip/voip.eq:system/etc/audio/tfa9887/TEDGE/normal/voip/voip.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/normal/voip/voip.preset:system/etc/audio/tfa9887/TEDGE/normal/voip/voip.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/TEDGE/speaker_model.speaker:system/etc/audio/tfa9887/TEDGE/speaker_model.speaker \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/fm/fm.eq:system/etc/audio/tfa9887/UEDGE/ce/fm/fm.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/fm/fm.preset:system/etc/audio/tfa9887/UEDGE/ce/fm/fm.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/incall_nb/incall_nb.eq:system/etc/audio/tfa9887/UEDGE/ce/incall_nb/incall_nb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/incall_nb/incall_nb.preset:system/etc/audio/tfa9887/UEDGE/ce/incall_nb/incall_nb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/incall_wb/incall_wb.eq:system/etc/audio/tfa9887/UEDGE/ce/incall_wb/incall_wb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/incall_wb/incall_wb.preset:system/etc/audio/tfa9887/UEDGE/ce/incall_wb/incall_wb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/music/music.eq:system/etc/audio/tfa9887/UEDGE/ce/music/music.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/music/music.preset:system/etc/audio/tfa9887/UEDGE/ce/music/music.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/ringtone/ringtone.eq:system/etc/audio/tfa9887/UEDGE/ce/ringtone/ringtone.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/ringtone/ringtone.preset:system/etc/audio/tfa9887/UEDGE/ce/ringtone/ringtone.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.eq:system/etc/audio/tfa9887/UEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.preset:system/etc/audio/tfa9887/UEDGE/ce/ringtone_hs_spk/ringtone_hs_spk.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/voip/voip.eq:system/etc/audio/tfa9887/UEDGE/ce/voip/voip.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/ce/voip/voip.preset:system/etc/audio/tfa9887/UEDGE/ce/voip/voip.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/fm/fm.eq:system/etc/audio/tfa9887/UEDGE/normal/fm/fm.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/fm/fm.preset:system/etc/audio/tfa9887/UEDGE/normal/fm/fm.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/incall_nb/incall_nb.eq:system/etc/audio/tfa9887/UEDGE/normal/incall_nb/incall_nb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/incall_nb/incall_nb.preset:system/etc/audio/tfa9887/UEDGE/normal/incall_nb/incall_nb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/incall_wb/incall_wb.eq:system/etc/audio/tfa9887/UEDGE/normal/incall_wb/incall_wb.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/incall_wb/incall_wb.preset:system/etc/audio/tfa9887/UEDGE/normal/incall_wb/incall_wb.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/music/music.eq:system/etc/audio/tfa9887/UEDGE/normal/music/music.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/music/music.preset:system/etc/audio/tfa9887/UEDGE/normal/music/music.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/ringtone/ringtone.eq:system/etc/audio/tfa9887/UEDGE/normal/ringtone/ringtone.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/ringtone/ringtone.preset:system/etc/audio/tfa9887/UEDGE/normal/ringtone/ringtone.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.eq:system/etc/audio/tfa9887/UEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.preset:system/etc/audio/tfa9887/UEDGE/normal/ringtone_hs_spk/ringtone_hs_spk.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/voip/voip.eq:system/etc/audio/tfa9887/UEDGE/normal/voip/voip.eq \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/normal/voip/voip.preset:system/etc/audio/tfa9887/UEDGE/normal/voip/voip.preset \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/UEDGE/speaker_model.speaker:system/etc/audio/tfa9887/UEDGE/speaker_model.speaker \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/coldboot.patch:system/etc/audio/tfa9887/coldboot.patch \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/tfa9887_feature.config:system/etc/audio/tfa9887/tfa9887_feature.config \
    $(LOCAL_PATH)/prebuilt/etc/audio/tfa9887/tfa9887_firmware.patch:system/etc/audio/tfa9887/tfa9887_firmware.patch \
    $(LOCAL_PATH)/prebuilt/etc/audio/tpa2028/tpa2028_U9701G.cfg:system/etc/audio/tpa2028/tpa2028_U9701G.cfg \
    $(LOCAL_PATH)/prebuilt/etc/audio/tpa2028/tpa2028_U9701L.cfg:system/etc/audio/tpa2028/tpa2028_U9701L.cfg


PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/bin/adbd:system/bin/adbd \
    $(LOCAL_PATH)/prebuilt/etc/camera_orientation.cfg:system/etc/camera_orientation.cfg \
    $(LOCAL_PATH)/prebuilt/etc/camera_resolutions.cfg:system/etc/camera_resolutions.cfg \
    $(LOCAL_PATH)/prebuilt/etc/es305.bin:system/etc/es305.bin \
    $(LOCAL_PATH)/prebuilt/etc/es305_uart.bin:system/etc/es305_uart.bin \
    $(LOCAL_PATH)/prebuilt/etc/gps.conf:system/etc/gps.conf \
    $(LOCAL_PATH)/prebuilt/etc/gpsconfig.xml:system/etc/gpsconfig.xml \
    $(LOCAL_PATH)/prebuilt/etc/install-recovery.sh:system/etc/install-recovery.sh \
    $(LOCAL_PATH)/prebuilt/etc/k3_omx.cfg:system/etc/k3_omx.cfg \
    $(LOCAL_PATH)/prebuilt/etc/ril_balong_radio.cfg:/system/etc/ril_balong_radio.cfg \
    $(LOCAL_PATH)/prebuilt/etc/ril_xgold_radio.cfg:/system/etc/ril_xgold_radio.cfg \
    $(LOCAL_PATH)/prebuilt/etc/vold.fstab:system/etc/vold.fstab \
    $(LOCAL_PATH)/prebuilt/etc/audio_policy.conf:system/etc/audio_policy.conf \
    $(LOCAL_PATH)/prebuilt/etc/media_codecs.xml:system/etc/media_codecs.xml \
    $(LOCAL_PATH)/prebuilt/etc/media_profiles.xml:system/etc/media_profiles.xml \
    $(LOCAL_PATH)/prebuilt/etc/asound_ADL.dat:system/etc/asound_ADL.dat \
    $(LOCAL_PATH)/prebuilt/etc/batt_fw.bin:system/etc/batt_fw.bin

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/device.config:system/etc/camera/davinci/device.config \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/default/cm_correction.dat:system/etc/camera/davinci/default/cm_correction.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/default/cm_foliage.dat:system/etc/camera/davinci/default/cm_foliage.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/default/cm_normal.dat:system/etc/camera/davinci/default/cm_normal.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/default/cm_sky.dat:system/etc/camera/davinci/default/cm_sky.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/default/cm_sunset.dat:system/etc/camera/davinci/default/cm_sunset.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/default/imgproc.xml:system/etc/camera/davinci/default/imgproc.xml \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/hi542/cm_correction.dat:system/etc/camera/davinci/hi542/cm_correction.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/hi542/cm_foliage.dat:system/etc/camera/davinci/hi542/cm_foliage.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/hi542/cm_normal.dat:system/etc/camera/davinci/hi542/cm_normal.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/hi542/cm_sky.dat:system/etc/camera/davinci/hi542/cm_sky.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/hi542/cm_sunset.dat:system/etc/camera/davinci/hi542/cm_sunset.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/hi542/imgproc.xml:system/etc/camera/davinci/hi542/imgproc.xml \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/ov8830/cm_correction.dat:system/etc/camera/davinci/ov8830/cm_correction.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/ov8830/cm_foliage.dat:system/etc/camera/davinci/ov8830/cm_foliage.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/ov8830/cm_normal.dat:system/etc/camera/davinci/ov8830/cm_normal.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/ov8830/cm_sky.dat:system/etc/camera/davinci/ov8830/cm_sky.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/ov8830/cm_sunset.dat:system/etc/camera/davinci/ov8830/cm_sunset.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/ov8830/imgproc.xml:system/etc/camera/davinci/ov8830/imgproc.xml \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/s5k4e1ga_foxconn/cm_correction.dat:system/etc/camera/davinci/s5k4e1ga_foxconn/cm_correction.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/s5k4e1ga_foxconn/cm_foliage.dat:system/etc/camera/davinci/s5k4e1ga_foxconn/cm_foliage.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/s5k4e1ga_foxconn/cm_normal.dat:system/etc/camera/davinci/s5k4e1ga_foxconn/cm_normal.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/s5k4e1ga_foxconn/cm_sky.dat:system/etc/camera/davinci/s5k4e1ga_foxconn/cm_sky.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/s5k4e1ga_foxconn/cm_sunset.dat:system/etc/camera/davinci/s5k4e1ga_foxconn/cm_sunset.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/s5k4e1ga_foxconn/imgproc.xml:system/etc/camera/davinci/s5k4e1ga_foxconn/imgproc.xml \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx134_liteon/cm_correction.dat:system/etc/camera/davinci/sonyimx134_liteon/cm_correction.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx134_liteon/cm_foliage.dat:system/etc/camera/davinci/sonyimx134_liteon/cm_foliage.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx134_liteon/cm_normal.dat:system/etc/camera/davinci/sonyimx134_liteon/cm_normal.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx134_liteon/cm_sky.dat:system/etc/camera/davinci/sonyimx134_liteon/cm_sky.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx134_liteon/cm_sunset.dat:system/etc/camera/davinci/sonyimx134_liteon/cm_sunset.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx134_liteon/imgproc.xml:system/etc/camera/davinci/sonyimx134_liteon/imgproc.xml \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx134_liteon/luma_boost.dat:system/etc/camera/davinci/sonyimx134_liteon/luma_boost.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx134_liteon/skin_mask_lite.dat:system/etc/camera/davinci/sonyimx134_liteon/skin_mask_lite.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx135_liteon/cm_correction.dat:system/etc/camera/davinci/sonyimx135_liteon/cm_correction.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx135_liteon/cm_foliage.dat:system/etc/camera/davinci/sonyimx135_liteon/cm_foliage.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx135_liteon/cm_normal.dat:system/etc/camera/davinci/sonyimx135_liteon/cm_normal.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx135_liteon/cm_sky.dat:system/etc/camera/davinci/sonyimx135_liteon/cm_sky.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx135_liteon/cm_sunset.dat:system/etc/camera/davinci/sonyimx135_liteon/cm_sunset.dat \
    $(LOCAL_PATH)/prebuilt/etc/camera/davinci/sonyimx135_liteon/imgproc.xml:system/etc/camera/davinci/sonyimx135_liteon/imgproc.xml \
    $(LOCAL_PATH)/prebuilt/etc/camera/lowlight/lowlightcfg.xml:system/etc/camera/lowlight/lowlightcfg.xml \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/ExpMatrixTOA.txt:system/etc/camera/tornado/ExpMatrixTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/FireworksMinMaxTOA.txt:system/etc/camera/tornado/FireworksMinMaxTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/FireworksModelTOA.model:system/etc/camera/tornado/FireworksModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/FoliageMinMaxTOA.txt:system/etc/camera/tornado/FoliageMinMaxTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/FoliageModelTOA.model:system/etc/camera/tornado/FoliageModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/IndoorDstMinMaxTOA.txt:system/etc/camera/tornado/IndoorDstMinMaxTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/IndoorMixModelTOA.model:system/etc/camera/tornado/IndoorMixModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/IndoorMixModelTOA_linear.model:system/etc/camera/tornado/IndoorMixModelTOA_linear.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/IndoorOutdoorModelTOA.model:system/etc/camera/tornado/IndoorOutdoorModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/IndoorOutdoorModelTOA_rbf.model:system/etc/camera/tornado/IndoorOutdoorModelTOA_rbf.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/NightMinMaxTOA.txt:system/etc/camera/tornado/NightMinMaxTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/NightModelTOA.model:system/etc/camera/tornado/NightModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/OutdoorMixModelTOA.model:system/etc/camera/tornado/OutdoorMixModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/OutdoorMixModelTOA_linear.model:system/etc/camera/tornado/OutdoorMixModelTOA_linear.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/scene_debugX.xml:system/etc/camera/tornado/scene_debugX.xml \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/SunsetMinMaxTOA.txt:system/etc/camera/tornado/SunsetMinMaxTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/SunsetModelTOA.model:system/etc/camera/tornado/SunsetModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado/TornadoI.ini:system/etc/camera/tornado/TornadoI.ini \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/ExpMatrixTOA.txt:system/etc/camera/tornado_front/ExpMatrixTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/FireworksMinMaxTOA.txt:system/etc/camera/tornado_front/FireworksMinMaxTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/FireworksModelTOA.model:system/etc/camera/tornado_front/FireworksModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/FoliageMinMaxTOA.txt:system/etc/camera/tornado_front/FoliageMinMaxTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/FoliageModelTOA.model:system/etc/camera/tornado_front/FoliageModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/IndoorDstMinMaxTOA.txt:system/etc/camera/tornado_front/IndoorDstMinMaxTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/IndoorMixModelTOA.model:system/etc/camera/tornado_front/IndoorMixModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/IndoorMixModelTOA_linear.model:system/etc/camera/tornado_front/IndoorMixModelTOA_linear.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/IndoorOutdoorModelTOA.model:system/etc/camera/tornado_front/IndoorOutdoorModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/IndoorOutdoorModelTOA_rbf.model:system/etc/camera/tornado_front/IndoorOutdoorModelTOA_rbf.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/NightMinMaxTOA.txt:system/etc/camera/tornado_front/NightMinMaxTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/NightModelTOA.model:system/etc/camera/tornado_front/NightModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/OutdoorMixModelTOA.model:system/etc/camera/tornado_front/OutdoorMixModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/OutdoorMixModelTOA_linear.model:system/etc/camera/tornado_front/OutdoorMixModelTOA_linear.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/scene_debugX.xml:system/etc/camera/tornado_front/scene_debugX.xml \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/SunsetMinMaxTOA.txt:system/etc/camera/tornado_front/SunsetMinMaxTOA.txt \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/SunsetModelTOA.model:system/etc/camera/tornado_front/SunsetModelTOA.model \
    $(LOCAL_PATH)/prebuilt/etc/camera/tornado_front/TornadoI.ini:system/etc/camera/tornado_front/TornadoI.ini \
    $(LOCAL_PATH)/prebuilt/etc/camera/tracking/targettracking.xml:system/etc/camera/tracking/targettracking.xml \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-C9800D.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-C9800D.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-CEDGE.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-CEDGE.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-T9800L.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-T9800L.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-T9900.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-T9900.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-TEDGE.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-TEDGE.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-U9700LVA.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-U9700LVA.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-U9701L_VA.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-U9701L_VA.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-U9800DNV1.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-U9800DNV1.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-U9900.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-U9900.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-U9900L.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-U9900L.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-UEDGE.bin:system/etc/firmware/ti-connectivity/wl18xx-conf/wl18xx-conf-UEDGE.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-fw.bin:system/etc/firmware/ti-connectivity/wl18xx-fw.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl18xx-fw-mc.bin:system/etc/firmware/ti-connectivity/wl18xx-fw-mc.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/ti-connectivity/wl1271-nvs.bin:system/etc/firmware/ti-connectivity/wl1271-nvs.bin \
    $(LOCAL_PATH)/prebuilt/etc/firmware/fmc_ch8_1893.3.bts:system/etc/firmware/fmc_ch8_1893.3.bts \
    $(LOCAL_PATH)/prebuilt/etc/firmware/fm_rx_ch8_1893.3.bts:system/etc/firmware/fm_rx_ch8_1893.3.bts \
    $(LOCAL_PATH)/prebuilt/etc/firmware/TIInit_12.8.32.bts:system/etc/firmware/TIInit_12.8.32.bts \
    $(LOCAL_PATH)/prebuilt/usr/idc/hisik3_touchscreen.idc:system/usr/idc/hisik3_touchscreen.idc \
    $(LOCAL_PATH)/prebuilt/usr/idc/cyttsp4_mt.idc:system/usr/idc/cyttsp4_mt.idc \
    $(LOCAL_PATH)/prebuilt/usr/idc/k3_keypad.idc:system/usr/idc/k3_keypad.idc \
    $(LOCAL_PATH)/prebuilt/usr/idc/screenovate_keyboard.idc:system/usr/idc/screenovate_keyboard.idc \
    $(LOCAL_PATH)/prebuilt/usr/idc/synaptics.idc:system/usr/idc/synaptics.idc \
    $(LOCAL_PATH)/prebuilt/usr/idc/screenovate_mouse.idc:system/usr/idc/screenovate_mouse.idc \
    $(LOCAL_PATH)/prebuilt/usr/keylayout/k3_keypad.kl:system/usr/keylayout/k3_keypad.kl \
    $(LOCAL_PATH)/prebuilt/usr/keylayout/screenovate_keyboard.kl:system/usr/keylayout/screenovate_keyboard.kl \
    $(LOCAL_PATH)/prebuilt/usr/keychars/screenovate_keyboard.kcm:system/usr/keychars/screenovate_keyboard.kcm


# This device have enough room for precise davick
PRODUCT_TAGS += dalvik.gc.type-precise

# Prime spacific overrides
PRODUCT_PROPERTY_OVERRIDES += \
    ro.product.manufacturer=HUAWEI\
    ro.product.model=P6-U06


# Audio
PRODUCT_PACKAGES += \
    audio.a2dp.default \
    libaudioutils

#PRODUCT_PACKAGES += \
#    FmRadio

# Wifi
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/etc/wifi/wpa_supplicant.conf:/system/etc/wifi/wpa_supplicant.conf \
    $(LOCAL_PATH)/prebuilt/etc/wifi/p2p_supplicant.conf:/system/etc/wifi/p2p_supplicant.conf \
    $(LOCAL_PATH)/prebuilt/etc/wifi/hostapd.conf:/system/etc/wifi/hostapd.conf


# Misc
PRODUCT_PACKAGES += \
    setup_fs \
    librs_jni \
    libsrec_jni \
    com.android.future.usb.accessory \
    make_ext4fs 

# Live Wallpapers
PRODUCT_PACKAGES += \
    LiveWallpapers \
    LiveWallpapersPicker \
    VisualizationWallpapers

PRODUCT_PACKAGES += \
    Camera \
    Stk \
    Torch

# Bluetooth
#PRODUCT_PACKAGES += \
#    uim-sysfs \
#    libbt-vendor

# General
PRODUCT_PROPERTY_OVERRIDES := \
    ro.ril.hsxpa=2 \
    wifi.interface=wlan0 \
    wifi.supplicant_scan_interval=45 \
    sys.usb.state=mtp,adb \
    persist.sys.usb.config=mtp,adb \
    ro.setupwizard.enable_bypass=1

# OpenGL ES
PRODUCT_PROPERTY_OVERRIDES += \
    ro.opengles.version=131072

$(call inherit-product, build/target/product/full.mk)

# call dalvik heap config
$(call inherit-product-if-exists, frameworks/native/build/phone-xhdpi-1024-dalvik-heap.mk)

PRODUCT_BUILD_PROP_OVERRIDES += BUILD_UTC_DATE=0
PRODUCT_NAME := full_hwp6_u06
PRODUCT_DEVICE := hwp6_u06
