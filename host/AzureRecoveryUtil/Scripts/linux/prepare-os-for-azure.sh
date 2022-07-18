#!/bin/bash

##+----------------------------------------------------------------------------------+
##            Copyright(c) Microsoft Corp. 2015
##+----------------------------------------------------------------------------------+
## File         :   prepare-os-for-azure.sh
##
## Description  :   
##
## History      :   11-11-2018 (Venu Sivanadham) - Created
##
## Usage        :   prepare-os-for-azure.sh <chroot-path>
##+----------------------------------------------------------------------------------+

###Start: Script error codes.
_E_AZURE_SMS_INTERNAL=1
_E_SCRIPT_SYNTAX_ERROR=2
_E_AZURE_SMS_TOOLS_MISSING=3
_E_AZURE_SMS_ARGS=4
_E_AZURE_SMS_OS_UNSUPPORTED=5
_E_AZURE_SMS_CONF_MISSING=6
_E_AZURE_SMS_INITRD_IMAGE_GENERATION_FAILED=7
_E_AZURE_SMS_HV_DRIVERS_MISSING=8
_E_AZURE_GA_INSTALLATION_FAILED=9
_E_AZURE_ENABLE_DHCP_FAILED=10

###End: Script error codes.

###Start: Constants
_FIX_DHCP_SCRIPT_="fix_dhcp.sh"
_AM_STARTUP_="azure-migrate-startup"
_STARTUP_SCRIPT_="${_AM_STARTUP_}.sh"
_AM_SCRIPT_DIR_="/usr/local/azure-migrate"
_AM_SCRIPT_LOG_FILE_="/var/log/${_AM_STARTUP_}.log"
_AM_INSTALLGA_="asr-installga"
_AM_HYDRATION_LOG_="/var/log/am-hydration-log"
###End: Constants

#@1 - Error Code
#@2 - Error Data
throw_error()
{
    local error_code=$_E_AZURE_SMS_INTERNAL
    [[ $1 =~ [[:digit:]] ]] && error_code=$1
    
    local error_data="Unavailable"
    [[ -z "$2" ]] || error_data="$2"
    
    echo "[Sms-Scrip-Error-Data]:${error_data}"
    echo "[Sms-Telemetry-Data]:${telemetry_data}"

    exit $error_code
}

# Usage: Add telemetries about utilities absent on source VM.
unset telemetry_data
#@1 - Telemetry Data
add_telemetry_data()
{
    if [[ -z $telemetry_data ]]; then
        telemetry_data="$1"
    else
        telemetry_data="$telemetry_data*#*$1"
        echo "New TelemetryData: $telemetry_data"
    fi
}

#@1: Property modified by Azure Migrate
#@2: Remarks for modification - May contain value/ status if modification was made
unset am_telem_suffix
add_am_hydration_log()
{
    if [[ -z $am_telem_suffix ]]; then
        am_telem_suffix="-$(date).log"
        echo "---Hydration Log for Azure Migrate/Site Recovery Begin---" >> "${chroot_path}${_AM_HYDRATION_LOG_}$am_telem_suffix"
        echo "Some paths may begin with $chroot_path relative to hydration VM." >> "${chroot_path}${_AM_HYDRATION_LOG_}$am_telem_suffix"
    fi

    echo "$1 :-: $2" >> "${chroot_path}${_AM_HYDRATION_LOG_}$am_telem_suffix"
}

#@1 - string to log
trace()
{
    echo "$(date +"%a %b %d %Y %T") : $1"
}

unset chroot_path
unset hydration_config_settings
#@1 - chroot path
validate_script_input()
{
    [[ $# -lt 1 ]] && throw_error $_E_AZURE_SMS_ARGS "chroot-path missing"  
    [[ ! -d "$1" ]] && throw_error $_E_AZURE_SMS_ARGS "$1 invalid"
    
    trace "chroot: $1"

    if [[ $# -ge 2 ]]; then
        trace "Hydration Config Settings: $2"
        hydration_config_settings="$2"
    fi
    chroot_path="$1"
}

unset src_distro
#@1 - Root path
find_src_distro()
{
    local root_path=$chroot_path
    [[ -z "$1" ]] || root_path=$1
    
    local script_dir=$(cd $(dirname "$0") > /dev/null && pwd)
    [[ -d "$script_dir" ]] || script_dir=/usr/local/AzureRecovery
    
    src_distro=$($script_dir/OS_details_target.sh "$chroot_path")
    if [[ -z "src_distro" ]]; then
        throw_error $_E_AZURE_SMS_OS_UNSUPPORTED "unknown"
    else
        trace "OS : $src_distro"
    fi
}

unset firmware_type
unset _grub2_efi_path
get_firmware_type()
{
    _grub2_efi_path="$chroot_path/boot/efi/EFI/"

    case "$src_distro" in
        OL7*|RHEL6*|RHEL7*|RHEL8*|OL8*)
            _grub2_efi_path="${_grub2_efi_path}redhat"
            ;;
        CENTOS7*|CENTOS8*)
            _grub2_efi_path="${_grub2_efi_path}centos"
            ;;
        SLES11*|SLES12*|SLES15*)
            _grub2_efi_path="${_grub2_efi_path}opensuse"
            ;;
        UBUNTU*)
            _grub2_efi_path="${_grub2_efi_path}ubuntu"
            ;;
        DEBIAN*)
            _grub2_efi_path="${_grub2_efi_path}debian"
            ;;
        *)
            echo "Unsupported OS version for Migration - $src_distro "
            ;;
    esac

    firmware_type="BIOS"
    if [[ -d $_grub2_efi_path ]]; then
        if [[ -f "$_grub2_efi_path/grub.conf" ]] || [[ -f "$_grub2_efi_path/menu.lst" ]] || [[ -f "$_grub2_efi_path/grub.cfg" ]]; then
            firmware_type="UEFI"
            echo "Grub.cfg path in case of UEFI is: $_grub2_efi_path"
        fi
    else
        echo "Default grub path applicable in case of BIOS"
    fi

    add_am_hydration_log "Firmware Type" "$firmware_type"
}

#@1 - Source file path
#@2 - Target file path
copy_file()
{
    local _source_=$1
    local _target_=$2

    /bin/cp -f $_source_ $_target_
    
    return $?
}

# Recursively copy the folder and contents
function CopyDirectory
{
    if [ -d "$1" ]; then
        /bin/cp -r $1 $2
    else
        echo "Folder $1 is absent. Copy failed."
    fi
    return $_SUCCESS_
}

_BCK_DIR_="azure_sms_bck"
_BCK_EXT_=".bck_azure_sms"
#@1 - File path
backup_file()
{
    local _source_=$1
    local _target_="${_source_}${_BCK_EXT_}"
    
    [[ -f $_source_ ]] || { echo "$_source_ file not found."; return 1; }
    [[ -f $_target_ ]] && { echo "$_target_ already exist."; return 1; }
    
    copy_file $_source_ $_target_
    if [[ $? -ne 0 ]]; then
        # We ignore error in copy step for migration.
        # But log a trace so it shows up in logs.
        trace "WARNING: Could not backup file $1."
    fi

    return $?
}

#@1 - File path
move_to_backup()
{
    local _source_=$1
    local _target_="${_source_}${_BCK_EXT_}"
    
    [[ $_source_ =~ .*${_BCK_EXT_}$ ]] && {
        echo "$_source_ is a backup file.";
        return 1;
    }
    
    [[ -f $_source_ ]] || { 
        echo "$_source_ file not found.";
        return 1;
    }
    
    mv -f $_source_ $_target_
    return $?
}

#@1 - File path
#@2 - Backup directory (optional)
move_to_backup_dir()
{
    [[ -z $1 ]] && return 0
    
    local _source_=$1
    [[ -f $_source_ ]] || {
        echo "File $_source_ not found"
        return 1;
    }
    
    local _target_dir_=$2
    [[ -z $_target_dir_ ]] && _target_dir_="$(dirname $_source_)/$_BCK_DIR_"
    
    [[ -d $_target_dir_ ]] || {
        echo "Creating backup directory $_target_dir_"
        mkdir $_target_dir_
    }
    
    [[ -d $_target_dir_ ]] || {
        echo "Could not create backup directory $_target_dir_"
        return 1;
    }
    
    mv -f $_source_ $_target_dir_
    return $?
}

#@1 - Tool name
verify_tools()
{
    [[ -z "$1" ]] && return 0
    
    for tool_name in $1
    do
        trace "Looking for the tool $tool_name"
        chroot $chroot_path which $tool_name
        if [[ $? -ne 0 ]] ; then
            # Fail with error codes related to usage of the tool
            # Severity of effect due to tool's absence will be checked later.
            add_am_hydration_log "$tool_name" "ABSENT"
            add_telemetry_data $tool_name
        else
            add_am_hydration_log "$tool_name" "PRESENT"
            trace "$tool_name is available on source."
        fi
    done

    return 0
}

#@1 - Kernel version
#@2 - Module list
verify_kernel_modules()
{
    [[ -z "$1" ]] && return 1
    [[ -z "$2" ]] && return 0
    
    local kernel_name="$1"
    for $module_name in $2
    do
        chroot $chroot_path modinfo -k "$kernel_name" $module_name
        if [[ $? -ne 0 ]] ; then
            throw_error $_E_AZURE_SMS_HV_DRIVERS_MISSING $kernel_name
        fi
    done
    
    return 0
}

#@1 - options to remove
#@2 - file path
#@3 - command prefix
remove_cmd_options()
{
    [[ -z "$1" ]] && return 1
    [[ -z "$2" ]] && return 1
    [[ -z "$3" ]] && return 1
    
    local _opts_=$1
    local _file_=$2
    local _cmd_=$3
    
    trace "Removing [${_opts_}] options from $_file_" 
    for _opt_ in $_opts_; do
        sed -i --follow-symlinks "/^[[:space:]]*${_cmd_}.*[[:space:]]root=.*/s/[[:space:]]${_opt_}[[:space:]]*/ /" $_file_
    done
    
    return 0
}

