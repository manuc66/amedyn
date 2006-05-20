#!/bin/bash

. /etc/amedyn

. /usr/sbin/amfunctions.sh

TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

echo $">>> Down RFC1483/2684 bridged network interface <<<"
echo

DHCPPROC=$(ps x | grep nas0\$)
if [ "$DHCPPROC" != "" ]; then
  DHCPCMD=$(echo $DHCPPROC | cut -d' ' -f5)
  if [ "$DHCPCMD" = "dhclient" ] || [ "$DHCPCAD" = "dhcpcd" ] || [ "$DHCPCMD" = "pump" ]; then
    echo $">>> Stopping DHCP client..."
    kill $(echo $DHCPPROC | cut -d' ' -f1)
    echo
  fi
fi

stop_transfer

if grep -q "nas0" /proc/net/dev; then
  echo $">>> Shutting down nas0 interface..."
  ifconfig nas0 down
  echo
fi

pid=`pidof br2684ctl`
if [ "$pid" != "" ]; then
  echo $">>> Killing br2684ctl daemon..."
  killall br2684ctl
  echo
fi

