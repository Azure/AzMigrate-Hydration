#!/bin/bash

_LOG_FILE_="PrepareTarget.Log" #this is not much in use
_DATE_STRING_=`date +'%b_%d_%H_%M_%S'` #this is also for temporary log generation not used in code.
_TEMP_LOG_F_="PrepareTarget-$_DATE_STRING_.log" 
_WORK_FLOW_="" #this is used to mention whether it is P2V or V2P.

_LIB_FILE_="Linux_P2V_Lib.sh"
_INPUT_FILE_="" #Input File for Prepare Target.

_SUCCESS_=0
_FAILED_=1

#GLOBALVAR#
_MOUNT_PATH_="" #mount point
_MOUNT_PATH_FOR_="" #either for boot or etc as of now
_MACHINE_UUID_="" #_InMage_UUID_
_PLAN_NAME_="" #Plan Name for Protection/Recover/DrDrill. The actual input for this parameter is path to failover_data-Folder.
_INSTALL_VMWARE_TOOLS_=1
_UNINSTALL_VMWARE_TOOLS_=0
_NEW_GUID_=""
_VX_INSTALL_PATH_="" #InMage VX Installation Path.
_VX_FAILOVER_DATA_PATH_=""  #InMage VX Failover Data Folder Path. Should we require this?
_FILES_PATH_=""
_TASK_="" #Protect/Add/Recover/DrDrill
_INPUT_CONFIG_FILE_=""
_PROTECTED_DISK_LIST_FILE_PATH_=""
_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_=""
_MPARTITION_="" #Contains partition in format of multipath.
_MDEVICE_="" #will be base Multipath Device for a multipath partition.
_SRC2TGT_MAPPED_DEVICE_="" # contains recovered VM device name after mapping
_SCSI_DEVICE_NAME_FOR_MPARTITION_="" #SCSI device name for a multipath device.
_PARTITON_NUMBER_="" #integer which say i'th partition.
_PARTIION_LIST_ON_A_DEVICE_="" #Holds List of PARTITIONS On a device.
_INMAGE_ORG_="-INMAGE_ORG"
_INMAGE_MOD_="-INMAGE_MOD"
_MULTIPATH_CLEANUP_RETRY_COUNT=10
_VCON_ACTION_FILE_=""
_ORIGINAL_ACTIVE_PARTITION_=""
_SOURCE_CONFIGURATION_DATA_FILE="source_configuration.dat"
_MOD_PROBE_FILE_NAME_="modprobe.conf"
_LOGS_=""
_WORKING_DIRECTORY_="" #Introduced for Azure PAAS model. Holds where all script files and input to scripts are placed.
_SCRIPTS_PATH_=""
_GET_HOST_INFO_XML_FILE_NAME_=""
_PROTECTED_DISKS_LIST_FILE_NAME_=""
###

function ReadInputFile
{
	#check if file exists and it is valid input.
	_INPUT_FILE_=$1
	echo "Input File = \"$_INPUT_FILE_\""
	if [ ! "${_INPUT_FILE_}" ];then
        	echo "ERROR :: Please pass an input file."
	        return $_FAILED_
	fi

	#WE have a valid Input, check for the File.
	if [ ! -f $_INPUT_FILE_ ] || [ ! -r $_INPUT_FILE_  ]; then #need to check the read permission code.
		echo "ERROR :: Could not find file \"$_INPUT_FILE_\"."
		echo "ERROR :: Make sure file \"$_INPUT_FILE_\" exists."
		return $_FAILED_
	fi

	#We have file as well. Read the file
	exec 4<"$_INPUT_FILE_"
	while read -u 4 _attribute_ _value_
	do 
		case $_attribute_ in 
			_MOUNT_PATH_) _MOUNT_PATH_=$_value_;;
			_MOUNT_PATH_FOR_) _MOUNT_PATH_FOR_=$_value_;;
			_MACHINE_UUID_) _MACHINE_UUID_=$_value_;;
			_PLAN_NAME_) _PLAN_NAME_=$_value_;;
			_VX_INSTALL_PATH_) _VX_INSTALL_PATH_=$_value_;;
			_TASK_) _TASK_=$_value_;;
			_MPARTITION_) _MPARTITION_=$_value_;;
			_INSTALL_VMWARE_TOOLS_) _INSTALL_VMWARE_TOOLS_=$_value_;;
			_NEW_GUID_) _NEW_GUID_=$_value_;;
			_WORKING_DIRECTORY_) _WORKING_DIRECTORY_=$_value_;;
			_UNINSTALL_VMWARE_TOOLS_)_UNINSTALL_VMWARE_TOOLS_=$_value_;;
			*) echo "Invalid attribute:$_attribute_ , value:$_value_";;
		esac
	done
	4<&-

	#When VX installation path and Plan name are defined consider as SCOUT otherwise as PAAS model.
	if [ -z $_VX_INSTALL_PATH_ ] && [ -z $_PLAN_NAME_ ] && [ ! -z $_WORKING_DIRECTORY_ ] ; then
		_FILES_PATH_=$_WORKING_DIRECTORY_
		_VX_FAILOVER_DATA_PATH_=$_WORKING_DIRECTORY_
		_PLAN_NAME_=$_WORKING_DIRECTORY_
		_SCRIPTS_PATH_=$_WORKING_DIRECTORY_
		_GET_HOST_INFO_XML_FILE_NAME_="hostinfo-${_MACHINE_UUID_}.xml"
		_PROTECTED_DISKS_LIST_FILE_NAME_="${_MACHINE_UUID_}.PROTECTED_DISKS_LIST"
	else
		_FILES_PATH_=$_VX_INSTALL_PATH_
		_VX_FAILOVER_DATA_PATH_="$_VX_INSTALL_PATH_/failover_data/"
		_SCRIPTS_PATH_="$_VX_INSTALL_PATH_/scripts/vCon/"
		_GET_HOST_INFO_XML_FILE_NAME_="${_MACHINE_UUID_}_cx_api.xml"
		_PROTECTED_DISKS_LIST_FILE_NAME_="${_MACHINE_UUID_}.PROTECTED_DISKS_LIST"
	fi


	echo "MACHINE UUID		= $_MACHINE_UUID_"
	echo "TASK 			= $_TASK_"
	echo "PLAN NAME 		= $_PLAN_NAME_"
	echo "VX INSTALL PATH 		= $_VX_INSTALL_PATH_"
	echo "MOUNT PATH 		= $_MOUNT_PATH_"
	echo "MOUNT PATH FOR 		= $_MOUNT_PATH_FOR_"
	echo "WORKING DIRECTORY 	= $_WORKING_DIRECTORY_"
	echo "FILES PATH 		= $_FILES_PATH_"
	echo "VX FAILOVER DATA PATH 	= $_VX_FAILOVER_DATA_PATH_"
	
	return $_SUCCESS_
}

function ExecuteTask
{
	#TASK CAN BE EITHER Protect/ADDPROTECTION/FAILOVER/INSTALLGRUB.
	#MAKE SURE A TASK IS PASSED.
	if [ ! "${_TASK_}" ]; then
		echo "ERROR :: Failed to determine which task has to be performed from input file \"$_input_file_\"."
		echo "ERROR :: A Task can be either PROTECT/ADDPROTECT/FAILOVER/INSTALLGRUB in file \"$_input_file_\"."
		return $_FAILED_
	fi
	
	#We have a TASK.
	echo "Task = $_TASK_"
	case $_TASK_ in 
		PROTECT) 
				#In Protect scenario we have to Make List of Protected devices in Order..pInfo files does not contain them in order.
				ExecuteTask_PROTECT
				_ret_code_=$?
				;;
		ADDPROTECT)
				#In this case we have to append information of new disk to already existing List.
				ExecuteTask_ADDPROTECT
				_ret_code_=$?
				;;
		PREPARETARGET)
				#In this case All changes for PREPARE TARGET HAS TO BE TAKEN PLACE.
				_WORK_FLOW_="p2v"
				ExecuteTask_PREPARETARGET
				_ret_code_=$?
				;;
		INSTALLGRUB)
				#In this case GRUB has to be installed.Might be this is one of Old hardware where by default GRUB is not installed.
				ExecuteTask_INSTALLGRUB
				_ret_code_=$?
				;;
		PREPARETARGET_V2P)
				#This can be either failback of P2V or direct protection from V -> P. Even is this case this will work.
				_WORK_FLOW_="v2p"
				ExecuteTask_V2P
				_ret_code_=$?
				;;		
			*)
				echo "ERROR ::Invalid Task specified. Task Specified=$_TASK_"
				echo "ERROR ::Task can be either PROTECT/ADDPROTECT/PREPARETARGET."
				_ret_code_=$_FAILED_
				;;
	esac

	return $_ret_code_	
}

