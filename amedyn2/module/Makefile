ifndef KERNELDIR
	KERNELDIR  := /lib/modules/$(shell uname -r)/build
endif

obj-m := usbatm.o amedyn2.o
obj-m := usb_atm.o usbatm.o amedyn.o
obj-m := usb_atm.o amedyn.o
obj-m := usb_atm.o amedyn.o usbatm.o amedyn2.o
obj-m := usbatm.o amedyn2.o
obj-m := usbatm.o amedyn2.o xusbatm.o

all help modules modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $(filter-out all,$(MAKECMDGOALS)) EXTRA_CFLAGS=-DDEBUG

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

nodebug:
	make -C $(KERNELDIR) M=$(PWD)

install:
install: uninstall_amedyn
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install
	/sbin/depmod -ae

uninstall:
	rm -f /lib/modules/$(shell uname -r)/extra/amedyn.ko
	rm -f /lib/modules/$(shell uname -r)/extra/usb_atm.ko
uninstall_amedyn:
	rm -f /lib/modules/$(shell uname -r)/extra/amedyn.ko
uninstall_usbatm:
	rm -f /lib/modules/$(shell uname -r)/extra/usbatm.ko
	rm -f /lib/modules/$(shell uname -r)/extra/amedyn2.ko
uninstall_xusbatm:
	rm -f /lib/modules/$(shell uname -r)/extra/xusbatm.ko
	/sbin/depmod -ae
keep_xusbatm: uninstall_old uninstall_amedyn2

uninstall: uninstall_old uninstall_amedyn2 uninstall_usbatm uninstall_xusbatm
