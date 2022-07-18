#!/bin/bash -x

# root fs mountpoint
export mntpath=""

export working_dir=""
export hydration_config_settings=""

export src_vx_dir=""

export os_name=""
export _firmware_type=""
export _grub2_efi_path=""


export RPM=/bin/rpm
export COPY=/bin/cp
export REMOVE=/bin/rm
export MKDIR=/bin/mkdir

_SUCCESS_=0
_FAILED_=1
_INMAGE_ORG_="-INMAGE-ORG"
_INMAGE_MOD_="-INMAGE-MOD"
_ASR_INSTALLGA_="asr-installga"

function PrintDataTime
{
    echo "$1: $(date +"%a %b %d %Y %T")"
}

function CopyFile
{
    _source_=$1
    _target_=$2

    echo "Copying file \"$_source_\" to \"$_target_\"."
    /bin/cp -f $_source_ $_target_
    if [ 0 -ne $? ]; then
        return $?
    fi
    return $_SUCCESS_
}

unset telemetry_data
#@1 - Telemetry Data
function add_telemetry_data
{
    if [[ -z $telemetrydata ]]; then
        telemetry_data="$1"
    else
        telemetry_data="$telemetry_data*#*$1"
    fi
}

function MoveFile
{
    _source_=$1
    _target_=$2

    echo "Moving file \"$_source_\" to \"$_target_\"."
    /bin/mv -f $_source_ $_target_
    if [ 0 -ne $? ]; then
        return $?
    fi
    return $_SUCCESS_
}

function BackupFile
{
    _source_=$1
    _target_="$1$_INMAGE_ORG_"
    
    if [ -f "$1" ]; then
        CopyFile $_source_ $_target_
        if [ 0 -ne $? ]; then
            echo "Copy error. Could not backup file $1."
            return $_FAILED_
        else
            echo "File $1 backup successful."
        fi
    else
        echo "File $1 does not exist. Nothing to backup."
    fi
    
    return $_SUCCESS_
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

function AddOrUpdateConfigValue
{
    _config_file_="$1"
    _setting_name_="$2"
    _setting_value_="$3"

    if grep -q "^$_setting_name_[[:space:]]*=.*" "$_config_file_" ; then
        # Line starts with setting key
        sed -i --follow-symlinks "s/\(^$_setting_name_[[:space:]]*=[[:space:]]*[\"]\?\)\(.*[\"]\?\)/\1$_setting_value_ \2/" "$_config_file_"
    elif grep -q "^[[:space:]][[:space:]]*$_setting_name_[[:space:]]*=.*" "$_config_file_" ; then
        # Line starts with space(s) and then setting key
        sed -i --follow-symlinks "s/\(^[[:space:]][[:space:]]*$_setting_name_[[:space:]]*=[[:space:]]*[\"]\?\)\(.*[\"]\?\)/\1$_setting_value_ \2/" "$_config_file_"
    else
        # Setting key not found, adding the setting key=value
        echo "$_setting_name_=\"$_setting_value_\"" >> "$_config_file_"
    fi
}

function UpdateConfigValue
{
    _config_file_="$1"
    _setting_name_="$2"
    _setting_value_="$3"

    if grep -q "^$2=$3" $1 ; then
        echo "$2=$3 config value is already in place."
    elif grep -q "^$2=.*" $1 ; then
        sed -i --follow-symlinks "s/\(^$2=\)\(.*\?\)/\1$3/" $1
    else
        echo "$2 setting not found, adding $2=$3"
        echo "$2=$3" >> $1
    fi
}

function UpdateConfigValueIfExists
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

function RemoveConfig
{
    _config_file_="$1"
    _setting_name_="$2"

    if grep -q "^$2=.*" $1 ; then
        sed -i --follow-symlinks "/^$2=/d" $1
    else
        echo "$2 config setting not found, nothing to remove."
    fi
}

function Usage
{
    echo "post-rec-script.sh <src-root-mnt-path> <working-dir>"
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
                /bin/cp -f "$_lvm_conf_file_" "$_lvm_conf_file_.old"
                #cp -f "${initrd_temp_dir}/init" "${_LOGS_}"
                #now we have to make in place replacement in lvm.conf file.
                #First we need to get the Line and then
                #check if all block devices are allowed?
                #if not, then check whether sd pattern is already allowed.
                #if not, then add sd* pattern to list of allowed block devices.
                _line_=`cat "$_lvm_conf_file_" | grep -P "^(\s*)filter(\s*)=(\s*).*"`
                echo $_line_
                if [ ! -z "$_line_" ]; then
                    _line_number_=`cat "$_lvm_conf_file_" | grep -P -n "^(\s*)filter(\s*)=(\s*).*" | awk -F":" '{print $1}'`
                    echo "Updating filter entry in $_lvm_conf_file_ file at line: $_line_number_"
                    sed -i "$_line_number_ c\filter=\"a\/.*\/\"" $_lvm_conf_file_
                else
                    echo "filter entry not found. Nothing to update in $_lvm_conf_file_ file"
                fi

                #cp -rf "etc/" "${_LOGS_}"
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

function UpdateVMRepositories()
{
    case $src_distro in
        CENTOS6*)
            sources_list_file="$mntpath/etc/yum.repos.d/CentOS-Base.repo"
            BackupFile $sources_list_file
            copy_file "/usr/local/AzureRecovery/CentOS6-Base.repo" "$sources_list_file"
        ;;
        CENTOS7*)
            sources_list_file="$mntpath/etc/yum.repos.d/CentOS-Base.repo"
            BackupFile $sources_list_file
            copy_file "/usr/local/AzureRecovery/CentOS7-Base.repo" "$sources_list_file"
        ;;
        CENTOS8*)
            sources_list_file="$mntpath/etc/yum.repos.d/CentOS-Base.repo"
            BackupFile $sources_list_file
            copy_file "/usr/local/AzureRecovery/CentOS7-Base.repo" "$sources_list_file"
        ;;
        OL6*)
            sources_list_file="$mntpath/etc/yum.repos.d/public-yum-ol6.repo"
            BackupFile $sources_list_file
            copy_file "/usr/local/AzureRecovery/public-yum-ol6.repo" "$sources_list_file"
        ;;
        OL7*)
            sources_list_file="$mntpath/etc/yum.repos.d/public-yum-ol7.repo"
            BackupFile $sources_list_file
            copy_file "/usr/local/AzureRecovery/public-yum-ol6.repo" "$sources_list_file"
        ;;
        OL8*)
            sources_list_file="$mntpath/etc/yum.repos.d/public-yum-ol8.repo"
            BackupFile $sources_list_file
            copy_file "/usr/local/AzureRecovery/public-yum-ol8.repo" "$sources_list_file"
        ;;
        UBUNTU*)
            sources_list_file="$mntpath/etc/apt/sources.list"
            BackupFile "$sources_list_file"
            sed -i 's/http:\/\/archive\.ubuntu\.com\/ubuntu\//http:\/\/azure\.archive\.ubuntu\.com\/ubuntu\//g' "$sources_list_file"
            sed -i 's/http:\/\/[a-z][a-z]\.archive\.ubuntu\.com\/ubuntu\//http:\/\/azure\.archive\.ubuntu\.com\/ubuntu\//g' "$sources_list_file"
        ;;
    esac
}

# $1: Complete path of zip file.
# $2: Target directory for extraction of zip file.
function Extract_ZipFile_Using_Python
{
    echo "Extracting zip from $1 to $2"

    # Create python script to extract the zip file.
    local unzip_py="$mntpath/${base_linuxga_path}/unzip.py"
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
    python3 $unzip_py >> "$mntpath/$base_linuxga_path/ASRLinuxGA.log" 2>&1

    return $?
}

