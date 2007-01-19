#!/bin/bash


function vpi()
{
    vpi=""
    while [ -z "$vpi" ]; do
	echo -n "Type in your VPI: "
	read vpi
	echo $vpi | grep -E "^[0-9]+$" > /dev/null 2>&1
	if [ $? -ne 0 ]; then
	    vpi=""
	    echo -e "** invalid VPI, please enter it again
"
	else
	    if [ $vpi -ge 4096 ]; then
		vpi=""
		echo -e "** VPI not in range [0-4095], please enter it again
"
	    fi
	fi
    done
}

function vci()
{
    vci=""
    while [ -z "$vci" ]; do
	echo -n "Type in your VCI: "
	read vci
	echo $vci | grep -E "^[0-9]+$" > /dev/null 2>&1
	if [ $? -ne 0 ]; then
	    vci=""
	    echo -e "** invalid VCI, please enter it again
"
	else
	    if [ $vci -ge 65536 ]; then
		vci=""
		echo -e "** VCI not in range [0-65535], please enter it again
"
	    fi
	fi
    done
}

function protocol()
{
  while [ !$(true) ]
    do
    echo -e "
-------------------------------------"
    echo " Protocol Mode Menu "
    echo "-------------------------------------"
    echo "[1] RFC1483/2684 routed"
    echo "[2] PPP over ATM (pppoa)"
    echo "[3] RFC1483/2684 bridged"
    echo "[4] PPP over Ethernet (pppoe)"
    echo "======================="
    echo -n "Enter your menu choice. [1-4]: "
    read PROTOCOL_MODE
    case $PROTOCOL_MODE in
      1)
	  echo "RFC1483/2684 routed"
	  break
	  ;;

      2)
	  echo "PPP over ATM (pppoa)"
	  break
	  ;;

      3)
	  echo "RFC1483/2684 bridged"
	  break
	  ;;

      4)
	  echo "PPP over Ethernet (pppoe)"
	  break
	  ;;

      *)
	  echo "Opps!!! Please select choice 1,2,3,4";
#	  echo "Press a key. . ." ;
	  read
	  ;;
    esac
  done
}



function drivermode()
{
  while [ !$(true) ]
    do
    echo -e "
-------------------------------------"
    echo " Driver Mode Menu "
    echo "-------------------------------------"
    echo "[1] amload + amedyn : Old classical userspace driver (mainly for 2.4 kernels)"
    echo "[2] amedyn2 + usbatm : New kernel space driver (experimental, see public aid request on website)"
    echo "[3] amload + xusbatm : Updated generic driver (actualy the best solution for last 2.6 kernels)"
    echo "======================="
    echo -n "Enter your menu choice. [1-3]: "
    read DRIVER_MODE
    case $DRIVER_MODE in
      1)
	  echo "amedyn"
	  break
	  ;;

      2)
	  echo "amedyn2"
	  break
	  ;;

      3)
	  echo "xusbatm"
	  break
	  ;;

      *)
	  echo "Opps!!! Please select choice 1,2,3";
	  read
	  ;;
    esac
  done
}

function linetype()
{
  while [ !$(true) ]
    do
    echo -e "
-------------------------------------"
    echo " Line Type Menu "
    echo "-------------------------------------"
    echo "[1] Analog line"
    echo "[2] ISDN line"
    echo "======================="
    echo -n "Enter your menu choice. [1-2]: "
    read linetype
    case $linetype in
      1)
	  echo "Analog"
	  line=0x15
	  break
	  ;;

      2)
	  echo "ISDN"
	  line=0x11
	  break
	  ;;

      *)
	  echo "Opps!!! Please select choice 1,2";
	  read
	  ;;
    esac
  done
}


