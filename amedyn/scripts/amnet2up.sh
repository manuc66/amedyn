#!/bin/bash

. /etc/amedyn

. /usr/sbin/amfunctions.sh

TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

# For PPPoA
echo $">>> Setting PPPoA <<<"
echo

echo $">>> Loading ppp_generic..."
# No exit if error, module can be inserted in kernel
/sbin/modprobe ppp_generic 
echo

echo $">>> Loading pppoatm..."
# No exit if error, module can be inserted in kernel
/sbin/modprobe pppoatm 
echo

activate_transfer

echo $">>> Loading pppd daemon..."
pppd || exit 1
echo

echo $0 $"successful"
 
