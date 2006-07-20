#
# Makefile to build and install all files for Zyxel 630-11 & Asus AAM6000UG
#

RPMBIN="rpmbuild"
ifeq ($(shell [ -f /etc/debian_version ] && echo "yes"), yes)
	DISTRO = "deb"
else
ifeq ($(shell [ -f /etc/mandrake-release ] && echo "yes"), yes)
	DISTRO = ""
else
ifeq ($(shell [ -f /etc/redhat-release ] && echo "yes"), yes)
	DISTRO = "rh"
else
ifeq ($(shell [ -f /etc/SuSE-release ] && echo "yes"), yes)
	DISTRO = "suse"
	RPMBIN = "rpm"
else
	DISTRO = ""
endif
endif
endif
endif

DEST_MACHINE = $(shell uname -m)
DEST_KERNEL = $(shell uname -r)
DEST_KERNEL_MAIN = $(shell uname -r | cut -d'.' -f1-2)
KERNEL_PATH = /lib/modules/$(DEST_KERNEL)/build
CONFIG_PATH = /usr/src/linux
CONFIG_NAME = .config
MODULE_PATH = /lib/modules/$(DEST_KERNEL)/kernel/drivers/usb
NAME = amedyn-$(shell date +%Y-%m-%d)
RPM_BIN_NAME = ${NAME}.k${DEST_KERNEL}${DISTRO}.${DEST_MACHINE}.rpm
TGZ_SRC_NAME = ${NAME}-src.tgz
ifeq ($(DEST_KERNEL_MAIN), 2.4)
MODULE_NORMAL = $(MODULE_PATH)/amedyn.o
MODULE_DEBUG = $(MODULE_PATH)/amedyndbg.o
else
MODULE_NORMAL = $(MODULE_PATH)/amedyn.ko
MODULE_DEBUG = $(MODULE_PATH)/amedyndbg.ko
endif
RPM_VERSION = $(shell date +%Y_%m_%d)_$(shell echo $(DEST_KERNEL) | tr "-" "_")
RPM_RELEASE = 1

all: clean compile

#install: CX_INIT CX_FIRMWARE CX_MODULE CX_SCRIPTS BRIDGED CX_PANEL
compile: userspace modules

userspace: AME_INIT AME_CONTROL

modules: AME_MODULE

install: userspace_install module_install
install: userspace_install modules_install

userspace_install:
	make -C init install        
	make -C scripts        
	make -C bridged install        
	make -C amcontrol install        
   
module_install: module
	make -C module install 

AME_INIT: init
	cd init && make

AME_CONTROL:
	cd amcontrol && make

AME_MODULE: module
	cd module && make normal

AME_SCRIPTS: scripts
	cd scripts && make

BRIDGED: bridged
	cd bridged && make

package: clean
	cd .. && tar -zcvf ${TGZ_SRC_NAME} amedyn

deb: clean compile_install
	mv -f /etc/amedyn /etc/amedyn.config
	cp -pf scripts/config/amedyn /etc/amedyn
	cd debian && ./create-pkg.sh ${DEST_MACHINE}
	mv -f /etc/amedyn.config /etc/amedyn

rpm: clean compile_install
	mv -f /etc/amedyn /etc/amedyn.config
	cp -pf scripts/config/amedyn /etc/amedyn
	${RPMBIN} -bb \
	         --define "_rpmdir ${PWD}" \
	         --define "_builddir ${PWD}" \
	         --define "_arch ${DEST_MACHINE}" \
	         --define "__check_files ''" \
	         --define "_rpmfilename ${RPM_BIN_NAME}" \
	         --define "module_normal ${MODULE_NORMAL}" \
	         --define "module_debug ${MODULE_DEBUG}" \
	         --define "version ${RPM_VERSION}" \
	         --define "release ${RPM_RELEASE}" \
	         amedyn.spec
	mv ${RPM_BIN_NAME} ..
	mv -f /etc/amedyn.config /etc/amedyn

#uninstall: firmware init module scripts panel bridged
uninstall: init module scripts bridged
	cd init && make uninstall
	cd module && make uninstall
	cd scripts && make uninstall
	cd bridged && make uninstall

#clean: init module sarlib panel bridged
clean: init module bridged amcontrol
	cd init && make clean
	cd module && make clean
	cd bridged && make clean
	cd amcontrol && make clean

add_service: scripts/Makefile
	cd scripts && make add_service

remove_service: scripts/Makefile
	cd scripts && make remove_service

help:
	echo "Type:"
	echo "  make -> To compile driver for your system"
	echo "  make -e DEST_MACHINE=i586 -> To compile driver for i586 machine"
	echo "  make rpm -> To build a rpm package for your system (it will be created in previous directory)"
	echo "  make -e DEST_MACHINE=i586 rpm -> To build a rpm package for i586 machine"
	echo "  make deb -> To build a deb package for your system (it will be created in previous directory)"
	echo "  make package -> To build a tgz package (it will be created in previous directory)"
	echo "  make uninstall -> To remove installed files"
	echo "  make add_service -> To install driver as a service (you don't need type amstart.sh)"
	echo "  make remove_service -> To remove driver service"