function ExecuteTask_INSTALLGRUB
{
	_install_grub_flag=0
	_returnCode_=$_FAILED_
	#Check Whether MACHINE UUID IS SENT
	echo "Executing Task INSTALLGRUB."
	if [ ! "${_MACHINE_UUID_}" ]; then
		echo "ERROR :: A Host ID has to be specified."
		echo "ERROR :: Host ID is required to fetch corresponding configuration file."
		return $_returnCode_
	fi

	#WE HAVE TO CHECK WHETHER VX INSTALL PATH IS SPECIFIED
	if [ -z "$_FILES_PATH_" ]; then
		echo "ERROR :: Working directory where script files exist is not specified."
		echo "ERROR :: Failed to INSTALL GRUB on machine with host id = \"$_MACHINE_UUID_\"."
		return $_returnCode_
	fi

	_INPUT_CONFIG_FILE_="$_VX_FAILOVER_DATA_PATH_/${_MACHINE_UUID_}_Configuration.info"
	if [ ! -f "${_INPUT_CONFIG_FILE_}" ]; then
		echo "ERROR :: Failed to find file \"$_INPUT_CONFIG_FILE_\"."
		echo "ERROR :: Failed to Install GRUB on machine with host id = \"$_MACHINE_UUID_\"."
		return $_returnCode_
	fi

	#NOW WE HAVE THE FILE WE HAVE TO INSTALL GRUB FOR THIS CONFIGURATION.
	#Find the SCSI device for _MPARTITION_
	if [ -z "${_MPARTITION_}" ]; then
		echo "ERROR :: Partition information on which GRUB has to be installed is not specified."
		echo "ERROR :: Please pass a multipath partition information in Input file for installing GRUB."
		return $_returnCode_
	fi
	
	echo "multipath partition on which GRUB has to be installed = $_MPARTITION_."
	FindMultipathDeviceForPartition "$_MPARTITION_"
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to Install GRUB on machine with host id = \"$_MACHINE_UUID_\"."
		return $_returnCode_
	fi

	FindSCSIDeviceForAMultipathDevice "$_MDEVICE_"
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to Install GRUB on machine with host id = \"$_MACHINE_UUID_\"."
		return $_returnCode_
	fi

	_MACHINE_MANUFACTURER_=`grep "Machine Manufacturer                   : " $_INPUT_CONFIG_FILE_ | awk '{print $4}'`
	_MACHINE_PRODUCT_TYPE_=`cat $_INPUT_CONFIG_FILE_ | grep "Machine Product Type" | awk -F": " '{print $2}'`
	_DISTRIBUTION_DETAILS_=`grep "Distribution details                   : " $_INPUT_CONFIG_FILE_ | awk '{print $4}'`
	_ROOT_DEVICE_=`grep "Root device                            : " $_INPUT_CONFIG_FILE_ | awk '{print $4}'`
	_BOOT_DEVICE_=`grep "Boot device                            : " $_INPUT_CONFIG_FILE_ | awk '{print $4}'`
	_PROBE_DEVICE=""
	echo "Machine Manufacturer = ${_MACHINE_MANUFACTURER_}"
	echo "Machine Product Type = ${_MACHINE_PRODUCT_TYPE_}"
	echo "Distribution details = ${_DISTRIBUTION_DETAILS_}"
	echo "Root device : ${_ROOT_DEVICE_}"
	echo "Boot device : ${_BOOT_DEVICE_}"

	case "${_DISTRIBUTION_DETAILS_}" in 
		SLES10*|SLES9*)
			#if [ "${_MACHINE_MANUFACTURER_}" = "HP" ] || [ "${_MACHINE_MANUFACTURER_}" = "Hewlett-Packard" ]; then
        		#	if [ "${_MACHINE_PRODUCT_TYPE_}" = "ProLiant DL380 G4" ]; then
        		#		_install_grub_flag=1
        		#	fi
			#fi
                        if [ -z "${_BOOT_DEVICE_}" ]; then
                        	_PROBE_DEVICE="${_ROOT_DEVICE_}"
                        else
                        	_PROBE_DEVICE="${_BOOT_DEVICE_}"
                        fi
                       	echo "${_PROBE_DEVICE}" | grep -i cciss 2>&1 > /dev/null
                       	if [ $? -eq 0 ]; then
                            _install_grub_flag=1
                       	fi

			if [ $_install_grub_flag -eq 1 ]; then
				_first_partition_=""
				# Check if first parition is not active then proceed otherewise
				# no need to install GRUB in this case.
				_active_partition_=`fdisk -l ${_MDEVICE_} | grep ".*.Linux" | grep "*" | awk '{print $1}'`
				#FIND THE PARTITION NUMBER FOR ACTIVE PARTITION.
				FindPartitionNumber ${_active_partition_} ${_MDEVICE_}
				if [ 0 -ne $? ]; then
					echo "ERROR :: Failed to evaluate GRUB installation procedure = \"$_MACHINE_UUID_\"."
        				_install_grub_flag=0
				else
					
					# Make sure GRUB install does not step on LVM partition
					_first_partition_=`fdisk -l ${_MDEVICE_} | grep -m 1 "Linux" | awk '{ print $1 }'`
					_lvm_partition_=`fdisk -l ${_MDEVICE_} | grep -m 1 "LVM" | awk '{ print $1 }'`
					if [ "${_lvm_partition_}" = "${_first_partition_}" ]; then
						echo "INFO :: Found LVM as first partition. Skipping GRUB installation procedure = \"$_MACHINE_UUID_\"."
        					_install_grub_flag=0
					fi
					if [ ${_PARTITON_NUMBER_} -eq 1 ]; then
        					# DO NOT INSTALL GRUB
        					_install_grub_flag=0
					fi

				fi
				

			else
				echo "INFO :: GRUB installation is not required = \"$_MACHINE_UUID_\"."
        			_install_grub_flag=0
			fi
		;;
		SLES11*|SLES12*)
			echo "GRUB Installation is not required for $_MACHINE_MANUFACTURER:$MACHINE_PRODUCT_TYPE"
			echo "No changes required on ${_DISTRIBUTION_DETAILS_} for GRUB installtion for now"
		;;
		RHEL*)
			echo "GRUB Installation is not required for $_MACHINE_MANUFACTURER:$MACHINE_PRODUCT_TYPE"
			echo "No changes required on ${_DISTRIBUTION_DETAILS_} for GRUB installtion"
		;;
		OL*)
			echo "GRUB Installation is not required for $_MACHINE_MANUFACTURER:$MACHINE_PRODUCT_TYPE"
			echo "No changes required on ${_DISTRIBUTION_DETAILS_} for GRUB installtion"
		;;
		*) echo "Invalid distribution details $_DISTRIBUTION_DETAILS_"
                ;;
	esac

	dd bs=512 count=1 if=${_SCSI_DEVICE_NAME_FOR_MPARTITION_} 2>/dev/null | strings | grep GRUB
	if [ 0 -ne $?  ]; then
		echo "GRUB is not found in sector 0.Turning on flag which says to install GRUB."
		_install_grub_flag=1
	fi

	if [ $_install_grub_flag -eq 0 ]; then
		echo "GRUB Installation is not required."		
		return $_returnCode_
	fi

	#WE HAVE BASE SCSI DEVICE NAME FOR PARTITION.NOW FIND THE PARTITION NUMBER.
	FindListOfMultipathPartitions ${_MDEVICE_}
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to Install GRUB on machine with host id = \"$_MACHINE_UUID_\"."
		return $_returnCode_
	fi
	
	FindPartitionNumber ${_MPARTITION_} ${_MDEVICE_}
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to Install GRUB on machine with host id = \"$_MACHINE_UUID_\"."
		return $_returnCode_
	fi
	
	_wwid_=`echo ${_multipath_device_} | sed -e s:/dev/mapper/:: | sed -e s:/::`
	_multipath_org_file_="/etc/multipath.conf"
	_multipath_mod_file_="${_multipath_org_file_}.BLACKLISTDEVICE"
	_mount_path_="/mnt/tempMount_${_wwid_}"
	
	_counter_=1
	_returnCode_=$_SUCCESS_
	while [ 0 -ne $_counter_ ]
	do
		_counter_=$((_counter_-1))

		COPY ${_multipath_org_file_} ${_multipath_mod_file_}

		BlackListAMultipathDevice ${_MDEVICE_} ${_multipath_org_file_} ${_multipath_mod_file_}
		
		#NOW WE HAVE THE PARTITON NUMBER AS WELL.REMOVE A MULTIPATH DEVICE
		_loop_count_=0
		_returnCode_=$_FAILED_
		while [ $_MULTIPATH_CLEANUP_RETRY_COUNT -ne $_loop_count_ ];
		do
			RemoveAMultipathDevice $_MDEVICE_
			if [ 0 -ne $? ]; then
				echo "ERROR :: Failed to remove multipath device in RemoveAMultipathDevice err:$? host id = \"$_MACHINE_UUID_\". Retrying now"
				`sleep 2`
				_loop_count_=$((_loop_count_+1))
				continue
			fi
			_returnCode_=$_SUCCESS_
			break
		done

		if [ $_FAILED_ -eq $_returnCode_ ]; then
			echo "ERROR :: Failed to remove multipath device in RemoveAMultipathDevice err:$? host id = \"$_MACHINE_UUID_\". Retry failed"
			break
		fi
		
		_scsi_partition_="${_SCSI_DEVICE_NAME_FOR_MPARTITION_}${_PARTITON_NUMBER_}"
		
		if [ 1 != ${_PARTITON_NUMBER_} ]; then
			#MAKE FIRST PARTITION ACTIVE
			MakeFirstPartitionActive "${_SCSI_DEVICE_NAME_FOR_MPARTITION_}"
			if [ 0 -ne $? ]; then
				echo "ERROR :: Failed to Install GRUB on machine with host id = \"$_MACHINE_UUID_\"."
				_returnCode_=$_FAILED_
				break
			fi
		fi

		/sbin/blockdev --rereadpt "${_SCSI_DEVICE_NAME_FOR_MPARTITION_}"
		_rCode_=$?
		if [ 0 -ne $_rCode_ ]; then
			echo "ERROR :: Failed to re-read partition table of device ${_SCSI_DEVICE_NAME_FOR_MPARTITION_}."
		fi

		MountAPartition "${_scsi_partition_}" "${_mount_path_}"
		if [ 0 -ne $? ]; then
			echo "ERROR :: Failed to Install GRUB on machine with host id = \"$_MACHINE_UUID_\"."
			_returnCode_=$_FAILED_
			break
		fi
		
		#FIRST WE HAVE TO TAKE BACK UP OF device.map file and restore it back.
		_device_map_file_=""
		_boot_path_=""
		if [ "boot" = "${_MOUNT_PATH_FOR_}" ] || [ "all" = "${_MOUNT_PATH_FOR_}" ]; then
			if [ "boot" = "${_MOUNT_PATH_FOR_}"  ]; then
				_device_map_file_="${_mount_path_}/grub/device.map"
				_boot_path_=${_mount_path_}
			elif [ -d "$_MOUNT_PATH_/boot" ]; then
				_device_map_file_="${_mount_path_}/boot/grub/device.map"
				_boot_path_="${_mount_path_}/boot/"
			else
				echo "ERROR :: Failed to backup device.map file while installing GRUB."
			fi
		fi

		_device_map_file_mod_="${_device_map_file_}.MODGRUBINSTALL"	
		if [ "${_device_map_file_}" ]; then
			#MOVE
			MOVE "${_device_map_file_}" "${_device_map_file_mod_}"
			
			#CHANGE CONTENTS
			echo "(hd0)	${_SCSI_DEVICE_NAME_FOR_MPARTITION_}" >> ${_device_map_file_}
		fi

		if [ "${_boot_path_}" ]; then
			#write Active Partition file.
			echo "orginal_active_partition=${_ORIGINAL_ACTIVE_PARTITION_}" > "${_boot_path_}/${_SOURCE_CONFIGURATION_DATA_FILE}"
		fi
				
		InstallGRUB "${_SCSI_DEVICE_NAME_FOR_MPARTITION_}" "${_mount_path_}"
		if [ 0 -ne $? ]; then
			echo "ERROR :: Failed to Install GRUB on machine with host id = \"$_MACHINE_UUID_\"."
			_returnCode_=$_FAILED_
		fi

		echo "installed_grub_on_target=1" >> "${_boot_path_}/${_SOURCE_CONFIGURATION_DATA_FILE}"
		if [ "${_device_map_file_}" ]; then
			MOVE "${_device_map_file_mod_}" "${_device_map_file_}"
		fi

	done

	#UNMOUNT THE PARTITON and REVERT MULTIPATH.CONF FILE.
	UnmountAPartition ${_mount_path_}
	`/bin/rmdir  ${_mount_path_}`

	MOVE  ${_multipath_mod_file_} ${_multipath_org_file_}
	
	if [ ! $_returnCode_ ]; then
		echo "Successfully Installed GRUB for host with hostid:$_MACHINE_UUID_"
	fi
	return $_returnCode_
}

#Finds LIST OF PARTITIONS ON A DEVICE.
function FindListOfPartitions 
{
	_device_=$1
	echo "Finding partitions on device \"${_device_}\"."
	
	_PARTIION_LIST_ON_A_DEVICE_=""
	_PARTIION_LIST_ON_A_DEVICE_=`fdisk -l ${_device_} | grep "^${_device_}" | awk '{print $1}'`
	if [ -z "${_PARTIION_LIST_ON_A_DEVICE_}" ]; then
		echo "ERROR :: Failed to find partitions on device \"${_device_}\".Might be device is not partitioned at all."
		return $_FAILED_
	fi	
	return $_SUCCESS_
}

#Finds LIST OF PARTITIONS ON A MULTIPATH DEVICE.
function FindListOfMultipathPartitions 
{
	_device_=$1
	_tmp_device_=`echo $_device_ | sed -e "s:/dev/mapper/::g"`
	echo "Finding partitions on multipath device \"${_device_}\"."
	
	_PARTIION_LIST_ON_A_DEVICE_=""
	_PARTIION_LIST_ON_A_DEVICE_=`dmsetup ls | grep "^${_tmp_device_}" | awk '{ if ($1 != "'"$_tmp_device_"'") { print $1 } }'`
	if [ -z "${_PARTIION_LIST_ON_A_DEVICE_}" ]; then
		echo "ERROR :: Failed to find partitions on device \"${_device_}\".Might be device is not partitioned at all."
		return $_FAILED_
	fi	
	return $_SUCCESS_
}

function RemoveAndAddSingleDevice 
{
	#REMOVE AND ADD A SINFLE DEVICE.
	_scsi_dev_=$1
	echo "trying to remove and add scsi device ${_scsi_dev_}."
	
	#find SCSI id for the device.
	_scsi_id_=`lsscsi | grep -w ${_scsi_dev_} | awk '{print $1}' | sed s:[\[]::g | sed s:[\]]::g`
	echo "scsi id of device \"${_scsi_dev_}\" is ${_scsi_id_}."

	#REMOVE DEVICE
	`echo "scsi remove-single-device ${_scsi_id_}" > /proc/scsi/scsi`
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to remove device \"${_scsi_id_}\"."
		return $_FAILED_
	fi
	`sleep 1`
	#ADD DEVICE
	`echo "scsi add-single-device ${_scsi_id_}" > /proc/scsi/scsi`
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to add device \"${_scsi_id_}\"."
		return $_FAILED_
	fi
	`sleep 1`
	return $_SUCCESS_
}

function MakeFirstPartitionActive
{
	_device_=$1

	#FIND THE ACTIVE PARTITION FROM BELOW COAMMAND
	#fdisk -l /dev/sda | grep ".*.Linux" | grep "*" | awk '{print $1}'

	_active_partition_=`fdisk -l ${_device_} | grep ".*.Linux" | grep "*" | awk '{print $1}'`
	#if [ ! -b "${_active_partition_}" ]; then
	#	echo "ERROR :: Failed to find active partition on device \"${_device_}\"."
	#	return $_FAILED_
	#fi

	#FIND THE PARTITION NUMBER FOR ACTIVE PARTITION.
	FindPartitionNumber ${_active_partition_} ${_device_}
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to Install GRUB on machine with host id = \"$_MACHINE_UUID_\"."
		return $_FAILED_
	fi
	_ORIGINAL_ACTIVE_PARTITION_=${_PARTITON_NUMBER_}
	#THEN EXECUTE FDISK COMMAND TO MAKE THE FIRST PARTITION AS ACTIVE INSTEAD OF n'th PARTITION.
	_activate_partition_script_="${_SCRIPTS_PATH_}/ActivatePartition.sh"
	echo "de-marking \"${_PARTITON_NUMBER_}\" as active using script \"${_activate_partition_script_}\" and marking partition 1 as active."
	${_activate_partition_script_} ${_device_} ${_PARTITON_NUMBER_} 1
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to make first partition as active."
		return $_FAILED_
	fi
	echo "Successfully marked first partition as active."
	return $_SUCCESS_	
}

function MakePartitionActive
{
	_device_=$1
	_make_partition_num_as_active_=$2
	_active_partition_=`fdisk -l ${_device_} | grep ".*.Linux" | grep "*" | awk '{print $1}'`

	#FIND THE PARTITION NUMBER FOR ACTIVE PARTITION.
	FindPartitionNumber ${_active_partition_} ${_device_}
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to Install GRUB on machine with host id = \"$_MACHINE_UUID_\"."
		return $_FAILED_
	fi

	if [ ${_make_partition_num_as_active_} = ${_PARTITON_NUMBER_} ]; then
		echo "Partition ${_make_partition_num_as_active_} is already active."
		return $_SUCCESS_
	fi

	#THEN EXECUTE FDISK COMMAND TO MAKE THE OTHER PARTITION AS ACTIVE INSTEAD OF n'th PARTITION.
	_activate_partition_script_="${_VX_INSTALL_PATH_}/scripts/vCon/ActivatePartition.sh"
	echo "de-marking \"${_PARTITON_NUMBER_}\" as active using script \"${_activate_partition_script_}\" and marking partition ${_make_partition_num_as_active_} as active."
	${_activate_partition_script_} ${_device_} ${_PARTITON_NUMBER_} ${_make_partition_num_as_active_}
	#if [ 0 -ne $? ]; then
	#	echo "ERROR :: Failed to make ${_make_partition_num_as_active_} partition as active."
	#	return $_FAILED_
	#fi
	echo "Successfully marked ${_make_partition_num_as_active_} partition as active."
	return $_SUCCESS_
}


function UnmountAPartition
{
	_partition_=$1

	echo "Unmounting partition ${_partition_}"
	
	`umount ${_partition_}`
	if [ 0 -ne $? ]; then
		echo "Failed to unmount partition \"${_partition_}\"."
	fi
	
	return $_SUCCESS_
}

