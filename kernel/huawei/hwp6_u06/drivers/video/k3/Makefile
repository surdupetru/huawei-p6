obj-$(CONFIG_FB_K3_CLCD) := k3fb.o
#obj-m := k3fb.o

k3fb-objs := edc_reg.o \
	ldi_reg.o \
	mipi_reg.o \
	sbl_reg.o \
	k3_fb_panel.o \
	edc_overlay.o \
	mipi_dsi_host.o \
	k3_fb.o \
	ldi.o \
	mipi_dsi.o \
	backlight_pwm.o \
	panel/ldi_samsung_LMS350DF04.o \
	panel/mipi_sharp_LS035B3SX.o \
	panel/mipi_samsung_S6E39A.o \
	panel/mipi_toshiba_MDW70_V001.o \
	panel/mipi_panasonic_VVX10F002A00.o \
	panel/mipi_cmi_OTM1280A.o \
	panel/mipi_jdi_OTM1282B.o \
	panel/mipi_cmi_PT045TN07.o \
	panel/mipi_toshiba_MDY90.o

EXTRA_CFLAGS += -Iarch/arm/mach-k3v2 \
		-Idrivers/video/k3 \
		-Idrivers/video/k3/panel

ifeq ($(CONFIG_K3_HDMI),y)
EXTRA_CFLAGS += -DHDMI_DISPLAY
endif

clean:
	rm *.o .*cmd