#@1 - options to add
#@2 - file path
#@3 - command prefix
add_cmd_options()
{
    [[ -z "$1" ]] && return 1
    [[ -z "$2" ]] && return 1
    [[ -z "$3" ]] && return 1
    
    local _opts_=$1
    local _file_=$2
    local _cmd_=$3
    
    trace "Adding [${_opts_}] options to $_file_" 
    for _opt_ in $_opts_; do
        if grep -q "^[[:space:]]*${_cmd_}.*[[:space:]]root=.*[[:space:]]${_opt_}\>.*" $_file_; then
            trace "$_opt_ already present in grub command line."
            continue
        fi
        
        sed -i --follow-symlinks "s/\(^[[:space:]]*${_cmd_}.*[[:space:]]root=.*\)\($\)/\1 ${_opt_}\2/" $_file_
    done
    
    return 0
}

#@1 - File name
#@2 - Setting name
#@3 - Setting value
append_config_setting_value()
{
    if grep -q "\<${2}\>[[:space:]]*=.*\<${3}\>.*" $1 ; then
        # Setting key with value already present.
        return 0
    elif grep -q "\<${2}\>[[:space:]]*=.*" $1 ; then
        # setting key present but not the value, append it.
        sed -i --follow-symlinks "s/\(\<${2}\>[[:space:]]*=[[:space:]]*[\"]\?\)\(.*$\)/\1${3} \2/" $1
    else
        # Setting key not found, adding the setting key=value
        echo "${2}=\"${3}\"" >> $1
    fi
    
    return 0
}

#@1 - Grub command options
#@2 - Grub command setting name
#@3 - Grub command configuration file path
add_grub2_config_options()
{
    local grub_cmd_setting=$2
    local grub_conf_file=$3
    [[ -z $grub_cmd_setting ]] && grub_cmd_setting="GRUB_CMDLINE_LINUX"
    [[ -z $grub_conf_file ]] && grub_conf_file="$chroot_path/etc/default/grub"
    
    for _opt_ in $1; do
        append_config_setting_value $grub_conf_file $grub_cmd_setting $_opt_
    done
    
    return 0
}

#@1 - File name
#@2 - Setting name
#@3 - Setting value
remove_config_setting_value()
{
    if grep -q "\<$2\>[[:space:]]*=.*\<$3\>.*" $1 ; then
        sed -i --follow-symlinks "s/\(\<$2\>[[:space:]]*=.*\)\<$3\>\(.*\)/\1\2/" $1
    else
        echo "Value \"$3\" not found for the $2."
        return 1
    fi
    return 0
}

#@1 - Setting name
#@2 - File name
comment_config_setting()
{
  echo "Printing line in file $2 containing the pattern $1 if found."
  sed -nr "/$1/p" "$2"

  # Comment out the entry.
  sed -i "/$1/s/^/#/" "$2"
}

#@1 - Setting name
#@2 - File name
uncomment_config_setting()
{
  echo "Printing line in file $2 containing the pattern $1 if found."
  sed -nr "/$1/p" "$2"

  # Uncomment the entry.
  sed -i "/$1/s/^#//" "$2"
}

#@1 - File name
#@2 - Setting name
#@3 - Setting value
update_config_value()
{
    _config_file_="$1"
    _setting_name_="$2"
    _setting_value_="$3"

    if grep -q "^$2=$3" $1 ; then
        echo "$2=$3 config value is already in place."
    elif grep -q "^$2=.*" $1 ; then
        sed -i --follow-symlinks "s/\(^$2=\)\(.*\?\)/\1$3/" $1
    else
        # Setting key not found, adding the setting key=value
        echo "$2=$3" >> $1
    fi
}

#@1 - File name
#@2 - Setting name
#@3 - Setting value
update_config_value_if_exists()
{
    _config_file_="$1"
    _setting_name_="$2"
    _setting_value_="$3"

    if grep -q "^$2=$3" $1 ; then
        echo "$2=$3 config value is already in place."
    elif grep -q "^$2=.*" $1 ; then
        sed -i --follow-symlinks "s/\(^$2=\)\(.*\?\)/\1$3/" $1
    else
        echo "$2 setting not found, nothing to update."
    fi
}

#@1 - File name
#@2 - Setting name
remove_config_entry()
{
    _config_file_="$1"
    _setting_name_="$2"

    if grep -q "^$2=.*" $1 ; then
        sed -i --follow-symlinks "/^$2=/d" $1
    else
        echo "$2 config setting not found, nothing to remove."
    fi
}

#@1 - Grub command options
#@2 - Grub command setting name
#@3 - Grub command configuration file path
remove_grub2_config_options()
{
    local grub_cmd_setting=$2
    local grub_conf_file=$3
    [[ -z $grub_cmd_setting ]] && grub_cmd_setting="GRUB_CMDLINE_LINUX"
    [[ -z $grub_conf_file ]] && grub_conf_file="$chroot_path/etc/default/grub"
    
    for _opt_ in $1; do
        remove_config_setting_value $grub_conf_file $grub_cmd_setting $_opt_
    done
    
    return 0
}

#@1 - options to remove
#@2 - grub file path
remove_grub_cmd_options()
{
    [[ -z "$1" ]] && return 1
    
    local _grub_conf_file="$2"
    if [[ -z "$2" ]]; then
        if [ "$firmware_type" = "UEFI" ]; then
            _grub_conf_file="$_grub2_efi_path/grub.conf"
        else
            _grub_conf_file="$chroot_path/grub/grub.conf"
        fi
    fi
    
    
    remove_cmd_options "$1" "$_grub_conf_file" "kernel" 
    return $?
}

#@1 - options to remove
#@2 - grub file path
remove_grub2_cmd_options()
{
    [[ -z "$1" ]] && return 1
    
    local _grub2_conf_file="$2"
    if [[ -z "$2" ]]; then
        if [ "$firmware_type" = "UEFI" ]; then
            _grub_conf_file="$_grub2_efi_path/grub.cfg"
        else
            _grub2_conf_file="$chroot_path/boot/grub2/grub.cfg"
        fi
    fi
     
    remove_cmd_options "$1" "$_grub2_conf_file" "linux"
    return $?
}

#@1 - options to add
#@2 - grub file path
add_grub_cmd_options()
{
    [[ -z "$1" ]] && return 1
    
    local _grub_conf_file="$2"
    if [[ -z "$2" ]]; then
        if [ "$firmware_type" = "UEFI" ]; then
            _grub_conf_file="$_grub2_efi_path/grub.conf"
        else
            _grub_conf_file="$chroot_path/grub/grub.conf"
        fi
    fi
    
    add_cmd_options "$1" "$_grub_conf_file" "kernel"
    return $?
}

#@1 - options to add
#@2 - grub file path
add_grub2_cmd_options()
{
    [[ -z "$1" ]] && return 1
    
    local _grub2_conf_file="$2"
    if [[ -z "$2" ]]; then
        if [ "$firmware_type" = "UEFI" ]; then
            _grub_conf_file="$_grub2_efi_path/grub.cfg"
        else
            _grub2_conf_file="$chroot_path/boot/grub2/grub.cfg"
        fi
    fi    

    add_cmd_options "$1" "$_grub2_conf_file" "linux"
    return $?
}

#@1 - kernel version
is_kernel_in_use()
{
    [[ -z "$1" ]] && return 1
    
    local kernel_version=$1
    local grub_conf_files="$chroot_path/boot/grub/menu.lst \
            $chroot_path/boot/grub/grub.cfg \
            $chroot_path/boot/grub2/grub.cfg \
            $_grub2_efi_path/menu.lst \
            $_grub2_efi_path/grub.conf \
            $_grub2_efi_path/grub.cfg"
    
    for grub_file in $grub_conf_files
    do
        if [[ -f $grub_file ]]; then
            grep -q "\<initrd.*[[:space:]].*/initr.*${kernel_version}.*" $grub_file
            return $?
        fi
    done
    
    return 1
}

mount_runtime_partitions()
{
    for partition in proc dev sys
    do
        mount --bind "/$partition" "$chroot_path/$partition"
    done
}

remove_persistent_net_rules()
{
    local _files="${chroot_path}/lib/udev/rules.d/75-persistent-net-generator.rules \
    ${chroot_path}/etc/udev/rules.d/70-persistent-net.rules"
    
    for _file in $_files
    do
        backup_file $_file
        rm -f $_file
    done
}

reset_persistent_net_gen_rules()
{
    move_to_backup "${chroot_path}/etc/udev/rules.d/75-persistent-net-generator.rules"
    chroot ${chroot_path} ln -f -s /dev/null /etc/udev/rules.d/75-persistent-net-generator.rules
}

remove_network_manager_rpm()
{
    trace "Removing persistent net rules"
    
    reset_persistent_net_gen_rules
    
    backup_file "${chroot_path}/etc/udev/rules.d/70-persistent-net.rules"
    rm -f ${chroot_path}/etc/udev/rules.d/70-persistent-net.rules
    
    echo "Removing NetworkManager ..."
    chroot ${chroot_path} rpm -e --nodeps NetworkManager
    
    # Remove network manager binary if exist.
    local net_mngr_file="${chroot_path}/etc/init.d/NetworkManager"
    if [[ -f $net_mngr_file ]]; then
        rm -f $net_mngr_file
    fi
}

update_network_dhcp_file()
{
    local dhcp_file="${chroot_path}/etc/sysconfig/network/dhcp"
    if [[ -f $dhcp_file ]]; then
        update_config_value $dhcp_file DHCLIENT_SET_HOSTNAME '"no"'
        if [[ $telemetry_data != *"dhclient"* ]]; then
            # The default value is empty.
            # If empty, a linux VM by design searches for dhcpcd, then dhclient
            update_config_value $dhcp_file DHCLIENT_BIN '"dhclient"'
        fi
    fi
}

