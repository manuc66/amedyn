ifndef KERNELDIR
	KERNELDIR  := /lib/modules/$(shell uname -r)/build
endif

obj-m := usbatm.o amedyn2.o
obj-m := usbatm.o amedyn2.o xusbatm.o

all help modules modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $(filter-out all,$(MAKECMDGOALS)) EXTRA_CFLAGS=-DDEBUG

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

nodebug:
	make -C $(KERNELDIR) M=$(PWD)

install: uninstall_amedyn
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install
	/sbin/depmod -ae

uninstall_amedyn:
	rm -f /lib/modules/$(shell uname -r)/extra/amedyn.ko
	/sbin/depmod -ae

uninstall_amedyn2:
	rm -f /lib/modules/$(shell uname -r)/extra/amedyn2.ko
	/sbin/depmod -ae

uninstall_usbatm:
	rm -f /lib/modules/$(shell uname -r)/extra/usbatm.ko
	/sbin/depmod -ae

uninstall_usb_atm:
	rm -f /lib/modules/$(shell uname -r)/extra/usb_atm.ko
	/sbin/depmod -ae

uninstall_xusbatm:
	rm -f /lib/modules/$(shell uname -r)/extra/xusbatm.ko
	/sbin/depmod -ae

uninstall_old: uninstall_amedyn uninstall_usb_atm

uninstall: uninstall_old uninstall_amedyn2 uninstall_usbatm
keep_xusbatm: uninstall_old uninstall_amedyn2

uninstall: uninstall_old uninstall_amedyn2 uninstall_usbatm uninstall_xusbatm
