#!/bin/bash

TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

. /etc/amedyn

echo $">>> Inits Zyxel 630-11 & Asus AAM6000UG <<<"
echo

FLOAD_NAME=amload
MODULE_NAME=amedyn
MODULE_NAMEDBG=amedyndbg

# Load usb host controller if is not loaded
KERNEL_VERSION=`uname -r | cut -d'.' -f1-2`
if [ "$KERNEL_VERSION" = "2.4" ]; then
  usbcon=`lsmod | cut -d ' ' -f1 | grep -E "^uhci|usb-uhci|usb-ohci|ehci-hcd$"`
  if [ "$usbcon" = "" ]; then
    echo $">>> Loading USB controller..."
    modprobe uhci > /dev/null || modprobe usb-ohci > /dev/null || modprobe ehci-hcd > /dev/null 
    sleep 5s
    echo
  fi
else
  usbcon=`lsmod | cut -d ' ' -f1 | grep -E "^uhci_hcd|ohci_hcd|ehci_hcd$"`
  if [ "$usbcon" = "" ]; then
    echo $">>> Loading USB controller..."
    modprobe uhci-hcd
    modprobe ohci-hcd
    modprobe ehci-hcd
    sleep 5s
    echo
  fi
fi

# Mount USB file systems if is not mounted
mt_old=`mount -t usbdevfs`
mt_new=`mount -t usbfs`
if [ "$mt_old" = "" ] && [ "$mt_new" = "" ]; then
  echo $">>> Mounting USB file system..."
  mount -t usbfs usbfs /proc/bus/usb || mount -t usbdevfs none /proc/bus/usb || exit 1
  echo
fi

# Remove module if it is loaded, we only have 1 interface, this is need to load firmware
driver=`lsmod | cut -d' ' -f1 | grep -E "^$MODULE_NAME|$MODULE_NAMEDBG$"`
if [ "$driver" != "" ]; then
  echo $">>> Removing amedyn driver..."
  rmmod $driver || exit 1
  echo
  sleep 1s
fi

# Load firmware
echo $">>> Loading firmware..."
$FLOAD_NAME || exit 1


# Wait processor (?)
#sleep 5s
#echo

# Load Zyxel 630-11 & Asus AAM6000UG module
echo $">>> Loading driver..."

if [ "$KERNEL_VERSION" != "2.4" ]; then
  crc32=`lsmod | cut -d ' ' -f1 | grep -E "^crc32$"`
  if [ "$crc32" = "" ]; then
    modprobe crc32
  fi
fi

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
modprobe $MODULE_RUN || exit 1
sleep 3s 

echo
echo $0 $"successful"

