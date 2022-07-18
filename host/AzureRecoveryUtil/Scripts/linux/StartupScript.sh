#!/bin/bash

##+----------------------------------------------------------------------------------+
##            Copyright(c) Microsoft Corp. 2015
##+----------------------------------------------------------------------------------+
## File         :   StartupScript.sh
##
## Description  :   Start-up script invoked by Azure guest agent when the custom-script
##                  extension is set for hydration-vm as part of recovery work-flow.
##
## History      :   1-6-2015 (Venu Sivanadham) - Created
##+----------------------------------------------------------------------------------+

#
# Binaries and command
#
export MKDIR=/bin/mkdir
export RM=/bin/rm
export COPY=/bin/cp
export SET_PERMS=/bin/chmod
export UNZIP_CMD=/usr/bin/unzip

#
# Other global variables
#
export LOGFILE=/usr/local/AzureRecovery/AzureRecoveryUtil.log
export WORK_DIR=/usr/local/AzureRecovery
export LINUX_GA_DIR=/usr/local/AzureRecovery/WALinuxAgentASR

export RECOVERY_TOOLS_ZIPFILE="AzureRecoveryTools.zip"
export LINUX_GA_TOOLS_ZIPFILE="master.zip"

export HOSTINFO_PREFIX="hostinfo"
export REC_INFO_PREFIX="azurerecovery"
export AZURE_RECOVERY_UTIL="AzureRecoveryUtil"

#
# Info files global variables
#
export RECOVERY_INFO_FILE=""
export HOSTINFO_FILE=""
export HYDRATION_CONFIG_SETTINGS="emptyconfig"

#
# Status global variable
# 
export EXEC_STATUS="Processing"
export EXEC_TASK_DESC="Preparing Environment"
export EXEC_ERROR="0"
export EXEC_ERROR_MSG="Preparing the environment for pre-recovery steps execution"
export EXEC_PROGRESS="0"

_MAX_RETRY_ATTEMPTS_=10
_TRUE_=1
_FALSE_=0
_HOST_ID_=""
_SCENARIO_=""
_SCENARIO_RECOVERY_="recovery"
_SCENARIO_MIGRATION_="migration"

#
# Trace functions to log the trace messages to log file.
#
function Trace
{
    echo "$(date +"%a %b %d %Y %T") : $1" >> $LOGFILE 

    # Log to the standard console as well.
    echo "$(date +"%a %b %d %Y %T") : $1"
}

function Trace_Error 
{
    Trace "Error : $1"
}

function Trace_Cmd 
{
    Trace " Command: $1"
}

function Verify_User 
{
    if [ "$(id -u)" != "0" ]
    then
        echo "Only root user can run this script." >2
        exit 1
    fi
}

#
# Synopsis    : Create_Dir <directory-path>
#
# Description : Creates a directory if not exist. If it exist then the existing directory will be
#               forcefully deleted and then new one will be created.
#
function Create_Dir 
{
    if [ -z "$1" ] ; then
        echo "Create_Dir : Directory argument missing"
        return 1
    fi
    
    if [ -d $1 ] ; then
        echo "Directory $1 already exist. Deleting it..."
        $RM -rf $1
        if [ $? -ne 0 ] ; then
            echo "Could not clean-up old directory"
            return 1
        fi
    fi
    
    echo  "Creating directory $1"
    
    $MKDIR -p $1 
    if [ $? -ne 0 ] ; then
        echo "Error creating directory. Error $?"
        return 1
    fi
    
    return 0
}

#
# Synopsis    : Change_Dir <target-directory>
#
# Description : Changes the present working directory to target-directory
#               
#
function Change_Dir
{
    if [ "$1" = "" ] ; then
        Trace "Change_Dir : Directory argument missing"
        return 1
    fi
    
    if [ -d $1 ] ; then
        cd $1 >> $LOGFILE 2>&1
        
        if [ $? -ne 0 ] ; then
            Trace "Can not change to directory $1"
            return 1
        fi
    else
        Trace "$1 Is not a directory or it does not exist"
        return 1
    fi
    
    return 0
}

