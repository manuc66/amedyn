#
## Makefile for USB ADSL Modem
#

DEST_KERNEL = $(shell uname -r)
DEST_KERNEL_MAIN = $(shell uname -r | cut -d'.' -f1-2)
KERNEL_PATH = /lib/modules/$(DEST_KERNEL)/build
#KERNEL_PATH = /usr/src/linux-2.6.0-0.test4.1.33
MODULE_PATH = /lib/modules/$(DEST_KERNEL)/kernel/drivers/usb
# You need remove .config file from $(KERNEL_PATH) if you use CONFIG_PATH different of KERNEL_PATH or if you use CONFIG_NAME different of .config
CONFIG_PATH = $(KERNEL_PATH)
#CONFIG_PATH = /usr/src/linux-2.6.0-0.test4.1.33/configs
CONFIG_NAME = .config
#CONFIG_NAME = kernel-2.6.0-test4-i686.config 

xdslusb-module := amedyn
xdslusb-crc := speedcrc

ifeq ($(DEST_KERNEL_MAIN), 2.4)
module-extension = .o
xdslusb-module-objs := xdslusb_2.4.o
xdslusb-files := xdslusb_2.4.c $(xdslusb-crc).c $(xdslusb-crc).h Makefile
else
module-extension = .ko
xdslusb-files := xdslusb_2.6.c $(xdslusb-crc).c $(xdslusb-crc).h Makefile
xdslusb-module-objs := xdslusb_2.6.o
endif

xdslusb-module-name := $(xdslusb-module)$(module-extension)
xdslusb-debug-module-name := $(xdslusb-module)dbg$(module-extension)

ifneq ($(KERNELRELEASE),)

ifndef CONFIG_USB
$(error USB support not turned on in the kernel!)
endif

ifndef CONFIG_ATM
$(error ATM support not turned on in the kernel!)
endif

ifeq ($(PATCHLEVEL), 4)
	obj-y := $(xdslusb-module-objs) $(xdslusb-crc).o
	obj-m	:= $(XDSLUSB-MODULE).o
	O_TARGET := $(XDSLUSB-MODULE).o
	include $(TOPDIR)/Rules.make
else
	obj-m := $(XDSLUSB-MODULE).o
	$(XDSLUSB-MODULE)-objs := $(xdslusb-module-objs)
endif

else

	KDIR := $(KERNEL_PATH)
	PWD := $(shell pwd)

all: normal debug

normal: $(xdslusb-files)
	rm -f $(xdslusb-module-objs)
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) XDSLUSB-MODULE=$(xdslusb-module) modules
#	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) XDSLUSB-MODULE=$(xdslusb-module) EXTRA_CFLAGS="-Wsign-compare" modules
#	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) MAKEFILES=$(CONFIG_PATH)/$(CONFIG_NAME) XDSLUSB-MODULE=$(xdslusb-module) modules
#	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) V=1 modules

debug: $(xdslusb-files)
	rm -f $(xdslusb-module-objs)
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) XDSLUSB-MODULE=$(xdslusb-module)dbg EXTRA_CFLAGS="-DDEBUG -DVERBOSE_DEBUG" modules
#	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) XDSLUSB-MODULE=$(xdslusb-module)dbg EXTRA_CFLAGS="-DDEBUG -DVERBOSE_DEBUG -Wsign-compare" modules
#	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) MAKEFILES=$(CONFIG_PATH)/$(CONFIG_NAME) XDSLUSB-MODULE=$(xdslusb-module)dbg EXTRA_CFLAGS="-DDEBUG -DVERBOSE_DEBUG" modules

install: $(xdslusb-module-name) $(xdslusb-debug-module-name) 
	install -d $(MODULE_PATH)
	install -c $(xdslusb-module-name) $(MODULE_PATH)
	install -c $(xdslusb-debug-module-name) $(MODULE_PATH)
	depmod -a

uninstall:
	rm -f $(MODULE_PATH)/$(xdslusb-module-name)
	rm -f $(MODULE_PATH)/$(xdslusb-debug-module-name)

clean:
	rm -f *.o .*.flags *.ko *.mod.* .*.o.cmd .*.ko.cmd

endif

