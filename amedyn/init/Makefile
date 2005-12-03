#
## Makefile for Zyxel 630-11 & Asus AAM6000UG (ADSL Modem USB)
#

# variables
DEST_MACHINE = $(shell uname -m)
CC = gcc
CFLAGS = -O2 -Wstrict-prototypes -fomit-frame-pointer -pipe -Wall -DLINUX -Wsign-compare
INCLUDES = -I../include
LDFLAGS = -lusb -lpopt
LDFLAGS = -lusb 
ifndef RPM_BUILD_ROOT
EXE_FILES = amload amioctl amcheckline
BIN_PATH = /usr/sbin
EXE_FILES = amload amioctl amcheckline amsyncline
EXE_FILES_DEBUG = amloaddbg amloaddbgt
LANGUAGES = es
SHARE_PATH = $(RPM_BUILD_ROOT)/usr/share
SHARE_PATH = /usr/share
TMESG_PATH = /usr/share/locale

ifdef SIMULATE
	CFLAGS += -DSIMULATE
endif

program: amload amioctl amcheckline
all: program

program: amload amioctl amcheckline amsyncline

debug: amloaddbg

debugt: amloaddbgt


#
## Zyxel 630-11 & Asus AAM6000UG entries
#

amfunctions.o: amfunctions.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -c -o $@

amload: amfunctions.o amload.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $^ -o $@

amcheckline: amfunctions.o amcheckline.c
amsyncline: amfunctions.o amsyncline.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $^ -o $@

amioctl: amioctl.c 
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $^ -o $@

amloaddbg: amload.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -DDEBUG $^ -o $@

amloaddbgt: amload.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -DDEBUG -DDEBUG_TRANSFER $^ -o $@

clean:
	rm -f $(EXE_FILES) $(EXE_FILES_DEBUG) amfunctions.o

#install: install_program install_strings
install: install_program 

install_program: $(EXE_FILES)
	install -d $(BIN_PATH)
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


