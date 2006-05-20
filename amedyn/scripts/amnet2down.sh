#!/bin/bash

. /etc/amedyn

. /usr/sbin/amfunctions.sh

TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

echo $">>> Down PPPoA network interface <<<"
echo

pid=`pidof pppd`
if [ "$pid" != "" ]; then
  echo $">>> Killing pppd daemon..."
  killall pppd
  echo
fi

stop_transfer
