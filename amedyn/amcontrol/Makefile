#
## Makefile for Zyxel 630-11 & Asus AAM6000UG (ADSL Modem USB)
#

# variables
DEST_MACHINE = $(shell uname -m)
CC = gcc
CFLAGS = -O2 -Wstrict-prototypes -fomit-frame-pointer -pipe -Wall -DLINUX -Wsign-compare
INCLUDES = -I../include
LDFLAGS = -lusb -lncurses
LDFLAGS = -lusb 
ifndef RPM_BUILD_ROOT
BIN_PATH = /usr/sbin
EXE_FILES = amcontrol
EXE_FILES_DEBUG = amcontroldbg amcontroldbgt
LANGUAGES = es
SHARE_PATH = $(RPM_BUILD_ROOT)/usr/share
SHARE_PATH = /usr/share
TMESG_PATH = /usr/share/locale

ifdef SIMULATE
	CFLAGS += -DSIMULATE
endif

all: program

program: amcontrol

debug: amcontroldbg 

debugt: amcontroldbgt


#
## Zyxel 630-11 & Asus AAM6000UG entries
#


	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -lcurses $^ -o $@
amcontrol: amcontrol.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $^ -o $@
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -lcurses -DDEBUG $^ -o $@
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -lcurses -DDEBUG -DDEBUG_TRANSFER $^ -o $@
