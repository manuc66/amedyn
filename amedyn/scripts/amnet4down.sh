#!/bin/bash

. /etc/amedyn
TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

echo $">>> Down PPPoE network interface <<<"
echo

stop_transfer
if lsmod | cut -d' ' -f1 | grep -q -E "^amedyn|amedyndbg$"; then 
  echo $">>> Stopping transfers..."
  amioctl 2
  echo
fi

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
    adsl-stop
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