#Mounts a Partition
#_IN_ partition
#_IN_ mountpath
function MountAPartition
{
	_partition_=$1
	_mount_path_=$2
	echo "mounting partition $_partition_ at $_mount_path_"

	#FIRST CHECK IF ALREADY A MOUNT EXISTS? MIGHT BE AN EMPTY DIRECTORY?
	if [ -d "${_mount_path_}" ] && [ -b  "${_mount_path_}" ]; then
		echo "ERROR :: Already some device is mounted in path \"${_mount_path_}\"."
		return $_FAILED_
	fi

	#WE HAVE TO CREATE DIRECTORY.
	`/bin/mkdir -p ${_mount_path_}`
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to create mount path \"${_mount_path_}\" to mount partition \"${_partition_}\"."
		return $_FAILED_
	fi

	#NOW MOUNT THE PARTITION.WE NEEED TO KNOW THE FILE SYSTEM FOR IT?
	`mount ${_partition_} ${_mount_path_}`
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to mount partition \"${_partition_}\" at \"${_mount_path_}\""
		`/bin/rmdir  ${_mount_path_}`
		return $_FAILED_
	fi 
	return $_SUCCESS_
}

#It installs GRUB.
#_IN_ MOUNT_PATH
#_IN_ Partition
function InstallGRUB
{
	_device_=$1
	_mount_path_=$2
	
	_active_partition_="${_device_}1" 
	echo "Install GRUB : ${_device_} , ${_mount_path_}, ${_active_partition_}."
	grub-install --force-lba --no-floppy --root-directory=${_mount_path_} ${_active_partition_}
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to install GRUB on partition \"${_partition_}\".Using option --recheck."
		grub-install --force-lba --no-floppy --recheck --root-directory=${_mount_path_} ${_active_partition_}
		if [ 0 -ne $? ]; then
			echo "ERROR :: Failed to install GRUB on partition \"${_partition_}\"."
			return $_FAILED_
		fi
	fi

	#Performing a check to see whether GRUB exists in sector 0	
	dd bs=512 count=1 if=${_device_} 2>/dev/null | strings | grep GRUB
	if [ 0 -ne $?  ]; then
		echo "GRUB is not found in sector 0.executing command to install it on base device."
		echo "Install GRUB : ${_device_} , ${_mount_path_}, ${_active_partition_}."
		grub-install --force-lba --no-floppy --root-directory=${_mount_path_} ${_device_}
		if [ 0 -ne $? ]; then
			echo "ERROR :: Failed to install GRUB on device \"${_device_}\".Using option --recheck."
			grub-install --force-lba --no-floppy --recheck --root-directory=${_mount_path_} ${_device_}
			if [ 0 -ne $? ]; then
				echo "ERROR :: Failed to install GRUB on device \"${_device_}\"."
				return $_FAILED_
			fi
		fi
	fi

	return $_SUCCESS_
}

#Finds PartitionNumber
#_IN_ BASE DEVICE --> either /dev/sda (or) /dev/cciss/c0do (or) /dev/mapper/<id>
#_IN_ PARTITION --> either /dev/sda1 (or) /dev/cciss/c0d0p1 (or) /dev/mapper/<id>p1
function FindPartitionNumber
{
	_partition_=$1
	_device_=$2
	echo "finding partition number of partition \"${_partition_}\" on device \"${_device_}\"."	
	#FIRST WE NEED TO CHECK WHETHER PARTITION AND DEVICE CORRESPONDS TO EACH OTHER?
	echo ${_partition_} | grep ${_device_} > /dev/null 2>&1
	if [ 0 -ne $? ]; then
		echo "ERROR :: device \"${_device_}\" and partition \"${_partition_}\" does not corresponds to each other."
		return $_FAILED_
	fi

	_partition_number_=`echo ${_partition_} | sed -e "s:${_device_}::"`	
	#CHECK IF PARTITION NUMBER CONTAINS ALPHABETS?
	_partition_number_=`echo ${_partition_number_} | sed -e "s:[a-z|A-Z]*::"`
	
	echo "partition \"${_partition_}\" is $_partition_number_ partition of device \"${_device_}\"."
	_PARTITON_NUMBER_=${_partition_number_}
	return $_SUCCESS_
}

#It finds SCSI name for a multipath device
#_IN_ multipath device --> /dev/mapper/<id>
function FindSCSIDeviceForAMultipathDevice 
{
	_multipath_device_=$1
	echo "Finding SCSI device for multipath device \"${_multipath_device_}\"."

	_scsi_device_name_=`multipath -l ${_multipath_device_} | awk 'END {print $3}'` >/dev/null 2>&1
	echo "scsi device name for multipath device \"${_multipath_device_}\" is \"${_scsi_device_name_}\"."
	_scsi_device_name_="/dev/${_scsi_device_name_}"

	if [ -z "${_scsi_device_name_}" ]; then
		echo "ERROR :: Failed to find scsi device name for partition \"${_MPARTITION_}\"."
		return $_FAILED_
	fi
	
	_SCSI_DEVICE_NAME_FOR_MPARTITION_=$_scsi_device_name_
	
	return $_SUCCESS_
}

function BlackListAMultipathDevice
{
	#WE NEED TO BLOCKLIST THE DEVICE FROM MULTIPATH.CONF FILE.
	#blacklist {
	#       wwid 26353900f02796769
	#	devnode "^(ram|raw|loop|fd|md|dm-|sr|scd|st)[0-9]*"
	#	devnode "^hd[a-z]"
	#}
	# Assumes that blacklist does not start with #, needs to be handled
	_multipath_device_=$1
	_multipath_org_file_=$2
	_multipath_mod_file_=$3

	_wwid_=`echo ${_multipath_device_} | sed -e s:/dev/mapper/:: | sed -e s:/::`
	sed -e "/^blacklist.*{/ a wwid ${_wwid_}" ${_multipath_mod_file_} > ${_multipath_org_file_}
	return $_SUCCESS_
}

#It Finds the base multipath device for a partition.
#_IN_ Partition --> Should be in format of /dev/mapper/<ID>
function FindMultipathDeviceForPartition
{
	_multipath_partition_=$1
	echo "finding base multipath device for multipath partition ${_multipath_partition_}."	
	_multipath_list_=`multipath -ll | grep -i "Virtual disk" | awk '{print $1}'` > /dev/null 2>&1
	echo "multipath device list = ${_multipath_list_}."
	_multipath_device_=""
	for _multipath_id_ in ${_multipath_list_}
	do
		_contains_mpartition_=`fdisk -l /dev/mapper/${_multipath_id_} | grep ${_multipath_partition_}` > /dev/null 2>&1
		if [ -z "${_contains_mpartition_}" ]; then
			continue
		fi
		
		_multipath_device_="/dev/mapper/${_multipath_id_}"
		echo "base multipath device for partition \"${_multipath_partition_}\" is \"${_multipath_device_}\"."
	done
	
	if [ -z "${_multipath_device_}" ]; then
		echo "ERROR :: Failed to find multipath device for partition \"${_multipath_partition_}\"."
		return $_FAILED_
	fi
	_MDEVICE_=${_multipath_device_}
	return $_SUCCESS_
}

function RemoveAMultipathDevice
{
	_MDEVICE_=$1
	echo "Cleaning up Multipath device \"${_MDEVICE_}\""
	for _m_partition_ in ${_PARTIION_LIST_ON_A_DEVICE_}
	do
		echo "Removing partition \"${_m_partition_}\""
		dmsetup -f remove ${_m_partition_} > /dev/null 2>&1
		if [ 0 -ne $? ]; then
			echo "ERROR :: Failed to remove partition \"${_m_partition_}\"."
			continue
		fi
		`sleep 3`
	done

	#RUN REMOVE command
	`sleep 10`
	if [ ! -b ${_MDEVICE_} ]; then
		echo "Device \"${_MDEVICE_}\" is not a block device."
		return $_FAILED_
	fi

	dmsetup -f remove ${_MDEVICE_} > /dev/null 2>&1
	if [ 0 -ne $? ]; then
		dmsetup clear ${_MDEVICE_} > /dev/null 2>&1
		dmsetup -f remove ${_MDEVICE_} > /dev/null 2>&1
		if [ 0 -ne $? ]; then
			echo "ERROR :: Failed to remove multipath device \"${_MDEVICE_}\"."
			ps -aef
			dmsetup ls | grep "${_MDEVICE_}"
			dmsetup info ${_MDEVICE_}
			dmsetup ls --tree
			lsof "${_MDEVICE_}"
			fuser "${_MDEVICE_}"
			#Remove ALL Partitions using kpartx -d
			kpartx -d ${_MDEVICE_}
			return $_FAILED_
		fi
	fi

	#RUN CLEAR command.
	dmsetup clear ${_MDEVICE_} > /dev/null 2>&1
	if [ 0 -ne $? ]; then
	       echo "ERROR :: Failed to clear multipath device \"${_MDEVICE_}\"."
	fi

	`sleep 2`
	return $_SUCCESS_
}

function FindSCSIDeviceForMultipathPartition 
{
	#First find the corresponding multipath device for partition.
	_multipath_list_=`multipath -ll | grep -i "Virtual disk" | awk '{print $1}'` >/dev/null 2>&1
	echo "multipath devices = ${_multipath_list_}."
	_scsi_device_name_=""
	for _multipath_id_ in ${_multipath_list_}
	do
		_multipath_device_="/dev/mapper/${_multipath_id_}"
		_contains_mpartition_=`fdisk -l ${_multipath_device_} | grep ${_MPARTITION_}` >/dev/null 2>&1
		if [ -z "${_contains_mpartition_}" ]; then
			continue
		fi
		echo "base multipath device for partition \"${_MPARTITION_}\" is \"${_multipath_device_}\"."
		
		_scsi_device_name_=`multipath -l ${_multipath_device_} | awk 'END {print $3}'` >/dev/null 2>&1
		echo "scsi device name for multipath device \"${_multipath_device_}\" is \"${_scsi_device_name_}\"."
		_scsi_device_name_="/dev/${_scsi_device_name_}"
		break
	done

	if [ -z "${_scsi_device_name_}" ]; then
		echo "ERROR :: Failed to find scsi device name for partition \"${_MPARTITION_}\"."
		return $_FAILED_
	fi
	
	_SCSI_DEVICE_NAME_FOR_MPARTITION_=$_scsi_device_name_
	
	return $_SUCCESS_
}

#THIS FUNCTION MAKE SURE THAT A NEW FILE IS CREATED FOR HOST SPECIFIED. FILE CONTAINS PROTECTED DEVICES IN ORDER.
#IT READS INPUT FROM FILE _INPUT_INMAGE_UNIT_DISKS_FILE_ 
function ExecuteTask_PROTECT
{
	#MAKE SURE WE HAVE AN INMAGE_HOST_ID
	echo "Executing Task PROTECT."
	if [ ! "${_MACHINE_UUID_}" ]; then
		echo "ERROR :: A Host ID has to be specified."
		echo "ERROR :: Host ID is required to write protected disk list"
		return $_FAILED_
	fi

	#WE HAVE TO CHECK WHETHER VX INSTALL PATH IS SPECIFIED
	if [ -z "$_FILES_PATH_" ]; then
		echo "ERROR :: Working directory where script files exist is not specified."
		echo "ERROR :: Failed to write protected disks list for host with host id = \"$_MACHINE_UUID_\"."
		return $_FAILED_
	fi

	#WE HAVE HOST ID,MAKE SURE INPUT FILE PATH IS SPECIFIED 	
	_INMAGE_SCSI_UNITS_FILE_PATH_="$_PLAN_NAME_/Inmage_scsi_unit_disks.txt"
	_PROTECTED_DISK_LIST_FILE_PATH_="$_VX_FAILOVER_DATA_PATH_/${_PROTECTED_DISKS_LIST_FILE_NAME_}"
	_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_="$_VX_FAILOVER_DATA_PATH_/${_PROTECTED_DISKS_LIST_FILE_NAME_}"
	echo "InMage scsi units file path = $_INMAGE_SCSI_UNITS_FILE_PATH_"
	echo "Protected Disk List File = $_PROTECTED_DISK_LIST_FILE_PATH_"
	
	#WE HAVE FILE NAME, CHECK IF FILE EXISTS
	if [ ! -f "$_INMAGE_SCSI_UNITS_FILE_PATH_" ] || [ ! -r "${_INMAGE_SCSI_UNITS_FILE_PATH_}" ]; then
		echo "ERROR :: Failed to find/read file \"$_INMAGE_SCSI_UNITS_FILE_PATH_\"."
		echo "ERROR :: Please make sure that \"$_INMAGE_SCSI_UNITS_FILE_PATH_\" exists and is granted permmission to read."
		return $_FAILED_
	fi
	
	#NEED TO DELETE FILE IF ALREADY EXISTS IN /INMAGE_INSTALLATION_PATH/failover_data
	if [ -f "$_PROTECTED_DISK_LIST_FILE_PATH_" ]; then
		echo "As Task is PROTECT removing file \"$_PROTECTED_DISK_LIST_FILE_PATH_\"."
		rm -rf "$_PROTECTED_DISK_LIST_FILE_PATH_"
	fi

	#WE HAVE FILE AS WELL. READ THE FILE AND OUTPUT IT"S CONTENT IN TO A PROTECTED_DISK_LIST.
	tr -d '\r' < $_INMAGE_SCSI_UNITS_FILE_PATH_ > $_INMAGE_SCSI_UNITS_FILE_PATH_.tmp
	cp -f $_INMAGE_SCSI_UNITS_FILE_PATH_.tmp $_INMAGE_SCSI_UNITS_FILE_PATH_
	_line_num_=`cat $_INMAGE_SCSI_UNITS_FILE_PATH_ | grep -n "$_MACHINE_UUID_" | awk -F":" '{print $1}'`
	echo "Line Num = $_line_num_"
	_next_line_num_=$((_line_num_+1))
	echo "next line number = $_next_line_num_"
	_num_lines_to_read_=`sed -n "$_next_line_num_ , $_next_line_num_ p" "$_INMAGE_SCSI_UNITS_FILE_PATH_"`
	_read_from_line_num_=`echo $_num_lines_to_read_ | sed -e "s:^M::g"`
	_read_from_line_num_=$((_next_line_num_ + 1))
	_read_untill_line_num_=$((_next_line_num_+_num_lines_to_read_))
	_lines_=`sed -n "$_read_from_line_num_ , $_read_untill_line_num_ p" "$_INMAGE_SCSI_UNITS_FILE_PATH_"`
	for _line_ in ${_lines_}
	do
		echo $_line_>>"$_PROTECTED_DISK_LIST_FILE_PATH_"
	done

	if [ -f $_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_ ]; then
		rm -f $_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_
	fi

	_protected_dev_entry=0
	echo "Mapping table for protected source devices and target devices under recovered VM" >> $_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_
	echo "Source device		Target device in recovered VM" >> $_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_
	for _line_ in ${_lines_}
	do
		# Get protected device name
		_src_dev_=`echo $_line_ | awk -F'!@!@!' '{ print $1 }'`
		GetTargetDiskName $_protected_dev_entry 
		target_device=$_disk_name_
		echo "${_src_dev_}	${target_device}" >> $_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_
		_protected_dev_entry=$((_protected_dev_entry+1))
	done
	return $_SUCCESS_
}

