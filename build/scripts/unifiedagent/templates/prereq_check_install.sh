#!/usr/bin/env bash
#//--------------------------------------------------------------------
#//  <copyright file="prereq_check_install.sh company="Microsoft">
#//      Copyright (c) Microsoft Corporation. All rights reserved.
#//  </copyright>
#//
#//  <summary>
#//     This script checks pre-requistes to install linux agent
#//  </summary>
#//
#//--------------------------------------------------------------------

THIS_SCRIPT_DIR=`dirname $0`
JQ="${THIS_SCRIPT_DIR}/jq"
GFDISK="${THIS_SCRIPT_DIR}/inm_list_part"
PREREQ_CONF_FILE="${THIS_SCRIPT_DIR}/prereq_check_installer.json"
Supported_Kernels="${THIS_SCRIPT_DIR}/supported_kernels"
LOG_FILE="/var/log/ua_install.log"
TEMP_FILE="${THIS_SCRIPT_DIR}/prereq_check_temp_file"
MIN_ROOT_FREESPACE_KB=1048576
RETURN_VALUE=0
json_errors_file=
AGENT_ROLE=
OS=$(${THIS_SCRIPT_DIR}/OS_details.sh 1)
PLATFORM="VmWare"
WORKFLOW="VMWARE"
umask 0002

Existing_Check=(CLocaleAvailable HostNameSet SecureBoot FreeSpaceOnRoot LisDriversAvailable KernelSupported KernelDirectoryExists SystemDiskAvailable BootDiskAvailable LVMPackageInstalled BootOnLVMFromMultipleDisks BootPartitionMounted IsRootFsSupported IsBootFsSupported IsRootFsSupportedAzure IsBootFsSupportedAzure DirsWritable AcpiEqualsToZeroSet DeviceExistInGrubFiles GrubDeviceNameChange DhcpClientAvailable CheckMTKernelVersion OldOnePassEncryption RunlevelCommandCheck)

declare -a CheckName
declare -a CheckType
declare -a Params
NR_CHECKS=0
IDX=

declare -a MULIPATH_DEVICES
declare -a MULIPATH_DEVICES_DEVT
declare -a LINEAR_DEVICES
declare -a LINEAR_DEVICE_TYPE
declare -a DEVICES
NR_BOOTABLE_DISKS=0
BOOT_DISKS=
BOOT_FULL_DISKS=
SAME_BOOT_DISK=1
SAME_ROOT_DISK=1
SAME_PV=
NR_PVS=
PVS=
ROOT_DISKS=
ROOT_DEV=
BOOT_DEV=
NR_ROOT_DISKS=0
IS_ROOT_ON_LV=0
IS_LVM_PACKAGE_AVAILABLE=1
BOOT_CONFIGURED_ON_EFI_PARTITION=0
GRUB_FILE=
BOOT_FS=
ROOT_FS=
IS_ROOT_LVM=0
IS_BOOT_LVM=0

source ${THIS_SCRIPT_DIR}/teleAPIs.sh
SCENARIO_NAME="ProductPreReqChecks"


LOG()
{
    DATE_TIME=`date '+%m/%d/%Y %H:%M:%S'`
    echo "${DATE_TIME} : $@ " >> ${LOG_FILE} 2>&1
}

