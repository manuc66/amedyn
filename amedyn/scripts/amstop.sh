#!/bin/bash

TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

. /etc/amedyn

if [ "$RESYNC" = "1" ] && [ "$1" != "amline" ] ; then
fi

if [ "$PROTOCOL_MODE" = "" ]; then
  echo $"Error: PROTOCOL_MODE not defined" 1>&2
  exit 1
else
  if [ "$PROTOCOL_MODE" -eq 1 ]; then
    echo $"Stopping RFC1483/2684 routed..." 
    amnetdown.sh || exit 1 
  else
    if [ "$PROTOCOL_MODE" -eq 2 ]; then
      echo $"Stopping PPP over ATM..."
      amnet2down.sh || exit 1 
    else
      if [ "$PROTOCOL_MODE" -eq 3 ]; then
        echo $"Stopping RFC1483/2684 bridged..."
        amnet3down.sh || exit 1 
      else
        if [ "$PROTOCOL_MODE" -eq 4 ]; then
          echo $"Stopping PPP over Ethernet..."
          amnet4down.sh || exit 1 
        else
          echo $"Error: unknow protocol mode" 1>&2
          exit 1
        fi
      fi
    fi
  fi
fi

sleep 7s
amunload.sh || exit 1

if [ "$1" = "service" ]; then
  rm -f /var/lock/subsys/amedyn
fi