update_network_file()
{
    local network_file="${chroot_path}/etc/sysconfig/network"
    if [[ -f $network_file ]]; then
        backup_file $network_file
        update_config_value $network_file "NETWORKING" "yes"
        remove_config_entry $network_file "GATEWAY"
        update_config_value_if_exists $network_file "GATEWAYDEV" "eth0"
    else
        echo "NETWORKING=yes" > $network_file
        echo "HOSTNAME=localhost.localdomain" >> $network_file
    fi
}

#@1 - kernel image name or path
is_kernel_image_in_use()
{
    [[ -z "$1" ]] && return 1
    
    local kernel_image=$(basename $1)
    local grub_conf_files="$chroot_path/boot/grub/menu.lst \
            $chroot_path/boot/grub/grub.conf \
            $chroot_path/boot/grub/grub.cfg \
            $chroot_path/boot/grub2/grub.cfg \
            $_grub2_efi_path/menu.lst \
            $_grub2_efi_path/grub.conf \
            $_grub2_efi_path/grub.cfg"
    
    for grub_file in $grub_conf_files
    do
        if [[ -f $grub_file ]] && 
        grep -q "\<initrd.*[[:space:]].*/${kernel_image}\>" $grub_file; then
            trace "${kernel_image} found in $grub_file"
            return 0
        fi
    done
    
    return 1
}

#@1 - module name
verify_hv_drivers_in_module()
{
    [[ -z "$1" ]] && return 0
    
    local kernel_version=$1
    trace "Checking hyper-v drivers in the kernel $kernel_version"
    
    for driver_name in hv_vmbus hv_storvsc hv_netvsc
    do
        chroot $chroot_path modinfo -k $kernel_version $driver_name
        [[ $? -ne 0 ]] && throw_error $_E_AZURE_SMS_HV_DRIVERS_MISSING $kernel_version
        
        trace "$driver_name available in the kernel."
    done
    
    return 0
}

#@1 - kernel image file path
verify_hv_drivers_in_kernel_image()
{
    [[ -z "$1" ]] && return 1
    
    local kernel_image_name=$(basename $1)
    local ls_image=$(chroot $chroot_path lsinitrd /boot/$kernel_image_name)
    
    trace "Checking hyper-v drivers in the image $kernel_image_name"
    
    for hv_driver in hv_vmbus hv_storvsc hv_netvsc
    do
        echo $ls_image | grep -q "${hv_driver}.ko"
        [[ $? -ne 0 ]] && return 1
    done
    
    add_am_hydration_log "$kernel_image_name Hyper-V Drivers Status" "All Hyper-V drivers already present."
    trace "hyper-v drivers present in $kernel_image_name"
    
    return 0
}

#@1 - Kernel version
#@2 - Kernel image file path
generate_initrd_image()
{
    [[ -z "$1" ]] && return 1
    [[ -z "$2" ]] && return 1
    
    local hv_drivers="hv_vmbus hv_storvsc hv_netvsc"
    local kernel_image=$(basename $2)
    local kernel_image_path=/boot/$kernel_image

    trace "start re-generating kernel image $kernel_image_path with version $1 ..."
    chroot $chroot_path dracut -f --add-drivers "$hv_drivers" $kernel_image_path $1
    if [[ $? -ne 0 ]]; then
        # We throw an error in dracut step for migration.
        # TODO: Propagate this error up and handle so Migration passes but user\CSS knows step to take.
        # This will be a multi phase \ step error code bitmap. Since steps are independent of 
        # each other.
        trace "WARNING: Could not perform dracut and generate image step."
        throw_error $_E_AZURE_SMS_INITRD_IMAGE_GENERATION_FAILED $1
    else
        trace "successfully generated the image!"
    fi
    
    return 0
}

#@1 - grub options to add
#@2 - grub options to remove
modify_grub_config()
{
    local _grub_path="$chroot_path/boot/grub/menu.lst"
    local opts_to_comment="password --encrypted"

    if [ "$firmware_type" = "UEFI" ]; then
        if [ -f "$_grub2_efi_path/grub.conf" ]; then
            _grub_path="$_grub2_efi_path/grub.conf"
        elif [ -f "$_grub2_efi_path/menu.lst" ]; then
            _grub_path="$_grub2_efi_path/menu.lst"
        elif [ -f "$_grub2_efi_path/grub.cfg" ]; then
            _grub_path="$_grub2_efi_path/grub.cfg"
        fi
    fi

    [[ ! -f $_grub_path ]] && _grub_path="$chroot_path/boot/grub/grub.conf"
    echo "Grub File: $_grub_path"

    [[ ! -f $_grub_path ]] && throw_error $_E_AZURE_SMS_CONF_MISSING $(basename $_grub_path)
        
    remove_grub_cmd_options "$opts_to_remove" $_grub_path
    add_grub_cmd_options "$opts_to_add" $_grub_path
    comment_config_setting "$opts_to_comment" $_grub_path

    if [[ $src_distro == *"SLES11"* ]]; then
        echo "Adding additonal lines for SLES11:"
        append_config_line_parameter "serial --unit=0 --speed=115200 --parity=no" $_grub_path
        append_config_line_parameter "terminal --timeout=15 serial console" $_grub_path
    fi
}

#@1 - grub options to add
#@2 - grub config setting name
#@3 - grub options to remove
modify_grub2_config()
{
    local _grub2_path="$chroot_path/boot/grub2/grub.cfg"

    if [ "$firmware_type" = "UEFI" ]; then
        if [ -f "$_grub2_efi_path/grub.cfg" ]; then
            _grub2_path="$_grub2_efi_path/grub.cfg"
        elif [ -f "$_grub2_efi_path/menu.lst" ]; then
            _grub2_path="$_grub2_efi_path/menu.lst"
        elif [ -f "$_grub2_efi_path/grub.conf" ]; then
            _grub2_path="$_grub2_efi_path/grub.conf"
        fi
    fi

    echo "Grub File: $_grub2_path Setting Name: $2 Add: $1 Remove: $3"
    [[ ! -f $_grub2_path ]] && _grub2_path="$chroot_path/boot/grub/grub.cfg"
    [[ ! -f $_grub2_path ]] && throw_error $_E_AZURE_SMS_CONF_MISSING $(basename $_grub2_path)
    
    modify_grub2_config_helper "$1" "$2" "$3" $_grub2_path

    # Additionally change grub.cfg file located at grub and grub2 folder. 
    if [[ -f "$chroot_path/boot/grub2/grub.cfg" && $_grub2_path != "$chroot_path/boot/grub2/grub.cfg" ]]; then
        modify_grub2_config_helper "$1" "$2" "$3" "$chroot_path/boot/grub2/grub.cfg"
    fi

    if [[ -f "$chroot_path/boot/grub/grub.cfg" && $_grub2_path != "$chroot_path/boot/grub/grub.cfg" ]]; then
        modify_grub2_config_helper "$1" "$2" "$3" "$chroot_path/boot/grub/grub.cfg"
    fi
}

#@1 - grub options to add
#@2 - grub config setting name
#@3 - grub options to remove
#@4 - grub config file
modify_grub2_config_helper()
{
    _grub2_path="$4"

    add_am_hydration_log "Serial Console - $_grub2_path Remove" "$3"
    add_am_hydration_log "Serial Console $_grub2_path Add" "$2 - $1"

    if [[ ! -z "$3" ]]; then
        remove_grub2_cmd_options "$3" $_grub2_path
        remove_grub2_config_options "$3" $2
    fi
    
    if [[ ! -z "$1" ]]; then
        add_grub2_config_options "$1" "$2"
        add_grub2_cmd_options "$1" $_grub2_path
    fi

    local opts_to_comment="password --encrypted"
    comment_config_setting "$opts_to_comment" $_grub2_path
}

update_vm_repositories()
{
    case $src_distro in
        CENTOS6*)
            # Bullet 8 - https://docs.microsoft.com/en-us/azure/virtual-machines/linux/create-upload-centos#centos-6x
            sources_list_file="$chroot_path/etc/yum.repos.d/CentOS-Base.repo"
            backup_file $sources_list_file
            copy_file "/usr/local/AzureRecovery/CentOS6-Base.repo" "$sources_list_file"
        ;;
        CENTOS7*)
            # Bullet 6 - https://docs.microsoft.com/en-us/azure/virtual-machines/linux/create-upload-centos#centos-70
            sources_list_file="$chroot_path/etc/yum.repos.d/CentOS-Base.repo"
            backup_file $sources_list_file
            copy_file "/usr/local/AzureRecovery/CentOS7-Base.repo" "$sources_list_file"
        ;;
        CENTOS8*)
            # Not documented. Replicated from VM created through platform image.
            sources_list_file="$chroot_path/etc/yum.repos.d/CentOS-Base.repo"
            backup_file $sources_list_file
            copy_file "/usr/local/AzureRecovery/CentOS7-Base.repo" "$sources_list_file"
        ;;
        OL6*)
            # https://public-yum.oracle.com/public-yum-ol6.repo
            # https://docs.microsoft.com/en-us/azure/virtual-machines/linux/oracle-create-upload-vhd#oracle-linux-installation-notes
            sources_list_file="$chroot_path/etc/yum.repos.d/public-yum-ol6.repo"
            backup_file $sources_list_file
            copy_file "/usr/local/AzureRecovery/public-yum-ol6.repo" "$sources_list_file"
        ;;
        OL7*)
            # https://public-yum.oracle.com/public-yum-ol7.repo
            # https://docs.microsoft.com/en-us/azure/virtual-machines/linux/oracle-create-upload-vhd#oracle-linux-installation-notes
            sources_list_file="$chroot_path/etc/yum.repos.d/public-yum-ol7.repo"
            backup_file $sources_list_file
            copy_file "/usr/local/AzureRecovery/public-yum-ol6.repo" "$sources_list_file"
        ;;
        OL8*)
            # Not documented. Replicated from VM created through platform image.
            sources_list_file="$chroot_path/etc/yum.repos.d/public-yum-ol8.repo"
            backup_file $sources_list_file
            copy_file "/usr/local/AzureRecovery/public-yum-ol8.repo" "$sources_list_file"
        ;;
        UBUNTU*)
            # https://docs.microsoft.com/en-us/azure/virtual-machines/linux/create-upload-ubuntu#manual-steps
            sources_list_file="$chroot_path/etc/apt/sources.list"
            copy_file "$sources_list_file" "$(sources_list_file)_azr_sms_bak"
            sed -i 's/http:\/\/archive\.ubuntu\.com\/ubuntu\//http:\/\/azure\.archive\.ubuntu\.com\/ubuntu\//g' "$sources_list_file"
            sed -i 's/http:\/\/[a-z][a-z]\.archive\.ubuntu\.com\/ubuntu\//http:\/\/azure\.archive\.ubuntu\.com\/ubuntu\//g' "$sources_list_file"
        ;;
    esac
}

