#!/bin/bash

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

if lsmod | cut -d' ' -f1 | grep -q -E "^amedyn|amedyndbg$"; then
  echo $">>> Stopping transfers..."
  amioctl 2
  echo
fi

