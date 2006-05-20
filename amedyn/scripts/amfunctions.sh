#!/bin/bash

remove_controller() {
  usbcon=`lsmod | cut -d' ' -f1 | grep -E "^uhci|usb-ohci|ehci-hcd$"`
  if [ "$usbcon" != "" ]; then
    echo $">>> Removing USB controller..."
    /sbin/rmmod $usbcon
    echo
    sleep 1s
  fi
  echo $">>> Removing $driver..."
  /sbin/rmmod $driver
  echo
  sleep 1s
  echo $">>> Loading again USB controller..."
  /sbin/modprobe $usbcon > /dev/null
  sleep 5s
  echo
}

# Remove module if it is loaded, we only have 1 interface, this is need to load firmware
function remove_module ()
    {
    driver=`lsmod | cut -d' ' -f1 | grep -E "^amedyn|amedyn2|xusbatm$"`
    if [ "$driver" != "" ]; then
        echo $">>> Removing [$driver] driver..."
        /sbin/rmmod $driver || exit 1
        echo
        sleep 1s
    fi
    }

function stop_transfer ()
    {
    if [ "$DRIVER_MODE" == "1" ]; then
        if lsmod | cut -d' ' -f1 | grep -q -E "^amedyn$"; then
            echo $">>> Stopping transfers..."
            amioctl 2
            echo
        fi
    fi
    }

function activate_transfer ()
    {
    if [ "$DRIVER_MODE" == "1" ]; then
        echo $">>> Activating send/receive data..."
        amioctl 1 || exit 1
        sleep 3s
        echo
    fi
    }