#THIS FUNCTION MAKE SURE THAT A NEW PROTECTED DEVICES ARE APPENDED IN ORDER TO FILE.
#IT READS INPUT FROM FILE _INPUT_INMAGE_UNIT_DISKS_FILE_
function ExecuteTask_ADDPROTECT
{
	#MAKE SURE WE HAVE AN INMAGE_HOST_ID
	echo "Executing Task ADDPROTECT."
	if [ ! "${_MACHINE_UUID_}" ]; then
		echo "ERROR :: A Host ID has to be specified."
		echo "ERROR :: Host ID is required to write protected disk list"
		return $_FAILED_
	fi
	#WE HAVE TO CHECK WHETHER VX INSTALL PATH IS SPECIFIED
	if [ -z "$_FILES_PATH_" ]; then
		echo "ERROR :: Working direcotry where script files exist is not specified."
		echo "ERROR :: Failed to write protected disks list for host with host id = \"$_MACHINE_UUID_\"."
		return $_FAILED_
	fi
	#WE HAVE HOST ID,MAKE SURE INPUT FILE PATH IS SPECIFIED
	_INMAGE_SCSI_UNITS_FILE_PATH_="$_PLAN_NAME_/Inmage_scsi_unit_disks.txt"
	_PROTECTED_DISK_LIST_FILE_PATH_="$_VX_FAILOVER_DATA_PATH_/${_PROTECTED_DISKS_LIST_FILE_NAME_}"
	_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_="$_VX_FAILOVER_DATA_PATH_/${_PROTECTED_DISKS_LIST_FILE_NAME_}"
	echo "InMage scsi units file path = $_INMAGE_SCSI_UNITS_FILE_PATH_"
	echo "Protected Disk List File = $_PROTECTED_DISK_LIST_FILE_PATH_"

	#WE HAVE FILE NAME, CHECK IF FILE EXISTS
	if [ ! -f "$_INMAGE_SCSI_UNITS_FILE_PATH_" ] || [ ! -r "${_INMAGE_SCSI_UNITS_FILE_PATH_}" ]; then
		echo "ERROR :: Failed to find/read file \"$_INMAGE_SCSI_UNITS_FILE_PATH_\"."
		echo "ERROR :: Please make sure that \"$_INMAGE_SCSI_UNITS_FILE_PATH_\" exists and is granted permmission to read."
		return $_FAILED_
	fi

	#WE HAVE FILE AS WELL. READ THE FILE AND OUTPUT IT"S CONTENT IN TO A PROTECTED_DISK_LIST.
	tr -d '\r' < $_INMAGE_SCSI_UNITS_FILE_PATH_ > $_INMAGE_SCSI_UNITS_FILE_PATH_.tmp
	cp -f $_INMAGE_SCSI_UNITS_FILE_PATH_.tmp $_INMAGE_SCSI_UNITS_FILE_PATH_
	_line_num_=`cat $_INMAGE_SCSI_UNITS_FILE_PATH_ | grep -n "$_MACHINE_UUID_" | awk -F":" '{print $1}'`
	echo "Line Num = $_line_num_"
	_next_line_num_=$((_line_num_+1))
	echo "next line number = $_next_line_num_"
	_num_lines_to_read_=`sed -n "$_next_line_num_ , $_next_line_num_ p" "$_INMAGE_SCSI_UNITS_FILE_PATH_"`
	_read_from_line_num_=`echo $_num_lines_to_read_ | sed -e "s:^M::g"`
	_read_from_line_num_=$((_next_line_num_ + 1))
	_read_untill_line_num_=$((_next_line_num_+_num_lines_to_read_))
	_lines_=`sed -n "$_read_from_line_num_ , $_read_untill_line_num_ p" "$_INMAGE_SCSI_UNITS_FILE_PATH_"`
																
	for _line_ in ${_lines_}
	do
		echo $_line_>>"$_PROTECTED_DISK_LIST_FILE_PATH_"
	done
	_protected_dev_entry=`cat $_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_ | wc -l` 
	for _line_ in ${_lines_}
	do
		# Get protected device name
		_src_dev_=`echo $_line_ | awk -F'!@!@!' '{ print $1 }'`
		GetTargetDiskName $_protected_dev_entry 
		target_device=$_disk_name_
		echo "${_src_dev_}	${target_device}" >> $_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_
		_protected_dev_entry=$((_protected_dev_entry+1))
	done
	return $_SUCCESS_
}

#THIS FUNCTION PREPARES TARGET INFORMATION AND MAKES NECESSARY CHANGES.
function ExecuteTask_PREPARETARGET
{
	#FIRST THING TO MAKE SURE --> _MOUNT_PATH_ ?? DID WE GET SOME STRING?
	if [ ! "${_MOUNT_PATH_}" ]; then
		echo "ERROR :: Mount point is not specified."
		echo "ERROR :: Mount point has to be specfied in order to modify files."
		return $_FAILED_
	fi
	
	#NEED TO CHECK IF MOUNT PATH SPECIFIED IS A BLOCK DEVICE
	#if [ ! -b "$_MOUNT_PATH_" ]; then
	#	echo "ERROR :: Invalid mount point. Mount point : $_MOUNT_PATH_."
	#	echo "ERROR :: Please specify a valid mount point which is of a block device type."
	#	return $_FAILED_
	#fi
	_LOGS_="$_PLAN_NAME_/${_MACHINE_UUID_}-Logs/"
	if [ ! -d "$_LOGS_" ]; then
		mkdir -p "${_LOGS_}"
	fi

	_ret_code_boot_=$_SUCCESS_
	_ret_code_etc_=$_SUCCESS_
	if [ "boot" = "${_MOUNT_PATH_FOR_}" ] || [ "all" = "${_MOUNT_PATH_FOR_}" ]; then
		if [ "boot" = "${_MOUNT_PATH_FOR_}"  ]; then
			BackupConfigFile "$_MOUNT_PATH_"
			Prepare_boot "$_MOUNT_PATH_"
			_ret_code_boot_=$?
		elif [ -d "$_MOUNT_PATH_/boot" ]; then
			BackupConfigFile "$_MOUNT_PATH_/boot/"
			Prepare_boot "$_MOUNT_PATH_/boot/"
			_ret_code_boot_=$?
		else
			echo "ERROR :: Cross check whether mounted partition contains boot information or not."
			_ret_code_boot_=$_FAILED_
		fi
	fi

	if [ "etc" = "${_MOUNT_PATH_FOR_}" ] || [ "all" = "${_MOUNT_PATH_FOR_}" ]; then
		if [ -d "$_MOUNT_PATH_/etc" ]; then
			Prepare_etc "$_MOUNT_PATH_/etc"
			_ret_code_etc_=$?
		elif [ -f "$_MOUNT_PATH_/fstab" ]; then
			Prepare_etc "$_MOUNT_PATH_"
			_ret_code_etc_=$?
		else
			echo "ERROR :: Cross check whether mounted partition contains etc information or not."
			_ret_code_boot_=$_FAILED_
		fi
	fi
	
	if [ $_FAILED_ -eq $_ret_code_boot_ ] || [ $_FAILED_ -eq $_ret_code_etc_ ]; then
		return $_FAILED_
	fi

	return $_SUCCESS_
}

#THIS FUNCTION MODIFIES INITRD MODULES LIST WHICH ARE LOADED DURING BOOT.
function ModifyInitrdModulesList
{
	_kernel_base_file_path_=$1
	_kernel_config_file_path_="$_kernel_base_file_path_/kernel"
	_kernel_config_file_orig_="${_kernel_config_file_path_}${_INMAGE_ORG_}"
	
	#MAKE SURE THAT FILE EXISTS?
	if [ ! -f $_kernel_config_file_path_ ]; then
		echo "WARNING :: Failed to find file \"$_kernel_config_file_path_\"."
		return $_SUCCESS_
	fi

	#FILE EXISTS TAKE A COPY
	if [ -f "${_kernel_config_file_orig_}" ]; then
		MOVE "${_kernel_config_file_orig_}" "$_kernel_config_file_path_"
	fi
	COPY "$_kernel_config_file_path_" "${_kernel_config_file_orig_}"

	#MODIFY - valid only for SuSE as INITRD_MODULES exists only on SuSE
	sed -i "/^INITRD_MODULES=\"/{
		s/\"$/ sd_mod mptspi\"/
	}" $_kernel_config_file_path_
	return $_SUCCESS_
}


