#!/bin/bash

#variables
TARGET=/tmp/amedyn-deb-pkg
VERSION=`date +%Y-%m-%d`_`uname -r`
MAINTAINER="Emmanuel Counasse <manuc66[a]yahoo.fr>"
DATE=`date -R`
if [ "$1" = "" ]; then
  DEST_MACHINE=`uname -m`
else
  DEST_MACHINE=$1
fi
FILE_CONTROL="$TARGET"/DEBIAN/control
FILE_COPYRIGHT="$TARGET"/DEBIAN/copyright
FILE_CHANGELOG="$TARGET"/DEBIAN/changelog
FILE_CONFFILES="$TARGET"/DEBIAN/conffiles
FILE_POSTINST="$TARGET"/DEBIAN/postinst
FILE_PRERM="$TARGET"/DEBIAN/prerm
SOURCE_BIN=/usr/sbin
SOURCE_MODULE=/lib/modules/`uname -r`/kernel/drivers/usb
PACKAGE_NAME=../../amedyn-`date +%Y-%m-%d`.k`uname -r`."$DEST_MACHINE".deb
SHLIBDEPS=`dpkg-shlibdeps -O $SOURCE_BIN/amload $SOURCE_BIN/amioctl $SOURCE_BIN/br2684ctl | sed 's/shlibs:Depends=//g'` 


#
#create DEBIAN directory and create files to build deb package
#

rm -rf $TARGET
mkdir -p $TARGET/DEBIAN

#Architecture: $DEST_MACHINE changed by Architecture: i386

cat > $FILE_CONTROL << EOF
Source: amedyn
Section: contrib/net
Priority: optional
Maintainer: $MAINTAINER
Package: amedyn
Architecture: i386
Version: $VERSION
Depends: $SHLIBDEPS
Description: ASUS AAM600UG AME/ALC & Zyxel 630-11  USB ADSL modem kernel space driver
 This package contains software to use alcatel based ship under Linux. It supports several protocols.
EOF

cat > $FILE_COPYRIGHT << EOF
$DATE

It was downloaded from: http://sourceforge.net/projects/zyxel630-11 or http://sourceforge.net/projects/aam6000ug/

Upstream author: $MAINTAINER

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

On Debian systems, the complete text of the GNU General Public
License can be found in /usr/share/common-licenses/GPL file.
EOF

cat > $FILE_CHANGELOG << EOF
amedyn ($VERSION) unstable; urgency=low

  * Maintaining the Debian package myself.

 -- $MAINTAINER  $DATE 
EOF

cat > $FILE_CONFFILES << EOF
/etc/amedyn
EOF

cat > $FILE_POSTINST << EOF
#!/bin/bash
/sbin/depmod -a
if [ -f /etc/debian_version ]; then 
  install -c -m 755 -p /etc/amedyn.service /etc/init.d/amedyn; 
  update-rc.d amedyn defaults; 
  update-rc.d -f atm remove; 
else 
  if [ -f /etc/redhat-release ]; then 
    install -c -m 755 -p /etc/amedyn.service /etc/rc.d/init.d/amedyn; 
    chkconfig --add amedyn; 
  else 
    if [ -f /etc/SuSE-release ] ; then 
      install -c -m 755 -p /etc/amedyn.service /etc/init.d/amedyn; 
      install -c -m 755 -p /etc/amdedyn.service /etc/rc.d/amedyn; 
      insserv amedyn; 
    else 
      if [ -f /etc/slackware-version ] ; then 
        if grep -q -E "^ */usr/sbin/amstart.sh *$" /etc/rc.d/rc.local; then 
          cd /etc/rc.d; 
          cp -f rc.local rc.local.tmp; 
          echo "/usr/sbin/amstart.sh" >> rc.local.tmp; 
          mv -f rc.local.tmp rc.local; 
        fi 
      fi 
    fi 
  fi 
fi
EOF

cat > $FILE_PRERM << EOF
#!/bin/bash
if [ -f /etc/debian_version ]; then 
        update-rc.d -f amedyn remove; 
        rm -f /etc/init.d/amedyn; 
else 
        if [ -f /etc/redhat-release ]; then 
                chkconfig --del amedyn; 
                rm -f /etc/rc.d/init.d/amedyn; 
        else 
                if [ -f /etc/SuSE-release ] ; then 
                        insserv -r amedyn; 
                        rm -f /etc/init.d/amedyn; 
                        rm -f /etc/rc.d/amedyn; 
                else 
                        if [ -f /etc/slackware-version ] ; then 
                                cd /etc/rc.d; 
                                grep -v -E "^ */usr/sbin/amstart.sh *$" rc.local > rc.local.tmp; 
                                mv -f rc.local.tmp rc.local; 
                        fi 
                fi 
        fi 
fi
EOF

chmod 644 $FILE_CONTROL
chmod 644 $FILE_COPYRIGHT
chmod 644 $FILE_CHANGELOG
chmod 644 $FILE_CONFFILES
chmod 755 $FILE_POSTINST
chmod 755 $FILE_PRERM


#
#copy driver files in temp directory for deb package
#

mkdir -p $TARGET$SOURCE_BIN
mkdir -p $TARGET$SOURCE_MODULE
mkdir -p $TARGET/etc

#init
cp -p $SOURCE_BIN/amload $TARGET$SOURCE_BIN 
cp -p $SOURCE_BIN/amioctl $TARGET$SOURCE_BIN

#firmware
cp -p $SOURCE_BIN/Fw-usb_A.bin $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/fw-usb.bin $TARGET$SOURCE_BIN


#module
KERNEL_VERSION=`uname -r | cut -d'.' -f1-2`
if [ "$KERNEL_VERSION" = "2.4" ]; then
  cp -p $SOURCE_MODULE/amedyn.o $TARGET$SOURCE_MODULE
  cp -p $SOURCE_MODULE/amedyndbg.o $TARGET$SOURCE_MODULE
else
  cp -p $SOURCE_MODULE/amedyn.ko $TARGET$SOURCE_MODULE
  cp -p $SOURCE_MODULE/amedyndbg.ko $TARGET$SOURCE_MODULE
fi

#scripts
cp -p $SOURCE_BIN/amstart.sh $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/amstop.sh $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/amload.sh $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/amunload.sh $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/amnetup.sh $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/amnet2up.sh $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/amnet3up.sh $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/amnet4up.sh $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/amnetdown.sh $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/amnet2down.sh $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/amnet3down.sh $TARGET$SOURCE_BIN
cp -p $SOURCE_BIN/amnet4down.sh $TARGET$SOURCE_BIN

#config
cp -p /etc/amedyn $TARGET/etc

#service
cp -p /etc/amedyn.service $TARGET/etc

#bridged
cp -p $SOURCE_BIN/br2684ctl $TARGET$SOURCE_BIN

#panel
#cp -p $SOURCE_BIN/cxpanel $TARGET$SOURCE_BIN
#cp -p $SOURCE_BIN/cxpanel.glade $TARGET$SOURCE_BIN
#cp -p $SOURCE_BIN/cxacru-tux.xpm $TARGET$SOURCE_BIN


#
#build deb package
#

dpkg -b $TARGET $PACKAGE_NAME 
rm -rf $TARGET

