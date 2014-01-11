obj-m += pppoe_dzc.o
pppoe_dzc-objs += pppoe_dzc_api.o pppoe_kernel.o

KDIR := ${OUT}/obj/KERNEL_OBJ/
include $(KDIR).config
   
PWD := $(shell pwd)


all:
	$(RM) -rf $(PWD)/scripts
	ln -s $(KDIR)/scrips $(PWD)/scripts
	cp pppoe_dzc_api.dzc pppoe_dzc_api.o
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-eabi- $(KDIR).config modules
	rm -rf .tmp_versions
	rm *.mod.c
	rm *.o
	rm .*.cmd
	rm *.symvers
	rm *.order

install:all
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
  
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