unset pythonver
check_valid_python_version()
{
    # Check for Major Version 3 for python.
    chroot $mntpath python3 --version
    if [[ $? -ne 0 ]] ; then
        echo "Python3 is not installed on the target VM."
    else
        pythonver=3
        return
    fi

    # Check for Major version 2 and Minor Version >= 6 for Python.
    chroot $mntpath python --version
    if [[ $? -ne 0 ]] ; then
        echo "Python is not installed on the target VM.\n"
        pythonver=1
        return
    else
        vermajor=$(chroot $mntpath python -c"import platform; major, minor, patch = platform.python_version_tuple(); print(major)")
        verminor=$(chroot $mntpath python -c"import platform; major, minor, patch = platform.python_version_tuple(); print(minor)")
        echo "Major Version: $vermajor Minor Version: $verminor"
        if [ $vermajor -eq 2 ] && [ $verminor -ge 6 ]; then
            pythonver=2
			return
        else
            echo "Failed to install Linux Guest Agent on the VM.\n"
            echo "Install python version 2.6+ to install Guest Agent.\n"
            pythonver=1
            return
        fi
    fi

    pythonver=1
}

function test_setuptools_prereq
{
    setup_uuid=$(uuidgen)
    setuptools_file_path="$mntpath/$base_linuxga_path/setuptools-test-$setup_uuid.py"

    echo "import sys"                   >> $setuptools_file_path
    echo "try:"                         >> $setuptools_file_path
    echo "        import setuptools"    >> $setuptools_file_path
    echo "except ImportError:"          >> $setuptools_file_path
    echo "        print(\"ABSENT\")"    >> $setuptools_file_path
    echo "else:"                        >> $setuptools_file_path
    echo "        print(\"PRESENT\")"   >> $setuptools_file_path

    setuptools_output=""
    if [[ "$1" == "python3" ]]; then
        setuptools_output=$(chroot $mntpath python3 "/$base_linuxga_path/setuptools-test-$setup_uuid.py")
    else
        setuptools_output=$(chroot $mntpath python "/$base_linuxga_path/setuptools-test-$setup_uuid.py")
    fi

	setup_tools_package_path="/usr/local/AzureRecovery/setuptools-33.1.1.zip"

    if [[ "$setuptools_output" == "ABSENT" ]]; then
        echo "setuptools is absent on the VM. Trying manual pre-boot installation."

        Extract_ZipFile_Using_Python $setup_tools_package_path $mntpath/$base_linuxga_path/
        add_telemetry_data "no-setuptools"
    else
        echo "setuptools is present on the VM."
    fi

    rm -f $setuptools_file_path
}

function validate_guestagent_prereqs
{
    check_valid_python_version

    if [[ $pythonver -eq 1 ]]; then
        echo "Python not installed/ incompatible with linux guest agent requirements."
        add_telemetry_data "no-python"
    elif [[ $$pythonver -eq 3 ]]; then
        echo "Python3 present on the source VM."
        add_telemetry_data "python2"
        test_setuptools_prereq "python3"
    elif [[ $$pythonver -eq 2 ]]; then
        echo "Python 2.6+ installed on the source VM."
        add_telemetry_data "python3"
        test_setuptools_prereq "python"
    else
        echo "Unsupported Python version"
        add_telemetry_data "no-python"
    fi

    chroot $mntpath systemctl

    if [[ $? -eq 127 ]]; then
        echo "systemctl is not installed on the source VM"
        add_telemetry_data "systemctl"
    fi
}

#This handles all requirements to make a modification in initrd file..
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
        _retCode_=$_SUCCESS_
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
                        _retCode_=$_FAILED_
                fi

                _initrd_ramfs_file_=`echo $_virtual_initrd_ramfs_file_ | sed "s/-InMage-Virtual$//g"`
                echo "Initrd file = $_initrd_ramfs_file_"
                #TAKE COPY OF THIS FILE
                #CHECK IF THE ORIG FILE ALREADY EXISTS?
                if [ -f "${_initrd_ramfs_file_}${_INMAGE_ORG_}" ]; then
                    /bin/mv -f  "${_initrd_ramfs_file_}${_INMAGE_ORG_}" "$_initrd_ramfs_file_"
                fi

                $COPY -f  "$_initrd_ramfs_file_" "${_initrd_ramfs_file_}${_INMAGE_ORG_}"
                $COPY -f  "$_virtual_initrd_ramfs_file_" "$_initrd_ramfs_file_"
        done
        
        return $_retCode_
}

function Enable_installga_chkconfig
{
    echo "Adding guest agent installation service inside /etc/init.d"
    local asr_installga_exfile_path="$mntpath/etc/init.d/${_ASR_INSTALLGA_}"
    echo "#!/bin/bash
# This is for RHEL systems
# processname: ${_ASR_INSTALLGA_}
# chkconfig: 2345 90 90
# description: Azure migrate startup script to configure networks

### BEGIN INIT INFO
# Provides: ${_ASR_INSTALLGA_}
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

    local _chkconfig_gastartup_file="/etc/init.d/${_ASR_INSTALLGA_}"
    if [[ -h "${chroot_path}$_chkconfig_gastartup_file" ]]; then
        echo "Startup script is already configured!"
        return;
    fi

    chroot $mntpath ln -s $asr_installga_exfile_path $_chkconfig_gastartup_file
    if [[ $? -ne 0 ]]; then
        echo "WARNING: Could not create start-up script."
    fi
    
    local _config_startup_cmd=""
    if chroot $mntpath which chkconfig > /dev/null 2>1$ ;then
        _config_startup_cmd="chroot $mntpath chkconfig --add ${_ASR_INSTALLGA_}"
    elif chroot $mntpath which update-rc.d > /dev/null 2>1$ ;then
        _config_startup_cmd="chroot $mntpath update-rc.d ${_ASR_INSTALLGA_} defaults"
    else
        echo "WARNING: No tools found to configure start-up script."
        add_telemetry_data "startup"
        return 0
    fi
    
    echo "Adding startup script."
    $_config_startup_cmd > /dev/null 2>&1
    if [[ $? -ne 0 ]]; then
        echo "WARNING: Could not add start-up script."
        add_telemetry_data "startup"
    else
        echo "Successfully added startup script!"
    fi

    return $?
}

