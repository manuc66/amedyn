 #!/bin/bash


 TEXTDOMAIN=`basename $0`
 #if [ -d "./locale" ]; then
 #  TEXTDOMAINDIR="./locale"
 #fi


 . /etc/amedyn


 # For PPPoE
 echo $">>> Setting PPPoE <<<"
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


 # Now, we can send & receive data
 echo $">>> Activating send/receive data..."
 amioctl 1 || exit 1
 sleep 3s
 echo 


 # Load br2684 kernel module
 brmod=`lsmod | cut -d' ' -f1 | grep -q -E "^br2684$"`
 if [ "$brmod" = "" ]; then
   echo $">>> Loading br2684 kernel module..."
 # No exit if error, module can be inserted in kernel
   modprobe br2684
   echo
 fi


 echo $">>> Loading ppp_generic..."
 # No exit if error, module can be inserted in kernel
 modprobe ppp_generic
 echo


 PPPOE=`which pppoe 2>/dev/null`
 if [ "$PPPOE" = "" ]; then
   echo $">>> Loading pppoe..."
   modprobe pppoe || exit 1
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
 ifconfig nas0 up || exit 1
 sleep 3s


 # Remove default Route
 if route -n | cut -d' ' -f1 | grep -q "0.0.0.0"; then
   echo $">>> Removing default route..."
   route del default || exit 1
   echo
 fi


 echo $">>> Loading pppd daemon..."
 if [ "$PPPOE" = "" ]; then
   pppd || exit 1
 else
   if [ -x /usr/bin/pon ]; then
     pon dsl-provider || exit 1
   else
     adsl-start || exit 1
   fi
 fi
 echo


 echo $0 $"successful"