#!/bin/bash

. /etc/amedyn

. /usr/sbin/amfunctions.sh

TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

echo $">>> Down PPPoE network interface <<<"
echo

stop_transfer

PPPOE=`which pppoe 2>/dev/null`
if [ "$PPPOE" = "" ]; then
  pid=`pidof pppd`
  if [ "$pid" != "" ]; then
    echo $">>> Killing pppd daemon..."
    killall pppd
    echo
  fi
else
  if [ -x /usr/bin/poff ]; then
    poff dsl-provider 
  else
    if [ -x /usr/bin/adsl-stop ]; then
	adsl-stop
    else
	pppoe-stop
    fi
  fi
fi

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

