#!/bin/bash

. /etc/amedyn
TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

. /etc/amedyn

# For RFC1483/2684 routed protocols
echo $">>> Setting RFC1483/2684 routed <<<"
echo

if [ "$ATM_PATH" = "" ]; then
  if [ -x /usr/local/sbin/atmarp ]; then
    ATM_PATH=/usr/local/sbin
  else
    if [ -x /usr/sbin/atmarp ]; then
      ATM_PATH=/usr/sbin
    fi
  fi
fi

# Load ATMARP daemon
pid=`pidof atmarpd`
if [ "$pid" = "" ]; then 
  echo $">>> Loading atmarpd..."
  $ATM_PATH/atmarpd -b -l syslog -m || exit 1
  sleep 3s
  echo
fi

# Create interface
if ! grep -q "atm0" /proc/net/dev; then
  echo $">>> Creating interface..."
  $ATM_PATH/atmarp -c atm0 || exit 1
  echo
fi

# Activate interface
echo $">>> Activating interface..."
ifconfig atm0 $IP_ADDRESS netmask $NETMASK up || exit 1
sleep 3s
echo 

# Remove default route
if route -n | cut -d' ' -f1 | grep -q "0.0.0.0"; then
  echo $">>> Removing default route..."
  route del default || exit 1
  echo
fi

# Now, we can send & receive data
activate_transfer
echo $">>> Activating send/receive data..."
amioctl 1 || exit 1
sleep 3s
echo

# Create a PVC, link IP_ADDRESS with INTERFACE, VPI, VCI
echo $">>> Creating PVC..."
$ATM_PATH/atmarp -s $GATEWAY 0.$VPI.$VCI || exit 1
echo

# Add default route
echo $">>> Adding default route..."
route add default gw $GATEWAY || exit 1
echo

echo $0 $"successful"
 
