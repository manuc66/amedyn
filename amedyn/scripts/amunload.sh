#!/bin/bash

. /etc/amedyn
TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

MODULE_NAME=amedyn
MODULE_NAMEDBG=amedyndbg
IOCTL_NAME=amioctl

remove_controller() {
  usbcon=`lsmod | cut -d' ' -f1 | grep -E "^uhci|usb-ohci|ehci-hcd$"`
  if [ "$usbcon" != "" ]; then
    echo $">>> Removing USB controller..."
    rmmod $usbcon
    echo
    sleep 1s
  fi
  echo $">>> Removing $driver..."
  rmmod $driver
  echo
  sleep 1s
  echo $">>> Loading again USB controller..."
  modprobe $usbcon > /dev/null
  sleep 5s
  echo
}

echo $">>> Remove $MODULE_NAME/$MODULE_NAMEDBG driver <<<"
echo

remove_module
#if lsmod | cut -d' ' -f1 | grep -q -E "^$MODULE_NAME|$MODULE_NAMEDBG$"; then
#  echo $">>> Closing ADSL line..."
#  ($IOCTL_NAME 5 && echo && sleep 4s)
#fi
driver=`lsmod | cut -d' ' -f1 | grep -E "^$MODULE_NAME|$MODULE_NAMEDBG$"`

driver=`lsmod | cut -d' ' -f1 | grep -E "^$MODULE_NAME|xusbatm|$MODULE_NAMEDBG$"`
if [ "$driver" != "" ]; then
  echo $">>> Removing $driver..."
  (rmmod $driver && echo && sleep 1s ) || remove_controller
fi

