#!/bin/bash

TEXTDOMAIN=`basename $0`
#if [ -d "./locale" ]; then
#  TEXTDOMAINDIR="./locale"
#fi

. /etc/amedyn

amload.sh || exit 1

if [ "$1" = "service" ]; then
  if [ -d /var/lock/subsys ]; then
    touch /var/lock/subsys/amedyn
  fi
fi

if [ "$PROTOCOL_MODE" = "" ]; then
  echo $"Error: PROTOCOL_MODE not defined" 1>&2
  exit 1
else
  if [ "$PROTOCOL_MODE" -eq 1 ]; then
    echo $"Setting RFC1483/2684 routed..." 
    amnetup.sh || exit 1
  else
    if [ "$PROTOCOL_MODE" -eq 2 ]; then
      echo $"Setting PPP over ATM..."
      amnet2up.sh || exit 1
    else
      if [ "$PROTOCOL_MODE" -eq 3 ]; then
        echo $"Setting RFC1483/2684 bridged..."
        amnet3up.sh || exit 1
      else
        if [ "$PROTOCOL_MODE" -eq 4 ]; then
          echo $"Setting PPP over Ethernet..."
          amnet4up.sh || exit 1
        else
          echo $"Error: unknow protocol mode" 1>&2
          exit 1
        fi
      fi
    fi
  fi
fi

