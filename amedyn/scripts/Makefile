#
## Makefile for Zyxel 630-11 & Asus AAM6000UG (install scripts)
#

BIN_PATH = /usr/sbin
CONFIG_PATH = /etc
CONFIG_FILE = amedyn
SERVICE_FILE = amedyn
SHARE_PATH = /usr/share
TMESG_PATH = /usr/share/locale
LANG=
#LANGUAGES = es ca
LANGUAGES = es

generic_scripts = amload.sh amunload.sh amstart.sh amstop.sh
routed_scripts = amnetup.sh amnetdown.sh
pppoa_scripts = amnet2up.sh amnet2down.sh
bridged_scripts = amnet3up.sh amnet3down.sh
pppoe_scripts = amnet4up.sh amnet4down.sh
all_scripts = $(generic_scripts) $(routed_scripts) $(pppoa_scripts) $(bridged_scripts) $(pppoe_scripts)

#install: generic config_file routed pppoa install_strings
install: generic config_file routed pppoa bridged pppoe

generic: $(generic_scripts) 
	install -c -m 755 -p $^ $(BIN_PATH)
	install -c -m 755 -p service/$(SERVICE_FILE) /etc/$(SERVICE_FILE).service

config_file: config/$(CONFIG_FILE)
	if [ ! -f $(CONFIG_PATH)/$(CONFIG_FILE) ]; then \
		install -c -m 644 -p $^ $(CONFIG_PATH); \
	fi

routed: $(routed_scripts) 
	install -c -m 755 -p $^ $(BIN_PATH) 

pppoa: $(pppoa_scripts)
	install -c -m 755 -p $^ $(BIN_PATH)

bridged: $(bridged_scripts)
	install -c -m 755 -p $^ $(BIN_PATH)

pppoe: $(pppoe_scripts)
	install -c -m 755 -p $^ $(BIN_PATH)

uninstall:
	for i in $(all_scripts); do \
		rm -f $(BIN_PATH)/$$i; \
	done
	rm -f /etc/$(SERVICE_FILE).service;
#	for i in $(LANGUAGES); do \
# 		for j in $(all_scripts); do \
#			rm -f $(TMESG_PATH)/$$i/LC_MESSAGES/$$j.mo; \
#		done; \
#	done

add_service: service/$(SERVICE_FILE)
	install -c -m 755 -p $^ /etc/$(SERVICE_FILE).service
	if [ -f /etc/debian_version ]; then \
		install -c -m 755 -p $^ /etc/init.d/$(SERVICE_FILE); \
		update-rc.d $(SERVICE_FILE) defaults; \
		update-rc.d -f atm remove; \
	else \
		if [ -f /etc/redhat-release ]; then \
			install -c -m 755 -p $^ /etc/rc.d/init.d/$(SERVICE_FILE); \
			chkconfig --add $(SERVICE_FILE); \
		else \
			if [ -f /etc/SuSE-release ] ; then \
				install -c -m 755 -p $^ /etc/init.d/$(SERVICE_FILE); \
				install -c -m 755 -p $^ /etc/rc.d/$(SERVICE_FILE); \
				insserv $(SERVICE_FILE); \
			else \
				if [ -f /etc/slackware-version ] ; then \
					if grep -q -E "^ *$(BIN_PATH)/amstart.sh *$$" /etc/rc.d/rc.local; then \
						cd /etc/rc.d; \
						cp -f rc.local rc.local.tmp; \
						echo "$(BIN_PATH)/amstart.sh" >> rc.local.tmp; \
						mv -f rc.local.tmp rc.local; \
					fi \
				fi \
			fi \
		fi \
	fi

remove_service:
	if [ -f /etc/debian_version ]; then \
		update-rc.d -f $(SERVICE_FILE) remove; \
		rm -f /etc/init.d/$(SERVICE_FILE); \
	else \
		if [ -f /etc/redhat-release ]; then \
			chkconfig --del $(SERVICE_FILE); \
			rm -f /etc/rc.d/init.d/$(SERVICE_FILE); \
		else \
			if [ -f /etc/SuSE-release ] ; then \
				insserv -r $(SERVICE_FILE); \
				rm -f /etc/init.d/$(SERVICE_FILE); \
				rm -f /etc/rc.d/$(SERVICE_FILE); \
			else \
				if [ -f /etc/slackware-version ] ; then \
					cd /etc/rc.d; \
					grep -v -E "^ *$(BIN_PATH)/amstart.sh *$$" rc.local > rc.local.tmp; \
					mv -f rc.local.tmp rc.local; \
				fi \
			fi \
		fi \
	fi

extract_strings: $(all_scripts)
	for i in $^; do \
		bash --dump-po-strings $$i > po/$$i.po; \
	done

compile_strings: $(all_scripts)
	if [ "$(LANG)" = "" ]; then \
		echo "Error, type: make -e LANG=language_code $@" 1>&2; \
		echo "Example: make -e LANG=es $@" 1>&2; \
		exit 1; \
	fi
	for i in $^; do \
		msgfmt -o locale/$(LANG)/LC_MESSAGES/$$i.mo po/$(LANG)/$$i.po; \
	done

install_strings:
	for i in $(LANGUAGES); do \
		if [ ! -d $(TMESG_PATH)/$$i/LC_MESSAGES ]; then \
			mkdir -p $(TMESG_PATH)/$$i/LC_MESSAGES; \
		fi; \
 		for j in locale/$$i/LC_MESSAGES/*; do \
			install -c -m 644 -p $$j $(SHARE_PATH)/`dirname $$j`; \
		done; \
	done