function MapSrcToTgt
{
	_SRC2TGT_MAPPED_DEVICE_=""
	_CX_XML_FILE_="$_VX_FAILOVER_DATA_PATH_/${_GET_HOST_INFO_XML_FILE_NAME_}"
	_search_dev_=$1
	_lv_alias=""
	_base_device=""

	if [ -z $_search_dev_ ]; then
		# Empty input
		return $_FAILED_
	fi

	# Find if _search_dev_ is a logical volume
	IsDevice_LV $_search_dev_
	_lv_alias=$_lv_dev_name_

	Deref_SymLink $_search_dev_
	_real_device_name=$_real_partition_name_

	# Now find the base device from device hierarchy
	if [ -z $_lv_alias ]; then
		if [ -z $_real_device_name ]; then
			MapDeviceHierarchyToBaseDevice $_CX_XML_FILE_ $_search_dev_
		else
			MapDeviceHierarchyToBaseDevice $_CX_XML_FILE_ $_real_device_name
		fi
		_base_device=$_base_diskname_
	else
		MapDeviceHierarchyToBaseDevice $_CX_XML_FILE_ $_search_dev_
		_base_device=$_base_diskname_
		if [ -z $_base_device ]; then
			MapDeviceHierarchyToBaseDevice $_CX_XML_FILE_ $_lv_alias
			_base_device=$_base_diskname_
		fi
	fi

	if [ -z $_base_device ]; then
			return $_FAILED_
	else
		GetProtectedDeviceEntry $_base_device
		_protected_dev_entry=$?
		echo "entry = ${_protected_dev_entry}."
		if [ "${_protected_dev_entry}" = "0" ]; then
			return $_FAILED_
		fi
	fi
	
	# _search_dev_ contains device that needs to be replaced with UUID
	if [ ${_search_dev_} ]; then
		if [ -z $_lv_alias ]; then
			partition_num=""
			_protected_dev_entry=$((_protected_dev_entry-1))
			GetTargetDiskName $_protected_dev_entry 
			target_device=$_disk_name_
			if [ -z $_real_device_name ]; then
				tp_str=`echo ${_search_dev_} | sed "s:[0-9]*$::"`
				partition_num=`echo ${_search_dev_} | sed "s:${tp_str}::"`
				target_device="${target_device}${partition_num}"
			else
				tp_str=`echo ${_real_device_name} | sed "s:[0-9]*$::"`
				partition_num=`echo ${_real_device_name} | sed "s:${tp_str}::"`
				target_device="${target_device}${partition_num}"
			fi
			_SRC2TGT_MAPPED_DEVICE_=$target_device
		else
			_SRC2TGT_MAPPED_DEVICE_=$_search_dev_
		fi
	fi

	if [ -z "${_SRC2TGT_MAPPED_DEVICE_}" ]; then
		return $_FAILED_
	fi
	return $_SUCCESS_
}
function ModifyFstabFile
{
	_fstab_file_=$1
	#have a back up of original file.
	_CX_XML_FILE_="$_VX_FAILOVER_DATA_PATH_/${_GET_HOST_INFO_XML_FILE_NAME_}"
	_fstab_file_orig_="${_fstab_file_}${_INMAGE_ORG_}"
		
	#if [ -f "${_fstab_file_orig_}" ]; then
	#	MOVE "${_fstab_file_orig_}" "$_fstab_file_"
	#fi
	COPY "$_fstab_file_" "${_fstab_file_orig_}"
	
	cat "$_fstab_file_orig_" | while read _line_
	do 
		echo "Line read = ${_line_}."
		_uuid_=""
		_label_=""
		_search_dev_=""
		_lv_alias=""
		_base_device=""
		# Find if base device of UUID or LABEL is protected
		temp_var=`echo $_line_ | egrep "^[ ]*UUID|[ ]*LABEL" | awk -F"=" '{print $1 }'`
		if [ "${temp_var}" ]; then
			# Find the device name with UUID or LABEL to see if it protected or not
			_uuid_=`echo $_line_ | awk -F"UUID=" '{ print $2 }' | awk '{ print $1 }' | sed -e "s/\"//g"`
			if [ "${_uuid_}" ]; then
				Find_Device_From_UUID $_uuid_
				_search_dev_=$_dev_name_
			else
				_label_=`echo $_line_ | awk -F"LABEL=" '{ print $2 }' | awk '{ print $1 }' | sed -e "s/\"//g"`
				
				Find_Device_From_LABEL $_label_
				_search_dev_=$_dev_name_
			fi
		else
			# Take care of device name(/dev/*) entries in /etc/fstab
			_fstab_dev_name_=`echo $_line_ | awk '{print $1}'`
			echo "$_INPUT_CONFIG_FILE_"
			echo `sed -n  "/^[=]* FSTAB UUID Information - START [=]*/,/^[=]* FSTAB UUID Information - END [=]*/p" "$_INPUT_CONFIG_FILE_"`
			_fstab_dev_safe_pattern_=$(printf "%s\n" "$_fstab_dev_name_" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
			_temp_search_dev_=`sed -n  "/^[=]* FSTAB UUID Information - START [=]*/,/^[=]* FSTAB UUID Information - END [=]*/p" "$_INPUT_CONFIG_FILE_" | grep "${_fstab_dev_safe_pattern_}:" | head -1 | awk -F': ' '{print $1}'`
			echo "temp_search_dev = ${_temp_search_dev_}."
			if [ "${_temp_search_dev_}" ]; then
				_search_dev_=`echo "$_temp_search_dev_" | awk '{ print $1 }'`
				_search_dev_=`echo $_search_dev_ | sed -e "s: ::g"`
			else
				# Entry is not a label or uuid or block device
				# or if swap entry is a file, then control would come here.
				continue
			fi
		fi

		if [ -z $_search_dev_ ]; then
			# comment out UUID or LABEL entry 
			# $_label_ or $_uuid_ or $_search_dev_
			continue
		fi

		# Find if _search_dev_ is a logical volume
		IsDevice_LV $_search_dev_
		_lv_alias=$_lv_dev_name_

		Deref_SymLink $_search_dev_
		_real_device_name=$_real_partition_name_

		# Now find the base device from device hierarchy
		if [ -z $_lv_alias ]; then
			if [ -z $_real_device_name ]; then
				MapDeviceHierarchyToBaseDevice $_CX_XML_FILE_ $_search_dev_
			else
				MapDeviceHierarchyToBaseDevice $_CX_XML_FILE_ $_real_device_name
			fi
			_base_device=$_base_diskname_
		else
            echo "${_lv_alias} is an LV."
			MapDeviceHierarchyToBaseDevice $_CX_XML_FILE_ $_search_dev_
			_base_device=$_base_diskname_
			if [ -z $_base_device ]; then
				MapDeviceHierarchyToBaseDevice $_CX_XML_FILE_ $_lv_alias
				_base_device=$_base_diskname_
			fi
		fi

		if [ -z $_base_device ]; then
			# comment out UUID or LABEL entry 
			# $_label_ or $_uuid_ or $_search_dev_
			continue
		else
			GetProtectedDeviceEntry $_base_device
			_protected_dev_entry=$?
			echo "entry = ${_protected_dev_entry}."
			if [ "${_protected_dev_entry}" = "0" ]; then
				# comment out UUID or LABEL entry 
				# $_label_ or $_uuid_ or $_search_dev_
				safe_pattern=$(printf "%s\n" "$_line_" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
				sed -i "s/${safe_pattern}/#${safe_pattern}/" "${_fstab_file_}"
				continue
			fi
			if [  "${_uuid_}" ] || [ "${_label_}" ] || [ "${_lv_alias}" ]; then
				# It is protected and  if it's a uuid or label or lv entry in fstab file then continue.
				continue
			fi
		fi
		
		# _search_dev_ contains device that needs to be replaced with UUID
		if [ ${_search_dev_} ]; then
			Find_UUID_FSTABINFO $_search_dev_
			final_uuid=$_UUID_
			echo "Final UUID = $_UUID_"		 
			if [ "$final_uuid" ]; then
				#Replace with UUID.
				safe_pattern=$(printf "%s\n" "$_search_dev_" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
				sed -i "s/${safe_pattern}\s/UUID=$final_uuid /" "${_fstab_file_}"
			else
				#Need to take care of swap file.	
				#check if Line contains swap
				echo $_line_ | grep -i "swap" >/dev/null
				if [ 0 -eq $? ]; then
					echo "SWAP partition : $_search_dev_"
					#now we have to match this partition to target device.
					if [ -z $_lv_alias ]; then
						# it is a partition, so map it target device on recovered VM
						#_protected_disklist="$_VX_INSTALL_PATH_/failover_data/$_MACHINE_UUID_.PROTECTED_DISKS_LIST"
						#_protected_dev_entry=`GetProtectedDeviceEntry $`
						partition_num=""
						_protected_dev_entry=$((_protected_dev_entry-1))
						GetTargetDiskName $_protected_dev_entry 
						target_device=$_disk_name_
						if [ -z $_real_device_name ]; then
							tp_str=`echo ${_search_dev_} | sed "s:[0-9]*$::"`
							partition_num=`echo ${_search_dev_} | sed "s:${tp_str}::"`
							target_device="${target_device}${partition_num}"
						else
							tp_str=`echo ${_real_device_name} | sed "s:[0-9]*$::"`
							partition_num=`echo ${_real_device_name} | sed "s:${tp_str}::"`
							target_device="${target_device}${partition_num}"
						fi
						safe_pattern=$(printf "%s\n" "$_search_dev_" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
						safe_pattern1=$(printf "%s\n" "$target_device" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
						sed -i "s/${safe_pattern}/${safe_pattern1}/" "${_fstab_file_}"
					fi
				fi
			fi
		fi
	done
	return $_SUCCESS_
}


#THIS FUNCTION MODIFIES "SYSCONFIG/KERNEL" FILE AND FSTAB FILE.
function Prepare_etc
{
	_etc_path_="$1"
	_returnCode_=$_SUCCESS_
	
	_INPUT_CONFIG_FILE_="$_etc_path_/Configuration.info"
	
	if [ -d "${_etc_path_}/sysconfig" ]; then
		ModifyInitrdModulesList "$_etc_path_/sysconfig"
		if [ 0 -ne $? ]; then
		    _returnCode_=$_FAILED_
		fi
	else
	    echo "WARNING :: Failed to find \"sysconfig\" folder under mount point \"$_etc_path_\"."
		echo "Skipping initrd modules list changes."
	fi

	if [ ! -f "$_etc_path_/fstab" ]; then
		echo "ERROR :: Failed to find file \"fstab\" file under path \"$_etc_path_\"."
		echo "ERROR :: Failed to modify fstab file."
		_returnCode_=$_FAILED_
		#return $_FAILED_
	fi

	ModifyFstabFile "$_etc_path_/fstab"
	if [ 0 -ne $? ]; then
		_returnCode_=$_FAILED_
		#return $?
	fi

	ModifyModprobeFile "$_etc_path_"
	if [ 0 -ne $? ]; then
		_returnCode_=$_FAILED_
		#return $?
	fi
	
	_lvm_conf_file_="$_etc_path_/lvm/lvm.conf"
	ModifyLvmConfFile "$_lvm_conf_file_"
	if [ 0 -ne $? ]; then
		_returnCode_=$_FAILED_
		#return $?
	fi
	
	COPY "${_etc_path_}/multipath.conf" "$_LOGS_"
	/bin/cp -rf "${_etc_path_}/vxagent/vcon" "$_LOGS_"
	/bin/cp -rf "${_etc_path_}/vxagent/logs" "$_LOGS_"
	#if [ -f "${_MOUNT_PATH_}/var/log/messages" ]; then
	#	COPY "${_MOUNT_PATH_}/var/log/messages" "$_LOGS_"
	#fi

	_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_="$_VX_FAILOVER_DATA_PATH_/$_PROTECTED_DISKS_LIST_FILE_NAME_"
	if [ -f "$_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_" ]; then
		COPY $_PROTECTED_DISK_MAPPING_LIST_FILE_PATH_ $_etc_path_/vxagent/logs/
	fi

	_VCON_ACTION_FILE_="$_etc_path_/vxagent/vcon/p2v_actions"
	if [ ! -d "$_etc_path_/vxagent/vcon/" ]; then
		mkdir -p "$_etc_path_/vxagent/vcon/"
	fi

	echo "workflow=${_WORK_FLOW_}" > $_VCON_ACTION_FILE_
	if [ 1 -eq ${_INSTALL_VMWARE_TOOLS_} ]; then
		echo "install_vmware_tools=1" >> $_VCON_ACTION_FILE_
	fi

	if [ 1 -eq ${_UNINSTALL_VMWARE_TOOLS_} ]; then
		echo "uninstall_vmware_tools=1" >> $_VCON_ACTION_FILE_	
	fi

	echo "New GUID = ${_NEW_GUID_}"

	if [ ! -z ${_NEW_GUID_} ]; then
		echo "new_guid=${_NEW_GUID_}" >> $_VCON_ACTION_FILE_
	fi
	
        echo "create_initrd_images=1" >> $_VCON_ACTION_FILE_

	#set VolumeFilteringDisabled to 1.
	setVolumeFilteringDisabled "$_etc_path_" "1"
	
	ClearBitMapFiles "$_etc_path_/.." "$_etc_path_"
	return $_returnCode_
}

function setVolumeFilteringDisabled
{
	_etc_path_=$1
	_set_volume_filtering_value_=$2
	_return_code_=$_SUCCESS_

	_involflt_path_="$_etc_path_/vxagent/involflt/"
	_dev_directories_=`ls $_involflt_path_`
	for _dev_directory_ in ${_dev_directories_}
	do
		_check_file_="$_involflt_path_/$_dev_directory_/VolumeFilteringDisabled"
		if [ -f "$_check_file_" ]; then
			echo 1 > "$_check_file_"
		fi
	done

	return $_return_code_
}

function ModifyLvmConfFile_V2P
{
	_lvm_conf_file_=$1
	_return_code_=$_SUCCESS_

	_lvm_conf_file_orig_="${_lvm_conf_file_}${_INMAGE_ORG_}"
	_lvm_conf_file_mod_="${_lvm_conf_file_}${_INMAGE_MOD_}"

	if [ ! -f "${_lvm_conf_file_orig_}" ]; then
		echo "Failed to find file ${_lvm_conf_file_orig_}."
		_return_code_=$_FAILED_
	else
		COPY "${_lvm_conf_file_}" "${_lvm_conf_file_mod_}"
		MOVE "${_lvm_conf_file_orig_}" "${_lvm_conf_file_}"
	fi

	return $_return_code_
}

function ModifyLvmConfFile
{
	_lvm_conf_file_=$1
	_return_code_=$_SUCCESS_

	_lvm_conf_file_orig_="${_lvm_conf_file_}${_INMAGE_ORG_}"

	if [ ! -f "${_lvm_conf_file_}" ]; then
		echo "lvm.conf file does not exist under /etc/lvm."
		return $_return_code_
	fi

	if [ -f "${_lvm_conf_file_orig_}" ]; then 
		MOVE "${_lvm_conf_file_orig_}" "${_lvm_conf_file_}"
	else
		COPY "${_lvm_conf_file_}" "${_lvm_conf_file_orig_}"
	fi
	
	COPY "${_lvm_conf_file_}" "$_LOGS_"
	#here we need to call Modify lvm.conf file to include all devices.
	_line_=`cat "$_lvm_conf_file_" | grep -P "^(\s*)filter(\s*)=(\s*).*"`
    echo $_line_
    
    if [ ! -z "$_line_" ]; then
        _line_number_=`cat "$_lvm_conf_file_" | grep -P -n "^(\s*)filter(\s*)=(\s*).*" | awk -F":" '{print $1}'`
        echo "Updating filter entry in $_lvm_conf_file_ file at line: $_line_number_"
        sed -i "$_line_number_ c\filter=\"a\/.*\/\"" $_lvm_conf_file_
    else
        echo "filter entry not found. Nothing to update in $_lvm_conf_file_ file"
    fi

	return $_return_code_
}

#Modified modprobe.conf file under /etc directory.
function  ModifyModprobeFile 
{
	_etc_path_=$1
	_mod_probe_file_="$_etc_path_/$_MOD_PROBE_FILE_NAME_"
	_return_code_=$_SUCCESS_

	#READ THE OS ENTRY FROM CONFIGURATION FILE.
	_DISTRIBUTION_DETAILS_=`grep "Distribution details                   : " $_INPUT_CONFIG_FILE_ | awk '{print $4}'`
	echo "Distribution details = ${_DISTRIBUTION_DETAILS_}"

	case "${_DISTRIBUTION_DETAILS_}" in
                RHEL4*)
			#NEED TO MODIFY MODPROBE FILE.
			echo "Performing modprobe changes for distribution ${_DISTRIBUTION_DETAILS_}."
			ModifyModprobeFile_RHEL4 "${_DISTRIBUTION_DETAILS_}" "$_mod_probe_file_"
			if [ 0 -ne $? ]; then
				_return_code_=$_FAILED_
			fi
			;;
		*)
			echo "Skipping modprobes changes for distribution ${_DISTRIBUTION_DETAILS_}."
			;;
	esac

	return $_return_code_
}

function ModifyModprobeFile_RHEL4 
{
	_os_=$1
	_mod_probe_file_=$2

	_modprobe_file_orig_="${_mod_probe_file_}${_INMAGE_ORG_}"
	_modprobe_file_mod_="${_mod_probe_file_}${_INMAGE_MOD_}"

	if [! -f "${_mod_probe_file_}" ]; then
		echo "modprobe.conf file does not exist. OS=${_os_}."
		return $_SUCCESS_
	fi
	
	if [ -f "${_modprobe_file_orig_}" ]; then
		MOVE "${_modprobe_file_orig_}" "$_mod_probe_file_"
	fi
	COPY "$_mod_probe_file_" "${_modprobe_file_orig_}"
	#alias eth0 e1000
	#alias scsi_hostadapter mptbase
	#alias scsi_hostadapter1 mptscsi
	#alias scsi_hostadapter2 mptfc
	#alias scsi_hostadapter3 mptspi
	#alias scsi_hostadapter4 mptsas
	#alias scsi_hostadapter5 mptscsih
	cat "${_modprobe_file_orig_}" | grep -i "scsi_hostadapter" | grep -v "involflt" | while read _line_
	do
        	echo $_line_
	        sed -i "/$_line_/d" "${_mod_probe_file_}"
	done

	cat "${_modprobe_file_orig_}" | grep -P -i "eth[0-9]+" | while read _line_
	do
        	echo $_line_
	        sed -i "/$_line_/d" "${_mod_probe_file_}"
	done

	echo "alias eth0 e1000
	alias scsi_hostadapter mptbase
	alias scsi_hostadapter1 mptscsi
	alias scsi_hostadapter2 mptfc
	alias scsi_hostadapter3 mptspi
	alias scsi_hostadapter4 mptsas
	alias scsi_hostadapter5 mptscsih" > "$_modprobe_file_mod_"

	cat "${_mod_probe_file_}" >> "$_modprobe_file_mod_"

	MOVE "$_modprobe_file_mod_" "${_mod_probe_file_}"
	return $_SUCCESS_
}

#This handles change requirements in lvm.conf file in initrd image.
#input : initrd file name.
#Output : Either SUCCESS or FAILED.
function CheckLvmConfFile
{
	_retCode_=$_SUCCESS_

	 #filter=[ "a|^/dev/cciss/c0d0$|", "a|^/dev/mapper/*|", "r/.*/" ]
        #need to split to get initrd file Name.
        initrd_image_absolutepath=$1
        initrd_image=`echo $initrd_image_absolutepath | sed -e "s:[/].*[/]::"`
        initrd_image=`echo $initrd_image | sed -e "s:/::"`
        out_initrd_image="$initrd_image.out"
        DATE_TIME_SUFFIX=`date +%d`_`date +%b`_`date +%Y`_`date +%H`_`date +%M`_`date +%S`
        initrd_temp_dir="/tmp/temp_$DATE_TIME_SUFFIX"
        boot_temp_dir=$initrd_temp_dir
        /bin/mkdir $initrd_temp_dir

        (cd $initrd_temp_dir ; gunzip -9 < $initrd_image_absolutepath > ../$initrd_image.unzipped ; cpio -id < ../$initrd_image.unzipped)
        cd $initrd_temp_dir
        _lvm_conf_file_="$initrd_temp_dir/etc/lvm/lvm.conf"
        if [ ! -f "$_lvm_conf_file_" ]; then
                #mean to say lvm.conf does not exist no need to change any thing...
                echo "initrd : $initrd_image, does not contain lvm.conf file."
        else
                cp -f "$_lvm_conf_file_" "$_lvm_conf_file_.old"
		cp -f "${initrd_temp_dir}/init" "${_LOGS_}"
                #now we have to make in place replacement in lvm.conf file.
                #First we need to get the Line and then
                #check if all block devices are allowed?
                #if not, then check whether sd pattern is already allowed.
                #if not, then add sd* pattern to list of allowed block devices.
                _line_=`cat "$_lvm_conf_file_" | grep -P "^(\s*)filter(\s*)=(\s*).*"`
                echo $_line_
                _line_number_=`cat "$_lvm_conf_file_" | grep -P -n "^(\s*)filter(\s*)=(\s*).*" | awk -F":" '{print $1}'`
                sed -i "$_line_number_ c\filter=\"a\/.*\/\"" $_lvm_conf_file_

		cp -rf "etc/" "${_LOGS_}"
                #now again we have zip the file and then replace image file.
                (cd $initrd_temp_dir/ ; find . | cpio --create --format='newc' > $boot_temp_dir/../$initrd_image.unzipped_$DATE_TIME_SUFFIX)
                (cd $initrd_temp_dir/ ; gzip -9 < $boot_temp_dir/../$initrd_image.unzipped_$DATE_TIME_SUFFIX > $boot_temp_dir/../$out_initrd_image)
                mv -f "$boot_temp_dir/../$out_initrd_image" "$initrd_image_absolutepath"
                rm -f "$boot_temp_dir/../$initrd_image.unzipped_$DATE_TIME_SUFFIX"
        fi

        rm -f "/$boot_temp_dir/../$initrd_image.unzipped"
        rm -rf "$initrd_temp_dir"
	

	return $_retCode_
}

#This handles all requiments to make a modification in initrd file..
#input : either it can be initrd file or virtual initrd file.
#return SUCCESS or FAILURE.
function ModifyVirtualInitrdFile
{
	_retCode_=$_SUCCESS_
	_initrd_filename_=$1
	echo "Checking \"$_initrd_filename_\" for modifications if required any."

	CheckLvmConfFile $_initrd_filename_
	if [ 0 -ne $? ]; then
		echo "ERROR :: Failed to modify lvm.conf file in initrd image : $_initrd_filename_."
		_retCode_=$_FAILED_
	fi

	return $_retCode_
}


#THIS FUNCTION RENAMES INITRD OR INITRAMFS FILES.
function Rename_INITRD_RAMFS_Files
{
	#FIRST LIST ALL PATTERNS WHICH ARE IN THE FORM OF "-InMage-Virtual"
	_boot_path_=$1
	_virtual_initrd_ramfs_files_=`find $_boot_path_ -name "*-InMage-Virtual"`
	echo "initrd files = $_virtual_initrd_ramfs_files_"
	for _virtual_initrd_ramfs_file_ in ${_virtual_initrd_ramfs_files_}
	do
		#here we need to check if we have to modify lvm.conf file in virtual Initrd image.
		ModifyVirtualInitrdFile $_virtual_initrd_ramfs_file_		
		if [ 0 -ne $? ]; then 
			echo "Error :: Failed to modify virtual initrd image file for any changes."
		fi

		_initrd_ramfs_file_=`echo $_virtual_initrd_ramfs_file_ | sed "s/-InMage-Virtual$//g"`
		echo "Initrd file = $_initrd_ramfs_file_"
		#TAKE COPY OF THIS FILE
		#CHECK IF THE ORIG FILE ALREADY EXISTS?
		if [ -f "${_initrd_ramfs_file_}${_INMAGE_ORG_}" ]; then
			MOVE "${_initrd_ramfs_file_}${_INMAGE_ORG_}" "$_initrd_ramfs_file_"
		fi

		COPY "$_initrd_ramfs_file_" "${_initrd_ramfs_file_}${_INMAGE_ORG_}"
		MOVE "$_virtual_initrd_ramfs_file_" "$_initrd_ramfs_file_"
	done
}

function ModifyGrubV2Config
{
	_grub_path_="$1/grub2/"
	_menu_list_file_path_="$_grub_path_/grub.cfg"
    if [ ! -f "$_menu_list_file_path_" ]
    then
	    _grub_path_="$1/grub/"
	    _menu_list_file_path_="$_grub_path_/grub.cfg"
    fi 

	if [ ! -f "$_menu_list_file_path_" ]; then
		echo "WARNING :: File \"grub.cfg\" not found in \"$_grub_path_\"."
		return $_SUCCESS_
	fi
	
	#MAKE COPY OF IT.
	_menu_list_file_orig_="${_menu_list_file_path_}${_INMAGE_ORG_}"
	if [ -f "${_menu_list_file_orig_}" ]; then
		MOVE "${_menu_list_file_orig_}" "$_menu_list_file_path_"
	fi

    echo "Grub config -> $_menu_list_file_path_"
	COPY "$_menu_list_file_path_" "${_menu_list_file_orig_}"
	COPY "$_menu_list_file_path_" "$_LOGS_"
		
	#MODIFY ROOT ENTRY WITH UUID VALUE
	exec 4<"$_menu_list_file_path_"
	while read -u 4 -r _line_
	do 
		_ret_val_=`echo $_line_ | grep "^#"`
		if [ "${_ret_val_}" ]; then
			#echo $_line_
			continue
		fi

		_root_dev_name_=`echo $_line_ |  grep 'linux.*root[ ]*=[ ]*\/dev\/' |\
                         awk -F"root[ ]*=[ ]*" '{print $2}' | awk '{print $1}'`
		if [ "${_root_dev_name_}" ]; then
			# IF ROOT PARTITION IS GETTING IDENTIFIED UUID OR LABEL? 
            # IF SO LEAVE IT AS IT IS DO NOT CHANGE.
			echo $_root_dev_name_ | egrep "^UUID|^LABEL" > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				echo "Existing Root Dev: $_root_dev_name"
				continue
			fi
			# CHECK IF IT IS A DEVICE
			echo $_root_dev_name_ | grep "\/dev\/" > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				#NOW IT IS A DEVICE : FIND ITS CORRESPONDING UUID.
				Find_UUID "$_root_dev_name_" 
				if [ 0 -eq $? ]; then
					_uuid_=$_UUID_ 
					safe_pattern=$(printf "%s\n" "$_root_dev_name_" |\
                                 sed 's/[][\.*^$(){}?+|/]/\\&/g')
                    echo "New Root Dev: $_root_dev_name -> $_UUID_"
					sed -i "s/${safe_pattern}/UUID=$_uuid_/" ${_menu_list_file_path_}
				fi
			fi
		fi

		#MODIFY RESUME AS WELL
		_resume_dev_name_=`echo $_line_ |  grep 'linux.*resume[ ]*=[ ]*\/dev\/' |\
                           awk -F"resume[ ]*=[ ]*" '{print $2}' | awk '{print $1}'`
		if [ "${_resume_dev_name_}" ]; then
			#IF RESUME PARTITION IS GETTING IDENTIFIED UUID OR LABEL? 
            #IF SO LEAVE IT AS IT IS DO NOT CHANGE.
			echo $_resume_dev_name_ | egrep "^UUID|^LABEL" > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				echo "Existing resume dev: $_resume_dev_name"
				continue
			fi
			# CHECK IF IT IS A DEVICE
			echo $_resume_dev_name_ | grep "\/dev\/" > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				#NOW IT IS A DEVICE : FIND ITS CORRESPONDING UUID.
				Find_UUID "$_resume_dev_name_" 
				if [ 0 -eq $? ]; then
					_uuid_=$_UUID_ 
					safe_pattern=$(printf "%s\n" "$_resume_dev_name_" |\
                                 sed 's/[][\.*^$(){}?+|/]/\\&/g')
					sed -i "s/${safe_pattern}/UUID=$_uuid_/" ${_menu_list_file_path_}
                    echo "New Root Dev: $_resume_dev_name -> $_UUID_"
				else
					MapSrcToTgt "${_resume_dev_name_}"
					if [ $? -eq $_FAILED_ ]; then
						echo "ERROR :: Failed to modify resume device."
						continue
					fi
					safe_pattern=$(printf "%s\n" "$_resume_dev_name_" |\
                                 sed 's/[][\.*^$(){}?+|/]/\\&/g')
					echo "resume device on target side = ${_SRC2TGT_MAPPED_DEVICE_}."
					safe_pattern1=$(printf "%s\n" "${_SRC2TGT_MAPPED_DEVICE_}" |\
                                  sed 's/[][\.*^$(){}?+|/]/\\&/g')
					sed -i "s/${safe_pattern}/${safe_pattern1}/" ${_menu_list_file_path_}
				fi
			fi
		fi
                #Removing console Option
                _console_=`echo $_line_ | awk -F"console=" '{print $2}'|awk '{print $1}'`
                if [ ! -z "$_console_" ];then
                    echo "console parameter : $_console_"
                    _pattern_to_be_removed="console=$_console_"
                    echo "Pattern to be removed : $_pattern_to_be_removed"
                    sed -i "s/$_pattern_to_be_removed//" ${_menu_list_file_path_}
                fi
	done
	4<&-
	return $_SUCCESS_
}
#THIS FUNCTION MAKES SURE THAT ROOT ENTRY IN menu.lst FILE IS REPLACED WITH IT"S UUID
function ModifyMenuListFile
{
	_grub_path_="$1/grub/"
	_menu_list_file_path_="$_grub_path_/menu.lst"
	_menu_list_file_path_=`readlink -f $_grub_path_/menu.lst`
	#FIRST MAKE SURE THAT FILE EXISTS.
	if [ ! -f "$_menu_list_file_path_" ]; then
		echo "WARNING :: File \"menu.lst\" not found in path \"$_grub_path_\"."
		echo "Nothing to modify \"menu.lst\" file."
		#ModifyGrubV2Config "$1"
		return $_SUCCESS_
	fi
	
	#MAKE COPY OF IT.
	_menu_list_file_orig_="${_menu_list_file_path_}${_INMAGE_ORG_}"
	if [ -f "${_menu_list_file_orig_}" ]; then
		MOVE "${_menu_list_file_orig_}" "$_menu_list_file_path_"
	fi

	COPY "$_menu_list_file_path_" "${_menu_list_file_orig_}"
		
	COPY "$_menu_list_file_path_" "$_LOGS_"
		
	#WE HAVE FILE.MODIFY ROOT ENTRY WITH IT"S UUID VALUE
	exec 4<"$_menu_list_file_path_"
	while read -u 4 -r _line_
	do 
		_ret_val_=`echo $_line_ | grep "^#"`
		if [ "${_ret_val_}" ]; then
			#echo $_line_
			continue
		fi

		_root_dev_name_=`echo $_line_ |  grep 'kernel.*root[ ]*=[ ]*\/dev\/' | awk -F"root[ ]*=[ ]*" '{print $2}' | awk '{print $1}'`
		if [ "${_root_dev_name_}" ]; then
			#IF ROOT PARTITION IS GETTING IDENTIFIED UUID OR LABEL? IS SO LEAVE IT AS IT IS DO NOT CHANGE.
			echo $_root_dev_name_ | egrep "^UUID|^LABEL" > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				echo " Appropriate UUID or LABEL entry exists in $_menu_list_file_path_ : $_root_dev_name"
				continue
			fi
			# CHECK IF IT IS A DEVICE
			echo $_root_dev_name_ | grep "\/dev\/" > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				#NOW IT IS A DEVICE : FIND ITS CORRESPONDING UUID.
				Find_UUID "$_root_dev_name_" 
				if [ 0 -eq $? ]; then
					_uuid_=$_UUID_ 
					safe_pattern=$(printf "%s\n" "$_root_dev_name_" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
					sed -i "s/${safe_pattern}/UUID=$_uuid_/" ${_menu_list_file_path_}
				fi
			fi
		fi

		#MODIFY RESUME AS WELL
		_resume_dev_name_=`echo $_line_ |  grep 'kernel.*resume[ ]*=[ ]*\/dev\/' | awk -F"resume[ ]*=[ ]*" '{print $2}' | awk '{print $1}'`
		if [ "${_resume_dev_name_}" ]; then
			#IF RESUME PARTITION IS GETTING IDENTIFIED UUID OR LABEL? IS SO LEAVE IT AS IT IS DO NOT CHANGE.
			echo $_resume_dev_name_ | egrep "^UUID|^LABEL" > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				echo " Appropriate UUID or LABEL entry exists in $_menu_list_file_path_ : $_resume_dev_name"
				continue
			fi
			# CHECK IF IT IS A DEVICE
			echo $_resume_dev_name_ | grep "\/dev\/" > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				#NOW IT IS A DEVICE : FIND ITS CORRESPONDING UUID.
				Find_UUID "$_resume_dev_name_" 
				if [ 0 -eq $? ]; then
					_uuid_=$_UUID_ 
					safe_pattern=$(printf "%s\n" "$_resume_dev_name_" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
					sed -i "s/${safe_pattern}/UUID=$_uuid_/" ${_menu_list_file_path_}
				else
					MapSrcToTgt "${_resume_dev_name_}"
					if [ $? -eq $_FAILED_ ]; then
						echo "ERROR :: Failed to modify resume device."
						continue
					fi
					safe_pattern=$(printf "%s\n" "$_resume_dev_name_" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
					echo "resume device on target side = ${_SRC2TGT_MAPPED_DEVICE_}."
					safe_pattern1=$(printf "%s\n" "${_SRC2TGT_MAPPED_DEVICE_}" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
					sed -i "s/${safe_pattern}/${safe_pattern1}/" ${_menu_list_file_path_}
				fi
			fi
		fi
                #Removing console Option
                _console_=`echo $_line_ | awk -F"console=" '{print $2}'|awk '{print $1}'`
                if [ ! -z "$_console_" ];then
                    echo "console parameter : $_console_"
                    _pattern_to_be_removed="console=$_console_"
                    echo "Pattern to be removed : $_pattern_to_be_removed"
                    sed -i "s/$_pattern_to_be_removed//" ${_menu_list_file_path_}
                fi
	done
	4<&-
	return $_SUCCESS_
}

#THIS FUNCTION ENSURE THAT IT PLACES ONLY PROTECTED DISK ENTRIES IN DEVICE.MAP
function ModifyDeviceMapFile
{
	#WHILE MODIFYING MAKE SURE THAT WE HAVE READ THE VALUE AND CORRECT AND VERIFY IN PROTECTED DISK LIST.
	#IT MIGHT NOT BE DISK NAME?? IT MIGHT CONTAIN BY-PATH, BY-ID, BY-UUID, BY-LABEL?
	_grub_path_="$1/grub";
	_device_map_file_="$_grub_path_/device.map"
	_device_map_file_orig_="${_device_map_file_}${_INMAGE_ORG_}"
	
	#CHECK IF DEVICE.MAP FILE EXISTS.
	if [ ! -f "$_device_map_file_" ]; then
		echo "WARNING :: \"device.map\" file not found in path \"$_grub_path_\"."
		echo "Nothing to modify \"device.map\" file."
		return $_SUCCESS_
	fi

	#WE HAVE TO TAKE COPY OF ORIGINAL
	if [ -f "${_device_map_file_orig_}" ]; then
		MOVE "${_device_map_file_orig_}" "$_device_map_file_"
	fi
	COPY "$_device_map_file_" "${_device_map_file_orig_}"

	#WE HAVE FILE.
	_ret_code_=$_SUCCESS_
	exec 4<"$_device_map_file_"
	while read -u 4 -r _line_
	do
		_hard_disk_info_=`echo $_line_ | grep "/dev/*" | awk -F"^[ ]*(.*)[ ]+" '{print $2}'`
		echo "Hard Disk Info = $_hard_disk_info_"
		if [ "${_hard_disk_info_}" ]; then
			_device_symlink_=""
			#CHECK IF DISK INFOR  CONTAINS /dev/disk
			echo ${_hard_disk_info_} | grep "\/dev\/disk\/" > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				#NOW THIS HAS TO BE DE-REFERENCED TO BASE DEVICE
				Deref_SymLink "${_hard_disk_info_}"	
				if [ 0 -ne $? ] || [ -z $_real_partition_name_ ]; then 
					echo "ERROR :: Failed to Find real device name for symlink=${_hard_disk_info_}."
					continue
				fi
				echo "Found real device name for symlink ${_hard_disk_info_} as ${_real_partition_name_}"
				_device_symlink_=$_hard_disk_info_
				_hard_disk_info_=$_real_partition_name_
			fi
				
			#CHECK IF THE DISK IS PROTECTED? IF PROTECTED GET IT"S TARGET DRIVE NAME AND REPLACE. IF NOT REMOVE LINE.
			GetProtectedDeviceEntry "${_hard_disk_info_}"
			_dev_num_=$?
			if [ 0 -ne $_dev_num_ ]; then
				echo "Device $_hard_disk_info_ is protected and device number at target side = $_dev_num_"
				#FIND ITS DISK NAME ON TARGET SIDE.
				_dev_num_=$((_dev_num_-1))
				GetTargetDiskName $_dev_num_ 
				if [ 0 -ne $?  ]; then
					echo "ERROR :: Failed to find target disk name for device \"$_hard_disk_info_\"."	
					_ret_code_=$_FAILED_
					continue
				fi
				echo "Disk Name = $_disk_name_"
				
				#SAFE LHS PATTERN AND INPUT PATTERN
				_safe_input_=$(printf "%s\n" "$_hard_disk_info_" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
				if [ "${_device_symlink_}" ]; then
					_safe_input_=$(printf "%s\n" "$_device_symlink_" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
				fi
				_safe_sub_input_=$(printf "%s\n" "$_disk_name_" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
				echo "Input = $_safe_input_ , sub = $_safe_sub_input_"
				echo $_line_ | sed "s/${_safe_input_}/${_safe_sub_input_}/" >>"${_device_map_file_}.tmp"
			fi
		fi
	done
	4<&-
	MOVE "${_device_map_file_}.tmp" "${_device_map_file_}"
	return $_SUCCESS_
}

#THIS FUNCTION CREATES A BACKUP OF CONFIGURATION FILE FOR GRUB INSTALLATION ON CCISS MACHINE H/W
function BackupConfigFile
{
	_boot_path_=$1
	_install_grub_flag=0
	echo "boot path = $_boot_path_"
	BACKUP_INPUT_CONFIG_FILE_=""

	_INPUT_CONFIG_FILE_="$_boot_path_/Configuration.info"
	#echo "Machine Manufacturer                   : $machine_manufacturer"
	#echo "Machine Product Type                   : $machine_product_type"
	#echo "Distribution details                   : $OS"
	_MACHINE_MANUFACTURER_=`grep "Machine Manufacturer                   : " $_INPUT_CONFIG_FILE_ | awk '{print $4}'`
	_MACHINE_PRODUCT_TYPE_=`cat $_INPUT_CONFIG_FILE_ | grep "Machine Product Type" | awk -F": " '{print $2}'`
	_DISTRIBUTION_DETAILS_=`grep "Distribution details                   : " $_INPUT_CONFIG_FILE_ | awk '{print $4}'`
	_ROOT_DEVICE_=`grep "Root device                            : " $_INPUT_CONFIG_FILE_ | awk '{print $4}'`
	_BOOT_DEVICE_=`grep "Boot device                            : " $_INPUT_CONFIG_FILE_ | awk '{print $4}'`
	_PROBE_DEVICE=""
	case "${_DISTRIBUTION_DETAILS_}" in 
		SLES10*|SLES9*)
			_first_partition_=""
			#if [ "${_MACHINE_MANUFACTURER_}" = "HP" ] || [ "${_MACHINE_MANUFACTURER_}" = "Hewlett-Packard" ]; then
			#	if [ "${_MACHINE_PRODUCT_TYPE_}" = "ProLiant DL380 G4" ]; then
			#		_install_grub_flag=1
			#	fi
			#fi
                        # Controller type is CCISS
                        # Check during GRUB installation - if first partition is not active
                        # then only install GRUB
                        if [ -z "${_BOOT_DEVICE_}" ]; then
                        	_PROBE_DEVICE="${_ROOT_DEVICE_}"
                        else
                        	_PROBE_DEVICE="${_BOOT_DEVICE_}"
                        fi
                       	echo "${_PROBE_DEVICE}" | grep -i cciss 2>&1 > /dev/null
                       	if [ $? -eq 0 ]; then
				_install_grub_flag=1
                       	fi
		;;
		SLES11*|SLES12*)
			echo "No changes required on $_DISTRIBUTION_DETAILS_ for GRUB installtion for now"
		;;
		RHEL*)
			echo "No changes required on $_DISTRIBUTION_DETAILS_ for GRUB installtion"
		;;
		OL*)
			echo "No changes required on $_DISTRIBUTION_DETAILS_ for GRUB installtion"
		;;
		UBUNTU*)
			echo "No changes required on $_DISTRIBUTION_DETAILS_ for GRUB installtion"
		;;
		*) echo "Invalid distribution details $_DISTRIBUTION_DETAILS_";;
	esac
	if [ $_install_grub_flag -eq 1 ]; then
		BACKUP_INPUT_CONFIG_FILE_="$_VX_FAILOVER_DATA_PATH_/${_MACHINE_UUID_}_Configuration.info"
		COPY $_INPUT_CONFIG_FILE_ $BACKUP_INPUT_CONFIG_FILE_
	fi
}
#THIS FUNCTION MAKES SURE THAT ALL CHANGES ARE PERFORMED IN /boot DIRECTORY OF TARGET MACHINE TO BOOT.
function Prepare_boot
{
	_boot_path_=$1
	_ret_code_=$_SUCCESS_
	echo "boot path = $_boot_path_"

	_INPUT_CONFIG_FILE_="$_boot_path_/Configuration.info"
	#TODO
	#THIS HAS TO CHECK FOR INPUT CONFIGURATION FILE AND PROTECTED DISK LIST FILE.
	
    # Log Configuration.info for debugging purpose.
    if [ -f $_INPUT_CONFIG_FILE_ ]; then
        echo "Logging Configuration.info file for debugging purpose."
        cat $_INPUT_CONFIG_FILE_
    fi

	COPY $_INPUT_CONFIG_FILE_ $_LOGS_
	#FIRST WE NEED TO RENAME INITRD/INITRAMFS FILES.
	Rename_INITRD_RAMFS_Files "$_boot_path_"
	_ret_code_=$((_ret_code_+$?))
	
	#NOW WE NEED TO MODIFY MENU.LST FILE.
	ModifyMenuListFile "$_boot_path_"
	_ret_code_=$((_ret_code_+$?))

	#NOW WE NEED TO MODIFY DEVICE.MAP FILE.
	ModifyDeviceMapFile "$_boot_path_"
	_ret_code_=$((_ret_code_+$?))

	return $_ret_code_
}

function ExecuteTask_V2P
{
	#WE HAVE TO RENAME FILES WHICH ARE EITHER UNDER /boot or /etc.
	#under '/boot'
		#1. Rename back initrd (or) initramfs files.
		#2. menu.lst file.
		#3. device.map
	#under '/etc'
		#1. /etc/sysconfig/kernel
		#2. fstab.
	
	if [ ! "${_MOUNT_PATH_}" ]; then
		echo "ERROR :: Mount point is not specified."
		echo "ERROR :: Mount point has to be specfied in order to modify files."
		return $_FAILED_
	fi

	#NEED TO CHECK IF MOUNT PATH SPECIFIED IS A BLOCK DEVICE
	#if [ ! -b "$_MOUNT_PATH_" ]; then
	#       echo "ERROR :: Invalid mount point. Mount point : $_MOUNT_PATH_."
	#       echo "ERROR :: Please specify a valid mount point which is of a block device type."
	#       return $_FAILED_
	#fi

	#NOW WE HAVE A VALID MOUNT POINT. NOW WE HAVE TO CHECK FOR WHICH THE MOUNT POINT IS?
	_ret_code_boot_=$_FAILED_
	_ret_code_etc_=$_FAILED_
	if [ "boot" = "${_MOUNT_PATH_FOR_}" ] || [ "all" = "${_MOUNT_PATH_FOR_}" ]; then
		if [ "boot" = "${_MOUNT_PATH_FOR_}"  ]; then
			Prepare_boot_V2P "$_MOUNT_PATH_"
			_ret_code_boot_=$?
		elif [ -d "$_MOUNT_PATH_/boot" ]; then
			Prepare_boot_V2P "$_MOUNT_PATH_/boot/"
			_ret_code_boot_=$?
		else
			echo "ERROR :: Mounted partition(${_MOUNT_PATH_}) does not contain /boot information.Failed to replace original files."
		fi
	fi

	if [ "etc" = "${_MOUNT_PATH_FOR_}" ] || [ "all" = "${_MOUNT_PATH_FOR_}" ]; then
		if [ -d "$_MOUNT_PATH_/etc" ]; then
			Prepare_etc_V2P "$_MOUNT_PATH_/etc"
			_ret_code_etc_=$?
		elif [ -f "$_MOUNT_PATH_/fstab" ]; then
			Prepare_etc_V2P "$_MOUNT_PATH_"
			_ret_code_etc_=$?
			else
		    echo "ERROR :: Mounted partition(${_MOUNT_PATH_}) does not contain /etc information.Failed to replace original files."
		fi
	fi
	if [ $_ret_code_boot_ ] || [ $_ret_code_etc_ ]; then
		return $_FAILED_
	fi
	
	echo "Successfully reverted original configuration files."
	return $_SUCCESS_
}

function Prepare_etc_V2P
{
	_etc_path_="$1"
	_returnCode_=$_SUCCESS_

	_INPUT_CONFIG_FILE_="$_etc_path_/Configuration.info"

	ModifyInitrdModulesList_V2P "$_etc_path_/sysconfig"
	if [ 0 -ne $? ]; then
		_returnCode_=$_FAILED_
	fi

	if [ ! -f "$_etc_path_/fstab" ]; then
		echo "ERROR :: Failed to find file \"fstab\" file under path \"$_etc_path_\"."
		echo "ERROR :: Failed to modify fstab file."
		return $_FAILED_
	fi

	ModifyFstabFile_V2P "$_etc_path_/fstab"
	if [ 0 -ne $? ]; then
		_returnCode_=$_FAILED_
	fi

	ModifyModProbeFile_V2P "$_etc_path_"
	if [ 0 -ne $? ]; then
		_returnCode_=$_FAILED_
	fi

	_lvm_conf_file_="${_etc_path_}/lvm/lvm.conf"
	ModifyLvmConfFile_V2P "$_lvm_conf_file_"
	if [ 0 -ne $? ]; then
		_returnCode_=$_FAILED_
	fi

	_VCON_ACTION_FILE_="$_etc_path_/vxagent/vcon/p2v_actions"
	if [ ! -d "$_etc_path_/vxagent/vcon/" ]; then
		mkdir -p "$_etc_path_/vxagent/vcon/"
	fi
	echo "workflow=${_WORK_FLOW_}" > $_VCON_ACTION_FILE_
	echo "install_vmware_tools=0" >> $_VCON_ACTION_FILE_
	echo "uninstall_vmware_tools=0" >> $_VCON_ACTION_FILE_
        echo "create_initrd_images=1" >> $_VCON_ACTION_FILE_
	
	#If OS=OL and Kernel = UEK and bitness = 32 then configure uninstall_vmware_tools to 1.
	_DISTRIBUTION_DETAILS_=`grep "Distribution details                   : " $_INPUT_CONFIG_FILE_ | awk '{print $4}'`
	_KERNEL_DETAILS_=`grep "Operating System Kernel Release        : " $_INPUT_CONFIG_FILE_ | awk '{print $6}'`
	
	echo "Distribution Details = $_DISTRIBUTION_DETAILS_"
	echo "Kernel = $_KERNEL_DETAILS_"
	case "${_DISTRIBUTION_DETAILS_}" in
		OL*)
			echo $_DISTRIBUTION_DETAILS_ | grep -P "5\-32$" > /dev/null 2>&1
			if [ 0 -eq $? ]; then
				_is_32_bit_=1
			fi
			echo $_KERNEL_DETAILS_ | grep -P "uek$" > /dev/null 2>&1 
			if [ 0 -eq $? ]; then
				_is_uek_=1
			fi
			
			echo "OEL : is_32_bit = ${_is_32_bit_}"
			echo "is_uek = ${_is_uek_}"

			if [ $_is_32_bit_ && $_is_uek_ ]; then
				echo "setting flag to uninstall VMware tools."
				echo "uninstall_vmware_tools=1" >> $_VCON_ACTION_FILE_
			fi
			;;
		*) 
			echo "skipping uninstall of vmware tools for distribution $_DISTRIBUTION_DETAILS_"
                	;;
	esac 
	#set VolumeFilteringDisabled to 1.
	setVolumeFilteringDisabled "$_etc_path_" "1"

	ClearBitMapFiles "$_etc_path_/.." "$_etc_path_"

	return $_returnCode_
}

function ClearBitMapFiles
{	
	_root_path_="$1"
    _etc_path_="$2"

    _bitmap_files_=`ls ${_root_path_}/root/InMage-*.VolumeLog ${_etc_path_}/involflt/*/InMage-*.VolumeLog`
        for _bitmap_file_ in ${_bitmap_files_}
        do
                if [ -f "${_bitmap_file_}" ]; then
			echo "Removing file : ${_bitmap_file_}."
                        rm -f "${_bitmap_file_}"
                fi
        done
}

function ModifyModProbeFile_V2P
{
	_etc_path_=$1
	_mod_probe_file_="$_etc_path_/$_MOD_PROBE_FILE_NAME_"
        _modprobe_file_orig_="${_mod_probe_file_}${_INMAGE_ORG_}"
        _modprobe_file_mod_="${_mod_probe_file_}${_INMAGE_MOD_}"

        if [ ! -f "${_modprobe_file_orig_}" ]; then
                echo "ERROR :: Failed to find file \"${_modprobe_file_orig_}\". Failed to change it with original file."
        else
                COPY "${_mod_probe_file_}" "${_modprobe_file_mod_}"

                MOVE "${_modprobe_file_orig_}" "${_mod_probe_file_}"

                echo "Successfully changed modprobe file with its original file."
        fi

        return $_SUCCESS_
}

function ModifyFstabFile_V2P
{
	_fstab_file_=$1
	_fstab_file_orig_="${_fstab_file_}${_INMAGE_ORG_}"
	_fstab_file_mod_="${_fstab_file_}${_INMAGE_MOD_}"

	if [ ! -f "${_fstab_file_orig_}" ]; then
		echo "ERROR :: Failed to find file \"${_fstab_file_orig_}\". Failed to change it with original file."
	else
		COPY "${_fstab_file_}" "${_fstab_file_mod_}"
		
		MOVE "${_fstab_file_orig_}" "${_fstab_file_}"
		
		echo "Successfully changed fstab file with its original file."
	fi
	
	return $_SUCCESS_
}

function ModifyInitrdModulesList_V2P
{
	_kernel_base_file_path_=$1
    	_kernel_config_file_="$_kernel_base_file_path_/kernel"
	_kernel_config_file_orig_="${_kernel_config_file_}${_INMAGE_ORG_}"
	_kernel_config_file_mod_="${_kernel_config_file_}${_INMAGE_MOD_}"
		
	if [ ! -f "${_kernel_config_file_orig_}" ]; then
		echo "ERROR :: Failed to find file \"${_kernel_config_file_orig_}\".Failed to change it with original file."
		return $_FAILED_
	fi
	
	#COPY
	COPY "${_kernel_config_file_}" "${_kernel_config_file_mod_}"
	
	MOVE "${_kernel_config_file_orig_}" "${_kernel_config_file_}"

	echo "Successfully modified initrd module list with original list."
	return $_SUCCESS_
}

function Prepare_boot_V2P
{
	#WE HAVE TO RENAME FILES WHICH ARE EITHER UNDER /boot or /etc.
	#under '/boot'
		#1. Rename back initrd (or) initramfs files.
		#2. menu.lst file.
		#3. device.map

	_boot_path_=$1
	_ret_code_=$_SUCCESS_
	echo "boot path = $_boot_path_"

	_INPUT_CONFIG_FILE_="$_boot_path_/Configuration.info"

	#FIRST WE NEED TO RENAME INITRD/INITRAMFS FILES.
	Rename_INITRD_RAMFS_Files_V2P "$_boot_path_"
	_ret_code_=$((_ret_code_+$?))

	#NOW WE NEED TO REPLACE menu.lst FILE.
	ModifyMenuListFile_V2P "$_boot_path_"
	_ret_code_=$((_ret_code_+$?))

	#NOW WE NEED TO REPLACE DEVICE.MAP FILE.
	ModifyDeviceMapFile_V2P "$_boot_path_"
	_ret_code_=$((_ret_code_+$?))

	ChangeActivePartition "$_boot_path_"
	_ret_code_=$((_ret_code_+$?))
	
	return $_ret_code_
}

function ChangeActivePartition 
{
	_boot_path_=$1
	
	#CHECK IF FILE EXITS?
	_active_partition_file_="${_boot_path_}/${_SOURCE_CONFIGURATION_DATA_FILE}"
	echo "Active partiton file = ${_active_partition_file_}."
	if [ ! -f "${_active_partition_file_}" ]; then
		echo "File ${_active_partition_file_} does not exist. No need to change active partition."
		return $_SUCCESS_
	fi

	_installed_grub_on_target=`grep installed_grub_on_target $_active_partition_file_ | awk -F= '{ print $2 }'`
	_active_partition_number_=`grep orginal_active_partition $_active_partition_file_ | awk -F= '{ print $2 }'`
	if [ -z "${_active_partition_number_}" ]; then
		echo "ERROR :: Failed to find the active partition from $_active_partition_file_"
		echo "ERROR :: Failed to change active partition."
		return $_FAILED_ 
	fi
    	if [ $_installed_grub_on_target -eq 1 ]; then
        	echo "GRUB was installed on target virtual machine."
	else
        	echo "GRUB was not installed on target virtual machine."
	fi
	echo "Original active partition number = ${_active_partition_number_}"
	#find base device for mount_path.
	_mount_device_=`mount | grep " on ${_MOUNT_PATH_} " | awk '{print $1}'`
	echo "Mount device for mount path ${_MOUNT_PATH_} is ${_mount_device_}."
	#find base device from it.
	_base_device_="${_mount_device_}"
	_chars_in_mount_device=`echo "${_mount_device_}" | wc -c`
	
	while [ 0 -ne $_chars_in_mount_device ]
	do
		_base_device_=`echo "${_base_device_}" | sed -e "s/[0-9|a-z|A-Z|\~\!\@\#\$\%\^\&\*\(\)\_\+\-]$//g"`

		if [ -z "${_base_device_}" ]; then
			break;
		else
			if [ -b ${_base_device_} ]; then
				break;
			fi
		fi
		_chars_in_mount_device=$((_chars_in_mount_device-1))
	done
	echo "base device for ${_mount_device_} is ${_base_device_}."
	if [ -z "${_base_device_}" ]; then
		echo "ERROR :: Failed to find the base deivce"
		return $_FAILED_
	fi
	MakePartitionActive "${_base_device_}" "${_active_partition_number_}"
	if [ $_FAILED_ -eq $? ]; then
		return $_FAILED_
	fi
	
	echo "Successfully changed active partiton." 
	return $_SUCCESS_
}

function Rename_INITRD_RAMFS_Files_V2P
{
	 #FIRST LIST ALL PATTERNS WHICH ARE IN THE FORM OF ".INMAGE_ORG"
        _boot_path_=$1
        _orig_initrd_ramfs_files_=`find $_boot_path_ -name "init*${_INMAGE_ORG_}"`

        echo "initrd files = $_orig_initrd_ramfs_files_"
        for _orig_initrd_ramfs_file_ in ${_orig_initrd_ramfs_files_}
        do
		#WE HAVE TO USE SAFE PATTERN?PROBABALY WE HAVE TO
		if [ ! -f "${_orig_initrd_ramfs_file_}" ]; then
			echo "ERROR :: Failed to find file \"${_orig_initrd_ramfs_file_}\". Failed to change it back to original file."
			continue
		fi	
		safe_pattern=$(printf "%s\n" "${_INMAGE_ORG_}" | sed 's/[][\.*^$(){}?+|/]/\\&/g')
                _initrd_ramfs_file_=`echo $_orig_initrd_ramfs_file_ | sed "s/${safe_pattern}$//g"`
                echo "Initrd file = $_initrd_ramfs_file_"
                #TAKE COPY OF THIS FILE
		COPY "$_initrd_ramfs_file_" "${_initrd_ramfs_file_}${_INMAGE_MOD_}" 
                MOVE "$_orig_initrd_ramfs_file_" "$_initrd_ramfs_file_"
        done
	
	return $_SUCCESS_
}

function ModifyMenuListFile_V2P
{
	#first we need to back it up.
	_boot_path_=$1
	_menu_list_file_="${_boot_path_}/grub/menu.lst"
	_menu_list_file_orig_="${_menu_list_file_}${_INMAGE_ORG_}"
	_menu_list_file_mod_="${_menu_list_file_}${_INMAGE_MOD_}"

	#CHECK IF WE HAVE ORG FILE?
	if [ ! -f "${_menu_list_file_orig_}" ]; then
		echo "ERROR :: Unable to find file \"${_menu_list_file_orig_}\"."
		return $_FAILED_
	fi

	#WE HAVE FILE.
	COPY "${_menu_list_file_}" "${_menu_list_file_mod_}"

	#REVERT ORIG FILE.
	MOVE "${_menu_list_file_orig_}" "${_menu_list_file_}"

	echo "Successfully reverted menu.lst configuration."
	return $_SUCCESS_	
}

function ModifyDeviceMapFile_V2P
{
	_boot_path_=$1
        _device_map_file_="${_boot_path_}/grub/device.map"
        _device_map_file_orig_="${_device_map_file_}${_INMAGE_ORG_}"
        _device_map_file_mod_="${_device_map_file_}${_INMAGE_MOD_}"

        #CHECK IF WE HAVE ORG FILE?
        if [ ! -f "${_device_map_file_orig_}" ]; then
                echo "ERROR :: Unable to find file \"${_device_map_file_orig_}\"."
                return $_FAILED_
        fi

        #WE HAVE FILE.
        COPY "${_device_map_file_}" "${_device_map_file_mod_}"

        #REVERT ORIG FILE.
        MOVE "${_device_map_file_orig_}" "${_device_map_file_}"

        echo "Successfully reverted device.map configuration."
        return $_SUCCESS_
}

function Usage
{
	#TODO
	#NEED TO WRITE USAGE FOR THIS PROGRAM WITH DIFFERENT INPUT"S POSSIBLE.
	return $_SUCCESS_
}

function EXIT
{
	_exit_code_=$1
	date
	echo "-------END-------"
#	3>&-
#	cat $_TEMP_LOG_F_ >>"$_LOG_FILE_"
#	rm -rf $_TEMP_LOG_F_
	exit $_exit_code_
}


#MAIN#

#first we need to validate options passed.
#This could be at protection/Addition of Disk / failover.
echo "------- START -------"

date
_return_code_=""

ReadInputFile $1
_return_code_=$?
if [ 0 -ne $_return_code_ ]; then
	EXIT $_return_code_
fi

#WE NEED TO IMPORT LIB FILE.
if [ -d "$_FILES_PATH_/scripts/vCon/" ]; then
	. "$_FILES_PATH_/scripts/vCon/$_LIB_FILE_"
else
	. "$_FILES_PATH_/$_LIB_FILE_"
fi

ExecuteTask
_return_code_=$?
if [ 0 -ne $_return_code_ ]; then
	EXIT $_return_code_
fi

EXIT $_return_code_

