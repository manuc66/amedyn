#
## Makefile for Zyxel 630-11 & Asus AAM6000UG (install firmware)
#

FIRMWARE_FILES = fw-usb.bin Fw-usb_A.bin
BIN_PATH = /usr/sbin

install: $(FIRMWARE_FILES)
	install -c -m 644 -p $^ $(BIN_PATH)

uninstall:
	for i in $(FIRMWARE_FILES); do \
		rm -f $(BIN_PATH)/$$i; \
	done

