#!/bin/bash

TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

. /etc/amedyn

# For RFC1483/2684 bridged protocols
echo $">>> Setting RFC1483/2684 bridged <<<"
echo

if [ "$ATM_PATH" = "" ]; then
  if [ -x /usr/local/sbin/br2684ctl ]; then
    ATM_PATH=/usr/local/sbin
  else
    if [ -x /usr/sbin/br2684ctl ]; then
      ATM_PATH=/usr/sbin
    fi
  fi
fi

# Load br2684 kernel module
brmod=`lsmod | cut -d' ' -f1 | grep -q -E "^br2684$"`
if [ "$brmod" = "" ]; then
  echo $">>> Loading br2684 kernel module..."
# No exit if error, module can be inserted in kernel
  modprobe br2684 
  echo
fi

# Load ATM daemon
pid=`pidof br2684ctl`
if [ "$pid" = "" ]; then 
  echo $">>> Loading br2684ctl..."
  $ATM_PATH/br2684ctl -b -c 0 -a 0.$VPI.$VCI || exit 1
  sleep 3s
  echo
fi

# Activate interface
echo $">>> Activating interface..."
if [ "$IP_ADDRESS" = "" ]; then
  ifconfig nas0 up || exit 1
else
  ifconfig nas0 $IP_ADDRESS netmask $NETMASK up || exit 1
fi
sleep 3s
echo 

# Add default route
if [ "$GATEWAY" != "" ]; then
  echo $">>> Adding default route..."
  if route -n | cut -d' ' -f1 | grep -q "0.0.0.0"; then
    route del default || exit 1
  fi
  route add default gw $GATEWAY || exit 1
  echo
fi

# Now, we can send & receive data
echo $">>> Activating send/receive data..."
amioctl 1 || exit 1
sleep 3s
echo 

# If no static address, use DHCP
if [ "$IP_ADDRESS" = "" ]; then
  echo $">>> Requesting IP address from DHCP server..."
  DHCLIENT=dhclient
  if ! which $DHCLIENT >/dev/null; then
    DHCLIENT=dhcpcd
  fi
  if ! which $DHCLIENT >/dev/null; then
    DHCLIENT=pump
    DHCOPT="-i"
  fi
  $DHCLIENT $DHCOPT nas0 || exit 1
  echo 
fi

echo $0 $"successful"
 
