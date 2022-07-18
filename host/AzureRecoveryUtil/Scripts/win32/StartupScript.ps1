##+----------------------------------------------------------------------------------+
##            Copyright(c) Microsoft Corp. 2015
##+----------------------------------------------------------------------------------+
## File         :   StartupScript.ps1
##
## Description  :   Start-up script invoked by Azure guest agent when the custom-script
##                  extension is set for hydration-vm as part of recovery work-flow.
##
## History      :   1-6-2015 (Venu Sivanadham) - Created
##+----------------------------------------------------------------------------------+

Param 
(
    [Parameter(Mandatory=$false,Position=1)]
    [string]$hostid="",

    [Parameter(Mandatory=$false,Position=2)]
    [string]$Scenario="recovery",

    [Parameter(Mandatory=$false)]
    [int]$MaxRetryCount = 2,

    [Parameter(Mandatory=$false)]
    [int]$MaxProcessWaitTimeSec = 900,

    [Parameter(Mandatory=$false)]
    [int]$MaxWaitForKillTimeSec = 120,

    [Parameter(Mandatory=$false)]
    [string]$HydrationConfigSettings="emptyconfig"
)

#
# Global Variables declaration
#
$global:Working_Dir = "$env:SystemDrive\AzureRecovery"
$global:Log_File    = "$Working_Dir\AzureRecoveryUtil.log"

#
# Global variables/constants related to config files and executables
#
$global:HostInfoFile_Prefix          = "hostinfo"
$global:HostInfoFile                 = ""
$global:AzureRecoveryInfoFile_Prefix = "azurerecovery"
$global:AzureRecoveryInfoFile        = ""
$global:AzureRecoveryToolsZipFile    = "AzureRecoveryTools.zip"
$global:AzureRecoveryUtil            = "AzureRecoveryUtil.exe"

#
# Status global variables used for updating execution status
#
[string]$global:Exec_Status    = "Processing"
[string]$global:Exec_Task_Desc = "Preparing Environment"
[int]$global:Exec_Error        = 0
[string]$global:Exec_ErrorMsg  = "Preparing the environment for pre-recovery steps execution"
[int]$global:Exec_Progress     = 0

[int]$global:retCode = 0

[string]$global:Scenario_Migration = "migration"
[string]$global:Scenario_Recovery = "recovery"
[string]$global:Scenario_GenConversion = "genconversion"

#
# Trace Log helper functions
#
function WriteToLog ( [string]$msg )
{
    $log_dir = $(Split-Path -Path "$Log_File" -Parent)

    if( $(Test-Path -Path $log_dir -PathType Container) ) 
    {
        $msg | Out-File -Encoding ascii -Width 2048 -Append -Force -FilePath "$Log_File"
    }

    # Write log to standard console as well.
    Write-Host "$msg"
}

function Trace_Error ( [string]$msg )
{
    $trace_time = $(Get-Date -UFormat "%a %b %d %Y %T")

    WriteToLog "$trace_time : $PID : Error : $msg"
}

function Trace ( [string]$msg )
{
    $trace_time = $(Get-Date -UFormat "%a %b %d %Y %T")

    WriteToLog "$trace_time : $PID : $msg"
}

function IsMigration ()
{
    return $Scenario -ieq $global:Scenario_Migration;
}

function IsGenConversion ()
{
    return $Scenario -ieq $global:Scenario_GenConversion;
}

#
# Verify the list of files Azure Guest agent should download and keep in Startup Script current directory
#
function Verify-Downloaded-Files ( [string]$hostid )
{
    #
    # Prepare the downloaded files list and verify them
    #
    $Files = $("")
    if( IsMigration )
    {
        #
        # In case of migration there won't be hostinfo xml file.
        #
        $Files = "$PWD\$global:AzureRecoveryToolsZipFile", "$PWD\$global:AzureRecoveryInfoFile_Prefix-$hostid.conf"
    }
    elseif( IsGenConversion )
    {
        #
        # In case of genconversion there won't be hostinfo xml file.
        #
        $Files = "$PWD\$global:AzureRecoveryToolsZipFile", "$PWD\$global:AzureRecoveryInfoFile_Prefix-$hostid.conf"
    }
    else
    {
        $Files = "$PWD\$global:AzureRecoveryToolsZipFile", "$PWD\$global:AzureRecoveryInfoFile_Prefix-$hostid.conf", "$PWD\$global:HostInfoFile_Prefix-$hostid.xml"
    }

    foreach ( $file in $Files )
    {
        Trace "Verifying the file $file"

        if ( !$(Test-Path "$file" -PathType Leaf) )
        {
            Write-Error "Error verifying file $file"

            return $false
        }
    }

    Trace "All the files are verified"

    return $true
}

#
# Create directory, if it already exist then delete it and then create a new.
#
function Force-Create-Dorectory ( [string]$dir_name )
{
    #
    # Check wheather dir exists
    #
    if ( $(Test-Path -Path "$dir_name" -PathType Container) )
    {
        Write-Host "Directory $dir_name already exist. Deleting it ..."

        Remove-Item $dir_name -Recurse -Force
    }

    #
    # Create the directory
    #
    Write-Host "Creating the directory $dir_name ..."

    New-Item -Path "$dir_name" -ItemType Directory -Force

    return $?
}

