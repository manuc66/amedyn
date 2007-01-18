#!/bin/bash

function user_name()
{
    user=""
    while [ -z "$user" ]; do
	echo -n "Type in your user name (given by your provider): "
	read user
    done
}

function provider()
{
    provider=""
    while [ -z "$provider" ]; do
	echo -n "Type in your provider name (by example tiscali.be): "
	read provider
    done
}


function password()
{
    pwdmatch=0
    while [ $pwdmatch -eq 0 ]; do
	stty -echo
	password1=""
	while [ -z "$password1" ]; do
	    echo -n "Type in your password (given by your provider): "
	    read password1
	    echo
	done
	password2=""
	while [ -z "$password2" ]; do
	    echo -n "Type in your password again (for verification): "
	    read password2
	    echo
	done
	stty echo
	if [ "$password1" == "$password2" ]; then
	    pwdmatch=1
	else
	    echo -e "** passwords don't match, try again
"
	fi
    done
}

function dns()
{
    dns1=""
    while [ -z "$dns1" ]; do
	echo -n "Type in an IP for DNS1: "
	read dns1
	echo $dns1 | grep -E "^([0-9]{1,3}\.){3}[0-9]{1,3}$" > /dev/null 2>&1
	if [ $? -ne 0 ]; then
	    dns1=""
	    echo -e "** invalid IP for DNS1, please retry
"
	fi
    done
    echo
    dns2=""
    while [ -z "$dns2" ]; do
	echo -n "Type in an IP for DNS2: "
	read dns2
	echo $dns2 | grep -E "^([0-9]{1,3}\.){3}[0-9]{1,3}$" > /dev/null 2>&1
	if [ $? -ne 0 ]; then
	    dns2=""
	    echo -e "** invalid IP for DNS2, please retry
"
	fi
    done
}

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

function pppoatm()
{

  pppoatm=""

  pppoatm=$(find /usr/lib/pppd | grep pppoatm | head -n1)

}

function summary()
{
    echo "==== Configuration will be created with these values :"
    echo
    echo "  + User          : $user"
    echo "  + Password      : (hidden)"
    echo "  + Provider      : $provider"
#    echo "      DNS 1       : $dns1"
#    echo "      DNS 2       : $dns2"
    echo "  + pppoatm       : $pppoatm"
    echo "      VPI/VCI       : $vpi/$vci"
}

function secret()
{
    for auth in "chap" "pap"
      do
      secretfile="./$auth-secrets"
      if [ -s $secretfile ]
	  then
	  backup "$secretfile"

	  echo -n "modifying $secretfile.. "
	  TMP=$(echo "$username" | sed "s/\\/\\\\\\/g")
	  grep -v -E "^[ \t]*\"?$TMP\"?[ \t]*.*
?" $backupfile > "$secretfile"
      else
	  echo -n "creating $secretfile.. "
	  echo "# Secrets for authentication using $auth" > "$secretfile"
      fi
      echo -e "\"$user\"\t*\t\"$password1\"\t*" >> "$secretfile"
      echo "OK"
    done
}

function write_config()
{
  sed -e "s:pppoatm:$pppoatm:g" -e "s/VCI/$vci/g" -e "s/VPI/$vpi/g" -e "s/user_name/$user/g" -e "s/provider/$provider/g" pppoatm_options > options
  sed -e "s/user_name/$user/g" -e "s/provider/$provider/g" -e "s/password/$password1/g" pppoatm_pap-secrets > pap-secrets
  sed -e "s/user_name/$user/g" -e "s/provider/$provider/g" -e "s/password/$password1/g" pppoatm_chap-secrets > chap-secrets
}

echo
echo "Enter now your VPI/VCI (depending on your provider/country)"
echo "Example for BELGIUM: 8 35  (VPI=8, VCI=35)"
echo "These values correspond to the number dialed under Windows."
echo
vpi
vci
user_name
provider
password
clear
pppoatm
summary
write_config

cp pap-secrets /etc/pap-secrets
cp chap-secrets /etc/chap-secrets
cp options /etc/options
rm pap-secrets chap-secrets options
echo "DONE!"
