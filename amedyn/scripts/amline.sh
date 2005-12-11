#!/bin/bash

. /etc/amedyn

if [ $PROTOCOL_MODE -ge 1 ] && [ $PROTOCOL_MODE -le 4 ]; then
    if [ $PROTOCOL_MODE -eq 1 ]; then
	amnet=amnet
    else
	amnet=amnet${PROTOCOL_MODE}
    fi
fi

while [ 1 ]; do
# check line status
    amcheckline || exit 1
# line has gone down : stop the driver
    amstop.sh amline || exit 1
# try to sync line
    amsyncline || exit 1
# line is now up : restart the driver
    ammodule.sh 1 || exit 1
    ${amnet}up.sh || exit 1
done