unset base_linuxga_path
install_linux_guest_agent()
{
    ga_uuid=$(uuidgen)
    base_linuxga_path="var/ASRLinuxGA-$ga_uuid"
	
	if [[ ! -d $chroot_path/$base_linuxga_path ]]; then
        trace "Base linux guest agent packages directory doesn't exist'. Creating $chroot_path/$base_linuxga_path"
        mkdir $chroot_path/$base_linuxga_path
    fi
	
    validate_guestagent_prereqs
    update_vm_repositories

    echo "Hydration Being Performed on the VM. $(date)" >> "$chroot_path/$base_linuxga_path/ASRLinuxGA.log"

    # If python is absent, guest agent installation will not work.
    # We continue to push the script to target VM to facilitate easier installation
    if [[ $telemetry_data != *"systemctl"* ]]; then
        enable_installga_service
    else
        enable_installga_chkconfig
    fi
	
    enable_postlogin_installga

    add_am_hydration_log "Guest agent installation logs location" "/$base_linuxga_path/ASRLinuxGA.log"

    linuxgadir="/usr/local/AzureRecovery/WALinuxAgentASR/WALinuxAgent-master"
    targetgadir="$chroot_path/$base_linuxga_path/WALinuxAgentASR/"

    if [[ ! -d "$linuxgadir" ]]; then
        trace "Folder containing Guest agent binaries is not present."
        return 1;
    fi

    if [[ ! -d $targetgadir ]]; then
        trace "Target guest agent folder doesn't exist. Creating $targetgadir"
        mkdir $targetgadir
    fi

    CopyDirectory $linuxgadir $targetgadir
    if [[ $? -ne 0 ]]; then
        trace "Could not copy LinuxGuestAgent installation directory to target location."
        return 1;
    fi

    copy_file "/usr/local/AzureRecovery/InstallLinuxGuestAgent.sh" "$chroot_path/$base_linuxga_path/InstallLinuxGuestAgent.sh"
    if [[ $? -ne 0 ]]; then
        trace "Could not copy LinuxGuestAgent installation file."
        return 1;
    fi

    copy_file "/usr/local/AzureRecovery/PythonSetupPrereqs.py" "$chroot_path/$base_linuxga_path/PythonSetupPrereqs.py"
    if [[ $? -ne 0 ]]; then
        trace "Could not copy python setup prereqs file."
        # Don't fail.
    fi

    chmod +x $chroot_path/$base_linuxga_path/*
}

configure_dhcp_rhel()
{
    local ifcfg_dir="$chroot_path/etc/sysconfig/network-scripts"
    
    # move the existing cscfg files to backup.
    for _file in $(ls $ifcfg_dir/ifcfg-*)
    do
        local ifcfg_file=$(basename $_file)
        
        # ignore loop back ifcfg.
        [[ "$ifcfg_file" = "ifcfg-lo" ]] && continue
        
        ifcfg_file="$ifcfg_dir/$ifcfg_file"
        move_to_backup_dir $ifcfg_file
    done
    
    local default_dhcp_cscfg="$ifcfg_dir/ifcfg-eth0"
    echo "DEVICE=eth0" > $default_dhcp_cscfg
    echo "ONBOOT=yes" >> $default_dhcp_cscfg
    echo "DHCP=yes" >> $default_dhcp_cscfg
    echo "BOOTPROTO=dhcp" >> $default_dhcp_cscfg
    echo "TYPE=Ethernet" >>  $default_dhcp_cscfg
    echo "USERCTL=no" >>  $default_dhcp_cscfg
    echo "PEERDNS=yes" >>  $default_dhcp_cscfg
    echo "IPV6INIT=no" >> $default_dhcp_cscfg
}

configure_dhcp_sles()
{
    local ifcfg_dir="$chroot_path/etc/sysconfig/network"
    
    # move the existing cscfg files to backup.
    for _file in $(ls $ifcfg_dir/ifcfg-*)
    do
        local ifcfg_file=$(basename $_file)
        
        # ignore loop back ifcfg.
        [[ "$ifcfg_file" = "ifcfg-lo" ]] && continue
        
        ifcfg_file="$ifcfg_dir/$ifcfg_file"
        move_to_backup_dir $ifcfg_file
    done
    
    local default_dhcp_cscfg="$ifcfg_dir/ifcfg-eth0"
    echo "BOOTPROTO='dhcp'"  >$default_dhcp_cscfg
    echo "MTU='' " >> $default_dhcp_cscfg
    echo "REMOTE_IPADDR='' " >>$default_dhcp_cscfg
    echo "STARTMODE='onboot'" >>$default_dhcp_cscfg
}

configure_dhcp_ubuntu()
{
    _interfaces_file="$chroot_path/etc/network/interfaces"
    
    # Backup the original interfaces file
    backup_file "$_interfaces_file"
    
    # Create new interfaces file
    echo "# The loopback network interface" > $_interfaces_file
    echo "auto lo" >> $_interfaces_file
    echo "iface lo inet loopback" >> $_interfaces_file
    echo "" >> $_interfaces_file
    echo "# The primary network interface" >> $_interfaces_file
    echo "auto eth0" >> $_interfaces_file
    echo "iface eth0 inet dhcp" >> $_interfaces_file
}

create_dhcp_netplan_config_and_apply()
{
    # As a workaround, taking a random filename for netplan dhcp configuration.
    # As soon as Azure Compute publishes steps for preparing Ubuntu 18.04 LTS
    # we will follow those instructions to prepare netplan configuration.
    local dhcp_netplan_file="$chroot_path/etc/netplan/50-azure_migrate_dhcp.yaml"
    if [[ -f $dhcp_netplan_file ]]; then
        echo "DHCP netplan configuration for Azure migrate is already exist."
        return;
    fi
    
    echo "# This is generated for Azure SMS to make NICs DHCP in Azure." > $dhcp_netplan_file
    echo "network:" >> $dhcp_netplan_file
    echo "    version: 2" >> $dhcp_netplan_file
    echo "    ethernets:" >> $dhcp_netplan_file
    echo "        ephemeral:" >> $dhcp_netplan_file
    echo "            dhcp4: true" >> $dhcp_netplan_file
    echo "            match:" >> $dhcp_netplan_file
    echo "                driver: hv_netvsc" >> $dhcp_netplan_file
    echo "                name: '!eth0'" >> $dhcp_netplan_file
    echo "            optional: true" >> $dhcp_netplan_file
    echo "        hotpluggedeth0:" >> $dhcp_netplan_file
    echo "            dhcp4: true" >> $dhcp_netplan_file
    echo "            match:" >> $dhcp_netplan_file
    echo "                driver: hv_netvsc" >> $dhcp_netplan_file
    echo "                name: 'eth0'" >> $dhcp_netplan_file
    
    trace "Applying the dhcp netplan configuration... "
    chroot $chroot_path netplan apply
    if [[ $? -eq 0 ]]; then
        trace "netplan with dhcp settings applied!"
    else
        trace "WARNING: netplan couldn't apply dhcp settings."
    fi
}

enable_network_service()
{
    trace "Making network service to start at boot time."

    case $src_distro in
        RHEL6*|CENTOS6*|OL6*)
            chroot $chroot_path chkconfig network on
            ;;
        RHEL7*|CENTOS7*|RHEL8*|CENTOS8*|OL7*|OL8*)
            chroot $chroot_path systemctl enable network
            ;;
        *)
            # For rest no operation.
            ;;
        esac
}

verify_src_os_version()
{
    find_src_distro
    
    local supported_distros="OL6 OL7 OL8 \
          RHEL6 RHEL7 RHEL8 RHEL9 \
          CENTOS6 CENTOS7 CENTOS8 CENTOS9 \
          SLES11 SLES12 SLES15 \
          UBUNTU14 UBUNTU16 UBUNTU18 UBUNTU19 UBUNTU20 UBUNTU21 UBUNTU22 \
          DEBIAN7 DEBIAN8 DEBIAN9 DEBIAN10 DEBIAN11 KALI-ROLLING"

    add_am_hydration_log "Identified OS Version" $src_distro

    for distro in $supported_distros
    do
        [[ "$src_distro" =~ "${distro}-"* ]] && [[ "$src_distro" == *"-64" ]] && return
    done

    throw_error $_E_AZURE_SMS_OS_UNSUPPORTED $src_distro
}

verify_requited_tools()
{
    # Prereqs:
    # Kernel image update with hyper-V drivers: dracut/mkinitrd, lsinitrd, modinfo
    # Networking Changes: dhclient/dhcpcd
    # Guest Agent: systemctl/service, python 2.6+ (Python to be checked later as minor version is also verified)
    # For all the tools absence, errors will be thrown out at a later stage depending on need.

    local tools_to_verify="lsinitrd dracut modinfo dhclient systemctl"

    verify_tools "$tools_to_verify"

    # For each tool, check for backup options if primary tool is absent.
    if [[ $telemetry_data == *"dracut"* ]]; then
        verify_tools "mkinitrd"
        # Don't fail immediately if mkinitrd is also absent.
        # Check if kernel and kernel image both contain Hyper-V drivers already.
        # If not, Return ToolsMissing Error. 
    fi

    if [[ $telemetry_data == *"dhclient"* ]]; then
        verify_tools "dhcpcd"
        # Return a soft warning that dhcp might not work if both are absent.
        # Not sending tools missing error as it will be hard failure.
    fi

    if [[ $telemetry_data == *"systemctl"* ]]; then
        verify_tools "service"
        # Return a soft warning that guest agent installation might not work if both are absent.
    fi
}

verify_uefi_bootloader_files()
{
    config_file_name=""
    bootloader_folder_path="$chroot_path/boot/efi/EFI/boot"

    if [ "$firmware_type" = "UEFI" ]; then
        if [ ! -f "$_grub2_efi_path/bootx64.efi" ]; then
            if [ -f "$_grub2_efi_path/shimx64.efi" ]; then
                copy_file "$_grub2_efi_path/shimx64.efi" "$_grub2_efi_path/bootx64.efi"
            elif [ -f "$_grub2_efi_path/grubx64.efi" ]; then
                copy_file "$_grub2_efi_path/grubx64.efi" "$_grub2_efi_path/bootx64.efi"
            elif [ -f "$_grub2_efi_path/grub.efi" ]; then
                copy_file "$_grub2_efi_path/grub.efi" "$_grub2_efi_path/bootx64.efi"
            else
                if [ ! -d "$bootloader_folder_path" ] || [ ! -f "$bootloader_folder_path/bootx64.efi" ]; then
                    echo "efi file is absent on source disk which will lead to boot failure in Gen2 vm."
                    add_telemetry_data "bootx64.efi"
                    add_am_hydration_log "Missing UEFI firmware .efi file" "$bootloader_folder_path/bootx64.efi"
                    # Treat the VM as BIOS to increase chances of boot
                    # This may happpen for customers who may have accidentally placed grub.cfg 
                    # in /boot/efi/EFI/<distribution> folder. Warn the customer.
                    firmware_type="BIOS"
                    return
                fi
            fi
        fi

        if [ -f "$_grub2_efi_path/grub.cfg" ]; then
            config_file_name="grub.cfg"
            copy_file "$_grub2_efi_path/grub.cfg" "$_grub2_efi_path/bootx64.cfg"
        elif [ -f "$_grub2_efi_path/grub.conf" ]; then
            config_file_name="grub.conf"
            copy_file "$_grub2_efi_path/grub.conf" "$_grub2_efi_path/bootx64.conf"
        else
            echo "grub config file not found in grub folder."
        fi

        # Folder needs to be explicitly added for Ubuntu/Debian/RHEL6/CENTOS6.
        # Checking and adding for other distros too if not added.
        if [ ! -d "$bootloader_folder_path" ]; then
            CopyDirectory "$_grub2_efi_path/" "$bootloader_folder_path"
        else
            trace "BOOT folder is present on the disk."
            if [ ! -f  "$bootloader_folder_path/bootx64.efi" ]; then
                copy_file "$_grub2_efi_path/bootx64.efi" "$bootloader_folder_path/bootx64.efi"
            fi

            # bootx64.conf required for RHEL6/CENTOS6. Copying for other distros too.
            if [ $config_file_name = "grub.cfg" ] && [ ! -f "$bootloader_folder_path/bootx64.cfg" ]; then
                copy_file "$_grub2_efi_path/bootx64.cfg" "$bootloader_folder_path/bootx64.cfg"
            elif [ $config_file_name = "grub.conf" ] && [ ! -f "$bootloader_folder_path/bootx64.conf" ]; then
                copy_file "$_grub2_efi_path/bootx64.conf" "$bootloader_folder_path/bootx64.conf"
            fi
        fi
    else
        trace "Boot folder verification not required for BIOS."
    fi

}

#@1 - Kernel version
#@2 - Kernel image file path
mkinitrd_generate_initrd_image()
{
    trace "Updating $1's image $2"
    newstr=`sed  -n "/^[[:space:]]*INITRD_MODULES*/p " $chroot_path/etc/sysconfig/kernel`

    hypervdrivers="hv_vmbus hv_storvsc hv_netvsc"
    for driver_name in $hypervdrivers
    do
        echo "$newstr"|grep -q "$driver_name"
        if [[ $? -ne 0 ]]; then
            echo "HyperV drivers are not present, adding them to INITRD_MODULES."
            newstr=`echo $newstr|awk -F\" ' {print $1"\""$2 " hv_vmbus hv_storvsc hv_netvsc" "\""}'`
            sed  -i  "s/^[[:space:]]*INITRD_MODULES=\".*\"/$newstr/" $chroot_path/etc/sysconfig/kernel
            break
        fi
    done

    mount_runtime_partitions
    sleep 5
    chroot $chroot_path mkinitrd
}

