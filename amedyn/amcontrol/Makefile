#
## Makefile for Zyxel 630-11 & Asus AAM6000UG (ADSL Modem USB)
#

# variables
DEST_MACHINE = $(shell uname -m)
CC = gcc
CFLAGS = -O2 -Wstrict-prototypes -fomit-frame-pointer -pipe -march=$(DEST_MACHINE) -Wall -DLINUX -Wsign-compare
INCLUDES = -I../include
LDFLAGS = -lusb 
BIN_PATH = /usr/sbin
EXE_FILES = amcontrol
EXE_FILES_DEBUG = amcontroldbg amcontroldbgt
LANGUAGES = es
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


amcontrol: amcontrol.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -lcurses $^ -o $@


amloaddbg: amcontrol.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -lcurses -DDEBUG $^ -o $@

amloaddbgt: amcontrol.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -lcurses -DDEBUG -DDEBUG_TRANSFER $^ -o $@

clean:
	rm -f $(EXE_FILES) $(EXE_FILES_DEBUG) 

#install: install_program install_strings
install: install_program 

install_program: $(EXE_FILES)
	install -c -m 755 -p $(EXE_FILES) $(BIN_PATH)

uninstall:
	for i in $(EXE_FILES); do \
		rm -f $(BIN_PATH)/$$i; \
	done
#	for i in $(LANGUAGES); do \
#		rm -f $(TMESG_PATH)/$$i/LC_MESSAGES/3cload.mo; \
#		rm -f $(TMESG_PATH)/$$i/LC_MESSAGES/3cioctl.mo; \
#	done

#extract_strings: 3cload.c
#	xgettext -o 3cload.po -p po --omit-header 3cload.c
#	xgettext -o 3cioctl.po -p po --omit-header 3cioctl.c

#compile_strings:
#	for i in $(LANGUAGES); do \
#		for j in po/$$i/*.po; do \
#			msgfmt -o locale/$$i/LC_MESSAGES/`basename $$j | sed "s/\(.*\)\.[^\.]*$$/\1/"`.mo $$j; \
#		done; \
#	done;

#install_strings:
#	for i in $(LANGUAGES); do \
#		if [ ! -d $(TMESG_PATH)/$$i/LC_MESSAGES ]; then \
#                        mkdir -p $(TMESG_PATH)/$$i/LC_MESSAGES; \
#                fi; \
#                for j in locale/$$i/LC_MESSAGES/*.mo; do \
#                        install -c -m 644 -p $$j $(SHARE_PATH)/`dirname $$j`; \
#                done; \
#	done