#
# Synopsis    : CopyFiles <file-list> <destination-directory>
#
# Description : Copies the list of files to destination directory
#               
#
function CopyFiles
{
    if [ $# -ne 2 ] ; then
        Trace_Error "CopyFiles Invalid arguments. Argument count: $#"
        return 1
    fi
    
    if [ "$1" = "" ] ; then
        Trace_Error "CopyFiles Invalid argument. Empty file list"
        return 1
    fi
    FileList=$1
    
    if [ "$2" = "" ] ; then
        Trace_Error "CopyFiles Invalid argument. target-directory path is empty"
        return 1
    fi
    DestDir=$2
    
    Trace "Copying files $FileList to $DestDir"
    
    for file in $FileList
    do
        Trace_Cmd "$COPY -f $file $DestDir"
        $COPY -f $file $DestDir >> $LOGFILE 2>&1
        if [ $? -ne 0 ] ; then
            Trace_Error "Can not copy file $file to $DestDir"
            return 1
        fi
    done
}

function Extract_ZipFile_Using_Python
{
    # Create python script to extract the zip file.
    local unzip_py="${WORK_DIR}/unzip.py"
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
    
    # Run python script
    python3 $unzip_py >> $LOGFILE 2>&1

    return $?
}

#
# Synopsis    : Extract_ZipFile <zipfile_path> <target-directory>
#
# Description : Extracts the zip file content to target directory.
#               
#
function Extract_ZipFile
{
    if [ $# -ne 2 ] ; then
        Trace_Error "Extract_ZipFile Invalid arguments. Argument count: $#"
        return 1
    fi
    
    if [ "$1" = "" ] ; then
        Trace_Error "Extract_ZipFile Invalid argument. zipfile path is empty"
        return 1
    fi
    
    if [ "$2" = "" ] ; then
        Trace_Error "Extract_ZipFile Invalid argument. target-directory path is empty"
        return 1
    fi
    
    Trace "Extracting $1 to $2"
    
    if [[ -f $UNZIP_CMD ]]; then    
        Trace_Cmd "$UNZIP_CMD -oq $1 -d $2"
        $UNZIP_CMD -oq $1 -d $2 >> $LOGFILE 2>&1
    else
        Trace "Extracting zip file using python..."
        Extract_ZipFile_Using_Python $1 $2
    fi
    
    if [ $? -ne 0 ] ; then
        Trace_Error "Error extracting $1 to $2"
        return 1
    fi
    
    Trace "Unzip successful"
    return 0
}

#
# Synopsis    : ScanAllScsiHosts
#
# Description : Scans all the hosts buses available to the machine
#
function ScanAllScsiHosts 
{
    for host in /sys/class/scsi_host/host*
    do
        Trace "Scanning the $host ..."
        echo "- - -" > $host/scan
    done
}

#
# Synopsis    : ScanLVM
#
# Description : Scans all the VGs
#
function ScanLVM
{
    # Scan PVs
    Trace "Scanning PVs ..."
    pvscan >> $LOGFILE 2>&1
    
    # Scan LVs
    Trace "Scanning VGs ..."
    vgscan >> $LOGFILE 2>&1
}

#
# Synopsis    : ActivateVGs
#
# Description : Activate all the VGs visible to the system.
#
function ActivateVGs
{
    for vg in $(vgs -o vg_name --noheadings)
    do
        Trace "Activating VG: $vg"
        vgchange -ay "$vg" >> $LOGFILE 2>&1
    done
    
    Trace "VG details:"
    vgs -o vg_name,vg_attr,lv_count >> $LOGFILE 2>&1 #Trace purpose.
}

#
# Synopsis    : VerifyFiles <file-list>
#
# Description : Verifies files existence
#
function VerifyFiles 
{
    if [ $# -eq 0 ] ; then
        Trace_Error "VerifyFiles invalid argument: Argument missing or empty"
        return 1
    fi
    
    for file in $1
    do
        Trace "Verifying file : $file"
        
        if [ -f $file ] ; then
            Trace "File exist"
        else
            Trace_Error "File does not exist"
            return 1
        fi
    done
    
    return 0
}

#
# Synopsis    : EnableExe_Permissions Directory
#
# Description : Enables the execute permissions to all the files in Directory
#
function EnableExe_Permissions 
{
    Trace "Setting execute permissions for the scripts ..."
    
    if [ $# -eq 0 ] ; then
        Trace_Error "EnableExe_Permissions invalid argument: Argument missing or empty"
        return 1
    fi
    
    for file in "$1"/*
    do  
        if [ -f $file ] ; then
            Trace_Cmd "$SET_PERMS +x $file"
            $SET_PERMS +x $file >> $LOGFILE 2>&1
            if [ $? -ne 0 ] ; then
                 return 1
            fi
        else
            Trace_Error "$file is not a regular file or it does not exist"
        fi
    done
    
    Trace "Successfully enabled permissions."
    
    return 0
}

#
# Synopsis    : Update_Status
#
# Description : Invokes recovery utility to update the current status to blob.
#               
#
function Update_Status 
{
    Trace "Updating execution status : $EXEC_STATUS : $EXEC_TASK_DESC"
        
    StatusArgs="--operation updatestatus --recoveryinfofile $RECOVERY_INFO_FILE --status $EXEC_STATUS  --errorcode $EXEC_ERROR --progress $EXEC_PROGRESS"
    StatusCmd="$WORK_DIR/$AZURE_RECOVERY_UTIL"
    
    Trace_Cmd "$StatusCmd  $StatusArgs --taskdescription $EXEC_TASK_DESC --errormsg $EXEC_ERROR_MSG"
    
    $StatusCmd  $StatusArgs "--taskdescription" "$EXEC_TASK_DESC" "--errormsg" "$EXEC_ERROR_MSG" >> $LOGFILE 2>&1
    
    RetCode=$?
    
    if [ $RetCode -ne 0 ] ; then
        Trace_Error "Status update failed. Return code $RetCode"
        return 1
    else
        Trace "Successfully updated the status."
    fi
    
    return 0;
}

#
# Synopsis    : Upload_Log
#
# Description : Invokes recovery utility for uploading the execution log content to status blob
#               
#
function Upload_Log
{
    Trace "Uploading log to status blob ..."
    
    UploadCmd="$WORK_DIR/$AZURE_RECOVERY_UTIL"
    UploadCmdArgs="--operation uploadlog --recoveryinfofile $RECOVERY_INFO_FILE --logfile $LOGFILE"
    
    Trace_Cmd "$UploadCmd $UploadCmdArgs"
    
    $UploadCmd $UploadCmdArgs >> $LOGFILE 2>&1
    if [ $? -ne 0 ] ; then
        Trace_Error "Log upload failed"
        return 1
    else
        Trace "Log upload successful"
    fi
    
    return 0;
}

#
# Synopsis    : Prepare_Env <host-id>
#
# Description : Prepare the environment for pre-recovery steps execution.
#
function Prepare_Env
{
    Trace "Preparing the environment for pre-recovery steps execution ..."

    HostInfoFile="$HOSTINFO_PREFIX-$_HOST_ID_.xml"
    RecInfoFile="$REC_INFO_PREFIX-$_HOST_ID_.conf"
    PWD=$(pwd)
    
    RECOVERY_INFO_FILE="$WORK_DIR/$RecInfoFile"
    HOSTINFO_FILE="$WORK_DIR/$HostInfoFile"
    
    #
    # Extract the zip file to working directory
    #
    Extract_ZipFile "$PWD/$RECOVERY_TOOLS_ZIPFILE" $WORK_DIR
    if [ $? -ne 0 ] ; then
        Trace_Error "Azure recovery files extraction failed."
        return 1
    fi

    #
    # Downlaod and extract Linux Guest Agent files.
    # Timeout: 300 seconds
    #
    
    if curl --silent -LO "https://github.com/Azure/WALinuxAgent/archive/master.zip" -m 300; then
        Trace "Successfully downloaded WALinuxAgent zip file."
        # Extract master.zip into WALinuxAgentASR
        Extract_ZipFile "$PWD/$LINUX_GA_TOOLS_ZIPFILE" $LINUX_GA_DIR
        if [ $? -ne 0 ] ; then
            Trace_Error "Linux guest agent binaries extraction failed."
        else
            # Enable execution permissions on downloaded guest agent files.
            EnableExe_Permissions  $LINUX_GA_DIR
            if [ $? -ne 0 ] ; then
                Trace_Error "Could not enable execute permission for one of the executables/scripts in Linux GA directory".
            fi
        fi        
    else
        Trace_Error "Could not download WALinuxAgent zip file from github."
    fi

    #
    # Set execute permission to the executables/scripts after extraction
    #
    EnableExe_Permissions $WORK_DIR
    if [ $? -ne 0 ] ; then
        Trace_Error "Could not enable execute permission for one of the executables/scripts on working directory."
        return 1
    fi
    
    #
    # Verify the host-info, recovery-info & executables zip file.
    #
    Trace "Verifying extension files..."
    local ext_files="$PWD/$RECOVERY_TOOLS_ZIPFILE $PWD/$RecInfoFile"
    if [ "$_SCENARIO_" = "$_SCENARIO_RECOVERY_" ]; then
        ext_files="$ext_files $PWD/$HostInfoFile"
    fi
    
    VerifyFiles "$ext_files"
    if [ $? -ne 0 ] ; then
        Trace_Error "Required files are missing: $ext_files"
        return 1
    fi
    
    #
    # Copy host-info & recovery-info files to working directory
    #
    Trace "Copying configuration files to working directory ..."
    
    local config_files="$PWD/$RecInfoFile"
    if [ "$_SCENARIO_" = "$_SCENARIO_RECOVERY_" ]; then
        config_files="$config_files $PWD/$HostInfoFile"
    fi
    CopyFiles "$config_files" $WORK_DIR
    if [ $? -ne 0 ] ; then
        Trace_Error "Can not copy configuration files to $WORK_DIR"
        return 1
    fi
    
    #
    # Update Execution Status
    #
    Update_Status

    #
    # Change the directory to working directory
    #
    Trace "Changing current directory to working directory ..."
    Change_Dir $WORK_DIR
    if [ $? -ne 0 ] ; then
        EXEC_ERROR_MSG="Can not change to working directory $WORK_DIR. Error $?"
        EXEC_ERROR="2"
        EXEC_STATUS="Failed"
        
        Trace_Error "$EXEC_ERROR_MSG"
        
        Update_Status
        Upload_Log
        
        return 1
    fi 
    
    #
    # Rescan all scsi host buses
    #
    ScanAllScsiHosts
    
    #
    # Rescan VGs
    #
    ScanLVM
    
    Trace "Successfully prepared the environment."
    
    return 0
}

#
# Synopsis    : StarRecovery
#
# Description : Start recovery utility to execute pre-recovery steps
#               
#
function StarRecovery
{
    #
    # Update status with Initiating recovery status.
    #
    EXEC_TASK_DESC="Initiating recovery steps"
    EXEC_ERROR_MSG="Environment was prepared successfully"
    EXEC_PROGRESS="20"
    
    Update_Status
    #not considering update status return code
    
    Trace "Starting pre-recovery steps ..."
    
    RecCmd="$WORK_DIR/$AZURE_RECOVERY_UTIL"
    RecCmdArgs="--operation recovery --recoveryinfofile $RECOVERY_INFO_FILE --hostinfofile $HOSTINFO_FILE --workingdir $WORK_DIR --hydrationconfigsettings $HYDRATION_CONFIG_SETTINGS"
    
    Trace_Cmd "$RecCmd $RecCmdArgs"
    
    $RecCmd $RecCmdArgs >> $LOGFILE 2>&1
    
    RetCode=$?
    
    if (( $RetCode != 0 )) ; then
        EXEC_ERROR_MSG="Recovery tool exited unexpectedly with error code $RetCode"
        EXEC_ERROR="1"
        EXEC_STATUS="Failed"
        
        Trace_Error "$EXEC_ERROR_MSG"
        
        Update_Status
        Upload_Log
        
        return 1
    fi
    #
    # For recovery operation the tool returns 0 even on recovery failures if it can handle
    # the error status reporting. If its not returning 0 means something wrong happened 
    # where the recovery tool was not able to update status to blob. So on non-zero return
    # code the script should attempt updating failure status to blob.
    #
    Trace "Pre-Recovery steps executed successfully"
    
    return 0;
}

#
# Synopsis    : StartMigration
#
# Description : Start recovery utility to execute migration steps
#               
#
function StartMigration
{
    #
    # Update status with Initiating recovery status.
    #
    EXEC_TASK_DESC="Initiating migration steps"
    EXEC_ERROR_MSG="Environment was prepared successfully"
    EXEC_PROGRESS="20"
    
    Update_Status
    #not considering update status return code
    
    # Activate all the VGs visible to system.
    Trace "Activating VGs ..."
    ActivateVGs
    
    Trace "Starting migration steps ..."
    
    RecCmd="$WORK_DIR/$AZURE_RECOVERY_UTIL"
    RecCmdArgs="--operation migration --recoveryinfofile $RECOVERY_INFO_FILE --workingdir $WORK_DIR --hydrationconfigsettings $HYDRATION_CONFIG_SETTINGS"
    
    Trace_Cmd "$RecCmd $RecCmdArgs"
    
    $RecCmd $RecCmdArgs >> $LOGFILE 2>&1
    
    RetCode=$?
    
    if (( $RetCode != 0 )) ; then
        EXEC_ERROR_MSG="Recovery tool exited unexpectedly with error code $RetCode"
        EXEC_ERROR="1"
        EXEC_STATUS="Failed"
        
        Trace_Error "$EXEC_ERROR_MSG"
        
        Update_Status
        Upload_Log
        
        return 1
    fi

    Trace "Migration steps executed successfully"
    
    return 0;
}

#
# Synopsis    : Usage
#
# Description : Displays script usage
#               
#
function Usage
{
    echo "Usage: $1 <recovery/migration> <host-id>"
}

#
# Synopsis    : ### Main ###
#
# Description : Entry point function which starts the work-flow
#               
#
function Main
{
    if [ $# -eq 1 ] ; then
        # For backward compatibility.
        _SCENARIO_=$_SCENARIO_RECOVERY_
        _HOST_ID_=$1
    elif [ $# -eq 2 ] ; then
        _SCENARIO_=$1
        _HOST_ID_=$2
    elif [ $# -eq 3 ] ; then
        _SCENARIO_=$1
        _HOST_ID_=$2
        HYDRATION_CONFIG_SETTINGS=${3-emptyconfig}
    else
        Usage $0
        exit 1
    fi
    
    Create_Dir "$WORK_DIR"
    if [ $? -ne 0 ] ; then
        echo "Error: Can not create working directory $WORK_DIR"
        exit 1
    fi

    if [[ $HYDRATION_CONFIG_SETTINGS =~ .*$_ENABLE_LINUX_GA_CONFIG_.* ]]; then
        Create_Dir "$LINUX_GA_DIR"
        if [ $? -ne 0 ] ; then
            echo "Error: Can not create linux guest agent directory $LINUX_GA_DIR"
            #Don't fail
        fi
    fi
 
    Verify_User
    
    #
    # Don't use Trace before this point because the log file creation may fail
    # as log file will be created inside the working directory and the working
    # directory has been created just now.
    #
    
    Prepare_Env
    if [ $? -ne 0 ] ; then
        exit 1
    fi
    
    case "$_SCENARIO_" in
        $_SCENARIO_RECOVERY_)
            StarRecovery
            if [ $? -ne 0 ] ; then
                exit 1
            fi
            ;;
        $_SCENARIO_MIGRATION_)
            StartMigration
            if [ $? -ne 0 ] ; then
                exit 1
            fi
            ;;
        *)
            Usage $0
            echo "$_SCENARIO_ is unsupported for Linux Hydration."
            exit 1
            ;;
    esac
    
    #
    # Normal exit
    #
    exit 0
}

#
# Calling Entry point function for script execution
#
Main "$@"