function Enable_installga_Service
{
    # Startup script to install azure guest agent.
    local asr_installga_exfile_path="usr/local/${_ASR_INSTALLGA_}/asr.installga"

    # Create a directory if it doesn't exist.
    if [[ ! -d "$mntpath/usr/local/${_ASR_INSTALLGA_}" ]]; then
        echo "Creating a new directory /usr/local/${_ASR_INSTALLGA_}"
        mkdir "$mntpath/usr/local/${_ASR_INSTALLGA_}"
    fi
    
    if [[ -f $mntpath/$asr_installga_exfile_path ]]; then
        echo "/usr/local/${_ASR_INSTALLGA_}/asr.installga exists. Deleting the file."
        rm $mntpath/$asr_installga_exfile_path
    fi

    setup_tool_install="install_setup_tools_false"
    if [[ $telemetry_data == *"no-setuptools"* ]]; then
        setup_tool_install="install_setup_tools_true"
    fi

    # Write execution commands for asr.installga executable.
    echo "Creating asr.installga for execution during boot."

    echo "#!/bin/bash" >> $mntpath/$asr_installga_exfile_path
    echo "#This file is generated for execution during boot" >> $mntpath/$asr_installga_exfile_path
    echo "bash /$base_linuxga_path/InstallLinuxGuestAgent.sh $os_name  \"$base_linuxga_path\" $setup_tool_install >> /$base_linuxga_path/ASRLinuxGA.log"  >> $mntpath/$asr_installga_exfile_path
    echo "exit 0" >> $mntpath/$asr_installga_exfile_path

    # Add executable permissions.
    chmod +x $mntpath/$asr_installga_exfile_path

    local _systemd_config_file="$mntpath/lib/systemd/system/${_ASR_INSTALLGA_}.service"
    if [[ ! -d $mntpath/lib/systemd/system ]]; then
        if [[ -d $mntpath/usr/lib/systemd/system ]]; then
            _systemd_config_file="$mntpath/usr/lib/systemd/system/${_ASR_INSTALLGA_}.service"
        elif [[ -d $mntpath/etc/systemd/system ]]; then
            _systemd_config_file="$mntpath/etc/systemd/system/${_ASR_INSTALLGA_}.service"
        fi
    fi

    # Delete the file if it already exists.
    if [[ -f $_systemd_config_file ]]; then
        echo "$_systemd_config_file exists. Deleting the file."
        rm $_systemd_config_file
    fi

    echo "Creating systemd config file. $_systemd_config_file"

    echo "[Unit]" >> $_systemd_config_file
    echo "Description=Install Azure Linux Guest agent startup script by ASR" >> $_systemd_config_file
    echo "ConditionPathExists=/$asr_installga_exfile_path" >> $_systemd_config_file
    echo "" >> $_systemd_config_file
    echo "[Service]" >> $_systemd_config_file
    echo "Type=forking" >> $_systemd_config_file
    echo "ExecStart=/$asr_installga_exfile_path start" >> $_systemd_config_file
    echo "TimeoutSec=0" >> $_systemd_config_file
    echo "StandardOutput=tty" >> $_systemd_config_file
    echo "RemainAfterExit=yes" >> $_systemd_config_file
    echo "SysVStartPriority=99" >> $_systemd_config_file
    echo "" >> $_systemd_config_file
    echo "[Install]" >> $_systemd_config_file
    echo "WantedBy=multi-user.target" >> $_systemd_config_file

    chroot $mntpath systemctl enable "${_ASR_INSTALLGA_}.service"

    # This part validates presence of Python & whether installga service has properly enabled.
    echo "Checking for linux installation prereqs within source vm."

    gacheck=$(chroot $mntpath python3 --version)
    echo "Python 3 version. Absent if empty or fails: $gacheck"

    gacheck=$(chroot $mntpath python --version)
    echo "Python 2 version. Absent if empty or fails: $gacheck"

    gacheck=$(cat $_systemd_config_file)
    echo "systemd service file: $gacheck"

    gacheck=$(cat $mntpath/$asr_installga_exfile_path)
    echo "asr.installga: $gacheck"
}

function enable_postlogin_installga
{
    target_profiled_file="$mntpath/etc/profile.d/ASRLinuxGAStartup.sh"
    
    if [[ ! -d "$mntpath/etc/profile.d" ]]; then
        echo "/etc/profile.d directory doesn't exist"
    else
        echo "#!/bin/bash" >> $target_profiled_file
        echo "echo \"This is Azure Site Recovery One-Time Guest Agent Installation Setup.\"" >> $target_profiled_file
        echo "echo \"The service quietly exits if WALinuxAgent is already installed.\"" >> $target_profiled_file
        echo "echo \"If guest agent installation failed at pre-boot step due to python/installation prereqs not fulfilled, \"" >> $target_profiled_file
        echo "echo \"this service is triggered post login to install WALinuxAgent using VM's repository.\"" >> $target_profiled_file
        echo "echo \"The service may prompt you to login using elevated bash shell for installation to proceed.\"" >> $target_profiled_file
        echo "echo \"You can skip the installation, and can install manually, or retrigger the service by running\"" >> $target_profiled_file
        echo "echo \"systemctl start asr-installga or service asr-installga start\"" >> $target_profiled_file
        echo "bash /$base_linuxga_path/InstallLinuxGuestAgent.sh $os_name  \"$base_linuxga_path\" $setup_tool_install" >> $target_profiled_file
        echo "echo \"Installation process completed. Proceed with Login.\"" >> $target_profiled_file
        echo "rm -f \"/etc/profile.d/ASRLinuxGAStartup.sh\"" >> $target_profiled_file

        echo "/etc/profile.d post login startup file created."
    fi    
}

