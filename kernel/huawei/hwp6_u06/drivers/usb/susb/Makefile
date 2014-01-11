#
# Makefile for DWC_otg Highspeed USB controller driver
#

DWCPATH=$(PWD)/drivers/usb/susb/dwc_common_port

# Use the BUS_INTERFACE variable to compile the software for either
# PCI(PCI_INTERFACE) or LM(LM_INTERFACE) bus.
ifeq ($(BUS_INTERFACE),)
	BUS_INTERFACE = -DLM_INTERFACE
endif

#CPPFLAGS	+= -DDEBUG

# Use one of the following flags to compile the software in host-only or
# device-only mode.
#CPPFLAGS        += -DDWC_HOST_ONLY
#CPPFLAGS        += -DDWC_DEVICE_ONLY

CPPFLAGS	+= -Dlinux -DDWC_HS_ELECT_TST
#CPPFLAGS	+= -DDWC_EN_ISOC
CPPFLAGS   	+= -I$(DWCPATH)
CPPFLAGS   	+= -I$(PWD)
#CPPFLAGS   	+= -I$(PORTLIB)
CPPFLAGS   	+= -DDWC_LINUX
CPPFLAGS   	+= $(CFI)
CPPFLAGS	+= $(BUS_INTERFACE)
#CPPFLAGS	+= -DDWC_DEV_SRPCAP


#KBUILD_CPPFLAGS	+= -DDEBUG

# Use one of the following flags to compile the software in host-only or
# device-only mode.
#KBUILD_CPPFLAGS        += -DDWC_HOST_ONLY
#KBUILD_CPPFLAGS        += -DDWC_DEVICE_ONLY

KBUILD_CPPFLAGS		+= -Dlinux -DDWC_HS_ELECT_TST
#KBUILD_CPPFLAGS	+= -DDWC_EN_ISOC
KBUILD_CPPFLAGS   	+= -I$(DWCPATH)
#KBUILD_CPPFLAGS   	+= -I$(PORTLIB)
KBUILD_CPPFLAGS   	+= -DDWC_LINUX
KBUILD_CPPFLAGS   	+= $(CFI)
KBUILD_CPPFLAGS		+= $(BUS_INTERFACE)
#KBUILD_CPPFLAGS		+= -DDWC_DEV_SRPCAP

obj-y	:= dwc_otg.o

dwc_otg-objs	:= dwc_otg_driver.o dwc_otg_attr.o
dwc_otg-objs	+= dwc_otg_cil.o dwc_otg_cil_intr.o dwc_otg_core_param.o
dwc_otg-objs	+= dwc_otg_pcd_linux.o dwc_otg_pcd.o dwc_otg_pcd_intr.o
dwc_otg-objs	+= dwc_otg_hcd.o dwc_otg_hcd_linux.o dwc_otg_hcd_intr.o dwc_otg_hcd_queue.o dwc_otg_hcd_ddma.o
dwc_otg-objs	+= dwc_otg_adp.o
dwc_otg-objs	+= hiusb_android.o

ifneq ($(CFI),)
dwc_otg-objs	+= dwc_otg_cfi.o
endif

clean:
	rm -rf   *.o *.ko .*cmd *.mod.c .tmp_versions Module.symvers