function modem()
{
  echo "Please connect your modem if it's not already done ? (press any key when ready)"
  read -n1

  modem=$(cat /proc/bus/usb/devices | grep "Vendor=0b05 ProdID=6206")
  if [ $? -eq 0 ]; then
    vid=0x0b05
    pid=0x6206
  else
    modem=$(cat /proc/bus/usb/devices | grep "Vendor=06b9 ProdID=a5a5")
    if [ $? -eq 0 ]; then
      vid=0x06b9
      pid=0xa5a5
    else
      modem=$(cat /proc/bus/usb/devices | grep "Vendor=1767 ProdID=0005")
      if [ $? -eq 0 ]; then
        vid=0x1767
        pid=0x0005
      fi
    fi
  fi

  if [ -n $vid ]; then
    echo "Modem found! ($vid:$pid)"
    return
  fi


  while [ !$(true) ]
    do
    echo -e "
-------------------------------------"
    echo " The modem was not found please select one : "
    echo "-------------------------------------"
    echo "[1] 0x06b9/0xa5a5 : Zyxel Prestige 630-11, Zyxel Prestige 630-13, Topcom Webracer 851, ..."
    echo "[2] 0x0b05/0x6206 : Asus AAM600UG"
    echo "[3] 0x1767/0x0005 : Medi@com 103/MADSLU"
    echo "======================="
    echo -n "Enter your menu choice. [1-3]: "
    read modemtype
    case $modemtype in
      1)
	  echo "0x06b9/0xa5a5"
	  vid=0x06b9
	  pid=0xa5a5
	  break
	  ;;

      2)
	  echo "0x0b05/0x6206"
	  vid=0x0b05
	  pid=0x6206
	  break
	  ;;

      3)
	  echo "0x1767/0x0005"
	  vid=0x1767
	  pid=0x0005
	  break
	  ;;

      *)
	  echo "Opps!!! Please select choice 1,2 or 3";
	  read
	  ;;
    esac
  done
}

clear

echo -e "
===== Welcome to the driver configuration  tool for the Alcatel Dynamite USB modem ship based =====
"
echo -e "At any time, press Ctrl+C to quit this script without saving modifications.
"
echo -n "Do you want to set your connection setting by using this script (Y/n) ? "
read -n1 ans

echo -e "

"
if [[ "$ans" = "n" || "$ans" = "N" ]]; then
    exit 0
fi

resync=0
NETMASK=255.255.255.0

modem
linetype
protocol
drivermode


if [[ $PROTOCOL_MODE -eq 1 || $PROTOCOL_MODE -eq 3 || $PROTOCOL_MODE -eq 4 ]]; then
vpi
vci
fi

if [[ "$PROTOCOL_MODE" = 1 || "$PROTOCOL_MODE" = 3 ]];
    then
    echo -e "
-------------------------------------"
    echo " For or RFC1483/2684 routed/bridged "
    echo -e "-------------------------------------
"
    echo -n "Enter IP address (If you left it blank in bridged mode, then it will use DHCP to get IP) : "
    read IP_ADDRESS
    echo -n "Enter the network mask (If you left if blank 255.255.255.0 will bu used) : "
    read NETMASK
    if [ "$NETMASK" = "" ]
	then
	NETMASK=255.255.255.0
    fi
    echo -n "Enter the gateway IP  : "
    read GATEWAY
fi

echo -e "
Writing configuration to file... "

while [ $(echo "


#
# Config file for Zyxel 630-11 & Asus AAM6000UG (ADSL Modem USB)
#

# Line type
# 0x15 = ANALOG
# 0x11 = ISDN
LINE_TYPE=\"$line\"

# Driver mode
# 1 = amedyn
# 2 = amedyn2
# 3 = xusbatm
DRIVER_MODE=$DRIVER_MODE

# Protocol
# 1 = RFC1483/2684 routed
# 2 = PPP over ATM (pppoa)
# 3 = RFC1483/2684 bridged
# 4 = PPP over Ethernet (pppoe)
PROTOCOL_MODE=$PROTOCOL_MODE

# xusbatm
VENDOR=\"$vid\"  # 0x06b9 / 0x0b05 / 0x1767
PRODUCT=\"$pid\" # 0xa5a5 / 0x6206 / 0x0005
RX_ENDPOINT=\"0x87\"
TX_ENDPOINT=\"0x07\"
RX_ALTSETTING=1
TX_ALTSETTING=1

# Paths
BINARY_PATH=\"/usr/sbin\"
ATM_PATH=\"\"

# ATM
VPI=$vpi
VCI=$vci

# Specific for RFC1483/2684 routed/bridged
#  if IP_ADDRESS is blank in bridged mode then it uses DHCP to get IP
IP_ADDRESS=$IP_ADDRESS
NETMASK=$NETMASK
GATEWAY=$GATEWAY

# === Re-sync modem line if line goes down
# - don't resync on line down = 0
# - resync when line goes down = 1
RESYNC=$resync

" > /etc/amedyn) ]
  do
  echo -e "
ERROR writing config to file"
  echo -n "Try again ? (Y/n) : "
  read -n1 ans
  if [[ "$ans" = "n" || "$ans" = "N" ]]
      then
      exit -1
  fi
done

echo "done"

if [[ "$PROTOCOL_MODE" = 2 ]]; then
  echo -n "Would you like to set connection settings ? (Y/n) : "
  read -n1 ans
  echo -e "



"
  if [[ "$ans" = "n" || "$ans" = "N" ]]
      then
      exit -1
  fi
  ./pppoatm_config.sh
fi