unset base_linuxga_path
function InstallLinuxGuestAgent
{
    ga_uuid=$(uuidgen)
    base_linuxga_path="var/ASRLinuxGA-$ga_uuid"

    if [[ ! -d $mntpath/$base_linuxga_path ]]; then
        echo "Base linux guest agent packages directory doesn't exist'. Creating $mntpath/$base_linuxga_path"
        mkdir $mntpath/$base_linuxga_path
    fi

    validate_guestagent_prereqs

    if [[ $telemetry_data != *"systemctl"* ]]; then
        Enable_installga_Service
    else
        Enable_installga_chkconfig
    fi

    UpdateVMRepositories
    enable_postlogin_installga

    echo "Hydration Being Performed on the VM. $(date)" >> "$mntpath/$base_linuxga_path/ASRLinuxGA.log"

    linuxgadir="/usr/local/AzureRecovery/WALinuxAgentASR/WALinuxAgent-master"
    targetgadir="$mntpath/$base_linuxga_path/WALinuxAgentASR/"

    if [[ ! -d "$linuxgadir" ]]; then
        echo "Folder containing Guest agent binaries is not present."
        return 1;
    fi

    if [[ ! -d $targetgadir ]]; then
        echo "Target guest agent folder doesn't exist. Creating $targetgadir"
        mkdir $targetgadir
    fi

    CopyDirectory $linuxgadir $targetgadir
    if [[ $? -ne 0 ]]; then
        echo "Could not copy LinuxGuestAgent installation directory to target location."
        return 1;
    fi

    CopyFile "/usr/local/AzureRecovery/InstallLinuxGuestAgent.sh" "$mntpath/$base_linuxga_path/InstallLinuxGuestAgent.sh"
    if [[ $? -ne 0 ]]; then
        echo "Could not copy LinuxGuestAgent installation file."
        return 1;
    fi

    CopyFile "/usr/local/AzureRecovery/PythonSetupPrereqs.py" "$mntpath/$base_linuxga_path/PythonSetupPrereqs.py"
    if [[ $? -ne 0 ]]; then
        echo "Could not copy python setup prereqs file."
        # Don't fail.
    fi

    chmod +x $mntpath/$base_linuxga_path/*
}

# This function appends kernel entry in grub with numa=off console=ttyS0 earlyprintk=ttyS0 rootdelay=300
# Remove rhgb quiet from grub configuration file
function ModifyMenuListFile
{
        _grub_path_="$1/boot/grub"
        _menu_list_file_path_="$_grub_path_/menu.lst"

        if [[ "$firmware_type" = "UEFI" ]]; then
            _grub_path_="$_grub2_efi_path";
            _menu_list_file_path_="$_grub_path_/grub.conf"
        fi
        
        # ensure file exists, it should be either symblink or regular file
        if [ -h "$_menu_list_file_path_" ]; then
            tmp_file=`ls -l "${_menu_list_file_path_}" | awk -F'->' '{ print $2 }'`
            tmp_file=`echo $tmp_file | sed -e 's: ::g'`

            if [[ "$firmware_type" = "UEFI" ]]; then
               echo "grub.conf sym link points to - ${tmp_file}"
                # Only RHEL6/CENTOS6 are supported for UEFI.
                if [ -f "$1/boot/efi/EFI/redhat/$tmp_file" ]; then
                    # menu.lst sym link points to grub file relative path.
                    _menu_list_file_path_="$1/boot/efi/EFI/redhat/$tmp_file"
                elif [ -f "$1/$tmp_file" ]; then
                    # menu.lst sym link points to grub file absolute path.
                    _menu_list_file_path_="$_grub2_efi_path/$tmp_file"
                else
                    echo "ERROR :: Failed to find file ${_menu_list_file_path_}"
                    echo "ERROR :: Could not modify grub file."
                    return $_FAILED_
                fi
            else
                echo "menu.lst sym link points to - ${tmp_file}"
                        
                if [ -f "$1/boot/grub/$tmp_file" ]; then
                    # menu.lst sym link points to grub file relative path.
                    _menu_list_file_path_="$1/boot/grub/$tmp_file"
                elif [ -f "$1/$tmp_file" ]; then
                    # menu.lst sym link points to grub file absolute path.
                    _menu_list_file_path_="$1/$tmp_file"
                else
                    echo "ERROR :: Failed to find file ${_menu_list_file_path_}"
                    echo "ERROR :: Could not modify grub file."
                    return $_FAILED_
                fi
            fi
        elif [ ! -f "$_menu_list_file_path_" ]; then
                echo "ERROR :: Failed to find file \"menu.lst\" in path \"$_grub_path_\"."
                return $_FAILED_
        fi

        # backup the original file
        _menu_list_file_orig_="${_menu_list_file_path_}${_INMAGE_ORG_}"
        if [ -f "${_menu_list_file_orig_}" ]; then
                /bin/mv -f "${_menu_list_file_orig_}" "$_menu_list_file_path_"
        fi

        /bin/cp -f "$_menu_list_file_path_" "${_menu_list_file_orig_}"
        sed -i --follow-symlinks "s/ rhgb / /" ${_menu_list_file_path_}
        sed -i --follow-symlinks "s/quiet/ /" ${_menu_list_file_path_}
        grep "numa=off  console=ttyS0  earlyprintk=ttyS0  rootdelay=300 " ${_menu_list_file_path_}
        if [ $? -ne 0 ]; then
                sed -i  --follow-symlinks  '/[\t]*[ ]*kernel.*\/.*root=/s/$/& numa=off  console=ttyS0  earlyprintk=ttyS0  rootdelay=300 /' ${_menu_list_file_path_}
        fi

        # Temporary workaround for sym link breakages
        _menu_list_file_path_bak_="$_grub_path_/grub.conf"
        sed -i --follow-symlinks "s/ rhgb / /" ${_menu_list_file_path_bak_}
        sed -i --follow-symlinks "s/quiet/ /" ${_menu_list_file_path_bak_}
        grep "numa=off  console=ttyS0  earlyprintk=ttyS0  rootdelay=300 " ${_menu_list_file_path_bak_}
        if [ $? -ne 0 ]; then
                sed -i  --follow-symlinks  '/[\t]*[ ]*kernel.*\/.*root=/s/$/& numa=off  console=ttyS0  earlyprintk=ttyS0  rootdelay=300 /' ${_menu_list_file_path_bak_}
        fi
        return $_SUCCESS_
}

function ModifyGrubConfig_SLES
{
    _grub_conf_file_path="$mntpath/boot/grub2/grub.cfg"
    [[ "$firmware_type" = "UEFI" ]] && _grub_conf_file_path="$_grub2_efi_path/grub.cfg"

    # Backup /boot/grub2/grub.cfg file
    BackupFile "$_grub_conf_file_path"
    
    # Backup the /etc/default/grub file
    _grub_options_file_path="$mntpath/etc/default/grub"
    BackupFile "$_grub_options_file_path"
    
    # Edit "/etc/default/grub" file
    _grub_config_options="$1"   
    if [[ -z "$_grub_config_options" ]] ; then
        echo "Default azure grub options: "
        _grub_config_options="console=ttyS0 earlyprintk=ttyS0 rootdelay=300"
    fi

    # Filter out the options need to add by excluding existing options
    _required_grub_config_options=""
    for option in $_grub_config_options
    do
        if grep -q "^GRUB_CMDLINE_LINUX[[:space:]]*=.*[[:space:]\"]$option[[:space:]\"].*" "$_grub_options_file_path" ||
           grep -q "^[[:space:]][[:space:]]*GRUB_CMDLINE_LINUX[[:space:]]*=.*[[:space:]\"]$option[[:space:]\"].*" "$_grub_options_file_path"
        then
            echo "$option is already present. Skipping it..."
        else
            _required_grub_config_options="$_required_grub_config_options $option"
        fi
    done
    
    if [[ -z "$_required_grub_config_options" ]] ; then
        echo "All grub options are already in place. Not making any more changes to grub."
    else
        echo "Required Grub options are: $_required_grub_config_options"
        
        echo "### BEGIN: $_grub_options_file_path file (Before) ###"
        cat "$_grub_options_file_path"
        echo "### END: $_grub_options_file_path file (Before) ###"
        
        AddOrUpdateConfigValue "$_grub_options_file_path" "GRUB_CMDLINE_LINUX" "$_required_grub_config_options"
        
        echo "### BEGIN: $_grub_options_file_path file (After) ###"
        cat "$_grub_options_file_path"
        echo "### END: $_grub_options_file_path file (After) ###"
    fi
        
    # Temporarily modify grub.cfg for first boot, as part of boot up script "grub2-mkconfig" will run
    # and updates the grub.cfg again.
    if [[ -z "$_required_grub_config_options" ]] ; then
        echo "All grub options are already in place. Not making any more changes to grub.cfg."
    else
        # This command also inserts the options to recovery mode grub common line, and the grub.cfg will be re-build on first boot
        # when boot-up script runs 'grub2-mkconfig' command.
        sed -i --follow-symlinks "s/^[[:space:]][[:space:]]*linux[[:space:]]\/.* root=.*[[:space:]] .*/&$_required_grub_config_options /" "$_grub_conf_file_path"
    fi

    return $_SUCCESS_
}

function ModifyGrubConfig_DEBIAN
{
    # Backup the grub & grub.cfg files
    _grub_conf_file_path="$mntpath/boot/grub/grub.cfg"
    _grub_options_file_path="$mntpath/etc/default/grub"

    # Change the file path to /boot/efi/efi/debian folder for UEFI scenario.
    [[ "$firmware_type" = "UEFI" ]] && _grub_conf_file_path="$_grub2_efi_path/grub.cfg"

    BackupFile "$_grub_conf_file_path"
    BackupFile "$_grub_options_file_path"
    # Edit "/etc/default/grub" file
    #  GRUB_CMDLINE_LINUX="console=tty0 console=ttyS0,115200n8 earlyprintk=ttyS0,115200 rootdelay=30"
    _grub_config_options="console=tty0 console=ttyS0,115200n8 earlyprintk=ttyS0,115200 rootdelay=30"
    _required_grub_config_options=""
    # Filter out the options need to add by excluding existing options
    for option in $_grub_config_options
    do
        if grep -q "^GRUB_CMDLINE_LINUX[[:space:]]*=.*[[:space:]\"]$option[[:space:]\"].*" "$_grub_options_file_path" ||
           grep -q "^[[:space:]][[:space:]]*GRUB_CMDLINE_LINUX[[:space:]]*=.*[[:space:]\"]$option[[:space:]\"].*" "$_grub_options_file_path"
        then
            echo "$option is already present. Skipping it..."
        else
            _required_grub_config_options="$_required_grub_config_options $option"
        fi
    done
    if [[ -z "$_required_grub_config_options" ]] ; then
        echo "All grub options are already in place. Not making any more chages to grub."
    else
        echo "Required Grub options are: $_required_grub_config_options"
        echo "### BEGIN: $_grub_options_file_path file (Before) ###"
        cat "$_grub_options_file_path"
        echo "### END: $_grub_options_file_path file (Before) ###"
        AddOrUpdateConfigValue "$_grub_options_file_path" "GRUB_CMDLINE_LINUX" "$_required_grub_config_options"
        echo "### BEGIN: $_grub_options_file_path file (After) ###"
        cat "$_grub_options_file_path"
        echo "### END: $_grub_options_file_path file (After) ###"
    fi
    # Temporarily modify grub.cfg for first boot, as part of boot up script "update-grub" will run
    # and updates the grub.cfg again.
    if [[ -z "$_required_grub_config_options" ]] ; then
        echo "All grub options are already in place. Not making any more changes to grub.cfg."
    else
        # This command also inserts the optoins to recovery mode grub common line, and the grub.cfg will be re-build on first boot
        # when boot-up script runs 'update-grub' command.
        sed -i --follow-symlinks "s/^[[:space:]][[:space:]]*linux[[:space:]]\/.* root=.*[[:space:]]ro.*/&$_required_grub_config_options /" "$_grub_conf_file_path"
    fi
    return $_SUCCESS_
}

