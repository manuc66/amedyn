#
# Alcatel dynamite usb modem spec file to build rpm package
#

Summary: Zyxel 630-11 and Asus aam6000ug USB ADSL modem kernel space driver
Name: amedyn
Version: %{version}
Release: %{release}
Copyright: GPL
Group: System/Kernel and hardware
Source: http://sourceforge.net/projects/zyxel630-11/
URL: http://sourceforge.net/projects/zyxel630-11/
BuildArch: %{_arch}
#Distribution: My distribution
#Vendor: No available
#Packager: Josep Comas <jcomas@gna.es>
#Prefix: %{_prefix}
#BuildRoot: %{_builddir}/%{name}

%description
Zyxel 630-11 and Asus aam6000ug USB ADSL modem kernel space driver. This
package contains software to use Dynamite usb ADSL modem under Linux. It supports several protocols.

#%prep
#%setup -n %{name}

#%build
#make clean
#make

#%install
#make install

%files
%defattr(755, root, root)

#init
/usr/sbin/amload
/usr/sbin/amioctl

#firmware
%attr(644, root, root) /usr/sbin/Fw-usb_A.bin
%attr(644, root, root) /usr/sbin/fw-usb.bin

#module
%{module_normal}
%{module_debug}

#scripts
/usr/sbin/amstart.sh
/usr/sbin/amstop.sh
/usr/sbin/amload.sh
/usr/sbin/amunload.sh
/usr/sbin/amnetup.sh
/usr/sbin/amnet2up.sh
/usr/sbin/amnet3up.sh
/usr/sbin/amnet4up.sh
/usr/sbin/amnetdown.sh
/usr/sbin/amnet2down.sh
/usr/sbin/amnet3down.sh
/usr/sbin/amnet4down.sh

%config %attr(644, root, root) /etc/amedyn

#service
/etc/amedyn.service

#bridged
/usr/sbin/br2684ctl

#panel
#/usr/sbin/cxpanel
#%attr(644, root, root) /usr/sbin/cxpanel.glade
#%attr(644, root, root) /usr/sbin/cxacru-tux.xpm

#falta doc

%post
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
      install -c -m 755 -p /etc/amedyn.service /etc/rc.d/amedyn;
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

%preun
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

%define date %(echo `LC_ALL="C" date +"%a %b %d %Y"`)
%changelog
* %{date} Josep Comas <jcomas@gna.es>
- Maintaining the rpm package myself.

