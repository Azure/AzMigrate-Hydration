#!/bin/bash

##+----------------------------------------------------------------------------------+
##            Copyright(c) Microsoft Corp. 2015
##+----------------------------------------------------------------------------------+
## File         :   fix_dhcp.sh
##
## Description  :   
##
## History      :   10-12-2018 (Venu Sivanadham) - Created
##
## Usage        :   fix_dhcp.sh
##+----------------------------------------------------------------------------------+

###Start: Constants
_FIX_DHCP_SCRIPT_="fix_dhcp.sh"
_AM_STARTUP_="azure-migrate-startup"
_STARTUP_SCRIPT_="${_AM_STARTUP_}.sh"
_AM_SCRIPT_DIR_="/usr/local/azure-migrate"
_AM_SCRIPT_LOG_FILE_="/var/log/${_AM_STARTUP_}.log"
_AZURE_ASSET_TAG_="7783-7084-3265-9085-8269-3286-77"
###End: Constants

unset _DISTRO_
find_distro()
{
if [ -f /etc/oracle-release ] && [ -f /etc/redhat-release ] ; then
    if grep -q 'Oracle Linux Server release 6.*' /etc/oracle-release ; then
        _DISTRO_="OL6"
    elif grep -q 'Oracle Linux Server release 7.*' /etc/oracle-release ; then
        _DISTRO_="OL7"
    elif grep -q 'Oracle Linux Server release 8.*' /etc/oracle-release ; then
		_DISTRO_="OL8"
    fi
elif [ -f /etc/redhat-release ]; then
    if grep -q 'Red Hat Enterprise Linux Server release 6.*' /etc/redhat-release || \
       grep -q 'Red Hat Enterprise Linux Workstation release 6.*' /etc/redhat-release; then
            _DISTRO_="RHEL6"
    elif grep -q 'Red Hat Enterprise Linux Server release 7.*' /etc/redhat-release || \
         grep -q 'Red Hat Enterprise Linux Workstation release 7.*' /etc/redhat-release; then
            _DISTRO_="RHEL7"
    elif grep -q 'Red Hat Enterprise Linux release 8.*' $mntpath/etc/redhat-release; then
            _DISTRO_="RHEL8"
    elif grep -q 'Red Hat Enterprise Linux release 9.*' $mntpath/etc/redhat-release; then
            _DISTRO_="RHEL9"
    elif grep -q 'CentOS Linux release 6.*' /etc/redhat-release || \
         grep -q 'CentOS release 6.*' /etc/redhat-release; then
            _DISTRO_="CENTOS6"
    elif grep -q 'CentOS Linux release 7.*' /etc/redhat-release; then
            _DISTRO_="CENTOS7"
    elif grep -q 'CentOS Linux release 8.*' /etc/redhat-release ||
         grep -q 'CentOS Stream release 8.*' /etc/redhat-release; then
            _DISTRO_="CENTOS8"
    elif grep -q 'CentOS Linux release 9.*' /etc/redhat-release ||
         grep -q 'CentOS Stream release 9.*' /etc/redhat-release; then
            _DISTRO_="CENTOS9"
    fi
elif [ -f /etc/SuSE-release ]; then
    if grep -q 'VERSION = 11' /etc/SuSE-release; then
        _DISTRO_="SLES11"
    fi
    if grep -q 'VERSION = 12' /etc/SuSE-release; then
        _DISTRO_="SLES12"
    elif grep -q 'VERSION="15' /etc/SuSE-release; then
		_DISTRO_="SLES15"
    fi
elif [ -f /etc/os-release ] && grep -q 'SLES' /etc/os-release; then
	if grep -q 'VERSION="12' /etc/os-release; then
		_DISTRO_="SLES12"
	elif grep -q 'VERSION="15' /etc/os-release; then
		_DISTRO_="SLES15"
    fi
elif [ -f /etc/lsb-release ]; then
    if grep -q 'Ubuntu 14.*' /etc/lsb-release ; then
        _DISTRO_="UBUNTU14"
    elif grep -q 'Ubuntu 16.*' /etc/lsb-release ; then
        _DISTRO_="UBUNTU16"
    elif grep -q 'Ubuntu 18.*' /etc/lsb-release ; then
        _DISTRO_="UBUNTU18"
    elif grep -q 'Ubuntu 19.*' /etc/lsb-release ; then
        _DISTRO_="UBUNTU19"
    elif grep -q 'Ubuntu 20.*' /etc/lsb-release ; then
        DISTRO_="UBUNTU20"
    elif grep -q 'Ubuntu 21.*' /etc/lsb-release ; then
        _DISTRO_="UBUNTU21"
    elif grep -q 'Ubuntu 22.*' /etc/lsb-release ; then
        _DISTRO_="UBUNTU22"
    fi
elif [ -f /etc/debian_version ]; then
    if grep -q '^7.*' /etc/debian_version; then
        _DISTRO_="DEBIAN7"
    elif grep -q '^8.*' /etc/debian_version; then
        _DISTRO_="DEBIAN8"
    elif grep -q '^9.*' /etc/debian_version; then
        _DISTRO_="DEBIAN9"
    elif grep -q '^10.*' /etc/debian_version; then
        _DISTRO_="DEBIAN10"
    elif grep -q '^11.*' /etc/debian_version; then
        _DISTRO_="DEBIAN11"
    elif grep -q '^kali-rolling*' /etc/debian_version; then
        _DISTRO_="KALI-ROLLING"
    fi
fi
}

