#
## Makefile for bridged control program 
#

# variables
DEST_MACHINE = $(shell uname -m)
DEST_KERNEL = $(shell uname -r)
KERNEL_PATH = /lib/modules/$(DEST_KERNEL)/build
#KERNEL_PATH = /usr/src/linux
CC = gcc
CFLAGS = -O2 -Wstrict-prototypes -fomit-frame-pointer -pipe -Wall -DLINUX -Wsign-compare
CFLAGS = -O2 -Wstrict-prototypes -fomit-frame-pointer -pipe -march=$(DEST_MACHINE) -Wall -DLINUX -Wsign-compare
INCLUDES = -idirafter$(KERNEL_PATH)/include
LDFLAGS = -latm -lresolv
ifndef RPM_BUILD_ROOT
BIN_PATH = /usr/sbin
EXE_FILES = br2684ctl

all: program

program: br2684ctl


#
## entries
#

br2684ctl: br2684ctl.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(EXE_FILES) 

install: install_program 

install_program: $(EXE_FILES)
	install -d $(BIN_PATH)
	install -c -m 755 -p $(EXE_FILES) $(BIN_PATH)

uninstall:
	for i in $(EXE_FILES); do \
		rm -f $(BIN_PATH)/$$i; \
	done

