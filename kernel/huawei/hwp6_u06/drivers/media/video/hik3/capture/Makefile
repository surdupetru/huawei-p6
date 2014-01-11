#
# Makefile for hisilicon K3 soc camera v4l2 driver
# Author: m55267
#
EXTRA_CFLAGS += -Idrivers/media/video/hik3/capture/isp
EXTRA_CFLAGS += -fno-pic


k3_capture-objs := isp/cam_util.o \
		   isp/cam_dbg.o \
		   isp/k3_v4l2_capture.o \
		   isp/k3_isp.o \
		   isp/k3_isp_io.o \
		   isp/k3_ispv1.o \
		   isp/k3_ispv1_afae.o \
		   isp/k3_ispv1_tune_ops.o \
		   isp/k3_ispv1_io.o \
		   isp/sensor_common.o \
		   isp/k3_ispv1_hdr_movie_ae.o



obj-$(CONFIG_VIDEO_HIK3_CAMERA) += k3_capture.o

#obj-$(CONFIG_HIK3_CAMERA_OV5642) += ov5642/

obj-$(CONFIG_HIK3_CAMERA_OV8830) += ov8830/

obj-$(CONFIG_HIK3_CAMERA_SONYIMX105) += sonyimx105/

obj-$(CONFIG_HIK3_CAMERA_S5K3H2YX_FOXCONN) += s5k3h2yx_foxconn/

obj-$(CONFIG_HIK3_CAMERA_S5K3H2YX_SAMSUNGEM) += s5k3h2yx_samsungem/

obj-$(CONFIG_HIK3_CAMERA_MT9M114) += mt9m114/

obj-$(CONFIG_HIK3_CAMERA_FLASH) += flash/

obj-$(CONFIG_HIK3_CAMERA_SONYIMX091) += sonyimx091/

obj-$(CONFIG_HIK3_CAMERA_S5K5CAG) += s5k5cag/

obj-$(CONFIG_HIK3_CAMERA_SONYIMX135) += sonyimx135/

obj-$(CONFIG_HIK3_CAMERA_SONYIMX188) += sonyimx188/

obj-$(CONFIG_HIK3_CAMERA_SONYIMX134) += sonyimx134/

obj-$(CONFIG_HIK3_CAMERA_HI542) += hi542/

obj-$(CONFIG_HIK3_CAMERA_S5K4E1GA_FOXCONN) += s5k4e1ga_foxconn/