verify_generate_initrd_images()
{
    case "$src_distro" in
    UBUNTU14*|UBUNTU16*|UBUNTU18*|UBUNTU19*|UBUNTU20*|DEBIAN*)
        # Supported Ubuntu & Debian distros will have
        # hyper-v drivers build-in to the kernel image,
        # so skipping this step for these distros.
        add_am_hydration_log "Kernel image Hyper-V updates" "Skipped for Ubuntu"
        return 0
        ;;
    *)
        # Will verify and regenerate initrd images for rest
        # of the supported distros, and control won't come
        # this far for unsupported distros.
        ;;
    esac
    
    trace "$(ls -d ${chroot_path}/lib/modules/*)"
    
    # Version sort the kernel images.
    latest_kernel_in_use=$(ls $chroot_path/boot/vmlinuz* | sed 's/\/mnt\/sms_azure_chroot\/boot\/vmlinuz-//' | sed 's/[.-][[:alpha:]][[:alnum:][:punct:]]*//' | sort -V | tail -n 1)
    trace "Latest Kernel in use: $latest_kernel_in_use"

    add_am_hydration_log "Latest Kernel" "$latest_kernel_in_use"

    local last_error_in_generate_initrd_image=0
    for mod_path in $(ls -d ${chroot_path}/lib/modules/*)
    do
        [[ ! -d $mod_path ]] && continue
        [[ ! -f $mod_path/modules.dep ]] && continue
        
        local kernel_version=$(basename $mod_path)
        is_kernel_in_use $kernel_version
        if [[ $? -ne 0 ]]; then
            trace "$kernel_version not found in grub.cfg"
            if [[ $kernel_version != *"$latest_kernel_in_use"* ]]; then
                trace "Skipping $kernel_version."
                add_am_hydration_log "$kernel_version" "Skipped. Inactive kernel."
                continue
            else
                add_am_hydration_log "$kernel_version" "Modified. Latest Kernel"
                trace "Latest kernel $kernel_version is in use. It won't be skipped."
            fi
        else
            add_am_hydration_log "$kernel_version" "Modifying. Active Kernel"
        fi

        # Checking if hyper-v drivers are available
        # in the kernel, if not then error will be thrown.
        verify_hv_drivers_in_module $kernel_version
        
        for kernel_image_file in $(ls ${chroot_path}/boot/initr*${kernel_version}*)
        do
            is_kernel_image_in_use $kernel_image_file
            if [[ $? -ne 0 ]] ; then 
                trace "$kernel_image_file is not present in grub.cfg"
                if [[ $kernel_version == *"$latest_kernel_in_use"* ]] && [[ $kernel_image_file == *"initr"*"$kernel_version.img"* ]]; then
                    # Add hyper-V drivers in the primary image file of the kernel in use.
                    trace "$kernel_version seems to be in use and won't be skipped"
                    add_am_hydration_log "$kernel_image_file Kernel Image" "Modifying. Kernel in use."
                else
                    trace "Skipping $kernel_version's image file $kernel_image_file"
                    add_am_hydration_log "$kernel_image_file Kernel Image" "Skipped. Kernel not in use."
                    continue
                fi
            fi
            
            verify_hv_drivers_in_kernel_image $kernel_image_file
            [[ $? -eq 0 ]] && continue

            trace "Re-generating $kernel_image_file with hyper-v drivers"

            if [[ $telemetry_data != *"dracut"* ]]; then
                generate_initrd_image $kernel_version $kernel_image_file
            elif [[ $telemetry_data != "mkinitrd" ]]; then
                mkinitrd_generate_initrd_image $kernel_version $kernel_image_file
            else
                trace "Kernel Image doesn't have the necessary drivers."
                trace "dracut and mkinitrd - tools to update kernel images with hyper-V drivers were found absent"
                trace "Fail the hydration with tools missing error."

                add_am_hydration_log "$kernel_image_file Kernel image update failure" "No dracut or mkinitrd."

                if [[ $src_distro == *"SLES11"* ]]; then
                    throw_error $_E_AZURE_SMS_TOOLS_MISSING "mkinitrd"
                else
                    throw_error $_E_AZURE_SMS_TOOLS_MISSING "dracut"
                fi
            fi

            add_am_hydration_log "$kernel_image_file Kernel Image" "Hyper-V drivers update - Success."

        done
    done
}

#@1 - Config value
#@2 - File name
# Appends an entire line to the file if absent
append_config_line_parameter()
{
    _line_number=$(sed -n "/$1/=" "$2")
    if [[ ! -z $_line_number ]]; then
        echo "$1 is already present in $2 at $_line_number"
    else
        echo "$1 is absent in $2. Appending the line"
        sed -i "2i$1" "$2"
    fi
}

# Enable Getty on ttyS0 for SLES11
set_sles11_inittab()
{
    uncomment_config_setting "S0:12345:respawn" "$chroot_path/etc/inittab"
    chroot $chroot_path "telinit q"
}

set_serial_console_grub_options()
{
    trace "Updating boot grub options to redirect serial console logs ..."
    
    local opts_to_remove="rhgb quiet crashkernel=auto acpi=0"
    local opts_to_add="rootdelay=300 earlyprintk=ttyS0 console=ttyS0 numa=off"
    case $src_distro in
    RHEL6*|CENTOS6*|OL6*)
        modify_grub_config "$opts_to_add" "$opts_to_remove"
        ;;
    SLES11*)
        modify_grub_config "$opts_to_add" "$opts_to_remove"
        set_sles11_inittab
        ;;
    UBUNTU14*|UBUNTU16*|UBUNTU18*|UBUNTU19|UBUNTU20*)
        opts_to_add="splash quiet rootdelay=300 earlyprintk=ttyS0,115200 console=ttyS0,115200n8 console=tty1"
        modify_grub2_config "$opts_to_add" "GRUB_CMDLINE_LINUX_DEFAULT" "$opts_to_remove"
        modify_grub2_config "$opts_to_add" "GRUB_CMDLINE_LINUX" "$opts_to_remove"
        modify_grub2_config "console serial" "GRUB_TERMINAL_OUTPUT" ""
        modify_grub2_config "--stop=1 --parity=no --word=8 --unit=0 --speed=115200 serial" "GRUB_SERIAL_COMMAND" ""
        modify_grub2_config "console serial" "GRUB_TERMINAL" ""
        ;;
    DEBIAN*)
        opts_to_add="splash quiet rootdelay=300 earlyprintk=ttyS0,115200 console=ttyS0,115200n8 console=tty1"
        modify_grub2_config "$opts_to_add" "GRUB_CMDLINE_LINUX_DEFAULT" "$opts_to_remove"
        modify_grub2_config "$opts_to_add" "GRUB_CMDLINE_LINUX" "$opts_to_remove"
        modify_grub2_config "console serial" "GRUB_TERMINAL_OUTPUT" ""
        modify_grub2_config "--stop=1 --parity=no --word=8 --unit=0 --speed=115200 serial" "GRUB_SERIAL_COMMAND" ""
        modify_grub2_config "console serial" "GRUB_TERMINAL" ""
        ;;
    SLES12*|SLES15*)
        opts_to_add="rootdelay=300 earlyprintk=ttyS0 console=ttyS0"
        modify_grub2_config "$opts_to_add" "GRUB_CMDLINE_LINUX"
        modify_grub2_config "console serial" "GRUB_TERMINAL_OUTPUT" ""
        modify_grub2_config "console serial" "GRUB_TERMINAL" ""
        ;;
    RHEL7*|CENTOS7*|OL7*|OL8*|RHEL8*|CENTOS8*)
        #Following options are taken from official Azure document for RHEL7.
        #https://docs.microsoft.com/en-us/azure/virtual-machines/troubleshooting/serial-console-grub-single-user-mode#grub-access-in-rhel
        #and 
        #https://docs.microsoft.com/en-us/azure/virtual-machines/troubleshooting/serial-console-grub-proactive-configuration
        opts_to_add="rootdelay=300 console=tty1 console=ttyS0,115200n8 earlyprintk=ttyS0,115200 earlyprintk=ttyS0 net.ifnames=0"
        modify_grub2_config "$opts_to_add" "GRUB_CMDLINE_LINUX" "$opts_to_remove"
        modify_grub2_config "console serial" "GRUB_TERMINAL_OUTPUT" ""
        modify_grub2_config "--stop=1 --parity=no --word=8 --unit=0 --speed=115200 serial" "GRUB_SERIAL_COMMAND" ""
        # This value should be Serial console. But our text replacement code reverses order
        # So writing console serial which should become serial console when committed.
        modify_grub2_config "console serial" "GRUB_TERMINAL" ""
        ;;
    *)
        throw_error $_E_AZURE_SMS_OS_UNSUPPORTED $src_distro
        ;;
    esac
    trace "Successfully updated boot grub options!"
}

enable_systemd_on_startupscipt()
{
    local _systemd_cofig_file="$chroot_path/lib/systemd/system/${_AM_STARTUP_}.service"
    if [[ ! -d $chroot_path/lib/systemd ]]; then
        _systemd_cofig_file="$chroot_path/usr/lib/systemd/system/${_AM_STARTUP_}.service"
    fi
    
    if [[ -f $_systemd_cofig_file ]]; then
        trace "Startup script is already configured!"
        return 0
    fi
        
    echo "[Unit]" > $_systemd_cofig_file
    echo "Description=Azure migrate startup script to create nic config." >> $_systemd_cofig_file
    echo "After=network.target" >> $_systemd_cofig_file
    echo "" >> $_systemd_cofig_file
    echo "[Service]" >> $_systemd_cofig_file
    echo "ExecStart=${_AM_SCRIPT_DIR_}/${_STARTUP_SCRIPT_} start" >> $_systemd_cofig_file
    echo "" >> $_systemd_cofig_file
    echo "[Install]" >> $_systemd_cofig_file
    echo "WantedBy=default.target" >> $_systemd_cofig_file

    trace "Adding startup script."
    chroot $chroot_path systemctl enable "${_AM_STARTUP_}.service"
    if [[ $? -eq 0 ]]; then
        trace "Successfully added startup script!"
    else
        trace "WARNING: Could not add startup script for DHCP."
    fi

    return 0
}

enable_chkconfig_on_startupscript()
{
    local _chkconfig_startup_file="/etc/init.d/${_AM_STARTUP_}"
    if [[ -h "${chroot_path}$_chkconfig_startup_file" ]]; then
        trace "Startup script is already configured!"
        return;
    fi
    
    local _startup_script_path="${_AM_SCRIPT_DIR_}/${_STARTUP_SCRIPT_}"
    chroot $chroot_path ln -s $_startup_script_path $_chkconfig_startup_file
    if [[ $? -ne 0 ]]; then
        trace "WARNING: Could not create start-up script."
        return 0
    fi
    
    local _config_startup_cmd=""
    if chroot $chroot_path which chkconfig > /dev/null 2>1$ ;then
        _config_startup_cmd="chroot $chroot_path chkconfig --add ${_AM_STARTUP_}"
    elif chroot $chroot_path which update-rc.d > /dev/null 2>1$ ;then
        _config_startup_cmd="chroot $chroot_path update-rc.d ${_AM_STARTUP_} defaults"
    else
        trace "WARNING: No tools found to configure start-up script."
        return 0
    fi
    
    trace "Adding startup script."    
    $_config_startup_cmd > /dev/null 2>&1
    if [[ $? -ne 0 ]]; then
        trace "WARNING: Could not add start-up script."
    else
        trace "Successfully added startup script!"
    fi
    
    return 0
}

create_startup_scripts()
{
    trace "Adding startup script to configure dhcp on migrated azure vm."
    local startup_script_dir=$1
    [[ -d $startup_script_dir ]] || mkdir $startup_script_dir 
    if [[ ! -d $startup_script_dir ]]; then
        trace "WARNING: Could not create ${startup_script_dir} for startup script."
        return 1
    fi
    
    local working_dir=$(cd $(dirname "$0") > /dev/null && pwd)
    [[ -d "$working_dir" ]] || working_dir=/usr/local/AzureRecovery
    
    copy_file "$working_dir/$_FIX_DHCP_SCRIPT_" "${startup_script_dir}/${_FIX_DHCP_SCRIPT_}"
    if [[ $? -ne 0 ]]; then
        trace "WARNING: Could not copy scripts to startup script directory."
        return 1
    fi
    
    # Startup script file
    local _startup_script_path="$startup_script_dir/$_STARTUP_SCRIPT_"
    echo "#!/bin/bash
# This is for RHEL systems
# processname: ${_AM_STARTUP_}
# chkconfig: 2345 90 90
# description: Azure migrate startup script to configure networks

### BEGIN INIT INFO
# Provides: ${_AM_STARTUP_}
# Required-Start: \$local_fs
# Required-Stop: \$local_fs
# X-Start-Before: \$network
# X-Stop-After: \$network
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Description: Azure migrate startup script to configure networks
### END INIT INFO " > $_startup_script_path

    echo 'case "$1" in' >> $_startup_script_path
    echo "start)" >> $_startup_script_path
    echo "${_AM_SCRIPT_DIR_}/${_FIX_DHCP_SCRIPT_} > ${_AM_SCRIPT_LOG_FILE_} 2>&1 ;;" >> $_startup_script_path
    echo "*)" >> $_startup_script_path
    echo ";;" >> $_startup_script_path
    echo "esac" >> $_startup_script_path
    echo "" >> $_startup_script_path
    
    # Execute permissions for the scripts
    chmod +x $startup_script_dir/*
    return $?
}

enable_installga_chkconfig()
{
    trace "Adding guest agent installation service inside /etc/init.d"
    local asr_installga_exfile_path="$chroot_path/etc/init.d/${_AM_INSTALLGA_}"
    echo "#!/bin/bash
# This is for RHEL systems
# processname: ${_AM_INSTALLGA_}
# chkconfig: 2345 90 90
# description: Azure migrate startup script to configure networks

### BEGIN INIT INFO
# Provides: ${_AM_INSTALLGA_}
# Required-Start: \$local_fs
# Required-Stop: \$local_fs
# X-Start-Before: \$network
# X-Stop-After: \$network
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Description: Azure migrate startup script to configure networks
### END INIT INFO " > $asr_installga_exfile_path

    echo 'case "$1" in' >> $asr_installga_exfile_path
    echo "start)" >> $asr_installga_exfile_path
    echo "bash /$base_linuxga_path/InstallLinuxGuestAgent.sh $src_distro \"$base_linuxga_path\" $setup_tool_install" >> $asr_installga_exfile_path
    echo ";;" >> $asr_installga_exfile_path
    echo "*)" >> $asr_installga_exfile_path
    echo ";;" >> $asr_installga_exfile_path
    echo "esac" >> $asr_installga_exfile_path
    echo "" >> $asr_installga_exfile_path
    
    # Execute permissions for the scripts
    chmod +x $asr_installga_exfile_path

    local _chkconfig_gastartup_file="/etc/init.d/${_AM_INSTALLGA_}"
    if [[ -h "${chroot_path}$_chkconfig_gastartup_file" ]]; then
        trace "Startup script is already configured!"
        return;
    fi

    chroot $chroot_path ln -s $asr_installga_exfile_path $_chkconfig_gastartup_file
    if [[ $? -ne 0 ]]; then
        trace "WARNING: Could not create start-up script."
    fi
    
    local _config_startup_cmd=""
    if chroot $chroot_path which chkconfig > /dev/null 2>1$ ;then
        _config_startup_cmd="chroot $chroot_path chkconfig --add ${_AM_INSTALLGA_}"
    elif chroot $chroot_path which update-rc.d > /dev/null 2>1$ ;then
        _config_startup_cmd="chroot $chroot_path update-rc.d ${_AM_INSTALLGA_} defaults"
    else
        trace "WARNING: No tools found to configure start-up script."
        add_telemetry_data "startup"
        return 0
    fi
    
    trace "Adding startup script."
    $_config_startup_cmd > /dev/null 2>&1
    if [[ $? -ne 0 ]]; then
        trace "WARNING: Could not add start-up script."
        add_telemetry_data "startup"
    else
        trace "Successfully added startup script!"
    fi

    return $?
}

add_startup_script_for_dhcp()
{
    local startup_script_dir="${chroot_path}${_AM_SCRIPT_DIR_}"
    create_startup_scripts $startup_script_dir
    if [[ $? -ne 0 ]]; then
        trace "WARNING: Skip configuring startup script."
        return 0;
    fi
    
    case $src_distro in
        RHEL6*|CENTOS6*|OL6*|SLES11*|UBUNTU14*|DEBIAN7*)
            enable_chkconfig_on_startupscript
            ;;
        SLES12*|SLES15*|DEBIAN8*|DEBIAN9*|UBUNTU16*|RHEL7*|CENTOS7*|OL7*|OL8*|RHEL8*|CENTOS8*|UBUNTU18*|UBUNTU19*|UBUNTU20*)
            enable_systemd_on_startupscipt
            ;;
        *)
            trace "WARNING: Unknown distro '$src_distro' to configure startup script."
            
            trace "Removing startup script directory '$startup_script_dir'."
            rm -rf $startup_script_dir
            ;;
    esac
}

enable_postlogin_installga()
{
    target_profiled_file="$chroot_path/etc/profile.d/ASRLinuxGAStartup.sh"

    if [[ ! -d "$chroot_path/etc/profile.d" ]]; then
        trace "/etc/profile.d directory doesn't exist"
    else
        echo "#!/bin/bash" >> $target_profiled_file
        echo "echo \"This is Azure Migrate One-Time Guest Agent Installation Setup.\"" >> $target_profiled_file
        echo "echo \"The service quietly exits if WALinuxAgent is already installed.\"" >> $target_profiled_file
        echo "echo \"If guest agent installation failed at pre-boot step due to python/installation prereqs not fulfilled, \"" >> $target_profiled_file
        echo "echo \"this service is triggered post login to install WALinuxAgent using VM's repository.\"" >> $target_profiled_file
        echo "echo \"The service may prompt you to login using elevated bash shell for installation to proceed.\"" >> $target_profiled_file
        echo "echo \"You can skip the installation, and can install manually, or retrigger the service by running\"" >> $target_profiled_file
        echo "echo \"systemctl start asr-installga or service asr-installga start\"" >> $target_profiled_file
        echo "bash /$base_linuxga_path/InstallLinuxGuestAgent.sh $src_distro \"$base_linuxga_path\" $setup_tool_install >> /$base_linuxga_path/ASRLinuxGA.log" >> $target_profiled_file
        echo "echo \"Installation process completed. Proceed with Login.\"" >> $target_profiled_file
        echo "rm -f \"/etc/profile.d/ASRLinuxGAStartup.sh\"" >> $target_profiled_file

        trace "/etc/profile.d post login startup file created."	
    fi
}

enable_installga_service()
{
    # Startup script to install azure guest agent.
    local asr_installga_exfile_path="usr/local/${_AM_INSTALLGA_}/asr.installga"

    # Create a directory if it doesn't exist.
    if [[ ! -d "$chroot_path/usr/local/${_AM_INSTALLGA_}" ]]; then
        trace "Creating a new directory /usr/local/${_AM_INSTALLGA_}"
        mkdir "$chroot_path/usr/local/${_AM_INSTALLGA_}"
    fi
    
    if [[ -f $chroot_path/$asr_installga_exfile_path ]]; then
        trace "/usr/local/${_AM_INSTALLGA_}/asr.installga exists. Deleting the file."
        rm $chroot_path/$asr_installga_exfile_path
    fi
 
    # Write execution commands for asr.installga executable.
    trace "Creating asr.installga for execution during boot."

    setup_tool_install="install_setup_tools_false"
    if [[ $telemetry_data == *"no-setuptools"* ]]; then
        setup_tool_install="install_setup_tools_true"
    fi

    echo "#!/bin/bash" >> $chroot_path/$asr_installga_exfile_path
    echo "#This file is generated for execution during boot" >> $chroot_path/$asr_installga_exfile_path
    echo "bash /$base_linuxga_path/InstallLinuxGuestAgent.sh $src_distro \"$base_linuxga_path\" $setup_tool_install >> /$base_linuxga_path/ASRLinuxGA.log" >> $chroot_path/$asr_installga_exfile_path
    echo "exit 0" >> $chroot_path/$asr_installga_exfile_path

    # Add executable permissions.
    chmod +x $chroot_path/$asr_installga_exfile_path

    # Check for systemd path. The path may vary among linux distros.
    local _systemd_config_file="$chroot_path/lib/systemd/system/${_AM_INSTALLGA_}.service"
    if [[ ! -d $chroot_path/lib/systemd/system ]]; then
        if [[ -d $chroot_path/usr/lib/systemd/system ]]; then
            _systemd_config_file="$chroot_path/usr/lib/systemd/system/${_AM_INSTALLGA_}.service"
        elif [[ -d $chroot_path/etc/systemd/system ]]; then
            _systemd_config_file="$chroot_path/etc/systemd/system/${_AM_INSTALLGA_}.service"
        fi
    fi

    # Delete the file if it already exists
    if [[ -f $_systemd_config_file ]]; then
        trace "$_systemd_config_file exists. Deleting the file."
        rm $_systemd_config_file
    fi

    trace "Creating systemd config file. $_systemd_config_file"

    echo "[Unit]" >> $_systemd_config_file
    echo "Description=Install Azure Linux Guest agent startup script by ASR" >> $_systemd_config_file
    echo "ConditionPathExists=/$asr_installga_exfile_path" >> $_systemd_config_file
    echo "" >> $_systemd_config_file
    echo "[Service]" >> $_systemd_config_file
    echo "Type=forking" >> $_systemd_config_file
    echo "ExecStart=/$asr_installga_exfile_path start" >> $_systemd_config_file
    echo "TimeoutSec=0" >> $_systemd_config_file
    echo "StandardOutput=tty" >> $_systemd_config_file
    echo "RemainAfterExit=no" >> $_systemd_config_file
    echo "SysVStartPriority=99" >> $_systemd_config_file
    echo "" >> $_systemd_config_file
    echo "[Install]" >> $_systemd_config_file
    echo "WantedBy=multi-user.target" >> $_systemd_config_file

    chroot $chroot_path systemctl enable "${_AM_INSTALLGA_}.service"

    if [[ $? -ne 0 ]]; then
        # CommandNotFound case if systemctl is not installed on the VM.
        chroot $chroot_path service "${_AM_INSTALLGA_}.service" start
    fi

    gacheck=$(cat $_systemd_config_file)
    trace "systemd service file: $gacheck"
    
    gacheck=$(cat $chroot_path/$asr_installga_exfile_path)
    trace "asr.installga: $gacheck"
}

unset pythonver
check_valid_python_version()
{
    # Check for Major Version 3 for python.
    chroot $chroot_path python3 --version
    if [[ $? -ne 0 ]] ; then
        trace "Python3 is not installed on the target VM."
    else
        pythonver=3
        add_am_hydration_log "Python Version" "python3"
		return
    fi

    # Check for Major version 2 and Minor Version >= 6 for Python.
    chroot $chroot_path python --version
    if [[ $? -ne 0 ]] ; then
        trace "Python is not installed on the target VM.\n"
        pythonver=1
		return
    else
        vermajor=$(chroot $chroot_path python -c"import platform; major, minor, patch = platform.python_version_tuple(); print(major)")
        verminor=$(chroot $chroot_path python -c"import platform; major, minor, patch = platform.python_version_tuple(); print(minor)")
        trace "Major Version: $vermajor Minor Version: $verminor"
        if [ $vermajor -eq 2 ] && [ $verminor -ge 6 ]; then
            pythonver=2
            add_am_hydration_log "Python Version" "${vermajor}.${verminor}"
            return
        else
            trace "Failed to install Linux Guest Agent on the VM.\n"
            trace "Install python version 2.6+ to install Guest Agent.\n"
            add_am_hydration_log "Python Version incompatible for WALinuxAgent" "${vermajor}.${verminor}"
            pythonver=1
            return
        fi
    fi

    add_am_hydration_log "Python Version absent" "WALinuxAgent installation prereq FAILED."

    pythonver=1
}

# $1: Complete path of zip file.
# $2: Target directory for extraction of zip file.
extract_zipFile_using_python()
{
    trace "Extracting zip from $1 to $2"

    # Create python script to extract the zip file.
    local unzip_py="$chroot_path/${base_linuxga_path}/unzip.py"
    echo '#!/usr/bin/python' > $unzip_py
    echo 'import sys' >> $unzip_py
    echo 'from zipfile import ZipFile' >> $unzip_py
    echo 'from zipfile import BadZipfile' >> $unzip_py
    echo 'try:' >> $unzip_py
    echo "    zip_file = \"$1\"" >> $unzip_py
    echo "    dest_dir = \"$2\"" >> $unzip_py
    echo '    pzf = ZipFile(zip_file)' >> $unzip_py
    echo '    pzf.extractall(dest_dir)' >> $unzip_py
    echo 'except BadZipfile:' >> $unzip_py
    echo '    print("Error: Bad zip file format.")' >> $unzip_py
    echo '    sys.exit(1)' >> $unzip_py
    echo 'else:' >> $unzip_py
    echo '    print("Successfully extracted the zip file.")' >> $unzip_py
    echo '    sys.exit(0)' >> $unzip_py
    
    # Run python script using python3 present on hydration VM.
    python3 $unzip_py >> "$chroot_path/$base_linuxga_path/ASRLinuxGA.log" 2>&1

    return $?
}

# $1: python version command
test_setuptools_prereq()
{
    setup_uuid=$(uuidgen)
    setuptools_file_path="$chroot_path/$base_linuxga_path/setuptools-test-$setup_uuid.py"

    echo "import sys"                   >> $setuptools_file_path
    echo "try:"                         >> $setuptools_file_path
    echo "        import setuptools"    >> $setuptools_file_path
    echo "except ImportError:"          >> $setuptools_file_path
    echo "        print(\"ABSENT\")"    >> $setuptools_file_path
    echo "else:"                        >> $setuptools_file_path
    echo "        print(\"PRESENT\")"   >> $setuptools_file_path

    setuptools_output=""
    if [[ "$1" == "python3" ]]; then
        setuptools_output=$(chroot $chroot_path python3 "/$base_linuxga_path/setuptools-test-$setup_uuid.py")
    else
        setuptools_output=$(chroot $chroot_path python "/$base_linuxga_path/setuptools-test-$setup_uuid.py")
    fi

    setup_tools_package_path="/usr/local/AzureRecovery/setuptools-33.1.1.zip"

    if [[ "$setuptools_output" == "ABSENT" ]]; then
        add_am_hydration_log "python setuptools" "ABSENT"
        trace "setuptools is absent on the VM. Trying manual pre-boot installation."

        extract_zipFile_using_python $setup_tools_package_path $chroot_path/$base_linuxga_path/
        add_telemetry_data "no-setuptools"
    else
        trace "setuptools is present on the VM."
    fi

    rm -f $setuptools_file_path
}

validate_guestagent_prereqs()
{
    check_valid_python_version

    if [[ $pythonver -eq 1 ]]; then
        trace "Python not installed/ incompatible with linux guest agent requirements."
        add_telemetry_data "no-python"
    elif [[ $pythonver -eq 3 ]]; then
        trace "Python3 present on the source VM."
        add_telemetry_data "python3"
        test_setuptools_prereq "python3"
    elif [[ $pythonver -eq 2 ]]; then
        trace "Python 2.6+ installed on the source VM."
        add_telemetry_data "python2"
        test_setuptools_prereq "python"
    else
        trace "Unsupported Python version"
        add_telemetry_data "no-python"
    fi
}

fix_network_config()
{
    if [[ $telemetry_data == *"dhclient"* ]] && [[ $telemetry_data == *"dhcpcd"* ]]; then
        trace "Both dhclient and dhcpcd are absent on the source VM."
        trace "Unable to find a tool to set dhcp on the Azure VM."
        # Create the ifcfg-eth0 file nevertheless and send warning about consequences.
    fi

    #TODO: Needs some refactoring. Will add detailed customer logs accordingly.
    add_am_hydration_log "Networking Changes" "Enabling DHCP for the machine"

    trace "Making network changes ..."
    case $src_distro in
    RHEL6*|CENTOS6*|OL6*)
        remove_network_manager_rpm
        update_network_file
        enable_network_service
        configure_dhcp_rhel
        add_startup_script_for_dhcp
    ;;
    SLES11*|SLES12*|SLES15*)
        remove_network_manager_rpm
        update_network_dhcp_file
        configure_dhcp_sles
        add_startup_script_for_dhcp
    ;;
    UBUNTU14*|DEBIAN*|UBUNTU16*)
        remove_persistent_net_rules
        configure_dhcp_ubuntu
        add_startup_script_for_dhcp
    ;;
    UBUNTU18*|UBUNTU19*|UBUNTU20*)
        remove_persistent_net_rules
        create_dhcp_netplan_config_and_apply
    ;;
    RHEL7*|RHEL8*)
        update_network_file
        enable_network_service
        configure_dhcp_rhel
        add_startup_script_for_dhcp
    ;;
    CENTOS7*|OL7*|OL8*|CENTOS8*)
        reset_persistent_net_gen_rules
        update_network_file
        configure_dhcp_rhel
        add_startup_script_for_dhcp
    ;;
    *)
        throw_error $_E_AZURE_SMS_OS_UNSUPPORTED $src_distro
    ;;
    esac
    trace "Successfully completed network changes!"
}

update_lvm_conf_to_allow_all_device_types()
{
    local _lvm_conf_file_="${chroot_path}/etc/lvm/lvm.conf"
    
    if [[ -f $_lvm_conf_file_ ]]; then
        trace "$_lvm_conf_file_ not found."
        return 0
    fi
    
    # Modify lvm.conf file to include all devices.
    local _line_=$(cat "$_lvm_conf_file_" | grep -P "^(\s*)filter(\s*)=(\s*).*")
    if [[ ! -z "$_line_" ]]; then
        backup_file $_lvm_conf_file_
        _line_number_=$(cat "$_lvm_conf_file_" | grep -P -n "^(\s*)filter(\s*)=(\s*).*" | awk -F":" '{print $1}')
        trace "Modifying filter in $_lvm_conf_file_ (line: $_line_number_)"
        sed -i "$_line_number_ c\filter=\"a\/.*\/\"" $_lvm_conf_file_
        return $?
    fi

    return 0
}

update_root_device_uuid_in_boot_cmd()
{
    local _boot_cmd_starts_with_="linux"
    local _grub_file_="${chroot_path}/boot/grub2/grub.cfg"
    
    case $src_distro in
        CENTOS6*|OL6*|RHEL6*|SLES11*)
            _grub_file_="${chroot_path}/boot/grub/menu.lst"
            if [ "$firmware_type" = "UEFI" ]; then
                _grub_file_="$_grub2_efi_path/menu.lst"
            fi
            _boot_cmd_starts_with_="kernel"
        ;;
        UBUNTU*|DEBIAN*)
            _grub_file_="${chroot_path}/boot/grub/grub.cfg"
            if [ "$firmware_type" = "UEFI" ]; then
                _grub_file_="$_grub2_efi_path/grub.cfg"
            fi
        ;;
    esac

    if [ "$firmware_type" = "UEFI" ]; then
        if [ -f "$_grub2_efi_path/grub.cfg" ]; then
            _grub_file_="$_grub2_efi_path/grub.cfg"
        elif [ -f "$_grub2_efi_path/menu.lst" ]; then
            _grub_file_="$_grub2_efi_path/menu.lst"
        elif [ -f "$_grub2_efi_path/grub.conf" ]; then
            _grub_file_="$_grub2_efi_path/grub.conf"
        fi
    fi
    
    local src_root_uuid=$(findmnt -nf -o UUID $chroot_path)
    if [[ $? -ne 0 ]]; then
        trace "could not find device UUID for the $chroot_path"
        return 0
    elif [[ -z $src_root_uuid ]]; then
        trace "root UUID not available, skipping root device update in grub file."
        return 0
    fi
    
    exec 4<$_grub_file_
    while read -u 4 -r _line_
    do
        # Ignore commented lines
        if [[ $_line_ =~ ^# ]]; then
            continue
        fi
        
        local _dev_name_=$(echo $_line_ |\
                         grep "${_boot_cmd_starts_with_}.*root=\/dev\/" |\
                         awk -F"root=" '{print $2}' | awk '{print $1}')

        [[ -z $_dev_name_ ]] && continue
        
        if [[ $_dev_name_ =~ /dev/[x]*[svh]d.* ]]; then
            trace "Replacing $_dev_name_ with $src_root_uuid"
            local safe_device=$(printf "%s\n" "$_dev_name_" |\
                         sed 's/[][\.*^$(){}?+|/]/\\&/g')
            sed -i --follow-symlinks "s/${safe_device}/UUID=$src_root_uuid/" $_grub_file_
        else
            trace "$_dev_name_ is not a standard partition name, no need to replace with UUID."
        fi
        
        # TODO: Remove "resume=" option if its referring to standard device name.
    done
    4<&-
}

main()
{
    local error_in_generate_initrd_image=0
    validate_script_input "$@"
    
    verify_src_os_version

    get_firmware_type

    mount_runtime_partitions

    verify_requited_tools

    verify_generate_initrd_images
    
    set_serial_console_grub_options

    verify_uefi_bootloader_files

    update_root_device_uuid_in_boot_cmd
    
    fix_network_config
    
    update_lvm_conf_to_allow_all_device_types

    install_linux_guest_agent

    # Most Hard failures will be immediately thrown.
    # Return Soft Failures, call failure checks in increasing order of priority
    # So that most critical failure is shown to the customer.

    final_error_code="0"
    final_error_data=""
    if [[ $telemetry_data == *"systemctl"* ]] && [[ $telemetry_data == "service" ]]; then
        final_error_code="$_E_AZURE_GA_INSTALLATION_FAILED"
        final_error_data="systemctl"
    fi

    if [[ $telemetry_data == *"no-python"* ]]; then
        final_error_code="$_E_AZURE_GA_INSTALLATION_FAILED"
        final_error_data="no-python"
    fi

    if [[ $telemetry_data == *"dhclient"* ]] && [[ $telemetry_data == *"dhcpcd"* ]]; then
        final_error_code="$_E_AZURE_ENABLE_DHCP_FAILED"
        final_error_data="dhclient"
    fi

    if [[ $telemetry_data == *"bootx64.efi"* ]]; then
        final_error_code="$_E_AZURE_SMS_CONF_MISSING"
        final_error_data="bootx64.efi"
    fi

    add_am_hydration_log "---Hydration Log End---" ""

    cat "${chroot_path}${_AM_HYDRATION_LOG_}$am_telem_suffix"

    if [[ $final_error_code -ne 0 ]]; then
        throw_error $final_error_code $final_error_data
    else
        # Log Telemetry Data
        echo "[Sms-Telemetry-Data]:${telemetry_data}"
    fi
}

main "$@"