#
# Function Name: trace_log_message()
#
#  Description  Prints date and time followed by all arguments to
#               stdout and to LOG_FILE.  If -q is used, only send
#               message to log file, not to stdout.  If no arguments
#               are given, a blank line is returned to stdout and to
#               LOG_FILE.
#
#  Usage        trace_log_message [-q] "Message"
#
#  Notes        If -q is used, it must be the first argument.
#
trace_log_message()
{
    QUIET_MODE=FALSE

    if [ $# -gt 0 ]
    then
        if [ "X$1" = "X-q" ]
        then
            QUIET_MODE=TRUE
            shift
        fi
    fi

    if [ $# -gt 0 ]
    then
        DATE_TIME=`date '+%m/%d/%Y %H:%M:%S'`

        if [ "${QUIET_MODE}" = "TRUE" ]
        then
            echo "${DATE_TIME}: $*" >> ${LOG_FILE}
        else
            echo -e "$@"
            echo "${DATE_TIME} : $@ " >> ${LOG_FILE} 2>&1
        fi
    else
        if [ "${QUIET_MODE}" = "TRUE" ]
        then
            echo "" >> ${LOG_FILE}
        else
            echo "" | tee -a ${LOG_FILE}
        fi
    fi
}

mnt_to_dev()
{
    df $1 | sed -n 2p | cut -f 1 -d " "
}

_dev_to_fs()
{
    blkid -o value -s TYPE $1
}

num_dev_in_fs()
{
    local _ndev="1"
    local _fs=""

    _fs=$(_dev_to_fs $1)
    if [ "$_fs" = "btrfs" ]; then
        _ndev=$(btrfs filesystem show $1 | grep devid | wc -l)
    fi   

    echo $_ndev
}

# TODO: Should be _dev_to_fs. Remove it once multidisk btrfs support is enabled.
dev_to_fs()
{
    local _fs=""
    local _ndev=""

    _fs=$(_dev_to_fs $1)
    if [ "$_fs" = "btrfs" ]; then
        _ndev=$(num_dev_in_fs $1)
        LOG "BTRFS ($1): Num Disks = $_ndev"
        if [ $_ndev -ne 1 ]; then
            _fs="btrfs(multidevice)"
        fi
    fi

    echo $_fs 
}

usage()
{
    LOG "Usage of the program."
    LOG "prereq_check_install.sh -r <Agent Role, Agent or MasterTarget> -j <Output json file>"
}

# This function will detect the VM platform (Azure/VmWare)
detect_platform()
{
    tag=$(cat /sys/devices/virtual/dmi/id/chassis_asset_tag 2>/dev/null)
    if [ "$tag" = "7783-7084-3265-9085-8269-3286-77" ] && [ ! is_azure_stack_platform ] ; then
        LOG "The platform is Azure"
        PLATFORM="Azure"
    fi
}

log_to_json_file()
{
    log_to_json_errors_file $json_errors_file "$@"
}
# Make sure the required locale is available in host. Abort if does not exists.
check_c_locale_available()
{
    LOG "Checking for C locale availability"

    locale=`echo ${Params[${IDX}]} | ${JQ} '.Locale' | awk -F"\"" '{print $2}'`
    if [ -z "${locale}" ]; then
        LOG "Locale is not provided in the configuration JSON file"
        locale="C"
    fi

    SetOP "CheckCLocaleAvailable"
    if ! $(locale -a 2>> ${LOG_FILE} | grep -iqw ${locale}); then
        RETURN_VALUE=1
        LOG "C locale is not available. Install the locale and retry enable replication."
        log_to_json_file ASRMobilityServiceCLocaleNotAvailable "C locale is not available. Install the locale and retry enable replication."
        RecordOP 99 "C locale is not available. Install the locale and retry enable replication."
        return
    fi

    RecordOP 0 ""
}

# Make sure hostname is set. Otherwise abort.
check_hostname_for_null()
{
    LOG "Checking for Hostname set to NULL"

    SetOP "CheckHostNameSet"
    HOST_NAME=$(hostname 2>> ${LOG_FILE})
    if [ -z "$HOST_NAME" ]; then
        RETURN_VALUE=1
        LOG "The hostname is NULL. Cannot proceed with the installation."
        log_to_json_file ASRMobilityServiceHostNameNotSet "Mobility Service  Setup was unable to procced as the hostname on the VM is not set. Set the host name on the VM."
        RecordOP 99 "Hostname is NULL"
        return
    fi

    RecordOP 0 ""
}

# Make sure secure boot is not enabled.
check_secure_boot()
{
    LOG "Checking for secure boot"

    SetOP "CheckSecureBoot"
    which mokutil
    if [ $? -eq "0" ]; then
        local mokutil_output=`mokutil --sb-state 2>&1`
        if [[ $mokutil_output == *"SecureBoot enabled"* ]]; then
            RETURN_VALUE=1
            LOG "Secure Boot is enabled for the system. Cannot proceed with the installation."
            log_to_json_file ASRMobilityServiceSecureBootEnabled "Mobility Service setup was unable to procced as secure boot is enabled on the system." VMName $HOST_NAME
            RecordOP 99 "Secure Boot is enabled."
            return
        fi
    fi

    RecordOP 0 ""
}

# Check for runlevel. If it is null abort
check_runlevel()
{
	LOG "Checking Runlevel for null"
	
	SetOP "CheckRunlevel"
	# Check for systemd
    which systemctl > /dev/null 2>&1
    if [ $? -eq 0 ]
    then
        # The 'runlevel' command in systemd works different than
        # sysvinit runlevels. On systemd, the a specific runlevel
        # is only returned when it is reached, while sysvinit would
        # return it as soon as the switch was requested. IOW, runlevel
        # is changed at start in sysv init and at the end by systemd
        # so we cant rely on runlevel command for systemd
        #
        # We check if shutdown.target (common for reboot and poweroff)
        # is currently active to set appropriate runlevel. Current logic
        # does not distinguish between poweroff and reboot and always
        # marks runlevel=0 == poweroff. Similarly, it always returns
        # runlevel=3 == multiuser+nw even for graphical env or single
        # user mode. This needs to be changed if finer distinction is 
        # required.
        #
        systemctl list-units --type=target | grep start | grep -q "shutdown\.target" && RUNLEVEL=0 || RUNLEVEL=3
    else
        # RUNLEVEL variable is set only when this control script is invoked by init
        # Other times, we have to fetch the runlevel ourselves
        RUNLEVEL=${RUNLEVEL:-`/sbin/runlevel | cut -d' ' -f2`}
    fi
	
	if [ -z "$RUNLEVEL" ] || [ "$RUNLEVEL" -ne 2 -a "$RUNLEVEL" -ne 3 -a "$RUNLEVEL" -ne 5 ]; then
		if [ -z "$RUNLEVEL" ]; then
			runlevel="null"
		else runlevel=$RUNLEVEL
		fi
		RETURN_VALUE=1
		LOG "runlevel cannot be retrived, or is not equal to 2,3 or 5. Cannot proceed with installation. Current runlevel $runlevel"
		log_to_json_file ASRMobilityServiceRunlevelCommandFailure "Please make sure that runlevel of the system can be retrived and is set to 2, 3 or 5" Runlevel ${runlevel}
		RecordOP 99 "Runlevel is $runlevel"
        return
    fi

    RecordOP 0 ""
}

# Check sufficient space on root is available or not
check_free_space_on_root()
{
    LOG "Checking for free free space availability"

    SetOP "CheckFreeSpaceOnRoot"
    free_space=`echo ${Params[${IDX}]} | ${JQ} '.FreeSpace' | awk -F"\"" '{print $2}'`
    if [ -z "${free_space}" ]; then
        LOG "Freespace needed is not specified in the configuration JSON file"
        free_space=${MIN_ROOT_FREESPACE_KB}
    fi

    free_space_on_root=$(df -Pk / 2>> ${LOG_FILE} | sed -n '2p' | awk '{print $4}')
    LOG "Free space needed on root file-system is ${free_space}KB and actual free space available is ${free_space_on_root}KB"
    if [ ${free_space_on_root} -lt ${free_space} ]; then
        RETURN_VALUE=1
        LOG "At least ${free_space}KB of free space is required on \"/\""
        log_to_json_file ASRMobilityServiceFreeSpaceOnRootNotSufficient "Insufficient free space available on the root file-system(/). Ensure that there is at least ${free_space}KB free space available on the root file-system before you try to protect the virtual machine again." FreeSpace ${free_space}
        RecordOP 99 "Root file-system has less than ${free_space}KB free space"
        return
    fi

    RecordOP 0 ""
}

# Check if the LIS components are installed or not
check_lis_drivers_available()
{
    LOG "Checking for LIS drivers availability"

    SetOP "CheckLisDriversAvailable"
    drivers=`echo ${Params[${IDX}]} | ${JQ} '.Drivers' | awk -F"\"" '{print $2}'`
    if [ -z "${drivers}" ]; then
        drivers="hv_storvsc.ko"
    fi

    if [ ${PLATFORM} = "Azure" ]; then
        LOG "Check for LIS driver(s) ${drivers} is skipped as the platform is Azure"
        RecordOP 0 ""
        return
    fi

    LOG "Checking for LIS driver(s) ${drivers}"
    if [ "$OS" = "RHEL5-64" -o "$OS" = "RHEL6-64" ]; then
        KERNEL=`uname -r`
        for drv in `echo ${drivers} | sed 's/,/ /g'`
        do
            if [ "$OS" = "RHEL5-64" ]; then
                (zcat /boot/initrd-${KERNEL}.img | cpio -itv) 2> /dev/null | grep -q ${drv}
            else
                lsinitrd /boot/initramfs-${KERNEL}.img | grep -q ${drv}
            fi

            if [ $? -ne "0" ]; then
                RETURN_VALUE=1
                LOG "Linux Integration Services(LIS) components are not installed on this server. Please install them and retry installation. For more details please refer https://aka.ms/asr-os-support"
                log_to_json_file ASRMobilityServiceLisDriversNotAvailable "The necessary Hyper-V Linux Integration Services components are not installed on this machine. Install the Linux Integration service components on this machine and retry the operation. Read more about Linux Integration services https://blogs.technet.microsoft.com/virtualization/2016/07/12/which-linux-integration-services-should-i-use-in-my-linux-vms/"

                RecordOP 99 "LIS components are not installed"
                return
            fi
        done
    fi

    RecordOP 0 ""
}

# Check if ubuntu 16.04 kernel is upgraded to 4.15 to support RHEL8
check_mt_kernel_version()
{
    SetOP "CheckMTKernelVersion"
    CURR_KERNEL=`uname -r`

    if [ "$OS" = "UBUNTU-16.04-64" ]; then 
        _kernel=`uname -r | cut -f 1-2 -d "."`
        if [ "$_kernel" != "4.15" ]; then
            LOG "FAILED: Please retry after upgrading the kernel using following command and rebooting to new kernel"
            LOG "        apt-get install linux-image-generic-hwe-16.04"
            echo "FAILED: Please retry after upgrading the kernel using following command and rebooting to new kernel"
            echo "        apt-get install linux-image-generic-hwe-16.04"
            RecordOP 99 "Kernel not upgraded"
            RETURN_VALUE=1
            return 1
        fi
    fi
    
    RecordOP 0 ""
    return 0
}

is_supported_rhel8_kernel()
{
    CURR_KERNEL=$1
    EXTRA_VER1=$2
    EXTRA_VER2=$3

    ret=1
    extra_minor1=`echo ${CURR_KERNEL} | awk -F"." '{print $4}'`
    extra_minor2=`echo ${CURR_KERNEL} | awk -F"." '{print $5}'`
    if [ ! -z $extra_minor1 -a $extra_minor1 -eq $extra_minor1 ] 2> /dev/null; then
        if [ $extra_minor1 -gt $EXTRA_VER1 ]; then
            ret=0
        elif [ $extra_minor1 -eq $EXTRA_VER1 ]; then
            if [ ! -z $extra_minor2 -a $extra_minor2 -eq $extra_minor2 ] 2> /dev/null; then
                if [ $extra_minor2 -ge $EXTRA_VER2 ]; then
                    ret=0
                fi
            fi
        fi
    fi

    return $ret
}

# Check if the current running kernel is a supported one or not
check_current_kernel_supported()
{
    LOG "Check if the current running kernel is supported"

    SetOP "CheckKernelSupported"
    CURR_KERNEL=`uname -r`
    LOG "Current running kernel is ${CURR_KERNEL}"

    ret=1
    case ${OS} in
        "RHEL5-64")
            echo ${CURR_KERNEL} | grep -q "^2.6.18" && (echo ${CURR_KERNEL} | egrep -q '.el5$|.el5xen$')
            if [ $? -eq "0" ]; then
                minor_version=`echo ${CURR_KERNEL} | awk -F"-" '{print $2}' | awk -F"." '{print $1}'`
                if [ ${minor_version} -ge "92" -o ${minor_version} -le "398" ]; then
                    ret=0
                fi
            fi
            ;;

        "RHEL6-64")
            echo ${CURR_KERNEL} | grep -q "^2.6.32" && (echo ${CURR_KERNEL} | egrep -q '.el6.x86_64$|.el6.centos.plus.x86_64$')
            if [ $? -eq "0" ]; then
                minor_version=`echo ${CURR_KERNEL} | awk -F"-" '{print $2}' | awk -F"." '{print $1}'`
                if [ ${minor_version} -ge "71" -a ${minor_version} -le "754" ]; then
                    ret=0
                fi
            fi
            ;;

        "RHEL7-64")
            echo ${CURR_KERNEL} | grep -q ".el7.x86_64$"
            ret=$?
            ;;

        "RHEL8-64")
            echo ${CURR_KERNEL} | grep -q ".el8.*"
            if [ $? -eq "0" ]; then
               minor_version=`echo ${CURR_KERNEL} | awk -F"-" '{print $2}' | awk -F"." '{print $1}'`
               case ${minor_version} in
                   "305")
                       is_supported_rhel8_kernel ${CURR_KERNEL} 30 1
                       if [ $? -eq "0" ]; then
                           ret=0
                       fi
                       ;;

                   "348")
                       is_supported_rhel8_kernel ${CURR_KERNEL} 5 1
                       if [ $? -eq "0" ]; then
                           ret=0
                       fi
                       ;;

                   *)
                       ret=0
                       ;;
               esac
            fi
            ;;

        "OL7-64")
            echo ${CURR_KERNEL} | egrep -q ".el7.x86_64$|.el7uek.x86_64"
            ret=$?
            ;;

        "OL8-64")
            echo ${CURR_KERNEL} | grep -q ".el8uek.x86_64"
            if [ $? -eq "0" ]; then
                ret=0
            else
                echo ${CURR_KERNEL} | grep -q ".el8.*"
                if [ $? -eq "0" ]; then
                    minor_version=`echo ${CURR_KERNEL} | awk -F"-" '{print $2}' | awk -F"." '{print $1}'`
                    if [ ${minor_version} -ge "80" -a ${minor_version} -le "348" ]; then
                        case ${minor_version} in
                            "305")
                                is_supported_rhel8_kernel ${CURR_KERNEL} 30 1
                                if [ $? -eq "0" ]; then
                                    ret=0
                                fi
                                ;;

                            "348")
                                is_supported_rhel8_kernel ${CURR_KERNEL} 5 1
                                if [ $? -eq "0" ]; then
                                    ret=0
                                fi
                                ;;

                            *)
                                ret=0
                                ;;
                        esac
                    fi
                fi
            fi

            ;;

        "SLES11-SP3-64"|"SLES11-SP4-64")
            echo ${CURR_KERNEL} | grep -q "^3.0" && (echo ${CURR_KERNEL} | egrep -q '\-default$|-xen$')
            ret=$?
            ;;

        "OL6-64")
            echo ${CURR_KERNEL} | grep -q "^2.6.32" && (echo ${CURR_KERNEL} | grep -q ".el6.x86_64$")
            if [ $? -eq "0" ]; then
                minor_version=`echo ${CURR_KERNEL} | awk -F"-" '{print $2}' | awk -F"." '{print $1}'`
                if [ ${minor_version} -ge "358" -a ${minor_version} -le "754" ]; then
                    ret=0
                fi
            fi

            if [ ${ret} -eq "1" ]; then
                echo ${CURR_KERNEL} | grep -q "^3.8.13" && (echo ${CURR_KERNEL} | grep -q ".el6uek.x86_64")
                ret=$?
            fi
            
            if [ ${ret} -eq "1" ]; then
                echo ${CURR_KERNEL} | grep -q "^4.1.12" && (echo ${CURR_KERNEL} | grep -q ".el6uek.x86_64")
                ret=$?
            fi
            ;;

        UBUNTU*|DEBIAN*)
            if [ -f "${Supported_Kernels}" ]; then
                while read ker
                do
                    if [ ${CURR_KERNEL} = "${ker}" ]; then
                        ret=0
                        break
                    fi
                done < ${Supported_Kernels}
            fi
            ;;

        "SLES12-64"|"SLES15-64")
            echo ${CURR_KERNEL} | grep -q "\-default$"
            ret=$?

            if [ ${ret} -ne "0" -a -f "${Supported_Kernels}" ]; then
                grep -q "^${CURR_KERNEL}:" ${Supported_Kernels}
                ret=$?
            fi
    esac

    if [ ${ret} -ne "0" ]; then
        RETURN_VALUE=1
        LOG "This version of mobility service doesn't support the operating system kernel version (${CURR_KERNEL}) running on the source machine. Please refer the list of operating systems supported by Azure Site Recovery : https://aka.ms/asr-os-support"
        log_to_json_file ASRMobilityServiceKernelNotSupported "This version of mobility service doesn't support the operating system kernel version (${CURR_KERNEL}) running on the source machine. Please refer the list of operating systems supported by Azure Site Recovery : https://aka.ms/asr-os-support" KernelVersion ${CURR_KERNEL}
        RecordOP 99 "This version of mobility service doesn't support the operating system kernel version (${CURR_KERNEL}) running on the source machine."
        return
    fi

    RecordOP 0 ""
}