delete()
{
    if [[ -e $1 ]] || [[ -d $1 ]]; then
        rm -rf $1
        return $?
    else
        echo "$1 does not exist."
        return 0
    fi
}

is_vm_running_in_azure()
{
    test -d /sys/bus/vmbus/drivers/hv_storvsc || return 1
    test -f /sys/devices/virtual/dmi/id/chassis_asset_tag || return 1
    
    local atag="`cat /sys/devices/virtual/dmi/id/chassis_asset_tag`"
    if [ "$atag" != "$_AZURE_ASSET_TAG_" ]; then
        return 1
    fi
    
    return 0
}

set_dhcp_ip_for_ubuntu()
{
    local _ubuntu_network_interfaces_file="/etc/network/interfaces"
    
    echo "# The loopback network interface" > $_ubuntu_network_interfaces_file
    echo "auto lo" >> $_ubuntu_network_interfaces_file
    echo "iface lo inet loopback" >> $_ubuntu_network_interfaces_file
    echo "" >> $_ubuntu_network_interfaces_file
    
    ip -o link show | awk '{ print $2 }' | sed 's\:\\' | while read _line_
    do
        local name=`echo $_line_|awk '{ print $1 }'`
        
        if [ "$name" = "lo" ]
        then
            echo "Skipping loopback nic."
            continue
        fi

        echo "Configuring $name ..."
        echo "" >> $_ubuntu_network_interfaces_file
        echo "auto $name" >> $_ubuntu_network_interfaces_file
        echo "iface $name inet dhcp" >> $_ubuntu_network_interfaces_file
        
        #restart the interface
        echo "Restarting $name ..."
        ifdown "$name"
        ifup "$name"
    done
}

set_dhcp_ip_for_rhel()
{
    local _nw_scripts_dir="/etc/sysconfig/network-scripts"
    local host_name=`hostname`
    ip -o link show | awk '{ print $2 }' | sed 's\:\\' | while read _line_
    do
        local name=`echo $_line_|awk '{ print $1 }'`
        
        if [ "$name" = "lo" ]; then
            echo "Skipping the loopback nic."
            continue
        fi
        
        local _ifconfig_file="$_nw_scripts_dir/ifcfg-$name"
        
        echo "DHCP_HOSTNAME=$host_name" >$_ifconfig_file
        echo "DEVICE=$name" >> $_ifconfig_file
        echo "ONBOOT=yes" >> $_ifconfig_file
        echo "DHCP=yes" >> $_ifconfig_file
        echo "BOOTPROTO=dhcp" >> $_ifconfig_file
        echo "TYPE=Ethernet" >>  $_ifconfig_file
        echo "USERCTL=no" >>  $_ifconfig_file
        echo "PEERDNS=yes" >>  $_ifconfig_file
        echo "IPV6INIT=no" >> $_ifconfig_file
        
        #Restart the interfaces and NetworkManager for RHEL7/CentOS7
        case $_DISTRO_ in
            RHEL7|CENTOS7|OL7)
                echo "Restarting $name ..."
                ifdown "$name"
                ifup "$name"
                
                echo "Restarting NetworkManager"
                systemctl restart NetworkManager
                echo "Exit code: $?"
            ;;
            *)
                echo "Skipping $name interface restart"
            ;;
        esac        
    done

    service network restart 
}

