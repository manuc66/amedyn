#!/bin/bash

. /etc/amedyn

. /usr/sbin/amfunctions.sh

TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

echo $">>> Down RFC1483/2684 routed network interface <<<"
echo

pid=`pidof atmarpd`
if [ "$pid" != "" ]; then
  echo $">>> Killing atmarpd daemon..."
  killall atmarpd
  echo
fi

if grep -q "atm0" /proc/net/dev; then
  echo $">>> Shutting down atm0 interface..."
  ifconfig atm0 down
  echo
fi

stop_transfer