function ModifyGrubConfig_UBUNTU
{
    # Backup the grub & grub.cfg files
    _grub_conf_file_path="$mntpath/boot/grub/grub.cfg"
    _grub_options_file_path="$mntpath/etc/default/grub"
    
    BackupFile "$_grub_conf_file_path"
    BackupFile "$_grub_options_file_path"
    
    # Edit "/etc/default/grub" file
    #  GRUB_CMDLINE_LINUX_DEFAULT="console=tty1 console=ttyS0 earlyprintk=ttyS0 rootdelay=300"
    _grub_config_options="console=ttyS0 console=tty1 earlyprintk=ttyS0 rootdelay=300"
    _required_grub_config_options=""
    
    # Filter out the options need to add by excluding existing options
    for option in $_grub_config_options
    do
        if grep -q "^GRUB_CMDLINE_LINUX_DEFAULT[[:space:]]*=.*[[:space:]\"]$option[[:space:]\"].*" "$_grub_options_file_path" ||
           grep -q "^[[:space:]][[:space:]]*GRUB_CMDLINE_LINUX_DEFAULT[[:space:]]*=.*[[:space:]\"]$option[[:space:]\"].*" "$_grub_options_file_path"
        then
            echo "$option is already present. Skipping it..."
        else
            _required_grub_config_options="$_required_grub_config_options $option"
        fi
    done
    
    if [[ -z "$_required_grub_config_options" ]] ; then
        echo "All grub options are already in place. Not making any more chages to grub."
    else
        echo "Required Grub options are: $_required_grub_config_options"
        
        echo "### BEGIN: $_grub_options_file_path file (Before) ###"
        cat "$_grub_options_file_path"
        echo "### END: $_grub_options_file_path file (Before) ###"
        
        AddOrUpdateConfigValue "$_grub_options_file_path" "GRUB_CMDLINE_LINUX_DEFAULT" "$_required_grub_config_options"
        
        echo "### BEGIN: $_grub_options_file_path file (After) ###"
        cat "$_grub_options_file_path"
        echo "### END: $_grub_options_file_path file (After) ###"
    fi
        
    # Temporarily modify grub.cfg for first boot, as part of boot up script "update-grub" will run
    # and updates the grub.cfg again.
    if [[ -z "$_required_grub_config_options" ]] ; then
        echo "All grub options are already in place. Not making any more chages to grub.cfg."
    else
        # This command also inserts the options to recovery mode grub command line, and the grub.cfg will be re-build on first boot
        # when boot-up script runs 'update-grub' command.
        sed -i --follow-symlinks "s/^[[:space:]][[:space:]]*linux[[:space:]]\/.* root=.*[[:space:]]ro.*/&$_required_grub_config_options /" "$_grub_conf_file_path"
    fi

    return $_SUCCESS_
}

function ModifyGrubOptions
{
    # Backup /boot/grub2/grub.cfg file
    _grub_options_file_path="$mntpath/etc/default/grub"
    BackupFile "$_grub_options_file_path"
    
    # Edit "/etc/default/grub" file
    _grub_config_options="$1"   
    if [[ -z "$_grub_config_options" ]] ; then
        echo "Default azure grub options: "
        _grub_config_options="rootdelay=300 console=ttyS0 earlyprintk=ttyS0"
    fi

    # Filter out the options need to add by excluding existing options
    _required_grub_config_options=""
    for option in $_grub_config_options
    do
        if grep -q "^GRUB_CMDLINE_LINUX[[:space:]]*=.*[[:space:]\"]$option[[:space:]\"].*" "$_grub_options_file_path" ||
           grep -q "^[[:space:]][[:space:]]*GRUB_CMDLINE_LINUX[[:space:]]*=.*[[:space:]\"]$option[[:space:]\"].*" "$_grub_options_file_path"
        then
            echo "$option is already present. Skipping it..."
        else
            _required_grub_config_options="$_required_grub_config_options $option"
        fi
    done
    
    if [[ -z "$_required_grub_config_options" ]] ; then
        echo "All grub options are already in place. Not making any more chages to grub."
    else
        echo "Required Grub options are: $_required_grub_config_options"
        
        echo "### BEGIN: $_grub_options_file_path file (Before) ###"
        cat "$_grub_options_file_path"
        echo "### END: $_grub_options_file_path file (Before) ###"
        
        AddOrUpdateConfigValue "$_grub_options_file_path" "GRUB_CMDLINE_LINUX" "$_required_grub_config_options"
        
        echo "### BEGIN: $_grub_options_file_path file (After) ###"
        cat "$_grub_options_file_path"
        echo "### END: $_grub_options_file_path file (After) ###"
    fi

    return $_SUCCESS_
}

function ModifyGrubConfig_RHEL_CENTOS
{
    ModifyGrubOptions "$1"

    # Temporarily modify grub.cfg for first boot, as part of boot up script "grub2-mkconfig" will run
    # and updates the grub.cfg again.
    if [[ -z "$_required_grub_config_options" ]] ; then
        echo "All grub options are already in place. Not making any more chages to grub.cfg."
    else
        # Backup the /etc/default/grub file
        _grub_conf_file_path="$mntpath/boot/grub2/grub.cfg"

        # Change the file path to /boot/efi/efi/redhat for UEFI scenario.
    [[ "$firmware_type" = "UEFI" ]] && _grub_conf_file_path="$_grub2_efi_path/grub.cfg"

        BackupFile "$_grub_conf_file_path"

        # This command also inserts the options to recovery mode grub command line, and the grub.cfg will be re-build on first boot
        # when boot-up script runs 'update-grub' command.
        sed -i --follow-symlinks "s/^[[:space:]][[:space:]]*linux16[[:space:]]\/.* root=.*[[:space:]]ro.*/&$_required_grub_config_options /" "$_grub_conf_file_path"
    fi

    return $_SUCCESS_
}

function ModifyGrub_BLS
{
    ModifyGrubOptions "$1"

    if [[ -z "$_required_grub_config_options" ]] ; then
        echo "Grub options already in place. Skip chages to grub.cfg."
    else
        echo "Add options: $_required_grub_config_options"
        blsdir="$mntpath/boot/loader/entries"

        for _efile in `ls $blsdir`; do
            _grub_conf_file_path=$blsdir/$_efile
            BackupFile "$_grub_conf_file_path"
            sed -i -e "/^options/s/$/ $_required_grub_config_options/" $_grub_conf_file_path
        done
    fi

    return $_SUCCESS_
}

