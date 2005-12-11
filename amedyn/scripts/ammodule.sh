#!/bin/bash

. /etc/amedyn

. /usr/sbin/amfunctions.sh
MODULE_NAME=amedyn
MODULE_NAMEDBG=amedyndbg

KERNEL_VERSION=`uname -r | cut -d'.' -f1-2`

if [ "$1" = "1" ]; then

    case "$DRIVER_MODE" in
	1)
          # normal mode:
	    echo $"Launching driver in normal mode...";
	    MODULE_RUN=$MODULE_NAME
	    ;;
	
	2)
           # debug mode:
	    echo $"Launching driver in debug mode...";
	    MODULE_RUN=$MODULE_NAMEDBG
	    ;;

    esac
    
    # Load Zyxel 630-11 & Asus AAM6000UG module
    echo $">>> Loading driver..."
    
    if [ "$KERNEL_VERSION" != "2.4" ]; then
	    crc32=`lsmod | cut -d ' ' -f1 | grep -E "^crc32$"`
	crc32=`lsmod | cut -d ' ' -f1 | grep -E "^crc32$"`
	if [ "$crc32" = "" ]; then
	    modprobe crc32
	fi
	    fi
    fi
    
    modprobe $MODULE_RUN || exit 1
   
    /sbin/modprobe $MODULE_RUN || exit 1
    sleep 3s
    driver=`lsmod | cut -d' ' -f1 | grep -E "^$MODULE_NAME|$MODULE_NAMEDBG$"`
    if [ "$driver" != "" ]; then
	echo $">>> Removing amedyn driver..."
	rmmod $driver || exit 1
	echo
	sleep 1s
    fi
else
    remove_module 
fi 