#
# Copy config files to working directory
#
function Copy-Files-To-WorkingDir ( $files )
{
    Trace "Copying files to working directory ..."

    foreach ( $file in $files )
    {
        Trace "$file --> $Working_Dir"

        Copy-Item -Path "$file" -Destination "$Working_Dir" -Force
        if( !$? )
        {
            return $false
        }
    }

    Trace "Successfuly copied all the files"

    return $true
}

#
# Extract Executables to working directory
#
function Extract-Executables-To-Dir ( [string]$dir_name )
{
    $zipfile = "$PWD\$global:AzureRecoveryToolsZipFile"

    Trace "Extracting $zipfile to $dir_name"

    $copyflags = [int](
                  4   +   # No progress dlg
                  16  +   # Yes to all
                  512 +   # No conformation for create dir
                  1024)   # No error dlg

    $shell = New-Object -ComObject shell.application

    $ziped_files = $shell.NameSpace($zipfile)
    if ( $ziped_files -eq $null )
    {
        Trace_Error "Could not open the zip file $zipfile"

        return $false
    }

    foreach ( $file in $ziped_files.items() )
    {
        $shell.NameSpace($dir_name).copyhere($file, $copyflags)
        if( !$? )
        {
            Trace_Error "Error extracting files from $zipfile"

            return $false
        }
    }

    Trace "Successfully extracted the files from $zipfile"

    return $true
}

#
# Invoke the Recovery tool to update status
#
function Update-Status-To-Blob ()
{
    $StatusCmdAgrs = @("--operation"       ,"updatestatus",
                       "--recoveryinfofile",$global:AzureRecoveryInfoFile,
                       "--status"          ,$global:Exec_Status,
                       "--errorcode"       ,$global:Exec_Error,
                       "--progress"        ,$global:Exec_Progress,
                       "--taskdescription" ,$global:Exec_Task_Desc,
                       "--errormsg"        ,$global:Exec_ErrorMsg
                       )
    
    Trace "$global:Working_Dir\$global:AzureRecoveryUtil $StatusCmdAgrs"

    &"$global:Working_Dir\$global:AzureRecoveryUtil" $StatusCmdAgrs

    return $?
}

#
# Invke the Recovery tool to upload the log
#
function Upload-Execution-Log ()
{
    $UploadCmdArgs = @("--operation"       ,"uploadlog",
                       "--recoveryinfofile",$global:AzureRecoveryInfoFile,
                       "--logfile"         ,$global:Log_File
                      )
    
    Trace "$global:Working_Dir\$global:AzureRecoveryUtil $UploadCmdArgs"

    &"$global:Working_Dir\$global:AzureRecoveryUtil" $UploadCmdArgs

    return $?
}

#
# Prepare Evnvironment to run the azure pre-recovery steps
#
function Prepare-Environmet ( [string]$hostid )
{
    Write-Host "Preparing Environment ..."

    if ( !$(Force-Create-Dorectory "$Working_Dir") )
    {
        Write-Error "Error creating working directory $global:Working_Dir"

        return $false
    }

    #
    # Start trace messages from here as the working directory is ready.
    #
    if ( !$(Verify-Downloaded-Files $hostid) )
    {
        Write-Error "Error verifying script dependent files. They might be missing"

        return $false
    }

    $copy_files = $("")
    if ( IsMigration )
    {
        $copy_files = $("$PWD\$global:AzureRecoveryInfoFile_Prefix-$hostid.conf")
    }
    elseif ( IsGenConversion )
    {
        $copy_files = $("$PWD\$global:AzureRecoveryInfoFile_Prefix-$hostid.conf")
    }
    else
    {
        $copy_files = $("$PWD\$global:AzureRecoveryInfoFile_Prefix-$hostid.conf", "$PWD\$global:HostInfoFile_Prefix-$hostid.xml")
    }

    if ( !$( Copy-Files-To-WorkingDir $copy_files ) )
    {
        Write-Error "Could not copy config files to working directory"

        return $false
    }
    
    #
    # Update config file-paths golbal variables
    #
    $global:AzureRecoveryInfoFile = "$global:Working_Dir\$global:AzureRecoveryInfoFile_Prefix-$hostid.conf"
    $global:HostInfoFile          = "$global:Working_Dir\$global:HostInfoFile_Prefix-$hostid.xml"
    #
    # Extract the executables to working directory from zip file
    #
    if ( !$(Extract-Executables-To-Dir "$global:Working_Dir") )
    {
        Write-Error "Error extracting executables to working directory"

        return $false
    }

    #
    # Move to working directory
    #
    Trace "Changing directory to $global:Working_Dir"
    cd $global:Working_Dir
    if( !$? )
    {
        Write-Error "Can change directory to $global:Working_Dir"

        return $false
    }

    return $true
}