function RemovePersistentNetGeneratorRules_CENTOS7
{
    BackupFile $mntpath/etc/udev/rules.d/75-persistent-net-generator.rules
    /bin/rm -f $mntpath/etc/udev/rules.d/75-persistent-net-generator.rules
}

function RemoveNetworkManager
{
    /bin/rm -f $mntpath/etc/udev/rules.d/*-persistent-net.rules
    /bin/rm -f $mntpath/lib/udev/rules.d/*-persistent-net-generator.rules
    
    PrintDataTime "Removing NetworkManager StartTime: "
    /sbin/chroot $mntpath /bin/rpm -e --nodeps NetworkManager
    PrintDataTime "EndTime: "
    
    # Remove netwokr manager binary if exist.
    /bin/rm -f $mntpath/etc/init.d/NetworkManager
}

function RemoveNetworkManager_UBUNTU
{
    BackupFile $mntpath/lib/udev/rules.d/75-persistent-net-generator.rules
    /bin/rm -f $mntpath/lib/udev/rules.d/75-persistent-net-generator.rules
    
    BackupFile $mntpath/etc/udev/rules.d/70-persistent-net.rules
    /bin/rm -f $mntpath/etc/udev/rules.d/70-persistent-net.rules
    
    # TODO : Verify if the rpm can be able to remove the NetworkManager package.
    #        if not then need to investigate the similar tool such as : get-apt, apt ...etc.
}

function ConfigureNetworkFile_CentOS_RHEL
{
    local gtwdev=$1
    # /etc/sysconfig/network file modifications
    local network_file=$mntpath/etc/sysconfig/network
    if [[ -f $network_file ]]; then
        # Log the original network file
        echo "**** network file ****"
        cat $network_file
        echo "**** end ****"
        
        BackupFile $network_file
        UpdateConfigValue $network_file "NETWORKING" "yes"
        RemoveConfig $network_file "GATEWAY"
    else
        echo "NETWORKING=yes" > $network_file
        echo "HOSTNAME=localhost.localdomain" >> $network_file
    fi
    
    # Update GATEWAYDEV if provided.
    if [[ ! -z "$gtwdev" ]]; then
        UpdateConfigValue $network_file "GATEWAYDEV" "$gtwdev"
    else
        UpdateConfigValueIfExists $network_file "GATEWAYDEV" "eth0"
    fi
    
    # Log the modified network file.
    echo "**** updated network file ****"
    cat $network_file
    echo "**** end ****"
}

function ConfigureDefaultNICForUBUNTUCloudInit
{
    # As a workaround, taking a random filename for netplan dhcp configuration.
    # As soon as Azure Compute publishes steps for preparing Ubuntu 18.04 LTS
    # we will follow those instructions to prepare netplan configuration.
    # NOTE: Change the file name in setazureip.sh when the change is made here
    local dhcp_netplan_file="$mntpath/etc/netplan/50-azure_migrate_dhcp.yaml"
    if [[ -f $dhcp_netplan_file ]]; then
        echo "DHCP netplan configuration for Azure already exists"
        return;
    fi
    
    echo "# This is generated for ASR to make NICs DHCP in Azure." > $dhcp_netplan_file
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
    
    echo "Applying the dhcp netplan configuration... "
    chroot $mntpath netplan apply
    if [[ $? -eq 0 ]]; then
        echo "netplan with dhcp settings applied!"
    else
        echo "WARNING: netplan couldn't apply dhcp settings."
    fi
}


function ConfigureDefaultNICForUBUNTU
{
    _ubuntu_network_interfaces_file="$mntpath/etc/network/interfaces"
    
    # Backup the original interfaces file
    BackupFile "$_ubuntu_network_interfaces_file"
    
    # Create new interfaces file
    echo "# The loopback network interface" > $_ubuntu_network_interfaces_file
    echo "auto lo" >> $_ubuntu_network_interfaces_file
    echo "iface lo inet loopback" >> $_ubuntu_network_interfaces_file
    echo "" >> $_ubuntu_network_interfaces_file
    echo "# The primary network interface" >> $_ubuntu_network_interfaces_file
    echo "auto eth0" >> $_ubuntu_network_interfaces_file
    echo "iface eth0 inet dhcp" >> $_ubuntu_network_interfaces_file
    # TODO: Conform below lines are required or not
    #echo "" >> $_ubuntu_network_interfaces_file
    #echo "# This is an autoconfigured IPv6 interface" >> $_ubuntu_network_interfaces_file
    #echo "iface eth0 inet6 auto" >> $_ubuntu_network_interfaces_file
}

function ConfigureDefaultNICForRHEL_CENTOS_OEL 
{
    #Setting the ipaddress for default NIC ifcfg-eth0
    NETWORKCONFIGFILESDIR="$mntpath/etc/sysconfig/network-scripts"
    BACKUP_NETWORKCONFIGFILES="$NETWORKCONFIGFILESDIR/asr_nw"
    
    if [ ! -d $BACKUP_NETWORKCONFIGFILES ]
    then
        mkdir -p $BACKUP_NETWORKCONFIGFILES
    fi

    ###Moving all the network config files to backup folder#####
    mv $NETWORKCONFIGFILESDIR/ifcfg-e* $BACKUP_NETWORKCONFIGFILES/

    name="eth0"
    ConfigFileName=$NETWORKCONFIGFILESDIR/ifcfg-$name
    
    echo "DEVICE=$name" > $ConfigFileName
    echo "ONBOOT=yes" >> $ConfigFileName
    echo "DHCP=yes" >> $ConfigFileName
    echo "BOOTPROTO=dhcp" >> $ConfigFileName
    echo "TYPE=Ethernet" >>  $ConfigFileName
    echo "USERCTL=no" >>  $ConfigFileName
    echo "PEERDNS=yes" >>  $ConfigFileName
    echo "IPV6INIT=no" >> $ConfigFileName
    
    RefIfcfgFile=$(ls $BACKUP_NETWORKCONFIGFILES/ifcfg-e* | head -n 1)
    if [ $? -ne 0 ]; then
        # default permissions. Log permissions for debug purpose
        ls -lrt $ConfigFileName
    else
        # preserve permissions from existing ifcfg-eth* file
        chown --reference=$RefIfcfgFile $ConfigFileName
        chmod --reference=$RefIfcfgFile $ConfigFileName
    fi
}

function ConfigureDefaultNICForSLES 
{
    #Setting the ipaddress for default NIC ifcfg-eth0
    
    NETWORKCONFIGFILESDIR="$mntpath/etc/sysconfig/network"
    BACKUP_NETWORKCONFIGFILES="$NETWORKCONFIGFILESDIR/asr_nw"
    
    if [ ! -d $BACKUP_NETWORKCONFIGFILES ]
    then
        mkdir -p $BACKUP_NETWORKCONFIGFILES
    fi
    
    ###Moving all the network config files to backup folder#####
    mv -f $NETWORKCONFIGFILESDIR/ifcfg-eth* $BACKUP_NETWORKCONFIGFILES/
    
    name="eth0"
    ConfigFileName=$NETWORKCONFIGFILESDIR/ifcfg-$name
    
    echo "BOOTPROTO='dhcp'"  >$ConfigFileName
    echo "MTU='' " >> $ConfigFileName
    echo "REMOTE_IPADDR='' " >>$ConfigFileName
    echo "STARTMODE='onboot'" >>$ConfigFileName

    RefIfcfgFile=$(ls $BACKUP_NETWORKCONFIGFILES/ifcfg-eth* | head -n 1)
    if [ $? -ne 0 ]; then
        # default permissions. Log permissions for debug purpose
        ls -lrt $ConfigFileName
    else
        chown --reference=$RefIfcfgFile $ConfigFileName
        chmod --reference=$RefIfcfgFile $ConfigFileName
    fi
}

function AddHyperVDriversToInitrdModules
{
    newstr=`sed  -n "/^INITRD_MODULES*/p " $mntpath/etc/sysconfig/kernel`
    echo "$newstr"|grep -q "hv_storvsc"
    if [ $? -eq 0 ];then
        echo "HyperV drivers are already present"
    else
        echo "HyperV drivers are not present, adding them to INITRD_MODULES."
        newstr=`echo $newstr|awk -F\" ' {print $1"\""$2 " hv_vmbus hv_storvsc hv_netvsc" "\""}'`
        sed  -i  "s/^INITRD_MODULES=\".*\"/$newstr/"  $mntpath/etc/sysconfig/kernel
    fi
}

