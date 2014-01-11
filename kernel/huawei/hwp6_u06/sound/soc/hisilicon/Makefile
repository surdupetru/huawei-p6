# HI3620 Platform Support
snd-soc-hi3620-objs := hi3620-pcm.o hi3620-digital.o

# HI3620-HI6421 Machine Support
snd-soc-hi3620-hi6421-objs := hi3620_hi6421.o

obj-$(CONFIG_SND_SOC_PCM_HI3620) += snd-soc-hi3620.o
obj-$(CONFIG_SND_HI3620_HI6421) += snd-soc-hi3620-hi6421.o

ifeq ($(CONFIG_K3_HDMI),y)
EXTRA_CFLAGS := -DHDMI_DISPLAY
endif