set_dhcp_ip_for_suse() 
{
    _nw_config_dir="/etc/sysconfig/network"

    #Creating network config files for all the nics
    ip -o link show | awk '{ print $2 }' | sed 's\:\\' | while read line
    do
        name=`echo $line|awk '{ print $1 }'`
        
        if [ "$name" = "lo" ]; then
            echo "Skipping the loopback nic entry"
            continue
        fi
        
        local _ifconfig_file="$_nw_config_dir/ifcfg-$name"
        
        echo "BOOTPROTO='dhcp'"  >$_ifconfig_file
        echo "MTU='' " >> $_ifconfig_file
        echo "REMOTE_IPADDR='' " >>$_ifconfig_file
        echo "STARTMODE='onboot'" >>$_ifconfig_file
    done
    
    service network restart
}

configure_nics_to_dhcp()
{
    case $_DISTRO_ in
        CENTOS*|OL*|RHEL*)
            set_dhcp_ip_for_rhel
        ;;
        SLES*)
            set_dhcp_ip_for_suse
        ;;
        UBUNTU14|UBUNTU16|DEBIAN*|KALI*)
            set_dhcp_ip_for_ubuntu
        ;;
        UBUNTU18|UBUNTU20)
            echo "No operation for $_DISTRO_"
        ;;
        *)
            echo "ERROR: Unknown distro ${_DISTRO_}."
        ;;
    esac
}

remove_systemd_startup_script()
{
    echo "Remove start-up script..."
    
    local script_path="${_AM_SCRIPT_DIR_}/${_FIX_DHCP_SCRIPT_}"
    local service_file_path="/lib/systemd/system/${_AM_STARTUP_}.service"
    
    # Unregister the startup script
    systemctl disable azure-migrate-startup.service

    # Delete the startup script files.
    delete $service_file_path
    delete $_AM_SCRIPT_DIR_
}

remove_chkconfig_startup_script()
{
    echo "Remove start-up script..."
    
    local script_path="${_AM_SCRIPT_DIR_}/${_FIX_DHCP_SCRIPT_}"
    local service_file_path="/etc/init.d/${_AM_STARTUP_}"
    
    # Unregister the startup script
    if which chkconfig > /dev/null 2>&1 ;then
        chkconfig --del "${_AM_STARTUP_}"
    elif which update-rc.d > /dev/null 2>&1 ;then
        update-rc.d -f "${_AM_STARTUP_}" remove
    else
        echo "ERROR: Could not remove startup script."
    fi
        

    # Delete the startup script files.
    delete $service_file_path
    delete $_AM_SCRIPT_DIR_
}

remove_startup_script()
{
    case $_DISTRO_ in
        RHEL6|CENTOS6|OL6|SLES11|UBUNTU14|DEBIAN7)
            remove_chkconfig_startup_script
            ;;
        RHEL7|CENTOS7|OL7|SLES12|DEBIAN8|UBUNTU16|KALI*)
            remove_systemd_startup_script
            ;;
        *)
            echo "Unknown distro $_DISTRO_"
            ;;
    esac
}

fix_dhcp_on_startup()
{
    is_vm_running_in_azure
    if [[ $? -eq 0 ]]; then
        echo "VM is running in azure. Fixing dhcp ..."
        configure_nics_to_dhcp
        remove_startup_script
    else
        echo "VM is not running in azure. Exiting ..."
    fi
}

main()
{
find_distro

fix_dhcp_on_startup
}

main "$@"