function ResetNetworkResolverAndRoutesCache_SLES
{
    # TODO: Remove this code after observing live site data for couple of months.
    if [ -f $mntpath/etc/asr-reset-resolv-conf ]; then
        echo "Resetting resolv.conf file."
        /bin/mv -f $mntpath/etc/resolv.conf $mntpath/etc/resolv.conf"${_INMAGE_ORG_}"
        /bin/touch  $mntpath/etc/resolv.conf
        chown --reference=$mntpath/etc/resolv.conf"${_INMAGE_ORG_}" $mntpath/etc/resolv.conf
        chmod --reference=$mntpath/etc/resolv.conf"${_INMAGE_ORG_}" $mntpath/etc/resolv.conf
    fi
    
    # This is a workaround to skip static routing reset.
    if [ -f $mntpath/etc/asr-skip-routing-reset ]; then
        echo "Skipping the static routing reset."
    else
        /bin/mv -f $mntpath/etc/sysconfig/network/ifroute-eth0 $mntpath/etc/sysconfig/network/ifroute-eth0"${_INMAGE_ORG_}"
        /bin/mv -f $mntpath/etc/sysconfig/network/routes $mntpath/etc/sysconfig/network/routes"${_INMAGE_ORG_}"
    fi
}

_DATE_STRING_=`date +'%b_%d_%H_%M_%S'`
echo "-------------------  $_DATE_STRING_ : Starting post recovery script -------------------"

if [ ! $1 ]
then
    echo "Provide Linux Root Filesystem mount point"
    exit 1
else
    if [ -d $1 ]
    then
        mntpath=$1
        /bin/mount | grep "$mntpath"
        if [ $? -eq 0 ]; then
            echo "Root fs $mntpath is mounted"
        else
            echo "ERROR - Root Filesystem is not mounted at directory $mntpath"
            exit 1
        fi
    else
        echo "ERROR - Provided Linux Root Filesystem mount point is not a directory $mntpath"
        exit 1
    fi
fi

if [ ! $2 ]
then
    echo "Provide working directory"
    exit 1
else
    if [ -d $2 ]
    then
        working_dir=$2
    else
        echo "ERROR - Provided working directory does not exist : $2"
        exit 1
    fi
fi

if [ ! $3 ]
then
    echo "Hydration config settings are empty"
else
    hydration_config_settings=$3
    echo "Hydration config settings: $3"
fi