# Check if the current running kernel's directory exists under /lib/modules path
check_kernel_directory_exists()
{
    LOG "Checking if the directory for the current running kernel exists under /lib/modules"

    SetOP "CheckKernelDirectoryExists"
    DRV_PATH="/lib/modules/`uname -r`/kernel/drivers/char"
    if [ ! -d "${DRV_PATH}" ]; then
        RETURN_VALUE=1
        LOG "The directory for the current running kernel (`uname -r`) doesn't exist under /lib/modules/ path. Install the current kernel package forcefully."
        log_to_json_file ASRMobilityServiceKernelDirectoryNotExist "The directory for the current running kernel (`uname -r`) doesn't exist under /lib/modules/ path. Install the current kernel package forcefully." KernelVersion `uname -r`
        RecordOP 99 "The directory for the current running kernel (`uname -r`) doesn't exist under /lib/modules/ path."
        return
    fi

    RecordOP 0 ""
}

# Finds all the devices listed by dmsetup with type "multipath"
find_multipath_devices()
{
    LOG "Finding multipath devices"

    :> ${TEMP_FILE}
    (dmsetup table 2>> ${LOG_FILE} | grep -v 'No devices found') > ${TEMP_FILE}

    LOG "The dmsetup output:"
    LOG "`echo;cat ${TEMP_FILE}`"

    idx=0
    while read line
    do
        type=`echo ${line} | awk '{print $4}'`
        if [ ${type} != "multipath" ]; then
            continue
        fi

        dev=`echo ${line} | awk -F":" '{print $1}'`
        if [ ! -e "/dev/mapper/${dev}" ]; then
            continue
        fi

        MULIPATH_DEVICES[${idx}]="/dev/mapper/${dev}"
        MULIPATH_DEVICES_DEVT[${idx}]=`echo ${line} | awk '{print $13}'`
        idx=`expr ${idx} + 1`
    done < ${TEMP_FILE}

    if [ ${#MULIPATH_DEVICES[*]} -eq "0" ]; then
        LOG "No multipath disks found"
    else
        LOG "Multipath Disks: ${MULIPATH_DEVICES[@]}"
        LOG "Multipath Disk Devt: ${MULIPATH_DEVICES_DEVT[@]}"
    fi
}

# Finds all the linear devices except the ones are the partitions
find_linear_devices()
{
    LOG "Finding linear devices"

    :> ${TEMP_FILE}
    (dmsetup table 2>> ${LOG_FILE} | grep -v 'No devices found') > ${TEMP_FILE}

    idx=0
    while read line
    do
        type=`echo ${line} | awk '{print $4}'`
        if [ ${type} != "linear" -a ${type} != "crypt" ]; then
            continue
        fi

        dev=`echo ${line} | awk -F":" '{print $1}'`
        if [ ! -e "/dev/mapper/${dev}" ]; then
            continue
        fi


        skip=0
        midx=0
        while [ ${midx} -lt ${#MULIPATH_DEVICES[*]} ]
        do
            pnum=""
            mdev=`basename ${MULIPATH_DEVICES[${midx}]}`
            echo ${dev} | grep -q "^${mdev}_part"
            if [ $? -eq "0" ]; then
                pnum=`echo ${dev} | awk -F"_part" '{print $2}'`
            else
                echo ${dev} | grep -q "^mpath" && echo ${mdev} | grep -q "^mpath"
                if [ $? -eq "0" ]; then
                    pnum=`echo ${dev} | awk -F"${mdev}" '{print $2}'`
                else
                    echo ${dev} | grep -q "^${mdev}p"
                    if [ $? -eq "0" ]; then
                        pnum=`echo ${dev} | awk -F"p" '{print $2}'`
                    fi
                fi
            fi

            if [ -z "${pnum}" ]; then
                pnum="non-digit"
                skip=0
            fi

            if [[ ${pnum} =~ [^[:digit:]] ]]; then
                skip=0
            else
                skip=1
                break
            fi

            midx=`expr ${midx} + 1`
        done

        if [ ${skip} -eq "1" ]; then
            continue
        fi

        LINEAR_DEVICES[${idx}]="/dev/mapper/${dev}"
        LINEAR_DEVICE_TYPE[${idx}]=${type}
        idx=`expr ${idx} + 1`
    done < ${TEMP_FILE}

    if [ ${#LINEAR_DEVICES[*]} -eq "0" ]; then
        LOG "No linear devices found"
    else
        LOG "Linear Devices: ${LINEAR_DEVICES[@]}"
        LOG "Linear Device Type: ${LINEAR_DEVICE_TYPE[@]}"
    fi
}

# Finds all the full disks with its partitions and the boot disk
find_devices()
{
    LOG "Finding full disks with partitions and the boot disks"

    :> ${TEMP_FILE}
    (sfdisk -luS 2>> ${LOG_FILE} | grep -e '/dev/' | grep -v Empty) > ${TEMP_FILE}

    LOG "The output of command \"sfdisk -luS 2> /dev/null | grep -e '/dev/' | grep -v Empty\" is"
    LOG "`echo;cat ${TEMP_FILE}`"

    boot_disk=""
    partitions=""
    bootable=0
    dev_idx=-1
    skip=0
    while read line
    do
        echo ${line} | grep -q "^Disk"
        if [ $? -eq "0" ]; then
            if [ ! -z "${partitions}" ]; then
                DEVICES[${dev_idx}]=`echo "${DEVICES[${dev_idx}]}:${bootable}${partitions}"`
            fi

            disk=`echo ${line} | awk '{print $2}' | awk -F":" '{print $1}'`
            bootable=0
            partitions=""
            idx=0
            found=1
            skip=0
            while [ ${idx} -lt ${#LINEAR_DEVICES[*]} ]
            do
                if [ "${disk}" = "${LINEAR_DEVICES[${idx}]}" ]; then
                    found=0
                    skip=1
                    break
                fi

                idx=`expr ${idx} + 1`
            done

            if [ ${found} -eq "0" ]; then
                continue
            fi

            disk_base=`basename ${disk}`
            if [ -e "/sys/block/${disk_base}/dev" ]; then
                devt=`cat /sys/block/${disk_base}/dev`
            else
                devt=""
            fi

            is_multipath_disk=0
            idx=0
            while [ ${idx} -lt ${#MULIPATH_DEVICES[*]} ]
            do
                if [ "${disk}" = "${MULIPATH_DEVICES[${idx}]}" ]; then
                    is_multipath_disk=1
                    break
                fi

                if [ ! -z "${devt}" -a "${devt}" = "${MULIPATH_DEVICES_DEVT[${idx}]}" ]; then
                    found=0
                    skip=1
                    break
                fi

                idx=`expr ${idx} + 1`
            done

            if [ ${found} -eq "1" ]; then
                partitions=":"
                dev_idx=`expr ${dev_idx} + 1`

                LOG "Executing the command \"${GFDISK} ${disk} 2>> ${LOG_FILE}\""
                gpartdevs=`${GFDISK} ${disk} 2>> ${LOG_FILE}`
                if [ $? -eq "0" ]; then
                    gpartdevs=`echo ${gpartdevs} | sed 's/\n/ /g'`
                    LOG "The disk ${disk} has GPT partitions ${gpartdevs}"
                    skip=1
                    for gpdev in ${gpartdevs}
                    do
                        if [ "${partitions}" = ":" ]; then
                            partitions=`echo "${partitions}${gpdev}"`
                        else
                            partitions=`echo "${partitions},${gpdev}"`
                        fi
                    done
                fi

                DEVICES[${dev_idx}]="${disk}"
            fi

            continue
        fi

        if [ ${skip} -eq "1" ]; then
            continue
        fi

        dev=`echo ${line} | awk '{print $1}'`
        if [ ! -e "${dev}" ]; then
            LOG "The partition ${dev} of ${disk} doesn't exist. Checking for _part device."

            if [ ${is_multipath_disk} -eq "1" ]; then
                basename ${dev} | grep -q "^mpath" && basename ${dev} | grep -q "^${disk_base}"
                if [ $? -eq "0" ]; then
                    basename ${dev} | grep -q "^${disk_base}"
                    pnum=`basename ${dev} | awk -F"${disk_base}" '{print $2}'`
                    base_dev="${disk_base}"
                else
                    basename ${dev} | grep -q "^${disk_base}p"
                    if [ $? -eq "0" ]; then
                        pnum=`basename ${dev} | awk -F"p" '{print $2}'`
                        base_dev=`basename ${dev} | awk -F"p" '{print $1}'`
                    else
                        continue
                    fi
                fi

                temp_dev=`dirname ${dev}`
                dev="${temp_dev}/${base_dev}_part${pnum}"
                if [ ! -e "${dev}" ]; then
                    LOG "The _part device partition ${dev} of ${disk} doesn't exist."
                    continue
                fi

                LOG "The _part device ${dev} for ${disk} exists."
            else
                continue
            fi
        fi

        if [ "${partitions}" = ":" ]; then
            partitions=`echo "${partitions}${dev}"`
        else
            partitions=`echo "${partitions},${dev}"`
        fi
    done < ${TEMP_FILE}

    if [ ! -z "${partitions}" ]; then
        DEVICES[${dev_idx}]=`echo "${DEVICES[${dev_idx}]}:${bootable}${partitions}"`
    fi

    if [ ${#DEVICES[*]} -eq "0" ]; then
        LOG "No Full disks found"
    else
        LOG "Full disks: ${DEVICES[@]}"
    fi
}

# Checks if the command exists
is_command_exist()
{
    command=$1
    path=$(which ${command} 2>> ${LOG_FILE})
    if [ -e "${path}" ]; then
        return 1
    else
        return 0
    fi
}

# Provided an LV device, finds the associated full disks
find_full_disks_of_lv()
{
    lv_dev=$1
    LOG "Finding full disk for the LV device ${lv_dev}"

    lv_disk=""
    SAME_PV=1
    NR_PVS=0
    PVS=""

    vg_name=`lvdisplay ${lv_dev} 2>> ${LOG_FILE} | grep "VG Name" | awk '{print $3}'`
    LOG "VG name for LV ${lv_dev} is ${vg_name}"
    LOG "The output of command \'vgdisplay -v ${vg_name}\' is `echo; vgdisplay -v ${vg_name} 2>> ${LOG_FILE}`"
    for pv in `vgdisplay -v ${vg_name} 2>> ${LOG_FILE} | grep "PV Name" | awk '{print $3}'`
    do
        echo ${pv} | grep -q "^/dev/"
        if [ $? -ne "0" ]; then
            continue
        fi

        idx=0
        while [ ${idx} -lt ${#DEVICES[*]} ]
        do
            disk=`echo ${DEVICES[${idx}]} | awk -F":" '{print $1}'`
            parts=`echo ${DEVICES[${idx}]} | awk -F":" '{print $3}'`
            parts=`echo ${parts} | sed 's/,/ /g'`

            devices=`echo "${disk} ${parts}"`

            for dev in `echo ${devices}`
            do
                if [ "${pv}" = "${dev}" ]; then
                    if [ -z "${PVS}" ]; then
                        PVS="${disk}"
                        lv_disk="${disk}"
                        NR_PVS=1
                    else
                        if [ "${lv_disk}" != "${disk}" ]; then
                            SAME_PV=0
                            found=0
                            for root_dev in `echo ${PVS}`
                            do
                                if [ ${root_dev} = "${disk}" ]; then
                                    found=1
                                    break
                                fi
                            done

                            if [ ${found} -eq "0" ]; then
                                NR_PVS=`expr ${NR_PVS} + 1`
                                PVS=`echo ${PVS},${disk}`
                            fi
                        fi
                    fi

                    break
                fi
            done

            idx=`expr ${idx} + 1`
        done
    done
}

# Finds the full disk that hosts the boot-filesystem partition or volume 
find_boot_disk()
{
    LOG "Finding the boot disk"

    mount | awk '{print $1,$3,$5}' | grep -v "/boot/" | grep -qw "/boot"
    if [ $? -eq "1" ]; then
        if [ -d "/boot" ]; then
            SAME_BOOT_DISK=1
            BOOT_DISKS=${ROOT_DEV}
            BOOT_DEV=${ROOT_DEV}
            NR_BOOTABLE_DISKS=${NR_ROOT_DISKS}
            BOOT_FULL_DISKS=${ROOT_DISKS}
            IS_BOOT_LVM=${IS_ROOT_LVM}
            LOG "/boot is part of /, NR_BOOTABLE_DISKS=${NR_BOOTABLE_DISKS}, BOOT_FULL_DISKS=${BOOT_FULL_DISKS}, IS_BOOT_LVM=${IS_BOOT_LVM}"
        else
            LOG "/boot is not a directory"
        fi

        return
    fi

    # For btrfs, same volume can be mounted at different mount points.
    BOOT_DEV=`mnt_to_dev /boot`
    boot_link_dev=`readlink -f $BOOT_DEV`
    root_link_dev=`readlink -f $ROOT_DEV`
    if [ "$boot_link_dev" = "$root_link_dev" ]; then
            SAME_BOOT_DISK=1
            BOOT_DISKS=${ROOT_DEV}
            BOOT_DEV=${ROOT_DEV}
            NR_BOOTABLE_DISKS=${NR_ROOT_DISKS}
            BOOT_FULL_DISKS=${ROOT_DISKS}
            IS_BOOT_LVM=${IS_ROOT_LVM}
            LOG "/boot is part of /, NR_BOOTABLE_DISKS=${NR_BOOTABLE_DISKS}, BOOT_FULL_DISKS=${BOOT_FULL_DISKS}, IS_BOOT_LVM=${IS_BOOT_LVM}"
            return
    fi

    LOG "/boot is a mount point not part of /"
    for dev in `mount | awk '{print $1,$3}' | grep -w "/boot" | grep -v "/boot/" | awk '{print $1}'`
    do
        if [ -b ${dev} ]; then
            BOOT_DISKS=${dev}
            break
        fi
    done
    LOG "The device mounted on /boot is ${BOOT_DISKS}"
    boot_link_dev=`readlink -f ${BOOT_DISKS}`

    idx=0
    while [ ${idx} -lt ${#DEVICES[*]} ]
    do
        parts=`echo ${DEVICES[${idx}]} | awk -F":" '{print $3}'`
        if [ -z "${parts}" ]; then
            idx=`expr ${idx} + 1`
            continue
        fi

        for dev in `echo ${parts} | sed 's/,/ /g'`
        do
            link_dev=`readlink -f ${dev}`
            if [ "${boot_link_dev}" = "${link_dev}" ]; then
                BOOT_FULL_DISKS=`echo ${DEVICES[${idx}]} | awk -F":" '{print $1}'`
                BOOT_DEV=${boot_link_dev}
                NR_BOOTABLE_DISKS=1
                LOG "Boot is on partition, SAME_BOOT_DISK=${SAME_BOOT_DISK}, BOOT_DISKS=${BOOT_DISKS}, BOOT_DEV=${BOOT_DEV}, BOOT_FULL_DISKS=${BOOT_FULL_DISKS}, NR_BOOT_DISKS=${NR_BOOTABLE_DISKS}"
                return
            fi
        done

        idx=`expr ${idx} + 1`
    done

    idx=0
    while [ ${idx} -lt ${#LINEAR_DEVICES[*]} ]
    do
        lvm_dev="${LINEAR_DEVICES[${idx}]}"
        linear_link_dev=`readlink -f ${lvm_dev}`
        if [ "${boot_link_dev}" = "${linear_link_dev}" ]; then
            IS_BOOT_ON_LV=1
            is_command_exist vgdisplay
            if [ $? -eq "0" ]; then
                IS_LVM_PACKAGE_AVAILABLE=0
                return
            fi

            LOG "The output of \'lvdisplay ${lvm_dev}\'"
            lvdisplay ${LINEAR_DEVICES[${idx}]} >> ${LOG_FILE} 2>&1
            if [ $? -ne "0" ]; then
                LOG "find_boot_disk: lvdisplay failed"
                return
            fi

            find_full_disks_of_lv ${lvm_dev}
            SAME_BOOT_DISK=${SAME_PV}
            BOOT_FULL_DISKS=${PVS}
            NR_BOOTABLE_DISKS=${NR_PVS}
            BOOT_DEV=${lvm_dev}
            IS_BOOT_LVM=1
            LOG "Boot is on LV, SAME_BOOT_DISK=${SAME_BOOT_DISK}, BOOT_FULL_DISKS=${BOOT_FULL_DISKS}, BOOT_DEV=${BOOT_DEV}, NR_BOOT_DISKS=${NR_BOOTABLE_DISKS}"
            return
        fi

        idx=`expr $idx + 1`
    done
}

# Finds the full disk that hosts the root-filesystem partition or volume
find_system_disk()
{
    LOG "Finding the system disk"

    dev=$(mnt_to_dev /)
    if [ -b ${dev} ]; then
        ROOT_DEV=${dev}
        ROOT_FS=`dev_to_fs $dev`
        LOG "Root filesystem is ${ROOT_FS}"
    fi
    
    LOG "The device mounted on / is ${ROOT_DEV}"
    root_link_dev=`readlink -f ${ROOT_DEV}`
    root_link_dev_uuid=`blkid -s UUID -o value $root_link_dev`

    idx=0
    while [ ${idx} -lt ${#DEVICES[*]} ]
    do
        parts=`echo ${DEVICES[${idx}]} | awk -F":" '{print $3}'`
        if [ -z "${parts}" ]; then
            idx=`expr ${idx} + 1`
            continue
        fi

        for dev in `echo ${parts} | sed 's/,/ /g'`
        do
            link_dev=`readlink -f ${dev}`
            link_dev_uuid=`blkid -s UUID -o value $link_dev`
            if [ "${root_link_dev}" = "${link_dev}" -o "${root_link_dev_uuid}" = "${link_dev_uuid}" ]; then
                ROOT_DISKS=`echo ${DEVICES[${idx}]} | awk -F":" '{print $1}'`
                NR_ROOT_DISKS=1
                LOG "Root is on partition, SAME_ROOT_DISK=${SAME_ROOT_DISK}, ROOT_DISKS=${ROOT_DISKS}, ROOT_DEV=${root_link_dev}, NR_ROOT_DISKS=${NR_ROOT_DISKS}"
                return
            fi
        done

        idx=`expr ${idx} + 1`
    done

    idx=0
    while [ ${idx} -lt ${#LINEAR_DEVICES[*]} ]
    do
        lvm_dev="${LINEAR_DEVICES[${idx}]}"
        linear_link_dev=`readlink -f ${lvm_dev}`
        linear_link_dev_uuid=`blkid -s UUID -o value $linear_link_dev`
        if [ "${root_link_dev}" = "${linear_link_dev}" -o "${root_link_dev_uuid}" = "${linear_link_dev_uuid}" ]; then
            IS_ROOT_ON_LV=1
            is_command_exist vgdisplay
            if [ $? -eq "0" ]; then
                IS_LVM_PACKAGE_AVAILABLE=0
                return
            fi

            LOG "The output of \'lvdisplay ${lvm_dev}\'"
            lvdisplay ${LINEAR_DEVICES[${idx}]} >> ${LOG_FILE} 2>&1
            if [ $? -ne "0" ]; then
                LOG "find_system_disk: lvdisplay failed"
                return
            fi

            find_full_disks_of_lv ${lvm_dev}
            SAME_ROOT_DISK=${SAME_PV}
            ROOT_DISKS=${PVS}
            NR_ROOT_DISKS=${NR_PVS}
            IS_ROOT_LVM=1
            LOG "Root is on LV, SAME_ROOT_DISK=${SAME_ROOT_DISK}, ROOT_DISKS=${ROOT_DISKS}, ROOT_DEV=${lvm_dev}, NR_ROOT_DISKS=${NR_ROOT_DISKS}"
            return
        fi

        idx=`expr $idx + 1`
    done

    LOG "System disk is not found"
}

# Collects the boot and system disks information
collect_volume_info()
{
    # DON"T CHNAGE THE ORDER
    find_multipath_devices
    find_linear_devices
    find_devices
    find_system_disk
    find_boot_disk
}


# Check if the system disk exists
check_system_disk_available()
{
    LOG "Check if the system disk is available"

    SetOP "CheckSystemDiskAvailable"
    if [ ${NR_ROOT_DISKS} -eq "0" ]; then
        RETURN_VALUE=1
        LOG "The system/root disk is not available. Check if any device is mounted on /."
        log_to_json_file ASRMobilityServiceSystemDiskNotFound "The system/root disk is not available. Check if any device is mounted on /."
        RecordOP 99 "The system/root disk is not available"
        return
    fi

    RecordOP 0 ""
}

# Check if the boot disk exists
check_boot_disk_available()
{
    LOG "Check if the boot disk is available"

    SetOP "CheckBootDiskAvailable"
    if [ ${NR_BOOTABLE_DISKS} -eq "0" ]; then
        RETURN_VALUE=1
        LOG "The boot disk is not available. Create a bootable partition."
        log_to_json_file ASRMobilityServiceBootDiskNotFound "The boot disk is not available. Create a bootable partition."
        RecordOP 99 "The boot disk is not available"
        return
    fi

    RecordOP 0 ""
}

# Check if the LVM related commands exist
check_lvm_package_available()
{
    LOG "Check if the LVM package installed"

    SetOP "CheckLVMPackageInstalled"
    if [ ${IS_ROOT_ON_LV} -eq "0" ]; then
        LOG "Skipping the check LVMPackageInstalled as root file-system is not laid on an LV"
        RecordOP 0 ""
        return
    fi

    is_command_exist vgdisplay
    if [ $? -eq "0" ]; then
        RETURN_VALUE=1
        LOG "LVM package is missing. Install the lvm2 package and reboot the VM before trying to protect again."
        log_to_json_file ASRMobilityServiceLvmPackageMissing "LVM package is missing. Install the lvm2 package and reboot the VM before trying to protect again."
        RecordOP 99 "LVM package is missing"
        return
    fi

    RecordOP 0 ""
}

check_boot_on_lvm_from_multiple_disks()
{
    LOG "Check if the boot is configured on LVM device which is coming from multiple full disks"

    SetOP "CheckBootOnLVMFromMultipleDisks"
    if [ ${IS_BOOT_LVM} -eq "1" -a ${NR_BOOTABLE_DISKS} -gt "1" ]; then
        RETURN_VALUE=1
        LOG "The /boot is configured on an LVM device (${BOOT_DEV}) and this LVM device is created using multiple disks (${BOOT_FULL_DISKS})"
        log_to_json_file ASRMobilityServiceBootOnLVMFromMultipleDisks "The /boot is configured on an LVM device (${BOOT_DEV}) and this LVM device is created using multiple disks (${BOOT_FULL_DISKS})" LVMDevice ${BOOT_DEV} BootDisks ${BOOT_FULL_DISKS}
        RecordOP 99 "The boot is configured on LVM device which is created using multiple full disks"
        return
    fi

    RecordOP 0 ""
}

# Check if the boot partition is mounted
check_boot_partition_mounted()
{
    LOG "Check if the boot partition is mounted"

    SetOP "CheckBootPartitionMounted"
    if [ ${NR_BOOTABLE_DISKS} -eq "1" ]; then
        boot_link_dev=`readlink -f ${BOOT_DISKS}`
        dev=`mnt_to_dev /boot`
        final_dev=`readlink -f ${dev}`
        if [ "${boot_link_dev}" = "${final_dev}" ]; then
            BOOT_FS=`dev_to_fs $dev`
            LOG "Boot filesystem is ${BOOT_FS}"
            LOG "Boot disk (${BOOT_DISKS}) is mounted as ${dev}"
            RecordOP 0 ""
            return
        fi

        RETURN_VALUE=1
        LOG "The boot partition (${BOOT_DISKS}) is not mounted. Mount the boot partition."
        log_to_json_file ASRMobilityServiceBootPartitionNotMounted "The boot partition (${BOOT_DISKS}) is not mounted. Mount the boot partition." BootPartition ${BOOT_DISKS}
        RecordOP 99 "The boot partition is not mounted"
        return
    fi

    RecordOP 0 ""
}

check_rootfs_is_supported()
{
    LOG "Check if the root filesystem is supported"

    SetOP "CheckIsRootFsSupported"
    filesystems=`echo ${Params[${IDX}]} | ${JQ} '.SupportedFileSystems' | awk -F"\"" '{print $2}'`
    for fs in `echo ${filesystems} | sed 's/,/ /g'`
    do
        if [ "${ROOT_FS}" = "${fs}" ]; then
            RecordOP 0 ""
            return
        fi
    done

    RETURN_VALUE=1
    LOG "The supported file-systems for root file-system are ${filesystems} and the current root file-system is ${ROOT_FS}"
    log_to_json_file ASRMobilityServiceUnsupportedRootFileSystem "The supported file-systems for root file-system are ${filesystems} and the current root file-system is ${ROOT_FS}" SupportedRootFileSystems ${filesystems} RootFileSystem ${ROOT_FS}
    RecordOP 99 "Unsupported root file-system"
}

check_dirs_writable()
{
    local _dirlist=""
    local _dir=""
    local _nwlist=""
    
    LOG "Check if directories is are writeable"
    SetOP "DirsWritable"

    _dirlist=`echo ${Params[${IDX}]} | ${JQ} '.Dirs' | awk -F"\"" '{print $2}'`
    for _dir in `echo ${_dirlist} | sed 's/,/ /g'`
    do
        if [ ! -w $_dir ]; then 
            _nwlist="$_dir,$_nwlist"
        fi
    done

    if [ "$_nwlist" = "" ]; then
        RecordOP 0 ""
    else
        RETURN_VALUE=1
        _nwlist=`echo $_nwlist | sed "s/,$//"`
        LOG "FAIL: $_nwlist is not writable"
        log_to_json_file ASRMobilityServiceDirsNotWritable           \
            "The following directories do not have write permission: $_nwlist. Without write access, Site Recovery agent cannot be copied and installed." DirList "$_nwlist" 
        RecordOP 99 "FAIL: Dir is not writable: $_nwlist"
    fi
}


# Check if the boot filesystem is supported
check_bootfs_is_supported()
{
    LOG "Check if the boot filesystem is supported"

    SetOP "CheckIsBootFsSupported"
    if [ ${NR_BOOTABLE_DISKS} -ne "1" ]; then
        RecordOP 0 ""
        return
    fi

    filesystems=`echo ${Params[${IDX}]} | ${JQ} '.SupportedFileSystems' | awk -F"\"" '{print $2}'`
    for fs in `echo ${filesystems} | sed 's/,/ /g'`
    do
        if [ "${BOOT_FS}" = "${fs}" ]; then
            RecordOP 0 ""
            return
        fi
    done

    RETURN_VALUE=1
    LOG "The supported file-systems for boot file-system are ${filesystems} and the current boot file-system is ${BOOT_FS}"
    log_to_json_file ASRMobilityServiceUnsupportedBootFileSystem "The supported file-systems for boot file-system are ${filesystems} and the current boot file-system is ${BOOT_FS}" SupportedBootFileSystems ${filesystems} BootFileSystem ${BOOT_FS}
    RecordOP 99 "Unsupported boot file-system"
}

# Points to the Grub configuration file
find_grub_config_file()
{
    LOG "Finding the Grub file"

    GRUB_FILE="/boot/grub/grub.cfg"
    if [ ! -f "${GRUB_FILE}" ]; then
        GRUB_FILE="/boot/grub2/grub.cfg"
        if [ ! -f "${GRUB_FILE}" ]; then
            GRUB_FILE="/boot/grub/menu.lst"
            GRUB_FILE=`readlink -f ${GRUB_FILE}`
            if [ ! -f "${GRUB_FILE}" ]; then
                LOG "The Grub file doesn't exist"
                GRUB_FILE=""
                return
            fi
        fi
    fi

    LOG "The Grub file: ${GRUB_FILE}"
}

# Check if the Grub command line contains "acpi=0"
check_grub_commandline_for_acpi_set_to_zero()
{
    LOG "Check if the commandline parameter acpi=0 exists"

    SetOP "CheckAcpiEqualsToZeroSet"
    if [ ${BOOT_CONFIGURED_ON_EFI_PARTITION} -eq "1" ]; then
        LOG "Skipping the check AcpiEqualsToZeroSet as boot is configured on an EFI partition"
        RecordOP 0 ""
        return
    fi

    if [ -z ${GRUB_FILE} ]; then
        LOG "Skipping the check AcpiEqualsToZeroSet as Grub configrauion file is missing"
        RecordOP 0 ""
        return
    fi

    cat /proc/cmdline | grep -wqi "acpi=0"
    if [ $? -eq "0" ]; then
        RETURN_VALUE=1
        LOG "The Grub command line contains 'acpi=0'. Remove the entry and then try to protect the VM."
        log_to_json_file ASRMobilityServiceAcpiEqualsToZeroSet "The Grub command line contains 'acpi=0'. Remove the entry and then try to protect the VM."
        RecordOP 99 "The Grub command line contains 'acpi=0'"
        return
    fi

    RecordOP 0 ""
}

# Check if the LV devices mentioned in the Grub exist
check_device_in_grub_file_exist()
{
    LOG "Check if the devices mentioned in the Grub exist"

    SetOP "CheckDeviceExistInGrubFiles"
    if [ ${BOOT_CONFIGURED_ON_EFI_PARTITION} -eq "1" ]; then
        LOG "Skipping the check DeviceExistInGrubFiles as boot is configured on an EFI partition"
        RecordOP 0 ""
        return
    fi

    if [ -z "${GRUB_FILE}" ]; then
        LOG "Skipping the check DeviceExistInGrubFiles as Grub configrauion file is missing"
        RecordOP 0 ""
        return
    fi

    cmdline=`egrep "rd_LVM_LV|rd.lvm.lv" ${GRUB_FILE}`

    if [ -f "/etc/default/grub" ]; then
        line=`egrep "rd_LVM_LV|rd.lvm.lv" /etc/default/grub`
        for opt in `echo ${line}`
        do
            echo ${opt} | grep -q "rd_LVM_LV"
            if [ $? -eq "0" ]; then
                sep="rd_LVM_LV"
            else
                echo ${opt} | grep -q "rd.lvm.lv"
                if [ $? -eq "0" ]; then
                    sep="rd.lvm.lv"
                else
                    continue
                fi
            fi

            opt="${sep}`echo ${opt} | awk -F"${sep}" '{print $2}'`"
            cmdline=`echo "${cmdline} ${opt}"`
        done
    fi

    devices=
    for opt in `echo ${cmdline}`
    do
        echo ${opt} | egrep -q "rd_LVM_LV|rd.lvm.lv"
        if [ $? -ne "0" ]; then
            continue
        fi

        opt=`echo ${opt} | tr -d \'\"`
        vg_name=`echo ${opt} | awk -F"=" '{print $2}' | awk -F"/" '{print $1}'`
        lv_name=`echo ${opt} | awk -F"=" '{print $2}' | awk -F"/" '{print $2}'`
        if [ ! -e "/dev/${vg_name}/${lv_name}" ]; then
            if [ -z "${devices}" ]; then
                devices="/dev/${vg_name}/${lv_name}"
            else
                for dev in `echo ${devices} | sed 's/,/ /g'`
                do
                    found=0
                    if [ ${dev} = "/dev/${vg_name}/${lv_name}" ]; then
                        found=1
                        break
                    fi
                done

                if [ ${found} -eq "0" ]; then
                    devices=`echo "${devices},/dev/${vg_name}/${lv_name}"`
                fi
            fi
        fi
    done

    if [ ! -z "${devices}" ]; then
        RETURN_VALUE=1
        LOG "The Grub configuration has the entry for the device (${devices}) which doesn't exist. Fix the Grub."
        log_to_json_file ASRMobilityServiceDeviceNotExist "The Grub configuration has the entry for the device (${devices}) which doesn't exist. Fix the Grub." Device ${devices}
        RecordOP 99 "The Grub configuration has the entry for the device which doesn't exist."
        return
    fi

    RecordOP 0 ""
}

# Reports if the device needs to be replaced by UUID or not.
report_device()
{
    dev=$1
    if [ ! -z "${dev}" ]; then
        echo $dev | egrep "^UUID|^LABEL|^/dev/disk/by-uuid/|^/dev/disk/by-label/" > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            return 0
        fi

        final_dev=`readlink -f ${dev}`
        idx=0
        while [ ${idx} -lt ${#LINEAR_DEVICES[*]} ]
        do
            linear_final_dev=`readlink -f ${LINEAR_DEVICES[${idx}]}`
            if [ "${final_dev}" = "${linear_final_dev}" ]; then
                return 0
            fi

            idx=`expr ${idx} + 1`
        done

        return 1
    fi

    return 0
}

# Check if the device needs to be replaced by UUID or not.
check_device_name_change_in_grub_file()
{
    LOG "Check if the device names mentioned in the Grub instead of UUID"

    SetOP "CheckGrubDeviceNameChange"
    if [ ${BOOT_CONFIGURED_ON_EFI_PARTITION} -eq "1" ]; then
        LOG "Skipping the check GrubDeviceNameChange as boot is configured on an EFI partition"
        RecordOP 0 ""
        return
    fi

    if [ -z "${GRUB_FILE}" ]; then
        LOG "Skipping the check GrubDeviceNameChange as Grub configrauion file is missing"
        RecordOP 0 ""
        return
    fi

    devices=
    exec 4<"${GRUB_FILE}"
    while read -u 4 -r _line_
    do
        _ret_val_=`echo $_line_ | grep "^#"`
        if [ "${_ret_val_}" ]; then
            continue
        fi

        _root_dev_name_=`echo $_line_ |  egrep 'kernel.*root[ ]*=[ ]*\/dev\/|linux.*root[ ]*=[ ]*\/dev\/' | awk -F"root[ ]*=[ ]*" '{print $2}' | awk '{print $1}'`
        report_device ${_root_dev_name_}
        if [ $? -eq 1 ]; then
            devices=`echo "${devices} ${_root_dev_name_}"`
        fi

        _resume_dev_name_=`echo $_line_ |  egrep 'kernel.*resume[ ]*=[ ]*\/dev\/|linux.*resume[ ]*=[ ]*\/dev\/' | awk -F"resume[ ]*=[ ]*" '{print $2}' | awk '{print $1}'`
        report_device ${_resume_dev_name_}
        if [ $? -eq 1 ]; then
            devices=`echo "${devices} ${_resume_dev_name_}"`
        fi
    done
    4<&-

    if [ ! -z "${devices}" ]; then
        devices=`echo ${devices} | sed 's/ /,/g'`
        RETURN_VALUE=1
        LOG "The Grub has the entry for the device ${devices}. Use the UUID instead of the device name."
        log_to_json_file ASRMobilityServiceGrubDeviceNameChange "The Grub has the entry for the device ${devices}. Use the UUID instead of the device name." Device ${devices}
        RecordOP 99 "The Grub has the entry for the device with name instead UUID"
        return
    fi

    RecordOP 0 ""
}

# Check if the DHCP client is instaaled or not
check_dhcp_client_available()
{
    LOG "Check if the DHCP client exists"

    SetOP "CheckDhcpClientAvailable"
    if $(echo "$OS" | egrep -q 'RHEL|OL'); then
        binary="dhclient"
        pkg_name="dhclient"
    elif $(echo "$OS" | egrep -q "SLES11"); then
        binary="dhcpcd"
        pkg_name="dhcpcd"
    elif $(echo "$OS" | egrep -q "SLES12"); then
        binary="dhclient"
        pkg_name="dhcp-client"
    elif $(echo "$OS" | egrep -q "SLES15"); then
        binary="/usr/lib/wicked/bin/wickedd-dhcp4"
        pkg_name="wicked"
    elif $(echo "$OS" | egrep -q 'UBUNTU|DEBIAN'); then
        binary="dhclient"
        pkg_name="isc-dhcp-client"
    fi

    command=$(which ${binary} 2>> ${LOG_FILE})
    if [ -z "${command}" ] ; then
        RETURN_VALUE=1
        LOG "The DHCP client is not available on the system. Install the package ${pkg_name}."
        log_to_json_file ASRMobilityServiceDhcpClientNotAvailable "The DHCP client is not available on the system. Install the package ${pkg_name}." Package ${pkg_name}
        RecordOP 99 "The DHCP client is not available on the system"
        return
    fi

    RecordOP 0 ""
}

# Check if the VM is encrypted using old way of one pass
check_old_one_pass_encryption_enabled()
{
    LOG "Check if the VM is encrypted using old way of one pass"

    SetOP "OldOnePassEncryption"
    if [ ${PLATFORM} != "Azure" ]; then
        LOG "Skipping the check if the VM is encrypted using old way of one pass as the platform is not Azure"
        RecordOP 0 ""
        return
    fi

    if [ ! -e "/var/lib/azure_disk_encryption_config/azure_crypt_mount" ]; then
        LOG "The file /var/lib/azure_disk_encryption_config/azure_crypt_mount doesn't exist"
        RecordOP 0 ""
        return
    fi

    idx=0
    while [ ${idx} -lt ${#LINEAR_DEVICES[*]} ]
    do
        if [ ${LINEAR_DEVICE_TYPE[${idx}]} != "crypt" ]; then
            idx=`expr ${idx} + 1`
            continue
        fi

        linear_device_basename=`basename ${LINEAR_DEVICES[${idx}]}`
        grep -qw "^${linear_device_basename}" /var/lib/azure_disk_encryption_config/azure_crypt_mount
        if [ $? -eq "0" ]; then
            RETURN_VALUE=1
            LOG "The VM is encrypted using old way of one pass encryption. Disable and enable the one pass encryption again."
            log_to_json_file ASRMobilityServiceInstallerBlockOldOnePassEncryption "The VM is encrypted using old way of one pass encryption. Disable and enable the one pass encryption again."
            RecordOP 99 "The VM is encrypted using old way of one pass encryption."
            return
        fi

        idx=`expr ${idx} + 1`
    done

    RecordOP 0 ""
}

# This functions checks for
# a) The Check is "null", go for next check
# b) If agent role is Agent, avoid MasterTarget checks and vice-versa
# c) Execute the corresponding check
execute_prereqs()
{
    IDX=0

    while [ ${IDX} -lt "${NR_CHECKS}" ]
    do
        if [ "${CheckName[${IDX}]}" = "null" ]; then
            IDX=`expr ${IDX} + 1`
            continue
        fi

        if [ "${AGENT_ROLE}" = "agent" -a ${CheckType[${IDX}]} = "MasterTarget" ]; then
            IDX=`expr ${IDX} + 1`
            continue
        fi

        if [ "${AGENT_ROLE}" = "mastertarget" -a ${CheckType[${IDX}]} = "MobilityService" ]; then
            IDX=`expr ${IDX} + 1`
            continue
        fi

        if [ "${CheckName[${IDX}]}" = "CLocaleAvailable" ]; then
            check_c_locale_available
        elif [ "${CheckName[${IDX}]}" = "HostNameSet" ]; then
            check_hostname_for_null
        elif [ "${CheckName[${IDX}]}" = "SecureBoot" ]; then
            check_secure_boot
        elif [ "${CheckName[${IDX}]}" = "FreeSpaceOnRoot" ]; then
            check_free_space_on_root
        elif [ "${CheckName[${IDX}]}" = "LisDriversAvailable" ]; then
            check_lis_drivers_available
        elif [ "${CheckName[${IDX}]}" = "KernelSupported" ]; then
            check_current_kernel_supported
        elif [ "${CheckName[${IDX}]}" = "KernelDirectoryExists" ]; then
            check_kernel_directory_exists
        elif [ "${CheckName[${IDX}]}" = "SystemDiskAvailable" ]; then
            check_system_disk_available
        elif [ "${CheckName[${IDX}]}" = "BootDiskAvailable" ]; then
            check_boot_disk_available
        elif [ "${CheckName[${IDX}]}" = "LVMPackageInstalled" ]; then
            check_lvm_package_available
        elif [ "${CheckName[${IDX}]}" = "BootOnLVMFromMultipleDisks" ]; then
            check_boot_on_lvm_from_multiple_disks
        elif [ "${CheckName[${IDX}]}" = "BootPartitionMounted" ]; then
            check_boot_partition_mounted
        elif [ "${CheckName[${IDX}]}" = "IsRootFsSupported" ]; then
            test "$WORKFLOW" = "AZURE" || check_rootfs_is_supported
        elif [ "${CheckName[${IDX}]}" = "IsBootFsSupported" ]; then
            test "$WORKFLOW" = "AZURE" || check_bootfs_is_supported
        elif [ "${CheckName[${IDX}]}" = "IsRootFsSupportedAzure" ]; then
            test "$WORKFLOW" = "AZURE" && check_rootfs_is_supported
        elif [ "${CheckName[${IDX}]}" = "IsBootFsSupportedAzure" ]; then
            test "$WORKFLOW" = "AZURE" && check_bootfs_is_supported
        elif [ "${CheckName[${IDX}]}" = "DirsWritable" ]; then
            check_dirs_writable
        elif [ "${CheckName[${IDX}]}" = "AcpiEqualsToZeroSet" ]; then
            check_grub_commandline_for_acpi_set_to_zero
        elif [ "${CheckName[${IDX}]}" = "DeviceExistInGrubFiles" ]; then
            check_device_in_grub_file_exist
        elif [ "${CheckName[${IDX}]}" = "GrubDeviceNameChange" ]; then
            check_device_name_change_in_grub_file
        elif [ "${CheckName[${IDX}]}" = "DhcpClientAvailable" ]; then
            check_dhcp_client_available
        elif [ "${CheckName[${IDX}]}" = "CheckMTKernelVersion" ]; then
            check_mt_kernel_version
        elif [ "${CheckName[${IDX}]}" = "OldOnePassEncryption" ]; then
            check_old_one_pass_encryption_enabled
		elif [ "${CheckName[${IDX}]}" = "RunlevelCommandCheck" ]; then
			check_runlevel
        fi

        IDX=`expr ${IDX} + 1`
    done
}

# This function validates whether the check is a known one or not and type of
# check has to be Common, MobilityService or MasterTarget
validate_checks()
{
    cidx=0

    while [ ${cidx} -lt "${NR_CHECKS}" ]
    do
        if [ ${CheckName[${cidx}]} = "null" ]; then
            cidx=`expr $cidx + 1`
            continue
        fi

        found=0
        for check in `echo ${Existing_Check[@]}`
        do
            if [ "${check}" = "${CheckName[${cidx}]}" ]; then
                found=1
                break
            fi
        done

        if [ $found -eq "0" ]; then
            LOG "The pre-requirement json file ${PREREQ_CONF_FILE} contains an invalid check ${CheckName[${cidx}]}"
            log_to_json_file ASRMobilityServiceInternalError "The pre-requirement json file ${PREREQ_CONF_FILE} contains an invalid check ${CheckName[${cidx}]}"
            exit 1
        fi

        echo ${CheckType[${cidx}]} | egrep -q '^Common$|^MobilityService$|^MasterTarget$'
        if [ $? -ne "0" ]; then
            LOG "CheckType ${CheckType[${cidx}]} for ${CheckName[${cidx}]} is wrong. It has to be either Common, MobilityService or MasterTarget"
            log_to_json_file ASRMobilityServiceInternalError "CheckType ${CheckType[${cidx}]} for ${CheckName[${cidx}]} is wrong. It has to be either Common, MobilityService or MasterTarget"
            exit 1
        fi

        cidx=`expr $cidx + 1`
    done
}

# This function parses the json configuration file and places the
# name & type of check and the parameters for the check in the
# seperate arrays.
parse_json()
{
    cidx=-1
    pidx=0

    CheckName=($(${JQ} -r '.[] | "\(.CheckName)"' ${PREREQ_CONF_FILE}))
    CheckType=($(${JQ} -r '.[] | "\(.CheckType)"' ${PREREQ_CONF_FILE}))
    Params=($(${JQ} -r '.[] | "\(.Params)"' ${PREREQ_CONF_FILE}))
    NR_CHECKS=${#CheckName[*]}
}

##Main##
wrong_usage=0
while getopts :r:j:v:l:h opt
do
    case ${opt} in
        r) AGENT_ROLE="${OPTARG}" ;;
        j) json_errors_file="${OPTARG}" ;;
        v) WORKFLOW="${OPTARG}" ;;
        l) LOG_FILE="${OPTARG}" ;;
        *) wrong_usage=1
           wrong_arg=${opt}
    esac
done

LOG "*******************************************************************************"
LOG "Prereq check installer is invoked with $@"

# Check if the output JSON file passed or not
if [ -z "${json_errors_file}" ]; then
    usage
    exit 2
fi

# Check if the parent directory of output JSON file exists
json_dir=`dirname ${json_errors_file}`
if [ ! -e "${json_dir}" ]; then
    LOG "The json file path is wrong. The parent directory of json file ${json_errors_file} doesn't exist"
    usage
    exit 2
fi

# Remove the Prereq conf file if exists
if [ -e "${json_errors_file}" ]; then
    rm -f ${json_errors_file}
fi

if [ -f libcommon.sh ]; then
. ./libcommon.sh
else
    LOG "File not found : libcommon.sh"
    exit 1
fi

# Check if any invalid switch is passed as an argument
if [ ${wrong_usage} -eq "1" ]; then
    LOG "Wrong argument ${wrong_arg} has passed"
    log_to_json_file ASRMobilityServiceInternalError "Wrong argument ${wrong_arg} has passed"
    usage
    exit 2
fi

# Agent role has to be MS|MT only
if [ -z "${AGENT_ROLE}" ]; then
    LOG "Agent Role can't be empty. It has to be either Agent|MasterTarget only."
    log_to_json_file ASRMobilityServiceInternalError "Agent Role can't be empty. It has to be either Agent|MasterTarget only."
    usage
    exit 2
fi

AGENT_ROLE=$(tr [A-Z] [a-z] <<<${AGENT_ROLE})
if [ "${AGENT_ROLE}" != "agent" -a "${AGENT_ROLE}" != "mastertarget" ] ; then
    LOG "Agent Role can have the values either Agent|MasterTarget only."
    log_to_json_file ASRMobilityServiceInternalError "Agent Role can have the values either MasterTarget only."
    usage
    exit 2
fi

detect_platform
parse_json
validate_checks
LOG "Mount information"
LOG "`echo;mount`"
LOG "df information"
LOG "`echo;df -h`"
LOG "blkid information"
LOG "`echo;blkid`"
collect_volume_info
find_grub_config_file
execute_prereqs

if [ -f "${TEMP_FILE}" ]; then
    rm -f ${TEMP_FILE}
fi

exit ${RETURN_VALUE}
