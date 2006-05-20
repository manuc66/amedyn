#!/bin/bash

. /etc/amedyn

. /usr/sbin/amfunctions.sh

TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

echo $">>> Inits Zyxel 630-11 & Asus AAM6000UG <<<"
echo

# Load usb host controller if is not loaded
KERNEL_VERSION=`uname -r | cut -d'.' -f1-2`
if [ "$KERNEL_VERSION" = "2.4" ]; then
  usbcon=`lsmod | cut -d ' ' -f1 | grep -E "^uhci|usb-uhci|usb-ohci|ehci-hcd$"`
  if [ "$usbcon" = "" ]; then
    echo $">>> Loading USB controller..."
    /sbin/modprobe uhci > /dev/null || /sbin/modprobe usb-ohci > /dev/null || /sbin/modprobe ehci-hcd > /dev/null 
    sleep 5s
    echo
  fi
else
  usbcon=`lsmod | cut -d ' ' -f1 | grep -E "uhci_hcd|ohci_hcd|ehci_hcd$"`
  if [ "$usbcon" = "" ]; then
    echo $">>> Loading USB controller..."
    /sbin/modprobe uhci-hcd
    /sbin/modprobe ohci-hcd
    /sbin/modprobe ehci-hcd
    sleep 5s
    echo
  fi
fi

# Mount USB file systems if is not mounted
mt_old=`mount -t usbdevfs`
mt_new=`mount -t usbfs`
if [ "$mt_old" = "" ] && [ "$mt_new" = "" ]; then
  echo $">>> Mounting USB file system..."
  mount -t usbfs usbfs /proc/bus/usb || mount -t usbdevfs none /proc/bus/usb
  mount -t usbfs usbfs /proc/bus/usb || mount -t usbdevfs none /proc/bus/usb || exit 1
  echo
fi

remove_module

# Load firmware
if [ "$DRIVER_MODE" == "1" -o "$DRIVER_MODE" == "3" ]; then
    echo $">>> Loading firmware..."
    amload || exit 1
    amload -fcs || exit 1
    amload -fcs --linetype $LINE_TYPE || exit 1
    amload $LOADPRMS || exit 1
fi

# Wait processor (?)
sleep 5s
echo

ammodule.sh 1 || exit 1

echo
echo $0 $"successful"