#Get recovering OS Name
os_details_script=$working_dir/OS_details_target.sh
os_name=`bash $os_details_script $mntpath`
echo "-- OS Information --"
echo "$os_name is being recovered now"
cat $mntpath/etc/*release*

_grub2_efi_path="$mntpath/boot/efi/EFI/"
config_file_name=""
bootloader_folder_path="$mntpath/boot/efi/EFI//boot"

case "$os_name" in
    OL7*|RHEL6*|RHEL7*|RHEL8*)
        _grub2_efi_path="${_grub2_efi_path}redhat"
        ;;
    CENTOS6*|CENTOS7*)
        _grub2_efi_path="${_grub2_efi_path}centos"
        ;;
    SLES12*|SLES15*)
        _grub2_efi_path="${_grub2_efi_path}opensuse"
        ;;
    UBUNTU*)
        _grub2_efi_path="${_grub2_efi_path}ubuntu"
        ;;
    DEBIAN*)
        _grub2_efi_path="${_grub2_efi_path}debian"
        ;;
    *)
        echo "Unidentified OS version - $os_name "
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

echo "-- Starting system reconfig --"

# Menu.lst or grub.cfg configuration changes
case $os_name in
    RHEL5* | OL5* | OL6* | SLES11* | RHEL6* | CENTOS6*)
        ModifyMenuListFile $mntpath
        if [ $? -ne $_SUCCESS_ ]; then
            echo "Error modifying menu.lst file"
            exit 1
        fi
        echo "install ata_piix  { /sbin/modprobe hv_storvsc 2>&1 || /sbin/modprobe --ignore-install ata_piix; }" > $mntpath/etc/modprobe.d/hyperv_pvdrivers.conf
    ;;
    SLES12*|SLES15*)
        _grub_config_options="console=ttyS0 earlyprintk=ttyS0 rootdelay=300"
        ModifyGrubConfig_SLES "$_grub_config_options"
        if [ $? -ne $_SUCCESS_ ]; then
            echo "Error modifying grub.cfg file"
            exit 1
        fi
    ;;
    UBUNTU*)
        ModifyGrubConfig_UBUNTU
        if [ $? -ne $_SUCCESS_ ]; then
            echo "Error modifying grub.cfg file"
            exit 1
        fi
    ;;
    RHEL7*)
        _grub_config_options="rootdelay=300 console=ttyS0 earlyprintk=ttyS0"
        ModifyGrubConfig_RHEL_CENTOS "$_grub_config_options"
        if [ $? -ne $_SUCCESS_ ]; then
            echo "Error modifying grub.cfg file"
            exit 1
        fi
    ;;
    CENTOS7* | OL7*)
        _grub_config_options="rootdelay=300 console=ttyS0 earlyprintk=ttyS0 net.ifnames=0"
        ModifyGrubConfig_RHEL_CENTOS "$_grub_config_options"
        if [ $? -ne $_SUCCESS_ ]; then
            echo "Error modifying grub.cfg file"
            exit 1
        fi
    ;;
    RHEL8* | CENTOS8* | OL8*)
        _grub_config_options="console=tty1 console=ttyS0 earlyprintk=ttyS0 rootdelay=300"
        ModifyGrub_BLS "$_grub_config_options"
        if [ $? -ne $_SUCCESS_ ]; then
            echo "Error modifying grub.cfg file"
            exit 1
        fi
    ;;

    DEBIAN*)
        ModifyGrubConfig_DEBIAN
        if [ $? -ne $_SUCCESS_ ]; then
            echo "Error modifying grub.cfg file"
            exit 1
        fi
    ;;
    *)
        echo "Incorrect input for os to modify grub entries-  $os_name "
        exit 1
        ;;
esac

if [ "$firmware_type" = "UEFI" ]; then
    if [ ! -f "$_grub2_efi_path/bootx64.efi" ]; then
        if [ -f "$_grub2_efi_path/shimx64.efi" ]; then
            CopyFile "$_grub2_efi_path/shimx64.efi" "$_grub2_efi_path/bootx64.efi"
        elif [ -f "$_grub2_efi_path/grubx64.efi" ]; then
            CopyFile "$_grub2_efi_path/grubx64.efi" "$_grub2_efi_path/bootx64.efi"
        elif [ -f "$_grub2_efi_path/grub.efi" ]; then
            CopyFile "$_grub2_efi_path/grub.efi" "$_grub2_efi_path/bootx64.efi"
        else
            if [ ! -d "$bootloader_folder_path" ] || [ ! -f "$bootloader_folder_path/bootx64.efi" ]; then
                echo "efi file is absent on source disk which will lead to boot failure in Gen2 vm."
                exit 1
            fi
        fi
    fi

    if [ -f "$_grub2_efi_path/grub.cfg" ]; then
        config_file_name="grub.cfg"
        CopyFile "$_grub2_efi_path/grub.cfg" "$_grub2_efi_path/bootx64.cfg"
    elif [ -f "$_grub2_efi_path/grub.conf" ]; then
        config_file_name="grub.conf"            
        CopyFile "$_grub2_efi_path/grub.conf" "$_grub2_efi_path/bootx64.conf"
    fi

    # Folder needs to be explicitly added for Ubuntu/Debian/RHEL6/CENTOS6.
    # Checking and adding for other distros too if not added.
    if [ ! -d "$bootloader_folder_path" ]; then
        CopyDirectory "$_grub2_efi_path/" "$bootloader_folder_path"
    else
        if [ ! -f  "$bootloader_folder_path/bootx64.efi" ]; then
            CopyFile "$_grub2_efi_path/bootx64.efi" "$bootloader_folder_path/bootx64.efi"
        fi

        # bootx64.conf required for RHEL6/CENTOS6. Copying for other distros too.
        if [ $config_file_name = "grub.cfg" ] && [ ! -f "$bootloader_folder_path/bootx64.cfg" ]; then
            CopyFile "$_grub2_efi_path/bootx64.cfg" "$bootloader_folder_path/bootx64.cfg"
        elif [ $config_file_name = "grub.conf" ] && [ ! -f "$bootloader_folder_path/bootx64.conf" ]; then
            CopyFile "$_grub2_efi_path/bootx64.conf" "$bootloader_folder_path/bootx64.conf"
        fi
    fi
fi

echo "Rootfs mount point:$mntpath"

# Networking and other configuration changes
case $os_name in
    RHEL5* | RHEL6* | CENTOS6*)
        touch $mntpath/etc/vxagent/setazureip

        # /etc/sysconfig/network file modifications
        # Setting GATEWAYDEV to eth0 to handle multi nic on same subnet.
        ConfigureNetworkFile_CentOS_RHEL "eth0"
        
        # Remove network manager if exist
        RemoveNetworkManager
        
        # Set default NIC settings
        ConfigureDefaultNICForRHEL_CENTOS_OEL
    ;;
    OL5* | OL6*)
        hostname=`grep HOSTNAME  $mntpath/etc/sysconfig/network | awk -F= '{ print $2 }'`
        hostname=`echo $hostname | sed -e 's: ::g'`

        if [ ! ${hostname} ]; then
                hostname=localhost
        fi
        
        touch $mntpath/etc/vxagent/setazureip

        # /etc/sysconfig/network file modifications
        ConfigureNetworkFile_CentOS_RHEL

        # Remove network manager if exist
        RemoveNetworkManager
        
        # Set default NIC settings
        ConfigureDefaultNICForRHEL_CENTOS_OEL
    ;;
    SLES11*)
    
        touch $mntpath/etc/vxagent/setazureip
        
        # Reset network resolver and routing caches.
        ResetNetworkResolverAndRoutesCache_SLES
        
        Rename_INITRD_RAMFS_Files $mntpath/boot/
        
        # Add Hyper-V Drivers o INITRD_MODULES if not present.
        AddHyperVDriversToInitrdModules
        
        # Remove network manager if exist.
        RemoveNetworkManager
        
        # Set default NIC settings.
        ConfigureDefaultNICForSLES
    ;;
    SLES12*|SLES15*)
        touch $mntpath/etc/vxagent/setazureip
        touch $mntpath/etc/vxagent/prepareforazure
        
        # Reset network resolver and routing caches.
        ResetNetworkResolverAndRoutesCache_SLES
        
        # Add Hyper-V Drivers o INITRD_MODULES if not present.
        AddHyperVDriversToInitrdModules
        
        # Remove network manager if exist
        RemoveNetworkManager
        
        # Set default NIC settings
        ConfigureDefaultNICForSLES
    ;;
    UBUNTU14-* | UBUNTU16-*)
        
        touch $mntpath/etc/vxagent/setazureip
        touch $mntpath/etc/vxagent/prepareforazure
        
        RemoveNetworkManager_UBUNTU
        
        ConfigureDefaultNICForUBUNTU
    ;;
    UBUNTU18-* | UBUNTU20-*)
        
        touch $mntpath/etc/vxagent/setazureip
        touch $mntpath/etc/vxagent/prepareforazure
        
        RemoveNetworkManager_UBUNTU
        
        ConfigureDefaultNICForUBUNTUCloudInit
    ;;
    DEBIAN*)
        touch $mntpath/etc/vxagent/setazureip
        touch $mntpath/etc/vxagent/prepareforazure
        
        # Udev rules files are same for UBUNTU and DEBIAN.
        RemoveNetworkManager_UBUNTU
        
        # /etc/network/interfaces file is same for UBUNTU and DEBIAN.
        ConfigureDefaultNICForUBUNTU
        
        # TODO: Use UBUNTU case block for DEBIAN as well, as the changes are same.
    ;;
    RHEL7*|RHEL8*|CENTOS8*|OL8*)
        touch $mntpath/etc/vxagent/setazureip
        touch $mntpath/etc/vxagent/prepareforazure
        
        # Configure /etc/sysconfig/network file
        ConfigureNetworkFile_CentOS_RHEL
        
        # Azure image preparation guidlines for RHEL7.X does not mentioned about removing udev rules.
        # If observer any issue then we need to uncomment below line to remove udev rules.
        # RemovePersistentNetGeneratorRules_CENTOS7
        
        # Set default NIC settings
        ConfigureDefaultNICForRHEL_CENTOS_OEL
    ;;
    CENTOS7* | OL7*)
        touch $mntpath/etc/vxagent/setazureip
        touch $mntpath/etc/vxagent/prepareforazure
        
        # Configure /etc/sysconfig/network file
        ConfigureNetworkFile_CentOS_RHEL
        
        # Remove udev rules
        RemovePersistentNetGeneratorRules_CENTOS7
        
        # Set default NIC settings
        ConfigureDefaultNICForRHEL_CENTOS_OEL
    ;;
    *)
        echo "In correct input for os to modify OS configuration entries-  $os_name "
        exit 1
        ;;
esac

    # Last step to avoid failures. 
    # Check if EnableLinuxGAInstalaltion is set to true;
    enable_linux_guest_agent_config="EnableLinuxGAInstallationDR:true"
    if [[ $hydration_config_settings =~ .*$enable_linux_guest_agent_config.* ]]; then
        InstallLinuxGuestAgent
        if [[ $? -ne 0 ]]; then
            echo "Could not push linux guest agent binaries on target vm."
        fi
    else
        echo "Linux guest agent installation setting is turned off for DR."
        fi

pid=`ps -aef | grep hv_kvp_daemon | grep -v "grep" | awk '{ print $2 }'`
if [ $pid ]; then
    kill -9 $pid
fi

echo "[Sms-Telemetry-Data]:${telemetry_data}"

_DATE_STRING_=`date +'%b_%d_%H_%M_%S'`
echo "------------  $_DATE_STRING_: Finishing post recovery script for $os_name ----------"
exit 0