#
# Invoke the Recovery tool to execute pre-recovery steps.
# Caller need to check $global:retCode value for the process exit code.
# If there is any exception in executing the command then $global:retCode set to 1.
# 
function Execute-Recovery-Steps ()
{
    Trace "Starting Pre-Recovery execution steps"

    $RecCmdArgs = $("")
    if ( IsMigration )
    {
        $RecCmdArgs = @("--operation"       , "migration", 
                        "--recoveryinfofile", $global:AzureRecoveryInfoFile,
                        "--workingdir"      , $global:Working_Dir,
                        "--hydrationconfigsettings"  , hydrationConfigSettings
                       )
    }
	elseif ( IsGenConversion )
	{
	    $RecCmdArgs = @("--operation"       , "genconversion", 
                        "--recoveryinfofile", $global:AzureRecoveryInfoFile,
                        "--workingdir"      , $global:Working_Dir,
                        "--hydrationconfigsettings"  , hydrationConfigSettings
                       )
	}
    else
    {
        $RecCmdArgs = @("--operation"       , "recovery", 
                        "--recoveryinfofile", $global:AzureRecoveryInfoFile,
                        "--hostinfofile"    , $global:HostInfoFile,
                        "--workingdir"      , $global:Working_Dir,
                        "--hydrationconfigsettings"  , hydrationConfigSettings
                       )
    }

    Trace "$global:Working_Dir\$global:AzureRecoveryUtil $RecCmdArgs"

    $global:retCode = 1

    try
    {
        for( $retry = 1; ; $retry++) 
        {

            $recProc = Start-Process -FilePath $global:Working_Dir\$global:AzureRecoveryUtil -ArgumentList $RecCmdArgs -PassThru

            # Wait for the process to exit.
            if ( !$recProc.WaitForExit($MaxProcessWaitTimeSec * 1000) )
            {
                Trace "Recovery command did not complete with-in time limit. Killing the process $($recProc.Id) ..."

                # Try to kill the process
                try
                {
                    $recProc.Kill();

                    Trace "Issued kill signal to the process $($recProc.Id)"
                }
                Catch [System.ComponentModel.Win32Exception] 
                {   
                    #Process might be terminating by the time of Kill call. Wait for the process to terminate
                    Trace "Kill has thown Win32Exception. Exception Details: `n$_"
                }
                Catch [System.InvalidOperationException]
                {
                    Trace "Process might have exited by the time of kill. Exception Details: `n$_"
                    if( $recProc.HasExited )
                    {
                        $global:retCode = $recProc.ExitCode
                        break
                    }
                }
                Catch
                {
                    Trace "Unkown exception for the Kill. Exception Details: `n$_"
                }

                # Wait for the process to exit
                $maxKillWait = 0
                while ( !$recProc.HasExited )
                {
                    if ( ++$maxKillWait -ge $MaxWaitForKillTimeSec )
                    {
                        Trace "Process $($recProc.Id) is not getting killed... Failing the operation."
                        return
                    }

                    # Sleep for a second and then check again.
                    Start-Sleep -Seconds 1
                }

                Trace "Process $($recProc.Id) has exited"

                #upload the available log.
                Upload-Execution-Log

                if ( $retry -lt $MaxRetryCount )
                {
                    Trace "Retrying the operation ..."
                }
                else
                {
                    Trace "Reached maximun retry attempts. Failing the operation."
                    break
                }
            }
            else
            {
                $global:retCode = $recProc.ExitCode
                Trace "Recovery command exited with exit-code: $global:retCode"
                break
            }
        }
    }
    Catch
    {
        Trace "Error executing the recovery command. Error details: `n$_"
    }

    return
}

function Main ( )
{
    #
    # hostid validation
    #
    if ( !$hostid -or !$Scenario)
    {
        Write-Error "Argument error: host-id or scenario is missing"

        Write-Host "Usage : StartupScript.ps1 <host-id> <recovery/migration/genconversion> [-MaxRetryCount <value>] [-MaxProcessWaitTimeSec <value>] [-MaxWaitForKillTimeSec <value>]"

        exit 1
    }

    #
    # Prepare Environment
    #
    if( !$(Prepare-Environmet $hostid) )
    {
        Write-Error "Error Preparing Environment."

        exit 1
    }

    #
    # Set Execution status golbal variables
    #
    $global:Exec_Task_Desc = "Initiating $Scenario steps"
    $global:Exec_ErrorMsg  = "Environment was prepared successfully"
    $global:Exec_Progress  = 20

    #
    # Update status
    #
    if ( Update-Status-To-Blob )
    {
        Trace "Successfuly updated prepare-environment status"
    }
    else
    {
        Trace_Error "Error updating prepare-environment status"

        Write-Error "Status update failed"

        #exit 1
    }

    #
    # Start Recovery
    #
    Execute-Recovery-Steps
    if( $global:retCode -ne 0 )
    {
        Trace_Error "Recovery steps execution failed."

        $global:Exec_ErrorMsg  = "Recovery tool exited unexpectedly"
        $global:Exec_Status    = "Failed"
        $global:Exec_Error     = 1
                
        if ( !$(Update-Status-To-Blob) ) { exit 1 }

        if ( !$(Upload-Execution-Log) ) { exit 1 }
    }
}

### Startup script Entry Point ###

Main