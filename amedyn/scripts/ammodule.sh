#!/bin/bash

. /etc/amedyn

. /usr/sbin/amfunctions.sh

KERNEL_VERSION=`uname -r | cut -d'.' -f1-2`

if [ "$1" = "1" ]; then

    # Load Zyxel 630-11 & Asus AAM6000UG module
    echo $">>> Loading driver..."
    
    if [ "$KERNEL_VERSION" != "2.4" ]; then
	    crc32=`lsmod | cut -d ' ' -f1 | grep -E "^crc32$"`
	    if [ "$crc32" = "" ]; then
	        /sbin/modprobe -q crc32
	        /sbin/modprobe crc32
	    fi
    fi

    case "$DRIVER_MODE" in
        1)
            # amedyn driver:
            echo $"Launching driver amedyn...";
            MODULE_RUN="amedyn"
            ;;

        2)
            # amedyn2 driver:
            echo $"Launching driver amedyn2...";
            MODULE_RUN="amedyn2 linetype=$LINE_TYPE"
            ;;

        3)
            # xusbatm driver:
            echo $"Launching generic driver xusbatm..."
            MODULE_RUN="xusbatm vendor=$VENDOR product=$PRODUCT rx_endpoint=$RX_ENDPOINT tx_endpoint=$TX_ENDPOINT rx_altsetting=$RX_ALTSETTING tx_altsetting=$TX_ALTSETTING"
            ;;
	*)
	    # unknown value:
	    echo $"DRIVER_MODE: Unknown value."
	    echo $"Check /etc/amedyn"
	    exit 1
	    ;;
    esac
   
    /sbin/modprobe $MODULE_RUN || exit 1
    sleep 3s
else
    remove_module 
fi 
